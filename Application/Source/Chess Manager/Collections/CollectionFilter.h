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
#include "HashCode.h"
#include "Move.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

// The filter structure defines a filter

enum FILTER_FIELDS {
  filterField_WhiteOrBlack = 1,
  filterField_White,
  filterField_Black,
  filterField_Event,
  filterField_Site,
  filterField_Date,
  filterField_Round,
  filterField_Result,
  filterField_ECO,
  filterField_Annotator,
  filterField_WhileELO,
  filterField_BlackELO,
  filterField_OpeningLine,  // Max 1 per filter
  filterField_Position      // Max 1 per filter
};

enum FILTER_CONDS {
  filterCond_Is = 1,  // Generic conditions
  filterCond_IsNot,
  filterCond_StartsWith,  // String conditions
  filterCond_EndsWith,
  filterCond_Contains,
  filterCond_Less,  // Number conditions
  filterCond_Greater,
  filterCond_LessEq,
  filterCond_GreaterEq,
  filterCond_Before,  // Date conditions
  filterCond_After,
  filterCond_Matches  // Position filter only
};

enum POS_FILTER_CONSTANTS {
  posFilter_wAny = wKing + 1,
  posFilter_bAny = bKing + 1,
  posFilter_Any = -1,
  posFilter_AllMoves = 1000
};

#define filterValueLen 30
#define maxFilterCond 8
#define maxFilterLineLen 20

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef struct {
  BOOL exactMatch;  // Exact or partial?

  INT sideToMove;  // white(0), black(0x10), any(-1)
  INT Pos[0x88];   // empty(0), wPawn...wKing,wAny(7), bPawn...bKing,bAny(0x17),
                   // any(-1)

  BOOL checkMoveRange;  // Check move range?
  INT minMove,
      maxMove;  // Move range in which to search for pos (both inclusive)

  INT wCountMin, wCountMax;  // Number of white pieces (if partial match)
  INT bCountMin, bCountMax;  // Number of white pieces (if partial match)

  // Utility data computed from the above. Is used to speed up the filtering
  // process
  HKEY hkey;  // Hash key for position (if exact match)
  INT wCountTotal,
      wCountPawns;  // Number of white pieces/pawns in position (if exact match)
  INT bCountTotal,
      bCountPawns;  // Number of white pieces/pawns in position (if exact match)

  INT Unused[128];  // Reserved for future use
} POS_FILTER;

typedef struct {
  INT count;
  INT Field[maxFilterCond];
  INT Cond[maxFilterCond];
  CHAR Value[maxFilterCond][filterValueLen + 1];

  // Opening line filter component. Is set if Field[i] ==
  // filterField_OpeningLine (in which case Value[i] is ignored).
  BOOL useLineFilter;
  INT lineLength;
  MOVE Line[maxFilterLineLen + 1];

  // Position filter component. Is set if Field[i] == filterField_Position (in
  // which case Cond[i] and Value[i] are ignored).
  BOOL usePosFilter;
  POS_FILTER posFilter;
} FILTER;

/**************************************************************************************************/
/*                                                                                                */
/*                                        FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

void ResetFilter(FILTER *filter);
void ResetPosFilter(POS_FILTER *pf);
void PreparePosFilter(POS_FILTER *pf);
