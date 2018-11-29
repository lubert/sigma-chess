/**************************************************************************************************/
/*                                                                                                */
/* Module  : ENGINECALLBACK.C */
/* Purpose : This module implements the "Engine Message Handler", which responds
 * to various       */
/*           engine initiated events (e.g. new iteration started) which require
 * visual feedback.  */
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

#include "GameOverDialog.h"
#include "GameWindow.h"
#include "SigmaApplication.h"

#include "Engine.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                     ENGINE MESSAGE HANDLER */
/*                                                                                                */
/**************************************************************************************************/

static void ProbeEndgameDB(ENGINE *E);

void GameWindow::ProcessEngineMessage(void) {
  if (engine->msgQueue & msg_Periodic)  // NOTE: We also get here when
                                        // backgrounding in user's time.
  {
    engine->msgQueue &= ~msg_Periodic;

    if (TickClock()) {
      // First update node count:
      infoAreaView->SetNodes(Engine_MoveCount(engine),
                             Engine_SearchTime(engine), Engine_Nps(engine),
                             Engine_HashFull(engine));

      // If solver -> check if time limit reached:
      if (level.mode == pmode_Solver && level.Solver.timeLimit > 0 &&
          Clock[game->player]->elapsed >= level.Solver.timeLimit)
        UCI_Engine_Stop(uciEngineId);
    }
  }

  if (engine->msgQueue & msg_BeginSearch) engine->msgQueue &= ~msg_BeginSearch;

  if (engine->msgQueue & msg_NewIteration) {
    infoAreaView->SetMainDepth(Engine_MainDepth(engine),
                               Engine_MultiPV(engine));
    engine->msgQueue &= ~msg_NewIteration;
  }

  if (engine->msgQueue & msg_NewRootMove) {
    infoAreaView->SetCurrent(Engine_CurrMoveNo(engine),
                             &(Engine_CurrMove(engine)));
    engine->msgQueue &= ~msg_NewRootMove;
  }

  if (engine->msgQueue & msg_NewScore) {
    infoAreaView->SetScore(Engine_BestScore(engine), Engine_ScoreType(engine),
                           Engine_MultiPV(engine));
    if (level.mode == pmode_Solver &&
        Engine_BestScore(engine) >= level.Solver.scoreLimit)
      UCI_Engine_Stop(uciEngineId);
    engine->msgQueue &= ~msg_NewScore;
  }

  if (engine->msgQueue & msg_NewMainLine) {
    infoAreaView->SetMainLine(Engine_MainLine(engine), Engine_MainDepth(engine),
                              Engine_MultiPV(engine));
    if (!backgrounding) hintMove = Engine_BestMove(engine);
    engine->msgQueue &= ~msg_NewMainLine;
  }

  if (engine->msgQueue & msg_NewNodeCount) {
    infoAreaView->SetNodes(Engine_MoveCount(engine), Engine_SearchTime(engine),
                           Engine_Nps(engine), Engine_HashFull(engine));
    engine->msgQueue &= ~msg_NewNodeCount;
  }

  if (engine->msgQueue & msg_EndSearch) {
    SearchCompleted();
    engine->msgQueue &= ~msg_EndSearch;
  }

  if (engine->msgQueue & msg_MateFound) {
    MateFinderDialog(this);
    engine->msgQueue &= ~msg_MateFound;
  }

  if (engine->msgQueue & msg_ProbeEndgDB) {
    ProbeEndgameDB(engine);
    engine->msgQueue &= ~msg_ProbeEndgDB;
  }

  if (engine->msgQueue & msg_DebugWrite) {
    engine->msgQueue &= ~msg_DebugWrite;
  }

#ifdef __debug_GameWin
  case msg_NewNode:
    gameWin->Debug_NewNode(E->S.currNode);  // Called at entry point of new node
    break;
  case msg_EndNode:
    gameWin->Debug_EndNode(E->S.currNode);  // Called at exit point of new node
    break;
  case msg_NewMove:
    gameWin->Debug_NewMove(E->S.currNode);  // Called when searching next move
    break;
  case msg_Cutoff:
    gameWin->Debug_Cutoff(E->S.currNode);  // Called if cutoff
    break;
#endif

    engine->msgQueue = 0;
} /* GameWindow::ProcessEngineMessage */

static void ProbeEndgameDB(ENGINE *E) {
  CFile efile(nil);
  CHAR fileName[50];
  ULONG bytes = 4, res;

  E->S.edbResult = -1;

  Format(fileName, ":Endgame Databases:%s", E->S.edbName);
  if (efile.Set(fileName, '·EDB') != fileError_NoError) return;
  if (efile.Open(filePerm_Rd) != fileError_NoError) return;

  if (efile.SetPos((E->S.edbPos / 4) * 3) != fileError_NoError) goto close;
  if (efile.Read(&bytes, (PTR)&res) != fileError_NoError) goto close;

  E->S.edbResult = (res >> (26 - 6 * (E->S.edbPos % 4))) & 0x003F;

close:
  efile.Close();
} /* ProbeEndgameDB */

/**************************************************************************************************/
/*                                                                                                */
/*                                     SEARCH TRACING/DEBUGGING */
/*                                                                                                */
/**************************************************************************************************/

#ifdef __debug_GameWin

static BOOL trace_SingleStep = true;
static INT trace_ReturnDepth = -1;

void DebugMob(ENGINE *E);

//--- Called at beginning of each new node ---
// Draws a new line in the debug view

void GameWindow::Debug_NewNode(NODE *N) {
  Debug_DrawTree(N->depth, false, false);
  DebugMob(engine);
} /* GameWindow::Debug_NewNode */

void GameWindow::Debug_EndNode(NODE *N) {
  Debug_DrawTree(N->depth - 1, true, false);
} /* GameWindow::Debug_EndNode */

void GameWindow::Debug_NewMove(NODE *N) {
  Debug_DrawTree(N->depth, true, false);
} /* GameWindow::Debug_NewMove */

void GameWindow::Debug_Cutoff(NODE *N) {
  Debug_DrawTree(N->depth, true, true);
} /* GameWindow::Debug_Cutoff */

static CHAR *Gen = "-ABCDE12GHIJKLrn";

void GameWindow::Debug_DrawTree(INT maxDepth, BOOL drawLeafMove, BOOL cutoff) {
  return;
  infoAreaView->SetNodes(engine->S.moveCount);

  debugView->DrawRectErase(&(debugView->bounds));

  debugView->MovePenTo(5, 12);
  // debugView->DrawStr(" d ply aply bply PV | alpha  beta   score  eval   best
  // | g move    Æply | killer1 killer2 refmove");
  debugView->DrawStr(
      " d ply aply bply PV | PVSum  MobSum | alpha  beta   score  eval   best  "
      "  | g move    Æply mthreat");
  debugView->MovePenTo(5, 15);
  debugView->DrawLine(600, 0);

  for (INT d = 0; d <= maxDepth; d++) {
    NODE *N = &(engine->S.rootNode[d]);
    CHAR s[200], bm[16], ms[50];  // km1[16], km2[16], rm[16];

    if (d < maxDepth || (drawLeafMove && !cutoff)) {
      CHAR tmp[20];
      CalcMoveStr(&(N->m), tmp);
      Format(ms, "%c %7s %4d", Gen[N->gen], tmp, N->m.dply);
    } else if (cutoff)
      CopyStr("cutoff", ms);
    else
      CopyStr("---", ms);

    CalcMoveStr(&(N->BestLine[0]), bm);

    Format(
        s,
        "%2d %3d %4d %4d %s | %6d %6d | %6d %6d %6d %6d %-7s | %-14s",  // |
                                                                        // %7d",
                                                                        // //
                                                                        // %-7s
                                                                        // %-7s",
        d, N->ply, N->sply, N->sply_, (N->pvNode ? "PV" : "  "), N->pvSumEval,
        N->mobEval, N->alpha, N->beta, N->score, N->totalEval, bm,
        ms);  //, N->moveThreat);
              /*
                    CalcMoveStr(&(N->killer1), km1);
                    CalcMoveStr(&(N->killer2), km2);
                    CalcMoveStr(&(N->rfm), rm);
          
                    Format(s, "%2d %3d %4d %4d %s | %6d %6d %6d %6d %-7s | %-14s | %-7s
                 %-7s %-7s",           d, N->ply, N->alphaPly, N->betaPly, (N->pvNode ? "PV" : "           "),
                              N->alpha, N->beta, N->score, N->totalEval, bm,
                              ms, km1,km2,rm);
              */
    debugView->MovePenTo(5, 26 + 12 * d);
    debugView->DrawStr(s);
  }

  if (!trace_SingleStep && maxDepth <= trace_ReturnDepth)
    trace_SingleStep = true;

  if (trace_SingleStep) {
    waiting = true;
    while (waiting) Task_Switch();
  }
} /* GameWindow::Debug_DrawTree */

void GameWindow::Debug_HandleKey(CHAR c, INT key) {
  if (!waiting) return;

  if (c == 's')
    waiting = false;
  else if (IsDigit(c)) {
    waiting = false;
    trace_SingleStep = false;
    trace_ReturnDepth = c - '0';
  }
} /* GameWindow::Debug_HandleKey */

static BOARD_STATE BState;
static ATTACK_STATE AState;

void DebugMob(ENGINE *E) {
  return;
  /*
     // Backup
     BState = E->B;
     AState = E->A;
     INT rootMobEval = E->S.rootNode->mobEval;

     // Calc
     CalcAttackState(E);
     INT realMobEval = E->S.rootNode->mobEval;
     INT currMobEval = E->S.currNode->mobEval;

     // Restore
     E->B = BState;
     E->A = AState;
     E->S.rootNode->mobEval = rootMobEval;

     // Verify:
     if (currMobEval != realMobEval)
     {
        CHAR s[200];
        Format(s, "Mob error: Should be %d iso %d", realMobEval,currMobEval);
        DebugWriteNL(s);
        E->S.currNode->mobEval = realMobEval;
        for (NODE *N = E->S.rootNode; N < E->S.currNode; N++)
        {  CalcMoveStr(&(N->m), s);
           DebugWriteNL(s);
        }
        while (! Button());
        while (Button());
     }
  */
  /*
     for (SQUARE sq = -boardSize1; sq < boardSize2; sq++)       // Clear attack
     table. E->A.AttackW[sq] = E->A.AttackB[sq] = 0;

     Asm_Begin(E);
        INT mobSum = 0;
        for (SQUARE sq = a1; sq <= h8; sq++)                 // Compute attack
     table, pawn if (onBoard(sq) && E->B.Board[sq])                // structure
     and mobility. mobSum += UpdPieceAttack(sq); E->S.rootNode->mobEval =
     mobSum; Asm_End();
  */
  /*
  return;
     DebugWriteNL("--- CHECK MOB ---");

     INT  sumMob = E->S.currNode->mobEval;
     INT  MobVal[boardSize];  // Backup table

     for (SQUARE sq = a1; sq <= h8; sq++)
        MobVal[sq] = E->V.MobVal[sq];

     CalcMobilityState(E);

     if (sumMob != E->V.sumMob)
     {
        CHAR s[100];
        Format(s, "Mob sum error: Is %d instead of %d", sumMob, E->V.sumMob);
        DebugWriteNL(s);
     }

     for (SQUARE sq = a1; sq <= h8; sq++)
        if (onBoard(sq) && E->B.Board[sq] != empty && MobVal[sq] !=
  E->V.MobVal[sq])
        {
           CHAR s[100];
           Format(s, "Mob error at %c%c: Is %d instead of %d", file(sq) + 'a',
  rank(sq) + '1', MobVal[sq], E->V.MobVal[sq]); DebugWriteNL(s);
        }
  */
}

#endif
