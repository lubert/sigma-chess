#pragma once

#include "Game.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

void CalcDisambFlags(MOVE *m, MOVE *M, INT moveCount);
void CalcVariationFlags(PIECE Board[], MOVE *MList);
void CalcMoveFlags(PIECE Board[], MOVE *m);

void SetGameNotation(CHAR s[], MOVE_NOTATION notation);

INT CalcMoveStr(MOVE *m, CHAR *s);
INT CalcGameMoveStr(MOVE *m, CHAR *s);
INT CalcGameMoveStrAlge(MOVE *m, CHAR *s, BOOL longNotation,
                        BOOL english = false, BOOL EPsuffix = true);
INT CalcGameMoveStrDesc(MOVE *m, CHAR *s);

void CalcSquareStr(SQUARE sq, CHAR *s);
void CalcInfoResultStr(INT result, CHAR *s);
SQUARE ParseSquareStr(CHAR *s);
void CalcScoreStr(CHAR s[], INT score, INT scoreType = scoreType_True);
BOOL ParseScoreStr(CHAR s[], INT *score);

INT CheckAbsScore(COLOUR player, INT score);
