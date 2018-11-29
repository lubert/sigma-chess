/**************************************************************************************************/
/*                                                                                                */
/* Module  : LEVELDIALOG.C */
/* Purpose : This module implements the level dialog. */
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

#include "LevelDialog.h"
#include "SigmaApplication.h"
#include "SigmaStrings.h"

/*----------------------------------------- Dialog Class
 * -----------------------------------------*/

class CLevelDialog : public CDialog {
 public:
  CLevelDialog(CRect frame, LEVEL *level, BOOL engineMatch);
  virtual void HandlePushButton(CPushButton *ctrl);
  virtual void HandlePopupMenu(CPopupMenu *ctrl, INT itemID);
  virtual void HandleCheckBox(CCheckBox *ctrl);

  void CreateTimeMoves(void);
  void CreateTournament(void);
  void CreateAverage(void);
  void CreateFixedDepth(void);
  void CreateSolver(void);
  void CreateMateFinder(void);
  void CreateNovice(void);

  void WriteFields(void);
  BOOL ReadFields(void);
  BOOL InvalidField(INT theMode, CControl *ctl, CHAR *title, CHAR *message);

 private:
  LEVEL *level;  // Actual level structure
  LEVEL L;       // Temporary level structure (copied to "level" if OK).

  CRect R1, R2;  // Left and Right groupbox interior rectangles.

  // General Control Handling:
  CMenu *modeMenu;

  CPushButton *cbutton_SetDef;
  CTextControl *ctext_Mode;
  CPopupMenu *cpopup_Mode;
  CIconControl *cicon_Mode;

  CControl
      *CTab[playingModeCount + 1][18];  // Generic mode dependant control table

  // "TimeMoves" Controls:
  CMenu *timeMenu, *movesMenu;
  CPopupMenu *cpopup_Time, *cpopup_Moves;
  CEditControl *cedit_Time, *cedit_Moves;
  CCheckBox *ccheck_Fischer;
  CEditControl *cedit_TimeDelta;

  // "Tournament" Controls:
  CEditControl *cedit_W[3], *cedit_B[3], *cedit_M[3];

  // "Average" Controls
  CMenu *averageMenu;
  CPopupMenu *cpopup_Avg;
  CEditControl *cedit_Avg;

  // "Fixed Depth" Controls
  CEditControl *cedit_Fixed;

  // "Solver" Controls
  CEditControl *cedit_SolverTime, *cedit_SolverScore;

  // "Mate Finder" Controls
  CEditControl *cedit_Mate;

  // "Novice" Controls
  CMenu *noviceMenu;
  CPopupMenu *cpopup_Novice;
};

enum MISC_STRINGS {
  s_Title = 0,
  s_PlayingMode,
  s_Level,
  s_NoLevel,
  s_TimeHHMM,
  s_Moves,
  s_Fischer,
  s_TimeMMSS,
  s_DepthPlies,
  s_ScoreNN
};

#define miscStr(id) GetStr(sgr_LD_Misc, id)

/**************************************************************************************************/
/*                                                                                                */
/*                                         RUN LEVEL DIALOG */
/*                                                                                                */
/**************************************************************************************************/

BOOL Level_Dialog(LEVEL *level, BOOL engineMatch) {
  CRect frame(0, 0, 370, 250);
  if (RunningOSX()) frame.right += 60, frame.bottom += 30;
  theApp->CentralizeRect(&frame);

  CLevelDialog *dialog = new CLevelDialog(frame, level, engineMatch);
  dialog->Run();

  BOOL ok = (dialog->reply == cdialogReply_OK);
  delete dialog;
  return ok;
} /* Level_Dialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor
 * ------------------------------------------*/

CLevelDialog::CLevelDialog(CRect frame, LEVEL *theLevel, BOOL engineMatch)
    : CDialog(nil,
              (engineMatch ? "Engine Match Time Controls" : miscStr(s_Title)),
              frame, cdialogType_Modal) {
  level = theLevel;
  L = *theLevel;

  CRect inner = InnerRect();

  //--- Calc Rectangles ---

  R1.Set(inner.left, inner.top - 5, inner.left + inner.Width() / 2 - 3,
         inner.bottom - 30);
  CRect GR1 = R1;

  R2 = R1;
  R2.left = R1.right + 6;
  R2.right = inner.right;
  CRect GR2 = R2;

  R1.Inset(10, 20);  // Calc interior groupbox rectangles.
  R2.Inset(10, 20);

  //--- Create the OK and Cancel buttons first:

  CRect rSetDefault(inner.left, inner.bottom - controlHeight_PushButton,
                    inner.left + 75, inner.bottom);
  cbutton_SetDef =
      new CPushButton(this, GetStr(sgr_Common, s_Default), rSetDefault);
  cbutton_Cancel =
      new CPushButton(this, GetStr(sgr_Common, s_Cancel), CancelRect());
  cbutton_Default =
      new CPushButton(this, GetStr(sgr_Common, s_OK), DefaultRect());
  SetDefaultButton(cbutton_Default);

  //--- Create Playing Modes part ---

  CRect r = R1;
  r.top++;
  r.bottom = r.top + controlHeight_PopupMenu;
  if (!RunningOSX()) r.right -= 30;
  modeMenu = sigmaApp->BuildPlayingModeMenu(false);
  cpopup_Mode = new CPopupMenu(this, "", modeMenu, L.mode, r);
  if (engineMatch) cpopup_Mode->Enable(false);

  r.Offset(25, 0);
  r.left = r.right - 16;
  r.bottom = r.top + 16;
  cicon_Mode =
      (RunningOSX() ? nil : new CIconControl(this, ModeIcon[L.mode], r));

  r = R1;
  r.top += 25;
  if (RunningOSX()) r.top += 10;
  ctext_Mode = new CTextControl(this, "", r, true, controlFont_SmallSystem);

  new CGroupBox(this, miscStr(s_PlayingMode), GR1);

  //--- Create Playing Levels part ---

  for (INT i = 1; i <= playingModeCount; i++) CTab[i][0] = nil;

  CreateTimeMoves();
  CreateTournament();
  CreateAverage();
  CreateFixedDepth();
  CreateSolver();
  CreateMateFinder();
  CreateNovice();

  r = R2;
  r.top += 3;
  for (INT i = 1; i <= playingModeCount; i++)
    if (!CTab[i][0]) {
      CTab[i][0] = new CTextControl(this, miscStr(s_NoLevel), r, false,
                                    controlFont_SmallSystem);
      CTab[i][1] = nil;
    }

  new CGroupBox(this, miscStr(s_Level), GR2);

  //--- Perform misc Initialization ---

  WriteFields();

  CurrControl(cpopup_Mode);
  HandlePopupMenu(cpopup_Mode, L.mode);
} /* CLevelDialog::CLevelDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                   CREATE CONTROLS FOR EACH MODE */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------- Normal Playing Mode (Time/Moves)
 * -----------------------------*/

void CLevelDialog::CreateTimeMoves(void) {
  CControl **C = CTab[pmode_TimeMoves];
  INT i = 0;
  INT dv = (RunningOSX() ? 30 : 25);

  //--- Create "Time" control triplet ---

  CRect rt(0, 0, (RunningOSX() ? 95 : 80), controlHeight_Text);
  rt.Offset(R2.left, R2.top + 3);
  C[i++] = new CTextControl(this, miscStr(s_TimeHHMM), rt, false);

  CRect re(0, 0, 42, controlHeight_Edit);
  re.Offset(rt.right + 5, R2.top + (RunningOSX() ? 3 : 0));
  C[i++] = cedit_Time = new CEditControl(this, "", re, 5, false);

  CRect rp(0, 0, (RunningOSX() ? 20 : 18), controlHeight_PopupMenu);
  rp.Offset(re.right + (RunningOSX() ? 8 : 6), R2.top + 1);
  INT mins[11] = {0, 5, 10, 15, 20, 30, 45, 60, 90, 120, 150};
  timeMenu = new CMenu(GetStr(sgr_LD_TimeMenu, 0));
  for (INT m = 1; m <= 10; m++)
    timeMenu->AddItem(GetStr(sgr_LD_TimeMenu, m), mins[m]);
  C[i++] = cpopup_Time = new CPopupMenu(this, "", timeMenu, 1, rp, false);

  //--- Create "Moves" control triplet ---

  rt.Offset(0, dv);
  C[i++] = new CTextControl(this, miscStr(s_Moves), rt, false);

  re.Offset(0, dv);
  C[i++] = cedit_Moves = new CEditControl(this, "", re, 3, false);

  rp.Offset(0, dv);
  INT moves[7] = {0, 20, 30, 40, 50, 60, allMoves};
  movesMenu = new CMenu(GetStr(sgr_LD_MovesMenu, 0));
  for (INT m = 1; m <= 6; m++)
    movesMenu->AddItem(GetStr(sgr_LD_MovesMenu, m), moves[m]);
  C[i++] = cpopup_Moves = new CPopupMenu(this, "", movesMenu, 1, rp, false);

  //--- Create "Fischer Clock" controls ---

  CRect rf = R2;
  rf.top = rp.top + 50;
  rf.bottom = rf.top + controlHeight_CheckBox;
  C[i++] = ccheck_Fischer =
      new CCheckBox(this, miscStr(s_Fischer), false, rf, false);

  rf.Offset(0, (RunningOSX() ? 25 : 22));
  rf.right = rf.left + (RunningOSX() ? 112 : 90);
  C[i++] = new CTextControl(this, "Increment (secs)", rf, false);

  rf.Offset(rf.Width() + 5, (RunningOSX() ? 0 : -3));
  rf.right = rf.left + 32;
  rf.bottom = rf.top + controlHeight_Edit;
  C[i++] = cedit_TimeDelta = new CEditControl(this, "", rf, 3, false);

  C[i] = nil;
} /* CLevelDialog::CreateTimeMoves */

/*------------------------------------------- Tournament
 * -----------------------------------------*/

void CLevelDialog::CreateTournament(void) {
  CControl **C = CTab[pmode_Tournament];
  INT i = 0;
  INT dh = (RunningOSX() ? 55 : 48);
  INT dv = (RunningOSX() ? 30 : 25);

  CRect r(0, 0, 40, controlHeight_Text);
  r.Offset(R2.left + (RunningOSX() ? 17 : 20), R2.top + 3);
  C[i++] = new CTextControl(this, GetCommonStr(s_White), r, false);
  r.Offset(dh, 0);
  C[i++] = new CTextControl(this, GetCommonStr(s_Black), r, false);
  r.Offset(dh, 0);
  C[i++] = new CTextControl(this, miscStr(s_Moves), r, false);

  for (INT n = 0; n <= 2; n++)  // For each of the rows/time controls:
  {
    CHAR s[10];
    NumToStr(n + 1, s);

    r.Set(0, 0, 10, controlHeight_Text);
    r.Offset(R2.left, R2.top + (RunningOSX() ? 28 : 23) + dv * n);
    C[i++] = new CTextControl(this, s, r, false);

    r.Set(0, 0, 41, controlHeight_Edit);
    r.Offset(R2.left + 20, R2.top + (RunningOSX() ? 28 : 20) + dv * n);
    C[i++] = cedit_W[n] = new CEditControl(this, "", r, 5, false);
    r.Offset(dh, 0);
    C[i++] = cedit_B[n] = new CEditControl(this, "", r, 5, false);
    r.Offset(dh, 0);
    r.right -= 10;
    C[i++] = cedit_M[n] = new CEditControl(this, "", r, 3, false);
  }

  cedit_M[2]->Enable(false);
  cedit_M[2]->SetText(GetCommonStr(s_All));

  C[i] = nil;
} /* CLevelDialog::CreateTournament */

/*-------------------------------------- Average Playing Mode
 * ------------------------------------*/

void CLevelDialog::CreateAverage(void) {
  CControl **C = CTab[pmode_Average];
  INT i = 0;

  CRect rt(0, 0, (RunningOSX() ? 95 : 80), controlHeight_Text);
  rt.Offset(R2.left, R2.top + 3);
  C[i++] = new CTextControl(this, miscStr(s_TimeMMSS), rt, false);

  CRect re(0, 0, 42, controlHeight_Edit);
  re.Offset(rt.right + 5, R2.top + (RunningOSX() ? 3 : 0));
  C[i++] = cedit_Avg = new CEditControl(this, "", re, 5, false);

  CRect rp(0, 0, (RunningOSX() ? 20 : 18), controlHeight_PopupMenu);
  rp.Offset(re.right + (RunningOSX() ? 8 : 6), R2.top + 1);
  INT avg[11] = {0, 5, 10, 15, 20, 30, 45, 60, 90, 120, 150};
  averageMenu = new CMenu(GetStr(sgr_LD_AvgMenu, 0));
  for (INT m = 1; m <= 10; m++)
    averageMenu->AddItem(GetStr(sgr_LD_AvgMenu, m), avg[m]);
  C[i++] = cpopup_Avg = new CPopupMenu(this, "", averageMenu, 1, rp, false);

  C[i] = nil;
} /* CLevelDialog::CreateAverage */

/*------------------------------------ Fixed Depth Playing Mode
 * ----------------------------------*/

void CLevelDialog::CreateFixedDepth(void) {
  CControl **C = CTab[pmode_FixedDepth];
  INT i = 0;

  CRect rt(0, 0, (RunningOSX() ? 90 : 80), controlHeight_Text);
  rt.Offset(R2.left, R2.top + 3);
  CRect re(0, 0, 42, controlHeight_Edit);
  re.Offset(rt.right + 5, R2.top + (RunningOSX() ? 3 : 0));

  C[i++] = cedit_Fixed = new CEditControl(this, "", re, 2, false);
  C[i++] = new CTextControl(this, miscStr(s_DepthPlies), rt, false);

  C[i] = nil;
} /* CLevelDialog::CreateFixedDepth */

/*-------------------------------------- Solver Playing Mode
 * -------------------------------------*/

void CLevelDialog::CreateSolver(void) {
  CControl **C = CTab[pmode_Solver];
  INT i = 0;
  INT dv = (RunningOSX() ? 30 : 25);

  CRect rt(0, 0, (RunningOSX() ? 100 : 82), controlHeight_Text);
  rt.Offset(R2.left, R2.top + 3);
  CRect re(0, 0, 50, controlHeight_Edit);
  re.Offset(rt.right + 5, R2.top + (RunningOSX() ? 3 : 0));

  C[i++] = new CTextControl(this, miscStr(s_TimeMMSS), rt, false);
  C[i++] = cedit_SolverTime = new CEditControl(this, "", re, 5, false);

  rt.Offset(0, dv);
  re.Offset(0, dv);
  C[i++] = new CTextControl(this, miscStr(s_ScoreNN), rt, false);
  C[i++] = cedit_SolverScore = new CEditControl(this, "", re, 6, false);

  C[i] = nil;
} /* CLevelDialog::CreateSolver */

/*------------------------------------ Mate Solver Playing Mode
 * ----------------------------------*/

void CLevelDialog::CreateMateFinder(void) {
  CControl **C = CTab[pmode_MateFinder];
  INT i = 0;

  CRect rt(0, 0, (RunningOSX() ? 65 : 80), controlHeight_Text);
  rt.Offset(R2.left, R2.top + 3);
  CRect re(0, 0, 42, controlHeight_Edit);
  re.Offset(rt.right + 5, R2.top + (RunningOSX() ? 3 : 0));

  C[i++] = new CTextControl(this, GetCommonStr(s_MateIn), rt, false);
  C[i++] = cedit_Mate = new CEditControl(this, "", re, 2, false);

  C[i] = nil;
} /* CLevelDialog::CreateMateFinder */

/*-------------------------------------- Novice Playing Mode
 * -------------------------------------*/

void CLevelDialog::CreateNovice(void) {
  CControl **C = CTab[pmode_Novice];
  INT i = 0;

  CRect r = R2;
  r.top++;
  r.bottom = r.top + controlHeight_PopupMenu;
  noviceMenu = new CMenu("Novice");
  noviceMenu->AddItem("[1] Easiest", 1);
  noviceMenu->AddItem("[2] Easy", 2);
  noviceMenu->AddItem("[3] Less easy", 3);
  noviceMenu->AddItem("[4] Not easy", 4);
  noviceMenu->AddItem("[5] Good", 5);
  noviceMenu->AddItem("[6] Better", 6);
  noviceMenu->AddItem("[7] Even Better", 7);
  noviceMenu->AddItem("[8] Best", 8);

  C[i++] = cpopup_Novice =
      new CPopupMenu(this, "", noviceMenu, L.Novice.level, r, false);

  C[i] = nil;
} /* CLevelDialog::CreateNovice */

/**************************************************************************************************/
/*                                                                                                */
/*                              READ/WRITE LEVEL FROM/TO DIALOG FIELDS */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------- Store Level data in dialog fields
 * -----------------------------*/
// Copies the LEVEL structure L to the dialog fields.

void CLevelDialog::WriteFields(void) {
  CHAR s[10];  // Generic character buffer.

  //--- Time/Moves ---
  FormatHHMM(L.TimeMoves.time / 60, s);
  cedit_Time->SetText(s);
  cpopup_Time->Set(L.TimeMoves.time / 60);

  if (L.TimeMoves.moves == allMoves)
    cedit_Moves->SetText(GetCommonStr(s_All));
  else {
    NumToStr(L.TimeMoves.moves, s);
    cedit_Moves->SetText(s);
  }
  cpopup_Moves->Set(L.TimeMoves.moves);

  BOOL useFischer = (L.TimeMoves.clockType == clock_Fischer);
  ccheck_Fischer->Check(useFischer);
  NumToStr(L.TimeMoves.delta, s);
  cedit_TimeDelta->Enable(useFischer);
  cedit_TimeDelta->SetText(s);

  //--- Tournament ---
  for (INT n = 0; n <= 2; n++) {
    FormatHHMM(L.Tournament.wtime[n] / 60, s);
    cedit_W[n]->SetText(s);
    FormatHHMM(L.Tournament.btime[n] / 60, s);
    cedit_B[n]->SetText(s);
    if (n < 2)
      NumToStr(L.Tournament.moves[n], s);
    else
      CopyStr(GetCommonStr(s_All), s);
    cedit_M[n]->SetText(s);
  }

  //--- Average ---
  FormatHHMM(L.Average.secs, s);
  cedit_Avg->SetText(s);
  cpopup_Avg->Set(L.Average.secs);

  //--- Fixed Depth ---
  NumToStr(L.FixedDepth.depth, s);
  cedit_Fixed->SetText(s);

  //--- Solver ---
  FormatHHMM(L.Solver.timeLimit, s);
  cedit_SolverTime->SetText(s);
  if (L.Solver.scoreLimit == maxVal)
    s[0] = 0;
  else
    CalcScoreStr(s, L.Solver.scoreLimit);
  cedit_SolverScore->SetText(s);

  //--- Mate Finder ---
  NumToStr(L.MateFinder.mateDepth, s);
  cedit_Mate->SetText(s);

  //--- Novice ---
  cpopup_Novice->Set(L.Novice.level);
} /* CLevelDialog::WriteFields */

/*-------------------------------- Read dialog fields to Level data
 * ------------------------------*/
// Copies the dialog fields to the LEVEL structure L.s

BOOL CLevelDialog::ReadFields(void) {
  CHAR s[10], s1[10];  // Generic character buffers.
  LONG n;              // Generic return value.

  //--- Time/Moves ---
  cedit_Time->GetTitle(s);
  if (!ParseHHMM(s, &L.TimeMoves.time))
    return InvalidField(pmode_TimeMoves, cedit_Time, "Invalid Time Format",
                        "Please use the format 'hh:mm'");
  L.TimeMoves.time *= 60;

  cedit_Moves->GetTitle(s);
  if (StrToNum(s, &n) && n >= 10 & n <= 200)
    L.TimeMoves.moves = n;
  else if (EqualStr(s, "All"))
    L.TimeMoves.moves = allMoves;
  else
    return InvalidField(pmode_TimeMoves, cedit_Moves, "Invalid Moves Format",
                        "The 'Moves' field must be a number between 10 and "
                        "200, or the text 'All'");

  L.TimeMoves.clockType =
      (ccheck_Fischer->Checked() ? clock_Fischer : clock_Normal);
  if (!ccheck_Fischer->Checked())
    L.TimeMoves.delta = Max(1, L.TimeMoves.time / 60);
  else {
    cedit_TimeDelta->GetTitle(s);
    if (StrToNum(s, &n) && n >= 1 & n <= 999)
      L.TimeMoves.delta = n;
    else
      return InvalidField(pmode_TimeMoves, cedit_TimeDelta,
                          "Invalid Fischer Increment",
                          "The 'Fischer Increment' field must be a whole "
                          "number of seconds between 1 and 999");
  }

  //--- Tournament ---
  for (INT i = 0; i <= 2; i++) {
    cedit_W[i]->GetTitle(s);
    if (!ParseHHMM(s, &L.Tournament.wtime[i]))
      return InvalidField(pmode_Tournament, cedit_W[i], "Invalid Time Format",
                          "Please use the format 'hh:mm'");
    L.Tournament.wtime[i] *= 60;

    cedit_B[i]->GetTitle(s);
    if (!ParseHHMM(s, &L.Tournament.btime[i]))
      return InvalidField(pmode_Tournament, cedit_B[i], "Invalid Time Format",
                          "Please use the format 'hh:mm'");
    L.Tournament.btime[i] *= 60;

    if (i < 2) {
      cedit_M[i]->GetTitle(s);
      if (StrToNum(s, &n) && n >= 10 & n <= 200)
        L.Tournament.moves[i] = n;
      else
        return InvalidField(
            pmode_Tournament, cedit_M[i], "Invalid Moves Format",
            "The 'Moves' field must be a number between 10 and 200");
    }
  }

  //--- Average ---
  cedit_Avg->GetTitle(s);
  if (!ParseHHMM(s, &L.Average.secs))
    return InvalidField(pmode_Average, cedit_Avg, "Invalid Time Format",
                        "Please use the format 'mm:ss'");

  //--- Fixed Depth ---
  cedit_Fixed->GetTitle(s);
  if (!StrToNum(s, &n) || n < 1 || n > maxSearchDepth)
    return InvalidField(pmode_FixedDepth, cedit_Fixed, "Invalid Search Depth",
                        "The fixed search depth must be a number of half moves "
                        "(plies) between 1 and 50");
  L.FixedDepth.depth = n;

  //--- Solver ---
  cedit_SolverTime->GetTitle(s);
  cedit_SolverScore->GetTitle(s1);
  if (!s[0] && !s1[0])
    return InvalidField(pmode_Solver, cedit_SolverTime,
                        "Invalid Time/Score Limit",
                        "You must fill in at least one of the two fields");
  if (!s[0])
    L.Solver.timeLimit = -1;
  else if (!ParseHHMM(s, &L.Solver.timeLimit))
    return InvalidField(pmode_Solver, cedit_SolverTime, "Invalid Time Limit",
                        "Please use the format 'mm:ss'");
  if (!s1[0])
    L.Solver.scoreLimit = maxVal;
  else if (!ParseScoreStr(s1, &L.Solver.scoreLimit))
    return InvalidField(
        pmode_Solver, cedit_SolverScore, "Invalid Score Limit",
        "Please use the format '±nn' or '±nn.nn' (e.g. '2.75', '-1.5' or "
        "'+1'). Scores are always specified in units of pawns.");

  //--- Mate Finder ---
  cedit_Mate->GetTitle(s);
  if (!StrToNum(s, &n) || n < 1 || n > maxSearchDepth / 2)
    return InvalidField(
        pmode_MateFinder, cedit_Mate, "Invalid Mate Depth",
        "The mate depth must be a number of moves between 1 and 25");
  L.MateFinder.mateDepth = n;

  //--- Novice ---
  L.Novice.level = cpopup_Novice->Get();

  return true;
} /* CLevelDialog::ReadFields */

BOOL CLevelDialog::InvalidField(INT theMode, CControl *ctl, CHAR *title,
                                CHAR *message) {
  if (L.mode != theMode)
    HandlePopupMenu(cpopup_Mode, theMode);  // First select the problem sheet
  if (ctl) CurrControl(ctl);
  NoteDialog(this, title, message);
  return false;
} /* CLevelDialog::InvalidField */

/*------------------------------------------- Utility
 * -------------------------------------------*/

void FormatHHMM(INT mins, CHAR *s) {
  if (mins == -1)
    s[0] = 0;
  else {
    INT hh = mins / 60, mm = mins % 60;

    s[0] = '0' + hh / 10;
    s[1] = '0' + hh % 10;
    s[2] = ':';
    s[3] = '0' + mm / 10;
    s[4] = '0' + mm % 10;
    s[5] = 0;
  }
} /* FormatHHMM */

BOOL ParseHHMM(CHAR *s, INT *mins)  // Returns false if parse failed.
{
  if (IsDigit(s[0]) && IsDigit(s[1]) && s[2] == ':' && IsDigit(s[3]) &&
      IsDigit(s[4]) && !s[5]) {
    INT hh = (s[0] - '0') * 10 + (s[1] - '0');
    INT mm = (s[3] - '0') * 10 + (s[4] - '0');
    if (hh < 60 || mm < 60) {
      *mins = 60 * hh + mm;
      return true;
    }
  }

  return false;
} /* ParseHHMM */

/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void CLevelDialog::HandlePushButton(CPushButton *ctl) {
  // Perform validation if user clicks "OK":
  if (ctl == cbutton_Default) {
    if (!ReadFields()) return;
    if (cpopup_Mode->Get() == pmode_Monitor &&
        !ProVersionDialog(this,
                          "Monitor mode is not available in Sigma Chess Lite."))
      return;
    *level = L;
  } else if (ctl == cbutton_SetDef) {
    Level_Reset(&L);
    WriteFields();
  }

  // Validation succeeded (or user pressed "Cancel") -> Call inherited function:
  CDialog::HandlePushButton(ctl);
} /* CLevelDialog::HandlePushButton */

void CLevelDialog::HandlePopupMenu(CPopupMenu *ctl, INT itemID) {
  if (ctl == cpopup_Mode) {
    ctext_Mode->SetTitle(GetStr(sgr_LD_ModesDescr, itemID - 1));
    if (cicon_Mode) cicon_Mode->Set(ModeIcon[itemID]);

    for (INT i = 0; CTab[L.mode][i]; i++) CTab[L.mode][i]->Show(false);

    L.mode = itemID;

    for (INT i = 0; CTab[L.mode][i]; i++) CTab[L.mode][i]->Show(true);
  } else if (ctl == cpopup_Time) {
    CHAR s[6];
    FormatHHMM(itemID, s);
    cedit_Time->SetTitle(s);
  } else if (ctl == cpopup_Moves) {
    CHAR s[6];
    INT moves = itemID;
    if (moves == allMoves)
      cedit_Moves->SetText(GetCommonStr(s_All));
    else {
      NumToStr(moves, s);
      cedit_Moves->SetText(s);
    }
  } else if (ctl == cpopup_Avg) {
    CHAR s[6];
    FormatHHMM(itemID, s);
    cedit_Avg->SetText(s);
  }
} /* CLevelDialog::HandlePopupMenu */

void CLevelDialog::HandleCheckBox(CCheckBox *ctrl) {
  CDialog::HandleCheckBox(ctrl);

  if (ctrl == ccheck_Fischer) cedit_TimeDelta->Enable(ctrl->Checked());
} /* CLevelDialog::HandleCheckBox */
