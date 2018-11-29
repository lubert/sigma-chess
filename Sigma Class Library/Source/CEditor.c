/**************************************************************************************************/
/*                                                                                                */
/* Module  : CEditor.c */
/* Purpose : Implements a multi-line editor control (with a scrollbar) and the
 * standard editor    */
/*           facilities such as cut/copy/paste and undo/redo). */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this
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

#include "CEditor.h"
#include "CApplication.h"
#include "CDialog.h"
#include "CMemory.h"

/*-------------------------------------- Constants/Macros
 * ------------------------------------*/

#define maxHisSize \
  10000  // Size in words of history buffer (MUST be larger than txSize)

#define hisInsert 0
#define hisRemove 1

#define firstVisLine() (GetControlValue(scrollBar) - 1)
#define lastVisLine() (firstVisLine() + visTxLines - 1)

/*-------------------------------- Local Function Prototypes
 * ---------------------------------*/

static INT CharWidthTab[256];
static BOOL charWidthInitialized = false;

pascal void EditScrollProc(ControlHandle ch, INT part);

/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CEditor::CEditor(CViewOwner *owner, CRect frame, CHAR *initText, INT bufferSize,
                 BOOL show, BOOL enable, BOOL isReadOnly)
    : CControl(owner, controlType_Editor, "", frame, true, show, enable) {
  readOnly = isReadOnly;
  wantsReturn = !readOnly;
  password = false;

  // Create scrollbar control:
  Rect mr;
  ::SetRect(&mr, bounds.right - controlWidth_ScrollBar, bounds.top,
            bounds.right, bounds.bottom);
  ::OffsetRect(&mr, origin.h, origin.v);
  if (RunningOSX()) ::InsetRect(&mr, 0, 1);
  scrollBar = ::NewControl((WindowPtr)(Window()->winRef), &mr, "\p", Visible(),
                           1, 1, 1, scrollBarProc, (long)this);

  // Initialize actual edit rect:
  CalcEditorDim();

  // Reset selection:
  caret = 0;
  selStart = selEnd = -1;

  // Allocate and initialize text buffer:
  txSize = StrLen(initText);
  maxTxSize = Max(txSize + 1, bufferSize);
  Text = (CHAR *)Mem_AllocPtr(maxTxSize + 1);
  CopyStr(initText, Text);
  WrapTextLines();

  // Initialize history buffer:
  EditHis = (INT *)Mem_AllocPtr(maxHisSize);
  addToHis = true;
  ResetHistory();

  // Initialize misc:
  dirty = false;

  searchStr[0] = 0;
  replaceStr[0] = 0;
  caseSensitive = true;

  // Finally show caret and update enable state
  ShowCaret(true);

  UpdateScrapEnable();
  UpdateHistoryEnable();
} /* CEditor::CEditor */

CEditor::~CEditor(void) {
  Mem_FreePtr((PTR)Text);
  Mem_FreePtr((PTR)EditHis);
  ::DisposeControl(scrollBar);
} /* CEditor::~CEditor */

void CEditor::CalcEditorDim(void) {
  textLineHeight = FontHeight();
  editRect = bounds;
  editRect.right -= controlWidth_ScrollBar - 1;
  editRect.Inset(4, 4);
  visTxLines = editRect.Height() / textLineHeight;
  editRect.bottom = editRect.top + visTxLines * textLineHeight;
} /* CEditor::CalcEditorDim */

/**************************************************************************************************/
/*                                                                                                */
/*                                       MISC EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void CEditor::HandleActivate(BOOL wasActivated) {
  AdjustScrollBar();
  Redraw();
} /* CEditor::HandleActivate */

void CEditor::HandleFocus(BOOL gotFocus) {
  if (!Enabled() || !Visible()) return;

  if (!gotFocus)
    Deselect();
  else {
    DrawText();
    ShowCaret(true);
  }
} /* CEditor::HandleFocus */

void CEditor::HandleNullEvent(void)  // Blink the caret if needed...
{
  if (!Enabled()) return;

  if (selStart == -1 && TickCount() >= lastCaret + GetCaretTime())
    ShowCaret(!showCaret);
} /* CEditor::HandleNullEvent */

void CEditor::HandleResize(void) {
  Rect mr;
  ::SetRect(&mr, bounds.right - controlWidth_ScrollBar, bounds.top,
            bounds.right, bounds.bottom);
  ::OffsetRect(&mr, origin.h, origin.v);
  if (RunningOSX()) ::InsetRect(&mr, 0, 1);
  ::MoveControl(scrollBar, mr.left, mr.top);
  ::SizeControl(scrollBar, controlWidth_ScrollBar, mr.bottom - mr.top);
  CalcEditorDim();
  AdjustScrollBar();
} /* CEditor::HandleResize */

void CEditor::HandleVisChange(void) {
  CRect r = bounds;
  r.Inset(-2, -2);

  if (!Visible()) {
    ::HideControl(scrollBar);
    if (Window()->IsDialog()) DrawRectFill(r, &color_Dialog);
    if (Window()->focusCtl == this) Window()->focusCtl = nil;
  } else {
    if (Window()->IsDialog()) DrawRectFill(r, &color_Dialog);
    ::ShowControl(scrollBar);
    if (Window()->focusCtl == nil) Window()->focusCtl = this;
  }
} /* CEditor::HandleVisChange */

void CEditor::Enable(BOOL wasEnabled) {
  CView::Enable(wasEnabled);
  AdjustScrollBar();
  ShowCaret(false);
} /* CEditor::Enable */

void CEditor::Password(BOOL pwd) { password = pwd; } /* CEditor::Password */

/**************************************************************************************************/
/*                                                                                                */
/*                                       PROCESS KEYBOARD INPUT */
/*                                                                                                */
/**************************************************************************************************/

// This is the main editing routine for keyboard based input. If a selection
// currently exists, this will be deleted first. Also, if the insertion mark is
// not within the current scroll view, we have to scroll back first.

BOOL CEditor::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  if (!Enabled() || !Visible()) return false;

  if (modifiers & modifier_Command)  // First handle cut/copy/paste commands:
  {
    if (readOnly) {
      if (c == 'C' || c == 'c') {
        HandleCopy();
        return true;
      }
      return false;
    }

    switch (c) {
      case 'X':
      case 'x':
        HandleCut();
        break;
      case 'C':
      case 'c':
        HandleCopy();
        break;
      case 'V':
      case 'v':
        HandlePaste();
        break;
      case 'Z':
      case 'z':
        if (modifiers & modifier_Shift)
          HandleRedo();
        else
          HandleUndo();
        break;
      default:
        return false;
    }
  } else  // Next check for other keys:
  {
    if (readOnly && key == key_Return) return false;

    ShowCaret(false);

    switch (key) {
      case key_LeftArrow:
        DoPrevChar(0, modifiers);
        break;
      case key_UpArrow:
        DoPrevChar(1, modifiers);
        break;
      case key_RightArrow:
        DoNextChar(0, modifiers);
        break;
      case key_DownArrow:
        DoNextChar(1, modifiers);
        break;
      case key_PageUp:
        EditScrollProc(scrollBar, kControlPageUpPart);
        break;
      case key_PageDown:
        EditScrollProc(scrollBar, kControlPageDownPart);
        break;
      case key_Home:
        ::SetControlValue(scrollBar, ::GetControlMinimum(scrollBar));
        DrawText();
        break;
      case key_End:
        ::SetControlValue(scrollBar, ::GetControlMaximum(scrollBar));
        DrawText();
        break;
      case key_BackDel:
        DoBackDel();
        break;
      case key_FwdDel:
        DoForwardDel();
        break;
      default:
        if (readOnly) return false;
        DoTypeChar(c);  //### ELIMINATE INVALID ASCII CHARACTERS!!!
    }

    ShowCaret(true);
  }

  return true;
} /* CEditor::HandleKeyDown */

/*---------------------------------- Typing Normal Characters
 * ------------------------------------*/

void CEditor::DoTypeChar(CHAR c) {
  if (readOnly) return;

  if (selStart != -1)   // If something is selected, delete those
    DeleteSelection();  // characters first.
  InsertText(caret++, 1, &c);
  ScrollToCaret();
} /* CEditor::DoTypeChar */

/*------------------------------------- Delete Characters
 * ----------------------------------------*/

void CEditor::DoBackDel(void) {
  if (readOnly) return;

  if (selStart != -1)  // If no selection simply delete previous character.
    DeleteSelection();
  else if (caret > 0)
    RemoveText(--caret, 1);
  ScrollToCaret();
} /* CEditor::DoBackDel */

void CEditor::DoForwardDel(void) {
  if (readOnly) return;

  if (selStart != -1)  // If no selection simply delete next character.
    DeleteSelection();
  else if (caret < txSize)
    RemoveText(caret, 1);
  ScrollToCaret();
} /* CEditor::DoForwardDel */

void CEditor::DeleteSelection(void) {
  INT count = selEnd - selStart + 1;

  caret = selStart;
  selStart = selEnd = -1;
  RemoveText(caret, count);
  ShowCaret(true);
  UpdateScrapEnable();
} /* CEditor::DeleteSelection */

/*---------------------- Cursor Movement (Arrow keys, page up/down e.t.c.)
 * -----------------------*/
// DoPrevChar and DoNextChar controls cursor movement and selection, both single
// chars and multiple chars (e.g. words, lines, pages) via key modifiers. The
// contents of the text buffer is NOT altered.

void CEditor::DoPrevChar(BOOL lineChange, INT modifiers) {
  LONG caret0 = caret;
  INT line;

  if (caret > 0) {
    if (selStart != -1)  // If selection currently exists, start by moving
    {                    // insertion mark to START of selection...
      caret = caret0 = selStart;
      if (!(modifiers &
            shiftKey))  // ...and deselect if selection process has stopped
      {
        Deselect();
        return;
      }
    }

    //--- MOVE INSERTION MARK

    if (!lineChange)  // Previous character/word
    {
      caret--;  // Move ins mark left (to beginning of current word if option)

      if (modifiers & optionKey)
        while (caret > 0 &&
               (!IsAlphaNum(Text[caret]) || IsAlphaNum(Text[caret - 1])))
          caret--;
    } else if (caret >= LineStart[1])  // Previous line (or home)
    {
      if (modifiers & optionKey)
        caret = 0;
      else {
        line = CalcLine(caret);
        caret = Min(LineStart[line - 1] + (caret - LineStart[line]),
                    LineStart[line] - 1);
      }
    }

    //--- UPDATE/REMOVE SELECTION

    if (selStart != 0 &&
        (modifiers & shiftKey))  // Select text (left) if shift key is down
    {
      selStart = caret;
      if (selEnd == -1) selEnd = Max(0, caret0 - 1);
      DrawText(CalcLine(selStart), CalcLine(caret0));
    }
  } else if (!(modifiers & shiftKey)) {
    Deselect();
  }

  ScrollToCaret();

  UpdateScrapEnable();
} /* CEditor::DoPrevChar */

void CEditor::DoNextChar(BOOL lineChange, INT modifiers) {
  LONG caret0 = caret;
  INT line;

  if (caret < txSize) {
    if (selStart != -1)  // If selection currently exists, start by moving
    {                    // insertion mark to START of selection...
      caret = caret0 = selEnd + 1;
      if (!(modifiers &
            shiftKey))  // ...and deselect if selection process has stopped
      {
        Deselect();
        return;
      }
    }

    //--- MOVE INSERTION MARK

    if (!lineChange) {
      caret++;  // Move ins mark right (to end of current word if option)

      if (modifiers & optionKey)
        while (caret < txSize &&
               (!IsAlphaNum(Text[caret - 1]) || IsAlphaNum(Text[caret])))
          caret++;
    } else if (txLines > 0 &&
               caret < LineStart[txLines - 1])  // Next line (or home)
    {
      if (modifiers & optionKey)
        caret = txSize;
      else {
        line = CalcLine(caret);
        caret = LineStart[line + 1] + (caret - LineStart[line]);
        if (caret > txSize)
          caret = txSize;
        else
          caret = Min(caret, LineStart[line + 2] - 1);
      }
    }

    //--- UPDATE/REMOVE SELECTION

    if (selEnd != txSize - 1 &&
        (modifiers & shiftKey))  // Select text (right) if shift key is down
    {
      selEnd = caret - 1;
      if (selStart == -1) selStart = caret0;
      DrawText(CalcLine(caret0), CalcLine(selEnd));
    }
  } else if (!(modifiers & shiftKey)) {
    Deselect();
  }

  ScrollToCaret();

  UpdateScrapEnable();
} /* CEditor::DoNextChar */

/**************************************************************************************************/
/*                                                                                                */
/*                                       PROCESS MOUSE INPUT */
/*                                                                                                */
/**************************************************************************************************/

BOOL CEditor::HandleMouseDown(CPoint thePt, INT modifiers, BOOL doubleClick)

// This is the main editing routine for mouse based input. Is called when the
// user clicks in the open annotation editor. It handles selection, point and
// click e.t.c in the actual text area, as well as numeric annotation glyphs,
// fonts/styles/sizes e.t.c.

{
  INT from, to;
  CPoint pt0, pt;

  if (!Enabled()) return false;

  Window()->CurrControl(this);

  ShowCaret(false);

  pt0 = thePt;
  to = PointToPos(pt0);  // Calc "pivot" char

  if (to >= 0) {
    if (!(modifiers & shiftKey))  // If shift key not down, we have to deselect
      caret = from = to,          // first (if selection exists).
          Deselect();
    else if (selStart == -1)
      from = caret,
      //       UpdateSelection(from, to, txSize, 0);
          UpdateSelection(from, to, 0, txSize);
    else
      from = caret = (selStart <= to ? selStart : selEnd + 1),
      UpdateSelection(from, to, selStart, selEnd);

    if (!modifiers && selStart == -1 && doubleClick && IsAlphaNum(Text[to])) {
      for (selStart = to; selStart > 0 && IsAlphaNum(Text[selStart - 1]);
           selStart--)
        ;
      for (selEnd = to; selEnd < txSize && IsAlphaNum(Text[selEnd + 1]);
           selEnd++)
        ;
      from = caret = selStart;
      to = selEnd + 1;
      UpdateSelection(from, to, selStart, selEnd);
    }

    BOOL done = false;

    do  // Loop until mouse button released and draw selection continuosly...
    {
      MOUSE_TRACK_RESULT trackResult;
      do {
        TrackMouse(&pt, &trackResult);
        if (trackResult == mouseTrack_Released) done = true;
      } while (pt.Equal(pt0) && !done);

      if (!done) {
        if (pt.v > editRect.bottom)
          ScrollDown();
        else if (pt.v < editRect.top)
          ScrollUp();

        pt.h = Max(pt.h, editRect.left);
        pt.h = Min(pt.h, editRect.right);
        pt.v = Max(pt.v, editRect.top);
        pt.v = Min(pt.v, editRect.bottom);

        pt0 = pt;
        to = PointToPos(pt0);
        UpdateSelection(from, to, selStart, selEnd);
      }
    } while (!done);
  }

  ShowCaret(true);
  UpdateScrapEnable();

  return true;
} /* CEditor::HandleMouseDown */

void CEditor::UpdateSelection(INT from, INT to, INT selStart0, INT selEnd0) {
  if (from == to)
    Deselect();
  else {
    if (showCaret && selStart == -1) ShowCaret(false);
    selStart = Min(from, to);
    selEnd = Max(from, to) - 1;
    DrawText(CalcLine(Min(selStart, selStart0)),
             CalcLine(Max(selEnd, selEnd0)));
  }
} /* CEditor::UpdateSelection */

INT CEditor::PointToPos(CPoint pt) {
  register INT line, h, i;

  line = firstVisLine() + Max(0, (pt.v - editRect.top) / textLineHeight);

  if (line >= txLines)
    return txSize;
  else {
    h = editRect.left;
    for (i = LineStart[line]; i < LineStart[line + 1]; i++) {
      h += CharWidthTab[Text[i]];
      if (h >= pt.h) return i;
    }

    return (line < txLines - 1 ? i - 1 : txSize);
  }
} /* CEditor::PointToPos */

/**************************************************************************************************/
/*                                                                                                */
/*                                              EDITING */
/*                                                                                                */
/**************************************************************************************************/

void CEditor::InsertText(INT pos, INT count, CHAR s[]) {
  if (count + txSize >= maxTxSize || txLines + 1 >= maxTxLines) {
    Deselect();
    caret = 0;
    ScrollToCaret();
    DrawText();
    return;
  }

  dirty = true;
  HisAddEvent(hisInsert, pos, count, s);

  for (INT i = txSize - 1; i >= pos; i--) Text[i + count] = Text[i];
  for (INT i = 0; i < count; i++) Text[i + pos] = s[i];
  txSize += count;

  WrapTextLines();
  DrawText(CalcLine(pos), txLines - 1);
  UpdateScrapEnable();
} /* CEditor::InsertText */

void CEditor::RemoveText(INT pos, INT count) {
  dirty = true;
  HisAddEvent(hisRemove, pos, count, &Text[pos]);

  for (INT i = pos; i < txSize - count; i++) Text[i] = Text[i + count];
  txSize -= count;

  WrapTextLines();
  DrawText(CalcLine(pos), txLines - 1, true);
  UpdateScrapEnable();
} /* CEditor::RemoveText */

void CEditor::SetText(CHAR *s, INT size) {
  ShowCaret(false);

  if (size < 0) size = StrLen(s);
  if (size > maxTxSize) size = maxTxSize;

  selStart = selEnd = -1;
  ResetHistory();
  for (INT i = 0; i < size; i++) Text[i] = s[i];
  txSize = size;
  WrapTextLines();
  DrawText(0, txLines - 1, true);

  caret = 0;
  ShowCaret(true);
  dirty = false;

  UpdateScrapEnable();
  UpdateHistoryEnable();
} /* CEditor::SetText */

void CEditor::InsText(CHAR *s, INT size) {
  if (size < 0) size = StrLen(s);

  if (selStart != -1)   // If something is selected, delete those
    DeleteSelection();  // characters first.

  ShowCaret(false);
  InsertText(caret, size, s);
  caret += size;
  ShowCaret(true);
  ScrollToCaret();
} /* CEditor::InsText */

INT CEditor::GetText(CHAR *s) {
  for (INT i = 0; i < txSize; i++) s[i] = Text[i];
  s[txSize] = 0;
  return txSize;
} /* CEditor::GetText */

INT CEditor::CharCount(void) { return txSize; } /* CEditor::CharCount */

BOOL CEditor::Dirty(void) { return dirty; } /* CEditor::Dirty */

void CEditor::ClearDirty(void) { dirty = false; } /* CEditor::ClearDirty */

/***************************************************************************************************/
/*                                                                                                 */
/*                                             DRAWING */
/*                                                                                                 */
/***************************************************************************************************/

void CEditor::HandleUpdate(CRect updateRect) {
  if (scrollBar && Visible()) {
    ::ShowControl(scrollBar);
    ::Draw1Control(scrollBar);
  }

  // First draw 3D Frame (if it's a dialog):
  if (Window()->IsDialog()) {
    CRect frame3D = bounds;
    frame3D.Inset(-1, -1);
    Draw3DFrame(frame3D, &color_Gray, &color_White);
  }

  // Next draw the black boundary rectangle
  SetForeColor(RunningOSX() || !Active() ? &color_MdGray : &color_Black);
  DrawRectFrame(bounds);
  SetForeColor(&color_Black);

  // Then clear edit field (white):
  CRect r = bounds;
  r.Inset(1, 1);
  r.right -= controlWidth_ScrollBar - 1;
  DrawRectFill(r, &color_White);

  // Finally draw actual text and caret (if any):
  DrawText();
  // DrawCaret(); ???
} /* CEditor::HandleUpdate */

/*-------------------------------------- Main Drawing Routine
 * -------------------------------------*/
// This is the main drawing routine, which draws the characters from i0 to i1
// (inclusive) whilst clipping to the currently visible "scroll" area. If
// fullLines is true, all characters in each line will be drawn, and the area to
// end of each line will be erased. Note that 0 <= line < txLines. If line =
// txLines - 1, we also have to draw the end of text marker.

void CEditor::DrawText(INT firstLine, INT lastLine, BOOL eraseRest) {
  if (!Visible()) return;

  if (firstLine == -1) firstLine = 0;
  if (lastLine == -1) lastLine = txLines - 1;

  firstLine = Max(firstLine - 1, firstVisLine());
  lastLine = Min(lastLine, lastVisLine());

  for (INT line = firstLine; line <= lastLine; line++) DrawTextLine(line);

  SetForeColor(&color_Black);
  SetBackColor(&color_White);

  if (eraseRest) {
    CRect r = editRect;
    r.top += (lastLine + 1 - firstVisLine()) * textLineHeight;
    DrawRectErase(r);
  }
} /* CEditor::DrawText */

/*---------------------------------------- Drawing Single Line
 * ------------------------------------*/
// This routine draws all characters of a single line. Style indicates the style
// of the last character on the previous line (if any). If this is specified, it
// is assumed that the current font style has already been set accordingly, and
// that the background (selection) colour has also been set. This function
// should only be called within a Begin/End draw context.

void CEditor::DrawTextLine(INT line) {
  if (line < firstVisLine() || line > lastVisLine()) return;

  INT imin = LineStart[line];  // Index of first and last character on the line.
  INT imax = LineStart[line + 1] - 1;

  MoveToLine(line);
  SetColorMode(selStart != -1 && imin >= selStart && imin <= selEnd);

  while (imin <= imax)  // Find and draw uniform sequence of characters
  {
    INT i;
    for (i = imin + 1; i <= imax && i != selStart && i != selEnd + 1; i++)
      ;
    if (selStart != -1)
      if (imin == selStart)
        SetColorMode(true);
      else if (imin == selEnd + 1)
        SetColorMode(false);

    if (!password)
      DrawStr(Text, imin, i - imin);
    else
      for (INT j = imin; j < i - imin; j++) DrawStr("*");
    imin = i;
  }

  TextEraseTo(editRect.right);
} /* CEditor::DrawTextLine */

/*--------------------------------------------- Utility
 * -------------------------------------------*/

void CEditor::MoveToLine(INT line) {
  MovePenTo(editRect.left, editRect.top +
                               (line - firstVisLine() + 1) * textLineHeight -
                               FontDescent());
} /* CEditor::MoveToLine */

void CEditor::SetColorMode(BOOL hilited) {
  SetForeColor(Active() && Enabled() ? &color_Black : &color_MdGray);
  if (!hilited)
    SetBackColor(&color_White);
  else if (!Active())
    SetBackColor(&color_BtGray);
  else {
    RGB_COLOR hiColor;
    GetHiliteColor(&hiColor);
    SetBackColor(&hiColor);
    if (((ULONG)hiColor.red + (ULONG)hiColor.green + (ULONG)hiColor.red) / 3 <
        33000L)
      SetForeColor(&color_White);
  }
} /* CEditor::SetColorMode */

INT CEditor::CalcLine(INT i)  // Calculate line number of character i.
{
  for (INT j = 1; j <= txLines; j++)
    if (i < LineStart[j]) return j - 1;
  return Max(0, txLines - 1);
} /* CEditor::CalcLine */

INT CEditor::TextExtent(INT i0, INT i1) {
  return (i0 > i1 ? 0 : StrWidth(Text, i0, i1 - i0 + 1));
} /* CEditor::TextExtent */

/***************************************************************************************************/
/*                                                                                                 */
/*                                      INSERTION MARK/SELECTION */
/*                                                                                                 */
/***************************************************************************************************/

/*------------------------------------------ Insertion Mark
 * ---------------------------------------*/

void CEditor::ShowCaret(BOOL showIt) {
  showCaret = showIt;
  lastCaret = TickCount();
  DrawCaret(caret);
} /* CEditor::ShowInsMark */

void CEditor::DrawCaret(INT i) {
  if (selStart != -1 || !Active() || !Enabled() || readOnly) return;

  SetForeColor(showCaret && HasFocus() && selStart == -1 ? &color_Black
                                                         : &color_White);
  INT line = CalcLine(i);
  if (line >= firstVisLine() && line <= lastVisLine()) {
    MoveToLine(line);
    MovePen(TextExtent(LineStart[line], i - 1) - 1, -FontAscent());
    DrawLine(0, textLineHeight - 2);
  }
  SetForeColor(&color_Black);
} /* CEditor::DrawCaret */

/*-------------------------------------- Text Selection
 * --------------------------------------*/

void CEditor::DoSelectAll(void)  // Selects all characters in annotation text.
{
  if (txSize == 0) return;
  ShowCaret(false);
  selStart = 0;
  selEnd = txSize - 1;
  DrawText();
  UpdateScrapEnable();
} /* CEditor::DoSelectAll */

void CEditor::Deselect(void) {
  if (selStart != -1) {
    INT i1 = selStart;
    INT i2 = selEnd;
    selStart = selEnd = -1;
    DrawText(CalcLine(i1), CalcLine(i2));
    UpdateScrapEnable();
  }
  ShowCaret(true);
} /* CEditor::Deselect */

BOOL CEditor::TextSelected(void) {
  return (Enabled() && selStart != -1);
} /* CEditor::TextSelected */

/***************************************************************************************************/
/*                                                                                                 */
/*                                        UNDO/REDO HISTORY */
/*                                                                                                 */
/***************************************************************************************************/

// The editor history is maintained through a cyclical buffer of editor events
// (currently only events which actually modify the contents of the text
// buffer). Event format:
//
// INT   type;      // Event type, i.e. insertion/deletion of text.
// INT   pos;       // Starting character position of inserted/deleted text.
// INT   count;     // Number of inserted/deleted characters.
// BYTE  Data[];    // Variable length buffer of inserted/deleted text. Is
// padded so that it
//                  // takes up an even number of bytes.
// INT   size;      // Total size of event in WORDS (including this size field).
// Is needed in
//                  // order to move backwards (undo) through the history.

/*---------------------------------------- Reset History
 * ------------------------------------------*/

void CEditor::ResetHistory(void) {
  hisStart = hisEnd = hisMaxEnd = 0;
} /* CEditor::ResetHistory */

/*------------------------------ Adding Edit Events to History
 * -------------------------------*/

void CEditor::HisAddEvent(INT type, INT pos, INT count, CHAR *s) {
  if (!addToHis) return;

  HisAddWord(type);
  HisAddWord(pos);
  HisAddWord(count);
  for (INT n = 0; n < count; n += 2, s += 2)
    HisAddWord((s[0] << 8) | (s[1] & 0x00FF));
  HisAddWord(HisEventSize(count));
  UpdateHistoryEnable();
} /* CEditor::HisAddEvent */

void CEditor::HisAddWord(INT word) {
  INT count;

  EditHis[hisEnd] = word;
  hisEnd = hisMaxEnd = (hisEnd + 1) % maxHisSize;

  // If buffer is full, delete oldest event by advancing hisStart pointer:

  if (hisEnd == hisStart) {
    count =
        EditHis[(hisStart + 2) % maxHisSize];  // Count field of oldest event
    hisStart = (hisStart + HisEventSize(count)) % maxHisSize;
  }
} /* CEditor::HisAddWord */

/*---------------------------------------- Undo/Redo
 * -----------------------------------------*/

void CEditor::HandleUndo(void) {
  if (CanUndo()) HisUndoRedo(true);
} /* CEditor::HandleUndo */

void CEditor::HandleRedo(void) {
  if (CanRedo()) HisUndoRedo(false);
} /* CEditor::HandleRedo */

void CEditor::HisUndoRedo(BOOL undoing) {
  INT type, pos, count, wrapCount, size;
  CHAR *s;

  Deselect();

  ShowCaret(false);

  if (undoing)
    size = EditHis[(hisEnd - 1) % maxHisSize],  // "Pop" history pointer
        hisEnd = (hisEnd - size) % maxHisSize;

  type = EditHis[hisEnd];
  pos = EditHis[(hisEnd + 1) % maxHisSize];
  count = EditHis[(hisEnd + 2) % maxHisSize];
  s = (CHAR *)&EditHis[(hisEnd + 3) % maxHisSize];
  wrapCount = 2 * (maxHisSize - (hisEnd + 3));

  addToHis = false;
  caret = pos;
  if (undoing) switch (type) {
      case hisInsert:
        RemoveText(pos, count);
        break;
      case hisRemove:
        HisInsertText(pos, count, wrapCount, s);
        caret += count;
        break;
    }
  else
    switch (type) {
      case hisInsert:
        HisInsertText(pos, count, wrapCount, s);
        caret += count;
        break;
      case hisRemove:
        RemoveText(pos, count);
        break;
    }
  addToHis = true;

  if (!undoing) hisEnd = (hisEnd + HisEventSize(count)) % maxHisSize;

  ShowCaret(true);
  ScrollToCaret();

  UpdateHistoryEnable();
  UpdateScrapEnable();
} /* CEditor::HisUndoRedo */

void CEditor::HisInsertText(INT pos, INT count, INT wrapCount, CHAR *s)

// When re-inserting text from the history buffer, we have to take into account
// that the string in question may have been "wrapped" and "restarted" from the
// beginning of the cyclical history buffer.

{
  dirty = true;

  for (INT i = txSize - 1; i >= pos; i--) Text[i + count] = Text[i];
  for (INT i = 0; i < count; i++) {
    if (i == wrapCount) s = ((CHAR *)EditHis) - wrapCount;
    Text[i + pos] = s[i];
  }
  txSize += count;

  WrapTextLines();
  DrawText(CalcLine(pos), txLines - 1);
} /* CEditor::HisInsertText */

/*----------------------------------------- Miscellaneous
 * -----------------------------------------*/

INT CEditor::HisEventSize(INT count) {
  return 4 + (count + 1) / 2;
} /* CEditor::HisEventSize */

BOOL CEditor::CanUndo(void) {
  return (hisEnd != hisStart);
} /* CEditor::CanUndo */

BOOL CEditor::CanRedo(void) {
  return (hisEnd != hisMaxEnd);
} /* CEditor::CanRedo */

void CEditor::UpdateHistoryEnable(void) {
  Window()->HandleEditor(this, true, false, false);
} /* CEditor::UpdateHistoryEnable */

/***************************************************************************************************/
/*                                                                                                 */
/*                                           SCRAP HANDLING */
/*                                                                                                 */
/***************************************************************************************************/

void CEditor::HandleCut(void) {
  if (selStart == -1) return;
  theApp->ResetClipboard();
  theApp->WriteClipboard('TEXT', (PTR)&Text[selStart], selEnd - selStart + 1);
  DeleteSelection();
  ScrollToCaret();
} /* CEditor::HandleCut */

void CEditor::HandleCopy(void) {
  if (selStart == -1) return;
  theApp->ResetClipboard();
  theApp->WriteClipboard('TEXT', (PTR)&Text[selStart], selEnd - selStart + 1);
} /* CEditor::HandleCopy */

void CEditor::HandlePaste(void) {
  PTR data;
  LONG size;

  if (selStart != -1)   // If something is selected, delete those
    DeleteSelection();  // characters first.

  if (theApp->ReadClipboard('TEXT', &data, &size) == appErr_NoError) {
    if (size + txSize < maxTxSize) {
      ShowCaret(false);
      InsertText(caret, size, (CHAR *)data);
      caret += size;
      ShowCaret(true);
      ScrollToCaret();
    }
    Mem_FreePtr(data);
  }
} /* CEditor::HandlePaste */

void CEditor::HandleClear(void) {
  if (selStart == -1) return;
  DeleteSelection();
  ScrollToCaret();
} /* CEditor::HandleClear */

void CEditor::HandleClearAll(void) {
  if (txSize == 0) return;
  selStart = selEnd = -1;
  ShowCaret(false);
  caret = 0;
  RemoveText(0, txSize);
  // DrawText(0, txLines - 1);
  ShowCaret(true);
  ScrollToCaret();
} /* CEditor::HandleClearAll */

// Enables the Cut/Copy/Paste/Clear commands according to the current selection.

void CEditor::UpdateScrapEnable(void) {
  Window()->HandleEditor(this, false, true, false);
} /* CEditor::UpdateScrapEnable */

/***************************************************************************************************/
/*                                                                                                 */
/*                                         SEARCH/REPLACE TEXT */
/*                                                                                                 */
/***************************************************************************************************/

void CEditor::HandleFind(void) {
  if (!SearchReplaceDialog(searchStr, replaceStr, &caseSensitive)) return;

  FindAgain();
  Window()->HandleEditor(this, false, false, true);
} /* CEditor::HandleFind */

void CEditor::HandleFindAgain(void) {
  FindAgain();
} /* CEditor::HandleFindAgain */

void CEditor::HandleReplace(void) { Replace(); } /* CEditor::HandleReplace */

void CEditor::HandleReplaceFind(void) {
  if (Replace()) FindAgain();
} /* CEditor::HandleReplaceFind */

void CEditor::HandleReplaceAll(void) {
  do
    Replace();
  while (FindAgain());
} /* CEditor::HandleReplaceAll */

BOOL CEditor::FindAgain(void) {
  INT i0 = (selStart == -1 ? caret : selEnd + 1);  // Start of search
  INT i;

  Text[txSize] = 0;
  if (!SearchStr(&Text[i0], searchStr, caseSensitive, &i)) {
    Beep(1);
    return false;
  }

  i += i0;
  Deselect();
  ShowCaret(false);
  selStart = caret = i;
  selEnd = i + StrLen(searchStr) - 1;
  DrawText();
  UpdateScrapEnable();
  ScrollToCaret();

  return true;
} /* CEditor::FindAgain */

BOOL CEditor::Replace(void) {
  if (selStart != -1)  // && CompareText(&Text[selStart], (CHAR*)&searchStr[1],
                       // searchStr[0]))
  {
    InsText(replaceStr);
    return true;
  } else {
    Beep(1);
    return false;
  }
} /* CEditor::Replace */

/*------------------------------------- Menu Enabling
 * ----------------------------------------*/

BOOL CEditor::CanFindAgain(void) {
  return (searchStr[0] > 0);
} /* CEditor::CanFindAgain */

BOOL CEditor::CanReplace(void) {
  return (replaceStr[0] > 0 && selStart != -1);
} /* CEditor::CanReplace */

/***************************************************************************************************/
/*                                                                                                 */
/*                                               SCROLLING */
/*                                                                                                 */
/***************************************************************************************************/

/*----------------------------------------- Adjusting Scroll Bar
 * ----------------------------------*/

void CEditor::AdjustScrollBar(void)  // Adjust scroll bar to length of edit text
{                                    // and to activation state.
  INT lastLine = Max(0, txLines - visTxLines);

  if (Visible())
    ::ShowControl(scrollBar);
  else
    ::HideControl(scrollBar);
  if (Visible())
    ::HiliteControl(
        scrollBar,
        (lastLine > 0 && Active() && Enabled() ? 0 : kControlInactivePart));

  if (::GetControlValue(scrollBar) > lastLine + 1) {
    ::SetControlValue(scrollBar, lastLine + 1);
    DrawText();
  }
  ::SetControlMaximum(scrollBar, lastLine + 1);
} /* CEDitor::AdjustScrollBar */

/*------------------------------------------ Scrolling
 * --------------------------------------------*/

void CEditor::Track(Point pt, INT part) {
  INT oldVal;

  Window()->CurrControl(this);

  ShowCaret(false);

  if (part != kControlIndicatorPart)
    part = ::TrackControl(scrollBar, pt, NewControlActionUPP(EditScrollProc));
  else {
    oldVal = GetControlValue(scrollBar);
    part = ::TrackControl(scrollBar, pt, nil);
    if (GetControlValue(scrollBar) != oldVal) DrawText();
  }

  ShowCaret(true);
} /* CEditor::Track */

pascal void EditScrollProc(ControlHandle ch, INT part) {
  INT delta;
  CEditor *ce = (CEditor *)::GetControlReference(ch);

  switch (part) {
    case 0:
      return;
    case kControlUpButtonPart:
      if (GetControlValue(ch) == GetControlMinimum(ch)) return;
      delta = -1;
      break;
    case kControlDownButtonPart:
      if (GetControlValue(ch) == GetControlMaximum(ch)) return;
      delta = 1;
      break;
    case kControlPageUpPart:
      delta = 1 - ce->visTxLines;
      break;
    case kControlPageDownPart:
      delta = ce->visTxLines - 1;
  }

  SetControlValue(ch, GetControlValue(ch) + delta);
  ce->DrawText();
} /* EditScrollProc */

/*--------------------------------------- Miscellaneous
 * -------------------------------------------*/

void CEditor::ScrollToCaret(void) {
  INT line = CalcLine(caret);
  if (line < firstVisLine())
    SetControlValue(scrollBar, line + 1);
  else if (line > lastVisLine())
    SetControlValue(scrollBar, line - visTxLines + 2);
  else
    return;

  DrawText();
} /* CEditor::ScrollToCaret */

void CEditor::ScrollUp(void) {
  if (GetControlValue(scrollBar) > GetControlMinimum(scrollBar))
    SetControlValue(scrollBar, GetControlValue(scrollBar) - 1), DrawText();
} /* CEditor::ScrollUp */

void CEditor::ScrollDown(void) {
  if (GetControlValue(scrollBar) < GetControlMaximum(scrollBar))
    SetControlValue(scrollBar, GetControlValue(scrollBar) + 1), DrawText();
} /* CEditor::ScrollDown */

/***************************************************************************************************/
/*                                                                                                 */
/*                                      TEXT LINE WRAPPING */
/*                                                                                                 */
/***************************************************************************************************/

void CEditor::WrapTextLines(void) {
  if (!charWidthInitialized) {
    for (INT c = 32; c < 256; c++) CharWidthTab[c] = ChrWidth(c);
    charWidthInitialized = true;
  }

  INT line = 0;        // Current line being analyzed.
  INT iBreak = 0;      // Index of last "break" char (space or tab).
  INT lineWidth = 0;   // Pixel width of current line (up to current character).
  INT lineWidth0 = 0;  // Pixel width of current line (up to last space/tab).
  INT maxLineWidth = editRect.Width();

  LineStart[0] = 0;

  for (INT i = 0; i < txSize; i++) {
    switch (INT c = Text[i] & 0x00FF) {
      case '\n':
      case '\r':
        LineStart[++line] = i + 1;
        lineWidth = lineWidth0 = 0;
        break;
      case ' ':
      case '-':
        iBreak = i;
        lineWidth += CharWidthTab[c];
        lineWidth0 = lineWidth;
        break;

      default:
        lineWidth += CharWidthTab[c];
    }

    if (lineWidth > maxLineWidth - 5)
      if (iBreak > LineStart[line])
        LineStart[++line] = iBreak + 1, lineWidth -= lineWidth0;
      else
        LineStart[++line] = i, lineWidth = 0;
  }

  LineStart[++line] = txSize;

  txLines = line;

  AdjustScrollBar();
} /* CEditor::WrapTextLines */
