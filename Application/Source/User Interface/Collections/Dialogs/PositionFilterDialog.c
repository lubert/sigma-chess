/**************************************************************************************************/
/*                                                                                                */
/* Module  : PositionFilterDialog.c                                                               */
/* Purpose : This module implements the collections filter dialogs.                               */
/*                                                                                                */
/**************************************************************************************************/

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

#include "PositionFilterDialog.h"
#include "BoardArea2D.h"
#include "SigmaApplication.h"
#include "SigmaStrings.h"


class CPosFilterDialog : public CDialog
{
public:
   CPosFilterDialog (CRect frame, POS_FILTER *pf);
   virtual ~CPosFilterDialog (void);

   virtual void HandleMessage (LONG msg, LONG submsg = 0, PTR data = nil);
   virtual void HandlePushButton (CPushButton *ctl);
   virtual void HandleRadioButton (CRadioButton *ctrl);

   POS_FILTER pf;  // Temporary filter

private:
   void CreatePalette (void);
   void CreateSideToMove (void);
   void CreateMoveRange (void);
   void CreateExactPartial (void);

   void Process_SideToMove (void);
   BOOL Process_MoveRange (void);
   BOOL Process_ExactPartial (void);
   BOOL Process_Board (void);

   void PastePosition (void);

   CGame *game;    // Utility game object
   CRect inner;

   // Board view
   CRect boardRect;
   BoardView *boardView;
   CPushButton *cbutton_ClearBoard, *cbutton_NewBoard, *cbutton_PasteBoard;

   // Piece/token palette
   CRect paletteRect;
   CButton *PieceButton[pieces];

   // Side to move
   CRect playerRect;
   CRadioButton *cradio_White, *cradio_Black, *cradio_Either;

   // Move Range
   CRect rangeRect;
   CRadioButton *cradio_EntireGame, *cradio_MoveRange;
   CEditControl *cedit_Mmin, *cedit_Mmax;

   // Exact/Partial (and Piece count)
   CRect exactPartialRect;
   CRadioButton *cradio_Exact, *cradio_Partial;
   CEditControl *cedit_Wmin, *cedit_Wmax;
   CEditControl *cedit_Bmin, *cedit_Bmax;

};

#define boardViewSize      (8*minSquareWidth + 2*BoardFrame_Width(minSquareWidth))
#define paletteWidth       (2*pieceButtonSize - 1)
#define paletteHeight      (8*pieceButtonSize - 1)
#define miscParamWidth     155
#define sepSize            15

#define dialogWidth        (20 + boardViewSize + sepSize + paletteWidth + sepSize + miscParamWidth + 20)
#define dialogHeight       (20 + boardViewSize + sepSize + 10 + controlHeight_PushButton + 20)


/**************************************************************************************************/
/*                                                                                                */
/*                                          RUN FILTER DIALOG                                     */
/*                                                                                                */
/**************************************************************************************************/

BOOL PositionFilterDialog (POS_FILTER *pf)
{
   CRect frame(0, 0, dialogWidth, dialogHeight);
   if (! RunningOSX()) frame.bottom -= 20;
   theApp->CentralizeRect(&frame, true);

   CPosFilterDialog *dialog = new CPosFilterDialog(frame, pf);
   dialog->Run();

   BOOL wasOK = (dialog->reply == cdialogReply_OK);
   if (wasOK)
   {  *pf = dialog->pf;
      PreparePosFilter(pf);
   }
   delete dialog;
   return wasOK;
} /* PositionFilterDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

/*----------------------------------------- Constructor ------------------------------------------*/

CPosFilterDialog::CPosFilterDialog (CRect frame, POS_FILTER *theFilter)
 : CDialog(nil, "Position Filter", frame)
{
   inner = InnerRect();

   // First load position filter "into" game object and enter "Edit" mode.
   pf = *theFilter;
   game = new CGame();
   game->Edit_Begin();
   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq))
         game->Edit_SetPiece(sq, pf.Pos[sq]);

   //--- Board view ---
   boardRect.Set(0,0,boardViewSize,boardViewSize);
   boardRect.Offset(inner.left, inner.top);
   boardView = new BoardView(this, boardRect, game);

   //--- Palette ---
   paletteRect.Set(0,0,paletteWidth,paletteHeight);
   paletteRect.Offset(boardRect.right + sepSize, inner.top);
   CreatePalette();

   //--- Global dividers
   CRect divRect(inner.right - miscParamWidth - 1, inner.top, inner.right - miscParamWidth + 1, boardRect.bottom);
   new CDivider(this, divRect);
   divRect = inner;
   divRect.top = boardRect.bottom + 12;
   divRect.bottom = divRect.top + 2;
   new CDivider(this, divRect);
   
   //--- Side to move ---
   playerRect.Set(0,0,miscParamWidth - 10,95);
   playerRect.Offset(inner.right - miscParamWidth + 10, inner.top);
   CreateSideToMove();
   playerRect.top = playerRect.bottom - 2;
   new CDivider(this, playerRect);

   //--- Move Range ---
   rangeRect = playerRect;
   rangeRect.top += 10;
   rangeRect.bottom = rangeRect.top + 105;
   CreateMoveRange();
   rangeRect.top = rangeRect.bottom - 2;
   new CDivider(this, rangeRect);

   //--- Piece count ---
   exactPartialRect = rangeRect;
   exactPartialRect.top += 10;
   exactPartialRect.bottom = boardRect.bottom;
   CreateExactPartial();

   //--- OK/Cancel buttons ---
   cbutton_Cancel  = new CPushButton(this, "Cancel",  CancelRect());
   cbutton_Default = new CPushButton(this, "OK",   DefaultRect());
   SetDefaultButton(cbutton_Default);
} /* CPosFilterDialog::CPosFilterDialog */


void CPosFilterDialog::CreatePalette (void)
{
   for (COLOUR player = white; player <= black; player += black)
      for (PIECE piece = king; piece >= pawn; piece--)
      {
         CRect dst(0,0,pieceButtonSize,pieceButtonSize);
         dst.Offset(paletteRect.left + (player == white ? 0 : pieceButtonSize - 1), paletteRect.top + (king - piece)*(pieceButtonSize - 1));
         CRect src = pieceBmp1->CalcPieceRect(player + piece);
         PieceButton[player + piece] =
            new CButton
            (  this,dst,posEditor_SelectPiece, player + piece, true, true,
               pieceBmp1, pieceBmp1, &src, &src, "", &color_Blue
            );
      }
   PieceButton[game->editPiece]->Press(true);

   CRect r = paletteRect;
   r.top = boardRect.bottom - controlHeight_PushButton - 2*28 - 2;
   r.bottom = r.top + controlHeight_PushButton;
   cbutton_ClearBoard = new CPushButton(this, "Clear Board", r); r.Offset(0, 28);
   cbutton_NewBoard   = new CPushButton(this, "New Board", r); r.Offset(0, 28);
   cbutton_PasteBoard = new CPushButton(this, "Paste Board", r);
} /* CPosFilterDialog::CreatePalette */


void CPosFilterDialog::CreateSideToMove (void)
{
   CRect r = playerRect;
   r.bottom = r.top + controlHeight_RadioButton;
   new CTextControl(this, "Side to Move", r); r.Offset(0, controlVDiff_RadioButton);
   cradio_White  = new CRadioButton(this, "White",  1, r); r.Offset(0, controlVDiff_RadioButton);
   cradio_Black  = new CRadioButton(this, "Black",  1, r); r.Offset(0, controlVDiff_RadioButton);
   cradio_Either = new CRadioButton(this, "Either", 1, r);

   switch (pf.sideToMove)
   {  case white : cradio_White->Select(); break;
      case black : cradio_Black->Select(); break;
      default    : cradio_Either->Select();
   }
} /* CPosFilterDialog::CreateSideToMove */


void CPosFilterDialog::CreateMoveRange (void)
{
   CRect r = rangeRect;
   r.bottom = r.top + controlHeight_RadioButton;
   new CTextControl(this, "Move Range", r); r.Offset(0, controlVDiff_Text);
   cradio_EntireGame = new CRadioButton(this, "Check entire game", 2, r); r.Offset(0, controlVDiff_RadioButton);
   cradio_MoveRange  = new CRadioButton(this, "Only check moves",  2, r); r.Offset(24, controlVDiff_Edit - 3);
   if (pf.checkMoveRange) cradio_MoveRange->Select();
   else cradio_EntireGame->Select();

   CHAR minMoveStr[5], maxMoveStr[5];
   NumToStr(pf.minMove, minMoveStr);
   if (pf.maxMove != posFilter_AllMoves) NumToStr(pf.maxMove, maxMoveStr);
   else CopyStr("", maxMoveStr);

   r.bottom = r.top + controlHeight_Edit; r.right = r.left + 35;
   cedit_Mmin = new CEditControl(this, minMoveStr, r, 3, true, pf.checkMoveRange); r.Offset(55, 0);
   cedit_Mmax = new CEditControl(this, maxMoveStr, r, 3, true, pf.checkMoveRange);
} /* CPosFilterDialog::CreateMoveRange */


void CPosFilterDialog::CreateExactPartial (void)
{
   CRect r = exactPartialRect;
   r.bottom = r.top + controlHeight_RadioButton;

   cradio_Exact   = new CRadioButton(this, "Exact Match",   3, r); r.Offset(0, controlVDiff_RadioButton);
   cradio_Partial = new CRadioButton(this, "Partial Match", 3, r); r.Offset(0, controlVDiff_RadioButton);
   if (pf.exactMatch) cradio_Exact->Select();
   else cradio_Partial->Select();

   r.left += 20;
   new CTextControl(this, "# of white pieces", r); r.Offset(0, controlVDiff_Edit - 3);
   CRect re = r;
   re.bottom = re.top + controlHeight_Edit; re.right = re.left + 35;
   CHAR wminStr[5]; NumToStr(pf.wCountMin, wminStr);
   CHAR wmaxStr[5]; NumToStr(pf.wCountMax, wmaxStr);
   cedit_Wmin = new CEditControl(this, wminStr, re, 3, true, ! pf.exactMatch); re.Offset(55, 0);
   cedit_Wmax = new CEditControl(this, wmaxStr, re, 3, true, ! pf.exactMatch);

   r.top = re.bottom + 10;
   r.bottom = r.top + controlHeight_Text;
   new CTextControl(this, "# of black pieces", r); r.Offset(0, controlVDiff_Edit - 3);
   re = r;
   re.bottom = re.top + controlHeight_Edit; re.right = re.left + 35;
   CHAR bminStr[5]; NumToStr(pf.bCountMin, bminStr);
   CHAR bmaxStr[5]; NumToStr(pf.bCountMax, bmaxStr);
   cedit_Bmin = new CEditControl(this, bminStr, re, 3, true, ! pf.exactMatch); re.Offset(55, 0);
   cedit_Bmax = new CEditControl(this, bmaxStr, re, 3, true, ! pf.exactMatch);
} /* CPosFilterDialog::CreateExactPartial */

/*----------------------------------------- Destructor -------------------------------------------*/

CPosFilterDialog::~CPosFilterDialog (void)
{
   game->Edit_End(false);
   delete game;
} /* CPosFilterDialog::~CPosFilterDialog */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

void CPosFilterDialog::HandleRadioButton (CRadioButton *ctrl)
{
   ctrl->Select();       // Default behaviour is to select radiobutton

   if (ctrl == cradio_EntireGame)
   {  cedit_Mmin->Enable(false);
      cedit_Mmax->Enable(false);
   }
   else if (ctrl == cradio_MoveRange)
   {  cedit_Mmin->Enable(true);
      cedit_Mmax->Enable(true);
   }
   else if (ctrl == cradio_Exact)
   {  cedit_Wmin->Enable(false);
      cedit_Wmax->Enable(false);
      cedit_Bmin->Enable(false);
      cedit_Bmax->Enable(false);
   }
   else if (ctrl == cradio_Partial)
   {  cedit_Wmin->Enable(true);
      cedit_Wmax->Enable(true);
      cedit_Bmin->Enable(true);
      cedit_Bmax->Enable(true);
   }
} /* CPosFilterDialog::HandleRadioButton */


void CPosFilterDialog::HandlePushButton (CPushButton *ctl)
{
   if (ctl == cbutton_NewBoard)
   {
      game->Edit_NewBoard();
      boardView->DrawAllSquares();
   }
   else if (ctl == cbutton_ClearBoard)
   {
      game->Edit_ClearBoard();
      boardView->DrawAllSquares();
   }
   else if (ctl == cbutton_PasteBoard)
   {
      PastePosition();
   }
   else if (ctl == cbutton_Default)
   {
      Process_SideToMove();
      if (! Process_MoveRange()) return;
      if (! Process_ExactPartial()) return;
      if (! Process_Board()) return;
   }

   CDialog::HandlePushButton(ctl);
} /* CPosFilterDialog::HandlePushButton */


void CPosFilterDialog::HandleMessage (LONG msg, LONG submsg, PTR data)
{
   switch (msg)
   {
      case posEditor_SelectPiece :
         PieceButton[game->editPiece]->Press(false);
         PieceButton[submsg]->Press(true);
         game->editPiece = submsg;
         break;

      case msg_RefreshPieceSet :
         boardView->DrawAllSquares();
         for (COLOUR player = white; player <= black; player += black)
            for (PIECE piece = king; piece >= pawn; piece--)
               PieceButton[player + piece]->Redraw();
         break;

      case msg_RefreshBoardType :
         boardView->DrawFrame();
         boardView->DrawAllSquares();
         break;
   }
} /* CPosFilterDialog::HandleMessage */


void CPosFilterDialog::PastePosition (void)
{
   PTR  s = nil;
   LONG size;

   if (theApp->ReadClipboard('TEXT', &s, &size) != appErr_NoError)
   {
      NoteDialog(this, "Paste Board", "No board position was found on the clipboard...", cdialogIcon_Warning);
   }
   else
   {
      if (game->Read_EPD((CHAR*)s) != epdErr_NoError)
         NoteDialog(this, "Error", "Failed parsing EPD position", cdialogIcon_Error);
      else
      {  HandleRadioButton(game->player == white ? cradio_White : cradio_Black);
         HandleRadioButton(cradio_Exact);
         boardView->DrawAllSquares();
      }

      Mem_FreePtr((PTR)s);
   }
} /* CPosFilterDialog::PastePosition */

/*--------------------------------- Dialog -> Position Filter ------------------------------------*/

void CPosFilterDialog::Process_SideToMove (void)
{
   if (cradio_White->Selected()) pf.sideToMove = white;
   else if (cradio_Black->Selected()) pf.sideToMove = black;
   else pf.sideToMove = posFilter_Any;
} /* CPosFilterDialog::Process_SideToMove */


BOOL CPosFilterDialog::Process_MoveRange (void)
{
   if (cradio_EntireGame->Selected())
   {
      pf.checkMoveRange = false;
   }
   else
   {
      LONG n;  // Edit control utility

      pf.checkMoveRange = true;
      
      if (! cedit_Mmin->ValidateNumber(1,gameRecSize/2,false))
      {  CurrControl(cedit_Mmin);
         NoteDialog(this, "Invalid move number", "The first move number must a whole number between 1 and 400.", cdialogIcon_Error);
         return false;
      }
      cedit_Mmin->GetLong(&n); pf.minMove = n;
      if (! cedit_Mmax->ValidateNumber(n+1,gameRecSize/2,true))
      {  CurrControl(cedit_Mmax);
         NoteDialog(this, "Invalid move number", "The last move number must either be between the first number and 400, or be empty (rest of game).", cdialogIcon_Error);
         return false;
      }
      if (cedit_Mmax->GetLong(&n)) pf.maxMove = n;
      else pf.maxMove = posFilter_AllMoves;
   }

   return true;
} /* CPosFilterDialog::Process_MoveRange */


BOOL CPosFilterDialog::Process_ExactPartial (void)
{
   if (cradio_Exact->Selected())
   {
      pf.exactMatch = true;
   }
   else
   {
      LONG n;  // Edit control utility

      pf.exactMatch = false;

      if (! cedit_Wmin->ValidateNumber(1,16,false))
      {  CurrControl(cedit_Wmin);
         NoteDialog(this, "Invalid piece count", "The minimum number of white pieces must be between 1 and 16", cdialogIcon_Error);
         return false;
      }
      cedit_Wmin->GetLong(&n); pf.wCountMin = n;
      if (! cedit_Wmax->ValidateNumber(n,16,false))
      {  CurrControl(cedit_Wmax);
         NoteDialog(this, "Invalid piece count", "The maximum number of white pieces must be between the minimum number and 16", cdialogIcon_Error);
         return false;
      }
      cedit_Wmax->GetLong(&n); pf.wCountMax = n;

      if (! cedit_Bmin->ValidateNumber(1,16,false))
      {  CurrControl(cedit_Bmin);
         NoteDialog(this, "Invalid piece count", "The minimum number of black pieces must be between 1 and 16", cdialogIcon_Error);
         return false;
      }
      cedit_Bmin->GetLong(&n); pf.bCountMin = n;
      if (! cedit_Bmax->ValidateNumber(n,16,false))
      {  CurrControl(cedit_Bmax);
         NoteDialog(this, "Invalid piece count", "The maximum number of black pieces must be between the minimum number and 16", cdialogIcon_Error);
         return false;
      }
      cedit_Bmax->GetLong(&n); pf.bCountMax = n;
   }

   return true;
} /* CPosFilterDialog::Process_ExactPartial */


BOOL CPosFilterDialog::Process_Board (void)
{
   for (SQUARE sq = a1; sq <= h8; sq++)
      if (onBoard(sq))
         pf.Pos[sq] = game->Board[sq];

   // If exact filter it doesn't make any sense to accept an illegal position,
   // because obviously no such positions will be found during the search.

   INT posCode;

   switch (pf.sideToMove)
   {
      case white :
         posCode = CheckLegalPosition(game->Board, white);
         break;
      case black :
         posCode = CheckLegalPosition(game->Board, black);
         break;
      case posFilter_Any :
         // Any player will do here (so we choose white), because we don't
         // care if the opponent king is check
         posCode = CheckLegalPosition(game->Board, white);
         if (posCode == pos_OpponentInCheck)
            posCode = pos_Legal;
   }

   if (! pf.exactMatch)  // If partial it's OK if kings are missing
      if (posCode == pos_WhiteKingMissing || posCode == pos_BlackKingMissing)
         posCode = pos_Legal;

   CHAR s[100], message[300];

   switch (posCode)
   {
      case pos_Legal                : return true;
      case pos_TooManyWhitePawns    : CopyStr("there are too many white pawns", s); break;
      case pos_TooManyBlackPawns    : CopyStr("there are too many black pawns", s); break;
      case pos_WhiteKingMissing     : CopyStr("there is no white king", s); break;
      case pos_BlackKingMissing     : CopyStr("there is no black king", s); break;
      case pos_TooManyWhiteKings    : CopyStr("there is more than one white king", s); break;
      case pos_TooManyBlackKings    : CopyStr("there is more than one black king", s); break;
      case pos_TooManyWhiteOfficers : CopyStr("there are too many white pieces", s); break;
      case pos_TooManyBlackOfficers : CopyStr("there are too many black pieces", s); break;
      case pos_PawnsOn1stRank       : CopyStr("pawns are not allowed on the 1st and 8th rank", s); break;
      case pos_OpponentInCheck      : CopyStr("the opponent king is in check", s); break;
   }

   Format(message, "Invalid position (%s). Applying this filter would result in an empty game list.", s);
   NoteDialog(this, "Invalid Position", message);

   return false;
} /* CPosFilterDialog::Process_Board */
