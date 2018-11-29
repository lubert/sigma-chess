/**************************************************************************************************/
/*                                                                                                */
/* Module  : CAPPLICATION.C */
/* Purpose : Implements the main application object that runs the main event
 * loop and keeps track */
/*           of all windows. */
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

#include "CApplication.h"
#include "CControl.h"
#include "CMemory.h"
#include "CPrint.h"
#include "CSocket.h"
#include "CUtility.h"
#include "TaskScheduler.h"

CApplication *theApp = NULL;

static BOOL mbarVisChanged = false;

/**************************************************************************************************/
/*                                                                                                */
/*                                          CONSTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CApplication::CApplication(CHAR *theAppName, OSTYPE theCreator) {
  theApp = this;

  launching = true;
  running = false;
  quitting = false;
  suspended = false;

  modalCount = 0;
  checkAppleEvents = true;

  CopyStr(theAppName, appName);
  creator = theCreator;
  responsive = true;
  checkSocketEvents = false;

  for (int i = 0; i < 128; i++) MenuTab[i] = nil;

  InitToolBox();
  CSocket_Init();
  CControl_Init();
} /* CApplication::CApplication */

/*----------------------------------------- InitToolBox
 * ------------------------------------------*/

pascal void userRoutine(SndChannelPtr chan, SndCommand *cmd);

void CApplication::InitToolBox(void) {
  ::RegisterAppearanceClient();

  if (!RunningOSX()) {
    // Initialize memory:
    ::MoreMasterPointers(32);

    // Flush event queue:
    ::FlushEvents(everyEvent, 0);
  }

  // Initialize menu bar (insert Apple menu):
  MenuHandle appleMenu;
  Str255 appleTitle, aboutPStr;
  CHAR aboutStr[100];

  appleTitle[0] = 1;
  appleTitle[1] = appleMark;
  appleMenu = ::NewMenu(appleMenuID, appleTitle);

  Format(aboutStr, "About %s", appName);
  C2P_Str(aboutStr, aboutPStr);
  ::AppendMenu(appleMenu, aboutPStr);
  if (RunningOSX()) ::AppendMenu(appleMenu, "\p(----------");
  ::InsertMenu(appleMenu, 0);
  if (RunningOSX()) ::EnableMenuCommand(appleMenu, kHICommandPreferences);

  ::InvalMenuBar();

  ::SetAntiAliasedTextEnabled(true, 6);

  // Set cursor to normal arrow cursor:
  ::InitCursor();
  SetCursor(1000);

  // Initialize print manager:
  Print_Init();

  // Create main sound channel:
  sndChan = nil;  // <-- VERY, VERY, VERY Important!!!
  OSErr err =
      SndNewChannel(&sndChan, sampledSynth, -1, NewSndCallBackUPP(userRoutine));
  if (err != noErr) sndChan = nil;

  ::SetQDGlobalsRandomSeed(::TickCount());
} /* CApplication::InitToolBox */

pascal void userRoutine(SndChannelPtr chan,
                        SndCommand *cmd)  // Dummy sound callback
{}

/*---------------------------------- Check System Configuration
 * ----------------------------------*/
// Verify that we can run on the current configuration
// We need:
//    Apple events
//    FSSpec-based Standard File
//    FSSpec-based file traps

BOOL CApplication::CheckSysConfig(void) {
  LONG result;
  OSErr err;
  Boolean hasAppleEvents, hasFSpTraps, has8BitColor;

  // Check mandatory system features:

  err = Gestalt(gestaltAppleEventsAttr, &result);
  hasAppleEvents =
      ((err == noErr) && (result & (1 << gestaltAppleEventsPresent)));

  err = Gestalt(gestaltFSAttr, &result);
  hasFSpTraps = ((err == noErr) && (result & (1 << gestaltHasFSSpecCalls)));

  err = Gestalt(gestaltQuickdrawVersion, &result);
  has8BitColor = ((err == noErr) && (result >= gestalt8BitQD));

  return (OSVersion() >= 0x0860 && hasAppleEvents && hasFSpTraps &&
          has8BitColor && NavServicesAvailable());
} /* CApplication::CheckSysConfig */

LONG CApplication::OSVersion(void) {
  LONG result;
  OSErr err;

  err = Gestalt(gestaltSystemVersion, &result);
  return (err == noErr ? result : 0x0600);
} /* CApplication::OSVersion */

static short runningOSX = -1;

BOOL RunningOSX(void) {
  if (runningOSX < 0) {
    LONG result;
    OSErr err = Gestalt(gestaltMenuMgrAttr, &result);
    runningOSX = (err == noErr && (result & gestaltMenuMgrAquaLayoutMask));
  }
  return runningOSX;
} /* RunningOSX */

BOOL UsingMetalTheme(void) { return false; }  // UsingMetalTheme

/*---------------------------------- Check For Shared Libraries
 * ----------------------------------*/

BOOL CApplication::SharedLibAvailable(Str255 libName) {
  CFragConnectionID connID;
  Ptr mainAddr;
  Str255 errName;

  return (::GetSharedLibrary(libName, kAnyCFragArch, kFindCFrag, &connID,
                             &mainAddr, errName) == noErr);
} /* CApplication::SharedLibAvailable */

/**************************************************************************************************/
/*                                                                                                */
/*                                          DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CApplication::~CApplication(void) {
  // Terminate the current printing session:
  Print_End();
  CSocket_End();

  if (sndChan) SndDisposeChannel(sndChan, true);
} /* CApplication::~CApplication */

/**************************************************************************************************/
/*                                                                                                */
/*                                        MAIN EVENT LOOP */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Maint Event Loop
 * ----------------------------------------*/
// The "Run()" method should normally be called directly after the creation of
// the CApplication object.

static LONG MainFunc(void *) {
  theApp->InitAppleEvents();
  theApp->ProcessEvents();  // Process any outstanding initial events (e.g AE
                            // ODoc)
  theApp->launching = false;
  theApp->running = true;

  theApp->HandleLaunch();
  while (!theApp->quitting) theApp->MainLooper();

  theApp->running = false;

  return 0;
} /* MainFunc */

void CApplication::Run(void) {
  Task_RunScheduler(MainFunc, (PTR)this, 10);
} /* CApplication::Run */

void CApplication::MainLooper(void) {
  ProcessEvents();
  Task_Switch();
} /* CApplication::MainLooper */

/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------- General Event Processing
 * -----------------------------------*/

static LONG nextEventTick = 0;

void CApplication::ProcessEvents(
    INT evtMask)  // Processes all events designated by "evtMask".
{
  CWindow *frontWin = GetFrontWindow();
  EventRecord event;

  if (!checkAppleEvents) evtMask &= ~highLevelEventMask;

  // if (frontWin) frontWin->HandleNullEvent();
  winList.Scan();
  while (CWindow *win = (CWindow *)(winList.Next())) win->HandleNullEvent();

  if (Task_GetCount() == 1 && responsive) {
    ProcessEvent(evtMask);
  } else if (::EventAvail(evtMask, &event) || Timer() > nextEventTick) {
    do
      ProcessEvent(evtMask);
    while (::EventAvail(evtMask, &event));

    nextEventTick = Timer() + (suspended ? 30 : 60);
  }

  if (mbarVisChanged) {
    ::InvalMenuBar();
    mbarVisChanged = false;
  }

  if (checkSocketEvents) CSocket_ProcessEvent();
} /* CApplication::ProcessEvents */

void CApplication::ProcessSysEvents(void) {
  ProcessEvents(everyEvent - mDownMask - keyDownMask - autoKeyMask);
} /* CApplication::ProcessSysEvents */

void CApplication::ProcessEvent(
    INT evtMask)  // Fetches and processes the next event (if any).
{
  EventRecord event;
  INT sleep = (Task_GetCount() == 1 && responsive ? 20 : 0);

  if (WaitNextEvent(evtMask, &event, sleep, nil)) DoEvent(&event);
} /* CApplication::ProcessEvent */

void CApplication::DoEvent(EventRecord *event) {
  switch (event->what) {
    case mouseDown:
      DoMouseDown(event);
      break;
    case keyDown:
      DoKeyPress(event, false);
      break;
    case autoKey:
      DoKeyPress(event, true);
      break;
    case updateEvt:
      DoUpdate(event);
      break;
    case activateEvt:
      DoActivate(event);
      break;
    case osEvt:
      DoOSEvent(event);
      break;
    case kHighLevelEvent:
      AEProcessAppleEvent(event);
      break;
  }
} /* CApplication::DoEvent */

/*--------------------------------------- Mouse Down Events
 * --------------------------------------*/

void CApplication::DoMouseDown(EventRecord *event) {
  WindowPtr win;
  CWindow *cwin;
  INT part;

  switch (part = FindWindow(event->where, &win)) {
    case inMenuBar:
      DoMenuCommand(MenuSelect(event->where));
      break;
    case inDrag:
      if (win != FrontWindow() &&
          (IsMovableModal(FrontWindow()) ||
           ModalLoopRunning()))  // ||
                                 // ((CWindow*)GetWRefCon(FrontWindow()))->sheetChild))
        Beep(1);
      else {
        BitMap screenBits;
        ::GetQDGlobalsScreenBits(&screenBits);
        DragWindow(win, event->where, &screenBits.bounds);
      }
      break;
    case inContent:
      if (win == FrontWindow())
        DoContentClick(win, event);
      else if (!GetFrontWindow()->IsModalDialog())
        SelectWindow(win);
      else
        Beep(1);
      break;
    case inGrow:
      cwin = LookupCWindow(win);
      if (cwin && cwin->sizeable) {
        Rect growRect;
        cwin->resizeLimit.SetMacRect(&growRect);
        LONG growSize = ::GrowWindow(win, event->where, &growRect);
        if (growSize != 0)
          cwin->HandleResize(LoWord(growSize), HiWord(growSize));
      }
      break;
    case inGoAway:
      cwin = LookupCWindow(win);
      if (cwin && ::TrackGoAway(win, event->where) &&
          cwin->HandleCloseRequest())
        delete cwin;
      else
        Beep(1);
      break;

    case inZoomIn:
    case inZoomOut:
      cwin = LookupCWindow(win);
      if (cwin && ::TrackBox(win, event->where, part)) cwin->HandleZoom();
      break;
  }
} /* CApplication::DoMouseDown */

static LONG lastClick = 0;
static Point lastPoint = {0, 0};

void CApplication::DoContentClick(WindowPtr win, EventRecord *event) {
  Point pt = event->where;
  ControlHandle ctrl;
  SInt16 part;

  ::SetPort(::GetWindowPort(win));
  ::GlobalToLocal(&pt);

  ctrl = ::FindControlUnderMouse(pt, win, &part);
  // part = ::FindControl(pt,win,&ctrl);

  if (part && ctrl) {
    ((CControl *)(::GetControlReference(ctrl)))->Track(pt, part);
  } else {
    CWindow *cwin = LookupCWindow(win);
    if (cwin) {
      CPoint cpt(pt.h, pt.v);
      BOOL doubleClick =
          (Timer() < lastClick + GetDblTime() && EqualPt(pt, lastPoint));

      if (RunningOSX() && cwin->sizeable && cpt.h >= cwin->bounds.right - 16 &&
          cpt.v >= cwin->bounds.bottom - 16) {
        Rect growRect;
        cwin->resizeLimit.SetMacRect(&growRect);
        LONG growSize = ::GrowWindow(win, event->where, &growRect);
        if (growSize != 0)
          cwin->HandleResize(LoWord(growSize), HiWord(growSize));
      } else {
        cwin->DispatchMouseDown(cpt, event->modifiers, doubleClick);
      }
    }
  }

  lastClick = Timer();
  lastPoint = pt;
} /* CApplication::DoContentClick */

BOOL CApplication::IsMovableModal(WindowPtr win) {
  return (GetWVariant(win) == movableDBoxProc ||
          GetWVariant(win) == kSheetWindowClass);
} /* CApplication::IsMovableModal */

/*---------------------------------------- Key Down Events
 * ---------------------------------------*/

void CApplication::DoKeyPress(EventRecord *event, BOOL autoKey) {
  CHAR c = (event->message & charCodeMask);
  INT key = (event->message & keyCodeMask) >> 8;
  BOOL cmd = (event->modifiers & cmdKey), alt = (event->modifiers & optionKey),
       shift = (event->modifiers & shiftKey);

  CWindow *frontWin = GetFrontWindow();

  if (!frontWin) {
    if (cmd) DoMenuCommand(MenuEvent(event));
  } else if (cmd && !frontWin->IsModalDialog())
    DoMenuCommand(MenuEvent(event));
  else if (alt && key == key_Tab && winList.Count() > 1 &&
           !frontWin->IsModalDialog() && winList.Find((PTR)frontWin))
    SetFrontWindow(
        (CWindow *)(shift ? winList.PrevCyclic() : winList.NextCyclic()));
  else {
    if (c == kEnterCharCode)
      key = key_Enter;
    else if (c == kReturnCharCode)
      key = key_Return;
    frontWin->HandleKeyDown(
        c, key, event->modifiers | (autoKey ? modifier_AutoKey : 0));
  }
} /* CApplication::DoKeyPress */

/*--------------------------------------- Update Events
 * ------------------------------------------*/

void CApplication::DoUpdate(EventRecord *event) {
  WindowPtr win = (WindowPtr)event->message;

  ::BeginUpdate(win);
  CWindow *cwin = LookupCWindow(win);
  if (cwin) {
    RgnHandle visRgn = ::NewRgn();
    ::GetPortVisibleRegion(::GetWindowPort(win), visRgn);
    Rect mr;
    ::GetRegionBounds(visRgn, &mr);
    ::DisposeRgn(visRgn);

    CRect cr(mr.left, mr.top, mr.right, mr.bottom);
    cwin->DispatchUpdate(cr);
  }
  ::EndUpdate(win);
} /* CApplication::DoUpdate */

/*---------------------------------- Handle Activate Events
 * --------------------------------------*/

void CApplication::DoActivate(EventRecord *event) {
  ActivateWind((WindowPtr)event->message, event->modifiers & activeFlag);
} /* CApplication::DoActivate */

void CApplication::Deactivate(void) {
  if (::FrontWindow()) ActivateWind(::FrontWindow(), false);
} /* CApplication::Deactivate */

void CApplication::ActivateWind(WindowPtr win, BOOL activate) {
  if (activate) HandleCursorAdjust();

  CWindow *cwin = LookupCWindow(win);
  if (cwin) cwin->DispatchActivate(activate);
} /* CApplication::ActivateWind */

/*-------------------------------------- Handle OS Events
 * ----------------------------------------*/

void CApplication::DoOSEvent(EventRecord *event) {
  switch ((event->message >> 24) & 0x00FF) {
    case suspendResumeMessage:
      suspended = ((event->message & resumeFlag) == 0);
      if (::FrontWindow()) ActivateWind(::FrontWindow(), !suspended);
      HandleActivate(!suspended);
      break;
      //    case mouseMovedMessage:
      //       break;
  }
} /* CApplication::DoOSEvent */

/*------------------------------------------ Quitting
 * --------------------------------------------*/

void CApplication::Quit(void) {
  if (HandleQuitRequest()) {
    running = false;
    quitting = true;
  }
} /* CApplication::Quit */

BOOL CApplication::HandleQuitRequest(
    void)  // Returns true if OK to quit. Can be overridden by user.
{          // Default behaviour is to ask all windows if quitting OK.
  CWindow *WinTab[100];
  INT winCount = 0;

  // Copy into WinTab[] first (because HandleQuitRequest updates the winList):
  winList.Scan();
  while (winCount < 100 && (WinTab[winCount++] = (CWindow *)(winList.Next())))
    ;

  // Then scan WinTab[] and send a "HandleQuitRequest" message to each window
  // (which are still in the app's winList):
  for (INT i = 0; i < winCount - 1; i++)
    if (winList.Find(WinTab[i]))  // If window still in win list
    {
      if (!WinTab[i]->HandleQuitRequest())
        return false;
      else
        delete WinTab[i];
    }
  return true;
} /* CApplication::HandleQuitRequest */

void CApplication::Abort(
    void)  // Should only be called in case of serious errors...
{
  ::ExitToShell();
} /* CApplication::Abort */

/*------------------------------------------ Cursors
 * --------------------------------------------*/

void CApplication::CheckCursorAdjust(void)  // control = 3B, command = 37
{
#ifndef TARGET_API_MAC_CARBON
  INT newCursorID;  //### ONLY IF MOUSE IS OVER "RELEVANT" VIEW!!
  ULONG kmap[4];
  ::GetKeys(kmap);

  if (kmap[1] & 0x00008000)
    newCursorID = 1002;
  else if (kmap[1] & 0x00000008)
    newCursorID = 1001;
  else
    newCursorID = 1000;

  if (newCursorID != currCursorID) SetCursor(newCursorID);
#endif
} /* CApplication::CheckCursorAdjust */

static ULONG nextSpinTick = 0;
static INT spinCount = 0;

void CApplication::SetCursor(INT cursorID) {
  ::SetCursor(*::GetCursor(currCursorID = cursorID));
  if (cursorID == cursor_Watch) nextSpinTick = TickCount() + 4, spinCount = 0;
} /* CApplication::SetCursor */

void CApplication::SpinCursor(void) {
  if (currCursorID != cursor_Watch || TickCount() < nextSpinTick) return;
  nextSpinTick = TickCount() + 4;
  spinCount = (spinCount + 1) % 8;
  ::SetCursor(*::GetCursor(cursor_Watch + spinCount));
} /* CApplication::SetCursor */

void CApplication::HandleCursorAdjust(
    void)  // Can be overridden to show other cursors
{
  ::SetCursor(*::GetCursor(cursor_Arrow));
} /* CApplication::HandleCursorAdjust */

/**************************************************************************************************/
/*                                                                                                */
/*                                        MENU HANDLING */
/*                                                                                                */
/**************************************************************************************************/

BOOL CApplication::HandleMessage(LONG msg, LONG submsg, PTR data)
// Should be overridden to handle all application wide events (e.g. Quit,
// New/Open windows/docs
{
  return false;
} /* CApplication::HandleMessage */

void CApplication::HandleAbout(
    void)  // The default about method does nothing...
{}         /* CApplication::HandleAbout */

void CApplication::HandleShowPrefs(
    void)  // The default about method does nothing...
{}         /* CApplication::HandleShowPrefs */

void CApplication::HandleMenuAdjust(
    void)  // The default enables and redraws the menubar...
{
  theApp->EnableMenuBar(true);
} /* CApplication::HandleMenuAdjust */

/*------------------------------------- Menu Mangement
 * -------------------------------------------*/
// When a new CMenu object is created it is assigned a unique menuID between 130
// and 255 (the ids from 0 to 128 are reserved by Apple and id 129 are used for
// the mandatory Apple menu).

void CApplication::AddMenu(CMenu *menu) {
  if (menu->inMenuBar || menu->inMenuList) return;  // Internal error

  ::InsertMenu(menu->hmenu, 0);
  menu->inMenuBar = true;
  menu->inMenuList = true;
} /* CApplication::AddMenu */

void CApplication::RemoveMenu(CMenu *menu) {
  if (!menu->inMenuBar) return;  // Internal error

  ::DeleteMenu(menu->menuID);
  menu->inMenuBar = false;
  menu->inMenuList = false;
} /* CApplication::RemoveMenu */

void CApplication::RedrawMenuBar(void) {
  if (::IsMenuBarVisible()) ::DrawMenuBar();
} /* CApplication::RedrawMenuBar*/

void CApplication::RegisterMenu(
    CMenu *menu)  // Must be called from CMenu constructor
{
  for (int i = 0; i < cApp_MaxMenus; i++)
    if (!MenuTab[i]) {
      MenuTab[i] = menu;
      menu->menuID = i + appleMenuID + 1;
      return;
    }
  // Internal error if we get here...
} /* CApplication::RegisterMenu */

void CApplication::UnregisterMenu(
    CMenu *menu)  // Must be called from CMenu destructor
{
  MenuTab[menu->menuID - appleMenuID - 1] = nil;
} /* CApplication::RegisterMenu */

CMenu *CApplication::LookupMenu(INT menuID) {
  return MenuTab[menuID - appleMenuID - 1];
} /* CApplication::LookupMenu */

void CApplication::EnableMenuBar(BOOL enabled, BOOL redraw) {
  if (enabled)
    ::EnableMenuItem(GetMenuHandle(appleMenuID), 1);
  else
    ::DisableMenuItem(GetMenuHandle(appleMenuID), 1);

  for (int i = 0; i < cApp_MaxMenus; i++)
    if (MenuTab[i] && MenuTab[i]->inMenuBar)
      if (enabled)
        ::EnableMenuItem(MenuTab[i]->hmenu, 0);
      else
        ::DisableMenuItem(MenuTab[i]->hmenu, 0);

  if (redraw && ::IsMenuBarVisible()) RedrawMenuBar();
} /* CApplication::EnableMenuBar */

INT CApplication::GetMenuBarHeight(void) {
  return ::GetMBarHeight();
} /* CApplication::GetMenuBarHeight */

void CApplication::EnableQuitCmd(BOOL enabled) {
  if (!RunningOSX()) return;
  if (enabled)
    ::EnableMenuCommand(::GetMenuHandle(appleMenuID), kHICommandQuit);
  else
    ::DisableMenuCommand(::GetMenuHandle(appleMenuID), kHICommandQuit);
} /* CApplication::EnableQuitCmd */

void CApplication::EnablePrefsCmd(BOOL enabled) {
  if (!RunningOSX()) return;
  if (enabled)
    ::EnableMenuCommand(::GetMenuHandle(appleMenuID), kHICommandPreferences);
  else
    ::DisableMenuCommand(::GetMenuHandle(appleMenuID), kHICommandPreferences);
} /* CApplication::EnablePrefsCmd */

void CApplication::EnableAboutCmd(BOOL enabled) {
  if (!RunningOSX()) return;
  if (enabled)
    ::EnableMenuCommand(::GetMenuHandle(appleMenuID), kHICommandAbout);
  else
    ::DisableMenuCommand(::GetMenuHandle(appleMenuID), kHICommandAbout);
} /* CApplication::EnableAboutCmd */

/*---------------------------------- Low Level Menu Handling
 * -------------------------------------*/

void CApplication::ClickMenuBar(void) {
  LONG menuCommand;
  Point pt;

  ::GetMouse(&pt);

  if (theApp->OSVersion() < 0x0850)
    menuCommand = MenuSelect(pt);
  else {
    BOOL wasVisible = ::IsMenuBarVisible();
    if (!wasVisible) ::ShowMenuBar();
    menuCommand = MenuSelect(pt);
    if (!wasVisible) ::HideMenuBar();
  }

  DoMenuCommand(menuCommand);
} /* CApplication::ClickMenuBar */

void CApplication::DoMenuCommand(LONG menuCommand) {
  INT menu = HiWord(menuCommand);
  INT item = LoWord(menuCommand);

  Sleep(5);
  HiliteMenu(0);

  if (menu == appleMenuID) {
    if (item == 1) {
      if (!RunningOSX()) ::DisableMenuItem(::GetMenuHandle(menu), 1);
      HandleAbout();
      if (!RunningOSX())
        ::EnableMenuItem(::GetMenuHandle(menu), 1);
      else
        EnableAboutCmd(true);
    } else {
      Deactivate();
    }
  } else {
    if (item > 0) {
      for (int i = 0; i < cApp_MaxMenus; i++)
        if (MenuTab[i]->menuID == menu && MenuTab[i]->inMenuList) {
          INT command = MenuTab[i]->ItemID[item - 1];
          if (MenuTab[i]->MenuItemEnabled(command) && !HandleMessage(command) &&
              GetFrontWindow())
            GetFrontWindow()->HandleMessage(command);
          return;  // Must return here, otherwise crashes under OS X???
        }
    }
  }
} /* CApplication::DoMenuCommand */

/**************************************************************************************************/
/*                                                                                                */
/*                                       WINDOW HANDLING */
/*                                                                                                */
/**************************************************************************************************/

CWindow *CApplication::GetFrontWindow(void) {
  CWindow *frontWin;
  WindowPtr win = ::FrontWindow();
  if (!win) return nil;
  frontWin = (CWindow *)GetWRefCon(win);
  return (frontWin->sheetChild ? frontWin->sheetChild : frontWin);
} /* CApplication::GetFrontWindow */

void CApplication::SetFrontWindow(CWindow *cwin) {
  ::SelectWindow(cwin->winRef);
} /* CApplication::SetFrontWindow */

void CApplication::ActivateFrontWindow(BOOL activated) {
  if (GetFrontWindow())
    GetFrontWindow()->DispatchActivate(activated);
  else
    HandleMenuAdjust();
} /* CApplication::ActivateFrontWindow */

void CApplication::CycleWindows(BOOL forward) {
  if (winList.Count() > 1 && !IsMovableModal(FrontWindow()) &&
      winList.Find((PTR)GetFrontWindow()))
    SetFrontWindow(
        (CWindow *)(forward ? winList.NextCyclic() : winList.PrevCyclic()));
} /* CApplication::CycleWindows */

BOOL CApplication::ModalLoopRunning(void) {
  return (modalCount > 0);
  // CWindow *frontWin = GetFrontWindow();
  // return (frontWin && frontWin->IsModalDialog());
} /* CApplication::ModalDialogRunning */

void CApplication::ModalLoopBegin(void) {
  modalCount++;
}  // CApplication::ModalLoopBegin

void CApplication::ModalLoopEnd(void) {
  modalCount--;
}  // CApplication::ModalLoopEnd

/*------------------------------------------ Utility
 * ---------------------------------------------*/

CWindow *CApplication::LookupCWindow(WindowPtr win) {
  winList.Scan();
  while (CWindow *window = (CWindow *)(winList.Next()))
    if ((WindowPtr)(window->winRef) == win) return window;
  return nil;
} /* CApplication::LookupWindow */

void CApplication::HandleWindowCreated(CWindow *theWin) {
} /* CApplication::HandleWindowCreated */

void CApplication::HandleWindowDestroyed(CWindow *theWin) {
} /* CApplication::HandleWindowDestroyed */

/**************************************************************************************************/
/*                                                                                                */
/*                                        MISCELLANEOUS */
/*                                                                                                */
/**************************************************************************************************/

void CApplication::GetMouseLoc(CPoint *p) {
  Point mp;

  ::GetMouse(&mp);
  p->h = mp.h;
  p->v = mp.v;
} /* CApplication::GetMouseLoc */

/*
BOOL CApplication::MouseButtonDown (void)   // Shouldn't be used in tight loop
{
   return ::Button();
}  CApplication::MouseButtonDown*/

void CApplication::WaitMouseReleased(void) {
  Point mp;
  MouseTrackingResult mresult;
  ::TrackMouseLocation(nil, &mp, &mresult);
} /* CApplication::WaitMouseReleased */

void CApplication::ShowHideCursor(BOOL visible) {
  if (visible)
    ::ShowCursor();
  else
    ::HideCursor();
} /* CApplication::ShowHideCursor */

BOOL CApplication::PageSetupDialog(void) {
  return ::Print_PageSetupDialog();
} /* CApplication::PageSetupDialog */

void CApplication::PlaySound(INT soundID, BOOL async) {
  if (sndChan) {
    Handle sh = ::GetResource(soundListRsrc, soundID);
    ::HLock(sh);
    OSErr err = ::SndPlay(sndChan, (SndListHandle)sh, true);
    ::HUnlock(sh);
  }
} /* CApplication::PlaySound */

void CApplication::HandleLaunch(void) {} /* CApplication::HandleLaunch */

void CApplication::HandleActivate(BOOL activated) {
} /* CApplication::HandleActivate */

void CApplication::HandleOpenFile(CFile *file) {
  // The default handler does nothing
} /* CApplication::HandleOpenFile */

void CApplication::ShowMenuBar(BOOL visible) {
  if (visible == ::IsMenuBarVisible()) return;

  if (visible)
    ::ShowMenuBar();
  else
    ::HideMenuBar();

  mbarVisChanged = true;
} /* CApplication::ShowMenuBar */

void CApplication::ShowControlStrip(BOOL show) {
#ifndef TARGET_API_MAC_CARBON
  ::SBShowHideControlStrip(show);
#endif
} /* CApplication::ShowControlStrip */

void CApplication::EnableSocketEvents(BOOL enabled) {
  checkSocketEvents = enabled;
} /* CApplication::EnableSocketEvents */

/*---------------------------------- Screen/Window Rectangles
 * ------------------------------------*/

CRect CApplication::ScreenRect(void) {
  BitMap screenBits;
  ::GetQDGlobalsScreenBits(&screenBits);
  Rect mr = screenBits.bounds;  // Fetch screen rectangle
  return CRect(mr.left, mr.top, mr.right, mr.bottom);
} /* CApplication::ScreenRect */

void CApplication::CentralizeRect(
    CRect *r, BOOL toScreen)  // Centralizes rect with respect to front
{                             // window, if any; otherwise with respect to
  if (!toScreen && theApp->GetFrontWindow())  // screen.
    theApp->GetFrontWindow()->CentralizeRect(r);
  else {
    CRect frame = ScreenRect();
    INT h = Max(20, (frame.Width() - r->Width()) / 2);
    INT v = Max(20, (frame.Height() - r->Height()) / 2);
    r->Normalize();
    r->Offset(frame.left + h, frame.top + v);
  }

  if (r->left < 5) r->Offset(5 - r->left, 0);
  if (r->top < 45) r->Offset(0, 45 - r->top);
} /* CApplication::CentralizeRect */

void CApplication::StackRect(CRect *r, INT hor,
                             INT ver)  // Stacks rect with respect to front
{  // window, if any; otherwise with respect to
  CWindow *frontWin = theApp->GetFrontWindow();
  CRect frame(0, 0, 0, 0);

  if (frontWin) frame = frontWin->Frame();

  r->Normalize();
  r->Offset(frame.left + hor, frame.top + ver);
  if (!frontWin || r->right > ScreenRect().right ||
      r->bottom > ScreenRect().bottom)
    r->Offset(10 - r->left, 45 - r->top);

  if (r->left < 5) r->Offset(5 - r->left, 0);
  if (r->top < 45) r->Offset(0, 45 - r->top);
} /* CApplication::StackRect */

CRect CApplication::NewDocRect(INT width, INT height) {
  CRect r(0, 0, width, height);
  if (!GetFrontWindow())
    CentralizeRect(&r);
  else
    StackRect(&r);
  return r;
} /* CApplication::NewDocRect */

/*------------------------------------- Clipboard Handling
 * ---------------------------------------*/

void CApplication::ResetClipboard(void) {
  ::LoadScrap();
  ::ClearCurrentScrap();
  ::UnloadScrap();
} /* CApplication::ResetClipboard*/

INT CApplication::ReadClipboard(OSTYPE type, PTR *data, LONG *size) {
  OSStatus err = noErr;
  ScrapRef scrapRef;

  *data = nil;
  *size = kScrapFlavorSizeUnknown;

  if ((err = ::GetCurrentScrap(&scrapRef)) != noErr)
    return appErr_ClipboardReadError;

  ::LoadScrap();

  ScrapFlavorType flavorType = type;
  ScrapFlavorFlags flavorFlags = kScrapFlavorMaskNone;

  if ((err = ::GetScrapFlavorFlags(scrapRef, flavorType, &flavorFlags)) ==
          noErr &&
      (err = ::GetScrapFlavorSize(scrapRef, flavorType, size)) == noErr) {
    *data = Mem_AllocPtr(*size);
    if (*data) err = ::GetScrapFlavorData(scrapRef, flavorType, size, *data);
  }

  ::UnloadScrap();

  if (err == noErr)
    return (*data ? appErr_NoError : appErr_MemFullError);
  else
    return appErr_ClipboardReadError;
} /* CApplication::ReadClipboard */

INT CApplication::WriteClipboard(OSTYPE type, PTR data, LONG size) {
  OSStatus err = noErr;
  ScrapRef scrapRef;

  err = ::LoadScrap();

  if (err == noErr) err = ::GetCurrentScrap(&scrapRef);
  if (err == noErr)
    err = ::PutScrapFlavor(scrapRef, type, kScrapFlavorMaskNone, size, data);

  err = ::UnloadScrap();

  return (err == noErr ? appErr_NoError : appErr_ClipboardWriteError);
} /* CApplication::WriteClipboard*/

/*---------------------------------------- Color Picker
 * ------------------------------------------*/

BOOL CApplication::ColorPicker(CHAR *prompt, RGB_COLOR *color) {
  ActivateFrontWindow(false);

  Str255 pprompt;
  ::C2P_Str(prompt, pprompt);
  RGBColor incolor = *color;
  Point where = {-1, -1};
  BOOL colorPicked = ::GetColor(where, pprompt, &incolor, color);

  ActivateFrontWindow(true);

  return colorPicked;
} /* CApplication::ColorPicker */

/**************************************************************************************************/
/*                                                                                                */
/*                                     APPLE EVENT HANDLING */
/*                                                                                                */
/**************************************************************************************************/

static pascal OSErr AEHandleOapp(const AEDescList *aevt, AEDescList *reply,
                                 long refCon);
static pascal OSErr AEHandleQuit(const AEDescList *aevt, AEDescList *reply,
                                 long refCon);
static pascal OSErr AEHandleOdoc(const AEDescList *aevt, AEDescList *reply,
                                 long refCon);
static pascal OSErr AEHandlePdoc(const AEDescList *aevt, AEDescList *reply,
                                 long refCon);
static pascal OSErr AEHandlePref(const AEDescList *aevt, AEDescList *reply,
                                 long refCon);

static AEDesc gSelf;  // target for Apple Events to self

void CApplication::InitAppleEvents(void) {
  // First verify that we actually have AppleEvent Support:
  long result;
  if (Gestalt(gestaltAppleEventsAttr, &result) != noErr) return;

  // If so, then initialize AppleEvents:
  AppleEvent *firstEvent, *firstReply;
  static AppleEvent firstevent = {0, NULL}, firstreply = {0, NULL};
  OSErr err = noErr;
  ProcessSerialNumber thePSN = {0, kCurrentProcess};

  firstEvent = &firstevent;
  firstReply = &firstreply;

  // Make target for messages to self
  err = AECreateDesc(typeProcessSerialNumber, (Ptr)&thePSN, sizeof(thePSN),
                     &gSelf);
  if (err != noErr) return;

  HandleAEInstall();
} /* CApplication::InitAppleEvents */

void CApplication::HandleAEInstall(void)  // Default AppleEvent installer
{
  OSErr err;
  err = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
                              NewAEEventHandlerUPP(AEHandleOapp), 0, false);
  err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
                              NewAEEventHandlerUPP(AEHandleOdoc), 0, false);
  err = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
                              NewAEEventHandlerUPP(AEHandlePdoc), 0, false);
  err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
                              NewAEEventHandlerUPP(AEHandleQuit), 0, false);
  err = AEInstallEventHandler(kCoreEventClass, kAEShowPreferences,
                              NewAEEventHandlerUPP(AEHandlePref), 0, false);
} /* CApplication::HandleAEInstall */

static pascal OSErr AEHandleOapp(const AEDescList *aevt, AEDescList *reply,
                                 long refCon) {
  //   theApp->HandleLaunch();  ###
  return noErr;
} /* AEHandleOapp */

static pascal OSErr AEHandleOdoc(const AEDescList *aevt, AEDescList *reply,
                                 long refCon) {
  AEDesc fileListDesc = {'NULL', NULL};
  long numFiles;
  DescType actualType;
  long actualSize;
  AEKeyword actualKeyword;

  OSErr err = noErr;

  // The "odoc" and "pdoc" messages contain a list of aliases as the direct
  // paramater. This means that we'll need to extract the list, count the list's
  // elements, and then process each file in turn.

  // Extract the list of aliases into fileListDesc
  err = AEGetKeyDesc(aevt, keyDirectObject, typeAEList, &fileListDesc);
  if (err) goto done;

  // Count the list elements
  err = AECountItems(&fileListDesc, &numFiles);
  if (err) goto done;

  // now get each file from the list and process it.
  // Even though the event contains a list of alises, the Apple Event Manager
  // will convert each alias to an FSSpec if we ask it to.
  for (LONG index = 1; index <= numFiles; index++) {
    FSSpec theSpec;
    err = AEGetNthPtr(&fileListDesc, index, typeFSS, &actualKeyword,
                      &actualType, (Ptr)&theSpec, sizeof(theSpec), &actualSize);
    if (err) goto done;
    theApp->HandleOpenFSSpec(&theSpec);
  }

done:
  AEDisposeDesc(&fileListDesc);

  return err;
} /* AEHandleOdoc */

static pascal OSErr AEHandlePdoc(const AEDescList *aevt, AEDescList *reply,
                                 long refCon) {
  return errAEEventNotHandled;
} /* AEHandlePdoc */

static pascal OSErr AEHandleQuit(const AEDescList *aevt, AEDescList *reply,
                                 long refCon) {
  theApp->Quit();
  return noErr;
} /* AEHandleQuit */

static pascal OSErr AEHandlePref(const AEDescList *aevt, AEDescList *reply,
                                 long refCon) {
  ::HiliteMenu(0);
  ::DisableMenuCommand(::GetMenuHandle(appleMenuID), kHICommandPreferences);
  theApp->HandleShowPrefs();
  ::EnableMenuCommand(::GetMenuHandle(appleMenuID), kHICommandPreferences);
  return noErr;
} /* AEHandlePref */

void CApplication::HandleOpenFSSpec(FSSpec *theSpec) {
  CFile *cfile = new CFile();
  FSSpec2CFile(theSpec, cfile);
  HandleOpenFile(cfile);
  delete cfile;
} /* CApplication::HandleOpenFSSpec */

/**************************************************************************************************/
/*                                                                                                */
/*                                     FINDER/PROCESS INTERFACE */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------------- Launch Application
 * -------------------------------------*/

OSErr FinderLaunch(long nTargets, FSSpec *targetList);

INT CApplication::LaunchApp(CHAR *appName, BOOL background, BOOL hide) {
  StrFileName pAppFile;
  C2P_Str(appName, pAppFile);
  FSSpec appFSSpec;
  ::FSMakeFSSpec(0, 0, pAppFile, &appFSSpec);

  LaunchParamBlockRec LP;
  LP.reserved1 = 0;
  LP.reserved2 = 0;
  LP.launchBlockID = extendedBlock;
  LP.launchEPBLength = extendedBlockLen;
  LP.launchFileFlags = 0;  // Set by specifying "launchNoFileFlags" below
  LP.launchControlFlags = launchContinue | launchNoFileFlags;
  if (background) LP.launchControlFlags |= launchDontSwitch;
  LP.launchAppSpec = &appFSSpec;

  LP.launchProcessSN.highLongOfPSN = 0;
  LP.launchProcessSN.lowLongOfPSN = 0;
  LP.launchPreferredSize = 0;
  LP.launchMinimumSize = 0;
  LP.launchAvailableSize = 0;
  LP.launchAppParameters = NULL;  //...AppParametersPtr;

  OSErr err = LaunchApplication(&LP);

  if (err == noErr) {
    theApp->ProcessEvents();
    if (hide) ::ShowHideProcess(&LP.launchProcessSN, false);
  }

  return err;
}  // CApplication::LaunchApp

/*
INT CApplication::LaunchConsoleApp (CHAR *appName)
{
   StrFileName pAppFile;
   C2P_Str(appName, pAppFile);
   FSSpec appFSSpec;
   ::FSMakeFSSpec(0,0,pAppFile, &appFSSpec);

   OSErr err = FinderLaunch(1, &appFSSpec);

   return err;
} // CApplication::LaunchConsoleApp
*/
/*
OSErr FinderLaunch(long nTargets, FSSpec *targetList)
{
        OSErr err;
        AppleEvent theAEvent, theReply;
        AEAddressDesc fndrAddress;
        AEDescList targetListDesc;
        OSType fndrCreator;
        Boolean wasChanged;
        AliasHandle targetAlias;
        long index;

        // Verify parameters
        if ((nTargets == 0) || (targetList == NULL)) return paramErr;

        // Set up locals
        AECreateDesc(typeNull, NULL, 0, &theAEvent);
        AECreateDesc(typeNull, NULL, 0, &fndrAddress);
        AECreateDesc(typeNull, NULL, 0, &theReply);
        AECreateDesc(typeNull, NULL, 0, &targetListDesc);
        targetAlias = NULL;
        fndrCreator = 'MACS';  // 'trmx' for Terminal

   // create an open documents event targeting the finder
        err = AECreateDesc(typeApplSignature, (Ptr) &fndrCreator,
sizeof(fndrCreator), &fndrAddress); if (err != noErr) goto bail;

   err = AECreateAppleEvent(kCoreEventClass, kAEOpenDocuments, &fndrAddress,
kAutoGenerateReturnID, kAnyTransactionID, &theAEvent); if (err != noErr) goto
bail;

        // Create the list of files to open
        err = AECreateList(NULL, 0, false, &targetListDesc);
        if (err != noErr) goto bail;

        for (index = 0; index < nTargets; index++)
        {
                if (targetAlias == NULL)
                        err = NewAlias(NULL, (targetList + index),
&targetAlias); else err = UpdateAlias(NULL, (targetList + index), targetAlias,
&wasChanged); if (err != noErr) goto bail; HLock((Handle) targetAlias); err =
AEPutPtr(&targetListDesc, (index + 1), typeAlias, *targetAlias,
GetHandleSize((Handle) targetAlias)); HUnlock((Handle) targetAlias); if (err !=
noErr) goto bail;
        }

        // Add the file list to the apple event
        err = AEPutParamDesc(&theAEvent, keyDirectObject, &targetListDesc);
        if (err != noErr) goto bail;

        // Send the event to the Finder
        err = AESend(&theAEvent, &theReply, kAENoReply, kAENormalPriority,
kAEDefaultTimeout, NULL, NULL);

        // Clean up and leave
bail:
        if (targetAlias != NULL) DisposeHandle((Handle) targetAlias);
        AEDisposeDesc(&targetListDesc);
        AEDisposeDesc(&theAEvent);
        AEDisposeDesc(&fndrAddress);
        AEDisposeDesc(&theReply);
        return err;
}
*/

/*---------------------------------------- Quit Application
 * --------------------------------------*/

INT CApplication::QuitApp(OSTYPE appCreator) {
  OSErr err;
  AppleEvent theAEvent, theReply;
  AEAddressDesc appAddress;

  // set up locals
  AECreateDesc(typeNull, NULL, 0, &theAEvent);
  AECreateDesc(typeNull, NULL, 0, &appAddress);
  AECreateDesc(typeNull, NULL, 0, &theReply);

  err = AECreateDesc(typeApplSignature, (Ptr)&appCreator, sizeof(appCreator),
                     &appAddress);
  if (err != noErr) goto bail;

  // Create a quit app event targeting the specified app
  err =
      AECreateAppleEvent(kCoreEventClass, kAEQuitApplication, &appAddress,
                         kAutoGenerateReturnID, kAnyTransactionID, &theAEvent);
  if (err != noErr) goto bail;

  // Send the event to the Finder
  err = AESend(&theAEvent, &theReply, kAENoReply, kAENormalPriority,
               kAEDefaultTimeout, NULL, NULL);

  // Clean up and leave
bail:
  AEDisposeDesc(&theAEvent);
  AEDisposeDesc(&appAddress);
  AEDisposeDesc(&theReply);
  return err;
}  // CApplication::QuitApp

/*---------------------------------------- Hide Application
 * --------------------------------------*/

INT CApplication::HideApp(OSTYPE appCreator) {
  OSErr err;
  ProcessSerialNumber PSN;
  ProcessInfoRec PIR;

  PIR.processInfoLength = sizeof(ProcessInfoRec);
  PIR.processName = nil;
  PIR.processAppSpec = nil;

  err = ::GetFrontProcess(&PSN);
  if (err != noErr) return err;

  err = ::GetProcessInformation(&PSN, &PIR);
  if (err != noErr) return err;

  if (PIR.processSignature == appCreator) err = ::ShowHideProcess(&PSN, false);

  return err;
}  // CApplication::HideApp
