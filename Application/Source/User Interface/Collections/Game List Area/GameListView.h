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

#include "Collection.h"

#include "CButton.h"
#include "CControl.h"

#include "BackView.h"
#include "DataHeaderView.h"
#include "DataView.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define maxColCells 8

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- The Game List Box
 * ------------------------------------*/

class GameListHeaderView;
class ColListView;

class GameListView : public DataView {
 public:
  GameListView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
  virtual void HandleResize(void);
  virtual BOOL HandleKeyDown(CHAR c, INT key, INT modifiers);

  void Enable(BOOL enable);

  SigmaCollection *Collection(void);
  void DrawList(void);

  void HandleColumnSelect(INT i);
  void HandleSortDir(BOOL ascend);
  void SmartSearch(CHAR c);

  void TogglePublishing(void);

  LONG GetSelStart(void);
  LONG GetSelEnd(void);
  LONG GetSel(void);
  void SetSelection(LONG start, LONG end);
  void AdjustSelection(LONG delta, BOOL multi);
  BOOL CheckScrollEvent(CScrollBar *ctrl, BOOL tracking);
  void AdjustScrollBar(void);

  void ResetColumns(void);

  HEADER_COLUMN HCTab[maxColCells];
  INT columnCount;
  DataHeaderView *headerView;
  CScrollBar *scrollBar;

 private:
  ColListView *listView;
  INT linesVis;     // Number of visible lines in listbox
  LONG linesTotal;  // Total number of lines (= collection->viewCount)

  LONG selected;  // Currently selected game [0...linesTotal-1]
  LONG selStart;
  LONG selEnd;
};

class GameListHeaderView : public DataHeaderView {
 public:
  GameListHeaderView(CViewOwner *parent, CRect frame, INT columns,
                     HEADER_COLUMN *HCTab);
  virtual void HandleSelect(INT i);
  virtual void HandleSortDir(BOOL ascend);  // Should be overridden
  virtual void HandleColumnResize(INT i);   // Should be overridden
};

class ColListView : public CView {
 public:
  ColListView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
  virtual BOOL HandleMouseDown(CPoint pt, INT modifiers, BOOL doubleClick);
  virtual void HandleActivate(BOOL wasActivated);

  INT VisLines(void);
  void DrawList(void);
  void DrawRow(INT n);
  void CalcFieldStr(ULONG gameNo, INT i, CHAR *fs);
  GameListView *Parent(void);
  SigmaCollection *Collection(void);
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

void DefaultCollectionCellWidth(INT width[]);
