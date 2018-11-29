/**************************************************************************************************/
/*                                                                                                */
/* Module  : EVALUATE.C                                                                           */
/* Purpose : This is the main Engine Evaluation module. The piece values are added to the pawn    */
/*           structure evaluation and the special end game evaluation. Additionally, this module  */
/*           contains the main incremental evaluation routines.                                   */
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

#include "Evaluate.f"
#include "Engine.f"
#include "Mobility.f"

// At the root node the total evaluation is computed from scratch by "CalcEvaluateState()". Then
// during the search, the evaluation for the next node NN is computed incrementally based on the
// following evaluation components from the current node N:
//
// pvSumEval (The piece value sum of the current board)
// ----------------------------------------------------
// NN->pvSumVal = N->pvSumVal + pvChange(N->m)
//
// pawnStructEval (The pawn structure evaluation)
// ----------------------------------------------
// If N->m is a pawn move, a pawn capture or a promotion (i.e. if N->m changes the pawn structure),
// NN->pawnStructEval is computed from scratch by the "EvalpawnStruct" routine. Otherwise we simply
// copy N->pawnStructEval to NN->pawnStructEval.
//
// endGameEval (The special endgame evaluation)
// --------------------------------------------
// For certain material configurations a special value is computed.
//
// This evaluation is always done BEFORE performing the move in order to speed up the search by
// pruning (selecting) moves without performing them.
// ### optimization : If move looks very "bad" and it's going to be skipped by the selection scheme
// anyway, make "fast" evaluation.

static asm INT EvalMovePV   (void);
static asm INT EvalPawnStruct (void);
static asm INT EvalEndGame    (void);

static void EvalPawnStructRoot (ENGINE *E);


/**************************************************************************************************/
/*                                                                                                */
/*                                    ROOT EVALUATION FUNCTION                                    */
/*                                                                                                */
/**************************************************************************************************/

void CalcEvaluateState (ENGINE *E)       // NOTE: Must be called AFTER "CalcPieceValState()".
{
   NODE *N = E->S.rootNode;

   Asm_Begin(E);

      // Calc PieceValue sum evaluation:
      N->pvSumEval = E->V.sumPV;
      
      // Calc pawn structure evaluation:
      EvalPawnStructRoot(E);
      EvalPawnStruct();

      // Calc end game evaluation (turn PN->m temporarily into capture to force recomputation!):
      PN->m.cap++;
      N->endGameEval = EvalEndGame();
      PN->m.cap--;

      // Finally calc total evaluation for root node:
      N->totalEval = N->pvSumEval + N->mobEval + N->pawnStructEval + N->endGameEval;  //### TEMP: IGNORE MOBVAL
      if (N->player == black) N->totalEval = -N->totalEval;

      N->capSelVal = 0;

   Asm_End();
} /* CalcEvaluateState */


/**************************************************************************************************/
/*                                                                                                */
/*                                EVALUATE SINGLE MOVE (INCREMENTALLY)                            */
/*                                                                                                */
/**************************************************************************************************/

// The "EvalMove" routine is the heart of the evaluation routine. It evaluates the current move
// incrementally and updates the evaluation components and total at the next node.

asm INT EvalMove (void)
{
// #define eval rLocal1

   frame_begin0

   bl      EvalMovePV                        // EvalMovePV();
/*
   mr      eval, rTmp1
   bl      EvalMoveMob                       // EvalMoveMob();
   li      rTmp1, 0             //### TEMP: IGNORE MOBVAL
   add     rTmp1, eval,rTmp1
*/
   cmpi    cr5,0, rPlayer,black              // NN->totalEval = NN->pvSumEval + NN->mobEval;
   beq     cr5, @E                           // if (player == black)
   neg     rTmp1, rTmp1                      //    NN->totalEval = -NN->totalEval;
@E sth     rTmp1, nnode(totalEval)

   frame_end0                                // return NN->totalEval;
   blr
} /* EvalMove */


/**************************************************************************************************/
/*                                                                                                */
/*                                      EVALUATE POSITION                                         */
/*                                                                                                */
/**************************************************************************************************/

#define pPassSq(n)  (NODE.PassSqW - sizeof(NODE) + n)(rNode)
#define PassSq(n)   (NODE.PassSqW + n)(rNode)

asm void Evaluate (void)
{
   frame_begin0

   //--- Evaluate pawn structure ---
   // If last move did NOT affect the pawn structure, we simply copy the pawn structure evaluation
   // of the current node (including the PassSq[] tables).

@PAWNSTRUCT
   lhz     rTmp1, pmove(piece)                   // if (PN->m.piece == pawn ||
   lhz     rTmp2, pmove(cap)
   andi.   rTmp1, rTmp1, 0x06                    //     PN->m.cap == pawn ||
   cmpi    cr5,0, rTmp2,empty   
   beq     @RecalcPawnStruct                     //     (PN->m.cap && 
   beq     cr5, @CopyPawnStruct
   andi.   rTmp2, rTmp2, 0x06
   beq     @RecalcPawnStruct
   andi.   rTmp0, rPieceCount,0xFFF0             //      ("white has no offi" ||
   bne+    @CopyPawnStruct
   andis.  rTmp0, rPieceCount,0xFFF0             //       "black has no offi")))
   bne+    @CopyPawnStruct                       // {
@RecalcPawnStruct
   bl      EvalPawnStruct                        //    EvalPawnStruct();
   b       @ENDGAME                              // }
@CopyPawnStruct                                  // else
   lwz     rTmp1, pPassSq(0)                     // {
   lwz     rTmp2, pPassSq(4)                     //    CopyList(PN->PassSqW, N->PassSqW);
   lwz     rTmp3, pPassSq(8)
   lwz     rTmp4, pPassSq(12)
   lwz     rTmp5, pPassSq(16)                    //    CopyList(PN->PassSqB, N->PassSqB);
   lha     rTmp6, pnode(pawnStructEval)         
   stw     rTmp1, PassSq(0)
   stw     rTmp2, PassSq(4)
   stw     rTmp3, PassSq(8)
   stw     rTmp4, PassSq(12)                     //    N->pawnStrEval = PN->pawnStrEval;
   stw     rTmp5, PassSq(16)
   sth     rTmp6, node(pawnStructEval)           // }

   //--- Evaluate end game ---
   // In certain endgames with limited material left, the score is reduced (based on pvSumEval)

@ENDGAME
   bl      EvalEndGame                           // N->endGameEval = EvalEndGame();
   sth     rTmp1, node(endGameEval)
   mr      rTmp9, rTmp1                          // N->capSelVal = N->endGameEval;

   //--- Total Evaluation ---
   lha     rTmp2, node(pvSumEval)                // N->totalEval = N->pvSumEval +
   lha     rTmp3, node(mobEval)                  //                N->mobEval +
// li      rTmp3, 0             //### TEMP: IGNORE MOBVAL
   lha     rTmp4, node(pawnStructEval)           //                N->pawnStructEval + 
   cmpi    cr5,0, rPlayer,white                  //                N->endGameEval;
   add     rTmp1, rTmp1,rTmp2
   add     rTmp1, rTmp1,rTmp3
   add     rTmp1, rTmp1,rTmp4
   beq     cr5, @storeEval                       // if (player == black)
   neg     rTmp1, rTmp1                          //    N-totalEval  = -N->totalEval,
   neg     rTmp9, rTmp9                          //    N->capSelVal = -N->capSelVal;
@storeEval
   cmpi    cr6,0, rTmp9,0
   sth     rTmp1, node(totalEval)
   ble     cr6, @storeCapSel                     // if (N->capSelVal > 0)
   li      rTmp9, 0                              //    N->capSelVal = 0;
@storeCapSel
   sth     rTmp9, node(capSelVal)

@RETURN
   frame_end0
   blr
} /* Evaluate */


/**************************************************************************************************/
/*                                                                                                */
/*                                   PIECE VALUE CHANGE EVALUATION                                */
/*                                           NN->pvSumEval                                        */
/*                                                                                                */
/**************************************************************************************************/

// Piece values form the main part of the evaluation. The "Piece Value Sum" is computed
// incrementally before each move is performed:
//
//    NN->pvSumEval = N->pvSumEval + deltaPV(N->m)

static asm INT EvalMovePV (void)
{
   lhz     rTmp1, move(piece)                    // dPV = PieceVal[m->piece][m->to];
   lhz     rTmp2, move(from)
   lhz     rTmp3, move(to)
   lha     rTmp10, node(pvSumEval)
   addi    rTmp4, rEngine,ENGINE.V.PieceVal
   rlwinm. rTmp5, rTmp1,8, 18,23                 // dPV -= PieceVal[m->piece][m->from];
   add     rTmp5, rTmp4,rTmp5
   add     rTmp2, rTmp2,rTmp2
   add     rTmp3, rTmp3,rTmp3
   lhax    rTmp6, rTmp5,rTmp2
   lhax    rTmp0, rTmp5,rTmp3

   lhz     rTmp8, move(cap)                      // if (m->cap)
   subf    rTmp0, rTmp6,rTmp0                    //    dPV -= PieceVal[m->cap][m->to];
   lhz     rTmp1, move(type)                     
   rlwinm. rTmp8, rTmp8,8, 18,23
   beq+    @Special
   add     rTmp8, rTmp4,rTmp8
   lhax    rTmp9, rTmp8,rTmp3
   subf    rTmp0, rTmp9,rTmp0

@Special                                         // switch (m->type)
   cmpi    cr5,0, rTmp1,mtype_Normal             // {
   andi.   rTmp6, rTmp1,mtype_Promotion
   beq+    cr5, @RETURN
   bne+    @Prom
   cmpi    cr6,0, rTmp1,mtype_O_O_O
   blt+    cr6, @o_o
   beq+    cr6, @o_o_o
@Ep
   neg     rTmp8, rPlayer                        //    case mtype_EP :
   addi    rTmp8, rTmp8,(pawn + black)           //       dPV -= PieceVal[enemy(pawn)][behind(m->to)]; break;
   add     rTmp1, rPawnDir,rPawnDir
   rlwinm. rTmp8, rTmp8,8, 18,23
   subf    rTmp3, rTmp1,rTmp3
   add     rTmp8, rTmp4,rTmp8
   lhax    rTmp9, rTmp8,rTmp3
   subf    rTmp0, rTmp9,rTmp0
   b       @RETURN
@o_o
   addi    rTmp4, rEngine,ENGINE.V.o_oPV         //    case mtype_O_O :
   add     rTmp5, rPlayer,rPlayer                //       dPV = o_oPV[player]; break;
   lhax    rTmp0, rTmp4,rTmp5
   b       @RETURN
@o_o_o
   addi    rTmp4, rEngine,ENGINE.V.o_o_oPV       //    case mtype_O_O_O :
   add     rTmp5, rPlayer,rPlayer                //       dPV = o_o_oPV[player]; break;
   lhax    rTmp0, rTmp4,rTmp5
   b       @RETURN
@Prom
   rlwinm. rTmp6, rTmp6,8, 18,23                 //    default :     // Prom
   add     rTmp6, rTmp6,rTmp3
   lhax    rTmp9, rTmp4,rTmp6                    //       dPV += PieceVal[m->type][m->to];
   add     rTmp0, rTmp9,rTmp0                    // }

@RETURN
   add     rTmp1, rTmp0,rTmp10                   // NN->pvSumEval = N->pvSumVal + dPV;
   sth     rTmp0, node(dPV)
   sth     rTmp1, nnode(pvSumEval)               // return NN->pvSumEval;
   blr
} /* EvalMovePV */


/**************************************************************************************************/
/*                                                                                                */
/*                                    PAWN STRUCTURE EVALUATION                                   */
/*                                        N->pawnStructEval                                       */
/*                                                                                                */
/**************************************************************************************************/

// Pawn structure evaluation (f = game phase [0...9]):

#define isoBack_Closed   2            //  -8 if isolated/backward on closed file.
#define isoBack_Open(f)  (6 - f/2)    // -28...-16 if isolated/backward on semi-open file.
#define iso_Any          3            // -12 extra if isolated.
#define dob_Any          4            // -16 if doubled on any file.
#define dobIso_Any      18            // -72 if doubled and isolated on any file.
#define passVal(f)      (7 + f/2)     // +20...+36 if Passed pawn (isoBack_Open is subtracted during eval).

/*--------------------------------- Root Pawn Structure Evaluation -------------------------------*/
// First we compute the various pawn structure evaluation tables (some of which depend on the
// game phase). The bonus must be high enough to outweigh the penalty
// of isolated pawns. This also automatically gives an extra bonus for connected passed pawns.

// It's also very important to reset the PassSqW[] and PassSqB[] tables at the root node before
// calling the general pawn structure evaluation.

static void EvalPawnStructRoot (ENGINE *E)
{
   EVAL_STATE *e = &(E->E);
   INT         f = E->V.phase;

   for (INT i = 0; i <= 0xFF; i++)
   {
      INT n = E->Global->A.NumBits[i];

      e->IsoBackVal[i]  = n*isoBack_Closed;
      e->IsoBackVal_[i] = n*(isoBack_Open(f) - isoBack_Closed);
      e->IsoVal[i]      = n*iso_Any;
      e->DobVal[i]      = n*dob_Any;
      e->DobIsoVal[i]   = n*(dobIso_Any - dob_Any - iso_Any);

      e->PassedVal[i]   = n*passVal(f);
      if (n > 1 && (i & 0x03) && (i & 0xC0))
         e->PassedVal[i] += f/2 + 2;
   }

   E->S.rootNode->PassSqW[0] = E->S.rootNode->PassSqB[0] = 0;
} /* EvalPawnStructRoot */

/*------------------------------- General Pawn Structure Evaluation ------------------------------*/
// EvalPawnStruct() returns a pawn structure evaluation of doubled, isolated, backward and
// passed pawns. Also the lists N->PassSqW[] and N->PassSqB[] of white and black passed pawns
// are computed if the rule of the square is applicable. These lists are used by the end game
// routines EvalRuleOfSquare() and EvalKPK().
// The evaluation values (punishments) below are multiplied by 4 (<< 2) during evaluation.

#define sum    rLocal1                // Friendly occupied files so far (bit list).
#define back   rLocal2                // Files with friendly backward pawns so far (bit list).
#define dob    rLocal3                // Files with friendly doubled pawns so far (bit list).
#define pass   rLocal4                // Files with enemy passed pawns so far (bit list).
#define iso    rLocal5                // Files with friendly isolated pawns (bit list).
#define sum_   rTmp10                 // Enemy occupied files (bit list). For evaluation only

#define Blk    rTmp9                  // Base pointers to PawnBlk[] and IsoPawns[].
#define Iso    rTmp8
#define Psq    rTmp7                  // List of passed pawns (excl. on 7th), stored in N->PassSqX[]

#define peval  rTmp6                  // Pawn struct evaluation sum
#define val    rTmp5                  // Pawn struct evaluation sum
#define ValTab rTmp4                  // Pawn struct evaluation sum

#define pwn    rTmp5                  // White pawns on current rank (temp)
#define pbn    rTmp4                  // Black pawns on current rank (temp)
#define blk    rTmp3                  // Block bits (temp)

#define pw(n)  (ENGINE.B.PawnStructW + n - 1)(rEngine)   // E->B.PawnStructW[n]
#define pb(n)  (ENGINE.B.PawnStructB + n - 1)(rEngine)   // E->B.PawnStructB[n]


asm void GetPassedPawns (register INT _rank, register INT _pass1);

asm INT EvalPawnStruct (void)
{
   #define evalRankW(LN,L,n,pasRank) \
      lbz     pwn, pw(n)                         ;                                            \
      lbz     pbn, pb(n)                         ;                                            \
      lbzx    blk, Blk,sum                       ;                                            \
      cmpi    cr5,0, pwn,0                       ; /* if (pw[n])   pawns on rank n         */ \
      beq+    cr5, L                             ; /* {                                    */ \
      lbzx    iso, Iso,pwn                       ;                                            \
      and     rTmp1, sum,pwn                     ; /*    dob |= sum & pw[n];               */ \
      or      dob, dob,rTmp1                     ;                                            \
      or      sum, sum,pwn                       ; /*    sum |= pw[n];                     */ \
      and     rTmp1, iso,blk                     ; /*    back = (Iso(pw[n]) & BlkN[sum]) | */ \
      andc    rTmp2, back,pwn                    ; /*            (back & �pw[n]);          */ \
      or      back, rTmp1,rTmp2                  ; /* }                                    */ \
   L  and     rTmp1, pbn,blk                     ; /* ps = (pb[n] & Blk(sum)) & �sum;      */ \
      or      sum_, pbn,sum_                     ; /* sum_ |= pb[n];                       */ \
      andc.   rTmp2, rTmp1,sum                   ; /* if (ps)                              */ \
      beq+    LN                                 ; /* {                                    */ \
      or      pass, pass,rTmp2                   ; /*    pass |= ps;                       */ \
      andi.   rTmp0, rPieceCount,0xFFF0          ; /*    if ("white has no officers")      */ \
      bne+    LN                                 ;                                            \
      li      rTmp1, pasRank                     ; /*       GetPassedPawns(7 - n, ps);     */ \
      bl      GetPassedPawns                       // }

   #define evalRankB(LN,L,n,pasRank) \
      lbz     pbn, pb(n)                         ;                                            \
      lbz     pwn, pw(n)                         ;                                            \
      lbzx    blk, Blk,sum                       ;                                            \
      cmpi    cr5,0, pbn,0                       ; /* if (pb[n])   pawns on rank n         */ \
      beq+    cr5, L                             ; /* {                                    */ \
      lbzx    iso, Iso,pbn                       ;                                            \
      and     rTmp1, sum,pbn                     ; /*    dob |= sum & pb[n];               */ \
      or      dob, dob,rTmp1                     ;                                            \
      or      sum, sum,pbn                       ; /*    sum |= pb[n];                     */ \
      and     rTmp1, iso,blk                     ; /*    back = (Iso(pb[n]) & BlkN[sum]) | */ \
      andc    rTmp2, back,pbn                    ; /*            (back & �pw[n]);          */ \
      or      back, rTmp1,rTmp2                  ; /* }                                    */ \
   L  and     rTmp1, pwn,blk                     ; /* ps = (pw[n] & BlkN(sum)) & �sum;     */ \
      or      sum_, pwn,sum_                     ; /* sum_ |= pw[n];                       */ \
      andc.   rTmp2, rTmp1,sum                   ; /* if (ps)                              */ \
      beq+    LN                                 ; /* {                                    */ \
      or      pass, pass,rTmp2                   ; /*    pass |= ps;                       */ \
      andis.  rTmp0, rPieceCount,0xFFF0          ; /*    if ("black has no officers")      */ \
      bne+    LN                                 ;                                            \
      li      rTmp1, pasRank                     ; /*       GetPassedPawns(7 - n, ps);     */ \
      bl      GetPassedPawns                       // }

   /* - - - - - - - - - - - - - - - - - - - CODE BEGIN - - - - - - - - - - - - - - - - - - - -*/

   frame_begin5

   addi    Blk,  rGlobal, GLOBAL.E.BlkPawnsN
   addi    Iso,  rGlobal, GLOBAL.E.IsoPawns
   li      peval, 0

   /* - - - - - - - - - - - - - - - - - WHITE Pawn Structure - - - - - - - - - - - - - - - - -*/

@WHITE                                          // 2nd RANK
   lbz     sum, pw(2)                           //    sum  = pw[2];
   lbz     pass, pb(2)                          //    pass = pb[2];
   li      dob,  0                              //    dob  = 0;
   li      sum_, 0                              //    sum_ = 0;
   lbzx    back, Iso,sum                        //    back = IsoPawns[sum];
   addi    Psq, rNode, NODE.PassSqB - 1         //    PSq = N->PassSqB;
   andi.   rTmp0, rPieceCount,0xFFF0
   bne+    @W3                                  //    if (pass && "white has no officers")
   cmpi    cr5,0, pass,0
   beq     cr5, @W3                             //       GetPassedPawns(7, pass)
   li      rTmp1, 0x10
   mr      rTmp2, pass
   bl      GetPassedPawns

@W3 evalRankW(@W4, @w3,3,0x20)                  // 3rd RANK
@W4 evalRankW(@W5, @w4,4,0x30)                  // 4th RANK
@W5 evalRankW(@W6, @w5,5,0x40)                  // 5th RANK
@W6 evalRankW(@W7, @w6,6,0x50)                  // 6th RANK
@W7 evalRankW(@W , @w7,7,0x50)                  // 7th RANK

@W li      rTmp0, 0                             // Psq[n] = 0;
   stb     rTmp0, 1(Psq)

@WIso                                           //--- ISOLATED PAWNS ---
   lbzx    iso, Iso,sum                         // iso = IsoPawns[sum];
   addi    ValTab, rEngine,ENGINE.E.IsoVal      // peval -= IsoVal[iso];
   lbzx    val, ValTab,iso
   subf    peval, val,peval
@WDob                                           //--- DOUBLED PAWNS ---
   cmpi    cr5,0, dob,0                         // if (dob)
   beq+    cr5, @WIsoBack                       //    peval -= DobVal[dob];
   addi    ValTab, rEngine,ENGINE.E.DobVal
   lbzx    val, ValTab,dob
   subf    peval, val,peval
@WDobIso                                        //--- DOUBLED & ISOLATED PAWNS ---
   and.    rTmp1, dob,iso                       // if (dob & iso)
   beq+    @WIsoBack                            // {
   addi    ValTab, rEngine,ENGINE.E.DobIsoVal   //    peval -= DobIsoVal[dob & iso];
   lbzx    val, ValTab,rTmp1
   xor     back, back,rTmp1                     //    back ^= (dob & iso); // Don't eval again as backward
   subf    peval, val,peval                     // }
@WIsoBack                                       //--- BACKWARD/ISOLATED PAWNS ---
   cmpi    cr6,0, back,0                        // if (back)
   beq+    cr6, @WPass                          // {
   addi    ValTab, rEngine,ENGINE.E.IsoBackVal  //    peval -= IsoBackVal[back];
   lbzx    val, ValTab,back
   subf    peval, val,peval                     //    Non-passed on non-closed (semi open) files
// or      sum_, sum_,pass                      //    bits = ~(sum | pass) & back
   andc.   rTmp1, back,sum_                     //    if (bits)
   beq+    @WPass                               //       peval -= IsoBackVal_[bits];
   addi    ValTab, rEngine,ENGINE.E.IsoBackVal_
   lbzx    val, ValTab,rTmp1
   subf    peval, val,peval                     // }
@WPass                                          //--- PASSED BLACK PAWNS ---
   cmpi    cr5,0, pass,0                        // if (pass)
   beq+    cr5, @BLACK                          //    peval -= PassedVal[pass];
   addi    ValTab, rEngine,ENGINE.E.PassedVal
   lbzx    val, ValTab,pass
   subf    peval, val,peval

   /* - - - - - - - - - - - - - - - - - BLACK Pawn Structure - - - - - - - - - - - - - - - - -*/

@BLACK                                          // 2nd RANK
   lbz     sum, pb(7)                           //    sum  = pb[7];
   lbz     pass, pw(7)                          //    pass = pw[7];
   li      dob,  0                              //    dob  = 0;
   li      sum_, 0                              //    sum_ = 0;
   lbzx    back, Iso,sum                        //    back = IsoPawns[sum];
   addi    Psq, rNode, NODE.PassSqW - 1         //    PSq = N->PassSqW;
   andis.  rTmp0, rPieceCount,0xFFF0
   bne+    @B3                                  //    if (pass && "black has no officers")
   cmpi    cr5,0, pass,0
   beq     cr5, @B3                             //       GetPassedPawns(7, pass)
   li      rTmp1, 0x60
   mr      rTmp2, pass
   bl      GetPassedPawns

@B3 evalRankB(@B4, @b3,6,0x50)                  // 3rd RANK
@B4 evalRankB(@B5, @b4,5,0x40)                  // 4th RANK
@B5 evalRankB(@B6, @b5,4,0x30)                  // 5th RANK
@B6 evalRankB(@B7, @b6,3,0x20)                  // 6th RANK
@B7 evalRankB(@B , @b7,2,0x20)                  // 7th RANK

@B li      rTmp0, 0                              // Psq[n] = 0;
   stb     rTmp0, 1(Psq)

@BIso                                            //--- ISOLATED PAWNS ---
   lbzx    iso, Iso,sum                          // iso = IsoPawns[sum];
   addi    ValTab, rEngine,ENGINE.E.IsoVal       // peval += IsoVal[iso];
   lbzx    val, ValTab,iso
   add     peval, val,peval
@BDob                                            //--- DOUBLED PAWNS ---
   cmpi    cr5,0, dob,0                          // if (dob)
   beq+    cr5, @BIsoBack                        //    peval += DobVal[dob];
   addi    ValTab, rEngine,ENGINE.E.DobVal
   lbzx    val, ValTab,dob
   add     peval, val,peval
@BDobIso                                         //--- DOUBLED & ISOLATED PAWNS ---
   and.    rTmp1, dob,iso                        // if (dob & iso)
   beq+    @BIsoBack                             // {
   addi    ValTab, rEngine,ENGINE.E.DobIsoVal    //    peval += DobIsoVal[dob & iso];
   lbzx    val, ValTab,rTmp1
   xor     back, back,rTmp1                      //    back ^= (dob & iso); // Don't eval again as backward
   add     peval, val,peval                      // }
@BIsoBack                                        //--- BACKWARD/ISOLATED PAWNS ---
   cmpi    cr6,0, back,0                         // if (back)
   beq+    cr6, @BPass                           // {
   addi    ValTab, rEngine,ENGINE.E.IsoBackVal   //    peval += IsoBackVal[back];
   lbzx    val, ValTab,back
   add     peval, val,peval                      //    Non-passed on non-closed (semi open) files
// or      sum_, sum_,pass                       //    bits = ~(sum_ | pass) & back
   andc.   rTmp1, back,sum_                      //    if (bits)
   beq+    @BPass                                //       peval += IsoBackVal_[bits];
   addi    ValTab, rEngine,ENGINE.E.IsoBackVal_
   lbzx    val, ValTab,rTmp1
   add     peval, val,peval                      // }
@BPass                                           //--- PASSED WHITE PAWNS ---
   cmpi    cr5,0, pass,0                         // if (pass)
   beq+    cr5, @RETURN                          //    peval += PassedVal[pass];
   addi    ValTab, rEngine,ENGINE.E.PassedVal
   lbzx    val, ValTab,pass
   add     peval, val,peval

   /* - - - - - - - - - - - - - - - - - - - Return - - - - - - - - - - - - - - - - - - - - -*/

@RETURN
   rlwinm  rTmp1,peval,2, 0,29                   // N->pawnStructEval = 4*peval;
   sth     rTmp1, node(pawnStructEval)

   frame_end5                                    // return N->pawnStructEval;
   blr
} /* EvalPawnStruct */


// Stores the bit list "_pass1" of passed pawns on current rank "_rank" in "PassSq?[]".

asm void GetPassedPawns (register INT _rank, register INT _pass1)
{
   #define _file  rTmp3

   li      rTmp4, 31
   li      rTmp5, 1
@L cntlzw  _file, _pass1
   subf    _file, _file,rTmp4   // Get next high order bit in _passBits (7..0)
   or      rTmp0, _rank,_file
   stbu    rTmp0, 1(Psq)
   rlwnm   rTmp0, rTmp5,_file,0,31
   andc.   _pass1, _pass1,rTmp0
   bne-    @L

   #undef _file
} /* GetPassedPawns */


#undef sum
#undef back
#undef dob
#undef pass
#undef iso

#undef Blk
#undef Iso
#undef Psq

#undef peval
#undef val
#undef ValTab

#undef pwn
#undef pbn
#undef blk

#undef pw
#undef pb


/**************************************************************************************************/
/*                                                                                                */
/*                                         END GAME EVALUATION                                    */
/*                                           N->endGameEval                                       */
/*                                                                                                */
/**************************************************************************************************/

// EvalEndGame() computes an evaluation in special end games where the normal evaluation (piece
// values, mobility and pawn structure) would be insufficient: If the rule of the square is
// applicable EvalRuleOfSquare() or EvalKPK() is called. Otherwise a material configuration
// modification value N->mtrlEndVal is computed and added to N->E to deal with cases such
// as KNNK, KNKP, KQKR e.t.c. This value is computed incrementally, that is if the previous
// move was not a capture we simply set N->mtrlEndVal = PN->mtrlEndVal. Otherwise we compute
// N->mtrlEndVal from scratch.

// Distinct endgame groups:
//
// (1) BOTH WHITE AND BLACK HAS PAWNS
//     Check opposite coloured bishops
//
// (2) ONLY WHITE HAS PAWNS
//     Exit if too much material (i.e. any Queens or Rooks or too many Bishop/Knight/Pawns)
//     KP:K(N|B)      +175    White cannot lose even though 200 points behind
//     KPP:K(N|B)     +125    White cannot lose (and may even win!) even though 100 points behind
//     KP(N|B):K(N|B) -100    The extra pawn isn't worth much. Black just needs to sacrifice his B/N
//     KP*(N|B):KNN   +200    The extra Black knight isn't worth much
//     KP:KNN         +450    Usually a draw (because K:KNN drawn), except in rare zugswang cases.
//     KPP:KNN        +400    Usually a draw (because K:KNN drawn), except in rare zugswang cases.
//     KPPP:KNN       +350    Probably slightly advantagous to white!!
//
// (3) ONLY BLACK HAS PAWNS
//     Exit if too much material (i.e. any Queens or Rooks or too many Bishop/Knight/Pawns)
//     K(N|B):KP      -175    Black cannot lose even though 200 points behind
//     K(N|B):KPP     -125    Black cannot lose (and may even win!) even though 100 points behind
//     K(N|B):KP(N|B) +100    The extra pawn isn't worth much. White just needs to sacrifice his B/N
//     KNN:KP         -450    Usually a draw (because KNN:K drawn), except in rare zugswang cases.
//     KNN:KPP        -400    Usually a draw (because KNN:K drawn), except in rare zugswang cases.
//     KNN:KPPP       -350    Probably slightly advantagous to black!!
//
// (4) NO PAWNS AT ALL
//     (a) BOTH WHITE AND BLACK HAS OFFICERS
//         Reduce advantage of leading side (drawish)
//     (b) ONLY WHITE HAS OFFICERS
//         if KNNK it's a draw
//         otherwise it's a forced win (the cases KNK and KBK are handled by the draw check).
//     (c) ONLY WHITE HAS OFFICERS
//         if KKNN it's a draw
//         otherwise it's a forced win (the cases KKN and KKB are handled by the draw check).

static asm INT EvalRuleOfSquare (void);
static asm INT EvalKPK (COLOUR pawnColor);
static asm INT OppColBishops (void);

asm INT EvalEndGame (void)
{
   lbz     rTmp4, node(PassSqW)
   lbz     rTmp5, node(PassSqB)

   lwz     rTmp6, pmove(cap)       
   li      rTmp1, 0                               // N->endGameVal = 0;  ("white" param to EvalKPK)
   addis   rTmp2, r0,0xEC00
   ori     rTmp2, rTmp2,0xEC00     

   or.     rTmp0, rTmp4,rTmp5                     // if (N->PassSqW[0] || N->PassSqB[0])
   beq+    @EVALMTRL                              // {
   addis   rTmp3, r0,0x0001                       //    if (pieceCount == 0x00000001)
   cmpi    cr5,0, rPieceCount,0x0001              //       return EvalKPK(white);
   cmp     cr6,0, rPieceCount,rTmp3
   beq-    cr5, EvalKPK                           //    else if (pieceCount == 0x00010000)
   bne+    cr6, EvalRuleOfSquare                  //       return EvalKPK(black);
   li      rTmp1,black                            //    else
   b       EvalKPK                                //       return EvalRuleOfSquare();
                                                  // }
@EVALMTRL
   and.    rTmp2, rTmp2,rPieceCount               // if ("Too much material")
   bnelr+                                         //    return 0;
   andis.  rTmp0, rTmp6,0x000F                    // if (! PN->m.cap && ! isPromotion(PN->m) && 
   bne     @CHECK_PAWNS                           //     PN->type != mtype_EP)
   andi.   rTmp2, rTmp6,0x008F                    // {
   bne     @CHECK_PAWNS
   lha     rTmp1, pnode(endGameEval)              //    return PN->endGameEval;
   blr                                            // }

/*- - - - - - - - - - - - - - - - - - - Check Pawns - - - - - - - - - - - - - - - - - - - - -*/
// First check if either side has any pawns

#define pWhite  rTmp8
#define pBlack  rTmp9

@CHECK_PAWNS
   andi.   pWhite, rPieceCount,0xFFFF
   srawi   pBlack, rPieceCount,16
   andi.   pBlack, pBlack,0xFFFF

   andi.   rTmp0, pWhite,0x000F                   // if (White has pawns AND Black has pawns)
   beq-    @BLACK_P                               // {
   andi.   rTmp0, pBlack,0x000F
   beq-    @WHITE_P
   addis   rTmp2, r0,0xFFF0                       //    if ("KBP*KBP*")
   addis   rTmp3, r0,0x0100                       //       return OppColBishops();
   ori     rTmp2, rTmp2,0xFFF0
   ori     rTmp3, rTmp3,0x0100                    //    else
   and     rTmp2, rPieceCount,rTmp2               //       return 0;
   cmp     cr5,0, rTmp2,rTmp3
   bnelr+  cr5
   b       OppColBishops                          // }

@WHITE_P                                          // else if (Only white has pawns)
                                                  // {
   andi.   rTmp0, pWhite,0xFE0C                   //    if (Any w Q/R or > 1 w B/N or > 3 w P)
   bnelr+                                         //       return 0;
   andi.   rTmp0, pBlack,0xFC00                   //    else if (Any b Q/R or > 3 b B/N)
   bnelr+                                         //       return 0;
   cmpi    cr5,0, pBlack,0x0220                   //    else if (Black only has 2 N)
   bne+    cr5, @5                                //    {
   andi.   rTmp3, pWhite,0xFFF0                   //       if (KP*(B|N)KNN) return 200;
   li      rTmp1, 200
   bnelr+
   cmpi    cr5,0, pWhite,0x0001                   //       if (KPKNN) return 450;
   li      rTmp1, 450
   beqlr   cr5
   cmpi    cr6,0, pWhite,0x0002                   //       if (KPPKNN) return 400;
   li      rTmp1, 400
   beqlr   cr6
   li      rTmp1, 350                             //       if (KPPPKNN) return 350;
   blr                                            //    }
@5 andi.   rTmp2, pBlack,0x0F00                   //    else if (Black does NOT have exactly 1 B/N)
   cmpi    cr0,0, rTmp2,0x0100                    //       return 0;
   bnelr+
   andi.   rTmp2, pWhite,0x0100                   //    else if (White has 1 B/N)
   beq     @6
   andi.   rTmp3, pWhite,0x0002                   //       return (White only has 1 P ? -100 : 0);
   bnelr+
   li      rTmp1, -100
   blr
@6 andi.   rTmp3, pWhite,0x0002                   //    else if (KPKN or KPKB) return 175;
   li      rTmp1, 175
   beqlr+
   andi.   rTmp3, pWhite,0x0001                   //    else if (KPPKN or KPPKB) return 125;
   li      rTmp1, 125
   beqlr+
   li      rTmp1, 0                               //    return 0;
   blr                                            // }

@BLACK_P
   andi.   rTmp2, pBlack,0x000F                   // else if (Only black has pawns)
   beq-    @NO_PAWNS                              // {
   andi.   rTmp0, pBlack,0xFE0C                   //    if (Any b Q/R or > 1 b B/N or > 3 b P)
   bnelr+                                         //       return 0;
   andi.   rTmp0, pWhite,0xFC00                   //    else if (Any w Q/R or > 3 w B/N)
   bnelr+                                         //       return 0;
   cmpi    cr5,0, pWhite,0x0220                   //    else if (White only has 2 N)
   bne+    cr5, @7                                //    {
   andi.   rTmp3, pBlack,0xFFF0                   //       if (KNNKP*(B|N)) return -200;
   li      rTmp1, -200
   bnelr+
   cmpi    cr5,0, pBlack,0x0001                   //       if (KNNKP) return -450;
   li      rTmp1, -450
   beqlr   cr5
   cmpi    cr6,0, pBlack,0x0002                   //       if (KNNKPP) return -400;
   li      rTmp1, -400
   beqlr   cr6
   li      rTmp1, -350                            //       if (KNNKPPP) return -350;
   blr                                            //    }
@7 andi.   rTmp2, pWhite,0x0F00                   //    else if (White does NOT have exactly 1 B/N)
   cmpi    cr0,0, rTmp2,0x0100                    //       return 0;
   bnelr+
   andi.   rTmp2, pBlack,0x0100                   //    else if (Black has 1 B/N)
   beq     @8
   andi.   rTmp3, pBlack,0x0002                   //       return (Black only has 1 P ? +100 : 0);
   bnelr+
   li      rTmp1, 100
   blr
@8 andi.   rTmp3, pBlack,0x0002                   //    else if (KNKP or KBKP) return -175;
   li      rTmp1, -175
   beqlr+
   andi.   rTmp3, pBlack,0x0001                   //    else if (KNKPP or KBKPP) return -125;
   li      rTmp1, -125
   beqlr+
   li      rTmp1, 0                               //    return 0;
   blr                                            // }

/*- - - - - - - - - - - - - - - - - - - Check Officers - - - - - - - - - - - - - - - - - - - -*/
// If there are no pawns we then check the officers:

@NO_PAWNS
   andi.   rTmp2, pWhite,0xFFF0                   // else if (White has officers AND Black has officers")
   lha     rTmp1, node(pvSumEval)
   beq-    @BLACK_OFFI                            // {
   andi.   rTmp3, pBlack,0xFFF0                   //    INT val = -N->pvSumEval/2;
   beq-    @WHITE_OFFI
   srawi   rTmp1, rTmp1,1                         //    if (val < -75) val = -75;
   neg     rTmp1, rTmp1    
   cmpi    cr5,0, rTmp1,-75                       //    else if (val > 75) val = 75;
   cmpi    cr6,0, rTmp1,75
   blt     cr5, @1
   blelr   cr6                                    //    return val;
   li      rTmp1, 75
   blr
@1 li      rTmp1, -75
   blr                                            // }

@WHITE_OFFI                                       // else if (Only White has officers)
   cmpi    cr5,0, pWhite,0x0220                   // {
   bne+    cr5, @2                                //    if ("KNNK")
   li      rTmp2, 30                              //       return 30 - N->pvSumEval;
   subf    rTmp1, rTmp1,rTmp2
   blr                                            //    else
@2 li      rTmp1, 200                             //       return 200;
   blr                                            // }

@BLACK_OFFI                                       // else if (Only Black has officers)
   cmpi    cr5,0, pBlack,0x0220                   // {
   bne+    cr5, @3                                //    if ("KNNK")
   li      rTmp2, -30                             //       return -30 - N->pvSumEval;
   subf    rTmp1, rTmp1,rTmp2
   blr                                            //    else
@3 li      rTmp1, -200                            //       return -200;
   blr                                            // }

#undef pWhite
#undef pBlack

} /* EvalEndGame */

/*-------------------------------------- The Rule of the Square ----------------------------------*/
// Evaluates the rule of the square by scanning N->PassSqW[] and N->PassSqB[] and returns an
// endgame modification value if necessary.

static asm INT EvalRuleOfSquare (void)                     
{
   #define RTab     rTmp10
   #define sqMax    rTmp9
   #define sqMin    rTmp8
   #define Psq      rTmp7
   #define sq       rTmp6
   #define wksq     rTmp5
   #define bksq     rTmp4
   #define diff     rTmp3

   lhz    wksq, (ENGINE.B.PieceLoc)(rEngine)
   lhz    bksq, (ENGINE.B.PieceLoc + 32)(rEngine)
   addi   RTab, rGlobal,GLOBAL.E.RuleOfSquareTab

@WHITE                                            // --- Check WHITE Passed Pawns ---
   li     sqMax, (h1 - 0x10)                      // sqMax  = h1 - 0x10;
   addi   Psq, rNode,(NODE.PassSqW - 1)           // PassSq = N->PassSqW;
   b      @ROFW
@FORW                                             // while (sq = *(PassSq++))
   subf   rTmp1, sq,wksq                          // {
   addi   rTmp1, rTmp1,0x10                       //    "Check king block/support of prom. sq."
   lbzx   diff, RTab,rTmp1                        //    diff = RuleOfSquareTab[front(kingSqW - sq)];
   extsb. diff, diff                              //    if (diff < 0)
   bge    @blackW                                 //       sq = behind(sq), diff++;
   addi   sq, sq,-0x10
   addi   diff, diff,1
@blackW
   cmpi   cr5,0, rPlayer,black                    //    if (player == black)
   bne    cr5, @checkW                            //       sq = behind(sq);
   addi   sq, sq,-0x10
@checkW
   cmp    cr6,0, sq,sqMax                         //    if (sq > sqMax &&
   ble    cr6, @ROFW
   cmpi   cr5,0, diff,0                           //        (d == 0 && kingSqW >= a7
   bne    cr5, @oleW
   cmpi   cr7,0, wksq,0x60                        //                ||
   bge    cr7, @yesW
@oleW
   subf   rTmp1, bksq,sq                          //         sq >= RuleOfSquareTab[sq - kingSqB]))
   lbzx   rTmp2, RTab,rTmp1
   extsb  rTmp2, rTmp2
   cmp    cr5,0, sq,rTmp2
   blt    cr5, @ROFW
@yesW
   ori    sqMax, sq,0x07                          //        sqMax = sq | 0x07;
@ROFW
   lbzu   sq, 1(Psq)
   cmpi   cr5,0, sq,0
   bne+   cr5, @FORW                              // }

@BLACK                                            // --- Check BLACK Passed Pawns ---
   li     sqMin, (a8 + 0x10)                      // sqMin  = a8 + 0x10;
   addi   Psq, rNode,(NODE.PassSqB - 1)           // PassSq = N->PassSqB;
   b      @ROFB
@FORB                                             // while (sq = *(PassSq++))
   subf   rTmp1, bksq,sq                          // {
   addi   rTmp1, rTmp1,0x10                       //    "Check king block/support of prom. sq."
   lbzx   diff, RTab,rTmp1                        //    diff = RuleOfSquareTab[front(sq - kingSqB)];
   extsb. diff, diff                              //    if (diff < 0)
   bge    @whiteB                                 //       sq = front(sq), diff++;
   addi   sq, sq,0x10
   addi   diff, diff,1
@whiteB
   cmpi   cr5,0, rPlayer,white                    //    if (player == white)
   bne    cr5, @checkB                            //       sq = front(sq);
   addi   sq, sq,0x10
@checkB
   cmp    cr6,0, sq,sqMin                         //    if (sq < sqMin &&
   bge    cr6, @ROFB
   cmpi   cr5,0, diff,0                           //        (d == 0 && kingSqB <= h2
   bne    cr5, @oleB
   cmpi   cr7,0, bksq,h2                          //                ||
   ble    cr7, @yesB
@oleB
   subf   rTmp1, sq,wksq                          //         sq >= RuleOfSquareTab[kingSqW - sq]))
   lbzx   rTmp1, RTab,rTmp1
   andi.  rTmp2, sq,0x70
   neg    rTmp2, rTmp2
   addi   rTmp2, rTmp2,0x70
   extsb  rTmp1, rTmp1
   cmp    cr5,0, rTmp2,rTmp1
   blt    cr5, @ROFB
@yesB
   andi.  sqMin, sq,0x70                          //        sqMin = sq & 0x70;
@ROFB
   lbzu   sq, 1(Psq)
   cmpi   cr5,0, sq,0
   bne+   cr5, @FORB                              // }

@EVAL                                             // --- Evaluate/Compare WHITE & BLACK
   andi.  sqMax, sqMax,0xFFF0                     // rankW = rank(sqMax);
   extsh  sqMax, sqMax
   addi   sqMin, sqMin,-0x0070                    // rankB = 7 - rank(sqMin);
   add.   rTmp1, sqMax,sqMin
   beqlr-                                         // if (rankW > rankB)
   blt    @B                                      // {
@W cmpi   cr5,0, rTmp1,0x10                       //    if (rankW > rankB + 1 || player == black)
   bgt    cr5, @winW                              //       return 500 + 16*rankW;
   cmpi   cr6,0, rPlayer,black                    //    else
   bne    cr6, @RETURN0                           //       return 0;
@winW
   addi   rTmp1, sqMax,500                        // }
   blr                                            // else if (rankB > rankW)
@B cmpi   cr5,0, rTmp1,-0x10                      // {  if (rankB > rankW + 1 || player == white)
   blt    cr5, @winB                              //       return -500 - 16*rankB;
   cmpi   cr6,0, rPlayer,white                    //    else
   bne    cr6, @RETURN0                           //       return 0;
@winB
   addi   rTmp1, sqMin,-500
   blr                                            // }

@RETURN0                                          // else
   li     rTmp1,0                                 // {� return 0;
   blr                                            // }

   #undef RTab
   #undef sqMax
   #undef sqMin
   #undef Psq
   #undef sq
   #undef wksq
   #undef bksq
   #undef diff
} /* EvalRuleOfSquare */

/*----------------------------------------- KPK Evaluation ---------------------------------------*/
// The KPK data base (size: 192 Kbit = 24 Kb). Is indexed by an 18 bit position descriptor (white
// pawn on the a-d files):
//
//    � Bit 17..15 : Rank ([1..6]) of white pawn.
//    � Bit 14..13 : File ([a..d] = [0..3]) of white pawn.
//    � Bit 12..10 : Rank of white king.
//    � Bit 09..07 : File of white king.
//    � Bit 06..04 : Rank of black king.
//    � Bit 12..10 : Rank of white king.
//    � Bit 03..01 : File of black king.
//    � Bit 00     : Side to move (0 = white; 1 = black).
//
// Bits 17..3 is used to index a byte in "kpkData", the remaining 3 bits is a bit index in the
// byte.
// Evaluates the KPK end game. On entry: rTmp1 indicates the colour of the pawn. The
// evaluation is stored returned in rTmp1.

static asm INT EvalKPK (register COLOUR _pawnColor)
{
   #define psq   rTmp10
   #define ksq   rTmp9
   #define ksq_  rTmp8
   #define ploc  rTmp7
   #define val   rTmp6
   #define n     rTmp7

   cmpi    cr6,0, _pawnColor,white              // <--- for later use : Reserves cr6      
   cmp     cr7,0, _pawnColor,rPlayer            // <--- for later use : Reserves cr7

   addi    ploc, rEngine,ENGINE.B.PieceLoc      // if (pawnColour == white)
   bne     cr6, @BLACK                          // {
@WHITE
   lbz     psq, node(PassSqW)                   //    psq = N->PassSqW[0];
   add     rTmp1, psq,psq
   lhzx    rTmp2, rBoard,rTmp1                  //    if (Board[psq] != wPawn)   "double move"
   cmpi    cr5,0, rTmp2,wPawn                   //       psq -= 0x10;
   beq+    cr5, @w
   addi    psq, psq,-0x10                       //    ksq  = PieceLoc[white];
@w lhz     ksq, 0(ploc)                         //    ksq_ = PieceLoc[black];
   lhz     ksq_, 32(ploc)
   b       @normFile                            // }
@BLACK                                          // else
   lbz     psq, node(PassSqB)                   // {  psq = N->PassSqB[0];
   add     rTmp1, psq,psq
   lhzx    rTmp2, rBoard,rTmp1                  //    if (Board[psq] != bPawn)   "double move"
   cmpi    cr5,0, rTmp2,bPawn                   //       psq += 0x10;
   beq+    cr5, @b
   addi    psq, psq,0x10
@b lhz     ksq, 32(ploc)                        //    ksq  = PieceLoc[white] ^ 0x70;
   lhz     ksq_, 0(ploc)                        //    ksq_ = PieceLoc[black] ^ 0x70;
   xori    psq, psq,0x70                        //    psq ^= 0x70; 
   xori    ksq, ksq,0x70
   xori    ksq_, ksq_,0x70                      // }   

@normFile
   andi.   rTmp1, psq,0x07                      // if (file(psq) > 3)
   cmpi    cr5,0, rTmp1,3                       // {
   ble     cr5, @EVAL
   xori    psq, psq,0x07                        //    psq  ^= 0x07; 
   xori    ksq, ksq,0x07                        //    ksq  ^= 0x07; 
   xori    ksq_, ksq_,0x07                      //    ksq_ ^= 0x07; 
                                                // }   
@EVAL
   addi    rTmp1, psq,0x10                      // val = 4*rank(psq) - Closeness[psq + 0x10 - ksq_]/2;
   subf    rTmp1, ksq_,rTmp1
   addi    rTmp2, rGlobal,GLOBAL.V.Closeness
   add     rTmp1, rTmp1,rTmp1
   lhax    rTmp3, rTmp2,rTmp1
   andi.   rTmp4, psq,0x70
   srawi   rTmp4, rTmp4,2
   srawi   rTmp3, rTmp3,1
   subf    val, rTmp3,rTmp4
   
@PACKn
   andi.   n, psq,0x07                          // n = ((rank(psq) - 1) << 2) + file(psq);
   addi    rTmp1, rTmp4,-4
   or      n, rTmp1,n
   rlwinm  n, n,6,0,31                          // n <<= 6;
   andi.   rTmp1, ksq,0x70                      // n |= (rank(ksq) << 3) + file(ksq);
   andi.   rTmp2, ksq,0x07
   srawi   rTmp1, rTmp1,1
   or      n, n,rTmp2
   or      n, n,rTmp1
   rlwinm  n, n,6,0,31                          // n <<= 6;
   andi.   rTmp1, ksq_,0x70                     // n |= (rank(ksq_) << 3) + file(ksq_);
   andi.   rTmp2, ksq_,0x07
   srawi   rTmp1, rTmp1,1
   or      n, n,rTmp2
   or      n, n,rTmp1
   add     n, n,n                               // n <<= 1;
   beq     cr7, @LOOKUP                         // if (player != pawnColour)
   addi    n, n,1                               //    n |= 1;

@LOOKUP
   lha     rTmp8, node(pvSumEval)     
   lha     rTmp9, node(pawnStructEval)
   addi    rTmp1, rGlobal,GLOBAL.E.kpkData      // if (kpkData[n >> 3] & (1 << (n & 7)))
   srawi   rTmp2, n,3
   lbzx    rTmp3, rTmp1,rTmp2                   //    val += 600;
   andi.   rTmp4, n,0x07
   li      rTmp5, 1
   rlwnm   rTmp5, rTmp5,rTmp4,0,31
   and.    rTmp0, rTmp3,rTmp5
   beq     @chkNeg
   addi    val, val,600
@chkNeg                           
   beq     cr6, @RETURN                         // if (pawnColour == black)
   neg     val, val                             //    val = -val;
@RETURN
   subf    val, rTmp8,val                       // return val - N->pvSumEval - N->pawnStructEval;
   subf    rTmp1, rTmp9,val
   blr

   #undef psq
   #undef ksq
   #undef ksq_
   #undef ploc
   #undef val
   #undef n
} /* EvalKPK */

/*------------------------------------ Opposite Coloured Bishops ---------------------------------*/
// Punishes the leading side if we have an opposite coloured bishop ending.

static asm INT OppColBishops (void)
{
   mr     rTmp1, rPieceLoc                     // for (i = 1; wsq = PieceLocW[i] < a1; i++);
@w lhau   rTmp2, 2(rTmp1)
   cmpi   cr5,0, rTmp2,0
   blt-   cr5, @w
   mr     rTmp3, rPieceLoc_                    // for (i = 1; bsq = PieceLocW[i] < a1; i++);
@b lhau   rTmp4, 2(rTmp3)
   cmpi   cr6,0, rTmp4,0
   blt-   cr6, @b

   xor    rTmp1, rTmp2,rTmp4                   // if ((wsq^bsq^rank(wsq)^rank(bsq)) & 0x01 == 0)
   srawi  rTmp3, rTmp1,4
   xor    rTmp1, rTmp1,rTmp3                   //    return 0;
   andi.  rTmp1, rTmp1,0x01
   beqlr+                                      // else
   lha    rTmp1, node(pvSumEval)
   neg    rTmp1, rTmp1                         //    return -N->pvSumEval/2;
   srawi  rTmp1, rTmp1,1
   blr
} /* OppColBishops */


/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION                                   */
/*                                                                                                */
/**************************************************************************************************/

void InitEvaluateModule (GLOBAL *Global, PTR kpkData)
{
   EVAL_COMMON *E = &(Global->E);

   //--- Initialize bit tables ---

   for (INT i = 0; i <= 255; i++)
   {
      E->BlkPawnsN[i] = ~((i << 1) | (i >> 1));      // Negated!
      E->IsoPawns[i]  = i & ~((i << 1) | (i >> 1));
   }

   //--- Initialize "Rule of Square" table ---

   BYTE *R = E->RuleOfSquareTab;
   R[0] = 0x70;

   for (INT f = 1; f <= 7; f++)
   {
      for (INT r = 1; r <= 7; r++)
      {  R[f + (r << 4)] = 1;
         R[f - (r << 4)] = (r >= f ? 0x70 : (8 - f) << 4);
         R[f + 0]        = R[f - 0x10];
      }

      SQUARE sq = f << 4;
      R[-sq] = 0x70;

      if (sq > 0x30)
         R[sq] = -2;
      else
         R[sq] = -1,
         R[left(sq)] = R[right(sq)] = 0;

      for (INT r = -0x70; r <= 0x70; r += 0x10)
         R[r - f] = R[r + f];
   }

   //--- Initialize "KPK" database ---
   
   if (kpkData)
   {
      for (LONG i = 0; i < kpkDataSize; i++)
         E->kpkData[i] = kpkData[i];
   }
   else
   {
      for (LONG i = 0; i < kpkDataSize; i++)
         E->kpkData[i] = 0;
   }
} /* InitEvaluateModule */
