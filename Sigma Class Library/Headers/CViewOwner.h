/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include "CUtility.h"


#define forAllChildViews(V) for (CView *V = (CView*)vFirstChild; V; V = (CView*)(V->vNextSibling))

enum VIEW_OWNER_TYPE
{
   viewOwner_Window = 1,
   viewOwner_Bitmap = 2,
   viewOwner_View   = 3,
   viewOwner_Print  = 4
};

class CViewOwner
{
public:
   CViewOwner (VIEW_OWNER_TYPE type);
   virtual ~CViewOwner (void);

   void RegisterChild (CViewOwner *child);
   void UnregisterChild (CViewOwner *child);

   VIEW_OWNER_TYPE viewOwnerType;

   CViewOwner *vParent;        // Nil if no parent (yet) -> call RegisterChild.
   CViewOwner *vFirstChild;    // Points to first (oldest child). Nil if no children.
   CViewOwner *vLastChild;     // Points to last (youngest child). Nil if no children.
   CViewOwner *vPrevSibling;   // Nil for first (oldest) child
   CViewOwner *vNextSibling;   // Nil for last (youngest) child
};
