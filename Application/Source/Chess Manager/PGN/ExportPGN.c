/**************************************************************************************************/
/*                                                                                                */
/* Module  : ExportPGN.c */
/* Purpose : This module implements the PGN export methods. */
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

#include "Board.f"
#include "PGN.h"
#include "SigmaPrefs.h"

/*
Formal PGN syntax

<PGN-database> ::= <PGN-game> <PGN-database>
                   <empty>

<PGN-game> ::= <tag-section> <movetext-section>

<tag-section> ::= <tag-pair> <tag-section>
                  <empty>

<tag-pair> ::= [ <tag-name> <tag-value> ]

<tag-name> ::= <identifier>

<tag-value> ::= <string>

<movetext-section> ::= <element-sequence> <game-termination>

<element-sequence> ::= <element> <element-sequence>
                       <recursive-variation> <element-sequence>
                       <empty>

<element> ::= <move-number-indication>
              <SAN-move>
              <numeric-annotation-glyph>

<recursive-variation> ::= ( <element-sequence> )

<game-termination> ::= 1-0
                       0-1
                       1/2-1/2
                       *
<empty> ::=
*/

// Newline on mac : 0x0D = 13
// PGN standard requires : 0x0A = 10
// Give user option when saving ### (and be TOLERANT when loading!!).

#define WriteChar(c) buf[pos++] = c

/**************************************************************************************************/
/*                                                                                                */
/*                                         EXPORTING PGN */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- Export Full Game
 * ---------------------------------------*/
// This is the only public PGN Export method. It simply convertes the CGame
// object to PGN format and writes the result to the "pgnBuf" parameter. This
// buffer must already have been allocated by the calling routine (with a size =
// "pgn_BufferSize"). On exit the routine returns the number of bytes written.

LONG CPgn::WriteGame(CHAR *pgnBuf) {
  buf = (PTR)pgnBuf;
  pos = 0;
  WriteTagSection();
  WriteStr("\n");
  WriteMoveSection();
  WriteStr("\n");
  bufSize = pos;

  // Mac newline converter:
  if (true)  //###
    for (LONG i = 0; i < bufSize; i++)
      if (buf[i] == 0x0A) buf[i] = 0x0D;

  return bufSize;
} /* CPgn::Write */

/*--------------------------------------- Export FEN Tag
 * -----------------------------------------*/

LONG CPgn::WriteFEN(CHAR *fenBuf) {
  buf = (PTR)fenBuf;
  pos = 0;
  WriteTagFEN();
  bufSize = pos;
  buf[pos] = 0;  // Null terminate
  return bufSize;
} /* CPgn::WriteFEN */

/**************************************************************************************************/
/*                                                                                                */
/*                                     EXPORT TAG SECTION */
/*                                                                                                */
/**************************************************************************************************/

void CPgn::WriteTagSection(void) {
  // Write the 7 mandatory tags (STR = Seven Tag Roster):

  WriteTagStr("Event", game->Info.event);
  WriteTagStr("Site", game->Info.site);
  WriteTagStr("Date",
              StrLen(game->Info.date) == 10 ? game->Info.date : "????.??.??");
  WriteTagStr("Round", game->Info.round);
  WriteTagStr("White", game->Info.whiteName);
  WriteTagStr("Black", game->Info.blackName);
  WriteStr("[Result \"");
  WriteResult(game->Info.result);
  WriteStr("\"]\n");

  // Write optional tags (unless they are blank):

  WriteTagInt("WhiteElo", game->Info.whiteELO, true);
  WriteTagInt("BlackElo", game->Info.blackELO, true);
  if (game->Info.ECO[0] != 0) WriteTagStr("ECO", game->Info.ECO);
  if (game->Info.annotator[0] != 0)
    WriteTagStr("Annotator", game->Info.annotator);

  // If the initial position was set up we have to include a "SetUp" and a "FEN"
  // tag:

  if (game->Init.wasSetup) {
    WriteTagInt("SetUp", 1, false);
    WriteStr("[FEN \"");
    WriteTagFEN();
    WriteStr("\"]\n");
  }
} /* CPgn::WriteTagSection */

/*------------------------------------- Export Result Tag
 * ----------------------------------------*/

void CPgn::WriteResult(INT result) {
  switch (game->Info.result) {
    case infoResult_Unknown:
      WriteStr("*");
      break;
    case infoResult_WhiteWin:
      WriteStr("1-0");
      break;
    case infoResult_BlackWin:
      WriteStr("0-1");
      break;
    case infoResult_Draw:
      WriteStr("1/2-1/2");
      break;
    default:
      WriteStr("*");
      break;
  }
} /* CPgn::WriteResult */

/*--------------------------------------- Export FEN Tag
 * -----------------------------------------*/
// If the initial position was set up we have to include a "SetUp" and a "FEN"
// tag. Example:
//
//   [SetUp "1"]
//   [FEN "rnbqr1k1/pp2bppp/4p3/2ppP3/3P3P/2NB1N2/PPP2PP1/R2QK2R w KQ - 0 1"]
//
// The "WriteTagFEN" routine below generates the RAW fen string inside the
// quotes above. It is up to the caller to include extra tags.

void CPgn::WriteTagFEN(void) {
  //--- Write the position field ---

  for (INT rank = 7; rank >= 0; rank--) {
    INT emptyCount = 0;

    for (INT file = 0; file <= 7; file++) {
      PIECE p = game->Init.Board[(rank << 4) + file];
      if (!p)
        emptyCount++;
      else {
        if (file > 0 && emptyCount > 0) WriteInt(emptyCount), emptyCount = 0;
        CHAR c = PieceCharEng[pieceType(p)];
        if (pieceColour(p) == black) c = (c - 'A') + 'a';
        WriteChar(c);
      }
    }
    if (emptyCount > 0) WriteInt(emptyCount);
    if (rank > 0) WriteChar('/');
  }
  WriteChar(' ');

  //--- Write initial player field ---

  WriteChar(game->Init.player == white ? 'w' : 'b');
  WriteChar(' ');

  //--- Write castling rights field ---
  INT cast = game->Init.castlingRights;
  if (game->Init.Board[e1] != wKing)
    cast &= ~(castRight_WO_O | castRight_WO_O_O);
  if (game->Init.Board[h1] != wRook) cast &= ~castRight_WO_O;
  if (game->Init.Board[a1] != wRook) cast &= ~castRight_WO_O_O;
  if (game->Init.Board[e8] != bKing)
    cast &= ~(castRight_BO_O | castRight_BO_O_O);
  if (game->Init.Board[h8] != bRook) cast &= ~castRight_BO_O;
  if (game->Init.Board[a8] != bRook) cast &= ~castRight_BO_O_O;

  if (!cast)
    WriteChar('-');
  else {
    if (cast & castRight_WO_O) WriteChar('K');
    if (cast & castRight_WO_O_O) WriteChar('Q');
    if (cast & castRight_BO_O) WriteChar('k');
    if (cast & castRight_BO_O_O) WriteChar('q');
  }
  WriteChar(' ');

  //--- Write en passant field ---

  if (game->Init.epSquare == nullSq)
    WriteChar('-');
  else
    WriteChar(file(game->Init.epSquare) + 'a'),
        WriteChar(rank(game->Init.epSquare) + '1');
  WriteChar(' ');

  //--- Write reversable moves field ---

  WriteInt(game->Init.revMoves);  // Half moves since last capture/pawn move
  WriteChar(' ');

  //--- Write initial move no field ---

  WriteInt(game->Init.moveNo);  // Initial move no

} /* CPgn::WriteTagFEN */

/*------------------------------------- Tag Export Utility
 * ---------------------------------------*/

void CPgn::WriteTagStr(CHAR *tag, CHAR *s) {
  WriteChar('[');
  WriteStr(tag);
  WriteStr(" \"");
  if (s[0] == 0)
    WriteChar('?');
  else
    WriteStrBS(s);
  WriteStr("\"]\n");
} /* CPgn::WriteTagStr */

void CPgn::WriteTagInt(CHAR *tag, INT n, BOOL skipIfBlank) {
  if (n < 0 && skipIfBlank) return;

  WriteChar('[');
  WriteStr(tag);
  WriteStr(" \"");
  if (n < 0)
    WriteChar('?');
  else
    WriteInt(n);
  WriteStr("\"]\n");
} /* CPgn::WriteTagInt */

/**************************************************************************************************/
/*                                                                                                */
/*                                      EXPORT MOVE SECTION */
/*                                                                                                */
/**************************************************************************************************/

void CPgn::WriteMoveSection(void) {
  INT pos0 = pos;

  WriteAnnText(0);

  for (INT i = 1; i <= game->lastMove; i++) {
    WriteMoveNo(i, pieceColour(game->Record[i].piece),
                (i == 1 /*|| AnnDataMap[i - 1]*/));
    WriteMove(&game->Record[i], game->Record[i].flags, game->Record[i].misc);
    WriteAnnText(i);
  }

  WriteResult(game->Info.result);
  WriteChar('\n');
  WrapLines(pos0, pos);
} /* CPgn::WriteMoveSection */

void CPgn::WriteMoveNo(INT i, COLOUR player, BOOL forceWrite) {
  if (player == white || forceWrite) {
    WriteInt(game->Init.moveNo + (game->Init.player == black ? i : i - 1) / 2);
    WriteStr(player == white ? "." : "...");
    if (!(flags & pgnFlag_SkipMoveSep)) WriteChar(' ');
  }
} /* CPgn::WriteMoveNo */

void CPgn::WriteMove(MOVE *m, BYTE flags, BYTE glyph) {
  switch (m->type) {
    case mtype_O_O:
      WriteStr("O-O");
      break;

    case mtype_O_O_O:
      WriteStr("O-O-O");
      break;

    default:
      if (pieceType(m->piece) != pawn)  // If not pawn:
      {
        WriteChar(PieceCharEng[pieceType(
            m->piece)]);  //   Write piece char (ENGLISH!!!)

        if (flags & moveFlag_ShowFile) WriteChar(file(m->from) + 'a');
        if (flags & moveFlag_ShowRank) WriteChar(rank(m->from) + '1');
      } else if (m->cap ||
                 m->type == mtype_EP)  // If pawn capture/en passant...
      {
        WriteChar(file(m->from) + 'a');  //   Write source file
      }

      if (m->cap || m->type == mtype_EP)  // Write 'x' if capture/en passant
        WriteChar('x');
      WriteChar(file(m->to) + 'a');  // Finally write destination
      WriteChar(rank(m->to) + '1');  // square.

      if (isPromotion(*m))  // If promotion, indicate
      {                     // promotion piece.
        WriteChar('=');
        WriteChar(PieceCharEng[pieceType(m->type)]);
      }
  }

  if (flags & moveFlag_Check) WriteChar(flags & moveFlag_Mate ? '#' : '+');

  switch (glyph) {
    case 0:
      break;
    case 1:
      WriteStr("!");
      break;
    case 2:
      WriteStr("?");
      break;
    case 3:
      WriteStr("!!");
      break;
    case 4:
      WriteStr("??");
      break;
    case 5:
      WriteStr("!?");
      break;
    case 6:
      WriteStr("?!");
      break;
  }

  WriteChar(' ');
} /* CPgn::WriteMove */

void CPgn::WriteAnnText(INT j) {
  if (flags & pgnFlag_SkipAnn) return;

  if (!game->ExistAnnotation(j))
    return;  // Exit if no annotations for this move.

  WriteStr("{ ");
  INT charCount;
  game->GetAnnotation(j, (CHAR *)&buf[pos], &charCount);

  // Replace illegal/unwanted ascii chars with blanks:
  for (INT i = 0; i < charCount; i++, pos++) {
    BYTE c = buf[pos];  // Important to be unsigned char
    if (Prefs.PGN.keepNewLines && IsNewLine(c))
      buf[pos] = '\n';
    else if (c < 32 ||
             /*(c >= 128 && c <= 159) Sigma 6.1.4 bug fix ||*/ c == '{' ||
             c == '}')
      buf[pos] = ' ';
  }
  WriteStr(" } ");
} /* CPgn::WriteAnnText */

/**************************************************************************************************/
/*                                                                                                */
/*                                         EXPORT UTILITY */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Export Utility Routines
 * -----------------------------------*/

void CPgn::WriteStr(CHAR *s) {
  while (*s) WriteChar(*(s++));
} /* CPgn::WriteStr */

void CPgn::WriteStrBS(CHAR *s) {
  while (*s) {
    if (*s == '"' || *s == backSlash) WriteChar(backSlash);
    WriteChar(*(s++));
  }
} /* CPgn::WriteStrBS */

void CPgn::WriteInt(INT n) {
  Str255 s;

  NumToString(n, s);
  s[s[0] + 1] = 0;
  WriteStr((CHAR *)&s[1]);
} /* CPgn::WriteInt */

void CPgn::WrapLines(LONG imin, LONG imax) {
  LONG i, isp = -1;

  for (i = imin; i < imax;
       i++)  // As long as there is more than one line left...
  {
    if (i >= imin + 80 && isp != -1)  // If line is now too long, then
      buf[isp] = '\n',                // insert new line and reset indicators.
          imin = isp + 1, isp = -1;

    if (buf[i] == ' ')  // Remember latest blank...
      isp = i;
    else if (buf[i] == '\n')   // If we meet a new line, then reset
      imin = i + 1, isp = -1;  // indicators.
  }
} /* CPgn::WrapLines */
