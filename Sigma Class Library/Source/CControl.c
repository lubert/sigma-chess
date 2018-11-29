/**************************************************************************************************/
/*                                                                                                */
/* Module  : CControl.c */
/* Purpose : Implements various controls (buttons, scrollbars, radio buttons,
 * check boxes and     */
/*           popup menus). */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CControl.h"
#include "CApplication.h"
#include "CDialog.h"
#include "CEditor.h"
#include "CMemory.h"
#include "CWindow.h"

INT controlHeight_PushButton = 20;
INT controlWidth_PushButton = 70;
INT controlHeight_CheckBox = 15;
INT controlHeight_RadioButton = 15;
INT controlWidth_ScrollBar = 16;
INT controlHeight_Edit = 18;
INT controlHeight_PopupMenu = 16;
INT controlHeight_ProgressBar = 14;
INT controlHeight_Text = 15;
INT controlSize_Icon = 32;

INT controlVDiff_PushButton = 30;
INT controlVDiff_CheckBox = 18;
INT controlVDiff_RadioButton = 18;
INT controlVDiff_Text = 20;
INT controlVDiff_Edit = 25;

/**************************************************************************************************/
/*                                                                                                */
/*                                      GENERIC CONTROL CLASS */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Constructor/Destructor
 * -----------------------------------*/

CControl::CControl(CViewOwner *parent, CCONTROL_TYPE theType, CHAR *newTitle,
                   CRect frame, BOOL focus, BOOL _show, BOOL _enable)
    : CView(parent, frame) {
  type = theType;
  SetForeColor(&color_Black);
  SetBackColor(&color_Dialog);
  ch = nil;
  CopyStr(newTitle, title);
  prevCtl = nextCtl = nil;
  acceptsFocus =
      (theType == controlType_Edit || theType == controlType_ListBox);
  wantsReturn = false;
  Show(_show);
  Enable(_enable);

  if (Window() && Window()->IsDialog()) Window()->AddControl(this);
} /* CControl::CControl */

CControl::~CControl(void) {
  if (ch) {
    ::DisposeControl(ch);
    ch = nil;
  }
  if (Window() && Window()->IsDialog()) Window()->RemoveControl(this);
} /* CControl::~CControl */

/*------------------------------------- Generic Event Handling
 * -----------------------------------*/

void CControl::HandleUpdate(CRect updateRect) {
  if (!Visible()) return;

  if (!RunningOSX()) SetBackColor(&color_Dialog);
  if (ch) ::Draw1Control(ch);

  if (RunningOSX() && type == controlType_ScrollBar && !Window()->IsDialog()) {
    SetForeColor(&color_MdGray);
    MovePenTo(bounds.right - 1, bounds.top);
    DrawLineTo(bounds.right - 1, bounds.bottom - 1);
  }
} /* CControl::HandleUpdate */

void CControl::HandleActivate(BOOL wasActivated) {
  if (!Visible()) return;

  if (ch)
    if (false)  //! RunningOSX())
      ::HiliteControl(ch,
                      (wasActivated && Enabled() ? 0 : kControlInactivePart));
    else if (wasActivated && Enabled())
      ::ActivateControl(ch);
    else
      ::DeactivateControl(ch);

  if (ch && RunningOSX() && type == controlType_ScrollBar &&
      !Window()->IsDialog())  //### Hmmm
  {
    SetForeColor(&color_MdGray);
    MovePenTo(bounds.right - 1, bounds.top);
    DrawLineTo(bounds.right - 1, bounds.bottom - 1);
  }

  if (!ch) Redraw();
} /* CControl::HandleActivate */

void CControl::HandleMove(void) {
  if (ch) {
    ::HideControl(ch);
    ::MoveControl(ch, origin.h, origin.v);
    if (Visible()) ::ShowControl(ch);
  }
} /* CControl::HandleMove */

void CControl::HandleResize(void) {
  if (ch) {
    ::HideControl(ch);
    ::SizeControl(ch, bounds.Width(), bounds.Height());
    if (Visible()) ::ShowControl(ch);
  }
} /* CControl::HandleResize */

void CControl::HandleFocus(BOOL gotFocus) {} /* CControl::HandleFocus */

void CControl::HandleNullEvent(
    void)  // Should be overridden by e.g. edit controls
{}         /* CControl::HandleNullEvent */

void CControl::HandleCut(void) {} /* CControl::HandleCut */

void CControl::HandleCopy(void) {} /* CControl::HandleCopy */

void CControl::HandlePaste(void) {} /* CControl::HandlePaste */

void CControl::HandleClear(void) {} /* CControl::HandleClear */

void CControl::HandleClearAll(void) {} /* CControl::HandleClearAll */

void CControl::HandleUndo(void) {} /* CControl::HandleUndo */

void CControl::HandleRedo(void) {} /* CControl::HandleRedo */

void CControl::HandleFind(void) {} /* CControl::HandleFind */

void CControl::HandleFindAgain(void) {} /* CControl::HandleFindAgain */

void CControl::HandleReplace(void) {} /* CControl::HandleReplace */

void CControl::HandleReplaceFind(void) {} /* CControl::HandleReplaceFind */

void CControl::HandleReplaceAll(void) {} /* CControl::HandleReplaceAll */

void CControl::Track(
    Point pt, INT part)  // This virtual function MUST be overridden for Mac
{                        // controls accepting user input.
} /* CControl::Track*/

/*----------------------------------------- Properties
 * -------------------------------------------*/

void CControl::HandleVisChange(void) {
  CRect r = bounds;
  r.Inset(-2, -2);

  if (!Visible()) {
    if (HasFocus()) ::ClearKeyboardFocus(Window()->winRef);
    if (ch) ::HideControl(ch);
    if (Window()->IsDialog()) {
      visible = true;
      DrawRectErase(r);
      visible = false;
    }
    if (Window()->focusCtl == this) Window()->focusCtl = nil;
  } else {
    if (Window()->IsDialog()) DrawRectErase(r);
    if (ch)
      ::ShowControl(ch);
    else
      Redraw();
  }
} /* CControl::HandleVisChange */

void CControl::Enable(BOOL wasEnabled) {
  CView::Enable(wasEnabled);
  if (ch)
    ::HiliteControl(
        ch, (Enabled() && Visible() && Active() ? 0 : kControlInactivePart));
  else
    Redraw();
} /* CControl::Enable */

void CControl::SetFrame(CRect frame, BOOL update) {
  if (ch) ::HideControl(ch);

  if (Window()->IsDialog()) {
    CRect r = bounds;
    r.Inset(-5, -5);  //###hmmm
    DrawRectErase(r);
  }

  CView::SetFrame(frame, false);
  Redraw();

  // if (ch && Visible()) ::ShowControl(ch);
} /* CControl::SetFrame */

void CControl::SetTitle(CHAR *newTitle)  // Default handling of SetText
{
  ::CopyStr(newTitle, title);

  if (ch) {
    Str255 mtitle;
    ::C2P_Str(title, mtitle);
    ::SetControlTitle(ch, mtitle);
  } else {
    Redraw();
  }
} /* CControl::SetTitle */

void CControl::GetTitle(CHAR *s) {
  ::CopyStr(title, s);
} /* CControl::GetTitle */

BOOL CControl::HasFocus(void) {
  return (Window()->focusCtl == this);
} /* CControl::HasFocus */

Rect CControl::MacRect(void) {
  Rect mr;
  bounds.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  return mr;
} /* CControl::MacRect */

/**************************************************************************************************/
/*                                                                                                */
/*                                            PUSHBUTTONS */
/*                                                                                                */
/**************************************************************************************************/

CPushButton::CPushButton(CViewOwner *parent, CHAR *title, CRect frame,
                         BOOL show, BOOL enable, BOOL useSysFont)
    : CControl(parent, controlType_PushButton, title, frame, true, show,
               enable) {
  Rect mr = MacRect();
  Str255 mtitle;
  ::C2P_Str(title, mtitle);

  UInt16 proc =
      pushButProc + (useSysFont ? 0 : kControlUsesOwningWindowsFontVariant);
  ch = ::NewControl((WindowPtr)(Window()->winRef), &mr, mtitle, Visible(), 0, 0,
                    1, proc, (long)this);
  if (!enable) ::HiliteControl(ch, kControlInactivePart);
} /* CPushButton::CPushButton */

BOOL CPushButton::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (modifiers & modifier_AutoKey) return false;

  if (c == ' ' && Enabled() && Visible()) {
    Press(true);
    return true;
  } else
    return false;
  //### key/command events should return indicator if handled
} /* CPushButton::HandleKeyDown */

void CPushButton::Track(Point pt, INT part) {
  SetBackColor(&color_Dialog);
  Window()->CurrControl(this);
  if (part == kControlButtonPart && ::HandleControlClick(ch, pt, 0, NULL))
    Window()->HandlePushButton(this);
} /* CPushButton::Track */

/*---------------------------------------- Access Routines
 * ---------------------------------------*/

void CPushButton::Press(BOOL notifyParent) {
  ::HiliteControl(ch, kControlButtonPart);
  FlushPortBuffer();
  Sleep(8);
  ::HiliteControl(ch, 0);

  if (notifyParent) Window()->HandlePushButton(this);
} /* CPushButton::Press */

/**************************************************************************************************/
/*                                                                                                */
/*                                          CHECKBOXES */
/*                                                                                                */
/**************************************************************************************************/

CCheckBox::CCheckBox(CViewOwner *parent, CHAR *title, BOOL checked, CRect frame,
                     BOOL show, BOOL enable)
    : CControl(parent, controlType_CheckBox, title, frame, true, show, enable) {
  Rect mr = MacRect();
  Str255 mtitle;
  ::C2P_Str(title, mtitle);

  INT procID =
      checkBoxProc + (RunningOSX() ? 0 : kControlUsesOwningWindowsFontVariant);
  ch = ::NewControl((WindowPtr)(Window()->winRef), &mr, mtitle, Visible(),
                    (checked ? true : false), 0, 1, procID, (long)this);
  if (!enable) ::HiliteControl(ch, kControlInactivePart);
} /* CCheckBox::CCheckBox */

/*----------------------------------------- Event Handling
 * ---------------------------------------*/

BOOL CCheckBox::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (modifiers & modifier_AutoKey) return false;

  if (c == ' ' && Enabled() && Visible()) {
    Window()->HandleCheckBox(this);
    return true;
  } else
    return false;
} /* CCheckBox::HandleKeyDown */

void CCheckBox::Track(Point pt, INT part) {
  if (!RunningOSX()) SetBackColor(&color_Dialog);
  Window()->CurrControl(this);
  if (part == kControlCheckBoxPart && ::HandleControlClick(ch, pt, 0, NULL))
    Window()->HandleCheckBox(this);
} /* CCheckBox::Track */

/*---------------------------------------- Access Routines
 * ---------------------------------------*/

void CCheckBox::Toggle(void) { Check(!Checked()); } /* CCheckBox::Toggle */

void CCheckBox::Check(BOOL checked) {
  ::SetControlValue(ch, checked);
} /* CCheckBox::Check */

BOOL CCheckBox::Checked(void) {
  return ::GetControlValue(ch);
} /* CCheckBox::Checked */

/**************************************************************************************************/
/*                                                                                                */
/*                                          RADIO BUTTONS */
/*                                                                                                */
/**************************************************************************************************/

CRadioButton::CRadioButton(CViewOwner *parent, CHAR *title, INT theGroupID,
                           CRect frame, BOOL show, BOOL enable)
    : CControl(parent, controlType_RadioButton, title, frame, true, show,
               enable) {
  groupID = theGroupID;

  Rect mr = MacRect();
  if (RunningOSX()) mr.bottom = mr.top + 18;

  Str255 mtitle;
  ::C2P_Str(title, mtitle);

  BOOL selected = true;
  CControl *ctl = Window()->firstCtl;
  do {
    if (ctl->type == controlType_RadioButton &&
        groupID == ((CRadioButton *)ctl)->groupID)
      selected = false;
    ctl = ctl->nextCtl;
  } while (selected && ctl != Window()->firstCtl);

  INT procID =
      radioButProc + (RunningOSX() ? 0 : kControlUsesOwningWindowsFontVariant);
  ch = ::NewControl((WindowPtr)(Window()->winRef), &mr, mtitle, Visible(),
                    selected, 0, 1, procID, (long)this);
  if (!enable) ::HiliteControl(ch, kControlInactivePart);
} /* CCheckBox::CCheckBox */

/*----------------------------------------- Event Handling
 * ---------------------------------------*/

BOOL CRadioButton::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (modifiers & modifier_AutoKey) return false;

  if (c == ' ' && Enabled() && Visible()) {
    Window()->HandleRadioButton(this);
    return true;
  } else
    return false;
} /* CRadioButton::HandleKeyDown */

void CRadioButton::Track(Point pt, INT part) {
  if (!RunningOSX()) SetBackColor(&color_Dialog);
  Window()->CurrControl(this);
  if (part == kControlRadioButtonPart && ::HandleControlClick(ch, pt, 0, NULL))
    Window()->HandleRadioButton(this);
} /* CRadioButton::Track */

/*---------------------------------------- Access Routines
 * ---------------------------------------*/

void CRadioButton::Select(void) {
  CControl *ctl = Window()->firstCtl;
  do {
    if (ctl->type == controlType_RadioButton &&
        groupID == ((CRadioButton *)ctl)->groupID)
      SetControlValue(ctl->ch, (ctl == this));
    ctl = ctl->nextCtl;
  } while (ctl != Window()->firstCtl);
} /* CRadioButton::Select */

BOOL CRadioButton::Selected(void) {
  return GetControlValue(ch);
} /* CRadioButton::Selected */

/**************************************************************************************************/
/*                                                                                                */
/*                                           SCROLLBARS */
/*                                                                                                */
/**************************************************************************************************/

#define maxScrollBarRange 10000L

CScrollBar::CScrollBar(CViewOwner *parent, LONG min, LONG max, LONG val,
                       LONG deltaPage, CRect frame, BOOL show, BOOL enable,
                       BOOL isSlider)
    : CControl(parent, (isSlider ? controlType_Slider : controlType_ScrollBar),
               title, frame, false, show, enable) {
  lmin = min;
  lmax = max;
  lval = val;
  pageIncr = deltaPage;
  RecalcSCvalues();

  Rect mr = MacRect();
  INT proc = (isSlider ? kControlSliderProc + kControlSliderNonDirectional
                       : scrollBarProc);
  ch = ::NewControl((WindowPtr)(Window()->winRef), &mr, "\p", Visible(), scval,
                    scmin, scmax, proc, (long)this);
  if (!enable) ::HiliteControl(ch, kControlInactivePart);
} /* CScrollBar::CScrollBar */

void CScrollBar::RecalcSCvalues(void) {
  if (lval < lmin)
    lval = lmin;
  else if (lval > lmax)
    lval = lmax;

  stepFactor = 1 + (lmax - lmin) / maxScrollBarRange;
  scmin = 0;
  scmax = (lmax - lmin) / stepFactor;
  scval = (lval - lmin) / stepFactor;
} /* CScrollBar::RecalcSCvalues */

/*----------------------------------------- Event Handling
 * ---------------------------------------*/

BOOL CScrollBar::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (!Enabled()) return false;

  switch (key) {
    case key_UpArrow:
      LineUp();
      break;
    case key_DownArrow:
      LineDown();
      break;
    case key_PageUp:
      PageUp();
      break;
    case key_PageDown:
      PageDown();
      break;
    case key_Home:
      SetVal(GetMin(), true);
      break;
    case key_End:
      SetVal(GetMax(), true);
      break;
    default:
      return false;
  }

  return true;
} /* CScrollBar::HandleKeyDown */

/*---------------------------------------- Scrolling
 * ---------------------------------------------*/

pascal void ScrollProc(ControlHandle ctrl, short part);

void CScrollBar::Track(Point pt, INT part) {
  INT oldVal;

  Window()->CurrControl(this);

  if (part != kControlIndicatorPart)  //### HMM Continuous scrolling??!?!
  {
    ControlActionUPP action = ::NewControlActionUPP(ScrollProc);
    part = ::HandleControlClick(ch, pt, 0, action);
    ::DisposeControlActionUPP(action);
  } else {
    oldVal = GetVal();
    part = ::HandleControlClick(ch, pt, 0, NULL);

    scval = ::GetControlValue(ch);
    if (scval == ::GetControlMaximum(ch))
      lval = lmax;
    else
      lval = lmin + stepFactor * (LONG)(::GetControlValue(ch));

    if (GetVal() != oldVal) Window()->HandleScrollBar(this, false);
  }
} /* CScrollBar::Track */

pascal void ScrollProc(ControlHandle ctrl, short part) {
  INT delta;
  CScrollBar *sc = (CScrollBar *)::GetControlReference(ctrl);

  switch (part) {
    case 0:
      return;
    case kControlUpButtonPart:
      if (sc->GetVal() == sc->GetMin()) return;
      delta = -1;
      break;
    case kControlDownButtonPart:
      if (sc->GetVal() == sc->GetMax()) return;
      delta = 1;
      break;
    case kControlPageUpPart:
      delta = 0 - sc->GetIncrement();
      break;
    case kControlPageDownPart:
      delta = sc->GetIncrement() - 0;
      break;
      /*
            case kControlIndicatorPart :
               sc->Window()->HandleScrollBar(sc, true);
               return;
      */
  }

  sc->SetVal(sc->GetVal() + delta);
  sc->Window()->HandleScrollBar(sc, true);
} /* ScrollProc */

/*-------------------------------------- Set/Get Range/Value
 * -------------------------------------*/

void CScrollBar::SetMin(LONG min) {
  lmin = min;
  if (lmax < lmin) lmax = lmin;
  RecalcSCvalues();
  ::SetControlValue(ch, scval);
  ::SetControlMinimum(ch, scmin);
  ::SetControlMaximum(ch, scmax);
} /* CScrollBar::SetMin */

void CScrollBar::SetMax(LONG max) {
  lmax = max;
  if (lmin > lmax) lmin = lmax;
  RecalcSCvalues();
  ::SetControlValue(ch, scval);
  ::SetControlMinimum(ch, scmin);
  ::SetControlMaximum(ch, scmax);
} /* CScrollBar::SetMax */

void CScrollBar::SetVal(LONG val, BOOL notifyParent) {
  lval = val;
  if (lval < GetMin())
    lval = GetMin();
  else if (lval > GetMax())
    lval = GetMax();
  RecalcSCvalues();
  ::SetControlValue(ch, scval);
  if (notifyParent) Window()->HandleScrollBar(this, false);
} /* CScrollBar::SetVal */

LONG CScrollBar::GetMin(void) { return lmin; } /* CScrollBar::GetMin */

LONG CScrollBar::GetMax(void) { return lmax; } /* CScrollBar::GetMax */

LONG CScrollBar::GetVal(void) { return lval; } /* CScrollBar::GetVal */

void CScrollBar::SetIncrement(LONG deltaPage) {
  pageIncr = deltaPage;
} /* CScrollBar::SetIncrement */

LONG CScrollBar::GetIncrement(void) {
  return pageIncr;
} /* CScrollBar::GetIncrement */

/*-------------------------------------- Auto Scrolling
 * ------------------------------------------*/
// These methods change the setting of the scrollbar AND informs the owning
// window of the change.

void CScrollBar::LineUp(void) {
  if (GetVal() > GetMin()) SetVal(GetVal() - 1, true);
} /* CScrollBar::LineUp */

void CScrollBar::LineDown(void) {
  if (GetVal() < GetMax()) SetVal(GetVal() + 1, true);
} /* CScrollBar::LineDown */

void CScrollBar::PageUp(void) {
  if (GetVal() > GetMin()) SetVal(MaxL(GetMin(), GetVal() - pageIncr), true);
} /* CScrollBar::PageUp */

void CScrollBar::PageDown(void) {
  if (GetVal() < GetMax()) SetVal(MinL(GetMax(), GetVal() + pageIncr), true);
} /* CScrollBar::PageDown */

/**************************************************************************************************/
/*                                                                                                */
/*                                         LIST BOXES */
/*                                                                                                */
/**************************************************************************************************/

CListBox::CListBox(CViewOwner *parent, CRect frame, INT rows, INT columns,
                   BOOL horScroll, BOOL verScroll, INT cellWidth,
                   INT cellHeight, BOOL _show, BOOL _enable)
    : CControl(parent, controlType_ListBox, "", frame, false, _show, _enable) {
  selRow = -1;
  rowCount = rows;
  colCount = columns;

  Rect mr = MacRect();
  ListDefSpec listDef;
  listDef.defType = kListDefStandardTextType;

  ::CreateListBoxControl(Window()->winRef, &mr, true, rows, columns, horScroll,
                         verScroll, cellHeight, cellWidth, false, &listDef,
                         &ch);
  ::SetControlReference(ch, (long)this);
  ::GetControlData(ch, kControlNoPart, kControlListBoxListHandleTag,
                   sizeof(ListHandle), (Ptr)&listHnd, NULL);

  ::LSetDrawingMode(true, listHnd);

  if (!_show) ::HideControl(ch);
} /* CListBox::CListBox */

BOOL CListBox::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (key == key_UpArrow && selRow > 0) {
    SelectRow(selRow - 1);
    Window()->HandleListBox(this, selRow, 0, false);
    return true;
  } else if (key == key_DownArrow && selRow < rowCount - 1) {
    SelectRow(selRow + 1);
    Window()->HandleListBox(this, selRow, 0, false);
    return true;
  }

  return false;
} /* CListBox::HandleKeyDown */

void CListBox::Track(Point pt, INT part) {
  ControlHandle focusCh;
  ::GetKeyboardFocus(Window()->winRef, &focusCh);
  if (ch != focusCh)
    ::SetKeyboardFocus(Window()->winRef, ch, kControlFocusNextPart);

  part = ::HandleControlClick(ch, pt, 0, NULL);

  if (part == kControlListBoxPart || part == kControlListBoxDoubleClickPart) {
    Cell c;
    c.h = c.v = 0;
    if (::LGetSelect(true, &c, listHnd)) {
      SelectRow(c.v);
      Window()->HandleListBox(this, c.v, c.h,
                              (part == kControlListBoxDoubleClickPart));
    }
  }
} /* CListBox::Track */

void CListBox::Clear(INT rows)  // Deletes all rows
{
  ::LDelRow(0, 0, listHnd);
  ::LAddRow(rows, 0, listHnd);
  rowCount = rows;
  selRow = -1;
  ::Draw1Control(ch);
} /* CListBox::Clear */

void CListBox::SetCell(INT row, INT column, CHAR *text) {
  Cell c;
  c.h = column;
  c.v = row;

  ::LSetCell(text, StrLen(text), c, listHnd);
} /* CListBox::SetCell */

INT CListBox::GetSelectedRow(void) {
  return selRow;
} /* CListBox::GetSelectedRow */

void CListBox::SelectRow(INT row, BOOL selected) {
  if (row == selRow) return;  // Ignore if not changed

  // First de-select old row
  Cell c;
  if (selRow >= 0) {
    c.v = selRow;
    for (c.h = 0; c.h < colCount; c.h++) ::LSetSelect(false, c, listHnd);
  }

  // Select new row
  selRow = row;
  c.v = selRow;
  for (c.h = 0; c.h < colCount; c.h++) ::LSetSelect(true, c, listHnd);

  // Auto scroll if needed
  ::LAutoScroll(listHnd);
  ::Draw1Control(ch);
} /* CListBox::GetSelectedRow */

/**************************************************************************************************/
/*                                                                                                */
/*                                        EDIT CONTROLS */
/*                                                                                                */
/**************************************************************************************************/

CEditControl::CEditControl(CViewOwner *parent, CHAR *text, CRect frame,
                           INT _maxChars, BOOL _show, BOOL _enable,
                           BOOL _password)
    : CControl(parent, controlType_Edit, text, frame, true, _show, _enable) {
  maxChars = Min(_maxChars, control_TitleLength - 1);
  password = _password;
  Rect mr = MacRect();
  if (!RunningOSX()) {
    ::InsetRect(&mr, 3, 3);
    mr.bottom++;
  }

  ControlFontStyleRec style;
  style.flags = kControlUseFontMask | kControlUseFaceMask;
  style.font = kControlFontSmallSystemFont;

  ::CreateEditTextControl(Window()->winRef, &mr, nil, password, true,
                          (RunningOSX() ? nil : &style), &ch);
  ::SetControlData(ch, kControlEntireControl, kControlEditTextTextTag,
                   Min(maxChars, StrLen(title)), title);
  ::SetControlReference(ch, (long)this);

  if (!RunningOSX()) ::EmbedControl(ch, Window()->rootControl);

  if (!_enable) ::HiliteControl(ch, kControlInactivePart);
  if (!_show) ::HideControl(ch);
} /* CEditControl::CEditControl */

/*------------------------------------- Setting/Selecting
 * ----------------------------------------*/

void CEditControl::SetTitle(CHAR *s) {
  SetText(s);
} /* CEditControl::SetTitle */

void CEditControl::GetTitle(CHAR *s) {
  GetText(s);
} /* CEditControl::GetTitle */

void CEditControl::SetText(CHAR *s) {
  ::SetControlData(
      ch, kControlEntireControl,
      (password ? kControlEditTextPasswordTag : kControlEditTextTextTag),
      Min(maxChars, StrLen(s)), s);
  if (Visible()) ::Draw1Control(ch);
} /* CEditControl::SetText */

void CEditControl::GetText(CHAR *s) {
  Size size;
  ::GetControlData(
      ch, kControlEntireControl,
      (password ? kControlEditTextPasswordTag : kControlEditTextTextTag),
      maxChars, s, &size);
  s[size] = 0;
} /* CEditControl::GetText */

void CEditControl::GetTextISOLatin(CHAR *s) {
  Size size;
  CFStringRef cfstr =
      ::CFStringCreateWithCString(NULL, "Bla bla", kCFStringEncodingISOLatin1);
  ::GetControlData(ch, kControlEntireControl,
                   (password ? kControlEditTextPasswordCFStringTag
                             : kControlEditTextCFStringTag),
                   7, (void *)cfstr, &size);
  ::CFStringGetCString(cfstr, s, size + 1, kCFStringEncodingISOLatin1);
}  // CEditControl::GetTextISOLatin

void CEditControl::SetLong(LONG n) {
  CHAR s[20];
  NumToStr(n, s);
  SetText(s);
} /* CEditControl::SetLong */

BOOL CEditControl::GetLong(LONG *n) {
  CHAR s[256];
  GetText(s);
  return StrToNum(s, n);
} /* CEditControl::GetLong */

BOOL CEditControl::ValidateNumber(LONG min, LONG max, BOOL allowEmpty) {
  if (allowEmpty && IsEmpty()) return true;
  LONG n;
  if (GetLong(&n))
    return (n >= min && n <= max);
  else
    return false;
} /* CEditControl::ValidateNumber */

BOOL CEditControl::IsEmpty(void) {
  CHAR s[256];
  GetText(s);
  return (StrLen(s) == 0);
} /* CEditControl::IsEmpty */

/*------------------------------------- Handle Key Events
 * ----------------------------------------*/

BOOL CEditControl::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (!Enabled() || key == key_Enter || key == key_Return) return false;

  ::HandleControlKey(ch, key, c, modifiers);
  CHAR s[control_TitleLength];
  GetText(s);
  if (StrLen(s) > maxChars) SetText(s);
  Window()->HandleEditControl(this, true, true);
  return true;
} /* CEditControl::HandleKeyDown */

/*------------------------------------- Handle Mouse Events
 * --------------------------------------*/

void CEditControl::Track(Point pt, INT part) {
  Window()->CurrControl(this);
  ::HandleControlClick(ch, pt, 0, NULL);
} /* CEditControl::Track */

BOOL CEditControl::HandleMouseDown(CPoint pt, INT modifiers,
                                   BOOL doubleClick)  //### Needed???
{
  return false;  //### Hmm: This method not really needed
} /* CEditControl::HandleMouseDown */

/*-------------------------------------- Handle Misc Events
 * --------------------------------------*/

void CEditControl::HandleFocus(BOOL gotFocus) {
  /*
     if (! Enabled() || ! Visible()) return;
     if (RunningOSX())
        ::SetKeyboardFocus(Window()->winRef, ch, (gotFocus ?
     kControlFocusNextPart : kControlFocusNoPart)); else {  if (gotFocus)
     ::SetKeyboardFocus(Window()->winRef, ch, kControlFocusNoPart); else
     ::ClearKeyboardFocus(Window()->winRef); Redraw();
     }
  */
} /* CEditControl::HandleFocus */

void CEditControl::HandleNullEvent(void)  // Blink the caret if needed...
{
  // if (! RunningOSX()) ::IdleControls(Window()->winRef);
  CControl::HandleNullEvent();  //### Not really needed
} /* CEditControl::HandleNullEvent */

/**************************************************************************************************/
/*                                                                                                */
/*                                          POPUP MENUS */
/*                                                                                                */
/**************************************************************************************************/

CPopupMenu::CPopupMenu(CViewOwner *parent, CHAR *text, CMenu *theMenu,
                       INT itemID, CRect frame, BOOL show, BOOL enable)
    : CControl(parent, controlType_PopupMenu, text, frame, true, show, enable) {
  menu = theMenu;
  ::InsertMenu(menu->hmenu, -1);
  menu->inMenuList = true;

  Rect mr = MacRect();
  if (RunningOSX()) mr.bottom = mr.top + 20;

  Str255 mtitle;
  ::C2P_Str(title, mtitle);
  INT procID =
      popupMenuProc + popupFixedWidth + (RunningOSX() ? 0 : popupUseWFont);
  ch = ::NewControl((WindowPtr)(Window()->winRef), &mr, mtitle, Visible(), 0,
                    menu->menuID, 0, procID, (long)this);
  if (!enable) ::HiliteControl(ch, kControlInactivePart);
  Set(itemID);
} /* CPopupMenu::CPopupMenu */

CPopupMenu::~CPopupMenu(void) {
  ::DeleteMenu(menu->menuID);
  menu->inMenuList = false;
  delete menu;
} /* CPopupMenu::~CPopupMenu */

/*----------------------------------------- Event Handling
 * ---------------------------------------*/

BOOL CPopupMenu::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (!Enabled()) return false;

  INT itemNo, itemID;

  switch (key) {
    case key_UpArrow:
      itemNo = GetControlValue(ch);
      while (--itemNo >= 1) {
        itemID = menu->GetItemID(itemNo);
        if (itemID != -1 && menu->MenuItemEnabled(itemID)) {
          SetControlValue(ch, itemNo);
          Window()->HandlePopupMenu(this, Get());
          return true;
        }
      }
      return true;
    case key_DownArrow:
      itemNo = GetControlValue(ch);
      while (++itemNo <= menu->itemCount) {
        itemID = menu->GetItemID(itemNo);
        if (itemID != -1 && menu->MenuItemEnabled(itemID)) {
          SetControlValue(ch, itemNo);
          Window()->HandlePopupMenu(this, Get());
          return true;
        }
      }
      return true;
    default:
      return false;
  }
} /* CPopupMenu::HandleKeyDown */

void CPopupMenu::Track(Point pt, INT part) {
  INT oldval = Get();

  SetBackColor(&color_Dialog);
  Window()->CurrControl(this);
  if (::HandleControlClick(ch, pt, 0, (OpaqueControlActionProcPtr *)-1))
    Window()->HandlePopupMenu(this, Get());
} /* CPopupMenu::Track */

void CPopupMenu::Set(INT itemID) {
  INT itemNo = menu->GetItemNo(itemID);
  if (itemNo >= 1 && itemNo <= menu->itemCount) ::SetControlValue(ch, itemNo);
} /* CPopupMenu::Set */

INT CPopupMenu::Get(void) {
  return menu->GetItemID(::GetControlValue(ch));
} /* CPopupMenu::Set */

void CPopupMenu::EnableItem(INT itemID, BOOL enable) {
  menu->EnableMenuItem(itemID, enable);
} /* CPopupMenu::EnableItem */

/**************************************************************************************************/
/*                                                                                                */
/*                                         PROGRESS BARS */
/*                                                                                                */
/**************************************************************************************************/

CProgressBar::CProgressBar(CViewOwner *parent, CRect frame, LONG theMin,
                           LONG theMax, BOOL show, BOOL indeterminate)
    : CControl(parent, controlType_ProgressBar, "", frame, false, show) {
  min = theMin;
  max = theMax;
  val = min;

  indeterm = indeterminate;
  indetermPhase = 0;
} /* CProgressBar::CProgressBar */

void CProgressBar::HandleUpdate(CRect updateRect) {
  DrawBar();
} /* CProgressBar::HandleUpdate */

void CProgressBar::DrawBar(void) {
  ThemeTrackDrawInfo T;
  T.kind = (indeterm ? kThemeMediumIndeterminateBar : kThemeMediumProgressBar);
  bounds.SetMacRect(&T.bounds);
  ::OffsetRect(&T.bounds, origin.h, origin.v);

  T.min = min;
  T.max = max;
  T.value = val;
  T.reserved = 0;
  T.attributes = kThemeTrackHorizontal;
  T.enableState = (Active() ? kThemeTrackActive : kThemeTrackInactive);
  T.filler1 = 0;
  T.trackInfo.progress.phase = (indeterm ? indetermPhase + 1 : 0);

  CGrafPtr oldPort = nil;
  GDHandle oldDevice = nil;
  ::GetGWorld(&oldPort, &oldDevice);
  ::SetGWorld(::GetWindowPort(Window()->winRef), nil);

  // Draw the track
  ::DrawThemeTrack(&T, NULL, NULL, 0);

  // Remove annoying black frame?!?!
  if (RunningOSX()) {
    ::RGBForeColor(&color_BtGray);
    ::MoveTo(T.bounds.left, T.bounds.top + 1);
    ::Line(0, 10);
    ::Line(T.bounds.right - T.bounds.left - 1, 0);
    ::Line(0, -10);

    ::RGBForeColor(&color_LtGray);
    ::MoveTo(T.bounds.left, T.bounds.top + 12);
    ::Line(T.bounds.right - T.bounds.left - 1, 0);
  }

  ::SetGWorld(oldPort, oldDevice);
} /* CProgressBar::DrawBar */

void CProgressBar::Reset(void) {
  val = min;
  Redraw();
} /* CProgressBar::Reset */

void CProgressBar::Set(LONG newVal) {
  if (newVal < min)
    newVal = min;
  else if (newVal > max)
    newVal = max;
  if (newVal <= val && !indeterm) return;
  val = newVal;

  if (indeterm) indetermPhase = (indetermPhase + 1) % 16;

  DrawBar();
  FlushPortBuffer();
} /* CProgressBar::Set */

/**************************************************************************************************/
/*                                                                                                */
/*                                          STATIC TEXT */
/*                                                                                                */
/**************************************************************************************************/

CTextControl::CTextControl(CViewOwner *parent, CHAR *text, CRect frame,
                           BOOL show, CCONTROL_FONT font)
    : CControl(parent, controlType_Text, text, frame, false, show) {
  if (RunningOSX()) {
    Rect mr = MacRect();

    ControlFontStyleRec style;
    style.flags = kControlUseFontMask | kControlUseFaceMask;
    style.font = font;

    ::CreateStaticTextControl(Window()->winRef, &mr, nil, &style, &ch);
    ::SetControlData(ch, kControlEntireControl, kControlStaticTextTextTag,
                     StrLen(text) + 1, text);
    ::SetControlReference(ch, (long)this);

    if (!show) ::HideControl(ch);
  }
} /* CTextControl::CTextControl */

void CTextControl::SetTitle(CHAR *newTitle) {
  if (!RunningOSX())
    CControl::SetTitle(newTitle);
  else {
    ::CopyStr(newTitle, title);
    ::SetControlData(ch, kControlEntireControl, kControlStaticTextTextTag,
                     StrLen(title) + 1, title);
    if (Visible()) ::Draw1Control(ch);
  }
} /* CTextControl::SetTitle */

void CTextControl::HandleUpdate(CRect r) {
  if (RunningOSX())
    CControl::HandleUpdate(r);
  else {
    SetStdForeColor();  //&color_Black);
    SetFontMode(fontMode_Or);
    DrawStr(title, bounds, textAlign_Left, true);
  }
} /* CTextControl::HandleUpdate */

/**************************************************************************************************/
/*                                                                                                */
/*                                           GROUP BOX */
/*                                                                                                */
/**************************************************************************************************/

CGroupBox::CGroupBox(CViewOwner *parent, CHAR *text, CRect frame, BOOL show)
    : CControl(parent, controlType_GroupBox, text, frame, false, show) {
  if (RunningOSX()) {
    ::UseThemeFont(kThemeApplicationFont, smSystemScript);
    INT width = ::TextWidth(text, 0, StrLen(text) + 2);

    CRect r(bounds.left + 10, bounds.top - 2, bounds.left + 10 + width,
            bounds.top + 20);
    Rect mr;
    r.SetMacRect(&mr);
    ::OffsetRect(&mr, origin.h, origin.v);

    ::CreateStaticTextControl(Window()->winRef, &mr, nil, nil, &ch);
    ::SetControlData(ch, kControlEntireControl, kControlStaticTextTextTag,
                     StrLen(text) + 1, text);
    ::SetControlReference(ch, (long)this);
  }
} /* CGroupBox::CGroupBox */

void CGroupBox::HandleUpdate(CRect updateRect) {
  CRect r = bounds;
  r.top += FontHeight() / 2;

  if (RunningOSX()) {
    SetForeColor(&color_Gray);
    DrawRectFrame(r);
    Draw1Control(ch);
  } else {
    Draw3DFrame(r, &color_Gray, &color_White);
    r.Inset(1, 1);
    Draw3DFrame(r, &color_White, &color_Gray);

    SetForeColor(&color_Black);
    SetFontStyle(fontStyle_Bold);
    r.Set(bounds.left + 10, bounds.top, bounds.left + 20 + StrWidth(title),
          bounds.top + FontHeight());
    DrawStr(title, r, textAlign_Center);
  }
} /* CGroupBox::HandleUpdate */

/**************************************************************************************************/
/*                                                                                                */
/*                                           DIVIDER */
/*                                                                                                */
/**************************************************************************************************/

CDivider::CDivider(CViewOwner *parent, CRect frame, BOOL show)
    : CControl(parent, controlType_Divider, "", frame, false, show, true) {
  Rect mr = MacRect();

  ::CreateSeparatorControl(Window()->winRef, &mr, &ch);
  ::SetControlReference(ch, (long)this);
  if (!show) ::HideControl(ch);
} /* CDivider::CDivider */

BOOL CDivider::HandleMouseDown(CPoint pt, INT modifiers,
                               BOOL doubleClick)  //### Needed???
{
  return false;
} /* CDivider::HandleMouseDown */

/**************************************************************************************************/
/*                                                                                                */
/*                                            ICONS */
/*                                                                                                */
/**************************************************************************************************/

CIconControl::CIconControl(CViewOwner *parent, INT newIconID, CRect frame,
                           BOOL show)
    : CControl(parent, controlType_Icon, "", frame, false, show) {
  iconID = newIconID;

  Rect mr = MacRect();

  ControlButtonContentInfo iconInfo;
  iconInfo.contentType = kControlContentCIconRes;
  iconInfo.u.resID = newIconID;

  ::CreateIconControl(Window()->winRef, &mr, &iconInfo, true, &ch);
  ::SetControlReference(ch, (long)this);
  if (!show) ::HideControl(ch);
  // if (! RunningOSX()) ::HiliteControl(ch, 0);
} /* CIconButton::CIconButton */

void CIconControl::Set(INT newIconID) {
  //### OS X todo (SetControlData)
  iconID = newIconID;
  Redraw();
} /* CIconControl::Set */

/**************************************************************************************************/
/*                                                                                                */
/*                                            BITMAPS */
/*                                                                                                */
/**************************************************************************************************/

CBitmapControl::CBitmapControl(CViewOwner *parent, CBitmap *bmp, CRect frame,
                               BMP_MODE mode, BOOL show)
    : CControl(parent, controlType_Bitmap, "", frame, false, show, false) {
  bitmap = bmp;
  bitmapMode = mode;
} /* CBitmapControl::CBitmapControl */

void CBitmapControl::HandleUpdate(CRect updateRect) {
  SetForeColor(&color_Black);
  SetBackColor(&color_White);
  DrawBitmap(bitmap, bitmap->bounds, bounds, bitmapMode);
} /* CBitmapControl::HandleUpdate */

BOOL CBitmapControl::HandleMouseDown(CPoint pt, INT modifiers,
                                     BOOL doubleClick)  //### Needed???
{
  return false;
} /* CBitmapControl::HandleMouseDown */

void CBitmapControl::Set(CBitmap *newBmp, BMP_MODE mode) {
  bitmap = newBmp;
  bitmapMode = mode;
  Redraw();
} /* CBitmapControl::Set */

/**************************************************************************************************/
/*                                                                                                */
/*                                           TAB CONTROL */
/*                                                                                                */
/**************************************************************************************************/

CTabControl::CTabControl(CViewOwner *parent, CRect frame, INT count,
                         CHAR *TabStrArray[], BOOL show)
    : CControl(parent, controlType_Tab, "", frame, false, show, false) {
  ControlTabEntry tabArray[30];

  for (INT i = 0; i < count; i++) {
    tabArray[i].icon = NULL;
    tabArray[i].name = ::CFStringCreateWithCString(
        kCFAllocatorDefault, TabStrArray[i], kCFStringEncodingMacRoman);
    tabArray[i].enabled = true;
  };

  Rect mr = MacRect();

  ::CreateTabsControl(Window()->winRef, &mr, kControlTabSizeSmall,
                      kControlTabDirectionEast, count, tabArray, &ch);
  ::SetControlReference(ch, (long)this);
  if (!show) ::HideControl(ch);
} /* CTabControl::CTabControl */

void CTabControl::HandleUpdate(CRect updateRect) {
  return;  //###
  SetClip(bounds);
  CControl::HandleUpdate(updateRect);
  ClrClip();
} /* CTabControl::HandleUpdate */

void CTabControl::Track(Point pt, INT part) {
  Window()->CurrControl(this);
  ::HandleControlClick(ch, pt, 0, NULL);
} /* CTabControl::Track */

/**************************************************************************************************/
/*                                                                                                */
/*                                 STARTUP INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

void CControl_Init(void) {
  if (RunningOSX()) {
    controlHeight_PushButton = 20;
    controlWidth_PushButton = 69;
    controlHeight_CheckBox = 18;
    controlHeight_RadioButton = 18;
    controlWidth_ScrollBar = 16;
    controlHeight_Edit = 16;  //(useMacEdit ? 16 : 21);
    controlHeight_PopupMenu = 20;
    controlHeight_ProgressBar = 14;
    controlHeight_Text = 20;
    controlSize_Icon = 32;

    controlVDiff_PushButton = controlHeight_PushButton + 12;
    controlVDiff_CheckBox = 22;
    controlVDiff_RadioButton = 22;
    controlVDiff_Text = 22;
    controlVDiff_Edit = 32;
  } else {
    controlVDiff_PushButton = controlHeight_PushButton + 10;
    controlVDiff_CheckBox = 18;
    controlVDiff_RadioButton = 18;
  }
} /* CControl_Init */
