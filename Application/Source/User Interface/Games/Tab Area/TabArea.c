/**************************************************************************************************/
/*                                                                                                */
/* Module  : TabArea.c                                                                            */
/* Purpose : This module defines the "Tab Area" view used to control the state/mode of the info   */
/*           area.                                                                                */
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

#include "TabArea.h"
#include "GameWindow.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                         DIMENSIONS                                             */
/*                                                                                                */
/**************************************************************************************************/

INT TabArea_Width (void)
{
   return 56;
} /* TabArea_Width */


/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

// The "Info View" merely acts as a container for the various info sub areas. Therefore
// the "Info View doesn't draw anything by itself.

TabAreaView::TabAreaView (CViewOwner *parent, CRect frame)
 : BackView(parent, frame, false)
{
   selected = infoMode_MovesOnly;
   SetFontSize(9);
   SetFontMode(fontMode_Or);
} /* TabAreaView::TabAreaView */


void TabAreaView::CalcFrames (INT i, CRect *rTab, CRect *rIcon, CRect *rText)
{
   INT tabHeight = (bounds.Height() - 2*8)/5;

   CRect r(0,0,45,tabHeight);
   r.Offset(0, 8 + (4-i)*tabHeight);

   CRect ri(0,0,32,32);
   ri.Offset(r.left + (r.Width() - 32)/2, r.top + (r.Height() - 32)/2 - 6);

   CRect rt(0,0,r.Width() - 1, 24);
   rt.Offset(r.left, ri.bottom + 2);

   if (rTab)  *rTab  = r;
   if (rIcon) *rIcon = ri;
   if (rText) *rText = rt;
} /* TabAreaView::CalcFrames */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

void TabAreaView::HandleUpdate (CRect updateRect)
{
   CRect r = bounds;

   // First draw background:
   if (RunningOSX())
   {  Draw3DFrame(r, &color_White, &color_BtGray); r.Inset(1, 1);
      DrawStripeRect(r);
   }
   else
   {  Draw3DFrame(r, &color_White, &color_Gray); r.Inset(1, 1);
      DrawRectFill(r, &color_LtGray);
   }

   // Then draw the tabs
   SetFontSize(9);
   SetFontStyle(fontStyle_Plain);
   DrawAllTabs();
} /* TabAreaView::HandleUpdate */


BOOL TabAreaView::HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)
{
   if (BackView::HandleMouseDown(pt, modifiers, doubleClick))
      return true;

   for (INT i = 0; i <= 4; i++)
   {
      CRect r;
      CalcFrames(i, &r, nil, nil);

      if (pt.InRect(r))
      {
         if (i == selected) return true;

	      BOOL isPushed = true;
         DrawTabIcon(i, isPushed);

         MOUSE_TRACK_RESULT trackResult;
         do
         {
            TrackMouse(&pt, &trackResult);
            if (trackResult != mouseTrack_Released && isPushed != pt.InRect(r))
            {  isPushed = ! isPushed;
               DrawTabIcon(i, isPushed);
            }
         } while (trackResult != mouseTrack_Released);

         DrawTabIcon(i, false);

         if (isPushed)
            SelectTab(i);

         return true;
      }
   }

   return false;
} /* TabAreaView::HandleMouseDown */


void TabAreaView::SelectTab (INT i)
{
   GameWindow *win = (GameWindow*)(Window());

   switch (i)
   {
      case infoMode_EditPos : 
         win->HandleMessage(game_PositionEditor);
         break;

      case infoMode_EditLib :
         if (! PosLib_Loaded())
            NoteDialog(Window(), "Library Editor", "No Position Library is currently loaded.", cdialogIcon_Warning);
         else
            win->HandleMessage(library_Editor);
         break;

      case infoMode_Annotate :
         win->HandleMessage(game_AnnotationEditor);
         break;

      case infoMode_Analysis :
      case infoMode_MovesOnly :
         if (win->posEditor)
            win->HandleMessage(posEditor_Done);
         else
         {
            if (win->libEditor)
               win->HandleMessage(library_Editor);
            else if (win->annEditor)
               win->HandleMessage(game_AnnotationEditor); 

            if ((i == infoMode_Analysis && ! win->infoAreaView->showAnalysis) || (i == infoMode_MovesOnly && win->infoAreaView->showAnalysis))
               win->HandleMessage(display_ShowAnalysis);
         }
         break;
   }
} /* TabAreaView::SelectTab */

/*------------------------------------------ Drawing ---------------------------------------------*/
// Note: The selected tab should always be drawn last:

void TabAreaView::DrawAllTabs (void)  // Call initially and when tab selection changed
{
   GameWindow *win = (GameWindow*)(Window());

   SetForeColor(&sigmaPrefs->darkColor);
   MovePenTo(bounds.left - 1, bounds.top + 1); DrawLine(0, bounds.Height() - 3);

   RGB_COLOR col = sigmaPrefs->mainColor;
   AdjustColorLightness(&col, -10);
   SetForeColor(&col);
   MovePenTo(bounds.left - 2, bounds.top + 1); DrawLine(0, bounds.Height() - 3);

   selected = infoMode_None;
   if (win->annEditor) selected = infoMode_Annotate;
   else if (win->libEditor) selected = infoMode_EditLib;
   else if (win->posEditor) selected = infoMode_EditPos;
   else if (win->infoAreaView->showAnalysis) selected = infoMode_Analysis;
   else selected = infoMode_MovesOnly;

// for (INT i = 0; i <= 4; i++)
   for (INT i = 4; i >= 0; i--)
      if (i != selected) DrawTab(i, false);

   if (selected >= 0)
      DrawTab(selected, true);
} /* TabAreaView::DrawAllTabs */


void TabAreaView::DrawTabIcon (INT i, BOOL isPushed)
{
   CRect ri;
   CalcFrames(i, nil, &ri, nil);
   DrawIcon(1330 + i, ri, (isPushed ? iconTrans_Selected : iconTrans_None));
} /*TabAreaView::DrawTabIcon */


static CHAR *tabText[5] =
{
   "Edit Position",
   "Edit Library",
   "Annotate",
   "Analysis",
   "Moves only"
};

void TabAreaView::DrawTab (INT i, BOOL front)
{
   CRect r, ri, rt;
   CalcFrames(i, &r, &ri, &rt);

   // Setup colors:
   RGB_COLOR lt1 = sigmaPrefs->lightColor;
   RGB_COLOR lt2;
   RGB_COLOR bg  = sigmaPrefs->mainColor;
   RGB_COLOR dk2;
   RGB_COLOR dk1 = sigmaPrefs->darkColor;

   if (i != selected)
      AdjustColorLightness(&lt1,-10),
      AdjustColorLightness(&bg, -10);
   lt2 = bg; AdjustColorLightness(&lt2, +10);
   dk2 = bg; AdjustColorLightness(&dk2, -10);

   // Draw main background rect:
   r.right -= 5;
   DrawRectFill(r, &bg);
   
   // Make rounded right side:
   SetForeColor(&bg);
   INT h = r.right; INT dv = r.Height() - 5;
   MovePenTo(h, r.top + 1); DrawLine(0, dv); h++;
   MovePenTo(h, r.top + 1); DrawLine(0, dv); h++; dv -= 2;
   MovePenTo(h, r.top + 2); DrawLine(0, dv); h++; dv -= 3;
   MovePenTo(h, r.top + 3); DrawLine(0, dv); h++; dv -= 1;
   SetForeColor(&dk2);
   MovePenTo(h, r.top + 4); DrawLine(0, dv); h++; dv -= 2;
   SetForeColor(&dk1);
   MovePenTo(h, r.top + 6); DrawLine(0, dv); h++;

   SetForeColor(&lt1); MovePenTo(r.left + 4, r.top);
   DrawLine(r.Width() - 5, 0);
   SetForeColor(&lt2); MovePenTo(r.left + 4, r.top + 1);
   DrawLine(r.Width() - 4, 0);
   SetForeColor(&dk2); MovePenTo(r.left + 4, r.bottom - 2);
   DrawLine(r.Width() - 5, 0); DrawLine(1,-1); DrawLine(1,0); DrawLine(2, -2); DrawLine(0, -1);
   SetForeColor(&dk1); MovePenTo(r.left + 4, r.bottom - 1);
   DrawLine(r.Width() - 5, 0); DrawLine(1,-1); DrawLine(1,0); DrawLine(3, -3); DrawLine(0, -1);

   SetForeColor(&bg);

   // Draw top curves
   if (i == selected)
   {
      MovePenTo(r.left - 2, r.top); DrawLine(0, -6);
      MovePenTo(r.left - 1, r.top); DrawLine(0, -4);
   }

   if (1) //i == 4 || i == selected)
   {
      SetForeColor(&lt1); MovePenTo(r.left, r.top - 4); DrawLine(0, 1); DrawLine(2, 2); DrawLine(1, 0);
      SetForeColor(&lt2); MovePenTo(r.left, r.top - 2); DrawLine(2, 2); DrawLine(1, 0);
      SetForeColor(&bg);  MovePenTo(r.left, r.top - 1); DrawLine(0, 0);
   }

   // Draw bottom curves
   if (i == selected)
   {
      MovePenTo(r.left - 2, r.bottom); DrawLine(0, 5);
      MovePenTo(r.left - 1, r.bottom); DrawLine(0, 3);
   }

   if (i == 0 || i == selected)
   {
      SetForeColor(&dk1); MovePenTo(r.left, r.bottom + 3); DrawLine(0, -1); DrawLine(2, -2); DrawLine(1,0);
      SetForeColor(&dk2); MovePenTo(r.left, r.bottom + 1); DrawLine(2, -2); DrawLine(1,0);
      SetForeColor(&bg);  MovePenTo(r.left, r.bottom); DrawLine(0, 0);
   }

   // "Attach" selected tab to info pane:
   if (i == selected)
   {  SetForeColor(&sigmaPrefs->mainColor);
      MovePenTo(r.left - 1, r.top); DrawLineTo(r.left - 1, r.bottom - 1);
      MovePenTo(r.left - 2, r.top); DrawLineTo(r.left - 2, r.bottom - 1);
   }

   // Draw icon:
   DrawIcon(1330 + i, ri, iconTrans_None);
   
   // Draw icon text
   SetForeColor(&color_Black);
   SetBackColor(&bg);
   DrawStr(tabText[4 - i], rt, textAlign_Center, true);

} /* TabAreaView::DrawTab */
