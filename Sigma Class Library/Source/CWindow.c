/**************************************************************************************************/
/*                                                                                                */
/* Module  : CWINDOW.C                                                                            */
/* Purpose : Implements the generic windows class, which serves as the base class for all other   */
/*           windows.                                                                             */
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

#include "CWindow.h"
#include "CApplication.h"
#include "CUtility.h"
#include "CEditor.h"

//#define _withThemeFont 1


/**************************************************************************************************/
/*                                                                                                */
/*                                          CONSTRUCTOR                                           */
/*                                                                                                */
/**************************************************************************************************/

CWindow::CWindow (CWindow *parent, CHAR *title, CRect r, CWINDOW_TYPE theWinType, BOOL isSizeable, CRect theResizeLimit)
  : CViewOwner(viewOwner_Window)
{
   // Set parent:
   winParent = parent;

   // Set window/dialog type:
   winType = theWinType;
   if (winType == cwindowType_Sheet)
      if (! RunningOSX())
         winType = cwindowType_ModalDialog;
      else if (parent && ! parent->IsFront())
         parent->SetFront();

   // Reset misc
   winRef = nil;
   dlgRef = nil;
   sheetChild = nil;

   hasFile = false;
   modalRunning = false;
   firstCtl = lastCtl = focusCtl = nil;

   // Add to application window list:
   theApp->winList.Append(this);

   // Normalize bounds rectangle (top left corner at (0,0)):
   bounds = r;
   bounds.Normalize();
   sizeable = isSizeable;
   resizeLimit = theResizeLimit;

   // Build frame and title (in internal Carbon format):
   Rect mr;
   ::SetRect(&mr, r.left, r.top, r.right, r.bottom);
   Str255 ptitle;
   ::C2P_Str(title, ptitle);

   // Create the dialog or window:

   if (IsDialog())    // DIALOG (initially invisible)
   {
      SInt16 procID = (winType == cwindowType_Sheet ? kWindowSheetProc : kWindowMovableModalDialogProc);
      UInt32 flags  = kDialogFlagsUseThemeBackground | kDialogFlagsHandleMovableModal | kDialogFlagsUseThemeControls;
	   dlgRef = ::NewFeaturesDialog(nil, &mr, ptitle, false, procID, (WindowRef)-1L, false, (SInt32)this, nil, flags);
	   winRef = ::GetDialogWindow(dlgRef);
	   ::SetWRefCon(winRef, (SInt32)this);
	   cgrafPtr = ::GetWindowPort(winRef);
   }
   else               // WINDOW (initially invisible)
   {
	   INT attr = 0;
	   if (winType == cwindowType_Document)
	   {  attr = (kWindowCloseBoxAttribute | kWindowFullZoomAttribute | kWindowCollapseBoxAttribute);
	   	if (UsingMetalTheme()) attr |= (1L << 8);
	      if (sizeable && (UsingMetalTheme() || ! RunningOSX())) attr |= (kWindowResizableAttribute | kWindowLiveResizeAttribute);
	   }
	   ::CreateNewWindow(winType, attr, &mr, &winRef);
	   ::SetWTitle(winRef, ptitle);
	   ::SetWRefCon(winRef, (SInt32)this);
	   cgrafPtr = ::GetWindowPort(winRef);
   }

   ::CreateRootControl(winRef, &rootControl);

   // Reset window font/face/style/size/mode
   CGrafPtr oldPort = nil;
   GDHandle oldDevice = nil;
   ::GetGWorld(&oldPort, &oldDevice);
   ::SetGWorld(cgrafPtr, nil);
   ::PenNormal();
   ::TextMode(srcCopy);
#ifdef _withThemeFont
   ::UseThemeFont(kThemeLabelFont, smSystemScript);
#else
   ::TextFont(font_Geneva);
   ::TextFace(0);
   ::TextSize(10);
#endif
   ::RGBForeColor(&color_Black);
   ::RGBBackColor(&color_White);
   ::SetGWorld(oldPort, oldDevice);   

   ::ShowCursor();

   // Finally notify application that new window has been created
   theApp->HandleWindowCreated(this);
} /* CWindow::CWindow */


/**************************************************************************************************/
/*                                                                                                */
/*                                          DESTRUCTOR                                            */
/*                                                                                                */
/**************************************************************************************************/

CWindow::~CWindow (void)
{
   // Then destroy all views recursively:
   while (vFirstChild) delete vFirstChild;

   // Remove from application window list:
   theApp->winList.Remove(this);
   if (theApp->winList.Count() == 0)
      theApp->HandleMenuAdjust();

   // Finally destroy the underlying Mac window (and process pending update events):
   if (RunningOSX() && winType == cwindowType_Sheet && winParent)
      ::HideSheetWindow(winRef);

   if (dlgRef) ::DisposeDialog(dlgRef);
   else if (winRef) ::DisposeWindow(winRef);

   theApp->HandleWindowDestroyed(this);
} /* CWindow::~CWindow*/


/**************************************************************************************************/
/*                                                                                                */
/*                                         MISC METHODS                                           */
/*                                                                                                */
/**************************************************************************************************/

void CWindow::Show (BOOL visible)
{
   if (! winRef) return;

   ::ShowHide(winRef, visible);
} /* CWindow::Show */


void CWindow::Collapse (void)
{
   if (! winRef) return;

   ::CollapseWindow(winRef, true);
} /* CWindow::Collapse */


void CWindow::SetTitle (CHAR *title)
{
   if (! winRef) return;

   Str255 ptitle;
   ::C2P_Str(title, ptitle);
   ::SetWTitle(winRef, ptitle);
} /* CWindow::SetTitle */


void CWindow::GetTitle (CHAR *title)
{
   if (! winRef) return;

   Str255 ptitle;
   ::GetWTitle(winRef, ptitle);
   ::P2C_Str(ptitle, title);
} /* CWindow::GetTitle */


CRect CWindow::Frame (void)
{
   Rect gr;
   ::GetWindowBounds(winRef, kWindowGlobalPortRgn, &gr);
   return CRect(gr.left, gr.top, gr.right, gr.bottom);
} /* CWindow::Frame */


CRect CWindow::Bounds (void)
{
   return bounds;
} /* CWindow::Bounds */


void CWindow::Resize (INT newWidth, INT newHeight)
{
   bounds.right = bounds.left + newWidth;
   bounds.bottom = bounds.top + newHeight;
   ::SizeWindow(winRef, newWidth, newHeight, true);
   theApp->ProcessSysEvents();
} /* CWindow::Resize */


void CWindow::Move (INT left, INT top, BOOL toFront)
{
   ::MacMoveWindow(winRef, left, top, toFront);
} /* CWindow::Move */


void CWindow::CentralizeRect (CRect *r)
{
   INT h = Max(20, (Frame().Width()  - r->Width())/2);
   INT v = Max(20, (Frame().Height() - r->Height())/2);
   r->Normalize();
   r->Offset(Frame().left + h, Frame().top + v);
} /* CWindow::CentralizeRect */


void CWindow::FlushPortBuffer (void)
{
   if (! RunningOSX()) return;
   ::QDFlushPortBuffer(cgrafPtr, nil);
} /* CWindow::FlushPortBuffer */


void CWindow::Redraw (void)
{
   DispatchUpdate(bounds);
} /* CWindow::Redraw */


void CWindow::SetFront (void)
{
   ::SelectWindow(winRef);
} /* CWindow::SetFront */


BOOL CWindow::IsFront (void)
{
   return (winRef == ::FrontWindow());
} /* CWindow::IsFront */


BOOL CWindow::IsDialog (void)
{
   return (winType == cwindowType_ModalDialog || winType == cwindowType_ModelessDialog || winType == cwindowType_Sheet);
} /* CWindow::IsDialog */


BOOL CWindow::IsModalDialog (void)
{
   return (winType == cwindowType_ModalDialog || winType == cwindowType_Sheet);
} /* CWindow::IsModalDialog */


void CWindow::SetModified (BOOL modified)
{
   ::SetWindowModified(winRef, modified);
} /* CWindow::SetModified */

 
BOOL CWindow::IsModified (void)
{
   return ::IsWindowModified(winRef);
} /* CWindow::IsModified */


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONTROL HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------- Add/Remove/Get Controls -------------------------------------*/

void CWindow::AddControl (CControl *ctl)
{
   // Next add to window's control list:
   if (! firstCtl)
      firstCtl = lastCtl = ctl->nextCtl = ctl->prevCtl = ctl;
   else
   {  ctl->nextCtl = firstCtl;
      ctl->prevCtl = lastCtl;
      lastCtl->nextCtl = firstCtl->prevCtl = ctl;
      lastCtl = ctl;
   }

   // Finally update focus control:
   if (! focusCtl) focusCtl = ctl;
} /* CWindow::AddControl */


void CWindow::RemoveControl (CControl *ctl)
{
   if (! ctl->prevCtl && ! ctl->nextCtl) return;  // Skip if already removed

   // First remove from window's control list:
   if (firstCtl == lastCtl)
      firstCtl = lastCtl = focusCtl = nil;
   else
   {  ctl->prevCtl->nextCtl = ctl->nextCtl;
      ctl->nextCtl->prevCtl = ctl->prevCtl;
      if (ctl == firstCtl) firstCtl = ctl->nextCtl;
      if (ctl == lastCtl)  lastCtl  = ctl->prevCtl;
      if (ctl == focusCtl) focusCtl = ctl->nextCtl;
   }

   ctl->prevCtl = ctl->nextCtl = nil;
} /* CWindow::RemoveControl */

/*--------------------------------------- Focus Handling -----------------------------------------*/

void CWindow::PrevControl (void)
{
   ::ReverseKeyboardFocus(winRef);
   if (! focusCtl) return; //### Simply select first focusable control instead.
} /* CWindow::PrevControl */


void CWindow::NextControl (void)
{
   ::AdvanceKeyboardFocus(winRef);
   if (! focusCtl) return; //### Simply select first focusable control instead.
} /* CWindow::NextControl */


void CWindow::CurrControl (CControl *ctl)
{
   if (! ctl || ! IsDialog() || ! ctl->acceptsFocus || ! ctl->Enabled() || ! ctl->Visible()) return;

   ControlRef focusCh = NULL;
   ::GetKeyboardFocus(winRef, &focusCh);
   if (ctl->ch && ctl->ch != focusCh) ::SetKeyboardFocus(winRef, ctl->ch, kControlFocusNextPart);
} /* CWindow::CurrControl */


/**************************************************************************************************/
/*                                                                                                */
/*                                 CLIPBOARD/HISTORY HANDLING                                     */
/*                                                                                                */
/**************************************************************************************************/

void CWindow::Cut (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleCut();
} /* CWindow::Cut */


void CWindow::Copy (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleCopy();
} /* CWindow::Copy */


void CWindow::Paste (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandlePaste();
} /* CWindow::Paste */


void CWindow::Clear (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleClear();
} /* CWindow::Clear */


void CWindow::ClearAll (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleClearAll();
} /* CWindow::ClearAll */


void CWindow::Undo (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleUndo();
} /* CWindow::Undo */


void CWindow::Redo (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleRedo();
} /* CWindow::Redo */


void CWindow::Find (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleFind();
} /* CWindow::Find */


void CWindow::FindAgain (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleFindAgain();
} /* CWindow::FindAgain */


void CWindow::Replace (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleReplace();
} /* CWindow::Replace */


void CWindow::ReplaceFind (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleReplaceFind();
} /* CWindow::ReplaceFind */


void CWindow::ReplaceAll (void)
{
   if (focusCtl && focusCtl->Enabled()) focusCtl->HandleReplaceAll();
} /* CWindow::ReplaceAll */


/**************************************************************************************************/
/*                                                                                                */
/*                                         WINDOW EVENTS                                          */
/*                                                                                                */
/**************************************************************************************************/

// The event routines are normally called by the CApplication in response to events. Therefore
// we should rarely need to call these routines ourselves.

/*-------------------------------------- Non-virtual Events --------------------------------------*/

void CWindow::DispatchMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)    // Should not be overridden.
// The point pt is in the local window coordinates:
{
   forAllChildViews(subView)
      if (subView->Visible() && pt.InRect(subView->frame))
      {
         CPoint lpt = pt;
         lpt.Offset(-subView->frame.left + subView->bounds.left, -subView->frame.top + subView->bounds.top);
         subView->DispatchMouseDown(lpt, modifiers, doubleClick);
      }
} /* CWindow::DispatchMouseDown */


void CWindow::DispatchUpdate (CRect updateRect)             // Should not be overridden.
{
   CRect sect;

   // Dispatch update to affected views:
   forAllChildViews(subView)
      if (sect.Intersect(&updateRect, &(subView->frame)))
      {
         sect.Offset(-subView->frame.left, -subView->frame.top);  // Transform to view coordinates.
         subView->DispatchUpdate(sect);
      }
} /* CWindow::DispatchUpdate */


void CWindow::DispatchActivate (BOOL activated)     // Dispatch activation info to all views:
{
   HandleActivate(activated);

   forAllChildViews(subView)
      subView->DispatchActivate(activated);
} /* CWindow::DispatchActivate */


BOOL CWindow::IsActive (void)
{
   return (! theApp->suspended && IsFront());
} /* CWindow::IsActive */


void CWindow::DrawGrow (void)
{
/*
   if (! RunningOSX()) return;

   Rect portRect, mr;
   ::GetPortBounds(cgrafPtr, &portRect);
   mr = portRect;
   mr.left = mr.right - 15;
   mr.top = mr.bottom - 15;
   
   CGrafPtr oldPort = nil;
   GDHandle oldDevice = nil;
      ::GetGWorld(&oldPort, &oldDevice);
      ::SetGWorld(cgrafPtr, nil);
      ::DrawGrowIcon(winRef);
   ::ClipRect(&portRect);
   ::SetGWorld(oldPort, oldDevice);
*/
} /* CWindow::DrawGrow */

/*---------------------------------------- Virtual Events ----------------------------------------*/
// These events should normally be overridden.

void CWindow::HandleMessage (LONG msg, LONG submsg, PTR data)  // Should be overridden.
{
} /* CWindow::HandleMessage */


void CWindow::HandleActivate (BOOL activated)  // Should be overridden to handle menu-enabling if activated
{
   if (IsActive()) HandleMenuAdjust();
} /* CWindow::HandleActivate */


void CWindow::HandleMenuAdjust (void)       // The default method simply enables and redraws the menubar...
{
   theApp->EnableQuitCmd(true);
   theApp->EnablePrefsCmd(true);
   theApp->EnableAboutCmd(true);
   theApp->EnableMenuBar(true);
} /* CWindow::HandleMenuAdjust */


void CWindow::HandleResize (INT newWidth, INT newHeight)
{
   Resize(newWidth, newHeight);
} /* CWindow::HandleResize */


void CWindow::HandleZoom (void)        // The default method does nothing...
{
} /* CWindow::HandleZoom */


void CWindow::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   ControlRef ch;
   ::GetKeyboardFocus(winRef, &ch);
   if (ch)
      ((CControl*)(GetControlReference(ch)))->HandleKeyDown(c, key, modifiers);
} /* CWindow::HandleKeyDown */


void CWindow::HandleKeyUp (CHAR c, INT key, INT modifiers)
{
} /* CWindow::HandleKeyUp */


BOOL CWindow::HandleCloseRequest (void)
{
   return true;              // Default behaviour is to agree to close...
} /* CWindow::HandleCloseRequest */


BOOL CWindow::HandleQuitRequest (void)
{
   return true;              // Default behaviour is to agree to quit...
} /* CWindow::HandleQuitRequest */


void CWindow::HandleNullEvent (void)  // Default idle sends null event to focus control (if any)
{
   if (focusCtl && focusCtl->Enabled())
      focusCtl->HandleNullEvent();
} /* CWindow::HandleNullEvent */


void CWindow::HandlePushButton (CPushButton *ctrl)
{
} /* CWindow::HandlePushButton */


void CWindow::HandleScrollBar (CScrollBar *ctrl, BOOL tracking)
{
} /* CWindow::HandleScrollBar */


void CWindow::HandleCheckBox (CCheckBox *ctrl)
{
   ctrl->Toggle();       // Default behaviour is to toggle checkbox
} /* CWindow::HandleCheckBox */


void CWindow::HandleRadioButton (CRadioButton *ctrl)
{
   ctrl->Select();       // Default behaviour is to select radiobutton
} /* CWindow::HandleRadioButton */


void CWindow::HandlePopupMenu (CPopupMenu *ctrl, INT itemID)
{
} /* CWindow::HandlePopupMenu */


void CWindow::HandleEditControl (CEditControl *ctrl, BOOL textChanged, BOOL selChanged)
{
} /* CWindow::HandleEditControl */


void CWindow::HandleEditor (CEditor *ctrl, BOOL textChanged, BOOL selChanged, BOOL srcRplChanged)
{
} /* CWindow::HandleEditor */


void CWindow::HandleListBox (CListBox *ctrl, INT row, INT column, BOOL dblClick)
{
} /* CWindow::HandleListBox */
