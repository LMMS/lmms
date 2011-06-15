//
// "$Id: Fl_Table.H 8301 2011-01-22 22:40:11Z AlbrechtS $"
//
// Fl_Table -- A table widget
//
// Copyright 2002 by Greg Ercolano.
// Copyright (c) 2004 O'ksi'D
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
// Please report all bugs and problems to "erco at seriss dot com".
//
// TODO:
//    o Auto scroll during dragged selection
//    o Keyboard navigation (up/down/left/right arrow)
//

#ifndef _FL_TABLE_H
#define _FL_TABLE_H

#include <sys/types.h>
#include <string.h>		// memcpy
#ifdef WIN32
#include <malloc.h>		// WINDOWS: malloc/realloc
#else /*WIN32*/
#include <stdlib.h>		// UNIX: malloc/realloc
#endif /*WIN32*/

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scrollbar.H>

/**
 A table of widgets or other content.
 
 This is the base class for table widgets.
 
 To be useful it must be subclassed and several virtual functions defined.
 Normally applications use widgets derived from this widget, and do not use this 
 widget directly; this widget is usually too low level to be used directly by 
 applications.
 
 This widget does \em not handle the data in the table. The draw_cell()
 method must be overridden by a subclass to manage drawing the contents of 
 the cells.
 
 This widget can be used in several ways:
 
 - As a custom widget; see examples/table-simple.cxx and test/table.cxx.
   Very optimal for even extremely large tables.
 - As a table made up of a single FLTK widget instanced all over the table,
   simulating a numeric spreadsheet. See examples/table-spreadsheet.cxx and
   examples/table-spreadsheet-with-keyboard-nav.cxx. Optimal for large tables.
 - As a regular container of FLTK widgets, one widget per cell.
   See examples/table-as-container.cxx. \em Not recommended for large tables.
 
 \image html table-simple.png
 \image latex table-simple.png "table-simple example" width=6cm

 \image html table-as-container.png
 \image latex table-as-container.png "table-as-container example" width=6cm
 
 When acting as part of a custom widget, events on the cells and/or headings
 generate callbacks when they are clicked by the user. You control when events 
 are generated based on the setting for Fl_Table::when().
 
 When acting as a container for FLTK widgets, the FLTK widgets maintain 
 themselves. Although the draw_cell() method must be overridden, its contents 
 can be very simple. See the draw_cell() code in examples/table-simple.cxx.
 
 The following variables are available to classes deriving from Fl_Table:
 
 \image html table-dimensions.png
 \image latex table-dimensions.png "Fl_Table Dimensions" width=6cm
 
 <table border=0>
 <tr><td>x()/y()/w()/h()</td>
 <td>Fl_Table widget's outer dimension. The outer edge of the border of the 
 Fl_Table. (Red in the diagram above)</td></tr>
 
 <tr><td>wix/wiy/wiw/wih</td>
 <td>Fl_Table widget's inner dimension. The inner edge of the border of the 
 Fl_Table. eg. if the Fl_Table's box() is FL_NO_BOX, these values are the same 
 as x()/y()/w()/h(). (Yellow in the diagram above)</td></tr>
 
 <tr><td>tox/toy/tow/toh</td>
 <td>The table's outer dimension. The outer edge of the border around the cells,
 but inside the row/col headings and scrollbars. (Green in the diagram above)
 </td></tr>
 
 <tr><td>tix/tiy/tiw/tih</td>
 <td>The table's inner dimension. The inner edge of the border around the cells,
 but inside the row/col headings and scrollbars. AKA the table's clip region. 
 eg. if the table_box() is FL_NO_BOX, these values are the same as
 tox/toyy/tow/toh. (Blue in the diagram above)
 </td></tr></table>
 
 CORE DEVELOPERS
 
 - Greg Ercolano : 12/16/2002 - initial implementation 12/16/02. Fl_Table, Fl_Table_Row, docs.
 - Jean-Marc Lienher : 02/22/2004 - added keyboard nav + mouse selection, and ported Fl_Table into fltk-utf8-1.1.4
 
 OTHER CONTRIBUTORS
 
 - Inspired by the Feb 2000 version of FLVW's Flvw_Table widget. Mucho thanks to those folks.
 - Mister Satan : 04/07/2003 - MinGW porting mods, and singleinput.cxx; a cool Fl_Input oriented spreadsheet example
 - Marek Paliwoda : 01/08/2003 - Porting mods for Borland
 - Ori Berger : 03/16/2006 - Optimizations for >500k rows/cols
 
 LICENSE
 
 Greg added the following license to the original distribution of Fl_Table. He 
 kindly gave his permission to integrate Fl_Table and Fl_Table_Row into FLTK,
 allowing FLTK license to apply while his widgets are part of the library.
 
 If used on its own, this is the license that applies:

 \verbatim 
 Fl_Table License
 December 16, 2002
 
 The Fl_Table library and included programs are provided under the terms
 of the GNU Library General Public License (LGPL) with the following
 exceptions:
 
 1. Modifications to the Fl_Table configure script, config
 header file, and makefiles by themselves to support
 a specific platform do not constitute a modified or
 derivative work.
 
 The authors do request that such modifications be
 contributed to the Fl_Table project - send all
 contributions to "erco at seriss dot com".
 
 2. Widgets that are subclassed from Fl_Table widgets do not
 constitute a derivative work.
 
 3. Static linking of applications and widgets to the
 Fl_Table library does not constitute a derivative work
 and does not require the author to provide source
 code for the application or widget, use the shared
 Fl_Table libraries, or link their applications or
 widgets against a user-supplied version of Fl_Table.
 
 If you link the application or widget to a modified
 version of Fl_Table, then the changes to Fl_Table must be
 provided under the terms of the LGPL in sections
 1, 2, and 4.
 
 4. You do not have to provide a copy of the Fl_Table license
 with programs that are linked to the Fl_Table library, nor
 do you have to identify the Fl_Table license in your
 program or documentation as required by section 6
 of the LGPL.
 
 However, programs must still identify their use of Fl_Table.
 The following example statement can be included in user
 documentation to satisfy this requirement:
 
 [program/widget] is based in part on the work of
 the Fl_Table project http://seriss.com/people/erco/fltk/Fl_Table/
 \endverbatim
 
 
 */
class FL_EXPORT Fl_Table : public Fl_Group {
public:
  /**
   The context bit flags for Fl_Table related callbacks (eg. draw_cell(), callback(), etc)
   */
  enum TableContext {
    CONTEXT_NONE       = 0,	///< no known context
    CONTEXT_STARTPAGE  = 0x01,	///< before a page is redrawn
    CONTEXT_ENDPAGE    = 0x02,	///< after a page is redrawn
    CONTEXT_ROW_HEADER = 0x04,	///< in the row header
    CONTEXT_COL_HEADER = 0x08,	///< in the col header
    CONTEXT_CELL       = 0x10,	///< in one of the cells
    CONTEXT_TABLE      = 0x20,	///< in a dead zone of table
    CONTEXT_RC_RESIZE  = 0x40 	///< column or row being resized
  };
  
private:
  int _rows, _cols;	// total rows/cols
  int _row_header_w;	// width of row header
  int _col_header_h;	// height of column header
  int _row_position;	// last row_position set (not necessarily == toprow!)
  int _col_position;	// last col_position set (not necessarily == leftcol!)
  
  char _row_header;	// row header enabled?
  char _col_header;	// col header enabled?
  char _row_resize;	// row resizing enabled?
  char _col_resize;	// col resizing enabled?
  int _row_resize_min;	// row minimum resizing height (default=1)
  int _col_resize_min;	// col minimum resizing width (default=1)
  
  // OPTIMIZATION: partial row/column redraw variables
  int _redraw_toprow;
  int _redraw_botrow;
  int _redraw_leftcol;
  int _redraw_rightcol;
  Fl_Color _row_header_color;
  Fl_Color _col_header_color;
  
  int _auto_drag;
  int _selecting;
  
  // An STL-ish vector without templates
  class FL_EXPORT IntVector {
    int *arr;
    unsigned int _size;
    void init() {
      arr = NULL;
      _size = 0;
    }
    void copy(int *newarr, unsigned int newsize) {
      size(newsize);
      memcpy(arr, newarr, newsize * sizeof(int));
    }
  public:
    IntVector() { init(); }					// CTOR
    ~IntVector() { if ( arr ) free(arr); arr = NULL; }		// DTOR
    IntVector(IntVector&o) { init(); copy(o.arr, o._size); }	// COPY CTOR
    IntVector& operator=(IntVector&o) {				// ASSIGN
      init();
      copy(o.arr, o._size);
      return(*this);
    }
    int operator[](int x) const { return(arr[x]); }
    int& operator[](int x) { return(arr[x]); }
    unsigned int size() { return(_size); }
    void size(unsigned int count) {
      if ( count != _size ) {
        arr = (int*)realloc(arr, count * sizeof(int));
        _size = count;
      }
    }
    int pop_back() { int tmp = arr[_size-1]; _size--; return(tmp); }
    void push_back(int val) { unsigned int x = _size; size(_size+1); arr[x] = val; }
    int back() { return(arr[_size-1]); }
  };
  
  IntVector _colwidths;			// column widths in pixels
  IntVector _rowheights;		// row heights in pixels
  
  Fl_Cursor _last_cursor;		// last mouse cursor before changed to 'resize' cursor
  
  // EVENT CALLBACK DATA
  TableContext _callback_context;	// event context
  int _callback_row, _callback_col;	// event row/col
  
  // handle() state variables.
  //    Put here instead of local statics in handle(), so more
  //    than one Fl_Table can exist without crosstalk between them.
  //
  int _resizing_col;			// column being dragged
  int _resizing_row;			// row being dragged
  int _dragging_x;			// starting x position for horiz drag
  int _dragging_y;			// starting y position for vert drag
  int _last_row;			// last row we FL_PUSH'ed
  
  // Redraw single cell
  void _redraw_cell(TableContext context, int R, int C);
  
  void _start_auto_drag();
  void _stop_auto_drag();
  void _auto_drag_cb();
  static void _auto_drag_cb2(void *d);
  
protected:
  enum ResizeFlag {
    RESIZE_NONE      = 0,
    RESIZE_COL_LEFT  = 1,
    RESIZE_COL_RIGHT = 2,
    RESIZE_ROW_ABOVE = 3,
    RESIZE_ROW_BELOW = 4
  };
  
  int table_w, table_h;				// table's virtual size (in pixels)
  int toprow, botrow, leftcol, rightcol;	// four corners of viewable table
  
  // selection
  int current_row, current_col;
  int select_row, select_col;
  
  // OPTIMIZATION: Precomputed scroll positions for the toprow/leftcol
  int toprow_scrollpos;
  int leftcol_scrollpos;
  
  // Dimensions
  int tix, tiy, tiw, tih;			// data table inner dimension xywh
  int tox, toy, tow, toh;			// data table outer dimension xywh
  int wix, wiy, wiw, wih;			// widget inner dimension xywh
  
  Fl_Scroll *table;				// container for child fltk widgets (if any)
  Fl_Scrollbar *vscrollbar; 			// vertical scrollbar
  Fl_Scrollbar *hscrollbar;			// horizontal scrollbar
  
  // Fltk
  int handle(int e);				// fltk handle() override
  
  // Class maintenance
  void recalc_dimensions();
  void table_resized();				// table resized; recalc
  void table_scrolled();			// table scrolled; recalc
  void get_bounds(TableContext context,		// return x/y/w/h bounds for context
                  int &X, int &Y, int &W, int &H);
  void change_cursor(Fl_Cursor newcursor);	// change mouse cursor to some other shape
  TableContext cursor2rowcol(int &R, int &C, ResizeFlag &resizeflag);
  // find r/c given current x/y event
  int find_cell(TableContext context,		// find cell's x/y/w/h given r/c
                int R, int C, int &X, int &Y, int &W, int &H);
  int row_col_clamp(TableContext context, int &R, int &C);
  // clamp r/c to known universe
  
  /**
   Subclass should override this method to handle drawing the cells.
   
   This method will be called whenever the table is redrawn, once per cell.
   
   Only cells that are completely (or partially) visible will be told to draw.
   
   \p context will be one of the following:
   
   <table border=1>
   <tr>
   <td>\p Fl_Table::CONTEXT_STARTPAGE</td>
   <td>When table, or parts of the table, are about to be redrawn.<br>
       Use to initialize static data, such as font selections.<p>
       R/C will be zero,<br>
       X/Y/W/H will be the dimensions of the table's entire data area.<br>
       (Useful for locking a database before accessing; see
       also visible_cells())</td>
   </tr><tr>
   <td>\p Fl_Table::CONTEXT_ENDPAGE</td>
   <td>When table has completed being redrawn.<br>
       R/C will be zero, X/Y/W/H dimensions of table's data area.<br>
       (Useful for unlocking a database after accessing)</td>
   </tr><tr>
   <td>\p Fl_Table::CONTEXT_ROW_HEADER</td>
   <td>Whenever a row header cell needs to be drawn.<br>
       R will be the row number of the header being redrawn,<br>
       C will be zero,<br>
       X/Y/W/H will be the fltk drawing area of the row header in the window </td>
   </tr><tr>
   <td>\p Fl_Table::CONTEXT_COL_HEADER</td>
   <td>Whenever a column header cell needs to be drawn.<br>
       R will be zero, <br>
       C will be the column number of the header being redrawn,<br>
       X/Y/W/H will be the fltk drawing area of the column header in the window </td>
   </tr><tr>
   <td>\p Fl_Table::CONTEXT_CELL</td>
   <td>Whenever a data cell in the table needs to be drawn.<br>
       R/C will be the row/column of the cell to be drawn,<br>
       X/Y/W/H will be the fltk drawing area of the cell in the window </td>
   </tr><tr>
   <td>\p Fl_Table::CONTEXT_RC_RESIZE</td>
   <td>Whenever table or row/column is resized or scrolled,
       either interactively or via col_width() or row_height().<br>
       R/C/X/Y/W/H will all be zero.
       <p> 
       Useful for fltk containers that need to resize or move
       the child fltk widgets.</td>
   </tr>
   </table>
   
   \p row and \p col will be set to the row and column number
   of the cell being drawn. In the case of row headers, \p col will be \a 0.
   In the case of column headers, \p row will be \a 0.
   
   <tt>x/y/w/h</tt> will be the position and dimensions of where the cell
   should be drawn.
   
   In the case of custom widgets, a minimal draw_cell() override might
   look like the following. With custom widgets it is up to the caller to handle
   drawing everything within the dimensions of the cell, including handling the
   selection color.  Note all clipping must be handled as well; this allows drawing
   outside the dimensions of the cell if so desired for 'custom effects'.
   
   \code
   // This is called whenever Fl_Table wants you to draw a cell
   void MyTable::draw_cell(TableContext context, int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0) {
       static char s[40];
       sprintf(s, "%d/%d", R, C);              // text for each cell
       switch ( context ) {
	   case CONTEXT_STARTPAGE:             // Fl_Table telling us its starting to draw page
	       fl_font(FL_HELVETICA, 16);
	       return;
       
	   case CONTEXT_ROW_HEADER:            // Fl_Table telling us it's draw row/col headers
	   case CONTEXT_COL_HEADER:
	       fl_push_clip(X, Y, W, H);
	       {
		   fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, color());
		   fl_color(FL_BLACK);
		   fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
	       }
	       fl_pop_clip();
	       return;
	   
	   case CONTEXT_CELL:                  // Fl_Table telling us to draw cells
	       fl_push_clip(X, Y, W, H);
	       {
		   // BG COLOR
		   fl_color( row_selected(R) ? selection_color() : FL_WHITE);
		   fl_rectf(X, Y, W, H);
		   
		   // TEXT
		   fl_color(FL_BLACK);
		   fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
		   
		   // BORDER
		   fl_color(FL_LIGHT2);
		   fl_rect(X, Y, W, H);
	       }
	       fl_pop_clip();
	       return;
	   
       default:
	   return;
       }
       //NOTREACHED
   }
   \endcode
   */
  virtual void draw_cell(TableContext context, int R=0, int C=0, 
                         int X=0, int Y=0, int W=0, int H=0)
  { }						// overridden by deriving class
  
  long row_scroll_position(int row);		// find scroll position of row (in pixels)
  long col_scroll_position(int col);		// find scroll position of col (in pixels)
  
  int is_fltk_container() { 			// does table contain fltk widgets?
    return( Fl_Group::children() > 3 );		// (ie. more than box and 2 scrollbars?)
  }
  
  static void scroll_cb(Fl_Widget*,void*);	// h/v scrollbar callback
  
  void damage_zone(int r1, int c1, int r2, int c2, int r3 = 0, int c3 = 0);
  
  void redraw_range(int toprow, int botrow, int leftcol, int rightcol) {
    if ( _redraw_toprow == -1 ) {
      // Initialize redraw range
      _redraw_toprow = toprow;
      _redraw_botrow = botrow;
      _redraw_leftcol = leftcol;
      _redraw_rightcol = rightcol;
    } else {
      // Extend redraw range
      if ( toprow < _redraw_toprow ) _redraw_toprow = toprow;
      if ( botrow > _redraw_botrow ) _redraw_botrow = botrow;
      if ( leftcol < _redraw_leftcol ) _redraw_leftcol = leftcol;
      if ( rightcol > _redraw_rightcol ) _redraw_rightcol = rightcol;
    }
    
    // Indicate partial redraw needed of some cells
    damage(FL_DAMAGE_CHILD);
  }
  
public:
  /**
   The constructor for the Fl_Table.
   This creates an empty table with no rows or columns,
   with headers and row/column resize behavior disabled.
   */
  Fl_Table(int X, int Y, int W, int H, const char *l=0);
  
  /**
   The destructor for the Fl_Table.
   Destroys the table and its associated widgets.
   */
  ~Fl_Table();
  
  /**
   Clears the table to zero rows, zero columns.
   Same as rows(0); cols(0);
   \see rows(int), cols(int)
   */
  virtual void clear() { rows(0); cols(0); }
  
  // \todo: add topline(), middleline(), bottomline()
  
  /**
   Sets the kind of box drawn around the data table,
   the default being FL_NO_BOX. Changing this value will cause the table
   to redraw.
   */
  inline void table_box(Fl_Boxtype val) {
    table->box(val);
    table_resized();
  }
  
  /**
   Returns the current box type used for the data table.
   */
  inline Fl_Boxtype table_box( void ) {
    return(table->box());
  }
  
  /**
   Sets the number of rows in the table, and the table is redrawn.
   */
  virtual void rows(int val);			// set/get number of rows
  
  /**
   Returns the number of rows in the table.
   */
  inline int rows() {
    return(_rows);
  }
  
  /**
   Set the number of columns in the table and redraw.
   */
  virtual void cols(int val);			// set/get number of columns
  
  /** 
   Get the number of columns in the table.
   */
  inline int cols() {
    return(_cols);
  }
  
  /**
   Returns the range of row and column numbers for all visible 
   and partially visible cells in the table.
   
   These values can be used e.g. by your draw_cell() routine during
   CONTEXT_STARTPAGE to figure out what cells are about to be redrawn
   for the purposes of locking the data from a database before it's drawn.
   
   \code
          leftcol             rightcol
             :                   :
   toprow .. .-------------------.
             |                   |
             |  V I S I B L E    |
             |                   |
             |    T A B L E      |
             |                   |
   botrow .. '-------------------`
   \endcode
   
   e.g. in a table where the visible rows are 5-20, and the
   visible columns are 100-120, then those variables would be:
   
   - toprow = 5
   - botrow = 20
   - leftcol = 100
   - rightcol = 120
   */
  inline void visible_cells(int& r1, int& r2, int& c1, int& c2) {
    r1 = toprow;
    r2 = botrow;
    c1 = leftcol;
    c2 = rightcol;
  } 
  
  /**
   Returns 1 if someone is interactively resizing a row or column.
   You can currently call this only from within your callback().
   */
  int is_interactive_resize() {
    return(_resizing_row != -1 || _resizing_col != -1);
  } 
  
  /**
   Returns the current value of this flag.
   */
  inline int row_resize() {
    return(_row_resize);
  }
  
  /**
   Allows/disallows row resizing by the user.
   1=allow interactive resizing, 0=disallow interactive resizing.
   Since interactive resizing is done via the row headers,
   row_header() must also be enabled to allow resizing.
   */   
  void row_resize(int flag) {			// enable row resizing
    _row_resize = flag;
  }
  
  /**
   Returns the current value of this flag.
   */
  inline int col_resize() {
    return(_col_resize);
  }
  /**
   Allows/disallows column resizing by the user.
   1=allow interactive resizing, 0=disallow interactive resizing.
   Since interactive resizing is done via the column headers,
   \p col_header() must also be enabled to allow resizing.
   */
  void col_resize(int flag) {			// enable col resizing
    _col_resize = flag;
  }
  
  /**
   Sets the current column minimum resize value.
   This is used to prevent the user from interactively resizing
   any column to be smaller than 'pixels'. Must be a value >=1.
   */
  inline int col_resize_min() {			// column minimum resizing width
    return(_col_resize_min);
  }
  
  /**
   Returns the current column minimum resize value.
   */
  void col_resize_min(int val) {
    _col_resize_min = ( val < 1 ) ? 1 : val;
  } 
  
  /**
   Returns the current row minimum resize value.
   */
  inline int row_resize_min() {			// column minimum resizing width
    return(_row_resize_min);
  }
  
  /**
   Sets the current row minimum resize value.
   This is used to prevent the user from interactively resizing
   any row to be smaller than 'pixels'. Must be a value >=1.
   */
  void row_resize_min(int val) {
    _row_resize_min = ( val < 1 ) ? 1 : val;
  }
  
  /**
   Returns the value of this flag.
   */
  inline int row_header() {			// set/get row header enable flag
    return(_row_header);
  }
  
  /**
   Enables/disables showing the row headers. 1=enabled, 0=disabled.
   If changed, the table is redrawn.
   */
  void row_header(int flag) {
    _row_header = flag;
    table_resized();
    redraw();
  }
  
  /**
   Returns if column headers are enabled or not.
   */
  inline int col_header() {			// set/get col header enable flag
    return(_col_header);
  }
  
  /**
   Enable or disable column headers.
   If changed, the table is redrawn.
   */
  void col_header(int flag) {
    _col_header = flag;
    table_resized();
    redraw();
  }
  
  /**
   Sets the height in pixels for column headers and redraws the table.
   */
  inline void col_header_height(int height) {	// set/get col header height
    _col_header_h = height;
    table_resized();
    redraw();
  }
  
  /**
   Gets the column header height.
   */
  inline int col_header_height() {
    return(_col_header_h);
  }
  
  /**
   Sets the row header width to n and causes the screen to redraw.
   */
  inline void row_header_width(int width) {	// set/get row header width
    _row_header_w = width;
    table_resized();
    redraw();
  }
  
  /**
   Returns the current row header width (in pixels).
   */
  inline int row_header_width() {
    return(_row_header_w);
  }
  
  /**
   Sets the row header color and causes the screen to redraw.
   */
  inline void row_header_color(Fl_Color val) {	// set/get row header color
    _row_header_color = val;
    redraw();
  }
  
  /**
   Returns the current row header color.
   */
  inline Fl_Color row_header_color() {
    return(_row_header_color);
  } 
  
  /**
   Sets the color for column headers and redraws the table.
   */
  inline void col_header_color(Fl_Color val) {	// set/get col header color
    _col_header_color = val;
    redraw();
  }
  
  /**
   Gets the color for column headers.
   */
  inline Fl_Color col_header_color() {
    return(_col_header_color);
  }
  
  /**
   Sets the height of the specified row in pixels,
   and the table is redrawn.
   callback() will be invoked with CONTEXT_RC_RESIZE
   if the row's height was actually changed, and when() is FL_WHEN_CHANGED.
   */
  void row_height(int row, int height);		// set/get row height
  
  /**
   Returns the current height of the specified row as a value in pixels.
   */
  inline int row_height(int row) {
    return((row<0 || row>=(int)_rowheights.size()) ? 0 : _rowheights[row]);
  }
  
  /**
   Sets the width of the specified column in pixels, and the table is redrawn.
   callback() will be invoked with CONTEXT_RC_RESIZE
   if the column's width was actually changed, and when() is FL_WHEN_CHANGED.
   */   
  void col_width(int col, int width);		// set/get a column's width
  
  /**
   Returns the current width of the specified column in pixels.
   */
  inline int col_width(int col) {
    return((col<0 || col>=(int)_colwidths.size()) ? 0 : _colwidths[col]);
  }
  
  /**
   Convenience method to set the height of all rows to the
   same value, in pixels. The screen is redrawn.
   */
  void row_height_all(int height) {		// set all row/col heights
    for ( int r=0; r<rows(); r++ ) {
      row_height(r, height);
    }
  }
  
  /**
   Convenience method to set the width of all columns to the
   same value, in pixels. The screen is redrawn.
   */
  void col_width_all(int width) {
    for ( int c=0; c<cols(); c++ ) {
      col_width(c, width);
    }
  }
  
  /**
   Sets the row scroll position to 'row', and causes the screen to redraw.
   */
  void row_position(int row);			// set/get table's current scroll position
  
  /** 
   Sets the column scroll position to column 'col', and causes the screen to redraw.
   */
  void col_position(int col);
  
  /**
   Returns the current row scroll position as a row number.
   */
  int row_position() {				// current row position
    return(_row_position);
  }
  
  /**
   Returns the current column scroll position as a column number.
   */
  int col_position() {				// current col position
    return(_col_position);
  }
  
  /**
   Sets which row should be at the top of the table,
   scrolling as necessary, and the table is redrawn. If the table
   cannot be scrolled that far, it is scrolled as far as possible.
   */
  inline void top_row(int row) {		// set/get top row (deprecated)
    row_position(row);
  }
  
  /**
   Returns the current top row shown in the table.
   This row may be partially obscured.
   */
  inline int top_row() {
    return(row_position());
  }
  int is_selected(int r, int c);		// selected cell
  void get_selection(int &row_top, int &col_left, int &row_bot, int &col_right);
  void set_selection(int row_top, int col_left, int row_bot, int col_right);
  int move_cursor(int R, int C);
  
  /**
   Changes the size of the Fl_Table, causing it to redraw.
   */
  void resize(int X, int Y, int W, int H);	// fltk resize() override
  void draw(void);				// fltk draw() override
  
  // This crashes sortapp() during init.
  //  void box(Fl_Boxtype val) {
  //    Fl_Group::box(val);
  //    if ( table ) {
  //      resize(x(), y(), w(), h());
  //    }
  //  }
  //  Fl_Boxtype box(void) const {
  //    return(Fl_Group::box());
  //  }
  
  // Child group
  void init_sizes() {
    table->init_sizes();
    table->redraw();
  }
  void add(Fl_Widget& w) {
    table->add(w);
  }
  void add(Fl_Widget* w) {
    table->add(w);
  }
  void insert(Fl_Widget& w, int n) {
    table->insert(w,n);
  }
  void insert(Fl_Widget& w, Fl_Widget* w2) {
    table->insert(w,w2);
  }
  void remove(Fl_Widget& w) {
    table->remove(w);
  }
  void begin() {
    table->begin();
  }
  void end() {
    table->end();
    // HACK: Avoid showing Fl_Scroll; seems to erase screen
    //       causing unnecessary flicker, even if its box() is FL_NO_BOX.
    //
    if ( table->children() > 2 ) {
      table->show();
    } else {
      table->hide();
    } 
    Fl_Group::current(Fl_Group::parent());
  }
  Fl_Widget * const *array() {
    return(table->array());
  }
  
  /**
   Returns the child widget by an index.
   
   When using the Fl_Table as a container for FLTK widgets, this method returns 
   the widget pointer from the internal array of widgets in the container.
   
   Typically used in loops, eg:
   \code
   for ( int i=0; i<children(); i++ ) {
     Fl_Widget *w = child(i);
     [..]
   }
   \endcode
   */
  Fl_Widget *child(int n) const {
    return(table->child(n));
  }
  
  /**
   Returns the number of children in the table.
   
   When using the Fl_Table as a container for FLTK widgets, this method returns 
   how many child widgets the table has.
   
   \see child(int)
   */
  int children() const {
    return(table->children()-2);    // -2: skip Fl_Scroll's h/v scrollbar widgets
  }
  int find(const Fl_Widget *w) const {
    return(table->find(w));
  }
  int find(const Fl_Widget &w) const {
    return(table->find(w));
  } 
  // CALLBACKS
  
  /**
   * Returns the current row the event occurred on.
   *
   * This function should only be used from within the user's callback function
   */
  int callback_row() {
    return(_callback_row);
  }
  
  /**
   * Returns the current column the event occurred on.
   *
   * This function should only be used from within the user's callback function
   */
  int callback_col() {
    return(_callback_col);
  }
  
  /**
   * Returns the current 'table context'.
   *
   * This function should only be used from within the user's callback function
   */
  TableContext callback_context() {
    return(_callback_context);
  }
  
  void do_callback(TableContext context, int row, int col) {
    _callback_context = context;
    _callback_row = row;
    _callback_col = col;
    Fl_Widget::do_callback();
  }
  
#if FL_DOXYGEN
  /**
   The Fl_Widget::when() function is used to set a group of flags, determining
   when the widget callback is called:
   
   <table border=1>
   <tr>
   <td>\p FL_WHEN_CHANGED</td>
   <td>
   callback() will be called when rows or columns are resized (interactively or 
   via col_width() or row_height()), passing CONTEXT_RC_RESIZE via 
   callback_context().
   </td>
   </tr><tr>
   <td>\p FL_WHEN_RELEASE</td>
   <td>
   callback() will be called during FL_RELEASE events, such as when someone 
   releases a mouse button somewhere on the table.
   </td>
   </tr>
   </table>
   
   The callback() routine is sent a TableContext that indicates the context the 
   event occurred in, such as in a cell, in a header, or elsewhere on the table.  
   When an event occurs in a cell or header, callback_row() and 
   callback_col() can be used to determine the row and column. The callback can 
   also look at the regular fltk event values (ie. Fl::event() and Fl::button()) 
   to determine what kind of event is occurring.
   */
  void when(Fl_When flags);
#endif
  
#if FL_DOXYGEN
  /**
   Callbacks will be called depending on the setting of Fl_Widget::when().
   
   Callback functions should use the following functions to determine the 
   context/row/column:
   
   * Fl_Table::callback_row() returns current row
   * Fl_Table::callback_col() returns current column
   * Fl_Table::callback_context() returns current table context
   
   callback_row() and callback_col() will be set to the row and column number the 
   event occurred on. If someone clicked on a row header, \p col will be \a 0.  
   If someone clicked on a column header, \p row will be \a 0.
   
   callback_context() will return one of the following:
   
   <table border=1>
   <tr><td><tt>Fl_Table::CONTEXT_ROW_HEADER</tt></td>
   <td>Someone clicked on a row header. Excludes resizing.</td>
   </tr><tr>
   <td><tt>Fl_Table::CONTEXT_COL_HEADER</tt></td>
   <td>Someone clicked on a column header. Excludes resizing.</td>
   </tr><tr>
   <td><tt>Fl_Table::CONTEXT_CELL</tt></td>
   <td>
   Someone clicked on a cell.
   
   To receive callbacks for FL_RELEASE events, you must set
   when(FL_WHEN_RELEASE).
   </td>
   </tr><tr>
   <td><tt>Fl_Table::CONTEXT_RC_RESIZE</tt></td>
   <td>
   Someone is resizing rows/columns either interactively,
   or via the col_width() or row_height() API.
   
   Use is_interactive_resize()
   to determine interactive resizing.
   
   If resizing a column, R=0 and C=column being resized.
   
   If resizing a row, C=0 and R=row being resized.
   
   NOTE: To receive resize events, you must set when(FL_WHEN_CHANGED).
   </td>
   </tr>
   </table>
   
   \code
   class MyTable : public Fl_Table {
     [..]
   private:
     // Handle events that happen on the table
     void event_callback2() {
       int R = callback_row(),                         // row where event occurred
       C = callback_col();                             // column where event occurred
       TableContext context = callback_context();      // which part of table
       fprintf(stderr, "callback: Row=%d Col=%d Context=%d Event=%d\n",
               R, C, (int)context, (int)Fl::event());
     }
   
     // Actual static callback
     static void event_callback(Fl_Widget*, void* data) {
       MyTable *o = (MyTable*)data;
       o-&gt;event_callback2();
     }
     
   public:
     // Constructor
     MyTable() {
       [..]
       table.callback(&event_callback, (void*)this);   // setup callback
       table.when(FL_WHEN_CHANGED|FL_WHEN_RELEASE);    // when to call it
     }
   };
   \endcode
   */
  void callback(Fl_Widget*, void*);
#endif
};

#endif /*_FL_TABLE_H*/

//
// End of "$Id: Fl_Table.H 8301 2011-01-22 22:40:11Z AlbrechtS $".
//
