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

#include "CWindow.h"
#include "Game.h"
#include "Collection.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define htmlBufSize 10000


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class CExportHTML
{
public:
   CExportHTML (CHAR *theTitle, CFile *theFile); //, HTML_OPTIONS *Options);
   virtual ~CExportHTML (void);

   void  ExportGame (CGame *theGame);
   void  ExportCollection (SigmaCollection *collection, LONG start, LONG end);

private:
   void  ExportOneGame (BOOL isCollectionGame, BOOL isPublishing);

   void  Write_Header (void);
   void  Write_BodyStart (void);
   void  Write_BodyEnd (void);
   void  Write_GameLine (INT N, INT Nmax, LONG gameNo);
   void  Write_Diagram (void);
   void  Write_Special (INT type, GAMEINFO *Info, LONG gameNo);
   void  Write_FrontPage (SigmaCollection *collection);

   void  ResetBuffer (void);
   void  FlushBuffer (void);
   void  Write (CHAR *s);
   void  WriteLine (CHAR *s);

   CFile *file;
   CGame *game;

   CHAR  title[100];
   CHAR  gifPath[100];
   CHAR  str[1000];             // Utility str buffer for use with Format(...)

   CHAR  htmlBuf[htmlBufSize];  // The actual HTML buffer
   ULONG bytes;                 // Bytes written so far to buffer

   GAMEMAP GMap[4000];
   ULONG   gameNo;
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

void HTMLGifReminder (CWindow *parent);
