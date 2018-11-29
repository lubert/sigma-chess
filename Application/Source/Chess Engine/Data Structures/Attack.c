/**************************************************************************************************/
/*                                                                                                */
/* Module  : ATTACK.C                                                                             */
/* Purpose : This module implements the "Attack" data structure and various operations. Two       */
/*           attack tables are maintained - one for each side - which for each square describes   */
/*           how this is attacked by the various piece types.                                     */
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

#include "Attack.f"
#include "Engine.f"


/**************************************************************************************************/
/*                                                                                                */
/*                                        RESET ATTACK TABLE                                      */
/*                                                                                                */
/**************************************************************************************************/

// Before the engine can start searching, the attack state must first be rebuilt from scratch from
// the current board position (Engine->B.Board).

void CalcAttackState (register ENGINE *E)
{
   for (SQUARE sq = -boardSize1; sq < boardSize2; sq++)       // Clear attack table.
      E->A.AttackW[sq] = E->A.AttackB[sq] = 0;

   Asm_Begin(E);
      INT mobSum = 0;
      for (SQUARE sq = a1; sq <= h8; sq++)                 // Compute attack table, pawn
         if (onBoard(sq) && E->B.Board[sq])                // structure and mobility.
            mobSum += UpdPieceAttack(sq);
      E->S.rootNode->mobEval = mobSum;
   Asm_End();
} /* CalcAttackState */


/**************************************************************************************************/
/*                                                                                                */
/*                                     UPDATING ATTACK TABLE                                      */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------- Adding/Removing Piece Attack ---------------------------------*/
// When a piece is added to a square, we must additionally add attack information for that piece.
// Similarily, when removing a piece from a square we must also remove attack information. This
// is handled by the "UpdPieceAttack(sq)" routine which simply inverts the relevant attack bits for
// the affected squares.

#define wdispatch(L)  addi rTmp4,rTmp4,ENGINE.A.AttackW ; b L
#define bdispatch(L)  addi rTmp4,rTmp4,ENGINE.A.AttackB ; b L

#define upd_qrb_attack(Dir,Bit,L,dm) \
   addi   rTmp7, rTmp4,2             ; \
   add    rTmp6, rBoard,rTmp2        ; \
L  lhz    rTmp9, (4*Dir)(rTmp7)      ; \
   lhau   rTmp8, (2*Dir)(rTmp6)      ; \
   addi   rTmp1, rTmp1,dm            ; \
   xori   rTmp9, rTmp9,Bit           ; \
   cmpi   cr0,0, rTmp8,empty         ; \
   sthu   rTmp9, (4*Dir)(rTmp7)      ; \
   beq+   cr0,L

asm INT UpdPieceAttack (register SQUARE sq)  // sq in r3

// rTmp0  = temporary link register
// rTmp1  = 2*sq. On exit: Mobility change
// rTmp2  = 4*sq + rEngine (used to compute r6 by adding attack table offset in dispatch macros)
// rTmp3  = Bsq = &Board[sq]
// rTmp4  = Asq = &Attack[colour][sq]
// rTmp5  = Scratch + piece on Board[sq]
// rTmp6  = Scratch + temp Bsq + offset
// rTmp7  = Scratch + temp Asq + offset
// rTmp8  = Scratch + temp B[offset]
// rTmp9  = Scratch + temp Asq[offset]
// rTmp10 = Scratch + temporary link register + mob change

{
   add     rTmp2, rTmp1,rTmp1
   mflr    r0
   lhzx    rTmp3, rBoard,rTmp2                   // piece = Board[sq]
   add     rTmp4, rTmp2,rTmp2
   bl      @d                                    // switch (piece)
@d mflr    rTmp10
   add     rTmp4, rTmp4,rEngine
   rlwinm  rTmp3, rTmp3,3,0,28
   addi    rTmp3, rTmp3,20
   add     rTmp10, rTmp10,rTmp3
   mtlr    rTmp10
   blr                                           // {

   // Dispatch table:
   wdispatch(@WPAWN)
   wdispatch(@KNIGHT)
   wdispatch(@WBISHOP)
   wdispatch(@WROOK)
   wdispatch(@WQUEEN)
   wdispatch(@KING)
   nop16 ; nop4
   bdispatch(@BPAWN)
   bdispatch(@KNIGHT)
   bdispatch(@BBISHOP)
   bdispatch(@BROOK)
   bdispatch(@BQUEEN)
   addi    rTmp4, rTmp4,ENGINE.A.AttackB

@KING                                            // case wKing: case bKing:
   upd_king_attack(rTmp4)
   mtlr    r0
   li      rTmp1,0
   blr                                           //    return 0;

@WQUEEN                                          // case wQueen:
   li      rTmp1,0                               //    dmob = 0;
   upd_qrb_attack(-0x0F, 0x0001, @wq1, queenMob)
   upd_qrb_attack(-0x11, 0x0002, @wq2, queenMob)
   upd_qrb_attack( 0x11, 0x0004, @wq3, queenMob)
   upd_qrb_attack( 0x0F, 0x0008, @wq4, queenMob)
   upd_qrb_attack(-0x10, 0x0010, @wq5, queenMob)
   upd_qrb_attack( 0x10, 0x0020, @wq6, queenMob)
   upd_qrb_attack( 0x01, 0x0040, @wq7, queenMob)
   upd_qrb_attack(-0x01, 0x0080, @wq8, queenMob)
   mtlr    r0
   blr                                           //    return dmob;

@BQUEEN                                          // case bQueen:
   li      rTmp1,0                               //    dmob = 0;
   upd_qrb_attack(-0x0F, 0x0001, @bq1, -queenMob)
   upd_qrb_attack(-0x11, 0x0002, @bq2, -queenMob)
   upd_qrb_attack( 0x11, 0x0004, @bq3, -queenMob)
   upd_qrb_attack( 0x0F, 0x0008, @bq4, -queenMob)
   upd_qrb_attack(-0x10, 0x0010, @bq5, -queenMob)
   upd_qrb_attack( 0x10, 0x0020, @bq6, -queenMob)
   upd_qrb_attack( 0x01, 0x0040, @bq7, -queenMob)
   upd_qrb_attack(-0x01, 0x0080, @bq8, -queenMob)
   mtlr    r0
   blr                                           //    return dmob;

@WROOK                                           // case wRook:
   li      rTmp1,0                               //    dmob = 0;
   upd_qrb_attack(-0x10, 0x1000, @wr1, rookMob)
   upd_qrb_attack( 0x10, 0x2000, @wr2, rookMob)
   upd_qrb_attack( 0x01, 0x4000, @wr3, rookMob)
   upd_qrb_attack(-0x01, 0x8000, @wr4, rookMob)
   mtlr    r0
   blr                                           //    return dmob;

@BROOK                                           // case bRook:
   li      rTmp1,0                               //    dmob = 0;
   upd_qrb_attack(-0x10, 0x1000, @br1, -rookMob)
   upd_qrb_attack( 0x10, 0x2000, @br2, -rookMob)
   upd_qrb_attack( 0x01, 0x4000, @br3, -rookMob)
   upd_qrb_attack(-0x01, 0x8000, @br4, -rookMob)
   mtlr    r0
   blr                                           //    return dmob;

@WBISHOP                                         // case wBishop:
   li      rTmp1,0                               //    dmob = 0;
   upd_qrb_attack(-0x0F, 0x0100, @wb1, bishopMob)
   upd_qrb_attack(-0x11, 0x0200, @wb2, bishopMob)
   upd_qrb_attack( 0x11, 0x0400, @wb3, bishopMob)
   upd_qrb_attack( 0x0F, 0x0800, @wb4, bishopMob)
   mtlr    r0
   blr                                           //    return dmob;

@BBISHOP                                         // case bBishop:
   li      rTmp1,0                               //    dmob = 0;
   upd_qrb_attack(-0x0F, 0x0100, @bb1, -bishopMob)
   upd_qrb_attack(-0x11, 0x0200, @bb2, -bishopMob)
   upd_qrb_attack( 0x11, 0x0400, @bb3, -bishopMob)
   upd_qrb_attack( 0x0F, 0x0800, @bb4, -bishopMob)
   mtlr    r0
   blr                                           //    return dmob;

@KNIGHT                                          // case wKnight: case bKnight:
   upd_knight_attack(rTmp4)
   mtlr    r0
   li      rTmp1,0
   blr                                           //    return 0;

@WPAWN                                           // case wPawn:
   lhz     rTmp8, 60(rTmp4)                      //    Asq[+0x0F] xor= 0x04000000;
   lhz     rTmp9, 68(rTmp4)                      //    Asq[+0x11] xor= 0x02000000;
   addi    rTmp2, rEngine,ENGINE.B.PawnStructW   //    PawnStructW[rank(sq)] xor= bit(file(sq));  // Toggle pawn structure bit
   srawi   rTmp3, rTmp1,4
   andi.   rTmp10,rTmp1,0x07
   lbzx    rTmp5, rTmp2,rTmp3
   xori    rTmp8, rTmp8,0x0400
   li      rTmp6, 1
   xori    rTmp9, rTmp9,0x0200
   rlwnm   rTmp6, rTmp6,rTmp10,0,31
   sth     rTmp8, 60(rTmp4)
   sth     rTmp9, 68(rTmp4)
   xor     rTmp5, rTmp5,rTmp6
   mtlr    r0
   stbx    rTmp5, rTmp2,rTmp3
   li      rTmp1,0
   blr                                           //    return 0;

@BPAWN                                           // case bPawn:
   lhz     rTmp8, -68(rTmp4)                     //    Asq[-0x11] xor= 0x04000000;
   lhz     rTmp9, -60(rTmp4)                     //    Asq[-0x0F] xor= 0x02000000;
   addi    rTmp2, rEngine,ENGINE.B.PawnStructB   //    PawnStructB[rank(sq)] xor= bit(file(sq));  // Toggle pawn structure bit
   srawi   rTmp3, rTmp1,4
   andi.   rTmp10,rTmp1,0x07
   lbzx    rTmp5, rTmp2,rTmp3
   xori    rTmp8, rTmp8,0x0400
   li      rTmp6, 1
   xori    rTmp9, rTmp9,0x0200
   rlwnm   rTmp6, rTmp6,rTmp10,0,31
   sth     rTmp8, -68(rTmp4)
   sth     rTmp9, -60(rTmp4)
   xor     rTmp5, rTmp5,rTmp6
   mtlr    r0                                    //    return 0;
   stbx    rTmp5, rTmp2,rTmp3
   li      rTmp1,0
   blr                                           // }
} /* UpdPieceAttack */

/*---------------------------------- Adding/Removing Block Attack --------------------------------*/
// During the search, as moves are performed, the moving pieces may block/unblock attack of
// other pieces (queens, rooks and bishop). The "UpdBlockAttack(sq)" routine inverts the
// relevant attack bits for the block/unblock on square "sq".
// NOTE: Only uses the volatile registers rTmp0...rTmp10, and cr0 and cr5.

asm INT UpdBlockAttack (register SQUARE sq)
{
   #define sq2    rTmp1
   #define sq4    rTmp2
   #define dir2   rTmp3
   #define dir4   rTmp4
   #define Asq    rTmp5
   #define Bsq    rTmp6
   #define At     rTmp7
   #define bits   rTmp8    // Note: Occupies same register as BD
   #define BD     rTmp9
   #define dA     rTmp10
   #define B      rTmp9
   #define Ax     rTmp0

   #define dmob   rLocal1
   #define dm     rLocal2

   rlwinm  sq4, sq,2,0,31
   add     sq2, sq,sq
   lwzx    At, rAttack,sq4
   stw     dmob, -4(SP)
   stw     dm, -8(SP)
   li      dmob, 0                               // dmob = 0;

//--- Updating PLAYER Block Attack ---

@PLAYER
   andi.   At, At,0xFFFF                         // if (Attack[sq] & qrbMask)
   beq+    @OPPONENT                             // {
   srawi   bits, At,8                            //    bits = fold(Attack[sq] & qrbMask);
   or      bits, bits,At
   rlwinm  bits, bits,3,21,28
@PLoop                                           //    do
   addi    BD, rGlobal,GLOBAL.A.BlockTab         //    {
   add     Bsq, rBoard,sq2                       //       Bsq  = &Board[sq];
   lhaux   dir2, BD,bits                         //       dir  = BlockData[bits].dir;
   lhz     dA, BLOCKTAB.rayBits(BD)              //       dA   = BlockData[bits].dA;
   lhz     bits, BLOCKTAB.nextBits(BD)           //       bits = BlockData[bits].nextBits;
   add     Asq, rAttack,sq4                      //       Asq  = &Attack[sq];
   add     dir4, dir2,dir2
   and     dA, dA,At                             //       dA &= A;  // Build the bit mask

   andi.   Ax, dA,0x0F00                         //       if (dA & bMask)
   li      dm, bishopMob                         //          dm = bishopMob;
   bne+    @PDir
   andi.   Ax, dA,0xF000                         //       else if (dA & rMask)
   li      dm, rookMob                           //          dm = bishopMob;
   bne+    @PDir                                 //       else
   li      dm, queenMob                          //          dm = queenMob;

@PDir                                            //       do
   lwzux   Ax, Asq,dir4                          //       {  Asq += dir;
   lhzux   B, Bsq,dir2                           //          Bsq += dir; 
   add     dmob, dmob,dm                         //          dmob += dm;
   xor     Ax, Ax,dA                             //          *Asq xor= dA;
   cmpi    cr0,0, B,empty
   sth     Ax, 2(Asq)
   beq+    cr0,@PDir                             //       } while (! *Bsq);
   cmpi    cr5,0, bits,0                         //    } while (bits);
   bne-    cr5,@PLoop                            // }

//--- Updating OPPONENT Block Attack ---

@OPPONENT
   lwzx    At, rAttack_,sq4                      // if (Attack_[sq] & qrbMask)
   andi.   At, At,0xFFFF
   beq+    @RETURN                               // {
   srawi   bits, At,8                            //    bits = fold(Attack_[sq] & qrbMask);
   or      bits, bits,At
   rlwinm  bits, bits,3,21,28
@OLoop                                           //    do
   addi    BD, rGlobal,GLOBAL.A.BlockTab         //    {
   add     Bsq, rBoard,sq2                       //       Bsq  = &Board[sq];
   lhaux   dir2, BD,bits                         //       dir  = BlockData[bits].dir;
   lhz     dA, BLOCKTAB.rayBits(BD)              //       dA   = BlockData[bits].dA;
   lhz     bits, BLOCKTAB.nextBits(BD)           //       bits = BlockData[bits].nextBits;
   add     Asq, rAttack_,sq4                     //       Asq  = &Attack_[sq];
   add     dir4, dir2,dir2
   and     dA, dA,At                             //       dA &= A;  // Build the bit mask

   andi.   Ax, dA,0x0F00                         //       if (dA & bMask)
   li      dm, -bishopMob                        //          dm = -bishopMob;
   bne+    @ODir
   andi.   Ax, dA,0xF000                         //       else if (dA & rMask)
   li      dm, -rookMob                          //          dm = -bishopMob;
   bne+    @ODir                                 //       else
   li      dm, -queenMob                         //          dm = -queenMob;

@ODir                                            //       do
   lwzux   Ax, Asq,dir4                          //       {  Asq += dir;
   lhzux   B, Bsq,dir2                           //          Bsq += dir; 
   add     dmob, dmob,dm                         //          dmob += dm;
   xor     Ax, Ax,dA                             //          *Asq xor= dA;
   cmpi    cr0,0, B,empty
   sth     Ax, 2(Asq)
   beq+    cr0,@ODir                             //       } while (! *Bsq);
   cmpi    cr5,0, bits,0                         //    } while (bits);
   bne-    cr5,@OLoop                            // }

@RETURN
   mr      rTmp1, dmob                           // return (player == white ? dmob : -dmob);
   cmpi    cr5,0, rPlayer,white
   lwz     dmob, -4(SP)
   lwz     dm, -8(SP)
   beqlr   cr5
   neg     rTmp1, rTmp1
   blr

   #undef sq2
   #undef sq4
   #undef dir2
   #undef dir4
   #undef Asq
   #undef Bsq
   #undef At
   #undef bits
   #undef BD
   #undef dA
   #undef B
   #undef Ax

   #undef dmob
   #undef dm
} /* UpdBlockAttack */


/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION                                   */
/*                                                                                                */
/**************************************************************************************************/

// The InitAttackModule() function must be called exactly once at startup. The function resets
// various global (const) data structures which are used (read only) by all engine processes.
// Must be initialized AFTER the Board module.

static void ComputeBitMasks   (GLOBAL *Global);
static void ComputeBitTables  (GLOBAL *Global);
static void ComputeSmattMasks (GLOBAL *Global);
static void ComputeAttackDir  (GLOBAL *Global);

void InitAttackModule (GLOBAL *Global)
{
   ComputeBitMasks(Global);
   ComputeBitTables(Global);
   ComputeSmattMasks(Global);
   ComputeAttackDir(Global);
} /* InitAttackModule */


static void ComputeBitMasks (GLOBAL *Global)         // Computes the 5 bit mask tables.
{
   ATTACK_COMMON *A = &(Global->A);

   for (INT i = 0; i <= 7; i++)
   {
      A->QueenBit[i]  = 1L << i;
      A->RookBit[i]   = (i < 4  ? 0 : 1L << (i + 8));
      A->BishopBit[i] = (i >= 4 ? 0 : 1L << (i + 8));
      A->KnightBit[i] = 1L << (i + 16);
      A->RayBit[i]    = A->QueenBit[i] + A->RookBit[i] + A->BishopBit[i];
      A->DirBit[Global->B.QueenDir[i]] = A->RayBit[i];
   }
} /* ComputeBitMasks */


static void ComputeBitTables (GLOBAL *Global)       // Computes the "LowBit", "HighBit" and "NumBits" tables.
{
   ATTACK_COMMON *A = &(Global->A);

   A->LowBit[0]  = -1;
   A->HighBit[0] = 8;
   A->NumBits[0] = 0;

   for (INT bits = 1; bits < 256; bits++)
   {
      A->NumBits[bits] = 0;
      for (INT i = 7; i >= 0; i--)
         if (bits & bit(i)) A->LowBit[bits] = i, A->NumBits[bits]++;
      for (INT i = 0; i <= 7; i++)
         if (bits & bit(i)) A->HighBit[bits] = i;
      A->NumBitsB[bits] = A->NumBits[bits];

      INT j = A->LowBit[bits];

      A->BlockTab[bits].rayBits  = 0x0101 << j;
      A->BlockTab[bits].dir      = 2*Global->B.QueenDir[j];
      A->BlockTab[bits].nextBits = (bits - bit(j))*sizeof(BLOCKTAB);
      A->BlockTab[bits].padding  = 0;
   }
} /* ComputeBitTables */


static void ComputeSmattMasks (GLOBAL *Global)
{
   ATTACK_COMMON *A = &(Global->A);

   A->SmattMask[wPawn]   = A->SmattMask[bPawn]   = 0L;
   A->SmattMask[wKnight] = A->SmattMask[bKnight] = pMask;
   A->SmattMask[wBishop] = A->SmattMask[bBishop] = pMask;
   A->SmattMask[wRook]   = A->SmattMask[bRook]   = bnpMask;
   A->SmattMask[wQueen]  = A->SmattMask[bQueen]  = rbnpMask;
} /* ComputeSmattMasks */


static void ComputeAttackDir (GLOBAL *Global)
{
   ATTACK_COMMON *A = &(Global->A);

   for (SQUARE sq = -h8; sq <= h8; sq++)
      A->AttackDir[sq] = 0;

   for (INT i = 0; i < 8; i++)
   {
      SQUARE dir = Global->B.QueenDir[i];
      for (SQUARE sq = dir, j = 1; j <= 7; sq += dir, j++)
         A->AttackDir[sq] = (dir << 5) | (i < 4 ? bDirMask : rDirMask);
      A->AttackDir[Global->B.KnightDir[i]] = nDirMask;
   }

   A->AttackDir[0x11]  |= wPawnDirMask; A->AttackDir[0xF]  |= wPawnDirMask;
   A->AttackDir[-0x11] |= bPawnDirMask; A->AttackDir[-0xF] |= bPawnDirMask;

   A->AttackDirMask[wQueen]  = qDirMask;
   A->AttackDirMask[wRook]   = rDirMask;
   A->AttackDirMask[wBishop] = bDirMask;
   A->AttackDirMask[wKnight] = nDirMask;
   A->AttackDirMask[wPawn]   = wPawnDirMask;

   A->AttackDirMask[bQueen]  = qDirMask;
   A->AttackDirMask[bRook]   = rDirMask;
   A->AttackDirMask[bBishop] = bDirMask;
   A->AttackDirMask[bKnight] = nDirMask;
   A->AttackDirMask[bPawn]   = bPawnDirMask;
} /* ComputeAttackDir */
