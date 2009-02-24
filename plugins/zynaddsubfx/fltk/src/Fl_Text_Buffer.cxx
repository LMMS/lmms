//
// "$Id: Fl_Text_Buffer.cxx 6011 2008-01-04 20:32:37Z matt $"
//
// Copyright 2001-2005 by Bill Spitzak and others.
// Original code Copyright Mark Edel.  Permission to distribute under
// the LGPL for the FLTK library granted by Mark Edel.
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

#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <ctype.h>
#include <FL/Fl.H>
#include <FL/Fl_Text_Buffer.H>


#define PREFERRED_GAP_SIZE 1024
/* Initial size for the buffer gap (empty space
in the buffer where text might be inserted
if the user is typing sequential chars ) */

static void histogramCharacters( const char *string, int length, char hist[ 256 ],
                                 int init );
static void subsChars( char *string, int length, char fromChar, char toChar );
static char chooseNullSubsChar( char hist[ 256 ] );
static void insertColInLine( const char *line, char *insLine, int column, int insWidth,
                             int tabDist, int useTabs, char nullSubsChar, char *outStr, int *outLen,
                             int *endOffset );
static void deleteRectFromLine( const char *line, int rectStart, int rectEnd,
                                int tabDist, int useTabs, char nullSubsChar, char *outStr, int *outLen,
                                int *endOffset );
static void overlayRectInLine( const char *line, char *insLine, int rectStart,
                               int rectEnd, int tabDist, int useTabs, char nullSubsChar, char *outStr,
                               int *outLen, int *endOffset );

static void addPadding( char *string, int startIndent, int toIndent,
                        int tabDist, int useTabs, char nullSubsChar, int *charsAdded );
static char *copyLine( const char* text, int *lineLen );
static int countLines( const char *string );
static int textWidth( const char *text, int tabDist, char nullSubsChar );
static char *realignTabs( const char *text, int origIndent, int newIndent,
                          int tabDist, int useTabs, char nullSubsChar, int *newLength );
static char *expandTabs( const char *text, int startIndent, int tabDist,
                         char nullSubsChar, int *newLen );
static char *unexpandTabs( char *text, int startIndent, int tabDist,
                           char nullSubsChar, int *newLen );
static int max( int i1, int i2 );
static int min( int i1, int i2 );

static const char *ControlCodeTable[ 32 ] = {
  "nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel",
  "bs", "ht", "nl", "vt", "np", "cr", "so", "si",
  "dle", "dc1", "dc2", "dc3", "dc4", "nak", "syn", "etb",
  "can", "em", "sub", "esc", "fs", "gs", "rs", "us"};

static char* undobuffer;
static int undobufferlength;
static Fl_Text_Buffer* undowidget;
static int undoat;	// points after insertion
static int undocut;	// number of characters deleted there
static int undoinsert;	// number of characters inserted
static int undoyankcut;	// length of valid contents of buffer, even if undocut=0

static void undobuffersize(int n) {
  if (n > undobufferlength) {
    if (undobuffer) {
      do {undobufferlength *= 2;} while (undobufferlength < n);
      undobuffer = (char*)realloc(undobuffer, undobufferlength);
    } else {
      undobufferlength = n+9;
      undobuffer = (char*)malloc(undobufferlength);
    }
  }
}

/*
** Create an empty text buffer of a pre-determined size (use this to
** avoid unnecessary re-allocation if you know exactly how much the buffer
** will need to hold
*/
Fl_Text_Buffer::Fl_Text_Buffer( int requestedSize ) {
  mLength = 0;
  mBuf = (char *)malloc( requestedSize + PREFERRED_GAP_SIZE );
  mGapStart = 0;
  mGapEnd = PREFERRED_GAP_SIZE;
  mTabDist = 8;
  mUseTabs = 1;
  mPrimary.mSelected = 0;
  mPrimary.mRectangular = 0;
  mPrimary.mStart = mPrimary.mEnd = 0;
  mSecondary.mSelected = 0;
  mSecondary.mStart = mSecondary.mEnd = 0;
  mSecondary.mRectangular = 0;
  mHighlight.mSelected = 0;
  mHighlight.mStart = mHighlight.mEnd = 0;
  mHighlight.mRectangular = 0;
  mNodifyProcs = NULL;
  mCbArgs = NULL;
  mNModifyProcs = 0;
  mNPredeleteProcs = 0;
  mPredeleteProcs = NULL;
  mPredeleteCbArgs = NULL;
  mCursorPosHint = 0;
  mNullSubsChar = '\0';
  mCanUndo = 1;
#ifdef PURIFY
{ int i; for (i = mGapStart; i < mGapEnd; i++) mBuf[ i ] = '.'; }
#endif
}

/*
** Free a text buffer
*/
Fl_Text_Buffer::~Fl_Text_Buffer() {
  free( mBuf );
  if ( mNModifyProcs != 0 ) {
    delete[] mNodifyProcs;
    delete[] mCbArgs;
  }
  if ( mNPredeleteProcs != 0 ) {
    delete[] mPredeleteProcs;
    delete[] mPredeleteCbArgs;
  }
}

/*
** Get the entire contents of a text buffer.  Memory is allocated to contain
** the returned string, which the caller must free.
*/
char * Fl_Text_Buffer::text() {
  char *t;

  t = (char *)malloc( mLength + 1 );
  memcpy( t, mBuf, mGapStart );
  memcpy( &t[ mGapStart ], &mBuf[ mGapEnd ],
          mLength - mGapStart );
  t[ mLength ] = '\0';
  return t;
}

/*
** Replace the entire contents of the text buffer
*/
void Fl_Text_Buffer::text( const char *t ) {
  int insertedLength, deletedLength;
  const char *deletedText;

  call_predelete_callbacks(0, length());

  /* Save information for redisplay, and get rid of the old buffer */
  deletedText = text();
  deletedLength = mLength;
  free( (void *)mBuf );

  /* Start a new buffer with a gap of PREFERRED_GAP_SIZE in the center */
  insertedLength = strlen( t );
  mBuf = (char *)malloc( insertedLength + PREFERRED_GAP_SIZE );
  mLength = insertedLength;
  mGapStart = insertedLength / 2;
  mGapEnd = mGapStart + PREFERRED_GAP_SIZE;
  memcpy( mBuf, t, mGapStart );
  memcpy( &mBuf[ mGapEnd ], &t[ mGapStart ], insertedLength - mGapStart );
#ifdef PURIFY
{ int i; for ( i = mGapStart; i < mGapEnd; i++ ) mBuf[ i ] = '.'; }
#endif

  /* Zero all of the existing selections */
  update_selections( 0, deletedLength, 0 );

  /* Call the saved display routine(s) to update the screen */
  call_modify_callbacks( 0, deletedLength, insertedLength, 0, deletedText );
  free( (void *)deletedText );
}

/*
** Return a copy of the text between "start" and "end" character positions
** from text buffer "buf".  Positions start at 0, and the range does not
** include the character pointed to by "end"
*/
char * Fl_Text_Buffer::text_range( int start, int end ) {
  char * s;
  int copiedLength, part1Length;

  /* Make sure start and end are ok, and allocate memory for returned string.
     If start is bad, return "", if end is bad, adjust it. */
  if ( start < 0 || start > mLength ) {
    s = (char *)malloc( 1 );
    s[ 0 ] = '\0';
    return s;
  }
  if ( end < start ) {
    int temp = start;
    start = end;
    end = temp;
  }
  if ( end > mLength )
    end = mLength;
  copiedLength = end - start;
  s = (char *)malloc( copiedLength + 1 );

  /* Copy the text from the buffer to the returned string */
  if ( end <= mGapStart ) {
    memcpy( s, &mBuf[ start ], copiedLength );
  } else if ( start >= mGapStart ) {
    memcpy( s, &mBuf[ start + ( mGapEnd - mGapStart ) ], copiedLength );
  } else {
    part1Length = mGapStart - start;
    memcpy( s, &mBuf[ start ], part1Length );
    memcpy( &s[ part1Length ], &mBuf[ mGapEnd ], copiedLength - part1Length );
  }
  s[ copiedLength ] = '\0';
  return s;
}

/*
** Return the character at buffer position "pos".  Positions start at 0.
*/
char Fl_Text_Buffer::character( int pos ) {
  if ( pos < 0 || pos >= mLength )
    return '\0';
  if ( pos < mGapStart )
    return mBuf[ pos ];
  else
    return mBuf[ pos + mGapEnd - mGapStart ];
}

/*
** Insert null-terminated string "text" at position "pos" in "buf"
*/
void Fl_Text_Buffer::insert( int pos, const char *s ) {
  int nInserted;

  /* if pos is not contiguous to existing text, make it */
  if ( pos > mLength ) pos = mLength;
  if ( pos < 0 ) pos = 0;

  /* Even if nothing is deleted, we must call these callbacks */
  call_predelete_callbacks( pos, 0 );

  /* insert and redisplay */
  nInserted = insert_( pos, s );
  mCursorPosHint = pos + nInserted;
  call_modify_callbacks( pos, 0, nInserted, 0, NULL );
}

/*
** Delete the characters between "start" and "end", and insert the
** null-terminated string "text" in their place in in "buf"
*/
void Fl_Text_Buffer::replace( int start, int end, const char *s ) {
  const char * deletedText;
  int nInserted;

  // Range check...
  if (!s) return;
  if (start < 0) start = 0;
  if (end > mLength) end = mLength;

  call_predelete_callbacks( start, end-start );
  deletedText = text_range( start, end );
  remove_( start, end );
  //undoyankcut = undocut;
  nInserted = insert_( start, s );
  mCursorPosHint = start + nInserted;
  call_modify_callbacks( start, end - start, nInserted, 0, deletedText );
  free( (void *)deletedText );
}

void Fl_Text_Buffer::remove( int start, int end ) {
  const char * deletedText;

  /* Make sure the arguments make sense */
  if ( start > end ) {
    int temp = start;
    start = end;
    end = temp;
  }
  if ( start > mLength ) start = mLength;
  if ( start < 0 ) start = 0;
  if ( end > mLength ) end = mLength;
  if ( end < 0 ) end = 0;

  if (start == end) return;

  call_predelete_callbacks( start, end-start );
  /* Remove and redisplay */
  deletedText = text_range( start, end );
  remove_( start, end );
  mCursorPosHint = start;
  call_modify_callbacks( start, end - start, 0, 0, deletedText );
  free( (void *)deletedText );
}

void Fl_Text_Buffer::copy( Fl_Text_Buffer *fromBuf, int fromStart,
                           int fromEnd, int toPos ) {
  int copiedLength = fromEnd - fromStart;
  int part1Length;

  /* Prepare the buffer to receive the new text.  If the new text fits in
     the current buffer, just move the gap (if necessary) to where
     the text should be inserted.  If the new text is too large, reallocate
     the buffer with a gap large enough to accomodate the new text and a
     gap of PREFERRED_GAP_SIZE */
  if ( copiedLength > mGapEnd - mGapStart )
    reallocate_with_gap( toPos, copiedLength + PREFERRED_GAP_SIZE );
  else if ( toPos != mGapStart )
    move_gap( toPos );

  /* Insert the new text (toPos now corresponds to the start of the gap) */
  if ( fromEnd <= fromBuf->mGapStart ) {
    memcpy( &mBuf[ toPos ], &fromBuf->mBuf[ fromStart ], copiedLength );
  } else if ( fromStart >= fromBuf->mGapStart ) {
    memcpy( &mBuf[ toPos ],
            &fromBuf->mBuf[ fromStart + ( fromBuf->mGapEnd - fromBuf->mGapStart ) ],
            copiedLength );
  } else {
    part1Length = fromBuf->mGapStart - fromStart;
    memcpy( &mBuf[ toPos ], &fromBuf->mBuf[ fromStart ], part1Length );
    memcpy( &mBuf[ toPos + part1Length ], &fromBuf->mBuf[ fromBuf->mGapEnd ],
            copiedLength - part1Length );
  }
  mGapStart += copiedLength;
  mLength += copiedLength;
  update_selections( toPos, 0, copiedLength );
}

/*
** remove text according to the undo variables or insert text 
** from the undo buffer
*/
int Fl_Text_Buffer::undo(int *cursorPos) {
  if (undowidget != this || !undocut && !undoinsert &&!mCanUndo) return 0;

  int ilen = undocut;
  int xlen = undoinsert;
  int b = undoat-xlen;

  if (xlen && undoyankcut && !ilen) {
    ilen = undoyankcut;
  }

  if (xlen && ilen) {
    undobuffersize(ilen+1);
    undobuffer[ilen] = 0;
    char *tmp = strdup(undobuffer);
    replace(b, undoat, tmp);
    if (cursorPos) *cursorPos = mCursorPosHint;
    free(tmp);
  }
  else if (xlen) {
    remove(b, undoat);
    if (cursorPos) *cursorPos = mCursorPosHint;
  }
  else if (ilen) {
    undobuffersize(ilen+1);
    undobuffer[ilen] = 0;
    insert(undoat, undobuffer);
    if (cursorPos) *cursorPos = mCursorPosHint;
    undoyankcut = 0;
  }

  return 1;
}

/*
** let the undo system know if we can undo changes
*/
void Fl_Text_Buffer::canUndo(char flag) {
  mCanUndo = flag;
}

/*
** Insert "text" columnwise into buffer starting at displayed character
** position "column" on the line beginning at "startPos".  Opens a rectangular
** space the width and height of "text", by moving all text to the right of
** "column" right.  If charsInserted and charsDeleted are not NULL, the
** number of characters inserted and deleted in the operation (beginning
** at startPos) are returned in these arguments
*/
void Fl_Text_Buffer::insert_column( int column, int startPos, const char *s,
                                    int *charsInserted, int *charsDeleted ) {
  int nLines, lineStartPos, nDeleted, insertDeleted, nInserted;
  const char *deletedText;

  nLines = countLines( s );
  lineStartPos = line_start( startPos );
  nDeleted = line_end( skip_lines( startPos, nLines ) ) -
             lineStartPos;
  call_predelete_callbacks( lineStartPos, nDeleted );
  deletedText = text_range( lineStartPos, lineStartPos + nDeleted );
  insert_column_( column, lineStartPos, s, &insertDeleted, &nInserted,
                  &mCursorPosHint );
  if ( nDeleted != insertDeleted )
    Fl::error("Fl_Text_Buffer::insert_column(): internal consistency check ins1 failed");
  call_modify_callbacks( lineStartPos, nDeleted, nInserted, 0, deletedText );
  free( (void *) deletedText );
  if ( charsInserted != NULL )
    * charsInserted = nInserted;
  if ( charsDeleted != NULL )
    * charsDeleted = nDeleted;
}

/*
** Overlay "text" between displayed character positions "rectStart" and
** "rectEnd" on the line beginning at "startPos".  If charsInserted and
** charsDeleted are not NULL, the number of characters inserted and deleted
** in the operation (beginning at startPos) are returned in these arguments.
*/
void Fl_Text_Buffer::overlay_rectangular( int startPos, int rectStart,
    int rectEnd, const char *s, int *charsInserted, int *charsDeleted ) {
  int nLines, lineStartPos, nDeleted, insertDeleted, nInserted;
  const char *deletedText;

  nLines = countLines( s );
  lineStartPos = line_start( startPos );
  nDeleted = line_end( skip_lines( startPos, nLines ) ) -
             lineStartPos;
  call_predelete_callbacks( lineStartPos, nDeleted );
  deletedText = text_range( lineStartPos, lineStartPos + nDeleted );
  overlay_rectangular_( lineStartPos, rectStart, rectEnd, s, &insertDeleted,
                        &nInserted, &mCursorPosHint );
  if ( nDeleted != insertDeleted )
    Fl::error("Fl_Text_Buffer::overlay_rectangle(): internal consistency check ovly1 failed");
  call_modify_callbacks( lineStartPos, nDeleted, nInserted, 0, deletedText );
  free( (void *) deletedText );
  if ( charsInserted != NULL )
    * charsInserted = nInserted;
  if ( charsDeleted != NULL )
    * charsDeleted = nDeleted;
}

/*
** Replace a rectangular area in buf, given by "start", "end", "rectStart",
** and "rectEnd", with "text".  If "text" is vertically longer than the
** rectangle, add extra lines to make room for it.
*/
void Fl_Text_Buffer::replace_rectangular( int start, int end, int rectStart,
    int rectEnd, const char *s ) {
  char *insPtr;
  const char *deletedText;
  char *insText = (char *)"";
  int i, nInsertedLines, nDeletedLines, insLen, hint;
  int insertDeleted, insertInserted, deleteInserted;
  int linesPadded = 0;

  /* Make sure start and end refer to complete lines, since the
     columnar delete and insert operations will replace whole lines */
  start = line_start( start );
  end = line_end( end );

  call_predelete_callbacks( start, end-start );

  /* If more lines will be deleted than inserted, pad the inserted text
     with newlines to make it as long as the number of deleted lines.  This
     will indent all of the text to the right of the rectangle to the same
     column.  If more lines will be inserted than deleted, insert extra
     lines in the buffer at the end of the rectangle to make room for the
     additional lines in "text" */
  nInsertedLines = countLines( s );
  nDeletedLines = count_lines( start, end );
  if ( nInsertedLines < nDeletedLines ) {
    insLen = strlen( s );
    insText = (char *)malloc( insLen + nDeletedLines - nInsertedLines + 1 );
    strcpy( insText, s );
    insPtr = insText + insLen;
    for ( i = 0; i < nDeletedLines - nInsertedLines; i++ )
      *insPtr++ = '\n';
    *insPtr = '\0';
  } else if ( nDeletedLines < nInsertedLines ) {
    linesPadded = nInsertedLines - nDeletedLines;
    for ( i = 0; i < linesPadded; i++ )
      insert_( end, "\n" );
  } /* else nDeletedLines == nInsertedLines; */

  /* Save a copy of the text which will be modified for the modify CBs */
  deletedText = text_range( start, end );

  /* Delete then insert */
  remove_rectangular_( start, end, rectStart, rectEnd, &deleteInserted, &hint );
  insert_column_( rectStart, start, insText, &insertDeleted, &insertInserted,
                  &mCursorPosHint );

  /* Figure out how many chars were inserted and call modify callbacks */
  if ( insertDeleted != deleteInserted + linesPadded )
    Fl::error("Fl_Text_Buffer::replace_rectangular(): internal consistency check repl1 failed");
  call_modify_callbacks( start, end - start, insertInserted, 0, deletedText );
  free( (void *) deletedText );
  if ( nInsertedLines < nDeletedLines )
    free( (void *) insText );
}

/*
** Remove a rectangular swath of characters between character positions start
** and end and horizontal displayed-character offsets rectStart and rectEnd.
*/
void Fl_Text_Buffer::remove_rectangular( int start, int end, int rectStart,
    int rectEnd ) {
  const char * deletedText;
  int nInserted;

  start = line_start( start );
  end = line_end( end );
  call_predelete_callbacks( start, end-start );
  deletedText = text_range( start, end );
  remove_rectangular_( start, end, rectStart, rectEnd, &nInserted,
                       &mCursorPosHint );
  call_modify_callbacks( start, end - start, nInserted, 0, deletedText );
  free( (void *) deletedText );
}

/*
** Clear a rectangular "hole" out of the buffer between character positions
** start and end and horizontal displayed-character offsets rectStart and
** rectEnd.
*/
void Fl_Text_Buffer::clear_rectangular( int start, int end, int rectStart,
                                        int rectEnd ) {
  int i, nLines;
  char *newlineString;

  nLines = count_lines( start, end );
  newlineString = (char *)malloc( nLines + 1 );
  for ( i = 0; i < nLines; i++ )
    newlineString[ i ] = '\n';
  newlineString[ i ] = '\0';
  overlay_rectangular( start, rectStart, rectEnd, newlineString,
                       NULL, NULL );
  free( (void *) newlineString );
}

char * Fl_Text_Buffer::text_in_rectangle( int start, int end,
    int rectStart, int rectEnd ) {
  int lineStart, selLeft, selRight, len;
  char *textOut, *outPtr, *retabbedStr;
  const char *textIn;

  start = line_start( start );
  end = line_end( end );
  textOut = (char *)malloc( ( end - start ) + 1 );
  lineStart = start;
  outPtr = textOut;
  while ( lineStart <= end ) {
    rectangular_selection_boundaries( lineStart, rectStart, rectEnd,
                                      &selLeft, &selRight );
    textIn = text_range( selLeft, selRight );
    len = selRight - selLeft;
    memcpy( outPtr, textIn, len );
    free( (void *) textIn );
    outPtr += len;
    lineStart = line_end( selRight ) + 1;
    *outPtr++ = '\n';
  }
  if ( outPtr != textOut )
    outPtr--;    /* don't leave trailing newline */
  *outPtr = '\0';

  /* If necessary, realign the tabs in the selection as if the text were
     positioned at the left margin */
  retabbedStr = realignTabs( textOut, rectStart, 0, mTabDist,
                             mUseTabs, mNullSubsChar, &len );
  free( (void *) textOut );
  return retabbedStr;
}

/*
** Set the hardware tab distance used by all displays for this buffer,
** and used in computing offsets for rectangular selection operations.
*/
void Fl_Text_Buffer::tab_distance( int tabDist ) {
  const char * deletedText;

    /* First call the pre-delete callbacks with the previous tab setting 
       still active. */
  call_predelete_callbacks( 0, mLength );
    
  /* Change the tab setting */
  mTabDist = tabDist;

  /* Force any display routines to redisplay everything (unfortunately,
     this means copying the whole buffer contents to provide "deletedText" */
  deletedText = text();
  call_modify_callbacks( 0, mLength, mLength, 0, deletedText );
  free( (void *) deletedText );
}

void Fl_Text_Buffer::select( int start, int end ) {
  Fl_Text_Selection oldSelection = mPrimary;

  mPrimary.set( start, end );
  redisplay_selection( &oldSelection, &mPrimary );
}

void Fl_Text_Buffer::unselect() {
  Fl_Text_Selection oldSelection = mPrimary;

  mPrimary.mSelected = 0;
  redisplay_selection( &oldSelection, &mPrimary );
}

void Fl_Text_Buffer::select_rectangular( int start, int end, int rectStart,
    int rectEnd ) {
  Fl_Text_Selection oldSelection = mPrimary;

  mPrimary.set_rectangular( start, end, rectStart, rectEnd );
  redisplay_selection( &oldSelection, &mPrimary );
}

int Fl_Text_Buffer::selection_position( int *start, int *end
                                      ) {
  return mPrimary.position( start, end );
}

int Fl_Text_Buffer::selection_position( int *start, int *end,
                                        int *isRect, int *rectStart, int *rectEnd ) {
  return mPrimary.position( start, end, isRect, rectStart,
                            rectEnd );
}

char * Fl_Text_Buffer::selection_text() {
  return selection_text_( &mPrimary );
}

void Fl_Text_Buffer::remove_selection() {
  remove_selection_( &mPrimary );
}

void Fl_Text_Buffer::replace_selection( const char *s ) {
  replace_selection_( &mPrimary, s );
}

void Fl_Text_Buffer::secondary_select( int start, int end ) {
  Fl_Text_Selection oldSelection = mSecondary;

  mSecondary.set( start, end );
  redisplay_selection( &oldSelection, &mSecondary );
}

void Fl_Text_Buffer::secondary_unselect() {
  Fl_Text_Selection oldSelection = mSecondary;

  mSecondary.mSelected = 0;
  redisplay_selection( &oldSelection, &mSecondary );
}

void Fl_Text_Buffer::secondary_select_rectangular( int start, int end,
    int rectStart, int rectEnd ) {
  Fl_Text_Selection oldSelection = mSecondary;

  mSecondary.set_rectangular( start, end, rectStart, rectEnd );
  redisplay_selection( &oldSelection, &mSecondary );
}

int Fl_Text_Buffer::secondary_selection_position( int *start, int *end
                                      ) {
  return mSecondary.position( start, end );
}

int Fl_Text_Buffer::secondary_selection_position( int *start, int *end,
    int *isRect, int *rectStart, int *rectEnd ) {
  return mSecondary.position( start, end, isRect, rectStart,
                              rectEnd );
}

char * Fl_Text_Buffer::secondary_selection_text() {
  return selection_text_( &mSecondary );
}

void Fl_Text_Buffer::remove_secondary_selection() {
  remove_selection_( &mSecondary );
}

void Fl_Text_Buffer::replace_secondary_selection( const char *s ) {
  replace_selection_( &mSecondary, s );
}

void Fl_Text_Buffer::highlight( int start, int end ) {
  Fl_Text_Selection oldSelection = mHighlight;

  mHighlight.set( start, end );
  redisplay_selection( &oldSelection, &mHighlight );
}

void Fl_Text_Buffer::unhighlight() {
  Fl_Text_Selection oldSelection = mHighlight;

  mHighlight.mSelected = 0;
  redisplay_selection( &oldSelection, &mHighlight );
}

void Fl_Text_Buffer::highlight_rectangular( int start, int end,
    int rectStart, int rectEnd ) {
  Fl_Text_Selection oldSelection = mHighlight;

  mHighlight.set_rectangular( start, end, rectStart, rectEnd );
  redisplay_selection( &oldSelection, &mHighlight );
}

int Fl_Text_Buffer::highlight_position( int *start, int *end
                                      ) {
  return mHighlight.position( start, end );
}

int Fl_Text_Buffer::highlight_position( int *start, int *end,
                                        int *isRect, int *rectStart, int *rectEnd ) {
  return mHighlight.position( start, end, isRect, rectStart,
                              rectEnd );
}

char * Fl_Text_Buffer::highlight_text() {
  return selection_text_( &mHighlight );
}

/*
** Add a callback routine to be called when the buffer is modified
*/
void Fl_Text_Buffer::add_modify_callback( Fl_Text_Modify_Cb bufModifiedCB,
    void *cbArg ) {
  Fl_Text_Modify_Cb * newModifyProcs;
  void **newCBArgs;
  int i;

  newModifyProcs = new Fl_Text_Modify_Cb [ mNModifyProcs + 1 ];
  newCBArgs = new void * [ mNModifyProcs + 1 ];
  for ( i = 0; i < mNModifyProcs; i++ ) {
    newModifyProcs[ i + 1 ] = mNodifyProcs[ i ];
    newCBArgs[ i + 1 ] = mCbArgs[ i ];
  }
  if ( mNModifyProcs != 0 ) {
    delete [] mNodifyProcs;
    delete [] mCbArgs;
  }
  newModifyProcs[ 0 ] = bufModifiedCB;
  newCBArgs[ 0 ] = cbArg;
  mNModifyProcs++;
  mNodifyProcs = newModifyProcs;
  mCbArgs = newCBArgs;
}

void Fl_Text_Buffer::remove_modify_callback( Fl_Text_Modify_Cb bufModifiedCB,
    void *cbArg ) {
  int i, toRemove = -1;
  Fl_Text_Modify_Cb *newModifyProcs;
  void **newCBArgs;

  /* find the matching callback to remove */
  for ( i = 0; i < mNModifyProcs; i++ ) {
    if ( mNodifyProcs[ i ] == bufModifiedCB && mCbArgs[ i ] == cbArg ) {
      toRemove = i;
      break;
    }
  }
  if ( toRemove == -1 ) {
    Fl::error("Fl_Text_Buffer::remove_modify_callback(): Can't find modify CB to remove");
    return;
  }

  /* Allocate new lists for remaining callback procs and args (if
     any are left) */
  mNModifyProcs--;
  if ( mNModifyProcs == 0 ) {
    mNModifyProcs = 0;
    delete[] mNodifyProcs;
    mNodifyProcs = NULL;
    delete[] mCbArgs;
    mCbArgs = NULL;
    return;
  }
  newModifyProcs = new Fl_Text_Modify_Cb [ mNModifyProcs ];
  newCBArgs = new void * [ mNModifyProcs ];

  /* copy out the remaining members and free the old lists */
  for ( i = 0; i < toRemove; i++ ) {
    newModifyProcs[ i ] = mNodifyProcs[ i ];
    newCBArgs[ i ] = mCbArgs[ i ];
  }
  for ( ; i < mNModifyProcs; i++ ) {
    newModifyProcs[ i ] = mNodifyProcs[ i + 1 ];
    newCBArgs[ i ] = mCbArgs[ i + 1 ];
  }
  delete[] mNodifyProcs;
  delete[] mCbArgs;
  mNodifyProcs = newModifyProcs;
  mCbArgs = newCBArgs;
}

/*
** Add a callback routine to be called before text is deleted from the buffer.
*/
void Fl_Text_Buffer::add_predelete_callback(Fl_Text_Predelete_Cb bufPreDeleteCB,
	void *cbArg) {
    Fl_Text_Predelete_Cb *newPreDeleteProcs;
    void **newCBArgs;
    int i;
    
    newPreDeleteProcs = new Fl_Text_Predelete_Cb[ mNPredeleteProcs + 1 ];
    newCBArgs = new void * [ mNPredeleteProcs + 1 ];
    for ( i = 0; i < mNPredeleteProcs; i++ ) {
    	newPreDeleteProcs[i + 1] = mPredeleteProcs[i];
    	newCBArgs[i + 1] = mPredeleteCbArgs[i];
    }
    if (! mNPredeleteProcs != 0) {
		 delete [] mPredeleteProcs;
		 delete [] mPredeleteCbArgs;
    }
    newPreDeleteProcs[0] =  bufPreDeleteCB;
    newCBArgs[0] = cbArg;
    mNPredeleteProcs++;
    mPredeleteProcs = newPreDeleteProcs;
    mPredeleteCbArgs = newCBArgs;
}

void Fl_Text_Buffer::remove_predelete_callback(
   Fl_Text_Predelete_Cb bufPreDeleteCB, void *cbArg) {
    int i, toRemove = -1;
    Fl_Text_Predelete_Cb *newPreDeleteProcs;
    void **newCBArgs;

    /* find the matching callback to remove */
    for ( i = 0; i < mNPredeleteProcs; i++) {
    	if (mPredeleteProcs[i] == bufPreDeleteCB && 
	       mPredeleteCbArgs[i] == cbArg) {
    	    toRemove = i;
    	    break;
    	}
    }
    if (toRemove == -1) {
    	Fl::error("Fl_Text_Buffer::remove_predelete_callback(): Can't find pre-delete CB to remove");
    	return;
    }
    
    /* Allocate new lists for remaining callback procs and args (if
       any are left) */
    mNPredeleteProcs--;
    if (mNPredeleteProcs == 0) {
    	mNPredeleteProcs = 0;
		delete[] mPredeleteProcs;
    	mPredeleteProcs = NULL;
		delete[] mPredeleteCbArgs;
	   mPredeleteCbArgs = NULL;
	   return;
    }
    newPreDeleteProcs = new Fl_Text_Predelete_Cb [ mNPredeleteProcs ];
    newCBArgs = new void * [ mNPredeleteProcs ];
    
    /* copy out the remaining members and free the old lists */
    for ( i = 0; i < toRemove; i++) {
    	newPreDeleteProcs[i] = mPredeleteProcs[i];
    	newCBArgs[i] = mPredeleteCbArgs[i];
    }
    for ( ; i < mNPredeleteProcs; i++) {
	   newPreDeleteProcs[i] = mPredeleteProcs[i+1];
    	newCBArgs[i] = mPredeleteCbArgs[i+1];
    }
    delete[] mPredeleteProcs;
    delete[] mPredeleteCbArgs;
    mPredeleteProcs = newPreDeleteProcs;
    mPredeleteCbArgs = newCBArgs;
}

/*
** Return the text from the entire line containing position "pos"
*/
char * Fl_Text_Buffer::line_text( int pos ) {
  return text_range( line_start( pos ), line_end( pos ) );
}

/*
** Find the position of the start of the line containing position "pos"
*/
int Fl_Text_Buffer::line_start( int pos ) {
  if ( !findchar_backward( pos, '\n', &pos ) )
    return 0;
  return pos + 1;
}

/*
** Find the position of the end of the line containing position "pos"
** (which is either a pointer to the newline character ending the line,
** or a pointer to one character beyond the end of the buffer)
*/
int Fl_Text_Buffer::line_end( int pos ) {
  if ( !findchar_forward( pos, '\n', &pos ) )
    pos = mLength;
  return pos;
}

int Fl_Text_Buffer::word_start( int pos ) {
  while ( pos && ( isalnum( character( pos ) ) || character( pos ) == '_' ) ) {
    pos--;
  }
  if ( !( isalnum( character( pos ) ) || character( pos ) == '_' ) ) pos++;
  return pos;
}

int Fl_Text_Buffer::word_end( int pos ) {
  while (pos < length() && (isalnum(character(pos)) || character(pos) == '_' )) {
    pos++;
  }
  return pos;
}

/*
** Get a character from the text buffer expanded into it's screen
** representation (which may be several characters for a tab or a
** control code).  Returns the number of characters written to "outStr".
** "indent" is the number of characters from the start of the line
** for figuring tabs.  Output string is guranteed to be shorter or
** equal in length to FL_TEXT_MAX_EXP_CHAR_LEN
*/
int Fl_Text_Buffer::expand_character( int pos, int indent, char *outStr ) {
  return expand_character( character( pos ), indent, outStr,
                           mTabDist, mNullSubsChar );
}

/*
** Expand a single character from the text buffer into it's screen
** representation (which may be several characters for a tab or a
** control code).  Returns the number of characters added to "outStr".
** "indent" is the number of characters from the start of the line
** for figuring tabs.  Output string is guranteed to be shorter or
** equal in length to FL_TEXT_MAX_EXP_CHAR_LEN
*/
int Fl_Text_Buffer::expand_character( char c, int indent, char *outStr, int tabDist,
                                      char nullSubsChar ) {
  int i, nSpaces;

  /* Convert tabs to spaces */
  if ( c == '\t' ) {
    nSpaces = tabDist - ( indent % tabDist );
    for ( i = 0; i < nSpaces; i++ )
      outStr[ i ] = ' ';
    return nSpaces;
  }

  /* Convert control codes to readable character sequences */
  /*... is this safe with international character sets? */
  if ( ( ( unsigned char ) c ) <= 31 ) {
    sprintf( outStr, "<%s>", ControlCodeTable[ ( unsigned char ) c ] );
    return strlen( outStr );
  } else if ( c == 127 ) {
    sprintf( outStr, "<del>" );
    return 5;
  } else if ( c == nullSubsChar ) {
    sprintf( outStr, "<nul>" );
    return 5;
  }

  /* Otherwise, just return the character */
  *outStr = c;
  return 1;
}

/*
** Return the length in displayed characters of character "c" expanded
** for display (as discussed above in BufGetExpandedChar).  If the
** buffer for which the character width is being measured is doing null
** substitution, nullSubsChar should be passed as that character (or nul
** to ignore).
*/
int Fl_Text_Buffer::character_width( char c, int indent, int tabDist, char nullSubsChar ) {
  /* Note, this code must parallel that in Fl_Text_Buffer::ExpandCharacter */
  if ( c == '\t' )
    return tabDist - ( indent % tabDist );
  else if ( ( ( unsigned char ) c ) <= 31 )
    return strlen( ControlCodeTable[ ( unsigned char ) c ] ) + 2;
  else if ( c == 127 )
    return 5;
  else if ( c == nullSubsChar )
    return 5;
  return 1;
}

/*
** Count the number of displayed characters between buffer position
** "lineStartPos" and "targetPos". (displayed characters are the characters
** shown on the screen to represent characters in the buffer, where tabs and
** control characters are expanded)
*/
int Fl_Text_Buffer::count_displayed_characters( int lineStartPos, int targetPos ) {
  int pos, charCount = 0;
  char expandedChar[ FL_TEXT_MAX_EXP_CHAR_LEN ];

  pos = lineStartPos;
  while ( pos < targetPos )
    charCount += expand_character( pos++, charCount, expandedChar );
  return charCount;
}

/*
** Count forward from buffer position "startPos" in displayed characters
** (displayed characters are the characters shown on the screen to represent
** characters in the buffer, where tabs and control characters are expanded)
*/
int Fl_Text_Buffer::skip_displayed_characters( int lineStartPos, int nChars ) {
  int pos, charCount = 0;
  char c;

  pos = lineStartPos;
  while ( charCount < nChars && pos < mLength ) {
    c = character( pos );
    if ( c == '\n' )
      return pos;
    charCount += character_width( c, charCount, mTabDist, mNullSubsChar );
    pos++;
  }
  return pos;
}

/*
** Count the number of newlines between startPos and endPos in buffer "buf".
** The character at position "endPos" is not counted.
*/
int Fl_Text_Buffer::count_lines( int startPos, int endPos ) {
  int pos, gapLen = mGapEnd - mGapStart;
  int lineCount = 0;

  pos = startPos;
  while ( pos < mGapStart ) {
    if ( pos == endPos )
      return lineCount;
    if ( mBuf[ pos++ ] == '\n' )
      lineCount++;
  }
  while ( pos < mLength ) {
    if ( pos == endPos )
      return lineCount;
    if ( mBuf[ pos++ + gapLen ] == '\n' )
      lineCount++;
  }
  return lineCount;
}

/*
** Find the first character of the line "nLines" forward from "startPos"
** in "buf" and return its position
*/
int Fl_Text_Buffer::skip_lines( int startPos, int nLines ) {
  int pos, gapLen = mGapEnd - mGapStart;
  int lineCount = 0;

  if ( nLines == 0 )
    return startPos;

  pos = startPos;
  while ( pos < mGapStart ) {
    if ( mBuf[ pos++ ] == '\n' ) {
      lineCount++;
      if ( lineCount == nLines )
        return pos;
    }
  }
  while ( pos < mLength ) {
    if ( mBuf[ pos++ + gapLen ] == '\n' ) {
      lineCount++;
      if ( lineCount >= nLines )
        return pos;
    }
  }
  return pos;
}

/*
** Find the position of the first character of the line "nLines" backwards
** from "startPos" (not counting the character pointed to by "startpos" if
** that is a newline) in "buf".  nLines == 0 means find the beginning of
** the line
*/
int Fl_Text_Buffer::rewind_lines( int startPos, int nLines ) {
  int pos, gapLen = mGapEnd - mGapStart;
  int lineCount = -1;

  pos = startPos - 1;
  if ( pos <= 0 )
    return 0;

  while ( pos >= mGapStart ) {
    if ( mBuf[ pos + gapLen ] == '\n' ) {
      if ( ++lineCount >= nLines )
        return pos + 1;
    }
    pos--;
  }
  while ( pos >= 0 ) {
    if ( mBuf[ pos ] == '\n' ) {
      if ( ++lineCount >= nLines )
        return pos + 1;
    }
    pos--;
  }
  return 0;
}

/*
** Search forwards in buffer for string "searchString", starting with the
** character "startPos", and returning the result in "foundPos"
** returns 1 if found, 0 if not.
*/
int Fl_Text_Buffer::search_forward( int startPos, const char *searchString,
                                    int *foundPos, int matchCase )
{
  if (!searchString) return 0;
  int bp;
  const char* sp;
  while (startPos < length()) {
    bp = startPos;
    sp = searchString;
    do {
      if (!*sp) { *foundPos = startPos; return 1; }
    } while ((matchCase ? character(bp++) == *sp++ :
                         toupper(character(bp++)) == toupper(*sp++))
             && bp < length());
    startPos++;
  }
  return 0;
}

/*
** Search backwards in buffer for string "searchString", starting with the
** character BEFORE "startPos", returning the result in "foundPos"
** returns 1 if found, 0 if not.
*/
int Fl_Text_Buffer::search_backward( int startPos, const char *searchString,
                                     int *foundPos, int matchCase )
{
  if (!searchString) return 0;
  int bp;
  const char* sp;
  while (startPos > 0) {
    bp = startPos-1;
    sp = searchString+strlen(searchString)-1;
    do {
      if (sp < searchString) { *foundPos = bp+1; return 1; }
    } while ((matchCase ? character(bp--) == *sp-- :
                         toupper(character(bp--)) == toupper(*sp--))
             && bp >= 0);
    startPos--;
  }
  return 0;
}

/*
** Search forwards in buffer for characters in "searchChars", starting
** with the character "startPos", and returning the result in "foundPos"
** returns 1 if found, 0 if not.
*/
int Fl_Text_Buffer::findchars_forward( int startPos, const char *searchChars,
                                    int *foundPos ) {
  int pos, gapLen = mGapEnd - mGapStart;
  const char *c;

  pos = startPos;
  while ( pos < mGapStart ) {
    for ( c = searchChars; *c != '\0'; c++ ) {
      if ( mBuf[ pos ] == *c ) {
        *foundPos = pos;
        return 1;
      }
    }
    pos++;
  }
  while ( pos < mLength ) {
    for ( c = searchChars; *c != '\0'; c++ ) {
      if ( mBuf[ pos + gapLen ] == *c ) {
        *foundPos = pos;
        return 1;
      }
    }
    pos++;
  }
  *foundPos = mLength;
  return 0;
}

/*
** Search backwards in buffer for characters in "searchChars", starting
** with the character BEFORE "startPos", returning the result in "foundPos"
** returns 1 if found, 0 if not.
*/
int Fl_Text_Buffer::findchars_backward( int startPos, const char *searchChars,
                                     int *foundPos ) {
  int pos, gapLen = mGapEnd - mGapStart;
  const char *c;

  if ( startPos == 0 ) {
    *foundPos = 0;
    return 0;
  }
  pos = startPos == 0 ? 0 : startPos - 1;
  while ( pos >= mGapStart ) {
    for ( c = searchChars; *c != '\0'; c++ ) {
      if ( mBuf[ pos + gapLen ] == *c ) {
        *foundPos = pos;
        return 1;
      }
    }
    pos--;
  }
  while ( pos >= 0 ) {
    for ( c = searchChars; *c != '\0'; c++ ) {
      if ( mBuf[ pos ] == *c ) {
        *foundPos = pos;
        return 1;
      }
    }
    pos--;
  }
  *foundPos = 0;
  return 0;
}

/*
** A horrible design flaw in NEdit (from the very start, before we knew that
** NEdit would become so popular), is that it uses C NULL terminated strings
** to hold text.  This means editing text containing NUL characters is not
** possible without special consideration.  Here is the special consideration.
** The routines below maintain a special substitution-character which stands
** in for a null, and translates strings an buffers back and forth from/to
** the substituted form, figure out what to substitute, and figure out
** when we're in over our heads and no translation is possible.
*/

/*
** The primary routine for integrating new text into a text buffer with
** substitution of another character for ascii nuls.  This substitutes null
** characters in the string in preparation for being copied or replaced
** into the buffer, and if neccessary, adjusts the buffer as well, in the
** event that the string contains the character it is currently using for
** substitution.  Returns 0, if substitution is no longer possible
** because all non-printable characters are already in use.
*/
int Fl_Text_Buffer::substitute_null_characters( char *string, int len ) {
  char histogram[ 256 ];

  /* Find out what characters the string contains */
  histogramCharacters( string, len, histogram, 1 );

  /* Does the string contain the null-substitute character?  If so, re-
     histogram the buffer text to find a character which is ok in both the
     string and the buffer, and change the buffer's null-substitution
     character.  If none can be found, give up and return 0 */
  if ( histogram[ ( unsigned char ) mNullSubsChar ] != 0 ) {
    char * bufString;
    char newSubsChar;
    bufString = (char*)text();
    histogramCharacters( bufString, mLength, histogram, 0 );
    newSubsChar = chooseNullSubsChar( histogram );
    if ( newSubsChar == '\0' )
      return 0;
    subsChars( bufString, mLength, mNullSubsChar, newSubsChar );
    remove_( 0, mLength );
    insert_( 0, bufString );
    free( (void *) bufString );
    mNullSubsChar = newSubsChar;
  }

  /* If the string contains null characters, substitute them with the
     buffer's null substitution character */
  if ( histogram[ 0 ] != 0 )
    subsChars( string, len, '\0', mNullSubsChar );
  return 1;
}

/*
** Convert strings obtained from buffers which contain null characters, which
** have been substituted for by a special substitution character, back to
** a null-containing string.  There is no time penalty for calling this
** routine if no substitution has been done.
*/
void Fl_Text_Buffer::unsubstitute_null_characters( char *string ) {
  register char * c, subsChar = mNullSubsChar;

  if ( subsChar == '\0' )
    return;
  for ( c = string; *c != '\0'; c++ )
    if ( *c == subsChar )
      * c = '\0';
}

/*
** Create a pseudo-histogram of the characters in a string (don't actually
** count, because we don't want overflow, just mark the character's presence
** with a 1).  If init is true, initialize the histogram before acumulating.
** if not, add the new data to an existing histogram.
*/
static void histogramCharacters( const char *string, int length, char hist[ 256 ],
                                 int init ) {
  int i;
  const char *c;

  if ( init )
    for ( i = 0; i < 256; i++ )
      hist[ i ] = 0;
  for ( c = string; c < &string[ length ]; c++ )
    hist[ *( ( unsigned char * ) c ) ] |= 1;
}

/*
** Substitute fromChar with toChar in string.
*/
static void subsChars( char *string, int length, char fromChar, char toChar ) {
  char * c;

  for ( c = string; c < &string[ length ]; c++ )
    if ( *c == fromChar ) * c = toChar;
}

/*
** Search through ascii control characters in histogram in order of least
** likelihood of use, find an unused character to use as a stand-in for a
** null.  If the character set is full (no available characters outside of
** the printable set, return the null character.
*/
static char chooseNullSubsChar( char hist[ 256 ] ) {
#define N_REPLACEMENTS 25
  static char replacements[ N_REPLACEMENTS ] = {1, 2, 3, 4, 5, 6, 14, 15, 16, 17, 18, 19,
      20, 21, 22, 23, 24, 25, 26, 28, 29, 30, 31, 11, 7};
  int i;
  for ( i = 0; i < N_REPLACEMENTS; i++ )
    if ( hist[ replacements[ i ] ] == 0 )
      return replacements[ i ];
  return '\0';
}

/*
** Internal (non-redisplaying) version of BufInsert.  Returns the length of
** text inserted (this is just strlen(text), however this calculation can be
** expensive and the length will be required by any caller who will continue
** on to call redisplay).  pos must be contiguous with the existing text in
** the buffer (i.e. not past the end).
*/
int Fl_Text_Buffer::insert_( int pos, const char *s ) {
  int insertedLength = strlen( s );

  /* Prepare the buffer to receive the new text.  If the new text fits in
     the current buffer, just move the gap (if necessary) to where
     the text should be inserted.  If the new text is too large, reallocate
     the buffer with a gap large enough to accomodate the new text and a
     gap of PREFERRED_GAP_SIZE */
  if ( insertedLength > mGapEnd - mGapStart )
    reallocate_with_gap( pos, insertedLength + PREFERRED_GAP_SIZE );
  else if ( pos != mGapStart )
    move_gap( pos );

  /* Insert the new text (pos now corresponds to the start of the gap) */
  memcpy( &mBuf[ pos ], s, insertedLength );
  mGapStart += insertedLength;
  mLength += insertedLength;
  update_selections( pos, 0, insertedLength );

  if (mCanUndo) {
    if ( undowidget==this && undoat==pos && undoinsert ) {
      undoinsert += insertedLength;
    }
    else {
      undoinsert = insertedLength;
      undoyankcut = (undoat==pos) ? undocut : 0 ;
    }
    undoat = pos+insertedLength;
    undocut = 0;
    undowidget = this;
  }

  return insertedLength;
}

/*
** Internal (non-redisplaying) version of BufRemove.  Removes the contents
** of the buffer between start and end (and moves the gap to the site of
** the delete).
*/
void Fl_Text_Buffer::remove_( int start, int end ) {
  /* if the gap is not contiguous to the area to remove, move it there */

  if (mCanUndo) {
    if ( undowidget==this && undoat==end && undocut ) {
      undobuffersize( undocut+end-start+1 );
      memmove( undobuffer+end-start, undobuffer, undocut );
      undocut += end-start;
    } 
    else {
      undocut = end-start;
      undobuffersize(undocut);
    }
    undoat = start;
    undoinsert = 0;
    undoyankcut = 0;
    undowidget = this;
  }

  if ( start > mGapStart ) {
    if (mCanUndo)
      memcpy( undobuffer, mBuf+(mGapEnd-mGapStart)+start, end-start );
    move_gap( start );
  }
  else if ( end < mGapStart ) {
    if (mCanUndo)
      memcpy( undobuffer, mBuf+start, end-start );
    move_gap( end );
  }
  else {
    int prelen = mGapStart - start;
    if (mCanUndo) {
      memcpy( undobuffer, mBuf+start, prelen );
      memcpy( undobuffer+prelen, mBuf+mGapEnd, end-start-prelen);
    }
  }

  /* expand the gap to encompass the deleted characters */
  mGapEnd += end - mGapStart;
  mGapStart -= mGapStart - start;

  /* update the length */
  mLength -= end - start;

  /* fix up any selections which might be affected by the change */
  update_selections( start, end - start, 0 );
}

/*
** Insert a column of text without calling the modify callbacks.  Note that
** in some pathological cases, inserting can actually decrease the size of
** the buffer because of spaces being coalesced into tabs.  "nDeleted" and
** "nInserted" return the number of characters deleted and inserted beginning
** at the start of the line containing "startPos".  "endPos" returns buffer
** position of the lower left edge of the inserted column (as a hint for
** routines which need to set a cursor position).
*/
void Fl_Text_Buffer::insert_column_( int column, int startPos, const char *insText,
                                     int *nDeleted, int *nInserted, int *endPos ) {
  int nLines, start, end, insWidth, lineStart, lineEnd;
  int expReplLen, expInsLen, len, endOffset;
  char *c, *outStr, *outPtr, *expText, *insLine;
  const char *line;
  const char  *replText;
  const char *insPtr;

  if ( column < 0 )
    column = 0;

  /* Allocate a buffer for the replacement string large enough to hold
     possibly expanded tabs in both the inserted text and the replaced
     area, as well as per line: 1) an additional 2*FL_TEXT_MAX_EXP_CHAR_LEN
     characters for padding where tabs and control characters cross the
     column of the selection, 2) up to "column" additional spaces per
     line for padding out to the position of "column", 3) padding up
     to the width of the inserted text if that must be padded to align
     the text beyond the inserted column.  (Space for additional
     newlines if the inserted text extends beyond the end of the buffer
     is counted with the length of insText) */
  start = line_start( startPos );
  nLines = countLines( insText ) + 1;
  insWidth = textWidth( insText, mTabDist, mNullSubsChar );
  end = line_end( skip_lines( start, nLines - 1 ) );
  replText = text_range( start, end );
  expText = expandTabs( replText, 0, mTabDist, mNullSubsChar,
                        &expReplLen );
  free( (void *) replText );
  free( (void *) expText );
  expText = expandTabs( insText, 0, mTabDist, mNullSubsChar,
                        &expInsLen );
  free( (void *) expText );
  outStr = (char *)malloc( expReplLen + expInsLen +
                           nLines * ( column + insWidth + FL_TEXT_MAX_EXP_CHAR_LEN ) + 1 );

  /* Loop over all lines in the buffer between start and end removing the
     text between rectStart and rectEnd and padding appropriately.  Trim
     trailing space from line (whitespace at the ends of lines otherwise
     tends to multiply, since additional padding is added to maintain it */
  outPtr = outStr;
  lineStart = start;
  insPtr = insText;
  for (;;) {
    lineEnd = line_end( lineStart );
    line = text_range( lineStart, lineEnd );
    insLine = copyLine( insPtr, &len );
    insPtr += len;
    insertColInLine( line, insLine, column, insWidth, mTabDist,
                     mUseTabs, mNullSubsChar, outPtr, &len, &endOffset );
    free( (void *) line );
    free( (void *) insLine );
    for ( c = outPtr + len - 1; c > outPtr && isspace( *c ); c-- )
      len--;
    outPtr += len;
    *outPtr++ = '\n';
    lineStart = lineEnd < mLength ? lineEnd + 1 : mLength;
    if ( *insPtr == '\0' )
      break;
    insPtr++;
  }
  if ( outPtr != outStr )
    outPtr--;   /* trim back off extra newline */
  *outPtr = '\0';

  /* replace the text between start and end with the new stuff */
  remove_( start, end );
  insert_( start, outStr );
  *nInserted = outPtr - outStr;
  *nDeleted = end - start;
  *endPos = start + ( outPtr - outStr ) - len + endOffset;
  free( (void *) outStr );
}

/*
** Delete a rectangle of text without calling the modify callbacks.  Returns
** the number of characters replacing those between start and end.  Note that
** in some pathological cases, deleting can actually increase the size of
** the buffer because of tab expansions.  "endPos" returns the buffer position
** of the point in the last line where the text was removed (as a hint for
** routines which need to position the cursor after a delete operation)
*/
void Fl_Text_Buffer::remove_rectangular_( int start, int end, int rectStart,
    int rectEnd, int *replaceLen, int *endPos ) {
  int nLines, lineStart, lineEnd, len, endOffset;
  char *outStr, *outPtr, *expText;
  const char *s, *line;

  /* allocate a buffer for the replacement string large enough to hold
     possibly expanded tabs as well as an additional  FL_TEXT_MAX_EXP_CHAR_LEN * 2
     characters per line for padding where tabs and control characters cross
     the edges of the selection */
  start = line_start( start );
  end = line_end( end );
  nLines = count_lines( start, end ) + 1;
  s = text_range( start, end );
  expText = expandTabs( s, 0, mTabDist, mNullSubsChar, &len );
  free( (void *) s );
  free( (void *) expText );
  outStr = (char *)malloc( len + nLines * FL_TEXT_MAX_EXP_CHAR_LEN * 2 + 1 );

  /* loop over all lines in the buffer between start and end removing
     the text between rectStart and rectEnd and padding appropriately */
  lineStart = start;
  outPtr = outStr;
  endOffset = 0;
  while ( lineStart <= mLength && lineStart <= end ) {
    lineEnd = line_end( lineStart );
    line = text_range( lineStart, lineEnd );
    deleteRectFromLine( line, rectStart, rectEnd, mTabDist,
                        mUseTabs, mNullSubsChar, outPtr, &len, &endOffset );
    free( (void *) line );
    outPtr += len;
    *outPtr++ = '\n';
    lineStart = lineEnd + 1;
  }
  if ( outPtr != outStr )
    outPtr--;   /* trim back off extra newline */
  *outPtr = '\0';

  /* replace the text between start and end with the newly created string */
  remove_( start, end );
  insert_( start, outStr );
  *replaceLen = outPtr - outStr;
  *endPos = start + ( outPtr - outStr ) - len + endOffset;
  free( (void *) outStr );
}

/*
** Overlay a rectangular area of text without calling the modify callbacks.
** "nDeleted" and "nInserted" return the number of characters deleted and
** inserted beginning at the start of the line containing "startPos".
** "endPos" returns buffer position of the lower left edge of the inserted
** column (as a hint for routines which need to set a cursor position).
*/
void Fl_Text_Buffer::overlay_rectangular_(int startPos, int rectStart,
    int rectEnd, const char *insText,
    int *nDeleted, int *nInserted,
    int *endPos ) {
  int nLines, start, end, lineStart, lineEnd;
  int expInsLen, len, endOffset;
  char *c, *outStr, *outPtr, *expText, *insLine;
  const char *line;
  const char *insPtr;

  /* Allocate a buffer for the replacement string large enough to hold
     possibly expanded tabs in the inserted text, as well as per line: 1)
     an additional 2*FL_TEXT_MAX_EXP_CHAR_LEN characters for padding where tabs
     and control characters cross the column of the selection, 2) up to
     "column" additional spaces per line for padding out to the position
     of "column", 3) padding up to the width of the inserted text if that
     must be padded to align the text beyond the inserted column.  (Space
     for additional newlines if the inserted text extends beyond the end
     of the buffer is counted with the length of insText) */
  start = line_start( startPos );
  nLines = countLines( insText ) + 1;
  end = line_end( skip_lines( start, nLines - 1 ) );
  expText = expandTabs( insText, 0, mTabDist, mNullSubsChar,
                        &expInsLen );
  free( (void *) expText );
  outStr = (char *)malloc( end - start + expInsLen +
                           nLines * ( rectEnd + FL_TEXT_MAX_EXP_CHAR_LEN ) + 1 );

  /* Loop over all lines in the buffer between start and end overlaying the
     text between rectStart and rectEnd and padding appropriately.  Trim
     trailing space from line (whitespace at the ends of lines otherwise
     tends to multiply, since additional padding is added to maintain it */
  outPtr = outStr;
  lineStart = start;
  insPtr = insText;
  for (;;) {
    lineEnd = line_end( lineStart );
    line = text_range( lineStart, lineEnd );
    insLine = copyLine( insPtr, &len );
    insPtr += len;
    overlayRectInLine( line, insLine, rectStart, rectEnd, mTabDist,
                       mUseTabs, mNullSubsChar, outPtr, &len, &endOffset );
    free( (void *) line );
    free( (void *) insLine );
    for ( c = outPtr + len - 1; c > outPtr && isspace( *c ); c-- )
      len--;
    outPtr += len;
    *outPtr++ = '\n';
    lineStart = lineEnd < mLength ? lineEnd + 1 : mLength;
    if ( *insPtr == '\0' )
      break;
    insPtr++;
  }
  if ( outPtr != outStr )
    outPtr--;   /* trim back off extra newline */
  *outPtr = '\0';

  /* replace the text between start and end with the new stuff */
  remove_( start, end );
  insert_( start, outStr );
  *nInserted = outPtr - outStr;
  *nDeleted = end - start;
  *endPos = start + ( outPtr - outStr ) - len + endOffset;
  free( (void *) outStr );
}

/*
** Insert characters from single-line string "insLine" in single-line string
** "line" at "column", leaving "insWidth" space before continuing line.
** "outLen" returns the number of characters written to "outStr", "endOffset"
** returns the number of characters from the beginning of the string to
** the right edge of the inserted text (as a hint for routines which need
** to position the cursor).
*/
static void insertColInLine( const char *line, char *insLine, int column, int insWidth,
                             int tabDist, int useTabs, char nullSubsChar, char *outStr, int *outLen,
                             int *endOffset ) {
  char * c, *outPtr, *retabbedStr;
  const char *linePtr;
  int indent, toIndent, len, postColIndent;

  /* copy the line up to "column" */
  outPtr = outStr;
  indent = 0;
  for ( linePtr = line; *linePtr != '\0'; linePtr++ ) {
    len = Fl_Text_Buffer::character_width( *linePtr, indent, tabDist, nullSubsChar );
    if ( indent + len > column )
      break;
    indent += len;
    *outPtr++ = *linePtr;
  }

  /* If "column" falls in the middle of a character, and the character is a
     tab, leave it off and leave the indent short and it will get padded
     later.  If it's a control character, insert it and adjust indent
     accordingly. */
  if ( indent < column && *linePtr != '\0' ) {
    postColIndent = indent + len;
    if ( *linePtr == '\t' )
      linePtr++;
    else {
      *outPtr++ = *linePtr++;
      indent += len;
    }
  } else
    postColIndent = indent;

  /* If there's no text after the column and no text to insert, that's all */
  if ( *insLine == '\0' && *linePtr == '\0' ) {
    *outLen = *endOffset = outPtr - outStr;
    return;
  }

  /* pad out to column if text is too short */
  if ( indent < column ) {
    addPadding( outPtr, indent, column, tabDist, useTabs, nullSubsChar, &len );
    outPtr += len;
    indent = column;
  }

  /* Copy the text from "insLine" (if any), recalculating the tabs as if
     the inserted string began at column 0 to its new column destination */
  if ( *insLine != '\0' ) {
    retabbedStr = realignTabs( insLine, 0, indent, tabDist, useTabs,
                               nullSubsChar, &len );
    for ( c = retabbedStr; *c != '\0'; c++ ) {
      *outPtr++ = *c;
      len = Fl_Text_Buffer::character_width( *c, indent, tabDist, nullSubsChar );
      indent += len;
    }
    free( (void *) retabbedStr );
  }

  /* If the original line did not extend past "column", that's all */
  if ( *linePtr == '\0' ) {
    *outLen = *endOffset = outPtr - outStr;
    return;
  }

  /* Pad out to column + width of inserted text + (additional original
     offset due to non-breaking character at column) */
  toIndent = column + insWidth + postColIndent - column;
  addPadding( outPtr, indent, toIndent, tabDist, useTabs, nullSubsChar, &len );
  outPtr += len;
  indent = toIndent;

  /* realign tabs for text beyond "column" and write it out */
  retabbedStr = realignTabs( linePtr, postColIndent, indent, tabDist,
                             useTabs, nullSubsChar, &len );
  strcpy( outPtr, retabbedStr );
  free( (void *) retabbedStr );
  *endOffset = outPtr - outStr;
  *outLen = ( outPtr - outStr ) + len;
}

/*
** Remove characters in single-line string "line" between displayed positions
** "rectStart" and "rectEnd", and write the result to "outStr", which is
** assumed to be large enough to hold the returned string.  Note that in
** certain cases, it is possible for the string to get longer due to
** expansion of tabs.  "endOffset" returns the number of characters from
** the beginning of the string to the point where the characters were
** deleted (as a hint for routines which need to position the cursor).
*/
static void deleteRectFromLine( const char *line, int rectStart, int rectEnd,
                                int tabDist, int useTabs, char nullSubsChar, char *outStr, int *outLen,
                                int *endOffset ) {
  int indent, preRectIndent, postRectIndent, len;
  const char *c;
  char *retabbedStr, *outPtr;

  /* copy the line up to rectStart */
  outPtr = outStr;
  indent = 0;
  for ( c = line; *c != '\0'; c++ ) {
    if ( indent > rectStart )
      break;
    len = Fl_Text_Buffer::character_width( *c, indent, tabDist, nullSubsChar );
    if ( indent + len > rectStart && ( indent == rectStart || *c == '\t' ) )
      break;
    indent += len;
    *outPtr++ = *c;
  }
  preRectIndent = indent;

  /* skip the characters between rectStart and rectEnd */
  for ( ; *c != '\0' && indent < rectEnd; c++ )
    indent += Fl_Text_Buffer::character_width( *c, indent, tabDist, nullSubsChar );
  postRectIndent = indent;

  /* If the line ended before rectEnd, there's nothing more to do */
  if ( *c == '\0' ) {
    *outPtr = '\0';
    *outLen = *endOffset = outPtr - outStr;
    return;
  }

  /* fill in any space left by removed tabs or control characters
     which straddled the boundaries */
  indent = max( rectStart + postRectIndent - rectEnd, preRectIndent );
  addPadding( outPtr, preRectIndent, indent, tabDist, useTabs, nullSubsChar,
              &len );
  outPtr += len;

  /* Copy the rest of the line.  If the indentation has changed, preserve
     the position of non-whitespace characters by converting tabs to
     spaces, then back to tabs with the correct offset */
  retabbedStr = realignTabs( c, postRectIndent, indent, tabDist, useTabs,
                             nullSubsChar, &len );
  strcpy( outPtr, retabbedStr );
  free( (void *) retabbedStr );
  *endOffset = outPtr - outStr;
  *outLen = ( outPtr - outStr ) + len;
}

/*
** Overlay characters from single-line string "insLine" on single-line string
** "line" between displayed character offsets "rectStart" and "rectEnd".
** "outLen" returns the number of characters written to "outStr", "endOffset"
** returns the number of characters from the beginning of the string to
** the right edge of the inserted text (as a hint for routines which need
** to position the cursor).
*/
static void overlayRectInLine( const char *line, char *insLine, int rectStart,
                               int rectEnd, int tabDist, int useTabs, char nullSubsChar, char *outStr,
                               int *outLen, int *endOffset ) {
  char * c, *outPtr, *retabbedStr;
  int inIndent, outIndent, len, postRectIndent;
  const char *linePtr;

  /* copy the line up to "rectStart" */
  outPtr = outStr;
  inIndent = outIndent = 0;
  for ( linePtr = line; *linePtr != '\0'; linePtr++ ) {
    len = Fl_Text_Buffer::character_width( *linePtr, inIndent, tabDist, nullSubsChar );
    if ( inIndent + len > rectStart )
      break;
    inIndent += len;
    outIndent += len;
    *outPtr++ = *linePtr;
  }

  /* If "rectStart" falls in the middle of a character, and the character
     is a tab, leave it off and leave the outIndent short and it will get
     padded later.  If it's a control character, insert it and adjust
     outIndent accordingly. */
  if ( inIndent < rectStart && *linePtr != '\0' ) {
    if ( *linePtr == '\t' ) {
      linePtr++;
      inIndent += len;
    } else {
      *outPtr++ = *linePtr++;
      outIndent += len;
      inIndent += len;
    }
  }

  /* skip the characters between rectStart and rectEnd */
  postRectIndent = rectEnd;
  for ( ; *linePtr != '\0'; linePtr++ ) {
    inIndent += Fl_Text_Buffer::character_width( *linePtr, inIndent, tabDist, nullSubsChar );
    if ( inIndent >= rectEnd ) {
      linePtr++;
      postRectIndent = inIndent;
      break;
    }
  }

  /* If there's no text after rectStart and no text to insert, that's all */
  if ( *insLine == '\0' && *linePtr == '\0' ) {
    *outLen = *endOffset = outPtr - outStr;
    return;
  }

  /* pad out to rectStart if text is too short */
  if ( outIndent < rectStart ) {
    addPadding( outPtr, outIndent, rectStart, tabDist, useTabs, nullSubsChar,
                &len );
    outPtr += len;
  }
  outIndent = rectStart;

  /* Copy the text from "insLine" (if any), recalculating the tabs as if
     the inserted string began at column 0 to its new column destination */
  if ( *insLine != '\0' ) {
    retabbedStr = realignTabs( insLine, 0, rectStart, tabDist, useTabs,
                               nullSubsChar, &len );
    for ( c = retabbedStr; *c != '\0'; c++ ) {
      *outPtr++ = *c;
      len = Fl_Text_Buffer::character_width( *c, outIndent, tabDist, nullSubsChar );
      outIndent += len;
    }
    free( (void *) retabbedStr );
  }

  /* If the original line did not extend past "rectStart", that's all */
  if ( *linePtr == '\0' ) {
    *outLen = *endOffset = outPtr - outStr;
    return;
  }

  /* Pad out to rectEnd + (additional original offset
     due to non-breaking character at right boundary) */
  addPadding( outPtr, outIndent, postRectIndent, tabDist, useTabs,
              nullSubsChar, &len );
  outPtr += len;
  outIndent = postRectIndent;

  /* copy the text beyond "rectEnd" */
  strcpy( outPtr, linePtr );
  *endOffset = outPtr - outStr;
  *outLen = ( outPtr - outStr ) + strlen( linePtr );
}

void Fl_Text_Selection::set( int startpos, int endpos ) {
  mSelected = startpos != endpos;
  mRectangular = 0;
  mStart = min( startpos, endpos );
  mEnd = max( startpos, endpos );
}

void Fl_Text_Selection::set_rectangular( int startpos, int endpos,
    int rectStart, int rectEnd ) {
  mSelected = rectStart < rectEnd;
  mRectangular = 1;
  mStart = startpos;
  mEnd = endpos;
  mRectStart = rectStart;
  mRectEnd = rectEnd;
}

int Fl_Text_Selection::position( int *startpos, int *endpos ) {
  if ( !mSelected )
    return 0;
  *startpos = mStart;
  *endpos = mEnd;

  return 1;
}

int Fl_Text_Selection::position( int *startpos, int *endpos,
                                 int *isRect, int *rectStart, int *rectEnd ) {
  if ( !mSelected )
    return 0;
  *isRect = mRectangular;
  *startpos = mStart;
  *endpos = mEnd;
  if ( mRectangular ) {
    *rectStart = mRectStart;
    *rectEnd = mRectEnd;
  }
  return 1;
}

/*
** Return true if position "pos" with indentation "dispIndex" is in
** the Fl_Text_Selection.
*/
int Fl_Text_Selection::includes(int pos, int lineStartPos, int dispIndex) {
  return selected() &&
         ( (!rectangular() && pos >= start() && pos < end()) ||
           (rectangular() && pos >= start() && lineStartPos <= end() &&
            dispIndex >= rect_start() && dispIndex < rect_end())
         );
}



char * Fl_Text_Buffer::selection_text_( Fl_Text_Selection *sel ) {
  int start, end, isRect, rectStart, rectEnd;
  char *s;

  /* If there's no selection, return an allocated empty string */
  if ( !sel->position( &start, &end, &isRect, &rectStart, &rectEnd ) ) {
    s = (char *)malloc( 1 );
    *s = '\0';
    return s;
  }

  /* If the selection is not rectangular, return the selected range */
  if ( isRect )
    return text_in_rectangle( start, end, rectStart, rectEnd );
  else
    return text_range( start, end );
}

void Fl_Text_Buffer::remove_selection_( Fl_Text_Selection *sel ) {
  int start, end;
  int isRect, rectStart, rectEnd;

  if ( !sel->position( &start, &end, &isRect, &rectStart, &rectEnd ) )
    return;
  if ( isRect )
    remove_rectangular( start, end, rectStart, rectEnd );
  else {
    remove( start, end );
    //undoyankcut = undocut;
  }
}

void Fl_Text_Buffer::replace_selection_( Fl_Text_Selection *sel, const char *s ) {
  int start, end, isRect, rectStart, rectEnd;
  Fl_Text_Selection oldSelection = *sel;

  /* If there's no selection, return */
  if ( !sel->position( &start, &end, &isRect, &rectStart, &rectEnd ) )
    return;

  /* Do the appropriate type of replace */
  if ( isRect )
    replace_rectangular( start, end, rectStart, rectEnd, s );
  else
    replace( start, end, s );

  /* Unselect (happens automatically in BufReplace, but BufReplaceRect
     can't detect when the contents of a selection goes away) */
  sel->mSelected = 0;
  redisplay_selection( &oldSelection, sel );
}

static void addPadding( char *string, int startIndent, int toIndent,
                        int tabDist, int useTabs, char nullSubsChar, int *charsAdded ) {
  char * outPtr;
  int len, indent;

  indent = startIndent;
  outPtr = string;
  if ( useTabs ) {
    while ( indent < toIndent ) {
      len = Fl_Text_Buffer::character_width( '\t', indent, tabDist, nullSubsChar );
      if ( len > 1 && indent + len <= toIndent ) {
        *outPtr++ = '\t';
        indent += len;
      } else {
        *outPtr++ = ' ';
        indent++;
      }
    }
  } else {
    while ( indent < toIndent ) {
      *outPtr++ = ' ';
      indent++;
    }
  }
  *charsAdded = outPtr - string;
}

/*
** Call the stored modify callback procedure(s) for this buffer to update the
** changed area(s) on the screen and any other listeners.
*/
void Fl_Text_Buffer::call_modify_callbacks( int pos, int nDeleted,
    int nInserted, int nRestyled, const char *deletedText ) {
  int i;

  for ( i = 0; i < mNModifyProcs; i++ )
    ( *mNodifyProcs[ i ] ) ( pos, nInserted, nDeleted, nRestyled,
                             deletedText, mCbArgs[ i ] );
}

/*
** Call the stored pre-delete callback procedure(s) for this buffer to update 
** the changed area(s) on the screen and any other listeners.
*/
void Fl_Text_Buffer::call_predelete_callbacks(int pos, int nDeleted) {
    int i;
    
    for (i=0; i<mNPredeleteProcs; i++)
    	(*mPredeleteProcs[i])(pos, nDeleted, mPredeleteCbArgs[i]);
}

/*
** Call the stored redisplay procedure(s) for this buffer to update the
** screen for a change in a selection.
*/
void Fl_Text_Buffer::redisplay_selection( Fl_Text_Selection *oldSelection,
    Fl_Text_Selection *newSelection ) {
  int oldStart, oldEnd, newStart, newEnd, ch1Start, ch1End, ch2Start, ch2End;

  /* If either selection is rectangular, add an additional character to
     the end of the selection to request the redraw routines to wipe out
     the parts of the selection beyond the end of the line */
  oldStart = oldSelection->mStart;
  newStart = newSelection->mStart;
  oldEnd = oldSelection->mEnd;
  newEnd = newSelection->mEnd;
  if ( oldSelection->mRectangular )
    oldEnd++;
  if ( newSelection->mRectangular )
    newEnd++;

  /* If the old or new selection is unselected, just redisplay the
     single area that is (was) selected and return */
  if ( !oldSelection->mSelected && !newSelection->mSelected )
    return;
  if ( !oldSelection->mSelected ) {
    call_modify_callbacks( newStart, 0, 0, newEnd - newStart, NULL );
    return;
  }
  if ( !newSelection->mSelected ) {
    call_modify_callbacks( oldStart, 0, 0, oldEnd - oldStart, NULL );
    return;
  }

  /* If the selection changed from normal to rectangular or visa versa, or
     if a rectangular selection changed boundaries, redisplay everything */
  if ( ( oldSelection->mRectangular && !newSelection->mRectangular ) ||
       ( !oldSelection->mRectangular && newSelection->mRectangular ) ||
       ( oldSelection->mRectangular && (
           ( oldSelection->mRectStart != newSelection->mRectStart ) ||
           ( oldSelection->mRectEnd != newSelection->mRectEnd ) ) ) ) {
    call_modify_callbacks( min( oldStart, newStart ), 0, 0,
                           max( oldEnd, newEnd ) - min( oldStart, newStart ), NULL );
    return;
  }

  /* If the selections are non-contiguous, do two separate updates
     and return */
  if ( oldEnd < newStart || newEnd < oldStart ) {
    call_modify_callbacks( oldStart, 0, 0, oldEnd - oldStart, NULL );
    call_modify_callbacks( newStart, 0, 0, newEnd - newStart, NULL );
    return;
  }

  /* Otherwise, separate into 3 separate regions: ch1, and ch2 (the two
     changed areas), and the unchanged area of their intersection,
     and update only the changed area(s) */
  ch1Start = min( oldStart, newStart );
  ch2End = max( oldEnd, newEnd );
  ch1End = max( oldStart, newStart );
  ch2Start = min( oldEnd, newEnd );
  if ( ch1Start != ch1End )
    call_modify_callbacks( ch1Start, 0, 0, ch1End - ch1Start, NULL );
  if ( ch2Start != ch2End )
    call_modify_callbacks( ch2Start, 0, 0, ch2End - ch2Start, NULL );
}

void Fl_Text_Buffer::move_gap( int pos ) {
  int gapLen = mGapEnd - mGapStart;

  if ( pos > mGapStart )
    memmove( &mBuf[ mGapStart ], &mBuf[ mGapEnd ],
             pos - mGapStart );
  else
    memmove( &mBuf[ pos + gapLen ], &mBuf[ pos ], mGapStart - pos );
  mGapEnd += pos - mGapStart;
  mGapStart += pos - mGapStart;
}

/*
** reallocate the text storage in "buf" to have a gap starting at "newGapStart"
** and a gap size of "newGapLen", preserving the buffer's current contents.
*/
void Fl_Text_Buffer::reallocate_with_gap( int newGapStart, int newGapLen ) {
  char * newBuf;
  int newGapEnd;

  newBuf = (char *)malloc( mLength + newGapLen );
  newGapEnd = newGapStart + newGapLen;
  if ( newGapStart <= mGapStart ) {
    memcpy( newBuf, mBuf, newGapStart );
    memcpy( &newBuf[ newGapEnd ], &mBuf[ newGapStart ],
            mGapStart - newGapStart );
    memcpy( &newBuf[ newGapEnd + mGapStart - newGapStart ],
            &mBuf[ mGapEnd ], mLength - mGapStart );
  } else { /* newGapStart > mGapStart */
    memcpy( newBuf, mBuf, mGapStart );
    memcpy( &newBuf[ mGapStart ], &mBuf[ mGapEnd ],
            newGapStart - mGapStart );
    memcpy( &newBuf[ newGapEnd ],
            &mBuf[ mGapEnd + newGapStart - mGapStart ],
            mLength - newGapStart );
  }
  free( (void *) mBuf );
  mBuf = newBuf;
  mGapStart = newGapStart;
  mGapEnd = newGapEnd;
#ifdef PURIFY
{int i; for ( i = mGapStart; i < mGapEnd; i++ ) mBuf[ i ] = '.'; }
#endif
}

/*
** Update all of the selections in "buf" for changes in the buffer's text
*/
void Fl_Text_Buffer::update_selections( int pos, int nDeleted,
                                        int nInserted ) {
  mPrimary.update( pos, nDeleted, nInserted );
  mSecondary.update( pos, nDeleted, nInserted );
  mHighlight.update( pos, nDeleted, nInserted );
}

/*
** Update an individual selection for changes in the corresponding text
*/
void Fl_Text_Selection::update( int pos, int nDeleted,
                                int nInserted ) {
  if ( !mSelected || pos > mEnd )
    return;
  if ( pos + nDeleted <= mStart ) {
    mStart += nInserted - nDeleted;
    mEnd += nInserted - nDeleted;
  } else if ( pos <= mStart && pos + nDeleted >= mEnd ) {
    mStart = pos;
    mEnd = pos;
    mSelected = 0;
  } else if ( pos <= mStart && pos + nDeleted < mEnd ) {
    mStart = pos;
    mEnd = nInserted + mEnd - nDeleted;
  } else if ( pos < mEnd ) {
    mEnd += nInserted - nDeleted;
    if ( mEnd <= mStart )
      mSelected = 0;
  }
}

/*
** Search forwards in buffer "buf" for character "searchChar", starting
** with the character "startPos", and returning the result in "foundPos"
** returns 1 if found, 0 if not.  (The difference between this and
** BufSearchForward is that it's optimized for single characters.  The
** overall performance of the text widget is dependent on its ability to
** count lines quickly, hence searching for a single character: newline)
*/
int Fl_Text_Buffer::findchar_forward( int startPos, char searchChar,
                                    int *foundPos ) {
  int pos, gapLen = mGapEnd - mGapStart;

  if (startPos < 0 || startPos >= mLength) {
    *foundPos = mLength;
    return 0;
  }

  pos = startPos;
  while ( pos < mGapStart ) {
    if ( mBuf[ pos ] == searchChar ) {
      *foundPos = pos;
      return 1;
    }
    pos++;
  }
  while ( pos < mLength ) {
    if ( mBuf[ pos + gapLen ] == searchChar ) {
      *foundPos = pos;
      return 1;
    }
    pos++;
  }
  *foundPos = mLength;
  return 0;
}

/*
** Search backwards in buffer "buf" for character "searchChar", starting
** with the character BEFORE "startPos", returning the result in "foundPos"
** returns 1 if found, 0 if not.  (The difference between this and
** BufSearchBackward is that it's optimized for single characters.  The
** overall performance of the text widget is dependent on its ability to
** count lines quickly, hence searching for a single character: newline)
*/
int Fl_Text_Buffer::findchar_backward( int startPos, char searchChar,
                                     int *foundPos ) {
  int pos, gapLen = mGapEnd - mGapStart;

  if ( startPos <= 0 || startPos > mLength ) {
    *foundPos = 0;
    return 0;
  }
  pos = startPos - 1;
  while ( pos >= mGapStart ) {
    if ( mBuf[ pos + gapLen ] == searchChar ) {
      *foundPos = pos;
      return 1;
    }
    pos--;
  }
  while ( pos >= 0 ) {
    if ( mBuf[ pos ] == searchChar ) {
      *foundPos = pos;
      return 1;
    }
    pos--;
  }
  *foundPos = 0;
  return 0;
}

/*
** Copy from "text" to end up to but not including newline (or end of "text")
** and return the copy as the function value, and the length of the line in
** "lineLen"
*/
static char *copyLine( const char *text, int *lineLen ) {
  int len = 0;
  const char *c;
  char *outStr;

  for ( c = text; *c != '\0' && *c != '\n'; c++ )
    len++;
  outStr = (char *)malloc( len + 1 );
  strlcpy( outStr, text, len + 1);
  *lineLen = len;
  return outStr;
}

/*
** Count the number of newlines in a null-terminated text string;
*/
static int countLines( const char *string ) {
  const char * c;
  int lineCount = 0;

  for ( c = string; *c != '\0'; c++ )
    if ( *c == '\n' ) lineCount++;
  return lineCount;
}

/*
** Measure the width in displayed characters of string "text"
*/
static int textWidth( const char *text, int tabDist, char nullSubsChar ) {
  int width = 0, maxWidth = 0;
  const char *c;

  for ( c = text; *c != '\0'; c++ ) {
    if ( *c == '\n' ) {
      if ( width > maxWidth )
        maxWidth = width;
      width = 0;
    } else
      width += Fl_Text_Buffer::character_width( *c, width, tabDist, nullSubsChar );
  }
  if ( width > maxWidth )
    return width;
  return maxWidth;
}

/*
** Find the first and last character position in a line within a rectangular
** selection (for copying).  Includes tabs which cross rectStart, but not
** control characters which do so.  Leaves off tabs which cross rectEnd.
**
** Technically, the calling routine should convert tab characters which
** cross the right boundary of the selection to spaces which line up with
** the edge of the selection.  Unfortunately, the additional memory
** management required in the parent routine to allow for the changes
** in string size is not worth all the extra work just for a couple of
** shifted characters, so if a tab protrudes, just lop it off and hope
** that there are other characters in the selection to establish the right
** margin for subsequent columnar pastes of this data.
*/
void Fl_Text_Buffer::rectangular_selection_boundaries( int lineStartPos,
    int rectStart, int rectEnd, int *selStart, int *selEnd ) {
  int pos, width, indent = 0;
  char c;

  /* find the start of the selection */
  for ( pos = lineStartPos; pos < mLength; pos++ ) {
    c = character( pos );
    if ( c == '\n' )
      break;
    width = Fl_Text_Buffer::character_width( c, indent, mTabDist, mNullSubsChar );
    if ( indent + width > rectStart ) {
      if ( indent != rectStart && c != '\t' ) {
        pos++;
        indent += width;
      }
      break;
    }
    indent += width;
  }
  *selStart = pos;

  /* find the end */
  for ( ; pos < mLength; pos++ ) {
    c = character( pos );
    if ( c == '\n' )
      break;
    width = Fl_Text_Buffer::character_width( c, indent, mTabDist, mNullSubsChar );
    indent += width;
    if ( indent > rectEnd ) {
      if ( indent - width != rectEnd && c != '\t' )
        pos++;
      break;
    }
  }
  *selEnd = pos;
}

/*
** Adjust the space and tab characters from string "text" so that non-white
** characters remain stationary when the text is shifted from starting at
** "origIndent" to starting at "newIndent".  Returns an allocated string
** which must be freed by the caller with XtFree.
*/
static char *realignTabs( const char *text, int origIndent, int newIndent,
                          int tabDist, int useTabs, char nullSubsChar, int *newLength ) {
  char * expStr, *outStr;
  int len;

  /* If the tabs settings are the same, retain original tabs */
  if ( origIndent % tabDist == newIndent % tabDist ) {
    len = strlen( text );
    outStr = (char *)malloc( len + 1 );
    strcpy( outStr, text );
    *newLength = len;
    return outStr;
  }

  /* If the tab settings are not the same, brutally convert tabs to
     spaces, then back to tabs in the new position */
  expStr = expandTabs( text, origIndent, tabDist, nullSubsChar, &len );
  if ( !useTabs ) {
    *newLength = len;
    return expStr;
  }
  outStr = unexpandTabs( expStr, newIndent, tabDist, nullSubsChar, newLength );
  free( (void *) expStr );
  return outStr;
}

/*
** Expand tabs to spaces for a block of text.  The additional parameter
** "startIndent" if nonzero, indicates that the text is a rectangular selection
** beginning at column "startIndent"
*/
static char *expandTabs( const char *text, int startIndent, int tabDist,
                         char nullSubsChar, int *newLen ) {
  char * outStr, *outPtr;
  const char *c;
  int indent, len, outLen = 0;

  /* rehearse the expansion to figure out length for output string */
  indent = startIndent;
  for ( c = text; *c != '\0'; c++ ) {
    if ( *c == '\t' ) {
      len = Fl_Text_Buffer::character_width( *c, indent, tabDist, nullSubsChar );
      outLen += len;
      indent += len;
    } else if ( *c == '\n' ) {
      indent = startIndent;
      outLen++;
    } else {
      indent += Fl_Text_Buffer::character_width( *c, indent, tabDist, nullSubsChar );
      outLen++;
    }
  }

  /* do the expansion */
  outStr = (char *)malloc( outLen + 1 );
  outPtr = outStr;
  indent = startIndent;
  for ( c = text; *c != '\0'; c++ ) {
    if ( *c == '\t' ) {
      len = Fl_Text_Buffer::expand_character( *c, indent, outPtr, tabDist, nullSubsChar );
      outPtr += len;
      indent += len;
    } else if ( *c == '\n' ) {
      indent = startIndent;
      *outPtr++ = *c;
    } else {
      indent += Fl_Text_Buffer::character_width( *c, indent, tabDist, nullSubsChar );
      *outPtr++ = *c;
    }
  }
  outStr[ outLen ] = '\0';
  *newLen = outLen;
  return outStr;
}

/*
** Convert sequences of spaces into tabs.  The threshold for conversion is
** when 3 or more spaces can be converted into a single tab, this avoids
** converting double spaces after a period withing a block of text.
*/
static char *unexpandTabs( char *text, int startIndent, int tabDist,
                           char nullSubsChar, int *newLen ) {
  char * outStr, *outPtr, *c, expandedChar[ FL_TEXT_MAX_EXP_CHAR_LEN ];
  int indent, len;

  outStr = (char *)malloc( strlen( text ) + 1 );
  outPtr = outStr;
  indent = startIndent;
  for ( c = text; *c != '\0'; ) {
    if ( *c == ' ' ) {
      len = Fl_Text_Buffer::expand_character( '\t', indent, expandedChar, tabDist,
                                              nullSubsChar );
      if ( len >= 3 && !strncmp( c, expandedChar, len ) ) {
        c += len;
        *outPtr++ = '\t';
        indent += len;
      } else {
        *outPtr++ = *c++;
        indent++;
      }
    } else if ( *c == '\n' ) {
      indent = startIndent;
      *outPtr++ = *c++;
    } else {
      *outPtr++ = *c++;
      indent++;
    }
  }
  *outPtr = '\0';
  *newLen = outPtr - outStr;
  return outStr;
}

static int max( int i1, int i2 ) {
  return i1 >= i2 ? i1 : i2;
}

static int min( int i1, int i2 ) {
  return i1 <= i2 ? i1 : i2;
}

int
Fl_Text_Buffer::insertfile(const char *file, int pos, int buflen) {
  FILE *fp;  int r;
  if (!(fp = fopen(file, "r"))) return 1;
  char *buffer = new char[buflen];
  for (; (r = fread(buffer, 1, buflen - 1, fp)) > 0; pos += r) {
    buffer[r] = (char)0;
    insert(pos, buffer);
  }

  int e = ferror(fp) ? 2 : 0;
  fclose(fp);
  delete[] buffer;
  return e;
}

int
Fl_Text_Buffer::outputfile(const char *file, int start, int end, int buflen) {
  FILE *fp;
  if (!(fp = fopen(file, "w"))) return 1;
  for (int n; (n = min(end - start, buflen)); start += n) {
    const char *p = text_range(start, start + n);
    int r = fwrite(p, 1, n, fp);
    free((void *)p);
    if (r != n) break;
  }

  int e = ferror(fp) ? 2 : 0;
  fclose(fp);
  return e;
}


//
// End of "$Id: Fl_Text_Buffer.cxx 6011 2008-01-04 20:32:37Z matt $".
//
