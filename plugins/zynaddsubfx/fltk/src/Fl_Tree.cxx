//
// "$Id: Fl_Tree.cxx 7672 2010-07-10 09:44:45Z matt $"
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FL/Fl_Tree.H>
#include <FL/Fl_Preferences.H>

#define SCROLL_W 15

//////////////////////
// Fl_Tree.cxx
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

// INTERNAL: scroller callback
static void scroll_cb(Fl_Widget*,void *data) {
  ((Fl_Tree*)data)->redraw();
}

// INTERNAL: Parse elements from path into an array of null terminated strings
//    Path="/aa/bb"
//    Return: arr[0]="aa", arr[1]="bb", arr[2]=0
//    Caller must: free(arr[0]); free(arr);
//
static char **parse_path(const char *path) {
  while ( *path == '/' ) path++;	// skip leading '/' 
  // First pass: identify, null terminate, and count separators
  int seps = 1;			// separator count (1: first item)
  int arrsize = 1;			// array size (1: first item)
  char *save = strdup(path);		// make copy we can modify
  char *s = save;
  while ( ( s = strchr(s, '/') ) ) {
    while ( *s == '/' ) { *s++ = 0; seps++; }
    if ( *s ) { arrsize++; }
  }
  arrsize++;				// (room for terminating NULL) 
  // Second pass: create array, save nonblank elements
  char **arr = (char**)malloc(sizeof(char*) * arrsize);
  int t = 0;
  s = save;
  while ( seps-- > 0 ) {
    if ( *s ) { arr[t++] = s; }	// skips empty fields, eg. '//'
    s += (strlen(s) + 1);
  }
  arr[t] = 0;
  return(arr);
}

// INTERNAL: Recursively descend tree hierarchy, accumulating total child count
static int find_total_children(Fl_Tree_Item *item, int count=0) {
  count++;
  for ( int t=0; t<item->children(); t++ ) {
    count = find_total_children(item->child(t), count);
  }
  return(count);
}

/// Constructor.
Fl_Tree::Fl_Tree(int X, int Y, int W, int H, const char *L) : Fl_Group(X,Y,W,H,L) { 
  _root = new Fl_Tree_Item(_prefs);
  _root->parent(0);				// we are root of tree
  _root->label("ROOT");
  _item_clicked = 0;
  box(FL_DOWN_BOX);
  color(FL_WHITE);
  when(FL_WHEN_CHANGED);
  _vscroll = new Fl_Scrollbar(0,0,0,0);	// will be resized by draw()
  _vscroll->hide();
  _vscroll->type(FL_VERTICAL);
  _vscroll->step(1);
  _vscroll->callback(scroll_cb, (void*)this);
  end();
}

/// Destructor.
Fl_Tree::~Fl_Tree() {
  if ( _root ) { delete _root; _root = 0; }
}

/// Adds a new item, given a 'menu style' path, eg: "/Parent/Child/item".
/// Any parent nodes that don't already exist are created automatically.
/// Adds the item based on the value of sortorder().
/// \returns the child item created, or 0 on error.
///
Fl_Tree_Item* Fl_Tree::add(const char *path) {
  if ( ! _root ) {					// Create root if none
    _root = new Fl_Tree_Item(_prefs);
    _root->parent(0);
    _root->label("ROOT");
  }
  char **arr = parse_path(path);
  Fl_Tree_Item *item = _root->add(_prefs, arr);
  free((void*)arr[0]);
  free((void*)arr);
  return(item);
}

/// Inserts a new item above the specified Fl_Tree_Item, with the label set to 'name'.
/// \returns the item that was added, or 0 if 'above' could not be found.
/// 
Fl_Tree_Item* Fl_Tree::insert_above(Fl_Tree_Item *above, const char *name) {
  return(above->insert_above(_prefs, name));
}

/// Insert a new item into a tree-item's children at a specified position.
/// \returns the item that was added.
Fl_Tree_Item* Fl_Tree::insert(Fl_Tree_Item *item, const char *name, int pos) {
  return(item->insert(_prefs, name, pos));
}

/// Add a new child to a tree-item.
/// \returns the item that was added.
Fl_Tree_Item* Fl_Tree::add(Fl_Tree_Item *item, const char *name) {
  return(item->add(_prefs, name));
}

/// Find the item, given a menu style path, eg: "/Parent/Child/item".
///
/// There is both a const and non-const version of this method.
/// Const version allows pure const methods to use this method 
/// to do lookups without causing compiler errors.
/// \returns the item, or 0 if not found.
///
Fl_Tree_Item *Fl_Tree::find_item(const char *path) {
  if ( ! _root ) return(0);
  char **arr = parse_path(path);
  Fl_Tree_Item *item = _root->find_item(arr);
  free((void*)arr[0]);
  free((void*)arr);
  return(item);
}

/// A const version of Fl_Tree::find_item(const char *path)
const Fl_Tree_Item *Fl_Tree::find_item(const char *path) const {
  if ( ! _root ) return(0);
  char **arr = parse_path(path);
  const Fl_Tree_Item *item = _root->find_item(arr);
  free((void*)arr[0]);
  free((void*)arr);
  return(item);
}

/// Standard FLTK draw() method, handles draws the tree widget.
void Fl_Tree::draw() {
  // Let group draw box+label but *NOT* children.
  // We handle drawing children ourselves by calling each item's draw()
  //
  Fl_Group::draw_box();
  Fl_Group::draw_label();
  if ( ! _root ) return;
  int cx = x() + Fl::box_dx(box());
  int cy = y() + Fl::box_dy(box());
  int cw = w() - Fl::box_dw(box());
  int ch = h() - Fl::box_dh(box());
  // These values are changed during drawing
  // 'Y' will be the lowest point on the tree
  int X = cx + _prefs.marginleft();
  int Y = cy + _prefs.margintop() - (_vscroll->visible() ? _vscroll->value() : 0);
  int W = cw - _prefs.marginleft();		// - _prefs.marginright();
  int Ysave = Y;
  fl_push_clip(cx,cy,cw,ch);
  {
    fl_font(_prefs.labelfont(), _prefs.labelsize());
    _root->draw(X, Y, W, this, _prefs);
  }
  fl_pop_clip();
  
  // Show vertical scrollbar?
  int ydiff = (Y+_prefs.margintop())-Ysave;		// ydiff=size of tree
  int ytoofar = (cy+ch) - Y;				// ytoofar -- scrolled beyond bottom (eg. stow)
  
  //printf("ydiff=%d ch=%d Ysave=%d ytoofar=%d value=%d\n",
  //int(ydiff),int(ch),int(Ysave),int(ytoofar), int(_vscroll->value()));
  
  if ( ytoofar > 0 ) ydiff += ytoofar;
  if ( Ysave<cy || ydiff > ch || int(_vscroll->value()) > 1 ) {
    _vscroll->visible();
    int sx = x()+w()-Fl::box_dx(box())-SCROLL_W;
    int sy = y()+Fl::box_dy(box());
    int sw = SCROLL_W;
    int sh = h()-Fl::box_dh(box());
    _vscroll->show();
    _vscroll->range(0.0,ydiff-ch);
    _vscroll->resize(sx,sy,sw,sh);
    _vscroll->slider_size(float(ch)/float(ydiff));
  } else {
    _vscroll->Fl_Slider::value(0);
    _vscroll->hide();
  }
  fl_push_clip(cx,cy,cw,ch);
  Fl_Group::draw_children();	// draws any FLTK children set via Fl_Tree::widget()
  fl_pop_clip();
}

/// Standard FLTK event handler for this widget.
int Fl_Tree::handle(int e) {
  static Fl_Tree_Item *lastselect = 0;
  int changed = 0;
  int ret = Fl_Group::handle(e);
  if ( ! _root ) return(ret);
  switch ( e ) {
    case FL_PUSH: {
      lastselect = 0;
      item_clicked(0);				// assume no item was clicked
      Fl_Tree_Item *o = _root->find_clicked(_prefs);
      if ( o ) {
        ret |= 1;				// handled
        if ( Fl::event_button() == FL_LEFT_MOUSE ) {
          // Was collapse icon clicked?
          if ( o->event_on_collapse_icon(_prefs) ) {
            o->open_toggle();
            redraw();
          }
          // Item's label clicked?
          else if ( o->event_on_label(_prefs) && 
                   (!o->widget() || !Fl::event_inside(o->widget())) &&
                   callback() &&
                   (!_vscroll->visible() || !Fl::event_inside(_vscroll)) ) {
            item_clicked(o);			// save item clicked

            // Handle selection behavior
            switch ( _prefs.selectmode() ) {
              case FL_TREE_SELECT_NONE: {	// no selection changes
                break;
              }
              case FL_TREE_SELECT_SINGLE: {
                changed = select_only(o);
                break;
              }
              case FL_TREE_SELECT_MULTI: {
                int state = Fl::event_state();
                if ( state & FL_SHIFT ) {
                  if ( ! o->is_selected() ) {
                    o->select();		// add to selection
                    changed = 1;		// changed
                  }
                } else if ( state & FL_CTRL ) {
                  changed = 1;			// changed
                  o->select_toggle();		// toggle selection state
                  lastselect = o;		// save we toggled it (prevents oscillation)
                } else {
                  changed = select_only(o);
                }
                break;
              }
            }

            if ( changed ) {
              redraw();						// make change(s) visible
              if ( when() & FL_WHEN_CHANGED ) {
                set_changed();
                do_callback((Fl_Widget*)this, user_data());	// item callback
              }
            }
          }
        }
      }
      break;
    }
    case FL_DRAG: {
      if ( Fl::event_button() != FL_LEFT_MOUSE ) break;
      Fl_Tree_Item *o = _root->find_clicked(_prefs);
      if ( o ) {
        ret |= 1;				// handled
        // Item's label clicked?
        if ( o->event_on_label(_prefs) && 
	  (!o->widget() || !Fl::event_inside(o->widget())) &&
	  callback() &&
	  (!_vscroll->visible() || !Fl::event_inside(_vscroll)) ) {
          item_clicked(o);			// save item clicked
          // Handle selection behavior
          switch ( _prefs.selectmode() ) {
            case FL_TREE_SELECT_NONE: {		// no selection changes
              break;
            }
            case FL_TREE_SELECT_SINGLE: {
              changed = select_only(o);
              break;
            }
            case FL_TREE_SELECT_MULTI: {
              int state = Fl::event_state();
              if ( state & FL_CTRL ) {
                if ( lastselect != o ) {// not already toggled from last microdrag?
                  changed = 1;	// changed
                  o->select_toggle();	// toggle selection
                  lastselect = o;	// save we toggled it (prevents oscillation)
                }
              } else {
	        if ( ! o->is_selected() ) {
                  changed = 1;		// changed
                  o->select();		// select this
	        }
              }
              break;
            }
          }
          if ( changed ) {
            redraw();			// make change(s) visible
            if ( when() & FL_WHEN_CHANGED ) {
              set_changed();
              do_callback((Fl_Widget*)this, user_data());	// item callback
            }
          }
        }
      }
      break;
    }
    case FL_RELEASE: {
      if ( Fl::event_button() == FL_LEFT_MOUSE ) {
        ret |= 1;
        if ( when() & FL_WHEN_RELEASE || ( this->changed() && (when() & FL_WHEN_CHANGED)) ) {
	  do_callback((Fl_Widget*)this, user_data());       // item callback
        }
      }
      break;
    }
  }
  return(ret);
}

/// Deselect item and all its children.
///     If item is NULL, root() is used.
///     Handles calling redraw() if anything was changed.
///     Returns count of how many items were in the 'selected' state,
///     ie. how many items were "changed".
///
///     \p docallback is an optional paramemter that can either be 0 or 1:
///
///     - 0 - the callback() is not invoked (default)
///     - 1 - the callback() is invoked once if \b any items changed state,
///           and item_clicked() will be NULL (since many items could have been changed).
//
/// \todo deselect_all()'s docallback should support '2' (invoke callback for each item changed)
///
int Fl_Tree::deselect_all(Fl_Tree_Item *item, int docallback) {
  item = item ? item : root();			// NULL? use root()
  int count = item->deselect_all();
  if ( count ) {
    redraw();					// anything changed? cause redraw
    if ( docallback == 1 )
      do_callback_for_item(0);
  }
  return(count);
}

/// Select item and all its children.
///     If item is NULL, root() is used.
///     Handles calling redraw() if anything was changed.
///     Returns count of how many items were in the 'deselected' state,
///     ie. how many items were "changed".
///
///     \p docallback is an optional paramemter that can either be 0 or 1:
///
///     - 0 - the callback() is not invoked (default)
///     - 1 - the callback() is invoked once if \b any items changed state,
///           and item_clicked() will be NULL (since many items could have been changed).
///
/// \todo select_all()'s docallback should support '2' (invoke callback for each item changed)
///
int Fl_Tree::select_all(Fl_Tree_Item *item, int docallback) {
  item = item ? item : root();			// NULL? use root()
  int count = item->select_all();
  if ( count ) {
    redraw();					// anything changed? cause redraw
    if (docallback == 1)
      do_callback_for_item(0);
  }
  return(count);
}

/// Select only this item.
///     If item is NULL, root() is used.
///     Handles calling redraw() if anything was changed.
///     Returns how many items were changed, if any.
///
///     \p docallback is an optional paramemter that can either be 0, 1 or 2:
///
///     - 0 - the callback() is not invoked (default)
///     - 1 - the callback() is invoked once if \b any items changed state,
///          and item_clicked() will be NULL (since many items could have been changed).
///     - 2 - the callback() is invoked once for \b each item that changed state,
///          and the callback() can use item_clicked() to determine the item changed.
///
int Fl_Tree::select_only(Fl_Tree_Item *selitem, int docallback) {
  selitem = selitem ? selitem : root();		// NULL? use root()
  int changed = 0;
  for ( Fl_Tree_Item *item = first(); item; item = item->next() ) {
    if ( item == selitem ) {
      if ( item->is_selected() ) continue;	// don't count if already selected
      item->select();
      ++changed;
      if ( docallback == 2 ) do_callback_for_item(item);
    } else {
      if ( item->is_selected() ) {
        item->deselect();
        ++changed;
        if ( docallback == 2 ) do_callback_for_item(item);
      }
    }
  }
  if ( changed ) {
    redraw();			// anything changed? redraw
    if ( docallback == 1 ) do_callback_for_item(0);
  }
  return(changed);
}

/**
 * Read a preferences database into the tree widget.
 * A preferences database is a hierarchical collection of data which can be
 * directly loaded into the tree view for inspection.
 * \param[in] prefs the Fl_Preferences database
 */
void Fl_Tree::load(Fl_Preferences &prefs) 
{
  int i, j, n, pn = strlen(prefs.path());
  char *p;
  const char *path = prefs.path();
  if (strcmp(path, ".")==0)
    path += 1; // root path is empty
  else
    path += 2; // child path starts with "./"
  n = prefs.groups();
  for (i=0; i<n; i++) {
    Fl_Preferences prefsChild(prefs, i);
    add(prefsChild.path()+2); // children always start with "./"
    load(prefsChild);
  }
  n = prefs.entries();
  for (i=0; i<n; i++) {
    // We must remove all fwd slashes in the key and value strings. Replace with backslash.
    char *key = strdup(prefs.entry(i));
    int kn = strlen(key);
    for (j=0; j<kn; j++) {
      if (key[j]=='/') key[j]='\\'; 
    }
    char *val;  prefs.get(key, val, "");
    int vn = strlen(val);
    for (j=0; j<vn; j++) {
      if (val[j]=='/') val[j]='\\'; 
    }
    if (vn<40) {
      int sze = pn + strlen(key) + vn;
      p = (char*)malloc(sze+5);
      sprintf(p, "%s/%s = %s", path, key, val);
    } else {
      int sze = pn + strlen(key) + 40;
      p = (char*)malloc(sze+5);
      sprintf(p, "%s/%s = %.40s...", path, key, val);
    }
    add(p[0]=='/'?p+1:p);
    free(p);
    free(val);
    free(key);
  }
}

//
// End of "$Id: Fl_Tree.cxx 7672 2010-07-10 09:44:45Z matt $".
//
