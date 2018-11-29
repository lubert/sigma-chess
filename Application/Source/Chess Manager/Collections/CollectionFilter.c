/**************************************************************************************************/
/*                                                                                                */
/* Module  : CollectionFilter.c                                                                   */
/* Purpose : This module implements the game collections filter.                                  */
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

#include "CDialog.h"

#include "Collection.h"
#include "Move.f"
#include "Board.f"
#include "HashCode.f"


/**************************************************************************************************/
/*                                                                                                */
/*                                           FILTERING                                            */
/*                                                                                                */
/**************************************************************************************************/

static BOOL Filter_Str (CHAR *s, INT cond, CHAR *fs);
static BOOL Filter_PosExact (POS_FILTER *pf, CGame *game);
static BOOL Filter_PosPartial (POS_FILTER *pf, CGame *game);

BOOL SigmaCollection::FilterGame (ULONG g)
{
   if (! useFilter) return true;

   if (filter.useLineFilter || filter.usePosFilter)  // Entire game needed if line or pos filter
   {
      if (GetGame(g,game,true) != colErr_NoErr) return false;  // Get RAW game (no flags, glyphs)
   }
   else
   {
      if (GetGameInfo(g) != colErr_NoErr) return false;
   }

   //--- First check Game Info filter ---

   GAMEINFO *info = &(game->Info);
   BOOL     lineCondIs = true;

   for (INT i = 0; i < filter.count; i++)
   {
      if (filter.Field[i] == filterField_WhiteOrBlack)
      {
         if (! Filter_Str(info->whiteName,filter.Cond[i],filter.Value[i]) &&
             ! Filter_Str(info->blackName,filter.Cond[i],filter.Value[i]))
            return false;
      }
      else if (filter.Field[i] == filterField_OpeningLine)
      {
         lineCondIs = (filter.Cond[i] == filterCond_Is);
      }
      else if (filter.Field[i] != filterField_Position)
      {
	      CHAR *s, tmp[100];

	      switch (filter.Field[i])
	      {
	         case filterField_White     : s = info->whiteName; break;
	         case filterField_Black     : s = info->blackName; break;
	         case filterField_Event     : s = info->event; break;
	         case filterField_Site      : s = info->site; break;
	         case filterField_Date      : s = info->date; break;
	         case filterField_Round     : s = info->round; break;
	         case filterField_Result    : s = tmp; CalcInfoResultStr(info->result,s);  break;
	         case filterField_ECO       : s = info->ECO; break;
	         case filterField_Annotator : s = info->annotator; break;
	         case filterField_WhileELO  : s = tmp; NumToStr(info->whiteELO, s); break;
	         case filterField_BlackELO  : s = tmp; NumToStr(info->blackELO, s); break;
	      }

	      if (! Filter_Str(s,filter.Cond[i],filter.Value[i])) return false;
      }
   }

   //--- Then check opening line filter ---

   if (filter.useLineFilter)
   {
      if (filter.lineLength > game->lastMove)
      {
         if (lineCondIs) return false;   // Game too short
      }
      else
      {
         MOVE *m1 = filter.Line;
         MOVE *m2 = game->Record;
         INT  j;
         for (j = 1; j <= filter.lineLength && EqualMove(++m1,++m2); j++);
         if ((j > filter.lineLength) != lineCondIs) return false;
      }
   }

   //--- Finally check position filter ---

   if (filter.usePosFilter)
   {
      POS_FILTER *pf = &(filter.posFilter);
      if (pf->exactMatch ? ! Filter_PosExact(pf,game) : ! Filter_PosPartial(pf,game))
         return false;
   }

   return true;
} /* SigmaCollection::FilterGame */

/*------------------------------------------- Utility --------------------------------------------*/

static BOOL Filter_Str (CHAR *s, INT cond, CHAR *fs)
{
   switch (cond)
   {
      case filterCond_Is :
         return ::SameStr(s, fs);
      case filterCond_IsNot :
         return ! ::SameStr(s, fs);
      case filterCond_StartsWith :
         while (*s && *fs)
            if (! ::SameChar(*(s++), *(fs++))) return false;
         return (!*fs);
      case filterCond_EndsWith :
         INT sLen = StrLen(s);
         INT fsLen = StrLen(fs);
         return (sLen >= fsLen && ::SameStr(&s[sLen - fsLen], fs));
      case filterCond_Contains :
         return ::SearchStr(s, fs, false);

      case filterCond_Less :
      case filterCond_Before :
         return (::CompareStr(s, fs) < 0);
      case filterCond_Greater :
      case filterCond_After :
         return (::CompareStr(s, fs) > 0);
      case filterCond_LessEq :
         return (::CompareStr(s, fs) <= 0);
      case filterCond_GreaterEq :
         return (::CompareStr(s, fs) >= 0);
   }

   return false;
} /* Filter_Str */

/*---------------------------------------- Position Filter ---------------------------------------*/

static BOOL Filter_PosExact (POS_FILTER *pf, CGame *game)
{
   INT  jmin = (pf->checkMoveRange ? 2*pf->minMove - 2 : 0);
   INT  jmax = (pf->checkMoveRange ? Min(game->lastMove, 2*pf->maxMove) : game->lastMove);
   HKEY hkey = game->DrawData[0].hashKey;

   INT  wCountTotal, bCountTotal;
   INT  wCountPawns, bCountPawns;

   //--- Compute the piece counters ---

   if (! game->Init.wasSetup)
   {
      wCountTotal = bCountTotal = 16;
      wCountPawns = bCountPawns = 8;
   }
   else
   {
      wCountTotal = bCountTotal = 0;
      wCountPawns = bCountPawns = 0;
      
      PIECE p;
      for (SQUARE sq = a1; sq <= h8; sq++)
         if (onBoard(sq) && (p = game->Board[sq]))
            if (pieceColour(p) == white)
            {  wCountTotal++;
               if (p == wPawn) wCountPawns++;
            }
            else
            {  bCountTotal++;
               if (p == bPawn) bCountPawns++;
            }

      if (wCountTotal < pf->wCountTotal || bCountTotal < pf->bCountTotal) return false;
      if (wCountPawns < pf->wCountPawns || bCountPawns < pf->bCountPawns) return false;
   }

   //--- Replay the game and search for matching positions ---

   for (INT j = 0; j <= jmax; j++)
   {
      if (j > 0)
      {
         game->RedoMove(false);

         MOVE *m = &(game->Record[j]);
         hkey ^= ::HashKeyChange(&Global, m);

	      // Check piece count limits (optimization):

	      if (pieceColour(m->piece) == white)
         {
            if (m->cap)
            {  if (--bCountTotal < pf->bCountTotal) return false;
               if (m->cap == bPawn && --bCountPawns < pf->bCountPawns) return false;
            }
            else if (m->type == mtype_EP)
            {  if (--bCountTotal < pf->bCountTotal || --bCountPawns < pf->bCountPawns) return false;
            }

            if (isPromotion(*m))
            {  if (--wCountPawns < pf->wCountPawns) return false;
            }
         }
         else
         {
            if (m->cap)
            {  if (--wCountTotal < pf->wCountTotal) return false;
               if (m->cap == wPawn && --wCountPawns < pf->wCountPawns) return false;
            }
            else if (m->type == mtype_EP)
            {  if (--wCountTotal < pf->wCountTotal || --wCountPawns < pf->wCountPawns) return false;
            }

            if (isPromotion(*m))
            {  if (--bCountPawns < pf->bCountPawns) return false;
            }
	      }
      }

      // Check if position matches on hash key and side to move. If so verify by doing a real
      // board compare.

      if (j >= jmin && hkey == pf->hkey &&
          (pf->sideToMove == posFilter_Any || pf->sideToMove == game->player))
      {
         BOOL match = true;
         for (SQUARE sq = a1; sq <= h8 && match; sq++)
            if (onBoard(sq) && pf->Pos[sq] != game->Board[sq])
               match = false;
         if (match) return true;
      }
   }

   return false;
} /* Filter_PosExact */


static BOOL Filter_PosPartial (POS_FILTER *pf, CGame *game)
{
   INT   jmin   = (pf->checkMoveRange ? 2*pf->minMove - 2 : 0);
   INT   jmax   = (pf->checkMoveRange ? Min(game->lastMove, 2*pf->maxMove) : game->lastMove);
   INT   wCountTotal, bCountTotal;

   //--- Compute the piece counters ---

   if (! game->Init.wasSetup)
   {
      wCountTotal = 16;
      bCountTotal = 16;
   }
   else
   {
      wCountTotal = bCountTotal = 0;

      PIECE p;
      for (SQUARE sq = a1; sq <= h8; sq++)
         if (onBoard(sq) && (p = game->Board[sq]))
            if (pieceColour(p) == white) wCountTotal++; else bCountTotal++;

      if (wCountTotal < pf->wCountMin || bCountTotal < pf->bCountMin) return false;
   }

   //--- Replay the game and search for matching positions ---

   for (INT j = 0; j <= jmax; j++)
   {
      if (j > 0)
      {
         game->RedoMove(false);

         MOVE *m = &(game->Record[j]);
         if (m->cap || m->type == mtype_EP)
            if (pieceColour(m->piece) == white)
            {  if (--bCountTotal < pf->bCountMin) return false;
            }
            else
            {  if (--wCountTotal < pf->wCountMin) return false;
            }
      }

      if (j >= jmin && wCountTotal <= pf->wCountMax && bCountTotal <= pf->bCountMax && 
          (pf->sideToMove == posFilter_Any || pf->sideToMove == game->player))
      {
         BOOL match = true;
         for (SQUARE sq = a1; sq <= h8 && match; sq++)
            if (onBoard(sq) && pf->Pos[sq] && pf->Pos[sq] != game->Board[sq])
               match = false;
         if (match) return true;
      }
   }

   return false;
} /* Filter_PosPartial */


/**************************************************************************************************/
/*                                                                                                */
/*                                           RESET FILTER                                         */
/*                                                                                                */
/**************************************************************************************************/

void SigmaCollection::ResetFilter (void)
{
   ::ResetFilter(&filter);
} /* SigmaCollection::ResetFilter */


void ResetFilter (FILTER *filter)
{
   filter->count = 1;

   for (INT i = 0; i < maxFilterCond; i++)
   {  filter->Field[i] = filterField_WhiteOrBlack + i;
      filter->Cond[i]  = filterCond_Is;
      CopyStr("", filter->Value[i]);
   }

   filter->useLineFilter = false;
   filter->lineLength = 0;

   filter->usePosFilter  = false;
   ResetPosFilter(&(filter->posFilter));
} /* ResetFilter */


void ResetPosFilter (POS_FILTER *pf)
{
   pf->exactMatch = true;
   pf->sideToMove = posFilter_Any;
   ::NewBoard(pf->Pos);

   pf->checkMoveRange = false;
   pf->minMove    = 1;
   pf->maxMove    = posFilter_AllMoves;

   pf->wCountMin  = 1;
   pf->wCountMax  = 16;
   pf->bCountMin  = 1;
   pf->bCountMax  = 16;

   for (INT i = 0; i < 128; i++)
      pf->Unused[i] = 0;

   PreparePosFilter(pf);
} /* ResetPosFilter */


void PreparePosFilter (POS_FILTER *pf)  // Should be called by Position Filter Dialog
{
   // The stuff below is really only necessary if exact match:

   pf->hkey = ::CalcHashKey(&Global, pf->Pos);
   pf->wCountTotal = pf->wCountPawns = 0;
   pf->bCountTotal = pf->bCountPawns = 0;

   PIECE p;
   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq) && (p = pf->Pos[sq]))
         if (pieceColour(p) == white)
         {  pf->wCountTotal++;
            if (p == wPawn) pf->wCountPawns++;
         }
         else
         {  pf->bCountTotal++;
            if (p == bPawn) pf->bCountPawns++;
         }
} /* PreparePosFilter */
