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

#include "PGNFile.h"
#include "CollectionWindow.h"
#include "PGN.h"

static BOOL CheckOpenSingleGamePGN(CFile *pgnFile);

void OpenPGNFile(CFile *pgnFile) {
  if (!sigmaApp->CheckWinCount() || !sigmaApp->CheckMemFree(250)) return;

  if (CheckOpenSingleGamePGN(pgnFile)) return;

  CHAR colName[100],
      pgnColName[100];  // Name of collection created from PGN file.

  // First create collection file name from PGN file name by stripping the
  // ".pgn" extension.

  ::CopyStr(pgnFile->name, colName);
  INT n = StrLen(colName);
  if (n >= 5 && (::SameStr(&colName[n - 4], ".pgn") ||
                 ::SameStr(&colName[n - 4], ".epd")))
    colName[n - 4] = 0;
  /*
     if (RunningOSX())
        Format(pgnColName, "%s", colName);
     else
        Format(pgnColName, ":PGN:%s", colName);
  */
  Format(pgnColName, "%s", colName);

  // Once the new file name has been created we create the collection:

  CFile *colFile = new CFile();
  BOOL createColFile = false;

  if (Prefs.Collections.autoName &&
      colFile->Set(pgnColName, '·GC5', theApp->creator, filePath_Documents) ==
          fileError_NoError &&
      //       colFile->Set(pgnColName, '·GC5', theApp->creator, (RunningOSX() ?
      //       filePath_Documents : filePath_Default)) == fileError_NoError &&
      !SameStr(pgnFile->name, colFile->name) &&
      !sigmaApp->WindowTitleUsed(colFile->name, false) &&
      !(colFile->Exists() && colFile->Delete() != fileError_NoError)) {
    createColFile = true;
  } else if (colFile->SaveDialog("Create Collection from PGN", colName) &&
             !sigmaApp->WindowTitleUsed(colFile->name, true)) {
    if (SameStr(pgnFile->name, colFile->name))
      NoteDialog(nil, "Invalid File Name",
                 "The collection file name may NOT be the same as the PGN file "
                 "name...");
    else {
      createColFile = true;
      if (colFile->saveReplace) colFile->Delete();
    }
  }

  if (createColFile) {
    CollectionWindow *colWin = new CollectionWindow(
        colFile->name, theApp->NewDocRect(ColWin_Width(), ColWin_Height()),
        colFile);

    colWin->SetBusy(true);

    if (colWin->collection->ImportPGN((CFile *)pgnFile)) {
      colWin->gameListArea->ResetScroll();
      colWin->HandleMenuAdjust();
    }

    colWin->SetBusy(false);
  }

  delete colFile;
} /* OpenPGNFile */

BOOL IsPGNFileName(CHAR *fileName) {
  INT n = StrLen(fileName);
  return (n >= 5 && (::SameStr(&fileName[n - 4], ".pgn") ||
                     ::SameStr(&fileName[n - 4], ".epd")));
} /* IsPGNFileName */

/*------------------------------- Check for Single Game PGN Files
 * --------------------------------*/
// Open single game PGN files in GameWindows instead (prefs)

#define maxPGNSingleSize 10000L

static BOOL IsSingleGamePGN(CHAR *pgnBuf, ULONG pgnSize);
static void OpenSingleGamePGNFile(CFile *pgnFile, CHAR *pgnBuf,
                                  ULONG pgnFileSize);

static BOOL CheckOpenSingleGamePGN(CFile *pgnFile) {
  if (!Prefs.PGN.openSingle) return false;

  if (::SameStr(&(pgnFile->name[StrLen(pgnFile->name) - 4]), ".epd"))
    return false;

  ULONG pgnFileSize;  // Total size of PGN file.
  CHAR *pgnBuf = nil;
  BOOL success = false;

  //--- Open the PGN file, get its size and check it ain't too big for a single
  //game PGN file ---

  if (FileErr(pgnFile->Open(filePerm_Rd))) return false;
  if (FileErr(pgnFile->GetSize(&pgnFileSize))) goto done;
  if (pgnFileSize > maxPGNSingleSize) goto done;

  //--- Read the file and check if it's a single game file (only one tag
  //section) ---

  pgnBuf = (CHAR *)Mem_AllocPtr(pgnFileSize + 1);
  if (!pgnBuf) goto done;
  pgnFile->Read(&pgnFileSize, (PTR)pgnBuf);
  pgnBuf[pgnFileSize] = 0;
  if (!IsSingleGamePGN(pgnBuf, pgnFileSize)) goto done;

  // If we get here, it seems to be a single game PGN file, although there may
  // still be syntax errors.
  success = true;

done:
  //--- Release pgn buffer, close file and return success value ---

  FileErr(pgnFile->Close());

  if (success) OpenSingleGamePGNFile(pgnFile, pgnBuf, pgnFileSize);
  if (pgnBuf) Mem_FreePtr(pgnBuf);

  return success;
} /* CheckOpenSingleGamePGN */

static BOOL IsSingleGamePGN(CHAR *pgnBuf, ULONG pgnBufSize) {
  INT pos;
  if (!SearchStr(pgnBuf, "[White ", true, &pos)) return false;
  return !SearchStr(&pgnBuf[pos + 1], "[White ", true, &pos);
  /*
     INT i = 0;

     // First skip forward to first '[' starting on a new line
     while (! (pgnBuf[i] == '[' && (i == 0 || IsNewLine(pgnBuf[i-1]))))
        if (++i >= pgnBufSize) return false;

     // Look for newline. As long as next char is a '[' we are still in the tag
     section
     // of the first game.
     do
     {  while (! IsNewLine(pgnBuf[i++]))      // Find next newline:
           if (i >= pgnBufSize) return false;
     } while (pgnBuf[i] == '[');

     // If another tag section found, assume multi game PGN file and abort
     while (++i < pgnBufSize - 1)
        if (IsNewLine(pgnBuf[i]) && pgnBuf[i + 1] == '[')
           return false;

     return true;
  */
} /* IsSingleGamePGN */

static void OpenSingleGamePGNFile(CFile *pgnFile, CHAR *pgnBuf,
                                  ULONG pgnFileSize) {
  CGame *gameTemp = new CGame();
  CPgn *pgnTemp = new CPgn(gameTemp);

  pgnTemp->ReadBegin((CHAR *)pgnBuf);

  if (pgnTemp->ReadGame(pgnFileSize)) {
    GameWindow *win = NewGameWindow(pgnFile->name, false, false);
    if (!win) goto done;
    win->file = pgnFile;
    win->game->CopyFrom(gameTemp);
    win->game->UndoAllMoves();

    if (Prefs.Games.gotoFinalPos && win->game->CanRedoMove())
      win->HandleMessage(game_RedoAllMoves);
    else
      win->GameMoveAdjust(false);

    win->CheckTurnPlayer();
  } else if (gameTemp->lastMove > 0) {
    CHAR s[200];
    Format(s,
           "An error occured in move %d. You can try to correct this by "
           "opening the game in a text editor.",
           gameTemp->lastMove / 2 + 1);
    NoteDialog(nil, "Failed Opening PGN Game", s, cdialogIcon_Error);
  } else {
    NoteDialog(nil, "Failed Opening PGN Game",
               "The format of the PGN file is invalid...", cdialogIcon_Error);
  }

done:
  delete pgnTemp;
  delete gameTemp;
} /* OpenSingleGamePGNFile */

void ForcePGNExtension(CFile *file) {
  CHAR pgnFileName[maxFileNameLen + 1];
  INT n = StrLen(file->name);

  if (file->fileType == 'TEXT' && n < maxFileNameLen - 4 &&
      !SameStr(&file->name[n - 4], ".pgn")) {
    AppendStr(file->name, ".pgn", pgnFileName);
    file->SetName(pgnFileName);
  }
} /* ForcePGNExtension */
