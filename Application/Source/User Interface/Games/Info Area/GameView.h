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

#include "BackView.h"
#include "DataView.h"
#include "SpringHeaderView.h"
#include "CControl.h"

#include "Game.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define digitWidth         7
#define digitBWidth        8
#define moveStrWidth       54
#define moveStrBWidth      80

#define defaultGameViewHeight 260
#define minGameViewHeight     186


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class GameDataView;

class GameView : public BackView
{
public:
   GameView (CViewOwner *parent, CRect frame);

   virtual void HandleUpdate (CRect updateRect);
   virtual void HandleResize (void);
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);

   BOOL CheckScrollEvent (CScrollBar *ctrl, BOOL tracking);
   void RefreshGameStatus (void);
   void RefreshGameInfo (void);
   void RefreshLibInfo (void);
   void ResizeHeader (void);
   void UpdateGameList (void);
   void RedrawGameList (void);

   INT restoreHeight;
   INT futureLineCount;   // Currently visible number of future lines

private:
   GameDataView *gameDataView;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES                                 */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

void DrawGameMove (CView *view, MOVE *m, BOOL printing = false);
void DrawGameMoveStr (CView *view, MOVE *m, CHAR *s, BOOL printing = false);
void DrawTextLine (CView *view, CHAR *s, INT n, INT lineWidth, BOOL isLastLine);
void DrawGameSpecial (CView *view, INT lineWidth, INT type, GAMEINFO *Info, LONG gameNo = -1, BOOL printing = false);
