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

#include "PromotionDialog.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                      RUN GAME INFO DIALOG                                      */
/*                                                                                                */
/**************************************************************************************************/

CPromotionDialog *PromotionDialog (COLOUR player)
{
   CRect frame(0, 0, 4*(pieceButtonSize - 1) + 20, pieceButtonSize + 40);
   if (RunningOSX()) frame.right += 20, frame.bottom += 25;
   theApp->CentralizeRect(&frame);

   CPromotionDialog *dialog = new CPromotionDialog(frame, player);

   dialog->Show(true);
   dialog->SetFront();

   return dialog;
} /* PromotionDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor ------------------------------------------*/

CPromotionDialog::CPromotionDialog (CRect frame, COLOUR thePlayer)
 : CDialog (nil, "Promotion", frame, cdialogType_Modal)
{
   player = thePlayer;

   CRect inner = InnerRect();   

   CRect r = inner; r.bottom = r.top + controlHeight_Text;
   new CTextControl(this, "Select promotion piece", r); //, true, controlFont_SmallSystem);

   for (INT i = 0; i < 4; i++)
   {
      PIECE p = player + queen - i;
      CRect dst(0,0,pieceButtonSize,pieceButtonSize);
      dst.Offset(inner.left + i*(pieceButtonSize - 1), inner.bottom - pieceButtonSize);
      CRect src = pieceBmp1->CalcPieceRect(p);
      button[i] =
         new CButton
         (  this,dst,p, p, true, true,
            pieceBmp1, pieceBmp1, &src, &src, "", &color_Blue
         );
   }
} /* CPromotionDialog::CPromotionDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

void CPromotionDialog::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   if (key == key_Enter || key == key_Return)
   {
      button[0]->Press(true);
      Sleep(5);
      button[0]->Press(false);
      HandleMessage(player + queen);
   }
} /* CPromotionDialog::HandleKeyDown */


void CPromotionDialog::HandleMessage (LONG msg, LONG submsg, PTR data)
{
   prom = msg;
   modalRunning = false;
} /* CPromotionDialog::HandleMessage */
