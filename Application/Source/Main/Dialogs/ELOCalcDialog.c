/**************************************************************************************************/
/*                                                                                                */
/* Module  : ELOCALCDIALOG.C                                                                      */
/* Purpose : This module implements the ELO Calculator dialog.                                    */
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

#include "ELOCalcDialog.h"
#include "StrengthDialog.h"

#include "Rating.h"
#include "SigmaApplication.h"
#include "SigmaPrefs.h"
#include "SigmaStrings.h"
#include "CDialog.h"
#include "DataHeaderView.h"
#include "DataView.h"

#define hscale 5
#define vscale 3

/*--------------------------------- Dialog Class Definitions -------------------------------------*/

class ELOGraphView;
class FIDECatView;

class CELODialog : public CDialog
{
public:
   CELODialog (CRect frame);

   virtual void HandlePushButton (CPushButton *ctrl);

private:
   void BuildTriple (CPoint pt, CHAR *text, INT len, CEditControl **cedit, CPushButton **cbutton);

   ELOGraphView *graph;

   CEditControl *cedit_RelScore, *cedit_Diff;
   CPushButton  *cbutton_RelScore, *cbutton_Diff;

   CEditControl *cedit_AbsScore, *cedit_YourELO, *cedit_OppELO;
   CPushButton  *cbutton_AbsScore, *cbutton_YourELO, *cbutton_OppELO;
};


class ELOGraphView : public CView
{
public:
   ELOGraphView (CViewOwner *parent, CRect frame);
   virtual void HandleUpdate (CRect updateRect);

   void SetELODiff (INT diff);  // Sets "x" value and calculates "y" value
   void SetScore (INT score);   // Sets "y" value and calculates "x" value

   void DrawAxis (void);
   void DrawGraph (void);

private:
   CRect gr;   // Inner graph rectangle (above right to axis)
   INT diff, score;
};


class FIDECatView : public CView
{
public:
   FIDECatView (CViewOwner *parent, CRect frame);
   virtual void HandleUpdate (CRect updateRect);
};


/**************************************************************************************************/
/*                                                                                                */
/*                                    RATING CALCULATOR DIALOG                                    */
/*                                                                                                */
/**************************************************************************************************/

void RatingCalculatorDialog (void)
{
   CRect frame(0, 0, 500, 380);
   if (RunningOSX()) frame.right += 50, frame.bottom += 50;
   theApp->CentralizeRect(&frame);

   CELODialog *dialog = new CELODialog(frame);
   dialog->Run();
   delete dialog;
} /* RatingCalculatorDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                       DIALOG CONSTRUCTOR                                       */
/*                                                                                                */
/**************************************************************************************************/

CELODialog::CELODialog (CRect frame)
 : CDialog (nil, "ELO Calculator", frame, cdialogType_Modal)
{
   CRect inner = InnerRect();

   // Calc group box frames:
   CRect R1(0,0,280,230);
   R1.Offset(inner.left, inner.top - 5);
   CRect GR1 = R1;

   CRect R2 = R1;
   R2.left = R1.right + 6; R2.right = inner.right;
   CRect GR2 = R2;

   CRect R3 = inner;
   R3.top = R1.bottom + 5;
   R3.bottom -= 30;
   CRect R4 = R3;
   R3.right = R3.left + inner.Width()/2 - 3;
   R4.left = R3.right + 6;
   CRect GR3 = R3;
   CRect GR4 = R4;

   // Create ELO Graph view:
   CRect gr = R1; gr.Inset(14, 14); gr.top += 7;
   new ELOGraphView(this, gr);

   // Create FIDE Table view:
   CRect fr = R2; fr.Inset(14, 14); fr.top += 7;
   new FIDECatView(this, fr);

   // Create ELO Calculator controls:
   INT dv = (RunningOSX() ? 30 : 25);
   R3.Inset(10, dv);
   BuildTriple(CPoint(R3.left,R3.top), "Score (%)", 3, &cedit_RelScore, &cbutton_RelScore);
   BuildTriple(CPoint(R3.left,R3.top + dv), "ELO Diff", 4, &cedit_Diff, &cbutton_Diff);

   R4.Inset(10, dv);
   BuildTriple(CPoint(R4.left,R4.top), "Score (%)", 3, &cedit_AbsScore, &cbutton_AbsScore);
   BuildTriple(CPoint(R4.left,R4.top + dv), "Your ELO", 4, &cedit_YourELO, &cbutton_YourELO);
   BuildTriple(CPoint(R4.left,R4.top + 2*dv), "Opponent ELO", 4, &cedit_OppELO, &cbutton_OppELO);

   // Create group boxes:
   new CGroupBox(this, "ELO Graph", GR1);
   new CGroupBox(this, "FIDE Categories", GR2);
   new CGroupBox(this, "ELO Calculator (Relative)", GR3);
   new CGroupBox(this, "ELO Calculator (Absolute)", GR4);

   // Finally Create the OK and Cancel buttons
   cbutton_Default = new CPushButton(this, GetStr(sgr_Common,s_Close), DefaultRect());
   SetDefaultButton(cbutton_Default);

   CurrControl(cedit_RelScore);
} /* CELODialog::CELODialog */


void CELODialog::BuildTriple (CPoint pt, CHAR *text, INT len, CEditControl **cedit, CPushButton **cbutton)
{
   INT twidth = (RunningOSX() ? 96 : 75);
   INT ewidth = (RunningOSX() ? 40 : 40);
   INT dv = (RunningOSX() ? 0 : 3);

   CRect r(pt.h, pt.v + dv, pt.h + twidth, pt.v + controlHeight_Text + dv);
   new CTextControl(this, text, r);

   r.Set(pt.h, pt.v, pt.h + ewidth, pt.v + controlHeight_Edit); r.Offset(twidth + 8, 0);
   *cedit = new CEditControl(this, "", r, len);

   r.Set(pt.h, pt.v, pt.h + ewidth + 10, pt.v + controlHeight_PushButton - 1);
   r.Offset(twidth + ewidth + 25, (RunningOSX() ? -2 : 0));
   *cbutton = new CPushButton(this, "<- Calc", r, true, true, false);
} /* CELODialog::BuildTriple */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

void CELODialog::HandlePushButton (CPushButton *ctl)
{
   LONG n, m;
   CHAR s[10];

   if (ctl == cbutton_RelScore)      // DIFF --> REL SCORE
   {
      cedit_RelScore->SetText("");
      if (! cedit_Diff->ValidateNumber(-999,999,false))
         NoteDialog(this, "Invalid ELO Difference", "The ELO difference must be a number between 0 and 999");
      else
      {  cedit_Diff->GetLong(&n);
         m = (n >= 0 ? 100*ELO_to_Score(n) : 100*(1 - ELO_to_Score(-n)));
         NumToStr(m, s);
         cedit_RelScore->SetText(s);
      }
   }
   else if (ctl == cbutton_Diff)     // REL SCORE --> DIFF
   {
      cedit_Diff->SetText("");
      if (! cedit_RelScore->ValidateNumber(1,99,false))
         NoteDialog(this, "Invalid Score", "The score must be a percentage between 1 % and 99 %");
      else
      {  cedit_RelScore->GetLong(&n);
         m = (n >= 50 ? Score_to_ELO((REAL)n/100.0) : -Score_to_ELO(1.0 - (REAL)n/100.0));
         NumToStr(m, s);
         cedit_Diff->SetText(s);
      }
   }
   else if (ctl == cbutton_AbsScore)  // YOUR ELO, OPP ELO --> ABS SCORE 
   {
      cedit_AbsScore->SetText("");
      if (! cedit_YourELO->ValidateNumber(800,3000,false) || ! cedit_OppELO->ValidateNumber(800,3000,false))
         NoteDialog(this, "Invalid ELO Rating", "The ELO rating must be a number between 800 and 3000");
      else
      {  cedit_YourELO->GetLong(&n);
         cedit_OppELO->GetLong(&m);
         NumToStr(100*ELO_to_Score(n - m), s);
         cedit_AbsScore->SetText(s);
      }
   }
   else if (ctl == cbutton_YourELO)   // ABS SCORE, OPP ELO --> YOUR ELO
   {
      cedit_YourELO->SetText("");
      if (! cedit_AbsScore->ValidateNumber(1,99,false))
         NoteDialog(this, "Invalid Score", "The score must be a percentage between 1 % and 99 %");
      else if (! cedit_OppELO->ValidateNumber(800,3000,false))
         NoteDialog(this, "Invalid ELO Rating", "The ELO rating must be a number between 800 and 3000");
      else
      {  cedit_AbsScore->GetLong(&n);
         cedit_OppELO->GetLong(&m);
         m += (n >= 50 ? Score_to_ELO((REAL)n/100.0) : -Score_to_ELO(1.0 - (REAL)n/100.0));
         NumToStr(m, s);
         cedit_YourELO->SetText(s);
      }
   }
   else if (ctl == cbutton_OppELO)   // ABS SCORE, YOUR ELO --> OPP ELO
   {
      cedit_OppELO->SetText("");
      if (! cedit_AbsScore->ValidateNumber(1,99,false))
         NoteDialog(this, "Invalid Score", "The score must be a percentage between 1 % and 99 %");
      else if (! cedit_YourELO->ValidateNumber(800,3000,false))
         NoteDialog(this, "Invalid ELO Rating", "The ELO rating must be a number between 800 and 3000");
      else
      {  cedit_AbsScore->GetLong(&n);
         cedit_YourELO->GetLong(&m);
         m -= (n >= 50 ? Score_to_ELO((REAL)n/100.0) : -Score_to_ELO(1.0 - (REAL)n/100.0));
         NumToStr(m, s);
         cedit_OppELO->SetText(s);
      }
   }

   CDialog::HandlePushButton(ctl);
} /* CELODialog::HandlePushButton */


/**************************************************************************************************/
/*                                                                                                */
/*                                        ELO GRAPH VIEW                                          */
/*                                                                                                */
/**************************************************************************************************/

// This wiew should have a with of 250 and heightof 200.
// Scale : 1 pixel = 5 ELO points

ELOGraphView::ELOGraphView (CViewOwner *parent, CRect frame)
 : CView(parent, frame)
{
   SetBackColor(&color_Dialog);
   gr = bounds;
   gr.Inset(20, 20);
   diff = 0; score = 50;
   SetFontSize(9);
   SetFontMode(fontMode_Or);
} /* ELOGraphView::ELOGraphView */


void ELOGraphView::HandleUpdate (CRect updateRect)
{
// DrawRectFill(bounds, &color_White); //###

   DrawAxis();
   DrawGraph();
   // DrawMarker();
} /* ELOGraphView::HandleUpdate */


void ELOGraphView::DrawAxis (void)
{
   SetForeColor(&color_Black);

   //--- Draw horisontal "ELO Diff" axis ---
   CHAR *diffLabel = "ELO Diff";

   MovePenTo(gr.left - 1, gr.bottom);
   DrawLine(gr.Width() + 10, 0);
   DrawLine(-2, -2); DrawLine(0, 4); DrawLine(2, -2);

   SetFontStyle(fontStyle_Bold);
   MovePenTo(gr.right - StrWidth(diffLabel)/2, gr.bottom - 4); DrawStr(diffLabel);

   SetFontStyle(fontStyle_Plain);
   for (INT d = 0; d <= 1000; d += 100)
   {
      MovePenTo(gr.left + d/hscale - 1, gr.bottom); DrawLine(0, 1);
      if (d % 500 == 0)
      {  CHAR s[10]; NumToStr(d, s);
         MovePen(-StrWidth(s)/2, 12); DrawStr(s);
      }
   }

   //--- Draw vertical "Score (%)" axis ---
   CHAR *scoreLabel = "Score (%)";

   MovePenTo(gr.left - 1, gr.bottom);
   DrawLine(0, -gr.Height() - 5);
   DrawLine(-2, 2); DrawLine(4, 0); DrawLine(-2,-2);

   SetFontStyle(fontStyle_Bold);
   MovePenTo(bounds.left, gr.top - 9); DrawStr(scoreLabel);
   
   SetFontStyle(fontStyle_Plain);
   for (INT p = 50; p <= 100; p += 10)
   {
      MovePenTo(gr.left - 2, gr.bottom - (p - 50)*vscale); DrawLine(1, 0);
      CHAR s[10]; NumToStr(p, s);
      MovePen(-StrWidth(s) - 5, 4); DrawStr(s);
   }
} /* ELOGraphView::DrawAxis */


void ELOGraphView::DrawGraph (void)
{
   SetForeColor(&color_Red);

   for (INT d = 0; d <= 1000; d++)
      DrawPoint(gr.left + d/hscale, gr.bottom - 100*vscale*(ELO_to_Score(d) - .5));

   SetForeColor(&color_Black);
} /* ELOGraphView::DrawGraph */


/**************************************************************************************************/
/*                                                                                                */
/*                                      FIDE CATEGORY VIEW                                        */
/*                                                                                                */
/**************************************************************************************************/

#define catTabWidth 125

static HEADER_COLUMN HCTab[2] = {{"Category",0,catTabWidth},{"ELO",0,-1}};

FIDECatView::FIDECatView (CViewOwner *parent, CRect frame)
 : CView(parent, frame)
{
   CRect r = bounds;
   r.Inset(1, 1); r.bottom = r.top + headerViewHeight;
   new DataHeaderView(this, r, false, true, 2, HCTab);
} /* FIDECatView::FIDECatView */


void FIDECatView::HandleUpdate (CRect updateRect)
{
   Draw3DFrame(bounds, &color_Gray, &color_White);

   CRect r = bounds;
   r.Inset(1,1); r.top += headerViewHeight - 1;

   // Draw frame
   DrawRectFill(r, &color_White);
   SetForeColor(&color_Black);
   DrawRectFrame(r);
   MovePenTo(r.left + catTabWidth, r.top);
   DrawLineTo(r.left + catTabWidth, r.bottom - 1);

   // Draw contents
   SetForeColor(&color_Black);
   SetFontMode(fontMode_Or);
   SetFontSize(9);
   r.left += 5;
   INT v = r.top + 15;
   for (INT i = 0; i < categoryCount; i++, v += 15)
   {  
      MovePenTo(r.left, v); DrawStr(GetStr(sgr_PSD_Cat,i));
      MovePenTo(r.left + catTabWidth, v); DrawNum(CategoryMap[i]); DrawStr("+");
   }
} /* FIDECatView::HandleUpdate */
