//
// "$Id: Enumerations.H 8710 2011-05-21 21:55:59Z manolo $"
//
// Enumerations for the Fast Light Tool Kit (FLTK).
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

/** \file
    This file contains type definitions and general enumerations.
 */

#ifndef Fl_Enumerations_H
#define Fl_Enumerations_H

#  include "Fl_Export.H"
#  include "fl_types.h"

/** \name Version Numbers

    FLTK defines some constants to help the programmer to
    find out, for which FLTK version a program is compiled.
    
    The following constants are defined:
 */
/*@{*/

/**
   The major release version of this FLTK library.
   \sa FL_VERSION
 */
#define FL_MAJOR_VERSION	1

/**
   The minor release version for this library.

   FLTK remains mostly source-code compatible between minor version changes.
 */
#define FL_MINOR_VERSION	3

/**
   The patch version for this library.

   FLTK remains binary compatible between patches.
 */
#define FL_PATCH_VERSION	0

/**
    The FLTK version number as a \em double.

    FL_VERSION is a double that describes the major and minor version numbers.
    Version 1.1 is actually stored as 1.01 to allow for more than 9 minor
    releases.

    The FL_MAJOR_VERSION, FL_MINOR_VERSION, and FL_PATCH_VERSION constants
    give the integral values for the major, minor, and patch releases
    respectively.
 */
#define FL_VERSION		((double)FL_MAJOR_VERSION + \
				 (double)FL_MINOR_VERSION * 0.01 + \
				 (double)FL_PATCH_VERSION * 0.0001)

/*@}*/	// group: Version Numbers

/**
    Every time a user moves the mouse pointer, clicks a button,
    or presses a key, an event is generated and sent to your
    application. Events can also come from other programs like the
    window manager.
   
    Events are identified by the integer argument passed to the 
    Fl_Widget::handle() virtual method. Other information about the 
    most recent event is stored in static locations and acquired  by 
    calling the Fl::event_*() methods. This static information remains 
    valid until the next event is read from the window system, so it 
    is ok to look at it outside of the handle() method.

    \sa Fl::event_text(), Fl::event_key(), class Fl::
 */
enum Fl_Event {	// events
  /** No event. */
  FL_NO_EVENT		= 0,

  /** A mouse button has gone down with the mouse pointing at this
      widget. You can find out what button by calling Fl::event_button(). 
      You find out the mouse position by calling Fl::event_x() and
      Fl::event_y().

      A widget indicates that it "wants" the mouse click by returning non-zero 
      from its Fl_Widget::handle() method. It will then become the 
      Fl::pushed() widget and will get FL_DRAG and the matching FL_RELEASE events.  
      If Fl_Widget::handle() returns zero then FLTK will try sending the FL_PUSH 
      to another widget. 
   */
  FL_PUSH		= 1,

  /** A mouse button has been released. You can find out what button by 
      calling Fl::event_button().

      In order to receive the FL_RELEASE event, the widget must return 
      non-zero when handling FL_PUSH.
   */
  FL_RELEASE		= 2,

  /** The mouse has been moved to point at this widget.  This can
      be used for highlighting feedback.  If a widget wants to
      highlight or otherwise track the mouse, it indicates this by
      returning non-zero from its handle() method. It then
      becomes the Fl::belowmouse() widget and will receive 
      FL_MOVE and FL_LEAVE events.
   */
  FL_ENTER		= 3,

  /** The mouse has moved out of the widget.
      In order to receive the FL_LEAVE event, the widget must 
      return non-zero when handling FL_ENTER.
   */
  FL_LEAVE		= 4,

  /** The mouse has moved with a button held down. The current button state 
      is in Fl::event_state(). The mouse position is in Fl::event_x() and 
      Fl::event_y().

      In order to receive FL_DRAG events, the widget must return non-zero 
      when handling FL_PUSH.
   */
  FL_DRAG		= 5,

  /** This indicates an <I>attempt</I> to give a widget the keyboard focus.

      If a widget wants the focus, it should change itself to display the 
      fact that it has the focus, and return non-zero from its handle() method.
      It then becomes the Fl::focus() widget and gets FL_KEYDOWN, FL_KEYUP, 
      and FL_UNFOCUS events.

      The focus will change either because the window manager changed which 
      window gets the focus, or because the user tried to navigate using tab, 
      arrows, or other keys. You can check Fl::event_key() to figure out why 
      it moved. For navigation it will be the key pressed and for interaction 
      with the window manager it will be zero.
   */
  FL_FOCUS		= 6,
  
  /** This event is sent to the previous Fl::focus() widget when another 
      widget gets the focus or the window loses focus.
   */
  FL_UNFOCUS		= 7,

  /** A key was pressed (FL_KEYDOWN) or released (FL_KEYUP). 
      Fl_KEYBOARD is a synonym for FL_KEYDOWN.
      The key can be found in Fl::event_key().
      The text that the key should insert can be found with Fl::event_text() 
      and its length is in Fl::event_length(). If you use the key handle()
      should return 1. If you return zero then FLTK assumes you ignored the 
      key and will then attempt to send it to a parent widget. If none of 
      them want it, it will change the event into a FL_SHORTCUT event.

      To receive FL_KEYBOARD events you must also respond to the FL_FOCUS
      and FL_UNFOCUS events.

      If you are writing a text-editing widget you may also want to call 
      the Fl::compose() function to translate individual keystrokes into 
      non-ASCII characters.

      FL_KEYUP events are sent to the widget that currently has focus. This 
      is not necessarily the same widget that received the corresponding 
      FL_KEYDOWN event because focus may have changed between events.
   */
  FL_KEYDOWN		= 8,

  /** Equivalent to FL_KEYDOWN.
      \see FL_KEYDOWN
   */
  FL_KEYBOARD		= 8,
 
  /** Key release event.
      \see FL_KEYDOWN
   */
  FL_KEYUP		= 9,

  /** The user clicked the close button of a window.
      This event is used internally only to trigger the callback of
      Fl_Window derived classed. The default callback closes the 
      window calling Fl_Window::hide().
   */
  FL_CLOSE		= 10,

  /** The mouse has moved without any mouse buttons held down. 
      This event is sent to the Fl::belowmouse() widget.

      In order to receive FL_MOVE events, the widget must return 
      non-zero when handling FL_ENTER.
   */
  FL_MOVE		= 11,

  /** If the Fl::focus() widget is zero or ignores an FL_KEYBOARD
      event then FLTK tries sending this event to every widget it 
      can, until one of them returns non-zero. FL_SHORTCUT is first 
      sent to the Fl::belowmouse() widget, then its parents and siblings, 
      and eventually to every widget in the window, trying to find an 
      object that returns non-zero. FLTK tries really hard to not to ignore 
      any keystrokes!

      You can also make "global" shortcuts by using Fl::add_handler(). A 
      global shortcut will work no matter what windows are displayed or 
      which one has the focus.
   */
  FL_SHORTCUT		= 12,

  /** This widget is no longer active, due to Fl_Widget::deactivate() 
      being called on it or one of its parents. Fl_Widget::active() may 
      still be true after this, the widget is only active if Fl_Widget::active()
      is true on it and all its parents (use Fl_Widget::active_r() to check this).
   */
  FL_DEACTIVATE		= 13,

  /** This widget is now active, due to Fl_Widget::activate() being 
      called on it or one of its parents.
   */
  FL_ACTIVATE		= 14,

  /** This widget is no longer visible, due to Fl_Widget::hide() being 
      called on it or one of its parents, or due to a parent window being 
      minimized.  Fl_Widget::visible() may still be true after this, but the 
      widget is visible only if visible() is true for it and all its 
      parents (use Fl_Widget::visible_r() to check this).
   */
  FL_HIDE		= 15,

  /** This widget is visible again, due to Fl_Widget::show() being called on 
      it or one of its parents, or due to a parent window being restored. 
      Child Fl_Windows respond to this by actually creating the window if not 
      done already, so if you subclass a window, be sure to pass FL_SHOW 
      to the base class Fl_Widget::handle() method!
   */
  FL_SHOW		= 16,

  /** You should get this event some time after you call Fl::paste(). 
      The contents of Fl::event_text() is the text to insert and the number 
      of characters is in Fl::event_length().
   */
  FL_PASTE		= 17,

  /** The Fl::selection_owner() will get this event before the selection is 
      moved to another widget. This indicates that some other widget or program 
      has claimed the selection. Motif programs used this to clear the selection 
      indication. Most modern programs ignore this.
   */
  FL_SELECTIONCLEAR	= 18,

  /** The user has moved the mouse wheel. The Fl::event_dx() and Fl::event_dy()
      methods can be used to find the amount to scroll horizontally and vertically.
   */
  FL_MOUSEWHEEL		= 19,

  /** The mouse has been moved to point at this widget. A widget that is 
      interested in receiving drag'n'drop data must return 1 to receive 
      FL_DND_DRAG, FL_DND_LEAVE and FL_DND_RELEASE events.
   */
  FL_DND_ENTER		= 20,

  /** The mouse has been moved inside a widget while dragging data.  A 
      widget that is interested in receiving drag'n'drop data should 
      indicate the possible drop position.
   */
  FL_DND_DRAG		= 21,

  /** The mouse has moved out of the widget.
   */
  FL_DND_LEAVE		= 22,

  /** The user has released the mouse button dropping data into the widget. 
      If the widget returns 1, it will receive the data in the immediately 
      following FL_PASTE event.
   */
  FL_DND_RELEASE	= 23
};

/** \name When Conditions */
/*@{*/
/** These constants determine when a callback is performed.

    \sa Fl_Widget::when();
    \todo doxygen comments for values are incomplete and maybe wrong or unclear
 */
enum Fl_When { // Fl_Widget::when():
  FL_WHEN_NEVER		= 0,	///< Never call the callback
  FL_WHEN_CHANGED	= 1,	///< Do the callback only when the widget value changes
  FL_WHEN_NOT_CHANGED	= 2,	///< Do the callback whenever the user interacts with the widget
  FL_WHEN_RELEASE	= 4,	///< Do the callback when the button or key is released and the value changes
  FL_WHEN_RELEASE_ALWAYS= 6,	///< Do the callback when the button or key is released, even if the value doesn't change
  FL_WHEN_ENTER_KEY	= 8,	///< Do the callback when the user presses the ENTER key and the value changes
  FL_WHEN_ENTER_KEY_ALWAYS=10,	///< Do the callback when the user presses the ENTER key, even if the value doesn't change
  FL_WHEN_ENTER_KEY_CHANGED=11	///< ?
};

/*@}*/		// group: When Conditions

/** \name Mouse and Keyboard Events

	This and the following constants define the non-ASCII keys on the
	keyboard for FL_KEYBOARD and FL_SHORTCUT events.

	\todo	FL_Button and FL_key... constants could be structured better
		(use an enum or some doxygen grouping ?)
  
	\sa	Fl::event_key() and Fl::get_key(int) (use ascii letters for all other keys):
 */

/*@{*/

// FIXME: These codes collide with valid Unicode keys

#define FL_Button	0xfee8	///< A mouse button; use Fl_Button + n for mouse button n.
#define FL_BackSpace	0xff08	///< The backspace key.
#define FL_Tab		0xff09	///< The tab key.
#define FL_Enter	0xff0d	///< The enter key. 
#define FL_Pause	0xff13	///< The pause key.
#define FL_Scroll_Lock	0xff14	///< The scroll lock key.
#define FL_Escape	0xff1b	///< The escape key.
#define FL_Home		0xff50	///< The home key.
#define FL_Left		0xff51	///< The left arrow key.
#define FL_Up		0xff52	///< The up arrow key.
#define FL_Right	0xff53	///< The right arrow key.
#define FL_Down		0xff54	///< The down arrow key.
#define FL_Page_Up	0xff55	///< The page-up key.
#define FL_Page_Down	0xff56	///< The page-down key.
#define FL_End		0xff57	///< The end key.
#define FL_Print	0xff61	///< The print (or print-screen) key.
#define FL_Insert	0xff63	///< The insert key. 
#define FL_Menu		0xff67	///< The menu key.
#define FL_Help		0xff68	///< The 'help' key on Mac keyboards
#define FL_Num_Lock	0xff7f	///< The num lock key.
#define FL_KP		0xff80	///< One of the keypad numbers; use FL_KP + n for number n.
#define FL_KP_Enter	0xff8d	///< The enter key on the keypad, same as Fl_KP+'\\r'.
#define FL_KP_Last	0xffbd	///< The last keypad key; use to range-check keypad.
#define FL_F		0xffbd	///< One of the function keys; use FL_F + n for function key n.
#define FL_F_Last	0xffe0	///< The last function key; use to range-check function keys.
#define FL_Shift_L	0xffe1	///< The lefthand shift key.
#define FL_Shift_R	0xffe2	///< The righthand shift key.
#define FL_Control_L	0xffe3	///< The lefthand control key.
#define FL_Control_R	0xffe4	///< The righthand control key.
#define FL_Caps_Lock	0xffe5	///< The caps lock key.
#define FL_Meta_L	0xffe7	///< The left meta/Windows key.
#define FL_Meta_R	0xffe8	///< The right meta/Windows key.
#define FL_Alt_L	0xffe9	///< The left alt key.
#define FL_Alt_R	0xffea	///< The right alt key. 
#define FL_Delete	0xffff	///< The delete key.

// These use the Private Use Area (PUA) of the Basic Multilingual Plane
// of Unicode. Guaranteed not to conflict with a proper Unicode character.

// These primarily map to the XFree86 keysym range
#define FL_Volume_Down  0xEF11   /* Volume control down        */
#define FL_Volume_Mute  0xEF12   /* Mute sound from the system */
#define FL_Volume_Up    0xEF13   /* Volume control up          */
#define FL_Media_Play   0xEF14   /* Start playing of audio     */
#define FL_Media_Stop   0xEF15   /* Stop playing audio         */
#define FL_Media_Prev   0xEF16   /* Previous track             */
#define FL_Media_Next   0xEF17   /* Next track                 */
#define FL_Home_Page    0xEF18   /* Display user's home page   */
#define FL_Mail         0xEF19   /* Invoke user's mail program */
#define FL_Search       0xEF1B   /* Search                     */
#define FL_Back         0xEF26   /* Like back on a browser     */
#define FL_Forward      0xEF27   /* Like forward on a browser  */
#define FL_Stop         0xEF28   /* Stop current operation     */
#define FL_Refresh      0xEF29   /* Refresh the page           */
#define FL_Sleep        0xEF2F   /* Put system to sleep        */
#define FL_Favorites    0xEF30   /* Show favorite locations    */

/*@}*/	// group: Mouse and Keyboard Events

/** \name Mouse Buttons

	These constants define the button numbers for FL_PUSH and FL_RELEASE events.
	
	\sa Fl::event_button()
*/

/*@{*/

#define FL_LEFT_MOUSE	1	///< The left mouse button
#define FL_MIDDLE_MOUSE	2	///< The middle mouse button
#define FL_RIGHT_MOUSE	3	///< The right mouse button

/*@}*/		// group: Mouse Buttons


/** \name	Event States

	The following constants define bits in the Fl::event_state() value.
*/

/*@{*/		// group: Event States

// FIXME: it would be nice to have the modifiers in the upper 8 bit so that
//        a unicode ke (24bit) can be sent as an unsigned with the modifiers.

#define FL_SHIFT	0x00010000	///< One of the shift keys is down
#define FL_CAPS_LOCK	0x00020000	///< The caps lock is on
#define FL_CTRL		0x00040000	///< One of the ctrl keys is down
#define FL_ALT		0x00080000	///< One of the alt keys is down
#define FL_NUM_LOCK	0x00100000	///< The num lock is on
					// most X servers do this?
#define FL_META		0x00400000	///< One of the meta/Windows keys is down
					// correct for XFree86
#define FL_SCROLL_LOCK	0x00800000	///< The scroll lock is on
					// correct for XFree86
#define FL_BUTTON1	0x01000000	///< Mouse button 1 is pushed
#define FL_BUTTON2	0x02000000	///< Mouse button 2 is pushed
#define FL_BUTTON3	0x04000000	///< Mouse button 3 is pushed
#define FL_BUTTONS	0x7f000000	///< Any mouse button is pushed
#define FL_BUTTON(n)	(0x00800000<<(n)) ///< Mouse button n (n > 0) is pushed

#define FL_KEY_MASK 0x0000ffff		///< All keys are 16 bit for now 
					//   FIXME: Unicode needs 24 bits!

#ifdef __APPLE__
#  define FL_COMMAND	FL_META		///< An alias for FL_CTRL on WIN32 and X11, or FL_META on MacOS X
#  define FL_CONTROL 	FL_CTRL		///< An alias for FL_META on WIN32 and X11, or FL_CTRL on MacOS X
#else
#  define FL_COMMAND	FL_CTRL		///< An alias for FL_CTRL on WIN32 and X11, or FL_META on MacOS X
#  define FL_CONTROL	FL_META		///< An alias for FL_META on WIN32 and X11, or FL_CTRL on MacOS X
#endif // __APPLE__

/*@}*/		// group: Event States

/** \name Box Types
    \brief FLTK standard box types
    
    This enum defines the standard box types included with FLTK.
    
    FL_NO_BOX means nothing is drawn at all, so whatever is already 
    on the screen remains. The FL_..._FRAME types only draw their edges, 
    leaving the interior unchanged. The blue color in Figure 1 
    is the area that is not drawn by the frame types.
    
    \image html boxtypes.png "Figure 1: FLTK standard box types"
    \image latex boxtypes.png "FLTK standard box types" width=10cm
    \todo	Description of boxtypes is incomplete.
    		See below for the defined enum Fl_Boxtype.
		\see src/Fl_get_system_colors.cxx
*/
/*@{*/
enum Fl_Boxtype { // boxtypes (if you change these you must fix fl_boxtype.C):

  FL_NO_BOX = 0,		///< nothing is drawn at all, this box is invisible
  FL_FLAT_BOX,			///< a flat box
  FL_UP_BOX,			///< see figure 1
  FL_DOWN_BOX,			///< see figure 1
  FL_UP_FRAME,			///< see figure 1
  FL_DOWN_FRAME,		///< see figure 1
  FL_THIN_UP_BOX,		///< see figure 1
  FL_THIN_DOWN_BOX,		///< see figure 1
  FL_THIN_UP_FRAME,		///< see figure 1
  FL_THIN_DOWN_FRAME,		///< see figure 1
  FL_ENGRAVED_BOX,		///< see figure 1
  FL_EMBOSSED_BOX,		///< see figure 1
  FL_ENGRAVED_FRAME,		///< see figure 1
  FL_EMBOSSED_FRAME,		///< see figure 1
  FL_BORDER_BOX,		///< see figure 1
  _FL_SHADOW_BOX,		///< see figure 1
  FL_BORDER_FRAME,		///< see figure 1
  _FL_SHADOW_FRAME,		///< see figure 1
  _FL_ROUNDED_BOX,		///< see figure 1
  _FL_RSHADOW_BOX,		///< see figure 1
  _FL_ROUNDED_FRAME,		///< see figure 1
  _FL_RFLAT_BOX,		///< see figure 1
  _FL_ROUND_UP_BOX,		///< see figure 1
  _FL_ROUND_DOWN_BOX,		///< see figure 1
  _FL_DIAMOND_UP_BOX,		///< see figure 1
  _FL_DIAMOND_DOWN_BOX,		///< see figure 1
  _FL_OVAL_BOX,			///< see figure 1
  _FL_OSHADOW_BOX,		///< see figure 1
  _FL_OVAL_FRAME,		///< see figure 1
  _FL_OFLAT_BOX,		///< see figure 1
  _FL_PLASTIC_UP_BOX,		///< plastic version of FL_UP_BOX
  _FL_PLASTIC_DOWN_BOX,		///< plastic version of FL_DOWN_BOX
  _FL_PLASTIC_UP_FRAME,		///< plastic version of FL_UP_FRAME
  _FL_PLASTIC_DOWN_FRAME,	///< plastic version of FL_DOWN_FRAME
  _FL_PLASTIC_THIN_UP_BOX,	///< plastic version of FL_THIN_UP_BOX
  _FL_PLASTIC_THIN_DOWN_BOX,	///< plastic version of FL_THIN_DOWN_BOX
  _FL_PLASTIC_ROUND_UP_BOX,	///< plastic version of FL_ROUND_UP_BOX
  _FL_PLASTIC_ROUND_DOWN_BOX,	///< plastic version of FL_ROUND_DOWN_BOX
  _FL_GTK_UP_BOX,		///< gtk+ version of FL_UP_BOX
  _FL_GTK_DOWN_BOX,		///< gtk+ version of FL_DOWN_BOX
  _FL_GTK_UP_FRAME,		///< gtk+ version of FL_UP_FRAME
  _FL_GTK_DOWN_FRAME,		///< gtk+ version of FL_DOWN_RAME
  _FL_GTK_THIN_UP_BOX,		///< gtk+ version of FL_THIN_UP_BOX
  _FL_GTK_THIN_DOWN_BOX,	///< gtk+ version of FL_THIN_DOWN_BOX
  _FL_GTK_THIN_UP_FRAME,	///< gtk+ version of FL_UP_FRAME
  _FL_GTK_THIN_DOWN_FRAME,	///< gtk+ version of FL_THIN_DOWN_FRAME
  _FL_GTK_ROUND_UP_BOX,		///< gtk+ version of FL_ROUND_UP_BOX
  _FL_GTK_ROUND_DOWN_BOX,	///< gtk+ version of FL_ROUND_DOWN_BOX
  FL_FREE_BOXTYPE		///< the first free box type for creation of new box types
};
extern FL_EXPORT Fl_Boxtype fl_define_FL_ROUND_UP_BOX();
#define FL_ROUND_UP_BOX fl_define_FL_ROUND_UP_BOX()
#define FL_ROUND_DOWN_BOX (Fl_Boxtype)(fl_define_FL_ROUND_UP_BOX()+1)
extern FL_EXPORT Fl_Boxtype fl_define_FL_SHADOW_BOX();
#define FL_SHADOW_BOX fl_define_FL_SHADOW_BOX()
#define FL_SHADOW_FRAME (Fl_Boxtype)(fl_define_FL_SHADOW_BOX()+2)
extern FL_EXPORT Fl_Boxtype fl_define_FL_ROUNDED_BOX();
#define FL_ROUNDED_BOX fl_define_FL_ROUNDED_BOX()
#define FL_ROUNDED_FRAME (Fl_Boxtype)(fl_define_FL_ROUNDED_BOX()+2)
extern FL_EXPORT Fl_Boxtype fl_define_FL_RFLAT_BOX();
#define FL_RFLAT_BOX fl_define_FL_RFLAT_BOX()
extern FL_EXPORT Fl_Boxtype fl_define_FL_RSHADOW_BOX();
#define FL_RSHADOW_BOX fl_define_FL_RSHADOW_BOX()
extern FL_EXPORT Fl_Boxtype fl_define_FL_DIAMOND_BOX();
#define FL_DIAMOND_UP_BOX fl_define_FL_DIAMOND_BOX()
#define FL_DIAMOND_DOWN_BOX (Fl_Boxtype)(fl_define_FL_DIAMOND_BOX()+1)
extern FL_EXPORT Fl_Boxtype fl_define_FL_OVAL_BOX();
#define FL_OVAL_BOX fl_define_FL_OVAL_BOX()
#define FL_OSHADOW_BOX (Fl_Boxtype)(fl_define_FL_OVAL_BOX()+1)
#define FL_OVAL_FRAME (Fl_Boxtype)(fl_define_FL_OVAL_BOX()+2)
#define FL_OFLAT_BOX (Fl_Boxtype)(fl_define_FL_OVAL_BOX()+3)

extern FL_EXPORT Fl_Boxtype fl_define_FL_PLASTIC_UP_BOX();
#define FL_PLASTIC_UP_BOX fl_define_FL_PLASTIC_UP_BOX()
#define FL_PLASTIC_DOWN_BOX (Fl_Boxtype)(fl_define_FL_PLASTIC_UP_BOX()+1)
#define FL_PLASTIC_UP_FRAME (Fl_Boxtype)(fl_define_FL_PLASTIC_UP_BOX()+2)
#define FL_PLASTIC_DOWN_FRAME (Fl_Boxtype)(fl_define_FL_PLASTIC_UP_BOX()+3)
#define FL_PLASTIC_THIN_UP_BOX (Fl_Boxtype)(fl_define_FL_PLASTIC_UP_BOX()+4)
#define FL_PLASTIC_THIN_DOWN_BOX (Fl_Boxtype)(fl_define_FL_PLASTIC_UP_BOX()+5)
#define FL_PLASTIC_ROUND_UP_BOX (Fl_Boxtype)(fl_define_FL_PLASTIC_UP_BOX()+6)
#define FL_PLASTIC_ROUND_DOWN_BOX (Fl_Boxtype)(fl_define_FL_PLASTIC_UP_BOX()+7)

extern FL_EXPORT Fl_Boxtype fl_define_FL_GTK_UP_BOX();
#define FL_GTK_UP_BOX fl_define_FL_GTK_UP_BOX()
#define FL_GTK_DOWN_BOX (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+1)
#define FL_GTK_UP_FRAME (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+2)
#define FL_GTK_DOWN_FRAME (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+3)
#define FL_GTK_THIN_UP_BOX (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+4)
#define FL_GTK_THIN_DOWN_BOX (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+5)
#define FL_GTK_THIN_UP_FRAME (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+6)
#define FL_GTK_THIN_DOWN_FRAME (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+7)
#define FL_GTK_ROUND_UP_BOX (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+8)
#define FL_GTK_ROUND_DOWN_BOX (Fl_Boxtype)(fl_define_FL_GTK_UP_BOX()+9)

// conversions of box types to other boxtypes:
/**
  Get the filled version of a frame.
  If no filled version of a given frame exists, the behavior of this function
  is undefined and some random box or frame is returned.
 */
inline Fl_Boxtype fl_box(Fl_Boxtype b) {
  return (Fl_Boxtype)((b<FL_UP_BOX||b%4>1)?b:(b-2));
}
/**
  Get the "pressed" or "down" version of a box.
  If no "down" version of a given box exists, the behavior of this function
  is undefined and some random box or frame is returned.
 */
inline Fl_Boxtype fl_down(Fl_Boxtype b) {
  return (Fl_Boxtype)((b<FL_UP_BOX)?b:(b|1));
}
/**
  Get the unfilled, frame only version of a box.
  If no frame version of a given box exists, the behavior of this function
  is undefined and some random box or frame is returned.
 */
inline Fl_Boxtype fl_frame(Fl_Boxtype b) {
  return (Fl_Boxtype)((b%4<2)?b:(b+2));
}

// back-compatibility box types:
#define FL_FRAME FL_ENGRAVED_FRAME
#define FL_FRAME_BOX FL_ENGRAVED_BOX
#define FL_CIRCLE_BOX FL_ROUND_DOWN_BOX
#define FL_DIAMOND_BOX FL_DIAMOND_DOWN_BOX

/*@}*/	// group: Box Types

/**
   The labeltype() method sets the type of the label.
   
   The following standard label types are included:
   
   \todo	The doxygen comments are incomplete, and some labeltypes
   		are starting with an underscore. Also, there are three
		external functions undocumented (yet):
		  - fl_define_FL_SHADOW_LABEL()
		  - fl_define_FL_ENGRAVED_LABEL()
		  - fl_define_FL_EMBOSSED_LABEL()
*/
enum Fl_Labeltype {	// labeltypes:
  FL_NORMAL_LABEL	= 0,	///< draws the text (0)
  FL_NO_LABEL,			///< does nothing
  _FL_SHADOW_LABEL,		///< draws a drop shadow under the text
  _FL_ENGRAVED_LABEL,		///< draws edges as though the text is engraved
  _FL_EMBOSSED_LABEL,		///< draws edges as though the text is raised
  _FL_MULTI_LABEL,		///< ?
  _FL_ICON_LABEL,		///< draws the icon associated with the text
  _FL_IMAGE_LABEL,		///< ?

  FL_FREE_LABELTYPE		///< first free labeltype to use for creating own labeltypes
};

/** 
  Sets the current label type and return its corresponding Fl_Labeltype value. 
  @{
*/
#define FL_SYMBOL_LABEL FL_NORMAL_LABEL
extern Fl_Labeltype FL_EXPORT fl_define_FL_SHADOW_LABEL();
#define FL_SHADOW_LABEL fl_define_FL_SHADOW_LABEL()
extern Fl_Labeltype FL_EXPORT fl_define_FL_ENGRAVED_LABEL();
#define FL_ENGRAVED_LABEL fl_define_FL_ENGRAVED_LABEL()
extern Fl_Labeltype FL_EXPORT fl_define_FL_EMBOSSED_LABEL();
#define FL_EMBOSSED_LABEL fl_define_FL_EMBOSSED_LABEL()
/** @} */

/** \name Alignment Flags 
    Flags to control the label alignment. 
    This controls how the label is displayed next to or inside the widget. 
    The default value is FL_ALIGN_CENTER for most widgets, which centers the label
    inside the widget.

    Flags can be or'd to achieve a combination of alignments.

    \code
    Outside alignments:
               TOP_LEFT        TOP       TOP_RIGHT
       LEFT_TOP+---------------------------------+RIGHT_TOP
               |                                 |
           LEFT|                                 |RIGHT
               |                                 |
    LEFT_BOTTOM+---------------------------------+RIGHT_BOTTOM
               BOTTOM_RIGHT   BOTTOM   BOTTOM_LEFT

    Inside alignments:
               +---------------------------------+
               |TOP_LEFT       TOP      TOP_RIGHT|
               |                                 |
               |LEFT                        RIGHT|
               |                                 |
               |BOTTOM_RIGHT  BOTTOM  BOTTOM_LEFT|
               +---------------------------------+
    \endcode
    \see #FL_ALIGN_CENTER, etc.
 */
/*@{*/
/** FLTK type for alignment control */
typedef unsigned Fl_Align;
  /** Align the label horizontally in the middle. */
const Fl_Align FL_ALIGN_CENTER		= (Fl_Align)0;
  /** Align the label at the top of the widget. Inside labels appear below the top,
      outside labels are drawn on top of the widget. */
const Fl_Align FL_ALIGN_TOP		= (Fl_Align)1;
  /** Align the label at the bottom of the widget. */
const Fl_Align FL_ALIGN_BOTTOM		= (Fl_Align)2;
  /** Align the label at the left of the widget. Inside labels appear left-justified
      starting at the left side of the widget, outside labels are right-justified and
      drawn to the left of the widget. */
const Fl_Align FL_ALIGN_LEFT		= (Fl_Align)4;
  /** Align the label to the right of the widget. */
const Fl_Align FL_ALIGN_RIGHT		= (Fl_Align)8;
  /** Draw the label inside of the widget. */
const Fl_Align FL_ALIGN_INSIDE		= (Fl_Align)16;
  /** If the label contains an image, draw the text on top of the image. */
const Fl_Align FL_ALIGN_TEXT_OVER_IMAGE	= (Fl_Align)0x0020;
  /** If the label contains an image, draw the text below the image. */
const Fl_Align FL_ALIGN_IMAGE_OVER_TEXT	= (Fl_Align)0x0000;
  /** All parts of the label that are lager than the widget will not be drawn . */
const Fl_Align FL_ALIGN_CLIP		= (Fl_Align)64;
  /** Wrap text that does not fit the width of the widget. */
const Fl_Align FL_ALIGN_WRAP		= (Fl_Align)128;
  /** If the label contains an image, draw the text to the right of the image. */
const Fl_Align FL_ALIGN_IMAGE_NEXT_TO_TEXT = (Fl_Align)0x0100;
  /** If the label contains an image, draw the text to the left of the image. */
const Fl_Align FL_ALIGN_TEXT_NEXT_TO_IMAGE = (Fl_Align)0x0120;
/** If the label contains an image, draw the image or deimage in the background. */
const Fl_Align FL_ALIGN_IMAGE_BACKDROP  = (Fl_Align)0x0200;
const Fl_Align FL_ALIGN_TOP_LEFT	= FL_ALIGN_TOP | FL_ALIGN_LEFT;
const Fl_Align FL_ALIGN_TOP_RIGHT	= FL_ALIGN_TOP | FL_ALIGN_RIGHT;
const Fl_Align FL_ALIGN_BOTTOM_LEFT	= FL_ALIGN_BOTTOM | FL_ALIGN_LEFT;
const Fl_Align FL_ALIGN_BOTTOM_RIGHT	= FL_ALIGN_BOTTOM | FL_ALIGN_RIGHT;
const Fl_Align FL_ALIGN_LEFT_TOP	= 0x0007; // magic value
const Fl_Align FL_ALIGN_RIGHT_TOP	= 0x000b; // magic value
const Fl_Align FL_ALIGN_LEFT_BOTTOM	= 0x000d; // magic value
const Fl_Align FL_ALIGN_RIGHT_BOTTOM	= 0x000e; // magic value
const Fl_Align FL_ALIGN_NOWRAP		= (Fl_Align)0; // for back compatibility
const Fl_Align FL_ALIGN_POSITION_MASK   = 0x000f; // left, right, top, bottom
const Fl_Align FL_ALIGN_IMAGE_MASK      = 0x0320; // l/r, t/b, backdrop
/*@}*/


/** \name Font Numbers
    The following constants define the standard FLTK fonts:
 */
/*@{*/
/** A font number is an index into the internal font table. */
typedef int Fl_Font;

const Fl_Font FL_HELVETICA              = 0;	///< Helvetica (or Arial) normal (0)
const Fl_Font FL_HELVETICA_BOLD         = 1;	///< Helvetica (or Arial) bold
const Fl_Font FL_HELVETICA_ITALIC       = 2;	///< Helvetica (or Arial) oblique
const Fl_Font FL_HELVETICA_BOLD_ITALIC  = 3;	///< Helvetica (or Arial) bold-oblique
const Fl_Font FL_COURIER                = 4;	///< Courier normal
const Fl_Font FL_COURIER_BOLD           = 5;	///< Courier bold 
const Fl_Font FL_COURIER_ITALIC         = 6;	///< Courier italic
const Fl_Font FL_COURIER_BOLD_ITALIC    = 7;	///< Courier bold-italic
const Fl_Font FL_TIMES                  = 8;	///< Times roman
const Fl_Font FL_TIMES_BOLD             = 9;	///< Times roman bold
const Fl_Font FL_TIMES_ITALIC           = 10;	///< Times roman italic
const Fl_Font FL_TIMES_BOLD_ITALIC      = 11;	///< Times roman bold-italic
const Fl_Font FL_SYMBOL                 = 12;	///< Standard symbol font
const Fl_Font FL_SCREEN                 = 13;	///< Default monospaced screen font
const Fl_Font FL_SCREEN_BOLD            = 14;	///< Default monospaced bold screen font
const Fl_Font FL_ZAPF_DINGBATS          = 15;	///< Zapf-dingbats font

const Fl_Font FL_FREE_FONT              = 16;	///< first one to allocate
const Fl_Font FL_BOLD                   = 1;	///< add this to helvetica, courier, or times
const Fl_Font FL_ITALIC                 = 2;	///< add this to helvetica, courier, or times
const Fl_Font FL_BOLD_ITALIC            = 3;	///< add this to helvetica, courier, or times

/*@}*/

/** Size of a font in pixels.
    This is the approximate height of a font in pixels.
 */
typedef int Fl_Fontsize;

extern FL_EXPORT Fl_Fontsize FL_NORMAL_SIZE;	///< normal font size

/** \name Colors 
    The Fl_Color type holds an FLTK color value.

    Colors are either 8-bit indexes into a virtual colormap
    or 24-bit RGB color values.

    Color indices occupy the lower 8 bits of the value, while
    RGB colors occupy the upper 24 bits, for a byte organization of RGBI.

<pre>
 Fl_Color => 0xrrggbbii
                | | | |
                | | | +--- index between 0 and 255
                | | +----- blue color component (8 bit)
                | +------- green component (8 bit)
                +--------- red component (8 bit)
 </pre>

    A color can have either an index or an rgb value. Colors with rgb set 
    and an index >0 are reserved for special use.

 */
/*@{*/
/** an FLTK color value */
typedef unsigned int Fl_Color;

// Standard colors. These are used as default colors in widgets and altered as necessary
const Fl_Color FL_FOREGROUND_COLOR  = 0;	///< the default foreground color (0) used for labels and text
const Fl_Color FL_BACKGROUND2_COLOR = 7;	///< the default background color for text, list, and valuator widgets
const Fl_Color FL_INACTIVE_COLOR    = 8;	///< the inactive foreground color
const Fl_Color FL_SELECTION_COLOR   = 15;	///< the default selection/highlight color

  // boxtypes generally limit themselves to these colors so
  // the whole ramp is not allocated:

const Fl_Color FL_GRAY0   = 32;			// 'A'
const Fl_Color FL_DARK3   = 39;			// 'H'
const Fl_Color FL_DARK2   = 45;			// 'N'
const Fl_Color FL_DARK1   = 47;			// 'P'
const Fl_Color FL_BACKGROUND_COLOR  = 49;	// 'R' default background color
const Fl_Color FL_LIGHT1  = 50;			// 'S'
const Fl_Color FL_LIGHT2  = 52;			// 'U'
const Fl_Color FL_LIGHT3  = 54;			// 'W'

  // FLTK provides a 5x8x5 color cube that is used with colormap visuals

const Fl_Color FL_BLACK   = 56;
const Fl_Color FL_RED     = 88;
const Fl_Color FL_GREEN   = 63;
const Fl_Color FL_YELLOW  = 95;
const Fl_Color FL_BLUE    = 216;
const Fl_Color FL_MAGENTA = 248;
const Fl_Color FL_CYAN    = 223;
const Fl_Color FL_DARK_RED = 72;

const Fl_Color FL_DARK_GREEN    = 60;
const Fl_Color FL_DARK_YELLOW   = 76;
const Fl_Color FL_DARK_BLUE     = 136;
const Fl_Color FL_DARK_MAGENTA  = 152;
const Fl_Color FL_DARK_CYAN     = 140;

const Fl_Color FL_WHITE         = 255;


#define FL_FREE_COLOR     (Fl_Color)16
#define FL_NUM_FREE_COLOR 16
#define FL_GRAY_RAMP      (Fl_Color)32
#define FL_NUM_GRAY       24
#define FL_GRAY           FL_BACKGROUND_COLOR
#define FL_COLOR_CUBE     (Fl_Color)56
#define FL_NUM_RED        5
#define FL_NUM_GREEN      8
#define FL_NUM_BLUE       5

FL_EXPORT Fl_Color fl_inactive(Fl_Color c);

FL_EXPORT Fl_Color fl_contrast(Fl_Color fg, Fl_Color bg);

FL_EXPORT Fl_Color fl_color_average(Fl_Color c1, Fl_Color c2, float weight);

/** Returns a lighter version of the specified color. */
inline Fl_Color fl_lighter(Fl_Color c) { return fl_color_average(c, FL_WHITE, .67f); }

/** Returns a darker version of the specified color. */
inline Fl_Color fl_darker(Fl_Color c) { return fl_color_average(c, FL_BLACK, .67f); }

/** Returns the 24-bit color value closest to \p r, \p g, \p b. */
inline Fl_Color fl_rgb_color(uchar r, uchar g, uchar b) {
  if (!r && !g && !b) return FL_BLACK;
  else return (Fl_Color)(((((r << 8) | g) << 8) | b) << 8);
}

/** Returns the 24-bit color value closest to \p g (grayscale). */
inline Fl_Color fl_rgb_color(uchar g) {
  if (!g) return FL_BLACK;
  else return (Fl_Color)(((((g << 8) | g) << 8) | g) << 8);
}

/** Returns a gray color value from black (i == 0) to white (i == FL_NUM_GRAY - 1). 
    FL_NUM_GRAY is defined to be 24 in the current FLTK release. 
    To get the closest FLTK gray value to an 8-bit grayscale color 'I' use:

 \code 
   fl_gray_ramp(I * (FL_NUM_GRAY - 1) / 255)
 \endcode
*/ 
inline Fl_Color fl_gray_ramp(int i) {return (Fl_Color)(i+FL_GRAY_RAMP);}

/** Returns a color out of the color cube.

  \p r must be in the range 0 to FL_NUM_RED (5) minus 1, 
  \p g must be in the range 0 to FL_NUM_GREEN (8) minus 1,
  \p b must be in the range 0 to FL_NUM_BLUE (5) minus 1.

  To get the closest color to a 8-bit set of R,G,B values use:

  \code
    fl_color_cube(R * (FL_NUM_RED - 1) / 255,
       G * (FL_NUM_GREEN - 1) / 255,
       B * (FL_NUM_BLUE - 1) / 255);
  \endcode
 */
inline Fl_Color fl_color_cube(int r, int g, int b) {
  return (Fl_Color)((b*FL_NUM_RED + r) * FL_NUM_GREEN + g + FL_COLOR_CUBE);}

/*@}*/		// group: Colors  

/** \name Cursors */
/*@{*/

/** The following constants define the mouse cursors that are available in FLTK.

    The double-headed arrows are bitmaps provided by FLTK on X, the others
    are provided by system-defined cursors.

    \todo enum Fl_Cursor needs maybe an image.
*/
enum Fl_Cursor {
  FL_CURSOR_DEFAULT	=  0, /**< the default cursor, usually an arrow. */
  FL_CURSOR_ARROW	= 35, /**< an arrow pointer. */
  FL_CURSOR_CROSS	= 66, /**< crosshair. */
  FL_CURSOR_WAIT	= 76, /**< watch or hourglass. */
  FL_CURSOR_INSERT	= 77, /**< I-beam. */
  FL_CURSOR_HAND	= 31, /**< hand (uparrow on MSWindows). */
  FL_CURSOR_HELP	= 47, /**< question mark. */
  FL_CURSOR_MOVE	= 27, /**< 4-pointed arrow. */
  // fltk provides bitmaps for these:
  FL_CURSOR_NS		= 78, /**< up/down arrow. */
  FL_CURSOR_WE		= 79, /**< left/right arrow. */
  FL_CURSOR_NWSE	= 80, /**< diagonal arrow. */
  FL_CURSOR_NESW	= 81, /**< diagonal arrow. */
  FL_CURSOR_NONE	=255, /**< invisible. */
  // for back compatibility (non MSWindows ones):
  FL_CURSOR_N		= 70, /**< for back compatibility. */
  FL_CURSOR_NE		= 69, /**< for back compatibility. */
  FL_CURSOR_E		= 49, /**< for back compatibility. */
  FL_CURSOR_SE		=  8, /**< for back compatibility. */
  FL_CURSOR_S		=  9, /**< for back compatibility. */
  FL_CURSOR_SW		=  7, /**< for back compatibility. */
  FL_CURSOR_W		= 36, /**< for back compatibility. */
  FL_CURSOR_NW		= 68 /**< for back compatibility. */
};
/*@}*/		// group: Cursors  

/** FD "when" conditions */
enum { // values for "when" passed to Fl::add_fd()
  FL_READ   = 1, /**< Call the callback when there is data to be read. */
  FL_WRITE  = 4, /**< Call the callback when data can be written without blocking. */
  FL_EXCEPT = 8  /**< Call the callback if an exception occurs on the file. */
};

/** visual types and Fl_Gl_Window::mode() (values match Glut) */
enum Fl_Mode { 
  FL_RGB	= 0,
  FL_INDEX	= 1,
  FL_SINGLE	= 0,
  FL_DOUBLE	= 2,
  FL_ACCUM	= 4,
  FL_ALPHA	= 8,
  FL_DEPTH	= 16,
  FL_STENCIL	= 32,
  FL_RGB8	= 64,
  FL_MULTISAMPLE= 128,
  FL_STEREO     = 256,
  FL_FAKE_SINGLE = 512	// Fake single buffered windows using double-buffer
};

// image alpha blending

#define FL_IMAGE_WITH_ALPHA 0x40000000

/** Damage masks */
enum Fl_Damage {
  FL_DAMAGE_CHILD    = 0x01, /**< A child needs to be redrawn. */
  FL_DAMAGE_EXPOSE   = 0x02, /**< The window was exposed. */
  FL_DAMAGE_SCROLL   = 0x04, /**< The Fl_Scroll widget was scrolled. */
  FL_DAMAGE_OVERLAY  = 0x08, /**< The overlay planes need to be redrawn. */
  FL_DAMAGE_USER1    = 0x10, /**< First user-defined damage bit. */
  FL_DAMAGE_USER2    = 0x20, /**< Second user-defined damage bit. */
  FL_DAMAGE_ALL      = 0x80  /**< Everything needs to be redrawn. */
};

// FLTK 1.0.x compatibility definitions...
#  ifdef FLTK_1_0_COMPAT
#    define contrast	fl_contrast
#    define down	fl_down
#    define frame	fl_frame
#    define inactive	fl_inactive
#  endif // FLTK_1_0_COMPAT

#endif

//
// End of "$Id: Enumerations.H 8710 2011-05-21 21:55:59Z manolo $".
//
