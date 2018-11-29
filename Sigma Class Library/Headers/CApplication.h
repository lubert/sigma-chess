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

#pragma once

#include "CFile.h"
#include "CMenu.h"
#include "CUtility.h"
#include "CWindow.h"
#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define appleMenuID 129

#define cApp_MaxMenus (253 - appleMenuID)

#define evt_MouseDown (mDownMask)
#define evt_MouseUp (mUpMask)
#define evt_KeyDown (keyDownMask)
#define evt_KeyUp (keyUpMask)
#define evt_AutoKey (autoKeyMask)
#define evt_Update (updateMask)
//#define evt_Disk          (diskMask)  // Not supported by MacOS X
#define evt_Activate (activMask)
#define evt_HighLevel (highLevelEventMask)
#define evt_OS (osMask)
#define evt_All (everyEvent)

#define key_LeftArrow 0x7B
#define key_RightArrow 0x7C
#define key_UpArrow 0x7E
#define key_DownArrow 0x7D
#define key_PageUp 0x74
#define key_PageDown 0x79
#define key_Home 0x73
#define key_End 0x77
#define key_FwdDel 0x75
#define key_BackDel 0x33
#define key_Tab 0x30
#define key_Space 0x31
#define key_Escape 0x35
#define key_Return 0x24
#define key_Enter 0x4C

#define nullCommand 0
/*
        kNullCharCode				= 0,
        kHomeCharCode				= 1,
        kEnterCharCode				= 3,
        kEndCharCode				= 4,
        kHelpCharCode				= 5,
        kBellCharCode				= 7,
        kBackspaceCharCode			= 8,
        kTabCharCode				= 9,
        kLineFeedCharCode			= 10,
        kVerticalTabCharCode		= 11,
        kPageUpCharCode				= 11,
        kFormFeedCharCode			= 12,
        kPageDownCharCode			= 12,
        kReturnCharCode				= 13,
        kFunctionKeyCharCode		= 16,
        kEscapeCharCode				= 27,
        kClearCharCode				= 27,
        kLeftArrowCharCode			= 28,
        kRightArrowCharCode			= 29,
        kUpArrowCharCode			= 30,
        kDownArrowCharCode			= 31,
        kDeleteCharCode				= 127,
        kNonBreakingSpaceCharCode	= 202
*/

#define modifier_Option optionKey
#define modifier_CapsLoc alphaLock
#define modifier_Shift shiftKey
#define modifier_Command cmdKey
#define modifier_Control controlKey
#define modifier_AutoKey (1 << 6)

enum APP_ERRORS {
  appErr_NoError = 0,
  appErr_ClipboardReadError,
  appErr_ClipboardWriteError,
  appErr_MemFullError
};

enum CURSORS {
  cursor_Arrow = 1000,
  cursor_Watch = 3000,
  cursor_IBeam = iBeamCursor,
  cursor_Cross = crossCursor,
  cursor_Plus = plusCursor,

  cursor_HResize = 1003,
  cursor_VResize = 1004
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

class CApplication {
 public:
  CApplication(CHAR *appName, OSTYPE theCreator);
  virtual ~CApplication(void);

  virtual void MainLooper(void);

  void Run(void);
  void Quit(void);
  void Abort(void);

  void InitAppleEvents(void);

  void ProcessEvents(INT evtMask = evt_All);
  void ProcessSysEvents(void);
  void ProcessEvent(INT evtMask = evt_All);
  void PostMessage(CWindow *win, INT message);

  void AddMenu(CMenu *menu);
  void RemoveMenu(CMenu *menu);
  void RedrawMenuBar(void);
  void EnableMenuBar(BOOL enabled, BOOL redraw = true);
  INT GetMenuBarHeight(void);

  void GetMouseLoc(CPoint *p);
  // BOOL MouseButtonDown (void);
  void WaitMouseReleased(void);

  void ShowHideCursor(BOOL visible);
  void SetCursor(INT cursorID = cursor_Arrow);
  void SpinCursor(void);

  void ResetClipboard(void);
  INT ReadClipboard(OSTYPE type, PTR *data, LONG *size);
  INT WriteClipboard(OSTYPE type, PTR data, LONG size);

  BOOL ColorPicker(CHAR *prompt, RGB_COLOR *color);

  CWindow *GetFrontWindow(void);
  void SetFrontWindow(CWindow *);
  void ActivateFrontWindow(BOOL activated);
  void CycleWindows(BOOL forward = true);

  BOOL ModalLoopRunning(void);
  void ModalLoopBegin(void);
  void ModalLoopEnd(void);

  BOOL PageSetupDialog(void);
  void PlaySound(INT soundID, BOOL async);
  void ShowMenuBar(BOOL visible);
  void ShowControlStrip(BOOL show);
  void ClickMenuBar(void);

  CRect ScreenRect(void);
  void CentralizeRect(CRect *r, BOOL toScreen = false);
  void StackRect(CRect *r, INT hor = 20, INT ver = 20);
  CRect NewDocRect(INT width, INT height);
  void EnableSocketEvents(BOOL enabled);

  void EnableQuitCmd(BOOL enabled);
  void EnablePrefsCmd(BOOL enabled);
  void EnableAboutCmd(BOOL enabled);

  INT LaunchApp(CHAR *appName, BOOL background, BOOL hide);
  INT LaunchConsoleApp(
      CHAR *appName);  // Returns error code if failure (0 if success).
  INT QuitApp(OSTYPE appCreator);
  INT HideApp(OSTYPE appCreator);

  LONG OSVersion(void);

  virtual void HandleAEInstall(void);
  virtual void HandleLaunch(void);
  virtual BOOL HandleQuitRequest(void);
  virtual void HandleActivate(BOOL activated);
  virtual void HandleAbout(void);
  virtual void HandleShowPrefs(void);
  virtual BOOL HandleMessage(LONG msg, LONG submsg = 0, PTR data = nil);
  virtual void HandleOpenFile(CFile *file);
  virtual void HandleMenuAdjust(void);
  virtual void HandleCursorAdjust(void);
  virtual void HandleWindowCreated(CWindow *theWin);
  virtual void HandleWindowDestroyed(CWindow *theWin);

  void DoEvent(EventRecord *event);
  void HandleOpenFSSpec(FSSpec *theSpec);

  OSTYPE creator;

  BOOL launching;
  BOOL running;
  BOOL quitting;
  BOOL suspended;

  CList winList;

  CMenu *MenuTab[128];  // Entry 0 (apple menu) = menuID 129, entry 1 = menuID
                        // 130, ...
  void RegisterMenu(CMenu *menu);
  void UnregisterMenu(CMenu *menu);
  CMenu *LookupMenu(INT menuID);

  BOOL responsive;
  BOOL checkSocketEvents;
  BOOL checkAppleEvents;

 private:
  void InitToolBox(void);
  BOOL CheckSysConfig(void);
  BOOL SharedLibAvailable(Str255 libName);

  void DoMenuCommand(LONG menuCommand);
  void DoMouseDown(EventRecord *event);
  void DoContentClick(WindowPtr win, EventRecord *event);
  void DoKeyPress(EventRecord *event, BOOL autoKey);
  void DoUpdate(EventRecord *event);
  void DoActivate(EventRecord *event);
  void DoOSEvent(EventRecord *event);
  void Deactivate(void);
  void ActivateWind(WindowPtr win, BOOL activate);
  void CheckCursorAdjust(void);
  BOOL IsMovableModal(WindowPtr win);
  CWindow *LookupCWindow(WindowPtr win);

  INT currCursorID;
  CHAR appName[50];

  SndChannelPtr sndChan;

  INT modalCount;  // Number of modal dialogs currently running
};

/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES */
/*                                                                                                */
/**************************************************************************************************/

extern CApplication *theApp;

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

BOOL RunningOSX(void);
BOOL UsingMetalTheme(void);
