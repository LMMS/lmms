//
// "$Id: Fl_Text_Buffer.H 8148 2010-12-31 22:38:03Z matt $"
//
// Header file for Fl_Text_Buffer class.
//
// Copyright 2001-2010 by Bill Spitzak and others.
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


#undef ASSERT_UTF8

#ifdef ASSERT_UTF8
# include <assert.h>
# define IS_UTF8_ALIGNED(a) if (a && *a) assert(fl_utf8len(*(a))>0);
# define IS_UTF8_ALIGNED2(a, b) if (b>=0 && b<a->length()) assert(fl_utf8len(a->byte_at(b))>0);
#else
# define IS_UTF8_ALIGNED(a)
# define IS_UTF8_ALIGNED2(a, b)
#endif


/*
 "character size" is the size of a UTF-8 character in bytes
 "character width" is the width of a Unicode character in pixels 
 "column" was orginally defined as a character offset from the left margin. 
 It was identical to the byte offset. In UTF-8, we have neither a byte offset 
 nor truly fixed width fonts (*). Column could be a pixel value multiplied with
 an average character width (which is a bearable approximation).
 
 * in Unicode, there are no fixed width fonts! Even if the ASCII characters may 
   happen to be all the same width in pixels, chinese charcaters surely are not.
   There are plenty of exceptions, like ligatures, that make special handling of
   "fixed" character widths a nightmare. I decided to remove all references to
   fixed fonts and see "columns" as a multiple of the average width of a 
   character in the main font.
     - Matthias
 */


/* Maximum length in characters of a tab or control character expansion
 of a single buffer character */
#define FL_TEXT_MAX_EXP_CHAR_LEN 20

#include "Fl_Export.H"


/** 
 \class Fl_Text_Selection
 \brief This is an internal class for Fl_Text_Buffer to manage text selections.
 This class works correctly with utf-8 strings assuming that the parameters
 for all calls are on character boundaries.
 */
class FL_EXPORT Fl_Text_Selection {
  friend class Fl_Text_Buffer;
  
public:
  
  /**
   \brief Set the selection range.
   \param start byte offset to first selected character
   \param end byte offset pointing after last selected character
   */
  void set(int start, int end);
  
  /**
   \brief Updates a selection afer text was modified.
   Updates an individual selection for changes in the corresponding text
   \param pos byte offset into text buffer at which the change occured
   \param nDeleted number of bytes deleted from the buffer
   \param nInserted number of bytes inserted into the buffer
   */
  void update(int pos, int nDeleted, int nInserted);
  
  /**
   \brief Return the byte offset to the first selected character.
   \return byte offset
   */
  int start() const { return mStart; }
  
  /**
   \brief Return the byte ofsset to the character after the last selected character.
   \return byte offset
   */
  int end() const { return mEnd; }
  
  /**
   \brief Returns true if any text is selected.
   \return a non-zero number if any text has been selected, or 0
   if no text is selected.
   */
  bool selected() const { return mSelected; }
  
  /**
   \brief Modify the 'selected' flag.
   \param b new flag
   */
  void selected(bool b) { mSelected = b; }
  
  /**
   Return true if position \p pos with indentation \p dispIndex is in
   the Fl_Text_Selection.
   */
  int includes(int pos) const;
  
  /**
   \brief Return the positions of this selection.
   \param start retrun byte offset to first selected character
   \param end retrun byte offset pointing after last selected character
   \return true if selected
   */
  int position(int* start, int* end) const;
  
protected:
  
  int mStart;         ///< byte offset to the first selected character
  int mEnd;           ///< byte offset to the character after the last selected character
  bool mSelected;     ///< this flag is set if any text is selected  
};


typedef void (*Fl_Text_Modify_Cb)(int pos, int nInserted, int nDeleted,
                                  int nRestyled, const char* deletedText,
                                  void* cbArg);


typedef void (*Fl_Text_Predelete_Cb)(int pos, int nDeleted, void* cbArg);


/**
 \brief This class manages unicode displayed in one or more Fl_Text_Display widgets.
 
 All text in Fl_Text_Buffermust be encoded in UTF-8. All indices used in the 
 function calls must be aligned to the start of a UTF-8 sequence. All indices 
 and pointers returned will be aligned. All functions that return a single 
 character will return that in an unsiged int in UCS-4 encoding.
 
 The Fl_Text_Buffer class is used by the Fl_Text_Display
 and Fl_Text_Editor to manage complex text data and is based upon the
 excellent NEdit text editor engine - see http://www.nedit.org/.
 */
class FL_EXPORT Fl_Text_Buffer {
public:

  /**
   Create an empty text buffer of a pre-determined size.   
   \param requestedSize use this to avoid unnecessary re-allocation 
    if you know exactly how much the buffer will need to hold
   \param preferredGapSize Initial size for the buffer gap (empty space
    in the buffer where text might be inserted
    if the user is typing sequential chars)
   */
  Fl_Text_Buffer(int requestedSize = 0, int preferredGapSize = 1024);
  
  /** 
   Frees a text buffer 
   */
  ~Fl_Text_Buffer();
  
  /**
   \brief Returns the number of bytes in the buffer.
   \return size of text in bytes
   */
  int length() const { return mLength; }
  
  /**
   \brief Get a copy of the entire contents of the text buffer.
   Memory is allocated to contain the returned string, which the caller 
   must free.
   \return newly allocated text buffer - must be free'd, text is utf8
   */  
  char* text() const;
  
  /**  
   Replaces the entire contents of the text buffer.
   \param text Text must be valid utf8.
   */
  void text(const char* text);
  
  /**
   \brief Get a copy of a part of the text buffer.
   Return a copy of the text between \p start and \p end character positions
   from text buffer \p buf. Positions start at 0, and the range does not
   include the character pointed to by \p end.
   When you are done with the text, free it using the free() function.
   \param start byte offset to first character
   \param end byte offset after last character in range
   \return newly allocated text buffer - must be free'd, text is utf8
   */
  char* text_range(int start, int end) const;
  
  /**
   Returns the character at the specified position pos in the buffer.
   Positions start at 0 
   \param pos byte offset into buffer, pos must be at acharacter boundary
   \return Unicode UCS-4 encoded character
   */
  unsigned int char_at(int pos) const;
  
  /**
   Returns the raw byte at the specified position pos in the buffer.
   Positions start at 0 
   \param pos byte offset into buffer
   \return unencoded raw byte
   */
  char byte_at(int pos) const;
  
  /**
   Convert a byte offset in buffer into a memory address.
   \param pos byte offset into buffer
   \return byte offset converted to a memory address
   */
  const char *address(int pos) const
  { return (pos < mGapStart) ? mBuf+pos : mBuf+pos+mGapEnd-mGapStart; }

  /**
   Convert a byte offset in buffer into a memory address.
   \param pos byte offset into buffer
   \return byte offset converted to a memory address
   */
  char *address(int pos)
  { return (pos < mGapStart) ? mBuf+pos : mBuf+pos+mGapEnd-mGapStart; }
  
  /** 
   Inserts null-terminated string \p text at position \p pos. 
   \param pos insertion position as byte offset (must be utf-8 character aligned)
   \param text utf-8 encoded and nul terminated text
   */
  void insert(int pos, const char* text);
  
  /**
   Appends the text string to the end of the buffer.  
   \param t utf-8 encoded and nul terminated text
   */
  void append(const char* t) { insert(length(), t); }
  
  /**
   Deletes a range of characters in the buffer.
   \param start byte offset to first character to be removed
   \param end byte offset to charcatre after last character to be removed
   */
  void remove(int start, int end);
  
  /**
   Deletes the characters between \p start and \p end, and inserts the null-terminated string \p text in their place in the buffer.
   \param start byte offset to first character to be removed and new insert position
   \param end byte offset to charcatre after last character to be removed
   \param text utf-8 encoded and nul terminated text
   */
  void replace(int start, int end, const char *text);
  
  /**
   Copies text from one buffer to this one.
   \param fromBuf source text buffer may be the same as this
   \param fromStart byte offset into buffer
   \param fromEnd byte offset into buffer
   \param toPos destination byte offset into buffer
   */
  void copy(Fl_Text_Buffer* fromBuf, int fromStart, int fromEnd, int toPos);
  
  /**
   Undo text modification according to the undo variables or insert text 
   from the undo buffer
   */
  int undo(int *cp=0);
  
  /** 
   Lets the undo system know if we can undo changes 
   */
  void canUndo(char flag=1);
  
  /**
   Inserts a file at the specified position. Returns 0 on success, 
   non-zero on error (strerror() contains reason).  1 indicates open 
   for read failed (no data loaded). 2 indicates error occurred 
   while reading data (data was partially loaded).
   File can be UTF-8 or CP1252-encoded.
   If the input file is not UTF-8-encoded, the Fl_Text_Buffer widget will contain
   UTF-8-transcoded data. By default, the message Fl_Text_Buffer::file_encoding_warning_message
   will warn the user about this.
   \see input_file_was_transcoded and transcoding_warning_action.
   */
  int insertfile(const char *file, int pos, int buflen = 128*1024);
  
  /**
   Appends the named file to the end of the buffer. See also insertfile().
   */
  int appendfile(const char *file, int buflen = 128*1024)
  { return insertfile(file, length(), buflen); }
  
  /** 
   Loads a text file into the buffer. See also insertfile().
   */
  int loadfile(const char *file, int buflen = 128*1024)
  { select(0, length()); remove_selection(); return appendfile(file, buflen); }
  
  /**
   Writes the specified portions of the file to a file. Returns 0 on success, non-zero 
   on error (strerror() contains reason).  1 indicates open for write failed 
   (no data saved). 2 indicates error occurred while writing data 
   (data was partially saved).
   */
  int outputfile(const char *file, int start, int end, int buflen = 128*1024);
  
  /** 
   Saves a text file from the current buffer 
   */
  int savefile(const char *file, int buflen = 128*1024)
  { return outputfile(file, 0, length(), buflen); }
  
  /**
   Gets the tab width.  
   */
  int tab_distance() const { return mTabDist; }
  
  /**
   Set the hardware tab distance (width) used by all displays for this buffer,
   and used in computing offsets for rectangular selection operations.
   */
  void tab_distance(int tabDist);
  
  /**  
   Selects a range of characters in the buffer.
   */
  void select(int start, int end);
  
  /** 
   Returns a non 0 value if text has been selected, 0 otherwise 
   */
  int selected() const { return mPrimary.selected(); }
  
  /** 
   Cancels any previous selection on the primary text selection object 
   */
  void unselect();
  
  /** 
   Gets the selection position 
   */
  int selection_position(int* start, int* end);
  
  /** 
   Returns the currently selected text. When you are done with
   the text, free it using the free() function.
   */
  char* selection_text();

  /**  
   Removes the text in the primary selection.
   */
  void remove_selection();
  
  /**
   Replaces the text in the primary selection.
   */
  void replace_selection(const char* text);
  
  /**
   Selects a range of characters in the secondary selection.
   */
  void secondary_select(int start, int end);
  
  /** 
   Returns a non 0 value if text has been selected in the secondary
   text selection, 0 otherwise 
   */
  int secondary_selected() { return mSecondary.selected(); }
  
  /** 
   Clears any selection in the secondary text selection object. 
   */
  void secondary_unselect();
  
  /** 
   Returns the current selection in the secondary text selection object.
   */
  int secondary_selection_position(int* start, int* end);
  
  /** 
   Returns the text in the secondary selection. When you are
   done with the text, free it using the free() function.
   */
  char* secondary_selection_text();
  
  /**  
   Removes the text from the buffer corresponding to the secondary text selection object.
   */
  void remove_secondary_selection();
  
  /**  
   Replaces the text from the buffer corresponding to the secondary 
   text selection object with the new string \p text.
   */
  void replace_secondary_selection(const char* text);
  
  /**  
   Highlights the specified text within the buffer.
   */
  void highlight(int start, int end);
  
  /**
   Returns the highlighted text. When you are done with the
   text, free it using the free() function.
   */
  int highlight() { return mHighlight.selected(); }
  
  /**
   Unhighlights text in the buffer.
   */
  void unhighlight();

  /** 
   Highlights the specified text between \p start and \p end within the buffer.
   */
  int highlight_position(int* start, int* end);
  
  /** 
   Returns the highlighted text. When you are done with the
   text, free it using the free() function.
   */
  char* highlight_text();
  
  /**
   Adds a callback function that is called whenever the text buffer is
   modified. The callback function is declared as follows:
   
   \code
   typedef void (*Fl_Text_Modify_Cb)(int pos, int nInserted, int nDeleted,
      int nRestyled, const char* deletedText,
      void* cbArg);
   \endcode
   */
  void add_modify_callback(Fl_Text_Modify_Cb bufModifiedCB, void* cbArg);
  
  /**
   Removes a modify callback.
   */
  void remove_modify_callback(Fl_Text_Modify_Cb bufModifiedCB, void* cbArg);
  
  /**
   Calls all modify callbacks that have been registered using
   the add_modify_callback()
   method.
   */
  void call_modify_callbacks() { call_modify_callbacks(0, 0, 0, 0, 0); }
  
  /** 
   Adds a callback routine to be called before text is deleted from the buffer. 
   */
  void add_predelete_callback(Fl_Text_Predelete_Cb bufPredelCB, void* cbArg);

  /** 
   Removes a callback routine \p bufPreDeleteCB associated with argument \p cbArg 
   to be called before text is deleted from the buffer. 
   */
  void remove_predelete_callback(Fl_Text_Predelete_Cb predelCB, void* cbArg);
  
  /**
   Calls the stored pre-delete callback procedure(s) for this buffer to update 
   the changed area(s) on the screen and any other listeners.
   */
  void call_predelete_callbacks() { call_predelete_callbacks(0, 0); }
  
  /**
   Returns the text from the entire line containing the specified
   character position. When you are done with the text, free it
   using the free() function.
   \param pos byte index into buffer
   \return copy of utf8 text, must be free'd
   */
  char* line_text(int pos) const;
  
  /** 
   Returns the position of the start of the line containing position \p pos. 
   \param pos byte index into buffer
   \return byte offset to line start
   */
  int line_start(int pos) const;
  
  /** 
   Finds and returns the position of the end of the line containing position \p pos
   (which is either a pointer to the newline character ending the line,
   or a pointer to one character beyond the end of the buffer)
   \param pos byte index into buffer
   \return byte offset to line end
   */
  int line_end(int pos) const;

  /** 
   Returns the position corresponding to the start of the word 
   \param pos byte index into buffer
   \return byte offset to word start
   */
  int word_start(int pos) const;

  /**  
   Returns the position corresponding to the end of the word.
   \param pos byte index into buffer
   \return byte offset to word end
   */
  int word_end(int pos) const;
  
  /**
   Count the number of displayed characters between buffer position
   \p lineStartPos and \p targetPos. (displayed characters are the characters
   shown on the screen to represent characters in the buffer, where tabs and
   control characters are expanded)
   */
  int count_displayed_characters(int lineStartPos, int targetPos) const;

  /**
   Count forward from buffer position \p startPos in displayed characters
   (displayed characters are the characters shown on the screen to represent
   characters in the buffer, where tabs and control characters are expanded)
   \param lineStartPos byte offset into buffer
   \param nChars number of bytes that are sent to the display
   \return byte offset in input after all output bytes are sent
   */
  int skip_displayed_characters(int lineStartPos, int nChars);
  
  /**
   Counts the number of newlines between \p startPos and \p endPos in buffer.
   The character at position \p endPos is not counted.
   */
  int count_lines(int startPos, int endPos) const;

  /**
   Finds the first character of the line \p nLines forward from \p startPos
   in the buffer and returns its position
   */
  int skip_lines(int startPos, int nLines);
  
  /**
   Finds and returns the position of the first character of the line \p nLines backwards
   from \p startPos (not counting the character pointed to by \p startpos if
   that is a newline) in the buffer.  \p nLines == 0 means find the beginning of the line
   */
  int rewind_lines(int startPos, int nLines);

  /** 
   Finds the next occurrence of the specified character.
   Search forwards in buffer for character \p searchChar, starting
   with the character \p startPos, and returning the result in \p foundPos
   returns 1 if found, 0 if not.  (The difference between this and
   BufSearchForward is that it's optimized for single characters.  The
   overall performance of the text widget is dependent on its ability to
   count lines quickly, hence searching for a single character: newline)
   \param startPos byte offset to start position
   \param searchChar UCS-4 character that we want to find
   \param foundPos byte offset where the character was found
   \return 1 if found, 0 if not
   */
  int findchar_forward(int startPos, unsigned searchChar, int* foundPos) const;
  
  /**
   Search backwards in buffer \p buf for character \p searchChar, starting
   with the character BEFORE \p startPos, returning the result in \p foundPos
   returns 1 if found, 0 if not.  (The difference between this and
   BufSearchBackward is that it's optimized for single characters.  The
   overall performance of the text widget is dependent on its ability to
   count lines quickly, hence searching for a single character: newline)
   \param startPos byte offset to start position
   \param searchChar UCS-4 character that we want to find
   \param foundPos byte offset where the character was found
   \return 1 if found, 0 if not
   */
  int findchar_backward(int startPos, unsigned int searchChar, int* foundPos) const;
  
  /**
   Search forwards in buffer for string \p searchString, starting with the
   character \p startPos, and returning the result in \p foundPos
   returns 1 if found, 0 if not.
   \param startPos byte offset to start position
   \param searchString utf8 string that we want to find
   \param foundPos byte offset where the string was found
   \param matchCase if set, match character case
   \return 1 if found, 0 if not
   */
  int search_forward(int startPos, const char* searchString, int* foundPos,
                     int matchCase = 0) const;
  
  /**
   Search backwards in buffer for string <i>searchCharssearchString</i>, starting with the
   character BEFORE \p startPos, returning the result in \p foundPos
   returns 1 if found, 0 if not.
   \param startPos byte offset to start position
   \param searchString utf8 string that we want to find
   \param foundPos byte offset where the string was found
   \param matchCase if set, match character case
   \return 1 if found, 0 if not
   */
  int search_backward(int startPos, const char* searchString, int* foundPos,
                      int matchCase = 0) const;
  
  /** 
   Returns the primary selection.  
   */
  const Fl_Text_Selection* primary_selection() const { return &mPrimary; }
  
  /**
   Returns the primary selection. 
   */
  Fl_Text_Selection* primary_selection() { return &mPrimary; }
  
  /**
   Returns the secondary selection.
   */
  const Fl_Text_Selection* secondary_selection() const { return &mSecondary; }
  
  /**
   Returns the current highlight selection.
   */
  const Fl_Text_Selection* highlight_selection() const { return &mHighlight; }
  
  /**
   Returns the index of the previous character.
   \param ix index to the current char
   */
  int prev_char(int ix) const;
  int prev_char_clipped(int ix) const;
  
  /**
   Returns the index of the next character.
   \param ix index to the current char
   */
  int next_char(int ix) const;
  int next_char_clipped(int ix) const;
  
  /**
   Align an index into the buffer to the current or previous utf8 boundary.
   */
  int utf8_align(int) const;
  
  /**
   \brief true iff the loaded file has been transcoded to UTF-8
   */
  int input_file_was_transcoded;

  /** This message may be displayed using the fl_alert() function when a file
   which was not UTF-8 encoded is input.
   */
  static const char* file_encoding_warning_message;
  
  /** 
   \brief Pointer to a function called after reading a non UTF-8 encoded file.
   
   This function is called after reading a file if the file content
   was transcoded to UTF-8. Its default implementation calls fl_alert()
   with the text of \ref file_encoding_warning_message. No warning message is
   displayed if this pointer is set to NULL. Use \ref input_file_was_transcoded
   to be informed if file input required transcoding to UTF-8.
   */
  void (*transcoding_warning_action)(Fl_Text_Buffer*);
  
protected:

  /**
   Calls the stored modify callback procedure(s) for this buffer to update the
   changed area(s) on the screen and any other listeners.
   */
  void call_modify_callbacks(int pos, int nDeleted, int nInserted,
                             int nRestyled, const char* deletedText) const;
  
  /**
   Calls the stored pre-delete callback procedure(s) for this buffer to update 
   the changed area(s) on the screen and any other listeners.
   */
  void call_predelete_callbacks(int pos, int nDeleted) const;
  
  /**
   Internal (non-redisplaying) version of BufInsert. Returns the length of
   text inserted (this is just strlen(\p text), however this calculation can be
   expensive and the length will be required by any caller who will continue
   on to call redisplay). \p pos must be contiguous with the existing text in
   the buffer (i.e. not past the end).
   \return the number of bytes inserted
   */
  int insert_(int pos, const char* text);
  
  /**
   Internal (non-redisplaying) version of BufRemove.  Removes the contents
   of the buffer between start and end (and moves the gap to the site of
   the delete).
   */
  void remove_(int start, int end);
  
  /**
   Calls the stored redisplay procedure(s) for this buffer to update the
   screen for a change in a selection.
   */
  void redisplay_selection(Fl_Text_Selection* oldSelection,
                           Fl_Text_Selection* newSelection) const;
  
  /**
   Move the gap to start at a new position.
   */
  void move_gap(int pos);
  
  /**
   Reallocates the text storage in the buffer to have a gap starting at \p newGapStart
   and a gap size of \p newGapLen, preserving the buffer's current contents.
   */
  void reallocate_with_gap(int newGapStart, int newGapLen);
  
  char* selection_text_(Fl_Text_Selection* sel) const;
  
  /**  
   Removes the text from the buffer corresponding to \p sel.
   */
  void remove_selection_(Fl_Text_Selection* sel);
  
  /** 
   Replaces the \p text in selection \p sel.
   */
  void replace_selection_(Fl_Text_Selection* sel, const char* text);
  
  /**
   Updates all of the selections in the buffer for changes in the buffer's text
   */
  void update_selections(int pos, int nDeleted, int nInserted);
  
  Fl_Text_Selection mPrimary;     /**< highlighted areas */
  Fl_Text_Selection mSecondary;   /**< highlighted areas */
  Fl_Text_Selection mHighlight;   /**< highlighted areas */
  int mLength;                    /**< length of the text in the buffer (the length
                                   of the buffer itself must be calculated:
                                   gapEnd - gapStart + length) */
  char* mBuf;                     /**< allocated memory where the text is stored */
  int mGapStart;                  /**< points to the first character of the gap */
  int mGapEnd;                    /**< points to the first char after the gap */
  // The hardware tab distance used by all displays for this buffer,
  // and used in computing offsets for rectangular selection operations.
  int mTabDist;                   /**< equiv. number of characters in a tab */
  int mNModifyProcs;              /**< number of modify-redisplay procs attached */
  Fl_Text_Modify_Cb *mModifyProcs;/**< procedures to call when buffer is 
                                   modified to redisplay contents */
  void** mCbArgs;                 /**< caller arguments for modifyProcs above */
  int mNPredeleteProcs;           /**< number of pre-delete procs attached */
  Fl_Text_Predelete_Cb *mPredeleteProcs; /**< procedure to call before text is deleted
                                   from the buffer; at most one is supported. */
  void **mPredeleteCbArgs;        /**< caller argument for pre-delete proc above */
  int mCursorPosHint;             /**< hint for reasonable cursor position after
                                   a buffer modification operation */
  char mCanUndo;                  /**< if this buffer is used for attributes, it must
                                   not do any undo calls */
  int mPreferredGapSize;          /**< the default allocation for the text gap is 1024
                                   bytes and should only be increased if frequent
                                   and large changes in buffer size are expected */
};

#endif

//
// End of "$Id: Fl_Text_Buffer.H 8148 2010-12-31 22:38:03Z matt $".
//
