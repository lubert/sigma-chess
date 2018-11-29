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

#include "CButton.h"
#include "CView.h"

#define toolbar_Height 57
#define toolbar_HeightSmall 25
#define toolbar_SeparatorWidth 10
#define toolbar_MaxSeparators 20

class CToolbar : public CView {
 public:
  CToolbar(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
  virtual void HandleResize(void);

  void AddSeparator(INT width = toolbar_SeparatorWidth);
  CButton *AddButton(INT command, INT iconID, INT iconSize, INT width,
                     CHAR *title = "", CHAR *helpText = "");
  CButton *AddPopup(INT command, CMenu *popupMenu, INT iconID, INT iconSize,
                    INT width, CHAR *title = "", CHAR *helpText = "");
  // CButton *AddButton (INT command, CBitmap *enabledBmp, CBitmap *disabledBmp,
  // INT width, CHAR *helpText = "");
  void AddCustomView(CView *view);

  CRect NextItemRect(INT width);

 private:
  INT end;  // Right end of toolbar (i.e. of last button/separator
  INT sepCount;
  INT SepPos[toolbar_MaxSeparators];
};

class CToolbarTextView : public CView {
 public:
  CToolbarTextView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
  virtual void HandleActivate(BOOL activated);
};
