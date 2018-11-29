/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAMEWINDOW3D.C */
/* Purpose : This module implements the 3D game windows. */
/* Revised : 2000-02-20 */
/*                                                                                                */
/**************************************************************************************************/

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

#include "GameWindow.h"
#include "SigmaApplication.h"

void GameWindow::Toggle3D(void) {
  Show(false);

  mode3D = !mode3D;

  if (mode3D) {
    if (annEditor) HandleMessage(game_AnnotationEditor);

    boardArea2DView->Show(false);
    infoAreaView->Show(false);
    toolbar->Show(false);

    boardArea3DView = new BoardArea3DView(this, theApp->ScreenRect());
    boardAreaView = boardArea3DView;

    frame2D = Frame();  // Save frame before moving
    Move(0, 0, false);
    Resize(theApp->ScreenRect().Width(), theApp->ScreenRect().Height());
  } else {
    delete boardArea3DView;
    boardArea3DView = nil;

    boardAreaView = boardArea2DView;

    boardArea2DView->Show(true);
    infoAreaView->Show(true);
    toolbar->Show(true);

    Move(frame2D.left, frame2D.top, false);
    Resize((showInfoArea ? gameWin_Width : boardArea_Width), gameWin_Height);
  }

  Show(true);

  if (!IsFront()) SetFront();
  AdjustToolbar();
  HandleMenuAdjust();
} /* GameWindow::Toggle3D */
