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

#include "GameListView.h"
#include "CollectionWindow.h"
#include "SigmaApplication.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                         GAME LIST VIEW                                         */
/*                                                                                                */
/**************************************************************************************************/

// The collection game list view is a container view, that comprises the following 3 component
// views:
// * The listbox header (headerView)
// * The scroll bar (scrollBar)
// * The listbox contents/interior (contentView)

GameListView::GameListView (CViewOwner *parent, CRect frame)
 : DataView(parent,frame)
{
   selected = 0;
   selStart = selEnd = selected;
   ResetColumns();

   CRect headerRect, scrollRect, listRect;
   CalcDimensions(&headerRect, &listRect, &scrollRect);
   headerRect.Inset(1,0);
   headerRect.Offset(0,1);
   headerView = new GameListHeaderView(this, headerRect, columnCount, HCTab);
   scrollBar  = new CScrollBar(this, 0,0,0, 10, scrollRect);
   listView   = new ColListView(this, listRect);
   AdjustScrollBar();
} /* GameListView::GameDataView */


SigmaCollection *GameListView::Collection (void)
{
   return ((CollectionWindow*)Window())->collection;
} /* GameListView::Collection */

/*---------------------------------------- Event Handling ----------------------------------------*/

void GameListView::HandleUpdate (CRect updateRect)
{
   // First we call the inherited Draw() method that draws the exterior 3D frame:
   DataView::HandleUpdate(updateRect);

   // Then we draw the game list:
   listView->DrawList();
} /* GameListView::HandleUpdate */


void GameListView::HandleResize (void)
{
   CRect headerRect, scrollRect, listRect;
   CalcDimensions(&headerRect, &listRect, &scrollRect);
   headerRect.Inset(1,0);
   headerRect.Offset(0,1);
   headerView->SetFrame(headerRect);
   scrollBar->SetFrame(scrollRect);
   listView->SetFrame(listRect);
   AdjustScrollBar();
} /* GameListArea::HandleResize */


void GameListView::DrawList (void)
{
   listView->DrawList();
} /* GameListView::DrawList */


BOOL GameListView::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   if (! Enabled()) return false;

   BOOL option = (modifiers & modifier_Option);
   BOOL shift  = (modifiers & modifier_Shift);

   switch (key)
   {
      case key_Return :
      case key_Enter :
         if (selected >= 0 && selStart == selEnd)
            ((CollectionWindow*)Window())->OpenGame(Collection()->View_GetGameNo(selected));
         return true;
      case key_UpArrow :
         AdjustSelection(-(option ? linesVis - 1 : 1), shift);
         return true;
      case key_DownArrow :
         AdjustSelection(+(option ? linesVis - 1 : 1), shift);
         return true;

      default :
         if (IsAlphaNum(c) && !option && !shift)
         {  SmartSearch(c); return true; }
         else
            return scrollBar->HandleKeyDown(c,key,modifiers);
   }
} /* GameListView::HandleKeyDown */


BOOL GameListView::CheckScrollEvent (CScrollBar *ctrl, BOOL tracking)
{
   if (ctrl != scrollBar) return false;
   listView->DrawList();
   return true;
} /* GameListView::CheckScrollEvent */


void GameListView::HandleColumnSelect (INT i)
{
   listView->DrawList();  // First refresh column hilite
   if (! ((CollectionWindow*)Window())->Sort((INDEX_FIELD)i))
      headerView->SelectCell(0);
   selected = selStart = selEnd = 0;
   scrollBar->SetVal(0);
   listView->DrawList();

   Window()->HandleMessage(msg_ColSelChanged);
} /* GameListView::HandleColumnSelect */


void GameListView::HandleSortDir (BOOL ascend)
{
   ((CollectionWindow*)Window())->SetSortDir(ascend);
   selected = selStart = selEnd = 0;
   scrollBar->SetVal(0);
   listView->DrawList();

   Window()->HandleMessage(msg_ColSelChanged);
} /* GameListView::HandleSortDir */


static LONG nextSmartTick = 0;
static CHAR smartStr[nameStrLen + 1];
static INT  smartStrLen = 0;

void GameListView::SmartSearch (CHAR c)
{
   if (Collection()->inxField == inxField_GameNo || Collection()->viewCount <= 1) return;

   if (Timer() >= nextSmartTick)
      smartStrLen = 0;
   
   nextSmartTick = Timer() + 30;

   if (smartStrLen < nameStrLen)
      smartStr[smartStrLen++] = c,
      smartStr[smartStrLen] = 0;

   selected = selStart = selEnd = Collection()->View_Search(smartStr);
   scrollBar->SetVal(selected);
   listView->DrawList();

   Window()->HandleMessage(msg_ColSelChanged);
} /* GameListView::SmartSearch */


void GameListView::Enable (BOOL enable)
{
   headerView->Enable(enable);
   listView->Enable(enable);
   scrollBar->Enable(enable);
} /* GameListView::Enable */

/*------------------------------------- Selection Adjusting --------------------------------------*/

void GameListView::AdjustSelection (LONG delta, BOOL multi)
{
   ULONG newSel;                       // New selection state
   ULONG newStart = selStart;
   ULONG newEnd   = selEnd;

   // First compute new selection values:
   if (delta > 0)
   {
      newSel = MinL(selected + delta, linesTotal - 1);
      if (! multi)
         newStart = newEnd = newSel;
      else if (selected == selEnd)
         newEnd = newSel;
      else if (newSel <= selEnd)
         newStart = newSel;
      else
         newStart = selEnd, newEnd = newSel;
   }
   else if (delta < 0)
   {
      newSel = MaxL(selected + delta, 0);
      if (! multi)
         newStart = newEnd = newSel;
      else if (selected == selStart)
         newStart = newSel;
      else if (newSel >= selStart)
         newEnd = newSel;
      else
         newStart = newSel, newEnd = selStart;
   }

   // Update selection values (and remember previous settings):
   ULONG oldSel   = selected;
   ULONG oldStart = selStart;
   ULONG oldEnd   = selEnd;

   selected = newSel;
   selStart = newStart;
   selEnd   = newEnd;

   // Then check auto-scrolling (if "newSel" no longer in view):
   if (selected < scrollBar->GetVal())                     // New selection "above"
      scrollBar->SetVal(selected),
      listView->DrawList();
   else if (selected >= scrollBar->GetVal() + linesVis)    // New selection "below"
      scrollBar->SetVal(selected - linesVis + 1),
      listView->DrawList();
   else                                                    // New selection "within"
      for (INT n = 0; n < linesVis; n++)
      {
         ULONG N = (LONG)n + scrollBar->GetVal();
         BOOL wasSelected = (N >= oldStart && N <= oldEnd);
         BOOL isSelected  = (N >= newStart && N <= newEnd);
         if (wasSelected != isSelected)
            listView->DrawRow(n);
      }

   Window()->HandleMessage(msg_ColSelChanged);
} /* GameListView::AdjustSelection */


LONG GameListView::GetSelStart (void)
{
   return selStart;
} /* GameListView::GetSelStart */


LONG GameListView::GetSelEnd (void)
{
   return selEnd;
} /* GameListView::GetSelEnd */


LONG GameListView::GetSel (void)
{
   return selected;
} /* GameListView::GetSel */


void GameListView::SetSelection (LONG start, LONG end)
{
   selected = selStart = start;
   selEnd = end;
   Window()->HandleMessage(msg_ColSelChanged);
} /* GameListView::SetSelection */

/*------------------------------------- ScrollBar Adjusting --------------------------------------*/
// Whenever the size of scrollbar or the number of lines has changed, we need to adjust the
// scrollbar.

void GameListView::AdjustScrollBar (void)
{
   linesVis   = listView->VisLines();
   linesTotal = Collection()->View_GetGameCount();
   LONG scmax = MaxL(0, linesTotal - linesVis);
   scrollBar->SetMax(scmax);
   scrollBar->SetIncrement(linesVis - 1);
   if (scrollBar->GetVal() > scmax)
      scrollBar->SetVal(scmax);
} /* GameListView::AdjustScrollBar */

/*------------------------------------- Drawing Game List ----------------------------------------*/

void GameListView::ResetColumns (void)
{
   HCTab[0].text = "Game";
   HCTab[1].text = "White";
   HCTab[2].text = "Black";
   HCTab[3].text = "Event/Site";
   HCTab[4].text = "Date";
   HCTab[5].text = "Round";
   HCTab[6].text = "Result";
   HCTab[7].text = "ECO";

   if (! Prefs.Collections.keepColWidths)
      DefaultCollectionCellWidth(Prefs.ColDisplay.cellWidth);

   for (INT i = 0; i < maxColCells; i++)
      HCTab[i].width = Prefs.ColDisplay.cellWidth[i],
      HCTab[i].iconID = 0;

   if (Collection()->Publishing())
      HCTab[0].width = 150;

   columnCount = maxColCells;
} /* GameListView::ResetColumns */


void DefaultCollectionCellWidth (INT width[])
{
   width[0] =  60;   // An additional 115 will be added if publishing.
   width[1] = 120;
   width[2] = 120;
   width[3] = 120;
   width[4] =  70;
   width[5] =  60;
   width[6] =  60;
   width[7] =  80;
} /* DefaultCollectionCellWidth */

/*---------------------------------------- Set Publishing ----------------------------------------*/
// If the user toggles the publishing we have to resize the "Game #" column and redraw.

void GameListView::TogglePublishing (void)
{
   INT oldCellWidth = Prefs.ColDisplay.cellWidth[0];
   headerView->SetCellWidth(0, Collection()->Publishing() ? 165 : 50);
   Prefs.ColDisplay.cellWidth[0] = oldCellWidth;
   listView->Redraw();
} /* GameListView::TogglePublishing */


/**************************************************************************************************/
/*                                                                                                */
/*                                         LIST HEADER VIEW                                       */
/*                                                                                                */
/**************************************************************************************************/

GameListHeaderView::GameListHeaderView (CViewOwner *parent, CRect frame, INT columns, HEADER_COLUMN *HCTab)
 : DataHeaderView(parent, frame, true, false, columns, HCTab, 0, true, true)
{
} /* GameListHeaderView::GameListHeaderView */


void GameListHeaderView::HandleSelect (INT i)
{
   ((GameListView*)Parent())->HandleColumnSelect(i);
} /* GameListHeaderView::HandleSelect */


void GameListHeaderView::HandleSortDir (BOOL ascend)
{
   ((GameListView*)Parent())->HandleSortDir(ascend);
} /* GameListHeaderView::HandleSortDir */


void GameListHeaderView::HandleColumnResize (INT i)
{
   if (i > 0 || ! ((GameListView*)Parent())->Collection()->Publishing())
      Prefs.ColDisplay.cellWidth[i] = ((GameListView*)Parent())->HCTab[i].width;
   ((GameListView*)Parent())->DrawList();
} /* GameListHeaderView::HandleColumnResize */


/**************************************************************************************************/
/*                                                                                                */
/*                                         LIST CONTENTS VIEW                                     */
/*                                                                                                */
/**************************************************************************************************/

// The actual contents of the collection game list is controlled by the ColListView (which does
// NOT include the black frame but only the white interior).

ColListView::ColListView (CViewOwner *parent, CRect frame)
  : CView(parent,frame)
{
} /* ColListView::ColListView */


void ColListView::HandleUpdate (CRect updateRect)
{
   DrawList();
} /* ColListView::HandleUpdate */


BOOL ColListView::HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)
{
   if (! Enabled()) return false;

   INT  n = pt.v/FontHeight();
   LONG N = (LONG)n + Parent()->scrollBar->GetVal();
   if (N < 0 || N >= Collection()->View_GetGameCount()) return false;

   BOOL shift  = (modifiers & modifier_Shift);
   LONG delta  = N - Parent()->GetSel();

   if (delta != 0)
      Parent()->AdjustSelection(delta, shift);
      
   if (doubleClick && Parent()->GetSelStart() == Parent()->GetSelEnd())
   {  CollectionWindow *colWin = (CollectionWindow*)Window();
      colWin->OpenGame(colWin->collection->View_GetGameNo(N));
   }

   return true;
} /* ColListView::HandleMouseDown */


void ColListView::HandleActivate (BOOL wasActivated)
{
   Redraw();
} /* ColListView::HandleActivate */


INT ColListView::VisLines (void)
{
   return (bounds.Height() - 1)/FontHeight();
} /* ColListView::VisLines */


GameListView *ColListView::Parent (void)
{
   return (GameListView*)CView::Parent();
} /* ColListView::Parent */


SigmaCollection *ColListView::Collection (void)
{
   return ((CollectionWindow*)Window())->collection;
} /* ColListView::Collection */

/*------------------------------------- Drawing Game List ----------------------------------------*/

void ColListView::DrawList (void)
{
   for (INT n = 0; n < VisLines(); n++)
      DrawRow(n);
} /* ColListView::DrawList */


void ColListView::DrawRow (INT n)
{
   LONG N = Parent()->scrollBar->GetVal() + (LONG)n;

   if (n < 0 || n >= VisLines() || N >= Collection()->View_GetGameCount()) return;

   ULONG     gameNo = Collection()->View_GetGameNo(N);
   BOOL      lineSelected = (N >= Parent()->GetSelStart() && N <= Parent()->GetSelEnd());
   RGB_COLOR hilite;

   Collection()->View_GetGameInfo(N);

   if (lineSelected) GetHiliteColor(&hilite);

   SetFontForeColor();
   SetFontStyle(((CollectionWindow*)Window())->GameOpened(gameNo) ? fontStyle_Bold : fontStyle_Plain);

   CRect r(0, n*FontHeight() + 1, bounds.left - 1, (n + 1)*FontHeight());

   for (INT i = 0; i < Parent()->columnCount && r.right < bounds.right; i++)
   {
      // Calc cell rectangle:
      r.left = r.right + 1;
      r.right = r.left + (i < Parent()->columnCount - 1 ? Parent()->HCTab[i].width - 1 : bounds.right);
      if (r.right >= bounds.right) r.right = bounds.right - 1;
      if (i == 0) r.left++;

      // Calc fore and background colours:
      if (! Active())
         SetForeColor(&color_MdGray);
      else if (lineSelected && ((ULONG)hilite.red + (ULONG)hilite.green + (ULONG)hilite.red)/3 < 33000L)
         SetForeColor(&color_White);
      else
         SetForeColor(&color_Black);

      if (! lineSelected)
         SetBackColor(i == Parent()->headerView->Selected() ? &color_LtGray : &color_BrGray);
      else
         SetBackColor(Active() ? &hilite : &color_BtGray);

      // Finally draw the text (and vertical white divider):
      CHAR fs[100];
      CalcFieldStr(gameNo, i, fs);
      if (i > 0)
         DrawStr(fs, r, textAlign_Left, false);
      else if (! Collection()->Publishing())
         DrawStr(fs, r, textAlign_Right, false);
      else
      {  INT right = r.right;
         BOOL indent = (Collection()->game->Info.headingType != headingType_Chapter);
         r.right = r.left + 50;
         DrawStr(fs, r, textAlign_Right, false);
         SetFontStyle(fontStyle_Bold);
         Format(fs, " %s%s", (indent ? "    " : ""), Collection()->game->Info.heading);
         r.left = r.right; r.right = right;
         DrawStr(fs, r, textAlign_Left, false);
         SetFontStyle(fontStyle_Plain);
      }

      SetForeColor(&color_White);
      MovePenTo(r.right, r.top); DrawLine(0, r.bottom - r.top - 1);
   }

   // Show page breaks if in "publishing" mode (and sorting by game # ascending):
   SetForeColor(Collection()->Publishing() &&
                Collection()->game->Info.pageBreak &&
                Parent()->headerView->Selected() == 0 &&
                Parent()->headerView->Ascending() ?
                &color_Gray : &color_White);
   MovePenTo(bounds.left + 1, n*FontHeight()); DrawLine(bounds.Width() - 3, 0);

   SetForeColor(&color_Black);
   SetBackColor(&color_White);
} /* ColListView::DrawRow */


void ColListView::CalcFieldStr (ULONG gameNo, INT i, CHAR *fs)
{
   GAMEINFO *Info = &(Collection()->game->Info);
   CHAR *s, ts[100];

   switch (i)
   {
      case 0 : Format(ts, "%ld ", gameNo + 1); s = ts; break;
      case 1 : s = Info->whiteName; break;
      case 2 : s = Info->blackName; break;
      case 3 : if (Info->event[0] == 0) s = Info->site;
               else if (Info->site[0] == 0) s = Info->event;
               else Format(ts, "%s/%s", Info->event, Info->site), s = ts;
               break;
      case 4 : s = Info->date; break;
      case 5 : s = Info->round; break;
      case 6 : CalcInfoResultStr(Info->result, ts); s = ts; break;
      case 7 : s = Info->ECO; break;
   }
   Format(fs, " %s", s);
} /* ColListView::CalcFieldStr */
