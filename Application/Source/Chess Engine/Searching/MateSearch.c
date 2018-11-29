/**************************************************************************************************/
/*                                                                                                */
/* Module  : MATESEARCH.C                                                                         */
/* Purpose : This module contains the specialized mate search routine, which is called from the   */
/*           "Search.c"  module.                                                                  */
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

#include "Engine.h"

#include "MateSearch.f"
#include "Search.f"


void MSearchRootNode (ENGINE *E)
{
//   SearchRootNode(E); //###
  //...
} /* MSearchRootNode */


#ifdef kartoffel

#include "MateSearchE.h"
#include "MoveGen.h"
#include "SearchE.h"
#include "SearchMisc.h"
#include "PerformMove.h"
#include "EngineShell.h"

#define with_trace

#ifdef with_trace
	#include "InfoGraphics.h"
#endif

INT   mateDepth;									// Number of plies to mate.
BOOL	mateFound;

void (*MateDlg)(void) = nil;					// Pointer to mate dialog. Only used by · Chess.


/**********************************************************************************************/
/*																															 */
/*													MATE ROOT SEARCH													 */
/*																															 */
/**********************************************************************************************/

void MSearchRootNode ()										// Performs one iteration by searching all
{																	// the root moves.
	register NODE 		*N;
	register ROOTTAB	*R;

	setupA6(); getA6(N);
		N->score = N->looseVal;
		N->alpha = maxVal - mateDepth - 1;
		N->beta	= maxVal - N->ply + 1;
		NN->beta = -N->alpha;

		for (current = 0, R = RootTab; current < numRootMoves && ! stopSearch; current++, R++)
		{
			if (R->m.dply < 0)											// Skip moves that we already
			{	R->val = 1 - maxVal;										// know are mates.
				continue;
			}
			nodeCount++;
			N->m = R->m;													// Retrieve next root move incl.
			N->dPV = R->dPV;												// move evaluation and move
			N->gen = R->gen;												// generator.

			PrintCurrentE(current, &N->m);
#ifdef with_trace
			traceDepth = NN->depth;
			TraceMove(N);
#endif

			PerformMove();													// Perform this move.

				NN->alpha0 = -N->beta;									// Compute parameters at next node.
				NN->ply 	 = N->ply - Min(N->m.dply, 1);

				MSearchNode();												// Search the move...
				R->val = N->val;											// ...and get return value.

			RetractMove();													// Finally, retract the move.

			if (R->val > N->alpha && ! stopSearch)					// Update alpha values and main
			{																	// line if necessary...
				mainVal = N->score = R->val;
				NN->beta = -N->alpha;				/****/
				UpdBestLine();
				PrintMainLineE(MainLine);
				PrintScoreE(N->score);
				mateFound = true;
				R->m.dply = -1;											// Mark move to avoid research.
				R->val = 1 - maxVal;
				if (MateDlg != nil) (*MateDlg)();
			}
		}

#ifdef with_trace
		traceDepth = N->depth;
		UntraceMove(N);
#endif
	restoreA6();
}	/* MSearchRootNode */


/**********************************************************************************************/
/*																															 */
/*													MATE TREE SEARCH													 */
/*																															 */
/**********************************************************************************************/

void MSearchNode ()
{
	register NODE *N;
	incrementA6(); getA6(N);

	PeriodicTaskE();													// Do periodic task.

	/* - - - - - - - - - - - - - - - - - Compute Parameters - - - - - - - - - - - - - - - - - -*/

	N->alpha = N->alpha0;
	if (N->ply < 0)													// Adjust "ply" counter to the
		N->ply = 0;														// interval [0..maxPly].
	else if (N->ply > N->maxPly)
		N->ply = N->maxPly;

	/* - - - - - - - - - - - - - - - - - - Initialize Node - - - - - - - - - - - - - - - - - - */

	clrMove(N->BestLine[0]);										// No move has been found yet.
	N->kingSq 	= N->PieceLoc[0];									// Compute locations of both kings.
	N->kingSq_	= N->PieceLoc_[0];

	UpdateDrawState();												// Return in case of a true draw...
	if (N->drawType > noDraw)
	{	N->score = 0; goto exit; }
	
	if (ProbeTransTab()) goto exit;

	N->check = (N->Attack_[N->kingSq] > 0);					// Is the player in check?
	N->quies = (N->ply <= 0 && ! N->check);					// Is this a quiescence node?
	N->score = MEvaluate();

	if (! N->quies)													// Compute worst case evaluation.
		N->score = N->looseVal;
	else if (! N->program)
		goto exit;

	NN->beta		 = -N->alpha;
	N->bufStart	 = bufTop;											// Remember old state of "SBuf".
	N->gen	 	 = N->bestGen = noGen;							// Reset current/best move phase.
	N->firstMove = true;												// We are about to search first move
	saveCutEnv(@cut);													// Save cutoff-environment.
	PrepareKillers();													// Perpare killer table.

	/* - - - - - - - - - - - - - - - - - - - - Search Node - - - - - - - - - - - - - - - - - - */

	if (! isNull(N->rfm)) MSearchMove(); 	// +HASH

	N->storeSacri = true;											// Include sacrifices.
	N->m.dply = 0; 													// Set ply parameters for forced
																			// moves (dply = 0).
	if (N->check)														// CHECK EVASION.
	{	SearchCheckEvasion();
		if (N->score == N->looseVal &&							// Force cutoff at previous node in
			 PN->beta > -N->looseVal)								// case of mate.
			PN->beta = -N->looseVal;
	}
	else
	{
		SearchEnPriseCaptures();
		SearchPromotions();
		SearchRecaptures();
		N->m.dply = 1;													// Set ply parameters for non-forced
		SearchSafeCaptures();										// moves (dply = 1).

		if (N->depth == mateDepth - 1 ||							// CHECK SEARCH.
			 N->program && N->ply == 1)
		{
			N->killer1Active = N->killer2Active = false;
			N->gen = checkGen;
			SearchAllChecks();
			SearchPawnsTo7th();
			SearchCastling();
			N->gen = sacriGen;
			SearchSacrifices();
			if (N->score == N->looseVal)							// If we are stalemate return 0...
				N->score = MEvaluate();
		}
		else if (! N->quies)											// FULL WIDTH SEARCH.
		{
			SearchCastling();
			SearchKillers();
			N->gen = nonCapGen;
			MSearchNonCaptures();
			SearchSacrifices();
			if (N->score == N->looseVal)							// If we are stalemate return 0...
				N->score = 0;
		}
		else if (N->maxPly > 0)										// QUIESCENCE SEARCH.
		{
			N->gen = checkGen;
			SearchSafeChecks();
			SearchPawnsTo7th();
			N->gen = sacriGen;
			SearchSacrifices();
		}
	}

	/* - - - - - - - - - - - - - - - - - - - - Exit Node - - - - - - - - - - - - - - - - - - - */
cut:
	UpdateKillers();													// Update killer table.
	StoreTransTab();													// Update transposition table.
	bufTop = N->bufStart;											// Restore old state of "SBuf".
exit:
	PN->val = -N->score;												// "Return" score.
#ifdef with_trace
	traceDepth = N->depth;
	if (showTree) UntraceMove(N);
#endif

	decrementA6();
}	/* MSearchNode */


/*--------------------------------------- Search One Move ------------------------------------*/

void MSearchMove ()								// Searches the move "N->m". Is called by the move
{														// generators each time a move has been generated.
	declareNODE(N);

	nodeCount++;

	if (N->firstMove && ! isNull(N->rfm))
	{	N->firstMove = false;
		if (! GetTransMove()) return;
		else N->m = N->rfm;
	}
	else if (KillerRefCollision()) return;						 // Handle killer collision.

#ifdef with_trace
	traceDepth = NN->depth;
	if (showTree) TraceMove(N);
#endif

	NN->ply = N->ply - N->m.dply;									// Compute ply parameter.
	if (N->program && NN->ply <= 0 && ! GivesCheck())		// Skip search if loosing side can
		return;															// stand pat at next ply.

	PerformMove();														// Perform move...

		if (N->Attack_[N->PieceLoc[0]])							// Skip search if it's illegal or...
		{	RetractMove();
			return;
		}
		else if (N->depth == mateDepth)							// we could not mate the loosing side
			N->val = 0;													// within mate depth.
		else
			CompPVchange(),											// Compute move evaluation.
			NN->alpha0 = -N->beta,
			MSearchNode();												// Search the move...

	RetractMove();														// and retract it.

	if (N->val > N->score)											// Update alpha/beta values and the
	{																		// best line.
		N->score   = N->val;
		N->bestGen = N->gen;
		UpdBestLine();
		if (N->score > N->alpha)
			if (N->score >= N->beta)
			{	cutoff(); }
			else
				N->alpha = N->score,
				NN->beta = -N->alpha;
	}
}	/* MSearchMove */


/**********************************************************************************************/
/*																															 */
/*												 MATE SEARCH EVALUATION												 */
/*																															 */
/**********************************************************************************************/

INT MEvaluate ()								// Compute simple piece value based static evaluation.
{
	asm 68000
	{
				MOVE.W	pnode(sumPV), D0				;	N->sumPV = PN->sumPV + PN->dPV;
				ADD.W		pnode(dPV), D0
				MOVE.W	D0, node(sumPV)
				TST.W		node(player)					;	return (player == white ? N->sumPV :
				BEQ		@return							;									 -N->sumPV);
				NEG.W		D0
	@return
	}
}	/* MEvaluate */


/**********************************************************************************************/
/*																															 */
/*												 GENERATING ALL CHECKS												 */
/*																															 */
/**********************************************************************************************/

void SearchAllChecks ()						// Searches safe non-capturing checks (both direct and
{													// indirect check). Pawns to 7th or 8th rank are not
  	asm 68000	/* 48 bytes */				// searched. Registers A4, D3 and A3 are initialized
	{												// for use by the specialized routines. 
				MOVEM.L	D3/A3/A4, -(SP)
				MOVE.W	node(kingSq_), D3				;	ksq = kingLoc(opponent);	(D3)
				MOVE.W	D3, D0
				ADD.W		D0, D0							;	Bksq = &Board[ksq];			(A4)
				LEA		Board, A4
				ADDA.W	D0, A4
				ADD.W		D0, D0							;	Aksq = &AttackP[ksq];		(A3)
				MOVEA.L	node(Attack), A3
				ADDA.W	D0, A3
				CLR.L		move(cap)						;	m.cap = empty; m.type = normal;
				JSR		SearchAllCheckQRB				;	SearchAllCheckQRB();
				JSR		SearchAllCheckN				;	SearchAllCheckN();
				JSR		SearchAllCheckP				;	SearchAllCheckP();
				MOVEM.L	(SP)+, D3/A3/A4
	}
}	/* SearchSafeChecks */


/*---------------------------- Search All Queen/Rook/Bishop Checks ---------------------------*/

void SearchAllCheckQRB ()					// Searches direct check by queen, rook and bishop as
{													// well as indirect check. Only safe direct checks are
	register SQUARE *Bto, Bksq;			// searched. On entry: D3.W = ksq, A4.L = &Board[ksq]
	register ATTACK *Ato, Aksq;			// and A3.L = &AttackP[ksq].
	register SQUARE dir2, dir4;

	#define ScanDir(dir, dirMask, rayBit, for, rof, nxt)	}\
	asm {		MOVEA.L	Bksq, Bto						/*	dir = QueenDir[i];							*/	}\
	asm {		MOVEA.L	Aksq, Ato						/* to = ksq;										*/	}\
	asm {		MOVEQ		#(4*dir), dir4					}\
	asm {		MOVEQ		#(2*dir), dir2					}\
	asm {		BRA		rof								/* while (isEmpty(to -= dir))					*/	}\
	asm {for	SUBA.W	dir4, Ato						/*	{													*/	}\
	asm {		MOVE.W	(Ato), D2						/*		if (! (a = AttackP[to]))				*/	}\
	asm {		BEQ		rof								/*			continue;								*/	}\
	asm {		ANDI.W	#(dirMask), D2					/*		if (! (bits = a & dirMask))			*/	}\
	asm {		BEQ		rof								/*			continue;								*/	}\
	asm {		JSR		SearchAllCheckQRB1			/*		SearchAllCheckQRB1(to, a, bits);		*/	}\
	asm {rof	SUBA.W	dir2, Bto						}\
	asm {		MOVE.W	(Bto), D1						/*	}													*/	}\
	asm {		BEQ		for								}\
	asm {		BMI		nxt								/*	if (onBoard(to) &&							*/	}\
	asm {		MOVEQ		#0x10, D0						}\
	asm {		AND.W		D1, D0							/*		 friendly(Board[to]) &&					*/	}\
	asm {		CMP.W		node(player), D0				}\
	asm {		BNE		nxt								}\
	asm {		MOVE.W	(-4*dir)(Ato), D0				/*		 AttackP[to] & RayBit[i])				*/	}\
	asm {		ANDI.W	#(rayBit), D0					}\
	asm {		BEQ		nxt								}\
	asm {		MOVEQ		#(dir), D2						/*		SearchIndCheck(to, Board[to], dir);	*/	}\
	asm {		JSR		SearchIndCheck					}\
	asm {nxt

	asm 68000	/* 496 bytes */
	{
				MOVE.L	A4, Bksq
				MOVE.L	A3, Aksq
				ADDQ.L	#2, Aksq

				ScanDir(-0x0F, qbMask, 0x0101, @F0, @R0, @N0)
				ScanDir(-0x11, qbMask, 0x0202, @F1, @R1, @N1)
				ScanDir(+0x11, qbMask, 0x0404, @F2, @R2, @N2)
				ScanDir(+0x0F, qbMask, 0x0808, @F3, @R3, @N3)
				ScanDir(-0x10, qrMask, 0x1010, @F4, @R4, @N4)
				ScanDir(+0x10, qrMask, 0x2020, @F5, @R5, @N5)
				ScanDir(+0x01, qrMask, 0x4040, @F6, @R6, @N6)
				ScanDir(-0x01, qrMask, 0x8080, @F7, @R7, @N7)
	}
}	/* SearchAllCheckQRB */


void SearchAllCheckQRB1 ()				// Searches all direct queen/rook/bishop checks to the
{												// empty square "to". On entry: A4.L = &Board[to],	A3.L =
	register SQUARE *Bto;				// &AttackP[to] + "2", D2.W = AttackP[to] & dirMask and
	register ATTACK a;					// D3.W = ksq.
	register PTR	 Data;

	asm 68000	/* 86 bytes */
	{
				MOVE.L	Bto, D1
				LEA		Board, A0
				SUB.L		A0, D1

	@SCAN		MOVE.L	-2(A3), a
				LSR.W		#1, D1							;	m.to = to;
				MOVE.W	D1, move(to)
				MOVE.W	D2, D0
				LSR.W		#8, D0
				OR.B		D2, D0							;	for (bits = ...; bits; clrBit(j,bits))
				LSL.W		#4, D0							;	{
				LEA		QRBdata, Data
 	@FOR		ADDA.W	D0, Data							;		dir = QueenDir[j = LowBit[bits]];
				MOVE.W	(Data)+, D0
				MOVE.W	D0, move(dir)					;		m.dir = dir;
				ADD.W		D0, D0							;		m.from = to;
				MOVEA.L	Bto, A1							;		while (isEmpty(m.from -= dir));
	@while	SUBA.W	D0, A1
				MOVE.W	(A1), D2
				BEQ		@while
				MOVE.L	A1, D1
				LEA		Board, A0
				SUB.L		A0, D1
				LSR.W		#1, D1
				MOVE.W	D2, move(piece)				;		m.piece = Board[m.from];
				MOVE.W	D1, move(from)
				MOVEA.L	processMove, A0				;		(*processMove)();
				JSR		(A0)
	@ROF		MOVE.W	(Data), D0
				BNE		@FOR								;	}
	@return
	}
}	/* SearchAllCheckQRB1 */


/*--------------------------------- Search All Knight Checks ---------------------------------*/

void SearchAllCheckN ()						// Searches all checking moves by knights. On entry:
{													// A4.L = &Board[ksq] and A3.L = &AttackP[ksq].
	#define checkN(dir, nxt)							\
	asm {		MOVE.B	(4*dir + 1)(A3), D0			/*	if (! (bits = AttackP[m.to] & nMask))	*/	}\
	asm {		BEQ		nxt								/*		continue;									*/	}\
	asm {		TST.W		(2*dir)(A4)						/*	if (isOccupied(m.to))						*/	}\
	asm {		BNE		nxt								/*		continue;									*/	}\
	asm {		MOVE.W	#(4*dir), D1					}\
	asm {		JSR		SearchAllCheckN1				}\
	asm {nxt													}

	checkN(-0x0E, @0)		/* 162 bytes */
	checkN(-0x12, @1)
	checkN(-0x1F, @2)
	checkN(-0x21, @3)
	checkN(+0x12, @4)
	checkN(+0x0E, @5)
	checkN(+0x21, @6)
	checkN(+0x1F, @7)
}	/* SearchAllCheckN */


void SearchAllCheckN1 ()					// Searches all checking moves by knights to the
{													// square "sq" = ksq + dir. On entry: D3.W = ksq, A4.L =
	register ATTACK d;						// &Board[ksq], A3.L = &AttackP[ksq], D0.W = nBits and
	register PTR	 Data;					// D1.W = 4*dir.

	asm 68000	/* 56 bytes */
	{
				ASR.W		#2, D1							;	m.to = ksq + dir;
				ADD.W		D3, D1
				MOVE.W	D1, move(to)
				MOVE.W	node(knight), move(piece)	;	m.piece = friendly(knight);
				ANDI.W	#0x00FF, D0
				LSL.W		#3, D0							;	for (bits =...; bits; clrBit(j,bits))				
				LEA		Ndata, Data						;	{
	@for		ADDA.W	D0, Data							;		m.from = m.to - KnightDir[j=LowBit[bits]];
				MOVE.W	move(to), D1
				SUB.W		(Data)+, D1
				MOVE.W	D1, move(from)
				MOVEA.L	processMove, A0				;		(*processMove)();
				JSR		(A0)
	@rof		MOVE.W	(Data), D0
				BNE		@for								;	}
	@return
	}
}	/* SearchAllCheckN1 */


/*----------------------------------- Search All Pawn Checks ---------------------------------*/

void SearchAllCheckP ()						// Searches all, direct checks by pawns. On entry:
{													// A4.L = &Board[ksq] and A3.L = &AttackP[ksq] and
	/* 214 bytes */							// D3.W = ksq.	Local usage: D1.W = from, D2.W = dir/to
													// and A1.L = &Board[sq].
	#define checkP(dir, fp, sc, nxt)					}\
	asm {		MOVE.W	node(pawnDir2_), D2			/*	m.to = ksq + "dir" - pawnDir;				*/	}\
	asm {		LEA		(2*dir)(A4,D2.W), A1			/*	if (isEmpty(m.to))							*/ }\
	asm {		TST.W		(A1)								/*	{													*/	}\
	asm {		BNE		nxt								}\
	asm {		MOVE.W	D2, D1							/*		m.from = behind(m.to);					*/	}\
	asm {		MOVE.W	0(A1,D1.W), D0					/*		if (isEmpty(m.from))						*/	}\
	asm {		BNE		fp									/*			if ("double move") 					*/	}\
	asm {		MOVE.W	D3, D0							}\
	asm {		SUB.W		D2, D0							/*				m.from -= pawnDir;				*/ }\
	asm {		ANDI.W	#0xF0, D0						}\
	asm {		CMP.W		node(rank7), D0				}\
	asm {		BNE		nxt								/*			else										*/	}\
	asm {		ADD.W		D2, D1							/*				continue;							*/	}\
	asm {		MOVE.W	0(A1,D1.W), D0					}\
	asm { fp	CMP.W		node(pawn), D0					/*		if (Board[m.from]==friendly(pawn))	*/}\
	asm {		BNE		nxt								}\
	asm { sc	MOVEQ		#(dir), D0						/*		{												*/	}\
	asm {		ADD.W		D3, D0							}\
	asm {		ADD.W		node(pawnDir_), D0			}\
	asm {		MOVE.W	D0, move(to)					}\
	asm {		ASR.W		#1, D1							}\
	asm {		ADD.W		D1, D0							/*			m.piece = friendly(pawn);			*/	}\
	asm {		MOVE.W	D0, move(from)					}\
	asm {		MOVE.W	node(pawn), move(piece)		/*			(*processMove)();						*/	}\
	asm {		MOVEA.L	processMove, A0				/*		}												*/	}\
	asm {		JSR		(A0)								/*	}													*/	}\
	asm { nxt

	asm 68000
	{
				MOVEQ		#0x70, D0						;	if (! onRank8(kingSq_))
				AND.W		D3, D0							;	{
				CMP.W		node(rank8), D0				;		"Search pawn checks from left";
				BEQ		@return
				checkP(-1, @fpL, @scL, @RIGHT)		;		"Search pawn checks from right";
				checkP( 1, @fpR, @scR, @EXIT)			;	}
	@return
	}
}	/* SearchAllCheckP */


/**********************************************************************************************/
/*																															 */
/*												GENERATING NON CAPTURES												 */
/*																															 */
/**********************************************************************************************/

void MSearchNonCaptures ()								// Searches all normal, non-captures.
{
	register SQUARE *Loc;
	register INDEX	 i;

	asm 68000	/* 38 bytes */
	{
				CLR.L		move(cap)						;	m.type = normal; m.cap = empty;
				MOVE.W	node(lastPiece), i
				MOVEA.L	node(PieceLoc), Loc
				ADDQ.L	#2, Loc
				BRA		@rof
	@for		MOVE.W	(Loc)+, D0						;	for (i = 0; i <= LastPiece[player]; i++)
				BMI		@rof
				JSR		SearchNonCaptures1			;		SearchNonCaptures1(SLoc[i]);
	@rof		DBRA		i, @for
				MOVE.W	node(kingSq), D0				;	SearchNonCaptures1(kingLoc(player));
				JSR		SearchNonCaptures1
	}
}	/* MSearchNonCaptures */


/**********************************************************************************************/
/*																															 */
/*												 		CHECK TESTING													 */
/*																															 */
/**********************************************************************************************/

BOOL GivesCheck ()
{
	asm 68000
	{
				CMPI.W	#checkGen, node(gen)
				BEQ		@YES
				TST.W		move(type)
				BNE		@YES
				MOVE.W	move(piece), D2

	@DIRECT	LEA		AttackDir, A0
				MOVE.W	node(kingSq_), D0				;	dir = AttackDir[ksq - m->to];
				SUB.W		move(to), D0
				ADD.W		D0, D0							;	if (dir != 0)
				MOVE.W	0(A0,D0.W), D1					;	{
				BEQ		@INDIR
				LEA		AttackDirMask, A1				;		if (dir & AttackDirMask[m->piece])
				ADDA.W	D2, A1
				MOVE.W	0(A1,D2.W), D0					;		{
				AND.W		D1, D0
				BEQ		@INDIR							;			if (m->piece <= friendly(knight))
				CMP.W		node(knight), D2				;				return true;
				BLE		@YES
				ASR.W		#5, D1							;			dir >>= 5;
				ADD.W		D1, D1
				LEA		Board, A1						;			sq = m->to;
				MOVE.W	move(to), D0
				ADD.W		D0, D0
				ADDA.W	D0, A1							;			while (isEmpty(sq += dir));
	@loop		ADDA.W	D1, A1
				MOVE.W	(A1), D0							;			if (Board[sq] == enemy(king))
				BEQ		@loop								;				return true;
				CMP.W		node(king_), D0				;		}
				BEQ		@YES								;	}

	@INDIR	CMP.W		node(queen), D2				;	if (m->piece == friendly(queen))
				BEQ		@NO								;		return false;
				MOVE.W	node(kingSq_), D0				;	dir = AttackDir[ksq - m->from];
				SUB.W		move(from), D0
				ADD.W		D0, D0			
				MOVE.W	0(A0,D0.W), D1
				MOVEQ		#qDirMask, D0					;	if ((dir & qDirMask) &&
				AND.W		D1, D0
				BEQ		@NO
				MOVEA.L	node(Attack), A1				;		 (AttackP[m->from]Ê& DirBit[dir]))
				MOVE.W	move(from), D2
				ADD.W		D2, D2							;	{
				ADDA.W	D2, A1
				MOVE.W	2(A1,D2.W), D0
				BEQ		@NO
				LEA		DirBit, A1
				ASR.W		#3, D1
				ANDI.W	#0xFFFC, D1
				AND.W		2(A1,D1.W), D0
				BEQ		@NO
				ASR.W		#1, D1							;		dir >>= 5;
				LEA		Board, A1						;		sq = m->from;
				ADDA.W	D2, A1							;		while (isEmpty(sq += dir));
	@loop2	ADDA.W	D1, A1
				MOVE.W	(A1), D0							;		if (Board[sq] == enemy(king))
				BEQ		@loop2								;			return true;
				CMP.W		node(king_), D0
				BEQ		@YES								;	}
				RTS

	@NO		MOVEQ		#false, D0						;	return false;
				RTS
	@YES		MOVEQ		#true, D0
				RTS
	}				
}	/* GivesCheck */

#endif
