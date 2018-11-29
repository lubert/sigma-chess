#pragma once

#include "General.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                       CONSTANTS & MACROS                                       */
/*                                                                                                */
/**************************************************************************************************/

#define uci_NullEngineId     -1
#define uci_SigmaEngineId     0

#define uci_MaxEngineCount   20

#define uci_MaxCmdLen      1024

#define uci_NameLen          32
#define uci_AuthorLen        32
#define uci_EnginePathLen   256
#define uci_AddressLen      128

#define uci_UserNameLen      40
#define uci_UserRegCodeLen   60

#define uci_NullOptionId     -1

#define uci_OptionNameLen    31
#define uci_OptionTypeLen     6
#define uci_MaxOptionCount   50
#define uci_ComboNameLen     31   // Max length of combo item
#define uci_ComboListLen     16   // Max number of entries in a combo box
#define uci_StringOptionLen 512   // Max length of string item

#define uci_NalimovPathLen  uci_StringOptionLen

#define uci_MaxHashSizeLite  64

#define uci_MaxMultiPVcount   8

enum UCI_OPTION_TYPE
{
   uciOption_None   = 0,

   uciOption_Check  = 1,
   uciOption_Spin   = 2,
   uciOption_Combo  = 3,
   uciOption_Button = 4,
   uciOption_String = 5,

   uci_OptionTypeCount = 5
};


//--- Names of fixed options ---
#define uciOptionName_Hash                 "Hash"
#define uciOptionName_NalimovPath          "NalimovPath"
#define uciOptionName_NalimovCache         "NalimovCache"
#define uciOptionName_Ponder               "Ponder"
#define uciOptionName_OwnBook              "OwnBook"
#define uciOptionName_MultiPV              "MultiPV"
#define uciOptionName_UCI_ShowCurrLine     "UCI_ShowCurrLine"
#define uciOptionName_UCI_ShowRefutations  "UCI_ShowRefutations"
#define uciOptionName_UCI_LimitStrength    "UCI_LimitStrength"
#define uciOptionName_UCI_Elo              "UCI_Elo"
#define uciOptionName_UCI_AnalyseMode      "UCI_AnalyseMode"
#define uciOptionName_UCI_Opponent         "UCI_Opponent"
#define uciOptionName_UCI_EngineAbout      "UCI_EngineAbout"
//#define uciOptionName_UCI_LicenseInfo      "UCI_LicenseInfo"
//#define uciOptionName_UCI_RegistrationInfo "UCI_RegistrationInfo"


/**************************************************************************************************/
/*                                                                                                */
/*                                       TYPE/CLASS DEFINITIONS                                   */
/*                                                                                                */
/**************************************************************************************************/

typedef INT UCI_ENGINE_ID;   // Index in UCI_PREFS.Engine[]
