/**************************************************************************************************/
/*                                                                                                */
/* Module  : EXACHESSGLUE.C                                                                       */
/* Purpose : This module interfaces to ExaChess.                                                  */
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

#include "ExaChessGlue.h"
#include "ChessEvents.h"
#include "SigmaApplication.h"
#include "GameWindow.h"
#include "PGN.h"
#include "TaskScheduler.h"

#include "Debug.h"

static GameWindow *ExaWindow = nil;
static CGame *ExaGame = nil;
static CPgn *ExaPGN = nil;


/**************************************************************************************************/
/*                                                                                                */
/*                                        STARTUP/SHUTDOWN                                        */
/*                                                                                                */
/**************************************************************************************************/

// We must install the ExaChess Apple event handler at startup:

void InitExaChess (void)  // Installs the ExaChess AE handler
{
   OSErr err = AEInstallEventHandler('CHES', typeWildCard, NewAEEventHandlerUPP(DoCHESEvent), 0, false);
} /* InitExaChess */

// When quitting Sigma Chess, we must inform ExaChess by sending the 'quit' message.
/*
void QuitExaChess (void)
{
//  if (ExaWindow)
//    ::ChessAction('quit', "", "");
} /* QuitExaChess */


BOOL ExaWindowExists (void)
{
   return (ExaWindow != nil);
} // ExaWindowExists


/**************************************************************************************************/
/*                                                                                                */
/*                                     EXACHESS EVENT HANDLING                                    */
/*                                                                                                */
/**************************************************************************************************/

static void Exa_Quit (void);
static void Exa_NewGame (void);
static void Exa_SetBoard (CHAR *msg);
static void Exa_GetBoard (CHAR *replymsg);
static void Exa_PlayMoves (CHAR *msg, CHAR *replymsg);
static void Exa_TakeBack (CHAR *msg);
static void Exa_Search (CHAR *msg, CHAR *replymsg);
static void Exa_Cancel (void);

static void RefreshExaWindow (BOOL resetClocks);


void ChessAction (LONG eventID, CHAR *msg, CHAR *replymsg)
{
   CHAR ids[5];
   *((LONG*)ids) = eventID; ids[4] = 0;

   if (debugOn)
   {  Format(debugStr, ">>> EXACHESS MESSAGE RECEIVED '%s'\n'%s'\n", ids, msg);
      DebugWrite(debugStr);
   }

   *replymsg = 0;

   switch (eventID)
   {   
      case 'disp': break;
      case 'quit': Exa_Quit(); break;
      case 'newg': Exa_NewGame(); break;
      case 'stbd': Exa_SetBoard(msg); break;
      case 'gtbd': Exa_GetBoard(replymsg); break;
      case 'move': Exa_PlayMoves(msg, replymsg); break;
      case 'back': Exa_TakeBack(msg); break;
      case 'game': /* zzgame (replymsg); */ break;
      case 'srch': Exa_Search(msg, replymsg); break;
      case 'canc': Exa_Cancel(); break;
      case 'tctl': /* zztimecontrol (msg); */ break;
   }

   if (ExaGame) ExaGame->dirty = false;

   if (debugOn)
   {  DebugWriteNL(">>> REPLY MESSAGE");
      DebugWriteNL(replymsg);
   }
} /* ChessAction */


void CleanExaWindow (void)
{
   if (ExaPGN) delete ExaPGN;
   ExaWindow = nil;
   ExaGame = nil;
   ExaPGN = nil;
} /* CleanExaWindow */


static void RefreshExaWindow (BOOL resetClocks)
{
   ExaWindow->infoAreaView->ResetAnalysis();
   ExaWindow->GameMoveAdjust(true);
   if (resetClocks) ExaWindow->ResetClocks();
   ExaWindow->AdjustFileMenu();
} /* RefreshExaWindow */

/*-------------------------------------------- Quit ----------------------------------------------*/

static void Exa_Quit (void)
{
   Exa_Cancel();
   if (ExaWindow) ExaGame->dirty = false;
   CleanExaWindow();
   sigmaApp->Quit();
} /* Exa_Quit */

/*------------------------------------------ New Game --------------------------------------------*/

static void Exa_NewGame (void)
{
   if (! ExaWindow)
   {  ExaWindow = NewGameWindow("<ExaChess Client> ", false, true);
      if (! ExaWindow) return;
      ExaGame = ExaWindow->game;
      ExaPGN = new CPgn(ExaGame);  // For move parsing
   }
   else
   {  ExaGame->dirty = false;
      ExaGame->NewGame();
      RefreshExaWindow(true);
   }
} /* Exa_NewGame */

/*----------------------------------------- Set Board --------------------------------------------*/

static void Exa_SetBoard (CHAR *msg)
{
   if (! ExaWindow || ExaWindow->thinking) return;

   if (! msg)
      ExaGame->NewGame();
   else
   {
      Board B;
      ParseSetup(&B, msg);

      ExaGame->Init.wasSetup = true;

      // Decode board:
      for (INT xsq = 0; xsq < 64; xsq++)
      {
         SQUARE sq = square(File(xsq), Rank(xsq));
         PIECE p = empty;

         switch (B.theBoard[xsq])
         {
            case WPAWN   : p = wPawn; break;
            case WROOK   : p = wRook; break;
            case WKNIGHT : p = wKnight; break;
            case WBISHOP : p = wBishop; break;
            case WQUEEN  : p = wQueen; break;
            case WKING   : p = wKing; break;

            case BPAWN   : p = bPawn; break;
            case BROOK   : p = bRook; break;
            case BKNIGHT : p = bKnight; break;
            case BBISHOP : p = bBishop; break;
            case BQUEEN  : p = bQueen; break;
            case BKING   : p = bKing; break;
         }

         ExaGame->Init.Board[sq] = p;
      }

      // Decode player, castling rights etc:
      ExaGame->Init.player   = (B.turn == WHITE ? white : black);
      ExaGame->Init.epSquare = (B.enpas <= 0 ? nullSq : square(File(B.enpas), Rank(B.enpas)));
      ExaGame->Init.moveNo   = 1;
      ExaGame->Init.revMoves = 0;
      ExaGame->Init.castlingRights = B.cstat;

      // Finally reset game
      ExaGame->ResetGame();
   }

   RefreshExaWindow(true);
} /* Exa_SetBoard */

/*----------------------------------------- Set Board --------------------------------------------*/

static void Exa_GetBoard (CHAR *replymsg)
{
   Board B;

   // Encode board:
   for (INT xsq = 0; xsq < 64; xsq++)
   {
      SQUARE sq = square(File(xsq), Rank(xsq));
      INT xp;

      switch (ExaGame->Board[sq])
      {
         case wPawn   : xp = WPAWN; break;
         case wRook   : xp = WROOK; break;
         case wKnight : xp = WKNIGHT; break;
         case wBishop : xp = WBISHOP; break;
         case wQueen  : xp = WQUEEN; break;
         case wKing   : xp = WKING; break;

         case bPawn   : xp = BPAWN; break;
         case bRook   : xp = BROOK; break;
         case bKnight : xp = BKNIGHT; break;
         case bBishop : xp = BBISHOP; break;
         case bQueen  : xp = BQUEEN; break;
         case bKing   : xp = BKING; break;
      }

      B.theBoard[xsq] = xp;
   }

   // Encode player, castling rights etc:
   B.turn = (ExaGame->player == white ? WHITE : BLACK);
   if (ExaGame->currMove == 0)
   {  B.enpas = ExaGame->Init.epSquare;
      B.cstat = ExaGame->Init.castlingRights;
   }
   else
   {  B.enpas = -1;
      B.cstat = 0;
   }

   // Finally format setup
   FormatSetup(&B, replymsg);
} /* Exa_GetBoard */

/*---------------------------------------- Play Moves --------------------------------------------*/

static void Exa_PlayMoves (CHAR *msg, CHAR *replymsg)
{
   if (! ExaWindow || ExaWindow->thinking) return;

   // First read initial move number (so we know if we need to undo moves first)

   INT  i;
   CHAR numStr[6];
   LONG moveNo;
   INT  currMove;

   for (i = 0; IsDigit(msg[i]) && i < 5; i++) numStr[i] = msg[i];
   numStr[i] = 0;
   StrToNum(numStr, &moveNo);

   currMove = 2*(moveNo - 1);
   if (msg[i] == 'É') currMove++, i++;

   while (ExaGame->currMove > currMove)
      ExaGame->UndoMove(false);
   ExaGame->CalcMoves();

   msg += i;

   // Then parse and perform the actual move sequence

   for (CHAR *s = msg; *s; )
   {
      CHAR moveStr[20];
      INT len;

      //--- Strip white space, move numbers etc (Note: ExaChess uses digits for Castling) ---
      while (*s && ! IsLetter(*s) && ! (s[0] == '0' && s[1] == '-')) s++;
      if (! *s) goto done;

      for (len = 0; s[len] && s[len] != ' ' && len < 19; len++)
         moveStr[len] = s[len];
      moveStr[len] = 0;
      s += len;
   
      ExaPGN->ReadBegin(s);
      if (! ExaPGN->ParseMove(moveStr)) return;
   }

done:
   RefreshExaWindow(false);
} /* Exa_PlayMoves */

/*----------------------------------------- Take Back --------------------------------------------*/

static void Exa_TakeBack (CHAR *msg)
{
   LONG n;

   if (! ExaWindow || ExaWindow->thinking || ! StrToNum(msg, &n)) return;

   while (n-- > 0 && ExaGame->currMove > 0)
      ExaGame->UndoMove(false);
   ExaGame->CalcMoves();
   RefreshExaWindow(false);
} /* Exa_TakeBack */

/*------------------------------------------ Search ----------------------------------------------*/
// The "msg" field optionally specifies the time control (is only included by ExaChess if the
// time control has been changed by the user:
//
// tc=tmoves/tmin[+incr]  |  tc=[+]movetime  |  <empty string>
//
// tc=15          means 15 seconds per move (Average mode)
// tc=G/5 or G5   means game (all moves) in 5 minutes (Normal mode)
// tc=40/120      means 40 moves in 2 hours (Normal mode)

static void ParseTimeControl (CHAR *tc);
static void CalcSearchResult (CHAR *replymsg);

static void Exa_Search (CHAR *msg, CHAR *replymsg)
{
   if (! ExaWindow || ExaWindow->thinking) return;

   if (msg[0] == 't' && msg[1] == 'c' && msg[2] == '=')
      ParseTimeControl(&msg[3]);

   INT currMove = ExaGame->currMove;
   ExaWindow->Analyze_Go();
   while (ExaWindow && ExaWindow->thinking)
      sigmaApp->MainLooper();

   if (! ExaWindow) return;

   if (ExaGame->currMove == currMove + 1)
      CalcSearchResult(replymsg);
} /* Exa_Search */


static void ParseTimeControl (CHAR *tc)
{
   LEVEL *L = &(ExaWindow->level);
   LONG moves, time, incr;

// L->mode = pmode_Average;           // Default assume average 5 secs per move (if fail parsing)
// L->Average.secs = 5;

   L->mode = pmode_Solver;            // Default assume solver 5 secs per move (if fail parsing)
   L->Solver.timeLimit = 5;
   L->Solver.scoreLimit = maxVal;

   // CASE 1 : "tc=[+]movetime"

   if (StrToNum(tc,&time))            // N seconds per move (e.g. "tc=[+]15")
   {
      if (time <= 0) return;
      L->mode = pmode_Solver;
      L->Solver.timeLimit = time;
//    L->mode = pmode_Average;
//    L->Average.secs = time;
   }

   // CASE 2 : "tc=tmoves/tmin[+incr]"

   else
   {
      // First parse number of moves:

      if (tc[0] == 'G')                    // E.g "tc=G/5" or "tc=G5"
      {
         moves = allMoves;
         tc++;
         if (*tc == '/') tc++;
      }
      else                                 // E.g. "tc=40/120"
      {
         tc += FrontStrNum(tc, &moves);
         if (*tc != '/' || moves <= 0) return;
         tc++;
      }

      // Then parse number of minutes:

      tc += FrontStrNum(tc, &time);
      if (time <= 0) return;
      time *= 60;

      // Then parse optional increment:

      incr = 0;
      if (*tc == '+')
      {
         tc += FrontStrNum(tc, &incr);
         if (incr <= 0) return;
      }

      L->mode = pmode_TimeMoves;
      L->TimeMoves.time  = time;
      L->TimeMoves.moves = moves;
      L->TimeMoves.clockType = (incr == 0 ? clock_Normal : clock_Fischer);
      L->TimeMoves.delta = incr;
   }

   ExaWindow->ResetClocks();
   ExaWindow->boardAreaView->DrawModeIcons();
} /* ParseTimeControl */

// We have to return the entire main line + score, e.g.
// "3...Nc6 {-0.11} 4. Nc3 Nf6 5. Nf3 Bg4 6. Bd3"

static void CalcSearchResult (CHAR *replymsg)
{
   BuildExaChessResult(&ExaWindow->Analysis, replymsg);
} /* CalcSearchResult */

/*---------------------------------------- Cancel Search -----------------------------------------*/

static void Exa_Cancel (void)
{
   if (! ExaWindow || ! ExaWindow->thinking) return;

   ExaWindow->Analyze_Stop();
} /* Exa_Cancel */
