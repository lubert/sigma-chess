/**************************************************************************************************/
/*                                                                                                */
/* Module  : BOARDAREA.C                                                                          */
/* Purpose : Virtual base class common to the 2D and 3D boards.                                   */
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

#include "BoardArea.h"
#include "GameWindow.h"
#include "LevelDialog.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

BoardAreaView::BoardAreaView (CViewOwner *parent, CRect frame)
 : BackView(parent,frame)
{
   gameWin = ((GameWindow*)Window());
   game = gameWin->game;
   playerViewT = playerViewB = nil;
   clockViewT = clockViewB = nil;
} /* BoardAreaView::BoardAreaView */


/**************************************************************************************************/
/*                                                                                                */
/*                                      DRAW BOARD AREA PARTS                                     */
/*                                                                                                */
/**************************************************************************************************/

void BoardAreaView::HandleUpdate (CRect updateRect)
{
   BackView::HandleUpdate(updateRect);
} /* BoardAreaView::HandleUpdate */


void BoardAreaView::ClearMenu (void)
{
} /* BoardAreaView::ClearMenu */

/*------------------------------------- Squares & Board Frame ------------------------------------*/

void BoardAreaView::DrawBoard (void)
{
} /* BoardAreaView::DrawBoard */


void BoardAreaView::DrawBoardFrame (void)
{
} /* BoardAreaView::DrawBoardFrame */


void BoardAreaView::DrawAllSquares (void)
{
} /* BoardAreaView::DrawAllSquares */


void BoardAreaView::DrawSquare (SQUARE sq)
{
} /* BoardAreaView::DrawSquare */


void BoardAreaView::SetMoveMarker (BOOL engineMove)
{
} /* BoardAreaView::SetMoveMarker */


void BoardAreaView::ClearMoveMarker (void)
{
} /* BoardAreaView::ClearMoveMarker */


void BoardAreaView::DrawPlayerIndicator (void)
{
} /* BoardAreaView::DrawPlayerIndicator */


void BoardAreaView::RefreshGameStatus (void)
{
} /* BoardAreaView::RefreshGameStatus */

/*--------------------------------------- Draw Moves/Unmoves -------------------------------------*/

void BoardAreaView::DrawMove (BOOL engineMove)
{
   MOVE  *m = &game->Record[game->currMove];
   PIECE *B = game->Board;
   BOOL  sound = (! engineMove || ! ((GameWindow*)Window())->ExaChess);

   switch (m->type)
   {
      case mtype_O_O :
         Swap(&B[right(m->to)], &B[left(m->to)]);                               // Remove rook temp.
         DrawPieceMovement(m->piece, empty, m->from, m->to);                    // Draw king movement
         if (sound) sigmaApp->PlayMoveSound(false);
         Swap(&B[right(m->to)], &B[left(m->to)]);                               // Restore rook
         DrawPieceMovement(B[left(m->to)], empty, right(m->to), left(m->to));   // Draw rook movement
         break;
      case mtype_O_O_O :
         Swap(&B[left2(m->to)], &B[right(m->to)]);                              // Remove rook temp.
         DrawPieceMovement(m->piece, empty, m->from, m->to);                    // Draw king movement
         if (sound) sigmaApp->PlayMoveSound(false);
         Swap(&B[left2(m->to)], &B[right(m->to)]);                              // Restore rook
         DrawPieceMovement(B[right(m->to)], empty, left2(m->to), right(m->to)); // Draw rook movement
         break;
      case mtype_EP :
         SQUARE epSq = m->to + (pieceColour(m->piece) == white ? -0x10 : +0x10);
         B[epSq] = pawn + game->player;                                         // Restore cap pawn temp.
         DrawPieceMovement(m->piece, empty, m->from, m->to);                    // Draw pawn move.
         B[epSq] = empty;                                                       // Remove cap pawn again.
         DrawSquare(epSq);                                                      // Draw empty ep square.
         break;
      default:
         DrawPieceMovement(m->piece, m->cap, m->from, m->to);
   }

   if (sound && engineMove && Prefs.Sound.moveBeep) Beep(1);
   if (sound) sigmaApp->PlayMoveSound(m->cap != empty);
} /* BoardAreaView::DrawMove */


void BoardAreaView::DrawUndoMove (void)
{
   MOVE *m = &game->Record[game->currMove + 1];

   DrawSquare(m->to);
   DrawSquare(m->from);

   switch (m->type)
   {
      case mtype_O_O :
         DrawSquare(left(m->to));
         DrawSquare(right(m->to));
         break;
      case mtype_O_O_O :
         DrawSquare(right(m->to));
         DrawSquare(left2(m->to));
         break;
      case mtype_EP :
         DrawSquare(m->to + (pieceColour(m->piece) == white ? -0x10 : +0x10));
   }
} /* BoardAreaView::DrawUndoMove */


void BoardAreaView::DrawPieceMovement (PIECE piece, PIECE target, SQUARE from, SQUARE to)
{
} /* BoardAreaView::DrawPieceMovement */

/*-------------------------------------- Read/Track Player Moves ---------------------------------*/

void BoardAreaView::PerformMove (SQUARE from, SQUARE to)
{
   if (game->editingPosition)
   {
      if (from != to)
         game->Edit_MovePiece(from, to),
         DrawSquare(from);
      else
         game->Edit_ClearPiece(to);
      DrawSquare(to);
      FlushPortBuffer();
      sigmaApp->PlayMoveSound(false);
   }
   else
   {
      for (INT i = 0; i < game->moveCount; i++)
         if (game->Moves[i].from == from && game->Moves[i].to == to)
         {
            MOVE m = game->Moves[i];
            if (isPromotion(m))
            {  sigmaApp->PlayMoveSound(false);
               m.type = AskPromPiece(m.from, m.to);
            }

            PerformPlayerMove(&m);
            return;
         }

      // If we get here, the move was illegal and we "reset" the visual board:

      if (from != to) Beep(1);
      SetMoveMarker(false);
      DrawSquare(from);
      DrawSquare(to);
      FlushPortBuffer();
   }
} /* BoardAreaView::PerformMove */


void BoardAreaView::PerformPlayerMove (MOVE *m)
{
   // Stop player's clock:
   gameWin->StopClock();

   // Perform player's move:
   gameWin->FlushAnnotation();
   game->PlayMove(m);

   DrawSquare(m->to);
 
   switch (m->type)
   {
      case mtype_O_O :
         sigmaApp->PlayMoveSound(false);
         DrawPieceMovement(rook + pieceColour(m->piece), empty, right(m->to), left(m->to));
         break;
      case mtype_O_O_O :
         sigmaApp->PlayMoveSound(false);
         DrawPieceMovement(rook + pieceColour(m->piece), empty, left2(m->to), right(m->to));
         break;
      case mtype_EP :
         DrawSquare(m->to + (pieceColour(m->piece) == white ? -0x10 : +0x10));
         break;
   }

   FlushPortBuffer();

   // Play move sound, draw player indicator and adjust menus and game record:
   gameWin->PlayerMovePerformed();
} /* BoardAreaView::PerformPlayerMove */


PIECE BoardAreaView::AskPromPiece (SQUARE from, SQUARE to)
{
   return queen + game->player;  // Should be overridden 
} /* BoardAreaView::AskPromPiece */

/*------------------------------------------ Player Names ----------------------------------------*/

void BoardAreaView::DrawPlayerInfo (void)
{
   if (playerViewT) playerViewT->Redraw();
   if (playerViewB) playerViewB->Redraw();
} /* BoardAreaView::DrawPlayerInfo */

/*------------------------------------------ Mode & Clocks ---------------------------------------*/

void BoardAreaView::DrawModeIcons (void)   // Must be called whenever a move is played
{
//   if (clockViewT) clockViewT->DrawMode();
//   if (clockViewB) clockViewB->DrawMode();
} /* BoardAreaView::DrawModeIcons */


void BoardAreaView::DrawClockInfo (void)   // Draws both time and moves to play/level
{
   if (clockViewT) clockViewT->Redraw();
   if (clockViewB) clockViewB->Redraw();
} /* BoardAreaView::DrawClockInfo */


void BoardAreaView::DrawLevelInfo (COLOUR colour, BOOL redraw)
{
   if (! clockViewB || ! clockViewT) return;
   if (colour == clockViewB->Colour())
      clockViewB->DrawLevel(redraw);
   else
      clockViewT->DrawLevel(redraw);
} /* BoardAreaView::DrawLevelInfo */


void BoardAreaView::DrawClockTime (COLOUR colour)
{
   if (! clockViewB || ! clockViewT) return;
   if (colour == clockViewB->Colour())
      clockViewB->DrawTime();
   else
      clockViewT->DrawTime();
} /* BoardAreaView::DrawClockTime */

/*----------------------------------------- Position Editor --------------------------------------*/
// Only used if position editor included in board view (which it is in 3D but NOT in 2D).

void BoardAreaView::ShowPosEditor (BOOL showPos)
{
} /* BoardAreaView::ShowPosEditor */

/*-------------------------------------- Show Legal Moves ----------------------------------------*/

void BoardAreaView::ShowLegalMoves (SQUARE from)
{
   if (game->editingPosition) return;

   PIECE p = game->Board[from];
   if (p == empty) return;

   if (pieceColour(p) == game->opponent)
   {  ShowHelpTip("Legal moves can only be shown for the side to move");
      return;
   }

   FrameSquare(from, &color_Black);

   BOOL canMove = false;
   for (INT i = 0; i < game->moveCount; i++)
      if (game->Moves[i].from == from)
      {  canMove = true;
         FrameSquare(game->Moves[i].to, &color_Blue);
      }

   if (! canMove)
      ShowHelpTip("This piece has no legal moves");

   SetForeColor(&color_Black);
   sigmaApp->WaitMouseReleased();

   ClearShowLegal();

   DrawSquare(from);

   if (canMove)
      for (INT i = 0; i < game->moveCount; i++)
         if (game->Moves[i].from == from)
            DrawSquare(game->Moves[i].to);
} /* BoardAreaView::ShowLegalMoves */


void BoardAreaView::FrameSquare (SQUARE sq, RGB_COLOR *color) // Must be overridden
{
} /* BoardAreaView::FrameSquare */


void BoardAreaView::ClearShowLegal (void)
{
} /* BoardAreaView::ClearShowLegal */


/**************************************************************************************************/
/*                                                                                                */
/*                                     PLAYER/CLOCK/MODE VIEWS                                    */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Common Base View ----------------------------------------*/

PlayerClockView::PlayerClockView (CViewOwner *parent, CRect frame, BOOL atTheTop)
 : DataView(parent, frame, false)
{
   atTop = atTheTop;
// SetFontSize(10);
// SetFontStyle(fontStyle_Bold);

   SetThemeFont(kThemeEmphasizedSystemFont);

   gameWin = (GameWindow*)Window();
} /* PlayerClockView::PlayerClockView */


void PlayerClockView::HandleUpdate (CRect updateRect)
{
   CRect r = bounds;

   if (! gameWin->mode3D)
   {  SetForeColor(RunningOSX() || ! Active() ? &color_DkGray : &color_Black);
      DrawRectFrame(r);
   }
   else
   {  if (Colour() == white) Draw3DFrame(r, &color_Gray, &color_White);
      else Draw3DFrame(r, &color_DkGray, &color_Gray);
   }

   r.Inset(1, 1);

   if (Colour() == white)
   {// Draw3DFrame(r, &color_White, &color_Gray); r.Inset(1, 1);
      DrawRectFill(r, &color_LtGray);
      SetForeColor(&color_Black);
      SetBackColor(&color_LtGray);
   }
   else
   {//  Draw3DFrame(r, &color_Gray, &color_DkGray); r.Inset(1, 1);
      DrawRectFill(r, &color_MdGray);
      SetForeColor(&color_White);
      SetBackColor(&color_MdGray);
   }
} /* PlayerClockView::HandleUpdate */


COLOUR PlayerClockView::Colour (void)
{
   return (atTop == gameWin->boardTurned ? white : black);
} /* PlayerClockView::Colour */

/*---------------------------------------- Player View -------------------------------------------*/

PlayerView::PlayerView (CViewOwner *parent, CRect frame, BOOL atTop)
 : PlayerClockView(parent, frame, atTop)
{
} /* PlayerView::PlayerView */


void PlayerView::HandleUpdate (CRect updateRect)
{
   PlayerClockView::HandleUpdate(updateRect);

   DrawPlayerIndicator();

   GAMEINFO *Info = &(gameWin->game->Info);
   CHAR *name = (Colour() == white ? Info->whiteName : Info->blackName);
   INT  elo   = (Colour() == white ? Info->whiteELO  : Info->blackELO);
   CHAR str[100];
   if (elo <= 0) CopyStr(name, str);
   else if (! name[0]) Format(str, "%d ELO", elo);
   else Format(str, "%s, %d ELO", name, elo);

   CRect r = bounds;
   r.left += 13; // Make room for side to move indicator
   r.Inset(5, 4);
   DrawStr(str, r);
} /* PlayerView::HandleUpdate */


void PlayerView::DrawPlayerIndicator (void)
{
   MovePenTo(bounds.left + 5, bounds.bottom - 8);
   if (Colour() == gameWin->game->player) DrawStr("¥");
   TextEraseTo(bounds.left + 15);
} // PlayerView::DrawPlayerIndicator

/*---------------------------------------- Clock View --------------------------------------------*/

#define hClockDivider 70

ClockView::ClockView (CViewOwner *parent, CRect frame, BOOL atTop)
 : PlayerClockView(parent, frame, atTop)
{
   CopyStr("", levelStr);
} /* ClockView::ClockView */


void ClockView::HandleUpdate (CRect updateRect)
{
   // Draw background
   PlayerClockView::HandleUpdate(updateRect);

   // Draw vertical divider
   CRect r = bounds; r.Inset(2, 2);
   if (Colour() == white)
   {  MovePenTo(r.right - hClockDivider, r.top);
      SetForeColor(&color_Gray); DrawLine(0, r.Height());
      MovePenTo(r.right - hClockDivider + 1, r.top);
      SetForeColor(&color_White); DrawLine(0, r.Height() - 1);
      SetForeColor(&color_Black);
   }
   else
   {  MovePenTo(r.right - hClockDivider, r.top);
      SetForeColor(&color_DkGray); DrawLine(0, r.Height());
      MovePenTo(r.right - hClockDivider + 1, r.top);
      SetForeColor(&color_Gray); DrawLine(0, r.Height() - 1);
      SetForeColor(&color_White);
   }

   // Draw moves to play/level:
// DrawMode();
   DrawLevel(true);

   // Draw the actual time in the format HH:MM:SS
   DrawTime();
} /* ClockView::HandleUpdate */

/*
void ClockView::DrawMode (void)
{
   if (! Visible()) return;

   CRect r(0,0,16,16); r.Offset(bounds.left + 4, bounds.top + 2);
   DrawRectFill(r, (Colour() == white ? &color_LtGray : &color_MdGray));
   SetForeColor(Colour() == white ? &color_Black : &color_White);

   if (Colour() == gameWin->game->player)
      DrawIcon(ModeIcon[gameWin->level.mode], r);
} // ClockView::DrawMode
*/

void ClockView::DrawTime (void)
{
   if (! Visible()) return;

   INT v = (! gameWin->mode3D ? bounds.bottom - 7 : bounds.bottom - 5);
   MovePenTo(bounds.right - hClockDivider + 4, v);
   DrawStr(gameWin->Clock[Colour()]->state);
} /* ClockView::DrawTime */


void ClockView::DrawLevel (BOOL redraw)  // Draw moves to play/sub level
{
   if (! Visible()) return;

   CHAR s[10];
   LEVEL *L = &gameWin->level;
   INT played = (gameWin->game->currMove + (gameWin->game->opponent == Colour() ? 1 : 0))/2;

   switch (L->mode)
   {
      case pmode_TimeMoves :
         if (L->TimeMoves.moves == allMoves)
            CopyStr("All", s);
         else
         {  INT limit  = L->TimeMoves.moves; 
            NumToStr(limit - (played % limit), s);
         }
         break;
      case pmode_Tournament :
         INT limit1 = L->Tournament.moves[0];
         INT limit2 = L->Tournament.moves[1] + limit1;
         if (played < limit1)
            NumToStr(limit1 - played, s);
         else if (played < limit2)
            NumToStr(limit2 - played, s);
         else
            CopyStr("All", s);
         break;
      case pmode_FixedDepth :
          NumToStr(L->FixedDepth.depth, s);
          break;
      case pmode_MateFinder :
          NumToStr(L->MateFinder.mateDepth, s);
          break;
      case pmode_Novice :
          NumToStr(L->Novice.level, s);
          break;
      default :
          CopyStr("", s);
   }

   if (redraw || ! EqualStr(s,levelStr))
   {
      INT v = (! gameWin->mode3D ? bounds.bottom - 7 : bounds.bottom - 5);
      MovePenTo(bounds.left + 5 /*+ 20*/, v);
      DrawStr(s); TextEraseTo(bounds.right - hClockDivider - 4);
   }

   CopyStr(s, levelStr);
} /* ClockView::DrawLevel */
