/**************************************************************************************************/
/*                                                                                                */
/* Module  : GameFile.c                                                                           */
/* Purpose : This module implements save/load of games in the new version 5 super compressed.     */
/*           format.                                                                              */
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

// The � Super Compressed format only uses an average of 6-7 bits per move!!! while at the same time
// being easy and efficient to generate and decode (i.e. it is NOT based on indexes in a pseudo
// legal move list, which is very slow).
//
// Every move is composed of two parts:
//
// (1) A fixed 4 bit "Piece ID" (obtained by scanning the board from a1 to h8).
// (2) A 2-5 bit "Move Number" field, where the size and format depends on the moving piece.
//
// PAWNS : 2 bits for normal moves, and an additional 2 bits for promotions
//
// KNIGHTS : 3 bits denoting one of the 8 directions.
//
// BISHOPS/ROOKS : 4 bits denoting one of the 2x7 = 14 possible moves.
//
// QUEENS : 5 bits denoting one of the 4x7 = 28 possible moves.
//
// KINGS : 3 bits denoting one of the 8 directions. Moving "back&off" the board left/right indicates
// castling left/right.

// Compression tables (KNP only): Tables mapping move directions to move numbers:
static INT *KMoveNo, _KMoveNo[2*0x11 + 1];     // -0x11...0x11
static INT *NMoveNo, _NMoveNo[2*0x22 + 1];     // -0x22...0x22
static INT *PMoveNo, _PMoveNo[2*0x20 + 1];     // -0x20...0x20

// Decompression tables (KNP only): "Reverse" tables mapping the move number to the move direction:
static SQUARE KDir[8] = { -0x0F, -0x11,  0x11,  0x0F, -0x10, 0x10, 0x01, -0x01 };
static SQUARE NDir[8] = { -0x0E, -0x12, -0x1F, -0x21,  0x12, 0x0E, 0x21,  0x1F };
static SQUARE PDir[4] = {  0x10,  0x20,  0x0F,  0x11 };

// Number of piece bits depending on number of pieces:
static INT PBits[17] = { 0,1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4 };  // Piece no bits needed as function on
                                                                  // number of pieces
// Number of move no bits depending on piece type (PNBRQK):
//static INT MBits[7] = { 0,2,3,4,4,5,3 };


/**************************************************************************************************/
/*                                                                                                */
/*                                        COMPRESSING GAME                                        */
/*                                                                                                */
/**************************************************************************************************/

LONG CGame::Compress (PTR Data)
{
   LONG size = 0;
   size += CompressInfo (Data);
   size += CompressMoves(Data + size);
   size += CompressAux  (Data + size);
   return size;
} /* CGame::Compress */

/*-------------------------------------- Compress Game Info --------------------------------------*/

static INT CompressInfoStr (BYTE tag, CHAR *s, PTR Data);
static INT CompressInfoByte (BYTE tag, BYTE val, PTR Data, BYTE nilVal);
static INT CompressInfoInt (BYTE tag, INT val, PTR Data, INT nilVal);
static INT CompressInfoLayout (BYTE tag, GAMEINFO *Info, PTR Data);

INT CGame::CompressInfo (PTR Data)
{
   INT n = 2;

   // Mandatory PGN tags:
   n += CompressInfoStr (tag_WhiteName, Info.whiteName, Data + n);
   n += CompressInfoStr (tag_BlackName, Info.blackName, Data + n);
   n += CompressInfoStr (tag_Event,     Info.event,     Data + n);
   n += CompressInfoStr (tag_Site,      Info.site,      Data + n);
   n += CompressInfoStr (tag_Date,      Info.date,      Data + n);
   n += CompressInfoStr (tag_Round,     Info.round,     Data + n);
   n += CompressInfoByte(tag_Result,    Info.result,    Data + n, infoResult_Unknown);

   // Additional PGN tags:
   n += CompressInfoInt (tag_WhiteELO,  Info.whiteELO,  Data + n, -1);
   n += CompressInfoInt (tag_BlackELO,  Info.blackELO,  Data + n, -1);
   n += CompressInfoStr (tag_ECO,       Info.ECO,       Data + n);
   n += CompressInfoStr (tag_Annotator, Info.annotator, Data + n);

   // Sigma Chess specific layout tag:
   INT layoutFlags = Info.headingType;
   if (Info.pageBreak) layoutFlags |= 0x04;
   if (! Info.includeInfo) layoutFlags |= 0x08;
   if (layoutFlags)
   {  CHAR s[100];
      s[0] = layoutFlags;
      CopyStr(Info.heading, &s[1]);
      n += CompressInfoStr(tag_Layout, s, Data + n);
   }

   Data[0] = n >> 8; Data[1] = n & 0x00FF;
   return n;
} /* CGame::CompressInfo */


static INT CompressInfoStr (BYTE tag, CHAR *s, PTR Data)
{
   if (! s[0]) return 0;   // Skip completely for empty strings

   INT n = 2;
   CHAR *t = (CHAR*)(Data + 2);
   while (*(t++) = *(s++)) n++;
   Data[0] = (tag << 2) | (n >> 8);
   Data[1] = n & 0x00FF;
   return n;
} /* CompressInfoStr */


static INT CompressInfoByte (BYTE tag, BYTE val, PTR Data, BYTE nilVal)
{
   if (val == nilVal) return 0;

   Data[0] = tag << 2;
   Data[1] = 3;
   Data[2] = val;
   return 3;
} /* CompressInfoByte */


static INT CompressInfoInt (BYTE tag, INT val, PTR Data, INT nilVal)
{
   if (val == nilVal) return 0;

   Data[0] = tag << 2;
   Data[1] = 4;
   Data[2] = val >> 8;
   Data[3] = val & 0x00FF;
   return 4;
} /* CompressInfoByte */


static INT CompressInfoLayout (BYTE tag, GAMEINFO *Info, PTR Data)
{
   if (! Info->pageBreak && Info->headingType != headingType_None && Info->includeInfo) return 0;

   INT n = 3;
   CHAR *t = (CHAR*)&Data[3];
   CHAR *s = Info->heading;
   while (*(t++) = *(s++)) n++;

   Data[0] = tag << 2;
   Data[1] = n & 0x00FF;
   Data[2] = Info->headingType;
   if (Info->pageBreak) Data[2] |= 0x80;
   if (Info->includeInfo) Data[2] |= 0x40;
   return n;
} /* CompressInfoLayout */

/*------------------------------------ Compress Game Record --------------------------------------*/

#define northEastMove(m) (Sign(file(m->from) - file(m->to)) == Sign(rank(m->from) - rank(m->to)))

static INT WriteInitPos (PIECE B[], PTR Data, INT *wcount, INT *bcount);

INT CGame::CompressMoves (PTR Data)
{
   INT    n;                     // Bytes written so far
   COLOUR tplayer;               // Temporary side to move when replaying
   INT    count, xcount;         // Current number of pieces for player/opponent

   //--- WRITE NUMBER OF HALF MOVES PLAYED ---

   Data[2] = lastMove >> 8;
   Data[3] = lastMove & 0x00FF;

   //--- WRITE INITIAL POSITION (IF SETUP) ---

   if (! Init.wasSetup)
   {
      n = 4;
      tplayer = white;
      count = xcount = 16;
   }
   else
   {
      // Set wasSetup, initial player and castling flags (bytes 2-3):
      Data[2] |= (0x80 | ((Init.player | Init.castlingRights) << 2));

      // Write initial moveno, revMoves and epSquare info (bytes 4-6):
      Data[4] = (Init.revMoves << 1) | (Init.moveNo >> 8);
      Data[5] = Init.moveNo & 0x00FF;
      Data[6] = (Init.epSquare == nullSq ? 0x08 : file(Init.epSquare));

      // Finally write initial position (bytes 7-38):
      n = 7 + WriteInitPos(Init.Board, &Data[7], &count, &xcount);
      tplayer = Init.player;
      if (tplayer == black) Swap(&count, &xcount);
   }

   //--- WRITE ACTUAL MOVE RECORD ---

   PIECE B[boardSize];           // Temporary board
   MOVE  *m = &Record[1];        // Current move
   INT   nbits = 8;              // Number of unwritten bits in current byte Data[n]

   CopyTable(Init.Board, B);

   for (INT i = 1; i <= lastMove; i++, m++)
   {
      //--- First get piece ID ---

      INT    pid = 0;
      PIECE  p;
      SQUARE sq = a1;

      while (sq < m->from)
         if (offBoard(sq)) sq += 8;
         else if ((p = B[sq++]) && pieceColour(p) == tplayer) pid++;

      //--- Next calc move no ---

      INT moveNo;      // Move number (0..27)
      INT moveNoBits;  // Number of bits used for storing moveno (2 - 5).

      switch (pieceType(m->piece))
      {
         case pawn :
            moveNo = PMoveNo[m->to - m->from];
            if (isPromotion(*m))
               moveNoBits = 4, moveNo = (moveNo << 2) + (m->type & 0x07) - knight;
            else
               moveNoBits = 2;
            break;
         case knight :
            moveNo = NMoveNo[m->to - m->from];
            moveNoBits = 3;
            break;
         case bishop :
            moveNo = file(m->to) + (northEastMove(m) ? 0 : 8);
            moveNoBits = 4;
            break;
         case rook :
            moveNo = (file(m->from) == file(m->to) ? rank(m->to) : 8 + file(m->to));
            moveNoBits = 4;
            break;
         case queen :
            if (file(m->from) == file(m->to)) moveNo = rank(m->to);
            else if (rank(m->from) == rank(m->to)) moveNo = 8 + file(m->to);
            else if (northEastMove(m)) moveNo = 16 + file(m->to);
            else moveNo = 24 + file(m->to);
            moveNoBits = 5;
            break;
         case king :
            switch (m->type)
            {  case mtype_Normal : moveNo = KMoveNo[m->to - m->from]; break;
               case mtype_O_O    : moveNo = KMoveNo[tplayer == white ? -0x0F : 0x11]; break;
               case mtype_O_O_O  : moveNo = KMoveNo[tplayer == white ? -0x11 : 0x0F];
            }
            moveNoBits = 3;
      }

      //--- Concatenate piece no and move no ---

      INT move = (pid << moveNoBits) + moveNo;   // Generate whole move (piece id + move no)
      INT moveBits = PBits[count] + moveNoBits;  // Total bit size of move.

      //--- Append move bits to move buffer ---

      if (nbits == 8) Data[n] = 0;       // If no bits have been written yet -> Erase byte first.

      if (moveBits > nbits)              // Not enough room -> split across two bytes
      {  moveBits -= nbits;              // Calc remaining bits for next byte
         Data[n++] |= move >> moveBits;
         move &= (1 << moveBits) - 1;    // Clear the movebits that have already been written
         nbits = 8;
         Data[n] = 0;
      }
      if (moveBits == nbits)             // If precisely room for all bits in current byte
      {  Data[n++] |= move;
         nbits = 8;
      }
      else if (moveBits < nbits)         // If extra room
      {  nbits -= moveBits;
         Data[n] |= move << nbits;
      }

      //--- Perform move on temp board ---

      B[m->from] = empty;                           // Perform move on the board and
      B[m->to]   = m->piece;                        // update "pieceCount" (for use by
      if (m->cap || m->type == mtype_EP) xcount--;

      switch (m->type)
      {
         case mtype_Normal : break;
         case mtype_O_O    : B[right(m->to)] = empty; B[left(m->to)]  = rook + tplayer; break;
         case mtype_O_O_O  : B[left2(m->to)] = empty; B[right(m->to)] = rook + tplayer; break;
         case mtype_EP     : B[m->to + 2*tplayer - 0x10] = empty; break;
         default           : B[m->to] = m->type;
      }

      Swap(&count, &xcount);
      tplayer = black - tplayer;
   }

   //--- FINALLY UPDATE SIZE FIELD AND RETURN SIZE ---

   if (nbits < 8) n++;        // Increase byte count if we have started writing into current byte
   Data[0] = n >> 8;
   Data[1] = n & 0x00FF;
   return n;
} /* CGame::CompressMoves */


// Writes the initial position to the data buffer (5-34 bytes):

static INT WriteInitPos (PIECE B[], PTR Data, INT *wcount, INT *bcount)
{
   BYTE WK, WP[8], WX[15];     // Location of all white pieces
   BYTE BK, BP[8], BX[15];     // Location of all black pieces
   INT  nwp, nwx, nbp, nbx;    // Number of pawns (p) and QRBN (x) for white and black.
   INT  n = 0;                 // Number of bytes written.

   // First store location and count of each piece type:

   nwp = nwx = nbp = nbx = 0;
   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq))
      {
         SQUARE sq6 = file(sq) | (rank(sq) << 3);       // "Folded" 6 bit version of square

         switch (PIECE p = B[sq])
         {
            case wKing  : WK = sq; break;
            case bKing  : BK = sq; break;
            case wPawn  : WP[nwp++] = sq; break;
            case bPawn  : BP[nbp++] = sq; break;
            case wQueen : case wRook : case wBishop : case wKnight :
               WX[nwx++] = sq6 | ((p - wKnight) << 6); break;
            case bQueen : case bRook : case bBishop : case bKnight :
               BX[nbx++] = sq6 | ((p - bKnight) << 6); break;
         }
      }

   // Next write the actual data to the Data buffer:

   Data[n++] = WK;
   Data[n++] = BK;

   Data[n++] = (nwp << 4) | nwx;     // White pieces:
   for (INT i = 0; i < nwp; i++) Data[n++] = WP[i];
   for (INT i = 0; i < nwx; i++) Data[n++] = WX[i];

   Data[n++] = (nbp << 4) | nbx;     // Black pieces:
   for (INT i = 0; i < nbp; i++) Data[n++] = BP[i];
   for (INT i = 0; i < nbx; i++) Data[n++] = BX[i];

   // Finally return number of pieces of each color as well as number of bytes written:

   *wcount = 1 + nwp + nwx;
   *bcount = 1 + nbp + nbx;

   return n;
} /* WriteInitPos */

/*----------------------------------- Compress Auxiliary Data ------------------------------------*/
// The "Auxiliary Data" part contains optional extra information for each move, e.g. annotation
// text, annotation glyphs, RAVs etc. The first two bytes holds the size of this auxiliary data.
// This is followed by 1 or more records for the relevant half moves. Each record has the following
// format:
// � 2 bytes specifying the half move (0..1023). The high order bits specify what type of infor-
//   mation is stored for this move (bit15 = glyph, bit14 = annotation text, bit13 = RAVs etc.)
// � The actual auxiliary data for the half move:
//   1. Annotation glyph (1 byte).
//   2. Annotation text (2 byte size + variable length string data).
//   3. RAVs (2 byte size + variable length RAV data). ###NOT YET IMPLEMENTED!

LONG CGame::CompressAux (PTR Data)
{
   UINT n = 2;

   for (INT j = 0; j <= lastMove; j++)
   {
      INT  glyph  = GetAnnotationGlyph(j);
      BOOL hasAnn = ExistAnnotation(j);

      if (glyph || hasAnn)
      {
         UINT n0 = n;
         Data[n++] = (j >> 8);
         Data[n++] = (j & 0x00FF);

         if (glyph)
         {  Data[n0] |= 0x80;
            Data[n++] = glyph;
         }
         if (hasAnn)
         {  Data[n0] |= 0x40;
            INT annSize;
            GetAnnotation(j, (CHAR*)&Data[n+2], &annSize);
            Data[n++] = (annSize >> 8);
            Data[n++] = (annSize & 0x00FF);
            n += annSize;
         }
/*       if (hasRAV)
         { ...
         } */
      }
   }

   if (n == 2) return 0;   // Skip completely if no auxiliary data at all for the game.

   Data[0] = n >> 8; Data[1] = n & 0x00FF;
   return n;
} /* CGame::CompressAux */


/**************************************************************************************************/
/*                                                                                                */
/*                                      DECOMPRESSING GAME                                        */
/*                                                                                                */
/**************************************************************************************************/

void CGame::Decompress (PTR Data, LONG size, BOOL raw)
{
   LONG bytes = 0;
   bytes += DecompressInfo(Data);
   bytes += DecompressMoves(Data + bytes, raw);
   if (size > bytes)
      bytes += DecompressAux(Data + bytes);
   UndoAllMoves();
   dirty = false;
} /* CGame::Decompress */

/*------------------------------------ Decompress Game Info --------------------------------------*/

INT CGame::DecompressInfo (PTR Data)
{
   return ::DecompressGameInfo(Data, &Info);
} /* CGame::DecompressInfo */


static void DecompressInfoStr (PTR Data, CHAR *s, INT bytes);

INT DecompressGameInfo (PTR Data, GAMEINFO *Info)
{
   INT size = (Data[0] << 8) | Data[1];    // Total size of Game Info block
   INT n = 2;                              // Bytes decompressed so far

   ::ClearGameInfo(Info);

   while (n < size)
   {
      BYTE tag   = (Data[n] >> 2) & 0x3F;                       // Info record tag
      INT  bytes = ((Data[n] & 0x03) << 8) | Data[n + 1] - 2;   // Size of current info record
                                                                // (excl tag+size info)
      n += 2;  // Move to data part (i.e. skip tag+size header):

      switch (tag)
      {
         case tag_WhiteName : DecompressInfoStr(&Data[n], Info->whiteName, bytes); break;
         case tag_BlackName : DecompressInfoStr(&Data[n], Info->blackName, bytes); break;
         case tag_Event     : DecompressInfoStr(&Data[n], Info->event,     bytes); break;
         case tag_Site      : DecompressInfoStr(&Data[n], Info->site,      bytes); break;
         case tag_Date      : DecompressInfoStr(&Data[n], Info->date,      bytes); break;
         case tag_Round     : DecompressInfoStr(&Data[n], Info->round,     bytes); break;
         case tag_Result    : Info->result = Data[n]; break;

         case tag_WhiteELO  : Info->whiteELO = (Data[n] << 8) | Data[n + 1]; break;
         case tag_BlackELO  : Info->blackELO = (Data[n] << 8) | Data[n + 1]; break;
         case tag_ECO       : DecompressInfoStr(&Data[n], Info->ECO,       bytes); break;
         case tag_Annotator : DecompressInfoStr(&Data[n], Info->annotator, bytes); break;

         case tag_Layout    :
            CHAR s[100];
            DecompressInfoStr(&Data[n], s, bytes);
            CopyStr(&s[1], Info->heading);
            if (s[0] & 0x04) Info->pageBreak = true;
            if (s[0] & 0x08) Info->includeInfo = false;
            Info->headingType = s[0] & 0x03;
            break;

//       default : //### STORE IN GENERIC TAG INFO PART
      }

      n += bytes;  // Move to next record (if any)
   }

   return n;
} /* DecompressGameInfo */


static void DecompressInfoStr (PTR Data, CHAR *s, INT bytes)
{
   while (bytes-- > 0) *(s++) = *(Data++);
   *s = 0;
} /* DecompressInfoStr */

/*----------------------------------- Decompress Game Record -------------------------------------*/

static INT ReadInitPos (PTR Data, PIECE B[], INT *wcount, INT *bcount);

#define ReadBits(m,val) \
{  INT vm = m; \
   val = 0; \
   if (vm > nbits) val = Data[n++] << (vm -= nbits), nbits = 8; \
   if (vm == nbits) val |= Data[n++], nbits = 8; \
   else val |= (Data[n] >> (nbits - vm)), nbits -= vm; \
   val &= (1 << m) - 1; \
}

INT CGame::DecompressMoves (PTR Data, BOOL raw)
{
   INT      size;                  // Total size of move record block
   INT      moveCount;             // Number of half moves in move record
   INT      n;                     // Bytes read so far
   INT      count, xcount;         // Current number of pieces for player/opponent

   size = ((Data[0] & 0x07) << 8) | Data[1];

   //--- READ NUMBER OF HALF MOVES PLAYED ---

   moveCount = (((INT)Data[2] & 0x0003) << 8) | Data[3];

   //--- READ INITIAL POSITION (IF SETUP) ---

   Init.wasSetup = ((Data[2] & 0x80) != 0);

   if (! Init.wasSetup)
   {
      n = 4;
      count = xcount = 16;  

      NewGame(false);
   }
   else
   {
      // Read intial player and castling rights (byte 2):
      Init.player = (Data[2] >> 2) & 0x10;
      Init.castlingRights = (Data[2] >> 2) & 0x0F;

      // Read initial moveno, revMoves and epSquare info (bytes 4-6):
      Init.revMoves = (Data[4] >> 1) & 0x7F;
      Init.moveNo   = ((INT)(Data[4] & 0x01) << 8) | Data[5];
      Init.epSquare = (Data[6] & 0x08 ? nullSq : square(Data[6] & 0x07, 0x40 - Init.player));

      // Finally read initial position (bytes 7-38):
      n = 7 + ReadInitPos(&Data[7], Init.Board, &count, &xcount);
      if (Init.player == black) Swap(&count, &xcount);

      ResetGame(false);
   }

   //--- READ ACTUAL MOVE RECORD ---

   INT nbits = 8;                 // Number of unread bits from current byte Data[n].

   for (INT j = 1; j <= moveCount; j++)
   {
      MOVE m;                     // The current move being decoded.

      //--- First get piece ID and locate piece ---

      INT pid;                    // The piece id
      INT pbits = PBits[count];   // Number of bits used for storing piece id
      ReadBits(pbits, pid);       // Decode piece id

      SQUARE sq = a1;             // Get location of decoded piece
      PIECE p;

      m.from = nullSq;
      while (m.from == nullSq)
         if (offBoard(sq)) sq += 8;
         else if ((p = Board[sq]) && pieceColour(p) == player && pid-- == 0) m.from = sq;
         else sq++;

      m.piece = p;
      m.type  = mtype_Normal;

      //--- Next decode move no ---

      INT moveNo;                 // Move number (0..27)

      switch (pieceType(m.piece))
      {
         case pawn :
            ReadBits(2, moveNo);
            m.to = m.from + (player == white ? PDir[moveNo] : -PDir[moveNo]);
            if (rank(m.from) == Global.B.Rank7[player])
            {  INT promPiece;
               ReadBits(2, promPiece);
               m.type = player + knight + promPiece;
            }
            else if (! Board[m.to] && moveNo >= 2)
               m.type = mtype_EP;
            break;

         case knight :
            ReadBits(3, moveNo);
            m.to = m.from + NDir[moveNo];
            break;

         case bishop :
            ReadBits(4, moveNo);
            if (moveNo < 8)
               moveNo -= file(m.from),                   // [0..7]  : 45/-135 degrees
               m.to = m.from + square(moveNo,moveNo); 
            else
               moveNo -= file(m.from) + 8,               // [8..15] : -45/135 degrees
               m.to = m.from + square(moveNo,-moveNo); 
            break;

         case rook :
            ReadBits(4, moveNo);
            if (moveNo < 8)
               m.to = square(file(m.from), moveNo);      // [0..7]  : Same file
            else
               m.to = square(moveNo - 8, rank(m.from));  // [8..15] : Same rank
            break;

         case queen :
            ReadBits(5, moveNo);
            if (moveNo < 8)
               m.to = square(file(m.from), moveNo);      // [0..7]   : Same file
            else if (moveNo < 16)
               m.to = square(moveNo - 8, rank(m.from));  // [8..15]  : Same rank
            else if (moveNo < 24)
               moveNo -= file(m.from) + 16,              // [16..23] : 45/-135 degrees
               m.to = m.from + square(moveNo,moveNo); 
            else
               moveNo -= file(m.from) + 24,              // [24..31] : -45/135 degrees
               m.to = m.from + square(moveNo,-moveNo);
            break;

         case king :
            ReadBits(3, moveNo);
            m.to = m.from + KDir[moveNo];
            if (offBoard(m.to))
            {  m.to = m.from + 2*(KDir[moveNo] + (player == white ? 0x10 : -0x10));
               m.type = (m.to > m.from ? mtype_O_O : mtype_O_O_O);
            }
      }

      m.cap = Board[m.to];
      m.dir = m.dply = m.flags = 0;
      if (m.cap || m.type == mtype_EP) xcount--;

      //--- Perform move on temp board ---

      if (raw)
         PlayMoveRaw(&m);
      else
      {  PlayMove(&m);
         SetAnnotationGlyph(j, 0);
      }

      Swap(&count, &xcount);
   }

   //--- FINALLY UPDATE SIZE FIELD AND RETURN SIZE ---

   return size;
} /* CGame::DecompressMoves */

// Writes the initial position to the data buffer (5-34 bytes):

static INT ReadInitPos (PTR Data, PIECE B[], INT *wcount, INT *bcount)
{
   INT  nwp, nwx, nbp, nbx;    // Number of pawns (p) and QRBN (x) for white and black.
   INT  n = 0;                 // Number of bytes written.
   INT  byte;
   SQUARE sq;

   ClearTable(B);

   // Read initial position from the Data buffer:

   B[Data[n++]] = wKing;
   B[Data[n++]] = bKing;

   nwp = Data[n] >> 4;             // Read white pawns and officers
   nwx = Data[n++] & 0x0F;
   for (INT i = 0; i < nwp; i++)
      B[Data[n++]] = wPawn;
   for (INT i = 0; i < nwx; i++)
   {  byte  = (Data[n++] & 0x00FF);
      sq    = (byte & 0x07) | ((byte << 1) & 0x70);
      B[sq] = (byte >> 6) + wKnight;
   }

   nbp = Data[n] >> 4;             // Read black pawns and officers
   nbx = Data[n++] & 0x0F;
   for (INT i = 0; i < nbp; i++)
      B[Data[n++]] = bPawn;
   for (INT i = 0; i < nbx; i++)
   {  byte  = (Data[n++] & 0x00FF);
      sq    = (byte & 0x07) | ((byte << 1) & 0x70);
      B[sq] = (byte >> 6) + bKnight;
   }

   // Finally return number of pieces of each color as well as number of bytes read:

   *wcount = 1 + nwp + nwx;
   *bcount = 1 + nbp + nbx;

   return n;
} /* ReadInitPos */

/*----------------------------------- Decompress Annotations -------------------------------------*/

LONG CGame::DecompressAux (PTR Data)
{
   UINT size = (Data[0] << 8) | Data[1];    // Total size of Aux Data block
   UINT n = 2;                              // Bytes decompressed so far

   do
   {
      UINT rec = (Data[n] << 8) | Data[n + 1];
      INT  j   = rec & 0x03FF;
      n += 2;
      if (rec & 0x8000)
      {
         SetAnnotationGlyph(j, Data[n++] & 0x00FF);
      }
      if (rec & 0x4000)
      {
         UINT charCount = (Data[n] << 8) | Data[n + 1];
         n += 2;
         SetAnnotation(j, (CHAR*)&Data[n], charCount);
         n += charCount;
      }
   } while (n < size);

   return n;
} /* CGame::DecompressAux */


/**************************************************************************************************/
/*                                                                                                */
/*                                           UTILITY                                              */
/*                                                                                                */
/**************************************************************************************************/

// This routine initializes the utility tables at startup. Is called from "InitGameModule()".

void InitGameFile5 (void)
{
   KMoveNo = _KMoveNo + 0x11;
   NMoveNo = _NMoveNo + 0x22;
   PMoveNo = _PMoveNo + 0x20;

   for (INT i = 0; i <= 7; i++)
      KMoveNo[KDir[i]] = i,
      NMoveNo[NDir[i]] = i;

   PMoveNo[0x10] = PMoveNo[-0x10] = 0;
   PMoveNo[0x20] = PMoveNo[-0x20] = 1;
   PMoveNo[0x0F] = PMoveNo[-0x0F] = 2;
   PMoveNo[0x11] = PMoveNo[-0x11] = 3;
} /* InitGameFile5 */
