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
#include "CButton.h"
#include "DataHeaderView.h"
#include "DataView.h"

#include "Board.h"
#include "PosLibrary.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class LibToolbar;
class LibListView;

class LibEditorView : public BackView {
 public:
  LibEditorView(CViewOwner *parent, CRect frame);

  virtual void HandleUpdate(CRect r);
  virtual void HandleResize(void);

  void HandleOpen(void);
  void Refresh(void);
  void CheckAutoAdd(void);

  BOOL CheckScrollEvent(CScrollBar *ctrl, BOOL tracking);

 private:
  void CalcFrames(CRect *listRect, CRect *toolbarRect);

  CGame *game;
  LibToolbar *toolbar;
  LibListView *list;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/
