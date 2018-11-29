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

#pragma once

#include "DataView.h"
#include "Board.h"
#include "CBitmap.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define squareWidth1 42
#define squareWidth2 50
#define squareWidth3 58
#define squareWidth4 72

#define squareWidthCount 4

#define minSquareWidth squareWidth1
//#define maxSquareWidth squareWidth4


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class GameWindow;
class CGame;

class BoardView : public DataView
{
public:
   BoardView (CViewOwner *parent, CRect frame, CGame *game, GameWindow *gameWin = nil);

   virtual void HandleUpdate (CRect updateRect);
   virtual void HandleResize (void);
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);

   void   DrawFrame (void);
   void   DrawPlayerIndicator (void);
   void   DrawAllSquares (void);
   void   DrawSquare (SQUARE sq);
   void   SetMoveMarker (BOOL engineMove);
   void   ClearMoveMarker (void);

   void   DrawPieceMovement (PIECE piece, PIECE target, SQUARE from, SQUARE to);

   CRect  SquareToRect (SQUARE sq);
   SQUARE PointToSquare (CPoint p);

   BOOL   BoardTurned (void);

   void   ShowLegalMoves (SQUARE from);
   void   FrameSquare (SQUARE sq, RGB_COLOR *color);
   void   BoardTypePopup (void);
   void   PieceSetPopup (void);

private:
   void   CalcFrames (void);

   void   TrackMove (SQUARE from, CPoint p1);
   void   DrawPieceAt (PIECE piece, CRect *area, CRect *dst);
   void   DrawUtilSq (SQUARE sq, CRect d, INT hor, INT ver);

   GameWindow *gameWin;
   CGame      *game;
   INT        squareWidth, frameWidth;

   CRect  boardRect;           // "Inner" board view rect comprising all the squares.
   BOOL   markMove;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                        GLOBAL DATA STRUCTURES                                  */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

INT BoardFrame_Width (INT sqWidth);
