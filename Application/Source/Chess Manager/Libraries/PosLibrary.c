/**************************************************************************************************/
/*                                                                                                */
/* Module  : PosLibrary.c */
/* Purpose : This module implements the position libraries. */
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

#include "PosLibrary.h"
#include "CDialog.h"
#include "Game.h"
#include "SigmaApplication.h"

#include "Board.f"
#include "HashCode.f"
#include "Move.f"

/*
A bit like Word dictionaries!!! When the user wants to add a position to the
opening library, it is per default added to the custom user library (maybe
define default library dictionary). This method can be used for "adhoc" editing.

Store ALL libraries in the same folder (e.g. "Libraries" just like the
"Endgame Databases" folder). Requires dir scan... (later in version 6.0)

The positions in a library are per default "good" for the opponent (i.e. the
player who just played the move). Maybe included a simple evaluation from
opponent's point of view (0 = ?!, 1 = !?, 2 = !, 3 = !!) (later in version 6.0)

The user can then select which libraries Sigma Chess should look through
(store lib names in prefs file. user can add new libraries using StdOpen dialog
by clicking "Add..." button in "Library Selector" dialog. Include "Delete..."
and "Edit..." (same as "File/Open...") buttons too!!!. (later in version 6.0)

From a game window it should be possible to add the current position by
launching a "Library Editor" window.

Maybe the lib editor could just be a special mode for a normal game window,
where a library was attached (exclusively!!). Then each position would
automatically be added to the library (bad moves?? Either mark position as bad,
or do not include at all).

For each position, all moves that lead to a position which is also stored in the
library should be shown in the "replies" list.

Entries are retrieved using binary search on the hkey.

------
Position Specific Annotation Symbols (icon ids 370...379):

x     Unclassified/Unplayable (remove from library)

=     Level position (implicit value for lib positions with no explicit value)
°     Unclear position (once player material behind but possible compensated
positionally)

+/=   Slight advantage for White
+/-   Clear advantage for White
++/-- Winning advantage for White
°/=   With compensation for White (i.e. behind materially, but positionally
compensated)

=/+   Slight advantage for Black
-/+   Clear advantage for Black
--/++ Winning advantage for Black
=/°   With compensation for Black (i.e. behind materially, but positionally
compensated)

Move Specific Annotation Symbols:

!
!!
!?
?!
?
??
*/

PosLibrary *posLib = nil;   // Currently loaded position library
BOOL posLibEditor = false;  // Is library editor currently open?

static BOOL VerifyLibInvar(LIBRARY *lib);
static BOOL VerifyLibInvar5(LIBRARY5 *lib);

#ifdef __libTest_LoadECO
static void PosLib_LoadECOTxt(void);
#endif

/**************************************************************************************************/
/*                                                                                                */
/*                                              API */
/*                                                                                                */
/**************************************************************************************************/

// At any time at most one opening library is loaded. The engine and the library
// editor can access the positions in this library using the following API:

/*----------------------------------- Probe Position Library
 * -------------------------------------*/

LIB_CLASS PosLib_Probe(COLOUR player, PIECE Board[]) {
  if (!posLib) return libClass_Unclassified;

  return posLib->FindPos(player, CalcHashKey(&Global, Board));
} /* PosLib_Probe */

LIB_CLASS PosLib_ProbePos(COLOUR player, HKEY pos) {
  if (!posLib) return libClass_Unclassified;

  return posLib->FindPos(player, pos);
} /* PosLib_ProbePos */

BOOL PosLib_ProbeStr(COLOUR player, PIECE Board[], CHAR *eco, CHAR *comment) {
  return PosLib_ProbePosStr(player, CalcHashKey(&Global, Board), eco, comment);
} /* PosLib_ProbeStr */

BOOL PosLib_ProbePosStr(COLOUR player, HKEY pos, CHAR *eco, CHAR *comment) {
  CopyStr("", eco);
  CopyStr("", comment);

  if (!posLib) return false;
  return posLib->FindAux(player, pos, eco, comment);
} /* PosLib_ProbePosStr */

/*---------------------------------- Update Position Library
 * -------------------------------------*/

BOOL PosLib_Classify(COLOUR player, PIECE Board[], LIB_CLASS libClass,
                     BOOL overwrite) {
  if (!posLib) return false;

  return posLib->ClassifyPos(player, CalcHashKey(&Global, Board), libClass,
                             overwrite);
} /* PosLib_Classify */

BOOL PosLib_StoreStr(COLOUR player, PIECE Board[], CHAR *eco, CHAR *comment) {
  if (!posLib) return false;

  HKEY pos = CalcHashKey(&Global, Board);
  posLib->DelAux(player, pos);
  if (!EqualStr(eco, "") || !EqualStr(comment, ""))
    return posLib->AddAux(player, pos, eco, comment);
  else
    return true;
} /* PosLib_StoreStr */

/*---------------------------------------------- Misc
 * --------------------------------------------*/

INT PosLib_CalcVariations(CGame *game, LIBVAR Var[]) {
  if (!posLib) return 0;

  return posLib->CalcVariations(game, Var);
} /* PosLib_CalcVariations */

BOOL PosLib_Loaded(void) { return (posLib != nil); } /* PosLib_Loaded */

BOOL PosLib_Dirty(void) {
  return (posLib != nil && posLib->dirty);
} /* PosLib_Dirty */

LONG PosLib_Count(void) {
  if (!posLib) return 0;
  return posLib->lib->wPosCount + posLib->lib->bPosCount;
} /* PosLib_Count */

BOOL PosLib_Locked(void) {
  if (!posLib || !posLib->file->IsLocked()) return false;
  return true;
} /* PosLib_Locked */

BOOL PosLib_New(void) {
  if (!ProVersionDialog(nil,
                        "You cannot create new libraries in Sigma Chess Lite."))
    return false;

  if (posLibEditor || !PosLib_CheckSave("Save before creating new library?"))
    return false;

  PosLibrary *newPosLib = new PosLibrary();
  if (!newPosLib->SaveAs()) {
    delete newPosLib;
    return false;
  } else {
    if (posLib) delete posLib;
    posLib = newPosLib;
#ifdef __libTest_LoadECO
    PosLib_LoadECOTxt();
#endif
    sigmaApp->BroadcastMessage(msg_RefreshPosLib);
  }

  sigmaPrefs->SetLibraryName(posLib->file->name);
  sigmaPrefs->EnableLibrary(true);

  NoteDialog(nil, "Library Created",
             "The new empty library has been created (and enabled)...");
  return true;
} /* PosLib_New */

void PosLib_Save(void) {
  if (posLib) posLib->Save();
} /* PosLib_Save */

void PosLib_SaveAs(void) {
  if (posLib) posLib->SaveAs();
} /* PosLib_SaveAs */

LIBRARY *PosLib_Data(void) {
  return (posLib != nil ? posLib->lib : nil);
} /* PosLib_Data */

#ifdef __libTest_Verify

void PosLib_PurifyFlags(void) {
  if (!posLib) return;

  LIB_POS *Data = (LIB_POS *)(posLib->lib->Data);
  ULONG posCount = posLib->lib->wPosCount + posLib->lib->bPosCount;
  for (ULONG i = 0; i < posCount; i++) Data[i].flags &= 0x000F;
} /* PosLib_PurifyFlags */

#endif

/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

#define libBlockAllocSize (4 * 1024L)
#define libPosData(p) \
  (LIB_POS *)(lib->Data + (p == black ? sizeof(LIB_POS) * lib->wPosCount : 0))
#define libAuxData(p)                                               \
  (LIB_AUX *)(lib->Data +                                           \
              sizeof(LIB_POS) * (lib->wPosCount + lib->bPosCount) + \
              (p == black ? sizeof(LIB_AUX) * lib->wAuxCount : 0))

PosLibrary::PosLibrary(CFile *libFile) {
  lib = nil;
  libBytes = 0;
  file = libFile;
  dirty = false;

  if (file) {
    if (file->Load((ULONG *)&libBytes, (PTR *)(&lib)) != fileError_NoError) {
      delete file;
      file = nil;
    }
  }

  if (!file) {
    libBytes = libBlockAllocSize;
    lib = (LIBRARY *)Mem_AllocPtr(libBytes);
    if (!lib) return;

    lib->info[0] = 0;
    lib->flags = 0;
    for (INT i = 0; i < 32; i++) lib->unused[i] = 0;

    lib->size = sizeof(LIBRARY);
    lib->wPosCount = 0;
    lib->bPosCount = 0;
    lib->wAuxCount = 0;
    lib->bAuxCount = 0;
  }

  // VerifyLibInvar(lib);  //###
} /* PosLibrary::PosLibrary */

PosLibrary::~PosLibrary(void) {
  if (file) delete file;
  if (lib) Mem_FreePtr((PTR)lib);
} /* PosLibrary::~PosLibrary */

/**************************************************************************************************/
/*                                                                                                */
/*                                          FIND POSITION */
/*                                                                                                */
/**************************************************************************************************/

// "FindPos()" looks up the specified position in the library and returns the
// classification for the position (if found; if not found "unclassified" is
// returned)

LIB_CLASS PosLibrary::FindPos(COLOUR player, HKEY pos) {
  return ProbePosLib(lib, player, pos);  // Call routine in "HashCode.c"
} /* PosLibrary::FindPos */

BOOL PosLibrary::FindAux(COLOUR player, HKEY pos, CHAR eco[], CHAR comment[]) {
  LIB_AUX *Data = libAuxData(player);

  LONG count = (player == white ? lib->wAuxCount : lib->bAuxCount);
  LONG min = 0;
  LONG max = count - 1;

  while (min <= max) {
    LONG i = (min + max) / 2;
    if (pos < Data[i].pos)
      max = i - 1;
    else if (pos > Data[i].pos)
      min = i + 1;
    else {
      CopyStr(Data[i].eco, eco);
      CopyStr(Data[i].comment, comment);
      return true;
    }
  }

  return false;
} /* PosLibrary::FindAux */

// Fills the LibMoves[] table with all library moves in the current position
// in the specified "game".

INT PosLibrary::CalcVariations(CGame *game, LIBVAR Var[]) {
  HKEY pos = game->DrawData[game->currMove].hashKey;
  INT n = 0;

  for (INT i = 0; i < game->moveCount && n < libMaxVariations; i++) {
    HKEY newPos = pos ^ HashKeyChange(&Global, &game->Moves[i]);
    if (FindPos(game->opponent, newPos)) {
      Var[n].m = game->Moves[i];
      Var[n].pos = newPos;
      n++;
    }
  }

  return n;
} /* PosLibrary::CalcVariations */

/**************************************************************************************************/
/*                                                                                                */
/*                                         ADD/DELETE POSITIONS */
/*                                                                                                */
/**************************************************************************************************/

BOOL PosLibrary::ClassifyPos(COLOUR player, HKEY pos, LIB_CLASS libClass,
                             BOOL overwrite) {
  if (libClass == libClass_Unclassified)
    return DelPos(player, pos);
  else
    return AddPos(player, pos, libClass, overwrite);
} /* PosLibrary::ClassifyPos */

/*------------------------------------ Add/Delete Normal Positions
 * -------------------------------*/

BOOL PosLibrary::AddPos(COLOUR player, HKEY pos, LIB_CLASS libClass,
                        BOOL overwrite) {
  LIB_POS *Data = libPosData(player);

  LONG count = (player == white ? lib->wPosCount : lib->bPosCount);
  LONG min = 0;
  LONG max = count;

  while (min < max) {
    LONG i = (min + max) / 2;
    if (pos < Data[i].pos)
      max = i;
    else if (pos > Data[i].pos)
      min = i + 1;
    else {
      if (Data[i].flags != libClass &&
          overwrite)  // If we get here, the pos already exists
      {
        Data[i].flags = libClass;  // in the library -> Update
        dirty = true;
      }
      return false;
    }
  }

  if (!CreateEntry(((PTR)(&Data[min])) - lib->Data, sizeof(LIB_POS)))
    return false;

  Data = libPosData(
      player);  // <--- Important! Because "CreateEntry" may move memory!
  Data[min].pos = pos;
  Data[min].flags = libClass;
  if (player == white)
    lib->wPosCount++;
  else
    lib->bPosCount++;
  dirty = true;

  // VerifyLibInvar(lib);  //###
  return true;
} /* PosLibrary::AddPos */

BOOL PosLibrary::DelPos(COLOUR player, HKEY pos) {
  LIB_POS *Data = libPosData(player);

  LONG count = (player == white ? lib->wPosCount : lib->bPosCount);
  LONG min = 0;
  LONG max = count - 1;

  while (min <= max) {
    LONG i = (min + max) / 2;
    if (pos < Data[i].pos)
      max = i - 1;
    else if (pos > Data[i].pos)
      min = i + 1;
    else {
      DeleteEntry(((PTR)(&Data[i])) - lib->Data, sizeof(LIB_POS));
      if (player == white)
        lib->wPosCount--;
      else
        lib->bPosCount--;
      dirty = true;
      //       VerifyLibInvar(lib);  //###
      return true;
    }
  }

  return false;
} /* PosLibrary::DelPos */

/*------------------------------------ Add/Delete Auxiliary Info
 * ---------------------------------*/

BOOL PosLibrary::AddAux(COLOUR player, HKEY pos, CHAR eco[], CHAR comment[]) {
  LIB_AUX *Data = libAuxData(player);

  LONG count = (player == white ? lib->wAuxCount : lib->bAuxCount);
  LONG min = 0;
  LONG max = count;

  while (min < max) {
    LONG i = (min + max) / 2;
    if (pos < Data[i].pos)
      max = i;
    else if (pos > Data[i].pos)
      min = i + 1;
    else {
      min = i;
      goto storeAux;
    }
  }

  if (!CreateEntry(((PTR)(&Data[min])) - lib->Data, sizeof(LIB_AUX)))
    return false;
  Data = libAuxData(
      player);  // <--- Important! Because "CreateEntry" may move memory!

storeAux:
  Data[min].pos = pos;
  CopyStr(eco, Data[min].eco);
  CopyStr(comment, Data[min].comment);
  if (player == white)
    lib->wAuxCount++;
  else
    lib->bAuxCount++;
  dirty = true;
  // VerifyLibInvar(lib);  //###
  return true;
} /* PosLibrary::AddAux */

BOOL PosLibrary::DelAux(COLOUR player, HKEY pos) {
  LIB_AUX *Data = libAuxData(player);

  LONG count = (player == white ? lib->wAuxCount : lib->bAuxCount);
  LONG min = 0;
  LONG max = count - 1;

  while (min <= max) {
    LONG i = (min + max) / 2;
    if (pos < Data[i].pos)
      max = i - 1;
    else if (pos > Data[i].pos)
      min = i + 1;
    else {
      DeleteEntry(((PTR)(&Data[i])) - lib->Data, sizeof(LIB_AUX));
      if (player == white)
        lib->wAuxCount--;
      else
        lib->bAuxCount--;
      dirty = true;
      //       VerifyLibInvar(lib);  //###
      return true;
    }
  }

  return false;
} /* PosLibrary::DelAux */

/*--------------------------------------- Low Level Routines
 * -------------------------------------*/

BOOL PosLibrary::CreateEntry(LONG offset, INT dbytes) {
  // First check if enough room to add
  if (lib->size + dbytes > libBytes)  // If not enough room
  {
    LONG newLibBytes = libBytes + libBlockAllocSize;

    if (!Mem_SetPtrSize((PTR)lib, newLibBytes)) {
      LIBRARY *newLib = (LIBRARY *)Mem_AllocPtr(newLibBytes);
      if (!newLib) return false;
      Mem_Move((PTR)lib, (PTR)newLib, lib->size);
      Mem_FreePtr(lib);
      lib = newLib;
    }

    libBytes = newLibBytes;
  }

  // Then create room for entry by moving trailing entries:
  LONG dataSize = lib->size - sizeof(LIBRARY);
  for (LONG i = dataSize - 1; i >= offset; i--)
    lib->Data[i + dbytes] = lib->Data[i];
  lib->size += dbytes;
  return true;
} /* PosLibrary::CreateEntry */

void PosLibrary::DeleteEntry(LONG offset, INT dbytes) {
  LONG dataSize = lib->size - sizeof(LIBRARY);
  for (LONG i = offset + dbytes; i < dataSize; i++)
    lib->Data[i - dbytes] = lib->Data[i];
  lib->size -= dbytes;
} /* PosLibrary::DeleteEntry */

/*----------------------------------------------- Test
 * -------------------------------------------*/

static BOOL VerifyLibInvar(LIBRARY *lib) {
  /*
     ULONG size = sizeof(LIBRARY) +
                  sizeof(LIB_POS)*(lib->wPosCount + lib->bPosCount) +
                  sizeof(LIB_AUX)*(lib->wAuxCount + lib->bAuxCount);

     if (lib->size != size)
     {
        CHAR t[1000];
        Beep(1);
        Format(t, "Inconsistent size/position count info: %ld %ld\r%ld %ld %ld
     %ld", size, lib->size, lib->wPosCount, lib->bPosCount, lib->wAuxCount,
     lib->bAuxCount); NoteDialog(nil, "Library Error", t); return false;
     }
  */
  return true;
} /* VerifyLibInvar */

static BOOL VerifyLibInvar5(LIBRARY5 *lib) {
  /*
     ULONG size = sizeof(LIBRARY5) +
                  sizeof(HKEY)*(lib->wPosCount + lib->bPosCount) +
                  sizeof(LIB_AUX)*(lib->wAuxCount + lib->bAuxCount);

     if (lib->size != size)
     {
        CHAR t[1000];
        Beep(1);
        Format(t, "Inconsistent size/position count info v5: %ld %ld\r%ld %ld
     %ld %ld", size, lib->size, lib->wPosCount, lib->bPosCount, lib->wAuxCount,
     lib->bAuxCount); NoteDialog(nil, "Library Error", t); return false;
     }
  */
  return true;
} /* VerifyLibInvar5 */

/**************************************************************************************************/
/*                                                                                                */
/*                                         CASCADE DELETE */
/*                                                                                                */
/**************************************************************************************************/

static CProgressDialog *progDlg = nil;
static ULONG delCount = 0;

BOOL Poslib_CascadeDelete(CGame *game, BOOL delPos, BOOL delAux) {
  if (!posLib || (!delPos && !delAux)) return false;

  progDlg = ProgressDialog_Open(
      nil, "Deleting Variations",
      "Deleting all library variations reachable from the current position...",
      PosLib_Count(), true);
  sigmaApp->responsive = false;
  delCount = 0;
  theApp->ProcessEvents();

  CGame *utilGame = new CGame();
  utilGame->CopyFrom(game, false, false, false);
  posLib->CascadeDelete(utilGame, delPos, delAux);
  delete utilGame;

  delete progDlg;
  progDlg = nil;
  sigmaApp->responsive = true;

  return true;
} /* Poslib_CascadeDelete */

void PosLibrary::CascadeDelete(CGame *game, BOOL delPos, BOOL delAux) {
  LIBVAR Var[libMaxVariations];
  INT varCount;

  varCount = CalcVariations(game, Var);

  for (INT i = 0; i < varCount && !progDlg->Aborted(); i++) {
    game->PlayMove(&Var[i].m);
    if (delPos) DelPos(game->player, Var[i].pos);
    if (delAux) DelAux(game->player, Var[i].pos);

    CHAR status[100];
    Format(status, "%ld positions deleted", ++delCount);
    if (delCount % 10 == 0) progDlg->Set(delCount, status);

    CascadeDelete(game, delPos, delAux);
    game->UndoMove(true);
  }
} /* PosLibrary::CascadeDelete */

/**************************************************************************************************/
/*                                                                                                */
/*                                         SAVE/LOAD TO/FROM FILE */
/*                                                                                                */
/**************************************************************************************************/

BOOL PosLibrary::Save(void) {
  if (!file)
    return SaveAs();
  else {
    //    if (! VerifyLibInvar(lib)) return true;  //###
    file->Save(lib->size, (PTR)lib);
    dirty = false;
    return true;
  }
} /* PosLibrary::Save */

BOOL PosLibrary::SaveAs(void) {
  CFile *newFile = new CFile();

  if (!newFile->SaveDialog("Save Library", "Untitled")) {
    delete newFile;
    return false;
  } else {
    if (file) delete file;
    file = newFile;

    if (file->saveReplace) file->Delete();
    file->SetCreator(sigmaCreator);
    file->SetType('·LB6');
    file->Create();
    file->Save(lib->size, (PTR)lib);
    dirty = false;
    sigmaPrefs->SetLibraryName(file->name);

    return true;
  }
} /* PosLibrary::SaveAs */

/**************************************************************************************************/
/*                                                                                                */
/*                                         VERSION 5 IMPORT */
/*                                                                                                */
/**************************************************************************************************/

// The only difference between the version 5 and 6 format is that in the latter
// all the "raw" entries are twice as big: A 4 byte HKEY + a 4 byte flags field
// (where bits 0..3 holds the classification key).

void PosLibrary::Lib5_Import(LIBRARY5 *lib5) {
  // VerifyLibInvar5(lib5);

  //--- First compute new logical size of version 6 library ---
  // ULONG size6 = sizeof(LIBRARY) +
  //               sizeof(LIB_POS)*(lib5->wPosCount + lib5->bPosCount) +
  //               sizeof(LIB_AUX)*(lib5->wAuxCount + lib5->bAuxCount);
  ULONG size6 = lib5->size + sizeof(LIBRARY) - sizeof(LIBRARY5) +
                sizeof(ULONG) * (lib5->wPosCount + lib5->bPosCount);

  //--- Then allocate physical block ---
  ULONG libBytes6 = size6 + libBlockAllocSize;
  LIBRARY *lib6 = (LIBRARY *)Mem_AllocPtr(libBytes6);
  if (!lib6) {
    sigmaApp->MemErrorDialog();
    return;
  }

  if (lib) Mem_FreePtr(lib);
  libBytes = libBytes6;
  lib = lib6;

  //--- Initialize size, and position count stats ---
  lib->size = size6;
  lib->info[0] = 0;
  lib->flags = 0;
  for (INT i = 0; i < 32; i++) lib->unused[i] = 0;

  lib->wPosCount = lib5->wPosCount;
  lib->bPosCount = lib5->bPosCount;
  lib->wAuxCount = lib5->wAuxCount;
  lib->bAuxCount = lib5->bAuxCount;

  //--- Finally copy the library ---
  ULONG *src = (ULONG *)(lib5->Data);
  ULONG *dst = (ULONG *)(lib->Data);

  // Copy white & black positions
  for (LONG i = 0; i < lib->wPosCount + lib->bPosCount; i++) {
    *(dst++) = *(src++);
    *(dst++) = libClass_Level;
  }

  // Copy white & black aux info
  ULONG currSize = (ULONG)dst - (ULONG)lib;  // Number of bytes written so far
  for (LONG i = 0; currSize < size6; i++) {
    *(dst++) = *(src++);
    currSize += 4;
  }

  while (size6 < sizeof(LIBRARY) +
                     sizeof(LIB_POS) * (lib->wPosCount + lib->bPosCount) +
                     sizeof(LIB_AUX) * (lib->wAuxCount + lib->bAuxCount)) {
    lib->bAuxCount--;
  }

  /*
     for (LONG i = 0; i < ((lib->wAuxCount +
     lib->bAuxCount)*sizeof(LIB_AUX))/sizeof(ULONG); i++) {  *(dst++) =
     *(src++);
     }
  */

  // VerifyLibInvar(lib);  //###
} /* PosLibrary::Lib5_Import */

#ifdef __libTest_AppendV5

static void Lib5_Append(CFile *file);  // Only for internal test usage

static void Lib5_Append(
    CFile *file)  // Simply appends all LIB positions to current lib
{
  ULONG bytes;
  PTR data;

  if (file->Load(&bytes, &data) == fileError_NoError && data) {
    LIBRARY5 *lib5 = (LIBRARY5 *)data;
    HKEY *Pos = (HKEY *)lib5->Data;

    for (INT i = 0; i < lib5->wPosCount; i++)
      posLib->ClassifyPos(white, *(Pos++), libClass_Level, false);
    for (INT i = 0; i < lib5->bPosCount; i++)
      posLib->ClassifyPos(black, *(Pos++), libClass_Level, false);

    Mem_FreePtr(data);
  }
} /* Lib5_Append */

#endif

/**************************************************************************************************/
/*                                                                                                */
/*                                         VERSION 4 IMPORT */
/*                                                                                                */
/**************************************************************************************************/

// The import process takes place by traversing the version 4 library as a
// "search tree" (depth first).

/*------------------------------------------- Constants
 * ------------------------------------------*/

#define siblingBit 0x8000
#define childlessBit 0x0800
#define unplayBit 0x0080
#define xDataBit 0x0008
#define hasSibling(i) (Lib4[i] & siblingBit)
#define childless(i) (Lib4[i] & childlessBit)
#define unplayable(i) (Lib4[i] & unplayBit)
#define hasXData(i) (Lib4[i] & xDataBit)
#define moveMask 0x7777

// LIBXDATA type selectors (currently only one):
#define libXDataStr 0

#define libCommentLen 40
#define maxSiblings 15

#define nextLibMove(i) (hasXData(i) ? i + 1 + (Lib4[i + 1] >> 8) : i + 1)

/*-------------------------------------------- Types
 * ---------------------------------------------*/
/*
typedef UINT LIBMOVE;            // 16 bit integer

typedef struct                   // Additional library move data inserted after
lib moves {                                // with the "xdata"-bit set. BYTE
length;                // Total length of extra data struct in words. BYTE type;
// I.e. libXDataStr BYTE   data[];                // The actual data[0..n-1], n
even. Which in the case of
                                 // libxDataStr contains a Pascal string. The
last two bytes
                                 // are reserved for moving backwards through
the library:
                                 // data[n-1] = xDataBit and data[n-2] = length.
The size of
                                 // of this struct is thus always at least 6
bytes. } LIBXDATA;
*/
/*------------------------------------ Replay Version 4 "Tree"
 * -----------------------------------*/

void PosLibrary::Lib4_Import(PMOVE *theLib4) {
  Lib4 = theLib4;
  for (SQUARE sq = 0; sq < boardSize; sq++) Board[sq] = edge;
  NewBoard(Board);

  sigmaApp->SetCursor(cursor_Watch);
  Lib4_Replay(0, CalcHashKey(&Global, Board));
  sigmaApp->SetCursor();

  dirty = true;
} /* PosLibrary::Lib4_Import */

LONG PosLibrary::Lib4_Replay(LONG i, HKEY pos) {
  LONG i0, i1;

  do {
    sigmaApp->SpinCursor();

    MOVE m;

    Move_Unpack(Lib4[i], Board, &m);
    Move_Perform(Board, &m);

    HKEY newPos = (pos ^ HashKeyChange(&Global, &m));

    if (!unplayable(i))
      AddPos(black - pieceColour(m.piece), newPos, libClass_Level);

    if (hasXData(i)) {
      CHAR *s = (CHAR *)(&Lib4[i + 1]) + 2;
      CHAR eco[libECOLength + 1];
      eco[0] = 0;
      s[1 + (INT)s[0]] = 0;
      s++;

      if (s[0] == 'E' && s[1] == 'C' && s[2] == 'O' && s[3] == '/') {
        s += 4;
        INT k;
        for (k = 0; k < libECOLength && IsAlphaNum(*s); k++) eco[k] = *(s++);
        eco[k] = 0;
        while (*s == ' ' || *s == '-') s++;
      }

      AddAux(black - pieceColour(m.piece), newPos, eco, s);
    }

    i1 = nextLibMove(i);
    i0 = i;
    i = (childless(i0) ? i1 : Lib4_Replay(i1, newPos));

    Move_Retract(Board, &m);

  } while (hasSibling(i0));

  return i;
} /* PosLibrary::Lib4_Replay */

/**************************************************************************************************/
/*                                                                                                */
/*                                       LOAD/IMPORT LIBRARY */
/*                                                                                                */
/**************************************************************************************************/

// At startup, the default opening library is opened. If the user subsequently
// opens another library this will be the default library instead. The user can
// also decide to merge/import another library into the default library.

void PosLib_Open(CFile *file, BOOL displayPrompt) {
  if (posLibEditor || !PosLib_CheckSave("Save before opening new library?"))
    return;

  switch (file->fileType) {
    case '·LB6':
      if (posLib) delete posLib;
      posLib = new PosLibrary(file);
      sigmaApp->BroadcastMessage(msg_RefreshPosLib);
      break;

    case '·LB5':
#ifdef __libTest_AppendV5
      NoteDialog(nil, "Append Test", "");
      Lib5_Append(file);
      break;
#endif
    case '·LBX':
    case '·LIB':
      NoteDialog(
          nil, "Note",
          "This library was created with an earlier version of Sigma Chess and "
          "must be converted to the new Sigma Chess 6 format ...");

      PosLibrary *newPosLib = new PosLibrary();

      if (!newPosLib->SaveAs()) {
        delete newPosLib;
        return;
      } else {
        ULONG bytes;
        PTR data;

        if (file->Load(&bytes, &data) == fileError_NoError && data) {
          if (file->fileType == '·LB5')
            newPosLib->Lib5_Import((LIBRARY5 *)data);
          else
            newPosLib->Lib4_Import((PMOVE *)data);

          Mem_FreePtr(data);
          newPosLib->Save();
          if (posLib) delete posLib;
          posLib = newPosLib;
          sigmaApp->BroadcastMessage(msg_RefreshPosLib);
          NoteDialog(nil, "Library Converted",
                     "The library has been converted to the new Sigma Chess 6 "
                     "format...");
        } else {
          delete newPosLib;
          NoteDialog(nil, "Error", "Failing loading the library...",
                     cdialogIcon_Error);
          return;
        }
      }
      break;

    default:
      return;  // Shouldn't happen
  }

  sigmaPrefs->SetLibraryName(posLib->file->name);
  sigmaPrefs->EnableLibrary(true);
} /* PosLib_Open */

BOOL PosLib_CheckSave(CHAR *prompt) {
  if (!posLib || !posLib->dirty || !ProVersion()) return true;

  CRect frame(0, 0, 320, 100);
  if (RunningOSX()) frame.right += 20, frame.bottom += 15;
  sigmaApp->CentralizeRect(&frame);

  CHAR name[maxFileNameLen + 1];
  ::CopyStr((posLib->file ? posLib->file->name : "Untitled"), name);

  CHAR message[400];
  Format(message,
         "Changes to the position library Ò%sÓ have not been saved. %s", name,
         prompt);

  CConfirmDialog *dialog =
      new CConfirmDialog(nil, "Save Library?", frame, message, 1002, "Save",
                         "Cancel", "Don't Save");
  dialog->Run();
  CDIALOG_REPLY reply = dialog->reply;
  delete dialog;

  switch (reply) {
    case cdialogReply_OK:
      return posLib->Save();
    case cdialogReply_No:
      return true;
    default:
      return false;
  }
} /* PosLib_CheckSave */

/**************************************************************************************************/
/*                                                                                                */
/*                                          TEST ROUTINES */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- Load ECO Text File
 * -------------------------------------*/
#ifdef __libTest_LoadECO

//#define onlyBaseVar 1

CHAR *libClassStr[11] = {"",   "=",  "°",  "+=",   "+-", "++--",
                         "=°", "=+", "-+", "--++", "°="};

static void PosLib_LoadECOTxt(void) {
  CFileTextOpenDialog dlg;
  CFile ecoFile;
  if (!dlg.Run(&ecoFile, "Open ECO Txt File")) return;

  ULONG bytes;
  PTR data;
  if (ecoFile.Load(&bytes, &data) != fileError_NoError) return;

  CGame game;
  ULONG n = 0;

  while (n < bytes)  // For each line...
  {
    CHAR line[1000], eco[10], comment[100], *s;
    LIB_CLASS libClass;

    // Read next line
    ReadLine(data, bytes, &n, 1000, line);
    s = line;

    // Skip leading blanks
#ifndef onlyBaseVar
    while (*s == ' ') s++;
#endif

    if (s[0] >= 'A' && s[0] <= 'E')  // A25 English (closed system)
    {
      eco[0] = *(s++);
      eco[1] = *(s++);
      eco[2] = *(s++);
      eco[3] = 0;

      comment[0] = 0;

      if (*(s++) == ' ') {
        INT i;
        for (i = 0; i < libCommentLength && *s; i++, s++) comment[i] = *s;
        while (i > 0 && comment[i - 1] == ' ') i--;  // Skip trailing blanks
        while (i <= libCommentLength) comment[i++] = 0;
      }
    } else if (s[0] == '1' &&
               s[1] == '.')  // 1.c2c4 e7e5 b1c3 b8c6 g2g3 g7g6 f1g2 f8g7
    {
      s += 2;

      game.UndoAllMoves();
      libClass = libClass_Level;

      BOOL done = false;
      while (s[0] >= 'a' && s[0] <= 'h' && !done)  // For each move...
      {
        SQUARE from = square(s[0] - 'a', s[1] - '1');
        SQUARE to = square(s[2] - 'a', s[3] - '1');
        done = (s[4] != ' ');
        s += 5;

        if (!(onBoard(from) && onBoard(to)))
          done = true;
        else {
          MOVE *M = game.Moves;
          MOVE *m = nil;

          for (INT i = 0; i < game.moveCount && !m; i++, M++)
            if (M->from == from && M->to == to) m = M;

          if (!m)
            done = true;
          else {
            // Per default inherit current classification
            libClass = PosLib_Probe(game.player, game.Board);
            if (libClass == libClass_Unclassified) libClass = libClass_Level;

            // Play the move
            game.PlayMove(m);

            // Check if any classification glyphs after move (if not then simply
            // "inherit" current which has just been set above before move was
            // performed).
            if (!done && *s && !(s[0] >= 'a' && s[0] <= 'h')) {
              CHAR lstr[10];
              INT i;
              for (i = 0; i < 4 && *s && *s != ' '; i++) lstr[i] = *(s++);
              lstr[i] = 0;
              if (*s == ' ') s++;

              for (INT j = 1; j <= 10; j++)
                if (EqualStr(lstr, libClassStr[j])) libClass = (LIB_CLASS)j;
            }

            // If the position is NOT already classified -> Classify it
            PosLib_Classify(game.player, game.Board, libClass,
                            false);  // Overwrite = false

            // If no ECO/Comment currently exists -> add it
            CHAR tmpECO[10], tmpComment[100];
            if (!PosLib_ProbeStr(game.player, game.Board, tmpECO, tmpComment))
              PosLib_StoreStr(game.player, game.Board, eco, comment);
          }
        }
      }
    }
  }

  Mem_FreePtr(data);
} /* PosLib_LoadECOTxt */

#endif

/**************************************************************************************************/
/*                                                                                                */
/*                                       START UP INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

void PosLib_AutoLoad(void) {
  CFile *file = new CFile();

  file->Set(Prefs.Library.name, '·LB6');
  if (file->Exists()) {
    PosLib_Open(file, false);
    return;
  }

  file->Set("Sigma Library", '·LB6');
  if (file->Exists()) {
    PosLib_Open(file, false);
    return;
  }

  delete file;
} /* PosLib_AutoLoad */

void ResetLibImportParam(LIBIMPORT_PARAM *param) {
  param->libClass = libClass_Level;
  param->overwrite = false;
  param->impWhite = true;
  param->impBlack = true;
  param->skipLosersMoves = true;
  param->maxMoves = 10;
  param->resolveCap = true;
} /* ResetLibImportParam */
