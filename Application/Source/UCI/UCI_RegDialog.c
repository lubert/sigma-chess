/**************************************************************************************************/
/*                                                                                                */
/* Module  : UCI_REGDIALOG.C */
/* Purpose : This module implements the interface to the UCI engines. */
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

#include "UCI_RegDialog.h"
#include "CApplication.h"
#include "CDialog.h"
#include "CFile.h"
#include "GameWindow.h"
#include "SigmaApplication.h"
#include "SigmaPrefs.h"
#include "SigmaWindow.h"
#include "UCI.h"
#include "UCI_Option.h"

/*----------------------------------------- Dialog Class
 * -----------------------------------------*/

class CUCI_RegDialog : public CDialog {
 public:
  CUCI_RegDialog(CWindow *parent, CHAR *title, CRect frame, CHAR *engineName,
                 CHAR *engineAbout);

  virtual void HandleEditControl(CEditControl *ctrl, BOOL textChanged,
                                 BOOL selChanged);

  CIconControl *cicon_Icon;
  CEditControl *cedit_Name;
  CEditControl *cedit_Code;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                   RUN UCI REGISTRATION DIALOG */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_RegistrationDialog(CWindow *parent, CHAR *engineName,
                            CHAR *engineAbout, CHAR *name, CHAR *code) {
  CRect frame(0, 0, 470, 250);
  theApp->CentralizeRect(&frame);
  CHAR title[100];
  Format(title, "Register %s", engineName);
  CUCI_RegDialog *dialog =
      new CUCI_RegDialog(parent, title, frame, engineName, engineAbout);
  dialog->Run();

  BOOL ok = false;
  if (dialog->reply == cdialogReply_OK) {
    dialog->cedit_Name->GetText(name);
    dialog->cedit_Code->GetText(code);
    ok = true;
  }

  delete dialog;

  return ok;
}  // UCI_RegistrationDialog

/**************************************************************************************************/
/*                                                                                                */
/*                                           CONSTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CUCI_RegDialog::CUCI_RegDialog(CWindow *parent, CHAR *title, CRect frame,
                               CHAR *engineName, CHAR *engineAbout)
    : CDialog(parent, title, frame) {
  CRect inner = InnerRect();

  // Key/code icon
  CRect ri(0, 0, 32, 32);
  ri.Offset(InnerRect().left, InnerRect().top);
  new CIconControl(this, 2000, ri);

  CRect rt = ri;
  rt.top -= 3;
  rt.bottom = rt.top + 3 * controlHeight_Text + 10;
  rt.left = InnerRect().left + 52;
  rt.right = InnerRect().right;
  new CTextControl(this, engineAbout, rt, true, controlFont_SmallSystem);

  rt.Offset(0, rt.Height() + 5);
  rt.bottom = rt.top + 2 * controlHeight_Text;
  CHAR prompt[500];
  Format(prompt,
         "Please enter your name and license code below and click the "
         "'Register' button.",
         engineName);
  CTextControl *ctext =
      new CTextControl(this, prompt, rt, true, controlFont_SmallSystem);

  CRect rd = InnerRect();
  rd.top = rt.bottom + 2;
  rd.bottom = rd.top + 2;

  CRect r = InnerRect();
  r.right = r.left + 50;
  r.top = rd.bottom;
  r.bottom = r.top + controlHeight_Text;
  new CTextControl(this, "Name", r);
  r.Offset(0, controlVDiff_Edit);
  new CTextControl(this, "Code", r);
  r.Offset(0, controlVDiff_Edit);

  r = InnerRect();
  r.left += 55;
  r.top = rd.bottom;
  r.bottom = r.top + controlHeight_Edit;
  cedit_Name = new CEditControl(this, "", r, uci_UserNameLen);
  r.Offset(0, controlVDiff_Edit);
  cedit_Code = new CEditControl(this, "", r, uci_UserRegCodeLen);
  r.Offset(0, controlVDiff_Edit);

  cbutton_Cancel = new CPushButton(this, "Later", CancelRect());
  cbutton_Default =
      new CPushButton(this, "Register", DefaultRect(), true, false);
  SetDefaultButton(cbutton_Default);

  CurrControl(cedit_Name);
}  // CUCI_RegDialog::CUCI_RegDialog

/**************************************************************************************************/
/*                                                                                                */
/*                                           VALIDATION */
/*                                                                                                */
/**************************************************************************************************/

void CUCI_RegDialog::HandleEditControl(CEditControl *ctrl, BOOL textChanged,
                                       BOOL selChanged) {
  if (textChanged) cbutton_Default->Enable(!cedit_Code->IsEmpty());
}  // CUCI_RegDialog::HandleEditControl
