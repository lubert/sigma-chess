/**************************************************************************************************/
/*                                                                                                */
/* Module  : CTOOLBAR.C                                                                           */
/* Purpose : Implements a standard toolbar class of CButton objects.                              */
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

#include "CApplication.h"
#include "CToolbar.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       TOOLBAR CONSTRUCTOR                                      */
/*                                                                                                */
/**************************************************************************************************/

CToolbar::CToolbar (CViewOwner *parent, CRect frame)
   : CView(parent, frame)
{
   end = 5;
   sepCount = 0;
} /* CToolbar::CToolbar */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

void CToolbar::HandleUpdate (CRect updateRect)
{
	if (UsingMetalTheme()) return;

   CRect r = bounds;

   if (RunningOSX())
   {
      Draw3DFrame(r, &color_White, &color_BtGray); r.Inset(1, 1);
      DrawStripeRect(r);

      if (Window()->sizeable && frame.right == Window()->bounds.right && frame.bottom == Window()->bounds.bottom)
      {
         SetForeColor(0x9000,0x9000,0x9000);
         MovePenTo(r.right - 3, r.bottom - 2); DrawLine(1, -1);
         MovePenTo(r.right - 7, r.bottom - 2); DrawLine(5, -5);
         MovePenTo(r.right -11, r.bottom - 2); DrawLine(9, -9);

         SetForeColor(&color_LtGray);
         MovePenTo(r.right - 4, r.bottom - 2); DrawLine(2, -2);
         MovePenTo(r.right - 8, r.bottom - 2); DrawLine(6, -6);
         MovePenTo(r.right -12, r.bottom - 2); DrawLine(10, -10);
      }
   }
   else
   {
      Draw3DFrame(r, &color_White, &color_Gray); r.Inset(1, 1);
      DrawRectFill(r, &color_LtGray);
   }

   if (false) //RunningOSX())  //###6.1.4
   {
	   SetForeColor(&color_MdGray);
	   for (INT i = 0; i < sepCount; i++)
	   {
	      MovePenTo(bounds.left + SepPos[i], bounds.top + 4);
	      for (INT v = 0; v < bounds.Height() - 9;  v += 3)
	      {  DrawLine(0,0); MovePen(0,3);
	      }
	   }
   }
   else
   {
	   SetForeColor(&color_BtGray);
	   for (INT i = 0; i < sepCount; i++)
	   {  MovePenTo(bounds.left + SepPos[i], bounds.top + 4);
	      DrawLine(0, bounds.Height() - 9);
	   }
   }
} /* CToolbar::HandleUpdate */


void CToolbar::HandleResize (void)
{
//   Redraw();  ###
} /* CToolbar::HandleResize */


CRect CToolbar::NextItemRect (INT width)
{
   CRect r = bounds;
   r.left = end - 1;
   r.right = r.left + width;
   return r;
} /* CToolbar::NextItemRect */


/**************************************************************************************************/
/*                                                                                                */
/*                                            BUTTONS                                             */
/*                                                                                                */
/**************************************************************************************************/

CButton *CToolbar::AddButton (INT command, INT iconID, INT iconSize, INT width, CHAR *title, CHAR *helpText)
{
   CRect r(0,0,iconSize,iconSize);
   r.Offset(end + (width - iconSize)/2, 2 + iconSize/8);
   end += width;

   return new CButton(this, r,command,nullCommand,true,true, iconID, title, helpText);
} /* CToolbar::AddButton */


CButton *CToolbar::AddPopup (INT command, CMenu *popup, INT iconID, INT iconSize, INT width, CHAR *title, CHAR *helpText)
{
   CRect r(0,0,iconSize,iconSize);
   r.Offset(end + (width - iconSize)/2, 2 + iconSize/8);
   end += width;

   return new CButton(this, r,command,nullCommand,popup,true,true, iconID, title, helpText);
} /* CToolbar::AddButton */


void CToolbar::AddCustomView (CView *view)
{
   end += view->bounds.Width() - 1;
} /* CToolbar::AddCustomView */


/**************************************************************************************************/
/*                                                                                                */
/*                                           SEPARATORS                                           */
/*                                                                                                */
/**************************************************************************************************/

void CToolbar::AddSeparator (INT width)
{
   if (sepCount >= toolbar_MaxSeparators) return;
   SepPos[sepCount++] = end + width/2;
   end += width;
} /* CToolbar::AddSeparator */


/**************************************************************************************************/
/*                                                                                                */
/*                                           TEXT VIEWS                                           */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Lib toolbar Text View -------------------------------------*/

CToolbarTextView::CToolbarTextView (CViewOwner *parent, CRect frame)
 : CView(parent, frame)
{
   SetFontMode(fontMode_Or);
} /* LibTextView::LibTextView */


void CToolbarTextView::HandleUpdate (CRect updateRect)
{
   CRect r = bounds; r.Inset(0,1);
   if (UsingMetalTheme())
      DrawThemeBackground(r);
   else
      DrawStripeRect(r, 0);

	SetFontForeColor();
	MovePenTo(bounds.left, bounds.bottom - 8);
} /* CToolbarTextView::HandleUpdate */


void CToolbarTextView::HandleActivate (BOOL activated)
{
   Redraw();
} /* CToolbarTextView::HandleActivate */

