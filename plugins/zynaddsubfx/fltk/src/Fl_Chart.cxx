//
// "$Id: Fl_Chart.cxx 5942 2007-10-06 17:33:17Z matt $"
//
// Forms-compatible chart widget for the Fast Light Tool Kit (FLTK).
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

#include <FL/math.h>
#include <FL/Fl.H>
#include <FL/Fl_Chart.H>
#include <FL/fl_draw.H>
#include "flstring.h"
#include <stdlib.h>

#define ARCINC	(2.0*M_PI/360.0)

// this function is in fl_boxtype.cxx:
void fl_rectbound(int x,int y,int w,int h, Fl_Color color);

/* Widget specific information */

static void draw_barchart(int x,int y,int w,int h,
			  int numb, FL_CHART_ENTRY entries[],
			  double min, double max, int autosize, int maxnumb,
			  Fl_Color textcolor)
/* Draws a bar chart. x,y,w,h is the bounding box, entries the array of
   numb entries and min and max the boundaries. */
{
  double incr;
  int zeroh;
  double lh = fl_height();
  if (max == min) incr = h;
  else incr = h/(max-min);
  if ( (-min*incr) < lh) {
      incr = (h - lh + min*incr)/(max-min);
      zeroh = int(y+h-lh);
  } else {
      zeroh = (int)rint(y+h+min * incr);
  }
  int bwidth = (int)rint(w/double(autosize?numb:maxnumb));
  /* Draw base line */
  fl_color(textcolor);
  fl_line(x, zeroh, x+w, zeroh);
  if (min == 0.0 && max == 0.0) return; /* Nothing else to draw */
  int i;
  /* Draw the bars */
  for (i=0; i<numb; i++) {
      int hh = (int)rint(entries[i].val*incr);
      if (hh < 0)
	fl_rectbound(x+i*bwidth,zeroh,bwidth+1,-hh+1, (Fl_Color)entries[i].col);
      else if (hh > 0)
	fl_rectbound(x+i*bwidth,zeroh-hh,bwidth+1,hh+1,(Fl_Color)entries[i].col);
  }
  /* Draw the labels */
  fl_color(textcolor);
  for (i=0; i<numb; i++)
      fl_draw(entries[i].str,
	      x+i*bwidth+bwidth/2,zeroh,0,0,
	      FL_ALIGN_TOP);
}

static void draw_horbarchart(int x,int y,int w,int h,
			     int numb, FL_CHART_ENTRY entries[],
			     double min, double max, int autosize, int maxnumb,
			     Fl_Color textcolor)
/* Draws a horizontal bar chart. x,y,w,h is the bounding box, entries the
   array of numb entries and min and max the boundaries. */
{
  int i;
  double lw = 0.0;		/* Maximal label width */
  /* Compute maximal label width */
  for (i=0; i<numb; i++) {
      double w1 = fl_width(entries[i].str);
      if (w1 > lw) lw = w1;
  }
  if (lw > 0.0) lw += 4.0;
  double incr;
  int zeroh;
  if (max == min) incr = w;
  else incr = w/(max-min);
  if ( (-min*incr) < lw) {
      incr = (w - lw + min*incr)/(max-min);
      zeroh = x+(int)rint(lw);
  } else {
      zeroh = (int)rint(x-min * incr);
  }
  int bwidth = (int)rint(h/double(autosize?numb:maxnumb));
  /* Draw base line */
  fl_color(textcolor);
  fl_line(zeroh, y, zeroh, y+h);
  if (min == 0.0 && max == 0.0) return; /* Nothing else to draw */
  /* Draw the bars */
  for (i=0; i<numb; i++) {
      int ww = (int)rint(entries[i].val*incr);
      if (ww > 0)
	fl_rectbound(zeroh,y+i*bwidth,ww+1,bwidth+1, (Fl_Color)entries[i].col);
      else if (ww < 0)
	fl_rectbound(zeroh+ww,y+i*bwidth,-ww+1,bwidth+1,(Fl_Color)entries[i].col);
  }
  /* Draw the labels */
  fl_color(textcolor);
  for (i=0; i<numb; i++)
      fl_draw(entries[i].str,
	      zeroh-2,y+i*bwidth+bwidth/2,0,0,
	      FL_ALIGN_RIGHT);
}

static void draw_linechart(int type, int x,int y,int w,int h,
			   int numb, FL_CHART_ENTRY entries[],
			   double min, double max, int autosize, int maxnumb,
			   Fl_Color textcolor)
/* Draws a line chart. x,y,w,h is the bounding box, entries the array of
   numb entries and min and max the boundaries. */
{
  int i;
  double lh = fl_height();
  double incr;
  if (max == min) incr = h-2.0*lh;
  else incr = (h-2.0*lh)/ (max-min);
  int zeroh = (int)rint(y+h-lh+min * incr);
  double bwidth = w/double(autosize?numb:maxnumb);
  /* Draw the values */
  for (i=0; i<numb; i++) {
      int x0 = x + (int)rint((i-.5)*bwidth);
      int x1 = x + (int)rint((i+.5)*bwidth);
      int yy0 = i ? zeroh - (int)rint(entries[i-1].val*incr) : 0;
      int yy1 = zeroh - (int)rint(entries[i].val*incr);
      if (type == FL_SPIKE_CHART) {
	  fl_color((Fl_Color)entries[i].col);
	  fl_line(x1, zeroh, x1, yy1);
      } else if (type == FL_LINE_CHART && i != 0) {
	  fl_color((Fl_Color)entries[i-1].col);
	  fl_line(x0,yy0,x1,yy1);
      } else if (type == FL_FILLED_CHART && i != 0) {
	  fl_color((Fl_Color)entries[i-1].col);
	  if ((entries[i-1].val>0.0)!=(entries[i].val>0.0)) {
	      double ttt = entries[i-1].val/(entries[i-1].val-entries[i].val);
	      int xt = x + (int)rint((i-.5+ttt)*bwidth);
	      fl_polygon(x0,zeroh, x0,yy0, xt,zeroh);
	      fl_polygon(xt,zeroh, x1,yy1, x1,zeroh);
	  } else {
	      fl_polygon(x0,zeroh, x0,yy0, x1,yy1, x1,zeroh);
	  }
	  fl_color(textcolor);
	  fl_line(x0,yy0,x1,yy1);
      }
  }
  /* Draw base line */
  fl_color(textcolor);
  fl_line(x,zeroh,x+w,zeroh);
  /* Draw the labels */
  for (i=0; i<numb; i++)
      fl_draw(entries[i].str,
	      x+(int)rint((i+.5)*bwidth), zeroh - (int)rint(entries[i].val*incr),0,0,
	      entries[i].val>=0 ? FL_ALIGN_BOTTOM : FL_ALIGN_TOP);
}

static void draw_piechart(int x,int y,int w,int h,
			  int numb, FL_CHART_ENTRY entries[], int special,
			  Fl_Color textcolor)
/* Draws a pie chart. x,y,w,h is the bounding box, entries the array of
   numb entries */
{
  int i;
  double xc,yc,rad;	/* center and radius */
  double tot;		/* sum of values */
  double incr;		/* increment in angle */
  double curang;		/* current angle we are drawing */
  double txc,tyc;	/* temporary center */
  double lh = fl_height();
  /* compute center and radius */
  double h_denom = (special ? 2.3 : 2.0);
  rad = (h - 2*lh)/h_denom/1.1;
  xc = x+w/2.0; yc = y+h-1.1*rad-lh;
  /* compute sum of values */
  tot = 0.0;
  for (i=0; i<numb; i++)
    if (entries[i].val > 0.0) tot += entries[i].val;
  if (tot == 0.0) return;
  incr = 360.0/tot;
  /* Draw the pie */
  curang = 0.0;
  for (i=0; i<numb; i++)
    if (entries[i].val > 0.0)
    {
      txc = xc; tyc = yc;
      /* Correct for special pies */
      if (special && i==0)
      {
        txc += 0.3*rad*cos(ARCINC*(curang+0.5*incr*entries[i].val));
        tyc -= 0.3*rad*sin(ARCINC*(curang+0.5*incr*entries[i].val));
      }
      fl_color((Fl_Color)entries[i].col);
      fl_begin_polygon(); fl_vertex(txc,tyc);
      fl_arc(txc,tyc,rad,curang, curang+incr*entries[i].val);
      fl_end_polygon();
      fl_color(textcolor);
      fl_begin_loop(); fl_vertex(txc,tyc);
      fl_arc(txc,tyc,rad,curang, curang+incr*entries[i].val);
      fl_end_loop();
      curang += 0.5 * incr * entries[i].val;
      /* draw the label */
      double xl = txc + 1.1*rad*cos(ARCINC*curang);
      fl_draw(entries[i].str,
	      (int)rint(xl),
	      (int)rint(tyc - 1.1*rad*sin(ARCINC*curang)),
	      0, 0,
	      xl<txc ? FL_ALIGN_RIGHT : FL_ALIGN_LEFT);
      curang += 0.5 * incr * entries[i].val;
    }
}

void Fl_Chart::draw() {

    draw_box();
    Fl_Boxtype b = box();
    int xx = x()+Fl::box_dx(b); // was 9 instead of dx...
    int yy = y()+Fl::box_dy(b);
    int ww = w()-Fl::box_dw(b);
    int hh = h()-Fl::box_dh(b);
    fl_push_clip(xx, yy, ww, hh);

    ww--; hh--; // adjust for line thickness

    if (min >= max) {
	min = max = 0.0;
	for (int i=0; i<numb; i++) {
	    if (entries[i].val < min) min = entries[i].val;
	    if (entries[i].val > max) max = entries[i].val;
	}
    }

    fl_font(textfont(),textsize());

    switch (type()) {
    case FL_BAR_CHART:
	ww++; // makes the bars fill box correctly
	draw_barchart(xx,yy,ww,hh, numb, entries, min, max,
			autosize(), maxnumb, textcolor());
	break;
    case FL_HORBAR_CHART:
	hh++; // makes the bars fill box correctly
	draw_horbarchart(xx,yy,ww,hh, numb, entries, min, max,
			autosize(), maxnumb, textcolor());
	break;
    case FL_PIE_CHART:
	draw_piechart(xx,yy,ww,hh,numb,entries,0, textcolor());
	break;
    case FL_SPECIALPIE_CHART:
	draw_piechart(xx,yy,ww,hh,numb,entries,1,textcolor());
	break;
    default:
	draw_linechart(type(),xx,yy,ww,hh, numb, entries, min, max,
			autosize(), maxnumb, textcolor());
	break;
    }
    draw_label();
    fl_pop_clip();
}

/*------------------------------*/

#define FL_CHART_BOXTYPE	FL_BORDER_BOX
#define FL_CHART_COL1		FL_COL1
#define FL_CHART_LCOL		FL_LCOL
#define FL_CHART_ALIGN		FL_ALIGN_BOTTOM

Fl_Chart::Fl_Chart(int X, int Y, int W, int H,const char *l) :
Fl_Widget(X,Y,W,H,l) {
  box(FL_BORDER_BOX);
  align(FL_ALIGN_BOTTOM);
  numb       = 0;
  maxnumb    = 0;
  sizenumb   = FL_CHART_MAX;
  autosize_  = 1;
  min = max  = 0;
  textfont_  = FL_HELVETICA;
  textsize_  = 10;
  textcolor_ = FL_FOREGROUND_COLOR;
  entries    = (FL_CHART_ENTRY *)calloc(sizeof(FL_CHART_ENTRY), FL_CHART_MAX + 1);
}

Fl_Chart::~Fl_Chart() {
  free(entries);
}

void Fl_Chart::clear() {
  numb = 0;
  redraw();
}

void Fl_Chart::add(double val, const char *str, unsigned col) {
  /* Allocate more entries if required */
  if (numb >= sizenumb) {
    sizenumb += FL_CHART_MAX;
    entries = (FL_CHART_ENTRY *)realloc(entries, sizeof(FL_CHART_ENTRY) * (sizenumb + 1));
  }
  // Shift entries as needed
  if (numb >= maxnumb && maxnumb > 0) {
    memmove(entries, entries + 1, sizeof(FL_CHART_ENTRY) * (numb - 1));
    numb --;
  }
  entries[numb].val = float(val);
  entries[numb].col = col;
    if (str) {
	strlcpy(entries[numb].str,str,FL_CHART_LABEL_MAX + 1);
    } else {
	entries[numb].str[0] = 0;
    }
  numb++;
  redraw();
}

void Fl_Chart::insert(int ind, double val, const char *str, unsigned col) {
  int i;
  if (ind < 1 || ind > numb+1) return;
  /* Allocate more entries if required */
  if (numb >= sizenumb) {
    sizenumb += FL_CHART_MAX;
    entries = (FL_CHART_ENTRY *)realloc(entries, sizeof(FL_CHART_ENTRY) * (sizenumb + 1));
  }
  // Shift entries as needed
  for (i=numb; i >= ind; i--) entries[i] = entries[i-1];
  if (numb < maxnumb || maxnumb == 0) numb++;
  /* Fill in the new entry */
  entries[ind-1].val = float(val);
  entries[ind-1].col = col;
  if (str) {
      strlcpy(entries[ind-1].str,str,FL_CHART_LABEL_MAX+1);
  } else {
      entries[ind-1].str[0] = 0;
  }
  redraw();
}

void Fl_Chart::replace(int ind,double val, const char *str, unsigned col) {
  if (ind < 1 || ind > numb) return;
  entries[ind-1].val = float(val);
  entries[ind-1].col = col;
  if (str) {
      strlcpy(entries[ind-1].str,str,FL_CHART_LABEL_MAX+1);
  } else {
      entries[ind-1].str[0] = 0;
  }
  redraw();
}

void Fl_Chart::bounds(double mymin, double mymax) {
  this->min = mymin;
  this->max = mymax;
  redraw();
}

void Fl_Chart::maxsize(int m) {
  int i;
  /* Fill in the new number */
  if (m < 0) return;
  maxnumb = m;
  /* Shift entries if required */
  if (numb > maxnumb) {
      for (i = 0; i<maxnumb; i++)
	  entries[i] = entries[i+numb-maxnumb];
      numb = maxnumb;
      redraw();
  }
}

//
// End of "$Id: Fl_Chart.cxx 5942 2007-10-06 17:33:17Z matt $".
//
