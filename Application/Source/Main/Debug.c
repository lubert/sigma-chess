/*
Copyright (c) 2011, Ole K. Christensen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

� Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

� Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Debug.h"
#include "CApplication.h"

BOOL debugOn = false;

void DebugAutoEnable(void) {
  debugOn = true;
  DebugCreate();
}

/**************************************************************************************************/
/*                                                                                                */
/*                                            DEBUG WINDOW */
/*                                                                                                */
/**************************************************************************************************/

static CFile *debugFile = nil;

CHAR debugStr[500];

class DebugWindow : public CTextWindow {
 public:
  DebugWindow(CRect frame);
  virtual ~DebugWindow(void);
};

static DebugWindow *debugWin = nil;

DebugWindow::DebugWindow(CRect frame) : CTextWindow("Debug Window", frame) {
  debugWin = this;
  Show(true);
  SetFront();
} /* DebugWindow::DebugWindow */

DebugWindow::~DebugWindow(void) {
  debugWin = nil;
} /* DebugWindow::DebugWindow */

void DebugCreate(void) {
  CRect frame(0, 0, 80 * 6 + 10, textWinLines * 11 + 10);
  frame.Offset(10, 45);
  debugWin = new DebugWindow(&frame);
} /* DebugCreate */

void DebugWrite(CHAR *s) {
  if (!debugFile) {
    debugFile = new CFile();
    //    if (debugFile->Set("Sigma.log", 'TEXT', 'ttxt', filePath_Logs) !=
    //    fileError_NoError)
    if (debugFile->Set("Sigma.log", 'TEXT', 'ttxt', filePath_Documents) !=
        fileError_NoError)
      debugFile->Set("Sigma.log", 'TEXT', 'ttxt');
    debugFile->Delete();
    debugFile->Create();
  }

  if (debugFile) debugFile->AppendStr(s);

  if (debugWin) debugWin->DrawStr(s);
} /* DebugWrite */

void DebugWriteNL(CHAR *s) {
  DebugWrite(s);
  DebugWrite("\n");
} /* DebugWriteNL */
