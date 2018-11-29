/**************************************************************************************************/
/*                                                                                                */
/* Module  : CVIEWOWNER.C */
/* Purpose : Implements a base class for windows, views and bitmaps. */
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

#include "CViewOwner.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                          CONSTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CViewOwner::CViewOwner(VIEW_OWNER_TYPE type) {
  viewOwnerType = type;
  vParent = nil;
  vFirstChild = nil;
  vLastChild = nil;
  vPrevSibling = nil;
  vNextSibling = nil;
} /* CViewOwner::CViewOwner */

CViewOwner::~CViewOwner(void) {
  // while (vFirstChild) UnregisterChild(vFirstChild);
} /* CViewOwner::~CViewOwner */

void CViewOwner::RegisterChild(CViewOwner *child) {
  child->vParent = this;

  child->vPrevSibling = vLastChild;
  child->vNextSibling = nil;
  if (vLastChild)
    vLastChild->vNextSibling = child;
  else
    vFirstChild = child;
  vLastChild = child;
} /* CViewOwner::RegisterChild */

void CViewOwner::UnregisterChild(CViewOwner *child) {
  child->vParent = nil;

  if (child->vPrevSibling)
    child->vPrevSibling->vNextSibling = child->vNextSibling;
  else
    vFirstChild = child->vNextSibling;

  if (child->vNextSibling)
    child->vNextSibling->vPrevSibling = child->vPrevSibling;
  else
    vLastChild = child->vPrevSibling;

  child->vPrevSibling = nil;
  child->vNextSibling = nil;
} /* CViewOwner::UnregisterChild */
