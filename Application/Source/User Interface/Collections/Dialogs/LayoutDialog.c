/**************************************************************************************************/
/*                                                                                                */
/* Module  : ColInfoDialog.c */
/* Purpose : This module implements the collection info dialogs. */
/*                                                                                                */
/**************************************************************************************************/

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

#include "LayoutDialog.h"
#include "SigmaApplication.h"

class CLayoutDialog : public CDialog {
 public:
  CLayoutDialog(CRect frame, CHAR *title, GAMEINFO *Info, BOOL colLocked);
  virtual void HandleRadioButton(CRadioButton *ctrl);

  CRadioButton *cradio[4];
  CEditControl *cedit_Heading;

  CCheckBox *ccheck_PageBreak;
  CCheckBox *ccheck_IncludeInfo;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG */
/*                                                                                                */
/**************************************************************************************************/

BOOL LayoutDialog(LONG gameNo, GAMEINFO *Info, BOOL colLocked) {
  CHAR title[100];
  CRect frame(0, 0, 350, 270);
  if (RunningOSX()) frame.right += 60, frame.bottom += 30;
  theApp->CentralizeRect(&frame);
  Format(title, "Collection Layout (Game %ld)", gameNo + 1);

  CLayoutDialog *dialog = new CLayoutDialog(frame, title, Info, colLocked);
  dialog->Run();

  BOOL done = false;
  if (dialog->reply == cdialogReply_OK) {
    for (INT i = headingType_None; i <= headingType_Section; i++)
      if (dialog->cradio[i]->Selected()) Info->headingType = i;
    dialog->cedit_Heading->GetTitle(Info->heading);
    Info->pageBreak = dialog->ccheck_PageBreak->Checked();
    Info->includeInfo = dialog->ccheck_IncludeInfo->Checked();
    done = true;
  }

  delete dialog;
  return (done && !colLocked);
} /* LayoutDialog */

CLayoutDialog::CLayoutDialog(CRect frame, CHAR *title, GAMEINFO *Info,
                             BOOL colLocked)
    : CDialog(nil, title, frame) {
  CRect inner = InnerRect();

  CRect rt = inner;
  rt.bottom = rt.top + 30;
  new CTextControl(this,
                   "The layout information controls if a game is prefixed with "
                   "a chapter or section heading or a page break etc",
                   rt, true, controlFont_SmallSystem);

  inner.top = rt.bottom + 5;
  inner.bottom -= 30;
  if (RunningOSX()) inner.bottom -= 5;
  CRect R1 = inner;
  R1.bottom = R1.top + 110;
  if (RunningOSX()) R1.bottom += 5;
  CRect R2 = inner;
  R2.top = R1.bottom + 5;
  CRect GR1 = R1;
  R1.Inset(10, 20);
  CRect GR2 = R2;
  R2.Inset(10, 20);

  //--- Heading Layout ---
  INT vspacing = (RunningOSX() ? 22 : 20);
  CRect r = R1;
  r.bottom = r.top + controlHeight_RadioButton;
  r.right = r.left + 110;
  cradio[headingType_Chapter] = new CRadioButton(this, "Chapter", 0, r);
  r.Offset(0, vspacing);
  cradio[headingType_Section] = new CRadioButton(this, "Section", 0, r);
  r.Offset(0, vspacing);
  cradio[headingType_GameNo] = new CRadioButton(this, "Game No", 0, r);
  r.Offset(0, vspacing);
  cradio[headingType_None] = new CRadioButton(this, "No Heading", 0, r);
  cradio[Info->headingType]->Select();

  r = R1;
  r.left += 120;
  r.bottom = r.top + controlHeight_Edit;
  r.right = R1.right - 10;
  r.Offset(0, 12);
  cedit_Heading = new CEditControl(this, Info->heading, r, 30, false);

  new CGroupBox(this, "Heading Layout", GR1);

  //--- Misc Options ---
  r = R2;
  r.bottom = r.top + controlHeight_CheckBox;
  ccheck_PageBreak =
      new CCheckBox(this, "Page break before game", Info->pageBreak, r);
  r.Offset(0, vspacing);
  ccheck_IncludeInfo =
      new CCheckBox(this, "Include game info", Info->includeInfo, r);
  new CGroupBox(this, "Printing & HTML Export Options", GR2);

  //--- Create the OK and Cancel buttons last ---
  cbutton_Cancel = new CPushButton(this, "Cancel", CancelRect());
  cbutton_Default =
      new CPushButton(this, "OK", DefaultRect(), true, !colLocked);
  SetDefaultButton(cbutton_Default);

  HandleRadioButton(cradio[Info->headingType]);
  CurrControl(cradio[Info->headingType]);
} /* CLayoutDialog::CLayoutDialog */

void CLayoutDialog::HandleRadioButton(CRadioButton *ctrl) {
  CDialog::HandleRadioButton(ctrl);

  for (INT i = headingType_None; i <= headingType_Section; i++)
    if (cradio[i] == ctrl) {
      cedit_Heading->Show(i > headingType_GameNo);
      //       cedit_Heading->Enable(i > headingType_GameNo);
    }
} /* CLayoutDialog::HandleRadioButton */
