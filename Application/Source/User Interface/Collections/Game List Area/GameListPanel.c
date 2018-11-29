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

#include "GameListPanel.h"
#include "CollectionWindow.h"
#include "SigmaApplication.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                        GAME LIST PANEL                                         */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Constructor/Destructor ------------------------------------*/
// The game list panel is a general purpose area below the actual game list. It shows any progress
// bars as well as collection info and statistics (e.g. score in %).

PanelView::PanelView (CViewOwner *owner, CRect frame)
 : DataHeaderView(owner, frame, false, true, 1, HCTab)
{
   CopyStr("", s);
   HCTab[0].text = s;
   HCTab[0].width = -1;
} /* PanelView::PanelView */

/*--------------------------------------- Event Handling -----------------------------------------*/

void PanelView::HandleUpdate (CRect updateRect)
{
   SigmaCollection *col = ((CollectionWindow*)Window())->collection;
   if (col->useFilter)
      Format(s, "%ld of %ld games (filter applied)", col->View_GetGameCount(), col->GetGameCount());
   else
      Format(s, "%ld games", col->GetGameCount());

   DataHeaderView::HandleUpdate(updateRect);
} /* PanelView::HandleUpdate */
