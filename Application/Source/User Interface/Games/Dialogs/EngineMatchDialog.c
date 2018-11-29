/**************************************************************************************************/
/*                                                                                                */
/* Module  : ENGINEMATCHDIALOG.C                                                                  */
/* Purpose : This module implements the Initial Position Status dialog used in the pos editor.    */
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

#include "EngineMatchDialog.h"
#include "UCI_ConfigDialog.h"
#include "UCI_Option.h"
#include "LevelDialog.h"
#include "CollectionWindow.h"
#include "SigmaApplication.h"
#include "SigmaPrefs.h"


ENGINE_MATCH EngineMatch = { nil, nil, 0, false, 0,0,0, 0,0,0 };


class CEngineMatchDialog : public CDialog
{
public:
   CEngineMatchDialog (GameWindow *parent, CHAR *title, CRect frame);

   virtual void HandlePushButton (CPushButton *ctrl);
   virtual void HandleCheckBox   (CCheckBox *ctrl);

   void SetLevelText (void);
   void BuildCollectionMenu (CollectionWindow *selColWin);
	void CheckFullStrength (INT engineId);
	void ClearMultiPV (INT engineId);

   ENGINE_MATCH_PARAM Param;  // Local copy of the engine match parameters

   CPopupMenu   *cpopup_Engines1, *cpopup_Engines2;
   CPushButton  *cbutton_Options1, *cbutton_Options2;
   CEditControl *cedit_MatchLen;
   CCheckBox    *ccheck_AltColor;
   CTextControl *ctext_TimeControl;
   CPushButton  *cbutton_ChangeTC;
	CCheckBox    *ccheck_AdjWin, *ccheck_AdjDraw;
	CPopupMenu   *cpopup_Adj;
   CRect        rColPopup;
   CPopupMenu   *cpopup_ColMenu;
   CollectionWindow *colWinList[50];
   CPushButton  *cbutton_NewCol, *cbutton_OpenCol;
};


BOOL EngineMatchDialog (GameWindow *gameWin)
{
	UCI_QuitActiveEngine();

   //--- Reset Match Stats ---
   EngineMatch.gameWin      = nil;  // Is set to point to the gameWin only if the user clicks "Start"
   EngineMatch.colWin       = nil;
   EngineMatch.currGameNo   = 1;
   EngineMatch.timeForfeit  = false;   
   EngineMatch.prevScore    = 1;
   EngineMatch.adjWinCount  = 0;
   EngineMatch.adjDrawCount = 0;
   EngineMatch.winCount1    = 0;
   EngineMatch.drawCount    = 0;
   EngineMatch.winCount2    = 0;

   //--- Run the Dialog ---
   CRect frame(0, 0, 460, 465);
   theApp->CentralizeRect(&frame);

   CEngineMatchDialog *dialog = new CEngineMatchDialog(gameWin, "Engine Match", frame);
   dialog->Run();
   BOOL ok = (dialog->reply == cdialogReply_OK);
   delete dialog;
   
   //--- Process Dialog Result ---
   if (ok)
   {
      EngineMatch.gameWin = gameWin;
      gameWin->level = Prefs.EngineMatch.level;
      gameWin->ResetClocks();
      gameWin->boardAreaView->DrawModeIcons();
      gameWin->AdjustLevelMenu();
      gameWin->AdjustToolbar();
      gameWin->SetFront();  // Bring the game window to front
   }
   return ok;
} /* EngineMatchDialog */


void EngineMatch_ResetParam (ENGINE_MATCH_PARAM *Param)
{
	Param->engine1      = uci_SigmaEngineId;
	Param->engine2      = uci_SigmaEngineId;
	Param->matchLen     = 10;
	Param->alternate    = true;
	Level_Reset(&Param->level);
	Param->adjWin       = true;
	Param->adjWinLimit  = 5;
	Param->adjDraw      = true;
} // EngineMatch_ResetParam

/*----------------------------------- Constructor/Destructor -------------------------------------*/

CEngineMatchDialog::CEngineMatchDialog (GameWindow *parent, CHAR *title, CRect frame)
 : CDialog (parent, title, frame)
{
   Param = Prefs.EngineMatch; // Make local copy of the parameters
   CRect inner = InnerRect();

   //--- Create engine popup menus ---
   CRect rEngine1 = inner;
   rEngine1.bottom = rEngine1.top + controlHeight_PopupMenu;
   rEngine1.right = rEngine1.left + (inner.Width() - 50)/2;

   CRect rEngine2 = inner;
   rEngine2.bottom = rEngine2.top + controlHeight_PopupMenu;
   rEngine2.left = rEngine2.right - (inner.Width() - 50)/2;

   CMenu *engineMenu1 = new CMenu("");
   CMenu *engineMenu2 = new CMenu("");
   
   if (Param.engine1 >= Prefs.UCI.count)
      Param.engine1 = uci_SigmaEngineId;
   if (Param.engine2 >= Prefs.UCI.count)
      Param.engine2 = uci_SigmaEngineId;

   for (UCI_ENGINE_ID i = 0; i < Prefs.UCI.count; i++)
   {
      if (i == 1)
      {  engineMenu1->AddSeparator();
         engineMenu2->AddSeparator();
      }
      engineMenu1->AddItem(Prefs.UCI.Engine[i].name, i);
      engineMenu2->AddItem(Prefs.UCI.Engine[i].name, i);
   }
   cpopup_Engines1 = new CPopupMenu(this, "", engineMenu1, Param.engine1, rEngine1);
   cpopup_Engines2 = new CPopupMenu(this, "", engineMenu2, Param.engine2, rEngine2);

	CRect rTextVs = rEngine1;
	rTextVs.left  = rEngine1.right + 15;
	rTextVs.right = rEngine2.left;
	rTextVs.Offset(0, 2);
   new CTextControl(this, "vs", rTextVs);

	CRect rOptions1 = rEngine1; rOptions1.Offset(0,30); rOptions1.Inset(45,0);
	cbutton_Options1 = new CPushButton(this, "Options...", rOptions1);

	CRect rOptions2 = rEngine2; rOptions2.Offset(0,30); rOptions2.Inset(45,0);
	cbutton_Options2 = new CPushButton(this, "Options...", rOptions2);

	//--- Divider ---
	CRect rDiv0(0,0,inner.Width(),4);
	rDiv0.Offset(inner.left, rOptions1.bottom + 10);
   new CDivider(this, rDiv0);

   //--- Match Length ---
   CRect rMatchLen(0,0,95,controlHeight_Text);
   rMatchLen.Offset(inner.left, rDiv0.bottom + 10);
   new CTextControl(this, "Match Length:", rMatchLen);

   rMatchLen.left   = rMatchLen.right + 5;
   rMatchLen.right  = rMatchLen.left + 30;
   rMatchLen.bottom = rMatchLen.top + controlHeight_Edit;
   cedit_MatchLen   = new CEditControl(this, "", rMatchLen, 3);
   cedit_MatchLen->SetLong(Param.matchLen);

   rMatchLen.left   = rMatchLen.right + 10;
   rMatchLen.right  = rMatchLen.left + 50;
   rMatchLen.bottom = rMatchLen.top + controlHeight_Text;
   new CTextControl(this, "games", rMatchLen);

   //--- Alternate colors between games ---
   CRect rAltColor(0,0,inner.Width(),controlHeight_CheckBox);
   rAltColor.Offset(inner.left, rMatchLen.bottom + 10);
   ccheck_AltColor = new CCheckBox(this, "Alternate colors between games", true, rAltColor);

	//--- Divider ---
	CRect rDiv1(0,0,inner.Width(),4);
	rDiv1.Offset(inner.left, rAltColor.bottom + 10);
   new CDivider(this, rDiv1);

	//--- Time Controls ---
	CRect rTextTC = inner;
	rTextTC.top = rDiv1.bottom + 5;
	rTextTC.bottom = rTextTC.top + controlHeight_Text;
   new CTextControl(this, "Time Controls", rTextTC);
   rTextTC.Offset(25, controlVDiff_Text);
   ctext_TimeControl = new CTextControl(this, "", rTextTC);

   CRect rChangeTC(0,0,80,controlHeight_PushButton);
   rChangeTC.Offset(rTextTC.left, rTextTC.bottom + 5);
   cbutton_ChangeTC = new CPushButton(this, "Change...", rChangeTC);
   SetLevelText();

	//--- Divider ---
	CRect rDiv2(0,0,inner.Width(),4);
	rDiv2.Offset(inner.left, rChangeTC.bottom + 10);
   new CDivider(this, rDiv2);

	//--- Adjudicate ---
	CRect rTextAdj = inner;
	rTextAdj.top = rDiv2.bottom + 5;
	rTextAdj.bottom = rTextAdj.top + controlHeight_Text;
   new CTextControl(this, "Adjudicate", rTextAdj);

	CRect rAdjCbox(0,0,260,controlHeight_CheckBox);
	rAdjCbox.Offset(rTextAdj.left + 25, rTextAdj.bottom + 2);

   CRect rAdjPopup = rAdjCbox;
   rAdjPopup.bottom = rAdjPopup.top + controlHeight_PopupMenu;
   rAdjPopup.Offset(0, -2);
   rAdjPopup.left = rAdjCbox.right + 5;
   rAdjPopup.right = inner.right;

   ccheck_AdjWin  = new CCheckBox(this, "as WIN if score difference is at least", Param.adjWin, rAdjCbox); rAdjCbox.Offset(0,20);
   ccheck_AdjDraw = new CCheckBox(this, "as DRAW if score is 0", Param.adjDraw, rAdjCbox); rAdjCbox.Offset(0,23);

   CMenu *adjMenu = new CMenu("");
   for (INT i = 3; i <= 9; i++)
   {	CHAR s[10]; Format(s, "%d pawns", i);
   	adjMenu->AddItem(s, i);
   }
   cpopup_Adj = new CPopupMenu(this, "", adjMenu, Param.adjWinLimit, rAdjPopup);

   rTextAdj = rAdjCbox;
   rTextAdj.right = inner.right;
   new CTextControl(this, "Both engines need to agree for at least two moves", rTextAdj, true, controlFont_SmallSystem);

	//--- Divider ---
	CRect rDiv3(0,0,inner.Width(),4);
	rDiv3.Offset(inner.left, rTextAdj.bottom + 10);
   new CDivider(this, rDiv3);

	//--- Record Match ---
	CRect rRecord(0,0,inner.Width(),controlHeight_CheckBox);
	rRecord.Offset(inner.left, rDiv3.bottom + 5);
	new CTextControl(this, "Record match in the collection", rRecord);

	rColPopup.Set(0,0,0,controlHeight_PopupMenu);
	rColPopup.Offset(inner.left + 25, rRecord.bottom + 10);
	CRect rOpenCol = rColPopup;
	rOpenCol.right = inner.right; rOpenCol.left = rOpenCol.right - 70;
	CRect rNewCol = rOpenCol;
	rNewCol.Offset(-(rOpenCol.Width() + 10), 0);
	rColPopup.right = rNewCol.left - 15;

   cpopup_ColMenu = nil;
	BuildCollectionMenu(EngineMatch.colWin);
	cbutton_NewCol  = new CPushButton(this, "New...", rNewCol);
	cbutton_OpenCol = new CPushButton(this, "Open...", rOpenCol);

	//--- Divider ---
	CRect rDiv4(0,0,inner.Width(),4);
	rDiv4.Offset(inner.left, rColPopup.bottom + 10);
   new CDivider(this, rDiv4);

   //--- Create "Start"/"Cancel" buttons ---
   cbutton_Cancel  = new CPushButton(this, "Cancel", CancelRect());
   cbutton_Default = new CPushButton(this, "Start", DefaultRect());
   SetDefaultButton(cbutton_Default);
} /* CEngineMatchDialog::CEngineMatchDialog */


void CEngineMatchDialog::HandlePushButton (CPushButton *ctrl)
{
   if (ctrl == cbutton_Default)  // Exit if validation fails
   {
      if (! cedit_MatchLen->ValidateNumber(1, 10000, false))
      {  NoteDialog(this, "Invalid Match Length", "The Match Length must be a whole number of games between 1 and 10000", cdialogIcon_Error);
         return;
      }

      Param.engine1 = cpopup_Engines1->Get();
      Param.engine2 = cpopup_Engines2->Get();
      if (Param.engine1 == Param.engine2)
      {  NoteDialog(this, "Engine Selection", "You must select two different engines...", cdialogIcon_Error);
         return;
      }

		CheckFullStrength(Param.engine1);
		CheckFullStrength(Param.engine2);
		ClearMultiPV(Param.engine1);
		ClearMultiPV(Param.engine2);
       
      LONG matchLen;
      cedit_MatchLen->GetLong(&matchLen);
      Param.matchLen = matchLen;
      Param.alternate = ccheck_AltColor->Checked();
      Param.adjWin  = ccheck_AdjWin->Checked();
      Param.adjDraw = ccheck_AdjDraw->Checked();
      Param.adjWinLimit = cpopup_Adj->Get();

      EngineMatch.currGameNo = 1;
      EngineMatch.colWin  = colWinList[cpopup_ColMenu->Get()];

      Prefs.EngineMatch = Param;  // Copy to global engine match parameter block
   }
   else if (ctrl == cbutton_Options1)
   {
   	UCI_ConfigDialog(cpopup_Engines1->Get(), false);
   }
   else if (ctrl == cbutton_Options2)
   {
   	UCI_ConfigDialog(cpopup_Engines2->Get(), false);
   }
   else if (ctrl == cbutton_ChangeTC)
   {
      if (Level_Dialog(&Param.level, true))
         SetLevelText();
      return;
   }
   else if (ctrl == cbutton_NewCol)
   {
	   CollectionWindow *colWin = NewCollectionWindow();
	   if (colWin) BuildCollectionMenu(colWin);
	   DispatchActivate(true);
	   return;
   }
   else if (ctrl == cbutton_OpenCol)
   {
	   CollectionWindow *colWin = OpenCollectionWindow();
	   if (colWin) BuildCollectionMenu(colWin);
	   return;
   }

   CDialog::HandlePushButton(ctrl);
} /* CEngineMatchDialog::HandlePushButton */


void CEngineMatchDialog::CheckFullStrength (INT engineId)
{
	UCI_INFO *U = &Prefs.UCI.Engine[engineId];
	
	if (! U->LimitStrength.u.Check.val) return;

   CHAR msg[256];
   Format(msg, "Warning! The %s engine is not configured to play at full strength. Change to full strength?", U->name);
   if (QuestionDialog(this, "Engine Rating", msg, "Yes", "No"))
   	U->LimitStrength.u.Check.val = false;
} // CEngineMatchDialog::CheckFullStrength


void CEngineMatchDialog::ClearMultiPV (INT engineId)
{
	INT multiPVoptionId = UCI_GetMultiPVOptionId(engineId);

	if (multiPVoptionId != uci_NullOptionId)
	{
   	UCI_INFO *U = &Prefs.UCI.Engine[engineId];
   	U->Options[multiPVoptionId].u.Spin.val = 1;
	}
} // CEngineMatchDialog::ClearMultiPV


void CEngineMatchDialog::HandleCheckBox (CCheckBox *ctrl)
{
   CDialog::HandleCheckBox(ctrl);
} /* CEngineMatchDialog::HandleCheckBox */


void CEngineMatchDialog::SetLevelText (void)
{
   CHAR movesStr[20];
   CHAR timeStr[20];
   CHAR deltaStr[20];
   CHAR levelStr[100];

   if (Param.level.TimeMoves.moves == allMoves)
      CopyStr("Game", movesStr);
   else
      Format(movesStr, "%d moves", Param.level.TimeMoves.moves);

	Format(timeStr, "%d minutes", Param.level.TimeMoves.time/60);

   if (Param.level.TimeMoves.clockType == clock_Normal)
      CopyStr("", deltaStr);
   else
      Format(deltaStr, " (+ %d secs/move)", Param.level.TimeMoves.delta);

	Format(levelStr, "%s in %s%s", movesStr, timeStr, deltaStr);
   ctext_TimeControl->SetTitle(levelStr);
} /* CEngineMatchDialog::SetLevelText */


void CEngineMatchDialog::BuildCollectionMenu (CollectionWindow *selColWin)
{
   if (cpopup_ColMenu)
   {	delete cpopup_ColMenu;
      cpopup_ColMenu = nil;
   }

	INT selCol = 0;

   CMenu *colMenu = new CMenu("");
   colMenu->AddItem("<None>", 0);
	colWinList[0] = nil;

	INT col = 1;
   sigmaApp->winList.Scan();
   while (SigmaWindow *win = (SigmaWindow*)sigmaApp->winList.Next())
      if (! win->IsDialog() && win->winClass == sigmaWin_Collection)
      {
         if (col == 1) colMenu->AddSeparator();
         if (win == selColWin)
            selCol = col;
      	CHAR title[100];
      	win->GetTitle(title);
         colMenu->AddItem(title, col);
         colWinList[col] = (CollectionWindow*)win;
         col++;
      }

   colWinList[col] = nil;  // Null terminate the list

   if (selCol == 0)
      EngineMatch.colWin = nil;

   cpopup_ColMenu = new CPopupMenu(this, "", colMenu, selCol, rColPopup);
} // CEngineMatchDialog::BuildCollectionMenu
