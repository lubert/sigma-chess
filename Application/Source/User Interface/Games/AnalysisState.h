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

#include "Engine.h"
#include "General.h"
#include "UCI_Defs.h"

typedef struct {
  COLOUR initPlayer;
  INT initMoveNo;

  COLOUR player;
  INT gameMove;
  INT numRootMoves;

  CHAR status[50];  // Status text to be shown in stats header

  INT depth;
  INT current;
  MOVE currMove;

  LONG64 nodes;
  ULONG searchTime;
  ULONG nps;
  INT hashFull;

  INT depthPV[uci_MaxMultiPVcount + 1];
  INT score[uci_MaxMultiPVcount + 1];  // Current search score
  INT scoreType[uci_MaxMultiPVcount + 1];
  MOVE PV[uci_MaxMultiPVcount + 1][maxSearchDepth + 1];
} ANALYSIS_STATE;
