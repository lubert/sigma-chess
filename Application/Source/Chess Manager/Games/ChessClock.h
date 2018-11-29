#pragma once

#include "General.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                     TYPE/CLASS DEFINITIONS                                     */
/*                                                                                                */
/**************************************************************************************************/

class CChessClock
{
public:
   CChessClock (void);

   void Start (void);
   void Stop (void);
   void Reset (LONG maxSecs = -1);
   void RecalcState (void);
   BOOL Tick (void);

   BOOL running;           // Is this clock currently running?
   BOOL timeOut;
   CHAR state[9];
   LONG elapsed;           // Elapsed time in whole seconds
   BOOL countDown;         // Are we using countdown?
   LONG maxSecs;           // Maximum number of seconds (if countdown)

private:
   void Inc (void);
   void Dec (void);

   LONG ticks0;   
   LONG nextTick;          // Value of "Clock()" at start of search.
   INT  corrector;         // Corrector count. Because the true frequency of Clock ticks is 60.15 Hz, it
                           // will gain 3 ticks in 20 secs.
};


/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES                                      */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

void FormatClockTime (ULONG secs, CHAR s[]);
