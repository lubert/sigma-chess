/**************************************************************************************************/
/*                                                                                                */
/* Module  : InfoArea.c                                                                           */
/* Purpose : This module defines the "Info Area" container view.                                  */
/*                                                                                                */
/**************************************************************************************************/

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

#include "InfoArea.h"
#include "GameWindow.h"


class InfoDividerView : public CView
{
public:
   InfoDividerView (CViewOwner *parent, CRect frame);

   virtual void HandleUpdate (CRect updateRect);
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);

private:
   void DrawDivider (void);
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         DIMENSIONS                                             */
/*                                                                                                */
/**************************************************************************************************/

#define dividerViewHeight    8  //10
//#define dividerButtonWidth   17


INT InfoArea_Width (void)
{
   return 8 + 264 + 8;
//   return 8 + 245 + 8;
} /* InfoArea_Width */


/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

// The "Info View" merely acts as a container for the various info sub areas. Therefore
// the "Info View doesn't draw anything by itself.

InfoAreaView::InfoAreaView (CViewOwner *parent, CRect frame)
 : BackView(parent,frame)
{
   showAnalysis  = Prefs.GameDisplay.showAnalysis;
   showAnnEditor = false;
   showLibEditor = false;
   dividerPos    = Prefs.GameDisplay.dividerPos;

   CRect dividerRect, upperRect, lowerRect;
   CalcFrames(&dividerRect, &upperRect, &lowerRect);
   gameView      = new GameView(this, upperRect);
   analysisView  = new AnalysisView(this, lowerRect);
   annEditorView = new AnnEditorView(this, lowerRect);
   libEditorView = new LibEditorView(this, lowerRect);
   dividerView   = new InfoDividerView(this, dividerRect); // Must be allocated last so it's on top!

   analysisView->Show(showAnalysis);
   annEditorView->Show(false);
   libEditorView->Show(false);
   dividerView->Show(showAnalysis);

   // The optional position editor view is not created initially.
   posEditorView = nil;
} /* InfoAreaView::InfoAreaView */


/**************************************************************************************************/
/*                                                                                                */
/*                                    CALC SUBVIEW RECTANGLES                                     */
/*                                                                                                */
/**************************************************************************************************/

// The game view is always located at the top, followed by either the analysis view, the
// annotation editor view or the library editor view. If the position editor is open, the other
// views are hidden.

void InfoAreaView::CalcFrames (CRect *divider, CRect *upper, CRect *lower)
{
   // Confine divider pos, so the minimum heights of the upper and lower rects are respected.
   if (dividerPos - bounds.top < minGameViewHeight)
      dividerPos = bounds.top + minGameViewHeight;
   else if (bounds.bottom - dividerPos < minAnalysisViewHeight)
      dividerPos = bounds.bottom - minAnalysisViewHeight;

   // The resizerPos normally determines the size of the various subviews. The exception is
   // when only the move list is shown (in which case the resizerPos is ignored).
   INT gameViewHeight = (showAnalysis || showAnnEditor || showLibEditor ? dividerPos : bounds.Height());

   *upper = bounds;
   *lower = bounds;
   upper->bottom = upper->top + gameViewHeight;

   lower->top += dividerPos - dividerViewHeight;

   divider->Set(upper->left, upper->bottom - dividerViewHeight, upper->right, upper->bottom); // + dividerViewHeight/2);
} /* InfoAreaView::CalcFrames */


void InfoAreaView::SetDividerPos (INT newDividerPos)
{
   Prefs.GameDisplay.dividerPos = dividerPos = newDividerPos;

   CRect dividerRect, upperRect, lowerRect;
   CalcFrames(&dividerRect, &upperRect, &lowerRect);

   gameView->SetFrame(upperRect);
   if (showAnalysis)  analysisView->SetFrame(lowerRect, true);
   if (showAnnEditor) annEditorView->SetFrame(lowerRect, true);
   if (showLibEditor) libEditorView->SetFrame(lowerRect, true);
   dividerView->SetFrame(dividerRect, true);

   // Fix scrollbar redraw bug!
   CRect r = bounds;
   SetForeColor(&sigmaPrefs->darkColor);
   MovePenTo(r.left + 1, r.bottom - 1); DrawLineTo(r.right - 2, r.bottom - 1);
   RGB_COLOR c = sigmaPrefs->mainColor;
   AdjustColorLightness(&c, -10);
   SetForeColor(&c);
   MovePenTo(r.left + 2, r.bottom - 2); DrawLineTo(r.right - 3, r.bottom - 2);
} /* InfoAreaView::SetDividerPos */


void InfoAreaView::DrawDivider (void)
{
   dividerView->Redraw();
} /* InfoAreaView::DrawDivider */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

void InfoAreaView::HandleUpdate (CRect updateRect)
{
//   BackView::HandleUpdate(updateRect);
//   DrawTopRound();
//   DrawBottomRound();
 
   CRect r = bounds;
   r.Inset(-1, -1);
   Outline3DRect(r, false);
} /* InfoAreaView::HandleUpdate */


void InfoAreaView::HandleResize (void)
{
   SetDividerPos(dividerPos);
   if (posEditorView)
      posEditorView->SetFrame(bounds); 
} /* InfoAreaView::HandleResize */


BOOL InfoAreaView::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   if (annEditorView->HandleKeyDown(c, key, modifiers)) return true;
   if (posEditorView) return posEditorView->HandleKeyDown(c, key, modifiers);
   return gameView->HandleKeyDown(c, key, modifiers);
} /* InfoAreaView::HandleKeyDown */


void InfoAreaView::UpdateGameList (void)
{
   gameView->UpdateGameList();
   libEditorView->Refresh();
} /* InfoAreaView::UpdateGameList */


void InfoAreaView::RedrawGameList (void)
{
   gameView->RedrawGameList();
} /* InfoAreaView::RedrawGameList */


void InfoAreaView::RefreshPieceSet (void)
{
   if (posEditorView)
      posEditorView->RefreshPieceSet();
} /* InfoAreaView::RefreshPieceSet */


void InfoAreaView::RefreshNotation (void)
{
   gameView->RedrawGameList();
   analysisView->Refresh();
// if (annEditorView)
//    annEditorView->RefreshNotation();
} /* InfoAreaView::RefreshNotation */


void InfoAreaView::FlushAnnotation (void)
{
   if (annEditorView)
      annEditorView->Flush();
} /* InfoAreaView::FlushAnnotation */


void InfoAreaView::LoadAnnotation (void)
{
   if (annEditorView)
      annEditorView->Load();
} /* InfoAreaView::LoadAnnotation */


BOOL InfoAreaView::AdjustAnnEditor (CEditor *ctrl, BOOL textChanged, BOOL selChanged)
{
   if (! annEditorView || ! showAnnEditor || ctrl != annEditorView->editor) return false;
   annEditorView->AdjustToolbar();
   return true;
} /* InfoAreaView::AdjustAnnEditor */


void InfoAreaView::AdjustAnnGlyph (void)
{
   annEditorView->AdjustToolbar();
} /* InfoAreaView::AdjustAnnGlyph */


void InfoAreaView::Show (BOOL show)
{
   CView::Show(show);

   gameView->Show(show);
   analysisView->Show(show && showAnalysis);
   annEditorView->Show(show && showAnnEditor);
   libEditorView->Show(show && showLibEditor);
   dividerView->Show(show && (showAnalysis || showAnnEditor || showLibEditor));
} /* InfoAreaView::Show */


/**************************************************************************************************/
/*                                                                                                */
/*                                        MANAGE SUBVIEWS                                         */
/*                                                                                                */
/**************************************************************************************************/

void InfoAreaView::ShowAnalysis (BOOL show)
{
   showAnalysis = show;
   analysisView->Show(showAnalysis);
   dividerView->Show(showAnalysis || showAnnEditor || showLibEditor);

   CRect dividerRect, upperRect, lowerRect;
   CalcFrames(&dividerRect, &upperRect, &lowerRect);
   gameView->SetFrame(upperRect, true);

   if (showAnalysis)  analysisView->SetFrame(lowerRect, true);
   if (showAnnEditor) annEditorView->SetFrame(lowerRect, true);
   dividerView->SetFrame(dividerRect, true);

   ((GameWindow*)(Window()))->tabAreaView->Redraw();
} /* InfoAreaView::ShowAnalysis */


void InfoAreaView::ShowAnnEditor (BOOL show)
{
   showAnnEditor = show;
   analysisView->Show(! showAnnEditor && showAnalysis);
   annEditorView->Show(showAnnEditor);
   dividerView->Show(showAnalysis || showAnnEditor || showLibEditor);

   CRect dividerRect, upperRect, lowerRect;
   CalcFrames(&dividerRect, &upperRect, &lowerRect);
   gameView->SetFrame(upperRect, true);

   if (showAnalysis)  analysisView->SetFrame(lowerRect, true);
   if (showAnnEditor) annEditorView->SetFrame(lowerRect, true);
   dividerView->SetFrame(dividerRect, true);

   if (showAnnEditor)
      annEditorView->Load();
   else
   {  annEditorView->Flush();
      UpdateGameList();
   }

   ((GameWindow*)(Window()))->tabAreaView->Redraw();
} /* InfoAreaView::ShowAnnEditor */


void InfoAreaView::ShowLibEditor (BOOL show)
{
   showLibEditor = show;
   analysisView->Show(! showLibEditor && showAnalysis);
   libEditorView->Show(showLibEditor);
   dividerView->Show(showAnalysis || showAnnEditor || showLibEditor);

   CRect dividerRect, upperRect, lowerRect;
   CalcFrames(&dividerRect, &upperRect, &lowerRect);
   gameView->SetFrame(upperRect, true);

   if (showAnalysis)  analysisView->SetFrame(lowerRect);
   if (showLibEditor) libEditorView->SetFrame(lowerRect);
   dividerView->SetFrame(dividerRect);

   if (showLibEditor)
      libEditorView->HandleOpen();

   Redraw();

   ((GameWindow*)(Window()))->tabAreaView->Redraw();
} /* InfoAreaView::ShowLibEditor */


void InfoAreaView::ShowPosEditor (BOOL showPos)
{
   if (! showPos)
   {  delete posEditorView;
      posEditorView = nil;
   }
   
   gameView->Show(! showPos);
   analysisView->Show(! showPos && showAnalysis);
   annEditorView->Show(! showPos && showAnnEditor);
   libEditorView->Show(! showPos && showLibEditor);
   dividerView->Show(! showPos && (showAnalysis || showAnnEditor || showLibEditor));

   if (showPos)
   {  posEditorView = new PosEditorView(this, bounds);
   }

   Redraw();

   ((GameWindow*)(Window()))->tabAreaView->Redraw();
} /* InfoAreaView::ShowPosEditor */


/**************************************************************************************************/
/*                                                                                                */
/*                                    SET SEARCH RESULTS/STATISTICS                               */
/*                                                                                                */
/**************************************************************************************************/

void InfoAreaView::RefreshGameStatus (void)
{
   gameView->RefreshGameStatus();
} /* InfoAreaView::UpdateGameStatus */


void InfoAreaView::RefreshGameInfo (void)
{
   gameView->RefreshGameInfo();
} /* InfoAreaView::RefreshGameInfo */


void InfoAreaView::RefreshAnalysis (void)
{
   analysisView->Refresh();
} /* InfoAreaView::RefreshAnalysis */


void InfoAreaView::AdjustAnalysisToolbar (void)
{
   analysisView->AdjustToolbar();
} /* InfoAreaView::AdjustAnalysisToolbar */


void InfoAreaView::RefreshLibInfo (void)
{
   gameView->RefreshLibInfo();
   libEditorView->Refresh();
} /* InfoAreaView::RefreshLibInfo */


void InfoAreaView::ResizeGameHeader (void)
{
   gameView->ResizeHeader();
} /* InfoAreaView::ResizeGameHeader */


void InfoAreaView::ResetAnalysis (void)
{
   analysisView->Reset();
} /* InfoAreaView::ResetAnalysis */


void InfoAreaView::SetAnalysisStatus (CHAR *statusStr, BOOL flushPortBuffer)
{
   analysisView->SetStatus(statusStr);
   if (flushPortBuffer)
      FlushPortBuffer();
} /* InfoAreaView::SetAnalysisStatus */


void InfoAreaView::SetMainDepth (INT depth, INT pvNo)
{
   analysisView->SetMainDepth(depth, pvNo);
} /* InfoAreaView::SetMainDepth */


void InfoAreaView::SetMainLine (MOVE MainLine[], INT depth, INT pvNo)
{
   analysisView->SetMainLine(MainLine, depth, pvNo);
} /* InfoAreaView::SetMainLine */


void InfoAreaView::SetScore (INT score, INT scoreType, INT pvNo)
{
   analysisView->SetScore(score, scoreType, pvNo);
} /* InfoAreaView::SetScore */


void InfoAreaView::SetCurrent (INT n, MOVE *m)
{
   analysisView->SetCurrent(n, m);
} /* InfoAreaView::SetCurrent */


void InfoAreaView::SetNodes (LONG64 nodes, ULONG searchTime, ULONG nps, INT hashFull)
{
   analysisView->SetNodes(nodes, searchTime, nps, hashFull);
} /* InfoAreaView::SetNodes */


/**************************************************************************************************/
/*                                                                                                */
/*                                           INFO SLIDER                                          */ 
/*                                                                                                */
/**************************************************************************************************/

InfoDividerView::InfoDividerView (CViewOwner *parent, CRect frame)
 : CView(parent, frame)
{
} /* InfoDividerView::InfoDividerView */


void InfoDividerView::HandleUpdate (CRect updateRect)
{
   CRect r = bounds; r.Inset(2, 0);
//   DrawRectFill(r, &color_Green);
   DrawRectFill(r, &sigmaPrefs->mainColor);

// INT h1 = bounds.left + (bounds.Width() - dividerButtonWidth)/2;
   INT h1 = bounds.left + (bounds.Width() - 16)/2;
   CRect rdot(0,0,16,16);
   rdot.Offset(h1, bounds.top - 4);

   DrawIcon(418, rdot); //, ICON_TRANS trans)
/*
   for (INT i = 0; i < 6; i++)
   {  MovePenTo(h1, bounds.top + i + (dividerViewHeight - 6)/2);
      SetForeColor(even(i) ? &sigmaPrefs->lightColor : &sigmaPrefs->darkColor);
      DrawLine(dividerButtonWidth - 1, 0);
   }
*/
} /* InfoDividerView::HandleUpdate */


BOOL InfoDividerView::HandleMouseDown (CPoint p0, INT modifiers, BOOL doubleClick)
{
   CPoint p = p0;
   InfoAreaView *parent = (InfoAreaView*)Parent();
   INT vmin = parent->bounds.top + minGameViewHeight + 1;
   INT vmax = parent->bounds.bottom - minAnalysisViewHeight;

   theApp->SetCursor(cursor_VResize);

   MOUSE_TRACK_RESULT trackResult;
   do
   {
      TrackMouse(&p, &trackResult);
      if (trackResult == mouseTrack_Released || p.h < bounds.left || p.h > bounds.right) break;

      INT v = p.v + frame.top;
      if (p.v != p0.v && v >= vmin && v <= vmax)
         parent->SetDividerPos(v - 1);

      p0 = p;
   } while (trackResult != mouseTrack_Released);

   theApp->SetCursor();

   return true;
} /* InfoDividerView::HandleMouseDown */
