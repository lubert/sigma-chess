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

#include "BoardView.h"
#include "GameWindow.h"
#include "SigmaApplication.h"
#include "SigmaStrings.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                    BOARD VIEW CONSTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

BoardView::BoardView(CViewOwner *parent, CRect frame, CGame *theGame,
                     GameWindow *theGameWin)
    : DataView(parent, frame, false) {
  game = theGame;
  gameWin = theGameWin;
  markMove = false;
  CalcFrames();
} /* BoardView::BoardAreaView */

void BoardView::CalcFrames(void) {
  squareWidth = (gameWin ? gameWin->squareWidth : minSquareWidth);
  frameWidth = BoardFrame_Width(squareWidth);
  boardRect = bounds;
  boardRect.Inset(frameWidth, frameWidth);  // Calc actual board rectangle
} /* BoardView::CalcFrames */

INT BoardFrame_Width(INT sqWidth) {
  return (2 * (sqWidth + 1)) / 5;
} /* BoardFrameWidth */

/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void BoardView::HandleUpdate(CRect updateRect) {
  DataView::HandleUpdate(updateRect);
  DrawFrame();

  CRect sect;
  if (sect.Intersect(&updateRect, &boardRect))
    for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq)) {
        sect = SquareToRect(sq);
        if (sect.Intersect(&updateRect, &boardRect)) DrawSquare(sq);
      }
} /* BoardView::HandleUpdate */

void BoardView::HandleResize(void) {
  CalcFrames();
} /* BoardView::HandleResize */

/**************************************************************************************************/
/*                                                                                                */
/*                                         DRAWING BOARD */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- Draw Board Frame
 * ---------------------------------------*/

void BoardView::DrawFrame(void) {
  CRect r = bounds, ri, rf;

  RGB_COLOR c1 = boardFrameColor[2];
  RGB_COLOR c2 = boardFrameColor[2];
  AdjustColorLightness(&c1, +5);
  AdjustColorLightness(&c2, -5);

  // Draw 3D frame around board frame
  r.Inset(1, 1);
  Draw3DFrame(r, &boardFrameColor[1], &boardFrameColor[3]);
  r.Inset(1, 1);
  Draw3DFrame(r, &boardFrameColor[2], +10, -10);

  // Draw frame interior:
  r.Inset(1, 1);
  ri = r;
  ri.Inset(frameWidth - 4, frameWidth - 4);
  rf.Set(r.left, r.top, r.right, ri.top);
  DrawRectFill(rf, &boardFrameColor[2]);  // Top
  rf.Set(r.left, ri.bottom, r.right, r.bottom);
  DrawRectFill(rf, &boardFrameColor[2]);  // Bottom
  rf.Set(r.left, ri.top, ri.left, ri.bottom);
  DrawRectFill(rf, &boardFrameColor[2]);  // Left
  rf.Set(ri.right, ri.top, r.right, ri.bottom);
  DrawRectFill(rf, &boardFrameColor[2]);  // Right
  r.Inset(frameWidth - 6, frameWidth - 6);

  // Draw 3D frame inside board frame
  Draw3DFrame(r, &boardFrameColor[2], -10, +10);
  r.Inset(1, 1);
  Draw3DFrame(r, &boardFrameColor[3], &boardFrameColor[1]);

  // Draw black frame
  r.Inset(1, 1);
  SetForeColor(&color_Black);
  DrawRectFrame(r);

  // Draw file/rank designators (using the light square color):
  if (Prefs.Notation.moveNotation != moveNot_Descr) {
    SetForeColor(&boardFrameColor[0]);
    SetBackColor(&boardFrameColor[2]);
    SetFontMode(fontMode_Or);
    SetFontSize(squareWidth == minSquareWidth ? 10 : 12);

    for (INT i = 0; i <= 7; i++) {
      MovePenTo(boardRect.left + i * squareWidth + squareWidth / 2 - 2,
                bounds.bottom - (frameWidth - 7) / 2);
      DrawChr('a' + (BoardTurned() ? 7 - i : i));

      MovePenTo(bounds.left + (frameWidth - 7) / 2,
                boardRect.top + i * squareWidth + squareWidth / 2 + 3);
      DrawChr('1' + (BoardTurned() ? i : 7 - i));
    }

    SetFontMode(fontMode_Copy);
    SetFontSize(10);
  }

  DrawPlayerIndicator();

  // Finally restore fore/back colors:
  SetForeColor(&color_Black);
  SetBackColor(&color_White);
} /* BoardView::DrawFrame */

/*------------------------------------ Draw Side to Move Indicator
 * -------------------------------*/

void BoardView::DrawPlayerIndicator(void) {
  if (!gameWin) return;

  CRect r = bounds;
  r.Inset(6, 7);
  r.left = r.right - 8;

  CRect ron = r;
  CRect roff = r;
  if (BoardTurned() == (game->player == white))
    ron.bottom = ron.top + 5, roff.top = roff.bottom - 5;
  else
    roff.bottom = roff.top + 5, ron.top = ron.bottom - 5;

  Draw3DFrame(ron, &boardFrameColor[3], &boardFrameColor[1]);
  ron.Inset(1, 1);
  DrawRectFill(ron, &color_Yellow);
  ron.Inset(1, 1);
  DrawRectFill(ron, &color_White);

  Draw3DFrame(roff, &boardFrameColor[3], &boardFrameColor[1]);
  roff.Inset(1, 1);
  Draw3DFrame(roff, &color_Gray, &color_DkGray);
  roff.Inset(1, 1);
  DrawRectFill(roff, &color_MdGray);

  SetForeColor(&color_Black);
  SetBackColor(&color_White);
} /* BoardView::DrawPlayerIndicator */

/*---------------------------------------- Draw Squares
 * ------------------------------------------*/

void BoardView::DrawAllSquares(void) {
  for (SQUARE sq = a1; sq <= h8; sq++)
    if (onBoard(sq)) DrawSquare(sq);
} /* BoardView::DrawAllSquares*/

void BoardView::DrawSquare(SQUARE sq) {
  CRect r = SquareToRect(sq);
  DrawPieceAt(game->Board[sq], &r, &r);

  if (Prefs.GameDisplay.moveMarker > 0 && markMove) {
    MOVE *m = &(game->Record[game->currMove]);
    if (sq == m->from || sq == m->to) {
      RGB_COLOR hiColor = color_Red;
      FrameSquare(sq, &hiColor);
    }
  }
} /* BoardView::DrawSquare */

void BoardView::SetMoveMarker(BOOL engineMove) {
  if (game->currMove == 0) return;

  ClearMoveMarker();
  if (!engineMove && Prefs.GameDisplay.moveMarker == 1) return;
  markMove = true;
  if (Prefs.GameDisplay.moveMarker > 0) {
    RGB_COLOR hiColor = color_Red;
    MOVE *m = &(game->Record[game->currMove]);
    FrameSquare(m->from, &hiColor);
    FrameSquare(m->to, &hiColor);
  }
} /* BoardView::SetMoveMarker */

void BoardView::ClearMoveMarker(void) {
  if (!markMove || game->currMove == 0) return;

  markMove = false;
  if (Prefs.GameDisplay.moveMarker > 0) {
    MOVE *m = &(game->Record[game->currMove]);
    DrawSquare(m->from);
    DrawSquare(m->to);
  }
} /* BoardView::ClearMoveMarker */

/**************************************************************************************************/
/*                                                                                                */
/*                                           DRAWING MOVES */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Draw Piece Movement
 * ---------------------------------------*/
// It is assumed that the piece has already been moved on the board, i.e. the
// "from" square is empty and that the "to" square contains the piece (which in
// case of promotions is NOT equal to the moving piece "p", which is a pawn!).

void BoardView::DrawPieceMovement(PIECE piece, PIECE target, SQUARE from,
                                  SQUARE to) {
  INT dx, dy;  // Square and pixel displacements.
  INT steps;   // Loop counter.
  CRect r1, r2, u;

  dx = file(to) - file(from);  // Calc square/pixel displacements.
  dy = rank(from) - rank(to);
  if (BoardTurned()) dx = -dx, dy = -dy;

  if (pieceType(piece) == knight)  // Calc draw loop count.
    steps = squareWidth;
  else {
    steps = (Max(Abs(dx), Abs(dy)) * squareWidth) / 2;
    dx = Sign(dx) * 2;
    dy = Sign(dy) * 2;
  }

  PIECE tmpPiece =
      game->Board[to];  // Temporarily change contents of target square.
  game->Board[to] = target;

  UInt64 tm0, delay, tm;
  delay = 300 + 150 * (100 - Prefs.Games.moveSpeed) - steps;

  r2 = SquareToRect(from);
  while (--steps > 0)  // Draw the piece movement
  {
    MicroSecs(&tm0);
    tm0 += delay;

    r1 = r2;
    r2.Offset(dx, dy);
    u.Union(&r1, &r2);
    DrawPieceAt(piece, &u, &r2);
    FlushPortBuffer(&u);

    do
      MicroSecs(&tm);
    while (tm <= tm0);
  }

  game->Board[to] = tmpPiece;  // Restore contents of target square.

  DrawSquare(to);  // Finally draw destination square
  FlushPortBuffer();
} /* BoardView::DrawPieceMovement */

/**************************************************************************************************/
/*                                                                                                */
/*                                      RESPONDING TO MOUSE */
/*                                                                                                */
/**************************************************************************************************/

// When the user clicks in the board view we have to consider the following 4
// scenarios:
//
// (1) The user wants to perform a move by drag and drop. If the user is editing
// the current
//     position, any movement is allowed.
// (2) The user wants to place/remove a piece when editing the position.
// (3) The user wants to see the legal moves of the selected piece (by holding
// down the alt-key). (4) The user wants to to change piece set/board set via a
// popup menu (by holding down the
//     control key = right mouse??).

BOOL BoardView::HandleMouseDown(CPoint point, INT modifiers, BOOL doubleClick) {
  SQUARE from = PointToSquare(point);  // Convert click to square

  if (offBoard(from))  // If off board (i.e. on board frame) then show board
  {                    // frame popup menu...
    //    if (modifiers & modifier_Control)    //## Allows user to (a) Turn
    //    board, (b) toggle
    //       BoardFramePopup();                // file/rank letters, (c) select
    //       frame type.
  } else  // If on board then dispatch action (normally a move by
  {       // drag and drop):
          //    if (gameWin) DebugSquare(gameWin, from);
    if (modifiers & modifier_Command)
      ShowHelpTip(GetStr(sgr_HelpPiece, pieceType(game->Board[from]) - 1));
    else if (modifiers & modifier_Control)
      if (game->Board[from] == empty)
        BoardTypePopup();
      else
        PieceSetPopup();
    else if (modifiers & modifier_Option)
      ShowLegalMoves(from);
    else if (!gameWin || !gameWin->thinking)
      TrackMove(from, point);
    else
      Beep(1);
  }

  return true;
} /* BoardView::HandleMouseDown */

/*---------------------------------- Track Piece Movement
 * ----------------------------------------*/

static void ConfineRect(CRect *r, CRect *R);

void BoardView::TrackMove(SQUARE from, CPoint p1) {
  PIECE piece;  // The piece being dragged.
  SQUARE to;
  CRect r1, r2, u, s;  // The destination rectangle where the piece is drawn.
  CPoint p2;
  INT i;

  piece = game->Board[from];

  if (!game->editingPosition) {
    if (piece == empty) return;
    if (pieceColour(piece) != game->player) {
      Beep(1);
      return;
    }

    for (i = 0; i < game->moveCount && game->Moves[i].from != from; i++)
      ;
    if (i == game->moveCount) {
      Beep(1);
      return;
    }
  } else {
    if (piece == empty) {
      game->Edit_SetPiece(from, game->editPiece);
      DrawSquare(from);
      sigmaApp->PlayMoveSound(false);
      return;
    }
  }

  BOOL tracking = true;  // Are we still tracking?
  BOOL moveAborted = false;
  ULONG lastDrawTime = 0;

  sigmaApp->ShowHideCursor(false);  // Hide cursor while tracking...
  ClearMoveMarker();                // Also hide any move marker

  r1 = r2 = SquareToRect(from);  // Calc source square (and initial dest).
  game->Board[from] = empty;  // Temporarily remove moving piece from source sq.

  while (tracking && !moveAborted)  // BEGIN TRACKING
  {
    GetMouseLoc(&p2);
    MOUSE_TRACK_RESULT trackResult;
    TrackMouse(&p2, &trackResult);

    if (::GetCurrentKeyModifiers() & modifier_Command)
      moveAborted = true;
    else if (trackResult ==
             mouseTrack_Released)  // If primary mouse button no longer pressed
      tracking = false;            // (alone) we are done tracking...
    else if (!p1.Equal(p2))        // Otherwise if the mouse has been moved...
    {
      r2 = r1;  // Move the piece rectangle accordingly...
      r2.Offset(p2.h - p1.h, p2.v - p1.v);
      ConfineRect(&r2,
                  &boardRect);  // ...and confine it within board rectangle.

      if (s.Intersect(&r1, &r2))  // If the new destination rectangle intersects
        u.Union(&r1, &r2),        // (1) Compute union u.
            DrawPieceAt(piece, &u,
                        &r2);  // (2) Draw the piece at r2 ("inside" u)
      else
        DrawPieceAt(empty, &r1, &r1), DrawPieceAt(piece, &r2, &r2);

      FlushPortBuffer();
      lastDrawTime = Timer();

      p1 = p2;
      r1 = r2;
    }
    /*
          else if (Timer() > lastDrawTime + 30)
          {
             FlushPortBuffer();
             Task_Switch();
          }
    */
  }  // END TRACKING

  // Calc dest square and fit the piece to that square:

  to =
      PointToSquare(CPoint((r1.left + r1.right) / 2, (r1.top + r1.bottom) / 2));
  r2 = SquareToRect(to);
  u.Union(&r1, &r2);
  DrawPieceAt(piece, &u, &r2);

  game->Board[from] = piece;  // Restore moving piece to source square.

  sigmaApp->ShowHideCursor(true);  // Show cursor again.

  // Finally check if move is legal, and if so perform it...
  if (!moveAborted) {
    if (gameWin)
      ((BoardArea2DView *)Parent())->PerformMove(from, to);
    else if (game->editingPosition)  //### Hmmm: Needed due to pos filter, but
                                     //copied from BoardArea2DView
    {
      if (from != to)
        game->Edit_MovePiece(from, to), DrawSquare(from);
      else
        game->Edit_ClearPiece(to);
      DrawSquare(to);
      FlushPortBuffer();
      sigmaApp->PlayMoveSound(false);
    }
  } else {
    SetMoveMarker(false);
    DrawSquare(from);
    DrawSquare(to);
    FlushPortBuffer();
  }
} /* BoardView::TrackMove */

static void ConfineRect(CRect *r,
                        CRect *R)  // Confine r within R by offsetting r
{                                  // appropriately.
  if (r->left < R->left)
    r->Offset(R->left - r->left, 0);
  else if (r->right > R->right)
    r->Offset(R->right - r->right, 0);

  if (r->top < R->top)
    r->Offset(0, R->top - r->top);
  else if (r->bottom > R->bottom)
    r->Offset(0, R->bottom - r->bottom);
} /* ConfineRect */

/*-------------------------------------- Show Legal Moves
 * ----------------------------------------*/

void BoardView::ShowLegalMoves(SQUARE from) {
  if (!gameWin) return;
  ((BoardArea2DView *)Parent())->ShowLegalMoves(from);
} /* BoardView::ShowLegalMoves */

void BoardView::FrameSquare(SQUARE sq, RGB_COLOR *color) {
  CRect r = SquareToRect(sq);
  SetForeColor(color);
  SetPenSize(2, 2);
  // r.Inset(1, 1);
  DrawRectFrame(r);
  SetForeColor(&color_Black);
  SetPenSize(1, 1);
} /* BoardView::FrameSquare */

/*---------------------------------------- Popup Menus
 * -------------------------------------------*/

void BoardView::BoardTypePopup(void) {
  CMenu *pm = sigmaApp->BuildBoardTypeMenu(true);
  INT current = boardType_First + Prefs.Appearance.boardType;
  pm->CheckMenuItem(current, true);

  INT msg;
  if (pm->Popup(&msg)) sigmaApp->HandleMessage(msg);
  delete pm;
} /* BoardView::BoardTypePopup */

void BoardView::PieceSetPopup(void) {
  CMenu *pm = sigmaApp->BuildPieceSetMenu(true);
  INT current = pieceSet_First + Prefs.Appearance.pieceSet;
  pm->CheckMenuItem(current, true);

  INT msg;
  if (pm->Popup(&msg)) sigmaApp->HandleMessage(msg);
  delete pm;
} /* BoardView::PieceSetPopup */

/**************************************************************************************************/
/*                                                                                                */
/*                                           UTILITY */
/*                                                                                                */
/**************************************************************************************************/

CRect BoardView::SquareToRect(SQUARE sq) {
  INT f = file(sq);
  INT r = rank(sq);

  if (BoardTurned()) f = 7 - f, r = 7 - r;

  CRect rect(0, 0, squareWidth, squareWidth);
  rect.Offset(frameWidth + f * squareWidth, frameWidth + (7 - r) * squareWidth);
  return rect;
} /* BoardView::SquareToRect */

SQUARE BoardView::PointToSquare(CPoint p) {
  CRect rect = bounds;
  rect.Inset(frameWidth, frameWidth);
  if (!p.InRect(rect)) return nullSq;

  INT f = (p.h - rect.left) / squareWidth;
  INT r = 7 - (p.v - rect.top) / squareWidth;

  if (BoardTurned()) f = 7 - f, r = 7 - r;

  return (f >= 0 && f <= 7 && r >= 0 && r <= 7 ? square(f, r) : nullSq);
} /* BoardView::PointToSquare */

BOOL BoardView::BoardTurned(void) {
  return (gameWin && gameWin->boardTurned);
} /* BoardView::BoardTurned */

/*--------------------------------- Piece Drawing/Movement
 * ---------------------------------------*/
// This is a fairly complex routine, that first builds a complete offscreen
// bitmap for the requested area and draws it in the window.

void BoardView::DrawPieceAt(PIECE piece, CRect *area, CRect *dst) {
  INT h, v, h1, v1;     // The "pivot" points.
  INT w = squareWidth;  // The square width.
  CRect a = *area;      // Normalized destination area.
  CRect d;              // The destination square rectangles.
  CRect src;            // Piece bmp source retangle.
  SQUARE sq, df, dr;    // Pivot square and board directions

  // First compute the board directions (depending on whether the board is
  // turned):

  if (BoardTurned())
    df = -1, dr = -0x10;
  else
    df = 1, dr = 0x10;

  // Compute the local "pivot" point, i.e. the top left corner of the top/left
  // most square that has it's top-left corner inside the "area". The second
  // pivot point is defined as the bottom-right corner of the pivot square (but
  // confined to "area").

  a.Normalize();
  h = w - (area->left - frameWidth) % w;
  if (h == 0) h = w;
  v = w - (area->top - frameWidth) % w;
  if (v == 0) v = w;
  h1 = Min(h + w, a.right);
  v1 = Min(v + w, a.bottom);

  // Next determine the top-left most visible square (with the lower right
  // corner at the pivot point).

  CPoint pa(area->left, area->top);
  sq = PointToSquare(pa);

  DrawUtilSq(sq, CRect(0, 0, h, v), w - h, w - v);
  DrawUtilSq(sq + df, CRect(h, 0, h1, v), -h, w - v);
  DrawUtilSq(sq + 2 * df, CRect(h1, 0, a.right, v), -h - w, w - v);
  DrawUtilSq(sq - dr, CRect(0, v, h, v1), w - h, -v);
  DrawUtilSq(sq - dr + df, CRect(h, v, h1, v1), -h, -v);
  DrawUtilSq(sq - dr + 2 * df, CRect(h1, v, a.right, v1), -h - w, -v);
  DrawUtilSq(sq - 2 * dr, CRect(0, v1, h, a.bottom), w - h, -v - w);
  DrawUtilSq(sq - 2 * dr + df, CRect(h, v1, h1, a.bottom), -h, -v - w);
  DrawUtilSq(sq - 2 * dr + 2 * df, CRect(h1, v1, a.right, a.bottom), -h - w,
             -v - w);

  // Next we draw requested piece (if any):
  if (piece != empty) {
    src = CalcPieceBmpRect(piece, squareWidth);
    d = *dst;
    d.Normalize();
    d.Offset(dst->left - area->left, dst->top - area->top);
    utilBmpView->SetBackColor(&color_Blue);
    switch (squareWidth) {
      case squareWidth1:
        utilBmpView->DrawBitmap(pieceBmp1, src, d, bmpMode_Trans);
        break;
      case squareWidth2:
        utilBmpView->DrawBitmap(pieceBmp2, src, d, bmpMode_Trans);
        break;
      case squareWidth3:
        utilBmpView->DrawBitmap(pieceBmp3, src, d, bmpMode_Trans);
        break;
      case squareWidth4:
        utilBmpView->DrawBitmap(pieceBmp4, src, d, bmpMode_Trans);
        break;
    }
  }

  // Finally we draw the offscreen util bitmap on the screen:
  DrawBitmap(utilBmp, a, *area, bmpMode_Copy);
} /* BoardView::DrawPieceAt */

void BoardView::DrawUtilSq(SQUARE sq, CRect d, INT hor, INT ver) {
  CRect s, ps;

  if (d.left >= d.right || d.top >= d.bottom) return;

  s = d;
  s.Offset(hor, ver);
  utilBmpView->SetBackColor(&color_White);
  utilBmpView->DrawBitmap((odd(file(sq) + rank(sq)) ? wSquareBmp : bSquareBmp),
                          s, d, bmpMode_Copy);

  if (game->Board[sq]) {
    ps = CalcPieceBmpRect(game->Board[sq], squareWidth);
    s.Offset(ps.left, ps.top);
    utilBmpView->SetBackColor(&color_Blue);
    switch (squareWidth) {
      case squareWidth1:
        utilBmpView->DrawBitmap(pieceBmp1, s, d, bmpMode_Trans);
        break;
      case squareWidth2:
        utilBmpView->DrawBitmap(pieceBmp2, s, d, bmpMode_Trans);
        break;
      case squareWidth3:
        utilBmpView->DrawBitmap(pieceBmp3, s, d, bmpMode_Trans);
        break;
      case squareWidth4:
        utilBmpView->DrawBitmap(pieceBmp4, s, d, bmpMode_Trans);
        break;
    }
    //    utilBmpView->DrawBitmap((squareWidth == minSquareWidth ? minPieceBmp :
    //    maxPieceBmp), s, d, bmpMode_Trans);
  }
} /* BoardView::DrawUtilSq */
