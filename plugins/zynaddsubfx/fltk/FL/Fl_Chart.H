//
// "$Id: Fl_Chart.H 4288 2005-04-16 00:13:17Z mike $"
//
// Forms chart header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Chart_H
#define Fl_Chart_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

// values for type()
#define FL_BAR_CHART		0
#define FL_HORBAR_CHART		1
#define FL_LINE_CHART		2
#define FL_FILL_CHART		3
#define FL_SPIKE_CHART		4
#define FL_PIE_CHART		5
#define FL_SPECIALPIE_CHART	6

#define FL_FILLED_CHART  FL_FILL_CHART	// compatibility

#define FL_CHART_MAX		128
#define FL_CHART_LABEL_MAX	18

struct FL_CHART_ENTRY {
   float val;
   unsigned col;
   char str[FL_CHART_LABEL_MAX+1];
};

class FL_EXPORT Fl_Chart : public Fl_Widget {
    int numb;
    int maxnumb;
    int sizenumb;
    FL_CHART_ENTRY *entries;
    double min,max;
    uchar autosize_;
    uchar textfont_,textsize_;
    unsigned textcolor_;
protected:
    void draw();
public:
    Fl_Chart(int,int,int,int,const char * = 0);
    ~Fl_Chart();
    void clear();
    void add(double, const char * =0, unsigned=0);
    void insert(int, double, const char * =0, unsigned=0);
    void replace(int, double, const char * =0, unsigned=0);
    void bounds(double *a,double *b) const {*a = min; *b = max;}
    void bounds(double a,double b);
    int size() const {return numb;}
    void size(int W, int H) { Fl_Widget::size(W, H); }
    int maxsize() const {return maxnumb;}
    void maxsize(int);
    Fl_Font textfont() const {return (Fl_Font)textfont_;}
    void textfont(uchar s) {textfont_ = s;}
    uchar textsize() const {return textsize_;}
    void textsize(uchar s) {textsize_ = s;}
    Fl_Color textcolor() const {return (Fl_Color)textcolor_;}
    void textcolor(unsigned n) {textcolor_ = n;}
    uchar autosize() const {return autosize_;}
    void autosize(uchar n) {autosize_ = n;}
};

#endif

//
// End of "$Id: Fl_Chart.H 4288 2005-04-16 00:13:17Z mike $".
//
