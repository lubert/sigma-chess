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
#include "CWindow.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define licenseOwnerNameLen  30
#define licenseSerialNoLen    6
#define licenseKeyLen        14


/**************************************************************************************************/
/*                                                                                                */
/*                                        TYPE/CLASS DEFINITIONS                                  */
/*                                                                                                */
/**************************************************************************************************/

typedef struct
{
   INT   mainVersion;                          // 5, 6, ...
   BOOL  wasJustUpgraded;
   BOOL  pro;                                  // Is this the pro version?
   CHAR  ownerName[licenseOwnerNameLen + 1];   // Only 'A'..'Z', 'a'..'z', '-', '.', ',', ' '
   CHAR  serialNo[licenseSerialNoLen + 1];     // 5xxxxx
   CHAR  licenseKey[licenseKeyLen + 1];        // License key
} LICENSE;


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

void ResetLicense (LICENSE *L);
void VerifyLicense (void);
BOOL ProVersion (void);

void SigmaLicenseDialog (void);
void SigmaRegisterDialog (void);
void SigmaUpgradeDialog (void);

BOOL ProVersionDialog (CWindow *parent = nil, CHAR *prompt = nil);
