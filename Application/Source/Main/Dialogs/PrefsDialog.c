/**************************************************************************************************/
/*                                                                                                */
/* Module  : PREFSDIALOG.C */
/* Purpose : This module implements the application wide preferences dialog. */
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

#include "PrefsDialog.h"

#include "CDialog.h"
#include "CMemory.h"
#include "Engine.f"
#include "EngineMatchDialog.h"
#include "GameWindow.h"
#include "SigmaApplication.h"
#include "SigmaPrefs.h"
#include "SigmaStrings.h"
#include "TransTabManager.h"

// The preferences dialog provides access to various seldomly used/changed
// settings, which are divided into the following groups:

/**************************************************************************************************/
/*                                                                                                */
/*                                        DIALOG CLASS */
/*                                                                                                */
/**************************************************************************************************/

class CPrefsDialog : public CDialog {
 public:
  CPrefsDialog(CRect frame);

  virtual void HandlePushButton(CPushButton *ctrl);
  virtual void HandlePopupMenu(CPopupMenu *ctrl, INT itemNo);
  virtual void HandleCheckBox(CCheckBox *ctrl);

  void CreateGeneral(void);
  void CreateGames(void);
  void CreateCollections(void);
  // void CreatePGN (void);
  void CreateMessages(void);
  void CreateMisc(void);
  void CreateAnalysisFormat(void);
  void CreateMemory(void);
  void CreateTransTab(void);

 private:
  CControl *CTab[prefsGroupCount + 1][20];
  PREFS *P;
  INT totalTransMem;

  CPopupMenu *cpopup;
  CRect R;  // Inner client rect for each "group"

  //--- General ---
  CEditControl *cedit_Player;
  CRadioButton *cradio_MenuIcon[3];  // 0 = none, 1 = common, 2 = all
  CCheckBox *ccheck_Enable3D;

  //--- Games ---
  CRadioButton *cradio_InitPos, *cradio_FinalPos;
  CCheckBox *ccheck_TurnPlayer, *ccheck_FutureMoves, *ccheck_HiliteCurr,
      *ccheck_AskSave;
  CCheckBox *ccheck_SaveNative;
  CScrollBar *cslider_MoveSpeed;

  //--- Collections ---
  CCheckBox *ccheck_AutoNameCol, *ccheck_KeepColumnWidths;

  //--- PGN ---
  CCheckBox *ccheck_SkipMoveNumSep, *ccheck_OpenSingle, *ccheck_FileExtFilter,
      *ccheck_KeepNewLines;

  //--- Messages ---
  CCheckBox *ccheck_AnnounceMate, *ccheck_Announce1stMate, *ccheck_GameOver,
      *ccheck_Resign, *ccheck_DrawOffer;
  CCheckBox *ccheck_WoodSound, *ccheck_MoveBeep;

  //--- Misc ---
  CCheckBox *ccheck_PrintHeaders, *ccheck_HTMLGifNotify;

  //--- Analysis Formatting ---
  CRadioButton *cradio_NumRel, *cradio_NumAbs, *cradio_Glyph;
  CCheckBox *ccheck_ShowScore, *ccheck_ShowDepth;
  CCheckBox *ccheck_ShowTime, *ccheck_ShowNodes, *ccheck_ShowNSec;
  CCheckBox *ccheck_ShowMainLine;
  CRadioButton *cradio_ShortFormat, *cradio_LongFormat;

  //--- Memory ---
  CPopupMenu *cpopup_ResMem;

  //--- Transposition Tables ---
  CCheckBox *ccheck_UseTrans, *ccheck_UseTransMF;
  CTextControl *ctext_TotalTrans;
  CPushButton *cbutton_SetTotalTrans;
  CPopupMenu *cpopup_TransMem;

  CPushButton *cbutton_Factory;
};

/**************************************************************************************************/
/*                                                                                                */
/*                                     MAIN DIALOG ROUTINE */
/*                                                                                                */
/**************************************************************************************************/

static INT group = prefs_General;

void PrefsDialog(INT selectedGroup) {
  //   if (EngineMatch.gameWin)
  //   	NoteDialog(nil, "Engine Match Running", "You cannot change preferences
  //   while an engine match is running");

  CRect frame(0, 0, 370, 300);
  if (RunningOSX()) frame.right += 100, frame.bottom += 65;
  theApp->CentralizeRect(&frame);

  if (selectedGroup > 0) group = selectedGroup;

  CPrefsDialog *dialog = new CPrefsDialog(frame);
  dialog->Run();
  delete dialog;
} /* PrefsDialog */

/**************************************************************************************************/
/*                                                                                                */
/*                                        CONSTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CPrefsDialog::CPrefsDialog(CRect frame)
    : CDialog(nil, "Sigma Chess Preferences", frame, cdialogType_Modal) {
  P = &Prefs;
  R = InnerRect();
  R.Inset(0, (RunningOSX() ? 32 : 37));

  //--- Dividers ---
  CRect r = InnerRect();
  r.top += controlHeight_PopupMenu / 2;
  r.bottom = r.top + 2;
  CRect rd = r;
  // new CDivider(this, r);
  r = R;
  r.top = r.bottom - 2;
  new CDivider(this, r);

  //--- Popup Menu ---
  r = InnerRect();
  r.left += 15;
  r.bottom = r.top + controlHeight_PopupMenu;
  r.right = r.left + (RunningOSX() ? 180 : 140);
  if (RunningOSX()) r.Offset(0, -2);
  CMenu *cmenu = new CMenu("");
  cmenu->AddItem("General", prefs_General);
  cmenu->AddSeparator();
  cmenu->AddItem("Games", prefs_Games);
  cmenu->AddItem("Collections & PGN", prefs_Collections);
  cmenu->AddSeparator();
  cmenu->AddItem("Scores & Analysis", prefs_ScoreAnalysis);
  cmenu->AddItem("Messages & Sounds", prefs_Messages);
  cmenu->AddItem("Misc", prefs_Misc);
  cmenu->AddSeparator();
  if (!RunningOSX()) cmenu->AddItem("Memory", prefs_Memory);
  cmenu->AddItem("Transposition Tables", prefs_TransTab);
  cpopup = new CPopupMenu(this, "", cmenu, group, r);

  rd.right = r.left - 3;
  new CDivider(this, rd);
  rd.left = r.right + 3;
  rd.right = R.right;
  new CDivider(this, rd);

  //--- Create each control group ---
  if (!RunningOSX()) R.top -= 10;
  CreateGeneral();
  CreateGames();
  CreateCollections();
  CreateMessages();
  CreateMisc();
  CreateAnalysisFormat();
  CreateMemory();
  CreateTransTab();

  //--- Finally create OK/Cancel/Default buttons ---
  CRect inner = InnerRect();
  CRect rSetDefault(inner.left, inner.bottom - controlHeight_PushButton,
                    inner.left + 75, inner.bottom);
  cbutton_Factory =
      new CPushButton(this, GetStr(sgr_Common, s_Default), rSetDefault);
  cbutton_Cancel =
      new CPushButton(this, GetStr(sgr_Common, s_Cancel), CancelRect());
  cbutton_Default =
      new CPushButton(this, GetStr(sgr_Common, s_OK), DefaultRect());
  SetDefaultButton(cbutton_Default);

  CurrControl(cpopup);
  HandlePopupMenu(cpopup, group);
} /* CPrefsDialog::CPrefsDialog */

void CPrefsDialog::CreateGeneral(void) {
  CControl **C = CTab[prefs_General];
  INT i = 0;
  INT dv = (RunningOSX() ? 22 : 18);

  CRect r(0, 0, 68, controlHeight_Text);
  if (RunningOSX()) r.right += 25;
  r.Offset(R.left, R.top + (RunningOSX() ? 0 : 3));
  C[i++] = new CTextControl(this, "Player name", r, false);

  r = R;
  r.bottom = r.top + controlHeight_Edit;
  r.left += 70 + (RunningOSX() ? 28 : 0);
  r.right -= 3;
  C[i++] = cedit_Player =
      new CEditControl(this, P->General.playerName, r, nameStrLen, false);

  r = R;
  r.bottom = r.top + controlHeight_CheckBox;
  r.Offset(0, dv + 7);
  C[i++] = new CTextControl(this, "Show menu icons for:", r, false);
  r.Offset(18, dv - 2);
  r.right -= 15;
  C[i++] = cradio_MenuIcon[2] =
      new CRadioButton(this, "all menu items", 1, r, false);
  r.Offset(0, dv);
  C[i++] = cradio_MenuIcon[1] =
      new CRadioButton(this, "common menu items", 1, r, false);
  r.Offset(0, dv);
  C[i++] = cradio_MenuIcon[0] = new CRadioButton(this, "none", 1, r, false);
  r.Offset(-18, dv + 7);
  C[i++] = ccheck_Enable3D = new CCheckBox(
      this, "Enable/preload 3D board (takes effect after restart)",
      P->General.enable3D, r, false);

  C[i] = nil;

  cradio_MenuIcon[P->General.menuIcons]->Select();
} /* CPrefsDialog::CreateGeneral */

void CPrefsDialog::CreateGames(void) {
  CControl **C = CTab[prefs_Games];
  INT i = 0;
  INT dv = (RunningOSX() ? 22 : 18);

  CRect r = R;
  r.bottom = r.top + controlHeight_CheckBox;
  r.right -= 10;
  C[i++] = new CTextControl(this, "When opening a game:", r, false);
  r.Offset(18, dv - 2);
  C[i++] = cradio_InitPos =
      new CRadioButton(this, "Show initial position", 2, r, false);
  r.Offset(0, dv);
  C[i++] = cradio_FinalPos =
      new CRadioButton(this, "Show final position", 2, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_TurnPlayer =
      new CCheckBox(this, "Turn board if Black is identical to Player Name",
                    P->Games.turnPlayer, r, false);
  r.Offset(-18, dv + 10);

  C[i++] = new CTextControl(this, "When saving a game:", r, false);
  r.Offset(18, dv - 2);
  C[i++] = ccheck_SaveNative = new CCheckBox(
      this, "Always use native file format", P->Games.saveNative, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_AskSave =
      new CCheckBox(this, "Ask if user wants to save game changes",
                    P->Games.askGameSave, r, false);
  r.Offset(-18, dv + 10);

  C[i++] = ccheck_FutureMoves =
      new CCheckBox(this, "Show future moves in move list",
                    P->Games.showFutureMoves, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_HiliteCurr =
      new CCheckBox(this, "Hilite current move in move list",
                    P->Games.hiliteCurrMove, r, false);
  r.Offset(0, dv + 10);

  r.right = r.left + (RunningOSX() ? 90 : 75);
  C[i++] = new CTextControl(this, "Move Speed", r, false);
  r.left = r.right + 5;
  r.right = r.left + 200;
  r.bottom = r.top + controlWidth_ScrollBar;
  C[i++] = cslider_MoveSpeed = new CScrollBar(this, 1, 100, P->Games.moveSpeed,
                                              10, r, false, true, true);

  C[i] = nil;

  if (P->Games.gotoFinalPos)
    cradio_FinalPos->Select();
  else
    cradio_InitPos->Select();
} /* CPrefsDialog::CreateGames */

void CPrefsDialog::CreateCollections(void) {
  CControl **C = CTab[prefs_Collections];
  INT i = 0;
  INT dv = (RunningOSX() ? 22 : 18);

  CRect r = R;
  r.bottom = r.top + controlHeight_CheckBox;
  r.right -= 10;
  C[i++] = ccheck_AutoNameCol = new CCheckBox(
      this, "Open PGN files directly (auto assign collection name)",
      P->Collections.autoName, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_KeepColumnWidths = new CCheckBox(
      this, "Remember column widths", P->Collections.keepColWidths, r, false);
  r.Offset(0, dv);
  r.Offset(0, dv);
  C[i++] = ccheck_SkipMoveNumSep =
      new CCheckBox(this, "Skip move separator (i.e �1.d4� instead of �1. d4�)",
                    P->PGN.skipMoveSep, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_KeepNewLines = new CCheckBox(
      this, "Preserve newlines in annotations during PGN import/export",
      P->PGN.keepNewLines, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_OpenSingle =
      new CCheckBox(this, "Open single game PGN files in a Game window",
                    P->PGN.openSingle, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_FileExtFilter =
      new CCheckBox(this, "Only open files ending with �.PGN� (or �.EPD�)",
                    P->PGN.fileExtFilter, r, false);
  r.Offset(0, dv);

  C[i] = nil;
} /* CPrefsDialog::CreateCollection */

void CPrefsDialog::CreateMessages(void) {
  CControl **C = CTab[prefs_Messages];
  INT i = 0;
  INT dv = (RunningOSX() ? 22 : 18);

  CRect r = R;
  r.bottom = r.top + controlHeight_CheckBox;
  r.right -= 10;
  C[i++] = ccheck_AnnounceMate =
      new CCheckBox(this, "Announce mate", P->Messages.announceMate, r, false);
  r.Offset(18, dv);
  C[i++] = ccheck_Announce1stMate =
      new CCheckBox(this, "Only announce mate once per game",
                    P->Messages.announce1stMate, r, false);
  r.Offset(-18, dv);
  C[i++] = ccheck_GameOver = new CCheckBox(this, "Show �Game Over� dialogs",
                                           P->Messages.gameOverDlg, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_Resign =
      new CCheckBox(this, "Sigma Chess can resign in hopeless positions",
                    P->Messages.canResign, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_DrawOffer =
      new CCheckBox(this, "Sigma Chess can offer draws in level positions",
                    P->Messages.canOfferDraw, r, false);
  r.Offset(0, dv);
  r.Offset(0, dv);
  C[i++] = ccheck_WoodSound =
      new CCheckBox(this, "Play �wood� sound when moving pieces",
                    P->Sound.woodSound, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_MoveBeep =
      new CCheckBox(this, "Beep when Sigma Chess performs a new move",
                    P->Sound.moveBeep, r, false);
  C[i] = nil;
} /* CPrefsDialog::CreateMessages */

void CPrefsDialog::CreateMisc(void) {
  CControl **C = CTab[prefs_Misc];
  INT i = 0;
  INT dv = (RunningOSX() ? 22 : 18);

  CRect r = R;
  r.bottom = r.top + controlHeight_CheckBox;
  C[i++] = ccheck_PrintHeaders =
      new CCheckBox(this, "Include game headers when printing",
                    P->Misc.printPageHeaders, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_HTMLGifNotify = new CCheckBox(
      this, "Remind me that a gif folder is needed for HTML export",
      P->Misc.HTMLGifReminder, r, false);

  C[i] = nil;
} /* CPrefsDialog::CreateMisc */

void CPrefsDialog::CreateAnalysisFormat(void) {
  CControl **C = CTab[prefs_ScoreAnalysis];
  INT i = 0;
  INT dv = (RunningOSX() ? 22 : 18);
  CRect r = R;

  r.bottom = r.top + controlHeight_CheckBox;
  C[i++] = new CTextControl(
      this, "Score Notation (numerical scores are in units of pawns):", r,
      false, controlFont_Views);
  r.Offset(18, dv);
  C[i++] = cradio_NumRel = new CRadioButton(
      this, "Relative Numerical (seen from side to move)", 3, r, false);
  r.Offset(0, dv);
  C[i++] = cradio_NumAbs = new CRadioButton(
      this, "Absolute Numerical (seen from White)", 3, r, false);
  r.Offset(0, dv);
  C[i++] = cradio_Glyph = new CRadioButton(
      this, "Position classification glyphs (seen from White)", 3, r, false);
  r.Offset(0, dv);
  switch (P->AnalysisFormat.scoreNot) {
    case scoreNot_NumRel:
      cradio_NumRel->Select();
      break;
    case scoreNot_NumAbs:
      cradio_NumAbs->Select();
      break;
    case scoreNot_Glyph:
      cradio_Glyph->Select();
      break;
  }
  r.Offset(-18, 10);

  r.bottom = r.top + 30;
  C[i++] = new CTextControl(
      this,
      "Select which parts of the analysis to include when analyzing "
      "games/collections or copying analysis to the clipboard:",
      r, false, controlFont_Views);
  r.Offset(18, (RunningOSX() ? 40 : 32));

  r.bottom = r.top + controlHeight_CheckBox;
  r.right = r.left + 130;
  C[i++] = ccheck_ShowScore =
      new CCheckBox(this, "Score", P->AnalysisFormat.showScore, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_ShowDepth =
      new CCheckBox(this, "Depth", P->AnalysisFormat.showDepth, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_ShowMainLine = new CCheckBox(
      this, "Main Line", P->AnalysisFormat.showMainLine, r, false);
  r.Offset(150, -2 * dv);
  C[i++] = ccheck_ShowTime =
      new CCheckBox(this, "Time", P->AnalysisFormat.showTime, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_ShowNodes =
      new CCheckBox(this, "Nodes", P->AnalysisFormat.showNodes, r, false);
  r.Offset(0, dv);
  C[i++] = ccheck_ShowNSec =
      new CCheckBox(this, "Nodes/Sec", P->AnalysisFormat.showNSec, r, false);
  r.Offset(-150, dv + 5);

  r.right = r.left + 200;
  C[i++] = cradio_ShortFormat =
      new CRadioButton(this, "Short format (single line)", 4, r, false);
  r.Offset(0, dv);
  C[i++] = cradio_LongFormat =
      new CRadioButton(this, "Long format (multiple lines)", 4, r, false);
  r.Offset(0, dv);

  C[i] = nil;

  if (P->AnalysisFormat.shortFormat)
    cradio_ShortFormat->Select();
  else
    cradio_LongFormat->Select();
} /* CPrefsDialog::CreateAnalysysFormat */

static void FormatBytes(ULONG bytes, CHAR *s);

#define memValuePos (RunningOSX() ? 278 : 210)

void CPrefsDialog::CreateMemory(void) {
  if (RunningOSX()) return;

  CControl **C = CTab[prefs_Memory];
  INT i = 0;
  CRect r = R, r1;
  CHAR s[100];

  r.bottom = r.top + 45;
  C[i++] = new CTextControl(this,
                            "Sigma Chess needs some memory to be reserved for "
                            "general use (for collections, games, etc.). The "
                            "rest is allocated to the transposition tables:",
                            r, false);

  r.bottom = r.top + controlHeight_Text;
  r.Offset(18, 50);
  r.right = r.left + memValuePos;
  r1 = r;
  r1.left = r.right + 5;
  r1.right = r1.left + 80;

  FormatBytes(Mem_FreeBytes(), s);
  C[i++] = new CTextControl(this, "Current amount of free memory: ", r, false);
  r.Offset(0, 18);
  C[i++] = new CTextControl(this, s, r1, false);
  r1.Offset(0, 18);

  FormatBytes(TransTab_GetSize(), s);
  C[i++] = new CTextControl(
      this, "Memory allocated to transposition tables: ", r, false);
  r.Offset(0, 18);
  C[i++] = new CTextControl(this, s, r1, false);
  r1.Offset(0, 18);

  r.bottom += controlHeight_Text;
  C[i++] = new CTextControl(
      this, "Reserve memory for general use (takes effect after restart): ", r,
      false);

  CMenu *cmenu = new CMenu("");
  for (INT i = 5; i <= 100; i += 5) {
    Format(s, "%s%d MB", (i < 10 ? " " : ""), i);
    cmenu->AddItem(s, i);
  }
  r1.left -= 5;
  r1.bottom = r1.top + controlHeight_PopupMenu;
  C[i++] = cpopup_ResMem =
      new CPopupMenu(this, "", cmenu, P->Memory.reserveMem, r1, false);
  C[i] = nil;
} /* CPrefsDialog::CreateMemory */

void CPrefsDialog::CreateTransTab(void) {
  CControl **C = CTab[prefs_TransTab];
  INT i = 0;
  INT dv = (RunningOSX() ? 22 : 18);
  CRect r = R, r1;
  CHAR s[100];
  BOOL transSheetEnabled = (!EngineMatch.gameWin);

  r.bottom = r.top + controlHeight_CheckBox;
  C[i++] = new CTextControl(this, "Enable transposition tables:", r, false,
                            controlFont_Views);
  r.Offset(18, dv);
  C[i++] = ccheck_UseTrans =
      new CCheckBox(this, "In the normal playing modes",
                    P->Trans.useTransTables, r, false, transSheetEnabled);
  r.Offset(0, dv);
  C[i++] = ccheck_UseTransMF =
      new CCheckBox(this, "In the Mate Finder", P->Trans.useTransTablesMF, r,
                    false, transSheetEnabled);
  r.Offset(-18, dv + 15);

  r.bottom = r.top + (RunningOSX() ? 55 : 45);
  C[i++] =
      new CTextControl(this,
                       "As Sigma Chess supports multiple engine �instances� "
                       "running concurrently, the transposition table memory "
                       "must be shared among all the engines:",
                       r, false, controlFont_Views);

  r.bottom = r.top + controlHeight_Text;
  r.Offset(18, (RunningOSX() ? 60 : 50));
  r.right = r.left + 210;
  r1 = r;
  r1.left = r.right + 5;
  r1.right = r1.left + 55;
  r.bottom += controlHeight_Text;
  r.Offset(0, -5);

  FormatBytes(TransTab_GetSize(), s);
  C[i++] = new CTextControl(
      this, "Total memory allocated to transposition tables: ", r, false,
      controlFont_Views);
  C[i++] = ctext_TotalTrans = new CTextControl(this, s, r1, false);
  if (!RunningOSX())
    cbutton_SetTotalTrans = nil;
  else {
    totalTransMem = P->Trans.totalTransMem;
    CRect rb = r1;
    rb.Offset(r1.Width() + 5, -3);
    rb.right;
    C[i++] = cbutton_SetTotalTrans =
        new CPushButton(this, "Set...", rb, false, transSheetEnabled);
  }
  r.Offset(0, 2 * dv);
  r1.Offset(0, 2 * dv);
  r1.right += 60;

  C[i++] =
      new CTextControl(this, "Max transposition table size per engine: ", r,
                       false, controlFont_Views);

  CMenu *cmenu = new CMenu("");
  cmenu->AddItem("80 K", 1);
  cmenu->AddItem("160 K", 2);
  cmenu->AddItem("320 K", 3);
  cmenu->AddItem("640 K", 4);
  cmenu->AddItem("1.25 MB", 5);
  cmenu->AddItem("2.5 MB", 6);
  cmenu->AddItem("5 MB", 7);
  cmenu->AddItem("10 MB", 8);
  cmenu->AddItem("20 MB", 9);
  cmenu->AddItem("40 MB", 10);
  cmenu->AddItem("80 MB", 11);
  cmenu->AddItem("160 MB", 12);
  cmenu->AddItem("320 MB", 13);

  ULONG bytes = 320L * 1024L * 1024L;
  for (INT t = 13;
       t >= 1 && (bytes > TransTab_GetSize() || (!ProVersion() && t >= 9));
       t--) {
    cmenu->EnableMenuItem(t, false);
    bytes >>= 1;
  }

  r1.left -= 5;
  r1.bottom = r1.top + controlHeight_PopupMenu;
  if (!RunningOSX()) {
    r1.right -= 20;
    r1.Offset(0, -7);
  }
  C[i++] = cpopup_TransMem = new CPopupMenu(
      this, "", cmenu, P->Trans.maxTransSize, r1, false, transSheetEnabled);
  C[i] = nil;
} /* CPrefsDialog::CreateTransTab */

static void FormatBytes(ULONG bytes, CHAR *s) {
  bytes /= 1024;
  ULONG M = bytes / 1024;
  ULONG K = bytes % 1024;

  if (M == 0)
    Format(s, "%ld K", K);
  else if (K < 100)
    Format(s, "%ld MB", M);
  else
    Format(s, "%ld.%ld MB", M, K / 100);
} /* FormatBytes */

/**************************************************************************************************/
/*                                                                                                */
/*                                       EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

static INT TotalTransMemDialog(INT totalMem0);

void CPrefsDialog::HandlePushButton(CPushButton *ctl) {
  if (ctl == cbutton_Default) {
    if (!ProVersion() && cpopup_TransMem->Get() > 8) {
      ProVersionDialog(this,
                       "The transposition table size is limited to 10 MB in "
                       "Sigma Chess Lite.");
      return;
    }

    INT futureMovesOld = P->Games.showFutureMoves;
    INT menuIconsOld = P->General.menuIcons;
    INT maxTransSizeOld = P->Trans.maxTransSize;

    cedit_Player->GetTitle(P->General.playerName);
    for (INT i = 0; i <= 2; i++)
      if (cradio_MenuIcon[i]->Selected()) P->General.menuIcons = i;
    P->General.enable3D = ccheck_Enable3D->Checked();

    P->Games.gotoFinalPos = cradio_FinalPos->Selected();
    P->Games.turnPlayer = ccheck_TurnPlayer->Checked();
    P->Games.showFutureMoves = ccheck_FutureMoves->Checked();
    P->Games.hiliteCurrMove = ccheck_HiliteCurr->Checked();
    P->Games.askGameSave = ccheck_AskSave->Checked();
    P->Games.moveSpeed = cslider_MoveSpeed->GetVal();
    P->Games.saveNative = ccheck_SaveNative->Checked();

    P->Collections.autoName = ccheck_AutoNameCol->Checked();
    P->Collections.keepColWidths = ccheck_KeepColumnWidths->Checked();

    P->PGN.skipMoveSep = ccheck_SkipMoveNumSep->Checked();
    P->PGN.openSingle = ccheck_OpenSingle->Checked();
    P->PGN.fileExtFilter = ccheck_FileExtFilter->Checked();
    P->PGN.keepNewLines = ccheck_KeepNewLines->Checked();

    P->Messages.announceMate = ccheck_AnnounceMate->Checked();
    P->Messages.announce1stMate = ccheck_Announce1stMate->Checked();
    P->Messages.gameOverDlg = ccheck_GameOver->Checked();
    P->Messages.canResign = ccheck_Resign->Checked();
    P->Messages.canOfferDraw = ccheck_DrawOffer->Checked();

    P->Sound.woodSound = ccheck_WoodSound->Checked();
    P->Sound.moveBeep = ccheck_MoveBeep->Checked();

    if (cradio_NumRel->Selected())
      P->AnalysisFormat.scoreNot = scoreNot_NumRel;
    else if (cradio_NumAbs->Selected())
      P->AnalysisFormat.scoreNot = scoreNot_NumAbs;
    else
      P->AnalysisFormat.scoreNot = scoreNot_Glyph;

    P->AnalysisFormat.showScore = ccheck_ShowScore->Checked();
    P->AnalysisFormat.showDepth = ccheck_ShowDepth->Checked();
    P->AnalysisFormat.showTime = ccheck_ShowTime->Checked();
    P->AnalysisFormat.showNodes = ccheck_ShowNodes->Checked();
    P->AnalysisFormat.showNSec = ccheck_ShowNSec->Checked();
    P->AnalysisFormat.showMainLine = ccheck_ShowMainLine->Checked();
    P->AnalysisFormat.shortFormat = cradio_ShortFormat->Selected();

    P->Misc.printPageHeaders = ccheck_PrintHeaders->Checked();
    P->Misc.HTMLGifReminder = ccheck_HTMLGifNotify->Checked();

    if (!RunningOSX()) P->Memory.reserveMem = cpopup_ResMem->Get();

    P->Trans.useTransTables = ccheck_UseTrans->Checked();
    P->Trans.useTransTablesMF = ccheck_UseTransMF->Checked();
    P->Trans.maxTransSize = cpopup_TransMem->Get();

    if (P->General.menuIcons != menuIconsOld) sigmaApp->UpdateMenuIcons();

    if (P->Games.showFutureMoves != futureMovesOld)
      sigmaApp->BroadcastMessage(msg_RefreshGameMoveList);

    if (P->Trans.maxTransSize != maxTransSizeOld ||
        RunningOSX() && P->Trans.totalTransMem != totalTransMem) {
      if (P->Trans.maxTransSize > maxTransSizeOld &&
          P->Trans.maxTransSize >= 10)
        NoteDialog(
            this, "Memory Warning",
            "WARNING: Make sure the transposition table size never exceeds 75 "
            "% of the physical amount of RAM in your computer. Otherwise the "
            "performance of Sigma Chess will be severely reduced...",
            cdialogIcon_Warning);

      if (Engine_AnyRunning(&Global)) {
        NoteDialog(this, "Transposition Tables",
                   "You have changed the size of the transposition tables. All "
                   "running engines will be stopped...");
        for (INT i = 0; i < maxEngines; i++)
          if (Global.Engine[i])
            ((GameWindow *)(Global.Engine[i]->refID))->CheckAbortEngine();
      }

      if (RunningOSX() && P->Trans.totalTransMem != totalTransMem) {
        P->Trans.totalTransMem = totalTransMem;
        TransTab_Init();  // Also calls TransTab_Dim()
      } else {
        TransTab_Dim();
      }
    }
  } else if (ctl == cbutton_Factory) {
    cedit_Player->SetText(P->General.playerName);
    cradio_MenuIcon[2]->Select();
    ccheck_Enable3D->Check(true);

    cradio_FinalPos->Select();
    ccheck_TurnPlayer->Check(false);
    ccheck_FutureMoves->Check(true);
    ccheck_HiliteCurr->Check(true);
    ccheck_AskSave->Check(true);
    cslider_MoveSpeed->SetVal(75);
    ccheck_SaveNative->Check(false);

    ccheck_AutoNameCol->Check(true);
    ccheck_KeepColumnWidths->Check(true);

    ccheck_SkipMoveNumSep->Check(false);
    ccheck_OpenSingle->Check(true);
    ccheck_FileExtFilter->Check(true);
    ccheck_KeepNewLines->Check(false);

    ccheck_AnnounceMate->Check(true);
    ccheck_GameOver->Check(false);
    ccheck_Resign->Check(true);
    ccheck_DrawOffer->Check(true);

    ccheck_WoodSound->Check(true);
    ccheck_MoveBeep->Check(false);

    cradio_NumRel->Select();
    ccheck_ShowScore->Check(true);
    ccheck_ShowDepth->Check(true);
    ccheck_ShowTime->Check(false);
    ccheck_ShowNodes->Check(false);
    ccheck_ShowNSec->Check(false);
    ccheck_ShowMainLine->Check(true);
    cradio_LongFormat->Select();

    ccheck_PrintHeaders->Check(true);
    ccheck_HTMLGifNotify->Check(true);

    if (!RunningOSX()) cpopup_ResMem->Set(2);

    ccheck_UseTrans->Check(true);
    ccheck_UseTransMF->Check(true);
    cpopup_TransMem->Set(7);
    return;
  } else if (ctl == cbutton_SetTotalTrans) {
    INT newTotalTransMem = TotalTransMemDialog(totalTransMem);

    if (newTotalTransMem > 0) {
      CHAR s[20];
      totalTransMem = newTotalTransMem;
      LONG totalTransBytes = totalTransMem * 1024L * 1024L;
      FormatBytes(totalTransBytes, s);
      ctext_TotalTrans->SetTitle(s);

      // Reduce/enable single engine trans tab size if necessary
      ULONG bytes = 320 * 1024 * 1024;
      INT newt = 0;
      for (INT t = 13; t >= 1; t--) {
        cpopup_TransMem->EnableItem(t, bytes <= totalTransBytes);
        if (t > 1 && bytes > totalTransBytes) newt = t - 1;
        bytes >>= 1;
      }
      if (newt > 0 && newt < cpopup_TransMem->Get()) cpopup_TransMem->Set(newt);
    }
    return;
  }

  // Validation succeeded (or user pressed "Cancel") -> Call inherited function:
  CDialog::HandlePushButton(ctl);
} /* CPrefsDialog::HandlePushButton */

void CPrefsDialog::HandlePopupMenu(CPopupMenu *ctl, INT itemNo) {
  if (ctl == cpopup) {
    for (INT i = 0; CTab[group][i]; i++) CTab[group][i]->Show(false);

    group = itemNo;

    for (INT i = 0; CTab[group][i]; i++) CTab[group][i]->Show(true);
  }
} /* CPrefsDialog::HandlePopupMenu */

void CPrefsDialog::HandleCheckBox(CCheckBox *ctl) {
  CDialog::HandleCheckBox(ctl);

  if (ctl == ccheck_WoodSound && ccheck_WoodSound->Checked())
    ccheck_MoveBeep->Check(false);
  else if (ctl == ccheck_MoveBeep && ccheck_MoveBeep->Checked())
    ccheck_WoodSound->Check(false);
  else if (ctl == ccheck_AnnounceMate && !ccheck_AnnounceMate->Checked())
    ccheck_Announce1stMate->Check(false);
  else if (ctl == ccheck_Announce1stMate && ccheck_Announce1stMate->Checked())
    ccheck_AnnounceMate->Check(true);
  else if (ctl == ccheck_SkipMoveNumSep && ccheck_SkipMoveNumSep->Checked())
    NoteDialog(this, "Warning",
               "Skipping the move separator violates the PGN standard and "
               "could cause problems with other chess programs",
               cdialogIcon_Warning);
  else if (ctl == ccheck_KeepNewLines && ccheck_KeepNewLines->Checked())
    NoteDialog(this, "Warning",
               "Preserving newlines violates the PGN standard and could cause "
               "problems with other chess programs",
               cdialogIcon_Warning);
} /* CPrefsDialog::HandleCheckBox */

/*------------------------------ Set Total Transposition Memory Size
 * -----------------------------*/

class CTotalTransMemDialog : public CDialog {
 public:
  CTotalTransMemDialog(CRect frame, INT totalMem0);

  virtual void HandlePushButton(CPushButton *ctl);

  CEditControl *cedit_TotalMem;
};

CTotalTransMemDialog::CTotalTransMemDialog(CRect frame, INT totalMem0)
    : CDialog(nil, "Transposition Table Memory", frame) {
  CRect inner = InnerRect();

  //--- Create the OK and Cancel buttons first ---
  cbutton_Cancel =
      new CPushButton(this, GetStr(sgr_Common, s_Cancel), CancelRect());
  cbutton_Default =
      new CPushButton(this, GetStr(sgr_Common, s_OK), DefaultRect());
  SetDefaultButton(cbutton_Default);

  CRect r = InnerRect();
  r.bottom = r.top + 2 * controlHeight_Text;
  r.right -= 60;
  new CTextControl(this,
                   "Total memory allocated to transposition tables (MB):", r);

  r = inner;
  r.bottom = r.top + controlHeight_Edit;
  r.left = r.right - 45;
  CHAR numStr[10];
  NumToStr(totalMem0, numStr);
  if (!RunningOSX()) r.Offset(0, -3);
  r.Offset(0, 5);
  cedit_TotalMem = new CEditControl(this, numStr, r, 4);

  CurrControl(cedit_TotalMem);
} /* CTotalTransMemDialog::CTotalTransMemDialog */

void CTotalTransMemDialog::HandlePushButton(CPushButton *ctl) {
  if (ctl == cbutton_Default) {
    if (!cedit_TotalMem->ValidateNumber(1, 320, true)) {
      CurrControl(cedit_TotalMem);
      NoteDialog(this, "Invalid Memory Size",
                 "The total transposition table memory size must be a whole "
                 "number between 1 and 320 MB.",
                 cdialogIcon_Error);
      return;
    }

    ULONG machineRAM = Mem_PhysicalRAM() / (1024 * 1024);
    LONG n;
    if (cedit_TotalMem->GetLong(&n) &&
        ((!RunningOSX() && n >= 100) ||
         (RunningOSX() && n > machineRAM - 64))) {
      NoteDialog(this, "Warning",
                 "Make sure you have at least this amount of physical memory "
                 "installed in your Mac...",
                 cdialogIcon_Warning);
    }
  }

  CDialog::HandlePushButton(ctl);
} /* CTotalTransMemDialog::HandlePushButton */

static INT TotalTransMemDialog(INT totalMem0) {
  CRect frame(0, 0, 220, 80);
  if (RunningOSX()) frame.right += 65, frame.bottom += 30;
  theApp->CentralizeRect(&frame);

  CTotalTransMemDialog *dialog = new CTotalTransMemDialog(frame, totalMem0);
  dialog->Run();

  LONG totalMem = -1;

  if (dialog->reply == cdialogReply_OK)
    dialog->cedit_TotalMem->GetLong(&totalMem);

  delete dialog;
  return totalMem;
} /* TotalTransMemDialog */
