/**************************************************************************************************/
/*                                                                                                */
/* Module  : CDialog.c */
/* Purpose : Implements the generic dialog class, which serves as the base class
 * for all user     */
/*           defined dialogs. */
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

#include "CDialog.h"
#include "CApplication.h"
#include "CUtility.h"

#define sheetTitleAreaHeight 30

/**************************************************************************************************/
/*                                                                                                */
/*                                          CONSTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CDialog::CDialog(CWindow *parent, CHAR *title, CRect frame, CDIALOG_TYPE type)
    : CWindow(parent, title,
              CRect(frame.left, frame.top, frame.right,
                    frame.bottom +
                        (type == cdialogType_Sheet && RunningOSX() && parent
                             ? sheetTitleAreaHeight
                             : 0)),
              (CWINDOW_TYPE)type, false) {
  reply = cdialogReply_None;
  cbutton_Default = cbutton_Cancel = cbutton_No = nil;

  if (RunningOSX() && type == cdialogType_Sheet) {
    // For sheets "replace" title with static text control (bold)
    CRect rt = InnerRect();
    rt.top -= sheetTitleAreaHeight + 3;
    rt.bottom = rt.top + controlHeight_Text;
    new CTextControl(this, title, rt, (CCONTROL_FONT)kThemeWindowTitleFont);
    rt.top = rt.bottom + 2;
    rt.bottom = rt.top + 2;
    new CDivider(this, rt);
  }
} /* CDialog::CDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                     RUN MODAL DIALOG LOOP */
/*                                                                                                */
/**************************************************************************************************/

// Don't allow sheets if modal dialog already open and this is NOT the parent of
// the new sheet. Otherwise we get a deadlock!

void CDialog::Run(void) {
  BOOL openAsSheet =
      (RunningOSX() && winType == cwindowType_Sheet && winParent);
  CWindow *frontWin = theApp->GetFrontWindow();

  if (openAsSheet && frontWin && frontWin->IsModalDialog() &&
      winParent != frontWin)
    openAsSheet = false;

  if (openAsSheet) {
    ::ShowSheetWindow(winRef, winParent->winRef);
    winParent->sheetChild = this;
  } else {
    Open();
  }

  theApp->ModalLoopBegin();
  {
    modalRunning = true;
    while (modalRunning) {
      theApp->ProcessEvents();
      if (!RunningOSX()) ::IdleControls(winRef);
    }
  }
  theApp->ModalLoopEnd();

  if (openAsSheet) {
    ::HideSheetWindow(winRef);
    winParent->sheetChild = nil;
  }
} /* CDialog::Run */

void CDialog::Open(void) {
  ::SelectWindow(winRef);
  ::ShowWindow(winRef);
  ::DrawControls(winRef);
} /* CDialog::Open */

void CDialog::Close(void) { modalRunning = false; } /* CDialog::Close */

BOOL CDialog::IsModal(void) {
  return (winType == cdialogType_Modal || winType == cdialogType_Sheet);
} /* CDialog::IsModal */

/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void CDialog::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  // Implements tabbing between controls as well as default mapping of Enter and
  // Escape keys to the "OK" and "Cancel" buttons.

  if (key == key_Tab) {
    if (!focusCtl) return;
    if (modifiers & modifier_Shift)
      PrevControl();
    else
      NextControl();
    return;
  }

  CControl *ctl = nil;

  if ((key == key_Enter || key == key_Return) &&
      !(focusCtl && focusCtl->Enabled() && focusCtl->wantsReturn))
    ctl = cbutton_Default;
  else if (key == key_Escape)
    ctl = cbutton_Cancel;
  else if (modifiers & modifier_Command) {
    if (c == 'd' || c == 'D')
      ctl = cbutton_No;
    else if (c == '.')
      ctl = cbutton_Cancel;
  }

  if (ctl && ctl->type == controlType_PushButton && ctl->Enabled() &&
      ctl->Visible())
    ((CPushButton *)ctl)->Press();
  else
    CWindow::HandleKeyDown(c, key, modifiers);
  //### else if (focusCtl && focusCtl->Enabled())
  //###      focusCtl->HandleKeyDown(c, key, modifiers);

} /* CDialog::HandleKeyDown */

void CDialog::HandleMenuAdjust(void) {
  if (IsModal()) {
    theApp->EnableQuitCmd(false);
    theApp->EnablePrefsCmd(false);
    theApp->EnableAboutCmd(false);
    theApp->EnableMenuBar(false);
  }
} /* CDialog::HandleMenuAdjust */

/*------------------------------ Implement Default Control Handling
 * ------------------------------*/

void CDialog::HandlePushButton(CPushButton *ctrl) {
  if (!modalRunning) return;
  if (ctrl == cbutton_Default)
    reply = cdialogReply_OK;
  else if (ctrl == cbutton_Cancel)
    reply = cdialogReply_Cancel;
  else if (ctrl == cbutton_No)
    reply = cdialogReply_No;
  else
    return;

  modalRunning = false;
  //   Close();
} /* CDialog::HandlePushButton */

void CDialog::SetDefaultButton(CPushButton *ctl) {
  ::SetWindowDefaultButton(winRef, ctl->ch);

  if (ctl != cbutton_Default) {
    CPushButton *ctl0 = cbutton_Default;
    cbutton_Default = ctl;
    ctl0->Redraw();
    ctl->Redraw();
  }
} /* CDialog::SetDefaultButton */

void CDialog::SetCancelButton(CPushButton *ctl) {
  ::SetWindowCancelButton(winRef, ctl->ch);

  cbutton_Cancel = ctl;
} /* CDialog::SetCancelButton */

/**************************************************************************************************/
/*                                                                                                */
/*                                           UTILITY */
/*                                                                                                */
/**************************************************************************************************/

CRect CDialog::InnerRect(void) {
  CRect r = Bounds();

  if (!RunningOSX()) {
    r.Inset(10, 10);
    r.top -= 2;
  } else {
    r.Inset(20, 20);
    if (winType == cdialogType_Sheet) r.top += sheetTitleAreaHeight;
  }

  return r;
} /* CDialog::InnerRect */

CRect CDialog::DefaultRect(void) {
  CRect r(0, 0, controlWidth_PushButton, controlHeight_PushButton);
  r.Offset(InnerRect().right - r.Width(), InnerRect().bottom - r.Height());
  return r;
} /* CDialog::DefaultRect */

CRect CDialog::CancelRect(void) {
  CRect r = DefaultRect();
  r.Offset(-r.Width() - 10, 0);
  if (RunningOSX()) r.Offset(-2, 0);  // In Aqua space should be 12 pixels
  return r;
} /* CDialog::CancelRect */

/**************************************************************************************************/
/*                                                                                                */
/*                                       STANDARD DIALOGS */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Note Dialog
 * ------------------------------------------*/

CNoteDialog::CNoteDialog(CWindow *parent, CHAR *title, CRect frame, CHAR *text,
                         INT icon, CHAR *OKStr)
    : CDialog(parent, title, frame,
              (parent ? cdialogType_Sheet : cdialogType_Modal)) {
  CRect inner = InnerRect();
  CRect rIcon(0, 0, 32, 32);
  rIcon.Offset(inner.left, inner.top);
  CRect rText(rIcon.right + 10, inner.top, inner.right, DefaultRect().top - 5);

  cbutton_Default = new CPushButton(this, OKStr, DefaultRect());
  ctext_Prompt =
      new CTextControl(this, text, rText, true, controlFont_SmallSystem);
  cicon_Icon = new CIconControl(this, icon, rIcon);

  SetDefaultButton(cbutton_Default);
} /* CNoteDialog::CNoteDialog */

void NoteDialog(CWindow *parent, CHAR *title, CHAR *text, INT icon,
                CHAR *OKStr) {
  if (icon == cdialogIcon_Error) Beep(1);

  CRect frame(0, 0, 300, 100);
  if (RunningOSX()) frame.right += 40, frame.bottom += 15;
  if (StrLen(text) > 150) {
    frame.bottom += 20;
    if (RunningOSX()) frame.right += 50;
  }
  theApp->CentralizeRect(&frame);

  CNoteDialog *dialog =
      new CNoteDialog(parent, title, frame, text, icon, OKStr);
  dialog->cbutton_Default->acceptsFocus = false;
  dialog->focusCtl = nil;
  dialog->Run();
  delete dialog;
} /* NoteDialog */

/*--------------------------------------- Reminder Dialog
 * ----------------------------------------*/

CReminderDialog::CReminderDialog(CWindow *parent, CHAR *title, CRect frame,
                                 CHAR *text, INT icon, CHAR *OKStr)
    : CDialog(parent, title, frame,
              (parent ? cdialogType_Sheet : cdialogType_Modal)) {
  CRect inner = InnerRect();
  CRect rIcon(0, 0, 32, 32);
  rIcon.Offset(inner.left, inner.top);
  CRect rText(rIcon.right + 10, inner.top, inner.right, DefaultRect().top - 5);
  CRect rChk(rText.left, inner.bottom - controlHeight_CheckBox - 1,
             DefaultRect().left - 10, inner.bottom - 1);

  cbutton_Default = new CPushButton(this, OKStr, DefaultRect());
  ctext_Prompt =
      new CTextControl(this, text, rText, true, controlFont_SmallSystem);
  cicon_Icon = new CIconControl(this, icon, rIcon);
  ccheck_DontRemind = new CCheckBox(this, "Don't remind me again", false, rChk);

  SetDefaultButton(cbutton_Default);
} /* CReminderDialog::CReminderDialog */

BOOL ReminderDialog(CWindow *parent, CHAR *title, CHAR *text, INT icon,
                    CHAR *OKStr) {
  CRect frame(0, 0, 330, 100);
  if (RunningOSX()) frame.right += 40, frame.bottom += 15;
  theApp->CentralizeRect(&frame);

  if (StrLen(text) > 120) frame.bottom += 20;
  CReminderDialog *dialog =
      new CReminderDialog(parent, title, frame, text, icon, OKStr);
  dialog->cbutton_Default->acceptsFocus = false;
  dialog->focusCtl = nil;
  dialog->Run();
  BOOL dontRemind = dialog->ccheck_DontRemind->Checked();
  delete dialog;

  return dontRemind;
} /* ReminderDialog */

/*-------------------------------------- Question Dialog
 * -----------------------------------------*/

CQuestionDialog::CQuestionDialog(CWindow *parent, CHAR *title, CRect frame,
                                 CHAR *text, INT icon, CHAR *OKStr,
                                 CHAR *CancelStr)
    : CDialog(parent, title, frame,
              (parent ? cdialogType_Sheet : cdialogType_Modal)) {
  CRect inner = InnerRect();
  CRect rIcon(0, 0, 32, 32);
  rIcon.Offset(inner.left, inner.top);
  CRect rText(rIcon.right + 10, inner.top, inner.right, DefaultRect().top - 5);
  CRect rCancel = CancelRect();
  if (StrLen(CancelStr) > 6) rCancel.left -= 15;

  cbutton_Default = new CPushButton(this, OKStr, DefaultRect());
  cbutton_Cancel = new CPushButton(this, CancelStr, rCancel);
  ctext_Prompt =
      new CTextControl(this, text, rText, true, controlFont_SmallSystem);

  // if (RunningOSX()) rIcon.Offset(0, -(sheetTitleAreaHeight + 3));
  cicon_Icon = new CIconControl(this, icon, rIcon);

  SetDefaultButton(cbutton_Default);
  reply = cdialogReply_None;
} /* CQuestionDialog::CQuestionDialog */

BOOL QuestionDialog(CWindow *parent, CHAR *title, CHAR *text, CHAR *OKStr,
                    CHAR *cancelStr) {
  CDIALOG_REPLY reply;

  CRect frame(0, 0, 300, 100);
  if (RunningOSX()) frame.right += 40, frame.bottom += 15;
  if (StrLen(text) > 120) {
    frame.bottom += 20;
    if (RunningOSX()) frame.right += 50;
  }

  if (theApp->GetFrontWindow())
    theApp->GetFrontWindow()->CentralizeRect(&frame);
  else
    theApp->CentralizeRect(&frame);

  CQuestionDialog *dialog = new CQuestionDialog(
      parent, title, frame, text, cdialogIcon_Standard, OKStr, cancelStr);
  dialog->Run();
  reply = dialog->reply;  // "Save" reply value before deleting dialog
  delete dialog;

  return (reply == cdialogReply_OK);
} /* QuestionDialog */

/*--------------------------------------- Confirm Dialog
 * -----------------------------------------*/

CConfirmDialog::CConfirmDialog(CWindow *parent, CHAR *title, CRect frame,
                               CHAR *text, INT icon, CHAR *OKStr,
                               CHAR *CancelStr, CHAR *NoStr)
    : CDialog(parent, title, frame,
              (parent ? cdialogType_Sheet : cdialogType_Modal)) {
  CRect inner = InnerRect();
  CRect rIcon(0, 0, 32, 32);
  rIcon.Offset(inner.left, inner.top);
  CRect rText(rIcon.right + 10, inner.top, inner.right, DefaultRect().top - 5);
  CRect rNo = DefaultRect();
  rNo.left = rText.left;
  rNo.right = rNo.left + (RunningOSX() ? 95 : 80);

  cbutton_Default = new CPushButton(this, OKStr, DefaultRect());
  cbutton_No = new CPushButton(this, NoStr, rNo);
  cbutton_Cancel = new CPushButton(this, CancelStr, CancelRect());
  ctext_Prompt =
      new CTextControl(this, text, rText, true, controlFont_SmallSystem);

  // if (RunningOSX()) rIcon.Offset(0, -(sheetTitleAreaHeight + 3));
  cicon_Icon = new CIconControl(this, icon, rIcon);

  SetDefaultButton(cbutton_Default);
  reply = cdialogReply_None;
} /* CConfirmDialog::CConfirmDialog */

CDIALOG_REPLY ConfirmDialog(CWindow *parent, CHAR *title, CHAR *text) {
  CDIALOG_REPLY reply;

  CRect frame(0, 0, 300, 100);
  if (RunningOSX()) frame.right += 40, frame.bottom += 15;
  theApp->CentralizeRect(&frame);

  CConfirmDialog *dialog = new CConfirmDialog(parent, title, frame, text);
  dialog->Run();
  reply = dialog->reply;  // "Save" reply value before deleting dialog
  delete dialog;

  return reply;
} /* ConfirmDialog */

/*------------------------------------ Search/Replace Dialog
 * -------------------------------------*/

CSearchReplaceDialog::CSearchReplaceDialog(CRect frame, CHAR *searchStr,
                                           CHAR *replaceStr, BOOL caseSensitive)
    : CDialog(nil, "Search/Replace", frame) {
  CRect rtext(0, 0, 70, controlHeight_Text);
  if (RunningOSX()) rtext.right += 15;
  rtext.Offset(InnerRect().left, InnerRect().top);
  new CTextControl(this, "Search for", rtext);
  rtext.Offset(0, controlVDiff_Edit);
  new CTextControl(this, "Replace with", rtext);
  rtext.Offset(0, 2 * controlVDiff_Edit);

  CRect redit(0, 0, 10, controlHeight_Edit);
  redit.Offset(rtext.right + 8, InnerRect().top - (RunningOSX() ? 0 : 3));
  redit.right = InnerRect().right;
  cedit_Search = new CEditControl(this, searchStr, redit, 30);
  redit.Offset(0, controlVDiff_Edit);
  cedit_Replace = new CEditControl(this, replaceStr, redit, 30);

  CRect rchk(0, 0, 150, controlHeight_CheckBox);
  rchk.Offset(InnerRect().left, InnerRect().bottom - controlHeight_CheckBox -
                                    (RunningOSX() ? 0 : 3));
  ccheck_Case = new CCheckBox(this, "Case Sensitive", caseSensitive, rchk);

  cbutton_Cancel = new CPushButton(this, "Cancel", CancelRect());
  cbutton_Default = new CPushButton(this, "Find", DefaultRect());

  reply = cdialogReply_None;
  cbutton_Default->Enable(!cedit_Search->IsEmpty());

  CurrControl(cedit_Search);
} /* CSearchReplaceDialog::CSearchReplaceDialog */

BOOL SearchReplaceDialog(CHAR *searchStr, CHAR *replaceStr,
                         BOOL *caseSensitive) {
  CDIALOG_REPLY reply;

  CRect frame(0, 0, 350, 95);
  if (RunningOSX()) frame.right += 40, frame.bottom += 30;
  theApp->CentralizeRect(&frame);

  CSearchReplaceDialog *dialog =
      new CSearchReplaceDialog(frame, searchStr, replaceStr, *caseSensitive);
  dialog->Run();

  reply = dialog->reply;
  if (reply == cdialogReply_OK) {
    dialog->cedit_Search->GetTitle(searchStr);
    dialog->cedit_Replace->GetTitle(replaceStr);
    *caseSensitive = dialog->ccheck_Case->Checked();
  }

  delete dialog;

  return (reply == cdialogReply_OK);
} /* SearchReplaceDialog */

void CSearchReplaceDialog::HandleEditControl(CEditControl *ctrl,
                                             BOOL textChanged,
                                             BOOL selChanged) {
  if (ctrl == cedit_Search) cbutton_Default->Enable(!cedit_Search->IsEmpty());
} /* CSearchReplaceDialog::HandleEditControl */

/*-------------------------------------- Progress Dialog
 * -----------------------------------------*/

CProgressDialog::CProgressDialog(CWindow *parent, CHAR *title, CRect frame,
                                 CHAR *prompt, ULONG max, BOOL indeterminate)
    : CDialog(parent, title, frame,
              (parent ? cdialogType_Sheet : cdialogType_Modal)) {
  CRect r = InnerRect();

  CRect rStop(r.right - controlWidth_PushButton,
              r.bottom - controlHeight_PushButton, r.right, r.bottom);
  CRect rPrompt(r.left, r.top, r.right, r.top + 40);
  CRect rStatus(r.left, rPrompt.bottom + 5, r.right,
                rPrompt.bottom + 5 + controlHeight_Text);
  CRect rProgress(r.left, r.bottom - controlHeight_ProgressBar, rStop.left - 10,
                  r.bottom);

  cpush_Stop = new CPushButton(this, "Stop", rStop);
  ctext_Prompt = new CTextControl(this, prompt, rPrompt);
  ctext_Status =
      new CTextControl(this, "", rStatus, true, controlFont_SmallSystem);
  cprog_Progress =
      new CProgressBar(this, rProgress, 0, max, true, indeterminate);

  aborted = false;
} /* CProgressDialog::CProgressDialog */

void CProgressDialog::Set(ULONG n, CHAR *status) {
  ctext_Status->SetTitle(status);
  cprog_Progress->Set(n);
  theApp->ProcessEvents();
} /* CProgressDialog::Set */

BOOL CProgressDialog::Aborted(void) {
  return aborted;
} /* CProgressDialog::Aborted */

void CProgressDialog::HandlePushButton(CPushButton *ctl) {
  if (ctl == cpush_Stop) aborted = true;
} /* CProgressDialog::HandlePushButton */

CProgressDialog *ProgressDialog_Open(CWindow *parent, CHAR *title, CHAR *prompt,
                                     ULONG max, BOOL indeterminate) {
  CRect frame(0, 0, 320, 100);
  if (RunningOSX()) frame.right += 70, frame.bottom += 30;
  theApp->CentralizeRect(&frame, true);
  CProgressDialog *progressDlg =
      new CProgressDialog(parent, title, frame, prompt, max, indeterminate);
  progressDlg->Show(true);
  progressDlg->Set(0, "");
  progressDlg->Open();
  return progressDlg;
} /* ProgressDialog_Open */
