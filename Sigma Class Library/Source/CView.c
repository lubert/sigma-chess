/**************************************************************************************************/
/*                                                                                                */
/* Module  : CView.c */
/* Purpose : Implements the general purpose view class which controls all
 * drawing in both windows */
/*           and offscreen bitmaps. */
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

#include "CView.h"
#include "CApplication.h"
#include "CBitmap.h"
#include "CPrint.h"
#include "CWindow.h"

//#define _withThemeFont 1

static void SaveDrawEnv(DrawEnv *env, CGrafPtr port);
static void RestoreDrawEnv(DrawEnv *env, CGrafPtr port);

static CView *currView = nil;  // Current view being manipulated

RGB_COLOR color_White = {0xFFFF, 0xFFFF, 0xFFFF},
          color_Black = {0x0000, 0x0000, 0x0000},

          color_ClGray = {0x3000, 0x3000, 0x3000},
          color_DkGray = {0x5000, 0x5000, 0x5000},
          color_MdGray = {0x7800, 0x7800, 0x7800},
          color_Gray = {0xA000, 0xA000, 0xA000},
          color_BtGray = {0xBB00, 0xBB00, 0xBB00},
          color_LtGray = {0xDE00, 0xDE00, 0xDE00},
          color_BrGray = {0xEE00, 0xEE00, 0xEE00},

          color_Red = {0xFFFF, 0x0000, 0x0000},
          color_Green = {0x0000, 0xFFFF, 0x0000},
          color_Blue = {0x0000, 0x0000, 0xFFFF},
          color_Yellow = {0xFFFF, 0xFFFF, 0x0000},
          color_Cyan = {0x0000, 0xFFFF, 0xFFFF},
          color_Magenta = {0xFFFF, 0x0000, 0xFFFF};

RGB_COLOR color_Dialog = {0xDE00, 0xDE00, 0xDE00};  // color_LtGray

/**************************************************************************************************/
/*                                                                                                */
/*                                          CONSTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CView::CView(CViewOwner *owner, CRect r) : CViewOwner(viewOwner_View) {
  // Compute owner information:
  switch (owner->viewOwnerType) {
    case viewOwner_Window:
      window = (CWindow *)owner;
      bitmap = nil;
      parentView = nil;
      rootPort = ::GetWindowPort(window->winRef);
      break;
    case viewOwner_Bitmap:
      window = nil;
      bitmap = (CBitmap *)owner;
      parentView = nil;
      rootPort = (CGrafPtr)(bitmap->gworld);
      break;
    case viewOwner_View:
      window = ((CView *)owner)->window;
      bitmap = ((CView *)owner)->bitmap;
      parentView = (CView *)owner;
      rootPort = ((CView *)owner)->rootPort;
      break;
    case viewOwner_Print:
      window = nil;
      bitmap = nil;
      parentView = nil;
      rootPort = ((CPrint *)owner)->printPort;
  }

  // Initialize drawing environment:
  SaveDrawEnv(&env, rootPort);

  // Setup coordinates:
  bounds = frame = r;
  bounds.Normalize();
  origin.h = frame.left + (parentView ? parentView->origin.h : 0);
  origin.v = frame.top + (parentView ? parentView->origin.v : 0);

  // Set pen location to upper left corner.
  env.pnLoc.h = origin.h;
  env.pnLoc.v = origin.v;

  // Visible and enabled per default:
  show = visible = true;
  enabled = true;

  // Add to owners view list:
  if (owner) owner->RegisterChild(this);

  // Set default drawing state:
  SetForeColor(&color_Black);
  SetBackColor(&color_White);
  SetPenSize(1, 1);
  MoveTo(0, 0);
  SetDefaultFont();
} /* CView::CView */

/**************************************************************************************************/
/*                                                                                                */
/*                                          DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CView::~CView(void) {
  if (currView == this) currView = nil;

  while (vFirstChild) delete vFirstChild;
  if (vParent) vParent->UnregisterChild(this);
} /* CView::~CView */

/**************************************************************************************************/
/*                                                                                                */
/*                                         DRAWING ROUTINES */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------ Setting Color
 * ---------------------------------------*/

void CView::GetForeColor(RGB_COLOR *c) {
  SavePort();
  ::GetForeColor(c);
  RestorePort();
} /* CView::GetForeColor */

void CView::GetBackColor(RGB_COLOR *c) {
  SavePort();
  ::GetBackColor(c);
  RestorePort();
} /* CView::GetBackColor */

void CView::SetForeColor(RGB_COLOR *c) {
  SavePort();
  ::RGBForeColor(c);
  RestorePort();
} /* CView::SetForeColor */

void CView::SetForeColor(INT red, INT green, INT blue) {
  RGB_COLOR c;
  c.red = red;
  c.green = green;
  c.blue = blue;
  SetForeColor(&c);
} /* CView::SetForeColor */

void CView::SetBackColor(RGB_COLOR *c) {
  SavePort();
  ::RGBBackColor(c);
  RestorePort();
} /* CView::SetBackColor */

void CView::SetBackColor(INT red, INT green, INT blue) {
  RGB_COLOR c;
  c.red = red;
  c.green = green;
  c.blue = blue;
  SetBackColor(&c);
} /* CView::SetForeColor */

void CView::SetStdForeColor(void) {
  SetForeColor(Active() ? &color_Black : &color_DkGray);
} /* CView::SetStdForeColor */

void CView::SetStdBackColor(void) {
  SetBackColor(&color_White);
} /* CView::SetStdBackColor */

void CView::SetFontForeColor(void) {
  SetForeColor(Active() ? &color_Black : &color_MdGray);
} /* CView::SetFontForeColor */

void CView::GetHiliteColor(RGB_COLOR *c) {
  ::GetPortHiliteColor(rootPort, c);
} /* CView::GetHiliteColor */

/*------------------------------------------ Setting Pen
 * -----------------------------------------*/

void CView::GetPenPos(INT *h, INT *v) {
  SavePort();
  Point pnLoc;
  ::GetPortPenLocation(rootPort, &pnLoc);
  *h = pnLoc.h - origin.h;
  *v = pnLoc.v - origin.v;
  RestorePort();
} /* CView::GetPenSize */

void CView::SetPenSize(INT h, INT v) {
  SavePort();
  Point pnSize;
  pnSize.h = h;
  pnSize.v = v;
  ::SetPortPenSize(rootPort, pnSize);
  RestorePort();
} /* CView::SetPenSize */

void CView::GetPenSize(INT *h, INT *v) {
  SavePort();
  Point pnSize;
  ::GetPortPenSize(rootPort, &pnSize);
  *h = pnSize.h;
  *v = pnSize.v;
  RestorePort();
} /* CView::GetPenSize */

/*----------------------------------------- Setting Fonts
 * ----------------------------------------*/

void CView::SetFontSize(INT size) {
  SavePort();
#ifdef _withThemeFont
#else
  ::TextSize(size);
#endif
  RestorePort();
} /* CView::SetFontSize */

void CView::SetFontFace(FONT_FACE font) {
  SavePort();
#ifdef _withThemeFont
  ::UseThemeFont(kThemeLabelFont, smSystemScript);
#else
  ::TextFont(font);
#endif
  RestorePort();
} /* CView::SetFontFace */

void CView::SetFontStyle(INT style) {
  SavePort();
  ::TextFace(style);
  RestorePort();
} /* CView::SetFontStyle */

void CView::SetFontMode(INT mode) {
  SavePort();
  ::TextMode(mode);
  RestorePort();
} /* CView::SetFontMode */

void CView::SetDefaultFont(void) {
  SavePort();
  ::PenNormal();
  ::TextMode(srcCopy);
  ::TextFace(0);
#ifdef _withThemeFont
  ::NormalizeThemeDrawingState();
  ::UseThemeFont(kThemeLabelFont, smSystemScript);
#else
  ::TextFont(font_Geneva);
  ::TextSize(10);
#endif
  RestorePort();
} /* CView::SetDefaultFont */

void CView::SetThemeFont(INT themeFontId) {
  SavePort();
  ::UseThemeFont(themeFontId, smSystemScript);
  RestorePort();
}  // CView::SetThemeFont

/*
void CView::SetFont (CFont *font)
{
   SavePort();
      ::TextFont(font->face);
      ::TextFace(font->style);
      ::TextSize(font->size);
   RestorePort();
}  CView::SetFont */

/*
void CView::GetFont (CFont *font)
{
   SavePort();
      font->face  = (FONT_FACE)::GetPortTextFont(rootPort);
      font->style = (FONT_STYLE)::GetPortTextFace(rootPort);
      font->size  = ::GetPortTextSize(rootPort);
   RestorePort();
}  CView::GetFont */

/*------------------------------------------ Line Drawing
 * ----------------------------------------*/

void CView::MovePen(INT dh, INT dv) {
  if (!visible) return;
  SavePort();
  ::Move(dh, dv);
  RestorePort();
} /* CView::MovePen */

void CView::MovePenTo(INT h, INT v) {
  if (!visible) return;
  SavePort();
  ::MoveTo(h + origin.h, v + origin.v);
  RestorePort();
} /* CView::MovePenTo */

void CView::DrawLine(INT dh, INT dv) {
  if (!visible) return;
  SavePort();
  ::Line(dh, dv);
  RestorePort();
} /* CView::MovePen */

void CView::DrawLineTo(INT h, INT v) {
  if (!visible) return;
  SavePort();
  ::LineTo(h + origin.h, v + origin.v);
  RestorePort();
} /* CView::DrawLineTo */

void CView::DrawPoint(INT h, INT v, RGB_COLOR *color) {
  if (!visible) return;
  if (color != nil) SetForeColor(color);
  SavePort();
  ::MoveTo(h + origin.h, v + origin.v);
  ::Line(0, 0);
  RestorePort();
} /* CView::DrawPoint */

void CView::GetPixelColor(INT h, INT v, RGB_COLOR *pixelColor) {
  SavePort();
  ::GetCPixel(h, v, pixelColor);
  RestorePort();
} /* CView::GetPixelColor */

/*--------------------------------------- Rectangle Drawing
 * --------------------------------------*/

void CView::DrawRectFrame(CRect r) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::FrameRect(&mr);
  RestorePort();
} /* CView::DrawRectFrame */

void CView::DrawRoundRectFrame(CRect r, INT width, INT height) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::FrameRoundRect(&mr, width, height);
  RestorePort();
} /* CView::DrawRoundRectFrame */

void CView::DrawRectErase(CRect r) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::EraseRect(&mr);
  RestorePort();
} /* CView::DrawRectErase */

void CView::DrawThemeBackground(CRect r) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  //    ::InvokeThemeEraseUPP(&mr, 0, 32, false, NULL);
  //		::DrawThemePlacard(&mr, kThemeStatePressedUp);
  //    ::SetThemeBackground(kThemeBackgroundWindowHeader, 32, false);
  //    ::ApplyThemeBackground(1, &mr, kThemeStateActive, 32, false);
  //    ::InvalWindowRect(Window()->winRef, &mr);
  //  	::SetThemeWindowBackground(Window()->winRef,
  //  kThemeBrushDocumentWindowBackground, false);  //###
  //		::DrawThemeSeparator(&mr, kThemeStatePressedUp);
  ::DrawThemeEditTextFrame(&mr, kThemeStatePressedUp);
  //		::DrawThemeEditTextFrame(&mr, kThemeStatePressedUp);
  //      ::EraseRect(&mr);
  RestorePort();
} /* CView::DrawThemeBackground */

void CView::DrawRectFill(CRect r, RGB_COLOR *c) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::RGBForeColor(c);
  ::PaintRect(&mr);
  RestorePort();
} /* CView::DrawRectFill */

void CView::DrawRectFill(CRect r, INT red, INT green, INT blue) {
  RGB_COLOR c;
  c.red = red;
  c.green = green;
  c.blue = blue;
  DrawRectFill(r, &c);
} /* CView::DrawRectFill */

void CView::DrawRectFill(CRect r, INT patternID) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::FillCRect(&mr, ::GetPixPat(patternID));
  RestorePort();
} /* CView::DrawRectFill */

void CView::Draw3DFrame(CRect r, RGB_COLOR *topLeft, RGB_COLOR *bottomRight) {
  SetForeColor(topLeft);
  MovePenTo(r.left, r.top);
  DrawLineTo(r.right - 1, r.top);
  MovePenTo(r.left, r.top);
  DrawLineTo(r.left, r.bottom - 2);

  SetForeColor(bottomRight);
  MovePenTo(r.right - 1, r.top + 1);
  DrawLineTo(r.right - 1, r.bottom - 1);
  DrawLineTo(r.left, r.bottom - 1);
} /* CView::Draw3DFrame */

void CView::Draw3DFrame(CRect r, RGB_COLOR *baseColor, INT topLeftAdj,
                        INT bottomRightAdj) {
  RGB_COLOR c1 = *baseColor;
  RGB_COLOR c2 = *baseColor;
  AdjustColorLightness(&c1, topLeftAdj);
  AdjustColorLightness(&c2, bottomRightAdj);
  Draw3DFrame(r, &c1, &c2);
} /* CView::Draw3DFrame */

void CView::DrawStripeRect(CRect r, INT voffset) {
  if (!RunningOSX())
    DrawRectFill(r, &color_LtGray);
  else {
    //	   RGBColor col = { 59113,59113,59112 };
    //	   RGBColor ctab[4] = { color_White, color_BrGray, col, color_BrGray };

    RGBColor col1 = {61900, 61900, 61900};
    RGBColor col2 = {60900, 60900, 60900};
    //	   RGBColor col1 = { 61604,61604,61604 };
    //    RGBColor col2 = { 60948,60948,60948 };
    RGBColor ctab[4] = {col1, col1, col2, col2};

    for (INT v = 0; v < r.Height(); v++) {
      SetForeColor(&ctab[(v + voffset) % 4]);
      MovePenTo(r.left, r.top + v);
      DrawLine(r.Width() - 1, 0);
    }
  }
} /* CView::DrawStripeRect */

void CView::DrawFocusRect(CRect r, BOOL hasFocus) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::DrawThemeFocusRect(&mr, hasFocus);
  RestorePort();
} /* CView::DrawFocusRect */

void CView::DrawEditFrame(CRect r) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::DrawThemeEditTextFrame(
      &mr, kThemeStateInactive);  //(Active() ? kThemeStateActive :
                                  //kThemeStateInactive));
  RestorePort();
} /* CView::DrawEditFrame */

/*----------------------------------------- Oval Drawing
 * -----------------------------------------*/

void CView::DrawOvalFrame(CRect r) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::FrameOval(&mr);
  RestorePort();
} /* CView::DrawOvalFrame */

void CView::DrawOvalFill(CRect r, RGB_COLOR *c) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::RGBForeColor(c);
  ::PaintOval(&mr);
  RestorePort();
} /* CView::DrawOvalFill */

/*----------------------------------------- Text Drawing
 * -----------------------------------------*/

void CView::DrawStr(CHAR *s) { DrawStr(s, 0, StrLen(s)); } /* CView::DrawStr */

void CView::DrawStr(CHAR *s, INT maxWidth) {
  SavePort();

  INT count = StrLen(s);
  INT swidth = ::TextWidth(s, 0, count);

  // If string too wide -> remove some trailing characters
  if (swidth <= maxWidth)
    ::DrawText(s, 0, count);
  else {
    maxWidth -= ::CharWidth('�');
    while (count > 0 && swidth > maxWidth) swidth = ::TextWidth(s, 0, --count);
    ::DrawText(s, 0, count);
    ::DrawChar('�');
  }

  RestorePort();
} /* CView::DrawStr */

void CView::DrawStr(CHAR *s, INT pos, INT count) {
  if (!visible || count <= 0) return;
  SavePort();
  ::DrawText(s, pos, count);
  RestorePort();
} /* CView::DrawStr */

void CView::DrawStr(CHAR *s, CRect r, TEXT_ALIGNMENT align, BOOL lineWrap,
                    BOOL erase) {
  if (!visible) return;

  SavePort();

  Rect mr;
  r.SetMacRect(&mr);
  OffsetRect(&mr, origin.h, origin.v);

  if (lineWrap) {
    ::TETextBox(s, StrLen(s), &mr, align);
  } else {
    INT count = StrLen(s);
    INT rwidth = r.right - r.left;
    INT swidth = ::TextWidth(s, 0, count);
    INT ewidth = ::CharWidth('�');
    INT preWidth = 0;
    INT postWidth = 0;
    BOOL drawEllipsis = false;

    // If string too wide -> remove some trailing characters
    if (swidth > rwidth && align == textAlign_Left && rwidth >= ewidth) {
      drawEllipsis = true;
      while (count > 0 && swidth > rwidth - ewidth)
        swidth = ::TextWidth(s, 0, --count);
      swidth += ewidth;
    } else {
      while (count > 0 && swidth > rwidth) swidth = ::TextWidth(s, 0, --count);
    }

    FontInfo info;
    ::GetFontInfo(&info);

    switch (align) {
      case textAlign_Left:
        postWidth = rwidth - swidth;
        break;
      case textAlign_Center:
        preWidth = (rwidth - swidth) / 2;
        postWidth = rwidth - preWidth - swidth;
        break;
      case textAlign_Right:
        preWidth = rwidth - swidth;
        break;
    }

    mr.left = r.left + origin.h;
    mr.right = mr.left + preWidth;
    if (erase && preWidth > 0) ::EraseRect(&mr);

    ::MoveTo(mr.left + preWidth, mr.top + info.ascent);
    ::DrawText(s, 0, count);
    if (drawEllipsis) ::DrawChar('�');

    mr.right = r.right + origin.h;
    mr.left = mr.right - postWidth;
    if (erase && postWidth > 0) ::EraseRect(&mr);

    ::MoveTo(mr.right, mr.top + info.ascent);
  }

  RestorePort();
} /* CView::DrawStr */

void CView::DrawChr(CHAR c) {
  if (!visible) return;
  SavePort();
  ::DrawChar(c);
  RestorePort();
} /* CView::DrawChr */

void CView::DrawNum(LONG n) {
  Str255 nstr;

  if (!visible) return;
  ::NumToString(n, nstr);
  SavePort();
  ::DrawString(nstr);
  RestorePort();
} /* CView::DrawNum */

void CView::DrawNumR(LONG n, INT minDigits, BOOL preErase) {
  if (!visible) return;

  Str255 nstr;
  ::NumToString(n, nstr);
  INT pad = Max(0, minDigits - nstr[0]);
  if (pad > 0)
    if (preErase)
      TextErase(ChrWidth('0') * pad);
    else {
      SavePort();
      ::Move(ChrWidth('0') * pad, 0);
      ::DrawString(nstr);
      RestorePort();
      return;
    }
  SavePort();
  ::DrawString(nstr);
  RestorePort();
} /* CView::DrawNumR */

void CView::DrawNumR2(LONG n, INT fieldWidth) {
  if (!visible) return;

  SavePort();
  Str255 nstr;
  CHAR s[12];
  ::NumToString(n, nstr);
  P2C_Str(nstr, s);
  INT width = ::TextWidth(s, 0, nstr[0]);
  ::Move(fieldWidth - width, 0);
  ::DrawString(nstr);
  RestorePort();
} /* CView::DrawNumR2 */

void CView::TextErase(INT pixels) {
  if (pixels <= 0) return;
  SavePort();
  FontInfo info;
  Rect mr;
  Point pnLoc;

  ::GetPortPenLocation(rootPort, &pnLoc);
  ::GetFontInfo(&info);
  mr.top = pnLoc.v - info.ascent;
  mr.bottom = pnLoc.v + info.descent;
  mr.left = pnLoc.h;
  mr.right = mr.left + pixels;
  ::EraseRect(&mr);
  ::Move(pixels, 0);
  RestorePort();
} /* CView::TextErase */

void CView::TextEraseTo(INT h) {
  SavePort();
  Point pnLoc;
  ::GetPortPenLocation(rootPort, &pnLoc);
  INT pixels = h + origin.h - pnLoc.h;
  RestorePort();
  TextErase(pixels);
} /* CView::TextEraseTo */

void CView::SetTextSpacing(INT n, INT d) {
  SavePort();
  ::SpaceExtra(::FixRatio(n, d));
  RestorePort();
} /* CView::SetTextSpacing */

void CView::ResetTextSpacing(void) {
  SavePort();
  ::SpaceExtra(0);
  RestorePort();
} /* CView::ResetTextSpacing */

INT CView::ChrWidth(CHAR c) {
  SavePort();
  INT width = ::CharWidth(c);
  RestorePort();
  return width;
} /* CView::ChrWidth */

INT CView::StrWidth(CHAR *s) {
  return StrWidth(s, 0, StrLen(s));
} /* CView::StrWidth */

INT CView::StrWidth(CHAR *s, INT pos, INT count) {
  if (count <= 0) return 0;

  SavePort();
  INT width = ::TextWidth(s, pos, count);
  RestorePort();
  return width;
} /* CView::StrWidth */

/*------------------------------------------ Font Info
 * -------------------------------------------*/

INT CView::FontAscent(void) {
  FontInfo info;
  SavePort();
  ::GetFontInfo(&info);
  RestorePort();
  return info.ascent;
} /* CView::FontAscent */

INT CView::FontDescent(void) {
  FontInfo info;
  SavePort();
  ::GetFontInfo(&info);
  RestorePort();
  return info.descent;
} /* CView::FontDescent */

INT CView::FontLineSpacing(void) {
  FontInfo info;
  SavePort();
  ::GetFontInfo(&info);
  RestorePort();
  return info.leading;
} /* CView::FontLineSpacing */

INT CView::FontHeight(void) {
  FontInfo info;
  SavePort();
  ::GetFontInfo(&info);
  RestorePort();
  return info.ascent + info.descent + info.leading;
} /* CView::FontHeight */

INT CView::FontMaxChrWidth(void) {
  FontInfo info;
  SavePort();
  ::GetFontInfo(&info);
  RestorePort();
  return info.widMax;
} /* CView::FontMaxChrWidth */

/*----------------------------------------- BitMap Drawing
 * ---------------------------------------*/

void CView::DrawBitmap(CBitmap *srcMap, CRect srcRect, CRect dstRect,
                       BMP_MODE mode) {
  if (!visible) return;
  SavePort();
  Rect r1, r2;
  srcRect.SetMacRect(&r1);
  dstRect.SetMacRect(&r2);
  ::OffsetRect(&r2, origin.h, origin.v);
  ::CopyBits(::GetPortBitMapForCopyBits(srcMap->gworld),
             ::GetPortBitMapForCopyBits(rootPort), &r1, &r2, mode, nil);
  RestorePort();
} /* CView::DrawBitmap */

void CView::DrawPict(INT picID, CRect r) {
  Rect mr;
  PicHandle ph;

  if (!visible) return;
  r.SetMacRect(&mr);
  SavePort();
  ph = GetPicture(picID);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::DrawPicture(ph, &mr);
  ::ReleaseResource((Handle)ph);
  RestorePort();
} /* CView::DrawPict */

void CView::DrawPict(INT picID, INT left, INT top) {
  Rect mr;

  if (!visible) return;
  SavePort();
  PicHandle ph = ::GetPicture(picID);
  mr = (**ph).picFrame;
  ::OffsetRect(&mr, origin.h + left - mr.left, origin.v + top - mr.top);
  ::DrawPicture(ph, &mr);
  ::ReleaseResource((Handle)ph);
  RestorePort();
} /* CView::DrawPict */

/*
void CView::DrawIcon (INT iconID, INT left, INT top, INT size)
{
   CRect r(left, top, left + size, top + size);
   DrawIcon(iconID, r);
} /* CView::DrawIcon */

/*
void CView::DrawIcon (INT iconID, CRect r)
{
   Rect mr;
   r.SetMacRect(&mr);
   SavePort();
      ::OffsetRect(&mr, origin.h, origin.v);
      CIconHandle ch = ::GetCIcon(iconID);
      ::PlotCIcon(&mr, ch);
      ::DisposeCIcon(ch);
   RestorePort();
} /* CView::DrawIcon */

void CView::DrawIcon(INT iconID, CRect r, ICON_TRANS trans) {
  Rect mr;
  r.SetMacRect(&mr);
  SavePort();
  ::OffsetRect(&mr, origin.h, origin.v);
  CIconHandle ch = ::GetCIcon(iconID);
  ::PlotCIconHandle(&mr, kAlignNone, (IconTransformType)trans, ch);
  ::DisposeCIcon(ch);
  RestorePort();
} /* CView::DrawIcon */

/*------------------------------------------- Clipping
 * -------------------------------------------*/

void CView::SetClip(CRect r) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::ClipRect(&mr);
  RestorePort();
} /* CView::SetClip */

void CView::ClrClip(void) {
  if (!visible) return;

  SavePort();
  Rect mr = {-32768, -32768, 32767, 32767};
  ::ClipRect(&mr);
  RestorePort();
} /* CView::ClrClip */

/**************************************************************************************************/
/*                                                                                                */
/*                                    THEME/APPEARANCE DRAWING */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------------- Focus Rectangles
 * --------------------------------------*/

void CView::DrawThemeFocusRectFrame(CRect r, BOOL hasFocus) {
  if (!visible) return;
  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);
  ::DrawThemeFocusRect(&mr, hasFocus);
  RestorePort();
} /* CView::DrawThemeFocusRectFrame */

/*---------------------------------------- List Header Cells
 * -------------------------------------*/

typedef struct {
  CHAR *title;
  INT iconID;
  BOOL active;
} ThemeButtonDrawParam;

static ThemeButtonDrawUPP myThemeButtonDrawUPP = NULL;
pascal void MyThemeButtonDrawCallback(const Rect *bounds, ThemeButtonKind kind,
                                      const ThemeButtonDrawInfo *info,
                                      UInt32 userData, SInt16 depth,
                                      Boolean isColorDev);

void CView::DrawThemeListHeaderCell(CRect r, CHAR *title, INT iconID,
                                    BOOL selected, BOOL pushed,
                                    BOOL ascendDir) {
  if (!visible || r.left > r.right) return;

  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  ::OffsetRect(&mr, origin.h, origin.v);

  ThemeButtonDrawInfo dinfo;
  dinfo.state = (pushed ? kThemeStatePressed : kThemeStateActive);
  dinfo.value = (selected && Active() ? kThemeButtonOn : kThemeButtonOff);
  dinfo.adornment = (!selected ? kThemeAdornmentNone
                               : (ascendDir ? kThemeAdornmentHeaderButtonSortUp
                                            : kThemeAdornmentDefault));

  if (!myThemeButtonDrawUPP)
    myThemeButtonDrawUPP = NewThemeButtonDrawUPP(MyThemeButtonDrawCallback);

  ThemeButtonDrawParam param;
  param.title = (r.Width() > 30 ? title : nil);
  param.iconID = iconID;
  param.active = Active();

  ::DrawThemeButton(&mr, kThemeListHeaderButton, &dinfo, NULL, NULL,
                    myThemeButtonDrawUPP, (UInt32)&param);
  RestorePort();
} /* CView::DrawThemeListHeaderCell */

pascal void MyThemeButtonDrawCallback(const Rect *bounds, ThemeButtonKind kind,
                                      const ThemeButtonDrawInfo *info,
                                      UInt32 userData, SInt16 depth,
                                      Boolean isColorDev) {
  ThemeButtonDrawParam *param = (ThemeButtonDrawParam *)userData;
  if (!param || !param->title) return;
  ::RGBForeColor(param->active ? &color_Black : &color_MdGray);

  if (param->iconID > 0) {
    Rect mr;
    mr.left = bounds->left;
    mr.top = bounds->top + 1;
    mr.right = mr.left + 16;
    mr.bottom = mr.top + 16;
    CIconHandle ch = ::GetCIcon(param->iconID);
    ::PlotCIconHandle(&mr, kAlignNone,
                      (param->active ? kTransformNone : kTransformDisabled),
                      ch);
    ::DisposeCIcon(ch);
  }

  ::MoveTo(bounds->left, bounds->bottom - 3);
  if (param->iconID > 0) ::Move(18, 0);
  ::UseThemeFont(kThemeViewsFont, smSystemScript);
  ::DrawText(param->title, 0, StrLen(param->title));
} /* MyThemeButtonDrawCallback */

/*--------------------------------------------- Tabs
 * ---------------------------------------------*/

void CView::DrawThemeTab(CRect r, TAB_DIR dir, BOOL front, BOOL pushed) {
  if (!Visible()) return;

  SavePort();
  Rect mr;
  r.SetMacRect(&mr);
  MacOffsetRect(&mr, origin.h, origin.v);

  ThemeTabStyle style;
  if (front && Active())
    style = kThemeTabFront;
  else if (pushed)
    style = kThemeTabNonFrontPressed;
  else
    style = kThemeTabNonFront;

  ::DrawThemeTab(&mr, style, (ThemeTabDirection)dir, NULL, (long)this);
  RestorePort();
} /* CView::DrawThemeTab */

/**************************************************************************************************/
/*                                                                                                */
/*                                  DRAWING ENVIRONMENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------- Utility
 * --------------------------------------------*/
// No need to save restore draw envs if it hasn't changed

void CView::SavePort(void) {
  oldPort = nil;
  oldDevice = nil;
  ::GetGWorld(&oldPort, &oldDevice);

  if (this == currView)
    ::SetGWorld(rootPort, nil);
  else {
    if (currView !=
        nil)  // If another view is currently "active", we need to flush its
    {         // drawing environment first
      ::SaveDrawEnv(&currView->env, currView->rootPort);
      ::SetGWorld(currView->rootPort, nil);
      ::RestoreDrawEnv(&currView->saveEnv, currView->rootPort);
    }

    ::SaveDrawEnv(&saveEnv, rootPort);
    ::SetGWorld(rootPort, nil);
    ::RestoreDrawEnv(&env, rootPort);  // <-- Assumes rootPort is current port
    currView = this;
  }

  if (bitmap) bitmap->Lock();
} /* CView::SavePort */

void CView::RestorePort(void) {
  if (bitmap) bitmap->Unlock();

  SetGWorld(oldPort, oldDevice);
} /* CView::RestorePort */

static void SaveDrawEnv(DrawEnv *env, CGrafPtr port) {
  ::GetPortForeColor(port, &env->rgbFgColor);
  ::GetPortBackColor(port, &env->rgbBkColor);
  ::GetPortPenLocation(port, &env->pnLoc);
  ::GetPortPenSize(port, &env->pnSize);
  env->pnMode = ::GetPortPenMode(port);
  env->txFont = ::GetPortTextFont(port);
  env->txFace = ::GetPortTextFace(port);
  env->txMode = ::GetPortTextMode(port);
  env->txSize = ::GetPortTextSize(port);
} /* SaveDrawEnv */

static void RestoreDrawEnv(DrawEnv *env, CGrafPtr port) {
  ::RGBForeColor(&env->rgbFgColor);
  ::RGBBackColor(&env->rgbBkColor);
  ::MoveTo(env->pnLoc.h, env->pnLoc.v);
  ::SetPortPenSize(port, env->pnSize);
  ::SetPortPenMode(port, env->pnMode);
  ::TextFont(env->txFont);
  ::TextFace(env->txFace);
  ::TextMode(env->txMode);
  ::TextSize(env->txSize);
} /* RestoreDrawEnv */

/**************************************************************************************************/
/*                                                                                                */
/*                                          MISC METHODS */
/*                                                                                                */
/**************************************************************************************************/

CWindow *CView::Window(void) { return window; } /* CView::Window */

CView *CView::Parent(void) { return parentView; } /* CView::Parent */

void CView::Show(BOOL _show, BOOL redraw) {
  if (show == _show) return;

  show = _show;
  DispatchShow();
  if (visible && redraw) Redraw();
} /* CView::Show */

void CView::Enable(BOOL wasEnabled, BOOL dispatch) {
  enabled = wasEnabled;

  if (dispatch) {
    forAllChildViews(subView) subView->Enable(enabled);
  }
} /* CView::Enable */

BOOL CView::Active(void) {
  return (window && window->IsActive());
} /* CView::Active */

BOOL CView::Enabled(void) { return enabled; } /* CView::Enabled */

BOOL CView::Visible(void) { return visible; } /* CView::Visible */

void CView::Redraw(BOOL flush) {
  DispatchUpdate(bounds);
  if (flush) FlushPortBuffer();
} /* CView::Redraw */

void CView::Invalidate(void) {
  Rect r;

  if (window) {
    ::SetRect(&r, bounds.left, bounds.top, bounds.right, bounds.bottom);
    ::OffsetRect(&r, origin.h - bounds.left, origin.v - bounds.top);
    ::InvalWindowRect(window->winRef, &r);
  }
} /* CView::Invalidate */

void CView::FlushPortBuffer(CRect *r) {
  if (!RunningOSX()) return;
  ::QDFlushPortBuffer(rootPort, nil);
} /* CView::FlushPortBuffer */

BOOL CView::GetMouseLoc(
    CPoint *p)  // Returns mouse location in view coordinates.
{
  if (!window) return false;

  Point mp;
  ::GetMouse(&mp);
  CRect winFrame = window->Frame();
  winFrame.Normalize();
  p->h = mp.h - winFrame.left - origin.h;
  p->v = mp.v - winFrame.top - origin.v;
  return p->InRect(bounds);
} /* CView::GetMouseLoc */

BOOL CView::TrackMouse(CPoint *p, MOUSE_TRACK_RESULT *result) {
  if (!window) return false;

  Point mp;
  MouseTrackingResult mresult;
  ::TrackMouseLocation(nil, &mp, &mresult);

  CRect winFrame = window->Frame();
  winFrame.Normalize();
  p->h = mp.h - winFrame.left - origin.h;
  p->v = mp.v - winFrame.top - origin.v;
  *result = (MOUSE_TRACK_RESULT)mresult;

  return p->InRect(bounds);
} /* CView::TrackMouse */

/**************************************************************************************************/
/*                                                                                                */
/*                                             EVENTS */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Move/Resize Events
 * ---------------------------------------*/

void CView::SetFrame(CRect newFrame, BOOL update) {
  // First move it (if needed):
  INT dh = newFrame.left - frame.left;
  INT dv = newFrame.top - frame.top;
  if (dh != 0 || dv != 0) {
    frame.Offset(dh, dv);
    DispatchMove(dh, dv);
  }

  // Next resize it (if needed):
  if (frame.Width() != newFrame.Width() ||
      frame.Height() != newFrame.Height()) {
    frame = newFrame;
    bounds = frame;
    bounds.Normalize();
    HandleResize();
  }

  if (window) {
    Rect mr;
    bounds.SetMacRect(&mr);
    ::OffsetRect(&mr, origin.h - mr.left, origin.v - mr.top);
    ::ValidWindowRect(window->winRef, &mr);
  }

  if (update && visible) DispatchUpdate(bounds);
} /* CView::SetFrame */

void CView::DispatchMove(INT dh,
                         INT dv)  // Usually called by the parent view/window
{
  origin.h += dh;
  origin.v += dv;
  env.pnLoc.h += dh;
  env.pnLoc.v += dv;

  forAllChildViews(subView) subView->DispatchMove(dh, dv);

  HandleMove();
} /* CView::DispatchMove */

void CView::HandleMove(
    void)  // Only override this if you need to know that view has moved.
{}         /* CView::HandleMove */

void CView::HandleResize(void) {} /* CView::HandleResize */

void CView::SetBounds(INT h, INT v)  // Changes local coordinate system in view
{
  CRect r = bounds;
  r.Normalize();
  r.Offset(h, v);
  SetBounds(r);
} /* CView::SetBounds */

void CView::SetBounds(CRect r) {
  origin.h = frame.left + (parentView ? parentView->origin.h : 0) - r.left;
  origin.v = frame.top + (parentView ? parentView->origin.v : 0) - r.top;

  forAllChildViews(subView)
      subView->frame.Offset(r.left - bounds.left, r.top - bounds.top);

  bounds = r;
} /* CView::SetBounds */

/*-------------------------------------- Mouse Down/Up Events
 * ------------------------------------*/

BOOL CView::DispatchMouseDown(CPoint pt, INT modifiers, BOOL doubleClick)
// The point pt is in the local view coordinates:
{
  // Exit if invisible or disabled:
  if (!visible) return false;

  // First check if clicked in subview. If so pass the event on and return:
  BOOL inSubView = false;

  forAllChildViews(subView) if (subView->visible && pt.InRect(subView->frame)) {
    // Translate to local view coordinates:
    CPoint lpt = pt;
    lpt.Offset(-subView->frame.left + subView->bounds.left,
               -subView->frame.top + subView->bounds.top);
    if (subView->DispatchMouseDown(lpt, modifiers, doubleClick)) return true;
  }

  // Mouse click in this view (if no clicks in subviews):
  return (!inSubView && HandleMouseDown(pt, modifiers, doubleClick));
} /* CView::DispatchMouseDown */

BOOL CView::HandleMouseDown(CPoint pt, INT modifiers, BOOL doubleClick) {
  // Should be overridden by user to handle mouse down events in this view.
  return false;
} /* CView::HandleMouseDown */

/*---------------------------------------- Key Down Events
 * ---------------------------------------*/

BOOL CView::HandleKeyDown(CHAR c, INT key, INT modifiers) {
  return false;
} /* CView::HandleKeyDown */

/*----------------------------------------- Update Events
 * ----------------------------------------*/

void CView::DispatchUpdate(CRect r) {
  CRect sect;

  // Exit if invisible:
  if (!visible) return;

  // Update this view first:
  HandleUpdate(r);

  // Then dispatch update to affected sub views:
  forAllChildViews(subView) if (sect.Intersect(&r, &(subView->frame))) {
    sect.Offset(-subView->frame.left,
                -subView->frame.top);  // Transform to view coordinates.
    subView->DispatchUpdate(&sect);
  }
} /* CView::DispatchUpdate */

void CView::HandleUpdate(CRect updateRect) {
  // Should be overridden by user to handle update events in this view.
} /* CView::HandleUpdate */

/*---------------------------------------- Activate Events
 * ---------------------------------------*/

void CView::DispatchActivate(BOOL activated) {
  // First handle activation in this view:
  HandleActivate(activated);
  // Then dispatch activation to all sub views:
  forAllChildViews(subView) subView->DispatchActivate(activated);
} /* CView::DispatchActivate */

void CView::HandleActivate(BOOL activated) {
  // Should be overridden by user to handle activate events in this view.
} /* CView::HandleActivate */

/*---------------------------------------- Command Events
 * ----------------------------------------*/

void CView::HandleMessage(LONG msg, LONG submsg, PTR data) {
} /* CView::HandleMessage */

/*--------------------------------------- Visibility Events
 * --------------------------------------*/

void CView::DispatchShow(void) {
  BOOL wasVisible = visible;

  visible = show;
  if (parentView && !parentView->Visible()) visible = false;

  if (visible != wasVisible) {
    forAllChildViews(subView) subView->DispatchShow();
    HandleVisChange();
  }
} /* CView::DispatchMove */

void CView::HandleVisChange(void) {} /* CView::HandleVisChange */

/*---------------------------------------- Root Port Events
 * --------------------------------------*/

void CView::DispatchRootPort(CGrafPtr newRootPort) {
  rootPort = newRootPort;

  forAllChildViews(subView) subView->DispatchRootPort(newRootPort);
} /* CView::DispatchRootPort */
