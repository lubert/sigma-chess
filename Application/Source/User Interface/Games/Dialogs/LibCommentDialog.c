/**************************************************************************************************/
/*                                                                                                */
/* Module  : LIBCOMMENTDIALOG.C                                                                   */
/* Purpose : This module implements the "Edit Library comment/eco code" dialog.                   */
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

#include "LibCommentDialog.h"
#include "PosLibrary.h"


class CLibCommentDialog : public CDialog
{
public:
   CLibCommentDialog (CRect frame, CHAR *eco, CHAR *comment);
   virtual void HandlePushButton (CPushButton *ctrl);

   CPushButton  *cbutton_Clear;
   CEditControl *cedit_ECO, *cedit_Comment;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG                                      */
/*                                                                                                */
/**************************************************************************************************/

BOOL LibCommentDialog (CGame *game)
{
   if (! PosLib_Loaded()) return false;

   // First fetch current comment:
   CHAR eco[libECOLength + 1], comment[libCommentLength + 1];
   PosLib_ProbeStr(game->player, game->Board, eco, comment);

   // Then run dialog:
   CRect frame(0,0,290,85);
   if (RunningOSX()) frame.right += 50, frame.bottom += 35;
   theApp->CentralizeRect(&frame);

   CLibCommentDialog *dialog = new CLibCommentDialog(frame, eco, comment);
   dialog->Run();

   if (dialog->reply == cdialogReply_OK)
   {  dialog->cedit_ECO->GetTitle(eco);
      dialog->cedit_Comment->GetTitle(comment);
      PosLib_StoreStr(game->player, game->Board, eco, comment);
      delete dialog;
      return true;
   }
   else
   {  delete dialog;
      return false;
   }
} /* LibCommentDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor ------------------------------------------*/

CLibCommentDialog::CLibCommentDialog (CRect frame, CHAR *eco, CHAR *comment)
 : CDialog (nil, "Edit ECO/Comment", frame)
{
   CRect inner = InnerRect();   

   CRect r = inner;
   r.bottom = r.top + controlHeight_Text; r.right = r.left + 55;
   new CTextControl(this, "ECO", r); r.Offset(0, controlHeight_Text + 5);
   r.bottom = r.top + controlHeight_Edit;
   cedit_ECO = new CEditControl(this, eco, r, 6);

   r = inner;
   r.bottom = r.top + controlHeight_Text; r.left += 65; if (RunningOSX()) r.left += 5;
   new CTextControl(this, "Comment", r); r.Offset(0, controlHeight_Text + 5);
   r.bottom = r.top + controlHeight_Edit;
   cedit_Comment = new CEditControl(this, comment, r, libCommentLength);

   r = CancelRect(); r.Offset(-r.left + inner.left, 0);
   cbutton_Clear    = new CPushButton(this, "Clear", r);
   cbutton_Cancel   = new CPushButton(this, "Cancel", CancelRect());
   cbutton_Default  = new CPushButton(this, "OK", DefaultRect(), true, ! PosLib_Locked());
   SetDefaultButton(cbutton_Default);

   CurrControl(cedit_ECO);
} /* CGotoMoveDialog::CGotoMoveDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

static BOOL ValidECO (CHAR *s);

void CLibCommentDialog::HandlePushButton (CPushButton *ctrl)
{
   if (ctrl == cbutton_Clear)
   {  cedit_ECO->SetText("");
      cedit_Comment->SetText("");
   }
   else if (ctrl == cbutton_Default)
   {
      CHAR eco[libECOLength + 1];
      cedit_ECO->GetText(eco);
      if (StrLen(eco) == 0 || ValidECO(eco))
         CDialog::HandlePushButton(ctrl);
      else
         NoteDialog(this, "Invalid ECO Code", "The ECO code must start with a letter (A...E) followed by two digits (e.g ÒA20Ó or ÒA02/01Ó).", cdialogIcon_Warning);
   }
   else
   {  CDialog::HandlePushButton(ctrl);
   }
} /* CLibCommentDialog::HandlePushButton */


static BOOL ValidECO (CHAR *eco)   // E.g. "A20" or "A01/02"
{
   if (eco[0] < 'A' || eco[0] > 'E') return false;
   if (! IsDigit(eco[1]) || ! IsDigit(eco[2])) return false;
   if (! eco[3]) return true;

   return (eco[3] == '/' && IsDigit(eco[4]) && IsDigit(eco[5]) && ! eco[6]);
} /* ValidECO */
