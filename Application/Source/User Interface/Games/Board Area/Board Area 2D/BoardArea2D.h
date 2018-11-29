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
#include "BoardArea.h"
#include "BoardView.h"
#include "DataView.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class BoardArea2DView : public BoardAreaView {
 public:
  BoardArea2DView(CViewOwner *parent, CRect frame);

  virtual void HandleUpdate(CRect updateRect);
  virtual void HandleResize(void);

  virtual void DrawBoardFrame(void);
  virtual void DrawAllSquares(void);
  virtual void DrawSquare(SQUARE sq);
  virtual void SetMoveMarker(BOOL engineMove);
  virtual void ClearMoveMarker(void);
  virtual void DrawPieceMovement(PIECE piece, PIECE target, SQUARE from,
                                 SQUARE to);

  virtual PIECE AskPromPiece(SQUARE from, SQUARE to);
  virtual void DrawPlayerIndicator(void);
  virtual void FrameSquare(SQUARE sq, RGB_COLOR *color);

 private:
  void CalcFrames(void);

  GameWindow *gameWin;
  INT squareWidth, frameWidth;
  CRect boardRect, playerTRect, playerBRect, clockTRect, clockBRect;
  BoardView *boardView;
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

INT BoardArea_Width(INT sqWidth);
INT BoardArea_Height(INT sqWidth);
