//
// "$Id: fl_symbols.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Symbol drawing code for the Fast Light Tool Kit (FLTK).
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

// These are small graphics drawn by the normal label-drawing
// code when the string starts with an '@' sign.

// Adapted from original code written by:

// Written by Mark Overmars
// Version 2.1 a
// Date: Oct  2, 1992

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/math.h>
#include "flstring.h"

typedef struct {
  const char *name;
  void (*drawit)(Fl_Color);
  char scalable;
  char notempty;
} SYMBOL;

#define MAXSYMBOL       211
   /* Maximal number of symbols in table. Only half of them are
      used. Should be prime. */

static SYMBOL symbols[MAXSYMBOL];      /* The symbols */
static int symbnumb = -1;              /* Their number */

static int find(const char *name) {
// returns hash entry if it exists, or first empty slot:
  int pos = name[0] ? (
    name[1] ? (
      name[2] ? 71*name[0]+31*name[1]+name[2] : 31*name[0]+name[1]
    ) :
      name[0]
  ) : 0;
  pos %= MAXSYMBOL;
  int hh2 = name[0] ? (
    (name[1]) ? 51*name[0]+3*name[1] : 3*name[0]
    ) : 1;
  hh2 %= MAXSYMBOL; if (!hh2) hh2 = 1;
  for (;;) {
    if (!symbols[pos].notempty) return pos;
    if (!strcmp(symbols[pos].name,name)) return pos;
    pos = (pos + hh2) % MAXSYMBOL;
  }
}

static void fl_init_symbols(void);

/**************** The routines seen by the user *************************/

int fl_add_symbol(const char *name, void (*drawit)(Fl_Color), int scalable)
/* Adds a symbol to the system. Returns whether correct. */
{
  fl_init_symbols();
  int pos;
  if (symbnumb > MAXSYMBOL / 2) return 0;	// table is full
  pos = find(name);
  symbols[pos].name = name;
  symbols[pos].drawit = drawit;
  symbols[pos].notempty = 1;
  symbols[pos].scalable = scalable;
  symbnumb++;
  return 1;
}

int fl_return_arrow(int x,int y,int w,int h);

// provided for back compatability:
int fl_draw_symbol(const char *label,int x,int y,int w,int h,Fl_Color col) {  
  const char *p = label;
  if (*p++ != '@') return 0;
  fl_init_symbols();
  int equalscale = 0;
  if (*p == '#') {equalscale = 1; p++;}
  if (*p == '-' && p[1]>='1' && p[1]<='9') {
    int n = p[1]-'0';
    x += n; y += n; w -= 2*n; h -= 2*n;
    p += 2;
  } else if (*p == '+' && p[1]>='1' && p[1]<='9') {
    int n = p[1]-'0';
    x -= n; y -= n; w += 2*n; h += 2*n;
    p += 2;
  }
  if (w < 10) {x -= (10-w)/2; w = 10;}
  if (h < 10) {y -= (10-h)/2; h = 10;}
  w = (w-1)|1; h = (h-1)|1;
  char flip_x = 0, flip_y = 0;
  if (*p=='$') {
    flip_x = 1;
    p++;
  }
  if (*p=='%') {
    flip_y = 1;
    p++;
  }
  int rotangle;
  switch (*p++) {
  case '0':
    rotangle = 1000*(p[1]-'0') + 100*(p[2]-'0') + 10*(p[3]-'0');
    p += 4;
    break;
  case '1': rotangle = 2250; break;
  case '2': rotangle = 2700; break;
  case '3': rotangle = 3150; break;
  case '4': rotangle = 1800; break;
  case '5':
  case '6': rotangle = 0; break;
  case '7': rotangle = 1350; break;
  case '8': rotangle =  900; break;
  case '9': rotangle =  450; break;
  default: rotangle = 0; p--; break;
  }
  int pos = find(p);
  if (!symbols[pos].notempty) return 0;
  if (symbols[pos].scalable == 3) { // kludge to detect return arrow
    fl_return_arrow(x,y,w,h);
    return 1;
  }
  fl_push_matrix();
  fl_translate(x+w/2,y+h/2);
  if (symbols[pos].scalable) {
    if (equalscale) {if (w<h) h = w; else w = h;}
    fl_scale(0.5*w, 0.5*h);
    fl_rotate(rotangle/10.0);
    if (flip_x) fl_scale(-1.0, 1.0);
    if (flip_y) fl_scale(1.0, -1.0);
  }
  (symbols[pos].drawit)(col);
  fl_pop_matrix();
  return 1;
}

/******************** THE DEFAULT SYMBOLS ****************************/

/* Some help stuff */

#define BP fl_begin_polygon()
#define EP fl_end_polygon()
#define BCP fl_begin_complex_polygon()
#define ECP fl_end_complex_polygon()
#define BL fl_begin_line()
#define EL fl_end_line()
#define BC fl_begin_loop()
#define EC fl_end_loop()
#define vv(x,y) fl_vertex(x,y)

//for the outline color
static void set_outline_color(Fl_Color c) {
  fl_color(fl_darker(c));
}

static void rectangle(double x,double y,double x2,double y2,Fl_Color col) {
  fl_color(col);
  BP; vv(x,y); vv(x2,y); vv(x2,y2); vv(x,y2); EP;
  set_outline_color(col);
  BC; vv(x,y); vv(x2,y); vv(x2,y2); vv(x,y2); EC;
}

/* The drawing routines */

static void draw_fltk(Fl_Color col) 
{
  fl_color(col);
  // F fill
  BCP; vv(-2.0, -0.5); vv(-1.0, -0.5); vv(-1.0, -0.3); vv(-1.8, -0.3);
  vv(-1.8, -0.1); vv(-1.2, -0.1); vv(-1.2, 0.1); vv(-1.8, 0.1);
  vv(-1.8, 0.5); vv(-2.0, 0.5); ECP;
  // L fill
  BCP; vv(-1.0, -0.5); vv(-0.8, -0.5); vv(-0.8, 0.3); vv(0.0, 0.3);
  vv(0.0, 0.5); vv(-1.0, 0.5); ECP;
  // T outline
  BCP; vv(-0.1, -0.5); vv(1.1, -0.5); vv(1.1, -0.3); vv(0.6, -0.3);
  vv(0.6, 0.5); vv(0.4, 0.5); vv(0.4, -0.3); vv(-0.1, -0.3); ECP;
  // K outline
  BCP; vv(1.1, -0.5); vv(1.3, -0.5); vv(1.3, -0.15); vv(1.70, -0.5);
  vv(2.0, -0.5); vv(1.43, 0.0); vv(2.0, 0.5); vv(1.70, 0.5);
  vv(1.3, 0.15); vv(1.3, 0.5); vv(1.1, 0.5); ECP;
  set_outline_color(col);
  // F outline
  BC; vv(-2.0, -0.5); vv(-1.0, -0.5); vv(-1.0, -0.3); vv(-1.8, -0.3);
  vv(-1.8, -0.1); vv(-1.2, -0.1); vv(-1.2, 0.1); vv(-1.8, 0.1);
  vv(-1.8, 0.5); vv(-2.0, 0.5); EC;
  // L outline
  BC; vv(-1.0, -0.5); vv(-0.8, -0.5); vv(-0.8, 0.3); vv(0.0, 0.3);
  vv(0.0, 0.5); vv(-1.0, 0.5); EC;
  // T outline
  BC; vv(-0.1, -0.5); vv(1.1, -0.5); vv(1.1, -0.3); vv(0.6, -0.3);
  vv(0.6, 0.5); vv(0.4, 0.5); vv(0.4, -0.3); vv(-0.1, -0.3); EC;
  // K outline
  BC; vv(1.1, -0.5); vv(1.3, -0.5); vv(1.3, -0.15); vv(1.70, -0.5);
  vv(2.0, -0.5); vv(1.43, 0.0); vv(2.0, 0.5); vv(1.70, 0.5);
  vv(1.3, 0.15); vv(1.3, 0.5); vv(1.1, 0.5); EC;
}

static void draw_search(Fl_Color col) 
{
  fl_color(col);
  BP; vv(-.4, .13); vv(-1.0, .73); vv(-.73, 1.0); vv(-.13, .4); EP;
  set_outline_color(col);
  fl_line_style(FL_SOLID, 3, 0);
  BC; fl_circle(.2, -.2, .6); EC;
  fl_line_style(FL_SOLID, 1, 0);
  BC; vv(-.4, .13); vv(-1.0, .73); vv(-.73, 1.0); vv(-.13, .4); EC;
}

static void draw_arrow1(Fl_Color col)
{
  fl_color(col);
  BP; vv(-0.8,-0.4); vv(-0.8,0.4); vv(0.0,0.4); vv(0.0,-0.4); EP;
  BP; vv(0.0,0.8); vv(0.8,0.0); vv(0.0,-0.8); vv(0.0,-0.4); vv(0.0,0.4); EP;
  set_outline_color(col);
  BC; vv(-0.8,-0.4); vv(-0.8,0.4); vv(0.0,0.4); vv(0.0,0.8); vv(0.8,0.0);
      vv(0.0,-0.8); vv(0.0,-0.4); EC;
}

static void draw_arrow1bar(Fl_Color col)
{
  draw_arrow1(col);
  rectangle(.6,-.8,.9,.8,col);
}

static void draw_arrow2(Fl_Color col)
{
  fl_color(col);
  BP; vv(-0.3,0.8); vv(0.50,0.0); vv(-0.3,-0.8); EP;
  set_outline_color(col);
  BC; vv(-0.3,0.8); vv(0.50,0.0); vv(-0.3,-0.8); EC;
}

static void draw_arrow3(Fl_Color col)
{
  fl_color(col);
  BP; vv(0.1,0.8); vv(0.9,0.0); vv(0.1,-0.8); EP;
  BP; vv(-0.7,0.8); vv(0.1,0.0); vv(-0.7,-0.8); EP;
  set_outline_color(col);
  BC; vv(0.1,0.8); vv(0.9,0.0); vv(0.1,-0.8); EC;
  BC; vv(-0.7,0.8); vv(0.1,0.0); vv(-0.7,-0.8); EC;
}

static void draw_arrowbar(Fl_Color col)
{
  fl_color(col);
  BP; vv(0.2,0.8); vv(0.6,0.8); vv(0.6,-0.8); vv(0.2,-0.8); EP;
  BP; vv(-0.6,0.8); vv(0.2,0.0); vv(-0.6,-0.8); EP;
  set_outline_color(col);
  BC; vv(0.2,0.8); vv(0.6,0.8); vv(0.6,-0.8); vv(0.2,-0.8); EC;
  BC; vv(-0.6,0.8); vv(0.2,0.0); vv(-0.6,-0.8); EC;
}

static void draw_arrowbox(Fl_Color col)
{
  fl_color(col);
  BP; vv(-0.6,0.8); vv(0.2,0.0); vv(-0.6,-0.8); EP;
  BC; vv(0.2,0.8); vv(0.6,0.8); vv(0.6,-0.8); vv(0.2,-0.8); EC;
  set_outline_color(col);
  BC; vv(0.2,0.8); vv(0.6,0.8); vv(0.6,-0.8); vv(0.2,-0.8); EC;
  BC; vv(-0.6,0.8); vv(0.2,0.0); vv(-0.6,-0.8); EC;
}

static void draw_bararrow(Fl_Color col)
{
  fl_color(col);
  BP; vv(0.1,0.8); vv(0.9,0.0); vv(0.1,-0.8); EP;
  BP; vv(-0.5,0.8); vv(-0.1,0.8); vv(-0.1,-0.8); vv(-0.5,-0.8); EP;
  set_outline_color(col);
  BC; vv(0.1,0.8); vv(0.9,0.0); vv(0.1,-0.8); EC;
  BC; vv(-0.5,0.8); vv(-0.1,0.8); vv(-0.1,-0.8); vv(-0.5,-0.8); EC;
}

static void draw_doublebar(Fl_Color col) { 
  rectangle(-0.6,-0.8,-.1,.8,col);
  rectangle(.1,-0.8,.6,.8,col); 
}

static void draw_arrow01(Fl_Color col)
  { fl_rotate(180); draw_arrow1(col); }

static void draw_arrow02(Fl_Color col)
  { fl_rotate(180); draw_arrow2(col); }

static void draw_arrow03(Fl_Color col)
  { fl_rotate(180); draw_arrow3(col); }

static void draw_0arrowbar(Fl_Color col)
  { fl_rotate(180); draw_arrowbar(col); }

static void draw_0arrowbox(Fl_Color col)
  { fl_rotate(180); draw_arrowbox(col); }

static void draw_0bararrow(Fl_Color col)
  { fl_rotate(180); draw_bararrow(col); }

static void draw_doublearrow(Fl_Color col)
{
  fl_color(col);
  BP; vv(-0.35,-0.4); vv(-0.35,0.4); vv(0.35,0.4); vv(0.35,-0.4); EP;
  BP; vv(0.15,0.8); vv(0.95,0.0); vv(0.15,-0.8); EP;
  BP; vv(-0.15,0.8); vv(-0.95,0.0); vv(-0.15,-0.8); EP;
  set_outline_color(col);
  BC; vv(-0.15,0.4); vv(0.15,0.4); vv(0.15,0.8); vv(0.95,0.0);
      vv(0.15,-0.8); vv(0.15,-0.4); vv(-0.15,-0.4); vv(-0.15,-0.8);
      vv(-0.95,0.0); vv(-0.15,0.8); EC;
}

static void draw_arrow(Fl_Color col)
{
  fl_color(col);
  BP; vv(0.65,0.1); vv(1.0,0.0); vv(0.65,-0.1); EP;
  BL; vv(-1.0,0.0); vv(0.65,0.0); EL;
  set_outline_color(col);
  BL; vv(-1.0,0.0); vv(0.65,0.0); EL;
  BC; vv(0.65,0.1); vv(1.0,0.0); vv(0.65,-0.1); EC;
}

static void draw_square(Fl_Color col)
  { rectangle(-1,-1,1,1,col); }

static void draw_circle(Fl_Color col) {
  fl_color(col); BP; fl_circle(0,0,1); EP;
  set_outline_color(col);
  BC; fl_circle(0,0,1); EC;
}

static void draw_line(Fl_Color col)
  { fl_color(col); BL; vv(-1.0,0.0); vv(1.0,0.0); EL; }

static void draw_plus(Fl_Color col)
{
  fl_color(col);
  BP; vv(-0.9,-0.15); vv(-0.9,0.15); vv(0.9,0.15); vv(0.9,-0.15); EP;
  BP; vv(-0.15,-0.9); vv(-0.15,0.9); vv(0.15,0.9); vv(0.15,-0.9); EP;
  set_outline_color(col);
  BC;
  vv(-0.9,-0.15); vv(-0.9,0.15); vv(-0.15,0.15); vv(-0.15,0.9);
  vv(0.15,0.9); vv(0.15,0.15); vv(0.9,0.15); vv(0.9,-0.15);
  vv(0.15,-0.15); vv(0.15,-0.9); vv(-0.15,-0.9); vv(-0.15,-0.15);
  EC;
}

static void draw_uparrow(Fl_Color) {
  fl_color(FL_LIGHT3);
  BL; vv(-.8,.8); vv(-.8,-.8); vv(.8,0); EL;
  fl_color(FL_DARK3);
  BL; vv(-.8,.8); vv(.8, 0); EL;
}

static void draw_downarrow(Fl_Color) {
  fl_color(FL_DARK3);
  BL; vv(-.8,.8); vv(-.8,-.8); vv(.8,0); EL;
  fl_color(FL_LIGHT3);
  BL; vv(-.8,.8); vv(.8, 0); EL;
}

static void draw_menu(Fl_Color col)
{
  rectangle(-0.65, 0.85, 0.65, -0.25, col);
  rectangle(-0.65, -0.6, 0.65, -1.0, col);
}

// Standard UI icons...
static void draw_filenew(Fl_Color c) {
  fl_color(c);
  BCP;
    vv(-0.7, -1.0);
    vv(0.1, -1.0);
    vv(0.1, -0.4);
    vv(0.7, -0.4);
    vv(0.7, 1.0);
    vv(-0.7, 1.0);
  ECP;

  fl_color(fl_lighter(c));
  BP;
    vv(0.1, -1.0);
    vv(0.1, -0.4);
    vv(0.7, -0.4);
  EP;

  fl_color(fl_darker(c));
  BC;
    vv(-0.7, -1.0);
    vv(0.1, -1.0);
    vv(0.1, -0.4);
    vv(0.7, -0.4);
    vv(0.7, 1.0);
    vv(-0.7, 1.0);
  EC;

  BL;
    vv(0.1, -1.0);
    vv(0.7, -0.4);
  EL;
}

static void draw_fileopen(Fl_Color c) {
  fl_color(c);
  BP;
    vv(-1.0, -0.7);
    vv(-0.9, -0.8);
    vv(-0.4, -0.8);
    vv(-0.3, -0.7);
    vv(0.6, -0.7);
    vv(0.6, 0.7);
    vv(-1.0, 0.7);
  EP;

  fl_color(fl_darker(c));
  BC;
    vv(-1.0, -0.7);
    vv(-0.9, -0.8);
    vv(-0.4, -0.8);
    vv(-0.3, -0.7);
    vv(0.6, -0.7);
    vv(0.6, 0.7);
    vv(-1.0, 0.7);
  EC;

  fl_color(fl_lighter(c));
  BP;
    vv(-1.0, 0.7);
    vv(-0.6, -0.3);
    vv(1.0, -0.3);
    vv(0.6, 0.7);
  EP;

  fl_color(fl_darker(c));
  BC;
    vv(-1.0, 0.7);
    vv(-0.6, -0.3);
    vv(1.0, -0.3);
    vv(0.6, 0.7);
  EC;
}

static void draw_filesave(Fl_Color c) {
  fl_color(c);
  BP;
    vv(-0.9, -1.0);
    vv(0.9, -1.0);
    vv(1.0, -0.9);
    vv(1.0, 0.9);
    vv(0.9, 1.0);
    vv(-0.9, 1.0);
    vv(-1.0, 0.9);
    vv(-1.0, -0.9);
  EP;

  fl_color(fl_lighter(c));
  BP;
    vv(-0.7, -1.0);
    vv(0.7, -1.0);
    vv(0.7, -0.4);
    vv(-0.7, -0.4);
  EP;

  BP;
    vv(-0.7, 0.0);
    vv(0.7, 0.0);
    vv(0.7, 1.0);
    vv(-0.7, 1.0);
  EP;

  fl_color(c);
  BP;
    vv(-0.5, -0.9);
    vv(-0.3, -0.9);
    vv(-0.3, -0.5);
    vv(-0.5, -0.5);
  EP;

  fl_color(fl_darker(c));
  BC;
    vv(-0.9, -1.0);
    vv(0.9, -1.0);
    vv(1.0, -0.9);
    vv(1.0, 0.9);
    vv(0.9, 1.0);
    vv(-0.9, 1.0);
    vv(-1.0, 0.9);
    vv(-1.0, -0.9);
  EC;
}

static void draw_filesaveas(Fl_Color c) {
  draw_filesave(c);

  fl_color(fl_color_average(c, FL_WHITE, 0.25f));
  BP;
    vv(0.6, -0.8);
    vv(1.0, -0.4);
    vv(0.0, 0.6);
    vv(-0.4, 0.6);
    vv(-0.4, 0.2);
  EP;

  fl_color(fl_darker(c));
  BC;
    vv(0.6, -0.8);
    vv(1.0, -0.4);
    vv(0.0, 0.6);
    vv(-0.4, 0.6);
    vv(-0.4, 0.2);
  EC;

  BP;
    vv(-0.1, 0.6);
    vv(-0.4, 0.6);
    vv(-0.4, 0.3);
  EP;
}

static void draw_fileprint(Fl_Color c) {
  fl_color(c);
  BP;
    vv(-0.8, 0.0);
    vv(0.8, 0.0);
    vv(1.0, 0.2);
    vv(1.0, 1.0);
    vv(-1.0, 1.0);
    vv(-1.0, 0.2);
  EP;

  fl_color(fl_color_average(c, FL_WHITE, 0.25f));
  BP;
    vv(-0.6, 0.0);
    vv(-0.6, -1.0);
    vv(0.6, -1.0);
    vv(0.6, 0.0);
  EP;

  fl_color(fl_lighter(c));
  BP;
    vv(-0.6, 0.6);
    vv(0.6, 0.6);
    vv(0.6, 1.0);
    vv(-0.6, 1.0);
  EP;

  fl_color(fl_darker(c));
  BC;
    vv(-0.8, 0.0);
    vv(-0.6, 0.0);
    vv(-0.6, -1.0);
    vv(0.6, -1.0);
    vv(0.6, 0.0);
    vv(0.8, 0.0);
    vv(1.0, 0.2);
    vv(1.0, 1.0);
    vv(-1.0, 1.0);
    vv(-1.0, 0.2);
  EC;

  BC;
    vv(-0.6, 0.6);
    vv(0.6, 0.6);
    vv(0.6, 1.0);
    vv(-0.6, 1.0);
  EC;
}

static void draw_round_arrow(Fl_Color c, float da=5.0) {
  double a, r, dr1=0.005, dr2=0.015;
  int i, j;
  for (j=0; j<2; j++) {
    if (j&1) {
      fl_color(c);
      set_outline_color(c);
      BC;
    } else {
      fl_color(c);
      BCP;
    }
    vv(-0.1, 0.0);
    vv(-1.0, 0.0);
    vv(-1.0, 0.9);
    for (i=27, a=140.0, r=1.0; i>0; i--, a-=da, r-=dr1) {
      double ar = a/180.0 * M_PI;
      vv(cos(ar)*r, sin(ar)*r);
    }
    for (i=27; i>=0; a+=da, i--, r-=dr2) {
      double ar = a/180.0 * M_PI;
      vv(cos(ar)*r, sin(ar)*r);
    }
    if (j&1) {
      EC;
    } else {
      ECP;
    }
  }
}

static void draw_refresh(Fl_Color c) {
  draw_round_arrow(c);
  fl_rotate(180.0);
  draw_round_arrow(c);
  fl_rotate(-180.0);
}

static void draw_reload(Fl_Color c) {
  fl_rotate(-135.0);
  draw_round_arrow(c, 10);
  fl_rotate(135.0);
}

static void draw_undo(Fl_Color c) {
  fl_translate(0.0, 0.2);
  fl_scale(1.0, -1.0);
  draw_round_arrow(c, 6);
  fl_scale(1.0, -1.0);
  fl_translate(0.0, -0.2);
}

static void draw_redo(Fl_Color c) {
  fl_scale(-1.0, 1.0);
  draw_undo(c);
  fl_scale(-1.0, 1.0);
}

static void fl_init_symbols(void) {
  static char beenhere;
  if (beenhere) return;
  beenhere = 1;
  symbnumb = 0;

  fl_add_symbol("",		draw_arrow1,		1);
  fl_add_symbol("->",		draw_arrow1,		1);
  fl_add_symbol(">",		draw_arrow2,		1);
  fl_add_symbol(">>",		draw_arrow3,		1);
  fl_add_symbol(">|",		draw_arrowbar,		1);
  fl_add_symbol(">[]",		draw_arrowbox,		1);
  fl_add_symbol("|>",		draw_bararrow,		1);
  fl_add_symbol("<-",		draw_arrow01,		1);
  fl_add_symbol("<",		draw_arrow02,		1);
  fl_add_symbol("<<",		draw_arrow03,		1);
  fl_add_symbol("|<",		draw_0arrowbar,		1);
  fl_add_symbol("[]<",		draw_0arrowbox,		1);
  fl_add_symbol("<|",		draw_0bararrow,		1);
  fl_add_symbol("<->",		draw_doublearrow,	1);
  fl_add_symbol("-->",		draw_arrow,		1);
  fl_add_symbol("+",		draw_plus,		1);
  fl_add_symbol("->|",		draw_arrow1bar,		1);
  fl_add_symbol("arrow",	draw_arrow,		1);
  fl_add_symbol("returnarrow",	0,			3);
  fl_add_symbol("square",	draw_square,		1);
  fl_add_symbol("circle",	draw_circle,		1);
  fl_add_symbol("line",		draw_line,		1);
  fl_add_symbol("plus",		draw_plus,		1);
  fl_add_symbol("menu",		draw_menu,		1);
  fl_add_symbol("UpArrow",	draw_uparrow,		1);
  fl_add_symbol("DnArrow",	draw_downarrow,		1);
  fl_add_symbol("||",		draw_doublebar,		1);
  fl_add_symbol("search",       draw_search,            1);
  fl_add_symbol("FLTK",         draw_fltk,              1);

  fl_add_symbol("filenew",      draw_filenew,           1);
  fl_add_symbol("fileopen",     draw_fileopen,          1);
  fl_add_symbol("filesave",     draw_filesave,          1);
  fl_add_symbol("filesaveas",   draw_filesaveas,        1);
  fl_add_symbol("fileprint",    draw_fileprint,         1);

  fl_add_symbol("refresh",      draw_refresh,           1);
  fl_add_symbol("reload",       draw_reload,            1);
  fl_add_symbol("undo",         draw_undo,              1);
  fl_add_symbol("redo",         draw_redo,              1);

//  fl_add_symbol("file",      draw_file,           1);
}

//
// End of "$Id: fl_symbols.cxx 5190 2006-06-09 16:16:34Z mike $".
//
