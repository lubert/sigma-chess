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

#pragma once

#include "CFont.h"
#include "CUtility.h"
#include "CViewOwner.h"
#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

enum BMP_MODE { bmpMode_Trans = transparent, bmpMode_Copy = srcCopy };

enum ICON_TRANS {
  iconTrans_None = kTransformNone,
  iconTrans_Disabled = kTransformDisabled,
  iconTrans_Selected = kTransformSelected
};

enum TEXT_ALIGNMENT {
  textAlign_Left = teJustLeft,
  textAlign_Center = teJustCenter,
  textAlign_Right = teJustRight
};

enum TAB_DIR {
  tabDir_North = kThemeTabNorth,
  tabDir_South = kThemeTabSouth,
  tabDir_East = kThemeTabEast,
  tabDir_West = kThemeTabWest
};

enum MOUSE_TRACK_RESULT {
  mouseTrack_Pressed = kMouseTrackingMousePressed,
  mouseTrack_Released = kMouseTrackingMouseReleased,
  mouseTrack_Exited = kMouseTrackingMouseExited,
  mouseTrack_Entered = kMouseTrackingMouseEntered,
  mouseTrack_Moved = kMouseTrackingMouseMoved
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef struct {
  RGBColor rgbFgColor;
  RGBColor rgbBkColor;
  Point pnLoc;
  Point pnSize;
  INT pnMode;
  INT txFont;
  Style txFace;
  INT txMode;
  INT txSize;
} DrawEnv;

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class CWindow;
class CBitmap;

class CView : public CViewOwner {
 public:
  CView(CViewOwner *parent, CRect frame);
  virtual ~CView(void);

  CWindow *Window(void);
  CView *Parent(void);

  void Show(BOOL show, BOOL redraw = false);
  void Enable(BOOL enable, BOOL dispatch = true);
  BOOL Active(void);
  BOOL Enabled(void);
  BOOL Visible(void);

  void Redraw(BOOL flush = false);
  void Invalidate(void);
  void FlushPortBuffer(CRect *r = nil);

  BOOL GetMouseLoc(CPoint *p);
  BOOL TrackMouse(CPoint *p, MOUSE_TRACK_RESULT *result);
  // BOOL MouseButtonDown (void);
  void SetFrame(CRect frame, BOOL update = false);
  void SetBounds(CRect r);
  void SetBounds(INT h, INT v);

  // Drawing routines:
  void GetForeColor(RGB_COLOR *c);
  void GetBackColor(RGB_COLOR *c);
  void SetForeColor(RGB_COLOR *c);
  void SetBackColor(RGB_COLOR *c);
  void SetForeColor(INT red, INT green, INT blue);
  void SetBackColor(INT red, INT green, INT blue);
  void SetStdForeColor(void);
  void SetStdBackColor(void);
  void SetFontForeColor(void);
  void GetHiliteColor(RGB_COLOR *c);

  void SetFontSize(INT size);
  void SetFontFace(FONT_FACE font);
  void SetFontStyle(INT style);
  void SetFontMode(INT mode);
  void SetDefaultFont(void);
  void SetThemeFont(INT themeFontId);
  // void SetFont (CFont *font);
  // void GetFont (CFont *font);

  void GetPenPos(INT *h, INT *v);
  void SetPenSize(INT h, INT v);
  void GetPenSize(INT *h, INT *v);

  void MovePen(INT dh, INT dv);
  void MovePenTo(INT h, INT v);
  void DrawLine(INT dh, INT dv);
  void DrawLineTo(INT h, INT v);
  void DrawPoint(INT h, INT v, RGB_COLOR *color = nil);
  void DrawRectFrame(CRect r);
  void DrawRoundRectFrame(CRect r, INT width, INT height);
  void DrawRectErase(CRect r);
  void DrawThemeBackground(CRect r);
  void DrawRectFill(CRect r, RGB_COLOR *c);
  void DrawRectFill(CRect r, INT red, INT green, INT blue);
  void DrawRectFill(CRect r, INT patternID);
  void Draw3DFrame(CRect r, RGB_COLOR *topLeft, RGB_COLOR *bottomRight);
  void Draw3DFrame(CRect r, RGB_COLOR *baseColor, INT topLeftAdj,
                   INT bottomRightAdj);
  void DrawStripeRect(CRect r, INT voffset = 0);
  void DrawFocusRect(CRect r, BOOL hasFocus);
  void DrawEditFrame(CRect r);
  void DrawOvalFrame(CRect r);
  void DrawOvalFill(CRect r, RGB_COLOR *c);
  void DrawStr(CHAR *s);
  void DrawStr(CHAR *s, INT maxWidth);
  void DrawStr(CHAR *s, CRect r, TEXT_ALIGNMENT align = textAlign_Left,
               BOOL lineWrap = true, BOOL erase = true);
  void DrawStr(CHAR *s, INT pos, INT count);
  void DrawChr(CHAR c);
  void DrawNum(LONG n);
  void DrawNumR(LONG n, INT minDigits, BOOL preErase = true);
  void DrawNumR2(LONG n, INT fieldWidth);
  void DrawBitmap(CBitmap *srcMap, CRect srcRect, CRect dstRect,
                  BMP_MODE mode = bmpMode_Copy);
  void DrawPict(INT picID, CRect rect);
  void DrawPict(INT picID, INT left, INT top);
  void DrawIcon(INT iconID, CRect rect, ICON_TRANS trans = iconTrans_None);
  void TextErase(INT pixels);
  void TextEraseTo(INT h);
  void SetTextSpacing(INT n, INT d);
  void ResetTextSpacing(void);

  void DrawThemeFocusRectFrame(CRect r, BOOL hasFocus = true);
  void DrawThemeListHeaderCell(CRect r, CHAR *title, INT iconID = 0,
                               BOOL selected = false, BOOL pushed = false,
                               BOOL ascendDir = true);
  void DrawThemeTab(CRect r, TAB_DIR dir, BOOL front, BOOL pushed);

  void GetPixelColor(INT h, INT v, RGB_COLOR *pixelColor);

  void SetClip(CRect r);
  void ClrClip(void);

  INT StrWidth(CHAR *s);
  INT StrWidth(CHAR *s, INT pos, INT count);
  INT ChrWidth(CHAR c);
  INT FontAscent(void);
  INT FontDescent(void);
  INT FontLineSpacing(void);
  INT FontHeight(void);
  INT FontMaxChrWidth(void);

  // Events (should normally not be overridden):
  void DispatchActivate(BOOL wasActivated);
  void DispatchUpdate(CRect r);
  BOOL DispatchMouseDown(CPoint pt, INT modifiers, BOOL doubleClick = false);
  void DispatchMove(INT dh, INT dv);
  void DispatchShow(void);
  void DispatchRootPort(CGrafPtr newRootPort);

  // Events (should be overridden):
  virtual void HandleActivate(BOOL wasActivated);
  virtual void HandleUpdate(CRect updateRect);
  virtual BOOL HandleMouseDown(CPoint pt, INT modifiers, BOOL doubleClick);
  virtual BOOL HandleKeyDown(CHAR c, INT key, INT modifiers);
  virtual void HandleMessage(LONG msg, LONG submsg = 0, PTR data = nil);
  virtual void HandleResize(void);
  virtual void HandleMove(void);
  virtual void HandleVisChange(
      void);  // When effective visibility has been changed.

  CRect frame;    // View rectangle in parent coordinates.
  CRect bounds;   // Local (0,0) rectangle.
  CPoint origin;  // Origin of bounds rect in root window coordinates.

  BOOL visible;  // Effective visibility (show && parent->visible)

  void SavePort(void);  //### should be private
  void RestorePort(void);

 private:
  CView *parentView;  // Parent view (NULL if topView).
  CWindow *window;    // The window to which the view belongs (NULL if bitmap).
  CBitmap *bitmap;    // The bitmap to which the view belongs (NULL if window).

  BOOL show;  // Logical visibility (set by Show()).

  BOOL enabled;

  CGrafPtr rootPort;  // Low level Graphics port of parent window.
  DrawEnv env;        // Current view drawing environment.
  DrawEnv saveEnv;    // Saved copy of windows port env when drawing.
  CGrafPtr oldPort;
  GDHandle oldDevice;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

extern RGB_COLOR color_White, color_Black;
extern RGB_COLOR color_ClGray, color_DkGray, color_MdGray, color_Gray,
    color_BtGray, color_LtGray, color_BrGray;
extern RGB_COLOR color_Red, color_Green, color_Blue, color_Yellow, color_Cyan,
    color_Magenta;
extern RGB_COLOR color_Dialog;

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/
