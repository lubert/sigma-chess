#pragma once

#include "General.h"
#include "Board.h"
#include "Move.h"
#include "HashCode.h"
#include "Engine.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

//#define gameRecWidth  220       // Width in pixels of game record (### SHOULDN'T BE HERE???)

#define gameRecSize      800          // Maximum game length (in game half moves).

#define gameDataSize    64000  // Size of generic/utility game buffer.

/*---------------------------------------- String Lengths ----------------------------------------*/

#define gameMoveStrLen  20    // R(QB3)xN(QN3)+?? (maxlen = 16 + EOF)
#define nameStrLen      50
#define dateStrLen      10
#define ecoStrLen       10    // Actually 6 is enough (XDD or XDD/DD), but just in case...
#define roundStrLen     10

/*-------------- Castling rights in initial position (GAME.Init.castlingRights) ------------------*/

#define castRight_WO_O        0x01         // Bit 0 : White king  sides castling.
#define castRight_WO_O_O      0x02         // Bit 1 : White queen sides castling.
#define castRight_BO_O        0x04         // Bit 2 : Black king  sides castling.
#define castRight_BO_O_O      0x08         // Bit 3 : Black queen sides castling.

/*--------------------------------- Game results (GAME.result) -----------------------------------*/
// "Hard" (automatically computed) results; when a new game move has been performed
// The game can NOT continue in these cases:

#define result_Unknown        0            // Unknown so far.
#define result_Mate           1            // Mate.
#define result_StaleMate      2            // Draw due to stale mate
#define result_Draw3rd        3            // Draw by repetition.
#define result_Draw50         4            // Draw by the 50 move rule.
#define result_DrawInsMtrl    5            // Draw by insufficient material.

// "Soft" (manually set) results; in case of draw accepted, resignation or time forfeit
// The game can still continue in these cases.
#define result_DrawAgreed     6            // Draw agreed (offered and accepted).
#define result_Resigned       7            // Game resigned.
#define result_TimeForfeit    8            // Game lost on time.

/*-------------------------- Game Info (PGN) results (GAME.Info.result) --------------------------*/

#define infoResult_Unknown    1     // Is compatible with the version 4 results.
#define infoResult_Draw       2
#define infoResult_WhiteWin   3
#define infoResult_BlackWin   4

/*----------------------------------------- EPD Constants ----------------------------------------*/

#define epdBufSize            200

enum EPD_ERROR
{
   epdErr_NoError = 0,
   epdErr_InvalidSquare,
   epdErr_UnexpectedEnd,
   epdErr_InvalidChar,
   epdErr_InvalidInitialPlayer,
   epdErr_InvalidCastlingFlags,
   epdErr_InvalidEPSquare
};

/*----------------------------------------- PGN Constants ----------------------------------------*/

enum GAMEINFO_TAG
{
   tag_Null         = 0,
   
   // PGN Supported Game Info tags:
   tag_WhiteName    =  1,
   tag_BlackName    =  2,
   tag_Event        =  3,
   tag_Site         =  4,
   tag_Date         =  5,
   tag_Round        =  6,
   tag_Result       =  7,
   tag_WhiteELO     =  8,
   tag_BlackELO     =  9,
   tag_ECO          = 10,
   tag_Opening      = 11,
   tag_Annotator    = 12,
// tag_Title        = 13,
/*
   tag_WhiteNameInx = 20,
   tag_BlackNameInx = 21,
   tag_EventInx     = 22,
   tag_SiteInx      = 23,
   tag_OpeningInx   = 24,
   tag_AnnotatorInx = 25,

   tag_Generic0     = 30,
   tag_Generic1     = 31,
   tag_Generic2     = 32,
   tag_Generic3     = 33,
   tag_Generic4     = 34,
   tag_Generic5     = 35,
   tag_Generic6     = 36,
   tag_Generic7     = 37,
   tag_Generic8     = 38,
   tag_Generic9     = 39,
*/
   // Private Sigma Chess tags (not supported by PGN):
   tag_Layout       = 40
};

/*--------------------------------- Position Legality Constants ----------------------------------*/

enum POSITION_LEGALITY
{
   pos_Legal = 0,
   pos_TooManyWhitePawns,
   pos_TooManyBlackPawns,
   pos_WhiteKingMissing,
   pos_BlackKingMissing,
   pos_TooManyWhiteKings,
   pos_TooManyBlackKings,
   pos_TooManyWhiteOfficers,
   pos_TooManyBlackOfficers,
   pos_PawnsOn1stRank,
   pos_OpponentInCheck
};

/*---------------------------------------- Move Notation -----------------------------------------*/

enum SCORE_NOTATION
{
   scoreNot_NumRel = 0,   // Numerical as seen from side to move (default)
   scoreNot_NumAbs = 1,   // Numerical as seen from White
   scoreNot_Glyph  = 2    // Position library annotation glyph (as seen from white)
};

/*---------------------------------------- Move Notation -----------------------------------------*/

enum MOVE_NOTATION
{
   moveNot_Short = 0,   // Short algebraic
   moveNot_Long  = 1,   // Long algebraic
   moveNot_Descr = 2    // Descriptive
};

/*--------------------------- Heading Types (Collection Publishing) ------------------------------*/

enum HEADING_TYPE
{
   headingType_None       = 0,
   headingType_GameNo     = 1,
   headingType_Chapter    = 2,
   headingType_Section    = 3
};

/*--------------------------------- Game Record Line Map Constants -------------------------------*/

#define gameMapSize         4000

// Game map entry flags (upper bits of GameMap[].moveNo) indicating type of line:

#define gameMap_White      0x8000      // Line contains a white move
#define gameMap_Black      0x4000      // Line contains a black move
#define gameMap_Move       0xC000      // Line contains a white and/or black move
#define gameMap_Special    0x2000      // Special line (e.g. title, or game info).
#define gameMap_AnnLine    0x0000      // Line contains annotation text line.

// If the gameMap_Special flag is set in GameMap[n].moveNo, then the following contents of the
// the GameMap[n].txLine describes the actual contents of the special line:

#define gameMap_SpecialBlank     0      // Blank line
#define gameMap_SpecialChapter   1      // Collection game title as chapter (bold, larger)
#define gameMap_SpecialSection   2      // Collection game title as section (bold, larger)
#define gameMap_SpecialGmTitle   3      // Collection game no + title (italic, larger)
#define gameMap_SpecialWhite     4      // GameInfo.whiteName (incl. ELO)
#define gameMap_SpecialBlack     5      // GameInfo.blackName (incl. ELO)
#define gameMap_SpecialEvent     6      // GameInfo.event
#define gameMap_SpecialSite      7      // GameInfo.site
#define gameMap_SpecialDate      8      // GameInfo.date
#define gameMap_SpecialRound     9      // GameInfo.round
#define gameMap_SpecialResult   10      // GameInfo.result
#define gameMap_SpecialECO      11      // GameInfo.result


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Initial Game State --------------------------------------*/

typedef struct
{
   BOOL   wasSetup;                    // Was initial position set up?

   PIECE  Board[boardSize];            // Initial board configuration.
   COLOUR player;                      // Side to start in initial position.
   INT    castlingRights;              // Initial castling rights.
   SQUARE epSquare;                    // En passant status. Destination of previous double pawn
                                       // pawn, or "nullSq" if no en passant in initial position.
   INT    moveNo;                      // Initial move number.
   INT    revMoves;                    // Number of half moves played since last irreversible move. 
} INITGAME;

/*----------------------------------- The GAME INFO Structure ------------------------------------*/

typedef struct
{
   // Mandatory PGN tags:
   CHAR   whiteName[nameStrLen + 1];
   CHAR   blackName[nameStrLen + 1];
   CHAR   event[nameStrLen + 1];
   CHAR   site[nameStrLen + 1];
   CHAR   date[dateStrLen + 1];
   CHAR   round[roundStrLen + 1];
   INT    result;

   // Optional PGN tags:
   INT    whiteELO;
   INT    blackELO;
   CHAR   ECO[ecoStrLen + 1];
   CHAR   annotator[nameStrLen + 1];

   // Sigma specific lauout info for basic desktop publishing based on game collections
   // (stored using non-standard PGN tags):
   BOOL   pageBreak;                  // Page break in print? Default off.
   INT    headingType;                // Heading text categori. Default none.
   CHAR   heading[nameStrLen + 1];    // The actual heading text (if used).
   BOOL   includeInfo;                // Include game info in display/print.
} GAMEINFO;

/*------------------------------------ The PACKET Structure --------------------------------------*/
// Used for loading/saving version 3.0 & 4.0 games.

typedef struct
{  
   ULONG  type;        // E.g. 'SIZE', 'INFO', 'INIT', 'MOVE', 'gmfl', 'ann '
   ULONG  size;        // Size in bytes of packet data (must be even).
   BYTE   Data[];      // The actual data. Must be an even number of bytes.
} PACKET;

/*--------------------------------- The Version 2.0 Game Format ----------------------------------*/

typedef struct
{
	BYTE		gameFormat;			// 2
	Str63		whitePlayerInfo;		
	Str63		blackPlayerInfo;

	BYTE		wasSetup;			// 1 if initial position was setup, 0 otherwise
	BYTE		intialPlayer;		// Player to start (only used if setup = 1)
	BYTE		InitialPos[64];	// Initial position (only used if setup = 1)

	BYTE		Game[];				// Null move terminated list of moves. >= 4 bytes pr move.
										// The first 3 bytes specifies from,to,type. The last byte
										// begins is a null terminated comment string (prior to the move).
} GAMEDATA2;

/*----------------------------------- The Game Record Line Map -----------------------------------*/

typedef struct                  // Game record map.
{
   INT       moveNo;            // Bits 15 and 14 indicate if a white and/or black move is to
   INT       txLine;
} GAMEMAP;


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class CPgn;
class CAnnotation;

/*-------------------------------------- The CGame Class -----------------------------------------*/
// The CGame class holds all information pertaining to a game:
// * Initial board configuration incl. castling rights and en passant status (if initial position
//   was set up).
// * The actual game record holding all the moves of the game, incl. draw state and game result
// * The current board configuration, incl. all strictly legal moves.
// * Game information such as player names, event, site, date, round e.t.c.
// * Auxiliary game information (such as annotations).

class CGame
{
public:

   /*----------------------------------- Public Methods ------------------------------------------*/

   CGame (void);
   virtual ~CGame(void);

   //--- New game ---
   void   NewGame          (BOOL resetInfo = true);  // Start new game.
   void   ResetInit        (void);
   void   ResetGame        (BOOL resetInfo = true);  // Reset game from initial game state.
   void   ResetGameInfo    (void);
   void   ClearGameInfo    (void);

   //--- Game Record Access ---
   void   PlayMove         (MOVE *m);            // Add new move to game record.
   void   PlayMoveRaw      (MOVE *m);            // Add new move to game record (no flags, disamb etc)
   void   RedoMove         (BOOL refresh);       // Redo next game move.
   void   UndoMove         (BOOL refresh);       // Undo last game move.
   void   RedoAllMoves     (void);               // Redo all game moves.
   void   UndoAllMoves     (void);               // Undo all game moves.
   BOOL   CanUndoMove      (void);               // Can moves be undone in current position?
   BOOL   CanRedoMove      (void);               // Can moves be redone in current position?
   INT    MovesPlayed      (void);               // Get number of whole game moves played (for player).
   INT    GetMoveNo        (void);               // Get real move number
   MOVE*  GetGameMove      (INT i);              // Get i'th game move (1 <= i <= GameMoveCount).
   BOOL   GameOver         (void);
   BOOL   UpdateInfoResult (void);
   void   CalcStatusStr    (CHAR *str);          // Max length 100 chars
   void   CalcMoves        (void);
   void   CopyFrom         (CGame *src, BOOL allmoves = true, BOOL includeinfo = true, BOOL includeann = true);

   //--- Board/Game Access ---
   PIECE  GetPiece         (SQUARE sq);          // Get the piece (if any) on the specified square.
   COLOUR GetPlayer        (void);               // Get the side to move.
   INT    GetBoardMoveCount(void);               // Get number of legal moves in current position.
   MOVE*  GetBoardMove     (INT i);              // Get i'th move in current position (1 <= i <=
                                                 // GetBoardMoveCount).
   //--- Editing Position ---
   void   Edit_Begin       (void);               // Begin editing position.
   void   Edit_End         (BOOL confirmed);     // End editing position (cancel if not confirmed).
   void   Edit_NewBoard    (void);               // New board.
   void   Edit_ClearBoard  (void);               // Clear the board.
   void   Edit_ClearPiece  (SQUARE sq);          // Clear contents of specified square.
   void   Edit_SetPiece    (SQUARE sq, PIECE p); // Set the piece "p" on the square "sq".
   void   Edit_MovePiece   (SQUARE from, SQUARE to); // Moves contents of square "from" to "to".
   void   Edit_SetPlayer   (COLOUR player);      // Get player (side to move).
   void   Edit_SetInitState(INT castlingRights, INT epSquare, INT moveNo);
   INT    Edit_CheckLegalPosition (void);

   //--- Annotations ---
   void   SetAnnotationGlyph (INT moveNo, INT glyph);
   INT    GetAnnotationGlyph (INT moveNo);
   void   SetAnnotation      (INT moveNo, CHAR *text, INT size, BOOL killNewLines = false);
   void   ClrAnnotation      (void);
   void   ClrAnnotation      (INT moveNo);
   void   GetAnnotation      (INT moveNo, CHAR *text, INT *size);
   INT    GetAnnotationLine  (INT moveNo, INT lineNo, CHAR *Text, BOOL *nl = nil);
   INT    GetAnnotationLineCount (INT moveNo);
   BOOL   ExistAnnotation    (INT moveNo);

   //--- Game Record Line Map ---
   INT    CalcGameMap (INT toMove, GAMEMAP GMap[], BOOL isPrinting, BOOL isCollectionGame, BOOL isPublishing);
   void   InsertAnnGameMap (INT j, INT *N, GAMEMAP GMap[]);
   INT    InsertGameMapHeader (GAMEMAP GMap[], BOOL isPrinting, BOOL isCollectionGame, BOOL isPublishing);
   BOOL   GameMapContainsDiagram (GAMEMAP GMap[], INT N);

   //--- File Routines ---

   LONG   Compress         (PTR Data);
   void   Decompress       (PTR Data, LONG size, BOOL raw = false);
   INT    DecompressInfo   (PTR Data);

   LONG   Write_V34        (PTR GameData);
   void   Read_V34         (PTR GameData, BOOL calcMoveFlags = true);

   LONG   Write_V2         (PTR GameData);
   void   Read_V2          (PTR GameData);

   LONG   Write_PGN        (CHAR *pgnBuf, BOOL includeAnn = true);
   INT    Read_PGN         (CHAR *pgnBuf, LONG *size);
   INT    Write_EPD        (CHAR *epdStr);
   INT    Read_EPD         (CHAR *epdStr);

   void   PGN_ReadBegin    (CHAR *pgnBuf, LONG bufSize);
   BOOL   PGN_ReadGame     (void);

   /*---------------------------------- Public Variables -----------------------------------------*/

   LONG     gameId;                    // Unique game id (> 0). Changed when new games are started.

   //--- Initial game state ---

   INITGAME Init;                      // Initial game state (board, player, castling rights etc.)

   //--- Move record ---

   INT      currMove;                  // Current number of half moves played.
   INT      lastMove;                  // Total number of half moves played.
   MOVE     Record[gameRecSize];       // The actual game record containing the moves. Record[0]
                                       // contains a dummy null move. The most recently played
                                       // move is thus stored in Record[currMove].

   DRAWDATA DrawData[gameRecSize +     // The draw information table. Auxiliary table used to check
                     maxSearchDepth];  // draw by repetition and draw by the 50 move rule. Is indexed
                                       // along with Record[]. The "hashKey" field of "DrawTab[i]" is
                                       // a 32 bit hash key of the position resulting from the move
                                       // "Record[i]". The "repCount" field indicates the number of
                                       // times the i'th position has occurred. The "irr" field is
                                       // the index in Record[]/DrawTab[] of the last irreversible move
                                       // (incl. the last move) and is used to check draw by the 50
                                       // move rule (as well as to facilitate repetition check).

   //--- Current board position (incrementally updated) ---

   PIECE    Board_[boardSize1],        // Current board configuration (position after current move).
            Board[boardSize2];
   
   COLOUR   player;                    // Side to move in current game position.
   COLOUR   opponent;                  // Side not to move in current game position.

   MOVE     Moves[maxLegalMoves];      // Strictly legal moves in current position. Should always be
                                       // kept up to date with current position.
   INT      moveCount;                 // Number of strictly legal moves in current position.

   SQUARE   kingSq;                    // Location of players king (computed during move generation).
   BOOL     kingInCheck;               // Is players king in check? (computed during move generation).

   INT      HasMovedTo[boardSize];     // Used for castling rights checking.
   ULONG    pieceCount;                // Piece count used for draw checking.

   //--- Misc game state information ---

   INT      result;                    // Unknown, mate, draw by repetition/50 move rule/insuf. mtrl
                                       // for the FINAL position (i.e. after Record[lastMove]).
   BOOL     hasResigned;               // Has program already resigned earlier?
   BOOL     hasOfferedDraw;            // Has program already offered draw? (###store move no??)

   BOOL     dirty;                     // Is game dirty? I.e. has moves been added but not saved.

   //--- Position Editor ---

   BOOL     editingPosition;           // Is the position editor currently open?
   PIECE    editPiece;                 // Currently selected piece.
   INITGAME InitBackup;                // Backup of initial game info when editing position.
   COLOUR   playerBackup;
   PIECE    BoardBackup[boardSize];

   //--- Misc game info (standard PGN tags) ---

   GAMEINFO Info;

   //--- Annotations ---

   CAnnotation *annotation;                  // Pointer to annotation object.

private:

   /*---------------------------------- Private Methods ------------------------------------------*/

   //--- Legal Move Generation ---

   void   CalcPawnMoves      (SQUARE sq);
   void   CalcQRBMoves       (SQUARE sq, const SQUARE Dir[], INT dirCount);
   void   CalcKnightMoves    (SQUARE sq);
   void   CalcKingMoves      (SQUARE sq);
   void   CalcCastling       (INT type);
   BOOL   LegalMove          (MOVE *m);

   void   CalcCheckFlags     (void);

   INT    CalcGameResult     (void);
   BOOL   VerifyRepetition   (INT n);

   //--- Loading/Saving Games in the new Super Compressed Format ---

   INT    CompressInfo (PTR Data);
   INT    CompressMoves (PTR Data);
   LONG   CompressAux (PTR Data);

   INT    DecompressMoves (PTR Data, BOOL raw = false);
   LONG   DecompressAux (PTR Data);

   //--- Loading/Saving Games ---
   // This makes use of the PACKET struct for the standard Sigma Chess game format.

   void   FirstPacket        (PTR GameData);
   void   NextPacket         (void);
   void   StoreInt           (INT n);
   void   StoreLong          (LONG n);
   void   StoreStr           (CHAR *s);
   INT    FetchInt           (void);
   LONG   FetchLong          (void);
   void   FetchStr           (CHAR *s);

   /*---------------------------------- Private Variables ----------------------------------------*/

   PACKET *Packet;
   LONG   packInx;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES                                      */
/*                                                                                                */
/**************************************************************************************************/

extern GLOBAL Global;    // Common global engine data structure

extern BYTE gameData[gameDataSize];
extern CHAR PieceCharEng[8];          // English piece letters.
extern SQUARE KingDir[9], QueenDir[9], RookDir[5], BishopDir[5], KnightDir[9];


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

void   ClearGameInfo (GAMEINFO *Info);
void   ResetGameInfo (GAMEINFO *Info);
INT    DecompressGameInfo (PTR Data, GAMEINFO *Info);

INT    CheckLegalPosition (PIECE Board[], COLOUR player);

void   InitGameModule (void);
void   InitGameFile5 (void);
