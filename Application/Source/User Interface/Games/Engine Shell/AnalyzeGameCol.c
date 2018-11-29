/**************************************************************************************************/
/*                                                                                                */
/* Module  : ANALYZEGAMECOL.C                                                                     */
/* Purpose : Implements automatic analysis of games/collections.                                  */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
  and the following disclaimer in the documentation and/or other materials provided with the 
  distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "GameWindow.h"
#include "CollectionWindow.h"
#include "SigmaApplication.h"
#include "PosLibrary.h"
#include "GameOverDialog.h"
#include "CDialog.h"

#include "Engine.f"
#include "Move.f"


/**************************************************************************************************/
/*                                                                                                */
/*                                   ANALYZE GAME/COLLECTION                                      */
/*                                                                                                */
/**************************************************************************************************/

void GameWindow::AnalyzeGame (void)
{
   SetMultiPVcount(1);

   if (analyzeCol && game->currMove > 0)
   {  game->UndoAllMoves();
      GameMoveAdjust(true);
   }

   game->ClrAnnotation();
   GameMoveAdjust(false);

   // Set solver mode:
   level0 = level;
   level.mode = pmode_Solver;
   level.Solver.timeLimit = Prefs.AutoAnalysis.timePerMove;
   level.Solver.scoreLimit = maxVal;
   ResetClocks();
   boardAreaView->DrawModeIcons();

   // Enter "analyze game" mode
   autoPlaying = thinking = analyzeGame = true;
   AdjustAnalyzeMenu();
   AdjustToolbar();

   // Start clock, init search parameters and launch engine task
   analyzeGameMove0 = game->currMove;
   PrevAnalysis.score[1] = 0;
   clrMove(PrevAnalysis.PV[1][0]);
   
   AnalyzeGame_StartSearch();
} /* GameWindow::AnalyzeGame */


void GameWindow::AnalyzeCollection (void)
{
   analyzeCol = true;
   AnalyzeGame();
} /* GameWindow::AnalyzeCollection */


void GameWindow::AnalyzeGame_StartSearch (void)
{
   ResetClocks();

   if (game->currMove < game->lastMove)
      StartSearch();
   else
      AnalyzeGame_End();
} /* GameWindow::AnalyzeGame_StartSearch */


void GameWindow::AnalyzeGame_SearchCompleted (void)
{
   if (Engine_Aborted(engine))
   {
      analyzeGame = analyzeCol = false;
      AnalyzeGame_End();
   }
   else  // Store the analysis
   {
//      INT analyzeScore = Engine_BestScore(engine);

      // Check if we should skip moves for this player (in previous position)
      if (game->currMove <= game->lastMove &&
          (game->player == black && Prefs.AutoAnalysis.skipWhitePos ||
          game->player == white && Prefs.AutoAnalysis.skipBlackPos)
         )
         goto nextMove;

      if (game->currMove < analyzeGameMove0 + 2)
         goto nextMove;

      // Skip if still in opening book
      if (engine->S.libMovesOnly)
         goto nextMove;

      // Check if we should skip matching moves
      MOVE *bestMove   = PrevAnalysis.PV[1];
      MOVE *actualMove = &(game->Record[game->currMove]);

      if (Prefs.AutoAnalysis.skipMatching && EqualMove(bestMove,actualMove))
         goto nextMove;

      // Calc score improvement and check if we should skip
      INT scoreImprovement = PrevAnalysis.score[1] + Analysis.score[1];
 
//    if (scoreImprovement <= 0)
//       goto nextMove;

      if (Prefs.AutoAnalysis.skipLowScore && scoreImprovement < Prefs.AutoAnalysis.scoreLimit)
         goto nextMove;

      // If we get here -> Store the analysis (temporarily switch to absolute numeric)
      CHAR Text[1000];
      INT  len = BuildAnalysisString(&PrevAnalysis, Text, ! EqualMove(bestMove,actualMove), -Analysis.score[1]);
      game->SetAnnotation(game->currMove, Text, len, false);

      if (scoreImprovement > 150)
         game->SetAnnotationGlyph(game->currMove, 4); // "??"
      else if (scoreImprovement > 75)
         game->SetAnnotationGlyph(game->currMove, 2); // "?"

nextMove:
      boardAreaView->ClearMoveMarker();
      game->RedoMove(true);
      boardAreaView->DrawMove(true);
      GameMoveAdjust(false, true);

      PrevAnalysis = Analysis;
      AnalyzeGame_StartSearch();
   }
} /* GameWindow::AnalyzeGame_SearchCompleted */


void GameWindow::AnalyzeGame_End (void)
{
   if (! colWin)
      analyzeCol = false;

   if (! analyzeCol || (colWin && ! colWin->CanNextGame()))
   {
      autoPlaying = thinking = analyzeGame = false;
      AdjustFileMenu();
      AdjustGameMenu();
      AdjustAnalyzeMenu();
      AdjustToolbar();
      infoAreaView->ResetAnalysis();

      level = level0;   // Restore previous playing
      ResetClocks();
      boardAreaView->DrawModeIcons();
   }
   else
   {
      Save();
      colWin->NextGame(this);
      AdjustCollectionMenu();

      game->ClrAnnotation();
      GameMoveAdjust(false);

      if (game->currMove > 0)
      {  game->UndoAllMoves();
         GameMoveAdjust(true);
      }
      AnalyzeGame_StartSearch();
   }
} /* GameWindow::AnalyzeGame_End */
