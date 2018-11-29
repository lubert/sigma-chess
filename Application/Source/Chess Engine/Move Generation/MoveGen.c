/**************************************************************************************************/
/*                                                                                                */
/* Module  : MOVEGEN.C */
/* Purpose : This module contains all the move generation routines. Move
 * generation is always     */
/*           performed implicitly in the current search node ("rNode"). */
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

#include "Engine.h"

#include "Engine.f"
#include "MoveGen.f"
#include "PerformMove.f"
#include "Search.f"
#include "Threats.f"

//### NB!!! Use Bucket Sort for normal non-captures (perhaps just 2 or 3
//buckets). E.g. introduce a
// a semi-sacrifice buffer for poor positional moves (if PV[to] < PV[from])

// The move generation routines all expect register "rNode" to contain a pointer
// to the current node "N". Sacrifice moves are stored in the sacrifice buffer
// "SBuf". Non-sacrifices are stored in "N->m" and are processed immediately by
// ProcessMove(). The move generation routines sets the "from", "to", "piece",
// "cap", "type", "dir" and "misc" (gen) fields of "N->m" - the "dply" field
// must be set by the calling search routines (with the exception of
// "SearchEscapes()" which sets "dply" itself). The move generation is divided
// into 12 phases (A-L):
//
// [A] SearchEnPriseCaptures()  Generates "en prise captures", i.e. captures of
// undefended or
//                              higher valued pieces. Queen promotions which are
//                              also captures are searched too (the under
//                              promotions are added to "SBuf").
//
// [B] SearchPromotions()       Generates non-capturing queen promotions. Again
// the under promo-
//                              tions are added to "SBuf".
//
// [C] SearchRecaptures()       Searches safe recaptures or en passant captures.
// In the former
//                              case non-safe recaptures are added to the
//                              sacrifice buffer and the square on which the
//                              captures are made is stored in the field
//                              "N->recapSq". If the last move was not a
//                              capture, "N->recapSq" is set to "nullSq" and en
//                              passsant captures are searched.
//
// [D] SearchSafeCaptures()     Searches safe captures which are not recaptures
// or en prise cap-
//                              tures. Non-safe captures are added to the
//                              sacrifice buffer.
//
// [E] SearchEscapes()          Searches escape moves, i.e. non-capturing moves
// of the highest
//                              valued piece which is threatened by a lower
//                              valued piece. Unsafe moves are added to the
//                              sacrifice buffer.
//
// [F] SearchKillers()          Searches the killer moves if they're
// pseudo-legal in the current
//                              position. The killer moves are defined as the
//                              best phase H or I moves so far in a previous
//                              line at the same depth.
//
// [G] SearchCastling()         Searches king-side and queen-side castling in
// that order.
//
// [H] SearchNonCaptures()      Searches safe, normal non-capturing moves which
// are not escape
//                              moves. Unsafe moves are added to "SBuf".
//
// [I] SearchSacrifices()       Searches the moves which have been stored in the
// sacrifice buffer
//                              by the previous move generation phases.
//
// [J] SearchSafeChecks()       Searches safe checks. Is only used in place of
// phase F, G and H
//                              in the quiescence search.
//
// [K] SearchFarPawns()         Searches all non-capturing pawn moves to the 6th
// and 7th rank. Like
//                              phase J, this is only done in the quiescence
//                              search.
//
// [L] SearchCheckEvasion()     Special routine that is called when the side to
// move is in check
//                              in place of all other phases.
//
// The move generators are basically used in three different scenarious:
//
// (1) If the side to move is check, only phase L is performed.
// (2) In the normal full width search, phases A - I are performed.
// (3) In the quiescence search, phases A - E, J, K and optionally I are
// performed.

static asm void ProcessMove(void);
static asm void AddSacrifice(void);

/**************************************************************************************************/
/*                                                                                                */
/*                                     GENERATING ROOT MOVES */
/*                                                                                                */
/**************************************************************************************************/

// Before the search can start, the GenRootMoves() routine must be called. This
// generates all strictly legal move for the "player" and stores these in the
// E->S.RootMoves[] table. Before this routine can be called, the E->S.RootNode
// pointer must have been initialized.

void GenRootMoves(ENGINE *E) {
  NODE *N = E->S.rootNode;

  E->R.state = state_Root;  // Instruct "ProcessMove()" to generate root moves.

  E->S.numRootMoves = 0;    // Clear RootMoves[] table.
  E->S.bufTop = E->S.SBuf;  // Reset sacrifice buffer.

  Asm_Begin(E);

  AnalyzeThreats();  // Compute escapeSq, ALoc/SLoc etc.

  if (N->check) {
    N->m.dply = 1;  // CHECK EVASION.
    SearchCheckEvasion();
  } else {
    N->m.dply = 0;            // [0] FORCED MOVES (dply = 0):
    SearchEnPriseCaptures();  // Generate en prise captures, queen
    SearchPromotions();       // promotions and safe recaptures.
    SearchRecaptures();

    N->m.dply = 1;         // [1] NON-FORCED MOVES (dply = 1):
    SearchSafeCaptures();  // Generate normal moves and (forced)
    N->eply = 0;
    SearchEscapes();  // escapes.
    N->m.dply = 1;
    SearchCastling();  // Generate castling if not in check.

    N->m.dply = 2;        // [2] GENERATE QUIET MOVES (dply = 2):
    SearchNonCaptures();  // Generate non-captures.
    SearchSacrifices();   // Generate sacrifice moves.
  }

  Asm_End();

  E->S.bufTop = E->S.SBuf;  // Reset the sacrifice buffer again!
  clrMove(N->m);
} /* GenRootMoves */

static void GenOneRootMove(register ENGINE *E);

static void GenOneRootMove(
    register ENGINE *E)  // Stores the generated move in RootMoves[] if
{                        // the move is strictly legal.
  SEARCH_STATE *S = &E->S;
  NODE *N = E->S.rootNode;

  PerformMove();

  if (!N->Attack_[N->PieceLoc[0]])  // If move is strictly legal
  {
    S->RootMoves[S->numRootMoves] =
        S->rootNode->m;  // then add it to the root moves
    S->RootMoves[S->numRootMoves].misc =
        S->rootNode->gen;  // list and store move generator
    S->numRootMoves++;
  }

  RetractMove();
} /* GenOneRootMove */

/**************************************************************************************************/
/*                                                                                                */
/*                                     PROCESS GENERATED MOVES */
/*                                                                                                */
/**************************************************************************************************/

// The ProcessMove() routine is called by the move generators for each generated
// move. Normally ProcessMove() searches the move in question. When the search
// is to be terminated (i.e. when E->R.state = state_Stopping is set), the
// routine simply does nothing. At the root node, before the search begins,
// ProcessMove() is used to generate all the strictly legal root moves (in the
// E->S.RootMoves[] table).

static asm void ProcessMove(void) {
  lhz rTmp2,
      ENGINE.R.state(rEngine)  // switch (E->R.state)
      cmpi cr5,
      0, rTmp2,
      state_Running  // {
              beq +
          cr5,
      SearchMove  //    case state_Running : SearchMove(); break;
          cmpi cr0,
      0, rTmp2,
      state_Root  //
          mr rTmp1,
      rEngine  //    case state_Root : GenOneRootMove(rEngine); break;
              bnelr -
          call_c(GenOneRootMove) blr  //    default : return;
} /* ProcessMove */                   // }

/**************************************************************************************************/
/*                                                                                                */
/*                                      [A] EN PRISE CAPTURES */
/*                                                                                                */
/**************************************************************************************************/

asm void SearchEnPriseCaptures1(register SQUARE _sq, register SQUARE _sq4,
                                register ATTACK _asq);
asm void SearchPromotion1(void);

// En Prise captures are generated by scanning the opponent PieceLoc table from
// top to bottom (i.e. Queens first, followed by rooks e.t.c.). If an opponent
// piece is en prise (i.e. attacked by a lower valued piece and/or undefended),
// then capture moves are generated/searched.

/*---------------------------------- Search All En Prise Captures
 * --------------------------------*/

asm void SearchEnPriseCaptures(
    void)  // Generates and searches all en prise captures, i.e.
{          // captures of undefended and/or higher valued pieces.
#define PL rLocal1
#define i rLocal2

   frame_begin2

   lhz     i, node(lastPiece_)
   li      rTmp4, mtype_Normal
   li      rTmp9, gen_A                   // N->gen = gen_A;
   mr      PL, rPieceLoc_
   sth     rTmp4, move(type)              // m.type = mtype_Normal;
   sth     rTmp9, node(gen)
   b       @rof
@for
   lhau    rTmp1, 2(PL)                   // for (i = 1; i <= LastPiece[opponent]; i++)
   rlwinm. rTmp2, rTmp1,2,0,29            // {
   blt     @rof                           //    sq = PieceLoc[opponent + i];
   lwzx    rTmp3, rAttack,rTmp2
   cmpi    cr5,0, rTmp3,0                 //    if (sq >= a1 && AttackP[sq])
   bnel    cr5, SearchEnPriseCaptures1    //       SearchEnPriseCaptures1(sq,sq4,AttackP[sq]);
@rof
   addi    i, i,-1
   cmpi    cr0,0, i,0
   bge+    cr0, @for                      // }

   frame_end2
   blr

#undef PL
#undef i
} /* SearchEnPriseCaptures */

/*---------------------------- Search En Prise Captures of 1 Piece
 * -------------------------------*/
// Searches en prise captures of one piece on the square "sq". On entry:
//    rTmp1 : sq
//    rTmp2 : sq4
//    rTmp3 : AttackP[sq]

asm void SearchEnPriseCaptures1(register SQUARE _sq, register SQUARE _sq4,
                                register ATTACK _asq) {
#define ec_search_p(pdirmask, pdir, L, P)                               \
  andis.rTmp3, a, pdirmask;   /* if (a & pawnBitX)                   */ \
  beq - L;                    /* {                                   */ \
  addi rTmp1, rPawnDir, pdir; /*    m.from = to - pawnDirX;          */ \
  addi rTmp2, rPlayer, pawn;  /*    m.piece = friendly(pawn);        */ \
  subf rTmp4, rTmp1, dest;    /*    m.dir = pawnDirX;                */ \
  sth rTmp2, move(piece);     /*    ProcessMove/SearchPromotion      */ \
  sth rTmp4, move(from);      /* }                                   */ \
  bl P

#define ec_search_qrb(p, L0, L1, L2)                                     \
  beq - L2; /* while (bits)                        */                    \
  addi Data, rGlobal, GLOBAL.M.QRBdata; /* { */                                                                      \
  addi rTmp2, rPlayer, p;      /*    m.piece = friendly(...);         */ \
  sth rTmp2, move(piece);      /*    Data = QRBdata[bits];            */ \
  L0 lhaux rTmp3, Data, rTmp1; /*    m.dir = dir = QRBData[bits].dir; */ \
  mr rTmp4, dest;              /*    m.from = m.to;                   */ \
  L1 subf rTmp4, rTmp3, rTmp4; /*    while (isEmpty(m.from -= dir));  */ \
  add rTmp5, rTmp4, rTmp4;                                               \
  lhax rTmp6, rBoard, rTmp5;                                             \
  cmpi cr0, 0, rTmp6, empty;                                             \
  beq + cr0, L1;                                                         \
  sth rTmp3, move(dir);                                                  \
  sth rTmp4, move(from);                                                 \
  bl ProcessMove; /*    (*ProcessMove)();                */              \
  lha rTmp1, QRB_DATA.offset(Data);                                      \
  cmpi cr0, 0, rTmp1, 0; /*    bits = QRBdata[bits].nextBits;   */       \
  bne - cr0, L0          /* }                                   */

#define d rLocal1     // Defense (opponent attack) of "sq".
#define a rLocal2     // Attack (player attack) of "sq".
#define dest rLocal3  // "sq"
#define pcap rLocal4  // The capture piece type.
#define Data rLocal5  // QRB capture data pointer.

  frame_begin5

      add rTmp4,
      rTmp1,
      rTmp1  // cap = pieceType(m.cap);
          lwzx d,
      rAttack_,
      rTmp2  // d = AttackO[sq];
          mr dest,
      rTmp1 lhzx pcap, rBoard, rTmp4 add rTmp5, dest, rPawnDir mr a,
      rTmp3  // a = AttackP[Sq];
          sth dest,
      move(to)  // m.to = sq;
      sth pcap,
      move(cap)  // m.cap = Board[sq];
      andi.pcap,
      pcap,
      0x07

      //--- CAPTURE WITH PAWNS (INCL PROMOTIONS) ---
      @PAWN andi.rTmp5,
      rTmp5,
      0x88  // if (onRank8(to))
          beq +
          @noProm  // {
              ec_search_p(0x0400, -1, @P0, SearchPromotion1)
                  @P0 ec_search_p(0x0200, 1, @KNIGHT, SearchPromotion1) b
          @KNIGHT  // }
          @noProm cmpi cr5,
      0, d,
      0  // else if (d == 0 || capMtrl > pawnMtrl)
      beq cr5,
      @P1  // {
          cmpi cr6,
      0, pcap, pawn beq cr6,
      @RETURN @P1 ec_search_p(0x0400, -1, @P2, ProcessMove)
          @P2 ec_search_p(0x0200, 1, @KNIGHT, ProcessMove)
      // }
      //--- CAPTURE WITH KNIGHTS ---
      @KNIGHT cmpi cr5,
      0, d,
      0  // if (d && capMtrl <= knightMtrl)
      cmpi cr6,
      0, pcap, bishop beq - cr5,
      @N1  //    return;
              ble +
          cr6,
      @RETURN @N1 andis.rTmp1, a,
      0x00FF  // bits = (a & nMask) >> 16;
          beq +
          @BISHOP srawi rTmp1,
      rTmp1,
      13  // if (bits)
      addi Data,
      rGlobal,
      GLOBAL.M.Ndata  // {
          addi rTmp2,
      rPlayer,
      knight  //    m.piece = friendly(knight);
          andi.rTmp1,
      rTmp1,
      0x07F8  //    Data = Ndata;
      sth rTmp2,
      move(piece) @NFor lhaux rTmp2, Data,
      rTmp1  //    do
          subf rTmp2,
      rTmp2,
      dest  //    {
          sth rTmp2,
      move(from)      //       m.from = m.to + Ndata[bits].ndir;
      bl ProcessMove  //       ProcessMove(...);
          lha rTmp1,
      N_DATA.offset(Data)  //       bits = Ndata[bits].nextBits;
      cmpi cr0,
      0, rTmp1,
      0  //    } while(bits);
          bne -
          cr0,
      @NFor  // }

      // --- CAPTURE WITH BISHOPS ---
      @BISHOP andi.rTmp1,
      a,
      0x0F00  // for (bits = (a & bMask) >> 8; bits; clrBit(j,bits))
      srawi.rTmp1,
      rTmp1,
      4                                       // {  ... MACRO ...
      ec_search_qrb(bishop, @B0, @B1, @ROOK)  // }

      // --- CAPTURE WITH ROOKS ---
      @ROOK cmpi cr5,
      0, d,
      0  // if (d && capMtrl <= rookMtrl)
      cmpi cr6,
      0, pcap, rook beq - cr5,
      @R0  //    return;
              ble +
          cr6,
      @RETURN @R0 srawi rTmp1, a,
      4  // for (bits = (a & rMask) >> 8; bits; clrBit(j,bits))
      andi.rTmp1,
      rTmp1,
      0x0F00                                 // {  ... MACRO ...
      ec_search_qrb(rook, @R1, @R2, @QUEEN)  // }

      // --- CAPTURE WITH QUEENS ---
      @QUEEN cmpi cr5,
      0, d,
      0  // if (d)
          bne +
          cr5,
      @RETURN  //    return;
          rlwinm.rTmp1,
      a, 4, 20,
      27  // for (bits = a & qMask; bits; clrBit(j,bits))
      ec_search_qrb(queen, @Q1, @Q2, @KING)  // {  ... MACRO ...
                                             // }
      // --- CAPTURE WITH KING ---
      @KING andis.rTmp1,
      a,
      0x0100  // if (a & kMask)
          beq +
          @RETURN  // {
              lha rTmp3,
      0(rPieceLoc)addi rTmp2, rPlayer,
      king  //    m.piece = friendly(king);
          sth rTmp2,
      move(piece)  //    m.from = kingLoc(player);
      sth rTmp3,
      move(from)      //    ProcessMove(...);
      bl ProcessMove  // }

      @RETURN frame_end5 blr

#undef d
#undef a
#undef dest
#undef pcap
#undef Data
} /* SearchEnpriseCaptures1 */

/**************************************************************************************************/
/*                                                                                                */
/*                                         [B] PROMOTIONS */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------- Search All Non-capturing Promotions
 * ------------------------------*/
// The "SearchPromotions()" routine generates and searches all non-capturing
// promotions. Queen promotions are searched immediately, whereas
// under-promotions are added to the sacrifice buffer.

asm void SearchPromotions(void) {
#define search_prom_pawns(psx, rto, L1, L2)                                   \
  lbz rTmp1, (psx)(rEngine); /* bits = PawnStructX[rfrom];               */   \
  sth rTmp9, node(gen);                                                       \
  cmpi cr5, 0, rTmp1, 0; /* if (bits == 0) return;                   */       \
  beqlr + cr5;                                                                \
  frame_begin1;                                                               \
  mr bits, rTmp1;                                                             \
  L1;                 /* for (bits = ...; bits; clrbit(j,bits))   */          \
  cntlzw rTmp2, bits; /* {                                        */          \
  li rTmp1, 31;       /*    f = LowBit[bits];                     */          \
  subf rTmp2, rTmp2, rTmp1;                                                   \
  addi rTmp3, rTmp2, rto; /*    m.to = rto + f;                       */      \
  add rTmp4, rTmp3, rTmp3;                                                    \
  li rTmp5, 1;                                                                \
  lhax rTmp6, rBoard, rTmp4; /*    if (Board[m.to] != empty) continue;   */   \
  rlwnm rTmp7, rTmp5, rTmp2, 0, 31;                                           \
  andc bits, bits, rTmp7;                                                     \
  cmpi cr5, 0, rTmp6, empty;                                                  \
  bne cr5, L2;                                                                \
  subf rTmp4, rPawnDir, rTmp3; /*    m.from  = m.to - pawnDir;             */ \
  li rTmp1, 0;                 /*    m.piece = friendly(pawn);             */ \
  addi rTmp2, rPlayer, pawn;   /*    m.cap   = empty;                      */ \
  sth rTmp1, move(cap);                                                       \
  sth rTmp2, move(piece);                                                     \
  sth rTmp3, move(to);                                                        \
  sth rTmp4, move(from);                                                      \
  bl SearchPromotion1; /*    SearchPromotion1();                   */         \
  L2;                                                                         \
  cmpi cr6, 0, bits, 0;                                                       \
  bne - cr6, L1;                                                              \
  frame_end1; /* }                                        */                  \
  blr

#define bits rLocal1

  cmpi cr0, 0, rPlayer, white li rTmp9,
      gen_B  // N->gen = gen_B;
          bne @BLACK
      @WHITE search_prom_pawns(ENGINE.B.PawnStructW + 6, 0x70, @W1a, @W2a)
          @BLACK search_prom_pawns(ENGINE.B.PawnStructB + 1, 0x00, @B1a, @B2a)

#undef bits
} /* SearchPromotions */

/*--------------------------------- Search One Promotion Move
 * ------------------------------------*/
// "Turns" the current promotion move N->m into a queen promotion and searches
// that move. Under promotions are stored in the sacrifice buffer. On exit,
// N->m.type is restored to mtype_Normal.

asm void SearchPromotion1(void) {
  addi rTmp2, rPlayer,
      queen  // m.type = friendly(queen);
          mflr r0 sth rTmp2,
      move(type) stwu r0,
      -4(SP)bl ProcessMove  // ProcessMove();
          lwz rTmp2,
      move(piece) lwz rTmp3, move(to) lwz rTmp5,
      ENGINE.S.bufTop(rEngine) li rTmp1, mtype_Normal lwz r0, 0(SP)sth rTmp1,
      move(type)  // m.type = mtype_Normal;

      addi rTmp4,
      rPlayer,
      rook  // SBuf[topS] = m;
          stw rTmp2,
      0(rTmp5)  // SBuf[topS++].type = friendly(rook);
      stw rTmp3,
      4(rTmp5)sth rTmp4,
      8(rTmp5)

          addi rTmp4,
      rPlayer,
      knight  // SBuf[topS] = m;
          stwu rTmp2,
      16(rTmp5)  // SBuf[topS++].type = friendly(knight);
      stw rTmp3,
      4(rTmp5)sth rTmp4,
      8(rTmp5)

          addi rTmp4,
      rPlayer,
      bishop  // SBuf[topS] = m;
          stwu rTmp2,
      16(rTmp5)  // SBuf[topS++].type = friendly(bishop);
      stw rTmp3,
      4(rTmp5)sth rTmp4,
      8(rTmp5)

          addi rTmp5,
      rTmp5, 16 addi SP, SP, 4 mtlr r0 stw rTmp5,
      ENGINE.S.bufTop(rEngine)

          blr
} /* SearchPromotion1 */

/**************************************************************************************************/
/*                                                                                                */
/*                                          [C] RECAPTURES */
/*                                                                                                */
/**************************************************************************************************/

asm void SearchSafeCaptures1(register SQUARE _sq, register SQUARE _sq4,
                             register ATTACK _attp, register ATTACK _atto);
asm void SearchEnPassant(void);

// If the previous move was a capture on a square "sq", then safe captures
// (which are not also en prise captures) of the piece on "sq" are searched and
// N->recapSq is set to "sq". Otherwise N->recapSq is set to "nullSq" and en
// passant moves (if any) are searched.

asm void SearchRecaptures(void) {
  lhz rTmp5, pmove(cap) li rTmp9,
      gen_C  // N->gen = gen_C;
          li rTmp6,
      nullSq sth rTmp9,
      node(gen)  // if (pm.cap)
      cmpi cr0,
      0, rTmp5,
      empty beq - @noRecap  // {

                      lhz rTmp1,
      pmove(to)  //    N->recapSq = pm.to;
      sth rTmp1,
      node(recapSq) rlwinm rTmp2, rTmp1, 2, 0,
      29  //    if (AttackP[recapSq] && AttackO[recapSq])
      lwzx rTmp3,
      rAttack,
      rTmp2  //    {
          lwzx rTmp4,
      rAttack_,
      rTmp2  //       m.type = mtype_Normal;
          cmpi cr5,
      0, rTmp3,
      0 beqlr cr5  //       SearchSafeCaptures1
          cmpi cr6,
      0, rTmp4,
      0  //       (  recapSq, recapSq4,
      li rTmp5,
      mtype_Normal   //         AttackP[recapSq], AttackO[recapSq]);
          beqlr cr6  //    }
              sth rTmp5,
      move(type) b SearchSafeCaptures1  // }
                                        // else
      @noRecap                          // {  N->recapSq = nullSq;
          sth rTmp6,
      node(recapSq)      //    SearchEnPassant();
      b SearchEnPassant  // }
} /* SearchRecaptures */

/*-------------------------------------- Search En Passant
 * ---------------------------------------*/
// Searches en passant captures. Need only be called if the previous move was
// not a capture.

asm void SearchEnPassant(void) {
  lhz rTmp1,
      pmove(piece)  // if (pm.piece == enemy(pawn) &&
      lhz rTmp2,
      pmove(from) andi.rTmp1, rTmp1, 0x06 bnelr + lhz rTmp3,
      pmove(to)  //     pm.from - pm.to == 2*pawnDir)
      add rTmp4,
      rPawnDir, rPawnDir subf rTmp5, rTmp4, rTmp2 cmp cr0, 0, rTmp3,
      rTmp5 bnelr +  // {

          @L addi rTmp1,
      rTmp3,
      -1  //    if (Board[left(pm.to) == friendly(pawn))
      add rTmp4,
      rTmp1,
      rTmp1  //    {
          lhzx rTmp4,
      rBoard, rTmp4 addi rTmp6, rPlayer, pawn cmp cr0, 0, rTmp4,
      rTmp6 bne - @R sth rTmp1,
      move(from)  //       m.from  = right(pm.to);
      add rTmp7,
      rPawnDir, rTmp3 sth rTmp6,
      move(piece)  //       m.to    = pm.to + pawnDir
      li rTmp8,
      mtype_EP sth rTmp7,
      move(to)  //       m.piece = friendly(pawn);
      mflr r0 li rTmp9,
      empty  //       m.cap   = empty;
          sth rTmp8,
      move(type)  //       m.type  = enPassant;
      stwu r0,
      -4(SP)sth rTmp9,
      move(cap)       //       ProcessMove();
      bl ProcessMove  //    }
          lwz r0,
      0(SP)lhz rTmp3, pmove(to) addi SP, SP,
      4 mtlr r0

      @R addi rTmp1,
      rTmp3,
      1  //    if (Board[left(pm.to) == friendly(pawn))
      add rTmp4,
      rTmp1,
      rTmp1  //    {
          lhzx rTmp4,
      rBoard, rTmp4 addi rTmp6, rPlayer, pawn cmp cr0, 0, rTmp4,
      rTmp6 bnelr - sth rTmp1,
      move(from)  //       m.from  = right(pm.to);
      add rTmp7,
      rPawnDir, rTmp3 sth rTmp6,
      move(piece)  //       m.to    = pm.to + pawnDir
      li rTmp8,
      mtype_EP sth rTmp7,
      move(to)  //       m.piece = friendly(pawn);
      li rTmp9,
      empty  //       m.cap   = empty;
          sth rTmp8,
      move(type)  //       m.type  = enPassant;
      sth rTmp9,
      move(cap)      //       ProcessMove();
      b ProcessMove  //    }
                     // }
} /* SearchEnPassant */

/**************************************************************************************************/
/*                                                                                                */
/*                                         [D] SAFE CAPTURES */
/*                                                                                                */
/**************************************************************************************************/

/*------------------------------------- Search all Safe Captures
 * ---------------------------------*/
// Searches safe captures which are neither recaptures or en prise captures. The
// former case is handled by not searching captures of the piece (if any) on
// N->recapSq. The latter case is checked individually for each potential
// capture. Also, sacrifice captures are stored in the sacrifice buffer.

asm void SearchSafeCaptures(void) {
#define PL rLocal1
#define i rLocal2
#define rcpSq rLocal3

   frame_begin3

   lhz     i, node(lastPiece_)
   lha     rcpSq, node(recapSq)
   li      rTmp9, gen_D                   // N->gen = gen_D;
   li      rTmp3, mtype_Normal
   mr      PL, rPieceLoc_
   sth     rTmp9, node(gen)
   sth     rTmp3, move(type)              // m.type = normal
   b       @rof
@for
   lhau    rTmp1, 2(PL)                   // for (i = 1; i <= LastPiece[opponent]; i++)
   rlwinm. rTmp2, rTmp1,2,0,29            // {
   cmp     cr5,0,rTmp1,rcpSq              //    sq = PieceLoc[opponent + i];
   blt     @rof
   beq     cr5,@rof                       //    if (sq >= a1 && sq != recapSq &&
   lwzx    rTmp3, rAttack,rTmp2           //        Attack[sq] && 
   lwzx    rTmp4, rAttack_,rTmp2          //        Attack_[sq])
   cmpi    cr6,0, rTmp3,0
   cmpi    cr7,0, rTmp4,0
   beq     cr6,@rof
   bnel    cr7,SearchSafeCaptures1        //       SearchSafeCaptures1(...);
@rof
   addi    i, i,-1
   cmpi    cr0,0,i,0
   bge+    cr0,@for                       // }

   frame_end3
   blr

#undef PL
#undef i
#undef rcpSq
} /* SearchSafeCaptures */

/*--------------------------------- Search Safe Captures of 1 Piece
 * ------------------------------*/
// Searches all safe captures of one piece on the specified square. It is
// assumed that m.type = mtype_Normal. On entry:
//    rTmp1 : sq
//    rTmp2 : 4*sq
//    rTmp3 : Attack[sq] (<> 0)
//    rTmp4 : Attack_[sq] (<> 0)

asm void SearchSafeCaptures1(register SQUARE _sq, register SQUARE _sq4,
                             register ATTACK _attp, register ATTACK _atto) {
#define sc_search_qrb(p, xMtrl, L0, L1, L2, L3, L4, L5)                  \
  beq L5; /* while (bits)                        */                      \
  addi Data, rGlobal, GLOBAL.M.QRBdata; /* { */                                                                      \
  addi rTmp2, rPlayer, p; /*    m.piece = friendly(...);         */      \
  sth rTmp2, move(piece);                                                \
  L0 lhaux rTmp3, Data, rTmp1; /*    m.dir = dir = QRBData[bits].dir; */ \
  mr rTmp4, dest;              /*    m.from = m.to;                   */ \
  L1 subf rTmp4, rTmp3, rTmp4; /*    while (isEmpty(m.from -= dir));  */ \
  add rTmp5, rTmp4, rTmp4;                                               \
  lhax rTmp6, rBoard, rTmp5;                                             \
  cmpi cr5, 0, rTmp6, empty;                                             \
  beq + cr5, L1;                                                         \
  cmpi cr6, 0, capMtrl, xMtrl; /*    if (capMtrl == xMtrl ||          */ \
  sth rTmp3, move(dir);                                                  \
  sth rTmp4, move(from);                                                 \
  beq cr6, L2; /*        maxMtrl >= xMtrl &&          */                 \
  cmpi cr5, 0, maxMtrl, xMtrl;                                           \
  cntlzw rTmp7, ap; /*        (bitcount(ap) > 1 ||         */            \
  blt cr5, L3;                                                           \
  rlwnm.rTmp7, ap, rTmp7, 1, 31;                                         \
  add rTmp5, rTmp5, rTmp5; /*         (Attack[m.from]&RayBit[j])) */     \
  bne L2;                                                                \
  lwzx rTmp2, rAttack, rTmp5;                                            \
  lwz rTmp3, QRB_DATA.rayBit(Data);                                      \
  and.rTmp1, rTmp2, rTmp3; /*       )                             */     \
  beq L3;                                                                \
  L2 bl ProcessMove;  /*       ProcessMove();                */          \
  b L4;               /*    else                             */          \
  L3 bl AddSacrifice; /*       Sacrifice();                  */          \
  L4 lha rTmp1, QRB_DATA.offset(Data);                                   \
  cmpi cr5, 0, rTmp1, 0; /*    bits = QRBdata[bits].nextBits;   */       \
  bne - cr5, L0          /* }                                   */

#define dest rLocal1     // The target/capture square (m.to)
#define ap rLocal2       // Player's attack bits on "dest" (Attack[m.to])
#define capMtrl rLocal3  // Material value of captured piece (Mtrl[Board[m.to]])
#define maxMtrl rLocal4  // Optimistic maximum value of exchange
#define Data rLocal5     // Auxiliary data table

  frame_begin5

      add rTmp5,
      rTmp1, rTmp1 mr ap, rTmp3 sth rTmp1,
      move(to)  // m.to = sq;
      lhax rTmp6,
      rBoard,
      rTmp5  // m.cap:6 = Board[to]
          addi rTmp7,
      rGlobal, GLOBAL.B.Mtrl mr dest,
      rTmp1

          andis.rTmp10,
      rTmp4,
      0x0600  // capMtrl = Mtrl[cap];
      li rTmp9,
      pawnMtrl add rTmp2, rTmp6, rTmp6 sth rTmp6,
      move(cap)  // maxMtrl:9 = capMtrl + SmattMtrl(d);
      lhzx capMtrl,
      rTmp7, rTmp2 bne @calcMaxMtrl andis.rTmp10, rTmp4, 0x00FF li rTmp9,
      knightMtrl bne @calcMaxMtrl andi.rTmp10, rTmp4,
      0x0F00 bne @calcMaxMtrl andi.rTmp10, rTmp4, 0xF000 li rTmp9,
      rookMtrl bne @calcMaxMtrl li rTmp9,
      queenMtrl

      @calcMaxMtrl cmpi cr5,
      0, capMtrl,
      knightMtrl  // switch (capMtrl)
          cmpi cr6,
      0, capMtrl,
      rookMtrl  // {
          add maxMtrl,
      capMtrl, rTmp9 blt + cr5, @PAWN beq cr5, @KNIGHT beq cr6,
      @ROOK b @QUEEN

      @PAWN  //    case pawnMtrl:
          andis.rTmp1,
      ap,
      0x0400  //       if (a & pawnBitL)
      beq cr0,
      @P0  //       {
          addi rTmp6,
      rPlayer,
      pawn  //          m.piece = friendly(pawn);
          addi rTmp5,
      rPawnDir, -1 sth rTmp6, move(piece) subf rTmp7, rTmp5,
      dest  //          m.from = m.to - m.dir;
          sth rTmp7,
      move(from) bl ProcessMove  //          ProcessMove();
      @P0                        //       }
          andis.rTmp1,
      ap,
      0x0200  //       if (a & pawnBitR)
      beq cr0,
      @KNIGHT  //       {
          addi rTmp6,
      rPlayer,
      pawn  //          m.piece = friendly(pawn);
          addi rTmp5,
      rPawnDir, 1 sth rTmp6, move(piece) subf rTmp7, rTmp5,
      dest  //          m.from = m.to - m.dir;
          sth rTmp7,
      move(from) bl ProcessMove  //          ProcessMove();
                                 //       }
      @KNIGHT                    //    case knightMtrl: case bishopMtrl:
          andis.rTmp1,
      ap,
      0x00FF  //       bits = (a & nMask) >> 16;
      beq @BISHOP srawi rTmp1,
      rTmp1,
      13  //       if (bits)
      addi Data,
      rGlobal,
      GLOBAL.M.Ndata  //       {
          addi rTmp3,
      rPlayer,
      knight  //          m.piece = friendly(knight);
          andi.rTmp1,
      rTmp1,
      0x07F8  //          Data = Ndata;
      sth rTmp3,
      move(piece) @NFor  //          do
          lhaux rTmp2,
      Data,
      rTmp1  //          {
          cmpi cr5,
      0, capMtrl,
      knightMtrl  //             m.from = m.to - Ndata[bits].ndir;
          cntlzw rTmp4,
      ap subf rTmp3, rTmp2, dest sth rTmp3,
      move(from)  //             if (capMtrl == knightMtrl ||
      beq cr5,
      @N1 cmpi cr6, 0, maxMtrl,
      knightMtrl  //                 maxMtrl >= knightMtrl &&
          rlwnm.rTmp4,
      ap, rTmp4, 1, 31 blt cr6,
      @N2                             //                 bitcount(a) > 1)
          beq @N2 @N1 bl ProcessMove  //                ProcessMove();
              b @N3 @N2               //             else
                  bl AddSacrifice     //                AddSacrifice();
      @N3 lha rTmp1,
      N_DATA.offset(Data)  //             bits = Ndata[bits].nextBits;
      cmpi cr0,
      0, rTmp1,
      0  //          } while(bits);
          bne -
          @NFor  //       }

          @BISHOP  //
              andi.rTmp1,
      ap,
      0x0F00  //       for (bits = (a & bMask) >> 8; bits; clrBit(j,bits))
      srawi.rTmp1,
      rTmp1,
      4 sc_search_qrb(bishop, bishopMtrl, @B0, @B1, @B2, @B3, @B4, @ROOK)

          @ROOK  //    case rookMtrl:
              srawi.rTmp1,
      ap, 4 andi.rTmp1, rTmp1,
      0x0F00  //       for (bits = (a & rMask) >> 8; bits; clrBit(j,bits))
      sc_search_qrb(rook, rookMtrl, @R0, @R1, @R2, @R3, @R4, @QUEEN)

          @QUEEN  //    case queenMtrl:
              rlwinm.rTmp1,
      ap, 4, 20,
      27  //       for (bits = (a & rMask) >> 8; bits; clrBit(j,bits))
      sc_search_qrb(queen, queenMtrl, @Q0, @Q1, @Q2, @Q3, @Q4, @RETURN)

          @RETURN  // }
              frame_end5 blr

#undef dest
#undef ap
#undef capMtrl
#undef maxMtrl
#undef Data

} /* SearchSafeCaptures1 */

/*
asm INT SmattMtrl (ATTACK a)          // Entry: rTmp1 = attack word. Exit: rTmp2
= material {                                     // value of smallest attacker.
   andis.  rTmp3, rTmp1,0x0600
   li      rTmp2, pawnMtrl
   bnelr
@bn
   andis.  rTmp4, rTmp1,0x00FF
   li      rTmp2, knightMtrl
   bnelr
   andi.   rTmp5, rTmp1,0x0F00
   bnelr
@r
   andi.   rTmp3, rTmp1,0xF000
   li      rTmp2, rookMtrl
   bnelr
@q
   li      rTmp2, queenMtrl
   blr
} /* SmattMtrl */

/**************************************************************************************************/
/*                                                                                                */
/*                                         [E] ESCAPE MOVES */
/*                                                                                                */
/**************************************************************************************************/

asm void SearchNonCaptures1(register SQUARE _sq1);

// Searches non-capture moves by the piece on N->escapeSq (which must not be a
// nullSq). NOTE: Changes m.dPly, which should be reset afterwards to 1.

asm void SearchEscapes(void) {
  lha rTmp1,
      node(escapeSq)  // If (N->escapeSq == nullSq) return;
      li rTmp9,
      gen_E  // N->gen = gen_E;
          li rTmp2,
      0 cmpi cr0, 0, rTmp1, nullSq beqlr + lha rTmp3,
      node(eply)  // m.dply = N->eply;
      sth rTmp9,
      node(gen) stw rTmp2,
      move(cap)  // m.cap = empty; m.type = mtype_Normal;
      sth rTmp3,
      move(dply) b SearchNonCaptures1  // SearchNonCaptures1(N->escapeSq);
} /* SearchEscapes */

/**************************************************************************************************/
/*                                                                                                */
/*                                         [F] KILLER MOVES */
/*                                                                                                */
/**************************************************************************************************/

// Searches the 2 killer moves if they are active and pseudo-legal.
// Only called at normal full width nodes (i.e. not quiescence or check):

asm void SearchKiller(register MOVE *_killer, register INT _gen);

asm void SearchKillers(void) {
  @1 lhz rTmp3,
      node(killer1Active)  // if (killer1Active)
      addi rTmp1,
      rNode,
      NODE.killer1  //    SearchKiller(&killer1, gen_F1);
          cmpi cr5,
      0, rTmp3, 0 beq - cr5, @2 frame_begin0 li rTmp2,
      gen_F1 bl SearchKiller frame_end0 @2 lhz rTmp3,
      node(killer2Active)  // if (killer2Active)
      addi rTmp1,
      rNode,
      NODE.killer2  //    SearchKiller(&killer2, gen_F2);
          cmpi cr6,
      0, rTmp3, 0 beqlr - cr6 li rTmp2,
      gen_F2 b SearchKiller  //<-- Skip this call if linker places
                             //"SearchKiller" routine next!!!
} /* SearchKillers */

asm void SearchKiller(register MOVE *_killer, register INT _gen) {
  lhz rTmp3,
      MOVE.from(_killer)  // if (killer.piece != Board[killer.from]) return;
      lhz rTmp4,
      MOVE.to(_killer) lhz rTmp5, MOVE.piece(_killer) lhz rTmp6,
      MOVE.cap(_killer) add rTmp9, rTmp3,
      rTmp3  // if (killer.cap != Board[killer.to]) return;
          add rTmp10,
      rTmp4, rTmp4 lhzx rTmp7, rBoard, rTmp9 lhzx rTmp8, rBoard,
      rTmp10 sth rTmp2,
      node(gen)  // N->gen = _gen;
      cmp cr5,
      0, rTmp5, rTmp7 cmp cr6, 0, rTmp6,
      rTmp8 bnelr - cr5 bnelr -
          cr6

              lhz rTmp7,
      MOVE.type(_killer)  // if (killer.type == normal)
      lha rTmp8,
      MOVE.dir(_killer) andi.rTmp9, rTmp5, 0x07 cmpi cr5, 0, rTmp7,
      mtype_Normal  // {

              bne -
          cr5,
      @SEARCH  //    switch (pieceType(killer.piece))
          cmpi cr6,
      0, rTmp9,
      knight  //    {
              beq -
          cr6,
      @SEARCH  //       case knight : break;
              bgt -
          cr6,
      @KING @PAWN add rTmp9, rPawnDir,
      rTmp3  //       case pawn :
          subf rTmp10,
      rPawnDir,
      rTmp4  //          if (killer.to != killer.from + 2*pawnDir) break;
          cmp cr5,
      0, rTmp9, rTmp10 add rTmp2, rTmp9, rTmp9 bne + cr5,
      @SEARCH  //          if (Board[killer.from + pawnDir] != empty) return;
          lhzx rTmp9,
      rBoard, rTmp2 cmpi cr6, 0, rTmp9,
      0  //          break;
          beq +
          cr6,
      @SEARCH blr @KING cmpi cr5, 0, rTmp9,
      king  //       case king :
          add rTmp9,
      rTmp3, rTmp8 bne + cr5,
      @2  //          if (Attack_[killer.to]) return;
      rlwinm.rTmp2,
      rTmp4, 2, 0, 29 lwzx rTmp10, rAttack_, rTmp2 cmpi cr6, 0, rTmp10,
      0 beq + cr6,
      @SEARCH           //          break;
          blr @DEFAULT  //       case queen : case rook : case bishop :
              lhzx rTmp2,
      rBoard,
      rTmp10  //          dir = killer.dir;
          add rTmp9,
      rTmp9, rTmp8 cmpi cr6, 0, rTmp2,
      empty  //          for (sq = killer.from + dir; sq != killer.to; sq +=
             //          dir)
                 bnelr -
          cr6 @2 cmp cr5,
      0, rTmp9,
      rTmp4  //             if (Board[sq] != empty) return;
          add rTmp10,
      rTmp9,
      rTmp9  //    }
              bne +
          cr5,
      @DEFAULT  // }

      @SEARCH sth rTmp5,
      move(piece)  // m = killer;
      sth rTmp3,
      move(from) sth rTmp4, move(to) sth rTmp6, move(cap) sth rTmp7,
      move(type) sth rTmp8, move(dir) b ProcessMove  // ProcessMove();
} /* SearchKiller */

/**************************************************************************************************/
/*                                                                                                */
/*                                      [G] CASTLING MOVES */
/*                                                                                                */
/**************************************************************************************************/

// Searches castling moves. May not be called if the player is in check.
// NOTE: Assumes Big Endian

asm void SearchCastling(void) {
  cmpi cr0, 0, rPlayer,
      white  // if (player == white)
          bne @BLACK

      /* - - - - - - - - - - - - - - - - - - White Castling Moves - - - - - - -
         - - - - - - - - - - - -*/

      @WHITE  // {
          lhz rTmp1,
      (2 * e1)(rBoard)  //    if (Board[e1] != wKing || HasMovedTo[e1])
      lhz rTmp2,
      (2 * (e1 + boardSize2))(rBoard)cmpi cr5, 0, rTmp1, wKing cmpi cr6, 0,
      rTmp2,
      0  //       return;
          bnelr +
          cr5 bnelr + cr6 @WO_O lhz rTmp3,
      (2 * h1)(rBoard)  //    if (Board[h1] == wRook &&
      lwz rTmp4,
      (2 * f1)(rBoard)li rTmp9, gen_G sth rTmp9, node(gen) cmpi cr5, 0, rTmp3,
      wRook  //        Board[f1] == empty &&
          cmpi cr6,
      0, rTmp4, 0 bne + cr5,
      @WO_O_O  //        Board[g1] == empty &&
              bne +
          cr6,
      @WO_O_O lhz rTmp5,
      (2 * (h1 + boardSize2))(rBoard)  //    ! HasMovedTo[h1] &&
      lwz rTmp6,
      (4 * f1)(rAttack_)lwz rTmp7,
      (4 * g1)(rAttack_)  //        ! AttackB[f1] &&
      cmpi cr5,
      0, rTmp5, 0 or.rTmp6, rTmp6,
      rTmp7  //        ! AttackB[g1])
              bne +
          cr5,
      @WO_O_O bne + @WO_O_O  //    {
                        addis rTmp1,
      r0,
      wKing  //        m.piece = wKing;
          mflr r0 ori rTmp1,
      rTmp1,
      e1  //        m.from  = e1;
          stwu r0,
      -4(SP)addis rTmp2, r0,
      g1  //        m.to    = g1;
          stw rTmp1,
      move(piece)  //        m.cap   = empty;
      li rTmp3,
      mtype_O_O  //        m.type  = mtype_O_O;
          stw rTmp2,
      move(to)  //        N->gen   = gen_G;
      sth rTmp3,
      move(type)      //        ProcessMove();
      bl ProcessMove  //    }
          lwz r0,
      0(SP)addi SP, SP, 4 mtlr r0 @WO_O_O lwz rTmp3,
      (2 * a1)(rBoard)  //    if (Board[a1] == wRook && Board[b1] == empty &&
      lwz rTmp4,
      (2 * c1)(rBoard)xoris rTmp3, rTmp3,
      wRook  //        Board[c1] == empty &&
          or.rTmp4,
      rTmp3,
      rTmp4 bnelr +  //        Board[d1] == empty &&
          lhz rTmp5,
      (2 * (a1 + boardSize2))(rBoard)  //    ! HasMovedTo[a1] &&
      lwz rTmp6,
      (4 * c1)(rAttack_)lwz rTmp7,
      (4 * d1)(rAttack_)  //        ! AttackB[c1] &&
      cmpi cr5,
      0, rTmp5, 0 or.rTmp6, rTmp6,
      rTmp7  //        ! AttackB[d1])
              bnelr +
          cr5 bnelr +  //    {
          addis rTmp1,
      r0,
      wKing  //        m.piece = wKing;
          ori rTmp1,
      rTmp1,
      e1  //        m.from  = e1;
          addis rTmp2,
      r0,
      c1  //        m.to    = c1;
          stw rTmp1,
      move(piece)  //        m.cap   = empty;
      li rTmp3,
      mtype_O_O_O  //        m.type  = mtype_O_O_O;
          stw rTmp2,
      move(to)  //        m.gen   = gen_G;
      sth rTmp3,
      move(type)     //        ProcessMove();
      b ProcessMove  //    }
                     // }

      /* - - - - - - - - - - - - - - - - - - Black Castling Moves - - - - - - -
         - - - - - - - - - - - -*/

      // else
      @BLACK  // {
          lhz rTmp1,
      (2 * e8)(rBoard)  //    if (Board[e8] != bKing || HasMovedTo[e8])
      lhz rTmp2,
      (2 * (e8 + boardSize2))(rBoard)cmpi cr5, 0, rTmp1, bKing cmpi cr6, 0,
      rTmp2,
      0  //       return;
          bnelr +
          cr5 bnelr + cr6 @BO_O lhz rTmp3,
      (2 * h8)(rBoard)  //    if (Board[h8] == bRook &&
      lwz rTmp4,
      (2 * f8)(rBoard)li rTmp9, gen_G sth rTmp9, node(gen) cmpi cr5, 0, rTmp3,
      bRook  //        Board[f8] == empty &&
          cmpi cr6,
      0, rTmp4, 0 bne + cr5,
      @BO_O_O  //        Board[g8] == empty &&
              bne +
          cr6,
      @BO_O_O lhz rTmp5,
      (2 * (h8 + boardSize2))(rBoard)  //    ! HasMovedTo[h8] &&
      lwz rTmp6,
      (4 * f8)(rAttack_)lwz rTmp7,
      (4 * g8)(rAttack_)  //        ! AttackB[f8] &&
      cmpi cr5,
      0, rTmp5, 0 or.rTmp6, rTmp6,
      rTmp7  //        ! AttackB[g8])
              bne +
          cr5,
      @BO_O_O bne + @BO_O_O  //    {
                        addis rTmp1,
      r0,
      bKing  //        m.piece = bKing;
          mflr r0 ori rTmp1,
      rTmp1,
      e8  //        m.from  = e8;
          stwu r0,
      -4(SP)addis rTmp2, r0,
      g8  //        m.to    = g8;
          stw rTmp1,
      move(piece)  //        m.cap   = empty;
      li rTmp3,
      mtype_O_O  //        m.type  = mtype_O_O;
          stw rTmp2,
      move(to)  //        N->gen  = gen_G;
      sth rTmp3,
      move(type)      //        ProcessMove();
      bl ProcessMove  //    }
          lwz r0,
      0(SP)addi SP, SP, 4 mtlr r0 @BO_O_O lwz rTmp3,
      (2 * a8)(rBoard)  //    if (Board[a8] == bRook && Board[b8] == empty &&
      lwz rTmp4,
      (2 * c8)(rBoard)xoris rTmp3, rTmp3,
      bRook  //        Board[c8] == empty &&
          or.rTmp4,
      rTmp3,
      rTmp4 bnelr +  //        Board[d8] == empty &&
          lhz rTmp5,
      (2 * (a8 + boardSize2))(rBoard)  //    ! HasMovedTo[a8] &&
      lwz rTmp6,
      (4 * c8)(rAttack_)lwz rTmp7,
      (4 * d8)(rAttack_)  //        ! AttackB[c8] &&
      cmpi cr5,
      0, rTmp5, 0 or.rTmp6, rTmp6,
      rTmp7  //        ! AttackB[d8])
              bnelr +
          cr5 bnelr +  //    {
          addis rTmp1,
      r0,
      bKing  //        m.piece = bKing;
          ori rTmp1,
      rTmp1,
      e8  //        m.from  = e8;
          addis rTmp2,
      r0,
      c8  //        m.to    = c8;
          stw rTmp1,
      move(piece)  //        m.cap   = empty;
      li rTmp3,
      mtype_O_O_O  //        m.type  = mtype_O_O_O;
          stw rTmp2,
      move(to)  //        m.gen   = gen_G;
      sth rTmp3,
      move(type)     //        ProcessMove();
      b ProcessMove  //    }
                     // }
} /* SearchCastling */

/**************************************************************************************************/
/*                                                                                                */
/*                                         [H] NON CAPTURES */
/*                                                                                                */
/**************************************************************************************************/

asm void SearchNonCaptures1(register SQUARE _sq1);
asm void SearchKing(register SQUARE _sq1, register SQUARE _sq2);
asm void SearchQueen(register SQUARE _sq1, register SQUARE _sq2);
asm void SearchRook(register SQUARE _sq1, register SQUARE _sq2);
asm void SearchBishop(register SQUARE _sq1, register SQUARE _sq2);
asm void SearchKnight(register SQUARE _sq1, register SQUARE _sq2);
asm void SearchPawn(register SQUARE _sq1, register SQUARE _sq2);

/*-------------------------------- Search All Normal Non-captures
 * --------------------------------*/
// Searches all normal, non-captures in the following order: Attacked pieces
// (from "ALoc[]"), safe pieces (from "SLoc[]") and finally the king.

asm void SearchNonCaptures(void) {
#define Loc rLocal1

  frame_begin1

      li rTmp9,
      gen_H  // N->gen = gen_G;
          li rTmp1,
      0  // m.type = mtype_Normal; m.cap = empty;
      addi Loc,
      rNode, (NODE.ALoc - 2) sth rTmp9, node(gen) stw rTmp1,
      move(cap) b @2            // for (i = 0; onBoard(ALoc[i]); i++)
      @1 bl SearchNonCaptures1  //    SearchNonCaptures1(ALoc[i]);
      @2 lhau rTmp1,
      2(Loc)cmpi cr5, 0, rTmp1, 0 bge + cr5,
      @1

      addi Loc,
      rNode,
      (NODE.SLoc - 2)                // for (i = 0; onBoard(SLoc[i]); i++)
      b @4 @3 bl SearchNonCaptures1  //    SearchNonCaptures1(SLoc[i]);
      @4 lhau rTmp1,
      2(Loc)cmpi cr5, 0, rTmp1, 0 bge + cr5,
      @3

      lhz rTmp1,
      0(rPieceLoc)  // SearchNonCaptures1(kingLoc(player));

      frame_end1

#undef Loc

          sth rTmp1,
      move(from)  // m.from = sq;
      addi rTmp3,
      rPlayer,
      king  // m.piece = friendly(king);
          add rTmp2,
      rTmp1, rTmp1 sth rTmp3, move(piece) b SearchKing  // SearchKing(sq, 2*sq);
} /* SearchNonCaptures */

/*----------------------------- Search Normal Non-captures for 1 Piece
 * ---------------------------*/
// Searches normal, non-captures of the piece on rTmp1. It is assumed that m.cap
// = empty and m.type = mtype_Normal.

asm void SearchNonCaptures1(register SQUARE _sq1) {
  add rTmp2, rTmp1,
      rTmp1  // m.from = sq;
          sth rTmp1,
      move(from) lhax rTmp3, rBoard,
      rTmp2  // m.piece = Board[sq];
          sth rTmp3,
      move(piece) andi.rTmp4, rTmp3,
      0x07  // switch (pieceType(m.piece))
      cmpi cr5,
      0, rTmp4, bishop bgt cr5,
      @1  // {
      beq cr5,
      SearchBishop  //    case pawn   : SearchPawn();   break;
          cmpi cr6,
      0, rTmp4,
      knight  //    case knight : SearchKnight(); break;
          beq cr6,
      SearchKnight      //    case bishop : SearchBishop(); break;
          b SearchPawn  //    case rook   : SearchRook();   break;
      @1 cmpi cr6,
      0, rTmp4,
      queen  //    case queen  : SearchQueen();  break;
          blt cr6,
      SearchRook  //    case king   : SearchKing();
          beq cr6,
      SearchQueen b SearchKing  // }
} /* SearchNonCaptures1 */

/*------------------------------------------ King Moves
 * ------------------------------------------*/
// Searches non capturing moves for the king on "From". On entry: rTmp1 =
// kingSq, rTmp2 = 2*kingSq, and all but the "to"-field of "N->m" must already
// be set.

asm void SearchKing(register SQUARE _sq1, register SQUARE _sq2) {
#define nc_search_k(dir, LNext)                                            \
  lhz rTmp1, (dir * 2)(B);  /* if (isEmpty(m.from + KingDir[i]) &&      */ \
  lwz rTmp2, (dir * 4)(Ao); /*     ! AttackO[m.from + kingDir[i]])      */ \
  addi rTmp4, src, dir;     /*    m.to = m.from + m.dir,                */ \
  or.rTmp3, rTmp1, rTmp2;                                                  \
  bne + LNext;                                                             \
  sth rTmp4, move(to); /*    ProcessMove();                        */      \
  bl ProcessMove;                                                          \
  LNext

#define B rLocal1
#define Ao rLocal2
#define src rLocal3

  frame_begin3

      add rTmp4,
      rTmp2, rTmp2 add B, rBoard, rTmp2 add Ao, rAttack_, rTmp4 mr src,
      rTmp1

          nc_search_k(-0x0F, @Einstein) nc_search_k(-0x11, @Raouline)
              nc_search_k(+0x11, @Raoul) nc_search_k(+0x0F, @Peter)
                  nc_search_k(-0x10, @Niels) nc_search_k(+0x10, @Eva)
                      nc_search_k(+0x01, @Anja) nc_search_k(-0x01, @Ole)

                          frame_end3 blr

#undef B
#undef Ao
#undef src
} /* SearchKing */

/*----------------------------------- Queen/Rook/Bishop Moves
 * ------------------------------------*/
// Generates non capturing moves for the queen/rook/bishop on "sq". On entry:
// rTmp1 = sq, rTmp2 = 2*sq, and all but the "to"- and "dir"-fields of "N->m"
// must have been set.

#define Bsrc rLocal1
#define Bdst rLocal2
#define src rLocal3
#define dst rLocal4
#define aFrom rLocal5
#define smattMask rLocal6

#define nc_search_qrb(xdir, xraybit, xpiecebit, L0, L1, L2, L3, L4)        \
  lha rTmp1, (2 * xdir)(Bsrc); /* if (Board[m.from + dir] != empty)  */    \
  li rTmp2, xdir;              /*    continue;                       */    \
  cmpi cr5, 0, rTmp1, empty;                                               \
  addi dst, src, xdir; /* dst = src + dir;                   */            \
  bne cr5, L4;                                                             \
  sth rTmp2, move(dir);           /* m.dir = dir;                       */ \
  L0 rlwinm rTmp4, dst, 2, 0, 29; /* while (Board[dst] == empty)        */ \
  sth dst, move(to);              /* {                                  */ \
  lwzx rTmp5, rAttack_, rTmp4;    /*    d = AttackO[dst];               */ \
  addi dst, dst, xdir;            /*    m.to = dst;                     */ \
  add rTmp2, dst, dst;                                                     \
  and.rTmp6, rTmp5, smattMask; /*    if ((d & smattMask) ||          */    \
  lhax Bdst, rBoard, rTmp2;                                                \
  bne - L1;                                                                \
  cmpi cr5, 0, rTmp5, 0;      /*        (d &&                       */     \
  andi.rTmp7, aFrom, xraybit; /*         ! (aFrom & xraybit) &&     */     \
  beq + cr5, L2;                                                           \
  bne - L2; /*         bitcount(AttackP[dst]) < 1 */                       \
  lwzx rTmp2, rAttack, rTmp4;                                              \
  li rTmp3, 0;                                                             \
  ori rTmp3, rTmp3, xpiecebit;                                             \
  cmp cr5, 0, rTmp2, rTmp3; /*        )                           */       \
  bne + cr5, L2;            /*       )                            */       \
  L1 bl AddSacrifice;       /*       AddSacrifice();              */       \
  b L3;                     /*    else                            */       \
  L2 bl ProcessMove;        /*       ProcessMove();               */       \
  L3 cmpi cr6, 0, Bdst, empty;                                             \
  beq + cr6, L0; /* }                                  */                  \
  L4

asm void SearchQueen(register SQUARE _sq1, register SQUARE _sq2) {
  frame_begin6

      add rTmp4,
      rTmp2,
      rTmp2  // aFrom = AttackP[src];
          add Bsrc,
      rBoard, rTmp2 lwzx aFrom, rAttack, rTmp4 addis smattMask, r0,
      0x06FF mr src, rTmp1 ori smattMask, smattMask,
      0xFF00

      nc_search_qrb(-0x0F, 0x0101, 0x0001, @A1, @B1, @C1, @D1, @E1)
          nc_search_qrb(-0x11, 0x0202, 0x0002, @A2, @B2, @C2, @D2, @E2)
              nc_search_qrb(+0x11, 0x0404, 0x0004, @A3, @B3, @C3, @D3, @E3)
                  nc_search_qrb(+0x0F, 0x0808, 0x0008, @A4, @B4, @C4, @D4, @E4)
                      nc_search_qrb(-0x10, 0x1010, 0x0010, @A5, @B5, @C5, @D5,
                                    @E5) nc_search_qrb(+0x10, 0x2020, 0x0020,
                                                       @A6, @B6, @C6, @D6, @E6)
                          nc_search_qrb(+0x01, 0x4040, 0x0040, @A7, @B7, @C7,
                                        @D7, @E7)
                              nc_search_qrb(-0x01, 0x8080, 0x0080, @A8, @B8,
                                            @C8, @D8, @E8)

                                  frame_end6 blr
} /* SearchQueen */

asm void SearchRook(register SQUARE _sq1, register SQUARE _sq2) {
  frame_begin6

      add rTmp4,
      rTmp2,
      rTmp2  // aFrom = AttackP[src];
          add Bsrc,
      rBoard, rTmp2 lwzx aFrom, rAttack, rTmp4 addis smattMask, r0,
      0x06FF mr src, rTmp1 ori smattMask, smattMask,
      0x0F00

      nc_search_qrb(-0x10, 0x1010, 0x1000, @A5, @B5, @C5, @D5, @E5)
          nc_search_qrb(+0x10, 0x2020, 0x2000, @A6, @B6, @C6, @D6, @E6)
              nc_search_qrb(+0x01, 0x4040, 0x4000, @A7, @B7, @C7, @D7, @E7)
                  nc_search_qrb(-0x01, 0x8080, 0x8000, @A8, @B8, @C8, @D8, @E8)

                      frame_end6 blr
} /* SearchRook */

asm void SearchBishop(register SQUARE _sq1, register SQUARE _sq2) {
  frame_begin6

      add rTmp4,
      rTmp2,
      rTmp2  // aFrom = AttackP[src];
          add Bsrc,
      rBoard, rTmp2 lwzx aFrom, rAttack, rTmp4 addis smattMask, r0,
      0x0600 mr src,
      rTmp1

          nc_search_qrb(-0x0F, 0x0101, 0x0100, @A1, @B1, @C1, @D1, @E1)
              nc_search_qrb(-0x11, 0x0202, 0x0200, @A2, @B2, @C2, @D2, @E2)
                  nc_search_qrb(+0x11, 0x0404, 0x0400, @A3, @B3, @C3, @D3, @E3)
                      nc_search_qrb(+0x0F, 0x0808, 0x0800, @A4, @B4, @C4, @D4,
                                    @E4)

                          frame_end6 blr
} /* SearchBishop */

#undef src
#undef dst
#undef aFrom
#undef xRay
#undef Bdst
#undef smattMask

/*------------------------------------------- Knight Moves
 * ---------------------------------------*/
// Generates non capturing moves for the knight on "sq". On entry: rTmp1 = sq,
// rTmp2 = 2*sq, and all but the "to"-field of "N->m" must be set.

asm void SearchKnight(register SQUARE _sq1, register SQUARE _sq2) {
#define nc_knight(dir, Nbit, L0, L1, L2)                                    \
  lha rTmp1, (dir * 2)(B);   /* if (isEmpty(m.from + KnightDir[i]))      */ \
  addi rTmp2, src, dir;      /* {                                        */ \
  lwz rTmp3, (dir * 4)(Ao);  /*    d = AttackO[m.to];                    */ \
  cmpi cr0, 0, rTmp1, empty; /*    m.to = m.from + KnightDir[i];         */ \
  bne + cr0, L2;                                                            \
  sth rTmp2, move(to);                                                      \
  cmpi cr5, 0, rTmp3, 0; /*    if (d &&                              */     \
  andis.rTmp4, rTmp3, 0x0600;                                               \
  beq cr5, L1; /*        (d & pMask ||                     */               \
  bne L0;                                                                   \
  lwz rTmp5, (dir * 4)(Ap); /*         AttackP[m.to] == KnightBit[j]))  */  \
  addis rTmp6, r0, Nbit;                                                    \
  cmp cr0, 0, rTmp5, rTmp6; /*       AddSacrifice();                    */  \
  bne + L1;                 /*    else                                  */  \
  L0 bl AddSacrifice;       /*       ProcessMove();                     */  \
  b L2;                                                                     \
  L1 bl ProcessMove; /* }                                        */         \
  L2

#define B rLocal1
#define Ap rLocal2
#define Ao rLocal3
#define src rLocal4

  frame_begin4

      add rTmp4,
      rTmp2, rTmp2 add B, rBoard, rTmp2 add Ap, rAttack, rTmp4 add Ao, rAttack_,
      rTmp4 mr src,
      rTmp1

          nc_knight(-0x0E, 0x0001, @sacri0, @search0, @next0) nc_knight(
              -0x12, 0x0002, @sacri1, @search1, @next1)
              nc_knight(-0x1F, 0x0004, @sacri2, @search2, @next2) nc_knight(
                  -0x21, 0x0008, @sacri3, @search3, @next3)
                  nc_knight(+0x12, 0x0010, @sacri4, @search4, @next4) nc_knight(
                      +0x0E, 0x0020, @sacri5, @search5, @next5)
                      nc_knight(+0x21, 0x0040, @sacri6, @search6, @next6)
                          nc_knight(+0x1F, 0x0080, @sacri7, @search7, @next7)

                              frame_end4 blr

#undef B
#undef Ap
#undef Ao
#undef src
} /* SearchKnight */

/*------------------------------------------ Pawn Moves
 * ------------------------------------------*/

asm void SearchPawn(register SQUARE _sq1, register SQUARE _sq2) {
#define xRay rLocal1
#define mfrom4 rLocal2

  cmpi cr0, 0, rPlayer,
      white  // if (player == white)
          add rTmp3,
      rBoard,
      rTmp2 bne @BLACK

      /* - - - - - - - - - - - - - - - - - - - - White Pawn Moves - - - - - - -
         - - - - - - - - - - - - */

      @WHITE  // {
          lha rTmp4,
      0x20(rTmp3)  //    if (isOccupied(front(m.from))) return;
      cmpi cr5,
      0, rTmp1,
      0x60  //    if (onRank7(m.from)) return;
      cmpi cr6,
      0, rTmp4,
      empty bgelr + cr5 bnelr -
          cr6

              frame_begin2

                  add mfrom4,
      rTmp2, rTmp2 lwzx xRay, rAttack,
      mfrom4  //    xRay = Attack[m.from] & wForwardMask;
          cmpi cr6,
      0, rTmp1,
      0x20

          @wpawn2  //    if (rank(m.from) == Rank2[player] &&
              bge +
          cr6,
      @wpawn1 lhz rTmp4,
      0x40(rTmp3)  //        isEmpty(m.from + 0x20))
      addi rTmp5,
      mfrom4, 0x80 cmpi cr5, 0, rTmp4,
      empty  //    {
          addi rTmp6,
      rTmp1, 0x20 bne - cr5,
      @wpawn1  //        m.to = m.from + 0x20;
          sth rTmp6,
      move(to) lwzx rTmp3, rAttack_, rTmp5 andi.rTmp7, xRay,
      wForwardMask bne - @wprocess2  //        if (! xRay &&
                             cmpi cr5,
      0, rTmp3,
      0  //            Attack_[m.to] &&
      lwzx rTmp4,
      rAttack,
      rTmp5  //            ! Attack[m.to])
          beq cr5,
      @wprocess2 cmpi cr6, 0, rTmp4, 0 bne cr6,
      @wprocess2                     //           AddSacrifice();
          bl AddSacrifice b @wdone2  //        else
      @wprocess2 bl ProcessMove      //           ProcessMove();
      @wdone2 srawi rTmp1,
      mfrom4,
      2
      //    }
      @wpawn1 addi rTmp6,
      rTmp1,
      0x10  //    m.to = m.from + 0x10
      addi rTmp5,
      mfrom4, 0x40 andi.rTmp7, xRay, wForwardMask lwzx rTmp3, rAttack_,
      rTmp5 sth rTmp6,
      move(to) bne - @wprocess1  //    if (! xRay &&
                         cmpi cr5,
      0, rTmp3,
      0  //        Attack_[m.to] &&
      lwzx rTmp4,
      rAttack,
      rTmp5  //        ! Attack[m.to])
          beq cr5,
      @wprocess1 cmpi cr6, 0, rTmp4, 0 bne cr6,
      @wprocess1                    //       AddSacrifice();
          bl AddSacrifice b @wdone  //    else
      @wprocess1                    //    {
          cmpi cr5,
      0, mfrom4,
      (0x40 * 4)  //       if (rank(m.from) >= Rank5[player])
          blt +
          cr5,
      @wpr1  //          m.dply = 0;
          li rTmp1,
      0 sth rTmp1,
      move(dply) @wpr1  //       ProcessMove();
          bl ProcessMove li rTmp1,
      2  //       m.dply = 2;      // <-- Important to restore
      sth rTmp1,
      move(dply)  //    }

      @wdone frame_end2 blr  // }
                             // else

      /* - - - - - - - - - - - - - - - - - - - - Black Pawn Moves - - - - - - -
         - - - - - - - - - - - - */

      @BLACK  // {
          lha rTmp4,
      -0x20(rTmp3)  //    if (isOccupied(front(m.from))) return;
      cmpi cr5,
      0, rTmp1,
      0x20  //    if (onRank7(m.from)) return;
      cmpi cr6,
      0, rTmp4,
      empty bltlr + cr5 bnelr -
          cr6

              frame_begin2

                  add mfrom4,
      rTmp2, rTmp2 lwzx xRay, rAttack,
      mfrom4  //    xRay = Attack[m.from] & bForwardMask;
          cmpi cr6,
      0, rTmp1,
      0x60

          @bpawn2  //    if (rank(m.from) == Rank2[player] &&
              blt +
          cr6,
      @bpawn1 lhz rTmp4,
      -0x40(rTmp3)  //        isEmpty(m.from - 0x20))
      addi rTmp5,
      mfrom4, -0x80 cmpi cr5, 0, rTmp4,
      empty  //    {
          addi rTmp6,
      rTmp1, -0x20 bne - cr5,
      @bpawn1  //        m.to = m.from - 0x20;
          sth rTmp6,
      move(to) lwzx rTmp3, rAttack_, rTmp5 andi.rTmp7, xRay,
      bForwardMask bne - @bprocess2  //        if (! xRay &&
                             cmpi cr5,
      0, rTmp3,
      0  //            Attack_[m.to] &&
      lwzx rTmp4,
      rAttack,
      rTmp5  //            ! Attack[m.to])
          beq cr5,
      @bprocess2 cmpi cr6, 0, rTmp4, 0 bne cr6,
      @bprocess2                     //           AddSacrifice();
          bl AddSacrifice b @bdone2  //        else
      @bprocess2 bl ProcessMove      //           ProcessMove();
      @bdone2 srawi rTmp1,
      mfrom4,
      2
      //    }
      @bpawn1 addi rTmp6,
      rTmp1,
      -0x10  //    m.to = m.from - 0x10
      addi rTmp5,
      mfrom4, -0x40 andi.rTmp7, xRay, wForwardMask lwzx rTmp3, rAttack_,
      rTmp5 sth rTmp6,
      move(to) bne - @bprocess1  //    if (! xRay &&
                         cmpi cr5,
      0, rTmp3,
      0  //        Attack_[m.to] &&
      lwzx rTmp4,
      rAttack,
      rTmp5  //        ! Attack[m.to])
          beq cr5,
      @bprocess1 cmpi cr6, 0, rTmp4, 0 bne cr6,
      @bprocess1                    //       AddSacrifice();
          bl AddSacrifice b @bdone  //    else
      @bprocess1                    //    {
          cmpi cr5,
      0, mfrom4,
      (0x40 * 4)  //       if (rank(m.from) >= Rank5[player])
          bge +
          cr5,
      @bpr1  //          m.dply = 0;
          li rTmp1,
      0 sth rTmp1,
      move(dply) @bpr1  //       ProcessMove();
          bl ProcessMove li rTmp1,
      2  //       m.dply = 2;      // <-- Important to restore
      sth rTmp1,
      move(dply)  //    }

      @bdone frame_end2 blr  // }

#undef xRay
#undef mfrom4
} /* SearchPawn */

/**************************************************************************************************/
/*                                                                                                */
/*                                          [I] SACRIFICES */
/*                                                                                                */
/**************************************************************************************************/

/*---------------------------------- Search All Sacrifice Moves
 * ----------------------------------*/
// Searches all moves of "SBuf" pertaining to the current node. N->m.dply must
// already have been set appropriately (it will not be changed).

asm void SearchSacrifices(void) {
#define sm rLocal1

   frame_begin1

   li      rTmp9, gen_I                     // N->gen = gen_I;
   lwz     sm, node(bufStart)
   sth     rTmp9, node(gen)                 // for (sm = bufStart; sm < bufTop; sm++)
   b       @rof                             // {
@for
   lwz     rTmp1, 0(sm)                     //    N->m.piece = sm->piece;
   lwz     rTmp2, 4(sm)                     //    N->m.from  = sm->from;
   lwz     rTmp3, 8(sm)                     //    N->m.to    = sm->to;
   addi    sm, sm,sizeof(MOVE)              //    N->m.cap   = sm->cap;
   stw     rTmp1, move(piece)               //    N->m.type  = sm->type;
   stw     rTmp2, move(to)                  //    N->m.dir   = sm->dir;
   stw     rTmp3, move(type)
   bl      ProcessMove                      //    ProcessMove();
@rof
   lwz     rTmp4, ENGINE.S.bufTop(rEngine)
   cmp     cr0,0,sm,rTmp4
   blt+    @for                             // }

   frame_end1
   blr

#undef sm
} /* SearchSacrifices */

/*--------------------------------- Add Move to Sacrifice Buffer
 * ---------------------------------*/
// Adds the move "N->m" to "SBuf". ### SHOULD CHECK IF SACRIFICE BUFFER FULL. IF
// SO -> SEARCH MOVE DIRECTLY ANYWAY...

static asm void AddSacrifice(void) {
  lhz rTmp1,
      node(storeSacri)  // if (N->storeSacri)
      lwz rTmp2,
      ENGINE.S.bufTop(rEngine) cmpi cr0, 0, rTmp1,
      0  //    SBuf[topS++] = N->m;
      beqlr lwz rTmp3,
      move(piece) lwz rTmp4, move(to) lwz rTmp5, move(type) addi rTmp6, rTmp2,
      sizeof(MOVE) stw rTmp3, 0(rTmp2)stw rTmp4, 4(rTmp2)stw rTmp5,
      8(rTmp2)stw rTmp6, ENGINE.S.bufTop(rEngine) blr
} /* AddSacrifice */

/**************************************************************************************************/
/*                                                                                                */
/*                                          [J] SAFE CHECKS */
/*                                                                                                */
/**************************************************************************************************/

asm void SearchCheckQRB(void);
asm void SearchCheckN(void);
asm void SearchCheckP(void);

/*---------------------------------------- Search Safe Checks
 * ------------------------------------*/
// Searches safe non-capturing checks (both direct and indirect check). Pawns to
// 7th or 8th rank are not searched.

asm void SearchSafeChecks(void) {
#define ksq rLocal1   // Location of enemy king
#define Aksq rLocal2  // &AttackP[ksq]
#define Bksq rLocal3  // &Board[ksq]

  frame_begin3

      lhz ksq,
      0(rPieceLoc_)  // ksq = kingLoc(opponent);
      li rTmp1,
      0  // m.cap = empty; m.type = normal;
      li rTmp9,
      gen_J  // N->gen = gen_J;
          stw rTmp1,
      move(cap) add rTmp2, ksq,
      ksq  // Aksq = &AttackP[ksq];
          sth rTmp9,
      node(gen) add rTmp3, rTmp2,
      rTmp2  // Bksq = &Board[ksq];
          add Bksq,
      rBoard, rTmp2 add Aksq, rAttack,
      rTmp3

          bl SearchCheckQRB        // SearchCheckQRB();
              bl SearchCheckN      // SearchCheckN();
                  bl SearchCheckP  // SearchCheckP();

                      frame_end3 blr

#undef ksq
#undef Aksq
#undef Bksq
} /* SearchSafeChecks */

/*---------------------- Search Queen/Rook/Bishop Checks (direct and indirect)
 * -------------------*/

asm void SearchCheckQRB1(register ATTACK _ato, register ATTACK _bits,
                         register SQUARE _to4);
asm void SearchIndCheck(register PIECE _piece, register SQUARE _from2,
                        register INT _idir);

// For each of the 8 directions emanating from the opponent king, we check if
// the squares are attacked by sliding pieces. If so we generate direct QRB
// checks. If the direction emanating from the king ends on a square occupied by
// a friendly piece, we check for indirect/discovered checks.
//
// On entry: rLocal1, rLocal2 and rLocal3 must have been set by the calling
// routine.

asm void SearchCheckQRB(void) {
#define scan_check_dir(dir, dirMask, rayBit, L1, L2, L3)                   \
  mr Ato, Aksq; /* dir = QueenDir[i];                       */             \
  mr Bto, Bksq; /* to = ksq;                                */             \
  b L2;         /* while (isEmpty(to -= dir))               */             \
  L1 andi.rTmp2, rTmp1,                                                    \
      dirMask;              /* {  if (! (a = AttackP[to]))  continue;   */ \
  subf rTmp3, rAttack, Ato; /*    if (! (bits = a & dirMask)) continue; */ \
  bnel - SearchCheckQRB1;   /*    SearchCheckQRB1(a,bits,to4);          */ \
  L2 lhau rTmp4, -(2 * dir)(Bto);                                          \
  lwzu rTmp1, -(4 * dir)(Ato);                                             \
  cmpi cr5, 0, rTmp4, empty;                                               \
  beq cr5, L1;             /* }                                        */  \
  blt - cr5, L3;           /* if (onBoard(to) &&                       */  \
  andi.rTmp5, rTmp4, 0x10; /*     friendly(Board[to]) &&               */  \
  li rTmp3, dir;                                                           \
  cmp cr5, 0, rTmp5, rPlayer;                                              \
  andi.rTmp6, rTmp1, rayBit;                                               \
  bne cr5, L3; /*     AttackP[to] & RayBit[i])             */              \
  mr rTmp1, rTmp4;                                                         \
  subf rTmp2, rBoard, Bto;                                                 \
  bnel - SearchIndCheck; /*    SearchIndCheck(Board[to],dir,to2);    */    \
  L3

#define ksq rLocal1   // Location of enemy king (set by caller - unused)
#define Aksq rLocal2  // &AttackP[ksq] (set by caller)
#define Bksq rLocal3  // &Board[ksq] (set by caller)
#define Ato rLocal4
#define Bto rLocal5

  frame_begin5

      scan_check_dir(-0x0F, qbMask, 0x0101, @F0, @R0, @N0) scan_check_dir(
          -0x11, qbMask, 0x0202, @F1, @R1, @N1)
          scan_check_dir(+0x11, qbMask, 0x0404, @F2, @R2, @N2) scan_check_dir(
              +0x0F, qbMask, 0x0808, @F3, @R3, @N3)
              scan_check_dir(-0x10, qrMask, 0x1010, @F4, @R4, @N4)
                  scan_check_dir(+0x10, qrMask, 0x2020, @F5, @R5, @N5)
                      scan_check_dir(+0x01, qrMask, 0x4040, @F6, @R6, @N6)
                          scan_check_dir(-0x01, qrMask, 0x8080, @F7, @R7, @N7)

                              frame_end5 blr

#undef ksq
#undef Aksq
#undef Bksq
#undef Ato
#undef Bto
} /* SearchCheckQRB */

// The "SearchCheckQRB1()" routine searches all safe checks to a single empty
// square "to". On entry, rTmp1 contains the relevant QRB attackbits of the
// target square "to". Additionally, rLocal4 = &Attack[to], and rLocal5 =
// &Board[to]. These registers are NOT changed.

asm void SearchCheckQRB1(register ATTACK _ato, register ATTACK _bits,
                         register SQUARE _to4) {
  lwzx rTmp4, rAttack_,
      rTmp3  // d = AttackO[to];
          andis.rTmp5,
      rTmp4,
      0x0600  // if (d & pMask) return;
      bnelr andis.rTmp6,
      rTmp4,
      0x00FF  // if (d & bnMask)
      bne @1  // {
      andi.rTmp7,
      rTmp4,
      0x0F00  //    bits &= bMask;
      beq @2  //    if (bits == 0) return;
      @1 andi.rTmp1,
      rTmp1,
      0x0F00 beqlr  // }
          b @SCAN   // else if (d & rMask)
      @2 andi.rTmp8,
      rTmp4,
      0xF000     // {
      beq @SCAN  //    bits &= rbMask;
          andi.rTmp1,
      rTmp1,
      0xFF00  //    if (bits == 0) return;
      beqlr   // }

      @SCAN

#define dest rLocal1
#define d rLocal2
#define a rLocal3
#define Data rLocal4

          frame_begin4

              addi Data,
      rGlobal,
      GLOBAL.M.QRBdata  // m.to = to;
          mr a,
      rTmp1 mr d, rTmp4 srawi rTmp4, rTmp1, 8 srawi dest, rTmp3, 2 or rTmp1,
      rTmp1, rTmp4 sth dest, move(to) rlwinm rTmp1, rTmp1, 4, 20,
      27    // for (bits = ...; bits; clrBit(j,bits))
      @FOR  // {
          lhaux rTmp3,
      Data,
      rTmp1  //    dir = QueenDir[j = LowBit[bits]];
          lhz rTmp6,
      node(escapeSq) mr rTmp2, dest @gfrom subf rTmp2, rTmp3,
      rTmp2  //    m.from = to;
          add rTmp4,
      rTmp2, rTmp2 lhzx rTmp5, rBoard,
      rTmp4  //    while (isEmpty(m.from -= dir));
          cmpi cr0,
      0, rTmp5, empty beq + cr0, @gfrom cmp cr5, 0, rTmp2,
      rTmp6  //    if (m.from != N->escapeSq ||
              beq -
          @ROF  //
              cmpi cr0,
      0, d,
      0  //        d == 0 ||
      add rTmp4,
      rTmp4, rTmp4 beq + cr0,
      @s  //        (a & ~RayBit[i]) ||
          lwz rTmp7,
      QRB_DATA.rayBit(Data) andc.rTmp1, a,
      rTmp7 bne @s  //        (AttackP[m.from] & RayBit[j])
          lwzx rTmp6,
      rAttack, rTmp4 and rTmp6, rTmp6,
      rTmp7  //       )
              beq +
          @ROF @s sth rTmp2,
      move(from)  //    {
      sth rTmp3,
      move(dir)  //       m.dir = dir;
      sth rTmp5,
      move(piece)     //       m.piece = Board[m.from];
      bl ProcessMove  //       ProcessMove();
      @ROF lha rTmp1,
      QRB_DATA.offset(Data)  //    }
      cmpi cr0,
      0, rTmp1,
      0  //    bits = QRBdata[bits].nextBits;
          bne -
          @FOR  // }

              frame_end4 blr

#undef dest
#undef d
#undef a
#undef Data
} /* SearchCheckQRB1 */

asm void SearchIndCheck(register PIECE _piece, register SQUARE _from2,
                        register INT _idir) {
#define ksq rLocal1      // Location of enemy king (set by caller)
#define src rLocal2      // Source square of moving piece.
#define dst rLocal3      // Destination square of moving piece.
#define idir rLocal4     // Direction of indirect check.
#define Dir rLocal5      // Generic direction table for piece type.
#define mdir rLocal6     // Direction if rook/bishop.
#define dirMask rLocal4  // Direction mask if rook/bishop (same reg as idir).

  frame_begin6

      andi.rTmp4,
      rTmp1, 0x07 srawi src, rTmp2,
      1  // m.piece = piece;
      sth rTmp1,
      move(piece)  // m.from = from;
      sth src,
      move(from)  // switch (pieceType(m.piece))
      cmpi cr5,
      0, rTmp4,
      knight  // {
          mr idir,
      rTmp3 beq cr5, @KNIGHT blt cr5, @PAWN cmpi cr6, 0, rTmp4, rook beq cr6,
      @ROOK blt cr6,
      @BISHOP

      /* - - - - - - - - - - - - - - - - Indirect Check With King - - - - - - -
         - - - - - - - - -*/

      @KING  //    case king:
          addi Dir,
      rGlobal,
      (GLOBAL.B.KingDir - 2)  //     for (i = 0; i < 7; i++)
      b @rofK                 //       {
      @forK                   //          dir = KingDir[i];
          cmp cr5,
      0, rTmp1, idir add.rTmp2, rTmp1,
      idir  //          if (dir == idir || dir == -idir)
              beq -
          cr5,
      @rofK  //             continue;
              beq -
          @rofK add dst,
      src,
      rTmp1  //          m.to = m.from + dir;
          add rTmp2,
      dst, dst lhax rTmp3, rBoard,
      rTmp2  //          if (Board[m.to] != empty) continue;
          add rTmp4,
      rTmp2, rTmp2 cmpi cr5, 0, rTmp3, empty lwzx rTmp5, rAttack_,
      rTmp4 bne cr5, @rofK cmpi cr6, 0, rTmp5,
      0  //          if (Attack_[m.to]) continue;
      bne cr6,
      @rofK sth dst,
      move(to) bl ProcessMove  //          ProcessMove();
      @rofK lhau rTmp1,
      2(Dir)cmpi cr0, 0, rTmp1, 0 bne + cr0,
      @forK          //       }
          b @RETURN  //       return;

      /* - - - - - - - - - - - - - - - Indirect Check With Knight - - - - - - -
         - - - - - - - - -*/
      // We have to skip direct checks (e.g. double checks), since these are
      // searched elsewhere.

      @KNIGHT  //    case knight:
          addi Dir,
      rGlobal,
      (GLOBAL.B.KnightDir - 2)  //   for (i = 0; i < 7; i++)
      b @rofN                   //       {
      @forN                     //          dir = KnightDir[i];
          add dst,
      src,
      rTmp1  //          m.to = m.from + dir;
          add rTmp2,
      dst, dst lhax rTmp3, rBoard,
      rTmp2  //          if (Board[m.to] != empty)
          cmpi cr5,
      0, rTmp3,
      empty  //             continue;
          addi rTmp4,
      rGlobal, GLOBAL.A.AttackDir bne cr5, @rofN subf.rTmp5, ksq,
      dst  //          if (AttackDir[m.to - ksq] & nDirMask)
          add rTmp5,
      rTmp5,
      rTmp5  //             continue;
          lhzx rTmp6,
      rTmp4, rTmp5 andi.rTmp6, rTmp6, nDirMask bne - @rofN sth dst,
      move(to) bl ProcessMove  //          ProcessMove();
      @rofN lhau rTmp1,
      2(Dir)cmpi cr0, 0, rTmp1, 0 bne + cr0,
      @forN          //       }
          b @RETURN  //       return;

      /* - - - - - - - - - - - - - - - - Indirect Check With Pawn - - - - - - -
         - - - - - - - - -*/
      // We have to skip direct checks (e.g. double checks), and pawn moves to
      // the 7th or 8th rank since these are searched elsewhere.

      @PAWN  //    case pawn:
          cmp cr5,
      0, rPawnDir, idir add.rTmp2, rPawnDir,
      idir  //       if (idir == pawnDir || idir == -pawnDir)
              beq -
          cr5,
      @RETURN  //          return;
              beq -
          @RETURN add dst,
      rPawnDir,
      src  //       m.to = m.from + pawnDir;
          add rTmp1,
      dst, dst lhzx rTmp2, rBoard,
      rTmp1  //       if (Board[m.to] != empty)
          sth dst,
      move(to)  //          return;
      cmpi cr0,
      0, rTmp2, empty bne @RETURN add rTmp3, rPawnDir,
      dst  //       if (onRank8(m.to) ||ÊonRank7(m.to))
          add rTmp3,
      rPawnDir,
      rTmp3  //          return;
          andi.rTmp3,
      rTmp3,
      0x88 bne - @RETURN bl ProcessMove  //       ProcessMove();
                     subf rTmp3,
      rPawnDir, src subf rTmp3, rPawnDir,
      rTmp3  //       if (! onRank2(m.from))
          andi.rTmp3,
      rTmp3,
      0x88  //          return;
          beq +
          @RETURN add dst,
      rPawnDir,
      dst  //       m.to += pawnDir;
          add rTmp1,
      dst, dst lhax rTmp2, rBoard,
      rTmp1  //       if (Board[m.to] != empty)
          cmpi cr0,
      0, rTmp2,
      empty  //          return;
          bne @RETURN sth dst,
      move(to) bl ProcessMove  //       ProcessMove();
          b @RETURN            //       return;

      /* - - - - - - - - - - - - - - Indirect Check With Rook/Bishop - - - - - -
         - - - - - - - - */

      @BISHOP  //    case bishop :
          addi Dir,
      rGlobal,
      (GLOBAL.B.BishopDir - 2)  //   Dir = BishopDir;
      li dirMask,
      bDirMask      //       dirMask = bDirMask;
          b @rofRB  //       break;

      @ROOK  //    case rook :
          addi Dir,
      rGlobal,
      (GLOBAL.B.RookDir - 2)  //     Dir = RookDir;
      li dirMask,
      bDirMask      //       dirMask = rDirMask;
          b @rofRB  //       break;
                    // }

      @forRB  // for (i = 0; i < 3; i++)
          add dst,
      mdir,
      src  // {
          sth mdir,
      move(dir)  //    m.dir = Dir[i];
      b @od2     //    m.to = m.from;
      @ while    //    while (Board[m.to += m.dir] == empty)
      addi rTmp1,
      rGlobal,
      GLOBAL.A.AttackDir  //    {
          subf rTmp2,
      dst,
      ksq  //       fdir = AttackDir[ksq - m.to];
          add rTmp2,
      rTmp2, rTmp2 lhax rTmp3, rTmp1,
      rTmp2  //       if (fdir & dirMask)
          and.rTmp4,
      rTmp3,
      dirMask  //       {
              beq +
          @search srawi rTmp3,
      rTmp3,
      5  //          fdir >>= 5;  // skip direct checks
      add rTmp2,
      dst, dst add rTmp3, rTmp3,
      rTmp3  //          sq = m.to;
          li rTmp5,
      bKing add rTmp2, rBoard, rTmp2 subf rTmp5, rPlayer,
      rTmp5 @loop  //          while (isEmpty(sq += fdir));
          lhaux rTmp4,
      rTmp2, rTmp3 cmpi cr5, 0, rTmp4, empty beq + cr5, @loop cmp cr6, 0, rTmp4,
      rTmp5  //          if (Board[sq] == enemy(king))
              bne +
          cr6,
      @od1     //             continue;
      @search  //       }
          sth dst,
      move(to) bl ProcessMove  //       ProcessMove();
      @od1
          // subf    dst, mdir,dst
          add dst,
      mdir, dst @od2 add rTmp1, dst, dst lhax rTmp2, rBoard, rTmp1 cmpi cr0, 0,
      rTmp2, empty beq + cr0,
      @ while  //    }
      @rofRB lhau mdir,
      2(Dir)cmpi cr0, 0, mdir,
      0
          // cmpi    cr0,0, rTmp1,0
          bne +
          @forRB  // }

          @RETURN frame_end6 blr

#undef ksq
#undef src
#undef dst
#undef idir
#undef Dir
#undef mdir
#undef dirMask
} /* SearchIndCheck */

/*--------------------------------- Search Safe Knight Checks
 * ------------------------------------*/

asm void SearchCheckN1(register INT _sq, register ATTACK _asq,
                       register INT _bits);

asm void SearchCheckN(void) {
#define checkN(dir, L)                                                       \
  lha rTmp4, (2 * dir)(Bksq); /* to = ksq + dir;                          */ \
  lwz rTmp2, (4 * dir)(Aksq); /* if (Board[to] != empty)                  */ \
  addi rTmp1, ksq, dir;       /*    continue;                             */ \
  cmpi cr5, 0, rTmp4, empty;  /* a = Attack[to];                          */ \
  andis.rTmp3, rTmp2, 0x00FF; /* bits = a & nMask;                        */ \
  bne + cr5, L;               /* if (bits)                                */ \
  bnel - SearchCheckN1;       /*    SearchCheckN1(sq, a, bits);           */ \
  L

#define ksq rLocal1   // Location of enemy king (set by caller - not used)
#define Aksq rLocal2  // &AttackP[ksq] (set by caller - not changed)
#define Bksq rLocal3  // &Board[ksq] (set by caller - not changed)

  frame_begin0

      checkN(-0x0E, @0) checkN(-0x12, @1) checkN(-0x1F, @2) checkN(-0x21, @3)
          checkN(+0x12, @4) checkN(+0x0E, @5) checkN(+0x21, @6)
              checkN(+0x1F, @7)

                  frame_end0 blr

#undef ksq
#undef Aksq
#undef Bksq
} /* SearchCheckN */

asm void SearchCheckN1(register INT _sq, register ATTACK _asq,
                       register INT _bits) {
  rlwinm rTmp4, rTmp1, 2, 0,
      31  // d = Attack_[to];
      lwzx rTmp5,
      rAttack_, rTmp4 cmpi cr5, 0, rTmp5,
      0  // if (d != 0)
      beq cr5,
      @BEGIN  // {
          andis.rTmp6,
      rTmp5,
      0x0600       //    if (d & pMask) return;
          bnelr -  //    if (! (AttackP[m.to] & ~nMask) &&
          cntlzw rTmp7,
      rTmp2  //        NumBits[bits] == 1)
          rlwnm.rTmp8,
      rTmp2, rTmp7, 1,
      31     //       return;
      beqlr  // }

      @BEGIN
#define Data rLocal1

          frame_begin1

              addi Data,
      rGlobal, GLOBAL.M.Ndata addi rTmp2, rPlayer,
      knight  // m.piece = friendly(knight);
          sth rTmp1,
      move(to)  // m.to = ksq + dir;
      srawi rTmp3,
      rTmp3, 13 sth rTmp2,
      move(piece) @FOR  // for (bits =...; bits; clrBit(j,bits))
          lhaux rTmp4,
      Data,
      rTmp3  // {
          lha rTmp6,
      node(escapeSq)  //    dir = KnightDir[j = LowBit[bits]];
      subf rTmp5,
      rTmp4,
      rTmp1  //    m.from = m.to - dir;
          cmp cr0,
      0, rTmp5, rTmp6 sth rTmp5,
      move(from)  //    if (m.from != escapeSq)
          bnel +
          ProcessMove  //       ProcessMove();
              lha rTmp3,
      N_DATA.offset(Data)  //
      lhz rTmp1,
      move(to) cmpi cr0, 0, rTmp3,
      0  //    bits = QRBdata[bits].nextBits;
          bne -
          cr0,
      @FOR  // }

          frame_end1 blr

#undef Data
} /* SearchCheckN1 */

/*------------------------------------ Search Safe Pawn Checks
 * -----------------------------------*/
// For simplicity it is sufficient to search all pawn checks - even unsafe ones.
// However pawn moves to the 6th and 7th rank are NOT searched here, because
// they are searched by another move generator [K].

asm void SearchCheckP(void) {
#define checkP(hdir, L1, L2, Next)                                             \
  add rTmp1, rPawnDir, rPawnDir; /* m.to = ksq - pawnDir + hdir; */                                                                            \
  addi rTmp2, Bksq, (hdir * 2); /* if (Board[m.to] != empty) continue;      */ \
  neg rTmp1, rTmp1;                                                            \
  lhaux rTmp3, rTmp2, rTmp1; /* m.from = m.to - pawnDir;                 */    \
  cmpi cr5, 0, rTmp3, empty;                                                   \
  lhaux rTmp4, rTmp2, rTmp1; /* if (Board[m.from] == friendly(pawn))     */    \
  bne - cr5, Next;           /* {                                        */    \
  addi rTmp5, rPlayer, pawn;                                                   \
  cmp cr6, 0, rTmp4, rTmp5;                                                    \
  bne + cr6, L2; /*    m.piece = friendly(pawn);             */                \
  L1 addi rTmp6, ksq, hdir;                                                    \
  subf rTmp7, rPawnDir, rTmp6;                                                 \
  add rTmp8, rTmp1, rTmp6;                                                     \
  sth rTmp7, move(to); /*    ProcessMove();                        */          \
  sth rTmp8, move(from);                                                       \
  sth rTmp5, move(piece);                                                      \
  frame_begin0;                                                                \
  bl ProcessMove; /* }                                        */               \
  frame_end0;                                                                  \
  b Next;                       /* else if (Board[m.from] == empty)         */ \
  L2 cmpi cr5, 0, rTmp4, empty; /* {                                        */ \
  bne cr5, Next;                /*    m.from -= pawnDir;                    */ \
  lhaux rTmp3, rTmp2, rTmp1;    /*    if (rank(m.from) == rank2(player) &&  */ \
  andi.rTmp6, ksq, 0x70;        /*        Board[m.from] == friendly(pawn))  */ \
  add rTmp6, rTmp6, rPlayer;    /*    {                                     */ \
  cmpi cr5, 0, rTmp6, 0x40;     /*       m.piece = friendly(pawn);          */ \
  bne - cr5, Next;                                                             \
  cmp cr6, 0, rTmp3, rTmp5;    /*    ProcessMove();                        */  \
  subf rTmp1, rPawnDir, rTmp1; /* }                                        */  \
  beq - cr6, L1;                                                               \
  Next

#define ksq rLocal1   // Location of enemy king (set by caller - not changed)
#define Aksq rLocal2  // &AttackP[ksq] (set by caller - not used)
#define Bksq rLocal3  // &Board[ksq] (set by caller - not changed)

  add rTmp1, ksq,
      rPlayer  // if (rank(ksq) < rank4(player)) return;
          cmpi cr0,
      0, rTmp1,
      0x30 bltlr  // if (rank(ksq) > rank6(player)) return;
          cmpi cr0,
      0, rTmp1,
      0x57 bgtlr

          checkP(-1, @L1, @L2, @L3) checkP(1, @R1, @R2, @R3)

              blr

#undef ksq
#undef Aksq
#undef Bksq
} /* SearchCheckP */

/**************************************************************************************************/
/*                                                                                                */
/*                                [K] PAWN MOVES TO 6th & 7TH RANK */
/*                                                                                                */
/**************************************************************************************************/

// This routines searches all non-capturing pawn moves to the 6th and 7th rank
// (including sacrifices).

asm void SearchFarPawns(void) {
#define search_far_pawns(psx, rto, L1, L2, L3)                                \
  lbz rTmp1, (psx)(rEngine); /* bits = PawnStructX[rfrom];               */   \
  sth rTmp9, node(gen);                                                       \
  cmpi cr5, 0, rTmp1, 0; /* if (bits == 0) return;                   */       \
  beq + cr5, L3;                                                              \
  frame_begin1;                                                               \
  mr bits, rTmp1;                                                             \
  L1;                 /* for (bits = ...; bits; clrbit(j,bits))   */          \
  cntlzw rTmp2, bits; /* {                                        */          \
  li rTmp1, 31;       /*    f = LowBit[bits];                     */          \
  subf rTmp2, rTmp2, rTmp1;                                                   \
  addi rTmp3, rTmp2, rto; /*    m.to = rto + f;                       */      \
  add rTmp4, rTmp3, rTmp3;                                                    \
  li rTmp5, 1;                                                                \
  lhax rTmp6, rBoard, rTmp4; /*    if (Board[m.to] != empty) continue;   */   \
  rlwnm rTmp7, rTmp5, rTmp2, 0, 31;                                           \
  andc bits, bits, rTmp7;                                                     \
  cmpi cr5, 0, rTmp6, empty;                                                  \
  bne cr5, L2;                                                                \
  subf rTmp4, rPawnDir, rTmp3; /*    m.from  = m.to - pawnDir;             */ \
  li rTmp1, 0;                 /*    m.piece = friendly(pawn);             */ \
  addi rTmp2, rPlayer, pawn;   /*    m.cap   = empty;                      */ \
  stw rTmp1, move(cap);        /*    m.type  = mtype_Normal;               */ \
  sth rTmp2, move(piece);                                                     \
  sth rTmp3, move(to);                                                        \
  sth rTmp4, move(from);                                                      \
  bl ProcessMove; /*    ProcessMove();                        */              \
  L2;                                                                         \
  cmpi cr6, 0, bits, 0;                                                       \
  bne - cr6, L1;                                                              \
  frame_end1; /* }                                        */                  \
  L3

#define bits rLocal1

  cmpi cr0, 0, rPlayer, white li rTmp9,
      gen_K  // N->gen = gen_K;
          bne @BLACK
      @WHITE search_far_pawns(ENGINE.B.PawnStructW + 5, 0x60, @W1a, @W2a, @W3a)
          search_far_pawns(ENGINE.B.PawnStructW + 4, 0x50, @W1b, @W2b, @W3b) blr
      @BLACK search_far_pawns(ENGINE.B.PawnStructB + 2, 0x10, @B1a, @B2a, @B3a)
          search_far_pawns(ENGINE.B.PawnStructB + 3, 0x20, @B1b, @B2b, @B3b) blr

#undef bits
} /* SearchFarPawns */

/**************************************************************************************************/
/*                                                                                                */
/*                                       [L] CHECK EVASION */
/*                                                                                                */
/**************************************************************************************************/

asm void SearchAllKingMoves(void);
asm void SearchInterpositions(void);
asm void SearchInterpositions1(void);

// The "SearchCheckEvasion()" routine should be called if the side to move is
// check. It generates and searches all strictly legal (except ep which are
// always tried) check evasion moves (including sacrifices).

asm void SearchCheckEvasion(void) {
#define ksq rLocal1   // Location of king
#define csq rLocal2   // Location of checking piece
#define cdir rLocal3  // Direction of checking piece (if QRB)

  frame_begin3

      lhz ksq,
      0(rPieceLoc)  // ksq = KingLoc(player);
      li cdir,
      0  // cdir = 0;
      li rTmp9,
      gen_L  // N->gen = gen_L;
          sth cdir,
      move(type)  // m.type = normal
      sth rTmp9,
      node(gen)

      /*- - - - - - - - - - - - - - - - - Find Checking Piece - - - - - - - - -
         - - - - - - - - -*/
      // First we locate the checking piece(s) and check if it's a double check.
      // If so we only search king moves, otherwise we search normal check
      // evasion:

      rlwinm.rTmp1,
      ksq, 2, 0,
      31  // a = Attack_[ksq];
      mr csq,
      ksq  // csq = ksq;
          lwzx rTmp2,
      rAttack_,
      rTmp1

      @PAWN andis.rTmp3,
      rTmp2,
      0x0600  // if (a & pMask)
          beq +
          @KNIGHT  // {
              andi.rTmp4,
      rTmp2,
      0xFFFF  //    if (a & qrbMask) goto DOUBLE;
          bne -
          @DOUBLE srawi rTmp3,
      rTmp3,
      24  //    csq += pawnDir + (a & pMaskL ? 1 : -1);
      add csq,
      ksq, rPawnDir addi rTmp3, rTmp3, -3 add csq, csq,
      rTmp3          //    goto NORMAL;
          b @NORMAL  // }
      @KNIGHT andis.rTmp3,
      rTmp2,
      0x00FF    // if (a & nMask)
      beq @QRB  // {
          andi.rTmp4,
      rTmp2,
      0xFFFF  //    if (a & qrbMask) goto DOUBLE;
      addi rTmp1,
      rGlobal, GLOBAL.M.Ndata bne - @DOUBLE srawi rTmp3, rTmp3,
      13  //    csq -= KnightDir[LowBit[nBits(a)]]);
      lhax rTmp4,
      rTmp1, rTmp3 subf csq, rTmp4,
      ksq            //    goto NORMAL;
          b @NORMAL  // }
      @QRB addi rTmp6,
      rGlobal,
      GLOBAL.M.QRBdata  //
          srawi rTmp3,
      rTmp2, 8 or rTmp4, rTmp2, rTmp3 rlwinm rTmp4, rTmp4, 4, 20, 27 lwzx rTmp1,
      rTmp6, rTmp4 andi.rTmp7, rTmp1, 0xFFFF bne - @DOUBLE srawi cdir, rTmp1,
      16 @loop  // while (isEmpty(csq -= cdir));
          subf csq,
      cdir, csq add rTmp1, csq, csq lhzx rTmp2, rBoard, rTmp1 cmpi cr0, 0,
      rTmp2,
      empty beq + @loop b @NORMAL  // goto NORMAL;

          /*- - - - - - - - - - - - - - - - - Double Check Evasion - - - - - - -
             - - - - - - - - - - */

          @DOUBLE                    // DOUBLE:
              bl SearchAllKingMoves  // SearchAllKingMoves(...);
                  b @RETURN          // goto RETURN;

          /*- - - - - - - - - - - - - - - - - Normal Check Evasion - - - - - - -
             - - - - - - - - - - */
          // The first thing we do here is to try and capture the checking
          // piece. Then we search king moves, en passant, interpositions and
          // finally sacrifices:

          @NORMAL rlwinm rTmp2,
      csq, 2, 0,
      31  // if (Attack[csq])
      lwzx rTmp3,
      rAttack,
      rTmp2  // {
          mr rTmp1,
      csq cmpi cr0, 0, rTmp3,
      0  //    SearchEnPriseCaptures1(...);
          beq +
          cr0,
      @noncap bl SearchEnPriseCaptures1 rlwinm rTmp2, csq, 2, 0, 31 lwzx rTmp4,
      rAttack_,
      rTmp2  //    if (Attack_[csq])
          lwzx rTmp3,
      rAttack, rTmp2 mr rTmp1, csq cmpi cr0, 0, rTmp4,
      0  //       SearchSafeCaptures1(...);
      bnel cr0,
      SearchSafeCaptures1            // }
      @noncap bl SearchAllKingMoves  // SearchAllKingMoves(...);
          bl SearchEnPassant         // SearchEnPassant();
              cmpi cr0,
      0, cdir,
      0  // if (cdir != 0)
          bnel +
          cr0,
      SearchInterpositions     //    SearchInterpositions(csq, cdir);
          bl SearchSacrifices  // SearchSacrifices();

      @RETURN frame_end3 blr

#undef ksq
#undef csq
#undef cdir

} /* SearchCheckEvasion */

/*--------------------------------- Search All Legal King Moves
 * ----------------------------------*/
// Although defined as having no parameters, this routine assumes that rLocal1,
// rLocal2 and rLocal3 has been set properly by the calling routine (ksq, csq,
// cdir). It is also assumed that m.type is 0 (mtype_Normal). First all capture
// moves are examined, followed by non-captures. NOTE: Captures to the location
// of the checking piece (csq = rLocal2) are skipped in case of single checks,
// since these moves will be searched by the capture routines.

asm void SearchAllKingMoves(void) {
#define ksq rLocal1  // Location of king (set by caller).
#define csq \
  rLocal2  // Location of checking piece (set by caller). = ksq if double check.
#define cdir \
  rLocal3  // Direction of checking piece if QRB (set by caller). 0 if double
           // check.
#define KDir rLocal4  // Table of king directions.
#define i rLocal5

  frame_begin5

      addi KDir,
      rGlobal, GLOBAL.B.KingDir addi rTmp2, rPlayer, king sth ksq,
      move(from)  // m.from = ksq;
      sth rTmp2,
      move(piece)  // m.piece = player + king;

      /* - - - - - - - - - - - - - - - - - - - Captures - - - - - - - - - - - -
         - - - - - - - - - - */

      li i,
      14     // for (INT i = 0; i <= 7; i++)
      @FOR1  // {
          lhax rTmp1,
      KDir,
      i  //    m.dir = KingDir[i]; m.to = m.from + m.dir;
          cmp cr5,
      0, rTmp1, cdir add rTmp2, ksq,
      rTmp1  //    if (m.dir == cdir ||Êm.to == csq) continue;
              beq -
          cr5,
      @ROF1 cmp cr6, 0, rTmp2, csq andi.rTmp3, rTmp2,
      0x88  //    if (offBoard(m.to)) continue;
          beq -
          cr6,
      @ROF1 add rTmp4, rTmp2,
      rTmp2 bne - @ROF1  //    m.cap = Board[m.to];
                      lhzx rTmp5,
      rBoard, rTmp4 add rTmp6, rTmp4,
      rTmp4  //    if (m.cap == empty) continue;
          cmpi cr5,
      0, rTmp5, empty andi.rTmp7, rTmp5,
      0x10  //    if (pieceColour(m.cap) == player) continue;
          beq +
          cr5,
      @ROF1 cmp cr6, 0, rPlayer, rTmp7 beq cr6, @ROF1 lwzx rTmp8, rAttack_,
      rTmp6  //    if (Attack_[m.to] != 0) continue;
          cmpi cr5,
      0, rTmp8, 0 bne cr5, @ROF1 sth rTmp2,
      move(to)  //    ProcessMove();
      sth rTmp5,
      move(cap) sth rTmp1, move(dir) bl ProcessMove @ROF1 addi i, i,
      -2 cmpi cr0, 0, i, 0 bge + cr0,
      @FOR1  // }

          /* - - - - - - - - - - - - - - - - - - - Non Captures - - - - - - - -
             - - - - - - - - - - - - */

              li rTmp2,
      0  // m.cap = empty;
      li i,
      14 sth rTmp2,
      move(cap)  // for (INT i = 0; i <= 7; i++)
      @FOR2      // {
          lhax rTmp1,
      KDir,
      i  //    m.dir = KingDir[i]; m.to = m.from + m.dir;
          cmp cr5,
      0, rTmp1, cdir add rTmp2, ksq,
      rTmp1  //    if (m.dir == cdir) continue;
              beq -
          cr5,
      @ROF2 andi.rTmp3, rTmp2,
      0x88  //    if (offBoard(m.to)) continue;
      add rTmp4,
      rTmp2,
      rTmp2 bne - @ROF2  //    if (Board[m.to] != empty) continue;
                      lhzx rTmp5,
      rBoard, rTmp4 add rTmp6, rTmp4, rTmp4 cmpi cr5, 0, rTmp5, empty bne + cr5,
      @ROF2 lwzx rTmp8, rAttack_,
      rTmp6  //    if (Attack_[m.to] != 0) continue;
          cmpi cr5,
      0, rTmp8, 0 bne cr5, @ROF2 sth rTmp2,
      move(to)  //    ProcessMove();
      sth rTmp1,
      move(dir) bl ProcessMove @ROF2 addi i, i, -2 cmpi cr0, 0, i, 0 bge + cr0,
      @FOR2  // }

          frame_end5 blr

#undef ksq
#undef csq
#undef cdir
#undef KDir
#undef i

} /* SearchAllKingMoves */

/*--------------------------------- Search All Interpositions
 * ------------------------------------*/
// If the checking piece is a QRB we also have to search "interposition moves":
// I.e. for each empty square between the king and the checking piece, we try to
// move a piece to that square. The registers rLocal1..rLocal3 must be set up by
// the caller.

asm void SearchInterpositions(void) {
#define isq rLocal1   // Interposition square. Initially = ksq (set by caller)
#define csq rLocal2   // Location of checking piece (set by caller).
#define cdir rLocal3  // Direction of checking piece if QRB (set by caller).

   frame_begin3

   li      rTmp1, 0                           // m.cap = empty;
   stw     rTmp1, move(cap)                   // m.type = normal;
   b       @rof
@for                                          // for (m.to = ksq - cdir; m.to != csq; m.to -= cdir)
   bl      SearchInterpositions1              //    SearchInterpositions1(m.to);
@rof
   subf    isq, cdir,isq
   cmp     cr0,0, isq,csq
   bne+    @for

   frame_end3
   blr

#undef isq
#undef csq
#undef cdir
} /* SearchInterpositions */

/*--------------------------- Search Interpositions to a Single Square
 * ---------------------------*/
// The "SearchInterpositions1" routine searches all interposition moves to the
// (empty) square specified in the rLocal1 register.

asm void SearchInterpositions1(void) {
#define isq rLocal1
#define a rLocal2
#define Data rLocal3

   frame_begin3

   sth     isq, move(to)                      // m.to = isq;

   /*- - - - - - - - - - - - - - - - - Move pawns in between - - - - - - - - - - - - - - - - -*/

@PAWN
   subf    rTmp1, rPawnDir,isq                // m.from = m.to - pawnDir;
   add     rTmp2, rTmp1,rTmp1
   lhzx    rTmp3, rBoard,rTmp2                // if (Board[m.from] == empty)
   cmpi    cr0,0, rTmp3,empty                 // {
   addi    rTmp4, rPlayer,pawn
   bne     @1
   addi    rTmp5, rPlayer,0x30                //    m.from = m.to - 2*pawnDir;
   andi.   rTmp6, isq,0x70
   cmp     cr0,0, rTmp5,rTmp6                 //    if (onRank2(m.from) &&
   subf    rTmp1, rPawnDir,rTmp1              //    
   bne     cr0, @KNIGHT
   add     rTmp2, rTmp1,rTmp1                 //        Board[m.from] == player + pawn)
   lhzx    rTmp3, rBoard,rTmp2
   cmp     cr0,0, rTmp3,rTmp4
   bne+    @KNIGHT                            //    {
   sth     rTmp4, move(piece)                 //       m.piece = friendly(pawn);
   sth     rTmp1, move(from)                  //       ProcessMove();
@niels
   bl      ProcessMove                        //    }      
   b       @KNIGHT                            // }
@1
   cmp     cr0,0, rTmp3,rTmp4                 // else if (Board[m.from] == player + pawn)
   bne     @KNIGHT                            // {
   add     rTmp5, isq,rPawnDir                //    m.piece = friendly(pawn);
   sth     rTmp1, move(from)
   andi.   rTmp5, rTmp5,0x88                  //    if (! onRank8(m.to))
   sth     rTmp3, move(piece)                 //       ProcessMove();
   beq+    @niels                             //    else
   bl      SearchPromotion1                   //       SearchPromotion1(...);
                                              // }

   /*- - - - - - - - - - - - - - - - Move knights in between - - - - - - - - - - - - - - - - -*/

@KNIGHT
   rlwinm  rTmp2, isq,2,0,29
   lwzx    a, rAttack,rTmp2
   andis.  rTmp1, a,0x00FF                // bits = (a & nMask) >> 16;
   beq+    @QRB
   srawi   rTmp1, rTmp1,13                // if (bits) 
   addi    Data, rGlobal,GLOBAL.M.Ndata   // {
   addi    rTmp2, rPlayer,knight          //    m.piece = friendly(knight);
   sth     rTmp2, move(piece)
   andi.   rTmp1, rTmp1,0x07F8            //    Data = Ndata;
@NFor
   lhaux   rTmp2, Data,rTmp1              //    do
   subf    rTmp2, rTmp2,isq               //    {  
   sth     rTmp2, move(from)              //       m.from = m.to + Ndata[bits].ndir;
   bl      ProcessMove                    //       ProcessMove(...);
   lha     rTmp1, N_DATA.offset(Data)     //       bits = Ndata[bits].nextBits;
   cmpi    cr0,0,rTmp1,0                  //    } while(bits);
   bne-    @NFor                          // }

   /*- - - - - - - - - - - - - - - Move sliding pieces in between - - - - - - - - - - - - - - */

@QRB
   andi.   rTmp2, a,0xFFFF
   srawi   rTmp1, a,8                     // bits = ((a >> 8) | a) & 0xFF;
   beq+    @RETURN
   or      rTmp1, a,rTmp1
   rlwinm. rTmp1, rTmp1,4,20,27           // while (bits)
   addi    Data, rGlobal,GLOBAL.M.QRBdata // {
@loop
   lhaux   rTmp3, Data,rTmp1              //    dir = QueenDir[j = LowBit[bits]];
   mr      rTmp4, isq
@for
   subf    rTmp4, rTmp3,rTmp4             //    m.from = m.to;
   add     rTmp5, rTmp4,rTmp4
   lhax    rTmp6, rBoard,rTmp5            //    while (isEmpty(m.from -= dir));
   cmpi    cr0,0,rTmp6,empty
   beq+    cr0,@for
   sth     rTmp6, move(piece)             //    m.piece = Board[m.from];
   sth     rTmp3, move(dir)               //    m.dir = dir;
   sth     rTmp4, move(from)
   bl      ProcessMove
   lha     rTmp1, QRB_DATA.offset(Data)
   cmpi    cr0,0,rTmp1,0                  //    clrBit(j,bits);
   bne-    cr0,@loop                      // }

@RETURN
   frame_end3
   blr

#undef isq
#undef a
#undef Data
} /* SearchInterpositions1 */

/**************************************************************************************************/
/*                                                                                                */
/*                                      START UP INITIALIZATION */
/*                                                                                                */
/**************************************************************************************************/

void InitMoveGenModule(
    GLOBAL *Global)  // Must be initialized AFTER the Attack and Board modules.
{
  ATTACK_COMMON *A = &(Global->A);
  MOVEGEN_COMMON *M = &(Global->M);
  N_DATA *n;
  QRB_DATA *q;

  for (INT bits = 1; bits < 256; bits++) {
    INT j = A->LowBit[bits];

    n = &(M->Ndata[bits]);
    n->dir = Global->B.KnightDir[j];
    n->Nbit = A->KnightBit[j];
    n->offset =
        (bits == bit(j) ? 0
                        : -bit(j) * sizeof(N_DATA));  // Offset to "next" entry

    q = &(M->QRBdata[bits]);
    q->dir = Global->B.QueenDir[j];
    q->RBbit = A->RookBit[j] | A->BishopBit[j];
    q->Qbit = A->QueenBit[j];
    q->rayBit = A->RayBit[j];
    q->offset = (bits == bit(j)
                     ? 0
                     : -bit(j) * sizeof(QRB_DATA));  // Offset to "next" entry
  }
} /* InitMoveGenModule */
