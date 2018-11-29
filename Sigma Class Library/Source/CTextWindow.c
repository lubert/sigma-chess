/**************************************************************************************************/
/*                                                                                                */
/* Module  : CTextWindow.c                                                                        */
/* Purpose : Implements a simple text/console window (e.g. for debugging).                        */
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

#include "CApplication.h"
#include "CTextWindow.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                          TEXT WINDOW                                           */
/*                                                                                                */
/**************************************************************************************************/

CTextWindow *CreateTextWindow (CHAR *title)
{
   return new CTextWindow(title, theApp->NewDocRect(textWinWidth*6 + 10, textWinLines*11 + 10));
} /* CreateTextWindow */


CTextWindow::CTextWindow (CHAR *title, CRect frame)
 : CWindow(nil, title, frame, cwindowType_Document, false)
{
   for (INT line = 0; line < textWinLines; line++)
      for (INT col = 0; col < textWinWidth; col++)
         buf[line][col] = ' ';
   pos = 0;

   textView = new CTextWinView(this, Bounds());
   Show(true);
} /* CTextWindow::CTextWindow */


void CTextWindow::DrawStr (CHAR *s)
{
   BOOL scroll = false;

   while (CHAR c = *(s++))
   {
      if (IsNewLine(c) || pos == textWinWidth)
      {  NewLine();
         scroll = true;
      }
      else
      {
         buf[textWinLines - 1][pos++] = c;
      }
   }

   if (scroll)
      textView->Redraw();
   else
      textView->DrawLine(textWinLines - 1);
} /* CTextWindow::DrawStr */


void CTextWindow::NewLine (void)
{
   // Scroll line information up:
   for (INT line = 0; line < textWinLines - 1; line++)
      for (INT col = 0; col < textWinWidth; col++)
         buf[line][col] = buf[line + 1][col];
   // Clear contents of last line:
   for (INT col = 0; col < textWinWidth; col++)
      buf[textWinLines - 1][col] = ' ';
   pos = 0;
} /* CTextWindow::NewLine */


/**************************************************************************************************/
/*                                                                                                */
/*                                           TEXT VIEW                                            */
/*                                                                                                */
/**************************************************************************************************/

CTextWinView::CTextWinView (CViewOwner *owner, CRect frame)
 : CView(owner, frame)
{
	SetFontFace(font_Fixed);
	SetFontStyle(fontStyle_Plain);
	SetFontSize(9);
//   CFont font(font_Fixed, fontStyle_Plain, 9);
//   SetFont(&font);
} /* CTextWinView::CTextWinView */


void CTextWinView::HandleUpdate (CRect updateRect)
{
   for (INT line = 0; line < textWinLines; line++)
      DrawLine(line);
} /* CTextWinView::HandleUpdate */


void CTextWinView::DrawLine (INT line)
{
   MovePenTo(5, (line + 1)*FontHeight());
   DrawStr(((CTextWindow*)Window())->buf[line], 0, textWinWidth);
} /* CTextWinView::DrawLine */
