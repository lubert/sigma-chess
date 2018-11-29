/**************************************************************************************************/
/*                                                                                                */
/* Module  : SEARCHMISC.C */
/* Purpose : This module implements various utility routines used during the
 * search, such as      */
/*           "Killer" handling, refutation collision checks, best line updating
 * etc.              */
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

#include "SearchMisc.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                       UPDATE BEST LINE */
/*                                                                                                */
/**************************************************************************************************/

asm void UpdateBestLine(void) {
  addi rTmp1, rNode,
      (NODE.BestLine - 4)  // L1 = N->BestLine;
      lwz rTmp4,
      move(piece) lwz rTmp5, move(to) lwz rTmp6, move(type) lwz rTmp7,
      move(dply) stwu rTmp4,
      4(rTmp1)  // *(L1++) = N->m;
      stwu rTmp5,
      4(rTmp1)stwu rTmp6, 4(rTmp1)stwu rTmp7,
      4(rTmp1)

          addi rTmp2,
      rNode,
      (sizeof(NODE) + NODE.BestLine - 4)  // L2 = NN->BestLine;
      @L lwzu rTmp4,
      4(rTmp2)lwzu rTmp5,
      4(rTmp2)  // do
      lwzu rTmp6,
      4(rTmp2)lwzu rTmp7, 4(rTmp2)stwu rTmp4,
      4(rTmp1)  //    *(L1++) = *(L2++);
      stwu rTmp5,
      4(rTmp1)andis.rTmp9, rTmp4, 0x00FF stwu rTmp6, 4(rTmp1)stwu rTmp7,
      4(rTmp1)bne + @L  // while (! isNull(*L2));

                        blr
} /* UpdateBestLine */

/**************************************************************************************************/
/*                                                                                                */
/*                                       UPDATE DRAW STATE */
/*                                                                                                */
/**************************************************************************************************/

asm void UpdateDrawState(void) {
#define dt rTmp10
#define newkey rTmp9
#define mpiece rTmp8
#define mfrom rTmp7
#define mto rTmp6
#define mcap rTmp5
#define mtype rTmp4

#define rev rTmp8
#define n rTmp7
#define dt1 rTmp6
#define sdt sizeof(DRAWDATA)

  //--- Compute hash key for current position ---
  // This is done by XOR'ing the hash key change from the most recently
  // performed move with the hash key of the previous position.

  @HASH li rTmp1,
      drawType_None  // N->drawType = drawType_None;
          lwz dt,
      node(drawData)  // dt = N->drawTab;
      sth rTmp1,
      node(drawType) lhz mpiece, pmove(piece) lhz mfrom,
      pmove(from)  // dt->hashKey = (dt-1)->hashKey
      lhz mto,
      pmove(to) lhz mcap,
      pmove(cap)  // dt->hashKey ^= HashCode[pm.piece][pm.from];
      lhz mtype,
      pmove(type) addi rTmp1, rGlobal, GLOBAL.H.HashCode rlwinm rTmp2, mpiece,
      9, 17, 22 add rTmp2, rTmp1,
      rTmp2  // dt->hashKey ^= HashCode[pm.piece][pm.to];
          rlwinm mfrom,
      mfrom, 2, 0, 29 rlwinm mto, mto, 2, 0, 29 lwz newkey,
      (DRAWDATA.hashKey - sdt)(dt)lwzx rTmp0, rTmp2, mfrom lwzx rTmp3, rTmp2,
      mto

          rlwinm.mcap,
      mcap, 9, 17,
      22  // if (pm.cap)
          xor newkey,
      newkey, rTmp0 xor newkey, newkey,
      rTmp3 beq @Switch  //    dt->hashKey ^= HashCode[pm.cap][pm.to];
          add rTmp2,
      rTmp1, mcap lwzx rTmp3, rTmp2, mto xor newkey, newkey,
      rTmp3

      @Switch  // switch (pm.type)
          cmpi cr5,
      0, mtype,
      mtype_Normal  // {
          andi.rTmp0,
      mtype, mtype_Promotion beq + cr5,
      @DRAW  //    case mtype_Normal : break;
              bne +
          @Prom cmpi cr6,
      0, mtype, mtype_O_O_O blt + cr6, @o_o beq + cr6, @o_o_o @Ep addi rTmp2,
      rPlayer,
      pawn  //    case mtype_EP :
          rlwinm rTmp3,
      rPawnDir, 2, 0,
      29  //       dt->hashKey ^= HashCode[friendly(pawn)][front(pm.to)]; break;
      rlwinm rTmp2,
      rTmp2, 9, 17, 22 add mto, mto, rTmp3 add rTmp2, rTmp1, rTmp2 lwzx rTmp3,
      rTmp2, mto xor newkey, newkey, rTmp3 b @Irrmove @o_o addi rTmp1, rGlobal,
      GLOBAL.H.o_oHashCodeB  //    case mtype_O_O :
          srawi rTmp2,
      rPlayer,
      2  //       dt->hashKey ^= o_oHashCode[opponent]; break;
      lwzx rTmp3,
      rTmp1, rTmp2 xor newkey, newkey, rTmp3 b @Irrmove @o_o_o addi rTmp1,
      rGlobal,
      GLOBAL.H.o_o_oHashCodeB  //    case mtype_O_O_O :
          srawi rTmp2,
      rPlayer,
      2  //       dt->hashKey ^= o_o_oHashCode[opponent]; break;
      lwzx rTmp3,
      rTmp1, rTmp2 xor newkey, newkey, rTmp3 b @Irrmove @Prom rlwinm.rTmp2,
      mtype, 9, 17,
      22  //    default :     // Prom
      add rTmp2,
      rTmp2,
      rTmp1  //       dt->hashKey ^= H->HashCode[pm.type][pm.to];
          lwzx rTmp3,
      rTmp2, mto xor newkey, newkey,
      rTmp3  // }
          b @Irrmove

      //--- Update Draw State ---
      // If the move is irreversible (i.e. capture, pawn move or special move),
      // then we update state info for the current node directly, and check if
      // it's a draw by insufficient material

      @DRAW andi.mpiece,
      mpiece,
      0x0F  // if (pm.cap ||Êpm.type != mtype_Normal || pieceType(pm.piece) ==
            // pawn)
      cmpi cr5,
      0, mcap, empty cmpi cr6, 0, mpiece, pawn bne + cr5, @Irrmove bne + cr6,
      @Revmove @Irrmove  // {
          lhz rTmp1,
      node(gameDepth)  //    dt->irr = N->gameDepth;
      li rTmp2,
      0 stw newkey,
      DRAWDATA.hashKey(dt)  //    dt->repCount = 0;
      stw newkey,
      node(hashKey)  //    N->hashKey = dt->hashKey;
      sth rTmp2,
      DRAWDATA.repCount(dt) sth rTmp1,
      DRAWDATA
          .irr(dt)  //    // exit if more than one B or N of either colour
      andi.rTmp3,
      rPieceCount,
      0xFE0F  //    if (pieceCount & 0xFE0FFE0F) return;
          bnelr +
          andis.rTmp4,
      rPieceCount, 0xFE0F bnelr + srawi rTmp5, rPieceCount,
      16  //    if (KK ||ÊKNK ||ÊKBK)
      add rTmp5,
      rPieceCount, rTmp5 andi.rTmp5, rTmp5,
      0x0E00  //       N->drawType = drawType_InsuffMtrl;
          bnelr +
          li rTmp1,
      drawType_InsuffMtrl sth rTmp1,
      node(drawType) blr  // }

      @Revmove  // else
          lhz rTmp1,
      (DRAWDATA.irr - sdt)(dt)  // {
      lhz rTmp2,
      node(gameDepth)  //    dt->irr = (dt-1)->irr;
      stw newkey,
      DRAWDATA.hashKey(dt) stw newkey,
      node(hashKey)  //    N->hashKey = dt->hashKey;
      sth rTmp1,
      DRAWDATA.irr(dt)  //    INT rev = N->gameDepth - dt->irr;
      subf rev,
      rTmp1, rTmp2 cmpi cr5, 0, rev,
      100  //    if (rev >= 100)
          blt +
          cr5,
      @CheckRep  //    {
      @Draw50    //       N->drawType = draw50;
          li rTmp1,
      drawType_50 sth rTmp1,
      node(drawType)  //    }
      blr             //    else
      @CheckRep       //    {
          srawi n,
      rev,
      1  //       INT n = rev/2 - 1;
      addi n,
      n, -1 cmpi cr5, 0, n,
      0  //       if (n > 0)
          ble -
          cr5,
      @Rep0  //       {
          mr dt1,
      dt  //          dt1 = dt - 2;
          addi dt1,
      dt1,
      -2 * sdt @Do  //          do
               lwzu rTmp1,
      (-2 * sdt)(dt1)  //          {  dt1 -= 2;
      cmp cr5,
      0, rTmp1,
      newkey  //             if (dt1->hashKey == dt->hashKey)
              bne +
          cr5,
      @Od  //             {
          lhz rTmp2,
      DRAWDATA.repCount(dt1)  //                dt->repCount = dt->repCount + 1;
      addi rTmp2,
      rTmp2, 1 sth rTmp2,
      DRAWDATA.repCount(dt)  //                N->drawType = dt->repCount;
      sth rTmp2,
      node(drawType) blr  //                return;
      @Od                 //             }
          addi n,
      n, -1 cmpi cr6, 0, n, 0 bgt + cr6,
      @Do    //          } while (--n > 0);
      @Rep0  //       }
          li rTmp1,
      0  //       dt->repCount = 0;
      sth rTmp1,
      DRAWDATA.repCount(dt)  //    }
      blr                    // }

#undef dt
#undef newkey
#undef mpiece
#undef mfrom
#undef mto
#undef mcap
#undef mtype

#undef rev
#undef n
#undef dt1
#undef sdt
} /* UpdateDrawState */

/**************************************************************************************************/
/*                                                                                                */
/*                                         KILLER MOVES */
/*                                                                                                */
/**************************************************************************************************/

// Up to 2 killer moves are maintained at each node.

/*-------------------------------------- Preparing Killers
 * ---------------------------------------*/
// "Prepares" the killers by setting the "killerActive" flags and making sure
// the most popular killer is searched first (i.e. "killer1").

asm void PrepareKillers(void) {
  lwz rTmp1,
      node(check)  // if (N->check || N->quies)
      lwz rTmp3,
      node(killer1Count) lwz rTmp4,
      node(killer2Count)  // {
      cmpi cr5,
      0, rTmp1,
      0  //    killer1Active = killer2Active = false;
      li rTmp2,
      0 beq - cr5,
      @ACTIVE  //    return;
          stw rTmp2,
      node(killer1Active)  // }  <-- 2 fields in one instruction
      blr @ACTIVE srawi rTmp5,
      rTmp3,
      16  // killer1Active = (killer1Count > 0);
      srawi rTmp6,
      rTmp4, 16 or rTmp3, rTmp3, rTmp5 or rTmp4, rTmp4,
      rTmp6  // killer2Active = (killer2Count > 0);
          sth rTmp3,
      node(killer1Active) sth rTmp4, node(killer2Active) blr
} /* PrepareKillers */

/*------------------------- Checking Killer/Refutation Move Collision
 * ----------------------------*/
// Each move must be compared against the killers and the refutation move, so we
// don't search a move more than once.

asm BOOL KillerRefCollision(void) {
  lhz rTmp1, node(killer1Active) lhz rTmp2, node(killer2Active) lwz rTmp3,
      move(from) lhz rTmp4, move(piece) lwz rTmp5, move(cap) lwz rTmp6,
      kmove1(from) lwz rTmp7, kmove2(from) lwz rTmp8, rmove(from) lhz rTmp9,
      node(gen)

          @Killer1 cmpi cr5,
      0, rTmp1,
      0  // if (killer1Active &&
      cmp cr6,
      0, rTmp3, rTmp6 beq cr5,
      @Killer2  //     EqualMove(m, killer1) &&
              bne +
          cr6,
      @Killer2 cmpi cr7, 0, rTmp9,
      gen_F1  //     gen != gen_F1)
              beq -
          cr7,
      @Killer2  // {

          lhz rTmp6,
      kmove1(piece) lwz rTmp10, kmove1(cap) cmp cr5, 0, rTmp4, rTmp6 cmp cr6, 0,
      rTmp5, rTmp10 li rTmp6,
      0  //    killer1Active = false;
          bne -
          cr5,
      @Killer2 bne - cr6, @Killer2 sth rTmp6,
      node(killer1Active)
      //    if (gen > killer1Gen) return true (rTmp1);
      bgtlr cr7       //    goto @RefMove
          b @RefMove  // }

      @Killer2 cmpi cr5,
      0, rTmp2,
      0  // if (killer2Active &&
      cmp cr6,
      0, rTmp3, rTmp7 beq cr5,
      @RefMove  //     EqualMove(m, killer2) &&
              bne +
          cr6,
      @RefMove cmpi cr7, 0, rTmp9,
      gen_F2  //     gen != gen_F1)
              beq -
          cr7,
      @RefMove  // {

          lhz rTmp6,
      kmove2(piece) lwz rTmp10, kmove2(cap) cmp cr5, 0, rTmp4, rTmp6 cmp cr6, 0,
      rTmp5, rTmp10 li rTmp6,
      0  //    killer2Active = false;
          bne -
          cr5,
      @RefMove bne - cr6, @RefMove sth rTmp6,
      node(killer2Active)
          //    if (gen > killer2Gen) return true;
          bgt +
          cr7,
      @True  // }

      @RefMove cmp cr5,
      0, rTmp3,
      rTmp8  // return (EqualMove(m, rfm));
          li rTmp1,
      false bnelr cr5

          lhz rTmp6,
      rmove(piece) lwz rTmp10, rmove(cap) cmp cr5, 0, rTmp4, rTmp6 cmp cr6, 0,
      rTmp5,
      rTmp10 bnelr cr5 bnelr cr6

      @True li rTmp1,
      true blr
} /* KillerRefCollision */

/*-------------------------------------- Update Killers
 * ------------------------------------------*/
// Updates the killer table upon exit from a node: If a killer worked again, its
// popularity count is increased by 1. Otherwise replace "killer2" by the best
// move if it's a non capture or a sacrifice.

asm void UpdateKillers(void) {
  lwz rTmp1,
      node(check)  // if (quies ||Êcheck || bestGen < killer1Gen)
      lhz rTmp2,
      node(bestGen)  //    return;
      cmpi cr5,
      0, rTmp1, 0 bnelr + cr5 cmpi cr6, 0, rTmp2, gen_F1 bltlr + cr6 bgt + cr6,
      @k2  // switch (bestGen)
      @k1 lwz rTmp3,
      node(killer1Count)  // {
      addi rTmp3,
      rTmp3,
      1  //    case killer1Gen : killer1Count++; break;
      stw rTmp3,
      node(killer1Count) blr @k2 cmpi cr0, 0, rTmp2,
      gen_G bgt + @newK beqlr - lwz rTmp3,
      node(killer2Count)  //    case killer2Gen : killer2Count++; break;
      addi rTmp3,
      rTmp3, 1 stw rTmp3, node(killer2Count) blr @newK lhz rTmp4,
      node(BestLine[0].to)  //    case nonCapGen : case sacriGen :
      lhz rTmp5,
      pmove(to)  //       if (BestLine[0].to == PN->m.to) return;
      lwz rTmp1,
      node(killer1Count) lwz rTmp2, node(killer2Count) cmp cr5, 0, rTmp4,
      rTmp5 beqlr - cr5  //       if (killer1Count > killer2Count)
                        lwz rTmp3,
      node(BestLine[0].piece)  //       {
      lwz rTmp4,
      node(BestLine[0].to) lwz rTmp5,
      node(BestLine[0].type)  //          killer2 = killer1;

      cmp cr6,
      0, rTmp1, rTmp2 ble + cr6,
      @Store  //          killer2Count = killer1Count;
          lwz rTmp6,
      kmove1(piece) lwz rTmp7, kmove1(to) lwz rTmp8, kmove1(type) stw rTmp1,
      node(killer2Count)  //       }
      stw rTmp6,
      kmove2(piece) stw rTmp7,
      kmove2(to)  //       killer1 = BestLine[0];
      stw rTmp8,
      kmove2(type)

          @Store li rTmp9,
      1 stw rTmp3, kmove1(piece) stw rTmp4,
      kmove1(to)  //       killer1Count = 1;
      stw rTmp5,
      kmove1(type) stw rTmp9,
      node(killer1Count)  // }
      blr
} /* UpdateKillers */
