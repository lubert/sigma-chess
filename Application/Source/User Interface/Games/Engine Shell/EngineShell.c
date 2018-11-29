/**************************************************************************************************/
/*                                                                                                */
/* Module  : ENGINESHELL.C */
/* Purpose : This module implements the "Engine Shell" for game windows, i.e.
 * the routines for    */
/*           starting/stopping the search. Additionally the engine shell
 * implements the "Engine"  */
/*           "Callback Message Handler", which responds to various engine
 * initiated events (e.g.  */
/*           new iteration started) which requires visual feedback. */
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

#include "CDialog.h"
#include "CollectionWindow.h"
#include "EngineMatchDialog.h"
#include "GameOverDialog.h"
#include "GameWindow.h"
#include "PosLibrary.h"
#include "SigmaApplication.h"
#include "TransTabManager.h"
#include "UCI.h"

#include "Board.f"
#include "Engine.f"
#include "Move.f"

//#include "Debug.h"  //###

/**************************************************************************************************/
/*                                                                                                */
/*                                            START SEARCH */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------------ Go
 * --------------------------------------------*/
// This method is called when the user invokes the "Go" command in the "Analyze"
// menu.

void GameWindow::Analyze_Go(BOOL nextBest) {
  if (thinking || backgrounding || monitoring || game->GameOver() ||
      !UCI_RequestLock(uciEngineId, this, true))
    return;

  if (level.mode == pmode_Monitor) {
    CheckMonitorMode();
  } else {
    CheckSwapPlayerNames();

    // Enter "thinking" mode
    thinking = true;
    AdjustGameMenu();
    AdjustAnalyzeMenu();
    AdjustToolbar();
    StartSearch(nextBest);
  }
} /* GameWindow::Analyze_Go */

/*---------------------------------------- Respond to User Move
 * ----------------------------------*/
// Is called when the user has performed a move (and NOT game over).

void GameWindow::Analyze_Reply(void) {
  if (!UCI_RequestLock(uciEngineId, this, true)) return;

  if (level.mode == pmode_Manual || level.mode == pmode_Infinite ||
      level.mode == pmode_Solver || EngineMatch.gameWin) {
    StartClock();
  } else if (level.mode == pmode_Monitor) {
    CheckMonitorMode();
  } else if (!backgrounding) {
    thinking = true;
    AdjustGameMenu();
    AdjustAnalyzeMenu();
    AdjustToolbar();
    StartSearch();
  } else {
    backgrounding = false;
    Engine_ClearBackgrounding(engine);

    if (EqualMove(&game->Record[game->currMove], &expectedMove))  // Ponder hit
    {
      thinking = true;
      AdjustGameMenu();
      AdjustAnalyzeMenu();
      AdjustToolbar();
      StartClock();
      infoAreaView->SetAnalysisStatus("Thinking...");
      UCI_SendPonderhit(uciEngineId);  // Ignored if Sigma engine
    } else  // User did NOT play the expected move -> abort search and restart
    {
      infoAreaView->SetAnalysisStatus("Stopping...", true);
      UCI_Engine_Abort(uciEngineId);
      Analyze_Go();
    }
  }
} /* GameWindow::Analyze_Reply */

/*---------------------------------------------- Utility
 * -----------------------------------------*/

void GameWindow::CheckSwapPlayerNames(void) {
  if (level.mode <= pmode_Novice && game->lastMove == 0 &&
      EqualStr(game->Info.whiteName, Prefs.General.playerName) &&
      EqualStr(game->Info.blackName, engineName)) {
    CopyStr(engineName, game->Info.whiteName);
    CopyStr(Prefs.General.playerName, game->Info.blackName);
    RefreshGameInfo();
  }
} /* GameWindow::CheckSwapPlayerNames */

void GameWindow::StartSearch(BOOL nextBest) {
  userStopped = false;
  drawOffered = false;

  if (EngineMatch.gameWin == this)  // Check if we need to swap engines
  {
    BOOL enginesSwapped =
        Prefs.EngineMatch.alternate && even(EngineMatch.currGameNo);
    BOOL useEngine1 = (even(game->currMove) == !enginesSwapped);

    UCI_ENGINE_ID newUciEngineId =
        (useEngine1 ? Prefs.EngineMatch.engine1 : Prefs.EngineMatch.engine2);

    if (uciEngineId != newUciEngineId) {
      uciEngineId = newUciEngineId;
      if (Prefs.EngineMatch.engine1 != uci_SigmaEngineId &&
          Prefs.EngineMatch.engine2 != uci_SigmaEngineId)
        UCI_SwapEngines();
    }

    engine->UCI = (uciEngineId != uci_SigmaEngineId);
    CopyStr(Prefs.UCI.Engine[uciEngineId].name, engineName);
  }

  infoAreaView->ResetAnalysis();
  if (analyzeGame)
    infoAreaView->SetAnalysisStatus("Analyzing game...");
  else if (autoPlaying)
    infoAreaView->SetAnalysisStatus(EngineMatch.gameWin == this
                                        ? "Playing Engine Match..."
                                        : "Auto playing...");
  else
    infoAreaView->SetAnalysisStatus("Thinking...");

  CopyTable(game->Board, BoardAnalyzed);
  moveAnalyzed = game->currMove;

  SetSearchParam(nextBest);
  if (engine->P.playingMode == mode_Infinite) ResetClocks();
  StartClock();

  UCI_Engine_Start(uciEngineId, engine, game,
                   !(autoPlaying && EngineMatch.gameWin == this));
} /* GameWindow::StartSearch */

/*--------------------------------------- Set Search Parameters
 * ----------------------------------*/

void GameWindow::SetSearchParam(BOOL nextBest) {
  PARAM *P = &engine->P;

  //--- Game state ---

  CopyTable(game->Board, P->Board);
  CopyTable(game->HasMovedTo, P->HasMovedTo);
  P->player = game->player;
  P->lastMove = game->Record[game->currMove];
  P->lastMoveNo = game->currMove;
  P->DrawData = game->DrawData;

  //--- Search/eval parameters ---

  P->pvSearch = (level.mode != pmode_MateFinder);
  P->alphaBetaWin = true;
  P->selection = (level.mode != pmode_MateFinder);
  P->deepSelection = true;
  P->backgrounding = backgrounding;
  P->useEndgameDB = Prefs.useEndgameDB;
  P->proVersion = ProVersion();
  P->nextBest = nextBest;

  //--- Mode/Level/Style parameters ---

  INT moveCount = (game->currMove + 1) / 2;
  INT moveLim = allMoves;  // Depends on mode.

  P->playingMode = mode_FixDepth;
  P->movesPlayed = moveCount;
  P->movesLeft = 10;
  P->timeLeft = 10;
  P->moveTime = 1;
  P->depth = 1;

  switch (level.mode) {
    case pmode_TimeMoves:
      P->playingMode = mode_Time;
      moveLim = level.TimeMoves.moves;
      P->movesPlayed = (moveLim == allMoves ? moveCount : moveCount % moveLim);
      P->movesLeft =
          (moveLim == allMoves ? moveLim : moveLim - P->movesPlayed + 1);
      P->timeLeft = Clock[P->player]->maxSecs - Clock[P->player]->elapsed;
      P->moveTime = level.TimeMoves.time / (moveLim == allMoves ? 60 : moveLim);
      P->timeInc = 0;
      if (level.TimeMoves.clockType == clock_Fischer) {
        P->timeInc = level.TimeMoves.delta;
        if (!UsingUCIEngine())
          P->timeLeft += level.TimeMoves.delta *
                         (moveLim == allMoves ? Max(1, 60 - P->movesPlayed)
                                              : P->movesLeft);
      }
      break;

    case pmode_Tournament:
      P->playingMode = mode_Time;
      P->movesPlayed = moveCount;
      INT i;
      for (i = 1; i < 3 && level.Tournament.moves[i - 1] <= P->movesPlayed; i++)
        P->movesPlayed -= level.Tournament.moves[i - 1];
      P->movesLeft =
          (i == 3 ? allMoves
                  : level.Tournament.moves[i - 1] - P->movesPlayed + 1);
      P->timeLeft = Clock[P->player]->maxSecs - Clock[P->player]->elapsed;
      P->moveTime = (P->player == white ? level.Tournament.wtime[0]
                                        : level.Tournament.btime[0]) /
                    level.Tournament.moves[0];
      break;

    case pmode_Average:
      P->playingMode = mode_Time;
      P->movesPlayed = moveCount;
      P->movesLeft = Max(60 - moveCount, 20);
      P->timeLeft = (P->movesPlayed + P->movesLeft) * level.Average.secs -
                    Clock[P->player]->elapsed;
      P->timeLeft = MaxL(P->movesLeft, P->timeLeft);
      P->moveTime = level.Average.secs;
      break;

    case pmode_Leisure:
      P->playingMode = mode_Time;
      P->movesPlayed = moveCount;
      P->movesLeft = Max(60 - moveCount, 20);
      P->moveTime =
          Max(1, (Clock[game->opponent]->elapsed + 5) / (moveCount + 1));
      P->timeLeft = (P->movesPlayed + P->movesLeft) * P->moveTime -
                    Clock[P->player]->elapsed;
      P->timeLeft = MaxL(P->movesLeft, P->timeLeft);
      break;

    case pmode_FixedDepth:
      P->playingMode = mode_FixDepth;
      P->depth = level.FixedDepth.depth;
      break;
    case pmode_Infinite:
      P->playingMode = mode_Infinite;
      break;
    case pmode_Monitor:
      P->playingMode = mode_Infinite;
      break;
    case pmode_Solver:
      P->playingMode = mode_Infinite;
      break;
    case pmode_MateFinder:
      P->playingMode = mode_Mate;
      P->depth = level.MateFinder.mateDepth;
      break;
    case pmode_Novice:
      P->playingMode = mode_Novice;
      P->depth = level.Novice.level;
      break;
    case pmode_Manual:
      P->playingMode = mode_Infinite;
      break;
  }

  P->playingStyle = Prefs.Level.playingStyle;
  P->permanentBrain = permanentBrain;
  P->reduceStrength =
      (engineRating.reduceStrength && P->playingMode != mode_Novice);
  P->engineELO = engineRating.engineELO;

  //--- Opening Library ---

  P->Library =
      (Prefs.Library.enabled && level.mode != pmode_Infinite ? PosLib_Data()
                                                             : nil);
  P->libSet =
      (Prefs.Library.enabled
           ? (EngineMatch.gameWin == this ? libSet_Solid : Prefs.Library.set)
           : libSet_None);

  //--- Transposition Tables ---

  TransTab_Allocate(engine);
} /* GameWindow::SetSearchParam */

/**************************************************************************************************/
/*                                                                                                */
/*                                            END SEARCH */
/*                                                                                                */
/**************************************************************************************************/

// Just before the engine task returns/completes it sends an "msg_EndSearch"
// message which in turn invokes the "GameWindow::SearchCompleted" routine. If
// the search wasn't aborted and it's not manual play, the best move found by
// the engine will be played.

void GameWindow::SearchCompleted(void) {
  // engine->R.taskRunning = false; // <-- Because it's otherwise set too late!!
  TransTab_Deallocate(engine);

  if (level.mode == pmode_Monitor) return;

  StopClock();

  AdjustTargetELO();

  if (analyzeGame)
    AnalyzeGame_SearchCompleted();
  else if (autoPlaying)
    AutoSearchCompleted();
  else if (thinking)  // "Normal" thinking completed (i.e. NOT backgrounding or
                      // monitor)
    NormalSearchCompleted();
} /* GameWindow::SearchCompleted */

void GameWindow::AdjustTargetELO(void) {
  if (drawOffered || game->GameOver()) return;

  if (!engine->P.reduceStrength || UsingUCIEngine()) return;
  if (engine->P.engineELO <= engine->P.actualEngineELO + 50 ||
      engine->P.engineELO <= 2000 || engine->S.bestScore > 300)
    return;

  if (!autoPlaying && !ExaChess)
    NoteDialog(this, "ELO Strength",
               "Unable to play at specified ELO strength. Your computer does "
               "unfortunately not seem to be fast enough...");

  if (engineRating.autoReduce) {
    engineRating.engineELO = 100 * ((engine->P.actualEngineELO + 50) / 100);
    miniToolbar->Adjust();
  }
} /* GameWindow::AdjustTargetELO */

/*------------------------------------ Normal Search Completion
 * ----------------------------------*/

void GameWindow::NormalSearchCompleted(void) {
  thinking = false;
  infoAreaView->SetAnalysisStatus("Idle");

#ifdef __libTest_Verify
  return;
#endif

  if (level.mode == pmode_MateFinder) {
    AdjustAnalyzeMenu();
    AdjustToolbar();

    INT n = level.MateFinder
                .mateDepth;  //(1 + maxVal - Engine_BestScore(engine))/2;
    CHAR s[100];

    if (Engine_Aborted(engine) || userStopped || UsingUCIEngine()) {
    } else if (!engine->S.mateFound) {
      Format(s, "There are no mate in %d move%s in the current position!", n,
             (n > 1 ? "s" : ""));
      NoteDialog(this, "Mate Finder", s, cdialogIcon_Error);

      //         if (UsingUCIEngine())
      //            NoteDialog(this, "Mate Finder & UCI", "Some UCI engines do
      //            NOT support the 'Mate Finder' mode. You can select the Sigma
      //            engine and try again...");
    } else if (engine->S.mateContinue) {
      NoteDialog(this, "Mate Finder", "No more solutions were found...",
                 cdialogIcon_Error);
    } else {
      PlayEngineMove();
      if (game->GameOver())
        GameOverDialog(this, false, false);
      else if (Engine_BestScore(engine) < maxVal - 1 &&
               level.MateFinder.mateDepth > 1) {
        level.MateFinder.mateDepth--;
        boardAreaView->DrawModeIcons();
        ResetClocks();
      }
    }
  } else if (level.mode == pmode_Manual || Engine_Aborted(engine)) {
    hintMove = Engine_BestMove(engine);
    AdjustAnalyzeMenu();
    AdjustToolbar();
  } else if (!AcceptDrawOffer() && !CheckResign()) {
    BOOL wasRated = isRated;
    PlayEngineMove();

    if (!game->GameOver()) {
      StartClock();
      CheckBackgrounding();
    } else if (!ExaChess && IsFront())
      GameOverDialog(this, false, wasRated);
  }

  UCI_ReleaseLock(uciEngineId,
                  this);  // Will be ignored if thinking/backgrounding
} /* GameWindow::NormalSearchCompleted */

void GameWindow::PlayEngineMove(void) {
  BOOL didCheckAppleEvents = theApp->checkAppleEvents;
  theApp->checkAppleEvents = false;

  // Before the engine plays its move, we first check for mate announcement
  // (only if front window):
  CheckAnnounceMate();

  // Then we play the actual move on the board...
  FlushAnnotation();
  boardAreaView->ClearMoveMarker();
  game->PlayMove(&Engine_BestMove(engine));
  // DebugWriteNL("Drawing move");  //###
  boardAreaView->DrawMove(true);
  // DebugWriteNL("Update game list");  //###
  GameMoveAdjust(false, true);
  // DebugWriteNL("Check clock alloc");  //###
  CheckClockAllocation();

  // ...and update hint moves and game result.
  hintMove = Engine_BestReply(engine);  // Must be done after game move adjust
  sigmaApp->analyzeMenu->EnableMenuItem(analyze_Hint, true);

  if (game->UpdateInfoResult()) {
    infoAreaView->RefreshGameInfo();
    SetGameResult();
  }

  theApp->checkAppleEvents = didCheckAppleEvents;
} /* GameWindow::PlayEngineMove */

/*--------------------------- Mate Announcements/Resignation/Draw Offers
 * -------------------------*/

void GameWindow::CheckAnnounceMate(void) {
  // In the Mate Finder, we announce mates during the search (and give the user
  // the option of continuing; playing the key move (and adjust mate level by
  // one); or cancelling the search.
  if (level.mode == pmode_MateFinder) return;

  if (IsFront() && Engine_BestScore(engine) > mateWinVal && !autoPlaying &&
      !analyzeEPD && !ExaChess && !hasAnnouncedMate && GetMultiPVcount() <= 1) {
    INT n = (1 + maxVal - Engine_BestScore(engine)) / 2;
    AnnounceMateDialog(this, n, Engine_MainLine(engine));
    hasAnnouncedMate = true;
  }
} /* GameWindow::CheckAnnounceMate */

BOOL GameWindow::CheckResign(void)  // Is done BEFORE the move is performed
{
  if (!Prefs.Messages.canResign || hasResigned || ExaChess || autoPlaying ||
      analyzeEPD)
    return false;
  if (Engine_BestScore(engine) >= resignVal ||
      Engine_BestScore(engine) <= mateLoseVal)
    return false;

  hasResigned = true;

  BOOL wasRated = isRated;
  SetGameResult(result_Resigned, (game->player == white ? infoResult_BlackWin
                                                        : infoResult_WhiteWin));
  GameOverDialog(this, true, wasRated);

  if (QuestionDialog(this, "Continue Game?",
                     "Do you wish to continue the game anyway?", "Stop",
                     "Continue"))
    return true;
  else {
    SetGameResult(result_Unknown, infoResult_Unknown);
    StopClock();
    return false;
  }
} /* GameWindow::CheckResign */

BOOL GameWindow::AcceptDrawOffer(void) {
  if (!drawOffered) return false;
  drawOffered = false;

  BOOL accept = false;
  INT score = Engine_BestScore(engine);
  INT phase = engine->V.phase;
  INT irr = game->DrawData[game->currMove].irr;

  if (score <= -150)
    accept = true;
  else if (score <= -100)
    accept = (phase >= 5);
  else if (score <= -50)
    accept = (phase >= 6 && irr < game->currMove - 10);
  else if (score == 0 && phase >= 7)
    accept = true;
  else if (score <= 0)
    accept = (phase >= 7 && irr < game->currMove - 20);
  else if (score <= 20)
    accept = (phase >= 7 && irr < game->currMove - 40);
  else if (score <= 40)
    accept = (phase >= 7 && irr < game->currMove - 60);

  if (!accept)
    NoteDialog(this, "Draw Offer", "Draw offer declined...", cdialogIcon_Error);
  else {
    BOOL wasRated = isRated;
    SetGameResult(result_DrawAgreed, infoResult_Draw);
    GameOverDialog(this, true, wasRated);
    StopClock();
  }

  return accept;
} /* GameWindow::AcceptDrawOffer */

/**************************************************************************************************/
/*                                                                                                */
/*                                        STOP/ABORT SEARCH */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------------- Stops the Search
 * --------------------------------------*/
// Instructs the engine to exit "gracefully" (i.e. we don't kill the task) and
// play the best move found so far. Just before the engine task
// returns/completes it sends an "msg_EndSearch" message which in turn invokes
// the "GameWindow::SearchCompleted" routine

void GameWindow::Analyze_Stop(void) {
  userStopped = true;

  infoAreaView->SetAnalysisStatus("Stopping...", true);

  if (monitoring) {
    UCI_Engine_Abort(uciEngineId);
    infoAreaView->SetAnalysisStatus("Idle", true);
    monitoring = false;
    AdjustAnalyzeMenu();
    AdjustToolbar();
    UCI_ReleaseLock(uciEngineId, this);
  } else if (autoPlaying) {
    UCI_Engine_Abort(uciEngineId);
    demoPlaying = analyzeCol = false;
    if (analyzeGame)
      AnalyzeGame_End();
    else
      EndAutoPlay();
    UCI_ReleaseLock(uciEngineId, this);
  } else {
    analyzeEPD = false;
    UCI_Engine_Stop(uciEngineId);
  }

  StopClock();
} /* GameWindow::Analyze_Stop */

/*---------------------------- Postponing Commands until Search Completes
 * ------------------------*/
// Certain commands cannot be performed while the engine is thinking. In these
// cases simply kill the engine task directly, without giving it a chance to
// complete.

void GameWindow::CheckAbortEngine(void) {
  if (!UsingUCIEngine() && !Engine_TaskRunning(engine)) return;

  BOOL wasThinking = (thinking || monitoring);
  autoPlaying = demoPlaying = thinking = backgrounding = monitoring =
      analyzeEPD = false;

  if (Engine_TaskRunning(engine)) {
    infoAreaView->SetAnalysisStatus("Stopping...", true);
    UCI_Engine_Abort(uciEngineId);
    infoAreaView->SetAnalysisStatus("Idle", true);
  }

  if (wasThinking) {
    infoAreaView->SetAnalysisStatus("Idle", true);
    AdjustAnalyzeMenu();
    AdjustToolbar();
  }

  UCI_ReleaseLock(uciEngineId, this);
} /* GameWindow::CheckAbortEngine */

/**************************************************************************************************/
/*                                                                                                */
/*                                          BACKGROUNDING */
/*                                                                                                */
/**************************************************************************************************/

// When the engine has played a move, and permanent brain is on (and it's one of
// the relevant playing modes), the engine will start searching in the
// background based on the user playing an expected move (MainLine[1] from the
// previous search. May NOT be called if the game is over.

void GameWindow::CheckBackgrounding(void) {
  if (!permanentBrain || level.mode > pmode_Leisure || ExaChess ||
      isNull(hintMove) || !Engine_IsPonderMove(engine))
    return;

  if (Engine_OtherRunning(
          &Global, engine))  // Disable backgrounding if other engines running
    return;

  game->PlayMove(&hintMove);

  if (!game->GameOver()) {
    backgrounding = true;
    expectedMove = hintMove;
    infoAreaView->ResetAnalysis();
    SetSearchParam();

    infoAreaView->SetAnalysisStatus("Pondering...");
    UCI_Engine_Start(uciEngineId, engine, game);
  }

  game->UndoMove(true);
  game->lastMove--;
  game->result = result_Unknown;
} /* GameWindow::CheckBackgrounding */

/**************************************************************************************************/
/*                                                                                                */
/*                                          MONITOR MODE */
/*                                                                                                */
/**************************************************************************************************/

// In monitor mode, we must start the search in the background each time the
// user plays/unplays moves, initiates a new game (new, reset, open, pos editor,
// paste), or switches to monitor mode. Enable Go button: If already searching
// abort first.

void GameWindow::CheckMonitorMode(void) {
  if (level.mode != pmode_Monitor || game->GameOver() ||
      theApp->ModalLoopRunning())
    return;

  if (Engine_TaskRunning(engine)) {
    infoAreaView->SetAnalysisStatus("Stopping...", true);
    UCI_Engine_Abort(uciEngineId);
  }

  if (!UCI_RequestLock(uciEngineId, this)) {
    infoAreaView->SetAnalysisStatus("Engine busy");
    return;
  }

  thinking = backgrounding = autoPlaying = false;
  monitoring = true;

  infoAreaView->ResetAnalysis();
  infoAreaView->SetAnalysisStatus("Monitoring...");
  AdjustAnalyzeMenu();
  AdjustToolbar();
  SetSearchParam();

  ResetClocks();
  StartClock();

  UCI_Engine_Start(uciEngineId, engine, game);
} /* GameWindow::CheckMonitorMode */

/**************************************************************************************************/
/*                                                                                                */
/*                                              HINT */
/*                                                                                                */
/**************************************************************************************************/

void GameWindow::Analyze_Hint(void) {
  if (isNull(hintMove))
    NoteDialog(
        this, "No Hints Available",
        "Sorry, there are no hints for this position. Hints are only available "
        "if Sigma Chess has been analyzing the previous position.");
  else {
    CHAR moveStr[20], hint[100];
    CalcMoveStr(&hintMove, moveStr);
    Format(hint, "I suggest you play %s...", moveStr);
    NoteDialog(this, "Hint", hint);
  }
} /* GameWindow::Analyze_Hint */
