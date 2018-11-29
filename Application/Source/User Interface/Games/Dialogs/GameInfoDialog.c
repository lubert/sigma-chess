/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAMEINFODIALOG.C                                                                     */
/* Purpose : This module implements the game info dialog.                                         */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
  and the following disclaimer in the documentation and/or other materials provided with the 
  distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "GameInfoDialog.h"
#include "SigmaApplication.h"


/*----------------------------------------- Dialog Class -----------------------------------------*/

class CGameInfoDialog : public CDialog
{
public:
   CGameInfoDialog (CWindow *parent, CRect frame, GAMEINFO *Info);
   void ProcessResult (GAMEINFO *Info);
   virtual void HandlePushButton (CPushButton *ctl);

   CPushButton  *cbutton_ClearAll;
   CCheckBox    *ccheck_Default;
   CEditControl *cedit_White, *cedit_Black, *cedit_WhiteELO, *cedit_BlackELO,
                *cedit_Event, *cedit_Site, *cedit_Date, *cedit_Round, *cedit_Result,
                *cedit_ECO, *cedit_Ann;
   CPopupMenu   *cpopup_Result;
   CMenu *resultMenu;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG                                      */
/*                                                                                                */
/**************************************************************************************************/

BOOL GameInfoDialog (CWindow *parent, GAMEINFO *Info)
{
   CRect frame(0, 0, 400, 270);
   if (RunningOSX()) frame.right += 100, frame.bottom += 80;
   theApp->CentralizeRect(&frame);

   CGameInfoDialog *dialog = new CGameInfoDialog(parent, frame, Info);
   dialog->Run();

   BOOL ok = (dialog->reply == cdialogReply_OK);
   if (ok) dialog->ProcessResult(Info);

   delete dialog;
   return ok;
} /* GameInfoDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor ------------------------------------------*/

CGameInfoDialog::CGameInfoDialog (CWindow *parent, CRect frame, GAMEINFO *Info)
 : CDialog (parent, "Game Info", frame, cdialogType_Modal)
{
   CRect inner = InnerRect();
   CRect r, r2;               // Generic control rectangle

   // Create the OK and Cancel buttons first:
   r.Set(0,0,100,controlHeight_CheckBox);
   if (RunningOSX()) r.right += 30;
   r.Offset(inner.left, inner.bottom - controlHeight_CheckBox - 3);
   ccheck_Default = new CCheckBox(this, "Set as default", false, r);

   CRect rClear = CancelRect(); rClear.left -= 15;
   rClear.Offset(-rClear.Width() - 10, 0);
   cbutton_ClearAll = new CPushButton(this, "Clear All", rClear);
   cbutton_Cancel   = new CPushButton(this, "Cancel", CancelRect());
   cbutton_Default  = new CPushButton(this, "OK", DefaultRect());

   // Create static text controls:
   INT rowdiff = (RunningOSX() ? 32 : 25);
   INT editLeft = (RunningOSX() ? 75 : 55);

   r.Set(0, 0, editLeft - 5, controlHeight_Text);
   r.Offset(inner.left, inner.top);
   if (! RunningOSX()) r.Offset(0, 3);
   new CTextControl(this, "White",  r); r.Offset(0, rowdiff);
   new CTextControl(this, "Black",  r); r.Offset(0, rowdiff);
   new CTextControl(this, "Event",  r); r.Offset(0, rowdiff);
   new CTextControl(this, "Site",   r); r.Offset(0, rowdiff);
   new CTextControl(this, "Date",   r); r.Offset(0, rowdiff);
   new CTextControl(this, "Round",  r); r.Offset(0, rowdiff);
   new CTextControl(this, "Result", r); r.Offset(0, rowdiff);
   new CTextControl(this, "ECO",    r); r.Offset(0, rowdiff);
   new CTextControl(this, "Annotator", r); r.Offset(0, rowdiff);

   r.Set(inner.right - 70, inner.top, inner.right - 45, inner.top + controlHeight_Text);
   if (! RunningOSX()) r.Offset(0, 3);
   else r.left -= 15, r.right -= 15;
   new CTextControl(this, "ELO", r); r.Offset(0, rowdiff);
   new CTextControl(this, "ELO", r);

   // Create edit controls:
   r.Set(inner.left + editLeft + 5, inner.top, inner.right - 85, inner.top + controlHeight_Edit);
   if (RunningOSX()) r.right -= 15;
   cedit_White = new CEditControl(this, Info->whiteName, r, nameStrLen); r.Offset(0, rowdiff);
   cedit_Black = new CEditControl(this, Info->blackName, r, nameStrLen); r.Offset(0, rowdiff);

   CHAR wELO[5], bELO[5];
   if (Info->whiteELO > 0) NumToStr(Info->whiteELO, wELO); else wELO[0] = 0;
   if (Info->blackELO > 0) NumToStr(Info->blackELO, bELO); else bELO[0] = 0;
   r2.Set(inner.right - 45, inner.top, inner.right, inner.top + controlHeight_Edit);
   cedit_WhiteELO = new CEditControl(this, wELO, r2, 4); r2.Offset(0, rowdiff);
   cedit_BlackELO = new CEditControl(this, bELO, r2, 4);

   r.right = inner.right;
   cedit_Event = new CEditControl(this, Info->event, r, nameStrLen); r.Offset(0, rowdiff);
   cedit_Site  = new CEditControl(this, Info->site,  r, nameStrLen); r.Offset(0, rowdiff);
   r.right = r.left + (RunningOSX() ? 115 : 90);
   cedit_Date  = new CEditControl(this, Info->date,  r, 10); r.Offset(0, rowdiff);
   cedit_Round = new CEditControl(this, Info->round, r, 10); r.Offset(0, rowdiff);

   CRect pr = r;
   if (! RunningOSX())
      pr.Inset(0, 1);
   else
   {  pr.bottom = pr.top + 20;
      pr.Offset(0, -3);
   }
   resultMenu = new CMenu("");
   resultMenu->AddItem("Unknown",   infoResult_Unknown);
   resultMenu->AddItem("1/2 - 1/2", infoResult_Draw);
   resultMenu->AddItem("1 - 0",     infoResult_WhiteWin);
   resultMenu->AddItem("0 - 1",     infoResult_BlackWin);
   cpopup_Result = new CPopupMenu(this, "", resultMenu, Info->result, pr); r.Offset(0, rowdiff);

   cedit_ECO = new CEditControl(this, Info->ECO,  r, 6); r.Offset(0, rowdiff); r.right = inner.right;
   cedit_Ann = new CEditControl(this, Info->annotator, r, nameStrLen);

   SetDefaultButton(cbutton_Default);
   CurrControl(cedit_White);
} /* CGameInfoDialog::CGameInfoDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

void CGameInfoDialog::HandlePushButton (CPushButton *ctrl)
{
   // Perform validation if user clicks "OK":

   if (ctrl == cbutton_Default)
   {
      if (! cedit_WhiteELO->ValidateNumber(0,3000,true))
      {  CurrControl(cedit_WhiteELO);
         NoteDialog(this, "Invalid ELO Rating", "The specified ELO rating for White is invalid: It must be a whole number between 0 and 3000 (or blank if unknown).", cdialogIcon_Error);
         return;
      }

      if (! cedit_BlackELO->ValidateNumber(0,3000,true))
      {  CurrControl(cedit_BlackELO);
         NoteDialog(this, "Invalid ELO Rating", "The specified ELO rating for Black is invalid: It must be a whole number between 0 and 3000 (or blank if unknown)", cdialogIcon_Error);
         return;
      }
   }
   else if (ctrl == cbutton_ClearAll)
   {
       cedit_White->SetText("");
       cedit_Black->SetText("");
       cedit_WhiteELO->SetText("");
       cedit_BlackELO->SetText("");
       cedit_Event->SetText("");
       cedit_Site->SetText("");
       cedit_Date->SetText("");
       cedit_Round->SetText("");
       cedit_ECO->SetText("");
       cedit_Ann->SetText("");
       cpopup_Result->Set(infoResult_Unknown);
   }

   // Validation succeeded (or user pressed "Cancel") -> Call inherited function:
   CDialog::HandlePushButton(ctrl);
} /* CGameInfoDialog::HandlePushButton */

/*------------------------------------ Process Dialog Result -------------------------------------*/

void CGameInfoDialog::ProcessResult (GAMEINFO *Info)
{
   LONG n;

   cedit_White->GetTitle(Info->whiteName);
   cedit_Black->GetTitle(Info->blackName);
   cedit_Event->GetTitle(Info->event);
   cedit_Site->GetTitle(Info->site);
   cedit_Date->GetTitle(Info->date);
   cedit_Round->GetTitle(Info->round);
   cedit_ECO->GetTitle(Info->ECO);
   cedit_Ann->GetTitle(Info->annotator);
   Info->whiteELO = (cedit_WhiteELO->GetLong(&n) ? n : -1);
   Info->blackELO = (cedit_BlackELO->GetLong(&n) ? n : -1);
   Info->result = cpopup_Result->Get();

   if (ccheck_Default->Checked())
   {
      GAMEINFO *pInfo = &(Prefs.gameInfo);
      cedit_White->GetTitle(pInfo->whiteName);
      cedit_Black->GetTitle(pInfo->blackName);
      cedit_Event->GetTitle(pInfo->event);
      cedit_Site->GetTitle(pInfo->site);
      cedit_Date->GetTitle(pInfo->date);
      cedit_Round->GetTitle(pInfo->round);
      cedit_ECO->GetTitle(pInfo->ECO);
      cedit_Ann->GetTitle(pInfo->annotator);
      pInfo->whiteELO = (cedit_WhiteELO->GetLong(&n) ? n : -1);
      pInfo->blackELO = (cedit_BlackELO->GetLong(&n) ? n : -1);
   }
} /* CGameInfoDialog::ProcessResult */
