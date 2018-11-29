/**************************************************************************************************/
/*                                                                                                */
/* Module  : ColInfoDialog.c                                                                      */
/* Purpose : This module implements the collection info dialogs.                                  */
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

#include "MoveGamesDialog.h"
#include "SigmaApplication.h"


class CMoveGamesDialog : public CDialog
{
public:
   CMoveGamesDialog (CRect frame, CHAR *title, LONG gfrom, LONG count, LONG totalCount);
   virtual void HandlePushButton (CPushButton *ctrl);

   CEditControl *cedit_To;
   LONG gmax;  // Valid range 
};


/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG                                      */
/*                                                                                                */
/**************************************************************************************************/

BOOL MoveGamesDialog (LONG gfrom, LONG count, LONG totalCount, LONG *gto)
{
   CRect frame(0, 0, 300, 80);
   if (RunningOSX()) frame.right += 20, frame.bottom += 20;
   theApp->CentralizeRect(&frame);

   CMoveGamesDialog *dialog = new CMoveGamesDialog(frame, "Move/Renumber Collection Games", gfrom, count, totalCount);
   dialog->Run();

   BOOL done = false;
   if (dialog->reply == cdialogReply_OK)
   {  dialog->cedit_To->GetLong(gto);
      *gto = *gto - 1;
      done = true;
   }

   delete dialog;
   return done;
} /* MoveGamesDialog */


CMoveGamesDialog::CMoveGamesDialog (CRect frame, CHAR *title, LONG gfrom, LONG count, LONG totalCount)
 : CDialog (nil, title, frame)
{
   CRect inner = InnerRect();

   gmax = totalCount - count;
   
   CRect r = inner; r.bottom = r.top + 30;
   CHAR msg[200];
   Format(msg, "Move/renumber the %ld game(s) starting with game number %ld to game number:", count, gfrom + 1);
   new CTextControl(this, msg, r, true, controlFont_Views);

   r.Offset(0, 40); r.right = r.left + 60; r.bottom = r.top + controlHeight_Edit;
   cedit_To = new CEditControl(this, "", r, 6);

   // Create the OK and Cancel buttons last:
   cbutton_Cancel  = new CPushButton(this, "Cancel", CancelRect());
   cbutton_Default = new CPushButton(this, "OK", DefaultRect());
   SetDefaultButton(cbutton_Default);

   CurrControl(cedit_To);
} /* CMoveGamesDialog::CMoveGamesDialog */


void CMoveGamesDialog::HandlePushButton (CPushButton *ctrl)
{
   if (ctrl == cbutton_Default)
   {
      if (! cedit_To->ValidateNumber(1, gmax + 1, false))
      {
         CHAR s[100];
         Format(s, "The game number must be a whole number between 1 and %ld...", gmax + 1);
         NoteDialog(this, "Invalid Game Number", s, cdialogIcon_Error);
         return;
      }
   }
   CDialog::HandlePushButton(ctrl);
} /* CMoveGamesDialog::HandlePushButton */
