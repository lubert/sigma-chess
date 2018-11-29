/**************************************************************************************************/
/*                                                                                                */
/* Module  : ANALYZEEPD.C                                                                         */
/* Purpose : This module analyzes and EPD file.                                                   */
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

#include "GameWindow.h"
#include "SigmaApplication.h"

#include "CDialog.h"
#include "CMemory.h"
#include "TaskScheduler.h"

#include "Engine.f"
#include "Move.f"
#include "Board.f"


/**************************************************************************************************/
/*                                                                                                */
/*                                          ANALYZE EPD                                           */
/*                                                                                                */
/**************************************************************************************************/

void GameWindow::AnalyzeEPD (void)
{
   if (! CheckSave("Save before analyzing EPD file?")) return;

   CFile inFile, outFile;
   ULONG bytes;
   PTR   data;
   CHAR  s[1000], outName[50];
   BOOL  posFound = false;

   // Open source EPD file:
   CFileTextOpenDialog dlg;
   if (! dlg.Run(&inFile, "Open EPD File")) return;

   // Create output file:
   Format(outName, "%s.out", inFile.name);
   if (! outFile.SaveDialog("Save EPD Output", outName)) return;
   if (outFile.saveReplace) outFile.Delete();

   outFile.SetCreator('ttxt');
   outFile.SetType('TEXT');
   outFile.Create();

   Format(s, "--- %s EPD Analysis Output ---\r", engineName);
   outFile.AppendStr(s);

   SetMultiPVcount(1);

   // Load the EPD file and analyze it:
   if (inFile.Load(&bytes, &data) == fileError_NoError)
   {
      ULONG n = 0;
      analyzeEPD = true;

      while (n < bytes && analyzeEPD)
      {
         ReadLine(data, bytes, &n, 1000, s);

         // #tc=600
         if (s[0] == '#')
         {
            LONG time;
            if (s[1] == 't' && s[2] == 'c' && s[3] == '=' && StrToNum(&s[4],&time))
            {
               level.mode = pmode_Solver;
               level.Solver.timeLimit = time;
               level.Solver.scoreLimit = maxVal;
               ResetClocks();
               boardAreaView->DrawModeIcons();
            }
         }

         // EPD Line: rn1qkb1r/pp2pppp/5n2/3p1b2/3P4/2N1P3/PP3PPP/R1BQKBNR w KQkq - bm Qb3; id "CCR.01";
         else if (IsAlphaNum(s[0]) && game->Read_EPD(s) == epdErr_NoError)
         {
            posFound = true;

            CHAR *id = game->Info.heading;
            CHAR *bm = game->Info.blackName;
            SetTitle(EqualStr(id,"") ? "<Untitled Position>" : id);
            RefreshGameInfo();
            GameMoveAdjust(true);
            ResetClocks();

            Analyze_Go();
            while (thinking) theApp->MainLooper();

            if (analyzeEPD)
            {
               CHAR AnaText[1000], OutText[1000];
               INT resLen = BuildAnalysisString(&Analysis, AnaText);
               CHAR clockStr[9];
               FormatClockTime(Engine_MainTime(engine)/60, clockStr);

               Format(OutText, "%-15s%-15s: [%s] %s\r", id, bm, clockStr, AnaText);
               outFile.AppendStr(OutText);
            }
         }
      }

      Mem_FreePtr(data);
   }

   outFile.CompleteSave();

//###   game->dirty = false;

   if (! posFound)
      NoteDialog(this, "Invalid EPD File", "This file doesn't seem to be a valid EPD file: No positions were found...", cdialogIcon_Error);
} /* GameWindow::AnalyzeEPD */
