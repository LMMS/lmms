//
// "$Id: Fl_Progress.cxx 5540 2006-11-12 20:44:12Z matt $"
//
// Progress bar widget routines.
//
// Copyright 2000-2005 by Michael Sweet.
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
// Contents:
//
//   Fl_Progress::draw()        - Draw the check button.
//   Fl_Progress::Fl_Progress() - Construct a Fl_Progress widget.
//

//
// Include necessary header files...
//

#include <FL/Fl.H>
#include <FL/Fl_Progress.H>
#include <FL/fl_draw.H>


//
// Fl_Progress is a progress bar widget based off Fl_Widget that shows a
// standard progress bar...
//


//
// 'Fl_Progress::draw()' - Draw the check button.
//

void Fl_Progress::draw()
{
  int	progress;	// Size of progress bar...
  int	bx, by, bw, bh;	// Box areas...
  int	tx, tw;		// Temporary X + width


  // Get the box borders...
  bx = Fl::box_dx(box());
  by = Fl::box_dy(box());
  bw = Fl::box_dw(box());
  bh = Fl::box_dh(box());

  tx = x() + bx;
  tw = w() - bw;

  // Draw the progress bar...
  if (maximum_ > minimum_)
    progress = (int)(w() * (value_ - minimum_) / (maximum_ - minimum_) + 0.5f);
  else
    progress = 0;

  // Draw the box and label...
  if (progress > 0) {
    Fl_Color c = labelcolor();
    labelcolor(fl_contrast(labelcolor(), color2()));

    fl_clip(x(), y(), progress + bx, h());
      draw_box(box(), x(), y(), w(), h(), active_r() ? color2() : fl_inactive(color2()));
      draw_label(tx, y() + by, tw, h() - bh);
    fl_pop_clip();

    labelcolor(c);

    if (progress<w()) {
      fl_clip(tx + progress, y(), w() - progress, h());
        draw_box(box(), x(), y(), w(), h(), active_r() ? color() : fl_inactive(color()));
        draw_label(tx, y() + by, tw, h() - bh);
      fl_pop_clip();
    }
  } else {
    draw_box(box(), x(), y(), w(), h(), active_r() ? color() : fl_inactive(color()));
    draw_label(tx, y() + by, tw, h() - bh);
  }
}


//
// 'Fl_Progress::Fl_Progress()' - Construct a Fl_Progress widget.
//

Fl_Progress::Fl_Progress(int X, int Y, int W, int H, const char* l)
: Fl_Widget(X, Y, W, H, l)
{
  align(FL_ALIGN_INSIDE);
  box(FL_DOWN_BOX);
  color(FL_BACKGROUND2_COLOR, FL_YELLOW);
  minimum(0.0f);
  maximum(100.0f);
  value(0.0f);
}


//
// End of "$Id: Fl_Progress.cxx 5540 2006-11-12 20:44:12Z matt $".
//
