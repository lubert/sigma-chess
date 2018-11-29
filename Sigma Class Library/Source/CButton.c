/**************************************************************************************************/
/*                                                                                                */
/* Module  : CBUTTON.C                                                                            */
/* Purpose : Implements text/bitmap buttons derived from the view class.                          */
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

#include "CButton.h"
#include "CApplication.h"
#include "General.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                          CONSTRUCTOR                                           */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------------- Icon Buttons ------------------------------------------*/
// This is the new, preferred button type in Sigma Chess 6 (mainly for toolbar buttons).
// The frame is the icon boundary rectangle. The optional subtitle is written below the icon
// (outside the view!?). 

CButton::CButton
(
   CViewOwner *parent,
   CRect   frame,            // Frame/location in parent view.
   INT     command,          // Command id sent to window's handler.
   INT     subCommand,       // Sub command id sent to window's handler.
   BOOL    isVisible,        // Initially visible?
   BOOL    isEnabled,        // Initially enabled?
   INT     theIconID,        // Icon ID for button
   CHAR    *title,           // Optional icon button (sub)title
   CHAR    *theHelpText      // Optional help text (if ctrl clicking)
) : CView (parent, frame)
{
   isTextButton = false;
   isIconButton = true;
   CView::Show(isVisible);
   CView::Enable(isEnabled);
   pressed = false;

   CopyStr(title, text);
   iconID           = theIconID;
   bitmapEnabled    = nil;
   bitmapDisabled   = nil;
   buttonCommand    = command;
   buttonSubCommand = subCommand;
   popupMenu        = nil;
   helpText         = theHelpText;
   isOnOff          = false;
   SetFontSize(9);
   SetFontMode(fontMode_Or);
} /* CButton::CButton */


CButton::CButton
(
   CViewOwner *parent,
   CRect   frame,            // Frame/location in parent view.
   INT     command,          // Command id sent to window's handler.
   INT     subCommand,       // Unused, but need to avoid bad function overloading
   CMenu   *thePopupMenu,    // Popup menu
   BOOL    isVisible,        // Initially visible?
   BOOL    isEnabled,        // Initially enabled?
   INT     theIconID,        // Icon ID for button
   CHAR    *title,           // Optional icon button (sub)title
   CHAR    *theHelpText      // Optional help text (if ctrl clicking)
) : CView (parent, frame)
{
   isTextButton = false;
   isIconButton = true;
   CView::Show(isVisible);
   CView::Enable(isEnabled);
   pressed = false;

   CopyStr(title, text);
   iconID           = theIconID;
   bitmapEnabled    = nil;
   bitmapDisabled   = nil;
   buttonCommand    = command;
   buttonSubCommand = nullCommand;
   popupMenu        = thePopupMenu;
   helpText         = theHelpText;
   isOnOff          = false;
   SetFontSize(9);
   SetFontMode(fontMode_Or);
} /* CButton::CButton */

/*-------------------------------------- Plain Text Buttons --------------------------------------*/

CButton::CButton
(
   CViewOwner *parent, 
   CRect   frame,            // Frame/location in parent view.
   INT     command,          // Command id sent to window's handler.
   INT     subCommand,       // Sub command id sent to window's handler.
   BOOL    isVisible,        // Initially visible?
   BOOL    isEnabled,        // Initially enabled?
   CHAR    *faceText,        // Text button
   CHAR    *theHelpText      // Optional help text (if ctrl clicking)
) : CView (parent, frame)
{
   isTextButton = true;
   isIconButton = false;
   CView::Show(isVisible);
   CView::Enable(isEnabled);
   pressed = false;

   CopyStr(faceText, text);
   bitmapEnabled    = nil;
   bitmapDisabled   = nil;
   buttonCommand    = command;
   buttonSubCommand = subCommand;
   popupMenu        = nil;
   helpText         = theHelpText;
   isOnOff          = false;
} /* CButton::CButton */

/*----------------------------------- Old Style Graphics Buttons ---------------------------------*/

CButton::CButton
(
   CViewOwner *parent, 
   CRect     frame,            // Frame/location in parent view.
   INT       command,          // Command id sent to window's handler.
   INT       subCommand,       // Sub command id sent to window's handler.
   BOOL      isVisible,        // Initially visible?
   BOOL      isEnabled,        // Initially enabled?
   CBitmap   *faceEnabled,
   CBitmap   *faceDisabled,
   CRect     *srcRectEnabled,
   CRect     *srcRectDisabled,
   CHAR      *theHelpText,     // Optional help text (if ctrl clicking)
   RGB_COLOR *theTransColor    // Optional transparency color.
) : CView(parent,frame)
{
   isTextButton = false;
   isIconButton = false;
   CView::Show(isVisible);
   CView::Enable(isEnabled);
   pressed = false;

   text[0] = nil;
   bitmapEnabled    = faceEnabled;
   bitmapDisabled   = faceDisabled;
   rEnabled         = (srcRectEnabled  ? *srcRectEnabled  : faceEnabled->bounds);
   rDisabled        = (srcRectDisabled ? *srcRectDisabled : faceDisabled->bounds);
   buttonCommand    = command;
   buttonSubCommand = subCommand;
   popupMenu        = nil;
   helpText         = theHelpText;
   isOnOff          = false;

   if (theTransColor == nil)
      transColor = color_BtGray;
   else
      transColor = *theTransColor;
   SetBackColor(&color_BtGray);
} /* CButton::CButton */


/**************************************************************************************************/
/*                                                                                                */
/*                                           DESTRUCTOR                                           */
/*                                                                                                */
/**************************************************************************************************/

CButton::~CButton (void)
{
   if (popupMenu)
   {  delete popupMenu;
      popupMenu = nil;
   }
} /* CButton::~CButton */


/**************************************************************************************************/
/*                                                                                                */
/*                                            DRAWING                                             */
/*                                                                                                */
/**************************************************************************************************/

void CButton::HandleUpdate (CRect updateRect)
{
   DrawBody();
   DrawFace();
} /* CButton::HandleUpdate */


void CButton::DrawBody (void)
{
   if (! Visible()) return;

   if (isIconButton)
   {
      CRect r = bounds;
      if (text[0])
      {  r.Inset(-Max(0, (StrWidth(text) - bounds.Width())/2), 0);
         r.bottom += 15;
      }
      if (! UsingMetalTheme())
	      DrawStripeRect(r, (frame.top + 3) % 4);
   }
   else if (RunningOSX())
   {
      SetForeColor(&color_Black);
      DrawPict((pressed ? 101 : 100), bounds);
      SetForeColor(&color_MdGray);
//    SetForeColor(&color_DkGray);
      DrawRectFrame(bounds);
   }
   else
   {
	   CRect r = bounds;
	   SetForeColor(&color_Black);
	   DrawRectFrame(r);
	   r.Inset(1, 1);
	   Draw3DFrame(r, (pressed ? &color_MdGray : &color_White), (pressed ? &color_White : &color_MdGray));
	   r.Inset(1, 1);
	   Draw3DFrame(r, (pressed ? &color_Gray : &color_LtGray), (pressed ? &color_LtGray : &color_Gray));
	   r.Inset(1, 1);
	   DrawRectFill(r, &color_BtGray);
   }
} /* CButton::DrawBody */


void CButton::DrawFace (void)
{
   if (! Visible()) return;

   BOOL reallyEnabled = (Enabled() && Active());

   if (isIconButton)
   {
      ICON_TRANS trans = (reallyEnabled ? (pressed ? (isOnOff ? iconTrans_Disabled : iconTrans_Selected) : iconTrans_None) : iconTrans_Disabled);
      DrawIcon(iconID, bounds, trans);

      if (text[0])
      {
         SetFontSize(9);
         SetFontFace(font_Geneva);

         INT   w = StrWidth(text) - bounds.Width();
         MovePenTo(bounds.left - w/2, bounds.bottom + 12);
         SetForeColor(reallyEnabled ? &color_Black : &color_MdGray);
         SetFontStyle(fontStyle_Plain);
//         SetThemeFont(kThemeSmallSystemFont);
         DrawStr(text);
      }
   }
   else if (isTextButton)
   {
      CRect r = bounds;
      r.Inset(2, 2);
      if (pressed && ! RunningOSX()) r.Offset(1, 1);
      SetForeColor(reallyEnabled ? &color_Black : &color_MdGray);
      SetBackColor(&color_BtGray);
      if (RunningOSX()) SetFontMode(fontMode_Or);
      SetFontSize(12);
      SetFontFace(font_System);
      r.Inset(1, 1);
      DrawStr(text, r, textAlign_Center, false, ! RunningOSX());
      SetFontMode(fontMode_Copy);
   }
   else
   {
      CBitmap *bmp = (reallyEnabled ? bitmapEnabled : bitmapDisabled);
      CRect dst = rEnabled;
      dst.Normalize();
      dst.Offset((bounds.Width() - dst.Width())/2, (bounds.Height() - dst.Height())/2);
      if (pressed && ! RunningOSX()) dst.Offset(1, 1);
      SetBackColor(&transColor);
      DrawBitmap(bmp, rEnabled, dst, bmpMode_Trans);
      SetBackColor(&color_BtGray);
   }
} /* CButton::DrawFace */


void CButton::HandleActivate (BOOL wasActivated)
{
   Redraw();
} /* CButton::HandleActivate */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

BOOL CButton::HandleMouseDown (CPoint thePt, INT modifiers, BOOL doubleClick)
{
   if (! Visible()) return false;

   if (modifiers & modifier_Command)
      ShowHelpTip(helpText);
   else if (! Enabled())
      return false;
   else if (popupMenu)
   {
      INT popupItem;
      if (popupMenu->Popup(&popupItem))
      {  if (! theApp->HandleMessage(buttonCommand, popupItem))
            Window()->HandleMessage(buttonCommand, popupItem);
      }
   }
   else if (buttonCommand)
   {
      BOOL wasPressed = pressed;

      Press(! wasPressed);

      MOUSE_TRACK_RESULT trackResult;
      do
      {
         CPoint pt;
         BOOL inButton = TrackMouse(&pt, &trackResult);
         if (trackResult != mouseTrack_Released)
         {  if (inButton && pressed == wasPressed) Press(! wasPressed);
            if (! inButton && pressed != wasPressed) Press(wasPressed);
         }
      } while (trackResult != mouseTrack_Released);

      Sleep(3);
      if (pressed != wasPressed)
      {  if (! wasPressed) Press(false);
         if (! theApp->HandleMessage(buttonCommand, buttonSubCommand))
            Window()->HandleMessage(buttonCommand, buttonSubCommand);
      }
   }

   return true;
} /* CButton::HandleMouseDown */


void CButton::Press (BOOL isPressed)
{
   if (pressed == isPressed) return;
   pressed = isPressed;
   if (isIconButton)
      DrawFace();
   else
      Redraw();
} /* CButton::Press */


void CButton::Enable (BOOL wasEnabled, BOOL redraw)
{
   if (Enabled() == wasEnabled) return;

   CView::Enable(wasEnabled);
   if (redraw && Visible()) Redraw();
} /* CButton::Enable */


void CButton::Show (BOOL _show, BOOL redraw)
{
   BOOL wasVisible = Visible();
   CView::Show(_show);
   if (redraw && Visible() && ! wasVisible) Redraw();
} /* CButton::Show */


/**************************************************************************************************/
/*                                                                                                */
/*                                          SETTING STATE                                         */
/*                                                                                                */
/**************************************************************************************************/

void CButton::SetHelpText (CHAR *s)
{
   helpText = s;
} /* CButton::SetHelpText */


void CButton::SetIcon (INT newIconID, BOOL redraw)
{
   if (! isIconButton) return;
   iconID = newIconID;
   if (redraw && Visible()) Redraw();
} /* CButton::SetIcon */


void CButton::SetOnOff (void)
{
   isOnOff = true;
} /* CButton::SetOnOff */
