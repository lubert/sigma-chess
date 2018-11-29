/**************************************************************************************************/
/*                                                                                                */
/* Module  : INITIALSTATUSDIALOG.C */
/* Purpose : This module implements the Initial Position Status dialog used in
 * the pos editor.    */
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

#include "InitialStatusDialog.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                 INITIAL POSITION STATUS DIALOG */
/*                                                                                                */
/**************************************************************************************************/

class CStatusDialog : public CDialog {
 public:
  CStatusDialog(CRect frame, INITGAME *Init);
  void ProcessResult(void);

  virtual void HandlePushButton(CPushButton *ctrl);

  INITGAME *Init;

  CEditControl *cedit_MoveNo, *cedit_RevMoves, *cedit_EP;
  CCheckBox *ccheck_WO_O, *ccheck_WO_O_O, *ccheck_BO_O, *ccheck_BO_O_O;
};

void InitialStatusDialog(CGame *game) {
  CRect frame(0, 0, 300, 215);
  if (RunningOSX()) frame.bottom += 30;
  theApp->CentralizeRect(&frame);

  CStatusDialog *dialog = new CStatusDialog(frame, &(game->Init));
  dialog->Run();
  dialog->ProcessResult();
  delete dialog;
} /* InitialStatusDialog */

/*----------------------------------- Constructor/Destructor
 * -------------------------------------*/

CStatusDialog::CStatusDialog(CRect frame, INITGAME *theInit)
    : CDialog(nil, "Initial Position Status", frame, cdialogType_Modal) {
  CRect inner = InnerRect();
  Init = theInit;

  INT dv = 30;
  CRect rText(0, 0, 150, 35);
  rText.Offset(inner.left, inner.top);
  new CTextControl(this, "Initial move number", rText);
  rText.Offset(0, dv - 8);
  new CTextControl(this, "Half moves since last capture/pawn move", rText);
  rText.Offset(0, dv + 8);
  new CTextControl(this, "En passant square", rText);

  CRect rEdit(0, 0, 40, controlHeight_Edit);
  rEdit.Offset(rText.right + 5, inner.top);
  CHAR moveNo[4], revMoves[4], epSquare[3];
  NumToStr(Init->moveNo, moveNo);
  NumToStr(Init->revMoves, revMoves);
  CalcSquareStr(Init->epSquare, epSquare);
  cedit_MoveNo = new CEditControl(this, moveNo, rEdit, 3);
  rEdit.Offset(0, dv);
  cedit_RevMoves = new CEditControl(this, revMoves, rEdit, 3);
  rEdit.Offset(0, dv);
  cedit_EP = new CEditControl(this, epSquare, rEdit, 2);

  CRect rGroup = inner;
  rGroup.top = rEdit.bottom + 15;
  // rGroup.left = rGroup.right - (RunningOSX() ? 140 : 110);
  rGroup.bottom = DefaultRect().top - 15;

  dv = (RunningOSX() ? 22 : 20);
  CRect rCheck(0, 0, (RunningOSX() ? 110 : 90), controlHeight_CheckBox);
  rCheck.Offset(rGroup.left + 10, rGroup.top + 25);
  if (!RunningOSX()) rCheck.Offset(0, -8);
  ccheck_WO_O = new CCheckBox(this, "White O-O",
                              (Init->castlingRights & castRight_WO_O), rCheck);
  rCheck.Offset(0, dv);
  ccheck_WO_O_O = new CCheckBox(
      this, "White O-O-O", (Init->castlingRights & castRight_WO_O_O), rCheck);
  rCheck.Offset(rCheck.Width() + 20, -dv);
  ccheck_BO_O = new CCheckBox(this, "Black O-O",
                              (Init->castlingRights & castRight_BO_O), rCheck);
  rCheck.Offset(0, dv);
  ccheck_BO_O_O = new CCheckBox(
      this, "Black O-O-O", (Init->castlingRights & castRight_BO_O_O), rCheck);
  new CGroupBox(this, "Castling Rights", rGroup);

  cbutton_Cancel = new CPushButton(this, "Cancel", CancelRect());
  cbutton_Default = new CPushButton(this, "OK", DefaultRect());
  SetDefaultButton(cbutton_Default);
} /* CStatusDialog::CStatusDialog */

/*-------------------------------------- Event Handling
 * ------------------------------------------*/

void CStatusDialog::HandlePushButton(CPushButton *ctrl) {
  if (ctrl == cbutton_Default) {
    if (!cedit_MoveNo->ValidateNumber(1, gameRecSize / 2)) {
      CurrControl(cedit_MoveNo);
      NoteDialog(this, "Invalid Move Number",
                 "The Initial move number must be a number between 1 and 400.",
                 cdialogIcon_Error);
      return;
    }

    if (!cedit_RevMoves->ValidateNumber(0, 100))  //### or should it be 99???
    {
      CurrControl(cedit_RevMoves);
      NoteDialog(this, "Invalid Number of Moves",
                 "At most 100 half moves can be played without pawn moves or "
                 "captures.",
                 cdialogIcon_Error);
      return;
    }

    BOOL epValid = true;
    CHAR s[3];
    cedit_EP->GetTitle(s);

    if (StrLen(s) != 0)
      if (StrLen(s) != 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' ||
          s[1] > '8')
        epValid = false;
      else {
        SQUARE epSquare = ParseSquareStr(s);
        epValid =
            (rank(epSquare) == (Init->player == white ? 5 : 2)) &&
            (Init->Board[epSquare + (Init->player == white ? -0x10 : 0x10)] ==
             pawn + (black - Init->player));
      }

    if (!epValid) {
      CurrControl(cedit_EP);
      NoteDialog(
          this, "Invalid En Passant Square",
          "The �En passant square� field must either be empty, or specify an "
          "empty square behind an enemy pawn on the 6th rank.",
          cdialogIcon_Error);
      return;
    }
  }

  // If all validation succeeds ("OK" button only), proceed with default
  // handling for button:
  CDialog::HandlePushButton(ctrl);
} /* CStatusDialog::HandlePushbutton */

/*------------------------------------ Process Dialog Result
 * -------------------------------------*/

void CStatusDialog::ProcessResult(void) {
  if (reply != cdialogReply_OK) return;  // Do nothing if "Cancel" pressed.

  // Store castling rights:
  Init->castlingRights = 0;
  if (ccheck_WO_O->Checked()) Init->castlingRights |= castRight_WO_O;
  if (ccheck_WO_O_O->Checked()) Init->castlingRights |= castRight_WO_O_O;
  if (ccheck_BO_O->Checked()) Init->castlingRights |= castRight_BO_O;
  if (ccheck_BO_O_O->Checked()) Init->castlingRights |= castRight_BO_O_O;

  // Store moveNo and half moves since last irreversible move:
  LONG n;
  Init->moveNo = (cedit_MoveNo->GetLong(&n) ? n : 1);
  Init->revMoves = (cedit_RevMoves->GetLong(&n) ? n : 0);

  // Store EP square (if any):
  CHAR s[3];
  cedit_EP->GetTitle(s);
  Init->epSquare = ParseSquareStr(s);
} /* CStatusDialog::ProcessResult */
