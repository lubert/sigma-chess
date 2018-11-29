/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this
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

#include "CView.h"
#include "Collection.h"
#include "Game.h"

#define springHeaderLineHeight 12

class SpringHeaderView : public CView {
 public:
  SpringHeaderView(CViewOwner *parent, CRect frame, BOOL divider = true,
                   BOOL vclosed = true, BOOL vblackFrame = true);

  virtual void HandleUpdate(CRect updateRect);
  virtual BOOL HandleMouseDown(CPoint pt, INT modifiers, BOOL doubleClick);
  virtual void HandleActivate(BOOL wasActivated);
  virtual void HandleResize(void);  // Should be overridden

  virtual void HandleToggle(BOOL closed);

  BOOL Closed(void);
  void DrawHeaderStr(CHAR *s);
  void DrawStrPair(INT h, INT v, CHAR *tag, CHAR *value);
  CGame *Game(void);
  SigmaCollection *Collection(void);

 private:
  CHAR headerStr[100];
  BOOL divider;
  BOOL closed;
  BOOL blackFrame;
};
