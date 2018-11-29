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

#include "General.h"
#include "SigmaApplication.h"
#include "Game.h"
#include "GameUtil.h"
#include "CollectionFilter.h"
#include "PosLibrary.h"

#include "CFile.h"
#include "CDialog.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define collectionVersion 0x0500
#define colMapBlockAllocationSize (100*sizeof(GMAP))

#define maxColGameSize 1000000L
#define maxGamesLite   1000L
#define maxGamesPro    maxColGameSize 

enum COLINFO_FLAG
{
   colInfoFlag_Publishing = 0x0001
};

enum INDEX_FIELD         // Indexable game info fields (columns in collection header win):
{
   inxField_GameNo    = 0,
   inxField_WhiteName = 1,
   inxField_BlackName = 2,
   inxField_EventSite = 3,   // Combined in one field
   inxField_Date      = 4,
   inxField_Round     = 5,
   inxField_Result    = 6,
   inxField_ECO       = 7
};

enum COL_PROGRESS_MSG
{
   colProgress_Begin,
   colProgress_Set,
   colProgress_End
};

enum COLERR
{
   colErr_NoErr = 0,
   colErr_MemFull,
   colErr_WriteInfoFail,
   colErr_ReadInfoFail,
   colErr_WriteMapFail,
   colErr_ReadMapFail,
   colErr_WriteGameFail,
   colErr_ReadGameFail,
   colErr_Locked
};

#define maxGameKeyLen (nameStrLen + 2)

#define colTitleLen    50
#define colAuthorLen   50
#define colDescrLen  1000


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

typedef ULONG FPOS;

typedef struct
{
   //--- User Defined Info ---
   INT      version;                    // Currently 0x0500.
   CHAR     title[colTitleLen + 1];     // Collection title (optional).
   CHAR     author[colAuthorLen + 1];   // Author (optional).
   CHAR     descr[colDescrLen + 1];     // Description (optional).
   ULONG    flags;                      // User definable options/flags.     

   //--- Game Count & Statistics ---
   ULONG    gameCount;        // Total number of games in collection.
   ULONG    gameBytes;        // Total number of bytes actually used in the game data block.
                              // Is used to determine fragmentation.
   ULONG    resultCount[5];   // Counter for each game result value (1..4)

   //--- File Pointers ---

   FPOS     fpMapStart;       // Start of Game Map (first byte).
   FPOS     fpMapEnd;         // Physical end of Game Map (next byte after last byte).

   FPOS     fpGameStart;      // Start of Game Block (first byte).
   FPOS     fpGameEnd;        // Physical end of Game Block (next byte after last byte = EOF).

   FPOS     fileSize;         // End of file (and end of Game Data).
} COLINFO;


typedef struct                // Collection game map entry:
{
   FPOS    pos;               // File position (in game data part) of game.
   UINT    size;              // Size of game data.
} COLMAP;


typedef struct                // Index entry:
{
   ULONG   g;                 // The game number (0...gameCount - 1)
   ULONG   k;                 // The first 4 bytes of the key (if a string). Often enough
                              // to compare keys.
} COLINX;


typedef struct                // Hashed key cache entry:
{
   ULONG g;                   // Reset to 0xFFFFFFFF
   CHAR  key[2];              // At least two bytes long...
} HKEY_CACHE;


typedef struct
{
   ULONG    gameNo;
   GAMEINFO Info;
} GAMEKEY;

/*----------------------------------- Sigma 4 Collection Format ----------------------------------*/
// IMPORTANT: Because � Chess uses 68K (2 byte) alignment, the version 4 collection map is NOT
// binary compatible. Therefore access to the fields of this map is done using direct/explicit
// pointer offsets.

typedef struct
{
   ULONG  pos;              // File position (in game data part) of game.
   ULONG  size;             // Size of game data.
} GMAP4;

typedef struct
{
   INT    version;          //   0: Currently 0x0100.
   Str31  title;            //   2: Collection title (optional).
   Str31  author;           //  34: Author (optional).
   Str255 descr;            //  66: Description (optional).
   ULONG  flags;            // 322: Various settings.
   LONG   auxDataSize;      // 326: Size of auxiliary data (if any). Follows after map entries.
   LONG   gameCount;        // 330: Total number of games in collection.
   LONG   currentGame;      // 334: Current game ("book mark").
   INT    currentMove;      // 338: Current move in current game ("book mark").
   LONG   reserved[100];    // 340: Reserved for future use.
   GMAP4  Map[];            // 740: The actual collection map
} COLMAP4_68K;


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class CProgressDialog;

// A SigmaCollection object implements access to a collection file, and must ALWAYS be associated
// with a file. This also applies to new collections; in order to create a new collection, a
// CFile object must first be created and the passed to the SigmaCollection constructor, which
// then creates and initializes the actual file.

class SigmaCollection
{
public:
   //--- Constructor/Destructor ---
   SigmaCollection (CFile *file, CWindow *window = nil);
   virtual ~SigmaCollection (void);

   //--- Collection Info Block ---
   void   ResetInfo (void);
   COLERR ReadInfo (void);
   COLERR WriteInfo (void);
   BOOL   IsLocked (void);

   INT    CalcScoreStat (void);
   BOOL   Publishing (void);

   //--- Collection Map Block ---
   COLERR ReadMap (void);
   COLERR WriteMap (ULONG gameNo = 0, ULONG count = 0);  // 0 means all!
   COLERR GrowMap (ULONG count);
   BOOL   MapFull (ULONG count);   // Does game map need growing if "count" games are added?

   //--- Access Games & Info ---
   ULONG  View_GetGameNo (ULONG N);
   ULONG  View_GetGameCount (void);
   COLERR View_GetGame (ULONG gameNo, CGame *toGame);
   COLERR View_GetGameInfo (ULONG N);

   ULONG  GetGameCount (void);
   COLERR GetGame (ULONG gameNo, PTR data, ULONG *gameSize);
   COLERR GetGame (ULONG gameNo, CGame *toGame = nil, BOOL raw = false);
   COLERR GetGameInfo (ULONG gameNo);                   // 0 <= N <= indexCount - 1

   //--- Add/Update/Delete Games ---
   COLERR AddGame (ULONG gameNo, CGame *game, BOOL flush = true);
   COLERR AddGame (ULONG gameNo, PTR data, ULONG gameSize, INT result, BOOL flush = true);
   COLERR UpdGame (ULONG gameNo, CGame *game, BOOL flush = true);
   COLERR DelGame (ULONG gameNo, BOOL flush = true);
   COLERR DelGames (ULONG gameNo, ULONG count, BOOL flush = true);
   COLERR DelMarkedGames (BOOL flush = true);

   void   View_Reset (void);
   BOOL   View_Add (ULONG gfirst, ULONG glast);
   void   View_Delete (ULONG first, ULONG last);
   ULONG  View_Search (CHAR *key);
   BOOL   View_UpdateGame (ULONG g);
   ULONG  View_CalcPos (ULONG g);
   void   View_Rebuild (void);

   BOOL   Move (LONG gfrom, LONG gto, LONG count);

   BOOL   CheckGameCount (CHAR *prompt);

   //--- Compact Collection ---
   COLERR Compact (void);

   //--- Import positions to library ---
   void  PosLibImport (ULONG i1, ULONG i2, const LIBIMPORT_PARAM *param);

   //--- � 4 collection conversion ---
   BOOL   Sigma4Convert (CFile *file4);
   void   Sigma4ConvertData (CFile *file4, PTR cm4);
   void   Sigma4ConvertProgress (ULONG g, ULONG gameCount4);

   //--- PGN Import/Export (CollectionPGN.c) ---
   BOOL   ImportPGN (CFile *pgnFile);
   void   ImportPGNProgress (ULONG gameCount, ULONG errorCount, ULONG bytesProcessed, ULONG pgnSize);
   void   HandlePGNError (CPgn *pgn);
   BOOL   ExportPGN (CFile *pgnFile, ULONG i1, ULONG i2);

   //--- EPD Import (CollectionPGN.c) ---
   BOOL   ImportEPD (CFile *epdFile);

   //--- Sorting/Indexing (CollectionSort.c) ---
   BOOL   Sort (INDEX_FIELD f);
   BOOL   SetSortDir (BOOL ascend);
   BOOL   SortView (void);
   BOOL   SortGameList (ULONG G[], ULONG N);
   void   MergeSort (ULONG G[], ULONG A[], ULONG B[], ULONG min, ULONG max);
   BOOL   CompareKey (ULONG G[], ULONG i1, ULONG i2);
   BOOL   CreateKeyCache (ULONG G[], ULONG N);
   void   FreeKeyCache (void);
   void   RetrieveGameKey (ULONG gameNo, CHAR *key);

   //--- Filtering (CollectionFilter.c) ---
   BOOL   FilterGame (ULONG g);
   void   ResetFilter (void);

   //--- Generic progress dialog ---
   void   BeginProgress (CHAR *title, CHAR *prompt, ULONG max, BOOL useProgressDlg = false);
   void   SetProgress (ULONG n, CHAR *status);
   void   EndProgress (void);
   BOOL   ProgressAborted (void);
   BOOL   progressAborted;

   //--- Instance variables ---

   CFile        *file;          // File "pointer". Nil if collection object was not created
                                // properly.
   CWindow      *window;        // Window to which collection belongs (nil if none).
   COLINFO      Info;           // Copy of the collection info block at beginning of file
   COLMAP       *Map;           // Pointer to first collection map entry.

   ULONG        *ViewMap;       // The ordered set/subset of the game map, that's presented
                                // to the user (e.g. after sorting and filtering).
   ULONG        viewCount;      // Number of games in this view.
   INDEX_FIELD  inxField;       // Index field/tag by which we are currently sorting.
   BOOL         ascendDir;      // Sort ascending?

   BOOL         hashedKey;      // Are we using the hashed key cache?
   INT          keyRecSize;     // Size in bytes of key cache (always even!).
   CHAR         *DKeyCache;     // Hashed Key cache used for the "large" string fields.
// HKEY_CACHE   *HKeyCache;     // Hashed Key cache used for the "large" string fields.
// ULONG        keyCacheMask;   // Key cache hash mask.

   FILTER       filter;         // Current filter.
   BOOL         useFilter;      // Should this filter be applied?

   BOOL         infoDirty;
   BOOL         mapDirty;
   BOOL         colLocked;      // Is collection locked?
   BOOL         liteLimit;      // Were there too many games for Lite version?

   CGame        *game;          // Utility game object.

   CProgressDialog *progressDlg;   // Utility progress dialog.

   // PGN Import utility
   BOOL         pgn_SkipThisGame,       // Skip current PGN game and continue with next.
                pgn_AutoSkipErrors,     // Automatically skip all errors.
                pgn_AbortImport,
                pgn_DeleteImported;     // Delete imported games (after import process halted).
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES                                 */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/
