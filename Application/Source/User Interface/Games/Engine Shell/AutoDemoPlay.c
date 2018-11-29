/**************************************************************************************************/
/*                                                                                                */
/* Module  : AUTODEMOPLAY.C */
/* Purpose : Implements auto/demo play. */
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
#include "UCI_AppleEvents.h"

#include "Engine.f"
#include "Move.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                          AUTO/DEMO PLAY */
/*                                                                                                */
/**************************************************************************************************/

void GameWindow::Analyze_AutoPlay(void) {
  if (!CanAutoPlay() || !UCI_RequestLock(uciEngineId, this, true)) return;

  // Enter "autoplay/thinking" mode
  autoPlaying = thinking = true;
  AdjustAnalyzeMenu();
  AdjustToolbar();

  // Start clock, init search parameters and launch engine task
  StartSearch();
} /* GameWindow::Analyze_AutoPlay */

void GameWindow::AutoSearchCompleted(void) {
  if (Engine_Aborted(engine)) {
    demoPlaying = false;
    EndAutoPlay();
  } else {
    PlayEngineMove();

    if (EngineMatch.gameWin == this) {
      INT score = Engine_BestScore(engine);
      CHAR adjText[100];

      // Stop game if time forfeit
      if (EngineMatch.timeForfeit) {
        SetGameResult(result_TimeForfeit,
                      (game->player == white ? infoResult_WhiteWin
                                             : infoResult_BlackWin));
        Format(adjText, "Time forfeit: %s wins",
               (game->player == white ? "White" : "Black"));
        SetAnnotation(adjText);
      }
      // Adjudicate engine match
      else if (Prefs.EngineMatch.adjDraw && score == 0) {
        EngineMatch.adjWinCount = 0;
        if (game->lastMove <=
            30)  // Don't check for draw in the opening (first 15 moves
          EngineMatch.adjDrawCount = 0;
        else if (++EngineMatch.adjDrawCount >= 4) {
          SetGameResult(result_DrawAgreed, infoResult_Draw);
          Format(adjText, "Draw agreed");
          SetAnnotation(adjText);
        }
      } else if (Prefs.EngineMatch.adjWin &&
                 score >= 100 * Prefs.EngineMatch.adjWinLimit) {
        EngineMatch.adjDrawCount = 0;
        EngineMatch.adjWinCount++;
      } else if (Prefs.EngineMatch.adjWin &&
                 score <= -100 * Prefs.EngineMatch.adjWinLimit &&
                 EngineMatch.prevScore >= 100 * Prefs.EngineMatch.adjWinLimit) {
        EngineMatch.adjDrawCount = 0;
        if (++EngineMatch.adjWinCount >= 4) {
          SetGameResult(result_Resigned,
                        (game->player == white ? infoResult_WhiteWin
                                               : infoResult_BlackWin));
          CHAR scoreStr[20];
          ::CalcScoreStr(scoreStr, score, scoreType_True);
          Format(adjText, "Adjudicated: %s wins (score %s)",
                 (game->player == white ? "White" : "Black"), scoreStr);
          SetAnnotation(adjText);
        }
      } else {
        EngineMatch.adjWinCount = 0;
        EngineMatch.adjDrawCount = 0;
      }

      EngineMatch.prevScore = score;
    }

    if (game->GameOver() || CheckResign())
      EndAutoPlay();
    else
      StartSearch();
  }
} /* GameWindow::AutoSearchCompleted */

void GameWindow::EndAutoPlay(void) {
  // Check if currently playing an Engine Match
  if (EngineMatch.gameWin == this) {
    // Optionally save in collection window
    if (EngineMatch.colWin) {
      EngineMatch.colWin->AddGame(this);
      AdjustFileMenu();
      AdjustToolbar();
    }

    // Update statistics
    BOOL enginesSwapped =
        Prefs.EngineMatch.alternate && even(EngineMatch.currGameNo);
    switch (game->Info.result) {
      case infoResult_WhiteWin:
        if (enginesSwapped)
          EngineMatch.winCount2++;
        else
          EngineMatch.winCount1++;
        break;
      case infoResult_BlackWin:
        if (enginesSwapped)
          EngineMatch.winCount1++;
        else
          EngineMatch.winCount2++;
        break;
      default:
        if (!userStopped) EngineMatch.drawCount++;
    }

    // Check if engine match is over
    if (EngineMatch.currGameNo < Prefs.EngineMatch.matchLen && !userStopped) {
      EngineMatch.currGameNo++;
    } else {
      UCI_ForceQuitEngines();

      EngineMatch.gameWin = nil;
      game->dirty = false;
      HandleMessage(game_ResetGame);
      demoPlaying = false;  // Fall through to "if" statement below

      ShowEngineMatchResult();

      SelectEngine(uci_SigmaEngineId);
    }
  }

  if (!demoPlaying) {
    autoPlaying = thinking = false;
    EngineMatch.gameWin = nil;
    AdjustFileMenu();
    AdjustGameMenu();
    AdjustAnalyzeMenu();
    AdjustToolbar();
    infoAreaView->ResetAnalysis();

    UCI_ReleaseLock(uciEngineId, this);
  } else {
    game->dirty = false;
    HandleMessage(
        game_ResetGame,
        (EngineMatch.gameWin == this ? analyze_EngineMatch : analyze_DemoPlay));

    StartSearch();
  }
} /* GameWindow::EndAutoPlay */

void GameWindow::Analyze_DemoPlay(void) {
  if (!CanDemoPlay() || !UCI_RequestLock(uciEngineId, this, true)) return;
  if (!CheckSave("Save before demo play?")) return;

  game->dirty = false;
  HandleMessage(game_ResetGame, analyze_DemoPlay);
  demoPlaying = true;
  Analyze_AutoPlay();
} /* GameWindow::Analyze_DemoPlay */

BOOL GameWindow::CanAutoPlay(void) {
  if (autoPlaying || thinking) return false;
  return (!game->GameOver() && level.mode <= pmode_Novice &&
          level.mode != pmode_Leisure);  //###
} /* GameWindow::CanAutoPlay */

BOOL GameWindow::CanDemoPlay(void) {
  if (autoPlaying || thinking) return false;
  return (level.mode <= pmode_Novice && level.mode != pmode_Leisure);  //###
} /* GameWindow::CanDemoPlay */

/**************************************************************************************************/
/*                                                                                                */
/*                                          ENGINE MATCH */
/*                                                                                                */
/**************************************************************************************************/

void GameWindow::Analyze_EngineMatch(void) {
  if (!CanDemoPlay() || !UCI_RequestLock(uciEngineId, this, true)) return;

  level = Prefs.EngineMatch.level;

  game->dirty = false;
  HandleMessage(game_ResetGame, analyze_EngineMatch);

  demoPlaying = true;
  Analyze_AutoPlay();
} /* GameWindow::Analyze_EngineMatch */

BOOL GameWindow::AbandonEngineMatch(BOOL confirm) {
  if (!autoPlaying || EngineMatch.gameWin != this) return true;

  if (!confirm) {
    NoteDialog(this, "Engine Match Running",
               "This command is not available because an engine match is "
               "currently running");
    return false;
  } else {
    if (QuestionDialog(this, "Abort Engine Match",
                       "Are you sure you want to abort the engine match?",
                       "Resume", "Abort"))
      return false;
  }

  return true;
} /* GameWindow::AbandonEngineMatch */

void GameWindow::ShowEngineMatchResult(
    void)  // Called when engine match is over
{
  INT gameCount =
      EngineMatch.winCount1 + EngineMatch.winCount2 + EngineMatch.drawCount;
  if (gameCount == 0) return;

  INT pct1 =
      50 * (2 * EngineMatch.winCount1 + EngineMatch.drawCount) / gameCount;
  INT pct2 = 100 - pct1;

  CHAR str[256], scoreStr1[30], scoreStr2[30], halfStr[10];
  CopyStr(odd(EngineMatch.drawCount) ? ".5" : "", halfStr);
  Format(scoreStr1, "%d%s (%d%c)",
         EngineMatch.winCount1 + EngineMatch.drawCount / 2, halfStr, pct1, '%');
  Format(scoreStr2, "%d%s (%d%c)",
         EngineMatch.winCount2 + EngineMatch.drawCount / 2, halfStr, pct2, '%');

  Format(str, "%s : %s\n%s : %s", UCI_EngineName(Prefs.EngineMatch.engine1),
         scoreStr1, UCI_EngineName(Prefs.EngineMatch.engine2), scoreStr2);
  NoteDialog(this, "Engine Match Result", str);

  if (boardTurned) TurnBoard();
} /* GameWindow::ShowEngineMatchResult */
