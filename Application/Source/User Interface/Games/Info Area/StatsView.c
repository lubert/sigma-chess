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

#include "StatsView.h"
#include "AnalysisView.h"
#include "Engine.f"
#include "GameWindow.h"

#define hOffset 54
#define vOffset1 (headerViewHeight + 1 * springHeaderLineHeight - 1)
#define vOffset2 (headerViewHeight + 2 * springHeaderLineHeight - 1)
#define vOffset3 (headerViewHeight + 3 * springHeaderLineHeight - 1)
#define vOffset4 (headerViewHeight + 4 * springHeaderLineHeight - 1)

#define digitWidth9 6

/**************************************************************************************************/
/*                                                                                                */
/*                                        CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

// The StatsView is a spring header view showing the search depth, score, node
// count e.t.c are shown.

StatsView::StatsView(CViewOwner *parent, CRect frame)
    : SpringHeaderView(parent, frame, true,
                       Prefs.GameDisplay.statsHeaderClosed) {
  win = (GameWindow *)Window();
  Analysis = &win->Analysis;

  Reset();
} /* StatsView::StatsView */

/*--------------------------------------- Event Handling
 * -----------------------------------------*/

void StatsView::HandleUpdate(CRect updateRect) {
  CRect r3D = bounds;
  r3D.Inset(-1, -1);
  SpringHeaderView::HandleUpdate(updateRect);

  DrawHeaderStr(Analysis->status);

  // If open draw additional lines
  if (!Closed()) {
    CRect r = bounds;
    r.Inset(1, 1);

    SetFontStyle(fontStyle_Bold);

    INT h = 5;
    MovePenTo(r.left + h, bounds.top + vOffset1);
    DrawStr("Engine");
    MovePenTo(r.left + h, bounds.top + vOffset2);
    DrawStr("Score");
    MovePenTo(r.left + h, bounds.top + vOffset3);
    DrawStr("Depth");
    MovePenTo(r.left + h, bounds.top + vOffset4);
    DrawStr("Current");
    h += r.Width() / 2;
    MovePenTo(r.left + h, bounds.top + vOffset2);
    DrawStr("Nodes");
    MovePenTo(r.left + h, bounds.top + vOffset3);
    DrawStr("N/sec");
    MovePenTo(r.left + h, bounds.top + vOffset4);
    DrawStr("Hash %");

    SetFontStyle(fontStyle_Plain);

    DrawEngineName();
    DrawScore();
    DrawDepthCurrent();
    DrawNodes();
  }
} /* StatsView::HandleUpdate */

void StatsView::HandleActivate(BOOL wasActivated) {
  Redraw();
} /* StatsView::HandleActivate */

void StatsView::HandleToggle(BOOL closed) {
  Prefs.GameDisplay.statsHeaderClosed = closed;
  ((AnalysisView *)Parent())->ToggleStatsHeader(closed);
} /* StatsView::HandleToggle */

/*----------------------------------------- Setting Stats
 * ----------------------------------------*/

void StatsView::Reset(void) {
  SetStatus("Idle");
  DrawEngineName();
  SetScore(0, scoreType_Unknown, 1);
  SetMainDepth(0, 1);
  SetCurrent();
  SetNodes(0, 0, 0, 0);
} /* StatsView::Reset */

void StatsView::SetStatus(CHAR *newStatus) {
  CopyStr(newStatus, Analysis->status);
  DrawHeaderStr(Analysis->status);
} /* StatsView::SetStatus */

void StatsView::SetScore(INT newScore, INT scoreType, INT pvNo) {
  Analysis->score[pvNo] = newScore;
  Analysis->scoreType[pvNo] = scoreType;
  DrawScore();
} /* StatsView::SetScore */

void StatsView::SetMainDepth(INT newDepth, INT pvNo) {
  if (pvNo == 1) {
    Analysis->depth = newDepth;
    DrawDepthCurrent();
  }
} /* StatsView::SetMainDepth */

void StatsView::SetCurrent(void) {
  DrawDepthCurrent();
} /* StatsView::SetCurrent */

void StatsView::SetNodes(LONG64 newNodes, ULONG searchTime, ULONG nps,
                         INT hashFull) {
  Analysis->nodes = newNodes;
  Analysis->searchTime = searchTime;
  Analysis->nps =
      (nps > 0 ? nps
               : (60.0 * Analysis->nodes) / MaxL(1, Analysis->searchTime));
  Analysis->hashFull = hashFull;
  DrawNodes();
} /* StatsView::SetNodes */

/*----------------------------------------- Drawing Stats
 * ----------------------------------------*/

void StatsView::DrawEngineName(void) {
  if (!Visible() || Closed()) return;

  MovePenTo(bounds.left + hOffset, bounds.top + vOffset1);
  DrawStr(((GameWindow *)Window())->engineName);
  TextEraseTo(bounds.right - 10);
} /* StatsView::DrawEngineName */

void StatsView::DrawScore(void) {
  if (!Visible() || Closed()) return;

  MovePenTo(bounds.left + hOffset, bounds.top + vOffset2);

  if (((GameWindow *)Window())->isRated) {
    DrawStr("<hidden>");
    TextEraseTo(bounds.left + 105);
  } else if (Prefs.AnalysisFormat.scoreNot == scoreNot_Glyph &&
             Analysis->scoreType[1] == scoreType_True &&
             Analysis->score[1] > mateLoseVal &&
             Analysis->score[1] < mateWinVal) {
    TextEraseTo(bounds.left + 105);

    CRect r(0, 0, 16, 16);
    r.Offset(bounds.left + hOffset - 4, bounds.top + vOffset2 - 11);

    INT score = CheckAbsScore(Analysis->player, Analysis->score[1]);
    INT p = Analysis->player;
    LIB_CLASS c;
    if (score >= 150)
      c = libClass_WinningAdvW, r.Offset(1, 0);
    else if (score >= 50)
      c = libClass_ClearAdvW;
    else if (score >= 25)
      c = libClass_SlightAdvW;
    else if (score > -25)
      c = libClass_Level;
    else if (score > -50)
      c = libClass_SlightAdvB;
    else if (score > -150)
      c = libClass_ClearAdvB;
    else
      c = libClass_WinningAdvB, r.Offset(1, 0);

    DrawIcon(369 + c, r, (Active() ? iconTrans_None : iconTrans_Disabled));
  } else {
    CHAR s[20];
    ::CalcScoreStr(s, CheckAbsScore(Analysis->player, Analysis->score[1]),
                   Analysis->scoreType[1]);
    DrawStr(s);
    TextEraseTo(bounds.left + 105);
  }
} /* StatsView::DrawScore */

void StatsView::DrawDepthCurrent(void) {
  if (!Visible() || Closed()) return;

  CHAR s[20];
  INT i = 0;
  INT depth = Analysis->depth;
  INT rootMoves = Analysis->numRootMoves;
  INT current = Min(rootMoves, Analysis->current);

  if (depth == 0) {
    s[i++] = '-';
  } else {
    if (depth < 10)
      s[i++] = depth + '0';
    else
      s[i++] = depth / 10 + '0', s[i++] = depth % 10 + '0';

    s[i++] = ' ';
    s[i++] = ':';
    s[i++] = ' ';

    if (current < 10)
      s[i++] = current + '0';
    else if (current < 100)
      s[i++] = current / 10 + '0', s[i++] = current % 10 + '0';
    s[i++] = '/';

    if (rootMoves < 10)
      s[i++] = rootMoves + '0';
    else if (rootMoves < 100)
      s[i++] = rootMoves / 10 + '0', s[i++] = rootMoves % 10 + '0';
  }

  s[i++] = 0;

  MovePenTo(bounds.left + hOffset, bounds.top + vOffset3);
  DrawStr(s);
  TextEraseTo(bounds.left + bounds.Width() / 2 - 1);

  // Draw current move
  MovePenTo(bounds.left + hOffset, bounds.top + vOffset4);
  if (isNull(Analysis->currMove))
    DrawStr("-");
  else
    ::DrawGameMove(this, &Analysis->currMove, false);
  TextEraseTo(bounds.left + bounds.Width() / 2 - 1);

} /* StatsView::DrawDepthCurrent */

void StatsView::DrawNodes(void) {
  if (!Visible() || Closed()) return;

  MovePenTo(bounds.left + bounds.Width() / 2 + hOffset, bounds.top + vOffset2);

  if (Analysis->nodes < 1000000000.0) {
    DrawNumR(Analysis->nodes, 10);
    if (Analysis->nodes == 0) TextErase(digitWidth9);
  } else {
    DrawNumR(Analysis->nodes / 1000.0, 10);
    DrawChr('K');
  }

  MovePenTo(bounds.left + bounds.Width() / 2 + hOffset + 3 * digitWidth9,
            bounds.top + vOffset3);
  DrawNumR(Analysis->nps, 7);

  if (Analysis->hashFull > 0) {
    MovePenTo(bounds.right - 51, bounds.top + vOffset4);
    CHAR s[10];
    Format(s, "%d.%d", Analysis->hashFull / 10, Analysis->hashFull % 10);
    MovePen(33 - StrWidth(s), 0);
    DrawStr(s);
  } else if (Analysis->nps == 0) {
    MovePenTo(bounds.right - 60, bounds.top + vOffset4);
    TextEraseTo(bounds.right - 15);
  }
} /* StatsView::DrawNodes */
