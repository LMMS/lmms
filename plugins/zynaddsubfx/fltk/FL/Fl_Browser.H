//
// "$Id: Fl_Browser.H 8623 2011-04-24 17:09:41Z AlbrechtS $"
//
// Browser header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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
   Fl_Browser widget . */

// Forms-compatible browser.  Probably useful for other
// lists of textual data.  Notice that the line numbers
// start from 1, and 0 means "no line".

#ifndef Fl_Browser_H
#define Fl_Browser_H

#include "Fl_Browser_.H"
#include "Fl_Image.H"

struct FL_BLINE;

/**
  The Fl_Browser widget displays a scrolling list of text
  lines, and manages all the storage for the text.  This is not a text
  editor or spreadsheet!  But it is useful for showing a vertical list of
  named objects to the user.
  
  Each line in the browser is identified by number. <I>The numbers
  start at one</I> (this is so that zero can be reserved for "no line" in
  the selective browsers). <I>Unless otherwise noted, the methods do not
  check to see if the passed line number is in range and legal.  It must
  always be greater than zero and &lt;= size().</I>

  Each line contains a null-terminated string of text and a void *
  data pointer.  The text string is displayed, the void *
  pointer can be used by the callbacks to reference the object the text
  describes.

  The base class does nothing when the user clicks on it.  The
  subclasses 
  Fl_Select_Browser, 
  Fl_Hold_Browser, and 
  Fl_Multi_Browser react to user clicks to select lines in
  the browser and do callbacks.

  The base class
  Fl_Browser_ provides the scrolling and selection mechanisms of
  this and all the subclasses, but the dimensions and appearance of each
  item are determined by the subclass. You can use Fl_Browser_
  to display information other than text, or text that is dynamically
  produced from your own data structures. If you find that loading the
  browser is a lot of work or is inefficient, you may want to make a
  subclass of Fl_Browser_.

  Some common coding patterns used for working with Fl_Browser:
  \code
      // How to loop through all the items in the browser
      for ( int t=1; t<=browser->size(); t++ ) {       // index 1 based..!
	  printf("item #%d, label='%s'\n", t, browser->text(t));
      }
  \endcode

  Note: If you are <I>subclassing</I> Fl_Browser, it's more efficient
  to use the protected methods item_first() and item_next(), since
  Fl_Browser internally uses linked lists to manage the browser's items.
  For more info, see find_item(int).
*/
class FL_EXPORT Fl_Browser : public Fl_Browser_ {

  FL_BLINE *first;		// the array of lines
  FL_BLINE *last;
  FL_BLINE *cache;
  int cacheline;		// line number of cache
  int lines;                	// Number of lines
  int full_height_;
  const int* column_widths_;
  char format_char_;		// alternative to @-sign
  char column_char_;		// alternative to tab

protected:

  // required routines for Fl_Browser_ subclass:
  void* item_first() const ;
  void* item_next(void* item) const ;
  void* item_prev(void* item) const ;
  void* item_last()const ;
  int item_selected(void* item) const ;
  void item_select(void* item, int val);
  int item_height(void* item) const ;
  int item_width(void* item) const ;
  void item_draw(void* item, int X, int Y, int W, int H) const ;
  int full_height() const ;
  int incr_height() const ;
  const char *item_text(void *item) const;
  /** Swap the items \p a and \p b.
      You must call redraw() to make any changes visible.
      \param[in] a,b the items to be swapped.
      \see swap(int,int), item_swap()
   */
  void item_swap(void *a, void *b) { swap((FL_BLINE*)a, (FL_BLINE*)b); }
  /** Return the item at specified \p line.
      \param[in] line The line of the item to return. (1 based)
      \returns The item, or NULL if line out of range.
      \see item_at(), find_line(), lineno()
   */
  void *item_at(int line) const { return (void*)find_line(line); }

  FL_BLINE* find_line(int line) const ;
  FL_BLINE* _remove(int line) ;
  void insert(int line, FL_BLINE* item);
  int lineno(void *item) const ;
  void swap(FL_BLINE *a, FL_BLINE *b);

public:

  void remove(int line);
  void add(const char* newtext, void* d = 0);
  void insert(int line, const char* newtext, void* d = 0);
  void move(int to, int from);
  int  load(const char* filename);
  void swap(int a, int b);
  void clear();

  /**
    Returns how many lines are in the browser.
    The last line number is equal to this.
    Returns 0 if browser is empty.
  */
  int size() const { return lines; }
  void size(int W, int H) { Fl_Widget::size(W, H); }

  int topline() const ;
  /** For internal use only? */
  enum Fl_Line_Position { TOP, BOTTOM, MIDDLE };
  void lineposition(int line, Fl_Line_Position pos);
  /**
    Scrolls the browser so the top item in the browser
    is showing the specified \p line.
    \param[in] line The line to be displayed at the top.
    \see topline(), middleline(), bottomline(), displayed(), lineposition()
   */
  void topline(int line) { lineposition(line, TOP); }
  /**
    Scrolls the browser so the bottom item in the browser
    is showing the specified \p line.
    \param[in] line The line to be displayed at the bottom.
    \see topline(), middleline(), bottomline(), displayed(), lineposition()
   */
  void bottomline(int line) { lineposition(line, BOTTOM); }
  /**
    Scrolls the browser so the middle item in the browser
    is showing the specified \p line.
    \param[in] line The line to be displayed in the middle.
    \see topline(), middleline(), bottomline(), displayed(), lineposition()
   */
  void middleline(int line) { lineposition(line, MIDDLE); }

  int select(int line, int val=1);
  int selected(int line) const ;
  void show(int line);
  /** Shows the entire Fl_Browser widget -- opposite of hide(). */
  void show() { Fl_Widget::show(); }
  void hide(int line);
  /** Hides the entire Fl_Browser widget -- opposite of show(). */
  void hide() { Fl_Widget::hide(); }
  int visible(int line) const ;

  int value() const ;
  /**
    Sets the browser's value(), which selects the specified \p line.
    This is the same as calling select(line).
    \see select(), selected(), value(), item_select(), item_selected()
   */
  void value(int line) { select(line); }
  const char* text(int line) const ;
  void text(int line, const char* newtext);
  void* data(int line) const ;
  void data(int line, void* d);

  Fl_Browser(int X, int Y, int W, int H, const char *L = 0);
  /**
    The destructor deletes all list items and destroys the browser.
   */
  ~Fl_Browser() { clear(); }

  /**
    Gets the current format code prefix character, which by default is '\@'.
    A string of formatting codes at the start of each column are stripped off
    and used to modify how the rest of the line is printed:
    
    \li <tt>'\@.'</tt> Print rest of line, don't look for more '\@' signs
    \li <tt>'\@\@'</tt> Print rest of line starting with '\@'
    \li <tt>'\@l'</tt> Use a LARGE (24 point) font
    \li <tt>'\@m'</tt> Use a medium large (18 point) font
    \li <tt>'\@s'</tt> Use a <SMALL>small</SMALL> (11 point) font
    \li <tt>'\@b'</tt> Use a <B>bold</B> font (adds FL_BOLD to font)
    \li <tt>'\@i'</tt> Use an <I>italic</I> font (adds FL_ITALIC to font)
    \li <tt>'\@f' or '\@t'</tt> Use a fixed-pitch
    font (sets font to FL_COURIER)
    \li <tt>'\@c'</tt> Center the line horizontally
    \li <tt>'\@r'</tt> Right-justify the text
    \li <tt>'\@B0', '\@B1', ... '\@B255'</tt> Fill the backgound with
    fl_color(n)
    \li <tt>'\@C0', '\@C1', ... '\@C255'</tt> Use fl_color(n) to draw the text
    \li <tt>'\@F0', '\@F1', ...</tt> Use fl_font(n) to draw the text
    \li <tt>'\@S1', '\@S2', ...</tt> Use point size n to draw the text
    \li <tt>'\@u' or '\@_'</tt> Underline the text.
    \li <tt>'\@-'</tt> draw an engraved line through the middle.

    Notice that the '\@.' command can be used to reliably
    terminate the parsing.  To print a random string in a random color, use 
    <tt>sprintf("@C%d@.%s", color, string)</tt> and it will work even if the
    string starts with a digit or has the format character in it.
  */
  char format_char() const { return format_char_; }
  /**
    Sets the current format code prefix character to \p c.
    The default prefix is '\@'.  Set the prefix to 0 to disable formatting.
    \see format_char() for list of '\@' codes
  */
  void format_char(char c) { format_char_ = c; }
  /**
    Gets the current column separator character.
    The default is '\\t' (tab).
    \see column_char(), column_widths()
  */
  char column_char() const { return column_char_; }
  /**
    Sets the column separator to c.
    This will only have an effect if you also set column_widths().
    The default is '\\t' (tab).
    \see column_char(), column_widths()
  */
  void column_char(char c) { column_char_ = c; }
  /**
    Gets the current column width array.
    This array is zero-terminated and specifies the widths in pixels of
    each column. The text is split at each column_char() and each part is
    formatted into it's own column.  After the last column any remaining
    text is formatted into the space between the last column and the
    right edge of the browser, even if the text contains instances of
    column_char() .  The default value is a one-element array of just
    a zero, which means there are no columns.

    Example:
    \code
  Fl_Browser *b = new Fl_Browser(..);
  int widths[] = { 50, 50, 50, 70, 70, 40, 40, 70, 70, 50, 0 };  // widths for each column
  b->column_widths(widths); // assign array to widget
  b->column_char('\t');     // use tab as the column character
  b->add("USER\tPID\tCPU\tMEM\tVSZ\tRSS\tTTY\tSTAT\tSTART\tTIME\tCOMMAND");
  b->add("root\t2888\t0.0\t0.0\t1352\t0\ttty3\tSW\tAug15\t0:00\t@b@f/sbin/mingetty tty3");
  b->add("root\t13115\t0.0\t0.0\t1352\t0\ttty2\tSW\tAug30\t0:00\t@b@f/sbin/mingetty tty2");
  [..]
    \endcode
    \see column_char(), column_widths()
  */
  const int* column_widths() const { return column_widths_; }
  /**
    Sets the current array to \p arr.  Make sure the last entry is zero.
    \see column_char(), column_widths()
  */
  void column_widths(const int* arr) { column_widths_ = arr; }

  /**
    Returns non-zero if \p line has been scrolled to a position where it is being displayed.
    Checks to see if the item's vertical position is within the top and bottom
    edges of the display window. This does NOT take into account the hide()/show()
    status of the widget or item.
    \param[in] line The line to be checked
    \returns 1 if visible, 0 if not visible.
    \see topline(), middleline(), bottomline(), displayed(), lineposition()
  */
  int displayed(int line) const { return Fl_Browser_::displayed(find_line(line)); }

  /**
    Make the item at the specified \p line visible().
    Functionally similar to show(int line).
    If \p line is out of range, redisplay top or bottom of list as appropriate.
    \param[in] line The line to be made visible.
    \see show(int), hide(int), display(), visible(), make_visible()
  */
  void make_visible(int line) {
    if (line < 1) Fl_Browser_::display(find_line(1));
    else if (line > lines) Fl_Browser_::display(find_line(lines));
    else Fl_Browser_::display(find_line(line));
  }

  // icon support
  void icon(int line, Fl_Image* icon);
  Fl_Image* icon(int line) const;
  void remove_icon(int line);

  /** For back compatibility only. */
  void replace(int a, const char* b) { text(a, b); }
  void display(int line, int val=1);
};

#endif

//
// End of "$Id: Fl_Browser.H 8623 2011-04-24 17:09:41Z AlbrechtS $".
//
