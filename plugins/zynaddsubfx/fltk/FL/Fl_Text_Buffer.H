//
// "$Id: Fl_Text_Buffer.H 7527 2010-04-18 14:33:33Z engelsman $"
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

/*
 Suggested UTF-8 terminology for this file:
 
 ?? "length" is the number of characters in a string
 ?? "size" is the number of bytes
 ?? "index" is the position in a string in number of characters
 ?? "offset" is the position in a strin in bytes (and must be kept on a charater boundary)
 (there seems to be no standard in Uncode documents, howevere "length" is commonly
 referencing the number of bytes. Maybe "bytes" and "glyphs" would be the most
 obvious way to describe sizes?)
 
 "character size" is the size of a UTF-8 character in bytes
 "character width" is the width of a Unicode character in pixels
 
 "column" was orginally defined as a character offset from the left margin. It was
 identical to the byte offset. In UTF-8, we have neither a byte offset nor
 truly fixed width fonts. Column could be a pixel value multiplied with
 an average character width (which is a bearable approximation).
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
   \brief Set a regtangular selection range.
   \param start byte offset to first selected character
   \param end byte offset pointing after last selected character
   \param rectStart first selected column
   \param rectEnd last selected column +1
   */
  void set_rectangular(int start, int end, int rectStart, int rectEnd);
  
  /**
   \brief Updates a selection afer text was modified.
   Updates an individual selection for changes in the corresponding text
   \param pos byte offset into text buffer at which the change occured
   \param nDeleted number of bytes deleted from the buffer
   \param nInserted number of bytes inserted into the buffer
   */
  void update(int pos, int nDeleted, int nInserted);
  
  /**
   \brief Returns true if the selection is rectangular.
   \return flag
   */
  char rectangular() const { return mRectangular; }
  
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
   \brief Return the first column of the rectangular selection.
   \return first column of rectangular selection
   */
  int rect_start() const { return mRectStart; }
  
  /**
   \brief Return the last column of the rectangular selection + 1.
   \return last column of rectangular selection +1
   */
  int rect_end() const { return mRectEnd; }
  
  /**
   \brief Returns true if any text is selected.
   \return a non-zero number if any text has been selected, or 0
   if no text is selected.
   */
  char selected() const { return mSelected; }
  
  /**
   \brief Modify the 'selected' flag.
   \param b new flag
   */
  void selected(char b) { mSelected = b; }
  
  /**
   Return true if position \p pos with indentation \p dispIndex is in
   the Fl_Text_Selection.
   */
  int includes(int pos, int lineStartPos, int dispIndex) const;
  
  /**
   \brief Return the positions of this selection.
   \param start retrun byte offset to first selected character
   \param end retrun byte offset pointing after last selected character
   \return true if selected
   */
  int position(int* start, int* end) const;
  
  /**
   \brief Return the positions of this rectangular selection.
   \param start return byte offset to first selected character
   \param end return byte offset pointing after last selected character
   \param isRect return if the selection is rectangular
   \param rectStart return first selected column
   \param rectEnd return last selected column +1
   \return true if selected
   */
  int position(int* start, int* end, int* isRect, int* rectStart, int* rectEnd) const;
  
protected:
  
  char mSelected;     ///< this flag is set if any text is selected  
  char mRectangular;  ///< this flag is set if the selection is rectangular
  int mStart;         ///< byte offset to the first selected character
  int mEnd;           ///< byte offset to the character after the last selected character
  int mRectStart;     ///< first selected column (see "column")
  int mRectEnd;       ///< last selected column +1 (see "column")
};

typedef void (*Fl_Text_Modify_Cb)(int pos, int nInserted, int nDeleted,
                                  int nRestyled, const char* deletedText,
                                  void* cbArg);

typedef void (*Fl_Text_Predelete_Cb)(int pos, int nDeleted, void* cbArg);


/**
 \brief This class manages unicode displayed in one or more Fl_Text_Display widgets.
 
 The Fl_Text_Buffer class is used by the Fl_Text_Display
 and Fl_Text_Editor to manage complex text data and is based upon the
 excellent NEdit text editor engine - see http://www.nedit.org/.
 \todo unicode check
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
   \return newly allocated text buffer - must be free'd
   */  
  char* text() const;
  
  /**  
   Replaces the entire contents of the text buffer 
   \todo unicode check
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
   \return newly allocated text buffer - must be free'd
   */
  char* text_range(int start, int end) const;
  
  /**
   Returns the character at the specified position pos in the buffer.
   Positions start at 0 
   \param pos byte offset into buffer
   \return Unicode UCS-4 encoded character
   */
  unsigned int character(int pos) const;
  
  /**
   Convert a byte offset in buffer into a memory address.
   */
  const char *address(int pos) const
  { return (pos < mGapStart) ? mBuf+pos : mBuf+pos+mGapEnd-mGapStart; }

  /**
   Convert a byte offset in buffer into a memory address.
   */
  char *address(int pos)
  { return (pos < mGapStart) ? mBuf+pos : mBuf+pos+mGapEnd-mGapStart; }
  
  /** 
   Returns the text from the given rectangle. When you are done
   with the text, free it using the free() function.
   \todo unicode check
   */
  char* text_in_rectangle(int start, int end, int rectStart, int rectEnd) const;
  
  /** 
   Inserts null-terminated string \p text at position \p pos. 
   \param pos insertion position as byte offset (must be utf-8 character aligned)
   \param text utf-8 encoded and nul terminated text
   */
  void insert(int pos, const char* text);
  
  /**
   Appends the text string to the end of the buffer.  
   \todo unicode check
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
   Copies text from one buffer to this one; fromBuf may
   be the same as this.
   \todo unicode check
   */
  void copy(Fl_Text_Buffer* fromBuf, int fromStart, int fromEnd, int toPos);
  
  /**
   Undo text modification according to the undo variables or insert text 
   from the undo buffer
   \todo unicode check
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
   \todo unicode check
   */
  int insertfile(const char *file, int pos, int buflen = 128*1024);
  
  /**
   Appends the named file to the end of the buffer. Returns 0 on 
   success, non-zero on error (strerror() contains reason).  1 indicates 
   open for read failed (no data loaded). 2 indicates error occurred 
   while reading data (data was partially loaded).
   \todo unicode check
   */
  int appendfile(const char *file, int buflen = 128*1024)
  { return insertfile(file, length(), buflen); }
  
  /** 
   Loads a text file into the buffer 
   \todo unicode check
   */
  int loadfile(const char *file, int buflen = 128*1024)
  { select(0, length()); remove_selection(); return appendfile(file, buflen); }
  
  /**
   Writes the specified portions of the file to a file. Returns 0 on success, non-zero 
   on error (strerror() contains reason).  1 indicates open for write failed 
   (no data saved). 2 indicates error occurred while writing data 
   (data was partially saved).
   \todo unicode check
   */
  int outputfile(const char *file, int start, int end, int buflen = 128*1024);
  
  /** 
   Saves a text file from the current buffer 
   \todo unicode check
   */
  int savefile(const char *file, int buflen = 128*1024)
  { return outputfile(file, 0, length(), buflen); }
  
  /**
   Insert \p s columnwise into buffer starting at displayed character
   position \p column on the line beginning at \p startPos. Opens a rectangular
   space the width and height of \p s, by moving all text to the right of
   \p column right.  If \p charsInserted and \p charsDeleted are not NULL, the
   number of characters inserted and deleted in the operation (beginning
   at \p startPos) are returned in these arguments.
   \todo unicode check
   */
  void insert_column(int column, int startPos, const char* text,
                     int* charsInserted, int* charsDeleted);
  
  /**
   Replaces a rectangular area in the buffer, given by \p start, \p end,
   \p rectStart, and \p rectEnd, with \p text.  If \p text is vertically
   longer than the rectangle, add extra lines to make room for it.
   \todo unicode check
   */
  void replace_rectangular(int start, int end, int rectStart, int rectEnd,
                           const char* text);
  
  /**
   Overlay \p text between displayed character positions \p rectStart and
   \p rectEnd on the line beginning at \p startPos.  If \p charsInserted and
   \p charsDeleted are not NULL, the number of characters inserted and deleted
   in the operation (beginning at \p startPos) are returned in these arguments.
   \todo unicode check
   */
  void overlay_rectangular(int startPos, int rectStart, int rectEnd,
                           const char* text, int* charsInserted,
                           int* charsDeleted);
  
  /**
   Removes a rectangular swath of characters between character positions start
   and end and horizontal displayed-character offsets rectStart and rectEnd.
   \todo unicode check
   */
  void remove_rectangular(int start, int end, int rectStart, int rectEnd);
  
  /**
   Clears text in the specified area.
   It clears a rectangular "hole" out of the buffer between character positions
   start and end and horizontal displayed-character offsets rectStart and
   rectEnd.
   \todo unicode check
   */
  void clear_rectangular(int start, int end, int rectStart, int rectEnd);
  
  /** 
   Gets the tab width.  
   \todo unicode check
   */
  int tab_distance() const { return mTabDist; }
  
  /**
   Set the hardware tab distance (width) used by all displays for this buffer,
   and used in computing offsets for rectangular selection operations.
   \todo unicode check
   */
  void tab_distance(int tabDist);
  
  /**  
   Selects a range of characters in the buffer.
   \todo unicode check
   */
  void select(int start, int end);
  
  /** 
   Returns a non 0 value if text has been selected, 0 otherwise 
   */
  int selected() const { return mPrimary.selected(); }
  
  /** 
   Cancels any previous selection on the primary text selection object 
   \todo unicode check
   */
  void unselect();
  
  /** 
   Achieves a rectangular selection on the primary text selection object 
   \todo unicode check
   */
  void select_rectangular(int start, int end, int rectStart, int rectEnd);
  
  /** 
   Gets the selection position 
   \todo unicode check
   */
  int selection_position(int* start, int* end);
  
  /** 
   Gets the selection position, and rectangular selection info 
   \todo unicode check
   */
  int selection_position(int* start, int* end, int* isRect, int* rectStart,
                         int* rectEnd);
  
  /**
   Returns the currently selected text. When you are done with
   the text, free it using the free() function.
   \todo unicode check
   */
  char* selection_text();

  /**  
   Removes the text in the primary selection.
   \todo unicode check
   */
  void remove_selection();
  
  /**
   Replaces the text in the primary selection.
   \todo unicode check
   */
  void replace_selection(const char* text);
  
  /**
   Selects a range of characters in the secondary selection.
   \todo unicode check
   */
  void secondary_select(int start, int end);
  
  /** 
   Returns a non 0 value if text has been selected in the secondary
   text selection, 0 otherwise 
   */
  int secondary_selected() { return mSecondary.selected(); }
  
  /** 
   Clears any selection in the secondary text selection object. 
   \todo unicode check
   */
  void secondary_unselect();
  
  /** 
   Achieves a rectangular selection on the secondary text selection object 
   \todo unicode check
   */
  void secondary_select_rectangular(int start, int end, int rectStart,
                                    int rectEnd);
  
  /**  
   Returns the current selection in the secondary text selection object.
   \todo unicode check
   */
  int secondary_selection_position(int* start, int* end);
  
  /** 
   Returns the current selection in the secondary text selection object.
   \todo unicode check
   */
  int secondary_selection_position(int* start, int* end, int* isRect,
                                   int* rectStart, int* rectEnd);
  
  /**
   Returns the text in the secondary selection. When you are
   done with the text, free it using the free() function.
   \todo unicode check
   */
  char* secondary_selection_text();
  
  /**  
   Removes the text from the buffer corresponding to the secondary text selection object.
   \todo unicode check
   */
  void remove_secondary_selection();
  
  /**  
   Replaces the text from the buffer corresponding to the secondary 
   text selection object with the new string \p text.
   \todo unicode check
   */
  void replace_secondary_selection(const char* text);
  
  /**  
   Highlights the specified text within the buffer.
   \todo unicode check
   */
  void highlight(int start, int end);
  
  /**
   Returns the highlighted text. When you are done with the
   text, free it using the free() function.
   */
  int highlight() { return mHighlight.selected(); }
  
  /**
   Unhighlights text in the buffer.
   \todo unicode check
   */
  void unhighlight();

  /** 
   Highlights a rectangular selection in the buffer 
   \todo unicode check
   */
  void highlight_rectangular(int start, int end, int rectStart, int rectEnd);
  
  /** 
   Highlights the specified text between \p start and \p end within the buffer.
   \todo unicode check
   */
  int highlight_position(int* start, int* end);
  
  /** 
   Highlights the specified rectangle of text within the buffer.
   \todo unicode check
   */
  int highlight_position(int* start, int* end, int* isRect, int* rectStart,
                         int* rectEnd);
  
  /**
   Returns the highlighted text. When you are done with the
   text, free it using the free() function.
   \todo unicode check
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
   \todo unicode check
   */
  void add_modify_callback(Fl_Text_Modify_Cb bufModifiedCB, void* cbArg);
  
  /**
   Removes a modify callback.
   \todo unicode check
   */
  void remove_modify_callback(Fl_Text_Modify_Cb bufModifiedCB, void* cbArg);
  
  /**
   Calls all modify callbacks that have been registered using
   the add_modify_callback()
   method.
   \todo unicode check
   */
  void call_modify_callbacks() { call_modify_callbacks(0, 0, 0, 0, 0); }
  
  /** 
   Adds a callback routine to be called before text is deleted from the buffer. 
   \todo unicode check
   */
  void add_predelete_callback(Fl_Text_Predelete_Cb bufPredelCB, void* cbArg);

  /** 
   Removes a callback routine \p bufPreDeleteCB associated with argument \p cbArg 
   to be called before text is deleted from the buffer. 
   \todo unicode check
   */
  void remove_predelete_callback(Fl_Text_Predelete_Cb predelCB, void* cbArg);
  
  /**
   Calls the stored pre-delete callback procedure(s) for this buffer to update 
   the changed area(s) on the screen and any other listeners.
   \todo unicode check
   */
  void call_predelete_callbacks() { call_predelete_callbacks(0, 0); }
  
  /**
   Returns the text from the entire line containing the specified
   character position. When you are done with the text, free it
   using the free() function.
   \todo unicode check
   */
  char* line_text(int pos) const;
  
  /** 
   Returns the position of the start of the line containing position \p pos. 
   \todo unicode check
   */
  int line_start(int pos) const;
  
  /** 
   Finds and returns the position of the end of the line containing position \p pos
   (which is either a pointer to the newline character ending the line,
   or a pointer to one character beyond the end of the buffer)
   \todo unicode check
   */
  int line_end(int pos) const;

  /** 
   Returns the position corresponding to the start of the word 
   \todo unicode check
   */
  int word_start(int pos) const;

  /**  
   Returns the position corresponding to the end of the word.
   \todo unicode check
   */
  int word_end(int pos) const;
  
  /**
   Expands the given character to a displayable format. Tabs and
   other control characters are given special treatment.
   Get a character from the text buffer expanded into its screen
   representation (which may be several characters for a tab or a
   control code).  Returns the number of characters written to \p outStr.
   \p indent is the number of characters from the start of the line
   for figuring tabs.  Output string is guranteed to be shorter or
   equal in length to FL_TEXT_MAX_EXP_CHAR_LEN
   \todo unicode check
   */
  int expand_character(int pos, int indent, char *outStr) const;
  
  /**
   Expand a single character \p c from the text buffer into it's displayable
   screen representation (which may be several characters for a tab or a
   control code).  Returns the number of characters added to \p outStr.
   \p indent is the number of characters from the start of the line
   for figuring tabs of length \p tabDist.  Output string is guaranteed 
   to be shorter or equal in length to FL_TEXT_MAX_EXP_CHAR_LEN
   Tabs and  other control characters are given special treatment.
   \param src address of utf-8 text
   \param indent 
   \param[out] outStr write substitution here
   \param tabDist
   \return number of byte in substitution
   */
  static int expand_character(const char *src, int indent, char* outStr, int tabDist);
  
  /**
   Return the length in displayed characters of character \p c expanded
   for display (as discussed above in expand_character() ).  
   \param src address of utf-8 text
   \param indent 
   \param tabDist
   \return number of byte in substitution
   */
  static int character_width(const char *src, int indent, int tabDist);
  static int character_width(const char    c, int indent, int tabDist);

  /**
   Count the number of displayed characters between buffer position
   \p lineStartPos and \p targetPos. (displayed characters are the characters
   shown on the screen to represent characters in the buffer, where tabs and
   control characters are expanded)
   \todo unicode check
   */
  int count_displayed_characters(int lineStartPos, int targetPos) const;

  /**
   Count forward from buffer position \p startPos in displayed characters
   (displayed characters are the characters shown on the screen to represent
   characters in the buffer, where tabs and control characters are expanded)
   \param lineStartPos byte offset into buffer
   \param nChars number of bytes that are sent to the display
   \return byte offset in input after all output bytes are sent
   \todo unicode check
   */
  int skip_displayed_characters(int lineStartPos, int nChars);
  
  /**
   Counts the number of newlines between \p startPos and \p endPos in buffer.
   The character at position \p endPos is not counted.
   \todo unicode check
   */
  int count_lines(int startPos, int endPos) const;

  /**
   Finds the first character of the line \p nLines forward from \p startPos
   in the buffer and returns its position
   \todo unicode check
   */
  int skip_lines(int startPos, int nLines);
  
  /**
   Finds and returns the position of the first character of the line \p nLines backwards
   from \p startPos (not counting the character pointed to by \p startpos if
   that is a newline) in the buffer.  \p nLines == 0 means find the beginning of the line
   \todo unicode check
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
   \todo unicode check
   */
  int findchar_forward(int startPos, char searchChar, int* foundPos) const;
  
  /**
   Search backwards in buffer \p buf for character \p searchChar, starting
   with the character BEFORE \p startPos, returning the result in \p foundPos
   returns 1 if found, 0 if not.  (The difference between this and
   BufSearchBackward is that it's optimized for single characters.  The
   overall performance of the text widget is dependent on its ability to
   count lines quickly, hence searching for a single character: newline)
   \todo unicode check
   */
  int findchar_backward(int startPos, char searchChar, int* foundPos) const;
  
  /**
   Finds the next occurrence of the specified characters.
   Search forwards in buffer for characters in \p searchChars, starting
   with the character \p startPos, and returning the result in \p foundPos
   returns 1 if found, 0 if not.
   \todo unicode check
   */
  int findchars_forward(int startPos, const char* searchChars, int* foundPos) const;
  
  /** 
   Finds the previous occurrence of the specified characters.
   Search backwards in buffer for characters in \p searchChars, starting
   with the character BEFORE \p startPos, returning the result in \p foundPos
   returns 1 if found, 0 if not.
   \todo unicode check
   */
  int findchars_backward(int startPos, const char* searchChars, int* foundPos) const;
  
  /**
   Search forwards in buffer for string \p searchString, starting with the
   character \p startPos, and returning the result in \p foundPos
   returns 1 if found, 0 if not.
   \todo unicode check
   */
  int search_forward(int startPos, const char* searchString, int* foundPos,
                     int matchCase = 0) const;
  
  /**
   Search backwards in buffer for string <i>searchCharssearchString</i>, starting with the
   character BEFORE \p startPos, returning the result in \p foundPos
   returns 1 if found, 0 if not.
   \todo unicode check
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
  
protected:

  /**
   Calls the stored modify callback procedure(s) for this buffer to update the
   changed area(s) on the screen and any other listeners.
   \todo unicode check
   */
  void call_modify_callbacks(int pos, int nDeleted, int nInserted,
                             int nRestyled, const char* deletedText) const;
  
  /**
   Calls the stored pre-delete callback procedure(s) for this buffer to update 
   the changed area(s) on the screen and any other listeners.
   \todo unicode check
   */
  void call_predelete_callbacks(int pos, int nDeleted) const;
  
  /**
   Internal (non-redisplaying) version of BufInsert. Returns the length of
   text inserted (this is just strlen(\p text), however this calculation can be
   expensive and the length will be required by any caller who will continue
   on to call redisplay). \p pos must be contiguous with the existing text in
   the buffer (i.e. not past the end).
   \todo unicode check
   */
  int insert_(int pos, const char* text);
  
  /**
   Internal (non-redisplaying) version of BufRemove.  Removes the contents
   of the buffer between start and end (and moves the gap to the site of
   the delete).
   \todo unicode check
   */
  void remove_(int start, int end);
  
  /**
   Deletes a rectangle of text without calling the modify callbacks. Returns
   the number of characters replacing those between \p start and \p end. Note that
   in some pathological cases, deleting can actually increase the size of
   the buffer because of tab expansions. \p endPos returns the buffer position
   of the point in the last line where the text was removed (as a hint for
   routines which need to position the cursor after a delete operation)
   \todo unicode check
   */
  void remove_rectangular_(int start, int end, int rectStart, int rectEnd,
                           int* replaceLen, int* endPos);
  
  /**
   Inserts a column of text without calling the modify callbacks.  Note that
   in some pathological cases, inserting can actually decrease the size of
   the buffer because of spaces being coalesced into tabs.  \p nDeleted and
   \p nInserted return the number of characters deleted and inserted beginning
   at the start of the line containing \p startPos.  \p endPos returns buffer
   position of the lower left edge of the inserted column (as a hint for
   routines which need to set a cursor position).
   \todo unicode check
   */
  void insert_column_(int column, int startPos, const char* insText,
                      int* nDeleted, int* nInserted, int* endPos);
  
  /**
   Overlay a rectangular area of text without calling the modify callbacks.
   \p nDeleted and \p nInserted return the number of characters deleted and
   inserted beginning at the start of the line containing \p startPos.
   \p endPos returns buffer position of the lower left edge of the inserted
   column (as a hint for routines which need to set a cursor position).
   \todo unicode check
   */
  void overlay_rectangular_(int startPos, int rectStart, int rectEnd,
                            const char* insText, int* nDeleted,
                            int* nInserted, int* endPos);
  
  /**
   Calls the stored redisplay procedure(s) for this buffer to update the
   screen for a change in a selection.
   \todo unicode check
   */
  void redisplay_selection(Fl_Text_Selection* oldSelection,
                           Fl_Text_Selection* newSelection) const;
  
  /**
   \todo unicode check
   */
  void move_gap(int pos);
  
  /**
   Reallocates the text storage in the buffer to have a gap starting at \p newGapStart
   and a gap size of \p newGapLen, preserving the buffer's current contents.
   \todo unicode check
   */
  void reallocate_with_gap(int newGapStart, int newGapLen);
  
  char* selection_text_(Fl_Text_Selection* sel) const;
  
  /**  
   Removes the text from the buffer corresponding to \p sel.
   \todo unicode check
   */
  void remove_selection_(Fl_Text_Selection* sel);
  
  /** 
   Replaces the \p text in selection \p sel.
   \todo unicode check
   */
  void replace_selection_(Fl_Text_Selection* sel, const char* text);
  
  /**
   Finds the first and last character position in a line within a rectangular
   selection (for copying).  Includes tabs which cross rectStart, but not
   control characters which do so.  Leaves off tabs which cross rectEnd.
   
   Technically, the calling routine should convert tab characters which
   cross the right boundary of the selection to spaces which line up with
   the edge of the selection.  Unfortunately, the additional memory
   management required in the parent routine to allow for the changes
   in string size is not worth all the extra work just for a couple of
   shifted characters, so if a tab protrudes, just lop it off and hope
   that there are other characters in the selection to establish the right
   margin for subsequent columnar pastes of this data.
   \todo unicode check
   */
  void rectangular_selection_boundaries(int lineStartPos, int rectStart,
                                        int rectEnd, int* selStart,
                                        int* selEnd) const;
  
  /**
   Updates all of the selections in the buffer for changes in the buffer's text
   \todo unicode check
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
  int mUseTabs;                   /**< True if buffer routines are allowed to use
                                   tabs for padding in rectangular operations */
  int mNModifyProcs;              /**< number of modify-redisplay procs attached */
  Fl_Text_Modify_Cb*              /**< procedures to call when buffer is */
  mModifyProcs;                   /**< modified to redisplay contents */
  void** mCbArgs;                 /**< caller arguments for modifyProcs above */
  int mNPredeleteProcs;           /**< number of pre-delete procs attached */
  Fl_Text_Predelete_Cb*           /**< procedure to call before text is deleted */
  mPredeleteProcs;                /**< from the buffer; at most one is supported. */
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
// End of "$Id: Fl_Text_Buffer.H 7527 2010-04-18 14:33:33Z engelsman $".
//
