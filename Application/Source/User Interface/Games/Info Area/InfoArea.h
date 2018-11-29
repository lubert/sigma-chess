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
#include "GameView.h"
#include "AnalysisView.h"
#include "StatsView.h"
#include "PosEditor.h"
#include "AnnEditor.h"
#include "LibEditor.h"

#include "Move.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

enum INFO_MODE
{
   infoMode_None      = -1,
   infoMode_MovesOnly = 0,
   infoMode_Analysis  = 1,
   infoMode_Annotate  = 2,
   infoMode_EditLib   = 3,
   infoMode_EditPos   = 4
};


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class InfoDividerView;

class InfoAreaView : public BackView
{
public:
   InfoAreaView (CViewOwner *parent, CRect frame);

   virtual void HandleUpdate (CRect updateRect);
   virtual void HandleResize (void);
   virtual BOOL HandleKeyDown (CHAR c, INT key, INT modifiers);

   void Show (BOOL show);

   void UpdateGameList  (void);
   void RedrawGameList  (void);
   void RefreshPieceSet (void);
   void RefreshNotation (void);
   
   void FlushAnnotation (void);
   void LoadAnnotation  (void);
   BOOL AdjustAnnEditor (CEditor *ctrl, BOOL textChanged, BOOL selChanged);
   void AdjustAnnGlyph  (void);

   void ShowPosEditor (BOOL show);
   void ShowAnnEditor (BOOL show);
   void ShowLibEditor (BOOL show);
   void ShowAnalysis  (BOOL show);

   void SetDividerPos (INT dividerPos);
   void DrawDivider (void);

   void RefreshGameStatus (void);
   void RefreshGameInfo (void);
   void RefreshAnalysis (void);
   void RefreshLibInfo (void);
   void ResizeGameHeader (void);
   void AdjustAnalysisToolbar (void);

   // Setting search results/statistics:
   void ResetAnalysis (void);
   void SetAnalysisStatus (CHAR *statusStr, BOOL flushPortBuffer = false);
   void SetMainDepth (INT depth, INT pvNo);
   void SetMainLine  (MOVE MainLine[], INT depth, INT pvNo);
   void SetScore     (INT score, INT scoreType, INT pvNo);
   void SetCurrent   (INT n, MOVE *m);
   void SetNodes     (LONG64 nodes, ULONG searchTime, ULONG nps, INT hashFull);

   // Subwindow dimensions:
   INT  dividerPos;    // Identical to game view height
   void CalcFrames (CRect *divider, CRect *upperRect, CRect *lowerRect);

   // Subviews:
   GameView        *gameView;
   InfoDividerView *dividerView;
   AnalysisView    *analysisView;
   AnnEditorView   *annEditorView;
   LibEditorView   *libEditorView;

   PosEditorView  *posEditorView;   // nil if position editor not open

   BOOL showAnalysis;               // Is the analysis window visible?
   BOOL showAnnEditor;              // Is the analysis window visible?
   BOOL showLibEditor;              // Is the analysis window visible?
};


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

INT InfoArea_Width (void);
