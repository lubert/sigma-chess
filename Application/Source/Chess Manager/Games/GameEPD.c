/**************************************************************************************************/
/*                                                                                                */
/* Module  : GameEPD.c                                                                            */
/* Purpose : This module implements save/load of positions in the EPD format.                     */
/*                                                                                                */
/**************************************************************************************************/

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

#include "Game.h"
#include "Board.f"
#include "Pgn.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                        WRITE EPD STRING                                        */
/*                                                                                                */
/**************************************************************************************************/

// Copies the current board position in to a string in EPD format (which can then e.g. be
// copied to the clipboard).

INT CGame::Write_EPD (CHAR *s)
{
   INT n = 0;

   //--- Write the position field ---

   for (INT rank = 7; rank >= 0; rank--)
   {
      INT emptyCount = 0;

      for (INT file = 0; file <= 7; file++)
      {
         PIECE p = Board[(rank << 4) + file];
         if (! p)
            emptyCount++;
         else
         {   if (file > 0 && emptyCount > 0)
               s[n++] = emptyCount + '0',
               emptyCount = 0;
            CHAR c = PieceCharEng[pieceType(p)];
            if (pieceColour(p) == black) c = (c - 'A') + 'a';
            s[n++] = c;
         }
      }
      if (emptyCount > 0) s[n++] = emptyCount + '0';
      if (rank > 0) s[n++] = '/';
   }
   s[n++] = ' ';

   //--- Write initial player field ---

   s[n++] = (player == white ? 'w' : 'b');
   s[n++] = ' ';

   //--- Write castling rights field ---

   INT ncast = n;

   if (Board[e1] == wKing && ! HasMovedTo[e1])    // White castling rights
   {
      if (Board[h1] == wRook && ! HasMovedTo[h1]) s[n++] = 'K';
      if (Board[a1] == wRook && ! HasMovedTo[a1]) s[n++] = 'Q';
   }
   if (Board[e8] == bKing && ! HasMovedTo[e8])    // Black castling rights
   {
      if (Board[h8] == bRook && ! HasMovedTo[h8]) s[n++] = 'k';
      if (Board[a8] == bRook && ! HasMovedTo[a8]) s[n++] = 'q';
   }
   if (ncast == n) s[n++] = '-';                // If no rights, simple write '-'
   s[n++] = ' ';

   //--- Write en passant field ---

   MOVE *m = &Record[currMove];
   INT  mdir = (opponent == white ? 0x10 : -0x10);

   if (m->piece == pawn + opponent && m->to - m->from == 2*mdir)
      s[n++] = file(m->from + mdir) + 'a',
      s[n++] = rank(m->from + mdir) + '1';
   else
      s[n++] = '-';
   s[n++] = ' ';

   //--- Write reversable moves field ---

	CHAR irrStr[10];
	NumToStr(currMove - DrawData[currMove].irr, irrStr);
	CopyStr(irrStr, &s[n]);
	n += StrLen(irrStr);
   s[n++] = ' ';

   //--- Write initial move no field ---

	CHAR moveNoStr[10];
	NumToStr(GetMoveNo(), moveNoStr);
	CopyStr(moveNoStr, &s[n]);
	n += StrLen(moveNoStr);

   //--- Finally null-terminate ---
   s[n] = 0;
   return n;

}   /* CGame::Write_EPD */


/**************************************************************************************************/
/*                                                                                                */
/*                                         READ EPD STRING                                        */
/*                                                                                                */
/**************************************************************************************************/

// Parses the input EPD string and (if it's legal) stores the specified position in the game.
// Can be used after the user has pasted an EPD position to the clipboard. Returns a result
// code (which is <> 0 if a parse error occured). The string is assumed to be null terminated.

INT CGame::Read_EPD (CHAR *s)
{
   INITGAME EPD;
   BOOL     done;

   //--- First parse the actual board configuration ---

   INT rank, file;

   ClearTable(EPD.Board);
   for (done = false, rank = 7, file = 0; ! done; s++)
   {
      PIECE p = empty;
      switch (*s)
      {
         case 'K' : p = wKing;   break;
         case 'Q' : p = wQueen;  break;
         case 'R' : p = wRook;   break;
         case 'B' : p = wBishop; break;
         case 'N' : p = wKnight; break;
         case 'P' : p = wPawn;   break;

         case 'k' : p = bKing;   break;
         case 'q' : p = bQueen;  break;
         case 'r' : p = bRook;   break;
         case 'b' : p = bBishop; break;
         case 'n' : p = bKnight; break;
         case 'p' : p = bPawn;   break;

         case '/' : if (file != 8 || rank == 0)
                       return epdErr_InvalidSquare;
                    rank--; file = 0; break;
         case ' ' : done = true; break;

         case 0   : return epdErr_UnexpectedEnd;

         default  :
            if (IsDigit(*s)) file += (*s - '0');
            else return epdErr_InvalidChar;
      }

      if (p)
      {   if (rank < 0 || file > 7)
            return epdErr_InvalidSquare;
         EPD.Board[(file++) + (rank << 4)] = p;
      }
   }

   //--- Parse initial player field ---

   switch (*(s++))
   {  case 'w' : EPD.player = white; break;
      case 'b' : EPD.player = black; break;
      default  : return epdErr_InvalidInitialPlayer;
   }

   if (*(s++) != ' ') return epdErr_InvalidInitialPlayer;

   //--- Parse castling rights field ---

   if (*s == ' ') return epdErr_InvalidCastlingFlags;

   EPD.castlingRights = 0;
   done = false;
   while (! done)
      switch (*(s++))
      {  case 'K' : EPD.castlingRights |= castRight_WO_O;   break;
         case 'Q' : EPD.castlingRights |= castRight_WO_O_O; break;
         case 'k' : EPD.castlingRights |= castRight_BO_O;   break;
         case 'q' : EPD.castlingRights |= castRight_BO_O_O; break;
         case '-' : EPD.castlingRights = 0; break;
         case ' ' : done = true; break;
         default  : return epdErr_InvalidCastlingFlags;
      }

   //--- Parse en passant square field ---

   if (*s == '-')
   {
      EPD.epSquare = nullSq;
      s++;
   }
   else if (s[0] >= 'a' && s[0] <= 'h' && s[1] >= '1' && s[1] <= '8')
   {
      EPD.epSquare  = *(s++) - 'a';
      EPD.epSquare += (*(s++) - '1') << 4;

      if (EPD.player == black)                     // Verify en passant square
      {
         if (rank(EPD.epSquare) != 2 ||
             EPD.Board[front(EPD.epSquare)] != wPawn ||
             EPD.Board[EPD.epSquare] != empty ||
             EPD.Board[behind(EPD.epSquare)] != empty)
         {
            return epdErr_InvalidEPSquare;
         }
      }
      else
      {
         if (rank(EPD.epSquare) != 5 ||
             EPD.Board[behind(EPD.epSquare)] != bPawn ||
             EPD.Board[EPD.epSquare] != empty ||
             EPD.Board[front(EPD.epSquare)] != empty)
         {
            return epdErr_InvalidEPSquare;
         }
      }
   }

   //--- EPD String was Legal ---

   EPD.wasSetup = true;
   EPD.moveNo = 1;
   EPD.revMoves = 0;

   Init = EPD;
   ResetGame();
   dirty = true;

   //--- Parse optional fields (am/bm, id etc) ---
   // New field : <space> <tag> <space> <value string> <separator>

   CHAR type[10], value[30], text[100];
   
   while (*s == ' ')  // While more fields...
   {
      // Skip leading blanks before tag type:
      do s++; while (*s == ' ');

      // Parse type ("am", "bm", "id", ...)
      INT i;
      for (i = 0; i < 9 && *s && *s != ' '; i++)
         type[i] = *(s++);
      type[i] = 0;
      if (*s != ' ') goto done;

      // Skip leading blanks before tag value:
      do s++; while (*s == ' ');

      // Parse value:
      if (*s == '"') s++;
      for (i = 0; i < 29 && *s && *s != ';'; i++)
         value[i] = *(s++);
      if (i > 0 && value[i-1] == '"') i--;
      value[i] = 0;
      if (*(s++) != ';') goto done;

      // Dispatch on tag type:
      if (SameStr(type, "am"))
      {
         Format(Info.blackName, "am %s", value);
         Format(text, "Avoid move : %s", value);
         SetAnnotation(0, text, StrLen(text));
      }
      else if (SameStr(type, "bm"))
      {
         Format(Info.blackName, "bm %s", value);
         Format(text, "Best move : %s", value);
         SetAnnotation(0, text, StrLen(text));
      }
      else if (SameStr(type, "id"))
      {
         CopyStr(value, Info.heading),
         CopyStr(value, Info.whiteName);
      }
   }

done:
   return epdErr_NoError;
} /* CGame::Read_EPD */
