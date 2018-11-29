/**************************************************************************************************/
/*                                                                                                */
/* Module  : SELECTION.C */
/* Purpose : This module implements various selection routines used during the
 * search.            */
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

#include "Selection.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                 COMPUTING SELECTIVE BASE VALUE */
/*                                                                                                */
/**************************************************************************************************/

asm void ComputeSelBaseVal(void) {
  @ENDGAME cmpi cr5, 0, rPlayer,
      white  // if ("opponent has no officers and player has pawns")
          bne cr5,
      @B @W andis.rTmp0, rPieceCount,
      0xFFF0  //    N->selMargin = maxVal; // Turn off selection
      lbz rTmp2,
      (ENGINE.B.PawnStructW + 6)(rEngine)bne + @PAWNS_7TH andi.rTmp0,
      rPieceCount, 0x000F beq - @PAWNS_7TH @M li rTmp1, maxVal sth rTmp1,
      node(selMargin) blr @B andi.rTmp0, rPieceCount, 0xFFF0 lbz rTmp2,
      (ENGINE.B.PawnStructB + 1)(rEngine)bne + @PAWNS_7TH andis.rTmp0,
      rPieceCount,
      0x000F bne + @M

          @PAWNS_7TH  // else if ("player has pawns on 7th")
              cmpi cr6,
      0, rTmp2,
      0  //    N->selMargin = maxVal;
          bne -
          cr6,
      @M

      @SEL  // else
          lhz rTmp1,
      node(ply)  // {
      lha rTmp3,
      pnode(escapeSq)  //    N->selMargin = 4*(N->ply - 1);
      lhz rTmp4,
      pmove(from) addi rTmp1, rTmp1, -1 rlwinm rTmp1, rTmp1, 2, 0, 31 cmpi cr5,
      0, rTmp3,
      0  //    if (PN->escapeSq != nullSq &&
      cmp cr6,
      0, rTmp3, rTmp4 blt + cr5,
      @Store  //        PN->m.from != PN->escapeSq &&
              beq -
          cr6,
      @Store rlwinm rTmp5, rTmp3, 2, 0,
      29  //        Attack[PN->escapeSq] != 0)
      lwzx rTmp6,
      rAttack, rTmp5 cmpi cr7, 0, rTmp6, 0 beq - cr7, @Store lha rTmp7,
      pnode(threatEval)  //       selMargin += PN->threatEval;
      add rTmp1,
      rTmp1, rTmp7 @Store sth rTmp1, node(selMargin) blr  // }
} /* ComputeSelBaseVal */

/**************************************************************************************************/
/*                                                                                                */
/*                                        FORWARD PRUNE MOVES */
/*                                                                                                */
/**************************************************************************************************/

static asm BOOL Turbulent(register INT mval);

asm BOOL SelectMove(void) {
#define diff rTmp10

  //--- Never prune special moves ---

  lhz rTmp2,
      move(type)  // if (N->m.type || N->gen == gen_J)
      lhz rTmp3,
      node(gen) li rTmp1,
      true  //    return true;
      cmpi cr5,
      0, rTmp2, mtype_Normal cmpi cr6, 0, rTmp3,
      gen_J bnelr - cr5 beqlr -
          cr6

              //--- Calc evaluation difference ---
              // diff = -NN->totalEval + N->selMargin - N->alpha. If this is
              // positive, the move should immediately be searched. If it's
              // negative we need to check if the move is turbulent enough to
              // outweigh the poor positional/material score.

                  lha rTmp1,
      nnode(totalEval)  // diff = -NN->totalEval + N->selMargin - N->alpha;
      lha rTmp2,
      node(selMargin) lha rTmp3, node(alpha) lhz rTmp4, move(cap) lhz rTmp5,
      move(piece) subf diff, rTmp1, rTmp2 subf diff, rTmp3, diff cmpi cr5, 0,
      rTmp4,
      empty  // if (m.cap)
              beq -
          cr5,
      @CheckPawn  //    diff += 40 - N->capSelVal;
          lha rTmp6,
      node(capSelVal) addi diff, diff, 40 subf diff, rTmp6,
      diff @CheckPawn andi.rTmp5, rTmp5,
      0x07  // if (pieceType(m.piece) == pawn)
      cmpi cr6,
      0, rTmp5,
      pawn  //    diff += 35;
              bne +
          cr6,
      @CheckDiff addi diff, diff,
      35

      @CheckDiff cmpi cr7,
      0, diff,
      0  // if (diff > 0) return true;
      li rTmp1,
      true bgtlr + cr7 neg rTmp1,
      diff  // return Turbulent(-diff);
          b Turbulent

#undef diff
} /* SelectMove */

/**************************************************************************************************/
/*                                                                                                */
/*                                         TURBULENCE CHECKING */
/*                                                                                                */
/**************************************************************************************************/

#define tp rTmp10  // <--- global in subsequent routines

static asm BOOL DirectThreat(void);
// static asm BOOL DirectThreatS (void);
static asm BOOL IndirectThreat(register ATTACK qrbAtt);

// The main turbulence check routine "Turbulent(mval)" checks if the current
// move poses a threat that is a least worth "mval" points. This is done by
// converting mval to a piece, i.e. the piece that must at least be threatened
// in order for the move to be considered turbulent enough.

static asm BOOL Turbulent(register INT mval) {
  // #define ngen rTmp3

  frame_begin0

      cmpi cr5,
      0, mval,
      350  // if (mval < 350)
           // lhz     ngen, node(gen)
      bgt cr5,
      @KQR cmpi cr6, 0, mval,
      150  //    if (mval < 150)
      li tp,
      pawn + black  //       tp = enemy(pawn);
                 blt cr6,
      @Ole  //    else
          li tp,
      knight + black  //       tp = enemy(knight);
                   b @Ole @KQR cmpi cr7,
      0, mval,
      550  // else if (mval < 550)
      li tp,
      rook + black  //    tp = enemy(rook);
                 blt cr7,
      @Ole cmpi cr6, 0, mval,
      950  // else if (mval < 950)
      li tp,
      queen + black  //    tp = enemy(queen);
                  blt cr6,
      @Ole  // else
          li tp,
      king + black  //    tp = enemy(king);
          @Ole subf tp,
      rPlayer,
      tp  // if (DirectThreat(tp)) return true;
          bl DirectThreat cmpi cr6,
      0, rTmp1, 0 bne cr6,
      @RETURN
      /*
         cmpi    cr5,0, ngen,gen_I                   // if (N->gen != gen_I)
         subf    tp, rPlayer,tp                      // {
         beq     cr5, @Sacri                         //    if (DirectThreat(tp))
      return true; bl      DirectThreat cmpi    cr6,0, rTmp1,0 bne     cr6,
      @RETURN                        // } b       @Indirect // else
      @Sacri                                         // {
         bl      DirectThreatS                       //    if
      (DirectThreatS(tp)) return true; cmpi    cr6,0, rTmp1,0 bne     cr6,
      @RETURN                        // }
      */
      @Indirect lhz rTmp2,
      move(from)  // a = Attack[m->from] & qrbMask;
      frame_end0 rlwinm rTmp2,
      rTmp2, 2, 0, 29 lwzx rTmp1, rAttack,
      rTmp2  // return (a && IndirectThreat(tp, a));
          andi.rTmp1,
      rTmp1,
      0xFFFF beqlr b IndirectThreat

      @RETURN frame_end0 blr

  // #undef ngen
} /* Turbulent */

/*----------------------------------------- Direct Threats
 * ---------------------------------------*/
// On entry: rTmp10 = tp holds the minimum piece that must be threatened

static asm BOOL DirectThreat(void) {
#define dtN(dir, nxt)                                                     \
  lhz p, (2 * dir)(Bto);     /* p = B[dir];                            */ \
  cmp cr5, 0, p, tp;         /* if (p >= tp &&                         */ \
  blt cr5, nxt;              /*                                        */ \
  cmp cr6, 0, p, rTmp1;      /*     p <= enemy(king) &&                */ \
  bgt cr6, nxt;              /*                                        */ \
  lwz rTmp0, (4 * dir)(Ato); /*     (p >= enemy(rook) || ! A[dir]))    */ \
  cmp cr7, 0, p, rTmp2;      /*                                        */ \
  bgelr cr7;                 /*    return true;                        */ \
  cmpi cr5, 0, rTmp0, 0;     /*                                        */ \
  beqlr cr5;                 /*                                        */ \
  nxt

#define dtK(dir, nxt)                                                     \
  lhz p, (2 * dir)(Bto);     /* p = B[dir];                            */ \
  cmp cr5, 0, p, tp;         /* if (p >= tp &&                         */ \
  blt cr5, nxt;              /*                                        */ \
  cmp cr6, 0, p, rTmp1;      /*     p <= enemy(rook) &&                */ \
  lwz rTmp0, (4 * dir)(Ato); /*     ! A[dir])                          */ \
  bgt cr6, nxt;              /*                                        */ \
  cmpi cr5, 0, rTmp0, 0;     /*    return true;                        */ \
  beqlr cr5;                 /*                                        */ \
  nxt

#define dtQRB(sc, nxt)                                                  \
  mr Bsq, Bto;             /* sq = m.to;                             */ \
  sc lhzux p, Bsq, tdir;   /* while (isEmpty(sq += tdir));           */ \
  cmpi cr5, 0, p, empty;   /*                                        */ \
  beq + cr5, sc;           /* p = B[sq];                             */ \
  cmp cr5, 0, p, tp;       /* if (p >= tp &&                         */ \
  blt cr5, nxt;            /*                                        */ \
  cmp cr6, 0, p, rTmp1;    /*     p <= enemy(king) &&                */ \
  bgt cr6, nxt;            /*                                        */ \
  cmp cr7, 0, p, rTmp2;    /*     (p >= enemy(P) || ! A[dir]))       */ \
  bgelr cr7;               /*                                        */ \
  subf rTmp0, Bto, Bsq;    /*                                        */ \
  add rTmp0, rTmp0, rTmp0; /*                                        */ \
  lwzx rTmp0, Ato, rTmp0;  /*                                        */ \
  cmpi cr5, 0, rTmp0, 0;   /*                                        */ \
  beqlr cr5;               /*    return true;                        */ \
  nxt

#define mpiece rTmp9
#define p rTmp9
#define p1 rTmp9
#define p2 rTmp6
#define Ato rTmp8
#define Bto rTmp7
#define Bsq rTmp4
#define tdir rTmp6
#define QDir rTmp3
#define mto rTmp5
#define ksq rTmp4
#define mdir rTmp4

   lhz     mto, move(to)                       // if (Closeness[m->to - kingSq_] >= 7)
   lhz     ksq, 0(rPieceLoc_)
   addi    rTmp2, rGlobal,GLOBAL.V.Closeness   //    return true;
   lhz     mpiece, move(piece)
   subf    rTmp1, ksq,mto
   add     rTmp1, rTmp1,rTmp1
   lhzx    rTmp3, rTmp2,rTmp1
   cmpi    cr0,0, rTmp3,7
   bgelr-

   add     rTmp1, mto,mto                      // B = &Board[m->to];
   rlwinm  rTmp2, mto,2,0,29
   add     Bto, rBoard,rTmp1                   // A = &Attack_[m->to];
   andi.   mpiece, mpiece,0x07
   add     Ato, rAttack_,rTmp2

   cmpi    cr5,0, mpiece,bishop                // switch (pieceType(p))
   bgt-    cr5, @KQR                           // {
   beq     cr5, @BISHOP
   cmpi    cr6,0, mpiece,pawn
   beq+    cr6, @PAWN
   b       @KNIGHT
@KQR
   cmpi    cr6,0, mpiece,queen
   blt+    cr6, @ROOK
   beq+    cr6, @QUEEN
   b       @KING

   /*- - - - - - - - - - - - - - - - - - - - - Pawn - - - - - - - - - - - - - - - - - - - - - */

@PAWN                                          // case pawn :
   subf    rTmp1, rPawnDir,mto                 //    if (rank(m->to) in {6,7}) return true;
   rlwinm  rTmp2, rTmp1,2,27,27
   cmp     cr5,0, rTmp2,rPlayer
   bnelr-  cr5

   add     rTmp1, rPawnDir,rPawnDir            //    B += pawnDir;
   add     Bto, Bto,rTmp1
   add     rTmp2, rTmp1,rTmp1                  //    A += pawnDir;
   lhz     p1, -2(Bto)                         //    p1 = B[-1];
   lhz     p2, +2(Bto)                         //    p2 = B[+1];
   add     Ato, Ato,rTmp2

   li      rTmp1, king + black                 //    rTmp1 <-- enemy(king)
   subf    rTmp1, rPlayer,rTmp1                //    rTmp2 <-- enemy(knight)
   addi    rTmp2, rTmp1,-4

@Left
   cmp     cr5,0, p1,tp                        //    if (p1 >= tp &&
   blt     cr5, @Right
   cmp     cr6,0, p1,rTmp1                     //        p1 <= enemy(king) &&
   bgt     cr6, @Right
   lwz     rTmp3, -4(Ato)                      //        (p1 >= enemy(knight || ! A[-1]))
   cmp     cr7,0, p1,rTmp2
   bgelr+  cr7
   cmpi    cr5,0, rTmp3,0                      //       return true;
   beqlr   cr5
@Right
   cmp     cr5,0, p2,tp                        //    if (p2 >= tp &&
   blt     cr5, @NTHREAT
   cmp     cr6,0, p2,rTmp1                     //        p2 <= enemy(king) &&
   bgt     cr6, @NTHREAT
   lwz     rTmp3, +4(Ato)                      //        (p2 >= enemy(knight || ! A[+1]))
   cmp     cr7,0, p2,rTmp2
   bgelr+  cr7
   cmpi    cr5,0, rTmp3,0                      //       return true;
   beqlr   cr5
   b       @NTHREAT                            //    break;

   /* - - - - - - - - - - - - - - - - - - - - Knight - - - - - - - - - - - - - - - - - - - - -*/

@KNIGHT                                        // case knight :
   li      rTmp1, king + black                 //    rTmp1 <-- enemy(king)
   subf    rTmp1, rPlayer,rTmp1                //    rTmp2 <-- enemy(rook)
   addi    rTmp2, rTmp1,-2
   dtN(-0x0E, @N1)
   dtN(-0x12, @N2)
   dtN(-0x1F, @N3)
   dtN(-0x21, @N4)
   dtN(+0x12, @N5)
   dtN(+0x0E, @N6)
   dtN(+0x21, @N7)
   dtN(+0x1F, @N8)
   b       @NTHREAT                            //    break;

   /* - - - - - - - - - - - - - - - - - - - - Bishop - - - - - - - - - - - - - - - - - - - - -*/

@BISHOP                                        // case bishop :
   lhz     rTmp3, 2(Ato)
   li      rTmp1, queen + black                //    rTmp1 <-- enemy(queen)
   subf    rTmp1, rPlayer,rTmp1
   cmp     cr5,0, tp,rTmp1                     //    if (tp <= enemy(queen) &&
   bgt     cr5, @B0                            //        (Attack_[dest] & 0x0F))
   andi.   rTmp3, rTmp3,0x0F                   //       return true;
   bnelr
@B0
   lha     tdir, move(dir)
   lhz     rTmp3, move(cap)
   addi    rTmp1, rTmp1,1                      //    rTmp1 <-- enemy(king)
   addi    rTmp2, rTmp1,-2                     //    rTmp2 <-- enemy(rook)
   cmpi    cr5,0, rTmp3,empty
   add     tdir, tdir,tdir                     //    if (m->cap)
   beq-    cr5, @B1                            //       scanQRB(m->dir, enemy(rook));
   dtQRB(@B1L, @B1)
   addi    rTmp3, rGlobal,GLOBAL.B.Turn90      //    dir = Turn90[m->dir];
   lhax    tdir, rTmp3,tdir
   add     tdir, tdir,tdir
   dtQRB(@B2L, @B2)                            //    scanQRB(dir, enemy(rook));
   neg     tdir, tdir
   dtQRB(@B3L, @B3)                            //    scanQRB(-dir, enemy(rook));
   b       @NTHREAT                            //    break;

   /*- - - - - - - - - - - - - - - - - - - - - Rook - - - - - - - - - - - - - - - - - - - - - */

@ROOK                                          // case rook :
   lhz     rTmp3, 2(Ato)
   li      rTmp1, queen + black                //    rTmp1 <-- enemy(queen)
   subf    rTmp1, rPlayer,rTmp1
   cmp     cr5,0, tp,rTmp1                     //    if (tp <= enemy(queen) &&
   bgt     cr5, @R0                            //        (Attack_[dest] & 0xF0))
   andi.   rTmp3, rTmp3,0xF0                   //       return true;
   bnelr
@R0
   lha     tdir, move(dir)
   lhz     rTmp3, move(cap)
   mr      rTmp2, rTmp1                        //    rTmp1 <-- enemy(king)
   addi    rTmp1, rTmp1,1                      //    rTmp2 <-- enemy(queen)
   cmpi    cr5,0, rTmp3,empty
   add     tdir, tdir,tdir                     //    if (m->cap)
   beq     cr5, @R1                            //       scanQRB(m->dir, enemy(queen));
   dtQRB(@R1L, @R1)
   addi    rTmp3, rGlobal,GLOBAL.B.Turn90      //    dir = Turn90[m->dir];
   lhax    tdir, rTmp3,tdir
   add     tdir, tdir,tdir
   dtQRB(@R2L, @R2)                            //    scanQRB(dir, enemy(queen));
   neg     tdir, tdir
   dtQRB(@R3L, @R3)                            //    scanQRB(-dir, enemy(queen));
   b       @NTHREAT                            //    break;

   /* - - - - - - - - - - - - - - - - - - - - Queen - - - - - - - - - - - - - - - - - - - - - */

@QUEEN                                         // case queen :
   li      rTmp1, king + black                 //    rTmp1 = rTmp2 <-- enemy(king)
   addi    QDir, rGlobal,(GLOBAL.B.QueenDir-2) //    for (INT i = 0; i <= 7; i++)
   subf    rTmp1, rPlayer,rTmp1
   lha     mdir, move(dir)                     //    {
   mr      rTmp2, rTmp1
   b       @rof                                //       dir = QueenDir[i];
@for    
   cmp     cr6,0, tdir,mdir                    //       if ((dir != m->dir || m->cap) &&
   bne     cr6, @neg
   lhz     rTmp0, move(cap)
   cmpi    cr5,0, rTmp0,empty                  //           dir != -m->dir)
   beq     cr5, @rof
   b       @sc
@neg
   add.    rTmp0, mdir,tdir
   beq     @rof
@sc
   add     tdir, tdir,tdir                     //          scanQRB(dir, enemy(king));
   dtQRB(@QL, @Q)
@rof
   lhau    tdir, 2(QDir)
   cmpi    cr5,0, tdir,0
   bne+    cr5, @for                           //    }

   b       @NTHREAT                            //    break;

   /*- - - - - - - - - - - - - - - - - - - - - King - - - - - - - - - - - - - - - - - - - - - */

@KING                                          // case king :
   li      rTmp1, rook + black                 //    rTmp1 <-- enemy(rook)
   subf    rTmp1, rPlayer,rTmp1
   cmp     cr5,0, tp,rTmp1                     //    if (tp > enemy(rook)) return false;
   bgt     cr5, @NTHREAT
   dtK(-0x0F, @K1)
   dtK(-0x11, @K2)
   dtK(+0x11, @K3)
   dtK(+0x0F, @K4)
   dtK(-0x10, @K5)
   dtK(+0x10, @K6)
   dtK(+0x01, @K7)
   dtK(-0x01, @K8)                             // }

@NTHREAT
   li      rTmp1, false                        // return false;
   blr

#undef mpiece
#undef p
#undef p1
#undef p2
#undef Ato
#undef Bto
#undef Bsq
#undef tdir
#undef QDir
#undef mto
#undef ksq
#undef mdir

#undef dtN
#undef dtK
#undef dtQRB
} /* DirectThreat */

/*------------------------------------ Direct Sacrifice Threats
 * ----------------------------------*/

#ifdef kartoffel

static asm BOOL DirectThreatS(void) {
  b DirectThreat  //### !!!!!!

      li rTmp1,
      false blr
} /* DirectThreatS */

#endif

/*--------------------------------------- Indirect Threats
 * ---------------------------------------*/
// Checks if "N->m" poses an indirect threat. On entry:
//    rTmp1(qrbAtt) = Attack[m->from] & qrbMask
//    rTmp10(tp)    = Minimum target piece

static asm BOOL IndirectThreat(register ATTACK qrbAtt) {
#define qrb rTmp9
#define abits rTmp7
#define Data rTmp8
#define adir rTmp4
#define mdir rTmp6
#define mfrom rTmp5
#define ip rTmp7
#define rbit rTmp4

  srawi rTmp2, qrbAtt,
      8  // abits = (qrbAtt | (qrbAtt>>8)) & 0x00FF;
      lhz mfrom,
      move(from) or rTmp2, qrbAtt, rTmp2 lha mdir, move(dir) rlwinm.abits,
      rTmp2, 4, 20, 27 addi Data, rGlobal, GLOBAL.M.QRBdata add mfrom, mfrom,
      mfrom mr qrb,
      qrbAtt

      @FOR  // for (bits = abits; bits; clrBit(j,bits))
          lhaux adir,
      Data,
      abits  // {
          add rTmp2,
      rBoard, mfrom cmp cr7, 0, adir,
      mdir  //    adir = QueenDir[j = LowBit[bits]];
          add.rTmp0,
      adir, mdir beq - cr7,
      @ROF  //    if (adir == m->dir || adir == -m->dir)
              beq -
          @ROF  //       continue;
              add rTmp1,
      adir,
      adir  //    sq = m.from;
      @L lhzux ip,
      rTmp2,
      rTmp1  //    while (Board[sq + dir] == empty) sq += dir;
          cmpi cr5,
      0, ip, empty beq + cr5,
      @L  //    ip = Board[sq];

          cmp cr6,
      0, ip,
      tp  //    if (ip < tp) continue;
          add rTmp1,
      ip, rPlayer blt cr6,
      @ROF  //    if (ip > enemy(king)) continue;
          cmpi cr7,
      0, rTmp1, king + black bgt cr7,
      @ROF  //    if (ip == enemy(king)) return true;
              beqlr -
          cr7

              subf rTmp2,
      rBoard,
      rTmp2  //    if (! Attack_[sq]) return true;
          add rTmp2,
      rTmp2, rTmp2 lwzx rTmp0, rAttack_, rTmp2 andi.ip, ip,
      0x07  //    switch (pieceType(Board[sq]))
      cmpi cr6,
      0, ip,
      rook  //    {
          cmpi cr5,
      0, rTmp0,
      0 beqlr cr5

              blt +
          cr6,
      @ROF lwz rbit, QRB_DATA.rayBit(Data) beq + cr6, @R @Q andi.rTmp1, qrb,
      0xFF00  //       case queen:
          and.rTmp1,
      rTmp1,
      rbit              //          if (a & RayBit[j] & rbMask) return true;
          bnelr b @ROF  //          break;
      @R andi.rTmp1,
      qrb,
      0x0F00  //       case rook:
          and.rTmp1,
      rTmp1,
      rbit       //          if (a & RayBit[j] & bMask) return true;
          bnelr  //    }

      @ROF lha abits,
      QRB_DATA.offset(Data) cmpi cr5, 0, abits,
      0  //    clrBit(j,bits);
          bne -
          cr5,
      @FOR  // }

          li rTmp1,
      false  // return false;
      blr

#undef qrb
#undef abits
#undef Data
#undef adir
#undef mdir
#undef mfrom
#undef ip
#undef rbit
} /* IndirectThreat */

/*------------------------------------------------------------------------------------------------*/

#undef tp
