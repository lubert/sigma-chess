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

#pragma once

#include "General.h"
#include "CViewOwner.h"
#include "CView.h"
#include "CControl.h"
#include "CEditor.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define cwindow_MaxTitleLen 31

enum CWINDOW_TYPE
{
   cwindowType_Document       = kDocumentWindowClass, //zoomDocProc,
   cwindowType_Plain          = kPlainWindowClass, //plainDBox,
   cwindowType_ModalDialog    = kMovableModalWindowClass, //movableDBoxProc,
   cwindowType_ModelessDialog = noGrowDocProc,
   cwindowType_Sheet          = kSheetWindowClass
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class CWindow : public CViewOwner
{
public:
   CWindow (CWindow *parent, CHAR *title, CRect r, CWINDOW_TYPE winType = cwindowType_Document, BOOL sizeable = true, CRect resizeLimit = CRect(100,100,maxint,maxint));
   virtual ~CWindow (void);

   void Show (BOOL visible);
   void Collapse (void);
   void SetTitle (CHAR *title);
   void GetTitle (CHAR *title);
   CRect Frame (void);
   CRect Bounds (void);
   void Resize (INT newWidth, INT newHeight);
   void Move (INT left, INT top, BOOL toFront);
   void SetFront (void);
   BOOL IsFront (void);
   BOOL IsActive (void);
   BOOL IsDialog (void);
   BOOL IsModalDialog (void);
   void CentralizeRect (CRect *r);    // Centralize "r" with respect to window's frame.
   void FlushPortBuffer (void);
   void Redraw (void);

   void SetModified (BOOL modified);
   BOOL IsModified (void);

   void Undo (void);
   void Redo (void);

   void Cut (void);
   void Copy (void);
   void Paste (void);
   void Clear (void);
   void ClearAll (void);

   void Find (void);
   void FindAgain (void);
   void Replace (void);
   void ReplaceFind (void);
   void ReplaceAll (void);

   // Events (should normally not be overridden):
   void DispatchActivate (BOOL activated);
   void DispatchUpdate (CRect r);
   void DispatchMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);
   void DrawGrow (void);

   // Events (should be overridden):
   virtual void HandleKeyDown (CHAR c, INT key, INT modifiers); // If dialog -> ctrl with focus
   virtual void HandleKeyUp (CHAR c, INT key, INT modifiers);
   virtual void HandleMessage (LONG msg, LONG submsg = 0, PTR data = nil);
   virtual void HandleActivate (BOOL activated);
   virtual void HandleMenuAdjust (void);
   virtual void HandleResize (INT newWidth, INT newHeight);
   virtual void HandleZoom (void);
   virtual BOOL HandleCloseRequest (void);
   virtual BOOL HandleQuitRequest (void);

   virtual void HandlePushButton  (CPushButton *ctrl);
   virtual void HandleScrollBar   (CScrollBar *ctrl, BOOL tracking);
   virtual void HandleCheckBox    (CCheckBox *ctrl);
   virtual void HandleRadioButton (CRadioButton *ctrl);
   virtual void HandlePopupMenu   (CPopupMenu *ctrl, INT itemNo);
   virtual void HandleEditControl (CEditControl *ctrl, BOOL textChanged, BOOL selChanged);
   virtual void HandleEditor      (CEditor *ctrl, BOOL textChanged, BOOL selChanged, BOOL srcRplChanged);
   virtual void HandleListBox     (CListBox *ctrl, INT row, INT column, BOOL dblClick);

   virtual void HandleNullEvent (void);

   CGrafPtr GetCGrafPtr (void);

   void AddControl (CControl *ctl);
   void RemoveControl (CControl *ctl);
   void PrevControl (void);
   void NextControl (void);
   void CurrControl (CControl *ctl);

   CControl *firstCtl, *lastCtl, *focusCtl;

   CRect resizeLimit;

   CWindow *winParent;        // The parent window/dialog if this is a dialog
   CWindow *sheetChild;       // If this window has opened a sheet dialog (else NIL)
   CWINDOW_TYPE winType;
   BOOL hasFile;
   BOOL sizeable;
   BOOL modalRunning;

   WindowRef winRef;          // The actual underlying Macintosh color window.
   DialogRef dlgRef;          // Mac dialog ref (nil if NOT a dialog)
   ControlRef rootControl;
   CGrafPtr  cgrafPtr;

   CRect bounds;              // Local coordinate system (0,0).
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES                                 */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/
