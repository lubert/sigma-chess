/**************************************************************************************************/
/*                                                                                                */
/* Module  : SIGMAPPLICATION.C                                                                    */
/* Purpose : This is the main application module, which creates, starts and terminates the Sigma  */
/*           Chess Application.                                                                   */
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

#include "SigmaApplication.h"

#include "SigmaStrings.h"
#include "SigmaLicense.h"
#include "PrefsDialog.h"
#include "StrengthDialog.h"
#include "ELOCalcDialog.h"
#include "GameWindow.h"
#include "CollectionWindow.h"
#include "PGNFile.h"
#include "BoardView.h"
#include "Engine.f"
#include "Annotations.h"
#include "ExaChessGlue.h"
#include "BoardArea3D.h"
#include "PosLibrary.h"
#include "TransTabManager.h"
#include "UCI.h"
#include "UCI_AppleEvents.h"
#include "UCI_ConfigDialog.h"

#include "Debug.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       THE MAIN ROUTINE                                         */
/*                                                                                                */
/**************************************************************************************************/

SigmaApplication *sigmaApp = nil;

void main (void)
{
   Task_Begin(sigmaTaskCount);

   sigmaApp = new SigmaApplication();
   sigmaApp->ShowAboutDialog(true);
   sigmaApp->Run();
   delete sigmaApp;

   Task_End();
} /* main */


/**************************************************************************************************/
/*                                                                                                */
/*                             APPLICATION CONSTRUCTOR/DESTRUCTOR                                 */
/*                                                                                                */
/**************************************************************************************************/

SigmaApplication::SigmaApplication (void)
 : CApplication(sigmaAppName, sigmaCreator)
{
   if (! VerifySystem()) Abort();

   // Load prefs file and check license info:
   sigmaApp = this;
   sigmaPrefs = new SigmaPrefs();
   wasFront = nil;
   if (EqualStr(Prefs.General.playerName, "debug"))
      debugOn = true;

   colWinImportTarget = nil;
   postponedOpenFileCount = 0;

   // Initialize engine:
   PTR kpkData = nil;
   HANDLE kpkHand = nil;
   if (Res_Load('KPK ', 1000, &kpkHand) == fileError_NoError)
   {  Mem_LockHandle(kpkHand);
      kpkData = *kpkHand;
   }

   Engine_InitSystem(&Global, kpkData);

   if (kpkData)
   {  Mem_UnlockHandle(kpkHand);
      Res_Free(kpkHand);
   }

   // Initialize rest
   InitGameModule();
   InitAnnotationModule();
   InitBoard3DModule();
   InitBmpUtilModule();

   // Load menus
   BuildMenus();

   // Check we're not running on a read-only volume
   FSSpec logFile;
   ::FSMakeFSSpec(0,0,"\p__sigma__.log", &logFile);
   ::FSpDelete(&logFile);
   if (::FSpCreate(&logFile,'????','TEXT',smSystemScript))
   {  NoteDialog(nil, "Failed Starting Sigma Chess", "Sigma Chess cannot run from read-only volumes or folders. Please copy the 'Sigma 6.2 HIARCS 13' folder to your hard drive first and then run the application from there...", cdialogIcon_Error, "Quit");
      Abort();
   }
   ::FSpDelete(&logFile);

   // Finally allocated and initialize the transposition tables. A single memory block is allocated,
   // which is then later sub-allocated.
   // NOTE: Does NOT work the same way in Classic and OSX:
   // 1) In Classic : All but the reserved memory will be allocated (based on Finder memory setup).
   // 2) OS X       : The size is determined by the Prefs.trans.totalTransSize parameter (default 20 MB)

   TransTab_Init();

   UCI_AEQuitLoader();  // Quit any previous instances of the UCI Loader.
} /* SigmaApplication::SigmaApplication */


SigmaApplication::~SigmaApplication (void)
{
   sigmaPrefs->Save();
   delete sigmaPrefs;

   UCI_AEQuitLoader();
} /* SigmaApplication::~SigmaApplication */

/*-------------------------------------- Verify System -------------------------------------------*/
// At start we first need to check if Sigma Chess runs on a compatible system:
// * System 7.5 or later (done by CApplication)
// * At least XXXX MB RAM
// * At least 256 colors/grays
// * At least 800x600 resolution

BOOL SigmaApplication::VerifySystem (void)
{
   if (RunningOSX()) return true;

   ULONG bytes = Mem_FreeBytes()/1024;

   if (Mem_FreeBytes()/1024 < minTotalMem - 400)
   {
      NoteDialog(nil, "Failed Starting Sigma Chess", "Not enough memory. Sigma Chess needs a minimum of 2.5 MB memory in order to run...", cdialogIcon_Error, "Quit");
      return false;
   }

   if (ScreenRect().Width() < 800 || ScreenRect().Height() < 600)
   {
      NoteDialog(nil, "Failed Starting Sigma Chess", "The screen is too small. Sigma Chess needs a minimum resolution of 800 x 600...", cdialogIcon_Error, "Quit");
      return false;
   }

   return true;
} /* SigmaApplication::VerifySystem */


/**************************************************************************************************/
/*                                                                                                */
/*                                       EVENT HANDLING                                           */
/*                                                                                                */
/**************************************************************************************************/

void SigmaApplication::MainLooper (void)
{
   ProcessEvents();
   ProcessEngineMessages();
   Task_Switch();
} /* SigmaApplication::MainLooper */

/*--------------------------------------- Engine Events ------------------------------------------*/
// Called periodically in main application task: Reads and dispatches engine events
// to the relevant wins

void SigmaApplication::ProcessEngineMessages (void)
{
   if (! Global.msgBitTab) return;  // If no engines have posted messages -> return

   for (INT i = 0; Global.msgBitTab; i++)
      if (Global.msgBitTab & bit(i))
      {
         ((GameWindow*)(Global.Engine[i]->refID))->ProcessEngineMessage();
         Global.msgBitTab ^= bit(i);  // Clear message table bit
      }
} /* SigmaApplication::ProcessEngineMessages */

/*---------------------------- Install Custom Apple Event Handlers -------------------------------*/

void SigmaApplication::HandleAEInstall (void)
{
   CApplication::HandleAEInstall();
   InitExaChess(); // Install ExaChess Apple Event handler (the 'CHES') event.
} /* SigmaApplication::HandleAEInstall */

/*-------------------------------- Application Launch Handling -----------------------------------*/

void SigmaApplication::HandleLaunch (void)
{
   BOOL firstLaunch = Prefs.firstLaunch;  // We need to save this because it will be cleared by the license dialog

   if (Prefs.firstLaunch)
      SigmaLicenseDialog();
   else if (ProVersion())
      VerifyLicense();

   UCI_InitModule(firstLaunch);

   PosLib_AutoLoad();

   HandlePostponedOpenFiles();
   
   if (! GetFrontWindow() || GetFrontWindow()->IsDialog())
      HandleMessage(file_NewGame);

   if (debugOn) DebugCreate();
} /* SigmaApplication::HandleLaunch */


BOOL SigmaApplication::HandleQuitRequest (void)
{
   if (ModalLoopRunning())
   {  Beep(1);
      return false;
   }
   if (ProVersion() && ! PosLib_CheckSave("Save before quitting?")) return false;
   if (! CApplication::HandleQuitRequest()) return false;

   // Application specific clean up here (just before quitting)
   UCI_QuitActiveEngine();
   return true;
} /* SigmaApplication::HandleQuitRequest */

/*--------------------------------- Generic Message Handling -------------------------------------*/

// All application wide command handling is performed here. Other commands are sent to the front
// window (by returning "false", meaning that the command was not handled here).

BOOL SigmaApplication::HandleMessage (LONG msg, LONG submsg, PTR data)
{
   switch (msg)
   {
      //--- FILE Menu ---

      case file_NewGame :
         NewGameWindow("<Untitled Game>", true);
         break;
      case file_NewCollection :
         NewCollectionWindow();
         break;
      case file_NewLibrary :
         PosLib_New();
         break;
      case file_Open :
         OpenDoc();
         break;
      case file_OpenGame :
         OpenDoc(fileFormat_Game);
         break;
      case file_PageSetup :
         PageSetupDialog();
         break;
      case file_Preferences :
         PrefsDialog();
         break;
      case file_Quit :
         Quit();
         break;

      //--- ANALYZE Menu ---

      //--- ENGINE Menu ---

      //--- LEVEL Menu ---

      case level_NonDeterm :
         sigmaPrefs->SetNonDeterm(! Prefs.Level.nonDeterm);
         break;
      case level_PlayerELO :
         PlayerRatingDialog();
         break;
      case level_ELOCalc :
         RatingCalculatorDialog();
         break;

      //--- DISPLAY Menu ---

      case notation_Short :
         sigmaPrefs->SetNotation(moveNot_Short);
         break;
      case notation_Long :
         sigmaPrefs->SetNotation(moveNot_Long);
         break;
      case notation_Descr :
         sigmaPrefs->SetNotation(moveNot_Descr);
         break;
      case notation_Figurine :
         sigmaPrefs->SetFigurine(! Prefs.Notation.figurine);
         break;

      case display_ShowFutureMoves :
         Prefs.Games.showFutureMoves = ! Prefs.Games.showFutureMoves;
         BroadcastMessage(msg_RefreshGameMoveList);
         break;
      case display_HiliteCurrMove :
         Prefs.Games.hiliteCurrMove = ! Prefs.Games.hiliteCurrMove;
         BroadcastMessage(msg_RefreshMoveNotation);
         break;

      //--- LIBRARY Menu ---

      case library_Name :
         if (PosLib_Loaded())
         {  if (QuestionDialog(nil, "Position Library", "This menu command shows the name of the currently loaded position library. Do you want to load another library?", "Yes", "No"))
               OpenDoc();
         }
         else
         {  if (QuestionDialog(nil, "Position Library", "No position library is currently loaded. Do you want to load a library?", "Yes", "No"))
               OpenDoc();
         }         
         break;
      case librarySet_Disabled :
         sigmaPrefs->EnableLibrary(! Prefs.Library.enabled);
         break;
      case librarySet_Solid :
      case librarySet_Tournament :
      case librarySet_Wide :
      case librarySet_Full :
         sigmaPrefs->SetLibraryAccess((LIB_SET)(msg - librarySet_Disabled));
         break;

      case library_AutoClassify :
         Prefs.Library.autoClassify = (LIB_AUTOCLASS)submsg;
         BroadcastMessage(msg_RefreshPosLib);
         break;
      case library_Save :
         if (! ProVersionDialog(nil, "Saving is disabled for position libraries in Sigma Chess Lite.")) return true;
         PosLib_Save();
         HandleMenuAdjust();
         break;
      case library_SaveAs :
         if (! ProVersionDialog(nil, "Saving is disabled for position libraries in Sigma Chess Lite.")) return true;
         PosLib_SaveAs();
         HandleMenuAdjust();
         break;

      //--- WINDOW Menu ---

      case window_CloseAll :
         CWindow *win;
         while ((win = GetFrontWindow()) && win->HandleCloseRequest())
            delete win;
         break;
      case window_Minimize :
         if (GetFrontWindow())
            GetFrontWindow()->Collapse();
         break;
      case window_Cycle :
         CycleWindows(true);
         break;

      //--- DEBUG Menu ---
/*
#ifdef __with_debug
      case debug_Window :
         DebugCreate();
         break;
      case debug_A : DebugA(); break;
      case debug_B : DebugB(); break;
      case debug_C : DebugC(); break;
#endif
*/
      default :
         if (msg >= window_WinFirst && msg <= window_WinLast)
            SelectWindow(msg);
         else if (msg >= pieceSet_First && msg <= pieceSet_Last + PieceSetPluginCount())
            sigmaPrefs->SetPieceSet(msg - pieceSet_First);
         else if (msg >= boardType_First && msg <= boardType_Last + BoardTypePluginCount())
            sigmaPrefs->SetBoardType(msg - boardType_First);
         else if (msg >= pieceLetters_First && msg <= pieceLetters_Last)
            sigmaPrefs->SetPieceLetters(msg - pieceLetters_First);
         else if (msg >= colorScheme_First && msg <= colorScheme_Last)
            sigmaPrefs->SetColorScheme(msg - colorScheme_First);
         else if (msg >= playingStyle_Chicken && msg <= playingStyle_Desperado)
            sigmaPrefs->SetPlayingStyle(msg - playingStyle_Chicken + style_Chicken);
         else if (msg >= moveMarker_Off && msg <= moveMarker_LastMove)
            sigmaPrefs->SetMoveMarker(msg - moveMarker_Off);
         else
            return false;          // The remaining events are not handled here.
   }
   return true;
} /* SigmaApplication::HandleMessage */


void SigmaApplication::HandleActivate (BOOL activated)
{
   if (! activated) wasFront = GetFrontWindow();

   sigmaApp->winList.Scan();
   while (SigmaWindow *win = (SigmaWindow*)sigmaApp->winList.Next())
      if (! win->IsDialog() && win->winClass == sigmaWin_Game && ((GameWindow*)win)->mode3D)
      {  ((GameWindow*)win)->Show(activated);
         if (activated && win == wasFront) ((GameWindow*)win)->SetFront();
      }
} /* SigmaApplication::HandleActivate */

/*--------------------------------------- Miscellaneous ------------------------------------------*/

BOOL SigmaApplication::WindowTitleUsed (CHAR *s, BOOL defaultPrompt)
{
   winList.Scan();
   while (CWindow *win = (CWindow*)(winList.Next()))
      if (! win->IsDialog() && win->hasFile)
      {
         CHAR title[cwindow_MaxTitleLen + 1];
         win->GetTitle(title);

         if (SameStr(s,title))
         {
            if (defaultPrompt)
            {
               CHAR prompt[400];
               Format(prompt, "Another document with the name Ò%sÓ is already open. It is not possible to open two documents with the same name...", s);
               NoteDialog(nil, "Document Already Open", prompt);
            }
            return true;
         }
      }

   return false;
} /* SigmaApplication::WindowTitleUsed */


void SigmaApplication::BroadcastMessage (LONG msg, LONG submsg, PTR data)
{
   winList.Scan();
   while (CWindow *win = (CWindow*)(winList.Next()))
      win->HandleMessage(msg, submsg, data);
} /* SigmaApplication::BroadcastMessage */


/**************************************************************************************************/
/*                                                                                                */
/*                                     GENERIC FILE HANDLING                                      */
/*                                                                                                */
/**************************************************************************************************/

// The "File"->"Open..." command supports all the Sigma Chess file types (i.e. both games,
// collections and libraries). The "SigmaOpenDialog" class implements the "Filter"
// for these file types:
//
// '·GM5' : The new · Compressed game format used in version 5 and later versions.
// '·GMX' : The · Extended game format used in version 3 and 4.
// '·GAM' : 'XLGM' : 'CHGM' : The version 2 and earlier game format.
// 'TEXT' : The PGN/EPD format.
//
// '·GC5' : The · Collection/Database format used in version 5 and later.
// '·GCX' : The · Collection format used in version 4. The actual games are stored
//          in the data fork, whereas the index and misc information is stored in the resource
//          fork.
//
// '·LB5' : The hash tables based opening library format used in version 5.
// '·LB6' : The new hash tables based opening library format with classifications used in version 6.
// '·LBX' : The · Opening Library format used in version 3 and 4.
// '·LIB' : The · Opening Library format used in 2 and earlier.

/*------------------------------------- Open File Dialog -----------------------------------------*/

class SigmaOpenDialog : public CFileOpenDialog
{
public:
   SigmaOpenDialog (void);
   virtual BOOL Filter (OSTYPE fileType, CHAR *name);
};


SigmaOpenDialog::SigmaOpenDialog (void)
  : CFileOpenDialog ()
{
} /* SigmaOpenDialog::SigmaOpenDialog */


BOOL SigmaOpenDialog::Filter (OSTYPE fileType, CHAR *fileName)
{
   switch (fileType)
   {
      case '·GM5' : return (currFormat == fileFormat_All || currFormat == fileFormat_Game); 
      case '·GMX' : return (currFormat == fileFormat_All || currFormat == fileFormat_Game34);
      case '·GAM' :
      case 'XLGM' :
      case 'CHGM' : return (currFormat == fileFormat_All || currFormat == fileFormat_Game2);

      case 'TEXT' : if (! IsPGNFileName(fileName) && Prefs.PGN.fileExtFilter) return false;
                    return (currFormat == fileFormat_All || currFormat == fileFormat_PGN);
      case '·GC5' :
      case '·GCX' : return (currFormat == fileFormat_All || currFormat == fileFormat_Col);

      case '·LB6' : return (currFormat == fileFormat_All || currFormat == fileFormat_Lib);
      case '·LB5' : return (currFormat == fileFormat_All || currFormat == fileFormat_Lib5);
      case '·LBX' : return (currFormat == fileFormat_All || currFormat == fileFormat_LibOld);
      default     : if (! IsPGNFileName(fileName)) return false;
                    return (currFormat == fileFormat_All || currFormat == fileFormat_PGN);
   }
} /* SigmaOpenDialog::Filter */

/*------------------------------------- Open File Handling ---------------------------------------*/

void SigmaApplication::OpenDoc (INT fileFormatItem)
{
   FILE_FORMAT FormatTab[11];

   for (INT i = 0; i < 11; i++)
   {  FormatTab[i].id = i + 1;
      CopyStr(GetStr(sgr_FileOpenMenu,i), FormatTab[i].text);
   }

   SigmaOpenDialog *dlg = new SigmaOpenDialog();
   dlg->Run(nil, "Open Sigma Chess Document", fileFormatItem, 11, FormatTab);
   delete dlg;
} /* SigmaApplication::OpenDoc */


// NOTE: The caller is responsible for deleting the file object.

void SigmaApplication::HandleOpenFile (CFile *file)
{
   if (Prefs.firstLaunch) return;

   if (! theApp->running && postponedOpenFileCount < kMaxPostponedOpenFile)
   {
   	  postponedOpenFile[postponedOpenFileCount++] = new CFile(file);
   	  return;
   }

   // First check if file is already open:

   if (WindowTitleUsed(file->name,true) && ! colWinImportTarget)
      return;

   // If not, then dispatch on the file type and open the document:

   CFile *sfile = new CFile();
   sfile->Set(file);

   switch (sfile->fileType)
   {
      case '·GM5' :                  // Version 5.0 games
      case '·GMX' :                  // Version 3.0 & 4.0 games
      case '·GAM' :                  // Version 2.0 games
      case 'CHGM' : case 'XLGM' :    // Version 1.3 games (and ExSel Chess)
         ::OpenGameFile(sfile); break;

      case '·GC5' : case '·GCX' :
         ::OpenCollectionFile(sfile); break;

      case 'TEXT' :
         if (colWinImportTarget)
            colWinImportTarget->ImportPGNFile(sfile);
         else
            ::OpenPGNFile(sfile);
         break;

      case '·LB6' : case '·LB5' : case '·LBX' : case '·LIB' :
         PosLib_Open(sfile, true); break;

      case '·GCF' : break;  // Ignore collection filter files here (can only be opened from filter dialog).

      case 'pref' : break;  // Ignore if prefs file opened!

      default :
         if (! IsPGNFileName(sfile->name))
            NoteDialog(nil, "Open", "Unknown file format...", cdialogIcon_Error);
         else
         {  sfile->fileType = 'TEXT'; //SetType('TEXT');
            if (colWinImportTarget)
               colWinImportTarget->ImportPGNFile(sfile);
            else
               ::OpenPGNFile(sfile);
         }
   }
} /* SigmaApplication::HandleOpenFile */


void SigmaApplication::HandlePostponedOpenFiles (void)
{
   for (INT i = 0; i < postponedOpenFileCount; i++)
   {
      HandleOpenFile(postponedOpenFile[i]);
      delete postponedOpenFile[i];
   }

   postponedOpenFileCount = 0;

} /* SigmaApplication::HandlePostponedOpenFiles */


/**************************************************************************************************/
/*                                                                                                */
/*                                      GENERIC WINDOW HANDLING                                   */
/*                                                                                                */
/**************************************************************************************************/

// Sigma Chess supports 3 window types: Games, collections and libraries. Whenever these windows
// are created, they should be registered to the application object. This will among other things
// build the windows menu.

void SigmaApplication::HandleWindowCreated (CWindow *win)
{
// RebuildWindowMenu();
} /* SigmaApplication::HandleWindowCreated */


void SigmaApplication::HandleWindowDestroyed (CWindow *win)
{
   RebuildWindowMenu();
} /* SigmaApplication::HandleWindowDestroyed */


void SigmaApplication::RebuildWindowMenu (void)
{
   // First delete "Window" menu and "Game->Add to Collection" submenu:

   if (windowMenu)
   {  RemoveMenu(windowMenu);
      delete windowMenu;
      windowMenu = nil;
   }

   if (debugMenu)
   {  RemoveMenu(debugMenu);
      delete debugMenu;
      debugMenu = nil;
   }

   if (addToColMenu)
   {  gameMenu->ClrSubMenu(game_AddToCollection);
      delete addToColMenu;
      addToColMenu = nil;
   }

   // Create the new "Windows" menu by first adding the fixed menu items:

   INT g = sgr_WindowMenu;
   windowMenu = new SigmaMenu(GetStr(g,0));
   windowMenu->AddItem(GetStr(g,1), window_CloseAll, 'W', cMenuModifier_Option);
   windowMenu->SetIcon(window_CloseAll, (RunningOSX() ? icon_CloseX : icon_Close), true);
   windowMenu->EnableMenuItem(window_CloseAll, winList.Count() > 0);

   if (RunningOSX())
   {  windowMenu->AddItem(GetStr(g,2), window_Minimize, 'M');
      windowMenu->SetIcon(window_Minimize, icon_MinimizeX, true);
      windowMenu->EnableMenuItem(window_Minimize, winList.Count() > 0);
   }

   windowMenu->AddItem(GetStr(g,3), window_Cycle, kTabCharCode, cMenuModifier_Option | cMenuModifier_NoCmd);
   windowMenu->SetIcon(window_Cycle, icon_CycleWindows, false);
   windowMenu->SetGlyph(window_Cycle, kMenuTabRightGlyph);
   windowMenu->EnableMenuItem(window_Cycle, winList.Count() > 1);

   // Next scan the window list for Sigma Windows (Game/Collection/Libraries) and add these to
   // the windows menu. Additionally add any collection windows to the "Game->Add to Collection"
   // submenu:

   if (winList.Count() > 0)
   {
      windowMenu->AddSeparator();
      
      INT item  = window_WinFirst;
      INT citem = game_AddToColFirst;
      CHAR shortCut = '0';

      winList.Scan();
      while (SigmaWindow *win = (SigmaWindow*)winList.Next())
         if (! win->IsDialog())
         {
            CHAR wtitle[cwindow_MaxTitleLen + 1];
            win->GetTitle(wtitle);

            INT iconID = cMenu_BlankIcon;
            switch (win->winClass)
            {  case sigmaWin_Game       : iconID = icon_Game; break;
               case sigmaWin_Collection : iconID = icon_Col; break;
               case sigmaWin_Library    : iconID = icon_Lib; break;
            }

            windowMenu->AddItem(wtitle, item, shortCut);
            windowMenu->SetIcon(item, iconID, true);

            if (shortCut >= '0' && shortCut < '9') shortCut++;
            else shortCut = cMenu_NoShortCut;
            item++;

            if (win->winClass == sigmaWin_Collection)
            {
               if (! addToColMenu)
                  addToColMenu = new CMenu("");
               addToColMenu->AddItem(wtitle, citem, (citem == game_AddToColFirst ? 'K' : cMenu_NoShortCut), cMenuModifier_None, icon_Col);
               citem++;
            }
         }
   }

   if (addToColMenu)
      gameMenu->SetSubMenu(game_AddToCollection, addToColMenu);

   AddMenu(windowMenu);

   BuildDebugMenu();
} /* SigmaApplication::RebuildWindowMenu */


void SigmaApplication::SelectWindow (INT winNo)
{
   INT item = window_WinFirst;

   winList.Scan();
   while (SigmaWindow *win = (SigmaWindow*)winList.Next())
      if (! win->IsDialog())
         if (item < winNo)
            item++;
         else
         {  if (! win->IsFront())
            {  win->SetFront();
//             ProcessEvents();
//             DrawMenuBar();
//             win->HandleMenuAdjust();
            }
            return;
         }
} /* SigmaApplication::SelectWindow */


CollectionWindow *SigmaApplication::GetColWindow (INT winNo)
{
   INT citem = game_AddToColFirst;

   winList.Scan();
   while (SigmaWindow *win = (SigmaWindow*)winList.Next())
      if (! win->IsDialog() && win->winClass == sigmaWin_Collection)
         if (citem < winNo)
            citem++;
         else
            return (CollectionWindow*)win;
   return nil;   //###Shouldn't get here!!
} /* SigmaApplication::GetColWindow */


/**************************************************************************************************/
/*                                                                                                */
/*                                          ABOUT DIALOG                                          */
/*                                                                                                */
/**************************************************************************************************/

class CAboutDialog : public CDialog
{
public:
   CAboutDialog (CHAR *title, CRect frame, BOOL launching);
   ~CAboutDialog(void);

   virtual void HandlePushButton (CPushButton *ctrl);

   CPushButton *cbutton_License, *cbutton_Register, *cbutton_Upgrade;
   CBitmap *bmp;
};

/*--------------------------------------- About Dialog -------------------------------------------*/

extern LONG gViewCount;

static BOOL aboutDlgOpen = false;

#define aboutBmpWidth  520
#define aboutBmpHeight 350

void SigmaApplication::HandleAbout (void)
{
   ShowAboutDialog();
} /* SigmaApplication::HandleAbout */


void SigmaApplication::HandleShowPrefs (void)
{
   PrefsDialog();
} /* SigmaApplication::HandleShowPrefs */


void SigmaApplication::ShowAboutDialog (BOOL launching)
{
   if (aboutDlgOpen) return;

   aboutDlgOpen = true;

   CHAR title[50];
   Format(title, "%s %s %s", (launching ? "Welcome to" : "About"), sigmaAppName, (ProVersion() ? "" : "Lite"));

   CRect frame(0, 0, aboutBmpWidth + 10, aboutBmpHeight + 45);
   if (RunningOSX()) frame.right += 10, frame.bottom += 20;
   theApp->CentralizeRect(&frame, true);
   CAboutDialog *dialog = new CAboutDialog(title, frame, launching);
   dialog->Show(true);

   if (! launching)
      dialog->Run();
   else
   {  LONG t = Timer() + 100;
      while (Timer() < t)
         ProcessEvents(evt_All - evt_HighLevel);
   }

   delete dialog;

   aboutDlgOpen = false;
} /* SigmaApplication::ShowAboutDialog */


CAboutDialog::CAboutDialog (CHAR *title, CRect frame, BOOL launching)
 : CDialog (nil, title, frame, cdialogType_Modal) //(launching && RunningOSX() ? cdialogType_Plain : cdialogType_Modal))
{
   bmp = new CBitmap(7000, 16);
   CRect r = bmp->bounds;
   if (RunningOSX()) r.Offset(10, 10); else r.Offset(5,5);
   new CBitmapControl(this, bmp, r);
/*
   if (! ProVersion())
   {
      r = InnerRect(); r.top = r.bottom - controlHeight_Text - 1;
      if (RunningOSX()) r.left = 10, r.top--;
      r.right = r.left + 100;
      r.Offset(0, 5);
      CTextControl *ctext = new CTextControl(this, "NOT REGISTERED", r, true, controlFont_Views);
      ctext->SetFontStyle(fontStyle_Bold);
   }
   else
   {
      r = InnerRect(); r.top = r.bottom - 24;
      if (RunningOSX()) r.left = 10, r.right = 110;
      else r.right = r.left + 84;
      CTextControl *ctext1 = new CTextControl(this, "Registered to", r, true, controlFont_Views); r.Offset(0, 15);
      ctext1->SetFontStyle(fontStyle_Bold);
      CTextControl *ctext2 = new CTextControl(this, "Serial No", r, true, controlFont_Views);
      ctext2->SetFontStyle(fontStyle_Bold);

      r = InnerRect(); r.top = r.bottom - 24; r.left += 85; r.right = r.left + 150;
      new CTextControl(this, Prefs.license.ownerName, r, true, controlFont_Views); r.Offset(0, 15);
      new CTextControl(this, Prefs.license.serialNo, r, true, controlFont_Views);
   }
*/
   r = InnerRect();
   if (RunningOSX()) r.right = Bounds().right - 10;
   r.left = r.right - 90; r.top = r.bottom - controlHeight_PushButton;
   r.Offset(-100*(ProVersion() ? 1 : 3), 0);
//   if (RunningOSX()) r.left -= 15;
   cbutton_License  = new CPushButton(this, "License", r, ! launching); r.Offset(100, 0);
//   if (RunningOSX()) r.left += 15;

   if (! ProVersion())
   {  cbutton_Register = new CPushButton(this, "Register", r, ! launching); r.Offset(100, 0);
      cbutton_Upgrade  = new CPushButton(this, "Upgrade...", r, ! launching); r.Offset(100, 0);
   }
   else
   {  cbutton_Register = cbutton_Upgrade = nil;
   }

   cbutton_Default  = new CPushButton(this, "Close", r, ! launching);
   if (! launching) SetDefaultButton(cbutton_Default);

   CurrControl(cbutton_License);
} /* CAboutDialog::CAboutDialog */


CAboutDialog::~CAboutDialog (void)
{
   delete bmp;
} /* CAboutDialog::~CAboutDialog */


void CAboutDialog::HandlePushButton (CPushButton *ctrl)
{
   if (ctrl == cbutton_License)
      SigmaLicenseDialog();
   else if (ctrl == cbutton_Register)
      SigmaRegisterDialog();
   else if (ctrl == cbutton_Upgrade)
      SigmaUpgradeDialog();

   CDialog::HandlePushButton(ctrl);
} /* CAboutDialog::HandlePushButton */


/**************************************************************************************************/
/*                                                                                                */
/*                                        MISCELLANEOUS                                           */
/*                                                                                                */
/**************************************************************************************************/

void SigmaApplication::PlayMoveSound (BOOL isCapture)
{
   if (! Prefs.Sound.woodSound) return;
   if (isCapture) PlaySound(moveSound_Capture, false);
   PlaySound(moveSound_Normal, false);
} /* SigmaApplication::PlayMoveSound */


BOOL SigmaApplication::CheckMemFree (ULONG kbNeeded, BOOL showDialog)
{
   if (RunningOSX()) return true;

   ::PurgeMem(kbNeeded*1024); //###
   ::CompactMem(kbNeeded*1024); //###
   if (Mem_MaxBlockSize()/1024 > kbNeeded &&
       Mem_FreeBytes()/1024 > kbNeeded + minReserveMem) return true;
   return (showDialog ? MemErrorDialog() : false);
} /* SigmaApplication::CheckMemError */


BOOL SigmaApplication::MemErrorDialog (void)
{
   NoteDialog(nil, "Out of Memory", "There was not enough memory to complete this operation. \
Try closing some windows first or assign more memory to Sigma Chess (from the Finder Info or \
the Sigma Memory preferences).", cdialogIcon_Error);
   return false;
} /* SigmaApplication::MemErrorDialog */


BOOL SigmaApplication::InternalError (CHAR *message)
{
   ::NoteDialog(nil, "Internal Error", message, cdialogIcon_Error);
   return false;
} /* SigmaApplication::InternalError */


BOOL SigmaApplication::CheckWinCount (void)   // Should be called before creating new window
{
   INT maxWindows = (ProVersion() ? maxEngines : 3);
   if (sigmaApp->winList.Count() < maxWindows) return true;
   
   if (ProVersion())
      NoteDialog(nil, "Too many open Windows", "At most 10 windows can be opened (hey, that should be enough anyway!)", cdialogIcon_Error);
   else
      ProVersionDialog(nil, "At most 3 windows can be opened in Sigma Chess Lite.");
   return false;
} /* SigmaApplication::CheckWinCount */
