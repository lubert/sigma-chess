/**************************************************************************************************/
/*                                                                                                */
/* Module  : ENDGAMEDB.C */
/* Purpose : This module implements the endgame database table access. */
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

#include "EndGameDB.f"
#include "Engine.f"

#define nonWinVal 61

/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSULT ENDGAME DATABASES */
/*                                                                                                */
/**************************************************************************************************/

// ConsultEndgameDB is the main end game database consulting routine. It is
// called by the search engine at the ply 1 node (first iteration only). If the
// position is found in the database, then RootNode->val is updated accordingly
// (indicating the number of moves) and true is returned. Otherwise false is
// returned (in which case the engine should analyze the position itself).

asm ULONG asmPieceCount(void);

static BOOL Consult_KxKy(ENGINE *E, CHAR *edbName, SQUARE wK, SQUARE wX,
                         SQUARE bK, SQUARE bX, COLOUR player);

BOOL ConsultEndGameDB(ENGINE *E) {
  if ((asmPieceCount() & 0xECCFECCF) || E->S.mainDepth > 1 ||
      !E->S.edbPresent || !E->P.useEndgameDB ||
      E->P.playingMode == mode_Novice || E->P.playingMode == mode_Mate)
    return false;

  NODE *N = E->S.rootNode;
  PIECE *Board = E->B.Board;
  SQUARE *PieceLocW = E->B.PieceLocW;
  SQUARE *PieceLocB = E->B.PieceLocB;
  SQUARE wK = PieceLocW[0];
  SQUARE bK = PieceLocB[0];
  SQUARE wX =
      PieceLocW[E->B.PieceLocW[1] != nullSq ? 1 : 2];  // In case last move was
                                                       // a capture
  SQUARE bX = PieceLocB[E->B.PieceLocB[1] != nullSq ? 1 : 2];
  SQUARE xN, xB;
  INT i;

  switch (asmPieceCount()) {
    // --- KQKR ---
    case 0x10001000:
      if (Board[wX] == wQueen && Board[bX] == bRook)
        return Consult_KxKy(E, "KQKR", wK, wX, bK, bX, N[1].player);
      else if (Board[wX] == wRook && Board[bX] == bQueen)
        return Consult_KxKy(E, "KQKR", bK, bX, wK, wX, N[1].opponent);
      else
        return false;

    // --- KQK/KRK [white] ---
    case 0x00001000:
      if (Board[wX] == wQueen)
        return Consult_KxKy(E, "KQKR", wK, wX, bK, bK, N[1].player);
      else
        return E->P.proVersion &&
               Consult_KxKy(E, "KRKN", wK, wX, bK, bK, N[1].player);

    // --- KQK/KRK [black] ---
    case 0x10000000:
      if (Board[bX] == bQueen)
        return Consult_KxKy(E, "KQKR", bK, bX, wK, wK, N[1].opponent);
      else
        return E->P.proVersion &&
               Consult_KxKy(E, "KRKN", bK, bX, wK, wK, N[1].opponent);

    // --- KQKB/KRKB [white] ---
    case 0x01001000:
      if (!E->P.proVersion) return false;
      if (Board[wX] == wQueen)
        return Consult_KxKy(E, "KQKB", wK, wX, bK, bX, N[1].player);
      else
        return Consult_KxKy(E, "KRKB", wK, wX, bK, bX, N[1].player);

    // --- KQKB/KRKB [black] ---
    case 0x10000100:
      if (!E->P.proVersion) return false;
      if (Board[bX] == bQueen)
        return Consult_KxKy(E, "KQKB", bK, bX, wK, wX, N[1].opponent);
      else
        return Consult_KxKy(E, "KRKB", bK, bX, wK, wX, N[1].opponent);

    // --- KQKN/KRKN [white] ---
    case 0x01101000:
      if (!E->P.proVersion) return false;
      if (Board[wX] == wQueen)
        return Consult_KxKy(E, "KQKN", wK, wX, bK, bX, N[1].player);
      else
        return Consult_KxKy(E, "KRKN", wK, wX, bK, bX, N[1].player);

    // --- KQKN/KRKN [black] ---
    case 0x10000110:
      if (!E->P.proVersion) return false;
      if (Board[bX] == bQueen)
        return Consult_KxKy(E, "KQKN", bK, bX, wK, wX, N[1].opponent);
      else
        return Consult_KxKy(E, "KRKN", bK, bX, wK, wX, N[1].opponent);

    // --- KBNK [white] ---
    case 0x00000210:
      for (i = 1; i <= E->B.LastOffi[white]; i++)
        if (Board[PieceLocW[i]] == wBishop) xB = PieceLocW[i];
      for (i = 1; i <= E->B.LastOffi[white]; i++)
        if (Board[PieceLocW[i]] == wKnight) xN = PieceLocW[i];
      return Consult_KxKy(E, "KBNK", wK, xB, xN, bK, N[1].player);

    // --- KBNK [black] ---
    case 0x02100000:
      for (i = 1; i <= E->B.LastOffi[black]; i++)
        if (Board[PieceLocB[i]] == bBishop) xB = PieceLocB[i];
      for (i = 1; i <= E->B.LastOffi[black]; i++)
        if (Board[PieceLocB[i]] == bKnight) xN = PieceLocB[i];
      return Consult_KxKy(E, "KBNK", bK, xB, xN, wK, N[1].opponent);

    // --- KBBK [white] ---
    case 0x00000200:
      if (!E->P.proVersion) return false;
      for (i = 1; Board[PieceLocW[i]] != wBishop; i++)
        ;
      xB = PieceLocW[i++];
      for (; Board[PieceLocW[i]] != wBishop; i++)
        ;
      xN = PieceLocW[i];
      return Consult_KxKy(E, "KBBK", wK, xB, xN, bK, N[1].player);

    // --- KBBK [black] ---
    case 0x02000000:
      if (!E->P.proVersion) return false;
      for (i = 1; Board[PieceLocB[i]] != bBishop; i++)
        ;
      xB = PieceLocB[i++];
      for (; Board[PieceLocB[i]] != bBishop; i++)
        ;
      xN = PieceLocB[i];
      return Consult_KxKy(E, "KBBK", bK, xB, xN, wK, N[1].opponent);

    default:
      return false;
  }
} /* ConsultEndGameDB */

asm ULONG asmPieceCount(void) { mr rTmp1, rPieceCount blr } /* asmPieceCount */

/**************************************************************************************************/
/*                                                                                                */
/*                                         K*K* ENDGAME DATABASES */
/*                                                                                                */
/**************************************************************************************************/

#define packSquare(sq) ((rank(sq) << 3) + file(sq))

static INT wkRankOffset[4] = {0, 3, 5, 6};

static SQUARE Transpose0(SQUARE sq);
static SQUARE Transpose1(SQUARE sq);
static SQUARE Transpose2(SQUARE sq);
static SQUARE Transpose3(SQUARE sq);
static SQUARE Transpose4(SQUARE sq);
static SQUARE Transpose5(SQUARE sq);
static SQUARE Transpose6(SQUARE sq);
static SQUARE Transpose7(SQUARE sq);

// All K*K* databases (e.g. KQKR, KRKN e.t.c) are coded in the same way. When
// consulting the databases, we first normalize the position (so that the
// white/winning king is in octant 0). Then we build the database index, and
// consult the database (if present). If successful, we update RootNode->val and
// return true, otherwise we simply return false.

static BOOL Consult_KxKy(ENGINE *E, CHAR *edbName, SQUARE wK, SQUARE wX,
                         SQUARE bK, SQUARE bX, COLOUR thePlayer) {
  INT f = file(wK), r = rank(wK);
  SQUARE (*T)(SQUARE sq);

  // The positions must first be normalized, so the white king lies in octant 0
  // (i.e. the triangular area formed by the 10 squares a1,b1,c1,d1, b2,c2,d2,
  // c3,d3, d4).

  if (f <= 3)
    if (r <= f)
      T = Transpose0;  // Octant 0 : (f,r) -> (f,r)
    else if (r <= 3)
      T = Transpose1;  // Octant 1 : (f,r) -> (r,f)
    else if (f + r <= 7)
      T = Transpose2;  // Octant 2 : (f,r) -> (7-r,f)
    else
      T = Transpose3;  // Octant 3 : (f,r) -> (f,7-r)
  else if (r >= f)
    T = Transpose4;  // Octant 4 : (f,r) -> (7-f,7-r)
  else if (r >= 4)
    T = Transpose5;  // Octant 5 : (f,r) -> (7-r,7-f)
  else if (f + r >= 7)
    T = Transpose6;  // Octant 6 : (f,r) -> (r,7-f)
  else
    T = Transpose7;  // Octant 7 : (f,r) -> (7-f,r)

  wK = T(wK);
  wX = T(wX);
  bK = T(bK);
  bX = T(bX);

  // Next we "code" the position, i.e. build a 23 bit database index:

  LONG pos = file(wK) + wkRankOffset[rank(wK)];
  pos <<= 6;
  pos += packSquare(wX);
  pos <<= 6;
  pos += packSquare(bK);
  pos <<= 6;
  pos += packSquare(bX);
  pos <<= 1;
  pos += (thePlayer >> 4);

  // Finally we open the database and retrieve the designated entry:

  CopyStr(edbName, E->S.edbName);
  E->S.edbPos = pos;
  E->S.edbResult = -1;
  SendMsg_Sync(E, msg_ProbeEndgDB);  // MUST be a SYNC call, i.e. we wait for
                                     // host app to probe Endgame Database.
  INT n = E->S.edbResult;

  if (n == -1) {
    E->S.edbPresent = false;
    return false;
  } else {
    // If the position is drawn and the "major" player is to move, we bypass the
    // endgame database (since this will consider all drawn moves equal, whereas
    // the normal search engine will play more aggressively).

    if (n >= nonWinVal && thePlayer == black) return false;

    clrMove(E->S.rootNode[1].m);
    clrMove(E->S.rootNode[1].BestLine[0]);

    if (n >= nonWinVal)
      E->S.rootNode->val = 0;
    else
      E->S.rootNode->val =
          (thePlayer == black ? maxVal - (2 * n + 1) : 2 * n - maxVal);

    return true;
  }

  return false;
} /* Consult_KxKy */

/*-------------------------------------- Square Transposition
 * ------------------------------------*/

static SQUARE Transpose0(SQUARE sq) { return square(file(sq), rank(sq)); }
static SQUARE Transpose1(SQUARE sq) { return square(rank(sq), file(sq)); }
static SQUARE Transpose2(SQUARE sq) { return square(7 - rank(sq), file(sq)); }
static SQUARE Transpose3(SQUARE sq) { return square(file(sq), 7 - rank(sq)); }
static SQUARE Transpose4(SQUARE sq) {
  return square(7 - file(sq), 7 - rank(sq));
}
static SQUARE Transpose5(SQUARE sq) {
  return square(7 - rank(sq), 7 - file(sq));
}
static SQUARE Transpose6(SQUARE sq) { return square(rank(sq), 7 - file(sq)); }
static SQUARE Transpose7(SQUARE sq) { return square(7 - file(sq), rank(sq)); }
