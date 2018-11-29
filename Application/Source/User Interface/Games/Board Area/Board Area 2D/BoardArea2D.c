/**************************************************************************************************/
/*                                                                                                */
/* Module  : BOARD2DVIEW.C                                                                        */
/* Purpose : This module implements the 2D board area.                                            */
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

#include "BoardArea2D.h"
#include "SigmaApplication.h"
#include "BoardView.h"
#include "GameWindow.h"
#include "LevelDialog.h"
#include "PromotionDialog.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                         DIMENSIONS                                             */
/*                                                                                                */
/**************************************************************************************************/

#define dividerSize         8
#define playerViewHeight   24 //20
#define playerViewWidth   (8*squareWidth + 2*frameWidth - clockViewWidth - 8)

INT BoardArea_Width (INT sqWidth)
{
   return 2*(dividerSize + BoardFrame_Width(sqWidth)) + 8*sqWidth;
} /* BoardArea_Width */


INT BoardArea_Height (INT sqWidth)
{
   return 2*(dividerSize + playerViewHeight + dividerSize + BoardFrame_Width(sqWidth)) + 8*sqWidth;
} /* BoardArea_Height */


/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

BoardArea2DView::BoardArea2DView (CViewOwner *parent, CRect frame)
 : BoardAreaView(parent,frame)
{
   gameWin = (GameWindow*)(Window());

   CalcFrames();
   boardView   = new BoardView(this,  boardRect, gameWin->game, gameWin);
   playerViewT = new PlayerView(this, playerTRect, true);
   playerViewB = new PlayerView(this, playerBRect, false);
   clockViewT  = new ClockView(this, clockTRect, true);
   clockViewB  = new ClockView(this, clockBRect, false);
} /* BoardArea2DView::BoardArea2DView */


void BoardArea2DView::CalcFrames (void)
{
   squareWidth = gameWin->squareWidth;
   frameWidth  = gameWin->frameWidth;

   boardRect = bounds;
   boardRect.Inset(dividerSize, playerViewHeight + 2*dividerSize);

   playerTRect.Set(0,0,playerViewWidth,playerViewHeight);
   playerTRect.Offset(dividerSize, dividerSize);

   playerBRect.Set(0,0,playerViewWidth,playerViewHeight);
   playerBRect.Offset(dividerSize, bounds.bottom - playerViewHeight - dividerSize);

   clockTRect.Set(0,0,clockViewWidth,playerViewHeight);
   clockTRect.Offset(bounds.right - clockViewWidth - dividerSize, dividerSize);

   clockBRect.Set(0,0,clockViewWidth,playerViewHeight);
   clockBRect.Offset(bounds.right - clockViewWidth - dividerSize, bounds.bottom - playerViewHeight - dividerSize);

   ExcludeRect(boardRect);
} /* BoardArea2DView::CalcFrames */


/**************************************************************************************************/
/*                                                                                                */
/*                                    UPDATE BOARD AREA PARTS                                     */
/*                                                                                                */
/**************************************************************************************************/

void BoardArea2DView::HandleUpdate (CRect updateRect)
{
   BackView::HandleUpdate(updateRect);

   Outline3DRect(playerViewT->frame);
   Outline3DRect(playerViewB->frame);
   Outline3DRect(clockViewT->frame);
   Outline3DRect(clockViewB->frame);

   DrawLevelInfo(white);
   DrawLevelInfo(black);
} /* BoardArea2DView::HandleUpdate */


void BoardArea2DView::HandleResize (void)
{
   CalcFrames();
   boardView->SetFrame(boardRect);
   playerViewT->SetFrame(playerTRect);
   playerViewB->SetFrame(playerBRect);
   clockViewT->SetFrame(clockTRect);
   clockViewB->SetFrame(clockBRect);
} /* BoardArea2DView::HandleResize */

/*------------------------------------- Squares & Board Frame ------------------------------------*/

void BoardArea2DView::DrawBoardFrame (void)
{
   boardView->DrawFrame();
} /* BoardArea2DView::DrawBoardFrame */


void BoardArea2DView::DrawAllSquares (void)
{
   boardView->DrawAllSquares();
} /* BoardArea2DView::DrawAllSquares */


void BoardArea2DView::DrawSquare (SQUARE sq)
{
   boardView->DrawSquare(sq);
} /* BoardArea2DView::DrawSquare */


void BoardArea2DView::SetMoveMarker (BOOL engineMove)
{
   boardView->SetMoveMarker(engineMove);
} /* BoardArea2DView::SetMoveMarker */


void BoardArea2DView::ClearMoveMarker (void)
{
   boardView->ClearMoveMarker();
} /* BoardArea2DView::ClearMoveMarker */


void BoardArea2DView::DrawPlayerIndicator (void)
{
   boardView->DrawPlayerIndicator();
   if (playerViewT) playerViewT->DrawPlayerIndicator();
   if (playerViewB) playerViewB->DrawPlayerIndicator();
} /* BoardArea2DView::DrawPlayerIndicator */


void BoardArea2DView::FrameSquare (SQUARE sq, RGB_COLOR *color) // Must be overridden
{
   boardView->FrameSquare(sq, color);
} /* BoardArea2DView::FrameSquare */

/*--------------------------------------- Draw Moves/Unmoves -------------------------------------*/

void BoardArea2DView::DrawPieceMovement (PIECE piece, PIECE target, SQUARE from, SQUARE to)
{
   boardView->DrawPieceMovement(piece, target, from, to);
} /* BoardArea2DView::DrawPieceMovement */


PIECE BoardArea2DView::AskPromPiece (SQUARE from, SQUARE to)
{
   CPromotionDialog *dialog = PromotionDialog(game->player);

   LONG T = Timer() + 30;

   game->Board[from] = empty;
   game->Board[to]   = pawn + game->player;

   gameWin->promoting  = true;
   dialog->modalRunning = true;
   do
   {
      theApp->ProcessEvents();

      if (Timer() > T)
      {
         if (! game->Board[to])
            game->Board[to] = pawn + game->player;
         else
            game->Board[to] = empty;
         DrawSquare(to);
         T = Timer() + 30;
      }
   } while (dialog->modalRunning && gameWin->promoting);

   gameWin->promoting = false;

   PIECE prom = dialog->prom;
   delete dialog;

   return prom;
} /* BoardArea2DView::AskPromPiece */
