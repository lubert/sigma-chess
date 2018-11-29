/**************************************************************************************************/
/*                                                                                                */
/* Module  : CMEMORY.C */
/* Purpose : Implements the dynamic memory allocation API. */
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

#include "CMemory.h"
#include "CApplication.h"

/*--------------------------------------- Direct Pointers
 * ----------------------------------------*/

PTR Mem_AllocPtr(ULONG size) { return (PTR)NewPtr(size); } /* Mem_AllocPtr */

void Mem_FreePtr(void *ptr) {
  if (ptr) DisposePtr((Ptr)ptr);
} /* Mem_FreePtr */

BOOL Mem_SetPtrSize(PTR ptr, ULONG newSize) {
  SetPtrSize((Ptr)ptr, newSize);
  return (::MemError() == noErr);
} /* Mem_SetPtrSize */

/*------------------------------------------- Handles
 * --------------------------------------------*/

HANDLE Mem_AllocHandle(ULONG size) {
  return (HANDLE)NewHandle(size);
} /* Mem_AllocHandle */

void Mem_FreeHandle(HANDLE h) { DisposeHandle((Handle)h); } /* Mem_FreeHandle */

void Mem_LockHandle(HANDLE h) { HLock((Handle)h); } /* Mem_LockHandle */

void Mem_UnlockHandle(HANDLE h) { HUnlock((Handle)h); } /* Mem_UnlockHandle */

/*-------------------------------------------- Misc
 * ----------------------------------------------*/

void Mem_Move(PTR from, PTR to, ULONG bytes) {
  BlockMove((Ptr)from, (Ptr)to, bytes);
} /* Mem_Move */

ULONG Mem_PhysicalRAM(void) {
  long memSize = 0;
  OSErr err = ::Gestalt(gestaltPhysicalRAMSize, &memSize);
  return memSize;
} /* Mem_MaxBlockSize */

ULONG Mem_MaxBlockSize(void)  // <-- Should NOT be used under OS X
{
  if (RunningOSX()) return 10L * 1024L * 1024L;  // Just a dummy value
  return (ULONG)::MaxBlock();
} /* Mem_MaxBlockSize */

ULONG Mem_FreeBytes(void)  // <-- Should NOT be used under OS X
{
  if (RunningOSX()) return 10L * 1024L * 1024L;  // Just a dummy value
  return (ULONG)::FreeMem();
} /* Mem_FreeBytes */
