/**************************************************************************************************/
/*                                                                                                */
/* Module  : CFont.c                                                                              */
/* Purpose : Implements a generic font object used in CView and CPrint.                           */
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

#include "CFont.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                          CONSTRUCTOR                                           */
/*                                                                                                */
/**************************************************************************************************/

CFont::CFont (FONT_FACE theFace, FONT_STYLE theStyle, INT theSize)
{
   face  = theFace;
   style = theStyle;
   size  = theSize;
} /* CFont::CFont */


/**************************************************************************************************/
/*                                                                                                */
/*                                    ACCESS TO FONT METRICS                                      */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Char/String Width ---------------------------------------*/

INT CFont::ChrWidth (CHAR c)
{
   SavePort();
      INT width = ::CharWidth(c);
   RestorePort();
   return width;
} /* CFont::ChrWidth */


INT CFont::MaxChrWidth (void)
{
   FontInfo info;
   SavePort(); ::GetFontInfo(&info); RestorePort();
   return info.widMax;
} /* CFont::MaxChrWidth */


INT CFont::StrWidth (CHAR *s)
{
   return StrWidth(s, 0, StrLen(s));
} /* CFont::StrWidth */


INT CFont::StrWidth (CHAR *s, INT pos, INT count)
{
   if (count <= 0) return 0;
   SavePort();
      INT width = ::TextWidth(s, pos, count);
   RestorePort();
   return width;
} /* CFont::StrWidth */

/*-------------------------------------- Height/Ascent/Descent -----------------------------------*/

INT CFont::Ascent (void)
{
   FontInfo info;
   SavePort(); ::GetFontInfo(&info); RestorePort();
   return info.ascent;
} /* CFont::Ascent */


INT CFont::Descent (void)
{
   FontInfo info;
   SavePort(); ::GetFontInfo(&info); RestorePort();
   return info.descent;
} /* CFont::Descent */


INT CFont::LineSpacing (void)
{
   FontInfo info;
   SavePort(); ::GetFontInfo(&info); RestorePort();
   return info.leading;
} /* CFont::LineSpacing */


INT CFont::Height (void)
{
   FontInfo info;
   SavePort(); ::GetFontInfo(&info); RestorePort();
   return info.ascent + info.descent + info.leading;
} /* CFont::Height */

/*------------------------------------------ Utility ---------------------------------------------*/

void CFont::SavePort (void)
{
   oldPort = nil;
   oldDevice = nil;
   ::GetGWorld(&oldPort, &oldDevice);
   oldFace  = ::GetPortTextFont(oldPort); ::TextFont(face);
   oldStyle = ::GetPortTextFace(oldPort); ::TextFace(style);
   oldSize  = ::GetPortTextSize(oldPort); ::TextSize(size);
} /* CFont::SavePort */


void CFont::RestorePort (void)
{
   ::TextFont(oldFace);
   ::TextFace(oldStyle);
   ::TextSize(oldSize);
} /* CFont::RestorePort */
