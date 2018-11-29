/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAMEUTIL.C */
/* Purpose : This module implements the game API, where games can be started and
 * where moves can  */
/*           be played, taken back and replayed. All information pertaining to a
 * game (except     */
/*           annotations and other external stuff) is stored in a "CGame"
 * object, which contains  */
/*           the initial board position (incl. castling and en passant rights),
 * the actual moves  */
/*           of the game as well as the current board position and various
 * status flags. Also     */
/*           a GAMEINFO record is included holding information about players,
 * site, event e.t.c.  */
/*           The Sigma Engine takes a "CGame" object as input and starts
 * analysing the current    */
/*           board position. */
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

#include "GameUtil.h"
#include "SigmaPrefs.h"

#include "Board.f"
#include "Move.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                        CALC MOVE FLAGS */
/*                                                                                                */
/**************************************************************************************************/

static CHAR WingChar[9] =
    "RNBQKBNR";  // File (wing independant) signatures in descriptive notation.

/*-------------------------- Compute Disambiguation Flags for Game Moves
 * -------------------------*/
// Computes the move disambiguation flags Must be done BEFORE CalcMoves().

void CalcDisambFlags(MOVE *m, MOVE *M, INT moveCount) {
  BOOL ambiguous = false;
  BOOL rankUnique = true;
  BOOL fileUnique = true;
  INT dfrom = 0;
  INT dto = 0;

  m->flags = 0;  // Reset move flags.

  for (INT j = 0; j < moveCount; j++, M++)
    if (M->piece == m->piece && M->cap == m->cap) {
      if (M->to == m->to && M->from != m->from) {
        // Algebraic flags:
        ambiguous = true;
        if (rank(M->from) == rank(m->from)) rankUnique = false;
        if (file(M->from) == file(m->from)) fileUnique = false;

        // Descriptive flags:
        if (pieceType(m->piece) == pawn &&
            WingChar[file(M->from)] != WingChar[file(m->from)])
          dfrom = Max(dfrom, 1);
        else if (wing(M->from) != wing(m->from))
          dfrom = Max(dfrom, 2);
        else
          dfrom = 3;
      }

      // Additional Descriptive Notation Disambiguation:
      if (!m->cap) {  // Check if destination wing should be shown
        if (rank(M->to) == rank(m->to) && file(M->to) == 7 - file(m->to))
          dto = Max(dto, 3);
        else
          dto = Max(dto, 2);
      } else if (M->to != m->to) {  // If there is more than one capture of the
                                    // same piece type but on different squares
        // we have to include destination indication.
        if (pieceType(m->piece) == pawn &&
            WingChar[file(M->from)] != WingChar[file(m->from)])
          dfrom = Max(dfrom, 1);
        else if (pieceType(m->cap) == pawn &&
                 WingChar[file(M->to)] != WingChar[file(m->to)])
          dto = Max(dto, 1);
        else if (wing(M->to) != wing(m->to))
          dto = Max(dto, 2);
        else
          dto = 3;
      }
    }

  if (ambiguous) {
    if (fileUnique || !rankUnique) m->flags |= moveFlag_ShowFile;
    if (!fileUnique) m->flags |= moveFlag_ShowRank;
  }

  m->flags |= (dfrom << 4) | (dto << 6);
} /* CalcDisambFlags */

/*----------------------- Compute Disambiguation Flags for List of Moves
 * -------------------------*/
// Computes the move disambiguation flags m->flag for the specified list of
// moves (null terminated). The m->misc fields are all cleared. Can subsequently
// be used for computing SAN output strings (but not descriptive notation).

void CalcVariationFlags(PIECE Board[], MOVE *M) {
  PIECE B[boardSize];

  CopyTable(Board, B);

  while (M->piece) {
    CalcMoveFlags(B, M);
    Move_Perform(B, M);
    M++;
  }
} /* CalcVariationFlags */

void CalcMoveFlags(PIECE B[], MOVE *m) {
  if (isNull(*m)) return;

  BOOL ambiguous = false;
  BOOL rankUnique = true;
  BOOL fileUnique = true;
  INT *Dir;

  m->flags = m->misc = 0;

  //--- Switch on piece type ---

  switch (pieceType(m->piece)) {
    case king:
    case pawn:
      goto CheckAmb;

    case knight:
      for (INT i = 0; i < 8; i++) {
        SQUARE sq = m->to - KnightDir[i];
        if (onBoard(sq) && B[sq] == m->piece && sq != m->from) {
          ambiguous = true;
          if (rank(m->from) == rank(sq)) rankUnique = false;
          if (file(m->from) == file(sq)) fileUnique = false;
        }
      }
      goto CheckAmb;

    case bishop:
      Dir = BishopDir;
      break;
    case rook:
      Dir = RookDir;
      break;
    case queen:
      Dir = QueenDir;
      break;
  }

  for (INT i = 0; Dir[i]; i++) {
    SQUARE sq = m->to;
    do
      sq -= Dir[i];
    while (B[sq] == empty);
    if (onBoard(sq) && B[sq] == m->piece && sq != m->from) {
      ambiguous = true;
      if (rank(m->from) == rank(sq)) rankUnique = false;
      if (file(m->from) == file(sq)) fileUnique = false;
    }
  }

CheckAmb:
  if (ambiguous) {
    if (fileUnique || !rankUnique) m->flags |= moveFlag_ShowFile;
    if (!fileUnique) m->flags |= moveFlag_ShowRank;
  }
}  // CalcMoveFlags

/**************************************************************************************************/
/*                                                                                                */
/*                                          MOVE NOTATION */
/*                                                                                                */
/**************************************************************************************************/

static CHAR PieceChar[8];  // Language dependant piece letters. SHOULD NOT BE
                           // USED BY PGN
static MOVE_NOTATION notation;  // Use long algebraic notation?

/*----------------------------------------- Move Notation
 * ----------------------------------------*/

void SetGameNotation(CHAR s[], MOVE_NOTATION theNotation)  // s = "PNBRQK"
{
  PieceChar[empty] = ' ';
  for (PIECE p = pawn; p <= king; p++) PieceChar[p] = s[p - 1];

  notation = theNotation;
} /* SetGameNotation */

/*-------------------------- Compute Basic Move String (Long Algebraic)
 * --------------------------*/
// Converts a move into "clean" long algebraic notation (i.e. text only and no
// check/mate indicators). This type of move string is generally used for
// non-game moves (where there is no disambiguation info or check/mate flags).

INT CalcMoveStr(MOVE *m, CHAR *s) {
  if (isNull(*m)) {
    *(s++) = 'n';
    *(s++) = 'o';
    *(s++) = 'n';
    *(s++) = 'e';
    *(s++) = 0;
    return 4;
  }

  switch (m->type) {
    case mtype_Normal:
      if (pieceType(m->piece) != pawn) *(s++) = PieceChar[pieceType(m->piece)];
      *(s++) = file(m->from) + 'a';
      *(s++) = rank(m->from) + '1';
      *(s++) = (m->cap ? 'x' : '-');
      *(s++) = file(m->to) + 'a';
      *(s++) = rank(m->to) + '1';
      *(s++) = 0;
      return (pieceType(m->piece) == pawn ? 5 : 6);
    case mtype_O_O:
      *(s++) = 'O';
      *(s++) = '-';
      *(s++) = 'O';
      *(s++) = 0;
      return 3;
    case mtype_O_O_O:
      *(s++) = 'O';
      *(s++) = '-';
      *(s++) = 'O';
      *(s++) = '-';
      *(s++) = 'O';
      *(s++) = 0;
      return 5;
    case mtype_EP:
      *(s++) = file(m->from) + 'a';
      *(s++) = rank(m->from) + '1';
      *(s++) = 'x';
      *(s++) = file(m->to) + 'a';
      *(s++) = rank(m->to) + '1';
      *(s++) = 'E';
      *(s++) = 'P';
      *(s++) = 0;
      return 7;
    case mtype_Null:
      *(s++) = 'n';
      *(s++) = 'u';
      *(s++) = 'l';
      *(s++) = 'l';
      *(s++) = 0;
      return 4;
    default:  // Promotion
      *(s++) = file(m->from) + 'a';
      *(s++) = rank(m->from) + '1';
      *(s++) = (m->cap ? 'x' : '-');
      *(s++) = file(m->to) + 'a';
      *(s++) = rank(m->to) + '1';
      *(s++) = '=';
      *(s++) = PieceChar[pieceType(m->type)];
      *(s++) = 0;
      return 7;
  }
} /* CalcMoveStr */

/*-------------------------- Compute Game Move String (Algebraic)
 * --------------------------------*/
// This routine computes a move string in either long or short algebraic
// notation, including check/mate indicators and annotation glyphs. May only be
// called for game moves, i.e. moves where the m->flags field has been set (with
// source/destination disambiguation info etc.)

INT CalcGameMoveStr(MOVE *m, CHAR *s) {
  if (notation == moveNot_Descr)
    return CalcGameMoveStrDesc(m, s);
  else
    return CalcGameMoveStrAlge(m, s, (notation == moveNot_Long));
} /* CalcGameMoveStr */

INT CalcGameMoveStrAlge(MOVE *m, CHAR *s, BOOL longNotation, BOOL english,
                        BOOL EPsuffix) {
  INT n = 0;  // Move str length.
  CHAR *PC = (english ? PieceCharEng : PieceChar);

  //--- Build actual move string:

  switch (m->type) {
    case mtype_O_O:
      CopyStr("O-O", s);
      n = 3;
      break;

    case mtype_O_O_O:
      CopyStr("O-O-O", s);
      n = 5;
      break;

    default:
      if (pieceType(m->piece) != pawn)  // If not pawn -> write piece letter
        s[n++] = PC[pieceType(m->piece)];

      if (longNotation || pieceType(m->piece) != pawn)  // Write source square
      {
        if (longNotation || (m->flags & moveFlag_ShowFile))
          s[n++] = file(m->from) + 'a';
        if (longNotation || (m->flags & moveFlag_ShowRank))
          s[n++] = rank(m->from) + '1';
      } else if (m->cap ||
                 m->type == mtype_EP)  // If pawn capture/en passant...
      {
        s[n++] = file(m->from) + 'a';  //   Write source file
      }

      if (m->cap || m->type == mtype_EP)  // Write 'x' if capture/en passant
        s[n++] = 'x';
      else if (longNotation)
        s[n++] = '-';

      s[n++] = file(m->to) + 'a';  // Write destination square.
      s[n++] = rank(m->to) + '1';

      if (m->type == mtype_EP && EPsuffix)  // Write 'EP' suffix if en passant.
        s[n++] = 'E', s[n++] = 'P';

      if (isPromotion(*m))  // If promotion, indicate
      {                     // promotion piece.
        s[n++] = '=';
        s[n++] = PC[pieceType(m->type)];
      }
  }

  //--- Include check indicator (if any):

  if (m->flags & moveFlag_Check)
    s[n++] = (m->flags & moveFlag_Mate ? '#' : '+');

  //--- Include annotation glyph (if any):

  switch (m->misc) {
    case 1:
      s[n++] = '!';
      break;
    case 2:
      s[n++] = '?';
      break;
    case 3:
      s[n++] = '!';
      s[n++] = '!';
      break;
    case 4:
      s[n++] = '?';
      s[n++] = '?';
      break;
    case 5:
      s[n++] = '!';
      s[n++] = '?';
      break;
    case 6:
      s[n++] = '?';
      s[n++] = '!';
      break;
  }

  //--- Null-terminate and return string length:

  s[n] = 0;
  return n;
} /* CalcGameMoveStrAlge */

/*-------------------------- Compute Game Move String (Descriptive)
 * ------------------------------*/
// This routine computes a move string in the "old" descriptive notation,
// including check/mate indicators and annotation glyphs. May only be called for
// game moves, i.e. moves where the m->flags field has been set (with
// source/destination disambiguation info etc.)

INT CalcGameMoveStrDesc(MOVE *m, CHAR *s) {
  INT n = 0;  // Move str length.

  //--- Build actual move string:

  switch (m->type) {
    case mtype_O_O:
      CopyStr("O-O", s);
      n = 3;
      break;

    case mtype_O_O_O:
      CopyStr("O-O-O", s);
      n = 5;
      break;

    case mtype_EP:
      if (m->flags & moveFlag_DescrFrom) s[n++] = WingChar[file(m->from)];
      CopyStr("PxP ep", s + n);
      n += 6;
      break;
      break;

    default:
      INT piece = pieceType(m->piece);
      INT cap = pieceType(m->cap);
      INT sf = file(m->from);
      INT df = file(m->to);
      INT sr =
          (pieceColour(m->piece) == white ? rank(m->from) : 7 - rank(m->from));
      INT dr = (pieceColour(m->piece) == white ? rank(m->to) : 7 - rank(m->to));
      INT dfrom = (m->flags >> 4) & 0x03;
      INT dto = (m->flags >> 6) & 0x03;

      // Write optional source square disambiguation prefix:
      if (dfrom < 3)
        if (piece == pawn) {
          if (dfrom == 2)
            if (sf < 3)
              s[n++] = 'Q';
            else if (sf > 4)
              s[n++] = 'K';
          if (dfrom >= 1) s[n++] = WingChar[sf];
        } else {
          if (dfrom > 0) s[n++] = (sf < 4 ? 'Q' : 'K');
        }

      // Write moving piece:
      s[n++] = PieceCharEng[piece];

      // Write optional source square disambiguation postfix):
      if (dfrom == 3) {
        s[n++] = '(';
        //          if (sf < 3) s[n++] = 'Q'; else if (sf > 4) s[n++] = 'K';
        //          //### Necessary??
        s[n++] = WingChar[sf];
        s[n++] = sr + '1';
        s[n++] = ')';
      }

      // Write separator:
      s[n++] = (cap ? 'x' : '-');

      // Write destination square
      if (!cap) {
        if (dto == 3)
          if (df < 3)
            s[n++] = 'Q';
          else if (df > 4)
            s[n++] = 'K';
        s[n++] = WingChar[df];
        s[n++] = dr + '1';
      } else if (cap == pawn && dto <= 2) {
        if (dto == 2)
          if (df < 3)
            s[n++] = 'Q';
          else if (df > 4)
            s[n++] = 'K';
        if (dto >= 1) s[n++] = WingChar[df];
        s[n++] = 'P';
      } else {
        if (dto == 1)
          if (df < 3)
            s[n++] = 'Q';
          else if (df > 4)
            s[n++] = 'K';
        s[n++] = PieceCharEng[cap];

        if (dto == 3) {
          s[n++] = '(';
          if (df < 3)
            s[n++] = 'Q';
          else if (df > 4)
            s[n++] = 'K';
          s[n++] = WingChar[df];
          s[n++] = dr + '1';
          s[n++] = ')';
        }
      }

      if (isPromotion(*m))  // If promotion, indicate
      {                     // promotion piece.
        s[n++] = '=';
        s[n++] = PieceCharEng[pieceType(m->type)];
      }
  }

  //--- Include check indicator (if any):

  if (m->flags & moveFlag_Check)
    s[n++] = (m->flags & moveFlag_Mate ? '#' : '+');

  //--- Include annotation glyph (if any):

  switch (m->misc) {
    case 1:
      s[n++] = '!';
      break;
    case 2:
      s[n++] = '?';
      break;
    case 3:
      s[n++] = '!';
      s[n++] = '!';
      break;
    case 4:
      s[n++] = '?';
      s[n++] = '?';
      break;
    case 5:
      s[n++] = '!';
      s[n++] = '?';
      break;
    case 6:
      s[n++] = '?';
      s[n++] = '!';
      break;
  }

  //--- Null-terminate and return string length:

  s[n] = 0;
  return n;
} /* CalcGameMoveStrDesc */

/**************************************************************************************************/
/*                                                                                                */
/*                                          MOVE NOTATION */
/*                                                                                                */
/**************************************************************************************************/

/*
static CHAR *vstr;

INT CalcVariationStr (PIECE Board[], INT moveNo, MOVE *M, CHAR *str)
{
   vstr = str;
   NumToStr(
} /* CalcVariationStr */

/**************************************************************************************************/
/*                                                                                                */
/*                                           UTILITY */
/*                                                                                                */
/**************************************************************************************************/

void CalcSquareStr(SQUARE sq, CHAR *s) {
  if (sq == nullSq)
    s[0] = 0;
  else {
    s[0] = file(sq) + 'a';
    s[1] = rank(sq) + '1';
    s[2] = 0;
  }
} /* CalcSquareStr */

SQUARE ParseSquareStr(CHAR *s) {
  return (StrLen(s) == 2 ? square(s[0] - 'a', s[1] - '1') : nullSq);
} /* ParseSquareStr */

void CalcInfoResultStr(INT result, CHAR *s) {
  switch (result) {
    case infoResult_Unknown:
      CopyStr("Unknown", s);
      break;
    case infoResult_WhiteWin:
      CopyStr("1-0", s);
      break;
    case infoResult_BlackWin:
      CopyStr("0-1", s);
      break;
    case infoResult_Draw:
      CopyStr("1/2-1/2", s);
      break;
    default:
      CopyStr("", s);
  }
} /* CalcInfoResultStr */

void CalcScoreStr(CHAR *s, INT score, INT scoreType) {
  if (scoreType == scoreType_Book) {
    CopyStr("book", s);
  } else if (scoreType == scoreType_Unknown) {
    CopyStr("-", s);
  } else if (score == 0) {
    CopyStr("0.00", s);
  } else {
    if (scoreType == scoreType_LowerBound)
      *(s++) = '³', *(s++) = ' ';
    else if (scoreType == scoreType_UpperBound)
      *(s++) = '²', *(s++) = ' ';

    *(s++) = (score > 0 ? '+' : '-');
    if (score < 0) score = -score;

    if (score >= mateWinVal) {
      CopyStr("mate ", s);
      s += 5;
      INT n = (1 + maxVal - score) / 2;
      if (n > 9) *(s++) = n / 10 + '0';
      *(s++) = n % 10 + '0';
    } else {
      INT n = score / 100;
      if (n > 9) *(s++) = n / 10 + '0';
      *(s++) = n % 10 + '0';
      *(s++) = '.';
      n = score % 100;
      *(s++) = n / 10 + '0';
      *(s++) = n % 10 + '0';
    }

    if (scoreType == scoreType_Temp) *(s++) = ' ', *(s++) = 'É';

    *s = '\0';
  }
} /* CalcScoreStr */

BOOL ParseScoreStr(CHAR s[], INT *score)  // Format: [±]d[d][.d[d]]
{
  BOOL negative = false;
  INT i = 0, N;

  // First parse optional sign:
  if (s[i] == '+')
    negative = false, i++;
  else if (s[i] == '-')
    negative = true, i++;

  // Then parse mandatory leading digit:
  if (!IsDigit(s[i])) return false;
  N = (s[i++] - '0') * 100;

  // Parse optional 2nd digit (in decimal part):
  if (IsDigit(s[i])) N = 10 * N + (s[i++] - '0') * 100;

  // Parse optional fractional part:
  if (s[i] == '.') {
    i++;
    if (IsDigit(s[i]))
      N += 10 * (s[i++] - '0');
    else
      return false;
    if (IsDigit(s[i])) N += (s[i++] - '0');
  }

  // If the string contains no other chars, then we are done and the score is
  // returned:
  if (s[i]) return false;
  *score = (negative ? -N : N);
  return true;
} /* ParseScoreStr */

INT CheckAbsScore(COLOUR player, INT score) {
  return (Prefs.AnalysisFormat.scoreNot != scoreNot_NumRel && player == black
              ? -score
              : score);
} /* CheckAbsScore */
