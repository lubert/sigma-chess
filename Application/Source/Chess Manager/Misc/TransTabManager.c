/**************************************************************************************************/
/*                                                                                                */
/* Module  : TRANSTABMANAGER.C */
/* Purpose : Handles the allocation of transposition tables to each engine
 * instance.              */
/*                                                                                                */
/**************************************************************************************************/

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

#include "TransTabManager.h"

#include "EngineMatchDialog.h"
#include "SigmaApplication.h"
#include "SigmaPrefs.h"

#include "Debug.h"

#define mapTransSize(N) (sizeof(TRANS) * (1L << (N + 12)))

/**************************************************************************************************/
/*                                                                                                */
/*                                     MANAGE TRANSPOSITION TABLES */
/*                                                                                                */
/**************************************************************************************************/

// Since multiple engines can be running at the same time, we have to divide the
// available trans tab block between the engines.

/*----------------------------------------- Data Structures
 * --------------------------------------*/

static PTR metaTransTable = nil;  // The single "meta" transposition table/block
static LONG metaTransTableSize = 0;  // Size (in bytes) of this block

struct {
  TRANS *Tab;
  ULONG size;
  ENGINE *E;                  // Engine currently using this "slot"
} TransAllocTab[maxEngines];  // Current allocation of meta transposition table

/*-------------------------------------- Startup Initialization
 * ----------------------------------*/
// At startup we allocated most of the available memory to the meta
// transposition table. Next we divide - dimensionate - the meta transposition
// table into smaller tables (one per engine), depending on the
// PREFS.maxTransSize setting. This is also done when this setting is changed
// from the prefs dialog.

void TransTab_Init(void) {
  if (metaTransTable) Mem_FreePtr(metaTransTable);

  metaTransTable = nil;
  metaTransTableSize = 0;

  if (!RunningOSX()) {
    ULONG resMemSize =
        Prefs.Memory.reserveMem * 1024L * 1024L + minReserveMem * 1024L;
    if (Mem_FreeBytes() <= resMemSize + trans_MinSize) return;
    metaTransTableSize = MinL(Mem_FreeBytes() - resMemSize, Mem_MaxBlockSize());
  } else {
    metaTransTableSize = Prefs.Trans.totalTransMem * 1024L * 1024L;
  }

  if (metaTransTableSize < trans_MinSize) {
    metaTransTableSize = 0;
    return;
  }

  metaTransTable = Mem_AllocPtr(metaTransTableSize);
  if (!metaTransTable)
    metaTransTableSize = 0;
  else  // Make sure "PREFS.maxTransSize" is NOT larger than
        // "metaTransTableSize":
  {
    ULONG maxTransSize = mapTransSize(Prefs.Trans.maxTransSize);
    while (maxTransSize > metaTransTableSize) {
      maxTransSize >>= 1;
      Prefs.Trans.maxTransSize--;
    }
  }

  TransTab_Dim();
} /* TransTab_Init */

ULONG TransTab_GetSize(void) {
  return (RunningOSX() ? Prefs.Trans.totalTransMem * 1024L * 1024L
                       : metaTransTableSize);
} /* TransTab_GetSize */

/*------------------------------------- Dimensionate Trans Tables
 * --------------------------------*/
// This routine may NOT be called whilst any engines are running. The
// Engine_AbortAll() routine should be called first.

void TransTab_Dim(void) {
  ULONG bytesUsed = 0;
  ULONG bytesLeft = metaTransTableSize;
  ULONG bytesPerTab = mapTransSize(Prefs.Trans.maxTransSize);

  for (INT i = 0; i < maxEngines; i++) {
    TransAllocTab[i].E = nil;

    if (bytesLeft >= trans_MinSize) {
      TransAllocTab[i].Tab = (TRANS *)&metaTransTable[bytesUsed];
      TransAllocTab[i].size = bytesPerTab;
      while (TransAllocTab[i].size > bytesLeft) TransAllocTab[i].size >>= 1;
      bytesUsed += TransAllocTab[i].size;
      bytesLeft -= TransAllocTab[i].size;
    } else {
      TransAllocTab[i].Tab = nil;
      TransAllocTab[i].size = 0;
    }
  }
} /* TransTab_Dim */

/*----------------------------------- Allocate Single Trans Table
 * --------------------------------*/
// Is called when an engine starts searching. The routine looks up an available
// "slot" in the meta transposition table, reserves it and stores a reference to
// it in the engine parameters.

void TransTab_Allocate(ENGINE *E) {
  E->P.TransTables = nil;
  E->P.transSize = 0;
  /*
     if (debugOn)
     {  CHAR s[200];
        Format(s, "TransTab_Allocate[%d] UCI = %d", E->localID, E->UCI);
        DebugWriteNL(s);
     }
  */
  if (!metaTransTable || E->UCI) return;

  // First garbage collect unused entries
  for (INT i = 0; i < maxEngines; i++)
    if (TransAllocTab[i].Tab && TransAllocTab[i].E) {
      INT j;
      for (j = 0; j < maxEngines && Global.Engine[j] != TransAllocTab[i].E; j++)
        ;
      // If we have scanned all entries (and thus j = maxEngines) then entry "i"
      // is unused and can be "released", because TransAllocTab[i].E is not a
      // running engine.
      if (j == maxEngines) TransAllocTab[i].E = nil;
    }

  // Next find a free slot if transposition tables are enabled:
  if (E->P.playingMode == mode_Mate ? Prefs.Trans.useTransTablesMF
                                    : Prefs.Trans.useTransTables) {
    for (INT i = 0; i < maxEngines; i++)
      if (TransAllocTab[i].Tab &&
          (!TransAllocTab[i].E || !TransAllocTab[i].E->R.taskRunning)) {
        E->P.TransTables = TransAllocTab[i].Tab;
        E->P.transSize = TransAllocTab[i].size;
        TransAllocTab[i].E = E;
        return;
      }
  }
} /* TransTab_Allocate */

/*---------------------------------- Deallocate Single Trans Table
 * -------------------------------*/
// Is called when an engine is destroyed. Releases the engine's current grab on
// the transposition tables.

void TransTab_Deallocate(ENGINE *E) {
  for (INT i = 0; i < maxEngines; i++)
    if (TransAllocTab[i].E == E) {
      TransAllocTab[i].E = nil;
      return;
    }
} /* TransTab_Deallocate */

/*----------------------------------------- Auto Intialize
 * ---------------------------------------*/
// If all allocated engines are UCI engines, we deallocate the transposition
// tables. Is called whenever a game window is closed/created, and when the user
// selects an engine

void TransTab_AutoInit(void) {
  if (!RunningOSX() || EngineMatch.gameWin) return;

  if (debugOn) DebugWriteNL("---AUTO ALLOCATE SIGMA TRANSTABLES---");

  BOOL sigmaEngineExists = false;

  for (INT i = 0; i < maxEngines && !sigmaEngineExists; i++)
    if (Global.Engine[i] && !Global.Engine[i]->UCI) sigmaEngineExists = true;

  if (!sigmaEngineExists) {
    if (debugOn) DebugWriteNL("  No Sigma engines selected");
    if (metaTransTable) {
      if (debugOn) DebugWriteNL("  Releasing...");
      Mem_FreePtr(metaTransTable);
      metaTransTable = nil;
      metaTransTableSize = 0;
    }
  } else if (!metaTransTable) {
    if (debugOn) DebugWriteNL("  Sigma engines selected -> Allocating");
    TransTab_Init();
  }
}  // TransTab_AutoInit
