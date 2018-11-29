/**************************************************************************************************/
/*                                                                                                */
/* Module  : COLLECTIONWINDOW.C                                                                   */
/* Purpose : This module implements the game windows.                                             */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
  and the following disclaimer in the documentation and/or other materials provided with the 
  distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CollectionWindow.h"
#include "GameWindow.h"
#include "FilterDialog.h"
#include "ColInfoDialog.h"
#include "LayoutDialog.h"
#include "MoveGamesDialog.H"
#include "InfoFilterDialog.h"
#include "GamePrint.h"
#include "PGN.h"
#include "PGNFile.h"
#include "ExportHTML.h"
#include "PosLibrary.h"
#include "EngineMatchDialog.h"

#include "SigmaApplication.h"
#include "CDialog.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                CREATE/OPEN COLLECTION WINDOW                                   */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------- New Collection Window --------------------------------------*/

CollectionWindow *NewCollectionWindow (void)
{
   if (! sigmaApp->CheckWinCount() || ! sigmaApp->CheckMemFree(250)) return nil;

	CollectionWindow *colWin = nil;
   CFile *file = new CFile();

   if (file->SaveDialog("Create Collection", "Untitled") && ! sigmaApp->WindowTitleUsed(file->name,true))
   {
      if (file->saveReplace) file->Delete();
      colWin = new CollectionWindow(file->name, theApp->NewDocRect(ColWin_Width(),ColWin_Height()), file);
      file->CompleteSave();
   }

   delete file;
   return colWin;
} /* NewCollectionWindow */


INT ColWin_Width (void)
{
   return GameWin_Width(minSquareWidth);
} /* GameWin_Width() */


INT ColWin_Height (void)
{
   return GameWin_Height(minSquareWidth);
} /* ColWin_Height() */

/*---------------------------------- Open Collection Window -------------------------------------*/

class ColOpenDialog : public CFileOpenDialog
{
public:
   ColOpenDialog (void);
   virtual BOOL Filter (OSTYPE fileType, CHAR *name);
};


ColOpenDialog::ColOpenDialog (void)
  : CFileOpenDialog ()
{
} /* FilterOpenDialog::FilterOpenDialog */


BOOL ColOpenDialog::Filter (OSTYPE fileType, CHAR *fileName)
{
   return (fileType == '�GC5');
} /* ColOpenDialog::Filter */


CollectionWindow *OpenCollectionWindow (void)
{
   ColOpenDialog dlg;
   CFile         file;
   return (dlg.Run(&file,"Open Collection") ? OpenCollectionFile(&file) : nil);
} /* OpenCollectionWindow */


CollectionWindow *OpenCollectionFile (CFile *file)
{
   if (! sigmaApp->CheckWinCount() || ! sigmaApp->CheckMemFree(250)) return nil;
   
   CollectionWindow *colWin = nil;

   if (file->fileType == '�GCX')       // If a � 4 collection has been selected we have to
   {                                   // convert it to the new � 5 collection format.
      NoteDialog(nil, "Note", "This collection was created with Sigma Chess 4.0 and must be \\
converted to the new format supported by Sigma Chess 5 & 6 ...");

      CFile *file5 = new CFile();

         if (file5->SaveDialog("Create Collection", "Untitled") && ! sigmaApp->WindowTitleUsed(file5->name,true))
         {
            if (file5->saveReplace) file5->Delete();
            SigmaCollection *collection = new SigmaCollection(file5, nil);
            file5->CompleteSave();
            collection->Sigma4Convert(file);
            delete collection;

            colWin = OpenCollectionFile(file5);
         }

      delete file5;
   }
   else
   {
      colWin = new CollectionWindow(file->name, theApp->NewDocRect(ColWin_Width(),ColWin_Height()), file);
   }

   return colWin;
} /* OpenCollectionFile */


/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESCTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

CollectionWindow::CollectionWindow (CHAR *title, CRect frame, CFile *file)
  : SigmaWindow(title, frame, sigmaWin_Collection, true, CRect(640,200,1024,1024))

// The CollectionWindow constructor sets up the main views in the window: 
// Also the Collection object is created, initialized and "attached" to the window.

{
   // Create and attach the corresponding Collection object before initializing window views
   // (since these depend on the existence of the Collection object but not vice versa).

   gameListArea = nil;
   toolbar = nil;
   busy = false;
   hasFile = true;
   collection = new SigmaCollection(file, this);
   if (! collection->Map || ! collection->ViewMap)
   {  delete this;
      return;
   }

   toolbarTop = Prefs.ColDisplay.toolbarTop;

   // Add the views:
   CalcFrames();
   gameListArea = new GameListArea(this, gameListRect);
   toolbar      = new CollectionToolbar(this, toolbarRect);
   miniToolbar  = new MiniCollectionToolbar(this, miniToolbarRect);

   // Finally adjust toolbar and show the window:
   AdjustToolbar();

   Show(true);
   SetFront();

   HandleMessage(msg_ColSelChanged);

   if (collection->liteLimit)
      ProVersionDialog(this, "Collections are limited to 1000 games in Sigma Chess Lite. \
The collection will be opened in read-only mode showing the first 1000 games only.");

   // Auto-open first game if publishing:
   if (collection->Publishing() && collection->GetGameCount() > 0)
      HandleMessage(collection_OpenGame);

} /* CollectionWindow::CollectionWindow */


CollectionWindow::~CollectionWindow (void)
{
   // Detach any open games:
   gameWinList.Scan();
   while (GameWindow *gameWin = (GameWindow*)gameWinList.Next())
      gameWin->Detach();

   if (EngineMatch.colWin == this)
      EngineMatch.colWin = nil;
    
   // Delete the collection object (which flushes and closes the collection file):
   delete collection;
} /* CollectionWindow::~CollectionWindow */


void CollectionWindow::CalcFrames (void)
{
   miniToolbarRect = Bounds();
   miniToolbarRect.top = miniToolbarRect.bottom - toolbar_HeightSmall;

   toolbarRect = Bounds();
   gameListRect = Bounds();

   if (! toolbarTop)
   {  toolbarRect.bottom  = miniToolbarRect.top;
      toolbarRect.top     = toolbarRect.bottom - toolbar_Height;
      gameListRect.bottom = toolbarRect.top;
   }
   else
   {  toolbarRect.bottom  = toolbarRect.top + toolbar_Height;
      gameListRect.top    = toolbarRect.bottom;
      gameListRect.bottom = miniToolbarRect.top;
   }
} /* CollectionWindow::CalcFrames */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Closing/Quitting ----------------------------------------*/

BOOL CollectionWindow::HandleCloseRequest (void)
{
   if (busy) return false;
   return CheckSave("Save in collection before closing?");
} /* CollectionWindow::HandleCloseRequest */


BOOL CollectionWindow::HandleQuitRequest (void)
{
   if (busy) return false;
   return CheckSave("Save in collection before quitting?");
} /* CollectionWindow::HandleQuitRequest */

// If any games have been opened and changes made, we first have to ask the user if he wants to
// save the changes to the collection before closing/quitting.

BOOL CollectionWindow::CheckSave (CHAR *prompt)
{
   gameWinList.Scan();
   while (GameWindow *gameWin = (GameWindow*)gameWinList.Next())
      if (! gameWin->CheckSave(prompt))
         return false;
   return true;
} /* CollectionWindow::CheckSave */


BOOL CollectionWindow::IsLocked (void)
{
   return collection->IsLocked();
} /* void CollectionWindow::IsLocked */

/*---------------------------------------- Misc Events -------------------------------------------*/

void CollectionWindow::HandleResize (INT width, INT height)
{
   Resize(width, height);
   
   CalcFrames();
// CRect gameListRect = Bounds();
// gameListRect.bottom -= toolbar_Height;
   gameListArea->SetFrame(gameListRect,true);

// CRect toolbarRect = Bounds();
// toolbarRect.top = gameListRect.bottom + 1;
// toolbarRect.Inset(0,-1);
   toolbar->SetFrame(toolbarRect, true);
   miniToolbar->SetFrame(miniToolbarRect, true);
} /* CollectionWindow::HandleResize */


void CollectionWindow::HandleZoom (void)
{
   CRect r = sigmaApp->ScreenRect(); r.Inset(5, 25);
// INT maxWidth  = Max(windowWidth, r.right - Frame().left);
   INT maxHeight = Max(150, r.bottom - Frame().top);
   if (/*Frame().Width() != maxWidth ||*/ Frame().Height() != maxHeight)
      HandleResize(Frame().Width(), maxHeight);
   else
      HandleResize(Frame().Width(), ColWin_Height());
} /* CollectionWindow::HandleZoom */


void CollectionWindow::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   if (busy) return;

   if (key == key_BackDel && ! IsLocked())
      DeleteSelection();
   else if (! gameListArea->HandleKeyDown(c, key, modifiers))
   {
     //### send key strokes to "next" view
   }
} /* CollectionWindow::HandleKeyDown */


void CollectionWindow::HandleScrollBar (CScrollBar *ctrl, BOOL tracking)
{
   gameListArea->CheckScrollEvent(ctrl, tracking);
} /* CollectionWindow::HandleScrollBar */


/**************************************************************************************************/
/*                                                                                                */
/*                                     LOADING/SAVING/CLOSING GAMES                               */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Loading Games ----------------------------------------*/
// Opens the specified game in a new game window (just as if the game had been opened from a normal
// file):

#define maxSuffixLen 8  // " [99999]"

void CollectionWindow::OpenGame (LONG gameNo, GameWindow *targetWin)
{
   if (GameOpened(gameNo))
   {
      gameWinList.Scan();
      while (GameWindow *gameWin = (GameWindow*)gameWinList.Next())
         if (gameWin->colGameNo == gameNo)
         {  if (! gameWin->IsFront()) gameWin->SetFront();
            return;
         }
   }

   ULONG bytes;

   if (collection->GetGame(gameNo, gameData, &bytes) == colErr_NoErr)
   {
      // First create game title, by suffixing collection name with game number (in brackets).
      // NOTE: We have to ensure that the resulting string isn't too long!!!
      CHAR prefix[cwindow_MaxTitleLen + 1];
      CHAR title[cwindow_MaxTitleLen + 1];

      CopyStr(collection->file->name, prefix);
      if (StrLen(collection->file->name) + maxSuffixLen > cwindow_MaxTitleLen)
      {  INT i = cwindow_MaxTitleLen - maxSuffixLen - 1;
         prefix[i++] = '�';
         prefix[i++] = 0;
      }
      
      Format(title, "%s [%ld]", prefix, gameNo + 1);

      // Next create/select the actual game window:
      GameWindow *win = (targetWin == nil ? NewGameWindow(title) : targetWin);
      if (win)
      {
	      if (targetWin) win->SetTitle(title);
	      win->game->Decompress(gameData, bytes);
	      win->Attach(this, gameNo);
	      gameWinList.Append(win);
	      win->RefreshGameInfo();

	      if (Prefs.Games.gotoFinalPos && win->game->CanRedoMove() && ! win->analyzeGame && ! collection->Publishing())
	         win->HandleMessage(game_RedoAllMoves);
	      else
	         win->GameMoveAdjust(true);

         win->CheckTurnPlayer();
	   }
   }
} /* CollectionWindow::OpenGame */


BOOL CollectionWindow::GameOpened (LONG gameNo)
{
   gameWinList.Scan();
   while (GameWindow *gameWin = (GameWindow*)gameWinList.Next())
      if (gameWin->colGameNo == gameNo) return true;
   return false;
} /* CollectionWindow::GameOpened */


void CollectionWindow::DetachGameWin (GameWindow *gameWin)
// Called by the GameWindow destructor if the game was attached to a collection. Is also called
// if the user explicitly detaches a games from its collection.
{
   gameWinList.Remove(gameWin);
   gameListArea->DrawList(); //listView->Redraw();
} /* CollectionWindow::DetachGameWin */


void CollectionWindow::PrevGame (GameWindow *target)
{
   if (gameListArea->GetSel() == 0) return;

   if (target) DetachGameWin(target);
   gameListArea->HandleKeyDown(' ', key_UpArrow, 0);
   OpenGame(collection->View_GetGameNo(gameListArea->GetSel()), target);
   gameListArea->DrawList();
} /* CollectionWindow::NextGame */


void CollectionWindow::NextGame (GameWindow *target)
{
   if (gameListArea->GetSel() >= collection->View_GetGameCount() - 1) return;

   if (target) DetachGameWin(target);
   gameListArea->HandleKeyDown(' ', key_DownArrow, 0);
   OpenGame(collection->View_GetGameNo(gameListArea->GetSel()), target);
   gameListArea->DrawList();
} /* CollectionWindow::NextGame */


BOOL CollectionWindow::CanPrevGame (void)
{
   return (gameListArea->GetSel() > 0);
} /* CollectionWindow::CanNextGame */


BOOL CollectionWindow::CanNextGame (void)
{
   return (gameListArea->GetSel() < gameListArea->GetTotalCount() - 1);
} /* CollectionWindow::CanNextGame */

/*----------------------------------------- Saving Games -----------------------------------------*/
// When a changed game is saved we first update the collection file. Next we update the view and
// redraw the game list.

void CollectionWindow::SaveGame (ULONG gameNo, CGame *game)
{
   if (IsLocked())
   {  NoteDialog(this, "Collection Locked", "This collection is locked. The game cannot be saved.");
      return;
   }

   collection->UpdGame(gameNo, game, true);
   collection->View_UpdateGame(gameNo);
   gameListArea->RefreshList();
} /* CollectionWindow::SaveGame */

/*----------------------------------------- Adding Games -----------------------------------------*/

void CollectionWindow::AddGame (GameWindow *gameWin)
{
   if (IsLocked())
   {  NoteDialog(this, "Collection Locked", "This collection is locked. No games can be added.");
      return;
   }
   else if (! collection->CheckGameCount("No more games can be added")) return;

   ULONG gameNo = collection->Info.gameCount;
   CHAR title[100];

   collection->AddGame(gameNo, gameWin->game, true);
   collection->View_Add(gameNo, gameNo);

   gameWin->Attach(this, gameNo);
   Format(title, "%s : [%ld]", collection->file->name, gameNo + 1);
   gameWin->SetTitle(title);

   gameWinList.Append(gameWin);
   gameListArea->RefreshList();
   AdjustToolbar();
} /* CollectionWindow::AddGame */

/*----------------------------------------- Deleting Games ---------------------------------------*/

void CollectionWindow::DeleteSelection (void)
{
   if (gameWinList.Count() > 0)
   {  NoteDialog(this, "Delete Games", "You cannot delete from a collection where games are currently open...");
      return;
   }

   ULONG first = gameListArea->GetSelStart();
   ULONG last  = gameListArea->GetSelEnd();
   ULONG count = last + 1 - first;
   CHAR  prompt[200], title[cwindow_MaxTitleLen];

   GetTitle(title);
   Format(prompt, "Are you sure you want to delete %ld game%s from the collection �%s�?", count, (count > 1 ? "s" : ""), title);
   if (! QuestionDialog(this, "Delete Games", prompt)) return;

   collection->View_Delete(first, last);

   ULONG newSel = MaxL(0, first - 1);
   gameListArea->SetSelection(newSel, newSel);
   gameListArea->RefreshList();
   AdjustToolbar();
} /* CollectionWindow::DeleteSelection */

/*-------------------------------------- Setting Layout Info -------------------------------------*/

void CollectionWindow::EditLayout (LONG gameNo)
{
   CGame *game = nil;
   BOOL  delGame = false;

   if (GameOpened(gameNo))
   {
      gameWinList.Scan();
      while (GameWindow *gameWin = (GameWindow*)gameWinList.Next())
         if (gameWin->colGameNo == gameNo)
            game = gameWin->game;
   }
   else
   {
      ULONG bytes = 0;
      if (collection->GetGame(gameNo, gameData, &bytes) == colErr_NoErr)
      {  game = new CGame();
         delGame = true;
         game->Decompress(gameData, bytes);
      }
   }

   if (game && LayoutDialog(gameNo, &(game->Info), IsLocked()))
      collection->UpdGame(gameNo, game, true);

   if (delGame) delete game;

   gameListArea->DrawList();
} /* CollectionWindow::EditLayout */


/**************************************************************************************************/
/*                                                                                                */
/*                                       INDEXING/SORTING                                         */
/*                                                                                                */
/**************************************************************************************************/

BOOL CollectionWindow::Sort (INDEX_FIELD inxField)
{
   BOOL sortOK = false;

   SetBusy(true);
   sortOK = collection->Sort(inxField);
   SetBusy(false);
   return sortOK;
} /* CollectionWindow::Sort */


BOOL CollectionWindow::SetSortDir (BOOL ascend)
{
   return collection->SetSortDir(ascend);
} /* CollectionWindow::SetSortDir */


/**************************************************************************************************/
/*                                                                                                */
/*                                       PGN IMPORT/EXPORT                                        */
/*                                                                                                */
/**************************************************************************************************/

class PGNOpenDialog : public CFileOpenDialog
{
public:
   PGNOpenDialog (void);
   virtual BOOL Filter (OSTYPE fileType, CHAR *name);
};


PGNOpenDialog::PGNOpenDialog (void)
  : CFileOpenDialog ()
{
} /* EPDOpenDialog::EPDOpenDialog */


BOOL PGNOpenDialog::Filter (OSTYPE fileType, CHAR *fileName)
{
   return (IsPGNFileName(fileName) || (fileType == 'TEXT' && ! Prefs.PGN.fileExtFilter));
// return (fileType == 'TEXT' && IsPGNFileName(fileName));
} /* PGNOpenDialog::Filter */


void CollectionWindow::ImportPGN (void)
{
   CFile         file;
   PGNOpenDialog dlg;

   sigmaApp->colWinImportTarget = this;
   SetBusy(true);   
      collection->pgn_AbortImport = false;
      dlg.Run(nil,"Import PGN");
   SetBusy(false);
   sigmaApp->colWinImportTarget = nil;
} /* CollectionWindow::ImportPGN */


void CollectionWindow::ImportPGNFile (CFile *file)
{
   if (! collection->pgn_AbortImport && collection->ImportPGN(file))
   {  gameListArea->RefreshList();
      HandleMenuAdjust();
   }
} // ImportPGNFile


void CollectionWindow::ExportPGN (ULONG i1, ULONG i2)
{
   CFile *pgnFile = new CFile();

   if (pgnFile->SaveDialog("Export PGN", ".pgn"))
   {
      SetBusy(true);
      if (! pgnFile->Exists())
         pgnFile->Create();
      pgnFile->SetType('TEXT');
      collection->ExportPGN((CFile*)pgnFile, i1, i2);
      SetBusy(false);
   }

   delete pgnFile;
} /* CollectionWindow::ExportPGN */


/**************************************************************************************************/
/*                                                                                                */
/*                                             MISC                                               */
/*                                                                                                */
/**************************************************************************************************/

void CollectionWindow::InfoDialog (void)
{
   BOOL wasPub = collection->Publishing();

   if (ColInfoDialog(this, &collection->Info, IsLocked()))
   {
      collection->WriteInfo();
      if (wasPub != collection->Publishing())
         gameListArea->TogglePublishing();
   }
} /* CollectionWindow::InfoDialog */


void CollectionWindow::Renumber (void)
{
   if (collection->inxField != inxField_GameNo || ! collection->ascendDir)
      NoteDialog(this, "Move/Renumber", "The game list must be sorted by ascending �Game #� before you can move/renumber games...");
   else if (collection->GetGameCount() > collection->View_GetGameCount())
      NoteDialog(this, "Move/Renumber", "You must turn off the collection filter before you can move/renumber games...");
   else if (gameWinList.Count() > 0)
      NoteDialog(this, "Move/Renumber", "You cannot move/renumber games in a collection where games are currently open...");
   else
   {
      LONG gfrom = gameListArea->GetSelStart();
      LONG count = gameListArea->GetSelCount();
      LONG gto;

      if (MoveGamesDialog(gfrom, count, collection->Info.gameCount, &gto))
      {  collection->Move(gfrom, gto, count);
         gameListArea->SetSelection(gto, gto + count - 1);
      }
   }
} /* CollectionWindow::Renumber */


void CollectionWindow::PrintCollection (void)
{
   if (collection->View_GetGameCount() == 0) return;

   LONG selStart = gameListArea->GetSelStart();
   LONG selEnd   = gameListArea->GetSelEnd();

   CHAR title[cwindow_MaxTitleLen + 1];
   GetTitle(title);
   CGamePrint *gamePrint = new CGamePrint(title);
   gamePrint->PrintCollection(collection, selStart, selEnd);
   delete gamePrint;
} /* CollectionWindow::PrintCollection */


void CollectionWindow::ExportHTML (void)
{
   HTMLGifReminder(this);

   CFile *htmlFile = new CFile();

   if (htmlFile->SaveDialog("Export HTML", ".html"))
   {
      if (htmlFile->saveReplace)
         htmlFile->Delete();
   
      htmlFile->SetCreator('ttxt');
      htmlFile->SetType('TEXT');
      htmlFile->Create();

      LONG selStart = gameListArea->GetSelStart();
      LONG selEnd   = gameListArea->GetSelEnd();

      CHAR title[cwindow_MaxTitleLen + 1];
      GetTitle(title);
      CExportHTML *html = new CExportHTML(title, htmlFile);
      html->ExportCollection(collection, selStart, selEnd);
      htmlFile->CompleteSave();
      delete html;
   }

   delete htmlFile;
} /* CollectionWindow::ExportHTML */

/*------------------------------------------ Busy Handling ---------------------------------------*/
// When performing a time consuming operation such as PGN import/export, sorting, filtering etc.
// we need to disable the collection window and menu.

void CollectionWindow::SetBusy (BOOL isBusy)
{
   sigmaApp->responsive = ! isBusy;

   if (busy == isBusy) return;
   busy = isBusy;

   AdjustCollectionMenu();
   AdjustToolbar();
   gameListArea->Enable(! busy);
} /* CollectionWindow::SetBusy */
