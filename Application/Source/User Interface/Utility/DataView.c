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

#include "DataView.h"
#include "GameWindow.h"
#include "CollectionWindow.h"
#include "SigmaApplication.h"


DataView::DataView (CViewOwner *parent, CRect frame, BOOL erase)
 : CView(parent, frame)
{
   eraseContents = erase;
} /* DataView::DataView */


void DataView::HandleUpdate (CRect updateRect)
{
   CRect r = bounds;

   SetForeColor(RunningOSX() || ! Active() ? &color_MdGray : &color_Black);
   DrawRectFrame(r);

   if (eraseContents)
   {  r.Inset(1,1);
      DrawRectFill(r, &color_White);
   }

   SetForeColor(&color_Black);
} /* DataView::Draw */


void DataView::CalcDimensions (CRect *headerRect, CRect *dataRect, CRect *scrollRect, INT headerHeight)
{
   CRect r = bounds;
   headerRect->Set(r.left, r.top, r.right, r.top + headerHeight);
   
   r.top += headerHeight - 1;
   r.Inset(1, 1);
   dataRect->Set(r.left, r.top, r.right, r.bottom);
   if (scrollRect)
   {  dataRect->right -= controlWidth_ScrollBar - 1;
      scrollRect->Set(dataRect->right, r.top, r.right + 1, r.bottom);
      if (! RunningOSX()) scrollRect->Inset(0, -1);
   }
} /* DataView::CalcDimensions */


CGame *DataView::Game (void)
{
   return ((GameWindow *)Window())->game;
} /* DataView::Game */


SigmaCollection *DataView::Collection (void)
{
   return ((CollectionWindow *)Window())->collection;
} /* DataView::Collection */
