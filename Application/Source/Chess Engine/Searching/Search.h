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

#include "General.h"
#include "Board.h"
#include "Attack.h"
#include "Move.h"
#include "HashCode.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

//#define __debug_Search

#define maxSearchDepth       50
#define maxVal               20000
#define mateWinVal           (maxVal - 1000)     // Non mate values lie in the interval
#define mateLoseVal          (-mateWinVal)       // [mateLoseVal+1 .. mateWinVal-1]
#define drawVal              0
#define resignVal            -600
#define maxLegalMoves        300
#define sacrificeBufferSize  700

enum DRAW_TYPE
{
   drawType_None = 0,
   drawType_Rep1,
   drawType_Rep2,
   drawType_50,
   drawType_InsuffMtrl,
   drawType_staleMate
};

#define NN               (N+1)
#define PN               (N-1)

/*------------------------------------- PPC Assembler Macros -------------------------------------*/

#define node(field)    NODE.field(rNode)
#define move(field)    NODE.m.field(rNode)

#define kmove1(field)  NODE.killer1.field(rNode)
#define kmove2(field)  NODE.killer2.field(rNode)
#define rmove(field)   NODE.rfm.field(rNode)
#define pmove(field)   (NODE.m.field - sizeof(NODE))(rNode)
#define pnode(field)   (NODE.field - sizeof(NODE))(rNode)
#define nnode(field)   (NODE.field + sizeof(NODE))(rNode)


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

typedef struct
{
   LONG lr;               // Link register (return address of calling routine)
   LONG sp;               // Stack pointer (SP = r1)  
   LONG gpr[7];           // rLocal1..rLocal7 (r25..r31)
} CUTENV;

/*-------------------------------------- Draw Information ----------------------------------------*/

typedef struct            // 8 bytes
{
   HKEY   hashKey;        // 32 bit hash key of position resulting from previous move.
   INT    irr;            // Index in "DrawTab"/"Game" of latest irreversible move.
   INT    repCount;       // Repetition count: Number of times this position has
                          // occured previously.
} DRAWDATA;

/*------------------------------------- Search NODE Data Structure -------------------------------*/
// The central data structure which controls searching is the NODE structure below. It contains all
// information pertaining to one node in the search tree at a given depth [0..maxDepth - 1]. During
// the search register "rNode" contains a pointer to the current node. Access to the fields of this
// node (as wells as the previous and next node) is controlled through offsets of "rNode" through a
// number of macro's (above): node(<field>), pnode(<field>) and nnode(<field>). Additionally, macros
// for accessing the fields of the moves "m" and "killer" of the current node as well as "m" of the
// previous node are provided: move(<field>), killer(<field>) and pmove(<field>).

typedef struct
{
   /* - - - - - - - - - - - - - - - - - - - PARAMETERS  - - - - - - - - - - - - - - - - - - - */

   INT      alpha0,                      // Initial alpha value.
            alpha, beta,                 // Effective alpha/beta values.
            ply,                         // Ply counter (number of plies to quiescence).
            alphaPly, betaPly;           // Selection ply counters for player/opponent.

   /* - - - - - - - - - - - - - - - - - - RETURN VALUES - - - - - - - - - - - - - - - - - - - */

   INT      score,                       // Best score found so far during search ofcurrent node.
            val;                         // Value returned from search of current move.

   /* - - - - - - - - - - - - - - - - - - - EVALUATION  - - - - - - - - - - - - - - - - - - - */

   INT      totalEval;                   // Total static positional evaluation (seen from PLAYER).

   INT      pvSumEval,                   // Piece value sum of current position (seen from WHITE).
            mobEval,                     // Mobility evaluation of current position (seen from WHITE).
            pawnStructEval,              // Pawn structure evaluation of current position (seen from WHITE).
            endGameEval;                 // End game modification value (special mtrl config) (seen from WHITE).

   INT      threatEval;                  // Static threat evaluation (subtracted from totalEval).

// INT      moveThreat;                  // Optimistic threat evaluation for current move (seen from WHITE)
   INT      selMargin,                   // Selection margin = epsilon(alphaPly).
            capSelVal;                   // Selection value to subtract from capture moves to
                                         // eliminate unwanted effects of "mtrlEndVal".

   INT      dPV;                         // Piece value change caused by current move (seen from WHITE).
/*
   INT      moveMob,                     // Mobility value change caused by current move (seen from WHITE).
            oldDestMob,                  // Mobility of piece at destination square BEFORE performing move.
            destMob;                     // Mobility of piece at destination square AFTER performing move.
                                         // Used for restoring MobVal[m->to] when retracting move.
   SQUARE   mobSq;                       // Tempory utility square used during mobility evaluation.

   DMOB     DMobList[32];                // Mobility change list: List of changes to MobVal[] caused
   DMOB     *dmobTop;                    // by current move. Computed by "EvalMoveMob()" and
                                         // subsequently used by "PerformMove" and "RetractMove".
*/
   /* - - - - - - - - - - - - - - - - - - - - MOVES - - - - - - - - - - - - - - - - - - - - - */

   MOVE     m;                           // Current move (which has just been generated).
   INDEX    capInx,                      // Index of captured piece (if any).
            promInx;                     // Index of swapped pawn in case of promotions.
   INT      gen, bestGen;                // Current/best move generator.

   MOVE     rfm;                         // Refutation move (from transposition table or
                                         // principal variation) to be searched as the very
                                         // first thing.

   /* - - - - - - - - - - - - - - - - - - - KILLERS - - - - - - - - - - - - - - - - - - - - - */

   MOVE     killer1, killer2;            // Killer moves at this ply.
   LONG     killer1Count,                // Popularity counts for each killer
            killer2Count;
   BOOL     killer1Active,               // Active flags for each killer.
            killer2Active;               // MUST BE ALLOCATED AFTER killer1Active

   /* - - - - - - - - - - - - - - - - - - - - FLAGS - - - - - - - - - - - - - - - - - - - - - */

   BOOL     check,                       // Indicates if player is in check.
            quies,                       // Quiescence node? MUST BE ALLOCATED AFTER "check".
            storeSacri,                  // Should sacrifices be stored in "SBuf"?
            canMove,                     // Are there any (pseudo-) legal moves (so far)?
            firstMove,                   // Is this the first move at current node?
            pvNode;                      // Is this a node containing a move from principal
                                         // variation?

   /* - - - - - - - - - - - - - - - - - - - LOCATIONS - - - - - - - - - - - - - - - - - - - - */

   INT      eply;                        // Ply decrement for escape moves.
   SQUARE   escapeSq,                    // Origin of threatened piece.
            recapSq,                     // "Recapture"-square.
            checkSq,                     // Origin of checking piece (nullSq if double check).
            ALoc[16],                    // Locations of attacked pieces (excl. "escapeSq").
            SLoc[16];                    // Locations of safe (unattacked) pieces.
   BYTE     PassSqW[10],                 // Lists of passed pawns in descending order of advan-
            PassSqB[10];                 // cement (only if "rule of square" is applicable). 0 terminated.

   /* - - - - - - - - - - - - - - - - - - - - MISC - - - - - - - - - - - - - - - - - - - - - -*/

   MOVE     *bufStart;                   // Old top of sacrifice buffer.
   CUTENV   cutEnv;                      // Long-jump environment. Is used to restore processor
                                         // state (i.e. registers r16..r31, SP and instruction
                                         // address) in case of cutoff.

   /* - - - - - - - - - - - - - - - - - TRANSPOSITION TABLES  - - - - - - - - - - - - - - - - */

   HKEY     hashKey;                     // Hashkey for this position (= drawData->hashKey)
   TRANS    *trans1, *trans2,            // Transposition table entries of current position
                                         // (set by "ProbeTransTab" at "start" of node).
            *tmove;

   /* - - - - - - - - - - - - - - - DEPTH DEPENDANT CONSTANTS - - - - - - - - - - - - - - - - */

   INT      depth,                       // Depth of current node.
            gameDepth,                   // "depth" + currentGameMove.
            maxPly,                      // Maximum number of plies to restricted quiescence.
            loseVal,                     // Worst case evaluation (mate) at this depth.
            hungVal1,                    // Punishment of player for having 1 or more hung pieces.
            hungVal2;
   BOOL     program,                     // Is program to move at this node?
            bottomNode;                  // Is this a bottom node?
   BOOL     isMateDepth;                 // If mate finder the depth at which player must play
                                         // the mating move.
   INT      drawType;
   DRAWDATA *drawData;                   // = &DrawTab[gameDepth].

   /* - - - - - - - - - - - - - - - COLOUR DEPENDANT CONSTANTS  - - - - - - - - - - - - - - - */

   COLOUR   player,                      // Also in register "rPlayer".
            opponent;

   SQUARE   *PieceLoc, *PieceLoc_,       // Also in registers "rPieceLoc" and "rPieceLoc_".
            pawnDir;                     // Also in register "rPawnDir".
   INT      lastPiece, lastPiece_;
   ATTACK   *Attack, *Attack_;           // Also in registers "rAttack" and "rAttack_".
   TRANS    *TransTab1, *TransTab2;      // Pointers to transposition tables of side to move.

   /* - - - - - - - - - - - - - - - - - - BEST LINE - - - - - - - - - - - - - - - - - - - - - */

   MOVE     BestLine[maxSearchDepth+3];  // Best line found so far during search of current node

} NODE;

/*--------------------------------------- Root Move list -----------------------------------------*/
// All strictly valid moves at the root node are stored in a table of ROOTTAB entries:

typedef struct
{
   INT   i;                              // Index in E->S.RootMoves[] of move. Used for sorting.
   INT   val;                            // Search score for this move.
} ROOTTAB;

/*------------------------------------- SEARCH_STATE Data Structure ------------------------------*/

typedef struct search_state
{
   //--- Strictly legal root moves ---
   INT     numRootMoves;                 // Number of strictly legal moves in the root position.
   MOVE    RootMoves[maxLegalMoves];     // The actual moves.
   ROOTTAB RootTab[maxLegalMoves];       // Root move search info/ordering.
   BOOL    Ignore[maxLegalMoves];        // Root moves that should be ignored (next best & lib moves)
   BOOL    BadLibMove[maxLegalMoves];    // Moves classified as really bad in pos lib
                                         // (i.e. winning advantage for opponent)

   //--- Search results/statistics ---
   INT     mainDepth;                    // Nominal search depth in plies (= rootNode->ply).
   INT     currMove;                     // Index of current root move being searched.
   INT     mainScore;                    // Score of main line. NOTE: Is updated by PV Search.
   INT     bestScore;                    // Last score reported back to host app.
   INT     scoreType;                    // Score type for bestScore (true, lower, upper).

   INT     prevScore;                    // Score of main line in previous iteration.
   INT     alphaWin;
   INT     betaWin;
   INT     checkDepth;                   // Maximum depth for safe check generation

   INT     multiPV;                      // PV number (always 1 for single PV; 1..k for best k vars)
   MOVE    *MainLine;                    // The main line.
   MOVE    bestReply;                    // Used for hints and pondering.
   BOOL    isPonderMove;                 // Is the best reply a ponder move?
   BOOL    iMain;                        // Index in RootMoves of final MainLine. Used to update
                                         // Ignore[] table.
   LONG64  nodeCount;                    // Nodes searched so far (calls to SearchNode()).
   LONG64  moveCount;                    // Moves searched so far (calls to SearchMove()).
   INT     hashFull;                     // Hash full (in permile)

   BOOL    libMovesOnly;                 // Only library moves should be searched.

   //--- Timers ---
   ULONG   startTime;                    // Timer() at start of search.
   ULONG   searchTime;                   // Number elapsed ticks (1/60th sec) at end of search.
   ULONG   mainTime;                     // Elapsed ticks until engine settled/decided on it's final move.
   INT     periodicCounter;              // Limits number of calls to "Timer()" (which is VERY expensive under
                                         // OS X!! Probably because it's an out-of-process system call).
   ULONG   periodicTime;                 // Next "tick" where periodic stuff will be performed
                                         // (e.g. Engine_CallBack and Task_Switch()).
   ULONG   uciNps;                       // NPS if UCI engine (Sigma does its own calculation)

   //--- Mate Finder ---
   INT     mateDepth;
   BOOL    mateFound;                    // Has a mate been found? MateSolver only.
   ULONG   mateTime;                     // Number elapsed ticks (1/60th sec) at end of (each) search.
   BOOL    mateContinue;                 // Continue looking for alternative mates (cooks)?

   //--- Endgame Databases ---
   BOOL    edbPresent;                   // Is endgame database present?
   BOOL    edbMovesOnly;                 // Endgame database moves only?
   CHAR    edbName[5];                   // "Passed" to host app via callback
   LONG    edbPos;                       // "Passed" to host app via callback
   INT     edbResult;                    // "Returned" from host app

   //--- ELO Strength Control ---
   INT     eloAdjust;                    // ELO adjustment (depending on time controls)
   INT     eloTarget;                    // Target ELO rating (= P.engineELO - eloAdjust)
   ULONG   npsTarget;                    // Target NPS. This controls the actual search reduction.   

   //--- Sacrifice buffer ---
   MOVE    SBuf[sacrificeBufferSize];    // The sacrifice buffer.
   MOVE    *bufTop;                      // Pointer to top (next available entry).

   //--- Search nodes ---
   NODE    *rootNode,                    // Pointer to root node in this search.
           *currNode,                    // Pointer to current node (also in register rNode).
           *whiteNode,                   // Const pointer to white root node.
           *blackNode;                   // Const pointer to black root node.
   NODE    _Nodes[maxSearchDepth + 3];   // The actual search "tree". Should normally not be
                                         // accessed directly. Rather access should done via
                                         // "rootNode".
} SEARCH_STATE;
