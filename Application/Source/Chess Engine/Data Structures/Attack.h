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

#pragma once

#include "Board.h"
#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- ATTACK Masks
 * -----------------------------------------*/

#define pMask 0x06000000
#define nMask 0x00FF0000
#define bMask 0x00000F00
#define bnMask 0x00FF0F00
#define bnpMask 0x06FF0F00
#define rMask 0x0000F000
#define rbMask 0x0000FF00
#define rbnMask 0x00FFFF00
#define rbnpMask 0x06FFFF00
#define qMask 0x000000FF
#define qrbMask 0x0000FFFF
#define qrbnMask 0x00FFFFFF
#define qrbnpMask 0x06FFFFFF
#define qbMask 0x00000FFF
#define qrMask 0x0000F0FF
#define kMask 0x01000000
#define pMaskL 0x04000000
#define pMaskR 0x02000000

/*----------------------------------------- Direction Masks
 * --------------------------------------*/

#define qDirMask 0x18
#define rDirMask 0x10
#define bDirMask 0x08
#define nDirMask 0x04
#define wPawnDirMask 0x02
#define bPawnDirMask 0x01

#define wForwardMask 0x2020
#define bForwardMask 0x1010

/*------------------------------------------ Misc Macros
 * -----------------------------------------*/

#define nBits(a) (((a) >> 16) & 0x00FF)
#define bBits(a) (((a) >> 8) & 0x000F)
#define rBits(a) (((a) >> 8) & 0x00F0)
#define qBits(a) ((a)&0x00FF)
#define qrbBits(a) ((((a) >> 8) | (a)) & 0x00FF)

/*---------------------------------------- Assembler Macros
 * --------------------------------------*/

#define upd_knight_attack(Asq) \
  lhz rTmp6, -56(Asq);         \
  lhz rTmp7, -72(Asq);         \
  lhz rTmp8, -124(Asq);        \
  lhz rTmp9, -132(Asq);        \
  xori rTmp6, rTmp6, 0x0001;   \
  xori rTmp7, rTmp7, 0x0002;   \
  xori rTmp8, rTmp8, 0x0004;   \
  xori rTmp9, rTmp9, 0x0008;   \
  sth rTmp6, -56(Asq);         \
  sth rTmp7, -72(Asq);         \
  sth rTmp8, -124(Asq);        \
  sth rTmp9, -132(Asq);        \
  lhz rTmp6, 72(Asq);          \
  lhz rTmp7, 56(Asq);          \
  lhz rTmp8, 132(Asq);         \
  lhz rTmp9, 124(Asq);         \
  xori rTmp6, rTmp6, 0x0010;   \
  xori rTmp7, rTmp7, 0x0020;   \
  xori rTmp8, rTmp8, 0x0040;   \
  xori rTmp9, rTmp9, 0x0080;   \
  sth rTmp6, 72(Asq);          \
  sth rTmp7, 56(Asq);          \
  sth rTmp8, 132(Asq);         \
  sth rTmp9, 124(Asq)

#define upd_king_attack(Asq) \
  lhz rTmp6, -4(Asq);        \
  lhz rTmp7, 4(Asq);         \
  lhz rTmp8, -64(Asq);       \
  lhz rTmp9, 64(Asq);        \
  xori rTmp6, rTmp6, 0x0100; \
  xori rTmp7, rTmp7, 0x0100; \
  xori rTmp8, rTmp8, 0x0100; \
  xori rTmp9, rTmp9, 0x0100; \
  sth rTmp6, -4(Asq);        \
  sth rTmp7, 4(Asq);         \
  sth rTmp8, -64(Asq);       \
  sth rTmp9, 64(Asq);        \
  lhz rTmp6, -60(Asq);       \
  lhz rTmp7, 60(Asq);        \
  lhz rTmp8, -68(Asq);       \
  lhz rTmp9, 68(Asq);        \
  xori rTmp6, rTmp6, 0x0100; \
  xori rTmp7, rTmp7, 0x0100; \
  xori rTmp8, rTmp8, 0x0100; \
  xori rTmp9, rTmp9, 0x0100; \
  sth rTmp6, -60(Asq);       \
  sth rTmp7, 60(Asq);        \
  sth rTmp8, -68(Asq);       \
  sth rTmp9, 68(Asq)

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------------- The ATTACK Type
 * ---------------------------------------*/

typedef ULONG
    ATTACK;  // Indicates directions attack of a single square (for one colour).
             //
             //    Bit  0 : Queen down, right
             //    Bit  1 : Queen down, left
             //    Bit  2 : Queen up,   right
             //    Bit  3 : Queen up,   left
             //    Bit  4 : Queen down
             //    Bit  5 : Queen up
             //    Bit  6 : Queen right
             //    Bit  7 : Queen left
             //
             //    Bit  8 : Bishop down, right
             //    Bit  9 : Bishop down, left
             //    Bit 10 : Bishop up,   right
             //    Bit 11 : Bishop up,   left
             //
             //    Bit 12 : Rook down
             //    Bit 13 : Rook up
             //    Bit 14 : Rook right
             //    Bit 15 : Rook left
             //
             //    Bit 16 : Knight down 1, right 2
             //    Bit 17 : Knight down 1, left  2
             //    Bit 18 : Knight down 2, right 1
             //    Bit 19 : Knight down 2, left  1
             //    Bit 20 : Knight up   1, right 2
             //    Bit 21 : Knight up   1, left  2
             //    Bit 22 : Knight up   2, right 1
             //    Bit 23 : Knight up   2, left  1
             //
             //    Bit 24 : King (no direction indication)
             //    Bit 25 : Pawn right, up/down
             //    Bit 26 : Pawn left, up/down

/*---------------------------------- ATTACK_STATE for Current Node
 * -------------------------------*/
// The ATTACK_STATE structure contains two attack tables; one for each player.

typedef struct {
  ATTACK AttackW_[boardSize1], AttackW[boardSize2];
  ATTACK AttackB_[boardSize1], AttackB[boardSize2];
} ATTACK_STATE;

/*--------------------------------- Global Read-only Utility Tables
 * ------------------------------*/
/*
typedef struct
{
   INT   lowBit;
   INT   offset;
} BITFILT;
*/

typedef struct {
  SQUARE dir;
  INT rayBits;
  INT nextBits;
  INT padding;
} BLOCKTAB;

typedef struct {
  ATTACK QueenBit[8],  // Direction sensitive bit masks for the various pieces.
      RookBit[8], BishopBit[8], KnightBit[8], RayBit[8], DirBit_[17],
      DirBit[18];

  ATTACK SmattMask[pieces];  // Holds for each piece a smaller attacker mask.

  INT LowBit[256],   // Indicates the least significant 1 bit in any byte.
      HighBit[256],  // Indicates the most significant 1 bit in any byte.
      NumBits[256];  // Indicates number of 1 bits in any byte.

  BYTE NumBitsB[256];  // Indicates number of 1 bits in any byte.

  INT AttackDir_[119],  // AttackDir[sq - sq0] indicates direction and type of
      AttackDir[120],   // attack from square sq0 to sq.
      AttackDirMask[pieces];

  // BITFILT  BitFilter[256];            // Table used to extract bit numbers of
  // all set bits in any byte one by one.

  BLOCKTAB BlockTab[256];  // Table facilitating updating of block attack.
} ATTACK_COMMON;
