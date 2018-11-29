/**************************************************************************************************/
/*                                                                                                */
/* Module  : LibImportDialog.c                                                                      */
/* Purpose : This module implements the collection info dialogs.                                  */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

• Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

• Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include "LibImportDialog.h"
#include "SigmaPrefs.h"
#include "SigmaStrings.h"


class CLibImportDialog : public CDialog
{
public:
   CLibImportDialog (CRect frame, CHAR *colFileName, LIBIMPORT_PARAM *param);
   virtual void HandlePushButton (CPushButton *ctrl);

private:
   LIBIMPORT_PARAM *param;
   CPopupMenu   *cpopup_Classify;
   CCheckBox    *ccheck_DontOverwrite;
   CRadioButton *cradio_White, *cradio_Black, *cradio_Both;
   CCheckBox    *ccheck_Loser;
   CEditControl *cedit_MaxMoves;
   CCheckBox    *ccheck_ResolveCap;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN LIB IMPORT DIALOG                                     */
/*                                                                                                */
/**************************************************************************************************/

BOOL LibImportDialog (CHAR *colFileName, LIBIMPORT_PARAM *param)
{
   CRect frame(0, 0, 430, 320);
   if (RunningOSX()) frame.right += 40, frame.bottom += 60;
   theApp->CentralizeRect(&frame);

   CLibImportDialog *dialog = new CLibImportDialog(frame, colFileName, param);
   dialog->Run();
   BOOL done = (dialog->reply == cdialogReply_OK);
   delete dialog;
   return done;
} /* LibImportDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

CLibImportDialog::CLibImportDialog (CRect frame, CHAR *colFileName, LIBIMPORT_PARAM *theParam)
 : CDialog (nil, "Import Library Positions", frame)
{
   param = theParam;

   CRect inner = InnerRect();

   // Add header text control:
   CRect r = inner;
   r.bottom = r.top + 2*controlHeight_Text;
   CHAR header[300];
   Format(header, "Import (classify) moves/positions from the selected games in the collection “%s” to the active position library “%s”", colFileName, Prefs.Library.name);
   new CTextControl(this, header, r, true, controlFont_SmallSystem);

   r.top = r.bottom; r.bottom = r.top + 2;
   new CDivider(this, r);

   // Add classification control:
   r.top = r.bottom + 5; r.bottom = r.top + controlHeight_Text;
   new CTextControl(this, "Classify the imported positions as:", r);
   r.Offset(0, controlVDiff_Text + 3);

   CRect rpm = r; rpm.left += 18; rpm.right = rpm.left + 250;
   rpm.bottom = rpm.top + controlHeight_PopupMenu;
   CMenu *pmenu = new CMenu("");
   for (INT i = libClass_First; i <= libClass_Last; i++)
   {  if (i == 1 || i == 3 || i == 7) pmenu->AddSeparator();
      pmenu->AddItem(GetStr(sgr_LibClassifyMenu,i+1), i);
      pmenu->SetIcon(i, 369 + i);
   }
   cpopup_Classify = new CPopupMenu(this, "", pmenu, param->libClass, rpm);

   r.Offset(18, controlVDiff_Text + 5);
   r.bottom = r.top + controlHeight_Text;
   r.right = inner.right;
   new CTextControl(this, "Note: Choosing “Unclassified” will REMOVE positions from the library", r, true, controlFont_SmallSystem); r.Offset(0, controlVDiff_Text);
   ccheck_DontOverwrite = new CCheckBox(this, "Don't re-classify positions already in the library", ! param->overwrite, r);

   // White/Black/Both radio buttons:
   r.left = inner.left;
   r.top = r.bottom + 10;
   r.bottom = r.top + controlVDiff_Text;
   new CTextControl(this, "Include moves played by:", r); r.Offset(0, controlVDiff_Text);
   r.left += 18; r.right = inner.right;
   cradio_White = new CRadioButton(this, "White", 1, r); r.Offset(0, controlVDiff_RadioButton);
   cradio_Black = new CRadioButton(this, "Black", 1, r); r.Offset(0, controlVDiff_RadioButton);
   cradio_Both  = new CRadioButton(this, "Both sides", 1, r); r.Offset(0, controlVDiff_RadioButton);
   ccheck_Loser = new CCheckBox(this, "Skip moves played by losing side", param->skipLosersMoves, r); r.Offset(0, controlVDiff_RadioButton);
   if (param->impWhite && param->impBlack)
      cradio_Both->Select();
   else if (param->impWhite)
      cradio_White->Select();
   else
      cradio_Black->Select();

   // Max moves
   r.left   = inner.left;
   r.top    = r.top + 10;
   r.bottom = r.top + controlHeight_Text;
   r.right  = r.left + (RunningOSX() ? 140 : 120);
   new CTextControl(this, "Only include the first", r);

   r.left = r.right + 10;
   r.right = r.left + 30;
   r.bottom = r.top + controlHeight_Edit;
   CHAR numstr[10]; NumToStr(param->maxMoves, numstr);
   CRect re = r;
   re.Offset(0, -3);
   cedit_MaxMoves = new CEditControl(this, numstr, re, 2);

   r.left = r.right + 10;
   r.right = r.left + 50;
   r.bottom = r.top + controlHeight_Text;
   new CTextControl(this, "moves", r);

   r.Offset(0, controlVDiff_Text);
   r.left = inner.left + 18; r.right = inner.right;
   ccheck_ResolveCap = new CCheckBox(this, "But continue capture sequences", param->resolveCap, r);

   // Create the OK and Cancel buttons last:
   cbutton_Cancel  = new CPushButton(this, "Cancel", CancelRect());
   cbutton_Default = new CPushButton(this, "OK", DefaultRect());
   SetDefaultButton(cbutton_Default);
} /* CLibImportDialog::CLibImportDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

void CLibImportDialog::HandlePushButton (CPushButton *ctrl)
{
   if (ctrl == cbutton_Default)
   {
      LONG n;
      if (! cedit_MaxMoves->GetLong(&n) || n < 1 || n > 100)
      {  NoteDialog(this, "Invalid Move Limit", "The number of moves must be a whole number between 1 and 100");
         return;
      }

      param->libClass = (LIB_CLASS)(cpopup_Classify->Get());
      param->overwrite = ! ccheck_DontOverwrite->Checked();

      param->impWhite = (cradio_White->Selected() || cradio_Both->Selected());
      param->impBlack = (cradio_Black->Selected() || cradio_Both->Selected());
      param->skipLosersMoves = ccheck_Loser->Checked();

      param->maxMoves = n;
      param->resolveCap = ccheck_ResolveCap->Checked();

      ProVersionDialog(this, "Please note that saving is disabled for position libraries in Sigma Chess Lite.");
   }

   CDialog::HandlePushButton(ctrl);
} /* CLibImportDialog::HandlePushButton */
