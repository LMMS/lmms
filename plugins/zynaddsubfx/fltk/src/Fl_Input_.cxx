//
// "$Id: Fl_Input_.cxx 6104 2008-04-21 20:54:37Z matt $"
//
// Common input widget routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2006 by Bill Spitzak and others.
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

// This is the base class for Fl_Input.  You can use it directly
// if you are one of those people who like to define their own
// set of editing keys.  It may also be useful for adding scrollbars
// to the input field.

#include <FL/Fl.H>
#include <FL/Fl_Input_.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <math.h>
#include "flstring.h"
#include <stdlib.h>
#include <ctype.h>

#define MAXBUF 1024

extern void fl_draw(const char*, int, float, float);

////////////////////////////////////////////////////////////////

// Copy string p..e to the buffer, replacing characters with ^X and \nnn
// as necessary.  Truncate if necessary so the resulting string and
// null terminator fits in a buffer of size n.  Return new end pointer.
const char* Fl_Input_::expand(const char* p, char* buf) const {
  char* o = buf;
  char* e = buf+(MAXBUF-4);
  const char* lastspace = p;
  char* lastspace_out = o;
  int width_to_lastspace = 0;
  int word_count = 0;
  int word_wrap;

  if (input_type()==FL_SECRET_INPUT) {
    while (o<e && p < value_+size_) {*o++ = '*'; p++;}
  } else while (o<e) {
    if (wrap() && (p >= value_+size_ || isspace(*p & 255))) {
      word_wrap = w() - Fl::box_dw(box()) - 2;
      width_to_lastspace += (int)fl_width(lastspace_out, o-lastspace_out);
      if (p > lastspace+1) {
	if (word_count && width_to_lastspace > word_wrap) {
	  p = lastspace; o = lastspace_out; break;
	}
	word_count++;
      }
      lastspace = p;
      lastspace_out = o;
    }

    if (p >= value_+size_) break;
    int c = *p++ & 255;
    if (c < ' ' || c == 127) {
      if (c=='\n' && input_type()==FL_MULTILINE_INPUT) {p--; break;}
      if (c == '\t' && input_type()==FL_MULTILINE_INPUT) {
	for (c = (o-buf)%8; c<8 && o<e; c++) *o++ = ' ';
      } else {
	*o++ = '^';
	*o++ = c ^ 0x40;
      }
#ifdef __APPLE__
    // In MacRoman, all characters are defined, and non-break-space is 0xca
    } else if (c == 0xCA) { // nbsp
      *o++ = ' ';
#else
    // in ISO 8859-1, undefined characters are rendered as octal
    // this is commented out since most X11 seems to use MSWindows Latin-1
    //} else if (c >= 128 && c < 0xA0) {
      // these codes are not defined in ISO code, so we output the octal code instead
    //  *o++ = '\\'; 
    //  *o++ = ((c>>6)&0x03) + '0'; 
    //  *o++ = ((c>>3)&0x07) + '0'; 
    //  *o++ = (c&0x07) + '0';
    } else if (c == 0xA0) { // nbsp
      *o++ = ' ';
#endif
    } else {
      *o++ = c;
    }
  }
  *o = 0;
  return p;
}

// After filling in such a buffer, find the width to e
double Fl_Input_::expandpos(
  const char* p,	// real string
  const char* e,	// pointer into real string
  const char* buf,	// conversion of real string by expand()
  int* returnn		// return offset into buf here
) const {
  int n = 0;
  if (input_type()==FL_SECRET_INPUT) n = e-p;
  else while (p<e) {
    int c = *p++ & 255;
    if (c < ' ' || c == 127) {
      if (c == '\t' && input_type()==FL_MULTILINE_INPUT) n += 8-(n%8);
      else n += 2;
#ifdef __APPLE__
    // in MacRoman, all characters are defined
#else
    // in Windows Latin-1 all characters are defined
    //} else if (c >= 128 && c < 0xA0) {
      // these codes are not defined in ISO code, so we output the octal code instead
    //  n += 4;
#endif
    } else {
      n++;
    }
  }
  if (returnn) *returnn = n;
  return fl_width(buf, n);
}

////////////////////////////////////////////////////////////////

// minimal update:
// Characters from mu_p to end of widget are redrawn.
// If erase_cursor_only, small part at mu_p is redrawn.
// Right now minimal update just keeps unchanged characters from
// being erased, so they don't blink.

void Fl_Input_::minimal_update(int p) {
  if (damage() & FL_DAMAGE_ALL) return; // don't waste time if it won't be done
  if (damage() & FL_DAMAGE_EXPOSE) {
    if (p < mu_p) mu_p = p;
  } else {
    mu_p = p;
  }

  damage(FL_DAMAGE_EXPOSE);
  erase_cursor_only = 0;
}

void Fl_Input_::minimal_update(int p, int q) {
  if (q < p) p = q;
  minimal_update(p);
}

////////////////////////////////////////////////////////////////

static double up_down_pos;
static int was_up_down;

void Fl_Input_::setfont() const {
 fl_font(textfont(), textsize());
}

void Fl_Input_::drawtext(int X, int Y, int W, int H) {
  int do_mu = !(damage()&FL_DAMAGE_ALL);

  if (Fl::focus()!=this && !size()) {
    if (do_mu) { // we have to erase it if cursor was there
      draw_box(box(), X-Fl::box_dx(box()), Y-Fl::box_dy(box()),
               W+Fl::box_dw(box()), H+Fl::box_dh(box()), color());
    }
    return;
  }

  int selstart, selend;
  if (Fl::focus()!=this && /*Fl::selection_owner()!=this &&*/ Fl::pushed()!=this)
    selstart = selend = 0;
  else if (position() <= mark()) {
    selstart = position(); selend = mark();
  } else {
    selend = position(); selstart = mark();
  }

  setfont();
  const char *p, *e;
  char buf[MAXBUF];

  // count how many lines and put the last one into the buffer:
  // And figure out where the cursor is:
  int height = fl_height();
  int lines;
  int curx, cury;
  for (p=value(), curx=cury=lines=0; ;) {
    e = expand(p, buf);
    if (position() >= p-value() && position() <= e-value()) {
      curx = int(expandpos(p, value()+position(), buf, 0)+.5);
      if (Fl::focus()==this && !was_up_down) up_down_pos = curx;
      cury = lines*height;
      int newscroll = xscroll_;
      if (curx > newscroll+W-20) {
	// figure out scrolling so there is space after the cursor:
	newscroll = curx+20-W;
	// figure out the furthest left we ever want to scroll:
	int ex = int(expandpos(p, e, buf, 0))+2-W;
	// use minimum of both amounts:
	if (ex < newscroll) newscroll = ex;
      } else if (curx < newscroll+20) {
	newscroll = curx-20;
      }
      if (newscroll < 0) newscroll = 0;
      if (newscroll != xscroll_) {
	xscroll_ = newscroll;
	mu_p = 0; erase_cursor_only = 0;
      }
    }
    lines++;
    if (e >= value_+size_) break;
    p = e+1;
  }

  // adjust the scrolling:
  if (input_type()==FL_MULTILINE_INPUT) {
    int newy = yscroll_;
    if (cury < newy) newy = cury;
    if (cury > newy+H-height) newy = cury-H+height;
    if (newy < -1) newy = -1;
    if (newy != yscroll_) {yscroll_ = newy; mu_p = 0; erase_cursor_only = 0;}
  } else {
    yscroll_ = -(H-height)/2;
  }

  fl_clip(X, Y, W, H);
  Fl_Color tc = active_r() ? textcolor() : fl_inactive(textcolor());

  p = value();
  // visit each line and draw it:
  int desc = height-fl_descent();
  float xpos = (float)(X - xscroll_ + 1);
  int ypos = -yscroll_;
  for (; ypos < H;) {

    // re-expand line unless it is the last one calculated above:
    if (lines>1) e = expand(p, buf);

    if (ypos <= -height) goto CONTINUE; // clipped off top

    if (do_mu) {	// for minimal update:
      const char* pp = value()+mu_p; // pointer to where minimal update starts
      if (e < pp) goto CONTINUE2; // this line is before the changes
      if (readonly()) erase_cursor_only = 0; // this isn't the most efficient way
      if (erase_cursor_only && p > pp) goto CONTINUE2; // this line is after
      // calculate area to erase:
      float r = (float)(X+W);
      float xx;
      if (p >= pp) {
	xx = (float)X;
	if (erase_cursor_only) r = xpos+2;
	else if (readonly()) xx -= 3;
      } else {
	xx = xpos + (float)expandpos(p, pp, buf, 0);
	if (erase_cursor_only) r = xx+2;
	else if (readonly()) xx -= 3;
      }
      // clip to and erase it:
      fl_push_clip((int)xx-1-height/8, Y+ypos, (int)(r-xx+2+height/4), height);
      draw_box(box(), X-Fl::box_dx(box()), Y-Fl::box_dy(box()),
               W+Fl::box_dw(box()), H+Fl::box_dh(box()), color());
      // it now draws entire line over it
      // this should not draw letters to left of erased area, but
      // that is nyi.
    }

    // Draw selection area if required:
    if (selstart < selend && selstart <= e-value() && selend > p-value()) {
      const char* pp = value()+selstart;
      float x1 = xpos;
      int offset1 = 0;
      if (pp > p) {
	fl_color(tc);
	x1 += (float)expandpos(p, pp, buf, &offset1);
	fl_draw(buf, offset1, xpos, (float)(Y+ypos+desc));
      }
      pp = value()+selend;
      float x2 = (float)(X+W);
      int offset2;
      if (pp <= e) x2 = xpos + (float)expandpos(p, pp, buf, &offset2);
      else offset2 = strlen(buf);
      fl_color(selection_color());
      fl_rectf((int)(x1+0.5), Y+ypos, (int)(x2-x1+0.5), height);
      fl_color(fl_contrast(textcolor(), selection_color()));
      fl_draw(buf+offset1, offset2-offset1, x1, (float)(Y+ypos+desc));
      if (pp < e) {
	fl_color(tc);
	fl_draw(buf+offset2, strlen(buf+offset2), x2, (float)(Y+ypos+desc));
      }
    } else {
      // draw unselected text
      fl_color(tc);
      fl_draw(buf, strlen(buf), xpos, (float)(Y+ypos+desc));
    }

    if (do_mu) fl_pop_clip();

  CONTINUE2:
    // draw the cursor:
    if (Fl::focus() == this && selstart == selend &&
	position() >= p-value() && position() <= e-value()) {
      fl_color(cursor_color());
      if (readonly()) {
        fl_line((int)(xpos+curx-2.5f), Y+ypos+height-1,
	        (int)(xpos+curx+0.5f), Y+ypos+height-4,
	        (int)(xpos+curx+3.5f), Y+ypos+height-1);
      } else {
        fl_rectf((int)(xpos+curx+0.5), Y+ypos, 2, height);
      }
    }

  CONTINUE:
    ypos += height;
    if (e >= value_+size_) break;
    if (*e == '\n' || *e == ' ') e++;
    p = e;
  }

  // for minimal update, erase all lines below last one if necessary:
  if (input_type()==FL_MULTILINE_INPUT && do_mu && ypos<H
      && (!erase_cursor_only || p <= value()+mu_p)) {
    if (ypos < 0) ypos = 0;
    fl_push_clip(X, Y+ypos, W, H-ypos);
    draw_box(box(), X-Fl::box_dx(box()), Y-Fl::box_dy(box()),
             W+Fl::box_dw(box()), H+Fl::box_dh(box()), color());
    fl_pop_clip();
  }

  fl_pop_clip();
}

static int isword(char c) {
  return (c&128 || isalnum(c) || strchr("#%&-/@\\_~", c));
}

int Fl_Input_::word_end(int i) const {
  if (input_type() == FL_SECRET_INPUT) return size();
  //while (i < size() && !isword(index(i))) i++;
  while (i < size() && isword(index(i))) i++;
  return i;
}

int Fl_Input_::word_start(int i) const {
  if (input_type() == FL_SECRET_INPUT) return 0;
//   if (i >= size() || !isword(index(i)))
//     while (i > 0 && !isword(index(i-1))) i--;
  while (i > 0 && isword(index(i-1))) i--;
  return i;
}

int Fl_Input_::line_end(int i) const {
  if (input_type() != FL_MULTILINE_INPUT) return size();

  if (wrap()) {
    // go to the start of the paragraph:
    int j = i;
    while (j > 0 && index(j-1) != '\n') j--;
    // now measure lines until we get past i, end of that line is real eol:
    setfont();
    for (const char* p=value()+j; ;) {
      char buf[MAXBUF];
      p = expand(p, buf);
      if (p-value() >= i) return p-value();
      p++;
    }
  } else {
    while (i < size() && index(i) != '\n') i++;
    return i;
  }
}

int Fl_Input_::line_start(int i) const {
  if (input_type() != FL_MULTILINE_INPUT) return 0;
  int j = i;
  while (j > 0 && index(j-1) != '\n') j--;
  if (wrap()) {
    // now measure lines until we get past i, start of that line is real eol:
    setfont();
    for (const char* p=value()+j; ;) {
      char buf[MAXBUF];
      const char* e = expand(p, buf);
      if (e-value() >= i) return p-value();
      p = e+1;
    }
  } else return j;
}

void Fl_Input_::handle_mouse(int X, int Y, int /*W*/, int /*H*/, int drag) {
  was_up_down = 0;
  if (!size()) return;
  setfont();

  const char *p, *e;
  char buf[MAXBUF];

  int theline = (input_type()==FL_MULTILINE_INPUT) ?
    (Fl::event_y()-Y+yscroll_)/fl_height() : 0;

  int newpos = 0;
  for (p=value();; ) {
    e = expand(p, buf);
    theline--; if (theline < 0) break;
    if (e >= value_+size_) break;
    p = e+1;
  }
  const char *l, *r, *t; double f0 = Fl::event_x()-X+xscroll_;
  for (l = p, r = e; l<r; ) {
    double f;
    t = l+(r-l+1)/2;
    f = X-xscroll_+expandpos(p, t, buf, 0);
    if (f <= Fl::event_x()) {l = t; f0 = Fl::event_x()-f;}
    else r = t-1;
  }
  if (l < e) { // see if closer to character on right:
    double f1 = X-xscroll_+expandpos(p, l+1, buf, 0)-Fl::event_x();
    if (f1 < f0) l = l+1;
  }
  newpos = l-value();

  int newmark = drag ? mark() : newpos;
  if (Fl::event_clicks()) {
    if (newpos >= newmark) {
      if (newpos == newmark) {
	if (newpos < size()) newpos++;
	else newmark--;
      }
      if (Fl::event_clicks() > 1) {
	newpos = line_end(newpos);
	newmark = line_start(newmark);
      } else {
	newpos = word_end(newpos);
	newmark = word_start(newmark);
      }
    } else {
      if (Fl::event_clicks() > 1) {
	newpos = line_start(newpos);
	newmark = line_end(newmark);
      } else {
	newpos = word_start(newpos);
	newmark = word_end(newmark);
      }
    }
    // if the multiple click does not increase the selection, revert
    // to single-click behavior:
    if (!drag && (mark() > position() ?
                  (newmark >= position() && newpos <= mark()) :
                  (newmark >= mark() && newpos <= position()))) {
      Fl::event_clicks(0);
      newmark = newpos = l-value();
    }
  }
  position(newpos, newmark);
}

int Fl_Input_::position(int p, int m) {
  was_up_down = 0;
  if (p<0) p = 0;
  if (p>size()) p = size();
  if (m<0) m = 0;
  if (m>size()) m = size();
  if (p == position_ && m == mark_) return 0;
  //if (Fl::selection_owner() == this) Fl::selection_owner(0);
  if (p != m) {
    if (p != position_) minimal_update(position_, p);
    if (m != mark_) minimal_update(mark_, m);
  } else {
    // new position is a cursor
    if (position_ == mark_) {
      // old position was just a cursor
      if (Fl::focus() == this && !(damage()&FL_DAMAGE_EXPOSE)) {
	minimal_update(position_); erase_cursor_only = 1;
      }
    } else { // old position was a selection
      minimal_update(position_, mark_);
    }
  }
  position_ = p;
  mark_ = m;
  return 1;
}

int Fl_Input_::up_down_position(int i, int keepmark) {
  // unlike before, i must be at the start of the line already!

  setfont();
  char buf[MAXBUF];
  const char* p = value()+i;
  const char* e = expand(p, buf);
  const char *l, *r, *t;
  for (l = p, r = e; l<r; ) {
    t = l+(r-l+1)/2;
    int f = (int)expandpos(p, t, buf, 0);
    if (f <= up_down_pos) l = t; else r = t-1;
  }
  int j = l-value();
  j = position(j, keepmark ? mark_ : j);
  was_up_down = 1;
  return j;
}

int Fl_Input_::copy(int clipboard) {
  int b = position();
  int e = mark();
  if (b != e) {
    if (b > e) {b = mark(); e = position();}
    if (input_type() == FL_SECRET_INPUT) e = b;
    Fl::copy(value()+b, e-b, clipboard);
    return 1;
  }
  return 0;
}

#define MAXFLOATSIZE 40

static char* undobuffer;
static int undobufferlength;
static Fl_Input_* undowidget;
static int undoat;	// points after insertion
static int undocut;	// number of characters deleted there
static int undoinsert;	// number of characters inserted
static int yankcut;	// length of valid contents of buffer, even if undocut=0

static void undobuffersize(int n) {
  if (n > undobufferlength) {
    if (undobuffer) {
      do {undobufferlength *= 2;} while (undobufferlength < n);
      undobuffer = (char*)realloc(undobuffer, undobufferlength);
    } else {
      undobufferlength = n+9;
      undobuffer = (char*)malloc(undobufferlength);
    }
  }
}

// all changes go through here, delete characters b-e and insert text:
int Fl_Input_::replace(int b, int e, const char* text, int ilen) {

  was_up_down = 0;

  if (b<0) b = 0;
  if (e<0) e = 0;
  if (b>size_) b = size_;
  if (e>size_) e = size_;
  if (e<b) {int t=b; b=e; e=t;}
  if (text && !ilen) ilen = strlen(text);
  if (e<=b && !ilen) return 0; // don't clobber undo for a null operation
  if (size_+ilen-(e-b) > maximum_size_) {
    ilen = maximum_size_-size_+(e-b);
    if (ilen < 0) ilen = 0;
  }

  put_in_buffer(size_+ilen);

  if (e>b) {
    if (undowidget == this && b == undoat) {
      undobuffersize(undocut+(e-b));
      memcpy(undobuffer+undocut, value_+b, e-b);
      undocut += e-b;
    } else if (undowidget == this && e == undoat && !undoinsert) {
      undobuffersize(undocut+(e-b));
      memmove(undobuffer+(e-b), undobuffer, undocut);
      memcpy(undobuffer, value_+b, e-b);
      undocut += e-b;
    } else if (undowidget == this && e == undoat && (e-b)<undoinsert) {
      undoinsert -= e-b;
    } else {
      undobuffersize(e-b);
      memcpy(undobuffer, value_+b, e-b);
      undocut = e-b;
      undoinsert = 0;
    }
    memmove(buffer+b, buffer+e, size_-e+1);
    size_ -= e-b;
    undowidget = this;
    undoat = b;
    if (input_type() == FL_SECRET_INPUT) yankcut = 0; else yankcut = undocut;
  }

  if (ilen) {
    if (undowidget == this && b == undoat)
      undoinsert += ilen;
    else {
      undocut = 0;
      undoinsert = ilen;
    }
    memmove(buffer+b+ilen, buffer+b, size_-b+1);
    memcpy(buffer+b, text, ilen);
    size_ += ilen;
  }
  undowidget = this;
  undoat = b+ilen;

  // Insertions into the word at the end of the line will cause it to
  // wrap to the next line, so we must indicate that the changes may start
  // right after the whitespace before the current word.  This will
  // result in sub-optimal update when such wrapping does not happen
  // but it is too hard to figure out for now...
  if (wrap()) {
    // if there is a space in the pasted text, the whole line may have rewrapped
    int i;
    for (i=0; i<ilen; i++) 
      if (text[i]==' ') break;
    if (i==ilen)
      while (b > 0 && !isspace(index(b) & 255) && index(b)!='\n') b--;
    else
      while (b > 0 && index(b)!='\n') b--;
  }

  // make sure we redraw the old selection or cursor:
  if (mark_ < b) b = mark_;
  if (position_ < b) b = position_;

  minimal_update(b);

  mark_ = position_ = undoat;

  set_changed();
  if (when()&FL_WHEN_CHANGED) do_callback();
  return 1;
}

int Fl_Input_::undo() {
  was_up_down = 0;
  if (undowidget != this || !undocut && !undoinsert) return 0;

  int ilen = undocut;
  int xlen = undoinsert;
  int b = undoat-xlen;
  int b1 = b;

  put_in_buffer(size_+ilen);

  if (ilen) {
    memmove(buffer+b+ilen, buffer+b, size_-b+1);
    memcpy(buffer+b, undobuffer, ilen);
    size_ += ilen;
    b += ilen;
  }

  if (xlen) {
    undobuffersize(xlen);
    memcpy(undobuffer, buffer+b, xlen);
    memmove(buffer+b, buffer+b+xlen, size_-xlen-b+1);
    size_ -= xlen;
  }

  undocut = xlen;
  if (xlen) yankcut = xlen;
  undoinsert = ilen;
  undoat = b;
  mark_ = b /* -ilen */;
  position_ = b;

  if (wrap())
    while (b1 > 0 && index(b1)!='\n') b1--;
  minimal_update(b1);
  set_changed();
  if (when()&FL_WHEN_CHANGED) do_callback();
  return 1;
}

int Fl_Input_::copy_cuts() {
  // put the yank buffer into the X clipboard
  if (!yankcut || input_type()==FL_SECRET_INPUT) return 0;
  Fl::copy(undobuffer, yankcut, 1);
  return 1;
}

void Fl_Input_::maybe_do_callback() {
  if (changed() || (when()&FL_WHEN_NOT_CHANGED)) {
    do_callback();
  }
}

int Fl_Input_::handletext(int event, int X, int Y, int W, int H) {
  switch (event) {

  case FL_ENTER:
  case FL_MOVE:
    if (active_r() && window()) window()->cursor(FL_CURSOR_INSERT);
    return 1;

  case FL_LEAVE:
    if (active_r() && window()) window()->cursor(FL_CURSOR_DEFAULT);
    return 1;

  case FL_FOCUS:
    if (mark_ == position_) {
      minimal_update(size()+1);
    } else //if (Fl::selection_owner() != this)
      minimal_update(mark_, position_);
    return 1;

  case FL_UNFOCUS:
    if (active_r() && window()) window()->cursor(FL_CURSOR_DEFAULT);
    if (mark_ == position_) {
      if (!(damage()&FL_DAMAGE_EXPOSE)) {minimal_update(position_); erase_cursor_only = 1;}
    } else //if (Fl::selection_owner() != this)
      minimal_update(mark_, position_);
  case FL_HIDE:
    if (!readonly() && (when() & FL_WHEN_RELEASE))
      maybe_do_callback();
    return 1;

  case FL_PUSH:
    if (active_r() && window()) window()->cursor(FL_CURSOR_INSERT);

    handle_mouse(X, Y, W, H, Fl::event_state(FL_SHIFT));

    if (Fl::focus() != this) {
      Fl::focus(this);
      handle(FL_FOCUS);
    }
    return 1;

  case FL_DRAG:
    handle_mouse(X, Y, W, H, 1);
    return 1;

  case FL_RELEASE:
    copy(0);
    return 1;

  case FL_PASTE: {
    // Don't allow pastes into readonly widgets...
    if (readonly()) {
      fl_beep(FL_BEEP_ERROR);
      return 1;
    }

    // See if we have anything to paste...
    if (!Fl::event_text() || !Fl::event_length()) return 1;

    // strip trailing control characters and spaces before pasting:
    const char* t = Fl::event_text();
    const char* e = t+Fl::event_length();
    if (input_type() != FL_MULTILINE_INPUT) while (e > t && isspace(*(e-1) & 255)) e--;
    if (!t || e <= t) return 1; // Int/float stuff will crash without this test
    if (input_type() == FL_INT_INPUT) {
      while (isspace(*t & 255) && t < e) t ++;
      const char *p = t;
      if (*p == '+' || *p == '-') p ++;
      if (strncmp(p, "0x", 2) == 0) {
        p += 2;
        while (isxdigit(*p & 255) && p < e) p ++;
      } else {
        while (isdigit(*p & 255) && p < e) p ++;
      }
      if (p < e) {
        fl_beep(FL_BEEP_ERROR);
        return 1;
      } else return replace(0, size(), t, e - t);
    } else if (input_type() == FL_FLOAT_INPUT) {
      while (isspace(*t & 255) && t < e) t ++;
      const char *p = t;
      if (*p == '+' || *p == '-') p ++;
      while (isdigit(*p & 255) && p < e) p ++;
      if (*p == '.') {
        p ++;
        while (isdigit(*p & 255) && p < e) p ++;
	if (*p == 'e' || *p == 'E') {
	  p ++;
	  if (*p == '+' || *p == '-') p ++;
	  while (isdigit(*p & 255) && p < e) p ++;
	}
      }
      if (p < e) {
        fl_beep(FL_BEEP_ERROR);
        return 1;
      } else return replace(0, size(), t, e - t);
    }
    return replace(position(), mark(), t, e-t);}

  default:
    return 0;
  }
}

/*------------------------------*/

Fl_Input_::Fl_Input_(int X, int Y, int W, int H, const char* l)
: Fl_Widget(X, Y, W, H, l) {
  box(FL_DOWN_BOX);
  color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
  align(FL_ALIGN_LEFT);
  textsize_ = (uchar)FL_NORMAL_SIZE;
  textfont_ = FL_HELVETICA;
  textcolor_ = FL_FOREGROUND_COLOR;
  cursor_color_ = FL_FOREGROUND_COLOR; // was FL_BLUE
  mark_ = position_ = size_ = 0;
  bufsize = 0;
  buffer  = 0;
  value_ = "";
  xscroll_ = yscroll_ = 0;
  maximum_size_ = 32767;
}

void Fl_Input_::put_in_buffer(int len) {
  if (value_ == buffer && bufsize > len) {
    buffer[size_] = 0;
    return;
  }
  if (!bufsize) {
    if (len > size_) len += 9; // let a few characters insert before realloc
    bufsize = len+1; 
    buffer = (char*)malloc(bufsize);
  } else if (bufsize <= len) {
    // we may need to move old value in case it points into buffer:
    int moveit = (value_ >= buffer && value_ < buffer+bufsize);
    // enlarge current buffer
    if (len > size_) {
      do {bufsize *= 2;} while (bufsize <= len);
    } else {
      bufsize = len+1;
    }
    // Note: the following code is equivalent to:
    //
    //   if (moveit) value_ = value_ - buffer;
    //   char* nbuffer = (char*)realloc(buffer, bufsize);
    //   if (moveit) value_ = value_ + nbuffer;
    //   buffer = nbuffer;
    //
    // We just optimized the pointer arithmetic for value_...
    //
    char* nbuffer = (char*)realloc(buffer, bufsize);
    if (moveit) value_ += (nbuffer-buffer);
    buffer = nbuffer;
  }
  memmove(buffer, value_, size_); buffer[size_] = 0;
  value_ = buffer;
}

int Fl_Input_::static_value(const char* str, int len) {
  clear_changed();
  if (undowidget == this) undowidget = 0;
  if (str == value_ && len == size_) return 0;
  if (len) { // non-empty new value:
    if (xscroll_ || yscroll_) {
      xscroll_ = yscroll_ = 0;
      minimal_update(0);
    } else {
      int i = 0;
      // find first different character:
      if (value_) {
	for (; i<size_ && i<len && str[i]==value_[i]; i++);
	if (i==size_ && i==len) return 0;
      }
      minimal_update(i);
    }
    value_ = str;
    size_ = len;
  } else { // empty new value:
    if (!size_) return 0; // both old and new are empty.
    size_ = 0;
    value_ = "";
    xscroll_ = yscroll_ = 0;
    minimal_update(0);
  }
  position(readonly() ? 0 : size());
  return 1;
}

int Fl_Input_::static_value(const char* str) {
  return static_value(str, str ? strlen(str) : 0);
}

int Fl_Input_::value(const char* str, int len) {
  int r = static_value(str, len);
  if (len) put_in_buffer(len);
  return r;
}

int Fl_Input_::value(const char* str) {
  return value(str, str ? strlen(str) : 0);
}

void Fl_Input_::resize(int X, int Y, int W, int H) {
  if (W != w()) xscroll_ = 0;
  if (H != h()) yscroll_ = 0;
  Fl_Widget::resize(X, Y, W, H);
}

Fl_Input_::~Fl_Input_() {
  if (undowidget == this) undowidget = 0;
  if (bufsize) free((void*)buffer);
}

//
// End of "$Id: Fl_Input_.cxx 6104 2008-04-21 20:54:37Z matt $".
//
