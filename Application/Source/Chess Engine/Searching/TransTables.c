/**************************************************************************************************/
/*                                                                                                */
/* Module  : TRANSTABLES.C                                                                        */
/* Purpose : This module implements the transposition tables.                                     */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include "TransTables.f"


/**************************************************************************************************/
/*                                                                                                */
/*                                ALLOCATE/RESET TRANSPOSITION TABLES                             */
/*                                                                                                */
/**************************************************************************************************/

// Allocates and initializes the transposition tables based on the amount of memory set aside for
// the specified engine (in the engine parameter block). Additionally, the transposition table
// pointers in the search nodes are initialized.

void CalcTransState (ENGINE *E)                  
{
   TRANS_STATE *T = &E->Tr;

   T->transSize = (1L << 13);                            // Minimum number of entries.
   ULONG free   = E->P.transSize/sizeof(TRANS);          // Total number of free entries.

   if (T->transSize > free)                              // A minimum of 4*2^11 = 8192 entries are required.
   {
      T->transTabOn    = false;

      T->transSize     = 0;
      T->transUsed     = 0;
      T->hashIndexMask = 0;

      T->TransTab1W    = nil;
      T->TransTab1B    = nil;
      T->TransTab2W    = nil;
      T->TransTab2B    = nil;
   }
   else
   {
      T->transTabOn = true;

      while (2*T->transSize <= free) T->transSize *= 2;   // Calc the total number of used entries

      T->transUsed = 0;
      ULONG singleSize = T->transSize/4;                  // Size of a single transposition table
      T->hashIndexMask = singleSize - 1;                  // Compute the hash index mask.

      T->TransTab1W = E->P.TransTables;                   // "Allocate" the 4 transpostion tables
      T->TransTab1B = T->TransTab1W + singleSize;
      T->TransTab2W = T->TransTab1B + singleSize;
      T->TransTab2B = T->TransTab2W + singleSize;
   }

   for (INT d = 0; d <= maxSearchDepth + 2; d++)       // Finally update search nodes.
   {
      E->S.whiteNode[d - 2].TransTab1 = (even(d) ? T->TransTab1W : T->TransTab1B);
      E->S.whiteNode[d - 2].TransTab2 = (even(d) ? T->TransTab2W : T->TransTab2B);
   }
} /* CalcTransState */

/*---------------------------------- Reset Transposition Tables ----------------------------------*/
// Resets the transposition table by setting the "ply" and "maxPly" fields of all entries to -1,
// as well as "piece" = empty.

asm void ResetTransTab (ENGINE *E)
{
   #define engine rTmp1
   #define trans1 rTmp2
   #define trans2 rTmp3
   #define trans3 rTmp4
   #define trans4 rTmp5
   #define tsize  rTmp6
   #define dt     rTmp7
   #define reset  rTmp8

   lhz    rTmp0, ENGINE.Tr.transTabOn(engine)      // If (! E->Tr.transTabOn) return;
   cmpi   cr0,0, rTmp0,0
   beqlr-
   lwz    trans1, ENGINE.Tr.TransTab1W(engine)
   lwz    trans2, ENGINE.Tr.TransTab1B(engine)
   lwz    trans3, ENGINE.Tr.TransTab2W(engine)
   lwz    trans4, ENGINE.Tr.TransTab2B(engine)
   addi   trans1, trans1,-8
   addi   trans2, trans2,-8
   addi   trans3, trans3,-8
   addi   trans4, trans4,-8

   ori    reset, r0,0
   stw    reset, ENGINE.Tr.transUsed(engine)      // E->Tr.transUsed = 0;

   lwz    tsize, ENGINE.Tr.transSize(engine)
   li     dt, -16
   ori    reset, r0,0xFFFF

@loop                                             // Resets 16 entries in each loop.
   stwu   reset, 10(trans1)
   stwu   reset, 10(trans2)
   stwu   reset, 10(trans3)
   stwu   reset, 10(trans4)
   stwu   reset, 10(trans1)
   stwu   reset, 10(trans2)
   stwu   reset, 10(trans3)
   stwu   reset, 10(trans4)
   add.   tsize, tsize,dt
   stwu   reset, 10(trans1)
   stwu   reset, 10(trans2)
   stwu   reset, 10(trans3)
   stwu   reset, 10(trans4)
   stwu   reset, 10(trans1)
   stwu   reset, 10(trans2)
   stwu   reset, 10(trans3)
   stwu   reset, 10(trans4)
   bgt+   @loop
   blr

   #undef engine
   #undef trans1
   #undef trans2
   #undef trans3
   #undef trans4
   #undef tsize
   #undef dt
   #undef reset
} /* ResetTransTab */


/**************************************************************************************************/
/*                                                                                                */
/*                                      PROBE TRANSPOSITION TABLE                                 */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------------- Probe Trans Table -------------------------------------*/
// On entry to each node (but AFTER having checked draw info and computed the hashkey for the
// current position), we probe the transposition table: If the position is found, we first check
// if the score can be used directly. If it can't we instead return the stored "refutation" move
// (if any), which can then be search subsequently.
//
// NOTE: "ProbeTransTab" sets the rfm.piece field (even if it returns false). If rfm.piece = empty,
// then obviously there is NO refutation move (it's a null move). If on the other hand, a refutation
// move exists, then rfm.piece = the moving piece, and the reset of the rfm fields are subsequently
// set by "GetTransMove" (which may then NOT be called if rfm.piece = empty).

asm BOOL ProbeTransTab (void)
{
   #define hashkey  rTmp10
   #define hashmask rTmp5
   #define trn1     rTmp9
   #define trn2     rTmp8
   #define nply     rTmp7
   #define nmaxply  rTmp6
   #define tscore   rTmp5
   #define thashkey rTmp4
   #define tflags   rTmp4

   lwz     hashkey, node(hashKey)
   lwz     hashmask, ENGINE.Tr.hashIndexMask(rEngine)
   lwz     trn1, node(TransTab1)                // trans1 = &TransTab1[hashKey & hashIndexMask];
   lwz     trn2, node(TransTab2)
   lhz     rTmp2, node(drawType)
   lhz     rTmp3, node(pvNode)
   and     rTmp1, hashkey,hashmask              // trans2 = &TransTab2[hashKey & hashIndexMask];
   mulli   rTmp1, rTmp1,10
   add     trn1, trn1,rTmp1
   add     trn2, trn2,rTmp1
   stw     trn1, node(trans1)
   stw     trn2, node(trans2)
   dcbt    r0, trn1                             // Preload trans entries into processor cache
   dcbt    r0, trn2

   andi.   rTmp0, rFlags,rflag_TransTabOn       // if (! transTabOn || drawType != drawType_None)
   cmpi    cr5,0, rTmp2,drawType_None           // {
   cmpi    cr6,0, rTmp3,false
   beq-    @p
   beq+    cr5, @HADDR
@p li      rTmp1, 0                             //    if (! pvNode)
   bnelr-  cr6                                  //        clrMove(rfm);
   sth     rTmp1, rmove(piece)                  //    return false;
   blr                                          // }

   /* - - - - - - - - - - - - - - - Compute Hash Entry Addresses - - - - - - - - - - - - - - -*/

@HADDR
   bne-    cr6, @RET_FALSE                      // if (pvNode) return false;

   sth     rTmp3, rmove(piece)                  // clrMove(rfm);
   lhz     nply,  node(ply)
   lhz     nmaxply, node(maxPly)

   /* - - - - - - - - - - - - - - - Probe Conditional Hash Table - - - - - - - - - - - - - - -*/

@TRANS1
   lwz     thashkey, 0(trn1)                    // if (match(hashkey, trans1->hashkey))
   lbz     rTmp1, 4(trn1)
   lbz     rTmp2, 5(trn1)                       // {
   xor     rTmp0, hashkey,thashkey
   srawi.  rTmp0, rTmp0,11                      //    if (trans1->ply >= ply &&
   bne+    @TRANS2
   extsb   rTmp1, rTmp1
   extsb   rTmp2, rTmp2
   cmp     cr5,0, rTmp1,nply                    //        trans1->maxPly >= maxPly)
   cmp     cr6,0, rTmp2,nmaxply
   blt     cr5, @loadm1                         //    {
   blt     cr6, @loadm1

   lha     tscore, 6(trn1)                      //       score = trans1->score
   cmpi    cr7,0, tscore,mateWinVal
   lhz     rTmp1, node(depth)
   blt+    cr7, @nmate1                         //       if (score >= mateWinVal)
@pmate1
   subf    tscore, rTmp1,tscore                 //          score -= depth;
   b       @score1
@nmate1                                         //       else if (score <= mateLoseVal)
   cmpi    cr5,0, tscore,mateLoseVal
   bgt+    cr5, @score1                         //          score += depth;
   add     tscore, rTmp1,tscore
@score1
   sth     tscore, node(score)

   andi.   rTmp0, tflags,trans_TrueScoreBit     //       if (trans1->flags & trans_TrueScoreBit)
   bne     @found1                              //          return true;
@lower1
   andi.   rTmp1, tflags,trans_CutoffBit        //       else if (trans1->flags & trans_CutoffBit)
   lha     rTmp2, node(beta)                    //       {
   beq     @upper1                              //          if (score >= beta) return true;
   cmp     cr5,0, tscore,rTmp2
   blt+    cr5, @loadm1
@found1                                         //       }
   li      rTmp1,true                           //       else if (score <= alpha0)
   blr                                          //       {
@upper1
   lha     rTmp3, node(alpha0)                  //          return true;
   cmp     cr6,0, tscore,rTmp3
   ble     cr6, @found1                         //       }
                                                //    }
@loadm1
   andi.   rTmp1, tflags,trans_PieceMask        //    if (trans1->piece != empty)
   beq-    @TRANS2                              //    {
   stw     trn1, node(tmove)                    //       tmove = trans1;
   add     rTmp2, rPlayer,rTmp1                 //       rfm.piece = trans1->piece + player;
   sth     rTmp2, rmove(piece)                  //    }
                                                // }

   /* - - - - - - - - - - - - - - Probe Unconditional Hash Table - - - - - - - - - - - - - - -*/

@TRANS2
   lwz     thashkey, 0(trn2)                    // if (match(hashkey, trans2->hashkey))
   lbz     rTmp1, 4(trn2)
   lbz     rTmp2, 5(trn2)                       // {
   xor     rTmp0, hashkey,thashkey
   srawi.  rTmp0, rTmp0,11                      //    if (trans2->ply >= ply &&
   bne+    @RET_FALSE
   extsb   rTmp1, rTmp1
   extsb   rTmp2, rTmp2
   cmp     cr5,0, rTmp1,nply                    //        trans2->maxPly >= maxPly)
   cmp     cr6,0, rTmp2,nmaxply
   blt     cr5, @loadm2                         //    {
   blt     cr6, @loadm2

   lha     tscore, 6(trn2)                      //       score = trans2->score
   cmpi    cr7,0, tscore,mateWinVal
   lhz     rTmp1, node(depth)
   blt+    cr7, @nmate2                         //       if (score >= mateWinVal)
@pmate2
   subf    tscore, rTmp1,tscore                 //          score -= depth;
   b       @score2
@nmate2                                         //       else if (score <= mateLoseVal)
   cmpi    cr5,0, tscore,mateLoseVal
   bgt+    cr5, @score2                         //          score += depth;
   add     tscore, rTmp1,tscore
@score2
   sth     tscore, node(score)

   andi.   rTmp0, tflags,trans_TrueScoreBit     //       if (trans2->flags & trans_TrueScoreBit)
   bne     @found2                              //          return true;
@lower2
   andi.   rTmp1, tflags,trans_CutoffBit        //       else if (trans2->flags & trans_CutoffBit)
   lha     rTmp2, node(beta)                    //       {
   beq     @upper2                              //          if (score >= beta) return true;
   cmp     cr5,0, tscore,rTmp2
   blt+    cr5, @loadm2
@found2                                         //       }
   li      rTmp1,true                           //       else if (score <= alpha0)
   blr                                          //       {
@upper2
   lha     rTmp3, node(alpha0)                  //          return true;
   cmp     cr6,0, tscore,rTmp3
   ble     cr6, @found2                         //       }
                                                //    }
@loadm2
   andi.   rTmp1, tflags,trans_PieceMask        //    if (trans2->piece != empty)
   beqlr-                                        //    {
   stw     trn2, node(tmove)                    //       tmove = trans2;
   add     rTmp2, rPlayer,rTmp1                 //       rfm.piece = trans2->piece + player;
   sth     rTmp2, rmove(piece)                  //    }
                                                // }
@RET_FALSE
   li      rTmp1,false                          // return false;
   blr

   #undef hashkey
   #undef hashmask
   #undef trn1
   #undef trn2
   #undef nply
   #undef nmaxply
   #undef tscore
   #undef thashkey
   #undef tflags
} /* ProbeTransTab */

/*----------------------------------------- Get Trans Move ---------------------------------------*/
// The "GetTransMove" routine indicates if the transposition table move (found by "ProbeTransTab")
// may be searched (i.e. if it is both pseudo-legal and applicable at the current node). If so, the
// move is stored in "rfm". May NOT be called if rfm is a null move (i.e. if rfm.piece = empty).

asm BOOL GetTransMove (void)
{
   #define tm        rTmp10
   #define tfrom     rTmp9
   #define tto       rTmp8
   #define tflags    rTmp7
   #define tpiece    rTmp6
   #define tcap      rTmp5
   #define tdir      rTmp7
   #define tsq       rTmp10

   lwz     tm, node(tmove)                 // if (! (tmove->hkey & trans_Cap) &&
   lhz     rTmp1, node(quies)
   lhz     rTmp2, node(maxPly)             //     N->quies &&
   lhz     tflags, 2(tm)
   lbz     tfrom, 8(tm)
   lbz     tto, 9(tm)
   andi.   tcap, tflags,trans_CapMask
   bne     @MCAP
   cmpi    cr5,0, rTmp1,0                  //     (N->maxPly == 0 ||
   cmpi    cr6,0, rTmp2,0
   beq     cr5, @C                         //      ! (tmove->hkey & trans_ShallowQuies))
   beq     cr6, @NULLM                     //    )
   andi.   rTmp0, tflags,trans_ShQuiesBit  //    goto NullM;
   beq     @NULLM
   b       @C

@MCAP
   srawi   tcap, tcap,3                    // rfm.cap = (tmove->hkey & trans_Cap) >> 3;
   addi    tcap, tcap,black                // if (rfm.cap)
   subf    tcap, rPlayer,tcap              //    rfm.cap += opponent;
@C sth     tcap, rmove(cap)

   andi.   rTmp2, tflags,trans_DPlyMask    // rfm.dply = tmove->dply; (tmove->from & 0x80)
   srawi   rTmp2, rTmp2,6
   sth     tfrom, rmove(from)              // rfm.from = tmove->from;
   sth     rTmp2, rmove(dply)
   sth     tto,   rmove(to)                // rfm.to = tmove->to;

   li      rTmp1, 0                        // rfm.type = mtype_Normal;  NOTE: Only normal moves!
   andi.   tpiece, tflags,trans_PieceMask  // Get piece (already stored by "ProbeTransTab").
   add     tpiece, tpiece,rPlayer
   sth     rTmp1, rmove(type)

   subf    rTmp2, tfrom,tto                // rfm.dir = AttackDir[rfm.to - rfm.from] >> 5;
   addi    rTmp3, rGlobal,GLOBAL.A.AttackDir
   add     rTmp2, rTmp2,rTmp2
   lhax    tdir, rTmp3,rTmp2
   srawi   tdir, tdir,5
   sth     tdir, rmove(dir)

@LEGAL
   add     rTmp1, tfrom,tfrom              // if (Board[rfm.from] != rfm.piece ||
   add     rTmp2, tto,tto
   lhax    rTmp3, rBoard,rTmp1             //     Board[rfm.to] != rfm.cap)
   lhax    rTmp4, rBoard,rTmp2
   cmp     cr5,0, rTmp3,tpiece
   cmp     cr6,0, rTmp4,tcap               //    goto NullM;
   bne-    cr5, @NULLM
   bne-    cr6, @NULLM

   andi.   rTmp1, tpiece,0x07              // switch (pieceType(rfm.piece))
   cmpi    cr7,0, rTmp1,knight             // {
   beqlr-  cr7                             //    case knight : return true;
   bgt-    cr7, @King
@Pawn
   add     rTmp2, tfrom,rPawnDir           //    case pawn :
   cmpi    cr5,0, tcap,empty               //       if (rfm.cap || isEmpty(front(rfm.from)))
   add     rTmp2, rTmp2,rTmp2
   bnelr   cr5                             //          return true;
   lhzx    rTmp3, rBoard,rTmp2
   cmpi    cr6,0, rTmp3,empty              //       break;
   beqlr+
   b       @NULLM
@King
   cmpi    cr5,0, rTmp1,king               //    case king :
   rlwinm  rTmp2, tto,2, 0,31              //       if (! Attack_[rfm.to])
   add     tsq, tfrom,tdir
   blt+    cr5, @D
   lwzx    rTmp3, rAttack_,rTmp2           //           return true;
   cmpi    cr6,0, rTmp3,0
   beqlr+  cr6                             //       break;
   b       @NULLM
@Default                                   //    case queen : case rook : case bishop :
   lhzx    rTmp3, rBoard,rTmp2             //       dir = rfm.dir;
   add     tsq, tsq,tdir
   cmpi    cr6,0, rTmp3,empty              //       for (sq = rfm.from + dir; sq != rfm.to; sq += dir)  
   bne-    cr6, @NULLM
@D cmp     cr5,0, tsq,tto                  //          if (Board[sq] != empty) goto NullM;
   add     rTmp2, tsq,tsq
   bne+    cr5, @Default                   //       return true;
   blr                                     // }

@NULLM                                     // NullM:
   li      rTmp1, empty                    //    clrMove(rfm);
   sth     rTmp1, rmove(piece)             //    return false;
   blr

   #undef tm
   #undef tfrom
   #undef tto
   #undef tflags
   #undef tpiece
   #undef tcap
   #undef tdir
   #undef tsq
} /* GetTransMove */


/**************************************************************************************************/
/*                                                                                                */
/*                                       STORE TRANSPOSITION TABLE                                */
/*                                                                                                */
/**************************************************************************************************/

// Stores the current position (score, ply counters, flags, best move) in the transposition table,
// thus overriding the previous entry.

asm void StoreTransTab (void)
{
   #define trn      rTmp10
   #define thashkey rTmp9
   #define tflags   rTmp9
   #define nply     rTmp8
   #define tscore   rTmp7
   #define nalpha0  rTmp6
   #define nbeta    rTmp5

   #define mbest    rTmp8
   #define mpiece   rTmp7
   #define mcap     rTmp6
   #define mtype    rTmp5
   #define mfrom    rTmp4
   #define mto      rTmp3
   #define mdply    rTmp2
   #define bgen     rTmp1

   lhz     rTmp1, node(drawType)                // if (! transTabOn || drawType != drawType_None)
   lwz     trn, node(trans1)
   lhz     nply, node(ply)
   andi.   rTmp0, rFlags,rflag_TransTabOn       //    return;
   cmpi    cr5,0, rTmp1,drawType_None
   beqlr-
   lbz     rTmp2, 4(trn)
   bnelr-  cr5

   lwz     thashkey, node(hashKey)
   extsb   rTmp2, rTmp2
   cmp     cr6,0, nply,rTmp2                    // transX = (N->ply >= trans1->ply ?
   bge+    cr6, @STINFO                         //           trans1 : trans2);
   lwz     trn, node(trans2)
   
@STINFO
   lbz     rTmp2, 4(trn)
   lwz     rTmp1, ENGINE.Tr.transUsed(rEngine)  // if (transX->ply == 0xFF)
   cmpi    cr5,0, rTmp2,0xFF
   bne+    cr5, @stinf                          //    E->Tr.transUsed++;
   addi    rTmp1, rTmp1,1
   stw     rTmp1, ENGINE.Tr.transUsed(rEngine)

@stinf
   lhz     rTmp1, node(maxPly)                  // transX->hashKey = hashKey & trans_HashLockMask;
   lha     nbeta, node(beta)
   lha     nalpha0, node(alpha0)
   lha     tscore, node(score)                  // transX->flags   = 0;
   li      rTmp2, 0x07FF
   stb     nply, 4(trn)                         // transX->ply     = ply;
   andc    thashkey, thashkey,rTmp2             // transX->piece   = transX->cap = 0;
   stb     rTmp1, 5(trn)                        // transX->maxPly  = maxPly;

   cmp     cr5,0, tscore,nbeta                  // if (score >= beta)
   blt     cr5, @trueSc
   ori     tflags, tflags,trans_CutoffBit       //    transX->flags |= trans_CutoffBit,
   mr      tscore, nbeta                        //    transX->score = beta;
   b       @MATECHK
@trueSc
   cmp     cr6,0, tscore,nalpha0                // else if (score > alpha0)
   ble     cr6, @upper
   ori     tflags, tflags,trans_TrueScoreBit    //    transX->flags |= trans_TrueScoreBit,
   b       @MATECHK                             //    transX->score = score;
@upper                                          // else
   mr      tscore, nalpha0                      //    transX->score = alpha0;

@MATECHK
   lhz     rTmp1, node(depth)
   cmpi    cr5,0, tscore,mateWinVal             // if (transX->score >= mateWinVal)
   blt+    cr5, @m                              //    transX->score += depth;
   add     tscore, rTmp1,tscore
   b       @stScore
@m cmpi    cr6,0, tscore,mateLoseVal            // else if (transX->score <= mateLoseVal)
   bgt+    cr6, @stScore                        //    transX->score -= depth;
   subf    tscore, rTmp1,tscore
@stScore
   sth     tscore, 6(trn)

@STMOVE
   addi    mbest, rNode,NODE.BestLine           // MOVE *m = &BestLine[0];
   lhz     mpiece, MOVE.piece(mbest)
   lhz     mtype, MOVE.type(mbest)             // if (isNull(*m) || m->type != normal)
   andi.   mpiece, mpiece,0x07
   cmpi    cr6,0, mtype,mtype_Normal            //    return;
   beq-    @RETURN
   bne-    cr6, @RETURN

   lhz     mfrom, MOVE.from(mbest)              // transX->from = m->from;
   lhz     mto, MOVE.to(mbest)                  // transX->to = m->to;
   lhz     mcap, MOVE.cap(mbest)                // transX->cap = m->cap;
   lhz     mdply, MOVE.dply(mbest)              // transX->dply = m->dply; ### Min(1, ...)
   stb     mfrom, 8(trn)
   stb     mto, 9(trn)
   rlwinm  mcap, mcap,3, 26,28
   rlwinm  mdply, mdply,6, 24,25
   or      tflags,tflags,mpiece                 // transX->piece = m->piece;
   or      tflags,tflags,mcap
   or      tflags,tflags,mdply

   lhz     bgen, node(bestGen)                  // if (bestGen == gen_E ||ÊbestGen == gen_J);
   cmpi    cr5,0, bgen,gen_E
   cmpi    cr6,0, bgen,gen_J                    //    transX->flags |= trans_ShQuiesBit;
   beq-    cr5, @q
   bne+    cr6, @RETURN
@q ori     tflags,tflags, trans_ShQuiesBit

@RETURN
   stw     tflags, 0(trn)
   blr

   #undef trn
   #undef thashkey
   #undef tflags
   #undef nply
   #undef tscore
   #undef nalpha0
   #undef nbeta

   #undef mbest
   #undef mpiece
   #undef mcap
   #undef mtype
   #undef mfrom
   #undef mto
   #undef mdply
   #undef bgen

} /* StoreTransTab */
