/**************************************************************************************************/
/*                                                                                                */
/* Module  : SIGMAPREFS.C */
/* Purpose : This module implements the preferences handling. */
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

#include "SigmaPrefs.h"
#include "BmpUtil.h"
#include "BoardView.h"
#include "CDialog.h"
#include "CUtility.h"
#include "CustomBoardDialog.h"
#include "EngineMatchDialog.h"
#include "Game.h"
#include "GameListView.h"
#include "GameUtil.h"
#include "GameView.h"
#include "SigmaApplication.h"
#include "SigmaStrings.h"

SigmaPrefs *sigmaPrefs = nil;

/**************************************************************************************************/
/*                                                                                                */
/*                                      GLOBAL PREFS/VARIABLES */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Global Prefs Structure
 * -----------------------------------*/

PREFS Prefs;

/*--------------------------------- Language Dependant Piece Letters
 * -----------------------------*/

static CHAR PieceLetters[17][8] = {
    "PJSVDK",  //  0 : Czech
    "BSLTDK",  //  1 : Danish
    "OPLTDK",  //  2 : Dutch
    "PNBRQK",  //  3 : English
    "PRLTDK",  //  4 : Finnish
    "PCFTDR",  //  5 : French
    "BSLTDK",  //  6 : German
    "GHFBVK",  //  7 : Hungarian
    "PRBHDK",  //  8 : Icelandic
    "PCATDR",  //  9 : Italian
    "BSLTDK",  // 10 : Norwegian
    "PSGWHK",  // 11 : Polish
    "PCBTDR",  // 12 : Portuguese
    "PCNTDR",  // 13 : Romanian
    "PCATDR",  // 14 : Spanish
    "BSLTDK",  // 15 : Swedish
    "PNBRQK"   // 16 : US
};

/*------------------------------------- Colour Schemes
 * -------------------------------------------*/

static RGB_COLOR ColorScheme[numSchemeColor][3] =  // Colour scheme table:
    {
        {{00, 00, 00},
         {00, 00, 00},
         {00, 00,
          00}},  //  0 Color Picker (unused) Appearance.pick used instead
        {{87, 87, 87},
         {85, 85, 85},
         {80, 80, 80}},  //  1 Standard (button gray)
        // {{87,87,87},    {83,83,83}, {68,68,68}},     //  1 Standard (button
        // gray)
        {{73, 73, 73}, {53, 53, 53}, {30, 30, 30}},   //  2 Graphite
        {{60, 60, 60}, {40, 40, 40}, {20, 20, 20}},   //  3 Coal
        {{80, 80, 60}, {60, 60, 40}, {40, 40, 0}},    //  4 Forest
        {{80, 80, 60}, {40, 60, 40}, {0, 40, 20}},    //  5 Leprechaun
        {{100, 80, 60}, {80, 60, 40}, {60, 40, 20}},  //  6 Wood
        {{80, 80, 40}, {60, 60, 20}, {40, 40, 0}},    //  7 Olive
        {{60, 80, 80}, {40, 60, 60}, {20, 40, 40}},   //  8 Ice
        {{100, 60, 40}, {80, 40, 20}, {60, 20, 0}},   //  9 Salmon
        {{100, 60, 60}, {80, 40, 40}, {60, 20, 20}}   // 10 Rose
};

/*-------------------------------------- Board Frames
 * --------------------------------------------*/
// There is one frame colour set for each board type:

static RGB_COLOR FrameColorTab[11] = {
    {20, 20, 20},  // Color Picker (unused)
    {60, 40, 20},  // Standard
    {40, 40, 20},  // Olive
    {60, 40, 0},   // Peanut
    {46, 0, 0},    // Butter
    {20, 40, 40},  // Ice
    {46, 46, 46},  // Gray
    {66, 66, 66},  // Light Gray
    {46, 46, 46},  // Diagram
    {40, 20, 20},  // Wood
    {33, 33, 33}   // Marble
};

RGB_COLOR boardFrameColor[4] =  // Current board frame colours
    {
        {65535, 52428, 39321},  // Frame text colour
        {52428, 39321, 26214},  // Light 3D frame edge
        {39321, 26214, 13107},  // Main board frame color (from FrameColorTab[])
        {26214, 13107, 00000}   // Dark 3D frame edge
};

/**************************************************************************************************/
/*                                                                                                */
/*                                      CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

SigmaPrefs::SigmaPrefs(void) {
  ::InitPieceSetPlugins();   // Must be done first, so we can get them counted
  ::InitBoardTypePlugins();  // Must be done first, so we can get them counted

  Reset();

  prefsFile.Set("Sigma Chess 6.2.1 Prefs", 'pref', theApp->creator,
                filePath_ConfigDir);
  if (prefsFile.Exists())
    Load();
  else {
    prefsFile.Create();
    Save();
    TryUpgradePrevious();
    Prefs.firstLaunch = true;
  }
} /* SigmaPrefs::SigmaPrefs */

SigmaPrefs::~SigmaPrefs(void) {
  if (Prefs.license.wasJustUpgraded) {
    Prefs.license.pro = true;
    Prefs.license.wasJustUpgraded = false;
  }
  Save();
} /* SigmaPrefs::~SigmaPrefs */

// The "Apply" method may NOT be called in the constructor, since here the menus
// haven't been created yet.

void SigmaPrefs::Apply(void) {
  // SetPermanentBrain(Prefs.Level.permanentBrain, true);
  SetNonDeterm(Prefs.Level.nonDeterm, true);
  SetPlayingStyle(Prefs.Level.playingStyle, true);

  SetNotation(Prefs.Notation.moveNotation, true);
  SetFigurine(Prefs.Notation.figurine, true);
  SetPieceLetters(Prefs.Notation.pieceLetters, true);

  EnableLibrary(Prefs.Library.enabled, true);
  SetLibraryAccess(Prefs.Library.set, true);

  SetPieceSet(Prefs.Appearance.pieceSet, true);
  SetBoardType(Prefs.Appearance.boardType, true);
  SetColorScheme(Prefs.Appearance.colorScheme, true);

  SetMoveMarker(Prefs.GameDisplay.moveMarker, true);

  sigmaApp->displayMenu->CheckMenuItem(display_Show3DClock,
                                       Prefs.GameDisplay.show3DClocks);
  sigmaApp->analyzeMenu->CheckMenuItem(analyze_EndgameDB, Prefs.useEndgameDB);
} /* SigmaPrefs::Apply */

/**************************************************************************************************/
/*                                                                                                */
/*                                      RESET/LOAD & SAVE */
/*                                                                                                */
/**************************************************************************************************/

static GAMEINFOFILTER defaultGameInfoFilter = {true,  false, false, false,
                                               false, true,  false};

void SigmaPrefs::Reset(void) {
  ::ClearBlock((PTR)&Prefs, sizeof(PREFS));

  Prefs.mainVersion = sigmaVersion_Main;
  Prefs.subVersion = sigmaVersion_Sub;
  Prefs.release = sigmaRelease_Beta;
  Prefs.buildNumber = sigmaVersion_Build;

  Prefs.firstLaunch = true;

  ResetLicense(&Prefs.license);

  Prefs.Appearance.pieceSet = 1;
  Prefs.Appearance.boardType = 1;
  Prefs.Appearance.boardType3D = 0;
  Prefs.Appearance.squareWidth = squareWidth1;
  Prefs.Appearance.colorScheme = 1;
  Prefs.Appearance.pickScheme = color_LtGray;
  Prefs.Appearance.whiteSquare = color_LtGray;
  Prefs.Appearance.blackSquare = color_Gray;
  Prefs.Appearance.frame = color_MdGray;

  Prefs.Notation.moveNotation = moveNot_Short;
  Prefs.Notation.figurine = true;
  Prefs.Notation.pieceLetters = 16;

  Prefs.General.playerName[0] = 0;
  Prefs.General.menuIcons = 0;
  Prefs.General.enable3D = true;

  Prefs.Games.gotoFinalPos = true;
  Prefs.Games.turnPlayer = false;
  Prefs.Games.showFutureMoves = true;
  Prefs.Games.hiliteCurrMove = true;
  Prefs.Games.askGameSave = true;
  Prefs.Games.moveSpeed = 75;
  Prefs.Games.saveNative = false;

  Prefs.Collections.autoName = true;
  Prefs.Collections.keepColWidths = true;

  Prefs.PGN.skipMoveSep = false;
  Prefs.PGN.openSingle = true;
  Prefs.PGN.fileExtFilter = true;
  Prefs.PGN.keepNewLines = false;

  Prefs.Messages.announceMate = true;
  Prefs.Messages.announce1stMate = true;
  Prefs.Messages.gameOverDlg = true;
  Prefs.Messages.canResign = true;
  Prefs.Messages.canOfferDraw = true;

  Prefs.Sound.woodSound = true;
  Prefs.Sound.moveBeep = false;

  Prefs.Misc.printPageHeaders = true;
  Prefs.Misc.HTMLGifReminder = true;

  Prefs.AnalysisFormat.showScore = true;
  Prefs.AnalysisFormat.showDepth = false;
  Prefs.AnalysisFormat.showTime = false;
  Prefs.AnalysisFormat.showNodes = false;
  Prefs.AnalysisFormat.showNSec = false;
  Prefs.AnalysisFormat.showMainLine = true;
  Prefs.AnalysisFormat.shortFormat = true;
  Prefs.AnalysisFormat.scoreNot = scoreNot_NumRel;

  Prefs.Memory.reserveMem = 5;  // I.e. 5 MB. Obsolete in OS X

  Prefs.Trans.useTransTables = true;
  Prefs.Trans.useTransTablesMF = true;
  Prefs.Trans.totalTransMem = 20;  // I.e. 20 MB total (ONLY IN OS X)
  Prefs.Trans.maxTransSize = 8;    // I.e. 10 MB trans (= 80*2^(8-1))

  Prefs.useEndgameDB = true;

  Prefs.AutoAnalysis.timePerMove = 5;
  Prefs.AutoAnalysis.skipWhitePos = false;
  Prefs.AutoAnalysis.skipBlackPos = false;
  Prefs.AutoAnalysis.skipMatching = true;
  Prefs.AutoAnalysis.skipLowScore = true;
  Prefs.AutoAnalysis.scoreLimit = 25;

  Level_Reset(&Prefs.Level.level);
  for (INT i = 0; i < maxCustomLevels; i++)
    Level_Reset(&Prefs.Level.CustomLevel[i]);  //### Blitz, Fischer rapid,
                                               //monitor, tournament, mate in 2
  Prefs.Level.permanentBrain_OBSOLETE = true;
  Prefs.Level.nonDeterm = false;
  Prefs.Level.strength_OBSOLETE.reduceStrength = false;
  Prefs.Level.strength_OBSOLETE.engineELO = 2400;
  Prefs.Level.strength_OBSOLETE.autoReduce = false;
  Prefs.Level.playingStyle = style_Normal;

  Prefs.GameDisplay.boardTurned = false;
  Prefs.GameDisplay.moveMarker = 2;
  Prefs.GameDisplay.showAnalysis = true;
  Prefs.GameDisplay.showSearchTree = false;
  Prefs.GameDisplay.gameHeaderClosed = false;
  Prefs.GameDisplay.statsHeaderClosed = false;
  Prefs.GameDisplay.dividerPos = defaultGameViewHeight;
  Prefs.GameDisplay.mode3D = false;
  Prefs.GameDisplay.show3DClocks = false;
  Prefs.GameDisplay.gameInfoFilter = defaultGameInfoFilter;
  Prefs.GameDisplay.toolbarTop = false;
  Prefs.GameDisplay.hideInfoArea = false;
  Prefs.GameDisplay.varDisplayVer = false;

  DefaultCollectionCellWidth(Prefs.ColDisplay.cellWidth);
  Prefs.ColDisplay.toolbarTop = false;

  ClearGameInfo(&Prefs.gameInfo);

  Prefs.Library.enabled = true;
  Prefs.Library.set = libSet_Tournament;
  Prefs.Library.autoClassify = libAutoClass_Level;
  CopyStr("Sigma Library", Prefs.Library.name);
  ResetLibImportParam(&Prefs.Library.param);

  Prefs.playerELOCount = 1;
  ResetPlayerRating(&Prefs.PlayerELO, 1200);

  UCI_ResetPrefs();

  EngineMatch_ResetParam(&Prefs.EngineMatch);
} /* SigmaPrefs::Reset */

void SigmaPrefs::Load(void) {
  ULONG size;
  BOOL error = false;

  prefsFile.Open(filePerm_Rd);
  prefsFile.GetSize(&size);

  if (size != sizeof(PREFS) && size < prevPrefsSize - 4)
    error = true;
  else {
    Reset();
    prefsFile.Read(&size, (PTR)&Prefs);
    if (Prefs.mainVersion != sigmaVersion_Main) error = true;
  }

  prefsFile.Close();

  if (Prefs.Appearance.pieceSet >= pieceSet_Count + PieceSetPluginCount())
    Prefs.Appearance.pieceSet = 1;
  if (Prefs.Appearance.boardType >= boardType_Count + BoardTypePluginCount())
    Prefs.Appearance.boardType = 0;
  if (!ProVersion() && Prefs.Trans.maxTransSize > 8)
    Prefs.Trans.maxTransSize = 6;
  if (!Prefs.Collections.keepColWidths)
    DefaultCollectionCellWidth(Prefs.ColDisplay.cellWidth);

  if (error) {
    Reset();
    Save();
  }
} /* SigmaPrefs::Load */

void SigmaPrefs::Save(void) {
  prefsFile.Save(sizeof(PREFS), (PTR)&Prefs);
} /* SigmaPrefs::Save */

static void ResetUnused(INT unused[], INT count) {
  for (INT i = 0; i < count; i++) unused[i] = 0;
} /* ResetUnused */

void SigmaPrefs::TryUpgradePrevious(void) {
  CFile oldFile;
  oldFile.Set("Sigma Chess 6.2.0 Prefs", 'pref', theApp->creator,
              filePath_ConfigDir);
  if (!oldFile.Exists()) return;

  ULONG size;
  oldFile.Open(filePerm_Rd);
  oldFile.GetSize(&size);
  oldFile.Read(&size, (PTR)&Prefs);
  oldFile.Close();
  /*
     // Update defaults of various settings
     Prefs.General.menuIcons = 0;
     Prefs.Appearance.colorScheme = 1;
     Prefs.AutoAnalysis.skipLowScore = true;
     if (Prefs.AutoAnalysis.scoreLimit <= 0)
        Prefs.AutoAnalysis.scoreLimit = 25;
  */
} /* SigmaPrefs::TryUpgradePrevious */

/**************************************************************************************************/
/*                                                                                                */
/*                                      GLOBAL SETTINGS */
/*                                                                                                */
/**************************************************************************************************/

// The following routines are used for changing global (window independant)
// preferences. This also involves setting menu check marks, menus icons,
// submenus e.t.c.:

/*-------------------------------------- Color Scheme
 * --------------------------------------------*/

void SigmaPrefs::SetColorScheme(INT newColorScheme, BOOL startup) {
  RGB_COLOR *scheme = ColorScheme[newColorScheme];

  if (!startup) {
    if (newColorScheme != 0 && Prefs.Appearance.colorScheme == newColorScheme)
      return;
    if (newColorScheme == 0 &&
        !sigmaApp->ColorPicker("Pick Color Scheme",
                               &Prefs.Appearance.pickScheme))
      return;
  }

  sigmaApp->colorSchemeMenu->CheckMenuItem(
      colorScheme_First + Prefs.Appearance.colorScheme, false);
  sigmaApp->colorSchemeMenu->CheckMenuItem(colorScheme_First + newColorScheme,
                                           true);
  sigmaApp->displayMenu->SetIcon(display_ColorScheme,
                                 icon_ColorScheme - 1 + newColorScheme, true);
  Prefs.Appearance.colorScheme = newColorScheme;

  if (newColorScheme == 0) {
    mainColor = Prefs.Appearance.pickScheme;
  } else {
    SetRGBColor100(&mainColor, scheme[1].red, scheme[1].green, scheme[1].blue);
  }

  lightColor = mainColor;
  AdjustColorLightness(&lightColor, +15);
  darkColor = mainColor;
  AdjustColorLightness(&darkColor, -30);

  sigmaApp->BroadcastMessage(msg_RefreshColorScheme);
} /* SigmaPrefs::SetColorScheme */

/*------------------------------------- Level Settings
 * -------------------------------------------*/
/*
void SigmaPrefs::SetPermanentBrain (BOOL permBrain, BOOL startup)
{
   if (Prefs.Level.permanentBrain == permBrain && ! startup) return;

   sigmaApp->levelMenu->CheckMenuItem(level_PermanentBrain, permBrain);
   Prefs.Level.permanentBrain = permBrain;
   sigmaApp->BroadcastMessage(msg_RefreshInfoSep);
} /* SigmaPrefs::SetPermanentBrain */

void SigmaPrefs::SetNonDeterm(BOOL nonDeterm, BOOL startup) {
  if (Prefs.Level.nonDeterm == nonDeterm && !startup) return;

  sigmaApp->levelMenu->CheckMenuItem(level_NonDeterm, nonDeterm);
  Prefs.Level.nonDeterm = nonDeterm;
  sigmaApp->BroadcastMessage(msg_RefreshInfoSep);
} /* SigmaPrefs::SetNonDeterm */

void SigmaPrefs::SetPlayingStyle(INT newStyle, BOOL startup) {
  if (Prefs.Level.playingStyle == newStyle && !startup) return;

  sigmaApp->styleMenu->CheckMenuItem(
      Prefs.Level.playingStyle - style_Chicken + playingStyle_Chicken, false);
  sigmaApp->styleMenu->CheckMenuItem(
      newStyle - style_Chicken + playingStyle_Chicken, true);
  sigmaApp->levelMenu->SetIcon(level_PlayingStyle,
                               icon_Style1 + newStyle - style_Chicken, true);
  Prefs.Level.playingStyle = newStyle;
  sigmaApp->BroadcastMessage(msg_RefreshInfoSep);
} /* SigmaPrefs::SetPlayingStyle */

/*------------------------------------- Library Settings
 * -----------------------------------------*/

void SigmaPrefs::SetLibraryName(CHAR *name, BOOL startup) {
  sigmaApp->libraryMenu->SetItemText(library_Name, name);
  CopyStr(name, Prefs.Library.name);
} /* SigmaPrefs::SetLibraryName */

void SigmaPrefs::EnableLibrary(BOOL enabled, BOOL startup) {
  Prefs.Library.enabled = enabled;
  sigmaApp->libSetMenu->CheckMenuItem(librarySet_Disabled,
                                      !Prefs.Library.enabled);
} /* SigmaPrefs::EnableLibrary */

void SigmaPrefs::SetLibraryAccess(LIB_SET set, BOOL startup) {
  sigmaApp->libSetMenu->CheckMenuItem(librarySet_Disabled + Prefs.Library.set,
                                      false);
  Prefs.Library.set = set;
  sigmaApp->libSetMenu->CheckMenuItem(librarySet_Disabled + Prefs.Library.set,
                                      true);
} /* SigmaPrefs::SetLibraryAccess */

/*---------------------------------- Piece Sets/Board Types
 * --------------------------------------*/

void SigmaPrefs::SetPieceSet(INT newPieceSet, BOOL startup) {
  if (Prefs.Appearance.pieceSet == newPieceSet && !startup) return;

  sigmaApp->pieceSetMenu->CheckMenuItem(
      pieceSet_First + Prefs.Appearance.pieceSet, false);
  sigmaApp->pieceSetMenu->CheckMenuItem(pieceSet_First + newPieceSet, true);
  INT iconID = icon_PieceSet + Min(newPieceSet, pieceSet_Count);
  sigmaApp->displayMenu->SetIcon(display_PieceSet, iconID, true);
  Prefs.Appearance.pieceSet = newPieceSet;
  pieceBmp1->LoadPieceSet(Prefs.Appearance.pieceSet);

  sigmaApp->BroadcastMessage(msg_RefreshPieceSet);
} /* SigmaPrefs::SetPieceSet */

void SigmaPrefs::SetBoardType(INT newBoardType, BOOL startup) {
  if (!startup) {
    if (newBoardType != 0 && Prefs.Appearance.boardType == newBoardType) return;
    if (newBoardType == 0 && !CustomBoardDialog()) return;
  }

  sigmaApp->boardTypeMenu->CheckMenuItem(
      boardType_First + Prefs.Appearance.boardType, false);
  sigmaApp->boardTypeMenu->CheckMenuItem(boardType_First + newBoardType, true);
  INT iconID = icon_BoardType - 1 + Min(newBoardType, boardType_Count);
  sigmaApp->displayMenu->SetIcon(display_BoardType, iconID, true);
  Prefs.Appearance.boardType = newBoardType;
  LoadSquareBmp(Prefs.Appearance.boardType);

  // Set board frame too before refreshing windows:
  if (newBoardType == 0) {
    boardFrameColor[2] = Prefs.Appearance.frame;
  } else if (newBoardType < boardType_Count) {
    RGB_COLOR c = FrameColorTab[Prefs.Appearance.boardType];
    SetRGBColor100(&boardFrameColor[2], c.red, c.green, c.blue);
  } else {
    RGB_COLOR c = color_Black, px;
    for (INT i = 0; i < 8; i++) {
      bSquareBmpView->GetPixelColor(i, i, &px);
      c.red += ((ULONG)px.red) / 8;
      c.green += ((ULONG)px.green) / 8;
      c.blue += ((ULONG)px.blue) / 8;
    }
    boardFrameColor[2] = c;
  }

  RGB_COLOR fc = boardFrameColor[2];
  BOOL isLightFrame =
      ((fc.red / 655 + fc.green / 655 + fc.blue / 655) / 3) > 60;

  boardFrameColor[0] = (isLightFrame ? color_Black : color_White);
  boardFrameColor[1] = boardFrameColor[2];
  boardFrameColor[3] = boardFrameColor[2];
  AdjustRGBHue(&boardFrameColor[1], 30);
  AdjustRGBHue(&boardFrameColor[3], -20);

  sigmaApp->BroadcastMessage(msg_RefreshBoardType);
} /* SigmaPrefs::SetPieceSet */

/*-------------------------------------- Move Notation
 * -------------------------------------------*/

void SigmaPrefs::SetNotation(MOVE_NOTATION newMoveNotation, BOOL startup) {
  if (Prefs.Notation.moveNotation == newMoveNotation && !startup) return;

  sigmaApp->notationMenu->CheckMenuItem(notation_Short,
                                        newMoveNotation == moveNot_Short);
  sigmaApp->notationMenu->CheckMenuItem(notation_Long,
                                        newMoveNotation == moveNot_Long);
  sigmaApp->notationMenu->CheckMenuItem(notation_Descr,
                                        newMoveNotation == moveNot_Descr);
  Prefs.Notation.moveNotation = newMoveNotation;
  SetGameNotation(PieceLetters[Prefs.Notation.pieceLetters],
                  Prefs.Notation.moveNotation);
  sigmaApp->BroadcastMessage(msg_RefreshMoveNotation);
} /* SigmaPrefs::SetNotation */

void SigmaPrefs::SetFigurine(BOOL newFigurine, BOOL startup) {
  if (Prefs.Notation.figurine == newFigurine && !startup) return;

  sigmaApp->notationMenu->CheckMenuItem(notation_Figurine, newFigurine);
  Prefs.Notation.figurine = newFigurine;
  sigmaApp->BroadcastMessage(msg_RefreshMoveNotation);
} /* SigmaPrefs::SetNotation */

void SigmaPrefs::SetPieceLetters(INT newPieceLetters, BOOL startup) {
  if (Prefs.Notation.pieceLetters == newPieceLetters && !startup) return;

  sigmaApp->pieceLettersMenu->CheckMenuItem(
      pieceLetters_First + Prefs.Notation.pieceLetters, false);
  sigmaApp->pieceLettersMenu->CheckMenuItem(
      pieceLetters_First + newPieceLetters, true);
  sigmaApp->displayMenu->SetIcon(display_PieceLetters,
                                 icon_PieceLetters + newPieceLetters);
  Prefs.Notation.pieceLetters = newPieceLetters;
  SetGameNotation(PieceLetters[Prefs.Notation.pieceLetters],
                  Prefs.Notation.moveNotation);
  sigmaApp->BroadcastMessage(msg_RefreshMoveNotation);
} /* SigmaPrefs::SetPieceLetters */

/*--------------------------------------- Move Markers
 * -------------------------------------------*/

void SigmaPrefs::SetMoveMarker(INT moveMarker, BOOL startup) {
  if (Prefs.GameDisplay.moveMarker == moveMarker && !startup) return;

  sigmaApp->moveMarkerMenu->CheckMenuItem(
      moveMarker_Off + Prefs.GameDisplay.moveMarker, false);
  Prefs.GameDisplay.moveMarker = moveMarker;
  sigmaApp->moveMarkerMenu->CheckMenuItem(
      moveMarker_Off + Prefs.GameDisplay.moveMarker, true);
  sigmaApp->BroadcastMessage(msg_RefreshPieceSet);
} /* SigmaPrefs::SetMoveMarker */
