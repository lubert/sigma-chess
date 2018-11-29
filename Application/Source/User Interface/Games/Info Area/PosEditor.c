/**************************************************************************************************/
/*                                                                                                */
/* Module  : PosEditor.c                                                                          */
/* Purpose : This module implements the Position Editor.                                          */
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

#include "PosEditor.h"
#include "GameWindow.h"
#include "BoardView.h"

// The Position Editor is a subview of the InfoArea View (declared in "InfoArea.h") and is
// initially invisible. Then, when the user opens the position editor, the other subviews of the
// Info Area (Game, Analysis & Statistics) are hidden, and the PosEditor view is made visible.
//
// Controlling the visibility of the various InfoArea subviews is handled by the InfoArea View
// itself.

// The Position Editor contains the following controls/subviews:
// ¥ The 12 piece type buttons (white king initially selected).
// ¥ The "Clear Board" button.
// ¥ The "Switch Colours" button.
// ¥ The "Initial Position Status" button (###ctrl-click to set EP square!!)
// ¥ The "OK" and "Cancel" buttons.
// ¥ (The "Copy/Paste EPD" buttons are no longer necessary??? Instead use Copy/Paste from Edit
//   menu "outside" Position Editor").


/**************************************************************************************************/
/*                                                                                                */
/*                                           CONSTRUCTOR                                          */
/*                                                                                                */
/**************************************************************************************************/

HEADER_COLUMN PieceHeader[1] = {{ "Current Piece", 0, 0 }};
HEADER_COLUMN ColorHeader[1] = {{ "", 0, 0 }};


PosEditorView::PosEditorView (CViewOwner *parent, CRect frame)
 : BackView(parent, frame, true)
{
   CRect r = bounds;

   game = ((GameWindow*)Window())->game;

   // Create a container view for the 12 piece buttons (and the header):
   r.Inset(15, 8);
   r.right  = r.left + 2*pieceButtonSize - 1;
   r.bottom = r.top + 19 + 6*(pieceButtonSize - 1) - 1;
   ExcludeRect(r);
   CreatePieceButtons(r);

   // Create a container view for the 2 colour/player buttons (and the header):
   r.top = r.bottom + 8;
   r.bottom = r.top + 19 + pieceButtonSize - 2;
   CreatePlayerButtons(r);

   // Create the remaining buttons: "Clear Board", "Status...", "Done", "Cancel".
   CreateTextButtons();

   SelectPiece(game->editPiece);
   SelectPlayer(game->player);
} /* PosEditorView */


void PosEditorView::CreatePieceButtons (CRect r)
{
   CHAR *helpText = "Change the ÒCurrent PieceÓ. Clicking on an empty square on the board \\
will place the current piece on that square.";

   CView *pieceView = new DataView(this, r, false);

   CRect rheader = pieceView->bounds;
   rheader.bottom = rheader.top + 18;
   new DataHeaderView(pieceView, rheader, false, true, 1, PieceHeader);

   for (COLOUR player = white; player <= black; player += black)
      for (PIECE piece = king; piece >= pawn; piece--)
      {
         CRect dst(0,0,pieceButtonSize,pieceButtonSize);
         dst.Offset((player == white ? 0 : pieceButtonSize - 1), rheader.bottom - 1 + (king - piece)*(pieceButtonSize - 1));
         CRect src = pieceBmp1->CalcPieceRect(player + piece);
         src.Inset(2, 2);
         PieceButton[player + piece] =
            new CButton
            (  pieceView,dst,posEditor_SelectPiece, player + piece, true, true,
               pieceBmp1, pieceBmp1, &src, &src, helpText, &color_Blue
            );
      }
} /* PosEditorView::CreatePieceButtons */


void PosEditorView::CreatePlayerButtons (CRect r)
{
   colorView = new DataView(this, r, false);

   CRect rheader = colorView->bounds;
   rheader.bottom = rheader.top + headerViewHeight;
   colorHeader = new DataHeaderView(colorView, rheader, false, true, 1, ColorHeader);

   CRect rcolor(0,0,pieceButtonSize,pieceButtonSize);
   rcolor.Offset(0, rheader.bottom - 1);
   whiteToMove = 
      new CButton
      (  colorView, rcolor, posEditor_SelectPlayer, white, true, true,
         GetBmp(3101), GetBmp(3101), nil, nil, "Set WHITE to move in the current board position.", &color_White
      );
   rcolor.Offset(pieceButtonSize - 1, 0);
   blackToMove =
      new CButton
      (  colorView, rcolor, posEditor_SelectPlayer, black, true, true,
         GetBmp(3102), GetBmp(3102), nil, nil, "Set BLACK to move in the current board position.", &color_White
      );
} /* PosEditorView::CreatePlayerButtons */


void PosEditorView::CreateTextButtons (void)
{
   // Create the remaining buttons: "Clear Board", "Status...", "Done", "Cancel".
   CRect r = bounds;
   r.Inset(15, 25);
   r.left = r.right - (2*pieceButtonSize - 1);
   r.bottom = colorView->frame.bottom;
   r.top = r.bottom - 21;

   buttonDone   = new CButton(this, r,posEditor_Done,0,true,true,"Done","Exit Position Editor and store the new position."); r.Offset(0, -30);
   buttonCancel = new CButton(this, r,posEditor_Cancel,0,true,true,"Cancel","Exit Position Editor and restore the previous position."); r.Offset(0, -50);
   buttonStatus = new CButton(this, r,posEditor_Status,0,true,true,"Status...","Set initial position status: Castling rights, EP status, 50 move rule etc."); r.Offset(0, -30);
   buttonNew    = new CButton(this, r,posEditor_NewBoard,0,true,true,"New Board","Setup all pieces in their initial position."); r.Offset(0, -30);
   buttonClear  = new CButton(this, r,posEditor_ClearBoard,0,true,true,"Clear Board","Remove all pieces from the board.");
} /* PosEditorView::CreateTextButtons */


/**************************************************************************************************/
/*                                                                                                */
/*                                         EVENT HANDLING                                         */
/*                                                                                                */
/**************************************************************************************************/

void PosEditorView::HandleUpdate (CRect updateRect)
{
   BackView::HandleUpdate(updateRect);

   Outline3DRect(colorView->frame);
   Outline3DRect(buttonDone->frame);
   Outline3DRect(buttonCancel->frame);
   Outline3DRect(buttonStatus->frame);
   Outline3DRect(buttonClear->frame);
   Outline3DRect(buttonNew->frame);
} /* PosEditorView::HandleUpdate */


void PosEditorView::HandleCloseRequest (void)
{
   ((GameWindow*)Window())->HandleMessage(posEditor_Done);
} /* PosEditorView::HandleCloseRequest */


BOOL PosEditorView::HandleKeyDown (CHAR c, INT key, INT modifiers)
{
   switch (key)
   {
      case key_Return :
      case key_Enter :
         buttonDone->Press(true); Sleep(10); buttonDone->Press(false);
         Window()->HandleMessage(posEditor_Done);
         return true;
      case key_Escape :
         buttonCancel->Press(true); Sleep(10); buttonCancel->Press(false);
         Window()->HandleMessage(posEditor_Cancel);
         return true;
   }
   return false;
} /* PosEditorView::HandleKeyDown */


void PosEditorView::SelectPiece (PIECE newPiece)
{
   PieceButton[game->editPiece]->Press(false);
   PieceButton[newPiece]->Press(true);
   game->editPiece = newPiece;
} /* PosEditorView::SelectPiece */


void PosEditorView::SelectPlayer (COLOUR player)
{
   whiteToMove->Press(player == white);
   blackToMove->Press(player == black);
   game->Edit_SetPlayer(player);
   ColorHeader[0].text = (player == white ? "White to move" : "Black to move");
   colorHeader->Redraw();
} /* PosEditorView::SelectPlayer */


/**************************************************************************************************/
/*                                                                                                */
/*                                              MISC                                              */
/*                                                                                                */
/**************************************************************************************************/

void PosEditorView::RefreshPieceSet (void)
{
   for (PIECE piece = king; piece >= pawn; piece--)
      PieceButton[white + piece]->Redraw(),
      PieceButton[black + piece]->Redraw();
} /* PosEditorView::RefreshPieceSet */
