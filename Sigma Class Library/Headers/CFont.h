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

#include "CUtility.h"
#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

enum FONT_FACE {
  font_System = systemFont,
  font_NewYork = kFontIDNewYork,
  font_Geneva = kFontIDGeneva,
  font_Times = kFontIDTimes,
  font_Fixed = kFontIDMonaco,
  font_Helvetica = kFontIDHelvetica
  /*
          kFontIDNewYork				= 2,
          kFontIDGeneva				= 3,
          kFontIDMonaco				= 4,
          kFontIDVenice				= 5,
          kFontIDLondon				= 6,
          kFontIDAthens				= 7,
          kFontIDSanFrancisco			= 8,
          kFontIDToronto				= 9,
          kFontIDCairo				= 11,
          kFontIDLosAngeles			= 12,
          kFontIDTimes				= 20,
          kFontIDHelvetica			= 21,
          kFontIDCourier				= 22,
          kFontIDSymbol				= 23,
          kFontIDMobile				= 24
  */
};

enum FONT_STYLE {
  fontStyle_Plain = 0,
  fontStyle_Bold = 1,
  fontStyle_Italic = 2,
  fontStyle_UnderLine = 4
};

enum FONT_MODE {
  fontMode_Copy = srcCopy,
  fontMode_Or = srcOr,
  fontMode_Xor = srcXor
};

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

class CFont {
 public:
  CFont(FONT_FACE face = font_Geneva, FONT_STYLE style = fontStyle_Plain,
        INT size = 10);

  INT ChrWidth(CHAR c);
  INT MaxChrWidth(void);
  INT StrWidth(CHAR *s);
  INT StrWidth(CHAR *s, INT pos, INT count);
  INT Ascent(void);
  INT Descent(void);
  INT LineSpacing(void);
  INT Height(void);

  FONT_FACE face;    // port->txFont
  FONT_STYLE style;  // port->txFace
  INT size;          // port->txSize

 private:
  void SavePort(void);
  void RestorePort(void);

  CGrafPtr oldPort;
  GDHandle oldDevice;
  INT oldFace;   // port->txFont
  INT oldStyle;  // port->txFace
  INT oldSize;   // port->txSize
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
