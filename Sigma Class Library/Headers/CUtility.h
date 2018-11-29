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

#include "General.h"

// extern INT OSError;

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- The CRect Class
 * --------------------------------------*/

class CRect {
 public:
  CRect(INT left, INT top, INT right, INT bottom);
  CRect(CRect *r);
  CRect(void);

  void Set(INT left, INT top, INT right, INT bottom);
  void Offset(INT hor, INT ver);
  void Inset(INT hor, INT ver);
  void Normalize(void);
  BOOL Intersect(CRect *r1, CRect *r2);
  void Union(CRect *r1, CRect *r2);
  INT Width(void);
  INT Height(void);

  void SetMacRect(Rect *r);

  INT left;
  INT top;
  INT right;
  INT bottom;
};

/*----------------------------------------- The CPoint Class
 * -------------------------------------*/

class CPoint {
 public:
  CPoint(INT h, INT v);
  CPoint(void);

  void Set(INT h, INT v);
  void Offset(INT dh, INT dv);
  BOOL InRect(CRect r);
  BOOL Equal(CPoint p);

  INT h;
  INT v;
};

/*----------------------------------------- The CList Class
 * --------------------------------------*/

typedef struct CListElem {
  void *data;
  struct CListElem *next, *prev;
} CLISTELEM;

class CList {
 public:
  CList(void);
  virtual ~CList(void);

  void Append(void *data);
  void *Front(void);
  void Remove(void *data);
  INT Count(void);

  void Scan(void);
  void *Next(void);  // Returns NULL if no more elements.

  BOOL Find(void *data);
  void *NextCyclic(void);
  void *PrevCyclic(void);
  void *Current(void);

 private:
  INT count;
  CLISTELEM *first, *last;

  CLISTELEM *current;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

BOOL OSError(INT resultCode, BOOL showErrorDialog = true);

void Beep(INT n);
void Sleep(LONG ticks);
void SetRGBColor100(RGB_COLOR *c, INT red, INT green, INT blue);
void GetRGBColor100(INT red, INT green, INT blue, RGB_COLOR *c);
void AdjustRGBHue(RGB_COLOR *c, INT deltaPct);

CHAR *LoadStr(INT groupID, INT index);
CHAR *LoadText(INT id);

void ShowHelpTip(CHAR *text);
