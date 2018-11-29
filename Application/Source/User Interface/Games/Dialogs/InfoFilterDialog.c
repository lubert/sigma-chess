/**************************************************************************************************/
/*                                                                                                */
/* Module  : INFOFILTERDIALOG.C */
/* Purpose : This module implements the info filter dialog. */
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

#include "InfoFilterDialog.h"
#include "SigmaApplication.h"

/*----------------------------------------- Dialog Class
 * -----------------------------------------*/

class CInfoFilterDialog : public CDialog {
 public:
  CInfoFilterDialog(CRect frame, GAMEINFOFILTER *Filter);
  void ProcessResult(GAMEINFOFILTER *Filter);
  virtual void HandlePushButton(CPushButton *ctl);

  CPushButton *cbutton_AllOn, *cbutton_AllOff;
  CCheckBox *ccheck_Players, *ccheck_Event, *ccheck_Site, *ccheck_Date,
      *ccheck_Round, *ccheck_Result, *ccheck_ECO;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG */
/*                                                                                                */
/**************************************************************************************************/

BOOL GameInfoFilterDialog(GAMEINFOFILTER *Filter) {
  CRect frame(0, 0, 330, 165);
  if (RunningOSX()) frame.right += 50, frame.bottom += 35;
  theApp->CentralizeRect(&frame);

  CInfoFilterDialog *dialog = new CInfoFilterDialog(frame, Filter);
  dialog->Run();

  BOOL ok = (dialog->reply == cdialogReply_OK);
  if (ok) dialog->ProcessResult(Filter);

  delete dialog;
  return ok;
} /* GameInfoFilterDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor
 * ------------------------------------------*/

CInfoFilterDialog::CInfoFilterDialog(CRect frame, GAMEINFOFILTER *Filter)
    : CDialog(nil, "Game Info Filter", frame, cdialogType_Modal) {
  CRect inner = InnerRect();

  // Create static text controls:
  CRect r = inner;
  r.bottom = r.top + (RunningOSX() ? 35 : 30);
  new CTextControl(this,
                   "Select the set of game information to be shown in the move "
                   "list (and when printing)",
                   r, true, controlFont_Views);
  inner.top = r.bottom + 5;

  INT dv = (RunningOSX() ? 22 : 20);
  r.Set(0, 0, (RunningOSX() ? 130 : 100), controlHeight_Text);
  r.Offset(inner.left + 20, inner.top);
  ccheck_Players = new CCheckBox(this, "Players & ELO", Filter->players, r);
  r.Offset(0, dv);
  ccheck_Event = new CCheckBox(this, "Event", Filter->event, r);
  r.Offset(0, dv);
  ccheck_Site = new CCheckBox(this, "Site", Filter->site, r);
  r.Offset(0, dv);
  ccheck_Date = new CCheckBox(this, "Date", Filter->date, r);
  r.Offset(r.Width() + 20, -3 * dv);
  ccheck_Round = new CCheckBox(this, "Round", Filter->round, r);
  r.Offset(0, dv);
  ccheck_Result = new CCheckBox(this, "Result", Filter->result, r);
  r.Offset(0, dv);
  ccheck_ECO = new CCheckBox(this, "ECO", Filter->ECO, r);

  r = CancelRect();
  r.Offset(-r.left + InnerRect().left, 0);
  cbutton_AllOn = new CPushButton(this, "All On", r);
  r.Offset(80, 0);
  cbutton_AllOff = new CPushButton(this, "All Off", r);
  r.Offset(80, 0);
  cbutton_Cancel = new CPushButton(this, "Cancel", CancelRect());
  cbutton_Default = new CPushButton(this, "OK", DefaultRect());
  SetDefaultButton(cbutton_Default);

  CurrControl(ccheck_Players);
} /* CInfoFilterDialog::CInfoFilterDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void CInfoFilterDialog::HandlePushButton(CPushButton *ctrl) {
  if (ctrl == cbutton_AllOn) {
    ccheck_Players->Check(true);
    ccheck_Event->Check(true);
    ccheck_Site->Check(true);
    ccheck_Date->Check(true);
    ccheck_Round->Check(true);
    ccheck_Result->Check(true);
    ccheck_ECO->Check(true);
    //    ccheck_Annotator->Check(true);
  } else if (ctrl == cbutton_AllOff) {
    ccheck_Players->Check(false);
    ccheck_Event->Check(false);
    ccheck_Site->Check(false);
    ccheck_Date->Check(false);
    ccheck_Round->Check(false);
    ccheck_Result->Check(false);
    ccheck_ECO->Check(false);
    //    ccheck_Annotator->Check(false);
  }

  CDialog::HandlePushButton(ctrl);
} /* CInfoFilterDialog::HandlePushButton */

void CInfoFilterDialog::ProcessResult(GAMEINFOFILTER *Filter) {
  Filter->players = ccheck_Players->Checked();
  Filter->event = ccheck_Event->Checked();
  Filter->site = ccheck_Site->Checked();
  Filter->date = ccheck_Date->Checked();
  Filter->round = ccheck_Round->Checked();
  Filter->result = ccheck_Result->Checked();
  Filter->ECO = ccheck_ECO->Checked();
  // Filter->annotator = ccheck_Annotator->Checked();
} /* CInfoFilterDialog::ProcessResult */
