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

#include "SpringHeaderView.h"
#include "GameWindow.h"
#include "CollectionWindow.h"
#include "SigmaApplication.h"
#include "BmpUtil.h"

#define springSize 12


//static RGB_COLOR fillColor = color_BrGray;
static RGB_COLOR fillColor = { 0xE500, 0xE500, 0xFFFF };


/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

SpringHeaderView::SpringHeaderView (CViewOwner *parent, CRect frame, BOOL vdivider, BOOL vclosed, BOOL vblackFrame)
 : CView(parent,frame)
{
   CopyStr("", headerStr);
   divider = vdivider;
   closed = vclosed;
   blackFrame = vblackFrame;
} /* SpringHeaderView::SpringHeaderView */


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

void SpringHeaderView::HandleUpdate (CRect updateRect)
{
   CRect r = bounds;

   if (blackFrame)
   {  SetForeColor(RunningOSX() || ! Active() ? &color_MdGray : &color_Black);
      DrawRectFrame(r); r.Inset(1, 1);
   }

   if (! RunningOSX())
   {
      Draw3DFrame(r, &color_White, &color_Gray); r.Inset(1, 1);
      DrawRectFill(r, &color_LtGray);

      SetBackColor(&color_White);
      CRect src(0,0,springSize,springSize);
      CRect dst = src; dst.Offset(bounds.left + 3, bounds.top + 3);
      DrawBitmap(GetBmp(closed ? 1301 : 1302), src, dst, bmpMode_Trans);

	   if (divider && ! closed)
	   {
	      r.Inset(-1, -1);
	      SetForeColor(&color_Gray);
	      MovePenTo(r.left, r.top + headerViewHeight - 3); DrawLine(r.Width() - 1, 0);
	      SetForeColor(&color_White);
	      MovePenTo(r.left, r.top + headerViewHeight - 2); DrawLine(r.Width() - 1, 0);
	   }
   }
   else if (! closed)
   {
      r.top += headerViewHeight - 1;
//      Draw3DFrame(r, &color_White, &color_Gray); r.Inset(1, 1);
      DrawRectFill(r, &fillColor);
   }
} /* SpringHeaderView::HandleUpdate */


BOOL SpringHeaderView::HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)
{
   CRect r;

   if (! RunningOSX())
   {
      r.Set(0,0,springSize,springSize);
      r.Offset(bounds.left + 3, bounds.top + 3);
   }
   else
   {
      SetFontSize(10);

	   BOOL isPushed = true;
      r = bounds;
      if (blackFrame) r.Inset(1, 1);
      r.bottom = r.top + headerViewHeight - 1;

      DrawThemeListHeaderCell(r, headerStr, 0, true, isPushed, closed);

      MOUSE_TRACK_RESULT trackResult;
      do
      {
         TrackMouse(&pt, &trackResult);
         if (trackResult != mouseTrack_Released && isPushed != pt.InRect(r))
            DrawThemeListHeaderCell(r, headerStr, 0, true, isPushed = ! isPushed, closed);
      } while (trackResult != mouseTrack_Released);

	   if (isPushed)
         DrawThemeListHeaderCell(r, headerStr, 0, true, false, closed);

      SetFontSize(9);
   }

   if (! pt.InRect(r)) return false;

   closed = ! closed;
   HandleToggle(closed);
   return true;
} /* SpringHeaderView::HandleMouseDown */


void SpringHeaderView::HandleActivate (BOOL wasActivated)
{
   Redraw();
} /* SpringHeaderView::HandleActivate */


void SpringHeaderView::HandleResize (void)            // Dummy routine. Should be overridden
{
   Redraw();
} /* SpringHeaderView::HandleResize */


void SpringHeaderView::HandleToggle (BOOL closed)     // Dummy routine. Should be overridden
{
} /* SpringHeaderView::HandleToggle */


/**************************************************************************************************/
/*                                                                                                */
/*                                          MISCELLANEOUS                                         */
/*                                                                                                */
/**************************************************************************************************/

BOOL SpringHeaderView::Closed (void)
{
   return closed;
} /* SpringHeaderView::Closed */


void SpringHeaderView::DrawHeaderStr (CHAR *s)
{
   CopyStr(s, headerStr);

   SetFontSize(10);

   if (RunningOSX())
   {
      CRect r = bounds;
      if (blackFrame) r.Inset(1, 1);
      r.bottom = r.top + headerViewHeight - 1;
      DrawThemeListHeaderCell(r, s, 0, true, false, closed);

	   SetFontForeColor();
      SetBackColor(&fillColor);
   }
   else
   {
	   SetFontForeColor();
	   SetBackColor(&color_LtGray);

	   CRect r = bounds;
	   r.Inset(5, 3);
	   r.left += 12;
	   r.bottom = r.top + 12;

	   DrawStr(s, r, textAlign_Left, false);
   }

   SetFontSize(9);
} /* SpringHeaderView::DrawHeaderStr */


void SpringHeaderView::DrawStrPair (INT h, INT v, CHAR *tag, CHAR *value)
{
   SetBackColor(RunningOSX() ? &fillColor : &color_LtGray);
   MovePenTo(h, v); SetFontStyle(fontStyle_Bold); DrawStr(tag);
   MovePenTo(h + 40, v); SetFontStyle(fontStyle_Plain); DrawStr(value);
} /* GameHeaderView::DrawStrPair */


CGame *SpringHeaderView::Game (void)
{
   return ((GameWindow *)Window())->game;
} /* SpringHeaderView::Game */


SigmaCollection *SpringHeaderView::Collection (void)
{
   return ((CollectionWindow *)Window())->collection;
} /* SpringHeaderView::Collection */
