/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAMEPRINT.C */
/* Purpose : This module implements the printing of single games. */
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

#include "GamePrint.h"
#include "SigmaPrefs.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CGamePrint::CGamePrint(CHAR *theTitle) : CPrint() {
  CopyStr(theTitle, title);
  game = new CGame();
} /* CGamePrint::CGamePrint */

CGamePrint::~CGamePrint(void) { delete game; } /* CGamePrint::~CGamePrint */

/**************************************************************************************************/
/*                                                                                                */
/*                                       PRINT SINGLE GAME */
/*                                                                                                */
/**************************************************************************************************/

void CGamePrint::PrintGame(CGame *theGame) {
  if (StartJob()) {
    // First make local copy of game:
    game->CopyFrom(theGame);

    // Then initialize page, column and line number information:
    pageNo = column = line = 0;
    gameNo = 0;

    // Perform print job:
    pview = new CGamePrintView(this, PageRect(), VRes(), game, GMap);
    PrintOneGame(false, false);
    if (pageNo > 0) ClosePage();
    delete pview;

    // Finally terminate print job:
    EndJob();
  }
} /* CGamePrint::PrintGame */

/**************************************************************************************************/
/*                                                                                                */
/*                                        PRINT COLLECTION */
/*                                                                                                */
/**************************************************************************************************/

void CGamePrint::PrintCollection(SigmaCollection *collection, LONG start,
                                 LONG end) {
  if (collection->Info.title[0])
    CopySubStr(collection->Info.title, cwindow_MaxTitleLen, title);

  if (collection->Publishing())
    ProVersionDialog(nil,
                     "Please note that Sigma Chess Lite does NOT include "
                     "diagrams when printing game collections.");

  if (StartJob()) {
    // First initialize page, column and line number information:
    pageNo = column = line = 0;

    // Perform print job:
    pview = new CGamePrintView(this, PageRect(), VRes(), game, GMap);

    CHAR progressStr[100];
    Format(progressStr, "Printing the game collection Ò%sÓ...", title);
    ULONG gameCount = end - start + 1;
    collection->BeginProgress("Print Collection", progressStr, end - start + 1,
                              true);

    PrintFrontPage(collection);

    for (LONG i = start; i <= end && !Error(); i++) {
      gameNo = collection->View_GetGameNo(i);
      collection->View_GetGame(i, game);
      PrintOneGame(true, collection->Publishing());

      Format(progressStr, "Page %d (%ld games of %ld)", pageNo, i - start + 1,
             gameCount);
      collection->SetProgress(i - start + 1, progressStr);
      if (collection->ProgressAborted()) Abort();
    }

    if (pageNo > 0) ClosePage();

    collection->EndProgress();

    delete pview;

    // Finally terminate print job:
    EndJob();
  }
} /* CGamePrint::PrintCollection */

void CGamePrint::PrintFrontPage(SigmaCollection *collection) {
  if (!collection->Publishing()) return;

  OpenPage();

  // Draw top horisontal line:
  CRect r = pview->bounds;
  r.Offset(0, 20);
  pview->MovePenTo(0, r.top);
  pview->DrawLine(pview->bounds.Width(), 0);
  pview->MovePenTo(0, r.top + 2);
  pview->DrawLine(pview->bounds.Width(), 0);

  // Draw collection title:
  r.bottom = r.top + 35;
  r.Offset(0, 20);
  pview->SetFontFace(font_Helvetica);
  pview->SetFontStyle(fontStyle_Plain);
  pview->SetFontSize(28);
  pview->DrawStr(collection->Info.title, r, textAlign_Center, true);

  // Draw name of author:
  r.Offset(0, 50);
  pview->SetFontFace(font_Times);
  pview->SetFontStyle(fontStyle_Italic);
  pview->SetFontSize(14);
  pview->DrawStr(collection->Info.author, r, textAlign_Center, true);

  // Draw bottom horisontal line:
  pview->MovePenTo(0, r.bottom);
  pview->DrawLine(pview->bounds.Width(), 0);
  pview->MovePenTo(0, r.bottom + 2);
  pview->DrawLine(pview->bounds.Width(), 0);

  // Draw collection description:
  r.Offset(0, 80);
  r.Inset(50, 0);
  r.bottom = r.top + 200;
  pview->DrawStr(collection->Info.descr, r, textAlign_Left, true);

  pview->SetStandardFont();

  ClosePage();
} /* CGamePrint::PrintFrontPage */

/**************************************************************************************************/
/*                                                                                                */
/*                                             UTILITY */
/*                                                                                                */
/**************************************************************************************************/

void CGamePrint::PrintOneGame(BOOL isCollectionGame, BOOL isPublishing) {
  INT Nmax = game->CalcGameMap(game->lastMove, GMap, true, isCollectionGame,
                               isPublishing);

  game->UndoAllMoves();

  //--- First check if we should force a page break prior to printing this game
  //---
  if (pageNo == 0)
    NextPage();
  else if (isCollectionGame && !(column == 0 && line == 0) &&
           game->Info.pageBreak)
    NextPage();

  //--- Check if we should "prefix" game with chapter and/or section titles:
  if (isCollectionGame && (GMap[0].moveNo & gameMap_Special)) {
    CheckColumnPageBreak(4);
    if (line > 0) line++;
  }

  //--- Finally print the actual lines (incl. diagrams) from the game map:
  for (INT N = 0; N < Nmax && !Error(); N++) {
    if (!ContainsDiagram(N) || (isCollectionGame && !ProVersion())) {
      CheckColumnPageBreak(1);
      pview->PrintGameLine(N, Nmax, column, line, gameNo);
      line++;
    } else {
      CheckColumnPageBreak(diagramLineHeight);
      pview->PrintDiagram(column, line);
      line += diagramLineHeight;
    }
  }

  // Add collection games separator line:
  if (isCollectionGame && line < pview->pageLines - 1) {
    CheckColumnPageBreak(1);
    line++;
  }
} /* CGamePrint::PrintOneGame */

CRect CGamePrint::PageRect(void) {
  CRect r = PageFrame();
  r.Inset(40, 20);
  r.top += 10;
  r.bottom += 10;
  return r;
} /* CGamePrint::PageRect */

BOOL CGamePrint::ContainsDiagram(INT N) {
  return game->GameMapContainsDiagram(GMap, N);
} /* CGamePrint::ContainsDiagram */

void CGamePrint::CheckColumnPageBreak(INT deltaLines) {
  if (line + deltaLines > pview->pageLines) {
    line = 0;
    if (column == 0)
      column++;
    else
      NextPage();
  }
} /* CGamePrint::CheckColumnPageBreak */

void CGamePrint::NextPage(void) {
  if (pageNo > 0) ClosePage();
  pageNo++;
  OpenPage();
  pview->PrintPageHeader(title, pageNo);
  column = line = 0;
} /* CGamePrint::NextPage */
