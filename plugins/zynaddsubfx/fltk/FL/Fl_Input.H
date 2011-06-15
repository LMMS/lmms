//
// "$Id: Fl_Input.H 8111 2010-12-23 08:13:18Z manolo $"
//
// Input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

/* \file
   Fl_Input widget . */

#ifndef Fl_Input_H
#define Fl_Input_H

#include "Fl_Input_.H"

/**
  This is the FLTK text input widget. It displays a single line
  of text and lets the user edit it. Normally it is drawn with an
  inset box and a white background. The text may contain any
  characters, and will correctly display any UTF text, using
  ^X notation for unprintable control characters. It assumes the
  font can draw any characters of the used scripts, which is true
  for standard fonts under MSWindows and Mac OS X.
  Characters can be input using the keyboard or the character palette/map.
  Character composition is done using dead keys and/or a compose
  key as defined by the operating system.
  <P>
  <TABLE WIDTH=90% BORDER=1 SUMMARY="Fl_Input keyboard and mouse bindings.">
    <CAPTION ALIGN=TOP>Fl_Input keyboard and mouse bindings.</CAPTION>
    <TR><TD NOWRAP="NOWRAP" WIDTH="1%">
      <B>Mouse button 1</B> 
    </TD><TD>
      Moves the cursor to this point. 
      Drag selects characters. 
      Double click selects words. 
      Triple click selects all line.
      Shift+click extends the selection.
      When you select text it is automatically copied to the selection buffer.
    </TD></TR><TR><TD NOWRAP="NOWRAP">
      <B>Mouse button 2</B> 
    </TD><TD>
      Insert the selection buffer at the point clicked.
      You can also select a region and replace it with the selection buffer
      by selecting the region with mouse button 2.
    </TD></TR><TR><TD NOWRAP="NOWRAP">
      <B>Mouse button 3</B>
    </TD><TD>
      Currently acts like button 1.
    </TD></TR><TR><TD NOWRAP="NOWRAP">
      <B>Backspace</B>
    </TD><TD>
      Deletes one character to the left, or deletes the selected region.
    </TD></TR><TR><TD NOWRAP="NOWRAP">
      <B>Delete</B>
    </TD><TD>
      Deletes one character to the right, or deletes the selected region.
      Combine with Shift for equivalent of ^X (copy+cut).
    </TD></TR><TR><TD NOWRAP="NOWRAP">
      <B>Enter</b>
    </TD><TD>
      May cause the callback, see when().
  </TD></TR></TABLE>

  <P>

  <TABLE WIDTH="90%" BORDER="1" SUMMARY="Fl_Input platform specific keyboard bindings.">
    <CAPTION ALIGN=TOP>Fl_Input platform specific keyboard bindings.</CAPTION>
  <TR>
    <TD NOWRAP="NOWRAP" WIDTH="1%"><B> Windows/Linux </B></TD>
    <TD NOWRAP="NOWRAP" WIDTH="1%"><B> Mac </B></TD>
    <TD NOWRAP="NOWRAP"           ><B> Function </B></TD>

  </TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^A </B></TD>
    <TD NOWRAP="NOWRAP"><B> Command-A </B></TD>
    <TD>
      <B>Selects all text in the widget.</B>

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^C </B></TD>
    <TD NOWRAP="NOWRAP"><B> Command-C </B></TD>
    <TD>
      <B>Copy the current selection to the clipboard.</B>

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^I </B></TD>
    <TD NOWRAP="NOWRAP"><B> ^I </B></TD>
    <TD>
      <B>Insert a tab.</B>

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^J </B></TD>
    <TD NOWRAP="NOWRAP"><B> ^J </B></TD>
    <TD>
      <B>Insert a Line Feed.</B> <BR>
      (Similar to literal 'Enter' character)

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^L </B></TD>
    <TD NOWRAP="NOWRAP"><B> ^L </B></TD>
    <TD>
      <B>Insert a Form Feed.</B>

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^M </B></TD>
    <TD NOWRAP="NOWRAP"><B> ^M </B></TD>
    <TD>
      <B>Insert a Carriage Return.</B>

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^V,<BR>Shift-Insert </B></TD>
    <TD NOWRAP="NOWRAP"><B> Command-V </B></TD>
    <TD>
      <B>Paste the clipboard.</B> <BR>
      (Macs keyboards don't have "Insert" keys, 
      but if they did, Shift-Insert would work)

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^X,<BR>Shift-Delete </B></TD>
    <TD NOWRAP="NOWRAP"><B> Command-X,<BR>Shift-Delete </B></TD>
    <TD>
      <B>Cut.</B> <BR>
      Copy the selection to the clipboard and delete it.
      (If there's no selection, Shift-Delete acts like Delete)

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> ^Z </B></TD>
    <TD NOWRAP="NOWRAP"><B> Command-Z </B></TD>
    <TD>
      <B>Undo.</B> <BR>
      This is a single-level undo mechanism, but all adjacent 
      deletions and insertions are concatenated into a single "undo".
      Often this will undo a lot more than you expected. 

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> Shift-^Z </B></TD>
    <TD NOWRAP="NOWRAP"><B> Shift-Command-Z </B></TD>
    <TD>
      <B>Redo.</B> <BR>
      Currently same behavior as ^Z. 
      Reserved for future multilevel undo/redo.

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> Arrow Keys </B></TD>
    <TD NOWRAP="NOWRAP"><B> Arrow Keys </B></TD>
    <TD>
      <B>Standard cursor movement.</B> <BR>
      Can be combined with Shift to extend selection.

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> Home </B></TD>
    <TD NOWRAP="NOWRAP"><B> Command-Up,<BR>Command-Left </B></TD>
    <TD>
      <B>Move to start of line.</B> <BR>
      Can be combined with Shift to extend selection.

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> End </B></TD>
    <TD NOWRAP="NOWRAP"><B> Command-Down,<BR>Command-Right </B></TD>
    <TD>
      <B>Move to end of line.</B> <BR>
      Can be combined with Shift to extend selection.

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B>Ctrl-Home</B></TD>
    <TD NOWRAP="NOWRAP"><B>Command-Up,<BR>Command-PgUp,<BR>Ctrl-Left</B></TD>
    <TD>
      <B>Move to top of document/field.</B> <BR>
      In single line input, moves to start of line.
      In multiline input, moves to start of top line.
      Can be combined with Shift to extend selection.

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> Ctrl-End </B></TD>
    <TD NOWRAP="NOWRAP"><B> Command-End,<BR>Command-PgDn,<BR>Ctrl-Right</B></TD>
    <TD>
      <B>Move to bottom of document/field.</B> <BR>
      In single line input, moves to end of line.
      In multiline input, moves to end of last line.
      Can be combined with Shift to extend selection.

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> Ctrl-Left </B></TD>
    <TD NOWRAP="NOWRAP"><B> Alt-Left </B></TD>
    <TD>
      <B>Word left.</B> <BR>
      Can be combined with Shift to extend selection.

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> Ctrl-Right </B></TD>
    <TD NOWRAP="NOWRAP"><B> Alt-Right </B></TD>
    <TD>
      <B>Word right.</B> <BR>
      Can be combined with Shift to extend selection.

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> Ctrl-Backspace </B></TD>
    <TD NOWRAP="NOWRAP"><B> Alt-Backspace </B></TD>
    <TD>
      <B>Delete word left.</B>

  </TD></TR><TR>
    <TD NOWRAP="NOWRAP"><B> Ctrl-Delete </B></TD>
    <TD NOWRAP="NOWRAP"><B> Alt-Delete </B></TD>
    <TD>
      <B>Delete word right.</B>

  </TD></TR></TABLE>
  */
class FL_EXPORT Fl_Input : public Fl_Input_ {
  int handle_key();
  int shift_position(int p);
  int shift_up_down_position(int p);
  void handle_mouse(int keepmark=0);

  // Private keyboard functions
  int kf_lines_up(int repeat_num);
  int kf_lines_down(int repeat_num);
  int kf_page_up();
  int kf_page_down();
  int kf_insert_toggle();
  int kf_delete_word_right();
  int kf_delete_word_left();
  int kf_delete_sol();
  int kf_delete_eol();
  int kf_delete_char_right();
  int kf_delete_char_left();
  int kf_move_sol();
  int kf_move_eol();
  int kf_clear_eol();
  int kf_move_char_left();
  int kf_move_char_right();
  int kf_move_word_left();
  int kf_move_word_right();
  int kf_move_up_and_sol();
  int kf_move_down_and_eol();
  int kf_top();
  int kf_bottom();
  int kf_select_all();
  int kf_undo();
  int kf_redo();
  int kf_copy();
  int kf_paste();
  int kf_copy_cut();

protected:
  void draw();
public:
  int handle(int);
  Fl_Input(int,int,int,int,const char * = 0);
};

#endif 

//
// End of "$Id: Fl_Input.H 8111 2010-12-23 08:13:18Z manolo $".
//
