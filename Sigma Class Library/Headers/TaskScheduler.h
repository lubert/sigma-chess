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


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define taskStackSize   (30L*1024L - 108L)  // Must be a multiple of 4

#define taskFuncWrapper(Func) \
   mflr r4 ; bl @L ; addi r3, r3,20 ; mtlr r4 ; blr ; @L mflr r3 ; blr ; \
@T call_c(MainSearch) ; \
   blr


/**************************************************************************************************/
/*                                                                                                */
/*                                        TYPE DEFINITIONS                                        */
/*                                                                                                */
/**************************************************************************************************/

typedef LONG (*TASKFUNC)(void *);

typedef struct task                   // Size: 108 bytes + stacksize
{
   INT      id;                       // Id = 0...maxTaskCount - 1
   BOOL     active;                   // Is this task in active/running task list?
   BOOL     paused;                   // Is the task paused? Not applicable to the main task!
   INT      priority;                 // Number of ticks to wait/sleep when switched out.
   ULONG    sleepTime;                // Tick count when task wants to wake up again.

   PTR      data;                     // Initial data supplied to task function in r3.

   PTR      lr;                       // Saved processor state (registers, including SP and LR).
   PTR      sp;
   LONG     gpr[19];
 
   struct task *next;                 // Next task in active queue.
   struct task *prev;                 // previous task in active queue.

   ULONG Stack[taskStackSize/4];      // Variable length stack (empty for main task as it uses the
                                      // application stack). MUST START ON 4 WORD BOUNDARY!!
} TASK;


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

asm void Task_Begin (register INT count);
asm void Task_End (void);
void Task_RunScheduler (TASKFUNC MainFunc, PTR data, INT priority = 5);
INT  Task_GetCurrent (void);
INT  Task_GetCount (void);
INT  Task_Create (TASKFUNC Func, PTR data, INT priority = 5);
void Task_Kill (INT id);
asm void Task_Switch (void);
