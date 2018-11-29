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

#include "CBitmap.h"
#include "CUtility.h"
#include "CMenu.h"
#include "Board.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define firstPieceSetID   1000     // Resource ID of first piece set PICT.
#define figurineID        1200

#define pieceButtonSize   (minSquareWidth + 6)


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class PieceBmp : public CBitmap
{
public:
   PieceBmp (INT pieceSet);
   void  LoadPieceSet (INT n);        // n = 0..5
   void  LoadPieceSetPlugin (INT n);  // n = 0..9
   CRect CalcPieceRect (PIECE p);
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES                                 */
/*                                                                                                */
/**************************************************************************************************/

extern PieceBmp *pieceBmp1;
extern CBitmap  *pieceBmp2, *pieceBmp3, *pieceBmp4;
extern CBitmap  *wSquareBmp, *bSquareBmp;
extern CBitmap  *figurineBmp;
extern CBitmap  *utilBmp;
extern CView    *utilBmpView, *wSquareBmpView, *bSquareBmpView;


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

CBitmap *GetBmp (INT bmpID, INT depth = 8);
void LoadSquareBmp (INT boardType);
CRect CalcPieceBmpRect (PIECE p, INT sqWidth);

void InitPieceSetPlugins (void);
void AddPieceSetPlugins (CMenu *pm);
INT  PieceSetPluginCount (void);

void InitBoardTypePlugins (void);
void AddBoardTypePlugins (CMenu *pm);
INT  BoardTypePluginCount (void);

void InitBmpUtilModule (void);
