/**************************************************************************************************/
/*                                                                                                */
/* Module  : SIGMAMENUS.C                                                                         */
/* Purpose : This module sets up and manages the menus.                                           */
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
#include "PosLibrary.h"

//#define __with_debug


INT ModeIcon[playingModeCount + 1] = { 0, 281,280,282,289,288,284, 287,440,286, 403,291 };


/**************************************************************************************************/
/*                                                                                                */
/*                                     CUSTOM SIGMA MENU CLASS                                    */
/*                                                                                                */
/**************************************************************************************************/

// The purpose of this class is to override the "SetIcon" method, so icons are only set if selected
// in the prefs dialog.

SigmaMenu::SigmaMenu (CHAR *title)
 : CMenu(title)
{
} /* SigmaMenu::SigmaMenu */


void SigmaMenu::SetIcon (INT itemID, INT iconID, BOOL permanent)
{
   if (Prefs.General.menuIcons == 0)
      CMenu::ClrIcon(itemID);
   else if (Prefs.General.menuIcons == 1)
      CMenu::SetIcon(itemID, (permanent ? iconID : cMenu_BlankIcon));
   else
      CMenu::SetIcon(itemID, iconID);
} /* SigmaMenu::SetIcon */


/**************************************************************************************************/
/*                                                                                                */
/*                                        MENU CREATION                                           */
/*                                                                                                */
/**************************************************************************************************/

void SigmaApplication::BuildMenus (void)
{
   addToColMenu = nil;
   windowMenu   = nil;
   debugMenu    = nil;

   BuildFileMenu();
   BuildEditMenu();
   BuildGameMenu();
   BuildAnalyzeMenu();
   BuildLevelMenu();
   BuildDisplayMenu();
   BuildCollectionMenu();
   BuildLibraryMenu();
   BuildDebugMenu();
   BuildWindowMenu();
   RedrawMenuBar();

   // Now that all menus have been created -> Update checkmarks etc from prefs.
   sigmaPrefs->Apply();
   UpdateMenuIcons();

   // Finally adjust enabling:
   HandleMenuAdjust();
} /* SigmaApplication::BuildMenus */

/*------------------------------------------ File Menu -------------------------------------------*/

void SigmaApplication::BuildFileMenu (void)
{
   INT g = sgr_FileMenu;
   fileMenu = new SigmaMenu(GetStr(g,0));
   fileMenu->AddItem(GetStr(g,1),  file_NewGame, 'N');
   fileMenu->AddItem(GetStr(g,2),  file_NewCollection, 'N', cMenuModifier_Control);
   fileMenu->AddItem(GetStr(g,3),  file_NewLibrary);
   fileMenu->AddSeparator();
   fileMenu->AddItem(GetStr(g,4),  file_Open, 'O');
   fileMenu->AddItem(GetStr(g,6),  file_Save, 'S');
   fileMenu->AddItem(GetStr(g,7),  file_SaveAs, 'S', cMenuModifier_Shift);
   fileMenu->AddItem(GetStr(g,8),  file_Close, 'W');
   fileMenu->AddSeparator();
   fileMenu->AddItem(GetStr(g,9),  file_ExportHTML);
   fileMenu->AddSeparator();
   fileMenu->AddItem(GetStr(g,10),  file_PageSetup, 'P', cMenuModifier_Shift);
   fileMenu->AddItem(GetStr(g,11), file_Print, 'P');
   if (! RunningOSX())
   {  fileMenu->AddSeparator();
      fileMenu->AddItem(GetStr(g,13), file_Preferences, 'P', cMenuModifier_Option);
      fileMenu->AddSeparator();
      fileMenu->AddItem(GetStr(g,14), file_Quit, 'Q');
   }
   AddMenu(fileMenu);
} /* SigmaApplication::BuildFileMenu */

/*------------------------------------------ Edit Menu -------------------------------------------*/

void SigmaApplication::BuildEditMenu (void)
{
   INT g = sgr_EditMenu;
   editMenu = new SigmaMenu(GetStr(g,0));
   editMenu->AddItem(GetStr(g,1),  edit_Undo, 'Z');
   editMenu->AddItem(GetStr(g,2),  edit_Redo, 'Z', cMenuModifier_Shift);
   editMenu->AddSeparator();
   editMenu->AddItem(GetStr(g,3),  edit_Cut);
   editMenu->AddItem(GetStr(g,4),  edit_Copy);
   editMenu->AddItem(GetStr(g,5),  edit_Paste);
   editMenu->AddItem(GetStr(g,6),  edit_Clear);
   editMenu->AddItem(GetStr(g,7),  edit_SelectAll, 'A', cMenuModifier_Option);
   editMenu->AddSeparator();
   editMenu->AddItem(GetStr(g,8),  edit_Find, 'F');
   editMenu->AddItem(GetStr(g,9),  edit_FindAgain, 'F', cMenuModifier_Shift);
   editMenu->AddItem(GetStr(g,10), edit_Replace);
   editMenu->AddItem(GetStr(g,11), edit_ReplaceFind);
   editMenu->AddItem(GetStr(g,12), edit_ReplaceAll);
   AddMenu(editMenu);

   // "Cut" sub menu:
   cutMenu = new CMenu("Cut");
   cutMenu->AddItem("Text",         cut_Standard, 'X');
   cutMenu->AddSeparator();
   cutMenu->AddItem("Game",         cut_Game);
   editMenu->SetSubMenu(edit_Cut,   cutMenu);
   cutMenu->SetIcon(cut_Standard,   icon_SelectAll);
   cutMenu->SetIcon(cut_Game,       icon_Game);

   // "Copy" sub menu:
   copyMenu = new CMenu("Copy");
   copyMenu->AddItem("Text",        copy_Standard, 'C');
   copyMenu->AddSeparator();
   copyMenu->AddItem("Game",        copy_Game);
   copyMenu->AddItem("Game (without annotations)", copy_GameNoAnn);
   copyMenu->AddItem("Position",    copy_Position);
   copyMenu->AddItem("Analysis",    copy_Analysis);
   editMenu->SetSubMenu(edit_Copy,  copyMenu);
   copyMenu->SetIcon(copy_Standard, icon_SelectAll);
   copyMenu->SetIcon(copy_Game,     icon_Game);
   copyMenu->SetIcon(copy_GameNoAnn,icon_Game);
   copyMenu->SetIcon(copy_Position, icon_Position);
   copyMenu->SetIcon(copy_Analysis, icon_ShowAnalysis);

   // "Paste" sub menu:
   pasteMenu = new CMenu("Paste");
   pasteMenu->AddItem("Text",       paste_Standard, 'V');
   pasteMenu->AddSeparator();
   pasteMenu->AddItem("Game",       paste_Game);
   pasteMenu->AddItem("Position",   paste_Position);
   editMenu->SetSubMenu(edit_Paste, pasteMenu);
   pasteMenu->SetIcon(paste_Standard, icon_SelectAll);
   pasteMenu->SetIcon(paste_Game,     icon_Game);
   pasteMenu->SetIcon(paste_Position, icon_Position);
} /* SigmaApplication::BuildEditMenu */

/*------------------------------------------ Game Menu -------------------------------------------*/

void SigmaApplication::BuildGameMenu (void)
{
   INT g = sgr_GameMenu;
   gameMenu = new SigmaMenu(GetStr(g,0));
   gameMenu->AddItem(GetStr(g,1),  game_ResetGame, 'N', cMenuModifier_Shift);
   gameMenu->AddItem(GetStr(g,2),  game_RateGame, 'N', cMenuModifier_Option);
   gameMenu->AddItem(GetStr(g,3),  game_BranchGame, 'B');
// gameMenu->AddItem(GetStr(g,4),  game_ReplayGame);
   gameMenu->AddSeparator();
   gameMenu->AddItem(GetStr(g,5),  game_UndoMove, 'U');
   gameMenu->AddItem(GetStr(g,6),  game_RedoMove, 'R');
   gameMenu->AddItem(GetStr(g,7),  game_UndoAllMoves,'U', cMenuModifier_Option);
   gameMenu->AddItem(GetStr(g,8),  game_RedoAllMoves,'R', cMenuModifier_Option);
   gameMenu->AddItem(GetStr(g,9),  game_GotoMove,'M', cMenuModifier_Control);
   gameMenu->AddItem(GetStr(g,10), game_ClearRest);
   gameMenu->AddSeparator();
   gameMenu->AddItem(GetStr(g,11), game_PositionEditor, 'E');
   gameMenu->AddItem(GetStr(g,12), game_AnnotationEditor, 'A');
   gameMenu->AddItem(GetStr(g,13), game_ClearAnn);
   gameMenu->AddSeparator();
   gameMenu->AddItem(GetStr(g,14), game_AddToCollection);
   gameMenu->AddItem(GetStr(g,15), game_Detach, 'K', cMenuModifier_Control);
   gameMenu->AddSeparator();
   gameMenu->AddItem(GetStr(g,16), game_GameInfo, 'I');
   AddMenu(gameMenu);
} /* SigmaApplication::BuildGameMenu */

/*---------------------------------------- Analyze Menu ------------------------------------------*/

void SigmaApplication::BuildAnalyzeMenu (void)
{
   INT g = sgr_AnalyzeMenu;
   analyzeMenu = new SigmaMenu(GetStr(g,0));
   if (RunningOSX())
   {  analyzeMenu->AddItem(GetStr(g,1),  analyze_Engine);
      analyzeMenu->AddSeparator();
   }
   analyzeMenu->AddItem(GetStr(g,2),  analyze_Go, 'G');
   analyzeMenu->AddItem(GetStr(g,3),  analyze_NextBest, 'G', cMenuModifier_Option);
   analyzeMenu->AddItem(GetStr(g,4),  analyze_Stop, '.');
   analyzeMenu->AddItem(GetStr(g,5),  analyze_Pause, ',');
   analyzeMenu->AddItem(GetStr(g,6),  analyze_Hint, 'H', cMenuModifier_Shift);
   analyzeMenu->AddItem(GetStr(g,7),  analyze_PlayMainLine, 'M', cMenuModifier_Shift);
   analyzeMenu->AddSeparator();
   analyzeMenu->AddItem(GetStr(g,8),  analyze_DrawOffer, 'D', cMenuModifier_Shift);
   analyzeMenu->AddItem(GetStr(g,9),  analyze_Resign, 'R', cMenuModifier_Shift);
   analyzeMenu->AddSeparator();
   analyzeMenu->AddItem(GetStr(g,10), analyze_AutoPlay, 'A', cMenuModifier_Shift);
   analyzeMenu->AddItem(GetStr(g,11), analyze_DemoPlay);
   analyzeMenu->AddItem(GetStr(g,12), analyze_AnalyzeGame, 'A', cMenuModifier_Control);
   analyzeMenu->AddItem(GetStr(g,13), analyze_AnalyzeCol);
   analyzeMenu->AddItem(GetStr(g,14), analyze_AnalyzeEPD);
   if (RunningOSX())
      analyzeMenu->AddItem(GetStr(g,15), analyze_EngineMatch);
   analyzeMenu->AddSeparator();
   analyzeMenu->AddItem(GetStr(g,16), analyze_TransTables, 'T', cMenuModifier_Shift);
   analyzeMenu->AddItem(GetStr(g,17), analyze_EndgameDB);
   AddMenu(analyzeMenu);

   engineMenu = nil;
   RebuildEngineMenu();
} /* SigmaApplication::BuildAnalyzeMenu */


void SigmaApplication::RebuildEngineMenu (void)
{
   if (! RunningOSX()) return;

   if (engineMenu)
   {	analyzeMenu->ClrSubMenu(analyze_Engine);
      delete engineMenu;
      engineMenu = nil;
   }

   INT g = sgr_EngineMenu;
   engineMenu = new SigmaMenu(GetStr(g,0));

   for (INT i = 0; i < Prefs.UCI.count; i++)
   {  if (i == 1) engineMenu->AddSeparator();
      engineMenu->AddItem(Prefs.UCI.Engine[i].name, engine_Sigma + i);
      engineMenu->SetIcon(engine_Sigma + i, (i == 0 ? icon_SigmaChess : icon_Engine));
   }

   engineMenu->AddSeparator();   
   engineMenu->AddItem(GetStr(g,2), engine_Configure, 'M', cMenuModifier_Option);
   engineMenu->SetIcon(engine_Configure, icon_EngineMgr);
   analyzeMenu->SetSubMenu(analyze_Engine, engineMenu);
} /* SigmaApplication::RebuildEngineMenu */

/*----------------------------------------- Level Menu -------------------------------------------*/

void SigmaApplication::BuildLevelMenu (void)
{
   INT g = sgr_LevelMenu;
   levelMenu = new SigmaMenu(GetStr(g,0));
   levelMenu->AddItem(GetStr(g,1), level_Select, 'L');
   levelMenu->AddItem(GetStr(g,2), level_PlayingStyle);
   levelMenu->AddItem(GetStr(g,3), level_PermanentBrain);
   levelMenu->AddItem(GetStr(g,4), level_NonDeterm);
   levelMenu->AddSeparator();
   levelMenu->AddItem(GetStr(g,5), level_SigmaELO, 'E', cMenuModifier_Shift);
   levelMenu->AddItem(GetStr(g,6), level_PlayerELO, 'E', cMenuModifier_Option);
   levelMenu->AddItem(GetStr(g,7), level_ELOCalc);
   AddMenu(levelMenu);

   styleMenu = BuildPlayingStyleMenu(false);
   levelMenu->SetSubMenu(level_PlayingStyle, styleMenu);
} /* SigmaApplication::BuildLevelMenu */


CMenu *SigmaApplication::BuildPlayingModeMenu (BOOL popup)
{
   CMenu *pm = new CMenu(GetStr(sqr_LD_ModesMenu,0));

   if (popup) pm->AddPopupHeader("Playing Mode");

   for (INT m = pmode_TimeMoves; m <= pmode_Manual; m++)
   {
      if (m == pmode_Infinite || m == pmode_Monitor)
         pm->AddSeparator();
      pm->AddItem(GetStr(sqr_LD_ModesMenu,m), m);
      if (popup || RunningOSX()) pm->SetIcon(m, ModeIcon[m]);
   }

   return pm;
} /* SigmaApplication::BuildPlayingModeMenu */


CMenu *SigmaApplication::BuildPlayingStyleMenu (BOOL popup)
{
   CMenu *pm = new CMenu(GetStr(sqr_LD_ModesMenu,0));

   INT g = sgr_PlayingStyleMenu;
   pm = new CMenu(GetStr(g,0));
   if (popup) pm->AddPopupHeader("Playing Style");
   pm->AddItem(GetStr(g,1), playingStyle_Chicken);
   pm->AddItem(GetStr(g,2), playingStyle_Defensive);
   pm->AddItem(GetStr(g,3), playingStyle_Normal);
   pm->AddItem(GetStr(g,4), playingStyle_Aggressive);
   pm->AddItem(GetStr(g,5), playingStyle_Desperado);
   for (INT i = 0; i < 5; i++)
      pm->SetIcon(playingStyle_Chicken + i, icon_Style1 + i);

   return pm;
} /* SigmaApplication::BuildPlayingStyleMenu */

/*----------------------------------------- Display Menu -----------------------------------------*/

void SigmaApplication::BuildDisplayMenu (void)
{
   INT g = sgr_DisplayMenu;
   displayMenu = new SigmaMenu(GetStr(g,0));
   displayMenu->AddItem(GetStr(g,1),  display_TurnBoard, 'T');
   displayMenu->AddItem(GetStr(g,2),  display_PieceSet);
   displayMenu->AddItem(GetStr(g,3),  display_BoardType);
   displayMenu->AddItem(GetStr(g,4),  display_BoardSize);
   displayMenu->AddItem(GetStr(g,5),  display_MoveMarker);
   displayMenu->AddSeparator();
   displayMenu->AddItem(GetStr(g,8),  display_ToggleInfoArea, 'T', cMenuModifier_Option);
   displayMenu->AddItem(GetStr(g,6),  display_Notation);
   displayMenu->AddItem(GetStr(g,7),  display_PieceLetters);
   displayMenu->AddItem(GetStr(g,9),  display_GameRecord, 'I', cMenuModifier_Option);
   displayMenu->AddSeparator();
   displayMenu->AddItem(GetStr(g,10), display_3DBoard, 'D');
   displayMenu->AddItem(GetStr(g,11), display_Show3DClock);
   displayMenu->AddSeparator();
   displayMenu->AddItem(GetStr(g,12), display_ColorScheme);
   displayMenu->AddItem(GetStr(g,13), display_ToolbarTop);
   AddMenu(displayMenu);

   // "Piece Set" menu:
   pieceSetMenu = BuildPieceSetMenu(false);
   displayMenu->SetSubMenu(display_PieceSet, pieceSetMenu);

   // "Board Type" menu:
   boardTypeMenu = BuildBoardTypeMenu(false);
   displayMenu->SetSubMenu(display_BoardType, boardTypeMenu);

   // "Board Size" menu:
   g = sgr_BoardSizeMenu;
   boardSizeMenu = new CMenu(GetStr(g,0));
   boardSizeMenu->AddItem(GetStr(g,1), boardSize_Standard);
   boardSizeMenu->AddItem(GetStr(g,2), boardSize_Medium);
   boardSizeMenu->AddItem(GetStr(g,3), boardSize_Large);
   boardSizeMenu->AddItem(GetStr(g,4), boardSize_EvenLarger);
   displayMenu->SetSubMenu(display_BoardSize, boardSizeMenu);

   // "Move Marker" menu:
   g = sgr_MoveMarkerMenu;
   moveMarkerMenu = new CMenu(GetStr(g,0));
   moveMarkerMenu->AddItem(GetStr(g,1), moveMarker_Off);
   moveMarkerMenu->AddItem(GetStr(g,2), moveMarker_LastCompMove);
   moveMarkerMenu->AddItem(GetStr(g,3), moveMarker_LastMove);
   displayMenu->SetSubMenu(display_MoveMarker, moveMarkerMenu);

   // "Notation" menu:
   g = sgr_NotationMenu;
   notationMenu = new CMenu(GetStr(g,0));
   notationMenu->AddItem(GetStr(g,1), notation_Short);
   notationMenu->AddItem(GetStr(g,2), notation_Long);
   notationMenu->AddItem(GetStr(g,3), notation_Descr);
   notationMenu->AddSeparator();
   notationMenu->AddItem(GetStr(g,4), notation_Figurine);
   displayMenu->SetSubMenu(display_Notation, notationMenu);

   // "Piece Letters" menu:
   g = sgr_PieceLettersMenu;
   pieceLettersMenu = new CMenu(GetStr(g,0));
   for (INT p = pieceLetters_First; p <= pieceLetters_Last; p++)
   {  INT i = p - pieceLetters_First;
      pieceLettersMenu->AddItem(GetStr(g,i+1), p);
      pieceLettersMenu->SetIcon(p, icon_PieceLetters + i);
   }
   displayMenu->SetSubMenu(display_PieceLetters, pieceLettersMenu);

   // "Color Scheme" menu:
   g = sgr_ColorSchemeMenu;
   colorSchemeMenu = new CMenu(GetStr(g,0));
   for (INT cmd = colorScheme_First; cmd <= colorScheme_Last; cmd++)
   {  colorSchemeMenu->AddItem(GetStr(g,cmd - colorScheme_First + 1), cmd);
      colorSchemeMenu->SetIcon(cmd, icon_ColorScheme - 1 + cmd - colorScheme_First);
      if (cmd == colorScheme_First) colorSchemeMenu->AddSeparator();
   }
   displayMenu->SetSubMenu(display_ColorScheme, colorSchemeMenu);
} /* SigmaApplication::BuildDisplayMenu */


CMenu *SigmaApplication::BuildPieceSetMenu (BOOL popup)
{
   INT g = sgr_PieceSetMenu;
   CMenu *pm = new CMenu(GetStr(g,0));
   if (popup) pm->AddPopupHeader("Piece Set");
   pm->AddItem(GetStr(g,1), pieceSet_American);
   pm->AddItem(GetStr(g,2), pieceSet_European);
   pm->AddItem(GetStr(g,3), pieceSet_Classical);
   pm->AddSeparator();
   pm->AddItem(GetStr(g,4), pieceSet_Metal);
   pm->AddItem(GetStr(g,5), pieceSet_Wood);
   pm->AddItem(GetStr(g,6), pieceSet_Childrens);
   AddPieceSetPlugins(pm);

   for (INT i = 0; i < pieceSet_Count; i++)
      pm->SetIcon(pieceSet_First + i, icon_PieceSet + i);
   for (INT i = 0; i < PieceSetPluginCount(); i++)
      pm->SetIcon(pieceSet_Last + 1 + i, icon_PieceSet + pieceSet_Count);

   return pm;
} /* SigmaApplication::BuildPieceSetMenu */


CMenu *SigmaApplication::BuildBoardTypeMenu (BOOL popup)
{
   INT g = sgr_BoardTypeMenu;
   CMenu *pm = new CMenu(GetStr(g,0));
   if (popup) pm->AddPopupHeader("Board Type");
   for (INT cmd = boardType_First; cmd <= boardType_Last; cmd++)
   {  pm->AddItem(GetStr(g,cmd - boardType_First + 1), cmd);
      pm->SetIcon(cmd, icon_BoardType - 1 + cmd - boardType_First);
      if (cmd == boardType_First) pm->AddSeparator();
   }
   AddBoardTypePlugins(pm);

   for (INT i = 0; i < BoardTypePluginCount(); i++)
      pm->SetIcon(boardType_Last + 1 + i, icon_BoardType - 1 + boardType_Count);

   return pm;
} /* SigmaApplication::BuildBoardTypeMenu */

/*--------------------------------------- Collection Menu ----------------------------------------*/

void SigmaApplication::BuildCollectionMenu (void)
{
   INT g = sgr_CollectionMenu;
   collectionMenu = new SigmaMenu(GetStr(g,0));
   collectionMenu->AddItem(GetStr(g,1), collection_EditFilter, 'F');
   collectionMenu->AddItem(GetStr(g,2), collection_EnableFilter, 'F', cMenuModifier_Shift);
   collectionMenu->AddSeparator();
   collectionMenu->AddItem(GetStr(g,3), collection_OpenGame, 'G');
   collectionMenu->AddItem(GetStr(g,4), collection_PrevGame, 0, cMenuModifier_Control | cMenuModifier_NoCmd);
   collectionMenu->AddItem(GetStr(g,5), collection_NextGame, 0, cMenuModifier_Control | cMenuModifier_NoCmd);
   collectionMenu->AddItem(GetStr(g,6), collection_Layout, 'L');
   collectionMenu->AddSeparator();
   collectionMenu->AddItem(GetStr(g,7), collection_ImportPGN, 'I', cMenuModifier_Shift);
   collectionMenu->AddItem(GetStr(g,8), collection_ExportPGN, 'E', cMenuModifier_Shift);
   collectionMenu->AddItem(GetStr(g,9), collection_Compact);
   collectionMenu->AddItem(GetStr(g,10),collection_Renumber, 'R');
   collectionMenu->AddSeparator();
   collectionMenu->AddItem(GetStr(g,11),collection_Info, 'I');

   collectionMenu->SetGlyph(collection_PrevGame, kMenuLeftArrowGlyph);
   collectionMenu->SetGlyph(collection_NextGame, kMenuRightArrowGlyph);
   AddMenu(collectionMenu);
} /* SigmaApplication::BuildCollectionMenu */

/*---------------------------------------- Library Menu ------------------------------------------*/

void SigmaApplication::BuildLibraryMenu (void)
{
   INT g = sgr_LibraryMenu;
   libraryMenu = new SigmaMenu(GetStr(g,0));
   libraryMenu->AddItem(GetStr(g,1), library_Name);
   libraryMenu->AddItem(GetStr(g,2), library_SigmaAccess);
   libraryMenu->AddSeparator();
   libraryMenu->AddItem(GetStr(g,3), library_Editor, 'L', cMenuModifier_Shift);
   libraryMenu->AddItem(GetStr(g,4), library_ECOComment, 'E', cMenuModifier_Control);
   libraryMenu->AddItem(GetStr(g,5), library_DeleteVar);
   libraryMenu->AddItem(GetStr(g,6), library_ImportCollection);
   libraryMenu->AddSeparator();
   libraryMenu->AddItem(GetStr(g,7), library_Save);
   libraryMenu->AddItem(GetStr(g,8), library_SaveAs);
   AddMenu(libraryMenu);

   // "Sigma Chess Access" menu:
   g = sgr_LibSetMenu;
   libSetMenu = new CMenu(GetStr(g,0));
   libSetMenu->AddItem(GetStr(g,1), librarySet_Disabled);
   libSetMenu->AddSeparator();
   libSetMenu->AddItem(GetStr(g,2), librarySet_Solid);
   libSetMenu->AddItem(GetStr(g,3), librarySet_Tournament);
   libSetMenu->AddItem(GetStr(g,4), librarySet_Wide);
   libSetMenu->AddItem(GetStr(g,5), librarySet_Full);
   libraryMenu->SetSubMenu(library_SigmaAccess, libSetMenu);
} /* SigmaApplication::BuildLibraryMenu */

/*----------------------------------------- Window Menu ------------------------------------------*/

void SigmaApplication::BuildWindowMenu (void)
{
   windowMenu = nil;
   RebuildWindowMenu();
} /* SigmaApplication::BuildWindowMenu */

/*------------------------------------------ Debug Menu ------------------------------------------*/

void SigmaApplication::BuildDebugMenu (void)
{
#ifdef __with_debug
   debugMenu = new CMenu("Debug");
   debugMenu->AddItem("Debug Window", debug_Window, 'D', cMenuModifier_Control);
   debugMenu->AddSeparator();
   debugMenu->AddItem("A", debug_A);
   debugMenu->AddItem("B", debug_B);
   debugMenu->AddItem("C", debug_C);
   AddMenu(debugMenu);
#endif
} /* SigmaApplication::BuildDebugMenu */


/**************************************************************************************************/
/*                                                                                                */
/*                                         MENU ICONS                                             */
/*                                                                                                */
/**************************************************************************************************/

void SigmaApplication::UpdateMenuIcons (void)
{
   // FILE menu icons:
   fileMenu->SetIcon(file_NewGame,            icon_Game, true);
   fileMenu->SetIcon(file_NewCollection,      icon_Col);
   fileMenu->SetIcon(file_NewLibrary,         icon_Lib);
   fileMenu->SetIcon(file_Open,               icon_Open, true);
   fileMenu->SetIcon(file_Save,               icon_Save, true);
   fileMenu->SetIcon(file_SaveAs,             icon_SaveAs);
   fileMenu->SetIcon(file_Close,              (RunningOSX() ? icon_CloseX : icon_Close), true);
   fileMenu->SetIcon(file_ExportHTML,         icon_ExportHTML, false);
   fileMenu->SetIcon(file_PageSetup,          icon_PageSetup, true);
   fileMenu->SetIcon(file_Print,              icon_Print, true);
   if (! RunningOSX())
      fileMenu->SetIcon(file_Preferences, icon_Settings, true);
   fileMenu->SetIcon(file_Quit,               icon_Quit, true);

   // EDIT menu icons:
   editMenu->SetIcon(edit_Undo,               icon_Undo, true);
   editMenu->SetIcon(edit_Redo,               icon_Redo, true);
   editMenu->SetIcon(edit_Cut,                icon_Cut, true);
   editMenu->SetIcon(edit_Copy,               icon_Copy, true);
   editMenu->SetIcon(edit_Paste,              icon_Paste, true);
   editMenu->SetIcon(edit_Clear,              icon_Trash);
   editMenu->SetIcon(edit_SelectAll,          icon_SelectAll);
   editMenu->SetIcon(edit_Find,               icon_Search, true);
   editMenu->SetIcon(edit_FindAgain,          icon_SearchNext, true);
   editMenu->SetIcon(edit_Replace,            icon_Replace);
   editMenu->SetIcon(edit_ReplaceFind,        icon_ReplaceFind);
   editMenu->SetIcon(edit_ReplaceAll,         icon_ReplaceAll);

   // GAME menu icons:
   gameMenu->SetIcon(game_ResetGame,          icon_Position, true);
   gameMenu->SetIcon(game_RateGame,           icon_Rate, true);
   gameMenu->SetIcon(game_BranchGame,         icon_Tree);
   gameMenu->SetIcon(game_ClearRest,          icon_Trash);
   gameMenu->SetIcon(game_ClearAnn,           icon_Trash);
   gameMenu->SetIcon(game_AddToCollection,    icon_ColAddGame);
   gameMenu->SetIcon(game_Detach,             icon_ColDetachGame);
   gameMenu->SetIcon(game_UndoMove,           icon_UndoMove, true);
   gameMenu->SetIcon(game_RedoMove,           icon_RedoMove, true);
   gameMenu->SetIcon(game_UndoAllMoves,       icon_UndoAll, true);
   gameMenu->SetIcon(game_RedoAllMoves,       icon_RedoAll, true);
   gameMenu->SetIcon(game_GotoMove,           icon_Goto);
   gameMenu->SetIcon(game_PositionEditor,     icon_Editor, true);
   gameMenu->SetIcon(game_AnnotationEditor,   icon_Editor, true);
   gameMenu->SetIcon(game_GameInfo,           icon_Info, true);

   // ANALYZE menu icons:
   analyzeMenu->SetIcon(analyze_Engine,       icon_Engine, true);
   analyzeMenu->SetIcon(analyze_Go,           icon_Go, true);
   analyzeMenu->SetIcon(analyze_NextBest,     icon_Go, true);
   analyzeMenu->SetIcon(analyze_Stop,         icon_Stop, true);
   analyzeMenu->SetIcon(analyze_Pause,        icon_Pause, true);
   analyzeMenu->SetIcon(analyze_Hint,         icon_Hint, true);
   analyzeMenu->SetIcon(analyze_PlayMainLine, icon_Goto);
   analyzeMenu->SetIcon(analyze_DrawOffer,    icon_DrawOffer);
   analyzeMenu->SetIcon(analyze_Resign,       icon_Resign);
   analyzeMenu->SetIcon(analyze_AutoPlay,     icon_AutoPlay, true);
   analyzeMenu->SetIcon(analyze_DemoPlay,     icon_DemoPlay);
   analyzeMenu->SetIcon(analyze_AnalyzeGame,  icon_AnalyzeGame);
   analyzeMenu->SetIcon(analyze_AnalyzeCol,   icon_AnalyzeCol);
   analyzeMenu->SetIcon(analyze_AnalyzeEPD,   icon_AutoPlay);
   analyzeMenu->SetIcon(analyze_EngineMatch,  icon_Engine);
   analyzeMenu->SetIcon(analyze_TransTables,  icon_TransTables, true);
   analyzeMenu->SetIcon(analyze_EndgameDB,    icon_EndgameDB);

   // LEVEL menu icons:
   levelMenu->SetIcon(level_Select,           281, true); //### Current playing mode for front window
   levelMenu->SetIcon(level_PlayingStyle,     icon_Style1 + Prefs.Level.playingStyle - 1, true);
   levelMenu->SetIcon(level_PermanentBrain,   icon_LightOn, true);
   levelMenu->SetIcon(level_NonDeterm,        icon_NonDeterm);
   levelMenu->SetIcon(level_SigmaELO,         icon_SigmaChess, true);
   levelMenu->SetIcon(level_PlayerELO,        icon_Player, true);
   levelMenu->SetIcon(level_ELOCalc,          icon_Calc, true);

   // DISPLAY menu icons:
   displayMenu->SetIcon(display_TurnBoard,    icon_TurnBoard, true);
   displayMenu->SetIcon(display_MoveMarker,   icon_MoveMarker);
   displayMenu->SetIcon(display_Notation,     icon_Editor);
   displayMenu->SetIcon(display_BoardSize,    icon_BoardSize);

   displayMenu->SetIcon(display_PieceLetters, icon_PieceLetters + Prefs.Notation.pieceLetters);
   displayMenu->SetIcon(display_ToggleInfoArea,icon_ToggleInfoArea);
   displayMenu->SetIcon(display_GameRecord,   icon_Info);
   displayMenu->SetIcon(display_PieceSet,     icon_PieceSet + Min(Prefs.Appearance.pieceSet, pieceSet_Count), true);
   displayMenu->SetIcon(display_BoardType,    icon_BoardType - 1 + Min(Prefs.Appearance.boardType, boardType_Count), true);
   displayMenu->SetIcon(display_ColorScheme,  icon_ColorScheme - 1 + Prefs.Appearance.colorScheme, true);
   displayMenu->SetIcon(display_3DBoard,      icon_3DBoard, true);
   displayMenu->SetIcon(display_Show3DClock,  icon_ChessClock);
   displayMenu->SetIcon(display_ToolbarTop,   icon_ToolbarTop);

   // COLLECTION menu icons:
   collectionMenu->SetIcon(collection_EditFilter,   icon_Search, true);
   collectionMenu->SetIcon(collection_EnableFilter, icon_Search);
   collectionMenu->SetIcon(collection_OpenGame,     icon_Game, true);
   collectionMenu->SetIcon(collection_PrevGame,     icon_UndoMove, true);
   collectionMenu->SetIcon(collection_NextGame,     icon_RedoMove, true);
   collectionMenu->SetIcon(collection_Layout,       icon_Editor);
   collectionMenu->SetIcon(collection_ImportPGN,    icon_ColImport, true);
   collectionMenu->SetIcon(collection_ExportPGN,    icon_ColExport, true);
   collectionMenu->SetIcon(collection_Compact,      icon_Compact);
   collectionMenu->SetIcon(collection_Renumber,     icon_Renumber);
   collectionMenu->SetIcon(collection_Info,         icon_Info, true);

   // LIBRARY menu icons:
   libraryMenu->SetIcon(library_Name,             icon_Lib, true);
   libraryMenu->SetIcon(library_SigmaAccess,      icon_SigmaChess);
   libraryMenu->SetIcon(library_Editor,           icon_Editor);
   libraryMenu->SetIcon(library_ECOComment,       icon_LibECO);
   libraryMenu->SetIcon(library_DeleteVar,        icon_Trash);
   libraryMenu->SetIcon(library_ImportCollection, icon_LibImport);
   libraryMenu->SetIcon(library_Save,             icon_Save, true);
   libraryMenu->SetIcon(library_SaveAs,           icon_SaveAs);

} /* SigmaApplication::UpdateMenuIcons */


/**************************************************************************************************/
/*                                                                                                */
/*                                        MENU ADJUSTING                                          */
/*                                                                                                */
/**************************************************************************************************/

// This method is called when there are no open windows, in which case most of the menu items
// should be disabled. Additionally, the various settings should reflect the global default values
// in the prefs record.

void SigmaApplication::HandleMenuAdjust (void)
{
   if (winList.Count() > 0) return;

   sigmaApp->EnableQuitCmd(true);   // OS X Menu enabling
   sigmaApp->EnablePrefsCmd(true);
   sigmaApp->EnableAboutCmd(true);

   fileMenu->EnableAllItems(true);
   fileMenu->EnableMenuItem(file_NewGame,       true);
   fileMenu->EnableMenuItem(file_NewCollection, true);
   fileMenu->EnableMenuItem(file_NewLibrary,    true);
   fileMenu->EnableMenuItem(file_Open,          true);
   fileMenu->EnableMenuItem(file_Save,          false);
   fileMenu->EnableMenuItem(file_SaveAs,        false);
   fileMenu->EnableMenuItem(file_Close,         false);
   fileMenu->EnableMenuItem(file_PageSetup,     true);
   fileMenu->EnableMenuItem(file_ExportHTML,    false);
   fileMenu->EnableMenuItem(file_Print,         false);
   fileMenu->EnableMenuItem(file_Preferences,   true);
   fileMenu->EnableMenuItem(file_Quit,          true);

   editMenu->EnableAllItems(false);
   gameMenu->EnableAllItems(false);
   analyzeMenu->EnableAllItems(false);
   displayMenu->EnableAllItems(false);
   collectionMenu->EnableAllItems(false);

   for (INT i = 0; i < Prefs.UCI.count; i++)
      engineMenu->CheckMenuItem(engine_Sigma + i, (i == Prefs.UCI.defaultId));

   levelMenu->EnableAllItems(true);
   levelMenu->EnableMenuItem(level_Select,          false);
   levelMenu->EnableMenuItem(level_PlayingStyle,    false);
   levelMenu->EnableMenuItem(level_PermanentBrain,  false);
   levelMenu->EnableMenuItem(level_NonDeterm,       false);
   levelMenu->EnableMenuItem(level_SigmaELO,        false);
   levelMenu->EnableMenuItem(level_PlayerELO,       false);
   levelMenu->EnableMenuItem(level_ELOCalc,         true);
   levelMenu->SetIcon(level_Select, ModeIcon[Prefs.Level.level.mode]);

   libraryMenu->EnableAllItems(true);
   libraryMenu->EnableMenuItem(library_Name,         true);
   libraryMenu->EnableMenuItem(library_SigmaAccess,  PosLib_Loaded());
   libraryMenu->EnableMenuItem(library_Editor,       false);
   libraryMenu->EnableMenuItem(library_ECOComment,   false);
   libraryMenu->EnableMenuItem(library_DeleteVar,    false);
   libraryMenu->EnableMenuItem(library_ImportCollection, false);
   libraryMenu->EnableMenuItem(library_Save,         PosLib_Loaded() && ! PosLib_Locked() && PosLib_Dirty());
   libraryMenu->EnableMenuItem(library_SaveAs,       PosLib_Loaded());

   displayMenu->CheckMenuItem(display_TurnBoard,    Prefs.GameDisplay.boardTurned);
   displayMenu->CheckMenuItem(display_3DBoard,      Prefs.GameDisplay.mode3D);
   displayMenu->CheckMenuItem(display_ToolbarTop,   false);

// windowMenu->EnableMenuItem(window_Minimize, (GetFrontWindow() ? true : false));

   RedrawMenuBar();
} /* SigmaApplication::HandleMenuAdjust */
