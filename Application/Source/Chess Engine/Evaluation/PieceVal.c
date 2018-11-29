/**************************************************************************************************/
/*                                                                                                */
/* Module  : PIECEVAL.C                                                                           */
/* Purpose : This is the main evaluation module which computes all the piece value tables at the  */
/*           root node.                                                                           */
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

#include "PieceVal.f"


/**************************************************************************************************/
/*                                                                                                */
/*                                      INITIALIZE PIECE VALUES                                   */
/*                                                                                                */
/**************************************************************************************************/

static void ComputeGamePhase      (ENGINE *E);
static void ComputeBaseVal        (ENGINE *E);
static void CalcPawnData          (ENGINE *E);
static void CalcPawnCenter        (ENGINE *E);

static void ResetPieceVal         (ENGINE *E);
static void ComputeCastlingPV     (ENGINE *E);

static void ComputePawnPV         (ENGINE *E);
static void ComputeKnightPV       (ENGINE *E);
static void ComputeBishopPV       (ENGINE *E);
static void ComputeRookPV         (ENGINE *E);
static void ComputeQueenPV        (ENGINE *E);
static void ComputeKingPV         (ENGINE *E);

static void OffiPawnCoordPV       (ENGINE *E);
static void ComputePlayingStylePV (ENGINE *E);

static void ComputeMatePV         (ENGINE *E);
static void ComputeSumPV          (ENGINE *E);

/*------------------------------------------ Main Routine ----------------------------------------*/
// This is the main routine which is called once at the root of each search. This computes all the
// piece value tables, which form the main evaluation during the search.
// May only be called AFTER a call the Board and Attack state structures have been initialized.

void CalcPieceValState (ENGINE *E)
{
   ComputeGamePhase(E);
   ComputeBaseVal(E);
   CalcPawnData(E);
   CalcPawnCenter(E);
   E->V.styleNormPV = 0;

   if (E->V.phase <= 9)
   {
      ResetPieceVal(E);
      ComputeCastlingPV(E);

      ComputePawnPV(E);
      ComputeKnightPV(E);
      ComputeBishopPV(E);
      ComputeRookPV(E);
      ComputeQueenPV(E);
      ComputeKingPV(E);

      OffiPawnCoordPV(E);
      ComputePlayingStylePV(E);
   }
   else
   {
      ComputeMatePV(E);
   }

   ComputeSumPV(E);
} /* CalcPieceValState */


/**************************************************************************************************/
/*                                                                                                */
/*                                   GENERAL PIECE VALUE COMPUTATION                              */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Determining Game Phase ----------------------------------*/
// Determines the game phase from the current board position: 0 = opening, ... , 9 = endgame with
// pawns only.

static void ComputeGamePhase (ENGINE *E)
{
   GLOBAL         *G = E->Global;
   PIECEVAL_STATE *V = &E->V;
   BOARD_STATE    *B = &E->B;

   for (COLOUR c = white; c <= black; c += black)                 // Count officer material
   {                                                              // for each side.
      V->OffiMtrl[c] = 0;
      for (INDEX i = 1; i <= B->LastOffi[c]; i++)
         V->OffiMtrl[c] += G->B.Mtrl[B->Board[B->PieceLoc[c + i]]];
      V->PawnMtrl[c]  = B->LastPiece[c] - B->LastOffi[c];
      V->TotalMtrl[c] = V->OffiMtrl[c] + V->PawnMtrl[c];          // Count total material for
   }                                                              // each side.

   if (V->TotalMtrl[white] == 0 && V->PawnMtrl[black] == 0)
   {
      V->phase = mateWhite;
      V->kbnk  = (B->pieceCount == 0x02100000);
   }
   else if (V->TotalMtrl[black] == 0 && V->PawnMtrl[white] == 0)
   {
      V->phase = mateBlack;
      V->kbnk  = (B->pieceCount == 0x00000210);
   }
   else
   {
      V->phase = (63 - V->OffiMtrl[white] - V->OffiMtrl[black])/7;
      V->kbnk  = false;
   }
} /* ComputeGamePhase */

/*------------------------------------ Basis Value Computation -----------------------------------*/
// Computes basis (square independent) value for each piece i.e. BaseVal[p] Å 100*Mtrl[p]. If one
// side is materially ahead, exchanges are encouraged during the search by lowering the values of
// the officers of the winning side (10-30). Pawn values are increased in this case, if the winning
// side has less than 4 pawns left.

static void ComputeBaseVal (ENGINE *E)
{
   PIECEVAL_STATE *V = &E->V;

   V->BaseVal[wPawn]   =  90;
   V->BaseVal[wKnight] = 300;
   V->BaseVal[wBishop] = 300;
   V->BaseVal[wRook]   = 480;
   V->BaseVal[wQueen]  = 950;
   V->BaseVal[wKing]   =   0;

   for (PIECE p = pawn; p <= king; p++)
      V->BaseVal[black + p] = -V->BaseVal[p];

   V->winColour = V->loseColour = -1;                              // Reset win/lose colour

   // If the last move was NOT a capture, we start computing exchange values, i.e. encourage
   // the winning side to exchange material.

   INT mtrlDiff = V->TotalMtrl[white] - V->TotalMtrl[black];        // Calc material diff
   INT exgVal   = (Abs(mtrlDiff) > 1 ? 30 : 20);

   switch (Sign(mtrlDiff))                                          // Determine exhange factors:
   {
      case 0 : break;

      case 1 : V->winColour  = white;
               V->loseColour = black;
               if (V->PawnMtrl[white] <= 3)                         // Increase pawn values.
                  V->BaseVal[wPawn] += 10*(4 - V->PawnMtrl[white]);
               V->BaseVal[wKnight] -= exgVal;                       // Decrease officer values.
               V->BaseVal[wBishop] -= exgVal;
               V->BaseVal[wRook]   -= exgVal + 5;
               V->BaseVal[wQueen]  -= exgVal + 10;
               break;

      case -1: V->winColour  = black;
               V->loseColour = white;
               if (V->PawnMtrl[black] <= 3)                         // Increase pawn values.
                  V->BaseVal[bPawn] -= 10*(4 - V->PawnMtrl[black]);
               V->BaseVal[bKnight] += exgVal;                       // Decrease officer values.
               V->BaseVal[bBishop] += exgVal;
               V->BaseVal[bRook]   += exgVal + 5;
               V->BaseVal[bQueen]  += exgVal + 10;
   }
} /* ComputeBaseVal */

/*------------------------------------ Resetting Piece Values ------------------------------------*/
// Initializes the piece value tables through "BaseVal" and the basic piece value tables.

static void ResetPieceVal (ENGINE *E)
{
   GLOBAL          *G = E->Global;
   PIECEVAL_STATE  *V  = &(E->V);
   PIECEVAL_COMMON *GV = &(G->V);

   INT    kcf  = G->V.Kcf[V->phase];
   SQUARE wkSq = kingLocW(E);
   SQUARE bkSq = kingLocB(E);
   INT    i    = 0;

   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq))
      {
         INT cl = closeness(sq, bkSq);
         V->PieceVal[wPawn][sq]   = GV->PawnPV[63 - i] + (rank(sq) == 7 ? 0 : V->BaseVal[wPawn]);
         V->PieceVal[wKnight][sq] = GV->KnightPV[63 - i] + V->BaseVal[wKnight] + 2*cl;
         V->PieceVal[wBishop][sq] = GV->BishopPV[63 - i] + V->BaseVal[wBishop] + 2*cl;
         V->PieceVal[wRook][sq]   = GV->RookPV[63 - i]   + V->BaseVal[wRook] + cl;
         V->PieceVal[wQueen][sq]  = GV->QueenPV[63 - i]  + V->BaseVal[wQueen] + 4*cl; //*(1 + V->phase/3);
         V->PieceVal[wKing][sq]   = kcf*V->PawnCentrePV[i];

         cl = closeness(sq, wkSq);
         V->PieceVal[bPawn][sq]   = -GV->PawnPV[i] + (rank(sq) == 0 ? 0 : V->BaseVal[bPawn]);
         V->PieceVal[bKnight][sq] = -GV->KnightPV[i] + V->BaseVal[bKnight] - 2*cl;
         V->PieceVal[bBishop][sq] = -GV->BishopPV[i] + V->BaseVal[bBishop] - 2*cl;
         V->PieceVal[bRook][sq]   = -GV->RookPV[i]   + V->BaseVal[bRook] - cl;
         V->PieceVal[bQueen][sq]  = -GV->QueenPV[i]  + V->BaseVal[bQueen] - 4*cl; //(1 + V->phase/3);
         V->PieceVal[bKing][sq]   = -kcf*V->PawnCentrePV[i];
         i++;
      }
} /* ResetPieceVal */

/*------------------------------ Compute Pawn Structure Information ------------------------------*/
// Computes the "PawnDataW[]" and "PawnDataB[]" tables from scratch.

#define pdW_L      pdW[f > 0 ? -1 : 1]
#define pdW_R      pdW[f < 7 ? 1 : -1]
#define pdB_L      pdB[f > 0 ? -1 : 1]
#define pdB_R      pdB[f < 7 ? 1 : -1]

static void CalcPawnData (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   PAWNDATA        *pdW, *pdB;

   // Reset file data:

   pdW = V->PawnDataW;
   pdB = V->PawnDataB;
   for (INT f = 0; f <= 7; f++, pdW++, pdB++)
   {
      pdW->occupied = pdB->occupied = false;
      pdW->sq       = pdB->sq       = nullSq;
      pdW->rFront   = pdB->rRear    = 0;
      pdW->rRear    = pdB->rFront   = 7;
      pdW->passed   = pdB->passed   = false;
      pdW->backward = pdB->backward = false;
      pdW->isolated = pdB->isolated = false;
   }

   // Find files occupied by white pawns (front/rear):

   for (INDEX i = B->LastOffi[white] + 1; i <= B->LastPiece[white]; i++)
   {
      SQUARE sq = B->PieceLocW[i];
      INT r = rank(sq);
      pdW = &V->PawnDataW[file(sq)];
      pdW->occupied = true;
      if (r > pdW->rFront) pdW->rFront = r, pdW->sq = sq;
      if (r < pdW->rRear)  pdW->rRear  = r;
   }

   // Find files occupied by black pawns (front/rear):

   for (INDEX i = B->LastOffi[black] + 1; i <= B->LastPiece[black]; i++)
   {
      SQUARE sq = B->PieceLocB[i];
      INT r = rank(sq);
      pdB = &V->PawnDataB[file(sq)];
      pdB->occupied = true;
      if (r < pdB->rFront) pdB->rFront = r, pdB->sq = sq;
      if (r > pdB->rRear)  pdB->rRear  = r;
   }

   // Find passed, isolated and backward pawns.

   pdW = V->PawnDataW;
   pdB = V->PawnDataB;                           
   for (INT f = 0; f <= 7; f++, pdW++, pdB++)
   {
      if (pdW->rFront > pdB->rRear)                              // If front pawn is unopposed:
      { 
         if (pdW->rFront >= Max(pdB_L.rRear, pdB_R.rRear))
            pdW->passed = true;
         if (! pdW_L.occupied && ! pdW_R.occupied)
             pdW->isolated = true;
         else if (pdW->rFront < Min(pdW_L.rRear, pdW_R.rRear))
            pdW->backward = true;
      }

      if (pdB->rFront < pdW->rRear)                              // If front pawn is unopposed:
      { 
         if (pdB->rFront <= Min(pdW_L.rRear, pdW_R.rRear))
            pdB->passed = true;
         if (! pdB_L.occupied && ! pdB_R.occupied)
             pdB->isolated = true;
         else if (pdB->rFront > Max(pdB_L.rRear, pdB_R.rRear))
            pdB->backward = true;
      }
   }
} /* CalcPawnData */

// "CalcPawnCenter()" Computes the geometrical centre of all pawns (passed pawns are weighed higher
// than other pawns) and constructs a centrality table based on this centre. Is used in the end
// game to keep the kings and minor officers close to the action, which is not necessarily the
// centre of the board.

static void CalcPawnCenter (ENGINE *E)                        
{
   GLOBAL          *G = E->Global;
   PIECEVAL_STATE  *V = &E->V;
   INT count, fSum, rSum;

   count = fSum = rSum = 0;

   if (V->phase >= 6)
   {
      for (INT f = 0; f <= 7; f++)
      {
         INT c = 0;

         if (V->PawnDataW[f].occupied)
         {
            INT r = V->PawnDataW[f].rFront + 1;
            if (V->PawnDataW[f].passed) c = 1 + r/2;
            else if (V->PawnDataW[f].isolated || V->PawnDataW[f].backward) c = 2;
            else c = 1;
            rSum += c*r;
            fSum += c*f;
            count += c;
         }

         if (V->PawnDataB[f].occupied)
         {
            INT r = V->PawnDataB[f].rFront - 1;
            if (V->PawnDataB[f].passed) c = 1 + (7 - r)/2;
            else if (V->PawnDataB[f].isolated || V->PawnDataB[f].backward) c = 2;
            else c = 1;
            rSum += c*r;
            fSum += c*f;
            count += c;
         }
      }
   }

   if (count > 0)
   {
      SQUARE csq = (rSum/count << 4) + fSum/count;    // Compute geometrical centre.
      if (file(csq) <= 1) csq++;                      // Add small board center bias.
      else if (file(csq) >= 6) csq--;

      INT i = 0;
      for (SQUARE sq = a1; sq <= h8; sq++)
         if (onBoard(sq))
            V->PawnCentrePV[i++] = closeness(csq,sq);
   }
   else
   {
      for (INT i = 0; i <= 63; i++)
         V->PawnCentrePV[i] = G->V.KingPV[i];
   }
} /* CalcPawnCenter */

/*----------------------------------- Compute Piece Value Sum ------------------------------------*/
// Computes "sumPV" from the "PieceVal"-table based on the piece locations in "PieceLoc".

static void NormalizeSumPV (ENGINE *E);

static void ComputeSumPV (ENGINE *E)                              
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;

   V->sumPV = 0;
   for (INDEX i = 0; i <= 31; i++)
   {  SQUARE sq = B->PieceLoc[i];
      if (sq >= a1)
         V->sumPV += V->PieceVal[B->Board[sq]][sq];
   }

   if (B->Board[e1] == wKing && ! B->HasMovedTo[e1] && ! B->HasMovedTo[h1])
      V->sumPV += V->PieceVal[wKing][d1] - V->PieceVal[wKing][e1];
   if (B->Board[e8] == bKing && ! B->HasMovedTo[e8] && ! B->HasMovedTo[h8])
      V->sumPV += V->PieceVal[bKing][d8] - V->PieceVal[bKing][e8];

   V->sumPV -= V->styleNormPV;

   if (V->winColour != -1)
      NormalizeSumPV(E);
} /* ComputeSumPV */


static void NormalizeSumPV (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;

   for (INDEX i = 1; i < B->LastPiece[V->winColour]; i++)
   {
      PIECE p = pieceType(B->Board[B->PieceLoc[V->winColour + i]]);
      V->sumPV -= V->BaseVal[white + p] + V->BaseVal[black + p];
   }
} /* NormalizeSumPV */


/**************************************************************************************************/
/*                                                                                                */
/*                                   INDIVIDUAL PIECE VALUE COMPUTATION                           */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------------- King Safety/Castling ----------------------------------*/
// Analyzes castling posibilities for each side (KingRight[]) and computes castling bonuses
// (o_oPV[]/o_o_oPV[]) for use during search. Also loss of castling rights is punished.

static INT KingSafety (PIECEVAL_STATE *V, COLOUR c, SQUARE dir);

static void ComputeCastlingPV (ENGINE *E)     
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   INT o_oVal, o_o_oVal;

   if (kingLocW(E) == e1 && ! B->HasMovedTo[e1])              // If WHITE king has NOT moved...
   {
      o_oVal = o_o_oVal = -maxVal;

      if (B->Board[h1] == wRook && ! B->HasMovedTo[h1])       // King side castling (o-o):
      {  o_oVal = 6*KingSafety(V, white, +1);                 // Compute o-o bonus.
         V->PieceVal[wRook][h1] += o_oVal;                    // Don't lose o-o rights.
         V->PieceVal[wBishop][f1] -= 10;                      // Force Bf1 out!
         V->PieceVal[wKnight][g1] -= 10;                      // Force Ng1 out!
      }
      if (B->Board[a1] == wRook && ! B->HasMovedTo[a1])       // Queen side castling (o-o-o):
      {   o_o_oVal = 3*KingSafety(V, white, -1);              // Compute o-o-o bonus.
         if (o_o_oVal > o_oVal)                               // Don't lose o-o-o rights if o-o
            V->PieceVal[wRook][a1] += o_o_oVal;               // is bad or impossible.
      }
      if (B->opponent == white)                               // Reduce castling bonuses for
         o_oVal /= 2, o_o_oVal /= 2;                          // opponent.

      V->PieceVal[wKing][e1] += Max(0,Max(o_oVal,o_o_oVal));  // Punish king for moving if
                                                              // castling is possible.
      V->KingRight[white] = (o_oVal >= o_o_oVal);             // Should king move right (to g1)?
      V->o_oPV[white]   = o_oVal;
      V->o_o_oPV[white] = o_o_oVal;
   }
   else                                                       // If WHITE king HAS moved, then
      V->KingRight[white] = (file(kingLocW(E)) >= 4);         // check if located right (at g1)?

   if (kingLocB(E) == e8 && ! B->HasMovedTo[e8])              // If BLACK king has NOT moved...
   {
      o_oVal = o_o_oVal = -maxVal;

      if (B->Board[h8] == bRook && ! B->HasMovedTo[h8])       // King side castling (o-o):
      {  o_oVal = 6*KingSafety(V, black, +1);                 // Compute o-o bonus.
         V->PieceVal[bRook][h8] -= o_oVal;                    // Don't lose o-o rights.
         V->PieceVal[bBishop][f8] += 10;                      // Force Bf8 out!
         V->PieceVal[bKnight][g8] += 10;                      // Force Ng8 out!
      }
      if (B->Board[a8] == bRook && ! B->HasMovedTo[a8])       // Queen side castling (o-o-o):
      {  o_o_oVal = 3*KingSafety(V, black, -1);               // Compute o-o-o bonus.
         if (o_o_oVal > o_oVal)                               // Don't lose o-o-o rights if o-o
            V->PieceVal[bRook][a8] -= o_o_oVal;               // is bad or impossible.
      }
      if (B->opponent == black)                               // Reduce castling bonuses for
         o_oVal /= 2, o_o_oVal /= 2;                          // opponent.

      V->PieceVal[bKing][e8] -= Max(0,Max(o_oVal,o_o_oVal));  // Punish king for moving if
                                                              // castling is possible.
      V->KingRight[black] = (o_oVal >= o_o_oVal);             // Should king move right (to g8)?
      V->o_oPV[black]   = -o_oVal;
      V->o_o_oPV[black] = -o_o_oVal;
   }
   else                                                       // If BLACK king HAS moved, then
      V->KingRight[black] = (file(kingLocB(E)) >= 4);         // check if located right (at g8)?
} /* ComputeCastlingPV */

// Computes king safety [0..10]. Defined in terms of the pawns at the king side and the game phase.

static INT KingSafety (PIECEVAL_STATE *V, COLOUR c, SQUARE dir)
{
   INT    FileSafety[8] = { 0, 5, 4, 2, 1, 0, 0, 0 };
   INT    wSafe, bSafe;
   SQUARE f = (dir > 0 ? 6 : 1);

   wSafe = FileSafety[V->PawnDataW[f].rRear] +
           FileSafety[V->PawnDataW[f + dir].rRear];
   bSafe = FileSafety[7 - V->PawnDataB[f].rRear] +
           FileSafety[7 - V->PawnDataB[f + dir].rRear];

   if (c == black) Swap(&wSafe, &bSafe);
   return Max(0, wSafe - (5 - bSafe/2) - V->phase/2) - 3;
} /* KingSafety */

/*---------------------------------------------- Pawns -------------------------------------------*/

static void KingSidePawnsPV (ENGINE *E, COLOUR c);
static void PawnStormPV (ENGINE *E, COLOUR c);
static void PassedPawnPV (ENGINE *E);

static void ComputePawnPV (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   ATTACK_STATE    *A = &E->A;

   if (V->phase <= 6)                                     // Don't weaken king side pawn structure.
   { 
      KingSidePawnsPV(E, white),
      KingSidePawnsPV(E, black);

      if (V->KingRight[white] != V->KingRight[black] &&   // Pawn storm if kings on opposite sides.
          B->Board[e1] != wKing && B->Board[e8] != bKing)
      {
         PawnStormPV(E, white);
         PawnStormPV(E, black);
      }
/*
      else                                                // Otherwise don't move those edge pawns
      {  
         V->PieceVal[wPawn][a4] -= 5;                     // too early.
         V->PieceVal[wPawn][a5] -= 5;
         V->PieceVal[bPawn][a5] += 5;
         V->PieceVal[bPawn][a4] += 5;
      }
*/
   }

   if (B->Board[b2] == wBishop || (A->AttackW[b2] & bMask))               // Fianchetto.
      V->PieceVal[wPawn][b3] += 10;
   if (B->Board[g2] == wBishop || (A->AttackW[g2] & bMask))
      V->PieceVal[wPawn][g3] += 10;
   if (B->Board[b7] == bBishop || (A->AttackB[b7] & bMask))
      V->PieceVal[bPawn][b6] -= 10;
   if (B->Board[g7] == bBishop || (A->AttackB[g7] & bMask))
      V->PieceVal[bPawn][g6] -= 10;

   PassedPawnPV(E);                                      // Passed pawn evaluation.
} /* ComputepawnPV */


static void KingSidePawnsPV (ENGINE *E, COLOUR c)    // Punish weakening of the king side pawn
{                                                    // structure.
   GLOBAL          *G = E->Global;
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   SQUARE          sq0, sq1, sq2, dir;
   INT             *PV, f;

   sq1 = G->B.Rank2[c] << 4;
   if (V->KingRight[c])
      sq1 += 6, sq0 = left(sq1), sq2 = right(sq1);
   else
      sq1++, sq2 = left(sq1), sq0 = right(sq1);
   PV = V->PieceVal[c + pawn];
   dir = (c == white ? 0x10 : -0x10);
   f = Sign(dir)*(6 - V->phase);

   PV[sq0] += 3*f; PV[sq0 + dir] += f;
   PV[sq1] += 7*f; PV[sq1 + dir] += 4*f; PV[sq1 + 2*dir] -= f; PV[sq1 + 3*dir] -= 2*f;
   PV[sq2] += 6*f; PV[sq2 + dir] += 5*f;
   if (B->Board[sq1] != pawn + c)                  // Don't move f2 or h2 if no pawn at g2.
      PV[sq0] += 2*f,
      PV[sq2] += 2*f;

   // If castling is lost, DON'T trap rook in corner (h1) by playing Kg1?? Rather play Pg3
   // followed by Kg2.

   if ((B->Board[c == white ? e1 : e8] != c + king ||
        B->HasMovedTo[c == white ? e1 : e8]) &&
       (B->Board[sq2] == c + pawn ||
        B->Board[sq2 + dir] == c + pawn) &&
       (B->Board[sq1] == c + pawn ||
        B->Board[sq1 + dir] == c + pawn) &&
       (B->Board[sq2 - dir] == c + rook ||
        B->Board[sq1 - dir] == c + rook))
   {   
      if (B->Board[sq2] == c + pawn && B->Board[sq2 - dir] == c + rook)
         V->PieceVal[c + king][sq1 - dir] -= dir,
         V->PieceVal[c + rook][sq2 - dir] -= dir;

      if (B->Board[sq0 - dir] == c + king ||
          B->Board[sq1 - dir] == c + king ||
          B->Board[sq2 - dir] == c + king)
      {
         V->PieceVal[c + rook][sq2 - dir] -= dir;
         V->PieceVal[c + rook][sq1 - dir] -= dir;
         V->PieceVal[c + pawn][sq1]       -= (3*dir)/2;
         V->PieceVal[c + king][sq0 - dir] -= dir;
         V->PieceVal[c + king][sq1 - dir] -= dir;
         V->PieceVal[c + king][sq1]       += dir;
      }
   }
} /* KingSidePawnsPV */


static void PawnStormPV (ENGINE *E, COLOUR c)     // Give bonus for pawn storm.
{
   PIECEVAL_STATE  *V = &E->V;
   INT             fmax = (V->KingRight[c] ? 2 : 7);
   INT             val = 13 - V->phase;

   if (c == white)
   { 
      for (INT f = fmax - 2; f <= fmax; f++)
         if (onBoard(V->PawnDataW[f].sq))
            V->PieceVal[wPawn][V->PawnDataW[f].sq] -= val;
   }
   else
   {  
      for (INT f = fmax - 2; f <= fmax; f++)
         if (onBoard(V->PawnDataB[f].sq))
            V->PieceVal[bPawn][V->PawnDataB[f].sq] += val;
   }
} /* PawnStormPV */


static void PassedPawnPV (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   INT             PassBonus[8] = { 0, 0, 0, 2, 4, 8, 14, 0 };  // Rank dependant passed pawn bonus.

   for (INT f = 0; f <= 7; f++)
   {
      if (V->PawnDataW[f].passed)
      {  INT c = V->phase + (B->player == white ? -2 : 0);
         for (INT r = V->PawnDataW[f].rFront; r < 7; r++)
            V->PieceVal[wPawn][(r << 4) + f] += c*PassBonus[r];
      }

      if (V->PawnDataB[f].passed)
      {  INT c = V->phase + (B->player == black ? -2 : 0);
         for (INT r = V->PawnDataB[f].rFront; r > 0; r--)
            V->PieceVal[bPawn][(r << 4) + f] -= c*PassBonus[7 - r];
      }
   }
} /* PassedPawnPV */

/*--------------------------------------------- Knights ------------------------------------------*/

static void ComputeKnightPV (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;

   // Avoid blockade of c-pawn:

   if (B->Board[b1] == wKnight && B->Board[c2] == wPawn && B->Board[c4] == empty)
   {  V->PieceVal[wKnight][c3] -= 7;
      V->PieceVal[wKnight][d2] += 7;
      V->PieceVal[wPawn][d2]   -= 5;
      V->PieceVal[wBishop][d2] -= 5;
   }

   if (B->Board[b8] == bKnight && B->Board[c7] == bPawn && B->Board[c5] == empty)
   {  V->PieceVal[bKnight][c6] += 7;
      V->PieceVal[bKnight][d7] -= 7;
      V->PieceVal[bPawn][d7]   += 5;
      V->PieceVal[bBishop][d7] += 5;
   }
} /* ComputeKnightPV */

/*--------------------------------------------- Bishops ------------------------------------------*/

static void ComputeBishopPV (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;

   if (V->PawnDataW[file(g2)].occupied)                          // Encourage fianchetto.
      V->PieceVal[wBishop][g2] += 10;
   if (V->PawnDataW[file(b2)].occupied)
      V->PieceVal[wBishop][b2] += 10;
   if (V->PawnDataB[file(g7)].occupied)
      V->PieceVal[bBishop][g7] -= 10;
   if (V->PawnDataB[file(b7)].occupied)
      V->PieceVal[bBishop][b7] -= 10;

   if (B->Board[d2] == wPawn) V->PieceVal[wBishop][d3] -= 25;    // Punish central pawn block.
   if (B->Board[e2] == wPawn) V->PieceVal[wBishop][e3] -= 25;
   if (B->Board[c2] == wPawn) V->PieceVal[wBishop][c3] -=  5;
   if (B->Board[d7] == bPawn) V->PieceVal[bBishop][d6] += 25;
   if (B->Board[e7] == bPawn) V->PieceVal[bBishop][e6] += 25;
   if (B->Board[c7] == bPawn) V->PieceVal[bBishop][c6] +=  5;

   if (B->Board[d8] == bQueen && B->Board[f6] == bKnight)        // Encourage pin of Nf6 + Qd8.
      V->PieceVal[wBishop][g5] += 5,
      V->PieceVal[wBishop][h4] = V->PieceVal[wBishop][g5];
   if (B->Board[d1] == wQueen && B->Board[f3] == wKnight)        // Encourage pin of Nf3 + Qd1.
      V->PieceVal[bBishop][g4] -= 5,
      V->PieceVal[bBishop][h5] = V->PieceVal[bBishop][g4];

   if (B->player == white)
   {  if (B->Board[f7] == bPawn && B->Board[g7] == bPawn)        // Don't get stuck at Òh2Ó like
         V->PieceVal[wBishop][h7] -= 150;                        // Fischer did versus Spasskij!!
      if (B->Board[b7] == bPawn && B->Board[c7] == bPawn)        // (program only).
         V->PieceVal[wBishop][a7] -= 150;
   }
   else
   {  if (B->Board[f2] == wPawn && B->Board[g2] == wPawn)        // Don't get stuck at Òh2Ó like
         V->PieceVal[bBishop][h2] += 150;                        // Fischer did versus Spasskij!!
      if (B->Board[b2] == wPawn && B->Board[c2] == wPawn)        // (program only).
         V->PieceVal[bBishop][a2] += 150;
   }
} /* ComputeBishopPV */

/*---------------------------------------------- Rooks -------------------------------------------*/

#define rookOnOpen          12                        // Bonus for rooks on open files.
#define rookOnSemiOpen       7                        // Bonus for rooks on semi open files.

static void ComputeRookPV (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   INT             bonus  = (10 - V->phase),              // Bonus for 1st rank occupation.
                   punish = 2*Min(0, V->phase - 5),       // Punishment for 2-4 rank occupation.
//                 punish = Max(V->phase - 2, 0),         // Punishment for 2-4 rank occupation.
                   *PVW = V->PieceVal[wRook],
                   *PVB = V->PieceVal[bRook];
   PAWNDATA        *pdW = V->PawnDataW,
                   *pdB = V->PawnDataB;

   for (INT f = 0; f <= 7; f++, pdW++, pdB++)
   {
      //--- WHITE rooks---

      if (pdW->occupied)
      {  
         PVW[0x10 + f] += punish;                                  // Punish on closed 2nd rank.
         if (pdW->passed)
         {  INT val = V->phase + 3*pdW->rFront;                    // Rooks behind passed pawns.
            for (SQUARE sq = f; sq <= pdW->sq; sq += 0x10)
               PVW[sq] += val + 5, PVB[sq] -= val;
         }
      }
      else
      { 
         INT val = (pdB->occupied ? rookOnSemiOpen : rookOnOpen);  // Open/semi open files.
         if (Abs(f - file(kingLocB(E))) <= 1)                      // Bonues for attacking files
            val += 7 - V->phase;                                   // adjacent to enemy king.
         for (SQUARE sq = f; sq <= h8; sq += 0x10)
            PVW[sq] += val;
      }
      PVW[f] += bonus;                                             // Bonus on 1st rank.

      //--- BLACK rooks---

      if (pdB->occupied)
      {
         PVB[0x60 + f] -= punish;                                  // Punish on closed 2nd rank.
         if (pdB->passed)
         {  INT val = V->phase + 3*(7 - pdB->rFront);              // Rooks behind passed pawns.
            for (SQUARE sq = f + 0x70; sq >= pdB->sq; sq -= 0x10)
               PVB[sq] -= val + 5, PVW[sq] += val;
         }
      }
      else
      {  
         INT val = (pdW->occupied ? rookOnSemiOpen : rookOnOpen);  // Open/semi open files.
         if (Abs(f - file(kingLocW(E))) <= 1)                      // Bonues for attacking files
            val += 7 - V->phase;                                   // adjacent to enemy king.
         for (SQUARE sq = f; sq <= h8; sq += 0x10)
            PVB[sq] -= val;
      }
      PVB[f + 0x70] -= bonus;                                      // Bonus on 1st rank.

      //--- Common ---

      if (V->phase < 5)                                            // Don't stand on 3rd or 4th
      {                                                            // rank in middle game.
         PVW[f + 0x20] += punish; PVW[f + 0x30] += punish;
         PVB[f + 0x50] -= punish; PVB[f + 0x40] -= punish;
      }
   }
} /* ComputeRookPV */

/*-------------------------------------------- Queens --------------------------------------------*/

#define queenOutEarly  -10                           // Penalty factor for early queen sortie.

static void ComputeQueenPV (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;

   if (V->phase < 3)                                 // Punish early queen sortie.
   {
      if (B->Board[d1] == wQueen)
      {
         INT n = -1;                                 // Number of undeveloped minor officers.
         if (B->Board[b1] == wKnight) n++;
         if (B->Board[g1] == wKnight) n++;
         if (B->Board[c1] == wBishop) n++;
         if (B->Board[f1] == wBishop) n++;
         if (n > 0) V->PieceVal[wQueen][d1] -= n*queenOutEarly;
      }

      if (B->Board[d8] == bQueen)
      {
         INT n = -1;                                 // Number of undeveloped minor officers.
         if (B->Board[b8] == bKnight) n++;
         if (B->Board[g8] == bKnight) n++;
         if (B->Board[c8] == bBishop) n++;
         if (B->Board[f8] == bBishop) n++;
         if (n > 0) V->PieceVal[bQueen][d8] += n*queenOutEarly;
      }
   }
} /* ComputeQueenPV */

/*---------------------------------------------- Kings -------------------------------------------*/

static void ComputeKingPV (ENGINE *E)
{
   GLOBAL          *G = E->Global;
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;

   if (G->V.Kcf[V->phase] < 0)
   {
      INT wval = V->PieceVal[wKing][g1] - (10 - V->phase)/2;
      INT bval = V->PieceVal[bKing][g8] + (10 - V->phase)/2;
      V->PieceVal[wKing][h1] = V->PieceVal[wKing][h2] = wval;      // Don't stay in a corner.
      V->PieceVal[wKing][a1] = V->PieceVal[wKing][a2] = wval;
      V->PieceVal[bKing][h8] = V->PieceVal[bKing][h7] = bval;
      V->PieceVal[bKing][a8] = V->PieceVal[bKing][a7] = bval;
      V->PieceVal[wKing][a3] = V->PieceVal[wKing][h3] = wval;      // Don't walk out of shelter.
      V->PieceVal[bKing][a6] = V->PieceVal[bKing][h6] = bval;
   }

   if (B->Board[g2] == wPawn && B->Board[h2] == wPawn)             // "Matten i bunden"
      V->PieceVal[wKing][h1] -= 7;
   if (B->Board[a2] == wPawn && B->Board[b2] == wPawn)
      V->PieceVal[wKing][a1] -= 7;
   if (B->Board[g7] == bPawn && B->Board[h7] == bPawn)
      V->PieceVal[bKing][h8] += 7;
   if (B->Board[a7] == bPawn && B->Board[b7] == bPawn)
      V->PieceVal[bKing][a8] += 7;
} /* ComputeKingPV */


/**************************************************************************************************/
/*                                                                                                */
/*                                   SPECIAL PIECE VALUE COMPUTATION                              */
/*                                                                                                */
/**************************************************************************************************/

static void OccupySqPV
(  ENGINE *E,
   SQUARE sq,                                           // Assign bonus/penalty
   INT wP, INT wN, INT wB, INT wR, INT wQ, INT wK,      // for occupying "sq".
   INT bP, INT bN, INT bB, INT bR, INT bQ, INT bK
);
static void AttackSqPV 
(  ENGINE *E,
   SQUARE sq,                                           // Assign bonus/penalty
   INT wP, INT wN, INT wB, INT wR, INT wQ, INT wK,      // for attacking "sq".
   INT bP, INT bN, INT bB, INT bR, INT bQ, INT bK
);

/*------------------------------------ Officer/Pawn Coordination ---------------------------------*/
// Compute piece values for strategic placement of officers relative to pawns (e.g. block opponent
// passed, isolated and backward pawns; knight ÒoutpostsÓ e.t.c.

static void OffiPawnCoordPV (ENGINE *E)
{
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   PAWNDATA        *pdW = V->PawnDataW;
   PAWNDATA        *pdB = V->PawnDataB;
   INT              b, c;                  // Block and Control bonus/penalty.


   for (INT f = 0; f <= 7; f++, pdW++, pdB++)
   {
      if (pdW->occupied)
      {
         if (pdW->passed)
            b = Sqr(pdW->rFront - (B->player == white ? 1 : 0)),
            c = b/3;
         else if (! pdB->occupied && (pdW->backward || pdW->isolated))
            b = 7,
            c = 5;
         else
            b = c = 0;

         INT b2 = b/2;
         if (b > 0) OccupySqPV(E, front(pdW->sq), 0,-b2,-b2,-b2,-b2,0, b,b,b,b,b,b);
         if (c > 0) AttackSqPV(E, front(pdW->sq), c,c,c,c,c,c, c,c,c,c,c,c);
      }

      if (pdB->occupied)
      {
         if (pdB->passed)
            b = Sqr((B->player == black ? 6 : 7) - pdB->rFront),
            c = b/3;
         else if (! pdW->occupied && (pdB->backward || pdB->isolated))
            b = 7,
            c = 5;
         else
            b = c = 0;

         INT b2 = b/2;
         if (b > 0) OccupySqPV(E, behind(pdB->sq), b,b,b,b,b,b, 0,-b2,-b2,-b2,-b2,0);
         if (c > 0) AttackSqPV(E, behind(pdB->sq), c,c,c,c,c,c, c,c,c,c,c,c);
      }
   }
}   /* OffiPawnCoordPV */

/*-------------------------------- Mating Piece Value Computation --------------------------------*/

static void ComputeMateWhitePV (ENGINE *E);
static void ComputeMateBlackPV (ENGINE *E);

static void ComputeMatePV (ENGINE *E)
{
   if (E->V.phase == mateWhite)
      ComputeMateWhitePV(E);
   else
      ComputeMateBlackPV(E);
} /* ComputeMatePV */


static void ComputeMateWhitePV (ENGINE *E)         // Computes piece values if white only has his
{                                                  // king left and black has mating material. 
   GLOBAL          *G = E->Global;
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   INT             c, kbnk[64], *wKingPV;

   if (V->kbnk)                                                          // If KBNK ending...
   {
      SQUARE sq = B->PieceLocB[B->Board[B->PieceLocB[1]] == bBishop ? 1 : 2];   // ¥ Get bishop colour
      INT mask = (even(file(sq) ^ rank(sq)) ? 0x00 : 0x07);
      for (INT i = 0; i < 64; i++)                                       // ¥ Compute lone king
         kbnk[i] = G->V.KBNK[i ^ mask];
      wKingPV = kbnk;
      c = -4;
   }
   else                                                          // Otherwise use normal
   {  wKingPV = G->V.LoneKingPV;                                 // "lone king" table.
      c = 20;
   }

   SQUARE kingSq = kingLocW(E);
   INT    i = 0;
   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq))
      {  
         V->PieceVal[bKnight][sq] = -3*closeness(sq, kingSq) - 3*G->V.KingPV[i] + V->BaseVal[bKnight];
         for (PIECE p = bBishop; p <= bQueen; p++)      
            V->PieceVal[p][sq] = V->BaseVal[p];
         V->PieceVal[bKing][sq] = -5*closeness(sq, kingSq) - 2*G->V.MKingPV[i];
         V->PieceVal[wKing][sq] = c*wKingPV[i];
         i++;
      }
}   /* ComputeMateWhitePV */


static void ComputeMateBlackPV (ENGINE *E)         // Computes piece values if black only has his
{                                                  // king left and white has mating material.
   GLOBAL          *G = E->Global;
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   INT             c, kbnk[64], *bKingPV;

   if (V->kbnk)                                                      // If KBNK ending...
   {
      SQUARE sq = B->PieceLocW[B->Board[B->PieceLocW[1]] == wBishop ? 1 : 2];   // ¥ Get bishop colour
      INT mask = (even(file(sq) ^ rank(sq)) ? 0x00 : 0x07);
      for (INT i = 0; i < 64; i++)                                   // ¥ Compute lone king
         kbnk[i] = G->V.KBNK[i ^ mask];
      bKingPV = kbnk; c = 4;
   }
   else                                                          // Otherwise use normal
   {  bKingPV = G->V.LoneKingPV;                                 // "lone king" table.
      c = -20;
   }

   SQUARE kingSq = kingLocB(E);
   INT    i = 0;
   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq))
      {  
         V->PieceVal[wKnight][sq] = 3*closeness(sq, kingSq) + 3*G->V.KingPV[i] + V->BaseVal[wKnight];
         for (PIECE p = wBishop; p <= wQueen; p++)      
            V->PieceVal[p][sq] = V->BaseVal[p];
         V->PieceVal[wKing][sq] = 5*closeness(sq, kingSq) + 2*G->V.MKingPV[i];
         V->PieceVal[bKing][sq] = c*bKingPV[i];
         i++;
      }
} /* ComputeMateBlackPV */

/*---------------------------------------- Playing Styles ------------------------------------*/
// · Chess supports 5 different playing styles (solid, defensive, normal, aggressive and
// desperado). These are implemented by adding/subtracting a king attack (closeness) value to
// the piece values tables as follows: 
//
//    solid/defensive      -> give opponent bonus for attacking players king
//    normal               -> don't change anything
//    aggressive/desperado -> give player bonus for attacking opponents king

static void ComputePlayingStylePV (ENGINE *E)
{
   GLOBAL          *G = E->Global;
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   INT             c, f;               // Colour selection and attack factor.

   V->styleNormPV = 0;

   switch (E->P.playingStyle)
   {
      case style_Chicken    : c = B->opponent; f = V->phase/2 - 6; break;
      case style_Defensive  : c = B->opponent; f = V->phase/3 - 3; break;
      case style_Normal     : return;
      case style_Aggressive : c = B->player;   f = 4 - V->phase/3; break;
      case style_Desperado  : c = B->player;   f = 8 - V->phase/2; break;
   }

   if (B->player == black) f = -f;
   SQUARE kingSq = kingLoc(E, black - c);

   for (PIECE p = pawn; p <= queen; p++)
   {
      INT *PV = V->PieceVal[p + c];
      for (SQUARE sq = a1; sq <= h8; sq++)
         if (onBoard(sq))
         {
            INT dv = f*G->V.Closeness[sq - kingSq];
            PV[sq] += dv;
            if (B->Board[sq] == p + c)
               V->styleNormPV += dv;
         }
   }
} /* ComputePlayingStylePV */

/*---------------------------------- Store KBNK Data in Hash Table -------------------------------*/

// "StoreKBNKpositions()" stores the "critical" KBNK positions from the KBNK data base
// ("KBNKdata[]") in the hash table. The hash table should be at least 640K to avoid too many
// hash collisions. Once the root position is found in the data base, the rest of the moves
// leading all the way to mate can be found in the database. In this case no positions are
// stored in the hash table and the normal search is abandoned - instead the routine simply
// stores the data base move and mate value in "MainLine[0]" and "mainVal".

void StoreKBNKpositions (ENGINE *E)
{
}

#ifdef kartoffel

#define mapA(sq) (sq)
#define mapB(sq) ((file(sq)<<4) + rank(sq))
#define mapC(sq) ((sq) ^ 0x77)
#define mapD(sq) mapC(mapB(sq))

COLOUR    col;                           // Side to move in current data base position.
INT       mateScore;                     // Score of position (always a mate score).


void StoreKBNKpositions (ENGINE *E)
{
   KBNK_DATA *Data;
   ULONG       ksq, Ksq, Bsq, Nsq, from, to, dest,
             kbnkSq, kbnkSqA, kbnkSqB, kbnkSqC, kbnkSqD,
             mirrorMask;
   INT       mateDepth, quadrant;
   HKEY      hashKey, hashIndex;
   TRANS     *t;

   if (! kbnk) return;                                       // Exit, if not KBNK.

   ksq = PieceLoc[loseColour + 0];                           // Get piece locations.
   Ksq = PieceLoc[winColour  + 0];
   Bsq = PieceLoc[winColour  + 1];
   Nsq = PieceLoc[winColour  + 2];
   kbnkSq = (ksq << 24) | (Ksq << 16) | (Bsq << 8) | Nsq;

   mirrorMask = (even(file(Bsq) ^ rank(Bsq)) ?               // Compute mirror mask.
                 0x00000000 : 0x07070707);

   if (! transTabOn)                                          // Don't store in hash table if
      quadrant = 0;                                          // it's off.
   else if (file(ksq) <= 3)                                 // Compute quadrant of lone king.      
      quadrant = (rank(ksq) <= 3 ? 1 : 2);
   else
      quadrant = (rank(ksq) >= 4 ? 3 : 4);

   for (Data = KBNKdata; Data->ksq >= a1; Data++)            // Store the KBNK data incl.
   {                                                         // symmetrical cases (depending
      kbnkSqA   = *(ULONG *)Data ^ mirrorMask;               // on the quadrant of the lone
      kbnkSqB   = ((kbnkSqA & 0x70707070) >> 4) |            // king).
                  ((kbnkSqA & 0x07070707) << 4);
      kbnkSqC   = kbnkSqA ^ 0x77777777;
      kbnkSqD   = kbnkSqB ^ 0x77777777;
      col       = pieceColour(Data->piece);                  // Get colour of side to move in
      if (winColour == black) col = black - col;            // data base.
      mateDepth = Data->mateDepth;
      mateScore = (mateDepth > 0 ? maxVal + 1 - 2*mateDepth : -2*mateDepth - maxVal);

      if (col == player)                                    // Check if root position is in
      {                                                      // data base. If so abort hash table
         to = nullSq;                                       // storing and return the move and
         dest = Data->to ^ (mirrorMask & 0x00FF);            // the mate value in MainLine and
         if (kbnkSq == kbnkSqA) to = mapA(dest); else         // mainVal.
         if (kbnkSq == kbnkSqB) to = mapB(dest); else
         if (kbnkSq == kbnkSqC) to = mapC(dest); else
         if (kbnkSq == kbnkSqD) to = mapD(dest);

         if (to != nullSq)
         {
            switch (Data->piece)
            {   case wKing   : from = Ksq; break;
               case wBishop : from = Bsq; break;
               case wKnight : from = Nsq; break;
               case bKing   : from = ksq;
            }
            MainLine[0].from   = from;
            MainLine[0].to      = to;
            MainLine[0].piece   = pieceType(Data->piece) + player;
            MainLine[0].cap   = empty;
            MainLine[0].type   = normal;
            MainLine[0].dir   = AttackDir[to - from] >> 5;
            MainLine[0].dply   = 1;
            clrMove(MainLine[1]);
            mainVal = mateScore;

            PrintLookE(++RootNode->ply);
            PrintCurrentE(0, MainLine);
            PrintMainLineE(MainLine);
            PrintScoreE(mainVal);
            return;
         }
      }

      switch (quadrant)
      {
         case 1 :   StoreKBNKpos(kbnkSqA); StoreKBNKpos(kbnkSqB); break;
         case 2 :   StoreKBNKpos(kbnkSqB); StoreKBNKpos(kbnkSqC); break;
         case 3 :   StoreKBNKpos(kbnkSqC); StoreKBNKpos(kbnkSqD); break;
         case 4 :   StoreKBNKpos(kbnkSqD); StoreKBNKpos(kbnkSqA);
      }
   }
}   /* StoreKBNKpositions */


void StoreKBNKpos (ULONG kbnkSq)
{
   HKEY   hashKey, hashIndex;
   TRANS  *t;

   hashKey  = HashCode[knight + winColour ][kbnkSq & 0x77];
   hashKey ^= HashCode[bishop + winColour ][(kbnkSq >>= 8) & 0x77];
   hashKey ^= HashCode[king   + winColour ][(kbnkSq >>= 8) & 0x77];
   hashKey ^= HashCode[king   + loseColour][kbnkSq >>= 8];
   hashIndex = hashKey & hashIndexMask;
   t = (col == white ? &TransTab1W[hashIndex] :
                       &TransTab1B[hashIndex]);
   if (t->ply == maxDepth)                           // If entry in primary hash table is already
      t += transSize/2;                              // occupied, try secondary hash table.
   if (t->ply == maxDepth || t >= TransTab2W)      // Skip if this entry is also occupied.
      return;

   t->key     = (hashKey & hashLockMask) | (1L << trueScoreBit);
   t->ply     = maxDepth;
   t->maxPly = maxDepth;
   t->score  = mateScore;
}   /* StoreKBNKpos */

#endif

/**************************************************************************************************/
/*                                                                                                */
/*                                         MISCELLANEOUS ROUTINES                                 */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------- Bonus/Penalty for Occupying Square ---------------------------*/

static void OccupySqPV
(  ENGINE *E,
   SQUARE sq,                                           // Assign bonus/penalty
   INT wP, INT wN, INT wB, INT wR, INT wQ, INT wK,      // for occupying "sq".
   INT bP, INT bN, INT bB, INT bR, INT bQ, INT bK
)
{
   PIECEVAL_STATE  *V = &E->V;

   V->PieceVal[wPawn][sq]   += wP;
   V->PieceVal[wKnight][sq] += wN;
   V->PieceVal[wBishop][sq] += wB;
   V->PieceVal[wRook][sq]   += wR;
   V->PieceVal[wQueen][sq]  += wQ;
   V->PieceVal[wKing][sq]   += wK;

   V->PieceVal[bPawn][sq]   -= bP;
   V->PieceVal[bKnight][sq] -= bN;
   V->PieceVal[bBishop][sq] -= bB;
   V->PieceVal[bRook][sq]   -= bR;
   V->PieceVal[bQueen][sq]  -= bQ;
   V->PieceVal[bKing][sq]   -= bK;
} /* OccupySqPV */

/*----------------------------- Bonus/Penalty for Attacking Square ---------------------------*/

static void AttackSqPV 
(  ENGINE *E,
   SQUARE sq,                                           // Assign bonus/penalty
   INT wP, INT wN, INT wB, INT wR, INT wQ, INT wK,      // for attacking "sq".
   INT bP, INT bN, INT bB, INT bR, INT bQ, INT bK
)
{
   GLOBAL          *G = E->Global;
   PIECEVAL_STATE  *V = &E->V;
   BOARD_STATE     *B = &E->B;
   SQUARE          asq, dir;

   if (file(sq) > 0)                                          // Pawn attack.
      V->PieceVal[wPawn][left(behind(sq))] += wP,
      V->PieceVal[bPawn][left(front(sq))]  -= bP;

   if (file(sq) < 7)
      V->PieceVal[wPawn][right(behind(sq))] += wP,
      V->PieceVal[bPawn][right(front(sq))]  -= bP;

   for (INT i = 0; i < 8; i++)
   {
      if (onBoard(asq = sq + G->B.KnightDir[i]))              // Knight attack.
         V->PieceVal[wKnight][asq] += wN,
         V->PieceVal[bKnight][asq] -= bN;

      dir = G->B.QueenDir[i];                                 // Queen, rook and bishop
      asq = sq;                                               // attack.
      while (onBoard(asq += dir))
      {
         if (i < 4)
            V->PieceVal[wBishop][asq] += wB,
            V->PieceVal[bBishop][asq] -= bB;
         else
            V->PieceVal[wRook][asq] += wR,
            V->PieceVal[bRook][asq] -= bR;

         V->PieceVal[wQueen][asq] += wQ;
         V->PieceVal[bQueen][asq] -= bQ;
         if (B->Board[asq] != empty) break;
      }

      if (onBoard(asq = sq + dir))                            // King attack.
         V->PieceVal[wKing][asq] += wK,
         V->PieceVal[bKing][asq] -= bK;
   }
} /* AttackSqPV */


/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION                                   */
/*                                                                                                */
/**************************************************************************************************/

static void InitBasePV (PIECEVAL_COMMON *V);
static void InitSpecialPV (PIECEVAL_COMMON *V);
static void InitKBNKPV (PIECEVAL_COMMON *V);

void InitPieceValModule (GLOBAL *Global)
{
   InitBasePV(&(Global->V));
   InitSpecialPV(&(Global->V));
} /* InitPieceValModule */

/*------------------------------------ Basic Constant PV Tables ----------------------------------*/
// Seen from White's point of view

static void InitBasePV (PIECEVAL_COMMON *V)
{
   INT PawnPV[64] =                               // Basic piece value table for pawns
   {   0,   0,   0,   0,   0,   0,   0,   0,
       7,  12,  15,  15,  15,  15,  12,   7,
      10,  15,  19,  22,  22,  19,  15,  10,
       7,  15,  18,  27,  27,  18,  15,   7,
       5,  12,  16,  25,  25,  16,  12,   5,
       2,   5,   8,  15,  15,   8,   5,   2,
       0,   0,   2,   5,   5,   2,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0
   };

   INT KnightPV[64] =                             // Basic piece value table for knights.
   { -15,   0,   0,   0,   0,   0,   0, -15,
       0,   5,   8,  10,  10,   8,   5,   0,
       5,  10,  15,  20,  20,  15,  10,   5,
      -5,  10,  20,  25,  25,  20,  10,  -5,
     -10,   7,  15,  20,  20,  15,   7, -10,
     -15,   5,  10,  15,  15,  10,   5, -15,
     -20,   2,   5,   7,   7,   5,   2, -20,
     -40, -20, -20, -20, -20, -20, -20, -40
   };

   INT BishopPV[64] =                             // Basic piece value table for bishops.
   {  -5,  -2,   0,   3,   3,   0,  -2,  -5,
      -2,   5,   6,   8,   8,   6,   5,  -2,
       0,   6,  10,  13,  13,  10,   6,   0,
       3,  10,  13,  15,  15,  13,  10,   3,
       3,  10,  13,  15,  15,  13,  10,   3,
       0,   6,  10,  13,  13,  10,   6,   0,
      -5,   5,   6,   8,   8,   6,   5,  -5,
     -25, -20, -20, -20, -20, -20, -20, -25,
   };

   INT RookPV[64] =                               // Basic piece value table for rooks.
   {  10,  12,  15,  15,  15,  15,  12,  10,
      20,  22,  25,  25,  25,  25,  22,  20,
       8,   9,  10,  10,  10,  10,   9,   8,
       2,   4,   6,   6,   6,   6,   4,   2,
       0,   2,   3,   4,   4,   3,   2,   0,
       0,   0,   1,   2,   2,   1,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,
       0,   1,   2,   3,   3,   2,   1,   0
   };

   INT QueenPV[64] =                             // Basic piece value table for queens.
   {  10,  12,  14,  14,  14,  14,  12,  10,
      12,  14,  16,  16,  16,  16,  14,  12,
       8,   9,  10,  10,  10,  10,   9,   8,
       2,   5,   7,   7,   7,   7,   5,   2,
       0,   2,   3,   4,   4,   3,   2,   0,
       0,   0,   1,   2,   2,   1,   0,   0,
       0,   0,   1,   1,   1,   1,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0
   };

   INT KingPV[64] =                              // Basic piece value table for kings.
   {   0,   0,   1,   2,   2,   1,   0,   0,
       0,   2,   3,   4,   4,   3,   2,   0,
       1,   3,   5,   7,   7,   5,   3,   1,
       2,   4,   7,  10,  10,   7,   4,   2,
       2,   4,   7,  10,  10,   7,   4,   2,
       1,   3,   5,   7,   7,   5,   3,   1,
       0,   2,   3,   4,   4,   3,   2,   0,
       0,   0,   1,   2,   2,   1,   0,   0
   };

   for (INT i = 0; i < 64; i++)
   {
      V->PawnPV[i]   = PawnPV[i];
      V->KnightPV[i] = KnightPV[i];
      V->BishopPV[i] = BishopPV[i];
      V->RookPV[i]   = RookPV[i];
      V->QueenPV[i]  = QueenPV[i];
      V->KingPV[i]   = KingPV[i];
   }
} /* InitBasePV */

/*----------------------------------- Special Constant PV Tables ---------------------------------*/

static void InitSpecialPV (PIECEVAL_COMMON *V)
{
   INT LoneKingPV[64] =                           // Piece value table for lonely king (that
   {   0,   0,   1,   2,   2,   1,   0,   0,      // is to be mated).
       0,   3,   4,   5,   5,   4,   3,   0,
       1,   4,   7,   9,   9,   7,   4,   1,
       2,   5,   9,  13,  13,   9,   5,   2,
       2,   5,   9,  13,  13,   9,   5,   2,
       1,   4,   7,   9,   9,   7,   4,   1,
       0,   3,   4,   5,   5,   4,   3,   0,
       0,   0,   1,   2,   2,   1,   0,   0
   };

   INT MKingPV[64] =                              // Piece value table for king when
   {  -8,  -7,  -6,  -5,  -5,  -6,  -7,  -8,      // opponent only has king left.
      -7,  -3,   0,   1,   1,   0,  -3,  -7,
      -6,   0,   5,   7,   7,   5,   0,  -6,
      -5,   1,   7,  10,  10,   7,   1,  -5,
      -5,   1,   7,  10,  10,   7,   1,  -5,
      -6,   0,   5,   7,   7,   5,   0,  -6,
      -7,  -3,   0,   1,   1,   0,  -3,  -7,
      -8,  -7,  -6,  -5,  -5,  -6,  -7,  -8
   };

   INT KBNK[64] =                                 // Special piece value table for the lone
   { 115, 110, 100,  90,  80,  70,  60,  55,      // king in the KBNK ending.
     110,  50,  45,  40,  35,  30,  35,  60,
     100,  45,  20,  15,  10,  10,  30,  70,
      90,  40,  15,   0,   0,  10,  35,  80,
      80,  35,  10,   0,   0,  15,  40,  90,
      70,  30,  10,  10,  15,  20,  45, 100,
      60,  35,  30,  35,  40,  45,  50, 110,
      55,  60,  70,  80,  90, 100, 110, 115
   };

   INT Closeness[256] = 
   {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
      0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0, 0, 0,
      0, 1, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 1, 0, 0,
      0, 1, 2, 3, 4, 5, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0,
      0, 1, 2, 3, 5, 7, 8, 8, 8, 7, 5, 3, 2, 1, 0, 0,
      0, 1, 2, 4, 6, 8,10,10,10, 8, 6, 4, 2, 1, 0, 0,
      0, 1, 2, 4, 6, 8,10,12,10, 8, 6, 4, 2, 1, 0, 0,
      0, 1, 2, 4, 6, 8,10,10,10, 8, 6, 4, 2, 1, 0, 0,
      0, 1, 2, 3, 5, 7, 8, 8, 8, 7, 5, 3, 2, 1, 0, 0,
      0, 1, 2, 3, 4, 5, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0,
      0, 1, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 1, 0, 0,
      0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0, 0, 0,
      0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  
   };

   INT Kcf[10] =                                  // Phase dependent king centrality factor.
   { -10, -8, -6, -4, -2, 1, 3, 5, 7, 9 };

   for (INT i = 0; i < 64; i++)
   {
      V->LoneKingPV[i] = LoneKingPV[i];
      V->MKingPV[i]    = MKingPV[i];
      V->KBNK[i]       = KBNK[i];
   }

   for (INT i = 0; i < 256; i++)
      V->Closeness[i - 119] = Closeness[i];

   for (INT i = 0; i < 10; i++)
      V->Kcf[i] = Kcf[i];
} /* InitSpecialPV */

/*------------------------------------ KBNK Transposition Data -----------------------------------*/
// This table is used in the absence of the KBNK endgame table base. It's used for prefilling the
// transposition tables with the known mating positions.

static void InitKBNKPV (PIECEVAL_COMMON *V)
{
   KBNK_DATA KBNKdata[kbnkDataSize] =
   {
      { f1, f3, h2, f2, bKing,   e1, -17 }, //  1 ke1:2
      { e1, f3, h2, f2, wKnight, e4, +17 }, //  2 Ne4:3
      { e1, f3, h2, e4, bKing,   d1, -16 }, //  3 kf1:4, kd1:56
      { f1, f3, h2, e4, wKnight, d2, +14 }, //  4 Nd2:5
      { f1, f3, h2, d2, bKing,   e1, -13 }, //  5 ke1:6
      { e1, f3, h2, d2, wKing,   e3, +13 }, //  6 Ke3:7
      { e1, e3, h2, d2, bKing,   d1, -12 }, //  7 kd1:8
      { d1, e3, h2, d2, wKing,   d3, +12 }, //  8 Kd3:9
      { d1, d3, h2, d2, bKing,   c1, -11 }, //  9 ke1:10, kc1:32
      { e1, d3, h2, d2, wBishop, g1, +11 }, // 10 Bg1:11, (Bg3:30)
      { e1, d3, g1, d2, bKing,   d1, -10 }, // 11 kd1:12
      { d1, d3, g1, d2, wBishop, f2, +10 }, // 12 Bf2:13
      { d1, d3, f2, d2, bKing,   c1,  -9 }, // 13 kc1:14
      { c1, d3, f2, d2, wKnight, c4,  +9 }, // 14 Nc4:15
      { c1, d3, f2, c4, bKing,   d1,  -8 }, // 15 kd1:16, kb1:24
      { d1, d3, f2, c4, wKnight, b2,  +8 }, // 16 Nb2:17
      { d1, d3, f2, b2, bKing,   c1,  -7 }, // 17 kc1:18
      { c1, d3, f2, b2, wKing,   c3,  +7 }, // 18 Kc3:19
      { c1, c3, f2, b2, bKing,   b1,  -6 }, // 19 kb1:20
      { b1, c3, f2, b2, wKing,   b3,  +6 }, // 20 Kb3:21
      { b1, b3, f2, b2, bKing,   c1,  -5 }, // 21 ka1:22, kc1:23
      { a1, b3, f2, b2, wKing,   c2,  +5 }, // 22 Kc2:97
      { c1, b3, f2, b2, wBishop, e1,  +5 }, // 23 Be1:105
      { b1, d3, f2, c4, wKing,   d2,  +7 }, // 24 Kd2:25
      { b1, d2, f2, c4, bKing,   a2,  -6 }, // 25 ka2:26, ka1:29
      { a2, d2, f2, c4, wKing,   c2,  +6 }, // 26 Kc2:27
      { a2, c2, f2, c4, bKing,   a1,  -5 }, // 27 ka1:28
      { a1, c2, f2, c4, wKnight, b2,  +5 }, // 28 Nb2:97
      { a1, d2, f2, c4, wKing,   c2,  +5 }, // 29 Kc2:113
//    { e1, d3, g3, d2, bKing,   d1, -10 }, // 30 kd1:31
//    { d1, d3, g3, d2, wBishop, f2, +10 }, // 31 Bf2:13
      { c1, d3, h2, d2, wBishop, e5, +11 }, // 32 Be5:33
      { c1, d3, e5, d2, bKing,   d1, -10 }, // 33 kd1:34
      { d1, d3, e5, d2, wBishop, g3, +10 }, // 34 Bg3:35, (Bb2:52)
      { d1, d3, g3, d2, bKing,   c1,  -9 }, // 35 kc1:36
      { c1, d3, g3, d2, wKnight, c4,  +9 }, // 36 Nc4:37
      { c1, d3, g3, c4, bKing,   d1,  -8 }, // 37 kd1:38, kb1:46
      { d1, d3, g3, c4, wKnight, b2,  +8 }, // 38 Nb2:39
      { d1, d3, g3, b2, bKing,   c1,  -7 }, // 39 kc1:40
      { c1, d3, g3, b2, wKing,   c3,  +7 }, // 40 Kc3:41
      { c1, c3, g3, b2, bKing,   b1,  -6 }, // 41 kb1:42
      { b1, c3, g3, b2, wKing,   b3,  +6 }, // 42 Kb3:43
      { b1, b3, g3, b2, bKing,   c1,  -5 }, // 43 ka1:44, kc1:45
      { a1, b3, g3, b2, wBishop, f4,  +5 }, // 44 Bf4:129
      { c1, b3, g3, b2, wBishop, e1,  +5 }, // 45 Be1:105
      { b1, d3, g3, c4, wKing,   d2,  +7 }, // 46 Kd2:47 
      { b1, d2, g3, c4, bKing,   a2,  -6 }, // 47 ka2:48, ka1:51
      { a2, d2, g3, c4, wKing,   49,  +6 }, // 48 Kc2:49
      { a2, c2, g3, c4, bKing,   a1,  -5 }, // 49 ka1:50
      { a1, c2, g3, c4, wBishop, f2,  +5 }, // 50 Bf2:113
      { a1, d2, g3, c4, wKing,   c2,  +5 }, // 51 Kc2:121
//    { d1, d3, b2, d2, bKing,   e1, -11 }, // 52 ke1:53
//    { e1, d3, b2, d2, wBishop, d4, +11 }, // 53 Bd4:54
//    { e1, d3, d4, d2, bKing,   d1, -10 }, // 54 kd1:55
//    { d1, d3, d4, d2, wBishop, f2, +10 }, // 55 Bf2:13
      { d1, f3, h2, e4, wKing,   e3, +16 }, // 56 Ke3:57
      { d1, e3, h2, e4, bKing,   c2, -15 }, // 57 ke1:58, kc2:59, kc1:94
      { e1, e3, h2, e4, wKnight, d2, +13 }, // 58 Nd2:7
      { c2, e3, h2, e4, wKnight, d2, +15 }, // 59 Nd2:60
      { c2, e3, h2, d2, bKing,   b2, -14 }, // 60 kd1:8, kc3:61, kc1:76, kb2:80
      { c3, e3, h2, d2, wBishop, d6, +14 }, // 61 Bd6:62
      { c3, e3, d6, d2, bKing,   c2, -13 }, // 62 kc2:63, kb2:71
      { c2, e3, d6, d2, wBishop, e5, +13 }, // 63 Be5:64
      { c2, e3, e5, d2, bKing,   d1, -12 }, // 64 kd1:65, kc1:70
      { d1, e3, e5, d2, wKing,   d3, +12 }, // 65 Kd3:66
      { d1, d3, e5, d2, bKing,   c1, -11 }, // 66 ke1:67, kc1:68
      { e1, d3, e5, d2, wBishop, d4, +11 }, // 67 Bd4:54
      { c1, d3, e5, d2, wBishop, d4, +11 }, // 68 Bd4:69
      { c1, d3, d4, d2, bKing,   d1, -10 }, // 69 kd1:55
      { c1, e3, e5, d2, wKing,   d3, +11 }, // 70 Kd3:33
      { b2, e3, d6, d2, wKing,   d3, +12 }, // 71 Kd3:72
      { b2, d3, d6, d2, bKing,   c1, -11 }, // 72 ka1:73, ka2:74, kc1:75
      { a1, d3, d6, d2, wKing,   c3,  +5 }, // 73 Kc3:137
      { a2, d3, d6, d2, wKing,   c2,  +4 }, // 74 Kc2:131
      { c1, d3, d6, d2, wBishop, e5, +11 }, // 75 Be5:33
      { c1, e3, h2, d2, wKing,   d3, +13 }, // 76 Kd3:77
      { c1, d3, h2, d2, bKing,   b2, -12 }, // 77 kd1:78, kb2:79
      { d1, d3, h2, d2, wBishop, e5, +12 }, // 78 Be5:66
      { b2, d3, h2, d2, wBishop, d6, +12 }, // 79 Bd6:75
      { b2, e3, h2, d2, wBishop, d6, +14 }, // 80 Bd6:81
      { b2, e3, d6, d2, bKing,   c2, -13 }, // 81 kc1:82, kc2:63, ka1:87, ka2:89
      { c1, e3, d6, d2, wBishop, e5, +13 }, // 82 Be5:83
      { c1, e3, e5, d2, bKing,   c2, -12 }, // 83 kd1:65, kc2:84
      { c2, e3, e5, d2, wKing,   e2, +12 }, // 84 Ke2:85
      { c2, e2, e5, d2, bKing,   c1, -11 }, // 85 kc1:86
      { c1, e2, e5, d2, wKing,   d3, +11 }, // 86 Kd3:33
      { a1, e3, d6, d2, wKing,   d3,  +5 }, // 87 Kd3:88
      { a1, d3, d6, d2, bKing,   a2,  -4 }, // 88 ka2:74
      { a2, e3, d6, d2, wKing,   d3,  +7 }, // 89 Kd3:90
      { a2, d3, d6, d2, bKing,   b2,  -6 }, // 90 ka1:73, kb2:91
      { b2, d3, d6, d2, wBishop, b4,  +6 }, // 91 Bb4:92
      { b2, d3, b4, d2, bKing,   a1,  -5 }, // 92 ka1:93, 
      { a1, d3, b4, d2, wKing,   c2,  +5 }, // 93 Kc2:139
      { c1, e3, h2, e4, wKnight, d2, +15 }, // 94 Nd2:95
      { c1, e3, h2, d2, bKing,   b2, -14 }, // 95 kd1:8, kc2:96, kb2:80
      { c2, e3, h2, d2, wBishop, e5, +13 }, // 96 Be5:64

      { a1, c2, f2, b2, bKing,   a2,  -4 }, // 97 ka2:98
      { a2, c2, f2, b2, wBishop, c5,  +4 }, // 98 Bc5:99
      { a2, c2, c5, b2, bKing,   a1,  -3 }, // 99 kc1:100
      { a1, c2, c5, b2, wKnight, d3,  +3 }, //100 Nd3:101
      { a1, c2, c5, d3, bKing,   a2,  -2 }, //101 ka2:102
      { a2, c2, c5, d3, wKnight, c1,  +2 }, //102 Nc1:103
      { a2, c2, c5, c1, bKing,   a1,  -1 }, //103 ka1:104
      { a1, c2, c5, c1, wBishop, d4,  +1 }, //104 Bd4 mate
   
      { c1, b3, e1, b2, bKing,   b1,  -4 }, //105 kb1:106
      { b1, b3, e1, b2, wBishop, d2,  +4 }, //106 Bd2:107
      { b1, b3, d2, b2, bKing,   a1,  -3 }, //107 ka1:108
      { a1, b3, d2, b2, wKnight, c4,  +3 }, //108 Nc4:109
      { a1, b3, d2, c4, bKing,   b1,  -2 }, //109 kb1:110
      { b1, b3, d2, c4, wKnight, a3,  +2 }, //110 Na3:111
      { b1, b3, d2, a3, bKing,   a1,  -1 }, //111 ka1:112
      { a1, b3, d2, a3, wBishop, c3,  +1 }, //112 Bc3 mate
   
      { a1, c2, f2, c4, bKing,   a2,  -4 }, //113 ka2:114
      { a2, c2, f2, c4, wBishop, e3,  +4 }, //114 Be3:115
      { a2, c2, e3, c4, bKing,   a1,  -3 }, //115 ka1:116
      { a1, c2, e3, c4, wKing,   b3,  +3 }, //116 Kb3:117
      { a1, b3, e3, c4, bKing,   b1,  -2 }, //117 kb1:118
      { b1, b3, e3, c4, wKnight, a3,  +2 }, //118 Na3:119
      { b1, b3, e3, a3, bKing,   a1,  -1 }, //119 ka1:120
      { a1, b3, e3, a3, wBishop, d4,  +1 }, //120 Bd4 mate
   
      { a1, c2, g3, c4, bKing,   a2,  -4 }, //121 ka2:122
      { a2, c2, g3, c4, wBishop, f4,  +4 }, //122 Bf4:123
      { a2, c2, f4, c4, bKing,   a1,  -3 }, //123 ka1:124
      { a1, c2, f4, c4, wKing,   b3,  +3 }, //124 Kb3:125
      { a1, b3, f4, c4, bKing,   b1,  -2 }, //125 kb1:126
      { b1, b3, f4, c4, wKnight, a3,  +2 }, //126 Na3:127
      { b1, b3, f4, a3, bKing,   a1,  -1 }, //127 ka1:128
      { a1, b3, f4, a3, wBishop, e5,  +1 }, //128 Be5 mate
   
      { a1, b3, f4, b2, bKing,   b1,  -4 }, //129 kb1:130
      { b1, b3, f4, b2, wBishop, d2,  +4 }, //130 Bd2:107
   
      { a2, c2, d6, d2, bKing,   a1,  -3 }, //131 ka1:132
      { a1, c2, d6, d2, wKnight, b3,  +3 }, //132 Nb3:133
      { a1, c2, d6, b3, bKing,   a2,  -2 }, //133 ka2:134
      { a2, c2, d6, b3, wKnight, c1,  +2 }, //134 Nc1:135
      { a2, c2, d6, c1, bKing,   a1,  -1 }, //135 ka1:136
      { a1, c2, d6, c1, wBishop, e5,  +1 }, //136 Be5 mate
   
      { a1, c3, d6, d2, bKing,   a2,  -4 }, //137 ka2:138
      { a2, c3, d6, d2, wKing,   c2,  +4 }, //138 Kc2:132
   
      { a1, c2, b4, d2, bKing,   a2,  -4 }, //139 ka2:140
      { a2, c2, b4, d2, wBishop, d6,  +4 }, //140 Bd6:131

      { nullSq, 0, 0, 0, 0, 0, 0 }
   };

   for (INT i = 0; i < kbnkDataSize; i++)
      V->KBNKdata[i] = KBNKdata[i];
} /* InitKBNKPV */
