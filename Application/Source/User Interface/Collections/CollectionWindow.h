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

#pragma once

#include "Collection.h"
#include "CollectionToolbar.h"
#include "GameList.h"
#include "GameWindow.h"
#include "SigmaWindow.h"

#include "CFile.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class CollectionWindow : public SigmaWindow {
 public:
  CollectionWindow(CHAR *title, CRect frame, CFile *file);
  virtual ~CollectionWindow(void);

  // Events (should be overridden):
  virtual void HandleKeyDown(CHAR c, INT key, INT modifiers);
  virtual void HandleMessage(LONG msg, LONG submsg = 0, PTR data = nil);
  virtual void HandleMenuAdjust(void);
  virtual void HandleResize(INT newWidth, INT newHeight);
  virtual void HandleZoom(void);
  virtual void HandleScrollBar(CScrollBar *ctrl, BOOL tracking);

  virtual BOOL HandleCloseRequest(void);
  virtual BOOL HandleQuitRequest(void);

  // New methods/instance variables:

  void CalcFrames(void);

  void AdjustFileMenu(void);
  void AdjustEditMenu(void);
  void AdjustGameMenu(void);
  void AdjustAnalyzeMenu(void);
  void AdjustLevelMenu(void);
  void AdjustDisplayMenu(void);
  void AdjustCollectionMenu(void);
  void AdjustLibraryMenu(void);
  void AdjustToolbar(void);

  BOOL CheckSave(CHAR *prompt);
  BOOL IsLocked(void);

  void CutGames(void);
  BOOL CopyGames(void);
  void PasteGames(void);

  void OpenGame(LONG gameNo, GameWindow *target = nil);
  void CloseGame(LONG gameNo);
  BOOL GameOpened(LONG gameNo);
  void PrevGame(GameWindow *target);
  void NextGame(GameWindow *target);
  BOOL CanPrevGame(void);
  BOOL CanNextGame(void);

  void EditLayout(LONG gameNo);

  void DetachGameWin(GameWindow *gameWin);
  void SaveGame(ULONG gameNo, CGame *game);
  void AddGame(GameWindow *gameWin);

  void InfoDialog(void);
  void Renumber(void);
  void PrintCollection(void);
  void ExportHTML(void);

  BOOL Sort(INDEX_FIELD f);
  BOOL SetSortDir(BOOL ascend);
  void DeleteSelection(void);

  void ImportPGN(void);
  void ImportPGNFile(CFile *file);
  void ExportPGN(ULONG i1, ULONG i2);

  void SetBusy(BOOL isBusy);

  SigmaCollection *collection;

  CRect miniToolbarRect, toolbarRect, gameListRect;
  GameListArea *gameListArea;
  CollectionToolbar *toolbar;
  MiniCollectionToolbar *miniToolbar;

  BOOL busy;
  BOOL toolbarTop;

  CList gameWinList;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

CollectionWindow *NewCollectionWindow(void);
CollectionWindow *OpenCollectionWindow(void);
CollectionWindow *OpenCollectionFile(CFile *file);

INT ColWin_Width(void);
INT ColWin_Height(void);
