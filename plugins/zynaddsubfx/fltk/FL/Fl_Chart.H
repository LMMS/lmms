//
// "$Id: Fl_Chart.H 7981 2010-12-08 23:53:04Z greg.ercolano $"
//
// Forms chart header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Chart widget . */

#ifndef Fl_Chart_H
#define Fl_Chart_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

// values for type()
#define FL_BAR_CHART		0	/**< type() for Bar Chart variant */
#define FL_HORBAR_CHART		1	/**< type() for Horizontal Bar Chart variant */
#define FL_LINE_CHART		2	/**< type() for Line Chart variant */
#define FL_FILL_CHART		3	/**< type() for Fill Line Chart variant */
#define FL_SPIKE_CHART		4	/**< type() for Spike Chart variant */
#define FL_PIE_CHART		5	/**< type() for Pie Chart variant */
#define FL_SPECIALPIE_CHART	6	/**< type() for Special Pie Chart variant */

#define FL_FILLED_CHART  FL_FILL_CHART	/**< for compatibility */

#define FL_CHART_MAX		128	/**< max entries per chart */
#define FL_CHART_LABEL_MAX	18	/**< max label length for entry */

/** For internal use only */
struct FL_CHART_ENTRY {
   float val;				/**< For internal use only. */
   unsigned col;			/**< For internal use only. */
   char str[FL_CHART_LABEL_MAX+1];	/**< For internal use only. */
};

/**
  \class Fl_Chart
  \brief Fl_Chart displays simple charts.
  It is provided for Forms compatibility.

  \image html charts.png  
  \image latex charts.png  "Fl_Chart" width=10cm
  \todo Refactor Fl_Chart::type() information.

  The type of an Fl_Chart object can be set using type(uchar t) to:
  \li \c FL_BAR_CHART: Each sample value is drawn as a vertical bar.
  \li \c FL_FILLED_CHART: The chart is filled from the bottom of the graph
         to the sample values.
  \li \c FL_HORBAR_CHART: Each sample value is drawn as a horizontal bar.
  \li \c FL_LINE_CHART: The chart is drawn as a polyline with vertices at
         each sample value.
  \li \c FL_PIE_CHART: A pie chart is drawn with each sample value being
         drawn as a proportionate slice in the circle.
  \li \c FL_SPECIALPIE_CHART: Like \c FL_PIE_CHART, but the first slice is
         separated from the pie.
  \li \c FL_SPIKE_CHART: Each sample value is drawn as a vertical line.
 */
class FL_EXPORT Fl_Chart : public Fl_Widget {
    int numb;
    int maxnumb;
    int sizenumb;
    FL_CHART_ENTRY *entries;
    double min,max;
    uchar autosize_;
    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    Fl_Color textcolor_;
protected:
    void draw();
public:
    Fl_Chart(int X, int Y, int W, int H, const char *L = 0);

    ~Fl_Chart();

    void clear();

    void add(double val, const char *str = 0, unsigned col = 0);

    void insert(int ind, double val, const char *str = 0, unsigned col = 0);

    void replace(int ind, double val, const char *str = 0, unsigned col = 0);

    /**
      Gets the lower and upper bounds of the chart values.
      \param[out] a, b are set to lower, upper
     */
    void bounds(double *a,double *b) const {*a = min; *b = max;}

    void bounds(double a,double b);

    /**
      Returns the number of data values in the chart.
     */
    int size() const {return numb;}

    void size(int W, int H) { Fl_Widget::size(W, H); }

    /**
      Gets the maximum number of data values for a chart.
     */
    int maxsize() const {return maxnumb;}

    void maxsize(int m);

    /** Gets the chart's text font */
    Fl_Font textfont() const {return textfont_;}
    /** Sets the chart's text font to \p s. */
    void textfont(Fl_Font s) {textfont_ = s;}

    /** Gets the chart's text size */
    Fl_Fontsize textsize() const {return textsize_;}
    /** gets the chart's text size to \p s. */
    void textsize(Fl_Fontsize s) {textsize_ = s;}

    /** Gets the chart's text color */
    Fl_Color textcolor() const {return textcolor_;}
    /** gets the chart's text color to \p n. */
    void textcolor(Fl_Color n) {textcolor_ = n;}

    /**
      Get whether the chart will automatically adjust the bounds of the chart.
      \returns non-zero if auto-sizing is enabled and zero if disabled.
     */
    uchar autosize() const {return autosize_;}

    /**
      Set whether the chart will automatically adjust the bounds of the chart.
      \param[in] n non-zero to enable automatic resizing, zero to disable.
     */
    void autosize(uchar n) {autosize_ = n;}
};

#endif

//
// End of "$Id: Fl_Chart.H 7981 2010-12-08 23:53:04Z greg.ercolano $".
//
