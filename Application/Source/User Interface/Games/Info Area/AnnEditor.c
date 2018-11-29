/**************************************************************************************************/
/*                                                                                                */
/* Module  : AnnEditor.c                                                                          */
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

#include "AnnEditor.h"
#include "GameWindow.h"

/*---------------------------------------- Sub Views ---------------------------------------------*/

class AnnToolbar : public CToolbar
{
public:
   AnnToolbar (CViewOwner *parent, CRect frame);

   void Adjust (BOOL canUndo, BOOL canRedo, BOOL textSelected);

private:
   CMenu   *pm_Glyph;
   CButton *tb_Glyph;
   CButton *tb_Diagram;
   CButton *tb_Undo;
   CButton *tb_Redo;
   CButton *tb_Cut;
   CButton *tb_Copy;
   CButton *tb_Paste;
   CButton *tb_Trash;
   CButton *tb_Search;
};


/**************************************************************************************************/
/*                                                                                                */
/*                                     CONSTRUCTOR/DESTRUCTOR                                     */
/*                                                                                                */
/**************************************************************************************************/

AnnEditorView::AnnEditorView (CViewOwner *parent, CRect frame)
 : BackView(parent, frame, false)
{
   game = ((GameWindow*)Window())->game;

   CRect editorRect, toolbarRect;
   CalcFrames(&editorRect, &toolbarRect);
   editor  = new CEditor(this, editorRect, "", 30000);
   toolbar = new AnnToolbar(this, toolbarRect);
   Window()->AddControl(editor);

   Show(false);
   Load();
} /* AnnEditorView */


AnnEditorView::~AnnEditorView (void)
{
   Flush();

   Window()->RemoveControl(editor);
} /* AnnEditorView::~AnnEditorView */


void AnnEditorView::CalcFrames (CRect *editorRect, CRect *toolbarRect)
{
   *editorRect = *toolbarRect = DataViewRect();
   toolbarRect->Inset(1, 1);
   toolbarRect->bottom = toolbarRect->top + toolbar_HeightSmall;
   editorRect->top = toolbarRect->bottom; // - 1;

   ExcludeRect(DataViewRect());
} /* AnnEditorView::CalcFrames */


/**************************************************************************************************/
/*                                                                                                */
/*                                        EVENT HANDLING                                          */
/*                                                                                                */
/**************************************************************************************************/

void AnnEditorView::HandleUpdate (CRect updateRect)
{
   BackView::HandleUpdate(updateRect);
   DrawBottomRound();

   SetForeColor(RunningOSX() || ! Active() ? &color_MdGray : &color_Black);
   DrawRectFrame(DataViewRect());
} /* AnnEditorView::HandleUpdate */


void AnnEditorView::HandleResize (void)
{
   CRect editorRect, toolbarRect;
   CalcFrames(&editorRect, &toolbarRect);
   editor->SetFrame(editorRect, true);
   toolbar->SetFrame(toolbarRect);
} /* AnnEditorView::HandleResize */


void AnnEditorView::InsertDiagram (void)
{
   editor->InsText("\r[DIAGRAM]\r");
} /* AnnEditorView::InsertDiagram */


/**************************************************************************************************/
/*                                                                                                */
/*                                            MISC                                                */
/*                                                                                                */
/**************************************************************************************************/

void AnnEditorView::Load (void)
{
   INT size;
   CHAR text[10000];
   game->GetAnnotation(game->currMove, text, &size);
   editor->SetText(text, size);
   AdjustToolbar();
} /* AnnEditorView::Load */

// When the annotation editor is closed we automatically "flush" and store the new annotation text.
// Additionally, if the user browses the game records, we again need to flush BEFORE undo/redoing
// moves.

void AnnEditorView::Flush (void)
{
   if (editor->Dirty())
   {
      CHAR text[10000];
      INT size = editor->GetText(text);
      editor->ClearDirty();
      game->SetAnnotation(game->currMove, text, size);
      ((GameWindow*)(Window()))->AdjustFileMenu();
      ((GameWindow*)(Window()))->AdjustGameMenu();
      ((GameWindow*)(Window()))->AdjustToolbar();
   }
} /* AnnEditorView::Flush */

// When the contents/selection of the editor control has changed we need to enable the toolbar
// accordingly

void AnnEditorView::AdjustToolbar (void)
{
   toolbar->Adjust(editor->CanUndo(), editor->CanRedo(), editor->TextSelected());
} /* AnnEditorView::AdjustToolbar */


/**************************************************************************************************/
/*                                                                                                */
/*                                           TOOLBAR                                              */
/*                                                                                                */
/**************************************************************************************************/

AnnToolbar::AnnToolbar (CViewOwner *parent, CRect frame)
 : CToolbar(parent, frame)
{
   pm_Glyph = new CMenu("");
   pm_Glyph->AddPopupHeader("Move Annotation");
   pm_Glyph->AddItem("None", 0);
   pm_Glyph->AddItem("!",    1);
   pm_Glyph->AddItem("?",    2);
   pm_Glyph->AddItem("!!",   3);
   pm_Glyph->AddItem("??",   4);
   pm_Glyph->AddItem("!?",   5);
   pm_Glyph->AddItem("?!",   6);

   INT width = (bounds.Width() - 3*toolbar_SeparatorWidth)/9;

   tb_Glyph   = AddPopup (edit_SetAnnGlyph, pm_Glyph, icon_AnnGlyph, 16, width, "", "Set move annotation glyph.");
   tb_Diagram = AddButton(edit_Diagram,  308, 16, width, "", "Insert chess diagram (will be included when printing).");
   AddSeparator();
   tb_Undo    = AddButton(edit_Undo,     430, 16, width, "", "Undo last change [Cmd-Z].");
   tb_Redo    = AddButton(edit_Redo,     431, 16, width, "", "Redo last change.");
   AddSeparator();
   tb_Cut     = AddButton(edit_Cut,      432, 16, width, "", "Cut [Cmd-X].");
   tb_Copy    = AddButton(edit_Copy,     433, 16, width, "", "Copy [Cmd-C].");
   tb_Paste   = AddButton(edit_Paste,    434, 16, width, "", "Paste [Cmd-V].");
   tb_Trash   = AddButton(edit_ClearAll, 437, 16, width, "", "Clear annotation text for current move.");
   AddSeparator();
   tb_Search  = AddButton(edit_Find,     440, 16, width, "", "Search and/or Replace text [Cmd-F].");
} /* AnnToolbar::AnnToolbar */


void AnnToolbar::Adjust (BOOL canUndo, BOOL canRedo, BOOL textSelected)
{
   GameWindow *win = (GameWindow*)(Window());
   INT glyph = win->game->GetAnnotationGlyph(win->game->currMove);
   for (INT i = 0; i <= 6; i++)
      pm_Glyph->CheckMenuItem(i, i == glyph);

   tb_Glyph->Enable(win->game->currMove > 0);
   tb_Undo->Enable(canUndo);
   tb_Redo->Enable(canRedo);
   tb_Cut->Enable(textSelected);
   tb_Copy->Enable(textSelected);
} /* AnnToolbar::Adjust */
