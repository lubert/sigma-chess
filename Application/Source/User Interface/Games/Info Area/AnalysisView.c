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

#include "AnalysisView.h"
#include "InfoArea.h"
#include "GameWindow.h"
#include "EngineMatchDialog.h"

/*---------------------------------------- Sub Views ---------------------------------------------*/

class MultiPVTextView;

class AnaToolbar : public CToolbar
{
public:
   AnaToolbar (CViewOwner *parent, CRect frame);

   void Adjust (void);

private:
   CButton *tb_EngineMgr;
   CButton *tb_VerticalPV;
   CButton *tb_HorizontalPV;
   MultiPVTextView *cv_MultiPVText;
   CButton *tb_IncMultiPV;
   CButton *tb_DecMultiPV;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         ANALYSIS VIEW                                          */
/*                                                                                                */
/**************************************************************************************************/

// The AnalysisView is mainly a "container" view that includes the Statistics view (a spring header
// view) and the Variation view (showing the main line and current move/line), where the 
// main and current lines of analysis are shown.

AnalysisView::AnalysisView (CViewOwner *parent, CRect frame)
 : BackView(parent, frame, false)
{
   win = (GameWindow*)(Window());
   Analysis = &(win->Analysis);

   restoreHeight = bounds.Height();

   CRect statsRect, varRect, toolbarRect;
   CalcRects(Prefs.GameDisplay.statsHeaderClosed, &statsRect, &varRect, &toolbarRect);
   statsView = new StatsView(this, statsRect);
   varView   = new VariationView(this, varRect);
   toolbar   = new AnaToolbar(this, toolbarRect);

   Reset();
} /* AnalysisView::AnalysisView */


void AnalysisView::CalcRects (BOOL statsClosed, CRect *statsRect, CRect *varRect, CRect *toolbarRect)
{
   CRect rs = DataViewRect();
   rs.bottom = rs.top + headerViewHeight + (statsClosed ? 0 : 4*statsHeaderLineHeight + 5) + 1;
// rs.bottom = rs.top + headerViewHeight + (statsClosed ? 0 : 3*statsHeaderLineHeight + 5) + 1;
   if (statsRect) *statsRect = rs;

   CRect rv = DataViewRect();
   rv.top = rs.bottom - 1; // + 8;  // V6.1.4
   ExcludeRect(rv);

   CRect rt = rv;
   rt.Inset(1, 1);
   rv.bottom -= toolbar_HeightSmall;
   rt.top = rv.bottom;
   
   if (varRect) *varRect = rv;
   if (toolbarRect) *toolbarRect = rt;
} /* AnalysisView::CalcRects */

/*----------------------------------------- Event Handling ---------------------------------------*/

void AnalysisView::HandleUpdate (CRect updateRect)
{
   BackView::HandleUpdate(updateRect);
   DrawBottomRound();

   SetForeColor(RunningOSX() || ! Active() ? &color_MdGray : &color_Black);
   DrawRectFrame(DataViewRect());
} /* AnalysisView::HandleUpdate */


void AnalysisView::HandleResize (void)
{
   CRect varRect, toolbarRect;
   CalcRects(statsView->Closed(), nil, &varRect, &toolbarRect);
   varView->SetFrame(varRect);
   toolbar->SetFrame(toolbarRect);
} /* AnalysisView::HandleResize */


void AnalysisView::Refresh (void)
{
	statsView->Redraw();
   varView->CalcCoord();
   varView->Redraw();
   toolbar->Adjust();
} /* AnalysisView::Refresh */


void AnalysisView::AdjustToolbar (void)
{
   toolbar->Adjust();
} // AnalysisView::AdjustToolbar


void AnalysisView::ToggleStatsHeader (BOOL closed)
{
   CRect statsRect, varRect;
   CalcRects(statsView->Closed(), &statsRect, &varRect, nil);
   statsView->SetFrame(statsRect);
   varView->SetFrame(varRect);

   Redraw();
   ((InfoAreaView*)Parent())->DrawDivider();
} /* AnalysisView::ToggleStatsHeader */

/*------------------------------------ Setting Stats/Variations ----------------------------------*/

void AnalysisView::Reset (void)
{
   Analysis->initPlayer   = win->game->Init.player;
   Analysis->initMoveNo   = win->game->Init.moveNo;
   Analysis->player       = win->game->player;
   Analysis->gameMove     = win->game->currMove;
   Analysis->numRootMoves = win->game->moveCount;
   Analysis->searchTime   = 0;

   Analysis->current      = 0;
   clrMove(Analysis->currMove);

   for (INT i = 1; i <= uci_MaxMultiPVcount; i++)
   {  clrMove(Analysis->PV[i][0]);
      Analysis->depthPV[i] = 0;
   }

   statsView->Reset();
   varView->Reset();
} /* AnalysisView::Reset */


void AnalysisView::SetStatus (CHAR *statusStr)
{
   statsView->SetStatus(statusStr);
} /* AnalysisView::SetStatus */


void AnalysisView::SetScore (INT score, INT scoreType, INT pvNo)
{
   statsView->SetScore(score, scoreType, pvNo);
} /* AnalysisView::SetScore */


void AnalysisView::SetMainDepth (INT depth, INT pvNo)
{
   statsView->SetMainDepth(depth, pvNo);
} /* AnalysisView::SetMainDepth */


void AnalysisView::SetCurrent (INT current, MOVE *m)
{
   Analysis = &win->Analysis;

   Analysis->current  = current;
   Analysis->currMove = *m;
   CalcMoveFlags(win->engine->P.Board, &Analysis->currMove);

   statsView->SetCurrent();
} /* AnalysisView::SetCurrent */


void AnalysisView::SetNodes (LONG64 nodes, ULONG searchTime, ULONG nps, INT hashFull)
{
   statsView->SetNodes(nodes, searchTime, nps, hashFull);
} /* AnalysisView::SetNodes */


void AnalysisView::SetMainLine (MOVE M[], INT depth, INT pvNo)
{
   if (pvNo < 1 || pvNo > uci_MaxMultiPVcount || pvNo > win->GetMultiPVcount()) return;

   Analysis->depthPV[pvNo] = depth;

   MOVE *PV = Analysis->PV[pvNo];

   INT i = 0;
   do
      PV[i] = M[i];
   while (! isNull(M[i++]));

   CalcVariationFlags(win->engine->P.Board, PV);

   varView->SetMainLine(pvNo);
} /* AnalysisView::SetMainLine */


/**************************************************************************************************/
/*                                                                                                */
/*                                           TOOLBAR                                              */
/*                                                                                                */
/**************************************************************************************************/

class MultiPVTextView : public CToolbarTextView
{
public:
   MultiPVTextView (CViewOwner *parent, CRect frame);
   virtual void HandleUpdate (CRect updateRect);
};


AnaToolbar::AnaToolbar (CViewOwner *parent, CRect frame)
 : CToolbar(parent, frame)
{
   if (UCI_Enabled())
   {  tb_EngineMgr = AddButton(engine_Configure, icon_EngineMgr, 16, 16, "", "Engine Manager");
      AddSeparator();
   }
   tb_VerticalPV   = AddButton(display_VerPV, icon_VerPV, 16, 16, "", "Vertical PV Display");
   tb_HorizontalPV = AddButton(display_HorPV, icon_HorPV, 16, 16, "", "Horizontal PV Display");
   AddSeparator();
   tb_IncMultiPV = AddButton(display_IncMultiPV, icon_Plus, 16, 16, "", "Add Multi PV line.");
   tb_DecMultiPV = AddButton(display_DecMultiPV, icon_Minus, 16, 16, "", "Remove Multi PV Line.");
   AddCustomView(cv_MultiPVText = new MultiPVTextView(this, NextItemRect(70)));
   Adjust();
} // GameToolbar::AnaToolbar


void AnaToolbar::Adjust (void)
{
   GameWindow *win = (GameWindow*)(Window());

   if (UCI_Enabled())
      tb_EngineMgr->Enable(! EngineMatch.gameWin);

   tb_VerticalPV->Press(win->varDisplayVer);
   tb_HorizontalPV->Press(! win->varDisplayVer);

   tb_IncMultiPV->Enable(win->GetMultiPVcount() < win->GetMaxMultiPVcount());
   tb_DecMultiPV->Enable(win->GetMultiPVcount() > 1);

   cv_MultiPVText->Redraw();
} // AnaToolbar::Adjust

/*------------------------------------ Lib toolbar Text View -------------------------------------*/

MultiPVTextView::MultiPVTextView (CViewOwner *parent, CRect frame)
 : CToolbarTextView(parent, frame)
{
} /* LibTextView::LibTextView */


void MultiPVTextView::HandleUpdate (CRect updateRect)
{
   CToolbarTextView::HandleUpdate(updateRect);

   GameWindow *win = (GameWindow*)Window();
	CHAR s[100];
	INT count = win->GetMultiPVcount();
	if (count == 1)
	   Format(s, "Single PV");
	else
	   Format(s, "Multi PV [%d]", count);

	MovePen(3, 0);
	DrawStr(s);
} /* MultiPVTextView::HandleUpdate */
