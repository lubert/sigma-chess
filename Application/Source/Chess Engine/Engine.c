/**************************************************************************************************/
/*                                                                                                */
/* Module  : ENGINE.C */
/* Purpose : This is the main Sigma Chess Engine module which provides routines
 * for initializing, */
/*           starting and stopping the engine. The Engine Interface and the GUI
 * modules deal with */
/*           this module only, and never with the other Engine Modules. */
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

#include "Engine.h"
#include "TaskScheduler.h"

#include "Attack.f"
#include "Board.f"
#include "Engine.f"
#include "Evaluate.f"
#include "HashCode.f"
#include "MoveGen.f"
#include "PerformMove.f"
#include "PieceVal.f"
#include "Search.f"
#include "Time.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                 SIGMA CHESS PPC Engine 2.00 Build 05a */
/*                                                                                                */
/**************************************************************************************************/

// This is the main Sigma Chess PPC engine interface module. It implements the
// API through which the UI part of the host program communicates with the
// Engine. The engine is designed to be re-entrant; all data structures are
// stored in a single structure (contiguous memory block) which contains the
// entire engine state (excluding transposition tables, opening libraries and
// endgame databases).
//
// The engine can run as a separate task, and supports multiple engine
// instances, i.e. the engine can search multiple positions simultaneously via
// multitasking.

/**************************************************************************************************/
/*                                                                                                */
/*                                    ENGINE SYSTEM INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Initialize Engine System
 * ---------------------------------*/
// At startup the common "Global" engine data structure is initialized. However,
// allocation of this structure is left to the calling host application, in
// order to keep all memory allocation and global data structure definitions
// outside the engine.

void Engine_InitSystem(GLOBAL *Global, PTR kpkData) {
  Global->msgBitTab = 0;

  Global->engineCount = 0;
  for (INT i = 0; i < maxEngines; i++) Global->Engine[i] = nil;

  // The remaining engine modules MUST be initialized in the following order:
  InitBoardModule(Global);
  InitAttackModule(Global);
  InitMoveGenModule(Global);
  InitPerformMoveModule(Global);
  InitPieceValModule(Global);
  InitEvaluateModule(Global, kpkData);
  InitSearchModule(Global);
  InitHashCodeModule(Global);
} /* Engine_InitSystem */

/**************************************************************************************************/
/*                                                                                                */
/*                                      CREATE/DISPOSE ENGINE */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Create Engine
 * ----------------------------------------*/
// The engine struct must already have been allocated by the caller.

static void InitSearchParam(PARAM *P);

BOOL Engine_Create(GLOBAL *Global, ENGINE *E, ULONG refID) {
  if (Global->engineCount >= maxEngines || !E) return false;

  // First insert in engine table:
  INT i;
  for (i = 0; i < maxEngines && Global->Engine[i]; i++)
    ;
  if (i >= maxEngines) return false;
  Global->Engine[i] = E;
  Global->engineCount++;

  // Then initialize engine parameters and state for each module/component:
  E->refID = refID;
  E->localID = i;
  E->Global = Global;
  E->msgQueue = 0;
  E->UCI = false;

  E->R.taskID = 0;
  E->R.taskRunning = false;
  E->R.state = state_Stopped;

  InitSearchParam(&E->P);
  InitBoardState(&E->B);
  InitSearchState(E);

  return true;
} /* Engine_Create */

static void InitSearchParam(
    PARAM *P)  // Initializes the search parameters with some happy
{              // defaults.
  P->backgrounding = false;

  // Set default engine search/eval parameters:
  P->pvSearch = true;
  P->alphaBetaWin = true;
  P->extensions = true;
  P->selection = true;
  P->deepSelection = false;
  P->nondeterm = false;
  P->useEndgameDB = true;
  P->proVersion = true;

  // Mode/Level/Style parameters:
  P->playingMode = mode_FixDepth;  // Playing mode.
  P->movesPlayed = 0;       // Moves played so far since last time control.
  P->movesLeft = allMoves;  // Moves left to next time control.
  P->timeLeft =
      300;  // Time left in seconds to next time control (if time mode).
  P->timeInc = 0;
  P->moveTime = 5;
  P->depth = 1;  // Depth/level if fixed depth, mate finder or novice.
  P->playingStyle = style_Normal;
  P->reduceStrength = false;
  P->engineELO = 2400;
  P->nextBest = false;

  // Opening Library:
  P->Library = nil;               // Opening library to be used.
  P->libSet = libSet_Tournament;  // Use "tournament" positions sub set

  // Transposition Tables:
  P->TransTables = nil;  // Transposition table memory block.
  P->transSize = 0;
} /* InitSearchParam */

/*----------------------------------------- Destroy Engine
 * ---------------------------------------*/
// Note: It's the caller's responsibility to deallocate the engine struct AFTER
// calling Engine_Destroy.

void Engine_Destroy(ENGINE *E) {
  GLOBAL *G = E->Global;

  if (E->R.taskRunning) Engine_Abort(E);

  // Remove from engine table:
  G->msgBitTab &=
      ~bit(E->localID);  // Can't use ^= because we are not sure if bit is set
  G->Engine[E->localID] = nil;
  G->engineCount--;
} /* Engine_Destroy */

/**************************************************************************************************/
/*                                                                                                */
/*                                       START/STOP ENGINE */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Start Engine
 * -----------------------------------------*/
// This routine starts the engine, i.e. the engine starts analyzing the
// specified position given the specified search constraints/parameters (the
// PARAM record in the ENGINE data). This call starts a separate task in which
// the engine runs.

asm LONG Engine_TaskFuncASM(void *data);

asm LONG Engine_TaskFuncASM(void *data) {
  taskFuncWrapper(MainSearch)
} /* Engine_TaskFuncASM */

void Engine_Start(ENGINE *E) {
  E->Global->currentEngine =
      E;  //### Only for debugging (single engine instance).
  E->R.taskRunning = true;

  if (E->UCI)
    MainSearch_BeginUCI(E);
  else
    E->R.taskID = Task_Create(Engine_TaskFuncASM, (PTR)E);
} /* Engine_Start */

/*--------------------------------------- Stop/Abort Engine
 * --------------------------------------*/

void Engine_Stop(ENGINE *E) {
  if (E->R.state != state_Stopped)
    if (E->UCI)
      MainSearch_EndUCI(E);
    else
      E->R.state = state_Stopping;

  E->R.aborted = false;
} /* Engine_Stop */

void Engine_Abort(ENGINE *E) {
  E->R.state = state_Stopped;
  E->R.aborted = true;
  E->msgQueue = 0;
  E->Global->msgBitTab &=
      ~bit(E->localID);  // Can't use ^= because we are not sure if bit is set

  if (E->UCI) {
    E->R.taskRunning = false;
  } else if (E->R.taskRunning && E->R.taskID != Task_GetCurrent()) {
    Task_Kill(E->R.taskID);
    E->R.taskRunning = false;
  }
} /* Engine_Abort */

void Engine_AbortAll(GLOBAL *Global) {
  for (INT i = 0; i < maxEngines; i++)
    if (Global->Engine[i]) Engine_Abort(Global->Engine[i]);
} /* Engine_AbortAll */

/**************************************************************************************************/
/*                                                                                                */
/*                                               MISC */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                               MISC */
/*                                                                                                */
/**************************************************************************************************/

BOOL Engine_AnyRunning(GLOBAL *Global) {
  for (INT i = 0; i < maxEngines; i++)
    if (Global->Engine[i] && Global->Engine[i]->R.taskRunning) return true;
  return false;
} /* Engine_AnyRunning */

BOOL Engine_OtherRunning(GLOBAL *Global, ENGINE *Except) {
  for (INT i = 0; i < maxEngines; i++) {
    ENGINE *E = Global->Engine[i];
    if (E && E != Except && E->R.taskRunning) return true;
  }

  return false;
} /* Engine_OtherRunning */

void Engine_Periodic(ENGINE *E) {
  if (E->UCI || Timer() < E->S.periodicTime) return;

  if (E->Tr.transSize > 0)
    E->S.hashFull = (10 * E->Tr.transUsed) / MaxL(1, E->Tr.transSize / 100);

  if (TimeOut(E) && E->R.state == state_Running) Engine_Stop(E);

  E->msgQueue |= msg_Periodic;
  E->Global->msgBitTab |= bit(E->localID);
  Task_Switch();
  E->S.periodicTime = Timer() + (Task_GetCount() > 2 ? 5 : 20);
} /* Engine_Periodic */

/**************************************************************************************************/
/*                                                                                                */
/*                                      ENGINE MESSAGE HANDLING */
/*                                                                                                */
/**************************************************************************************************/

// This routine is called from within the engine and passes information back to
// the host application via the specified callback routine (which is mainly used
// for displaying the information to the user).

void SendMsg_Async(ENGINE *E, ULONG message) {
  E->msgQueue |= message;
  E->Global->msgBitTab |= bit(E->localID);

  if (!E->UCI && Timer() >= E->S.periodicTime) {
    Task_Switch();
    E->S.periodicTime = Timer() + (Task_GetCount() > 2 ? 5 : 20);
  }
} /* SendMsg_Async */

void SendMsg_Sync(ENGINE *E, ULONG message) {
  E->msgQueue |= message;
  E->Global->msgBitTab |= bit(E->localID);

  do
    Task_Switch();
  while (E->msgQueue & message);  // Wait until host app has processed message
} /* SendMsg_Sync */

/**************************************************************************************************/
/*                                                                                                */
/*                                      PPC ASSEMBLY WRAPPERS */
/*                                                                                                */
/**************************************************************************************************/

// Many engine routines are written in assembly language only. These routines
// all expect the General Purpose Registers to be set up in a special way. Thus,
// when calling an ASM-routine from a normal C-routine we have to "wrap" the
// ASM-routine call in a PPCAsm_Begin/PPCAsm_End context:
//
// void SomeCRoutine(...)
// {
//    Asm_Begin(Engine);
//       TheAsmRoutine(...);
//    Asm_End();
// }

#define engineEnvSize (4 * 19)  // 19 registers are saved (r13...r31)

asm void Asm_Begin(ENGINE *E)  // Sets up the registers e.t.c.
{
  stmw rEngine, -engineEnvSize(SP) addi SP, SP, -engineEnvSize mr rEngine,
      rTmp1 addi rBoard, rEngine, ENGINE.B.Board lwz rPieceCount,
      ENGINE.B.pieceCount(rEngine) lwz rNode,
      ENGINE.S.currNode(rEngine) lhz rPlayer, NODE.player(rNode) lha rPawnDir,
      NODE.pawnDir(rNode) lwz rAttack, NODE.Attack(rNode) lwz rAttack_,
      NODE.Attack_(rNode) lwz rPieceLoc, NODE.PieceLoc(rNode) lwz rPieceLoc_,
      NODE.PieceLoc_(rNode) lwz rFlags, ENGINE.R.rflags(rEngine) lwz rGlobal,
      ENGINE.Global(rEngine) blr
} /* Asm_Begin */

asm void Asm_End(
    void)  // Restores the registers (and flushes "copyback" registers)
{
  stw rPieceCount, ENGINE.B.pieceCount(rEngine) stw rNode,
      ENGINE.S.currNode(rEngine) addi SP, SP, engineEnvSize lmw rEngine,
      -engineEnvSize(SP) blr
} /* Asm_End */
