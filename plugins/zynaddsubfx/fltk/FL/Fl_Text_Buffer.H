//
// "$Id: Fl_Text_Buffer.H 6822 2009-07-04 00:24:26Z fabien $"
//
// Header file for Fl_Text_Buffer class.
//
// Copyright 2001-2009 by Bill Spitzak and others.
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

/* \file
   Fl_Text_Buffer, Fl_Text_Selection widget . */

#ifndef FL_TEXT_BUFFER_H
#define FL_TEXT_BUFFER_H

/* Maximum length in characters of a tab or control character expansion
   of a single buffer character */
#define FL_TEXT_MAX_EXP_CHAR_LEN 20

#include "Fl_Export.H"

/** \class Fl_Text_Selection
    This is an internal class for Fl_Text_Buffer to manage text selections.
    
    \todo	members must be documented
 */
class FL_EXPORT Fl_Text_Selection {
  friend class Fl_Text_Buffer;

  public:
    void set(int start, int end);
    void set_rectangular(int start, int end, int rectStart, int rectEnd);
    void update(int pos, int nDeleted, int nInserted);
    char rectangular() const { return mRectangular; }
    int start() const { return mStart; }
    int end() const { return mEnd; }
    int rect_start() const { return mRectStart; }
    int rect_end() const { return mRectEnd; }
    /**
       Returns a non-zero number if any text has been selected, or 0
       if no text is selected.
    */
    char selected() const { return mSelected; }
    void selected(char b) { mSelected = b; }
    int includes(int pos, int lineStartPos, int dispIndex) const;
    int position(int* start, int* end) const;
    int position(int* start, int* end, int* isRect, int* rectStart, int* rectEnd) const;


  protected:
    char mSelected;
    char mRectangular;
    int mStart;
    int mEnd;
    int mRectStart;
    int mRectEnd;
};

typedef void (*Fl_Text_Modify_Cb)(int pos, int nInserted, int nDeleted,
                                  int nRestyled, const char* deletedText,
                                  void* cbArg);
typedef void (*Fl_Text_Predelete_Cb)(int pos, int nDeleted, void* cbArg);

/**
  The Fl_Text_Buffer class is used by the Fl_Text_Display
  and Fl_Text_Editor to manage complex text data and is based upon the
  excellent NEdit text editor engine - see http://www.nedit.org/.
*/
/**
  The Fl_Text_Buffer class is used by the 
  Fl_Text_Display
  and 
  Fl_Text_Editor
  to manage complex text data and is based upon the
  excellent NEdit text editor engine - see
  http://www.nedit.org/.
*/
class FL_EXPORT Fl_Text_Buffer {
  public:
    Fl_Text_Buffer(int requestedSize = 0, int preferredGapSize = 1024);
    ~Fl_Text_Buffer();

  /**    Returns the number of characters in the buffer.  */
    int length() const { return mLength; }
    char* text() const;
    void text(const char* text);
    char* text_range(int start, int end) const;
    char character(int pos) const;
    char* text_in_rectangle(int start, int end, int rectStart, int rectEnd) const;
    void insert(int pos, const char* text);
  /**    Appends the text string to the end of the buffer.  */
    void append(const char* t) { insert(length(), t); }
    void remove(int start, int end);
    void replace(int start, int end, const char *text);
    void copy(Fl_Text_Buffer* fromBuf, int fromStart, int fromEnd, int toPos);
    int undo(int *cp=0);
    void canUndo(char flag=1);
    int insertfile(const char *file, int pos, int buflen = 128*1024);
    /**
      Appends the named file to the end of the buffer. Returns 0 on 
      success, non-zero on error (strerror() contains reason).  1 indicates 
      open for read failed (no data loaded). 2 indicates error occurred 
      while reading data (data was partially loaded).
    */
    int appendfile(const char *file, int buflen = 128*1024)
      { return insertfile(file, length(), buflen); }
    /** Loads a text file into the buffer */
    int loadfile(const char *file, int buflen = 128*1024)
      { select(0, length()); remove_selection(); return appendfile(file, buflen); }
    int outputfile(const char *file, int start, int end, int buflen = 128*1024);
    /** Saves a text file from the current buffer */
    int savefile(const char *file, int buflen = 128*1024)
      { return outputfile(file, 0, length(), buflen); }

    void insert_column(int column, int startPos, const char* text,
                       int* charsInserted, int* charsDeleted);

    void replace_rectangular(int start, int end, int rectStart, int rectEnd,
                             const char* text);

    void overlay_rectangular(int startPos, int rectStart, int rectEnd,
                             const char* text, int* charsInserted,
                             int* charsDeleted);

    void remove_rectangular(int start, int end, int rectStart, int rectEnd);
    void clear_rectangular(int start, int end, int rectStart, int rectEnd);
    /** Gets the tab width.  */
    int tab_distance() const { return mTabDist; }
    void tab_distance(int tabDist);
    void select(int start, int end);
    /** Returns a non 0 value if text has been selected, 0 otherwise */
    int selected() const { return mPrimary.selected(); }
    void unselect();
    void select_rectangular(int start, int end, int rectStart, int rectEnd);
    int selection_position(int* start, int* end);

    int selection_position(int* start, int* end, int* isRect, int* rectStart,
                           int* rectEnd);

    char* selection_text();
    void remove_selection();
    void replace_selection(const char* text);
    void secondary_select(int start, int end);
     /** Returns a non 0 value if text has been selected in the secondary
       text selection, 0 otherwise */
    int secondary_selected() { return mSecondary.selected(); }
     /** Clears any selection in the secondary text selection object. */
    void secondary_unselect();

    void secondary_select_rectangular(int start, int end, int rectStart,
                                      int rectEnd);

    int secondary_selection_position(int* start, int* end);
    int secondary_selection_position(int* start, int* end, int* isRect,
                                     int* rectStart, int* rectEnd);

    char* secondary_selection_text();
    void remove_secondary_selection();
    void replace_secondary_selection(const char* text);
    void highlight(int start, int end);
    /**
      Returns the highlighted text. When you are done with the
      text, free it using the free() function.
    */
    int highlight() { return mHighlight.selected(); }
    void unhighlight();
    void highlight_rectangular(int start, int end, int rectStart, int rectEnd);

    int highlight_position(int* start, int* end);
    int highlight_position(int* start, int* end, int* isRect, int* rectStart,
                           int* rectEnd);

    char* highlight_text();
    void add_modify_callback(Fl_Text_Modify_Cb bufModifiedCB, void* cbArg);
    void remove_modify_callback(Fl_Text_Modify_Cb bufModifiedCB, void* cbArg);

    /**
       Calls all modify callbacks that have been registered using
       the add_modify_callback()
       method.
    */
    void call_modify_callbacks() { call_modify_callbacks(0, 0, 0, 0, 0); }

    void add_predelete_callback(Fl_Text_Predelete_Cb bufPredelCB, void* cbArg);
    void remove_predelete_callback(Fl_Text_Predelete_Cb predelCB, void* cbArg);

   /**
     Calls the stored pre-delete callback procedure(s) for this buffer to update 
     the changed area(s) on the screen and any other listeners.
   */
   void call_predelete_callbacks() { call_predelete_callbacks(0, 0); }

    char* line_text(int pos) const;
    int line_start(int pos) const;
    int line_end(int pos) const;
    int word_start(int pos) const;
    int word_end(int pos) const;
    int expand_character(int pos, int indent, char *outStr) const;

    static int expand_character(char c, int indent, char* outStr, int tabDist,
                                char nullSubsChar);

    static int character_width(char c, int indent, int tabDist, char nullSubsChar);
    int count_displayed_characters(int lineStartPos, int targetPos) const;
    int skip_displayed_characters(int lineStartPos, int nChars);
    int count_lines(int startPos, int endPos) const;
    int skip_lines(int startPos, int nLines);
    int rewind_lines(int startPos, int nLines);
    int findchar_forward(int startPos, char searchChar, int* foundPos) const;
    int findchar_backward(int startPos, char searchChar, int* foundPos) const;
    int findchars_forward(int startPos, const char* searchChars, int* foundPos) const;
    int findchars_backward(int startPos, const char* searchChars, int* foundPos) const;

    int search_forward(int startPos, const char* searchString, int* foundPos,
                       int matchCase = 0) const;

    int search_backward(int startPos, const char* searchString, int* foundPos,
                        int matchCase = 0) const;

    int substitute_null_characters(char* string, int length);
    void unsubstitute_null_characters(char* string);
    /**    Returns the current nul substitution character.  */
    char null_substitution_character() const { return mNullSubsChar; }
    /**    Returns the primary selection.  */
    const Fl_Text_Selection* primary_selection() const { return &mPrimary; }
    /**    Returns the primary selection.  */
    Fl_Text_Selection* primary_selection() { return &mPrimary; }
    /**    Returns the secondary selection.  */
    const Fl_Text_Selection* secondary_selection() const { return &mSecondary; }
    /**    Returns the current highlight selection.  */
    const Fl_Text_Selection* highlight_selection() const { return &mHighlight; }

  protected:
    void call_modify_callbacks(int pos, int nDeleted, int nInserted,
                               int nRestyled, const char* deletedText) const;
    void call_predelete_callbacks(int pos, int nDeleted) const;

    int insert_(int pos, const char* text);
    void remove_(int start, int end);

    void remove_rectangular_(int start, int end, int rectStart, int rectEnd,
                             int* replaceLen, int* endPos);

    void insert_column_(int column, int startPos, const char* insText,
                        int* nDeleted, int* nInserted, int* endPos);

    void overlay_rectangular_(int startPos, int rectStart, int rectEnd,
                              const char* insText, int* nDeleted,
                              int* nInserted, int* endPos);

    void redisplay_selection(Fl_Text_Selection* oldSelection,
                             Fl_Text_Selection* newSelection) const;

    void move_gap(int pos);
    void reallocate_with_gap(int newGapStart, int newGapLen);
    char* selection_text_(Fl_Text_Selection* sel) const;
    void remove_selection_(Fl_Text_Selection* sel);
    void replace_selection_(Fl_Text_Selection* sel, const char* text);

    void rectangular_selection_boundaries(int lineStartPos, int rectStart,
                                          int rectEnd, int* selStart,
                                          int* selEnd) const;

    void update_selections(int pos, int nDeleted, int nInserted);

    Fl_Text_Selection mPrimary;   /**< highlighted areas */
    Fl_Text_Selection mSecondary; /**< highlighted areas */
    Fl_Text_Selection mHighlight; /**< highlighted areas */
    int mLength;                /**< length of the text in the buffer (the length
                                   of the buffer itself must be calculated:
                                   gapEnd - gapStart + length) */
    char* mBuf;                 /**< allocated memory where the text is stored */
    int mGapStart;              /**< points to the first character of the gap */
    int mGapEnd;                /**< points to the first char after the gap */
    // The hardware tab distance used by all displays for this buffer,
    // and used in computing offsets for rectangular selection operations.
    int mTabDist;               /**< equiv. number of characters in a tab */
    int mUseTabs;               /**< True if buffer routines are allowed to use
                                   tabs for padding in rectangular operations */
    int mNModifyProcs;          /**< number of modify-redisplay procs attached */
    Fl_Text_Modify_Cb*          /**< procedures to call when buffer is */
    mModifyProcs;               /**< modified to redisplay contents */
    void** mCbArgs;             /**< caller arguments for modifyProcs above */
    int mNPredeleteProcs;	/**< number of pre-delete procs attached */
    Fl_Text_Predelete_Cb*	/**< procedure to call before text is deleted */
	 mPredeleteProcs;	/**< from the buffer; at most one is supported. */
    void **mPredeleteCbArgs;	/**< caller argument for pre-delete proc above */
    int mCursorPosHint;         /**< hint for reasonable cursor position after
                                   a buffer modification operation */
    char mNullSubsChar;         /**< NEdit is based on C null-terminated strings,
                                   so ascii-nul characters must be substituted
                                   with something else.  This is the else, but
                                   of course, things get quite messy when you
                                   use it */
    char mCanUndo;		/**< if this buffer is used for attributes, it must
				   not do any undo calls */
    int mPreferredGapSize;	/**< the default allocation for the text gap is 1024
                                   bytes and should only be increased if frequent
                                   and large changes in buffer size are expected */
};

#endif

//
// End of "$Id: Fl_Text_Buffer.H 6822 2009-07-04 00:24:26Z fabien $".
//
