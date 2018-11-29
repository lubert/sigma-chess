/**************************************************************************************************/
/*                                                                                                */
/* Module  : ANALYZEGAMEDIALOG.C                                                                  */
/* Purpose : This module implements the Initial Position Status dialog used in the pos editor.    */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include "AnalyzeGameDialog.h"
#include "LevelDialog.h"
#include "SigmaApplication.h"
#include "SigmaPrefs.h"

/*
Analyze Game/Collection

Each game in the collection will be replayed move by move, and � Chess will then analyze
each position for the specified time.

The game will be replayed move by move, and � Chess will then analyze
each position for the specified time.

---Time per Move---
Analysis time per move [____] (mm:ss)

---Filter---
Skip positions (i.e. do NOT store the analysis) where
[x] white is to move
[ ] black is to move
[x] � Chess "agrees" with the move that was actually played
[x] The score is less than [___]

WARNING: Existing annotation text for the positions being analyzed will
be overwritten.

[Cancel]  [Analyze]
*/


class CAnalyzeDialog : public CDialog
{
public:
   CAnalyzeDialog (GameWindow *parent, CHAR *title, CRect frame);

   virtual void HandlePushButton (CPushButton *ctrl);
   virtual void HandleCheckBox   (CCheckBox *ctrl);

   CEditControl *cedit_Time;
   CCheckBox    *ccheck_SkipWhite, *ccheck_SkipBlack, *ccheck_SkipMatch, *ccheck_SkipLow;
   CEditControl *cedit_Score;
};


// The dialog simply stores the analysis settings in the prefs file (unless user
// presses "Cancel") and returns true if user presses "Analyze".

BOOL AnalyzeGameDialog (GameWindow *parent, BOOL game)
{
   CRect frame(0, 0, 320, 240);
   if (RunningOSX()) frame.right += 110, frame.bottom += 53;
   theApp->CentralizeRect(&frame);

   CAnalyzeDialog *dialog = new CAnalyzeDialog(parent, (game ? "Analyze Game" : "Analyze Collection"), frame);
   dialog->Run();
   BOOL ok = (dialog->reply == cdialogReply_OK);
   delete dialog;
   return ok;
} /* AnalyzeGameDialog */

/*----------------------------------- Constructor/Destructor -------------------------------------*/

CAnalyzeDialog::CAnalyzeDialog (GameWindow *parent, CHAR *title, CRect frame)
 : CDialog (parent, title, frame)
{
   CRect inner = InnerRect();

   //--- Create group boxes ---
   CRect R1(inner.left, inner.top - 5, inner.right, inner.top + (RunningOSX() ? 47 : 42));
   CRect R2(inner.left, R1.bottom + 3, inner.right, inner.bottom - 35);
   CRect GR1 = R1;
   CRect GR2 = R2;
   R1.Inset(10, 20);
   R2.Inset(10, 20);

   //--- Create "Time" box ---
   CRect r(0,0,170,controlHeight_Text);
   r.Offset(R1.left, R1.top);

   new CTextControl(this, "Analysis time per move (mm:ss)", r);

   CHAR hhmm[10]; FormatHHMM(Prefs.AutoAnalysis.timePerMove, hhmm);
   CRect redit(0,0,42,controlHeight_Edit); redit.Offset(r.right + 5, r.top - (RunningOSX() ? 0 : 3));
   cedit_Time = new CEditControl(this, hhmm, redit, 5);  

   new CGroupBox(this, "Time", GR1);

   //--- Create "Filter" box ---
   r.Set(0,0,250,controlHeight_Text);
   r.Offset(R2.left, R2.top); r.right = R2.right - 6;

   CHAR agreeText[300];
   Format(agreeText, "%s agrees with the move", parent->engineName);
// Format(agreeText, "%s agrees with the move that was played", parent->engineName);
   new CTextControl(this, "Skip positions (don't store analysis) where:", r); r.Offset(10, controlVDiff_CheckBox);
   ccheck_SkipWhite = new CCheckBox(this, "white is to move", Prefs.AutoAnalysis.skipWhitePos, r); r.Offset(0,controlVDiff_CheckBox);
   ccheck_SkipBlack = new CCheckBox(this, "black is to move", Prefs.AutoAnalysis.skipBlackPos, r); r.Offset(0,controlVDiff_CheckBox);
   ccheck_SkipMatch = new CCheckBox(this, agreeText, Prefs.AutoAnalysis.skipMatching, r); r.Offset(0,controlVDiff_CheckBox);
   ccheck_SkipLow   = new CCheckBox(this, "the score improvement is less than", Prefs.AutoAnalysis.skipLowScore, r); r.Offset(0,controlVDiff_CheckBox);

   CHAR score[20];
   ::CalcScoreStr(score, Prefs.AutoAnalysis.scoreLimit);
   if (RunningOSX()) r.Offset(20, 7); else r.Offset(18, 2);
   r.bottom = r.top + controlHeight_Edit;
   r.right = r.left + 50;
   cedit_Score = new CEditControl(this, score, r, 6, true, Prefs.AutoAnalysis.skipLowScore);

   new CGroupBox(this, "Filter", GR2);

   //--- Create "Analyze"/"Cancel" buttons ---
   cbutton_Cancel  = new CPushButton(this, "Cancel", CancelRect()); 
   cbutton_Default = new CPushButton(this, "Analyze", DefaultRect());
   SetDefaultButton(cbutton_Default);
} /* CAnalyzeDialog::CAnalyzeDialog */


void CAnalyzeDialog::HandlePushButton (CPushButton *ctrl)
{
   if (ctrl == cbutton_Default)  // Exit if validation fails
   {
      CHAR s[10];

      cedit_Time->GetTitle(s);
      if (EqualStr(s, "00:00") || ! ParseHHMM(s, &Prefs.AutoAnalysis.timePerMove))
      {  NoteDialog(this, "Invalid Time", "Please use the format 'mm:ss'", cdialogIcon_Error);
         return;
      }

      cedit_Score->GetTitle(s);
      if (! ParseScoreStr(s, &Prefs.AutoAnalysis.scoreLimit) || Prefs.AutoAnalysis.scoreLimit <= 0)
      {  NoteDialog(this, "Invalid Score Improvement Limit", "The Score Improvement must be a positive number measured in units of pawns (e.g. '2.75', '1.5' or '+1').", cdialogIcon_Error);
         return;
      }

      Prefs.AutoAnalysis.skipWhitePos = ccheck_SkipWhite->Checked();
      Prefs.AutoAnalysis.skipBlackPos = ccheck_SkipBlack->Checked();
      Prefs.AutoAnalysis.skipMatching = ccheck_SkipMatch->Checked();
      Prefs.AutoAnalysis.skipLowScore = ccheck_SkipLow->Checked();
   }

   CDialog::HandlePushButton(ctrl);
} /* CAnalyzeDialog::HandlePushButton */


void CAnalyzeDialog::HandleCheckBox (CCheckBox *ctrl)
{
   CDialog::HandleCheckBox(ctrl);

   if (ctrl == ccheck_SkipWhite && ccheck_SkipWhite->Checked())
      ccheck_SkipBlack->Check(false);
   else if (ctrl == ccheck_SkipBlack && ccheck_SkipBlack->Checked())
      ccheck_SkipWhite->Check(false);
   else if (ctrl == ccheck_SkipLow)
      cedit_Score->Enable(ccheck_SkipLow->Checked());
} /* CAnalyzeDialog::HandleCheckBox */
