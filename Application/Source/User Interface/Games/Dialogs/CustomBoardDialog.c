/**************************************************************************************************/
/*                                                                                                */
/* Module  : PROMOTIONDIALOG.C                                                                    */
/* Purpose : This module implements the promotion dialog.                                         */
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

#include "CustomBoardDialog.h"

class BoardTypeView;

class CCustomBoardDialog : public CDialog
{
public:
   CCustomBoardDialog (CRect frame);
   virtual void HandlePushButton (CPushButton *ctl);

   RGB_COLOR whiteCol, blackCol, frameCol;

private:
   void DrawBoard (void);
   void Reactivate (void);

   CPushButton *cbutton_WhiteSquare, *cbutton_BlackSquare, *cbutton_Frame;
   BoardTypeView *boardView;
};


class BoardTypeView : public CView
{
public:
   BoardTypeView (CViewOwner *parent, CRect frame);
   virtual void HandleUpdate (CRect updateRect);
};


#define frsize     6
#define sqsize    12
#define brdsize   (2*frsize + 8*sqsize)


/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG                                      */
/*                                                                                                */
/**************************************************************************************************/

BOOL CustomBoardDialog (void)
{
   CRect frame(0, 0, 180 + brdsize, 75 + brdsize);
   theApp->CentralizeRect(&frame);

   CCustomBoardDialog *dialog = new CCustomBoardDialog(frame);
   dialog->Run();
   BOOL ok = (dialog->reply == cdialogReply_OK);
   delete dialog;

   return ok;
} /* CustomBoardDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor ------------------------------------------*/

CCustomBoardDialog::CCustomBoardDialog (CRect frame)
 : CDialog (nil, "Custom Board Type", frame, cdialogType_Modal)
{
   CRect inner = InnerRect();

   whiteCol = Prefs.Appearance.whiteSquare;
   blackCol = Prefs.Appearance.blackSquare;
   frameCol = Prefs.Appearance.frame;

   CRect r(0,0,120,controlHeight_PushButton); r.Offset(inner.left, inner.top);
   cbutton_WhiteSquare = new CPushButton(this, "White Squares", r); r.Offset(0, controlVDiff_PushButton);
   cbutton_BlackSquare = new CPushButton(this, "Black Squares", r); r.Offset(0, controlVDiff_PushButton);
   cbutton_Frame       = new CPushButton(this, "Board Frame", r);

   CRect br(0,0,brdsize,brdsize);
   br.Offset(inner.right - brdsize, inner.top);
   boardView = new BoardTypeView(this, br);

   cbutton_Cancel  = new CPushButton(this, "Cancel", CancelRect()); SetCancelButton(cbutton_Cancel);
   cbutton_Default = new CPushButton(this, "OK", DefaultRect()); SetDefaultButton(cbutton_Default);
} /* CCustomBoardDialog::CCustomBoardDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

void CCustomBoardDialog::HandlePushButton (CPushButton *ctl)
{
   if (ctl == cbutton_WhiteSquare)
   {
      if (sigmaApp->ColorPicker("White Squares", &whiteCol)) DrawBoard();
      Reactivate();
   }
   else if (ctl == cbutton_BlackSquare)
   {
      if (sigmaApp->ColorPicker("Black Squares", &blackCol)) DrawBoard();
      Reactivate();
   }
   else if (ctl == cbutton_Frame)
   {
      if (sigmaApp->ColorPicker("Board Frame", &frameCol)) DrawBoard();
      Reactivate();
   }
   else if (ctl == cbutton_Default)
   {
      Prefs.Appearance.whiteSquare = whiteCol;
      Prefs.Appearance.blackSquare = blackCol;
      Prefs.Appearance.frame       = frameCol;
   }

   CDialog::HandlePushButton(ctl);
} /* CCustomBoardDialog::HandlePushButton */


void CCustomBoardDialog::Reactivate (void)  // Due to bug in Color Picker
{
   SetFront();
   DispatchActivate(true);
   cbutton_WhiteSquare->Enable(true);
   cbutton_BlackSquare->Enable(true);
   cbutton_Frame->Enable(true);
   cbutton_Cancel->Enable(true);
   cbutton_Default->Enable(true);
} /* CCustomBoardDialog::Reactivate */


void CCustomBoardDialog::DrawBoard (void)
{
  boardView->Redraw();
} /* CCustomBoardDialog::DrawBoard */


/**************************************************************************************************/
/*                                                                                                */
/*                                        THE PREVIEW BOARD                                       */
/*                                                                                                */
/**************************************************************************************************/

BoardTypeView::BoardTypeView (CViewOwner *parent, CRect frame)
 : CView(parent, frame)
{
} /* BoardTypeView::BoardTypeView */


void BoardTypeView::HandleUpdate (CRect updateRect)
{
   CCustomBoardDialog* dialog = (CCustomBoardDialog*)Window();

   CRect fr = bounds;
   DrawRectFill(fr, &(dialog->frameCol));
   Draw3DFrame(fr, &(dialog->frameCol), +30, -30); fr.Inset(frsize - 1, frsize - 1);
   Draw3DFrame(fr, &(dialog->frameCol), -30, +30);

   for (INT y = 0; y < 8; y++)
      for (INT x = 0; x < 8; x++)
      {
         CRect r(0,0,sqsize,sqsize);
         r.Offset(frsize + x*sqsize, frsize + y*sqsize);
         DrawRectFill(r, (odd(x + y) ? &(dialog->blackCol) : &(dialog->whiteCol)));
      }
} /* BoardTypeView::HandleUpdate */
