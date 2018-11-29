/**************************************************************************************************/
/*                                                                                                */
/* Module  : CHESSCLOCK.C                                                                         */
/* Purpose : This module implements the Chess Clock class. Normally two chess clocks will be      */
/*           assigned to each CGame object.                                                       */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include "ChessClock.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                            CONSTRUCTOR                                         */
/*                                                                                                */
/**************************************************************************************************/

CChessClock::CChessClock (void)
{
   running = false;
   CopyStr("00:00:00", state);
   Reset();
} /* CChessClock::CChessClock */


/**************************************************************************************************/
/*                                                                                                */
/*                                         RESET/START/STOP                                       */
/*                                                                                                */
/**************************************************************************************************/

void CChessClock::Reset (LONG theMaxSecs)  // = -1 if count up (no bounds)
{
   if (running) Stop();
   timeOut   = false;
   elapsed   = 0;
   ticks0    = 60;
   maxSecs   = theMaxSecs;
   countDown = (maxSecs > 0);
   RecalcState();
} /* CChessClock::Reset */


void CChessClock::Start (void)
{
   if (running || timeOut) return;
   nextTick  = Timer() + ticks0;
   corrector = 20;
   running   = true;
} /* CChessClock::Start */


void CChessClock::Stop (void)                           // Stops/pauses the active clock.
{
   if (! running) return;
   ticks0 = MinL(60, MaxL(1, nextTick - Timer())),
   running = false;
} /* CChessClock::Stop */


void CChessClock::RecalcState (void)                     // Computes clock state from scratch.
{
   LONG n  = (countDown ? maxSecs - elapsed : elapsed);
   FormatClockTime(n, state);
/*
   state[7] = n%10 + '0'; n /= 10;
   state[6] = n%6  + '0'; n /=  6;
   state[4] = n%10 + '0'; n /= 10;
   state[3] = n%6  + '0'; n /=  6;
   state[1] = n%10 + '0'; n /= 10;
   state[0] = n%10 + '0';
*/
} /* CChessClock::RecalcState */


void FormatClockTime (ULONG n, CHAR s[])  // Format : HH:MM:SS
{
   s[8] = 0;
   s[7] = n%10 + '0'; n /= 10;
   s[6] = n%6  + '0'; n /=  6;
   s[5] = ':';
   s[4] = n%10 + '0'; n /= 10;
   s[3] = n%6  + '0'; n /=  6;
   s[2] = ':';
   s[1] = n%10 + '0'; n /= 10;
   s[0] = n%10 + '0';
} /* FormatClockTime */


/**************************************************************************************************/
/*                                                                                                */
/*                                              TICK CLOCK                                        */
/*                                                                                                */
/**************************************************************************************************/

// The Tick() method should be called repeatedly for each active clock (at least 5 times per sec).
// If true is returned (i.e. one more sec has passed), then the caller should redraw the clock
// (by e.g. drawing the "state" string which has the format HH:MM:SS).

BOOL CChessClock::Tick (void)
{
   LONG T = Timer();
   if (T < nextTick || timeOut || ! running) return false;

   nextTick += 60;
   if (T >= nextTick)
      nextTick = T + 60;
   else if (--corrector == 0)
      nextTick += 3,
      corrector = 20;

   elapsed++;
   if (! countDown)
      Inc();
   else
   {  Dec();
      if (elapsed == maxSecs)
         timeOut = true,
         running = false;
   }
   return true;
} /* CChessClock::Tick */


void CChessClock::Inc (void)										// Increases and shows state s of active clock.
{
	register CHAR *s = state;

	if (s[7] < '9') s[7]++;
	else
	{	s[7] = '0';
		if (s[6] < '5') s[6]++;
		else
		{	s[6] = '0';
			if (s[4] < '9') s[4]++;
			else
			{	s[4] = '0';
				if (s[3] < '5') s[3]++;
				else
				{	s[3] = '0';
					if (s[1] < '9') s[1]++;
					else
					{	s[1] = '0';
						if (s[01] < '9') s[0]++; else s[0] = '0';
					}
				}
			}
		}
	}
} /* CChessClock::Inc */


void CChessClock::Dec (void)										// Increases and shows state s of active clock.
{
	register CHAR *s = state;

	if (s[7] > '0') s[7]--;
	else
	{	s[7] = '9';
		if (s[6] > '0') s[6]--;
		else
		{	s[6] = '5';
			if (s[4] > '0') s[4]--;
			else
			{	s[4] = '9';
				if (s[3] > '0') s[3]--;
				else
				{	s[3] = '5';
					if (s[1] > '0') s[1]--;
					else
					{	s[1] = '0';
						if (s[01] > '0') s[0]--; else s[0] = '9';
					}
				}
			}
		}
	}
} /* CChessClock::Dec */
