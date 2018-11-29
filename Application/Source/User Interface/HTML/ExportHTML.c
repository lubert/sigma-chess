/**************************************************************************************************/
/*                                                                                                */
/* Module  : ExportHTML.c */
/* Purpose : This module implements HTML export of games (including diagrams
 * etc).                */
/*           The basic logic/structure resembles that of the printing routines.
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

#include "ExportHTML.h"

/*
HTML export config/formatting

User must specify

HTML Export/Config dialog
Maybe put config stuff in Prefs dialog (new sheet)?
Important to inform user, that the "gif" folder must be located
at the correct location.

-----------------------------------------------------------------------------

   [x] Generate complete HTML document (i.e. with header, body tags)

   Background
   (*) Default (white)
   (*) Custom color [___]
   ( ) Custom image [back.gif   ]

   Board/piece graphics folder (relative to HTML document) [gif    ]

                                                      [Cancel] [ Export ]

-----------------------------------------------------------------------------

Export opens file save dialog

   Fo
        Indicate if complete HTML (i.e. with header and body tags)
        Background color or image
        Foreground color (for text)
        Link colors
        Font face, size, color (or default) for heading and main text

        Columns
           Count (default 2)
           Width (default 200)
*/

void HTMLGifReminder(CWindow *parent) {
  if (Prefs.Misc.HTMLGifReminder)
    Prefs.Misc.HTMLGifReminder =
        !ReminderDialog(parent, "HTML Export",
                        "For diagrams to be shown you must export the HTML "
                        "file to a location where a ÒgifÓ piece set folder "
                        "exists (e.g. in the ÒPlug-ins/HTMLÓ folder)");
} /* HTMLGifReminder */

/**************************************************************************************************/
/*                                                                                                */
/*                                    CONSTRUCTOR/DESTRUCTOR */
/*                                                                                                */
/**************************************************************************************************/

CExportHTML::CExportHTML(CHAR *theTitle,
                         CFile *theFile)  //, HTML_OPTIONS *Options)
{
  CopyStr(theTitle, title);
  CopyStr("gif", gifPath);  //### Can be set by user as well (Prefs)
  bytes = 0;
  file = theFile;
  file->Open(filePerm_RdWr);
  game = new CGame();
} /* CExportHTML::CExportHTML */

CExportHTML::~CExportHTML(void) {
  FlushBuffer();
  file->Close();
  delete game;
} /* CExportHTML::~CExportHTML */

/**************************************************************************************************/
/*                                                                                                */
/*                                       PUBLIC EXPORT ROUTINES */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------------ Export Game
 * -----------------------------------------*/

void CExportHTML::ExportGame(CGame *theGame) {
  game->CopyFrom(theGame);

  ResetBuffer();

  WriteLine("<html>");
  Write_Header();
  Write_BodyStart();

  ExportOneGame(false, false);

  Write_BodyEnd();
  WriteLine("</html>");
} /* CExportHTML::ExportGame */

/*--------------------------------------- Export Collection
 * --------------------------------------*/

void CExportHTML::ExportCollection(SigmaCollection *collection, LONG start,
                                   LONG end) {
  if (collection->Info.title[0]) CopyStr(collection->Info.title, title);

  if (collection->Publishing())
    ProVersionDialog(nil,
                     "Please note that Sigma Chess Lite does NOT include "
                     "diagrams when exporting game collections.");

  CHAR progressStr[100];
  Format(progressStr, "Exporting the game collection Ò%sÓ...", title);
  ULONG gameCount = end - start + 1;
  collection->BeginProgress("Export Collection", progressStr, end - start + 1,
                            true);

  ResetBuffer();

  WriteLine("<html>");
  Write_Header();
  Write_BodyStart();

  Write_FrontPage(collection);

  for (LONG i = start; i <= end && !collection->ProgressAborted(); i++) {
    gameNo = collection->View_GetGameNo(i);
    collection->View_GetGame(i, game);
    ExportOneGame(true, collection->Publishing());

    Format(progressStr, "Game %d of %d", i - start + 1, gameCount);
    collection->SetProgress(i - start + 1, progressStr);
  }

  collection->EndProgress();

  Write_BodyEnd();
  WriteLine("</html>");
} /* CExportHTML::ExportCollection */

/**************************************************************************************************/
/*                                                                                                */
/*                                      MAIN EXPORT ROUTINE */
/*                                                                                                */
/**************************************************************************************************/

void CExportHTML::ExportOneGame(BOOL isCollectionGame, BOOL isPublishing) {
  //--- First export header info/lines etc.

  if (!isCollectionGame) {
    Write("<h3><i>");
    Write(title);
    WriteLine("</i></h3>");
  }

  if (!isPublishing)
    WriteLine("<hr><br>");
  else if (isCollectionGame && game->Info.pageBreak)
    WriteLine("<br><hr>");

  //--- Then export the actual lines (incl. diagrams) from the game map:

  INT Nmax = game->CalcGameMap(game->lastMove, GMap, true, isCollectionGame,
                               isPublishing);

  game->UndoAllMoves();

  for (INT N = 0; N < Nmax; N++) {
    BOOL prevLineMove = (N > 0 && (GMap[N - 1].moveNo & gameMap_Move));
    BOOL thisLineMove = (GMap[N].moveNo & gameMap_Move);

    if (!prevLineMove && thisLineMove)
      WriteLine("<blockquote><b>");
    else if (prevLineMove && !thisLineMove)
      WriteLine("</b></blockquote>");

    if (!game->GameMapContainsDiagram(GMap, N) ||
        (isCollectionGame && !ProVersion()))
      Write_GameLine(N, Nmax, gameNo);
    else
      Write_Diagram();

    if (thisLineMove && N == Nmax - 1) WriteLine("</b></blockquote>");
  }

  //--- Add collection games separator line:
  if (isCollectionGame) WriteLine("<br>");
} /* CExportHTML::ExportOneGame */

/**************************************************************************************************/
/*                                                                                                */
/*                                        HTML "OBJECTS" */
/*                                                                                                */
/**************************************************************************************************/

void CExportHTML::Write_Header(void) {
  WriteLine("<head>");
  WriteLine(
      "   <meta http-equiv=\"Content-Type\" content=\"text/html; "
      "charset=iso-8859-1\">");
  WriteLine("   <meta name=\"KeyWords\" content=\"Chess\">");
  WriteLine("   <meta name=\"Generator\" content=\"Sigma Chess 6.2\">");
  Format(str, "   <meta name=\"Author\" content=\"%s\">",
         Prefs.General.playerName);
  WriteLine(str);
  WriteLine(
      "   <meta name=\"Content-Type\" content=\"text/html; "
      "charset=iso-8859-1\">");
  Format(str, "   <title>%s</title>", title);
  WriteLine(str);
  WriteLine("</head>");
} /* CExportHTML::Write_Header */

void CExportHTML::Write_BodyStart(void) {
  WriteLine("<body>");
} /* CExportHTML::Write_BodyStart */

void CExportHTML::Write_BodyEnd(void) {
  WriteLine("</body>");
} /* CExportHTML::Write_BodyEnd */

void CExportHTML::Write_GameLine(INT N, INT Nmax, LONG gameNo) {
  if (N >= Nmax) return;

  INT j = GMap[N].moveNo & 0x0FFF;  // Game move number (index in Game[]).

  if (GMap[N].moveNo &
      gameMap_Special)  // Line contains header (title and/or game info)
  {
    Write_Special(GMap[N].txLine, &game->Info, gameNo);
  } else if (GMap[N].moveNo & gameMap_White)  // Line contains a WHITE move (and
                                              // perhaps a black one too).
  {
    Format(str, "%d", j / 2 + game->Init.moveNo);
    Write(str);
    Write(" ");
    CalcGameMoveStr(&game->Record[j], str);
    Write(str);
    Write(" ");

    game->RedoMove(false);
    if (GMap[N].moveNo & gameMap_Black)
      CalcGameMoveStr(&game->Record[j + 1], str), Write(str),
          game->RedoMove(false);
    else if (N < Nmax - 1 && j < game->lastMove)
      Write(". . .");
    WriteLine("<br>");
  } else if (GMap[N].moveNo &
             gameMap_Black)  // Line contains a BLACK move only.
  {
    Format(str, "%d", (j - 1) / 2 + game->Init.moveNo);
    Write(str);  //### Tab
    Write(" . . . ");
    CalcGameMoveStr(&game->Record[j], str);
    Write(str);  //### Tab
    game->RedoMove(false);
    WriteLine("<br>");
  } else  // Line contains a LINE of annotation text/data.
  {
    CHAR s[500];
    BOOL nl;
    INT lineNo = GMap[N].txLine & 0x0FFF;
    INT lineLen = game->GetAnnotationLine(j, lineNo, s, &nl);
    s[lineLen] = 0;
    Write(s);
    WriteLine(nl ? "<br>" : "");
  }
} /* CExportHTML::Write_GameLine */

void CExportHTML::Write_Diagram(void) {
  CHAR HTMLPieceChar[8] = "-pnbrqk";

  WriteLine("<p><center>");

  for (INT rank = 7; rank >= 0; rank--) {
    Write("      ");

    for (INT file = 0; file <= 7; file++) {
      SQUARE sq = square(file, rank);
      PIECE piece = game->Board[sq];
      CHAR id[4];  // GIF file Id = color (w/b) + piece (pnbrqk) + square color
                   // (w/b)
      INT i = 0;

      if (piece) {
        id[i++] = (pieceColour(piece) == white ? 'w' : 'b');
        id[i++] = HTMLPieceChar[pieceType(piece)];
      }
      id[i++] = (odd(file + rank) ? 'w' : 'b');
      id[i] = 0;

      Format(str, "<img src=\"%s/%s.gif\"%s>", gifPath, id,
             (piece ? "" : "  "));
      Write(str);
    }

    WriteLine("<br>");
  }

  WriteLine(game->player == white ? "<i>White to move</i><br>"
                                  : "<i>Black to move</i><br>");

  WriteLine("</center></p>");
} /* CExportHTML::Write_Diagram */

void CExportHTML::Write_Special(INT type, GAMEINFO *Info, LONG gameNo) {
  CHAR temp[20];
  CHAR *tag = nil, *s = nil;

  switch (type) {
    case gameMap_SpecialChapter:
      Write("<h1>");
      Write(Info->heading);
      WriteLine("</h1>");
      break;
    case gameMap_SpecialSection:
      Write("<h2>");
      Write(Info->heading);
      WriteLine("</h2>");
      break;
    case gameMap_SpecialGmTitle:
      Format(temp, "Game %d", gameNo + 1);
      Write("<h3></i>");
      Write(temp);
      WriteLine("</i></h3>");
      break;

    case gameMap_SpecialWhite:
      tag = "White";
      s = Info->whiteName;
      break;
    case gameMap_SpecialBlack:
      tag = "Black";
      s = Info->blackName;
      break;
    case gameMap_SpecialEvent:
      tag = "Event";
      s = Info->event;
      break;
    case gameMap_SpecialSite:
      tag = "Site";
      s = Info->site;
      break;
    case gameMap_SpecialDate:
      tag = "Date";
      s = Info->date;
      break;
    case gameMap_SpecialRound:
      tag = "Round";
      s = Info->round;
      break;
    case gameMap_SpecialResult:
      tag = "Result";
      s = temp;
      CalcInfoResultStr(Info->result, temp);
      break;
    case gameMap_SpecialECO:
      tag = "ECO";
      s = Info->ECO;
      break;
  }

  if (tag && s) {
    Write("<b> ");
    Write(tag);
    Write("</b> ");
    Write(s);
    WriteLine("<br>");
  }
} /* CExportHTML::Write_Special */

void CExportHTML::Write_FrontPage(SigmaCollection *collection) {
  if (!collection->Publishing()) return;

  // Draw top horisontal line:
  WriteLine("<hr>");

  // Draw collection title:
  Write("<h1><center>");
  Write(collection->Info.title);
  WriteLine("</center></h1><br>");

  // Draw name of author:
  Write("<i><center>");
  Write(collection->Info.author);
  WriteLine("</center></i><br>");

  // Draw bottom horisontal line:
  WriteLine("<hr><br><br>");

  // Draw collection description:
  if (collection->Info.descr[0]) {
    WriteLine("<br><br>");
    Write(collection->Info.descr);
    WriteLine("<br><br><hr><br><br>");
  }
} /* CExportHTML::Write_FrontPage */

/**************************************************************************************************/
/*                                                                                                */
/*                                            MISC */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------- HTML Write Buffer Handling
 * -----------------------------------*/

void CExportHTML::ResetBuffer(void) {
  bytes = 0;
} /* CExportHTML::ResetBuffer */

void CExportHTML::FlushBuffer(void) {
  if (bytes <= 0) return;
  file->Write(&bytes, (PTR)htmlBuf);
  ResetBuffer();
} /* CExportHTML::FlushBuffer */

void CExportHTML::Write(CHAR *s) {
  while (*s) {
    if (bytes >= htmlBufSize) FlushBuffer();
    htmlBuf[bytes++] = *(s++);
  }
} /* CExportHTML::Write */

void CExportHTML::WriteLine(CHAR *s) {
  Write(s);
  Write("\n");
} /* CExportHTML::WriteLine */
