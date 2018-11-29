/**************************************************************************************************/
/*                                                                                                */
/* Module  : UCI_CONFIGDIALOG.C                                                                   */
/* Purpose : This module implements the interface to the UCI engines.                             */
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

#include "UCI_ConfigDialog.h"
#include "UCI.h"
#include "UCI_Option.h"
#include "UCI_AppleEvents.h"
#include "CDialog.h"
#include "CApplication.h"
#include "CFile.h"
#include "SigmaApplication.h"
#include "SigmaWindow.h"
#include "SigmaPrefs.h"
#include "GameWindow.h"

#define kDlgWidth    534

#define kInfoLineHeight   45
#define kDlgHeight     (534 + kInfoLineHeight - 32)  // Port line removed

#define kNameColumnWidth 220

/*----------------------------------------- Dialog Class -----------------------------------------*/

class CUCI_ConfigDialog : public CDialog
{
public:
   CUCI_ConfigDialog (CRect frame, BOOL canSelectEngine);

   virtual void HandlePushButton (CPushButton *ctl);
   virtual void HandlePopupMenu (CPopupMenu *ctrl, INT itemID);
   virtual void HandleListBox (CListBox *ctrl, INT row, INT column, BOOL dblClick);
   virtual void HandleCheckBox (CCheckBox *ctrl);
   virtual void HandleScrollBar (CScrollBar *ctrl, BOOL tracking);
   virtual void HandleEditControl (CEditControl *ctrl, BOOL textChanged, BOOL selChanged);

   void RebuildEngineMenu (BOOL canSelectEngine);
   void RebuildOptionList (void);
   void SelectEngine (UCI_ENGINE_ID engineId);
   void AddEngine (void);
   void RemoveEngine (void);
   void SetEngineLocation (void);
   UCI_OPTION *GetSelectedOption (void); // Returns nil if no option selected
   void SaveOption (UCI_OPTION *Option);
   void FlushOption (void);

   CPopupMenu   *cpopup_Engines;
   CRect        rEnginePopup;
   
   CPushButton  *cbutton_Add, *cbutton_Remove, *cbutton_DefaultOptions;

   CTextControl *ctext_Author;

   CTextControl *ctext_LicenseInfo;
   
   CTextControl *ctext_Status;
   CPushButton  *cbutton_StartStop;
   
   CEditControl *cedit_Location;
   CPushButton  *cbutton_LocationBrowse;

   // Option list
   CTextControl *ctext_Options;
   CListBox     *clbox_Options;

   //--- Option "editors" (one for each type) ---
   CTextControl *ctext_OptionName;
   CCheckBox    *ccheck_OptionCheck;
   CScrollBar   *cscroll_OptionSpin; CTextControl *ctext_OptionSpinVal;
   CPopupMenu   *cpopup_OptionCombo; CRect rCombo;
   CEditControl *cedit_OptionString;
   CPushButton  *cbutton_OptionButton;
   CTextControl *ctext_OptionDescr;

   UCI_ENGINE_ID flushEngineId;
   UCI_OPTION   *flushOption;  // Postponed option change

   //--- Common UCI Options ---
//   CTextControl *ctext_NalimovPath;
   CEditControl *cedit_NalimovPath;
   CPushButton  *cbutton_NalimovBrowse;
};


static CUCI_ConfigDialog *uciConfigDialog = nil;

static void SetSpinValStr (CTextControl *spinVal, UCI_OPTION *Option);


/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN UCI CONFIG DIALOG                                     */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_ConfigDialog (UCI_ENGINE_ID currEngineId, BOOL canSelectEngine)
{
   if (! UCI_AbortAllEngines()) return false;

   // Run the dialog
   Prefs.UCI.defaultId = currEngineId;
   CRect frame(0, 0, kDlgWidth, kDlgHeight);
   theApp->CentralizeRect(&frame);

   uciConfigDialog = new CUCI_ConfigDialog(frame, canSelectEngine);
   uciConfigDialog->Run();

   // Rebuild engine menu
   sigmaApp->RebuildEngineMenu();

   delete uciConfigDialog;
   uciConfigDialog = nil;
   return true;
} // UCI_ConfigDialog


void UCI_ConfigDialogRefresh (void)
{
   if (uciConfigDialog)
      uciConfigDialog->RebuildOptionList();
} // UCI_ConfigDialogRefresh


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

CUCI_ConfigDialog::CUCI_ConfigDialog (CRect frame, BOOL canSelectEngine)
 : CDialog (nil, "Engine Manager", frame, cdialogType_Modal)
{
   CRect inner = InnerRect();
   CRect r;               // Generic control rectangles
   INT   rowdiff = 32;    // Standard row distance
   INT   dataColumn = 75 + 22; // Start (left side) of data column
   INT   authorRowdiff = rowdiff;

   flushEngineId = Prefs.UCI.defaultId;
   flushOption   = nil;

   authorRowdiff += kInfoLineHeight;

   //--- Create the Close button first ---
   cbutton_Default = new CPushButton(this, "Close", DefaultRect());

   CRect rDefOptions = DefaultRect();
   rDefOptions.left = inner.left;
   rDefOptions.right = rDefOptions.left + 130;
   cbutton_DefaultOptions = new CPushButton(this, "Default Options", rDefOptions);

   //--- Create static text labels in left side ---
   r.Set(0, 0, dataColumn - 5, controlHeight_Text);
   r.Offset(inner.left, inner.top);
   new CTextControl(this, "Engine:",   r); r.Offset(0, rowdiff);
   new CTextControl(this, "Author:",   r); r.Offset(0, authorRowdiff);
   new CTextControl(this, "Status:",   r); r.Offset(0, rowdiff);
   new CTextControl(this, "Location:", r); r.Offset(0, rowdiff + 10);
   ctext_Options = new CTextControl(this, "Options:", r, true); r.Offset(0, rowdiff);

   //--- Create Data Column
   r.Set(inner.left + dataColumn, inner.top, inner.right - 170, inner.top + controlHeight_Edit);

   //--- ROW 1: Create Engine popup list and "Add..."/"Remove" buttons ---
   rEnginePopup = r;
   rEnginePopup.bottom = rEnginePopup.top + controlHeight_PopupMenu;
   rEnginePopup.Offset(0, -2);
   cpopup_Engines = nil;
   RebuildEngineMenu(canSelectEngine);

   CRect rRemove = r;
   rRemove.right = inner.right;
   rRemove.left = rRemove.right - controlWidth_PushButton;
   rRemove.bottom = rRemove.top + controlHeight_PushButton;
   rRemove.Offset(0, -2);  // Align with text labels
   CRect rAdd = rRemove;
   rAdd.Offset(-rRemove.Width() - 12, 0);
   cbutton_Add = new CPushButton(this, "Add...", rAdd, canSelectEngine);
   cbutton_Remove = new CPushButton(this, "Remove", rRemove, canSelectEngine);
   
   //--- ROW 2: Author ---
   CRect rText(inner.left + dataColumn, inner.top + rowdiff, inner.right - 90, inner.top + rowdiff + controlHeight_Text);
   CRect rButton = rRemove; rButton.Offset(0, rowdiff);
   ctext_Author = new CTextControl(this, "-", rText);

   CRect rLic = rText;
   rLic.top = rText.bottom; rLic.bottom = rLic.top + kInfoLineHeight + 8;
   rLic.right = inner.right;
   ctext_LicenseInfo = new CTextControl(this, "-", rLic, true, controlFont_SmallSystem);

   rText.Offset(0, authorRowdiff);
   rButton.Offset(0, authorRowdiff);
   
   //--- ROW 3: Status ---
   rText.right = rText.left + 80;
   rButton.left = rText.right + 10; rButton.right = rButton.left + controlWidth_PushButton;

   ctext_Status = new CTextControl(this, "-", rText);
   cbutton_StartStop = new CPushButton(this, "Start", rButton);
   rText.Offset(0, rowdiff);
   rButton.Offset(0, rowdiff);
   
   //--- ROW 5: Location ---
   CRect rLoc = rText;
   rLoc.right = inner.right - 90;
   rLoc.bottom = rLoc.top + controlHeight_Edit;
   cedit_Location = new CEditControl(this, "", rLoc, uci_EnginePathLen, true, false);
   rText.Offset(0, rowdiff + 15);

   rLoc.left = rLoc.right + 15;
   rLoc.right = inner.right;
   rLoc.top -= 2;
   rLoc.bottom = rLoc.top + controlHeight_PushButton;
   cbutton_LocationBrowse = new CPushButton(this, "Browse...", rLoc);

   //--- Divider above options ---
   CRect rDivider = inner;
   rDivider.top = rText.top - 15;
   rDivider.bottom = rDivider.top + 2;
   new CDivider(this, rDivider);

   //--- ROW 6: Option list ---
   CRect rLbox = rText;
   rLbox.bottom = rLbox.top + 10*16; // 10 rows
   rLbox.right = inner.right;
   clbox_Options = new CListBox(this, rLbox, 0, 2, false, true, kNameColumnWidth, 16);
   
   //--- ROW 7: Individual options (depending on option type) ---
   CRect rOption = rLbox;
   rOption.top = rLbox.bottom + 14;
   CRect rOptionName = rOption;
   rOption.left  = rLbox.left + kNameColumnWidth;
   rOption.right = rLbox.right;
   
   // Option name (left)
   rOptionName.bottom = rOption.top + controlHeight_Text;
   rOptionName.right = rOptionName.left + kNameColumnWidth - 10;
   ctext_OptionName = new CTextControl(this, "", rOptionName, false);

   // Option "check"
   rOption.bottom = rOption.top + controlHeight_CheckBox;
   ccheck_OptionCheck = new CCheckBox(this, "", false, rOption, false);

   // Option "spin"
   rOption.bottom = rOption.top + controlHeight_CheckBox;
   cscroll_OptionSpin = new CScrollBar(this, 1, 100, 1, 10, rOption, false, true, false); //true);
   rText = rOption;
   rText.top = rOption.top + 22;
   rText.bottom = rText.top + 15;
// rText.left += rText.Width()/2 - 5;
   ctext_OptionSpinVal = new CTextControl(this, "", rText, false, controlFont_SmallSystem);

   // Option "combo"
   cpopup_OptionCombo = nil;
   rCombo = rOption;
   rCombo.bottom = rCombo.top + controlHeight_PopupMenu;
   rCombo.Offset(0, -2);

   // Option "button"
   cbutton_OptionButton = new CPushButton(this, "", rOptionName, false);

   // Option "string"
   rOption.bottom = rOption.top + controlHeight_Edit;
   cedit_OptionString = new CEditControl(this, "", rOption, uci_StringOptionLen, false);

   // Description of default option value
   rText = rOptionName;
   rText.top = rOption.top + 22;
   rText.bottom = rText.top + 15;
   ctext_OptionDescr = new CTextControl(this, "", rText, true, controlFont_SmallSystem);

   //--- Divider below Common Options ---
   rDivider = inner;
   rDivider.top = rText.bottom + 10;
   rDivider.bottom = rDivider.top + 2;
   new CDivider(this, rDivider);

   //--- Common Options (Nalimov Path) ---
   CRect rNal;
   rNal.Set(0, 0, dataColumn - 5, controlHeight_Text);
   rNal.Offset(inner.left, rDivider.top + 20);
   new CTextControl(this, "Nalimov Path:", rNal);

   rNal.left = inner.left + dataColumn;
   rNal.right = inner.right - 90;
   rNal.bottom = rNal.top + controlHeight_Edit;
   cedit_NalimovPath = new CEditControl(this, Prefs.UCI.NalimovPath, rNal, uci_NalimovPathLen, true, ProVersion());
   
   rNal.left = rNal.right + 15;
   rNal.right = inner.right;
   rNal.top -= 2;
   rNal.bottom = rNal.top + controlHeight_PushButton;
   cbutton_NalimovBrowse = new CPushButton(this, "Browse...", rNal);

   rText.top = rNal.bottom + 5;
   rText.bottom = rText.top + 16;
   rText.left = inner.left + dataColumn;
   rText.right = inner.right;
   new CTextControl(this, "Common option shared by all UCI engines", rText, true, controlFont_SmallSystem);

   //--- Divider below Common Options ---
   rDivider = inner;
   rDivider.bottom = inner.bottom - controlHeight_PushButton - 10;
   rDivider.top = rDivider.bottom - 2;
   new CDivider(this, rDivider);

   //--- Set default button ---
   SetDefaultButton(cbutton_Default);

   //--- Finally select current engine ---
   SelectEngine(Prefs.UCI.defaultId);
   CurrControl(clbox_Options);
} // CUCI_ConfigDialog::CUCI_ConfigDialog


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

static BOOL EditPortDialog (UCI_ENGINE_ID engineId, LONG *port);

/*----------------------------------- Handle Push Button Events ----------------------------------*/

void CUCI_ConfigDialog::HandlePushButton (CPushButton *ctrl)
{
   FlushOption();

   if (ctrl == cbutton_Add)
   {
      UCI_QuitActiveEngine();
      SelectEngine(cpopup_Engines->Get());  // Refreshes status to stopped
      AddEngine();
   }
   else if (ctrl == cbutton_Remove)
   {
      UCI_INFO *Engine = &Prefs.UCI.Engine[cpopup_Engines->Get()];

      CHAR msg[200];
      Format(msg, "Are you sure you want to remove the '%s' engine?", Engine->name);
      if (! QuestionDialog(this, "Remove Engine", msg, "No", "Yes"))
         RemoveEngine();
   }
   else if (ctrl == cbutton_StartStop)
   {
      UCI_ENGINE_ID engineId = cpopup_Engines->Get();

      cbutton_StartStop->Enable(false);
      if (! UCISession[engineId].active)
      {  UCI_QuitActiveEngine();
         UCI_LoadEngine(engineId);
	      RebuildEngineMenu(true);
      }
      else
         UCI_QuitEngine(engineId);
      SelectEngine(engineId);
   }
   else if (ctrl == cbutton_LocationBrowse)
   {
       SetEngineLocation();
   }
   else if (ctrl == cbutton_OptionButton)
   {
      UCI_ENGINE_ID engineId = cpopup_Engines->Get();

      if (! UCI_EngineLoaded(engineId))
         NoteDialog(this, "Engine Not Loaded", "You need to start the engine before executing this command.", cdialogIcon_Error);
      else
      {  SaveOption(GetSelectedOption());
         FlushOption();  // Flush right away
      }
   }
   else if (ctrl == cbutton_DefaultOptions)
   {
      UCI_ENGINE_ID engineId = cpopup_Engines->Get();
      UCI_INFO *Engine = &Prefs.UCI.Engine[engineId];
      CHAR msg[300];
      Format(msg, "This will revert all options for '%s' back to their default values. Continue?", Engine->name);
      if (! QuestionDialog(this, "Default Options", msg, "No", "Yes"))
      {  UCI_SetDefaultOptions(engineId);
         RebuildOptionList();
      }
   }
   else if (ctrl == cbutton_NalimovBrowse)
   {
       if (! ProVersionDialog(this, "Nalimov table bases are not supported in Sigma Chess Lite.")) return;

       if (CFile_GetFolderPathDialog(Prefs.UCI.NalimovPath, uci_NalimovPathLen))
       	 cedit_NalimovPath->SetText(Prefs.UCI.NalimovPath);
   }
   else
   {
      CDialog::HandlePushButton(ctrl);
   }
} // CUCI_ConfigDialog::HandlePushButton

/*----------------------------------- Handle Popup Menu Events -----------------------------------*/

void CUCI_ConfigDialog::HandlePopupMenu (CPopupMenu *ctl, INT itemID)
{
   FlushOption();

   if (ctl == cpopup_Engines)
   {
      SelectEngine(cpopup_Engines->Get());
  	}
  	else if (ctl == cpopup_OptionCombo)
  	{
      UCI_OPTION *Option = GetSelectedOption();
      if (! Option) return;
      Option->u.Combo.val = itemID;
      SaveOption(Option);
  	}
} // CUCI_ConfigDialog::HandlePopupMenu

/*------------------------------------ Handle CheckBox Events ------------------------------------*/

void CUCI_ConfigDialog::HandleCheckBox (CCheckBox *ctrl)
{
   FlushOption();

   CDialog::HandleCheckBox(ctrl);

   if (ctrl == ccheck_OptionCheck)
   {
      UCI_OPTION *Option = GetSelectedOption();
      if (! Option) return;
      Option->u.Check.val = ! Option->u.Check.val;
      SaveOption(Option);
      ccheck_OptionCheck->SetTitle(Option->u.Check.val ? "On" : "Off");
   }
} /* CUCI_ConfigDialog::HandleCheckBox */

/*----------------------------------- Handle ScrollBar Events ------------------------------------*/

void CUCI_ConfigDialog::HandleScrollBar (CScrollBar *ctrl, BOOL tracking)
{
   if (ctrl == cscroll_OptionSpin)
   {
      UCI_OPTION *Option = GetSelectedOption();
      if (! Option) return;

      INT val = cscroll_OptionSpin->GetVal();

      // Special handling of Hash size
      if (EqualStr(Option->name,uciOptionName_Hash))
      {
         // If "paging" then adjust to next nearest power of two
         if (val > Option->u.Spin.val + 1)
         {  for (val = Max(1,Option->u.Spin.min); val <= Option->u.Spin.val; val *= 2);
            cscroll_OptionSpin->SetVal(val);
         }
         else if (val < Option->u.Spin.val - 1)
         {  for (val = Max(1,Option->u.Spin.min); 2*val < Option->u.Spin.val; val *= 2);
            cscroll_OptionSpin->SetVal(val);
         }

	      // Check that we don't get too close to physical RAM size
	      ULONG machineRAM = Mem_PhysicalRAM()/(1024*1024);
/*
	      if (val > (3*machineRAM)/4)
	      {
	         CHAR msg[200];
	         Format(msg, "The hash table size may NOT exceed 75 %c of the physical memory in your computer (%d MB).", '%', machineRAM);
	      	NoteDialog(this, "Hash Table Limit Exceeded", msg, cdialogIcon_Error);
	      	val = (3*machineRAM)/4;
	      	cscroll_OptionSpin->SetVal(val);
	      }
*/	      
	      if (val > machineRAM/2)
	      {
	         CHAR msg[200];
	         Format(msg, "Setting the hash table size too high may result in performance problems due to virtual memory disk swapping. Continue?");
	      	if (! QuestionDialog(this, "Hash Table Size Warning", msg, "OK", "Cancel"))
	      	{  cscroll_OptionSpin->SetVal(Option->u.Spin.val);
	      	   return;
	      	}
	      }
      }

      if (EqualStr(Option->name,uciOptionName_Hash) && val > uci_MaxHashSizeLite && 
          ! ProVersionDialog(this, "Hash tables are limited to 64 MB in Sigma Chess Lite."))
      {
         Option->u.Spin.val = uci_MaxHashSizeLite;
         cscroll_OptionSpin->SetVal(uci_MaxHashSizeLite);
      }
      else
      {
         Option->u.Spin.val = cscroll_OptionSpin->GetVal();
      }
      SaveOption(Option);
   }
} // CUCI_ConfigDialog::HandleScrollBar

/*---------------------------------- Handle Edit Control Events ----------------------------------*/

void CUCI_ConfigDialog::HandleEditControl (CEditControl *ctrl, BOOL textChanged, BOOL selChanged)
{
   if (ctrl == cedit_OptionString)
   {
      if (textChanged)
      {
         UCI_OPTION *Option = GetSelectedOption();
         if (! Option) return;
         cedit_OptionString->GetText(Option->u.String.val);
         SaveOption(Option);
      }
   }
   else if (ctrl == cedit_Location)
   {
      if (textChanged)
      {
         UCI_INFO *Engine = &Prefs.UCI.Engine[cpopup_Engines->Get()];
         cedit_Location->GetText(Engine->path);
      }
   }
   else if (ctrl == cedit_NalimovPath)
   {
      if (textChanged)
         cedit_NalimovPath->GetText(Prefs.UCI.NalimovPath);
   }
} // CUCI_ConfigDialog::HandleEditControl

/*------------------------------------ Handle ListBox Events -------------------------------------*/

void CUCI_ConfigDialog::HandleListBox (CListBox *ctrl, INT row, INT column, BOOL dblClick)
{
   FlushOption();

   //--- First hide all ---
   ctext_OptionName->Show(false);
   ctext_OptionDescr->Show(false);
   ccheck_OptionCheck->Show(false);
   cscroll_OptionSpin->Show(false);
   ctext_OptionSpinVal->Show(false);
   cedit_OptionString->Show(false);
   cbutton_OptionButton->Show(false);
   if (cpopup_OptionCombo)
   {  delete cpopup_OptionCombo;
      cpopup_OptionCombo = nil;
   }

   //--- Fetch Selected Option (exit if none) ---
   UCI_OPTION *Option = GetSelectedOption();
   if (! Option) return;

   //--- Then dispatch on option type ---
   CHAR descr[100];
   CopyStr("", descr);

   switch (Option->type)
   {
      case uciOption_Check :
      {
         ccheck_OptionCheck->SetTitle(Option->u.Check.val ? "On" : "Off");
         ccheck_OptionCheck->Check(Option->u.Check.val);
         ccheck_OptionCheck->Show(true);
         Format(descr, "Default: %s", (Option->u.Check.def ? "On" : "Off"));
         break;
      }

      case uciOption_Spin :
      {
         cscroll_OptionSpin->SetMin(Option->u.Spin.min);
         cscroll_OptionSpin->SetMax(Option->u.Spin.max);
         cscroll_OptionSpin->SetVal(Option->u.Spin.val);
         INT range = Option->u.Spin.max - Option->u.Spin.min;
         INT inc = 1;
         if (range <= 10) inc = 1;
         else if (range <= 20) inc = 5;
         else if (range <= 100) inc = 10;
         else if (range <= 500) inc = 50;
         else inc = 100;
         cscroll_OptionSpin->SetIncrement(inc);
         cscroll_OptionSpin->Show(true);
         cscroll_OptionSpin->Enable(! EqualStr(Option->name,uciOptionName_MultiPV));

//         CHAR spinValStr[50];
         if (UCI_OptionUnitIsMB(Option))
         {  Format(descr, "Default: %d MB (range %d - %d MB)", Option->u.Spin.def, Option->u.Spin.min, Option->u.Spin.max);
  //          Format(spinValStr, "                      %d MB", Option->u.Spin.val);
         }
         else
         {  Format(descr, "Default: %d (range %d - %d)", Option->u.Spin.def, Option->u.Spin.min, Option->u.Spin.max);
         /*
            if (! EqualStr(Option->name,uciOptionName_MultiPV))
               Format(spinValStr, "                        %d", Option->u.Spin.val);
            else
               CopyStr("Only changable in game window", spinValStr);
*/
         }

         SetSpinValStr(ctext_OptionSpinVal, Option);
//         ctext_OptionSpinVal->SetTitle(spinValStr);
         ctext_OptionSpinVal->Show(true);
         break;
      }

      case uciOption_Combo :
      {
         CMenu *comboMenu = new CMenu("");
         for (INT i = 0; i < Option->u.Combo.count; i++)
            comboMenu->AddItem(Option->u.Combo.List[i], i);
         cpopup_OptionCombo = new CPopupMenu(this, "", comboMenu, Option->u.Combo.val, rCombo);
         Format(descr, "Default: %s", Option->u.Combo.List[Option->u.Combo.def]);
         break;
      }

      case uciOption_Button :
      {
         cbutton_OptionButton->SetTitle(Option->name);
         cbutton_OptionButton->Show(true);
         break;
      }

      case uciOption_String :
      {
         cedit_OptionString->SetTitle(Option->u.String.val);
         cedit_OptionString->Show(true);
         Format(descr, "Default: %s", Option->u.String.def);
         break;
      }
   }

   // Finally refresh option description
   if (Option->type != uciOption_Button)
   {  ctext_OptionName->SetTitle(Option->name);
      ctext_OptionName->Show(true);
      ctext_OptionDescr->Show(true);
   }

   ctext_OptionDescr->SetTitle(descr);

} // CUCI_ConfigDialog


static void SetSpinValStr (CTextControl *spinVal, UCI_OPTION *Option)
{
   CHAR spinValStr[50];
   if (UCI_OptionUnitIsMB(Option))
      Format(spinValStr, "                      %d MB", Option->u.Spin.val);
   else if (! EqualStr(Option->name,uciOptionName_MultiPV))
      Format(spinValStr, "                        %d", Option->u.Spin.val);
   else
      CopyStr("Only changable in game window", spinValStr);

   spinVal->SetTitle(spinValStr);
} // SetSpinValStr


/**************************************************************************************************/
/*                                                                                                */
/*                                         SELECT ENGINES                                         */
/*                                                                                                */
/**************************************************************************************************/

void CUCI_ConfigDialog::SelectEngine (UCI_ENGINE_ID engineId)
{
   sigmaApp->ProcessSysEvents();  // Needed so controls are properly activated

   BOOL isSigmaEngine  = (engineId == uci_SigmaEngineId);

   // First update current engine id in prefs
   Prefs.UCI.defaultId = engineId;

   UCI_INFO *Engine = &Prefs.UCI.Engine[engineId];

   // Update info and status   
   ctext_Author->SetTitle(! EqualStr(Engine->author, "") ? Engine->author : "Unknown");
   ctext_LicenseInfo->SetTitle(! EqualStr(Engine->engineAbout, "") ? Engine->engineAbout : "Click the Start button to start using the selected engine, and to access the various engine options");
   ctext_Status->SetTitle(UCI_EngineLoaded(engineId) ? "Loaded" : "Not loaded");
   cedit_Location->SetTitle(Engine->path);

   // Update Enable state
   cbutton_Remove->Enable(! isSigmaEngine);
   cbutton_StartStop->Enable(! isSigmaEngine);
   cbutton_StartStop->SetTitle(UCI_EngineLoaded(engineId) ? "Stop" : "Start");
   cedit_Location->Enable(! isSigmaEngine);
   cbutton_LocationBrowse->Enable(! isSigmaEngine && Engine->local);
   cbutton_DefaultOptions->Enable(! isSigmaEngine);

   RebuildOptionList();
} // CUCI_ConfigDialog::SelectEngine


/**************************************************************************************************/
/*                                                                                                */
/*                                        ADD/REMOVE ENGINES                                      */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- Add Engine Dialog --------------------------------------*/

class CAddEngineDialog : public CFileOpenDialog
{
public:
   CAddEngineDialog (void);
   virtual BOOL Filter (OSTYPE fileType, CHAR *name);
};


CAddEngineDialog::CAddEngineDialog (void)
  : CFileOpenDialog ()
{
} // FilterOpenDialog::FilterOpenDialog


BOOL CAddEngineDialog::Filter (OSTYPE fileType, CHAR *fileName)
{
   return true; //(fileType == 'APPL');
} // CAddEngineDialog::Filter


void CUCI_ConfigDialog::AddEngine (void)
{
   CAddEngineDialog dlg;
   CFile            file;

   if (! dlg.Run(&file, "Add UCI Chess Engine")) return;

   CHAR pathName[uci_EnginePathLen + 1];
   if (FileErr(file.GetPathName(pathName,uci_EnginePathLen))) return;

   UCI_ENGINE_ID newEngineId = UCI_AddLocalEngine(pathName);  // Doesn't return until engine loaded

   if (newEngineId == uci_NullEngineId)
   {
      NoteDialog(this, "Failed Adding Engine", "Please check that this is a valid chess engine supporting the UCI protocol.", cdialogIcon_Error);
   }
   else
   {
      Prefs.UCI.defaultId = newEngineId;
      RebuildEngineMenu(true);
      SelectEngine(Prefs.UCI.defaultId);
   }
} // CUCI_ConfigDialog::AddEngine

/*--------------------------------------- Remove Engine ------------------------------------------*/

void CUCI_ConfigDialog::RemoveEngine (void)
{
   UCI_ENGINE_ID engineId = cpopup_Engines->Get();

   UCI_RemoveEngine(engineId);
   RebuildEngineMenu(true);
   SelectEngine(Prefs.UCI.defaultId);
} // CUCI_ConfigDialog::RemoveEngine

/*--------------------------------------- Set Engine Location --------------------------------------*/

void CUCI_ConfigDialog::SetEngineLocation (void)
{
   CAddEngineDialog dlg;
   CFile            file;

   if (dlg.Run(&file, "Select Engine Location"))
   {
      UCI_INFO *Engine = &Prefs.UCI.Engine[cpopup_Engines->Get()];
      if (! FileErr(file.GetPathName(Engine->path,uci_EnginePathLen)))
      	cedit_Location->SetText(Engine->path);
   }
} // CUCI_ConfigDialog::SetEngineLocation


/**************************************************************************************************/
/*                                                                                                */
/*                                       DIALOG UTILITY                                           */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Rebuild Engine Menu ---------------------------------------*/

void CUCI_ConfigDialog::RebuildEngineMenu (BOOL canSelectEngine)
{
   if (cpopup_Engines)
      delete cpopup_Engines;
   
   CMenu *engineMenu = new CMenu("");
   for (INT i = 0; i < Prefs.UCI.count; i++)
   {  if (i == 1) engineMenu->AddSeparator();
      engineMenu->AddItem(Prefs.UCI.Engine[i].name, i);
   }
   cpopup_Engines = new CPopupMenu(this, "", engineMenu, Prefs.UCI.defaultId, rEnginePopup, true, canSelectEngine);
} // RebuildEngineMenu

/*------------------------------------ Build Options Listbox -------------------------------------*/

void CUCI_ConfigDialog::RebuildOptionList (void)
{
   UCI_INFO *Engine = &Prefs.UCI.Engine[Prefs.UCI.defaultId];
   
   clbox_Options->Clear(Engine->optionCount);
   for (INT i = 0; i < Engine->optionCount; i++)
   {
      CHAR val[100];
      UCI_OptionValueToStr(&Engine->Options[i], val);
      clbox_Options->SetCell(i, 0, Engine->Options[i].name);
      clbox_Options->SetCell(i, 1, val);
   }

   if (Engine->optionCount > 0)
      clbox_Options->SelectRow(0);
   clbox_Options->Redraw();

   HandleListBox(clbox_Options, 0, 0, false);
} // RebuildOptionList

/*------------------------------------ Select/Update Options -------------------------------------*/

UCI_OPTION *CUCI_ConfigDialog::GetSelectedOption (void)
{ 
   INT row = clbox_Options->GetSelectedRow();
   UCI_INFO *Engine = &Prefs.UCI.Engine[Prefs.UCI.defaultId];
      return (row < 0 || row >= Engine->optionCount ? nil : &Engine->Options[row]);
} // CUCI_ConfigDialog::GetSelectedOption


void CUCI_ConfigDialog::SaveOption (UCI_OPTION *Option)
{
   CHAR val[100]; UCI_OptionValueToStr(Option, val);
   clbox_Options->SetCell(clbox_Options->GetSelectedRow(), 1, val);
   clbox_Options->Redraw();

   if (Option->type == uciOption_Spin)
      SetSpinValStr(ctext_OptionSpinVal, Option);
//      ctext_OptionSpinVal->SetTitle(val);

   flushEngineId = Prefs.UCI.defaultId;
   flushOption   = Option;
} // CUCI_ConfigDialog::SaveOption


void CUCI_ConfigDialog::FlushOption (void)
{
   if (flushOption)
   {  UCI_SendOption(flushEngineId, flushOption);
      flushOption = nil;
   }
} // CUCI_ConfigDialog::FlushOption
