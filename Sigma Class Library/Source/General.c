/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this list of conditions 
  and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include "General.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                      STANDARD MATH FUNCTIONS                                   */
/*                                                                                                */
/**************************************************************************************************/

INT Abs (INT x)
{
   return (x < 0 ? -x : x);
} /* Abs */


LONG AbsL (LONG x)
{
   return (x < 0 ? -x : x);
} /* AbsL */


INT Sign (INT x)
{
   if (x > 0) return 1;
   if (x < 0) return -1;
   return 0;
} /* Sign */


LONG SignL (LONG x)
{
   if (x > 0) return 1;
   if (x < 0) return -1;
   return 0;
} /* SignL */


INT Min (INT x, INT y)
{
   return (x < y ? x : y);
} /* Min */


LONG MinL (LONG x, LONG y)
{
   return (x < y ? x : y);
} /* MinL */


INT Max (INT x, INT y)
{
   return (x > y ? x : y);
} /* Max */


LONG MaxL (LONG x, LONG y)
{
   return (x > y ? x : y);
} /* MaxL */


void Swap (INT *x, INT *y)
{
   INT t;

   t = *x;  *x = *y;  *y = t;
} /* Swap */


void SwapL (LONG *x, LONG *y)
{
   LONG t;

   t = *x;  *x = *y;  *y = t;
} /* SwapL */


INT Sqr (INT x)
{
   return x*x;
} /* Sqr */


INT Rand (INT n)															// Returns a random integer number
{																				// in the interval [0..n-1].
   return Abs(Random()) % n;
} /* Rand */


void ClearBlock (PTR block, ULONG size)
{
   for (ULONG i = 0; i < size; i++)
      block[i] = 0;
} /* ClearBlock */


/**************************************************************************************************/
/*                                                                                                */
/*                                    STANDARD STRING/CHAR FUNCTIONS                              */
/*                                                                                                */
/**************************************************************************************************/

void CopyStr (CHAR *s, CHAR *t)
{
   while (*(t++) = *(s++));
}   /* CopyStr */


void WriteBufStr (CHAR **buf, CHAR *str)  // Returns end of buf after "str" has been written
{
   while (*str) *((*buf)++) = *(str++);
   **buf = 0;
}  /* WriteBufStr */


void WriteBufNum (CHAR **buf, LONG num)  // Returns end of buf after "str" has been written
{
   CHAR numStr[11];
   ::NumToStr(num, numStr);
   ::WriteBufStr(buf, numStr);
}  /* WriteBufNum */


void CopySubStr (CHAR *s, INT count, CHAR *t)
{
   while (count-- && *s)
      *(t++) = *(s++);
   *t = 0;
}   /* CopySubStr */


void AppendStr (CHAR *s1, CHAR *s2, CHAR *t)
{
   while (*(t++) = *(s1++));
   t--;
   while (*(t++) = *(s2++));
} /* AppendStr */


BOOL EqualStr (CHAR *s1, CHAR *s2)
{
   while (*s1 && *s2)
      if (*(s1++) != *(s2++)) return false;
   return (!*s1 && !*s2);
} /* EqualStr */


BOOL EqualFrontStr (CHAR *source, CHAR *front)   // Checks if "source" starts with "front"
{
   while (*source && *front)
      if (*(source++) != *(front++)) return false;
   return (! *front);
} /* EqualFrontStr */


BOOL SameStr (CHAR *s1, CHAR *s2)
{
   while (*s1 && *s2)
      if (! SameChar(*(s1++), *(s2++))) return false;
   return (!*s1 && !*s2);
}   /* SameStr */


BOOL SameChar (CHAR c1, CHAR c2)      // Case insensitive character match.
{
   if (c1 >= 'a' && c1 <= 'z') c1 += 'A' - 'a';
   if (c2 >= 'a' && c2 <= 'z') c2 += 'A' - 'a';
   return (c1 == c2);
}   /* SameChar */


BOOL SearchChar (CHAR c, CHAR *s)
{
   while (*s)
      if (c == *s) return true; else s++;
   return false;
} /* SearchChar */


INT CompareStr (CHAR *s1, CHAR *s2, BOOL caseSensitive)
{
   CHAR c1, c2;
   do 
   {  c1 = *(s1++); if (!caseSensitive && c1 >= 'a' && c1 <= 'z') c1 += 'A' - 'a';
      c2 = *(s2++); if (!caseSensitive && c2 >= 'a' && c2 <= 'z') c2 += 'A' - 'a';
      if (c1 < c2) return -1;
      if (c1 > c2) return +1;
   } while (c1 && c2);
   return 0;
} /* CompareStr */


// Search for "sub" in "s". Returns true if found (and sets "pos" to the location)

BOOL SearchStr (CHAR *s, CHAR *sub, BOOL caseSensitive, INT *pos)
{
   for (INT i = 0; s[i]; i++)
   {
      BOOL failure = false;

      for (INT j = 0; sub[j] && !failure; j++)
         if (! s[i + j])
            failure = true;
         else
         {  CHAR c1 = s[i + j]; 
            CHAR c2 = sub[j];
            if (!caseSensitive && c1 >= 'a' && c1 <= 'z') c1 += 'A' - 'a';
            if (!caseSensitive && c2 >= 'a' && c2 <= 'z') c2 += 'A' - 'a';
            if (c1 != c2) failure = true;
         }

      if (! failure)
      {
         if (pos) *pos = i;
         return true;
      }
   }

   return false;
} /* SearchStr */


INT StrLen (CHAR *s)
{
   INT n = 0;

   while (s[n]) n++;
   return n;
} /* StrLen */


void NumToStr (LONG n, CHAR *s)
{
   CHAR t[12];
   INT  i;

   if (n == 0)     *(s++) = '0';
   else if (n < 0) *(s++) = '-', n = -n;

   for (i = 0; i <= 9 && n > 0; i++)
      t[i] = n % 10 + '0', n /= 10;
   while (--i >= 0)
      *(s++) = t[i];
   *(s++) = 0;                    // Null terminate
}  /* NumToStr */


BOOL StrToNum (CHAR *s, LONG *n)
{
   LONG N = 0;
   BOOL positive = true;

   if (*s == '-') positive = false, s++;       // First calc sign
   else if (*s == '+')  s++;
   if (! *s) return false;                     // Exit if empty string

   while (*s && IsDigit(*s))
      N = 10*N + (*s - '0'), s++;

   if (*s) return false;
   *n = (positive ? N : -N);
   return true;
} /* StrToNum */


INT FrontStrNum (CHAR *s, LONG *n)             // If front part of string is a number this is returned
{                                              // along with number of chars read
   LONG N = 0;
   BOOL positive = true;
   CHAR *s0 = s;

   if (*s == '-') positive = false, s++;       // First calc sign
   else if (*s == '+')  s++;
   if (! *s) return 0;                         // Exit if empty string

   while (*s && IsDigit(*s))
      N = 10*N + (*s - '0'), s++;

   *n = (positive ? N : -N);
   return s - s0;
} /* FrontStrNum */


BOOL IsDigit (CHAR c) 
{
   return (c >= '0' && c <= '9');
} /* IsDigit */


BOOL IsLetter (CHAR c)
{
   return (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z');
} /* IsLetter */


BOOL IsAlphaNum (CHAR c)
{
   return IsLetter(c) || IsDigit(c);
} /* IsAlphaNum */


BOOL IsNewLine (CHAR c) 
{
   return (c == 0x0A || c == 0x0D);
} /* IsNewLine */


BOOL IsTabChar (CHAR c) 
{
   return (c == '\t');
} /* IsTabChar */


void ReadLine (PTR data, ULONG bytes, ULONG *n, ULONG nmax, CHAR *s)
{
   ULONG i;
   for (i = *n; i < *n + nmax - 1 && ! IsNewLine(data[i]) && *n < bytes; i++)
      *(s++) = data[i];
   *s = 0;

   if (IsNewLine(data[i])) i++;
   *n = i;
} /* ReadLine */

/*------------------------------------ Pascal String Handling ------------------------------------*/

void C2P_Str (CHAR *cs, Str255 ps)
{
   INT n;

   for (n = 0; n <= 255 && cs[n]; n++)
      ps[n + 1] = cs[n];
   ps[0] = n;
} /* C2P_Str */


void P2C_Str (Str255 ps, CHAR *cs)
{
   INT len = ps[0];

   for (INT n = 1; n <= len; n++)
      cs[n - 1] = ps[n];
   cs[len] = 0;
} /* P2C_Str */


/**************************************************************************************************/
/*                                                                                                */
/*                                           MISCELLANEOUS                                        */
/*                                                                                                */
/**************************************************************************************************/

void AdjustColorLightness (RGB_COLOR *color, INT pct)
{
   pct += 100;
   color->red   = MinL(0x0000FFFF, (pct*((ULONG)color->red))/100);
   color->green = MinL(0x0000FFFF, (pct*((ULONG)color->green))/100);
   color->blue  = MinL(0x0000FFFF, (pct*((ULONG)color->blue))/100);
} /* AdjustColorLightness */


void GetDateStr (CHAR str[])  // Returns a date in the format: YYYY.MM.DD
{
   DateTimeRec dr;
   ::GetTime(&dr);
   Format(str, "%4d.%d%d.%d%d", dr.year, dr.month/10,dr.month%10, dr.day/10,dr.day%10);
} /* GetDateStr */
