//
// "$Id: Fl_Tree.H 7672 2010-07-10 09:44:45Z matt $"
//

#ifndef FL_TREE_H
#define FL_TREE_H

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>

#include <FL/Fl_Tree_Item.H>
#include <FL/Fl_Tree_Prefs.H>

//////////////////////
// FL/Fl_Tree.H
//////////////////////
//
// Fl_Tree -- This file is part of the Fl_Tree widget for FLTK
// Copyright (C) 2009 by Greg Ercolano.
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
/// \brief This file contains the definitions of the Fl_Tree class
///

/// \class Fl_Tree
///
/// \brief Tree widget.
///
/// \code
///     Fl_Tree                                         // Top level widget
///        |--- Fl_Tree_Item                            // Items in the tree
///        |--- Fl_Tree_Prefs                           // Preferences for the tree
///                  |--- Fl_Tree_Connector (enum)      // Connection modes
///                  |--- Fl_Tree_Select (enum)         // Selection modes
///                  |--- Fl_Tree_Sort (enum)           // Sort behavior
/// \endcode
///
///     An expandable tree widget.
///
///     Similar to Fl_Browser, Fl_Tree is browser of Fl_Tree_Item's, which can be
///     in a parented hierarchy. Subtrees can be expanded or closed. Items can be
///     added, deleted, inserted, sorted and re-ordered.
///
///     The tree items may also contain other FLTK widgets, like buttons, input fields,
///     or even "custom" widgets.
///
///     The callback() is invoked depending on the value of when():
///
///         - FL_WHEN_RELEASE -- callback invoked when left mouse button is released on an item
///         - FL_WHEN_CHANGED -- callback invoked when left mouse changes selection state
///
///     The simple way to define a tree:
/// \code
///    #include <FL/Fl_Tree.H>
///    [..]
///    Fl_Tree tree(X,Y,W,H);
///    tree.begin();
///      tree.add("Flintstones/Fred");
///      tree.add("Flintstones/Wilma");
///      tree.add("Flintstones/Pebbles");
///      tree.add("Simpsons/Homer");
///      tree.add("Simpsons/Marge");
///      tree.add("Simpsons/Bart");
///      tree.add("Simpsons/Lisa");
///    tree.end();
/// \endcode
///     
///     Items can be added with Fl_Tree::add(),
///     removed with Fl_Tree::remove(),
///     inserted with Fl_Tree::insert_above(),
///     selected/deselected with Fl_Tree::select() and Fl_Tree::deselect().
///     Items can be swapped with Fl_Tree_Item::swap_children(), sorting control via
///     Fl_Tree::sortorder().
///
///     The tree can have different selection behaviors controlled by Fl_Tree::selectmode().
///
///     FLTK and custom FLTK widgets can be assigned to tree items via Fl_Tree_Item::widget().
///
///     Parent nodes can be open/closed with open() and close(), icons can be assigned
///     or redefined with some or all items via 
///     Fl_Tree_Item::openicon(), 
///     Fl_Tree_Item::closeicon(),
///     Fl_Tree_Item::usericon().
///
///     Various default preferences can be manipulated via Fl_Tree_Prefs, including
///     colors, margins, connection lines. 
///
///     \image html tree-elements.png
///
///     \todo Needs handling of callbacks when items are procedurally select()ed
///

class Fl_Tree : public Fl_Group {
  Fl_Tree_Item  *_root;					// can be null!
  Fl_Tree_Item  *_item_clicked;
  Fl_Tree_Prefs  _prefs;				// all the tree's settings
  Fl_Scrollbar  *_vscroll;
  
public:
  /// Find the item that was clicked.
  /// You probably want to use item_clicked() instead, which is fast.
  ///
  /// This method walks the entire tree looking for the first item that is
  /// under the mouse (ie. at Fl::event_x()/Fl:event_y().
  ///
  /// Use this method /only/ if you've subclassed Fl_Tree, and are receiving
  /// events before Fl_Tree has been able to process and update item_clicked().
  /// 
  /// \returns the item clicked, or 0 if no item was under the current event.
  ///
  const Fl_Tree_Item *find_clicked() const {
    if ( ! _root ) return(0);
    return(_root->find_clicked(_prefs));
  }
protected:
  /// Set the item that was last clicked.
  /// Should only be used by subclasses needing to change this value.
  /// Normally Fl_Tree manages this value.
  ///
  void item_clicked(Fl_Tree_Item* val) {
    _item_clicked = val;
  }
  void do_callback_for_item(Fl_Tree_Item* item) {
    Fl_Tree_Item *save = _item_clicked;		// save previous 'item_clicked'
    _item_clicked = item;			// set item_clicked to this item while we do callback
    do_callback((Fl_Widget*)this, user_data());
    _item_clicked = save;			// restore item_clicked
  }
  
public:
  Fl_Tree(int X, int Y, int W, int H, const char *L=0);
  ~Fl_Tree();
  int handle(int e);
  void draw();
  
  ///////////////////////
  // root methods
  ///////////////////////
  
  /// Set the label for the root item. 
  ///
  /// Makes an internally managed copy of 'new_label'.
  ///
  void root_label(const char *new_label) {
    if ( ! _root ) return;
    _root->label(new_label);
  }
  /// Returns the root item.
  Fl_Tree_Item* root() {
    return(_root);
  }
  
  ////////////////////////////////
  // Item creation/removal methods
  ////////////////////////////////
  Fl_Tree_Item *add(const char *path);
  Fl_Tree_Item* add(Fl_Tree_Item *item, const char *name);
  Fl_Tree_Item *insert_above(Fl_Tree_Item *above, const char *name);
  Fl_Tree_Item* insert(Fl_Tree_Item *item, const char *name, int pos);
  
  /// Remove the specified \p item from the tree.
  /// If it has children, all those are removed too.
  /// \returns 0 if done, -1 if 'item' not found.
  ///
  int remove(Fl_Tree_Item *item) {
    if ( !item ) return(0);
    if ( item == _root ) {
      clear();
    } else {
      Fl_Tree_Item *parent = item->parent();		// find item's parent
      if ( ! parent ) return(-1);
      parent->remove_child(item);			// remove child + children
    }
    return(0);
  } 
  /// Clear all children from the tree.
  /// The tree will be left completely empty.
  ///
  void clear() {
    if ( ! _root ) return;
    _root->clear_children();
    delete _root; _root = 0;
  } 
  /// Clear all the children of a particular node in the tree specified by \p item.
  void clear_children(Fl_Tree_Item *item) {
    if ( item->has_children() ) {
      item->clear_children();
      redraw();						// redraw only if there were children to clear
    }
  } 
  
  ////////////////////////
  // Item lookup methods
  ////////////////////////
  Fl_Tree_Item *find_item(const char *path);
  const Fl_Tree_Item *find_item(const char *path) const;
  
  /// Return the item that was last clicked.
  ///
  /// Valid only from within an Fl_Tree::callback().
  ///
  /// \returns the item clicked, or 0 if none.
  ///          0 may also be used to indicate several items were clicked/changed.
  ///
  Fl_Tree_Item *item_clicked() {
    return(_item_clicked);
  }
  /// Returns the first item in the tree.
  ///
  /// Use this to walk the tree in the forward direction, eg:
  /// \code
  /// for ( Fl_Tree_Item *item = tree->first(); item; item = item->next() ) {
  ///     printf("Item: %s\n", item->label());
  /// }
  /// \endcode
  ///
  /// \returns first item in tree, or 0 if none (tree empty).
  ///
  Fl_Tree_Item *first() {
    return(_root);					// first item always root
  }
  /// Returns the last item in the tree.
  ///
  /// Use this to walk the tree in reverse, eg:
  ///
  /// \code
  /// for ( Fl_Tree_Item *item = tree->last(); item; item = item->prev() ) {
  ///     printf("Item: %s\n", item->label());
  /// }
  /// \endcode
  ///
  /// \returns last item in the tree, or 0 if none (tree empty).
  ///
  Fl_Tree_Item *last() {
    if ( ! _root ) return(0);
    Fl_Tree_Item *item = _root;
    while ( item->has_children() ) {
      item = item->child(item->children()-1);
    }
    return(item);
  }
  
  //////////////////////////
  // Item open/close methods
  //////////////////////////
  
  /// Open the specified 'item'.
  /// This causes the item's children (if any) to be shown.
  /// Handles redrawing if anything was actually changed.
  ///
  void open(Fl_Tree_Item *item) {
    if ( ! item->is_open() ) {
      item->open();
      redraw();
    }
  }
  /// Opens the item specified by \p path (eg: "Parent/child/item").
  /// This causes the item's children (if any) to be shown.
  /// Handles redrawing if anything was actually changed.
  ///
  /// \returns
  ///     -   0 : OK
  ///     -  -1 : item was not found
  ///         
  int open(const char *path) {
    Fl_Tree_Item *item = find_item(path);
    if ( item ) {
      open(item);
      return(0);
    }
    return(-1);
  }
  /// Closes the specified \p item.
  /// Handles redrawing if anything was actually changed.
  ///
  void close(Fl_Tree_Item *item) {
    if ( ! item->is_close() ) {
      item->close();
      redraw();
    }
  }
  /// Closes the item specified by \p path, eg: "Parent/child/item".
  ///
  /// Handles redrawing if anything was actually changed.
  ///
  /// \returns
  ///     -   0 -- OK
  ///     -  -1 -- item was not found
  ///         
  int close(const char *path) {
    Fl_Tree_Item *item = find_item(path);
    if ( item ) {
      close(item);
      return(0);
    }
    return(-1);
  }
  /// See if \p item is open.
  ///
  /// Items that are 'open' are themselves not necessarily visible;
  /// one of the item's parents might be closed.
  ///
  /// \returns
  ///     -  1 : item is open
  ///     -  0 : item is closed
  ///
  int is_open(Fl_Tree_Item *item) const {
    return(item->is_open()?1:0);
  }
  /// See if item specified by \p path (eg: "Parent/child/item") is open.
  ///
  /// Items that are 'open' are themselves not necessarily visible;
  /// one of the item's parents might be closed.
  ///
  /// \returns
  ///     -    1 : item is open
  ///     -    0 : item is closed
  ///     -   -1 : item was not found
  ///
  int is_open(const char *path) const {
    const Fl_Tree_Item *item = find_item(path);
    if ( item ) return(item->is_open()?1:0);
    return(-1);
  }
  /// See if the specified \p item is closed.
  /// \returns
  ///     -   1 : item is open
  ///     -   0 : item is closed
  ///
  int is_close(Fl_Tree_Item *item) const {
    return(item->is_close());
  }
  /// See if item specified by \p path (eg: "Parent/child/item") is closed.
  ///
  /// \returns
  ///     -   1 : item is closed
  ///     -   0 : item is open
  ///     -  -1 : item was not found
  ///
  int is_close(const char *path) const {
    const Fl_Tree_Item *item = find_item(path);
    if ( item ) return(item->is_close()?1:0);
    return(-1);
  }
  
  /////////////////////////
  // Item selection methods
  /////////////////////////
  
  /// Select the specified \p item. Use 'deselect()' to de-select it.
  /// Handles redrawing if anything was actually changed.
  ///
  /// \p docallback is an optional paramemter that can either be 0 or 1.
  ///     - 0 - the callback() is not invoked (default)
  ///     - 1 - the callback() is invoked if the item changed state,
  ///           and the callback can use item_clicked() to determine the selected item.
  ///
  void select(Fl_Tree_Item *item, int docallback=0) {
    if ( ! item->is_selected() ) {
      item->select();
      if ( docallback == 1 ) do_callback_for_item(item);
      redraw();
    }
  }
  /// Select the item specified by \p path (eg: "Parent/child/item").
  /// Handles redrawing if anything was actually changed.
  ///
  /// \p docallback is an optional paramemter that can either be 0 or 1.
  ///     - 0 - the callback() is not invoked (default)
  ///     - 1 - the callback() is invoked if the item changed state,
  ///           and the callback can use item_clicked() to determine the selected item.
  ///
  /// \returns
  ///     -   0 : OK
  ///     -  -1 : item was not found
  ///
  int select(const char *path, int docallback=0) {
    Fl_Tree_Item *item = find_item(path);
    if ( item ) {
      select(item);
      if ( docallback == 1 ) do_callback_for_item(item);
      return(0);
    }
    return(-1);
  }
  /// Toggle the select state of the specified \p item.
  /// Handles redrawing.
  ///
  /// \p docallback is an optional paramemter that can either be 0 or 1.
  ///     - 0 - the callback() is not invoked (default)
  ///     - 1 - the callback() is invoked,
  ///           and the callback can use item_clicked() to determine the selected item.
  ///
  void select_toggle(Fl_Tree_Item *item, int docallback=0) {
    item->select_toggle();
    if ( docallback == 1 ) do_callback_for_item(item);
    redraw();
  }
  /// De-select the specified \p item.
  /// Handles redrawing if anything was actually changed.
  ///
  /// \p docallback is an optional paramemter that can either be 0 or 1.
  ///     - 0 - the callback() is not invoked (default)
  ///     - 1 - the callback() is invoked if the item changed state,
  ///           and the callback can use item_clicked() to determine the selected item.
  ///
  void deselect(Fl_Tree_Item *item, int docallback=0) {
    if ( item->is_selected() ) {
      item->deselect();
      if ( docallback == 1 ) do_callback_for_item(item);
      redraw();
    }
  }
  /// De-select an item specified by \p path (eg: "Parent/child/item").
  /// Handles redrawing if anything was actually changed.
  ///
  /// \p docallback is an optional paramemter that can either be 0 or 1.
  ///     - 0 - the callback() is not invoked (default)
  ///     - 1 - the callback() is invoked if the item changed state,
  ///           and the callback can use item_clicked() to determine the selected item.
  ///
  ///  \returns
  ///     -   0 : OK
  ///     -  -1 : item was not found
  ///
  int deselect(const char *path, int docallback=0) {
    Fl_Tree_Item *item = find_item(path);
    if ( item ) {
      deselect(item, docallback);
      return(0);
    }
    return(-1);
  }
  
  int deselect_all(Fl_Tree_Item *item=0, int docallback=0);
  int select_only(Fl_Tree_Item *selitem, int docallback=0);
  int select_all(Fl_Tree_Item *item=0, int docallback=0);
  
  /// See if the specified \p item is selected.
  /// \return
  ///     -   1 : item selected
  ///     -   0 : item deselected
  ///
  int is_selected(Fl_Tree_Item *item) const {
    return(item->is_selected()?1:0);
  }
  /// See if item specified by \p path (eg: "Parent/child/item") is selected.
  ///
  /// \returns
  ///     -   1 : item selected
  ///     -   0 : item deselected
  ///     -  -1 : item was not found
  ///
  int is_selected(const char *path) {
    Fl_Tree_Item *item = find_item(path);
    if ( item ) return(is_selected(item));
    return(-1);
  }
  /// Print the tree as 'ascii art' to stdout.
  /// Used mainly for debugging.
  ///
  void show_self() {
    if ( ! _root ) return;
    _root->show_self();
  }
  
  /////////////////////////////////
  // Item attribute related methods
  /////////////////////////////////
  
  /// Get the default label fontsize used for creating new items.
  int labelsize() const {
    return(_prefs.labelsize());
  }
  /// Set the default label font size used for creating new items.
  /// To change the font size on a per-item basis, use Fl_Tree_Item::labelsize(int)
  ///
  void labelsize(int val) {
    _prefs.labelsize(val);
  }
  
  /// Get the default font face used for item's labels when new items are created.
  ///
  /// Don't use this if you want to change an existing label() size; use
  /// item->labelfont() instead.
  ///
  int labelfont() const {
    return(_prefs.labelfont());
  }
  /// Set the default font face used for item's labels when new items are created.
  ///
  /// Don't use this if you want to change an existing label() size; use
  /// item->labelfont(int) instead.
  ///
  void labelfont(int val) {
    _prefs.labelfont(val);
  }
  /// Get the amount of white space (in pixels) that should appear
  /// between the widget's left border and the tree's contents.
  ///
  int  marginleft() const {
    return(_prefs.marginleft());
  }
  /// Set the amount of white space (in pixels) that should appear
  /// between the widget's left border and the left side of the tree's contents.
  ///
  void marginleft(int val) {
    _prefs.marginleft(val);
    redraw();
  }
  /// Get the amount of white space (in pixels) that should appear
  /// between the widget's top border and the top of the tree's contents.
  ///
  int  margintop() const {
    return(_prefs.margintop());
  }
  /// Sets the amount of white space (in pixels) that should appear
  /// between the widget's top border and the top of the tree's contents.
  ///
  void margintop(int val) {
    _prefs.margintop(val);
    redraw();
  }
  /// Get the amount of white space (in pixels) that should appear
  /// below an open child tree's contents.
  ///
  int  openchild_marginbottom() const {
    return(_prefs.openchild_marginbottom());
  }
  /// Set the amount of white space (in pixels) that should appear
  /// below an open child tree's contents.
  ///
  void openchild_marginbottom(int val) {
    _prefs.openchild_marginbottom(val);
    redraw();
  }
  /// Gets the width of the horizontal connection lines (in pixels) 
  /// that appear to the left of each tree item's label.
  ///
  int  connectorwidth() const {
    return(_prefs.connectorwidth());
  }
  /// Sets the width of the horizontal connection lines (in pixels) 
  /// that appear to the left of each tree item's label.
  ///
  void connectorwidth(int val) {
    _prefs.connectorwidth(val);
    redraw();
  }
  /// Returns the Fl_Image being used as the default user icon for newly created items.
  /// Returns zero if no icon has been set, which is the default.
  ///
  Fl_Image *usericon() const {
    return(_prefs.usericon());
  }
  /// Sets the Fl_Image to be used as the default user icon for all
  /// newly created items.
  ///
  /// If you want to specify user icons on a per-item basis,
  /// use Fl_Tree_Item::usericon() instead.
  ///
  /// \param[in] val -- The new image to be used, or
  ///                   zero to disable user icons.
  ///
  void usericon(Fl_Image *val) {
    _prefs.usericon(val);
    redraw();
  }
  /// Returns the icon to be used as the 'open' icon.
  /// If none was set, the internal default is returned,
  /// a simple '[+]' icon.
  ///
  Fl_Image *openicon() const {
    return(_prefs.openicon());
  }
  /// Sets the icon to be used as the 'open' icon.
  /// This overrides the built in default '[+]' icon.
  ///
  /// \param[in] val -- The new image, or zero to use the default [+] icon.
  ///
  void openicon(Fl_Image *val) {
    _prefs.openicon(val);
    redraw();
  }
  /// Returns the icon to be used as the 'close' icon.
  /// If none was set, the internal default is returned,
  /// a simple '[-]' icon.
  ///
  Fl_Image *closeicon() const {
    return(_prefs.closeicon());
  }
  /// Sets the icon to be used as the 'close' icon.
  /// This overrides the built in default '[-]' icon.
  ///
  /// \param[in] val -- The new image, or zero to use the default [-] icon.
  ///
  void closeicon(Fl_Image *val) {
    _prefs.closeicon(val);
    redraw();
  }
  /// Returns 1 if the collapse icon is enabled, 0 if not.
  int showcollapse() const {
    return(_prefs.showcollapse());
  }
  /// Set if we should show the collapse icon or not.
  /// If collapse icons are disabled, the user will not be able
  /// to interactively collapse items in the tree, unless the application
  /// provides some other means via open() and close().
  ///
  /// \param[in] val 1: shows collapse icons (default),\n
  ///                0: hides collapse icons.
  ///
  void showcollapse(int val) {
    _prefs.showcollapse(val);
    redraw();
  }
  /// Returns 1 if the root item is to be shown, or 0 if not.
  int showroot() const {
    return(_prefs.showroot());
  }
  /// Set if the root item should be shown or not.
  /// \param[in] val 1 -- show the root item (default)\n
  ///                0 -- hide the root item.
  ///
  void showroot(int val) {
    _prefs.showroot(val);
    redraw();
  }
  /// Returns the line drawing style for inter-connecting items.
  Fl_Tree_Connector connectorstyle() const {
    return(_prefs.connectorstyle());
  }
  /// Sets the line drawing style for inter-connecting items.
  void connectorstyle(Fl_Tree_Connector val) {
    _prefs.connectorstyle(val);
    redraw();
  }
  /// Set the default sort order used when items are added to the tree.
  ///     See Fl_Tree_Sort for possible values.
  ///
  Fl_Tree_Sort sortorder() const {
    return(_prefs.sortorder());
  }
  /// Gets the sort order used to add items to the tree.
  void sortorder(Fl_Tree_Sort val) {
    _prefs.sortorder(val);
    // no redraw().. only affects new add()itions
  }
  /// Sets the style of box used to draw selected items.
  /// This is an fltk Fl_Boxtype.
  /// The default is influenced by FLTK's current Fl::scheme()
  ///
  Fl_Boxtype selectbox() const {
    return(_prefs.selectbox());
  }
  /// Gets the style of box used to draw selected items.
  /// This is an fltk Fl_Boxtype.
  /// The default is influenced by FLTK's current Fl::scheme()
  ///
  void selectbox(Fl_Boxtype val) {
    _prefs.selectbox(val);
    redraw();
  }
  /// Gets the tree's current selection mode.
  Fl_Tree_Select selectmode() const {
    return(_prefs.selectmode());
  }
  /// Sets the tree's selection mode.
  void selectmode(Fl_Tree_Select val) {
    _prefs.selectmode(val);
  }
  
  void load(class Fl_Preferences&);
};

#endif /*FL_TREE_H*/

//
// End of "$Id: Fl_Tree.H 7672 2010-07-10 09:44:45Z matt $".
//
