//
// "$Id: Fl_Input.cxx 8726 2011-05-23 18:32:47Z AlbrechtS $"
//
// Input widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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

// This is the "user interface", it decodes user actions into what to
// do to the text.  See also Fl_Input_.cxx, where the text is actually
// manipulated (and some ui, in particular the mouse, is done...).
// In theory you can replace this code with another subclass to change
// the keybindings.

#include <stdio.h>
#include <stdlib.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include "flstring.h"

#if defined(FL_DLL)	// really needed for c'tors for MS VC++ only
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Secret_Input.H>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif


void Fl_Input::draw() {
  if (input_type() == FL_HIDDEN_INPUT) return;
  Fl_Boxtype b = box();
  if (damage() & FL_DAMAGE_ALL) draw_box(b, color());
  Fl_Input_::drawtext(x()+Fl::box_dx(b), y()+Fl::box_dy(b),
		      w()-Fl::box_dw(b), h()-Fl::box_dh(b));
}

// kludge so shift causes selection to extend:
int Fl_Input::shift_position(int p) {
  return position(p, Fl::event_state(FL_SHIFT) ? mark() : p);
}

int Fl_Input::shift_up_down_position(int p) {
  return up_down_position(p, Fl::event_state(FL_SHIFT));
}

// Old text from FLTK 1.1 for reference:
// If you define NORMAL_INPUT_MOVE as zero you will get the peculiar fltk
// behavior where moving off the end of an input field will move the
// cursor into the next field:
// define it as 1 to prevent cursor movement from going to next field:
//
// Note: this has been replaced by Fl::option(Fl::OPTION_ARROW_FOCUS)
// in FLTK 1.3.  This option has "inverted" values:
//   1 = Arrow keys move focus (previously 0)
//   0 = Arrow keys don't move focus (previously 1)
// Hence we define ...
//
#define NORMAL_INPUT_MOVE (Fl::option(Fl::OPTION_ARROW_FOCUS) ? 0 : 1)

#define ctrl(x) ((x)^0x40)

// List of characters that are legal in a floating point input field.
// This text string is created at run-time to take the current locale
// into account (for example, continental Europe uses a comma instead
// of a decimal point). For back compatibility reasons, we always 
// allow the decimal point.
#ifdef HAVE_LOCALECONV
static const char *standard_fp_chars = ".eE+-"; 
static const char *legal_fp_chars = 0L;
#else
static const char *legal_fp_chars = ".eE+-"; 
#endif

// Move cursor up specified #lines
//    If OPTION_ARROW_FOCUS is disabled, return 1 to prevent focus navigation.
//
int Fl_Input::kf_lines_up(int repeat_num) {
  int i = position();
  if (!line_start(i)) {
    //UNNEEDED if (input_type()==FL_MULTILINE_INPUT && !Fl::option(Fl::OPTION_ARROW_FOCUS)) return 1;
    return NORMAL_INPUT_MOVE;
  }
  while(repeat_num--) {
    i = line_start(i);
    if (!i) break;
    i--;
  }
  shift_up_down_position(line_start(i));
  return 1;
}

// Move cursor down specified #lines
//    If OPTION_ARROW_FOCUS is disabled, return 1 to prevent focus navigation.
//
int Fl_Input::kf_lines_down(int repeat_num) {
  int i = position();
  if (line_end(i) >= size()) {
    //UNNEEDED if (input_type()==FL_MULTILINE_INPUT && !Fl::option(Fl::OPTION_ARROW_FOCUS)) return 1;
    return NORMAL_INPUT_MOVE;
  }
  while (repeat_num--) {  
    i = line_end(i);
    if (i >= size()) break;
    i++;
  }
  shift_up_down_position(i);
  return 1;
}

// Move up a page
int Fl_Input::kf_page_up() {
  return kf_lines_up(linesPerPage());
}

// Move down a page
int Fl_Input::kf_page_down() {
  return kf_lines_down(linesPerPage());
}

// Toggle insert mode
int Fl_Input::kf_insert_toggle() {
  if (readonly()) { fl_beep(); return 1; }
  return 1;				// \todo: needs insert mode
}

// Delete word right
int Fl_Input::kf_delete_word_right() {
  if (readonly()) { fl_beep(); return 1; }
  if (mark() != position()) return cut();
  cut(position(), word_end(position()));
  return 1;
}

// Delete word left
int Fl_Input::kf_delete_word_left() {
  if (readonly()) { fl_beep(); return 1; }
  if (mark() != position()) return cut();
  cut(word_start(position()), position());
  return 1;
}

// Delete to start of line
int Fl_Input::kf_delete_sol() {
  if (readonly()) { fl_beep(); return 1; }
  if (mark() != position()) return cut();
  cut(line_start(position()), position());
  return 1;
}

// Delete to end of line
int Fl_Input::kf_delete_eol() {
  if (readonly()) { fl_beep(); return 1; }
  if (mark() != position()) return cut();
  cut(position(), line_end(position()));
  return 1;
}

int Fl_Input::kf_delete_char_right() {
  if (readonly()) { fl_beep(); return 1; }
  if (mark() != position()) return cut();
  else return cut(1);
}

int Fl_Input::kf_delete_char_left() {
  if (readonly()) { fl_beep(); return 1; }
  if (mark() != position()) cut();
  else cut(-1);
  return 1;
}

// Move cursor to start of line
int Fl_Input::kf_move_sol() {
  return shift_position(line_start(position())) + NORMAL_INPUT_MOVE;
}

// Move cursor to end of line
int Fl_Input::kf_move_eol() {
  return shift_position(line_end(position())) + NORMAL_INPUT_MOVE;
}

// Clear to end of line
int Fl_Input::kf_clear_eol() {
  if (readonly()) { fl_beep(); return 1; }
  if (position()>=size()) return 0;
  int i = line_end(position());
  if (i == position() && i < size()) i++;
  cut(position(), i);
  return copy_cuts();
}

// Move cursor one character to the left
//    If OPTION_ARROW_FOCUS is disabled, return 1 to prevent focus navigation.
//
int Fl_Input::kf_move_char_left() {
  int i = shift_position(position()-1) + NORMAL_INPUT_MOVE;
  return Fl::option(Fl::OPTION_ARROW_FOCUS) ? i : 1;
}

// Move cursor one character to the right
//    If OPTION_ARROW_FOCUS is disabled, return 1 to prevent focus navigation.
//
int Fl_Input::kf_move_char_right() {
  int i = shift_position(position()+1) + NORMAL_INPUT_MOVE;
  return Fl::option(Fl::OPTION_ARROW_FOCUS) ? i : 1;
}

// Move cursor word-left
int Fl_Input::kf_move_word_left() {
  shift_position(word_start(position()));
  return 1; 
}

// Move cursor word-right
int Fl_Input::kf_move_word_right() {
  shift_position(word_end(position()));
  return 1;
}

// Move cursor up one line and to the start of line (paragraph up)
int Fl_Input::kf_move_up_and_sol() {
  if (line_start(position())==position() && position()>0)
    return shift_position(line_start(position()-1)) + NORMAL_INPUT_MOVE;
  else
    return shift_position(line_start(position())) + NORMAL_INPUT_MOVE;
}

// Move cursor down one line and to the end of line (paragraph down)
int Fl_Input::kf_move_down_and_eol() {
  if (line_end(position())==position() && position()<size())
    return shift_position(line_end(position()+1)) + NORMAL_INPUT_MOVE;
  else
    return shift_position(line_end(position())) + NORMAL_INPUT_MOVE;
}

// Move to top of document
int Fl_Input::kf_top() {
  shift_position(0);
  return 1;
}

// Move to bottom of document
int Fl_Input::kf_bottom() {
  shift_position(size());
  return 1; 
}

// Select all text in the widget
int Fl_Input::kf_select_all() {
  position(0,size());
  return 1;
}

// Undo.
int Fl_Input::kf_undo() {
  if (readonly()) { fl_beep(); return 1; }
  return undo();
}

// Redo. (currently unimplemented.. toggles undo() instead)
int Fl_Input::kf_redo() {
  if (readonly()) { fl_beep(); return 1; }
  return kf_undo();			// currently we don't support multilevel undo
}

// Do a copy operation
int Fl_Input::kf_copy() {
  return copy(1);
}

// Do a paste operation
int Fl_Input::kf_paste() {
  if (readonly()) { fl_beep(); return 1; }
  Fl::paste(*this, 1);
  return 1;
}

// Do a cut with copy
int Fl_Input::kf_copy_cut() {
  if (readonly()) { fl_beep(); return 1; }
  copy(1);
  return cut();
}

// Handle a keystroke.
//     Returns 1 if handled by us, 0 if not.
//
int Fl_Input::handle_key() {
  
  char ascii = Fl::event_text()[0];
  
  int del;
  if (Fl::compose(del)) {
    
    // Insert characters into numeric fields after checking for legality:
    if (input_type() == FL_FLOAT_INPUT || input_type() == FL_INT_INPUT) {
      Fl::compose_reset(); // ignore any foreign letters...
      
      // initialize the list of legal characters inside a floating point number
#ifdef HAVE_LOCALECONV
      if (!legal_fp_chars) {
        int len = strlen(standard_fp_chars);
        struct lconv *lc = localeconv();
        if (lc) {
          if (lc->decimal_point) len += strlen(lc->decimal_point);
          if (lc->mon_decimal_point) len += strlen(lc->mon_decimal_point);
          if (lc->positive_sign) len += strlen(lc->positive_sign);
          if (lc->negative_sign) len += strlen(lc->negative_sign);
        }
        // the following line is not a true memory leak because the array is only
        // allocated once if required, and automatically freed when the program quits
        char *chars = (char*)malloc(len+1);
	legal_fp_chars = chars;
        strcpy(chars, standard_fp_chars);
        if (lc) {
          if (lc->decimal_point) strcat(chars, lc->decimal_point);
          if (lc->mon_decimal_point) strcat(chars, lc->mon_decimal_point);
          if (lc->positive_sign) strcat(chars, lc->positive_sign);
          if (lc->negative_sign) strcat(chars, lc->negative_sign);
        }
      }
#endif // HAVE_LOCALECONV
      
      // find the insert position
      int ip = position()<mark() ? position() : mark();
      // This is complex to allow "0xff12" hex to be typed:
      if (   (!ip && (ascii == '+' || ascii == '-')) 
          || (ascii >= '0' && ascii <= '9') 
          || (ip==1 && index(0)=='0' && (ascii=='x' || ascii == 'X')) 
          || (ip>1 && index(0)=='0' && (index(1)=='x'||index(1)=='X') 
              && ((ascii>='A'&& ascii<='F') || (ascii>='a'&& ascii<='f'))) 
          || (input_type()==FL_FLOAT_INPUT && ascii && strchr(legal_fp_chars, ascii))) 
      {
	if (readonly()) fl_beep();
	else replace(position(), mark(), &ascii, 1);
      }
      return 1;
    }
    
    if (del || Fl::event_length()) {
      if (readonly()) fl_beep();
      else replace(position(), del ? position()-del : mark(),
	           Fl::event_text(), Fl::event_length());
    }
    return 1;
  }
  
  unsigned int mods = Fl::event_state() & (FL_META|FL_CTRL|FL_ALT);
  unsigned int shift = Fl::event_state() & FL_SHIFT;
  unsigned int multiline = (input_type() == FL_MULTILINE_INPUT) ? 1 : 0;
  //
  // The following lists apps that support these keypresses.
  // Prefixes: '!' indicates NOT supported, '?' indicates un-verified.
  //
  //    HIG=Human Interface Guide, 
  //    TE=TextEdit.app, SA=Safari.app, WOX=MS Word/OSX -- OSX 10.4.x
  //    NP=Notepad, WP=WordPad, WOW=MS Word/Windows     -- WinXP
  //    GE=gedit, KE=kedit                              -- Ubuntu8.04
  //    OF=old FLTK behavior (<=1.1.10)
  //
  // Example: (NP,WP,!WO) means supported in notepad + wordpad, but NOT word.
  //
  switch (Fl::event_key()) {

    case FL_Insert:
      // Note: Mac has no "Insert" key; it's the "Help" key.
      //       This keypress is apparently not possible on macs.
      //
      if (mods==0 && shift) return kf_paste();			// Shift-Insert   (WP,NP,WOW,GE,KE,OF)
      if (mods==0)          return kf_insert_toggle();		// Insert         (Standard)
      if (mods==FL_CTRL)    return kf_copy();			// Ctrl-Insert    (WP,NP,WOW,GE,KE,OF)
      return 0;							// ignore other combos, pass to parent

    case FL_Delete: {
#ifdef __APPLE__
      if (mods==0)          return kf_delete_char_right();	// Delete         (OSX-HIG,TE,SA,WOX)
      if (mods==FL_CTRL)    return kf_delete_char_right();	// Ctrl-Delete    (??? TE,!SA,!WOX)
      if (mods==FL_ALT)     return kf_delete_word_right();	// Alt-Delete     (OSX-HIG,TE,SA)
      return 0;							// ignore other combos, pass to parent
#else
      int selected = (position() != mark()) ? 1 : 0;
      if (mods==0 && shift && selected)
                            return kf_copy_cut();		// Shift-Delete with selection (WP,NP,WOW,GE,KE,OF)
      if (mods==0 && shift && !selected)
                            return kf_delete_char_right();	// Shift-Delete no selection (WP,NP,WOW,GE,KE,!OF)
      if (mods==0)          return kf_delete_char_right();	// Delete         (Standard)
      if (mods==FL_CTRL)    return kf_delete_word_right();	// Ctrl-Delete    (WP,!NP,WOW,GE,KE,!OF)
      return 0;							// ignore other combos, pass to parent
#endif
    }

    case FL_Left:
#ifdef __APPLE__
      if (mods==0)          return kf_move_char_left();		// Left           (OSX-HIG)
      if (mods==FL_ALT)     return kf_move_word_left();		// Alt-Left       (OSX-HIG)
      if (mods==FL_META)    return kf_move_sol();		// Meta-Left      (OSX-HIG)
      if (mods==FL_CTRL)    return kf_move_sol();		// Ctrl-Left      (TE/SA)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_move_char_left();		// Left           (WP,NP,WOW,GE,KE,OF)
      if (mods==FL_CTRL)    return kf_move_word_left();		// Ctrl-Left      (WP,NP,WOW,GE,KE,!OF)
      if (mods==FL_META)    return kf_move_char_left();		// Meta-Left      (WP,NP,?WOW,GE,KE)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_Right:
#ifdef __APPLE__
      if (mods==0)          return kf_move_char_right();	// Right          (OSX-HIG)
      if (mods==FL_ALT)     return kf_move_word_right();	// Alt-Right      (OSX-HIG)
      if (mods==FL_META)    return kf_move_eol();		// Meta-Right     (OSX-HIG)
      if (mods==FL_CTRL)    return kf_move_eol();		// Ctrl-Right     (TE/SA)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_move_char_right();	// Right          (WP,NP,WOW,GE,KE,OF)
      if (mods==FL_CTRL)    return kf_move_word_right();	// Ctrl-Right     (WP,NP,WOW,GE,KE,!OF)
      if (mods==FL_META)    return kf_move_char_right();	// Meta-Right     (WP,NP,?WOW,GE,KE,!OF)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_Up:
#ifdef __APPLE__
      if (mods==0)          return kf_lines_up(1);		// Up             (OSX-HIG)
      if (mods==FL_CTRL)    return kf_page_up();		// Ctrl-Up        (TE !HIG)
      if (mods==FL_ALT)     return kf_move_up_and_sol();	// Alt-Up         (OSX-HIG)
      if (mods==FL_META)    return kf_top();			// Meta-Up        (OSX-HIG)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_lines_up(1);		// Up             (WP,NP,WOW,GE,KE,OF)
      if (mods==FL_CTRL)    return kf_move_up_and_sol();	// Ctrl-Up        (WP,!NP,WOW,GE,!KE,OF)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_Down:
#ifdef __APPLE__
      if (mods==0)          return kf_lines_down(1);		// Dn             (OSX-HIG)
      if (mods==FL_CTRL)    return kf_page_down();		// Ctrl-Dn        (TE !HIG)
      if (mods==FL_ALT)     return kf_move_down_and_eol();	// Alt-Dn         (OSX-HIG)
      if (mods==FL_META)    return kf_bottom();			// Meta-Dn        (OSX-HIG)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_lines_down(1);		// Dn             (WP,NP,WOW,GE,KE,OF)
      if (mods==FL_CTRL)    return kf_move_down_and_eol();	// Ctrl-Down      (WP,!NP,WOW,GE,!KE,OF)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_Page_Up:
      // Fl_Input has no scroll control, so instead we move the cursor by one page
      // OSX-HIG recommends Alt increase one semantic unit, Meta next higher..
#ifdef __APPLE__
      if (mods==0)          return kf_page_up();		// PgUp           (OSX-HIG)
      if (mods==FL_ALT)     return kf_page_up();		// Alt-PageUp     (OSX-HIG)
      if (mods==FL_META)    return kf_top();			// Meta-PageUp    (OSX-HIG,!TE)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_page_up();		// PageUp         (WP,NP,WOW,GE,KE)
      if (mods==FL_CTRL)    return kf_page_up();		// Ctrl-PageUp    (!WP,!NP,!WOW,!GE,KE,OF)
      if (mods==FL_ALT)     return kf_page_up();		// Alt-PageUp     (!WP,!NP,!WOW,!GE,KE,OF)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_Page_Down:
#ifdef __APPLE__
      // Fl_Input has no scroll control, so instead we move the cursor by one page
      // OSX-HIG recommends Alt increase one semantic unit, Meta next higher..
      if (mods==0)          return kf_page_down();		// PgDn           (OSX-HIG)
      if (mods==FL_ALT)     return kf_page_down();		// Alt-PageDn     (OSX-HIG)
      if (mods==FL_META)    return kf_bottom();			// Meta-PageDn    (OSX-HIG,!TE)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_page_down();		// PageDn         (WP,NP,WOW,GE,KE)
      if (mods==FL_CTRL)    return kf_page_down();		// Ctrl-PageDn    (!WP,!NP,!WOW,!GE,KE,OF)
      if (mods==FL_ALT)     return kf_page_down();		// Alt-PageDn     (!WP,!NP,!WOW,!GE,KE,OF)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_Home:
#ifdef __APPLE__
      if (mods==0)          return kf_top();			// Home           (OSX-HIG)
      if (mods==FL_ALT)     return kf_top();			// Alt-Home       (???)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_move_sol();		// Home           (WP,NP,WOW,GE,KE,OF)
      if (mods==FL_CTRL)    return kf_top();			// Ctrl-Home      (WP,NP,WOW,GE,KE,OF)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_End:
#ifdef __APPLE__
      if (mods==0)          return kf_bottom();			// End            (OSX-HIG)
      if (mods==FL_ALT)     return kf_bottom();			// Alt-End        (???)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_move_eol();		// End            (WP,NP,WOW,GE,KE,OF)
      if (mods==FL_CTRL)    return kf_bottom();			// Ctrl-End       (WP,NP,WOW,GE,KE,OF)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_BackSpace:
#ifdef __APPLE__
      if (mods==0)          return kf_delete_char_left();	// Backspace      (OSX-HIG)
      if (mods==FL_CTRL)    return kf_delete_char_left();	// Ctrl-Backspace (TE/SA)
      if (mods==FL_ALT)     return kf_delete_word_left();	// Alt-Backspace  (OSX-HIG)
      if (mods==FL_META)    return kf_delete_sol();		// Meta-Backspace (OSX-HIG,!TE)
      return 0;							// ignore other combos, pass to parent
#else
      if (mods==0)          return kf_delete_char_left();	// Backspace      (WP,NP,WOW,GE,KE,OF)
      if (mods==FL_CTRL)    return kf_delete_word_left();	// Ctrl-Backspace (WP,!NP,WOW,GE,KE,!OF)
      return 0;							// ignore other combos, pass to parent
#endif

    case FL_Enter:
    case FL_KP_Enter:
      if (when() & FL_WHEN_ENTER_KEY) {
        position(size(), 0);
        maybe_do_callback();
        return 1;
      } else if (multiline && !readonly()) {
        return replace(position(), mark(), "\n", 1);
      } return 0;			// reserved for shortcuts

    case FL_Tab:
      // Handle special case for multiline input with 'old tab behavior';
      // tab handled as a normal insertable character.
      //
      if (mods==0 && !shift 		// Tab?
	   && !tab_nav()		// old tab behavior enabled?
	   && multiline) {		// multiline input?
        break;				// insert tab character
      }
      if (mods==0) return 0;					// Tab, Shift-Tab? nav focus      (Standard/OSX-HIG)
      return 0;							// ignore other combos, pass to parent

    case 'a':
      if (mods==FL_COMMAND) return kf_select_all();		// Ctrl-A, Mac:Meta-A             (Standard/OSX-HIG)
      break;							// handle other combos elsewhere
    case 'c':
      if (mods==FL_COMMAND) return kf_copy();			// Ctrl-C, Mac:Meta-C             (Standard/OSX-HIG)
      break;							// handle other combos elsewhere
    case 'v':
      if (mods==FL_COMMAND) return kf_paste();			// Ctrl-V, Mac:Meta-V             (Standard/OSX-HIG)
      break;							// handle other combos elsewhere
    case 'x':
      if (mods==FL_COMMAND) return kf_copy_cut();		// Ctrl-X, Mac:Meta-X             (Standard/OSX-HIG)
      break;
    case 'z':
      if (mods==FL_COMMAND && !shift) return kf_undo();		// Ctrl-Z, Mac:Meta-Z             (Standard/OSX-HIG)
      if (mods==FL_COMMAND && shift)  return kf_redo();		// Shift-Ctrl-Z, Mac:Shift-Meta-Z (Standard/OSX-HIG)
      break;							// handle other combos elsewhere
  }
  
  switch (ascii) {
    case ctrl('H'):
      return kf_delete_char_left();				// Ctrl-H                           (!WP,!NP,!WOW,!WOX,TE,SA,GE,KE,OF)
    case ctrl('I'): 						// Ctrl-I (literal Tab)             (!WP,NP,!WOW,!GE,KE,OF)
    case ctrl('J'):						// Ctrl-J (literal Line Feed/Enter) (Standard)
    case ctrl('L'):						// Ctrl-L (literal Form Feed)       (Standard)
    case ctrl('M'):						// Ctrl-M (literal Cr)              (Standard)
      if (readonly()) { fl_beep(); return 1; }
      // insert a few selected control characters literally:
      if (input_type() != FL_FLOAT_INPUT && input_type() != FL_INT_INPUT)
        return replace(position(), mark(), &ascii, 1);
      break;
  }
  
  return 0;		// ignored
}

int Fl_Input::handle(int event) {
  static int dnd_save_position, dnd_save_mark, drag_start = -1, newpos;
  static Fl_Widget *dnd_save_focus;
  switch (event) {
    case FL_FOCUS:
      switch (Fl::event_key()) {
        case FL_Right:
          position(0);
          break;
        case FL_Left:
          position(size());
          break;
        case FL_Down:
          up_down_position(0);
          break;
        case FL_Up:
          up_down_position(line_start(size()));
          break;
        case FL_Tab:
          position(size(),0);
          break;
        default:
          position(position(),mark());// turns off the saved up/down arrow position
          break;
      }
      break;
      
    case FL_KEYBOARD:
      // Handle special case for multiline input with 'old tab behavior'
      // where tab is entered as a character: make sure user attempt to 'tab over'
      // widget doesn't destroy the field, replacing it with a tab character.
      //
      if (Fl::event_key() == FL_Tab 			// Tab key?
          && !Fl::event_state(FL_SHIFT)			// no shift?
          && !tab_nav()					// with tab navigation disabled?
	  && input_type() == FL_MULTILINE_INPUT		// with a multiline input?
          && (mark()==0 && position()==size())) {	// while entire field selected?
        // Set cursor to the end of the selection...
        if (mark() > position())
          position(mark());
        else
          position(position());
        return (1);
      } else {
        if (active_r() && window() && this == Fl::belowmouse()) 
          window()->cursor(FL_CURSOR_NONE);
        return handle_key();
      }
      //NOTREACHED
      
    case FL_PUSH:
      if (Fl::dnd_text_ops()) {
        int oldpos = position(), oldmark = mark();
        Fl_Boxtype b = box();
        Fl_Input_::handle_mouse(x()+Fl::box_dx(b), y()+Fl::box_dy(b),
                                w()-Fl::box_dw(b), h()-Fl::box_dh(b), 0);
        newpos = position(); 
        position( oldpos, oldmark );
        if (Fl::focus()==this && !Fl::event_state(FL_SHIFT) && input_type()!=FL_SECRET_INPUT &&
           ( (newpos >= mark() && newpos < position()) || 
             (newpos >= position() && newpos < mark()) ) ) {
          // user clicked in the selection, may be trying to drag
          drag_start = newpos;
          return 1;
        }
        drag_start = -1;
      }
      
      if (Fl::focus() != this) {
        Fl::focus(this);
        handle(FL_FOCUS);
      }
      break;
      
    case FL_DRAG:
      if (Fl::dnd_text_ops()) {
        if (drag_start >= 0) {
          if (Fl::event_is_click()) return 1; // debounce the mouse
                                              // save the position because sometimes we don't get DND_ENTER:
          dnd_save_position = position();
          dnd_save_mark = mark();
          // drag the data:
          copy(0); Fl::dnd();
          return 1;
        }
      }
      break;
      
    case FL_RELEASE:
      if (Fl::event_button() == 2) {
        Fl::event_is_click(0); // stop double click from picking a word
        Fl::paste(*this, 0);
      } else if (!Fl::event_is_click()) {
        // copy drag-selected text to the clipboard.
        copy(0);
      } else if (Fl::event_is_click() && drag_start >= 0) {
        // user clicked in the field and wants to reset the cursor position...
        position(drag_start, drag_start);
        drag_start = -1;
      } else if (Fl::event_clicks()) {
        // user double or triple clicked to select word or whole text
        copy(0);
      }
      
      // For output widgets, do the callback so the app knows the user
      // did something with the mouse...
      if (readonly()) do_callback();
      
      return 1;
      
    case FL_DND_ENTER:
      Fl::belowmouse(this); // send the leave events first
      dnd_save_position = position();
      dnd_save_mark = mark();
      dnd_save_focus = Fl::focus();
      if (dnd_save_focus != this) {
        Fl::focus(this);
        handle(FL_FOCUS);
      }
      // fall through:
    case FL_DND_DRAG: 
      //int p = mouse_position(X, Y, W, H);
#if DND_OUT_XXXX
      if (Fl::focus()==this && (p>=dnd_save_position && p<=dnd_save_mark ||
                                p>=dnd_save_mark && p<=dnd_save_position)) {
        position(dnd_save_position, dnd_save_mark);
        return 0;
      }
#endif
      {
        Fl_Boxtype b = box();
        Fl_Input_::handle_mouse(x()+Fl::box_dx(b), y()+Fl::box_dy(b),
                                w()-Fl::box_dw(b), h()-Fl::box_dh(b), 0);
      }
      return 1;
      
    case FL_DND_LEAVE:
      position(dnd_save_position, dnd_save_mark);
#if DND_OUT_XXXX
      if (!focused())
#endif
        if (dnd_save_focus != this) {
          Fl::focus(dnd_save_focus);
          handle(FL_UNFOCUS);
        }
#if !(defined(__APPLE__) || defined(WIN32))
      Fl::first_window()->cursor(FL_CURSOR_MOVE);
#endif
      return 1;
      
    case FL_DND_RELEASE:
      take_focus();
      return 1;
      
      /* TODO: this will scroll the area, but stop if the cursor would become invisible.
       That clipping happens in drawtext(). Do we change the clipping or should 
       we move the cursor (ouch)?
       case FL_MOUSEWHEEL:
       if (Fl::e_dy > 0) {
       yscroll( yscroll() - Fl::e_dy*15 );
       } else if (Fl::e_dy < 0) {
       yscroll( yscroll() - Fl::e_dy*15 );
       }
       return 1;
       */
  }
  Fl_Boxtype b = box();
  return Fl_Input_::handletext(event,
                               x()+Fl::box_dx(b), y()+Fl::box_dy(b),
                               w()-Fl::box_dw(b), h()-Fl::box_dh(b));
}

/**
 Creates a new Fl_Input widget using the given position, size,
 and label string. The default boxtype is FL_DOWN_BOX.
 */
Fl_Input::Fl_Input(int X, int Y, int W, int H, const char *l)
: Fl_Input_(X, Y, W, H, l) {
}

/*
  The following constructors must not be in the header file(s) if we
  build a shared object (DLL). Instead they are defined here to force
  the constructor (and default destructor as well) to be defined in
  the DLL and exported (STR #2632).
  
  Note: if you change any of them, do the same changes in the specific
  header file as well.  This redundant definition was chosen to enable
  inline constructors in the header files (for static linking) as well
  as those here for dynamic linking (Windows DLL).
*/
#if defined(FL_DLL)

Fl_Float_Input::Fl_Float_Input(int X,int Y,int W,int H,const char *l)
    : Fl_Input(X,Y,W,H,l) {
  type(FL_FLOAT_INPUT);
}

Fl_Int_Input::Fl_Int_Input(int X,int Y,int W,int H,const char *l)
    : Fl_Input(X,Y,W,H,l) {
  type(FL_INT_INPUT);
}

Fl_Multiline_Input::Fl_Multiline_Input(int X,int Y,int W,int H,const char *l)
    : Fl_Input(X,Y,W,H,l) {
  type(FL_MULTILINE_INPUT);
}

Fl_Output::Fl_Output(int X,int Y,int W,int H, const char *l)
    : Fl_Input(X, Y, W, H, l) {
  type(FL_NORMAL_OUTPUT);
}

Fl_Multiline_Output::Fl_Multiline_Output(int X,int Y,int W,int H,const char *l)
    : Fl_Output(X,Y,W,H,l) {
  type(FL_MULTILINE_OUTPUT);
}

Fl_Secret_Input::Fl_Secret_Input(int X,int Y,int W,int H,const char *l)
    : Fl_Input(X,Y,W,H,l) {
  type(FL_SECRET_INPUT);
}

#endif // FL_DLL

//
// End of "$Id: Fl_Input.cxx 8726 2011-05-23 18:32:47Z AlbrechtS $".
//
