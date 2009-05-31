//
// "$Id: Fl_Preferences.H 6770 2009-04-18 18:52:59Z matt $"
//
// Preferences .
//
// Copyright 2002-2009 by Matthias Melcher.
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
   Fl_Preferences class . */

#ifndef Fl_Preferences_H
#  define Fl_Preferences_H

#  include <stdio.h>
#  include "Fl_Export.H"


/**
   \brief Fl_Preferences provides methods to store user
   settings between application starts. 

   It is similar to the
   Registry on WIN32 and Preferences on MacOS, and provides a
   simple configuration mechanism for UNIX.
   
   Fl_Preferences uses a hierarchy to store data. It
   bundles similar data into groups and manages entries into those
   groups as name/value pairs.
   
   Preferences are stored in text files that can be edited
   manually. The file format is easy to read and relatively
   forgiving. Preferences files are the same on all platforms. User
   comments in preference files are preserved. Filenames are unique
   for each application by using a vendor/application naming
   scheme. The user must provide default values for all entries to
   ensure proper operation should preferences be corrupted or not
   yet exist.
   
   Entries can be of any length. However, the size of each
   preferences file should be kept under 100k for performance
   reasons. One application can have multiple preferences files.
   Extensive binary data however should be stored in separate
   files: see getUserdataPath().

   \note Starting with FLTK 1.3, preference databases are expected to 
   be in utf8 encoding. Previous databases were stored in the 
   current chracter set or code page which renders them incompatible
   for text entries using international characters.
 */
class FL_EXPORT Fl_Preferences 
{

public:

  /**
     Define the scope of the preferences.
   */
  enum Root { 
    SYSTEM=0,   ///< Preferences are used system-wide
    USER        ///< Preferences apply only to the current user
  };

  Fl_Preferences( Root root, const char *vendor, const char *application );
  Fl_Preferences( const char *path, const char *vendor, const char *application );
  Fl_Preferences( Fl_Preferences &parent, const char *group );
  Fl_Preferences( Fl_Preferences*, const char *group );
  ~Fl_Preferences();

  int groups();
  const char *group( int num_group );
  char groupExists( const char *key );
  char deleteGroup( const char *group );

  int entries();
  const char *entry( int index );
  char entryExists( const char *key );
  char deleteEntry( const char *entry );

  char set( const char *entry, int value );
  char set( const char *entry, float value );
  char set( const char *entry, float value, int precision );
  char set( const char *entry, double value );
  char set( const char *entry, double value, int precision );
  char set( const char *entry, const char *value );
  char set( const char *entry, const void *value, int size ); 
  
  char get( const char *entry, int &value, int defaultValue );
  char get( const char *entry, float &value,  float defaultValue );
  char get( const char *entry, double &value, double defaultValue );
  char get( const char *entry, char *&value,  const char *defaultValue );
  char get( const char *entry, char *value,   const char *defaultValue, int maxSize );
  char get( const char *entry, void *&value,  const void *defaultValue, int defaultSize );
  char get( const char *entry, void *value,   const void *defaultValue, int defaultSize, int maxSize );

  int size( const char *entry );

  char getUserdataPath( char *path, int pathlen );

  void flush();

  // char export( const char *filename, Type fileFormat );
  // char import( const char *filename );

  /**
     'Name' provides a simple method to create numerical or more complex 
     procedural names for entries and groups on the fly.
     
     Example: prefs.set(Fl_Preferences::Name("File%d",i),file[i]);. 
    
     See test/preferences.cxx as a sample for writing arrays into preferences.<p>
     'Name' is actually implemented as a class inside Fl_Preferences. It casts
     into const char* and gets automatically destroyed after the enclosing call
     ends.
   */
  class FL_EXPORT Name {

    char *data_;

  public:

    Name( unsigned int n );
    Name( const char *format, ... );

    /**
       Return the Name as a "C" string.
       \internal
     */
    operator const char *() { return data_; }
    ~Name();
  };

  /** \internal An entry associates a preference name to its corresponding value */
  struct Entry
  {
    char *name, *value;
  };

private:

  // make the following functions unavailable
  Fl_Preferences(); 
  Fl_Preferences(const Fl_Preferences&); 
  Fl_Preferences &operator=(const Fl_Preferences&);

  static char nameBuffer[128];

  class FL_EXPORT Node // a node contains a list to all its entries 
  {          // and all means to manage the tree structure
    Node *child_, *next_, *parent_;
    char *path_;
    char dirty_;
  public:
    Node( const char *path );
    ~Node();
    // node methods
    int write( FILE *f );
    Node *find( const char *path );
    Node *search( const char *path, int offset=0 );
    Node *addChild( const char *path );
    void setParent( Node *parent );
    Node *parent() { return parent_; }
    char remove();
    char dirty();
    // entry methods
    int nChildren();
    const char *child( int ix );
    void set( const char *name, const char *value );
    void set( const char *line );
    void add( const char *line );
    const char *get( const char *name );
    int getEntry( const char *name );
    char deleteEntry( const char *name );
    // public values
    Entry *entry;
    int nEntry, NEntry;
    static int lastEntrySet;
  };
  friend class Node;

  class FL_EXPORT RootNode  // the root node manages file paths and basic reading and writing
  {
    Fl_Preferences *prefs_;
    char *filename_;
    char *vendor_, *application_;
  public:
    RootNode( Fl_Preferences *, Root root, const char *vendor, const char *application );
    RootNode( Fl_Preferences *, const char *path, const char *vendor, const char *application );
    ~RootNode();
    int read();
    int write();
    char getPath( char *path, int pathlen );
  };
  friend class RootNode;

  Node *node;
  RootNode *rootNode;
  
};


#endif // !Fl_Preferences_H

//
// End of "$Id: Fl_Preferences.H 6770 2009-04-18 18:52:59Z matt $".
//
