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
#include "CView.h"
#include "CBitmap.h"
#include "CMenu.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define control_TitleLength  256

enum CCONTROL_TYPE
{
   controlType_PushButton = 1,
   controlType_CheckBox,
   controlType_RadioButton,
   controlType_ScrollBar,
   controlType_ListBox,
   controlType_Slider,
   controlType_Edit,
   controlType_PopupMenu,
   controlType_ProgressBar,
   controlType_Editor,
   controlType_Tab,

   controlType_Text,
   controlType_GroupBox,
   controlType_Divider,
   controlType_Icon,
   controlType_Bitmap
};

enum CCONTROL_FONT
{
   controlFont_System = kControlFontBigSystemFont,
   controlFont_SmallSystem = kControlFontSmallSystemFont,
   controlFont_SmallSystemBold = kControlFontSmallBoldSystemFont,
   controlFont_Views = kControlFontViewSystemFont
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         TYPE DEFINITIONS                                       */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Generic Control Class -----------------------------------*/
// This is a virtual class. Actual control implementations must be derived from this class.

class CControl : public CView
{
public:
   CControl (CViewOwner *parent, CCONTROL_TYPE type, CHAR *title, CRect frame, BOOL acceptsFocus = false, BOOL visible = true, BOOL enabled = true);
   virtual ~CControl (void);

   void Enable (BOOL enabled);
   void SetFrame (CRect frame, BOOL update = false);

   virtual void HandleUpdate (CRect updateRect);
   virtual void HandleActivate (BOOL wasActivated);
   virtual void HandleFocus (BOOL gotFocus);
   virtual void HandleNullEvent (void);
   virtual void HandleMove (void);
   virtual void HandleResize (void);
   virtual void HandleVisChange (void);

   virtual void HandleUndo (void);
   virtual void HandleRedo (void);
   virtual void HandleCut (void);
   virtual void HandleCopy (void);
   virtual void HandlePaste (void);
   virtual void HandleClear (void);
   virtual void HandleClearAll (void);

   virtual void HandleFind (void);
   virtual void HandleFindAgain (void);
   virtual void HandleReplace (void);
   virtual void HandleReplaceFind (void);
   virtual void HandleReplaceAll (void);

   virtual void SetTitle (CHAR *s);
   virtual void GetTitle (CHAR *s);

   BOOL HasFocus (void);

   virtual void Track (Point pt, INT part);     // Low level tracking for Mac controls

   Rect MacRect (void);

   // Generic instance variables:
   CHAR          title[2000];
   CCONTROL_TYPE type;
   BOOL          acceptsFocus, wantsReturn;
   CControl      *prevCtl, *nextCtl;

   ControlHandle ch;                            // Low-level Mac control handle.
};

/*------------------------------ Pushbuttons, Checkboxes, Radiobuttons ---------------------------*/
// These are all active per default, an hence can accept user input.

class CPushButton : public CControl       // Based on standard Mac pushbuttons
{
public:
   CPushButton (CViewOwner *parent, CHAR *title, CRect frame, BOOL visible = true, BOOL enabled = true, BOOL useSysFont = true);
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);

   void Press (BOOL notifyParent = true);

   virtual void Track (Point pt, INT part);
};


class CCheckBox : public CControl         // Based on standard Mac checkboxes
{
public:
   CCheckBox (CViewOwner *parent, CHAR *title, BOOL checked, CRect frame, BOOL visible = true, BOOL enabled = true);
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);

   void Toggle (void);
   void Check (BOOL checked);
   BOOL Checked (void);

   virtual void Track (Point pt, INT part);
};


class CRadioButton : public CControl      // Based on standard Mac radiobuttons
{
public:
   CRadioButton (CViewOwner *parent, CHAR *title, INT groupID, CRect frame, BOOL visible = true, BOOL enabled = true);
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);

   void Select (void);
   BOOL Selected (void);

   virtual void Track (Point pt, INT part);

private:
   INT groupID;
};

/*----------------------------------------- Scrollbars -------------------------------------------*/
// These are active per default, an hence can accept user input.

class CScrollBar : public CControl       // Based on standard Mac scrollbars
{
public:
   CScrollBar (CViewOwner *parent, LONG min, LONG max, LONG val, LONG deltaPage, CRect frame, BOOL visible = true, BOOL enabled = true, BOOL isSlider = false);
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);

   virtual void Track (Point pt, INT part);

   void SetMin (LONG min);
   void SetMax (LONG max);
   void SetVal (LONG val, BOOL notifyParent = false);
   void SetIncrement (LONG deltaPage);

   LONG GetMin (void);
   LONG GetMax (void);
   LONG GetVal (void);
   LONG GetIncrement (void);

   void LineUp (void);
   void LineDown (void);
   void PageUp (void);
   void PageDown (void);

private:
   void RecalcSCvalues (void);

   LONG lmin, lmax, lval;   // The logical (LONG) scrollbar values
   LONG pageIncr;           // The logical number of "lines" visible at one time.

   LONG stepFactor;         // The step factor (= 1 for "normal" scrollbars, > 1 for "very long" scroll bars)
   INT  scmin;              // Physical scrollbar control minimum
   INT  scmax;              // Physical scrollbar control maximum
   INT  scval;              // Physical scrollbar control value
};

/*------------------------------------------ List Boxes -------------------------------------------*/

class CListBox : public CControl
{
public:
   CListBox
   (  CViewOwner *parent,
      CRect frame,
      INT  rows,
      INT  columns,
      BOOL horScroll,
      BOOL verScroll,
      INT  cellWidth,
      INT  cellHeight = 16,
      BOOL visible = true,
      BOOL enabled = true
   );
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);
   virtual void Track (Point pt, INT part);

   void Clear (INT rows);
   void SetCell (INT row, INT column, CHAR *text);
   INT  GetSelectedRow (void);  // Returns -1 if no row selected
   void SelectRow (INT row, BOOL selected = true);
   

private:
   ListHandle listHnd;
   INT selRow;
   INT rowCount;
   INT colCount;
};

/*---------------------------------------- Edit Controls ------------------------------------------*/
// These are single line edit controls (custom designed for maximum flexibility).

class CEditControl : public CControl
{
public:
   CEditControl
   (  CViewOwner *parent,
      CHAR *text,
      CRect frame,
      INT maxChars = control_TitleLength - 1,
      BOOL visible = true,
      BOOL enabled = true,
      BOOL password = false
   );
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);
   virtual void HandleFocus (BOOL gotFocus);
   virtual void HandleNullEvent (void);

   virtual void SetTitle (CHAR *s);
   virtual void GetTitle (CHAR *s);

   void SetText (CHAR *s);
   void GetText (CHAR *s);
   void GetTextISOLatin (CHAR *s);

   void SetLong (LONG n);
   BOOL GetLong (LONG *n);
   void SelectText (void);
   void SelectText (INT from, INT to);
   void Deselect (void);

   BOOL ValidateNumber (LONG min, LONG max, BOOL allowEmpty = false);
   BOOL IsEmpty (void);

   virtual void Track (Point pt, INT part);

private:
   INT  maxChars;
   BOOL password;
};

/*---------------------------------------- Popup Menus --------------------------------------------*/

class CPopupMenu : public CControl
{
public:
   CPopupMenu (CViewOwner *parent, CHAR *text, CMenu *theMenu, INT itemID, CRect frame, BOOL visible = true, BOOL enabled = true);
   virtual ~CPopupMenu (void);

   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);  //### Up/down arrows for "scrolling"!!!
   virtual void Track(Point pt, INT part);

   void Set (INT itemID);
   INT  Get (void);
   void EnableItem (INT itemID, BOOL enable);

   CMenu *menu;
};

/*---------------------------------------- Progress Bars ------------------------------------------*/

class CProgressBar : public CControl
{
public:
   CProgressBar (CViewOwner *parent, CRect frame, LONG min = 0, LONG max = 100, BOOL visible = true, BOOL indeterminate = false);
   virtual void HandleUpdate (CRect updateRect);

   void DrawBar (void);
   void Reset (void);
   void Set (LONG val);  // Must be in the interval [min..max]   

   LONG min, max, val;

   BOOL indeterm;
   INT  indetermPhase;
};

/*----------------------------------- Static Text, Group Boxes ------------------------------------*/
// These are all inactive. All user input is ignored.

class CTextControl : public CControl       // Based on standard Mac static text
{
public:
   CTextControl (CViewOwner *parent, CHAR *text, CRect frame, BOOL visible = true, CCONTROL_FONT font = controlFont_System);
   virtual void HandleUpdate (CRect updateRect);
   virtual void SetTitle (CHAR *s);
};


class CGroupBox : public CControl
{
public:
   CGroupBox (CViewOwner *parent, CHAR *text, CRect frame, BOOL visible = true);
   virtual void HandleUpdate (CRect updateRect);
};


class CDivider : public CControl
{
public:
   CDivider (CViewOwner *parent, CRect frame, BOOL visible = true);
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);
};

class CTabControl : public CControl
{
public:
   CTabControl (CViewOwner *parent, CRect frame, INT count, CHAR* TabStrArray[], BOOL visible = true);
   virtual void HandleUpdate (CRect updateRect);

   virtual void Track(Point pt, INT part);
};

/*---------------------------------------- Icons/Bitmaps ------------------------------------------*/
// These are all inactive. All user input is ignored per default###

class CIconControl : public CControl
{
public:
   CIconControl (CViewOwner *parent, INT iconID, CRect frame, BOOL visible = true);

   void Set (INT newIconID);

private:
   INT iconID;
};


class CBitmapControl : public CControl
{
public:
   CBitmapControl (CViewOwner *parent, CBitmap *bmp, CRect frame, BMP_MODE mode = bmpMode_Copy, BOOL visible = true);
   virtual void HandleUpdate (CRect updateRect);
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);

   void Set (CBitmap *newBmp, BMP_MODE mode = bmpMode_Copy);

private:
   CBitmap *bitmap;
   BMP_MODE bitmapMode;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES                                 */
/*                                                                                                */
/**************************************************************************************************/

extern INT controlHeight_PushButton;
extern INT controlWidth_PushButton;
extern INT controlHeight_CheckBox;
extern INT controlHeight_RadioButton;
extern INT controlWidth_ScrollBar;
extern INT controlHeight_Edit;
extern INT controlHeight_PopupMenu;
extern INT controlHeight_ProgressBar;
extern INT controlHeight_Text;
extern INT controlSize_Icon;

extern INT controlVDiff_PushButton;
extern INT controlVDiff_CheckBox;
extern INT controlVDiff_RadioButton;
extern INT controlVDiff_Text;
extern INT controlVDiff_Edit;


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

void CControl_Init (void);
