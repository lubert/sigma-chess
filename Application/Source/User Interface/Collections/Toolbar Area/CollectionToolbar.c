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

#include "CollectionToolbar.h"
#include "BmpUtil.h"
#include "CollectionWindow.h"
#include "SigmaApplication.h"

static CHAR *tbText[15] = {
    "Open the currently selected game in a new window.",
    "Open the Collection Info Dialog, where you can view/set various "
    "information for this Collection.",
    "Export the selected games to a PGN file.",
    "Import and append the games from a PGN file.",
    "Delete the selected games.",
    "Reduce the size of the collection by skipping unused space.",
    "Print the selected games.",
    "Filter the list of games according to e.g. player, date and opening",
    "Define/edit layout information for the selected game"};

/**************************************************************************************************/
/*                                                                                                */
/*                                           MAIN TOOLBAR */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------ Constructor
 * -----------------------------------------*/

CollectionToolbar::CollectionToolbar(CViewOwner *parent, CRect frame)
    : CToolbar(parent, frame) {
  tb_Filter =
      AddButton(collection_EditFilter, 1320, 32, 55, "Filter", tbText[7]);
  AddSeparator();
  tb_OpenGame =
      AddButton(collection_OpenGame, 1007, 32, 55, "Open Game", tbText[0]);
  tb_Layout = AddButton(collection_Layout, 1324, 32, 55, "Layout", tbText[8]);
  AddSeparator();
  tb_ImportPGN =
      AddButton(collection_ImportPGN, 1323, 32, 60, "Import PGN", tbText[3]);
  tb_ExportPGN =
      AddButton(collection_ExportPGN, 1322, 32, 60, "Export PGN", tbText[2]);
  tb_Delete = AddButton(edit_Clear, 1321, 32, 55, "Delete", tbText[4]);
  AddSeparator();
  tb_ColInfo = AddButton(collection_Info, 1315, 32, 55, "Info", tbText[1]);
  tb_Print = AddButton(file_Print, 1316, 32, 55, "Print", tbText[6]);
} /* CollectionToolbar::CollectionToolbar */

void CollectionToolbar::HandleResize(void) {
  CRect r = bounds;
  r.left = tb_Print->frame.right - 1;
  r.right++;
} /* CollectionToolbar::HandleResize */

/*------------------------------------------- Adjust
 * ---------------------------------------------*/

void CollectionToolbar::Adjust(void) {
  CollectionWindow *win = (CollectionWindow *)Window();
  BOOL busy = win->busy;
  BOOL locked = win->IsLocked();
  LONG totalCount = win->collection->GetGameCount();
  LONG selCount = win->gameListArea->GetSelCount();

  tb_Filter->Enable(!busy && totalCount > 0);
  tb_OpenGame->Enable(!busy && selCount == 1);
  tb_Layout->Enable(!busy && selCount == 1);
  tb_ImportPGN->Enable(!busy && !locked);
  tb_ExportPGN->Enable(!busy && selCount > 0);
  tb_Delete->Enable(!busy && selCount > 0 && !locked);
  tb_ColInfo->Enable(!busy);
  tb_Print->Enable(!busy && selCount > 0);
} /* CollectionToolbar::Adjust */

/**************************************************************************************************/
/*                                                                                                */
/*                                           MINI TOOLBAR */
/*                                                                                                */
/**************************************************************************************************/

#define progressViewWidth 580

class ToolbarProgressView : public CView {
 public:
  ToolbarProgressView(CViewOwner *parent, CRect frame);

  virtual void HandleUpdate(CRect updateRect);
  virtual void HandleResize(void);
  virtual void HandleActivate(BOOL wasActivated);

  CRect ProgressRect(void);
  CRect StopButtonRect(void);

  void BeginProgress(CHAR *prompt, LONG max);
  void SetProgress(CHAR *str, LONG n);
  void EndProgress(void);

  BOOL inProgress;
  CHAR text1[128], text2[128];
  CProgressBar *progressBar;
  CButton *button;
};

/*------------------------------------------ Constructor
 * -----------------------------------------*/

MiniCollectionToolbar::MiniCollectionToolbar(CViewOwner *parent, CRect frame)
    : CToolbar(parent, frame) {
  // Group 1 (last move)
  AddCustomView(cv_Progress = new ToolbarProgressView(
                    this, NextItemRect(progressViewWidth)));
  AddSeparator();
} /* MiniCollectionToolbar::MiniCollectionToolbar */

void MiniCollectionToolbar::HandleUpdate(CRect updateRect) {
  CToolbar::HandleUpdate(updateRect);
  DrawReadOnlyGroup(false);
} /* MiniCollectionToolbar::HandleUpdate */

/*------------------------------------------- Adjust
 * ---------------------------------------------*/

void MiniCollectionToolbar::Adjust(void) {} /* MiniCollectionToolbar::Adjust */

void MiniCollectionToolbar::DrawReadOnlyGroup(BOOL redrawBackground) {
  if (bounds.Width() < 600) return;

  if (redrawBackground) {
    CRect r = bounds;
    r.Inset(1, 1);
    r.left = r.right - 110;
    DrawStripeRect(r);
  }

  CollectionWindow *win = (CollectionWindow *)Window();
  ICON_TRANS trans = (win->IsFront() ? iconTrans_None : iconTrans_Disabled);
  CRect ri(0, 0, 16, 16);
  ri.Offset(bounds.right - 25 - 16, bounds.bottom - 21);
  if (win->IsLocked()) DrawIcon(icon_Lock, ri, trans);
} /* MiniCollectionToolbar::DrawReadOnlyGroup */

/*------------------------------------------ Progress
 * --------------------------------------------*/

void MiniCollectionToolbar::BeginProgress(CHAR *prompt, LONG max) {
  cv_Progress->BeginProgress(prompt, max);
} /* MiniCollectionToolbar::BeginProgress */

void MiniCollectionToolbar::SetProgress(CHAR *str, LONG n) {
  cv_Progress->SetProgress(str, n);
} /* MiniCollectionToolbar::SetProgress */

void MiniCollectionToolbar::EndProgress(void) {
  cv_Progress->EndProgress();
} /* MiniCollectionToolbar::EndProgress */

/**************************************************************************************************/
/*                                                                                                */
/*                                         PROGRESS VIEW */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Constructor/Destructor
 * ------------------------------------*/
// The progress view shows various collection progress data (PGN import/export
// etc) and is embedded in the toolbar as a fixed width custom view.

ToolbarProgressView::ToolbarProgressView(CViewOwner *parent, CRect frame)
    : CView(parent, frame) {
  SetFontMode(fontMode_Or);

  CopyStr("", text1);
  CopyStr("", text2);

  inProgress = false;
  progressBar = new CProgressBar(this, ProgressRect(), 0, 100, false);
  button = new CButton(this, StopButtonRect(), msg_ColStop, 0, false, true, 265,
                       "", "");
} /* ToolbarProgressView::ToolbarProgressView */

/*--------------------------------------- Event Handling
 * -----------------------------------------*/

void ToolbarProgressView::HandleUpdate(CRect updateRect) {
  CRect r = bounds;
  r.Inset(0, 1);
  DrawStripeRect(r, 0);

  // If the progress bar is running, draw progress text and progress bar frame:

  if (inProgress) {
    if (!RunningOSX()) Draw3DFrame(ProgressRect(), &color_Gray, &color_White);

    SetFontForeColor();
    // Draw main/fixed prompt (in bold):
    MovePenTo(bounds.left + 3, bounds.bottom - 8);
    SetFontStyle(fontStyle_Bold);
    DrawStr(text1);
    // Draw progress prompt (in plain):
    MovePenTo(bounds.left + 100, bounds.bottom - 8);
    SetFontStyle(fontStyle_Plain);
    DrawStr(text2);
  }
} /* ToolbarProgressView::HandleUpdate */

void ToolbarProgressView::HandleResize(void) {
  progressBar->SetFrame(ProgressRect());
  button->SetFrame(StopButtonRect());
  Redraw();
} /* ToolbarProgressView::HandleResize */

void ToolbarProgressView::HandleActivate(BOOL wasActivated) {
  Redraw();
} /* ToolbarProgressView::HandleActivate */

/*-------------------------------------- Setting Progress
 * ----------------------------------------*/

void ToolbarProgressView::BeginProgress(CHAR *prompt, LONG max) {
  CopyStr((prompt ? prompt : ""), text1);

  progressBar->max = max;
  progressBar->Show(true);
  progressBar->Reset();
  button->Show(true, false);
  inProgress = true;
  Redraw();
} /* ToolbarProgressView::BeginProgress */

void ToolbarProgressView::SetProgress(CHAR *str, LONG n) {
  CopyStr((str ? str : ""), text2);

  // Draw progress prompt (in plain):
  CRect r = bounds;
  r.Inset(0, 1);
  r.left += 100;
  r.right = ProgressRect().left - 5;
  DrawStripeRect(r, 0);
  SetFontForeColor();
  MovePenTo(bounds.left + 100, bounds.bottom - 8);
  DrawStr(text2);

  progressBar->Set(n);
  theApp->ProcessEvents();
} /* ToolbarProgressView::SetProgress */

void ToolbarProgressView::EndProgress(void) {
  CopyStr("", text1);
  CopyStr("", text2);

  progressBar->Show(false);
  button->Show(false, false);
  inProgress = false;
  Redraw();
} /* ToolbarProgressView::EndProgress */

/*-------------------------------------------- Misc
 * ----------------------------------------------*/

CRect ToolbarProgressView::ProgressRect(void) {
  CRect r = bounds;
  r.Inset(4, (toolbar_HeightSmall - controlHeight_ProgressBar) / 2);
  r.right -= 26;
  r.left = r.right - 200;
  return r;
} /* ToolbarProgressView::ProgressRect */

CRect ToolbarProgressView::StopButtonRect(void) {
  CRect r(0, 0, 16, 16);
  r.Offset(bounds.right - 20, 4);
  return r;
} /* ToolbarProgressView::StopButtonRect */
