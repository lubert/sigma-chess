/**************************************************************************************************/
/*                                                                                                */
/* Module  : CollectionPGN.c */
/* Purpose : This module implements the PGN Import/Export routines (based on the
 * CPgn objects).   */
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

#include "CMemory.h"
#include "Collection.h"
#include "Pgn.h"
#include "TaskScheduler.h"

#include "CDialog.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                          PGN IMPORT */
/*                                                                                                */
/**************************************************************************************************/

// An important feature supported by the Collections is the ability to
// import/export multi-game PGN files. Importing is done using the
// CGame::ReadPGN method for each game, followed by a a call to the
// CGame::Decompress method.

// All imported games are temporarily appended to the collection. After
// successfully importing all games, the game map is adjust so the newly
// imported games appear at the desired place in the game map (starting at
// "gameNo").

#define pgnBufSize 100000L
#define smallPgnSize 20000L

BOOL SigmaCollection::ImportPGN(CFile *pgnFile) {
  if (colLocked) return false;

  if (SearchStr(pgnFile->name, ".epd", false, nil)) return ImportEPD(pgnFile);

  ULONG pgnFileSize;  // Total size of PGN file.

  // Open the PGN file and get its size:
  if (FileErr(pgnFile->Open(filePerm_Rd))) return false;
  if (FileErr(pgnFile->GetSize(&pgnFileSize))) {
    FileErr(pgnFile->Close());
    return false;
  }

  // Allocate PGN buffer and read first block of data:
  ULONG bufSize = ::MinL(pgnBufSize, pgnFileSize);  // Temporary PGN read buffer
  PTR pgnBuf = ::Mem_AllocPtr(bufSize);
  ULONG totalBytes = bufSize;  // Total number of bytes read from PGN file

  if (!pgnBuf) {
    FileErr(pgnFile->Close());
    return sigmaApp->MemErrorDialog();
  }

  // Open progress dialog:
  if (EqualStr(pgnFile->name, "clipboard.pgn"))
    BeginProgress("Paste Games", "Paste Games", pgnFileSize);
  else {
    CHAR prompt[100];
    Format(prompt, "Importing PGN file \"%s\"...", pgnFile->name);
    BeginProgress("PGN Import", prompt, pgnFileSize);
  }

  LONG gameCount0 = Info.gameCount;  // Import "starting" point.
  LONG N = 0;                        // Number of games imported so far.
  LONG errorCount = 0;               // Number of erroneous games so far.

  pgn_SkipThisGame = false;    // Skip current PGN game and continue with next.
  pgn_AutoSkipErrors = false;  // Automatically skip all errors.
  pgn_AbortImport = false;     // Abort import process
  pgn_DeleteImported =
      false;  // Delete imported games (after import process halted).

  // Initialize PGN Import object and various status variables:
  CPgn pgn(game);
  pgn.ReadBegin((CHAR *)pgnBuf);

  if (FileErr(pgnFile->Read(&totalBytes, (PTR)pgnBuf))) goto done;

  while (!pgn_AbortImport) {
    // First updated progress information:
    if (pgnFileSize <= smallPgnSize || N % 10 == 0)
      ImportPGNProgress(N, errorCount, pgn.GetTotalBytesRead(), pgnFileSize);

    if (!CheckGameCount("No more games can be imported")) goto done;

    // Parse and retrieve next PGN game.
    if (pgn.ReadGame(bufSize))  // Write game to collection (append):
    {
      if (MapFull(1))
        if (GrowMap(Info.gameCount / 100 + 1) !=
            colErr_NoErr)  // Grow 1 % if no room for next game
        {
          ::NoteDialog(
              nil, "PGN Import Error",
              "Failed allocating memory - No more games can be imported",
              cdialogIcon_Error);
          goto done;
        }

      AddGame(Info.gameCount, game, false);
      N++;
    } else if (pgn.GetError() == pgnErr_EOFReached)  // EOF reached -> We're
                                                     // done. Not a real error.
    {
      goto done;
    } else  // An error has occured -> Notify user
    {
      errorCount++;
      ImportPGNProgress(N, errorCount, pgn.GetTotalBytesRead(), pgnFileSize);
      HandlePGNError(&pgn);
      if (pgn_AbortImport) goto done;
    }

    // "Move" on to next PGN game in the PGN file:
    ULONG bytes = pgn.GetBytesRead();
    bufSize -= bytes;
    if (bufSize <= 0)
      goto done;
    else
      ::Mem_Move(pgnBuf + bytes, pgnBuf, bufSize);

    // Fill up read buffer by appending new data to file:
    bytes = MinL(bytes, pgnFileSize - totalBytes);  // Bytes to append
    if (bytes > 0) {
      if (FileErr(pgnFile->Read(&bytes, pgnBuf + bufSize))) goto done;
      totalBytes += bytes;
      bufSize += bytes;
    }
  }

done:
  Mem_FreePtr(pgnBuf);

  ImportPGNProgress(N, errorCount, pgn.GetTotalBytesRead(), pgnFileSize);
  EndProgress();

  // Flush game map and info if the imported games should not be deleted.
  if (N > 0) {
    if (!pgn_DeleteImported)
      WriteMap(gameCount0);
    else
      Info.gameCount = gameCount0;
    WriteInfo();
  }

close:
  // Close the PGN file:
  FileErr(pgnFile->Close());

  if (N > 0 && !pgn_DeleteImported) View_Add(gameCount0, Info.gameCount - 1);

  return (N > 0 && !pgn_DeleteImported);
} /* SigmaCollection::ImportPGN */

void SigmaCollection::ImportPGNProgress(ULONG gameCount, ULONG errorCount,
                                        ULONG bytesProcessed,
                                        ULONG pgnFileSize) {
  CHAR status[100];
  Format(status, "%ld games (%d%c of %ldK), %ld error(s)", gameCount,
         (INT)((100.0 * bytesProcessed) / pgnFileSize), '%', pgnFileSize / 1024,
         errorCount);
  SetProgress(bytesProcessed, status);
  if (ProgressAborted()) pgn_AbortImport = true;
} /* SigmaCollection::ImportPGNProgress */

/*--------------------------------------- Error Reporting
 * ----------------------------------------*/
// The PGN error dialog shows the erroneous line (including line number and
// column) and also shows an error message/description and optionally the
// erroneous token. The user is then presented with the following 4 choices:
//
// ¥ CONTINUE AND SKIP CURRENT GAME
//   This will skip the current game by trying to locate the start of the next
//   game, and then
// resuming the PGN import process.
//
// ¥ CONTINUE AND AUTO-SKIP ALL ERRONEOUS GAMES
// Like above but will automatically skip ALL subsequent erroneous games without
// displaying the error dialog again (unless an unrecoverable error occurs, i.e.
// the start of the next game can not be found). This option only applies when
// importing into a collection.
//
// ¥ ABORT PGN IMPORT PROCESS
// This will terminate the PGN import process. The user can then examine and
// perhaps repair the PGN file with a text editor.
//
// ¥ ABORT AND DELETE GAMES IMPORTED SO FAR
// Like above, but will additionally remove all games from the current
// collection that was imported from the PGN file (this option thus only applies
// when importing into a collection, and should be disable/invisible when
// opening PGN games the normal way from the File-Open or File-Next PGN game
// commands).
//
// The chosen action will be stored in the pgnErrAction variable for use by the
// other routines. In cases 1 and 2 above, the error flag is cleared and the
// skipThisGame flag is set to true.

class CPGNErrorDialog : public CDialog {
 public:
  CPGNErrorDialog(CRect frame, CPgn *pgn);

  CRadioButton *cradio_Skip, *cradio_AutoSkip, *cradio_Abort, *cradio_AbortDel;
  CTextControl *ctext_ErrPos, *ctext_ErrMsg, *ctext_Comment;
  CEditControl *cedit_Details;
};

void SigmaCollection::HandlePGNError(CPgn *pgn) {
  if (pgn->GetError() == pgnErr_UnexpectedEOF) {
    pgn_AbortImport = true;
    NoteDialog(nil, "PGN Import Error", "Unexpected end of file...",
               cdialogIcon_Error);
    return;
  } else if (pgn_AutoSkipErrors) {
    pgn_SkipThisGame = true;
  } else {
    CRect frame(0, 0, 400, 230);
    if (RunningOSX()) frame.right += 50, frame.bottom += 16;
    theApp->CentralizeRect(&frame, true);
    CPGNErrorDialog *dialog = new CPGNErrorDialog(&frame, pgn);
    dialog->Run();

    pgn_SkipThisGame = dialog->cradio_Skip->Selected();
    pgn_AutoSkipErrors = dialog->cradio_AutoSkip->Selected();
    pgn_DeleteImported = dialog->cradio_AbortDel->Selected();
    pgn_AbortImport = (dialog->cradio_Abort->Selected() || pgn_DeleteImported);

    delete dialog;
  }

  if (pgn_SkipThisGame && !pgn->SkipGame()) {
    pgn_AbortImport = true;
    NoteDialog(nil, "Fatal PGN Import Error",
               "An unrecoverable error was encountered - The PGN Import "
               "process will be aborted",
               cdialogIcon_Error);
  }
} /* SigmaCollection::HandlePGNError */

CPGNErrorDialog::CPGNErrorDialog(CRect frame, CPgn *pgn)
    : CDialog(nil, "PGN Import Error", frame) {
  CRect r = InnerRect();
  r.bottom = r.top + 15;

  CHAR *comment =
      "You cannot repair the error here, but you can copy the above line\\
 to the clipboard and locate it in the PGN file with a text editor";
  CHAR errPosStr[100];
  CHAR errMsgStr[200];
  CHAR errLineStr[200];
  LONG line;
  INT column;
  pgn->CalcErrorStats(&line, &column, errMsgStr, errLineStr);
  Format(errPosStr, "Error in line %ld, position %d", line, column);

  ctext_ErrPos = new CTextControl(this, errPosStr, r);
  r.Offset(0, controlVDiff_Text);
  ctext_ErrMsg = new CTextControl(this, errMsgStr, r);
  r.Offset(0, controlVDiff_Edit);
  r.bottom = r.top + controlHeight_Edit;
  cedit_Details = new CEditControl(this, errLineStr, r, 80);
  r.Offset(0, controlVDiff_Edit);
  r.bottom = r.top + 30;
  ctext_Comment =
      new CTextControl(this, comment, r, true, controlFont_SmallSystem);
  r.Offset(0, 35);
  r.bottom = r.top + controlHeight_RadioButton;

  r.right = DefaultRect().left - 5;
  cradio_Skip =
      new CRadioButton(this, "Skip current game and continue PGN import", 1, r);
  r.Offset(0, controlVDiff_RadioButton);
  cradio_AutoSkip =
      new CRadioButton(this, "Continue PGN import and ignore errors", 1, r);
  r.Offset(0, controlVDiff_RadioButton);
  cradio_Abort = new CRadioButton(this, "Abort PGN import", 1, r);
  r.Offset(0, controlVDiff_RadioButton);
  cradio_AbortDel = new CRadioButton(
      this, "Abort PGN import and delete imported games", 1, r);

  if (pgn->GetError() != pgnErr_UnexpectedEOF)
    cradio_Skip->Select();
  else {
    cradio_Skip->Enable(false);
    cradio_AutoSkip->Enable(false);
    cradio_Abort->Select();
  }

  cbutton_Default = new CPushButton(this, "OK", DefaultRect());
  SetDefaultButton(cbutton_Default);
} /* CPGNErrorDialog::CPGNErrorDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                          EPD IMPORT */
/*                                                                                                */
/**************************************************************************************************/

// If the name of the file to be imported ends with ".epd" we instead assume
// it's an EPD file, and the ImportEPD routine is called instead.
//
// We read one line at a time. If it's empty, blank, a comment, or a time
// control line we skip it and move on to the next line.

BOOL SigmaCollection::ImportEPD(CFile *epdFile) {
  if (colLocked) return false;

  ULONG epdFileSize, bytesRead = 0;  // Total size of EPD file.
  PTR epdBuf, pos;
  CHAR epdLine[1000];
  LONG gameCount0 = Info.gameCount;  // Import "starting" point.
  LONG errorCount = 0;
  LONG N = 0;  // Number of games imported so far.

  // Open the EPD file and get its size:
  if (epdFile->Load(&epdFileSize, &epdBuf) != fileError_NoError) return false;

  // Open progress dialog:
  CHAR prompt[100];
  Format(prompt, "Importing EPD file \"%s\"...", epdFile->name);
  BeginProgress("EPD Import", prompt, epdFileSize);

  pos = epdBuf;

  while (bytesRead < epdFileSize) {
    // First updated progress information:
    if (epdFileSize <= smallPgnSize || N % 10 == 0)
      ImportPGNProgress(N, errorCount, bytesRead, epdFileSize);

    if (!CheckGameCount("No more positions can be imported")) goto done;

    // Read next line:
    ReadLine(pos, epdFileSize, &bytesRead, 1000, epdLine);

    // Parse and retrieve next PGN game.
    if (IsAlphaNum(epdLine[0]))
      if (game->Read_EPD(epdLine) == epdErr_NoError) {
        AddGame(Info.gameCount, game, false);
        N++;
      } else {
        ImportPGNProgress(N, ++errorCount, bytesRead, epdFileSize);
      }
  }

done:
  Mem_FreePtr(epdBuf);

  ImportPGNProgress(N, errorCount, epdFileSize, epdFileSize);
  EndProgress();

  // Flush game map and info if the imported games should not be deleted.
  if (N > 0) {
    WriteMap(gameCount0);
    WriteInfo();
    View_Add(gameCount0, Info.gameCount - 1);
  }

  return (N > 0);
} /* SigmaCollection::ImportEPD */

/**************************************************************************************************/
/*                                                                                                */
/*                                          PGN EXPORT */
/*                                                                                                */
/**************************************************************************************************/

BOOL SigmaCollection::ExportPGN(CFile *pgnFile, ULONG i1, ULONG i2) {
  // Open the newly created PGN file:
  if (!pgnFile->Exists()) pgnFile->Create();
  if (FileErr(pgnFile->Open(filePerm_Wr))) return false;

  PGN_FLAGS flags =
      (Prefs.PGN.skipMoveSep ? pgnFlag_SkipMoveSep : pgnFlag_None);

  CPgn pgn(game, flags);      // PGN export object
  LONG pgnPos = 0;            // PGN file pos
  ULONG count = i2 + 1 - i1;  // Total number of games to export.

  if (EqualStr(pgnFile->name, "clipboard.pgn"))
    BeginProgress("Copy Games", "Copy Games", count);
  else {
    CHAR prompt[200];
    Format(prompt, "Exporting games to PGN file \"%s\"...", pgnFile->name);
    BeginProgress("PGN Export", prompt, count);
  }

  for (ULONG i = i1; i <= i2 && !ProgressAborted(); i++) {
    ULONG g = ViewMap[i];
    ULONG n = i - i1;

    CHAR status[100];
    Format(status, "%d%c (%ld games of %ld)", (INT)((100.0 * n) / count), '%',
           n, count);
    if (n % 10 == 0) SetProgress(n, status);

    ULONG bytes = Map[g].size;
    if (FileErr(file->SetPos(Map[g].pos))) goto error;
    if (FileErr(file->Read(&bytes, gameData))) goto error;
    game->Decompress(gameData, bytes);
    bytes = pgn.WriteGame((CHAR *)gameData);
    if (FileErr(pgnFile->SetPos(pgnPos))) goto error;
    if (FileErr(pgnFile->Write(&bytes, gameData))) goto error;
    pgnPos += bytes;
  }

  if (FileErr(pgnFile->SetSize(pgnPos))) goto error;
  EndProgress();
  return !FileErr(pgnFile->Close());

error:
  EndProgress();
  FileErr(pgnFile->Close());
  return false;
} /* SigmaCollection::ExportPGN */
