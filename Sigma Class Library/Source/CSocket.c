/**************************************************************************************************/
/*                                                                                                */
/* Module  : CSocket.c */
/* Purpose : This implements a TCP/IP socket layer. */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CSocket.h"
#include "CApplication.h"

// <OpenTransport.h>

enum SOCKET_EVENT_TYPE {
  socketEvent_Null = 0,
  socketEvent_Connect = 1,
  socketEvent_Disconnect = 2,
  socketEvent_Message = 3,
  socketEvent_Error = 4
};

typedef struct {
  SOCKET_EVENT_TYPE type;  // Connect, disconnect, message received, errors
  SOCKETERR errCode;       // Only if error
  INT resCode;             // Only if error
  PTR data;                // Data pointer (message or error text)
  ULONG dataLen;           // Length of data (including trailing 0 if char data)

  OTLink link;  // OT LIFO link (private use only)
} SOCKET_EVENT;

enum  // Client states
{ kClientStopped = 0,
  kClientRunning = 1,
  kClientShuttingDown = 2 };

enum  // Bit numbers in EP_INFO stateFlags fields
{ kOpenInProgressBit = 0 };

typedef struct EP_INFO {
  EndpointRef erf;       // actual endpoint
  struct EP_INFO *next;  // used to link all acceptor's EPInfos (not atomic)
  OTLink link;           // link into an OT LIFO (atomic)
  UInt8 stateFlags;      // various status fields
} EP_INFO;

CSocket *csocket = NULL;
EP_INFO *gCurrEP = NULL;

EP_INFO *gDNS = NULL;
EP_INFO *gConnectors = NULL;
InetHostInfo gServerHostInfo;
Boolean gWaitForServerAddr = false;
int gClientState = kClientStopped;
CHAR gServerAddrStr[256];
long gServerAddr = 0;
long gServerPort = 0;
long gMaxConnections = 1;
SInt32 gCntrEndpts = 0;
SInt32 gCntrIdleEPs = 0;
SInt32 gCntrBrokenEPs = 0;
SInt32 gCntrPending = 0;
SInt32 gCntrConnections = 0;
SInt32 gCntrTotalConnections = 0;
SInt32 gCntrDiscon = 0;
OTLIFO gIdleEPLIFO;
OTLIFO *gIdleEPs = &gIdleEPLIFO;
OTLIFO gBrokenEPLIFO;
OTLIFO *gBrokenEPs = &gBrokenEPLIFO;
OTConfiguration *gCfgMaster = NULL;

/*----------------------------------------- Local Routines
 * ---------------------------------------*/

static void StartClient(void);
static void StopClient(void);
static BOOL EPOpen(EP_INFO *epi, OTConfiguration *cfg);
static BOOL EPClose(EP_INFO *epi);
static void DoBind(EP_INFO *epi);
static void DoConnect(EP_INFO *epi);
static void ReadData(EP_INFO *epi);
static void SendData(EP_INFO *epi, void *data, ULONG dataLen);
static void Recycle(void);

static void PostError(SOCKETERR errorCode, INT resultCode, CHAR *str);
static void PostSocketEvent(SOCKET_EVENT *event);
static void ProcessSocketEvents(void);

static pascal void Notifier(void *context, OTEventCode event, OTResult result,
                            void *cookie);

/**************************************************************************************************/
/*                                                                                                */
/*                                           EXTERNAL API */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Initialize Socket Layer
 * ----------------------------------*/
// Called at startup BEFORE any other Open Transport calls.

OSStatus CSocket_Init(void) {
  return ::InitOpenTransportInContext(kInitOTForApplicationMask, nil);
} /* CSocket_Init */

/*------------------------------------- Terminate Socket Layer
 * -----------------------------------*/
// Called at termination AFTER all other Open Transport calls.

void CSocket_End(void) { ::CloseOpenTransportInContext(nil); } /* CSocket_End */

/*-------------------------------------- Socket Event Handling
 * -----------------------------------*/
// This routine is called once during each pass through the program's event
// loop. If the program is running on OT 1.1.2 or an earlier release, this is
// where outbound orderly releases are started (see comments in
// DoSndOrderlyRelease for more information on that). This is also where
// endpoints are "fixed" by closing them and opening a new one to replace them.
// This is rarely necessary, but works around some timing issues in OTUnbind().
// Having passed through the event loop once, we assume it is safe to turn off
// throttle-back. And, finally, if we have deferred handing of a T_LISTEN, here
// we start it up again.

void CSocket_ProcessEvent(void)  // Called once per event loop by CApplication
                                 // if EnableSocketEvents(true)
{
  ProcessSocketEvents();

  if ((theApp->running && gClientState == kClientShuttingDown) ||
      (!theApp->running && gClientState != kClientStopped)) {
    if (csocket)
      delete csocket;
    else
      StopClient();
  } else if (theApp->running && gClientState == kClientRunning) {
    Recycle();
    DoConnect(NULL);
  }
} /* CSocket_ProcessEvent */

/**************************************************************************************************/
/*                                                                                                */
/*                                      MESSAGE QUEUE HANDLING */
/*                                                                                                */
/**************************************************************************************************/

// Because the notifier can be called at interrupt time, we can't send events
// directly for processing by the Socket class handler. Instead we need to put
// all events in the event queue (using the "interrupt-safe" OpenTransport APIs.
// During the main application event loop, we then dequeue and process any
// outstanding events.

OTLIFO gEventQueue;
OTLIFO *gEventQueuePtr = &gEventQueue;
UInt8 eventSem = 0;

// This event structure should reside on the local stack of the caller

static void PostSocketEvent(SOCKET_EVENT *event) {
  if (!event->data) event->dataLen = 0;
  SOCKET_EVENT *memEvent =
      (SOCKET_EVENT *)OTAllocMemInContext(sizeof(SOCKET_EVENT), NULL);
  OTMemcpy(memEvent, event, sizeof(SOCKET_EVENT));
  OTLIFOEnqueue(gEventQueuePtr, &memEvent->link);
} /* PostSocketEvent */

static void ProcessSocketEvents(void) {
  if (OTAtomicTestBit(&eventSem, 0)) return;  // Avoid recursive calls in here

  OTAtomicSetBit(&eventSem, 0);

  // First we atomically remove ("steal") all pending messages in the queue.
  // Then we reverse it into a FIFO list (so messages can be processed
  // chronologically).
  OTLink *eventList = OTLIFOStealList(gEventQueuePtr);  // Atomic
  eventList = OTReverseList(eventList);

  // Run through FIFO event queue and process and deallocate each message
  OTLink *link;
  while ((link = eventList) != NULL) {
    // Fetch next event
    eventList = link->fNext;
    SOCKET_EVENT *event = OTGetLinkObject(link, SOCKET_EVENT, link);

    // Dispatch to socket object event handler
    if (csocket) switch (event->type) {
        case socketEvent_Connect:
          csocket->HandleConnect();
          break;
        case socketEvent_Disconnect:
          csocket->HandleDisconnect(event->resCode);
          break;
        case socketEvent_Message:
          if (csocket->textMode)
            csocket->ReceiveDataStr((CHAR *)(event->data));
          else
            csocket->ReceiveData(event->data, event->dataLen);
          break;
        case socketEvent_Error:
          csocket->HandleError(event->errCode, event->resCode,
                               (CHAR *)(event->data));
          break;
      }

    // Release event data
    if (event->data) OTFreeMem(event->data);
    OTFreeMem(event);
  }

  OTAtomicClearBit(&eventSem, 0);
} /* ProcessSocketEvents */

/**************************************************************************************************/
/*                                                                                                */
/*                                     CSOCKET CLASS INTERFACE */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Constructor/Destructor
 * -----------------------------------*/

CSocket::CSocket(CHAR *serverAddr, INT port, BOOL pTextMode) {
  csocket = this;

  textMode = pTextMode;
  CopyStr(serverAddr, gServerAddrStr);
  gServerPort = port;

  ::StartClient();
  theApp->EnableSocketEvents(true);
} /* CSocket::CSocket */

CSocket::~CSocket(void) {
  theApp->EnableSocketEvents(false);
  csocket = nil;
  ::StopClient();
} /* CSocket::~CSocket */

/*------------------------------------- Send Data to Server
 * --------------------------------------*/

OSErr CSocket::SendData(PTR data, ULONG dataLen) {
  ::SendData(gCurrEP, data, dataLen);
  return 0;
} /* CSocket::SendData */

OSErr CSocket::SendDataStr(CHAR *dataStr) {
  return SendData((PTR)dataStr, StrLen(dataStr));
} /* CSocket::SendDataStr */

/*----------------------------------- Receive Data from Server
 * -----------------------------------*/
// Virtual methods one of which MUST be overridden.

void CSocket::ReceiveData(PTR data, ULONG dataLen) {} /* CSocket::ReceiveData */

void CSocket::ReceiveDataStr(CHAR *dataStr) {} /* CSocket::ReceiveDataStr */

/*-------------------------------------- Notification Events
 * -------------------------------------*/
// This reports various notification events back to the client. Should be
// overridden.

void CSocket::HandleConnect(void) {} /* CSocket::HandleConnect */

void CSocket::HandleDisconnect(INT errorCode) {} /* CSocket::HandleDisconnect */

void CSocket::HandleError(SOCKETERR errCode, INT resCode, CHAR *msg) {
} /* CSocket::HandleError */

/**************************************************************************************************/
/*                                                                                                */
/*                                       LOW LEVEL START/STOP */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------ Start Client
 * ----------------------------------------*/
// Open one InetServices (DNS) object, and as many connection endpoints as the
// program will use. Start making connections as soon as the server's name is
// translated to an IP address.

static void StartClient(void) {
  EP_INFO *epi;
  OSStatus err;

  gCntrEndpts = 0;
  gCntrPending = 0;
  gCntrConnections = 0;
  gCntrBrokenEPs = 0;
  gCntrTotalConnections = 0;
  gIdleEPs->fHead = NULL;
  gBrokenEPs->fHead = NULL;
  gClientState = kClientRunning;
  gWaitForServerAddr = true;
  gMaxConnections = 1;
  gEventQueuePtr->fHead = NULL;

  // Open an InternetServices so we have access to the DNR to translate the
  // server's name into an IP address (if necessary).

  gDNS = (EP_INFO *)NewPtr(sizeof(EP_INFO));
  if (gDNS == NULL) {
    PostError(socketErr_MemAlloc, 0,
              "StartClient: Failed allocating memory for EP_INFO");
    return;
  }

  OTMemzero(gDNS, sizeof(EP_INFO));
  OTAtomicSetBit(&gDNS->stateFlags, kOpenInProgressBit);
  err = OTAsyncOpenInternetServicesInContext(
      kDefaultInternetServicesPath, 0, NewOTNotifyUPP(Notifier), gDNS, nil);
  if (err != kOTNoError) {
    OTAtomicClearBit(&gDNS->stateFlags, kOpenInProgressBit);
    PostError(socketErr_OpenInetService, err, "OTAsyncOpenInternetServices");
    return;
  }

  // Get memory for EP_INFO structures

  for (INT i = 0; i < gMaxConnections; i++) {
    epi = (EP_INFO *)NewPtr(sizeof(EP_INFO));
    if (epi == NULL) {
      PostError(socketErr_MemAlloc, 0,
                "StartClient: Failed allocating memory for EP_INFO");
      return;
    }

    OTMemzero(epi, sizeof(EP_INFO));
    epi->next = gConnectors;
    gConnectors = epi;
  }

  // Open endpoints which can be used for outbound connections to the server.

  gCfgMaster = OTCreateConfiguration("tcp");
  if (gCfgMaster == NULL) {
    PostError(socketErr_CreatConfig, 0, "OTCreateConfiguration");
    return;
  }

  for (epi = gConnectors; epi != NULL; epi = epi->next) {
    if (!EPOpen(epi, OTCloneConfiguration(gCfgMaster))) break;
  }
} /* StartClient */

/*------------------------------------------- Stop Client
 * ----------------------------------------*/
// This is where the client is shut down, either because the user clicked the
// stop button, or because the program is exiting (error or quit). The tricky
// part is that we can't quit while there are outstanding OTAsyncOpenEndpoint
// calls (which can't be cancelled, by the way).

static void StopClient(void) {
  EP_INFO *epi, *last;

  gClientState = kClientShuttingDown;

  // First, make sure the DNS is closed.

  if (gDNS != NULL) {
    if (!EPClose(gDNS)) return;
    DisposePtr((char *)gDNS);
    gDNS = NULL;
  }

  // Start closing connector endpoints. While we could be rude and just close
  // the endpoints, we try to be polite and wait for all outstanding connections
  // to finish before closing the endpoints. The is a bit easier on the server
  // which won't end up keeping around control blocks for dead connections which
  // it doesn't know are dead.  Alternately, we could just send a disconnect,
  // but this seems cleaner.

  epi = gConnectors;
  last = NULL;
  while (epi != NULL) {
    if (!EPClose(epi)) {
      last = epi;  // Can't close this endpoint yet, so skip it.
      epi = epi->next;
      continue;
    } else {
      if (last != NULL) {
        last->next = epi->next;
        DisposePtr((char *)epi);
        epi = last->next;
      } else {
        gConnectors = epi->next;
        DisposePtr((char *)epi);
        epi = gConnectors;
      }
    }
  }

  // If the list is empty now, then all endpoints have been successfully closed,
  // so the client is stopped now. At this point we can either restart it or
  // exit the program safely.

  if (gConnectors == NULL) {
    gClientState = kClientStopped;
    gCntrEndpts = 0;
    gCntrIdleEPs = 0;
    gCntrPending = 0;
    gCntrConnections = 0;
    gCntrBrokenEPs = 0;
    gCntrTotalConnections = 0;
    gIdleEPs->fHead = NULL;
    gBrokenEPs->fHead = NULL;
    OTDestroyConfiguration(gCfgMaster);
  }

  // Finally release any memory still allocated to the event queue
  csocket = nil;
  ProcessSocketEvents();  // Doesn't process when csocket = nil, but simply
                          // removes and deallocates any events still hanging in
                          // the event queue (normally this shouldn't happen
                          // though).
} /* StopClient */

/**************************************************************************************************/
/*                                                                                                */
/*                                     LOW LEVEL NETWORKING CODE */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------------------- DoBind
 * ------------------------------------------*/
// This routine requests a wildcard port binding from the transport protocol.
// Since the program doesn't care what port is returned, it passes in NULL for
// the bind return parameter. The bind request structure is ephemeral and can be
// a local stack variable since OT is done with it when the call returns. The
// bind is done when the notifier receives a T_BINDCOMPLETE event.

static void DoBind(EP_INFO *epi) {
  OSStatus err;
  TBind bindReq;
  InetAddress inAddr;

  // Bind the endpoint to a wildcard address (assign us a port, we don't care
  // which one).
  OTInitInetAddress(&inAddr, 0, 0);
  bindReq.addr.len = sizeof(InetAddress);
  bindReq.addr.buf = (unsigned char *)&inAddr;
  bindReq.qlen = 0;

  err = OTBind(epi->erf, &bindReq, NULL);
  if (err != kOTNoError) PostError(socketErr_Bind, err, "OTBind");
} /* DoBind */

/*-------------------------------------------- DoConnect
 * -----------------------------------------*/
// This routine attempts to establish a new connection to the globally known
// server address and port. If the program is still trying to use the DNR to
// resolve the server's host name into an IP address, the endpoint is queued for
// later connection.

static void DoConnect(EP_INFO *epi) {
  OSStatus err;
  TCall sndCall;
  InetAddress inAddr;

  // Don't want new connections if already shutting down.
  if (!theApp->running || gClientState != kClientRunning) return;

  if (gWaitForServerAddr) {
    if (epi != NULL) {
      OTLIFOEnqueue(gIdleEPs, &epi->link);
      OTAtomicAdd32(1, &gCntrIdleEPs);
    }
    return;
  }

  // If we weren't passed a specific EP_INFO, try to get an idle one.
  if (epi == NULL) {
    OTLink *link = OTLIFODequeue(gIdleEPs);
    if (link == NULL) return;
    OTAtomicAdd32(-1, &gCntrIdleEPs);
    epi = OTGetLinkObject(link, EP_INFO, link);
  }

  OTInitInetAddress(&inAddr, gServerPort, gServerAddr);
  OTMemzero(&sndCall, sizeof(TCall));
  sndCall.addr.len = sizeof(InetAddress);
  sndCall.addr.buf = (unsigned char *)&inAddr;

  OTAtomicAdd32(1, &gCntrPending);
  err = OTConnect(epi->erf, &sndCall, NULL);
  if (err != kOTNoDataErr) {
    OTAtomicAdd32(-1, &gCntrPending);
    CHAR errMsg[100];
    Format(errMsg, "OTConnect(state %d)", OTGetEndpointState(epi->erf));
    PostError(socketErr_Connect, err, errMsg);
  }

  // If OTConnect didn't return an error, this thread will resume when the
  // notifier gets a T_CONNECT event...
} /* DoConnect */

/*--------------------------------------------- EPOpen
 * -------------------------------------------*/
// A front end to OTAsyncOpenEndpoint. A status bit is set so we know there is
// an open in progress. It is cleared when the notifier gets a T_OPENCOMPLETE
// where the context pointer is this EP_INFO. Until that happens, this EP_INFO
// can't be cleaned up and released.

static BOOL EPOpen(EP_INFO *epi, OTConfiguration *cfg) {
  OSStatus err;

  OTAtomicSetBit(&epi->stateFlags, kOpenInProgressBit);
  err = OTAsyncOpenEndpointInContext(cfg, 0, NULL, NewOTNotifyUPP(Notifier),
                                     epi, (OTClientContextPtr)0);
  if (err != kOTNoError) {
    OTAtomicClearBit(&epi->stateFlags, kOpenInProgressBit);
    PostError(socketErr_AsyncOpen, err, "OTAsyncOpenEndpoint");
    return false;
  }
  return true;
} /* EPOpen */

/*--------------------------------------------- EPClose
 * ------------------------------------------*/
// This routine is a front end to OTCloseProvider. Centralizing closing of
// endpoints makes debugging and instrumentation easier.

static BOOL EPClose(EP_INFO *epi) {
  OSStatus err;

  // If an endpoint is still being opened, we can't close it yet.
  // There is no way to cancel an OTAsyncOpenEndpoint, so we just
  // have to wait for the T_OPENCOMPLETE event at the notifier.
  //
  if (OTAtomicTestBit(&epi->stateFlags, kOpenInProgressBit)) return false;

  err = OTCloseProvider(epi->erf);
  epi->erf = NULL;
  if (err) PostError(socketErr_CloseProvider, err, "OTCloseProvider");

  if (epi != gDNS) OTAtomicAdd32(-1, &gCntrEndpts);
  return true;
} /* EPClose */

/**************************************************************************************************/
/*                                                                                                */
/*                                         READ/WRITE DATA */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------- Read Data
 * ------------------------------------------*/
// This routine attempts to read all available data from an endpoint. Since this
// routine is only called from inside the notifier in the current version of
// CSocket, it is not necessary to program to handle getting back a T_DATA
// notification DURING an OTRcv() call, as would be the case if we read from
// outside the notifier. We must read until we get a kOTNoDataErr in order to
// clear the T_DATA event so we will get another notification of T_DATA in the
// future.
//
// Currently this application uses no-copy receives to get data.  This obligates
// the program to return the buffers to OT asap.  Since this program does
// nothing with data other than count it, that's easy.  Future, more complex
// versions of this program will do more interesting things with regards to
// that.

static void ReadData(EP_INFO *epi) {
  OTBuffer *bp;
  OTResult res;
  OTFlags flags;
  ULONG dataLen = 0;  // Number of bytes read so far
  CHAR buf[30000];

  while (true) {
    res = OTRcv(epi->erf, &bp, kOTNetbufDataIsOTBufferStar, &flags);

    //--- CASE 1 : Still more data to read ---
    // Append the new data to the read buffer and then continue calling OTRcv in
    // next loop iteration.

    if (res > 0) {
      if (csocket) {
        if (csocket->textMode) {
          for (INT i = 0; i < bp->fLen; i++)
            if (bp->fData[i] != '\r') buf[dataLen++] = bp->fData[i];
        } else {
          for (INT i = 0; i < bp->fLen; i++) buf[dataLen++] = bp->fData[i];
        }
      }

      OTReleaseBuffer(bp);
      continue;
    }
    //--- CASE 2 : All data from message has been read -> Return to receiver ---
    // Since ReadData is only called from inside the notifier we don't have to
    // worry about having missed a T_DATA during the OTRcv.

    if (res == kOTNoDataErr) {
      if (csocket) {
        if (csocket->textMode) buf[dataLen++] = 0;

        SOCKET_EVENT event;
        event.type = socketEvent_Message;
        event.errCode = socketErr_NoErr;
        event.resCode = kOTNoDataErr;
        event.data = (PTR)OTAllocMemInContext(dataLen, NULL);
        event.dataLen = dataLen;
        OTMemcpy(event.data, buf, dataLen);
        PostSocketEvent(&event);
      }

      return;
    }
    //--- CASE 3 : A possible "look" error occured -> Check result code ---
    else if (res == kOTLookErr) {
      res = OTLook(epi->erf);
      if (res == T_ORDREL)
        return;
      else if (res == T_GODATA)
        continue;
      else
        PostError(socketErr_Read, res, "OTRcv/OTLookErr");
    }
    //--- CASE 4 : An unexpected error occured -> Nofify handler ---
    else if (res <= 0) {
      PostError(socketErr_Read, res, "OTRcv");
    }
  }
} /* ReadData */

/*------------------------------------------- Send Data
 * ------------------------------------------*/
// Send data to server.

static void SendData(EP_INFO *epi, void *data, ULONG dataLen) {
  OTResult res;

  res = OTSnd(epi->erf, data, dataLen, 0);

  // This is bogus and needs to add flow control. ???###
  // The only reason we get away with it here is because flow control
  // will never happen in the first 128 bytes sent, and that is all
  // we are sending.

  if (res != dataLen) PostError(socketErr_Send, res, "OTSnd");
} /* SendData */

/*-------------------------------------------- Recycle
 * -------------------------------------------*/
// This routine shouldn't be necessary, but it is helpful to work around both
// problems in OpenTransport and bugs in this program.  Basicly, whenever an
// unexpected error occurs which shouldn't be fatal to the program, the EP_INFO
// is queued on the BrokenEP queue.  When recycle is called, once per pass
// around the event loop, it will attempt to close the associated endpoint and
// open a new one to replace it using the same EP_INFO structure. This process
// of closing an errant endpoint and opening a replacement is probably the most
// reliable way to make sure that this program and OpenTransport can recover
// from unexpected happenings in a clean manner.

static void Recycle(void) {
  OTLink *list = OTLIFOStealList(gBrokenEPs);
  OTLink *link;
  EP_INFO *epi;

  while ((link = list) != NULL) {
    list = link->fNext;
    epi = OTGetLinkObject(link, EP_INFO, link);
    if (!EPClose(epi)) {
      OTLIFOEnqueue(gBrokenEPs, &epi->link);
      continue;
    }
    OTAtomicAdd32(-1, &gCntrBrokenEPs);
    EPOpen(epi, OTCloneConfiguration(gCfgMaster));
  }
} /* Recycle */

/**************************************************************************************************/
/*                                                                                                */
/*                                         LOW LEVEL NOTIFIER */
/*                                                                                                */
/**************************************************************************************************/

// Most of the interesting networking code in this program resides inside this
// notifier. In order to run asynchronously and as fast as possible, things are
// done inside the notifier whenever possible.  Since almost everything is done
// inside the notifier, there was little need for special synchronization code.
//
// Note: The DNR events are combined with normal endpoint events in this
// notifier. The only events which are expected from the DNR are
// T_DNRSTRINGTOADDRCOMPLETE and T_OPENCOMPLETE.
//
// IMPORTANT NOTE:  Normal events defined by XTI (T_LISTEN, T_CONNECT, etc) and
// OT completion events (T_OPENCOMPLETE, T_BINDCOMPLETE, etc.) are not
// reentrant. That is, whenever our notifier is invoked with such an event, the
// notifier will not be called again by OT for another normal or completion
// event until we have returned out of the notifier - even if we make OT calls
// from inside the notifier. This is a useful synchronization tool. However,
// there are two kinds of events which will cause the notifier to be reentered.
// One is T_MEMORYRELEASED, which always happens instantly. The other are state
// change events like kOTProviderWillClose.

static void Handle_T_DNRSTRINGTOADDRCOMPLETE(EP_INFO *epi, OTResult result);
static void Handle_T_OPENCOMPLETE(EP_INFO *epi, OTResult result, void *cookie);
static void Handle_T_BINDCOMPLETE(EP_INFO *epi, OTResult result);
static void Handle_T_CONNECT(EP_INFO *epi, OTResult result);
static void Handle_T_DATA(EP_INFO *epi, OTResult result);
static void Handle_T_GODATA(EP_INFO *epi, OTResult result);
static void Handle_T_DISCONNECT(EP_INFO *epi, OTResult result);
static void Handle_T_ORDREL(EP_INFO *epi, OTResult result);
static void Handle_T_UNBINDCOMPLETE(EP_INFO *epi, OTResult result);

static pascal void Notifier(void *context, OTEventCode event, OTResult result,
                            void *cookie) {
  // Once the program is shutting down, most events would be uninteresting.
  // However, we still need T_OPENCOMPLETE and T_MEMORYRELEASED events since we
  // can't call CloseOpenTransport until all OTAsyncOpenEndpoints and OTSends
  // with AckSends have completed. So those specific events are still accepted.

  if (!theApp->running) {
    if (event != T_OPENCOMPLETE) return;
  }

  // This really isn't necessary, it's just a sanity check which should be
  // removed once a program is debugged. It's just making sure we don't get
  // event notifications after all of our endpoints have been closed.

  if (gClientState == kClientStopped) {
    PostError(socketErr_NotRunning, event, "Notified: Client not running");
    return;
  }

  // Within the notifier, all action is based on the event code. In this
  // notifier, fatal errors all break out of the switch to the bottom. As long
  // as everything goes as expected, the case returns rather than breaks.

  EP_INFO *epi = (EP_INFO *)context;

  switch (event) {
    case T_DNRSTRINGTOADDRCOMPLETE:
      Handle_T_DNRSTRINGTOADDRCOMPLETE(epi, result);
      break;

    case T_OPENCOMPLETE:
      Handle_T_OPENCOMPLETE(epi, result, cookie);
      break;

    case T_BINDCOMPLETE:
      Handle_T_BINDCOMPLETE(epi, result);
      break;

    case T_CONNECT:
      Handle_T_CONNECT(epi, result);
      break;

    case T_DATA:
      Handle_T_DATA(epi, result);
      break;

    case T_GODATA:
      Handle_T_GODATA(epi, result);
      break;

    case T_DISCONNECT:
      Handle_T_DISCONNECT(epi, result);
      break;

    case T_ORDREL:
      Handle_T_ORDREL(epi, result);
      break;

    case T_UNBINDCOMPLETE:
      Handle_T_UNBINDCOMPLETE(epi, result);
      break;

      // There are events which we don't handle, but we don't expect to see
      // any of them.   When running in debugging mode while developing a
      // program, we exit with an informational alert.   Later, in the
      // production version of the program, we ignore the event and try to keep
      // running.

    default:
      PostError(socketErr_UnknownEvent, event, "Notifier: Unknown event");
  }
} /* Notifier */

/*---------------------------------- T_DNRSTRINGTOADDRCOMPLETE
 * -----------------------------------*/
// This event occurs when the DNR has finished an attempt to translate the
// server's name into an IP address we can use to connect to.

static void Handle_T_DNRSTRINGTOADDRCOMPLETE(EP_INFO *epi, OTResult result) {
  if (result != kOTNoError) {
    PostError(socketErr_DNRtoAddr, result,
              "Notifier: T_DNRSTRINGTOADDRCOMPLETE");
    return;
  }
  gServerAddr = gServerHostInfo.addrs[0];
  gWaitForServerAddr = false;
} /* Handle_T_DNRSTRINGTOADDRCOMPLETE */

/*----------------------------------------- T_OPENCOMPLETE
 * ---------------------------------------*/
// This event occurs when an OTAsyncOpenEndpoint() completes. Note that this
// event, just like any other async call made from outside the notifier, can
// occur during the call to OTAsyncOpenEndpoint(). That is, in the main thread
// the program did the OTAsyncOpenEndpoint(), and the notifier is invoked before
// control is returned to the line of code following the call to
// OTAsyncOpenEndpoint(). This is one event we need to keep track of even if we
// are shutting down the program since there is no way to cancel outstanding
// OTAsyncOpenEndpoint() calls.

static void Handle_T_OPENCOMPLETE(EP_INFO *epi, OTResult result, void *cookie) {
  OSStatus err;

  OTAtomicClearBit(&epi->stateFlags, kOpenInProgressBit);
  if (result == kOTNoError)
    epi->erf = (EndpointRef)cookie;
  else {
    PostError(socketErr_OpenComplete, result, "Notifier: T_OPENCOMPLETE");
    return;
  }

  if (!theApp->running) return;

  if (epi == gDNS) {
    err = OTInetStringToAddress((InetSvcRef)epi->erf, gServerAddrStr,
                                &gServerHostInfo);
    if (err != kOTNoError) {  // Can't translate the server address string
      PostError(socketErr_OpenComplete, err,
                "Notifier: T_OPENCOMPLETE - OTInetStringToAddress");
    }
    // DNS resumes at T_DNRSTRINGTOADDRCOMPLETE
  } else {
    OTAtomicAdd32(1, &gCntrEndpts);

    // Set to blocking mode so we don't have to deal with kEAGAIN errors.
    // Async/blocking is the best mode to write an OpenTransport application in.

    err = OTSetBlocking(epi->erf);
    if (err != kOTNoError) {
      PostError(socketErr_SetBlocking, err, "OTSetBlocking");
      return;
    }

    DoBind(epi);
    // resumes at T_BINDCOMPLETE
  }
} /* Handle_T_OPENCOMPLETE */

/*----------------------------------------- T_BINDCOMPLETE
 * ---------------------------------------*/
// This event is returned when an endpoint has been bound to a wildcard addr. No
// errors are expected. The program immediately attempts to establish a
// connection from this endpoint to the server.

static void Handle_T_BINDCOMPLETE(EP_INFO *epi, OTResult result) {
  if (result != kOTNoError)
    PostError(socketErr_BindComplete, result, "Notifier: T_BINDCOMPLETE");
  else
    DoConnect(epi);  // resumes at T_CONNECT
} /* Handle_T_BINDCOMPLETE */

/*------------------------------------------- T_CONNECT
 * ------------------------------------------*/
// This event is returned when a connection is established to the server. The
// program must call OTRcvConnect() to get the connection information and clear
// the T_CONNECT event from the stream. Since OTRcvConnect() returns immediately
// (rather than via a completion event to the notifier) we can use local stack
// structures for parameters.

static void Handle_T_CONNECT(EP_INFO *epi, OTResult result) {
  OSStatus err;
  TCall call;
  InetAddress caddr;

  if (result != kOTNoError) {
    PostError(socketErr_Connect, result, "Notifier: T_CONNECT");
    return;
  }

  call.addr.maxlen = sizeof(InetAddress);
  call.addr.buf = (unsigned char *)&caddr;
  call.opt.maxlen = 0;
  call.opt.buf = NULL;
  call.udata.maxlen = 0;
  call.udata.buf = NULL;

  err = OTRcvConnect(epi->erf, &call);
  if (err != kOTNoError) {
    PostError(socketErr_Connect, err, "Notifier: T_CONNECT - OTRcvConnect");
    return;
  }
  OTAtomicAdd32(-1, &gCntrPending);
  OTAtomicAdd32(1, &gCntrConnections);
  OTAtomicAdd32(1, &gCntrTotalConnections);

  OTMemcpy(&gCurrEP, &epi, sizeof(EP_INFO *));

  // Send "connect" event to client handler
  SOCKET_EVENT event;
  event.type = socketEvent_Connect;
  event.errCode = socketErr_NoErr;
  event.resCode = 0;
  event.data = NULL;
  event.dataLen = 0;
  PostSocketEvent(&event);
} /* Handle_T_CONNECT */

/*--------------------------------------------- T_DATA
 * -------------------------------------------*/
// The main rule for processing T_DATA's is to remember that once you have a
// T_DATA, you won't get another one until you have read to a kOTNoDataErr. The
// advanced rule is to remember that you could get another T_DATA during an
// OTRcv() which will eventually return kOTNoDataErr, presenting the application
// with a synchronization issue to be most careful about.
//
// In this application, since an OTRcv() calls are made from inside the
// notifier, this particular synchronization issue doesn't become a problem.

static void Handle_T_DATA(EP_INFO *epi, OTResult result) {
  ReadData(epi);
} /* Handle_T_DATA */

/*-------------------------------------------- T_GODATA
 * ------------------------------------------*/
// Because of the complexity involved in the implementation of OT flow control,
// it is sometimes possible to receive a T_GODATA even when we aren't subject to
// flow control - normally only at the start of a program. If this happens,
// ignoring it is the correct thing to do.

static void Handle_T_GODATA(EP_INFO *epi, OTResult result) {
} /* Handle_T_GODATA */

/*------------------------------------------ T_DISCONNECT
 * ----------------------------------------*/
// An inbound T_DISCONNECT event usually indicates that the other side of the
// connection did an abortive disconnect (as opposed to an orderly release). It
// also can be generated by the transport provider on the system (e.g. tcp) when
// it decides that a connection is no longer in existance.
//
// We receive the disconnect, but this program ignores the associated reason
// (NULL param). It is possible to get back a kOTNoDisconnectErr from the
// OTRcvDisconnect call. This can happen when either (1) the disconnect on the
// stream is hidden by a higher priority message, or (2) something has flushed
// or reset the disconnect event in the meantime. This is not fatal, and the
// appropriate thing to do is to pretend the T_DISCONNECT event never happened.
// Any other error is unexpected and needs to be reported so we can fix it.
// Next, unbind the endpoint so we can reuse it for a new inbound connection.
//
// It is possible to get an error on the unbind due to a bug in OT 1.1.1 and
// earlier. The best thing to do for that is close the endpoint and open a new
// one to replace it. We do this back in the main thread so we don't have to
// deal with synchronization problems.

static void Handle_T_DISCONNECT(EP_INFO *epi, OTResult result) {
  OSStatus err;
  OTResult epState;

  epState = OTGetEndpointState(epi->erf);
  if (epState == T_OUTCON) OTAtomicAdd32(-1, &gCntrPending);

  OTAtomicAdd32(1, &gCntrDiscon);
  err = OTRcvDisconnect(epi->erf, NULL);
  if (err != kOTNoError) {
    if (err != kOTNoDisconnectErr)
      PostError(socketErr_Disconnect, err,
                "Notifier: T_DISCONNECT - OTRcvDisconnect");
    goto done;
  }

  err = OTUnbind(epi->erf);
  if (err != kOTNoError) {
    OTLIFOEnqueue(gBrokenEPs, &epi->link);
    OTAtomicAdd32(1, &gCntrBrokenEPs);
  }

done:

  // Send "unexpected disconnect" event to client handler
  SOCKET_EVENT event;
  event.type = socketEvent_Disconnect;
  event.errCode = socketErr_Disconnect;
  event.resCode = err;
  event.data = NULL;
  event.dataLen = 0;
  PostSocketEvent(&event);
} /* Handle_T_DISCONNECT */

/*-------------------------------------------- T_ORDREL
 * ------------------------------------------*/
// This event occurs when an orderly release has been received on the stream.

static void Handle_T_ORDREL(EP_INFO *epi, OTResult result) {
  OSStatus err;
  OTResult epState;

  // Send "orderly disconnect" event to client handler
  SOCKET_EVENT event;
  event.type = socketEvent_Disconnect;
  event.errCode = socketErr_NoErr;
  event.resCode = result;
  event.data = NULL;
  event.dataLen = 0;
  PostSocketEvent(&event);

  err = OTRcvOrderlyDisconnect(epi->erf);
  if (err != kOTNoError) {
    PostError(socketErr_OrderlyDisconnect, err,
              "Notifier: T_ORDREL - OTRcvOrderlyDisconnect");
    return;
  }
  epState = OTGetEndpointState(epi->erf);
  if (epState != T_IDLE) return;
  OTAtomicAdd32(-1, &gCntrConnections);
  err = OTUnbind(epi->erf);
  if (err != kOTNoError) {
    OTLIFOEnqueue(gBrokenEPs, &epi->link);
    OTAtomicAdd32(1, &gCntrBrokenEPs);
  }
} /* Handle_T_ORDREL */

/*---------------------------------------- T_UNBINDCOMPLETE
 * --------------------------------------*/
// This event occurs on completion of an OTUnbind(). The endpoint is ready for
// reuse on a new inbound connection. Put it back into the queue of idle
// endpoints. Note that the OTLIFO structure has atomic queue and dequeue, which
// can be helpful for synchronization protection.

// Unbind errors can occur as a result of a bug in OT 1.1.1 and earlier
// versions. The best recovery is to put the endpoint in the broken list for
// recycling with a clean, new endpoint.

static void Handle_T_UNBINDCOMPLETE(EP_INFO *epi, OTResult result) {
  if (result == kOTNoError) {
    DoBind(epi);
  } else {
    OTLIFOEnqueue(gBrokenEPs, &epi->link);
    OTAtomicAdd32(1, &gCntrBrokenEPs);
  }
} /* Handle_T_UNBINDCOMPLETE */

/**************************************************************************************************/
/*                                                                                                */
/*                                           DEBUGGING */
/*                                                                                                */
/**************************************************************************************************/

static void PostError(SOCKETERR errorCode, INT resultCode, CHAR *str) {
  SOCKET_EVENT event;
  event.type = socketEvent_Error;
  event.errCode = errorCode;
  event.resCode = resultCode;
  event.dataLen = OTStrLength(str) + 1;
  event.data = (PTR)OTAllocMemInContext(event.dataLen, NULL);
  OTMemcpy(event.data, str, event.dataLen);
  PostSocketEvent(&event);
} /* PostError */
