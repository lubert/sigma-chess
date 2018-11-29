#pragma once

#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define maxRatingHistoryCount 10000

#define kSigmaMinELO 1200
#define kSigmaMaxELO 2500

enum RATING_INDEX { rating_White = 0, rating_Black = 1, rating_Total = 2 };

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef struct {
  BOOL reduceStrength;  // Reduce strength of engine?
  INT engineELO;        // Default maximum (2400) - Needs initial calibration.
  BOOL autoReduce;  // Automatically decrease ELO if Computer not fast enough?
                    // (default OFF!!)
} ENGINE_RATING;

typedef struct {
  CHAR name[50];  // Name of this rating profile

  // Score count
  INT gameCount[3];  // Number of games (total, white & black)
  INT wonCount[3];   // Number of won/lost/drawn games.
  INT lostCount[3];
  INT drawnCount[3];

  // ELO stats
  INT initELO;
  INT currELO;
  INT minELO;
  INT maxELO;
  LONG sigmaELOsum;  // Used for average calculation

  // Result history (one entry per game)
  UINT history[maxRatingHistoryCount];  // Bit 15     : player colour (0=white,
                                        // 1=black) Bits 14-13 : result (0=loss,
                                        // 1=draw, 2=win) Bits 11-0  : Sigma ELO
                                        // (0-4095)
} PLAYER_RATING;

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

void RatingCalculatorDialog(void);

// void ResetEngineRating (ENGINE_RATING *SR);

void ResetPlayerRating(PLAYER_RATING *PR, INT initPlayerELO);
void UpdatePlayerRating(PLAYER_RATING *PR, BOOL playerWasWhite,
                        REAL playerScore, INT sigmaELO);

INT Score_to_ELO(REAL score);
REAL ELO_to_Score(INT diff);
INT UpdateELO(INT playerELO, INT opponentELO, REAL actualScore);
