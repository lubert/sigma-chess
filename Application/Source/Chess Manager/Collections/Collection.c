/**************************************************************************************************/
/*                                                                                                */
/* Module  : Collection.c                                                                         */
/* Purpose : This module implements the game collections.                                         */
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

#include "Collection.h"
#include "CMemory.h"

/*
OPTIMIZED DATABASE GAME FORMAT:

Automatically build indexes on White/Black/Event/Site and only store the index in the game data.
This way we won't repeat the same information over and over again, thus saving space. Additionally,
searching should be faster. E.g. the player name "Kasparov, Gary" takes up 16 bytes in a normal file,
but will be reduced to 2 (or 4) bytes using the auto-indexing.

typedef struct
{
   INT type/Field;   // E.g. "White Name" in the game info
   LONG count;       // Number of entries
   STR63 Data[];
} STRING_INDEX;

When games are removed, some entries may become obsolete. These can later be removed when
rebuilding the indexes.

*/

// In the Lite version:
// (1) The user cannot create collections with more than 1000 games. If he tries to add a game
//     (manually or via PGN paste/import) to a collection with 1000 games, an error dialog is
//     opened.
// (2) The user can open an existing collection with more than 1000 games, but it will be 
//     truncated to 1000 games (CONFIRM first).

BYTE colGameData[64000];  // Global utility buffer used when e.g. packing/unpacking games.


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

SigmaCollection::SigmaCollection (CFile *theFile, CWindow *theWindow)
{
   game      = new CGame();
   file      = nil;
   Map       = nil;

   inxField  = inxField_GameNo;
   ascendDir = true;
   ViewMap   = nil;
   viewCount = 0;

   useFilter = false;
   ResetFilter();

   infoDirty = false;
   mapDirty  = false;
   colLocked = false;
   liteLimit = false;

   window    = theWindow;
   progressDlg = nil;

   if (! theFile->Exists())
   {
      if (FileErr(theFile->SetType('·GC5'))) return;
      if (FileErr(theFile->Create())) return;
      if (FileErr(theFile->Open(filePerm_RdWr))) return;
      file = new CFile(theFile);
      colLocked = false;
      ResetInfo();
      WriteInfo();
   }
   else
   {
      colLocked = theFile->IsLocked();
      if (FileErr(theFile->Open(colLocked ? filePerm_Rd : filePerm_RdWr))) return;
      file = new CFile(theFile);
      ReadInfo();
   }

   if (ReadMap() != colErr_NoErr)
   {
      NoteDialog(nil, "Failed Loading Game Map", "The collection game map could not be loaded. \
Try closing some windows or assigning more memory to Sigma Chess...", cdialogIcon_Error);
      return;
   }

   if (! ProVersion() && Info.gameCount > maxGamesLite)
   {
      Info.gameCount = viewCount = maxGamesLite;
      colLocked = true;
      liteLimit = true;
   }
} /* SigmaCollection::SigmaCollection */


SigmaCollection::~SigmaCollection (void)
{
   delete game;

   if (! file) return;

   // Flush collection info and map:
   if (infoDirty) WriteInfo();
   if (mapDirty) WriteMap();

   if (Map) Mem_FreePtr(Map);
   if (ViewMap) Mem_FreePtr(ViewMap);

   // Finally close the file and release file object:
   FileErr(file->Close());
   delete file;
} /* SigmaCollection::~SigmaCollection */

/*--------------------------------------- Error Handling -----------------------------------------*/
/*
void SigmaCollection::HandleError (
{
}
*/

/**************************************************************************************************/
/*                                                                                                */
/*                                     COLLECTION INFO BLOCK                                      */
/*                                                                                                */
/**************************************************************************************************/

// The Collection Info Block is located at the start of a collection, and contains various info
// about the actual games, indices etc. When a new collection is created the InfoBlock is first
// reset with some happy defaults.

/*------------------------------------ Reset/Clear Collection ------------------------------------*/

void SigmaCollection::ResetInfo (void)
{
   //--- User Defined Info ---
   Info.version     = collectionVersion;
   Info.title[0]    = 0;
   Info.author[0]   = 0;
   Info.descr[0]    = 0;
   Info.flags       = 0;

   //--- Game Count & Statistics ---
   Info.gameCount   = 0;
   Info.gameBytes   = 0;

   Info.resultCount[0]                   = 0;
   Info.resultCount[infoResult_Unknown]  = 0;
   Info.resultCount[infoResult_Draw]     = 0;
   Info.resultCount[infoResult_WhiteWin] = 0;
   Info.resultCount[infoResult_BlackWin] = 0;

   //--- File Pointers ---
   Info.fpMapStart  = sizeof(COLINFO);
   Info.fpMapEnd    = Info.fpMapStart + 10*sizeof(COLMAP);
   Info.fpGameStart = Info.fpMapEnd;
   Info.fpGameEnd   = Info.fpGameStart;

   Info.fileSize    = Info.fpGameEnd;
} /* SigmaCollection::ResetInfo */

/*----------------------------------- Read/Write Collection Info ---------------------------------*/

COLERR SigmaCollection::ReadInfo (void)
{
   ULONG bytes = sizeof(COLINFO);
   if (FileErr(file->SetPos(0))) return colErr_ReadInfoFail;
   if (FileErr(file->Read(&bytes, (PTR)&Info))) return colErr_ReadInfoFail;
   infoDirty = false;
   return colErr_NoErr;
} /* SigmaCollection::ReadInfo */


COLERR SigmaCollection::WriteInfo (void)   // Also updates file size (physical EOF)
{
   if (colLocked) return colErr_Locked;

   ULONG bytes = sizeof(COLINFO);
   Info.fileSize = Info.fpGameEnd;
   if (FileErr(file->SetPos(0))) return colErr_WriteInfoFail;
   if (FileErr(file->Write(&bytes, (PTR)&Info))) return colErr_WriteInfoFail;
   if (FileErr(file->SetSize(Info.fileSize))) return colErr_WriteInfoFail;
   infoDirty = false;
   return colErr_NoErr;
} /* SigmaCollection::WriteInfo */


BOOL SigmaCollection::IsLocked (void)
{
   return colLocked;
} /* SigmaCollection::IsLocked */

/*-------------------------------------------- Misc ----------------------------------------------*/

INT SigmaCollection::CalcScoreStat (void)
{
   ULONG total = 2*(Info.gameCount - Info.resultCount[infoResult_Unknown]);   // Total number of known results
   ULONG y = Info.resultCount[infoResult_Draw];
   ULONG x = Info.resultCount[infoResult_WhiteWin];
   ULONG score = 2*x + y;
// score = 2*Info.result[infoResult_WhiteWin] + Info.result[infoResult_Draw];
   if (total == 0) return 50; //### or -1 = unknown/none
   return (100L*score)/total;
} /* SigmaCollection::CalcScoreStat */


BOOL SigmaCollection::Publishing (void)
{
   return Info.flags & colInfoFlag_Publishing;
} /* SigmaCollection::Publishing */


/**************************************************************************************************/
/*                                                                                                */
/*                                     COLLECTION MAP BLOCK                                       */
/*                                                                                                */
/**************************************************************************************************/

// The Collection Map Block is located immediately after the Collection Info Block. The Map is
// simply an array of game map entries. Each entry contains two fields:
//
// ¥ pos  : The file position of the game in the Game Data Block.
// ¥ size : The size of the game.
//
// The "logical" size of the Map Block is the number of games times the entry size (8 bytes). The
// "physical" size of the Map Block is the actual number of bytes used in the file for storing the
// Map Block. Typically the physical size is greater than the logical size, so that new entries
// can be appended.

/*----------------------------------- Read/Write Collection Map ----------------------------------*/

COLERR SigmaCollection::ReadMap (void)
{
   // First deallocate if already loaded:
   if (Map)
   {  if (mapDirty) WriteMap();
      Mem_FreePtr((PTR)Map);
   }

   // Next allocate buffer:
   ULONG bytes = Info.fpMapEnd - Info.fpMapStart;
   Map = (COLMAP*)Mem_AllocPtr(bytes);
   if (! Map) return colErr_MemFull;

   if (! ViewMap)
   {  ViewMap = (ULONG*)Mem_AllocPtr(Info.gameCount*sizeof(ULONG));
      if (! ViewMap) 
      {  Mem_FreePtr(Map);
         return colErr_MemFull;
      }
      for (ULONG g = 0; g < Info.gameCount; g++) ViewMap[g] = g;
      viewCount = Info.gameCount;
   }

   // Finally read the game map from the collection file:
   if (FileErr(file->SetPos(Info.fpMapStart))) return colErr_ReadMapFail;
   if (FileErr(file->Read(&bytes, (PTR)Map))) return colErr_ReadMapFail;

   mapDirty = false;
   return colErr_NoErr;
} /* SigmaCollection::ReadMap */


COLERR SigmaCollection::WriteMap (ULONG gameNo, ULONG count)
{
   if (colLocked) return colErr_Locked;

   if (! Map) return colErr_WriteMapFail;
   ULONG bytes = sizeof(COLMAP)*(count > 0 ? count : Info.gameCount - gameNo);
   if (bytes <= 0) return colErr_NoErr;  //###NEW
   if (FileErr(file->SetPos(Info.fpMapStart + gameNo*sizeof(COLMAP)))) return colErr_WriteMapFail;
   if (FileErr(file->Write(&bytes, (PTR)(&Map[gameNo])))) return colErr_WriteMapFail;
   mapDirty = false;
   return colErr_NoErr;
} /* SigmaCollection::WriteMap */

/*----------------------------------- Increase Collection Map ------------------------------------*/
// Before adding new games to a collection, the game map has to be increased. If it can already
// accommodate the requested number of games, nothing happens. Otherwise we have to create
// extra room for the game map in the file by moving one or more games to the end of the file.

BOOL SigmaCollection::MapFull (ULONG count)
{
   ULONG newSize = (Info.gameCount + count)*sizeof(COLMAP); 
   return (newSize > Info.fpMapEnd - Info.fpMapStart);
} /* SigmaCollection::MapFull */


COLERR SigmaCollection::GrowMap (ULONG count)
{
   if (colLocked) return colErr_Locked;

   ULONG newSize = (Info.gameCount + count)*sizeof(COLMAP); 
   BOOL wasGrown = false;

   // If not room for new increased Map, then move some games:
   while (newSize > Info.fpMapEnd - Info.fpMapStart)
   {
      // Find first physical game g0:
      ULONG g0   = 0;
      ULONG pos0 = maxlong;
      for (ULONG g = 0; g < Info.gameCount; g++)
         if (Map[g].pos < pos0) pos0 = Map[g0 = g].pos; 

      // Move that game to end of file/game block:
      ULONG bytes = Map[g0].size;
      if (FileErr(file->SetPos(Map[g0].pos)))     return colErr_ReadGameFail;
      if (FileErr(file->Read(&bytes, colGameData)))  return colErr_ReadGameFail;
      if (FileErr(file->SetPos(Info.fpGameEnd)))  return colErr_ReadGameFail;
      if (FileErr(file->Write(&bytes, colGameData))) return colErr_ReadGameFail;

      // Update file block pointers and set file size:
      Info.fpMapEnd    =  Map[g0].pos + Map[g0].size;
      Map[g0].pos      =  Info.fpGameEnd;
      Info.fpGameStart += Map[g0].size;
      Info.fpGameEnd   += Map[g0].size;
      // Write the updated game map entry to disk (so we are always in sync):
      WriteMap(g0, 1);
      mapDirty = true;  // <-- Very important, as other parts of game map may still be dirty!!

      wasGrown = true;
   }

   // Finally flush info (in order to sync) and increase map buffer size:
   if (wasGrown)
   {
      WriteInfo();
//###      if (! MemSetSize((PTR)Map,newSize))
      return ReadMap(); //### Must be done in a better way
   }
 
   return colErr_NoErr;
} /* SigmaCollection::GrowMap */


/**************************************************************************************************/
/*                                                                                                */
/*                                       ACCESS GAMES & INFO                                      */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Hight level Routines ------------------------------------*/
// The high-level routines access the collection via the "ViewMap". If no view is present, the
// low-level routines are called instead.

ULONG SigmaCollection::View_GetGameNo (ULONG N)    // Return the absolute game number.
{
   return (ViewMap ? ViewMap[N] : N);
} /* SigmaCollection::View_GetGameNo */


ULONG SigmaCollection::View_GetGameCount (void)
{
   return (ViewMap ? viewCount : GetGameCount());
} /* SigmaCollection::View_GetGameCount */

/*
COLERR SigmaCollection::View_GetGame (ULONG N, PTR *gameData, ULONG *gameSize)
{
   return GetGame(View_GetGameNo(N), gameData, gameSize);
} /* SigmaCollection::View_GetGame */


COLERR SigmaCollection::View_GetGame (ULONG N, CGame *toGame)
{
   return GetGame(View_GetGameNo(N), toGame);
} /* SigmaCollection::View_GetGame */


COLERR SigmaCollection::View_GetGameInfo (ULONG N)
{
   return GetGameInfo(View_GetGameNo(N));
} /* SigmaCollection::View_GetGameInfo */

/*-------------------------------------- Low level Routines -------------------------------------*/
// The low-level routines specify "absolute" game numbers.

ULONG SigmaCollection::GetGameCount (void)
{
   return Info.gameCount;
} /* SigmaCollection::GetGameCount */


COLERR SigmaCollection::GetGame (ULONG gameNo, PTR data, ULONG *gameSize)
{
   ULONG size = Map[gameNo].size; 

   if (FileErr(file->SetPos(Map[gameNo].pos))) return colErr_ReadGameFail;
   if (FileErr(file->Read(&size,data))) return colErr_ReadGameFail;
   if (gameSize) *gameSize = size;
   return colErr_NoErr;
} /* SigmaCollection::GetGame */


COLERR SigmaCollection::GetGame (ULONG gameNo, CGame *toGame, BOOL raw)
{
   ULONG gameSize = Map[gameNo].size;

   if (FileErr(file->SetPos(Map[gameNo].pos))) return colErr_ReadGameFail;
   if (FileErr(file->Read(&gameSize,(PTR)gameData))) return colErr_ReadGameFail;
   if (toGame) toGame->Decompress(gameData,gameSize,raw);
   return colErr_NoErr;
} /* SigmaCollection::GetGame */


COLERR SigmaCollection::GetGameInfo (ULONG gameNo)
{
   BYTE Data[4096];
   ULONG bytes = MinL(4096,Map[gameNo].size);

   if (FileErr(file->SetPos(Map[gameNo].pos))) return colErr_ReadGameFail;
   if (FileErr(file->Read(&bytes, Data))) return colErr_ReadGameFail;
   game->DecompressInfo(Data);
   return colErr_NoErr;
} /* SigmaCollection::GetGameInfo */


/**************************************************************************************************/
/*                                                                                                */
/*                                     ADD/UPDATE/DELETE GAMES                                    */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Adding Games -----------------------------------------*/
// Games are added by first creating a new map entry. Next the game data is appended to the end of
// the Game Block (which is extended to accommodate for the new game data).

COLERR SigmaCollection::AddGame (ULONG gameNo, CGame *theGame, BOOL flush)
{
   if (colLocked) return colErr_Locked;

   ULONG gameSize = theGame->Compress(gameData);
   return AddGame(gameNo, gameData, gameSize, theGame->Info.result, flush);
} /* SigmaCollection::AddGame */


COLERR SigmaCollection::AddGame (ULONG gameNo, PTR data, ULONG gameSize, INT result, BOOL flush)
{
   if (colLocked) return colErr_Locked;

   ULONG bytes;

   //--- First check if room for new Game Map entry ---
   GrowMap(1);

   //--- Insert new game map entry ---

   Info.gameCount++;
   for (ULONG g = Info.gameCount - 1; g > gameNo; g--)
      Map[g] = Map[g - 1];
   Map[gameNo].pos  = Info.fpGameEnd;
   Map[gameNo].size = gameSize;
   mapDirty = true;

   if (flush) WriteMap(gameNo);

   //--- Append the new game data ---

   bytes = gameSize;
   if (FileErr(file->SetPos(Map[gameNo].pos))) return colErr_WriteGameFail;
   if (FileErr(file->Write(&bytes, data))) return colErr_WriteGameFail;
   Info.fpGameEnd += Map[gameNo].size;
   Info.gameBytes += Map[gameNo].size;
   Info.resultCount[result]++;

   infoDirty = true;

   //--- Finally update file size ---

   if (flush) return WriteInfo();
   return colErr_NoErr;
} /* SigmaCollection::AddGame */

/*----------------------------------------- Updating Games ---------------------------------------*/
// If the size of the updated game has not increased, we simply store the new data at the same
// position. Otherwise, the game is appended to the end of the file.

COLERR SigmaCollection::UpdGame (ULONG gameNo, CGame *theGame, BOOL flush)
{
   if (colLocked) return colErr_Locked;

   ULONG gameSize = theGame->Compress(gameData);

   GetGameInfo(gameNo);
   Info.resultCount[game->Info.result]--;
   Info.resultCount[theGame->Info.result]++;

   ULONG gameSize0 = Map[gameNo].size;

   if (gameSize <= Map[gameNo].size)
   {
      Map[gameNo].size = gameSize;
      if (FileErr(file->SetPos(Map[gameNo].pos))) return colErr_WriteGameFail;
      if (FileErr(file->Write(&gameSize, gameData))) return colErr_WriteGameFail;
      mapDirty = true;
   }
   else
   {
      Map[gameNo].pos  = Info.fpGameEnd;
      Map[gameNo].size = gameSize;
      if (FileErr(file->SetPos(Info.fpGameEnd))) return colErr_WriteGameFail;
      if (FileErr(file->Write(&gameSize, gameData))) return colErr_WriteGameFail;
      Info.fpGameEnd += gameSize;
      infoDirty = true;

      if (flush) WriteInfo();
   }

   Info.gameBytes += gameSize - gameSize0;

   if (flush) return WriteMap(gameNo, 1);
   return colErr_NoErr;
} /* SigmaCollection::UpdGame */

/*----------------------------------------- Removing Games ---------------------------------------*/
// Deleting games are very simple: The corresponding Map entries are simply removed and the
// gameCount decreased accordingly.

COLERR SigmaCollection::DelGame (ULONG gameNo, BOOL flush)
{
   if (colLocked) return colErr_Locked;

   return DelGames(gameNo, 1, flush);
} /* SigmaCollection::DelGame */


COLERR SigmaCollection::DelGames (ULONG gameNo, ULONG count, BOOL flush)
{
   if (colLocked) return colErr_Locked;

   for (ULONG g = gameNo; g < gameNo + count; g++)
      Info.gameBytes -= Map[g].size;

   for (ULONG g = gameNo + count; g < Info.gameCount; g++)
      Map[g - count] = Map[g];
   Info.gameCount -= count;

   infoDirty = true;
   mapDirty = true;

   if (! flush) return colErr_NoErr;

   COLERR err;
   if ((err = WriteInfo()) != colErr_NoErr) return err;        // Write collection info
   if ((err = WriteMap(gameNo)) != colErr_NoErr) return err;   // Write map from gameNo and on...
   return colErr_NoErr;
} /* SigmaCollection::DelGame */


// When many games are to be deleted from a large collection, it's more effecient to
// first "mark" the deleted games (by setting the "pos" field to 0), and then delete them
// all in one go.

COLERR SigmaCollection::DelMarkedGames (BOOL flush)
{
   ULONG g1 = 0;     // Destination index
   ULONG g2 = 0;     // Source index [0...Info.gameCount - 1]
   ULONG count = 0;  // Number of deleted games so far

   // During the deletion process we have g1 <= g2
   while (g2 < Info.gameCount)
   {
      // Find next entry which is NOT to be deleted
      while (Map[g2].pos == 0 && g2 < Info.gameCount)
      {  Info.gameBytes -= Map[g2].size;
         count++;
         g2++;
      }

      // Move this entry "down" to g1
      if (g1 < g2 && g2 < Info.gameCount) Map[g1] = Map[g2];
      g1++;
      g2++;
   }

   // Adjust game count
   Info.gameCount -= count;

   // Set dirty flags
   infoDirty = true;
   mapDirty = true;

   if (! flush) return colErr_NoErr;

   COLERR err;
   if ((err = WriteInfo()) != colErr_NoErr) return err;        // Write collection info
   if ((err = WriteMap()) != colErr_NoErr) return err;   // Write map from gameNo and on...
   return colErr_NoErr;
} /* SigmaCollection::DelMarkedGames */


/**************************************************************************************************/
/*                                                                                                */
/*                                          MOVE SUBLIST                                          */
/*                                                                                                */
/**************************************************************************************************/

// Renumbers the games gfrom...gfrom + count - 1 to gto...gto + count - 1
// The ViewMap MUST be sorted by game number and NO filtering allowed.
// The calling routine should change the selection in the game list accordingly.

BOOL SigmaCollection::Move (LONG gfrom, LONG gto, LONG count)
{
   if (colLocked) return false;

   if (gto + count - 1 >= Info.gameCount || gto == gfrom) return false;
 
   // Allocate temporary buffer:
   COLMAP* TmpMap = (COLMAP*)Mem_AllocPtr(count*sizeof(COLMAP));
   if (! TmpMap) return sigmaApp->MemErrorDialog();

   // First backup the games to be moved:
   for (LONG g = gfrom; g < gfrom + count; g++)
      TmpMap[g - gfrom] = Map[g];

   // Then move any trailing games down:
   for (LONG g = gfrom + count; g < Info.gameCount; g++)
      Map[g - count] = Map[g];

   // Then make room for the games to be moved:
   for (LONG g = Info.gameCount - 1; g >= gto + count; g--)
      Map[g] = Map[g - count];
 
   // Finally copy the games back in the new "hole":
   for (LONG g = gto; g < gto + count; g++)
      Map[g] = TmpMap[g - gto];
 
   // Free temporary buffer and write game map to disk:
   Mem_FreePtr(TmpMap);
   WriteMap(MinL(gfrom,gto), MaxL(gfrom,gto) + count - MinL(gfrom,gto));
   return true; 
} /* SigmaCollection::Move */


/**************************************************************************************************/
/*                                                                                                */
/*                                       COMPACT COLLECTION                                       */
/*                                                                                                */
/**************************************************************************************************/

// OR: USE MERGESORT ON GAMEMAP.POS AND THEN MOVE THE GAMES (SHOULD ALSO COMPACT GAMEMAP).

COLERR SigmaCollection::Compact (void)
{
   if (colLocked) return colErr_Locked;

   BOOL error = false;

   // New physical (and logical) end of Game Map:
   Info.fpMapEnd = Info.fpMapStart + Info.gameCount*sizeof(COLMAP);
   Info.fpGameStart = Info.fpMapEnd;
   WriteInfo();

   ULONG fpGameEnd = Info.fpGameStart;  // New/temporary logical end of file

   //--- Perform the actual compaction process ---

   BeginProgress("Compacting...", "Compacting...", Info.gameCount);

   for (ULONG n = 0; n < Info.gameCount && !error; n++)
   {
      FPOS minPos = 0x7FFFFFFF;                     // Find "first" game after current
      LONG gameNo = -1;                             // logical "eof"...

      for (ULONG g = 0; g < Info.gameCount; g++)
         if (Map[g].pos >= fpGameEnd && Map[g].pos < minPos)
            minPos = Map[g].pos,
            gameNo = g;

      if (Map[gameNo].pos > fpGameEnd)              // If compaction possible...
      {
         ULONG bytes = Map[gameNo].size;
         if (error = FileErr(file->SetPos(Map[gameNo].pos))) goto done;    // Initialize filemarker to old pos.
         if (error = FileErr(file->Read(&bytes, gameData)))  goto done;    // Read the data from old pos.
         if (error = FileErr(file->SetPos(fpGameEnd)))       goto done;    // Initialize filemarker to new pos.
         if (error = FileErr(file->Write(&bytes, gameData))) goto done;    // Rewrite the data at new pos.
         Map[gameNo].pos = fpGameEnd;                              // Update collection map entry.
         WriteMap(gameNo, 1);
      }

      fpGameEnd += Map[gameNo].size;                // Update new logical end of file.

      if (n % 10 == 0)
      {  SetProgress(n, "");
         if (ProgressAborted()) error = true;
      }
   }

done:
   EndProgress();

   if (! error)
   {  Info.fpGameEnd = fpGameEnd;
      WriteInfo();
   }

   return (error ? colErr_WriteGameFail : colErr_NoErr);
} /* SigmaCollection::Compact */


/**************************************************************************************************/
/*                                                                                                */
/*                                   POSITION LIBRARY IMPORT                                      */
/*                                                                                                */
/**************************************************************************************************/

void SigmaCollection::PosLibImport (ULONG i1, ULONG i2, const LIBIMPORT_PARAM *param)
{
   if (param->maxMoves <= 0 || i1 > i2 || ! (param->impWhite || param->impBlack)) return;

   LONG count = i2 + 1 - i1;          // Total number of games to process
   LONG posCount0 = PosLib_Count();   // Initial number of library positions

   BeginProgress("Library Import", "Library Import", count);

   for (ULONG i = i1; i <= i2 && ! ProgressAborted(); i++)
   {
      ULONG g = ViewMap[i];
      ULONG n = i - i1;

      CHAR status[100];
      if (param->libClass != libClass_Unclassified)
         Format(status, "%d%c (%ld positions added)", (100L*n)/count, '%', PosLib_Count() - posCount0);
      else
         Format(status, "%d%c (%ld positions removed)", (100L*n)/count, '%', posCount0 - PosLib_Count());
      if (n % 10 == 0) SetProgress(n, status);

      //--- Fetch and decompress next game ---

      ULONG bytes = Map[g].size;
      if (FileErr(file->SetPos(Map[g].pos))) goto error;
      if (FileErr(file->Read(&bytes,gameData))) goto error;
      game->Decompress(gameData, bytes, true);

      //--- Process the Game ---

      BOOL impWhite = param->impWhite;
      BOOL impBlack = param->impBlack;

      if (param->skipLosersMoves)                    // First check if loser's moves should be ignored
      {
         if (game->Info.result == infoResult_WhiteWin)
            impBlack = false;
         else if (game->Info.result == infoResult_BlackWin)
            impWhite = false;
      }

      if (! impWhite && ! impBlack) continue;

      if (game->Init.wasSetup)
         PosLib_Classify(game->player, game->Board, param->libClass, param->overwrite);

      // Replay the first "N" moves (but don't terminate a recapture sequence)
      for (INT j = 0; j < game->lastMove; j++)
      {
         // If move limit reached, only continue if recaptures
         if (j >= 2*param->maxMoves)
         {
            MOVE *m0 = game->GetGameMove(j);      // Most recent move (we know j > 0)
            MOVE *m1 = game->GetGameMove(j + 1);  // Next move (we know j < lastMove)

            if (! (m0->cap && m1->to == m0->to))  // If not a recapture of previous move -> stop
               break;
         }

         game->RedoMove(true);

         if (game->opponent == white)         // Check if position should be skipped based
         {  if (! impWhite) continue;         // on side to move (OPPONENT not player, because
         }                                    // we have just replayed the move).
         else
         {  if (! impBlack) continue;
         }

         PosLib_Classify(game->player, game->Board, param->libClass, param->overwrite);
      }
   }

error:
   EndProgress();
} /* SigmaCollection::PosLibImport */


/**************************************************************************************************/
/*                                                                                                */
/*                                      CONVERT FROM · 4 FORMAT                                   */
/*                                                                                                */
/**************************************************************************************************/

// If the user opens a · 4 collection, he is forced to convert this to the version 5 format by
// first creating a new collection, and then calling the "Sigma4Convert" method:

BOOL SigmaCollection::Sigma4Convert (CFile *file4)
{
   // First we load the collection map from the version 4 resource file:
   if (FileErr(file4->OpenRes(filePerm_Rd))) return false;

      HANDLE cmh4 = (HANDLE)::GetResource('·CMP', 0);

      if (cmh4 == nil)
         NoteDialog(nil, "Conversion Error", "ERROR: Failed loading collection map...", cdialogIcon_Error);
      else
      {  // Lock collection map handle and perform conversion:
         Mem_LockHandle(cmh4);
         Sigma4ConvertData(file4, *cmh4);
         Mem_UnlockHandle(cmh4);

         // Finally flush info and map:
         WriteInfo();
         WriteMap();
      }

   return FileErr(file4->CloseRes());
} /* SigmaCollection::Sigma4Convert */


void SigmaCollection::Sigma4ConvertData (CFile *file4, PTR cm4)
{
   // First convert collection "header" and game count:
   P2C_Str(cm4 +  2, Info.title);
   P2C_Str(cm4 + 34, Info.author);
   P2C_Str(cm4 + 66, Info.descr);
   LONG gameCount4 = *(LONG*)(cm4 + 330);

   // Next allocate space for the game map:
   Info.gameCount   = 0;  // This is incremented as each game is converted.
   Info.fpMapEnd    = Info.fpMapStart + gameCount4*sizeof(COLMAP);
   Info.fpGameStart = Info.fpGameEnd = Info.fpMapEnd;
   Mem_FreePtr((PTR)Map);
   Map = (COLMAP*)Mem_AllocPtr(gameCount4*sizeof(COLMAP));
   if (! Map)
   {  sigmaApp->MemErrorDialog();
      return;
   }

   if (gameCount4 > maxGamesLite && ! ProVersion())
   {  ProVersionDialog(nil, "Collections are limited to 1000 games in Sigma Chess Lite. Only the first 1000 games will be converted.");
      gameCount4 = maxGamesLite;
   }

   // Then open · 4 data fork and convert each game:
   if (FileErr(file4->Open(filePerm_Rd))) return;

   CHAR  prompt[200];
   Format(prompt, "Converting collection Ò%sÓ to Ò%sÓ...", file4->name, file->name);
   BeginProgress("Converting Collection", prompt, gameCount4);

   GMAP4* Map4 = (GMAP4*)(cm4 + 740);

   for (LONG g = 0; g < gameCount4 && ! ProgressAborted(); g++)
   {
      Sigma4ConvertProgress(g, gameCount4);

      ULONG bytes = Map4[g].size;
      if (FileErr(file4->SetPos(Map4[g].pos))) goto done;
      if (FileErr(file4->Read(&bytes, gameData))) goto done;

      game->Read_V34(gameData,false);
      Info.resultCount[game->Info.result]++;
      bytes = game->Compress(gameData);

      Map[g].pos = Info.fpGameEnd;
      Map[g].size = bytes;
      Info.fpGameEnd += Map[g].size;

      if (FileErr(file->SetSize(Info.fpGameEnd))) goto done;
      if (FileErr(file->SetPos(Map[g].pos))) goto done;
      if (FileErr(file->Write(&bytes, gameData))) goto done;
      Info.gameCount++;
   }

done:
   Sigma4ConvertProgress (gameCount4, gameCount4);
   EndProgress();

   FileErr(file4->Close());
} /* SigmaCollection::Sigma4ConvertData */


void SigmaCollection::Sigma4ConvertProgress (ULONG g, ULONG gameCount4)
{
   if (g % 10 == 0)        // Update progress indicator
   {  CHAR status[100];
      Format(status, "Game %ld of %ld", g, gameCount4);
      SetProgress(g, status);
   }
} /* SigmaCollection::Sigma4ConvertProgress */


/**************************************************************************************************/
/*                                                                                                */
/*                                   COLLECTION PROGRESS DIALOG                                   */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------- Collection Progress Dialog API --------------------------------*/

void SigmaCollection::BeginProgress (CHAR *title, CHAR *prompt, ULONG max, BOOL useProgressDlg)
{
   if (window && ! useProgressDlg)
   {  progressAborted = false;
      window->HandleMessage(colProgress_Begin, max, (PTR)title);
   }
   else
   {  progressDlg = ProgressDialog_Open(nil, title, prompt, max);
   }
} /* SigmaCollection::BeginProgress */


void SigmaCollection::SetProgress (ULONG n, CHAR *status)
{
   if (window && ! progressDlg)
      window->HandleMessage(colProgress_Set, n, (PTR)status);
   else
      progressDlg->Set(n, status);
} /* SigmaCollection::SetProgress */


void SigmaCollection::EndProgress (void)
{
   if (window && ! progressDlg)
      window->HandleMessage(colProgress_End);
   else
      delete progressDlg;
   progressDlg = nil;
} /* SigmaCollection::EndProgress */


BOOL SigmaCollection::ProgressAborted (void)
{
   return (window && ! progressDlg ? progressAborted : progressDlg->Aborted());
} /* SigmaCollection::ProgressAborted */


/**************************************************************************************************/
/*                                                                                                */
/*                                              MISC                                              */
/*                                                                                                */
/**************************************************************************************************/

BOOL SigmaCollection::CheckGameCount (CHAR *prompt)
{
   CHAR msg[500];

   if (GetGameCount() >= maxGamesLite && ! ProVersion())
   {  Format(msg, "Collections are limited to 1000 games in Sigma Chess Lite. %s.", prompt);
      ProVersionDialog(nil, msg);
      return false;
   }
   else if (GetGameCount() >= maxGamesPro)
   {  Format(msg, "Collections are limited to 1 million games. %s.", prompt);
      NoteDialog(nil, "Collection Limit", msg);
      return false;
   }
   return true;
} /* SigmaCollection::CheckGameCount */
