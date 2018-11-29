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

#include "CUtility.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

enum STRING_GROUPS
{
   // Menu string groups (0..9):
   sgr_FileMenu         = 0,
   sgr_EditMenu         = 1,
   sgr_GameMenu         = 2,
   sgr_AnalyzeMenu      = 3,
   sgr_LevelMenu        = 4,
   sgr_EngineMenu       = 8,
   sgr_DisplayMenu      = 5,
   sgr_CollectionMenu   = 6,
   sgr_LibraryMenu      = 7,
   sgr_WindowMenu       = 9,

   // Sub menu string groups (10...29):
   sgr_PlayingStyleMenu = 10,
   sgr_NotationMenu     = 11,
   sgr_PieceLettersMenu = 12,
   sgr_PieceSetMenu     = 13,
   sgr_BoardTypeMenu    = 14,
   sgr_ColorSchemeMenu  = 15,
   sgr_MoveMarkerMenu   = 16,
   sgr_FileOpenMenu     = 17,
   sgr_FileSaveMenu     = 18,
   sgr_BoardSizeMenu    = 19,
   sgr_LibClassifyMenu  = 20,
   sgr_LibAutoClassMenu = 21,
   sgr_LibSetMenu       = 22,
   sgr_AnalysisDispMenu = 23,

   // Common/Misc (30..34)
   sgr_Common           = 30,
   sgr_License          = 31,

   // Help text groups (35..49)
   sgr_HelpTbGame       = 35,
   sgr_HelpTbCol        = 36,
   sgr_HelpTbLib        = 37,
   sgr_HelpPiece        = 38,

   // Level Dialog groups (45..50)
   sqr_LD_ModesMenu     = 45,
   sgr_LD_ModesDescr    = 46,
   sgr_LD_TimeMenu      = 47,
   sgr_LD_MovesMenu     = 48,
   sgr_LD_AvgMenu       = 49,
   sgr_LD_Misc          = 50,

   // Playing Strength Dialog groups (54..55)
   sgr_PSD_Misc         = 54,
   sgr_PSD_Cat          = 55,

   // Collection Filter Dialog (56..60)
   sgr_Filter_Misc      = 56,
   sgr_Filter_FieldMenu = 57,
   sgr_Filter_CondMenu  = 58,

   sgr_Count
};

enum COMMON_STRINGS
{
   s_OK = 0,
   s_Cancel,
   s_Yes,
   s_No,
   s_Open,
   s_Close,
   s_Save,
   s_Delete,
   s_Default,
   s_White,
   s_Black,
   s_All,
   s_Mate,
   s_MateIn,
   s_mate
};


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

CHAR *GetStr (INT groupID, INT index);
CHAR *GetCommonStr (INT index);
