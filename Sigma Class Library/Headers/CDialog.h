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

#include "CControl.h"
#include "CWindow.h"
#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

enum CDIALOG_TYPE {
  cdialogType_Modal = cwindowType_ModalDialog,
  cdialogType_Modeless = cwindowType_ModelessDialog,
  cdialogType_Plain = cwindowType_Plain,
  cdialogType_Sheet = cwindowType_Sheet
};

enum CDIALOG_REPLY {
  cdialogReply_None = 0,
  cdialogReply_OK = 1,
  cdialogReply_Cancel = 2,
  cdialogReply_No = 3,

  cdialogReply_Close = 1
};

enum CDIALOG_ICON {
  cdialogIcon_Standard = 1,
  cdialogIcon_Warning = 2,
  cdialogIcon_Error = 0
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class CDialog : public CWindow {
 public:
  CDialog(CWindow *parent, CHAR *title, CRect frame,
          CDIALOG_TYPE type = cdialogType_Modal);

  void Run(void);
  void Open(void);   // Only call if Run() isn't called.
  void Close(void);  // Stops the run loop of a modal dialog
  BOOL IsModal(void);

  void SetDefaultButton(CPushButton *defaultButton);
  void SetCancelButton(CPushButton *cancelButton);

  virtual void HandlePushButton(CPushButton *ctl);
  virtual void HandleKeyDown(CHAR c, INT key,
                             INT modifiers);  // If dialog -> ctrl with focus
  virtual void HandleMenuAdjust(void);

  CRect InnerRect(void);
  CRect DefaultRect(void);
  CRect CancelRect(void);

  CPushButton *cbutton_Default, *cbutton_Cancel, *cbutton_No;
  CDIALOG_REPLY reply;
};

class CNoteDialog : public CDialog {
 public:
  CNoteDialog(CWindow *parent, CHAR *title, CRect frame, CHAR *text,
              INT icon = cdialogIcon_Standard, CHAR *OKStr = "OK");

  CTextControl *ctext_Prompt;
  CIconControl *cicon_Icon;
};

class CReminderDialog : public CDialog {
 public:
  CReminderDialog(CWindow *parent, CHAR *title, CRect frame, CHAR *text,
                  INT icon = cdialogIcon_Standard, CHAR *OKStr = "OK");

  CTextControl *ctext_Prompt;
  CIconControl *cicon_Icon;
  CCheckBox *ccheck_DontRemind;
};

class CQuestionDialog : public CDialog {
 public:
  CQuestionDialog(CWindow *parent, CHAR *title, CRect frame, CHAR *text,
                  INT icon = cdialogIcon_Standard, CHAR *OKStr = "OK",
                  CHAR *CancelStr = "Cancel");

  CTextControl *ctext_Prompt;
  CIconControl *cicon_Icon;
};

class CConfirmDialog : public CDialog {
 public:
  CConfirmDialog(CWindow *parent, CHAR *title, CRect frame, CHAR *text,
                 INT icon = cdialogIcon_Standard, CHAR *OKStr = "OK",
                 CHAR *CancelStr = "Cancel", CHAR *NoStr = "No");

  CTextControl *ctext_Prompt;
  CIconControl *cicon_Icon;
};

class CSearchReplaceDialog : public CDialog {
 public:
  CSearchReplaceDialog(CRect frame, CHAR *searchStr, CHAR *replaceStr,
                       BOOL caseSensitive);

  virtual void HandleEditControl(CEditControl *ctrl, BOOL textChanged,
                                 BOOL selChanged);

  CEditControl *cedit_Search, *cedit_Replace;
  CCheckBox *ccheck_Case;
};

class CProgressDialog : public CDialog {
 public:
  CProgressDialog(CWindow *parent, CHAR *title, CRect frame, CHAR *prompt,
                  ULONG max, BOOL indeterminate);
  virtual void HandlePushButton(CPushButton *ctl);

  void Set(ULONG n, CHAR *status);
  BOOL Aborted(void);

 private:
  BOOL aborted;

  CPushButton *cpush_Stop;
  CTextControl *ctext_Prompt;
  CTextControl *ctext_Status;
  CProgressBar *cprog_Progress;
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

void NoteDialog(CWindow *parent, CHAR *title, CHAR *text,
                INT icon = cdialogIcon_Standard, CHAR *OKStr = "OK");
BOOL ReminderDialog(CWindow *parent, CHAR *title, CHAR *text,
                    INT icon = cdialogIcon_Standard, CHAR *OKStr = "OK");
BOOL QuestionDialog(CWindow *parent, CHAR *title, CHAR *text,
                    CHAR *OKStr = "OK", CHAR *cancelStr = "Cancel");
CDIALOG_REPLY ConfirmDialog(CWindow *parent, CHAR *title, CHAR *text);
BOOL SearchReplaceDialog(CHAR *searchStr, CHAR *replaceStr,
                         BOOL *caseSensitive);
CProgressDialog *ProgressDialog_Open(CWindow *parent, CHAR *title, CHAR *prompt,
                                     ULONG max, BOOL indeterminate = false);
