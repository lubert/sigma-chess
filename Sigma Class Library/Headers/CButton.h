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

#include "CBitmap.h"
#include "CMenu.h"
#include "CView.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

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

class CButton : public CView {
 public:
  CButton(CViewOwner *parent,
          CRect frame,     // Frame/location in parent view.
          INT command,     // Command id sent to window's handler.
          INT subCommand,  // Sub command id sent to window's handler.
          BOOL visible,    // Initially visible?
          BOOL enabled,    // Initially enabled?

          INT iconID,          // Icon button
          CHAR *title,         // Optional icon title written below icon
          CHAR *helpText = ""  // Optional help text (if ctrl clicking)
  );

  CButton(CViewOwner *parent,
          CRect frame,       // Frame/location in parent view.
          INT command,       // Command id sent to window's handler.
          INT subCommand,    // Sub command id sent to window's handler.
          CMenu *popupMenu,  // Popup menu
          BOOL visible,      // Initially visible?
          BOOL enabled,      // Initially enabled?

          INT iconID,          // Icon button
          CHAR *title,         // Optional icon title written below icon
          CHAR *helpText = ""  // Optional help text (if ctrl clicking)
  );

  CButton(CViewOwner *parent,
          CRect frame,     // Frame/location in parent view.
          INT command,     // Command id sent to window's handler.
          INT subCommand,  // Sub command id sent to window's handler.
          BOOL visible,    // Initially visible?
          BOOL enabled,    // Initially enabled?

          CHAR *text,          // Text button
          CHAR *helpText = ""  // Optional help text (if ctrl clicking)
  );

  CButton(CViewOwner *parent,
          CRect frame,     // Frame/location in parent view.
          INT command,     // Command id sent to window's handler.
          INT subCommand,  // Sub command id sent to window's handler.
          BOOL visible,    // Initially visible?
          BOOL enabled,    // Initially enabled?

          CBitmap *faceEnabled, CBitmap *faceDisabled,
          CRect *srcRectEnabled = nil, CRect *srcRectDisabled = nil,
          CHAR *helpText = "",         // Optional help text (if ctrl clicking)
          RGB_COLOR *transColor = nil  // Optional transparency color.
  );

  virtual ~CButton(void);

  void DrawBody(void);
  void DrawFace(void);
  void Press(BOOL isPressed);
  void Enable(BOOL wasEnabled, BOOL redraw = true);
  void Show(BOOL isVisible, BOOL redraw = true);
  void SetHelpText(CHAR *s);
  void SetIcon(INT iconID, BOOL redraw = true);
  void SetOnOff(void);

  virtual void HandleUpdate(
      CRect updateRect);  // Need not be overridden by the user.
  virtual BOOL HandleMouseDown(CPoint pt, INT modifiers, BOOL doubleClick);
  virtual void HandleActivate(BOOL wasActivated);

  BOOL pressed;
  INT buttonCommand, buttonSubCommand;

 private:
  BOOL isTextButton;
  BOOL isIconButton;
  BOOL isOnOff;  // I.e. look disabled when pressed? (used for on/off toolbar
                 // buttons)
  CHAR text[30], *helpText;
  INT iconID;
  CMenu *popupMenu;
  CBitmap *bitmapEnabled, *bitmapDisabled;
  CRect rEnabled, rDisabled;
  RGB_COLOR transColor;
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
