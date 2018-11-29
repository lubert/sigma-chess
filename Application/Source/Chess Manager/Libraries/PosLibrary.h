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

#include "CFile.h"
#include "CMemory.h"
#include "General.h"

#include "Board.h"
#include "Game.h"
#include "HashCode.h"
#include "Move.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

//#define __libTest_LoadECO 1
//#define __libTest_AppendV5 1
//#define __libTest_Verify 1

enum LIB_AUTOCLASS {
  libAutoClass_Off = 0,
  libAutoClass_Level = 1,
  libAutoClass_Inherit = 2
};

typedef struct {
  MOVE m;    // The move that lead to this position
  HKEY pos;  // The resulting hash key (can be used for getting ECO & comment)
} LIBVAR;

typedef struct  // Library file (and memory) format:
{
  LONG size;       // Logical size in bytes of library
  LONG wPosCount;  // Number of "raw" uncommented white positions (4 bytes per
                   // pos).
  LONG bPosCount;  // Number of "raw" uncommented black positions (4 bytes per
                   // pos).
  LONG wAuxCount;  // Number of white positions with ECO codes and/or comments
  LONG bAuxCount;  // Number of black positions with ECO codes and/or comments
  BYTE Data[];
} LIBRARY5;

typedef struct {
  LIB_CLASS libClass;  // Classify the imported positions thus
  BOOL overwrite;  // Replace classification of positions already in library?

  BOOL impWhite;         // Import white MOVES
  BOOL impBlack;         // Import black MOVES
  BOOL skipLosersMoves;  // Skip moves played by the player who lost the game

  INT maxMoves;     // Maximum number of moves to replay
  BOOL resolveCap;  // But continue/finish off capture sequences
} LIBIMPORT_PARAM;

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class PosLibrary {
 public:
  PosLibrary(CFile *libFile = nil);
  ~PosLibrary(void);

  LIB_CLASS FindPos(COLOUR player, HKEY pos);
  BOOL FindAux(COLOUR player, HKEY pos, CHAR eco[], CHAR comment[]);

  BOOL ClassifyPos(COLOUR player, HKEY pos, LIB_CLASS libClass,
                   BOOL overwrite = true);
  BOOL AddAux(COLOUR player, HKEY pos, CHAR eco[], CHAR comment[]);
  BOOL DelAux(COLOUR player, HKEY pos);

  void CascadeDelete(CGame *game, BOOL delPos, BOOL delAux);

  INT CalcVariations(CGame *game, LIBVAR Var[]);

  BOOL Load(void);
  BOOL Import(CFile *impFile);
  BOOL Save(void);
  BOOL SaveAs(void);

  void Lib5_Import(LIBRARY5 *Lib5);
  void Lib4_Import(PMOVE *Lib4);

  LIBRARY *lib;
  BOOL dirty;
  CFile *file;

 private:
  BOOL AddPos(COLOUR player, HKEY pos, LIB_CLASS libClass,
              BOOL overwrite = true);
  BOOL DelPos(COLOUR player, HKEY pos);

  BOOL CreateEntry(LONG offset, INT dbytes);
  void DeleteEntry(LONG offset, INT dbytes);

  LONG Lib4_Replay(LONG i, HKEY pos);

  LONG libBytes;  // Actual number of allocated bytes to library (>= lib->size)

  PMOVE *Lib4;  // Version 4.0 import utility
  PIECE Board[boardSize];
};

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

LIB_CLASS PosLib_Probe(COLOUR player, PIECE Board[]);
LIB_CLASS PosLib_ProbePos(COLOUR player, HKEY pos);
BOOL PosLib_ProbePosStr(COLOUR player, HKEY pos, CHAR *eco, CHAR *comment);
BOOL PosLib_ProbeStr(COLOUR player, PIECE Board[], CHAR *eco, CHAR *comment);

BOOL PosLib_Classify(COLOUR player, PIECE Board[], LIB_CLASS libClass,
                     BOOL overwrite = true);
BOOL PosLib_StoreStr(COLOUR player, PIECE Board[], CHAR *eco, CHAR *comment);
INT PosLib_CalcVariations(CGame *game, LIBVAR Var[]);
BOOL Poslib_CascadeDelete(CGame *game, BOOL delPos, BOOL delAux);

BOOL PosLib_Loaded(void);
BOOL PosLib_Dirty(void);
LONG PosLib_Count(void);
BOOL PosLib_Locked(void);

BOOL PosLib_New(void);
void PosLib_Open(CFile *file, BOOL displayPrompt = false);
void PosLib_Save(void);
void PosLib_SaveAs(void);

BOOL PosLib_CheckSave(CHAR *prompt);

LIBRARY *PosLib_Data(void);

void PosLib_AutoLoad(void);

void ResetLibImportParam(LIBIMPORT_PARAM *param);

#ifdef __libTest_Verify
void PosLib_PurifyFlags(void);
#endif
