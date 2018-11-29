/**************************************************************************************************/
/*                                                                                                */
/* Module  : UCI_PROGRESSDIALOG.C */
/* Purpose : This module implements the generic UCI progress dialog. */
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

#include "UCI_ProgressDialog.h"
#include "CApplication.h"
#include "CDialog.h"
#include "SigmaPrefs.h"
#include "UCI.h"

#include "Debug.h"

/*----------------------------------------- Dialog Class
 * -----------------------------------------*/

static BOOL uciProgressIsOpen = false;
static CProgressDialog *uciProgressDlg = nil;
static CHAR uciProgressTitle[256];
static CHAR uciProgressMsg[1024];
static ULONG uciProgressTimeOut = 0;

/**************************************************************************************************/
/*                                                                                                */
/*                                      OPEN PROGRESS DIALOG */
/*                                                                                                */
/**************************************************************************************************/

void UCI_ProgressDialogOpen(CHAR *title, CHAR *message, BOOL withCancelButton,
                            INT timeOutSecs) {
  if (uciProgressIsOpen) return;

  uciProgressIsOpen = true;
  CopyStr(title, uciProgressTitle);
  CopyStr(message, uciProgressMsg);
  uciProgressTimeOut = Timer() + 60 * timeOutSecs;
  uciProgressDlg =
      ProgressDialog_Open(nil, uciProgressTitle, uciProgressMsg, 0, true);

  theApp->ModalLoopBegin();

  if (debugOn) {
    DebugWriteNL("--- OPENING UCI PROGRESS DIALOG ---");
    DebugWriteNL(title);
    DebugWriteNL(message);
  }
}  // UCI_ProgressDialogOpen

/**************************************************************************************************/
/*                                                                                                */
/*                                      OPEN PROGRESS DIALOG */
/*                                                                                                */
/**************************************************************************************************/

void UCI_ProgressDialogClose(void) {
  if (!uciProgressIsOpen) return;

  if (debugOn) DebugWriteNL("--- CLOSING UCI PROGRESS DIALOG ---");

  if (uciProgressDlg) delete uciProgressDlg;
  uciProgressDlg = nil;
  uciProgressIsOpen = false;

  theApp->ModalLoopEnd();
}  // UCI_ProgressDialogClose

/**************************************************************************************************/
/*                                                                                                */
/*                                CHECK IF PROGRESS DIALOG CANCELLED */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_ProgressDialogCancelled(void)  // Should be called repeatedly
{
  if (uciProgressDlg) uciProgressDlg->Set(1, "");

  return (uciProgressDlg && uciProgressDlg->Aborted() ||
          Timer() > uciProgressTimeOut);
}  // UCI_ProgressDialogCancelled

void UCI_ProgressDialogResetTimeOut(INT timeOutSecs) {
  uciProgressTimeOut = Timer() + 60 * timeOutSecs;
}  // UCI_ProgressDialogResetTimeOut
