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

#include "CollectionWindow.h"
#include "EngineMatchDialog.h"
#include "ExportHTML.h"
#include "GameOverDialog.h"
#include "GamePrint.h"
#include "GameWindow.h"
#include "PGNFile.h"
#include "SigmaApplication.h"
#include "SigmaStrings.h"
#include "StrengthDialog.h"
#include "TransTabManager.h"
#include "UCI_Option.h"

#include "Engine.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                            SAVING */
/*                                                                                                */
/**************************************************************************************************/

BOOL GameWindow::SaveAs(void) {
  FlushAnnotation();

  // If no file object associated with window (i.e. the game is "untitled"),
  // then we first present the user with the standard file save dialog:
  CFile *newFile = new CFile();

  CHAR title[cwindow_MaxTitleLen + 1];
  GetTitle(title);

  INT format = gameFormat_Compressed;
  if (file) switch (file->fileType) {
      case '·GM5':
        format = gameFormat_Compressed;
        break;
      case '·GMX':
        format = gameFormat_Extended;
        break;
      case '·GAM':
      case 'XLGM':
      case 'CHGM':
        format = gameFormat_Old;
        break;
      case 'TEXT':
        format = gameFormat_PGN;
        break;
      default:
        sigmaApp->InternalError("Invalid Game Format");
        return false;
    }

  FILE_FORMAT FormatTab[5];
  for (INT i = 0; i < 5; i++) {
    FormatTab[i].id = i + 1;
    CopyStr(GetStr(sgr_FileSaveMenu, i), FormatTab[i].text);
  }

  INT fileFormatCount = ((RunningOSX() || theApp->OSVersion() >= 0x0900) &&
                                 !Prefs.Games.saveNative
                             ? 5
                             : 0);

  if (newFile->SaveDialog("Save Game", title, format, fileFormatCount,
                          FormatTab)) {
    if (sigmaApp->WindowTitleUsed(newFile->name)) {
      CHAR prompt[200];
      Format(prompt,
             "Another document with the name Ò%sÓ is already open. It is not "
             "possible to open two documents with the same name...",
             newFile->name);
      NoteDialog(this, "Document Already Open", prompt);
      delete newFile;
      return false;
    }

    // File-Save dialog OK -> Replace old CFile object (if any) with the new one
    if (file) delete file;
    file = newFile;
    if (file->saveReplace) file->Delete();

    if (fileFormatCount == 0) file->fileFormatItem = gameFormat_Compressed;

    if (SameStr(&file->name[StrLen(file->name) - 4], ".pgn"))
      file->fileFormatItem = gameFormat_PGN;

    file->SetCreator(sigmaCreator);
    switch (file->fileFormatItem) {
      case gameFormat_Compressed:
        file->SetType('·GM5');
        break;
      case gameFormat_Extended:
        file->SetType('·GMX');
        break;
      case gameFormat_Old:
        file->SetType('·GAM');
        break;
      case gameFormat_PGN:
        file->SetType('TEXT');
        ForcePGNExtension(file);
        break;
      default:
        return sigmaApp->InternalError("Invalid Game Format");
    }

    if (colWin) {
      colWin->DetachGameWin(this);
      Detach();
      file = newFile;  // <-- Important because "Detach()" set "file" = nil!
    }
    file->Create();
    if (!Save())
      return false;
    else {
      file->CompleteSave();
      return true;
    }
  } else {
    delete newFile;
    return false;
  }
} /* GameWindow::SaveAs */

BOOL GameWindow::Save(void)

// Saves the game using the specified file type. The game must already have been
// associated with a file object. If, however, the game is currently attached to
// a collection it is saved in the collection instead.

{
  FlushAnnotation();

  if (colWin)
    colWin->SaveGame(colGameNo, game);
  else if (!file)
    return SaveAs();
  else {
    ULONG size;
    hasFile = true;

    switch (file->fileType) {
      case '·GM5':
        size = game->Compress(gameData);
        break;
      case '·GMX':
        size = game->Write_V34(gameData);
        break;
      case 'TEXT':
        size = game->Write_PGN((CHAR *)gameData);
        break;
      default:
        size = game->Write_V2(gameData);
    }

    file->Save(size, gameData);
    file->CompleteSave();

    SetTitle(file->name);
    sigmaApp->RebuildWindowMenu();
  }

  game->dirty = false;
  AdjustFileMenu();
  AdjustToolbar();

  return true;
} /* GameWindow::Save */

/*------------------------------------------- Dirty Saving
 * ---------------------------------------*/

BOOL GameWindow::CheckSave(CHAR *prompt) {
  if (!game->dirty || !Prefs.Games.askGameSave) return true;

  // Compute title and prompt from game name:
  CHAR gameName[100], dlgTitle[100], message[500];
  GetTitle(gameName);
  if (!RunningOSX())
    Format(dlgTitle, "Save Ò%sÓ?", gameName);
  else
    CopyStr(prompt, dlgTitle);
  Format(message, "Changes to the game Ò%sÓ have not been saved. %s", gameName,
         prompt);

  CRect frame(0, 0, 320, 100);
  if (RunningOSX()) frame.right += 20, frame.bottom += 15;
  CentralizeRect(&frame);
  CConfirmDialog *dialog = new CConfirmDialog(
      this, dlgTitle, frame, message, 1007, "Save", "Cancel", "Don't Save");
  dialog->Run();
  CDIALOG_REPLY reply = dialog->reply;
  delete dialog;

  switch (reply) {
    case cdialogReply_OK:
      return Save();
    case cdialogReply_No:
      return true;
    default:
      return false;
  }
} /* GameWindow::CheckSave */

BOOL GameWindow::IsLocked(void) {
  return (file && file->IsLocked()) || (colWin && colWin->IsLocked());
} /* GameWindow::IsLocked */

/**************************************************************************************************/
/*                                                                                                */
/*                                        PERFORMING PLAYER MOVES */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- Performing Player Moves
 * --------------------------------*/
// This method should be called whenever the player has performed a move:

void GameWindow::PlayerMovePerformed(BOOL drawMove) {
  if (drawMove)
    boardAreaView->DrawMove(true);
  else
    sigmaApp->PlayMoveSound(game->Record[game->currMove].cap != empty);

  AdjustFileMenu();
  GameMoveAdjust(false);
  CheckClockAllocation();

  BOOL wasRated = isRated;
  if (game->UpdateInfoResult()) SetGameResult();

  if (libEditor)
    infoAreaView->libEditorView->CheckAutoAdd();
  else if (game->GameOver())
    GameOverDialog(this, true, wasRated);
  else
    Analyze_Reply();
} /* GameWindow::PlayerMovePerformed */

/*------------------------------------------- Chess Clocks
 * ---------------------------------------*/
// The following methods control the two chess clocks (and refer implicitly to
// the chess clock of the player to move).

void GameWindow::ResetClocks(void) {
  ResetClock(white);
  ResetClock(black);
} /* GameWindow::ResetClocks */

void GameWindow::ResetClock(
    COLOUR colour)  // Resets the clock of the specified player
{
  Clock[colour]->Reset(Level_CalcTotalTime(&level, colour));
  boardAreaView->DrawLevelInfo(colour, true);
  boardAreaView->DrawClockTime(colour);
} /* GameWindow::ResetClock */

void GameWindow::StartClock(
    void)  // Starts the clock of the player to move (and stops the clock
{          // of the opponent if it's running).
  Clock[game->opponent]->Stop();
  Clock[game->player]->Start();
} /* GameWindow::StartClock */

void GameWindow::StopClock(void)  // Stops the clock of the player to move
{
  Clock[game->player]->Stop();
} /* GameWindow::StopClock */

void GameWindow::CheckClockAllocation(void)

// Checks if extra time should be added to the current player's clock (i.e. if
// time control reached or Fischer clock enabled). Must be called immediately
// after a new move has been played on the board (by either the user or the
// engine).

{
  LONG extraTime =
      Level_CheckTimeControl(&level, game->opponent, game->MovesPlayed());
  if (extraTime == 0) return;

  Clock[game->opponent]->maxSecs += extraTime;
  Clock[game->opponent]->RecalcState();
  boardAreaView->DrawClockTime(game->opponent);
} /* GameWindow::CheckClockAllocation */

BOOL GameWindow::TickClock(void) {
  // Return immediately if clock hasn't changed:
  if (!Clock[game->player]->Tick()) return false;

  // Otherwise redraw player's clock:
  boardAreaView->DrawClockTime(game->player);

  // Return if no time forfeit (or solver mode):
  if (Clock[game->player]->timeOut && level.mode != pmode_Solver) TimeForfeit();

  return true;
} /* GameWindow::TickClock */

void GameWindow::TimeForfeit(void) {
  promoting = false;

  FlushPortBuffer();  // <-- So clock shows 00:00:00 BEFORE game over dialog is
                      // opened

  // If auto/demo playing or ExaChess we automatically reset the clocks and
  // continue;
  if (autoPlaying || ExaChess || timeoutContinued) {
    ResetClock(game->player);
    if (EngineMatch.gameWin != this)
      StartClock();
    else
      EngineMatch.timeForfeit = true;
    return;
  }

  // Otherwise update game result:
  BOOL wasRated = isRated;
  SetGameResult(
      result_TimeForfeit,
      (game->player == white ? infoResult_BlackWin : infoResult_WhiteWin));

  // Show game over dialog and reset chess clocks:
  GameOverDialog(this, thinking, wasRated);

  // Although the game is "officially" lost, we still give the user the option
  // of continuing the game with new time budgets (when autoPlaying/demoPlaying
  // or ExaChess we continue automatically):

  if (!QuestionDialog(this, "Continue Game?",
                      "Do you wish to continue the game anyway?", "Stop",
                      "Continue")) {
    SetGameResult(result_Unknown, infoResult_Unknown);
    ResetClock(game->player);
    StartClock();
    timeoutContinued = true;
  } else {
    StopClock();

    // If the user does NOT wish to continue and the engine is currently running
    // we have to stop it (gracefully), but WITHOUT performing any engine moves.
    if (Engine_TaskRunning(engine)) {
      backgrounding = false;
      UCI_Engine_Stop(uciEngineId);
      engine->R.aborted =
          true;  // <-- Make sure engine doesn't play move in this case
    }
  }
} /* GameWindow::TimeForfeit */

/*---------------------------------------- Set Game Result
 * ---------------------------------------*/

void GameWindow::PlayerResigns(void) {
  StopClock();

  BOOL wasRated = isRated;
  SetGameResult(result_Resigned, (game->player == white ? infoResult_BlackWin
                                                        : infoResult_WhiteWin));
  GameOverDialog(this, false, wasRated);

  CheckAbortEngine();
} /* GameWindow::PlayerResigns */

/*---------------------------------------- Set Game Result
 * ---------------------------------------*/
// Is called when a game is over: If either site is mate or a draw occurs, or if
// either side resigns or both players agree on a draw. If the game is currently
// being rated, we need to update the rating stats.

void GameWindow::SetGameResult(INT result, INT infoResult) {
  if (result >= 0) game->result = result;
  if (infoResult >= 0) game->Info.result = infoResult;
  infoAreaView->RefreshGameInfo();
  AdjustAnalyzeMenu();
  AdjustToolbar();

  if (!isRated || game->result == result_Unknown ||
      game->Info.result == infoResult_Unknown)
    return;

  isRated = false;
  AdjustGameMenu();
  miniToolbar->Adjust();

  REAL score = 0.0;
  BOOL sigmaWhite = EqualStr(game->Info.whiteName, engineName);

  if (game->Info.result == infoResult_Draw)
    score = 0.5;
  else if ((game->Info.result == infoResult_WhiteWin) == sigmaWhite)
    score = 0.0;
  else
    score = 1.0;

  UpdatePlayerRating(&(Prefs.PlayerELO), !sigmaWhite, score,
                     engineRating.engineELO);
} /* GameWindow::SetGameResult */

/**************************************************************************************************/
/*                                                                                                */
/*                                          SIGMA/UCI ENGINES */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------- Select Engine
 * --------------------------------------*/

void GameWindow::SelectEngine(UCI_ENGINE_ID newEngineId) {
  UCI_INFO *UCIInfo = &Prefs.UCI.Engine[newEngineId];

  if (newEngineId == uciEngineId)  // Ignore if no changes (unless forced)
  {
    CopyStr(UCIInfo->name,
            engineName);  // NOTE: The engine name may have changed (e.g.
    infoAreaView->RefreshAnalysis();  // if Hiarcs has been upgraded), so we
                                      // refresh that
    return;
  }

  if (!AbandonRatedGame()) return;  // Check if rated game should be abandoned

  //   UCI_INFO *UCIInfo = &Prefs.UCI.Engine[newEngineId];

  // Stop current engine if necessary
  CheckAbortEngine();

  // Replace player name of previous engine
  BOOL replaceWhiteName = EqualStr(engineName, game->Info.whiteName);
  BOOL replaceBlackName = EqualStr(engineName, game->Info.blackName);

  // Change engine id
  uciEngineId = newEngineId;
  engine->UCI = (uciEngineId != uci_SigmaEngineId);
  CopyStr(UCIInfo->name, engineName);

  // Optionally replace player engine name
  if (EngineMatch.gameWin != this) {
    if (replaceWhiteName) CopyStr(engineName, game->Info.whiteName);
    if (replaceBlackName) CopyStr(engineName, game->Info.blackName);
    if (replaceWhiteName || replaceBlackName) RefreshGameInfo();
  }

  // Update fixed options
  permanentBrain =
      (UCIInfo->supportsPonder ? UCIInfo->Ponder.u.Check.val : false);
  engineRating.reduceStrength =
      (UCIInfo->supportsLimitStrength ? UCIInfo->LimitStrength.u.Check.val
                                      : false);
  engineRating.engineELO =
      (UCIInfo->supportsLimitStrength ? UCIInfo->UCI_Elo.u.Spin.val : 2000);
  engineRating.autoReduce =
      (UCIInfo->supportsLimitStrength ? UCIInfo->autoReduce : false);
  multiPVoptionId = UCI_GetMultiPVOptionId(uciEngineId);

  // Show engine name in analysis view and refresh analysis toolbar
  infoAreaView->ResetAnalysis();
  infoAreaView->RefreshAnalysis();

  // Finally adjust menu
  HandleMenuAdjust();
  AdjustToolbar();

  // Check if we should release or create hash transposition tables.
  TransTab_AutoInit();
} /* GameWindow::SelectEngine */

/*---------------------------------------------- Misc
 * --------------------------------------------*/

BOOL GameWindow::UsingUCIEngine(void) {
  return (uciEngineId != uci_SigmaEngineId);
} /* GameWindow::UsingUCIEngine */

BOOL GameWindow::EngineSupportsRating(CHAR *title) {
  if (!UsingUCIEngine() || UCI_SupportsStrengthOption(uciEngineId)) return true;

  CHAR msg[200];
  Format(msg, "The %s engine does not support configuration of ELO rating...",
         engineName);
  NoteDialog(this, title, msg);
  return false;
} /* GameWindow::EngineSupportsRating */

/*-------------------------------------------- Multi PV
 * ------------------------------------------*/

BOOL GameWindow::SupportsMultiPV(void) {
  return multiPVoptionId != uci_NullOptionId;
}  // GameWindow::SupportsMultiPV

INT GameWindow::GetMaxMultiPVcount(void) {
  if (multiPVoptionId == uci_NullOptionId || EngineMatch.gameWin == this)
    return 1;

  UCI_INFO *UCIInfo = &Prefs.UCI.Engine[uciEngineId];
  return UCIInfo->Options[multiPVoptionId].u.Spin.max;
}  // GameWindow::GetMaxMultiPVcount

INT GameWindow::GetMultiPVcount(void) {
  if (multiPVoptionId == uci_NullOptionId || EngineMatch.gameWin == this)
    return 1;

  UCI_INFO *UCIInfo = &Prefs.UCI.Engine[uciEngineId];
  return UCIInfo->Options[multiPVoptionId].u.Spin.val;
}  // GameWindow::GetMultiPVcount

void GameWindow::SetMultiPVcount(INT count) {
  if (multiPVoptionId == uci_NullOptionId || EngineMatch.gameWin == this)
    return;

  UCI_INFO *UCIInfo = &Prefs.UCI.Engine[uciEngineId];
  UCI_OPTION *Option = &UCIInfo->Options[multiPVoptionId];

  if (count >= 1 && count <= Option->u.Spin.max &&
      count != Option->u.Spin.val) {
    Option->u.Spin.val = count;
    UCI_SendOption(uciEngineId, Option);
    if (Option->u.Spin.val > 2) varDisplayVer = false;
    infoAreaView->RefreshAnalysis();
  }
}  // GameWindow::SetMultiPVcount

void GameWindow::IncMultiPVcount(void) {
  if (multiPVoptionId == uci_NullOptionId || EngineMatch.gameWin == this)
    return;

  if (!MultiPVAllowed()) {
    NoteDialog(this, "Multi PV not Available",
               "Multi PV is not available in the current playing mode. Choose "
               "'Monitor', 'Infinite' or 'Manual' playing mode instead...");
    return;
  }

  UCI_INFO *UCIInfo = &Prefs.UCI.Engine[uciEngineId];
  UCI_OPTION *Option = &UCIInfo->Options[multiPVoptionId];

  if (Option->u.Spin.val < Option->u.Spin.max) {
    if (thinking && level.mode == pmode_Monitor) Analyze_Stop();

    Option->u.Spin.val++;
    UCI_SendOption(uciEngineId, Option);
    if (Option->u.Spin.val > 2) varDisplayVer = false;
    infoAreaView->RefreshAnalysis();

    CheckMonitorMode();
  }
}  // GameWindow::IncMultiPVcount

void GameWindow::DecMultiPVcount(void) {
  if (multiPVoptionId == uci_NullOptionId || EngineMatch.gameWin == this)
    return;

  UCI_INFO *UCIInfo = &Prefs.UCI.Engine[uciEngineId];
  UCI_OPTION *Option = &UCIInfo->Options[multiPVoptionId];

  if (Option->u.Spin.val > 1) {
    if (thinking && level.mode == pmode_Monitor) Analyze_Stop();

    Option->u.Spin.val--;
    UCI_SendOption(uciEngineId, Option);
    infoAreaView->RefreshAnalysis();

    CheckMonitorMode();
  }
}  // GameWindow::DecMultiPVcount

BOOL GameWindow::MultiPVAllowed(void) {
  return (level.mode == pmode_Monitor || level.mode == pmode_Infinite ||
          level.mode == pmode_Solver || level.mode == pmode_Manual);
}  // GameWindow::MultiPVAllowed

/**************************************************************************************************/
/*                                                                                                */
/*                                               MISC */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------------- Rate Game
 * -----------------------------------------*/

void GameWindow::RateGame(void) {
  FlushAnnotation();

  if (!engineRating.reduceStrength) {
    CHAR msg[300];
    Format(msg,
           "You must first specify the playing strength of %s in order to play "
           "a rated game...",
           engineName);
    NoteDialog(this, "Play Rated Game", msg);
    HandleMessage(level_SigmaELO);
    if (!engineRating.reduceStrength) return;
  }

  if (level.mode != pmode_TimeMoves && level.mode != pmode_Tournament) {
    NoteDialog(this, "Play Rated Game",
               "You must select the ÒNormalÓ or the ÒTournamentÓ playing mode "
               "in order to play a rated game...");
    HandleMessage(level_Select);
    if (level.mode != pmode_TimeMoves && level.mode != pmode_Tournament) return;
  }

  if (!CheckSave("Save before starting new rated game?")) return;

  COLOUR initPlayer;
  if (!RateGameDialog(this, &initPlayer)) return;

  game->dirty = false;
  HandleMessage(game_ResetGame, game_RateGame);

  INT humanELO = Prefs.PlayerELO.currELO;
  game->Info.whiteELO =
      (initPlayer == white ? humanELO : engineRating.engineELO);
  game->Info.blackELO =
      (initPlayer == black ? humanELO : engineRating.engineELO);
  boardAreaView->DrawPlayerInfo();

  isRated = true;
  AdjustGameMenu();
  miniToolbar->Adjust();

  if (initPlayer == (boardTurned ? white : black)) TurnBoard();

  if (initPlayer == white)
    StartClock();
  else
    Analyze_Go();
} /* GameWindow::RateGame */

// If the game is currently being rated this routine should be called if the
// user tries to abandon or interrupt the game.

BOOL GameWindow::AbandonRatedGame(void) {
  if (!isRated) return true;
  if (QuestionDialog(this, "Abandon Rated Game?",
                     "This will interrupt the game and your ELO rating will be "
                     "adjusted as if you LOST the game...",
                     "Cancel", "OK"))
    return false;

  isRated = false;
  AdjustGameMenu();
  miniToolbar->Adjust();

  BOOL sigmaWhite = EqualStr(game->Info.whiteName, engineName);
  UpdatePlayerRating(&(Prefs.PlayerELO), !sigmaWhite, 0.0,
                     engineRating.engineELO);

  return true;
} /* GameWindow::AbandonRatedGame */

/*------------------------------------------ Replay Game
 * -----------------------------------------*/
// NOTE: Works best if each game window runs in separate thread.

void GameWindow::ReplayGame(
    void)  // Like auto play! But engine simply replays moves??
{
  while (game->CanRedoMove()) {
    HandleMessage(game_RedoMove);
    theApp->ProcessEvents();
  }
  /*
     case game_RedoMove :
        if (! game->CanRedoMove() || thinking || ExaChess) return;
  */
} /* GameWindow::ReplayGame */

/*----------------------------------------- Play Main Line
 * ---------------------------------------*/

void GameWindow::PlayMainLine(void) {
  if (thinking || ExaChess || posEditor || game->GameOver()) return;
  if (game->currMove != Analysis.gameMove) return;

  CheckAbortEngine();
  StopClock();
  FlushAnnotation();

  for (INT i = 0; !isNull(Analysis.PV[1][i]); i++) {
    boardAreaView->ClearMoveMarker();
    game->PlayMove(&Analysis.PV[1][i]);
    boardAreaView->DrawMove(true);
    GameMoveAdjust(false);
    CheckClockAllocation();
  }
} /* GameWindow::PlayMainLine */

/*---------------------------------------- Add To Collection
 * -------------------------------------*/

void GameWindow::Attach(CollectionWindow *win, ULONG gameNo) {
  colWin = win;
  colGameNo = gameNo;
  hasFile = true;
  file = nil;
  game->dirty = false;
  miniToolbar->Adjust();
  AdjustGameMenu();
} /* GameWindow::Attach */

void GameWindow::Detach(void) {
  colWin = nil;
  colGameNo = 0;
  hasFile = false;
  file = nil;
  game->dirty = false;
  miniToolbar->Adjust();
  SetTitle("<Untitled Game>");
  AdjustGameMenu();
} /* GameWindow::Detach */

void GameWindow::AddToCollection(INT colWinNo) {
  if (colWin) return;

  CollectionWindow *theColWin = sigmaApp->GetColWindow(colWinNo);
  theColWin->AddGame(this);
  AdjustFileMenu();
  AdjustToolbar();
} /* GameWindow::AddToCollection */

/*----------------------------------------- HTML Export
 * ------------------------------------------*/

void GameWindow::ExportHTML(void) {
  HTMLGifReminder(this);

  CFile *htmlFile = new CFile();

  if (htmlFile->SaveDialog("Export HTML", ".html")) {
    if (htmlFile->saveReplace) htmlFile->Delete();

    htmlFile->SetCreator('ttxt');
    htmlFile->SetType('TEXT');
    htmlFile->Create();

    CHAR title[cwindow_MaxTitleLen + 1];
    GetTitle(title);
    CExportHTML *html = new CExportHTML(title, htmlFile);
    html->ExportGame(game);
    htmlFile->CompleteSave();
    delete html;
  }

  delete htmlFile;
} /* GameWindow::ExportHTML */

/*------------------------------------------ Printing
 * --------------------------------------------*/

void GameWindow::PrintGame(void) {
  CHAR title[cwindow_MaxTitleLen + 1];
  GetTitle(title);
  CGamePrint *gamePrint = new CGamePrint(title);
  gamePrint->PrintGame(game);
  delete gamePrint;
} /* GameWindow::PrintGame */

/*-------------------------------------------- Misc
 * ----------------------------------------------*/

void GameWindow::DrawBoard(BOOL drawFrame) {
  boardAreaView->DrawAllSquares();
  if (drawFrame) boardAreaView->DrawBoardFrame();
  boardAreaView->DrawPlayerInfo();
  boardAreaView->DrawClockInfo();
} /* GameWindow::DrawBoard */

BOOL GameWindow::LegalPosition(void) {
  CHAR s[100], message[300];

  switch (game->Edit_CheckLegalPosition()) {
    case pos_Legal:
      return true;
    case pos_TooManyWhitePawns:
      CopyStr("there are too many white pawns", s);
      break;
    case pos_TooManyBlackPawns:
      CopyStr("there are too many black pawns", s);
      break;
    case pos_WhiteKingMissing:
      CopyStr("there is no white king", s);
      break;
    case pos_BlackKingMissing:
      CopyStr("there is no black king", s);
      break;
    case pos_TooManyWhiteKings:
      CopyStr("there is more than one white king", s);
      break;
    case pos_TooManyBlackKings:
      CopyStr("there is more than one black king", s);
      break;
    case pos_TooManyWhiteOfficers:
      CopyStr("there are too many white pieces", s);
      break;
    case pos_TooManyBlackOfficers:
      CopyStr("there are too many black pieces", s);
      break;
    case pos_PawnsOn1stRank:
      CopyStr("pawns are not allowed on the 1st and 8th rank", s);
      break;
    case pos_OpponentInCheck:
      CopyStr("the opponent king is in check", s);
      break;
  }

  Format(message,
         "The current position is not legal (%s). You must either correct the "
         "position or cancel the Position Editor.",
         s);
  NoteDialog(this, "Illegal Position", message);
  return false;
} /* GameWindow::LegalPosition */

void GameWindow::GotoMove(INT j, BOOL openAnnEditor) {
  if (thinking || j == game->currMove || j < 0) return;

  CheckAbortEngine();

  StopClock();
  FlushAnnotation();

  boardAreaView->ClearMoveMarker();
  while (game->currMove > j) game->UndoMove(false);
  while (game->currMove < j) game->RedoMove(false);

  game->CalcMoves();
  GameMoveAdjust(true);

  if (!annEditor && openAnnEditor) HandleMessage(game_AnnotationEditor);

  CheckMonitorMode();
} /* GameWindow::GotoMove */

void GameWindow::RefreshGameInfo(void) {
  boardAreaView->DrawPlayerInfo();
  infoAreaView->RefreshGameInfo();
} /* GameWindow::RefreshGameInfo */

void GameWindow::SetAnnotation(CHAR *s) {
  game->SetAnnotation(game->currMove, s, StrLen(s));
}  // GameWindow::SetAnnotation

void GameWindow::FlushAnnotation(void) {
  infoAreaView->FlushAnnotation();
} /* GameWindow::FlushAnnotation */

BOOL GameWindow::CheckEngineMatch(void) {
  if (!EngineMatch.gameWin || !UsingUCIEngine()) return false;
  NoteDialog(this, "Engine Match",
             "An engine match is currently being played...");
  return true;
}  // GameWindow::CheckEngineMatch

/**************************************************************************************************/
/*                                                                                                */
/*                                          TOGGLE 3D BOARD */
/*                                                                                                */
/**************************************************************************************************/

void GameWindow::Toggle3D(void) {
  Show(false);

  mode3D = !mode3D;

  if (mode3D) {
    if (annEditor) HandleMessage(game_AnnotationEditor);

    boardArea2DView->Show(false);
    infoAreaView->Show(false);
    toolbar->Show(false);
    miniToolbar->Show(false);
    tabAreaView->Show(false);

    boardArea3DView = new BoardArea3DView(this, theApp->ScreenRect());
    boardAreaView = boardArea3DView;

    frame2D = Frame();  // Save frame before moving
    Move(0, 0, false);
    Resize(theApp->ScreenRect().Width(), theApp->ScreenRect().Height());

    Show(true);
  } else {
    delete boardArea3DView;
    boardArea3DView = nil;
    boardAreaView = boardArea2DView;

    boardArea2DView->Show(true);
    infoAreaView->Show(true);
    toolbar->Show(true);
    miniToolbar->Show(true);
    tabAreaView->Show(true);

    Move(frame2D.left, frame2D.top, false);
    Resize((showInfoArea ? GameWin_Width(squareWidth)
                         : BoardArea_Width(squareWidth)),
           GameWin_Height(squareWidth));
  }

  Show(true);

  if (!IsFront()) SetFront();
  AdjustToolbar();
  HandleMenuAdjust();
} /* GameWindow::Toggle3D */

/**************************************************************************************************/
/*                                                                                                */
/*                                        COPY ANALYSIS */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------- Copy Analysis to Clipboard
 * -----------------------------------*/

void GameWindow::CopyAnalysis(void) {
  CHAR Text[1000];
  INT len = BuildAnalysisString(&Analysis, Text, false, 0, GetMultiPVcount());

  sigmaApp->ResetClipboard();
  sigmaApp->WriteClipboard('TEXT', (PTR)Text, len);
} /* GameWindow::CopyAnalysis */

/*--------------------------------- Compute Analysis String
 * ----------------------------------*/

static void WriteScoreStr(ANALYSIS_STATE *Analysis, BOOL includeAltScore,
                          INT altScore, INT pvNo = 1);
static void WriteDepthStr(ANALYSIS_STATE *Analysis);
static void WriteNodesStr(ANALYSIS_STATE *Analysis);
static void WriteMainLineStr(ANALYSIS_STATE *Analysis, INT pvNo = 1,
                             BOOL ExaChess = false);
static void WriteStr(CHAR *s);
static void WriteNum(LONG n);
static void WriteSeparator(void);

static CHAR *searchResStr;

INT BuildAnalysisString(ANALYSIS_STATE *Analysis, CHAR *Text,
                        BOOL includeAltScore, INT altScore, INT pvCount) {
  searchResStr = Text;

  if (pvCount <= 1) {
    WriteScoreStr(Analysis, includeAltScore, altScore);
    WriteDepthStr(Analysis);
    WriteNodesStr(Analysis);
    WriteMainLineStr(Analysis);
  } else {
    WriteDepthStr(Analysis);
    WriteNodesStr(Analysis);

    for (INT pvNo = 1; pvNo <= pvCount; pvNo++) {
      WriteStr("\n");
      WriteScoreStr(Analysis, includeAltScore, altScore, pvNo);
      WriteMainLineStr(Analysis, pvNo);
    }
  }

  *searchResStr = 0;
  return (searchResStr - Text);  // Return length (excluding trailing null)
} /* BuildAnalysisString */

void BuildExaChessResult(ANALYSIS_STATE *Analysis, CHAR *Text) {
  searchResStr = Text;
  WriteMainLineStr(Analysis, true);
  *searchResStr = 0;
} /* BuildExaChessResult */

static void WriteScoreStr(ANALYSIS_STATE *Analysis, BOOL includeAltScore,
                          INT altScore, INT pvNo) {
  if (Prefs.AnalysisFormat.showScore) {
    INT scoreType =
        (Analysis->scoreType[pvNo] == scoreType_Book ? scoreType_Book
                                                     : scoreType_True);
    CHAR scoreStr[20];

    if (!Prefs.AnalysisFormat.shortFormat) WriteStr("Score : ");
    if (includeAltScore) {
      WriteStr("(");
      ::CalcScoreStr(scoreStr, CheckAbsScore(Analysis->player, altScore),
                     scoreType_True);
      WriteStr(scoreStr);
      WriteStr(") ");
    }
    ::CalcScoreStr(scoreStr,
                   CheckAbsScore(Analysis->player, Analysis->score[pvNo]),
                   scoreType);
    WriteStr(scoreStr);
    WriteSeparator();
  }
} /* WriteScoreStr */

static void WriteDepthStr(ANALYSIS_STATE *Analysis) {
  if (Prefs.AnalysisFormat.showDepth) {
    if (!Prefs.AnalysisFormat.shortFormat) WriteStr("Depth : ");
    WriteNum(Analysis->depth);
    WriteStr("/");
    WriteNum(Analysis->current);
    WriteSeparator();
  }
} /* WriteDepthStr */

static void WriteNodesStr(ANALYSIS_STATE *Analysis) {
  LONG time = Analysis->searchTime;

  if (Prefs.AnalysisFormat.showTime) {
    if (!Prefs.AnalysisFormat.shortFormat) WriteStr("Time  : ");
    CHAR clockStr[9];
    FormatClockTime(time / 60, clockStr);
    WriteStr(clockStr);

    WriteSeparator();
  }

  if (Prefs.AnalysisFormat.showNodes) {
    if (!Prefs.AnalysisFormat.shortFormat) WriteStr("Nodes : ");
    if (Analysis->nodes < 1000000000.0)
      WriteNum(Analysis->nodes);
    else {
      WriteNum(Analysis->nodes / 1000.0);
      WriteStr("K");
    }
    WriteSeparator();
  }

  if (Prefs.AnalysisFormat.showNSec) {
    if (!Prefs.AnalysisFormat.shortFormat) WriteStr("N/sec : ");
    WriteNum((60.0 * Analysis->nodes) / MaxL(1, Analysis->searchTime));
    WriteSeparator();
  }
} /* WriteNodesStr */

static void WriteMainLineStr(ANALYSIS_STATE *Analysis, INT pvNo,
                             BOOL ExaChess) {
  INT moveNo;
  MOVE *M = Analysis->PV[pvNo];

  if (isNull(M[0]) || !Prefs.AnalysisFormat.showMainLine) return;

  moveNo = Analysis->initMoveNo +
           (Analysis->gameMove + (Analysis->initPlayer >> 4)) / 2;
  WriteNum(moveNo++);
  WriteStr(".");
  if (pieceColour(M[0].piece) == black) WriteStr("..");
  //    WriteStr(" ...");

  for (INT i = 0; !isNull(M[i]); i++) {
    WriteStr(" ");
    if (i > 0 && pieceColour(M[i].piece) == white)
      WriteNum(moveNo++), WriteStr(". ");
    searchResStr +=
        CalcGameMoveStrAlge(&M[i], searchResStr, false, ExaChess, !ExaChess);

    // If building ExaChess reply string we need to include the score after
    // first move. NOTE: This score is always seen from white and must hence be
    // negated if black to move. Also, for book moves we simply return the
    // string "book"

    if (i == 0 && ExaChess) {
      CHAR scoreStr[20];
      INT exaScore = (Analysis->player == white ? Analysis->score[pvNo]
                                                : -Analysis->score[pvNo]);
      INT scoreType =
          (Analysis->scoreType[pvNo] == scoreType_Book ? scoreType_Book
                                                       : scoreType_True);

      WriteStr(" {");
      ::CalcScoreStr(scoreStr, exaScore, scoreType);
      WriteStr(scoreStr);
      WriteStr("}");
    }
  }
} /* WriteMainLineStr */

static void WriteNum(LONG n) {
  CHAR s[15];
  NumToStr(n, s);
  WriteStr(s);
} /* WriteNum */

static void WriteSeparator(void) {
  WriteStr(Prefs.AnalysisFormat.shortFormat ? " " : "\r");
} /* WriteSeparator */

static void WriteStr(CHAR *s) {
  while (*s) *(searchResStr++) = *(s++);
} /* WriteStr */

/**************************************************************************************************/
/*                                                                                                */
/*                                          TEST ROUTINES */
/*                                                                                                */
/**************************************************************************************************/

#ifdef __libTest_Verify

/*---------------------------------------- Verify Library
 * ----------------------------------------*/
// Traverses the library from the current position and evaluates/verifies all
// reachable positions with a search (e.g. 4 ply).

void GameWindow::VerifyPosLib(void) {
  INT g = game->currMove;
  if (userStopped || g >= 30 ||
      (g > 5 && game->DrawData[g].hashKey == game->DrawData[g - 4].hashKey))
    return;

  LIBVAR Var[libMaxVariations];
  INT varCount;

  varCount = PosLib_CalcVariations(game, Var);

  for (INT i = 0; i < varCount && !userStopped; i++) {
    // Perform next variation move:
    game->PlayMove(&Var[i].m);
    GameMoveAdjust(false);

    LIB_CLASS curClass = PosLib_Probe(game->player, game->Board);

    if (!(curClass & 0x10))  // If we have not visited this position before...
    {
      // Analyze position:
      Analyze_Go();
      while (thinking) sigmaApp->MainLooper();

      // Verify score against library classification:
      INT score = Analysis.score;  // Calc abs (seen from White) score
      if (game->player == black) score = -score;

      LIB_CLASS newClass = curClass;

      if (score < -100)
        newClass = libClass_Unclassified;
      else if (score < -50)
        newClass = libClass_ClearAdvB;
      else if (score < -25)
        newClass = libClass_SlightAdvB;
      else if (score > 100)
        newClass = libClass_Unclassified;
      else if (score > 50)
        newClass = libClass_ClearAdvW;
      else if (score > 25)
        newClass = libClass_SlightAdvW;

      if (curClass != newClass &&
          !(curClass == libClass_Unclear && newClass != libClass_Unclassified))
        curClass = newClass;
      if (curClass != libClass_Unclassified)
        curClass = (LIB_CLASS)(curClass | 0x10);

      // Mark as visited and optionally update classification
      PosLib_Classify(game->player, game->Board, curClass, true);

      // Traverse library recursively:
      VerifyPosLib();
    }

    // Retract most recent move
    game->UndoMove(true);
    GameMoveAdjust(false);
  }
} /* GameWindow::VerifyPosLib */

#endif
