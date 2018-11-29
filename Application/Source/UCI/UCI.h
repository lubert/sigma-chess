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

#pragma once

#include "UCI_Defs.h"

#include "CWindow.h"
#include "Engine.h"
#include "Game.h"
#include "Rating.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                       TYPE/CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- UCI Engine Options
 * -------------------------------------*/

typedef struct {
  CHAR name[uci_OptionNameLen + 1];
  UCI_OPTION_TYPE type;

  // The actual option value (no value for buttons) depending on option type.
  // The "def" field for the first three options contains the default value.
  union {
    struct {
      BOOL def;
      BOOL val;
    } Check;
    struct {
      LONG def;
      LONG val;
      LONG min;
      LONG max;
    } Spin;
    struct {
      INT def;
      INT val;
      CHAR List[uci_ComboListLen][uci_ComboNameLen + 1];
      INT count;
    } Combo;
    struct {
    } Button;
    struct {
      CHAR def[uci_StringOptionLen + 1];
      CHAR val[uci_StringOptionLen + 1];
    } String;
  } u;

} UCI_OPTION;

/*----------------------------------------- UCI Engine Info
 * --------------------------------------*/
// Engine configuration (stored in prefs file)

typedef struct {
  // Engine info
  CHAR name[uci_NameLen + 1];
  CHAR author[uci_AuthorLen + 1];
  CHAR engineAbout[uci_StringOptionLen + 1];

  // Engine location
  BOOL local;  // Is engine running on local computer (same as Sigma)?
  CHAR path[uci_EnginePathLen +
            1];  // Path to location of engine ("" if local = false)

  // Generic UCI options
  INT optionCount;
  UCI_OPTION Options[uci_MaxOptionCount];

  // Specific option info
  BOOL supportsPonder;  // Does engine support pondering?
  UCI_OPTION
      Ponder;  // Current engine value (so we know if we need to send again)
  BOOL flushPonder;

  BOOL supportsLimitStrength;  // Does engine support ELO limit strength?
  UCI_OPTION LimitStrength;
  UCI_OPTION UCI_Elo;
  BOOL flushELO;

  BOOL autoReduce;

  BOOL supportsNalimovBases;  // Does engine support Nalimov table bases?
  UCI_OPTION NalimovPath;
} UCI_INFO;

/*----------------------------------------- UCI Preferences
 * --------------------------------------*/
// Settings for all UCI engines know by Sigma. The "Engine[]" array is indexed
// by the UCI_ENGINE_ID type.

typedef struct {
  // Engine list
  UCI_ENGINE_ID defaultId;  // Index of default engine (intially Sigma)
  INT count;  // Total number of 3rd party UCI engines currently installed
  UCI_INFO Engine[uci_MaxEngineCount];  // Indexed by UCI_ENGINE_ID

  // Common options
  CHAR NalimovPath[uci_NalimovPathLen + 1];
} UCI_PREFS;

/*------------------------------------------- UCI Sessions
 * ---------------------------------------*/
// Describes connection/session status for each engine.

typedef struct {
  UCI_ENGINE_ID engineId;  // Index in UCI_INFO list

  // Connection info:
  BOOL active;  // Has this session been assigned to an engine (i.e. has the
                // engine been launched)?

  // Owner ID
  CWindow *ownerWin;  // Game window currently using (locking) this engine. nil
                      // if not locked

  // Game Id
  LONG gameId;  // Used for determining when to send a 'ucinewgame' message

  // Engine Interface Ref
  ENGINE *engineRef;

  // Session state
  BOOL uci_Sent;     // Has engine been told to switch to UCI mode?
  BOOL name_Rcvd;    // Has engine replied with name?
  BOOL author_Rcvd;  // Has engine replied with author?
  BOOL uciok_Rcvd;   // Is engine ready?

  BOOL isready_Sent;  // Has "isready" command been sent to engine? Should clear
                      // readyok_Sent.
  BOOL readyok_Rcvd;  // Has engine replied with "readyok" command?

  // BOOL waitingForUser;
  BOOL thinking;
  BOOL pondering;

  BOOL quit_Sent;  // Has engine been told to quit?
} UCI_SESSION;

/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES */
/*                                                                                                */
/**************************************************************************************************/

// Indexed by UCI_ENGINE_ID. Sigma engine always in first entry (not a true UCI
// session)

extern UCI_SESSION UCISession[uci_MaxEngineCount];

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

// All access to UCI from the rest of the Sigma application should go through
// the API below:

BOOL UCI_Enabled(void);

//--- Startup Initialization ---
void UCI_InitModule(BOOL firstLaunch);
void UCI_ResetPrefs(void);

//--- Manipulate engine list ---
UCI_ENGINE_ID UCI_AddLocalEngine(
    CHAR *enginePath);  // Returns "uci_NullEngineId" if error
void UCI_RemoveEngine(UCI_ENGINE_ID engineId);
CHAR *UCI_EngineName(UCI_ENGINE_ID engineId);

//--- Load/Quit engines ---
BOOL UCI_LoadEngine(UCI_ENGINE_ID engineId,
                    BOOL autoQuitPrevious = true);  // Returns false if error
BOOL UCI_EngineLoaded(UCI_ENGINE_ID engineId);
void UCI_SwapEngines(void);
void UCI_QuitEngine(UCI_ENGINE_ID engineId);
void UCI_ForceQuitEngines(void);
void UCI_QuitActiveEngine(void);
void UCI_QuitSwappedEngine(void);
BOOL UCI_AbortAllEngines(void);

UCI_ENGINE_ID UCI_GetActiveEngineId(void);

//--- Lock/unlock ---
BOOL UCI_RequestLock(UCI_ENGINE_ID engineId, CWindow *gameWin,
                     BOOL showDenyDialog = false);
void UCI_ReleaseLock(UCI_ENGINE_ID engineId, CWindow *gameWin);

//--- Send commands to loaded engines ---
BOOL UCI_Engine_Start(UCI_ENGINE_ID engineId, ENGINE *E, CGame *game,
                      BOOL autoQuitPrevious = true);
void UCI_Engine_Stop(UCI_ENGINE_ID engineId);
void UCI_Engine_Abort(UCI_ENGINE_ID engineId);
BOOL UCI_Engine_Busy(UCI_ENGINE_ID engineId);
void UCI_SendPonderhit(UCI_ENGINE_ID engineId);

BOOL UCI_WaitIsReady(
    UCI_ENGINE_ID engineId);  // Sends "isready" command and waits for "readyok"

// Sending commands (should NOT be new line terminated here)
void UCI_SendCommand(UCI_ENGINE_ID engineId, CHAR *cmd);
