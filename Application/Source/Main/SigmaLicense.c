/**************************************************************************************************/
/*                                                                                                */
/* Module  : SIGMALICENSE.C */
/* Purpose : This module implements all the license/registration/upgrade
 * handling.                */
/*           resources. */
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

#include "SigmaLicense.h"

#include "SigmaApplication.h"
#include "SigmaPrefs.h"
#include "SigmaStrings.h"

#include "CDialog.h"
#include "CEditor.h"
#include "CMemory.h"

// There is NO difference between the "Pro" and "Lite" applications as such. The
// features in the "Pro" version are enabled if/when the user enters his correct
// owner name, serial number and license key (the license key is basically a
// scrambled version of the former) via the "Upgrade" dialog.

// When the user registers, he is assigned a serial number (in the form 6xxxxx),
// and a license key (scrambled version of owner name and serial number).

// Owner Name  : Ole
// Serial No   : 600000
// License Key : 1919-5278-0761

#define str(i) GetStr(sgr_License, i)

enum DLG_STRINGS {
  s_LaunchInvalTitle = 0,
  s_LaunchInval,

  s_LicenseTitle,
  s_LicenseHeader,
  s_Quit,
  s_Accept,

  s_RegisterTitle,

  s_UpgradeTitle,
  s_UpgradeMsg,
  s_Upgrade,
  s_YourName,
  s_SerialNo,
  s_LicenseKey,

  s_SigmaUpg,
  s_SigmaUpgraded,
  s_IncorLic,
  s_IncorLicense
};

static void BuildLicenseKey(CHAR *ownerName, CHAR *serialNo, CHAR *licenseKey);
static void ScrambleStr(CHAR *s, CHAR *t, INT len);

/**************************************************************************************************/
/*                                                                                                */
/*                                      LICENSE DATA HANDLING */
/*                                                                                                */
/**************************************************************************************************/

void ResetLicense(LICENSE *L) {
  L->mainVersion = sigmaVersion_Main;
  L->wasJustUpgraded = false;
  L->pro = false;
  L->ownerName[0] = 0;
  L->serialNo[0] = 0;
  L->licenseKey[0] = 0;
} /* ResetLicense */

// The first time the program is started we must first display the "License
// Agreement" dialog, and "force" the user to accept it. If not the program is
// terminated.

BOOL ProVersion(void) {
  return true;
  // return Prefs.license.pro;
} /* ProVersion */

// At startup we verify the license information if it's the PRO version:

void VerifyLicense(void) {
  /*
     if (! ProVersion()) return;

     LICENSE *L = &(Prefs.license);
     CHAR trueKey[licenseKeyLen + 1];

     BuildLicenseKey(L->ownerName, L->serialNo, trueKey);
     if (! EqualStr(L->licenseKey,trueKey) || L->serialNo[1] != '0')
     {
        ResetLicense(L);
        sigmaPrefs->Save();
        NoteDialog(nil, str(s_LaunchInvalTitle), str(s_LaunchInval),
     cdialogIcon_Error); sigmaApp->Abort();
     }
  */
} /* VerifyLicense */

static void BuildLicenseKey(CHAR *ownerName, CHAR *serialNo, CHAR *licenseKey) {
  INT len = StrLen(ownerName);
  CHAR tmp[licenseKeyLen];

  // First prefill licenseKey with serial no & ownerName (loop and xor):
  for (INT i = 0; i < licenseKeyLen; i++)
    tmp[i] = serialNo[i % licenseSerialNoLen];
  for (INT i = 0; i < len; i++) tmp[i % licenseKeyLen] += ownerName[i];

  // Next scramble:
  INT seed = 314;
  for (INT i = 0; i < licenseKeyLen; i++)
    tmp[i] += seed, seed = (1017 * seed + 419) % 256;

  // Finally turn into digits (and add separators):
  for (INT i = 0; i < licenseKeyLen; i++)
    licenseKey[i] = ((UINT)tmp[i] % 10) + '0';
  licenseKey[4] = licenseKey[9] = '-';
  licenseKey[licenseKeyLen] = 0;
} /* BuildLicenseKey */

static void ScrambleStr(CHAR *s, CHAR *t, INT len) {
  for (INT i = 0; i < len; i++) t[i] = ((((INT)s[i]) * 317) % 43) + '0';
} /* Scramble */

/**************************************************************************************************/
/*                                                                                                */
/*                                     LICENSE AGREEMENT DIALOG */
/*                                                                                                */
/**************************************************************************************************/

#define licensePages 3

static CHAR *LicenseHeader[licensePages + 1] = {
    "", "General Information", "License Agreement", "Disclaimer"};

static CHAR *LicenseBody[licensePages + 1] = {
    "",

    "Sigma Chess 6.2 is distributed via the Sigma Chess web-site:\r\\
       \r\\
       http://www.sigmachess.com\r\\
       \r\\
The author Ole K. Christensen can be contacted via e-mail at:\r\\
       \r\\
       ole@sigmachess.com\r\\
       \r\\
IMPORTANT: As of this version 6.2.0, Sigma Chess is only available as a single freeware version\\
 with ALL features available. Sigma Chess is thus no longer available as separate Lite and Pro versions.",

    "Although Sigma Chess 6.2 is freeware, it is copyrighted and NOT in the public domain. It may not be\\
 modified in any way and may thus only be distributed in its original form. It may not be sold,\\
 or included on a CD-ROM or any other physical media without explicit permission by the author.\r\\
 \r\\
Unless explicitly otherwise stated the above license also applies to all subsequent versions of\\
 Sigma Chess 6.x.",

    "The Sigma Chess 6.2 software is provided as is without any warranties of any kind either express or\\
 implied. By downloading, installing and/or using the Sigma Chess 6.2 software, the user/customer accepts\\
 all responsibility and agrees that the author of Sigma Chess cannot be held responsible or liable for any\\
 damage, data loss or harm of any kind whatsoever caused directly or indirectly by the installation\\
 and/or usage of the Sigma Chess 6.2 software."};

class CLicenseDialog : public CDialog {
 public:
  CLicenseDialog(CRect frame);
  // virtual void HandleActivate (BOOL activated);
  virtual void HandlePushButton(CPushButton *ctl);

 private:
  void SelectPage(void);

  INT pageNo;
  CPushButton *cbutton_Next, *cbutton_Prev;
  CTextControl *ctext_PageNo, *ctext_Header, *ctext_Body;
};

void SigmaLicenseDialog(void) {
  CRect frame(0, 0, 500, 350);
  theApp->CentralizeRect(&frame, true);

  CLicenseDialog *dialog = new CLicenseDialog(frame);
  dialog->Run();
  BOOL agreed = (dialog->reply == cdialogReply_OK);
  delete dialog;

  if (Prefs.firstLaunch) {
    if (!agreed) ResetLicense(&Prefs.license);
    Prefs.firstLaunch = !agreed;
    sigmaPrefs->Save();
  }

  if (!agreed) sigmaApp->Abort();
} /* SigmaLicenseDialog */

CLicenseDialog::CLicenseDialog(CRect frame)
    : CDialog(nil, str(s_LicenseTitle), frame) {
  pageNo = 1;

  CRect ri(0, 0, 32, 32);
  ri.Offset(InnerRect().left, InnerRect().top);
  new CIconControl(this, 2002, ri);

  CRect rt = ri;
  rt.left = ri.right + 10;
  rt.right = InnerRect().right;
  rt.bottom = rt.top + 55;
  CTextControl *ctext = new CTextControl(this, str(s_LicenseHeader), rt, true,
                                         controlFont_SmallSystem);
  ctext->SetFontStyle(fontStyle_Bold);

  CRect rd = InnerRect();
  rd.top = rt.bottom + 5;
  rd.bottom = rd.top + 2;
  new CDivider(this, rd);

  rt = InnerRect();
  rt.top = rd.bottom + (RunningOSX() ? 5 : 10);
  rt.bottom = rt.top + controlHeight_Text;
  ctext_Header = new CTextControl(this, "", rt);
  ctext_Header->SetFontStyle(fontStyle_Bold);

  rt.Offset(0, controlVDiff_Text);
  rt.bottom = CancelRect().top - 35;
  rt.left += 42;
  ctext_Body = new CTextControl(this, "", rt, true, controlFont_SmallSystem);

  rd = InnerRect();
  rd.top = rt.bottom + 5;
  rd.bottom = rd.top + 2;
  new CDivider(this, rd);

  rt.Offset(-42, controlVDiff_Text + 10);
  rt.top = rt.bottom - controlHeight_Text;
  ctext_PageNo = new CTextControl(this, "", rt, true, controlFont_SmallSystem);

  CRect rPrev = CancelRect();
  rPrev.left = InnerRect().left;
  rPrev.right = rPrev.left + 60;
  CRect rNext = rPrev;
  rNext.Offset(70, 0);
  cbutton_Prev = new CPushButton(this, "<<", rPrev, true, false);
  cbutton_Next = new CPushButton(this, ">>", rNext);
  cbutton_Cancel = new CPushButton(this, str(s_Quit), CancelRect());
  cbutton_Default =
      new CPushButton(this, str(s_Accept), DefaultRect(), true, false);
  SetDefaultButton(cbutton_Default);

  SelectPage();
} /* CLicenseDialog::CLicenseDialog */

void CLicenseDialog::HandlePushButton(CPushButton *ctl) {
  if (ctl == cbutton_Next) {
    pageNo++;
    SelectPage();
  } else if (ctl == cbutton_Prev) {
    pageNo--;
    SelectPage();
  } else {
    CDialog::HandlePushButton(ctl);
  }
} /* CLicenseDialog::HandlePushButton */

void CLicenseDialog::SelectPage(void) {
  cbutton_Prev->Enable(pageNo > 1);
  cbutton_Next->Enable(pageNo < licensePages);
  cbutton_Default->Enable(pageNo == licensePages);

  CHAR s[30];
  Format(s, "Page %d of %d", pageNo, licensePages);
  ctext_PageNo->SetTitle(s);
  ctext_Header->SetTitle(LicenseHeader[pageNo]);
  ctext_Body->SetTitle(LicenseBody[pageNo]);
} /* CLicenseDialog::SelectPage */

/**************************************************************************************************/
/*                                                                                                */
/*                                        REGISTRATION DIALOG */
/*                                                                                                */
/**************************************************************************************************/

#define registerPages 4

static CHAR *RegisterHeader[registerPages + 1] = {
    "", "Why Register?", "Sigma Chess 6.2 Lite Restrictions & Limitations",
    "How to Register", "How to Upgrade"};

static CHAR *RegisterBody[registerPages + 1] = {
    "",
    "If you register Sigma Chess 6.2 by paying the $20 registration fee, you get access to the full\\
 Sigma Chess 6.2 Pro feature set, i.e. all the restrictions and limitations in the Lite version\\
 are removed (see next page). Additionally, you support the future development of Sigma Chess,\\
 just like for other shareware products :-)",

    " ¥    The new position and opening line filters are only available for\r\\
       collections with at most 100 games.\r\\
 ¥    The new player rating history graph only shows the first 10 games.\r\\
 ¥    Collections can contain a maximum of 1000 games.\r\\
 ¥    Changes to opening/position libraries cannot be saved.\r\\
 ¥    Only the KQKR and KBNK endgame databases are included.\r\\
	       The commercial Pro version contains several other 4-piece endings.\r\\
 ¥    Automatic game annotation/analysis are disabled for collections.\r\\
 ¥    Diagrams are not printed when printing collections/online chess books.\r\\
 ¥    Diagrams are not included when exporting collections/online chess.\r\\
       books to HTML.\r\\
 ¥    Transposition tables are limited to 10 MB, whereas the commercial Pro\r\\
	       version can handle transposition tables up to a size of 320 MB.\r\\
 ¥    At most three windows can be opened simultaneously.\r\\
 ¥    Monitor mode is not available.\r\\
 ¥    UCI engines are limited to max 64 MB transposition/hash tables.\r\\
 ¥    UCI engines cannot reduce the playing strength to a specific ELO setting.\r\\
 ¥    UCI engines cannot access Nalimov tablebases.",

    "The price for upgrading to Sigma Chess 6.2 Pro is only $20. Registration and payment is\\
 processed online through the widely used Internet payment service Kagi (www.kagi.com)\\
 from the secure Kagi server at:\r\\
          \r\\
            https://order.kagi.com/?1CU",

    "When your payment has been processed you will receive a confirmation e-mail from Kagi.\\
 A few days later you will receive another e-mail from the author of Sigma Chess containing a\\
 unique Serial Number and a personal License Key. In order to upgrade to Sigma Chess 6.2 Pro, you\\
 then simply click the \"Upgrade\" button in the \"About\" dialog and enter this license\\
 information. From then on Sigma Chess 6.2 will run in \"Pro\" mode with the full feature list enabled!\r\\
 \r\\
All future updates to Sigma Chess 6 Pro are provided free of charge for owners of Sigma Chess 6.2 Pro.\\
 You simply need to download the new version 6.x.x and re-enter your license information."};

class CRegisterDialog : public CDialog {
 public:
  CRegisterDialog(CRect frame);
  virtual void HandlePushButton(CPushButton *ctl);

 private:
  void SelectPage(void);

  INT pageNo;
  CPushButton *cbutton_Next, *cbutton_Prev;
  CTextControl *ctext_PageNo, *ctext_Header, *ctext_Body;
};

void SigmaRegisterDialog(void) {
  CRect frame(0, 0, 500, 370);
  theApp->CentralizeRect(&frame, true);

  CRegisterDialog *dialog = new CRegisterDialog(frame);
  dialog->Run();
  delete dialog;

  //   URLOpenFlags urlFlags = 0;
  //   OSStatus err = ::URLSimpleDownload("http://www.sigmachess.com", NULL,
  //   NULL, urlFlags, NULL, NULL);
  /*
    const char *        url,
    FSSpec *            destination,             // can be NULL
    Handle              destinationHandle,       // can be NULL
    URLOpenFlags        openFlags,
    URLSystemEventUPP   eventProc,               // can be NULL
    void *              userContext);            // can be NULL
  */
} /* SigmaRegisterDialog */

CRegisterDialog::CRegisterDialog(CRect frame)
    : CDialog(nil, str(s_RegisterTitle), frame) {
  pageNo = 1;

  CRect rt = InnerRect();
  rt.bottom = rt.top + controlHeight_Text;
  ctext_Header = new CTextControl(this, "", rt);
  ctext_Header->SetFontStyle(fontStyle_Bold);

  rt.Offset(0, controlVDiff_Text);
  rt.bottom = CancelRect().top - 35;
  rt.left += 42;
  ctext_Body = new CTextControl(this, "", rt, true, controlFont_SmallSystem);

  CRect rd = InnerRect();
  rd.top = rt.bottom + 5;
  rd.bottom = rd.top + 2;
  new CDivider(this, rd);

  rt.Offset(-42, controlVDiff_Text + 10);
  rt.top = rt.bottom - controlHeight_Text;
  ctext_PageNo = new CTextControl(this, "", rt, true, controlFont_SmallSystem);

  CRect rPrev = CancelRect();
  rPrev.left = InnerRect().left;
  rPrev.right = rPrev.left + 60;
  CRect rNext = rPrev;
  rNext.Offset(70, 0);
  cbutton_Prev = new CPushButton(this, "<<", rPrev, true, false);
  cbutton_Next = new CPushButton(this, ">>", rNext);
  cbutton_Default = new CPushButton(this, "OK", DefaultRect());
  SetDefaultButton(cbutton_Default);

  SelectPage();
} /* CRegisterDialog::CRegisterDialog */

void CRegisterDialog::HandlePushButton(CPushButton *ctl) {
  if (ctl == cbutton_Next) {
    pageNo++;
    SelectPage();
  } else if (ctl == cbutton_Prev) {
    pageNo--;
    SelectPage();
  } else {
    CDialog::HandlePushButton(ctl);
  }
} /* CRegisterDialog::HandlePushButton */

void CRegisterDialog::SelectPage(void) {
  cbutton_Prev->Enable(pageNo > 1);
  cbutton_Next->Enable(pageNo < registerPages);

  CHAR s[30];
  Format(s, "Page %d of %d", pageNo, registerPages);
  ctext_PageNo->SetTitle(s);
  ctext_Header->SetTitle(RegisterHeader[pageNo]);
  ctext_Body->SetTitle(RegisterBody[pageNo]);
} /* CRegisterDialog::SelectPage */

/**************************************************************************************************/
/*                                                                                                */
/*                                          UPGRADE DIALOG */
/*                                                                                                */
/**************************************************************************************************/

class CUpgradeDialog : public CDialog {
 public:
  CUpgradeDialog(CRect frame);
  virtual void HandlePushButton(CPushButton *ctl);
  BOOL Validate(CHAR *owner, CHAR *serial, CHAR *key);

  CEditControl *cedit_Owner, *cedit_SerialNo, *cedit_LicenseKey;
};

void SigmaUpgradeDialog(void) {
  CRect frame(0, 0, 350, 175);
  if (RunningOSX()) frame.right += 50, frame.bottom += 45;
  theApp->CentralizeRect(&frame, true);

  CUpgradeDialog *dialog = new CUpgradeDialog(frame);
  dialog->Run();
  if (dialog->reply == cdialogReply_OK) {
    Beep(3);
    NoteDialog(dialog, str(s_SigmaUpg), str(s_SigmaUpgraded));
  }

  delete dialog;
} /* SigmaUpgradeDialog */

CUpgradeDialog::CUpgradeDialog(CRect frame)
    : CDialog(nil, str(s_UpgradeTitle), frame) {
  CRect ri(0, 0, 32, 32);
  ri.Offset(InnerRect().left, InnerRect().top);
  new CIconControl(this, 2000, ri);

  CRect rt = ri;
  rt.top -= 3;
  rt.bottom += 10;
  rt.left = ri.right + 10;
  rt.right = InnerRect().right;
  ;
  CTextControl *ctext = new CTextControl(this, str(s_UpgradeMsg), rt, true,
                                         controlFont_SmallSystem);
  ctext->SetFontStyle(fontStyle_Bold);

  CRect rd = InnerRect();
  rd.top = rt.bottom + 2;
  rd.bottom = rd.top + 2;
  new CDivider(this, rd);

  CRect r = InnerRect();
  r.right = r.left + (RunningOSX() ? 80 : 65);
  r.top = rd.bottom + 15;
  r.bottom = r.top + controlHeight_Text;
  new CTextControl(this, str(s_YourName), r);
  r.Offset(0, controlVDiff_Edit);
  new CTextControl(this, str(s_SerialNo), r);
  r.Offset(0, controlVDiff_Edit);
  new CTextControl(this, str(s_LicenseKey), r);

  INT cw = (RunningOSX() ? 8 : 7);
  r = InnerRect();
  r.left += (RunningOSX() ? 90 : 70);
  r.top = rd.bottom + (RunningOSX() ? 15 : 12);
  r.bottom = r.top + controlHeight_Edit;
  cedit_Owner = new CEditControl(this, "", r, licenseOwnerNameLen);
  r.Offset(0, controlVDiff_Edit);
  r.right = r.left + cw * licenseSerialNoLen + 10;
  cedit_SerialNo = new CEditControl(this, "", r, licenseSerialNoLen);
  r.Offset(0, controlVDiff_Edit);
  r.right = r.left + cw * licenseKeyLen + 10;
  cedit_LicenseKey = new CEditControl(this, "", r, licenseKeyLen);

  CRect rUpgrade = DefaultRect();
  if (RunningOSX()) rUpgrade.left -= 10;
  CRect rCancel = rUpgrade;
  rCancel.Offset(-rCancel.Width() - 10, 0);
  cbutton_Cancel = new CPushButton(this, GetCommonStr(s_Cancel), rCancel);
  cbutton_Default = new CPushButton(this, str(s_Upgrade), rUpgrade);
  SetDefaultButton(cbutton_Default);

  CurrControl(cedit_Owner);
} /* CUpgradeDialog::CUpgradeDialog */

void CUpgradeDialog::HandlePushButton(CPushButton *ctl) {
  if (ctl == cbutton_Default) {
    CHAR owner[licenseOwnerNameLen + 1], serial[licenseSerialNoLen + 1],
        key[licenseKeyLen + 1];
    cedit_Owner->GetTitle(owner);
    cedit_SerialNo->GetTitle(serial);
    cedit_LicenseKey->GetTitle(key);

    if (!Validate(owner, serial, key)) {
      NoteDialog(this, str(s_IncorLic), str(s_IncorLicense), cdialogIcon_Error);
      return;
    } else  // If successfully upgraded: Store license info in prefs file.
    {
      LICENSE *L = &Prefs.license;
      L->wasJustUpgraded = true;
      CopyStr(owner, L->ownerName);
      CopyStr(serial, L->serialNo);
      CopyStr(key, L->licenseKey);
      CopyStr(owner, Prefs.General.playerName);
      sigmaPrefs->Save();
    }
  }

  CDialog::HandlePushButton(ctl);
} /* CUpgradeDialog::HandlePushButton */

BOOL CUpgradeDialog::Validate(CHAR *owner, CHAR *serial, CHAR *key) {
  if (StrLen(owner) == 0) return false;  // Owner is mandatory

  // First validate serial no syntax:
  if ((serial[0] != '5' && serial[0] != '6') ||
      StrLen(serial) != licenseSerialNoLen)
    return false;
  for (INT i = 0; serial[i]; i++)
    if (!IsDigit(serial[i])) return false;

  // Next build the true/correct license key:
  CHAR trueKey[licenseKeyLen + 1];
  BuildLicenseKey(owner, serial, trueKey);

  // If master "password" generate license info and store on clipboard:
  if (EqualStr(key, "karToffel")) {
    CHAR licenseInfo[200];
    Format(licenseInfo,
           "Owner Name  : %s\rSerial No   : %s\rLicense Key : %s\r", owner,
           serial, trueKey);
    sigmaApp->ResetClipboard();
    sigmaApp->WriteClipboard('TEXT', (PTR)licenseInfo, StrLen(licenseInfo));
  }

  // Validate license key syntax:
  if (StrLen(key) != licenseKeyLen || key[4] != '-' || key[9] != '-')
    return false;
  for (INT i = 0; serial[i]; i++)
    if (i != 4 && i != 9 && !IsDigit(serial[i])) return false;

  // Finally verify the license key against the owner name and serial no:
  return EqualStr(key, trueKey);
} /* CUpgradeDialog::Validate */

/**************************************************************************************************/
/*                                                                                                */
/*                                LITE VERSION FEATURE RESTRICTION */
/*                                                                                                */
/**************************************************************************************************/

class CProVerDialog : public CDialog {
 public:
  CProVerDialog(CWindow *parent, CRect frame, CHAR *text);
};

// When the user of the LITE version tries to access one of the features which
// are only available in the commercial PRO version, a generic dialog is opened
// informing the user that that particular feature is only available in the
// commercial PRO version.

// Feature's restricted/disabled in the LITE version:
//  1. Collections are limited to max 1000 games.
//  2. Diagrams are not included when printing collections.
//  3. Automatic game annotation/analysis are disabled for collections.
//  4. Multi position EPD analysis is disabled.
//  5. The library editor is not available.
//  6. Transposition tables are limited to 10 MB
//  7. Only the KQKR and KBNK endgame databases are available.
//  8. Max 3 windows open
//  9. UCI engines are limited to max 64 MB transposition/hash tables.
// 10. UCI engines cannot reduce the playing strength to a specific ELO setting.
// 11. UCI engines cannot access Nalimov tablebases."

BOOL ProVersionDialog(CWindow *parent, CHAR *prompt) {
  if (ProVersion()) return true;

  INT lines = 1;
  if (prompt) {
    INT n = StrLen(prompt);
    while (n > 50) lines++, n -= 50;
  }

  CRect frame(0, 0, 300, 90 + 20 * lines);
  if (RunningOSX()) frame.right += 75, frame.bottom += lines * 4 + 15;
  theApp->CentralizeRect(&frame);

  CHAR text[1000];
  Format(text,
         "%s\r\rSee the About Dialog on how to register and upgrade to Sigma "
         "Chess 6.2 Pro...",
         (prompt ? prompt
                 : "This feature is not available in Sigma Chess 6.2 Lite"));
  CProVerDialog *dialog = new CProVerDialog(parent, frame, text);
  dialog->Run();
  delete dialog;

  return false;
} /* ProVersionDialog */

/*-------------------------------------- Pro Version Dialog
 * ---------------------------------------*/

CProVerDialog::CProVerDialog(CWindow *parent, CRect frame, CHAR *text)
    : CDialog(parent, "Sigma Chess 6.2 Lite Restriction", frame,
              (parent ? cdialogType_Sheet : cdialogType_Modal)) {
  CRect inner = InnerRect();

  CRect rIcon(0, 0, 32, 32);
  rIcon.Offset(inner.left, inner.top);
  new CIconControl(this, 1000, rIcon);

  CRect r = inner;
  r.left = rIcon.right + 10;
  r.bottom = inner.bottom - 30;
  new CTextControl(this, text, r, true, controlFont_SmallSystem);

  cbutton_Default = new CPushButton(this, "OK", DefaultRect());
  cbutton_Default->acceptsFocus = false;
  focusCtl = nil;
  SetDefaultButton(cbutton_Default);
} /* CProVerDialog::CProVerDialog */
