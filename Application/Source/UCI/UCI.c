/**************************************************************************************************/
/*                                                                                                */
/* Module  : UCI.C */
/* Purpose : This module implements the interface to the UCI engines. */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this
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

#include "UCI.h"
#include "CDialog.h"
#include "Engine.f"
#include "ExaChessGlue.h"
#include "GameWindow.h"
#include "Pgn.h"
#include "SigmaPrefs.h"
#include "UCI_AppleEvents.h"
#include "UCI_Option.h"
#include "UCI_ProcessCmd.h"
#include "UCI_ProgressDialog.h"

#include "Debug.h"

#define uci_MaxMessageLen 10000

static BOOL uci_enabled = false;

UCI_SESSION UCISession[uci_MaxEngineCount];

// Note: The "UCI Loader" is loaded when Sigma starts up and remains running.

/**************************************************************************************************/
/*                                                                                                */
/*                                              STARTUP */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_Enabled(void) { return RunningOSX() && uci_enabled; }  // UCI_Enabled

static void ClearSession(UCI_ENGINE_ID engineId, BOOL retainOwnerWin);
static void ResetSigmaUCIAbout(void);
static BOOL AutoInstallEngine(CHAR *engineFileName);
// static void AutoInstallHiarcs (void);

void UCI_InitModule(BOOL firstLaunch) {
  ResetSigmaUCIAbout();

  // Clear the session list
  for (UCI_ENGINE_ID i = 0; i < uci_MaxEngineCount; i++) ClearSession(i, false);

  if (!RunningOSX()) return;

  UCI_AEInit();
  uci_enabled = UCI_AELaunchLoader();
  if (!uci_enabled) {
    Prefs.UCI.defaultId = uci_SigmaEngineId;
    return;
  }
  /*
     // Try to auto install Hiarcs engine if first launch
     if (firstLaunch)
        AutoInstallHiarcs();
  */
  // Try to auto install Hiarcs engine if first launch
  if (firstLaunch) {
    AutoInstallEngine("::HIARCS:Hiarcs13.2SP");
    AutoInstallEngine("::HIARCS:Hiarcs13.2MP");
    AutoInstallEngine("::DeepJunior:Deep Junior 12");
    sigmaApp->RebuildEngineMenu();
  }
  // If not first launch, try to launch default UCI engine.
  else if (Prefs.UCI.defaultId != uci_SigmaEngineId && !ExaWindowExists())
    UCI_LoadEngine(Prefs.UCI.defaultId);
}  // UCI_InitModule

static void ClearSession(UCI_ENGINE_ID engineId, BOOL retainOwnerWin) {
  UCI_SESSION *Session = &UCISession[engineId];
  CWindow *ownerWin = (retainOwnerWin ? Session->ownerWin : nil);

  ClearBlock((PTR)Session, sizeof(UCI_SESSION));
  Session->engineId = engineId;
  Session->active = (engineId == uci_SigmaEngineId);
  Session->ownerWin = ownerWin;
  Session->gameId = 0;
}  // ClearSession

/*------------------------------------- Auto Add Hiarcs Engine
 * -----------------------------------*/
/*
static void AutoInstallHiarcs (void)
{
   if (! UCI_Enabled()) return;

        for (INT i = 1; i < Prefs.UCI.count; i++)
                if (EqualStr(Prefs.UCI.Engine[i].name, "HIARCS 13.1 SP"))
return;

   CHAR hiarcsEnginePath[uci_EnginePathLen + 1];

   // Retrieve the HIARCS engine path
   BOOL uhiarcsFound = false;

   CFile *uhiarcsFile = new CFile;
   if (uhiarcsFile->Set(":HIARCS:Hiarcs13.1SP", 'APPL') == fileError_NoError ||
       uhiarcsFile->Set("::HIARCS:Hiarcs13.1SP", 'APPL') == fileError_NoError)
   {
      uhiarcsFound = (uhiarcsFile->GetPathName(hiarcsEnginePath,
uci_EnginePathLen) == fileError_NoError);
   }
   delete uhiarcsFile;

   if (! uhiarcsFound)
   {  NoteDialog(nil, "HIARCS Engine Installation", "Sigma Chess could not find
the HIARCS engine. You can install HIARCS later from the 'Engine Manager' dialog
in the 'Analyze/Engine' menu...", cdialogIcon_Error); return;
   }

   // Ask if use wants to auto install
   if (! QuestionDialog(nil, "HIARCS Engine Installation", "Sigma Chess is ready
to install the HIARCS engine. Do you want to install now? If you click 'Cancel'
you can install later from the 'Engine Manager' dialog in the 'Analyze/Engine'
menu.", "Install", "Cancel")) return;

   // Auto install
   UCI_ENGINE_ID hiarcsId = UCI_AddLocalEngine(hiarcsEnginePath);
   if (hiarcsId == uci_NullEngineId)
   {  NoteDialog(nil, "Failed Loading HIARCS Engine", "An unexpected error
occured during the installation of the HIARCS engine...", cdialogIcon_Error);
   }
   else
   {  if (QuestionDialog(nil, "HIARCS Engine Installed", "The HIARCS engine was
successfully installed! Do you want to use HIARCS or Sigma Chess as the default
chess engine? You can switch engines later from the 'Engine' list in the
'Analyze' menu...", "HIARCS", "Sigma")) Prefs.UCI.defaultId = hiarcsId;
      sigmaApp->RebuildEngineMenu();
   }
} // AutoInstallHiarcs
*/

static BOOL AutoInstallEngine(CHAR *engineFileName) {
  if (!UCI_Enabled() || Prefs.UCI.count == uci_MaxEngineCount) return false;

  // Check if already installed
  for (INT i = 1; i < Prefs.UCI.count; i++)
    if (EqualStr(Prefs.UCI.Engine[i].name, engineFileName)) return false;

  CHAR enginePath[uci_EnginePathLen + 1];
  BOOL engineFound = false;

  CFile *engineFile = new CFile;
  if (engineFile->Set(engineFileName, 'APPL') == fileError_NoError) {
    engineFound = (engineFile->GetPathName(enginePath, uci_EnginePathLen) ==
                   fileError_NoError);
  }
  delete engineFile;

  if (!engineFound) return false;

  // Silent Install (don't launch yet)

  UCI_ENGINE_ID engineId = Prefs.UCI.count;  // Allocate new engine ID
  UCI_INFO *Info = &Prefs.UCI.Engine[engineId];

  //--- Reset engine info ---
  INT n = StrLen(enginePath);
  while (n > 0 && enginePath[n - 1] != '/') n--;  // Strip path from name
  CopyStr(&enginePath[n], Info->name);  // We don't know the name & author yet
  CopyStr("", Info->author);
  CopyStr("", Info->engineAbout);

  //--- Reset engine location ---
  Info->local = true;
  CopyStr(enginePath, Info->path);

  //--- Reset options ---
  Info->optionCount = 0;
  Info->supportsPonder = false;
  Info->supportsLimitStrength = false;
  Info->supportsNalimovBases = false;

  Prefs.UCI.count++;

  return true;
}  // AutoInstallEngine

/**************************************************************************************************/
/*                                                                                                */
/*                                          PREFS HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void UCI_ResetPrefs(void) {
  Prefs.UCI.defaultId = uci_SigmaEngineId;
  Prefs.UCI.count = 1;
  CopyStr("", Prefs.UCI.NalimovPath);

  //--- Reset info for Sigma Engine (although not a UCI engine as such) ---
  UCI_INFO *SigmaInfo = &(Prefs.UCI.Engine[uci_SigmaEngineId]);
  ResetSigmaUCIAbout();

  SigmaInfo->local = true;
  CopyStr("Built-in", SigmaInfo->path);

  SigmaInfo->optionCount = 0;

  SigmaInfo->supportsPonder = true;
  UCI_CreateCheckOption(&SigmaInfo->Ponder, true);

  SigmaInfo->supportsLimitStrength = true;
  SigmaInfo->autoReduce = false;
  UCI_CreateCheckOption(&SigmaInfo->LimitStrength, false);
  UCI_CreateSpinOption(&SigmaInfo->UCI_Elo, 2400, kSigmaMinELO, kSigmaMaxELO);

  SigmaInfo->supportsNalimovBases = false;
}  // UCI_ResetPrefs

static void ResetSigmaUCIAbout(void) {
  UCI_INFO *SigmaInfo = &Prefs.UCI.Engine[uci_SigmaEngineId];
  CopyStr(sigmaAppName, SigmaInfo->name);
  CopyStr("Ole K. Christensen", SigmaInfo->author);
  CopyStr("Copyright (C) 2010, Sigma GameWare - http://www.sigmachess.com",
          SigmaInfo->engineAbout);
}  // SetSigmaUCIPrefs

/**************************************************************************************************/
/*                                                                                                */
/*                                         ADD NEW ENGINE */
/*                                                                                                */
/**************************************************************************************************/

// When a UCI engine is registered (caused by the user clicking the "Add..."
// button in the "UCI Engine Config" dialog), it is launched automatically in
// order to retrieve info about its name, author and options.

/*---------------------------------------- Add Local Engine
 * --------------------------------------*/

UCI_ENGINE_ID UCI_AddLocalEngine(CHAR *enginePath) {
  if (Prefs.UCI.count == uci_MaxEngineCount) return uci_NullEngineId;

  UCI_ENGINE_ID engineId = Prefs.UCI.count;  // Allocate new engine ID
  UCI_INFO *Info = &Prefs.UCI.Engine[engineId];

  //--- Reset engine info ---
  INT n = StrLen(enginePath);
  while (n > 0 && enginePath[n - 1] != '/') n--;  // Strip path from name
  CopyStr(&enginePath[n], Info->name);  // We don't know the name & author yet
  CopyStr("", Info->author);
  CopyStr("", Info->engineAbout);

  //--- Reset engine location ---
  Info->local = true;
  CopyStr(enginePath, Info->path);

  //--- Reset options ---
  Info->optionCount = 0;
  Info->supportsPonder = false;
  Info->supportsLimitStrength = false;
  Info->supportsNalimovBases = false;

  //--- Launch engine ---
  // Launch engine to get info about name, author and options. If this fails,
  // we abort the allocation and return "uci_NullEngineId".
  if (UCI_LoadEngine(engineId)) {
    Prefs.UCI.count++;
    return engineId;
  }

  return uci_NullEngineId;
}  // UCI_AddLocalEngine

/**************************************************************************************************/
/*                                                                                                */
/*                                         REMOVE ENGINE */
/*                                                                                                */
/**************************************************************************************************/

// Removes the engine from the Prefs.UCI.Engine[] list. This will "shift" down
// engines with a higher id (and update the session list accordingly).

void UCI_RemoveEngine(UCI_ENGINE_ID engineId) {
  if (engineId == uci_SigmaEngineId) return;

  // First check if this engine is currently active. If so, stop it first.
  if (UCISession[engineId].active)
    UCI_QuitEngine(engineId);  // Tell engine to quit. Will remain in session
                               // list until socket closed

  // Shift down all trailing engines
  for (INT i = engineId + 1; i < Prefs.UCI.count; i++) {
    Prefs.UCI.Engine[i - 1] = Prefs.UCI.Engine[i];

    UCISession[i - 1] = UCISession[i];
    UCISession[i - 1].engineId = i - 1;
  }

  Prefs.UCI.count--;
  if (Prefs.UCI.defaultId >= Prefs.UCI.count)
    Prefs.UCI.defaultId = Prefs.UCI.count - 1;

  sigmaApp->BroadcastMessage(msg_UCIEngineRemoved, engineId);
}  // UCI_RemoveEngine

/**************************************************************************************************/
/*                                                                                                */
/*                                      LAUNCH/QUIT ENGINE */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Launch Engine
 * ----------------------------------------*/

static BOOL VerifyEnginePath(UCI_INFO *Info);

BOOL UCI_LoadEngine(UCI_ENGINE_ID engineId, BOOL autoQuitPrevious) {
  if (engineId == uci_SigmaEngineId || !UCI_Enabled()) return false;

  BOOL launchedOK = false;
  INT launchErrorCode = 0;
  UCI_SESSION *Session = &UCISession[engineId];

  //--- Exit if already active/launched ---
  if (Session->active) return false;

  //--- If another UCI engine currently running -> quit it! ---
  if (autoQuitPrevious) UCI_QuitActiveEngine();

  //--- Launch the engine and create socket via the SocketUCI command ---
  UCI_INFO *Info = &Prefs.UCI.Engine[engineId];

  // First check that the engine actually exists
  if (Info->local && !VerifyEnginePath(Info)) {
    CHAR title[100];
    Format(title, "Cannot find %s", Info->name);
    NoteDialog(theApp->GetFrontWindow(), title,
               "Sigma Chess could not locate the engine. Please check the "
               "engine path in the 'Engine Manager' dialog...",
               cdialogIcon_Error);
    return false;
  }

  // Next try to launch it
  CHAR msg[100];
  Format(msg, "Launching engine '%s'...", Info->name);
  UCI_ProgressDialogOpen("Launching Engine", msg, true, 30);
  {
    if (UCI_AELaunchEngine(engineId, Info->path)) {
      Session->active = true;
      Session->uci_Sent = false;
      Session->name_Rcvd = false;
      Session->author_Rcvd = false;
      Session->uciok_Rcvd = false;

      UCI_SendCommand(engineId, "uci");
      while (!UCI_ProgressDialogCancelled() &&
             !(Session->active && Session->uciok_Rcvd))
        sigmaApp->ProcessEvents();
    }
  }

  // Close progress dialog
  UCI_ProgressDialogClose();

  if (Session->active && Session->uciok_Rcvd) {
    launchedOK = true;
    UCI_SendAllOptions(engineId, true);  // Only send non-default options
  } else {
    UCI_SendCommand(engineId, "quit");
    ClearSession(engineId, true);
  }

  if (!launchedOK) {
    CHAR title[100];
    Format(title, "Failed Launching '%s'", UCI_EngineName(engineId));

    NoteDialog(
        theApp->GetFrontWindow(), title,
        "Please check that this is a valid chess engine supporting the UCI "
        "protocol. You can also try to restart Sigma Chess and try again...",
        cdialogIcon_Error);
    uci_enabled =
        UCI_AELaunchLoader();  // Try to relaunch UCI Loader just in case
  }

  return launchedOK;
}  // UCI_LoadEngine

static BOOL VerifyEnginePath(UCI_INFO *Info) {
  FSRef fsRef;
  return (::FSPathMakeRef((UInt8 *)(Info->path), &fsRef, NULL) == noErr);
}  // VerifyEnginePath

/*------------------------------------ Checks if Engine Loaded
 * -----------------------------------*/

BOOL UCI_EngineLoaded(UCI_ENGINE_ID engineId) {
  return UCISession[engineId].active;
}  // UCI_EngineLoaded

/*------------------------------------------- Swap Engine
 * ----------------------------------------*/
// Sends active engine to the background.

void UCI_SwapEngines(void) { UCI_AESwapEngine(); }  // UCI_SwapEngines

/*------------------------------------------- Quit Engine
 * ----------------------------------------*/

void UCI_QuitEngine(UCI_ENGINE_ID engineId) {
  if (engineId == uci_SigmaEngineId) return;

  UCI_SESSION *Session = &UCISession[engineId];

  UCI_SendCommand(engineId, "quit");

  Session->active = false;
  ClearSession(engineId, false);
}  // UCI_QuitEngine

void UCI_ForceQuitEngines(void) {
  UCI_AEQuitEngine(uci_NullEngineId);

  for (UCI_ENGINE_ID i = 0; i < uci_MaxEngineCount; i++)
    if (UCISession[i].active && i != uci_SigmaEngineId)
      UCISession[i].active = false;
}  // UCI_ForceQuitEngines

/*---------------------------------------- Quit Active Engine
 * ------------------------------------*/

UCI_ENGINE_ID UCI_GetActiveEngineId(void) {
  for (UCI_ENGINE_ID i = 0; i < uci_MaxEngineCount; i++)
    if (UCISession[i].active && i != uci_SigmaEngineId) return i;
  return uci_NullEngineId;
}  // UCI_GetActiveEngineId

void UCI_QuitActiveEngine(void) {
  UCI_ENGINE_ID activeEngineId = UCI_GetActiveEngineId();
  if (activeEngineId != uci_NullEngineId) UCI_QuitEngine(activeEngineId);
}  // UCI_QuitActiveEngine

/*-------------------------------------- Quit Swapped Engine
 * ------------------------------------*/

void UCI_QuitSwappedEngine(void) {
  ClearSession(UCI_AEGetCurrent(), false);
  UCI_AEQuitEngine2();
}  // UCI_QuitSwappedEngine

/*-------------------------------------- Abort All Running Engines
 * -------------------------------*/
// Should be called when opening UCI Engine Manager dialog and when selecting
// engines (unless switching from or to Sigma engine). For rated games, however,
// the user is given the option of cancelling.

BOOL UCI_AbortAllEngines(void) {
  // First check if any rated games are currently running. If so, give user the
  // option of cancelling.
  sigmaApp->winList.Scan();
  while (SigmaWindow *win = (SigmaWindow *)(sigmaApp->winList.Next()))
    if (!win->IsDialog() && win->winClass == sigmaWin_Game) {
      GameWindow *gameWin = (GameWindow *)win;
      if (!gameWin->AbandonRatedGame()) return false;
    }

  // Next abort all running engines
  sigmaApp->winList.Scan();
  while (SigmaWindow *win = (SigmaWindow *)(sigmaApp->winList.Next()))
    if (!win->IsDialog() && win->winClass == sigmaWin_Game) {
      GameWindow *gameWin = (GameWindow *)win;
      gameWin->CheckAbortEngine();
    }

  return true;
}  // UCI_AbortAllEngines

/**************************************************************************************************/
/*                                                                                                */
/*                                         LOCK/UNLOCK */
/*                                                                                                */
/**************************************************************************************************/

// A locking mechanism is maintained to ensure that at most one game window is
// using an engine at any time.

BOOL UCI_RequestLock(UCI_ENGINE_ID engineId, CWindow *ownerWin,
                     BOOL showDenyDialog) {
  if (theApp->ModalLoopRunning()) return false;

  if (engineId == uci_SigmaEngineId) return true;

#ifdef __with_debugLock
  DebugWriteNL("REQUESTING ENGINE LOCK...");
#endif

  UCI_SESSION *Session = &UCISession[engineId];

  if (!Session->ownerWin) {
    Session->ownerWin = ownerWin;
#ifdef __with_debugLock
    if (Session->thinking) {
      DebugWriteNL("...INTERNAL ERROR (LOCK FREE BUT SESSION THINKING)");
      //         return false;
    }
    DebugWriteNL("...SUCCESS (LOCK WAS FREE)");
#endif
    return true;
  } else if (Session->ownerWin == ownerWin) {
#ifdef __with_debugLock
    if (Session->thinking) {
      DebugWriteNL("...INTERNAL ERROR (SESSION THINKING)");
      //         return false;
    }
    DebugWriteNL("...SUCCESS (SAME WIN)");
#endif
    return true;
  }

#ifdef __with_debugLock
  DebugWriteNL("...DENIED");
#endif

  if (showDenyDialog) {
    CHAR msg[200];
    Format(msg,
           "The '%s' engine is currently running in another game window...",
           UCI_EngineName(engineId));
    NoteDialog(ownerWin, "Engine Busy", msg);
  }

  return false;
}  // UCI_RequestLock

void UCI_ReleaseLock(UCI_ENGINE_ID engineId, CWindow *ownerWin) {
  if (engineId == uci_SigmaEngineId) return;

#ifdef __with_debugLock
  DebugWriteNL("RELEASING ENGINE LOCK...");
#endif

  UCI_SESSION *Session = &UCISession[engineId];
  if (Session->thinking) {
#ifdef __with_debugLock
    DebugWriteNL("...IGNORED (STILL THINKING)");
#endif
  } else if (Session->ownerWin == ownerWin) {
#ifdef __with_debugLock
    DebugWriteNL("...DONE");
#endif
    Session->ownerWin = nil;
  } else {
#ifdef __with_debugLock
    DebugWriteNL("...IGNORED (NOT OWNED BY THIS WIN)");
#endif
  }
}  // UCI_ReleaseLock

/**************************************************************************************************/
/*                                                                                                */
/*                                         START ENGINE */
/*                                                                                                */
/**************************************************************************************************/

// Sends a "position" (optionally preceeded by a "ucinewgame") command, followed
// by the "go" commands.

static void SendPositionCmd(UCI_ENGINE_ID engineId, CGame *game);
static void SendGoCmd(UCI_ENGINE_ID engineId, ENGINE *E);

BOOL UCI_Engine_Start(UCI_ENGINE_ID engineId, ENGINE *E, CGame *game,
                      BOOL autoQuitPrevious) {
  UCI_SESSION *Session = &UCISession[engineId];
  UCI_INFO *Info = &Prefs.UCI.Engine[engineId];

  Session->engineRef = nil;
  E->UCI = (engineId != uci_SigmaEngineId);

  //--- Sigma engine ---
  // Simply call the Engine_Start function

  if (!E->UCI) {
    Session->engineRef = E;
    Engine_Start(E);
    return true;
  }

  //--- UCI engine ---
  // First check if we need to launch the engine first
  if (!Session->active && !UCI_LoadEngine(engineId, autoQuitPrevious)) {
    CHAR title[100];
    Format(title, "Failed Launching '%s'", UCI_EngineName(engineId));
    NoteDialog(theApp->GetFrontWindow(), title,
               "The built-in Sigma Chess engine will be used instead...");

    sigmaApp->BroadcastMessage(msg_UCISetSigmaEngine);
    Prefs.UCI.defaultId = uci_SigmaEngineId;

    E->UCI = false;
    Session->engineRef = E;
    Engine_Start(E);
    return false;
  }

  Session->engineRef = E;
  Engine_Start(E);

  //--- Optionally send Nalimov, ponder and strength options ---
  // If those for the engine differs from the current settings, they are sent to
  // the engine here
  if (Info->supportsNalimovBases)
    UCI_SetNalimovPathOption(engineId,
                             Prefs.UCI.NalimovPath);  // Only sends if changed

  if (Info->supportsPonder)
    UCI_SetPonderOption(
        engineId, E->P.permanentBrain);  // Only sends if changed or auto flush

  if (Info->supportsLimitStrength)
    UCI_SetStrengthOption(
        engineId, E->P.reduceStrength,
        E->P.engineELO);  // Only sends if changed or auto flush

  //--- Optionally send 'uninewgame' command ---
  // If we are now searching a new game
  if (Session->gameId != game->gameId) {
    Session->gameId = game->gameId;
    UCI_SendCommand(engineId, "ucinewgame");
    UCI_WaitIsReady(engineId);
  }

  //--- Send Postion command ---
  SendPositionCmd(engineId, game);

  //--- Send 'go' command ---
  // Should be formatted with the search mode, chess clock status etc
  SendGoCmd(engineId, E);

  //--- Switch to thinking mode ---
  Session->thinking = true;

  return true;
}  // UCI_Engine_Start

/*------------------------------------ Send "position" command
 * -----------------------------------*/
// "position [fen <fenstring> | startpos ]  moves <move1> .... <movei>"

static void SendPositionCmd(UCI_ENGINE_ID engineId, CGame *game) {
  CHAR positionCmd[100 + gameRecSize * 5];
  CHAR *buf = positionCmd;

  if (!game->Init.wasSetup) {
    WriteBufStr(&buf, "position startpos");
  } else {
    WriteBufStr(&buf, "position fen ");
    CPgn pgn(game);
    buf += pgn.WriteFEN(buf);
    *buf = 0;
  }

  if (game->currMove > 0) {
    WriteBufStr(&buf, " moves");

    // Note: If pondering/backgrounding, then the last move will be the ponder
    // move.
    for (INT i = 1; i <= game->currMove; i++) {
      MOVE *m = game->GetGameMove(i);
      *(buf++) = ' ';
      *(buf++) = file(m->from) + 'a';
      *(buf++) = rank(m->from) + '1';
      *(buf++) = file(m->to) + 'a';
      *(buf++) = rank(m->to) + '1';
      if (isPromotion(*m))
        *(buf++) = PieceCharEng[pieceType(m->type)] - 'P' +
                   'p';  // Convert to lower case
      *buf = 0;
    }
  }

  UCI_SendCommand(engineId, positionCmd);
}  // SendPositionCmd

/*-------------------------------------- Send "go" command
 * ---------------------------------------*/

static void SendGoCmd(UCI_ENGINE_ID engineId, ENGINE *E) {
  PARAM *P = &E->P;
  CHAR goCmd[200];
  CHAR *buf = goCmd;

  WriteBufStr(&buf, "go");
  if (E->P.backgrounding)
    WriteBufStr(&buf,
                " ponder");  // Ponders on last move in "position..." string

  switch (P->playingMode) {
    case mode_Time:
      WriteBufStr(&buf, (P->player == white ? " wtime " : " btime "));
      WriteBufNum(&buf, P->timeLeft * 1000);
      if (P->timeInc > 0) {
        WriteBufStr(&buf, (P->player == white ? " winc " : " binc "));
        WriteBufNum(&buf, P->timeInc * 1000);
      }
      if (P->movesLeft > 0 && P->movesLeft < allMoves) {
        WriteBufStr(&buf, " movestogo ");
        WriteBufNum(&buf, P->movesLeft);
      }
      break;

    case mode_FixDepth:
      WriteBufStr(&buf, " depth ");
      WriteBufNum(&buf, P->depth);
      break;

    case mode_Infinite:
      WriteBufStr(&buf, " infinite");
      break;

    case mode_Novice:
      WriteBufStr(&buf, " depth 1");
      break;

    case mode_Mate:
      WriteBufStr(&buf, " mate ");
      WriteBufNum(&buf, P->depth);
      break;
  }

  UCI_SendCommand(engineId, goCmd);
}  // SendGoCmd

/*----------------------------------- Send "ponderhit" command
 * -----------------------------------*/
// Should be sent to the engine if the user plays the expected move

void UCI_SendPonderhit(UCI_ENGINE_ID engineId) {
  if (engineId == uci_SigmaEngineId) return;

  UCI_SendCommand(engineId, "ponderhit");
}  // UCI_SendPonderhit

/**************************************************************************************************/
/*                                                                                                */
/*                                         STOP ENGINE */
/*                                                                                                */
/**************************************************************************************************/

// Send "stop" command to engine

void UCI_Engine_Stop(UCI_ENGINE_ID engineId) {
  UCI_SESSION *Session = &UCISession[engineId];
  ENGINE *E = Session->engineRef;

  if (!E) return;  // Internal error

  if (E->UCI) {
    if (!Session->thinking)
      return;  // This can happen if UCI engine has finished itself

    UCI_SendCommand(engineId, "stop");
    UCI_WaitIsReady(engineId);
  }

  Engine_Stop(E);
}  // UCI_Engine_Stop

/**************************************************************************************************/
/*                                                                                                */
/*                                         ABORT ENGINE */
/*                                                                                                */
/**************************************************************************************************/

// Send "stop" command to engine and ignore "bestmove"

void UCI_Engine_Abort(UCI_ENGINE_ID engineId) {
  UCI_SESSION *Session = &UCISession[engineId];
  ENGINE *E = Session->engineRef;

  if (!E) return;  // Internal error

  if (E->UCI) {
    if (!Session->thinking)
      return;  // This can happen if UCI engine has finished itself

    // First clear "thinking" flag, so we ignore all subsequent "info" and
    // "bestmove" messages
    Session->thinking = false;
    // Then stop and wait for engine to complete
    UCI_SendCommand(engineId, "stop");
    UCI_WaitIsReady(engineId);
  }

  Engine_Abort(E);
}  // UCI_Engine_Abort

/**************************************************************************************************/
/*                                                                                                */
/*                                               MISC */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Checks if Engine Busy
 * -------------------------------------*/

BOOL UCI_Engine_Busy(UCI_ENGINE_ID engineId) {
  // Return false for Sigma engines because this has no problem handling
  // multiple sessions (instances)
  if (engineId == uci_SigmaEngineId) return false;

  return (UCISession[engineId].active && UCISession[engineId].thinking);
}  // UCI_Engine_Busy

/*-------------------------------- Send Engine "isready" Command
 * ---------------------------------*/

BOOL UCI_WaitIsReady(UCI_ENGINE_ID engineId) {
  if (debugOn) DebugWriteNL("*** Wait is ready... ***");

  UCI_SESSION *Session = &UCISession[engineId];

  Session->isready_Sent = true;
  Session->readyok_Rcvd = false;
  UCI_SendCommand(engineId, "isready");

  // Wait for 'readok' reply (5 secs)
  theApp->ModalLoopBegin();
  {
    ULONG timeOut = Timer() + 5 * 60;
    while (Session->active && !Session->readyok_Rcvd && Timer() < timeOut)
      sigmaApp->ProcessEvents();
  }
  theApp->ModalLoopEnd();

  // If time out after 5 secs, then open progress dialog and wait another 55
  // secs
  if (!Session->readyok_Rcvd) {
    UCI_ProgressDialogOpen("", "Waiting for engine...", true, 115);
    while (Session->active && !Session->readyok_Rcvd &&
           !UCI_ProgressDialogCancelled())
      sigmaApp->ProcessEvents();
    UCI_ProgressDialogClose();
  }

  // Done waiting
  Session->isready_Sent = false;

  if (!Session->readyok_Rcvd) {
    Session->active = false;
    UCI_AEQuitEngine(engineId);
    DebugWriteNL(
        "UCI ERROR: Engine not responding to 'isready' command after 120 "
        "secs...");
    NoteDialog(theApp->GetFrontWindow(), "The Engine is not responding",
               "Please try to restart it from the 'Engine Manager' dialog. "
               "Alternatively you can restart Sigma Chess...",
               cdialogIcon_Error);
  } else if (debugOn)
    DebugWriteNL("*** Ready! ***");

  return Session->active && Session->readyok_Rcvd;
}  // UCI_WaitIsReady

/*-------------------------------- Send Engine "isready" Command
 * ---------------------------------*/

CHAR *UCI_EngineName(UCI_ENGINE_ID engineId) {
  return Prefs.UCI.Engine[engineId].name;
}  // UCI_EngineName

/**************************************************************************************************/
/*                                                                                                */
/*                                      SEND ENGINE COMMANDS */
/*                                                                                                */
/**************************************************************************************************/

// Generic low level routine for sending text commands directly to UCI engines

void UCI_SendCommand(UCI_ENGINE_ID engineId, CHAR *cmd) {
  // Get session struct
  UCI_SESSION *Session = &UCISession[engineId];

  // Send the actual command (newline terminated)
  CHAR msg[uci_MaxMessageLen];
  Format(msg, "%s\n", cmd);
  if (debugOn) {
    DebugWriteNL("SENDING:");
    DebugWriteNL(cmd);
  }

  UCI_AESendMessage(Session->engineId, msg);

  // Update session state
  if (EqualStr(cmd, "uci"))
    Session->uci_Sent = true;
  else if (EqualStr(cmd, "quit"))
    Session->quit_Sent = true;
  else if (EqualStr(cmd, "isready")) {
    Session->isready_Sent = true;
    Session->readyok_Rcvd = false;
  }
}  // UCI_SendCommand
