/**************************************************************************************************/
/*                                                                                                */
/* Module  : UCI_OPTION.C                                                                         */
/* Purpose : This module implements the interface to the UCI engines.                             */
/*                                                                                                */
/**************************************************************************************************/

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

#include "UCI_Option.h"
#include "SigmaPrefs.h"


/**************************************************************************************************/
/*                                                                                                */
/*                                           FIXED OPTIONS                                        */
/*                                                                                                */
/**************************************************************************************************/

/*-------------------------------------- Setting Fixed Options -----------------------------------*/

void UCI_SetNalimovPathOption (UCI_ENGINE_ID engineId, CHAR *NalimovPath)
{
   if (engineId == uci_SigmaEngineId || EqualStr(NalimovPath,"")) return;

   UCI_INFO *Info = &Prefs.UCI.Engine[engineId];
   if (! Info->supportsNalimovBases) return;

   UCI_OPTION *Option = &Info->NalimovPath;

   if (Option->type == uciOption_String && ! EqualStr(Option->u.String.val, NalimovPath))
   {  CopyStr(NalimovPath, Option->u.String.val);
      UCI_SendOption(engineId, Option);
   }
} // UCI_SetNalimovPathOption


void UCI_SetPonderOption (UCI_ENGINE_ID engineId, BOOL ponder)
{
   if (engineId == uci_SigmaEngineId) return;

   UCI_INFO *Info = &Prefs.UCI.Engine[engineId];
   if (! Info->supportsPonder) return;

   UCI_OPTION *Option = &Info->Ponder;

   if (Option->type == uciOption_Check && (Option->u.Check.val != ponder || Info->flushPonder))
   {
      Option->u.Check.val = ponder;
      UCI_SendOption(engineId, Option);
      Info->flushPonder = false;
   }
} // UCI_SetPonderOption


void UCI_SetStrengthOption (UCI_ENGINE_ID engineId, BOOL limitStrength, INT engineELO)
{
   if (engineId == uci_SigmaEngineId) return;

   UCI_INFO *Info = &Prefs.UCI.Engine[engineId];
   if (! Info->supportsLimitStrength) return;
   
   UCI_OPTION *OptionLimit = &Info->LimitStrength;
   UCI_OPTION *OptionELO   = &Info->UCI_Elo;

   if (OptionLimit->type != uciOption_Check || OptionELO->type != uciOption_Spin) return;

   if (! limitStrength) // If being turned off and currently on -> update
   {
      if (OptionLimit->u.Check.val || Info->flushELO)
      {
         OptionLimit->u.Check.val = false;
         UCI_SendOption(engineId, OptionLimit);
         Info->flushELO = false;
      }
   }
   else
   {
      // First clamp ELO to proper range (just in case)
      if (engineELO > OptionELO->u.Spin.max)
         engineELO = OptionELO->u.Spin.max;
      else if (engineELO < OptionELO->u.Spin.min)
         engineELO = OptionELO->u.Spin.min;

      // Then send if changed or flush
      if (! OptionLimit->u.Check.val || OptionELO->u.Spin.val != engineELO || Info->flushELO)
      {
         OptionLimit->u.Check.val = true;
         OptionELO->u.Spin.val = engineELO;
         UCI_SendOption(engineId, OptionLimit);
         UCI_SendOption(engineId, OptionELO);
         Info->flushELO = false;
      }
   }
} // UCI_SetStrengthOption

/*----------------------------------- Check Support for Fixed Options ----------------------------*/

BOOL UCI_SupportsPonderOption (UCI_ENGINE_ID engineId)
{
   return Prefs.UCI.Engine[engineId].supportsPonder;
} // UCI_SupportsPonderOption


BOOL UCI_SupportsStrengthOption (UCI_ENGINE_ID engineId)
{
   return Prefs.UCI.Engine[engineId].supportsLimitStrength;
} // UCI_SupportsStrengthOption


INT  UCI_GetMultiPVOptionId (UCI_ENGINE_ID engineId)
{
   if (engineId == uci_SigmaEngineId) return uci_NullOptionId;

   UCI_INFO *Info = &Prefs.UCI.Engine[engineId];

   for (INT i = 0; i < Info->optionCount; i++)
      if (EqualStr(uciOptionName_MultiPV, Info->Options[i].name))
         return i;
	
	return uci_NullOptionId;
} // UCI_GetMultiPVOptionId


/**************************************************************************************************/
/*                                                                                                */
/*                                          CREATING OPTIONS                                      */
/*                                                                                                */
/**************************************************************************************************/

void UCI_CreateCheckOption (UCI_OPTION *Option, BOOL def)
{
   Option->type = uciOption_Check;
   Option->u.Check.def = def;
   Option->u.Check.val = def;
} // UCI_CreateCheckOption


void UCI_CreateSpinOption (UCI_OPTION *Option, LONG def, LONG min, LONG max)
{
   Option->type = uciOption_Spin;
   Option->u.Spin.def = def;
   Option->u.Spin.val = def;
   Option->u.Spin.min = min;
   Option->u.Spin.max = max;
} // UCI_CreateCheckOption


/**************************************************************************************************/
/*                                                                                                */
/*                                          SETTING OPTIONS                                       */
/*                                                                                                */
/**************************************************************************************************/

void UCI_SetCheckOption (UCI_ENGINE_ID engineId, CHAR *name, BOOL value)
{
   if (engineId == uci_SigmaEngineId) return;

   UCI_OPTION *Option = UCI_LookupOption(UCISession[engineId].engineId, name);

   if (Option && Option->type == uciOption_Check)
   {  Option->u.Check.val = value;
      UCI_SendOption(engineId, Option);
   }
} // UCI_SetCheckOption


void UCI_SetSpinOption (UCI_ENGINE_ID engineId, CHAR *name, INT value)
{
   if (engineId == uci_SigmaEngineId) return;

   UCI_OPTION *Option = UCI_LookupOption(UCISession[engineId].engineId, name);

   if (Option && Option->type == uciOption_Spin &&
       Option->u.Spin.min <= value && value <= Option->u.Spin.max)
   {
      Option->u.Spin.val = value;
      UCI_SendOption(engineId, Option);
   }
} // UCI_SetSpinOption


void UCI_SetComboOption (UCI_ENGINE_ID engineId, CHAR *name, CHAR *value)
{
   if (engineId == uci_SigmaEngineId) return;

   UCI_OPTION *Option = UCI_LookupOption(UCISession[engineId].engineId, name);

   if (Option && Option->type == uciOption_Combo)
      for (INT i = 0; i < Option->u.Combo.count; i++)
         if (EqualStr(value, Option->u.Combo.List[i]))
         {  Option->u.Combo.val = i;
            UCI_SendOption(engineId, Option);
         }
} // UCI_SetComboOption


void UCI_SetStringOption (UCI_ENGINE_ID engineId, CHAR *name, CHAR *value)
{
   if (engineId == uci_SigmaEngineId) return;

   UCI_OPTION *Option = UCI_LookupOption(UCISession[engineId].engineId, name);

   if (Option && Option->type == uciOption_String)
   {
      CopySubStr(value, uci_StringOptionLen, Option->u.String.val);
      UCI_SendOption(engineId, Option);
   }
} // UCI_SetStringOption


void UCI_SetDefaultOptions (UCI_ENGINE_ID engineId)
{
   if (engineId == uci_SigmaEngineId) return;

   UCI_INFO *Engine = &Prefs.UCI.Engine[engineId];

   for (INT i = 0; i < Engine->optionCount; i++)
   {
      UCI_OPTION *Option = &Engine->Options[i];

      switch (Option->type)
      {
         case uciOption_Check  : Option->u.Check.val = Option->u.Check.def; break;
         case uciOption_Spin   : Option->u.Spin.val  = Option->u.Spin.def; break;
         case uciOption_Combo  : Option->u.Combo.val = Option->u.Combo.def; break;
         case uciOption_String : CopyStr(Option->u.String.def, Option->u.String.val); break;
         case uciOption_Button : break;
      }

      if (Option->type != uciOption_Button)
         UCI_SendOption(engineId, Option);
   }
} // UCI_SetDefaultOptions


/**************************************************************************************************/
/*                                                                                                */
/*                                      SEND OPTIONS TO ENGINE                                    */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_SendAllOptions (UCI_ENGINE_ID engineId, BOOL skipIfDefault)
{
   if (engineId == uci_SigmaEngineId || ! UCI_EngineLoaded(engineId)) return false;

   UCI_INFO *Engine = &Prefs.UCI.Engine[engineId];

   //--- Send non-fixed options ---
   for (INT i = 0; i < Engine->optionCount; i++)
   {
      UCI_OPTION *Option = &Engine->Options[i];
      if (! (skipIfDefault && UCI_IsDefaultOption(Option)))
         UCI_SendOption(engineId, Option);
   }

   //--- Send fixed options --
   if (Engine->supportsPonder)
      UCI_SendOption(engineId, &Engine->Ponder);  // Send even if default (cleaner & simpler!)

   if (Engine->supportsLimitStrength)
   {  UCI_SendOption(engineId, &Engine->LimitStrength);  // Send even if default (cleaner & simpler!)
      if (Engine->LimitStrength.u.Check.val)
         UCI_SendOption(engineId, &Engine->UCI_Elo);        // Send even if default (cleaner & simpler!)
   }

   if (Engine->supportsNalimovBases)
   {  CopyStr(Prefs.UCI.NalimovPath, Engine->NalimovPath.u.String.val);
      UCI_SendOption(engineId, &Engine->NalimovPath);  // Send even if default (cleaner & simpler!)
   }

   return UCI_WaitIsReady(engineId);
} // UCI_SendAllOptions


void UCI_SendOption (UCI_ENGINE_ID engineId, UCI_OPTION *Option)
{
   if (engineId == uci_SigmaEngineId || ! UCI_EngineLoaded(engineId)) return;

   CHAR value[uci_MaxCmdLen + 1];

   switch (Option->type)
   {
      case uciOption_Check  : Format(value, " value %s", (Option->u.Check.val ? "true" : "false")); break;
      case uciOption_Spin   : Format(value, " value %d", Option->u.Spin.val); break;
      case uciOption_Combo  : Format(value, " value %s", Option->u.Combo.List[Option->u.Combo.val]); break;
      case uciOption_Button : Format(value, ""); break;
      case uciOption_String : Format(value, " value %s", Option->u.String.val); break;
   }

   CHAR msg[uci_MaxCmdLen + 1];
   Format(msg, "setoption name %s%s", Option->name, value);
   UCI_SendCommand(engineId, msg);
} // UCI_SendOption


/**************************************************************************************************/
/*                                                                                                */
/*                                              MISC                                              */
/*                                                                                                */
/**************************************************************************************************/

BOOL UCI_IsDefaultOption (UCI_OPTION *Option)
{
   switch (Option->type)
   {
      case uciOption_Check  : return (Option->u.Check.def == Option->u.Check.val);
      case uciOption_Spin   : return (Option->u.Spin.def == Option->u.Spin.val);
      case uciOption_Combo  : return (Option->u.Combo.def == Option->u.Combo.val);
      case uciOption_Button : return true;
      case uciOption_String : return EqualStr(Option->u.String.def, Option->u.String.val);
   }

   return true;
} // UCI_IsDefaultOption


void UCI_OptionValueToStr (UCI_OPTION *Option, CHAR *str)
{
   switch (Option->type)
   {
      case uciOption_Check  : CopyStr((Option->u.Check.val ? "On" : "Off"), str); break;
      case uciOption_Spin   : Format(str, "%d%s", Option->u.Spin.val, (UCI_OptionUnitIsMB(Option) ? " MB" : "")); break;
      case uciOption_Combo  : CopyStr(Option->u.Combo.List[Option->u.Combo.val], str); break;
      case uciOption_Button : CopyStr("", str); break;
      case uciOption_String : CopyStr(Option->u.String.val, str); break;
   }
} // UCI_OptionValueToStr


BOOL UCI_OptionUnitIsMB (UCI_OPTION *Option)
{
   return (EqualStr(Option->name,uciOptionName_Hash) || EqualStr(Option->name,uciOptionName_NalimovCache));
} // UCI_OptionValIsMB


/**************************************************************************************************/
/*                                                                                                */
/*                                             UTILITY                                            */
/*                                                                                                */
/**************************************************************************************************/

UCI_OPTION *UCI_LookupOption (UCI_ENGINE_ID engineId, CHAR *name)
{
   if (engineId == uci_SigmaEngineId) return nil;

   UCI_INFO *Info = &Prefs.UCI.Engine[engineId];

   for (INT i = 0; i < Info->optionCount; i++)
      if (EqualStr(name, Info->Options[i].name))
         return &Info->Options[i];

   return nil;
} // UCI_LookupOption
