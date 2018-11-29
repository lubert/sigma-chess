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

#include "BackView.h"
#include "SigmaApplication.h"
#include "SigmaPrefs.h"
#include "SigmaStrings.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

BackView::BackView (CViewOwner *parent, CRect frame, BOOL vAutoOutline)
 : CView(parent,frame)
{
   autoOutline = vAutoOutline;
   exRect.Set(0,0,0,0);
} /* BackView::BackView */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

void BackView::HandleUpdate (CRect updateRect)
{
	if (UsingMetalTheme()) return;

   CRect r = bounds;

   // Draw the black frame (outside backview):
   r.Inset(-1, -1);
   if (! RunningOSX()) SetForeColor(&color_Black);

   // Stroke 3D Frame
   if (autoOutline) Outline3DRect(r, false);

   // Paint interior:
   r.Inset(3, 3);
   if (exRect.left == exRect.right)
      DrawRectFill(r, &sigmaPrefs->mainColor);
   else
   {
      CRect r1 = r;
      CRect r2 = exRect; r2.Inset(-2, -2);
      CRect xr;
      xr.Set(r1.left,  r1.top,    r1.right, r2.top);    DrawRectFill(xr, &sigmaPrefs->mainColor);
      xr.Set(r1.left,  r2.bottom, r1.right, r1.bottom); DrawRectFill(xr, &sigmaPrefs->mainColor);
      xr.Set(r1.left,  r2.top,    r2.left,  r2.bottom); DrawRectFill(xr, &sigmaPrefs->mainColor);
      xr.Set(r2.right, r2.top,    r.right,  r2.bottom); DrawRectFill(xr, &sigmaPrefs->mainColor);
      Outline3DRect(exRect);
   }

   if (autoOutline)
   {
      DrawTopRound();
      DrawBottomRound();
   }
} /* BackView::HandleUpdate */


void BackView::DrawTopRound (void)
{
	RGBColor c = sigmaPrefs->mainColor;

   for (INT i = 10; i >= 0; i--)
   {
      if (i == 0)
      	SetForeColor(&sigmaPrefs->lightColor);
      else
      {  AdjustColorLightness(&c, +1);
         SetForeColor(&c);
      }
      MovePenTo(bounds.left + 2, bounds.top + i);
      DrawLineTo(bounds.right - 3, bounds.top + i);
   }
} // BackView::DrawTopRound


void BackView::DrawBottomRound (void)
{
	RGBColor c = sigmaPrefs->mainColor;

   for (INT i = 10; i >= 0; i--)
   {
      if (i == 0)
      	SetForeColor(&sigmaPrefs->darkColor);
      else
      {  AdjustColorLightness(&c, (i > 4 ? -1 : -2));
         SetForeColor(&c);
      }
      MovePenTo(bounds.left + 2, bounds.bottom - i - 1);
      DrawLineTo(bounds.right - 3, bounds.bottom - i - 1);
   }
} // BackView::DrawBottomRound


BOOL BackView::HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)
{
   if (modifiers & modifier_Control)    // Show color scheme popup menu:
   {
      INT g = sgr_ColorSchemeMenu;
      CMenu *pm = new CMenu("");
      pm->AddPopupHeader(GetStr(g,0));
      for (INT cmd = colorScheme_First; cmd <= colorScheme_Last; cmd++)
      {  pm->AddItem(GetStr(g,cmd - colorScheme_First + 1), cmd);
         pm->SetIcon(cmd, icon_ColorScheme - 1 + cmd - colorScheme_First);
         if (cmd == colorScheme_First) pm->AddSeparator();
      }

      INT current = colorScheme_First + Prefs.Appearance.colorScheme;
      pm->CheckMenuItem(current, true);
      INT msg;
      if (pm->Popup(&msg)) sigmaApp->HandleMessage(msg);
      delete pm;
      return true;
   }

   return false;
} /* BackView::HandleMouseDown */


/**************************************************************************************************/
/*                                                                                                */
/*                                      SETTING CHARACTERISTICS                                   */
/*                                                                                                */
/**************************************************************************************************/

void BackView::ExcludeRect (CRect r)
{
   exRect = r;
} /* BackView::ExcludeRect */


/**************************************************************************************************/
/*                                                                                                */
/*                                             DRAWING                                            */
/*                                                                                                */
/**************************************************************************************************/

void BackView::Outline3DRect (CRect r, BOOL impress)
{
	if (UsingMetalTheme()) return;

   if (impress)
   {
   	// V6.1.4
   	if (RunningOSX())
   	{
	   	SetForeColor(&sigmaPrefs->mainColor);
	      r.Inset(-1, -1);
	      DrawRectFrame(r);
	      r.Inset(-1, -1);
	      DrawRectFrame(r);
     	}
     	else
     	{
	      r.Inset(-1, -1);
	      Draw3DFrame(r, &sigmaPrefs->darkColor, &sigmaPrefs->lightColor);
	      r.Inset(-1, -1);
	      Draw3DFrame(r, &sigmaPrefs->mainColor, -10, +10);
      }
   }
   else
   {  r.Inset(1, 1);
      Draw3DFrame(r, &sigmaPrefs->lightColor, &sigmaPrefs->darkColor);
      r.Inset(1, 1);
      Draw3DFrame(r, &sigmaPrefs->mainColor, +10, -10);
   }
} /* BackView::Outline3DRect */


CRect BackView::DataViewRect (void)   // Default data view rectangle 
{
   CRect r = bounds;
   r.Inset(8, 8);
   return r;
} /* BackView::DataViewRect */
