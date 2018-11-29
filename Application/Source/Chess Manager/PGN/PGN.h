#pragma once

#include "Game.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

enum PGN_FLAGS  // All flags are off per default
{ pgnFlag_None = 0x0000,
  pgnFlag_SkipMoveSep = 0x0001,
  pgnFlag_AllowBlankTag = 0x0002,
  pgnFlag_SkipAnn = 0x0004,
  pgnFlag_All = 0x7FFF };

enum PGN_ERROR {
  pgnErr_NoError = 0,
  pgnErr_ResultTagMissing,
  pgnErr_TagStartExpected,
  pgnErr_TagEndExpected,
  pgnErr_InvalidResultTag,
  pgnErr_FENInvalidSquare,
  pgnErr_FENUnterminated,
  pgnErr_FENInvalidChar,
  pgnErr_FENInvalidInitPlayer,
  pgnErr_FENInvalidCastlingFlags,
  pgnErr_FENInvalidEPSquare,
  pgnErr_FENInvalidIrrMoves,
  pgnErr_FENInvalidInitMoveNo,
  pgnErr_FENIllegalPosition,
  pgnErr_MoveElemErr,
  pgnErr_MoveSyntaxErr,
  pgnErr_CastlingMoveErr,
  pgnErr_IllegalMove,
  pgnErr_InvalidRAV,
  pgnErr_InvalidNAG,
  pgnErr_NewLineTabInString,
  pgnErr_CommentEscapeLineStart,
  pgnErr_InvalidToken,
  pgnErr_TokenTooLong,
  pgnErr_StrTokenTooLong,
  pgnErr_StrTokenTooShort,
  pgnErr_InvalidNumber,
  pgnErr_UnexpectedEOF,
  pgnErr_EOFReached  // Not really an error
};

#define pgn_BufferSize 64000L
#define maxPGNLineLength 1000
#define backSlash 0x5C

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- The CGame Class
 * -----------------------------------------*/
// The CGame class holds all information pertaining to a game:
// * Initial board configuration incl. castling rights and en passant status (if
// initial position
//   was set up).
// * The actual game record holding all the moves of the game, incl. draw state
// and game result
// * The current board configuration, incl. all strictly legal moves.
// * Game information such as player names, event, site, date, round e.t.c.
// * Auxiliary game information (such as annotations).

class CPgn {
 public:
  /*----------------------------------- Public Methods
   * ------------------------------------------*/

  CPgn(CGame *game, PGN_FLAGS flags = pgnFlag_None);

  // PGN Export
  LONG WriteGame(
      CHAR *pgnBuf);  // Writes game to PGN buffer (and returns size).
  LONG WriteFEN(
      CHAR *fenBuf);  // Creates a RAW (untagged) FEN string (and returns size).

  // PGN Import
  void ReadBegin(CHAR *pgnBuf);
  BOOL ReadGame(LONG pgnBufSize);
  BOOL SkipGame(void);
  LONG GetBytesRead(void);
  LONG GetTotalBytesRead(void);
  PGN_ERROR GetError(void);
  void CalcErrorStats(LONG *line, INT *pos, CHAR *errMsg, CHAR *errLine);

  BOOL ParseMove(CHAR *s);

 private:
  /*----------------------------------- Private Methods
   * -----------------------------------------*/

  //--- Writing/Saving PGN ---

  void WriteTagSection(void);
  void WriteMoveSection(void);
  void WriteStr(CHAR *s);
  void WriteStrBS(CHAR *s);
  void WriteChar(CHAR c);
  void WriteInt(INT n);

  void WriteTagStr(CHAR *tag, CHAR *s);
  void WriteTagInt(CHAR *tag, INT n, BOOL skipIfBlank);
  void WriteResult(INT result);
  void WriteTagFEN(void);

  void WriteMoveNo(INT i, COLOUR player, BOOL forceWrite);
  void WriteMove(MOVE *m, BYTE flags, BYTE glyph);
  void WriteOrigin(MOVE *m);
  void WrapLines(LONG imin, LONG imax);
  void WriteAnnText(INT j);

  //--- Reading/Loading PGN ---

  BOOL SetError(PGN_ERROR pgnErrorVal);

  BOOL CheckEmptyTagSection(void);
  void LocateTags(void);
  void ReadPreprocess(void);
  BOOL ReadTagSection(void);
  BOOL ReadTagPair(void);
  void CleanTagValue(void);
  BOOL ParseResultTag(CHAR *s);

  BOOL ParseFENTag(CHAR *s);

  BOOL ReadMoveSection(void);
  BOOL ReadMoveElement(void);
  BOOL ParseAnnotation(BOOL keep);
  BOOL ParseRAV(void);
  BOOL ParseNAG(CHAR *s);
  INT ParseSuffix(CHAR *s);
  BOOL ParseMoveNo(CHAR *s);
  BOOL SkipRestOfLine(void);
  BOOL SkipStringToken(void);

  CHAR ReadChar(void);
  BOOL ReadToken(CHAR *s);
  BOOL CopyString(CHAR *s, INT minLen, INT maxLen, CHAR *target);
  BOOL Str2Int(CHAR *s, INT *n);
  BOOL StripWhiteSpace(void);

  /*---------------------------------- Instance Variables
   * ---------------------------------------*/

  CGame *game;  // Game object associated with PGN object.

  INT flags;  // Customization flags

  PTR buf;       // Pointer to current PGN Buffer
  LONG bufSize;  // Total size of PGN Import buffer.
  LONG pos;  // Local index in PGN buffer of current char being read/written.
  LONG linePos;  // Local index in PGN buffer of most recent line start

  LONG totalBytesRead;  // Total number of bytes read
  LONG totalLinesRead;  // Total number of lines read

  BOOL emptyTagSection;

  INT result;  // Result value (and string). We cannot use gameInfo->result

  PGN_ERROR error;  // = pgnErr_NoError if parsed and processed successfully.
  LONG errLineNo,   // Last saved line before any errors.
      errLinePos,   // Last saved pos before any errors.
      //           errColumn,            // Column of erroneous token.
      errTokenPos,  // Position of erroneous token.
      errTokenLen;  // String length of erroneous token.
};

/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

BOOL IsSymbolChar(CHAR c);
BOOL IsWhiteSpace(CHAR c);
BOOL IsFileLetter(CHAR c);
BOOL IsRankDigit(CHAR c);
