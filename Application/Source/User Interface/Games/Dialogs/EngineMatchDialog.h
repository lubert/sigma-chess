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

#pragma once

#include "GameWindow.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef struct {
  // Derived Parameters
  GameWindow *gameWin;       // Nil if no engine match currently running
  CollectionWindow *colWin;  // Nil if games are not to be saved

  // Stats
  INT currGameNo;    // Current match game number (1..matchLen)
  BOOL timeForfeit;  // Did current engine just lose on time?

  INT prevScore;     // Previous score
  INT adjWinCount;   // Number of consecutive half moves where both scores where
                     // above limit
  INT adjDrawCount;  // Number of consecutive half moves where both scores where
                     // 0

  // Match result
  INT winCount1;  // Number of games won by engine 1
  INT winCount2;  // Number of games won by engine 2
  INT drawCount;  // Number of drawn games

} ENGINE_MATCH;

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

extern ENGINE_MATCH EngineMatch;

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

BOOL EngineMatchDialog(GameWindow *parent);
void EngineMatch_ResetParam(ENGINE_MATCH_PARAM *Param);
