//
// "$Id: Fl_Text_Editor.cxx 5671 2007-02-08 07:58:47Z matt $"
//
// Copyright 2001-2006 by Bill Spitzak and others.
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

Fl_Text_Editor::Key_Binding* Fl_Text_Editor::global_key_bindings = 0;

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
#endif // __APPLE__

  { 0,            0,                        0                             }
};

void Fl_Text_Editor::add_default_key_bindings(Key_Binding** list) {
  for (int i = 0; default_key_bindings[i].key; i++) {
    add_key_binding(default_key_bindings[i].key,
                    default_key_bindings[i].state,
                    default_key_bindings[i].func,
                    list);
  }
}

Fl_Text_Editor::Key_Func
Fl_Text_Editor::bound_key_function(int key, int state, Key_Binding* list) {
  Key_Binding* cur;
  for (cur = list; cur; cur = cur->next)
    if (cur->key == key)
      if (cur->state == FL_TEXT_EDITOR_ANY_STATE || cur->state == state)
        break;
  if (!cur) return 0;
  return cur->function;
}

void
Fl_Text_Editor::remove_all_key_bindings(Key_Binding** list) {
  Key_Binding *cur, *next;
  for (cur = *list; cur; cur = next) {
    next = cur->next;
    delete cur;
  }
  *list = 0;
}

void
Fl_Text_Editor::remove_key_binding(int key, int state, Key_Binding** list) {
  Key_Binding *cur, *last = 0;
  for (cur = *list; cur; last = cur, cur = cur->next)
    if (cur->key == key && cur->state == state) break;
  if (!cur) return;
  if (last) last->next = cur->next;
  else *list = cur->next;
  delete cur;
}

void
Fl_Text_Editor::add_key_binding(int key, int state, Key_Func function,
                                Key_Binding** list) {
  Key_Binding* kb = new Key_Binding;
  kb->key = key;
  kb->state = state;
  kb->function = function;
  kb->next = *list;
  *list = kb;
}

////////////////////////////////////////////////////////////////

#define NORMAL_INPUT_MOVE 0

static void kill_selection(Fl_Text_Editor* e) {
  if (e->buffer()->selected()) {
    e->insert_position(e->buffer()->primary_selection()->start());
    e->buffer()->remove_selection();
  }
}

int Fl_Text_Editor::kf_default(int c, Fl_Text_Editor* e) {
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

int Fl_Text_Editor::kf_ignore(int, Fl_Text_Editor*) {
  return 0; // don't handle
}

int Fl_Text_Editor::kf_backspace(int, Fl_Text_Editor* e) {
  if (!e->buffer()->selected() && e->move_left())
    e->buffer()->select(e->insert_position(), e->insert_position()+1);
  kill_selection(e);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

int Fl_Text_Editor::kf_enter(int, Fl_Text_Editor* e) {
  kill_selection(e);
  e->insert("\n");
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

extern void fl_text_drag_me(int pos, Fl_Text_Display* d);

int Fl_Text_Editor::kf_move(int c, Fl_Text_Editor* e) {
  int i;
  int selected = e->buffer()->selected();
  if (!selected)
    e->dragPos = e->insert_position();
  e->buffer()->unselect();
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

int Fl_Text_Editor::kf_shift_move(int c, Fl_Text_Editor* e) {
  kf_move(c, e);
  fl_text_drag_me(e->insert_position(), e);
  return 1;
}

int Fl_Text_Editor::kf_ctrl_move(int c, Fl_Text_Editor* e) {
  if (!e->buffer()->selected())
    e->dragPos = e->insert_position();
  if (c != FL_Up && c != FL_Down) {
    e->buffer()->unselect();
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

int Fl_Text_Editor::kf_c_s_move(int c, Fl_Text_Editor* e) {
  kf_ctrl_move(c, e);
  fl_text_drag_me(e->insert_position(), e);
  return 1;
}

int Fl_Text_Editor::kf_home(int, Fl_Text_Editor* e) {
    return kf_move(FL_Home, e);
}

int Fl_Text_Editor::kf_end(int, Fl_Text_Editor* e) {
  return kf_move(FL_End, e);
}

int Fl_Text_Editor::kf_left(int, Fl_Text_Editor* e) {
  return kf_move(FL_Left, e);
}

int Fl_Text_Editor::kf_up(int, Fl_Text_Editor* e) {
  return kf_move(FL_Up, e);
}

int Fl_Text_Editor::kf_right(int, Fl_Text_Editor* e) {
  return kf_move(FL_Right, e);
}

int Fl_Text_Editor::kf_down(int, Fl_Text_Editor* e) {
  return kf_move(FL_Down, e);
}

int Fl_Text_Editor::kf_page_up(int, Fl_Text_Editor* e) {
  return kf_move(FL_Page_Up, e);
}

int Fl_Text_Editor::kf_page_down(int, Fl_Text_Editor* e) {
  return kf_move(FL_Page_Down, e);
}


int Fl_Text_Editor::kf_insert(int, Fl_Text_Editor* e) {
  e->insert_mode(e->insert_mode() ? 0 : 1);
  return 1;
}

int Fl_Text_Editor::kf_delete(int, Fl_Text_Editor* e) {
  if (!e->buffer()->selected())
    e->buffer()->select(e->insert_position(), e->insert_position()+1);
  kill_selection(e);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

int Fl_Text_Editor::kf_copy(int, Fl_Text_Editor* e) {
  if (!e->buffer()->selected()) return 1;
  const char *copy = e->buffer()->selection_text();
  if (*copy) Fl::copy(copy, strlen(copy), 1);
  free((void*)copy);
  e->show_insert_position();
  return 1;
}

int Fl_Text_Editor::kf_cut(int c, Fl_Text_Editor* e) {
  kf_copy(c, e);
  kill_selection(e);
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

int Fl_Text_Editor::kf_paste(int, Fl_Text_Editor* e) {
  kill_selection(e);
  Fl::paste(*e, 1);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return 1;
}

int Fl_Text_Editor::kf_select_all(int, Fl_Text_Editor* e) {
  e->buffer()->select(0, e->buffer()->length());
  return 1;
}

int Fl_Text_Editor::kf_undo(int , Fl_Text_Editor* e) {
  e->buffer()->unselect();
  int crsr;
  int ret = e->buffer()->undo(&crsr);
  e->insert_position(crsr);
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  return ret;
}

int Fl_Text_Editor::handle_key() {
  // Call FLTK's rules to try to turn this into a printing character.
  // This uses the right-hand ctrl key as a "compose prefix" and returns
  // the changes that should be made to the text, as a number of
  // bytes to delete and a string to insert:
  int del;
  if (Fl::compose(del)) {
    if (del) buffer()->select(insert_position()-del, insert_position());
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

void Fl_Text_Editor::maybe_do_callback() {
//  printf("Fl_Text_Editor::maybe_do_callback()\n");
//  printf("changed()=%d, when()=%x\n", changed(), when());
  if (changed() || (when()&FL_WHEN_NOT_CHANGED)) do_callback();
}

int Fl_Text_Editor::handle(int event) {
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
        dragType = -1;
        Fl::paste(*this, 0);
        Fl::focus(this);
        set_changed();
        if (when()&FL_WHEN_CHANGED) do_callback();
        return 1;
      }
      break;
  }

  return Fl_Text_Display::handle(event);
}

//
// End of "$Id: Fl_Text_Editor.cxx 5671 2007-02-08 07:58:47Z matt $".
//
