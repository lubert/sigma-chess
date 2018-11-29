/**************************************************************************************************/
/*                                                                                                */
/* Module  : NODESEARCH.C                                                                         */
/* Purpose : This module controls all searching of non-root nodes.                                */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include "Engine.h"
#include "TaskScheduler.h"

#include "Search.f"
#include "SearchMisc.f"
#include "Selection.f"
#include "Threats.f"
#include "Board.f"
#include "Attack.f"
#include "PieceVal.f"
#include "Evaluate.f"
#include "MoveGen.f"
#include "PerformMove.f"
#include "Time.f"
#include "TransTables.f"
#include "Engine.f"
#include "HashCode.f"


static void SearchNodeC (register ENGINE *E, register NODE *N);
static void SearchMoveC (register ENGINE *E, register NODE *N);
static asm void ExitNode (void);


/**************************************************************************************************/
/*                                                                                                */
/*                                          SEARCH NODE                                           */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------- Search Node (ASM Wrapper) -----------------------------------*/
// This routine recursively searches all non root nodes (i.e. depth > 0).

// ON ENTRY: The global register rNode is expected to point to the previous/parent node "PN".
// Also, N->alpha0, N->beta and N->ply must have been set by the calling routine.

// ON EXIT: N->BestLine and N->score holds the best line and value found by the search at node "N".
// Also PN->val is set to -N->score.

void __exit_node (void);      // Entry point for exiting node directly in case of cutoff:

asm void SearchNode (void)
{
   //--- Increment node (and swap color dependant registers) ---

   addi    rNode, rNode,sizeof(NODE)                    // N++;
   neg     rPlayer, rPlayer                             // rPlayer = black - rPlayer
   neg     rPawnDir, rPawnDir
   addi    rPlayer, rPlayer,0x10
   stw     rNode, ENGINE.S.currNode(rEngine)
   mr      rTmp2, rAttack
   mr      rTmp3, rPieceLoc
   mr      rAttack, rAttack_
   mr      rPieceLoc, rPieceLoc_
   mr      rAttack_, rTmp2
   mr      rPieceLoc_, rTmp3

   //--- Save Processor state in case of cutoff ---

   mflr    r0
   stmw    rLocal7, NODE.cutEnv.gpr(rNode)
   stw     r0, NODE.cutEnv.lr(rNode)
   stw     SP, NODE.cutEnv.sp(rNode)

   //--- Call C-Search routine ---

   stwu    r0,-12(SP)
   mr      rTmp1, rEngine
   mr      rTmp2, rNode
   bl      SearchNodeC                                  // SearchNodeC(E,N);
   lwz     r0, 0(SP)
   addi    SP, SP,12
   mtlr    r0

   //--- Decrement node (and swap color dependant registers) ---

entry      __exit_node                                  // Entry point in case of cutoffs
   addi    rNode, rNode,-sizeof(NODE)                   // N--;
   neg     rPlayer, rPlayer                             // rPlayer = black - rPlayer
   neg     rPawnDir, rPawnDir
   addi    rPlayer, rPlayer,0x10
   stw     rNode, ENGINE.S.currNode(rEngine)
   mr      rTmp2, rAttack
   mr      rTmp3, rPieceLoc
   mr      rAttack, rAttack_
   mr      rPieceLoc, rPieceLoc_
   mr      rAttack_, rTmp2
   mr      rPieceLoc_, rTmp3
   blr
} /* SearchNode */

/*----------------------------------------- Search Node ------------------------------------------*/

void SearchNodeC (register ENGINE *E, register NODE *N)
{

   /* - - - - - - - - - - - - - - - - - - - Periodics - - - - - - - - - - - - - - - - - - - - */
   // Only check for periodics every rarely because Timer() is a VERY expensive call under
   // OS X!! (probably because it's an out-of-process call).

   if (--E->S.periodicCounter <= 0)
   {
      Engine_Periodic(E);
      E->S.periodicCounter = E->S.npsTarget >> 6;

      if (E->P.reduceStrength)     // Reduce ELO strength
      {
         while (E->S.moveCount >= E->S.npsTarget*((Timer() - E->S.startTime)/60))
            Engine_Periodic(E);
      }
   }

   /* - - - - - - - - - - - - - - - - - Compute Parameters - - - - - - - - - - - - - - - - - -*/

   N->alpha = N->alpha0;
   if (N->ply < 0) N->ply = 0;                           // Adjust "ply" counter to the
   else if (N->ply > N->maxPly) N->ply = N->maxPly;      // interval [0..maxPly].

   /* - - - - - - - - - - - - - - - - - - Preprocess Node - - - - - - - - - - - - - - - - - - */

   clrMove(N->BestLine[0]);                              // No move has been found yet.

   #ifdef __debug_Search
      SendMsg_Async(E, msg_NewNode);
   #endif

   UpdateDrawState();                                    // Return in case of a true draw or
   if (N->drawType != drawType_None)                     // first repetition (unless depth = 2).
   {
      N->score = drawVal;
      if (N->depth != 2 || N->drawType >= drawType_Rep2)
         goto exit;
   }

   if (ProbeTransTab())                                  // Probe transposition table.
   {
      goto exit;
   }

   N->check = (N->Attack_[N->PieceLoc[0]] > 0);          // Is the player in check?

   if (! N->check && N->ply > 0)                         // Undo futile extensions.
   {                                                     // Must be done here, because it may
      if (PN->m.dply == 0 && (N-2)->gen == gen_I &&      // change the "ply" counter (and
          PN->m.to == (N-2)->m.to && /*! (N-2)->m.cap &&*/   // hence "N->quies" and threat category...)
          PN->threatEval == 0)
         N->ply--;
      else if (N->depth >= E->S.mainDepth)
         if (N->totalEval - N->threatEval >= N->beta + 50) N->ply--;
   }

   N->quies = (N->ply <= 0 && ! N->check);               // Is this a quiescence node?
   AnalyzeThreats();                                     // Analyze threats.
	Evaluate();															// Compute static evalutation.

   if (N->bottomNode || (N->isMateDepth && ! N->check))  // Return if bottom node or max mate
   {  N->score = N->totalEval;                           // depth reached.
      goto exit;
   }

   if (N->quies)                                         // Compute worst case evaluation.
   {
      if (N->drawType == drawType_None)
         N->score = N->totalEval - N->threatEval;
      if (N->score > N->alpha)
         if (N->score >= N->beta) goto exit;
         else N->alpha = N->score;
      N->alphaPly = N->betaPly = (E->P.selection ? 0 : 1000);  // Always select in quiescence node.
   }
   else 
   {
      if (N->drawType == drawType_None)
         N->score = N->loseVal;
      else if (drawVal > N->alpha)                       // In case of first repetition, give
         if (drawVal >= N->beta) goto exit;              // program opportunity to improve the
         else N->alpha = drawVal;                        // draw score "drawVal".

      N->alphaPly = PN->betaPly - Min(PN->m.dply, 1);
      N->betaPly  = PN->alphaPly - PN->m.dply;
      if (PN->m.dply == 2 && (PN->m.cap || PN->m.type != normal || N->threatEval > 0))
         N->betaPly++;
   }

   NN->beta     = -N->alpha;                             // Set � value at next ply.
   N->bufStart  = E->S.bufTop;                           // Remember old state of "SBuf".
   N->gen       = N->bestGen = gen_None;                 // Reset current/best move generator.
   N->firstMove = true;                                  // We are about to search first move
   N->canMove   = false;                                 // and none has been searched yet.
   PrepareKillers();                                     // Prepare killer table.
   if (N->alphaPly <= 0) ComputeSelBaseVal();            // Compute selective base value.

   /* - - - - - - - - - - - - - - - - - - - - Search Node - - - - - - - - - - - - - - - - - - */

   if (! isNull(N->rfm))                                 // Search refutation move (if any).
   {
      SearchMoveC(E, N);
   }

   if (N->check)                                         // --- CHECK EVASION ---
   {
      N->m.dply = 0;
      N->storeSacri = true;                              // Search and extend check evasion
      SearchCheckEvasion();                              // moves (including sacrifices).
      if (N->score == N->loseVal &&                      // Force cutoff at previous node in
          PN->beta > -N->loseVal)                        // case of mate.
         PN->beta = -N->loseVal;
   }
   else
   {
      N->m.dply = 1;                                     // Forced moves (dply = 0).
      N->storeSacri = (N->maxPly > 0);                   // Store sacrifices?
      SearchEnPriseCaptures();                           // Search forced captures and queen
      N->m.dply = 0;                                     // Forced moves (dply = 0).
      SearchPromotions();                                // promotions.
      SearchRecaptures();
      N->m.dply = 1;                                     // Non-forced moves (dply = 1).
      SearchSafeCaptures();                              // Search non-forced, safe captures.

      if (! N->quies)                                    // --- FULL WIDTH SEARCH ---
      {
         if (N->escapeSq != nullSq)                      // Search escapes.
         {
            if (N->totalEval < N->alpha - 30 ||               // Do not extend search, if escapes
                N->totalEval - N->threatEval > N->beta + 30)  // are not forced/interesting.
               N->eply = 1;
            SearchEscapes();
         }

         SearchCastling();                               // Search castling, killer and safe non-captures.
         N->m.dply = 2;                                  // Quiet moves (dply = 2):
         SearchKillers();
         SearchNonCaptures();
         N->selMargin -= 50;                             // Search sacrifices and punish use- 
         SearchSacrifices();                             // less sacrifices during selection.

         if (! N->canMove) N->score = drawVal;           // If we are stalemate return drawVal.
      }
      else if (N->maxPly > 0)                            // --- QUIESCENCE SEARCH (SHALLOW) ---
      {
         if (N->escapeSq != nullSq)                      // Search forced/interesting escapes.
         {
            if (N->totalEval < N->alpha - 30)
               N->escapeSq = nullSq;
            else
               N->storeSacri = false,                    // Don't search sacrifice escapes.
               SearchEscapes();
         }

         SearchSafeChecks();                             // Search safe checks
         SearchFarPawns();                               // Pawn moves to the 6th or 7th rank.
         if (N->depth - E->S.mainDepth <= 1)             // Search �shallow� sacrifices.
         {  if (N->program) N->selMargin -= 50;          // Punish the program's sacrifices.
            SearchSacrifices();
         }
      }
      else                                               // --- QUIESCENCE SEARCH (DEEP) ---
      {
         if (N->depth < E->S.checkDepth)  //### Experiment
            SearchSafeChecks();
//       SearchFarPawns();                               // Pawn moves to the 6th or 7th rank.
      }
   }

   /* - - - - - - - - - - - - - - - - - - - - Exit Node - - - - - - - - - - - - - - - - - - - */
   // Cut Off results in a jump "here" (to the same code in the "CutOff" routine):

   UpdateKillers();                                      // Update killer table.
   if (N->score != 0) StoreTransTab();                   // Update transposition table.

// E->S.genMoveCount += (E->S.bufTop - N->bufStart);
   E->S.bufTop = N->bufStart;                            // Restore old state of "SBuf".

exit:
   N->pvNode  = false;                                   // This is no longer a PV node.
   PN->val = -N->score;                                  // "Return" score.

#ifdef __debug_Search
   SendMsg_Async(E, msg_EndNode);
// if (showTree) UntraceMove(N);
#endif
} /* SearchNodeC */


/**************************************************************************************************/
/*                                                                                                */
/*                                          SEARCH MOVE                                           */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Search Move ------------------------------------------*/
// This routine searches one move. Is called directly by the various move generator routines.

asm void SearchMove (void)
{
   mr      rTmp1, rEngine
   mr      rTmp2, rNode
   call_c(SearchMoveC)
   blr
} /* SearchMove */


static void SearchMoveC (register ENGINE *E, register NODE *N)
{
   E->S.moveCount++;

   /* - - - - - - - - - - - - - - - - Prepare Move Search - - - - - - - - - - - - - - - - - -*/

   if (N->firstMove && ! isNull(N->rfm))
      if (! N->pvNode && ! GetTransMove()) return;       // Retrieve refutation move
      else N->m = N->rfm;
   else if (KillerRefCollision())                        // Handle killer/refutation move
      return;                                            // collision.

   EvalMove();                                           // Compute move evaluation (pvSum and mobility only).
   NN->ply = N->ply - Min(N->m.dply, 1);                 // Decrement ply-counter at next node.
 
#ifdef __debug_Search
   SendMsg_Async(E, msg_NewMove);
// if (showTree) TraceMove(N);
#endif

   // Check selection/forward pruning (Note: It's very important that we NEVER prune the moves
   // in the PV line (otherwise the NN->pvNode is not cleared properly).

   if (N->alphaPly <= 0 && N->depth >= 2 && ! (N->pvNode && N->firstMove) && ! SelectMove())
   {
      N->firstMove = false;
      N->canMove = true;      // Not strictly true but works in most cases!!
      return;
   }

   /* - - - - - - - - - - - - - - Perform, Search and Retract Move - - - - - - - - - - - - - */

   PerformMove();                                        // Perform move.

      if (N->Attack_[N->PieceLoc[0]])                    // Skip search if it's illegal.
      {  RetractMove();
         N->firstMove = false;
         return;
      }

      N->canMove = true;                                 // Strictly legal move -> Now we REALLY can move

      if (N->isMateDepth)                                // If mate finder and the "loosing" side
      {                                                  // is not mate at the mate depth, exit
         N->score = N->beta;                             // and cutoff
      }
      else if (N->firstMove || ! E->P.pvSearch ||
          N->beta == N->alpha0 + 1 ||
          ! N->pvNode)                                   // If first move or not PV node
      {
         NN->alpha0 = -N->beta;                          // search with full window...
         SearchNode();
         N->firstMove = false;
      }
      else                                               // ...otherwise search with minimal
      {
         NN->alpha0 = -N->alpha - 1;                     // window and re-search if fail high.
         SearchNode();

         if (N->val > N->alpha)
         {  NN->alpha0 = -N->beta;
            NN->ply = N->ply - Min(N->m.dply, 1);
            SearchNode();
         }
      }

   RetractMove();                                        // Retract move.

   /* - - - - - - - - - - - - - - - - - - End Move Search - - - - - - - - - - - - - - - - - -*/

   if (N->val > N->score)                                // Update alpha/beta values and the
   {                                                     // best line.
      N->score   = N->val;
      N->bestGen = N->gen;
      UpdateBestLine();

      if (N->score > N->alpha)                           // If new score better than alpha value...
      {
         if (N->score >= N->beta)                        // then if cutoff then return score and exit node:
         {
            UpdateKillers();                             //    Update killer table.
            StoreTransTab();                             //    Update transposition table.
            E->S.bufTop = N->bufStart;                   //    Restore old state of "SBuf".
            N->pvNode   = false;                         //    This is no longer a PV node.
            PN->val     = -N->score;                     //    "Return" score.

         #ifdef __debug_Search
            SendMsg_Async(E, msg_Cutoff);
           // if (showTree) UntraceMove(N);
         #endif
            ExitNode();                                  //    Exit node (long jump).
         }
         else                                            // else simply update alpha and beta values
         {  N->alpha = N->score;
            NN->beta = -N->alpha;
         }
      }

      if (N->pvNode && E->R.state == state_Running)      // Update main value if this  is a PV node.
      {
         E->S.mainScore = (N->program ? N->score : -N->score);
/*
         E->S.bestScore = E->S.mainScore;                // Report score back to host.
         E->S.scoreType = scoreType_Temp;
         SendMsg_Async(E, msg_NewScore);
*/
      }
   }
} /* SearchMoveC */

/*----------------------------------------- Cut off Handling -------------------------------------*/
// Cutoffs are handled by jumping out of the current "SearchMove" call and back to the exit point
// of the calling "SearchNode" routine. In praxis this is achieved by calling the "ExitNode" routine
// which restores the processor state and then long jumps directly to the "__exit_node" entry point
// in the "SearchNode" routine.

static asm void ExitNode (void)      // Restore processor state (lr, rSP, rLocal1 - rLocal7):
{
   lwz     r0, NODE.cutEnv.lr(rNode)
   lmw     rLocal7, NODE.cutEnv.gpr(rNode)
   lwz     SP, NODE.cutEnv.sp(rNode)
   mtlr    r0
   b       __exit_node
} /* ExitNode */

