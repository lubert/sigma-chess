/**************************************************************************************************/
/*                                                                                                */
/* Module  : TIME.C */
/* Purpose : This module controls the time allocation, including the logic for
 * moving quicker for */
/*           forced moves etc. Is only applied in the "mode_Time" playing mode.
 */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Engine.h"

#include "Time.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                         TIME ALLOCATION/CONTROL */
/*                                                                                                */
/**************************************************************************************************/

void AllocateTime(ENGINE *E)  // Allocates time for the forthcoming search.
{
  TIME_STATE *T = &E->T;

  INT movesLeft = E->P.movesLeft;  // Number of moves left to next time control.
  INT movesPlayed =
      E->P.movesPlayed;  // Number of moves played since last time control.
  ULONG secsLeft = E->P.timeLeft;  // Time left to next time control.

  ULONG avgTicks,  // Average time pr. move to next time control.
      nomTicks,    // Nominal time for next move.
      maxTicks,    // Time left on players clock.
      Tm[4];       // Time dependant on move type of BestLine[0].

  if (movesLeft == allMoves)  // Moves to play within time limit.
    movesLeft = Max(60 - E->P.lastMoveNo / 2,
                    15);  // If "all moves" always assume at least 15 moves left

  avgTicks = (60 * (secsLeft - 2)) / movesLeft +
             1;  // Average time pr. move to next control.
  nomTicks =
      MaxL(15, 35 - movesPlayed) * avgTicks / 10;  // Time for current move.
  // maxTicks = MaxL(10, 60*(secsLeft - Min(30,movesLeft)));  // Maximum time
  // for current move: At
  maxTicks =
      MaxL(6, 60 * (secsLeft - 2) -
                  30 * Min(30, movesLeft));  // Maximum time for current move:
                                             // At least one 1/10 sec pr. move.
  Tm[timer_Recap] = nomTicks / 4 + 30;   // Forced recaptures.
  Tm[timer_Forced] = nomTicks / 2 + 30;  // Other forced moves.
  Tm[timer_Normal] = nomTicks;           // Normal moves.
  Tm[timer_Sacri] = (3 * nomTicks) / 2;  // Sacrifices.

  for (INT i = timer_Recap; i <= timer_Sacri; i++) {
    T->NormalTime[i] = MinL(maxTicks, Tm[i]) + Timer();
    T->IterationTime[i] = MinL(maxTicks, (5 * Tm[i]) / 6) + Timer();
  }

  T->timer = timer_Normal;  // Initially select normal timer.
  T->maxTime = MinL(maxTicks, 3 * nomTicks) + Timer();  // Emergency brakes.
  T->ultraTime = MinL(maxTicks, 6 * nomTicks) + Timer();
  T->nominalTime = T->NormalTime[timer_Normal];
} /* AllocateTime */

void AdjustTimeLimit(ENGINE *E)  // Adjusts the time limit for the search by
{                                // choosing one of the four timers
  NODE *N = E->S.rootNode;

  if (N->m.dply == 0)
    E->T.timer = (N->m.misc == gen_C ? timer_Recap : timer_Forced);
  else
    E->T.timer = (N->m.misc != gen_I ? timer_Normal : timer_Sacri);

  E->T.nominalTime = E->T.NormalTime[E->T.timer];
} /* AdjustTimeLimit */

/**************************************************************************************************/
/*                                                                                                */
/*                                             TIME CONTROL */
/*                                                                                                */
/**************************************************************************************************/

BOOL TimeOut(ENGINE *E) {
  LONG timer = Timer();
  if (timer < E->T.nominalTime || E->P.playingMode != mode_Time ||
      E->P.backgrounding ||
      (E->R.state == state_Running && E->S.rootNode->ply == 1 &&
       (!E->P.reduceStrength || E->P.engineELO >= 1500)))
    return false;

  if (timer < E->T.maxTime)
    return (E->S.mainScore > E->S.prevScore - 50 ||
            (E->S.mainScore > -200 && E->P.reduceStrength &&
             E->P.engineELO < 1500));

  return (E->S.mainScore > E->S.prevScore - 300 || timer >= E->T.ultraTime ||
          (E->P.reduceStrength && E->P.engineELO < 1500));
} /* TimeOut */

BOOL TimeForAnotherIteration(ENGINE *E) {
  return (E->P.backgrounding || Timer() <= E->T.IterationTime[E->T.timer]);
} /* TimeForAnotherIteration */

/**************************************************************************************************/
/*                                                                                                */
/*                                             MISCELLANEOUS */
/*                                                                                                */
/**************************************************************************************************/

void PauseTimeAdjust(ENGINE *E,
                     LONG pauseDuration)  // If the program has been paused we
{                                         // must adjust the time limits.
  for (INT i = timer_Recap; i <= timer_Sacri; i++)
    E->T.NormalTime[i] += pauseDuration, E->T.IterationTime[i] += pauseDuration;

  E->T.maxTime += pauseDuration;
  E->T.ultraTime += pauseDuration;
  E->T.nominalTime += pauseDuration;
} /* PauseTimeAdjust */
