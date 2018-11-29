/**************************************************************************************************/
/*                                                                                                */
/* Module  : THREATS.C */
/* Purpose : This module implements the threat handling. */
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

#include "Threats.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                       THREAT ANALYSIS */
/*                                                                                                */
/**************************************************************************************************/

static asm void AnalyzeThreats0(void);
static asm void AnalyzeThreats1(void);
static asm void AnalyzeThreats2(void);

asm void AnalyzeThreats(void) {
  lhz rTmp1,
      node(quies)  // if (! N->quies)
      lhz rTmp2,
      node(check) lhz rTmp3, node(maxPly) cmpi cr5, 0, rTmp1,
      true  //    if (! N->check)
          beq +
          cr5,
      @q cmpi cr6, 0, rTmp2,
      true  //       AnalyzeThreats0();
          bne +
          cr6,
      AnalyzeThreats0 li rTmp4,
      maxVal  //    else
          li rTmp5,
      nullSq  //    {
          sth rTmp4,
      node(threatEval)  //       N->threatEval = maxVal;
      sth rTmp5,
      node(escapeSq)  //       N->escapeSq   = nullSq;
      blr             //    }
      @q cmpi cr7,
      0, rTmp3,
      0  // else if (N->maxPly > 0)
          bgt +
          cr7,
      AnalyzeThreats1        //    AnalyzeThreats1();
          b AnalyzeThreats2  // else
} /* AnalyzeThreats */       //    AnalyzeThreats2();

/*--------------------------------------- Full Width Analysis
 * ------------------------------------*/
// Analyzes threats at a full width node. The main part of the analysis is
// threats of higher valued or undefended pieces. Additionally, opponent has
// pawns on his 6th or 7th rank are also considered as threats. On exit: ¥
// N->escapeSq   : The location of the highest valued hung piece (if any). ¥
// N->eply       : Controls whether escape moves should be extended for this
// piece. = 0 if
//                   there is only one hung piece and it is attacked by a lower
//                   valued piece.
// ¥ N->threatEval : Contains a pessimistic threat evaluation.
// ¥ N->Aloc/SLoc  : Contains the locations of the remaining attacked resp. safe
// pieces (excl.
//                   the king).

static asm void AnalyzeThreats0(void) {
#define tsq rTmp10
#define tply rTmp9
#define i rTmp8
#define PL rTmp7
#define AL rTmp6
#define SL rTmp5
#define sq rTmp4
#define tp rTmp3

#define ksq rTmp10
#define Asq rTmp10
#define ka rTmp2

  lhz i, node(lastPiece) mr PL, rPieceLoc addi AL, rNode,
      (NODE.ALoc - 2) addi SL, rNode, (NODE.SLoc - 2) li tsq,
      nullSq  // N->escapeSq = nullSq;
          li tply,
      1            // N->eply = 1;
      b @ROF @FOR  // for (i = 1; i <= N->LastPiece; i++)
          lhau sq,
      2(PL)  // {
      cmpi cr5,
      0, sq,
      0  //    sq = N->PieceLoc[i];
          blt -
          cr5,
      @ROF  //    if (offBoard(sq)) continue;
          rlwinm rTmp2,
      sq, 2, 0,
      29  //    a = Attack_[sq];
      lwzx rTmp1,
      rAttack_,
      rTmp2  //    if (! a)
          cmpi cr6,
      0, rTmp1,
      0  //       *(SLoc++) = sq;
      beq cr6,
      @Safe  //    else if (N->escapeSq != nullSq)
          cmpi cr7,
      0, tsq,
      nullSq  //       *(ALoc++) = sq;
          bne cr7,
      @Unsafe add rTmp2, sq,
      sq  //    else if (a & SmattMask[Board[sq]])
          lhzx tp,
      rBoard, rTmp2 addi rTmp2, rGlobal,
      GLOBAL.A.SmattMask  //       N->escapeSq = sq,
          rlwinm tp,
      tp, 2, 0,
      29  //       N->eply = 0;
      lwzx rTmp2,
      rTmp2, tp and.rTmp0, rTmp1, rTmp2 beq + @U li tply, 0 @T mr tsq,
      sq b @ROF  //    else if (! Attack[sq])  // Att but no def
      @U rlwinm rTmp2,
      sq, 2, 0, 29 lwzx rTmp1, rAttack,
      rTmp2  //       N->escapeSq = sq;
          cmpi cr5,
      0, rTmp1, 0 beq cr5,
      @T  //    else
      @Unsafe sthu sq,
      2(AL)  //       *(ALoc++) = sq;
      b @ROF @Safe sthu sq,
      2(SL) @ROF addi i, i, -1 cmpi cr5, 0, i, 0 bge + cr5,
      @FOR  // }

          li rTmp1,
      nullSq  // *(ALoc) = nullSq;
          sth tply,
      node(eply) sth rTmp1,
      2(AL)  // *(SLoc) = nullSq;
      sth rTmp1,
      2(SL)sth tsq,
      node(escapeSq)

          cmpi cr5,
      0, tsq,
      nullSq  // if (N->escapeSq == nullSq)
          li rTmp1,
      0 beq + cr5,
      @FarPawnThreat  //    N->threatEval = 0;
      @THREAT         // else
          srawi tp,
      tp, 1 addi rTmp8, rGlobal,
      GLOBAL.B.Mtrl100  //    N->threatEval = 100*Mtrl[Board[N->escapeSq]];
          lhzx rTmp1,
      rTmp8,
      tp

      @FarPawnThreat srawi rTmp2,
      rPlayer, 2 addi rTmp6, rEngine, (ENGINE.B.PawnStructW + 9) neg rTmp2,
      rTmp2 lhzx rTmp5, rTmp6,
      rTmp2  // if (opponent has far pawns)
          cmpi cr6,
      0, rTmp5,
      0  //    N->threatEval += 900;
          beq +
          cr6,
      @MateThreat  //    // player = white -> Pb[1(+2)] = pw[9]
          addi rTmp1,
      rTmp1,
      900  //    // player = black -> Pw[5(+6)]

      @MateThreat lhz ksq,
      0(rPieceLoc)  // ATTACK ka = 0;
      rlwinm ksq,
      ksq, 2, 0,
      29  // for (INT i = 0; i < 8; i++)
      add Asq,
      rAttack_,
      ksq  //    ka |= Attack_[ksq + KingDir[i]];
          lwz rTmp2,
      (-0x11 * 4)(Asq)lwz rTmp3, (-0x10 * 4)(Asq)lwz rTmp4,
      (-0x0F * 4)(Asq)lwz rTmp5, (-0x01 * 4)(Asq)lwz rTmp6,
      (+0x01 * 4)(Asq)lwz rTmp7, (+0x0F * 4)(Asq)lwz rTmp8,
      (+0x10 * 4)(Asq)lwz rTmp9, (+0x11 * 4)(Asq) or ka, rTmp2, rTmp3 or ka, ka,
      rTmp4 or ka, ka, rTmp5 or ka, ka, rTmp6 or ka, ka, rTmp7 or ka, ka,
      rTmp8 or ka, ka, rTmp9 cntlzw rTmp10,
      ka  // if (BitCount(ka) > 1)
          rlwnm.ka,
      ka, rTmp10, 1,
      31  //    N->threatEval = maxVal;
          beq +
          @SthThreatEval li rTmp1,
      maxVal

      @SthThreatEval sth rTmp1,
      node(threatEval) blr

#undef tsq
#undef tply
#undef i
#undef PL
#undef AL
#undef SL
#undef sq
#undef tp

#undef ksq
#undef Asq
#undef ka
} /* AnalyzeThreats0 */

/*------------------------------- Quiescence Analysis incl. Escapes
 * ------------------------------*/
// Analyzes threats at a "shallow" quiescence node (where escapes may be
// searched). On exit: ¥ N->escapeSq  : The location of THE piece (if any) that
// is threatened by a smaller piece.
//                  If there is more than one hung piece, N->escapeSq is set to
//                  "nullSq".
// ¥ N->eply       : Controls whether escape moves should be extended for this
// piece. = 0 if
//                   there is only one hung piece and it is attacked by a lower
//                   valued piece.
// ¥ N->threatEval : Contains a pessimistic threat evaluation.

asm void AnalyzeThreats1(void) {
#define tsq rTmp10
#define tply rTmp9
#define tval rTmp8
#define i rTmp7
#define PL rTmp6
#define sq rTmp5

  lhz i, node(lastPiece) mr PL, rPieceLoc li tsq,
      nullSq  // N->escapeSq = nullSq;
          li tply,
      1  // N->eply = 1;
      li tval,
      0            // N->threatEval = 0;
      b @ROF @FOR  // for (i = 1; i <= N->LastPiece; i++)
          lhau sq,
      2(PL)  // {
      cmpi cr5,
      0, sq,
      0  //    sq = N->PieceLoc[i];
          blt -
          cr5,
      @ROF  //    if (offBoard(sq)) continue;
          rlwinm rTmp4,
      sq, 2, 0,
      29  //    a = Attack_[sq];
      lwzx rTmp1,
      rAttack_,
      rTmp4  //    if (! a) continue;
          cmpi cr6,
      0, rTmp1, 0 beq cr6,
      @ROF

          cmpi cr7,
      0, tval,
      0  //    if (N->threatEval == 0)
      bne cr7,
      @Anja  //    {
          add rTmp2,
      sq, sq lhzx rTmp3, rBoard,
      rTmp2  //       if (a &= SmattMask[Board[sq]])
          addi rTmp2,
      rGlobal,
      GLOBAL.A.SmattMask  //       {
          rlwinm rTmp3,
      rTmp3, 2, 0, 29 lwzx rTmp2, rTmp2, rTmp3 and.rTmp1, rTmp1,
      rTmp2 beq + @Eva

                      addi rTmp2,
      rGlobal,
      GLOBAL.B.Mtrl  //          N->threatEval = Mtrl[Board[sq]];
          srawi rTmp3,
      rTmp3, 1 lhzx tval, rTmp2,
      rTmp3

          andis.rTmp0,
      rTmp1,
      0x0600  //          N->threatEval -= SmattMtrl(a);
      li rTmp2,
      pawnMtrl bne @Calc andis.rTmp0, rTmp1, 0x00FF li rTmp2,
      knightMtrl bne @Calc andi.rTmp0, rTmp1, 0x0F00 bne @Calc li rTmp2,
      rookMtrl @Calc subf tval, rTmp2,
      tval  //          N->threatEval *= 100;
          li tply,
      0  //          N->eply = 0;
      mulli tval,
      tval, 100 sth sq,
      node(escapeSq)  //          N->escapeSq = sq;
      sth tply,
      node(eply) sth tval,
      node(threatEval)  //          return;
      blr               //       }
      @Eva lwzx rTmp1,
      rAttack,
      rTmp4  //       else if (! Attack[sq])
          cmpi cr6,
      0, rTmp1, 0 bne cr6,
      @ROF  //          N->threatEval = hungVal1;
          lha tval,
      node(hungVal1) b @ROF  //    }

      @Anja lwzx rTmp2,
      rAttack,
      rTmp4  //    else if (! Attack[sq] ||
          cmpi cr6,
      0, rTmp2, 0 beq cr6,
      @Ole

          add rTmp2,
      sq,
      sq  //             (a & SmattMask[Board[sq]]))
          lhzx rTmp3,
      rBoard, rTmp2 addi rTmp2, rGlobal,
      GLOBAL.A.SmattMask  //    {
          rlwinm rTmp3,
      rTmp3, 2, 0, 29 lwzx rTmp0, rTmp2, rTmp3 and.rTmp0, rTmp1,
      rTmp0 beq @ROF @Ole lha tval,
      node(hungVal2)  //       N->threatEval = N->hungVal2;
      li tply,
      1  //       N->eply = 1;
      li tsq,
      nullSq         //       N->escapeSq = nullSq;
          b @RETURN  //       return;
      @ROF           //    }
          addi i,
      i, -1 cmpi cr5, 0, i, 0 bge + cr5,
      @FOR  // }

      @RETURN sth tsq,
      node(escapeSq) sth tply, node(eply) sth tval, node(threatEval) blr

#undef tsq
#undef tply
#undef tval
#undef i
#undef PL
#undef sq
} /* AnalyzeThreats1 */

/*------------------------------- Quiescence Analysis excl. Escapes
 * ------------------------------*/
// Analyzes threats at a "deep" quiescence node (where no escapes are searched).
// On exit: ¥ N->threatEval : Contains a conservative estimate of any threats.

asm void AnalyzeThreats2(void) {
#define tsq rTmp10
#define tply rTmp9
#define tval rTmp8
#define i rTmp7
#define PL rTmp6
#define sq rTmp5

  lhz i, node(lastPiece) mr PL, rPieceLoc li tsq,
      nullSq  // N->escapeSq = nullSq;
          li tply,
      1  // N->eply = 1;
      li tval,
      0            // N->threatEval = 0;
      b @ROF @FOR  // for (i = 1; i <= N->LastPiece; i++)
          lhau sq,
      2(PL)  // {
      cmpi cr5,
      0, sq,
      0  //    sq = N->PieceLoc[i];
          blt -
          cr5,
      @ROF  //    if (offBoard(sq)) continue;
          rlwinm rTmp2,
      sq, 2, 0,
      29  //    a = Attack_[sq];
      lwzx rTmp1,
      rAttack_,
      rTmp2  //    if (! a) continue;
          cmpi cr6,
      0, rTmp1, 0 beq cr6, @ROF add rTmp2, sq,
      sq  //    if (a & SmattMask[Board[sq]] == 0) continue;
          lhzx rTmp3,
      rBoard, rTmp2 addi rTmp2, rGlobal, GLOBAL.A.SmattMask rlwinm rTmp3, rTmp3,
      2, 0, 29 lwzx rTmp2, rTmp2, rTmp3 and.rTmp0, rTmp1,
      rTmp2 beq + @ROF cmpi cr5, 0, tval,
      0       //    if (tval == 0)
      beq @1  //       tval = hungVal1;
      lha tval,
      node(hungVal2)  //    else
      b @RETURN       //    {  tval = hungVal2;
      @1 lha tval,
      node(hungVal1)  //       return;
      @ROF            //    }
          addi i,
      i, -1 cmpi cr5, 0, i, 0 bge + cr5,
      @FOR  // }

      @RETURN sth tsq,
      node(escapeSq) sth tply, node(eply) sth tval, node(threatEval) blr

#undef tsq
#undef tply
#undef tval
#undef i
#undef PL
#undef sq
} /* AnalyzeThreats2 */
