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

#pragma once

#include "AnalysisState.h"
#include "BoardArea2D.h"
#include "BoardArea3D.h"
#include "ChessClock.h"
#include "Game.h"
#include "GameToolbar.h"
#include "GameUtil.h"
#include "InfoArea.h"
#include "Level.h"
#include "SigmaApplication.h"
#include "SigmaWindow.h"
#include "TabArea.h"

#include "Engine.h"

#include "CFile.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

enum GAMESAVE_FORMAT  // Matches the "Game Format" popup menu items in the
                      // "Save..." dialog.
{ gameFormat_Compressed = 1,  // The new compressed version 5 format
  gameFormat_PGN = 2,         // The PGN format
  gameFormat_Extended = 4,    // The � extended version 3 & 4 format
  gameFormat_Old = 5          // The old obsolete version 2 format
};

//#define __debug_GameWin  1

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

class CollectionWindow;

class GameWindow : public SigmaWindow {
 public:
  GameWindow(CHAR *title, CRect frame, BOOL ExaChess = false);
  virtual ~GameWindow(void);

  //--- Events ---
  virtual void HandleKeyDown(CHAR c, INT key, INT modifiers);
  virtual void HandleMessage(LONG msg, LONG submsg = 0, PTR data = nil);
  virtual void HandleMenuAdjust(void);
  virtual void HandleZoom(void);
  virtual void HandleScrollBar(CScrollBar *ctrl, BOOL tracking);
  virtual void HandleEditor(CEditor *ctrl, BOOL textChanged, BOOL selChanged,
                            BOOL srcRplChanged);
  virtual void HandleNullEvent(void);

  BOOL HandleCloseRequest(void);
  BOOL HandleQuitRequest(void);

  //--- Adjust Menus etc ---
  void AdjustFileMenu(void);
  void AdjustEditMenu(void);
  void AdjustTextEditMenu(void);
  void AdjustGameMenu(void);
  void AdjustAnalyzeMenu(void);
  void AdjustLevelMenu(void);
  void AdjustEngineMenu(void);
  void AdjustDisplayMenu(void);
  void AdjustCollectionMenu(void);
  void AdjustLibraryMenu(void);
  void AdjustToolbar(void);
  void GameMoveAdjust(BOOL redrawBoard, BOOL engineMovePlayed = false);
  void AutoSetECO(void);
  void TurnBoard(void);
  void CheckTurnPlayer(void);

  //--- Saving/Printing ---
  BOOL Save(void);
  BOOL SaveAs(void);
  BOOL CheckSave(CHAR *prompt);
  BOOL IsLocked(void);
  void ExportHTML(void);
  void PrintGame(void);

  //--- Copy/Paste ---
  void CopyGame(BOOL includeAnn = true);
  void PasteGame(void);
  void CopyPosition(void);
  void PastePosition(void);

  //--- Analyze ---
  void Analyze_Go(BOOL nextBest = false);
  void CheckSwapPlayerNames(void);
  void SetSearchParam(BOOL nextBest = false);
  void StartSearch(BOOL nextBest = false);
  void Analyze_Reply(void);

  void SearchCompleted(void);
  void AdjustTargetELO(void);
  void NormalSearchCompleted(void);
  void CheckAnnounceMate(void);
  void PlayEngineMove(void);
  BOOL AcceptDrawOffer(void);
  BOOL CheckResign(void);
  void CheckBackgrounding(void);
  void Analyze_Stop(void);
  void Analyze_Hint(void);

  void Analyze_AutoPlay(void);
  BOOL CanAutoPlay(void);
  void AutoSearchCompleted(void);
  void EndAutoPlay(void);
  void Analyze_DemoPlay(void);
  BOOL CanDemoPlay(void);
  void Analyze_EngineMatch(void);
  BOOL AbandonEngineMatch(BOOL confirm = false);
  void ShowEngineMatchResult(void);

  void AnalyzeGame(void);
  void AnalyzeCollection(void);
  void AnalyzeGame_StartSearch(void);
  void AnalyzeGame_SearchCompleted(void);
  void AnalyzeGame_End(void);

  void ProcessEngineMessage(void);
  void CheckAbortEngine(void);
  void CheckMonitorMode(void);
  BOOL CheckEngineMatch(void);

  void AnalyzeEPD(void);

  //--- Chess clocks ---
  void ResetClocks(void);
  void ResetClock(COLOUR player);
  void StartClock(void);
  void StopClock(void);
  BOOL TickClock(void);
  void CheckClockAllocation(void);
  void TimeForfeit(void);

  void PlayerMovePerformed(BOOL drawMove = false);
  void SetGameResult(INT result = -1, INT infoResult = -1);
  void PlayerResigns(void);

  //--- Collections ---
  void Attach(CollectionWindow *colWin, ULONG gameNo);
  void Detach(void);
  void AddToCollection(INT colWinNo);

  //--- Sigma/UCI Engine ---
  void SelectEngine(UCI_ENGINE_ID engineId);
  BOOL UsingUCIEngine(void);
  BOOL EngineSupportsRating(CHAR *title);

  //--- MultiPV ---
  BOOL SupportsMultiPV(void);
  INT GetMaxMultiPVcount(void);  // Always 1 if engine doesn't support multi PV
  INT GetMultiPVcount(void);     // Always 1 if engine doesn't support multi PV
  void SetMultiPVcount(INT count);
  void IncMultiPVcount(void);
  void DecMultiPVcount(void);
  BOOL MultiPVAllowed(void);

  //--- Misc ---
  void CalcFrames(INT theSquareWidth);
  void SetBoardSize(INT newSquareWidth);

  void RateGame(void);
  BOOL AbandonRatedGame(void);
  void DrawBoard(BOOL drawFrame);
  BOOL LegalPosition(void);
  void RefreshGameInfo(void);
  void SetAnnotation(CHAR *s);  // Sets annotation for current position (i.e.
                                // after current move)
  void FlushAnnotation(void);

  void ReplayGame(void);
  void PlayMainLine(void);

  void GotoMove(INT j, BOOL openAnnEditor = false);
  void Toggle3D(void);

  void CopyAnalysis(void);

#ifdef __libTest_Verify
  void VerifyPosLib(void);
#endif

  //--- Main State ---
  CGame *game;
  CChessClock *Clock[white_black];
  LEVEL level, level0;
  UCI_ENGINE_ID uciEngineId;
  CHAR engineName[uci_NameLen + 1];
  ENGINE *engine;
  BOOL permanentBrain;
  ENGINE_RATING engineRating;  // Current engine rating for this window
  BOOL isRated;
  GAMEINFOFILTER infoFilter;

  //--- Analysis State ---
  BOOL thinking;       // Is � Chess thinking (in the foreground)
  BOOL backgrounding;  // Is � Chess thinking in the background (permanent brain
                       // or monitor)
  BOOL monitoring;     // Is � Chess monitoring (in the background)
  BOOL autoPlaying;    // Is � Chess auto/demo playing (including analyze
                       // game/col/EPD)?
  BOOL demoPlaying;    // Is � Chess demo playing?
  BOOL analyzeGame;    // Analyzing game (or collection)?
  BOOL analyzeCol;     // Analyzing collection?
  BOOL analyzeEPD;     // Analyzing EPD?
  BOOL userStopped;    // Did user stop search?
  INT analyzeGameMove0;  // First move being analyzed in the game

  MOVE hintMove;  // Reply from last search (used for hints and backgrounding)
  MOVE expectedMove;  // Expected move on which background thinking is based.
  BOOL hasResigned;   // Has � Chess previously resigned in this game?
  BOOL hasAnnouncedMate;  // Has � Chess previously announced mate in this game?
  BOOL preDrawOffered;    // Has user offered draw before playing his move?
  BOOL drawOffered;       // Has user offered draw while Sigma is thinking?
  BOOL timeoutContinued;  // Has user continued timeout? If so we automatically
                          // reset the clocks on subsequent time outs

  PIECE BoardAnalyzed[boardSize];  // Last board that was analyzed (used with
                                   // next best).
  INT moveAnalyzed;  // Game move number of last analysis (used with next best).

  ANALYSIS_STATE Analysis, PrevAnalysis;

  //--- Graphics State ---
  BOOL mode3D;

  BOOL boardTurned;
  INT squareWidth, frameWidth;

  INFO_MODE infoMode, oldInfoMode;

  BOOL annEditor;  // Is annotation editor currently open?
  BOOL posEditor;  // Is position editor currently open?
  BOOL libEditor;  // Is library editor currently open?
  BOOL promoting;

  BOOL showInfoArea;
  BOOL varDisplayVer;   // Verical PV display?.
  INT multiPVoptionId;  // Index in UCI_INFO->Options[] of multiPV option. Set
                        // to -1 if multiPV not supported.
  BOOL toolbarTop;

  CRect mainRect, boardRect, infoRect, tabRect, toolbarRect,
      miniToolbarRect;  // Subview frames

  BoardAreaView *boardAreaView;  // Points to either 2D or 3D view
  BoardArea2DView *boardArea2DView;
  BoardArea3DView *boardArea3DView;

  InfoAreaView *infoAreaView;
  TabAreaView *tabAreaView;
  GameToolbar *toolbar;
  MiniGameToolbar *miniToolbar;

  CRect frame2D;

  //--- File/Collection State ---
  CFile *file;
  CollectionWindow *colWin;
  ULONG colGameNo;

  //--- Misc ---
  BOOL ExaChess;

#ifdef __debug_GameWin
  CView *debugView;
  BOOL tracing, waiting;
  void Debug_NewNode(NODE *N);
  void Debug_EndNode(NODE *N);
  void Debug_NewMove(NODE *N);
  void Debug_Cutoff(NODE *N);
  void Debug_DrawTree(INT d, BOOL drawLeafMove, BOOL cutoff);
  void Debug_HandleKey(CHAR c, INT key);
#endif
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

GameWindow *NewGameWindow(CHAR *title, BOOL setPlayerName = false,
                          BOOL ExaChess = false);
void OpenGameWindow(void);
void OpenGameFile(CFile *file);

INT GameWin_Width(INT sqWidth);
INT GameWin_Height(INT sqWidth);

INT BuildAnalysisString(ANALYSIS_STATE *Analysis, CHAR *Text,
                        BOOL includeAltScore = false, INT altScore = 0,
                        INT pvCount = 1);
void BuildExaChessResult(ANALYSIS_STATE *Analysis, CHAR *Text);
