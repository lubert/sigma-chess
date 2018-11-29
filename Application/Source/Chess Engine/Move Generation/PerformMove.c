/**************************************************************************************************/
/*                                                                                                */
/* Module  : PERFORMMOVE.C */
/* Purpose : This module contains Perform/Retract Move routines which are used
 * during the search. */
/*           "PerformMove()" performs the current move (m) at the current node
 * (rNode) on the     */
/*           and updates the various board related data structures
 * incrementally. Similarily, the */
/*           "RetractMove()" routine takes back the current move at the current
 * node.             */
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

#include "Attack.f"
#include "PerformMove.f"

// This module contains the routines for performing and retracting moves during
// the search. Register rNode is expected to contain a pointer to the current
// node "N". The two high-level routines "PerformMove" and "RetractMove"
// performs/retracts the move "N->m" by dispatching on the type of the moving
// piece (N->m.piece) and then calls the specialized lowel-level assembler
// routines. The following data structures are updated incrementally when a move
// is performed or retracted:
//
//         Board, HasMovedTo,
//         PieceLoc, PLinx, PawnStruct,
//         pieceCount,
//         Attack,
//         N->capInx, N->promInx
//
// The "Perform"-routines are basically designed as follows:
//
//         SubBlockAttack(to); (or SubPieceAttack(to) if it's a capture)
//         SubPieceAttack(from);
//         Move piece on the board
//         AddPieceAttack(to);
//         AddBlockAttack(from);
//
// whereas the "Retract"-routines basically are designed as follows:
//
//         SubBlockAttack(from);
//         SubPieceAttack(to);
//         Move piece back on the board
//         AddPieceAttack(from);
//         AddBlockAttack(to); (or AddPieceAttack(to) if it's a capture)

/**************************************************************************************************/
/*                                                                                                */
/*                                        GENERIC ASM MACROS */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------- Generic Perform Move Macros
 * --------------------------------*/
// Generic macro for removing captured pieces in case of captures. Doesn't
// actually move/remove any pieces on the board. NOTE: This macro expects a
// local register called "mobSum" to have been defined.

#define check_remove_cap                                                       \
  cmpi cr0, 0, mcap, empty /* if (! m.cap)                             */;     \
  mr rTmp1, dst /*    mobSum -= SubBlockAttack(m.to);       */;                \
  bne - cr0, @C /*                                          */;                \
  bl UpdBlockAttack /* else                                     */;            \
  b @D /* {                                        */;                         \
  @C addi rTmp6, rGlobal,                                                      \
      GLOBAL.B.PieceCountBit /*    pieceCount -= PieceCountBit[m.cap];   */;   \
  addi rTmp2, rEngine, ENGINE.B.PLinx /*    inx = PLinx[m.to]; */;           \
  add rTmp3, dst, dst /*                                          */;          \
  rlwinm rTmp7, mcap, 2, 0, 31 /*                                          */; \
  lhax rTmp4, rTmp2, rTmp3 /*                                          */;     \
  lwzx rTmp8, rTmp6, rTmp7 /*                                          */;     \
  li rTmp2, nullSq /*                                          */;             \
  add rTmp9, rTmp4, rTmp4 /*                                          */;      \
  sth rTmp4, node(capInx) /*    N->capInx = inx;                      */;      \
  subf rPieceCount, rTmp8,                                                     \
      rPieceCount /*                                          */;              \
  sthx rTmp2, rPieceLoc_,                                                      \
      rTmp9 /*    PieceLoc_[inx] = nullSq;              */;                    \
  bl UpdPieceAttack /*    mobSum -= SubPieceAttack(m.to);       */;            \
  @D subf mobSum, rTmp1, mobSum /* }                                        */

/*----------------------------------- Generic Retract Move Macros
 * --------------------------------*/
// Generic macro for re-placing captured pieces in case of captures. Restores
// the contents of the destination square.

#define check_replace_cap                                                      \
  add rTmp3, dst, dst /* Board[m.to] = m.cap;                     */;          \
  cmpi cr0, 0, mcap, empty /*                                          */;     \
  sthx mcap, rBoard, rTmp3 /* if (! m.cap)                             */;     \
  bne - cr0, @C /*                                          */;                \
  mr rTmp1, dst /*    AddBlockAttack(m.to);                 */;                \
  bl UpdBlockAttack /*                                          */;            \
  b @D /* else                                     */;                         \
  @C lhz rTmp4, node(capInx) /* {  inx = N->capInx;                      */;   \
  addi rTmp6, rGlobal,                                                         \
      GLOBAL.B.PieceCountBit /*    pieceCount += PieceCountBit[m.cap];   */;   \
  rlwinm rTmp7, mcap, 2, 0, 31 /*                                          */; \
  lwzx rTmp8, rTmp6, rTmp7 /*                                          */;     \
  addi rTmp2, rEngine,                                                         \
      ENGINE.B.PLinx /*                                          */;           \
  add rTmp9, rTmp4, rTmp4 /*                                          */;      \
  sthx rTmp4, rTmp2, rTmp3 /*    PLinx[m.to] = inx;                    */;     \
  mr rTmp1, dst /*                                          */;                \
  add rPieceCount, rTmp8,                                                      \
      rPieceCount /*                                          */;              \
  sthx dst, rPieceLoc_, rTmp9 /*    PieceLoc_[inx] = m.to;                */;  \
  bl UpdPieceAttack /*    AddPieceAttack(m.to);                 */;            \
  @D /* }                                        */

/**************************************************************************************************/
/*                                                                                                */
/*                                          PERFORM/RETRACT */
/*                                                                                                */
/**************************************************************************************************/

// The high level perform/retract routines simply dispatch on the piece type of
// moving piece and calls the specialized perform/retract routines. Note that
// the rooks and bishops share the same routines.

// NOTE: When the specialized routines are called, the function return address
// is stored in r0, and NOT in the link register. Therefore a specialized
// "frame_begin" macro is used.

/*-------------------------------------- Generic Perform Move
 * ------------------------------------*/

asm void PerformPawnMove(void);
asm void PerformKnightMove(void);
asm void PerformRookMove(void);
asm void PerformQueenMove(void);
asm void PerformKingMove(void);

asm void PerformMove(void) {
  // lhz     rTmp9, move(to)                  // E->V.MobVal[N->m.to] =
  // N->destMob; lhz     rTmp8, node(destMob)
  lhz rTmp1,
      move(piece) mflr r0
          // addi    rTmp7, rEngine,ENGINE.V.MobVal
          // add     rTmp9, rTmp9,rTmp9
          // sthx    rTmp8, rTmp7,rTmp9

          bl @d      // switch (pieceType(m->piece))
      @d mflr rTmp2  // {
          rlwinm rTmp1,
      rTmp1, 2, 27, 29 addi rTmp2, rTmp2, 20 add rTmp2, rTmp2,
      rTmp1 mtlr rTmp2 blr b
          PerformPawnMove          //    case pawn   : PerformPawnMove(); break;
              b PerformKnightMove  //    case knight : PerformKnightMove();
                                   //    break;
                                       b PerformRookMove  //    case bishop :
                                                          //    PerformBishopMove();
                                                          //    break;
                                                              b PerformRookMove  //    case rook   : PerformRookMove(); break;
                                                                  b PerformQueenMove  //    case queen  : PerformQueenMove(); break;
                                                                      b PerformKingMove  //    case king   : PerformKingMove(); break;
} /* PerformMove */  // }

/*-------------------------------------- Generic Retract Move
 * ------------------------------------*/

asm void RetractPawnMove(void);
asm void RetractKnightMove(void);
asm void RetractRookMove(void);
asm void RetractQueenMove(void);
asm void RetractKingMove(void);

asm void RetractMove(void) {
  // lhz     rTmp9, move(to)                  // E->V.MobVal[N->m.to] =
  // N->oldDestMob; lhz     rTmp8, node(oldDestMob)
  lhz rTmp1,
      move(piece) mflr r0
          // addi    rTmp7, rEngine,ENGINE.V.MobVal
          // add     rTmp9, rTmp9,rTmp9
          // sthx    rTmp8, rTmp7,rTmp9

          bl @d      // switch (pieceType(m->piece))
      @d mflr rTmp2  // {
          rlwinm rTmp1,
      rTmp1, 2, 27, 29 addi rTmp2, rTmp2, 20 add rTmp2, rTmp2,
      rTmp1 mtlr rTmp2 blr b
          RetractPawnMove          //    case pawn   : RetractPawnMove(); break;
              b RetractKnightMove  //    case knight : RetractKnightMove();
                                   //    break;
                                       b RetractRookMove  //    case bishop :
                                                          //    RetractBishopMove();
                                                          //    break;
                                                              b RetractRookMove  //    case rook   : RetractRookMove(); break;
                                                                  b RetractQueenMove  //    case queen  : RetractQueenMove(); break;
                                                                      b RetractKingMove  //    case king   : RetractKingMove(); break;
} /* RetractMove */  // }

/**************************************************************************************************/
/*                                                                                                */
/*                                            PAWN MOVES */
/*                                                                                                */
/**************************************************************************************************/

#define upd_pawn_attack(sq)                                              \
  add rTmp7, sq, rPawnDir /* A = (PTR)&AttackP[m.from + pawnDir];  */;   \
  rlwinm rTmp7, rTmp7, 2, 0, 31;                                         \
  add rTmp7, rAttack, rTmp7 /* A[-4] ^= pMaskL;                      */; \
  lhz rTmp8, -4(rTmp7);                                                  \
  lhz rTmp9, 4(rTmp7) /* A[+4] ^= pMaskR;                      */;       \
  xori rTmp8, rTmp8, 0x0400;                                             \
  xori rTmp9, rTmp9, 0x0200;                                             \
  sth rTmp8, -4(rTmp7);                                                  \
  sth rTmp9, 4(rTmp7);

/*-------------------------------------- Perform Pawn Moves
 * --------------------------------------*/

asm void PerformPawnMove(void) {
#define src rLocal1
#define dst rLocal2
#define mcap rLocal3
#define mobSum rLocal4

  frame_begin4_

      lhz mcap,
      move(cap) lhz dst, move(to) lhz src, move(from) lha mobSum,
      node(mobEval)  // mobSum = N->mobEval;

      /*- - - - - - - - - - - - - Remove any captured piece and its attack - - -
         - - - - - - - - - - */

      check_remove_cap

          /*- - - - - - - - - - - - - - - - - Move Pawn on the board - - - - - -
             - - - - - - - - - - - - */

              addi rTmp5,
      rEngine,
      ENGINE.B.PLinx  // PLinx[to] = PLinx[from];
          add rTmp1,
      src, src add rTmp2, dst, dst lhax rTmp6, rTmp5, rTmp1 li rTmp3,
      empty  // Board[from] = empty;
          addi rTmp4,
      rPlayer, pawn sthx rTmp3, rBoard,
      rTmp1  // Board[to] = pawn + player;
          sthx rTmp4,
      rBoard, rTmp2 sthx rTmp6, rTmp5, rTmp2 add rTmp7, rTmp6,
      rTmp6  // PieceLocP[PLinx[to]] = to;
          mr rTmp1,
      src sthx dst, rPieceLoc,
      rTmp7 bl UpdBlockAttack  // mobSum += AddBlockAttack(from);
          add mobSum,
      rTmp1,
      mobSum

          /*- - - - - - - - - - - - - - - - Remove source attack of Pawn - - - -
             - - - - - - - - - - - - */

              upd_pawn_attack(src)

      /*- - - - - - - - - - Add destination attack of Pawn and update pawn
         structure - - - - - - - - */

      addi rTmp1,
      rEngine,
      ENGINE.B.PawnStructW  // PawnStructP[rank(from)] -= bit(file(from));
          srawi rTmp2,
      rPlayer, 1 srawi rTmp3, src, 4 add rTmp1, rTmp1, rTmp2 lbzx rTmp4, rTmp1,
      rTmp3 li rTmp6, 1 andi.rTmp5, src, 0x07 lhz rTmp10,
      move(type) rlwnm rTmp6, rTmp6, rTmp5, 0, 31 xor rTmp4, rTmp6,
      rTmp4 stbx rTmp4, rTmp1,
      rTmp3

          andi.rTmp8,
      rTmp10,
      mtype_Promotion  // if (! isPromotion(*m))
          srawi rTmp3,
      dst,
      4  // {
          bne -
          @PROM  //    PawnStructP[rank(to)] += bit(file(to));
              lbzx rTmp4,
      rTmp1, rTmp3 li rTmp6, 1 andi.rTmp5, dst, 0x07 rlwnm rTmp6, rTmp6, rTmp5,
      0, 31 xor rTmp4, rTmp6, rTmp4 stbx rTmp4, rTmp1,
      rTmp3

          upd_pawn_attack(dst)  //    upd_pawn_attack(to);

      /*- - - - - - - - - - - - - - - - - - - - Do En Passant - - - - - - - - -
         - - - - - - - - - - -*/

      cmpi cr0,
      0, rTmp10,
      mtype_EP  //    if (m.type == mtype_EP)
              bne +
          @RETURN  //    {
              subf rTmp1,
      rPawnDir,
      dst                    //       sq = behind(to);
          bl UpdPieceAttack  //       mobSum -= SubPieceAttack(sq);
              subf mobSum,
      rTmp1, mobSum subf rTmp1, rPawnDir,
      dst bl UpdBlockAttack  //       mobSum += AddBlockAttack(sq);
          add mobSum,
      rTmp1, mobSum subf rTmp1, rPawnDir, dst addi rTmp4, rEngine,
      ENGINE.B.PLinx  //       inx = PLinx[sq];
          add rTmp1,
      rTmp1, rTmp1 addi rTmp9, rPlayer, -16 li rTmp5, 1 neg rTmp9,
      rTmp9 lhzx rTmp2, rTmp4, rTmp1 rlwnm rTmp6, rTmp5, rTmp9, 0,
      31  //       pieceCount -= PieceCountBit[enemy(pawn)];
      li rTmp8,
      empty subf rPieceCount, rTmp6, rPieceCount sthx rTmp8, rBoard,
      rTmp1  //       Board[sq] = empty;
          add rTmp3,
      rTmp2, rTmp2 li rTmp7,
      nullSq  //       PieceLoc_[inx] = nullSq;
          sth rTmp2,
      node(capInx)  //       N->capInx = inx
      sthx rTmp7,
      rPieceLoc_,
      rTmp3 b @RETURN  //    }
                       // }

      /*- - - - - - - - - - - - - - - - - - - - Do Promotion - - - - - - - - - -
         - - - - - - - - - - */

      @PROM  // else
          addi rTmp1,
      rGlobal,
      GLOBAL.B.PieceCountBit  // {
          rlwinm rTmp2,
      rTmp10, 2, 25,
      29  //    pieceCount -= PieceCountBit[pawn + player];
      lwzx rTmp3,
      rTmp1, rTmp2 addi rTmp5, rEngine,
      ENGINE.B.LastOffi  //    i = ++LastOffi[player];
          addi rTmp7,
      rEngine,
      ENGINE.B.PLinx  //    j = PLinx[m.to];
          add rTmp5,
      rTmp5, rPlayer add rTmp8, dst, dst lhzx rTmp6, rTmp5, rPlayer lhzx rTmp9,
      rTmp7, rTmp8 li rTmp4, 1 rlwnm rTmp4, rTmp4, rPlayer, 0,
      31  //    pieceCount += PieceCountBit[m.type];
      subf rPieceCount,
      rTmp4, rPieceCount addi rTmp6, rTmp6, 1 add rPieceCount, rTmp3,
      rPieceCount

          sth rTmp9,
      node(promInx)  //    N->promInx = j;
      sthx rTmp6,
      rTmp5,
      rPlayer

          cmp cr0,
      0, rTmp6,
      rTmp9  //    if (i != j)
              beq -
          cr0,
      @p  //    {
          sthx rTmp6,
      rTmp7,
      rTmp8  //       PLInx[to] = i;
          add rTmp3,
      rTmp6,
      rTmp6  //       sq = PieceLoc[i];
          lhax rTmp5,
      rPieceLoc, rTmp3 sthx dst, rPieceLoc,
      rTmp3  //       PieceLoc[i] = to;
          add rTmp4,
      rTmp9,
      rTmp9  //       PieceLoc[j] = sq;
          sthx rTmp5,
      rPieceLoc, rTmp4 cmpi cr0, 0, rTmp5,
      nullSq  //       if (sq != nullSq)
          beq cr0,
      @p  //           PLinx[sq] = j;
          add rTmp5,
      rTmp5, rTmp5 sthx rTmp9, rTmp7,
      rTmp5  //    }

      @p sthx rTmp10,
      rBoard,
      rTmp8  //    Board[m.to] = type;
          mr rTmp1,
      dst bl UpdPieceAttack  //    mobSum += AddPieceAttack(m.to);
          add mobSum,
      rTmp1,
      mobSum
      // }

      @RETURN sth mobSum,
      nnode(mobEval)  // NN->mobEval = mobSum;
      frame_end4 blr

#undef src
#undef dst
#undef mcap
#undef mobSum
} /* PerformPawnMove */

/*-------------------------------------- Retract Pawn Moves
 * --------------------------------------*/

asm void RetractPawnMove(void) {
#define src rLocal1
#define dst rLocal2
#define mcap rLocal3

  frame_begin3_

      lhz src,
      move(from) lhz dst, move(to) lhz mcap,
      move(cap)

      /* - - - - - - - - Remove destination attack of pawn and update pawn
         structure - - - - - - - - */

      addi rTmp1,
      rEngine,
      ENGINE.B.PawnStructW  // PawnStructP[rank(from)] += bit(file(from));
          srawi rTmp2,
      rPlayer, 1 srawi rTmp3, src, 4 add rTmp1, rTmp1, rTmp2 lbzx rTmp4, rTmp1,
      rTmp3 li rTmp6, 1 andi.rTmp5, src, 0x07 lhz rTmp10,
      move(type) rlwnm rTmp6, rTmp6, rTmp5, 0, 31 xor rTmp4, rTmp6,
      rTmp4 stbx rTmp4, rTmp1,
      rTmp3

          andi.rTmp8,
      rTmp10,
      mtype_Promotion  // if (! isPromotion(*m))
          srawi rTmp3,
      dst,
      4  // {
          bne -
          @PROM  //    PawnStructP[rank(to)] -= bit(file(to));
              lbzx rTmp4,
      rTmp1, rTmp3 li rTmp6, 1 andi.rTmp5, dst, 0x07 rlwnm rTmp6, rTmp6, rTmp5,
      0, 31 xor rTmp4, rTmp6, rTmp4 stbx rTmp4, rTmp1,
      rTmp3

          upd_pawn_attack(dst)  //    upd_pawn_attack(to);

      /*- - - - - - - - - - - - - - - - - - - Undo En Passant - - - - - - - - -
         - - - - - - - - - - -*/

      cmpi cr0,
      0, rTmp10,
      mtype_EP  //    if (m.type == mtype_EP)
              bne +
          @UNMOVE  //    {
              subf rTmp1,
      rPawnDir,
      dst bl UpdBlockAttack  //       UpdBlockAttack(sq);
          lhz rTmp2,
      node(capInx) subf rTmp1, rPawnDir, dst addi rTmp4, rEngine,
      ENGINE.B.PLinx  //       PLinx[sq] = capInx;
          add rTmp7,
      rTmp1, rTmp1 addi rTmp9, rPlayer, -16 sthx rTmp2, rTmp4, rTmp7 li rTmp5,
      1 neg rTmp9, rTmp9 rlwnm rTmp6, rTmp5, rTmp9, 0,
      31  //       pieceCount += PieceCountBit[enemy(pawn)];
      addi rTmp8,
      rTmp9, pawn add rPieceCount, rTmp6, rPieceCount sthx rTmp8, rBoard,
      rTmp7  //       Board[sq] = enemy(pawn);
          add rTmp3,
      rTmp2, rTmp2 sthx rTmp1, rPieceLoc_,
      rTmp3  //       PieceLoc_[capInx] = sq;
          subf rTmp1,
      rPawnDir,
      dst                    //       sq = behind(to);
          bl UpdPieceAttack  //       UpdPieceAttack(sq);
              b @UNMOVE      //    }
                             // }

      /*- - - - - - - - - - - - - - - - - - - Undo Promotion - - - - - - - - - -
         - - - - - - - - - - */

      @PROM  // else
          addi rTmp1,
      rGlobal,
      GLOBAL.B.PieceCountBit  // {
          rlwinm rTmp2,
      rTmp10, 2, 25,
      29  //    pieceCount -= PieceCountBit[pawn + player];
      lwzx rTmp3,
      rTmp1, rTmp2 addi rTmp5, rEngine,
      ENGINE.B.LastOffi  //    i = LastOffi[player]--;
          lhz rTmp9,
      node(promInx)  //    j = N->promInx;
      add rTmp5,
      rTmp5, rPlayer addi rTmp7, rEngine, ENGINE.B.PLinx lhzx rTmp6, rTmp5,
      rPlayer add rTmp8, dst, dst li rTmp4, 1 addi rTmp1, rTmp6, -1 rlwnm rTmp4,
      rTmp4, rPlayer, 0,
      31  //    pieceCount += PieceCountBit[m.type];
      sthx rTmp1,
      rTmp5, rPlayer subf rPieceCount, rTmp3, rPieceCount add rPieceCount,
      rTmp4,
      rPieceCount

          cmp cr0,
      0, rTmp6,
      rTmp9  //    if (i != j)
              beq -
          cr0,
      @p  //    {
          sthx rTmp9,
      rTmp7,
      rTmp8  //       PLInx[to] = j;
          add rTmp4,
      rTmp9,
      rTmp9  //       sq = PieceLoc[j];
          lhax rTmp5,
      rPieceLoc, rTmp4 sthx dst, rPieceLoc,
      rTmp4  //       PieceLoc[j] = to;

          add rTmp3,
      rTmp6,
      rTmp6  //       PieceLoc[i] = sq;
          sthx rTmp5,
      rPieceLoc,
      rTmp3

          cmpi cr0,
      0, rTmp5,
      nullSq      //       if (sq != nullSq)
          beq @p  //           PLinx[sq] = i;
              add rTmp5,
      rTmp5, rTmp5 sthx rTmp6, rTmp7,
      rTmp5  //    }
      @p mr rTmp1,
      dst bl UpdPieceAttack  //    UpdPieceAttack(m.to);
                             // }
      /*- - - - - - - - - - - - - - - - - Unmove Pawn on the board - - - - - - -
         - - - - - - - - - - */

      @UNMOVE addi rTmp7,
      rEngine,
      ENGINE.B.PLinx  // PLinx[from] = PLinx[to];
          add rTmp1,
      src, src add rTmp2, dst, dst lhzx rTmp8, rTmp7,
      rTmp2  // Board[from] = piece;
          addi rTmp3,
      rPlayer, pawn sthx rTmp3, rBoard, rTmp1 sthx rTmp8, rTmp7,
      rTmp1 add rTmp9, rTmp8,
      rTmp8  // PieceLoc[player + PLinx[from]] = from;
          mr rTmp1,
      src sthx src, rPieceLoc,
      rTmp9  // SubBlockAttack(from);
          bl UpdBlockAttack

              /*- - - - - - - - - - - - - - - - Replace source attack of Pawn -
                 - - - - - - - - - - - - - - -*/

                  upd_pawn_attack(src)

      /*- - - - - - - - - - - - - Replace any captured piece and its attack - -
         - - - - - - - - - - -*/

      check_replace_cap

      @RETURN frame_end3 blr

#undef src
#undef dst
#undef mcap
} /* RetractPawnMove */

/**************************************************************************************************/
/*                                                                                                */
/*                                           KNIGHT MOVES */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Perform Knight Moves
 * ------------------------------------*/

asm void PerformKnightMove(void) {
#define src rLocal1
#define dst rLocal2
#define mcap rLocal3
#define mobSum rLocal4

  frame_begin4_

      lhz mcap,
      move(cap) lhz dst, move(to) lhz src, move(from) lha mobSum,
      node(mobEval)  // mobSum = N->mobEval;

      /*- - - - - - - - - - - - - Remove any captured piece and its attack - - -
         - - - - - - - - - - */

      check_remove_cap

          /*- - - - - - - - - - - - - - - - - Move Knight on the board - - - - -
             - - - - - - - - - - - - */

              addi rTmp5,
      rEngine,
      ENGINE.B.PLinx  // PLinx[to] = PLinx[from];
          add rTmp1,
      src, src add rTmp2, dst, dst lhax rTmp6, rTmp5, rTmp1 li rTmp3,
      empty  // Board[from] = empty;
          addi rTmp4,
      rPlayer, knight sthx rTmp3, rBoard,
      rTmp1  // Board[to] = knight + player;
          sthx rTmp4,
      rBoard, rTmp2 sthx rTmp6, rTmp5, rTmp2 add rTmp7, rTmp6,
      rTmp6  // PieceLocP[PLinx[to]] = to;
          mr rTmp1,
      src sthx dst, rPieceLoc,
      rTmp7 bl UpdBlockAttack  // AddBlockAttack(from);
          add mobSum,
      rTmp1,
      mobSum

          /*- - - - - - - - - - - - - - - - - - "Move" Knight Attack - - - - - -
             - - - - - - - - - - - - */

              rlwinm rTmp1,
      src, 2, 0, 31 rlwinm rTmp2, dst, 2, 0, 31 add rTmp1, rAttack,
      rTmp1 add rTmp2, rAttack,
      rTmp2 upd_knight_attack(rTmp1) upd_knight_attack(rTmp2)

          sth mobSum,
      nnode(mobEval)  // NN->mobEval = mobSum;
      frame_end4 blr

#undef src
#undef dst
#undef mcap
#undef mobSum
} /* PerformKnightMove */

/*-------------------------------------- Retract Knight Moves
 * ------------------------------------*/

asm void RetractKnightMove(void) {
#define src rLocal1
#define dst rLocal2
#define mcap rLocal3

  frame_begin3_

      lhz src,
      move(from) lhz dst, move(to) lhz mcap,
      move(cap)

      /*- - - - - - - - - - - - - - - - Unmove Knight on the board - - - - - - -
         - - - - - - - - - - */

      mr rTmp1,
      src  // UpdBlockAttack(from);
          bl UpdBlockAttack addi rTmp5,
      rEngine,
      ENGINE.B.PLinx  // PLinx[from] = PLinx[to];
          add rTmp2,
      dst, dst add rTmp1, src, src lhax rTmp6, rTmp5, rTmp2 addi rTmp4, rPlayer,
      knight  // Board[from] = knight + player;
          sthx rTmp4,
      rBoard, rTmp1 add rTmp7, rTmp6,
      rTmp6  // PieceLoc[player + PLinx[from]] = from;
          sthx rTmp6,
      rTmp5, rTmp1 sthx src, rPieceLoc,
      rTmp7

          /*- - - - - - - - - - - - - Replace any captured piece and its attack
             - - - - - - - - - - - - -*/

              check_replace_cap

                  /*- - - - - - - - - - - - - - - - - - "Unmove" Knight Attack -
                     - - - - - - - - - - - - - - - - */

                      rlwinm rTmp1,
      src, 2, 0, 31 rlwinm rTmp2, dst, 2, 0, 31 add rTmp1, rAttack,
      rTmp1 add rTmp2, rAttack,
      rTmp2 upd_knight_attack(rTmp1) upd_knight_attack(rTmp2)

          frame_end3 blr

#undef src
#undef dst
#undef mcap
} /* RetractKnightMove */

/**************************************************************************************************/
/*                                                                                                */
/*                                         ROOK/BISHOP MOVES */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Perform Rook/Bishop Moves
 * ---------------------------------*/

asm void PerformRookMove(void) {
#define src2 rLocal1
#define dst2 rLocal2
#define mcap rLocal3
#define mbit_ rLocal3
#define mdir2 rLocal4
#define Data rLocal5
#define dbit rLocal5
#define mobSum rLocal6
#define dm rLocal7

#define src4 rTmp9
#define dst4 rTmp8
#define tdir2 rTmp7
#define tdir4 rTmp6
#define tbit rTmp5
#define tbit_ rTmp5
#define mdir4 rTmp7
#define mbit rTmp6
#define Ap rTmp4
#define Bp rTmp3

  frame_begin7_

      lhz mcap,
      move(cap) lhz dst2, move(to) lhz src2, move(from) lha mdir2,
      move(dir) lha mobSum,
      node(mobEval)  // mobSum = N->mobEval;

      /*- - - - - - - - - - - - - Remove any captured piece and its attack - - -
         - - - - - - - - - - */

      cmpi cr0,
      0, mcap,
      empty  // if (m.cap)
          add dst2,
      dst2, dst2 beq - cr0,
      @init  // {
          addi rTmp6,
      rGlobal,
      GLOBAL.B.PieceCountBit  //    pieceCount -= PieceCountBit[m.cap];
          addi rTmp2,
      rEngine,
      ENGINE.B.PLinx  //    inx:4 = PLinx[m.to];
          rlwinm rTmp7,
      mcap, 2, 0, 31 lhax rTmp4, rTmp2, dst2 lwzx rTmp8, rTmp6, rTmp7 li rTmp3,
      nullSq srawi rTmp1, dst2, 1 add rTmp9, rTmp4, rTmp4 sth rTmp4,
      node(capInx)  //    N->capInx = inx;
      subf rPieceCount,
      rTmp8, rPieceCount sthx rTmp3, rPieceLoc_,
      rTmp9                  //    PieceLoc_[inx] = nullSq;
          bl UpdPieceAttack  //    mobSum -= SubPieceAttack(m.to);
              subf mobSum,
      rTmp1,
      mobSum  // }

      /* - - - - - - - - - - - - - - - - - Initialize registers - - - - - - - -
         - - - - - - - - - - -*/
      @init addi Data,
      rGlobal,
      GLOBAL.P.RBUpdData  // RD = &RBUpdData[dir];
          rlwinm rTmp1,
      mdir2, 4, 0, 27 add src2, src2, src2 add src4, src2, src2 add dst4, dst2,
      dst2 add mdir2, mdir2,
      mdir2

          /* - - - - - - - - - - - - - - - - Update transversal attack - - - - -
             - - - - - - - - - - - - */

              lhaux tdir2,
      Data,
      rTmp1  // dm = (player == white ? RD->dm : -RD->dm);
          cmpi cr5,
      0, rPlayer, white lha dm, 10(Data)lhz tbit, 2(Data)beq cr5, @0 neg dm,
      dm

      @0 add Ap,
      rAttack,
      src4  // A = &Attack[from];
          add Bp,
      rBoard,
      src2  // B = &Board[from];
          add tdir4,
      tdir2, tdir2 @1 lwzux rTmp1, Ap,
      tdir4  // do
          lhzux rTmp2,
      Bp,
      tdir2  // {  A += RD->tdir;
          subf mobSum,
      dm,
      mobSum  //    mobSum -= dm;
          xor rTmp1,
      rTmp1,
      tbit  //    B += RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //    *A -= RD->tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @1  // }Êwhile (*B == empty);

      add Ap,
      rAttack,
      dst4  // A = &Attack[to];
          add Bp,
      rBoard,
      dst2  // B = &Board[to];
      @2 lwzux rTmp1,
      Ap,
      tdir4  // do
          lhzux rTmp2,
      Bp,
      tdir2  // {  A += RD->tdir;
          add mobSum,
      dm,
      mobSum  //    mobSum += dm;
          xor rTmp1,
      rTmp1,
      tbit  //    B += RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //    *A += RD->tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @2  // }Êwhile (*B == empty);

      lhz tbit_,
      4(Data)neg tdir2, tdir2 neg tdir4,
      tdir4

          add Ap,
      rAttack,
      src4  // A = &Attack[from];
          add Bp,
      rBoard,
      src2  // B = &Board[from];
      @3 lwzux rTmp1,
      Ap,
      tdir4  // do
          lhzux rTmp2,
      Bp,
      tdir2  // {  A -= RD->tdir;
          subf mobSum,
      dm,
      mobSum  //    mobSum -= dm;
          xor rTmp1,
      rTmp1,
      tbit_  //    B -= RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //    *A -= RD->_tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @3  // }Êwhile (*B == empty);

      add Ap,
      rAttack,
      dst4  // A = &Attack[to];
          add Bp,
      rBoard,
      dst2  // B = &Board[to];
      @4 lwzux rTmp1,
      Ap,
      tdir4  // do
          lhzux rTmp2,
      Bp,
      tdir2  // {  A -= RD->tdir;
          add mobSum,
      dm,
      mobSum  //    mobSum += dm;
          xor rTmp1,
      rTmp1,
      tbit_  //    B -= RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //    *A += RD->_tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @4  // }Êwhile (*B == empty);

      /* - - - - - - - - - - - Move Piece & Update attack along line of movement
         - - - - - - - - - - */

      addi rTmp4,
      rEngine, ENGINE.B.PLinx lhz rTmp10, move(piece) lhzx rTmp3, rTmp4,
      src2 lhz mbit,
      6(Data)  // Attack[to] -= RD->mbit;
      lwzx rTmp1,
      rAttack, dst4 cmpi cr5, 0, mcap, empty sthx rTmp10, rBoard,
      dst2  // Board[m.to]Ê= m.piece;
          sthx rTmp3,
      rTmp4,
      dst2  // PLinx[to] = PLinx[from];
          srawi rTmp2,
      dst2, 1 lhz mbit_,
      8(Data)  //  % "mbit_" overwrites "mcap" from now on
      add rTmp3,
      rTmp3, rTmp3 xor rTmp1, rTmp1, mbit sthx rTmp2, rPieceLoc,
      rTmp3  // PieceLocP[PLinx[to]] = to;
          stwx rTmp1,
      rAttack, dst4 or dbit, mbit,
      mbit_  //  % "dbit" overwrites "Data" from now on

          bne cr5,
      @C  // if (! m.cap)
          srawi rTmp1,
      dst2,
      1  //    mobSum -= SubBlockAttack(to);
      bl UpdBlockAttack subf mobSum,
      rTmp1, mobSum add src4, src2,
      src2  //  % restore volatile regs
          add dst4,
      dst2, dst2 add mdir4, mdir2, mdir2 mr rTmp2,
      src4 b @N  // else
      @C add Ap,
      rAttack,
      dst4  // {  A = &Attack[to];
          add mdir4,
      mdir2, mdir2 add Bp, rBoard,
      dst2  //    B = &Board[to];
      @F lwzux rTmp1,
      Ap,
      mdir4  //    do
          lhzux rTmp2,
      Bp,
      mdir2  //    {  A += RD->mdir;
          add mobSum,
      dm,
      mobSum  //       mobSum += dm;
          xor rTmp1,
      rTmp1,
      mbit  //       B += RD->mdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A += RD->mbit;
          stw rTmp1,
      0(Ap)  //    }Êwhile (*B == empty);
          beq +
          cr0,
      @F  // }

          mr rTmp2,
      src4  // sq = from + mdir;
          b @N @L lwzx rTmp1,
      rAttack,
      rTmp2  // while (sq != to)
          xor rTmp1,
      rTmp1,
      dbit  // {
          stwx rTmp1,
      rAttack,
      rTmp2  //    Attack[sq] += RD->dbit;
      @N add rTmp2,
      rTmp2, mdir4 cmp cr0, 0, rTmp2,
      dst4  //    sq += mdir;
              bne +
          cr0,
      @L  // }

          srawi rTmp1,
      src2,
      1  // mobSum += AddBlockAttack(from);
      bl UpdBlockAttack add mobSum,
      rTmp1,
      mobSum

          add src4,
      src2, src2 addi rTmp1, rEngine, ENGINE.B.HasMovedTo lwzx rTmp3, rAttack,
      src4  // Attack[from] += _mbit;
          li rTmp4,
      empty  // Board[m.from] = empty;
          lhzx rTmp2,
      rTmp1, dst2 sthx rTmp4, rBoard, src2 xor rTmp3, rTmp3, mbit_ addi rTmp2,
      rTmp2,
      1  // HasMovedTo[to]++;
      stwx rTmp3,
      rAttack, src4 sthx rTmp2, rTmp1,
      dst2

      @RETURN sth mobSum,
      nnode(mobEval)  // NN->mobEval = mobSum;
      frame_end7 blr

#undef src2
#undef dst2
#undef mcap
#undef mbit_
#undef mdir2
#undef Data
#undef dbit
#undef mobSum
#undef dm

#undef src4
#undef dst4
#undef tdir2
#undef tdir4
#undef tbit
#undef tbit_
#undef mdir4
#undef mbit
#undef Ap
#undef Bp
} /* PerformRookMove */

asm void CastleRook(void);

asm void CastleRook(void) { mflr r0 b PerformRookMove } /* CastleRook */

/*------------------------------------ Retract Rook/Bishop Moves
 * ---------------------------------*/

asm void RetractRookMove(void) {
#define src2 rLocal1
#define dst2 rLocal2
#define mdir2 rLocal3
#define Data rLocal4
#define mbit rLocal5
#define mbit_ rLocal4

#define mcap rTmp10
#define src4 rTmp9
#define dst4 rTmp8
#define tdir2 rTmp7
#define tdir4 rTmp6
#define tbit rTmp5
#define tbit_ rTmp5
#define mdir4 rTmp7
#define dbit rTmp5
#define Ap rTmp4
#define Bp rTmp3

  frame_begin5_

      lha mdir2,
      move(dir) lhz src2, move(from) lhz dst2,
      move(to)

      /* - - - - - - - - - - - - - - - - - Initialize registers - - - - - - - -
         - - - - - - - - - - -*/
      @init addi Data,
      rGlobal,
      GLOBAL.P.RBUpdData  // RD = &RBUpdData[dir];
          rlwinm rTmp1,
      mdir2, 4, 0, 27 add src2, src2, src2 add dst2, dst2, dst2 add src4, src2,
      src2 add dst4, dst2, dst2 add mdir2, mdir2,
      mdir2

          /* - - - - - - - - - - - - - - - - Update transversal attack - - - - -
             - - - - - - - - - - - - */

              lhaux tdir2,
      Data, rTmp1 lhz tbit,
      2(Data)

          add Ap,
      rAttack,
      src4  // A = &Attack[from];
          add Bp,
      rBoard,
      src2  // B = &Board[from];
          add tdir4,
      tdir2, tdir2 @1 lwzux rTmp1, Ap,
      tdir4  // do
          lhzux rTmp2,
      Bp,
      tdir2  // {  A += RD->tdir;
          xor rTmp1,
      rTmp1,
      tbit  //    B += RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //    *A += RD->tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @1  // }Êwhile (*B == empty);

      add Ap,
      rAttack,
      dst4  // A = &Attack[to];
          add Bp,
      rBoard,
      dst2  // B = &Board[to];
      @2 lwzux rTmp1,
      Ap,
      tdir4  // do
          lhzux rTmp2,
      Bp,
      tdir2  // {  A += RD->tdir;
          xor rTmp1,
      rTmp1,
      tbit  //    B += RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //    *A -= RD->tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @2  // }Êwhile (*B == empty);

      lhz tbit_,
      4(Data)neg tdir2, tdir2 neg tdir4,
      tdir4

          add Ap,
      rAttack,
      src4  // A = &Attack[from];
          add Bp,
      rBoard,
      src2  // B = &Board[from];
      @3 lwzux rTmp1,
      Ap,
      tdir4  // do
          lhzux rTmp2,
      Bp,
      tdir2  // {  A -= RD->tdir;
          xor rTmp1,
      rTmp1,
      tbit_  //    B -= RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //    *A += RD->_tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @3  // }Êwhile (*B == empty);

      add Ap,
      rAttack,
      dst4  // A = &Attack[to];
          add Bp,
      rBoard,
      dst2  // B = &Board[to];
      @4 lwzux rTmp1,
      Ap,
      tdir4  // do
          lhzux rTmp2,
      Bp,
      tdir2  // {  A -= RD->tdir;
          xor rTmp1,
      rTmp1,
      tbit_  //    B -= RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //    *A -= RD->_tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @4  // }Êwhile (*B == empty);

      /* - - - - - - - - - - Unmove Piece & Update attack along line of movement
         - - - - - - - - - - */

      lhz mbit,
      6(Data)lhz rTmp4, move(piece) lhz mbit_,
      8(Data)  // % overwrites "Data" from now on...
      lwzx rTmp3,
      rAttack,
      src4  // Attack[from] -= _mbit;
          addi rTmp1,
      rEngine, ENGINE.B.HasMovedTo lhzx rTmp2, rTmp1,
      dst2  // Board[m.from] = m.piece;
          sthx rTmp4,
      rBoard, src2 xor rTmp3, rTmp3, mbit_ addi rTmp2, rTmp2,
      -1  // HasMovedTo[to]--;
      stwx rTmp3,
      rAttack, src4 sthx rTmp2, rTmp1,
      dst2

          srawi rTmp1,
      src2,
      1  // SubBlockAttack(from);
      bl UpdBlockAttack lhz mcap,
      move(cap)

          add rTmp2,
      src2,
      src2  // sq = from + mdir;
          add dst4,
      dst2,
      dst2  //    % restore volatile regs
          or dbit,
      mbit, mbit_ add mdir4, mdir2, mdir2 b @N @L lwzx rTmp1, rAttack,
      rTmp2  // while (sq != to)
          xor rTmp1,
      rTmp1,
      dbit  // {
          stwx rTmp1,
      rAttack,
      rTmp2  //    Attack[sq] -= dbit;
      @N add rTmp2,
      rTmp2, mdir4 cmp cr0, 0, rTmp2,
      dst4  //    sq += mdir;
              bne +
          cr0,
      @L  // }

          cmpi cr6,
      0, mcap,
      empty  // % must set cr6 BEFORE calling
          srawi rTmp1,
      dst2, 1 bne cr6,
      @C                     // if (! m.cap)
          bl UpdBlockAttack  //    AddBlockAttack(to);
              lhz mcap,
      move(cap)  //    % reload volatile "mcap"
      add dst4,
      dst2,
      dst2      //    % restore volatile regs
          b @M  // else
      @C add Ap,
      rAttack,
      dst4  // {  A = &Attack[to];
          add mdir4,
      mdir2, mdir2 add Bp, rBoard,
      dst2  //    B = &Board[to];
      @F lwzux rTmp1,
      Ap,
      mdir4  //    do
          lhzux rTmp2,
      Bp,
      mdir2  //    {  A += mdir;
          xor rTmp1,
      rTmp1,
      mbit  //       B += mdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A -= mbit;
          stw rTmp1,
      0(Ap)  //    }Êwhile (*B == empty);
          beq +
          cr0,
      @F  // }

      @M addi rTmp4,
      rEngine,
      ENGINE.B.PLinx  // Attack[to] += mbit;
          lhzx rTmp3,
      rTmp4, dst2 lwzx rTmp1, rAttack, dst4 sthx mcap, rBoard,
      dst2  // Board[m.to]Ê= m.cap;
          sthx rTmp3,
      rTmp4,
      src2  // PLinx[from] = PLinx[to];
          srawi rTmp2,
      src2, 1 add rTmp3, rTmp3, rTmp3 xor rTmp1, rTmp1, mbit sthx rTmp2,
      rPieceLoc,
      rTmp3  // PieceLocP[PLinx[from]] = from;
          stwx rTmp1,
      rAttack,
      dst4

              /*- - - - - - - - - - - - - Replace any captured piece and its
                 attack - - - - - - - - - - - - -*/

                  beq -
          cr6,
      @RETURN  // if (m.cap)
          lhz rTmp4,
      node(capInx)  // {
      addi rTmp6,
      rGlobal,
      GLOBAL.B.PieceCountBit  //    pieceCount += PieceCountBit[m.cap];
          addi rTmp2,
      rEngine, ENGINE.B.PLinx rlwinm rTmp7, mcap, 2, 0, 31 sthx rTmp4, rTmp2,
      dst2  //    PLInx[m.to] = N->capInx;
          lwzx rTmp8,
      rTmp6, rTmp7 srawi rTmp1, dst2, 1 add rTmp9, rTmp4, rTmp4 add rPieceCount,
      rTmp8,
      rPieceCount  //    PieceLoc_[N->capInx] = m.to;
          sthx rTmp1,
      rPieceLoc_,
      rTmp9 bl UpdPieceAttack  //    AddPieceAttack(m.to);
                               // }
      @RETURN frame_end5 blr

#undef src2
#undef dst2
#undef mdir2
#undef Data
#undef mbit
#undef mbit_

#undef mcap
#undef src4
#undef dst4
#undef tdir2
#undef tdir4
#undef tbit
#undef tbit_
#undef mdir4
#undef dbit
#undef Ap
#undef Bp
} /* RetractRookMove */

asm void UncastleRook(void);

asm void UncastleRook(void) { mflr r0 b RetractRookMove } /* UncastleRook */

/**************************************************************************************************/
/*                                                                                                */
/*                                           QUEEN MOVES */
/*                                                                                                */
/**************************************************************************************************/

asm void PerformQueenMove(void) {
#define src2 rLocal1
#define dst2 rLocal2
#define mcap rLocal3
#define mbit_ rLocal3
#define mdir2 rLocal4
#define Data rLocal5
#define dbit rLocal5
#define mobSum rLocal6
#define dm rLocal7

#define ti rTmp10
#define src4 rTmp9
#define dst4 rTmp8
#define tdir2 rTmp7
#define tdir4 rTmp6
#define tbit rTmp5
#define tbit_ rTmp5
#define mdir4 rTmp7
#define mbit rTmp6
#define Ap rTmp4
#define Bp rTmp3

  frame_begin7_

      lhz mcap,
      move(cap) lhz dst2, move(to) lhz src2, move(from) lha mdir2,
      move(dir) lha mobSum,
      node(mobEval)  // mobSum = N->mobEval;

      /*- - - - - - - - - - - - - Remove any captured piece and its attack - - -
         - - - - - - - - - - */

      cmpi cr0,
      0, mcap,
      empty  // if (m.cap)
          add dst2,
      dst2, dst2 beq - cr0,
      @init  // {
          addi rTmp6,
      rGlobal,
      GLOBAL.B.PieceCountBit  //    pieceCount -= PieceCountBit[m.cap];
          addi rTmp2,
      rEngine,
      ENGINE.B.PLinx  //    inx:4 = PLinx[m.to];
          rlwinm rTmp7,
      mcap, 2, 0, 31 lhax rTmp4, rTmp2, dst2 lwzx rTmp8, rTmp6, rTmp7 li rTmp3,
      nullSq srawi rTmp1, dst2, 1 add rTmp9, rTmp4, rTmp4 sth rTmp4,
      node(capInx)  //    N->capInx = inx;
      subf rPieceCount,
      rTmp8, rPieceCount sthx rTmp3, rPieceLoc_,
      rTmp9                  //    PieceLoc_[inx] = nullSq;
          bl UpdPieceAttack  //    mobSum -= SubPieceAttack(m.to);
              subf mobSum,
      rTmp1,
      mobSum  // }

      /* - - - - - - - - - - - - - - - - - Initialize registers - - - - - - - -
         - - - - - - - - - - -*/
      @init addi Data,
      rGlobal,
      (GLOBAL.P.QUpdData - 2)  // RD = &QUpdData[dir];
      rlwinm rTmp1,
      mdir2, 5, 0, 26 add src2, src2, src2 add src4, src2, src2 add dst4, dst2,
      dst2 add Data, Data, rTmp1 add mdir2, mdir2,
      mdir2

          cmpi cr5,
      0, rPlayer,
      white  // dm = (player == white ? queenMob : -queenMob);
          li dm,
      queenMob beq cr5, @0 neg dm,
      dm @0
      /* - - - - - - - - - - - - - - - - Update transversal attack - - - - - - -
         - - - - - - - - - - */

      li ti,
      3           // for (ti = 0; ti <= 2; ti++)
      @transloop  // {
          lhau tdir2,
      2(Data)lhzu tbit, 2(Data)addi ti, ti,
      -1

      add Ap,
      rAttack,
      src4  //    A = &Attack[from];
          add Bp,
      rBoard,
      src2  //    B = &Board[from];
          add tdir4,
      tdir2, tdir2 @1 lwzux rTmp1, Ap,
      tdir4  //    do
          lhzux rTmp2,
      Bp,
      tdir2  //    {  A += RD->tdir;
          subf mobSum,
      dm,
      mobSum  //       mobSum -= dm;
          xor rTmp1,
      rTmp1,
      tbit  //       B += RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A -= RD->tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @1  //    }Êwhile (*B == empty);

      add Ap,
      rAttack,
      dst4  //    A = &Attack[to];
          add Bp,
      rBoard,
      dst2  //    B = &Board[to];
      @2 lwzux rTmp1,
      Ap,
      tdir4  //    do
          lhzux rTmp2,
      Bp,
      tdir2  //    {  A += RD->tdir;
          add mobSum,
      dm,
      mobSum  //       mobSum += dm;
          xor rTmp1,
      rTmp1,
      tbit  //       B += RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A += RD->tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @2  //    }Êwhile (*B == empty);

      lhzu tbit_,
      2(Data)neg tdir2, tdir2 neg tdir4, tdir4 cmpi cr6, 0, ti,
      0

      add Ap,
      rAttack,
      src4  //    A = &Attack[from];
          add Bp,
      rBoard,
      src2  //    B = &Board[from];
      @3 lwzux rTmp1,
      Ap,
      tdir4  //    do
          lhzux rTmp2,
      Bp,
      tdir2  //    {  A -= RD->tdir;
          subf mobSum,
      dm,
      mobSum  //       mobSum -= dm;
          xor rTmp1,
      rTmp1,
      tbit_  //       B -= RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A -= RD->_tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @3  //    }Êwhile (*B == empty);

      add Ap,
      rAttack,
      dst4  //    A = &Attack[to];
          add Bp,
      rBoard,
      dst2  //    B = &Board[to];
      @4 lwzux rTmp1,
      Ap,
      tdir4  //    do
          lhzux rTmp2,
      Bp,
      tdir2  //    {  A -= RD->tdir;
          add mobSum,
      dm,
      mobSum  //       mobSum += dm;
          xor rTmp1,
      rTmp1,
      tbit_  //       B -= RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A += RD->_tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @4  //    }Êwhile (*B == empty);

          bgt +
          cr6,
      @transloop  // }

          /* - - - - - - - - - - - Move Piece & Update attack along line of
             movement - - - - - - - - - - */

              addi rTmp4,
      rEngine, ENGINE.B.PLinx addi rTmp10, rPlayer, queen lhzx rTmp3, rTmp4,
      src2 lhzu mbit,
      2(Data)  // Attack[to] -= RD->mbit;
      lwzx rTmp1,
      rAttack, dst4 cmpi cr5, 0, mcap, empty sthx rTmp10, rBoard,
      dst2  // Board[m.to]Ê= player + queen;
          sthx rTmp3,
      rTmp4,
      dst2  // PLinx[to] = PLinx[from];
          srawi rTmp2,
      dst2, 1 lhz mbit_,
      2(Data)  //  % "mbit_" overwrites "mcap" from now on
      add rTmp3,
      rTmp3, rTmp3 xor rTmp1, rTmp1, mbit sthx rTmp2, rPieceLoc,
      rTmp3  // PieceLocP[PLinx[to]] = to;
          stwx rTmp1,
      rAttack, dst4 or dbit, mbit,
      mbit_  //  % "dbit" overwrites "Data" from now on

          bne cr5,
      @C  // if (! m.cap)
          srawi rTmp1,
      dst2,
      1  //    mobSum -= SubBlockAttack(to);
      bl UpdBlockAttack subf mobSum,
      rTmp1, mobSum add src4, src2,
      src2  //  % restore volatile regs
          add dst4,
      dst2, dst2 add mdir4, mdir2, mdir2 mr rTmp2,
      src4 b @N  // else
      @C add Ap,
      rAttack,
      dst4  // {  A = &Attack[to];
          add mdir4,
      mdir2, mdir2 add Bp, rBoard,
      dst2  //    B = &Board[to];
      @F lwzux rTmp1,
      Ap,
      mdir4  //    do
          lhzux rTmp2,
      Bp,
      mdir2  //    {  A += RD->mdir;
          add mobSum,
      dm,
      mobSum  //       mobSum += dm;
          xor rTmp1,
      rTmp1,
      mbit  //       B += RD->mdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A += RD->mbit;
          stw rTmp1,
      0(Ap)  //    }Êwhile (*B == empty);
          beq +
          cr0,
      @F  // }

          mr rTmp2,
      src4  // sq = from + mdir;
          b @N @L lwzx rTmp1,
      rAttack,
      rTmp2  // while (sq != to)
          xor rTmp1,
      rTmp1,
      dbit  // {
          stwx rTmp1,
      rAttack,
      rTmp2  //    Attack[sq] += RD->dbit;
      @N add rTmp2,
      rTmp2, mdir4 cmp cr0, 0, rTmp2,
      dst4  //    sq += mdir;
              bne +
          cr0,
      @L  // }

          srawi rTmp1,
      src2,
      1  // mobSum += AddBlockAttack(from);
      bl UpdBlockAttack add mobSum,
      rTmp1,
      mobSum

          add src4,
      src2, src2 lwzx rTmp3, rAttack,
      src4  // Attack[from] += _mbit;
          li rTmp4,
      empty  // Board[m.from] = empty;
          sthx rTmp4,
      rBoard, src2 xor rTmp3, rTmp3, mbit_ stwx rTmp3, rAttack,
      src4

      @RETURN sth mobSum,
      nnode(mobEval)  // NN->mobEval = mobSum;
      frame_end7 blr

#undef src2
#undef dst2
#undef mcap
#undef mbit_
#undef mdir2
#undef Data
#undef dbit
#undef mobSum
#undef dm

#undef ti
#undef src4
#undef dst4
#undef tdir2
#undef tdir4
#undef tbit
#undef tbit_
#undef mdir4
#undef mbit
#undef Ap
#undef Bp
} /* PerformQueenMove */

asm void RetractQueenMove(void) {
#define src2 rLocal1
#define dst2 rLocal2
#define mdir2 rLocal3
#define Data rLocal4
#define mbit rLocal5
#define mbit_ rLocal4

#define ti rTmp10
#define mcap rTmp10
#define src4 rTmp9
#define dst4 rTmp8
#define tdir2 rTmp7
#define tdir4 rTmp6
#define tbit rTmp5
#define tbit_ rTmp5
#define mdir4 rTmp7
#define dbit rTmp5
#define Ap rTmp4
#define Bp rTmp3

  frame_begin5_

      lha mdir2,
      move(dir) lhz src2, move(from) lhz dst2,
      move(to)

      /* - - - - - - - - - - - - - - - - - Initialize registers - - - - - - - -
         - - - - - - - - - - -*/
      @init addi Data,
      rGlobal,
      (GLOBAL.P.QUpdData - 2)  // RD = &QUpdData[dir];
      rlwinm rTmp1,
      mdir2, 5, 0, 26 add src2, src2, src2 add dst2, dst2, dst2 add src4, src2,
      src2 add dst4, dst2, dst2 add Data, Data, rTmp1 add mdir2, mdir2,
      mdir2

          /* - - - - - - - - - - - - - - - - Update transversal attack - - - - -
             - - - - - - - - - - - - */

              li ti,
      3           // for (ti = 0; ti <= 2; ti++)
      @transloop  // {
          lhau tdir2,
      2(Data)lhzu tbit, 2(Data)addi ti, ti,
      -1

      add Ap,
      rAttack,
      src4  //    A = &Attack[from];
          add Bp,
      rBoard,
      src2  //    B = &Board[from];
          add tdir4,
      tdir2, tdir2 @1 lwzux rTmp1, Ap,
      tdir4  //    do
          lhzux rTmp2,
      Bp,
      tdir2  //    {  A += RD->tdir;
          xor rTmp1,
      rTmp1,
      tbit  //       B += RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A += RD->tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @1  //    }Êwhile (*B == empty);

      add Ap,
      rAttack,
      dst4  //    A = &Attack[to];
          add Bp,
      rBoard,
      dst2  //    B = &Board[to];
      @2 lwzux rTmp1,
      Ap,
      tdir4  //    do
          lhzux rTmp2,
      Bp,
      tdir2  //    {  A += RD->tdir;
          xor rTmp1,
      rTmp1,
      tbit  //       B += RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A -= RD->tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @2  //    }Êwhile (*B == empty);

      lhzu tbit_,
      2(Data)neg tdir2, tdir2 neg tdir4, tdir4 cmpi cr6, 0, ti,
      0

      add Ap,
      rAttack,
      src4  //    A = &Attack[from];
          add Bp,
      rBoard,
      src2  //    B = &Board[from];
      @3 lwzux rTmp1,
      Ap,
      tdir4  //    do
          lhzux rTmp2,
      Bp,
      tdir2  //    {  A -= RD->tdir;
          xor rTmp1,
      rTmp1,
      tbit_  //       B -= RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A += RD->_tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @3  //    }Êwhile (*B == empty);

      add Ap,
      rAttack,
      dst4  //    A = &Attack[to];
          add Bp,
      rBoard,
      dst2  //    B = &Board[to];
      @4 lwzux rTmp1,
      Ap,
      tdir4  //    do
          lhzux rTmp2,
      Bp,
      tdir2  //    {  A -= RD->tdir;
          xor rTmp1,
      rTmp1,
      tbit_  //       B -= RD->tdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A -= RD->_tbit;
          stw rTmp1,
      0(Ap)beq + cr0,
      @4  //    }Êwhile (*B == empty);

          bgt +
          cr6,
      @transloop  // }

          /* - - - - - - - - - - Unmove Piece & Update attack along line of
             movement - - - - - - - - - - */

              lhzu mbit,
      2(Data)addi rTmp4, rPlayer, queen lhz mbit_,
      2(Data)  // % overwrites "Data" from now on...
      lwzx rTmp3,
      rAttack,
      src4  // Attack[from] -= _mbit;
          sthx rTmp4,
      rBoard,
      src2  // Board[m.from] = player + queen;
          xor rTmp3,
      rTmp3, mbit_ stwx rTmp3, rAttack,
      src4

          srawi rTmp1,
      src2,
      1  // SubBlockAttack(from);
      bl UpdBlockAttack lhz mcap,
      move(cap)

          add rTmp2,
      src2,
      src2  // sq = from + mdir;
          add dst4,
      dst2,
      dst2  //    % restore volatile regs
          or dbit,
      mbit, mbit_ add mdir4, mdir2, mdir2 b @N @L lwzx rTmp1, rAttack,
      rTmp2  // while (sq != to)
          xor rTmp1,
      rTmp1,
      dbit  // {
          stwx rTmp1,
      rAttack,
      rTmp2  //    Attack[sq] -= dbit;
      @N add rTmp2,
      rTmp2, mdir4 cmp cr0, 0, rTmp2,
      dst4  //    sq += mdir;
              bne +
          cr0,
      @L  // }

          cmpi cr6,
      0, mcap,
      empty  // % must set cr6 BEFORE calling
          srawi rTmp1,
      dst2, 1 bne cr6,
      @C                     // if (! m.cap)
          bl UpdBlockAttack  //    AddBlockAttack(to);
              lhz mcap,
      move(cap)  //    % reload volatile "mcap"
      add dst4,
      dst2,
      dst2      //    % restore volatile regs
          b @M  // else
      @C add Ap,
      rAttack,
      dst4  // {  A = &Attack[to];
          add mdir4,
      mdir2, mdir2 add Bp, rBoard,
      dst2  //    B = &Board[to];
      @F lwzux rTmp1,
      Ap,
      mdir4  //    do
          lhzux rTmp2,
      Bp,
      mdir2  //    {  A += mdir;
          xor rTmp1,
      rTmp1,
      mbit  //       B += mdir;
          cmpi cr0,
      0, rTmp2,
      empty  //       *A -= mbit;
          stw rTmp1,
      0(Ap)  //    }Êwhile (*B == empty);
          beq +
          cr0,
      @F  // }

      @M addi rTmp4,
      rEngine,
      ENGINE.B.PLinx  // Attack[to] += mbit;
          lhzx rTmp3,
      rTmp4, dst2 lwzx rTmp1, rAttack, dst4 sthx mcap, rBoard,
      dst2  // Board[m.to]Ê= m.cap;
          sthx rTmp3,
      rTmp4,
      src2  // PLinx[from] = PLinx[to];
          srawi rTmp2,
      src2, 1 add rTmp3, rTmp3, rTmp3 xor rTmp1, rTmp1, mbit sthx rTmp2,
      rPieceLoc,
      rTmp3  // PieceLocP[PLinx[from]] = from;
          stwx rTmp1,
      rAttack,
      dst4

              /*- - - - - - - - - - - - - Replace any captured piece and its
                 attack - - - - - - - - - - - - -*/

                  beq -
          cr6,
      @RETURN  // if (m.cap)
          lhz rTmp4,
      node(capInx)  // {
      addi rTmp6,
      rGlobal,
      GLOBAL.B.PieceCountBit  //    pieceCount += PieceCountBit[m.cap];
          addi rTmp2,
      rEngine, ENGINE.B.PLinx rlwinm rTmp7, mcap, 2, 0, 31 sthx rTmp4, rTmp2,
      dst2  //    PLInx[m.to] = N->capInx;
          lwzx rTmp8,
      rTmp6, rTmp7 srawi rTmp1, dst2, 1 add rTmp9, rTmp4, rTmp4 add rPieceCount,
      rTmp8,
      rPieceCount  //    PieceLoc_[N->capInx] = m.to;
          sthx rTmp1,
      rPieceLoc_,
      rTmp9 bl UpdPieceAttack  //    AddPieceAttack(m.to);
                               // }
      @RETURN frame_end5 blr

#undef src2
#undef dst2
#undef mdir2
#undef Data
#undef mbit
#undef mbit_

#undef ti
#undef mcap
#undef src4
#undef dst4
#undef tdir2
#undef tdir4
#undef tbit
#undef tbit_
#undef mdir4
#undef dbit
#undef Ap
#undef Bp
} /* RetractQueenMove */

/**************************************************************************************************/
/*                                                                                                */
/*                                           KING MOVES */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Perform King Moves
 * --------------------------------------*/

asm void PerformKingMove(void) {
#define src rLocal1
#define dst rLocal2
#define mcap rLocal3
#define mobSum rLocal4
#define mtype rTmp10

  lhz mtype,
      move(type) frame_begin4_

          lhz mcap,
      move(cap) lhz dst, move(to) cmpi cr5, 0, mtype, mtype_Normal lhz src,
      move(from) lha mobSum,
      node(mobEval)  // mobSum = N->mobEval;

          /*- - - - - - - - - - - - - - - - - - Perform Castling - - - - - - - -
             - - - - - - - - - - - - */

          beq +
          cr5,
      @cap  // if (castling(m))
          addi rTmp2,
      rPlayer,
      rook  // {
          cmpi cr6,
      0, mtype, mtype_O_O sth rTmp2,
      move(piece)  //    "Convert N->m to rook move"
          bne -
          cr6,
      @q addi rTmp2, dst, 1 @k li rTmp4, -1 b @mrook @q addi rTmp2, dst,
      -2 li rTmp4, 1 @mrook add rTmp3, rTmp4, dst sth rTmp2,
      move(from) sth rTmp3, move(to) sth rTmp4,
      move(dir) bl CastleRook  //    CastleRook();
          lha mobSum,
      nnode(mobEval)  //    mobSum = NN->mobVal; // Just computed by
                      //    PerformRookMove
      sth src,
      move(from) addi rTmp2, rPlayer,
      king  //    "Restore N->m to king move"
          sth dst,
      move(to) sth rTmp2,
      move(piece)  // }

      /*- - - - - - - - - - - - - Remove any captured piece and its attack - - -
         - - - - - - - - - - */

      @cap check_remove_cap

      /*- - - - - - - - - - - - - - - - - Move King on the board - - - - - - - -
         - - - - - - - - - - */
      @move addi rTmp4,
      rEngine,
      ENGINE.B.HasMovedTo  // HasMovedTo[to]++;
          add rTmp3,
      dst, dst add rTmp2, src, src lhzx rTmp5, rTmp4, rTmp3 sth dst,
      0(rPieceLoc)  // PieceLoc[0] = to;
      li rTmp8,
      empty  // Board[from] = empty;
          addi rTmp9,
      rPlayer, king sthx rTmp8, rBoard,
      rTmp2  // Board[to] = king + player;
          sthx rTmp9,
      rBoard, rTmp3 addi rTmp5, rTmp5, 1 mr rTmp1, src sthx rTmp5, rTmp4,
      rTmp3 bl UpdBlockAttack  // mobSum += AddBlockAttack(from);
          add mobSum,
      rTmp1,
      mobSum

          /*- - - - - - - - - - - - - - - - - - "Move" King Attack - - - - - - -
             - - - - - - - - - - - - */

              rlwinm rTmp1,
      src, 2, 0, 31 rlwinm rTmp2, dst, 2, 0, 31 add rTmp1, rAttack,
      rTmp1 add rTmp2, rAttack,
      rTmp2 upd_king_attack(rTmp1) upd_king_attack(rTmp2)

          sth mobSum,
      nnode(mobEval)  // NN->mobEval = mobSum;
      frame_end4 blr

#undef src
#undef dst
#undef mcap
#undef mtype
#undef mobSum
} /* PerformKingMove */

/*-------------------------------------- Retract King Moves
 * --------------------------------------*/

asm void RetractKingMove(void) {
#define src rLocal1
#define dst rLocal2
#define mcap rLocal3
#define mtype rLocal3

  frame_begin3_

      lhz dst,
      move(to) lhz src, move(from) lhz mcap,
      move(cap)

      /*- - - - - - - - - - - - - - - - - - "Unmove" King Attack - - - - - - - -
         - - - - - - - - - - */

      rlwinm rTmp1,
      dst, 2, 0, 31 rlwinm rTmp2, src, 2, 0, 31 add rTmp1, rAttack,
      rTmp1 add rTmp2, rAttack,
      rTmp2 upd_king_attack(rTmp1) upd_king_attack(rTmp2)

      /*- - - - - - - - - - - - - - - - - Unmove King on the board - - - - - - -
         - - - - - - - - - - */

      mr rTmp1,
      src  // SubBlockAttack(from);
          bl UpdBlockAttack addi rTmp4,
      rEngine,
      ENGINE.B.HasMovedTo  // HasMovedTo[to]--;
          add rTmp3,
      dst, dst add rTmp2, src, src lhzx rTmp5, rTmp4, rTmp3 sth src,
      0(rPieceLoc)  // PieceLoc[0] = from;
      addi rTmp9,
      rPlayer,
      king  // Board[from] = king + player;
          addi rTmp5,
      rTmp5, -1 sthx rTmp9, rBoard, rTmp2 sthx rTmp5, rTmp4,
      rTmp3

          /*- - - - - - - - - - - - - Replace any captured piece and its attack
             - - - - - - - - - - - - -*/

              check_replace_cap

                  /*- - - - - - - - - - - - - - - - - - Retract Castling - - - -
                     - - - - - - - - - - - - - - - - */

                      lhz mtype,
      move(type) cmpi cr5, 0, mtype, mtype_Normal beq + cr5,
      @RETURN  // if (castling(m))
          addi rTmp2,
      rPlayer,
      rook  // {
          cmpi cr6,
      0, mtype, mtype_O_O sth rTmp2,
      move(piece)  //    "Convert N->m to rook move"
          bne -
          cr6,
      @q addi rTmp2, dst, 1 @k li rTmp4, -1 b @mrook @q addi rTmp2, dst,
      -2 li rTmp4, 1 @mrook add rTmp3, rTmp4, dst sth rTmp2,
      move(from) sth rTmp3, move(to) sth rTmp4,
      move(dir) bl UncastleRook  //    UncastleRook();
          sth src,
      move(from) addi rTmp2, rPlayer,
      king  //    "Restore N->m to king move"
          sth dst,
      move(to) sth rTmp2,
      move(piece)  // }

      @RETURN frame_end3 blr

#undef src
#undef dst
#undef mcap
#undef mtype
} /* RetractKingMove */

/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

void InitPerformMoveModule(GLOBAL *Global) {
  SQUARE CQDir[8] = {0x1, 0x11, 0x10, 0xF, -0x1, -0x11, -0x10, -0xF};
  SQUARE mdir, tdir;

  for (INT i = 0; i <= 7; i++) {
    //--- Initialize "RBUpdData[]" ---

    mdir = Global->B.QueenDir[i];
    RBDATA *RB = &(Global->P.RBUpdData[mdir]);

    tdir = Global->B.Turn90[mdir];
    RB->tdir = 2 * tdir;
    RB->tbit = Global->A.DirBit[tdir] & rbMask;
    RB->tbit_ = Global->A.DirBit[-tdir] & rbMask;

    RB->mbit = Global->A.DirBit[mdir] & rbMask;
    RB->mbit_ = Global->A.DirBit[-mdir] & rbMask;

    RB->dm = (i <= 3 ? bishopMob : rookMob);

    RB->unused1 = 0;
    RB->unused2 = 0;

    //--- Initialize "QUpdData[]" ---

    mdir = CQDir[i];
    QDATA *Q = &(Global->P.QUpdData[mdir]);

    tdir = CQDir[(i + 1) % 8];
    Q->tdir0 = 2 * tdir;
    Q->tbit0 = Global->A.DirBit[tdir] & qMask;
    Q->tbit0_ = Global->A.DirBit[-tdir] & qMask;
    tdir = CQDir[(i + 2) % 8];
    Q->tdir1 = 2 * tdir;
    Q->tbit1 = Global->A.DirBit[tdir] & qMask;
    Q->tbit1_ = Global->A.DirBit[-tdir] & qMask;
    tdir = CQDir[(i + 3) % 8];
    Q->tdir2 = 2 * tdir;
    Q->tbit2 = Global->A.DirBit[tdir] & qMask;
    Q->tbit2_ = Global->A.DirBit[-tdir] & qMask;

    Q->mbit = Global->A.DirBit[mdir] & qMask;
    Q->mbit_ = Global->A.DirBit[-mdir] & qMask;

    for (INT j = 0; j < 5; j++) Q->padding[j] = 0;
  }
} /* InitPerformMoveModule */
