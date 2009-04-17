//
// "$Id: Fl_Help_View.cxx 6091 2008-04-11 11:12:16Z matt $"
//
// Fl_Help_View widget routines.
//
// Copyright 1997-2007 by Easy Software Products.
// Image support donated by Matthias Melcher, Copyright 2000.
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
//   Fl_Help_View::add_block()       - Add a text block to the list.
//   Fl_Help_View::add_link()        - Add a new link to the list.
//   Fl_Help_View::add_target()      - Add a new target to the list.
//   Fl_Help_View::compare_targets() - Compare two targets.
//   Fl_Help_View::do_align()        - Compute the alignment for a line in
//                                     a block.
//   Fl_Help_View::draw()            - Draw the Fl_Help_View widget.
//   Fl_Help_View::format()          - Format the help text.
//   Fl_Help_View::format_table()    - Format a table...
//   Fl_Help_View::free_data()       - Free memory used for the document.
//   Fl_Help_View::get_align()       - Get an alignment attribute.
//   Fl_Help_View::get_attr()        - Get an attribute value from the string.
//   Fl_Help_View::get_color()       - Get an alignment attribute.
//   Fl_Help_View::handle()          - Handle events in the widget.
//   Fl_Help_View::Fl_Help_View()    - Build a Fl_Help_View widget.
//   Fl_Help_View::~Fl_Help_View()   - Destroy a Fl_Help_View widget.
//   Fl_Help_View::load()            - Load the specified file.
//   Fl_Help_View::resize()          - Resize the help widget.
//   Fl_Help_View::topline()         - Set the top line to the named target.
//   Fl_Help_View::topline()         - Set the top line by number.
//   Fl_Help_View::value()           - Set the help text directly.
//   scrollbar_callback()            - A callback for the scrollbar.
//

//
// Include necessary header files...
//

#include <FL/Fl_Help_View.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Pixmap.H>
#include <FL/x.H>
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>

#if defined(WIN32) && ! defined(__CYGWIN__)
#  include <io.h>
#  include <direct.h>
// Visual C++ 2005 incorrectly displays a warning about the use of POSIX APIs
// on Windows, which is supposed to be POSIX compliant...
#  define getcwd _getcwd
#else
#  include <unistd.h>
#endif // WIN32

#define MAX_COLUMNS	200


//
// Typedef the C API sort function type the only way I know how...
//

extern "C"
{
  typedef int (*compare_func_t)(const void *, const void *);
}


//
// Local functions...
//

static int	quote_char(const char *);
static void	scrollbar_callback(Fl_Widget *s, void *);
static void	hscrollbar_callback(Fl_Widget *s, void *);


//
// Broken image...
//

static const char *broken_xpm[] =
		{
		  "16 24 4 1",
		  "@ c #000000",
		  "  c #ffffff",
		  "+ c none",
		  "x c #ff0000",
		  // pixels
		  "@@@@@@@+++++++++",
		  "@    @++++++++++",
		  "@   @+++++++++++",
		  "@   @++@++++++++",
		  "@    @@+++++++++",
		  "@     @+++@+++++",
		  "@     @++@@++++@",
		  "@ xxx  @@  @++@@",
		  "@  xxx    xx@@ @",
		  "@   xxx  xxx   @",
		  "@    xxxxxx    @",
		  "@     xxxx     @",
		  "@    xxxxxx    @",
		  "@   xxx  xxx   @",
		  "@  xxx    xxx  @",
		  "@ xxx      xxx @",
		  "@              @",
		  "@              @",
		  "@              @",
		  "@              @",
		  "@              @",
		  "@              @",
		  "@              @",
		  "@@@@@@@@@@@@@@@@",
		  NULL
		};

static Fl_Pixmap broken_image(broken_xpm);

//
// Simple margin stack for Fl_Help_View::format()...
//

struct fl_margins {
  int depth_;
  int margins_[100];

  fl_margins() { clear();  }

  int clear() {
//    puts("fl_margins::clear()");

    depth_ = 0;
    return margins_[0] = 4;
  }

  int current() { return margins_[depth_]; }

  int pop() {
//    printf("fl_margins::pop(): depth_=%d, xx=%d\n", depth_,
//           depth_ > 0 ? margins_[depth_ - 1] : 4);

    if (depth_ > 0) {
      depth_ --;
      return margins_[depth_];
    } else return 4;
  }

  int push(int indent) {
    int xx;

    xx = margins_[depth_] + indent;

//    printf("fl_margins::push(indent=%d): depth_=%d, xx=%d\n", indent,
//           depth_ + 1, xx);

    if (depth_ < 99) {
      depth_ ++;
      margins_[depth_] = xx;
    }

    return xx;
  }
};

//
// All the stuff needed to implement text selection in Fl_Help_View
//

/* matt:
 * We are trying to keep binary compatibility with previous versions
 * of FLTK. This means that we are limited to adding static variables
 * only to not enlarge the Fl_Help_View class. Lucky for us, only one
 * text can be selected system wide, so we can remember the selection
 * in a single set of variables.
 *
 * Still to do:
 * - &word; style characters mess up our count inside a word boundary
 * - we can only select words, no individual characters
 * - no dragging of the selection into another widget
 * - selection must be cleared if another widget get focus!
 * - write a comment for every new function
 */

/*
The following functions are also used to draw stuff and should be replaced with
local copies that are much faster when merely counting:

fl_color(Fl_Color);
fl_rectf(int, int, int, int);
fl_push_clip(int, int, int, int);
fl_xyline(int, int, int);
fl_rect()
fl_line()
img->draw()
*/

// We don't put the offscreen buffer in the help view class because
// we'd need to include x.H in the header...
static Fl_Offscreen fl_help_view_buffer;
int Fl_Help_View::selection_first = 0;
int Fl_Help_View::selection_last = 0;
int Fl_Help_View::selection_push_first = 0;
int Fl_Help_View::selection_push_last = 0;
int Fl_Help_View::selection_drag_first = 0;
int Fl_Help_View::selection_drag_last = 0;
int Fl_Help_View::selected = 0;
int Fl_Help_View::draw_mode = 0;
int Fl_Help_View::mouse_x = 0;
int Fl_Help_View::mouse_y = 0;
int Fl_Help_View::current_pos = 0;
Fl_Help_View *Fl_Help_View::current_view = 0L;
Fl_Color Fl_Help_View::hv_selection_color;
Fl_Color Fl_Help_View::hv_selection_text_color;

/*
 * Limitation: if a word contains &code; notations, we will calculate a wrong length.
 *
 * This function must be optimized for speed!
 */
void Fl_Help_View::hv_draw(const char *t, int x, int y)
{
  if (selected && current_view==this && current_pos<selection_last && current_pos>=selection_first) {
    Fl_Color c = fl_color();
    fl_color(hv_selection_color);
    int w = (int)fl_width(t);
    if (current_pos+(int)strlen(t)<selection_last) 
      w += (int)fl_width(' ');
    fl_rectf(x, y+fl_descent()-fl_height(), w, fl_height());
    fl_color(hv_selection_text_color);
    fl_draw(t, x, y);
    fl_color(c);
  } else {
    fl_draw(t, x, y);
  }
  if (draw_mode) {
    int w = (int)fl_width(t);
    if (mouse_x>=x && mouse_x<x+w) {
      if (mouse_y>=y-fl_height()+fl_descent()&&mouse_y<=y+fl_descent()) {
        int f = current_pos;
        int l = f+strlen(t); // use 'quote_char' to calculate the true length of the HTML string
        if (draw_mode==1) {
          selection_push_first = f;
          selection_push_last = l;
        } else {
          selection_drag_first = f;
          selection_drag_last = l;
        }
      }
    }
  }
}


//
// 'Fl_Help_View::add_block()' - Add a text block to the list.
//

Fl_Help_Block *					// O - Pointer to new block
Fl_Help_View::add_block(const char   *s,	// I - Pointer to start of block text
                	int           xx,	// I - X position of block
			int           yy,	// I - Y position of block
			int           ww,	// I - Right margin of block
			int           hh,	// I - Height of block
			unsigned char border)	// I - Draw border?
{
  Fl_Help_Block	*temp;				// New block


//  printf("add_block(s = %p, xx = %d, yy = %d, ww = %d, hh = %d, border = %d)\n",
//         s, xx, yy, ww, hh, border);

  if (nblocks_ >= ablocks_)
  {
    ablocks_ += 16;

    if (ablocks_ == 16)
      blocks_ = (Fl_Help_Block *)malloc(sizeof(Fl_Help_Block) * ablocks_);
    else
      blocks_ = (Fl_Help_Block *)realloc(blocks_, sizeof(Fl_Help_Block) * ablocks_);
  }

  temp = blocks_ + nblocks_;
  memset(temp, 0, sizeof(Fl_Help_Block));
  temp->start   = s;
  temp->end     = s;
  temp->x       = xx;
  temp->y       = yy;
  temp->w       = ww;
  temp->h       = hh;
  temp->border  = border;
  temp->bgcolor = bgcolor_;
  nblocks_ ++;

  return (temp);
}


//
// 'Fl_Help_View::add_link()' - Add a new link to the list.
//

void
Fl_Help_View::add_link(const char *n,	// I - Name of link
                      int        xx,	// I - X position of link
		      int        yy,	// I - Y position of link
		      int        ww,	// I - Width of link text
		      int        hh)	// I - Height of link text
{
  Fl_Help_Link	*temp;			// New link
  char		*target;		// Pointer to target name


  if (nlinks_ >= alinks_)
  {
    alinks_ += 16;

    if (alinks_ == 16)
      links_ = (Fl_Help_Link *)malloc(sizeof(Fl_Help_Link) * alinks_);
    else
      links_ = (Fl_Help_Link *)realloc(links_, sizeof(Fl_Help_Link) * alinks_);
  }

  temp = links_ + nlinks_;

  temp->x       = xx;
  temp->y       = yy;
  temp->w       = xx + ww;
  temp->h       = yy + hh;

  strlcpy(temp->filename, n, sizeof(temp->filename));

  if ((target = strrchr(temp->filename, '#')) != NULL)
  {
    *target++ = '\0';
    strlcpy(temp->name, target, sizeof(temp->name));
  }
  else
    temp->name[0] = '\0';

  nlinks_ ++;
}


//
// 'Fl_Help_View::add_target()' - Add a new target to the list.
//

void
Fl_Help_View::add_target(const char *n,	// I - Name of target
                	int        yy)	// I - Y position of target
{
  Fl_Help_Target	*temp;			// New target


  if (ntargets_ >= atargets_)
  {
    atargets_ += 16;

    if (atargets_ == 16)
      targets_ = (Fl_Help_Target *)malloc(sizeof(Fl_Help_Target) * atargets_);
    else
      targets_ = (Fl_Help_Target *)realloc(targets_, sizeof(Fl_Help_Target) * atargets_);
  }

  temp = targets_ + ntargets_;

  temp->y = yy;
  strlcpy(temp->name, n, sizeof(temp->name));

  ntargets_ ++;
}


//
// 'Fl_Help_View::compare_targets()' - Compare two targets.
//

int							// O - Result of comparison
Fl_Help_View::compare_targets(const Fl_Help_Target *t0,	// I - First target
                             const Fl_Help_Target *t1)	// I - Second target
{
  return (strcasecmp(t0->name, t1->name));
}


//
// 'Fl_Help_View::do_align()' - Compute the alignment for a line in a block.
//

int						// O - New line
Fl_Help_View::do_align(Fl_Help_Block *block,	// I - Block to add to
                      int          line,	// I - Current line
		      int          xx,		// I - Current X position
		      int          a,		// I - Current alignment
		      int          &l)		// IO - Starting link
{
  int	offset;					// Alignment offset


  switch (a)
  {
    case RIGHT :	// Right align
	offset = block->w - xx;
	break;
    case CENTER :	// Center
	offset = (block->w - xx) / 2;
	break;
    default :		// Left align
	offset = 0;
	break;
  }

  block->line[line] = block->x + offset;

  if (line < 31)
    line ++;

  while (l < nlinks_)
  {
    links_[l].x += offset;
    links_[l].w += offset;
    l ++;
  }

  return (line);
}


//
// 'Fl_Help_View::draw()' - Draw the Fl_Help_View widget.
//

void
Fl_Help_View::draw()
{
  int			i;		// Looping var
  const Fl_Help_Block	*block;		// Pointer to current block
  const char		*ptr,		// Pointer to text in block
			*attrs;		// Pointer to start of element attributes
  char			*s,		// Pointer into buffer
			buf[1024],	// Text buffer
			attr[1024];	// Attribute buffer
  int			xx, yy, ww, hh;	// Current positions and sizes
  int			line;		// Current line
  unsigned char		font, fsize;	// Current font and size
  int			head, pre,	// Flags for text
			needspace;	// Do we need whitespace?
  Fl_Boxtype		b = box() ? box() : FL_DOWN_BOX;
					// Box to draw...
  int			underline,	// Underline text?
                        xtra_ww;        // Extra width for underlined space between words


  // Draw the scrollbar(s) and box first...
  ww = w() ;
  hh = h();
  i  = 0;

  draw_box(b, x(), y(), ww, hh, bgcolor_);

  int ss = Fl::scrollbar_size();
  if (hscrollbar_.visible()) {
    draw_child(hscrollbar_);
    hh -= ss;
    i ++;
  }
  if (scrollbar_.visible()) {
    draw_child(scrollbar_);
    ww -= ss;
    i ++;
  }
  if (i == 2) {
    fl_color(FL_GRAY);
    fl_rectf(x() + ww - Fl::box_dw(b) + Fl::box_dx(b),
             y() + hh - Fl::box_dh(b) + Fl::box_dy(b), ss, ss);
  }

  if (!value_)
    return;

  if (current_view == this && selected) {
    hv_selection_color      = FL_SELECTION_COLOR;
    hv_selection_text_color = fl_contrast(textcolor_, FL_SELECTION_COLOR);
  }
  current_pos = 0;

  // Clip the drawing to the inside of the box...
  fl_push_clip(x() + Fl::box_dx(b), y() + Fl::box_dy(b),
               ww - Fl::box_dw(b), hh - Fl::box_dh(b));
  fl_color(textcolor_);

  // Draw all visible blocks...
  for (i = 0, block = blocks_; i < nblocks_; i ++, block ++)
    if ((block->y + block->h) >= topline_ && block->y < (topline_ + h()))
    {
      line      = 0;
      xx        = block->line[line];
      yy        = block->y - topline_;
      hh        = 0;
      pre       = 0;
      head      = 0;
      needspace = 0;
      underline = 0;

      initfont(font, fsize);

      for (ptr = block->start, s = buf; ptr < block->end;)
      {
	if ((*ptr == '<' || isspace((*ptr)&255)) && s > buf)
	{
	  if (!head && !pre)
	  {
            // Check width...
            *s = '\0';
            s  = buf;
            ww = (int)fl_width(buf);

            if (needspace && xx > block->x)
	      xx += (int)fl_width(' ');

            if ((xx + ww) > block->w)
	    {
	      if (line < 31)
	        line ++;
	      xx = block->line[line];
	      yy += hh;
	      hh = 0;
	    }

            hv_draw(buf, xx + x() - leftline_, yy + y());
	    if (underline) {
              xtra_ww = isspace((*ptr)&255)?(int)fl_width(' '):0;
              fl_xyline(xx + x() - leftline_, yy + y() + 1,
	                xx + x() - leftline_ + ww + xtra_ww);
            }
            current_pos = ptr-value_;

            xx += ww;
	    if ((fsize + 2) > hh)
	      hh = fsize + 2;

	    needspace = 0;
	  }
	  else if (pre)
	  {
	    while (isspace((*ptr)&255))
	    {
	      if (*ptr == '\n')
	      {
	        *s = '\0';
                s = buf;

                hv_draw(buf, xx + x() - leftline_, yy + y());
		if (underline) fl_xyline(xx + x() - leftline_, yy + y() + 1,
	                        	 xx + x() - leftline_ +
					     (int)fl_width(buf));

                current_pos = ptr-value_;
		if (line < 31)
	          line ++;
		xx = block->line[line];
		yy += hh;
		hh = fsize + 2;
	      }
	      else if (*ptr == '\t')
	      {
		// Do tabs every 8 columns...
		while (((s - buf) & 7))
	          *s++ = ' ';
	      }
	      else
	        *s++ = ' ';

              if ((fsize + 2) > hh)
	        hh = fsize + 2;

              ptr ++;
	    }

            if (s > buf)
	    {
	      *s = '\0';
	      s = buf;

              hv_draw(buf, xx + x() - leftline_, yy + y());
	      ww = (int)fl_width(buf);
	      if (underline) fl_xyline(xx + x() - leftline_, yy + y() + 1,
	                               xx + x() - leftline_ + ww);
              xx += ww;
              current_pos = ptr-value_;
	    }

	    needspace = 0;
	  }
	  else
	  {
            s = buf;

	    while (isspace((*ptr)&255))
              ptr ++;
            current_pos = ptr-value_;
	  }
	}

	if (*ptr == '<')
	{
	  ptr ++;

          if (strncmp(ptr, "!--", 3) == 0)
	  {
	    // Comment...
	    ptr += 3;
	    if ((ptr = strstr(ptr, "-->")) != NULL)
	    {
	      ptr += 3;
	      continue;
	    }
	    else
	      break;
	  }

	  while (*ptr && *ptr != '>' && !isspace((*ptr)&255))
            if (s < (buf + sizeof(buf) - 1))
	      *s++ = *ptr++;
	    else
	      ptr ++;

	  *s = '\0';
	  s = buf;

	  attrs = ptr;
	  while (*ptr && *ptr != '>')
            ptr ++;

	  if (*ptr == '>')
            ptr ++;

          // end of command reached, set the supposed start of printed eord here
          current_pos = ptr-value_;
	  if (strcasecmp(buf, "HEAD") == 0)
            head = 1;
	  else if (strcasecmp(buf, "BR") == 0)
	  {
	    if (line < 31)
	      line ++;
	    xx = block->line[line];
            yy += hh;
	    hh = 0;
	  }
	  else if (strcasecmp(buf, "HR") == 0)
	  {
	    fl_line(block->x + x(), yy + y(), block->w + x(),
	            yy + y());

	    if (line < 31)
	      line ++;
	    xx = block->line[line];
            yy += 2 * hh;
	    hh = 0;
	  }
	  else if (strcasecmp(buf, "CENTER") == 0 ||
        	   strcasecmp(buf, "P") == 0 ||
        	   strcasecmp(buf, "H1") == 0 ||
		   strcasecmp(buf, "H2") == 0 ||
		   strcasecmp(buf, "H3") == 0 ||
		   strcasecmp(buf, "H4") == 0 ||
		   strcasecmp(buf, "H5") == 0 ||
		   strcasecmp(buf, "H6") == 0 ||
		   strcasecmp(buf, "UL") == 0 ||
		   strcasecmp(buf, "OL") == 0 ||
		   strcasecmp(buf, "DL") == 0 ||
		   strcasecmp(buf, "LI") == 0 ||
		   strcasecmp(buf, "DD") == 0 ||
		   strcasecmp(buf, "DT") == 0 ||
		   strcasecmp(buf, "PRE") == 0)
	  {
            if (tolower(buf[0]) == 'h')
	    {
	      font  = FL_HELVETICA_BOLD;
	      fsize = (uchar)(textsize_ + '7' - buf[1]);
	    }
	    else if (strcasecmp(buf, "DT") == 0)
	    {
	      font  = (uchar)(textfont_ | FL_ITALIC);
	      fsize = textsize_;
	    }
	    else if (strcasecmp(buf, "PRE") == 0)
	    {
	      font  = FL_COURIER;
	      fsize = textsize_;
	      pre   = 1;
	    }

            if (strcasecmp(buf, "LI") == 0)
	    {
#ifdef __APPLE_QUARTZ__
              fl_font(FL_SYMBOL, fsize); 
              hv_draw("\245", xx - fsize + x() - leftline_, yy + y());
#else
              fl_font(FL_SYMBOL, fsize);
              hv_draw("\267", xx - fsize + x() - leftline_, yy + y());
#endif
	    }

	    pushfont(font, fsize);
	  }
	  else if (strcasecmp(buf, "A") == 0 &&
	           get_attr(attrs, "HREF", attr, sizeof(attr)) != NULL)
	  {
	    fl_color(linkcolor_);
	    underline = 1;
	  }
	  else if (strcasecmp(buf, "/A") == 0)
	  {
	    fl_color(textcolor_);
	    underline = 0;
	  }
	  else if (strcasecmp(buf, "FONT") == 0)
	  {
	    if (get_attr(attrs, "COLOR", attr, sizeof(attr)) != NULL) {
	      fl_color(get_color(attr, textcolor_));
	    }

            if (get_attr(attrs, "FACE", attr, sizeof(attr)) != NULL) {
	      if (!strncasecmp(attr, "helvetica", 9) ||
	          !strncasecmp(attr, "arial", 5) ||
		  !strncasecmp(attr, "sans", 4)) font = FL_HELVETICA;
              else if (!strncasecmp(attr, "times", 5) ||
	               !strncasecmp(attr, "serif", 5)) font = FL_TIMES;
              else if (!strncasecmp(attr, "symbol", 6)) font = FL_SYMBOL;
	      else font = FL_COURIER;
            }

            if (get_attr(attrs, "SIZE", attr, sizeof(attr)) != NULL) {
              if (isdigit(attr[0] & 255)) {
	        // Absolute size
	        fsize = (int)(textsize_ * pow(1.2, atof(attr) - 3.0));
	      } else {
	        // Relative size
	        fsize = (int)(fsize * pow(1.2, atof(attr) - 3.0));
	      }
	    }

            pushfont(font, fsize);
	  }
	  else if (strcasecmp(buf, "/FONT") == 0)
	  {
	    fl_color(textcolor_);
	    popfont(font, fsize);
	  }
	  else if (strcasecmp(buf, "U") == 0)
	    underline = 1;
	  else if (strcasecmp(buf, "/U") == 0)
	    underline = 0;
	  else if (strcasecmp(buf, "B") == 0 ||
	           strcasecmp(buf, "STRONG") == 0)
	    pushfont(font |= FL_BOLD, fsize);
	  else if (strcasecmp(buf, "TD") == 0 ||
	           strcasecmp(buf, "TH") == 0)
          {
	    int tx, ty, tw, th;

	    if (tolower(buf[1]) == 'h')
	      pushfont(font |= FL_BOLD, fsize);
	    else
	      pushfont(font = textfont_, fsize);

            tx = block->x - 4 - leftline_;
	    ty = block->y - topline_ - fsize - 3;
            tw = block->w - block->x + 7;
	    th = block->h + fsize - 5;

            if (tx < 0)
	    {
	      tw += tx;
	      tx  = 0;
	    }

	    if (ty < 0)
	    {
	      th += ty;
	      ty  = 0;
	    }

            tx += x();
	    ty += y();

            if (block->bgcolor != bgcolor_)
	    {
	      fl_color(block->bgcolor);
              fl_rectf(tx, ty, tw, th);
              fl_color(textcolor_);
	    }

            if (block->border)
              fl_rect(tx, ty, tw, th);
	  }
	  else if (strcasecmp(buf, "I") == 0 ||
                   strcasecmp(buf, "EM") == 0)
	    pushfont(font |= FL_ITALIC, fsize);
	  else if (strcasecmp(buf, "CODE") == 0 ||
	           strcasecmp(buf, "TT") == 0)
	    pushfont(font = FL_COURIER, fsize);
	  else if (strcasecmp(buf, "KBD") == 0)
	    pushfont(font = FL_COURIER_BOLD, fsize);
	  else if (strcasecmp(buf, "VAR") == 0)
	    pushfont(font = FL_COURIER_ITALIC, fsize);
	  else if (strcasecmp(buf, "/HEAD") == 0)
            head = 0;
	  else if (strcasecmp(buf, "/H1") == 0 ||
		   strcasecmp(buf, "/H2") == 0 ||
		   strcasecmp(buf, "/H3") == 0 ||
		   strcasecmp(buf, "/H4") == 0 ||
		   strcasecmp(buf, "/H5") == 0 ||
		   strcasecmp(buf, "/H6") == 0 ||
		   strcasecmp(buf, "/B") == 0 ||
		   strcasecmp(buf, "/STRONG") == 0 ||
		   strcasecmp(buf, "/I") == 0 ||
		   strcasecmp(buf, "/EM") == 0 ||
		   strcasecmp(buf, "/CODE") == 0 ||
		   strcasecmp(buf, "/TT") == 0 ||
		   strcasecmp(buf, "/KBD") == 0 ||
		   strcasecmp(buf, "/VAR") == 0)
	    popfont(font, fsize);
	  else if (strcasecmp(buf, "/PRE") == 0)
	  {
	    popfont(font, fsize);
	    pre = 0;
	  }
	  else if (strcasecmp(buf, "IMG") == 0)
	  {
	    Fl_Shared_Image *img = 0;
	    int		width, height;
	    char	wattr[8], hattr[8];


            get_attr(attrs, "WIDTH", wattr, sizeof(wattr));
            get_attr(attrs, "HEIGHT", hattr, sizeof(hattr));
	    width  = get_length(wattr);
	    height = get_length(hattr);

	    if (get_attr(attrs, "SRC", attr, sizeof(attr))) {
	      img = get_image(attr, width, height);
	      if (!width) width = img->w();
	      if (!height) height = img->h();
	    }

	    if (!width || !height) {
              if (get_attr(attrs, "ALT", attr, sizeof(attr)) == NULL) {
	        strcpy(attr, "IMG");
              }
	    }

	    ww = width;

	    if (needspace && xx > block->x)
	      xx += (int)fl_width(' ');

	    if ((xx + ww) > block->w)
	    {
	      if (line < 31)
		line ++;

	      xx = block->line[line];
	      yy += hh;
	      hh = 0;
	    }

	    if (img) {
	      img->draw(xx + x() - leftline_,
	                yy + y() - fl_height() + fl_descent() + 2);
	      img->release();
	    }

	    xx += ww;
	    if ((height + 2) > hh)
	      hh = height + 2;

	    needspace = 0;
	  }
	}
	else if (*ptr == '\n' && pre)
	{
	  *s = '\0';
	  s = buf;

          hv_draw(buf, xx + x() - leftline_, yy + y());

	  if (line < 31)
	    line ++;
	  xx = block->line[line];
	  yy += hh;
	  hh = fsize + 2;
	  needspace = 0;

	  ptr ++;
          current_pos = ptr-value_;
	}
	else if (isspace((*ptr)&255))
	{
	  if (pre)
	  {
	    if (*ptr == ' ')
	      *s++ = ' ';
	    else
	    {
	      // Do tabs every 8 columns...
	      while (((s - buf) & 7))
	        *s++ = ' ';
            }
	  }

          ptr ++;
          if (!pre) current_pos = ptr-value_;
	  needspace = 1;
	}
	else if (*ptr == '&')
	{
	  ptr ++;

          int qch = quote_char(ptr);

	  if (qch < 0)
	    *s++ = '&';
	  else {
	    *s++ = qch;
	    ptr = strchr(ptr, ';') + 1;
	  }

          if ((fsize + 2) > hh)
	    hh = fsize + 2;
	}
	else
	{
	  *s++ = *ptr++;

          if ((fsize + 2) > hh)
	    hh = fsize + 2;
        }
      }

      *s = '\0';

      if (s > buf && !pre && !head)
      {
	ww = (int)fl_width(buf);

        if (needspace && xx > block->x)
	  xx += (int)fl_width(' ');

	if ((xx + ww) > block->w)
	{
	  if (line < 31)
	    line ++;
	  xx = block->line[line];
	  yy += hh;
	  hh = 0;
	}
      }

      if (s > buf && !head)
      {
        hv_draw(buf, xx + x() - leftline_, yy + y());
	if (underline) fl_xyline(xx + x() - leftline_, yy + y() + 1,
	                         xx + x() - leftline_ + ww);
        current_pos = ptr-value_;
      }
    }

  fl_pop_clip();
}


//
// 'Fl_Help_View::find()' - Find the specified string...
//

int						// O - Matching position or -1 if not found
Fl_Help_View::find(const char *s,		// I - String to find
                   int        p)		// I - Starting position
{
  int		i,				// Looping var
		c;				// Current character
  Fl_Help_Block	*b;				// Current block
  const char	*bp,				// Block matching pointer
		*bs,				// Start of current comparison
		*sp;				// Search string pointer


  // Range check input and value...
  if (!s || !value_) return -1;

  if (p < 0 || p >= (int)strlen(value_)) p = 0;
  else if (p > 0) p ++;

  // Look for the string...
  for (i = nblocks_, b = blocks_; i > 0; i --, b ++) {
    if (b->end < (value_ + p))
      continue;

    if (b->start < (value_ + p)) bp = value_ + p;
    else bp = b->start;

    for (sp = s, bs = bp; *sp && *bp && bp < b->end; bp ++) {
      if (*bp == '<') {
        // skip to end of element...
	while (*bp && bp < b->end && *bp != '>') bp ++;
	continue;
      } else if (*bp == '&') {
        // decode HTML entity...
	if ((c = quote_char(bp + 1)) < 0) c = '&';
	else bp = strchr(bp + 1, ';') + 1;
      } else c = *bp;

      if (tolower(*sp) == tolower(c)) sp ++;
      else {
        // No match, so reset to start of search...
	sp = s;
	bs ++;
	bp = bs;
      }
    }

    if (!*sp) {
      // Found a match!
      topline(b->y - b->h);
      return (b->end - value_);
    }
  }

  // No match!
  return (-1);
}


//
// 'Fl_Help_View::format()' - Format the help text.
//

void
Fl_Help_View::format()
{
  int		i;		// Looping var
  int		done;		// Are we done yet?
  Fl_Help_Block	*block,		// Current block
		*cell;		// Current table cell
  int		cells[MAX_COLUMNS],
				// Cells in the current row...
		row;		// Current table row (block number)
  const char	*ptr,		// Pointer into block
		*start,		// Pointer to start of element
		*attrs;		// Pointer to start of element attributes
  char		*s,		// Pointer into buffer
		buf[1024],	// Text buffer
		attr[1024],	// Attribute buffer
		wattr[1024],	// Width attribute buffer
		hattr[1024],	// Height attribute buffer
		linkdest[1024];	// Link destination
  int		xx, yy, ww, hh;	// Size of current text fragment
  int		line;		// Current line in block
  int		links;		// Links for current line
  unsigned char	font, fsize;	// Current font and size
  unsigned char	border;		// Draw border?
  int		talign,		// Current alignment
		newalign,	// New alignment
		head,		// In the <HEAD> section?
		pre,		// <PRE> text?
		needspace;	// Do we need whitespace?
  int		table_width,	// Width of table
		table_offset;	// Offset of table
  int		column,		// Current table column number
		columns[MAX_COLUMNS];
				// Column widths
  Fl_Color	tc, rc;		// Table/row background color
  Fl_Boxtype	b = box() ? box() : FL_DOWN_BOX;
				// Box to draw...
  fl_margins	margins;	// Left margin stack...


  // Reset document width...
  hsize_ = w() - Fl::scrollbar_size() - Fl::box_dw(b);

  done = 0;
  while (!done)
  {
    // Reset state variables...
    done       = 1;
    nblocks_   = 0;
    nlinks_    = 0;
    ntargets_  = 0;
    size_      = 0;
    bgcolor_   = color();
    textcolor_ = textcolor();
    linkcolor_ = fl_contrast(FL_BLUE, color());

    tc = rc = bgcolor_;

    strcpy(title_, "Untitled");

    if (!value_)
      return;

    // Setup for formatting...
    initfont(font, fsize);

    line         = 0;
    links        = 0;
    xx           = margins.clear();
    yy           = fsize + 2;
    ww           = 0;
    column       = 0;
    border       = 0;
    hh           = 0;
    block        = add_block(value_, xx, yy, hsize_, 0);
    row          = 0;
    head         = 0;
    pre          = 0;
    talign       = LEFT;
    newalign     = LEFT;
    needspace    = 0;
    linkdest[0]  = '\0';
    table_offset = 0;

    for (ptr = value_, s = buf; *ptr;)
    {
      if ((*ptr == '<' || isspace((*ptr)&255)) && s > buf)
      {
        // Get width...
        *s = '\0';
        ww = (int)fl_width(buf);

	if (!head && !pre)
	{
          // Check width...
          if (ww > hsize_) {
	    hsize_ = ww;
	    done   = 0;
	    break;
	  }

          if (needspace && xx > block->x)
	    ww += (int)fl_width(' ');

  //        printf("line = %d, xx = %d, ww = %d, block->x = %d, block->w = %d\n",
  //	       line, xx, ww, block->x, block->w);

          if ((xx + ww) > block->w)
	  {
            line     = do_align(block, line, xx, newalign, links);
	    xx       = block->x;
	    yy       += hh;
	    block->h += hh;
	    hh       = 0;
	  }

          if (linkdest[0])
	    add_link(linkdest, xx, yy - fsize, ww, fsize);

	  xx += ww;
	  if ((fsize + 2) > hh)
	    hh = fsize + 2;

	  needspace = 0;
	}
	else if (pre)
	{
          // Add a link as needed...
          if (linkdest[0])
	    add_link(linkdest, xx, yy - hh, ww, hh);

	  xx += ww;
	  if ((fsize + 2) > hh)
	    hh = fsize + 2;

          // Handle preformatted text...
	  while (isspace((*ptr)&255))
	  {
	    if (*ptr == '\n')
	    {
              if (xx > hsize_) break;

              line     = do_align(block, line, xx, newalign, links);
              xx       = block->x;
	      yy       += hh;
	      block->h += hh;
	      hh       = fsize + 2;
	    }
	    else
              xx += (int)fl_width(' ');

            if ((fsize + 2) > hh)
	      hh = fsize + 2;

            ptr ++;
	  }

          if (xx > hsize_) {
	    hsize_ = xx;
	    done   = 0;
	    break;
	  }

	  needspace = 0;
	}
	else
	{
          // Handle normal text or stuff in the <HEAD> section...
	  while (isspace((*ptr)&255))
            ptr ++;
	}

	s = buf;
      }

      if (*ptr == '<')
      {
	start = ptr;
	ptr ++;

        if (strncmp(ptr, "!--", 3) == 0)
	{
	  // Comment...
	  ptr += 3;
	  if ((ptr = strstr(ptr, "-->")) != NULL)
	  {
	    ptr += 3;
	    continue;
	  }
	  else
	    break;
	}

	while (*ptr && *ptr != '>' && !isspace((*ptr)&255))
          if (s < (buf + sizeof(buf) - 1))
            *s++ = *ptr++;
	  else
	    ptr ++;

	*s = '\0';
	s = buf;

//        puts(buf);

	attrs = ptr;
	while (*ptr && *ptr != '>')
          ptr ++;

	if (*ptr == '>')
          ptr ++;

	if (strcasecmp(buf, "HEAD") == 0)
          head = 1;
	else if (strcasecmp(buf, "/HEAD") == 0)
          head = 0;
	else if (strcasecmp(buf, "TITLE") == 0)
	{
          // Copy the title in the document...
          for (s = title_;
	       *ptr != '<' && *ptr && s < (title_ + sizeof(title_) - 1);
	       *s++ = *ptr++);

	  *s = '\0';
	  s = buf;
	}
	else if (strcasecmp(buf, "A") == 0)
	{
          if (get_attr(attrs, "NAME", attr, sizeof(attr)) != NULL)
	    add_target(attr, yy - fsize - 2);

	  if (get_attr(attrs, "HREF", attr, sizeof(attr)) != NULL)
	    strlcpy(linkdest, attr, sizeof(linkdest));
	}
	else if (strcasecmp(buf, "/A") == 0)
          linkdest[0] = '\0';
	else if (strcasecmp(buf, "BODY") == 0)
	{
          bgcolor_   = get_color(get_attr(attrs, "BGCOLOR", attr, sizeof(attr)),
	                	 color());
          textcolor_ = get_color(get_attr(attrs, "TEXT", attr, sizeof(attr)),
	                	 textcolor());
          linkcolor_ = get_color(get_attr(attrs, "LINK", attr, sizeof(attr)),
	                	 fl_contrast(FL_BLUE, color()));
	}
	else if (strcasecmp(buf, "BR") == 0)
	{
          line     = do_align(block, line, xx, newalign, links);
          xx       = block->x;
	  block->h += hh;
          yy       += hh;
	  hh       = 0;
	}
	else if (strcasecmp(buf, "CENTER") == 0 ||
        	 strcasecmp(buf, "P") == 0 ||
        	 strcasecmp(buf, "H1") == 0 ||
		 strcasecmp(buf, "H2") == 0 ||
		 strcasecmp(buf, "H3") == 0 ||
		 strcasecmp(buf, "H4") == 0 ||
		 strcasecmp(buf, "H5") == 0 ||
		 strcasecmp(buf, "H6") == 0 ||
		 strcasecmp(buf, "UL") == 0 ||
		 strcasecmp(buf, "OL") == 0 ||
		 strcasecmp(buf, "DL") == 0 ||
		 strcasecmp(buf, "LI") == 0 ||
		 strcasecmp(buf, "DD") == 0 ||
		 strcasecmp(buf, "DT") == 0 ||
		 strcasecmp(buf, "HR") == 0 ||
		 strcasecmp(buf, "PRE") == 0 ||
		 strcasecmp(buf, "TABLE") == 0)
	{
          block->end = start;
          line       = do_align(block, line, xx, newalign, links);
	  newalign   = strcasecmp(buf, "CENTER") ? LEFT : CENTER;
          xx         = block->x;
          block->h   += hh;

          if (strcasecmp(buf, "UL") == 0 ||
	      strcasecmp(buf, "OL") == 0 ||
	      strcasecmp(buf, "DL") == 0)
          {
	    block->h += fsize + 2;
	    xx       = margins.push(4 * fsize);
	  }
          else if (strcasecmp(buf, "TABLE") == 0)
	  {
	    if (get_attr(attrs, "BORDER", attr, sizeof(attr)))
	      border = (uchar)atoi(attr);
	    else
	      border = 0;

            tc = rc = get_color(get_attr(attrs, "BGCOLOR", attr, sizeof(attr)), bgcolor_);

	    block->h += fsize + 2;

            format_table(&table_width, columns, start);

            if ((xx + table_width) > hsize_) {
#ifdef DEBUG
              printf("xx=%d, table_width=%d, hsize_=%d\n", xx, table_width,
	             hsize_);
#endif // DEBUG
	      hsize_ = xx + table_width;
	      done   = 0;
	      break;
	    }

            switch (get_align(attrs, talign))
	    {
	      default :
	          table_offset = 0;
	          break;

	      case CENTER :
	          table_offset = (hsize_ - table_width) / 2 - textsize_;
	          break;

	      case RIGHT :
	          table_offset = hsize_ - table_width - textsize_;
	          break;
	    }

	    column = 0;
	  }

          if (tolower(buf[0]) == 'h' && isdigit(buf[1]))
	  {
	    font  = FL_HELVETICA_BOLD;
	    fsize = (uchar)(textsize_ + '7' - buf[1]);
	  }
	  else if (strcasecmp(buf, "DT") == 0)
	  {
	    font  = (uchar)(textfont_ | FL_ITALIC);
	    fsize = textsize_;
	  }
	  else if (strcasecmp(buf, "PRE") == 0)
	  {
	    font  = FL_COURIER;
	    fsize = textsize_;
	    pre   = 1;
	  }
	  else
	  {
	    font  = textfont_;
	    fsize = textsize_;
	  }

	  pushfont(font, fsize);

          yy = block->y + block->h;
          hh = 0;

          if ((tolower(buf[0]) == 'h' && isdigit(buf[1])) ||
	      strcasecmp(buf, "DD") == 0 ||
	      strcasecmp(buf, "DT") == 0 ||
	      strcasecmp(buf, "P") == 0)
            yy += fsize + 2;
	  else if (strcasecmp(buf, "HR") == 0)
	  {
	    hh += 2 * fsize;
	    yy += fsize;
	  }

          if (row)
	    block = add_block(start, xx, yy, block->w, 0);
	  else
	    block = add_block(start, xx, yy, hsize_, 0);

	  needspace = 0;
	  line      = 0;

	  if (strcasecmp(buf, "CENTER") == 0)
	    newalign = talign = CENTER;
	  else
	    newalign = get_align(attrs, talign);
	}
	else if (strcasecmp(buf, "/CENTER") == 0 ||
		 strcasecmp(buf, "/P") == 0 ||
		 strcasecmp(buf, "/H1") == 0 ||
		 strcasecmp(buf, "/H2") == 0 ||
		 strcasecmp(buf, "/H3") == 0 ||
		 strcasecmp(buf, "/H4") == 0 ||
		 strcasecmp(buf, "/H5") == 0 ||
		 strcasecmp(buf, "/H6") == 0 ||
		 strcasecmp(buf, "/PRE") == 0 ||
		 strcasecmp(buf, "/UL") == 0 ||
		 strcasecmp(buf, "/OL") == 0 ||
		 strcasecmp(buf, "/DL") == 0 ||
		 strcasecmp(buf, "/TABLE") == 0)
	{
          line       = do_align(block, line, xx, newalign, links);
          xx         = block->x;
          block->end = ptr;

          if (strcasecmp(buf, "/UL") == 0 ||
	      strcasecmp(buf, "/OL") == 0 ||
	      strcasecmp(buf, "/DL") == 0)
	  {
	    xx       = margins.pop();
	    block->h += fsize + 2;
	  }
          else if (strcasecmp(buf, "/TABLE") == 0) 
          {
	    block->h += fsize + 2;
            xx       = margins.current();
          }
	  else if (strcasecmp(buf, "/PRE") == 0)
	  {
	    pre = 0;
	    hh  = 0;
	  }
	  else if (strcasecmp(buf, "/CENTER") == 0)
	    talign = LEFT;

          popfont(font, fsize);

          while (isspace((*ptr)&255))
	    ptr ++;

          block->h += hh;
          yy       += hh;

          if (tolower(buf[2]) == 'l')
            yy += fsize + 2;

          if (row)
	    block = add_block(ptr, xx, yy, block->w, 0);
	  else
	    block = add_block(ptr, xx, yy, hsize_, 0);

	  needspace = 0;
	  hh        = 0;
	  line      = 0;
	  newalign  = talign;
	}
	else if (strcasecmp(buf, "TR") == 0)
	{
          block->end = start;
          line       = do_align(block, line, xx, newalign, links);
          xx         = block->x;
          block->h   += hh;

          if (row)
	  {
            yy = blocks_[row].y + blocks_[row].h;

	    for (cell = blocks_ + row + 1; cell <= block; cell ++)
	      if ((cell->y + cell->h) > yy)
		yy = cell->y + cell->h;

            block = blocks_ + row;

            block->h = yy - block->y + 2;

	    for (i = 0; i < column; i ++)
	      if (cells[i])
	      {
		cell = blocks_ + cells[i];
		cell->h = block->h;
	      }
	  }

          memset(cells, 0, sizeof(cells));

	  yy        = block->y + block->h - 4;
	  hh        = 0;
          block     = add_block(start, xx, yy, hsize_, 0);
	  row       = block - blocks_;
	  needspace = 0;
	  column    = 0;
	  line      = 0;

          rc = get_color(get_attr(attrs, "BGCOLOR", attr, sizeof(attr)), tc);
	}
	else if (strcasecmp(buf, "/TR") == 0 && row)
	{
          line       = do_align(block, line, xx, newalign, links);
          block->end = start;
	  block->h   += hh;
	  talign     = LEFT;

          xx = blocks_[row].x;
          yy = blocks_[row].y + blocks_[row].h;

	  for (cell = blocks_ + row + 1; cell <= block; cell ++)
	    if ((cell->y + cell->h) > yy)
	      yy = cell->y + cell->h;

          block = blocks_ + row;

          block->h = yy - block->y + 2;

	  for (i = 0; i < column; i ++)
	    if (cells[i])
	    {
	      cell = blocks_ + cells[i];
	      cell->h = block->h;
	    }

	  yy        = block->y + block->h /*- 4*/;
          block     = add_block(start, xx, yy, hsize_, 0);
	  needspace = 0;
	  row       = 0;
	  line      = 0;
	}
	else if ((strcasecmp(buf, "TD") == 0 ||
                  strcasecmp(buf, "TH") == 0) && row)
	{
          int	colspan;		// COLSPAN attribute


          line       = do_align(block, line, xx, newalign, links);
          block->end = start;
	  block->h   += hh;

          if (strcasecmp(buf, "TH") == 0)
	    font = (uchar)(textfont_ | FL_BOLD);
	  else
	    font = textfont_;

          fsize = textsize_;

          xx = blocks_[row].x + fsize + 3 + table_offset;
	  for (i = 0; i < column; i ++)
	    xx += columns[i] + 6;

          margins.push(xx - margins.current());

          if (get_attr(attrs, "COLSPAN", attr, sizeof(attr)) != NULL)
	    colspan = atoi(attr);
	  else
	    colspan = 1;

          for (i = 0, ww = -6; i < colspan; i ++)
	    ww += columns[column + i] + 6;

          if (block->end == block->start && nblocks_ > 1)
	  {
	    nblocks_ --;
	    block --;
	  }

	  pushfont(font, fsize);

	  yy        = blocks_[row].y;
	  hh        = 0;
          block     = add_block(start, xx, yy, xx + ww, 0, border);
	  needspace = 0;
	  line      = 0;
	  newalign  = get_align(attrs, tolower(buf[1]) == 'h' ? CENTER : LEFT);
	  talign    = newalign;

          cells[column] = block - blocks_;

	  column += colspan;

          block->bgcolor = get_color(get_attr(attrs, "BGCOLOR", attr,
	                                      sizeof(attr)), rc);
	}
	else if ((strcasecmp(buf, "/TD") == 0 ||
                  strcasecmp(buf, "/TH") == 0) && row)
	{
          line = do_align(block, line, xx, newalign, links);
          popfont(font, fsize);
	  xx = margins.pop();
	  talign = LEFT;
	}
	else if (strcasecmp(buf, "FONT") == 0)
	{
          if (get_attr(attrs, "FACE", attr, sizeof(attr)) != NULL) {
	    if (!strncasecmp(attr, "helvetica", 9) ||
	        !strncasecmp(attr, "arial", 5) ||
		!strncasecmp(attr, "sans", 4)) font = FL_HELVETICA;
            else if (!strncasecmp(attr, "times", 5) ||
	             !strncasecmp(attr, "serif", 5)) font = FL_TIMES;
            else if (!strncasecmp(attr, "symbol", 6)) font = FL_SYMBOL;
	    else font = FL_COURIER;
          }

          if (get_attr(attrs, "SIZE", attr, sizeof(attr)) != NULL) {
            if (isdigit(attr[0] & 255)) {
	      // Absolute size
	      fsize = (int)(textsize_ * pow(1.2, atoi(attr) - 3.0));
	    } else {
	      // Relative size
	      fsize = (int)(fsize * pow(1.2, atoi(attr)));
	    }
	  }

          pushfont(font, fsize);
	}
	else if (strcasecmp(buf, "/FONT") == 0)
	  popfont(font, fsize);
	else if (strcasecmp(buf, "B") == 0 ||
        	 strcasecmp(buf, "STRONG") == 0)
	  pushfont(font |= FL_BOLD, fsize);
	else if (strcasecmp(buf, "I") == 0 ||
        	 strcasecmp(buf, "EM") == 0)
	  pushfont(font |= FL_ITALIC, fsize);
	else if (strcasecmp(buf, "CODE") == 0 ||
	         strcasecmp(buf, "TT") == 0)
	  pushfont(font = FL_COURIER, fsize);
	else if (strcasecmp(buf, "KBD") == 0)
	  pushfont(font = FL_COURIER_BOLD, fsize);
	else if (strcasecmp(buf, "VAR") == 0)
	  pushfont(font = FL_COURIER_ITALIC, fsize);
	else if (strcasecmp(buf, "/B") == 0 ||
		 strcasecmp(buf, "/STRONG") == 0 ||
		 strcasecmp(buf, "/I") == 0 ||
		 strcasecmp(buf, "/EM") == 0 ||
		 strcasecmp(buf, "/CODE") == 0 ||
		 strcasecmp(buf, "/TT") == 0 ||
		 strcasecmp(buf, "/KBD") == 0 ||
		 strcasecmp(buf, "/VAR") == 0)
	  popfont(font, fsize);
	else if (strcasecmp(buf, "IMG") == 0)
	{
	  Fl_Shared_Image	*img = 0;
	  int		width;
	  int		height;


          get_attr(attrs, "WIDTH", wattr, sizeof(wattr));
          get_attr(attrs, "HEIGHT", hattr, sizeof(hattr));
	  width  = get_length(wattr);
	  height = get_length(hattr);

	  if (get_attr(attrs, "SRC", attr, sizeof(attr))) {
	    img    = get_image(attr, width, height);
	    width  = img->w();
	    height = img->h();
	  }

	  ww = width;

          if (ww > hsize_) {
	    hsize_ = ww;
	    done   = 0;
	    break;
	  }

	  if (needspace && xx > block->x)
	    ww += (int)fl_width(' ');

	  if ((xx + ww) > block->w)
	  {
	    line     = do_align(block, line, xx, newalign, links);
	    xx       = block->x;
	    yy       += hh;
	    block->h += hh;
	    hh       = 0;
	  }

	  if (linkdest[0])
	    add_link(linkdest, xx, yy - height, ww, height);

	  xx += ww;
	  if ((height + 2) > hh)
	    hh = height + 2;

	  needspace = 0;
	}
      }
      else if (*ptr == '\n' && pre)
      {
	if (linkdest[0])
	  add_link(linkdest, xx, yy - hh, ww, hh);

        if (xx > hsize_) {
	  hsize_ = xx;
          done   = 0;
	  break;
	}

	line      = do_align(block, line, xx, newalign, links);
	xx        = block->x;
	yy        += hh;
	block->h  += hh;
	needspace = 0;
	ptr ++;
      }
      else if (isspace((*ptr)&255))
      {
	needspace = 1;

	ptr ++;
      }
      else if (*ptr == '&' && s < (buf + sizeof(buf) - 1))
      {
	ptr ++;

        int qch = quote_char(ptr);

	if (qch < 0)
	  *s++ = '&';
	else {
	  *s++ = qch;
	  ptr = strchr(ptr, ';') + 1;
	}

	if ((fsize + 2) > hh)
          hh = fsize + 2;
      }
      else
      {
	if (s < (buf + sizeof(buf) - 1))
          *s++ = *ptr++;
	else
          ptr ++;

	if ((fsize + 2) > hh)
          hh = fsize + 2;
      }
    }

    if (s > buf && !head)
    {
      *s = '\0';
      ww = (int)fl_width(buf);

  //    printf("line = %d, xx = %d, ww = %d, block->x = %d, block->w = %d\n",
  //	   line, xx, ww, block->x, block->w);

      if (ww > hsize_) {
	hsize_ = ww;
	done   = 0;
	break;
      }

      if (needspace && xx > block->x)
	ww += (int)fl_width(' ');

      if ((xx + ww) > block->w)
      {
	line     = do_align(block, line, xx, newalign, links);
	xx       = block->x;
	yy       += hh;
	block->h += hh;
	hh       = 0;
      }

      if (linkdest[0])
	add_link(linkdest, xx, yy - fsize, ww, fsize);

      xx += ww;
    }

    do_align(block, line, xx, newalign, links);

    block->end = ptr;
    size_      = yy + hh;
  }

//  printf("margins.depth_=%d\n", margins.depth_);

  if (ntargets_ > 1)
    qsort(targets_, ntargets_, sizeof(Fl_Help_Target),
          (compare_func_t)compare_targets);

  int dx = Fl::box_dw(b) - Fl::box_dx(b);
  int dy = Fl::box_dh(b) - Fl::box_dy(b);
  int ss = Fl::scrollbar_size();
  int dw = Fl::box_dw(b) + ss;
  int dh = Fl::box_dh(b);

  if (hsize_ > (w() - dw)) {
    hscrollbar_.show();

    dh += ss;

    if (size_ < (h() - dh)) {
      scrollbar_.hide();
      hscrollbar_.resize(x() + Fl::box_dx(b), y() + h() - ss - dy,
                         w() - Fl::box_dw(b), ss);
    } else {
      scrollbar_.show();
      scrollbar_.resize(x() + w() - ss - dx, y() + Fl::box_dy(b),
                        ss, h() - ss - Fl::box_dh(b));
      hscrollbar_.resize(x() + Fl::box_dx(b), y() + h() - ss - dy,
                         w() - ss - Fl::box_dw(b), ss);
    }
  } else {
    hscrollbar_.hide();

    if (size_ < (h() - dh)) scrollbar_.hide();
    else {
      scrollbar_.resize(x() + w() - ss - dx, y() + Fl::box_dy(b),
                        ss, h() - Fl::box_dh(b));
      scrollbar_.show();
    }
  }

  // Reset scrolling if it needs to be...
  if (scrollbar_.visible()) {
    int temph = h() - Fl::box_dh(b);
    if (hscrollbar_.visible()) temph -= ss;
    if ((topline_ + temph) > size_) topline(size_ - temph);
    else topline(topline_);
  } else topline(0);

  if (hscrollbar_.visible()) {
    int tempw = w() - ss - Fl::box_dw(b);
    if ((leftline_ + tempw) > hsize_) leftline(hsize_ - tempw);
    else leftline(leftline_);
  } else leftline(0);
}


//
// 'Fl_Help_View::format_table()' - Format a table...
//

void
Fl_Help_View::format_table(int        *table_width,	// O - Total table width
                           int        *columns,		// O - Column widths
	                   const char *table)		// I - Pointer to start of table
{
  int		column,					// Current column
		num_columns,				// Number of columns
		colspan,				// COLSPAN attribute
		width,					// Current width
		temp_width,				// Temporary width
		max_width,				// Maximum width
		incell,					// In a table cell?
		pre,					// <PRE> text?
		needspace;				// Need whitespace?
  char		*s,					// Pointer into buffer
		buf[1024],				// Text buffer
		attr[1024],				// Other attribute
		wattr[1024],				// WIDTH attribute
		hattr[1024];				// HEIGHT attribute
  const char	*ptr,					// Pointer into table
		*attrs,					// Pointer to attributes
		*start;					// Start of element
  int		minwidths[MAX_COLUMNS];			// Minimum widths for each column
  unsigned char	font, fsize;				// Current font and size


  // Clear widths...
  *table_width = 0;
  for (column = 0; column < MAX_COLUMNS; column ++)
  {
    columns[column]   = 0;
    minwidths[column] = 0;
  }

  num_columns = 0;
  colspan     = 0;
  max_width   = 0;
  pre         = 0;
  needspace   = 0;
  font        = fonts_[nfonts_][0];
  fsize       = fonts_[nfonts_][1];

  // Scan the table...
  for (ptr = table, column = -1, width = 0, s = buf, incell = 0; *ptr;)
  {
    if ((*ptr == '<' || isspace((*ptr)&255)) && s > buf && incell)
    {
      // Check width...
      if (needspace)
      {
        *s++      = ' ';
	needspace = 0;
      }

      *s         = '\0';
      temp_width = (int)fl_width(buf);
      s          = buf;

      if (temp_width > minwidths[column])
        minwidths[column] = temp_width;

      width += temp_width;

      if (width > max_width)
        max_width = width;
    }

    if (*ptr == '<')
    {
      start = ptr;

      for (s = buf, ptr ++; *ptr && *ptr != '>' && !isspace((*ptr)&255);)
        if (s < (buf + sizeof(buf) - 1))
          *s++ = *ptr++;
	else
	  ptr ++;

      *s = '\0';
      s = buf;

      attrs = ptr;
      while (*ptr && *ptr != '>')
        ptr ++;

      if (*ptr == '>')
        ptr ++;

      if (strcasecmp(buf, "BR") == 0 ||
	  strcasecmp(buf, "HR") == 0)
      {
        width     = 0;
	needspace = 0;
      }
      else if (strcasecmp(buf, "TABLE") == 0 && start > table)
        break;
      else if (strcasecmp(buf, "CENTER") == 0 ||
               strcasecmp(buf, "P") == 0 ||
               strcasecmp(buf, "H1") == 0 ||
	       strcasecmp(buf, "H2") == 0 ||
	       strcasecmp(buf, "H3") == 0 ||
	       strcasecmp(buf, "H4") == 0 ||
	       strcasecmp(buf, "H5") == 0 ||
	       strcasecmp(buf, "H6") == 0 ||
	       strcasecmp(buf, "UL") == 0 ||
	       strcasecmp(buf, "OL") == 0 ||
	       strcasecmp(buf, "DL") == 0 ||
	       strcasecmp(buf, "LI") == 0 ||
	       strcasecmp(buf, "DD") == 0 ||
	       strcasecmp(buf, "DT") == 0 ||
	       strcasecmp(buf, "PRE") == 0)
      {
        width     = 0;
	needspace = 0;

        if (tolower(buf[0]) == 'h' && isdigit(buf[1]))
	{
	  font  = FL_HELVETICA_BOLD;
	  fsize = (uchar)(textsize_ + '7' - buf[1]);
	}
	else if (strcasecmp(buf, "DT") == 0)
	{
	  font  = (uchar)(textfont_ | FL_ITALIC);
	  fsize = textsize_;
	}
	else if (strcasecmp(buf, "PRE") == 0)
	{
	  font  = FL_COURIER;
	  fsize = textsize_;
	  pre   = 1;
	}
	else if (strcasecmp(buf, "LI") == 0)
	{
	  width  += 4 * fsize;
	  font   = textfont_;
	  fsize  = textsize_;
	}
	else
	{
	  font  = textfont_;
	  fsize = textsize_;
	}

	pushfont(font, fsize);
      }
      else if (strcasecmp(buf, "/CENTER") == 0 ||
	       strcasecmp(buf, "/P") == 0 ||
	       strcasecmp(buf, "/H1") == 0 ||
	       strcasecmp(buf, "/H2") == 0 ||
	       strcasecmp(buf, "/H3") == 0 ||
	       strcasecmp(buf, "/H4") == 0 ||
	       strcasecmp(buf, "/H5") == 0 ||
	       strcasecmp(buf, "/H6") == 0 ||
	       strcasecmp(buf, "/PRE") == 0 ||
	       strcasecmp(buf, "/UL") == 0 ||
	       strcasecmp(buf, "/OL") == 0 ||
	       strcasecmp(buf, "/DL") == 0)
      {
        width     = 0;
	needspace = 0;

        popfont(font, fsize);
      }
      else if (strcasecmp(buf, "TR") == 0 || strcasecmp(buf, "/TR") == 0 ||
               strcasecmp(buf, "/TABLE") == 0)
      {
//        printf("%s column = %d, colspan = %d, num_columns = %d\n",
//	       buf, column, colspan, num_columns);

        if (column >= 0)
	{
	  // This is a hack to support COLSPAN...
	  max_width /= colspan;

	  while (colspan > 0)
	  {
	    if (max_width > columns[column])
	      columns[column] = max_width;

	    column ++;
	    colspan --;
	  }
	}

	if (strcasecmp(buf, "/TABLE") == 0)
	  break;

	needspace = 0;
	column    = -1;
	width     = 0;
	max_width = 0;
	incell    = 0;
      }
      else if (strcasecmp(buf, "TD") == 0 ||
               strcasecmp(buf, "TH") == 0)
      {
//        printf("BEFORE column = %d, colspan = %d, num_columns = %d\n",
//	       column, colspan, num_columns);

        if (column >= 0)
	{
	  // This is a hack to support COLSPAN...
	  max_width /= colspan;

	  while (colspan > 0)
	  {
	    if (max_width > columns[column])
	      columns[column] = max_width;

	    column ++;
	    colspan --;
	  }
	}
	else
	  column ++;

        if (get_attr(attrs, "COLSPAN", attr, sizeof(attr)) != NULL)
	  colspan = atoi(attr);
	else
	  colspan = 1;

//        printf("AFTER column = %d, colspan = %d, num_columns = %d\n",
//	       column, colspan, num_columns);

        if ((column + colspan) >= num_columns)
	  num_columns = column + colspan;

	needspace = 0;
	width     = 0;
	incell    = 1;

        if (strcasecmp(buf, "TH") == 0)
	  font = (uchar)(textfont_ | FL_BOLD);
	else
	  font = textfont_;

        fsize = textsize_;

	pushfont(font, fsize);

        if (get_attr(attrs, "WIDTH", attr, sizeof(attr)) != NULL)
	  max_width = get_length(attr);
	else
	  max_width = 0;

//        printf("max_width = %d\n", max_width);
      }
      else if (strcasecmp(buf, "/TD") == 0 ||
               strcasecmp(buf, "/TH") == 0)
      {
	incell = 0;
        popfont(font, fsize);
      }
      else if (strcasecmp(buf, "B") == 0 ||
               strcasecmp(buf, "STRONG") == 0)
	pushfont(font |= FL_BOLD, fsize);
      else if (strcasecmp(buf, "I") == 0 ||
               strcasecmp(buf, "EM") == 0)
	pushfont(font |= FL_ITALIC, fsize);
      else if (strcasecmp(buf, "CODE") == 0 ||
               strcasecmp(buf, "TT") == 0)
	pushfont(font = FL_COURIER, fsize);
      else if (strcasecmp(buf, "KBD") == 0)
	pushfont(font = FL_COURIER_BOLD, fsize);
      else if (strcasecmp(buf, "VAR") == 0)
	pushfont(font = FL_COURIER_ITALIC, fsize);
      else if (strcasecmp(buf, "/B") == 0 ||
	       strcasecmp(buf, "/STRONG") == 0 ||
	       strcasecmp(buf, "/I") == 0 ||
	       strcasecmp(buf, "/EM") == 0 ||
	       strcasecmp(buf, "/CODE") == 0 ||
	       strcasecmp(buf, "/TT") == 0 ||
	       strcasecmp(buf, "/KBD") == 0 ||
	       strcasecmp(buf, "/VAR") == 0)
	popfont(font, fsize);
      else if (strcasecmp(buf, "IMG") == 0 && incell)
      {
	Fl_Shared_Image	*img = 0;
	int		iwidth, iheight;


        get_attr(attrs, "WIDTH", wattr, sizeof(wattr));
        get_attr(attrs, "HEIGHT", hattr, sizeof(hattr));
	iwidth  = get_length(wattr);
	iheight = get_length(hattr);

        if (get_attr(attrs, "SRC", attr, sizeof(attr))) {
	  img     = get_image(attr, iwidth, iheight);
	  iwidth  = img->w();
	  iheight = img->h();
	}

	if (iwidth > minwidths[column])
          minwidths[column] = iwidth;

        width += iwidth;
	if (needspace)
	  width += (int)fl_width(' ');

	if (width > max_width)
          max_width = width;

	needspace = 0;
      }
    }
    else if (*ptr == '\n' && pre)
    {
      width     = 0;
      needspace = 0;
      ptr ++;
    }
    else if (isspace((*ptr)&255))
    {
      needspace = 1;

      ptr ++;
    }
    else if (*ptr == '&' && s < (buf + sizeof(buf) - 1))
    {
      ptr ++;

      int qch = quote_char(ptr);

      if (qch < 0)
	*s++ = '&';
      else {
	*s++ = qch;
	ptr = strchr(ptr, ';') + 1;
      }
    }
    else
    {
      if (s < (buf + sizeof(buf) - 1))
        *s++ = *ptr++;
      else
        ptr ++;
    }
  }

  // Now that we have scanned the entire table, adjust the table and
  // cell widths to fit on the screen...
  if (get_attr(table + 6, "WIDTH", attr, sizeof(attr)))
    *table_width = get_length(attr);
  else
    *table_width = 0;

#ifdef DEBUG
  printf("num_columns = %d, table_width = %d\n", num_columns, *table_width);
#endif // DEBUG

  if (num_columns == 0)
    return;

  // Add up the widths...
  for (column = 0, width = 0; column < num_columns; column ++)
    width += columns[column];

#ifdef DEBUG
  printf("width = %d, w() = %d\n", width, w());
  for (column = 0; column < num_columns; column ++)
    printf("    columns[%d] = %d, minwidths[%d] = %d\n", column, columns[column],
           column, minwidths[column]);
#endif // DEBUG

  // Adjust the width if needed...
  int scale_width = *table_width;

  if (scale_width == 0) {
    if (width > (hsize_ - Fl::scrollbar_size())) scale_width = hsize_ - Fl::scrollbar_size();
    else scale_width = width;
  }

  if (width < scale_width) {
#ifdef DEBUG
    printf("Scaling table up to %d from %d...\n", scale_width, width);
#endif // DEBUG

    *table_width = 0;

    scale_width = (scale_width - width) / num_columns;

#ifdef DEBUG
    printf("adjusted scale_width = %d\n", scale_width);
#endif // DEBUG

    for (column = 0; column < num_columns; column ++) {
      columns[column] += scale_width;

      (*table_width) += columns[column];
    }
  }
  else if (width > scale_width) {
#ifdef DEBUG
    printf("Scaling table down to %d from %d...\n", scale_width, width);
#endif // DEBUG

    for (column = 0; column < num_columns; column ++) {
      width       -= minwidths[column];
      scale_width -= minwidths[column];
    }

#ifdef DEBUG
    printf("adjusted width = %d, scale_width = %d\n", width, scale_width);
#endif // DEBUG

    if (width > 0) {
      for (column = 0; column < num_columns; column ++) {
	columns[column] -= minwidths[column];
	columns[column] = scale_width * columns[column] / width;
	columns[column] += minwidths[column];
      }
    }

    *table_width = 0;
    for (column = 0; column < num_columns; column ++) {
      (*table_width) += columns[column];
    }
  }
  else if (*table_width == 0)
    *table_width = width;

#ifdef DEBUG
  printf("FINAL table_width = %d\n", *table_width);
  for (column = 0; column < num_columns; column ++)
    printf("    columns[%d] = %d\n", column, columns[column]);
#endif // DEBUG
}


//
// 'Fl_Help_View::free_data()' - Free memory used for the document.
//

void
Fl_Help_View::free_data() {
  // Releae all images...
  if (value_) {
    const char	*ptr,		// Pointer into block
		*attrs;		// Pointer to start of element attributes
    char	*s,		// Pointer into buffer
		buf[1024],	// Text buffer
		attr[1024],	// Attribute buffer
		wattr[1024],	// Width attribute buffer
		hattr[1024];	// Height attribute buffer


    for (ptr = value_; *ptr;)
    {
      if (*ptr == '<')
      {
	ptr ++;

        if (strncmp(ptr, "!--", 3) == 0)
	{
	  // Comment...
	  ptr += 3;
	  if ((ptr = strstr(ptr, "-->")) != NULL)
	  {
	    ptr += 3;
	    continue;
	  }
	  else
	    break;
	}

        s = buf;

	while (*ptr && *ptr != '>' && !isspace((*ptr)&255))
          if (s < (buf + sizeof(buf) - 1))
            *s++ = *ptr++;
	  else
	    ptr ++;

	*s = '\0';

	attrs = ptr;
	while (*ptr && *ptr != '>')
          ptr ++;

	if (*ptr == '>')
          ptr ++;

	if (strcasecmp(buf, "IMG") == 0)
	{
	  Fl_Shared_Image	*img;
	  int		width;
	  int		height;


          get_attr(attrs, "WIDTH", wattr, sizeof(wattr));
          get_attr(attrs, "HEIGHT", hattr, sizeof(hattr));
	  width  = get_length(wattr);
	  height = get_length(hattr);

	  if (get_attr(attrs, "SRC", attr, sizeof(attr))) {
	    // Release the image twice to free it from memory...
	    img = get_image(attr, width, height);
	    img->release();
	    img->release();
	  }
	}
      }
      else
        ptr ++;
    }

    free((void *)value_);
    value_ = 0;
  }

  // Free all of the arrays...
  if (nblocks_) {
    free(blocks_);

    ablocks_ = 0;
    nblocks_ = 0;
    blocks_  = 0;
  }

  if (nlinks_) {
    free(links_);

    alinks_ = 0;
    nlinks_ = 0;
    links_  = 0;
  }

  if (ntargets_) {
    free(targets_);

    atargets_ = 0;
    ntargets_ = 0;
    targets_  = 0;
  }
}

//
// 'Fl_Help_View::get_align()' - Get an alignment attribute.
//

int					// O - Alignment
Fl_Help_View::get_align(const char *p,	// I - Pointer to start of attrs
                        int        a)	// I - Default alignment
{
  char	buf[255];			// Alignment value


  if (get_attr(p, "ALIGN", buf, sizeof(buf)) == NULL)
    return (a);

  if (strcasecmp(buf, "CENTER") == 0)
    return (CENTER);
  else if (strcasecmp(buf, "RIGHT") == 0)
    return (RIGHT);
  else
    return (LEFT);
}


//
// 'Fl_Help_View::get_attr()' - Get an attribute value from the string.
//

const char *					// O - Pointer to buf or NULL
Fl_Help_View::get_attr(const char *p,		// I - Pointer to start of attributes
                      const char *n,		// I - Name of attribute
		      char       *buf,		// O - Buffer for attribute value
		      int        bufsize)	// I - Size of buffer
{
  char	name[255],				// Name from string
	*ptr,					// Pointer into name or value
	quote;					// Quote


  buf[0] = '\0';

  while (*p && *p != '>')
  {
    while (isspace((*p)&255))
      p ++;

    if (*p == '>' || !*p)
      return (NULL);

    for (ptr = name; *p && !isspace((*p)&255) && *p != '=' && *p != '>';)
      if (ptr < (name + sizeof(name) - 1))
        *ptr++ = *p++;
      else
        p ++;

    *ptr = '\0';

    if (isspace((*p)&255) || !*p || *p == '>')
      buf[0] = '\0';
    else
    {
      if (*p == '=')
        p ++;

      for (ptr = buf; *p && !isspace((*p)&255) && *p != '>';)
        if (*p == '\'' || *p == '\"')
	{
	  quote = *p++;

	  while (*p && *p != quote)
	    if ((ptr - buf + 1) < bufsize)
	      *ptr++ = *p++;
	    else
	      p ++;

          if (*p == quote)
	    p ++;
	}
	else if ((ptr - buf + 1) < bufsize)
	  *ptr++ = *p++;
	else
	  p ++;

      *ptr = '\0';
    }

    if (strcasecmp(n, name) == 0)
      return (buf);
    else
      buf[0] = '\0';

    if (*p == '>')
      return (NULL);
  }

  return (NULL);
}


//
// 'Fl_Help_View::get_color()' - Get an alignment attribute.
//

Fl_Color				// O - Color value
Fl_Help_View::get_color(const char *n,	// I - Color name
                        Fl_Color   c)	// I - Default color value
{
  int	i;				// Looping var
  int	rgb, r, g, b;			// RGB values
  static const struct {			// Color name table
    const char *name;
    int r, g, b;
  }	colors[] = {
    { "black",		0x00, 0x00, 0x00 },
    { "red",		0xff, 0x00, 0x00 },
    { "green",		0x00, 0x80, 0x00 },
    { "yellow",		0xff, 0xff, 0x00 },
    { "blue",		0x00, 0x00, 0xff },
    { "magenta",	0xff, 0x00, 0xff },
    { "fuchsia",	0xff, 0x00, 0xff },
    { "cyan",		0x00, 0xff, 0xff },
    { "aqua",		0x00, 0xff, 0xff },
    { "white",		0xff, 0xff, 0xff },
    { "gray",		0x80, 0x80, 0x80 },
    { "grey",		0x80, 0x80, 0x80 },
    { "lime",		0x00, 0xff, 0x00 },
    { "maroon",		0x80, 0x00, 0x00 },
    { "navy",		0x00, 0x00, 0x80 },
    { "olive",		0x80, 0x80, 0x00 },
    { "purple",		0x80, 0x00, 0x80 },
    { "silver",		0xc0, 0xc0, 0xc0 },
    { "teal",		0x00, 0x80, 0x80 }
  };


  if (!n || !n[0]) return c;

  if (n[0] == '#') {
    // Do hex color lookup
    rgb = strtol(n + 1, NULL, 16);

    if (strlen(n) > 4) {
      r = rgb >> 16;
      g = (rgb >> 8) & 255;
      b = rgb & 255;
    } else {
      r = (rgb >> 8) * 17;
      g = ((rgb >> 4) & 15) * 17;
      b = (rgb & 15) * 17;
    }
    return (fl_rgb_color((uchar)r, (uchar)g, (uchar)b));
  } else {
    for (i = 0; i < (int)(sizeof(colors) / sizeof(colors[0])); i ++)
      if (!strcasecmp(n, colors[i].name)) {
        return fl_rgb_color(colors[i].r, colors[i].g, colors[i].b);
      }
    return c;
  }
}


//
// 'Fl_Help_View::get_image()' - Get an inline image.
//

Fl_Shared_Image *
Fl_Help_View::get_image(const char *name, int W, int H) {
  const char	*localname;		// Local filename
  char		dir[1024];		// Current directory
  char		temp[1024],		// Temporary filename
		*tempptr;		// Pointer into temporary name
  Fl_Shared_Image *ip;			// Image pointer...

  // See if the image can be found...
  if (strchr(directory_, ':') != NULL && strchr(name, ':') == NULL) {
    if (name[0] == '/') {
      strlcpy(temp, directory_, sizeof(temp));

      if ((tempptr = strrchr(strchr(directory_, ':') + 3, '/')) != NULL) {
        strlcpy(tempptr, name, sizeof(temp) - (tempptr - temp));
      } else {
        strlcat(temp, name, sizeof(temp));
      }
    } else {
      snprintf(temp, sizeof(temp), "%s/%s", directory_, name);
    }

    if (link_) localname = (*link_)(this, temp);
    else localname = temp;
  } else if (name[0] != '/' && strchr(name, ':') == NULL) {
    if (directory_[0]) snprintf(temp, sizeof(temp), "%s/%s", directory_, name);
    else {
      getcwd(dir, sizeof(dir));
      snprintf(temp, sizeof(temp), "file:%s/%s", dir, name);
    }

    if (link_) localname = (*link_)(this, temp);
    else localname = temp;
  } else if (link_) localname = (*link_)(this, name);
  else localname = name;

  if (!localname) return 0;

  if (strncmp(localname, "file:", 5) == 0) localname += 5;

  if ((ip = Fl_Shared_Image::get(localname, W, H)) == NULL)
    ip = (Fl_Shared_Image *)&broken_image;

  return ip;
}


//
// 'Fl_Help_View::get_length()' - Get a length value, either absolute or %.
//

int
Fl_Help_View::get_length(const char *l) {	// I - Value
  int	val;					// Integer value

  if (!l[0]) return 0;

  val = atoi(l);
  if (l[strlen(l) - 1] == '%') {
    if (val > 100) val = 100;
    else if (val < 0) val = 0;

    val = val * (hsize_ - Fl::scrollbar_size()) / 100;
  }

  return val;
}


Fl_Help_Link *Fl_Help_View::find_link(int xx, int yy)
{
  int		i;
  Fl_Help_Link	*linkp;
  for (i = nlinks_, linkp = links_; i > 0; i --, linkp ++) {
    if (xx >= linkp->x && xx < linkp->w &&
        yy >= linkp->y && yy < linkp->h)
      break;
  }
  return i ? linkp : 0L;
}

void Fl_Help_View::follow_link(Fl_Help_Link *linkp)
{
  char		target[32];	// Current target

  clear_selection();

  strlcpy(target, linkp->name, sizeof(target));

  set_changed();

  if (strcmp(linkp->filename, filename_) != 0 && linkp->filename[0])
  {
    char	dir[1024];	// Current directory
    char	temp[1024],	// Temporary filename
	      *tempptr;	// Pointer into temporary filename


    if (strchr(directory_, ':') != NULL &&
        strchr(linkp->filename, ':') == NULL)
    {
      if (linkp->filename[0] == '/')
      {
        strlcpy(temp, directory_, sizeof(temp));
        if ((tempptr = strrchr(strchr(directory_, ':') + 3, '/')) != NULL)
	  strlcpy(tempptr, linkp->filename, sizeof(temp));
	else
	  strlcat(temp, linkp->filename, sizeof(temp));
      }
      else
	snprintf(temp, sizeof(temp), "%s/%s", directory_, linkp->filename);
    }
    else if (linkp->filename[0] != '/' && strchr(linkp->filename, ':') == NULL)
    {
      if (directory_[0])
	snprintf(temp, sizeof(temp), "%s/%s", directory_, linkp->filename);
      else
      {
	getcwd(dir, sizeof(dir));
	snprintf(temp, sizeof(temp), "file:%s/%s", dir, linkp->filename);
      }
    }
    else
      strlcpy(temp, linkp->filename, sizeof(temp));

    if (linkp->name[0])
      snprintf(temp + strlen(temp), sizeof(temp) - strlen(temp), "#%s",
	       linkp->name);

    load(temp);
  }
  else if (target[0])
    topline(target);
  else
    topline(0);

  leftline(0);
}

void Fl_Help_View::clear_selection()
{
  if (current_view==this)
    clear_global_selection();
}

void Fl_Help_View::select_all()
{
  clear_global_selection();
  if (!value_) return;
  current_view = this;
  selection_drag_last = selection_last = strlen(value_);
  selected = 1;
}

void Fl_Help_View::clear_global_selection()
{
  if (selected) redraw();
  selection_push_first = selection_push_last = 0;
  selection_drag_first = selection_drag_last = 0;
  selection_first = selection_last = 0;
  selected = 0;
}

char Fl_Help_View::begin_selection()
{
  clear_global_selection();

  if (!fl_help_view_buffer) fl_help_view_buffer = fl_create_offscreen(1, 1);

  mouse_x = Fl::event_x();
  mouse_y = Fl::event_y();
  draw_mode = 1;

    current_view = this;
    fl_begin_offscreen(fl_help_view_buffer);
    draw();
    fl_end_offscreen();

  draw_mode = 0;

  if (selection_push_last) return 1;
  else return 0;
}

char Fl_Help_View::extend_selection()
{
  if (Fl::event_is_click())
    return 0;

//  printf("old selection_first=%d, selection_last=%d\n",
//         selection_first, selection_last);

  int sf = selection_first, sl = selection_last;

  selected = 1;
  mouse_x = Fl::event_x();
  mouse_y = Fl::event_y();
  draw_mode = 2;

    fl_begin_offscreen(fl_help_view_buffer);
    draw();
    fl_end_offscreen();

  draw_mode = 0;

  if (selection_push_first < selection_drag_first) {
    selection_first = selection_push_first;
  } else {
    selection_first = selection_drag_first;
  }

  if (selection_push_last > selection_drag_last) {
    selection_last = selection_push_last;
  } else {
    selection_last = selection_drag_last;
  }

//  printf("new selection_first=%d, selection_last=%d\n",
//         selection_first, selection_last);

  if (sf!=selection_first || sl!=selection_last) {
//    puts("REDRAW!!!\n");
    return 1;
  } else {
//    puts("");
    return 0;
  }
}

// convert a command with up to four letters into an unsigned int
static unsigned int command(const char *cmd)
{
  unsigned int ret = (tolower(cmd[0])<<24);
  char c = cmd[1];
  if (c=='>' || c==' ' || c==0) return ret;
  ret |= (tolower(c)<<16);
  c = cmd[2];
  if (c=='>' || c==' ' || c==0) return ret;
  ret |= (tolower(c)<<8);
  c = cmd[3];
  if (c=='>' || c==' ' || c==0) return ret;
  ret |= tolower(c);
  c = cmd[4];
  if (c=='>' || c==' ' || c==0) return ret;
  return 0;
}

#define CMD(a, b, c, d) ((a<<24)|(b<<16)|(c<<8)|d)

void Fl_Help_View::end_selection(int clipboard) 
{
  if (!selected || current_view!=this) 
    return;
  // convert the select part of our html text into some kind of somewhat readable ASCII
  // and store it in the selection buffer
  char p = 0, pre = 0;;
  int len = strlen(value_);
  char *txt = (char*)malloc(len+1), *d = txt;
  const char *s = value_, *cmd, *src;
  for (;;) {
    char c = *s++;
    if (c==0) break;
    if (c=='<') { // begin of some html command. Skip until we find a '>'
      cmd = s;
      for (;;) {
        c = *s++;
        if (c==0 || c=='>') break;
      }
      if (c==0) break;
      // do something with this command... .
      // the replacement string must not be longer that the command itself plus '<' and '>'
      src = 0;
      switch (command(cmd)) {
        case CMD('p','r','e', 0 ): pre = 1; break;
        case CMD('/','p','r','e'): pre = 0; break;
        case CMD('t','d', 0 , 0 ):
        case CMD('p', 0 , 0 , 0 ):
        case CMD('/','p', 0 , 0 ):
        case CMD('b','r', 0 , 0 ): src = "\n"; break;
        case CMD('l','i', 0 , 0 ): src = "\n * "; break;
        case CMD('/','h','1', 0 ):
        case CMD('/','h','2', 0 ):
        case CMD('/','h','3', 0 ):
        case CMD('/','h','4', 0 ):
        case CMD('/','h','5', 0 ):
        case CMD('/','h','6', 0 ): src = "\n\n"; break;
        case CMD('t','r', 0 , 0 ):
        case CMD('h','1', 0 , 0 ):
        case CMD('h','2', 0 , 0 ):
        case CMD('h','3', 0 , 0 ):
        case CMD('h','4', 0 , 0 ):
        case CMD('h','5', 0 , 0 ):
        case CMD('h','6', 0 , 0 ): src = "\n\n"; break;
        case CMD('d','t', 0 , 0 ): src = "\n "; break;
        case CMD('d','d', 0 , 0 ): src = "\n - "; break;
      }
      int n = s-value_;
      if (src && n>selection_first && n<=selection_last) {
        while (*src) {
          *d++ = *src++;
        }
        c = src[-1];
        p = isspace(c&255) ? ' ' : c;
      }
      continue;
    }
    if (c=='&') { // special characters
      int xx = quote_char(s);
      if (xx>=0) {
        c = (char)xx;
        for (;;) {
          char cc = *s++;
          if (!cc || cc==';') break;
        }
      }
    }
    int n = s-value_;
    if (n>selection_first && n<=selection_last) {
      if (!pre && isspace(c&255)) c = ' ';
      if (p!=' '||c!=' ')
        *d++ = c;
      p = c;
    }
  }
  *d = 0;
  Fl::copy(txt, strlen(txt), clipboard);
  free(txt);
}

#define ctrl(x) ((x)&0x1f)

//
// 'Fl_Help_View::handle()' - Handle events in the widget.
//

int				// O - 1 if we handled it, 0 otherwise
Fl_Help_View::handle(int event)	// I - Event to handle
{
  static Fl_Help_Link *linkp;   // currently clicked link

  int xx = Fl::event_x() - x() + leftline_;
  int yy = Fl::event_y() - y() + topline_;

  switch (event)
  {
    case FL_FOCUS:
      redraw();
      return 1;
    case FL_UNFOCUS:
      clear_selection();
      redraw();
      return 1;
    case FL_ENTER :
      Fl_Group::handle(event);
      return 1;
    case FL_LEAVE :
      fl_cursor(FL_CURSOR_DEFAULT);
      break;
    case FL_MOVE:
      if (find_link(xx, yy)) fl_cursor(FL_CURSOR_HAND);
      else fl_cursor(FL_CURSOR_DEFAULT);
      return 1;
    case FL_PUSH:
      if (Fl_Group::handle(event)) return 1;
      linkp = find_link(xx, yy);
      if (linkp) {
        fl_cursor(FL_CURSOR_HAND);
        return 1;
      }
      if (begin_selection()) {
        fl_cursor(FL_CURSOR_INSERT);
        return 1;
      }
      fl_cursor(FL_CURSOR_DEFAULT);
      return 1;
    case FL_DRAG:
      if (linkp) {
        if (Fl::event_is_click()) {
          fl_cursor(FL_CURSOR_HAND);
        } else {
          fl_cursor(FL_CURSOR_DEFAULT); // should be "FL_CURSOR_CANCEL" if we had it
        }
        return 1;
      }
      if (current_view==this && selection_push_last) {
        if (extend_selection()) redraw();
        fl_cursor(FL_CURSOR_INSERT);
        return 1;
      }
      fl_cursor(FL_CURSOR_DEFAULT);
      return 1;
    case FL_RELEASE:
      if (linkp) {
        if (Fl::event_is_click()) {
          follow_link(linkp);
        }
        fl_cursor(FL_CURSOR_DEFAULT);
        linkp = 0;
        return 1;
      }
      if (current_view==this && selection_push_last) {
        end_selection();
        return 1;
      }
      return 1;
    case FL_SHORTCUT: {
      char ascii = Fl::event_text()[0];
      switch (ascii) {
        case ctrl('A'): select_all(); redraw(); return 1;
        case ctrl('C'):
        case ctrl('X'): end_selection(1); return 1;
      }
      break; }
  }
  return (Fl_Group::handle(event));
}

//
// 'Fl_Help_View::Fl_Help_View()' - Build a Fl_Help_View widget.
//

Fl_Help_View::Fl_Help_View(int        xx,	// I - Left position
                	   int        yy,	// I - Top position
			   int        ww,	// I - Width in pixels
			   int        hh,	// I - Height in pixels
			   const char *l)
    : Fl_Group(xx, yy, ww, hh, l),
      scrollbar_(xx + ww - Fl::scrollbar_size(), yy,
                 Fl::scrollbar_size(), hh - Fl::scrollbar_size()),
      hscrollbar_(xx, yy + hh - Fl::scrollbar_size(),
                  ww - Fl::scrollbar_size(), Fl::scrollbar_size())
{
  color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);

  title_[0]     = '\0';
  defcolor_     = FL_FOREGROUND_COLOR;
  bgcolor_      = FL_BACKGROUND_COLOR;
  textcolor_    = FL_FOREGROUND_COLOR;
  linkcolor_    = FL_SELECTION_COLOR;
  textfont_     = FL_TIMES;
  textsize_     = 12;
  value_        = NULL;

  ablocks_      = 0;
  nblocks_      = 0;
  blocks_       = (Fl_Help_Block *)0;

  nfonts_       = 0;

  link_         = (Fl_Help_Func *)0;

  alinks_       = 0;
  nlinks_       = 0;
  links_        = (Fl_Help_Link *)0;

  atargets_     = 0;
  ntargets_     = 0;
  targets_      = (Fl_Help_Target *)0;

  directory_[0] = '\0';
  filename_[0]  = '\0';

  topline_      = 0;
  leftline_     = 0;
  size_         = 0;
  hsize_        = 0;

  scrollbar_.value(0, hh, 0, 1);
  scrollbar_.step(8.0);
  scrollbar_.show();
  scrollbar_.callback(scrollbar_callback);

  hscrollbar_.value(0, ww, 0, 1);
  hscrollbar_.step(8.0);
  hscrollbar_.show();
  hscrollbar_.callback(hscrollbar_callback);
  hscrollbar_.type(FL_HORIZONTAL);
  end();

  resize(xx, yy, ww, hh);
}


//
// 'Fl_Help_View::~Fl_Help_View()' - Destroy a Fl_Help_View widget.
//

Fl_Help_View::~Fl_Help_View()
{
  clear_selection();
  free_data();
}


//
// 'Fl_Help_View::load()' - Load the specified file.
//

int				// O - 0 on success, -1 on error
Fl_Help_View::load(const char *f)// I - Filename to load (may also have target)
{
  FILE		*fp;		// File to read from
  long		len;		// Length of file
  char		*target;	// Target in file
  char		*slash;		// Directory separator
  const char	*localname;	// Local filename
  char		error[1024];	// Error buffer
  char		newname[1024];	// New filename buffer


  clear_selection();

  strlcpy(newname, f, sizeof(newname));
  if ((target = strrchr(newname, '#')) != NULL)
    *target++ = '\0';

  if (link_)
    localname = (*link_)(this, newname);
  else
    localname = filename_;

  if (!localname)
    return (0);

  strlcpy(filename_, newname, sizeof(filename_));
  strlcpy(directory_, newname, sizeof(directory_));

  // Note: We do not support Windows backslashes, since they are illegal
  //       in URLs...
  if ((slash = strrchr(directory_, '/')) == NULL)
    directory_[0] = '\0';
  else if (slash > directory_ && slash[-1] != '/')
    *slash = '\0';

  if (value_ != NULL)
  {
    free((void *)value_);
    value_ = NULL;
  }

  if (strncmp(localname, "ftp:", 4) == 0 ||
      strncmp(localname, "http:", 5) == 0 ||
      strncmp(localname, "https:", 6) == 0 ||
      strncmp(localname, "ipp:", 4) == 0 ||
      strncmp(localname, "mailto:", 7) == 0 ||
      strncmp(localname, "news:", 5) == 0)
  {
    // Remote link wasn't resolved...
    snprintf(error, sizeof(error),
             "<HTML><HEAD><TITLE>Error</TITLE></HEAD>"
             "<BODY><H1>Error</H1>"
	     "<P>Unable to follow the link \"%s\" - "
	     "no handler exists for this URI scheme.</P></BODY>",
	     localname);
    value_ = strdup(error);
  }
  else
  {
    if (strncmp(localname, "file:", 5) == 0)
      localname += 5;	// Adjust for local filename...

    if ((fp = fopen(localname, "rb")) != NULL)
    {
      fseek(fp, 0, SEEK_END);
      len = ftell(fp);
      rewind(fp);

      value_ = (const char *)calloc(len + 1, 1);
      fread((void *)value_, 1, len, fp);
      fclose(fp);
    }
    else
    {
      snprintf(error, sizeof(error),
               "<HTML><HEAD><TITLE>Error</TITLE></HEAD>"
               "<BODY><H1>Error</H1>"
	       "<P>Unable to follow the link \"%s\" - "
	       "%s.</P></BODY>",
	       localname, strerror(errno));
      value_ = strdup(error);
    }
  }

  format();

  if (target)
    topline(target);
  else
    topline(0);

  return (0);
}


//
// 'Fl_Help_View::resize()' - Resize the help widget.
//

void
Fl_Help_View::resize(int xx,	// I - New left position
                     int yy,	// I - New top position
		     int ww,	// I - New width
		     int hh)	// I - New height
{
  Fl_Boxtype		b = box() ? box() : FL_DOWN_BOX;
					// Box to draw...


  Fl_Widget::resize(xx, yy, ww, hh);

  int ss = Fl::scrollbar_size();
  scrollbar_.resize(x() + w() - ss - Fl::box_dw(b) + Fl::box_dx(b),
                    y() + Fl::box_dy(b), ss, h() - ss - Fl::box_dh(b));
  hscrollbar_.resize(x() + Fl::box_dx(b),
                     y() + h() - ss - Fl::box_dh(b) + Fl::box_dy(b),
                     w() - ss - Fl::box_dw(b), ss);

  format();
}


//
// 'Fl_Help_View::topline()' - Set the top line to the named target.
//

void
Fl_Help_View::topline(const char *n)	// I - Target name
{
  Fl_Help_Target key,			// Target name key
		*target;		// Pointer to matching target


  if (ntargets_ == 0)
    return;

  strlcpy(key.name, n, sizeof(key.name));

  target = (Fl_Help_Target *)bsearch(&key, targets_, ntargets_, sizeof(Fl_Help_Target),
                                 (compare_func_t)compare_targets);

  if (target != NULL)
    topline(target->y);
}


//
// 'Fl_Help_View::topline()' - Set the top line by number.
//

void
Fl_Help_View::topline(int t)	// I - Top line number
{
  if (!value_)
    return;

  if (size_ < (h() - Fl::scrollbar_size()) || t < 0)
    t = 0;
  else if (t > size_)
    t = size_;

  topline_ = t;

  scrollbar_.value(topline_, h() - Fl::scrollbar_size(), 0, size_);

  do_callback();

  redraw();
}


//
// 'Fl_Help_View::leftline()' - Set the left position.
//

void
Fl_Help_View::leftline(int l)	// I - Left position
{
  if (!value_)
    return;

  if (hsize_ < (w() - Fl::scrollbar_size()) || l < 0)
    l = 0;
  else if (l > hsize_)
    l = hsize_;

  leftline_ = l;

  hscrollbar_.value(leftline_, w() - Fl::scrollbar_size(), 0, hsize_);

  redraw();
}


//
// 'Fl_Help_View::value()' - Set the help text directly.
//

void
Fl_Help_View::value(const char *v)	// I - Text to view
{
  clear_selection();
  free_data();
  set_changed();

  if (!v)
    return;

  value_ = strdup(v);

  format();

  topline(0);
  leftline(0);
}

#ifdef ENC
# undef ENC
#endif
#ifdef __APPLE__
# define ENC(a, b) b
#else
# define ENC(a, b) a
#endif

//
// 'quote_char()' - Return the character code associated with a quoted char.
//

static int			// O - Code or -1 on error
quote_char(const char *p) {	// I - Quoted string
  int	i;			// Looping var
  static struct {
    const char	*name;
    int		namelen;
    int		code;
  }	*nameptr,		// Pointer into name array
	names[] = {		// Quoting names
    { "Aacute;", 7, ENC(193,231) },
    { "aacute;", 7, ENC(225,135) },
    { "Acirc;",  6, ENC(194,229) },
    { "acirc;",  6, ENC(226,137) },
    { "acute;",  6, ENC(180,171) },
    { "AElig;",  6, ENC(198,174) },
    { "aelig;",  6, ENC(230,190) },
    { "Agrave;", 7, ENC(192,203) },
    { "agrave;", 7, ENC(224,136) },
    { "amp;",    4, ENC('&','&') },
    { "Aring;",  6, ENC(197,129) },
    { "aring;",  6, ENC(229,140) },
    { "Atilde;", 7, ENC(195,204) },
    { "atilde;", 7, ENC(227,139) },
    { "Auml;",   5, ENC(196,128) },
    { "auml;",   5, ENC(228,138) },
    { "brvbar;", 7, ENC(166, -1) },
    { "bull;",   5, ENC(149,165) },
    { "Ccedil;", 7, ENC(199,199) },
    { "ccedil;", 7, ENC(231,141) },
    { "cedil;",  6, ENC(184,252) },
    { "cent;",   5, ENC(162,162) },
    { "copy;",   5, ENC(169,169) },
    { "curren;", 7, ENC(164, -1) },
    { "deg;",    4, ENC(176,161) },
    { "divide;", 7, ENC(247,214) },
    { "Eacute;", 7, ENC(201,131) },
    { "eacute;", 7, ENC(233,142) },
    { "Ecirc;",  6, ENC(202,230) },
    { "ecirc;",  6, ENC(234,144) },
    { "Egrave;", 7, ENC(200,233) },
    { "egrave;", 7, ENC(232,143) },
    { "ETH;",    4, ENC(208, -1) },
    { "eth;",    4, ENC(240, -1) },
    { "Euml;",   5, ENC(203,232) },
    { "euml;",   5, ENC(235,145) },
    { "euro;",   5, ENC(128,219) },
    { "frac12;", 7, ENC(189, -1) },
    { "frac14;", 7, ENC(188, -1) },
    { "frac34;", 7, ENC(190, -1) },
    { "gt;",     3, ENC('>','>') },
    { "Iacute;", 7, ENC(205,234) },
    { "iacute;", 7, ENC(237,146) },
    { "Icirc;",  6, ENC(206,235) },
    { "icirc;",  6, ENC(238,148) },
    { "iexcl;",  6, ENC(161,193) },
    { "Igrave;", 7, ENC(204,237) },
    { "igrave;", 7, ENC(236,147) },
    { "iquest;", 7, ENC(191,192) },
    { "Iuml;",   5, ENC(207,236) },
    { "iuml;",   5, ENC(239,149) },
    { "laquo;",  6, ENC(171,199) },
    { "lt;",     3, ENC('<','<') },
    { "macr;",   5, ENC(175,248) },
    { "micro;",  6, ENC(181,181) },
    { "middot;", 7, ENC(183,225) },
    { "nbsp;",   5, ENC(' ',' ') },
    { "not;",    4, ENC(172,194) },
    { "Ntilde;", 7, ENC(209,132) },
    { "ntilde;", 7, ENC(241,150) },
    { "Oacute;", 7, ENC(211,238) },
    { "oacute;", 7, ENC(243,151) },
    { "Ocirc;",  6, ENC(212,239) },
    { "ocirc;",  6, ENC(244,153) },
    { "Ograve;", 7, ENC(210,241) },
    { "ograve;", 7, ENC(242,152) },
    { "ordf;",   5, ENC(170,187) },
    { "ordm;",   5, ENC(186,188) },
    { "Oslash;", 7, ENC(216,175) },
    { "oslash;", 7, ENC(248,191) },
    { "Otilde;", 7, ENC(213,205) },
    { "otilde;", 7, ENC(245,155) },
    { "Ouml;",   5, ENC(214,133) },
    { "ouml;",   5, ENC(246,154) },
    { "para;",   5, ENC(182,166) },
    { "premil;", 7, ENC(137,228) },
    { "plusmn;", 7, ENC(177,177) },
    { "pound;",  6, ENC(163,163) },
    { "quot;",   5, ENC('\"','\"') },
    { "raquo;",  6, ENC(187,200) },
    { "reg;",    4, ENC(174,168) },
    { "sect;",   5, ENC(167,164) },
    { "shy;",    4, ENC(173,'-') },
    { "sup1;",   5, ENC(185, -1) },
    { "sup2;",   5, ENC(178, -1) },
    { "sup3;",   5, ENC(179, -1) },
    { "szlig;",  6, ENC(223,167) },
    { "THORN;",  6, ENC(222, -1) },
    { "thorn;",  6, ENC(254, -1) },
    { "times;",  6, ENC(215,'x') },
    { "trade;",  6, ENC(153,170) },
    { "Uacute;", 7, ENC(218,242) },
    { "uacute;", 7, ENC(250,156) },
    { "Ucirc;",  6, ENC(219,243) },
    { "ucirc;",  6, ENC(251,158) },
    { "Ugrave;", 7, ENC(217,244) },
    { "ugrave;", 7, ENC(249,157) },
    { "uml;",    4, ENC(168,172) },
    { "Uuml;",   5, ENC(220,134) },
    { "uuml;",   5, ENC(252,159) },
    { "Yacute;", 7, ENC(221, -1) },
    { "yacute;", 7, ENC(253, -1) },
    { "yen;",    4, ENC(165,180) },
    { "Yuml;",   5, ENC(159,217) },
    { "yuml;",   5, ENC(255,216) }
  };

  if (!strchr(p, ';')) return -1;
  if (*p == '#') {
    if (*(p+1) == 'x' || *(p+1) == 'X') return strtol(p+2, NULL, 16);
    else return atoi(p+1);
  }
  for (i = (int)(sizeof(names) / sizeof(names[0])), nameptr = names; i > 0; i --, nameptr ++)
    if (strncmp(p, nameptr->name, nameptr->namelen) == 0)
      return nameptr->code;

  return -1;
}


//
// 'scrollbar_callback()' - A callback for the scrollbar.
//

static void
scrollbar_callback(Fl_Widget *s, void *)
{
  ((Fl_Help_View *)(s->parent()))->topline(int(((Fl_Scrollbar*)s)->value()));
}


//
// 'hscrollbar_callback()' - A callback for the horizontal scrollbar.
//

static void
hscrollbar_callback(Fl_Widget *s, void *)
{
  ((Fl_Help_View *)(s->parent()))->leftline(int(((Fl_Scrollbar*)s)->value()));
}


//
// End of "$Id: Fl_Help_View.cxx 6091 2008-04-11 11:12:16Z matt $".
//
