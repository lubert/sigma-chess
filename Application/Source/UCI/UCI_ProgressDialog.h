#pragma once

#include "UCI.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                       TYPE/CLASS DEFINITIONS                                   */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES                                      */
/*                                                                                                */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES                                   */
/*                                                                                                */
/**************************************************************************************************/

void UCI_ProgressDialogOpen (CHAR *title, CHAR *message, BOOL withCancelButton = false, INT timeOutSecs = 10);
void UCI_ProgressDialogClose (void);
BOOL UCI_ProgressDialogCancelled (void);  // Has the user cancelled it?
void UCI_ProgressDialogResetTimeOut (INT timeOutSecs);
