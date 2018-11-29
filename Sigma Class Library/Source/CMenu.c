/**************************************************************************************************/
/*                                                                                                */
/* Module  : CMENU.C                                                                              */
/* Purpose : Implements the generic menu class.                                                   */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
  and the following disclaimer in the documentation and/or other materials provided with the 
  distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CMenu.h"
#include "CApplication.h"
#include "CUtility.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                      CONSTRUCTOR/DESTRUCTOR                                    */
/*                                                                                                */
/**************************************************************************************************/

CMenu::CMenu (CHAR *title)
{
   Str255 ptitle;

   theApp->RegisterMenu(this);
   ::C2P_Str(title, ptitle);
   hmenu = ::NewMenu(menuID, ptitle);
   inMenuBar  = false;
   inMenuList = false;
   parentMenu = nil;
   parentItemNo = 0;
   itemCount  = 0;
} /* CMenu::CMenu */


CMenu::~CMenu (void)
{
   if (parentMenu)
      parentMenu->ClrSubMenu(parentItemNo);
   ::DisposeMenu(hmenu);
   theApp->UnregisterMenu(this);
} /* CMenu::CMenu */


void CMenu::Clear (void)     // Removes all items in a menu
{
   ::DeleteMenuItems(hmenu, 1, itemCount);
   itemCount = 0;
// while (itemCount-- > 0)
//    ::DeleteMenuItem(hmenu, 1);
} /* CMenu::Clear */


/**************************************************************************************************/
/*                                                                                                */
/*                                      ADDING ITEMS/SUBMENUS                                     */
/*                                                                                                */
/**************************************************************************************************/

void CMenu::AddItem (CHAR *title, INT id, CHAR shortCut, INT modifiers, INT iconID)
{
   Str255 ptitle;

   ItemID[itemCount++] = id;

   ::C2P_Str(title, ptitle);
   ::AppendMenu(hmenu, "\ptmp");
   ::SetMenuItemText(hmenu, itemCount, ptitle);  // To avoid meta characters like '/'

   ::SetItemCmd(hmenu, itemCount, shortCut);
   if (shortCut != cMenu_NoShortCut && modifiers != cMenuModifier_NoCmd)
      ::SetMenuItemModifiers(hmenu, itemCount, modifiers);

   if (iconID != cMenu_NoIcon)
      ::SetItemIcon(hmenu, itemCount, iconID - 256);
} /* CMenu::AddItem */


void CMenu::AddSeparator (void)
{
   ::AppendMenu(hmenu, "\p(----------");
   ItemID[itemCount++] = -1;
} /* CMenu::AddSeparator */


void CMenu::AddPopupHeader (CHAR *s, INT headerItemID)
{
   AddItem(s, headerItemID);
   EnableMenuItem(headerItemID, false);
   AddSeparator();
} /* CMenu::AddPopupHeader */


/**************************************************************************************************/
/*                                                                                                */
/*                                 SETTING ITEM CHARACTERISTICS                                   */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Enable/Disable ------------------------------------------*/

void CMenu::EnableAllItems (BOOL enable)
{
   if (enable)
      ::EnableMenuItem(hmenu, 0);
   else
      ::DisableMenuItem(hmenu, 0);
} /* CMenu::EnableAllItems */


void CMenu::EnableMenuItem (INT itemID, BOOL enable)
{
   if (enable)
      ::EnableMenuItem(hmenu, GetItemNo(itemID));
    else
      ::DisableMenuItem(hmenu, GetItemNo(itemID));
} /* CMenu::EnableMenuItem */


BOOL CMenu::MenuItemEnabled (INT itemID)
{
   INT n = GetItemNo(itemID);
   return ::IsMenuItemEnabled(hmenu, n);
} /* CMenu::MenuItemEnabled */


void CMenu::CheckMenuItem (INT itemID, BOOL check)
{
   ::CheckMenuItem(hmenu, GetItemNo(itemID), check);
} /* CMenu::CheckMenuItem */

/*-------------------------------------- Set Properties ------------------------------------------*/

void CMenu::SetItemText (INT itemID, CHAR *text)
{
   Str255 ptext;
   C2P_Str(text, ptext);
   ::SetMenuItemText(hmenu, GetItemNo(itemID), ptext);
} /* CMenu::SetItemText */


void CMenu::SetIcon (INT itemID, INT iconID)
{
   ::SetItemIcon(hmenu, GetItemNo(itemID), iconID - 256);
} /* CMenu::SetIcon */


void CMenu::ClrIcon (INT itemID)
{
   ::SetItemIcon(hmenu, GetItemNo(itemID), 0);
} /* CMenu::ClrIcon */


void CMenu::SetShortcut (INT itemID, CHAR shortcut, INT modifiers)
{
   ::SetItemCmd(hmenu, GetItemNo(itemID), shortcut);

   if (shortcut != cMenu_NoShortCut && modifiers != cMenuModifier_NoCmd)
      ::SetMenuItemModifiers(hmenu, GetItemNo(itemID), modifiers);
} /* CMenu::SetShortcut */


void CMenu::ClrShortcut (INT itemID)
{
   ::SetItemCmd(hmenu, GetItemNo(itemID), 0);
} /* CMenu::ClrShortcut */


void CMenu::SetGlyph (INT itemID, INT glyph)
{
   ::SetMenuItemKeyGlyph(hmenu, GetItemNo(itemID), glyph);
} /* CMenu::SetGlyph */


void CMenu::SetSubMenu (INT itemID, CMenu *subMenu)
{
   if (subMenu->inMenuBar || subMenu->inMenuList) return; // Internal error

   INT itemNo = GetItemNo(itemID);
   ::SetItemCmd(hmenu, itemNo, 0x1B);
   ::SetItemMark(hmenu, itemNo, subMenu->menuID);
   ::InsertMenu(subMenu->hmenu, -1);
   subMenu->parentMenu = this;
   subMenu->parentItemNo = itemNo;
   subMenu->inMenuList = true;
} /* CMenu::SetSubMenu */


void CMenu::ClrSubMenu (INT itemID)
{
   INT subMenuID;
   GetItemMark(hmenu, GetItemNo(itemID), &subMenuID);
   CMenu *subMenu = theApp->LookupMenu(subMenuID);

   if (subMenu->inMenuBar || ! subMenu->inMenuList) return; // Internal error

   INT itemNo = GetItemNo(itemID);
   ::SetItemCmd(hmenu, itemNo, 0);
   ::SetItemMark(hmenu, itemNo, 0);
   ::DeleteMenu(subMenu->menuID);
   subMenu->parentMenu = nil;
   subMenu->inMenuList = false;
} /* CMenu::ClrSubMenu */

/*------------------------------------------ Utility ---------------------------------------------*/

INT CMenu::GetItemID (INT itemNo)
{
   if (itemNo < 1 || itemNo > itemCount) return 0;
   return ItemID[itemNo - 1];
} /* CMenu::GetItemID */


INT CMenu::GetItemNo (INT itemID)
{
   for (INT i = 0; i < itemCount; i++)
      if (ItemID[i] == itemID)
         return i + 1;
   return 0;
} /* CMenu::GetItemNo) */


/**************************************************************************************************/
/*                                                                                                */
/*                                             MISC                                               */
/*                                                                                                */
/**************************************************************************************************/

BOOL CMenu::Popup (INT *itemID)
{
   Point  gpt;
   UInt16 itemNo;

   // Determine popup origin (but move up if clicked "near" bottom of screen)
   ::GetMouse(&gpt);
   ::LocalToGlobal(&gpt);
   BitMap screenBits;
   ::GetQDGlobalsScreenBits(&screenBits);
   gpt.v = Min(gpt.v, screenBits.bounds.bottom - /*::GetMenuHeight(hmenu) -*/ 200);

   ::InsertMenu(hmenu, -1); inMenuList = true;
   itemNo = ::PopUpMenuSelect(hmenu, gpt.v, gpt.h, 0);
   ::DeleteMenu(menuID); inMenuList = false;

   if (itemNo < 1 || itemNo > itemCount) return false;
   *itemID = GetItemID(itemNo);
   return true;
} /* CMenu::Popup */
