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

#include "ColInfoDialog.h"
#include "SigmaApplication.h"

class CInfoDialog : public CDialog {
 public:
  CInfoDialog(CWindow *parent, CRect frame, COLINFO *Info, BOOL colLocked);

  // Filter Name/List part:
  CEditControl *cedit_Title;
  CEditControl *cedit_Author;
  CEditControl *cedit_Descr;
  CCheckBox *ccheck_ShowHeadings;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG */
/*                                                                                                */
/**************************************************************************************************/

BOOL ColInfoDialog(CWindow *parent, COLINFO *Info, BOOL colLocked) {
  CRect frame(0, 0, 400, 250);
  theApp->CentralizeRect(&frame);
  if (RunningOSX()) frame.right += 30, frame.bottom += 30;

  CInfoDialog *dialog = new CInfoDialog(parent, frame, Info, colLocked);
  dialog->Run();

  BOOL done = false;
  if (dialog->reply == cdialogReply_OK) {
    dialog->cedit_Title->GetTitle(Info->title);
    dialog->cedit_Author->GetTitle(Info->author);
    dialog->cedit_Descr->GetText(Info->descr);
    Info->flags = 0;
    if (dialog->ccheck_ShowHeadings->Checked())
      Info->flags |= colInfoFlag_Publishing;

    done = true;
  }

  delete dialog;
  return done;
} /* ColInfoDialog */

CInfoDialog::CInfoDialog(CWindow *parent, CRect frame, COLINFO *Info,
                         BOOL colLocked)
    : CDialog(parent, "Collection Info", frame, cdialogType_Modal) {
  CRect inner = InnerRect();
  CRect r;  // Utility rectangles

  // Create static text fields:
  r.Set(0, 0, 40, controlHeight_Text);
  r.Offset(inner.left, inner.top);
  if (RunningOSX())
    r.right += 20;
  else
    r.Offset(0, 3);
  new CTextControl(this, "Title", r);
  r.Offset(0, controlVDiff_Edit);
  new CTextControl(this, "Author", r);
  r.Offset(0, controlVDiff_Edit);
  r.right += 40;
  new CTextControl(this, "Description", r);

  // Create edit fields:
  r.Set(inner.left + 45, inner.top, inner.right,
        inner.top + controlHeight_Edit);
  if (RunningOSX()) r.left += 25;
  cedit_Title = new CEditControl(this, Info->title, r, colTitleLen);
  r.Offset(0, controlVDiff_Edit);
  cedit_Author = new CEditControl(this, Info->author, r, colAuthorLen);
  r.Offset(0, 2 * controlVDiff_Edit - 5);
  r.bottom = inner.bottom - 35;
  r.left = inner.left;
  cedit_Descr = new CEditControl(this, Info->descr, r, colDescrLen);
  cedit_Descr->wantsReturn = true;

  // Create checkbox
  r = inner;
  r.top = r.bottom - controlHeight_CheckBox;
  r.right = CancelRect().left - 5;
  r.Offset(0, -3);
  ccheck_ShowHeadings =
      new CCheckBox(this, "View as �chess publishing�",
                    (Info->flags & colInfoFlag_Publishing), r);

  // Create the OK and Cancel buttons last:
  cbutton_Cancel = new CPushButton(this, "Cancel", CancelRect());
  cbutton_Default =
      new CPushButton(this, "OK", DefaultRect(), true, !colLocked);
  SetDefaultButton(cbutton_Default);

  CurrControl(cedit_Title);
} /* CInfoDialog::CInfoDialog */
