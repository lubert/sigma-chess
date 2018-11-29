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

/* ChessEvents.c */

#if 0
	ADDING SUPPORT FOR THE 'CHES' EVENT
	-----------------------------------
	To add 'CHES' event support to an existing chess program depends on whether
	Apple Events are already supported or not. Here's what to do if Apple Events
	are NOT already supported:
	
	1.	Set the 'size' resource flag 'High Level Event Aware' in the THINK C or
		Metrowerks project options.
	
	2.	In your main, just before your main event loop, do

			InitAppleEvents (NULL, NULL);

	4.	In your main event loop, handle the case of kHighLevelEvent:
	
			switch (event->what) {
				...
				case kHighLevelEvent:
					AEProcessAppleEvent (event);
					break;
				...
				}
	
	5.	Optionally, add an aete resource to make the application scriptable.
		Just copy this resource from some other file if you don't have an aete
		already. Otherwise you need to merge it.
			
	
	FUNCTIONS DEFINED IN THIS FILE
	------------------------------
	The external functions defined in this file can be tailored by defining or
	undefining three preprocessor symbols:
		NO_PRINTF
		INIT_APPLE_EVENTS
		CHESS_EVENT_TOOLS

	- If NO_PRINTF is defined, calls to printf in this file don't do anything.
	  Undefine for debugging. Doesn't actually affect defined external functions.
	  
	- If INIT_APPLE_EVENTS is defined, this file defines 5 functions for handling
	  AppleEvents.
	
		InitAppleEvents
		DoOAPPEvent
		DoODOCEvent
		DoPDOCEvent
		DoQUITEvent
	
	- This file always defines two functions for processing the CHES event. The 
	  caller must provide a function ChessAction in a separate glue file.
	
		DoCHESEvent
		SendMessage
	
	- If CHES_EVENT_TOOLS is defined, this file defines 4 functions to assist in
	  parsing and formatting arguments passed by the CHES event. The caller must
	  provide the four functions GetPiece, GenerateMoves, CheckMove, MateMove.
	
		ParseSetup
		FormatSetup
		ParseMove
		FormatMove
	

	THE 'CHES' EVENT
	----------------
	The 'CHES' event allows one program to call upon another program to provide
	the services of a chess engine. A chess engine is software that can
	manipulate a chessboard, generate legal moves, and search for the best move.
	The event supports a set of eventIDs that take no argument or a single
	string argument (a keyDirectObject of type typeChar), and return no reply
	(the 'init' eventID) or a reply with an optional string argument. 

		Cmd		Cmd name	Cmd arg		Reply arg
		-----------------------------------------
		gtbd	get board	-			<pos>		
		newg	new game	-			-
		stbd	set board	<pos>		-			
		move	move		<moves>		<move>
		back	back		<n>			-
		srch	search		-			-
		canc	cancel		-			-
	
	All events generate a reply.
	The sending program waits for a reply only in the case of 'gtbd' and 'srch'.
	
	Protocol
	--------
	ExaChess uses an external chess engine when the user selects Play from 
	ExaChess's menu. If the engine is not already running, ExaChess will launch 
	it in the background with an aevt/oapp event.
	
	It then sends it a CHES/newg or CHES/stbd event to set up a board
	position, possibly followed by a CHES/move event to play an initial set of
	moves.
	
	It then sends a CHES/srch event to request it to search for a move.
	
	If ExaChess launched the engine, then when the user closes the last game 
	window using that engine, ExaChess sends it an aevt/quit event.
	

	TO DO
	-----
	-	Add 'game' and 'movl' eventIDs to get game history, legal move list
	-	Add 'get' and 'set' events that support the following args:
	
	timeControl		A string that sets the time controls for the game <to be spec'd>
	realTime		If true, the clock runs on real time, otherwise it is stopped
					whenever the CPU is given to other processes
	useBook			If true, the book is used if possible
	eval			Return static evaluation of current position in centiPawns.
					>0 is good for White ['get' only]

		For the current or last completed search:
		
	bestline		Best line found so far
	besteval		Evaluation at end of best line
	wasBookMove;	True if last search result was book move
	nodes;			Nodes searched. Starts at 0 on each search
	ticks;			Ticks elapsed since search started

#endif

/* Includes
   -------- */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ChessEvents.h"

/* Globals
   ------- */

short gReturnID; /* return ID of timed-out search event */
pascal AEFilterProcType_ (*gAEIdleProc)(EventRecord *theEvent, long *sleepTime,
                                        RgnHandle *mouseRgn);
pascal AEFilterProcType_ (*gAEFilterProc)(EventRecord *theEvent, long returnID,
                                          long transactionID,
                                          const AEAddressDesc *address);

/* Universal headers
   ----------------- */

#ifndef __CONDITIONALMACROS__
#include <StandardFile.h>
#define AEIdleUPP FileFilterProcPtr
#define AEFilterUPP DlgHookYDProcPtr
#define NewAEEventHandlerProc(x) x
#define NewAEIdleProc(x) ((IdleProcPtr)(x))
#define NewAEFilterProc(x) ((EventFilterProcPtr)(x))
#endif

/* Static globals
   -------------- */

static AEDesc gSelf; /* target for Apple Events to self */

static AEIdleUPP appleEventIdleUPP;
static AEFilterUPP appleEventFilterUPP;

/* Local prototypes
   ---------------- */

static pascal OSErr DoOAPPEvent(AppleEvent *event, AppleEvent *reply,
                                long refcon);
static pascal OSErr DoODOCEvent(AppleEvent *event, AppleEvent *reply,
                                long refcon);
static pascal OSErr DoPDOCEvent(AppleEvent *event, AppleEvent *reply,
                                long refcon);
static pascal OSErr DoQUITEvent(AppleEvent *event, AppleEvent *reply,
                                long refcon);
static pascal OSErr DoFirstEvent(AppleEvent *event, AppleEvent *reply,
                                 long refcon);
static pascal AEFilterProcType_ AEIdleProc(EventRecord *theEvent,
                                           long *sleepTime,
                                           RgnHandle *mouseRgn);
static pascal AEFilterProcType_ AEFilterProc(EventRecord *theEvent,
                                             long returnID, long transactionID,
                                             const AEAddressDesc *address);

static int strcasecmp(char *a, char *b);

/* Tailoring of external functions
   ------------------------------- */

#define NO_PRINTF
//#define INIT_APPLE_EVENTS
#define CHES_EVENT_TOOLS

#ifdef NO_PRINTF
#define printf no_printf
static void no_printf(char *format, ...) {}
#endif

#ifdef INIT_APPLE_EVENTS

/*==== AEIdleProc ====*/

static pascal AEFilterProcType_ AEIdleProc(EventRecord *theEvent,
                                           long *sleepTime,
                                           RgnHandle *mouseRgn) {
  return (gAEIdleProc ? (*gAEIdleProc)(theEvent, sleepTime, mouseRgn) : 0);
}

/*==== AEFilterProc ====*/

static pascal AEFilterProcType_ AEFilterProc(EventRecord *theEvent,
                                             long returnID, long transactionID,
                                             const AEAddressDesc *address) {
  return (gAEFilterProc
              ? (*gAEFilterProc)(theEvent, returnID, transactionID, address)
              : -1);
}

/*==== InitAppleEvents ====*/
/*
        Initialize Apple events. If firstEvent is passed as NULL, then get the
        first event and process it. Otherwise get the first event and return
        it in firstEvent; the caller must resume this event by calling
        AEResumeTheCurrentEvent to actually process it when initialization
        is complete.

        Exit:	function result = error code.
                        firstEvent = event record for first event.
                        firstReply = reply record for first event.
*/

OSErr InitAppleEvents(AppleEvent *firstEvent, AppleEvent *firstReply) {
  static AppleEvent firstevent = {0, NULL}, firstreply = {0, NULL};
  OSErr err = noErr;
  Boolean gotEvt;
  EventRecord ev;
  ProcessSerialNumber thePSN = {0, kCurrentProcess};

  if (firstEvent == NULL) firstEvent = &firstevent, firstReply = &firstreply;

  /* Make target for messages to self */

  err = AECreateDesc(typeProcessSerialNumber, (Ptr)&thePSN, sizeof(thePSN),
                     &gSelf);
  if (err != noErr) return (err);

  err = AEInstallEventHandler(typeWildCard, typeWildCard,
                              NewAEEventHandlerProc(DoFirstEvent), 0, false);
  if (err != noErr) return err;

  for (;;) {
    gotEvt = WaitNextEvent(highLevelEventMask, &ev, GetCaretTime(), NULL);
    if (gotEvt && ev.what == kHighLevelEvent) {
      err = AEProcessAppleEvent(&ev);
      if (err != noErr) return err;
      break;
    }
  }

  err = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
                              NewAEEventHandlerProc(DoOAPPEvent), 0, false);
  if (err != noErr) return err;
  err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
                              NewAEEventHandlerProc(DoODOCEvent), 0, false);
  if (err != noErr) return err;
  err = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
                              NewAEEventHandlerProc(DoPDOCEvent), 0, false);
  if (err != noErr) return err;
  err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
                              NewAEEventHandlerProc(DoQUITEvent), 0, false);
  if (err != noErr) return err;
  err = AEInstallEventHandler('CHES', typeWildCard,
                              NewAEEventHandlerProc(DoCHESEvent), 0, false);

  if (err == noErr && firstEvent == &firstevent) {
    AEResumeTheCurrentEvent(firstEvent, firstReply,
                            (AEEventHandlerUPP)kAEUseStandardDispatch, 0);
  }

  return err;
}

/*==== DoFirstEvent - handle first event event ====*/

static pascal OSErr DoFirstEvent(AppleEvent *event, AppleEvent *reply,
                                 long refcon) {
  return AESuspendTheCurrentEvent(event);
}

/*==== DoOAPPEvent - handle OAPP event ====*/

static pascal OSErr DoOAPPEvent(AppleEvent *event, AppleEvent *reply,
                                long refcon) {
#if 0 /* enable this for testing */
	char	replymsg[256];
	
	SendMessage (&gSelf, 'newg', NULL, NULL);
	SendMessage (&gSelf, 'move', "1 e4 e5 2 Nf3 Nc6 3 Bc4", NULL);
	SendMessage (&gSelf, 'srch', "", replymsg);
	printf ("%s\n", replymsg);
#endif

  return noErr;
}

/*==== DoODOCEvent - handle ODOC event ====*/

static pascal OSErr DoODOCEvent(AppleEvent *event, AppleEvent *reply,
                                long refcon) {
  printf("odoc\n");
  return noErr;
}

/*==== DoPDOCEvent - handle PDOC event ====*/

static pascal OSErr DoPDOCEvent(AppleEvent *event, AppleEvent *reply,
                                long refcon) {
  printf("pdoc\n");
  return noErr;
}

/*==== DoQUITEvent - handle QUIT event ====*/

static pascal OSErr DoQUITEvent(AppleEvent *event, AppleEvent *reply,
                                long refcon) {
  printf("quit\n");
  ChessAction('quit', "", "");
  return noErr;
}

#endif

/*==== DoCHESEvent - handle CHES event ====*/

pascal OSErr DoCHESEvent(const AppleEvent *event, AppleEvent *reply,
                         long refcon) {
  char msg[2560], replymsg[2560];
  DescType eventID, typeCode;
  DescType actualType;
  Size actualSize;
  OSErr err = noErr;
  AEDesc stringDesc = {0, NULL};

  /* Get the event ID */

  err = AEGetAttributePtr(event, keyEventIDAttr, typeType, &actualType,
                          (Ptr)&eventID, sizeof(eventID), &actualSize);
  if (err != noErr) return err;

  /* Get string parameter */

  *msg = 0;
  if (eventID == 'stbd' || eventID == 'move' || eventID == 'back' ||
      eventID == 'srch' || eventID == 'spar' || eventID == 'stat' ||
      eventID == 'smov') {
    err = AEGetParamPtr(event, keyDirectObject, typeChar, &typeCode, msg, 2560,
                        &actualSize);
    if (err != noErr) return err;
    msg[actualSize] = 0;

#if 0
		{
		long	returnID;
		DescType	actualType;
		err = AEGetAttributePtr (event, keyReturnIDAttr, typeInteger, &actualType, (Ptr)&returnID, 
			sizeof(returnID), &actualSize);
		if (err != noErr) printf ("err = %d\n", (int)err);
		printf("Receiving ID=%ld\n", returnID);
		}
#endif
  }
  printf("Rcvd: %.4s %s\n", &eventID, msg);
  if (eventID == 'init' || eventID == 'disp') return (noErr);

  /* Process message */

  ChessAction(eventID, msg, replymsg);

  /* Return reply */

  printf("Replying: %s\n", replymsg);
  err = AECreateDesc(typeChar, replymsg, strlen(replymsg), &stringDesc);
  if (err != noErr) return err;
  err = AEPutParamDesc(reply, keyDirectObject, &stringDesc);
  if (err != noErr) return err;

  return noErr;
}

/*==== SendMessage - send message via Apple Events ====*/

OSErr SendMessage(AEDesc *target, DescType eventID, char *msg, char *replymsg) {
  AEDesc stringDesc = {0, NULL};
  AEDesc theTarget = *target; /* shadow copy */
  AppleEvent theEvent = {0, NULL};
  AppleEvent theReply = {0, NULL};
  DescType typeCode, actualType;
  OSErr err = noErr;
  Size actualSize;
  char replystr[256]; /* used if replymsg is NULL */
  AESendMode sendMode;

  if (appleEventIdleUPP == NULL) {
    appleEventIdleUPP = NewAEIdleUPP(AEIdleProc);
    appleEventFilterUPP = NewAEFilterUPP(AEFilterProc);
  }

  if (replymsg == NULL) replymsg = replystr;

  gReturnID = 0;

  if (eventID == 'quit') {
    err = AECreateAppleEvent('aevt', 'quit', &theTarget, kAutoGenerateReturnID,
                             kAnyTransactionID, &theEvent);
    if (err != noErr) goto exit;
    printf("Sending quit\n");
    err = AESend(&theEvent, &theReply, kAENoReply, kAENormalPriority,
                 1 * 60 /*ticks*/, appleEventIdleUPP, appleEventFilterUPP);
    if (err != noErr) goto exit;
    return err;
  }

  err = AECreateAppleEvent('CHES', eventID, &theTarget, kAutoGenerateReturnID,
                           kAnyTransactionID, &theEvent);
  if (err != noErr) goto exit;

  if (msg) {
    err = AECreateDesc(typeChar, msg, strlen(msg), &stringDesc);
    if (err != noErr) goto exit;
    err = AEPutParamDesc(&theEvent, keyDirectObject, &stringDesc);
    if (err != noErr) goto exit;
  }

  printf("Sending %.4s %s\n", &eventID, msg ? msg : "");

  sendMode =
      (eventID == 'gtbd' || eventID == 'srch') ? kAEWaitReply : kAENoReply;

  err = AESend(&theEvent, &theReply, sendMode, kAENormalPriority,
               1 * 60 /*ticks*/, appleEventIdleUPP, appleEventFilterUPP);

  if (err == errAETimeout && eventID == 'srch') {
    long aReturnID = 0;
    err =
        AEGetAttributePtr(&theEvent, keyReturnIDAttr, typeInteger, &actualType,
                          (Ptr)&aReturnID, sizeof(aReturnID), &actualSize);
    if (err != noErr) goto exit;
    gReturnID = (short)aReturnID;
  }

  if (err != noErr) {
    printf("Send error:%d\n", (int)err);
    goto exit;
  }

  err = AEGetParamPtr(&theReply, keyDirectObject, typeChar, &typeCode, replymsg,
                      255, &actualSize);
  if (err != noErr) goto exit;
  replymsg[actualSize] = 0;
  printf("Got reply %s\n", replymsg);

exit:

  if (stringDesc.dataHandle) AEDisposeDesc(&stringDesc);
  if (theEvent.dataHandle) AEDisposeDesc(&theEvent);
  if (theReply.dataHandle) AEDisposeDesc(&theReply);
  return err;
}

#ifdef CHES_EVENT_TOOLS

/*
        The functions here are helper functions to help programs process
        the arguments passed in CHES events. Many programs already have
        these functions, so the functions here are not needed. To deal with
        the many representations of chess boards and moves, these functions
        use the types Board and SMove defined in ChessEvents.h. Each program
        must convert its internal representation to these types; hopefully
        this is a very simple step.
*/

/*==== ParseSetup - parse setup string ====*/

void ParseSetup(Board *aBoard, char *str) {
  short i = 0, j, sq, sqi, wKing, bKing, piece, *board = aBoard->theBoard;
  char c, *p;
  char *PieceNames = "PRNBQKprnbqk";

  memset(aBoard, 0, sizeof(Board));
  if (str[i] == '/') i++;

  /* Get pieces */

  for (i = sqi = wKing = bKing = 0; (c = str[i]) != 0 && sqi < 64; i++) {
    sq = sqi + 8 * (7 - 2 * Rank(sqi));
    p = strchr(PieceNames, toupper(c));
    piece = (p) ? p - PieceNames + (isupper(c) ? WPAWN : BPAWN) : EMPTY;

    if (piece != EMPTY || c == '.') {
      board[sq] = (c == '.') ? EMPTY : piece;
      if (c == 'K')
        wKing++;
      else if (c == 'k')
        bKing++;
      sqi++;
    } else if (c >= '1' && c <= '8') /* multiple EMPTYs */
      for (j = 0; j < c - '0'; j++, sqi++) {
        sq = sqi + 8 * (7 - 2 * Rank(sqi));
        board[sq] = EMPTY;
      }
    else if (c == '/' || c == ',' || c == ' ' || c == '\n') /* ignore */
      ;
    else
      return;
  }
  if (sqi != 64 || wKing != 1 || bKing != 1) return;

  /* Get turn */

  aBoard->turn = WHITE;
  if (str[i] == '/' || str[i] == ',' || str[i] == ' ') i++;
  if (str[i] == 'w')
    aBoard->turn = WHITE, i += strncmp(str + i, "white", 5) == 0 ? 5 : 1;
  else if (str[i] == 'b')
    aBoard->turn = BLACK, i += strncmp(str + i, "black", 5) == 0 ? 5 : 1;

    /* Get castling status */

#define casok_from_board(bd, val, _king, _rook, ksq, rsq) \
  (bd[ksq] == _king && bd[rsq] == _rook ? val : 0)

  aBoard->cstat = casok_from_board(board, 1, WKING, WROOK, 4, 7) +
                  casok_from_board(board, 2, WKING, WROOK, 4, 0) +
                  casok_from_board(board, 4, BKING, BROOK, 60, 63) +
                  casok_from_board(board, 8, BKING, BROOK, 60, 56);
  if (str[i] == '/' || str[i] == ',' || str[i] == ' ') i++;
  if (str[i] == '-' || str[i] == 'K') {
    aBoard->cstat &= (str[i] == '-' ? 0 : 1) + (str[i + 1] == '-' ? 0 : 2) +
                     (str[i + 2] == '-' ? 0 : 4) + (str[i + 3] == '-' ? 0 : 8);
    i += 4;
  }

  /* Get en passant square */

  aBoard->enpas = -1;
  if (str[i] == '/' || str[i] == ',' || str[i] == ' ') i++;
  if ('a' <= str[i] && str[i] <= 'h' &&
      (str[i + 1] == ((aBoard->turn == WHITE) ? '6' : '3') ||
       str[i + 1] == ((aBoard->turn == WHITE) ? '5' : '4'))) {
    aBoard->enpas = (str[i] - 'a') + 8 * ((aBoard->turn == WHITE) ? 5 : 2);
    i += 2;
  }

  if (str[i++] == '/') /* skip over any terminating '/' */
    i++;

  return;
}

/*==== FormatSetup ====*/

void FormatSetup(Board *aBoard, char *str) {
  char *s = str;
  short sq, cstat_from_board, emptycnt = 0, i = 0;
  short piece, *board = aBoard->theBoard;

  *s++ = '/';
  for (sq = 0; sq < 64; sq++) {
    if ((piece = board[rankflip(sq)]) == EMPTY) emptycnt++;
    if (emptycnt > 0 && (piece != EMPTY || File(sq) == 7)) {
      *s++ = '0' + emptycnt;
      emptycnt = 0;
    }
    if (piece != EMPTY) *s++ = ".PRNBQKprnbqk"[piece];
    if (File(sq) == 7) *s++ = ',';
  }
  strcpy(s, (aBoard->turn == WHITE) ? "white" : "black");
  s += 5;

  cstat_from_board = casok_from_board(board, 1, WKING, WROOK, 4, 7) +
                     casok_from_board(board, 2, WKING, WROOK, 4, 0) +
                     casok_from_board(board, 4, BKING, BROOK, 60, 63) +
                     casok_from_board(board, 8, BKING, BROOK, 60, 56);
  if (aBoard->cstat != cstat_from_board) {
    *s++ = ',';
    *s++ = (aBoard->cstat & 1) ? 'K' : '-';
    *s++ = (aBoard->cstat & 2) ? 'Q' : '-';
    *s++ = (aBoard->cstat & 4) ? 'k' : '-';
    *s++ = (aBoard->cstat & 8) ? 'q' : '-';
  } else if (aBoard->enpas != -1)
    *s++ = ',';

  if (aBoard->enpas != -1) {
    *s++ = ',';
    *s++ = 'a' + File(aBoard->enpas);
    *s++ = '1' + Rank(aBoard->enpas);
  }
  *s++ = '/';
  *s++ = 0;
}

/*==== ParseMove - parse move string ====*/

SMove ParseMove(char *movstr) {
  SMove movebuf[256], move, moveci = 0;
  int i;
  char *s;

  GenerateMoves(0, -1, movebuf, 256);

  /*	Just compare the input string with the standard output string
          for all moves. This is a quick and dirty way to parse a move
          string: it is slow and won't match non-standard input strings.
  */

  if (strcasecmp(movstr, "O-O") == 0)
    movstr = "0-0";
  else if (strcasecmp(movstr, "O-O-O") == 0)
    movstr = "0-0-0";

  for (i = 0; (move = movebuf[i]) != 0; i++) {
    if (strcmp(movstr, s = FormatMove(move)) == 0) break;
    if (strcasecmp(movstr, s) == 0) moveci = move;
  }

  return (move ? move : moveci);
}

/*==== FormatMove - format move string ====*/

char *FormatMove(SMove move) {
  int sqf, sqt, piece, i, nfile, nrank;
  static char move_string[24];
  char *capstring, tostr[12], piecechar, disambig[12];
  SMove movbuf[256], mov;

  sqf = SQF(move);
  sqt = SQT(move);
  piece = GetPiece(sqf);
  capstring = (CAPTURE(move)) ? "x" : "";
  sprintf(tostr, "%c%c", File(sqt) + 'a', Rank(sqt) + '1');

  /* The easy cases: castling, pawn moves */

  if (SPECIAL(move) && (sqf == 4 || sqf == 60))
    strcpy(move_string, (sqf < sqt) ? "0-0" : "0-0-0");

  else if (piece == WPAWN || piece == BPAWN) {
    if (CAPTURE(move))
      sprintf(move_string, "%cx%s", 'a' + File(sqf), tostr);
    else
      sprintf(move_string, "%s", tostr);
    if (Rank(sqt) == 7 || Rank(sqt) == 0)
      sprintf(move_string + strlen(move_string), "=%c", "QBNR"[UPROM(move)]);
  } else { /* Piece moves: work out the disambiguator needed */
    GenerateMoves(piece, sqt, movbuf, 256);

    nfile = nrank = 0;
    for (i = 0; (mov = movbuf[i]) != 0; i++) {
      if (mov != move && File(SQF(move)) != File(SQF(mov))) nfile++;
      if (mov != move && Rank(SQF(move)) != Rank(SQF(mov))) nrank++;
    }

    piecechar = ".PRNBQKPRNBQK"[piece];
    if (nfile == 0 && nrank == 0)
      disambig[0] = '\0';
    else if (nfile == 1)
      disambig[0] = 'a' + File(SQF(move)), disambig[1] = '\0';
    else if (nrank == 1)
      disambig[0] = '1' + Rank(SQF(move)), disambig[1] = '\0';
    else
      disambig[0] = 'a' + File(SQF(move)), disambig[1] = '1' + Rank(SQF(move)),
      disambig[2] = '\0';

    sprintf(move_string, "%c%s%s%s", piecechar, disambig, capstring, tostr);
  }

  if (CheckMove(move)) strcat(move_string, MateMove(move) ? "++" : "+");

  return (move_string);
}

/*==== strcasecmp - case insensitive strcmp ====*/

static int strcasecmp(char *a, char *b) {
  for (; *a && *b && tolower(*a) == tolower(*b); a++, b++)
    ;
  return (tolower(*a) - tolower(*b));
}

#endif
