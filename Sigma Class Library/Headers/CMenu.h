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

#include "CUtility.h"
#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

#define cMenu_MaxItems 50

#define cMenu_NoShortCut 0
#define cMenu_NoIcon 0
#define cMenu_BlankIcon 259

#define cMenuModifier_None kMenuNoModifiers
#define cMenuModifier_Shift kMenuShiftModifier
#define cMenuModifier_Option kMenuOptionModifier
#define cMenuModifier_Control kMenuControlModifier
#define cMenuModifier_NoCmd kMenuNoCommandModifier

/**************************************************************************************************/
/*                                                                                                */
/*                                         TYPE DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

#ifdef CheckMenuItem
#undef CheckMenuItem
#endif

class CMenu {
 public:
  CMenu(CHAR *title);
  virtual ~CMenu(void);

  void Clear(void);

  void AddItem(CHAR *title, INT id, CHAR shortCut = cMenu_NoShortCut,
               INT modifiers = cMenuModifier_NoCmd, INT iconID = cMenu_NoIcon);
  void AddSeparator(void);

  void AddPopupHeader(CHAR *s, INT headerItemID = -1);

  void EnableAllItems(BOOL enable);
  void EnableMenuItem(INT id, BOOL enable);
  BOOL MenuItemEnabled(INT itemID);
  void CheckMenuItem(INT id, BOOL checked);

  void SetItemText(INT itemID, CHAR *text);
  void SetIcon(INT itemID, INT iconID);
  void ClrIcon(INT itemID);
  void SetShortcut(INT itemID, CHAR shortcut,
                   INT modifiers = cMenuModifier_NoCmd);
  void ClrShortcut(INT itemID);
  void SetSubMenu(INT itemID, CMenu *subMenu);
  void ClrSubMenu(INT itemID);
  void SetGlyph(INT itemID, INT glyph);

  BOOL Popup(INT *itemID);  // Call this in response to mouse down event in
                            // window/view.

  //--- Private ---
  BOOL inMenuBar;   // Added to application menubar?
  BOOL inMenuList;  // Submenu or active popup menu?
  INT menuID;       // Unique, auto generated id
  CMenu *parentMenu;
  INT parentItemNo;

  MenuHandle hmenu;

  INT itemCount;
  INT ItemID[cMenu_MaxItems];

  INT GetItemNo(INT itemID);
  INT GetItemID(INT itemNo);
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
