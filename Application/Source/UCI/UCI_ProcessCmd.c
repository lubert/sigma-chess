/**************************************************************************************************/
/*                                                                                                */
/* Module  : UCI_PROCESSCMD.C                                                                     */
/* Purpose : This module implements the interface to the UCI engines.                             */
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

#include "UCI_ProcessCmd.h"
#include "UCI_Option.h"
#include "UCI_ProgressDialog.h"
#include "UCI_RegDialog.h"
#include "UCI_ConfigDialog.h"
#include "SigmaPrefs.h"
#include "Move.f"
#include "Board.f"
#include "Engine.f"
#include "Search.f"

#include "Debug.h"  // Included for UCI error handling

static void ProcessMsg_id (void);
static void ProcessMsg_uciok (void);
static void ProcessMsg_readyok (void);
static void ProcessMsg_bestmove (void);
static void ProcessMsg_copyprotection (void);
static BOOL ProcessMsg_registration (void);
static BOOL ProcessMsg_info (void);
static void ProcessMsg_option (void);

static BOOL ReadToken (CHAR *token, INT maxLen);
static INT  ReadDelimitedToken (CHAR *token, INT maxLen, CHAR *endToken1, CHAR *endToken2 = nil);
static BOOL ReadNumberToken (LONG *number);
static BOOL Read64BitNumberToken (LONG64 *number);
static BOOL MatchToken (CHAR *token);
static BOOL PeekToken (CHAR *token);
static BOOL ReadRestToken (CHAR *token, INT maxLen);   // Read until end of buffer
static CHAR *LastToken (void);
static BOOL EndOfMessage (void);
static BOOL ParseMoveToken (PIECE Board[], MOVE *m, BOOL perform);
static BOOL UCIError (CHAR *errorMsg, CHAR *info);  // Always returns false

//--- Local state variables ---
static UCI_SESSION *Session = nil;
static UCI_INFO *Info = nil;
static UCI_INFO Info0;
static BOOL UCIErr = false;
static CHAR *bufPtr0 = nil;   // Oginial Message buffer pointer
static CHAR *bufPtr = nil;    // Message buffer pointer
static INT lastRegEngineId = -1;
static INT regEngineCount = 0;


/**************************************************************************************************/
/*                                                                                                */
/*                                 MAIN ENGINE MESSAGE HANDLER                                    */
/*                                                                                                */
/**************************************************************************************************/

// A command string contains a single UCI message (without trailing newlines) 

void UCI_ProcessEngineMsg (UCI_ENGINE_ID engineId, CHAR *msg)
{
   if (debugOn)
   {  DebugWrite("<---Receive Data BEGIN--->\n");
      DebugWrite(msg);
      DebugWrite("\n<---Receive Data END--->\n");
   }

   //--- Setup state variables ---
   Info = &Prefs.UCI.Engine[engineId];
   Session = &UCISession[engineId];
   UCIErr = false;
   bufPtr = bufPtr0 = msg;

   if (! Session->uci_Sent) return;

   //--- Dispatch on the first token (which is the actual command) ---
        if (PeekToken("id"))             ProcessMsg_id();
   else if (PeekToken("uciok"))          ProcessMsg_uciok();
   else if (PeekToken("readyok"))        ProcessMsg_readyok();
   else if (PeekToken("bestmove"))       ProcessMsg_bestmove();
   else if (PeekToken("copyprotection")) ProcessMsg_copyprotection();
   else if (PeekToken("registration"))   ProcessMsg_registration();
   else if (PeekToken("info"))           ProcessMsg_info();
   else if (PeekToken("option"))         ProcessMsg_option();
   else if (Session->name_Rcvd)          UCIError("Unknown UCI Message", bufPtr);
} // UCI_ProcessEngineMsg


/**************************************************************************************************/
/*                                                                                                */
/*                                    PROCESS 'id' MESSAGES                                       */
/*                                                                                                */
/**************************************************************************************************/

// The "name" and "author" messages must be sent after receiving the "uci" command to identify the
// engine.

static void ProcessMsg_id (void)
{
   lastRegEngineId = -1;

   // ".. name <x>"
   if (PeekToken("name"))
   {
      ReadRestToken(Info->name, uci_NameLen);
      Session->name_Rcvd = true;

      Info0 = *Info;  // Backup current options before we receive the new ones
      Info->optionCount = 0;
   }
   // ".. author <x>"
   else if (PeekToken("author"))
   {
      ReadRestToken(Info->author, uci_AuthorLen);
      Session->author_Rcvd = true;
   }
   else
   {
      UCIError("Unknown 'id' command", bufPtr);
   }
} // ProcessMsg_id


/**************************************************************************************************/
/*                                                                                                */
/*                                   PROCESS 'uciok' MESSAGES                                     */
/*                                                                                                */
/**************************************************************************************************/

// "uciok"
//
// Must be sent after the id and optional options to tell the GUI that the engine
// has sent all infos and is ready in uci mode.

static void ProcessMsg_uciok (void)
{
   Session->uciok_Rcvd = true;
} // ProcessMsg_uciok


/**************************************************************************************************/
/*                                                                                                */
/*                                  PROCESS 'readyok' MESSAGES                                    */
/*                                                                                                */
/**************************************************************************************************/

// "readyok"
//
// This must be sent when the engine has received an "isready" command and has
// processed all input and is ready to accept new commands now.
// It is usually sent after a command that can take some time to be able to wait for the engine,
// but it can be used anytime, even when the engine is searching,
// and must always be answered with "isready".

static void ProcessMsg_readyok (void)
{
   Session->readyok_Rcvd = true;
} // ProcessMsg_readyok


/**************************************************************************************************/
/*                                                                                                */
/*                                 PROCESS 'bestmove' MESSAGES                                    */
/*                                                                                                */
/**************************************************************************************************/

// "bestmove <move1> [ ponder <move2> ]"
//
// the engine has stopped searching and found the move <move> best in this position.
// the engine can send the move it likes to ponder on. The engine must not start pondering automatically.
// this command must always be sent if the engine stops searching, also in pondering mode if there is a
// "stop" command, so for every "go" command a "bestmove" command is needed!
// Directly before that the engine should send a final "info" command with the final search information,
// so the GUI has the complete statistics about the last search.

static void ProcessMsg_bestmove (void)
{
   if (! Session->thinking) return;
   Session->thinking = false;
   
   ENGINE *E = Session->engineRef;
   if (! E || Engine_Aborted(E)) return;

   PIECE Board[boardSize];
   CopyTable(E->P.Board, Board);
   MOVE  m;

   // Parse best move
   if (ParseMoveToken(Board, &m, true))
   {  E->S.MainLine[0] = m;
      Engine_Stop(E);
   }
   else
   {  E->S.MainLine[0] = E->S.RootMoves[0];
      Engine_Abort(E);
      UCIError("Invalid bestmove", bufPtr0);
      return;
   }

   // Parse optional ponder move
   if (PeekToken("ponder"))
      if (ParseMoveToken(Board, &m, false))
      {  E->S.bestReply = m;
         E->S.isPonderMove = true;
      }
      else
      {  UCIError("Invalid ponder move", bufPtr0);
      }

   SendMsg_Async(E, msg_EndSearch);
//   sigmaApp->ProcessEngineMessages();
} // ProcessMsg_bestmove


/**************************************************************************************************/
/*                                                                                                */
/*                              PROCESS 'copyprotection' MESSAGES                                 */
/*                                                                                                */
/**************************************************************************************************/

// "copyprotection .."
//
// this is needed for copyprotected engines. After the uciok command the engine can tell the GUI,
// that it will check the copy protection now. This is done by "copyprotection checking".
// If the check is ok the engine should sent "copyprotection ok", otherwise "copyprotection error".
// If there is an error the engine should not function properly but should not quit alone.
// If the engine reports "copyprotection error" the GUI should not use this engine
// and display an error message instead!

static void ProcessMsg_copyprotection (void)
{
   if (PeekToken("checking"))
   {
      CHAR str[200];
      Format(str, "Checking copy protection for %s...", Info->name);
      UCI_ProgressDialogOpen("Copy Protection", str);
   }
   else if (PeekToken("ok"))
   {
      UCI_ProgressDialogClose();
   }
   else if (PeekToken("error"))
   {
      UCI_ProgressDialogClose();

      CHAR errorMsg[100];
      Format(errorMsg, "Copy protection checking failed for '%s'. This engine cannot be used...", Info->name);
      NoteDialog(nil, "Copy Protection Failed", errorMsg, cdialogIcon_Error);
      
      UCI_QuitEngine(Session->engineId);
   }
   else
   {
      UCI_ProgressDialogClose();
      UCIError("Invalid copy protection message", bufPtr);
   }
} // ProcessMsg_copyprotection


/**************************************************************************************************/
/*                                                                                                */
/*                               PROCESS 'registration' MESSAGES                                  */
/*                                                                                                */
/**************************************************************************************************/

// "registration .."
//
// This is needed for engines that need a username and/or a code to function with all features.
// Analog to the "copyprotection" command the engine can send "registration checking"
// after the uciok command followed by either "registration ok" or "registration error".
// Also after every attempt to register the engine it should answer with "registration checking"
// and then either "registration ok" or "registration error".
// In contrast to the "copyprotection" command, the GUI can use the engine after the engine has
// reported an error, but should inform the user that the engine is not properly registered
// and might not use all its features.
// In addition the GUI should offer to open a dialog to
// enable registration of the engine. To try to register an engine the GUI can send
// the "register" command.
// The GUI has to always answer with the "register" command  if the engine sends "registration error"
// at engine startup (this can also be done with "register later")
// and tell the user somehow that the engine is not registered.
// This way the engine knows that the GUI can deal with the registration procedure and the user
// will be informed that the engine is not properly registered.

static BOOL ProcessMsg_registration (void)
{
   if (PeekToken("checking"))
   {
   }
   else if (PeekToken("ok"))
   {
   }
   else if (PeekToken("error"))
   {
	   // Make sure registration process for same engine isn't looping
   	if (lastRegEngineId == Session->engineId)
   		regEngineCount++;
   	else
	   {  lastRegEngineId = Session->engineId;
   		regEngineCount = 1;
   	}

      CHAR name[uci_UserNameLen + 1], code[uci_UserRegCodeLen + 1];
      if (regEngineCount <= 3 && UCI_RegistrationDialog(theApp->GetFrontWindow(), Info->name, Info->engineAbout, name, code))
      {
         CHAR regMsg[200];
         Format(regMsg, "register name %s code %s", name, code);
         UCI_SendCommand(Session->engineId, regMsg);
      }
      else
      {
         UCI_SendCommand(Session->engineId, "register later");
      }
   }
   else
   {
      return UCIError("Invalid registration message", bufPtr);
   }

   return true;
} // ProcessMsg_registration


/**************************************************************************************************/
/*                                                                                                */
/*                                  PROCESS 'info' MESSAGES                                       */
/*                                                                                                */
/**************************************************************************************************/

// "info .."
//
// The engine wants to send infos to the GUI. This should be done whenever one of the info has changed.
// The engine can send only selected infos and multiple infos can be send with one info command,
// e.g. "info currmove e2e4 currmovenumber 1" or
//      "info depth 12 nodes 123456 nps 100000".
// Also all infos belonging to the pv should be sent together
// e.g. "info depth 2 score cp 214 time 1242 nodes 2124 nps 34928 pv e2e4 e7e5 g1f3"
// I suggest to start sending "currmove", "currmovenumber", "currline" and "refutation" only after one second
// to avoid too much traffic.

static BOOL ProcessMsg_info (void)
{
   if (! Session->thinking) return false;

   ENGINE *E = Session->engineRef;
   if (! E || Engine_Aborted(E)) return false;

   PIECE Board[boardSize];  // For move parsing
   MOVE  m;                 // For move parsing
   LONG  n;                 // Number parsing util

   E->S.multiPV = 1;        // Initially assume single PV

   do
   {
      // "depth <x>"     {ÊSearch depth in plies }
      if (PeekToken("depth"))
      {
         if (! ReadNumberToken(&n))
            return UCIError("Failed parsing 'info depth'", bufPtr);
         else
         {  E->S.mainDepth = n;
            SendMsg_Async(E, msg_NewIteration);
         }
      }
      // "seldepth <x>"  { Selective search depth in plies. Must be present in same string as "depth" }
      else if (PeekToken("seldepth"))
      {
         // Currently ignored
         if (! ReadNumberToken(&n))
            return UCIError("Failed parsing 'info seldepth'", bufPtr);
      }
      // time <x>      { the time searched in ms, this should be sent together with the pv }
      else if (PeekToken("time"))
      {
         // Currently ignore and simply use engine struct's own time (OK because we handle "nps <x>" correctly
         if (! ReadNumberToken(&n))
            return UCIError("Failed parsing 'info time'", bufPtr);
      }
      // nodes <x>     { x nodes searched, the engine should send this info regularly }
      else if (PeekToken("nodes"))
      {
         LONG64 nodes;
         if (! Read64BitNumberToken(&nodes))
            return UCIError("Failed parsing 'info nodes'", bufPtr);
         else
         {  E->S.moveCount = nodes;
         	SendMsg_Async(E, msg_NewNodeCount);
         }
      }
      // pv <move1> ... <movei>   { the best line found }
      else if (PeekToken("pv"))
      {
         CopyTable(E->P.Board, Board);

         MOVE bestMove = E->S.MainLine[0];
         clrMove(E->S.MainLine[0]);

         for (INT d = 0; d < maxSearchDepth - 2 && ! EndOfMessage(); d++)
         {
            if (! ParseMoveToken(Board, &m, true))
               return UCIError("Failed parsing 'pv move'", bufPtr);
               
            if (d == 0 && ! EqualMove(&m, &bestMove))
            {  E->S.mainTime = Timer() - E->S.startTime;
               if (debugOn) DebugWriteNL("New best move");
            }
            
            E->S.MainLine[d] = m;

            if (d == 0 && isNull(E->S.rootNode->m))
            {  E->S.rootNode->m = m;
               E->S.currMove = 0;
               SendMsg_Async(E, msg_NewRootMove);
            }

            clrMove(E->S.MainLine[d+1]);
         }

         SendMsg_Async(E, msg_NewMainLine);
      }
      // multipv <num>  { will be sent right after "info" if enabled }
      else if (PeekToken("multipv"))
      {
	      if (ReadNumberToken(&n) && n > 0)
	         E->S.multiPV = n;
	      else
	         return UCIError("Failed parsing 'info multipv'", bufPtr0);
      }
      // score
      else if (PeekToken("score"))
      {
	      E->S.scoreType = scoreType_True;  // Assume true score by default

         // * cp <x>     {Êthe score from the engine's point of view in centipawns. }
         if (PeekToken("cp"))
         {
	         if (! ReadNumberToken(&n))
	            return UCIError("Failed parsing 'info score cp'", bufPtr);
	         else
	            E->S.bestScore = n;
	      }
         // * mate <y>   { mate in y moves, not plies. If the engine is getting mated use negativ values for y. }
         else if (PeekToken("mate"))
         {
	         if (! ReadNumberToken(&n))
	            return UCIError("Failed parsing 'info score mate'", bufPtr);
	         else
	         {
	            if (n >= maxSearchDepth/2)
	               n = maxSearchDepth/2;
	            else if (n <= -maxSearchDepth/2)
	               n = -maxSearchDepth/2;
	            E->S.bestScore = (n > 0 ? maxVal - (2*n - 1) : -maxVal + 2*(-n));
	         }
         }

         // * lowerbound { the score is just a lower bound. }
         if (PeekToken("lowerbound"))
            E->S.scoreType = scoreType_LowerBound;
         // * upperbound { the score is just an upper bound. }
         else if (PeekToken("upperbound"))
            E->S.scoreType = scoreType_UpperBound;

	      SendMsg_Async(E, msg_NewScore);
      }
      // currmove <move>          { currently searching this move }
      else if (PeekToken("currmove"))
      {
         CopyTable(E->P.Board, Board);

         if (ParseMoveToken(E->P.Board, &m, false))
         {  E->S.rootNode->m = m;
            SendMsg_Async(E, msg_NewRootMove);
         }
         else
         {	return UCIError("Failed parsing 'info currmove'", bufPtr0);
         }
      }
      // currmovenumber <x>       { currently searching move number x, for the first move x should be 1 not 0 }
      else if (PeekToken("currmovenumber"))
      {
         if (ReadNumberToken(&n))
         {  E->S.currMove = n - 1;
            SendMsg_Async(E, msg_NewRootMove);
         }
         else
         {	return UCIError("Failed parsing 'info currmovenumber'", bufPtr0);
         }
      }
      // hashfull <x>   { the hash is x permill full, the engine should send this info regularly }
      else if (PeekToken("hashfull"))
      {
         // Currently ignored
	      if (! ReadNumberToken(&n))
	         return UCIError("Failed parsing 'info hashfull'", bufPtr);
	      E->S.hashFull = n;
      }
      // nps <x>   { x nodes per second searched, the engine should send this info regularly }
      else if (PeekToken("nps"))
      {
	      if (! ReadNumberToken(&n))
	         return UCIError("Failed parsing 'info nps'", bufPtr);
	      E->S.uciNps = n;
      }
      // tbhits <x>       { x positions where found in the endgame table bases }
      else if (PeekToken("tbhits"))
      {
         // Currently ignored
	      if (! ReadNumberToken(&n))
	         return UCIError("Failed parsing 'info tbhits'", bufPtr);
      }
      // cpuload <x>      {Êthe cpu usage of the engine is x permill }
      else if (PeekToken("cpuload"))
      {
         // Currently ignored
	      if (! ReadNumberToken(&n))
	         return UCIError("Failed parsing 'info cpuload'", bufPtr);
      }
      // string <str>     {Êany string str which will be displayed be the engine. Rest of line }
      else if (PeekToken("string"))
      {
         return true;  // Currently ignored
      }
      // refutation <move1> <move2> ... <movei> {Ê<move1> is refuted by the line <move2> ... <movei>. Only if "UCI_ShowRefutations" enabled }
      else if (PeekToken("refutation"))
      {
         return true;  // Currently ignored
      }
      // currline <cpunr> <move1> ... <movei>   {Êonly if the option "UCI_ShowCurrLine" }
      else if (PeekToken("currline"))  
      {
         return true;  // Currently ignored
      }
      else
      	return true;  // Unknown info message -> stop parsing

   } while (! EndOfMessage());

   return true;
} // ProcessMsg_info


/**************************************************************************************************/
/*                                                                                                */
/*                                    PROCESS 'option' MESSAGES                                   */
/*                                                                                                */
/**************************************************************************************************/

static UCI_OPTION *FindOldOption (UCI_OPTION *Option);
static BOOL ParseOption_Check (UCI_OPTION *Option);
static BOOL ParseOption_Spin (UCI_OPTION *Option);
static BOOL ParseOption_Combo (UCI_OPTION *Option);
static BOOL ParseOption_String (UCI_OPTION *Option);
static BOOL ParseOption_Button (UCI_OPTION *Option);
static BOOL ProcessFixedOption (UCI_OPTION *Option);
static BOOL MatchOption (UCI_OPTION *Option, CHAR *name, UCI_OPTION_TYPE type);

// Syntax: "option name <Option name> type <Value>"

// If Sesstion->uciok_Rcvd is true then we are receiving an option CHANGE after
// the normal startup procedure. In this case we simply find the option and update
// the value (both default and actual).

static void ProcessMsg_option (void)
{
   if (Info->optionCount >= uci_MaxOptionCount)
   {  UCIError("No more options can be added", bufPtr);
      return;
   }

   UCI_OPTION NewOption;

   //--- First parse option name (and subsequent "type" token) ---
   // "name <Option Name> type "
   if (! MatchToken("name") || ReadDelimitedToken(NewOption.name, uci_OptionNameLen, "type") < 0)
   {  UCIError("Invalid option name", bufPtr);
      return;
   }

   //--- Then parse the value (depending on the type) ---
        if (PeekToken("check"))  { if (! ParseOption_Check(&NewOption))  return; }
   else if (PeekToken("spin"))   { if (! ParseOption_Spin(&NewOption))   return; }
   else if (PeekToken("combo"))  { if (! ParseOption_Combo(&NewOption))  return; }
   else if (PeekToken("button")) { if (! ParseOption_Button(&NewOption)) return; }
   else if (PeekToken("string")) { if (! ParseOption_String(&NewOption)) return; }
   else 
   {  UCIError("Invalid option type", bufPtr);
      return;
   }
 
   //--- Successfully parsed -> Check if fixed option ---
   if (ProcessFixedOption(&NewOption)) return;

   //--- Successfully parsed non-fixed option -> Add to option list ---
   // If we get here, it's a "normal" generic option which should be added to the list
   if (! Session->uciok_Rcvd)
   {
      Info->Options[Info->optionCount++] = NewOption;
   }
   else
   {
      UCI_OPTION *Option0 = FindOldOption(&NewOption);
      if (Option0) *Option0 = NewOption;
      UCI_ConfigDialogRefresh();
   }

} // ProcessMsg_option


static UCI_OPTION *FindOldOption (UCI_OPTION *Option)  // Returns nil if not found
{
	UCI_INFO *I = (! Session->uciok_Rcvd ? &Info0 : Info);

   // Fixed options
   if (MatchOption(Option, uciOptionName_Ponder, uciOption_Check))
      return &(I->Ponder);

   if (MatchOption(Option, uciOptionName_UCI_LimitStrength, uciOption_Check))
   {  //CHAR sss[1000]; Format(sss, "Old limitstrength : %d", Info0.LimitStrength.u.Check.val); DebugWriteNL(sss);
      return &(I->LimitStrength);
   }

   if (MatchOption(Option, uciOptionName_UCI_Elo, uciOption_Spin))
   {  //CHAR sss[1000]; Format(sss, "Old UCI_ELO : %d", Info0.UCI_Elo.u.Spin.val); DebugWriteNL(sss);
      return &(I->UCI_Elo);
   }

   // Look in old options for the value
   for (INT i = 0; i < I->optionCount; i++)
      if (SameStr(Option->name, I->Options[i].name) && Option->type == I->Options[i].type)
         return &(I->Options[i]);

   return nil;
} // FindOldOption

/*---------------------------------------- Parse Check Option ------------------------------------*/
//   Example: "option name Nullmove type check default true"

static BOOL ParseOption_Check (UCI_OPTION *Option)
{
   Option->type = uciOption_Check;

   // Read default
   if (! MatchToken("default")) return false;
   Option->u.Check.def = EqualStr(LastToken(), "true");

   // Set value
   UCI_OPTION *Option0 = (Session->uciok_Rcvd ? nil : FindOldOption(Option));
   Option->u.Check.val = (Option0 ? Option0->u.Check.val : Option->u.Check.def);

   return true;
} // ParseOption_Check

/*---------------------------------------- Parse Spin Option -------------------------------------*/
// Example: "option name Selectivity type spin default 2 min 0 max 4"
// Note that the min, max and default tokens can come in any order

static BOOL ParseOption_Spin (UCI_OPTION *Option)
{
   Option->type = uciOption_Spin;

   BOOL minDone = false;
   BOOL maxDone = false;
   BOOL defDone = false;

   for (INT i = 0; i < 3; i++)
   {
      if (PeekToken("default") && ReadNumberToken(&Option->u.Spin.def))
         defDone = true;
      else if (PeekToken("min") && ReadNumberToken(&Option->u.Spin.min))
         minDone = true;
      else if (PeekToken("max") && ReadNumberToken(&Option->u.Spin.max))
         maxDone = true;
      else
         return UCIError("Invalid spin option", Option->name);
   }

   if (! (minDone && maxDone && defDone)) return false;

   // Set value
   UCI_OPTION *Option0 = (Session->uciok_Rcvd ? nil : FindOldOption(Option));
   Option->u.Spin.val = (Option0 ? Option0->u.Spin.val : Option->u.Spin.def);

   return true;
} // ParseOption_Spin

/*---------------------------------------- Parse Combo Option ------------------------------------*/
// Example: "option name Style type combo default Normal var Solid var Normal var Risky"
// Note: The default can be located both before or after the var list.

#define kComboDefault 1
#define kComboVar     2

static BOOL ParseOption_Combo (UCI_OPTION *Option)
{
   Option->type = uciOption_Combo;

   CHAR defStr[uci_ComboNameLen + 1];
   INT  tokenId = 0;

   // Read first keyword ("var" or "default")
   if (PeekToken("default"))
      tokenId = kComboDefault;
   else if (PeekToken("var"))
      tokenId = kComboVar;
   else
      return UCIError("Invalid combo option", bufPtr);

   // Scan combo vars (and default)
   INT n = 0;
   while (! EndOfMessage())
      switch (tokenId)
      {
         case kComboDefault : tokenId = ReadDelimitedToken(defStr, uci_ComboNameLen, "default","var"); break;
         case kComboVar     : tokenId = ReadDelimitedToken(Option->u.Combo.List[n++], uci_ComboNameLen, "default","var"); break;
         default            : return UCIError("Invalid combo option", bufPtr);
      }

   Option->u.Combo.count = n;
   Option->u.Combo.def = 0;

   // Set default
   for (n = 0; n < Option->u.Combo.count; n++)
      if (SameStr(defStr, Option->u.Combo.List[n]))
         Option->u.Combo.def = n;

   // Set value
   UCI_OPTION *Option0 = (Session->uciok_Rcvd ? nil : FindOldOption(Option));
   Option->u.Combo.val = (Option0 ? Option0->u.Combo.val : Option->u.Combo.def);

   return true;
} // ParseOption_Combo

/*--------------------------------------- Parse Button Option ------------------------------------*/
// Example: "option name Clear Hash type button"

static BOOL ParseOption_Button (UCI_OPTION *Option)
{
   Option->type = uciOption_Button;

   return true;
} // ParseOption_Button

/*--------------------------------------- Parse String Option ------------------------------------*/
// Example: "option name NalimovPath type string default c:\\"

static BOOL ParseOption_String (UCI_OPTION *Option)
{
   Option->type = uciOption_String;

   // Read default string value
   if (! MatchToken("default") || ! ReadRestToken(Option->u.String.def,uci_StringOptionLen))
      return UCIError("Invalid string option", bufPtr);

   if (EqualStr(Option->u.String.def,"<empty>"))
      CopyStr("", Option->u.String.def);

   // Set value
   UCI_OPTION *Option0 = FindOldOption(Option);
   CopyStr((Option0 ? Option0->u.String.val : Option->u.String.def), Option->u.String.val);

   return true;
} // ParseOption_String

/*-------------------------------------- Process Fixed Options -----------------------------------*/
// Returns true if fixed option and it should be skipped:

static BOOL ProcessFixedOption (UCI_OPTION *Option)
{
   // Limit Hash tables to 64 MB in Lite version and check that we don't exceed physical RAM size
   if (MatchOption(Option, uciOptionName_Hash, uciOption_Spin))
   {
      if (! ProVersion() && Option->u.Spin.def > uci_MaxHashSizeLite)
         Option->u.Spin.val = Option->u.Spin.def = uci_MaxHashSizeLite;
/*
      ULONG machineRAM = Mem_PhysicalRAM()/(1024*1024);
      if (Option->u.Spin.max > machineRAM - 64)
      {  Option->u.Spin.max = machineRAM - 64;
         if (Option->u.Spin.val > Option->u.Spin.max)
            Option->u.Spin.val = Option->u.Spin.max;
      }
*/
      return false;  // Don't skip hash
   }

   if (MatchOption(Option, uciOptionName_NalimovPath, uciOption_String))
   {  Info->supportsNalimovBases = true;
      Info->NalimovPath = *Option;
      return true;
   }

   if (MatchOption(Option, uciOptionName_NalimovCache, uciOption_Spin))
      return false; // Don't skip "NalimovCache" option
   
   if (MatchOption(Option, uciOptionName_Ponder, uciOption_Check))
   {  Info->supportsPonder = true;
      Info->Ponder = *Option;
      return true;
   }

   // Don't ignore "OwnBook" because the user should still be able to disable the engine book
//   if (MatchOption(Option, uciOptionName_OwnBook, uciOption_Check)) return true;  // Ignore

   if (MatchOption(Option, uciOptionName_MultiPV, uciOption_Spin))
   {  if (Option->u.Spin.max > uci_MaxMultiPVcount)
         Option->u.Spin.max = uci_MaxMultiPVcount;
   }

   if (MatchOption(Option, uciOptionName_UCI_ShowCurrLine, uciOption_Check)) return true;  // Ignore

   if (MatchOption(Option, uciOptionName_UCI_ShowRefutations, uciOption_Check)) return true;  // Ignore

   if (MatchOption(Option, uciOptionName_UCI_LimitStrength, uciOption_Check))
   {  Info->supportsLimitStrength = true;
      Info->LimitStrength = *Option;
      return true;
   }

   if (MatchOption(Option, uciOptionName_UCI_Elo, uciOption_Spin))
   {  Info->UCI_Elo = *Option;
      Info->autoReduce = false;
      return true;
   }

   if (MatchOption(Option, uciOptionName_UCI_AnalyseMode, uciOption_Check)) return true;  // Ignore

   if (MatchOption(Option, uciOptionName_UCI_Opponent, uciOption_String)) return true;  // Ignore

   if (MatchOption(Option, uciOptionName_UCI_EngineAbout, uciOption_String))
   {
      if (! EqualStr(Option->u.String.def, Info->engineAbout))
      {
         CHAR title[7 + uci_NameLen];
         Format(title, "About %s", Info->name);
         BOOL didCheckAppleEvents = theApp->checkAppleEvents;
         theApp->checkAppleEvents = false;
         NoteDialog(theApp->GetFrontWindow(), title, Option->u.String.def);
         UCI_ProgressDialogResetTimeOut(15);
         theApp->checkAppleEvents = didCheckAppleEvents;
      }

      CopyStr(Option->u.String.def, Info->engineAbout);
      return true;
   }

   // Skip all other/unknown fixed options (starting with "UCI_")
   if (EqualFrontStr(Option->name, "UCI_"))
      return true;

   return false; // If we get here it's not a fixed option -> don't skip
} // ProcessFixedOption


static BOOL MatchOption (UCI_OPTION *Option, CHAR *name, UCI_OPTION_TYPE type)
{
   return (SameStr(Option->name, name) && Option->type == type);
} // MatchOption


/**************************************************************************************************/
/*                                                                                                */
/*                                              MISC                                              */
/*                                                                                                */
/**************************************************************************************************/

static BOOL ParseMoveToken (PIECE Board[], MOVE *m, BOOL perform)
{
// UCI Move format:
// The move format is in long algebraic notation.
// A nullmove from the Engine to the GUI should be send as 0000.
// Examples:  e2e4, e7e5, e1g1 (white short castling), e7e8q (for promotion)

   m->dir   = 0;
   m->dply  = 0;
   m->flags = 0;
   m->misc  = 0;

   //--- Null moves ("0000") ---
   if (bufPtr[0] == '0')
   {
      if (bufPtr[1] == '0' && bufPtr[2] == '0' && bufPtr[3] == '0')
      {
         m->piece = empty;
         m->from  = nullSq;
         m->to    = nullSq;
         m->cap   = empty;
         m->type  = mtype_Null;
         bufPtr += 4;
      }
      else
         return UCIError("Invalid move token", bufPtr);
   }
   //--- Normal moves ---
   else
   {
      // Read the from and to squares
      if (bufPtr[0] < 'a' || bufPtr[0] > 'h' || bufPtr[1] < '1' || bufPtr[1] > '8') return UCIError("Invalid move from square", bufPtr);
      if (bufPtr[2] < 'a' || bufPtr[2] > 'h' || bufPtr[3] < '1' || bufPtr[3] > '8') return UCIError("Invalid move from square", bufPtr);
      m->from = square(bufPtr[0] - 'a', bufPtr[1] - '1');
      m->to   = square(bufPtr[2] - 'a', bufPtr[3] - '1');
      bufPtr += 4;
      
      m->piece = Board[m->from];
      m->cap   = Board[m->to];

      // Check if promotion
      if (*bufPtr == ' ' || ! *bufPtr)
      {
         m->type = mtype_Normal;

         if (m->piece == wKing)
         {
            if (m->from == e1)
               if (m->to == g1) m->type = mtype_O_O;
               else if (m->to == c1) m->type = mtype_O_O_O;
         }
         else if (m->piece == bKing)
         {
            if (m->from == e8)
               if (m->to == g8) m->type = mtype_O_O;
               else if (m->to == c8) m->type = mtype_O_O_O;
         }
         else if (pieceType(m->piece) == pawn && ! m->cap && file(m->from) != file(m->to))
         {
            m->type = mtype_EP;
         }
      }
      else
      {
         m->type = pieceColour(m->piece);

         switch (*bufPtr)
         {
            case 'q' : case 'Q' : m->type |= queen; break;
            case 'r' : case 'R' : m->type |= rook;  break;
            case 'b' : case 'B' : m->type |= bishop; break;
            case 'n' : case 'N' : m->type |= knight; break;
            default  : return UCIError("Invalid move suffix", bufPtr);
         }
         bufPtr++;
      }
   }

   // Finally skip trailing blanks
   while (*bufPtr == ' ') bufPtr++;

   // Optionally perform move (in case we're parsing variations and are expecting more moves
   // to follow).

   if (perform && ! isNull(*m))
      Move_Perform(Board, m);

   return true;
} // ParseMoveToken


static BOOL UCIError (CHAR *errorMsg, CHAR *info)
{
   UCIErr = true;
   
   DebugWriteNL("UCI ERROR");
   DebugWriteNL(errorMsg);
   DebugWriteNL(info);

   return false;
} // UCIError


/**************************************************************************************************/
/*                                                                                                */
/*                                    LOW LEVEL PARSING ROUTINES                                  */
/*                                                                                                */
/**************************************************************************************************/

// Read and return next token (terminated by space). Returns error if maxLen exceeded.

static BOOL ReadToken (CHAR *token, INT maxLen)
{
   if (UCIErr) return false;

   while (*bufPtr && *bufPtr != ' ' && maxLen > 0)
   {  *(token++) = *(bufPtr++);
      maxLen--;
   }
   *token = 0;

   if (*bufPtr && *bufPtr != ' ')
   {  UCIErr = true;
      return false;
   }

   // Skip blanks
   while (*bufPtr == ' ')
      bufPtr++;

   return true;
} // ReadToken


// Read and return next token terminated by delimiter token (or empty string). The delimiter
// token is also read. Returns error if maxLen exceeded.

static INT ReadDelimitedToken (CHAR *token, INT maxLen, CHAR *endToken1, CHAR *endToken2)
{
   INT tokenId = -1;

   if (UCIErr) return tokenId;

   while (*bufPtr && maxLen > 0 && tokenId < 0)
   {
      // If blank check if next token is the delimiter
      if (*bufPtr == ' ' && EqualFrontStr(bufPtr + 1, endToken1))
      {  tokenId = 1;
         bufPtr += 1 + StrLen(endToken1); // Move past delimiter token
      }
      else if (*bufPtr == ' ' && endToken2 && EqualFrontStr(bufPtr + 1, endToken2))
      {  tokenId = 2;
         bufPtr += 1 + StrLen(endToken2); // Move past delimiter token
      }
      else
      {  *(token++) = *(bufPtr++);
         maxLen--;
      }
   }
   *token = 0;

   // Skip blanks
   while (*bufPtr == ' ')
      bufPtr++;

   if (! *bufPtr) tokenId = 0;

   if (maxLen == 0 && tokenId < 0)
      UCIErr = true;

   return tokenId;
} // ReadDelimitedToken


static BOOL ReadNumberToken (LONG *number)
{
   CHAR numStr[11];
   if (! ReadToken(numStr,10)) return false;
   UCIErr = ! StrToNum(numStr, number);
   return ! UCIErr;
} // ReadNumberToken


static BOOL Read64BitNumberToken (LONG64 *number)
{
   LONG64 n = 0;

   while (! UCIErr)
   {
      CHAR c = *bufPtr;

      if (c >= '0' && c <= '9')
      {  n = 10*n + (c - '0');
         bufPtr++;
      }
      else if (c == ' ')
      {  do bufPtr++; while (*bufPtr == ' ');
         *number = n;
         return true;
      }
      else if (! c)
      {  *number = n;
         return true;
      }
      else
      {  UCIErr = true;
      }
   }

   return ! UCIErr;
} // Read64BitNumberToken


static BOOL MatchToken (CHAR *token)   // Require next token and raise error if it doesn't match
{                                      // the required token.
   if (UCIErr) return false;

   while (*token && *bufPtr && *(token++) == *(bufPtr++));

   if (*token || (*bufPtr && *bufPtr != ' '))
   {  UCIErr = true;
      return false;
   }
   
   // Skip blanks
   while (*bufPtr == ' ') bufPtr++;

   return true;
} // MatchToken


static BOOL PeekToken (CHAR *token)    // Read/peek next token and check if it matches. Rewind and
{                                      // return false if not.
   if (UCIErr) return false;

   CHAR *bufPtr0 = bufPtr;
   while (*token && *bufPtr && *(token++) == *(bufPtr++));

   if (*token || (*bufPtr && *bufPtr != ' '))
   {  bufPtr = bufPtr0;  // Rewind
      return false;
   }
   
   // Skip blanks
   while (*bufPtr == ' ') bufPtr++;
   
   return true;
} // PeekToken


static BOOL ReadRestToken (CHAR *token, INT maxLen)   // Read until end of buffer
{
   while (*bufPtr && maxLen > 0)
   {  *(token++) = *(bufPtr++);
      maxLen--;
   }
   *token = 0;
   
   UCIErr = (*bufPtr != 0);
   return ! UCIErr;
} // ReadRestToken


static CHAR *LastToken (void)         // Return remaining string as one token.
{
   return bufPtr;
} // LastToken


static BOOL EndOfMessage (void)
{
   while (*bufPtr == ' ') bufPtr++;
   return (*bufPtr == 0);
} // EndOfMessage
