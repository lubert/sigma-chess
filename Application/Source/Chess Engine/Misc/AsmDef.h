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

#pragma once

/*--------------------------------------- Register Usage
 * -----------------------------------------*/
//
// General Purpose Register (GPR) conventions:
//
//   GPR0          Volatile      Function prolog and epilog
//   GPR1          Nonvolatile   Stack pointer
//   GPR2          Nonvolatile   TOC pointer (also known as RTOC)
//   GPR3-GPR10    Volatile      Arguments passed to a function or returned a
//   value or pointer GPR11-GPR12   Volatile      Function prolog and epilog
//   GPR13-GPR31   Nonvolatile   Storage for local variables
//
//   Condition Register (CR) conventions:
//
//   CR0           Volatile      Scratch area or set by integer instruction
//   record bit CR1           Volatile      Scratch area or set by
//   floating-point instruction record bit CR2-CR4       Nonvolatile   Local
//   storage CR5-CR7       Volatile      Scratch area
//
// The Engine pre-allocates many of the registers for global purposes before
// starting the actual analysis. Note that the nonvolatile registers MUST be
// restored when exiting the engine either upon termination, or termporarily
// when invoking a call back routine (e.g. when displaying best line etc.)

//--- Misc ---

#define rTmp0 r0  // Function prolog/epilog and scratch.

//--- Non-Volatile special purpose registers (SP and RTOC) ---

#define rSP r1  // Stack pointer (SP)
#define rTOC \
  r2  // TOC pointer (RTOC) - Not used by the engine (rGlobal instead).

//--- Volatile scratch registers (and function parameters) ---

#define rTmp1 r3
#define rTmp2 r4
#define rTmp3 r5
#define rTmp4 r6
#define rTmp5 r7
#define rTmp6 r8
#define rTmp7 r9
#define rTmp8 r10
#define rTmp9 r11
#define rTmp10 r12

//--- Non-Volatile "global" (engine specific) registers ---

#define rEngine r13  // Pointer to data for currently executing engine
#define rBoard r14   // ENGINE.B.Board (not changed during search)
#define rPieceCount \
  r15                   // ENGINE.B.pieceCount (incremental update + "copyback")
#define rNode r16       // ENGINE.S.currNode (incremental update + "copyback")
#define rPlayer r17     // ENGINE.S.currNode->player (incremental update)
#define rPawnDir r18    // ENGINE.S.currNode->pawnDir (incremental update)
#define rAttack r19     // ENGINE.S.currNode->Attack (incremental update)
#define rAttack_ r20    // ENGINE.S.currNode->Attack_ (incremental update)
#define rPieceLoc r21   // ENGINE.S.currNode->PieceLoc (incremental update)
#define rPieceLoc_ r22  // ENGINE.S.currNode->PieceLoc_ (incremental update)
#define rFlags r23  // Various flags (engine run state, flags, transtab on etc)

//--- Non-Volatile "global" (engine independant) registers ---

#define rGlobal r24  // Global read-only data used by all engine instances

//--- Non-Volatile general purpose local registers ---

#define rLocal7 r25
#define rLocal6 r26
#define rLocal5 r27
#define rLocal4 r28
#define rLocal3 r29
#define rLocal2 r30
#define rLocal1 r31

/*------------------------------------- PPC Assembler Macros
 * -------------------------------------*/
// Manage assembler stack frames. Must be used for assembler routines that use
// the non-volatile local registers, or calls other routines (an hence alters
// the LR).

#define frame_begin_(R, N) \
  stmw R, (-4 * N)(SP);    \
  stwu r0, (-4 * (N + 1))(SP)
#define frame_begin(R, N) \
  stmw R, (-4 * N)(SP);   \
  mflr r0;                \
  stwu r0, (-4 * (N + 1))(SP)
#define frame_end(R, N)     \
  lmw R, 4(SP);             \
  lwz r0, 0(SP);            \
  addi SP, SP, 4 * (N + 1); \
  mtlr r0

#define frame_begin0_ stwu r0, -4(SP)
#define frame_begin1_   \
  stwu rLocal1, -4(SP); \
  stwu r0, -4(SP)
#define frame_begin2_ frame_begin_(rLocal2, 2)
#define frame_begin3_ frame_begin_(rLocal3, 3)
#define frame_begin4_ frame_begin_(rLocal4, 4)
#define frame_begin5_ frame_begin_(rLocal5, 5)
#define frame_begin6_ frame_begin_(rLocal6, 6)
#define frame_begin7_ frame_begin_(rLocal7, 7)

#define frame_begin0 \
  mflr r0;           \
  stwu r0, -4(SP)
#define frame_begin1    \
  stwu rLocal1, -4(SP); \
  mflr r0;              \
  stwu r0, -4(SP)
#define frame_begin2 frame_begin(rLocal2, 2)
#define frame_begin3 frame_begin(rLocal3, 3)
#define frame_begin4 frame_begin(rLocal4, 4)
#define frame_begin5 frame_begin(rLocal5, 5)
#define frame_begin6 frame_begin(rLocal6, 6)
#define frame_begin7 frame_begin(rLocal7, 7)

#define frame_end0 \
  lwz r0, 0(SP);   \
  addi SP, SP, 4;  \
  mtlr r0
#define frame_end1    \
  lwz r0, 0(SP);      \
  lwz rLocal1, 4(SP); \
  addi SP, SP, 8;     \
  mtlr r0
#define frame_end2 frame_end(rLocal2, 2)
#define frame_end3 frame_end(rLocal3, 3)
#define frame_end4 frame_end(rLocal4, 4)
#define frame_end5 frame_end(rLocal5, 5)
#define frame_end6 frame_end(rLocal6, 6)
#define frame_end7 frame_end(rLocal7, 7)

// C-routine assembly wrappers. Must be used when calling C-routines from
// assembler routines (because the compiler for some strange reason stores the
// LR 8 bytes BELOW the current SP??!?).

#define call_c(Proc) \
  mflr r0;           \
  stwu r0, -12(SP);  \
  bl Proc;           \
  lwz r0, 0(SP);     \
  addi SP, SP, 12;   \
  mtlr r0

// Misc macros:

#define nop2 \
  nop;       \
  nop
#define nop4 \
  nop2;      \
  nop2
#define nop8 \
  nop4;      \
  nop4
#define nop16 \
  nop8;       \
  nop8
