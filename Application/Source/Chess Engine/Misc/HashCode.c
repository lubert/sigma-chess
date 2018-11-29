/**************************************************************************************************/
/*                                                                                                */
/* Module  : HASHCODE.C                                                                           */
/* Purpose : This module implements the hash code routines.                                       */
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

#include "Engine.h"

#include "HashCode.f"


/**************************************************************************************************/
/*                                                                                                */
/*                                   COMPUTING/UPDATING HASH KEY                                  */
/*                                                                                                */
/**************************************************************************************************/

HKEY CalcHashKey (GLOBAL *Global, PIECE Board[])     // Computes "hashKey" from scratch for the
{                                                    // specified board position.
   register HKEY hashKey = 0;

   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq) && Board[sq])
         hashKey ^= Global->H.HashCode[Board[sq]][sq];

   return hashKey;
}   /* CalcHashKey */


HKEY HashKeyChange (GLOBAL *Global, MOVE *m)         // Computes and returns hash key change caused by the move m.
{
   HASHCODE_COMMON *H = &(Global->H);
   HKEY   dkey;
   COLOUR player = pieceColour(m->piece);
   
   dkey = H->HashCode[m->piece][m->from] ^ H->HashCode[m->piece][m->to];
 
   if (m->cap) dkey ^= H->HashCode[m->cap][m->to];

   switch (m->type)
   {
      case mtype_Normal : break;
      case mtype_O_O   :
          dkey ^= H->HashCode[rook + player][m->from + 1] ^ H->HashCode[rook + player][m->from + 3];
          break;
      case mtype_O_O_O :
          dkey ^= H->HashCode[rook + player][m->from - 1] ^ H->HashCode[rook + player][m->from - 4];
          break;
      case mtype_EP :
          dkey ^= H->HashCode[pawn + (black - player)][m->to - (player == white ? 0x10 : -0x10)];
          break;
      default :
          dkey ^= H->HashCode[m->type][m->to];
   }

   return dkey;
} /* HashKeyChange */


/**************************************************************************************************/
/*                                                                                                */
/*                                      ACCESS POSITION LIBRARY                                   */
/*                                                                                                */
/**************************************************************************************************/

LIB_CLASS ProbePosLib (LIBRARY *lib, COLOUR player, HKEY pos)
{
   LIB_POS *Data = (LIB_POS*)(lib->Data);
   if (player == black) Data += lib->wPosCount;

   LONG count = (player == white ? lib->wPosCount : lib->bPosCount);
   LONG min   = 0;
   LONG max   = count - 1;

   while (min <= max)
   {
      LONG i = (min + max)/2;
      if (pos < Data[i].pos) max = i - 1;
      else if (pos > Data[i].pos) min = i + 1;
      else return (LIB_CLASS)(Data[i].flags);
   }

   return libClass_Unclassified;
} /* ProbePosLib */


/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION                                   */
/*                                                                                                */
/**************************************************************************************************/

static HKEY RandKey (GLOBAL *Global);

void InitHashCodeModule (GLOBAL *Global)                 // Initializes transposition hash codes.
{
   HASHCODE_COMMON *H = &(Global->H);

   H->randKey = 310660507;

   for (PIECE p = pawn; p <= king; p++)
      for (SQUARE sq = a1; sq <= h8; sq++)
         if (onBoard(sq))
         {  H->HashCode[white + p][sq] = RandKey(Global);
            H->HashCode[black + p][sq] = RandKey(Global);
         }

   // Clear hashcodes for pawns on first an eighth rank:
   for (INT f = 0; f <= 7; f++)
   {  H->HashCode[wPawn][square(f,0)] = 0;
      H->HashCode[wPawn][square(f,7)] = 0;
      H->HashCode[bPawn][square(f,0)] = 0;
      H->HashCode[bPawn][square(f,7)] = 0;
   }

   H->o_oHashCodeW   = H->HashCode[wRook][h1] ^ H->HashCode[wRook][f1];
   H->o_oHashCodeB   = H->HashCode[bRook][h8] ^ H->HashCode[bRook][f8];
   H->o_o_oHashCodeW = H->HashCode[wRook][a1] ^ H->HashCode[wRook][d1];
   H->o_o_oHashCodeB = H->HashCode[bRook][a8] ^ H->HashCode[bRook][d8];
} /* InitHashCodeModule */


static HKEY RandKey (GLOBAL *Global)
{
   Global->H.randKey *= 1103515245;
   Global->H.randKey += 12345;
   return Global->H.randKey;
} /* RandKey */
