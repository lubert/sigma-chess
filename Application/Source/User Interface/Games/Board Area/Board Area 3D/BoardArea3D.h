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

#include "BoardArea.h"
#include "CButton.h"
#include "CWindow.h"
#include "Game.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                      CLASS & TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef struct  // Graphical coordinates of a 3D square (polygon):
{
  INT top, topLeft, topRight;
  INT bottom, bottomLeft, bottomRight;
} SQUARE3D;

class GameWindow;

class BoardArea3DView : public BoardAreaView {
 public:
  BoardArea3DView(CViewOwner *parent, CRect frame);

  virtual void ClearMenu(void);

  virtual void HandleUpdate(CRect updateRect);
  virtual BOOL HandleMouseDown(CPoint pt, INT modifiers, BOOL doubleClick);
  virtual BOOL HandleKeyDown(CHAR c, INT key, INT modifiers);

  virtual void DrawBoard(void);
  // virtual void  DrawBoardFrame (void);
  virtual void DrawAllSquares(void);
  virtual void DrawSquare(SQUARE sq);
  virtual void DrawPieceMovement(PIECE piece, PIECE target, SQUARE from,
                                 SQUARE to);

  virtual PIECE AskPromPiece(SQUARE from, SQUARE to);

  virtual void DrawPlayerIndicator(void);

  virtual void RefreshGameStatus(void);

  virtual void ShowPosEditor(BOOL showPos);

  virtual void FrameSquare(SQUARE sq, RGB_COLOR *color);
  virtual void ClearShowLegal(void);

  void DrawPromMenu(void);
  void DrawSetupMenu(void);
  void DrawPanelBackground(void);
  void SelectPlayer(COLOUR player);
  void ToggleClocks(void);

  GameWindow *gameWin;
  CGame *game;
  // BOOL promoting;

 private:
  void AdjustBounds(void);

  void DrawArea(CRect destRect, CBitmap *tempPw = nil,
                CRect tempSrc = CRect(0, 0, 0, 0), CRect = CRect(0, 0, 0, 0));
  void DrawTempPiece(CRect destRect, CBitmap *tempPw, CRect tempSrc,
                     CRect tempDst);
  SQUARE TrackMove(SQUARE from, CPoint p);

  BOOL Point2RowCol(CPoint p, INT *row, INT *col);
  SQUARE Point2Square(CPoint pt);
  CBitmap *CalcPieceParam(PIECE piece, INT row, INT col, CRect *src,
                          CRect *dst);
  void CalcCoord(INT rank, INT file, SQUARE3D *sq3D, CRect *dest);

  void ReadPromotion(CPoint p0);

  PIECE GetSquare(INT row, INT col);
  PIECE GetSquare(SQUARE sq);
  void Square2RowCol(SQUARE sq, INT *row, INT *col);
  SQUARE RowCol2Square(INT row, INT col);

  SQUARE promSquare;
  PIECE promPiece;

  CButton *buttonWhite, *buttonBlack;
  CButton *buttonDone, *buttonCancel, *buttonStatus, *buttonClear, *buttonNew;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

extern BOOL board3Denabled;

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

void InitBoard3DModule(void);
