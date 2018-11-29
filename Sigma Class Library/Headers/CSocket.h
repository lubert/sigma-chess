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

#pragma once

#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

enum SOCKETERR {
  socketErr_NoErr = 0,          // No error
  socketErr_MemAlloc,           // Failed allocating memory
  socketErr_OpenInetService,    // Failed opening Internet Services
  socketErr_CreatConfig,        // Failed creating configuration
  socketErr_Bind,               // Failed binding end point
  socketErr_Connect,            // Failed connecting (OTConnect)
  socketErr_AsyncOpen,          // Failed opening end point
  socketErr_CloseProvider,      // Failed closing provider
  socketErr_Read,               // Failed reading/receiving
  socketErr_Send,               // Failed sending
  socketErr_NotRunning,         // Got event while client wasn't running
  socketErr_UnknownEvent,       // Got unknown event
  socketErr_DNRtoAddr,          // Failed converting DNR to IP address
  socketErr_OpenComplete,       // Got an error during open complete
  socketErr_SetBlocking,        // Failed setting blocking mode
  socketErr_BindComplete,       // Got an error during bind complete
  socketErr_Disconnect,         // Got unexpected disconnect error
  socketErr_OrderlyDisconnect,  // Got unexpected error during orderly
                                // disconnect

  socketErr_ErrorCount
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class CSocket {
 public:
  CSocket(CHAR *serverAddr, INT port, BOOL textMode = true);
  virtual ~CSocket(void);

  OSErr SendData(PTR data, ULONG dataLen);
  OSErr SendDataStr(CHAR *dataStr);

  virtual void HandleConnect(void);
  virtual void HandleDisconnect(INT errorCode);
  virtual void ReceiveData(
      PTR data, ULONG dataLen);  // MUST be overridden (if not text mode)
  virtual void ReceiveDataStr(
      CHAR *dataStr);  // MUST be overridden (if text mode)
  virtual void HandleError(SOCKETERR errCode, INT resCode, CHAR *msg);

  BOOL textMode;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

OSStatus CSocket_Init(void);
void CSocket_End(void);
void CSocket_ProcessEvent(void);
