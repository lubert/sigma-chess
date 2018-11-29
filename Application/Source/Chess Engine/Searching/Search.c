/**************************************************************************************************/
/*                                                                                                */
/* Module  : SEARCH.C */
/* Purpose : This is the main Engine Search module which controls the root
 * search. Non-root nodes */
/*           are handled in "TreeSearch.c". The special Mate Search routines are
 * defined in the   */
/*           "MateSearch.c" module. */
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

#include "Engine.h"
#include "TaskScheduler.h"

#include "Attack.f"
#include "Board.f"
#include "EndgameDB.f"
#include "Engine.f"
#include "Evaluate.f"
#include "HashCode.f"
#include "MateSearch.f"
#include "Mobility.f"
#include "MoveGen.f"
#include "PerformMove.f"
#include "PieceVal.f"
#include "Search.f"
#include "SearchMisc.f"
#include "Selection.f"
#include "Threats.f"
#include "Time.f"
#include "TransTables.f"

//#define __dumpEloNps 1  //###

/**************************************************************************************************/
/*                                                                                                */
/*                                        MAIN SEARCH ROUTINE */
/*                                                                                                */
/**************************************************************************************************/

static void PrepareSearch(ENGINE *E);
static void PrepareIteration(ENGINE *E);
static void SearchRootNode(ENGINE *E);
static void SearchRootNodeMate(ENGINE *E);
static void EndIteration(ENGINE *E);
static BOOL AnotherIteration(ENGINE *E);
static void EndSearch(ENGINE *E);

void MainSearch(ENGINE *E) {
  E->msgQueue = 0;
  E->R.taskRunning = true;
  SendMsg_Async(E, msg_BeginSearch);

  PrepareSearch(E);

  do {
    PrepareIteration(E);
    if (E->P.playingMode != mode_Mate)
      SearchRootNode(E);
    else
      SearchRootNodeMate(E);
    EndIteration(E);
  } while (AnotherIteration(E));

  EndSearch(E);

  SendMsg_Sync(E, msg_EndSearch);  // <-- Important: Must be a sync call so the
                                   // msg queue is flushed
  E->R.taskRunning = false;
  E->msgQueue = 0;
} /* MainSearch */

/*--------------------------------------- Special UCI Hooks
 * --------------------------------------*/

void MainSearch_BeginUCI(ENGINE *E) {
  E->msgQueue = 0;
  E->R.taskRunning = true;
  SendMsg_Async(E, msg_BeginSearch);

  PrepareSearch(E);
  //...
} /* MainSearch_BeginUCI */

void MainSearch_EndUCI(ENGINE *E) {
  //...
  EndSearch(E);

  //   SendMsg_Sync(E, msg_EndSearch);   // <-- Important: Must be a sync call
  //   so the msg queue is flushed SendMsg_Async(E, msg_EndSearch);   // OK to
  //   be an async call??
  E->R.taskRunning = false;
  //   E->msgQueue = 0;  // Don't call this if events not processed above!!
} /* MainSearch_EndUCI */

/**************************************************************************************************/
/*                                                                                                */
/*                                           ROOT SEARCH */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Prepare Search
 * ---------------------------------------*/

static void CalcRunFlags(ENGINE *E);
static void PrepareSearchTree(ENGINE *E);
static void PrepareMisc(ENGINE *E);
static void ResetRootTab(ENGINE *E);
static void SetPlayingStrength(ENGINE *E);
static void ConsultPosLibrary(ENGINE *E);

static void PrepareSearch(ENGINE *E) {
  // Set state to "root" while preparing search (e.g. Generating root moves)
  E->R.state = state_Root;

  if (E->P.playingMode == mode_Mate) {
    E->P.selection = false;
    E->P.pvSearch = false;
  }

  // First compute the root node (based on side to move):
  E->S.rootNode = (E->P.player == white ? E->S.whiteNode : E->S.blackNode);
  E->S.currNode = E->S.rootNode;

  // Next calc board, attack and transposition table state from search
  // parameters:
  CalcBoardState(E);
  CalcAttackState(E);  // Also resets mobility.
  CalcTransState(E);

  CalcRunFlags(
      E);  // Must be done AFTER "CalcTransState" but BEFORE "GenRootMoves"

  // Then reset search and evaluation state:
  PrepareSearchTree(E);
  PrepareMisc(E);
  CalcPieceValState(
      E);  // <--- MUST be done here since it is needed in the root tab (??)
  CalcEvaluateState(E);
  // CalcMobilityState(E);
  GenRootMoves(E);
  ResetRootTab(E);
  AllocateTime(E);
  SetPlayingStrength(E);  // Must be done after time allocation.
  ConsultPosLibrary(E);   // Must be done here

  // Display initially search results:
  SendMsg_Async(E, msg_NewIteration);
  SendMsg_Async(E, msg_NewMainLine);
  SendMsg_Async(E, msg_NewNodeCount);

  // Finally switch to "running" state (must be done after GenRootMoves).
  E->R.state = state_Running;
} /* PrepareSearch */

static void CalcRunFlags(ENGINE *E) {
  ULONG f = E->R.state;

  if (E->P.pvSearch) f |= rflag_PVSearch;
  if (E->P.extensions) f |= rflag_Extensions;
  if (E->P.selection) f |= rflag_Selection;
  if (E->P.deepSelection) f |= rflag_DeepSel;
  if (E->P.reduceStrength) f |= rflag_ReduceStrength;
  if (E->Tr.transTabOn) f |= rflag_TransTabOn;

  E->R.rflags = f;
} /* CalcRunState */

static void PrepareSearchTree(
    ENGINE *E)  // Initializes the nodes of the search tree.
{
  NODE *N = E->S.rootNode;

  // Initialize the "previous" node:

  N[-1].m = E->P.lastMove;  // Set previous move.
  N[-1].gen = gen_None;
  N[-1].lastPiece_ = E->B.LastPiece[E->B.player];
  N[-1].lastPiece = E->B.LastPiece[E->B.opponent];

  // Perform common initialization of search tree (i.e. all nodes):

  for (INT d = 0; d <= maxSearchDepth; d++) {
    N[d].depth = d;
    N[d].alphaPly = 1000;
    N[d].betaPly = 1000;
    N[d].maxPly = 0;
    N[d].gameDepth = E->P.lastMoveNo + d;
    N[d].drawData = &E->P.DrawData[N[d].gameDepth];
    N[d].loseVal = d - maxVal;
    N[d].program = even(d);
    N[d].isMateDepth = false;
    N[d].bottomNode = false;
    N[d].hungVal1 = (N[d].program ? 15 : 10);
    N[d].hungVal2 = (N[d].program ? 50 : 30);
    N[d].lastPiece = N[d - 1].lastPiece_;
    N[d].lastPiece_ = N[d - 1].lastPiece;
    N[d].killer1Count = 0;
    N[d].killer2Count = 0;
    N[d].killer1Active = false;
    N[d].killer2Active = false;
    clrMove(N[d].rfm);
    clrMove(N[d].killer1);
    clrMove(N[d].killer2);
  }

  N[maxSearchDepth - 1].bottomNode = false;
  N[maxSearchDepth].bottomNode = true;

  // Finally initialize the root node (in particular those fields needed for the
  // generation of the root moves):

  N[0].ply = 0;

  if (!E->P.selection) {
    N[0].alphaPly = N[0].betaPly = 1000;
  } else {
    if (E->P.playingMode == mode_Infinite) {
      N[0].alphaPly = -2;  //-1;
      N[0].betaPly = -3;   //-2;
    } else {
      N[0].alphaPly = -4;  //-2;
      N[0].betaPly = -3;   //-1;
      if (E->P.deepSelection) N[0].alphaPly--;
    }

    // Reduce selection in simple endgames
    INT incPly = Min(3, Max(E->V.phase - 6, 0));
    N[0].alphaPly += incPly;
    N[0].betaPly += incPly;
  }

  clrMove(N[0].m);
  N[0].gen = gen_None;
  N[0].check = (N->Attack_[N->PieceLoc[0]] > 0);
  N[0].quies = false;
  N[0].bufStart = E->S.SBuf;
  N[0].storeSacri = true;
  N[0].selMargin = 0;

  N[0].hashKey = N[0].drawData->hashKey;
} /* PrepareSearchTree */

static void PrepareMisc(ENGINE *E)  // Initialize miscellaneous variables.
{
  E->S.mainDepth = 0;  // Reset nominal search depth (= rootNode->ply).
  E->S.currMove = 0;   // Reset current root move (index).
  E->S.mainScore = 0;  // Reset main score.
  E->S.bestScore = 0;
  E->S.prevScore = mateWinVal;  // Reset previous iteration score.
  E->S.multiPV = 1;
  E->S.MainLine =
      E->S.rootNode->BestLine;  // Reset main line (= best line of root node).
  clrMove(E->S.MainLine[0]);
  clrMove(E->S.bestReply);
  E->S.isPonderMove = false;

  E->S.nodeCount = 0;  // Reset node count.
  E->S.moveCount = 0;  // Reset move count.
  E->S.hashFull = 0;   // = 1000*E->Tr.transUsed/E->Tr.transSize
  E->S.startTime = Timer();
  E->S.searchTime = 0;
  E->S.mainTime = 0;
  E->S.periodicTime = Timer() + 10;  //###
  E->S.periodicCounter = 0;
  E->S.uciNps = 0;

  E->S.bufTop = E->S.SBuf;  // Reset sacrifice buffer.
  E->R.aborted = false;
  E->S.mateDepth = 2 * E->P.depth - 1;
  E->S.mateFound = false;
  E->S.mateTime = Timer();
  E->S.libMovesOnly = false;
  E->S.edbPresent = true;
  E->S.edbMovesOnly = true;

  ResetTransTab(E);
  if (E->P.playingMode != mode_Mate)
    StoreKBNKpositions(E);
  else {
    E->S.rootNode[E->S.mateDepth].isMateDepth = true;
  }
} /* PrepareMisc */

static void ResetRootTab(ENGINE *E) {
  SEARCH_STATE *S = &E->S;
  ROOTTAB *R = S->RootTab;

  for (INT i = 0; i < S->numRootMoves; i++) {
    R[i].i = i;
    R[i].val = 0;
    S->BadLibMove[i] = false;
  }

  if (E->P.playingMode == mode_Infinite)
    for (INT i = 0; i < S->numRootMoves; i++) S->RootMoves[i].dply = 1;

  if (E->P.nextBest)  // If next best and all ignored -> reset anyway!
  {
    INT i;
    for (i = 0; i < S->numRootMoves && E->S.Ignore[i]; i++)
      ;
    if (i == S->numRootMoves) E->P.nextBest = false;
  }

  if (!E->P.nextBest) {
    for (INT i = 0; i < S->numRootMoves; i++) E->S.Ignore[i] = false;
  }

  S->iMain = 0;
} /* ResetRootTab */

/*---------------------------------------- Prepare Iteration
 * -------------------------------------*/

static void SortRootTab(ENGINE *E);
static void StorePrinVar(ENGINE *E);

static void PrepareIteration(ENGINE *E) {
  NODE *N = E->S.rootNode;

  if (E->P.playingMode == mode_Mate) {
    N[0].alpha = maxVal - E->S.mateDepth - 1;
    N[0].beta = maxVal - N->ply;
  } else {
    N[0].alpha = 2 - maxVal;  // Set initial alpha-beta window.
    N[0].beta = maxVal - 1;
  }

  N[0].ply++;       // Increase nominal search depth and
  N[0].alphaPly++;  // selective search depth.
  N[0].betaPly++;

  E->S.mainDepth = N[0].ply;
  E->S.currMove = 0;

  INT phase = Max(5, Min(9, E->V.phase));  // Set the maximum quiescence depth
  INT maxPly =
      ((20 - phase) * E->S.mainDepth) / 10;  // (i.e. maximum number of plies to
  if (E->P.playingMode == mode_Infinite)     // restricted quiescence search).
  {
    maxPly++;
  } else {
    maxPly--;
    if (odd(maxPly)) maxPly--;
  }

  maxPly = Max(maxPly, E->S.mainDepth + 3);  // Restrict maxPly to the interval
  maxPly = Min(maxPly, maxSearchDepth);      // [ply + 3...maxSearchDepth].

  E->S.checkDepth =
      maxPly + (E->P.playingMode == mode_Infinite ? 3 : 1);  //###Testing

  for (INT d = 0; maxPly >= 0; d++) N[d].maxPly = maxPly--;

  if (N[0].ply > 1) {
    SortRootTab(E);
    StorePrinVar(E);
  }

  SendMsg_Async(
      E, msg_NewIteration);  // Tell "host" that new iteration is starting:
} /* PrepareIteration */

static void SortRootTab(ENGINE *E)  // Sort the root move table on the values
{                           // returned by the search of each move in the
  SEARCH_STATE *S = &E->S;  // previous iteration (val > -maxVal).
  ROOTTAB R[maxLegalMoves];

  for (INT i = 0; i < S->numRootMoves; i++) R[i] = S->RootTab[i];

  for (INT i = 0; i < S->numRootMoves; i++) {
    INT vmax = -maxVal;  // Find best move "jmax" among
    INT jmax = 0;        // remaining moves.

    for (INT j = 0; j < S->numRootMoves; j++)
      if (R[j].val > vmax) vmax = R[jmax = j].val;
    R[jmax].val = -maxVal;
    S->RootTab[i] = R[jmax];
  }
} /* SortRootTab */

static void StorePrinVar(
    ENGINE *E)  // Stores the principal variation in the search tree
{               // so that it will be searched first.
  if (E->P.playingMode == mode_Mate)
    return;  // Mate mode does not use principal variation.

  SEARCH_STATE *S = &E->S;
  NODE *N = S->rootNode;
  INT d = 0;

  if (!isNull(S->MainLine[0]))  //###
    while (!isNull(S->MainLine[++d])) {
      N[d].pvNode = true;
      N[d].rfm = E->S.MainLine[d];
    }

  while (d < maxSearchDepth) N[d++].pvNode = false;
} /* StorePrinVar */

/*-------------------------- Perform Iteration - Normal Root Node Search
 * -------------------------*/
// Performs one iteration by searching all the root moves.

static void NoviceAdjust(ENGINE *E);

static void SearchRootNode(ENGINE *E) {
  register SEARCH_STATE *S = &E->S;
  register NODE *N = S->rootNode;
  register ROOTTAB *R = S->RootTab;

  Asm_Begin(E);

  S->nodeCount++;

  S->mainScore = N->score = N->loseVal;  //(E->P.playingMode == mode_Infinite ?
                                         //-1 : N->loseVal); //###
  NN->beta = -N->alpha;

#ifdef __debug_Search
  SendMsg_Async(E, msg_NewNode);
#endif

  //--- For each root move ---

  for (S->currMove = 0;
       S->currMove < S->numRootMoves && E->R.state == state_Running;
       S->currMove++, R++) {
    S->moveCount++;
    Engine_Periodic(E);

    N->m = S->RootMoves[R->i];  // Retrieve next root move to be searched
    N->gen = N->m.misc;
    R->val = 1 - maxVal;

    if (S->Ignore[R->i]) continue;

    SendMsg_Async(E, msg_NewRootMove);

#ifdef __debug_Search
    SendMsg_Async(E, msg_NewMove);
#endif

    NN->ply =
        N->ply - Min(N->m.dply, 1);  // Decrement ply-counter at next node.

    EvalMove();  // Compute move evaluation

    //--- Perform, Search & Retract Move ---

    PerformMove();

    if (S->libMovesOnly) {
      // Perform a quick 1-ply search (in case of transposition errors in the
      // book)
      NN->alpha0 = -N->beta;
      SearchNode();

      // If it's not a "bad" value (i.e. above -50), then replace with random
      // value
      if (N->val > -50) N->val = Rand(20);
      clrMove(NN->BestLine[0]);
    } else if (S->BadLibMove[R->i]) {
      N->val = N->totalEval - 200;
    } else if (!ConsultEndGameDB(E)) {
      S->edbMovesOnly = false;

      if (S->currMove == 0 || !E->P.pvSearch)  // If first move or not PV node
      {
        NN->alpha0 = -N->beta;  // search with full window...
        SearchNode();
      } else  // ...otherwise search with minimal
      {
        NN->alpha0 = -N->alpha - 1;  // window and re-search if fail high.
        SearchNode();
        if (N->val > N->alpha) {
          NN->alpha0 = -N->beta;
          NN->ply = N->ply - Min(N->m.dply, 1);
          SearchNode();
        }
      }
    }

    RetractMove();

    //--- Update Score ---

    if (E->P.playingMode == mode_Novice &&
        !S->libMovesOnly)  // Screw up value if novice mode.
      NoviceAdjust(E);

    if (N->val > N->score &&
        E->R.state == state_Running)  // Update alpha values and main
    {                                 // line if necessary...
      if (E->P.nondeterm &&
          !S->libMovesOnly &&  // Apply nondeterministic content
          N->val > mateLoseVal &&
          N->val < mateWinVal)  // factor (if not mate win/lose).
        N->val += Rand(5);
      S->mainScore = S->bestScore = N->score = N->alpha = R->val = N->val;
      S->scoreType = (S->libMovesOnly ? scoreType_Book : scoreType_True);
      NN->beta = -N->alpha;
      UpdateBestLine();
      S->iMain = R->i;

      if (S->currMove > 0 || S->mainDepth == 1)
        E->S.mainTime = Timer() - E->S.startTime;

      if (N->score >= maxVal - 1 - N->ply)  // Terminate search if a fast mate
      {
        E->R.state = state_Stopping;  // is found.
        E->R.aborted = false;
      }

      SendMsg_Async(E, msg_NewMainLine);
      SendMsg_Async(E, msg_NewScore);
      AdjustTimeLimit(E);
    }
  }

#ifdef __debug_Search
  SendMsg_Async(E, msg_EndNode);
#endif

  Asm_End();
} /* SearchRootNode */

static void SearchRootNodeMate(ENGINE *E) {
  register SEARCH_STATE *S = &E->S;
  register NODE *N = S->rootNode;
  register ROOTTAB *R = S->RootTab;

  Asm_Begin(E);

  S->nodeCount++;

  S->mainScore = N->score = 0;
  NN->beta = -N->alpha;

#ifdef __debug_Search
  SendMsg_Async(E, msg_NewNode);
#endif

  //--- For each root move ---

  for (S->currMove = 0;
       S->currMove < S->numRootMoves && E->R.state == state_Running;
       S->currMove++, R++) {
    S->moveCount++;
    Engine_Periodic(E);

    N->m = S->RootMoves[R->i];  // Retrieve next root move to be searched
    N->gen = N->m.misc;
    R->val = 1 - maxVal;

    if (S->Ignore[R->i]) continue;

    SendMsg_Async(E, msg_NewRootMove);

#ifdef __debug_Search
    SendMsg_Async(E, msg_NewMove);
#endif

    NN->ply =
        N->ply - Min(N->m.dply, 1);  // Decrement ply-counter at next node.

    EvalMove();  // Compute move evaluation

    //--- Perform, Search & Retract Move ---

    PerformMove();

    if (!ConsultEndGameDB(E)) {
      S->edbMovesOnly = false;
      NN->alpha0 = -N->beta;  // search with full window...
      SearchNode();
    }

    RetractMove();

    //--- Update Score ---

    if (N->val > N->alpha &&
        E->R.state == state_Running)  // Update alpha values and main
    {                                 // line if necessary...
      S->mainScore = S->bestScore = N->score = N->alpha = R->val = N->val;
      S->scoreType = scoreType_True;
      NN->beta = -N->alpha;
      UpdateBestLine();
      S->iMain = R->i;

      R->val = 1 - maxVal;
      E->S.Ignore[R->i] = true;

      S->mateFound = true;
      S->mateTime = Timer() - S->mateTime;
      S->mateContinue = false;

      SendMsg_Async(E, msg_NewMainLine);
      SendMsg_Async(E, msg_NewScore);
      SendMsg_Async(E, msg_NewNodeCount);
      SendMsg_Sync(E,
                   msg_MateFound);  // <-- Here host can open "MateFoundDialog"

      // Set by host if user/host wants to continue looking for cooks
      if (S->mateContinue) {
        N->alpha = maxVal - E->S.mateDepth - 1;
        N->beta = maxVal - N->ply;
        clrMove(S->MainLine[0]);
        S->mainScore = N->score = 0;
        NN->beta = -N->alpha;
        S->mateTime = Timer();

        SendMsg_Async(E, msg_NewMainLine);
        SendMsg_Async(E, msg_NewScore);
      }
    }
  }

#ifdef __debug_Search
  SendMsg_Async(E, msg_EndNode);
#endif

  Asm_End();
} /* SearchRootNodeMate */

/*------------------------------------------ End Iteration
 * ---------------------------------------*/

static void EndIteration(ENGINE *E)  // End current iteration.
{
  E->S.prevScore = E->S.mainScore;
} /* EndIteration */

static BOOL AnotherIteration(
    ENGINE *E)  // Should we perform one more iteration?
{
  if (E->P.playingMode != mode_Mate && (E->S.libMovesOnly || E->S.edbMovesOnly))
    return false;

  if (E->S.numRootMoves == 1)      // Stop if only one legal move
    return (E->S.mainDepth < 2 &&  // and a reply is found.
            isNull(E->S.MainLine[1]));

  if (E->P.playingMode !=
      mode_Mate)  // If not mate mode, don't continue if mate is
    if (E->S.mainScore <= mateLoseVal ||  // unavoidable.
        E->S.mainScore >= mateWinVal)
      return false;

  if (E->S.mainDepth ==
      maxSearchDepth)  // Stop if maximum search depth has been reached.
    return false;

  if (E->P.backgrounding)
    return !E->R.aborted;
  else if (E->R.state == state_Stopping || E->R.state == state_Stopped)
    return false;

  switch (E->P.playingMode) {
    case mode_Time:
      return TimeForAnotherIteration(E);
    case mode_FixDepth:
      return E->S.mainDepth < E->P.depth;
    case mode_Mate:
      return E->S.mainDepth <
             E->S.mateDepth;  //### -1 when real mate finder done at some point
    case mode_Infinite:
      return true;
    case mode_Novice:
      return false;
    default:
      return false;  // We should never get here!
  }
} /* AnotherIteration */

/*------------------------------------------- End Search
 * -----------------------------------------*/

static void RecalcPlayingStrength(ENGINE *E);

static void EndSearch(ENGINE *E) {
  if (isNull(E->S.MainLine[0]))  // In the rare case where no move was found we
                                 // simply
  {
    E->S.MainLine[0] = E->S.RootMoves[0];  // return the first legal move (and
                                           // clear the reply).
    clrMove(E->S.MainLine[1]);
  }

  E->S.MainLine[0].misc =
      0;  // Clear move generator, so we avoid strange glyphs

  if (!E->UCI)  // Set best reply/pondering
  {
    E->S.bestReply = E->S.MainLine[1];
    E->S.isPonderMove = true;
  } else if (isNull(E->S.bestReply))
    E->S.bestReply = E->S.MainLine[1];

  E->S.Ignore[E->S.iMain] =
      true;  // Update ignore list so next best can be applied.

  E->S.searchTime =
      Timer() - E->S.startTime;  // Calc elapsed search time (in Ticks).
  RecalcPlayingStrength(E);

  SendMsg_Async(E, msg_NewNodeCount);    // Print final node count.
  while (!E->UCI && E->P.backgrounding)  // Wait if we are backgrounding
    Engine_Periodic(E);
  E->R.state = state_Stopped;  // Go to "stopped" state
} /* EndSearch */

/**************************************************************************************************/
/*                                                                                                */
/*                                       POSITION LIBRARIES */
/*                                                                                                */
/**************************************************************************************************/

// Before the first iteration starts we first quickly scan the list of root
// moves to see if any of these are included it the current position library. If
// so, we "mark" them (and discards all other moves). Subsequently, we only
// perform a 1 ply "search" where we assign a small random value to each move.

static void ConsultPosLibrary(ENGINE *E) {
  if (!E->P.Library) return;

  // Library scores
  //
  // 0: Unplayable (unclassified or winning advantage opponent)
  // 1: Clear advantage opponent
  // 2: Unclear
  // 3: With compensation
  // 4: Slight advantage opponent
  // 5: Level or better

  SEARCH_STATE *S = &E->S;
  NODE *N = S->rootNode;
  HKEY rootKey = N->drawData->hashKey;
  INT maxLibVal = 0;  // Maximum lib value
  INT minLibVal = 6;  // Minimum required lib value

  INT LibVal[maxLegalMoves];
  INT LibValW[libClass_Count] = {0, 5, 2, 5, 5, 5, 3, 4, 1, 0, 5};
  INT LibValB[libClass_Count] = {0, 5, 2, 4, 1, 0, 5, 5, 5, 5, 3};

  for (INT i = 0; i < S->numRootMoves; i++) {
    HKEY varKey = rootKey ^ HashKeyChange(E->Global, &(S->RootMoves[i]));
    LIB_CLASS libClass = ProbePosLib(E->P.Library, E->B.opponent, varKey);

    LibVal[i] = (E->P.player == white ? LibValW[libClass] : LibValB[libClass]);
    maxLibVal = Max(LibVal[i], maxLibVal);

    if (libClass == libClass_WinningAdvW || libClass == libClass_WinningAdvB)
      S->BadLibMove[i] = true;
  }

  switch (E->P.libSet)  // Compute minimum acceptable library value
  {
    case libSet_Full:
      if (maxLibVal >= 2) minLibVal = 1;
      break;
    case libSet_Wide:
      if (maxLibVal >= 3) minLibVal = 2;
      break;
    case libSet_Tournament:
      if (maxLibVal >= 4) minLibVal = maxLibVal;
      break;
    case libSet_Solid:
      if (maxLibVal >= 5) minLibVal = 5;
      break;
  }

  if (maxLibVal >= minLibVal) {
    S->libMovesOnly = true;
    if (!E->UCI) E->P.reduceStrength = false;
    for (INT i = 0; i < S->numRootMoves; i++)
      S->Ignore[i] = (LibVal[i] < minLibVal);
  }
} /* ConsultPosLibrary */

/**************************************************************************************************/
/*                                                                                                */
/*                               PLAYING STRENGTH - ELO/NPS CONVERSION */
/*                                                                                                */
/**************************************************************************************************/

// ELO/Nps conversion formulas:
//
//    eloNpsFactor = exp(100*ln(npsMax/npsMin)/(eloMax - eloMin))
//
//    nps(elo) = npsMin*eloNpsFactor^((elo - eloMin)/100)
//
//    elo(nps) = eloMin + 100*ln(nps/npsMin)/ln(eloNpsFactor)

// ELO/Nps conversion constants for the standard/base configuration:
//
// � 40 moves in 2 hours (i.e. 180 secs per move)
// � permanent brain on
// � normal playing style
// � 2.5 Mb Hash

#define eloMin 1200
//#define npsMin       15           // Corresponding to eloMin in standard
//configuration
#define npsMin 25  // Corresponding to eloMin in standard configuration
//#define eloNpsFactor 2.350      // nps1200 = 15, nps2500 = 1000000
#define eloNpsFactor 2.259  // nps1200 = 25, nps2500 = 1000000

static LONG ELO_to_Nps(INT elo);
static INT Nps_to_ELO(LONG nps);
static INT AdjustELO(ENGINE *E);

/*-------------------------------- Set Playing Strength (Max NPS)
 * --------------------------------*/
// The playing strength is controlled by reducing the NPS according to the
// specified ELO rating.

static void SetPlayingStrength(ENGINE *E) {
  E->S.eloAdjust = AdjustELO(E);  // Adjust for blitz, hash tables etc.
  E->S.npsTarget = 1000000;

  if (!E->P.reduceStrength) return;

  E->S.eloTarget = E->P.engineELO - E->S.eloAdjust;
  E->S.npsTarget = ELO_to_Nps(E->S.eloTarget);

#ifdef __dumpEloNps
  CHAR s[100];
  Format(s, "ELO Target : %d, NPS : %ld", E->S.eloTarget, E->S.npsTarget);
  DebugWriteNL(s);

  for (INT elo = eloMin; elo <= 2500; elo += 100) {
    Format(s, "ELO : %d, NPS : %ld", elo, ELO_to_Nps(elo));
    DebugWriteNL(s);
  }
  for (LONG nps = npsMin; nps <= ELO_to_Nps(2500); nps *= 2) {
    Format(s, "NPS : %6ld, ELO : %d", nps, Nps_to_ELO(nps));
    DebugWriteNL(s);
  }
#endif
  /*
     E->S.eloTimer     = Timer() + 8;
     E->S.eloTMoves    = (nps*8)/60;
     E->S.eloTMCounter = E->S.eloTMoves;
  */
} /* SetPlayingStrength */

/*------------------------------------ Actual Playing Strength
 * -----------------------------------*/
// When the search completes we compute the actual/effective playing strength by
// converting back from the actual NPS to ELO rating. If the actual ELO rating
// is lower (within a margin) than the specified ELO rating, the host
// application could e.g. notify the user that the machine is too slow.

static void RecalcPlayingStrength(ENGINE *E) {
  ULONG nps = (60 * E->S.moveCount) / E->S.searchTime;

  if (E->S.moveCount < 100000 ||
      E->S.searchTime < 120)  // Ignore if too brief search
    E->P.actualEngineELO = E->P.engineELO;
  else
    E->P.actualEngineELO = Nps_to_ELO(nps) + E->S.eloAdjust;

#ifdef __dumpEloNps
  CHAR s[100];
  Format(s, "Actual/achieved ELO : %d (%d), NPS : %ld (%ld)",
         E->P.actualEngineELO, Nps_to_ELO(nps), nps,
         ELO_to_Nps(E->P.actualEngineELO));
  DebugWriteNL(s);
#endif
} /* RecalcPlayingStrength */

/*-------------------------------------- ELO/NPS Conversion
 * --------------------------------------*/

LONG ELO_to_Nps(INT elo) {
  return (LONG)(npsMin * pow(eloNpsFactor, (REAL)(elo - eloMin) / 100.0));
} /* ELO_to_Nps */

INT Nps_to_ELO(LONG nps) {
  return eloMin + 100 * log((REAL)nps / (REAL)npsMin) / log(eloNpsFactor);
} /* Nps_to_ELO */

/*----------------- ELO Adustment caused by deviation from standard
 * configuration  ---------------*/
// Computes an ELO adjustment offset based on the current search parameters. For
// the standard/base configuration the adjustment is 0. Deviations are handled
// as follows:
//
// � For shorter time controls the effective ELO against human opponents
// increases (up to 200 ELO
//   in blitz). This adjustment is only done in the mode_Time playing mode.
// � If permanent brain is off, the ELO is decreased by 50 ELO points.
// � Transposition tables: Doubling (resp. halving) the size corresponds to 5
// (resp -5) ELO points.
//   If the transposition tables are off an additional penalty of 50 ELO points
//   is included.

#define transDoubleVal 8  // ELO increase for doubling trans tables.

INT AdjustELO(ENGINE *E) {
  INT diff = 0;

  // Reduce ELO strength if permanent brain off:
  if (!E->P.permanentBrain) diff -= 30;

  // Reduce strength if small hash tables (default setting 10 MB hash)
  diff -= 5 * transDoubleVal;
  LONG t = E->P.transSize / 1024;
  while (t > 80) t >>= 1, diff += transDoubleVal;

  return Min(diff, 200);
} /* AdjustELO */

/**************************************************************************************************/
/*                                                                                                */
/*                                            NOVICE MODE */
/*                                                                                                */
/**************************************************************************************************/

// The novice levels are implemented by modifying the value returned from the
// search for certain moves: The value of forced moves (i.e en prise captures)
// is decreased with a probability depending on the current novice level, so the
// program won't capture all the pieces left en prise by the player. Similarly,
// the value of sacrifice moves can be increased - again with a probability
// depending on the current novice level - so that the program will deliberatly
// put moves en prise (or underpromote).

// For enprise captures, the following is done: The higher the value of the
// captured piece, the lower the probability of "overlooking" the move.

static void NoviceAdjust(ENGINE *E) {
  SEARCH_STATE *S = &E->S;
  NODE *N = S->rootNode;

  LONG twait = Timer() + (13 - E->P.depth);  // Perform a 2 tick delay.
  while (Timer() <= twait) Engine_Periodic(E);

  for (INT d = 0; d < 10; d++) N[d].bottomNode = false;
  INT dmax = 2;
  switch (E->P.depth) {
    case 1:
      dmax = 2;
      break;  // 2
    case 2:
      dmax = 2 + Rand(2);
      break;  // 2..3
    case 3:
      dmax = 2 + Rand(3);
      break;  // 2..4
    case 4:
      dmax = 3 + Rand(2);
      break;  // 3..4
    case 5:
      dmax = 3 + Rand(3);
      break;  // 3..5
    case 6:
      dmax = 4 + Rand(2);
      break;  // 4..5
    case 7:
      dmax = 4 + Rand(3);
      break;  // 4..6
    case 8:
      dmax = 5 + Rand(3);
      break;  // 5..7
  }
  N[dmax].bottomNode = true;

  /*
     if (E->P.depth == 5)                               // Never adjust in the
     highest novice level. return;

     if (N->val <= N->score ||                          // Only adjust if move
     is better and S->currMove >= S->numRootMoves - 3)            // not among
     the last 3 moves. return;

     if (E->P.lastMoveNo >= 1 &&                        // Never skip
     recaptures. E->P.lastMove.cap && E->P.lastMove.to == N->m.to) return;

     if (N->val >= mateWinVal)                          // Only play mates if
     either:
     {
        if (N->score > 1000 ||                          // � Already much ahead
            E->P.depth > 1 && N->val >= maxVal - 3 ||   // � Not absolute
     beginner and obvious (i.e. fast) mate. N->score < 0) // � If no good moves
     found yet. return;

        N->val = N->score;                              // If none of the above:
     Skip move.
     }
     else                                               // Non mates:
     {
        if (N->m.dply == 0 && N->m.cap && pieceType(N->m.cap) <= Rand(10 -
     2*E->P.depth) || N->m.dply == 2 && ! N->m.cap && ! isPromotion(N->m) &&
     Rand(10) >= 3 + E->P.depth)
        {
           // This move is too "good", so don't play it!

           if (N->score <= mateLoseVal)
              N->val -= 1000;
           else
              N->val = N->score;
        }
     }
  */
} /* NoviceAdjust */

/**************************************************************************************************/
/*                                                                                                */
/*                                     SEARCH STATE INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

// When a new engine is created/allocated, the InitSearchState() routine must be
// called. It sets up various "read only" values in the search nodes e.t.c.

void InitSearchState(ENGINE *E) {
  SEARCH_STATE *S = &E->S;
  BOARD_STATE *B = &E->B;
  ATTACK_STATE *A = &E->A;
  NODE *N = S->_Nodes;

  // Initialize main line e.t.c.:

  S->numRootMoves = 0;
  S->whiteNode = &N[2];
  S->blackNode = &N[1];
  S->mainDepth = 0;
  S->currMove = 0;
  S->mainScore = 0;
  S->bestScore = 0;
  S->MainLine = S->whiteNode->BestLine;
  clrMove(S->MainLine[0]);
  clrMove(S->RootMoves[0]);

  // Initialize the search nodes:

  for (INT d = 0; d <= maxSearchDepth + 2; d++, N++) {
    N->pvNode = N->bottomNode = false;
    N->killer1Active = false;
    N->killer2Active = false;

    if (even(d)) {
      N->player = white;
      N->opponent = black;
      N->pawnDir = +0x10;
      N->Attack = A->AttackW;
      N->Attack_ = A->AttackB;
      N->PieceLoc = B->PieceLocW;
      N->PieceLoc_ = B->PieceLocB;
    } else {
      N->player = black;
      N->opponent = white;
      N->pawnDir = -0x10;
      N->Attack = A->AttackB;
      N->Attack_ = A->AttackW;
      N->PieceLoc = B->PieceLocB;
      N->PieceLoc_ = B->PieceLocW;
    }
  }
} /* InitSearchState */

/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

void InitSearchModule(GLOBAL *G) {
  // At the moment no initialization needed!!
} /* InitSearchModule */
