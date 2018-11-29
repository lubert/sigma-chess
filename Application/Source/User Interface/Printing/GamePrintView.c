/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAMEPRINTVIEW.C                                                                      */
/* Purpose : This module implements the actual low level printing of a single page of a game or   */
/*           collection.                                                                          */
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

#include "GamePrintView.h"
#include "GameView.h"

#define columnSpacing       50   // Space between columns.
#define headerHeight        30   // Height of header area
#define footerHeight        20   // Height of footer area    
#define textLineHeight      14

#define hinset              20
#define hinset1             55
#define hinset2             135

#define printSqWidth        21


/**************************************************************************************************/
/*                                                                                                */
/*                                           CONSTRUCTOR                                          */
/*                                                                                                */
/**************************************************************************************************/

CGamePrintView::CGamePrintView (CViewOwner *owner, CRect frame, INT vres, CGame *vGame, GAMEMAP vGMap[])
 : CView(owner, frame)
{
   game = vGame;
   GMap = vGMap;

   columnWidth = (bounds.Width() - columnSpacing)/2;
   v0 = headerHeight + 25; // Base line of top text line.
   
   INT lineAreaHeight = bounds.Height() - v0 - footerHeight;
   pageLines = (lineAreaHeight*72L/vres)/textLineHeight;
} /* CGamePrintView::CGamePrintView */


/**************************************************************************************************/
/*                                                                                                */
/*                                          PRINT ROUTINES                                        */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Print Page Header/Footer ----------------------------------*/

void CGamePrintView::PrintPageHeader (CHAR *title, INT pageNo)
{
   SetFontFace(font_Times);
   SetFontSize(14);
   SetFontStyle(fontStyle_Bold | fontStyle_Italic);

   MovePenTo(0, headerHeight - 8);
   if (Prefs.Misc.printPageHeaders) DrawStr(title);
   MovePenTo(0, headerHeight);
   DrawLineTo(bounds.right, headerHeight);

   SetFontSize(12);
   SetFontStyle(fontStyle_Plain);

   MovePenTo(bounds.Width()/2, bounds.bottom);
   DrawNum(pageNo);
} /* CGamePrintView::PrintPageHeader */

/*------------------------------------- Print Single Line ------------------------------------*/
// Prints game line N (0 <= N < gameMapLines), at page position (h,v).

void CGamePrintView::PrintGameLine (INT N, INT Nmax, INT column, INT line, LONG gameNo)
{
   if (N >= Nmax) return;

   INT j = GMap[N].moveNo & 0x0FFF;               // Game move number (index in Game[]).

   INT h = column*(columnWidth + columnSpacing);
   INT v = v0 + textLineHeight*line;
   MovePenTo(h, v);

   SetForeColor(&color_Black);
   SetStandardFont();

   if (GMap[N].moveNo & gameMap_Special)       // Line contains header (title and/or game info)
   {
      ::DrawGameSpecial(this, columnWidth, GMap[N].txLine, &game->Info, gameNo, true);
   }
   else
   if (GMap[N].moveNo & gameMap_White)            // Line contains a WHITE move (and perhaps a black one too).
   {
      MovePen(hinset, 0);
      SetFontStyle(fontStyle_Bold);
         DrawNumR(j/2 + game->Init.moveNo, 3); MovePenTo(h + hinset1, v);
         ::DrawGameMove(this, &game->Record[j], true); MovePenTo(h + hinset2, v);
         game->RedoMove(false);
         if (GMap[N].moveNo & gameMap_Black)
            ::DrawGameMove(this, &game->Record[j + 1], true),
            game->RedoMove(false);
         else if (N < Nmax - 1 && j < game->lastMove)
            DrawStr(". . .");
      SetFontStyle(fontStyle_Plain);
   }
   else if (GMap[N].moveNo & gameMap_Black)       // Line contains a BLACK move only.
   {
      MovePen(hinset, 0);
      SetFontStyle(fontStyle_Bold);
         DrawNumR((j - 1)/2 + game->Init.moveNo, 3); MovePenTo(h + hinset1, v);
         DrawStr(". . ."); MovePenTo(h + hinset2, v);
         ::DrawGameMove(this, &game->Record[j], true);
         game->RedoMove(false);
      SetFontStyle(fontStyle_Plain);
   }
   else                              // Line contains a LINE of annotation text/data.
   {
      CHAR s[500];
      INT  lineNo     = GMap[N].txLine & 0x0FFF;
      INT  lineLen    = game->GetAnnotationLine(j, lineNo, s);
      BOOL isLastLine = (lineNo == game->GetAnnotationLineCount(j) - 1);
      DrawTextLine(this, s, lineLen, columnWidth, isLastLine);
   }
} /* CGamePrintView::PrintGameLine */

/*------------------------------------------ Print Diagram ---------------------------------------*/
// This routine prints a diagram of the current Board[] configuration.

void CGamePrintView::PrintDiagram (INT column, INT line)
{
   INT h = column*(columnWidth + columnSpacing) + hinset;
   INT v = v0 + textLineHeight*line - 3;

   CRect r(h, v, h + 8*printSqWidth, v + 8*printSqWidth);
   DrawRectFrame(r);

   r.right += 12;
   r.left = r.right - 7;

   if (game->player == white)
      r.top = r.bottom - 7,
      DrawRectFrame(r);
   else
      r.bottom = r.top + 7,
      DrawRectFill(r, &color_Black);

   for (INT rank = 7; rank >= 0; rank--)
      for (INT file = 0; file <= 7; file++)
      {
         r.left = file*printSqWidth + h;
         r.top  = (7 - rank)*printSqWidth + v;
         r.right = r.left + printSqWidth;
         r.bottom = r.top + printSqWidth;

         PIECE p  = game->Board[(rank << 4) + file];
         INT   id = 9000 + pieceType(p);
         if (pieceColour(p) == black) id += 10;

         if (even(file + rank)) id += 100;

         if (id != 9000)
            DrawPict(id, r);
      }
} /* CGamePrintView::PrintDiagram */


/**************************************************************************************************/
/*                                                                                                */
/*                                             UTILITY                                            */
/*                                                                                                */
/**************************************************************************************************/

void CGamePrintView::SetStandardFont (void)
{
   SetFontFace(font_Times);
   SetFontSize(11);
   SetFontStyle(fontStyle_Plain);
} /* CGamePrintView::SetStandardFont */
