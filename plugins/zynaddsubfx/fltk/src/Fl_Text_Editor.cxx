//
// "$Id: Fl_Text_Editor.cxx 8034 2010-12-15 12:21:55Z AlbrechtS $"
//
// Copyright 2001-2010 by Bill Spitzak and others.
// Original code Copyright Mark Edel.  Permission to distribute under
// the LGPL for the FLTK library granted by Mark Edel.
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

#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <ctype.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/fl_ask.H>


/* Keyboard Control Matrix

key\modifier   plain  Ctrl   Alt  Meta  
  left          1/1  13/9   0/13  0/9  
  right         2/2  14/10  0/14  0/10  
  up            3/19 21/7   0/15  0/17 
  down          4/20 22/8   0/16  0/18
  home          9/5  17/0   0/0   0/0
  end          10/6  18/0   0/0   0/0
  page up      11/7  23/0   0/11  0/0
  page down    12/8  24/0   0/12  0/0
    (FLTK action / OS X action)
    (adding the shift key extends the selection, all other combinations are no-op)

  0: no-op
  1: move cursor to the left, at line beginning wrap to end of prev line, at doc start no-op
  2: move cursor to the right, at line end move to beginning of the next line, at doc end no-op
  3: move cursor up, at doc top no-op
  4: move cursor down, at doc bottom no-op
  5: scroll display to top of text (cursor unchanged)
  6: scroll display to end of text (cursor unchanged)
  7: scroll text down one page (cursor unchanged)
  8: scroll text up one page (cursor unchanged)
  9: move cursor to beginning of line
 10: move cursor to end of line
 11: move cursor up one page and scroll down
 12: move cursor down one page and scroll up
 13: move to the beginning of the word or the previous word
 14: move to the end of the word or the next word
 15: if start of line: start of prev line, else start of this line
 16: if end of line: end of next line, else end of this line
 17: move cursor to the beginning of the document
 18: move cursor to the end of the document
 19: move cursor up, at doc top: home, at doc start: no-op)
 20: move cursor down, at doc bot: end, at doc end: no-op)
 21: scroll text down one line (cursor unchanged)
 22: scroll text up one line (cursor unchanged)
 23: move cursor to the beginning of the top of the screen
 24: move cursor to the beginning of the bottom of the window
*/

/**  The constructor creates a new text editor widget.*/
Fl_Text_Editor::Fl_Text_Editor(int X, int Y, int W, int H,  const char* l)
    : Fl_Text_Display(X, Y, W, H, l) {
  mCursorOn = 1;
  insert_mode_ = 1;
  key_bindings = 0;

  // handle the default key bindings
  add_default_key_bindings(&key_bindings);

  // handle everything else
  default_key_function(kf_default);
}

#ifndef FL_DOXYGEN
Fl_Text_Editor::Key_Binding* Fl_Text_Editor::global_key_bindings = 0;
#endif

// These are the default key bindings every widget should start with
static struct {
  int key;
  int state;
  Fl_Text_Editor::Key_Func func;
} default_key_bindings[] = {
  { FL_Escape,    FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor::kf_ignore     },
  { FL_Enter,     FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor::kf_enter      },
  { FL_KP_Enter,  FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor::kf_enter      },
  { FL_BackSpace, FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor::kf_backspace  },
  { FL_Insert,    FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor::kf_insert     },
  { FL_Delete,    FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor::kf_delete     },
  { FL_Home,      0,                        Fl_Text_Editor::kf_move       },
  { FL_End,       0,                        Fl_Text_Editor::kf_move       },
  { FL_Left,      0,                        Fl_Text_Editor::kf_move       },
  { FL_Up,        0,                        Fl_Text_Editor::kf_move       },
  { FL_Right,     0,                        Fl_Text_Editor::kf_move       },
  { FL_Down,      0,                        Fl_Text_Editor::kf_move       },
  { FL_Page_Up,   0,                        Fl_Text_Editor::kf_move       },
  { FL_Page_Down, 0,                        Fl_Text_Editor::kf_move       },
  { FL_Home,      FL_SHIFT,                 Fl_Text_Editor::kf_shift_move },
  { FL_End,       FL_SHIFT,                 Fl_Text_Editor::kf_shift_move },
  { FL_Left,      FL_SHIFT,                 Fl_Text_Editor::kf_shift_move },
  { FL_Up,        FL_SHIFT,                 Fl_Text_Editor::kf_shift_move },
  { FL_Right,     FL_SHIFT,                 Fl_Text_Editor::kf_shift_move },
  { FL_Down,      FL_SHIFT,                 Fl_Text_Editor::kf_shift_move },
  { FL_Page_Up,   FL_SHIFT,                 Fl_Text_Editor::kf_shift_move },
  { FL_Page_Down, FL_SHIFT,                 Fl_Text_Editor::kf_shift_move },
  { FL_Home,      FL_CTRL,                  Fl_Text_Editor::kf_ctrl_move  },
  { FL_End,       FL_CTRL,                  Fl_Text_Editor::kf_ctrl_move  },
  { FL_Left,      FL_CTRL,                  Fl_Text_Editor::kf_ctrl_move  },
  { FL_Up,        FL_CTRL,                  Fl_Text_Editor::kf_ctrl_move  },
  { FL_Right,     FL_CTRL,                  Fl_Text_Editor::kf_ctrl_move  },
  { FL_Down,      FL_CTRL,                  Fl_Text_Editor::kf_ctrl_move  },
  { FL_Page_Up,   FL_CTRL,                  Fl_Text_Editor::kf_ctrl_move  },
  { FL_Page_Down, FL_CTRL,                  Fl_Text_Editor::kf_ctrl_move  },
  { FL_Home,      FL_CTRL|FL_SHIFT,         Fl_Text_Editor::kf_c_s_move   },
  { FL_End,       FL_CTRL|FL_SHIFT,         Fl_Text_Editor::kf_c_s_move   },
  { FL_Left,      FL_CTRL|FL_SHIFT,         Fl_Text_Editor::kf_c_s_move   },
  { FL_Up,        FL_CTRL|FL_SHIFT,         Fl_Text_Editor::kf_c_s_move   },
  { FL_Right,     FL_CTRL|FL_SHIFT,         Fl_Text_Editor::kf_c_s_move   },
  { FL_Down,      FL_CTRL|FL_SHIFT,         Fl_Text_Editor::kf_c_s_move   },
  { FL_Page_Up,   FL_CTRL|FL_SHIFT,         Fl_Text_Editor::kf_c_s_move   },
  { FL_Page_Down, FL_CTRL|FL_SHIFT,         Fl_Text_Editor::kf_c_s_move   },
//{ FL_Clear,	  0,                        Fl_Text_Editor::delete_to_eol },
  { 'z',          FL_CTRL,                  Fl_Text_Editor::kf_undo	  },
  { '/',          FL_CTRL,                  Fl_Text_Editor::kf_undo	  },
  { 'x',          FL_CTRL,                  Fl_Text_Editor::kf_cut        },
  { FL_Delete,    FL_SHIFT,                 Fl_Text_Editor::kf_cut        },
  { 'c',          FL_CTRL,                  Fl_Text_Editor::kf_copy       },
  { FL_Insert,    FL_CTRL,                  Fl_Text_Editor::kf_copy       },
  { 'v',          FL_CTRL,                  Fl_Text_Editor::kf_paste      },
  { FL_Insert,    FL_SHIFT,                 Fl_Text_Editor::kf_paste      },
  { 'a',          FL_CTRL,                  Fl_Text_Editor::kf_select_all },

#ifdef __APPLE__
  // Define CMD+key accelerators...
  { 'z',          FL_COMMAND,               Fl_Text_Editor::kf_undo       },
  { 'x',          FL_COMMAND,               Fl_Text_Editor::kf_cut        },
  { 'c',          FL_COMMAND,               Fl_Text_Editor::kf_copy       },
  { 'v',          FL_COMMAND,               Fl_Text_Editor::kf_paste      },
  { 'a',          FL_COMMAND,               Fl_Text_Editor::kf_select_all },
  { FL_Left,      FL_COMMAND,               Fl_Text_Editor::kf_meta_move  },
  { FL_Right,     FL_COMMAND,               Fl_Text_Editor::kf_meta_move  },
  { FL_Up,        FL_COMMAND,               Fl_Text_Editor::kf_meta_move  },
  { FL_Down,      FL_COMMAND,               Fl_Text_Editor::kf_meta_move  },
  { FL_Left,      FL_COMMAND|FL_SHIFT,      Fl_Text_Editor::kf_m_s_move   },
  { FL_Right,     FL_COMMAND|FL_SHIFT,      Fl_Text_Editor::kf_m_s_move   },
  { FL_Up,        FL_COMMAND|FL_SHIFT,      Fl_Text_Editor::kf_m_s_move   },
  { FL_Down,      FL_COMMAND|FL_SHIFT,      Fl_Text_Editor::kf_m_s_move   },
#endif // __APPLE__

  { 0,            0,                        0                             }
};

/**  Adds all of the default editor key bindings to the specified key binding list.*/
void Fl_Text_Editor::add_default_key_bindings(Key_Binding** list) {
  for (int i = 0; default_key_bindings[i].key; i++) {
    add_key_binding(default_key_bindings[i].key,
                    default_key_bindings[i].state,
                    default_key_bindings[i].func,
                    list);
  }
}

/**  Returns the function associated with a key binding.*/
Fl_Text_Editor::Key_Func Fl_Text_Editor::bound_key_function(int key, int state, Key_Binding* list) {
  Key_Binding* cur;
  for (cur = list; cur; cur = cur->next)
    if (cur->key == key)
      if (cur->state == FL_TEXT_EDITOR_ANY_STATE || cur->state == state)
        break;
  if (!cur) return 0;
  return cur->function;
}

/**  Removes all of the key bindings associated with the text editor or list.*/
void Fl_Text_Editor::remove_all_key_bindings(Key_Binding** list) {
  Key_Binding *cur, *next;
  for (cur = *list; cur; cur = next) {
    next = cur->next;
    delete cur;
  }
  *list = 0;
}

/** Removes the key binding associated with the key "key" of state "state" */
void Fl_Text_Editor::remove_key_binding(int key, int state, Key_Binding** list) {
  Key_Binding *cur, *last = 0;
  for (cur = *list; cur; last = cur, cur = cur->next)
    if (cur->key == key && cur->state == state) break;
  if (!cur) return;
  if (last) last->next = cur->next;
  else *list = cur->next;
  delete cur;
}
/** Adds a key of state "state" with the function "function" */
void Fl_Text_Editor::add_key_binding(int key, int state, Key_Func function,
                                Key_Binding** list) {
  Key_Binding* kb = new Key_Binding;
  kb->key = key;
  kb->state = state;
  kb->function = function;
  kb->next = *list;
  *list = kb;
}

////////////////////////////////////////////////////////////////

static void kill_selection(Fl_Text_Editor* e) {
  if (e->buffer()->selected()) {
    e->insert_position(e->buffer()->primary_selection()->start());
    e->buffer()->remove_selection();
  }
}

/** Inserts the text associated with the key */
int Fl_Text_Editor::kf_default(int c, Fl_Text_Editor* e) {
  // FIXME: this function is a mess! Fix this!
  if (!c || (!isprint(c) && c != '\t')) return 0;
  char s[2] = "\0";
  s[0] = (char)c;
  kill_selection(e);
  if (e->insert_mode()) e->insert(s);
  else e->overstrike(s);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

/** Ignores the keypress */
int Fl_Text_Editor::kf_ignore(int, Fl_Text_Editor*) {
  return 0; // don't handle
}
/**  Does a backspace in the current buffer.*/
int Fl_Text_Editor::kf_backspace(int, Fl_Text_Editor* e) {
  if (!e->buffer()->selected() && e->move_left()) {
    int p1 = e->insert_position();
    int p2 = e->buffer()->next_char(p1);
    e->buffer()->select(p1, p2);
  }
  kill_selection(e);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

/** Inserts a newline at the current cursor position */
int Fl_Text_Editor::kf_enter(int, Fl_Text_Editor* e) {
  kill_selection(e);
  e->insert("\n");
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

extern void fl_text_drag_me(int pos, Fl_Text_Display* d);
/**  Moves the text cursor in the direction indicated by key c.*/
int Fl_Text_Editor::kf_move(int c, Fl_Text_Editor* e) {
  int i;
  int selected = e->buffer()->selected();
  if (!selected)
    e->dragPos = e->insert_position();
  e->buffer()->unselect();
  Fl::copy("", 0, 0);
  switch (c) {
  case FL_Home:
      e->insert_position(e->buffer()->line_start(e->insert_position()));
      break;
    case FL_End:
      e->insert_position(e->buffer()->line_end(e->insert_position()));
      break;
    case FL_Left:
      e->move_left();
      break;
    case FL_Right:
      e->move_right();
      break;
    case FL_Up:
      e->move_up();
      break;
    case FL_Down:
      e->move_down();
      break;
    case FL_Page_Up:
      for (i = 0; i < e->mNVisibleLines - 1; i++) e->move_up();
      break;
    case FL_Page_Down:
      for (i = 0; i < e->mNVisibleLines - 1; i++) e->move_down();
      break;
  }
  e->show_insert_position();
  return 1;
}

/**  Extends the current selection in the direction of key c.*/
int Fl_Text_Editor::kf_shift_move(int c, Fl_Text_Editor* e) {
  kf_move(c, e);
  fl_text_drag_me(e->insert_position(), e);
  char *copy = e->buffer()->selection_text();
  if (copy) {
    Fl::copy(copy, strlen(copy), 0);
    free(copy);
    }
  return 1;
}
/** Moves the current text cursor in the direction indicated by control key */
int Fl_Text_Editor::kf_ctrl_move(int c, Fl_Text_Editor* e) {
  if (!e->buffer()->selected())
    e->dragPos = e->insert_position();
  if (c != FL_Up && c != FL_Down) {
    e->buffer()->unselect();
    Fl::copy("", 0, 0);
    e->show_insert_position();
  }
  switch (c) {
    case FL_Home:
      e->insert_position(0);
      e->scroll(0, 0);
      break;
    case FL_End:
      e->insert_position(e->buffer()->length());
      e->scroll(e->count_lines(0, e->buffer()->length(), 1), 0);
      break;
    case FL_Left:
      e->previous_word();
      break;
    case FL_Right:
      e->next_word();
      break;
    case FL_Up:
      e->scroll(e->mTopLineNum-1, e->mHorizOffset);
      break;
    case FL_Down:
      e->scroll(e->mTopLineNum+1, e->mHorizOffset);
      break;
    case FL_Page_Up:
      e->insert_position(e->mLineStarts[0]);
      break;
    case FL_Page_Down:
      e->insert_position(e->mLineStarts[e->mNVisibleLines-2]);
      break;
  }
  return 1;
}

/** Moves the current text cursor in the direction indicated by meta key */
int Fl_Text_Editor::kf_meta_move(int c, Fl_Text_Editor* e) {
  if (!e->buffer()->selected())
    e->dragPos = e->insert_position();
  if (c != FL_Up && c != FL_Down) {
    e->buffer()->unselect();
    Fl::copy("", 0, 0);
    e->show_insert_position();
  }
  switch (c) {
    case FL_Up:				// top of buffer
      e->insert_position(0);
      e->scroll(0, 0);
      break;
    case FL_Down:			// end of buffer
      e->insert_position(e->buffer()->length());
      e->scroll(e->count_lines(0, e->buffer()->length(), 1), 0);
      break;
    case FL_Left:			// beginning of line
      kf_move(FL_Home, e);
      break;
    case FL_Right:			// end of line
      kf_move(FL_End, e);
      break;
  }
  return 1;
}

/** Extends the current selection in the direction indicated by meta key c. */
int Fl_Text_Editor::kf_m_s_move(int c, Fl_Text_Editor* e) {
  kf_meta_move(c, e);
  fl_text_drag_me(e->insert_position(), e);
  return 1;
}

/** Extends the current selection in the direction indicated by control key c. */
int Fl_Text_Editor::kf_c_s_move(int c, Fl_Text_Editor* e) {
  kf_ctrl_move(c, e);
  fl_text_drag_me(e->insert_position(), e);
  return 1;
}

/**  Moves the text cursor to the beginning of the current line.*/
int Fl_Text_Editor::kf_home(int, Fl_Text_Editor* e) {
    return kf_move(FL_Home, e);
}

/**  Moves the text cursor to the end of the current line.*/
int Fl_Text_Editor::kf_end(int, Fl_Text_Editor* e) {
  return kf_move(FL_End, e);
}

/**  Moves the text cursor one character to the left.*/
int Fl_Text_Editor::kf_left(int, Fl_Text_Editor* e) {
  return kf_move(FL_Left, e);
}

/**  Moves the text cursor one line up.*/
int Fl_Text_Editor::kf_up(int, Fl_Text_Editor* e) {
  return kf_move(FL_Up, e);
}

/**  Moves the text cursor one character to the right.*/
int Fl_Text_Editor::kf_right(int, Fl_Text_Editor* e) {
  return kf_move(FL_Right, e);
}
/**  Moves the text cursor one line down.*/
int Fl_Text_Editor::kf_down(int, Fl_Text_Editor* e) {
  return kf_move(FL_Down, e);
}

/**  Moves the text cursor up one page.*/
int Fl_Text_Editor::kf_page_up(int, Fl_Text_Editor* e) {
  return kf_move(FL_Page_Up, e);
}

/**  Moves the text cursor down one page.*/
int Fl_Text_Editor::kf_page_down(int, Fl_Text_Editor* e) {
  return kf_move(FL_Page_Down, e);
}
/**  Toggles the insert mode in the text editor.*/
int Fl_Text_Editor::kf_insert(int, Fl_Text_Editor* e) {
  e->insert_mode(e->insert_mode() ? 0 : 1);
  return 1;
}

/**  Does a delete of selected text or the current character in the current buffer.*/
int Fl_Text_Editor::kf_delete(int, Fl_Text_Editor* e) {
  if (!e->buffer()->selected()) {
    int p1 = e->insert_position();
    int p2 = e->buffer()->next_char(p1);
    e->buffer()->select(p1, p2);
  }

  kill_selection(e);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

/**  Does a copy of selected text or the current character in the current buffer.*/
int Fl_Text_Editor::kf_copy(int, Fl_Text_Editor* e) {
  if (!e->buffer()->selected()) return 1;
  const char *copy = e->buffer()->selection_text();
  if (*copy) Fl::copy(copy, strlen(copy), 1);
  free((void*)copy);
  e->show_insert_position();
  return 1;
}

/**  Does a cut of selected text in the current buffer.*/
int Fl_Text_Editor::kf_cut(int c, Fl_Text_Editor* e) {
  kf_copy(c, e);
  kill_selection(e);
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

/**  Does a paste of selected text in the current buffer.*/
int Fl_Text_Editor::kf_paste(int, Fl_Text_Editor* e) {
  kill_selection(e);
  Fl::paste(*e, 1);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

/**  Selects all text in the current buffer.*/
int Fl_Text_Editor::kf_select_all(int, Fl_Text_Editor* e) {
  e->buffer()->select(0, e->buffer()->length());
  const char *copy = e->buffer()->selection_text();
  if (*copy) Fl::copy(copy, strlen(copy), 0);
  free((void*)copy);
  return 1;
}
/**  Undo last edit in the current buffer. Also deselect previous selection. */
int Fl_Text_Editor::kf_undo(int , Fl_Text_Editor* e) {
  e->buffer()->unselect();
  Fl::copy("", 0, 0);
  int crsr;
  int ret = e->buffer()->undo(&crsr);
  e->insert_position(crsr);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return ret;
}

/** Handles a key press in the editor */
int Fl_Text_Editor::handle_key() {
  // Call FLTK's rules to try to turn this into a printing character.
  // This uses the right-hand ctrl key as a "compose prefix" and returns
  // the changes that should be made to the text, as a number of
  // bytes to delete and a string to insert:
  int del = 0;
  if (Fl::compose(del)) {
    if (del) {
      int dp = insert_position(), di = del;
      while (di--) dp = buffer()->prev_char_clipped(dp);
      buffer()->select(dp, insert_position());
    }
    kill_selection(this);
    if (Fl::event_length()) {
      if (insert_mode()) insert(Fl::event_text());
      else overstrike(Fl::event_text());
    }
    show_insert_position();
    set_changed();
    if (when()&FL_WHEN_CHANGED) do_callback();
    return 1;
  }

  int key = Fl::event_key(), state = Fl::event_state(), c = Fl::event_text()[0];
  state &= FL_SHIFT|FL_CTRL|FL_ALT|FL_META; // only care about these states
  Key_Func f;
  f = bound_key_function(key, state, global_key_bindings);
  if (!f) f = bound_key_function(key, state, key_bindings);
  if (f) return f(key, this);
  if (default_key_function_ && !state) return default_key_function_(c, this);
  return 0;
}

/** does or does not a callback according to changed() and when() settings */
void Fl_Text_Editor::maybe_do_callback() {
//  printf("Fl_Text_Editor::maybe_do_callback()\n");
//  printf("changed()=%d, when()=%x\n", changed(), when());
  if (changed() || (when()&FL_WHEN_NOT_CHANGED)) do_callback();
}

int Fl_Text_Editor::handle(int event) {
  static int dndCursorPos;
  
  if (!buffer()) return 0;

  switch (event) {
    case FL_FOCUS:
      show_cursor(mCursorOn); // redraws the cursor
      if (buffer()->selected()) redraw(); // Redraw selections...
      Fl::focus(this);
      return 1;

    case FL_UNFOCUS:
      show_cursor(mCursorOn); // redraws the cursor
      if (buffer()->selected()) redraw(); // Redraw selections...
    case FL_HIDE:
      if (when() & FL_WHEN_RELEASE) maybe_do_callback();
      return 1;

    case FL_KEYBOARD:
      if (active_r() && window() && this == Fl::belowmouse()) 
        window()->cursor(FL_CURSOR_NONE);
      return handle_key();

    case FL_PASTE:
      if (!Fl::event_text()) {
        fl_beep();
	return 1;
      }
      buffer()->remove_selection();
      if (insert_mode()) insert(Fl::event_text());
      else overstrike(Fl::event_text());
      show_insert_position();
      set_changed();
      if (when()&FL_WHEN_CHANGED) do_callback();
      return 1;

    case FL_ENTER:
// MRS: WIN32 only?  Need to test!
//    case FL_MOVE:
      show_cursor(mCursorOn);
      return 1;

    case FL_PUSH:
      if (Fl::event_button() == 2) {
        // don't let the text_display see this event
        if (Fl_Group::handle(event)) return 1;
        dragType = DRAG_NONE;
	if(buffer()->selected()) {
	  buffer()->unselect();
	  }
	int pos = xy_to_position(Fl::event_x(), Fl::event_y(), CURSOR_POS);
        insert_position(pos);
        Fl::paste(*this, 0);
        Fl::focus(this);
        set_changed();
        if (when()&FL_WHEN_CHANGED) do_callback();
        return 1;
      }
      break;

    case FL_SHORTCUT:
      if (!(shortcut() ? Fl::test_shortcut(shortcut()) : test_shortcut()))
        return 0;
      if (Fl::visible_focus() && handle(FL_FOCUS)) {
        Fl::focus(this);
        return 1;
      }
      break;
      
      // Handle drag'n'drop attempt by the user. This is a simplified 
      // implementation which allows dnd operations onto the scroll bars.
    case FL_DND_ENTER: // save the current cursor position
      if (Fl::visible_focus() && handle(FL_FOCUS))
        Fl::focus(this);
      show_cursor(mCursorOn);
      dndCursorPos = insert_position();
      /* fall through */
    case FL_DND_DRAG: // show a temporary insertion cursor
      insert_position(xy_to_position(Fl::event_x(), Fl::event_y(), CURSOR_POS));
      return 1;      
    case FL_DND_LEAVE: // restore original cursor
      insert_position(dndCursorPos);
      return 1;      
    case FL_DND_RELEASE: // keep insertion cursor and wait for the FL_PASTE event
      buffer()->unselect(); // FL_PASTE must not destroy current selection!
      return 1;
  }

  return Fl_Text_Display::handle(event);
}

//
// End of "$Id: Fl_Text_Editor.cxx 8034 2010-12-15 12:21:55Z AlbrechtS $".
//
