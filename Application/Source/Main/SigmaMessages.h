#pragma once

#include "UCI.h"

enum MESSSAGES {
  // FILE menu commands:
  file_NewGame = 10000,
  file_NewCollection,
  file_NewLibrary,
  file_Open,
  file_OpenGame,
  // file_OpenRecent,
  file_Save,
  file_SaveAs,
  file_Close,
  file_ExportHTML,
  file_PageSetup,
  file_Print,
  // file_PrintDiagram,
  file_Preferences,
  file_Quit,

  // EDIT menu commands:
  edit_Undo,
  edit_Redo,
  edit_Cut,
  edit_Copy,
  edit_Paste,
  edit_Clear,
  edit_ClearAll,
  edit_SetAnnGlyph,

  edit_Find,
  edit_FindAgain,
  edit_Replace,
  edit_ReplaceFind,
  edit_ReplaceAll,

  edit_Diagram,
  edit_SelectAll,

  cut_Standard,
  cut_Game,
  copy_Standard,
  copy_Game,
  copy_GameNoAnn,
  copy_Position,
  copy_Analysis,
  paste_Standard,
  paste_Game,
  paste_Position,

  // GAME menu commands:
  game_ResetGame,
  game_BranchGame,
  game_RateGame,
  game_ClearRest,
  game_ClearAnn,
  game_AddToCollection,
  game_AddToColFirst,
  game_AddToColLast = game_AddToColFirst + maxSigmaWindowCount - 1,
  game_Detach,
  game_UndoMove,
  game_RedoMove,
  game_UndoAllMoves,
  game_RedoAllMoves,
  game_GotoMove,
  game_ReplayGame,
  game_PositionEditor,
  posEditor_Open,
  posEditor_Close,
  posEditor_Done,
  posEditor_Cancel,
  posEditor_SelectPiece,
  posEditor_SelectPlayer,
  posEditor_ClearBoard,
  posEditor_NewBoard,
  posEditor_Status,
  game_AnnotationEditor,
  game_GameInfo,

  // ANALYZE menu commands:
  analyze_Engine,
  engine_Sigma,
  engine_CustomFirst,
  engine_CustomLast = engine_CustomFirst + uci_MaxEngineCount - 2,
  engine_Configure,
  analyze_Go,
  analyze_NextBest,
  analyze_Stop,
  analyze_Pause,
  analyze_Hint,
  analyze_PlayMainLine,
  analyze_DrawOffer,
  analyze_Resign,
  analyze_AutoPlay,
  analyze_DemoPlay,
  analyze_AnalyzeGame,
  analyze_AnalyzeCol,
  analyze_AnalyzeEPD,
  analyze_EngineMatch,
  analyze_TransTables,
  analyze_EndgameDB,
  analyze_Completed,

  // LEVEL menu commands:
  level_Select,
  level_PlayingStyle,
  playingStyle_Chicken,
  playingStyle_Defensive,
  playingStyle_Normal,
  playingStyle_Aggressive,
  playingStyle_Desperado,
  level_SetPlayingMode,
  level_SetPlayingStyle,
  level_PermanentBrain,
  level_NonDeterm,
  level_SigmaELO,
  level_PlayerELO,
  level_ELOCalc,

  // DISPLAY menu commands:
  display_TurnBoard,
  display_ShowAnalysis,
  display_MoveMarker,
  moveMarker_Off,
  moveMarker_LastCompMove,
  moveMarker_LastMove,
  display_Notation,
  notation_Short,
  notation_Long,
  notation_Descr,
  notation_Figurine,
  display_ShowFutureMoves,
  display_HiliteCurrMove,
  display_PieceLetters,
  pieceLetters_Czech,
  pieceLetters_Danish,
  pieceLetters_Dutch,
  pieceLetters_English,
  pieceLetters_Finnish,
  pieceLetters_French,
  pieceLetters_German,
  pieceLetters_Hungarian,
  pieceLetters_Icelandic,
  pieceLetters_Italian,
  pieceLetters_Norwegian,
  pieceLetters_Polish,
  pieceLetters_Portuguese,
  pieceLetters_Romanian,
  pieceLetters_Spanish,
  pieceLetters_Swedish,
  pieceLetters_US,
  display_ToggleInfoArea,
  display_GameRecord,
  display_PieceSet,
  pieceSet_American,
  pieceSet_European,
  pieceSet_Classical,
  pieceSet_Metal,
  pieceSet_Wood,
  pieceSet_Childrens,
  pieceSet_CustomFirst,
  pieceSet_CustomLast = pieceSet_CustomFirst + 32,
  display_BoardType,
  boardType_Picker,
  boardType_Standard,
  boardType_Olive,
  boardType_Peanut,
  boardType_Butter,
  boardType_Ice,
  boardType_Gray,
  boardType_LightGray,
  boardType_Diagram,
  boardType_Wood,
  boardType_Marble,
  boardType_CustomFirst,
  boardType_CustomLast = boardType_CustomFirst + 32,
  display_BoardSize,
  boardSize_Standard,
  boardSize_Medium,
  boardSize_Large,
  boardSize_EvenLarger,
  display_3DBoard,
  display_Show3DClock,
  display_ColorScheme,
  colorScheme_Picker,
  colorScheme_Standard,
  colorScheme_Graphite,
  colorScheme_Coal,
  colorScheme_Forest,
  colorScheme_Leprechaun,
  colorScheme_Oak,
  colorScheme_Olive,
  colorScheme_Ice,
  colorScheme_Salmon,
  colorScheme_Rose,
  display_ToolbarTop,

  // COLLECTION menu commands:
  collection_EditFilter,
  collection_EnableFilter,
  collection_OpenGame,
  collection_PrevGame,  // Also enable when front win is collection game win
  collection_NextGame,  // Also enable when front win is collection game win
  collection_Layout,
  collection_ImportPGN,
  collection_ExportPGN,
  collection_Compact,
  collection_Renumber,
  collection_Info,

  // LIBRARY menu commands:
  library_Name,
  library_SigmaAccess,
  librarySet_Disabled,
  librarySet_Solid,
  librarySet_Tournament,
  librarySet_Wide,
  librarySet_Full,
  library_Editor,
  library_ClassifyPos,
  library_AutoClassify,
  library_ECOComment,
  library_DeleteVar,
  library_ImportCollection,
  library_Save,
  library_SaveAs,

  // WINDOW menu commands:
  window_CloseAll,
  window_Minimize,
  window_Zoom,
  window_Cycle,
  window_WinFirst,
  window_WinLast = window_WinFirst + maxSigmaWindowCount - 1,

  // DEBUG menu commands:
  debug_Window,
  debug_A,
  debug_B,
  debug_C,

  // MISCELLANEOUS Application Commands:

  // MISCELLANEOUS Window Specific Commands:

  msg_RefreshColorScheme,
  msg_RefreshPieceSet,
  msg_RefreshBoardType,
  msg_RefreshMoveNotation,
  msg_RefreshInfoSep,
  msg_RefreshGameMoveList,
  msg_RefreshPosLib,
  msg_UCIEngineRemoved,  // submsg contains UCI engine id
  msg_UCISetSigmaEngine,

  // GAME WINDOW MESSAGES:

  display_VerPV,
  display_HorPV,
  display_IncMultiPV,
  display_DecMultiPV,

  // COLLECTION WINDOW MESSAGES:

  msg_ColStop,
  msg_ColSelChanged,

  // TOTAL NUMBER OF MESSAGES:

  msg_Count
};

#define pieceSet_First pieceSet_American
#define pieceSet_Last pieceSet_Childrens
#define pieceSet_Count (pieceSet_Last - pieceSet_First + 1)

#define boardType_First boardType_Picker
#define boardType_Last boardType_Marble
#define boardType_Count (boardType_Last - boardType_First + 1)

#define pieceLetters_First pieceLetters_Czech
#define pieceLetters_Last pieceLetters_US

#define colorScheme_First colorScheme_Picker
#define colorScheme_Last colorScheme_Rose
