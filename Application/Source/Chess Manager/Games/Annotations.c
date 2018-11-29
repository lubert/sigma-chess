/**************************************************************************************************/
/*                                                                                                */
/* Module  : Annotations.c */
/* Purpose : This module implements save/load of games in the � Extended format.
 */
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

#include "Annotations.h"
#include "CFont.h"
#include "CMemory.h"
#include "CUtility.h"

INT AnnCharWidth[256];

/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CAnnotation::CAnnotation(INT theLineWidth) {
  maxLineWidth = theLineWidth;
  for (LONG i = 0; i < gameRecSize; i++) AnnTab[i] = nil;
} /* CAnnotation::CAnnotation */

CAnnotation::~CAnnotation(void) { ClearAll(); } /* CAnnotation::~CAnnotation */

/**************************************************************************************************/
/*                                                                                                */
/*                                       CLEARING ANNOTATIONS */
/*                                                                                                */
/**************************************************************************************************/

void CAnnotation::ClearAll(void) {
  for (LONG i = 0; i < gameRecSize; i++) Clear(i);
} /* CAnnotation::ClearAll */

void CAnnotation::Clear(INT i) {
  // AnnGlyph[i] = 0;
  if (AnnTab[i]) {
    Mem_FreeHandle((PTR *)AnnTab[i]);
    AnnTab[i] = nil;
  }
} /* CAnnotation::Clear */

/**************************************************************************************************/
/*                                                                                                */
/*                                       SETTING ANNOTATIONS */
/*                                                                                                */
/**************************************************************************************************/

// The CAnnotation::Set method sets the annotation text for the specified move.
// This also involves computing the line wrapping information (but this is
// optional, since when for instance importing PGN into a collection we only
// need the actual text stored).

void CAnnotation::Set(INT i, CHAR *Text, INT charCount, BOOL wrapLines,
                      BOOL killNewLines) {
  if (charCount == 0) {
    Clear(i);
    return;
  }

  // Compute line wrapping data:
  INT LineStart[1000];  //### Max 1000 lines
  INT lineCount = -1;

  if (killNewLines)
    for (INT i = 0; i < charCount; i++)
      if ((BYTE)(Text[i]) < 32) Text[i] = ' ';

  if (wrapLines)
    lineCount =
        ::WrapLines(Text, charCount, AnnCharWidth, maxLineWidth, LineStart);

  // Allocate new annotation record:
  if (AnnTab[i]) Mem_FreeHandle((HANDLE)AnnTab[i]);
  AnnTab[i] = (ANNHANDLE)Mem_AllocHandle(
      sizeof(ANNREC) + sizeof(INT) * (lineCount + 1) + charCount);

  // Initialize new annotation record:
  Mem_LockHandle((HANDLE)AnnTab[i]);

  (*(AnnTab[i]))->lineCount = lineCount;  // Store line and char count info
  (*(AnnTab[i]))->charCount = charCount;

  INT *L = (*(AnnTab[i]))->Data;  // Store LineStart info
  for (INT n = 0; n <= lineCount; n++) *(L++) = LineStart[n];

  CHAR *A = (CHAR *)L;  // Finally store the actual text
  for (INT n = 0; n < charCount; n++) *(A++) = Text[n];

  Mem_UnlockHandle((HANDLE)AnnTab[i]);
} /* CAnnotation::Set */

/**************************************************************************************************/
/*                                                                                                */
/*                                       GETTING ANNOTATIONS */
/*                                                                                                */
/**************************************************************************************************/

BOOL CAnnotation::Exists(INT i)  // Is there any annotation text for move "i"?
{
  return (AnnTab[i] != nil);
} /* CAnnotation::Exists */

// GetCharCount() returns the total number of chars for move "i".

INT CAnnotation::GetCharCount(INT i) {
  return (AnnTab[i] ? (**(AnnTab[i])).charCount : 0);
} /* CAnnotation::GetCharCount */

// GetLineCount() returns the total number of lines for move "i".

INT CAnnotation::GetLineCount(INT i) {
  return (AnnTab[i] ? (**(AnnTab[i])).lineCount : 0);
} /* CAnnotation::GetLineCount */

// GetText() returns all text (null-terminated) for move "i" into the buffer
// "Text", which must already have been allocated (with extra space for the null
// terminator).

void CAnnotation::GetText(INT i, CHAR *Text, INT *theCharCount) {
  if (!Exists(i)) {
    Text[0] = 0;
    *theCharCount = 0;
  } else {
    Mem_LockHandle((HANDLE)AnnTab[i]);

    INT lineCount = (**AnnTab[i]).lineCount;
    INT charCount = (**AnnTab[i]).charCount;
    CHAR *s = (CHAR *)((**AnnTab[i]).Data + lineCount + 1);
    for (INT n = 0; n < charCount; n++) Text[n] = s[n];
    Text[charCount] = 0;  // Remember to null-terminate.
    *theCharCount = charCount;

    Mem_UnlockHandle((HANDLE)AnnTab[i]);
  }
} /* CAnnotation::GetText */

// GetTextLine() returns the specified line of text (0...lineCount-1) for move
// "i" into the buffer "Text", which must already have been allocated (with
// extra space for the null terminator).

INT CAnnotation::GetTextLine(INT moveNo, INT lineNo, CHAR *t, BOOL *nl) {
  ANNPTR A = *(AnnTab[moveNo]);
  if (nl) *nl = false;

  if (lineNo < 0 ||
      lineNo >= A->lineCount)  // Shouldn't happen -> Internal error
    return 0;
  else {
    INT *LineStart = A->Data;
    CHAR *s = (CHAR *)(&A->Data[A->lineCount + 1]) + LineStart[lineNo];
    INT lineLen = LineStart[lineNo + 1] - LineStart[lineNo];

    for (INT i = 0; i < lineLen; i++) *(t++) = *(s++);
    if (lineLen > 0 && IsNewLine(t[-1])) {
      lineLen--;
      if (nl) *nl = true;
    }
    return lineLen;
  }
} /* CAnnotation::GetTextLine */

/**************************************************************************************************/
/*                                                                                                */
/*                                         LINE WRAPPING */
/*                                                                                                */
/**************************************************************************************************/

// This function splits the contents of the annotation text buffer into a
// sequence of lines. This process starts at "firstLine", and redraws the
// necessary parts if "redraw" is true, i.e. those lines where line end/start
// has changed since last time. This function should be called whenever the
// contents of the text buffer has changed. Note, that if the last char in the
// text buffer is a newline, then the last two entries in LineStart are
// identical: LineStart[txLines - 1] = LineStart[txLines].

INT WrapLines(CHAR *Text, INT charCount, INT CharWidth[], INT maxLineWidth,
              INT LineStart[]) {
  INT line = 0;        // Current line being analyzed.
  INT iBreak = 0;      // Index of last "break" char (space or tab).
  INT lineWidth = 0;   // Pixel width of current line (up to current character).
  INT lineWidth0 = 0;  // Pixel width of current line (up to last space/tab).

  LineStart[0] = 0;

  for (INT i = 0; i < charCount; i++) {
    switch (INT c = Text[i] & 0x00FF) {
      case '\n':
      case '\r':
        LineStart[++line] = i + 1;
        lineWidth = lineWidth0 = 0;
        break;
      case ' ':
      case '\t':
      case '-':
        iBreak = i;
        lineWidth += CharWidth[c];
        lineWidth0 = lineWidth;
        break;

      default:
        lineWidth += CharWidth[c];
    }

    if (lineWidth > maxLineWidth - 5)
      if (iBreak > LineStart[line])
        LineStart[++line] = iBreak + 1, lineWidth -= lineWidth0;
      else
        LineStart[++line] = i, lineWidth = 0;
  }

  LineStart[++line] = charCount;
  return line;
} /* WrapLines */

/**************************************************************************************************/
/*                                                                                                */
/*                                       START UP INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

void InitAnnotationModule(void) {
  CFont font(font_Geneva, fontStyle_Plain, 10);

  for (INT c = 32; c < 256; c++) AnnCharWidth[c] = font.ChrWidth(c);

  // AnnCharWidth['\t'] = 10;
} /* InitAnnotationModule */
