/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAMEWINDOWMENU.C */
/* Purpose : This module implements the game window menus/message handling. */
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

#include "AnalyzeGameDialog.h"
#include "CDialog.h"
#include "CollectionWindow.h"
#include "Engine.f"
#include "EngineMatchDialog.h"
#include "ExaChessGlue.h"
#include "GameInfoDialog.h"
#include "GameWindow.h"
#include "GotoMoveDialog.h"
#include "InfoFilterDialog.h"
#include "InitialStatusDialog.h"
#include "LevelDialog.h"
#include "LibCommentDialog.h"
#include "PGN.h"
#include "PosLibrary.h"
#include "PrefsDialog.h"
#include "Rating.h"
#include "SigmaApplication.h"
#include "StrengthDialog.h"
#include "UCI_ConfigDialog.h"
#include "UCI_Option.h"

#include "Board.f"

#include "TaskScheduler.h"

//#include "Debug.h"  //###

/**************************************************************************************************/
/*                                                                                                */
/*                                       MESSAGE HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void GameWindow::HandleMessage(LONG msg, LONG submsg, PTR data) {
  if (promoting || theApp->ModalLoopRunning()) return;

  switch (msg) {
      /*----------------------------------------- FILE Menu
       * --------------------------------------------*/

    case file_Save:
      Save();
      break;
    case file_SaveAs:
      SaveAs();
      break;
    case file_Close:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      if (HandleCloseRequest()) delete this;
      break;
    case file_ExportHTML:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      ExportHTML();
      break;
    case file_Print:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      PrintGame();
      break;

      /*----------------------------------------- EDIT Menu
       * --------------------------------------------*/

    case edit_Undo:
      Undo();
      break;
    case edit_Redo:
      Redo();
      break;
    case cut_Standard:
      Cut();
      break;
    case copy_Standard:
      Copy();
      break;
    case paste_Standard:
      if (!AbandonRatedGame())
        return;
      else
        Paste();
      break;
    case edit_Clear:
      Clear();
      break;
    case edit_ClearAll:
      ClearAll();
      break;
    case edit_Diagram:
      infoAreaView->annEditorView->InsertDiagram();
      break;

    case edit_Find:
      (::GetCurrentKeyModifiers() & modifier_Option ? FindAgain() : Find());
      break;
    case edit_FindAgain:
      FindAgain();
      break;
    case edit_Replace:
      Replace();
      break;
    case edit_ReplaceFind:
      ReplaceFind();
      break;
    case edit_ReplaceAll:
      ReplaceAll();
      break;

    case copy_Game:
      CopyGame(true);
      break;
    case copy_GameNoAnn:
      CopyGame(false);
      break;
    case paste_Game:
      if (!AbandonRatedGame())
        return;
      else
        PasteGame();
      break;

    case copy_Position:
      CopyPosition();
      break;
    case paste_Position:
      if (!AbandonRatedGame())
        return;
      else
        PastePosition();
      break;

    case copy_Analysis:
      CopyAnalysis();
      break;

    case edit_SetAnnGlyph:
      game->SetAnnotationGlyph(game->currMove, submsg);
      game->dirty = true;
      infoAreaView->RedrawGameList();
      infoAreaView->AdjustAnnGlyph();
      miniToolbar->Adjust();
      toolbar->Adjust();  //###??
      break;

      /*----------------------------------------- GAME Menu
       * --------------------------------------------*/

    case game_ResetGame:
      if (!AbandonRatedGame()) return;
      FlushAnnotation();
      if (!CheckSave("Save before resetting game?")) return;
      if (!demoPlaying) CheckAbortEngine();
      if (colWin) HandleMessage(game_Detach);
      SetTitle("<Untitled Game>");
      if (file) delete file;
      file = nil;
      game->NewGame();
      infoAreaView->ResetAnalysis();
      GameMoveAdjust(true);
      AdjustFileMenu();

      if (submsg == analyze_DemoPlay) {
        CopyStr(engineName, game->Info.whiteName);
        CopyStr(engineName, game->Info.blackName);
      } else if (submsg == analyze_EngineMatch) {
        CHAR title[cwindow_MaxTitleLen + 1];
        Format(title, "Game %d of %d (+%d =%d -%d)", EngineMatch.currGameNo,
               Prefs.EngineMatch.matchLen, EngineMatch.winCount1,
               EngineMatch.drawCount, EngineMatch.winCount2);
        SetTitle(title);

        EngineMatch.timeForfeit = false;
        EngineMatch.prevScore = 1;
        EngineMatch.adjWinCount = 0;
        EngineMatch.adjDrawCount = 0;

        INT whiteEngineId = Prefs.EngineMatch.engine1;
        INT blackEngineId = Prefs.EngineMatch.engine2;
        if (Prefs.EngineMatch.alternate && even(EngineMatch.currGameNo))
          Swap(&whiteEngineId, &blackEngineId);

        CopyStr(UCI_EngineName(whiteEngineId), game->Info.whiteName);
        CopyStr(UCI_EngineName(blackEngineId), game->Info.blackName);

        if (EngineMatch.currGameNo > 1 && uciEngineId != whiteEngineId &&
            Prefs.EngineMatch.engine1 != uci_SigmaEngineId &&
            Prefs.EngineMatch.engine2 != uci_SigmaEngineId)
          UCI_SwapEngines();

        uciEngineId = whiteEngineId;

        if (boardTurned != (Prefs.EngineMatch.engine1 == blackEngineId))
          TurnBoard();
      } else if ((submsg == game_RateGame) ||
                 (!game->Info.whiteName[0] && !game->Info.blackName[0])) {
        CopyStr(Prefs.General.playerName, game->Info.whiteName);
        CopyStr(engineName, game->Info.blackName);
      }

      RefreshGameInfo();
      ResetClocks();
      timeoutContinued = false;
      break;

    case game_BranchGame:
#ifdef __libTest_Verify
      sigmaPrefs->EnableLibrary(false);
      VerifyPosLib();
      PosLib_PurifyFlags();
#else
      if (!AbandonRatedGame()) return;
      Prefs.GameDisplay.boardTurned =
          boardTurned;  // This way branched game inherits turn state
      GameWindow *varWin = NewGameWindow("<Variation>");
      if (!varWin) return;
      varWin->game->CopyFrom(game, false, true, true);
      varWin->GameMoveAdjust(true);
#endif
      break;

    case game_RateGame:
      if (!EngineSupportsRating("New Rated Game")) return;
      CheckAbortEngine();
      RateGame();
      break;

    case game_ReplayGame:
      ReplayGame();
      break;

    case game_ClearRest:
      CheckAbortEngine();
      game->lastMove = game->currMove;
      game->dirty = true;
      GameMoveAdjust(false);
      AdjustFileMenu();
      break;

    case game_ClearAnn:
      game->ClrAnnotation();
      GameMoveAdjust(false);
      break;

    case game_Detach:
      if (!colWin) return;
      colWin->DetachGameWin(this);
      Detach();
      AdjustCollectionMenu();
      break;

    case game_UndoMove:
      if (!game->CanUndoMove() || thinking || ExaChess) return;
      if (!AbandonRatedGame()) return;
      CheckAbortEngine();
      StopClock();
      FlushAnnotation();
      boardAreaView->ClearMoveMarker();
      game->UndoMove(true);
      boardAreaView->DrawUndoMove();
      GameMoveAdjust(false);
      hasResigned = hasAnnouncedMate = false;
      CheckMonitorMode();
      break;

    case game_RedoMove:
      if (!game->CanRedoMove() || thinking || ExaChess) return;
      CheckAbortEngine();
      StopClock();
      FlushAnnotation();
      boardAreaView->ClearMoveMarker();
      game->RedoMove(true);
      boardAreaView->DrawMove();
      GameMoveAdjust(false);
      CheckMonitorMode();
      break;

    case game_UndoAllMoves:
      if (!game->CanUndoMove() || thinking || ExaChess) return;
      if (!AbandonRatedGame()) return;
      CheckAbortEngine();
      StopClock();
      FlushAnnotation();
      boardAreaView->ClearMoveMarker();
      game->UndoAllMoves();
      GameMoveAdjust(true);
      hasResigned = hasAnnouncedMate = false;
      break;

    case game_RedoAllMoves:
      if (!game->CanRedoMove() || thinking || ExaChess) return;
      CheckAbortEngine();
      StopClock();
      FlushAnnotation();
      boardAreaView->ClearMoveMarker();
      game->RedoAllMoves();
      GameMoveAdjust(true);
      break;

    case game_GotoMove:
      if (game->lastMove == 0) return;
      if (!AbandonRatedGame()) return;
      FlushAnnotation();
      INT j =
          GotoMoveDialog(game->Init.player, game->Init.moveNo, game->lastMove);
      if (j >= 0) GotoMove(j, false);
      break;

    case game_PositionEditor:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      HandleMessage(!posEditor ? posEditor_Open : posEditor_Done);
      break;

    case game_AnnotationEditor:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      if (posEditor) {
        if (!QuestionDialog(this, "Annotation Editor",
                            "You need to abort the ÒPosition EditorÓ before "
                            "you can open the ÒAnnotation EditorÓ. Proceed?",
                            "Yes", "No"))
          return;
        HandleMessage(posEditor_Cancel);
      }
      if (!annEditor && libEditor) HandleMessage(library_Editor);
      annEditor = !annEditor;
      if (!showInfoArea)
        Resize((annEditor ? GameWin_Width(squareWidth)
                          : BoardArea_Width(squareWidth)),
               GameWin_Height(squareWidth));
      infoAreaView->ShowAnnEditor(annEditor);
      AdjustEditMenu();
      break;

    case game_GameInfo:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      if (GameInfoDialog(this, &game->Info)) {
        game->dirty = true;
        AdjustFileMenu();
        AdjustToolbar();
        RefreshGameInfo();
      }
      break;

      /*---------------------------------------- ANALYZE Menu
       * ------------------------------------------*/

    case engine_Configure:
      // First asks user if any rated games should be abandoned. Then all
      // engines taks are aborted.
      if (UCI_ConfigDialog(uciEngineId))  // Updates Prefs.UCI.defaultId
        SelectEngine(Prefs.UCI.defaultId);
      break;

    case analyze_Go:
      if (!AbandonRatedGame()) return;
      if (CheckEngineMatch()) return;
      CheckAbortEngine();
      Analyze_Go();
      break;

    case analyze_NextBest:
      if (CheckEngineMatch()) return;
      if (UsingUCIEngine()) {
        NoteDialog(this, "Next Best",
                   "ÒNext BestÓ is not available for UCI engines...");
        return;
      }
      if (!AbandonRatedGame()) return;
      CheckAbortEngine();
      if (game->currMove == moveAnalyzed + 1 && level.mode != pmode_Monitor)
        HandleMessage(game_UndoMove);
      if (EqualTable(BoardAnalyzed, game->Board) &&
          moveAnalyzed == game->currMove)
        Analyze_Go(true);
      else
        NoteDialog(this, "Next Best",
                   "ÒNext BestÓ can only be applied to the most recently "
                   "analyzed board position...");
      break;

    case analyze_Stop:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch(true)) return;
      Analyze_Stop();
      break;

    case analyze_Pause:
      if (!AbandonRatedGame()) return;
      NoteDialog(nil, "Game Paused",
                 "The game is currently paused. Click ÒResumeÓ when you want "
                 "to continue the game...",
                 1200, "Resume");
      break;

    case analyze_Hint:
      if (!AbandonRatedGame()) return;
      Analyze_Hint();
      break;

    case analyze_PlayMainLine:
      if (!AbandonRatedGame()) return;
      PlayMainLine();
      break;

    case analyze_DrawOffer:
      drawOffered = true;
      NoteDialog(this, "Draw Offer", "Your draw offer will be considered...");
      AdjustAnalyzeMenu();
      break;

    case analyze_Resign:
      PlayerResigns();
      break;

    case analyze_AutoPlay:
      if (!AbandonRatedGame()) return;
      if (CheckEngineMatch()) return;
      CheckAbortEngine();
      Analyze_AutoPlay();
      break;

    case analyze_DemoPlay:
      if (!AbandonRatedGame()) return;
      if (CheckEngineMatch()) return;
      CheckAbortEngine();
      Analyze_DemoPlay();
      break;

    case analyze_AnalyzeGame:
      if (CheckEngineMatch()) return;
      if (game->lastMove == 0) {
        NoteDialog(this, "Cannot Analyze Empty Games",
                   "'Analyze Game' analyzes the moves in a game, and is "
                   "therefore not available for empty games.");
      } else if (game->currMove == game->lastMove) {
        NoteDialog(this, "Cannot Analyze at End of Game",
                   "'Analyze Game' analyzes the moves in a game starting from "
                   "the current position, and is therefore not available at "
                   "the end of a game.");
      } else if (AnalyzeGameDialog(this, true)) {
        CheckAbortEngine();
        AnalyzeGame();
      }
      break;

    case analyze_AnalyzeCol:
      if (!ProVersionDialog(this)) return;
      if (!AbandonRatedGame()) return;
      if (CheckEngineMatch()) return;
      if (AnalyzeGameDialog(this, false)) {
        CheckAbortEngine();
        AnalyzeCollection();
      }
      break;

    case analyze_AnalyzeEPD:
      if (!AbandonRatedGame()) return;
      if (CheckEngineMatch()) return;
      AnalyzeEPD();
      break;

    case analyze_EngineMatch:
      if (!ProVersionDialog(this)) return;
      if (!AbandonRatedGame()) return;
      CheckAbortEngine();
      if (!CheckSave("Save before engine match?")) return;
      if (EngineMatchDialog(this)) Analyze_EngineMatch();
      break;

    case analyze_TransTables:
      if (!AbandonRatedGame()) return;
      if (EngineMatch.gameWin) {
        NoteDialog(this, "Engine Match",
                   "An engine match is currently being played...");
        return;
      }
      PrefsDialog(prefs_TransTab);
      break;

    case analyze_EndgameDB:
      Prefs.useEndgameDB = !Prefs.useEndgameDB;
      sigmaApp->analyzeMenu->CheckMenuItem(analyze_EndgameDB,
                                           Prefs.useEndgameDB);
      break;

    case analyze_Completed:
      SearchCompleted();
      break;

      /*----------------------------------------- LEVEL Menu
       * -------------------------------------------*/

    case level_Select:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      if (Level_Dialog(&level)) {  // if (monitoring) Analyze_Stop();
        CheckAbortEngine();
        Prefs.Level.level = level;
        ResetClocks();
        boardAreaView->DrawModeIcons();
        AdjustLevelMenu();
        AdjustToolbar();
        if (!MultiPVAllowed()) SetMultiPVcount(1);
      }
      break;

    case level_PermanentBrain:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      if (!UCI_SupportsPonderOption(uciEngineId)) {
        CHAR msg[300];
        Format(msg,
               "The '%s' engine does not support permanent brain (pondering)",
               engineName);
        NoteDialog(this, "Permanent Brain not Supported", msg);
      } else {
        permanentBrain = !permanentBrain;
        AdjustLevelMenu();
        AdjustToolbar();
      }
      break;

    case level_SetPlayingMode:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      if (submsg == pmode_Monitor &&
          !ProVersionDialog(
              this, "Monitor mode is not available in Sigma Chess Lite."))
        return;
      //       if (monitoring) Analyze_Stop();
      CheckAbortEngine();
      Prefs.Level.level.mode = level.mode = submsg;
      ResetClocks();
      boardAreaView->DrawModeIcons();
      AdjustAnalyzeMenu();
      AdjustLevelMenu();
      AdjustToolbar();
      if (!MultiPVAllowed()) SetMultiPVcount(1);
      break;

    case level_SetPlayingStyle:
      sigmaApp->HandleMessage(submsg);
      break;

    case level_SigmaELO:
      if (!AbandonRatedGame()) return;
      if (!EngineSupportsRating("Configure Rating")) return;
      if (UsingUCIEngine() &&
          !ProVersionDialog(this,
                            "The playing strength for UCI engines cannot be "
                            "configured in Sigma Chess Lite."))
        return;
      if (EngineRatingDialog(uciEngineId, &engineRating)) miniToolbar->Adjust();
      break;

      /*---------------------------------------- DISPLAY Menu
       * ------------------------------------------*/

    case display_TurnBoard:
      TurnBoard();
      Prefs.GameDisplay.boardTurned =
          boardTurned;  // Automatically update prefs
      break;
    case display_ShowAnalysis:
      infoAreaView->ShowAnalysis(!infoAreaView->showAnalysis);
      Prefs.GameDisplay.showAnalysis = infoAreaView->showAnalysis;
      break;

    case display_VerPV:
      Prefs.GameDisplay.varDisplayVer = varDisplayVer = true;
      infoAreaView->RefreshAnalysis();
      break;
    case display_HorPV:
      Prefs.GameDisplay.varDisplayVer = varDisplayVer = false;
      infoAreaView->RefreshAnalysis();
      break;

    case display_IncMultiPV:
      IncMultiPVcount();
      break;
    case display_DecMultiPV:
      DecMultiPVcount();
      break;

    case display_ToggleInfoArea:
      HandleZoom();
      break;
    case display_GameRecord:
      if (GameInfoFilterDialog(&infoFilter)) {
        Prefs.GameDisplay.gameInfoFilter = infoFilter;
        infoAreaView->ResizeGameHeader();
      }
      break;
    case display_3DBoard:
      if (posEditor)
        NoteDialog(this, "Toggle 2D/3D",
                   "You must close the Position Editor before switching "
                   "between 2D and 3D");
      else
        Toggle3D();
      break;
    case display_Show3DClock:
      Prefs.GameDisplay.show3DClocks = !Prefs.GameDisplay.show3DClocks;
      sigmaApp->displayMenu->CheckMenuItem(display_Show3DClock,
                                           Prefs.GameDisplay.show3DClocks);
      boardArea3DView->ToggleClocks();
      break;

    case display_ToolbarTop:
      toolbarTop = !toolbarTop;
      Prefs.GameDisplay.toolbarTop = toolbarTop;
      CalcFrames(squareWidth);
      toolbar->SetFrame(toolbarRect);
      boardArea2DView->SetFrame(boardRect);
      infoAreaView->SetFrame(infoRect);
      tabAreaView->SetFrame(tabRect);
      AdjustDisplayMenu();
      Redraw();
      FlushPortBuffer();
      break;

      /*-------------------------------------- COLLECTION Menu
       * -----------------------------------------*/

    case collection_PrevGame:
      if (colWin && colWin->CanPrevGame() &&
          CheckSave("Save before selecting previous game?")) {
        CheckAbortEngine();
        infoAreaView->ResetAnalysis();
        colWin->PrevGame(this);
        AdjustCollectionMenu();
      }
      break;
    case collection_NextGame:
      if (colWin && colWin->CanNextGame() &&
          CheckSave("Save before selecting next game?")) {
        CheckAbortEngine();
        infoAreaView->ResetAnalysis();
        colWin->NextGame(this);
        AdjustCollectionMenu();
      }
      break;

      /*---------------------------------------- LIBRARY Menu
       * ------------------------------------------*/

    case library_Editor:
      if (!AbandonRatedGame()) return;
      if (!AbandonEngineMatch()) return;
      if (!libEditor)
        ProVersionDialog(this,
                         "Please note that saving is disabled for position "
                         "libraries in Sigma Chess Lite.");
      if (posEditor) {
        if (!QuestionDialog(this, "Library Editor",
                            "You need to abort the ÒPosition EditorÓ before "
                            "you can open the ÒLibrary EditorÓ. Proceed?",
                            "Yes", "No"))
          return;
        HandleMessage(posEditor_Cancel);
      }

      if (!libEditor && annEditor) HandleMessage(game_AnnotationEditor);
      libEditor = !libEditor;
      sigmaApp->libraryMenu->CheckMenuItem(library_Editor, libEditor);
      if (!showInfoArea)
        Resize((libEditor ? GameWin_Width(squareWidth)
                          : BoardArea_Width(squareWidth)),
               GameWin_Height(squareWidth));
      infoAreaView->ShowLibEditor(libEditor);
      AdjustDisplayMenu();
      AdjustLibraryMenu();
      break;

    case library_ClassifyPos:
      if (!AbandonRatedGame()) return;
      PosLib_Classify(game->player, game->Board, (LIB_CLASS)submsg,
                      true);  // Overwrite per default
      AdjustLibraryMenu();
      infoAreaView->RefreshLibInfo();
      break;

    case library_ECOComment:
      if (!AbandonRatedGame()) return;
      if (LibCommentDialog(game)) infoAreaView->RefreshGameStatus();
      break;

    case library_DeleteVar:
      if (!QuestionDialog(
              this, "Delete Variations",
              "WARNING! This will delete ALL subsequent positions reachable "
              "from the current board position. Are you sure?",
              "No", "Yes")) {
        Poslib_CascadeDelete(game, true, true);
        sigmaApp->BroadcastMessage(msg_RefreshPosLib);
      }
      break;

    case library_ImportCollection:
      NoteDialog(this, "Import",
                 "You first need to open a collection and select some games to "
                 "import...");
      break;

      /*----------------------------------- POSITION EDITOR Commands
       * -----------------------------------*/

    case posEditor_Open:
      if (!CheckSave("Save before opening the position editor?")) return;
      CheckAbortEngine();
      ResetClocks();
      if (annEditor) HandleMessage(game_AnnotationEditor);
      if (libEditor) HandleMessage(library_Editor);
      posEditor = true;
      boardAreaView->ClearMoveMarker();
      game->Edit_Begin();
      HandleMenuAdjust();
      AdjustToolbar();
      if (mode3D)
        boardArea3DView->ShowPosEditor(true);
      else {
        if (!showInfoArea)
          Resize(GameWin_Width(squareWidth), GameWin_Height(squareWidth));
        infoAreaView->ShowPosEditor(true);
      }
      break;
    case posEditor_Close:
      posEditor = false;
      HandleMenuAdjust();
      AdjustToolbar();
      if (mode3D)
        boardArea3DView->ShowPosEditor(false);
      else {
        infoAreaView->ShowPosEditor(false);
        if (!showInfoArea)
          Resize(BoardArea_Width(squareWidth), GameWin_Height(squareWidth));
      }
      boardAreaView->SetMoveMarker(false);
      break;
    case posEditor_Done:
      if (LegalPosition()) {
        game->Edit_End(true);
        infoAreaView->UpdateGameList();
        HandleMessage(posEditor_Close);
      }
      break;
    case posEditor_Cancel:
      game->Edit_End(false);
      if (!mode3D) DrawBoard(false);
      HandleMessage(posEditor_Close);
      break;

    case posEditor_ClearBoard:
      game->Edit_ClearBoard();
      DrawBoard(false);
      break;
    case posEditor_NewBoard:
      game->Edit_NewBoard();
      DrawBoard(false);
      break;
    case posEditor_Status:
      InitialStatusDialog(game);
      break;
    case posEditor_SelectPiece:
      infoAreaView->posEditorView->SelectPiece(submsg);
      break;
    case posEditor_SelectPlayer:
      if (!mode3D)
        infoAreaView->posEditorView->SelectPlayer(submsg);
      else
        boardArea3DView->SelectPlayer(submsg);
      boardAreaView->DrawPlayerIndicator();
      boardAreaView->DrawModeIcons();
      break;

      /*------------------------------------ MISCELLANEOUS MESSAGES
       * ------------------------------------*/

    case boardSize_Standard:
      SetBoardSize(squareWidth1);
      break;
    case boardSize_Medium:
      SetBoardSize(squareWidth2);
      break;
    case boardSize_Large:
      SetBoardSize(squareWidth3);
      break;
    case boardSize_EvenLarger:
      SetBoardSize(squareWidth4);
      break;

    case msg_RefreshColorScheme:
      boardAreaView->Redraw();
      infoAreaView->Redraw();
      tabAreaView->DrawAllTabs();
      break;

    case msg_RefreshPieceSet:
      DrawBoard(false);
      if (posEditor || promoting) infoAreaView->RefreshPieceSet();
      break;

    case msg_RefreshBoardType:
      DrawBoard(true);
      break;

    case msg_RefreshMoveNotation:
      boardAreaView->DrawBoardFrame();
      if (!posEditor) infoAreaView->RefreshNotation();
      miniToolbar->Adjust();
      break;

    case msg_RefreshInfoSep:
      miniToolbar->Adjust();
      break;

    case msg_RefreshGameMoveList:
      infoAreaView->UpdateGameList();
      break;

    case msg_RefreshPosLib:
      infoAreaView->RefreshLibInfo();
      break;

    case msg_UCIEngineRemoved:
      if (submsg ==
          uciEngineId)  // Revert to Sigma engine if current engine was removed
        SelectEngine(uci_SigmaEngineId);
      else if (submsg < uciEngineId)  // "Shift" engine id down if an engine
                                      // with a lower id was removed
        uciEngineId--;
      break;

    case msg_UCISetSigmaEngine:
      SelectEngine(uci_SigmaEngineId);
      break;

    default:
      if (msg >= game_AddToColFirst && msg <= game_AddToColLast) {
        AddToCollection(msg);
      } else if (msg == engine_Sigma ||
                 (msg >= engine_CustomFirst && msg <= engine_CustomLast)) {
        if (UCI_AbortAllEngines()) {
          UCI_ENGINE_ID newEngineId = msg - engine_Sigma;

          // Select the new engine
          SelectEngine(newEngineId);
          Prefs.UCI.defaultId = uciEngineId;
        }
      }
  }
} /* GameWindow::HandleMessage */

/**************************************************************************************************/
/*                                                                                                */
/*                                              MISC */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------- Copy/Paste Games/Positions
 * ---------------------------------*/

void GameWindow::CopyGame(BOOL includeAnn) {
  theApp->ResetClipboard();
  UINT gameSize = game->Write_PGN((CHAR *)gameData, includeAnn);
  theApp->WriteClipboard('TEXT', gameData, gameSize);
} /* GameWindow::CopyGame */

void GameWindow::PasteGame(void) {
  CheckAbortEngine();
  if (colWin) HandleMessage(game_Detach);

  PTR gdata = nil;
  LONG gsize;

  if (theApp->ReadClipboard('TEXT', &gdata, &gsize) != appErr_NoError) {
    NoteDialog(this, "Paste Game", "No game was found on the clipboard...",
               cdialogIcon_Warning);
  } else {
    CGame *gameTemp = new CGame();
    CPgn *pgnTemp = new CPgn(gameTemp);

    pgnTemp->ReadBegin((CHAR *)gdata);
    if (pgnTemp->ReadGame(gsize)) {
      game->CopyFrom(gameTemp);

      game->dirty = true;
      SetTitle("<Pasted Game>");
      RefreshGameInfo();
      ResetClocks();

      if (!Prefs.Games.gotoFinalPos && game->CanUndoMove())
        HandleMessage(game_UndoAllMoves);
      else
        GameMoveAdjust(true);
    } else if (gameTemp->lastMove > 0) {
      CHAR s[200];
      Format(s,
             "An error occured in move %d. You can try to correct this by "
             "pasting the game into a text editor.",
             gameTemp->lastMove / 2 + 1);
      NoteDialog(this, "Failed Pasting Game", s, cdialogIcon_Error);
    } else {
      NoteDialog(this, "Failed Pasting Game",
                 "No valid game format was found on the clipboard...",
                 cdialogIcon_Error);
    }

    delete pgnTemp;
    delete gameTemp;

    Mem_FreePtr(gdata);
  }
} /* GameWindow::PasteGame */

void GameWindow::CopyPosition(void) {
  CHAR EPD[epdBufSize];
  INT n = game->Write_EPD(EPD);
  theApp->ResetClipboard();
  theApp->WriteClipboard('TEXT', (PTR)EPD, n);
  //### check return code!!!
} /* GameWindow::CopyPosition */

void GameWindow::PastePosition(void) {
  if (colWin) HandleMessage(game_Detach);

  PTR s = nil;
  LONG size;

  if (theApp->ReadClipboard('TEXT', &s, &size) != appErr_NoError) {
    NoteDialog(this, "Paste Position",
               "No position was found on the clipboard...",
               cdialogIcon_Warning);
  } else if (CheckSave("Save before pasting position?")) {
    CheckAbortEngine();

    if (game->Read_EPD((CHAR *)s) != epdErr_NoError)
      NoteDialog(this, "Error", "Failed parsing EPD position",
                 cdialogIcon_Error);
    else {
      CHAR *id = game->Info.heading;
      SetTitle(EqualStr(id, "") ? "<Untitled Position>" : id);
      RefreshGameInfo();
      GameMoveAdjust(true);
      ResetClocks();
    }

    Mem_FreePtr((PTR)s);
  }
} /* GameWindow::PastePosition */

/*--------------------------------------- Adjust Move List
 * ---------------------------------------*/
// Called whenever the contents of the game record has changed (i.e. new moves
// played or annotations added etc).

void GameWindow::GameMoveAdjust(BOOL redrawBoard, BOOL engineMovePlayed) {
  // DebugWriteNL("  Adjust ECO");  //###
  AutoSetECO();
  if (redrawBoard) DrawBoard(false);
  clrMove(hintMove);
  // DebugWriteNL("  Set Move Marker");  //###
  boardAreaView->SetMoveMarker(engineMovePlayed);
  // DebugWriteNL("  Draw player indicator");  //###
  boardAreaView->DrawPlayerIndicator();
  boardAreaView->DrawModeIcons();
  boardAreaView->DrawLevelInfo(white);
  boardAreaView->DrawLevelInfo(black);
  boardAreaView->RefreshGameStatus();  //###
  // DebugWriteNL("  Refresh Game Status");  //###
  infoAreaView->RefreshGameStatus();
  // DebugWriteNL("  Update Game List");  //###
  infoAreaView->UpdateGameList();
  // DebugWriteNL("  Load Annotation");  //###
  infoAreaView->LoadAnnotation();
  // DebugWriteNL("  Adjust menus");  //###
  AdjustFileMenu();
  AdjustGameMenu();
  AdjustAnalyzeMenu();
  AdjustToolbar();

  // DebugWriteNL("  Flush port buffer");  //###
  FlushPortBuffer();
} /* GameWindow::GameMoveAdjust */

void GameWindow::AutoSetECO(void) {
  CHAR eco[libECOLength + 1], comment[libCommentLength + 1];

  if (game->currMove == game->lastMove &&
      PosLib_ProbeStr(game->player, game->Board, eco, comment) &&
      !EqualStr(eco, ""))
    ::CopyStr(eco, game->Info.ECO);
} /* GameWindow::AutoSetECO */

void GameWindow::TurnBoard(void) {
  boardTurned = !boardTurned;
  sigmaApp->displayMenu->CheckMenuItem(display_TurnBoard, boardTurned);
  // toolbar->tb_TurnBoard->Press(boardTurned);
  DrawBoard(true);
} /* GameWindow::TurnBoard */

void GameWindow::CheckTurnPlayer(void) {
  if (!Prefs.Games.turnPlayer) return;

  CHAR *topPlayer = (boardTurned ? game->Info.whiteName : game->Info.blackName);
  if (SameStr(topPlayer, Prefs.General.playerName)) TurnBoard();
} /* GameWindow::CheckTurnPlayer */

/**************************************************************************************************/
/*                                                                                                */
/*                                       WINDOW/MENU ACTIVATION */
/*                                                                                                */
/**************************************************************************************************/

// If a game window is moved to the front, we have to update the menu state
// (enable, checks) accordingly. This method is also called after a window has
// been created:

void GameWindow::HandleMenuAdjust(void) {
  if (!IsActive()) return;

  sigmaApp->ShowMenuBar(!mode3D);
  sigmaApp->EnableQuitCmd(true);  // OS X Menu enabling
  sigmaApp->EnablePrefsCmd(true);
  sigmaApp->EnableAboutCmd(true);

  if (promoting)
    sigmaApp->EnableMenuBar(false);
  else {
    sigmaApp->EnableMenuBar(true);
    AdjustFileMenu();
    AdjustEditMenu();
    AdjustGameMenu();
    AdjustAnalyzeMenu();
    AdjustEngineMenu();
    AdjustLevelMenu();
    AdjustDisplayMenu();
    AdjustCollectionMenu();
    AdjustLibraryMenu();
    sigmaApp->RedrawMenuBar();
  }
} /* GameWindow::HandleMenuAdjust */

/*--------------------------------- Adjusting Game Window Menus
 * ----------------------------------*/

void GameWindow::AdjustFileMenu(void) {
  SetModified(game->dirty);

  if (!IsFront()) return;

  CMenu *m = sigmaApp->fileMenu;
  BOOL em = (EngineMatch.gameWin == this);

  m->EnableMenuItem(file_NewGame, true);        // Always enabled
  m->EnableMenuItem(file_NewCollection, true);  // Always enabled
  m->EnableMenuItem(file_NewLibrary, true);     // Always enabled
  m->EnableMenuItem(file_Open, true);           // Always enabled
  m->EnableMenuItem(file_Save, !em && game->dirty && !posEditor && !ExaChess);
  m->EnableMenuItem(file_SaveAs, !posEditor);
  m->EnableMenuItem(file_Close, !em);
  m->EnableMenuItem(file_PageSetup, !em);  // Always enabled
  m->EnableMenuItem(file_ExportHTML, !em && !posEditor);
  m->EnableMenuItem(file_Print, !em && !posEditor);
  m->EnableMenuItem(file_Preferences, true);  // Always enabled
  m->EnableMenuItem(file_Quit, true);         // Always enabled
} /* GameWindow::AdjustFileMenu */

void GameWindow::AdjustEditMenu(void) {
  if (!IsFront()) return;

  AdjustTextEditMenu();

  CMenu *m = sigmaApp->editMenu;
  m->EnableMenuItem(edit_SelectAll, annEditor);
  m->EnableMenuItem(edit_Find, annEditor);

  m = sigmaApp->cutMenu;
  m->EnableMenuItem(cut_Game, false);
  m->ClrShortcut(cut_Game);
  m->SetShortcut(cut_Standard, 'X');

  m = sigmaApp->copyMenu;
  m->EnableMenuItem(copy_Game, !posEditor);
  m->EnableMenuItem(copy_GameNoAnn, !posEditor);
  m->EnableMenuItem(copy_Position, !posEditor);
  m->EnableMenuItem(copy_Analysis, !posEditor && !annEditor);
  if (!annEditor) {
    m->ClrShortcut(copy_Standard);
    m->SetShortcut(copy_Game, 'C');
  } else {
    m->SetShortcut(copy_Standard, 'C');
    m->SetShortcut(copy_Game, 'C', cMenuModifier_Shift);
  }
  m->SetShortcut(copy_Position, 'C', cMenuModifier_Option);
  m->SetShortcut(copy_Analysis, 'C', cMenuModifier_Control);

  m = sigmaApp->pasteMenu;
  m->EnableMenuItem(paste_Standard, annEditor);
  m->EnableMenuItem(paste_Game, !annEditor && !ExaChess);
  m->EnableMenuItem(paste_Position, !annEditor && !ExaChess);
  if (!annEditor) {
    m->ClrShortcut(paste_Standard);
    m->SetShortcut(paste_Game, 'V');
    m->SetShortcut(paste_Position, 'V', cMenuModifier_Option);
  } else {
    m->SetShortcut(paste_Standard, 'V');
    m->ClrShortcut(paste_Game);
    m->ClrShortcut(paste_Position);
  }
} /* GameWindow::AdjustEditMenu */

void GameWindow::AdjustTextEditMenu(void) {
  CMenu *m = sigmaApp->editMenu;
  CEditor *editor = infoAreaView->annEditorView->editor;
  BOOL sel = (annEditor && editor->TextSelected());

  m->EnableMenuItem(edit_Undo, annEditor && editor->CanUndo());
  m->EnableMenuItem(edit_Redo, annEditor && editor->CanRedo());
  m->EnableMenuItem(edit_Clear, sel);

  m->EnableMenuItem(edit_FindAgain, annEditor && editor->CanFindAgain());
  m->EnableMenuItem(edit_Replace, annEditor && editor->CanReplace());
  m->EnableMenuItem(edit_ReplaceFind, annEditor && editor->CanReplace());
  m->EnableMenuItem(edit_ReplaceAll, annEditor && editor->CanReplace());

  sigmaApp->cutMenu->EnableMenuItem(cut_Standard, sel);
  sigmaApp->copyMenu->EnableMenuItem(copy_Standard, sel);
} /* GameWindow::AdjustTextEditMenu */

void GameWindow::AdjustGameMenu(void) {
  if (!IsFront()) return;

  BOOL tx = (thinking || ExaChess);
  BOOL em = (EngineMatch.gameWin == this);

  CMenu *m = sigmaApp->gameMenu;
  m->EnableMenuItem(game_ResetGame, !tx && !posEditor);
  m->EnableMenuItem(game_BranchGame, !tx && !posEditor);
  m->EnableMenuItem(game_RateGame, !posEditor && !isRated);
  m->EnableMenuItem(game_ClearRest, !tx && !posEditor && game->CanRedoMove());
  m->EnableMenuItem(game_AddToCollection,
                    !em && sigmaApp->addToColMenu && !colWin && !ExaChess);
  m->EnableMenuItem(game_Detach, !em && colWin != nil && !colWin->busy);
  m->EnableMenuItem(game_UndoMove, !tx && game->CanUndoMove());
  m->EnableMenuItem(game_UndoAllMoves, !tx && game->CanUndoMove());
  m->EnableMenuItem(game_RedoMove, !tx && game->CanRedoMove());
  m->EnableMenuItem(game_RedoAllMoves, !tx && game->CanRedoMove());
  m->EnableMenuItem(game_GotoMove,
                    !tx && (game->CanUndoMove() || game->CanRedoMove()));
  m->EnableMenuItem(game_PositionEditor, !tx);
  m->EnableMenuItem(game_AnnotationEditor, !em && !mode3D && !ExaChess);
  m->EnableMenuItem(game_GameInfo, !em && !posEditor && !ExaChess);

  m->CheckMenuItem(game_RateGame, isRated);
} /* GameWindow::AdjustGameMenu */

void GameWindow::AdjustAnalyzeMenu(void) {
  if (!IsFront()) return;

  BOOL tx = (thinking || ExaChess);
  BOOL em = (EngineMatch.gameWin != nil);  // == this);

  CMenu *m = sigmaApp->analyzeMenu;
  if (RunningOSX()) m->EnableMenuItem(analyze_Engine, !em && UCI_Enabled());
  m->EnableMenuItem(analyze_Go,
                    !tx && !posEditor && !monitoring && !game->GameOver());
  m->EnableMenuItem(analyze_NextBest,
                    !tx && !posEditor && !monitoring && !game->GameOver());
  m->EnableMenuItem(analyze_Stop, (thinking || monitoring) && !ExaChess);
  m->EnableMenuItem(analyze_Pause,
                    !isRated && !ExaChess && !posEditor && !game->GameOver());
  m->EnableMenuItem(analyze_Hint, !tx && !posEditor && !game->GameOver());
  m->EnableMenuItem(analyze_PlayMainLine,
                    !tx && !posEditor && !game->GameOver() &&
                        game->currMove == Analysis.gameMove);
  m->EnableMenuItem(analyze_DrawOffer,
                    thinking && !autoPlaying && !ExaChess && !drawOffered);
  m->EnableMenuItem(analyze_Resign,
                    !tx && !posEditor && !monitoring && !game->GameOver());
  m->EnableMenuItem(analyze_AutoPlay,
                    !tx && !posEditor && !annEditor && CanAutoPlay());
  m->EnableMenuItem(analyze_DemoPlay,
                    !tx && !posEditor && !annEditor && CanDemoPlay());
  m->EnableMenuItem(analyze_AnalyzeGame,
                    !tx && !posEditor && !annEditor && !isRated);
  m->EnableMenuItem(analyze_AnalyzeCol,
                    !tx && !posEditor && !annEditor && colWin);
  m->EnableMenuItem(analyze_AnalyzeEPD, !tx);
  m->EnableMenuItem(analyze_EngineMatch,
                    !tx && !posEditor && !annEditor && !EngineMatch.gameWin);
  m->EnableMenuItem(analyze_TransTables, !em);
  m->EnableMenuItem(analyze_EndgameDB, !thinking);
} /* GameWindow::AdjustAnalyzeMenu */

void GameWindow::AdjustEngineMenu(void) {
  if (!IsFront() || !RunningOSX()) return;

  BOOL tx = (thinking || ExaChess);

  CMenu *m = sigmaApp->engineMenu;
  for (INT i = 0; i < Prefs.UCI.count;
       i++) {  // m->EnableMenuItem(engine_Sigma + i, ! tx && ! monitoring);
    m->CheckMenuItem(engine_Sigma + i, (i == uciEngineId));
  }
} /* GameWindow::AdjustEngineMenu */

void GameWindow::AdjustLevelMenu(void) {
  if (!IsFront()) return;

  BOOL em = (EngineMatch.gameWin == this);

  SigmaMenu *m = sigmaApp->levelMenu;
  m->EnableMenuItem(level_Select, !em && !ExaChess);
  m->EnableMenuItem(level_PlayingStyle, !em && !UsingUCIEngine());
  m->EnableMenuItem(level_PermanentBrain, !em);
  m->EnableMenuItem(level_NonDeterm, !em && !UsingUCIEngine());
  m->EnableMenuItem(level_SigmaELO, !em);
  m->EnableMenuItem(level_PlayerELO, !em);
  m->EnableMenuItem(level_ELOCalc, true);

  m->SetIcon(level_Select, ModeIcon[level.mode], true);
  m->CheckMenuItem(level_PermanentBrain, permanentBrain);

  CHAR str[100];
  Format(
      str, "%s Rating",
      (!UsingUCIEngine() ? "Sigma Chess" : Prefs.UCI.Engine[uciEngineId].name));
  m->SetItemText(level_SigmaELO, str);
  //   m->SetIcon(level_SigmaELO, (! UsingUCIEngine() ? icon_SigmaChess :
  //   icon_Engine));
} /* GameWindow::AdjustLevelMenu */

void GameWindow::AdjustDisplayMenu(void) {
  if (!IsFront()) return;

  CMenu *m = sigmaApp->displayMenu;
  m->EnableMenuItem(display_TurnBoard, true);
  m->EnableMenuItem(display_PieceSet, !mode3D);
  m->EnableMenuItem(display_BoardType, !mode3D);
  m->EnableMenuItem(display_BoardSize, true);
  m->EnableMenuItem(display_Notation, true);
  m->EnableMenuItem(display_PieceLetters, true);
  m->EnableMenuItem(display_ToggleInfoArea,
                    !posEditor && !annEditor && !libEditor);
  m->EnableMenuItem(display_GameRecord, !mode3D);
  m->EnableMenuItem(display_3DBoard, board3Denabled && !libEditor);
  m->EnableMenuItem(display_Show3DClock, mode3D && !posEditor);
  m->EnableMenuItem(display_ColorScheme, true);
  m->EnableMenuItem(display_ToolbarTop, !mode3D);

  // Set window specific checkmarks:
  m->CheckMenuItem(display_TurnBoard, boardTurned);
  m->CheckMenuItem(display_3DBoard, mode3D);
  m->CheckMenuItem(display_ToolbarTop, toolbarTop);

  m = sigmaApp->boardSizeMenu;
  m->CheckMenuItem(boardSize_Standard, squareWidth == squareWidth1);
  m->CheckMenuItem(boardSize_Medium, squareWidth == squareWidth2);
  m->CheckMenuItem(boardSize_Large, squareWidth == squareWidth3);
  m->CheckMenuItem(boardSize_EvenLarger, squareWidth == squareWidth4);
} /* GameWindow::AdjustDisplayMenu */

void GameWindow::AdjustCollectionMenu(void) {
  if (!IsFront()) return;

  CMenu *m = sigmaApp->collectionMenu;
  m->EnableMenuItem(collection_EditFilter, false);
  m->EnableMenuItem(collection_EnableFilter, false);
  m->EnableMenuItem(collection_OpenGame, false);
  m->EnableMenuItem(collection_PrevGame,
                    colWin != nil && colWin->CanPrevGame());
  m->EnableMenuItem(collection_NextGame,
                    colWin != nil && colWin->CanNextGame());
  m->EnableMenuItem(collection_Layout, false);
  m->EnableMenuItem(collection_ImportPGN, false);
  m->EnableMenuItem(collection_ExportPGN, false);
  m->EnableMenuItem(collection_Compact, false);
  m->EnableMenuItem(collection_Renumber, false);
  m->EnableMenuItem(collection_Info, false);
} /* GameWindow::AdjustCollectionMenu */

void GameWindow::AdjustLibraryMenu(void) {
  if (!IsFront()) return;

  CMenu *m = sigmaApp->libraryMenu;
  m->EnableMenuItem(library_Name, true);
  m->EnableMenuItem(library_SigmaAccess, PosLib_Loaded());
  m->EnableMenuItem(library_Editor, PosLib_Loaded() && !mode3D);
  m->EnableMenuItem(library_ECOComment,
                    PosLib_Loaded() && libEditor && !mode3D);
  m->EnableMenuItem(library_DeleteVar, PosLib_Loaded() && libEditor && !mode3D);
  m->EnableMenuItem(library_ImportCollection, PosLib_Loaded() && !mode3D);
  m->EnableMenuItem(library_Save,
                    PosLib_Loaded() && !PosLib_Locked() && PosLib_Dirty());
  m->EnableMenuItem(library_SaveAs, PosLib_Loaded());

  m->CheckMenuItem(library_Editor, libEditor);
} /* GameWindow::AdjustLibraryMenu */

void GameWindow::AdjustToolbar(void) {
  toolbar->Adjust();
  miniToolbar->Adjust();
  infoAreaView->RefreshAnalysis();
} /* GameWindow::AdjustToolbar*/
