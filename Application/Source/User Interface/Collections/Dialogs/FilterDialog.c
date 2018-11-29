/**************************************************************************************************/
/*                                                                                                */
/* Module  : FilterDialog.c                                                                       */
/* Purpose : This module implements the collections filter dialogs.                               */
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

// This dialog resembles "Sherlock" in many ways, i.e. with dynamic conditions.

#include "FilterDialog.h"
#include "PositionFilterDialog.h"
#include "SigmaApplication.h"
#include "SigmaStrings.h"
#include "PGN.h"


#define rowHeight            (RunningOSX() ? 30 : 25)
#define dialogHeight(rows)   (16 + (rows)*rowHeight + (RunningOSX() ? 90 : 70))

class CFilterDialog : public CDialog
{
public:
   CFilterDialog (CRect frame, CHAR *colName, FILTER *Filter, ULONG gameCount);

   virtual void HandlePushButton (CPushButton *ctl);
   virtual void HandlePopupMenu  (CPopupMenu *ctrl, INT itemID);

   FILTER filter;  // Temporary dialog filter

private:
   void CalcButtonFrames (INT rows, CRect *rMore, CRect *rFewer, CRect *rSave, CRect *rOpen, CRect *rCancel, CRect *rOK);

   void FieldMenuSelect (INT i);
   void EnableCondMenu (INT i);
   void MoveButtons (INT rows);
   BOOL ValidateValues (void);
   
   void Save (void);
   void Open (void);

   void StoreFilter (void);
   void LoadFilter (INT oldCount);

   ULONG  gameCount;
   CRect inner;

   // Game Info Filter part:
   CPopupMenu *FieldMenu[maxFilterCond];
   CPopupMenu *CondMenu[maxFilterCond];
   CEditControl *Value[maxFilterCond];
   CPushButton *EditPos[maxFilterCond];

   // Bottom buttons part:
   CPushButton *cbutton_More, *cbutton_Fewer, *cbutton_Save, *cbutton_Open;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                          RUN FILTER DIALOG                                     */
/*                                                                                                */
/**************************************************************************************************/

BOOL FilterDialog (CHAR *colName, FILTER *filter, ULONG gameCount)
{
   CRect frame(0, 0, 490, dialogHeight(filter->count));
   if (RunningOSX()) frame.right += 80;
   theApp->CentralizeRect(&frame);

   CFilterDialog *dialog = new CFilterDialog(frame, colName, filter, gameCount);
   dialog->Run();

   BOOL wasOK = (dialog->reply == cdialogReply_OK);
   if (wasOK) *filter = dialog->filter;

   delete dialog;
   return wasOK;
} /* FilterDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor ------------------------------------------*/

CFilterDialog::CFilterDialog (CRect frame, CHAR *colName, FILTER *theFilter, ULONG vgameCount)
 : CDialog (nil, "Collection Filter", frame, cdialogType_Modal)
{
   filter = *theFilter;
   gameCount = vgameCount;
   inner  = InnerRect();

   // Create header/prompt row:
   CRect rIcon(0,0,32,32); rIcon.Offset(inner.right - 32, inner.top - 5);
   new CIconControl(this, 1320,  rIcon);

   CHAR prompt[300];
   Format(prompt, "Find those games in the collection �%s� where...", colName);
   CRect rPrompt = inner;
   rPrompt.right = rIcon.left - 5;
   rPrompt.bottom = rPrompt.top + controlHeight_Text;
   new CTextControl(this, prompt, rPrompt);

   inner.top += 25 + 10;

   // Create field/cond/value rows part:
   for (INT i = 0; i < maxFilterCond; i++)
   {
      CRect r(0,0,125,controlHeight_PopupMenu);
      if (RunningOSX()) r.right += 23;

      r.Offset(inner.left, inner.top + rowHeight*i);
      CMenu *fmenu = new CMenu("");
      for (INT m = filterField_WhiteOrBlack; m <= filterField_Position; m++)
      {  if (m == filterField_OpeningLine)
            fmenu->AddSeparator();
         fmenu->AddItem(GetStr(sgr_Filter_FieldMenu,m-1), m);
      }
      FieldMenu[i] = new CPopupMenu(this, "", fmenu, filter.Field[i], r, (i < filter.count));

      r.Offset(r.Width() + 10, 0);
      CMenu *cmenu = new CMenu("");
      for (INT m = filterCond_Is; m <= filterCond_Matches; m++)
      {  if (m == filterCond_StartsWith || m == filterCond_Less || m == filterCond_Before || m == filterCond_Matches)
            cmenu->AddSeparator();
         cmenu->AddItem(GetStr(sgr_Filter_CondMenu,m-1), m);
      }
      CondMenu[i] = new CPopupMenu(this, "", cmenu, filter.Cond[i], r, (i < filter.count));

      BOOL isPosRow = (filter.Field[i] == filterField_Position);
      r.Offset(r.Width() + 15, (RunningOSX() ? 3 : -1));
      r.bottom = r.top + controlHeight_Edit;
      r.right = inner.right;
      Value[i] = new CEditControl(this, filter.Value[i], r, filterValueLen, (i < filter.count && ! isPosRow));

      r.Inset(0, (controlHeight_Edit - controlHeight_PushButton)/2);
      EditPos[i] = new CPushButton(this, "Position Filter...", r, (i < filter.count && isPosRow));
   }

   for (INT i = 0; i < maxFilterCond; i++)
      EnableCondMenu(i);

   // Create the buttons last:
   CRect rMore, rFewer, rSave, rOpen, rCancel, rOK;
   CalcButtonFrames(filter.count, &rMore, &rFewer, &rSave, &rOpen, &rCancel, &rOK);

   cbutton_More    = new CPushButton(this, "More",    rMore, true, (filter.count < maxFilterCond));
   cbutton_Fewer   = new CPushButton(this, "Fewer",   rFewer, true, (filter.count > 1));
   cbutton_Save    = new CPushButton(this, "Save...", rSave);
   cbutton_Open    = new CPushButton(this, "Open...", rOpen);
   cbutton_Cancel  = new CPushButton(this, "Cancel",  rCancel);
   cbutton_Default = new CPushButton(this, "Apply",   rOK);
   SetDefaultButton(cbutton_Default);

   CurrControl(FieldMenu[0]);
} /* CFilterDialog::CFilterDialog */


void CFilterDialog::CalcButtonFrames (INT rows, CRect *rMore, CRect *rFewer, CRect *rSave, CRect *rOpen, CRect *rCancel, CRect *rOK)
{
   INT v = inner.top + rowHeight*rows + 5;
   INT width = (RunningOSX() ? 70 : 60);
   INT hspacing = (RunningOSX() ? 12 : 10);

   *rOK = DefaultRect(); rOK->Offset(0, v - rOK->top);
   *rCancel = CancelRect(); rCancel->Offset(0, v - rCancel->top);
   rMore->Set(0,0,width,controlHeight_PushButton); rMore->Offset(inner.left, v);
   *rFewer = *rMore;  rFewer->Offset(rFewer->Width() + hspacing, 0);
   *rSave  = *rFewer; rSave->Offset(rSave->Width() + hspacing, 0);
   *rOpen  = *rSave;  rOpen->Offset(rOpen->Width() + hspacing, 0);
} /* CFilterDialog::CalcButtonFrames */


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

void CFilterDialog::HandlePushButton (CPushButton *ctrl)
{
   StoreFilter();

   if (ctrl == cbutton_Default)
   {
      if (! ValidateValues()) return;
   }
   else if (ctrl == cbutton_Save)
   {
      if (ValidateValues())
         Save();
   }
   else if (ctrl == cbutton_Open)
   {
      Open();
   }
   else if (ctrl == cbutton_More && filter.count < maxFilterCond)
   {
      filter.count++;
      cbutton_More->Enable(false);
      if (filter.count == 2) cbutton_Fewer->Enable(true);

      // Resize dialog:
      Resize(Frame().Width(), Frame().Height() + rowHeight);
      // Move push buttons:
      MoveButtons(filter.count);
      // Show bottom row:
      FieldMenu[filter.count - 1]->Show(true);
      CondMenu[filter.count - 1]->Show(true);
      if (FieldMenu[filter.count - 1]->Get() != filterField_Position)
         Value[filter.count - 1]->Show(true);
      else
         EditPos[filter.count - 1]->Show(true);

      if (filter.count < maxFilterCond) cbutton_More->Enable(true);
      CurrControl(FieldMenu[filter.count - 1]);
   }
   else if (ctrl == cbutton_Fewer && filter.count > 0)
   {
      filter.count--;
      cbutton_Fewer->Enable(false);
      if (filter.count == maxFilterCond - 1) cbutton_More->Enable(true);

      // Hide bottom row:
      FieldMenu[filter.count]->Show(false);
      CondMenu[filter.count]->Show(false);
      Value[filter.count]->Show(false);
      EditPos[filter.count]->Show(false);
      // Move push buttons:
      MoveButtons(filter.count);
      // Resize dialog:
      Resize(Frame().Width(), Frame().Height() - rowHeight);

      if (filter.count > 1) cbutton_Fewer->Enable(true);
   }
   else
   {
      for (INT i = 0; i < filter.count; i++)
         if (ctrl == EditPos[i])
         {  PositionFilterDialog(&(filter.posFilter));
            return;
         }
   }

   // Validation succeeded (or user pressed "Cancel") -> Call inherited function:
   CDialog::HandlePushButton(ctrl);
} /* CFilterDialog::HandlePushButton */


void CFilterDialog::HandlePopupMenu (CPopupMenu *ctrl, INT itemID)
{
   for (INT i = 0; i < maxFilterCond; i++)
      if (ctrl == FieldMenu[i])
      {  FieldMenuSelect(i);
         return;
      }
} /* CFilterDialog::HandlePopupMenu */


void CFilterDialog::FieldMenuSelect (INT i)
{
   EnableCondMenu(i);

   if (FieldMenu[i]->Get() != filterField_Position)
   {
      CondMenu[i]->Set(filterCond_Is);
      EditPos[i]->Show(false);
      Value[i]->SetText("");
      Value[i]->Show(true);
   }
   else
   {
      CondMenu[i]->Set(filterCond_Matches);
      Value[i]->Show(false);
      EditPos[i]->Show(true);
      EditPos[i]->Enable(true);
   }
} /* CFilterDialog::FieldMenuSelect */


void CFilterDialog::MoveButtons (INT rows)
{
   CRect rMore, rFewer, rSave, rOpen, rCancel, rOK;
   CalcButtonFrames(rows, &rMore, &rFewer, &rSave, &rOpen, &rCancel, &rOK);

   cbutton_More->SetFrame(rMore);
   cbutton_Fewer->SetFrame(rFewer);
   cbutton_Save->SetFrame(rSave);
   cbutton_Open->SetFrame(rOpen);
   cbutton_Cancel->SetFrame(rCancel);
   cbutton_Default->SetFrame(rOK);
} /* CFilterDialog::MoveButtons */

/*--------------------------------- Load/Store Filter Fields -------------------------------------*/
// Stores the current dialog state in the FILTER struct.

void CFilterDialog::StoreFilter (void)
{
   for (INT i = 0; i < maxFilterCond; i++)
   {
      filter.Field[i] = FieldMenu[i]->Get();
      filter.Cond[i]  = CondMenu[i]->Get();
      Value[i]->GetTitle(filter.Value[i]);
   }
} /* CFilterDialog::StoreFilter */

// Loads the FILTER struct into the dialog fields

void CFilterDialog::LoadFilter (INT oldCount)
{
   if (filter.count != oldCount)
   {
      Show(false);
      Resize(Frame().Width(), dialogHeight(filter.count));
      MoveButtons(filter.count);
      Show(true);
      DispatchUpdate(Bounds());
   }

   cbutton_More->Enable(filter.count < maxFilterCond);
   cbutton_Fewer->Enable(filter.count > 0);

   for (INT i = 0; i < maxFilterCond; i++)
   {
      BOOL isPosField = (filter.Field[i] == filterField_Position);
   
      FieldMenu[i]->Set(filter.Field[i]);
      CondMenu[i]->Set(filter.Cond[i]);
      Value[i]->SetText(filter.Value[i]);
      EnableCondMenu(i);

      FieldMenu[i]->Show(i < filter.count);
      CondMenu[i]->Show(i < filter.count);
      Value[i]->Show(i < filter.count && ! isPosField);
      EditPos[i]->Show(i < filter.count && isPosField);
   }

   CurrControl(FieldMenu[0]);
} /* CFilterDialog::LoadFilter */


/**************************************************************************************************/
/*                                                                                                */
/*                                              UTILITY                                           */
/*                                                                                                */
/**************************************************************************************************/

void CFilterDialog::EnableCondMenu (INT i)
{
   INT f = FieldMenu[i]->Get();
   INT c1, c2;   // Enable range

   switch (f)
   {
      case filterField_Date :
         c1 = filterCond_Before;
         c2 = filterCond_After;
         break;
      case filterField_WhileELO :
      case filterField_BlackELO :
         c1 = filterCond_Less;
         c2 = filterCond_GreaterEq;
         break;
      case filterField_OpeningLine :
         c1 = filterCond_Is;
         c2 = filterCond_IsNot;
         break;
      case filterField_Position :
         c1 = c2 = filterCond_Matches;
         break;
      default :
         c1 = filterCond_StartsWith;
         c2 = filterCond_Contains;
   }

   CondMenu[i]->EnableItem(filterCond_Is,    f != filterField_Position);
   CondMenu[i]->EnableItem(filterCond_IsNot, f != filterField_Position);
   
   for (INT c = filterCond_StartsWith; c <= filterCond_Matches; c++)
      CondMenu[i]->EnableItem(c, c >= c1 && c <= c2);
} /* CFilterDialog::EnableCondMenu */


BOOL CFilterDialog::ValidateValues (void)
{
   filter.useLineFilter = false;
   filter.usePosFilter  = false;

   for (INT i = 0; i < filter.count; i++)
   {
      switch (filter.Field[i])
      {
         case filterField_WhileELO :
         case filterField_BlackELO :
            LONG n;
            if (! StrToNum(filter.Value[i], &n) || n < 0 || n > 3000)
            {  CurrControl(Value[i]);
               NoteDialog(this, "Invalid ELO Rating", "The ELO rating must be a whole number between 0 and 3000.", cdialogIcon_Error);
               return false;
            }
            break;

         case filterField_OpeningLine :
            if (gameCount > 100 && ! ProVersionDialog(this, "In Sigma Chess Lite, opening line filters are only available for collections with at most 100 games.")) return false;
               
            if (! filter.useLineFilter)
            {
               filter.useLineFilter = true;

               CGame *gameTmp = new CGame();
               CPgn  *pgnTmp  = new CPgn(gameTmp);
                  pgnTmp->ReadBegin(filter.Value[i]);
                  BOOL validLine = pgnTmp->ReadGame(StrLen(filter.Value[i]));
                  if (! validLine)
                     NoteDialog(this, "Invalid Opening Line", "The specified opening line is invalid. You need to enter something like e.g. �1 d4 Nf6 2 c4 e6�", cdialogIcon_Error);
                  else
                  {  for (INT j = 0; j <= gameTmp->lastMove; j++)
                        filter.Line[j] = gameTmp->Record[j];
                     filter.lineLength = gameTmp->lastMove;
                  }
               delete pgnTmp;
               delete gameTmp;

               if (! validLine) return false;
            }
            else
            {  NoteDialog(this, "Opening Line Filter", "Only one opening line filter can be defined.", cdialogIcon_Error);
               return false;
            }
            break;

         case filterField_Position :
            if (gameCount > 100 && ! ProVersionDialog(this, "In Sigma Chess Lite, position filters are only available for collections with at most 100 games.")) return false;

            if (! filter.usePosFilter)
               filter.usePosFilter = true;
            else
            {  NoteDialog(this, "Position Filter", "Only one position filter can be defined.", cdialogIcon_Error);
               return false;
            }
            break;
         }
      }

   return true;
} /* CFilterDialog::ValidateValues */


/**************************************************************************************************/
/*                                                                                                */
/*                                          SAVING/LOADING                                        */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------------- Save to File ------------------------------------------*/

void CFilterDialog::Save (void)
{
   CFile *file = new CFile();

   if (file->SaveDialog("Save Filter", "Untitled"))
   {
      if (file->saveReplace) file->Delete();
      file->SetCreator(sigmaCreator);
      file->SetType('�GCF');
      file->Create();
      file->Save(sizeof(FILTER), (PTR)&filter);
   }

   delete file;
} /* CFilterDialog::Save */

/*--------------------------------------- Load from File -----------------------------------------*/

class FilterOpenDialog : public CFileOpenDialog
{
public:
   FilterOpenDialog (void);
   virtual BOOL Filter (OSTYPE fileType, CHAR *name);
};


FilterOpenDialog::FilterOpenDialog (void)
  : CFileOpenDialog ()
{
} /* FilterOpenDialog::FilterOpenDialog */


BOOL FilterOpenDialog::Filter (OSTYPE fileType, CHAR *fileName)
{
   return (fileType == '�GCF');
} /* FilterOpenDialog::Filter */


void CFilterDialog::Open (void)
{
/*
typedef struct
{
   INT  count;
   INT  Field[maxFilterCond];
   INT  Cond[maxFilterCond];
   CHAR Value[maxFilterCond][filterValueLen + 1];

   // Position filter component:
   BOOL usePosFilter;
   INT  Pos[64];
} FILTER5;

*/
   FilterOpenDialog dlg;
   CFile            file;

   if (dlg.Run(&file,"Open Filter"))
   {
      FILTER tmpFilter;
      ULONG  bytes = sizeof(FILTER);
      INT    oldCount = filter.count;

      ResetFilter(&tmpFilter);
      file.Open(filePerm_Rd);
      file.Read(&bytes, (PTR)&tmpFilter);
      file.Close();

      if (bytes == sizeof(FILTER))
      {
         filter = tmpFilter;
         LoadFilter(oldCount);
      }
      else
      {
         NoteDialog(this, "Open Filter", "Filters created with Sigma Chess 5 are no longer supported.", cdialogIcon_Error);
      }
   }
} /* CFilterDialog::Open */
