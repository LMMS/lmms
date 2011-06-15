//
// "$Id: Fl_Tree_Prefs.H 8340 2011-01-30 20:22:06Z greg.ercolano $"
//

#ifndef FL_TREE_PREFS_H
#define FL_TREE_PREFS_H

//////////////////////
// FL/Fl_Tree_Prefs.H
//////////////////////
//
// Fl_Tree -- This file is part of the Fl_Tree widget for FLTK
// Copyright (C) 2009-2010 by Greg Ercolano.
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

///
/// \file
/// \brief This file contains the definitions for Fl_Tree's preferences.
///
/// \code
///                  Fl_Tree_Prefs
///                       :
///                  .....:.......
///                  :           :
///               Fl_Tree        :
///                  |_____ Fl_Tree_Item
///
/// \endcode
///

/// \class Fl_Tree_Prefs
/// \brief Tree widget's preferences.

/// \enum Fl_Tree_Sort
/// Sort order options for items added to the tree
///
enum Fl_Tree_Sort {
  FL_TREE_SORT_NONE=0,	///< No sorting; items are added in the order defined (default).
  FL_TREE_SORT_ASCENDING=1,	///< Add items in ascending sort order.
  FL_TREE_SORT_DESCENDING=2	///< Add items in descending sort order.
};

/// \enum Fl_Tree_Connector
/// Defines the style of connection lines between items.
///
enum Fl_Tree_Connector {
  FL_TREE_CONNECTOR_NONE=0,	///< Use no lines connecting items
  FL_TREE_CONNECTOR_DOTTED=1,	///< Use dotted lines connecting items (default)
  FL_TREE_CONNECTOR_SOLID=2	///< Use solid lines connecting items
};

/// \enum Fl_Tree_Select
/// Tree selection style.
///
enum Fl_Tree_Select {
  FL_TREE_SELECT_NONE=0,	///< Nothing selected when items are clicked
  FL_TREE_SELECT_SINGLE=1,	///< Single item selected when item is clicked (default)
  FL_TREE_SELECT_MULTI=2	///< Multiple items can be selected by clicking with
  ///< SHIFT or CTRL or mouse drags.
};

/// \class Fl_Tree_Prefs
///
/// \brief Fl_Tree's Preferences class.
///
/// This class manages the Fl_Tree's defaults.
/// You should probably be using the methods in Fl_Tree
/// instead of trying to accessing tree's preferences settings directly.
///
class FL_EXPORT Fl_Tree_Prefs {
  Fl_Font _labelfont;			// label's font face
  Fl_Fontsize _labelsize;		// label's font size
  int _margintop;			// -- 
  int _marginleft;			//   |- tree's margins
  //int _marginright;			//   |
  //int _marginbottom;			// --
  int _openchild_marginbottom;		// extra space below an open child tree
  int _usericonmarginleft;		// space to left of user icon (if any)
  int _labelmarginleft;			// space to left of label
  int _connectorwidth;			// connector width (right of open/close icon)
  int _linespacing;			// vertical space between lines
  // Colors
  Fl_Color _labelfgcolor;		// label's foreground color
  Fl_Color _labelbgcolor;		// background color
  Fl_Color _connectorcolor;		// connector dotted line color
  Fl_Tree_Connector _connectorstyle;	// connector line style
  Fl_Image *_openimage;			// the 'open' icon [+]
  Fl_Image *_closeimage;		// the 'close' icon [-]
  Fl_Image *_userimage;			// user's own icon
  char _showcollapse;			// 1=show collapse icons, 0=don't
  char _showroot;			// show the root item as part of the tree
  Fl_Tree_Sort   _sortorder;		// none, ascening, descending, etc.
  Fl_Boxtype     _selectbox;		// selection box type
  Fl_Tree_Select _selectmode;		// selection mode
public:
  Fl_Tree_Prefs();
  
  ////////////////////////////
  // Labels
  ////////////////////////////
  /// Return the label's font.
  inline Fl_Font labelfont() const {
    return(_labelfont);
  }
  /// Set the label's font to \p val.
  inline void labelfont(Fl_Font val) {
    _labelfont = val;
  }
  /// Return the label's size in pixels.
  inline Fl_Fontsize labelsize() const {
    return(_labelsize);
  }
  /// Set the label's size in pixels to \p val.
  inline void labelsize(Fl_Fontsize val) {
    _labelsize = val;
  }
  
  ////////////////////////////
  // Margins
  ////////////////////////////
  /// Get the left margin's value in pixels
  inline int marginleft() const {
    return(_marginleft);
  }
  /// Set the left margin's value in pixels
  inline void marginleft(int val) {
    _marginleft = val;
  }
  /// Get the top margin's value in pixels
  inline int margintop() const {
    return(_margintop);
  }
  /// Set the top margin's value in pixels
  inline void margintop(int val) {
    _margintop = val;
  }
  /// Get the margin below an open child in pixels
  inline int openchild_marginbottom() const {
    return(_openchild_marginbottom);
  }
  /// Set the margin below an open child in pixels
  inline void openchild_marginbottom(int val) {
    _openchild_marginbottom = val;
  }
  
  /****** NOT IMPLEMENTED
   inline int marginright() const {
   return(_marginright);
   }
   inline void marginright(int val) {
   _marginright = val;
   }
   inline int marginbottom() const {
   return(_marginbottom);
   }
   inline void marginbottom(int val) {
   _marginbottom = val;
   }
   *******/
  
  /// Get the user icon's left margin value in pixels
  inline int usericonmarginleft() const {
    return(_usericonmarginleft);
  }
  /// Set the user icon's left margin value in pixels
  inline void usericonmarginleft(int val) {
    _usericonmarginleft = val;
  }
  /// Get the label's left margin value in pixels
  inline int labelmarginleft() const {
    return(_labelmarginleft);
  }
  /// Set the label's left margin value in pixels
  inline void labelmarginleft(int val) {
    _labelmarginleft = val;
  }
  /// Get the line spacing value in pixels
  inline int linespacing() const {
    return(_linespacing);
  }
  /// Set the line spacing value in pixels
  inline void linespacing(int val) {
    _linespacing = val;
  }
  
  ////////////////////////////
  // Colors and Styles
  ////////////////////////////
  /// Get the default label foreground color
  inline Fl_Color labelfgcolor() const {
    return(_labelfgcolor);
  }
  /// Set the default label foreground color
  inline void labelfgcolor(Fl_Color val) {
    _labelfgcolor = val;
  }
  /// Get the default label background color
  inline Fl_Color labelbgcolor() const {
    return(_labelbgcolor);
  }
  /// Set the default label background color
  inline void labelbgcolor(Fl_Color val) {
    _labelbgcolor = val;
  }
  /// Get the connector color used for tree connection lines.
  inline Fl_Color connectorcolor() const {
    return(_connectorcolor);
  }
  /// Set the connector color used for tree connection lines.
  inline void connectorcolor(Fl_Color val) {
    _connectorcolor = val;
  }
  /// Get the connector style.
  inline Fl_Tree_Connector connectorstyle() const {
    return(_connectorstyle);
  }
  /// Set the connector style.
  inline void connectorstyle(Fl_Tree_Connector val) {
    _connectorstyle = val;
  }
  /// Set the connector style [integer].
  inline void connectorstyle(int val) {
    _connectorstyle = Fl_Tree_Connector(val);
  }
  /// Get the tree connection line's width.
  inline int connectorwidth() const {
    return(_connectorwidth);
  }
  /// Set the tree connection line's width.
  inline void connectorwidth(int val) {
    _connectorwidth = val;
  }
  
  ////////////////////////////
  // Icons
  ////////////////////////////
  /// Get the current default 'open' icon.
  ///     Returns the Fl_Image* of the icon, or 0 if none.
  ///
  inline Fl_Image *openicon() const {
    return(_openimage);
  }
  void openicon(Fl_Image *val);
  /// Gets the default 'close' icon
  ///     Returns the Fl_Image* of the icon, or 0 if none.
  ///
  inline Fl_Image *closeicon() const {
    return(_closeimage);
  }
  void closeicon(Fl_Image *val);
  /// Gets the default 'user icon' (default is 0)
  inline Fl_Image *usericon() const {
    return(_userimage);
  }
  /// Sets the default 'user icon'
  ///     Returns the Fl_Image* of the icon, or 0 if none (default).
  ///
  inline void usericon(Fl_Image *val) {
    _userimage = val;
  }
  
  ////////////////////////////
  // Options
  ////////////////////////////
  /// Returns 1 if the collapse icon is enabled, 0 if not.
  inline char showcollapse() const {
    return(_showcollapse);
  }
  /// Set if we should show the collapse icon or not.
  /// If collapse icons are disabled, the user will not be able
  /// to interactively collapse items in the tree, unless the application
  /// provides some other means via open() and close().
  ///
  /// \param[in] val 1: shows collapse icons (default),\n
  ///                0: hides collapse icons.
  ///
  inline void showcollapse(int val) {
    _showcollapse = val;
  }
  /// Get the default sort order value
  inline Fl_Tree_Sort sortorder() const {
    return(_sortorder);
  }
  /// Set the default sort order value.
  ///     Defines the order new items appear when add()ed to the tree.
  ///     See Fl_Tree_Sort for possible values.
  ///
  inline void sortorder(Fl_Tree_Sort val) {
    _sortorder = val;
  }
  /// Get the default selection box's box drawing style as an Fl_Boxtype.
  inline Fl_Boxtype selectbox() const {
    return(_selectbox);
  }
  /// Set the default selection box's box drawing style to \p val.
  inline void selectbox(Fl_Boxtype val) {
    _selectbox = val;
  }
  /// Returns 1 if the root item is to be shown, or 0 if not.
  inline int showroot() const {
    return(int(_showroot));
  }
  /// Set if the root item should be shown or not.
  /// \param[in] val 1 -- show the root item (default)\n
  ///                0 -- hide the root item.
  ///
  inline void showroot(int val) {
    _showroot = char(val);
  }
  /// Get the selection mode used for the tree
  inline Fl_Tree_Select selectmode() const {
    return(_selectmode);
  }
  /// Set the selection mode used for the tree to \p val.
  ///     This affects how items in the tree are selected
  ///     when clicked on and dragged over by the mouse.
  ///     See Fl_Tree_Select for possible values.
  ///
  inline void selectmode(Fl_Tree_Select val) {
    _selectmode = val;
  }
};

#endif /*FL_TREE_PREFS_H*/

//
// End of "$Id: Fl_Tree_Prefs.H 8340 2011-01-30 20:22:06Z greg.ercolano $".
//
