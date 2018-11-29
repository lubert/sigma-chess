#pragma once

#include "CFile.h"
#include "Collection.h"
#include "Engine.h"
#include "Game.h"
#include "General.h"
#include "Level.h"
#include "PosLibrary.h"
#include "Rating.h"
#include "SigmaAppConstants.h"
#include "SigmaLicense.h"
#include "UCI.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define numSchemeColor 11
#define maxCustomLevels 10

#define prevPrefsSize (sizeof(PREFS) - sizeof(ENGINE_MATCH_PARAM))

/**************************************************************************************************/
/*                                                                                                */
/*                                        TYPE/CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef struct {
  BOOL players;
  BOOL event;
  BOOL site;
  BOOL date;
  BOOL round;
  BOOL result;
  BOOL ECO;
} GAMEINFOFILTER;

typedef struct {
  // Parameters
  UCI_ENGINE_ID
      engine1;  // White in first game (all games if alternate = false)
  UCI_ENGINE_ID
      engine2;      // Black in first game (all games if alternate = false)
  INT matchLen;     // Number of games in match
  BOOL alternate;   // Swap colors between games?
  LEVEL level;      // Time controls
  BOOL adjWin;      // Adjudicate wins?
  INT adjWinLimit;  // Win adjucation limit (in pawns 3..9)
  BOOL adjDraw;     // Adjudicate draws?
} ENGINE_MATCH_PARAM;

typedef struct {
  INT mainVersion;  // 5, 6, ...
  INT subVersion;   // .1, .2, ...
  INT release;      // Developement, alpha, beta, final
  INT buildNumber;  // .x.1, .x.2

  BOOL firstLaunch;  // Is this the first time � Chess is launched

  LICENSE license;

  //----- GLOBAL PREFS (MENU SETTINGS) -----

  struct  // Graphics layout/appearance
  {
    INT pieceSet;     // 0...5
    INT boardType;    // 0...7
    INT boardType3D;  // 0..1
    INT squareWidth;  // Default square width for new windows (42 or 58)
    INT colorScheme;  // 0...6
    RGB_COLOR pickScheme;
    RGB_COLOR whiteSquare, blackSquare, frame;
    INT unused[8];
  } Appearance;

  struct  // Move notation
  {
    MOVE_NOTATION moveNotation;
    BOOL figurine;
    INT pieceLetters;  // Piece letters 0...16
    INT unused[8];
  } Notation;

  //----- GLOBAL PREFS (PREFS DIALOG) -----

  struct  // "General" prefs
  {
    CHAR playerName[nameStrLen + 1];
    INT menuIcons;
    BOOL enable3D;
    INT unused[8];
  } General;

  struct  // "Games" prefs
  {
    BOOL gotoFinalPos;
    BOOL turnPlayer;
    BOOL showFutureMoves;
    BOOL hiliteCurrMove;
    BOOL askGameSave;
    INT moveSpeed;  // 1 to 100
    BOOL saveNative;
    INT unused[7];
  } Games;

  struct  // "Collections" prefs
  {
    BOOL autoName;
    BOOL keepColWidths;
    INT unused[8];
  } Collections;

  struct  // "PGN" prefs
  {
    BOOL skipMoveSep;
    BOOL openSingle;
    BOOL fileExtFilter;
    BOOL keepNewLines;
    INT unused[7];
  } PGN;

  struct  // "Sound Effects" prefs
  {
    BOOL woodSound;
    BOOL moveBeep;
    INT unused[8];
  } Sound;

  struct  // "Announcements/Messages" prefs
  {
    BOOL announceMate;
    BOOL announce1stMate;
    BOOL gameOverDlg;
    BOOL canResign;
    BOOL canOfferDraw;
    INT unused[8];
  } Messages;

  struct  // "Misc" prefs
  {
    BOOL printPageHeaders;
    BOOL HTMLGifReminder;
    INT unused[8];
  } Misc;

  struct  // "Analysis Text Formatting" prefs
  {
    BOOL showScore;
    BOOL showDepth;
    BOOL showTime, showNodes, showNSec;
    BOOL showMainLine;
    BOOL shortFormat;
    SCORE_NOTATION scoreNot;  // Numerical/glyph & relative to player/White
    INT unused[8];
  } AnalysisFormat;

  struct  // "Memory" prefs (Classic Only)
  {
    INT reserveMem;  // Number of Mb reserved for general usage
    INT unused[8];
  } Memory;

  struct  // Transposition tables:
  {
    BOOL useTransTables;    // Use transposition tables?
    BOOL useTransTablesMF;  // Use transposition tables in Mate Finder?
    INT totalTransMem;      // Total trans mem block size in MB (OS X only)
    INT maxTransSize;       // Max trans table size per engine instance
    INT unused[8];
  } Trans;  // (1 = 80K, 2 = 160K, 3 = 320K, ...

  //----- GLOBAL ENGINE PREFS -----

  BOOL useEndgameDB;  // Enable endgame databases?

  struct  // Filter computer annotations in "Analyze Game" and "Analyze
          // Collection":
  {
    INT timePerMove;
    BOOL skipWhitePos;
    BOOL skipBlackPos;
    BOOL skipMatching;
    BOOL skipLowScore;
    INT scoreLimit;
    INT unused[8];
  } AutoAnalysis;

  //----- GAME WINDOW PREFS -----
  // These prefs are specific to each game window. The most recently used values
  // are used when creating new game windows:

  struct  // Level settings:
  {
    LEVEL level;                         // Current/default level (last used).
    LEVEL CustomLevel[maxCustomLevels];  // Custom/user defined levels.
    BOOL permanentBrain_OBSOLETE;  // OBSOLETE (stored in Prefs.UCI for each
                                   // engine instead)
    BOOL nonDeterm;
    ENGINE_RATING strength_OBSOLETE;  // OBSOLETE (stored in Prefs.UCI for each
                                      // engine instead)
    INT playingStyle;
    INT unused[8];
  } Level;

  struct  // Game display settings:
  {
    BOOL boardTurned;
    INT moveMarker;
    BOOL showAnalysis;
    BOOL showSearchTree;
    BOOL gameHeaderClosed;
    BOOL statsHeaderClosed;
    INT dividerPos;
    BOOL mode3D;  // Are new game windows per default opened in 3D mode?
    BOOL show3DClocks;
    GAMEINFOFILTER gameInfoFilter;
    BOOL toolbarTop;
    BOOL hideInfoArea;
    BOOL varDisplayVer;
    INT unused[6];
  } GameDisplay;

  struct  // Collection display settings:
  {
    BOOL toolbarTop;
    INT cellWidth[8];
    INT unused[8];
  } ColDisplay;

  GAMEINFO gameInfo;  // Default game info for new games.

  //----- POSITION LIBRARY PREFS -----

  struct  // Library settings:
  {
    BOOL enabled;
    LIB_SET set;
    CHAR name[maxFileNameLen + 1];
    LIB_AUTOCLASS autoClassify;
    LIBIMPORT_PARAM param;
    INT unused[8];
  } Library;

  //----- FUTURE USE -----

  INT unused[8];

  //----- PLAYER RATING STATS -----

  INT playerELOCount;  // Currently always 1!
  PLAYER_RATING PlayerELO;

  //----- UCI CONFIG (New in Sigma 6.1) -----

  UCI_PREFS UCI;

  //----- Engine Match Param (New in Sigma 6.1.3) ---

  ENGINE_MATCH_PARAM EngineMatch;

} PREFS;

class SigmaPrefs {
 public:
  SigmaPrefs(void);
  virtual ~SigmaPrefs(void);

  void Reset(void);
  void Load(void);
  void Save(void);
  void TryUpgradePrevious(void);

  void Apply(void);

  void SetColorScheme(INT newColorScheme, BOOL startup = false);

  // void SetPermanentBrain (BOOL permBrain, BOOL startup = false);
  void SetNonDeterm(BOOL nonDeterm, BOOL startup = false);
  void SetPlayingStyle(INT style, BOOL startup = false);

  void SetPieceSet(INT newPieceSet, BOOL startup = false);
  void SetBoardType(INT newBoardType, BOOL startup = false);
  void SetNotation(MOVE_NOTATION newNotation, BOOL startup = false);
  void SetFigurine(BOOL newFigurine, BOOL startup = false);
  void SetPieceLetters(INT newPieceLetters, BOOL startup = false);

  void SetLibraryName(CHAR *name, BOOL startup = false);
  void EnableLibrary(BOOL enabled, BOOL startup = false);
  void SetLibraryAccess(LIB_SET set, BOOL startup = false);

  void SetMoveMarker(INT moveMarker, BOOL startup = false);

  CFile prefsFile;

  // Scheme colours:
  RGB_COLOR mainColor;
  RGB_COLOR lightColor;
  RGB_COLOR darkColor;

  // PREFS prefs;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES */
/*                                                                                                */
/**************************************************************************************************/

extern SigmaPrefs *sigmaPrefs;
extern PREFS Prefs;
extern RGB_COLOR boardFrameColor[4];

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/
