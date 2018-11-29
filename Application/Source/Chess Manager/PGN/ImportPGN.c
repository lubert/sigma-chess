/**************************************************************************************************/
/*                                                                                                */
/* Module  : ImportPGN.c */
/* Purpose : This module implements PGN Import methods. */
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

#define PeekChar() buf[pos]

/**************************************************************************************************/
/*                                                                                                */
/*                                       PREPARE PGN IMPORT */
/*                                                                                                */
/**************************************************************************************************/

// Importing PGN is much more complex than exporting. First of all, since a PGN
// file normally contains multiple games, we can't just open it in a game
// window. Therefore, when opening PGN files, a new empty Game Collection is
// first created, and then the PGN games are imported. The same approach is used
// when importing PGN files into an existing collection.

// Since PGN games often contain errors, we have to react on these in a sensible
// way: If an error is detected a dialog is displayed giving the user the option
// of aborting the import process, continuing and skipping the erroneous game
// e.t.c.

// During the import process, the generic game object assigned to the game
// collection object is used as an "agent" for importing/parsing PGN games one
// at a time.

// It's the caller's responsibility to make sure that the PGN data is available
// in the buffer before calling CPgn::ReadGame(). Also after each games has been
// processed, the caller should move the next PGN game to the start of the
// buffer and fill the remainder with data from the PGN file. The
// CPgn::ReadGame() method returns "true" if no errors where found.

void CPgn::ReadBegin(CHAR *pgnBuf) {
  buf = (PTR)pgnBuf;

  totalBytesRead = 0;
  totalLinesRead = 0;

  error = pgnErr_NoError;
  errLineNo = -1;
  errLinePos = -1;
  errTokenPos = -1;
  errTokenLen = -1;

  result = -1;
} /* CPgn::ReadBegin */

/**************************************************************************************************/
/*                                                                                                */
/*                                       IMPORT SINGLE GAME */
/*                                                                                                */
/**************************************************************************************************/

// The "CGame::ReadPGN" routine reads a single PGN game from the specified
// buffer into the CGame object. On entry, the "size" parameter indicates the
// size of the PGN input buffer. On exit, the number of bytes read is returned
// in "size", and any error codes are returned as the function result. If no
// errors are found, ReadPGN tries to read ahead to the beginning of the next
// game if any.

// On entry "bytes" is size of the pgnBuffer and hence the maximum number of
// bytes to read. On exit "bytes" holds the actual number of read bytes.

BOOL CPgn::ReadGame(LONG pgnBufSize) {
  if (pgnBufSize <= 0) return SetError(pgnErr_EOFReached);

  bufSize = pgnBufSize;
  pos = 0;
  linePos = 0;

  ReadPreprocess();
  LocateTags();  // Read on till first tag begins (OK if EOF reached here)
  ReadTagSection();
  ReadMoveSection();

  return !error;
} /* CPgn::ReadGame */

/*--------------------------------------- Preprocess PGN File
 * ------------------------------------*/
// The preprocessor replaces invalid ascii values with spaces.

void CPgn::ReadPreprocess(void) {
  for (LONG i = 0; i < bufSize && !error; i++) {
    if (buf[i] == '\r' && i + 1 < bufSize && buf[i + 1] == '\n')
      buf[i] = ' ';
    else if (buf[i] == '\r')
      buf[i] = '\n';
    else if (buf[i] < 32 && buf[i] != '\n')
      buf[i] = ' ';
    //    else if (buf[i] >= 128) // && buf[i] <= 159)
    //       buf[i] = ' ';
  }
} /* CPgn::ReadPreprocess */

void CPgn::LocateTags(void) {
  emptyTagSection = CheckEmptyTagSection();
  if (emptyTagSection) return;

  while (!error && !(PeekChar() == '[' && pos == linePos)) ReadChar();

  if (error == pgnErr_UnexpectedEOF) error = pgnErr_EOFReached;
} /* CPgn::LocateTags */

// If there is no tag section, we may still be able to parse the game if the
// game starts with "1." followed by an optional space and a proper first White
// move

BOOL CPgn::CheckEmptyTagSection(void) {
  LONG i = pos;
  while (buf[i] == ' ') i++;

  if (buf[i++] != '1') return false;
  if (buf[i] == '.' && buf[i + 1] == ' ')
    i += 2;
  else if (buf[i] == '.' || buf[i] == ' ')
    i++;
  else
    return false;

  return (buf[i] >= 'a' && buf[i] <= 'h' &&
          SearchChar(buf[i + 1], "34")) ||  // Pawn move
         (buf[i] == 'N' && SearchChar(buf[i + 1], "acfh") &&
          buf[i + 2] == '3');  // Knight move
} /* CPgn::CheckEmptyTagSection */

/**************************************************************************************************/
/*                                                                                                */
/*                                      IMPORT TAG SECTION */
/*                                                                                                */
/**************************************************************************************************/

static BOOL CalcPGNErrMessage(PGN_ERROR, Str255 s);

BOOL CPgn::ReadTagSection(void) {
  if (error) return false;

  game->ResetInit();
  ::ClearGameInfo(&game->Info);

  result = infoResult_Unknown;  //-1;

  if (!StripWhiteSpace()) return false;
  if (emptyTagSection)
    result = infoResult_Unknown;
  else
    do
      if (!ReadTagPair()) return false;
    while (PeekChar() == '[');

  game->ResetGame(
      false);  // We should do this here (after any FEN tag has been processed)

  if (result == -1)  // Check mandatory result field present.
    return SetError(pgnErr_ResultTagMissing);

  return true;
} /* CPgn::ReadTagSection */

BOOL CPgn::ReadTagPair(void) {
  if (error) return false;

  CHAR s[maxPGNLineLength], tag[maxPGNLineLength], value[maxPGNLineLength];

  if (!ReadToken(s)) return false;
  if (!EqualStr(s, "[")) return SetError(pgnErr_TagStartExpected);
  if (!ReadToken(tag)) return false;
  CleanTagValue();
  if (!ReadToken(value)) return false;
  if (!ReadToken(s)) return false;
  if (!EqualStr(s, "]")) return SetError(pgnErr_TagEndExpected);

  if (SameStr(tag, "White"))
    return CopyString(value, 0, nameStrLen, game->Info.whiteName);
  if (SameStr(tag, "Black"))
    return CopyString(value, 0, nameStrLen, game->Info.blackName);
  if (SameStr(tag, "Event"))
    return CopyString(value, 0, nameStrLen, game->Info.event);
  if (SameStr(tag, "Site"))
    return CopyString(value, 0, nameStrLen, game->Info.site);
  if (SameStr(tag, "Date"))
    return CopyString(value, 0, dateStrLen, game->Info.date);
  if (SameStr(tag, "Round"))
    return CopyString(value, 0, roundStrLen, game->Info.round);
  if (SameStr(tag, "Result")) return ParseResultTag(value);
  if (SameStr(tag, "FEN")) return ParseFENTag(value);

  if (SameStr(tag, "WhiteElo")) return Str2Int(value, &game->Info.whiteELO);
  if (SameStr(tag, "BlackElo")) return Str2Int(value, &game->Info.blackELO);
  if (SameStr(tag, "ECO"))
    return CopyString(value, 0, ecoStrLen, game->Info.ECO);
  if (SameStr(tag, "Annotator"))
    return CopyString(value, 0, nameStrLen, game->Info.annotator);
  // if (SameStr(tag, "Title"))     return CopyString(value,  0, nameStrLen,
  // game->Info.title);
  return true;  // Never mind unknown tags
} /* CPgn::ReadTagPair */

void CPgn::CleanTagValue(void)

// The tag value must be a string (and we know the next token must be a "]").
// However, some PGN files (incorrectly) includes double quotes " in the tag
// value, and this will cause "ReadTagPair" to fail reading the tag properly.
// The "PreprocessTagValue()" function tries to replace illegal double quotes
// with single quotes

{
  if (buf[pos] != '"')
    return;  // Exit immediately if no leading quote in tag value.

  for (LONG i = pos + 1; i < bufSize - 1 && i - pos < 200; i++)
    if (buf[i] == '"') {
      LONG j;

      // First skip past any white space:
      for (j = i + 1; j < i + 10 && j < bufSize && IsWhiteSpace(buf[j]); j++)
        ;

      // We are done if we meet a closing bracket ']':
      if (buf[j] == ']') return;

      // Otherwise it's an illegal double quote, which must be replaced:
      buf[i] = '\'';
    }
} /* CPgn::CleanTagValue */

BOOL CPgn::ParseResultTag(CHAR *s) {
  if (error) return false;

  if (EqualStr(s, "*"))
    result = infoResult_Unknown;
  else if (EqualStr(s, "1/2-1/2"))
    result = infoResult_Draw;
  else if (EqualStr(s, "1/2"))
    result = infoResult_Draw;
  else if (EqualStr(s, "1-0"))
    result = infoResult_WhiteWin;
  else if (EqualStr(s, "0-1"))
    result = infoResult_BlackWin;
  else
    result = infoResult_Unknown;
  return true;
} /* CPgn::ParseResultTag */

BOOL CPgn::ParseFENTag(CHAR *s)

//   [FEN "rnbqr1k1/pp2bppp/4p3/2ppP3/3P3P/2NB1N2/PPP2PP1/R2QK2R w KQ - 0 1"]

{
  if (error) return false;

  INT file, rank;
  BOOL done;

  game->Init.wasSetup = true;

  //--- First parse the actual board configuration:

  ::ClearTable(game->Init.Board);

  for (done = false, rank = 7, file = 0; !done; s++) {
    PIECE p = empty;
    switch (*s) {
      case 'K':
        p = wKing;
        break;
      case 'Q':
        p = wQueen;
        break;
      case 'R':
        p = wRook;
        break;
      case 'B':
        p = wBishop;
        break;
      case 'N':
        p = wKnight;
        break;
      case 'P':
        p = wPawn;
        break;

      case 'k':
        p = bKing;
        break;
      case 'q':
        p = bQueen;
        break;
      case 'r':
        p = bRook;
        break;
      case 'b':
        p = bBishop;
        break;
      case 'n':
        p = bKnight;
        break;
      case 'p':
        p = bPawn;
        break;

      case '/':
        if (file != 8 || rank == 0) return SetError(pgnErr_FENInvalidSquare);
        rank--;
        file = 0;
        break;
      case ' ':
        done = true;
        break;

      case 0:
        return SetError(pgnErr_FENUnterminated);

      default:
        if (IsDigit(*s))
          file += (*s - '0');
        else
          return SetError(pgnErr_FENInvalidChar);
    }

    if (p) {
      if (rank < 0 || file > 7) return SetError(pgnErr_FENInvalidSquare);
      game->Init.Board[(file++) + (rank << 4)] = p;
    }
  }

  //--- Parse initial player field:

  switch (*(s++)) {
    case 'w':
      game->Init.player = white;
      break;
    case 'b':
      game->Init.player = black;
      break;
    default:
      return SetError(pgnErr_FENInvalidInitPlayer);
  }
  if (*(s++) != ' ') return SetError(pgnErr_FENInvalidInitPlayer);

  //--- Parse castling rights field:

  if (*s == ' ') return SetError(pgnErr_FENInvalidCastlingFlags);
  game->Init.castlingRights = 0;

  done = false;
  while (!done) switch (*(s++)) {
      case 'K':
        game->Init.castlingRights |= castRight_WO_O;
        break;
      case 'Q':
        game->Init.castlingRights |= castRight_WO_O_O;
        break;
      case 'k':
        game->Init.castlingRights |= castRight_BO_O;
        break;
      case 'q':
        game->Init.castlingRights |= castRight_BO_O_O;
        break;
      case '-':
        game->Init.castlingRights = 0;
        break;
      case ' ':
        done = true;
        break;
      default:
        return SetError(pgnErr_FENInvalidCastlingFlags);
    }

  //--- Parse en passant square field:

  if (*s == '-')
    game->Init.epSquare = nullSq, s++;
  else if (IsFileLetter(s[0]) && IsRankDigit(s[1])) {
    game->Init.epSquare = *(s++) - 'a';
    game->Init.epSquare += (*(s++) - '1') << 4;

    if (game->Init.player == black)  // Verify en passant square
    {
      if (rank(game->Init.epSquare) != 2 ||
          game->Board[front(game->Init.epSquare)] != wPawn ||
          game->Board[game->Init.epSquare] != empty ||
          game->Board[behind(game->Init.epSquare)] != empty) {
        return SetError(pgnErr_FENInvalidEPSquare);
      }
    } else {
      if (rank(game->Init.epSquare) != 5 ||
          game->Board[behind(game->Init.epSquare)] != bPawn ||
          game->Board[game->Init.epSquare] != empty ||
          game->Board[front(game->Init.epSquare)] != empty) {
        return SetError(pgnErr_FENInvalidEPSquare);
      }
    }
  }
  if (*(s++) != ' ') return SetError(pgnErr_FENInvalidIrrMoves);

  //--- Parse irr move count field:

  game->Init.revMoves = 0;
  while (IsDigit(*s))
    game->Init.revMoves = 10 * game->Init.revMoves + (*(s++) - '0');
  if (game->Init.revMoves >= 100 || *(s++) != ' ')
    return SetError(pgnErr_FENInvalidIrrMoves);

  //--- Parse initial move no field:

  game->Init.moveNo = 0;
  while (IsDigit(*s))
    game->Init.moveNo = 10 * game->Init.moveNo + (*(s++) - '0');
  if (game->Init.moveNo >= 200 || *s)
    return SetError(pgnErr_FENInvalidInitMoveNo);

  //--- Finally check that position is legal:

  return (CheckLegalPosition(game->Init.Board, game->Init.player) == pos_Legal
              ? true
              : SetError(pgnErr_FENIllegalPosition));
} /* CGame::ParseFENTag */

/**************************************************************************************************/
/*                                                                                                */
/*                                      IMPORT MOVE SECTION */
/*                                                                                                */
/**************************************************************************************************/

// The move section is composed of a sequence of tokens and elements: Move
// numbers, moves, comments, recursive annotations, numeric annotation glyphs

BOOL CPgn::ReadMoveSection(void) {
  if (error) return false;

  while (!error && ReadMoveElement())
    ;
  game->Info.result = result;
  return !error;
} /* CPgn::ReadMoveSection */

BOOL CPgn::ReadMoveElement(void) {
  if (error) return false;

  // First check if we have met the next game (and hence that the game
  // termination marker is missing). If so, we stop and consider the current
  // game to have been parsed successfully.

  if (PeekChar() == '[' &&
      pos == linePos)  // We have met the next game and apparently
    return false;      // no game termination marker was given.

  if (pos >= bufSize)  // EOF met and no game terminator -> accept
    return false;      // anyway.

  // Otherwise read next token, and - provided no errors occurred - try to parse
  // the token as a move element.

  CHAR s[maxPGNLineLength];

  if (!ReadToken(s)) return false;

  if (EqualStr(s, "*"))
    return false;  // We are done when we meet game termination
  if (EqualStr(s, "1/2-1/2"))
    return false;  // marker (and we don't care if it doesn't
  if (EqualStr(s, "1/2")) return false;  // match the result tag).
  if (EqualStr(s, "1-0")) return false;
  if (EqualStr(s, "0-1")) return false;
  if (EqualStr(s, "{")) return ParseAnnotation(true);
  if (EqualStr(s, "(")) return ParseRAV();
  if (s[0] == '$') return ParseNAG(s);
  if (IsDigit(s[0])) return ParseMoveNo(s);
  if (IsLetter(s[0])) return ParseMove(s);
  if (EqualStr(s, ";")) return SkipRestOfLine();  // Skip comment lines.
  if (EqualStr(s, "%")) return SkipRestOfLine();
  if (EqualStr(s, ".")) return !error;  // Never mind dots after move numbers
  if (EqualStr(s, "+")) return !error;  // Never mind "orphan" check/mate/ep
  if (EqualStr(s, "#")) return !error;  // markers.
  if (EqualStr(s, "++")) return !error;
  if (SameStr(s, "EP")) return !error;
  return SetError(pgnErr_MoveElemErr);
} /* CPgn::ReadMoveElement */

/*----------------------------------------- Parse Move
 * -------------------------------------------*/
// This routine parses a move token. If successful, the move is performed (and
// thus added to the game record).

BOOL CPgn::ParseMove(CHAR *s) {
  if (error) return false;

  MOVE m, *M;
  INT fromFile = -1, fromRank = -1;
  INT len, suffix = 0;

  m.piece = empty;
  m.from = nullSq;
  m.to = nullSq;
  m.type = normal;
  m.cap = empty;

  //--- Initially extract trailing suffix annotations and check/mate indicates
  //(if any)

  for (len = 0; s[len]; len++)
    ;  // Calculate string length

  if (len < 2) return SetError(pgnErr_MoveSyntaxErr);

  if (s[len - 1] == '!' || s[len - 1] == '?')  // Fetch and parse any suffix
  {
    len--;  // annotations (1 or 2 chars).
    if (s[len - 1] == '!' || s[len - 1] == '?') len--;
    suffix = ParseSuffix(&s[len]);
  }

  if (len < 2) return SetError(pgnErr_MoveSyntaxErr);

  if (s[len - 1] == '+' || s[len - 1] == '#')  // Ignore check/mate indications.
  {
    len--;
    if (s[len - 1] == '+') len--;  // We also have to accept ++ for #.
  }

  if ((s[len - 2] == 'e' ||
       s[len - 2] == 'E') &&  // Skip any en passant indicators.
      (s[len - 1] == 'p' || s[len - 1] == 'P'))
    len -= 2;

  if (len < 2) return SetError(pgnErr_MoveSyntaxErr);

  s[len] = 0;  // Terminate string if needed. IMPORTANT for EqualStr to work.

  //--- First gather preliminary info about move:

  switch (s[0]) {
    case 'K':
      m.piece = game->player + king;
      break;
    case 'Q':
      m.piece = game->player + queen;
      break;
    case 'R':
      m.piece = game->player + rook;
      break;
    case 'B':
      m.piece = game->player + bishop;
      break;
    case 'N':
      m.piece = game->player + knight;
      break;
    case 'O':
    case 'o':
    case '0':
      m.piece = game->player + king;
      m.from = (game->player == white ? e1 : e8);
      if (EqualStr(s, "O-O") || EqualStr(s, "o-o") || EqualStr(s, "0-0"))
        m.type = mtype_O_O, m.to = (game->player == white ? g1 : g8);
      else if (EqualStr(s, "O-O-O") || EqualStr(s, "o-o-o") ||
               EqualStr(s, "0-0-0"))
        m.type = mtype_O_O_O, m.to = (game->player == white ? c1 : c8);
      else
        return SetError(pgnErr_CastlingMoveErr);
      break;
    default:
      if (IsFileLetter(s[0]))
        m.piece = game->player + pawn, fromFile = s[0] - 'a';
      else
        return SetError(pgnErr_MoveSyntaxErr);
  }

  //--- Next decode the entire move:

  if (pieceType(m.piece) == pawn) {
    switch (s[len - 1])  // Check if it's a promotion:
    {
      case 'Q':
        m.type = game->player + queen;
        break;
      case 'R':
        m.type = game->player + rook;
        break;
      case 'B':
        m.type = game->player + bishop;
        break;
      case 'N':
        m.type = game->player + knight;
        break;
    }
    if (isPromotion(m)) {
      len--;
      if (s[len - 1] == '=')
        len--;  // Danny Mozes sometimes forgets the '=' sign!!
    }

    if (len == 2 || len == 4)  // Check move string length and fetch
    {                          // destination square.
      INT i = len - 2;
      if (IsFileLetter(s[i]) && IsRankDigit(s[i + 1]))
        m.to = (s[i] - 'a') + ((s[i + 1] - '1') << 4);
      else
        return SetError(pgnErr_MoveSyntaxErr);
    } else if (len == 5)  // Handle long notation (e.g "d2-d4"). FromFile has
                          // already been set above
    {
      if (IsRankDigit(s[1]) && IsFileLetter(s[3]) && IsRankDigit(s[4])) {
        fromRank = s[1] - '1';
        m.to = square(s[3] - 'a', s[4] - '1');
      } else
        return SetError(pgnErr_MoveSyntaxErr);
    } else
      return SetError(pgnErr_MoveSyntaxErr);

    if (len > 3 && s[len - 3] == 'x')  // Check if it's a capture (or enpassant)
    {
      m.cap = game->Board[m.to];
      if (!m.cap) m.type = mtype_EP;
    }
  } else if (m.type == normal)  // Parse normal moves (castling already done):
  {
    if (len == 6)  // Handle long notation (e.g "Ke1-e2")
    {
      if (IsFileLetter(s[1]) && IsRankDigit(s[2]) && IsFileLetter(s[4]) &&
          IsRankDigit(s[5])) {
        fromFile = s[1] - 'a';
        fromRank = s[2] - '1';
        m.to = square(s[4] - 'a', s[5] - '1');
        m.cap = game->Board[m.to];
      } else
        return SetError(pgnErr_MoveSyntaxErr);
    } else {
      INT i = len - 2;  // Fetch destination square.
      if (i < 0 || !IsFileLetter(s[i]) || !IsRankDigit(s[i + 1]))
        return SetError(pgnErr_MoveSyntaxErr);
      m.to = (s[i] - 'a') + ((s[i + 1] - '1') << 4);
      m.cap = game->Board[m.to];

      i--;  // Read capture/hyphen if any
      if (s[i] == 'x' || s[i] == '-') i--;

      for (INT j = 1; j <= i; j++)  // Read source square
        if (IsRankDigit(s[j]))
          fromRank = s[j] - '1';  // disambiguation (if any).
        else if (IsFileLetter(s[j]))
          fromFile = s[j] - 'a';
        else
          return SetError(pgnErr_MoveSyntaxErr);
    }
  }

  //--- Finally check if move is legal, and if so perform it:

  INT j;
  for (j = 0, M = game->Moves; j < game->moveCount; j++, M++)
    if (m.to == M->to && m.piece == M->piece && m.cap == M->cap &&
        m.type == M->type)
      if ((fromFile == -1 || file(M->from) == fromFile) &&
          (fromRank == -1 || rank(M->from) == fromRank)) {
        game->PlayMove(M);
        game->SetAnnotationGlyph(game->currMove, suffix);
        return true;
      }

  return SetError(pgnErr_IllegalMove);
} /* CPgn::ParseMove */

/*--------------------------------------- Parse Misc Move Stuff
 * ----------------------------------*/

BOOL CPgn::ParseAnnotation(BOOL keep)  // Read and store annotation record.
{
  if (error) return false;

  LONG pos0 = pos;

  while (!error) switch (ReadChar()) {
        //       case '"' :       REMOVED: Causes problems with unterminated
        //       qoutes
        //          SkipStringToken(); break;
      case '}':
        if (keep && !(flags & pgnFlag_SkipAnn))
          game->SetAnnotation(game->currMove, (CHAR *)&buf[pos0],
                              pos - pos0 - 1, !Prefs.PGN.keepNewLines);
        return StripWhiteSpace();
    }

  return !error;
} /* CPgn::ParseAnnotation */

BOOL CPgn::ParseRAV(void)  // ### Currently we just skip RAVs.
{
  INT n = 1;

  if (error) return false;

  while (!error && n != 0) switch (ReadChar()) {
      case '(':
        n++;
        break;
      case ')':
        n--;
        if (n < 0) SetError(pgnErr_InvalidRAV);
        break;
      case '"':
        SkipStringToken();
        break;
      case '{':
        ParseAnnotation(false);
        break;
    }

  if (n == 0) StripWhiteSpace();
  return !error;
} /* CPgn::ParseRAV */

BOOL CPgn::ParseNAG(CHAR *s)  // ### Currently we just skip NAGs.
{
  return true;
} /* CPgn::ParseNAG */

INT CPgn::ParseSuffix(CHAR *s)  // Parses suffix (!,?,!!,??,!?,?!)
{
  if (error) return 0;

  if (EqualStr(s, "!")) return 1;
  if (EqualStr(s, "?")) return 2;
  if (EqualStr(s, "!!")) return 3;
  if (EqualStr(s, "??")) return 4;
  if (EqualStr(s, "!?")) return 5;
  if (EqualStr(s, "?!")) return 6;
  return 0;
} /* CPgn::ParseSuffix */

BOOL CPgn::ParseMoveNo(CHAR *s)  // We don't really care about move numbers.
{                                // However, some annotaters use the digit zero
  if (error) return false;       // to represent castling moves (!?!?).
  if (s[0] == '0' && s[1] == '-') return ParseMove(s);
  return !error;
} /* CPgn::ParseMoveNo */

/**************************************************************************************************/
/*                                                                                                */
/*                                         TOKEN PARSING */
/*                                                                                                */
/**************************************************************************************************/

// This is the basic low level parsing routine that reads and returns the next
// token. It also strips any trailing white space so that the buffer pointer
// (inx) is located at the first char of the next token.

BOOL CPgn::ReadToken(CHAR *t) {
  CHAR c;
  LONG i, n, pos0 = pos;

  if (error) return false;

  //--- Store info about token in case of errors:

  errLineNo =
      totalLinesRead;    // Global line number [0...] of next potential error
  errLinePos = linePos;  // Local position of error line
  errTokenPos = pos;     // Local position of error token

  //--- Read the actual token:

  c = ReadChar();
  if (error) return false;

  switch (c) {
    case '[':
    case ']':  // Self terminating tokens:
    case '{':
    case '}':
    case '(':
    case ')':
    case '<':
    case '>':
    case '.':
    case '*':
    case '#':  // # is not really a token (but an orphan mate indicator)
      *(t++) = c;
      break;

    case '+':  // Handle orphan check/mate indicators (+ and ++)
      *(t++) = c;
      if (ReadChar() == '+') *(t++) = c;
      break;

    case '$':  // Numeric annotation glyph:
      *(t++) = c;
      for (i = n = 0; i < 3 && IsDigit(PeekChar()) && !error; i++) {
        *(t++) = c = ReadChar();
        n = 10 * n + (c - '0');
      }
      if (i == 0 || n > 255) SetError(pgnErr_InvalidNAG);
      break;
    case '"':  // String token:
      pos0 = pos;
      do {
        c = ReadChar();
        if (pos >= pos0 + maxPGNLineLength - 5)
          SetError(pgnErr_StrTokenTooLong);
        else if (c == backSlash &&
                 (PeekChar() == '"' || PeekChar() == backSlash))
          *(t++) = ReadChar();
        else if (IsNewLine(c) || IsTabChar(c))
          SetError(pgnErr_NewLineTabInString);
        else if (c != '"')
          *(t++) = c;
      } while (!error && c != '"');
      break;

    case ';':  // Comment & Escape characters...
    case '%':  // ...only at beginning of line
      if (pos - 1 == linePos)
        *(t++) = c;
      else
        SetError(pgnErr_CommentEscapeLineStart);
      break;

    default:
      if (c >= 127)             // Convert ASCII values above 126
        *(t++) = '?';           // to the '?' character.
      else if (!IsAlphaNum(c))  // Symbol token:
        SetError(pgnErr_InvalidToken);
      else {
        pos0 = pos;
        *(t++) = c;
        while (!error && pos < bufSize && IsSymbolChar(PeekChar()))
          if (pos >= pos0 + maxPGNLineLength - 5)
            SetError(pgnErr_TokenTooLong);
          else
            *(t++) = ReadChar();
      }
  }

  *t = 0;  // Terminate return string.

  //--- Store token length:

  errTokenLen = pos - errTokenPos;

  //--- Finally strip white space (and thus move on to next token):

  StripWhiteSpace();
  return !error;
} /* CPgn::ReadToken */

/**************************************************************************************************/
/*                                                                                                */
/*                                         IMPORT UTILITY */
/*                                                                                                */
/**************************************************************************************************/

CHAR CPgn::ReadChar(void) {
  if (error) return 0;

  if (pos >= bufSize) {
    SetError(pgnErr_UnexpectedEOF);
    return 0;
  } else {
    if (IsNewLine(buf[pos])) {
      linePos = pos + 1;
      totalLinesRead++;
    }
    totalBytesRead++;
    return buf[pos++];
  }
} /* CPgn::ReadChar */

BOOL IsSymbolChar(CHAR c) {
  return (IsAlphaNum(c) || c == '_' || c == '+' || c == '#' || c == '=' ||
          c == ':' || c == '-' || c == '/' || c == '!' || c == '?');
} /* IsSymbolChar */

BOOL IsWhiteSpace(CHAR c) {
  return (c == ' ' || IsNewLine(c) || IsTabChar(c));
} /* IsWhiteSpace */

BOOL IsFileLetter(CHAR c) { return (c >= 'a' && c <= 'h'); } /* IsFileLetter */

BOOL IsRankDigit(CHAR c) { return (c >= '1' && c <= '8'); } /* IsRankDigit */

BOOL CPgn::StripWhiteSpace(void) {
  while (pos < bufSize && !error && IsWhiteSpace(PeekChar())) ReadChar();
  return !error;
} /* CPgn::StripWhiteSpace */

BOOL CPgn::CopyString(CHAR *s, INT minLen, INT maxLen, CHAR *t) {
  if (EqualStr(s, "?"))  // ### Only test for tags!!
    *t = 0;
  else {
    INT len = 0;

    while (*s)
      if (++len <= maxLen)
        *(t++) = *(s++);
      else {
        *t = 0;
        return true;
        // return SetError(pgnErr_StrTokenTooLong);
      }

    if (len >= minLen)
      *t = 0;
    else
      return SetError(pgnErr_StrTokenTooShort);
  }

  return !error;
} /* CPgn::CopyString */

BOOL CPgn::Str2Int(CHAR *s, INT *n) {
  *n = -1;

  if (error)
    return false;
  else if (EqualStr(s, "") || EqualStr(s, "-") || EqualStr(s, "?"))
    return true;
  else if (StrLen(s) > 5)
    return SetError(pgnErr_InvalidNumber);
  else {
    INT N = 0;
    while (*s) {
      if (!IsDigit(*s)) return SetError(pgnErr_InvalidNumber);
      N = 10 * N + (*(s++) - '0');
    }
    *n = N;
    return true;
  }
} /* CPgn::Str2Int */

BOOL CPgn::SkipRestOfLine(void) {
  while (!error && !IsNewLine(ReadChar()))
    ;
  return !error;
} /* SkipRestOfLine */

BOOL CPgn::SkipStringToken(void)  // If a double quote " has just been read,
{                                 // this routine skips past this string token
  CHAR c;

  if (error) return false;

  do {
    c = ReadChar();
    if (c == backSlash && (PeekChar() == '"' || PeekChar() == backSlash))
      ReadChar();
  } while (!error && c != '"');

  return !error;
} /* SkipStringToken */

/**************************************************************************************************/
/*                                                                                                */
/*                                         ERROR HANDLING */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- Error Generation
 * ---------------------------------------*/

BOOL CPgn::SetError(PGN_ERROR pgnErrorVal) {
  error = pgnErrorVal;
  if (error == pgnErr_NoError) errLineNo = 0;
  return error;
} /* CPgn::SetError */

/*-------------------------------------- Build Error Message
 * -------------------------------------*/
// Convert PGN error code into string and indicate if error token should be
// shown.

void CPgn::CalcErrorStats(LONG *line, INT *column, CHAR *s, CHAR *errLine) {
  // Set error line & pos:
  *line = errLineNo + 1;
  *column = errTokenPos - errLinePos + 1;

  // Build error message string:
  errLine[0] = 0;

  switch (error) {
    case pgnErr_ResultTagMissing:
      CopyStr("Result tag missing", s);
      break;
    case pgnErr_TagStartExpected:
      CopyStr("[ expected (tag start):", s);
      break;
    case pgnErr_TagEndExpected:
      CopyStr("] expected (tag end)", s);
      break;
    case pgnErr_InvalidResultTag:
      CopyStr("Invalid result tag", s);
      break;
    case pgnErr_FENInvalidSquare:
      CopyStr("Invalid square in FEN tag", s);
      break;
    case pgnErr_FENUnterminated:
      CopyStr("FEN tag unterminated", s);
      break;
    case pgnErr_FENInvalidChar:
      CopyStr("Invalid char in FEN tag", s);
      break;
    case pgnErr_FENInvalidInitPlayer:
      CopyStr("Invalid initial player in FEN tag", s);
      break;
    case pgnErr_FENInvalidCastlingFlags:
      CopyStr("Invalid castling flags in FEN tag", s);
      break;
    case pgnErr_FENInvalidEPSquare:
      CopyStr("Invalid EP square in FEN tag", s);
      break;
    case pgnErr_FENInvalidIrrMoves:
      CopyStr("Invalid draw count in FEN tag", s);
      break;
    case pgnErr_FENInvalidInitMoveNo:
      CopyStr("Invalid initial move no in FEN tag", s);
      break;
    case pgnErr_FENIllegalPosition:
      CopyStr("Illegal position in FEN tag", s);
      break;
    case pgnErr_MoveElemErr:
      CopyStr("Unrecognized move element:", s);
      break;
    case pgnErr_MoveSyntaxErr:
      CopyStr("Move syntax error:", s);
      break;
    case pgnErr_CastlingMoveErr:
      CopyStr("Castling move syntax error:", s);
      break;
    case pgnErr_IllegalMove:
      CopyStr("Illegal move:", s);
      break;
    case pgnErr_InvalidRAV:
      CopyStr("Invalid RAV", s);
      break;
    case pgnErr_InvalidNAG:
      CopyStr("Invalid NAG:", s);
      break;
    case pgnErr_NewLineTabInString:
      CopyStr("Strings may not contain new lines/tabs", s);
      break;
    case pgnErr_CommentEscapeLineStart:
      CopyStr("Comments/escape sequences must begin on new line", s);
      break;
    case pgnErr_InvalidToken:
      CopyStr("Invalid token:", s);
      break;
    case pgnErr_UnexpectedEOF:
      CopyStr("Unexpected end of file", s);
      return;
    case pgnErr_TokenTooLong:
      CopyStr("Token too long:", s);
      break;
    case pgnErr_StrTokenTooLong:
      CopyStr("String too long", s);
      break;
    case pgnErr_StrTokenTooShort:
      CopyStr("String too short", s);
      break;
    case pgnErr_InvalidNumber:
      CopyStr("Number expected:", s);
      break;
    default:
      s[0] = 0;
      return;
  }

  INT n = StrLen(s);
  if (s[n - 1] == ':')  // Show error token if last char is colon!
  {
    s[n++] = ' ';
    for (INT i = 0; i < errTokenLen; i++) s[n + i] = buf[errTokenPos + i];
    s[n + errTokenLen] = 0;
  }

  // Build error line string:
  INT i;
  CHAR *lns = (CHAR *)&buf[errLinePos];
  for (i = 0; i < 150 && lns[i] >= 32 && lns[i] <= 127 && !IsNewLine(lns[i]);
       i++)
    errLine[i] = lns[i];
  while (i > 0 && errLine[i - 1] == ' ') i--;  // Skip trailing blanks
  errLine[i] = 0;

} /* CPgn::CalcErrorMessage */

BOOL CPgn::SkipGame(void)  // Look for beginning of next game (newline followed
                           // by tag start '[')
{
  CHAR c;

  error = pgnErr_NoError;
  do {
    c = ReadChar();
    if (!error && pos >= bufSize) SetError(pgnErr_UnexpectedEOF);
  } while (!(IsNewLine(c) && PeekChar() == '[') && !error);

  return !error;
} /* CPgn::SkipGame */

LONG CPgn::GetBytesRead(void) { return pos; } /* CPgn::GetBytesRead */

LONG CPgn::GetTotalBytesRead(void) {
  return totalBytesRead;
} /* CPgn::GetTotalBytesRead */

PGN_ERROR CPgn::GetError(void) { return error; } /* CPgn::GetError */
