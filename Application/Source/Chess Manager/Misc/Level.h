#pragma once

#include "General.h"
#include "Engine.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

enum PLAYING_MODE
{
   pmode_TimeMoves   =  1,
   pmode_Tournament  =  2,
   pmode_Average     =  3,
   pmode_Leisure     =  4,         // No sub-levels
   pmode_FixedDepth  =  5,
   pmode_Novice      =  6,

   pmode_Infinite    =  7,         // No sub-levels
   pmode_Solver      =  8,
   pmode_MateFinder  =  9,

   pmode_Monitor     = 10,         // No sub-levels
   pmode_Manual      = 11,

   playingModeCount  = 11
};

enum CLOCK_TYPES
{
   clock_Normal      = 0,
   clock_Fischer     = 1
// clock_Bronstein   = 2
};

#define levelTitleLen 50


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

// Each game window is associated with a CLevel object describing the playing modes, levels etc
// which applies to that window. Additionally, the most recently edited CLevel object is stored
// in the preferences file and is used as the default when creating new game windows. All times are
// specified in seconds.

typedef struct
{
   CHAR title[levelTitleLen + 1];    // User definable name of custom sets

   //--- Playing Mode ---

   INT mode;          // Currently used playing mode (i.e. default mode for custom sets)

   //--- Playing Levels for Each Mode ---

   struct
   {
      INT time;       // Total time budget.
      INT moves;      // Moves to play (-1 if all) within "time".

      INT clockType;  // Normal, Fischer, Bronstein
      INT delta;      // Time gain per move (in seconds) if Fischer or Bronstein clock.
   } TimeMoves;

   struct
   {
      INT wtime[3];   // Total White time for each of the 3 time control
      INT btime[3];   // Total Black time for each of the 3 time control
      INT moves[3];   // Moves to play within each time control (always all in last time control).
   } Tournament;

   struct
   {
      INT secs;       // Seconds per move.
   } Average;

   struct
   {
      INT depth;      // Maximum nominal search depth (in plies)
   } FixedDepth;

   struct
   {
      INT timeLimit;  // Time limit (or -1) if no time limit
      INT scoreLimit; // Search stops when this limit exceeded (maxVal) if no limit.
   } Solver;

   struct
   {
      INT mateDepth;  // Moves to mate.
   } MateFinder;

   struct
   {
      INT level;      // Novice level 1..8.
   } Novice;
} LEVEL;


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES                                 */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

void Level_Reset (LEVEL *L);
LONG Level_CalcTotalTime (LEVEL *L, COLOUR player);
LONG Level_CheckTimeControl (LEVEL *L, COLOUR player, INT movesPlayed);
