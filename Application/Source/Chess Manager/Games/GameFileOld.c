/**************************************************************************************************/
/*                                                                                                */
/* Module  : GameFile.c */
/* Purpose : This module implements save/load of games in the · Extended format.
 */
/*                                                                                                */
/**************************************************************************************************/

/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

¥ Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

¥ Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Board.f"
#include "Game.h"
#include "Move.f"

#define StoreByte(n) Packet->Data[packInx++] = n
#define FetchByte() (Packet->Data[packInx++])

typedef struct {
  INT size;       // Total size in bytes of this record (8 if empty, i.e. no ann
                  // text/data for this move. Always even!
  INT moveNo;     // Half move number to (after) which annotation applies.
  INT lineCount;  // Number of text lines.
  INT charCount;  // Number of characters in annotation text.
  BYTE Data[];    // The annotation data consisting of line start data
                  // followed by the actual annotation text data.
} * ANNPTR4;

/**************************************************************************************************/
/*                                                                                                */
/*                                        SIGMA EXTENDED FORMAT */
/*                                                                                                */
/**************************************************************************************************/

// The new Sigma Extended game format ('·GMX') is a very flexible format, that
// will provide backward compatibility from all future versions of Sigma Chess.
// The game data is organized as a sequence of consecutive "Packets" which
// resemble the resource formats in that the first two fields of a packet
// indicates its type and size followed by the actual data pertaining to that
// packet.
//
//      typedef struct
//      {  ULONG   type;      // E.g. 'SIZE', 'INFO', 'INIT', 'MOVE', 'text',
//      'gmfl'
//         ULONG   size;      // Size in bytes of packet data (must be even).
//         BYTE    Data[]
//      } PACKET;
//
// The mandatory packets are written in upper case, whereas optional packets are
// written in lower case.
//
// 'SIZE' : This must always be the first packet. It describes the total size of
// all packets
//          in the game data, as well as the extended format version (0x0101) in
//          Sigma Chess 4.0.
// 'cinf' : This packet contains collection specific info (title, flags e.t.c.)
// about games
//          which are (or has been) part of a collection.
// 'INFO' : This contains info about the the players, site, event e.t.c. (more
// precisely the
//          7 mandatory tags from the PGN format plus the optional ELO tags).
// 'INIT' : If the initial position was setup, this packet is mandatory. It
// describes the
//          initial position including castling and en passant status.
// 'gmfl' : Game flags (See "Game.h") indicating if the move is a check or a
// mate, and if
//          move disambiguation is needed. Also these game flags contain the
//          numerical annotation glyphs. This packet speeds up the load process,
//          because we don't have too recompute data structures for each game
//          move in order to compute e.g. check flags. MUST COME BEFORE 'MOVE'
//          PACKET if present.
// 'MOVE' : The is the main packet which contain all the moves of the game.
// 'ann ' : This optional packet contains annotations/comments for the moves.

/*------------------------------------------- Saving
 * ---------------------------------------------*/

LONG CGame::Write_V34(PTR GameData) {
  LONG totalSize;

  //--- 'SIZE' Packet ---

  FirstPacket(GameData);
  Packet->type = 'SIZE';  // 'SIZE'
  StoreLong(0);           // LONG      totalSize (unknown initially
  StoreInt(0x0101);       // INT       version
  Packet->size = packInx;
  /*
     //--- 'cinf' Packet ---                            // Optional collection
     game info

     if (isCollectionGame)
     {
        P = NextPacket(P); i = 0;                       // 'cinf'
        P->type = 'cinf';
        StorePStr(ColInfo.title);          // Str31     title
        StoreLong(ColInfo.flags);          // LONG      flags
        P->size = i;
     }
  */
  //--- 'INFO' Packet ---

  NextPacket();  // 'INFO'
  Packet->type = 'INFO';
  StoreStr(Info.event);      // Str63    event
  StoreStr(Info.site);       // Str63    site
  StoreStr(Info.date);       // Str15    date
  StoreStr(Info.round);      // Str15    round
  StoreStr(Info.whiteName);  // Str63    white name
  StoreStr(Info.blackName);  // Str63    black name
  StoreInt(Info.whiteELO);   // INT      white ELO
  StoreInt(Info.blackELO);   // INT      black ELO
  StoreInt(Info.result);     // INT      result
  //    StoreStr(Info.comment);                         // Str255   comment
  Packet->size = packInx;

  //--- 'INIT' Packet ---

  NextPacket();  // 'INIT'
  Packet->type = 'INIT';
  StoreInt(Init.wasSetup);  // BOOL     wasSetup
  if (Init.wasSetup) {
    StoreInt(Init.player);          // COLOUR   initialPlayer
    StoreInt(Init.castlingRights);  // INT      castlingFlags
    StoreInt(Init.epSquare);        // SQUARE   epSquare
    StoreInt(Init.revMoves);        // INT      initIrrMoves
    StoreInt(Init.moveNo);          // INT      initialMoveNo

    for (SQUARE sq = a1; sq <= h8; sq++)  // BYTE     initialPos[64]
      if (onBoard(sq)) StoreByte(Init.Board[sq]);
  }
  Packet->size = packInx;

  //--- 'gmfl' Packet ---

  NextPacket();  // 'gmfl'
  Packet->type = 'gmfl';
  StoreInt(lastMove);                  // INT      lastGameMove
  for (INT j = 1; j <= lastMove; j++)  // BYTE     GameFlags[]
    StoreByte(((Record[j].flags & 0x0F) << 3) | (Record[j].misc & 0x07));
  if (odd(packInx)) packInx++;
  Packet->size = packInx;

  //--- 'MOVE' Packet ---

  NextPacket();  // 'MOVE'
  Packet->type = 'MOVE';
  StoreInt(currMove);                  // INT      currentGameMove
  StoreInt(lastMove);                  // INT      lastGameMove
  for (INT j = 1; j <= lastMove; j++)  // PMOVE      MoveList[lastGameMove]
    StoreInt(Move_Pack(&Record[j]));
  Packet->size = packInx;

  //--- 'ann ' Packet ---
  /*
     NextPacket('ann ');                                // 'ann '
     Packet->type =
     WriteAnnData(P->data, &i);                         // ANNREC   AnnData[...]
     P->size = i;
  */
  //--- Finally update total size field in the 'SIZE' Packet ---

  NextPacket();
  totalSize = (PTR)Packet - GameData;

  FirstPacket(GameData);
  StoreLong(totalSize);

  return totalSize;
} /* CGame::Write_V34 */

/*------------------------------------------ Loading
 * ---------------------------------------------*/

void CGame::Read_V34(PTR GameData, BOOL calcMoveFlags) {
  ULONG totalSize;
  MOVE m;
  INT jmax, version;
  CHAR dummy[500];

  ClearGameInfo();

  FirstPacket(GameData);
  totalSize = FetchLong();
  version = FetchInt();

  //### VALIDATE HERE (AND RETURN ERROR CODE IF WRONG FORMAT):

  do {
    switch (Packet->type) {
      case 'SIZE':
        break;  // Never mind in this version

      case 'cinf':  // Should always be first packet after 'SIZE' packet (if
                    // present)
        FetchStr(Info.heading);    // Str31    heading
        LONG flags = FetchLong();  // LONG     flags
        if (flags & 0x0001)
          Info.headingType = headingType_Chapter;
        else if (flags & 0x0002)
          Info.headingType = headingType_Section;
        else if (flags & 0x0004)
          Info.headingType = headingType_GameNo;
        else if (flags & 0x0008)
          Info.headingType = headingType_None;
        Info.includeInfo = ((flags & 0x0010) != 0);
        Info.pageBreak = ((flags & 0x0020) != 0);
        break;

      case 'INFO':
        FetchStr(Info.event);        // Str63    event
        FetchStr(Info.site);         // Str63    site
        FetchStr(Info.date);         // Str15    date
        FetchStr(Info.round);        // Str15    round
        FetchStr(Info.whiteName);    // Str63    white name
        FetchStr(Info.blackName);    // Str63    black name
        Info.whiteELO = FetchInt();  // INT      white ELO
        Info.blackELO = FetchInt();  // INT      black ELO
        Info.result = FetchInt();    // INT      result
        FetchStr(dummy);             // Str255   comment
        break;

      case 'INIT':
        Init.wasSetup = FetchInt();  // BOOL     wasSetup
        if (!Init.wasSetup)
          NewGame(false);
        else {
          Init.player = FetchInt();          // COLOUR   initialPlayer
          Init.castlingRights = FetchInt();  // INT      castlingFlags
          Init.epSquare = FetchInt();        // SQUARE   epSquare
          Init.revMoves = FetchInt();        // INT      initIrrMoves
          Init.moveNo = FetchInt();          // INT      initialMoveNo

          for (SQUARE sq = a1; sq <= h8; sq++)  // BYTE     initialPos[64]
            if (onBoard(sq)) Init.Board[sq] = FetchByte();

          ResetGame(false);
        }
        break;

      case 'gmfl':
        jmax = FetchInt();
        for (INT j = 1; j <= jmax; j++)
          Record[j].misc = (FetchByte() & 0x07);  // Only load annotation glyph
        break;

      case 'MOVE':
        FetchInt();  //###CURRENT GAME MOVE?!?
        jmax = FetchInt();

        if (calcMoveFlags) {
          for (INT j = 1; j <= jmax; j++) {
            Move_Unpack(FetchInt(), Board, &m);
            PlayMove(&m);
          }
        } else {
          lastMove = jmax;  // Set last game move.
          while (currMove < lastMove) {
            Move_Unpack(FetchInt(), Board, &Record[currMove + 1]);
            RedoMove(false);
            Record[currMove].flags = 0;
            moveCount = 1;
            result = CalcGameResult();  // Necessary??
          }
        }

        UndoAllMoves();  // Go back to initial position.
        break;

      case 'ann ':
        ANNPTR4 annPtr = (ANNPTR4)(Packet->Data);
        LONG annSize = Packet->size;
        LONG i = 0;
        while (i < annSize) {
          SetAnnotation(annPtr->moveNo, (CHAR *)(annPtr->Data),
                        annPtr->charCount);
          i += annPtr->size;
          annPtr = (ANNPTR4)(((PTR)annPtr) + annPtr->size);
        }
        break;
    }

    NextPacket();

  } while ((PTR)Packet < GameData + totalSize);
  //      Info.result = result;  ????
  dirty = false;
} /* CGame::Read_V34 */

/*---------------------------------- Utility - Packet Access
 * -------------------------------------*/

void CGame::FirstPacket(PTR GameData) {
  Packet = (PACKET *)GameData;
  packInx = 0;
} /* CGame::FirstPacket */

void CGame::NextPacket(void) {
  LONG psize = sizeof(PACKET) + Packet->size;  // Calc total packet size and
  if (odd(psize)) psize++;                     // make sure it's even.
  Packet = (PACKET *)((BYTE *)Packet + psize);
  packInx = 0;
} /* NextPacket */

void CGame::StoreInt(INT n) {
  *((INT *)&Packet->Data[packInx]) = n;
  packInx += sizeof(INT);
} /* CGame::StoreInt */

void CGame::StoreLong(LONG n) {
  *((LONG *)&Packet->Data[packInx]) = n;
  packInx += sizeof(LONG);
} /* CGame::StoreLong */

void CGame::StoreStr(
    CHAR *s)  // Note: Strings are always STORED as Pascal strings
{
  LONG inx0 = packInx;

  packInx++;  // Make room for length byte
  while (*s)  // Copy the string (except null terminator)
    StoreByte(*(s++));
  Packet->Data[inx0] = packInx - inx0 - 1;  // Store length byte
  if (odd(packInx)) packInx++;              // Make packet size even
} /* CGame::StoreStr */

INT CGame::FetchInt(void) {
  INT n = *((INT *)&Packet->Data[packInx]);
  packInx += sizeof(INT);
  return n;
} /* CGame::FetchInt */

LONG CGame::FetchLong(void) {
  LONG n = *((LONG *)&Packet->Data[packInx]);
  packInx += sizeof(LONG);
  return n;
} /* CGame::FetchLong */

void CGame::FetchStr(
    CHAR *s)  // Note: Strings are always STORED as Pascal strings
{
  for (INT len = FetchByte(); len > 0; len--) *(s++) = FetchByte();
  *s = 0;  // Null terminate!
  if (odd(packInx)) packInx++;
} /* CGame::FetchStr */

/**************************************************************************************************/
/*                                                                                                */
/*                                     · 2.0 FORMAT (AND EARLIER) */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------------- Saving
 * --------------------------------------------*/
// The · 1.3/2.0 format ('·GAM') only contains move information (but no
// annotations, PGN tags e.t.c) Saving in earlier pre-· formats ('CHGM' and
// 'XLGM') are NOT supported.

LONG CGame::Write_V2(PTR Data) {
  INT i = 0;

  // Write version (byte 0)
  Data[i++] = 2;

  // Write player names (white = byte 1..64, black = 65..128)
  C2P_Str(Info.whiteName, &Data[i]);
  i += 64;
  C2P_Str(Info.blackName, &Data[i]);
  i += 64;

  // Write initial pos info (byte 129...198)
  Data[i++] = Init.wasSetup;
  Data[i++] = Init.player;
  for (SQUARE sq = a1; sq <= h8; sq++)
    if (onBoard(sq)) Data[i++] = Init.Board[sq];

  // Write game record (byte 199...)
  for (INT j = 1, i = 0; j <= lastMove; j++) {
    Data[i++] = Record[j].from;
    Data[i++] = Record[j].to;
    Data[i++] = Record[j].type;
    Data[i++] = 0;
  }

  Data[i++] = nullSq;  // Add a trailing null move.
  Data[i++] = nullSq;
  Data[i++] = 0;
  Data[i++] = 0;

  return i;
} /* CGame::Write_V2 */

/*------------------------------------------- Loading
 * --------------------------------------------*/
// Supports loading of both the version 1.3/2.0 format ('·GAM'), and the pre-·
// formats ('CHGM' and 'XLGM'). In the latter two, byte 0 indicates if the
// initial position was setup (0 = false, 1 = true), whereas the former format
// always sets byte 0 to 2.

void CGame::Read_V2(PTR Data) {
  INT i = 0;

  NewGame(true);
  ClearGameInfo();  // Do this because NewGame uses "Default" game info rather
                    // than blank

  INT version = Data[i++];

  if (version < 2)
    Init.wasSetup = !version;
  else {
    i++;
    P2C_Str(&Data[i], Info.whiteName);
    i += 64;
    P2C_Str(&Data[i], Info.blackName);
    i += 64;
    Init.wasSetup = Data[i++];
  }

  if (Init.wasSetup) {
    Init.player = Data[i++];
    for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq)) Init.Board[sq] = Data[i++];
    ResetGame(false);
  } else if (version == 2)
    i += 65;

  while (onBoard(Data[i]))  // Load game record.
  {
    MOVE m;

    m.from = Data[i++];
    m.to = Data[i++];
    m.type = Data[i++];
    if (version == 2) i++;
    m.piece = Board[m.from];
    m.cap = Board[m.to];
    m.misc = 0;
    PlayMove(&m);
  }

  UndoAllMoves();
  dirty = false;
} /* CGame::Read_V2 */
