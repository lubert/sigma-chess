/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAMEWINDOW.C */
/* Purpose : This module implements the game windows. */
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

#include "GameWindow.h"
#include "CDialog.h"
#include "CollectionWindow.h"
#include "Engine.f"
#include "ExaChessGlue.h"
#include "GameInfoDialog.h"
#include "GotoMoveDialog.h"
#include "InfoFilterDialog.h"
#include "LevelDialog.h"
#include "PGN.h"
#include "PosLibrary.h"
#include "PrefsDialog.h"
#include "Rating.h"
#include "SigmaApplication.h"
#include "TransTabManager.h"
#include "UCI_Option.h"

#include "Board.f"

#include "TaskScheduler.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                   CREATE/OPEN GAME WINDOW */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- New Game Window
 * -----------------------------------------*/

GameWindow *NewGameWindow(CHAR *title, BOOL setPlayerName, BOOL ExaChess) {
  if (!sigmaApp->CheckWinCount() || !sigmaApp->CheckMemFree(250)) return nil;

  INT squareWidth = Prefs.Appearance.squareWidth;
  INT width = (Prefs.GameDisplay.hideInfoArea ? BoardArea_Width(squareWidth)
                                              : GameWin_Width(squareWidth));
  INT height = GameWin_Height(squareWidth);

  CRect frame = theApp->NewDocRect(width, height);
  GameWindow *gameWin = new GameWindow(title, frame, ExaChess);

  GAMEINFO *info = &gameWin->game->Info;
  if (setPlayerName && !info->whiteName[0] && !info->blackName[0]) {
    CopyStr(Prefs.General.playerName, info->whiteName);
    CopyStr(gameWin->engineName, info->blackName);
  }

  return gameWin;
} /* NewGameWindow */

/*-------------------------------------- Open Game Window
 * ----------------------------------------*/
// This routine is invoked when a game is opened from a file.

void OpenGameFile(CFile *file) {
  GameWindow *win = NewGameWindow(file->name, false, false);
  if (!win) return;

  ULONG bytes;
  PTR data;
  file->Load(&bytes, &data);
  win->file = file;

  switch (file->fileType) {
    case '·GM5':
      win->game->Decompress(data, bytes);
      break;
    case '·GMX':
      win->game->Read_V34(data);
      break;
    case '·GAM':
    case 'XLGM':
    case 'CHGM':
      win->game->Read_V2(data);
      break;
    default:
      NoteDialog(nil, "Open Game", "Unknown file format...", cdialogIcon_Error);
  }

  Mem_FreePtr(data);
  if (Prefs.Games.gotoFinalPos && win->game->CanRedoMove())
    win->HandleMessage(game_RedoAllMoves);
  else
    win->GameMoveAdjust(false);

  win->CheckTurnPlayer();
} /* OpenGameFile */

/*----------------------------------------- Dimensions
 * -------------------------------------------*/

INT GameWin_Width(INT sqWidth) {
  return BoardArea_Width(sqWidth) + InfoArea_Width() + TabArea_Width();
} /* GameWin_Width */

INT GameWin_Height(INT sqWidth) {
  INT height = BoardArea_Height(sqWidth) + toolbar_Height + toolbar_HeightSmall;
#ifdef __debug_GameWin
  height += 250;
#endif
  return height;
} /* GameWin_Height */

/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

// The info area works in five different modes, each represented by a tab:
//
// A) "Moves Only": Only the move list shown and none of editors or analysis
// B) "Analysis"  : The move list shown at the top and the analysis window at
// the bottom C) "Annotate"  : The move list shown at the top and the annotation
// editor at the bottom D) "Edit Lib"  : The move list shown at the top and the
// library editor at the bottom E) "Edit Pos"  : Only the position editor shown
//
// In state A-D moves can be entered.
// In state A-C the engine can think. In fact the user can freely switch between
// these three modes without any restrictions (and without affecting the search
// state).
//
// So we have the following transition state diagram:
//
// ABC -> ABC : Just show hide subviews
// ABC -> D   : Stop the engine and disable "Go" button (and remember previous
// ABC tab) ABC -> E   : Stop engine and disable most things (and remember
// previous ABC tab) D -> ABC   : Just show hide subviews and re-enable "Go"
// button E -> ABCD  : Corresponds to cancel Pos Editor if clicked on tabs. Open
// "Cancel pos editor?"
//              confirm dialog. "Game/Pos Edit" command corresponds to "OK" and
//              returns to previously selected tab.
// D -> E     : "Close" lib editor and open pos editor.
//
// When moving to a new mode we always remember the previos mode, so we can
// return to this mode if the user exits the current mode using one of the menu
// commands.

GameWindow::GameWindow(CHAR *title, CRect frame, BOOL isExaChess)
    : SigmaWindow(title, frame, sigmaWin_Game, false)

// The GameWindow constructor sets up the menu bar and the main views in the
// window:
// * The board view to the left (including chess clocks and player information).
// * The info view to the right (including game record, search results and
// various editors).
// * The toolbar view at the bottom.
// Also the game object is created, initialized and "attached" to the window.

{
  // Create and attach the corresponding game object before initializing window
  // views (since these depend on the existence of the game object but not vice
  // versa).
  game = new CGame();
  level = Prefs.Level.level;
  file = nil;

  ExaChess = isExaChess;

  Clock[white] = new CChessClock();
  Clock[black] = new CChessClock();

  infoFilter = Prefs.GameDisplay.gameInfoFilter;

  colWin = nil;  // Initially not part of collection
  colGameNo = 0;

  // Create and attach new engine instance:
  uciEngineId = Prefs.UCI.defaultId;
  UCI_INFO *UCIInfo = &Prefs.UCI.Engine[uciEngineId];

  CopyStr(UCIInfo->name, engineName);

  engine = (ENGINE *)::Mem_AllocPtr(sizeof(ENGINE));
  if (engine && !Engine_Create(&Global, engine, (ULONG)this)) {
    Mem_FreePtr(engine);
    engine = nil;
  }

  if (engine) {
    engine->UCI = (uciEngineId != uci_SigmaEngineId);
    TransTab_AutoInit();
  }

  thinking = false;
  backgrounding = false;
  monitoring = false;
  autoPlaying = false;
  demoPlaying = false;
  analyzeGame = false;
  analyzeCol = false;
  analyzeEPD = false;

  hasResigned = false;
  hasAnnouncedMate = false;
  preDrawOffered = false;
  drawOffered = false;
  timeoutContinued = false;

  clrMove(hintMove);
  ClearTable(BoardAnalyzed);
  moveAnalyzed = -1;

  permanentBrain =
      (UCIInfo->supportsPonder ? UCIInfo->Ponder.u.Check.val : false);
  engineRating.reduceStrength =
      (UCIInfo->supportsLimitStrength ? UCIInfo->LimitStrength.u.Check.val
                                      : false);
  engineRating.engineELO =
      (UCIInfo->supportsLimitStrength ? UCIInfo->UCI_Elo.u.Spin.val : 2000);
  engineRating.autoReduce =
      (UCIInfo->supportsLimitStrength ? UCIInfo->autoReduce : false);
  isRated = false;

  // Initialize various window options/state:
  mode3D = false;

  boardTurned = Prefs.GameDisplay.boardTurned;
  infoMode = oldInfoMode = infoMode_Analysis;  // Default mode
  annEditor = false;
  posEditor = false;
  libEditor = false;
  promoting = false;

  showInfoArea = !Prefs.GameDisplay.hideInfoArea;

  varDisplayVer = Prefs.GameDisplay.varDisplayVer;
  multiPVoptionId = UCI_GetMultiPVOptionId(uciEngineId);

  toolbarTop = Prefs.GameDisplay.toolbarTop;

  // Create subviews:
  CalcFrames(Prefs.Appearance.squareWidth);
  boardAreaView = boardArea2DView = new BoardArea2DView(this, boardRect);
  infoAreaView = new InfoAreaView(this, infoRect);
  tabAreaView = new TabAreaView(this, tabRect);
  toolbar = new GameToolbar(this, toolbarRect);
  miniToolbar = new MiniGameToolbar(this, miniToolbarRect);
  boardArea3DView = nil;

#ifdef __debug_GameWin
  CRect debugRect = Bounds();
  debugRect.top = gameWin_Height + 1;
  debugView = new CView(this, debugRect);
  debugView->SetFontFace(font_Fixed);
  debugView->SetFontSize(9);
  tracing = waiting = false;
#endif

  // Finally show the window:
  ResetClocks();
  AdjustToolbar();

  Show(true);
  SetFront();

  if (!MultiPVAllowed()) SetMultiPVcount(1);
} /* GameWindow::GameWindow */

GameWindow::~GameWindow(void) {
  if (colWin) colWin->DetachGameWin(this);
  if (ExaChess) CleanExaWindow();

  delete boardAreaView;
  delete infoAreaView;
  delete toolbar;

  if (file) delete file;
  file = nil;

  delete Clock[white];
  delete Clock[black];
  delete game;

  if (engine) {
    Engine_Destroy(engine);
    TransTab_Deallocate(engine);
    ::Mem_FreePtr(engine);
    engine = nil;
    TransTab_AutoInit();
  }
} /* GameWindow::~GameWindow */

void GameWindow::CalcFrames(INT theSquareWidth) {
  // First fetch the total bounds of the window:
  squareWidth = theSquareWidth;
  frameWidth = BoardFrame_Width(squareWidth);

  mainRect.Set(0, 0, GameWin_Width(squareWidth), GameWin_Height(squareWidth));
  //   mainRect.bottom = GameWin_Height(squareWidth);

  // Board area view:
  boardRect = mainRect;
  boardRect.right = boardRect.left + BoardArea_Width(squareWidth);
  boardRect.bottom = boardRect.top + BoardArea_Height(squareWidth);

  // Info area view:
  infoRect = mainRect;
  infoRect.left = boardRect.right;
  infoRect.right = infoRect.left + InfoArea_Width();
  infoRect.bottom = boardRect.bottom;

  // Tab area view:
  tabRect = mainRect;
  tabRect.left = mainRect.right - TabArea_Width();
  tabRect.bottom = infoRect.bottom;

  // Toolbar view:
  toolbarRect = mainRect;
  toolbarRect.top = boardRect.bottom;
  toolbarRect.bottom = toolbarRect.top + toolbar_Height;

  // Mini toolbar view:
  miniToolbarRect = toolbarRect;
  miniToolbarRect.top = toolbarRect.bottom;
  miniToolbarRect.bottom = miniToolbarRect.top + toolbar_HeightSmall;

  if (Prefs.GameDisplay.toolbarTop) {
    boardRect.Offset(0, toolbar_Height);
    infoRect.Offset(0, toolbar_Height);
    tabRect.Offset(0, toolbar_Height);
    toolbarRect.Offset(0, -toolbarRect.top);
  }
} /* GameWindow::CalcFrames */

/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

BOOL GameWindow::HandleCloseRequest(void) {
  if (!AbandonRatedGame()) return false;
  if (!ExaChess) {
    CheckAbortEngine();
    return CheckSave("Save before closing?");
  } else {
    return QuestionDialog(this, "Close ExaChess Connection?",
                          "This window is currently used by ExaChess. Are you "
                          "sure you want to close?");
  }
} /* GameWindow::HandleCloseRequest */

BOOL GameWindow::HandleQuitRequest(void) {
  if (!AbandonRatedGame()) return false;
  CheckAbortEngine();
  //   UCI_QuitActiveEngine();
  //   UCI_QuitEngine(uciEngineId);
  return CheckSave("Save before quitting?");
} /* GameWindow::HandleQuitRequest */

void GameWindow::HandleZoom(void) {
  if (posEditor || annEditor || libEditor) return;
  showInfoArea = !showInfoArea;
  Prefs.GameDisplay.hideInfoArea = !showInfoArea;
  Resize((showInfoArea ? GameWin_Width(squareWidth)
                       : BoardArea_Width(squareWidth)),
         GameWin_Height(squareWidth));
  // Redraw(); //###
} /* GameWindow::HandleZoom */

void GameWindow::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (focusCtl && focusCtl->Enabled() &&
      focusCtl->HandleKeyDown(c, key, modifiers))
    return;

  if (theApp->ModalLoopRunning()) return;

  switch (key) {
    case key_LeftArrow:
      if (modifiers & modifier_Option)
        HandleMessage(game_UndoAllMoves);
      else if (modifiers & modifier_Control)
        HandleMessage(collection_PrevGame);
      else
        HandleMessage(game_UndoMove);
      break;
    case key_RightArrow:
      if (modifiers & modifier_Option)
        HandleMessage(game_RedoAllMoves);
      else if (modifiers & modifier_Control)
        HandleMessage(collection_NextGame);
      else
        HandleMessage(game_RedoMove);
      break;
    case key_Space:
      if (sigmaApp->analyzeMenu->MenuItemEnabled(analyze_Go))
        HandleMessage(modifiers & modifier_Option ? analyze_NextBest
                                                  : analyze_Go);
      else if (sigmaApp->analyzeMenu->MenuItemEnabled(analyze_Stop))
        HandleMessage(analyze_Stop);
      break;
    default:
#ifdef __debug_GameWin
      Debug_HandleKey(c, key);
#endif
      if (mode3D)
        boardAreaView->HandleKeyDown(c, key, modifiers);
      else if (!infoAreaView->HandleKeyDown(
                   c, key, modifiers)) {  //### handle other key strokes here...
      }
      break;
  }
} /* GameWindow::HandleKeyDown */

void GameWindow::HandleScrollBar(CScrollBar *ctrl, BOOL tracking) {
  if (infoAreaView->gameView->CheckScrollEvent(ctrl, tracking)) return;
  if (infoAreaView->libEditorView->CheckScrollEvent(ctrl, tracking)) return;
} /* GameWindow::HandleScrollBar */

void GameWindow::HandleEditor(CEditor *ctrl, BOOL textChanged, BOOL selChanged,
                              BOOL srcRplChanged) {
  if (annEditor && ctrl == infoAreaView->annEditorView->editor) {
    infoAreaView->AdjustAnnEditor(ctrl, textChanged, selChanged);
    AdjustTextEditMenu();
  }
  //## other editor controls here
} /* GameWindow::HandleEditor */

void GameWindow::HandleNullEvent(void) {
  if (sigmaApp->ModalLoopRunning())
    return;  // Ignore if any modal dialogs running

  if (!Engine_TaskRunning(engine))
    TickClock();
  else if (UsingUCIEngine())
    SendMsg_Async(engine, msg_Periodic);  // Make UCI clock tick. Remains in
                                          // queue until UCI engine done

  if (annEditor) CWindow::HandleNullEvent();
} /* GameWindow::HandleNullEvent */

void GameWindow::SetBoardSize(INT newSquareWidth) {
  Prefs.Appearance.squareWidth = newSquareWidth;
  CalcFrames(newSquareWidth);

  boardArea2DView->SetFrame(boardRect, true);
  infoAreaView->SetFrame(infoRect, true);
  tabAreaView->SetFrame(tabRect, true);
  toolbar->SetFrame(toolbarRect, true);
  miniToolbar->SetFrame(miniToolbarRect, true);
  Resize((showInfoArea ? GameWin_Width(squareWidth)
                       : BoardArea_Width(squareWidth)),
         GameWin_Height(squareWidth));
  AdjustDisplayMenu();
} /* GameWindow::SetBoardSize */
