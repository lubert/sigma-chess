/**************************************************************************************************/
/*                                                                                                */
/* Module  : LIBEDITOR.C                                                                          */
/* Purpose : This module implements the Library Editor.                                           */
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

#include "LibEditor.h"
#include "GameWindow.h"
#include "SigmaStrings.h"

#define hMargin 5
#define vMargin 3

/*---------------------------------------- Sub Views ---------------------------------------------*/

class LibListView : public DataView
{
public:
   LibListView (CViewOwner *parent, CRect frame);

   virtual void HandleUpdate (CRect updateRect);
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);
   virtual void HandleResize (void);
   virtual void HandleActivate (BOOL wasActivated);

   void UpdateVarList (BOOL redraw = true);

   void DrawVarList (void);
   void DrawLine (INT n, BOOL selected = false);        // 0 <= n <= visLines - 1

   CScrollBar *cscrollBar;

private:
   GameWindow *gameWin;
   CGame  *game;
   LIBVAR Var[libMaxVariations];
   INT    linesTotal;               // Total number of lines = number of lib moves
   INT    linesVis;                 // Number of visible lines

   DataHeaderView *headerView;
};


class LibTextView;

class LibToolbar : public CToolbar
{
public:
   LibToolbar (CViewOwner *parent, CRect frame);

   void Adjust (void);

private:
   INT     classifyPosItem;      // Currently selected item in the pm_ClassifyPos
   INT     autoClassifyItem;     // Currently selected item in the pm_AutoClassify
   CMenu   *pm_ClassifyPos, *pm_AutoClassify;
   CButton *tb_ClassifyPos, *tb_AutoClassify, *tb_Comment, *tb_DeleteVar;
   LibTextView *cv_LibText;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

LibEditorView::LibEditorView (CViewOwner *parent, CRect frame)
 : BackView(parent, frame, false)
{
   game = ((GameWindow*)Window())->game;

   CRect listRect, toolbarRect;
   CalcFrames(&listRect, &toolbarRect);
   list    = new LibListView(this, listRect);
   toolbar = new LibToolbar(this, toolbarRect);

   Show(false);
} /* LibEditorView */


void LibEditorView::CalcFrames (CRect *listRect, CRect *toolbarRect)
{
   *listRect = *toolbarRect = DataViewRect();
   toolbarRect->Inset(1, 1);
   toolbarRect->bottom = toolbarRect->top + toolbar_HeightSmall;
   listRect->top = toolbarRect->bottom;

   ExcludeRect(DataViewRect());
} /* LibEditorView::CalcFrames */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

void LibEditorView::HandleUpdate (CRect updateRect)
{
   BackView::HandleUpdate(updateRect);
   DrawBottomRound();

   SetForeColor(RunningOSX() || ! Active() ? &color_MdGray : &color_Black);
   DrawRectFrame(DataViewRect());
} /* LibEditorView::HandleUpdate */


BOOL LibEditorView::CheckScrollEvent (CScrollBar *ctrl, BOOL tracking)
{
   if (ctrl != list->cscrollBar) return false;
   list->DrawVarList();
   return true;
} /* LibEditorView::CheckScrollEvent */


void LibEditorView::HandleOpen (void)
{
   Refresh();
} /* LibEditorView::HandleOpen */


void LibEditorView::Refresh (void)
{
   if (! Visible()) return;

   toolbar->Adjust();
   list->UpdateVarList(true);
} /* LibEditorView::Refresh */


void LibEditorView::HandleResize (void)
{
   CRect listRect, toolbarRect;
   CalcFrames(&listRect, &toolbarRect);
   list->SetFrame(listRect);
   toolbar->SetFrame(toolbarRect);
} /* LibEditorView::HandleResize */


void LibEditorView::CheckAutoAdd (void)
{
   if (! PosLib_Loaded() || PosLib_Locked()) return;
   if (! ((GameWindow*)Window())->libEditor) return;
   if (Prefs.Library.autoClassify == libAutoClass_Off) return;

   // Don't touch positions already in library:
   HKEY thisPos = game->DrawData[game->currMove].hashKey;
   if (PosLib_ProbePos(game->player, thisPos) != libClass_Unclassified)
      return;

   switch (Prefs.Library.autoClassify)
   {
      case libAutoClass_Level :
         Window()->HandleMessage(library_ClassifyPos, libClass_Level);
         break;
      case libAutoClass_Inherit :
         if (game->currMove <= 0) return;
         HKEY prevPos = game->DrawData[game->currMove - 1].hashKey;
         LIB_CLASS prevClass = PosLib_ProbePos(game->opponent, prevPos);
         Window()->HandleMessage(library_ClassifyPos, prevClass);
         break;
   }
} /* LibEditorView::CheckAutoAdd */


/**************************************************************************************************/
/*                                                                                                */
/*                                           LIST VIEW                                            */
/*                                                                                                */
/**************************************************************************************************/

// The game data view is a listbox that comprises three subviews:
// * The DataHeaderView
// * The Listbox Control
// * The actual listbox interior 

static HEADER_COLUMN HCTab[3] = {{"Move",0,63}, {"ECO",0,46}, {"Comment",0,0}};


LibListView::LibListView (CViewOwner *parent, CRect frame)
 : DataView(parent,frame,false)
{
   gameWin = ((GameWindow*)Window());
   game = gameWin->game;
   linesTotal = 0;

   CRect headerRect, scrollRect, dataRect;
   CalcDimensions(&headerRect, &dataRect, &scrollRect);
   headerView = new DataHeaderView(this, headerRect, false, true, 3, HCTab);
   cscrollBar = new CScrollBar(this, 0,0,0, 10, scrollRect);
} /* LibListView::LibListView */

/*------------------------------------- Update Variation List -------------------------------------*/

void LibListView::UpdateVarList (BOOL redraw)
{
   linesTotal = PosLib_CalcVariations(game, Var);
   linesVis   = (bounds.Height() - FontLineSpacing() - headerView->bounds.Height() - vMargin - 5)/FontHeight();

   INT scmax = Max(0, linesTotal - linesVis);
   cscrollBar->SetMax(scmax);
   cscrollBar->SetVal(0);
   cscrollBar->SetIncrement(linesVis - 1);

   if (redraw) DrawVarList();
} /* LibListView::UpdateVarList */

/*--------------------------------------- Event Handling -----------------------------------------*/

void LibListView::HandleUpdate (CRect updateRect)
{
   // First we call the inherited Draw() method that draws the exterior 3D frame.
   DataView::HandleUpdate(updateRect);

   CRect headerRect, scrollRect, dataRect;
   CalcDimensions(&headerRect, &dataRect, &scrollRect);
   DrawRectFill(dataRect, &color_White);

   DrawVarList();
} /* LibListView::HandleUpdate */


BOOL LibListView::HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)
{
   if (true)
   {
      INT n = (pt.v - headerView->bounds.Height() - vMargin /* + FontDescent()*/)/FontHeight();
      INT N = cscrollBar->GetVal() + n;

      if (N >= 0 && N < linesTotal)
      {
         gameWin->CheckAbortEngine();
         DrawLine(n, true);
         gameWin->FlushAnnotation();
         gameWin->boardAreaView->ClearMoveMarker();
         game->PlayMove(&Var[N].m);
         gameWin->PlayerMovePerformed(true);
      }
   }
   else if (modifiers & modifier_Control)
   {
   }
   else if (modifiers & modifier_Command)
   {
//    ShowHelpTip("This is the Game Record list, which shows the moves of the current game (including any annotations).");
   }

   return true;
} /* LibListView::HandleMouseDown */


void LibListView::HandleResize (void)
{
   CRect headerRect, scrollRect, dataRect;

   CalcDimensions(&headerRect, &dataRect, &scrollRect);
   cscrollBar->SetFrame(scrollRect);
   DrawRectFill(dataRect, &color_White);
   UpdateVarList(true);
} /* LibListView::HandleResize */


void LibListView::HandleActivate (BOOL wasActivated)
{
   DrawVarList();
} /* LibListView::HandleActivate */

/*------------------------------------- Draw Variation List --------------------------------------*/

void LibListView::DrawVarList (void)
{
   if (! Visible() || ! ((GameWindow*)Window())->libEditor) return;

   for (INT n = 0; n < linesVis; n++)
      DrawLine(n);
} /* LibListView::DrawVarList */

/*---------------------------------------- Draw Single Line --------------------------------------*/
// n is the "local" line number (where n = 0 is the first visible line, and n = 1 is the second
// visible line etc. up to visLines - 1).

void LibListView::DrawLine (INT n, BOOL selected)
{
   INT N = cscrollBar->GetVal() + n;                 // Global line no (index in GMap[]).
   INT lineWidth = bounds.Width() - 2*hMargin - 16;
   INT v = (n + 1)*FontHeight() - FontDescent() + headerView->bounds.Height() + vMargin;

   SetFontForeColor();
   MovePenTo(hMargin, v);

   if (N < linesTotal)
   {
      CHAR      mstr[20], eco[libECOLength + 1], comment[libCommentLength + 1];
      LIB_CLASS libClass;

      CalcMoveStr(&Var[N].m, mstr);
      libClass = PosLib_ProbePos(game->opponent, Var[N].pos);
      PosLib_ProbePosStr(game->opponent, Var[N].pos, eco, comment);

      if (selected)
      {  RGB_COLOR hilite;
         GetHiliteColor(&hilite);
         SetBackColor(&hilite);
      }

      DrawStr(mstr);

      if (selected)
         SetStdBackColor();
    
      TextEraseTo(bounds.left + 57 + 13);
      DrawStr(eco); TextEraseTo(bounds.left + 117);
      DrawStr(comment, (bounds.right - 20) - (bounds.left + 115));

      ICON_TRANS iconTrans = (Enabled() && Active() ? iconTrans_None : iconTrans_Disabled);
      CRect rIcon(0,0,16,16); rIcon.Offset(49, v - 12);
      if (libClass < libClass_First || libClass > libClass_Last)
         DrawIcon(icon_LibUnclass, rIcon, iconTrans);
      else if (libClass != libClass_Unclassified)
         DrawIcon(libClass + icon_LibClass1 - 1, rIcon, iconTrans);
   }

   TextEraseTo(bounds.right - 18);

   if (selected)
      SetStdBackColor();
} /* LibListView::DrawLine */


/**************************************************************************************************/
/*                                                                                                */
/*                                           TOOLBAR                                              */
/*                                                                                                */
/**************************************************************************************************/

class LibTextView : public CToolbarTextView
{
public:
   LibTextView (CViewOwner *parent, CRect frame);
   virtual void HandleUpdate (CRect updateRect);
};


LibToolbar::LibToolbar (CViewOwner *parent, CRect frame)
 : CToolbar(parent, frame)
{
   // "Classify" popup menu:
   pm_ClassifyPos = new CMenu("");
   pm_ClassifyPos->AddPopupHeader(GetStr(sgr_LibClassifyMenu,0));
   for (INT i = libClass_First; i <= libClass_Last; i++)
   {
      if (i == 1 || i == 3 || i == 7) pm_ClassifyPos->AddSeparator();
      pm_ClassifyPos->AddItem(GetStr(sgr_LibClassifyMenu,i+1), i);
      pm_ClassifyPos->SetIcon(i, 369 + i);
   }
   classifyPosItem = libClass_First;
   pm_ClassifyPos->CheckMenuItem(classifyPosItem, true);

   // "Auto Classify" sub menu:
   INT g = sgr_LibAutoClassMenu;
   pm_AutoClassify = new CMenu("");
   pm_AutoClassify->AddPopupHeader(GetStr(g,0));
   pm_AutoClassify->AddItem(GetStr(g,1), libAutoClass_Off);
   pm_AutoClassify->AddItem(GetStr(g,2), libAutoClass_Level);
   pm_AutoClassify->AddItem(GetStr(g,3), libAutoClass_Inherit);
   autoClassifyItem = Prefs.Library.autoClassify;
   pm_AutoClassify->CheckMenuItem(autoClassifyItem, true);

   // Finally add the actual tb controls:
   tb_ClassifyPos  = AddPopup(library_ClassifyPos, pm_ClassifyPos, 369, 16, 24, "", "Classify Position");
   tb_AutoClassify = AddPopup(library_AutoClassify, pm_AutoClassify, icon_AutoPlay, 16, 24, "", "Auto Classify Position");
   AddSeparator();
   tb_Comment = AddButton(library_ECOComment, icon_LibECO, 16, 24, "", "Edit ECO/Comment");
   AddSeparator();
   tb_Comment = AddButton(library_DeleteVar, icon_Trash, 16, 24, "", "Delete Variations");
   AddSeparator();
   AddCustomView(cv_LibText = new LibTextView(this, NextItemRect(100)));
} /* LibToolbar::LibToolbar */


void LibToolbar::Adjust (void)
{
   CGame *game = ((GameWindow*)Window())->game;
   LIB_CLASS libClass = PosLib_Probe(game->player, game->Board);
   pm_ClassifyPos->CheckMenuItem(classifyPosItem, false);
   classifyPosItem = libClass;
   pm_ClassifyPos->CheckMenuItem(classifyPosItem, true);
   tb_ClassifyPos->SetIcon(369 + classifyPosItem);

   pm_AutoClassify->CheckMenuItem(autoClassifyItem, false);
   autoClassifyItem = Prefs.Library.autoClassify;
   pm_AutoClassify->CheckMenuItem(autoClassifyItem, true);

   cv_LibText->Redraw();
} /* LibToolbar::Adjust */

/*------------------------------------ Lib toolbar Text View -------------------------------------*/

LibTextView::LibTextView (CViewOwner *parent, CRect frame)
 : CToolbarTextView(parent, frame)
{
} /* LibTextView::LibTextView */


void LibTextView::HandleUpdate (CRect updateRect)
{
   CToolbarTextView::HandleUpdate(updateRect);

	CHAR s[100];
	Format(s, "%ld positions", PosLib_Count());
	MovePen(3, 0); DrawStr(s);
} /* LibTextView::HandleUpdate */
