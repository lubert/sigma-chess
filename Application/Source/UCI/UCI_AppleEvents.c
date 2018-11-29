/**************************************************************************************************/
/*                                                                                                */
/* Module  : UCI_APPLEEVENTS.C                                                                    */
/* Purpose : This module implements the interface to the UCI engines via apple events.            */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
  and the following disclaimer in the documentation and/or other materials provided with the 
  distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "UCI_AppleEvents.h"
#include "UCI_ProcessCmd.h"
#include "SigmaApplication.h"

#include "Debug.h"


#define kEventClass      '·UCI'  // UCI event class

#define kEventId_Launch  'laun'  // Launch engine (full engine path in data)
#define kEventId_Quit    'quit'  // Quit engine (same as "quit\n" UCI command)
#define kEventId_Quit2   'qui2'  // Kills background task (id any)
#define kEventId_UCI1    'uci1'  // Generic UCI message from primary task (message string in data)
#define kEventId_UCI2    'uci2'  // Generic UCI message from secondary task (message string in data)
#define kEventId_Swap    'swap'  // Swaps foreground and background task (only used in engine matches)

#define kMaxMsgLen       10000


pascal OSErr AE_XUCI_Handler (const AppleEvent *event, AppleEvent *reply, long refcon);
OSErr XUCI_SendMessage (AEEventID eventID, char *uciMsg, BOOL waitReply);


static OSType        gUCILoaderCreator = '·UCI';
static AEAddressDesc gUCILoaderAddress = {0, NULL};


static UCI_ENGINE_ID currEngineId = uci_NullEngineId;


/**************************************************************************************************/
/*                                                                                                */
/*                                    STARTUP INITIALIZATON                                       */
/*                                                                                                */
/**************************************************************************************************/

void UCI_AEInit (void)
{
   if (! RunningOSX()) return;

   OSErr err;
  	err = AECreateDesc(typeApplSignature, (Ptr)&gUCILoaderCreator, sizeof(gUCILoaderCreator), &gUCILoaderAddress);
   err = AEInstallEventHandler(kEventClass, typeWildCard, NewAEEventHandlerUPP(AE_XUCI_Handler), 0, false);
} // UCI_AEInit


/**************************************************************************************************/
/*                                                                                                */
/*                                    LAUNCH/QUIT UCI LOADER                                      */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_AELaunchLoader (void)
{
   if (! RunningOSX()) return false;

   if (theApp->LaunchApp("UCI Loader.app", true, true) == 0)
      return true;
   if (theApp->LaunchApp(":UCI Support:UCI Loader.app", true, true) == 0)
      return true;

   NoteDialog
   (  nil,
      "Failed Loading UCI",
      "Could not start the 'UCI Loader' application. Please check that it is located in the 'UCI Support' folder, and then restart Sigma Chess...",
      cdialogIcon_Error
   );
   return false;
} // UCI_StartLoader


void UCI_AEQuitLoader (void)
{
   if (! RunningOSX()) return;

   theApp->QuitApp(gUCILoaderCreator);
} // UCI_QuitLoader


/**************************************************************************************************/
/*                                                                                                */
/*                                      LAUNCH/QUIT ENGINE                                        */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_AELaunchEngine (UCI_ENGINE_ID engineId, CHAR *enginePath)
{
   if (! RunningOSX()) return false;

   currEngineId = engineId;
   OSErr err = XUCI_SendMessage(kEventId_Launch, enginePath, true);

   if (debugOn)
   {  CHAR ss[1000];
      Format(ss, "Launching engine : %s\nError code = %d\n", enginePath, err);
      DebugWrite(ss);
   }

   return (err == noErr);
} // UCI_AELaunchEngine


BOOL UCI_AEQuitEngine (UCI_ENGINE_ID engineId)
{
   currEngineId = engineId;
   OSErr err = XUCI_SendMessage(kEventId_Quit, "", false);
   return (err == noErr);
} // UCI_AEQuitEngine


BOOL UCI_AEQuitEngine2 (void)
{
   OSErr err = XUCI_SendMessage(kEventId_Quit2, "", false);
   return (err == noErr);
} // UCI_AEQuitEngine


/**************************************************************************************************/
/*                                                                                                */
/*                                SEND MESSAGES TO LOADER/ENGINE                                  */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_AESendMessage (UCI_ENGINE_ID engineId, CHAR *msg)  // Msg must be newline terminated
{
   currEngineId = engineId;
   OSErr err = XUCI_SendMessage(kEventId_UCI1, msg, false);
   return (err == noErr);
} // UCI_AESendMessage


/**************************************************************************************************/
/*                                                                                                */
/*                                         SWAP ENGINES                                           */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_AESwapEngine (void)
{
   currEngineId = uci_NullEngineId;
   OSErr err = XUCI_SendMessage(kEventId_Swap, "", false);
   return (err == noErr);
} // UCI_AESendMessage


/**************************************************************************************************/
/*                                                                                                */
/*                              LOW LEVEL APPLE EVENT HANDLING                                    */
/*                                                                                                */
/**************************************************************************************************/

UCI_ENGINE_ID UCI_AEGetCurrent (void)
{
   return currEngineId;
} // UCI_AEGetCurrent

//--------------------------------- UCI Apple Event Handler ---------------------------------------

pascal OSErr AE_XUCI_Handler (const AppleEvent *event, AppleEvent *reply, long refcon)
{
	char		  msg[kMaxMsgLen + 1];
	DescType   eventID, typeCode;
	DescType	  actualType;
	Size 		  actualSize;
	OSErr 	  err = noErr;

   // Get the event ID ('ucis')
	err = AEGetAttributePtr(event, keyEventIDAttr, typeType, &actualType, (Ptr) &eventID, sizeof(eventID), &actualSize);
	if (err != noErr) return err;

   if (eventID == kEventId_UCI1)  //### Ignore messages from engine2??? (unless 'quit'?)
   {
      // Get string parameter
      msg[0] = 0;
      err = AEGetParamPtr(event, keyDirectObject, typeChar, &typeCode, msg, kMaxMsgLen, &actualSize);
      if (err != noErr) return err;
      msg[actualSize] = 0;

//    if (debugOn) { DebugWriteNL("--- UCI APPLE EVENT ---"); DebugWrite(msg); }

      if (IsNewLine(msg[actualSize - 1]))
         msg[actualSize - 1] = 0;
      UCI_ProcessEngineMsg(currEngineId, msg);
   }
  
   return noErr;
} // AE_XUCI_Handler

//------------------------------------ Send Apple Events ---------------------------------------

OSErr XUCI_SendMessage (AEEventID eventID, char *uciMsg, BOOL waitReply)
{
   OSErr      err;
	AEDesc     stringDesc = {0, NULL};
   AppleEvent aeEvent = {0, NULL};
   AppleEvent aeReply = {0, NULL};

	err = AECreateAppleEvent(kEventClass, eventID, &gUCILoaderAddress, kAutoGenerateReturnID, kAnyTransactionID, &aeEvent);
	if (err != noErr) goto exit;

   err = AECreateDesc(typeChar, uciMsg, strlen(uciMsg), &stringDesc);
   if (err != noErr) goto exit;

   err = AEPutParamDesc(&aeEvent, keyDirectObject, &stringDesc);
   if (err != noErr) goto exit;

	err = AESend(&aeEvent, &aeReply, kAENoReply, kAENormalPriority, kAEDefaultTimeout, NULL, NULL);

exit:
	if (stringDesc.dataHandle) AEDisposeDesc(&stringDesc);
	if (aeEvent.dataHandle) AEDisposeDesc(&aeEvent);
	if (aeReply.dataHandle) AEDisposeDesc(&aeReply);

   return err;
} // XUCI_SendMessage
