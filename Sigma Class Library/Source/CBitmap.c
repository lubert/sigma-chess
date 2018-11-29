/**************************************************************************************************/
/*                                                                                                */
/* Module  : CBITMAP.C */
/* Purpose : Implements the generic off screen bitmap class based on the Mac
 * GWorld concept.      */
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

#include "CBitmap.h"
#include "CUtility.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                          CONSTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CBitmap::CBitmap(INT width, INT height, INT depth)
    : CViewOwner(viewOwner_Bitmap) {
  // Build normalized bounds rectangle:
  bounds.Set(0, 0, width, height);

  // Create the offscreen graphics world:
  Rect mr;
  ::SetRect(&mr, 0, 0, width, height);
  createdOK = (NewGWorld(&gworld, depth, &mr, nil, nil, 0) == noErr);
} /* CBitmap::CBitmap */

CBitmap::CBitmap(INT picID,
                 INT depth)  // Creates bitmap directly from picture resource.
    : CViewOwner(viewOwner_Bitmap) {
  // First load picture handle from resource
  PicHandle ph = GetPicture(picID);

  // Then calc frame rectangle and create offscreen graphics world:
  Rect mr = (**ph).picFrame;
  ::OffsetRect(&mr, -mr.left, -mr.top);
  createdOK = (NewGWorld(&gworld, depth, &mr, nil, nil, 0) == noErr);
  bounds.Set(0, 0, mr.right, mr.bottom);

  // Next draw the picture in the offscreen window:
  CGrafPtr oldPort = nil;
  GDHandle oldDevice = nil;

  ::GetGWorld(&oldPort, &oldDevice);
  ::SetGWorld(gworld, nil);
  ::DrawPicture(ph, &mr);
  ::SetGWorld(oldPort, oldDevice);

  // Finally release picture resource:
  ::ReleaseResource((Handle)ph);
} /* CBitmap::CBitmap */

void CBitmap::LoadPicture(INT picID) {
  // First load picture handle from resource
  PicHandle ph = GetPicture(picID);

  // Then calc frame rectangle and create offscreen graphics world:
  Rect mr;
  bounds.SetMacRect(&mr);

  // Next draw the picture in the offscreen window:
  CGrafPtr oldPort = nil;
  GDHandle oldDevice = nil;

  ::GetGWorld(&oldPort, &oldDevice);
  ::SetGWorld(gworld, nil);
  ::DrawPicture(ph, &mr);
  ::SetGWorld(oldPort, oldDevice);

  // Finally release picture resource:
  ::ReleaseResource((Handle)ph);
} /* CBitmap::LoadPicture */

/**************************************************************************************************/
/*                                                                                                */
/*                                          DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CBitmap::~CBitmap(void) {
  while (vFirstChild) delete vFirstChild;

  // Finally destroy the underlying Mac window:
  if (gworld) DisposeGWorld(gworld);
} /* CBitmap::~CBitmap*/

/**************************************************************************************************/
/*                                                                                                */
/*                                         MISC METHODS */
/*                                                                                                */
/**************************************************************************************************/

void CBitmap::Lock(void) {
  LockPixels(GetGWorldPixMap(gworld));
} /* CBitmap::Lock */

void CBitmap::Unlock(void) {
  UnlockPixels(GetGWorldPixMap(gworld));
} /* CBitmap::Unlock */

void CBitmap::Disable(RGB_COLOR *exceptColor) {
  CGrafPtr oldPort = nil;
  GDHandle oldDevice = nil;
  RGB_COLOR c;

  ::GetGWorld(&oldPort, &oldDevice);
  ::SetGWorld(gworld, nil);

  for (INT h = 0; h < bounds.right; h++)
    for (INT v = 0; v < bounds.bottom; v++) {
      ::GetCPixel(h, v, &c);
      if (!exceptColor || c.red != exceptColor->red ||
          c.green != exceptColor->green || c.blue != exceptColor->blue) {
        UINT avg = ((ULONG)c.red + (ULONG)c.green + (ULONG)c.blue) / 2;
        c.red = c.green = c.blue = avg;
        ::SetCPixel(h, v, &c);
      }
    }

  ::SetGWorld(oldPort, oldDevice);
} /* CBitmap::Disable */
