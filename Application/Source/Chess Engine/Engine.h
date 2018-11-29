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

#undef check  // Needed because this macro is used in Carbon headers!!

#include "AsmDef.h"
#include "Attack.h"
#include "Board.h"
#include "Evaluate.h"
#include "General.h"
#include "HashCode.h"
#include "Mobility.h"
#include "Move.h"
#include "MoveGen.h"
#include "PerformMove.h"
#include "PieceVal.h"
#include "Search.h"
#include "Time.h"
#include "TransTables.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define maxEngines 10  // Maximum number of active engines

/*--------------------------------------- Engine Run State
 * ---------------------------------------*/

enum ENGINE_STATE {
  state_Stopped = 0,
  state_Root = 1,
  state_Running = 2,
  state_Stopping = 3
};

/*--------------------------------------- Message Protocol
 * ---------------------------------------*/
// The engine communicates with the host application via simple outbound message
// queue. The queue is just an ULONG "msgQueue", and the messages are just bits
// which are set in this queue. There are two categories of messages:
// Asynchronous and Synchronous
//
// ASYNC : When the engine has posted such an async message it continues
// searching. These are mainly
//         "informational" bits indicating that some search statistics have
//         changed (i.e. search depth), and hence it's no problem if they are
//         ignored by the host app.
//
// SYNC  : After posting a synchronous message the engine "sleeps" waiting for
// some feedback/data
//         from the host app (i.e. if a mate has been found in the mate finder,
//         the host app needs to tell the engine to stop, continue or abort).
//         The host app should clear the sync message bit in order to wake up
//         the engine.
//
// After posting a message, the engine always calls Task_Switch in order to let
// the host app get some execution time (so it can respond to the message). For
// sync messages, the engine continually loops and calls Task_Switch until the
// sync message bit has been cleared, hence indicating that it has been
// processed.

enum ENGINE_MESSAGE  // Outbound Engine message types (i.e message FROM engine):
{
  // --- State/Statistics Messages (Async) ---
  msg_BeginSearch =
      0x0001,  // First message sent, just after engine has been started.
  msg_NewIteration =
      0x0002,  // When main search depth changes (new iteration starts).
  msg_NewRootMove = 0x0004,   // When a new root move is being analyzed.
  msg_NewScore = 0x0008,      // When the score changes.
  msg_NewMainLine = 0x0010,   // When new main line is found.
  msg_NewNodeCount = 0x0020,  // Just an advise to host, that it should now
                              // redisplay the node count
  msg_EndSearch = 0x0040,  // Last message sent, just before engine terminates

  // --- Periodic Messages (Async)
  msg_Periodic = 0x0080,  // Let host app perform periodic action

  // --- Host Query Messages (Sync) ---
  msg_ProbeEndgDB = 0x0100,  // Ask host application to probe endgame database.
  msg_MateFound = 0x0200,  // Called when Mate Finder finds a mate. The user can
                           // then either stop, continue or abort.

  // --- Debug Messages (Async) ---
  msg_DebugWrite =
      0x0400,            // Sent if engine has written string to debug buffer.
  msg_NewNode = 0x0800,  // New node entered (debug/trace mode only).
  msg_EndNode = 0x1000,  // Current node exited (debug/trace mode only).
  msg_NewMove = 0x2000,  // New move being analyzed (debug/trace mode only).
  msg_Cutoff = 0x4000    // Cut off occured (debug/trace mode only).
};

/*---------------------------------------- Runtime Flags
 * -----------------------------------------*/

enum ENGINE_RFLAGS  // Engine run flags stored in the "rFlags" register during
                    // search.
{ rflag_RunState =
      0x0003,  // Bits 0..1 : (ENGINE_STATE: Stopped, root, running, stopping)
  rflag_PVSearch = 0x0004,  // Bit  2    : Principal variation search?
  rflag_Extensions =
      0x0008,  // Bit  3    : Apply depth extensions for forced/dangerous moves?
  rflag_Selection = 0x0010,  // Bit  4    : Apply selection of "poor" moves?
  rflag_DeepSel =
      0x0020,  // Bit  5    : Start selection earlier (### determined at root?).
  rflag_ReduceStrength = 0x0040,  // Bit  6    : Reduce playing strength?
  rflag_TransTabOn = 0x0080       // Bit  7    : Are transposition tables on?
};

/*----------------------------------------- Score Types
 * ------------------------------------------*/

enum ENGINE_SCORETYPE    // Engine score types for E->S.bestScore:
{ scoreType_True,        // S->bestScore is a true score (i.e. inside alpha-beta
                         // window)
  scoreType_LowerBound,  // S->bestScore is an upper bound on true score
                         // (happens if fail low)
  scoreType_UpperBound,  // S->bestScore is a lower bound on true score (happens
                         // if fail high)
  scoreType_Temp,    // S->bestScore is a temporary score (returned from PV line
                     // at non root node)
  scoreType_Book,    // S->bestScore is a book score (which is random and not
                     // really relevant!)
  scoreType_Unknown  // S->bestScore is a useless value which should not be
                     // displayed
};

/*------------------------------------ Playing Modes/Styles
 * --------------------------------------*/

enum ENGINE_PLAYING_MODE {
  mode_Time = 1,
  mode_FixDepth = 2,
  mode_Infinite = 3,
  mode_Novice = 4,
  mode_Mate = 5
};

enum ENGINE_PLAYING_STYLE {
  style_Chicken = 1,
  style_Defensive = 2,
  style_Normal = 3,
  style_Aggressive = 4,
  style_Desperado = 5
};

#define allMoves 10000

/**************************************************************************************************/
/*                                                                                                */
/*                                        TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------ The Search Parameters Data Structure
 * ----------------------------*/
// Once the engine has been created (via Engine_Create()), all that is needed to
// start the engine is to initialize the PARAM structure below and then call
// Engine_Start.

typedef struct {
  //--- Game state ---
  PIECE
      Board[boardSize];  // Current board configuration of game (CGame->Board).
  INT HasMovedTo[boardSize];  // Castling detection (CGame->HasMovedTo).
  COLOUR player;              // Side to start in this position (CGame->player).
  MOVE lastMove;              // Last move played (CGame->Record[currMove]).
  INT lastMoveNo;  // Half moves played from initial position (CGame->currMove).
  DRAWDATA *DrawData;  // Draw information table (CGame->DrawData).

  //--- Analysis Category ---
  BOOL backgrounding;  // Is this a background analysis (in opponents time)?
  BOOL nextBest;       // True if "Next Best" search. If so, the "Ignore[]" list
                       // is NOT reset.
  //--- Search/eval parameters ---
  BOOL pvSearch;       // Principal variation search?
  BOOL alphaBetaWin;   // Narrow root alpha/beta win?
  BOOL extensions;     // Apply depth extensions for forced/dangerous moves?
  BOOL selection;      // Apply selection of "poor" moves?
  BOOL deepSelection;  // Start selection earlier.
  BOOL nondeterm;      // Non-deterministic (i.e. add small random value)?
  BOOL useEndgameDB;   // Are endgame databases enabled?
  BOOL proVersion;     // Pro-version?

  //--- Mode/Level/Style parameters ---
  INT playingMode;  // Playing mode.
  INT movesPlayed;  // Moves played so far since last time control.
  INT movesLeft;    // Moves left to next time control.
  LONG timeLeft;    // Time left in seconds to next time control (if time mode).
  INT timeInc;   // Time increment (in secs) per move if Fischer. 0 otherwise.
  INT moveTime;  // Avg. time assigned to each move (used for ELO adjustment).
  INT depth;     // Depth/level if fixed depth, mate finder or novice.
  INT playingStyle;  // The playing style.

  //--- Mode/Level/Style parameters ---
  BOOL reduceStrength;  // Reduce strength of engine (ie reduce nps)?
  BOOL permanentBrain;  // Used by the ELO to NPS calculation.
  INT engineELO;        // Engine "target" ELO if "reduceStrength".
  INT actualEngineELO;  // The actual/achieved strength during the search.

  //--- Opening Library ---
  LIBRARY *Library;  // Position library to be used (nil if none or disabled).
  LIB_SET libSet;    // Position subset to be used.

  //--- Transposition Tables ---
  TRANS *TransTables;  // Transposition table buffer (nil if disabled).
  LONG transSize;      // Size in bytes of transposition tables (0 if disabled).
} PARAM;

typedef struct {
  LONG taskID;
  BOOL taskRunning;
  INT state;     // Stopped, root, running, stopping, paused
  BOOL aborted;  // Was search aborted?

  ULONG
      rflags;  // Global flags stored in the rFlags register for faster access.
} RUN_STATE;

/*----------------------------------- Main ENGINE Data Structure
 * ---------------------------------*/
// The ENGINE data structure contains all information about the state of an
// engine, such as search parameters, board state, attack state, search nodes
// state etc. The logical engine ID is supplied by the creator/host, and is used
// to identify the specific engine in the "Callback" message handler (in ·
// Chess, the refID is actually a pointer to the "owning" game window).

typedef struct {
  ULONG refID;    // Unique logical, engine ID (reference constant) supplied by
                  // host.
  ULONG localID;  // Index in Global->Engine[] of this instance
  struct _GLOBAL *Global;  // Pointer to global struct (so we only need to pass
                           // engine struct to the various routines).
  BOOL UCI;        // Is this a UCI engine proxy?
  PARAM P;         // Engine search parameters (level/time/mode, flags...)
  RUN_STATE R;     // Engine run state information.
  ULONG msgQueue;  // Outbound message "queue" (one bit per message type).

  BOARD_STATE B;     // Board state. Incrementally updated during search.
  ATTACK_STATE A;    // Attack state. Incrementally update during search.
  EVAL_STATE E;      // Evaluate State.
  PIECEVAL_STATE V;  // Piece value tables e.t.c. for root position.
  TIME_STATE T;      // Time allocation state.
  TRANS_STATE Tr;    // Transposition tables.
  SEARCH_STATE S;    // Nodes of current branch in search tree.

  CHAR debugStr[1000];
} ENGINE;

/*------------------------------------ Global COMMON Data Structure
 * ------------------------------*/
// The global common structure contains various read only utility data
// structures shared by all engines (i.e. there is only a single instance of
// this structure, whereas there can be multiple engine structures - one for
// each engine).

typedef struct _GLOBAL {
  // Engine message bit table. If bit "i" is set, messages are pending for
  // Engine[i].
  ULONG msgBitTab;

  // Engine table (list of active engines):
  INT engineCount;
  ENGINE *Engine[maxEngines];

  ENGINE *currentEngine;  // Currently executing engine (mainly needed for
                          // debugging).

  // Common read-only data structures shared by all engine instances.
  BOARD_COMMON B;
  ATTACK_COMMON A;
  MOVEGEN_COMMON M;
  PERFORMMOVE_COMMON P;
  HASHCODE_COMMON H;
  PIECEVAL_COMMON V;
  EVAL_COMMON E;  // Must be last (because of KPKData and 32K limitation)
} GLOBAL;
