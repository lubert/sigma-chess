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

#pragma once

#include "CControl.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define maxTxLines 1024
#define srcStrLen 50

/**************************************************************************************************/
/*                                                                                                */
/*                                         TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class CEditor : public CControl {
 public:
  CEditor(CViewOwner *owner, CRect frame, CHAR *text, INT bufferSize = 1024,
          BOOL show = true, BOOL enable = true, BOOL readOnly = false);
  virtual ~CEditor(void);

  void Enable(BOOL enabled);
  void Password(BOOL password);

  virtual void HandleActivate(BOOL wasActivated);
  virtual void HandleResize(void);
  virtual void HandleUpdate(CRect updateRect);
  virtual BOOL HandleMouseDown(CPoint pt, INT modifiers, BOOL doubleClick);
  virtual BOOL HandleKeyDown(CHAR c, INT key, INT modifiers);
  virtual void HandleFocus(BOOL gotFocus);
  virtual void HandleNullEvent(void);
  virtual void HandleVisChange(void);

  virtual void HandleUndo(void);
  virtual void HandleRedo(void);
  virtual void HandleCut(void);
  virtual void HandleCopy(void);
  virtual void HandlePaste(void);
  virtual void HandleClear(void);
  virtual void HandleClearAll(void);

  virtual void HandleFind(void);
  virtual void HandleFindAgain(void);
  virtual void HandleReplace(void);
  virtual void HandleReplaceFind(void);
  virtual void HandleReplaceAll(void);

  virtual void Track(Point pt, INT part);

  void SetText(CHAR *s, INT count = -1);
  void InsText(CHAR *s, INT count = -1);
  INT GetText(CHAR *s);
  INT CharCount(void);

  BOOL CanUndo(void);
  BOOL CanRedo(void);

  BOOL CanFindAgain(void);
  BOOL CanReplace(void);

  BOOL TextSelected(void);
  BOOL Dirty(void);
  void ClearDirty(void);

  // pseudo-private:

  void DrawText(INT firstLine = -1, INT lastLine = -1, BOOL eraseRest = false);

  INT txLines;     // Number of lines.
  INT visTxLines;  // Maximum number of visible annotation text lines.

 private:
  //--- Editor data ---
  INT textLineHeight;  // = FontHeight(). Stored as variable for optimization
                       // reasons.
  CHAR *Text;          // Annotation text buffer for current game move.
  LONG txSize, maxTxSize;  // Number of characters in this buffer.
  INT LineStart[maxTxLines +
                1];  // Indices in Text[] of first char in each line.

  LONG selStart;  // If a character sequence has been selected, selStart and
  LONG selEnd;    // selEnd indicates the first and last character. Are both
  LONG caret;     // Index in Text[] buffer where next char will be typed.
  LONG lastCaret;
  BOOL showCaret;

  BOOL dirty;
  BOOL readOnly;
  BOOL password;

  ControlHandle scrollBar;

  CRect editRect;  // Rectangle enclosing the actual edit field.

  //--- Process Keyboard Input ---
  void DoTypeChar(CHAR c);
  void DoBackDel(void);
  void DoForwardDel(void);
  void DoPrevChar(BOOL lineChange, INT modifiers);
  void DoNextChar(BOOL lineChange, INT modifiers);
  void DeleteSelection(void);

  //--- Process Mouse Input ---
  void UpdateSelection(INT from, INT to, INT selStart0, INT selEnd0);
  INT PointToPos(CPoint pt);

  //--- Editing ---
  void InsertText(INT pos, INT count, CHAR s[]);
  void RemoveText(INT pos, INT count);

  //--- Drawing ---
  void CalcEditorDim(void);
  void DrawTextLine(INT line);
  void MoveToLine(INT line);
  void SetColorMode(BOOL hilited);
  INT CalcLine(INT i);
  INT TextExtent(INT i0, INT i1);

  //--- Insertion mark/selection ---
  void ShowCaret(BOOL showIt);
  void DrawCaret(INT i);
  void DoSelectAll(void);
  void Deselect(void);

  //--- History (Undo/Redo) handling ---
  void ResetHistory(void);
  void HisAddEvent(INT type, INT pos, INT count, CHAR *s);
  void HisAddWord(INT word);
  void HisUndoRedo(BOOL undoing);
  void HisInsertText(INT pos, INT count, INT wrapCount, CHAR *s);
  INT HisEventSize(INT count);
  void UpdateHistoryEnable(void);

  INT *EditHis;
  INT hisStart, hisEnd, hisMaxEnd;
  BOOL addToHis;

  //--- Scrap handling ---
  void UpdateScrapEnable(void);

  //--- Search Replace ---

  BOOL FindAgain(void);
  BOOL Replace(void);

  CHAR searchStr[srcStrLen + 1];
  CHAR replaceStr[srcStrLen + 1];
  BOOL caseSensitive;

  //--- Scrolling ---
  void AdjustScrollBar(void);
  void ScrollToCaret(void);
  void ScrollUp(void);
  void ScrollDown(void);

  //--- Text line wrapping ---
  void WrapTextLines(void);
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

INT WrapTextLines(CHAR *Text, INT charCount, INT CharWidth[], INT maxLineWidth,
                  INT LineStart[]);
