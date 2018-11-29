/**************************************************************************************************/
/*                                                                                                */
/* Module  : GAME.C                                                                               */
/* Purpose : This module implements the game API, where games can be started and where moves can  */
/*           be played, taken back and replayed. All information pertaining to a game (except     */
/*           annotations and other external stuff) is stored in a "CGame" object, which contains  */
/*           the initial board position (incl. castling and en passant rights), the actual moves  */
/*           of the game as well as the current board position and various status flags. Also     */
/*           a GAMEINFO record is included holding information about players, site, event e.t.c.  */
/*           The Sigma Engine takes a "CGame" object as input and starts analysing the current    */
/*           board position.                                                                      */
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

#include "Game.h"
#include "GameUtil.h"
#include "Pgn.h"
#include "Annotations.h"
#include "SigmaPrefs.h"

#include "Board.f"
#include "Move.f"
#include "HashCode.f"

#include "PosLibrary.h"

/*----------------------------------------- Data Structures --------------------------------------*/
// The engine only defines one global variable: The "Global" data structure which contains
// the engine table and various common (read-only) information shared among all engine
// instances.

GLOBAL Global;

// Game specific globals:

static MOVE nullMove = { empty,nullSq,nullSq,empty,-1,0,0,0 };

CHAR PieceCharEng[8] = "-PNBRQK";   // English piece chars (in PGN, EPD and descriptive notation).

BYTE gameData[64000];  // Global utility buffer used when e.g. packing/unpacking games.

static LONG nextGameId = 1;  // Needed for UCI newgame messages


/**************************************************************************************************/
/*                                                                                                */
/*                                         CGAME CONSTRUCTOR                                      */
/*                                                                                                */
/**************************************************************************************************/

//#define gameRecWidth 222  //### Shouldn't be here. Should be set dynamically from the app.
#define gameRecWidth 228  //### Shouldn't be here. Should be set dynamically from the app.

CGame::CGame (void)
{
   for (SQUARE sq = -boardSize1; sq < boardSize2; sq++)       // First initialize the edge entries
      Board[sq] = edge;                                       // and then clear the board.
   ClearTable(Board);

   editingPosition = false;

   annotation = new CAnnotation(gameRecWidth);          // Must be done before calling "NewGame()"

   NewGame();                                           // Finally start new game.
} /* CGame::CGame */


CGame::~CGame (void)
{
   delete annotation;
} /* CGame::~CGame*/


/**************************************************************************************************/
/*                                                                                                */
/*                                             NEW GAME                                           */
/*                                                                                                */
/**************************************************************************************************/

void CGame::NewGame (BOOL resetInfo)           // Initializes a CGame object to a new game.
{
   ResetInit();
   ResetGame(resetInfo);                                // Finally reset game structures.
   dirty = false;                                       // The game is not dirty...
} /* CGame::NewGame */


void CGame::ResetInit (void)                   // Initializes a CGame object to a new game.
{
   ::NewBoard(Init.Board);

   Init.wasSetup       = false;                         // The initial board position was NOT
   Init.player         = white;                         // Set initial player, castling rights etc.
   Init.castlingRights = castRight_WO_O | castRight_WO_O_O | castRight_BO_O | castRight_BO_O_O;
   Init.epSquare       = nullSq;
   Init.moveNo         = 1;
   Init.revMoves       = 0;
} /* CGame::ResetInit */


void CGame::ResetGame (BOOL resetInfo)       // Resets the game record etc. based on the initial
{                                            // position, player, castling rights etc.
   gameId = nextGameId++;

   // Initialize current board position:

   CopyTable(Init.Board, Board);

   player   = Init.player;
   opponent = black - player;
   result   = result_Unknown;

   // Clear game record and draw information:

   currMove = 0;
   lastMove = 0;

   pieceCount = CalcPieceCount(&Global, Board);

   // Initialize HasMovedTo[] table based on initial castling rights:

   ClearTable(HasMovedTo);
   if (! (Init.castlingRights & castRight_WO_O))   HasMovedTo[h1]++;
   if (! (Init.castlingRights & castRight_WO_O_O)) HasMovedTo[a1]++;
   if (! (Init.castlingRights & castRight_BO_O))   HasMovedTo[h8]++;
   if (! (Init.castlingRights & castRight_BO_O_O)) HasMovedTo[a8]++;

   // Store en passant status if any in the game move 0:

   if (Init.epSquare == nullSq)                    // If no enpassant square we should
      Record[0] = nullMove;                        // clear Record[0]...
   else                                            // ...otherwise we should store a
   {  Record[0].piece = pawn + opponent;           // double pawn move in Record[0].
      Record[0].cap   = 0;
      Record[0].from  = Init.epSquare + (player == white ? +0x10 : -0x10);
      Record[0].to    = Init.epSquare + (player == white ? -0x10 : +0x10);
      Record[0].type  = mtype_Normal;
   }

   // Reset misc state information:

   hasResigned    = false;
   hasOfferedDraw = false;

   CalcMoves();                                    // Finally calc all legal moves...
   result = CalcGameResult();                      // and update game result

   if (resetInfo)
   {
      ResetGameInfo();
      if (Info.date[0] == 0)
         GetDateStr(Info.date);
   }

   annotation->ClearAll();

   dirty = false;                                  // The game is not dirty...
} /* CGame::ResetGame */


void CGame::ResetGameInfo (void)                   // Stores default game info
{
   ::ResetGameInfo(&Info);
} /* CGame::ResetGameInfo */


void CGame::ClearGameInfo (void)                   // Clears game info completely
{
   ::ClearGameInfo(&Info);
} /* CGame::ClearGameInfo */


void ResetGameInfo (GAMEINFO *Info)
{
   *Info = Prefs.gameInfo;
} /* ResetGameInfo */


void ClearGameInfo (GAMEINFO *Info)
{
   Info->whiteName[0] = 0;
   Info->blackName[0] = 0;
   Info->event[0]     = 0;
   Info->site[0]      = 0;
   Info->date[0]      = 0;
   Info->round[0]     = 0;
   Info->result       = infoResult_Unknown;

   Info->whiteELO     = -1;
   Info->blackELO     = -1;
   Info->ECO[0]       = 0;
   Info->annotator[0] = 0;

   Info->pageBreak    = false;
   Info->headingType  = headingType_None;
   Info->heading[0]   = 0;
   Info->includeInfo  = true;
} /* ClearGameInfo */


/**************************************************************************************************/
/*                                                                                                */
/*                                          CALC LEGAL GAME MOVES                                 */
/*                                                                                                */
/**************************************************************************************************/

SQUARE KingDir  [9] = { -0x0F, -0x11,  0x11,  0x0F, -0x10, 0x10, 0x01, -0x01, 0 },
       QueenDir [9] = { -0x0F, -0x11,  0x11,  0x0F, -0x10, 0x10, 0x01, -0x01, 0 },
       RookDir  [5] = { -0x10,  0x10,  0x01, -0x01, 0 },
       BishopDir[5] = { -0x0F, -0x11,  0x11,  0x0F, 0 },
       KnightDir[9] = { -0x0E, -0x12, -0x1F, -0x21,  0x12, 0x0E, 0x21,  0x1F, 0 };

// We always make sure that the legal moves of the current game position are available (in the
// Moves[] array). This is done by calling CGame::CalcMoves() whenever the game is reset or
// moves are played/taken back. The move generation is based on the current board position
// (Board[]), the side to move (player) and the castling flags (HasmovedTo[]).

static BOOL SquareAttacked (PIECE Board[], SQUARE sq, COLOUR attacker);

void CGame::CalcMoves (void)
{
   // First compute players king location and check if he is check:

   kingSq = nullSq;
   for (SQUARE sq = a1; sq <= h8 && kingSq == nullSq; sq++)
      if (onBoard(sq) && Board[sq] == king + player)
         kingSq = sq;

   kingInCheck = SquareAttacked(Board, kingSq, opponent);

   // Then compute all strictly legal moves:
   
   moveCount = 0;
   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq))
      {
         PIECE p = Board[sq];
         if (p && pieceColour(p) == player)
            switch (pieceType(p))
            {
               case pawn   : CalcPawnMoves   (sq); break;
               case bishop : CalcQRBMoves    (sq, BishopDir, 4); break;
               case rook   : CalcQRBMoves    (sq, RookDir,   4); break;
               case queen  : CalcQRBMoves    (sq, QueenDir,  8); break;
               case knight : CalcKnightMoves (sq); break;
               case king   : CalcKingMoves   (sq); break;
            }
      }
} /* CGame::CalcMoves */

/*----------------------------------------- Pawn Moves -------------------------------------------*/

void CGame::CalcPawnMoves (SQUARE sq)
{
   MOVE    m = nullMove;
   SQUARE  dir = (player == white ? 0x10 : -0x10);

   m.piece = pawn + player;
   m.from  = sq;

   //--- Single square moves ---

   for (INT df = -1; df <= 1; df++)
   {
      m.to = m.from + dir + df;

      if (onBoard(m.to))
      {
         m.type = mtype_Normal;
         m.cap  = Board[m.to];

         if ((df == 0 && ! m.cap) || (df != 0 && m.cap && pieceColour(m.cap) == opponent))
            if (LegalMove(&m))
               if (rank(m.from) != Global.B.Rank7[player])              // Normal move
                  Moves[moveCount++] = m;
               else
                  for (PIECE p = queen; p >= knight; p--)      // Promotions
                  {  m.type = p + player;
                     Moves[moveCount++] = m;
                  }
      }
   }

   //--- Double square moves ---

   if (rank(m.from) == Global.B.Rank2[player] && ! Board[m.from + dir] && ! Board[m.from + 2*dir])
   {
      m.cap  = empty;
      m.to   = m.from + 2*dir;
      m.type = mtype_Normal;
      if (LegalMove(&m))
         Moves[moveCount++] = m;
   }

   //--- En passant moves ---

   MOVE *pm = &Record[currMove];                  // Previous move.

   if (pieceType(pm->piece) == pawn &&            // If previous move was a double pawn move
       Abs(m.from - pm->to) == 1 &&               // to the square next to our pawn, then
       Abs(pm->from - pm->to) == 0x20)            // generate en passant.
   {
      m.cap  = empty;
      m.to   = pm->to + dir;
      m.type = mtype_EP;

      Board[pm->to] = empty;                      // Remove opponent pawn temporarily (in order
      if (LegalMove(&m))                          // to check move legality.
         Moves[moveCount++] = m;
      Board[pm->to] = pm->piece;                  // Replace opponent pawn.
   }
}   /* CGame::CalcPawnMoves */

/*---------------------------------------- Knight Moves ------------------------------------------*/

void CGame::CalcKnightMoves (SQUARE sq)
{
   MOVE m = nullMove;

   m.piece = Board[sq];
   m.from  = sq;
   m.type  = mtype_Normal;

   for (INT i = 0; i <= 7; i++)
   {
      m.to = m.from + KnightDir[i];
      if (onBoard(m.to))
      {
         m.cap = Board[m.to];
         if ((! m.cap || pieceColour(m.cap) == opponent) && LegalMove(&m))
            Moves[moveCount++] = m;
      }
   }
}   /* CGame::CalcKnightMoves */

/*----------------------------------- Queen/Rook/Bishop Moves ------------------------------------*/

void CGame::CalcQRBMoves (SQUARE sq, const SQUARE Dir[], INT dirCount)
{
   MOVE m = nullMove;

   m.piece = Board[sq];
   m.from  = sq;
   m.type  = mtype_Normal;

   for (INT i = 0; i < dirCount; i++)
   {
      SQUARE dir = Dir[i];

      // Generate non-captures:

      m.cap = empty;
      for (m.to = m.from + dir; onBoard(m.to) && Board[m.to] == empty; m.to += dir)
         if (LegalMove(&m))
            Moves[moveCount++] = m;

      // Generate captures:

      m.cap = Board[m.to];
      if (m.cap && onBoard(m.to) && pieceColour(m.cap) == opponent)
         if (LegalMove(&m))
            Moves[moveCount++] = m;
   }
}   /* CGame::CalcQRBmoves */

/*------------------------------------------ King Moves ------------------------------------------*/

void CGame::CalcKingMoves (SQUARE sq)
{
   MOVE  m = nullMove;

   m.piece = Board[sq];
   m.from  = sq;
   m.type  = mtype_Normal;

   // Generate normal king moves:

   for (INT i = 0; i <= 7; i++)
   {
      m.to = kingSq = sq + KingDir[i];
      if (onBoard(m.to))
      {   m.cap = Board[m.to];
         if ((! m.cap || pieceColour(m.cap) == opponent) && LegalMove(&m))
            Moves[moveCount++] = m;
      }
   }

   kingSq = sq;

   // Generate castling:

   if (player == white)
   {
      if (sq == e1 && ! HasMovedTo[e1])
         CalcCastling(mtype_O_O),
         CalcCastling(mtype_O_O_O);
   }
   else
   {
      if (sq == e8 && ! HasMovedTo[e8])
         CalcCastling(mtype_O_O),
         CalcCastling(mtype_O_O_O);
   }
}   /* CGame::CalcKingMoves */


void CGame::CalcCastling (INT type)               // Generates a single castling move (if legal).
{
   MOVE   m = nullMove;
   SQUARE midSq, rookSq;

   m.from  = kingSq;

   if (type == mtype_O_O)
      midSq    = right(m.from),
      m.to     = right(midSq),
      rookSq   = right(m.to);
   else
      midSq    = left(m.from),
      m.to     = left(midSq),
      rookSq = left2(m.to);

   if (Board[rookSq] != rook + player) return;            // Rook may not have moved.
   if (HasMovedTo[rookSq]) return;

   if (Board[midSq] || Board[m.to]) return;                  // Intervening squares must be
   if (type == mtype_O_O_O && Board[right(rookSq)]) return;  // empty.

   if (SquareAttacked(Board, m.from, opponent)) return;   // King may not be check and
   if (SquareAttacked(Board, midSq,  opponent)) return;   // neighbour squares may not
   if (SquareAttacked(Board, m.to,   opponent)) return;   // be attacked by opponent.

   m.piece = king + player;
   m.cap   = empty;
   m.type  = type;
   Moves[moveCount++] = m;
}   /* CGame::CalcCastling */

/*-------------------------------------------- Utility -------------------------------------------*/
// A pseudo-legal move is strictly legal if it doesn't leave the player's king in check. Does not
// check legality of castling and En Passant moves.

BOOL CGame::LegalMove (MOVE *m)
{
   BOOL legal;

   Board[m->from] = empty;
   Board[m->to]   = m->piece;
   legal = ! SquareAttacked(Board, kingSq, opponent);
   Board[m->from] = m->piece;
   Board[m->to]   = m->cap;
   return legal;
}   /* CGame::LegalMove */


static BOOL SquareAttacked (PIECE Board[], SQUARE sq, COLOUR attacker)
{
   SQUARE   asq, dir;      // Attacker source square and direction.
   PIECE    p;

   for (INT i = 0; i <= 3; i++)                              // Check queen/rook/bishop attack.
   {
      dir = RookDir[i];
      for (asq = sq + dir; onBoard(asq) && Board[asq] == empty; asq += dir);
      if (onBoard(asq))
      {   p = Board[asq];
         if (p == attacker + rook || p == attacker + queen)   return true;
      }

      dir = BishopDir[i];
      for (asq = sq + dir; onBoard(asq) && Board[asq] == empty; asq += dir);
      if (onBoard(asq))
      {   p = Board[asq];
         if (p == attacker + bishop || p == attacker + queen) return true;
      }
   }

   for (INT i = 0; i <= 7; i++)                              // Check knight/king attack.
   {
      asq = sq + KnightDir[i];
      if (onBoard(asq) && Board[asq] == attacker + knight) return true;

      asq = sq + KingDir[i];
      if (onBoard(asq) && Board[asq] == attacker + king) return true;
   }

   asq = left(sq) + (attacker == black ? 0x10 : -0x10);      // Check pawn attack:
   if (onBoard(asq) && Board[asq] == attacker + pawn)
      return true;
   asq = right2(asq);
   if (onBoard(asq) && Board[asq] == attacker + pawn)
      return true;

   return false;
}   /* SquareAttacked */


/**************************************************************************************************/
/*                                                                                                */
/*                                         PLAY NEW GAME MOVE                                     */
/*                                                                                                */
/**************************************************************************************************/

// PlayMove(game, m) plays the move "m" in the game, by adding the move to the game record and
// by updating various internal structures in the game object. The move "m" should be a member
// of the Moves[] list of strictly legal moves in the current game position.

void CGame::PlayMove (MOVE *m)
{
   // Add the new move to the game record:
   lastMove = currMove + 1;
   Record[lastMove] = *m;
   ClrAnnotation(lastMove);
   // Compute file/rank disambiguation flags for the move:
   CalcDisambFlags(&Record[lastMove], Moves, moveCount);
   // Then "replay" it and calc legal moves:
   RedoMove(true);
   // Compute check/mate flags for the move:
   CalcCheckFlags();
   // Finally update game "result" field and set dirty flag:
   result = CalcGameResult();
   dirty  = true;
} /* CGame::PlayMove */


void CGame::PlayMoveRaw (MOVE *m)  // Only used by collection filter where only the moves are needed
{
   // Add the new move to the game record:
   lastMove = currMove + 1;
   Record[lastMove] = *m;
   // Clear file/rank disambiguation flags and glyph for the move:
   Record[lastMove].flags = 0;
   Record[lastMove].misc = 0;
   // Then "replay" it but don't calc legal moves or draw info:
   RedoMove(false);
} /* CGame::PlayMoveRaw */


/**************************************************************************************************/
/*                                                                                                */
/*                                       UNDO/REDO GAME MOVES                                     */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------- Undo Current Half Move -------------------------------------*/

void CGame::UndoMove (BOOL refresh)             // Takes back current game move.
{
   MOVE *m;

   if (currMove == 0) return;

   m = &Record[currMove--];
   Swap(&player, &opponent);                              // Change sides.

   switch (m->type)                                       // Retract move on the board.
   {  case mtype_Normal : HasMovedTo[m->to]--; break;
      case mtype_O_O    : Board[left(m->to)]  = empty;
                          Board[right(m->to)] = player + rook; break;
      case mtype_O_O_O  : Board[right(m->to)] = empty;
                          Board[left2(m->to)] = player + rook; break;
      case mtype_EP     : Board[m->to + 2*player - 0x10] = pawn + opponent;
                          pieceCount += Global.B.PieceCountBit[pawn + opponent]; break;
      default           : pieceCount -= Global.B.PieceCountBit[m->type] - Global.B.PieceCountBit[m->piece];
   }

   Board[m->from] = m->piece;
   Board[m->to]   = m->cap;
   if (m->cap)
      pieceCount += Global.B.PieceCountBit[m->cap];

   hasResigned    = false;
   hasOfferedDraw = false;

   if (refresh)
      CalcMoves();
} /* CGame::UndoMove */

/*------------------------------------ Redo Next Half Move ---------------------------------------*/

void CGame::RedoMove (BOOL refresh)                   // Replays next game move.
{
   MOVE *m;

   if (currMove == lastMove) return;

   m = &Record[++currMove];

   Board[m->from] = empty;                           // Perform move on the board and
   Board[m->to]   = m->piece;                        // update "pieceCount" (for use by
   if (m->cap)                                       // the draw checking).
      pieceCount -= Global.B.PieceCountBit[m->cap];

   switch (m->type)
   {
      case mtype_Normal : HasMovedTo[m->to]++; break;
      case mtype_O_O    : Board[right(m->to)] = empty;
                          Board[left(m->to)]  = player + rook; break;
      case mtype_O_O_O  : Board[left2(m->to)] = empty;
                          Board[right(m->to)] = player + rook; break;
      case mtype_EP     : Board[m->to + 2*player - 0x10] = empty;
                          pieceCount -= Global.B.PieceCountBit[pawn + opponent]; break;
      default           : Board[m->to] = m->type;
                          pieceCount += Global.B.PieceCountBit[m->type] - Global.B.PieceCountBit[m->piece];
   }

   Swap(&player, &opponent);                        // Change sides.

   if (refresh)
      CalcMoves();
} /* CGame::RedoMove */

/*-------------------------------------- Undo All Moves ------------------------------------------*/

void CGame::UndoAllMoves (void)
{
   if (currMove == 0 || editingPosition) return;
   while (currMove > 0)
      UndoMove(false);
   CalcMoves();
} /* CGame::UndoAllMoves */

/*-------------------------------------- Redo All Moves ------------------------------------------*/

void CGame::RedoAllMoves (void)
{
   if (currMove == lastMove || editingPosition) return;
   while (currMove < lastMove)
      RedoMove(false);
   CalcMoves();
} /* CGame::RedoAllMoves */

/*----------------------------------------- Utility ----------------------------------------------*/

BOOL CGame::CanUndoMove (void)
{
   return (currMove > 0 && ! editingPosition);
} /* CGame::CanUndoMove */


BOOL CGame::CanRedoMove (void)
{
   return (currMove < lastMove && ! editingPosition);
} /* CGame::CanRedoMove */


INT CGame::MovesPlayed (void)      // Number of moves played by side to move
{
   return (currMove + 1)/2;
} /* CGame::MovesPlayed */


INT CGame::GetMoveNo (void)
{
   return Init.moveNo + (currMove - (Init.player == white ? 1 : 0))/2;
} /* CGame::GetMoveNo */


MOVE* CGame::GetGameMove (INT j)
{
   return (j >= 1 && j <= lastMove ? &Record[j] : nil);
} /* CGame::GetGameMove */


/**************************************************************************************************/
/*                                                                                                */
/*                                            COPY GAME                                           */
/*                                                                                                */
/**************************************************************************************************/

void CGame::CopyFrom (CGame *src, BOOL allmoves, BOOL includeinfo, BOOL includeann)
{
   ResetInit();

   Init = src->Init;
   if (includeinfo) Info = src->Info;
   ResetGame(false);

   for (INT j = 0; j <= (allmoves ? src->lastMove : src->currMove); j++)
   {
      if (j >= 1) PlayMove(&(src->Record[j]));

      if (includeann && src->ExistAnnotation(j))
      {
         INT charCount;
         src->GetAnnotation(j, (CHAR*)gameData, &charCount);
         SetAnnotation(j, (CHAR*)gameData, charCount, false);
      }
   }

   dirty = false;
} /* CGame::CopyFrom */


/**************************************************************************************************/
/*                                                                                                */
/*                                       UPDATE GAME RESULT                                       */
/*                                                                                                */
/**************************************************************************************************/

// Whenever a game move has been played (CGame::PlayMove), we must update the game "result", i.e.
// check if the game is over due to mate, stale mate or one of the draw rules. This check also
// updates the "DrawData[]" entry for the current position.

static BOOL VerifyRepetition (void);

INT CGame::CalcGameResult (void)
{
   MOVE      *m = &Record[currMove];
   DRAWDATA  *D = &DrawData[currMove];
   INT       revCount;

   // First initialize the DrawData[] for the current position:

   if (currMove == 0)
   {
      D->hashKey  = CalcHashKey(&Global, Board);      // Reset draw information.
      D->irr      = -Init.revMoves;
      D->repCount = 0;
   }
   else
   {
      D->hashKey  = (D-1)->hashKey ^ HashKeyChange(&Global, m);   // Inititialize draw information
      D->irr      = 0;
      D->repCount = 0;
   }

   // If there are no legal moves we check if it's either mate or stale mate (and skip
   // subsequent draw checking):

   if (moveCount == 0)
      return (kingInCheck ? result_Mate : result_StaleMate);

   // First we check draw by insufficient material (at most one knight/bishop):

   ULONG p = pieceCount & 0xFF0FFF0F;
   if (p == 0 || p == 0x00000100 || p == 0x01000000)
      return result_DrawInsMtrl;

   // If we are at initial position, none of the other draw rules apply and we can
   // skip any further checks:

   if (currMove == 0)
      return result_Unknown;

   // If an irreversible move (capture/pawn move/castling) has just been made, the game is not
   // drawn:

   if (m->cap || m->type != mtype_Normal || pieceType(m->piece) == pawn)
   {
      D->irr = currMove;
      return result_Unknown;
   }

   // If the last game move was NOT irreversible, we check the the 50 move rule and
   // draw by repetition:

   D->irr   = (D-1)->irr;                     // Copy previous "irr" value.
   revCount = currMove - D->irr;              // Half moves since last irr move (> 0)

   if (revCount >= 100)                       // Check 50 move rule:
      return result_Draw50;
   
   // Next check draw by repetition. This is done by looking back through the game record (in
   // the draw information) at every second position and comparing hash keys:

   for (INT n = 4; n <= revCount; n += 2)
      if (D->hashKey == (D-n)->hashKey && VerifyRepetition(n))    // Position found!
      {
         D->repCount = (D-n)->repCount + 1;      // Increase repetition count.
         if (D->repCount == 2)                   // If position occured 2 times earlier, it's a
            return result_Draw3rd;               // draw by repetition.
      }

   // Finally return "result unknown" if no draw:

   return result_Unknown;
} /* CGame::UpdateGameResult */


BOOL CGame::VerifyRepetition(INT n)     // Verifies draw by repetition (since hash keys are not
{                                       // 100% reliable) by tracing back reversible moves to
   PIECE B[boardSize];                  // the position with the identical hash key.

   CopyTable(Board, B);

   for (INT i = currMove; i > currMove - n; i--)
   {  B[Record[i].from] = Record[i].piece;         // Take back reversible move.
      B[Record[i].to  ] = empty;
   }

   return EqualTable(Board, B);
}   /* CGame::VerifyRepetition */


BOOL CGame::GameOver (void)
{
   return (result != result_Unknown && currMove == lastMove);
} /* CGame::GameOver */


BOOL CGame::UpdateInfoResult (void)
{
   if (currMove < lastMove) return false;

   if (! GameOver())
      Info.result = infoResult_Unknown;
   else if (result == result_Mate)
      Info.result = (player == black ? infoResult_WhiteWin : infoResult_BlackWin);
   else
      Info.result = infoResult_Draw;
   return true;
} /* CGame::UpdateInfoResult */


void CGame::CalcStatusStr (CHAR *str)   // Max length 100 chars
{
   CHAR plStr[10];
   CopyStr((player == white ? "White" : "Black"), plStr);

   if (! GameOver())
      Format(str, "%s to move", plStr);
   else
	   switch (result)
	   {
	      case result_Mate        : Format(str, "%s is checkmated!", plStr); break;
	      case result_StaleMate   : Format(str, "%s is stalemated!", plStr); break;
	      case result_Draw3rd     : Format(str, "Draw by repetition!"); break;
	      case result_Draw50      : Format(str, "Draw by the 50 move rule!"); break;
	      case result_DrawInsMtrl : Format(str, "Draw - insufficient material!"); break;

	      case result_DrawAgreed  : Format(str, "Draw agreed!"); break;
	      case result_Resigned    : Format(str, "%s resigned!", plStr); break;
	      case result_TimeForfeit : Format(str, "%s lost on time!", plStr); break;

	      default                 : Format(str, "%s to move", plStr); break;
	   }
} /* CGame::CalcStatusStr */

/*
void CGame::CalcLibStr (CHAR *eco, CHAR *comment)
{
   CHAR eco[libECOLength + 1], comment[libCommentLength + 1];

   if (! PosLib_ProbeStr(player, Board, eco, comment))
      Format(str, "%s to move", plStr);
   else if (EqualStr("", eco))
      Format(str, "%s to move [%s]", plStr, comment);
   else if (EqualStr("", comment))
      Format(str, "%s to move [%s]", plStr, eco);
   else
      Format(str, "%s to move [%s - %s]", plStr, eco, comment);
} /* CGame::CalcLibStr */


/**************************************************************************************************/
/*                                                                                                */
/*                                     CURRENT POSITION ACCESS                                    */
/*                                                                                                */
/**************************************************************************************************/

/*
PIECE CGame::GetPiece (SQUARE sq)
{
   return (editingPosition ? Init.Board[sq] : Board[sq]);
}   /* CGame::ClearPiece */

/*
COLOUR CGame::GetPlayer (void)
{
   return (editingPosition ? Init.player : player);
}   /* CGame::GetPlayer */


INT CGame::GetBoardMoveCount (void)
{
   return moveCount;
} /* CGame::GetBoardMoveCount */


MOVE *CGame::GetBoardMove(INT i)
{
   return &Moves[i];
} /* CGame::GetBoardMove */


/**************************************************************************************************/
/*                                                                                                */
/*                                          EDITING POSITIONS                                     */
/*                                                                                                */
/**************************************************************************************************/

// The Position Editor manipulates the following parts of the CGame object:
//
// 1. The "Init" record.
// 2. The "player" and "opponent".
// 3. The "Board[]"
//
// At any time, "Init.player" = "player" and "Init.Board" = "Board" provided that only the
// "Edit_" methods are used when editing a position.

/*------------------------------------ Begin/End Position Editing --------------------------------*/

static INT MaxCastlingRights (PIECE *Board, INT *HasMovedTo);

void CGame::Edit_Begin (void)
{
   editingPosition = true;

   editPiece       = wKing;
   InitBackup      = Init;
   playerBackup    = player;
   CopyTable(Board, BoardBackup);

   // Initialize "Initial position status":
   CopyTable(Board, Init.Board);
   Init.wasSetup   = true;

   if (currMove > 0)
   {
      Init.castlingRights = MaxCastlingRights(Board, HasMovedTo);
      Init.moveNo         = 1; //Init.moveNo + (Init.player == white ? currMove : currMove + 1)/2;
      Init.revMoves       = currMove - DrawData[currMove].irr;
      
      MOVE m = Record[currMove];
      if (pieceType(m.piece) == pawn && Abs(m.from - m.to) == 0x20)
         Init.epSquare = (m.to + m.from)/2;
      else
         Init.epSquare = nullSq;
   }

   Init.player = player;  // Finally set initial player (must be done after Init.moveNo is
                          // computed.
} /* Game_BeginEditPos */


void CGame::Edit_End (BOOL confirmed)
{
   if (confirmed)
   {
      // Verify castling rights:
      Init.castlingRights &= MaxCastlingRights(Init.Board, nil);

      // Verify EP Square:
      if (Board[Init.epSquare + (player == white ? -0x10 : 0x10)] != pawn + opponent)
         Init.epSquare = nullSq;

      // Finally reset game
      ResetGame(false);
      dirty = true;
   }
   else
   {  Init = InitBackup;
      player = playerBackup;
      opponent = black - player;
      CopyTable(BoardBackup, Board);
   }

   editingPosition = false;
} /* CGame::EndEditPos */


static INT MaxCastlingRights (PIECE *Board, INT HasMovedTo[])
{
   INT maxRights = 0;

   if (Board[e1] == wKing && ! (HasMovedTo && HasMovedTo[e1]))
   {  if (Board[h1] == wRook && ! (HasMovedTo && HasMovedTo[h1])) maxRights |= castRight_WO_O;
      if (Board[a1] == wRook && ! (HasMovedTo && HasMovedTo[a1])) maxRights |= castRight_WO_O_O;
   }
   if (Board[e8] == bKing)
   {  if (Board[h8] == bRook && ! (HasMovedTo && HasMovedTo[h8])) maxRights |= castRight_BO_O;
      if (Board[a8] == bRook && ! (HasMovedTo && HasMovedTo[a8])) maxRights |= castRight_BO_O_O;
   }
   return maxRights;
} /* MaxCastlingRights */

/*------------------------------------ Setting/Removing Pieces -----------------------------------*/

void CGame::Edit_ClearBoard (void)
{
   ClearTable(Board);
   ClearTable(Init.Board);

   Init.castlingRights = 0;
   Init.epSquare       = nullSq;
   Init.moveNo         = 1;
   Init.revMoves       = 0;
} /* CGame::Edit_ClearBoard */


void CGame::Edit_NewBoard (void)
{
   NewBoard(Board);
   NewBoard(Init.Board);
} /* CGame::Edit_NewBoard */


void CGame::Edit_ClearPiece (SQUARE sq)
{
   Edit_SetPiece(sq, empty);
} /* CGame::Edit_ClearPiece */


void CGame::Edit_SetPiece (SQUARE sq, PIECE p)
{
   if (offBoard(sq)) return;
   Board[sq] = Init.Board[sq] = p;
} /* CGame::Edit_SetPiece */


void CGame::Edit_MovePiece (SQUARE from, SQUARE to)
{
   if (offBoard(from) || offBoard(to)) return;
   Board[to]   = Init.Board[to]   = Board[from];
   Board[from] = Init.Board[from] = empty;
} /* CGame::Edit_MovePiece */

/*---------------------------------- Setting Player (Side to Move) -------------------------------*/

void CGame::Edit_SetPlayer (COLOUR thePlayer)
{
   player = Init.player = thePlayer;
   opponent = black - player;
} /* CGame::Edit_SetPlayer */

/*-------------------------------- Set Initial Game/Position Status-------------------------------*/
/*
void CGame::Edit_SetInitState (INT castlingRights, INT epSquare, INT moveNo)
{
   Init.castlingRights = castlingRights;
   Init.epSquare       = epSquare;
   Init.moveNo         = moveNo;
} /* CGame::Edit_SetInitState */

/*------------------------------------- Check Legal Position -------------------------------------*/
// Checks legality of the specified position. Returns false if there are any pawns on the 1st or 8th
// rank or if there are an illegal number of pieces. Otherwise true is returned.

INT CGame::Edit_CheckLegalPosition (void)
{
   return ::CheckLegalPosition(Board, player);
} /* CGame::Edit_CheckLegalPosition */


INT CheckLegalPosition (PIECE Board[], COLOUR player)
{
   PIECE    p;
   SQUARE   sq, ksq;
   INT      Count[pieces],
            whiteProms, blackProms;

   for (p = wPawn; p <= bKing; p++)                    // Count number of pieces of each type.
      Count[p] = 0;
   for (sq = a1; sq <= h8; sq++)
      if (onBoard(sq))
      {   Count[Board[sq]]++;
         if (Board[sq] == king + (black - player))
            ksq = sq;
      }

   whiteProms = 0;                                     // Count number of promoted white pieces.
   for (p = wKnight; p <= wRook; p++)
      if (Count[p] > 2) whiteProms += Count[p] - 2;
   if (Count[wQueen] > 1) whiteProms += Count[wQueen] - 1;

   blackProms = 0;                                     // Count number of promoted black pieces.
   for (p = bKnight; p <= bRook; p++)
      if (Count[p] > 2) blackProms += Count[p] - 2;
   if (Count[bQueen] > 1) blackProms += Count[bQueen] - 1;

   if (Count[wKing] > 1 ) return pos_TooManyWhiteKings;
   if (Count[bKing] > 1 ) return pos_TooManyBlackKings;
   if (Count[wPawn] > 8 ) return pos_TooManyWhitePawns;
   if (Count[bPawn] > 8 ) return pos_TooManyBlackPawns;
   if (whiteProms > 8 - Count[wPawn]) return pos_TooManyWhiteOfficers;
   if (blackProms > 8 - Count[bPawn]) return pos_TooManyBlackOfficers;

   for (sq = a1; sq <= h1; sq++)                       // Return false, if there are any pawns
      if (pieceType(Board[sq]) == pawn)                // on the first or eighth rank.
         return pos_PawnsOn1stRank;
   for (sq = a8; sq <= h8; sq++)
      if (pieceType(Board[sq]) == pawn)
         return pos_PawnsOn1stRank;

   // Finally check king counts and that the opponent is not be check.
   // NOTE: These checks should be done last because they are not needed by the collection
   // position filter.
   if (Count[wKing] == 0) return pos_WhiteKingMissing;
   if (Count[bKing] == 0) return pos_BlackKingMissing;

   return (SquareAttacked(Board, ksq, player) ? pos_OpponentInCheck : pos_Legal);
} /* CheckLegalPosition */


/**************************************************************************************************/
/*                                                                                                */
/*                                           ANNOTATIONS                                          */
/*                                                                                                */
/**************************************************************************************************/
/*
void CGame::InitAnnotation (void)
{
} /* CGame::InitAnnotation */

/*--------------------------------------- Annotation Glyphs --------------------------------------*/

void CGame::SetAnnotationGlyph (INT moveNo, INT glyph)
{
   Record[moveNo].misc = glyph;
   dirty = true;
} /* CGame::SetAnnotationGlyph */


INT CGame::GetAnnotationGlyph (INT moveNo)
{
   return ((UINT)(Record[moveNo].misc)) & 0x00FF;
} /* CGame::SetAnnotationGlyph */

/*---------------------------------------- Annotation Text ---------------------------------------*/

void CGame::SetAnnotation (INT moveNo, CHAR *text, INT charCount, BOOL killNewLines)
{
   annotation->Set(moveNo, text, charCount, true, killNewLines);
   dirty = true;
} /* CGame::SetAnnotation */


void CGame::ClrAnnotation (void)
{
   annotation->ClearAll();
   dirty = true;
} /* CGame::ClrAnnotation */


void CGame::ClrAnnotation (INT moveNo)
{
   annotation->Clear(moveNo);
   dirty = true;
} /* CGame::ClrAnnotation */


void CGame::GetAnnotation (INT moveNo, CHAR *text, INT *charCount)
{
   annotation->GetText(moveNo, text, charCount);
} /* CGame::GetAnnotation */


INT CGame::GetAnnotationLine (INT moveNo, INT lineNo, CHAR *Text, BOOL *nl)
{
   return annotation->GetTextLine(moveNo, lineNo, Text, nl);
} /* CGame::GetAnnotationLine */


INT CGame::GetAnnotationLineCount (INT moveNo)
{
   return annotation->GetLineCount(moveNo);
} /* CGame::GetAnnotationLineCount */


BOOL CGame::ExistAnnotation (INT moveNo)
{
   return annotation->Exists(moveNo);
} /* CGame::ExistAnnotation */

/*-------------------------------- Compute Move Check/Mate Flags ---------------------------------*/

void CGame::CalcCheckFlags (void)                   // This routine sets the check/mate flags.
{                                                   // Must be done AFTER CalcMoves().
   MOVE *m = &Record[lastMove];

   if (kingInCheck)
   {  m->flags |= moveFlag_Check;
      if (moveCount == 0)
         m->flags |= moveFlag_Mate;
   }
} /* CGame::CalcCheckFlags */


/**************************************************************************************************/
/*                                                                                                */
/*                                              GAME MAP                                          */
/*                                                                                                */
/**************************************************************************************************/

INT CGame::CalcGameMap (INT toMove, GAMEMAP GMap[], BOOL isPrinting, BOOL isCollectionGame, BOOL isPublishing)
{
   INT N = InsertGameMapHeader(GMap, isPrinting, isCollectionGame, isPublishing);

   if (ExistAnnotation(0))
      InsertAnnGameMap(0, &N, GMap);

   for (INT j = 1; j <= toMove; j++)
   {
      GAMEMAP *gm = &GMap[N++];
      gm->moveNo = j;

      if (pieceColour(Record[j].piece) == white)
      {
         gm->moveNo |= gameMap_White;
         if (ExistAnnotation(j))
            InsertAnnGameMap(j, &N, GMap);
         else if (j < toMove)
         {  gm->moveNo |= gameMap_Black;
            j++;
            if (ExistAnnotation(j))
               InsertAnnGameMap(j, &N, GMap);
         }
      }
      else
      {
         gm->moveNo |= gameMap_Black;
         if (ExistAnnotation(j))
            InsertAnnGameMap(j, &N, GMap);         
      }
   }

   return N;
} /* CGame::CalcGameMap */


void CGame::InsertAnnGameMap (INT j, INT *N, GAMEMAP GMap[])
{
   INT lineCount = GetAnnotationLineCount(j);

   for (INT n = 0; n < lineCount; n++)
   {  GMap[*N].moveNo = j;
      GMap[*N].txLine = n;
      (*N)++;
   }
} /* CGame::InsertAnnGameMap */

/*------------------------------------ Compute Game Data Map -------------------------------------*/

static void SetGameMapEntry (GAMEMAP GMap[], INT i, INT moveNo, INT txLine);

INT CGame::InsertGameMapHeader (GAMEMAP GMap[], BOOL isPrinting, BOOL isCollectionGame, BOOL isPublishing)
{
   INT i = 0;

   // If the current game is a collection game and one of the three title flags have been set
   // we have to include the collection game title in the game map:

   if (isPublishing && Info.headingType != headingType_None)
   {
      INT type;
      switch (Info.headingType)
      {
         case headingType_Chapter : type = gameMap_SpecialChapter; break;
         case headingType_Section : type = gameMap_SpecialSection; break;
         case headingType_GameNo  : type = gameMap_SpecialGmTitle; break;  // Default when importing PGN
      }

      SetGameMapEntry(GMap, i++, gameMap_Special, type);
      SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialBlank);
   }

   // Include selected parts of game info:

   GAMEINFOFILTER *Filter = &(Prefs.GameDisplay.gameInfoFilter);

   if ((! isPublishing && isPrinting) || (isPublishing && Info.includeInfo))
   {
      INT i0 = i;

      if (isCollectionGame && ! isPublishing)
      {  SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialGmTitle);
         SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialBlank);
      }
      if (Filter->players) SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialWhite);
      if (Filter->players) SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialBlack);
      if (Filter->event)   SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialEvent);
      if (Filter->site)    SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialSite);
      if (Filter->date)    SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialDate);
      if (Filter->round)   SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialRound);
      if (Filter->result)  SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialResult);
      if (Filter->ECO)     SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialECO);

      if (i > i0) SetGameMapEntry(GMap, i++, gameMap_Special, gameMap_SpecialBlank);
   }

   return i;
} /* CGame::InsertGameMapHeader */


static void SetGameMapEntry (GAMEMAP GMap[], INT i, INT moveNo, INT txLine)
{
   GMap[i].moveNo = moveNo;
   GMap[i].txLine = txLine;
} /* SetGameMapEntry */


BOOL CGame::GameMapContainsDiagram (GAMEMAP GMap[], INT N)
{
   INT j = GMap[N].moveNo & 0x0FFF;
   if (j != GMap[N].moveNo) return false;

   CHAR s[500];
   INT lineNo  = GMap[N].txLine & 0x0FFF;
   INT lineLen = GetAnnotationLine(j, lineNo, s);
   s[lineLen] = 0;
   return SameStr(s, "[DIAGRAM]");
} /* CGame::GameMapContainsDiagram */


/**************************************************************************************************/
/*                                                                                                */
/*                                       START UP INITIALIZATION                                  */
/*                                                                                                */
/**************************************************************************************************/

void InitGameModule (void)
{
   SetGameNotation("PNBRQK", moveNot_Short);
   InitGameFile5();
} /* InitGameModule */
