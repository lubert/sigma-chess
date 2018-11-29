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

#include "CToolbar.h"

class GameToolbar : public CToolbar
{
public:
   GameToolbar (CViewOwner *parent, CRect frame);

   void Adjust (void);

   CButton *tb_UndoAllMoves;
   CButton *tb_UndoMove;
   CButton *tb_Go, *tb_Stop;
   CButton *tb_RedoMove;
   CButton *tb_RedoAllMoves;

   CButton *tb_NewGame;
   CButton *tb_SaveGame;
   CButton *tb_GameInfo;

   CButton *tb_Resign;
   CButton *tb_DrawOffer;
   CButton *tb_TurnBoard;

   CButton *tb_PrintGame;
};


class LastMoveView;
class SigmaELOView;
class PlayerELOView;

class MiniGameToolbar : public CToolbar
{
public:
   MiniGameToolbar (CViewOwner *parent, CRect frame);
   virtual void HandleUpdate (CRect updateRect);

   void Adjust (void);
   void DrawReadOnlyGroup (BOOL redrawBackground);

   // Group 1 (last move)
   LastMoveView *cv_LastMove;

   // Group 2 (hint)
   CButton *tb_Hint;

   // Group 3 (level etc)
   CButton *tb_Level; CMenu *pm_Level; INT modeItem;
   CButton *tb_Style; CMenu *pm_Style; INT styleItem;
   CButton *tb_PermBrain;
   CButton *tb_Randomize;

   // Group 4 (Sigma Strength + Player Strength + ELO Calc)
   CButton *tb_SigmaStrength;  SigmaELOView *cv_SigmaELO;
   CButton *tb_PlayerStrength; PlayerELOView *cv_PlayerELO;
   CButton *tb_ELOCalc;
};
