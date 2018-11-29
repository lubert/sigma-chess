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

#include "GameList.h"
#include "CollectionWindow.h"
#include "SigmaApplication.h"

#define vMargin 3

class GameListFooter : public DataHeaderView {
 public:
  GameListFooter(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);

  HEADER_COLUMN HCTab[1];
  CHAR s[100];
};

/**************************************************************************************************/
/*                                                                                                */
/*                                            GAME LIST AREA */
/*                                                                                                */
/**************************************************************************************************/

// The GameListArea is mainly a "container" view that includes the GameListView,
// where the actual collection game list is shown.

GameListArea::GameListArea(CViewOwner *parent, CRect frame)
    : BackView(parent, frame, true) {
  gameListView = new GameListView(this, GameListRect());
  footerView = new GameListFooter(this, FooterRect());

  CRect r = bounds;
  r.Inset(8, 8);
  ExcludeRect(r);
} /* GameListArea::GameListArea */

CRect GameListArea::FooterRect(void) {
  CRect r = bounds;
  r.Inset(8, 8);
  r.top = r.bottom - DataHeaderView_Height(true);
  r.bottom;
  return r;
} /* GameListArea::FooterRect */

CRect GameListArea::GameListRect(void) {
  CRect r = bounds;
  r.Inset(8, 8);
  r.bottom -= DataHeaderView_Height(true) - 1;
  return r;
} /* GameListArea::GameListRect */

SigmaCollection *GameListArea::Collection(void) {
  return ((CollectionWindow *)Window())->collection;
} /* GameListArea::Collection */

/*---------------------------------------- Event Handling
 * ----------------------------------------*/

void GameListArea::HandleResize(void) {
  gameListView->SetFrame(GameListRect());
  footerView->SetFrame(FooterRect());

  CRect r = bounds;
  r.Inset(8, 8);
  ExcludeRect(r);
} /* GameListArea::HandleResize */

BOOL GameListArea::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (gameListView->HandleKeyDown(c, key, modifiers)) return true;
  //### other key strokes here....
  return false;
} /* GameListView::HandleKeyDown */

BOOL GameListArea::CheckScrollEvent(CScrollBar *ctrl, BOOL tracking) {
  return gameListView->CheckScrollEvent(ctrl, tracking);
} /* GameListArea::CheckScrollEvent */

void GameListArea::SelectAll(void) {
  gameListView->SetSelection(0, Collection()->View_GetGameCount() - 1);
  gameListView->DrawList();
} /* GameListArea::SelectAll */

void GameListArea::SetSelection(LONG start, LONG end) {
  gameListView->SetSelection(start, end);
  gameListView->DrawList();
} /* GameListArea::SetSelection */

LONG GameListArea::GetTotalCount(void) {
  return Collection()->View_GetGameCount();
} /* GameListArea::GetTotalCount */

LONG GameListArea::GetSelCount(void) {
  if (Collection()->View_GetGameCount() == 0) return 0;
  return gameListView->GetSelEnd() - gameListView->GetSelStart() + 1;
} /* GameListArea::GetSelCount */

LONG GameListArea::GetSelStart(void) {
  return gameListView->GetSelStart();
} /* GameListArea::GetSelStart */

LONG GameListArea::GetSelEnd(void) {
  return gameListView->GetSelEnd();
} /* GameListArea::GetSelEnd */

LONG GameListArea::GetSel(void) {
  return gameListView->GetSel();
} /* GameListArea::GetSel */

void GameListArea::ResetScroll(void) {
  gameListView->SetSelection(0, 0);
  RefreshList();
} /* GameListArea::ResetScroll */

void GameListArea::RefreshList(void) {
  gameListView->AdjustScrollBar();
  gameListView->Redraw();
  footerView->Redraw();
} /* GameListArea::RefreshList */

void GameListArea::DrawList(void) {
  gameListView->DrawList();
} /* GameListArea::DrawList */

void GameListArea::DrawFooter(void) {
  footerView->Redraw();
} /* GameListArea::DrawList */

void GameListArea::Enable(BOOL enable) {
  gameListView->Enable(enable);
} /* GameListArea::Enable */

void GameListArea::TogglePublishing(void) {
  gameListView->TogglePublishing();
} /* GameListArea::TogglePublishing */

/**************************************************************************************************/
/*                                                                                                */
/*                                        GAME LIST FOOTER */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Constructor/Destructor
 * ------------------------------------*/
// The game list panel is a general purpose area below the actual game list. It
// shows any progress bars as well as collection info and statistics (e.g. score
// in %).

GameListFooter::GameListFooter(CViewOwner *owner, CRect frame)
    : DataHeaderView(owner, frame, false, true, 1, HCTab) {
  CopyStr("", s);
  HCTab[0].text = s;
  HCTab[0].iconID = 0;
  HCTab[0].width = -1;
} /* GameListFooter::GameListFooter */

/*--------------------------------------- Event Handling
 * -----------------------------------------*/

void GameListFooter::HandleUpdate(CRect updateRect) {
  SigmaCollection *col = ((CollectionWindow *)Window())->collection;
  if (col->useFilter)
    Format(s, "%ld of %ld games (filter applied)", col->View_GetGameCount(),
           col->GetGameCount());
  else
    Format(s, "%ld games", col->GetGameCount());

  DataHeaderView::HandleUpdate(updateRect);
  if (RunningOSX()) {
    SetForeColor(&color_MdGray);
    DrawRectFrame(bounds);
  }
} /* GameListFooter::HandleUpdate */
