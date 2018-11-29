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

#include "DataHeaderView.h"
#include "CApplication.h"
#include "CControl.h"

static HEADER_COLUMN HCDummy[1] = {{"", 0, 0}};

#define chgSortDirWidth (controlWidth_ScrollBar - 1)

/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

// Data header views can be used both with and without a black boundary
// rectangle. The standard height of the actual interior header view is 16
// pixels.

DataHeaderView::DataHeaderView(CViewOwner *parent, CRect frame, BOOL venabled,
                               BOOL vblackFrame, INT vcolumns,
                               HEADER_COLUMN vHCTab[], INT vselected,
                               BOOL vcanResize, BOOL vchangeSortDir)
    : CView(parent, frame) {
  if (vcolumns > 0 && vHCTab) {
    columns = vcolumns;
    HCTab = vHCTab;
  } else {
    columns = 1;
    HCTab = HCDummy;
  }

  Enable(venabled && columns > 1);

  blackFrame = vblackFrame;
  selected = (vselected < 0 || vselected >= columns ? -1 : vselected);
  canResize = vcanResize;
  changeSortDir = (vchangeSortDir && !RunningOSX());
  ascendDir = true;
} /* DataHeaderView::DataHeaderView */

INT DataHeaderView_Height(BOOL hasBlackFrame) {
  return 16 + (hasBlackFrame ? 2 : 0);
} /* DataHeaderView_Height */

/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void DataHeaderView::HandleUpdate(CRect updateRect) {
  for (INT i = 0; i < columns; i++) DrawCell(i);
  DrawSortDir();
  if (blackFrame) {
    SetForeColor(RunningOSX() || !Active() ? &color_MdGray : &color_Black);
    //    SetStdForeColor();
    DrawRectFrame(bounds);

    if (RunningOSX()) {
      SetForeColor(&color_Gray);
      MovePenTo(bounds.left + 1, bounds.bottom - 1);
      DrawLineTo(bounds.right - 2, bounds.bottom - 1);
    }
  }
} /* DataHeaderView::HandleUpdate */

BOOL DataHeaderView::HandleMouseDown(CPoint pt, INT modifiers,
                                     BOOL doubleClick) {
  if (!Enabled()) return false;

  CRect r = bounds;
  if (blackFrame) r.Inset(1, 1);
  if (!pt.InRect(r)) return false;

  if (changeSortDir && pt.h >= r.right - chgSortDirWidth) {
    BOOL isPushed = true;
    DrawSortDir(true);

    MOUSE_TRACK_RESULT trackResult;
    do {
      CPoint pt2;
      TrackMouse(&pt2, &trackResult);
      if (trackResult != mouseTrack_Released && isPushed != pt2.InRect(r))
        DrawSortDir(isPushed = !isPushed);
    } while (trackResult != mouseTrack_Released);

    if (isPushed) SetSortDir(!ascendDir);
  } else {
    INT i, h = r.left + HCTab[0].width;
    for (i = 0; i < columns && pt.h > h + 5; i++)
      if (i + 1 < columns)
        h += HCTab[i + 1].width;
      else
        h = r.right;
    r.right = h;
    r.left = r.right - HCTab[i].width;

    if (Abs(pt.h - r.right) < 5 &&
        canResize)  // If clicked in resize "area" between cells
    {
      theApp->SetCursor(cursor_HResize);
      INT dh = pt.h - r.right;

      MOUSE_TRACK_RESULT trackResult;
      do {
        CPoint pt2;
        TrackMouse(&pt2, &trackResult);

        if (trackResult != mouseTrack_Released) {
          INT oldWidth = HCTab[i].width;
          INT newWidth = Max(32, pt2.h - r.left - dh);
          INT minWidth = Max(55, StrWidth(HCTab[i].text) + 30);

          if (newWidth != oldWidth && newWidth >= minWidth) {
            HCTab[i].width = newWidth;

            for (INT j = i; j < columns; j++) DrawCell(j);
            DrawSortDir();
            if (blackFrame) {
              SetStdForeColor();
              DrawRectFrame(bounds);
            }

            HandleColumnResize(i);
          }
        }
      } while (trackResult != mouseTrack_Released);

      theApp->SetCursor();
    } else  // if clicked in cell
    {
      BOOL isPushed = true;
      DrawCell(i, true);

      MOUSE_TRACK_RESULT trackResult;
      do {
        CPoint pt2;
        TrackMouse(&pt2, &trackResult);
        if (trackResult != mouseTrack_Released && isPushed != pt2.InRect(r))
          DrawCell(i, isPushed = !isPushed);
      } while (trackResult != mouseTrack_Released);

      if (isPushed) SelectCell(i);
    }
  }

  return true;
} /* DataHeaderView::HandleMouseDown */

void DataHeaderView::HandleActivate(BOOL wasActivated) {
  Redraw();
} /* DataHeaderView::HandleActivate */

void DataHeaderView::HandleSelect(INT i)  // Dummy routine. Should be overridden
{}                                        /* DataHeaderView::HandleSelect */

void DataHeaderView::HandleSortDir(
    BOOL ascend)  // Dummy routine. Should be overridden
{}                /* DataHeaderView::HandleSortDir */

void DataHeaderView::HandleColumnResize(
    INT i)  // Dummy routine. Should be overridden
{}          /* DataHeaderView::HandleColumnResize */

void DataHeaderView::HandleResize(void)  // Dummy routine. Should be overridden
{}                                       /* DataHeaderView::HandleResize */

/**************************************************************************************************/
/*                                                                                                */
/*                                             DRAWING */
/*                                                                                                */
/**************************************************************************************************/

void DataHeaderView::DrawCell(INT i, BOOL pushed) {
  if (i < 0 || i >= columns || columns <= 0) return;

  // First compute cell boundary rect:
  CRect r = bounds;
  if (changeSortDir) r.right -= chgSortDirWidth;
  if (blackFrame) r.Inset(1, 1);

  for (INT j = 0; j < i; j++) r.left += HCTab[j].width;
  if (i < columns - 1) r.right = Min(r.right, r.left + HCTab[i].width - 1);
  if (r.left >= r.right) return;

  // Then draw the various parts:

  if (RunningOSX()) {
    if (i > 0) r.left--;
    if (i < columns - 1) r.right++;
    r.bottom--;
    DrawThemeListHeaderCell(r, HCTab[i].text, HCTab[i].iconID, (selected == i),
                            pushed, ascendDir);
  } else {
    BOOL sel = (selected == i || pushed);

    Draw3DFrame(r, (sel ? &color_DkGray : &color_White),
                (sel ? &color_Gray : &color_Gray));
    r.Inset(1, 1);
    DrawRectFill(r, (sel ? &color_MdGray : &color_LtGray));
    SetForeColor(sel ? &color_White
                     : (Active() ? &color_Black : &color_MdGray));
    SetBackColor(sel ? &color_MdGray : &color_LtGray);

    if (HCTab[i].iconID > 0) {
      CRect ri(0, 0, 16, 16);
      ri.Offset(r.left, r.top - 1);
      DrawIcon(HCTab[i].iconID, ri, iconTrans_None);
      r.left += 16;
    }

    r.Inset(2, 1);
    DrawStr(HCTab[i].text, r);
    r.Inset(-3, -2);

    // Draw vertical black cell divider (unless it's the last cell):

    SetStdForeColor();
    if (i < columns - 1 || changeSortDir) {
      MovePenTo(r.right, r.top);
      DrawLineTo(r.right, r.bottom - 1);
    }
  }

  SetForeColor(&color_DkGray);
  SetBackColor(&color_LtGray);
} /* DataHeaderView::DrawCell*/

void DataHeaderView::DrawSortDir(BOOL pushed) {
  if (!changeSortDir || RunningOSX()) return;

  // Draw button background:
  CRect r = bounds;
  if (blackFrame) r.Inset(1, 1);
  r.left = r.right - chgSortDirWidth + 1;
  Draw3DFrame(r, (pushed ? &color_DkGray : &color_White),
              (pushed ? &color_Gray : &color_Gray));
  r.Inset(1, 1);
  DrawRectFill(r, (pushed ? &color_MdGray : &color_LtGray));

  // Draw the stripes:
  SetFontForeColor();
  for (INT i = 1; i <= 4; i++) {
    INT j = (ascendDir ? i : 5 - i);
    MovePenTo(r.left + chgSortDirWidth / 2 - j - 1, r.top + 2 * i + 1);
    DrawLine(2 * j - 1, 0);
  }
} /* DataHeaderView::DrawSortDir */

/**************************************************************************************************/
/*                                                                                                */
/*                                          MISCELLANEOUS */
/*                                                                                                */
/**************************************************************************************************/

void DataHeaderView::SelectCell(INT i) {
  if (i < -1 || i >= columns) return;
  if (i == selected) {
    if (!RunningOSX()) return;
    SetSortDir(!ascendDir);
    DrawCell(selected);
  } else {
    INT i0 = selected;
    selected = i;
    if (i0 != -1) DrawCell(i0);
    if (i != -1) DrawCell(i);
    if (i != i0 && i != -1) HandleSelect(i);
  }
} /* DataHeaderView::Select */

void DataHeaderView::SetSortDir(BOOL ascend) {
  BOOL wasAscend = ascendDir;
  ascendDir = ascend;
  DrawSortDir(false);
  if (wasAscend != ascendDir) HandleSortDir(ascendDir);
} /* DataHeaderView::SetSortDir */

INT DataHeaderView::Selected(void) {
  return selected;
} /* DataHeaderView::Selected */

BOOL DataHeaderView::Ascending(void) {
  return ascendDir;
} /* DataHeaderView::Ascending */

void DataHeaderView::SetCellText(INT i, CHAR *text) {
  HCTab[i].text = text;
  DrawCell(i, i == selected);
} /* DataHeaderView::SetCellText */

void DataHeaderView::SetCellWidth(INT i, INT width) {
  HCTab[i].width = width;
  Redraw();
} /* DataHeaderView::SetCellWidth */
