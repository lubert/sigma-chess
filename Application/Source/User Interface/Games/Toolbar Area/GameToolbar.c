/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this
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

#include "GameToolbar.h"
#include "BmpUtil.h"
#include "GameWindow.h"
#include "LevelDialog.h"
#include "SigmaApplication.h"
#include "SigmaStrings.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                           MAIN TOOLBAR */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------ Constructor
 * -----------------------------------------*/

GameToolbar::GameToolbar(CViewOwner *parent, CRect frame)
    : CToolbar(parent, frame) {
  tb_UndoAllMoves = AddButton(game_UndoAllMoves, 1300, 32, 44, "Undo All",
                              GetStr(sgr_HelpTbGame, 0));
  tb_UndoMove =
      AddButton(game_UndoMove, 1301, 32, 44, "Undo", GetStr(sgr_HelpTbGame, 1));
  tb_Go = AddButton(analyze_Go, 1302, 32, 44, "Go", GetStr(sgr_HelpTbGame, 2));
  tb_Stop = new CButton(this, &(tb_Go->frame), analyze_Stop, 0, false, true,
                        1303, "Stop", GetStr(sgr_HelpTbGame, 15));
  tb_RedoMove =
      AddButton(game_RedoMove, 1304, 32, 44, "Redo", GetStr(sgr_HelpTbGame, 3));
  tb_RedoAllMoves = AddButton(game_RedoAllMoves, 1305, 32, 44, "Redo All",
                              GetStr(sgr_HelpTbGame, 4));
  AddSeparator();
  tb_NewGame = AddButton(game_ResetGame, 1312, 32, 50, "New Game",
                         GetStr(sgr_HelpTbGame, 5));
  tb_SaveGame =
      AddButton(file_Save, 1313, 32, 50, "Save", GetStr(sgr_HelpTbGame, 6));
  tb_GameInfo =
      AddButton(game_GameInfo, 1315, 32, 45, "Info", GetStr(sgr_HelpTbGame, 8));
  AddSeparator();
  tb_DrawOffer = AddButton(analyze_DrawOffer, 1317, 32, 55, "Draw Offer", "");
  tb_Resign = AddButton(analyze_Resign, 1318, 32, 50, "Resign", "");
  tb_TurnBoard = AddButton(display_TurnBoard, 1314, 32, 55, "Turn Board",
                           GetStr(sgr_HelpTbGame, 9));
  AddSeparator();
  tb_PrintGame =
      AddButton(file_Print, 1316, 32, 55, "Print", GetStr(sgr_HelpTbGame, 13));
} /* GameToolbar::GameToolbar */

/*------------------------------------------- Adjust
 * ---------------------------------------------*/

void GameToolbar::Adjust(void) {
  GameWindow *gameWin = (GameWindow *)Window();
  CGame *game = gameWin->game;
  BOOL think = gameWin->thinking;
  BOOL exa = gameWin->ExaChess;
  BOOL tx = (think || exa);
  BOOL showGo =
      (!gameWin->thinking && !gameWin->monitoring && !gameWin->autoPlaying);
  BOOL showStop = !showGo;
  BOOL canUndo = (!tx && game->CanUndoMove());
  BOOL canRedo = (!tx && game->CanRedoMove());
  BOOL canGo = (showGo && !exa && !game->GameOver() && !gameWin->posEditor);
  BOOL canStop = (showStop && !exa);

  tb_Go->Show(showGo);
  tb_Stop->Show(showStop);

  // CRect r = tb_Go->frame;
  // r.bottom = bounds.bottom - 1;
  // DrawStripeRect(r, 1);
  if (showGo)
    tb_Go->Redraw();
  else if (showStop)
    tb_Stop->Redraw();

  tb_UndoAllMoves->Enable(canUndo);
  tb_UndoMove->Enable(canUndo);
  tb_Go->Enable(canGo);
  tb_Stop->Enable(canStop);
  tb_RedoMove->Enable(canRedo);
  tb_RedoAllMoves->Enable(canRedo);

  tb_NewGame->Enable(!tx && !gameWin->posEditor);
  tb_SaveGame->Enable(game->dirty && !gameWin->posEditor);
  tb_GameInfo->Enable(!gameWin->posEditor && !exa);

  tb_DrawOffer->Enable(think && !gameWin->autoPlaying && !exa &&
                       !gameWin->drawOffered);
  tb_Resign->Enable(!tx && !gameWin->posEditor && !gameWin->monitoring &&
                    !game->GameOver());

  tb_PrintGame->Enable(!gameWin->posEditor);

  //   tb_TurnBoard->Press(gameWin->boardTurned);
} /* GameToolbar::Adjust */

/**************************************************************************************************/
/*                                                                                                */
/*                                           MINI TOOLBAR */
/*                                                                                                */
/**************************************************************************************************/

// The mini-toolbar serves several purposes:
//
// 1) Info about last move and variation name
// 2) Access to some less important/frequently used commands
// 3) Overview of "read-only" settings (i.e. if game is locked or a collection
// game or Exa game etc)

#define lastMoveViewWidth 120

class LastMoveView : public CToolbarTextView {
 public:
  LastMoveView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);

 private:
  CGame *game;
};

class SigmaELOView : public CToolbarTextView {
 public:
  SigmaELOView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
};

class PlayerELOView : public CToolbarTextView {
 public:
  PlayerELOView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
};

/*------------------------------------------ Constructor
 * -----------------------------------------*/

MiniGameToolbar::MiniGameToolbar(CViewOwner *parent, CRect frame)
    : CToolbar(parent, frame) {
  GameWindow *win = (GameWindow *)Window();

  // Group 1 (last move)
  AddCustomView(cv_LastMove =
                    new LastMoveView(this, NextItemRect(lastMoveViewWidth)));
  AddSeparator();

  // Group 2 (hint + play main line)
  tb_Hint = AddButton(analyze_Hint, icon_Hint, 16, 24, "");
  AddSeparator();

  // Group 3 (first part of Level menu)
  modeItem = win->level.mode;
  pm_Level = sigmaApp->BuildPlayingModeMenu(true);
  tb_Level = AddPopup(level_SetPlayingMode, pm_Level, ModeIcon[win->level.mode],
                      16, 24, "", "");
  pm_Level->CheckMenuItem(modeItem, true);

  styleItem = playingStyle_Chicken + Prefs.Level.playingStyle - style_Chicken;
  pm_Style = sigmaApp->BuildPlayingStyleMenu(true);
  tb_Style = AddPopup(level_SetPlayingStyle, pm_Style,
                      icon_Style1 + Prefs.Level.playingStyle - style_Chicken,
                      16, 24, "", "");
  pm_Style->CheckMenuItem(styleItem, true);

  tb_PermBrain = AddButton(level_PermanentBrain, icon_LightOn, 16, 24);
  tb_PermBrain->SetOnOff();

  tb_Randomize = AddButton(level_NonDeterm, icon_NonDeterm, 16, 24);
  tb_Randomize->SetOnOff();
  AddSeparator();

  // Group 4 (second part of Level menu)
  tb_SigmaStrength = AddButton(level_SigmaELO, icon_SigmaChess, 16, 24);
  AddCustomView(cv_SigmaELO = new SigmaELOView(this, NextItemRect(31)));
  tb_PlayerStrength = AddButton(level_PlayerELO, icon_Player, 16, 24);
  AddCustomView(cv_PlayerELO = new PlayerELOView(this, NextItemRect(31)));
  tb_ELOCalc = AddButton(level_ELOCalc, icon_Calc, 16, 24);
  AddSeparator();

  Adjust();
} /* MiniGameToolbar::MiniGameToolbar */

void MiniGameToolbar::HandleUpdate(CRect updateRect) {
  CToolbar::HandleUpdate(updateRect);
  DrawReadOnlyGroup(false);
} /* MiniGameToolbar::HandleUpdate */

/*------------------------------------------- Adjust
 * ---------------------------------------------*/

void MiniGameToolbar::Adjust(void) {
  GameWindow *win = (GameWindow *)Window();
  CGame *game = win->game;
  BOOL tx = (win->thinking || win->ExaChess);

  // Group 1 (last move)
  cv_LastMove->Redraw();

  // Group 2 (hint)
  tb_Hint->Enable(!tx && !win->posEditor && !game->GameOver());

  // Group 3 (level, style, perm brain, randomize)
  tb_Level->SetIcon(ModeIcon[win->level.mode]);
  pm_Level->CheckMenuItem(modeItem, false);
  modeItem = win->level.mode;
  pm_Level->CheckMenuItem(modeItem, true);

  tb_Style->SetIcon(icon_Style1 + Prefs.Level.playingStyle - style_Chicken);
  pm_Style->CheckMenuItem(styleItem, false);
  styleItem = playingStyle_Chicken + Prefs.Level.playingStyle - style_Chicken;
  pm_Style->CheckMenuItem(styleItem, true);
  tb_Style->Enable(!win->UsingUCIEngine());

  tb_PermBrain->Press(!win->permanentBrain);
  tb_Randomize->Press(!Prefs.Level.nonDeterm);
  tb_Randomize->Enable(!win->UsingUCIEngine());

  DrawReadOnlyGroup(true);
  cv_SigmaELO->Redraw();
  cv_PlayerELO->Redraw();
} /* MiniGameToolbar::Adjust */

void MiniGameToolbar::DrawReadOnlyGroup(BOOL redrawBackground) {
  if (!Visible() || bounds.Width() < 600) return;

  if (redrawBackground) {
    CRect r = bounds;
    r.Inset(1, 1);
    r.left = r.right - 110;
    DrawStripeRect(r);
  }

  GameWindow *win = (GameWindow *)Window();
  ICON_TRANS trans = (win->IsFront() ? iconTrans_None : iconTrans_Disabled);
  CRect ri(0, 0, 16, 16);
  ri.Offset(bounds.right - 25, bounds.bottom - 21);
  if (win->IsLocked()) DrawIcon(icon_Lock, ri, trans), ri.Offset(-24, 0);
  if (win->isRated) DrawIcon(icon_Rate, ri, trans), ri.Offset(-24, 0);
  if (win->colWin) DrawIcon(icon_Col, ri, trans), ri.Offset(-24, 0);
  if (win->ExaChess) DrawIcon(icon_ExaChess, ri, trans), ri.Offset(-24, 0);
} /* MiniGameToolbar::DrawReadOnlyGroup */

/*---------------------------------------- Last Move View
 * ---------------------------------------*/

LastMoveView::LastMoveView(CViewOwner *parent, CRect frame)
    : CToolbarTextView(parent, frame) {
  game = ((GameWindow *)(Window()))->game;
  SetFontStyle(fontStyle_Bold);
} /* LastMoveView::LastMoveView */

void LastMoveView::HandleUpdate(CRect updateRect) {
  CToolbarTextView::HandleUpdate(updateRect);

  MovePen(3, 0);
  DrawNum(game->GetMoveNo());
  DrawStr(".");
  if (game->currMove > 0) {
    MOVE *m = &game->Record[game->currMove];
    DrawStr(pieceColour(m->piece) == white ? " " : " ... ");
    if (!RunningOSX()) SetBackColor(&color_LtGray);
    ::DrawGameMove(this, m);
  }
} /* LastMoveView::HandleUpdate */

/*----------------------------------------- � ELO View
 * ------------------------------------------*/

SigmaELOView::SigmaELOView(CViewOwner *parent, CRect frame)
    : CToolbarTextView(parent, frame) {} /* SigmaELOView::SigmaELOView */

void SigmaELOView::HandleUpdate(CRect updateRect) {
  CToolbarTextView::HandleUpdate(updateRect);

  GameWindow *win = (GameWindow *)(Window());
  if (!win->engineRating.reduceStrength)
    DrawStr("Max");
  else
    DrawNum(win->engineRating.engineELO);
} /* SigmaELOView::HandleUpdate */

/*--------------------------------------- Player ELO View
 * ---------------------------------------*/

PlayerELOView::PlayerELOView(CViewOwner *parent, CRect frame)
    : CToolbarTextView(parent, frame) {} /* PlayerELOView::PlayerELOView */

void PlayerELOView::HandleUpdate(CRect updateRect) {
  CToolbarTextView::HandleUpdate(updateRect);
  DrawNum(Prefs.PlayerELO.currELO);
} /* PlayerELOView::HandleUpdate */
