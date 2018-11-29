/**************************************************************************************************/
/*                                                                                                */
/* Module  : COLLECTIONWINDOWMENU.C                                                               */
/* Purpose : This module implements the collection window menus.                                  */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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
#include "LibImportDialog.h"
#include "GamePrint.h"
#include "PGN.h"
#include "PGNFile.h"
#include "PosLibrary.h"

#include "SigmaApplication.h"
#include "CDialog.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       MENU HANDLING                                            */
/*                                                                                                */
/**************************************************************************************************/

void CollectionWindow::HandleMessage (LONG msg, LONG submsg, PTR data)
{
   switch (msg)
   {
      //--- FILE Menu ---

      case file_Close :
         if (HandleCloseRequest()) delete this;
         break;

      case file_ExportHTML :
         ExportHTML();
         break;

      case file_Print :
         PrintCollection();
         break;

      //--- EDIT Menu ---

      case edit_Clear :
         DeleteSelection();
         break;      
      case edit_SelectAll :
         gameListArea->SelectAll();
         break;

      case cut_Game :
         CutGames();
         break;
      case copy_Game :
         CopyGames();
         break;
      case paste_Game :
         PasteGames();
         break;

      //--- DISPLAY Menu ---

      case display_GameRecord :
         ::GameInfoFilterDialog(&(Prefs.GameDisplay.gameInfoFilter));
         break;

      case display_ToolbarTop :
         toolbarTop = ! toolbarTop;
         Prefs.ColDisplay.toolbarTop = toolbarTop;
         CalcFrames();
         toolbar->SetFrame(toolbarRect);
         gameListArea->SetFrame(gameListRect);
         AdjustDisplayMenu();
         Redraw();
         FlushPortBuffer();
         break;

      //--- ANALYZE Menu ---

      case analyze_AnalyzeCol :
         NoteDialog(this, "Analyze Collection", "You first need to open a game from the collection (the first game to be analyzed)...");
         break;

      //--- COLLECTION Menu ---

      case collection_EditFilter :
         CHAR colName[cwindow_MaxTitleLen + 1];
         GetTitle(colName);
         if (FilterDialog(colName, &collection->filter, collection->GetGameCount()))
         {  collection->useFilter = false;
            HandleMessage(collection_EnableFilter);
         }
         break;
      case collection_EnableFilter :
         SetBusy(true);
         collection->useFilter = ! collection->useFilter;
         collection->View_Rebuild();
         sigmaApp->collectionMenu->CheckMenuItem(collection_EnableFilter, collection->useFilter);
         gameListArea->ResetScroll();
         SetBusy(false);
         break;


      case collection_OpenGame :
         OpenGame(collection->View_GetGameNo(gameListArea->GetSel()));
         break;
      case collection_Layout :
         EditLayout(collection->View_GetGameNo(gameListArea->GetSel()));
         break;

      case collection_ImportPGN :
         ImportPGN();
         break;
      case collection_ExportPGN :
         ExportPGN(gameListArea->GetSelStart(), gameListArea->GetSelEnd());
         break;

      case collection_Renumber :
         Renumber();
         break;
      case collection_Compact :
         SetBusy(true);
         collection->Compact();
         SetBusy(false);
         break;
      case collection_Info :
         InfoDialog();
         break;

      //--- LIBRARY Menu ---

      case library_ImportCollection :
         CHAR colFileName[cwindow_MaxTitleLen + 1];
         GetTitle(colFileName);
         if (LibImportDialog(colFileName, &Prefs.Library.param))
         {  SetBusy(true);
            collection->PosLibImport(gameListArea->GetSelStart(), gameListArea->GetSelEnd(), &Prefs.Library.param);
            sigmaApp->BroadcastMessage(msg_RefreshPosLib);
            SetBusy(false);
         }
         break;

      //--- Progress Misc Events ---

      case colProgress_Begin :
         miniToolbar->BeginProgress((CHAR*)data, submsg);
         break;
      case colProgress_Set :
         miniToolbar->SetProgress((CHAR*)data, submsg);
         gameListArea->DrawFooter();
         break;
      case colProgress_End :
         miniToolbar->EndProgress();
         break;
      case msg_ColStop :
         collection->progressAborted = true;
         break;

      case msg_ColSelChanged :
         AdjustCollectionMenu();
         AdjustEditMenu();
         AdjustToolbar();
         break;
      case msg_RefreshColorScheme :
         gameListArea->Redraw();
         break;
   }
} /* CollectionWindow::HandleMessage */


/**************************************************************************************************/
/*                                                                                                */
/*                                              MISC                                              */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------------- Cut Games -----------------------------------------*/

void CollectionWindow::CutGames (void)
{
   if (CopyGames())
      DeleteSelection();
} /* CollectionWindow::CutGames */

/*-------------------------------------------- Copy Games ----------------------------------------*/

BOOL CollectionWindow::CopyGames (void)
{
   LONG selStart = gameListArea->GetSelStart();
   LONG selEnd   = gameListArea->GetSelEnd();

   if (selEnd - selStart >= 10000)
   {  NoteDialog(this, "Error", "At most 10.000 games can be copied to the clipboard. Try using PGN export/import instead...");
      return false;
   }

   CFile *clipFile = new CFile();
   clipFile->Set("clipboard.pgn", 'TEXT');
   if (clipFile->Exists()) clipFile->Delete();
   clipFile->Create();

   SetBusy(true);
      collection->ExportPGN(clipFile, selStart, selEnd);
   SetBusy(false);

   PTR   data  = nil;
   ULONG bytes = 0;
   BOOL  error = (clipFile->Load(&bytes, &data) != fileError_NoError);

   if (! error)
   {  theApp->ResetClipboard();
      error = (theApp->WriteClipboard('TEXT', data, bytes) != appErr_NoError);
   }

   if (error)
      NoteDialog(this, "Error", "Failed copying selected games to clipboard. Try closing some windows first or restart Sigma Chess.", cdialogIcon_Error);

   if (data) Mem_FreePtr(data);

   clipFile->Delete();
   delete clipFile;

   return ! error;
} /* CollectionWindow::CopyGames */

/*------------------------------------------- Paste Games ----------------------------------------*/

void CollectionWindow::PasteGames (void)
{
   PTR  data = nil;
   LONG bytes;

   if (theApp->ReadClipboard('TEXT', &data, &bytes) == appErr_NoError)
   {
      CFile *clipFile = new CFile();
      clipFile->Set("clipboard.pgn", 'TEXT');
      if (clipFile->Exists()) clipFile->Delete();
      clipFile->Create();
      clipFile->Save(bytes, data);
      if (data) Mem_FreePtr(data);

      SetBusy(true);
      if (collection->ImportPGN(clipFile))
      {  gameListArea->RefreshList();
         HandleMenuAdjust();
      }
      SetBusy(false);

      clipFile->Delete();
      delete clipFile;
   }
} /* CollectionWindow::PasteGames */


/**************************************************************************************************/
/*                                                                                                */
/*                                       WINDOW/MENU ACTIVATION                                   */
/*                                                                                                */
/**************************************************************************************************/

// If a Collection window is moved to the front, we have to update the menu state (enable, checks)
// accordingly. This method is also called after a window has been created:

void CollectionWindow::HandleMenuAdjust (void)
{
   if (! IsActive()) return;

   sigmaApp->EnableMenuBar(true);
   sigmaApp->ShowMenuBar(true);
   sigmaApp->EnableQuitCmd(true);   // OS X Menu enabling
   sigmaApp->EnablePrefsCmd(true);
   sigmaApp->EnableAboutCmd(true);

   AdjustFileMenu();
   AdjustEditMenu();
   AdjustGameMenu();
   AdjustAnalyzeMenu();
   AdjustLevelMenu();
   AdjustDisplayMenu();
   AdjustCollectionMenu();
   AdjustLibraryMenu();
   sigmaApp->RedrawMenuBar();
} /* CollectionWindow::HandleMenuAdjust */

/*--------------------------------- Adjusting Collection Window Menus ----------------------------------*/

void CollectionWindow::AdjustFileMenu (void)
{
   if (! IsFront()) return;

   CMenu *m = sigmaApp->fileMenu;
   m->EnableAllItems(true);
   m->EnableMenuItem(file_NewGame,       true);          // Always enabled
   m->EnableMenuItem(file_NewCollection, true);          // Always enabled
   m->EnableMenuItem(file_NewLibrary,    true);          // Always enabled
   m->EnableMenuItem(file_Open,          true);          // Always enabled
   m->EnableMenuItem(file_Save,          false);
   m->EnableMenuItem(file_SaveAs,        false);
   m->EnableMenuItem(file_Close,         true);
   m->EnableMenuItem(file_PageSetup,     true);          // Always enabled
   m->EnableMenuItem(file_ExportHTML,    collection->View_GetGameCount() > 0);
   m->EnableMenuItem(file_Print,         collection->View_GetGameCount() > 0);
   m->EnableMenuItem(file_Preferences,   true);          // Always enabled
   m->EnableMenuItem(file_Quit,          true);          // Always enabled
} /* CollectionWindow::AdjustFileMenu */


void CollectionWindow::AdjustEditMenu (void)
{
   if (! IsFront()) return;

   BOOL gamesSel = (collection->View_GetGameCount() > 0);
   BOOL locked = IsLocked();

   CMenu *m = sigmaApp->editMenu;
   m->EnableMenuItem(edit_Undo,             false);
   m->EnableMenuItem(edit_Redo,             false);
   m->EnableMenuItem(edit_Clear,            gamesSel && ! locked);
   m->EnableMenuItem(edit_SelectAll,        gamesSel);

   m = sigmaApp->cutMenu;
   m->EnableMenuItem(cut_Standard,          false);
   m->EnableMenuItem(cut_Game,              gamesSel && ! locked);
   m->ClrShortcut(cut_Standard);
   m->SetShortcut(cut_Game, 'X');

   m = sigmaApp->copyMenu;
   m->EnableMenuItem(copy_Standard,         false);
   m->EnableMenuItem(copy_Game,             gamesSel);
   m->EnableMenuItem(copy_GameNoAnn,        false);
   m->EnableMenuItem(copy_Position,         false);
   m->EnableMenuItem(copy_Analysis,         false);
   m->ClrShortcut(copy_Standard);
   m->ClrShortcut(copy_Position);
   m->ClrShortcut(copy_Analysis);
   m->ClrShortcut(copy_Game);
   m->SetShortcut(copy_Game, 'C');

   m = sigmaApp->pasteMenu;
   m->EnableMenuItem(paste_Standard,         false);
   m->EnableMenuItem(paste_Game,             ! locked);
   m->EnableMenuItem(paste_Position,         false);
   m->ClrShortcut(paste_Standard);
   m->ClrShortcut(paste_Position);
   m->SetShortcut(paste_Game, 'V');
} /* CollectionWindow::AdjustEditMenu */


void CollectionWindow::AdjustGameMenu (void)
{
   if (! IsFront()) return;

   CMenu *m = sigmaApp->gameMenu;
   m->EnableAllItems(false);
} /* CollectionWindow::AdjustGameMenu */


void CollectionWindow::AdjustAnalyzeMenu (void)
{
   if (! IsFront()) return;

   CMenu *m = sigmaApp->analyzeMenu;
   m->EnableMenuItem(analyze_Engine,        false);
   m->EnableMenuItem(analyze_Go,            false);
   m->EnableMenuItem(analyze_NextBest,      false);
   m->EnableMenuItem(analyze_Stop,          false);
   m->EnableMenuItem(analyze_Pause,         false);
   m->EnableMenuItem(analyze_Hint,          false);
   m->EnableMenuItem(analyze_PlayMainLine,  false);
   m->EnableMenuItem(analyze_DrawOffer,     false);
   m->EnableMenuItem(analyze_Resign,        false);
   m->EnableMenuItem(analyze_AutoPlay,      false);
   m->EnableMenuItem(analyze_DemoPlay,      false);
   m->EnableMenuItem(analyze_AnalyzeGame,   false);
   m->EnableMenuItem(analyze_AnalyzeCol,    true);
   m->EnableMenuItem(analyze_AnalyzeEPD,    false);
   m->EnableMenuItem(analyze_EngineMatch,   false);
   m->EnableMenuItem(analyze_TransTables,   false);
   m->EnableMenuItem(analyze_EndgameDB,     false);
} /* CollectionWindow::AdjustAnalyzeMenu */


void CollectionWindow::AdjustLevelMenu (void)
{
   if (! IsFront()) return;

   CMenu *m = sigmaApp->levelMenu;
   m->EnableMenuItem(level_Select,          false);
   m->EnableMenuItem(level_PlayingStyle,    false);
   m->EnableMenuItem(level_PermanentBrain,  false);
   m->EnableMenuItem(level_NonDeterm,       false);
   m->EnableMenuItem(level_SigmaELO,        false);
   m->EnableMenuItem(level_PlayerELO,       true);
   m->EnableMenuItem(level_ELOCalc,         true);

   m->SetIcon(level_Select, ModeIcon[Prefs.Level.level.mode]);
} /* CollectionWindow::AdjustLevelMenu */


void CollectionWindow::AdjustDisplayMenu (void)
{
   if (! IsFront()) return;

   CMenu *m = sigmaApp->displayMenu;
   m->EnableMenuItem(display_TurnBoard,     false);
   m->EnableMenuItem(display_PieceSet,      true);
   m->EnableMenuItem(display_BoardType,     true);
   m->EnableMenuItem(display_BoardSize,     false);
   m->EnableMenuItem(display_Notation,      true);
   m->EnableMenuItem(display_PieceLetters,  true);
   m->EnableMenuItem(display_ToggleInfoArea,false);
   m->EnableMenuItem(display_GameRecord,    false);
   m->EnableMenuItem(display_3DBoard,       false);
   m->EnableMenuItem(display_Show3DClock,   false);
   m->EnableMenuItem(display_ColorScheme,   true);
   m->EnableMenuItem(display_ToolbarTop,    true);

   // Set window specific checkmarks:
   m->CheckMenuItem(display_TurnBoard,  Prefs.GameDisplay.boardTurned);
   m->CheckMenuItem(display_3DBoard,    Prefs.GameDisplay.mode3D);
   m->CheckMenuItem(display_ToolbarTop, toolbarTop);

   m = sigmaApp->boardSizeMenu;
   m->CheckMenuItem(boardSize_Standard,   Prefs.Appearance.squareWidth == squareWidth1);
   m->CheckMenuItem(boardSize_Medium,     Prefs.Appearance.squareWidth == squareWidth2);
   m->CheckMenuItem(boardSize_Large,      Prefs.Appearance.squareWidth == squareWidth3);
   m->CheckMenuItem(boardSize_EvenLarger, Prefs.Appearance.squareWidth == squareWidth4);
} /* CollectionWindow::AdjustDisplayMenu */


void CollectionWindow::AdjustCollectionMenu (void)
{
   if (! IsFront()) return;

   LONG selCount = gameListArea->GetSelCount();
   
   CMenu *m = sigmaApp->collectionMenu;
   m->EnableMenuItem(collection_EditFilter,   ! busy && collection->GetGameCount() > 0);
   m->EnableMenuItem(collection_EnableFilter, ! busy && collection->GetGameCount() > 0);
   m->EnableMenuItem(collection_OpenGame,     ! busy && selCount == 1);
   m->EnableMenuItem(collection_PrevGame,     false);
   m->EnableMenuItem(collection_NextGame,     false);
   m->EnableMenuItem(collection_Layout,       ! busy && selCount == 1);
   m->EnableMenuItem(collection_ImportPGN,    ! busy && ! IsLocked());
   m->EnableMenuItem(collection_ExportPGN,    ! busy && selCount > 0);
   m->EnableMenuItem(collection_Compact,      ! busy && ! IsLocked() && collection->GetGameCount() > 0);
   m->EnableMenuItem(collection_Renumber,     ! busy && ! IsLocked() && selCount > 0);
   m->EnableMenuItem(collection_Info,         ! busy);

   m->CheckMenuItem(collection_EnableFilter, collection->useFilter);
} /* CollectionWindow::AdjustCollectionMenu */


void CollectionWindow::AdjustLibraryMenu (void)
{
   if (! IsFront()) return;

   CMenu *m = sigmaApp->libraryMenu;
   m->EnableMenuItem(library_Name,             true);
   m->EnableMenuItem(library_SigmaAccess,      PosLib_Loaded());
   m->EnableMenuItem(library_Editor,           false);
   m->EnableMenuItem(library_ECOComment,       false);
   m->EnableMenuItem(library_DeleteVar,        false);
   m->EnableMenuItem(library_ImportCollection, PosLib_Loaded());
   m->EnableMenuItem(library_Save,             PosLib_Loaded() && ! PosLib_Locked() && PosLib_Dirty());
   m->EnableMenuItem(library_SaveAs,           PosLib_Loaded());

   m->CheckMenuItem(library_Editor,        false);
} /* CollectionWindow::AdjustLibraryMenu */


void CollectionWindow::AdjustToolbar (void)
{
   if (toolbar)
      toolbar->Adjust();
} /* CollectionWindow::AdjustToolbar*/
