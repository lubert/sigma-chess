/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAMEOVERDIALOG.C */
/* Purpose : This module implements the game over dialog. */
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

#include "GameOverDialog.h"
#include "Engine.f"

/**************************************************************************************************/
/*                                                                                                */
/*                                        GAME OVER DIALOG */
/*                                                                                                */
/**************************************************************************************************/

class CGameOverDialog : public CDialog {
 public:
  CGameOverDialog(CWindow *parent, CHAR *title, CRect frame, CHAR *text,
                  CHAR *eloMsg, INT bmpID);
  ~CGameOverDialog(void);

  CBitmap *bmp;
};

// It is assumed that the game->Info.result has already been updated properly
// BEFORE this routine is called.

void GameOverDialog(GameWindow *win, BOOL sigmasTurn, BOOL showHumanELO) {
  CGame *game = win->game;
  Beep(3);

  if (!Prefs.Messages.gameOverDlg || game->result == result_Unknown ||
      game->Info.result == infoResult_Unknown)
    return;

  CRect frame(0, 0, 280, 85);
  if (RunningOSX()) frame.right += 40, frame.bottom += 20;
  theApp->CentralizeRect(&frame);

  CHAR title[50], prompt[100], eloMsg[100], result[20];

  switch (game->result) {
    case result_Mate:
      Format(prompt,
             (sigmasTurn ? "Checkmate - You win!" : "Checkmate - I win!"));
      break;
    case result_StaleMate:
      Format(prompt, "Stalemate - The game is drawn!");
      break;  //(sigmasTurn ? "I'm stalemated - The game is drawn!" : "You're
              //stalemated - The game is drawn!")); break;
    case result_Draw3rd:
      Format(prompt, "Draw by repetition!");
      break;
    case result_Draw50:
      Format(prompt, "Draw by the 50 move rule!");
      break;
    case result_DrawInsMtrl:
      Format(prompt, "Draw due to insufficient material!");
      break;
    case result_DrawAgreed:
      Format(prompt, "Draw agreed!");
      break;
    case result_Resigned:
      Format(prompt,
             (sigmasTurn ? "I resign - You win!" : "You resigned - I win!"));
      break;
    case result_TimeForfeit:
      Format(prompt, (sigmasTurn ? "I lost on time - You win!"
                                 : "You lost on time - I win!"));
      break;
  }

  CalcInfoResultStr(game->Info.result, result);
  Format(title, "Game Over : %s", result);

  if (showHumanELO)
    Format(eloMsg, "Your new rating is %d ELO.", Prefs.PlayerELO.currELO);
  else
    CopyStr("", eloMsg);

  CGameOverDialog *dialog = new CGameOverDialog(
      win, title, frame, prompt, eloMsg, 1100 + game->Info.result);
  dialog->Run();
  delete dialog;
} /* GameOverDialog */

/*------------------------------------ Game Over Dialog Class
 * ------------------------------------*/

CGameOverDialog::CGameOverDialog(CWindow *parent, CHAR *title, CRect frame,
                                 CHAR *text, CHAR *eloMsg, INT bmpID)
    : CDialog(parent, title, frame, cdialogType_Sheet) {
  CRect r = InnerRect();

  CRect rText(r.left + 55, r.top, r.right, r.top + controlHeight_Text);
  CRect rBmp(0, 0, 50, 50);
  rBmp.Offset(r.left, r.top);

  bmp = new CBitmap(bmpID, 16);

  new CTextControl(this, text, rText, true, controlFont_SmallSystem);
  rText.Offset(0, 18);
  new CTextControl(this, eloMsg, rText, true, controlFont_SmallSystem);
  new CBitmapControl(this, bmp, rBmp, bmpMode_Trans);

  cbutton_Default = new CPushButton(this, "OK", DefaultRect());
  cbutton_Default->acceptsFocus = false;
  SetDefaultButton(cbutton_Default);

  focusCtl = nil;
} /* CGameOverDialog::CGameOverDialog */

CGameOverDialog::~CGameOverDialog(void) {
  delete bmp;
} /* CGameOverDialog::~CGameOverDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                     ANNOUNCE MATE DIALOG */
/*                                                                                                */
/**************************************************************************************************/

void AnnounceMateDialog(CWindow *parent, INT n, MOVE MateLine[]) {
  if (!Prefs.Messages.announceMate || n == 1) return;

  Beep(1);
  CHAR title[50], msg[200], mstr[20];

  CalcMoveStr(&MateLine[0], mstr);
  Format(title, "Mate in %d moves!", n);
  Format(msg, "I found a forced mate in %d moves beginning with the move %s...",
         n, mstr);

  NoteDialog(parent, title, msg);
} /* AnnounceMateDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                      MATE FINDER DIALOG */
/*                                                                                                */
/**************************************************************************************************/

// When the Mate Finder finds a mate a dialog is displayed showing the key move
// and the search time. The user is given the option of
//
// 1) Playing the key move (which automatically decrements the mate level by
// one). 2) Continuing the search for alternate solutions (cooks) 3) Cancel mate
// search without performing the move.

static INT mateFinderAction = 1;

class CMateFinderDialog : public CDialog {
 public:
  CMateFinderDialog(GameWindow *parent, CHAR *title, CRect frame, MOVE *m,
                    ULONG ticks);

  CRadioButton *cradio_Play, *cradio_Continue, *cradio_Cancel;
};

void MateFinderDialog(GameWindow *gameWin) {
  ENGINE *E = gameWin->engine;

  Beep(1);

  INT n = (1 + maxVal - Engine_BestScore(E)) / 2;
  if (n <= 1) return;

  CHAR title[50];
  Format(title, "Mate in %d moves!", n);

  CRect frame(0, 0, 270, 150);
  if (RunningOSX()) frame.right += 95, frame.bottom += 25;
  theApp->CentralizeRect(&frame);
  CMateFinderDialog *dialog = new CMateFinderDialog(
      gameWin, title, frame, Engine_MainLine(E), Engine_MateTime(E));
  dialog->Run();

  if (dialog->cradio_Play->Selected()) {
    mateFinderAction = 1;
    UCI_Engine_Stop(gameWin->uciEngineId);
  } else if (dialog->cradio_Continue->Selected()) {
    mateFinderAction = 2;
    Engine_Continue(E);  // Releases engine from busy waiting loop
  } else {
    mateFinderAction = 3;
    UCI_Engine_Stop(gameWin->uciEngineId);
    E->R.aborted = true;
    //    gameWin->CheckAbortEngine();  ###Why doesn't this work??
  }

  delete dialog;
} /* MateFinderDialog */

CMateFinderDialog::CMateFinderDialog(GameWindow *gameWin, CHAR *title,
                                     CRect frame, MOVE *m, ULONG ticks)
    : CDialog(gameWin, title, frame, cdialogType_Sheet) {
  CRect r = InnerRect();
  r.bottom = r.top + controlHeight_Text;

  CHAR mstr[20], kstr[20], tstr[20];
  CalcMoveStr(m, mstr);
  Format(kstr, "%s!!", mstr);
  Format(tstr, "%ld.%d secs", ticks / 60, (ticks / 6) % 10);

  INT dv = (RunningOSX() ? 18 : 18);
  r.right = r.left + 50;
  new CTextControl(this, "Key move", r);
  r.Offset(0, dv);
  new CTextControl(this, "Time", r);
  r.Offset(50, -dv);
  r.right = r.left + 10;
  new CTextControl(this, ":", r);
  r.Offset(0, dv);
  new CTextControl(this, ":", r);
  r.Offset(10, -dv);
  r.right = r.left + 100;
  CTextControl *ctext_Key = new CTextControl(this, kstr, r);
  r.Offset(0, dv);
  CTextControl *ctext_Time = new CTextControl(this, tstr, r);
  r.Offset(-60, dv + 7);
  if (RunningOSX()) {
    ctext_Key->SetFontStyle(fontStyle_Bold);
    ctext_Time->SetFontStyle(fontStyle_Bold);
  }
  r.right = InnerRect().right;

  cradio_Play =
      new CRadioButton(this, "Play key move and decrement mate depth", 1, r);
  r.Offset(0, controlVDiff_RadioButton);
  cradio_Continue = new CRadioButton(
      this, "Continue search for alternate solutions (cooks)", 1, r);
  r.Offset(0, controlVDiff_RadioButton);
  cradio_Cancel =
      new CRadioButton(this, "Cancel search without performing move", 1, r);
  cbutton_Default = new CPushButton(this, "OK", DefaultRect());
  SetDefaultButton(cbutton_Default);

  CRadioButton *radio = nil;
  switch (mateFinderAction) {
    case 1:
      radio = cradio_Play;
      break;
    case 2:
      radio = cradio_Continue;
      break;
    case 3:
      radio = cradio_Cancel;
      break;
  }

  if (gameWin->UsingUCIEngine()) {
    cradio_Continue->Enable(false);
    if (mateFinderAction == 2) mateFinderAction = 1;
  }

  radio->Select();
  CurrControl(radio);
} /* CMateFinderDialog::CMateFinderDialog */
