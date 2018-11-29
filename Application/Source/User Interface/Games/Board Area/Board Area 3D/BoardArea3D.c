/**************************************************************************************************/
/*                                                                                                */
/* Module  : BOARD3DVIEW.C                                                                        */
/* Purpose : This module implements the single 3D board/window. It "sits" on top of frontmost     */
/*           game window and the corresponding CGame object.                                      */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include "BoardArea3D.h"
#include "GameWindow.h"
#include "SigmaStrings.h"

#define boardHeight    (480 - 41)   // Height from top of 3D "screen" to upper edge of panel
#define panel3DHeight  28

#define board3DMemNeeded  2500    // In KBytes

/*------------------------------------- Data Structures ------------------------------------------*/

BOOL           board3Denabled = false;    // [PREF] Is also reset if not enough memory.

static CBitmap *boardBmp3D = nil;    // Offscreen copy of empty 3D board
static CView   *boardView3D = nil;
static CRect   panelRect3D(0, boardHeight, 640, boardHeight + panel3DHeight);

static CBitmap *pieceBmp3D[7][4];
static CRect   pieceRect3D[7][4][4];

static CBitmap *interBmp3D = nil;    // Temporary utility bitmap
static CView   *interView3D = nil;   // Associated view

static INT     max3DPieceHeight = 0;

static INT     left3D[9]  = { 104,97,90,81,72,63,53,42,29 };
static INT     rows3D[10] = { 56,88,123,160,199,242,289,339,393,422 };


/**************************************************************************************************/
/*                                                                                                */
/*                                  3D VIEW CONSTRUCTOR/DESCTRUCTOR                               */
/*                                                                                                */
/**************************************************************************************************/

BoardArea3DView::BoardArea3DView (CViewOwner *parent, CRect frame)
 : BoardAreaView(parent, frame)
{
   gameWin = (GameWindow*)Window();
   game = gameWin->game;

   CRect rcb(0,0,clockViewWidth,20); rcb.Offset(10, panelRect3D.top + 4);
   CRect rct(0,0,clockViewWidth,20); rct.Offset(640 - clockViewWidth - 10, panelRect3D.top + 4);
   clockViewT = new ClockView(this, rct, true);
   clockViewB = new ClockView(this, rcb, false);

   if (! Prefs.GameDisplay.show3DClocks)
      clockViewT->Show(false),
      clockViewB->Show(false);
} /* BoardArea3DView::BoardArea3DView */


void BoardArea3DView::AdjustBounds (void)
{
   CRect bounds0 = bounds;

   CRect R = theApp->ScreenRect();
   CRect r = boardBmp3D->bounds;
   INT   h = (R.Width()  - r.Width())/2;
   INT   v = (R.Height() - r.Height())/2;
   R.Offset(-h, -v);
   SetBounds(R);

   CRect rcb(0,0,clockViewWidth,20); rcb.Offset(10, panelRect3D.top + 4);
   CRect rct(0,0,clockViewWidth,20); rct.Offset(640 - clockViewWidth - 10, panelRect3D.top + 4);
   clockViewB->SetFrame(rcb, false);
   clockViewT->SetFrame(rct, false);
   clockViewB->Show(Prefs.GameDisplay.show3DClocks && ! gameWin->posEditor, false);
   clockViewT->Show(Prefs.GameDisplay.show3DClocks && ! gameWin->posEditor, false);

   if (bounds.left != bounds0.left)
   {  Window()->Move(0,0,true);
      Window()->Resize(R.Width(), R.Height());
   }
} /* BoardArea3DView::AdjustBounds */


/**************************************************************************************************/
/*                                                                                                */
/*                                          EVENT HANDLING                                        */
/*                                                                                                */
/**************************************************************************************************/

void BoardArea3DView::HandleUpdate (CRect updateRect)
{
   DrawBoard();
} /* BoardArea3DView::HandleMessage */


BOOL BoardArea3DView::HandleMouseDown (CPoint point, INT modifiers, BOOL doubleClick)
{
   if (point.v < bounds.top + 25)
   {
      sigmaApp->ClickMenuBar();
   }
   else if (gameWin->promoting)
   {
      ReadPromotion(point);
   }
   else
   {
      SQUARE sq = Point2Square(point);

      if (onBoard(sq))
      {
         if (modifiers & modifier_Command)
            ShowHelpTip(GetStr(sgr_HelpPiece, pieceType(game->Board[sq]) - 1));
         else if (modifiers & modifier_Option)
            ShowLegalMoves(sq);
         else if (! gameWin->thinking)
            PerformMove(sq, TrackMove(sq, point));
      }
      else if (gameWin->posEditor)
      {
         SQUARE to = TrackMove(sq, point);

         if (onBoard(to))
         {  game->Edit_SetPiece(to, GetSquare(sq));
            DrawSquare(to);
            sigmaApp->PlayMoveSound(false);
         }
      }
   }

   return true;
} /* BoardArea3DView::HandleMouseDown */


BOOL BoardArea3DView::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   switch (key)
   {
      case key_Return :
      case key_Enter :
         if (gameWin->promoting)
         {  promPiece = queen + game->player;
            gameWin->promoting = false;
         }
         else if (gameWin->posEditor)
         {  buttonDone->Press(true); Sleep(10); buttonDone->Press(false);
            Window()->HandleMessage(posEditor_Done);
            return true;
         }
         break;
      case key_Escape :
         if (gameWin->posEditor)
         {  buttonCancel->Press(true); Sleep(10); buttonCancel->Press(false);
            Window()->HandleMessage(posEditor_Cancel);
            return true;
         }
   }
   return false;
} /* BoardArea3DView::HandleKeyDown */


void BoardArea3DView::ClearMenu (void)
{
} /* BoardArea3DView::ClearMenu */


/**************************************************************************************************/
/*                                                                                                */
/*                                          DRAW 3D BOARD                                         */
/*                                                                                                */
/**************************************************************************************************/

// Draw entire board from scratch:

void BoardArea3DView::DrawBoard (void)
{
   AdjustBounds();
   DrawRectFill(bounds, &color_Black);  //### Use exclude rect to optimize??
   DrawAllSquares();
} /* BoardArea3DView::DrawBoard */


void BoardArea3DView::DrawAllSquares (void)
{
   //--- First draw board itself:
   CRect r = boardBmp3D->bounds;
   r.bottom = r.top + boardHeight;
   DrawBitmap(boardBmp3D, r, r);

   //--- Next draw front panel:
   DrawPanelBackground();

   //--- Draw player indicator on board edge:
   DrawPlayerIndicator();

   //--- Next draw the pieces directly (back to front):
   CRect src, dst;

   for (INT row = 7; row >= 0; row--)
      for (INT col = 0; col <= 7; col++)
         if (CBitmap *pw = CalcPieceParam(GetSquare(row,col), row, col, &src, &dst))
            DrawBitmap(pw, src, dst, bmpMode_Trans);

   //--- If setting up or promoting we have to draw some extra stuff:
   if (gameWin->posEditor)
      DrawSetupMenu();
   else
   {  DrawClockInfo();
      if (gameWin->promoting)
         DrawPromMenu();
   }
} /* BoardArea3DView::DrawAllSquares */


void BoardArea3DView::DrawPlayerIndicator (void)
{
   // Draw lower right indicator:
   CRect r1(0,0,29,19); r1.Offset(611, 419);
   INT   id1 = ((game->player == white) != gameWin->boardTurned ? 5010 : 5011);

   DrawPict(id1, r1);
   boardView3D->DrawPict(id1, r1);

   // Draw upper right indicator:
   CRect r2(0,0,18,11); r2.Offset(535, 72);
   INT   id2 = ((game->player == black) != gameWin->boardTurned ? 5012 : 5013);

   DrawPict(id2, r2);
   boardView3D->DrawPict(id2, r2);
} /* BoardArea3DView::DrawPlayerIndicator */


void BoardArea3DView::RefreshGameStatus (void)
{
} /* BoardArea3DView::RefreshGameStatus */


void BoardArea3DView::DrawPanelBackground (void)
{
   if (! Prefs.GameDisplay.show3DClocks && ! gameWin->posEditor)
      DrawBitmap(boardBmp3D, panelRect3D, panelRect3D);
   else
      DrawPict(5001, panelRect3D);
} /* BoardArea3DView::DrawPanelBackground */


void BoardArea3DView::ToggleClocks (void)
{
   if (! clockViewT || ! clockViewB) return;
   DrawPanelBackground();
   clockViewT->Show(Prefs.GameDisplay.show3DClocks && ! gameWin->posEditor, true);
   clockViewB->Show(Prefs.GameDisplay.show3DClocks && ! gameWin->posEditor, true);
} /* BoardArea3DView::ToggleClocks */


/**************************************************************************************************/
/*                                                                                                */
/*                                            DRAW 3D SQUARES                                     */
/*                                                                                                */
/**************************************************************************************************/

void BoardArea3DView::DrawSquare (SQUARE sq)
{
   if (sq == nullSq) return;
   if (gameWin->boardTurned) sq = h8 - sq;

   SQUARE3D sq3D;
   CRect destRect;
   CalcCoord(rank(sq), file(sq), &sq3D, &destRect);
   DrawArea(destRect);
} /* BoardArea3DView::DrawSquare */

/*------------------------------------ Draw Generic Area -----------------------------------------*/
// This is the general purpose 3D board drawing routine used for drawing any subset of
// the 3D board (using the temporary graphics world interWorld), e.g. when drawing a
// single square, or when drawing piece movement. "destRect" is the area to be drawn
// (in local 3D board world coordinates - for screens larger than the standard 640x480,
// the actual screen destination rectangle in the 3D board window will be different).
// If a "temporary" unsynchronized piece is to be drawn in connection with piece movement,
// "tempPiece" will indicate that piece, and "pieceRect" indicates the destination rectangle
// (again in local 3D board world coordinates).

void BoardArea3DView::DrawArea (CRect destRect, CBitmap *tempPw, CRect tempSrc, CRect tempDst)
{
   INT    row, col;      // Current row and col (disregarding boardTurned).
   CRect  intRect,       // Relevant part of interWorld (normalized version of destRect).
          src,           // Full piece source rectangle (pieceBmp3D[p][i]).
          dst,           // Full piece destination rectangle (boardWorld/board3DWind).
          dstC,          // Clipped destination rectangle (dst clipped to destRect).
          srcC,          // Clipped source rectangle.
          dstCi;         // Clipped interWorld destination rectangle.

   // First "clear" area by copying destRect from the empty board world:

   intRect = destRect; intRect.Normalize();
   interView3D->DrawBitmap(boardBmp3D, destRect, intRect);

   // For each rank that intersects destRect (most distant rank first), we scan each non-empty
   // square to see if it also intersects destRect. If so, we draw the piece on that square.
   
   for (row = 7; row >= 0; row--)
      if (rows3D[8 - row] > destRect.top && rows3D[8 - row] - max3DPieceHeight < destRect.bottom)
      {
         if (tempPw && tempDst.bottom < rows3D[8 - row] - 10)
            DrawTempPiece(destRect, tempPw, tempSrc, tempDst),
            tempPw = nil;                                          // Don't test this again!

         for (col = -1; col <= 8; col++)
         {
            CBitmap *pw = CalcPieceParam(GetSquare(row,col), row, col, &src, &dst);

            if (pw != nil && dstC.Intersect(&destRect,&dst))
            {
               srcC  = dstC; srcC.Offset(src.left - dst.left, src.top - dst.top);
               dstCi = dstC; dstCi.Offset(-destRect.left, -destRect.top);
               interView3D->DrawBitmap(pw, srcC, dstCi, bmpMode_Trans);
            }
         }

         if (tempPw && row == 0)
            DrawTempPiece(destRect, tempPw, tempSrc, tempDst),
            tempPw = nil;                                          // Don't test this again!
      }

   DrawBitmap(interBmp3D, intRect, destRect);

} /* BoardArea3DView::DrawArea */


void BoardArea3DView::DrawTempPiece (CRect destRect, CBitmap *tempPw, CRect tempSrc, CRect tempDst)
{
   tempDst.Offset(-destRect.left, -destRect.top);
   interView3D->DrawBitmap(tempPw, tempSrc, tempDst, bmpMode_Trans);
} /* BoardArea3DView::DrawTempPiece */


/**************************************************************************************************/
/*                                                                                                */
/*                                         DRAW 3D PIECE MOVEMENT                                 */
/*                                                                                                */
/**************************************************************************************************/

// It is assumed that the piece has already been moved, but that any captured piece has been
// replaced to the destination square. If the move is not a capture, the destination square
// is cleared before drawing the move, and is restored afterwards.

#define delta3D 2      // Pixels to move in each "frame"

void BoardArea3DView::DrawPieceMovement (PIECE piece, PIECE target, SQUARE from, SQUARE to)
{
   CRect   src0, dst0, src1, dst1, destRect;
   INT     row, col;
   CPoint  p, p0, p1;
   CBitmap *pw;
   SQUARE  to0 = to;

   PIECE tmpPiece = game->Board[to0];       // Temporarily change contents of target square.
   game->Board[to0] = target;

   if (gameWin->boardTurned)
      from = h8 - from,
      to   = h8 - to;

   CalcPieceParam(piece, rank(from), file(from), &src0, &dst0);
   CalcPieceParam(piece, rank(to),   file(to),   &src1, &dst1);
   p0.h = (dst0.left + dst0.right)/2; p0.v = dst0.bottom;
   p1.h = (dst1.left + dst1.right)/2; p1.v = dst1.bottom;

   LONG steps = Max(Abs(p1.h - p0.h), Abs(p1.v - p0.v))/delta3D;
   UInt64 tm0, delay, tm;
   delay = 300 + 150*(100 - Prefs.Games.moveSpeed) - steps;

   for (LONG s = 0; s < steps; s++)
   {
      MicroSecs(&tm0);
      tm0 += delay;

      p.h = p0.h + ((p1.h - p0.h)*s)/steps;
      p.v = p0.v + ((p1.v - p0.v)*s)/steps;
      Point2RowCol(p, &row, &col);
      pw = CalcPieceParam(piece, row, col, &src0, &dst0);

      dst0.Normalize();
      dst0.Offset(p.h - dst0.right/2, p.v - dst0.bottom);
      destRect = dst0;
      destRect.Inset(-10, -10);

      DrawArea(destRect, pw, src0, dst0);
      FlushPortBuffer(&destRect);

      do MicroSecs(&tm); while (tm <= tm0);
   }

   game->Board[to0] = tmpPiece;             // Restore contents of target square.

   DrawSquare(to0);
   FlushPortBuffer();
} /* BoardArea3DView::DrawPieceMovement */


/**************************************************************************************************/
/*                                                                                                */
/*                                     TRACKING 3D PIECE MOVEMENT                                 */
/*                                                                                                */
/**************************************************************************************************/

// The tracking algorithm works as follows: When the mouse is moved, the new destination
// rectangle dst1 is compared with the previous destination rectangle dst0. If they intersect
// we redraw the 3D area covered by the union of the 2 rectangles (with the piece inserted).
// If the rectangles do not intersect, we first draw the 3D area covered by dst0 and then
// the area covered by dst1 (with the piece inserted).

SQUARE BoardArea3DView::TrackMove (SQUARE from, CPoint p0)
{
   PIECE piece = GetSquare(from);                        // Clear source square.

   if (! game->editingPosition && ! gameWin->promoting)
   {
      if (piece == empty || pieceColour(piece) != game->player) return nullSq;

      INT i;
      for (i = 0; i < game->moveCount && game->Moves[i].from != from; i++);
      if (i == game->moveCount) return nullSq;
   }

   if (onBoard(from)) game->Board[from] = empty;

   SQUARE to = nullSq;
   INT    row, col;
   CRect  src, dst0, dst1, u;
   BOOL   done = false;

   Square2RowCol(from, &row, &col);
   CBitmap *pw = CalcPieceParam(piece, row, col, &src, &dst0);
   CPoint base((dst0.left + dst0.right)/2, dst0.bottom);

   sigmaApp->ShowHideCursor(false);                      // Hide cursor while tracking...

   while (! done)                                        // Track piece movement.
   {
      CPoint p;
      MOUSE_TRACK_RESULT trackResult;
      BOOL moveAborted = false;

      do
      {
         TrackMouse(&p, &trackResult);
         KeyMap theKeys; ::GetKeys(theKeys);
         if (theKeys[1] & 0x00002000) moveAborted = true;
      } while (p.Equal(p0) && trackResult != mouseTrack_Released && ! moveAborted);

      base.h += p.h - p0.h;                              // Calc new destination base point. 
      base.v += p.v - p0.v;

      if (moveAborted || ! Point2RowCol(base,&row,&col) ||
          (offBoard(RowCol2Square(row,col)) && RowCol2Square(row,col) != from))
      {
         // If the piece has been moved off the board, then put back the piece on the origin
         //   square, clear previous drawn "instance" and return the nullSq.

         if (onBoard(from)) game->Board[from] = piece;
         DrawArea(dst0);
         sigmaApp->ShowHideCursor(true);
         return nullSq;
      }

      if (! p.Equal(p0))
      {
         // If the mouse is still down and it has been moved, then compute the new destination
         // rectangle.
      
         pw = CalcPieceParam(piece, row, col, &src, &dst1);
         dst1 = src;
         dst1.Offset(base.h - (src.left + src.right)/2, base.v - src.bottom);
      }
      else
      {
         // If the mouse button was released on the board, then compute destination rectangle
         // and square, and set the done flag.

         pw = CalcPieceParam(piece, row, col, &src, &dst1);
         to = RowCol2Square(row, col);
         done = true;
      }

      if (u.Intersect(&dst0, &dst1))
         u.Union(&dst0, &dst1);
      else
         DrawArea(dst0),
         u = dst1;

      DrawArea(u, pw, src, dst1);

      p0 = p; dst0 = dst1;
   }

   if (onBoard(from)) game->Board[from] = piece;

   sigmaApp->ShowHideCursor(true);               // Show cursor again.

   return to;
} /* BoardArea3DView::TrackMove */


/**************************************************************************************************/
/*                                                                                                */
/*                                         3D PROMOTION MENU                                      */
/*                                                                                                */
/**************************************************************************************************/

// The 3D promotion menu simply consist of a Queen, Rook, Bishop and Knight lined up at the
// front of the board. The promotion is performed by the user by simply clicking on the
// desired target piece.

PIECE BoardArea3DView::AskPromPiece (SQUARE from, SQUARE to)
{
   sigmaApp->EnableMenuBar(false);
   gameWin->promoting  = true;
   promSquare = to;
   DrawPromMenu();

   game->Board[from] = empty;
   game->Board[to]   = pawn + game->player;

   LONG T = Timer() + 30;
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
   } while (gameWin->promoting);

   sigmaApp->EnableMenuBar(true);
   gameWin->promoting = false;
   DrawBoard();

   return promPiece;
} /* BoardArea3DView::AskPromPiece */


void BoardArea3DView::DrawPromMenu (void)
{
   CRect src, dst;

   if (! gameWin->promoting) return;

   for (INT row = 7; row >= 4; row--)
      for (INT col = -1; col <= 8; col += 9)
         if (CBitmap *pw = CalcPieceParam(GetSquare(row,col), row, col, &src, &dst))
            DrawBitmap(pw, src, dst, bmpMode_Trans);
} /* BoardArea3DView::DrawPromMenu */


void BoardArea3DView::ReadPromotion (CPoint p0)
{
   SQUARE from, to;

   game->Board[promSquare] = pawn + game->player;
   DrawSquare(promSquare);

   from = Point2Square(p0);
   if (from == nullSq || onBoard(from)) return;

// sigmaApp->ShowHideCursor(false);  Doesn't work in OS X!!! (TrackMove does the work for us anyway)

      to = TrackMove(from, p0);
      if (promSquare == to)
         promPiece = GetSquare(from),
         gameWin->promoting = false;
      if (onBoard(to) && to != from)
         DrawSquare(to);

// sigmaApp->ShowHideCursor(true);  Doesn't work in OS X!!! (cursor disappears)
} /* BoardArea3DView::ReadPromotion */


/**************************************************************************************************/
/*                                                                                                */
/*                                            3D SETUP MENU                                       */
/*                                                                                                */
/**************************************************************************************************/

void BoardArea3DView::ShowPosEditor (BOOL showPos)
{
   if (gameWin->posEditor)
   {
      INT w = 80;
      CRect r = panelRect3D;
      r.Inset(10, 3);
      r.left = r.right - w;

      buttonDone   = new CButton(this, r,posEditor_Done,0,true,true,"Done","Exit Position Editor and store the new position."); r.Offset(-w-5, 0);
      buttonCancel = new CButton(this, r,posEditor_Cancel,0,true,true,"Cancel","Exit Position Editor and restore the previous position."); r.Offset(-w-5, 0);
      buttonStatus = new CButton(this, r,posEditor_Status,0,true,true,"Status...","Set initial position status: Castling rights, EP status, 50 move rule etc."); r.Offset(-w-5, 0);
      buttonNew    = new CButton(this, r,posEditor_NewBoard,0,true,true,"New Board","Setup all pieces in their initial position."); r.Offset(-w-5, 0);
      buttonClear  = new CButton(this, r,posEditor_ClearBoard,0,true,true,"Clear Board","Remove all pieces from the board."); r.Offset(-r.left + 10, 0);
      buttonWhite  = new CButton(this, r,posEditor_SelectPlayer,white,true,true,"White","Set WHITE to move in the current board position."); r.Offset(w+5, 0);
      buttonBlack  = new CButton(this, r,posEditor_SelectPlayer,black,true,true,"Black","Set BLACK to move in the current board position.");
      if (game->player == white)
          buttonWhite->Press(true);
      else
          buttonBlack->Press(true);
   }
   else
   {
      delete buttonDone;
      delete buttonCancel;
      delete buttonStatus;
      delete buttonNew;
      delete buttonClear;
      delete buttonBlack;
      delete buttonWhite;
   }

   clockViewT->Show(Prefs.GameDisplay.show3DClocks && ! gameWin->posEditor);
   clockViewB->Show(Prefs.GameDisplay.show3DClocks && ! gameWin->posEditor);

   DrawBoard();
} /* BoardArea3DView::ShowPosEditor */


void BoardArea3DView::DrawSetupMenu (void)
{
   if (! gameWin->posEditor) return;

   for (INT row = 7; row >= 2; row--)
      for (INT col = -1; col <= 8; col += 9)
      {  CRect src, dst;
         CBitmap *pw = CalcPieceParam(GetSquare(row,col), row, col, &src, &dst);
         DrawBitmap(pw, src, dst, bmpMode_Trans);
      }

   buttonDone->Redraw();
   buttonCancel->Redraw();
   buttonStatus->Redraw();
   buttonNew->Redraw();
   buttonClear->Redraw();
   buttonWhite->Redraw();
   buttonBlack->Redraw();
} /* BoardArea3DView::DrawSetupMenu */


void BoardArea3DView::SelectPlayer (COLOUR player)
{
   buttonWhite->Press(player == white);
   buttonBlack->Press(player == black);
   game->Edit_SetPlayer(player);
} /* BoardArea3DView::SelectPlayer */


/**************************************************************************************************/
/*                                                                                                */
/*                                               UTILITY                                          */
/*                                                                                                */
/**************************************************************************************************/

BOOL BoardArea3DView::Point2RowCol (CPoint p, INT *row, INT *col)
{
   INT dh, h0;

   p.v -= 10;
   if (p.v < rows3D[0] || p.v > rows3D[8] || p.h < 10 || p.h > 630)
      return false;

   for (*row = 7; p.v > rows3D[8 - (*row)]; (*row)--);

   dh = (640 - 2*left3D[8 - (*row)])/8;
   h0   = left3D[8 - (*row)];

   if (p.h < h0 - dh) *col = -2;
   else if (p.h < h0) *col = -1;
   else *col = ((p.h - h0)*8)/(640 - 2*h0);

   if (gameWin->posEditor && *row >= 2 || gameWin->promoting && *row >= 4)
      return (*col >= -1 && *col <= 8);
   else
      return (*col >= 0 && *col <= 7);
} /* BoardArea3DView::Point2RowCol */


SQUARE BoardArea3DView::Point2Square (CPoint pt)

// This is more complex than in the 2D case, because the user may need to click at the top of a
// piece to select/move it, and this top part will always overlap visually into the square in
// front (above), and for tall pieces even two squares in front.

{
   INT      row, col;
   SQUARE   sq, sq1, sq2, dir;
   CRect    src, dst;
   CPoint   p;
   CBitmap  *pw;

   p = pt; p.v = Max(rows3D[0] + 10, p.v);   //###

   if (! Point2RowCol(p,&row,&col))
      return nullSq;
   else
   {
      sq = RowCol2Square(row, col);
      dir = (onBoard(sq) && gameWin->boardTurned ? 0x10 : -0x10);
      sq1 = sq + dir;
      sq2 = sq1 + dir;

      if (row >= 2 && GetSquare(row - 2,col))
      {  pw = CalcPieceParam(GetSquare(row - 2,col), row - 2, col, &src, &dst);
         dst.Inset(5, 0);
         if (pt.InRect(dst)) return sq2;
      }

      if (row >= 1 && GetSquare(row - 1,col))
      {  pw = CalcPieceParam(GetSquare(row - 1,col), row - 1, col, &src, &dst);
         dst.Inset(5, 0);
         if (pt.InRect(dst)) return sq1;
      }

      return sq;
   }
} /* BoardArea3DView::Point2Square3D */


void BoardArea3DView::CalcCoord (INT rank, INT file, SQUARE3D *sq3D, CRect *dest)
{
   sq3D->top         = rows3D[7-rank];
   sq3D->bottom      = rows3D[8-rank];
   sq3D->topLeft     = left3D[7-rank] + ((640 - 2*left3D[7-rank])*file)/8;
   sq3D->topRight    = left3D[7-rank] + ((640 - 2*left3D[7-rank])*(file + 1))/8;
   sq3D->bottomLeft  = left3D[8-rank] + ((640 - 2*left3D[8-rank])*file)/8;
   sq3D->bottomRight = left3D[8-rank] + ((640 - 2*left3D[8-rank])*(file + 1))/8;

   if (dest != nil)
   {  dest->left     = Min(sq3D->topLeft,sq3D->bottomLeft) - 2;
      dest->right    = Max(sq3D->topRight,sq3D->bottomRight) + 2;
      dest->top      = sq3D->bottom - max3DPieceHeight;
      dest->bottom   = sq3D->bottom + 2;
   }
} /* BoardArea3DView::Calc3DCoord */


void BoardArea3DView::FrameSquare (SQUARE sq, RGB_COLOR *color)      // Frames a 3D square with the current pen.
{
   SQUARE3D sq3D;

   boardView3D->SetForeColor(color);
   boardView3D->SetPenSize(2, 2);

   if (gameWin->boardTurned) sq = h8 - sq;
   CalcCoord(rank(sq), file(sq), &sq3D, nil);
// if (rank(sq) == 0) sq3D.bottom -= 4;

   boardView3D->MovePenTo(sq3D.topLeft, sq3D.top);
   boardView3D->DrawLineTo(sq3D.topRight - 1, sq3D.top);
   boardView3D->DrawLineTo(sq3D.bottomRight - 1, sq3D.bottom - 1);
   boardView3D->DrawLineTo(sq3D.bottomLeft, sq3D.bottom - 1);
   boardView3D->DrawLineTo(sq3D.topLeft, sq3D.top);

   boardView3D->SetForeColor(&color_Black);
   boardView3D->SetPenSize(1, 1);

   DrawSquare(sq);
} /* BoardArea3DView::FrameSquare */


void BoardArea3DView::ClearShowLegal (void)
{
   boardBmp3D->LoadPicture(5000 + Prefs.Appearance.boardType3D);
} /* BoardArea3DView::ClearShowLegal */


CBitmap *BoardArea3DView::CalcPieceParam (PIECE piece, INT row, INT col, CRect *src, CRect *dst)

// Given a piece and board position (row,col), this function calculates the source
// graphics and rectangle as well as the destination rectangle. If "piece" is empty
// (or edge) then nil is returned and no rectangles are calculated.

{
   SQUARE3D  sq3D;
   INT       col0 = col;

   if (piece == empty || piece == edge) return nil;

   if (col < 0) col = 0;
   if (col > 7) col = 7;

   INT p = 7 - pieceType(piece);

   CalcCoord(row, col, &sq3D, nil);
   INT i = 3 - row/2;
   INT j = col/2;

   *src = pieceRect3D[p][i][j];
   if (pieceColour(piece) == black)
      src->Offset(pieceRect3D[p][i][3].right + 1, 0);
   *dst = *src;
   dst->Normalize();

   INT dh = (sq3D.bottomLeft + sq3D.bottomRight - dst->right)/2 + 2 - j;
   INT dv = (sq3D.bottom - dst->bottom) - 9 + (3*row)/5 - p/2;
   if (col0 < 0) dh -= (sq3D.bottomRight - sq3D.bottomLeft);
   if (col0 > 7) dh += (sq3D.bottomRight - sq3D.bottomLeft);   
   if (p == 3) dv -= 3; // Rooks have small bases
   dst->Offset(dh, dv);

   return pieceBmp3D[p][i];
} /* BoardArea3DView::CalcPieceParam */

/*---------------------------------- Accessing Board/Squares --------------------------------*/
// In setup mode and promotions, the 3D board is extended with pieces on the left/right board
// edges (independent of board orientation). I.e. for row in [2..7] and col -1 or 8. These
// speciel "board edge squares" are encoded into the SQUARE type via rank values above 8 (i.e.
// a9...a16 for the column left of column a, and h9...h16 for the column right of column h).

PIECE BoardArea3DView::GetSquare (INT row, INT col)
{
   SQUARE sq;

   if (col >= 0 && col <= 7)      // I.e. normal board square...
   {  sq = (row << 4) + col;
      return game->Board[gameWin->boardTurned ? h8 - sq : sq];
   }
   else if (gameWin->posEditor && row >= 2)
   {  return (king + row - 7) + (col < 0 ? white : black);
   }
   else if (gameWin->promoting && row >= 4 && (col == (game->player ? 8 : -1)))
   {  return (queen + row - 7) + game->player;
   }
   return edge;
} /* BoardArea3DView::GetSquare */


PIECE BoardArea3DView::GetSquare (SQUARE sq)
{
   INT row, col;

   Square2RowCol(sq, &row, &col);
   return GetSquare(row, col);
} /* BoardArea3DView::GetSquare */


void BoardArea3DView::Square2RowCol (SQUARE sq, INT *row, INT *col)
{
   if (onBoard(sq) && gameWin->boardTurned)
      sq = h8 - sq;
   *row = rank(sq);
   *col = file(sq);
   if (*row > 7)
   {  *row -= 8;
      *col = (*col == 0 ? -1 : 8);
   }
} /* BoardArea3DView::Square2RowCol */


SQUARE BoardArea3DView::RowCol2Square (INT row, INT col)
{
   SQUARE sq = (row << 4);

   switch (col)
   {  case -1 : sq += 0x80; break;
      case 8  : sq += 0x87; break;
      default : sq += col; if (gameWin->boardTurned) sq = h8 - sq;
   }
   return sq;
} /* BoardArea3DView::RowCol2Square */


/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION                                   */
/*                                                                                                */
/**************************************************************************************************/

#define bitDepth3D 16

static BOOL Load3DPiece (INT p, INT n);

void InitBoard3DModule (void)
{
   if (! Prefs.General.enable3D) return;
   if (! sigmaApp->CheckMemFree(board3DMemNeeded, false)) return;

   // First create and load all offscreen bitmaps:
   boardBmp3D = new CBitmap(5000 + Prefs.Appearance.boardType3D, bitDepth3D);
   boardView3D = new CView(boardBmp3D, boardBmp3D->bounds);

   interBmp3D = new CBitmap(140, 140, bitDepth3D);
   interView3D = new CView(interBmp3D, interBmp3D->bounds);

   for (INT p = 1; p <= 6; p++)
      for (INT n = 0; n < 4; n++)
         Load3DPiece(p, n);

   // Adjust various coordinates:
   for (INT i = 0; i <= 9; i++) rows3D[i] += 27;

   board3Denabled = true;
   
} /* InitBoard3DModule */


#define redPixel(c)   (c.red == 0xFFFF && c.green == 0 && c.blue == 0)

static BOOL Load3DPiece (INT p, INT n)
{
   pieceBmp3D[p][n] = nil;

   INT id = 5000 + 100*p + n;
   CBitmap *bmp = new CBitmap(id, bitDepth3D);
   if (! bmp->createdOK)
   {  delete bmp;
      return false;
   }

	max3DPieceHeight = Max(max3DPieceHeight, bmp->bounds.Height() + 15);

   CView *bview = new CView(bmp, bmp->bounds);

   INT h = 0;
   for (INT i = 0; i < 4; i++)
   {
      RGB_COLOR pcol;
      pieceRect3D[p][n][i] = bmp->bounds;
      pieceRect3D[p][n][i].left = h;
      h += 20;
      do bview->GetPixelColor(h++, 0, &pcol); while (! redPixel(pcol));
      pieceRect3D[p][n][i].right = h - 1;
   }

   delete bview;

   pieceBmp3D[p][n] = bmp;

   return true;
} /* Load3DPiece */
