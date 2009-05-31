//
// "$Id: Fl_Preferences.cxx 6716 2009-03-24 01:40:44Z fabien $"
//
// Preferences methods for the Fast Light Tool Kit (FLTK).
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


#include <FL/Fl.H>
#include <FL/Fl_Preferences.H>
#include <FL/filename.H>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <FL/fl_utf8.h>
#include "flstring.h"
#include <sys/stat.h>

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <direct.h>
#  include <io.h>
// Visual C++ 2005 incorrectly displays a warning about the use of POSIX APIs
// on Windows, which is supposed to be POSIX compliant...
#  define access _access
#  define mkdir _mkdir
#elif defined (__APPLE__)
#  include <Carbon/Carbon.h>
#  include <unistd.h>
#else
#  include <unistd.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif // WIN32

char Fl_Preferences::nameBuffer[128];


/**
   The constructor creates a group that manages name/value pairs and
   child groups. Groups are ready for reading and writing at any time.
   The root argument is either Fl_Preferences::USER
   or Fl_Preferences::SYSTEM.
     
   This constructor creates the <i>base</i> instance for all
   following entries and reads existing databases into memory. The
   vendor argument is a unique text string identifying the
   development team or vendor of an application.  A domain name or
   an EMail address are great unique names, e.g.
   "researchATmatthiasm.com" or "fltk.org". The
   application argument can be the working title or final
   name of your application. Both vendor and
   application must be valid relative UNIX pathnames and
   may contain '/'s to create deeper file structures.
    
   \param[in] root can be \c USER or \c SYSTEM for user specific or system wide preferences
   \param[in] vendor unique text describing the company or author of this file
   \param[in] application unique text describing the application
*/
Fl_Preferences::Fl_Preferences( Root root, const char *vendor, const char *application )
{
  node = new Node( "." );
  rootNode = new RootNode( this, root, vendor, application );
}


/**
   \brief Use this constructor to create or read a preferences file at an
   arbitrary position in the file system. 

   The file name is generated in the form
   <tt><i>path</i>/<i>application</i>.prefs</tt>. If \p application
   is \c NULL, \p path must contain the full file name.
   
   \param[in] path path to the directory that contains the preferences file
   \param[in] vendor unique text describing the company or author of this file
   \param[in] application unique text describing the application
 */
Fl_Preferences::Fl_Preferences( const char *path, const char *vendor, const char *application )
{
  node = new Node( "." );
  rootNode = new RootNode( this, path, vendor, application );
}


/**
   \brief Generate or read a new group of entries within another group. 

   Use the \p group argument to name the group that you would like to access.
   \p Group can also contain a path to a group further down the hierarchy by
   separating group names with a forward slash '/'. 

   \param[in] parent reference object for the new group
   \param[in] group name of the group to access (may contain '/'s)
 */
Fl_Preferences::Fl_Preferences( Fl_Preferences &parent, const char *group )
{
  rootNode = parent.rootNode;
  node = parent.node->addChild( group );
}


/**
   \see Fl_Preferences( Fl_Preferences&, const char *group )
 */
Fl_Preferences::Fl_Preferences( Fl_Preferences *parent, const char *group )
{
  rootNode = parent->rootNode;
  node = parent->node->addChild( group );
}


/**
   The destructor removes allocated resources. When used on the
   \em base preferences group, the destructor flushes all
   changes to the preferences file and deletes all internal
   databases.

   The destructor does not remove any data from the database. It merely
   deletes your reference to the database.
 */
Fl_Preferences::~Fl_Preferences()
{
  if (node && !node->parent()) delete rootNode;
  // DO NOT delete nodes! The root node will do that after writing the preferences
  // zero all pointer to avoid memory errors, even though
  // Valgrind does not complain (Cygwind does though)
  node = 0L;
  rootNode = 0L;
}


/**
   Returns the number of groups that are contained within a group.
  
   \return 0 for no groups at all
 */
int Fl_Preferences::groups()
{
  return node->nChildren();
}


/**
   Returns the name of the Nth (\p num_group) group.
   There is no guaranteed order of group names. The index must
   be within the range given by groups().
     
   \param[in] num_group number indexing the requested group
   \return 'C' string pointer to the group name
 */
const char *Fl_Preferences::group( int num_group )
{
  return node->child( num_group );
}


/**
   Returns non-zero if a group with this name exists.
   Group names are relative to the Preferences node and can contain a path.
   "." describes the current node, "./" describes the topmost node.
   By preceding a groupname with a "./", its path becomes relative to the topmost node.
     
   \param[in] key name of group that is searched for
   \return 0 if no group by that name was found
 */
char Fl_Preferences::groupExists( const char *key )
{
  return node->search( key ) ? 1 : 0 ;
}


/**
   Deletes a group.

   Removes a group and all keys and groups within that group
   from the database.
   
   \param[in] group name of the group to delete
   \return 0 if call failed
 */
char Fl_Preferences::deleteGroup( const char *group )
{
  Node *nd = node->search( group );
  if ( nd ) return nd->remove();
  return 0;
}


/**
   Returns the number of entries (name/value pairs) in a group.
  
   \return number of entries
 */
int Fl_Preferences::entries()
{
  return node->nEntry;
}


/**
   Returns the name of an entry. There is no guaranteed order of
   entry names. The index must be within the range given by
   entries().
    
   \param[in] index number indexing the requested entry
   \return pointer to value cstring
 */
const char *Fl_Preferences::entry( int index )
{
  return node->entry[index].name;
}


/**
   Returns non-zero if an entry with this name exists.
     
   \param[in] key name of entry that is searched for
   \return 0 if entry was not found
 */
char Fl_Preferences::entryExists( const char *key )
{
  return node->getEntry( key )>=0 ? 1 : 0 ;
}


/**
   Deletes a single name/value pair.
   
   This function removes the entry \p key from the database.
 
   \param[in] key name of entry to delete
   \return 0 if deleting the entry failed
 */
char Fl_Preferences::deleteEntry( const char *key )
{
  return node->deleteEntry( key );
}


/**
 Reads an entry from the group. A default value must be
 supplied. The return value indicates if the value was available
 (non-zero) or the default was used (0).
 
 \param[in] key name of entry
 \param[out] value returned from preferences or default value if none was set
 \param[in] defaultValue default value to be used if no preference was set
 \return 0 if the default value was used 
 */
char Fl_Preferences::get( const char *key, int &value, int defaultValue )
{
  const char *v = node->get( key );
  value = v ? atoi( v ) : defaultValue;
  return ( v != 0 );
}


/** 
 Sets an entry (name/value pair). The return value indicates if there
 was a problem storing the data in memory. However it does not
 reflect if the value was actually stored in the preferences
 file.
 
 \param[in] key name of entry
 \param[in] value set this entry to \p value
 \return 0 if setting the value failed
 */
char Fl_Preferences::set( const char *key, int value )
{
  sprintf( nameBuffer, "%d", value );
  node->set( key, nameBuffer );
  return 1;
}


/**
 Reads an entry from the group. A default value must be
 supplied. The return value indicates if the value was available
 (non-zero) or the default was used (0). 
 
 \param[in] key name of entry
 \param[out] value returned from preferences or default value if none was set
 \param[in] defaultValue default value to be used if no preference was set
 \return 0 if the default value was used 
 */
char Fl_Preferences::get( const char *key, float &value, float defaultValue )
{
  const char *v = node->get( key );
  value = v ? (float)atof( v ) : defaultValue;
  return ( v != 0 );
}


/** 
 Sets an entry (name/value pair). The return value indicates if there
 was a problem storing the data in memory. However it does not
 reflect if the value was actually stored in the preferences
 file.
 
 \param[in] key name of entry
 \param[in] value set this entry to \p value
 \return 0 if setting the value failed
 */
char Fl_Preferences::set( const char *key, float value )
{
  sprintf( nameBuffer, "%g", value );
  node->set( key, nameBuffer );
  return 1;
}


/** 
 Sets an entry (name/value pair). The return value indicates if there
 was a problem storing the data in memory. However it does not
 reflect if the value was actually stored in the preferences
 file.
 
 \param[in] key name of entry
 \param[in] value set this entry to \p value
 \param[in] precision number of decimal digits to represent value
 \return 0 if setting the value failed
 */
char Fl_Preferences::set( const char *key, float value, int precision )
{
  sprintf( nameBuffer, "%.*g", precision, value );
  node->set( key, nameBuffer );
  return 1;
}


/**
 Reads an entry from the group. A default value must be
 supplied. The return value indicates if the value was available
 (non-zero) or the default was used (0). 
 
 \param[in] key name of entry
 \param[out] value returned from preferences or default value if none was set
 \param[in] defaultValue default value to be used if no preference was set
 \return 0 if the default value was used 
 */
char Fl_Preferences::get( const char *key, double &value, double defaultValue )
{
  const char *v = node->get( key );
  value = v ? atof( v ) : defaultValue;
  return ( v != 0 );
}


/** 
 Sets an entry (name/value pair). The return value indicates if there
 was a problem storing the data in memory. However it does not
 reflect if the value was actually stored in the preferences
 file.
 
 \param[in] key name of entry
 \param[in] value set this entry to \p value
 \return 0 if setting the value failed
 */
char Fl_Preferences::set( const char *key, double value )
{
  sprintf( nameBuffer, "%g", value );
  node->set( key, nameBuffer );
  return 1;
}


/** 
 Sets an entry (name/value pair). The return value indicates if there
 was a problem storing the data in memory. However it does not
 reflect if the value was actually stored in the preferences
 file.
 
 \param[in] key name of entry
 \param[in] value set this entry to \p value
 \param[in] precision number of decimal digits to represent value
 \return 0 if setting the value failed
 */
char Fl_Preferences::set( const char *key, double value, int precision )
{
  sprintf( nameBuffer, "%.*g", precision, value );
  node->set( key, nameBuffer );
  return 1;
}


// remove control sequences from a string
static char *decodeText( const char *src )
{
  int len = 0;
  const char *s = src;
  for ( ; *s; s++, len++ )
  {
    if ( *s == '\\' )
      if ( isdigit( s[1] ) ) s+=3; else s+=1;
  }
  char *dst = (char*)malloc( len+1 ), *d = dst;
  for ( s = src; *s; s++ )
  {
    char c = *s;
    if ( c == '\\' )
    {
      if ( s[1] == '\\' ) { *d++ = c; s++; }
      else if ( s[1] == 'n' ) { *d++ = '\n'; s++; }
      else if ( s[1] == 'r' ) { *d++ = '\r'; s++; }
      else if ( isdigit( s[1] ) ) { *d++ = ((s[1]-'0')<<6) + ((s[2]-'0')<<3) + (s[3]-'0'); s+=3; }
      else s++; // error
    }
    else
      *d++ = c;
  }
  *d = 0;
  return dst;
}


/**
 Reads an entry from the group. A default value must be
 supplied. The return value indicates if the value was available
 (non-zero) or the default was used (0). 
 'maxSize' is the maximum length of text that will be read. 
 The text buffer must allow for one additional byte for a trailling zero.
 
 \param[in] key name of entry
 \param[out] text returned from preferences or default value if none was set
 \param[in] defaultValue default value to be used if no preference was set
 \param[in] maxSize maximum length of value plus one byte for a trailing zero
 \return 0 if the default value was used 
 */
char Fl_Preferences::get( const char *key, char *text, const char *defaultValue, int maxSize )
{
  const char *v = node->get( key );
  if ( v && strchr( v, '\\' ) ) {
    char *w = decodeText( v );
    strlcpy(text, w, maxSize);
    free( w );
    return 1;
  }
  if ( !v ) v = defaultValue;
  if ( v ) strlcpy(text, v, maxSize);
  else text = 0;
  return ( v != defaultValue );
}


/**
 Reads an entry from the group. A default value must be
 supplied. The return value indicates if the value was available
 (non-zero) or the default was used (0). get() allocates memory of
 sufficient size to hold the value. The buffer must be free'd by 
 the developer using 'free(value)'. 
 
 \param[in] key name of entry
 \param[out] text returned from preferences or default value if none was set
 \param[in] defaultValue default value to be used if no preference was set
 \return 0 if the default value was used 
 */
char Fl_Preferences::get( const char *key, char *&text, const char *defaultValue )
{
  const char *v = node->get( key );
  if ( v && strchr( v, '\\' ) )
  {
    text = decodeText( v );
    return 1;
  }    
  if ( !v ) v = defaultValue;
  if ( v )
    text = strdup( v );
  else
    text = 0;
  return ( v != defaultValue );
}



/** 
 Sets an entry (name/value pair). The return value indicates if there
 was a problem storing the data in memory. However it does not
 reflect if the value was actually stored in the preferences
 file.
 
 \param[in] key name of entry
 \param[in] text set this entry to \p value
 \return 0 if setting the value failed
 */
char Fl_Preferences::set( const char *key, const char *text )
{
  const char *s = text ? text : "";
  int n=0, ns=0;
  for ( ; *s; s++ ) { n++; if ( *s<32 || *s=='\\' || *s==0x7f ) ns+=4; }
  if ( ns )
  {
    char *buffer = (char*)malloc( n+ns+1 ), *d = buffer;
    for ( s=text; *s; ) 
    { 
      char c = *s;
      if ( c=='\\' ) { *d++ = '\\'; *d++ = '\\'; s++; }
      else if ( c=='\n' ) { *d++ = '\\'; *d++ = 'n'; s++; }
      else if ( c=='\r' ) { *d++ = '\\'; *d++ = 'r'; s++; }
      else if ( c<32 || c==0x7f ) 
	{ *d++ = '\\'; *d++ = '0'+((c>>6)&3); *d++ = '0'+((c>>3)&7); *d++ = '0'+(c&7);  s++; }
      else *d++ = *s++;
    }
    *d = 0;
    node->set( key, buffer );
    free( buffer );
  }
  else
    node->set( key, text );
  return 1;
}


// convert a hex string to binary data
static void *decodeHex( const char *src, int &size )
{
  size = strlen( src )/2;
  unsigned char *data = (unsigned char*)malloc( size ), *d = data;
  const char *s = src;
  int i;

  for ( i=size; i>0; i-- )
  {
    int v;
    char x = tolower(*s++);
    if ( x >= 'a' ) v = x-'a'+10; else v = x-'0';
    v = v<<4;
    x = tolower(*s++);
    if ( x >= 'a' ) v += x-'a'+10; else v += x-'0';
    *d++ = (uchar)v;
  }

  return (void*)data;
}


/**
 Reads an entry from the group. A default value must be
 supplied. The return value indicates if the value was available
 (non-zero) or the default was used (0). 
 'maxSize' is the maximum length of text that will be read. 
 
 \param[in] key name of entry
 \param[out] data value returned from preferences or default value if none was set
 \param[in] defaultValue default value to be used if no preference was set
 \param[in] defaultSize size of default value array
 \param[in] maxSize maximum length of value
 \return 0 if the default value was used 
 
 \todo maxSize should receive the number of bytes that were read.
 */
char Fl_Preferences::get( const char *key, void *data, const void *defaultValue, int defaultSize, int maxSize )
{
  const char *v = node->get( key );
  if ( v )
  {
    int dsize;
    void *w = decodeHex( v, dsize );
    memmove( data, w, dsize>maxSize?maxSize:dsize );
    free( w );
    return 1;
  }    
  if ( defaultValue )
    memmove( data, defaultValue, defaultSize>maxSize?maxSize:defaultSize );
  return 0;
}


/**
 Reads an entry from the group. A default value must be
 supplied. The return value indicates if the value was available
 (non-zero) or the default was used (0). get() allocates memory of
 sufficient size to hold the value. The buffer must be free'd by 
 the developer using 'free(value)'. 
 
 \param[in] key name of entry
 \param[out] data returned from preferences or default value if none was set
 \param[in] defaultValue default value to be used if no preference was set
 \param[in] defaultSize size of default value array
 \return 0 if the default value was used 
 */
char Fl_Preferences::get( const char *key, void *&data, const void *defaultValue, int defaultSize )
{
  const char *v = node->get( key );
  if ( v )
  {
    int dsize;
    data = decodeHex( v, dsize );
    return 1;
  }    
  if ( defaultValue )
  {
    data = (void*)malloc( defaultSize );
    memmove( data, defaultValue, defaultSize );
  }
  else
    data = 0;
  return 0;
}


/** 
 Sets an entry (name/value pair). The return value indicates if there
 was a problem storing the data in memory. However it does not
 reflect if the value was actually stored in the preferences
 file.
 
 \param[in] key name of entry
 \param[in] data set this entry to \p value
 \param[in] dsize size of data array
 \return 0 if setting the value failed
 */
char Fl_Preferences::set( const char *key, const void *data, int dsize )
{
  char *buffer = (char*)malloc( dsize*2+1 ), *d = buffer;;
  unsigned char *s = (unsigned char*)data;
  for ( ; dsize>0; dsize-- )
  {
    static char lu[] = "0123456789abcdef";
    unsigned char v = *s++;
    *d++ = lu[v>>4];
    *d++ = lu[v&0xf];
  }
  *d = 0;
  node->set( key, buffer );
  free( buffer );
  return 1;
}


/**
 Returns the size of the value part of an entry.
 
 \param[in] key name of entry
 \return size of value
 */
int Fl_Preferences::size( const char *key )
{
  const char *v = node->get( key );
  return v ? strlen( v ) : 0 ;
}


/**
 \brief Creates a path that is related to the preferences file and
 that is usable for additional application data.
 
 This function creates a directory that is named after the preferences
 database without the \c .prefs extension and located in the same directory.
 It then fills the given buffer with the complete path name.
 
 Exmaple:
 \code
 Fl_Preferences prefs( USER, "matthiasm.com", "test" );
 char path[FL_PATH_MAX];
 prefs.getUserdataPath( path );
 \endcode
 creates the preferences database in (MS Windows):
 \code
 c:/Documents and Settings/matt/Application Data/matthiasm.com/test.prefs
 \endcode
 and returns the userdata path:
 \code
 c:/Documents and Settings/matt/Application Data/matthiasm.com/test/
 \endcode
 
 \param[out] path buffer for user data path
 \param[in] pathlen size of path buffer (should be at least \c FL_PATH_MAX)
 \return 0 if path was not created or pathname can't fit into buffer
 */
char Fl_Preferences::getUserdataPath( char *path, int pathlen )
{
  if ( rootNode )
    return rootNode->getPath( path, pathlen );
  return 0;
}

/**
 Writes all preferences to disk. This function works only with
 the base preferences group. This function is rarely used as
 deleting the base preferences flushes automatically.
 */
void Fl_Preferences::flush()
{
  if ( rootNode && node->dirty() )
    rootNode->write();
}

//-----------------------------------------------------------------------------
// helper class to create dynamic group and entry names on the fly
//

/**
   Creates a group name or entry name on the fly.
   
   This version creates a simple unsigned integer as an entry name.
  
   \code
     int n, i;
     Fl_Preferences prev( appPrefs, "PreviousFiles" );
     prev.get( "n", 0 );
     for ( i=0; i<n; i++ )
       prev.get( Fl_Preferences::Name(i), prevFile[i], "" );
   \endcode
 */
Fl_Preferences::Name::Name( unsigned int n )
{
  data_ = (char*)malloc(20);
  sprintf(data_, "%u", n);
}

/**
   Creates a group name or entry name on the fly.
  
   This version creates entry names as in 'printf'.
   
   \code
     int n, i;
     Fl_Preferences prefs( USER, "matthiasm.com", "test" );
     prev.get( "nFiles", 0 );
     for ( i=0; i<n; i++ )
       prev.get( Fl_Preferences::Name( "File%d", i ), prevFile[i], "" );
    \endcode
 */
Fl_Preferences::Name::Name( const char *format, ... )
{
  data_ = (char*)malloc(1024);
  va_list args;
  va_start(args, format);
  vsnprintf(data_, 1024, format, args);
  va_end(args);
}

// delete the name
Fl_Preferences::Name::~Name()
{
  if (data_) {
    free(data_);
    data_ = 0L;
  }
}

//-----------------------------------------------------------------------------
// internal methods, do not modify or use as they will change without notice
//

int Fl_Preferences::Node::lastEntrySet = -1;

// recursively create a path in the file system
static char makePath( const char *path ) {
  if (access(path, 0)) {
    const char *s = strrchr( path, '/' );
    if ( !s ) return 0;
    int len = s-path;
    char *p = (char*)malloc( len+1 );
    memcpy( p, path, len );
    p[len] = 0;
    makePath( p );
    free( p );
#if defined(WIN32) && !defined(__CYGWIN__)
    return ( mkdir( path ) == 0 );
#else
    return ( mkdir( path, 0777 ) == 0 );
#endif // WIN32 && !__CYGWIN__
  }
  return 1;
}

#if 0
// strip the filename and create a path
static void makePathForFile( const char *path )
{
  const char *s = strrchr( path, '/' );
  if ( !s ) return;
  int len = s-path;
  char *p = (char*)malloc( len+1 );
  memcpy( p, path, len );
  p[len] = 0;
  makePath( p );
  free( p );
}
#endif

// create the root node
// - construct the name of the file that will hold our preferences
Fl_Preferences::RootNode::RootNode( Fl_Preferences *prefs, Root root, const char *vendor, const char *application )
{
  char filename[ FL_PATH_MAX ]; filename[0] = 0;
#ifdef WIN32
#  define FLPREFS_RESOURCE	"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"
#  define FLPREFS_RESOURCEW	L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"
  int appDataLen = strlen(vendor) + strlen(application) + 8;
  DWORD type, nn;
  LONG err;
  HKEY key;

  switch (root) {
    case SYSTEM:

        err = RegOpenKeyW( HKEY_LOCAL_MACHINE, FLPREFS_RESOURCEW, &key );

      if (err == ERROR_SUCCESS) {
	nn = FL_PATH_MAX - appDataLen;

		err = RegQueryValueExW( key, L"Common AppData", 0L, &type,
					 (BYTE*)filename, &nn );

		if ( ( err != ERROR_SUCCESS ) && ( type == REG_SZ ) ) {
	  filename[0] = 0;
			filename[1] = 0;
		}
		RegCloseKey(key);
      }
      break;
    case USER:
        err = RegOpenKeyW( HKEY_CURRENT_USER, FLPREFS_RESOURCEW, &key );


      if (err == ERROR_SUCCESS) {
	nn = FL_PATH_MAX - appDataLen;
          err = RegQueryValueExW( key, L"AppData", 0L, &type,
                                 (BYTE*)filename, &nn );

        if ( ( err != ERROR_SUCCESS ) && ( type == REG_SZ ) ) {
	    filename[0] = 0;
        filename[1] = 0;
	}
        RegCloseKey(key);
      }
      break;
  }


#ifndef __CYGWIN__
    if (!filename[1] && !filename[0]) {
    strcpy(filename, "C:\\FLTK");
    } else {
      xchar *b = (xchar*)_wcsdup((xchar*)filename);
//    filename[fl_unicode2utf(b, wcslen((xchar*)b), filename)] = 0;
      unsigned len = fl_utf8fromwc(filename, (FL_PATH_MAX-1), b, wcslen((xchar*)b));
      filename[len] = 0;
      free(b);
  }
#else
  if (!filename[0]) strcpy(filename, "C:\\FLTK");
#endif
  snprintf(filename + strlen(filename), sizeof(filename) - strlen(filename),
           "/%s/%s.prefs", vendor, application);
  for (char *s = filename; *s; s++) if (*s == '\\') *s = '/';
#elif defined ( __APPLE__ )
  FSSpec spec = { 0 };
  FSRef ref;
  OSErr err = fnfErr;
  switch (root) {
    case SYSTEM:
      err = FindFolder( kLocalDomain, kPreferencesFolderType,
			1, &spec.vRefNum, &spec.parID );
      break;
    case USER:
      err = FindFolder( kUserDomain, kPreferencesFolderType,
			1, &spec.vRefNum, &spec.parID );
      break;
  }
  FSpMakeFSRef( &spec, &ref );
  FSRefMakePath( &ref, (UInt8*)filename, FL_PATH_MAX );
  snprintf(filename + strlen(filename), sizeof(filename) - strlen(filename),
           "/%s/%s.prefs", vendor, application );
#else
  const char *e;
  switch (root) {
    case USER:
      if ((e = fl_getenv("HOME")) != NULL) {
	strlcpy(filename, e, sizeof(filename));

	if (filename[strlen(filename)-1] != '/') {
	  strlcat(filename, "/.fltk/", sizeof(filename));
	} else {
	  strlcat(filename, ".fltk/", sizeof(filename));
	}
	break;
      }

    case SYSTEM:
      strcpy(filename, "/etc/fltk/");
      break;
  }

  snprintf(filename + strlen(filename), sizeof(filename) - strlen(filename),
           "%s/%s.prefs", vendor, application);
#endif

  prefs_       = prefs;
  filename_    = strdup(filename);
  vendor_      = strdup(vendor);
  application_ = strdup(application);

  read();
}

// create the root node
// - construct the name of the file that will hold our preferences
Fl_Preferences::RootNode::RootNode( Fl_Preferences *prefs, const char *path, const char *vendor, const char *application )
{
  if (!vendor)
    vendor = "unknown";
  if (!application) {
    application = "unknown";
    filename_ = strdup(path);
  } else {
    char filename[ FL_PATH_MAX ]; filename[0] = 0;
    snprintf(filename, sizeof(filename), "%s/%s.prefs", path, application);
    filename_  = strdup(filename);
  }
  prefs_       = prefs;
  vendor_      = strdup(vendor);
  application_ = strdup(application);

  read();
}

// destroy the root node and all depending nodes
Fl_Preferences::RootNode::~RootNode()
{
  if ( prefs_->node->dirty() )
    write();
  if ( filename_ ) { 
    free( filename_ );
    filename_ = 0L;
  }
  if ( vendor_ ) {
    free( vendor_ );
    vendor_ = 0L;
  }
  if ( application_ ) {
    free( application_ );
    application_ = 0L;
  }
  delete prefs_->node;
  prefs_->node = 0L;
}

// read a preferences file and construct the group tree and with all entry leafs
int Fl_Preferences::RootNode::read()
{
  char buf[1024];
  FILE *f = fl_fopen( filename_, "rb" );
  if ( !f ) return 0;
  fgets( buf, 1024, f );
  fgets( buf, 1024, f );
  fgets( buf, 1024, f );
  Node *nd = prefs_->node;
  for (;;)
  {
    if ( !fgets( buf, 1024, f ) ) break;	// EOF or Error
    if ( buf[0]=='[' ) // read a new group
    {
      int end = strcspn( buf+1, "]\n\r" );
      buf[ end+1 ] = 0;
      nd = prefs_->node->find( buf+1 );
    }
    else if ( buf[0]=='+' ) // 
    { // value of previous name/value pair spans multiple lines
      int end = strcspn( buf+1, "\n\r" );
      if ( end != 0 ) // if entry is not empty
      {
	buf[ end+1 ] = 0;
	nd->add( buf+1 );
      }
    }
    else // read a name/value pair
    {
      int end = strcspn( buf, "\n\r" );
      if ( end != 0 ) // if entry is not empty
      {
	buf[ end ] = 0;
	nd->set( buf );
      }
    }
  }
  fclose( f );
  return 0;
}

// write the group tree and all entry leafs
int Fl_Preferences::RootNode::write()
{
  fl_make_path_for_file(filename_);
  FILE *f = fl_fopen( filename_, "wb" );
  if ( !f ) return 1;
  fprintf( f, "; FLTK preferences file format 1.0\n" );
  fprintf( f, "; vendor: %s\n", vendor_ );
  fprintf( f, "; application: %s\n", application_ );
  prefs_->node->write( f );
  fclose( f );
  return 0;
}

// get the path to the preferences directory
char Fl_Preferences::RootNode::getPath( char *path, int pathlen )
{
  strlcpy( path, filename_, pathlen);

  char *s;
  for ( s = path; *s; s++ ) if ( *s == '\\' ) *s = '/';
  s = strrchr( path, '.' );
  if ( !s ) return 0;
  *s = 0;
  char ret = fl_make_path( path );
  strcpy( s, "/" );
  return ret;
}

// create a node that represents a group
// - path must be a single word, prferable alnum(), dot and underscore only. Space is ok.
Fl_Preferences::Node::Node( const char *path )
{
  if ( path ) path_ = strdup( path ); else path_ = 0;
  child_ = 0; next_ = 0; parent_ = 0;
  entry = 0;
  nEntry = NEntry = 0;
  dirty_ = 0;
}

// delete this and all depending nodes
Fl_Preferences::Node::~Node()
{
  Node *nx;
  for ( Node *nd = child_; nd; nd = nx )
  {
    nx = nd->next_;
    delete nd;
  }
  child_ = 0L;
  if ( entry )
  {
    for ( int i = 0; i < nEntry; i++ )
    {
      if ( entry[i].name ) {
	free( entry[i].name );
	entry[i].name = 0L;
      }
      if ( entry[i].value ) {
	free( entry[i].value );
	entry[i].value = 0L;
      }
    }
    free( entry );
    entry = 0L;
    nEntry = 0;
  }
  if ( path_ ) {
    free( path_ );
    path_ = 0L;
  }
  next_ = 0L;
  parent_ = 0L;
}

// recursively check if any entry is dirty (was changed after loading a fresh prefs file)
char Fl_Preferences::Node::dirty()
{
  if ( dirty_ ) return 1;
  if ( next_ && next_->dirty() ) return 1;
  if ( child_ && child_->dirty() ) return 1;
  return 0;
}

// write this node (recursively from the last neighbor back to this)
// write all entries
// write all children
int Fl_Preferences::Node::write( FILE *f )
{
  if ( next_ ) next_->write( f );
  fprintf( f, "\n[%s]\n\n", path_ );
  for ( int i = 0; i < nEntry; i++ )
  {
    char *src = entry[i].value;
    if ( src )
    { // hack it into smaller pieces if needed
      fprintf( f, "%s:", entry[i].name );
      int cnt;
      for ( cnt = 0; cnt < 60; cnt++ )
	if ( src[cnt]==0 ) break;
      fwrite( src, cnt, 1, f );
      fprintf( f, "\n" );
      src += cnt;
      for (;*src;)
      {
	for ( cnt = 0; cnt < 80; cnt++ )
	  if ( src[cnt]==0 ) break;
        fputc( '+', f );
	fwrite( src, cnt, 1, f );
        fputc( '\n', f );
	src += cnt;
      }
    }
    else
      fprintf( f, "%s\n", entry[i].name );
  }
  if ( child_ ) child_->write( f );
  dirty_ = 0;
  return 0;
}

// set the parent node and create the full path
void Fl_Preferences::Node::setParent( Node *pn )
{
  parent_ = pn;
  next_ = pn->child_;
  pn->child_ = this;
  sprintf( nameBuffer, "%s/%s", pn->path_, path_ );
  free( path_ );
  path_ = strdup( nameBuffer );
}

// add a child to this node and set its path (try to find it first...)
Fl_Preferences::Node *Fl_Preferences::Node::addChild( const char *path )
{
  sprintf( nameBuffer, "%s/%s", path_, path );
  char *name = strdup( nameBuffer );
  Node *nd = find( name );
  free( name );
  dirty_ = 1;
  return nd;
}

// create and set, or change an entry within this node
void Fl_Preferences::Node::set( const char *name, const char *value )
{
  for ( int i=0; i<nEntry; i++ )
  {
    if ( strcmp( name, entry[i].name ) == 0 )
    {
      if ( !value ) return; // annotation
      if ( strcmp( value, entry[i].value ) != 0 )
      {
	if ( entry[i].value )
	  free( entry[i].value );
	entry[i].value = strdup( value );
	dirty_ = 1;
      }
      lastEntrySet = i;
      return;
    }
  }
  if ( NEntry==nEntry )
  {
    NEntry = NEntry ? NEntry*2 : 10;
    entry = (Entry*)realloc( entry, NEntry * sizeof(Entry) );
  }
  entry[ nEntry ].name = strdup( name );
  entry[ nEntry ].value = value?strdup( value ):0;
  lastEntrySet = nEntry;
  nEntry++;
  dirty_ = 1;
}

// create or set a value (or annotation) from a single line in the file buffer
void Fl_Preferences::Node::set( const char *line )
{
  // hmm. If we assume that we always read this file in the beginning,
  // we can handle the dirty flag 'quick and dirty'
  char dirt = dirty_;
  if ( line[0]==';' || line[0]==0 || line[0]=='#' )
  {
    set( line, 0 );
  }
  else
  {
    const char *c = strchr( line, ':' );
    if ( c )
    {
      unsigned int len = c-line+1;
      if ( len >= sizeof( nameBuffer ) )
        len = sizeof( nameBuffer );
      strlcpy( nameBuffer, line, len );
      set( nameBuffer, c+1 );
    }
    else
      set( line, "" );
  }
  dirty_ = dirt;
}

// add more data to an existing entry
void Fl_Preferences::Node::add( const char *line )
{
  if ( lastEntrySet<0 || lastEntrySet>=nEntry ) return;
  char *&dst = entry[ lastEntrySet ].value;
  int a = strlen( dst );
  int b = strlen( line );
  dst = (char*)realloc( dst, a+b+1 );
  memcpy( dst+a, line, b+1 );
  dirty_ = 1;
}

// get the value for a name, returns 0 if no such name
const char *Fl_Preferences::Node::get( const char *name )
{
  int i = getEntry( name );
  return i>=0 ? entry[i].value : 0 ;
}

// find the index of an entry, returns -1 if no such entry
int Fl_Preferences::Node::getEntry( const char *name )
{
  for ( int i=0; i<nEntry; i++ )
  {
    if ( strcmp( name, entry[i].name ) == 0 )
    {
      return i;
    }
  }
  return -1;
}

// remove one entry form this group
char Fl_Preferences::Node::deleteEntry( const char *name )
{
  int ix = getEntry( name );
  if ( ix == -1 ) return 0;
  memmove( entry+ix, entry+ix+1, (nEntry-ix-1) * sizeof(Entry) );
  nEntry--;
  dirty_ = 1;
  return 1;
}

// find a group somewhere in the tree starting here
// - this method will always return a valid node (except for memory allocation problems)
// - if the node was not found, 'find' will create the required branch
Fl_Preferences::Node *Fl_Preferences::Node::find( const char *path )
{
  int len = strlen( path_ );
  if ( strncmp( path, path_, len ) == 0 )
  {
    if ( path[ len ] == 0 ) 
      return this;
    if ( path[ len ] == '/' )
    {
      Node *nd;
      for ( nd = child_; nd; nd = nd->next_ )
      {
	Node *nn = nd->find( path );
	if ( nn ) return nn;
      }
      const char *s = path+len+1;
      const char *e = strchr( s, '/' );
      if (e) strlcpy( nameBuffer, s, e-s+1 );
      else strlcpy( nameBuffer, s, sizeof(nameBuffer));
      nd = new Node( nameBuffer );
      nd->setParent( this );
      return nd->find( path );
    }
  }
  return 0;
}

// find a group somewhere in the tree starting here
// caller must not set 'offset' argument
// - if the node does not exist, 'search' returns NULL
// - if the pathname is "." (current node) return this node
// - if the pathname is "./" (root node) return the topmost node
// - if the pathname starts with "./", start the search at the root node instead
Fl_Preferences::Node *Fl_Preferences::Node::search( const char *path, int offset )
{

  if ( offset == 0 )
  {
    if ( path[0] == '.' )
    {
      if ( path[1] == 0 )
      {
	return this; // user was searching for current node
      }
      else if ( path[1] == '/' )
      {
	Node *nn = this;
	while ( nn->parent_ ) nn = nn->parent_;
	if ( path[2]==0 )
	{ // user is searching for root ( "./" )
	  return nn;
	}
	return nn->search( path+2, 2 ); // do a relative search on the root node
      }
    }
    offset = strlen( path_ ) + 1;
  }
  
  int len = strlen( path_ );
  if ( len < offset-1 ) return 0;
  len -= offset;
  if ( ( len <= 0 ) || ( strncmp( path, path_+offset, len ) == 0 ) )
  {
    if ( len > 0 && path[ len ] == 0 )
      return this;
    if ( len <= 0 || path[ len ] == '/' )
    {
      for ( Node *nd = child_; nd; nd = nd->next_ )
      {
	Node *nn = nd->search( path, offset );
	if ( nn ) return nn;
      }
      return 0;
    }
  }
  return 0;
}

// return the number of child nodes (groups)
int Fl_Preferences::Node::nChildren()
{
  int cnt = 0;
  for ( Node *nd = child_; nd; nd = nd->next_ )
    cnt++;
  return cnt;
}

// return the n'th child node
const char *Fl_Preferences::Node::child( int ix )
{
  Node *nd;
  for ( nd = child_; nd; nd = nd->next_ )
  {
    if ( !ix-- ) break;
  }
  if ( nd && nd->path_ )
  {
    char *r = strrchr( nd->path_, '/' );
    return r ? r+1 : nd->path_ ;
  }
  return 0L ;
}

// remove myself from the list and delete me (and all children)
char Fl_Preferences::Node::remove()
{
  Node *nd = 0, *np;
  if ( parent_ )
  {
    nd = parent_->child_; np = 0L;
    for ( ; nd; np = nd, nd = nd->next_ )
    {
      if ( nd == this )
      {
	if ( np ) 
	  np->next_ = nd->next_; 
	else 
	  parent_->child_ = nd->next_;
	break;
      }
    }
    parent_->dirty_ = 1;
  }
  delete this;
  return ( nd != 0 );
}


//
// End of "$Id: Fl_Preferences.cxx 6716 2009-03-24 01:40:44Z fabien $".
//
