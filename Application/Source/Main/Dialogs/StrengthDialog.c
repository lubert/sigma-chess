/**************************************************************************************************/
/*                                                                                                */
/* Module  : STRENGTHDIALOG.C */
/* Purpose : This module implements the playing strength dialog. */
/*                                                                                                */
/**************************************************************************************************/

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

#include "StrengthDialog.h"

#include "CDialog.h"
#include "DataHeaderView.h"
#include "DataView.h"
#include "Rating.h"
#include "SigmaApplication.h"
#include "SigmaPrefs.h"
#include "SigmaStrings.h"

#define kMaxSigmaELO 2500

/**************************************************************************************************/
/*                                                                                                */
/*                                   SIGMA CHESS STRENGTH DIALOG */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------ Definitions
 * -----------------------------------------*/

INT CategoryMap[categoryCount + 1] = {
    1200, 1400, 1600, 1800,  // Amateurs D, C, B, A
    2000, 2100, 2200, 2400,  // Expert -> IM
    2500, 2600, 2700,        // GM -> World Championship Level
    3000};

class CSigmaRatingDialog : public CDialog {
 public:
  CSigmaRatingDialog(CHAR *title, CRect frame, UCI_ENGINE_ID engineId,
                     ENGINE_RATING *engineRating);

  virtual void HandlePushButton(CPushButton *ctrl);
  virtual void HandlePopupMenu(CPopupMenu *ctrl, INT itemID);
  virtual void HandleRadioButton(CRadioButton *ctrl);
  virtual void HandleScrollBar(CScrollBar *ctrl, BOOL tracking);

  void CreateComputer(void);
  void RefreshEngineELO(void);

 private:
  UCI_ENGINE_ID engineId;
  ENGINE_RATING *S;

  CRadioButton *cradio_Max, *cradio_ELO;
  CCheckBox *ccheck_AutoReduce;
  CMenu *cmenu_Cat;
  CPopupMenu *cpopup_Cat;

  CTextControl *ctext_ELO;
  CScrollBar *cscroll_ELO;

  CPushButton *cbutton_ELOCalc;
};

static CHAR *EngineName(UCI_ENGINE_ID engineId);

/*----------------------------------------- Main Routine
 * -----------------------------------------*/

BOOL EngineRatingDialog(UCI_ENGINE_ID engineId, ENGINE_RATING *engineRating) {
  CRect frame(0, 0, 420, 305);
  if (RunningOSX()) frame.right += 105, frame.bottom += 45;
  theApp->CentralizeRect(&frame);

  ENGINE_RATING dialogRating = *engineRating;

  CHAR title[100];
  Format(title, "%s Rating", EngineName(engineId));
  CSigmaRatingDialog *dialog =
      new CSigmaRatingDialog(title, frame, engineId, &dialogRating);
  dialog->Run();
  BOOL wasOK = (dialog->reply == cdialogReply_OK);
  delete dialog;

  if (wasOK) {
    *engineRating = dialogRating;
    UCI_INFO *Info = &Prefs.UCI.Engine[engineId];
    Info->LimitStrength.u.Check.val = engineRating->reduceStrength;
    Info->UCI_Elo.u.Spin.val = engineRating->engineELO;
    Info->flushELO = true;
  }

  return wasOK;
} /* EngineRatingDialog */

static CHAR *EngineName(UCI_ENGINE_ID engineId) {
  return (engineId == uci_SigmaEngineId ? "Sigma Chess"
                                        : UCI_EngineName(engineId));
} /* EngineName */

/*----------------------------------------- Constructor
 * ------------------------------------------*/

#define miscStr(i) GetStr(sgr_PSD_Misc, i)
#define s_Title miscStr(0)
#define s_ELOCalc miscStr(1)
#define s_Computer miscStr(2)
#define s_Human miscStr(3)
#define s_Select miscStr(4)
#define s_ELORating miscStr(5)
#define s_Category miscStr(6)
#define s_CompNote miscStr(7)

CSigmaRatingDialog::CSigmaRatingDialog(CHAR *title, CRect frame,
                                       UCI_ENGINE_ID inEngineId,
                                       ENGINE_RATING *engineRating)
    : CDialog(nil, title, frame, cdialogType_Modal) {
  CRect inner = InnerRect();

  engineId = inEngineId;
  S = engineRating;

  //--- Create the OK and Cancel buttons first ---
  CRect r(inner.left, inner.bottom - controlHeight_PushButton, inner.left + 115,
          inner.bottom);
  cbutton_ELOCalc = new CPushButton(this, s_ELOCalc, r);
  cbutton_Cancel =
      new CPushButton(this, GetStr(sgr_Common, s_Cancel), CancelRect());
  cbutton_Default =
      new CPushButton(this, GetStr(sgr_Common, s_OK), DefaultRect());
  SetDefaultButton(cbutton_Default);

  //--- "Computer" Controls ---
  CreateComputer();

  CurrControl(cscroll_ELO);
} /* CSigmaRatingDialog::CSigmaRatingDialog */

void CSigmaRatingDialog::CreateComputer(void) {
  CHAR str[1000];  // Utility string

  CRect r = InnerRect();
  r.bottom = r.top + controlHeight_CheckBox;
  r.right -= 18;
  Format(str, "Set the playing strength of %s to:", EngineName(engineId));
  new CTextControl(this, str, r);
  r.Offset(0, controlVDiff_Text);

  cradio_Max = new CRadioButton(this, "Maximum Strength", 1, r);
  {
    r.Offset(20, controlVDiff_Text);
    r.bottom += 15;
    Format(str,
           "%s searches as fast as it can (determined by the speed of your "
           "computer)",
           EngineName(engineId));
    new CTextControl(this, str, r, true, controlFont_SmallSystem);
    r.bottom -= 15;
    r.Offset(-20, controlVDiff_Text + 10 + 15);
  }

  cradio_ELO = new CRadioButton(this, "Specific ELO Rating or Category", 1, r);
  {
    r.Offset(20, controlVDiff_Text);
    Format(str, "%s reduces its search speed accordingly",
           EngineName(engineId));
    new CTextControl(this, str, r, true, controlFont_SmallSystem);
    r.Offset(20, controlVDiff_Text);

    CRect rCat = r;
    rCat.right = InnerRect().right;
    rCat.left = rCat.right - (RunningOSX() ? 205 : 175);
    CRect rELO = rCat;
    rELO.left = r.left;
    rELO.right = rCat.left - 20;

    new CTextControl(this, s_Category, rCat);
    rCat.Offset(0, (RunningOSX() ? 25 : 20));
    ctext_ELO = new CTextControl(this, "", rELO);
    rELO.Offset(0, (RunningOSX() ? 25 : 20));

    UCI_INFO *Info = &Prefs.UCI.Engine[engineId];
    INT eloMin10 = Info->UCI_Elo.u.Spin.min / 10;
    INT eloMax10 = Info->UCI_Elo.u.Spin.max / 10;
    cscroll_ELO = new CScrollBar(this, eloMin10, eloMax10, S->engineELO / 10, 5,
                                 rELO, true, S->reduceStrength);

    CRect rMin = rELO;
    rMin.right = rMin.left + 30;
    rMin.Offset(0, 18);
    NumToStr(10 * eloMin10, str);
    new CTextControl(this, str, rMin, true, controlFont_SmallSystem);

    CRect rMax = rELO;
    rMax.left = rMax.right - 30;
    rMax.Offset(0, 18);
    NumToStr(10 * eloMax10, str);
    new CTextControl(this, str, rMax, true, controlFont_SmallSystem);

    cmenu_Cat = new CMenu("");
    for (INT i = 0;
         i <= categoryCount - 1 && CategoryMap[i] <= Info->UCI_Elo.u.Spin.max;
         i++) {
      cmenu_Cat->AddItem(GetStr(sgr_PSD_Cat, i), i);
      if (i == 3 || i == 7) cmenu_Cat->AddSeparator();
    }
    rCat.bottom = rCat.top + controlHeight_PopupMenu;
    cpopup_Cat =
        new CPopupMenu(this, "", cmenu_Cat, 0, rCat, true, S->reduceStrength);
    HandleScrollBar(cscroll_ELO, false);

    r.top = rCat.bottom + 20;
    r.bottom = r.top + controlHeight_CheckBox;
    ccheck_AutoReduce = new CCheckBox(
        this, "Automatically reduce strength if computer is not fast enough",
        S->autoReduce, r, (engineId == uci_SigmaEngineId), S->reduceStrength);
  }

  r.left = InnerRect().left;
  r.right = InnerRect().right;
  r.top = r.bottom + 15;
  r.bottom = r.top + 45;
  Format(str, s_CompNote, EngineName(engineId));
  new CTextControl(this, str, r, true, controlFont_SmallSystem);

  if (S->reduceStrength)
    cradio_ELO->Select();
  else
    cradio_Max->Select();
} /* CSigmaRatingDialog::CreateComputer */

/*----------------------------------------- Event Handling
 * ---------------------------------------*/

void CSigmaRatingDialog::HandlePushButton(CPushButton *ctrl) {
  if (ctrl == cbutton_Default) {
    S->reduceStrength = cradio_ELO->Selected();
    S->engineELO = 10 * cscroll_ELO->GetVal();
    S->autoReduce = ccheck_AutoReduce->Checked();
  } else if (ctrl == cbutton_ELOCalc) {
    RatingCalculatorDialog();
  }

  CDialog::HandlePushButton(ctrl);
} /* CSigmaRatingDialog::HandlePushButton */

void CSigmaRatingDialog::HandlePopupMenu(CPopupMenu *ctrl, INT itemID) {
  if (ctrl == cpopup_Cat) {
    cscroll_ELO->SetVal(CategoryMap[itemID] / 10);
    RefreshEngineELO();
  }
} /* CSigmaRatingDialog::HandlePopupMenu */

void CSigmaRatingDialog::HandleRadioButton(CRadioButton *ctrl) {
  CDialog::HandleRadioButton(ctrl);

  if (ctrl == cradio_Max || ctrl == cradio_ELO) {
    cscroll_ELO->Enable(cradio_ELO->Selected());
    cpopup_Cat->Enable(cradio_ELO->Selected());
    ccheck_AutoReduce->Enable(cradio_ELO->Selected());
  }
} /* CSigmaRatingDialog::HandleCheckBox */

void CSigmaRatingDialog::HandleScrollBar(CScrollBar *ctrl, BOOL tracking) {
  if (ctrl == cscroll_ELO) {
    RefreshEngineELO();

    for (INT i = categoryCount; i >= 0; i--)
      if (S->engineELO >= CategoryMap[i]) {
        cpopup_Cat->Set(i);
        return;
      }
  }
} /* CSigmaRatingDialog::HandleScrollBar */

void CSigmaRatingDialog::RefreshEngineELO(void)  // Updates from scrollbar val
{
  S->engineELO = 10 * cscroll_ELO->GetVal();

  CHAR str[50];
  Format(str, "ELO Rating: %d", S->engineELO);
  ctext_ELO->SetTitle(str);
} /* CSigmaRatingDialog::RefreshEngineELO */

/**************************************************************************************************/
/*                                                                                                */
/*                                        PLAYER RATING DIALOG */
/*                                                                                                */
/**************************************************************************************************/

//#define test_elo_stats 1  //###
#define tableRowHeight 15

/*---------------------------------------- Subview Classes
 * ---------------------------------------*/
// Separate sub views are created for:
//
// � The "game count" table (game count/won/lost/drawn...)
// � The "ELO summary" table (curr/initial/min/max/avg ELO)
// � The "ELO History" graph

class GameCountView : public CView {
 public:
  GameCountView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
};

class ELOSummaryView : public CView {
 public:
  ELOSummaryView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
};

class ELOHistoryView : public CView {
 public:
  ELOHistoryView(CViewOwner *parent, CRect frame);
  virtual void HandleUpdate(CRect updateRect);
};

class CPlayerRatingDialog : public CDialog {
 public:
  CPlayerRatingDialog(CRect frame);

  virtual void HandlePushButton(CPushButton *ctrl);

 private:
  GameCountView *gmView;
  ELOSummaryView *eloSumView;
  ELOHistoryView *eloHisView;
  CPushButton *cbutton_Reset, *cbutton_ELOCalc;
#ifdef test_elo_stats
  CPushButton *test_Win, *test_Draw, *test_Loss;
#endif
};

/*----------------------------------------- Main Routine
 * -----------------------------------------*/

BOOL PlayerRatingDialog(void) {
  CRect frame(0, 0, 480, 400);
  if (RunningOSX()) frame.right += 25, frame.bottom += 20;
  theApp->CentralizeRect(&frame);

  if (!ProVersion() && Prefs.PlayerELO.gameCount[rating_Total] > 10)
    ProVersionDialog(nil,
                     "The Player ELO History graph only includes the first 10 "
                     "games in Sigma Chess Lite.");

  CPlayerRatingDialog *dialog = new CPlayerRatingDialog(frame);
  dialog->Run();
  BOOL wasOK = (dialog->reply == cdialogReply_OK);
  delete dialog;

  return wasOK;
} /* PlayerRatingDialog */

/*----------------------------------------- Constructor
 * ------------------------------------------*/

CPlayerRatingDialog::CPlayerRatingDialog(CRect frame)
    : CDialog(nil, "Player Rating", frame, cdialogType_Modal) {
  CRect inner = InnerRect();
  CRect rt = inner;
  rt.bottom = rt.top + 30;
  new CTextControl(this,
                   "When you have played a rated game against Sigma Chess, the "
                   "ELO rating statistics below are updated accordingly",
                   rt, true, controlFont_SmallSystem);

  //--- Create the OK and Cancel buttons first ---
  CRect relo(inner.left, inner.bottom - controlHeight_PushButton,
             inner.left + 115, inner.bottom);
  cbutton_ELOCalc = new CPushButton(this, s_ELOCalc, relo);
  cbutton_Reset = new CPushButton(this, "Reset...", CancelRect());
  cbutton_Default = new CPushButton(this, "Close", DefaultRect());
  SetDefaultButton(cbutton_Default);

#ifdef test_elo_stats
  CRect rb = CancelRect();
  rb.right = rb.left + 40;
  rb.Offset(-50, 0);
  test_Win = new CPushButton(this, "1", rb);
  rb.Offset(-50, 0);
  test_Draw = new CPushButton(this, "0.5", rb);
  rb.Offset(-50, 0);
  test_Loss = new CPushButton(this, "0", rb);
#endif

  // Create the "Game Count" subview:
  CRect r = InnerRect();
  r.top = rt.bottom + 10;
  r.bottom = r.top + headerViewHeight + 3 * tableRowHeight + 7 + 5;
  gmView = new GameCountView(this, r);

  // Create the "ELO Summary" subview:
  r.top = r.bottom + 10;
  r.bottom = r.top + headerViewHeight + tableRowHeight + 7;
  eloSumView = new ELOSummaryView(this, r);

  // Create the "ELO History" subview:
  r.top = r.bottom + 10;
  r.bottom = DefaultRect().top - 15;
  eloHisView = new ELOHistoryView(this, r);

} /* CPlayerRatingDialog::CPlayerRatingDialog */

/*----------------------------------------- Event Handling
 * ---------------------------------------*/

static INT InitELODialog(void);

void CPlayerRatingDialog::HandlePushButton(CPushButton *ctrl) {
  if (ctrl == cbutton_Reset) {
    if (Prefs.PlayerELO.gameCount[rating_Total] == 0 ||
        !QuestionDialog(
            this, "Reset Rating Statistics",
            "Are you sure you want to reset all the rating statistics?", "No",
            "Yes")) {
      INT initELO = InitELODialog();
      if (initELO > 0) ResetPlayerRating(&(Prefs.PlayerELO), initELO);
    }
  }
#ifdef test_elo_stats
  else if (ctrl == test_Win)
    UpdatePlayerRating(&(Prefs.PlayerELO), !Prefs.GameDisplay.boardTurned, 1.0,
                       1700);
  else if (ctrl == test_Draw)
    UpdatePlayerRating(&(Prefs.PlayerELO), !Prefs.GameDisplay.boardTurned, 0.5,
                       1700);
  else if (ctrl == test_Loss)
    UpdatePlayerRating(&(Prefs.PlayerELO), !Prefs.GameDisplay.boardTurned, 0.0,
                       1700);
#endif
  else if (ctrl == cbutton_ELOCalc) {
    RatingCalculatorDialog();
    return;
  } else {
    CDialog::HandlePushButton(ctrl);
    return;
  }

  gmView->Redraw();
  eloSumView->Redraw();
  eloHisView->Redraw();
} /* CPlayerRatingDialog::HandlePushButton */

/*--------------------------------------- Game Count Subview
 * -------------------------------------*/

#define gmWidth 75

static HEADER_COLUMN GameCountHCTab[6] = {
    {"", 0, gmWidth},      {"Games", 0, gmWidth}, {"Won", 0, gmWidth},
    {"Drawn", 0, gmWidth}, {"Lost", 0, gmWidth},  {"Score", 0, -1}};

static CHAR *RowStr[3] = {"White", "Black", "Total"};

GameCountView::GameCountView(CViewOwner *parent, CRect frame)
    : CView(parent, frame) {
  CRect r = bounds;
  r.Inset(1, 1);
  r.bottom = r.top + headerViewHeight;
  new DataHeaderView(this, r, false, true, 6, GameCountHCTab);
} /* GameCountView::GameCountView */

void GameCountView::HandleUpdate(CRect updateRect) {
  Draw3DFrame(bounds, &color_Gray, &color_White);

  // Clear contents
  CRect r = bounds;
  r.Inset(1, 1);
  r.top += headerViewHeight - 1;

  DrawRectFill(r, &color_White);
  SetStdForeColor();
  DrawRectFrame(r);
  SetForeColor(&color_Gray);

  for (INT i = 1; i <= 5; i++) {
    MovePenTo(r.left + gmWidth * i, r.top);
    DrawLineTo(r.left + gmWidth * i, r.bottom - 2);
  }

  MovePenTo(r.left + 1, r.bottom - tableRowHeight - 5);
  DrawLine(r.Width() - 3, 0);

  // Draw contents
  PLAYER_RATING *P = &(Prefs.PlayerELO);
  SetStdForeColor();
  SetBackColor(&color_White);
  INT v = r.top + tableRowHeight;
  for (INT i = 0; i <= 2; i++, v += tableRowHeight) {
    INT h = r.left + 5;
    INT pct = (100 * P->wonCount[i] + 50 * P->drawnCount[i]) / P->gameCount[i];
    if (i == 2) v += 5;
    SetFontStyle(i < 2 ? fontStyle_Plain : fontStyle_Bold);
    MovePenTo(h, v);
    DrawStr(RowStr[i]);
    h += gmWidth;
    MovePenTo(h, v);
    DrawNumR2(P->gameCount[i], 50);
    h += gmWidth;
    MovePenTo(h, v);
    DrawNumR2(P->wonCount[i], 50);
    h += gmWidth;
    MovePenTo(h, v);
    DrawNumR2(P->drawnCount[i], 50);
    h += gmWidth;
    MovePenTo(h, v);
    DrawNumR2(P->lostCount[i], 50);
    h += gmWidth;
    MovePenTo(h, v);
    DrawNumR2(pct, 50);
    DrawStr("%");
  }

} /* GameCountView::HandleUpdate */

/*--------------------------------------- ELO Summary Subview
 * ------------------------------------*/

#define sumWidth 75

static HEADER_COLUMN ELOSumHCTab[5] = {
    {"Current", 0, sumWidth},      {"Initial", 0, sumWidth},
    {"Min", 0, sumWidth},          {"Max", 0, sumWidth},
    {"Average Engine ELO", 0, -1},
};

ELOSummaryView::ELOSummaryView(CViewOwner *parent, CRect frame)
    : CView(parent, frame) {
  CRect r = bounds;
  r.Inset(1, 1);
  r.bottom = r.top + headerViewHeight;
  new DataHeaderView(this, r, false, true, 5, ELOSumHCTab);
} /* ELOSummaryView::ELOSummaryView */

void ELOSummaryView::HandleUpdate(CRect updateRect) {
  PLAYER_RATING *P = &(Prefs.PlayerELO);

  Draw3DFrame(bounds, &color_Gray, &color_White);

  // Clear contents
  CRect r = bounds;
  r.Inset(1, 1);
  r.top += headerViewHeight - 1;

  DrawRectFill(r, &color_White);
  SetStdForeColor();
  DrawRectFrame(r);
  SetForeColor(&color_Gray);

  for (INT i = 1; i <= 4; i++) {
    MovePenTo(r.left + sumWidth * i, r.top);
    DrawLineTo(r.left + sumWidth * i, r.bottom - 2);
  }

  // Draw contents
  INT v = r.top + tableRowHeight;
  INT h = r.left + 12;

  SetStdForeColor();
  SetBackColor(&color_White);
  SetFontStyle(fontStyle_Bold);
  MovePenTo(h, v);
  DrawNum(P->currELO);
  h += sumWidth;
  SetFontStyle(fontStyle_Plain);
  MovePenTo(h, v);
  DrawNum(P->initELO);
  h += sumWidth;

  if (P->gameCount[rating_Total] > 0) {
    MovePenTo(h, v);
    DrawNum(P->minELO);
    h += sumWidth;
    MovePenTo(h, v);
    DrawNum(P->maxELO);
    h += sumWidth;
    MovePenTo(h, v);
    DrawNum(P->sigmaELOsum / P->gameCount[rating_Total]);
  }
} /* ELOSummaryView::HandleUpdate */

/*--------------------------------------- ELO History Subview
 * ------------------------------------*/

static HEADER_COLUMN ELOHisHCTab[1] = {{"Player ELO History", 0, -1}};

ELOHistoryView::ELOHistoryView(CViewOwner *parent, CRect frame)
    : CView(parent, frame) {
  CRect r = bounds;
  r.Inset(1, 1);
  r.bottom = r.top + headerViewHeight;
  new DataHeaderView(this, r, false, true, 1, ELOHisHCTab);
} /* ELOHistoryView::ELOHistoryView */

#define elo2v(elo) (r.bottom - ((LONG)height * (elo - elo0)) / (elo1 - elo0))

void ELOHistoryView::HandleUpdate(CRect updateRect) {
  PLAYER_RATING *P = &(Prefs.PlayerELO);

  Draw3DFrame(bounds, &color_Gray, &color_White);

  // Clear contents
  CRect r = bounds;
  r.Inset(1, 1);
  r.top += headerViewHeight - 1;

  DrawRectFill(r, &color_White);
  SetStdForeColor();
  DrawRectFrame(r);

  // Draw ELO Graph
  r.Inset(15, 15);  // Calc interior graph rectangle
  r.left += 20;

  INT N = P->gameCount[rating_Total];  // Total number of played games
  INT elo = P->initELO;                // Player ELO after n games
  INT width = r.Width();               // Width of graph
  INT height = r.Height();             // Height of graph
  INT elo0 = 100 * (P->minELO / 100);  // Minimum ELO shown on graph (base line
  INT elo1 =
      100 * (P->maxELO / 100 + 1);  // Maximum ELO shown on graph (base line

  SetFontSize(9);

  for (INT e = elo0; e <= elo1; e += 100) {
    SetForeColor(&color_Gray), MovePenTo(r.left - 28, elo2v(e) + 4);
    DrawNum(e);
    if (e > elo0) {
      MovePenTo(r.left + 1, elo2v(e));
      SetForeColor(&color_LtGray);
      DrawLine(r.Width() - 2, 0);
    }
  }

  SetForeColor(&color_Gray),
      MovePenTo(r.left, r.bottom);  // Draw horisontal axis
  DrawLineTo(r.right - 1, r.bottom);
  MovePenTo(r.left, r.bottom);  // Draw vertical axis
  DrawLineTo(r.left, r.top);

  if (N == 0) return;

  SetForeColor(&color_Red);

  MovePenTo(r.left, elo2v(elo));

  INT nmax = (ProVersion() ? N : Min(10, N));
  for (INT n = 0; n < nmax; n++) {
    INT sigmaELO = (P->history[n] & 0x1FFF);  // Extract sigma ELO in game n
    REAL score = ((REAL)((P->history[n] >> 13) & 0x03)) / 2.0;

    elo = UpdateELO(elo, sigmaELO, score);
    DrawLineTo(r.left + ((n + 1) * width) / N, elo2v(elo));
  }
} /* ELOHistoryView::HandleUpdate */

/*--------------------------------------- Init ELO Dialog
 * ----------------------------------------*/

class CInitELODialog : public CDialog {
 public:
  CInitELODialog(CRect frame);

  virtual void HandlePushButton(CPushButton *ctl);

  CEditControl *cedit_InitELO;
};

CInitELODialog::CInitELODialog(CRect frame)
    : CDialog(nil, "Initial ELO", frame) {
  CRect inner = InnerRect();

  //--- Create the OK and Cancel buttons first ---
  cbutton_Cancel =
      new CPushButton(this, GetStr(sgr_Common, s_Cancel), CancelRect());
  cbutton_Default =
      new CPushButton(this, GetStr(sgr_Common, s_OK), DefaultRect());
  SetDefaultButton(cbutton_Default);

  CRect r = InnerRect();
  r.bottom = r.top + controlHeight_Text;
  r.right -= 60;
  new CTextControl(this, "Please specify your initial ELO rating", r);

  r = inner;
  r.bottom = r.top + controlHeight_Edit;
  r.left = r.right - 45;
  if (!RunningOSX()) r.Offset(0, -3);
  cedit_InitELO = new CEditControl(this, "1200", r, 4);

  CurrControl(cedit_InitELO);
} /* CInitELODialog::CInitELODialog */

void CInitELODialog::HandlePushButton(CPushButton *ctl) {
  if (ctl == cbutton_Default) {
    if (!cedit_InitELO->ValidateNumber(800, 3000, true)) {
      CurrControl(cedit_InitELO);
      NoteDialog(this, "Invalid ELO Rating",
                 "The specified ELO rating is invalid: It must be a whole "
                 "number between 800 and 3000.",
                 cdialogIcon_Error);
      return;
    }
  }

  CDialog::HandlePushButton(ctl);
} /* CInitELODialog::HandlePushButton */

static INT InitELODialog(void) {
  CRect frame(0, 0, 270, 70);
  if (RunningOSX()) frame.right += 65, frame.bottom += 30;
  theApp->CentralizeRect(&frame);

  CInitELODialog *dialog = new CInitELODialog(frame);
  dialog->Run();

  LONG initELO = -1;

  if (dialog->reply == cdialogReply_OK)
    dialog->cedit_InitELO->GetLong(&initELO);

  delete dialog;
  return initELO;
} /* InitELODialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                        RATE GAME DIALOG */
/*                                                                                                */
/**************************************************************************************************/

class CRateGameDialog : public CDialog {
 public:
  CRateGameDialog(CWindow *parent, CRect frame);

  CRadioButton *cradio_White, *cradio_Black;
};

CRateGameDialog::CRateGameDialog(CWindow *parent, CRect frame)
    : CDialog(parent, "Play Rated Game", frame, cdialogType_Sheet) {
  CRect inner = InnerRect();
  /*
     CRect rIcon(0,0,16,16);
     rIcon.Offset(inner.left, inner.top);
     new CIconControl(this, icon_Rate, rIcon);
  */
  CRect rt = inner;
  rt.bottom = rt.top + (RunningOSX() ? 22 : 15);
  new CTextControl(
      this, "Do you want to play with the White or the Black pieces?", rt);

  CRect rr(0, 0, 70, 20);
  rr.Offset(rt.left + 20, rt.bottom + 5);
  cradio_White = new CRadioButton(this, "White", 1, rr);
  rr.Offset(75, 0);
  cradio_Black = new CRadioButton(this, "Black", 1, rr);

  CRect rn = inner;
  rn.top = rr.bottom + 8;
  rn.bottom = CancelRect().top - 5;
  new CTextControl(
      this,
      "When the game is over your ELO rating will be adjusted accordingly. \\
If you abandon or interrupt the game before it's over (e.g. undo a move), the game will be considered lost.",
      rn, true, controlFont_Views);

  cbutton_Cancel = new CPushButton(this, "Cancel", CancelRect());
  cbutton_Default = new CPushButton(this, "Play", DefaultRect());
  SetDefaultButton(cbutton_Default);

  if (Prefs.PlayerELO.gameCount[rating_White] <=
      Prefs.PlayerELO.gameCount[rating_Black])
    cradio_White->Select();
  else
    cradio_Black->Select();

  CurrControl(cradio_White);
} /* CConfirmDialog::CConfirmDialog */

BOOL RateGameDialog(CWindow *parent, COLOUR *initPlayer) {
  CRect frame(0, 0, 360, 140);
  if (RunningOSX()) frame.right += 50, frame.bottom += 35;
  theApp->CentralizeRect(&frame);

  CRateGameDialog *dialog = new CRateGameDialog(parent, frame);
  dialog->Run();

  if (dialog->reply == cdialogReply_OK) {
    *initPlayer = (dialog->cradio_White->Selected() ? white : black);
    delete dialog;
    return true;
  } else {
    delete dialog;
    return false;
  }
} /* RateGameDialog */
