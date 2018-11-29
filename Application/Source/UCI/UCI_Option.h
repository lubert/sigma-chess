#pragma once

#include "UCI.h"

/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                       TYPE/CLASS DEFINITIONS */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL VARIABLES */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/*                                          FUNCTION PROTOTYPES */
/*                                                                                                */
/**************************************************************************************************/

// Set Fixed Options
void UCI_SetNalimovPathOption(UCI_ENGINE_ID engineId, CHAR *NalimovPath);
void UCI_SetPonderOption(UCI_ENGINE_ID engineId,
                         BOOL ponder);  // Only sent if changed or flush
void UCI_SetStrengthOption(UCI_ENGINE_ID engineId, BOOL limitStrength,
                           INT engineELO);  // Only sent if changed or flush

// Check Support for Fixed Options
BOOL UCI_SupportsPonderOption(UCI_ENGINE_ID engineId);
BOOL UCI_SupportsStrengthOption(UCI_ENGINE_ID engineId);
INT UCI_GetMultiPVOptionId(
    UCI_ENGINE_ID engineId);  // Returns -1 if not supported

// Creating Options
void UCI_CreateCheckOption(UCI_OPTION *Option, BOOL val);
void UCI_CreateSpinOption(UCI_OPTION *Option, LONG def, LONG min, LONG max);

// Setting Options
void UCI_SetCheckOption(UCI_ENGINE_ID engineId, CHAR *name, BOOL value);
void UCI_SetSpinOption(UCI_ENGINE_ID engineId, CHAR *name, INT value);
void UCI_SetComboOption(UCI_ENGINE_ID engineId, CHAR *name, CHAR *value);
void UCI_SetStringOption(UCI_ENGINE_ID engineId, CHAR *name, CHAR *value);
void UCI_SetDefaultOptions(UCI_ENGINE_ID engineId);

// Sending Options to engine
BOOL UCI_SendAllOptions(UCI_ENGINE_ID engineId, BOOL skipIfDefault = true);
void UCI_SendOption(UCI_ENGINE_ID engineId, UCI_OPTION *Option);

// Misc
UCI_OPTION *UCI_LookupOption(UCI_ENGINE_ID engineId, CHAR *name);
BOOL UCI_IsDefaultOption(UCI_OPTION *Option);
void UCI_OptionValueToStr(UCI_OPTION *Option,
                          CHAR *str);         // For display purposes only
BOOL UCI_OptionUnitIsMB(UCI_OPTION *Option);  // For display purposes only
