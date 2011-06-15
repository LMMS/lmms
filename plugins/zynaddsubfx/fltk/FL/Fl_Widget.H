//
// "$Id: Fl_Widget.H 8623 2011-04-24 17:09:41Z AlbrechtS $"
//
// Widget header file for the Fast Light Tool Kit (FLTK).
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

/** \file
   Fl_Widget, Fl_Label classes . */

#ifndef Fl_Widget_H
#define Fl_Widget_H

#include "Enumerations.H"

/**
  \todo	typedef's fl_intptr_t and fl_uintptr_t should be documented.
*/
#ifdef _WIN64
#ifdef __GNUC__
#include <stdint.h>
#else
#include <stddef.h>  // M$VC
#endif
typedef intptr_t fl_intptr_t;
typedef uintptr_t fl_uintptr_t;
#else
typedef long fl_intptr_t;
typedef unsigned long fl_uintptr_t;
#endif

class Fl_Widget;
class Fl_Window;
class Fl_Group;
class Fl_Image;

/** Default callback type definition for all fltk widgets (by far the most used) */
typedef void (Fl_Callback )(Fl_Widget*, void*);
/** Default callback type pointer definition for all fltk widgets */
typedef Fl_Callback* Fl_Callback_p; // needed for BORLAND
/** One parameter callback type definition passing only the widget */
typedef void (Fl_Callback0)(Fl_Widget*);
/** Callback type definition passing the widget and a long data value */
typedef void (Fl_Callback1)(Fl_Widget*, long);

/** This struct stores all information for a text or mixed graphics label.

    \todo For FLTK 1.3, the Fl_Label type will become a widget by itself. That way
          we will be avoiding a lot of code duplication by handling labels in 
          a similar fashion to widgets containing text. We also provide an easy
          interface for very complex labels, containing html or vector graphics.
 */
struct FL_EXPORT Fl_Label {
  /** label text */
  const char* value;
  /** optional image for an active label */
  Fl_Image* image;
  /** optional image for a deactivated label */
  Fl_Image* deimage;
  /** label font used in text */
  Fl_Font font;
  /** size of label font */
  Fl_Fontsize size;
  /** text color */
  Fl_Color color;
  /** alignment of label */
  Fl_Align align_;
  /** type of label. \see Fl_Labeltype */
  uchar type;

  /** Draws the label aligned to the given box */
  void draw(int,int,int,int, Fl_Align) const ;
  void measure(int &w, int &h) const ;
};


/** Fl_Widget is the base class for all widgets in FLTK.  
  
    You can't create one of these because the constructor is not public.
    However you can subclass it.  

    All "property" accessing methods, such as color(), parent(), or argument() 
    are implemented as trivial inline functions and thus are as fast and small 
    as accessing fields in a structure. Unless otherwise noted, the property 
    setting methods such as color(n) or label(s) are also trivial inline 
    functions, even if they change the widget's appearance. It is up to the 
    user code to call redraw() after these.
 */
class FL_EXPORT Fl_Widget {
  friend class Fl_Group;

  Fl_Group* parent_;
  Fl_Callback* callback_;
  void* user_data_;
  int x_,y_,w_,h_;
  Fl_Label label_;
  unsigned int flags_;
  Fl_Color color_;
  Fl_Color color2_;
  uchar type_;
  uchar damage_;
  uchar box_;
  uchar when_;

  const char *tooltip_;

  /** unimplemented copy ctor */
  Fl_Widget(const Fl_Widget &);
  /** unimplemented assignment operator */
  Fl_Widget& operator=(const Fl_Widget &);

protected:

  /** Creates a widget at the given position and size.

      The Fl_Widget is a protected constructor, but all derived widgets have a 
      matching public constructor. It takes a value for x(), y(), w(), h(), and 
      an optional value for label().
    
      \param[in] x, y the position of the widget relative to the enclosing window
      \param[in] w, h size of the widget in pixels
      \param[in] label optional text for the widget label
   */
  Fl_Widget(int x, int y, int w, int h, const char *label=0L);

  /** Internal use only. Use position(int,int), size(int,int) or resize(int,int,int,int) instead. */
  void x(int v) {x_ = v;}
  /** Internal use only. Use position(int,int), size(int,int) or resize(int,int,int,int) instead. */
  void y(int v) {y_ = v;}
  /** Internal use only. Use position(int,int), size(int,int) or resize(int,int,int,int) instead. */
  void w(int v) {w_ = v;}
  /** Internal use only. Use position(int,int), size(int,int) or resize(int,int,int,int) instead. */
  void h(int v) {h_ = v;}
  /** Gets the widget flags mask */
  unsigned int flags() const {return flags_;}
  /** Sets a flag in the flags mask */
  void set_flag(unsigned int c) {flags_ |= c;}
  /** Clears a flag in the flags mask */
  void clear_flag(unsigned int c) {flags_ &= ~c;}
  /** flags possible values enumeration.
      See activate(), output(), visible(), changed(), set_visible_focus()
  */
  enum {
        INACTIVE        = 1<<0,   ///< the widget can't receive focus, and is disabled but potentially visible
        INVISIBLE       = 1<<1,   ///< the widget is not drawn, but can receive a few special events
        OUTPUT          = 1<<2,   ///< for output only
        NOBORDER        = 1<<3,   ///< don't draw a decoration (Fl_Window)
        FORCE_POSITION  = 1<<4,   ///< don't let the window manager position the window (Fl_Window)
        NON_MODAL       = 1<<5,   ///< this is a hovering toolbar window (Fl_Window)
        SHORTCUT_LABEL  = 1<<6,   ///< the label contains a shortcut we need to draw
        CHANGED         = 1<<7,   ///< the widget value changed
        OVERRIDE        = 1<<8,   ///< position window on top (Fl_Window)
        VISIBLE_FOCUS   = 1<<9,   ///< accepts keyboard focus navigation if the widget can have the focus
        COPIED_LABEL    = 1<<10,  ///< the widget label is internally copied, its destruction is handled by the widget
        CLIP_CHILDREN   = 1<<11,  ///< all drawing within this widget will be clipped (Fl_Group)
        MENU_WINDOW     = 1<<12,  ///< a temporary popup window, dismissed by clicking outside (Fl_Window)
        TOOLTIP_WINDOW  = 1<<13,  ///< a temporary popup, transparent to events, and dismissed easily (Fl_Window)
        MODAL           = 1<<14,  ///< a window blocking input to all other winows (Fl_Window)
        NO_OVERLAY      = 1<<15,  ///< window not using a hardware overlay plane (Fl_Menu_Window)
        GROUP_RELATIVE  = 1<<16,  ///< position this widget relative to the parent group, not to the window
        COPIED_TOOLTIP  = 1<<17,  ///< the widget tooltip is internally copied, its destruction is handled by the widget
        // (space for more flags)
        USERFLAG3       = 1<<29,  ///< reserved for 3rd party extensions
        USERFLAG2       = 1<<30,  ///< reserved for 3rd party extensions
        USERFLAG1       = 1<<31   ///< reserved for 3rd party extensions
  };
  void draw_box() const;
  void draw_box(Fl_Boxtype t, Fl_Color c) const;
  void draw_box(Fl_Boxtype t, int x,int y,int w,int h, Fl_Color c) const;
  void draw_backdrop() const;
  /** draws a focus rectangle around the widget */
  void draw_focus() {draw_focus(box(),x(),y(),w(),h());}
  void draw_focus(Fl_Boxtype t, int x,int y,int w,int h) const;
  void draw_label() const;
  void draw_label(int, int, int, int) const;

public:

  /** Destroys the widget.
      Destroying single widgets is not very common. You almost always want to 
      destroy the parent group instead, which will destroy all of the child widgets 
      and groups in that group.
      
      \since FLTK 1.3, the widget's destructor removes the widget from its parent
      group, if it is member of a group.
   */
  virtual ~Fl_Widget();

  /** Draws the widget.
      Never call this function directly. FLTK will schedule redrawing whenever
      needed. If your widget must be redrawn as soon as possible, call redraw()
      instead.

      Override this function to draw your own widgets.

      If you ever need to call another widget's draw method <I>from within your
      own draw() method</I>, e.g. for an embedded scrollbar, you can do it
      (because draw() is virtual) like this:

      \code
        Fl_Widget *s = &scroll;		// scroll is an embedded Fl_Scrollbar
	s->draw();			// calls Fl_Scrollbar::draw()
      \endcode
   */
  virtual void draw() = 0;

  /** Handles the specified event. 
      You normally don't call this method directly, but instead let FLTK do 
      it when the user interacts with the widget.
     
      When implemented in a widget, this function must return 0 if the 
      widget does not use the event or 1 otherwise.

      Most of the time, you want to call the inherited handle() method in 
      your overridden method so that you don't short-circuit events that you 
      don't handle. In this last case you should return the callee retval.
    
      \param[in] event the kind of event received
      \retval 0 if the event was not used or understood
      \retval 1 if the event was used and can be deleted
      \see Fl_Event
   */
  virtual int handle(int event);

  /** Returns a pointer to the parent widget.  
      Usually this is a Fl_Group or Fl_Window. 
      \retval NULL if the widget has no parent
      \see Fl_Group::add(Fl_Widget*)
   */
  Fl_Group* parent() const {return parent_;}

  /** Internal use only - "for hacks only".
  
      It is \em \b STRONGLY recommended not to use this method, because it
      short-circuits Fl_Group's normal widget adding and removing methods,
      if the widget is already a child widget of another Fl_Group.

      Use Fl_Group::add(Fl_Widget*) and/or Fl_Group::remove(Fl_Widget*) instead.
  */
  void parent(Fl_Group* p) {parent_ = p;} // for hacks only, use Fl_Group::add()

  /** Gets the widget type.
      Returns the widget type value, which is used for Forms compatibility
      and to simulate RTTI.
      
      \todo Explain "simulate RTTI" (currently only used to decide if a widget
      is a window, i.e. type()>=FL_WINDOW ?). Is type() really used in a way
      that ensures "Forms compatibility" ?
   */
  uchar type() const {return type_;}

  /** Sets the widget type.
      This is used for Forms compatibility.
   */
  void type(uchar t) {type_ = t;}

  /** Gets the widget position in its window.
      \return the x position relative to the window
   */
  int x() const {return x_;}

  /** Gets the widget position in its window.
      \return the y position relative to the window
   */
  int y() const {return y_;}

  /** Gets the widget width.
      \return the width of the widget in pixels.
   */
  int w() const {return w_;}

  /** Gets the widget height.
      \return the height of the widget in pixels.
   */
  int h() const {return h_;}

  /** Changes the size or position of the widget.

      This is a virtual function so that the widget may implement its 
      own handling of resizing. The default version does \e not
      call the redraw() method, but instead relies on the parent widget 
      to do so because the parent may know a faster way to update the 
      display, such as scrolling from the old position.  

      Some window managers under X11 call resize() a lot more often 
      than needed. Please verify that the position or size of a widget 
      did actually change before doing any extensive calculations.

      position(X, Y) is a shortcut for resize(X, Y, w(), h()), 
      and size(W, H) is a shortcut for resize(x(), y(), W, H).
    
      \param[in] x, y new position relative to the parent window 
      \param[in] w, h new size
      \see position(int,int), size(int,int)
   */
  virtual void resize(int x, int y, int w, int h);

  /** Internal use only. */
  int damage_resize(int,int,int,int);

  /** Repositions the window or widget.

      position(X, Y) is a shortcut for resize(X, Y, w(), h()).
    
      \param[in] X, Y new position relative to the parent window 
      \see resize(int,int,int,int), size(int,int)
   */
  void position(int X,int Y) {resize(X,Y,w_,h_);}

  /** Changes the size of the widget.

      size(W, H) is a shortcut for resize(x(), y(), W, H).
    
      \param[in] W, H new size
      \see position(int,int), resize(int,int,int,int)
   */
  void size(int W,int H) {resize(x_,y_,W,H);}

  /** Gets the label alignment.

      \return label alignment
      \see label(), align(Fl_Align), Fl_Align
   */
  Fl_Align align() const {return label_.align_;}

  /** Sets the label alignment.
      This controls how the label is displayed next to or inside the widget. 
      The default value is FL_ALIGN_CENTER, which centers the label inside 
      the widget.
      \param[in] alignment new label alignment
      \see align(), Fl_Align
   */
  void align(Fl_Align alignment) {label_.align_ = alignment;}

  /** Gets the box type of the widget.
      \return the current box type
      \see box(Fl_Boxtype), Fl_Boxtype
   */
  Fl_Boxtype box() const {return (Fl_Boxtype)box_;}
  
  /** Sets the box type for the widget. 
      This identifies a routine that draws the background of the widget.
      See Fl_Boxtype for the available types. The default depends on the 
      widget, but is usually FL_NO_BOX or FL_UP_BOX.
      \param[in] new_box the new box type
      \see box(), Fl_Boxtype
   */
  void box(Fl_Boxtype new_box) {box_ = new_box;}

  /** Gets the background color of the widget.
      \return current background color
      \see color(Fl_Color), color(Fl_Color, Fl_Color)
   */
  Fl_Color color() const {return color_;}

  /** Sets the background color of the widget. 
      The color is passed to the box routine. The color is either an index into 
      an internal table of RGB colors or an RGB color value generated using 
      fl_rgb_color().
      
      The default for most widgets is FL_BACKGROUND_COLOR. Use Fl::set_color()
      to redefine colors in the color map.
      \param[in] bg background color
      \see color(), color(Fl_Color, Fl_Color), selection_color(Fl_Color)
   */
  void color(Fl_Color bg) {color_ = bg;}

  /** Gets the selection color.
      \return the current selection color
      \see selection_color(Fl_Color), color(Fl_Color, Fl_Color)
   */
  Fl_Color selection_color() const {return color2_;}

  /** Sets the selection color.
      The selection color is defined for Forms compatibility and is usually 
      used to color the widget when it is selected, although some widgets 
      use this color for other purposes. You can set both colors at once 
      with color(Fl_Color bg, Fl_Color sel).
      \param[in] a the new selection color
      \see selection_color(), color(Fl_Color, Fl_Color)
   */
  void selection_color(Fl_Color a) {color2_ = a;}

  /** Sets the background and selection color of the widget. 

      The two color form sets both the background and selection colors. 
      \param[in] bg background color
      \param[in] sel selection color
      \see color(unsigned), selection_color(unsigned)
   */
  void color(Fl_Color bg, Fl_Color sel) {color_=bg; color2_=sel;}

  /** Gets the current label text.
      \return a pointer to the current label text
      \see label(const char *), copy_label(const char *)
   */
  const char* label() const {return label_.value;}

  /** Sets the current label pointer.

      The label is shown somewhere on or next to the widget. The passed pointer 
      is stored unchanged in the widget (the string is \em not copied), so if 
      you need to set the label to a formatted value, make sure the buffer is 
      static, global, or allocated. The copy_label() method can be used 
      to make a copy of the label string automatically.
      \param[in] text pointer to new label text
      \see copy_label()
   */
  void label(const char* text);

  /** Sets the current label. 
      Unlike label(), this method allocates a copy of the label 
      string instead of using the original string pointer.

      The internal copy will automatically be freed whenever you assign
      a new label or when the widget is destroyed.

      \param[in] new_label the new label text
      \see label()
   */
  void copy_label(const char *new_label);

  /** Shortcut to set the label text and type in one call.
      \see label(const char *), labeltype(Fl_Labeltype)
   */
  void label(Fl_Labeltype a, const char* b) {label_.type = a; label_.value = b;}

  /** Gets the label type.
      \return the current label type.
      \see Fl_Labeltype
   */
  Fl_Labeltype labeltype() const {return (Fl_Labeltype)label_.type;}

  /** Sets the label type. 
      The label type identifies the function that draws the label of the widget. 
      This is generally used for special effects such as embossing or for using 
      the label() pointer as another form of data such as an icon. The value 
      FL_NORMAL_LABEL prints the label as plain text.
      \param[in] a new label type
      \see Fl_Labeltype
   */
  void labeltype(Fl_Labeltype a) {label_.type = a;}

  /** Gets the label color. 
      The default color is FL_FOREGROUND_COLOR. 
      \return the current label color
   */
  Fl_Color labelcolor() const {return label_.color;}

  /** Sets the label color. 
      The default color is FL_FOREGROUND_COLOR. 
      \param[in] c the new label color
   */
  void labelcolor(Fl_Color c) {label_.color=c;}

  /** Gets the font to use. 
      Fonts are identified by indexes into a table. The default value
      uses a Helvetica typeface (Arial for Microsoft&reg; Windows&reg;).
      The function Fl::set_font() can define new typefaces.
      \return current font used by the label
      \see Fl_Font
   */
  Fl_Font labelfont() const {return label_.font;}

  /** Sets the font to use. 
      Fonts are identified by indexes into a table. The default value
      uses a Helvetica typeface (Arial for Microsoft&reg; Windows&reg;).
      The function Fl::set_font() can define new typefaces.
      \param[in] f the new font for the label
      \see Fl_Font
   */
  void labelfont(Fl_Font f) {label_.font=f;}

  /** Gets the font size in pixels. 
      The default size is 14 pixels.
      \return the current font size
   */
  Fl_Fontsize labelsize() const {return label_.size;}

  /** Sets the font size in pixels.
      \param[in] pix the new font size
      \see Fl_Fontsize labelsize()
   */
  void labelsize(Fl_Fontsize pix) {label_.size=pix;}

  /** Gets the image that is used as part of the widget label.
      This image is used when drawing the widget in the active state.
      \return the current image
   */
  Fl_Image* image() {return label_.image;}
  const Fl_Image* image() const {return label_.image;}

  /** Sets the image to use as part of the widget label.
      This image is used when drawing the widget in the active state.
      \param[in] img the new image for the label 
   */
  void image(Fl_Image* img) {label_.image=img;}

  /** Sets the image to use as part of the widget label.
      This image is used when drawing the widget in the active state.
      \param[in] img the new image for the label 
   */
  void image(Fl_Image& img) {label_.image=&img;}

  /** Gets the image that is used as part of the widget label.  
      This image is used when drawing the widget in the inactive state.
      \return the current image for the deactivated widget
   */
  Fl_Image* deimage() {return label_.deimage;}
  const Fl_Image* deimage() const {return label_.deimage;}

  /** Sets the image to use as part of the widget label.  
      This image is used when drawing the widget in the inactive state.
      \param[in] img the new image for the deactivated widget
   */
  void deimage(Fl_Image* img) {label_.deimage=img;}

  /** Sets the image to use as part of the widget label.  
      This image is used when drawing the widget in the inactive state.
      \param[in] img the new image for the deactivated widget
   */
  void deimage(Fl_Image& img) {label_.deimage=&img;}

  /** Gets the current tooltip text.
      \return a pointer to the tooltip text or NULL
      \see tooltip(const char*), copy_tooltip(const char*)
   */
  const char *tooltip() const {return tooltip_;}

  void tooltip(const char *text);		// see Fl_Tooltip
  void copy_tooltip(const char *text);		// see Fl_Tooltip

  /** Gets the current callback function for the widget.
      Each widget has a single callback.
      \return current callback
   */
  Fl_Callback_p callback() const {return callback_;}

  /** Sets the current callback function for the widget.
      Each widget has a single callback.
      \param[in] cb new callback
      \param[in] p user data
   */
  void callback(Fl_Callback* cb, void* p) {callback_=cb; user_data_=p;}

  /** Sets the current callback function for the widget.
      Each widget has a single callback.
      \param[in] cb new callback
   */
  void callback(Fl_Callback* cb) {callback_=cb;}

  /** Sets the current callback function for the widget.
      Each widget has a single callback.
      \param[in] cb new callback
   */
  void callback(Fl_Callback0*cb) {callback_=(Fl_Callback*)cb;}

  /** Sets the current callback function for the widget.
      Each widget has a single callback.
      \param[in] cb new callback
      \param[in] p user data
   */
  void callback(Fl_Callback1*cb, long p=0) {callback_=(Fl_Callback*)cb; user_data_=(void*)p;}

  /** Gets the user data for this widget.
      Gets the current user data (void *) argument that is passed to the callback function.
      \return user data as a pointer
   */
  void* user_data() const {return user_data_;}

  /** Sets the user data for this widget.
      Sets the new user data (void *) argument that is passed to the callback function.
      \param[in] v new user data
   */
  void user_data(void* v) {user_data_ = v;}

  /** Gets the current user data (long) argument that is passed to the callback function.
   */
  long argument() const {return (long)(fl_intptr_t)user_data_;}

  /** Sets the current user data (long) argument that is passed to the callback function.
      \todo The user data value must be implemented using \em intptr_t or similar
      to avoid 64-bit machine incompatibilities.
   */
  void argument(long v) {user_data_ = (void*)v;}

  /** Returns the conditions under which the callback is called.

      You can set the flags with when(uchar), the default value is
      FL_WHEN_RELEASE.

      \return set of flags
      \see when(uchar)
   */
  Fl_When when() const {return (Fl_When)when_;}

  /** Sets the flags used to decide when a callback is called.

     This controls when callbacks are done. The following values are useful,
     the default value is FL_WHEN_RELEASE:
     
     \li 0: The callback is not done, but changed() is turned on.
     \li FL_WHEN_CHANGED: The callback is done each time the text is
         changed by the user.
     \li FL_WHEN_RELEASE: The callback will be done when this widget loses 
         the focus, including when the window is unmapped. This is a useful 
	 value for text fields in a panel where doing the callback on every
  	 change is wasteful. However the callback will also happen if the 
	 mouse is moved out of the window, which means it should not do 
	 anything visible (like pop up an error message).
	 You might do better setting this to zero, and scanning all the
	 items for changed() when the OK button on a panel is pressed.
     \li FL_WHEN_ENTER_KEY: If the user types the Enter key, the entire 
         text is selected, and the callback is done if the text has changed. 
	 Normally the Enter key will navigate to the next field (or insert 
	 a newline for a Fl_Multiline_Input) - this changes the behavior.
     \li FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED: The Enter key will do the
         callback even if the text has not changed. Useful for command fields.
      Fl_Widget::when() is a set of bitflags used by subclasses of 
      Fl_Widget to decide when to do the callback.

      If the value is zero then the callback is never done. Other values 
      are described  in the individual widgets. This field is in the base 
      class so that you can scan a panel and do_callback() on all the ones
      that don't do their own callbacks in response to an "OK" button.
      \param[in] i set of flags
   */
  void when(uchar i) {when_ = i;}

  /** Returns whether a widget is visible.
      \retval 0 if the widget is not drawn and hence invisible.
      \see show(), hide(), visible_r()
   */
  unsigned int visible() const {return !(flags_&INVISIBLE);}

  /** Returns whether a widget and all its parents are visible.
      \retval 0 if the widget or any of its parents are invisible.
      \see show(), hide(), visible()
   */
  int visible_r() const;

  /** Makes a widget visible.

      An invisible widget never gets redrawn and does not get keyboard
      or mouse events, but can receive a few other events like FL_SHOW.

      The visible() method returns true if the widget is set to be
      visible. The visible_r() method returns true if the widget and
      all of its parents are visible. A widget is only visible if
      visible() is true on it <I>and all of its parents</I>.

      Changing it will send FL_SHOW or FL_HIDE events to the widget.
      <I>Do not change it if the parent is not visible, as this
      will send false FL_SHOW or FL_HIDE events to the widget</I>.
      redraw() is called if necessary on this or the parent.

      \see hide(), visible(), visible_r()
   */
  virtual void show();
  
  /** Makes a widget invisible.
      \see show(), visible(), visible_r()
   */
  virtual void hide();

  /** Makes the widget visible. 
      You must still redraw the parent widget to see a change in the 
      window. Normally you want to use the show() method instead.
   */
  void set_visible() {flags_ &= ~INVISIBLE;}

  /** Hides the widget. 
      You must still redraw the parent to see a change in the window. 
      Normally you want to use the hide() method instead.
   */
  void clear_visible() {flags_ |= INVISIBLE;}

  /** Returns whether the widget is active.
      \retval 0 if the widget is inactive
      \see active_r(), activate(), deactivate()
   */
  unsigned int active() const {return !(flags_&INACTIVE);}

  /** Returns whether the widget and all of its parents are active. 
      \retval 0 if this or any of the parent widgets are inactive
      \see active(), activate(), deactivate()
   */
  int active_r() const;

  /** Activates the widget.
      Changing this value will send FL_ACTIVATE to the widget if 
      active_r() is true.
      \see active(), active_r(), deactivate()
   */
  void activate();

  /** Deactivates the widget.
      Inactive widgets will be drawn "grayed out", e.g. with less contrast 
      than the active widget. Inactive widgets will not receive any keyboard 
      or mouse button events. Other events (including FL_ENTER, FL_MOVE, 
      FL_LEAVE, FL_SHORTCUT, and others) will still be sent. A widget is 
      only active if active() is true on it <I>and all of its parents</I>.  

      Changing this value will send FL_DEACTIVATE to the widget if 
      active_r() is true.

      Currently you cannot deactivate Fl_Window widgets.

      \see activate(), active(), active_r()
   */
  void deactivate();

  /** Returns if a widget is used for output only.
      output() means the same as !active() except it does not change how the 
      widget is drawn. The widget will not receive any events. This is useful 
      for making scrollbars or buttons that work as displays rather than input 
      devices.
      \retval 0 if the widget is used for input and output
      \see set_output(), clear_output() 
   */
  unsigned int output() const {return (flags_&OUTPUT);}

  /** Sets a widget to output only.
      \see output(), clear_output() 
   */
  void set_output() {flags_ |= OUTPUT;}

  /** Sets a widget to accept input.
      \see set_output(), output() 
   */
  void clear_output() {flags_ &= ~OUTPUT;}

  /** Returns if the widget is able to take events.
      This is the same as (active() && !output() && visible())
      but is faster.
      \retval 0 if the widget takes no events
   */
  unsigned int takesevents() const {return !(flags_&(INACTIVE|INVISIBLE|OUTPUT));}

  /** 
      Checks if the widget value changed since the last callback.

      "Changed" is a flag that is turned on when the user changes the value 
      stored in the widget. This is only used by subclasses of Fl_Widget that 
      store values, but is in the base class so it is easier to scan all the 
      widgets in a panel and do_callback() on the changed ones in response 
      to an "OK" button.

      Most widgets turn this flag off when they do the callback, and when 
      the program sets the stored value.

     \retval 0 if the value did not change
     \see set_changed(), clear_changed()
   */
  unsigned int changed() const {return flags_&CHANGED;}

  /** Marks the value of the widget as changed.
      \see changed(), clear_changed()
   */
  void set_changed() {flags_ |= CHANGED;}

  /** Marks the value of the widget as unchanged.
      \see changed(), set_changed()
   */
  void clear_changed() {flags_ &= ~CHANGED;}

  /** Gives the widget the keyboard focus.
      Tries to make this widget be the Fl::focus() widget, by first sending 
      it an FL_FOCUS event, and if it returns non-zero, setting 
      Fl::focus() to this widget. You should use this method to 
      assign the focus to a widget.  
      \return true if the widget accepted the focus.
   */
  int take_focus();

  /** Enables keyboard focus navigation with this widget. 
      Note, however, that this will not necessarily mean that the widget
      will accept focus, but for widgets that can accept focus, this method
      enables it if it has been disabled.
      \see visible_focus(), clear_visible_focus(), visible_focus(int) 
   */
  void set_visible_focus() { flags_ |= VISIBLE_FOCUS; }

  /** Disables keyboard focus navigation with this widget. 
      Normally, all widgets participate in keyboard focus navigation.
      \see set_visible_focus(), visible_focus(), visible_focus(int) 
   */
  void clear_visible_focus() { flags_ &= ~VISIBLE_FOCUS; }

  /** Modifies keyboard focus navigation. 
      \param[in] v set or clear visible focus
      \see set_visible_focus(), clear_visible_focus(), visible_focus() 
   */
  void visible_focus(int v) { if (v) set_visible_focus(); else clear_visible_focus(); }

  /** Checks whether this widget has a visible focus.
      \retval 0 if this widget has no visible focus.
      \see visible_focus(int), set_visible_focus(), clear_visible_focus()
   */
  unsigned int  visible_focus() { return flags_ & VISIBLE_FOCUS; }

  /** Sets the default callback for all widgets.
      Sets the default callback, which puts a pointer to the widget on the queue 
      returned by Fl::readqueue(). You may want to call this from your own callback.
      \param[in] cb the new callback
      \param[in] d user data associated with that callback
      \see callback(), do_callback(), Fl::readqueue()
   */
  static void default_callback(Fl_Widget *cb, void *d);

  /** Calls the widget callback.
      Causes a widget to invoke its callback function with default arguments.
      \see callback()
   */
  void do_callback() {do_callback(this,user_data_);}

  /** Calls the widget callback.
      Causes a widget to invoke its callback function with arbitrary arguments.
      \param[in] o call the callback with \p o as the widget argument
      \param[in] arg call the callback with \p arg as the user data argument
      \see callback()
   */
  void do_callback(Fl_Widget* o,long arg) {do_callback(o,(void*)arg);}

  // Causes a widget to invoke its callback function with arbitrary arguments.
  // Documentation and implementation in Fl_Widget.cxx
  void do_callback(Fl_Widget* o,void* arg=0);

  /* Internal use only. */
  int test_shortcut();
  /* Internal use only. */
  static unsigned int label_shortcut(const char *t);
  /* Internal use only. */
  static int test_shortcut(const char*, const bool require_alt = false);

  /** Checks if w is a child of this widget.
      \param[in] w potential child widget
      \return Returns 1 if \p w is a child of this widget, or is
      equal to this widget. Returns 0 if \p w is NULL.
   */
  int contains(const Fl_Widget *w) const ;

  /** Checks if this widget is a child of w.
      Returns 1 if this widget is a child of \p w, or is
      equal to \p w. Returns 0 if \p w is NULL.
      \param[in] w the possible parent widget.
      \see contains()
   */
  int inside(const Fl_Widget* w) const {return w ? w->contains(this) : 0;}

  /** Schedules the drawing of the widget.
      Marks the widget as needing its draw() routine called.
   */
  void redraw();

  /** Schedules the drawing of the label.
     Marks the widget or the parent as needing a redraw for the label area 
     of a widget.
   */
  void redraw_label();

  /** Returns non-zero if draw() needs to be called. 
      The damage value is actually a bit field that the widget 
      subclass can use to figure out what parts to draw.
      \return a bitmap of flags describing the kind of damage to the widget
      \see damage(uchar), clear_damage(uchar)
   */
  uchar damage() const {return damage_;}

  /** Clears or sets the damage flags.
      Damage flags are cleared when parts of the widget drawing is repaired.

      The optional argument \p c specifies the bits that <b>are set</b>
      after the call (default: 0) and \b not the bits that are cleared!

      \note Therefore it is possible to set damage bits with this method, but
      this should be avoided. Use damage(uchar) instead.
      
      \param[in] c new bitmask of damage flags (default: 0)
      \see damage(uchar), damage()
   */
  void clear_damage(uchar c = 0) {damage_ = c;}

  /** Sets the damage bits for the widget.
      Setting damage bits will schedule the widget for the next redraw.
      \param[in] c bitmask of flags to set
      \see damage(), clear_damage(uchar)
   */
  void damage(uchar c);

  /** Sets the damage bits for an area inside the widget.
      Setting damage bits will schedule the widget for the next redraw.
      \param[in] c bitmask of flags to set
      \param[in] x, y, w, h size of damaged area
      \see damage(), clear_damage(uchar)
   */
  void damage(uchar c, int x, int y, int w, int h);

  void draw_label(int, int, int, int, Fl_Align) const;

  /** Sets width ww and height hh accordingly with the label size.
      Labels with images will return w() and h() of the image.
   */
  void measure_label(int& ww, int& hh) const {label_.measure(ww, hh);}

  /** Returns a pointer to the primary Fl_Window widget.
      \retval  NULL if no window is associated with this widget.  
      \note for an Fl_Window widget, this returns its <I>parent</I> window 
            (if any), not <I>this</I> window.
   */
  Fl_Window* window() const ;

  /** Returns an Fl_Group pointer if this widget is an Fl_Group.

      Use this method if you have a widget (pointer) and need to
      know whether this widget is derived from Fl_Group. If it returns
      non-NULL, then the widget in question is derived from Fl_Group,
      and you can use the returned pointer to access its children
      or other Fl_Group-specific methods.
      
      Example:
      \code
      void my_callback (Fl_Widget *w, void *) {
        Fl_Group *g = w->as_group();
	if (g)
	  printf ("This group has %d children\n",g->children());
	else
	  printf ("This widget is not a group!\n");
      }
      \endcode

      \retval NULL if this widget is not derived from Fl_Group.
      \note This method is provided to avoid dynamic_cast.
      \see Fl_Widget::as_window(), Fl_Widget::as_gl_window()
   */
  virtual Fl_Group* as_group() {return 0;}

  /** Returns an Fl_Window pointer if this widget is an Fl_Window.

      Use this method if you have a widget (pointer) and need to
      know whether this widget is derived from Fl_Window. If it returns
      non-NULL, then the widget in question is derived from Fl_Window,
      and you can use the returned pointer to access its children
      or other Fl_Window-specific methods.

      \retval NULL if this widget is not derived from Fl_Window.
      \note This method is provided to avoid dynamic_cast.
      \see Fl_Widget::as_group(), Fl_Widget::as_gl_window()
   */
  virtual Fl_Window* as_window() {return 0;}

  /** Returns an Fl_Gl_Window pointer if this widget is an Fl_Gl_Window.

      Use this method if you have a widget (pointer) and need to
      know whether this widget is derived from Fl_Gl_Window. If it returns
      non-NULL, then the widget in question is derived from Fl_Gl_Window.

      \retval NULL if this widget is not derived from Fl_Gl_Window.
      \note This method is provided to avoid dynamic_cast.
      \see Fl_Widget::as_group(), Fl_Widget::as_window()
   */
  virtual class Fl_Gl_Window* as_gl_window() {return 0;}
  
  /** For back compatibility only.
      \deprecated Use selection_color() instead.
  */
  Fl_Color color2() const {return (Fl_Color)color2_;}

  /** For back compatibility only.
      \deprecated Use selection_color(unsigned) instead.
  */
  void color2(unsigned a) {color2_ = a;}
};

/**
    Reserved type numbers (necessary for my cheapo RTTI) start here.
    Grep the header files for "RESERVED_TYPE" to find the next available
    number.
*/
#define FL_RESERVED_TYPE 100

#endif

//
// End of "$Id: Fl_Widget.H 8623 2011-04-24 17:09:41Z AlbrechtS $".
//
