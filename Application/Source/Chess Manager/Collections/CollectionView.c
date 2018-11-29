/**************************************************************************************************/
/*                                                                                                */
/* Module  : CollectionView.c                                                                     */
/* Purpose : This module implements the game collection sorting.                                  */
/*                                                                                                */
/**************************************************************************************************/

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

#include "Collection.h"

#include "CDialog.h"
#include "CMemory.h"

// The collection view controls how a collection is viewed, i.e. which games are shown (filter)
// and how these games are ordered. The user can change the sort direction (ascend/descend) and
// which field to sort by via the collection list view. Additionally, the collection filter
// defines which games to actually include in this list. When a new collection is opened, all
// games are shown, sorted by ascending gameNo.

// As games are added/updated/deleted the view needs to update too (incrementally):
//
// � ADDING GAMES
// Adding games can either be done manually by explicitly adding the frontmost game window, or
// multiple games can be added via PGN Import (or by pasting from the clipboard).

// � DELETEING GAMES
// The user can delete a contiguous block of games by selecting these in the collection list.


/**************************************************************************************************/
/*                                                                                                */
/*                                    HIGH LEVEL SORTING                                          */
/*                                                                                                */
/**************************************************************************************************/

BOOL SigmaCollection::Sort (INDEX_FIELD f)
{
   inxField = f; 
   return SortView();
} /* SigmaCollection::Sort */


BOOL SigmaCollection::SetSortDir (BOOL ascend)
{
   if (ascendDir == ascend) return true;
   ascendDir = ascend;

   if (! ViewMap)
      return Sort(inxField);
   else
      for (ULONG i = 0; i < viewCount/2; i++)
      {
         ULONG tmp = ViewMap[i];
         ViewMap[i] = ViewMap[viewCount - 1 - i];
         ViewMap[viewCount - 1 - i] = tmp;
      }

   return true;
} /* SigmaCollection::SetSortDir */


// The "SortView" method is a high level routine that sorts the current view "ViewMap[]" with respect
// to the current sort field "inxField" and sort direction "ascendDir".

BOOL SigmaCollection::SortView (void)
{
   BOOL sortOK = true;
   
   if (viewCount <= 1) return true;

   if (inxField != inxField_GameNo || viewCount < Info.gameCount)
      sortOK = SortGameList(ViewMap, viewCount);
   else if (ascendDir)
      for (ULONG g = 0; g < Info.gameCount; g++) ViewMap[g] = g;
   else
      for (ULONG g = 0; g < Info.gameCount; g++) ViewMap[g] = Info.gameCount - 1 - g;

   return sortOK;
} /* SigmaCollection::SortView */


/**************************************************************************************************/
/*                                                                                                */
/*                                    LOW LEVEL SORTING                                           */
/*                                                                                                */
/**************************************************************************************************/

// The "SortGameList" method sorts the specified set of game numbers according to the current sort
// sort field and sort direction.
//
// For the long fields (> 4 bytes) a hashed key cache is created to speed up the sorting process.
// � White      (40 bytes)
// � Black      (40 bytes)
// � Event/Site (40 bytes)
// � Date       (10 bytes)
// � Round      (10 bytes)
// � ECO        (10 bytes)
// � Annotator  (40 bytes)
//
// For the short fields (<= 4 bytes) a direct key cache is created, which is prefilled before
// the actual sort process starts.
// � Result     (4 bytes)
// � WhiteELO   (4 bytes)
// � BlackELO   (4 bytes)
//
// If there is enough memory we allocate a direct/complete key cache. Otherwise we will have to
// settle with a hashed/incomplete key cache.
//
// The actual table of game numbers, G[], is not sorted directly. Rather we introduce two
// temporary tables A[] and B[], whose entry values are indexes in G, rather than game numbers.
// The reason for this is that we can then index the key cache with indexes in G, i.e.
//
//    KeyCache[i] = CalcKey(G[i])

#define minNProgress 500

BOOL SigmaCollection::SortGameList (ULONG G[], ULONG N)
{
   if (N <= 1) return true;

   // First allocate the two temporary sort buffers:
   ULONG *A = (ULONG*)Mem_AllocPtr(N*sizeof(ULONG));
   ULONG *B = (ULONG*)Mem_AllocPtr(N*sizeof(ULONG));
   BOOL sortOK = false;

   if (A && B)
   {
      if (N >= minNProgress) BeginProgress("Sorting...", "Sorting...", N);

      if (CreateKeyCache(G,N))  // Returns false if mem error or user cancel
      {
         // Initialize sort buffer A:
         for (ULONG i = 0; i < N; i++) A[i] = i;

         // Perform merge sort:
         MergeSort(G, A, B, 0, N - 1);     // Sort.

         // Copy result back to G[]:
         for (ULONG i = 0; i < N; i++) B[i] = G[A[i]];
         for (ULONG i = 0; i < N; i++) G[i] = B[i];

         if (N >= minNProgress) SetProgress(N, "");
         sortOK = true;
      }

      if (N >= minNProgress) EndProgress();
   }

   BOOL sortMemErr = ! (A && B && DKeyCache);

   // Finally release temporary buffers:
   FreeKeyCache();
   if (B) Mem_FreePtr(B);
   if (A) Mem_FreePtr(A);

   return (sortMemErr ? sigmaApp->MemErrorDialog() : sortOK);
} /* SigmaCollection::SortGameList */

/*---------------------------------------- Compare Keys ------------------------------------------*/
// To improve cache hit rate, the merge sort routine is recursive so that "close" entries are
// completed first.

void SigmaCollection::MergeSort (ULONG G[], ULONG A[], ULONG B[], ULONG min, ULONG max)
{
   if (min >= max) return;

   //--- Compute midpoint and sort the two "halves" recursively ---
   ULONG mid = (max + min)/2;

   MergeSort(G, A, B, min,     mid);
   MergeSort(G, A, B, mid + 1, max);

   //--- Then merge the two "halves" ---
   ULONG j  = min;      // Target index
   ULONG i1 = min;      // Source index (left part)
   ULONG i2 = mid + 1;  // Source index (right part)

   while (i1 <= mid && i2 <= max)
      if (CompareKey(G, A[i1], A[i2]))
         B[j++] = A[i1++];
      else
         B[j++] = A[i2++];

   while (i1 <= mid) B[j++] = A[i1++];
   while (i2 <= max) B[j++] = A[i2++];

   //--- Finally copy back the sorted parts ---
   for (ULONG i = min; i <= max; i++)
      A[i] = B[i];
} /* SigmaCollection::MergeSort */

/*---------------------------------------- Key Cache ---------------------------------------------*/
// If there is enough memory we will create a direct/complete key cache. Otherwise we have to
// settle for hashed/incomplete key cache (which is slower but uses less memory ## NOT YET IMPLEMENTED).

static INT  CalcKeySize (INDEX_FIELD field);

BOOL SigmaCollection::CreateKeyCache (ULONG G[], ULONG N)
{
   DKeyCache = nil;
// HKeyCache = nil;

   // First calc key cache record size:
   keyRecSize = CalcKeySize(inxField);

   // Create direct/complete key cache:
   if (DKeyCache = (CHAR*)Mem_AllocPtr(keyRecSize*N))
   {
      hashedKey = false;

      for (ULONG i = 0; i < N; i++)
      {
         RetrieveGameKey(G[i], &DKeyCache[i*keyRecSize]);

         if (N >= minNProgress && i % 500 == 0)
         {  SetProgress(i, "");
            if (ProgressAborted()) return false;
         }
      }
   }
   else
   {
      hashedKey = true;

      keyRecSize += sizeof(ULONG);   // Make room for gameNo at beginning of entry.
      return false;  //###
   }

   return true;
} /* SigmaCollection::CreateKeyCache */


void SigmaCollection::FreeKeyCache (void)
{
   if (DKeyCache) Mem_FreePtr(DKeyCache);
// if (HKeyCache) Mem_FreePtr(HKeyCache);
   DKeyCache = nil;
// HKeyCache = nil;
} /* SigmaCollection::FreeKeyCache */


static INT CalcKeySize (INDEX_FIELD field)
{
   switch (field)
   {
      case inxField_WhiteName :
      case inxField_BlackName :
      case inxField_EventSite : return nameStrLen + 2;
      case inxField_Date      : return dateStrLen + 2;
      case inxField_Round     : return roundStrLen + 2;
      case inxField_Result    : return 2;
      case inxField_ECO       : return ecoStrLen + 2;
      default                 : return 4;
   }
} /* CalcKeySize */


void SigmaCollection::RetrieveGameKey (ULONG g, CHAR *key)
{
   GetGameInfo(g);
   GAMEINFO *Info = &game->Info;

   switch (inxField)
   {
      case inxField_WhiteName : CopyStr(Info->whiteName, key); break;
      case inxField_BlackName : CopyStr(Info->blackName, key); break;
      case inxField_EventSite :
         if (! Info->event[0]) CopyStr(Info->site, key);
         else if (! Info->site[0]) CopyStr(Info->event, key);
         else
         {  INT n = 0;
            CHAR *s = Info->event;
            while (*s) key[n++] = *(s++);
            key[n++] = '/';
            s = Info->site;
            while (*s && n < nameStrLen) key[n++] = *(s++);
            key[n] = 0;
         }
         break;
      case inxField_Date      : CopyStr(Info->date, key); break;
      case inxField_Round     : CopyStr(Info->round, key); break;
      case inxField_Result    : key[0] = Info->result + '0'; key[1] = 0; break;
      case inxField_ECO       : CopyStr(Info->ECO, key); break;
      default                 : key[0] = 0;
   }

   // Finally convert to upper:
   for (CHAR *s = key; *s; s++)
      if (*s >= 'a' && *s <= 'z') *s += 'A' - 'a';
} /* SigmaCollection::RetrieveGameKey */

/*---------------------------------------- Compare Keys ------------------------------------------*/
// The "CompareKey" routine returns true if game g1 sorts before game g2.

BOOL SigmaCollection::CompareKey (ULONG G[], ULONG i1, ULONG i2)
{
   CHAR *key1, *key2;

   if (! hashedKey)
   {
      key1 = &DKeyCache[keyRecSize*i1];
      key2 = &DKeyCache[keyRecSize*i2];
   }
   else
   {
      return G[i1] <= G[i2]; //###
/*
      HKEY_CACHE *K1 = (HKEY_CACHE *)((PTR)HKeyCache) + keyRecSize*(i1 % keyCacheMask);
      HKEY_CACHE *K2 = (HKEY_CACHE *)((PTR)HKeyCache) + keyRecSize*(i1 % keyCacheMask);

      // Retrieve first key:
      if (K1->g == G[i1])
         key1 = K1->key;
      else
      {  GetGameInfo(G[i1]);
         key1 = SelectKeyStr();
         if (K1->g != G[i2])    // Don't overwrite cache entry if it contains i2!
         {  K1->g = G[i1];
            CopyStr(key1, K1->key);
         }
      }

      // Retrieve second key:
      if (K2->g == G[i2])
         key2 = K2->key;
      else
      {  GetGameInfo(G[i2]);
         key2 = SelectKeyStr();
         K2->g = G[i2];
         CopyStr(key2, K2->key);
      }
*/
   }

   // Compare the two keys:
   do
   {  if (*key1 < *key2) return ascendDir;
      if (*key1 > *key2) return ! ascendDir;
      if (!*key1) return ((G[i1] <= G[i2]) == ascendDir);
      key1++; key2++;
   } while (true);
} /* SigmaCollection::CompareKey */


/**************************************************************************************************/
/*                                                                                                */
/*                                          RESET VIEW                                            */
/*                                                                                                */
/**************************************************************************************************/

void SigmaCollection::View_Reset (void)   // Simply copies game map into view map
{
   for (ULONG g = 0; g < Info.gameCount; g++)
      ViewMap[g] = g;
   viewCount = Info.gameCount;
} /* SigmaCollection::View_Reset */


/**************************************************************************************************/
/*                                                                                                */
/*                                         ADDING GAMES                                           */
/*                                                                                                */
/**************************************************************************************************/

// New games are always appended to the end of the game map, for instance when importing a PGN file.
// Once this has been done, we need to update the current view by calling the "View_Add" method.

BOOL SigmaCollection::View_Add (ULONG gfirst, ULONG glast)
{
   if (gfirst > glast) return false;

   // First resize ViewMap:
   ULONG oldViewCount = viewCount; 
   ULONG *NewViewMap = (ULONG*)Mem_AllocPtr(Info.gameCount*sizeof(ULONG));
   
   if (! NewViewMap)
      return sigmaApp->MemErrorDialog();

   for (ULONG i = 0; i < viewCount; i++)
      NewViewMap[i] = ViewMap[i];
   Mem_FreePtr(ViewMap);
   ViewMap = NewViewMap;

   // Next add each new game (that passes the filter) to the view, and count the number of
   // games being added.
   ULONG count = 0;
   for (ULONG g = gfirst; g <= glast; g++)
      if (FilterGame(g))
         ViewMap[viewCount++] = g,
         count++;

   if (count == 0) return false;

   // If only a few games are being added (< 10 %), it's faster to insert these directly one
   // by one than to perform a full merge, as this will needed to scan through all the existing
   // keys.

   SortView();
/*
   if (viewCount < 100 || count < viewCount/10)
   {
      CHAR key1[maxGameKeyLen], key2[maxGameKeyLen];
   }
   else
   {
      // Then sort this newly appended part of the view:
      if (inxField != inxField_GameNo)
         SortGameList(&ViewMap[oldViewCount], viewCount - oldViewCount);

      // Finally merge the old and the new part together:
   }
*/
   return true;
} /* SigmaCollection::View_Add */


/**************************************************************************************************/
/*                                                                                                */
/*                                        DELETING GAMES                                          */
/*                                                                                                */
/**************************************************************************************************/

// The "View_Delete" method deletes all games in the specified subpart of the view. 
// NOTE: As this routine renumbers the games, it may NOT be called while any games are open from
// this collection.

#define minNDelProgress 10000L

void SigmaCollection::View_Delete (ULONG first, ULONG last)
{
   if (! ViewMap) return;
   
   BOOL progress = false;

   if (first == 0 && last == Info.gameCount -1)   // Special case if all games deleted
   {
      Info.gameCount = viewCount = 0;
   }
   else
   {
      ULONG count = last + 1 - first;
/*
		DOESN'T WORK IF A FILTER IS ACTIVE (2010-11-07)
      if (inxField == inxField_GameNo)
      {
         DelGames(ViewMap[first], count, false);
         View_Reset();
      }
      else
*/
      {
         //--- Allocate temporary remap buffer ---
         LONG *R = (LONG*)Mem_AllocPtr(Info.gameCount*sizeof(ULONG));
         if (! R)
         {  sigmaApp->MemErrorDialog();
            return;
         }
         progress = (Info.gameCount >= minNDelProgress);
         if (progress) BeginProgress("Deleting...", "Deleting...", 7);

	      //--- Mark for deletion in game map ---
	      // by setting Map[g].pos = 0
         if (progress) SetProgress(0, "");
	      for (ULONG i = first; i <= last; i++)
	      {
	         ULONG g = ViewMap[i];                     // GameNo of game to be deleted.
//          GetGameInfo(g);                           // Update result statistics
//          Info.resultCount[game->Info.result]--;
	         Map[g].pos = 0; // Mark for deletion
	      }

	      //--- Physically delete and shift ViewMap entries ---
         if (progress) SetProgress(1, "");
	      for (ULONG i = first + count; i < viewCount; i++)
	         ViewMap[i - count] = ViewMap[i];          // Remove game from view map
	      viewCount -= count;

         //--- Remap the ViewMap entries ---
         if (progress) SetProgress(2, "");
         for (ULONG n = 0; n < Info.gameCount; n++)   // Clear remap buffer
            R[n] = false;

         if (progress) SetProgress(3, "");
         for (ULONG i = 0; i < viewCount; i++)        // Perform bucket sort
            R[ViewMap[i]] = true;

         if (progress) SetProgress(4, "");
         for (ULONG n = 0, j = 0; n < Info.gameCount; n++) // Remap
            if (R[n]) R[n] = j++;

         if (progress) SetProgress(5, "");
         for (ULONG i = 0; i < viewCount; i++)        // Copy back to ViewMap
            ViewMap[i] = R[ViewMap[i]];

	      //--- Release temporary remap buffer ---
	      Mem_FreePtr(R);

         //--- Finally delete the marked games ---
         if (progress) SetProgress(6, "");
	      DelMarkedGames(false);
         if (progress) SetProgress(7, "");
      }
   }

   // Finally write map.
   WriteInfo();
   WriteMap();
   if (progress) EndProgress();
} /* SigmaCollection::View_Delete */


/**************************************************************************************************/
/*                                                                                                */
/*                                        UPDATING GAMES                                          */
/*                                                                                                */
/**************************************************************************************************/

// When a collection game is saved and the game info was changed, it may be necessary to reorder
// the game in the view list. We may even need to add/remove the game to/from the view list if
// the filter changes. The return value indicates if the "ViewMap" was changed, and thus if the
// view list should be redrawn.

BOOL SigmaCollection::View_UpdateGame (ULONG g)
{
   ULONG oldInx;       // Old view index of game (= viewCount if not previously in view)
   ULONG newInx;       // New view index of game

   // First locate the game in the view (unless it was not filtered):
   for (oldInx = 0; oldInx < viewCount && ViewMap[oldInx] != g; oldInx++);

   BOOL wasInView = (oldInx < viewCount);
   BOOL isInView  = FilterGame(g);

   if (! wasInView && ! isInView) return false;
   
   if (wasInView)
   {
      for (ULONG i = oldInx; i < viewCount - 1; i++)
         ViewMap[i] = ViewMap[i + 1];
      viewCount--;
   }

   if (isInView)
   {
      newInx = View_CalcPos(g);
      for (ULONG i = viewCount; i > newInx; i--)
         ViewMap[i] = ViewMap[i - 1];
      ViewMap[newInx] = g;
      viewCount++;
   }

   return (wasInView != isInView || oldInx != newInx);
} /* SigmaCollection::View_UpdateGame */


ULONG SigmaCollection::View_CalcPos (ULONG g)
{
   if (viewCount < 1) return 0;
// if (viewCount <= 1) return 0;

   CHAR key[maxGameKeyLen + 1];
   RetrieveGameKey(g, key);

   ULONG i1 = 0, i2 = viewCount;
   do
   {
      ULONG i = (i1 + i2)/2;

      CHAR  tkey[maxGameKeyLen + 1];
      RetrieveGameKey(ViewMap[i], tkey);

      INT diff = CompareStr(key, tkey, false);
      if (diff == 0) diff = g - ViewMap[i];
      if (! ascendDir) diff = -diff;

      if (diff < 0) i2 = i;
      else if (diff > 0) i1 = i + 1;
      else return i;

   } while (i1 < i2);

   return i1;
} /* SigmaCollection::View_CalcPos */


/**************************************************************************************************/
/*                                                                                                */
/*                                            SEARCH                                              */
/*                                                                                                */
/**************************************************************************************************/

// The "SearchGame" method searches for the game with the specified key (current inxField). It
// returns the "ViewMap[]" index of the closest matching game. This routine should not be called
// if sorting by gameNo. The search is done using binary search.

ULONG SigmaCollection::View_Search (CHAR *key)
{
   if (viewCount <= 1) return 0;

   ULONG i1 = 0, i2 = viewCount - 1;

   do
   {
      ULONG i = (i1 + i2)/2;

      CHAR tkey[maxGameKeyLen + 1];
      RetrieveGameKey(ViewMap[i], tkey);

      INT diff = CompareStr(key, tkey, false);
      if (! ascendDir) diff = -diff;
      if (diff < 0) i2 = i;
      else if (diff > 0) i1 = i + 1;
      else return i;

   } while (i1 < i2);

   return i1;
} /* SigmaCollection::View_Search */


/**************************************************************************************************/
/*                                                                                                */
/*                                          APPLY FILTER                                          */
/*                                                                                                */
/**************************************************************************************************/

// When the filter has been changed or turned on/off we have to rebuild the view

void SigmaCollection::View_Rebuild (void)
{
   if (! useFilter)
   {
      View_Reset();
      SortView();
   }
   else
   {
      BOOL error = false;

      BeginProgress("Filtering...", "Filtering...", Info.gameCount);

         viewCount = 0;
         for (ULONG g = 0; g < Info.gameCount && ! error; g++)
         {
            if (FilterGame(g)) ViewMap[viewCount++] = g;
            if (g % 100 == 0)
            {  SetProgress(g, "");
               if (ProgressAborted()) error = true;
            }
         }

      EndProgress();

      if (error)
      {  useFilter = false;
         View_Reset();
      }

      SortView();
   }
} /* SigmaCollection::View_Rebuild */
