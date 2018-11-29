/**************************************************************************************************/
/*                                                                                                */
/* Module  : TASKSCHEDULER.C */
/* Purpose : This module implements a simple but efficient cooperative
 * multitasking scheduler.    */
/*           This allows multiple engine instances (e.g. one per game window) to
 * execute in       */
/*           parallel, but also allows other time consuming processes (i.e.
 * database searching)   */
/*           to be executed as separate tasks. The task scheduler is designed to
 * run the main     */
/*           application event loop (as a separate task; the "Main Task"). */
/*           Except for the "Main Task", all other tasks execute in their own
 * stack area.         */
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

#include "TaskScheduler.h"

// NOTE: If static then RTOC direct, else indirect

static TASK *TaskTab;     // Pointer to master task table
static INT taskTabCount;  // Maximum number of tasks

static BOOL schedulerRunning = false;

static INT taskCount = 0;  // Number of active tasks
static TASK *currTask = nil;
static TASK *mainTask = nil;

/**************************************************************************************************/
/*                                                                                                */
/*                                      RESET TASK SCHEDULER */
/*                                                                                                */
/**************************************************************************************************/

asm void Task_Begin(register INT count) {
  sth r3,
      taskTabCount(RTOC)  // taskTabCount = count;
      mulli r4,
      r3,
      sizeof(TASK)  // SP -= count*sizeof(TASK);
      subf SP,
      r4, SP stw SP,
      TaskTab(RTOC)  // TaskTab = SP;
      addi SP,
      SP,
      -128  // SP -= 128; // Because the compiler screws up the stack
      blr
} /* Task_Begin */

asm void Task_End(void) {
  addi SP, SP,
      128  // SP += 128; // Because the compiler screws up the stack
      lhz r3,
      taskTabCount(RTOC) mulli r4, r3,
      sizeof(TASK)  // SP += count*sizeof(TASK);
      add SP,
      r4, SP blr
} /* Task_End */

/**************************************************************************************************/
/*                                                                                                */
/*                              RUN TASK SCHEDULER - CREATE MAIN TASK */
/*                                                                                                */
/**************************************************************************************************/

// The "Task_RunScheduler()" routine runs the application main loop, where it
// fetches and dispatches operating system events and schedules the application
// tasks (cooperatively). The routine returns when the the Main Task finishes
// (i.e. just before the application quits).

void Task_RunScheduler(TASKFUNC MainFunc, PTR data, INT priority) {
  // First reset task table
  for (INT i = 0; i < taskTabCount; i++) {
    TaskTab[i].id = i;
    TaskTab[i].active = false;
  }

  // First allocate main task (return if error):
  TASK *T = &TaskTab[0];

  // Initialize main task state;
  T->id = 0;
  T->active = true;
  T->paused = false;
  T->priority = priority;
  T->sleepTime = Timer() + priority;
  T->data = data;

  T->lr = nil;
  T->sp = nil;
  for (INT i = 0; i < 19; i++) T->gpr[i] = 0;

  // Add it to the task queue as the current task:
  T->next = T;
  T->prev = T;
  currTask = T;
  taskCount = 1;

  // Start the task scheduler by running the main task:
  schedulerRunning = true;

  (*MainFunc)(data);

  // Clean up when the main task function completes (kill all other tasks). At
  // this point, the main task is also the current task, and therefore we may
  // safely kill the next task recursively.
  while (taskCount > 1) Task_Kill(currTask->next->id);

  schedulerRunning = false;

  // Finally remove and deallocate main task:
  T->active = false;
  taskCount = 0;

  currTask = nil;
} /* Task_RunScheduler */

INT Task_GetCurrent(void) {
  if (!schedulerRunning || !currTask) return 0;
  return currTask->id;
} /* Task_GetCurrent */

INT Task_GetCount(void) { return taskCount; } /* Task_GetCount */

/**************************************************************************************************/
/*                                                                                                */
/*                                         CREATE/DESTROY TASKS */
/*                                                                                                */
/**************************************************************************************************/

/*--------------------------------- Starting/Pausing/Stopping Tasks
 * ------------------------------*/

INT Task_Create(TASKFUNC func, PTR data, INT priority) {
  if (!schedulerRunning) return 0;

  INT id;
  for (id = 1; id < taskTabCount && TaskTab[id].active; id++)
    ;
  if (id == taskTabCount) return 0;

  TASK *T = &TaskTab[id];

  T->active = true;
  T->paused = false;
  T->priority = priority;
  T->sleepTime = Timer() + priority;
  T->data = data;

  // Set processor state, so that the task function is automatically started
  // from the begining (with the "data" parameter) the first time it's scheduled
  // to run.
  T->lr = (PTR)(*func)((void *)nil);
  T->sp = (PTR)(
      &T->Stack[(taskStackSize - 128) /
                sizeof(ULONG)]);  // Add some extra space to please compiler

  for (LONG j = 0; j < taskStackSize / sizeof(ULONG); j++)  //###
    T->Stack[j] = 0xFFFFFFFF;

  // Add to end of task queue:
  T->next = currTask;
  T->prev = currTask->prev;

  currTask->prev->next = T;
  currTask->prev = T;

  taskCount++;

  return id;
} /* Task_Create */

void Task_Kill(INT id)  // The current task may not kill itself.
{
  if (!schedulerRunning || id < 1 || id >= taskTabCount) return;

  TASK *T = &TaskTab[id];

  if (T == currTask || !T->active) return;

  T->active = false;
  T->prev->next = T->next;
  T->next->prev = T->prev;
  taskCount--;
} /* Task_Kill */

/**************************************************************************************************/
/*                                                                                                */
/*                                         TASK SWITCHING */
/*                                                                                                */
/**************************************************************************************************/

// Must be called periodically by each of the tasks in order to allow task
// switching (and system event handling in the Main Task).

asm void Task_Switch(void) {
#define T0 r4
#define T r5
#define link r6

  lwz T0, currTask(RTOC) lwz T, TASK.next(T0) cmp cr0, 0, T0,
      T beqlr -

          // Save state of current task:
          stmw r13,
      TASK.gpr(T0) mflr link stw SP, TASK.sp(T0) stw link,
      TASK.lr(T0)

      // ... and switch to the new task
      stw T,
      currTask(RTOC) @S lwz link, TASK.lr(T) lmw r13, TASK.gpr(T) lwz SP,
      TASK.sp(T) lwz r3,
      TASK.data(T) mtlr link
          blrl  // Link after branch so task func returns here when it finishes

              // If/when the new current task function terminates normally, it
              // returns here (unless it's the main task, which exits properly
              // where it was called in Task_RunScheduler).

                  lwz T0,
      currTask(RTOC) lwz T, TASK.next(T0) stw T, currTask(RTOC) lhz r3,
      TASK.id(T0) call_c(Task_Kill) lwz T, currTask(RTOC) b @S

#undef T0
#undef T
#undef link
} /* Task_Switch */
