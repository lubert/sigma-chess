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

#include "GameView.h"
#include "GameWindow.h"
#include "CollectionWindow.h"
#include "CWindow.h"
#include "CControl.h"
#include "SigmaStrings.h"
#include "Annotations.h"

#define hMargin 10
#define vMargin 6

#define numInsetH           (4*digitBWidth)
#define move1InsetH         (9*digitBWidth)
#define move2InsetH         (19*digitBWidth)

/*--------------------------------------- Local Classes ------------------------------------------*/

class GameHeaderView;
class GameFooterView;

class GameDataView : public DataView
{
public:
   GameDataView (CViewOwner *parent, CRect frame);

   virtual void HandleUpdate (CRect updateRect);
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);
   virtual void HandleResize (void);
   virtual void HandleActivate (BOOL wasActivated);

   void CalcFrames (BOOL headerClosed);
   void GotoMove (INT j, BOOL openAnnEditor = false);

   INT  HeaderHeight (BOOL closed);
   void RefreshGameStatus (void);
   void RefreshGameInfo (void);
   void RefreshLibInfo (void);
   void ToggleHeader (BOOL closed);
   void ResizeHeader (void);

   void UpdateGameList (BOOL redraw = true);

   void DrawGameList (void);
   void DrawLine (INT n);        // 0 <= n <= visLines - 1
   void DrawTextLine (CHAR *s, INT lineLen, INT lineWidth, BOOL isLastLine);

   CScrollBar *cscrollBar;

private:
   INT linesVis;                 // Number of visible lines
   INT linesTotal;               // Total number of lines in game map
   INT futureLineCount;          // Currently visible number of future lines (update by DrawGameList)
   GAMEMAP GMap[gameMapSize];    // The actual game map [0...lines-1]

   CRect headerRect, scrollRect, dataRect, footerRect;

   GameHeaderView *headerView;
   GameFooterView *footerView;
};


class GameHeaderView : public SpringHeaderView
{
public:
   GameHeaderView (CViewOwner *parent, CRect frame);

   virtual void HandleUpdate (CRect updateRect);
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);
   virtual void HandleToggle (BOOL closed);

   void RefreshGameStatus (void);
};


class GameFooterView : public DataHeaderView
{
public:
   GameFooterView (CViewOwner *parent, CRect frame);
   virtual void HandleUpdate (CRect updateRect);

   HEADER_COLUMN HCTab[1];
   CHAR s[libECOLength + libCommentLength + 10];
};


/**************************************************************************************************/
/*                                                                                                */
/*                                            GAME VIEW                                           */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Constructor/Destructor -----------------------------------*/
// The GameView is mainly a "container" view that includes the GameDataView, where the actual
// data of game (moves e.t.c.) is shown.

GameView::GameView (CViewOwner *parent, CRect frame)
 : BackView(parent, frame, false)
{
   restoreHeight = bounds.Height();

   ExcludeRect(DataViewRect());
   gameDataView = new GameDataView(this, DataViewRect());
} /* GameView::GameView */

/*---------------------------------------- Event Handling ----------------------------------------*/

void GameView::HandleUpdate (CRect updateRect)
{
   BackView::HandleUpdate(updateRect);
   DrawTopRound();
   
   if (bounds.bottom == Parent()->bounds.bottom)
     DrawBottomRound();
} /* GameView::HandleUpdate */


BOOL GameView::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   return gameDataView->HandleKeyDown(c, key, modifiers);
} /* GameView::HandleKeyDown */


void GameView::HandleResize (void)
{
   ExcludeRect(DataViewRect());
   gameDataView->SetFrame(DataViewRect());
//   BackView::HandleUpdate(bounds);
//   DrawTopRound();
//   DrawBottomRound();
} /* GameView::HandleResize */


void GameView::RefreshLibInfo (void)
{
   gameDataView->RefreshLibInfo();
} /* GameView::RefreshLibInfo */


BOOL GameView::CheckScrollEvent (CScrollBar *ctrl, BOOL tracking)
{
   if (ctrl != gameDataView->cscrollBar) return false;
   RedrawGameList();
   return true;
} /* GameView::CheckScrollEvent */


void GameView::RefreshGameStatus (void)
{
   gameDataView->RefreshGameStatus();
} /* GameView::UpdateGameStatus */


void GameView::RefreshGameInfo (void)
{
   gameDataView->RefreshGameInfo();
} /* GameView::RefreshGameInfo */


void GameView::ResizeHeader (void)
{
   gameDataView->ResizeHeader();
} /* GameView::ResizeHeader */


void GameView::UpdateGameList (void)
{
   gameDataView->UpdateGameList();
} /* GaneView::UpdateGameList */


void GameView::RedrawGameList (void)
{
   gameDataView->DrawGameList();
} /* GaneView::RedrawGameList */


/**************************************************************************************************/
/*                                                                                                */
/*                                         GAME DATA VIEW                                         */
/*                                                                                                */
/**************************************************************************************************/

// The game data view is a listbox that comprises three subviews:
// * The DataHeaderView
// * The Listbox Control
// * The actual listbox interior 

GameDataView::GameDataView (CViewOwner *parent, CRect frame)
 : DataView(parent,frame,false)
{
   linesTotal = 0;
   futureLineCount = 0;

   CalcFrames(Prefs.GameDisplay.gameHeaderClosed);
   headerView = new GameHeaderView(this, headerRect);
   cscrollBar = new CScrollBar(this, 0,0,0, 10, scrollRect);
   footerView = new GameFooterView(this, footerRect);

   UpdateGameList();
} /* GameDataView::GameDataView */


void GameDataView::CalcFrames (BOOL headerClosed)
{
   CalcDimensions(&headerRect, &dataRect, &scrollRect, HeaderHeight(headerClosed));
   footerRect = headerRect;
   footerRect.bottom = dataRect.bottom + 1;
   footerRect.top = footerRect.bottom - headerViewHeight;
   dataRect.bottom = scrollRect.bottom = footerRect.top;
   if (! RunningOSX()) scrollRect.bottom++;
} /* GameDataView::CalcFrames */

/*--------------------------------------- Event Handling -----------------------------------------*/

void GameDataView::HandleUpdate (CRect updateRect)
{
   // First we call the inherited Draw() method that draws the exterior 3D frame.
   DataView::HandleUpdate(updateRect);

   CalcFrames(headerView->Closed());
   DrawRectFill(dataRect, &color_White);
   DrawGameList();
} /* GameDataView::HandleUpdate */


BOOL GameDataView::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   if (cscrollBar->HandleKeyDown(c,key,modifiers)) return true;
   //### other key strokes here....
   return false;
} /* GameDataView::HandleKeyDown */


BOOL GameDataView::HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)
{
   if (doubleClick)
   {
      INT n = (pt.v - headerView->bounds.Height() - vMargin + FontDescent())/FontHeight();
      INT N = cscrollBar->GetVal() + n;

      if (N >= 0 && N < linesTotal)
      {
         INT j = GMap[N].moveNo & 0x0FFF;

         if (GMap[N].moveNo & gameMap_White)
         {
            if (pt.h < move2InsetH)
               GotoMove(j, false);
            else if (GMap[N].moveNo & gameMap_Black)
               GotoMove(j + 1, false);
         }
         else if (GMap[N].moveNo & gameMap_Black)
         {
            if (pt.h >= move2InsetH)
               GotoMove(j, false);
         }
         else
         {
            GotoMove(j, true);
         }
      }
   }
   else if (modifiers & modifier_Control)
   {
      CMenu *pm = new CMenu("");
      pm->AddPopupHeader("Move List Options");
      pm->AddItem(GetStr(sgr_NotationMenu,1), notation_Short);
      pm->AddItem(GetStr(sgr_NotationMenu,2), notation_Long);
      pm->AddItem(GetStr(sgr_NotationMenu,3), notation_Descr);
      pm->AddSeparator();
      pm->AddItem(GetStr(sgr_NotationMenu,4), notation_Figurine);
      pm->AddSeparator();
      pm->AddItem("Show Future Moves", display_ShowFutureMoves);
      pm->AddItem("Hilite Current Move", display_HiliteCurrMove);

      pm->CheckMenuItem(Prefs.Notation.moveNotation + notation_Short, true);
      pm->CheckMenuItem(notation_Figurine, Prefs.Notation.figurine);
      pm->CheckMenuItem(display_ShowFutureMoves, Prefs.Games.showFutureMoves);
      pm->CheckMenuItem(display_HiliteCurrMove, Prefs.Games.hiliteCurrMove);

      INT msg;
      if (pm->Popup(&msg)) sigmaApp->HandleMessage(msg);

      delete pm;
   }
   else if (modifiers & modifier_Command)
   {
      ShowHelpTip("This is the Game Record list, which shows the moves of the current game (including any annotations).");
   }

   return true;
} /* GameDataView::HandleMouseDown */


void GameDataView::HandleResize (void)
{
   CalcFrames(headerView->Closed());
   cscrollBar->SetFrame(scrollRect);
   DrawRectFill(dataRect, &color_White);
   footerView->SetFrame(footerRect, true);
   UpdateGameList(true);

   DataView::HandleUpdate(bounds);
} /* GameDataView::HandleResize */


void GameDataView::HandleActivate (BOOL wasActivated)
{
   DrawGameList();
} /* GameDataView::HandleActivate */


void GameDataView::ToggleHeader (BOOL closed)
{
   CalcFrames(headerView->Closed());
   headerView->SetFrame(headerRect);
   cscrollBar->SetFrame(scrollRect);
   DrawRectFill(dataRect, &color_White);
   UpdateGameList(true);
} /* GameDataView::ToggleHeader */


void GameDataView::ResizeHeader (void)
{
   ToggleHeader(headerView->Closed());
} /* GameDataView::ResizeHeader */


void GameDataView::RefreshGameStatus (void)
{
   headerView->RefreshGameStatus();
   footerView->Redraw();
} /* GameDataView::UpdateGameStatus */


void GameDataView::RefreshGameInfo (void)
{
   if (! headerView->Closed())
      headerView->Redraw();
} /* GameDataView::RefreshGameInfo */


INT GameDataView::HeaderHeight (BOOL closed)
{
   GAMEINFOFILTER *F = &(((GameWindow*)Window())->infoFilter);
   INT height = headerViewHeight + 1;

   if (! closed)
   {
      INT n = 0;

      if (F->players)   n += 2;
      if (F->event)     n++;
      if (F->site)      n++;
      if (F->date   || F->round) n++;
      if (F->result || F->ECO)   n++;
      height += n*springHeaderLineHeight + 5;
   }

   return height;
} /* GameDataView::HeaderHeight */


void GameDataView::RefreshLibInfo (void)
{
   footerView->Redraw();
} /* GameDataView::RefreshLibInfo */


void GameDataView::GotoMove (INT j, BOOL openAnnEditor)
{
   GameWindow *win = ((GameWindow*)Window());
   if (! win->AbandonRatedGame()) return;
   win->GotoMove(j, openAnnEditor);
} /* GameDataView::GotoMove */


/**************************************************************************************************/
/*                                                                                                */
/*                                        CALC/ADJUST LISTBOX                                     */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Compute Game Data Map -------------------------------------*/
// The contents of the game list box is controlled by the "Game Data Map". This map basically
// defines the contents of each line in the listbox. Whenever the actual game record has changed
// the game data map should be rebuilt, which in turn will also adjust the scrollbar accordingly.

void GameDataView::UpdateGameList (BOOL redraw)
{
   CollectionWindow *colWin = ((GameWindow*)Window())->colWin;
   BOOL isCollectionGame = (colWin != nil);
   BOOL isPublishing = (isCollectionGame && colWin->collection->Publishing());
   INT  toMove = (Prefs.Games.showFutureMoves ? Game()->lastMove : Game()->currMove);
   INT  N;

   linesTotal = Game()->CalcGameMap(toMove, GMap, false, isCollectionGame, isPublishing);
   linesVis   = (dataRect.Height() - FontLineSpacing() - vMargin - 5)/FontHeight();

   if (! Prefs.Games.showFutureMoves)
      N = linesTotal;
   else if (Game()->currMove == 0)
      N = 0;
   else
   {  for (N = 0; N < linesTotal && (GMap[N].moveNo & 0x0FFF) <= Game()->currMove; N++);
      if (futureLineCount > 0 && cscrollBar->GetVal() > 0 && N > linesVis/2)
         for (INT i = 0; i < Min(linesVis/2,futureLineCount) && N < linesTotal; i++, N++);
   }

   INT scmax = Max(0, linesTotal - linesVis);
   cscrollBar->SetMax(scmax);
   cscrollBar->SetVal(toMove == 0 ? 0 : Max(0, N - linesVis));
   cscrollBar->SetIncrement(linesVis - 1);

   if (redraw) DrawGameList();
} /* GameDataView::UpdateGameList */


/**************************************************************************************************/
/*                                                                                                */
/*                                       DRAW LISTBOX CONTENTS                                    */
/*                                                                                                */
/**************************************************************************************************/

void GameDataView::DrawGameList (void)
{
   if (! Visible()) return;
   futureLineCount = 0;
   for (INT n = 0; n < linesVis; n++)
      DrawLine(n);
} /* GameDataView::DrawGameList */

/*---------------------------------------- Draw Single Line --------------------------------------*/
// n is the "local" line number (where n = 0 is the first visible line, and n = 1 is the second
// visible line e.t.c. up to visLines - 1).

void GameDataView::DrawLine (INT n)
{
   INT N = cscrollBar->GetVal() + n;                 // Global line no (index in GMap[]).
   INT lineWidth = bounds.Width() - 2*hMargin - 16;
   RGB_COLOR hiColor;
   
   GetHiliteColor(&hiColor);

   MovePenTo(hMargin, dataRect.top + (n + 1)*FontHeight() - FontDescent()/* + headerView->bounds.Height()*/ + vMargin);

   if (N < linesTotal)
   {
      INT j = GMap[N].moveNo & 0x0FFF;               // Game move number (index in Game[]).
      if (j > Game()->currMove) futureLineCount++;

      RGB_COLOR *c = (j > Game()->currMove ? &color_MdGray : (Active() ? &color_Black : &color_MdGray));
      SetForeColor(c);

      if (GMap[N].moveNo & gameMap_Special)       // Line contains header (title and/or game info)
      {
         LONG gameNo = ((GameWindow*)Window())->colGameNo;
         ::DrawGameSpecial(this, lineWidth, GMap[N].txLine, &(Game()->Info), gameNo, false);
      }
      else if (GMap[N].moveNo & gameMap_White)            // Line contains a WHITE move (and perhaps a black one too).
      {
         SetFontStyle(fontStyle_Bold);
            TextEraseTo(numInsetH);
            DrawNumR(j/2 + Game()->Init.moveNo, 3); DrawStr("."); TextEraseTo(move1InsetH);

            BOOL hiliteCurr = (Prefs.Games.hiliteCurrMove && j == Game()->currMove && c == &color_Black);
            if (hiliteCurr) SetBackColor(&hiColor);
            ::DrawGameMove(this, &Game()->Record[j]);
            if (hiliteCurr) SetBackColor(&color_White);
            TextEraseTo(move2InsetH);

            if (j + 1 > Game()->currMove) SetForeColor(c = &color_MdGray);
            if (GMap[N].moveNo & gameMap_Black)
            {
               BOOL hiliteCurr = (Prefs.Games.hiliteCurrMove && j + 1 == Game()->currMove && c == &color_Black);
               if (hiliteCurr) SetBackColor(&hiColor);
               ::DrawGameMove(this, &Game()->Record[j + 1]);
               if (hiliteCurr) SetBackColor(&color_White);
            }
            else if (N < linesTotal - 1 && j < Game()->lastMove)
               DrawStr(". . .");
         SetFontStyle(fontStyle_Plain);
      }
      else if (GMap[N].moveNo & gameMap_Black)       // Line contains a BLACK move only.
      {
         SetFontStyle(fontStyle_Bold);
            TextEraseTo(numInsetH);
            DrawNumR((j - 1)/2 + Game()->Init.moveNo, 3); DrawStr("."); TextEraseTo(move1InsetH);
            DrawStr(". . ."); TextEraseTo(move2InsetH);

            BOOL hiliteCurr = (Prefs.Games.hiliteCurrMove && j == Game()->currMove && c == &color_Black);
            if (hiliteCurr) SetBackColor(&hiColor);
            ::DrawGameMove(this, &Game()->Record[j]);
            if (hiliteCurr) SetBackColor(&color_White);

         SetFontStyle(fontStyle_Plain);
      }
      else if (Game()->GameMapContainsDiagram(GMap, N))
      {
         TextEraseTo(bounds.Width()/2 - 5);
         INT hpen, vpen;
         GetPenPos(&hpen, &vpen);
         CRect r(hpen, vpen - 16, hpen + 16, vpen);
      	DrawIcon(icon_Position10x10, r, iconTrans_None);
      	MovePen(10, 0);
      }
      else                              // Line contains a LINE of annotation text/data.
      {
         CHAR s[500];
         INT  lineNo     = GMap[N].txLine & 0x0FFF;
         INT  lineLen    = Game()->GetAnnotationLine(j, lineNo, s);
         BOOL isLastLine = (lineNo == Game()->GetAnnotationLineCount(j) - 1);
         ::DrawTextLine(this, s, lineLen, lineWidth, isLastLine);
      }
   }

   TextEraseTo(bounds.right - 18);
} /* GameDataView::DrawLine */


void DrawTextLine (CView *view, CHAR *s, INT n, INT lineWidth, BOOL isLastLine)
{
   // If the first 3 chars are blank, treat it as a tab-indent
   if (s[0] == '\t' && n > 1)
   {  s++; n--;
      view->TextErase(AnnCharWidth['\t']);
      lineWidth -= AnnCharWidth['\t'];
   }

   if (isLastLine || IsNewLine(s[n]))
      view->DrawStr(s, 0, n);
   else
   {
//    INT spaceCount = 0;
      if (s[n - 1] == ' ') n--;
//    for (INT i = 0; i < n; i++)
//       if (s[i] == ' ') spaceCount++;
//    if (spaceCount > 0)
//       view->SetTextSpacing(lineWidth - view->StrWidth(s, 0, n), spaceCount);
      view->DrawStr(s, 0, n);
//    view->ResetTextSpacing();
   }
} /* DrawTextLine */


void DrawGameMove (CView *view, MOVE *m, BOOL printing)
{
   CHAR s[gameMoveStrLen];
   CalcGameMoveStr(m, s);
   DrawGameMoveStr(view, m, s, printing);
} // DrawGameMove


void DrawGameMoveStr (CView *view, MOVE *m, CHAR *s, BOOL printing)
{
   if (! Prefs.Notation.figurine ||
       Prefs.Notation.moveNotation == moveNot_Descr ||
       pieceType(m->piece) == pawn ||
       m->type != mtype_Normal
      )
      view->DrawStr(s);
   else
   {
      INT h, v;
      view->GetPenPos(&h, &v);
      if (! printing)
      {  CRect dst(h, v - 10, h + 10, v + 2);
         CRect src(0, 0, 10, 12);
         src.Offset((pieceType(m->piece) - 1)*10, (pieceColour(m->piece) == white ? 0 : 12));
         view->DrawBitmap(figurineBmp, src, dst, bmpMode_Copy);
         view->MovePen(10, 0);
      }
      else
      {  CRect dst(h - 2, v - 10, h + 10, v + 2);
         view->DrawPict(9000 + pieceType(m->piece) + (pieceColour(m->piece) == black ? 10 : 0), dst);
         view->MovePen(10, 0);
      }
      view->DrawStr(s + 1);
   }
} // DrawGameMoveStr

/*--------------------------------------- Draw Special Lines -------------------------------------*/

static void CalcPlayerNameELO (CHAR *name, INT elo, CHAR *dest);


void DrawGameSpecial (CView *view, INT lineWidth, INT type, GAMEINFO *Info, LONG gameNo, BOOL printing)
{
   CHAR  temp[100];
   CHAR  *tag = nil, *s = nil;
   INT   h, v;

   view->GetPenPos(&h, &v);
   CRect r(h, v - view->FontAscent(), h + lineWidth, v + view->FontDescent());

   switch (type)
   {
      case gameMap_SpecialChapter :
         view->SetFontStyle(fontStyle_Bold);
         view->SetFontFace(printing ? font_Helvetica : font_Geneva);
         view->SetFontSize(printing ? 18 : 12);
         r.Set(h, v - view->FontAscent(), h + lineWidth, v + view->FontDescent());
         view->DrawStr(Info->heading, r);
         break;
      case gameMap_SpecialSection :
         view->SetFontStyle(fontStyle_Bold);
         view->SetFontFace(printing ? font_Helvetica : font_Geneva);
         view->SetFontSize(printing ? 14 : 10);
         r.Set(h, v - view->FontAscent(), h + lineWidth, v + view->FontDescent());
         view->DrawStr(Info->heading, r);
         break;
      case gameMap_SpecialGmTitle :
         view->SetFontStyle(fontStyle_Italic);
         view->SetFontFace(printing ? font_Helvetica : font_Geneva);
         view->SetFontSize(printing ? 14 : 10);
         Format(temp, "Game %d", gameNo + 1);
         r.Set(h, v - view->FontAscent(), h + lineWidth, v + view->FontDescent());
         view->DrawStr(temp, r);
         break;

      case gameMap_SpecialWhite  : tag = "White";  s = temp; CalcPlayerNameELO(Info->whiteName, Info->whiteELO, temp); break;
      case gameMap_SpecialBlack  : tag = "Black";  s = temp; CalcPlayerNameELO(Info->blackName, Info->blackELO, temp); break;
      case gameMap_SpecialEvent  : tag = "Event";  s = Info->event; break;
      case gameMap_SpecialSite   : tag = "Site";   s = Info->site; break;
      case gameMap_SpecialDate   : tag = "Date";   s = Info->date; break;
      case gameMap_SpecialRound  : tag = "Round";  s = Info->round; break;
      case gameMap_SpecialResult : tag = "Result"; s = temp; CalcInfoResultStr(Info->result, temp); break;
      case gameMap_SpecialECO    : tag = "ECO";    s = Info->ECO; break;
   }

   if (tag && s)  // If info line
   {
      view->SetFontStyle(fontStyle_Bold);
      view->DrawStr(tag);
      if (! printing) view->TextEraseTo(h + 45);

      view->SetFontStyle(fontStyle_Plain);
      r.left += 46;
      view->DrawStr(s, r);
      if (! printing) view->TextEraseTo(r.right);
   }

   // Restore to normal font for game view/print:
   view->SetFontStyle(fontStyle_Plain);
   view->SetFontFace(printing ? font_Times : font_Geneva);
   view->SetFontSize(printing ? 12 : 10);
} /* DrawGameSpecial */


static void CalcPlayerNameELO (CHAR *name, INT elo, CHAR *dest)
{
   if (elo <= 0)
      CopyStr(name, dest);
   else
      Format(dest, "%s [%d ELO]", name, elo);
} // CalcPlayerNameELO


/**************************************************************************************************/
/*                                                                                                */
/*                                        GAME HEADER VIEW                                        */
/*                                                                                                */
/**************************************************************************************************/

GameHeaderView::GameHeaderView (CViewOwner *parent, CRect frame)
 : SpringHeaderView(parent,frame,true,Prefs.GameDisplay.gameHeaderClosed)
{
} /* GameHeaderView::GameHeaderView */


void GameHeaderView::HandleUpdate (CRect updateRect)
{
   SpringHeaderView::HandleUpdate(updateRect);

   CRect r = bounds;
   r.Inset(1, 1);

   // First draw top line
   RefreshGameStatus();

   // If the header is "open" then draw divider line and game info
   if (! Closed())
   {
      // Draw game info
      SetFontForeColor();
      SetBackColor(&color_LtGray);
      SetFontSize(9);

      GAMEINFO *Info = &(Game()->Info);
      GAMEINFOFILTER *Filter = &(((GameWindow*)Window())->infoFilter);
      CHAR resultStr[10];

      r.top += headerViewHeight - 1;
      r.bottom -= 3;
      INT dv = springHeaderLineHeight; //FontHeight();
      INT h = r.left + 5;
      INT v = r.top - 1 + dv;
      CalcInfoResultStr(Info->result, resultStr);

      CHAR wname[100], bname[100];
      if (Info->whiteELO <= 0) CopyStr(Info->whiteName, wname);
      else Format(wname, "%s [%d ELO]", Info->whiteName, Info->whiteELO);
      if (Info->blackELO <= 0) CopyStr(Info->blackName, bname);
      else Format(bname, "%s [%d ELO]", Info->blackName, Info->blackELO);

      SetClip(r);
      if (Filter->players) DrawStrPair(h, v, "White",  wname), v += dv;
      if (Filter->players) DrawStrPair(h, v, "Black",  bname), v += dv;
      if (Filter->event)   DrawStrPair(h, v, "Event",  Info->event), v += dv;
      if (Filter->site)    DrawStrPair(h, v, "Site",   Info->site), v += dv;

      if (Filter->date)
      {  DrawStrPair(h, v, "Date", Info->date);
         if (Filter->round) DrawStrPair(h + r.Width()/2, v, "Round",  Info->round);
         v += dv;
      }
      else if (Filter->round)
         DrawStrPair(h, v, "Round", Info->round), v += dv;

      if (Filter->result)
      {  DrawStrPair(h, v, "Result", resultStr);
         if (Filter->ECO) DrawStrPair(h + r.Width()/2, v, "ECO", Info->ECO);
         v += dv;
      }
      else if (Filter->ECO) DrawStrPair(h, v, "ECO", Info->ECO), v += dv;

      ClrClip();

      SetFontSize(10);
      SetFontStyle(fontStyle_Plain);
   }
} /* GameHeaderView::HandleUpdate */


BOOL GameHeaderView::HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)
{
   if (pt.v > headerViewHeight)
      Window()->HandleMessage(display_GameRecord);
   else
      SpringHeaderView::HandleMouseDown(pt, modifiers, doubleClick);

   return true;
} /* GameHeaderView::HandleMouseDown */


void GameHeaderView::HandleToggle (BOOL closed)
{
   Prefs.GameDisplay.gameHeaderClosed = closed;
   ((GameDataView*)Parent())->ToggleHeader(closed);
} /* GameHeaderView::HandleToggle */


void GameHeaderView::RefreshGameStatus (void)
{
   CHAR str[100];
   Game()->CalcStatusStr(str);
   DrawHeaderStr(str);
} /* GameHeaderView::RefreshGameStatus */


/**************************************************************************************************/
/*                                                                                                */
/*                                        GAME FOOTER VIEW                                        */
/*                                                                                                */
/**************************************************************************************************/

GameFooterView::GameFooterView (CViewOwner *owner, CRect frame)
 : DataHeaderView(owner, frame, false, true, 1, HCTab)
{
   CopyStr("", s);
   HCTab[0].text   = s;
   HCTab[0].iconID = 0;
   HCTab[0].width  = -1;
} /* GameFooterView::GameFooterView */

/*--------------------------------------- Event Handling -----------------------------------------*/

void GameFooterView::HandleUpdate (CRect updateRect)
{
   CGame *game = ((GameWindow*)(Window()))->game;
   CHAR eco[libECOLength + 1], comment[libCommentLength + 1];

   LIB_CLASS libClass = PosLib_Probe(game->player, game->Board);
   if (libClass < libClass_First || libClass > libClass_Last)
      HCTab[0].iconID = icon_LibUnclass;
   else if (libClass == libClass_Unclassified)
      HCTab[0].iconID = 0;
   else
      HCTab[0].iconID = icon_LibClass1 + libClass - 1;

   if (PosLib_ProbeStr(game->player, game->Board, eco, comment))
      Format(s, "%s%s%s", eco, (eco[0] && comment[0] ? "  " : ""), comment);
   else
      s[0] = 0;

   DataHeaderView::HandleUpdate(updateRect);

   if (RunningOSX())
   {  SetForeColor(&color_MdGray);
      DrawRectFrame(bounds);
   }
} /* GameFooterView::HandleUpdate */
