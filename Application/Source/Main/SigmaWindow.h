#pragma once

#include "CWindow.h"

#include "SigmaApplication.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

enum SIGMA_WINCLASS
{
   sigmaWin_Game       = 1,
   sigmaWin_Collection = 2,
   sigmaWin_Library    = 3
};

/**************************************************************************************************/
/*                                                                                                */
/*                                          TYPE DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                         CLASS DEFINITIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

class SigmaWindow: public CWindow
{
public:
   SigmaWindow (CHAR *title, CRect frame, SIGMA_WINCLASS winClass,  BOOL sizeable = true, CRect resizeLimit = CRect(100,100,maxint,maxint));

   SIGMA_WINCLASS winClass;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                         GLOBAL DATA STRUCTURES                                 */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/
