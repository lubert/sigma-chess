/**************************************************************************************************/
/*                                                                                                */
/* Module  : Rating.c                                                                             */
/* Purpose : This module implements the various ELO rating routines and dialogs.                  */
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

#include "Rating.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                   RESET PLAYING STRENGTH INFO                                  */
/*                                                                                                */
/**************************************************************************************************/

/*
void ResetEngineRating (ENGINE_RATING *S)
{
   S->reduceStrength = false;
   S->engineELO      = 2400;
   S->autoReduce     = false;
} /* ResetEngineRating */


/**************************************************************************************************/
/*                                                                                                */
/*                                       PLAYER RATING STATS                                      */
/*                                                                                                */
/**************************************************************************************************/

void ResetPlayerRating (PLAYER_RATING *PR, INT initPlayerELO)
{
   for (INT i = 0; i <= 2; i++)
   {  PR->gameCount[i]  = 0;
      PR->wonCount[i]   = 0;
      PR->lostCount[i]  = 0;
      PR->drawnCount[i] = 0;
   }

   PR->initELO = initPlayerELO;
   PR->currELO = initPlayerELO;
   PR->minELO  = initPlayerELO;
   PR->maxELO  = initPlayerELO;
   PR->sigmaELOsum = 0;
} /* ResetPlayerRating */


void UpdatePlayerRating (PLAYER_RATING *PR, BOOL playerWasWhite, REAL playerScore, INT sigmaELO)
{
   INT inx = (playerWasWhite ? rating_White : rating_Black);  // Player's counter index

   PR->gameCount[rating_Total]++;
   PR->gameCount[inx]++;

   if (playerScore == 1.0)
   {  PR->wonCount[rating_Total]++;
      PR->wonCount[inx]++;
   }
   else if (playerScore == 0.0)
   {  PR->lostCount[rating_Total]++;
      PR->lostCount[inx]++;
   }
   else
   {  PR->drawnCount[rating_Total]++;
      PR->drawnCount[inx]++;
   }

   PR->sigmaELOsum += sigmaELO;
   PR->currELO = UpdateELO(PR->currELO, sigmaELO, playerScore);
   PR->minELO = Min(PR->currELO, PR->minELO);
   PR->maxELO = Max(PR->currELO, PR->maxELO);

   if (PR->gameCount[rating_Total] <= maxRatingHistoryCount)
   {
      UINT h = sigmaELO;
      if (! playerWasWhite) h |= 0x8000;
      h |= ((UINT)(2*playerScore)) << 13;
      PR->history[PR->gameCount[rating_Total] - 1] = h;
   }
} /* UpdatePlayerRating */


/**************************************************************************************************/
/*                                                                                                */
/*                                     ELO CONVERSION FORMULA                                     */
/*                                                                                                */
/**************************************************************************************************/

// Conversion between ELO difference and expected score

INT Score_to_ELO (REAL score)  // score : [0..1] --> diff : [-1000..1000]
{
   if (score >= 1.0) return 1000;
   else if (score <= 0.0) return -1000;

   return -400*log10(1.0/score - 1.0);
} /* Score_to_ELO */


REAL ELO_to_Score (INT diff)   // diff : [-1000..1000] --> score : [0..1]
{
   if (diff >= 1000) return 1.0;
   else if (diff <= -1000) return 0.0;

   return 1.0/(1.0 + pow(10.0,-((REAL)diff)/400.0));
} /* ELO_to_Score */


INT UpdateELO (INT playerELO, INT opponentELO, REAL actualScore)  // actualScore [0..1]
{
   // Rating change coefficient:
   INT K;
   if (playerELO < 2000) K = 30;
   else if (playerELO > 2400) K = 10;
   else K = 130 - playerELO/20;

   // ELO difference:
   REAL expectedScore = ELO_to_Score(playerELO - opponentELO);

   // New player rating:
   return playerELO + K*((REAL)actualScore - expectedScore);
} /* UpdateELO */

