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

#pragma once

#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------- Piece Constant Definitions
 * ----------------------------------*/

enum PIECES { empty, pawn, knight, bishop, rook, queen, king, edge = -1 };
enum WHITE_PIECES { wPawn = 0x01, wKnight, wBishop, wRook, wQueen, wKing };
enum BLACK_PIECES { bPawn = 0x11, bKnight, bBishop, bRook, bQueen, bKing };

#define pieces 0x17
#define white 0
#define black 0x10
#define white_black 0x11

#define pieceColour(p) ((p)&0x10)
#define pieceType(p) ((p)&0x07)

#define isWhiteOffi(p) (((p) ^ 6) < 6)
#define isBlackOffi(p) ((p) > bPawn)

#define nullSq -1

/*--------------------------- Square & Board Constant/Macro Definitions
 * --------------------------*/

enum SQUARES {
  a8 = 0x70,
  b8,
  c8,
  d8,
  e8,
  f8,
  g8,
  h8,
  a7 = 0x60,
  b7,
  c7,
  d7,
  e7,
  f7,
  g7,
  h7,
  a6 = 0x50,
  b6,
  c6,
  d6,
  e6,
  f6,
  g6,
  h6,
  a5 = 0x40,
  b5,
  c5,
  d5,
  e5,
  f5,
  g5,
  h5,
  a4 = 0x30,
  b4,
  c4,
  d4,
  e4,
  f4,
  g4,
  h4,
  a3 = 0x20,
  b3,
  c3,
  d3,
  e3,
  f3,
  g3,
  h3,
  a2 = 0x10,
  b2,
  c2,
  d2,
  e2,
  f2,
  g2,
  h2,
  a1 = 0x00,
  b1,
  c1,
  d1,
  e1,
  f1,
  g1,
  h1
};

#define boardSize 0x78
#define boardSizeE 0xBA
#define boardSize1 34  // Number of squares allocated BEFORE start of Board (a1)
#define boardSize2 154  // Number of squares allocated AFTER start of Board (a1)

#define square(f, r) (((r) << 4) + (f))

#define offBoard(sq) ((sq)&0x88)
#define onBoard(sq) (!offBoard(sq))
//#define isEmpty(sq)      (Board[sq] == empty)
//#define isOccupied(sq)   (Board[sq])

#define front(sq) ((sq) + 0x10)
#define behind(sq) ((sq)-0x10)
#define left(sq) ((sq)-1)
#define right(sq) ((sq) + 1)
#define left2(sq) ((sq)-2)
#define right2(sq) ((sq) + 2)

#define file(sq) ((sq)&0x7)
#define rank(sq) ((sq) >> 4)
#define onRank8(sq) ((sq) >= 0x70 || (sq) < 0x10)

#define wing(sq) (((sq)&0x04) >> 2)

#define kingLocW(E) (E->B.PieceLocW[0])
#define kingLocB(E) (E->B.PieceLocB[0])
#define kingLoc(E, c) (E->B.PieceLoc[c])

/*------------------------------------- Main Material Values
 * -------------------------------------*/

#define pawnMtrl 1
#define knightMtrl 3
#define bishopMtrl 3
#define rookMtrl 5
#define queenMtrl 9
#define kingMtrl 0

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- Misc Type Definitions
 * ----------------------------------*/

typedef INT SQUARE;
typedef INT PIECE;
typedef INT INDEX;
typedef INT COLOUR;
typedef BYTE RANKBITS;

/*------------------------------------ BOARD_STATE Data Structure
 * --------------------------------*/

typedef struct {
  /* - - - - - - - - - - - - - - - - - The Board Configuration - - - - - - - - -
   * - - - - - - - - */
  // The "Board", "HasMovedTo" and "player" structures below provide - along
  // with the game record "Game" (defined in "Game.c") - ALL necessary
  // information about the state of the current game.

  PIECE Board_[boardSize1],  // The main board "Board[-34... 154]". Indexing is
                             // done as
      Board[boardSize2];  // follows: 0x<rank-1><file-1>, e.g. 0x00 = a1, 0x35 =
                          // f4, e.t.c. (see "Board.h").

  INT HasMovedTo[boardSize];  // Indicates how often a piece has moved to a
                              // given square and thus indicates castling
                              // rights. Is incrementally updated every time a
                              // move is performed (or retracted). NOTE: Must be
                              // located here after Board[].

  COLOUR player,  // Indicates the side to move in the current position###
                  // Needed???
      opponent;   // Indicates analogously the side not to move### Needed???

  /* - - - - - - - - - - - - - - - - - The Piece Location Table  - - - - - - - -
   * - - - - - - - - */

  SQUARE
      PieceLoc[32],  // The piece table "PieceLoc[white..black][0..15]". Holds
      *PieceLocW,    // for each colour and piece the location of the piece. The
      *PieceLocB;    // kings are located first, followed by the officers (in
                     // descending order of value) and followed by the pawns.
                     // When a piece is captured, its entry is set to "nullSq".

  INDEX LastOffi[white_black],  // "LastOffi[white..black]",
                                // "LastPiece[white..black]".
      LastPiece[white_black];   // Indicates the last officer and piece for both
                               // white and black. Facilitates "PieceLoc"
                               // lookup. Are calculated at the root of the
                               // search tree - "LastOffi"   is changed
                               // (incremented/decremented) during the search in
                               // case of pawn promotions, where the promoted
                               // pawn and the first pawn in the pawn list are
                               // swapped.

  INDEX
      PLinx[boardSize];  // The piece location index "PLinx[a1..h8]". Indicates
                         // for each occupied square the corresponding index in
                         // the piece location table "PieceLoc". "PLinx" is
                         // undefined for empty squares.

  /* - - - - - - - - - - - - - - - - - - - Miscellaneous - - - - - - - - - - - -
   * - - - - - - - - */

  RANKBITS
      PawnStructW[8],  // Pawn structure for white and black. Indicates for each
      PawnStructB[8];  // rank the pawns on that rank as a bit list, where a set
                       // bit at position "i" indicates that a pawn is standing
                       // on file "i". Is updated incrementally during the
                       // search.

  ULONG pieceCount;  // Count of pieces of each type and colour. Is updated
                     // incrementally each time a capture/promotion is performed
                     // or retracted and is used to check draw by insufficient
                     // mating material and to recognize certain end games (e.g.
                     // KQK, KRK, KBNK, KPK e.t.c.) during the search. Bit
                     // format: QRQR BNBN NNNN PPPP (black : high order word,
                     // white : low order word).
} BOARD_STATE;

/*------------------------------------ BOARD_COMMON Data Structure
 * -------------------------------*/

typedef struct {
  SQUARE KingDir[10];  // Piece directions on board (null terminated.
  SQUARE QueenDir[10];
  SQUARE RookDir[6];
  SQUARE BishopDir[6];
  SQUARE KnightDir[10];

  SQUARE Turn90_[18];
  SQUARE Turn90[18];

  ULONG
      PieceCountBit[pieces];  // Increment bits for each piece type and colour.

  INT Rank2[white_black];  // "RankX[white..black]".
  INT Rank7[white_black];

  INT Mtrl[pieces];
  INT Mtrl100[pieces];
} BOARD_COMMON;
