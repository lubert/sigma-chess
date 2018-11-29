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

#include "Engine.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                      SIGMA CHESS ENGINE API */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------- Engine System Initialization
 * -------------------------------*/

void Engine_InitSystem(GLOBAL *Global, PTR kpkData = nil);

/*---------------------------------- Create/Destroy Engine Instance
 * ------------------------------*/

BOOL Engine_Create(GLOBAL *Global, ENGINE *E, ULONG refID);
void Engine_Destroy(ENGINE *E);

/*------------------------------------ Start/Stop Engine Instance
 * --------------------------------*/

void Engine_Start(ENGINE *E);
void Engine_Stop(ENGINE *E);
void Engine_Abort(ENGINE *E);
void Engine_AbortAll(GLOBAL *Global);
void Engine_Pause(ENGINE *E, BOOL paused);
BOOL Engine_AnyRunning(GLOBAL *Global);
BOOL Engine_OtherRunning(GLOBAL *Global, ENGINE *Except);
void Engine_Periodic(ENGINE *E);

/*------------------------------- Access Engine Instance Stats/Results
 * ---------------------------*/

#define Engine_BestScore(E) E->S.bestScore
#define Engine_ScoreType(E) E->S.scoreType
#define Engine_BestMove(E) E->S.MainLine[0]
#define Engine_BestReply(E) E->S.bestReply  // E->S.MainLine[1]
#define Engine_IsPonderMove(E) E->S.isPonderMove
#define Engine_MultiPV(E) E->S.multiPV
#define Engine_MainLine(E) E->S.MainLine
#define Engine_MainDepth(E) E->S.mainDepth
#define Engine_MoveCount(E) E->S.moveCount
#define Engine_CurrMoveNo(E) (E->S.currMove + 1)  // [1..numRootMoves]
#define Engine_CurrMove(E) E->S.rootNode->m

#define Engine_SearchTime(E) (Timer() - E->S.startTime)
#define Engine_Nps(E) (E->UCI ? E->S.uciNps : 0)
#define Engine_MainTime(E) E->S.mainTime
#define Engine_MateTime(E) E->S.mateTime
#define Engine_HashFull(E) E->S.hashFull

#define Engine_TaskRunning(E) E->R.taskRunning
#define Engine_RunState(E) E->R.state
#define Engine_Aborted(E) E->R.aborted

/*---------------------------------------- Set Engine State
 * --------------------------------------*/

#define Engine_ClearBackgrounding(E) E->P.backgrounding = false
#define Engine_Continue(E) E->S.mateContinue = true

/**************************************************************************************************/
/*                                                                                                */
/*                                    INTERNAL FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

// These routines should NOT be called by the host application

void SendMsg_Async(ENGINE *E, ULONG message);
void SendMsg_Sync(ENGINE *E, ULONG message);

asm void Asm_Begin(ENGINE *E);
asm void Asm_End(void);
