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

#pragma once

#include "BackView.h"
#include "Board.h"
#include "BoardArea.h"
#include "DataView.h"
#include "Game.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define clockViewWidth 110

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Main Board Area View
 * -------------------------------------*/

class GameWindow;
class PlayerView;
class ClockView;

class BoardAreaView : public BackView {
 public:
  BoardAreaView(CViewOwner *parent, CRect frame);

  virtual void HandleUpdate(CRect updateRect);

  virtual void ClearMenu(void);

  virtual void DrawBoard(void);
  virtual void DrawBoardFrame(void);
  virtual void DrawAllSquares(void);
  virtual void DrawSquare(SQUARE sq);
  virtual void SetMoveMarker(BOOL engineMove);
  virtual void ClearMoveMarker(void);
  virtual void DrawPieceMovement(PIECE piece, PIECE target, SQUARE from,
                                 SQUARE to);

  virtual PIECE AskPromPiece(SQUARE from, SQUARE to);

  virtual void DrawPlayerInfo(void);
  virtual void DrawClockInfo(void);
  virtual void DrawClockTime(COLOUR player);
  virtual void DrawPlayerIndicator(void);

  virtual void DrawModeIcons(void);
  virtual void DrawLevelInfo(COLOUR colour, BOOL redraw = false);

  virtual void RefreshGameStatus(void);

  virtual void ShowPosEditor(BOOL showPos);

  virtual void FrameSquare(SQUARE sq, RGB_COLOR *color);
  virtual void ClearShowLegal(void);

  void DrawMove(BOOL engineMove = false);
  void DrawUndoMove(void);

  void PerformMove(SQUARE from, SQUARE to);
  void PerformPlayerMove(MOVE *m);

  void ShowLegalMoves(SQUARE from);

  GameWindow *gameWin;
  CGame *game;

  PlayerView *playerViewT, *playerViewB;
  ClockView *clockViewT, *clockViewB;
};

/*------------------------------------ Player Info/Clock Views
 * -----------------------------------*/

class PlayerClockView : public DataView {
 public:
  PlayerClockView(CViewOwner *parent, CRect frame, BOOL atTop);
  virtual void HandleUpdate(CRect updateRect);

  COLOUR Colour(void);

  BOOL atTop;
  GameWindow *gameWin;
};

class PlayerView : public PlayerClockView {
 public:
  PlayerView(CViewOwner *parent, CRect frame, BOOL atTop);
  virtual void HandleUpdate(CRect updateRect);

  void DrawPlayerIndicator(void);
};

class ClockView : public PlayerClockView {
 public:
  ClockView(CViewOwner *parent, CRect frame, BOOL atTop);
  virtual void HandleUpdate(CRect updateRect);

  void DrawLevel(BOOL redraw = false);
  void DrawTime(void);

 private:
  CHAR levelStr[10];
};

/**************************************************************************************************/
/*                                                                                                */
/*                                        GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/
