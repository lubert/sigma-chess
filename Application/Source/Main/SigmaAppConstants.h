#pragma once

#include "General.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------- Central Application Constants
 * ---------------------------------*/

#define sigmaCreator '·CHS'
#define sigmaAppName "Sigma Chess 6.2"

#define sigmaVersion_Main 0x06
#define sigmaVersion_Sub 0x21
#define sigmaVersion_Rel sigmaRelease_Final
#define sigmaVersion_Build 0x621

#define sigmaTaskCount 12
#define maxSigmaWindowCount 15

#define minTotalMem 2500   // At least 2.5 Mb needed to run Sigma Chess 6
#define minReserveMem 512  // At least 512 Kb mem in reserve for misc temp stuff

enum RELEASE {
  sigmaRelease_Dev = 0,  // Developement - not feature complete
  sigmaRelease_Alpha,    // Virtually feature complete, but filled with bugs!
  sigmaRelease_Beta,     // Feature complete, few bugs
  sigmaRelease_Final     // Feature complete, "no" bugs!
};
