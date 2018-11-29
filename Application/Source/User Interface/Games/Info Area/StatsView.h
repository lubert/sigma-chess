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

#include "BackView.h"
#include "DataView.h"
#include "SpringHeaderView.h"
#include "AnalysisState.h"

#define statsHeaderLineHeight  springHeaderLineHeight

class GameWindow;

class StatsView : public SpringHeaderView
{
public:
   StatsView (CViewOwner *parent, CRect frame);

   virtual void HandleUpdate (CRect updateRect);   
   virtual void HandleActivate (BOOL wasActivated);
   virtual void HandleToggle (BOOL closed);

   void Reset (void);
   void SetStatus (CHAR *statusStr);
   void SetScore (INT score, INT scoreType, INT pvNo);
   void SetMainDepth (INT depth, INT pvNo);
   void SetCurrent (void);
   void SetNodes (LONG64 nodes, ULONG searchTime, ULONG nps, INT hashFull);

private:
   void DrawEngineName (void);
   void DrawScore (void);
   void DrawDepthCurrent (void);
   void DrawNodes (void);

   GameWindow *win;
   ANALYSIS_STATE *Analysis;
};
