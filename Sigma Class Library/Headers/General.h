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

#include <stdio.h>
#include <string.h>


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#ifndef true
   #define true 1
#endif
#ifndef false
   #define false 0
#endif

#ifndef nil
   #define nil 0L
#endif

#define maxint         0x7FFF
#define maxlong        0x7FFFFFFF

#define bit(i)         (1 << i)
#define bitL(i)        (1L << i)
#define clrBit(i,a)    (a &= ~bit(i))
#define even(x)        (((x) & 1) == 0)
#define odd(x)         ((x) & 1)

#define Format         sprintf
#define Timer()        TickCount()
#define MicroSecs(m)   Microseconds((UnsignedWide*)m)

#define call_c(Proc)  mflr r0 ; stwu r0,-12(SP) ; bl Proc ; lwz r0, 0(SP) ; addi SP, SP,12 ; mtlr r0


/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

typedef short          INT;
typedef long           LONG;
typedef short          BOOL;
typedef char           CHAR;
typedef unsigned char  BYTE;
typedef unsigned char  *PTR;
typedef double         REAL;
typedef unsigned short UINT;
typedef unsigned long  ULONG;
//typedef double         LONG64;
//typedef double         ULONG64;
typedef SInt64         LONG64;
typedef UInt64         ULONG64;
typedef PTR            *HANDLE;
typedef RGBColor       RGB_COLOR;


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

INT   Abs        (INT x);
LONG  AbsL       (LONG x);
INT   Sign       (INT x);
LONG  SignL      (LONG x);
INT   Min        (INT x, INT y);
LONG  MinL       (LONG x, LONG y);
INT   Max        (INT x, INT y);
LONG  MaxL       (LONG x, LONG y);
void  Swap       (INT *x, INT *y);
void  SwapL      (LONG *x, LONG *y);
INT   Sqr        (INT x);
INT   Rand       (INT n);

void  ClearBlock (PTR block, ULONG size);

void  CopyStr    (CHAR *s, CHAR *t);
void  CopySubStr (CHAR *s, INT count, CHAR *t);
void  WriteBufStr (CHAR **buf, CHAR *str);
void  WriteBufNum (CHAR **buf, LONG num);
void  AppendStr  (CHAR *s1, CHAR *s2, CHAR *t);
BOOL  EqualStr   (CHAR *s1, CHAR *s2);
BOOL  EqualFrontStr (CHAR *source, CHAR *front);
BOOL  SameStr    (CHAR *s1, CHAR *s2);
BOOL  SameChar   (CHAR c1, CHAR c2);
BOOL  SearchChar (CHAR c, CHAR *s);
INT   CompareStr (CHAR *s1, CHAR *s2, BOOL caseSensitive = true);
BOOL  SearchStr  (CHAR *s, CHAR *sub, BOOL caseSensitive = true, INT *pos = nil);
INT   StrLen     (CHAR *s);
void  NumToStr   (LONG n, CHAR *s);
INT   FrontStrNum (CHAR *s, LONG *n);
BOOL  StrToNum   (CHAR *s, LONG *n);

BOOL  IsDigit    (CHAR c);
BOOL  IsLetter   (CHAR c);
BOOL  IsAlphaNum (CHAR c);
BOOL  IsNewLine  (CHAR c);
BOOL  IsTabChar  (CHAR c);

void  ReadLine (PTR data, ULONG bytes, ULONG *n, ULONG nmax, CHAR *s);

void  C2P_Str (CHAR *cs, Str255 ps);
void  P2C_Str (Str255 ps, CHAR *cs);

void  AdjustColorLightness (RGB_COLOR *color, INT pct);
void  GetDateStr (CHAR str[]);
