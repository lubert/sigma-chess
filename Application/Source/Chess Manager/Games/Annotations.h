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

#include "General.h"
#include "Game.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

typedef struct
{
   INT   lineCount;            // Number of text lines.
   INT   charCount;            // Number of characters in annotation text.
   INT   Data[];               // The annotation data consisting of line start data
                               // followed by the actual annotation text data.
} ANNREC, *ANNPTR, **ANNHANDLE;


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class CAnnotation
{
public:
   CAnnotation (INT lineWidth);
   virtual ~CAnnotation (void);

   void Clear (INT i);
   void ClearAll (void);

   void Set (INT i, CHAR *Text, INT charCount, BOOL wrapLines, BOOL killNewLines = false);

   BOOL Exists (INT i);
   INT  GetCharCount (INT i);
   INT  GetLineCount (INT i);
   void GetText (INT i, CHAR *Text, INT *charCount);
   INT  GetTextLine (INT i, INT lineNo, CHAR *Text, BOOL *nl = nil);

   INT  maxLineWidth;
   ANNHANDLE AnnTab[gameRecSize];
};


/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES                                      */
/*                                                                                                */
/**************************************************************************************************/

extern INT AnnCharWidth[256];

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

void InitAnnotationModule (void);

INT WrapLines (CHAR *Text, INT charCount, INT CharWidth[], INT maxLineWidth, INT LineStart[]);
