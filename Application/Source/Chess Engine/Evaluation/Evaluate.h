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

#include "Board.h"
#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define kpkDataSize (24 * 1024)

#define queenMob 1
#define rookMob 2
#define bishopMob 3

/**************************************************************************************************/
/*                                                                                                */
/*                                        TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

// Central pawns are penalized more heavily: 7/8 for a, h; 8/8 for b, g; 9/8 for
// c, d, e and f.

typedef struct {
  BYTE IsoBackVal[256],  // Punishment of isolated/backward pawns on closed
                         // files.
      IsoBackVal_[256],  // Extra punishment for semi open files.
      IsoVal[256],       // Extra punishment of isolated pawns.
      DobVal[256],       // Punishment of doubled pawns.
      DobIsoVal[256],    // Extra punishment for isolated and doubled pawns.
      PassedVal[256];    // Bonus for passed pawns.
} EVAL_STATE;

typedef struct {
  RANKBITS BlkPawnsN[256],  // Utility bit tables.
      IsoPawns[256];
  BYTE RuleOfSquareTab_[0x77], RuleOfSquareTab[0x78];
  BYTE kpkData[kpkDataSize];
} EVAL_COMMON;
