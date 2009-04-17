//
// "$Id: Fl_Tile.cxx 5606 2007-01-18 10:01:24Z matt $"
//
// Tile widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
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

// Group of 2 or 4 "tiles" that can be resized by dragging border
// The size of the first child determines where the resize border is.
// The resizebox is used to limit where the border can be dragged to.

#include <FL/Fl.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Window.H>
#include <stdlib.h>

// Drag the edges that were initially at oldx,oldy to newx,newy:
// pass zero as oldx or oldy to disable drag in that direction:

void Fl_Tile::position(int oix, int oiy, int newx, int newy) {
  Fl_Widget*const* a = array();
  short* p = sizes();
  p += 8; // skip group & resizable's saved size
  for (int i=children(); i--; p += 4) {
    Fl_Widget* o = *a++;
    if (o == resizable()) continue;
    int X = o->x();
    int R = X+o->w();
    if (oix) {
      int t = p[0];
      if (t == oix || t>oix && X<newx || t<oix && X>newx) X = newx;
      t = p[1];
      if (t == oix || t>oix && R<newx || t<oix && R>newx) R = newx;
    }
    int Y = o->y();
    int B = Y+o->h();
    if (oiy) {
      int t = p[2];
      if (t == oiy || t>oiy && Y<newy || t<oiy && Y>newy) Y = newy;
      t = p[3];
      if (t == oiy || t>oiy && B<newy || t<oiy && B>newy) B = newy;
    }
    o->damage_resize(X,Y,R-X,B-Y);
  }
}

// move the lower-right corner (sort of):
void Fl_Tile::resize(int X,int Y,int W,int H) {
  //Fl_Group::resize(X, Y, W, H);
  //return;
  // remember how much to move the child widgets:
  int dx = X-x();
  int dy = Y-y();
  int dw = W-w();
  int dh = H-h();
  short* p = sizes();
  // resize this (skip the Fl_Group resize):
  Fl_Widget::resize(X,Y,W,H);
  // find bottom-right of resiable:
  int OR = p[5];
  int NR = X+W-(p[1]-OR);
  int OB = p[7];
  int NB = Y+H-(p[3]-OB);
  // move everything to be on correct side of new resizable:
  Fl_Widget*const* a = array();
  p += 8;
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    int xx = o->x()+dx;
    int R = xx+o->w();
    if (*p++ >= OR) xx += dw; else if (xx > NR) xx = NR;
    if (*p++ >= OR) R += dw; else if (R > NR) R = NR;
    int yy = o->y()+dy;
    int B = yy+o->h();
    if (*p++ >= OB) yy += dh; else if (yy > NB) yy = NB;
    if (*p++ >= OB) B += dh; else if (B > NB) B = NB;
    o->resize(xx,yy,R-xx,B-yy);
    // do *not* call o->redraw() here! If you do, and the tile is inside a 
    // scroll, it'll set the damage areas wrong for all children!
  }
}

static void set_cursor(Fl_Tile*t, Fl_Cursor c) {
  static Fl_Cursor cursor;
  if (cursor == c || !t->window()) return;
  cursor = c;
#ifdef __sgi
  t->window()->cursor(c,FL_RED,FL_WHITE);
#else
  t->window()->cursor(c);
#endif
}

static Fl_Cursor cursors[4] = {
  FL_CURSOR_DEFAULT,
  FL_CURSOR_WE,
  FL_CURSOR_NS,
  FL_CURSOR_MOVE};

int Fl_Tile::handle(int event) {
  static int sdrag;
  static int sdx, sdy;
  static int sx, sy;
#define DRAGH 1
#define DRAGV 2
#define GRABAREA 4

  int mx = Fl::event_x();
  int my = Fl::event_y();

  switch (event) {

  case FL_MOVE:
  case FL_ENTER:
  case FL_PUSH: {
    int mindx = 100;
    int mindy = 100;
    int oldx = 0;
    int oldy = 0;
    Fl_Widget*const* a = array();
    short* q = sizes();
    short* p = q+8;
    for (int i=children(); i--; p += 4) {
      Fl_Widget* o = *a++;
      if (o == resizable()) continue;
      if (p[1]<q[1] && o->y()<=my+GRABAREA && o->y()+o->h()>=my-GRABAREA) {
	int t = mx - (o->x()+o->w());
	if (abs(t) < mindx) {
	  sdx = t;
	  mindx = abs(t);
	  oldx = p[1];
	}
      }
      if (p[3]<q[3] && o->x()<=mx+GRABAREA && o->x()+o->w()>=mx-GRABAREA) {
	int t = my - (o->y()+o->h());
	if (abs(t) < mindy) {
	  sdy = t;
	  mindy = abs(t);
	  oldy = p[3];
	}
      }
    }
    sdrag = 0; sx = sy = 0;
    if (mindx <= GRABAREA) {sdrag = DRAGH; sx = oldx;}
    if (mindy <= GRABAREA) {sdrag |= DRAGV; sy = oldy;}
    set_cursor(this, cursors[sdrag]);
    if (sdrag) return 1;
    return Fl_Group::handle(event);
  }

  case FL_LEAVE:
    set_cursor(this, FL_CURSOR_DEFAULT);
    break;

  case FL_DRAG:
    // This is necessary if CONSOLIDATE_MOTION in Fl_x.cxx is turned off:
    // if (damage()) return 1; // don't fall behind
  case FL_RELEASE: {
    if (!sdrag) return 0; // should not happen
    Fl_Widget* r = resizable(); if (!r) r = this;
    int newx;
    if (sdrag&DRAGH) {
      newx = Fl::event_x()-sdx;
      if (newx < r->x()) newx = r->x();
      else if (newx > r->x()+r->w()) newx = r->x()+r->w();
    } else
      newx = sx;
    int newy;
    if (sdrag&DRAGV) {
      newy = Fl::event_y()-sdy;
      if (newy < r->y()) newy = r->y();
      else if (newy > r->y()+r->h()) newy = r->y()+r->h();
    } else
      newy = sy;
    position(sx,sy,newx,newy);
    if (event == FL_DRAG) set_changed();
    do_callback();
    return 1;}

  }

  return Fl_Group::handle(event);
}

//
// End of "$Id: Fl_Tile.cxx 5606 2007-01-18 10:01:24Z matt $".
//
