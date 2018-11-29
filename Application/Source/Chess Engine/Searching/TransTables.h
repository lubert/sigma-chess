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

#pragma once

#include "HashCode.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define trans_MinSize  (10*4*(1L << 11))

// Transposition record layout (10 bytes):
//
// ¥ BYTE 0..3 : HKEY + flags (and m.piece & m.cap)
//    Bit 0..20  (31..9) : The "rest" of hash key, used after indexing/lookup to verify hashing.
//    Bit 21     (8)     : Shallow quiescence?
//    Bit 22     (7)     : Is it a cutoff (fail high) score?
//    Bit 23     (8)     : Is it a true score?
//    Bit 24..25 (7..6)  : rfm.dply (0..2)
//    Bit 26..28 (5..3)  : pieceType(rfm.cap)
//    Bit 29..31 (2..0)  : pieceType(rfm.piece)
//
// ¥ BYTE 4 : ply of stored position
//
// ¥ BYTE 5 : maxPly of stored position
//
// ¥ BYTE 6..7 : score of stored position
//
// ¥ BYTE 8 : from square of stored move (bits 3 and 7 currently unused)
//
// ¥ BYTE 9 : to square of stored move (bits 3 and 7 currently unused)
//
// NOTE 1: Since the low order 11 bits do NOT contain the hash key but is instead used for various
// flags, each of the 4 transposition tables must contain at least 2^11 = 2048 entries.
// 
// NOTE 2: Only NORMAL moves are stored (i.e. not enpassant, castling or promotions).

//--- TRANS.key masks ---
// The low order 11 bits of the contain various flags as well as m.piece and m.cap. Note that we
// The "trans_HashLockMask" masks out the part of the hashkey that's stored in the transposition
// table entry (to handle collisions).

#define trans_HashLockMask     0xFFFFF800  // Bits 0..20
#define trans_ShQuiesBit           0x0400  // Bit  21
#define trans_CutoffBit            0x0200  // Bit  22
#define trans_TrueScoreBit         0x0100  // Bit  23
#define trans_DPlyMask             0x00C0  // Bit  24..25
#define trans_CapMask              0x0038  // Bit  26..28
#define trans_PieceMask            0x0007  // Bit  29..31


/**************************************************************************************************/
/*                                                                                                */
/*                                        TYPE DEFINITIONS                                        */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------- Transposition Table Entries ----------------------------------*/

typedef struct   /* 10 bytes */
{
   BYTE data[10];
/*
   HKEY key;
   BYTE ply, maxPly;
   INT  score;
   BYTE from, to;
*/
} TRANS;

/*--------------------------------- Transposition Table Pointers ---------------------------------*/

typedef struct
{
   BOOL  transTabOn;                          // Use transposition tables?

   LONG  transSize;                           // Total number of ENTRIES in all 4 tables. Must
                                              // be a power of 2 and at least 8192 (= 4*2^11).
   HKEY  hashIndexMask;                       // Bit mask masking out the low order index bits
                                              // (³ 11 bits). Is equal to (transSize/4) - 1.
   LONG  transUsed;                           // Number of used transposition entries
   TRANS *TransTab1W, *TransTab1B;            // The transposition tables which are indexed by
   TRANS *TransTab2W, *TransTab2B;            // (the loworder part of) the hash key. In table 1,
                                              // entries are only overridden if the "ply"-field
                                              // is lower. Entries are always overridden in table
                                              // 2. Both tables are divided in two parts: one for
                                              // each colour (side to move).
} TRANS_STATE;
