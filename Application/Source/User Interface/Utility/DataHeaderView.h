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

#pragma once

#include "CView.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

typedef struct
{
   CHAR *text;
   INT  iconID;   // 0 if no icon
   INT  width;    // Width of last entry ignored
} HEADER_COLUMN;


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class DataHeaderView : public CView
{
public:
   DataHeaderView
   (  CViewOwner    *parent,
      CRect         frame,
      BOOL          enabled    = false,
      BOOL          blackFrame = true,
      INT           columns    = 0,
      HEADER_COLUMN *HCTab     = nil,
      INT           selected   = -1,
      BOOL          canResize  = false,
      BOOL          chgSortDir = false
   );
   virtual void HandleUpdate (CRect updateRect);
   virtual BOOL HandleMouseDown (CPoint pt, INT modifiers, BOOL doubleClick);
   virtual void HandleActivate (BOOL wasActivated);

   virtual void HandleSelect (INT i);         // Should be overridden
   virtual void HandleSortDir (BOOL ascend);  // Should be overridden
   virtual void HandleColumnResize (INT i);   // Should be overridden
   virtual void HandleResize (void);          // Should be overridden

   INT  Selected (void);
   BOOL Ascending (void);

   void DrawCell (INT i, BOOL pushed = false);
   void DrawSortDir (BOOL pushed = false);

   void SelectCell (INT i);
   void SetSortDir (BOOL ascend);

   void SetCellText (INT i, CHAR *text);
   void SetCellWidth (INT i, INT width);

private:
   INT           columns;
   HEADER_COLUMN *HCTab;
   BOOL          blackFrame;

   INT           selected;  // Selected column (-1 if none)
   BOOL          canResize;
   BOOL          changeSortDir;
   BOOL          ascendDir;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

INT DataHeaderView_Height (BOOL hasBlackFrame);
