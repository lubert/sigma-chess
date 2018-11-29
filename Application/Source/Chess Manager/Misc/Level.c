/**************************************************************************************************/
/*                                                                                                */
/* Module  : Level.c                                                                              */
/* Purpose : This module implements the Playing Mode and Level structure.                         */
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

#include "Level.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                          RESET LEVEL                                           */
/*                                                                                                */
/**************************************************************************************************/

void Level_Reset (LEVEL *L)
{
   L->title[0] = 0;  // Title for user defined settings.

   L->mode = pmode_TimeMoves;

   // Set "Time/Moves" default: All moves in 5 minutes (blitz)
   L->TimeMoves.time      = 5*60;
   L->TimeMoves.moves     = allMoves;
   L->TimeMoves.clockType = clock_Normal;
   L->TimeMoves.delta     = Max(1, L->TimeMoves.time/60);

   // Set "Tournament" default: 40 moves in 2 hours, 30 mins next 20 moves, and finally 30 mins for the remaining moves.
   L->Tournament.wtime[0]  = 120*60;
   L->Tournament.btime[0]  = 120*60;
   L->Tournament.moves[0]  = 40;
   L->Tournament.wtime[1]  = 30*60;
   L->Tournament.btime[1]  = 30*60;
   L->Tournament.moves[1]  = 20;
   L->Tournament.wtime[2]  = 30*60;
   L->Tournament.btime[2]  = 30*60;
   L->Tournament.moves[2]  = allMoves;   // This value may NOT be changed by the user

   // Set "Average" default: 5 seconds per move
   L->Average.secs = 5;

   // Set "Fixed Depth" default: 1 ply
   L->FixedDepth.depth = 1;

   // Set "Solver" default: 1 minute (and no score limit)
   L->Solver.timeLimit  = 10;        // Time limit in seconds (or -1 if no time limit)
   L->Solver.scoreLimit = maxVal;    // Search stops when/if this score limit exceeded

   // Set "MateFinder" default: 2 movers
   L->MateFinder.mateDepth = 2;

   // Set "Novice" default: Easiest
   L->Novice.level = 1;
} /* Level_Reset */


/**************************************************************************************************/
/*                                                                                                */
/*                                       TIME ALLOCATION                                          */
/*                                                                                                */
/**************************************************************************************************/

// Should e.g. be called when resetting chess clocks.

LONG Level_CalcTotalTime (LEVEL *L, COLOUR player)
{
   switch (L->mode)
   {
      case pmode_TimeMoves : 
         return L->TimeMoves.time;
      case pmode_Tournament :
         return (player == white ? L->Tournament.wtime[0] : L->Tournament.btime[0]);
      case pmode_Solver :
         return L->Solver.timeLimit;
      default :
         return -1;
   }
} /* Level_CalcTotalTime */

// Whenever a move has been played in the countdown time controls, we need to check if a time control
// has been reached, and hence if more time should be allocated. Additionally if the Fischer clock is
// enabled we must always add a small amount of extra time for each move performed.

LONG Level_CheckTimeControl (LEVEL *L, COLOUR player, INT played)
{
   LONG extraTime = 0;

   switch (L->mode)
   {
      case pmode_TimeMoves :
         INT limit = L->TimeMoves.moves;
         if (limit != allMoves && played % limit == 0)
            extraTime += L->TimeMoves.time;
         if (L->TimeMoves.clockType == clock_Fischer)
            extraTime += L->TimeMoves.delta;
         break;

      case pmode_Tournament :
         INT limit1 = L->Tournament.moves[0];
         INT limit2 = L->Tournament.moves[1] + limit1;
         if (played == limit1)
            extraTime += (player == white ? L->Tournament.wtime[1] : L->Tournament.btime[1]);
         else if (played == limit2)
            extraTime += (player == white ? L->Tournament.wtime[2] : L->Tournament.btime[2]);
         break;
      default :
         return 0;
   }

   return extraTime;
} /* Level_CheckTimeControl */
