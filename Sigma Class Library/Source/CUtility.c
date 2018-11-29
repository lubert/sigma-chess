/**************************************************************************************************/
/*                                                                                                */
/* Module  : CUTILITY.C                                                                           */
/* Purpose : Implements various general purpose classes and functions.                            */
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

#include "CUtility.h"
#include "CMemory.h"
#include "General.h"
#include "CDialog.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                          THE CPOINT CLASS                                      */
/*                                                                                                */
/**************************************************************************************************/

CPoint::CPoint (INT newh, INT newv)
{
   h = newh;
   v = newv;
} /* CPoint::CPoint */


CPoint::CPoint (void)
{
   h = 0;
   v = 0;
} /* CPoint::CPoint */

/*------------------------------------------- Adjusting ------------------------------------------*/

void CPoint::Set (INT newh, INT newv)
{
   h = newh;
   v = newv;
} /* CPoint::Set */


void CPoint::Offset (INT dh, INT dv)
{
   h += dh;
   v += dv;
} /* CPoint::Offset */


/*------------------------------------------ Miscellaneous ---------------------------------------*/

BOOL CPoint::InRect (CRect r)
{
   return (h >= r.left && h <= r.right && v >= r.top && v <= r.bottom);
} /* CPoint::InRect */


BOOL CPoint::Equal (CPoint p)
{
   return (h == p.h && v == p.v);
} /* CPoint::Equal */


/**************************************************************************************************/
/*                                                                                                */
/*                                          THE CRECT CLASS                                       */
/*                                                                                                */
/**************************************************************************************************/

CRect::CRect (INT rleft, INT rtop, INT rright, INT rbottom)
{
   left   = rleft;
   top    = rtop;
   right  = rright;
   bottom = rbottom;
} /* CRect::CRect */


CRect::CRect (CRect *r)
{
   left   = r->left;
   top    = r->top;
   right  = r->right;
   bottom = r->bottom;
} /* CRect::CRect */


CRect::CRect (void)
{
   left   = 0;
   top    = 0;
   right  = 0;
   bottom = 0;
} /* CRect::CRect */


/*
CRect::~CRect (void)
{
} /* CRect::~CRect*/

/*------------------------------------------- Adjusting ------------------------------------------*/

void CRect::Set (INT rleft, INT rtop, INT rright, INT rbottom)
{
   left   = rleft;
   top    = rtop;
   right  = rright;
   bottom = rbottom;
} /* CRect::Set */


void CRect::Offset (INT dh, INT dv)
{
   left   += dh;
   top    += dv;
   right  += dh;
   bottom += dv;
} /* CRect::Offset */


void CRect::Inset (INT dh, INT dv)
{
   left   += dh;
   top    += dv;
   right  -= dh;
   bottom -= dv;
} /* CRect::Inset */


void CRect::Normalize (void)
{
   right -= left; left = 0;
   bottom -= top; top = 0;
} /* CRect::Normalize */


BOOL CRect::Intersect (CRect *r1, CRect *r2)
{
   left   = Max(r1->left,   r2->left);
   top    = Max(r1->top,    r2->top);
   right  = Min(r1->right,  r2->right);
   bottom = Min(r1->bottom, r2->bottom);
   return (left < right && top < bottom);
} /* CRect::Intersect */


void CRect::Union (CRect *r1, CRect *r2)
{
   left   = Min(r1->left,   r2->left);
   top    = Min(r1->top,    r2->top);
   right  = Max(r1->right,  r2->right);
   bottom = Max(r1->bottom, r2->bottom);
} /* CRect::Union */

/*---------------------------------------------- Misc --------------------------------------------*/

INT CRect::Width (void)
{
   return right - left;
} /* CRect::Width */


INT CRect::Height (void)
{
   return bottom - top;
} /* CRect::Height */

/*--------------------------------------------- Private ------------------------------------------*/

void CRect::SetMacRect (Rect *r)
{
    r->left   = left;
    r->top    = top;
    r->right  = right;
    r->bottom = bottom;
} /* CRect::SetMacRect */


/**************************************************************************************************/
/*                                                                                                */
/*                                          THE CLIST CLASS                                       */
/*                                                                                                */
/**************************************************************************************************/

CList::CList (void)
{
   count = 0;
   first = last = current = NULL;
} /* CList::CList */


CList::~CList (void)
{
   while (Front());
} /* CList::CList */


INT CList::Count (void)
{
    return count;
} /* CList::Count */


void CList::Append (void *data)
{
   current = (CLISTELEM *)NewPtr(sizeof(CLISTELEM));
   current->data = data;
   current->next = NULL;
   current->prev = NULL;

   if (count == 0)
      first = last = current;
   else
   {  last->next = current;
      current->prev = last;
      last = current;
   }
   count++;
} /* CList::Append */


void *CList::Front (void)
{
   void *data = NULL;

   if (first)
   {  data = first->data;
      Remove(first->data);
   }
   return data;
} /* CList::Front */


void CList::Remove (void *data)
{
   for (current = first; current; current = current->next)
      if (current->data == data)
      {
         if (current != first) current->prev->next = current->next;
         else first = current->next;
         if (current != last ) current->next->prev = current->prev;
         else last = current->prev;
         DisposePtr((Ptr)current);
         current = NULL;
         count--;
         return;
      }
} /* CList::Remove */


void CList::Scan (void)
{
   current = first;
} /* CList::Scan */


void *CList::Next (void)
{
   void *data = NULL;

   if (current)
   {  data = current->data;
      current = current->next;
   }
   return data;
} /* CList::Next */


BOOL CList::Find (void *data)
{
   for (current = first; current; current = current->next)
      if (current->data == data)
         return true;
   return false;
} /* CList::Find */


void *CList::NextCyclic (void)
{
   if (! current) return NULL;
   current = current->next;
   if (! current) current = first;
   return current->data;
} /* CList::NextCyclic */


void *CList::PrevCyclic (void)
{
   if (! current) return NULL;
   current = current->prev;
   if (! current) current = last;
   return current->data;
} /* CList::PrevCyclic */


void *CList::Current (void)
{
   return (current ? current->data : NULL);
} /* CList::Current */


/**************************************************************************************************/
/*                                                                                                */
/*                                        THE CPICTURE CLASS                                      */
/*                                                                                                */
/**************************************************************************************************/

/*
CPicture::CPicture (INT resID)
{
} /* CPicture::CPicture */

/*
CPicture::CPicture (void)
{
   hor = 0;
   ver = 0;
} /* CPicture::CPicture */

/*
CPoint::~CPoint (void)
{
} /* CPoint::~CPoint*/



/**************************************************************************************************/
/*                                                                                                */
/*                                          MISCELLANEOUS                                         */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------ Check OS Err Result Codes ---------------------------------*/

BOOL OSError (INT resultCode, BOOL showErrorDialog)
{
   if (resultCode == noErr) return false;

   if (! showErrorDialog) return true;

   CHAR *s;
   switch (resultCode)
   {
      case dirFulErr  : s = "The directory is full...";       break;
      case dskFulErr  : s = "The disk is full...";            break;
      case mFulErr    : s = "The System heap is full...";     break;
      case wPrErr     : s = "The disk is write-protected..."; break;
      case fLckdErr   : s = "The file is locked...";          break;
      case vLckdErr   : s = "The volume is locked...";        break;
      case memFullErr : s = "Out of memory...";               break;
      case permErr    : s = "The file is locked...";          break;
      default         : s = "<No description>";
   }

   CHAR msg[200];
   Format(msg, "Error (%d): %s", resultCode,s);
   NoteDialog(nil, "OS Error", msg, cdialogIcon_Error);

   return true;
} /* OSError */


void Beep (INT n)
{
   while (n-- > 0) SysBeep(30);
} /* Beep */


void Sleep (LONG ticks)
{
   ULONG n;

   Delay(ticks, &n);
} /* Sleep */


void SetRGBColor100 (RGB_COLOR *c, INT red, INT green, INT blue)
{
   c->red   = (65535L*red)/100;
   c->green = (65535L*green)/100;
   c->blue  = (65535L*blue)/100;
} /* SetRGBColor100 */

/*
void GetRGBColor100 (INT *red, INT *green, INT *blue, RGB_COLOR *c)
{
   *red   = (100*c->red)/65535L;
   *green = (100*c->green)/65535L;
   *blue  = (100*c->blue)/65535L;
} /* GetRGBColor100 */


void AdjustRGBHue (RGB_COLOR *c, INT deltaPct)
{
   c->red   = MinL(65535L, ((ULONG)c->red/100)*(100 + deltaPct));
   c->green = MinL(65535L, ((ULONG)c->green/100)*(100 + deltaPct));
   c->blue  = MinL(65535L, ((ULONG)c->blue/100)*(100 + deltaPct));
} /* AdjustRGBHue */


CHAR *LoadStr (INT groupID, INT index)
{
   Str255 ps;
   ::GetIndString(ps, groupID, index);
   INT len = ps[0] & 0x00FF;

   CHAR *cs = (CHAR*)Mem_AllocPtr(len + 1);
   if (cs)
   {  for (INT i = 0; i < len; i++) cs[i] = ps[i + 1];
      cs[len] = 0;
   }
   return cs;
} /* LoadStr */


CHAR *LoadText (INT id)
{
   Handle H = (Handle)::GetResource('TEXT', id);
   HLock(H);

   LONG n = ::GetResourceSizeOnDisk(H);
   CHAR *s = (CHAR*)Mem_AllocPtr(n + 1);
   if (s)
   {  for (LONG i = 0; i < n; i++) s[i] = ((CHAR*)(*H))[i];
      s[n] = 0;
   }

   HUnlock(H);
   ReleaseResource(H);

   return s;
} /* LoadText */

/*-------------------------------------- Help Tips/Balloons --------------------------------------*/

void ShowHelpTip (CHAR *text)
{
#ifndef TARGET_API_MAC_CARBON
//###TODO : How does this work in Carbon???
   if (text[0] == 0) return;  // Ignore if empty string.

   HMMessageRecord hm;
   Point mp;
   BOOL wasEnabled = ::HMGetBalloons();

   if (! wasEnabled) ::HMSetBalloons(true);
      hm.hmmHelpType = khmmString;
      ::C2P_Str(text, hm.u.hmmString);
      ::GetMouse(&mp); LocalToGlobal(&mp);
      ::HMShowBalloon(&hm, mp, nil,nil,0,0,kHMRegularWindow);
      while (Button());
      ::HMRemoveBalloon();
   if (! wasEnabled) ::HMSetBalloons(false);
#endif
} /* ShowHelpTip */
