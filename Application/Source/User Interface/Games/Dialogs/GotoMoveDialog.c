/**************************************************************************************************/
/*                                                                                                */
/* Module  : GOTOMOVEDIALOG.C */
/* Purpose : This module implements the goto move dialog. */
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

#include "GotoMoveDialog.h"

class CGotoMoveDialog : public CDialog {
 public:
  CGotoMoveDialog(CRect frame, COLOUR initPlayer, INT initMoveNo, INT lastMove);
  virtual void HandlePushButton(CPushButton *ctrl);

  INT min, max;
  LONG move;
  CEditControl *cedit_Move;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG */
/*                                                                                                */
/**************************************************************************************************/

INT GotoMoveDialog(COLOUR initPlayer, INT initMoveNo, INT lastMove) {
  CRect frame(0, 0, 290, 70);
  if (RunningOSX()) frame.right += 65, frame.bottom += 30;
  theApp->CentralizeRect(&frame);

  CGotoMoveDialog *dialog =
      new CGotoMoveDialog(frame, initPlayer, initMoveNo, lastMove);
  dialog->Run();
  INT move = (dialog->reply == cdialogReply_OK ? dialog->move : -1);
  delete dialog;

  return (move >= 1 ? Max(0, (move - initMoveNo) * 2 - (initPlayer >> 4)) : -1);
} /* PromotionDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor
 * ------------------------------------------*/

CGotoMoveDialog::CGotoMoveDialog(CRect frame, COLOUR initPlayer, INT initMoveNo,
                                 INT lastMove)
    : CDialog(nil, "Goto Move", frame, cdialogType_Modal) {
  CRect inner = InnerRect();

  if (initPlayer == black) lastMove--;
  min = initMoveNo;
  max = initMoveNo + (lastMove + (initPlayer >> 4)) / 2;

  CRect r = inner;
  CHAR prompt[100];
  r.bottom = r.top + 2 * controlHeight_Text;
  r.right -= 48;
  Format(prompt, "Enter a move number between %d and %d", min, max);
  new CTextControl(this, prompt, r);

  r = inner;
  r.bottom = r.top + controlHeight_Edit;
  r.left = r.right - 43;
  if (!RunningOSX()) r.Offset(0, -3);
  cedit_Move = new CEditControl(this, "", r, 3);

  cbutton_Cancel = new CPushButton(this, "Cancel", CancelRect());
  cbutton_Default = new CPushButton(this, "OK", DefaultRect());
  SetDefaultButton(cbutton_Default);

  CurrControl(cedit_Move);
} /* CGotoMoveDialog::CGotoMoveDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void CGotoMoveDialog::HandlePushButton(CPushButton *ctrl) {
  // Perform validation if user clicks "OK":

  if (ctrl == cbutton_Default) {
    if (!cedit_Move->GetLong(&move) || move < min || move > max) {
      CurrControl(cedit_Move);
      CHAR s[100];
      Format(s, "Please enter a valid move number between %d and %d...", min,
             max);
      NoteDialog(this, "Invalid Move Number", s, cdialogIcon_Error);
      return;
    }
  }

  // Validation succeeded (or user pressed "Cancel") -> Call inherited function:
  CDialog::HandlePushButton(ctrl);
} /* CGotoMoveDialog::HandlePushButton */
