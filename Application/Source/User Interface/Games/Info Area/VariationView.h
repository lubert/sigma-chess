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

#include "AnalysisState.h"
#include "BackView.h"
#include "DataHeaderView.h"
#include "DataView.h"

#include "Move.h"
#include "Search.h"

class GameWindow;

class VariationView : public DataView {
 public:
  VariationView(CViewOwner *parent, CRect frame);

  virtual void HandleUpdate(CRect updateRect);
  virtual void HandleActivate(BOOL wasActivated);
  virtual BOOL HandleMouseDown(CPoint pt, INT modifiers, BOOL doubleClick);
  virtual void HandleResize(void);

  void Reset(void);
  void SetMainLine(INT pvNo);

  void CalcCoord(void);

 private:
  void DrawMainLine(INT pvNo);  // pvNo: 1 (MainLine) ... maxMultiPV (0 = all)
  void DrawMainLine_Vertical(INT pvNo);
  void DrawMainLine_Horizontal(INT pvNo);
  void DrawVarMove(MOVE *m, CHAR *moveStr);
  void DrawScore(INT h, INT v, INT pvNo);

  void CalcMultiPVRect(INT pvNo, CRect *r, INT *pvLines);

  INT maxLines;
  CRect dataRect;
  CRect MultiPVRect[uci_MaxMultiPVcount + 1];  // Horizontal MultiPV rects
  INT PVLines[uci_MaxMultiPVcount + 1];

  GameWindow *win;
  ANALYSIS_STATE *Analysis;
};
