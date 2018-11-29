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

#pragma once

#include "SigmaClassLibrary.h"

#include "SigmaAppConstants.h"
#include "SigmaPrefs.h"
#include "SigmaMessages.h"
#include "SigmaIcons.h"
#include "Level.h"
#include "BmpUtil.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------- Miscellaneous Constants -----------------------------------*/

enum FILEOPEN_FORMATS    // Matches the "File Format" popup menu items in the "Open..." dialog.
{
   fileFormat_All     = 1,
   fileFormat_Sep1    = 2,
   fileFormat_Game    = 3,
   fileFormat_Col     = 4,
   fileFormat_Lib     = 5,
   fileFormat_PGN     = 6,
   fileFormat_Sep2    = 7,
   fileFormat_Game34  = 8,
   fileFormat_Game2   = 9,
   fileFormat_Lib5    = 10,
   fileFormat_LibOld  = 11
};


#define moveSound_Normal  1000
#define moveSound_Capture 1001

#define kMaxPostponedOpenFile 20


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class CollectionWindow;
class SigmaMenu;

class SigmaApplication: public CApplication
{
public:
   SigmaApplication (void);
   virtual ~SigmaApplication (void);

   virtual void MainLooper (void);

   BOOL VerifySystem (void);

   void BuildMenus (void);
   void BuildFileMenu (void);
   void BuildEditMenu (void);
   void BuildGameMenu (void);
   void BuildAnalyzeMenu (void);
   void BuildLevelMenu (void);
   void BuildDisplayMenu (void);
   void BuildCollectionMenu (void);
   void BuildLibraryMenu (void);
   void BuildWindowMenu (void);
   void BuildDebugMenu (void);

   void RebuildEngineMenu (void);
   CMenu *BuildPieceSetMenu (BOOL popup);
   CMenu *BuildBoardTypeMenu (BOOL popup);
   CMenu *BuildPlayingModeMenu (BOOL popup);
   CMenu *BuildPlayingStyleMenu (BOOL popup);
   void UpdateMenuIcons (void);

   void BroadcastMessage (LONG msg, LONG submsg = 0, PTR data = nil);

   virtual void HandleAEInstall (void);
   virtual void HandleLaunch (void);
   virtual BOOL HandleQuitRequest (void);
   virtual void HandleActivate (BOOL activated);
   virtual void HandleAbout (void);
   virtual void HandleShowPrefs (void);
   virtual BOOL HandleMessage (LONG msg, LONG submsg = 0, PTR data = nil);
   virtual void HandleMenuAdjust (void);
   virtual void HandleOpenFile (CFile *file);
   virtual void HandleWindowCreated (CWindow *theWin);
   virtual void HandleWindowDestroyed (CWindow *theWin);

   void ShowAboutDialog (BOOL launching = false);

   void RebuildWindowMenu (void);
   BOOL WindowTitleUsed (CHAR *s, BOOL defaultPrompt = false);
   void SelectWindow (INT winNo);
   CollectionWindow *GetColWindow (INT winNo);

   void OpenDoc (INT fileFormatItem = fileFormat_All);
   void HandlePostponedOpenFiles (void);

   void ProcessEngineMessages (void);
   void PlayMoveSound (BOOL isCapture);
   BOOL CheckMemFree (ULONG kbNeeded, BOOL showDialog = true);
   BOOL MemErrorDialog (void);
   BOOL InternalError (CHAR *message);
   BOOL CheckWinCount (void);

   // Main menus:
   SigmaMenu *fileMenu;
   SigmaMenu *editMenu;
   SigmaMenu *gameMenu;
   SigmaMenu *analyzeMenu;
   SigmaMenu *levelMenu;
   SigmaMenu *engineMenu;
   SigmaMenu *displayMenu;
   SigmaMenu *collectionMenu;
   SigmaMenu *libraryMenu;
   SigmaMenu *windowMenu;

   // Sub menus:
   CMenu *cutMenu, *copyMenu, *pasteMenu;
   CMenu *addToColMenu;
   CMenu *styleMenu;
   CMenu *pieceSetMenu, *boardTypeMenu, *boardSizeMenu,
         *moveMarkerMenu, *notationMenu, *pieceLettersMenu,
         *colorSchemeMenu,
         *libSetMenu, *classifyMenu, *autoClassifyMenu;
   CMenu *debugMenu;

   // Misc:
   CWindow *wasFront;
   CollectionWindow *colWinImportTarget;

	INT postponedOpenFileCount;
   CFile *postponedOpenFile[kMaxPostponedOpenFile];
};

class SigmaMenu : public CMenu
{
public:
   SigmaMenu (CHAR *title);

   void SetIcon (INT itemID, INT iconID, BOOL permanent = false);
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES                                 */
/*                                                                                                */
/**************************************************************************************************/

extern SigmaApplication *sigmaApp;
extern INT ModeIcon[playingModeCount + 1];


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/
