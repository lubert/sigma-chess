/**************************************************************************************************/
/*                                                                                                */
/* Module  : CPRINT.C                                                                             */
/* Purpose : Implements a printer interface class.                                                */
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

#include "CPrint.h"
#include "CApplication.h"

// The CPrint object defines the interface to the printing routines. There can be multiple CPrint
// objects, but at any time at most one can be "active" (i.e. performing the print job).
//
// When a new job is started, a special "CView" object (called "page") is created through
// which the actual print actions are sent.


/**************************************************************************************************/
/*                                                                                                */
/*                                          GLOBAL STUFF                                          */
/*                                                                                                */
/**************************************************************************************************/

static PMPrintSession  printSession  = kPMNoReference;
static PMPageFormat    pageFormat    = kPMNoPageFormat;
static PMPrintSettings printSettings = kPMNoPrintSettings;

/*------------------------------------------- Startup --------------------------------------------*/

void Print_Init (void)
{
   pageFormat = kPMNoPageFormat;
   printSettings = kPMNoPrintSettings;
} /* Print_Init */

/*----------------------------------------- Termination ------------------------------------------*/

void Print_End (void)
{
   if (pageFormat != kPMNoPageFormat) ::PMRelease(pageFormat);
   if (printSettings != kPMNoPrintSettings) ::PMRelease(printSettings);
} /* Print_End */

/*-------------------------------------- Page Setup Dialog ---------------------------------------*/
// The Page Setup/Format dialog is a generic dialog that is not specific to any printer object.
// Therefore it is not part of the CPrint class.

BOOL Print_PageSetupDialog (void)
{
   Boolean accepted;

   ::PMCreateSession(&printSession);
   if (::OSError(::PMSessionError(printSession))) return false;

   // Set up a valid PageFormat object.
   if (pageFormat != kPMNoPageFormat)
      ::PMSessionValidatePageFormat(printSession, pageFormat, kPMDontWantBoolean);
   else if ((::PMCreatePageFormat(&pageFormat) == noErr) && (pageFormat != kPMNoPageFormat))
      ::PMSessionDefaultPageFormat(printSession, pageFormat);

   // Display the Page Setup dialog.   
   if (::PMSessionError(printSession) == noErr)
      ::PMSessionPageSetupDialog(printSession, pageFormat, &accepted);

   ::PMRelease(printSession);

   return (::PMSessionError(printSession) == noErr && accepted);
} /* Ptint_PageSetupDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                      CONSTRUCTOR/DESTRUCTOR                                    */
/*                                                                                                */
/**************************************************************************************************/

CPrint::CPrint (void)
  : CViewOwner(viewOwner_Print)
{
   jobStarted = false;
   pageOpen   = false;
   printPort  = nil;
} /* CPrint::CPrint */


CPrint::~CPrint (void)
{
   if (pageOpen) ClosePage();
   if (jobStarted) EndJob();
} /* CPrint::CPrint */


/**************************************************************************************************/
/*                                                                                                */
/*                                        START/END PRINT JOB                                     */
/*                                                                                                */
/**************************************************************************************************/

BOOL CPrint::StartJob (void)
{
   //--- First load printer driver for current printer ---

   ::PMCreateSession(&printSession);
   if (::OSError(::PMSessionError(printSession))) goto AbortPrintJob;

   //--- Then make sure we have initialized the page format record ---

	if (pageFormat != kPMNoPageFormat)
      ::PMSessionValidatePageFormat(printSession, pageFormat, kPMDontWantBoolean);
	else if (::PMCreatePageFormat(&pageFormat) == noErr && pageFormat != kPMNoPageFormat)
      ::PMSessionDefaultPageFormat(printSession, pageFormat);

   if (::OSError(::PMSessionError(printSession))) goto AbortPrintJob;

   //--- Validate the print settings record (in case of printer/page setup changes) ---

	if (printSettings != kPMNoPrintSettings)
      ::PMSessionValidatePrintSettings(printSession, printSettings, kPMDontWantBoolean);
   else if (::PMCreatePrintSettings(&printSettings) == noErr && printSettings != kPMNoPrintSettings)
      ::PMSessionDefaultPrintSettings(printSession, printSettings);

   if (::OSError(::PMSessionError(printSession))) goto AbortPrintJob;

   //--- Open print dialog ---

   theApp->ActivateFrontWindow(false);
//    ::PMSetPageRange(*printSettings, 1, realNumberOfPagesinDoc);
      Boolean accepted;
      ::PMSessionPrintDialog(printSession, printSettings, pageFormat, &accepted);
   theApp->ProcessSysEvents();
   theApp->ActivateFrontWindow(true);

   jobStarted = accepted;
   if (! accepted) goto AbortPrintJob;

   //--- Start the job (BeginDocument) ---

   if (jobStarted)
   {
      ::PMSessionBeginDocument(printSession, printSettings, pageFormat);
      if (::OSError(::PMSessionError(printSession)))
      {  jobStarted = false;
         goto AbortPrintJob;
      }
   }

   // Return true if user confirmed the print operation:
   return jobStarted;

AbortPrintJob:
   ::PMRelease(printSession);
   return false;
} /* CPrint::StartJob */


BOOL CPrint::EndJob (void)
{
   if (! jobStarted) return false;

   ::PMSessionEndDocument(printSession);
   if (::PMSessionError(printSession) != kDTPAbortJobErr)
      ::OSError(::PMSessionError(printSession));

   ::PMRelease(printSession);
   jobStarted = false;
   return true;
} /* CPrint::EndJob */


/**************************************************************************************************/
/*                                                                                                */
/*                                          OPEN/CLOSE PAGE                                       */
/*                                                                                                */
/**************************************************************************************************/

BOOL CPrint::OpenPage (void)
{
   if (pageOpen || ! jobStarted) return false;

   ::PMSessionBeginPage(printSession, pageFormat, NULL);
   ::PMSessionGetGraphicsContext(printSession, kPMGraphicsContextQuickdraw, (void**)&printPort);

   if (! ::OSError(::PMSessionError(printSession)))
   {
      forAllChildViews(subView)
         subView->DispatchRootPort(printPort);
      pageOpen = true;
   }
   return pageOpen;
} /* CPrint::OpenPage */


BOOL CPrint::ClosePage (void)
{
   if (! pageOpen) return false;

   ::PMSessionEndPage(printSession);

   pageOpen = false;
   return true;
} /* CPrint::ClosePage */


/**************************************************************************************************/
/*                                                                                                */
/*                                               MISC                                             */
/*                                                                                                */
/**************************************************************************************************/

CRect CPrint::PageFrame (void)
{
   PMRect mr;
	::PMGetAdjustedPageRect(pageFormat, &mr);
   CRect frame(mr.left, mr.top, mr.right, mr.bottom);
   return frame;
} /* CPrint::PageFrame */


INT CPrint::HRes (void)
{
   PMPrinter printer;
   PMResolution res;
   ::PMSessionGetCurrentPrinter(printSession, &printer);
   ::PMPrinterGetPrinterResolution(printer, kPMDefaultResolution, &res);
   return (::PMSessionError(printSession) == noErr ? res.hRes : 300);
} /* CPrint::HRes */


INT CPrint::VRes (void)
{
   PMPrinter printer;
   PMResolution res;
   ::PMSessionGetCurrentPrinter(printSession, &printer);
   ::PMPrinterGetPrinterResolution(printer, kPMDefaultResolution, &res);
   return (::PMSessionError(printSession) == noErr ? res.vRes : 300);
} /* CPrint::VRes */


INT CPrint::FirstPage (void)
{
   UInt32 firstPage = 1;
   ::PMGetFirstPage(printSettings, &firstPage);
   return firstPage;
} /* CPrint::FirstPage */


INT CPrint::LastPage (void)
{
   UInt32 lastPage = 1;
   ::PMGetLastPage(printSettings, &lastPage);
   return lastPage;
} /* CPrint::LastPage */


INT CPrint::NumCopies (void)
{
   UInt32 copies;
   ::PMGetCopies(printSettings, &copies);
   return copies;
} /* CPrint::NumCopies */


BOOL CPrint::Error (void)
{
   return (::PMSessionError(printSession) != noErr);
} /* CPrint::Error */


void CPrint::Abort (void)
{
   ::PMSessionSetError(printSession, kDTPAbortJobErr);
} /* CPrint::Abort */


/**************************************************************************************************/
/*                                                                                                */
/*                                          PRINT TEMPLATE                                        */
/*                                                                                                */
/**************************************************************************************************/

void CPrint::DoPrint (void)  // Should be overridden.
{
/*
   if (! StartJob()) return;

   for (INT n = 1; n <= pages; n++)
   {
      if (OpenPage())
      {
         ...Your code here
         ClosePage();
      }
   }

   EndJob();
*/
} /* CPrint::DoPrint */
