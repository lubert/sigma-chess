/**************************************************************************************************/
/*                                                                                                */
/* Module  : MOBILITY.C */
/* Purpose : This module provides routines for mobility evaluation, both from
 * scratch at the root */
/*           node and incrementally during the search. Additionally, the
 * mobility evalution       */
/*           computes a "turbulence" evaluation for each move, which is used
 * during selection.    */
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

#ifdef kartoffel

#include "Engine.h"

#include "Engine.f"
#include "Mobility.f"

// The mobility evaluation supports x-ray evaluation (with pawns and kings
// blocking the x-ray). Additionally weighted mobility is used (maybe not in
// first version).

// The SEARCH_STATE structure includes a mobility evaluation table MobVal[],
// which for each occupied square holds the mobility evaluation for the piece on
// that square. This is used when evaluating moves by providing instant mobility
// values of the moving piece at the source square as well as any captured
// piece.
//
// For each node N, the following mobility related values are stored in order to
// facilitate incremental evaluation:
//
// INT mobEval     : The total (incrementally computed) mobility evaluation for
// the current node. INT moveMob     : Mobility evaluation for current move
// (seen from WHITE) INT destMob     : Mobility evaluation to be stored in
// MobVal[m.to] when performing move. INT oldDestMob  : Previous value of
// MobVal[m.to] to be restored when retracting move.

/*--------------------------------------- Constants/Macros
 * ---------------------------------------*/
// Square independant mobility values:

#define mobVal_Pawn 5
#define mobVal_Knight 2
#define mobVal_Bishop 3
#define mobVal_Rook 2
#define mobVal_Queen 1
#define mobVal_King 1

// Positional attack/pin values for attacking/pinning undefended or higher
// valued pieces:

#define pinVal_Pawn 5
#define pinVal_Knight 7
#define pinVal_Bishop 7
#define pinVal_Rook 9
#define pinVal_Queen 13
#define pinVal_King 15

// Global register usage:

#define rMoveMob \
  rLocal7  // Used "globally" in all the routines below (seen from WHITE)
#define rMoveThreat \
  rLocal6  // Used "globally" in all the routines below (seen from PLAYER)

/*---------------------------------------- Local Routines
 * ----------------------------------------*/
// IMPORTANT: All these routines operate on the "global" rMoveMob and
// rMoveThreat registers (rLocal6 and rLocal7).

asm void ResetMobility(void);

asm void CalcPieceMob(register PIECE p,
                      register SQUARE from);  // Always computes threats

asm void CalcKingMob(register COLOUR colour, register SQUARE from);
asm void CalcQueenMob(register COLOUR colour, register SQUARE from);
asm void CalcRookMob(register COLOUR colour, register SQUARE from);
asm void CalcBishopMob(register COLOUR colour, register SQUARE from);
asm void CalcKnightMob(register COLOUR colour, register SQUARE from);
asm void CalcPawnMob(register COLOUR colour, register SQUARE from);

asm void CalcQueenMobDir(register COLOUR colour, register SQUARE from,
                         register INT dirInx, register INT dir,
                         BOOL calcThreats);
asm void CalcRookMobDir(register COLOUR colour, register SQUARE from,
                        register INT dirInx, register INT dir,
                        BOOL calcThreats);
asm void CalcBishopMobDir(register COLOUR colour, register SQUARE from,
                          register INT dirInx, register INT dir,
                          BOOL calcThreats);
asm void CalcKnightMobDir(register COLOUR colour, register SQUARE from,
                          register INT dir, BOOL calcThreats);

asm void CalcBlockMob(register PIECE oldp0, register PIECE newp0,
                      register SQUARE sq0);

/**************************************************************************************************/
/*                                                                                                */
/*                                   CALC MOBILITY FROM SCRATCH */
/*                                                                                                */
/**************************************************************************************************/

// This routine is called at the root of the search tree and calculates the
// "E->V.MobVal[]" table and the mobility sum "E->V.sumMob" from scratch (based
// on the current board E->B.Board[]". No threat evaluation is computed at this
// point.

void CalcMobilityState(ENGINE *E) {
  Asm_Begin(E);
  ResetMobility();  // Reset E->V.MobVal[] and E->V.sumMob.
  Asm_End();

  E->S.rootNode->mobEval = E->V.sumMob;
} /* CalcMobilityState */

asm void ResetMobility(void)

// Builds the E->V.MobVal[] table from scratch (based on the current board
// configuration) and stores the total mobility sum (seen from WHITE) in
// E->V.sumMob.

{
  // #define rMoveMob     rLocal7
  // #define rMoveThreat  rLocal6
#define sum rLocal3
#define rMobVal rLocal2
#define sq rLocal1

  frame_begin7  // We must save rLocal6 and rLocal7

      addi rMobVal,
      rEngine, ENGINE.V.MobVal li sum,
      0  // INT sum = 0;
      li sq,
      a1

      @FOR  // for (sq = a1; sq <= h8; sq++)
          andi.rTmp0,
      sq,
      0x88  //    if (onBoard(sq))
          bne -
          @ROF  //    {
              add rTmp2,
      sq,
      sq  //       PIECE p = E->B.Board[sq];
          lhzx rTmp1,
      rBoard, rTmp2 cmpi cr5, 0, rTmp1,
      empty  //       if (p == empty)
          bne cr5,
      @Occupied  //          E->V.MobVal[sq] = 0;
          sthx rTmp1,
      rMobVal,
      rTmp2 b @ROF @Occupied  //       else
          mr rTmp2,
      sq  //       {
          li rMoveMob,
      0  //          E->V.MobVal[sq] = CalcPieceMob(p, sq);
      li rMoveThreat,
      0 bl CalcPieceMob add rTmp2, sq, sq sthx rMoveMob, rMobVal, rTmp2 add sum,
      sum,
      rMoveMob  //          sum += E->V.MobVal[sq];
      @ROF      //       }
          addi sq,
      sq, 1 cmpi cr5, 0, sq, h8 ble + cr5,
      @FOR  //     }

          sth sum,
      ENGINE.V.sumMob(rEngine)  // E->V.sumMob = sum;

      frame_end7 blr

#undef sumMob
#undef rMobVal
#undef sq
} /* ResetMobility */

/**************************************************************************************************/
/*                                                                                                */
/*                                         EVAL MOVE MOBILITY */
/*                                                                                                */
/**************************************************************************************************/

// Mobility and threat/turbulence calculation for each move is done in several
// stages:
//
// 1. Before the actual move is performed, "EvalMoveMob()" is called (by
// "EvalMove()"). This computes
//    the mobility change and the turbulence evaluation for the move. Together
//    with the PV Sum, we subsequently perform selection/forward pruning.
// 2. When actually performing a move, we must update the MobVal[] table
// accordingly. This is handled
//    by the "PerformMoveMob()" routine.
// 3. When retracting a move we must "un"-update the MobVal[] table. This is
// handled by the
//    "RetractMoveMob()" routine.

/*-------------------------------------- Evaluate Move Mobility
 * ----------------------------------*/
// This routine evaluates mobility and threats for the current move N->m and
// stores the result in the N->moveMob, N->moveThreat, N->destMob and
// N->oldDestMob fields. Additionally the total mobility evaluation NN->mobEval
// for the next node is computed (just like "EvalMovePV()" computes the total
// piece value evaluation NN->pvSumEval for the next node). The total tentative
// evaluation (i.e no pawnstruct or endgame stuff included) of the next position
// is then used as a basis for the selection scheme.

// NOTE: When performing/retracting mobility, we don't have to set/restore
// MobVal[m.from], as entries are only changed when placing a piece on a square,
// NOT when moving a piece away from a square.

asm INT EvalMoveMob(void)  // Returns NN->mobEval
{
  // #define rMoveMob     rLocal7
  // #define rMoveThreat  rLocal6
#define mfrom rLocal5
#define mto rLocal4
#define mpiece rLocal3
#define mcap rLocal2
#define mtype rLocal1

  frame_begin7

      lhz mfrom,
      move(from) lhz mpiece, move(piece) lhz mto, move(to) lhz mcap,
      move(cap) li rMoveThreat,
      0  // N->moveThreat = 0;

      addi rTmp1,
      rNode,
      NODE.DMobList - 2  // N->dmobTop = &N->DMobList[-1];
          stw rTmp1,
      node(dmobTop)

          li rMoveMob,
      0  //###
      lha rTmp1,
      node(mobEval)  //###
      b @Return      //###

      //--- First remove mobility of moving piece and unblock ---

      @SourceMob addi rTmp8,
      rEngine,
      ENGINE.V.MobVal  // N->moveMob = -E->V.MobVal[N->m.from];
          add rTmp5,
      mfrom, mfrom lhz mtype, move(type) lhax rMoveMob, rTmp8,
      rTmp5

          mr rTmp1,
      mpiece  // CalcBlockMob(N->m.piece, empty, N->m.from);
          li rTmp2,
      empty  // Also sets E->B.Board[N->m.from] = empty
          mr rTmp3,
      mfrom neg rMoveMob,
      rMoveMob bl CalcBlockMob

          //--- Remove mobility of any captured pieces ---

          cmpi cr5,
      0, mcap,
      0  // if (N->m.cap)
      addi rTmp8,
      rEngine, ENGINE.V.MobVal beq cr5,
      @CheckEP  // {
          add rTmp2,
      mto,
      mto  //    N->oldDestMob = E->V.MobVal[N->m.to];
          lhax rTmp1,
      rTmp8, rTmp2 subf rMoveMob, rTmp1,
      rMoveMob  //    moveMob -= N->oldDestMob;
          sth rTmp1,
      node(oldDestMob) b @CheckPromotion  // }
      @CheckEP                            // else
          andi.rTmp1,
      mtype,
      mtype_EP  // {
          sth mcap,
      node(oldDestMob)  //    N->oldDestMob = 0;
          beq +
          @CheckPromotion  //    if (N->m.type != mtype_EP)
              subf rTmp3,
      rPawnDir,
      mto  //    {
          add rTmp2,
      rTmp3,
      rTmp3  //       moveMob -= E->V.MobVal[N->m.to - rPawnDir];
          lhax rTmp1,
      rTmp8, rTmp2 subf rMoveMob, rTmp1, rMoveMob neg rTmp4,
      rPlayer  //       CalcBlockMob(enemy(pawn), empty, N->m.to - pawnDir);
          addi rTmp1,
      rTmp4, black + pawn li rTmp2,
      empty bl CalcBlockMob  //    }
          b @DestMob         // }

      //--- Check promotion ---
      // For promotions we temporarily set mpiece to N->m.type:

      @CheckPromotion andi.rTmp1,
      mtype,
      mtype_Promotion  // if (isPromotion(m)) m.piece = m.type;
              beq +
          @DestMob mr mpiece,
      mtype

      //--- Add dest mobility and block ---

      @DestMob sth rMoveMob,
      node(moveMob)  // moveMob0 = N->moveMob;
      mr rTmp1,
      mpiece  // CalcPieceMob(N->m.piece, N->m.to);
          mr rTmp2,
      mto bl CalcPieceMob lha rTmp4,
      node(moveMob)  // N->destMob = N->moveMob - moveMob0;
      mr rTmp1,
      mcap mr rTmp2, mpiece mr rTmp3, mto subf rTmp5, rTmp4, rMoveMob sth rTmp5,
      node(destMob)
          bl CalcBlockMob  // CalcBlockMob(N->m.cap, N->m.piece, N->m.to);

      //--- Check castling ---
      // If it's a castling move, we also need to move the rook ---

      @CheckCastling andi.rTmp1,
      mtype, (mtype_O_O | mtype_O_O_O) beq + @Restore cmpi cr5, 0, mtype,
      mtype_O_O  // if (m.type == mtype_O_O_O)
              beq +
          cr5,
      @O_O  // {
      @O_O_O addi rTmp3,
      mfrom, -4 addi rTmp8, rEngine,
      ENGINE.V.MobVal  //    N->moveMob = -E->V.MobVal[N->m.from - 4];
          add rTmp4,
      rTmp3, rTmp3 lhax rTmp5, rTmp8, rTmp4 addi rTmp1, rPlayer,
      rook  //    CalcBlockMob(N->player + rook, empty, N->m.from - 4);
          li rTmp2,
      empty subf rMoveMob, rTmp5,
      rMoveMob bl CalcBlockMob

          sth rMoveMob,
      node(moveMob)  //    moveMob0 = N->moveMob;
      addi rTmp1,
      rPlayer,
      rook  //    CalcPieceMob(N->m.piece, N->m.to + 1);
          addi rTmp2,
      mto,
      1 bl CalcPieceMob

          lha rTmp4,
      node(moveMob)  //    E->V.MobVal[N->m.to + 1] = N->moveMob - moveMob0;
      addi rTmp8,
      rEngine,
      (ENGINE.V.MobVal + 2)  //    (OK because dest square is empty
      add rTmp2,
      mto, mto subf rTmp5, rTmp4, rMoveMob sthx rTmp5, rTmp8,
      rTmp2

          li rTmp1,
      empty  //    CalcBlockMob(empty, N->player + rook, N->m.to + 1);
          addi rTmp2,
      rPlayer, rook addi rTmp3, mto,
      1 bl CalcBlockMob

          add rTmp1,
      mfrom,
      mfrom  //    Board[m.from - 4]Ê= friendly + rook;
          add rTmp2,
      rBoard, rTmp1 addi rTmp3, rPlayer,
      rook  //    Board[m.from - 1]Ê= empty;
          li rTmp4,
      empty sth rTmp3, -8(rTmp2)sth rTmp4,
      -2(rTmp2)

          b @Restore  // }

      @O_O  // else
          addi rTmp3,
      mfrom,
      3  // {
      addi rTmp8,
      rEngine,
      ENGINE.V.MobVal  //    N->moveMob = -E->V.MobVal[N->m.from + 3];
          add rTmp4,
      rTmp3, rTmp3 lhax rTmp5, rTmp8, rTmp4 addi rTmp1, rPlayer,
      rook  //    CalcBlockMob(N->player + rook, empty, N->m.from + 3);
          li rTmp2,
      empty subf rMoveMob, rTmp5,
      rMoveMob bl CalcBlockMob

          sth rMoveMob,
      node(moveMob)  //    moveMob0 = N->moveMob;
      addi rTmp1,
      rPlayer,
      rook  //    CalcPieceMob(N->m.piece, N->m.to - 1);
          addi rTmp2,
      mto,
      -1 bl CalcPieceMob

          lha rTmp4,
      node(moveMob)  //    E->V.MobVal[N->m.to - 1] = N->moveMob - moveMob0;
      addi rTmp8,
      rEngine,
      (ENGINE.V.MobVal - 2)  //    (OK because dest square is empty
      add rTmp2,
      mto, mto subf rTmp5, rTmp4, rMoveMob sthx rTmp5, rTmp8,
      rTmp2

          li rTmp1,
      empty  //    CalcBlockMob(empty, N->player + rook, N->m.to - 1);
          addi rTmp2,
      rPlayer, rook addi rTmp3, mto,
      -1 bl CalcBlockMob

          add rTmp1,
      mfrom,
      mfrom  //    Board[m.from + 3]Ê= friendly + rook;
          add rTmp2,
      rBoard, rTmp1 addi rTmp3, rPlayer,
      rook  //    Board[m.from + 1]Ê= empty;
          li rTmp4,
      empty sth rTmp3, +6(rTmp2)sth rTmp4,
      +2(rTmp2)  // }

      //--- Restore board ---
      // "Unperform" move on the board.

      @Restore lhz mpiece,
      move(piece)  // Restore "mpiece" register
      lha rTmp1,
      node(mobEval) add rTmp2, mto,
      mto  // E->B.Board[N->m.to] = N->m.cap;
          add rTmp3,
      mfrom, mfrom andi.rTmp0, mtype, mtype_EP sthx mcap, rBoard,
      rTmp2  // E->B.Board[N->m.from] = N->m.piece;
          sthx mpiece,
      rBoard,
      rTmp3

              beq +
          @Return  // if (m.type == mtype_EP)
              neg rTmp4,
      rPlayer  //    E->B.Board[N->m.to - N->pawnDir] = enemy(pawn);
          addi rTmp4,
      rTmp4, black + pawn addi rTmp2, rTmp2, -0x20 sthx rTmp4, rBoard,
      rTmp2

      //--- Finally sum up mobility evaluation at next node ---

      @Return add rTmp1,
      rTmp1,
      rMoveMob  // NN->mobEval = N->mobEval + moveMob;
          sth rMoveThreat,
      node(moveThreat) sth rTmp1,
      nnode(mobEval)

          lwz rTmp2,
      node(dmobTop)  // (N->dmobTop + 1)->dmob = 0;
      li rTmp3,
      0 sth rTmp3,
      2(rTmp2)

          frame_end7  // return NN->mobEval;
              blr

  // #undef rMoveMob
  // #undef rMoveThreat
#undef mfrom
#undef mto
#undef mpiece
#undef mcap
#undef mtype
} /* EvalMoveMob */

/**************************************************************************************************/
/*                                                                                                */
/*                                  CALC MOBILITY FOR EACH PIECE */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------- Generic Piece Mobility
 * -------------------------------------*/
// Computes and returns the mobility and threat evaluation for the piece "p" on
// the square "sq". May NOT be called if "p" is empty.

asm void CalcPieceMob(register PIECE p, register SQUARE sq) {
  andi.rTmp0, p, 0x07 cmpi cr5, 0, rTmp0,
      bishop  // switch (pieceType(p))
          andi.rTmp1,
      p,
      0x10  // {
          bgt -
          cr5,
      @KQR  //    case king   : return CalcKingMob  (pieceColour(p), sq);
          beq cr5,
      CalcBishopMob  //    case queen  : return CalcQueenMob (pieceColour(p),
                     //    sq);
                         cmpi cr6,
      0, rTmp0,
      pawn  //    case rook   : return CalcRookMob  (pieceColour(p), sq);
              beq +
          cr6,
      CalcPawnMob  //    case bishop : return CalcBishopMob(pieceColour(p), sq);
          b CalcKnightMob  //    case knight : return
                           //    CalcKnightMob(pieceColour(p), sq);
      @KQR  //    case pawn   : return CalcPawnMob  (pieceColour(p), sq);
          cmpi cr6,
      0, rTmp0,
      queen  // }
              blt +
          cr6,
      CalcRookMob beq + cr6, CalcQueenMob b CalcKingMob
} /* CalcPieceMob */

/*------------------------------------- KING Mobility/Threats
 * ------------------------------------*/
// For kings we count mobility for all squares not occupied by a friendly pawn.
// Attacks of undefended enemy pieces are included too in the threat evaluation.

asm void CalcKingMob(register COLOUR colour, register SQUARE from) {
#define mob rTmp10
#define sq2 rTmp9
#define piece rTmp8
#define pc rTmp7
#define M100 rTmp6
#define cpawn rTmp5
#define from2 rTmp4

#define calcKingMobDir(M, N, dir)                                              \
  addi sq2, from2, 2 * (dir); /* for (INT i = 0; i <= 7; i++) */                                                                            \
  andi.rTmp0, sq2, 0x0110;  /* {                                           */  \
  bne - N;                  /*    SQUARE sq = from + Global.B.KingDir[i];  */  \
  lhzx piece, rBoard, sq2;  /*    if (offBoard(sq)) continue;              */  \
  addi cpawn, colour, pawn; /*    PIECE p = Board[sq];                     */  \
  cmp cr5, 0, piece, cpawn; /*    if (p == colour + pawn) continue;        */  \
  beq cr5, N;                                                                  \
  cmpi cr6, 0, piece, empty; /*                                             */ \
  addi mob, mob, mobVal_King; /*    mob += mobVal_King; */                                                                            \
  beq + cr6, N;         /*    if (p != empty &&                        */      \
  andi.pc, piece, 0x10; /*        pieceColour(p) != colour &&          */      \
  cmp cr7, 0, pc, colour;                                                      \
  add sq2, sq2, sq2; /*        ! N->Attack_[sq])                    */         \
  beq cr7, N;                                                                  \
  lwzx rTmp0, rAttack_, sq2;                                                   \
  cmpi cr6, 0, rTmp0, 0;                                                       \
  bne cr6, N; /*       N->moveThreat += Mtrl100[p];         */                 \
  addi M100, rGlobal, GLOBAL.B.Mtrl100;                                        \
  add piece, piece, piece;                                                     \
  lhzx rTmp0, M100, piece;                                                     \
  add rMoveThreat, rMoveThreat, rTmp0; /* } */                                                                            \
  N

  blr  //###

      li mob,
      0 add from2, from,
      from

          calcKingMobDir(@M0, @N0, -0x0F) calcKingMobDir(@M1, @N1, -0x11)
              calcKingMobDir(@M2, @N2, +0x11) calcKingMobDir(@M3, @N3, +0x0F)
                  calcKingMobDir(@M4, @N4, -0x10)
                      calcKingMobDir(@M5, @N5, +0x10)
                          calcKingMobDir(@M6, @N6, +0x01)
                              calcKingMobDir(@M7, @N7, -0x01)

                                  cmpi cr5,
      0, colour,
      white  // if (player == black) mob = -mob;
          beq cr5,
      @M neg mob, mob @M add rMoveMob, rMoveMob,
      mob  // N->moveMob += mob;

          blr

#undef mob
#undef sq2
#undef piece
#undef pc
#undef M100
#undef cpawn
#undef from2

#undef calcKingMobDir
} /* CalcKingMob */

/*---------------------------------- Queen Mobility/Threats/Pins
 * ---------------------------------*/
// Computes the queen mobility for the queen on the square "from". Also updates
// the "N->moveThreat" value.

asm void CalcQueenMob(register COLOUR colour, register SQUARE from) {
#define calcQueenMobDir(inx, dir) \
  li rTmp3, inx;                  \
  li rTmp4, 2 * dir;              \
  bl CalcQueenMobDir;             \
  add rMoveMob, rMoveMob, rTmp10

  frame_begin0 li rTmp5, true  // calcThreats = true; (common parameter)

      calcQueenMobDir(0, -0x0F) calcQueenMobDir(1, -0x11)
          calcQueenMobDir(2, +0x11) calcQueenMobDir(3, +0x0F)
              calcQueenMobDir(4, -0x10) calcQueenMobDir(5, +0x10)
                  calcQueenMobDir(6, +0x01) calcQueenMobDir(7, -0x01)

                      frame_end0 blr

#undef calcQueenMobDir
} /* CalcQueenMob */

asm void CalcQueenMobDir(
    register COLOUR colour,  // Colour of queen (NOT altered)
    register SQUARE from,    // Source square of queen (NOT altered)
    register INT
        dirInx,  // Index in QueenDir of the relevant direction (NOT altered)
    register INT dir2,         // 2*QueenDir[dirInx] (NOT altered)
    register BOOL calcThreats  // Calculate threats (i.e. update rMoveThreats)?
                               // (NOT altered)
    )

// This routine computes the queen mobility (and threats) along a single
// direction. "colour" is the colour of the queen, "from" is its location and
// "inxDir" is the direction index 0..7 in QueenDir[]. NOTE: Attacks/pins may
// NOT depend on whether the attacked/pinned piece is undefended. Otherwise it
// would be very tricky to update mobility incrementally.

// The returned mobility evaluation (rTmp10) is computed as seen from WHITE
// (i.e. negative if black) The returned move threat/turbulence evaluation is
// seen from PLAYER (i.e. always positive)

// NOTE: If calcThreats then colour = player!

{
#define mob rTmp10
#define sq2 rTmp9
#define xray rTmp8
#define p rTmp7
#define pc rTmp6
#define tmp rTmp6

  li mob,
      0    // mob = 0;
      blr  //###
          li xray,
      2  // xray = 2;
      add sq2,
      from,
      from b @OD

      @WHILE  // while (onBoard(sq) && xray > 0)
          lhzx p,
      rBoard,
      sq2  // {  PIECE p  = E->B.Board[sq];
          andi.pc,
      p,
      0x10  //    INT   pc = pieceColour(p);
      andi.p,
      p,
      0x07 beq + @CountMob

                     cmpi cr5,
      0, p,
      rook  //    switch (pieceColour(p))
              blt +
          cr5,
      @PNB  //    {
              beq +
          cr5,
      @Rook cmpi cr6, 0, p, queen beq + cr6,
      @Queen

      @King  //       case king :
          cmp cr5,
      0, pc,
      colour  //          if (pc == colour) goto RETURN;
          cmpi cr6,
      0, calcThreats,
      false  //          mob += mobVal_Queen + pinVal_King;
      beq cr5,
      @RETURN addi mob, mob,
      (mobVal_Queen + pinVal_King)  //          if (! calcThreats) goto RETURN;
      beq cr6,
      @RETURN cmpi cr7, 0, xray,
      2  //          N->moveThreat = (direct ? maxVal : 300);
      li rMoveThreat,
      maxVal beq cr7, @RETURN li rMoveThreat,
      300 b @RETURN  //          goto RETURN;

      @PNB cmpi cr6,
      0, p, knight bgt - cr6, @Bishop beq - cr6,
      @Knight @Pawn  //       case pawn :
          cmp cr5,
      0, pc,
      colour  //          if (pc == colour) goto RETURN;
          beq cr5,
      @RETURN addi mob, mob,
      mobVal_Queen  //          mob += mobVal_Queen;
          cmpi cr6,
      0, calcThreats,
      false  //          if (! calcThreats) goto RETURN;
      add tmp,
      sq2, sq2 beq cr6,
      @RETURN  //          if (! N->Attack_[sq])
          lwzx rTmp0,
      rAttack_,
      tmp  //             N->moveThreat += 100*pawnMtrl;
          cmpi cr7,
      0, rTmp0, 0 bne cr7, @RETURN addi rMoveThreat, rMoveThreat,
      100 * pawnMtrl b @RETURN  //          goto RETURN;

          @Knight  //       case knight :
              cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (Attack_[sq]) break;
          add tmp,
      sq2, sq2 lwzx rTmp0, rAttack_, tmp cmpi cr5, 0, rTmp0, 0 bne cr5,
      @Blocked li tmp,
      25 * knightMtrl  //          N->moveThreat += knightMtrl*(direct ? 100 :
                       //          50);
                           rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Bishop  //       case bishop :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (dirInx > 3 || Attack_[sq]) break;
          cmpi cr7,
      0, dirInx, 3 add tmp, sq2, sq2 ble cr7, @Blocked lwzx rTmp0, rAttack_,
      tmp cmpi cr5, 0, rTmp0, 0 bne cr5, @Blocked li tmp,
      25 * bishopMtrl  //          N->moveThreat += bishopMtrl*(direct ? 100 :
                       //          50);
                           rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Rook  //       case rook :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (dirInx <= 3 || Attack_[sq]) break;
          cmpi cr7,
      0, dirInx, 3 add tmp, sq2, sq2 bgt cr7, @Blocked lwzx rTmp0, rAttack_,
      tmp cmpi cr5, 0, rTmp0, 0 bne cr5, @Blocked li tmp,
      25 * rookMtrl  //          N->moveThreat += rookMtrl*(direct ? 100 : 50);
               rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Queen  //       case queen :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (Attack_[sq]) break;
          add tmp,
      sq2, sq2 lwzx rTmp0, rAttack_, tmp cmpi cr5, 0, rTmp0, 0 bne cr5,
      @Blocked li tmp,
      25 *
          queenMtrl  //          N->moveThreat += queenMtrl*(direct ? 100 : 50);
              rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp
      //    }
      @Blocked addi xray,
      xray,
      -1  //    if (p != empty) xray--;
      @CountMob addi mob,
      mob,
      mobVal_Queen  //    mob += mobVal_Queen;

      @OD add sq2,
      sq2, dir2 andi.rTmp0, sq2, 0x0110 cmpi cr5, 0, xray,
      0 bne -
          @RETURN  // }
              bgt +
          cr5,
      @WHILE

      @RETURN  // RETURN:
          cmpi cr5,
      0, colour,
      white  // if (colour == black) mob = -mob;
          beqlr cr5 neg mob,
      mob blr

#undef mob
#undef sq2
#undef xray
#undef p
#undef pc
#undef tmp
} /* CalcQueenMobDir */

/*---------------------------------- ROOK Mobility/Threats/Pins
 * ----------------------------------*/
// Computes the rook mobility for the rook on the square "from". Also updates
// the "N->moveThreat" value.

asm void CalcRookMob(register COLOUR colour, register SQUARE from) {
#define calcRookMobDir(inx, dir) /*li rTmp3,inx;*/ \
  li rTmp4, 2 * dir;                               \
  bl CalcRookMobDir;                               \
  add rMoveMob, rMoveMob, rTmp10

  frame_begin0 li rTmp5, true  // calcThreats = true; (common parameter)

      calcRookMobDir(4, -0x10) calcRookMobDir(5, +0x10) calcRookMobDir(6, +0x01)
          calcRookMobDir(7, -0x01)

              frame_end0 blr

#undef calcRookMobDir
} /* CalcQueenMob */

asm void CalcRookMobDir(
    register COLOUR colour,  // Colour of rook (NOT altered)
    register SQUARE from,    // Source square of rook (NOT altered)
    register INT dirInx,     // Index in QueenDir of the relevant direction (NOT
                             // NEEDED FOR ROOKS)
    register INT dir2,       // 2*QueenDir[dirInx] (NOT altered)
    register BOOL calcThreats  // Calculate threats (i.e. update rMoveThreats)?
                               // (NOT altered)
    )

// This routine computes the rook mobility (and threats) along a single
// direction. "colour" is the colour of the rook, "from" is its location and
// "dir" is the direction. NOTE: Attacks/pins may NOT depend on whether the
// attacked/pinned piece is undefended. Otherwise it would be very tricky to
// update mobility incrementally.

// The returned mobility evaluation (rTmp10) is computed as seen from WHITE
// (i.e. negative if black) The returned move threat/turbulence evaluation is
// seen from PLAYER (i.e. always positive)

// NOTE: If calcThreats then colour = player!

{
#define mob rTmp10
#define sq2 rTmp9
#define xray rTmp8
#define p rTmp7
#define pc rTmp6
#define tmp rTmp6

  li mob,
      0    // mob = 0;
      blr  //###
          li xray,
      2  // xray = 2;
      add sq2,
      from,
      from b @OD

      @WHILE  // while (onBoard(sq) && xray > 0)
          lhzx p,
      rBoard,
      sq2  // {  PIECE p  = E->B.Board[sq];
          andi.pc,
      p,
      0x10  //    INT   pc = pieceColour(p);
      andi.p,
      p,
      0x07 beq + @CountMob

                     cmpi cr5,
      0, p,
      rook  //    switch (pieceColour(p))
              blt +
          cr5,
      @PNB  //    {
              beq +
          cr5,
      @Rook cmpi cr6, 0, p, queen beq + cr6,
      @Queen

      @King  //       case king :
          cmp cr5,
      0, pc,
      colour  //          if (pc == colour) goto RETURN;
          cmpi cr6,
      0, calcThreats,
      false  //          mob += mobVal_Rook + pinVal_King;
      beq cr5,
      @RETURN addi mob, mob,
      (mobVal_Rook + pinVal_King)  //          if (! calcThreats) goto RETURN;
      beq cr6,
      @RETURN cmpi cr7, 0, xray,
      2  //          N->moveThreat = (direct ? maxVal : 300)
      li rMoveThreat,
      maxVal beq cr7, @RETURN li rMoveThreat,
      300 b @RETURN  //          goto RETURN;

      @PNB cmpi cr6,
      0, p, knight bgt - cr6, @Bishop beq - cr6,
      @Knight @Pawn  //       case pawn :
          cmp cr5,
      0, pc,
      colour  //          if (pc == colour) goto RETURN;
          beq cr5,
      @RETURN addi mob, mob,
      mobVal_Rook  //          mob += mobVal_Rook;
          cmpi cr6,
      0, calcThreats,
      false  //          if (! calcThreats) goto RETURN;
      add tmp,
      sq2, sq2 beq cr6,
      @RETURN  //          if (! N->Attack_[sq])
          lwzx rTmp0,
      rAttack_,
      tmp  //             N->moveThreat += 100*pawnMtrl;
          cmpi cr7,
      0, rTmp0, 0 bne cr7, @RETURN addi rMoveThreat, rMoveThreat,
      100 * pawnMtrl b @RETURN  //          goto RETURN;

          @Knight  //       case knight :
              cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (Attack_[sq]) break;
          add tmp,
      sq2, sq2 lwzx rTmp0, rAttack_, tmp cmpi cr5, 0, rTmp0, 0 bne cr5,
      @Blocked li tmp,
      25 * knightMtrl  //          N->moveThreat += knightMtrl*(direct ? 100 :
                       //          50);
                           rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Bishop  //       case bishop :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (Attack_[sq]) break;
          add tmp,
      sq2, sq2 lwzx rTmp0, rAttack_, tmp cmpi cr5, 0, rTmp0, 0 bne cr5,
      @Blocked li tmp,
      25 * bishopMtrl  //          N->moveThreat += bishopMtrl*(direct ? 100 :
                       //          50);
                           rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Rook  //       case rook :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (Attack_[sq]) break;
          add tmp,
      sq2, sq2 lwzx rTmp0, rAttack_, tmp cmpi cr5, 0, rTmp0, 0 bne cr5,
      @Blocked li tmp,
      25 * rookMtrl  //          N->moveThreat += rookMtrl*(direct ? 100 : 50);
               rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Queen  //       case queen :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6, @Blocked li tmp,
      25 *
          queenMtrl  //          N->moveThreat += queenMtrl*(direct ? 100 : 50);
              addi mob,
      mob,
      pinVal_Queen  //          mob += pinVal_Queen;
          rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp
      //    }
      @Blocked addi xray,
      xray,
      -1  //    if (p != empty) xray--;
      @CountMob addi mob,
      mob,
      mobVal_Rook  //    mob += mobVal_Rook;

      @OD add sq2,
      sq2, dir2 andi.rTmp0, sq2, 0x0110 cmpi cr5, 0, xray,
      0 bne -
          @RETURN  // }
              bgt +
          cr5,
      @WHILE

      @RETURN  // RETURN:
          cmpi cr5,
      0, colour,
      white  // if (colour == black) mob = -mob;
          beqlr cr5 neg mob,
      mob blr

#undef mob
#undef sq2
#undef xray
#undef p
#undef pc
#undef tmp
} /* CalcRookMobDir */

/*---------------------------------- BISHOP Mobility/Threats/Pins
 * --------------------------------*/
// Computes the bishop mobility for the bishop on the square "from". Also
// updates the "N->moveThreat" value.

asm void CalcBishopMob(register COLOUR colour, register SQUARE from) {
#define calcBishopMobDir(inx, dir) /*li rTmp3,inx;*/ \
  li rTmp4, 2 * dir;                                 \
  bl CalcBishopMobDir;                               \
  add rMoveMob, rMoveMob, rTmp10

  frame_begin0 li rTmp5, true  // calcThreats = true; (common parameter)

      calcBishopMobDir(0, -0x0F) calcBishopMobDir(1, -0x11)
          calcBishopMobDir(2, +0x11) calcBishopMobDir(3, +0x0F)

              frame_end0 blr

#undef calcBishopMobDir
} /* CalcQueenMob */

asm void CalcBishopMobDir(
    register COLOUR colour,  // Colour of bishop (NOT altered)
    register SQUARE from,    // Source square of bishop (NOT altered)
    register INT dirInx,     // Index in QueenDir of the relevant direction (NOT
                             // NEEDED FOR BISHOPS)
    register INT dir2,       // 2*QueenDir[dirInx] (NOT altered)
    register BOOL calcThreats  // Calculate threats (i.e. update rMoveThreats)?
                               // (NOT altered)
    )

// This routine computes the rook mobility (and threats) along a single
// direction. "colour" is the colour of the rook, "from" is its location and
// "dir" is the direction. NOTE: Attacks/pins may NOT depend on whether the
// attacked/pinned piece is undefended. Otherwise it would be very tricky to
// update mobility incrementally.

// The returned mobility evaluation (in rTmp10) is computed as seen from WHITE
// (i.e. negative if black) The returned move threat/turbulence evaluation is
// seen from PLAYER (i.e. always positive)

// NOTE: If calcThreats then colour = player!

{
#define mob rTmp10
#define sq2 rTmp9
#define xray rTmp8
#define p rTmp7
#define pc rTmp6
#define tmp rTmp6

  li mob,
      0    // mob = 0;
      blr  //###
          li xray,
      2  // xray = 2;
      add sq2,
      from,
      from b @OD

      @WHILE  // while (onBoard(sq) && xray > 0)
          lhzx p,
      rBoard,
      sq2  // {  PIECE p  = E->B.Board[sq];
          andi.pc,
      p,
      0x10  //    INT   pc = pieceColour(p);
      andi.p,
      p,
      0x07 beq + @CountMob

                     cmpi cr5,
      0, p,
      rook  //    switch (pieceColour(p))
              blt +
          cr5,
      @PNB  //    {
              beq +
          cr5,
      @Rook cmpi cr6, 0, p, queen beq + cr6,
      @Queen

      @King  //       case king :
          cmp cr5,
      0, pc,
      colour  //          if (pc == colour) goto RETURN;
          cmpi cr6,
      0, calcThreats,
      false  //          mob += mobVal_Bishop + pinVal_King;
      beq cr5,
      @RETURN addi mob, mob,
      (mobVal_Bishop + pinVal_King)  //          if (! calcThreats) goto RETURN;
      beq cr6,
      @RETURN cmpi cr7, 0, xray,
      2  //          N->moveThreat = (direct ? maxVal : 300)
      li rMoveThreat,
      maxVal beq cr7, @RETURN li rMoveThreat,
      300 b @RETURN  //          goto RETURN;

      @PNB cmpi cr6,
      0, p, knight bgt - cr6, @Bishop beq - cr6,
      @Knight @Pawn  //       case pawn :
          cmp cr5,
      0, pc,
      colour  //          if (pc == colour) goto RETURN;
          beq cr5,
      @RETURN addi mob, mob,
      mobVal_Bishop  //          mob += mobVal_Bishop;
          cmpi cr6,
      0, calcThreats,
      false  //          if (! calcThreats) goto RETURN;
      add tmp,
      sq2, sq2 beq cr6,
      @RETURN  //          if (! N->Attack_[sq])
          lwzx rTmp0,
      rAttack_,
      tmp  //             N->moveThreat += 100*pawnMtrl;
          cmpi cr7,
      0, rTmp0, 0 bne cr7, @RETURN addi rMoveThreat, rMoveThreat,
      100 * pawnMtrl b @RETURN  //          goto RETURN;

          @Knight  //       case knight :
              cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (Attack_[sq]) break;
          add tmp,
      sq2, sq2 lwzx rTmp0, rAttack_, tmp cmpi cr5, 0, rTmp0, 0 bne cr5,
      @Blocked li tmp,
      25 * knightMtrl  //          N->moveThreat += knightMtrl*(direct ? 100 :
                       //          50);
                           rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Bishop  //       case bishop :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6,
      @Blocked  //          if (Attack_[sq]) break;
          add tmp,
      sq2, sq2 lwzx rTmp0, rAttack_, tmp cmpi cr5, 0, rTmp0, 0 bne cr5,
      @Blocked li tmp,
      25 * bishopMtrl  //          N->moveThreat += bishopMtrl*(direct ? 100 :
                       //          50);
                           rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Rook  //       case rook :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6, @Blocked li tmp,
      25 * rookMtrl  //          N->moveThreat += rookMtrl*(direct ? 100 : 50);
               addi mob,
      mob,
      pinVal_Rook  //          mob += pinVal_Rook;
          rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp b @Blocked  //          break;

      @Queen  //       case queen :
          cmpi cr5,
      0, calcThreats,
      false  //          if (! calcThreats || pc == colour) break;
      cmp cr6,
      0, pc, colour beq cr5, @Blocked beq cr6, @Blocked li tmp,
      25 *
          queenMtrl  //          N->moveThreat += queenMtrl*(direct ? 100 : 50);
              addi mob,
      mob,
      pinVal_Queen  //          mob += pinVal_Queen;
          rlwnm tmp,
      tmp, xray, 0, 31 add rMoveThreat, rMoveThreat,
      tmp
      //    }
      @Blocked addi xray,
      xray,
      -1  //    if (p != empty) xray--;
      @CountMob addi mob,
      mob,
      mobVal_Bishop  //    mob += mobVal_Bishop;

      @OD add sq2,
      sq2, dir2 andi.rTmp0, sq2, 0x0110 cmpi cr5, 0, xray,
      0 bne -
          @RETURN  // }
              bgt +
          cr5,
      @WHILE

      @RETURN  // RETURN:
          cmpi cr5,
      0, colour,
      white  // if (colour == black) mob = -mob;
          beqlr cr5 neg mob,
      mob blr

#undef mob
#undef sq2
#undef xray
#undef p
#undef pc
#undef tmp
} /* CalcBishopMobDir */

/*----------------------------------- KNIGHT Mobility/Threats
 * ------------------------------------*/
// For knights we count mobility for all squares not occupied by a friendly
// pawn/king. And we calc threats too.

asm void CalcKnightMob(register COLOUR colour, register SQUARE from) {
  frame_begin0

      li rTmp4,
      true li rTmp3, -0x0E;
  bl CalcKnightMobDir  // for (INT i = 0; i < 8; i++)
      li rTmp3,
      -0x12;
  bl CalcKnightMobDir  //    CalcKnightMobDir(colour, from, dir, true);
      li rTmp3,
      -0x1F;
  bl CalcKnightMobDir li rTmp3, -0x21;
  bl CalcKnightMobDir li rTmp3, +0x12;
  bl CalcKnightMobDir li rTmp3, +0x0E;
  bl CalcKnightMobDir li rTmp3, +0x21;
  bl CalcKnightMobDir li rTmp3, +0x1F;
  bl CalcKnightMobDir

      frame_end0 blr
} /* CalcKnightMob */

asm void CalcKnightMobDir(
    register COLOUR colour,    // Colour of bishop (NOT altered)
    register SQUARE from,      // Source square of knight (NOT altered)
    register INT dir,          // KnightDir[dirInx] (NOT altered)
    register BOOL calcThreats  // Calculate threats (i.e. update rMoveThreats)?
                               // (NOT altered)
) {
#define sq rTmp8
#define p rTmp7
#define c rTmp6

  blr  //###

      add sq,
      from,
      dir  // SQUARE sq = from + dir;
          andi.rTmp0,
      sq,
      0x88  // if (offBoard(sq)) return;
          bnelr -

          add sq,
      sq,
      sq  // c = pieceColour(Board[sq]);
          lhzx p,
      rBoard,
      sq  // p = pieceType(Board[sq]);
          andi.c,
      p, 0x10 andi.p, p, 0x07 cmp cr7, 0, c,
      colour

          cmpi cr5,
      0, p,
      bishop  // switch (pieceType(p))
              bgt -
          cr5,
      @KQR  // {
          beq cr5,
      @Bishop cmpi cr6, 0, p,
      pawn  //    case empty :
              blt +
          cr6,
      @MOB  //       break;
              bgt -
          cr6,
      @Knight @Pawn  //    case pawn :
              beqlr +
          cr7  //       if (pc == colour) return;
              cmpi cr5,
      0, calcThreats,
      false  //       if (! calcThreats) break;
      beq cr5,
      @MOB add sq, sq,
      sq  //       if (Attack_[sq]) break;
          lwzx rTmp0,
      rAttack_, sq cmpi cr6, 0, rTmp0, 0 bne + cr6, @MOB addi rMoveThreat,
      rMoveThreat,
      100 * pawnMtrl      //       N->moveThreat += 100*pawnMtrl;
                  b @MOB  //       break;
              @Knight     //    case knight :
                  beq +
          cr7,
      @MOB  //       if (pc == colour ||Ê! calcThreats) break;
          cmpi cr5,
      0, calcThreats, false beq cr5, @MOB add sq, sq,
      sq  //       if (Attack_[sq]) break;
          lwzx rTmp0,
      rAttack_, sq cmpi cr6, 0, rTmp0, 0 bne + cr6, @MOB addi rMoveThreat,
      rMoveThreat,
      100 * knightMtrl    //       N->moveThreat += 100*knightMtrl;
                  b @MOB  //       break;
              @Bishop     //    case bishop :
                  beq +
          cr7,
      @MOB  //       if (pc == colour ||Ê! calcThreats) break;
          cmpi cr5,
      0, calcThreats, false beq cr5, @MOB add sq, sq,
      sq  //       if (Attack_[sq]) break;
          lwzx rTmp0,
      rAttack_, sq cmpi cr6, 0, rTmp0, 0 bne + cr6, @MOB addi rMoveThreat,
      rMoveThreat,
      100 * bishopMtrl    //       N->moveThreat += 100*bishopMtrl;
                  b @MOB  //       break;
              @Rook       //    case rook :
                  beq +
          cr7,
      @MOB  //       if (pc == colour ||Ê! calcThreats) break;
          cmpi cr5,
      0, calcThreats, false beq cr5, @MOB addi rMoveThreat, rMoveThreat,
      100 * rookMtrl      //       N->moveThreat += 100*rookMtrl;
                  b @MOB  //       break;
              @Queen      //    case queen :
                  beq +
          cr7,
      @MOB  //       if (pc == colour ||Ê! calcThreats) break;
          cmpi cr5,
      0, calcThreats, false beq cr5, @MOB addi rMoveThreat, rMoveThreat,
      100 * queenMtrl   //       N->moveThreat += 100*queenMtrl;
                b @MOB  //       break;
          @KQR cmpi cr6,
      0, p, queen blt + cr6, @Rook beq + cr6,
      @Queen @King  //    case king :
              beqlr +
          cr7  //       if (pc == colour) return;
              cmpi cr5,
      0, calcThreats,
      false  //       if (! calcThreats) break;
      beq cr5,
      @MOB li rMoveThreat,
      maxVal  //       N->moveThreat = maxVal;
              // }
      @MOB cmpi cr5,
      0, colour,
      white  // N->moveMob += (colour == white ? mobVal_Knight :
             // -mobVal_Knight);
                 bne cr5,
      @Black @White addi rMoveMob, rMoveMob, mobVal_Knight;
  blr @Black addi rMoveMob, rMoveMob, -mobVal_Knight;
  blr

#undef sq
#undef p
#undef c
} /* CalcKnightMobDir */

/*-------------------------------------- PAWN Mobility/Threats
 * -----------------------------------*/
// For pawns we count mobility if the square in front (2 squares if on 2nd rank)
// is unoccupied.
// ### Maybe threats only???
// Pawn moves to 6th, 7th or 8th rank are always searched (i.e. N->moveThreat =
// maxVal). Passed pawns??? May NOT be called for promotion moves (i.e. where
// the square is on the 8th rank). 50 points is always added to the threat
// evaluation to make up for the fact, that the pawn structure evaluation is not
// included.

asm void CalcPawnMob(register COLOUR colour, register SQUARE from) {
  blr  //###

#define sq rTmp8
#define p rTmp7
#define c rTmp6

      //--- Calc mobility ---
      //...

      //--- Check threats ---
      // If the pawn is standing on the 6th, 7th (or 8th) rank, its always a
      // threat (maxVal):
          cmpi cr5,
      0, colour, white srawi rTmp3, from, 4 beq cr5, @ChkFarPawn neg rTmp3,
      rTmp3 addi rTmp3, rTmp3, 7 @ChkFarPawn cmpi cr6, 0, rTmp3, 5 blt + cr6,
      @ChkAttack li rMoveThreat,
      maxVal blr

      // Otherwise check if there are any undefended and/or higher valued pieces
      // on the front left/right squares.

      @ChkAttack addi rMoveThreat,
      rMoveThreat,
      50  // N->moveThreat += 50;

      add from,
      from,
      rPawnDir  // from += pawnDir;
      @Left addi sq,
      from,
      -1  // sq = left(from);
       andi.rTmp0,
      sq,
      0x88  // if (onBoard(sq))
          bne -
          @R  // {
              add sq,
      sq,
      sq  //    PIECE p = Board[sq];
          lhzx p,
      rBoard, sq cmpi cr5, 0, p,
      empty  //    if (p != empty &&
              beq +
          cr5,
      @Right andi.c, p,
      0x10  //        pieceColour(p) != colour)
      cmp cr6,
      0, colour, c beq cr6, @Right add p, p,
      p  //       N->moveThreat += Global.B.Mtrl100[p];
          addi rTmp3,
      rGlobal, GLOBAL.B.Mtrl100 lhzx rTmp4, rTmp3,
      p  //
          add rMoveThreat,
      rMoveThreat,
      rTmp4  // }

      @Right addi sq,
      from,
      1  // sq = right(from);
      andi.rTmp0,
      sq,
      0x88         // if (onBoard(sq))
          bnelr -  // {
          @R add sq,
      sq,
      sq  //    PIECE p = Board[sq];
          lhzx p,
      rBoard, sq cmpi cr5, 0, p,
      empty  //    if (p != empty &&
              beqlr +
          cr5 andi.c,
      p,
      0x10  //        pieceColour(p) != colour)
      cmp cr6,
      0, colour, c beqlr cr6 add p, p,
      p  //       N->moveThreat += Global.B.Mtrl100[p];
          addi rTmp3,
      rGlobal, GLOBAL.B.Mtrl100 lhzx rTmp4, rTmp3,
      p  //
          add rMoveThreat,
      rMoveThreat,
      rTmp4  // }

          blr

#undef sq
#undef p
#undef c
} /* CalcPawnMob */

/**************************************************************************************************/
/*                                                                                                */
/*                                        CALC BLOCK MOBILITY */
/*                                                                                                */
/**************************************************************************************************/

// The "CalcBlockMob" function add/removes mobility caused by blocking pieces.
// "oldp0" is the current contents of the square "sq0", and  "newp0" is the new
// contents. On exit Board[sq0] is set to newp0.

asm void CalcBlockMob(register PIECE _oldp0, register PIECE _newp0,
                      register SQUARE _sq0) {
#define blockDir(col, src, proc)                                              \
  sthx oldp0, rBoard, sq0; /* Board[sq0] = oldp0;  */                         \
  mr rTmp1, col; /* mob -= CalcXMobDir(c1, src - dir, i, dir2, false); */     \
  subf rTmp2, dir, src;                                                       \
  srawi rTmp3, inx, 1;                                                        \
  srawi rTmp2, rTmp2, 1;                                                      \
  mr rTmp4, dir;                                                              \
  li rTmp5, false;                                                            \
  bl proc;                                                                    \
  subf rMoveMob, rTmp10, rMoveMob;                                            \
  sthx newp0, rBoard, sq0; /* Board[sq0] = newp0; */                          \
  add rTmp5, rPlayer, col;                                                    \
  addi rTmp5, rTmp5, -16;                                                     \
  bl proc; /* mob += CalcXMobDir(c1, src - dir, i, dir2, c1 == N->player); */ \
  add rMoveMob, rTmp10, rMoveMob

#define oldp0 rLocal1  // Old contents of sq0
#define newp0 rLocal2  // New contents of sq0
#define sq0 rLocal3    // Location of origin/source square (*2)
#define inx rLocal4    // Direction index 0..7 (*2)
#define dir rLocal5    // Direction (*2)

#define sq2 rTmp9    // Location of first blocking piece (*2)
#define sq1 rTmp8    // Location of first blocking piece (*2)
#define p1 rTmp7     // Piece type of first blocking piece
#define c1 rTmp6     // Colour of first blocking piece
#define p2 rTmp7     // Piece type of first blocking piece
#define c2 rTmp6     // Colour of first blocking piece
#define nbits rTmp6  // Knight bits for friendly knights attacking sq0.
#define Data rTmp7   // Knight data bits
#define dmob rTmp8   // Mobility change

  blr  //###

      frame_begin5

          mr oldp0,
      _oldp0 mr newp0, _newp0 add sq0, _sq0,
      _sq0

          /*- - - - - - - - - - - - - - - - - - - - KQRB(P) Blocks - - - - - - -
             - - - - - - - - - - -*/
              //### Also, we must remember to include pawns here for vertical
              //directions, if pawn mobility
              //    is included at some point.

                  li inx,
      0        // for (INT inx = 0; inx <= 7; inx++)
      @FORDIR  // {
          addi rTmp1,
      rGlobal,
      GLOBAL.B.QueenDir  //    INT dir = Global.B.QueenDir[inx];
          lhax dir,
      rTmp1, inx add dir, dir,
      dir

      //--- Find FIRST (direct) blocking piece in direction ---

      @DIRECT mr sq1,
      sq0  //    SQUARE sq1 = sq0;
      @L1  //    do
          subf sq1,
      dir,
      sq1  //    {  sq1 -= dir;
          lhzx p1,
      rBoard, sq1 andi.rTmp0, sq1,
      0x0110  //       if (offBoard(sq1)) goto NextDir;
          bne -
          @NEXTDIR  //    } while (Board[sq1] == empty);
              cmpi cr5,
      0, p1, empty beq + cr5,
      @L1

          andi.c1,
      p1,
      0x10  //    PIECE  p1 = Board[sq1];
      andi.p1,
      p1,
      0x07  //    PIECE  c1 = pieceColour(p1);

      cmpi cr5,
      0, p1,
      bishop  //    switch (pieceType(p1))
              bgt -
          cr5,
      @KQR1  //    {
          beq cr5,
      @Bishop1 cmpi cr6, 0, p1,
      pawn  //       case pawn : goto NextDir;
              beq +
          cr6,
      @NEXTDIR b @INDIRECT  //       case knight : break;
      @KQR1 cmpi cr6,
      0, p1, queen blt + cr6, @Rook1 beq + cr6,
      @Queen1 @King1  //       case king :
          subf rTmp1,
      dir,
      sq0  //          if (sq != sq0 - dir) goto NextDir;
          cmp cr5,
      0, sq1, rTmp1 addi rTmp2, c1, pawn bne + cr5,
      @NEXTDIR  //          if (oldp0 == c1 + pawn)
          cmp cr6,
      0, oldp0,
      rTmp2  //             N->moveMob += (c1 == white ? mobVal_King :
             //             -mobVal_King);
                 bne +
          cr6,
      @K cmpi cr7, 0, c1,
      white  //          else if (newp0 == c1 + pawn)
          beq cr7,
      @A  //             N->moveMob -= (c1 == white ? mobVal_King :
          //             -mobVal_King);
              b @B @K cmp cr6,
      0, newp0, rTmp2 bne + @NEXTDIR cmpi cr7, 0, c1, white beq cr7,
      @B @A addi rMoveMob, rMoveMob, +mobVal_King b @NEXTDIR @B addi rMoveMob,
      rMoveMob,
      -mobVal_King b @NEXTDIR  //          goto NextDir;

      @Queen1  //       case queen :
          sth sq1,
      node(mobSq) blockDir(c1, sq0, CalcQueenMobDir)  //          ...
      lhz sq1,
      node(mobSq) b @INDIRECT  //          break;
      @Rook1                   //       case rook :
          cmpi cr5,
      0, inx,
      6  //          if (i <= 3) break;
      ble cr5,
      @INDIRECT  //          ...
          sth sq1,
      node(mobSq) blockDir(c1, sq0, CalcRookMobDir) lhz sq1,
      node(mobSq) b @INDIRECT  //          break;
      @Bishop1                 //       case bishop :
          cmpi cr5,
      0, inx,
      6  //          if (i > 3) break;
      bgt cr5,
      @INDIRECT  //          ...
          sth sq1,
      node(mobSq) blockDir(c1, sq0, CalcBishopMobDir)  //          break;
      lhz sq1,
      node(mobSq)  //    }

      //--- Find SECOND (x-ray) blocking piece in direction ---

      @INDIRECT mr sq2,
      sq1  //    SQUARE sq2 = sq1;
      @L2  //    do
          subf sq2,
      dir,
      sq2  //    {  sq2 -= dir;
          lhzx p2,
      rBoard, sq2 andi.rTmp0, sq2,
      0x0110  //       if (offBoard(sq2)) goto NextDir;
          bne -
          @NEXTDIR  //    } while (Board[sq2] == empty);
              cmpi cr5,
      0, p2, empty beq + cr5,
      @L2

          andi.c2,
      p2,
      0x10  //    PIECE  p2 = Board[sq2];
      andi.p2,
      p2,
      0x07  //    PIECE  c2 = pieceColour(p2);

      cmpi cr5,
      0, p2,
      bishop  //    switch (pieceType(p1))
              blt +
          cr5,
      @NEXTDIR  //    {
          beq cr5,
      @Bishop2  //       case pawn : case knight : case king : break;
          cmpi cr6,
      0, p2, queen blt + cr6, @Rook2 bgt - cr6,
      @NEXTDIR @Queen2                        //       case queen :
          blockDir(c2, sq1, CalcQueenMobDir)  //          ...
      b @NEXTDIR                              //          break;
      @Rook2                                  //       case rook :
          cmpi cr5,
      0, inx,
      6  //          if (i <= 3) break;
      ble cr5,
      @NEXTDIR                                          //          ...
          blockDir(c2, sq1, CalcRookMobDir) b @NEXTDIR  //          break;
      @Bishop2                                          //       case bishop :
          cmpi cr5,
      0, inx,
      6  //          if (i > 3) break;
      bgt cr5,
      @NEXTDIR                                 //          ...
          blockDir(c2, sq1, CalcBishopMobDir)  //          break;
                                               //    }
      @NEXTDIR addi inx,
      inx, 2 cmpi cr5, 0, inx, 16 blt + cr5,
      @FORDIR  // }

          /*- - - - - - - - - - - - - - - - - - - - - N Blocks - - - - - - - - -
             - - - - - - - - - - - -*/
              // Finally check if we're moving a pawn/king and this unblocks any
              // friendly knights

                  add sq2,
      sq0,
      sq0  // ATTACK nbits = (N->Attack[sq0] & nMask);
          lhzx nbits,
      rAttack, sq2 sthx newp0, rBoard,
      sq0  // Board[sq0] = newp0;
          addi Data,
      rGlobal, GLOBAL.M.Ndata rlwinm.nbits, nbits, 3, 21,
      28 beq + @RETURN  // if (nbits == 0) return;

                   subf oldp0,
      rPlayer,
      oldp0  // if (oldp0 == pawn + N->player ||
          subf newp0,
      rPlayer, newp0 cmpi cr5, 0, oldp0,
      pawn  //     oldp0 == king + N->player)
          cmpi cr6,
      0, oldp0, king beq - cr5, @NOLDP0 bne + cr6,
      @NNEWP0  // {
      @NOLDP0 li dmob,
      mobVal_Knight  //    dmob = (player == white ? mobVal_Knight :
                     //    -mobVal_Knight);
                         cmpi cr7,
      0, rPlayer, white beq cr7, @OldFor neg dmob, dmob @OldFor lhaux dir, Data,
      nbits  //    do
             // subf    sq, dir,sq0                       //    {  sq = sq0 -
             // Ndata[nbits].ndir;
              add rMoveMob,
      dmob,
      rMoveMob  //       N->moveMob += dmob; // Later -> [sq];
          lha nbits,
      N_DATA.offset(Data)  //       bits = Ndata[bits].nextBits;
      cmpi cr5,
      0, nbits,
      0  //    } while(bits);
          bne -
          cr5,
      @OldFor b @RETURN  // }

      @NNEWP0  // else if (newp0 == pawn + N->player ||
          cmpi cr5,
      0, newp0, pawn cmpi cr6, 0, newp0,
      king  //          newp0 == king + N->player)
              beq -
          cr5,
      @N bne + cr6,
      @RETURN  // {
      @N li dmob,
      mobVal_Knight  //    dmob = (player == white ? mobVal_Knight :
                     //    -mobVal_Knight);
                         cmpi cr7,
      0, rPlayer, white beq cr7, @NewFor neg dmob, dmob @NewFor lhaux dir, Data,
      nbits  //    do
             // subf    sq, dir,sq0                       //    {  sq = sq0 -
             // Ndata[nbits].ndir;
              subf rMoveMob,
      dmob,
      rMoveMob  //       N->moveMob -= dmob; // Later -> [sq];
          lha nbits,
      N_DATA.offset(Data)  //       bits = Ndata[bits].nextBits;
      cmpi cr5,
      0, nbits,
      0  //    } while(bits);
          bne -
          cr5,
      @NewFor

      @RETURN frame_end5 blr  // }

#undef blockDir

#undef oldp0
#undef newp0
#undef sq0
#undef inx
#undef dir

#undef sq2
#undef sq1
#undef p1
#undef c1
#undef p2
#undef c2
#undef nbits
#undef Data
#undef dmob
} /* CalcBlockMob */

#endif
