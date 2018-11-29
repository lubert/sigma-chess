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

#include "Move.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define libECOLength 7
#define libCommentLength 35
#define libMaxVariations 30

enum LIB_CLASS {
  libClass_Unclassified = 0,

  libClass_Level = 1,
  libClass_Unclear = 2,

  libClass_SlightAdvW = 3,
  libClass_ClearAdvW = 4,
  libClass_WinningAdvW = 5,
  libClass_WithCompW = 6,

  libClass_SlightAdvB = 7,
  libClass_ClearAdvB = 8,
  libClass_WinningAdvB = 9,
  libClass_WithCompB = 10,

  libClass_First = 0,
  libClass_Last = 10,
  libClass_Count = 11
};

enum LIB_SET {
  libSet_None = 0,
  libSet_Solid = 1,       // Chicken & defensive
  libSet_Tournament = 2,  // Normal
  libSet_Wide = 3,        // Aggressive
  libSet_Full = 4         // Desperado
};

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

typedef ULONG HKEY;

typedef struct {
  ULONG randKey;

  HKEY HashCode[pieces][128];  // Hash codes for each piece on each square. Used
                               // for incremental update of "hashKey".
  HKEY o_oHashCodeB;           // Hash key changes caused by castling moves.
  HKEY o_oHashCodeW;           // NOTE: "UpdateDrawState" depends on word order.
  HKEY o_o_oHashCodeB;
  HKEY o_o_oHashCodeW;
} HASHCODE_COMMON;

/*-------------------------------------- Position Library Types
 * ----------------------------------*/

typedef struct  // Library file (and memory) format:
{
  CHAR info[1024];
  ULONG flags;      // For future use
  LONG unused[32];  // For future use

  LONG size;       // Logical size in bytes of library
  LONG wPosCount;  // Number of "raw" uncommented white positions.
  LONG bPosCount;  // Number of "raw" uncommented black positions.
  LONG wAuxCount;  // Number of white positions with ECO codes and/or comments
  LONG bAuxCount;  // Number of black positions with ECO codes and/or comments

  BYTE Data[];  // The LIBRARY.Data block is organized as follows:
                // LIB_POS[wPosCount];     8 bytes per entry (4 bytes in Sigma
                // 5) LIB_POS[bPosCount];     8 bytes per entry (4 bytes in
                // Sigma 5) LIB_AUX[wAuxCount];    48 bytes per entry
                // LIB_AUX[bAuxCount];    48 bytes per entry
} LIBRARY;

typedef struct  // 8 bytes
{
  HKEY pos;
  ULONG flags;  // E.g. open key classification (bits 0..3)
} LIB_POS;

typedef struct  // 48 bytes
{
  HKEY pos;
  CHAR eco[libECOLength + 1];
  CHAR comment[libCommentLength + 1];
} LIB_AUX;
