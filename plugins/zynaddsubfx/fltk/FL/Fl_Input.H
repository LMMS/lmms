//
// "$Id: Fl_Input.H 6699 2009-03-18 22:58:12Z engelsman $"
//
// Input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2009 by Bill Spitzak and others.
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
  characters (even 0), and will correctly display anything, using
  ^X notation for unprintable control characters and \\nnn notation
  for unprintable characters with the high bit set. It assumes the
  font can draw any characters in the ISO-8859-1 character set.</P>
  
  <CENTER><TABLE border=1 WIDTH=90% summary="Fl_Input keyboard and mouse bindings.">
  
  <TR><TD WIDTH=200><B>Mouse button 1</B></TD><TD>Moves the cursor to
  this point. Drag selects characters.  Double click selects words.
  Triple click selects all text.  Shift+click extends the selection.
  When you select text it is automatically copied to the clipboard.
  </TD></TR>
  
  <TR><TD><B>Mouse button 2</B></TD><TD>Insert the clipboard at
  the point clicked.  You can also select a region and replace it with the
  clipboard by selecting the region with mouse button 2.
  </TD></TR>
  
  <TR><TD><B>Mouse button 3</B></TD><TD>Currently acts like button 1.</TD></TR>
  
  <TR><TD><B>Backspace</B></TD><TD>Deletes one character to the left, or
  deletes the selected region.</TD></TR>
  <TR><TD><B>Enter</B></TD><TD>May cause the callback, see when().</TD></TR>
  <TR><TD><B>^A or Home</B></TD><TD>Go to start of line.</TD></TR>
  <TR><TD><B>^B or Left</B></TD><TD>Move left</TD></TR>
  <TR><TD><B>^C</B></TD><TD>Copy the selection to the clipboard</TD></TR>
  <TR><TD><B>^D or Delete</B></TD><TD>Deletes one character to the right
  or deletes the selected region.</TD></TR>
  <TR><TD><B>^E or End</B></TD><TD>Go to the end of line.</TD></TR>
  <TR><TD><B>^F or Right</B></TD><TD>Move right</TD></TR>
  <TR><TD><B>^K</B></TD><TD>Delete to the end of line (next \\n character)
  or deletes a single \\n character.  These deletions are all concatenated
  into the clipboard.</TD></TR>
  <TR><TD><B>^N or Down</B></TD><TD>Move down (for Fl_Multiline_Input
  only, otherwise it moves to the next input field).</TD></TR>
  <TR><TD><B>^P or Up</B></TD><TD>Move up (for Fl_Multiline_Input only,
  otherwise it moves to the previous input field).</TD></TR>
  <TR><TD><B>^U</B></TD><TD>Delete everything.</TD></TR>
  <TR><TD><B>^V or ^Y</B></TD><TD>Paste the clipboard</TD></TR>
  <TR><TD><B>^X or ^W</B></TD><TD>Copy the region to the clipboard and
  delete it.</TD></TR>
  <TR><TD><B>^Z or ^_</B></TD><TD>Undo.  This is a single-level undo
  mechanism, but all adjacent deletions and insertions are concatenated
  into a single "undo".  Often this will undo a lot more than you
  expected.</TD></TR>
  <TR><TD><B>Shift+move</B></TD><TD>Move the cursor but also extend the
  selection.</TD></TR>
  
  <TR><TD><B>RightCtrl or
  <BR>Compose</B></TD><TD>
  \anchor Fl_Input_Compose_Character
  Start a compose-character
  sequence.  The next one or two keys typed define the character to
  insert (see table that follows.)
  
  <p>For instance, to type "á" type [compose][a]['] or [compose]['][a].
  
  <P>The character "nbsp" (non-breaking space) is typed by using
  [compose][space].
  
  <P>The single-character sequences may be followed by a space if
  necessary to remove ambiguity.  For instance, if you really want to
  type "ª~" rather than "ã" you must type [compose][a][space][~].
  
  <p>The same key may be used to "quote" control characters into the
  text.  If you need a ^Q character you can get one by typing
  [compose][Control+Q].
  
  <p>X may have a key on the keyboard
  defined as XK_Multi_key.  If so this key may be used as well
  as the right-hand control key.  You can set this up with the program
  xmodmap.
  
  <p>If your keyboard is set to support a foreign language you should
  also be able to type "dead key" prefix characters.  On X you will
  actually be able to see what dead key you typed, and if you then move
  the cursor without completing the sequence the accent will remain
  inserted.</TD></TR>
  </TABLE></CENTER>
  
  <!-- NEW PAGE -->
  <center><table border=1 summary="Character Composition Table">
  <caption align="top">Character Composition Table</caption>
  <tr>
  	<th>Keys</th><th>Char</th>
  	<th>Keys</th><th>Char</th>
  	<th>Keys</th><th>Char</th>
  	<th>Keys</th><th>Char</th>
  	<th>Keys</th><th>Char</th>
  	<th>Keys</th><th>Char</th>
  
  </tr><tr>
  	<td align=center>sp</td><td align=center><small>nbsp</small></td>
  	<td align=center>*</td><td align=center>°</td>
  	<td align=center>` A</td><td align=center>À</td>
  	<td align=center>D -</td><td align=center>Ð</td>
  	<td align=center>` a</td><td align=center>à</td>
  	<td align=center>d -</td><td align=center>ð</td>
  </tr><tr>
  	<td align=center>!</td><td align=center>¡</td>
  	<td align=center>+ -</td><td align=center>±</td>
  	<td align=center>' A</td><td align=center>Á</td>
  	<td align=center>~ N</td><td align=center>Ñ</td>
  	<td align=center>' a</td><td align=center>á</td>
  	<td align=center>~ n</td><td align=center>ñ</td>
  </tr><tr>
  	<td align=center>%</td><td align=center>¢</td>
  	<td align=center>2</td><td align=center>²</td>
  	<td align=center>A ^</td><td align=center>Â</td>
  	<td align=center>` O</td><td align=center>Ò</td>
  	<td align=center>^ a</td><td align=center>â</td>
  	<td align=center>` o</td><td align=center>ò</td>
  </tr><tr>
  	<td align=center>#</td><td align=center>£</td>
  	<td align=center>3</td><td align=center>³</td>
  	<td align=center>~ A</td><td align=center>Ã</td>
  	<td align=center>' O</td><td align=center>Ó</td>
  	<td align=center>~ a</td><td align=center>ã</td>
  	<td align=center>' o</td><td align=center>ó</td>
  </tr><tr>
  	<td align=center>$</td><td align=center>¤</td>
  	<td align=center>'</td><td align=center>´</td>
  	<td align=center>: A</td><td align=center>Ä</td>
  	<td align=center>^ O</td><td align=center>Ô</td>
  	<td align=center>: a</td><td align=center>ä</td>
  	<td align=center>^ o</td><td align=center>ô</td>
  </tr><tr>
  	<td align=center>y =</td><td align=center>¥</td>
  	<td align=center>u</td><td align=center>µ</td>
  	<td align=center>* A</td><td align=center>Å</td>
  	<td align=center>~ O</td><td align=center>Õ</td>
  	<td align=center>* a</td><td align=center>å</td>
  	<td align=center>~ o</td><td align=center>õ</td>
  </tr><tr>
  	<td align=center>|</td><td align=center>¦</td>
  	<td align=center>p</td><td align=center>¶</td>
  	<td align=center>A E</td><td align=center>Æ</td>
  	<td align=center>: O</td><td align=center>Ö</td>
  	<td align=center>a e</td><td align=center>æ</td>
  	<td align=center>: o</td><td align=center>ö</td>
  </tr><tr>
  	<td align=center>&</td><td align=center>§</td>
  	<td align=center>.</td><td align=center>·</td>
  	<td align=center>, C</td><td align=center>Ç</td>
  	<td align=center>x</td><td align=center>×</td>
  	<td align=center>, c</td><td align=center>ç</td>
  	<td align=center>- :</td><td align=center>÷</td>
  </tr><tr>
  	<td align=center>:</td><td align=center>¨</td>
  	<td align=center>,</td><td align=center>¸</td>
  	<td align=center>E `</td><td align=center>È</td>
  	<td align=center>O /</td><td align=center>Ø</td>
  	<td align=center>` e</td><td align=center>è</td>
  	<td align=center>o /</td><td align=center>ø</td>
  </tr><tr>
  	<td align=center>c</td><td align=center>©</td>
  	<td align=center>1</td><td align=center>¹</td>
  	<td align=center>' E</td><td align=center>É</td>
  	<td align=center>` U</td><td align=center>Ù</td>
  	<td align=center>' e</td><td align=center>é</td>
  	<td align=center>` u</td><td align=center>ù</td>
  </tr><tr>
  	<td align=center>a</td><td align=center>ª</td>
  	<td align=center>o</td><td align=center>º</td>
  	<td align=center>^ E</td><td align=center>Ê</td>
  	<td align=center>' U</td><td align=center>Ú</td>
  	<td align=center>^ e</td><td align=center>ê</td>
  	<td align=center>' u</td><td align=center>ú</td>
  </tr><tr>
  	<td align=center>&lt; &lt;</td><td align=center>«</td>
  	<td align=center>&gt; &gt;</td><td align=center>»</td>
  	<td align=center>: E</td><td align=center>Ë</td>
  	<td align=center>^ U</td><td align=center>Û</td>
  	<td align=center>: e</td><td align=center>ë</td>
  	<td align=center>^ u</td><td align=center>û</td>
  </tr><tr>
  	<td align=center>~</td><td align=center>¬</td>
  	<td align=center>1 4</td><td align=center>¼</td>
  	<td align=center>` I</td><td align=center>Ì</td>
  	<td align=center>: U</td><td align=center>Ü</td>
  	<td align=center>` i</td><td align=center>ì</td>
  	<td align=center>: u</td><td align=center>ü</td>
  </tr><tr>
  	<td align=center>-</td><td align=center>­</td>
  	<td align=center>1 2</td><td align=center>½</td>
  	<td align=center>' I</td><td align=center>Í</td>
  	<td align=center>' Y</td><td align=center>Ý</td>
  	<td align=center>' i</td><td align=center>í</td>
  	<td align=center>' y</td><td align=center>ý</td>
  </tr><tr>
  	<td align=center>r</td><td align=center>®</td>
  	<td align=center>3 4</td><td align=center>¾</td>
  	<td align=center>^ I</td><td align=center>Î</td>
  	<td align=center>T H</td><td align=center>Þ</td>
  	<td align=center>^ i</td><td align=center>î</td>
  	<td align=center>t h</td><td align=center>þ</td>
  </tr><tr>
  	<td align=center>_</td><td align=center>¯</td>
  	<td align=center>?</td><td align=center>¿</td>
  	<td align=center>: I</td><td align=center>Ï</td>
  	<td align=center>s s</td><td align=center>ß</td>
  	<td align=center>: i</td><td align=center>ï</td>
  	<td align=center>: y</td><td align=center>ÿ</td>
  </tr>
  </table></center>
*/
class FL_EXPORT Fl_Input : public Fl_Input_ {
  int handle_key();
  int shift_position(int p);
  int shift_up_down_position(int p);
  void handle_mouse(int keepmark=0);
protected:
  void draw();
public:
  int handle(int);
  Fl_Input(int,int,int,int,const char * = 0);
};

#endif 

//
// End of "$Id: Fl_Input.H 6699 2009-03-18 22:58:12Z engelsman $".
//
