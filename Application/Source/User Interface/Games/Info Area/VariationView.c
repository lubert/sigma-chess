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

#include "VariationView.h"
#include "GameView.h"
#include "GameWindow.h"

//#include "Debug.h"  //###

#define hInset           4
//#define lineWidth        (9*digitWidth - 3)
#define textLineHeight   13  //FontHeight()


/**************************************************************************************************/
/*                                                                                                */
/*                                         VARIATION VIEW                                         */
/*                                                                                                */
/**************************************************************************************************/

VariationView::VariationView (CViewOwner *parent, CRect frame)
 : DataView(parent,frame)
{
   win = (GameWindow*)Window();
   Analysis = &(win->Analysis);

   // Calc the data rect & coordinates:
   CalcCoord();

   Reset();
} /* VariationView::VariationView */


void VariationView::CalcCoord (void)
{
   CRect headerRect;
   CalcDimensions(&headerRect, &dataRect);

   dataRect.top = headerRect.top;

   maxLines = (dataRect.Height() - 5)/textLineHeight;

   for (INT i = 1; i <= uci_MaxMultiPVcount; i++)
      CalcMultiPVRect(i, &MultiPVRect[i], &PVLines[i]);
} /* VariationView::CalcCoord */


void VariationView::CalcMultiPVRect (INT pvNo, CRect *r, INT *pvLines)
{
   *r = dataRect;
   r->Inset(hInset, 2);
   r->top += 2;

   INT pvHeight = r->Height()/win->GetMultiPVcount();
   if (pvHeight < textLineHeight + 4)
      pvHeight = textLineHeight + 4;

   r->top += (pvNo - 1)*pvHeight;
   r->bottom = r->top + pvHeight - 1;

   *pvLines = (r->bottom < dataRect.bottom - 2 ? (pvHeight - 4)/textLineHeight : 0);
} // VariationView::CalcMultiPVRect

/*--------------------------------------- Event Handling -----------------------------------------*/

void VariationView::HandleUpdate (CRect updateRect)
{
   if (win->mode3D) return;

   DataView::HandleUpdate(updateRect);

   // Draw Separator lines
	SetForeColor(&color_LtGray);

	if (win->varDisplayVer)
	{
      MovePenTo(dataRect.Width()/2 + 1, dataRect.top + 1);
      DrawLine(0, dataRect.Height() - 2);
   }
   else
   {
      INT imax = win->GetMultiPVcount();
      for (INT i = 2; i <= imax && PVLines[i] > 0; i++)
      {  MovePenTo(hInset, MultiPVRect[i].top - 1);
         DrawLine(dataRect.Width() - 2*hInset + 1, 0);
      }
   }

   // Draw the actual variations
   SetFontForeColor();
   DrawMainLine(0);
// DrawCurrLine();
} /* VariationView::HandleUpdate */


void VariationView::HandleActivate (BOOL wasActivated)
{
   if (win->mode3D) return;

   SetFontForeColor();
   DrawMainLine(0);
} /* VariationView::HandleActivate */


BOOL VariationView::HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick)
{
   if (doubleClick) return false;  //### or "go to" that move.

   if (modifiers & modifier_Control)
   {
   }
   else if (modifiers & modifier_Command)
   {
      ShowHelpTip("This is the Analysis window, which shows the best and the current variation.");
   }

   return true;
} /* VariationView::HandleMouseDown */


void VariationView::HandleResize (void)
{
   CalcCoord();
} /* VariationView::HandleResize */

/*-------------------------------------- Set Main/Curr Line --------------------------------------*/

void VariationView::Reset (void)
{
   if (win->mode3D) return;
   Redraw();
} /* VariationView::Reset */


void VariationView::SetMainLine (INT pvNo)
{
   DrawMainLine(pvNo);
} /* VariationView::SetMainLine */

/*-------------------------------------- Main Line Drawing ---------------------------------------*/
// If pvNo = 0 then all lines should be drawn

void VariationView::DrawMainLine (INT pvNo)
{
   if (win->isRated || ! Visible() || win->mode3D) return;

   if (win->varDisplayVer)
   {
      if (pvNo == 0)  // 0 -> All
      {  INT imax = Min(win->GetMultiPVcount(), 2);
         for (INT i = 1; i <= imax; i++)
            DrawMainLine_Vertical(i);
      }
      else if (pvNo <= 2)
      	DrawMainLine_Vertical(pvNo);
   }
   else
   {
      if (pvNo == 0)  // 0 -> All
      {  INT imax = win->GetMultiPVcount();
         for (INT i = 1; i <= imax; i++)
            DrawMainLine_Horizontal(i);
      }
      else
         DrawMainLine_Horizontal(pvNo);
   }
} /* VariationView::DrawMainLine */

/*----------------------------------- Vertical/Standard Layout -----------------------------------*/

void VariationView::DrawMainLine_Vertical (INT pvNo)
{
   //--- Erase PV rectangle ---
   CRect r = dataRect;
   if (pvNo == 1) r.right -= r.Width()/2; else r.left += r.Width()/2;
   r.Inset(hInset, 2);
   DrawRectErase(r);

	//--- Calc coordinates and pen location ---
   INT v = r.top + textLineHeight;
   INT hm = r.left;
   INT hw = hm + 30;
   INT hb = hm + 80;
   INT line = 0;

   SetForeColor((Active() && (pvNo == 1 || Analysis->depthPV[1] == Analysis->depthPV[pvNo])) ? &color_Black : &color_MdGray);

   //--- If MultiPV then precede PV with pvno and score ---
   MOVE *M = Analysis->PV[pvNo];
//	CHAR s[50];   // Utility string

   if (win->GetMultiPVcount() > 1)
   {
	   SetFontStyle(fontStyle_Bold);
// 	MovePenTo(hm, v); Format(s, "[%d]", pvNo); DrawStr(s);
   	MovePenTo(hw, v);
	   if (isNull(M[0]))
	      DrawStr("-");
	   else
	      VariationView::DrawScore(hw, v, pvNo);
	   SetFontStyle(fontStyle_Plain);
	   v += textLineHeight;
	   line++;
   }

   if (isNull(M[0])) return;

   //--- Draw actual PV ---
	COLOUR player = pieceColour(M[0].piece);
	INT moveNo = Analysis->initMoveNo + (Analysis->gameMove + (Analysis->initPlayer >> 4))/2;

   // Draw initial move number (and ... string if black)
   if (player == black)
   {
      MovePenTo(hm, v); DrawNumR(moveNo++, 3, false); DrawChr('.');
      MovePenTo(hw, v); DrawStr("...");
   }

   // Draw moves
   INT i = 0;

   while (! isNull(M[i]) && line < maxLines)
   {
   	if (player == white)
   	{  MovePenTo(hm, v); DrawNumR(moveNo++, 3, false); DrawChr('.');
         MovePenTo(hw, v);
   	}
   	else
   	{  MovePenTo(hb, v); 
   	   v += textLineHeight;
   	   line++;
   	}

		CHAR s[gameMoveStrLen];
      CalcGameMoveStr(&M[i], s);
      ::DrawGameMoveStr(this, &M[i], s);

      i++;
      player = black - player;
   }
} // VariationView::DrawMainLine_Vertical

/*--------------------------------------- Horizontal Layout --------------------------------------*/

void VariationView::DrawMainLine_Horizontal (INT pvNo)
{
   if (PVLines[pvNo] <= 0) return;

   //--- Erase PV rectangle ---
   CRect pvRect = MultiPVRect[pvNo];

   DrawRectErase(pvRect);

	//--- Calc coordinates and pen location ---
   INT v    = pvRect.top + textLineHeight - 1; // Vertical coordinate of first text line for PV (- descent)
	INT h0   = pvRect.left;
	INT hmax = pvRect.right - hInset;
	
	MovePenTo(h0, v);
   SetForeColor((Active() && (pvNo == 1 || Analysis->depthPV[1] == Analysis->depthPV[pvNo])) ? &color_Black : &color_MdGray);

   //--- If MultiPV then precede PV with pvno and score ---
   MOVE *M = Analysis->PV[pvNo];
	CHAR s[50];   // Utility string

   if (win->GetMultiPVcount() > 1)
   {
	   SetFontStyle(fontStyle_Bold);
//    Format(s, "[%d] ", pvNo); DrawStr(s);
	   if (isNull(M[0]))
	      DrawStr("-");
	   else
	   {  VariationView::DrawScore(h0, v, pvNo);
	      DrawStr(" : ");
	   }
	   SetFontStyle(fontStyle_Plain);
   }

   if (isNull(M[0])) return;

   //--- Draw actual PV ---
	COLOUR player = pieceColour(M[0].piece);
	INT    moveNo = Analysis->initMoveNo + (Analysis->gameMove + (Analysis->initPlayer >> 4))/2;

   // Draw initial move number (and ... string if black)
	Format(s, (player == white ? "%d." : "%d..."), moveNo);
	DrawStr(s);

   // Draw moves
   INT pvLines = PVLines[pvNo];
	INT i = 0;

	while (! isNull(M[i]))
	{
		INT hpen, vpen;

      //--- Draw move number before White moves ---
		if (i > 0 && player == white)
		{
		   Format(s, "%d.", ++moveNo);

		   GetPenPos(&hpen, &vpen);
		   if (hpen + StrWidth(s) > hmax) { MovePenTo(h0, v += textLineHeight); if (--pvLines <= 0) return; }

	      DrawStr(s);
		}

      //--- Draw the Move ---
      CalcGameMoveStr(&M[i], s);

		GetPenPos(&hpen, &vpen);
		INT width = StrWidth(s);
      if (Prefs.Notation.figurine && pieceType(M[i].piece) != pawn && M[i].type == mtype_Normal)  // Extra width if figurine
         width += 4;
		if (hpen + width > hmax) { MovePenTo(h0, v += textLineHeight); if (--pvLines <= 0) return; }

      ::DrawGameMoveStr(this, &M[i], s);

      //--- Draw separator and toggle player ---
      DrawStr(" ");
      player = black - player;
      i++;
	}
} // VariationView::DrawMainLine_Horizontal

/*---------------------------------------- Draw Score -------------------------------------------*/

void VariationView::DrawScore (INT h, INT v, INT pvNo)
{
   if (Prefs.AnalysisFormat.scoreNot == scoreNot_Glyph &&
       Analysis->scoreType[pvNo] == scoreType_True &&
       Analysis->score[pvNo] > mateLoseVal && Analysis->score[pvNo] < mateWinVal)
   {
      CRect r(0,0,16,16);
      r.Offset(h, v - 11);

      INT score = CheckAbsScore(Analysis->player, Analysis->score[pvNo]);
      INT p = Analysis->player;
      LIB_CLASS c;
      if (score >= 150)      c = libClass_WinningAdvW, r.Offset(1,0);
      else if (score >= 50)  c = libClass_ClearAdvW;
      else if (score >= 25)  c = libClass_SlightAdvW;
      else if (score > -25)  c = libClass_Level;
      else if (score > -50)  c = libClass_SlightAdvB;
      else if (score > -150) c = libClass_ClearAdvB;
      else                   c = libClass_WinningAdvB, r.Offset(1,0);

      DrawIcon(369 + c, r, (Active() ? iconTrans_None : iconTrans_Disabled));
      MovePen(16, 0);
   }
   else
   {
      CHAR s[20];
      ::CalcScoreStr(s, CheckAbsScore(Analysis->player, Analysis->score[pvNo]), Analysis->scoreType[pvNo]);
      DrawStr(s);
   }
} // VariationView::DrawScore

/*------------------------------------------- Utility --------------------------------------------*/
/*
void VariationView::DrawVarMove (MOVE *m, CHAR *moveStr)
{
   if (! Prefs.Notation.figurine || pieceType(m->piece) == pawn || m->type != mtype_Normal)
   {
      DrawStr(moveStr);
   }
   else
   {
		INT h, v;
	   GetPenPos(&h, &v);

      CRect dst(h, v - 10, h + 10, v + 2);
      CRect src(0, 0, 10, 12);
      src.Offset((pieceType(m->piece) - 1)*10, (pieceColour(m->piece) == white ? 0 : 12));
      DrawBitmap(figurineBmp, src, dst, bmpMode_Copy);
      MovePen(10, 0);
      DrawStr(moveStr + 1);
   }
} // VariationView::DrawVarMove
*/
