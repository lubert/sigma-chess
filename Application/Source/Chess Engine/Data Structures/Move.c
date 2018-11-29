/**************************************************************************************************/
/*                                                                                                */
/* Module  : MOVE.C */
/* Purpose : This module defines the MOVE record and some basic operations. */
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

#include "Move.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                      PACKING/UNPACKING MOVES */
/*                                                                                                */
/**************************************************************************************************/

// These routines convert between the normal 16 byte MOVE format and the packed
// 2 byte PMOVE format used in the opening libraries and in saved games. In the
// packed format, the upper byte holds the origin square, and the lower byte
// holds the destination, except that for promotions the destination rank (bits
// 4-6) holds the promotion piece. Note that bits 3, 7, 11 and 15 are reserved
// for use by the opening library.

PMOVE Move_Pack(MOVE *m)  // Packs an ordinary 16 byte MOVE to a 2 byte move.
{
  PMOVE pm = (m->from << 8) | m->to;
  return (isPromotion(*m) ? (pm & 0x7707) | ((m->type & 0x07) << 4) : pm);
} /* Move_Pack */

void Move_Unpack(PMOVE pm, PIECE Board[],
                 MOVE *m)  // Unpacks a 2 byte move to an ordinary 16 byte move
{                          // on the assumption that the move to be unpacked
  m->from = (pm >> 8) & 0x77;  // "belongs" to the current board position.
  m->piece = Board[m->from];

  if (m->piece == wPawn && rank(m->from) == 6)  // White pawn promotion
  {
    m->to = 0x70 + (pm & 0x07);
    m->type = white + ((pm & 0x70) >> 4);
  } else if (m->piece == bPawn && rank(m->from) == 1)  // Black pawn promotion
  {
    m->to = 0x00 + (pm & 0x07);
    m->type = black + ((pm & 0x70) >> 4);
  } else  // Non-promotion move
  {
    m->to = pm & 0x77;
    m->type = mtype_Normal;
  }

  m->cap = Board[m->to];

  switch (pieceType(m->piece)) {
    case king:
      switch (m->to - m->from) {
        case 2:
          m->type = mtype_O_O;
          break;
        case -2:
          m->type = mtype_O_O_O;
      }
      break;
    case pawn:
      if (!m->cap && file(m->from) != file(m->to)) m->type = mtype_EP;
  }
} /* Move_Unpack */

void Move_Perform(PIECE Board[], MOVE *m) {
  Board[m->from] = empty;
  Board[m->to] = m->piece;

  switch (m->type) {
    case mtype_Normal:
      break;
    case mtype_O_O:
      Board[right(m->to)] = empty;
      Board[left(m->to)] = pieceColour(m->piece) + rook;
      break;
    case mtype_O_O_O:
      Board[left2(m->to)] = empty;
      Board[right(m->to)] = pieceColour(m->piece) + rook;
      break;
    case mtype_EP:
      Board[m->to + 2 * pieceColour(m->piece) - 0x10] = empty;
      break;
    default:
      Board[m->to] = m->type;
  }
} /* Move_Perform */

void Move_Retract(PIECE Board[], MOVE *m) {
  switch (m->type)  // Retract move on the board.
  {
    case mtype_O_O:
      Board[left(m->to)] = empty;
      Board[right(m->to)] = pieceColour(m->piece) + rook;
      break;
    case mtype_O_O_O:
      Board[right(m->to)] = empty;
      Board[left2(m->to)] = pieceColour(m->piece) + rook;
      break;
    case mtype_EP:
      Board[m->to + 2 * pieceColour(m->piece) - 0x10] =
          pawn + black - pieceColour(m->piece);
      break;
  }

  Board[m->from] = m->piece;
  Board[m->to] = m->cap;
} /* Move_Retract */

BOOL EqualMove(MOVE *m1, MOVE *m2) {
  return (m1->from == m2->from && m1->to == m2->to && m1->piece == m2->piece &&
          m1->cap == m2->cap && m1->type == m2->type);
} /* EqualMove */
