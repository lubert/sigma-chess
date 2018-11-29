/**************************************************************************************************/
/*                                                                                                */
/* Module  : BOARD.C */
/* Purpose : This module is the base module in the engine, containing the board
 * structures which  */
/*           are updated incrementally during the search reflecting the current
 * board state at    */
/*           any point during the search. */
/*           Before the search starts, these board structures are computed from
 * scratch based on  */
/*           current board state as described in the GAME structure. */
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

#include "Engine.h"

#include "Board.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                    COMPUTE THE BOARD DATA */
/*                                                                                                */
/**************************************************************************************************/

void NewBoard(PIECE Board[]) {
  ClearTable(Board);  // First remove all existing pieces.

  Board[a1] = wRook;  // Set up white officers
  Board[b1] = wKnight;
  Board[c1] = wBishop;
  Board[d1] = wQueen;
  Board[e1] = wKing;
  Board[f1] = wBishop;
  Board[g1] = wKnight;
  Board[h1] = wRook;

  for (SQUARE sq = a1; sq <= h1; sq++) {
    Board[sq + 0x70] = black + Board[sq];  // Setup black officers.
    Board[sq + 0x10] = wPawn;              // Setup white pawns.
    Board[sq + 0x60] = bPawn;              // Setup black pawns.
  }
} /* NewBoard */

// The "CalcBoardState()" routine is called by the engine before starting the
// search. The routine computes various board related structures (the
// ENGINE.BOARD_STATE record) based on the board information specified in the
// search parameters (ENGINE.PARAM).

static void CalcPieceLoc(GLOBAL *G, BOARD_STATE *B);
static void SortPieceLoc(BOARD_STATE *B, COLOUR c);
static void CalcPLinx(BOARD_STATE *B);
static void ClearPawnStruct(BOARD_STATE *B);

void CalcBoardState(ENGINE *E) {
  PARAM *P = &E->P;
  BOARD_STATE *B = &E->B;

  CopyTable(P->Board, B->Board);  // First copy Board[], HasMovedTo[] and player
  CopyTable(P->HasMovedTo, B->HasMovedTo);  // information.
  B->player = P->player;
  B->opponent = black - B->player;

  CalcPieceLoc(E->Global, B);  // Next build piece location data and indexes
  CalcPLinx(B);                // as well as piece count information.
  ClearPawnStruct(B);          // Build pawn structure information.
  // B->pieceCount = CalcPieceCount(B->Board);  // Finally compute piece count.
} /* CalcBoardState */

static void CalcPieceLoc(
    GLOBAL *G, BOARD_STATE *B)  // Builds the "PieceLoc" table from the current
{  // Board configuration and computes "pieceCount".
  for (INDEX i = 0; i < 32; i++)  // Clear all entries.
    B->PieceLoc[i] = nullSq;
  B->pieceCount = 0;  // Reset "pieceCount".

  B->LastPiece[white] = B->LastPiece[black] = -1;
  for (SQUARE sq = a1; sq <= h8; sq++)  // Insert the pieces from the board into
    if (onBoard(sq) && B->Board[sq])    // "PieceLoc" and compute "LastPiece".
    {
      COLOUR c = pieceColour(B->Board[sq]);
      B->LastPiece[c]++;
      B->PieceLoc[c + B->LastPiece[c]] = sq;
      B->pieceCount +=
          G->B.PieceCountBit[B->Board[sq]];  // Update "pieceCount".
    }

  SortPieceLoc(B, white);  // Sort the "PieceLoc" entries into
  SortPieceLoc(B, black);  // descending order of piece values.

  for (COLOUR c = white; c <= black; c += 0x10)  // Compute "LastOffi".
  {
    INDEX i;
    for (i = 1; i <= B->LastPiece[c] &&
                pieceType(B->Board[B->PieceLoc[c + i]]) != pawn;
         i++)
      ;
    B->LastOffi[c] = i - 1;
  }
} /* CalcPieceLoc */

// Sorts the "PieceLoc[c]" table into descending order of piece value: KQRBNP.
// Additionally centralized pawns are put before other pawns.

static void SortPieceLoc(BOARD_STATE *B, COLOUR c) {
  for (INDEX i = 0; i < B->LastPiece[c]; i++)
    for (INDEX j = i + 1; j <= B->LastPiece[c]; j++) {
      SQUARE *sq1 = &B->PieceLoc[c + j];
      SQUARE *sq2 = &B->PieceLoc[c + i];
      PIECE p1 = B->Board[*sq1];
      PIECE p2 = B->Board[*sq2];

      if (p1 > p2)
        Swap(sq1, sq2);
      else if (p2 == c + pawn && Abs(file(*sq1) - 3) <= Abs(file(*sq2) - 3))
        Swap(sq1, sq2);
    }
} /* SortPieceLoc */

static void CalcPLinx(
    BOARD_STATE *B)  // Computes the PLinx table from the "PieceLoc"
{                    // table.
  for (COLOUR c = white; c <= black; c += 0x10)
    for (INDEX i = 0; i <= B->LastPiece[c]; i++)
      B->PLinx[B->PieceLoc[c + i]] = i;
} /* CalcPLinx */

static void ClearPawnStruct(
    BOARD_STATE *B)  // This routine should merely clear the PawnStruct.
{                    // The "UpdPieceAttack" routine will also update
  for (INT r = 0; r <= 7; r++)  // the pawn structure.
    B->PawnStructW[r] = B->PawnStructB[r] = 0;
} /* ClearPawnStruct */

ULONG CalcPieceCount(
    GLOBAL *Global,
    PIECE Board[])  // Builds and returns the pieceCount from scratch
{                   // from the specified Board[].
  ULONG pieceCount = 0;

  for (SQUARE sq = a1; sq <= h8; sq++)
    if (onBoard(sq) && Board[sq])
      pieceCount += Global->B.PieceCountBit[Board[sq]];

  return pieceCount;
} /* CalcPieceCount */

/**************************************************************************************************/
/*                                                                                                */
/*                                          BOARD UTILITY */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Miscellaneous
 * ----------------------------------------*/

void ClearTable(INT T[])  // Clears the (board sized) table "T".
{
  SQUARE sq;

  for (sq = a1; sq <= h8; sq++)
    if (onBoard(sq)) T[sq] = 0;
} /* ClearTable */

void CopyTable(INT Source[], INT Dest[])  // Copies the (board sized) table
{             // "Source" to "Dest". Edge squares are
  SQUARE sq;  // not copied.

  for (sq = a1; sq <= h8; sq++)
    if (onBoard(sq)) Dest[sq] = Source[sq];
} /* CopyTable */

BOOL EqualTable(INT T1[], INT T2[])  // Are the two board sized tables "T1"
{                                    // and "T2" identical?
  SQUARE sq;

  for (sq = a1; sq <= h8; sq++)
    if (onBoard(sq) && T1[sq] != T2[sq]) return false;
  return true;
} /* EqualTable */

/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

void InitBoardState(
    BOARD_STATE *B)  // Must be called before using board data the first time.
{
  SQUARE sq;

  for (sq = -boardSize1; sq < boardSize2; sq++) B->Board[sq] = edge;
  ClearTable(B->Board);

  B->PieceLocW = &B->PieceLoc[white];
  B->PieceLocB = &B->PieceLoc[black];
} /* InitBoardState */

void InitBoardModule(GLOBAL *Global) {
  BOARD_COMMON *B = &(Global->B);

  B->KingDir[0] = B->QueenDir[0] = B->BishopDir[0] = -0x0F;
  B->KingDir[1] = B->QueenDir[1] = B->BishopDir[1] = -0x11;
  B->KingDir[2] = B->QueenDir[2] = B->BishopDir[2] = +0x11;
  B->KingDir[3] = B->QueenDir[3] = B->BishopDir[3] = +0x0F;
  B->KingDir[4] = B->QueenDir[4] = B->RookDir[0] = -0x10;
  B->KingDir[5] = B->QueenDir[5] = B->RookDir[1] = +0x10;
  B->KingDir[6] = B->QueenDir[6] = B->RookDir[2] = +0x01;
  B->KingDir[7] = B->QueenDir[7] = B->RookDir[3] = -0x01;
  B->KingDir[8] = B->QueenDir[8] = B->RookDir[4] = B->BishopDir[4] = 0;

  B->KnightDir[0] = -0x0E;
  B->KnightDir[1] = -0x12;
  B->KnightDir[2] = -0x1F;
  B->KnightDir[3] = -0x21;
  B->KnightDir[4] = +0x12;
  B->KnightDir[5] = +0x0E;
  B->KnightDir[6] = +0x21;
  B->KnightDir[7] = +0x1F;
  B->KnightDir[8] = 0;

  B->Turn90[0x01] = 0x10;
  B->Turn90[0x10] = -0x01;
  B->Turn90[-0x01] = -0x10;
  B->Turn90[-0x10] = 0x01;
  B->Turn90[0x11] = 0x0F;
  B->Turn90[0x0F] = -0x11;
  B->Turn90[-0x11] = -0x0F;
  B->Turn90[-0x0F] = 0x11;

  B->PieceCountBit[empty] = 0;
  B->PieceCountBit[wPawn] = 0x00000001;
  B->PieceCountBit[wKnight] = 0x00000110;
  B->PieceCountBit[wBishop] = 0x00000100;
  B->PieceCountBit[wRook] = 0x00001000;
  B->PieceCountBit[wQueen] = 0x00001000;
  B->PieceCountBit[wKing] = 0;
  B->PieceCountBit[bPawn] = 0x00010000;
  B->PieceCountBit[bKnight] = 0x01100000;
  B->PieceCountBit[bBishop] = 0x01000000;
  B->PieceCountBit[bRook] = 0x10000000;
  B->PieceCountBit[bQueen] = 0x10000000;
  B->PieceCountBit[bKing] = 0;

  B->Rank2[white] = 1;
  B->Rank2[black] = 6;
  B->Rank7[white] = 6;
  B->Rank7[black] = 1;

  B->Mtrl[wPawn] = B->Mtrl[bPawn] = pawnMtrl;
  B->Mtrl[wKnight] = B->Mtrl[bKnight] = knightMtrl;
  B->Mtrl[wBishop] = B->Mtrl[bBishop] = bishopMtrl;
  B->Mtrl[wRook] = B->Mtrl[bRook] = rookMtrl;
  B->Mtrl[wQueen] = B->Mtrl[bQueen] = queenMtrl;
  B->Mtrl[wKing] = B->Mtrl[bKing] = kingMtrl;

  for (PIECE p = wPawn; p <= bKing; p++) B->Mtrl100[p] = 100 * B->Mtrl[p];
} /* InitBoardModule */
