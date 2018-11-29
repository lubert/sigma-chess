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

//--- Move types (MOVE.type):

#define mtype_Normal 0x0000  // Normal moves
#define mtype_Promotion \
  0x001F                    // Bit 0..4 : Promotion piece (colour + piecetype)
#define mtype_O_O 0x0020    // Bit 5    : King sides castling
#define mtype_O_O_O 0x0040  // Bit 6    : Queen sides castling
#define mtype_EP 0x0080     // Bit 7    : En passant
#define mtype_Null 0x0100   // Bit 8    : Null moves

#define isPromotion(m) ((m).type & mtype_Promotion)
#define irreversible(m) ((m).cap || pieceType((m).piece) == pawn)

//--- Null moves are identified by a blank piece field:

#define clrMove(m) ((m).piece = empty)
#define isNull(m) ((m).piece == empty)

//--- Game flags for each move (MOVE.flags):

#define moveFlag_Check 0x01  // Bit 0    : Does move give check? (bit 3 in �4)
#define moveFlag_Mate \
  0x02  // Bit 1    : Does move give check mate? (bit 4 in �4)
#define moveFlag_ShowFile \
  0x04  // Bit 2    : Must from file be indicated? (bit 5 in �4)
#define moveFlag_ShowRank \
  0x08  // Bit 3    : Must from rank be indicated? (bit 6 in �4)

#define moveFlag_DescrFrom \
  0x30  // Bit 4..5 : Source square disambiguation 0..3 (descriptive notation)
#define moveFlag_DescrTo \
  0xC0  // Bit 6..7 : Dest square disambiguation 0..3 (descriptive notation)

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef struct  // 16 bytes
{
  PIECE piece;  // Moving piece (king if castling).
  SQUARE from;  // Origin square (king origin if castling).
  SQUARE to;    // Destination square (king destination if castling).
  PIECE cap;    // Captured piece (empty if non-capture or en passant).
  INT type;     // Move type (normal, en passant, castling, promotion).

  SQUARE
      dir;   // Direction of movement for Queen/Rook/Bishop (engine moves only).
  INT dply;  // Ply decrementer (engine moves only).

  BYTE flags;  // Source/dest disambiguation (game moves only) and Check/mate
               // flag.
  BYTE misc;   // Glyph (game moves), or move generator (engine moves).
} MOVE;

typedef UINT
    PMOVE;  // Packed move format (2 bytes). The upper byte holds the origin
            // square and lower byte holds the destination, except that for
            // promotions the destination rank (bits 4-6) holds the promotion
            // piece. This format is used in opening libraries and the old
            // version 4.0 game file format:
            //
            // 15    : Sibling bit [LIB], i.e. does move have a sibling?
            // 14-12 : Origin rank (0..7).
            // 11    : Childless bit [LIB], i.e. is move childless?
            // 10-8  : Origin file (0..7).
            // 7     : Unplayable bit [LIB], i.e. is move unplayable?
            // 6-4   : Destination rank (0..7), or target piece for promotions.
            // 3     : Extra data bit [LIB], i.e. does move have extra data?
            // 2-0   : Destination file (0..7).
