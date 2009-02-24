//
// "$Id: Fl_Text_Display.cxx 6105 2008-04-21 21:03:22Z matt $"
//
// Copyright 2001-2006 by Bill Spitzak and others.
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
#include <limits.h>
#include <ctype.h>
#include <FL/Fl.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Window.H>

#undef min
#undef max

// Text area margins.  Left & right margins should be at least 3 so that
// there is some room for the overhanging parts of the cursor!
#define TOP_MARGIN 1
#define BOTTOM_MARGIN 1
#define LEFT_MARGIN 3
#define RIGHT_MARGIN 3

#define NO_HINT -1

/* Masks for text drawing methods.  These are or'd together to form an
   integer which describes what drawing calls to use to draw a string */
#define FILL_MASK         0x0100
#define SECONDARY_MASK    0x0200
#define PRIMARY_MASK      0x0400
#define HIGHLIGHT_MASK    0x0800
#define BG_ONLY_MASK      0x1000
#define TEXT_ONLY_MASK    0x2000
#define STYLE_LOOKUP_MASK   0xff

/* Maximum displayable line length (how many characters will fit across the
   widest window).  This amount of memory is temporarily allocated from the
   stack in the draw_vline() method for drawing strings */
#define MAX_DISP_LINE_LEN 1000

static int max( int i1, int i2 );
static int min( int i1, int i2 );
static int countlines( const char *string );

/* The variables below are used in a timer event to allow smooth
   scrolling of the text area when the pointer has left the area. */
static int scroll_direction = 0;
static int scroll_amount = 0;
static int scroll_y = 0;
static int scroll_x = 0;

// CET - FIXME
#define TMPFONTWIDTH 6

Fl_Text_Display::Fl_Text_Display(int X, int Y, int W, int H,  const char* l)
    : Fl_Group(X, Y, W, H, l) {
  int i;

  mMaxsize = 0;
  damage_range1_start = damage_range1_end = -1;
  damage_range2_start = damage_range2_end = -1;
  dragPos = dragType = dragging = 0;
  display_insert_position_hint = 0;

  color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
  box(FL_DOWN_FRAME);
  textsize((uchar)FL_NORMAL_SIZE);
  textcolor(FL_FOREGROUND_COLOR);
  textfont(FL_HELVETICA);

  text_area.x = 0;
  text_area.y = 0;
  text_area.w = 0;
  text_area.h = 0;

  mVScrollBar = new Fl_Scrollbar(0,0,1,1);
  mVScrollBar->callback((Fl_Callback*)v_scrollbar_cb, this);
  mHScrollBar = new Fl_Scrollbar(0,0,1,1);
  mHScrollBar->callback((Fl_Callback*)h_scrollbar_cb, this);
  mHScrollBar->type(FL_HORIZONTAL);

  end();

  scrollbar_width(Fl::scrollbar_size());
  scrollbar_align(FL_ALIGN_BOTTOM_RIGHT);

  mCursorOn = 0;
  mCursorPos = 0;
  mCursorOldY = -100;
  mCursorToHint = NO_HINT;
  mCursorStyle = NORMAL_CURSOR;
  mCursorPreferredCol = -1;
  mBuffer = 0;
  mFirstChar = 0;
  mLastChar = 0;
  mNBufferLines = 0;
  mTopLineNum = mTopLineNumHint = 1;
  mAbsTopLineNum = 1;
  mNeedAbsTopLineNum = 0;
  mHorizOffset = mHorizOffsetHint = 0;

  mCursor_color = FL_FOREGROUND_COLOR;

  mFixedFontWidth = -1;
  mStyleBuffer = 0;
  mStyleTable = 0;
  mNStyles = 0;
  mNVisibleLines = 1;
  mLineStarts = new int[mNVisibleLines];
  mLineStarts[0] = 0;
  for (i=1; i<mNVisibleLines; i++)
    mLineStarts[i] = -1;
  mSuppressResync = 0;
  mNLinesDeleted = 0;
  mModifyingTabDistance = 0;

  mUnfinishedStyle = 0;
  mUnfinishedHighlightCB = 0;
  mHighlightCBArg = 0;

  mLineNumLeft = mLineNumWidth = 0;
  mContinuousWrap = 0;
  mWrapMargin = 0;
  mSuppressResync = mNLinesDeleted = mModifyingTabDistance = 0;
}

/*
** Free a text display and release its associated memory.  Note, the text
** BUFFER that the text display displays is a separate entity and is not
** freed, nor are the style buffer or style table.
*/
Fl_Text_Display::~Fl_Text_Display() {
  if (scroll_direction) {
    Fl::remove_timeout(scroll_timer_cb, this);
    scroll_direction = 0;
  }
  if (mBuffer) {
    mBuffer->remove_modify_callback(buffer_modified_cb, this);
    mBuffer->remove_predelete_callback(buffer_predelete_cb, this);
  }
  if (mLineStarts) delete[] mLineStarts;
}

/*
** Attach a text buffer to display, replacing the current buffer (if any)
*/
void Fl_Text_Display::buffer( Fl_Text_Buffer *buf ) {
  /* If the text display is already displaying a buffer, clear it off
     of the display and remove our callback from it */
  if ( buf == mBuffer) return;
  if ( mBuffer != 0 ) {
    buffer_modified_cb( 0, 0, mBuffer->length(), 0, 0, this );
	mNBufferLines = 0;
    mBuffer->remove_modify_callback( buffer_modified_cb, this );
    mBuffer->remove_predelete_callback( buffer_predelete_cb, this );
  }

  /* Add the buffer to the display, and attach a callback to the buffer for
     receiving modification information when the buffer contents change */
  mBuffer = buf;
  if (mBuffer) {
    mBuffer->add_modify_callback( buffer_modified_cb, this );
    mBuffer->add_predelete_callback( buffer_predelete_cb, this );

    /* Update the display */
    buffer_modified_cb( 0, buf->length(), 0, 0, 0, this );
  }

  /* Resize the widget to update the screen... */
  resize(x(), y(), w(), h());
}

/*
** Attach (or remove) highlight information in text display and redisplay.
** Highlighting information consists of a style buffer which parallels the
** normal text buffer, but codes font and color information for the display;
** a style table which translates style buffer codes (indexed by buffer
** character - 'A') into fonts and colors; and a callback mechanism for
** as-needed highlighting, triggered by a style buffer entry of
** "unfinishedStyle".  Style buffer can trigger additional redisplay during
** a normal buffer modification if the buffer contains a primary Fl_Text_Selection
** (see extendRangeForStyleMods for more information on this protocol).
**
** Style buffers, tables and their associated memory are managed by the caller.
*/
void
Fl_Text_Display::highlight_data(Fl_Text_Buffer *styleBuffer,
                                const Style_Table_Entry *styleTable,
                                int nStyles, char unfinishedStyle,
                                Unfinished_Style_Cb unfinishedHighlightCB,
                                void *cbArg ) {
  mStyleBuffer = styleBuffer;
  mStyleTable = styleTable;
  mNStyles = nStyles;
  mUnfinishedStyle = unfinishedStyle;
  mUnfinishedHighlightCB = unfinishedHighlightCB;
  mHighlightCBArg = cbArg;

  mStyleBuffer->canUndo(0);
#if 0
  // FIXME: this is in nedit code -- is it needed?	
    /* Call TextDSetFont to combine font information from style table and
       primary font, adjust font-related parameters, and then redisplay */
    TextDSetFont(textD, textD->fontStruct);
#endif
  damage(FL_DAMAGE_EXPOSE);
}

#if 0
  // FIXME: this is in nedit code -- is it needed?	
/*
** Change the (non highlight) font
*/
void TextDSetFont(textDisp *textD, XFontStruct *fontStruct) {
    Display *display = XtDisplay(textD->w);
    int i, maxAscent = fontStruct->ascent, maxDescent = fontStruct->descent;
    int width, height, fontWidth;
    Pixel bgPixel, fgPixel, selectFGPixel, selectBGPixel;
    Pixel highlightFGPixel, highlightBGPixel;
    XGCValues values;
    XFontStruct *styleFont;
    
    /* If font size changes, cursor will be redrawn in a new position */
    blankCursorProtrusions(textD);
    
    /* If there is a (syntax highlighting) style table in use, find the new
       maximum font height for this text display */
    for (i=0; i<textD->nStyles; i++) {
    	styleFont = textD->styleTable[i].font;
	if (styleFont != NULL && styleFont->ascent > maxAscent)
    	    maxAscent = styleFont->ascent;
    	if (styleFont != NULL && styleFont->descent > maxDescent)
    	    maxDescent = styleFont->descent;
    }
    textD->ascent = maxAscent;
    textD->descent = maxDescent;
    
    /* If all of the current fonts are fixed and match in width, compute */
    fontWidth = fontStruct->max_bounds.width;
    if (fontWidth != fontStruct->min_bounds.width)
	fontWidth = -1;
    else {
	for (i=0; i<textD->nStyles; i++) {
    	    styleFont = textD->styleTable[i].font;
	    if (styleFont != NULL && (styleFont->max_bounds.width != fontWidth ||
		    styleFont->max_bounds.width != styleFont->min_bounds.width))
		fontWidth = -1;
	}
    }
    textD->fixedFontWidth = fontWidth;
    
    /* Don't let the height dip below one line, or bad things can happen */
    if (textD->height < maxAscent + maxDescent)
        textD->height = maxAscent + maxDescent;

    /* Change the font.  In most cases, this means re-allocating the
       affected GCs (they are shared with other widgets, and if the primary
       font changes, must be re-allocated to change it). Unfortunately,
       this requres recovering all of the colors from the existing GCs */
    textD->fontStruct = fontStruct;
    XGetGCValues(display, textD->gc, GCForeground|GCBackground, &values);
    fgPixel = values.foreground;
    bgPixel = values.background;
    XGetGCValues(display, textD->selectGC, GCForeground|GCBackground, &values);
    selectFGPixel = values.foreground;
    selectBGPixel = values.background;
    XGetGCValues(display, textD->highlightGC,GCForeground|GCBackground,&values);
    highlightFGPixel = values.foreground;
    highlightBGPixel = values.background;
    releaseGC(textD->w, textD->gc);
    releaseGC(textD->w, textD->selectGC);
    releaseGC(textD->w, textD->highlightGC);
    releaseGC(textD->w, textD->selectBGGC);
    releaseGC(textD->w, textD->highlightBGGC);
    if (textD->lineNumGC != NULL)
	releaseGC(textD->w, textD->lineNumGC);
    textD->lineNumGC = NULL;
    allocateFixedFontGCs(textD, fontStruct, bgPixel, fgPixel, selectFGPixel,
	    selectBGPixel, highlightFGPixel, highlightBGPixel);
    XSetFont(display, textD->styleGC, fontStruct->fid);
    
    /* Do a full resize to force recalculation of font related parameters */
    width = textD->width;
    height = textD->height;
    textD->width = textD->height = 0;
    TextDResize(textD, width, height);
    
    /* Redisplay */
    TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
    	    textD->height);
    
    /* Clean up line number area in case spacing has changed */
    draw_line_numbers(textD, True);
}

int TextDMinFontWidth(textDisp *textD, Boolean considerStyles) {
    int fontWidth = textD->fontStruct->max_bounds.width;
    int i;

    if (considerStyles) {
        for (i = 0; i < textD->nStyles; ++i) {
            int thisWidth = (textD->styleTable[i].font)->min_bounds.width;
            if (thisWidth < fontWidth) {
                fontWidth = thisWidth;
            }
        }
    }
    return(fontWidth);
}

int TextDMaxFontWidth(textDisp *textD, Boolean considerStyles) {
    int fontWidth = textD->fontStruct->max_bounds.width;
    int i;

    if (considerStyles) {
        for (i = 0; i < textD->nStyles; ++i) {
            int thisWidth = (textD->styleTable[i].font)->max_bounds.width;
            if (thisWidth > fontWidth) {
                fontWidth = thisWidth;
            }
        }
    }
    return(fontWidth);
}
#endif

int Fl_Text_Display::longest_vline() {
  int longest = 0;
  for (int i = 0; i < mNVisibleLines; i++)
    longest = max(longest, measure_vline(i));
  return longest;
}

/*
** Change the size of the displayed text area
*/
void Fl_Text_Display::resize(int X, int Y, int W, int H) {
#ifdef DEBUG
  printf("Fl_Text_Display::resize(X=%d, Y=%d, W=%d, H=%d)\n", X, Y, W, H);
#endif // DEBUG
  const int oldWidth = w();
#ifdef DEBUG
  printf("    oldWidth=%d, mContinuousWrap=%d, mWrapMargin=%d\n", oldWidth,
         mContinuousWrap, mWrapMargin);
#endif // DEBUG
  Fl_Widget::resize(X,Y,W,H);
  if (!buffer()) return;
  X += Fl::box_dx(box());
  Y += Fl::box_dy(box());
  W -= Fl::box_dw(box());
  H -= Fl::box_dh(box());

  text_area.x = X+LEFT_MARGIN;
  text_area.y = Y+BOTTOM_MARGIN;
  text_area.w = W-LEFT_MARGIN-RIGHT_MARGIN;
  text_area.h = H-TOP_MARGIN-BOTTOM_MARGIN;
  int i;

  /* Find the new maximum font height for this text display */
  for (i = 0, mMaxsize = fl_height(textfont(), textsize()); i < mNStyles; i++)
    mMaxsize = max(mMaxsize, fl_height(mStyleTable[i].font, mStyleTable[i].size));

  // did we have scrollbars initially?
  int hscrollbarvisible = mHScrollBar->visible();
  int vscrollbarvisible = mVScrollBar->visible();

  // try without scrollbars first
  mVScrollBar->clear_visible();
  mHScrollBar->clear_visible();

  for (int again = 1; again;) {
     again = 0;
    /* In continuous wrap mode, a change in width affects the total number of
       lines in the buffer, and can leave the top line number incorrect, and
       the top character no longer pointing at a valid line start */
    if (mContinuousWrap && !mWrapMargin && W!=oldWidth) {
      int oldFirstChar = mFirstChar;
      mNBufferLines = count_lines(0, buffer()->length(), true);
      mFirstChar = line_start(mFirstChar);
      mTopLineNum = count_lines(0, mFirstChar, true)+1;
      absolute_top_line_number(oldFirstChar);

#ifdef DEBUG
      printf("    mNBufferLines=%d\n", mNBufferLines);
#endif // DEBUG
    }
 
    /* reallocate and update the line starts array, which may have changed
       size and / or contents.  */
    int nvlines = (text_area.h + mMaxsize - 1) / mMaxsize;
    if (nvlines < 1) nvlines = 1;
    if (mNVisibleLines != nvlines) {
      mNVisibleLines = nvlines;
      if (mLineStarts) delete[] mLineStarts;
      mLineStarts = new int [mNVisibleLines];
    }

    calc_line_starts(0, mNVisibleLines);
    calc_last_char();

    // figure the scrollbars
    if (scrollbar_width()) {
      /* Decide if the vertical scroll bar needs to be visible */
      if (scrollbar_align() & (FL_ALIGN_LEFT|FL_ALIGN_RIGHT) &&
          mNBufferLines >= mNVisibleLines - 1)
      {
        mVScrollBar->set_visible();
        if (scrollbar_align() & FL_ALIGN_LEFT) {
          text_area.x = X+scrollbar_width()+LEFT_MARGIN;
          text_area.w = W-scrollbar_width()-LEFT_MARGIN-RIGHT_MARGIN;
          mVScrollBar->resize(X, text_area.y-TOP_MARGIN, scrollbar_width(),
                              text_area.h+TOP_MARGIN+BOTTOM_MARGIN);
        } else {
          text_area.x = X+LEFT_MARGIN;
          text_area.w = W-scrollbar_width()-LEFT_MARGIN-RIGHT_MARGIN;
          mVScrollBar->resize(X+W-scrollbar_width(), text_area.y-TOP_MARGIN,
                              scrollbar_width(), text_area.h+TOP_MARGIN+BOTTOM_MARGIN);
        }
      }

      /*
         Decide if the horizontal scroll bar needs to be visible.  If there
         is a vertical scrollbar, a horizontal is always created too.  This
         is because the alternatives are unatractive:
          * Dynamically creating a horizontal scrollbar based on the currently
            visible lines is what the original nedit does, but it always wastes
            space for the scrollbar even when it's not used.  Since the FLTK
            widget dynamically allocates the space for the scrollbar and
            rearranges the widget to make room for it, this would create a very
            visually displeasing "bounce" effect when the vertical scrollbar is
            dragged.  Trust me, I tried it and it looks really bad.
          * The other alternative would be to keep track of what the longest
            line in the entire buffer is and base the scrollbar on that.  I
            didn't do this because I didn't see any easy way to do that using
            the nedit code and this could involve a lengthy calculation for
            large buffers.  If an efficient and non-costly way of doing this
            can be found, this might be a way to go.
      */
      /* WAS: Suggestion: Try turning the horizontal scrollbar on when
	 you first see a line that is too wide in the window, but then
	 don't turn it off (ie mix both of your solutions). */
      if (scrollbar_align() & (FL_ALIGN_TOP|FL_ALIGN_BOTTOM) &&
          (mVScrollBar->visible() || longest_vline() > text_area.w))
      {
        if (!mHScrollBar->visible()) {
          mHScrollBar->set_visible();
          again = 1; // loop again to see if we now need vert. & recalc sizes
        }
        if (scrollbar_align() & FL_ALIGN_TOP) {
          text_area.y = Y + scrollbar_width()+TOP_MARGIN;
          text_area.h = H - scrollbar_width()-TOP_MARGIN-BOTTOM_MARGIN;
          mHScrollBar->resize(text_area.x-LEFT_MARGIN, Y,
                              text_area.w+LEFT_MARGIN+RIGHT_MARGIN, scrollbar_width());
        } else {
          text_area.y = Y+TOP_MARGIN;
          text_area.h = H - scrollbar_width()-TOP_MARGIN-BOTTOM_MARGIN;
          mHScrollBar->resize(text_area.x-LEFT_MARGIN, Y+H-scrollbar_width(),
                              text_area.w+LEFT_MARGIN+RIGHT_MARGIN, scrollbar_width());
        }
      }
    }
  }

  // user request to change viewport
  if (mTopLineNumHint != mTopLineNum || mHorizOffsetHint != mHorizOffset)
    scroll_(mTopLineNumHint, mHorizOffsetHint);

  // everything will fit in the viewport
  if (mNBufferLines < mNVisibleLines || mBuffer == NULL || mBuffer->length() == 0)
    scroll_(1, mHorizOffset);
  /* if empty lines become visible, there may be an opportunity to
     display more text by scrolling down */
  else while (mLineStarts[mNVisibleLines-2] == -1)
    scroll_(mTopLineNum-1, mHorizOffset);

  // user request to display insert position
  if (display_insert_position_hint)
    display_insert();

  // in case horizontal offset is now greater than longest line
  int maxhoffset = max(0, longest_vline()-text_area.w);
  if (mHorizOffset > maxhoffset)
    scroll_(mTopLineNumHint, maxhoffset);

  mTopLineNumHint = mTopLineNum;
  mHorizOffsetHint = mHorizOffset;
  display_insert_position_hint = 0;

  if (mContinuousWrap ||
      hscrollbarvisible != mHScrollBar->visible() ||
      vscrollbarvisible != mVScrollBar->visible())
    redraw();

  update_v_scrollbar();
  update_h_scrollbar();
}

/*
** Refresh a rectangle of the text display.  left and top are in coordinates of
** the text drawing window
*/
void Fl_Text_Display::draw_text( int left, int top, int width, int height ) {
  int fontHeight, firstLine, lastLine, line;

  /* find the line number range of the display */
  fontHeight = mMaxsize ? mMaxsize : textsize_;
  firstLine = ( top - text_area.y - fontHeight + 1 ) / fontHeight;
  lastLine = ( top + height - text_area.y ) / fontHeight + 1;

  fl_push_clip( left, top, width, height );

  /* draw the lines */
  for ( line = firstLine; line <= lastLine; line++ )
    draw_vline( line, left, left + width, 0, INT_MAX );

    /* draw the line numbers if exposed area includes them */
    if (mLineNumWidth != 0 && left <= mLineNumLeft + mLineNumWidth)
	draw_line_numbers(false);

  fl_pop_clip();
}

void Fl_Text_Display::redisplay_range(int startpos, int endpos) {
  if (damage_range1_start == -1 && damage_range1_end == -1) {
    damage_range1_start = startpos;
    damage_range1_end = endpos;
  } else if ((startpos >= damage_range1_start && startpos <= damage_range1_end) ||
             (endpos >= damage_range1_start && endpos <= damage_range1_end)) {
    damage_range1_start = min(damage_range1_start, startpos);
    damage_range1_end = max(damage_range1_end, endpos);
  } else if (damage_range2_start == -1 && damage_range2_end == -1) {
    damage_range2_start = startpos;
    damage_range2_end = endpos;
  } else {
    damage_range2_start = min(damage_range2_start, startpos);
    damage_range2_end = max(damage_range2_end, endpos);
  }
  damage(FL_DAMAGE_SCROLL);
}
/*
** Refresh all of the text between buffer positions "start" and "end"
** not including the character at the position "end".
** If end points beyond the end of the buffer, refresh the whole display
** after pos, including blank lines which are not technically part of
** any range of characters.
*/
void Fl_Text_Display::draw_range(int startpos, int endpos) {
  int i, startLine, lastLine, startIndex, endIndex;

  /* If the range is outside of the displayed text, just return */
  if ( endpos < mFirstChar || ( startpos > mLastChar &&
       !empty_vlines() ) ) return;

  /* Clean up the starting and ending values */
  if ( startpos < 0 ) startpos = 0;
  if ( startpos > mBuffer->length() ) startpos = mBuffer->length();
  if ( endpos < 0 ) endpos = 0;
  if ( endpos > mBuffer->length() ) endpos = mBuffer->length();

  /* Get the starting and ending lines */
  if ( startpos < mFirstChar )
    startpos = mFirstChar;
  if ( !position_to_line( startpos, &startLine ) )
    startLine = mNVisibleLines - 1;
  if ( endpos >= mLastChar ) {
    lastLine = mNVisibleLines - 1;
  } else {
    if ( !position_to_line( endpos, &lastLine ) ) {
      /* shouldn't happen */
      lastLine = mNVisibleLines - 1;
    }
  }

  /* Get the starting and ending positions within the lines */
  startIndex = mLineStarts[ startLine ] == -1 ? 0 :
               startpos - mLineStarts[ startLine ];
  if ( endpos >= mLastChar )
    endIndex = INT_MAX;
  else if ( mLineStarts[ lastLine ] == -1 )
    endIndex = 0;
  else
    endIndex = endpos - mLineStarts[ lastLine ];

  /* If the starting and ending lines are the same, redisplay the single
     line between "start" and "end" */
  if ( startLine == lastLine ) {
    draw_vline( startLine, 0, INT_MAX, startIndex, endIndex );
    return;
  }

  /* Redisplay the first line from "start" */
  draw_vline( startLine, 0, INT_MAX, startIndex, INT_MAX );

  /* Redisplay the lines in between at their full width */
  for ( i = startLine + 1; i < lastLine; i++ )
    draw_vline( i, 0, INT_MAX, 0, INT_MAX );

  /* Redisplay the last line to "end" */
  draw_vline( lastLine, 0, INT_MAX, 0, endIndex );
}

/*
** Set the position of the text insertion cursor for text display
*/
void Fl_Text_Display::insert_position( int newPos ) {
  /* make sure new position is ok, do nothing if it hasn't changed */
  if ( newPos == mCursorPos )
    return;
  if ( newPos < 0 ) newPos = 0;
  if ( newPos > mBuffer->length() ) newPos = mBuffer->length();

  /* cursor movement cancels vertical cursor motion column */
  mCursorPreferredCol = -1;

  /* erase the cursor at it's previous position */
  redisplay_range(mCursorPos - 1, mCursorPos + 1);

  mCursorPos = newPos;

  /* draw cursor at its new position */
  redisplay_range(mCursorPos - 1, mCursorPos + 1);
}

void Fl_Text_Display::show_cursor(int b) {
  mCursorOn = b;
  redisplay_range(mCursorPos - 1, mCursorPos + 1);
}

void Fl_Text_Display::cursor_style(int style) {
  mCursorStyle = style;
  if (mCursorOn) show_cursor();
}

void Fl_Text_Display::wrap_mode(int wrap, int wrapMargin) {
  mWrapMargin = wrapMargin;
  mContinuousWrap = wrap;

  if (buffer()) {
    /* wrapping can change the total number of lines, re-count */
    mNBufferLines = count_lines(0, buffer()->length(), true);

    /* changing wrap margins or changing from wrapped mode to non-wrapped
       can leave the character at the top no longer at a line start, and/or
       change the line number */
    mFirstChar = line_start(mFirstChar);
    mTopLineNum = count_lines(0, mFirstChar, true) + 1;

    reset_absolute_top_line_number();

    /* update the line starts array */
    calc_line_starts(0, mNVisibleLines);
    calc_last_char();
  } else {
    // No buffer, so just clear the state info for later...
    mNBufferLines  = 0;
    mFirstChar     = 0;
    mTopLineNum    = 1;
    mAbsTopLineNum = 0;
  }

  resize(x(), y(), w(), h());
}

/*
** Insert "text" at the current cursor location.  This has the same
** effect as inserting the text into the buffer using BufInsert and
** then moving the insert position after the newly inserted text, except
** that it's optimized to do less redrawing.
*/
void Fl_Text_Display::insert(const char* text) {
  int pos = mCursorPos;

  mCursorToHint = pos + strlen( text );
  mBuffer->insert( pos, text );
  mCursorToHint = NO_HINT;
}

/*
** Insert "text" (which must not contain newlines), overstriking the current
** cursor location.
*/
void Fl_Text_Display::overstrike(const char* text) {
  int startPos = mCursorPos;
  Fl_Text_Buffer *buf = mBuffer;
  int lineStart = buf->line_start( startPos );
  int textLen = strlen( text );
  int i, p, endPos, indent, startIndent, endIndent;
  const char *c;
  char ch, *paddedText = NULL;

  /* determine how many displayed character positions are covered */
  startIndent = mBuffer->count_displayed_characters( lineStart, startPos );
  indent = startIndent;
  for ( c = text; *c != '\0'; c++ )
    indent += Fl_Text_Buffer::character_width( *c, indent, buf->tab_distance(), buf->null_substitution_character() );
  endIndent = indent;

  /* find which characters to remove, and if necessary generate additional
     padding to make up for removed control characters at the end */
  indent = startIndent;
  for ( p = startPos; ; p++ ) {
    if ( p == buf->length() )
      break;
    ch = buf->character( p );
    if ( ch == '\n' )
      break;
    indent += Fl_Text_Buffer::character_width( ch, indent, buf->tab_distance(), buf->null_substitution_character() );
    if ( indent == endIndent ) {
      p++;
      break;
    } else if ( indent > endIndent ) {
      if ( ch != '\t' ) {
        p++;
        paddedText = new char [ textLen + FL_TEXT_MAX_EXP_CHAR_LEN + 1 ];
        strcpy( paddedText, text );
        for ( i = 0; i < indent - endIndent; i++ )
          paddedText[ textLen + i ] = ' ';
        paddedText[ textLen + i ] = '\0';
      }
      break;
    }
  }
  endPos = p;

  mCursorToHint = startPos + textLen;
  buf->replace( startPos, endPos, paddedText == NULL ? text : paddedText );
  mCursorToHint = NO_HINT;
  if ( paddedText != NULL )
    delete [] paddedText;
}

/*
** Translate a buffer text position to the XY location where the top left
** of the cursor would be positioned to point to that character.  Returns
** 0 if the position is not displayed because it is VERTICALLY out
** of view.  If the position is horizontally out of view, returns the
** X coordinate where the position would be if it were visible.
*/

int Fl_Text_Display::position_to_xy( int pos, int* X, int* Y ) {
  int charIndex, lineStartPos, fontHeight, lineLen;
  int visLineNum, charLen, outIndex, xStep, charStyle;
  char expandedChar[ FL_TEXT_MAX_EXP_CHAR_LEN ];
  const char *lineStr;

//  printf("position_to_xy(pos=%d, X=%p, Y=%p)\n", pos, X, Y);

  /* If position is not displayed, return false */
  if (pos < mFirstChar || (pos > mLastChar && !empty_vlines())) {
//    printf("    returning 0\n"
//           "    mFirstChar=%d, mLastChar=%d, empty_vlines()=0\n",
//	   mFirstChar, mLastChar);
    return 0;
  }

  /* Calculate Y coordinate */
  if (!position_to_line(pos, &visLineNum)) {
//    puts("    returning 0\n"
//         "    position_to_line()=0");
    return 0;
  }

  if (visLineNum < 0 || visLineNum > mNBufferLines) {
//    printf("    returning 0\n"
//           "    visLineNum=%d, mNBufferLines=%d\n",
//	   visLineNum, mNBufferLines);
    return 0;
  }

  fontHeight = mMaxsize;
  *Y = text_area.y + visLineNum * fontHeight;

  /* Get the text, length, and  buffer position of the line. If the position
     is beyond the end of the buffer and should be at the first position on
     the first empty line, don't try to get or scan the text  */
  lineStartPos = mLineStarts[visLineNum];
  if ( lineStartPos == -1 ) {
    *X = text_area.x - mHorizOffset;
    return 1;
  }
  lineLen = vline_length( visLineNum );
  lineStr = mBuffer->text_range( lineStartPos, lineStartPos + lineLen );

  /* Step through character positions from the beginning of the line
     to "pos" to calculate the X coordinate */
  xStep = text_area.x - mHorizOffset;
  outIndex = 0;
  for ( charIndex = 0; charIndex < lineLen && charIndex < pos - lineStartPos; charIndex++ ) {
    charLen = Fl_Text_Buffer::expand_character( lineStr[ charIndex ], outIndex, expandedChar,
              mBuffer->tab_distance(), mBuffer->null_substitution_character() );
    charStyle = position_style( lineStartPos, lineLen, charIndex,
                                outIndex );
    xStep += string_width( expandedChar, charLen, charStyle );
    outIndex += charLen;
  }
  *X = xStep;
  free((char *)lineStr);
  return 1;
}

/*
** Find the line number of position "pos".  Note: this only works for
** displayed lines.  If the line is not displayed, the function returns
** 0 (without the mLineStarts array it could turn in to very long
** calculation involving scanning large amounts of text in the buffer).
** If continuous wrap mode is on, returns the absolute line number (as opposed
** to the wrapped line number which is used for scrolling).
*/
int Fl_Text_Display::position_to_linecol( int pos, int* lineNum, int* column ) {
  int retVal;
    
    /* In continuous wrap mode, the absolute (non-wrapped) line count is
       maintained separately, as needed.  Only return it if we're actually
       keeping track of it and pos is in the displayed text */
    if (mContinuousWrap) {
	if (!maintaining_absolute_top_line_number() ||
       pos < mFirstChar || pos > mLastChar)
	    return 0;
	*lineNum = mAbsTopLineNum + buffer()->count_lines(mFirstChar, pos);
	*column
     = buffer()->count_displayed_characters(buffer()->line_start(pos), pos);
	return 1;
    }

  retVal = position_to_line( pos, lineNum );
  if ( retVal ) {
    *column = mBuffer->count_displayed_characters(
                mLineStarts[ *lineNum ], pos );
    *lineNum += mTopLineNum;
  }
  return retVal;
}

/*
** Return 1 if position (X, Y) is inside of the primary Fl_Text_Selection
*/
int Fl_Text_Display::in_selection( int X, int Y ) {
  int row, column, pos = xy_to_position( X, Y, CHARACTER_POS );
  Fl_Text_Buffer *buf = mBuffer;

  xy_to_rowcol( X, Y, &row, &column, CHARACTER_POS );
  if (range_touches_selection(buf->primary_selection(), mFirstChar, mLastChar))
    column = wrapped_column(row, column);
  return buf->primary_selection()->includes(pos, buf->line_start( pos ), column);
}

/*
** Correct a column number based on an unconstrained position (as returned by
** TextDXYToUnconstrainedPosition) to be relative to the last actual newline
** in the buffer before the row and column position given, rather than the
** last line start created by line wrapping.  This is an adapter
** for rectangular selections and code written before continuous wrap mode,
** which thinks that the unconstrained column is the number of characters
** from the last newline.  Obviously this is time consuming, because it
** invloves character re-counting.
*/
int Fl_Text_Display::wrapped_column(int row, int column) {
    int lineStart, dispLineStart;
    
    if (!mContinuousWrap || row < 0 || row > mNVisibleLines)
    	return column;
    dispLineStart = mLineStarts[row];
    if (dispLineStart == -1)
    	return column;
    lineStart = buffer()->line_start(dispLineStart);
    return column
		 + buffer()->count_displayed_characters(lineStart, dispLineStart);
}

/*
** Correct a row number from an unconstrained position (as returned by
** TextDXYToUnconstrainedPosition) to a straight number of newlines from the
** top line of the display.  Because rectangular selections are based on
** newlines, rather than display wrapping, and anywhere a rectangular selection
** needs a row, it needs it in terms of un-wrapped lines.
*/
int Fl_Text_Display::wrapped_row(int row) {
    if (!mContinuousWrap || row < 0 || row > mNVisibleLines)
    	return row;
    return buffer()->count_lines(mFirstChar, mLineStarts[row]);
}

/*
** Scroll the display to bring insertion cursor into view.
**
** Note: it would be nice to be able to do this without counting lines twice
** (scroll_() counts them too) and/or to count from the most efficient
** starting point, but the efficiency of this routine is not as important to
** the overall performance of the text display.
*/
void Fl_Text_Display::display_insert() {
  int hOffset, topLine, X, Y;
  hOffset = mHorizOffset;
  topLine = mTopLineNum;

//	FIXME: I don't understand this well enough to know if it is correct
//	       it is different than nedit 5.3
  if (insert_position() < mFirstChar) {
    topLine -= count_lines(insert_position(), mFirstChar, false);
  } else if (mLineStarts[mNVisibleLines-2] != -1) {
    int lastChar = line_end(mLineStarts[mNVisibleLines-2],true);
    if (insert_position() >= lastChar)
      topLine
        += count_lines(lastChar - (wrap_uses_character(mLastChar) ? 0 : 1),
                        insert_position(), false);
  }

  /* Find the new setting for horizontal offset (this is a bit ungraceful).
     If the line is visible, just use PositionToXY to get the position
     to scroll to, otherwise, do the vertical scrolling first, then the
     horizontal */
  if (!position_to_xy( mCursorPos, &X, &Y )) {
    scroll_(topLine, hOffset);
    if (!position_to_xy( mCursorPos, &X, &Y )) {
      #ifdef DEBUG
      printf ("*** display_insert/position_to_xy # GIVE UP !\n"); fflush(stdout);
      #endif // DEBUG
      return;   /* Give up, it's not worth it (but why does it fail?) */
    }
  }
  if (X > text_area.x + text_area.w)
    hOffset += X-(text_area.x + text_area.w);
  else if (X < text_area.x)
    hOffset += X-text_area.x;

  /* Do the scroll */
  if (topLine != mTopLineNum || hOffset != mHorizOffset)
    scroll_(topLine, hOffset);
}

void Fl_Text_Display::show_insert_position() {
  display_insert_position_hint = 1;
  resize(x(), y(), w(), h());
}

/*
** Cursor movement functions
*/
int Fl_Text_Display::move_right() {
  if ( mCursorPos >= mBuffer->length() )
    return 0;
  insert_position( mCursorPos + 1 );
  return 1;
}

int Fl_Text_Display::move_left() {
  if ( mCursorPos <= 0 )
    return 0;
  insert_position( mCursorPos - 1 );
  return 1;
}

int Fl_Text_Display::move_up() {
  int lineStartPos, column, prevLineStartPos, newPos, visLineNum;

  /* Find the position of the start of the line.  Use the line starts array
     if possible */
  if ( position_to_line( mCursorPos, &visLineNum ) )
    lineStartPos = mLineStarts[ visLineNum ];
  else {
    lineStartPos = line_start( mCursorPos );
    visLineNum = -1;
  }
  if ( lineStartPos == 0 )
    return 0;

  /* Decide what column to move to, if there's a preferred column use that */
  column = mCursorPreferredCol >= 0 ? mCursorPreferredCol :
           mBuffer->count_displayed_characters( lineStartPos, mCursorPos );

  /* count forward from the start of the previous line to reach the column */
  if ( visLineNum != -1 && visLineNum != 0 )
    prevLineStartPos = mLineStarts[ visLineNum - 1 ];
  else
    prevLineStartPos = rewind_lines( lineStartPos, 1 );
  newPos = mBuffer->skip_displayed_characters( prevLineStartPos, column );
  if (mContinuousWrap)
      newPos = min(newPos, line_end(prevLineStartPos, true));

  /* move the cursor */
  insert_position( newPos );

  /* if a preferred column wasn't aleady established, establish it */
  mCursorPreferredCol = column;
  return 1;
}

int Fl_Text_Display::move_down() {
  int lineStartPos, column, nextLineStartPos, newPos, visLineNum;

  if ( mCursorPos == mBuffer->length() )
    return 0;
  if ( position_to_line( mCursorPos, &visLineNum ) )
    lineStartPos = mLineStarts[ visLineNum ];
  else {
    lineStartPos = line_start( mCursorPos );
    visLineNum = -1;
  }
  column = mCursorPreferredCol >= 0 ? mCursorPreferredCol :
           mBuffer->count_displayed_characters( lineStartPos, mCursorPos );
  nextLineStartPos = skip_lines( lineStartPos, 1, true );
  newPos = mBuffer->skip_displayed_characters( nextLineStartPos, column );
    if (mContinuousWrap)
    	newPos = min(newPos, line_end(nextLineStartPos, true));

  insert_position( newPos );
  mCursorPreferredCol = column;
  return 1;
}

/*
** Same as BufCountLines, but takes in to account wrapping if wrapping is
** turned on.  If the caller knows that startPos is at a line start, it
** can pass "startPosIsLineStart" as True to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
*/
int Fl_Text_Display::count_lines(int startPos, int endPos,
    	bool startPosIsLineStart) {
    int retLines, retPos, retLineStart, retLineEnd;

#ifdef DEBUG
    printf("Fl_Text_Display::count_lines(startPos=%d, endPos=%d, startPosIsLineStart=%d\n",
           startPos, endPos, startPosIsLineStart);
#endif // DEBUG

    /* If we're not wrapping use simple (and more efficient) BufCountLines */
    if (!mContinuousWrap)
    	return buffer()->count_lines(startPos, endPos);
    
    wrapped_line_counter(buffer(), startPos, endPos, INT_MAX,
	    startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
	    &retLineEnd);

#ifdef DEBUG
    printf("   # after WLC: retPos=%d, retLines=%d, retLineStart=%d, retLineEnd=%d\n",
           retPos, retLines, retLineStart, retLineEnd);
#endif // DEBUG

    return retLines;
}

/*
** Same as BufCountForwardNLines, but takes in to account line breaks when
** wrapping is turned on. If the caller knows that startPos is at a line start,
** it can pass "startPosIsLineStart" as True to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
*/
int Fl_Text_Display::skip_lines(int startPos, int nLines,
    bool startPosIsLineStart) {
    int retLines, retPos, retLineStart, retLineEnd;
    
    /* if we're not wrapping use more efficient BufCountForwardNLines */
    if (!mContinuousWrap)
    	return buffer()->skip_lines(startPos, nLines);
    
    /* wrappedLineCounter can't handle the 0 lines case */
    if (nLines == 0)
    	return startPos;
    
    /* use the common line counting routine to count forward */
    wrapped_line_counter(buffer(), startPos, buffer()->length(),
    	    nLines, startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
    	    &retLineEnd);
    return retPos;
}

/*
** Same as BufEndOfLine, but takes in to account line breaks when wrapping
** is turned on.  If the caller knows that startPos is at a line start, it
** can pass "startPosIsLineStart" as True to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
**
** Note that the definition of the end of a line is less clear when continuous
** wrap is on.  With continuous wrap off, it's just a pointer to the newline
** that ends the line.  When it's on, it's the character beyond the last
** DISPLAYABLE character on the line, where a whitespace character which has
** been "converted" to a newline for wrapping is not considered displayable.
** Also note that, a line can be wrapped at a non-whitespace character if the
** line had no whitespace.  In this case, this routine returns a pointer to
** the start of the next line.  This is also consistent with the model used by
** visLineLength.
*/
int Fl_Text_Display::line_end(int pos, bool startPosIsLineStart) {
    int retLines, retPos, retLineStart, retLineEnd;
    
    /* If we're not wrapping use more efficien BufEndOfLine */
    if (!mContinuousWrap)
    	return buffer()->line_end(pos);
    
    if (pos == buffer()->length())
    	return pos;
    wrapped_line_counter(buffer(), pos, buffer()->length(), 1,
    	    startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
	    &retLineEnd);
    return retLineEnd;
}

/*
** Same as BufStartOfLine, but returns the character after last wrap point
** rather than the last newline.
*/
int Fl_Text_Display::line_start(int pos) {
    int retLines, retPos, retLineStart, retLineEnd;
    
    /* If we're not wrapping, use the more efficient BufStartOfLine */
    if (!mContinuousWrap)
    	return buffer()->line_start(pos);

    wrapped_line_counter(buffer(), buffer()->line_start(pos), pos, INT_MAX, true, 0,
			 &retPos, &retLines, &retLineStart, &retLineEnd);
    return retLineStart;
}

/*
** Same as BufCountBackwardNLines, but takes in to account line breaks when
** wrapping is turned on.
*/
int Fl_Text_Display::rewind_lines(int startPos, int nLines) {
    Fl_Text_Buffer *buf = buffer();
    int pos, lineStart, retLines, retPos, retLineStart, retLineEnd;
    
    /* If we're not wrapping, use the more efficient BufCountBackwardNLines */
    if (!mContinuousWrap)
    	return buf->rewind_lines(startPos, nLines);

    pos = startPos;
    for (;;) {
	lineStart = buf->line_start(pos);
	wrapped_line_counter(buf, lineStart, pos, INT_MAX,
	    	true, 0, &retPos, &retLines, &retLineStart, &retLineEnd, false);
	if (retLines > nLines)
    	    return skip_lines(lineStart, retLines-nLines,
    	    	    true);
    	nLines -= retLines;
    	pos = lineStart - 1;
    	if (pos < 0)
    	    return 0;
    	nLines -= 1;
    }
}

static inline int fl_isseparator(int c) {
  return c != '$' && c != '_' && (isspace(c) || ispunct(c));
}

void Fl_Text_Display::next_word() {
  int pos = insert_position();
  while (pos < buffer()->length() && !fl_isseparator(buffer()->character(pos))) {
    pos++;
  }
  while (pos < buffer()->length() && fl_isseparator(buffer()->character(pos))) {
    pos++;
  }

  insert_position( pos );
}

void Fl_Text_Display::previous_word() {
  int pos = insert_position();
  if (pos==0) return;
  pos--;
  while (pos && fl_isseparator(buffer()->character(pos))) {
    pos--;
  }
  while (pos && !fl_isseparator(buffer()->character(pos))) {
    pos--;
  }
  if (fl_isseparator(buffer()->character(pos))) pos++;

  insert_position( pos );
}

/*
** Callback attached to the text buffer to receive delete information before
** the modifications are actually made.
*/
void Fl_Text_Display::buffer_predelete_cb(int pos, int nDeleted, void *cbArg) {
    Fl_Text_Display *textD = (Fl_Text_Display *)cbArg;
    if (textD->mContinuousWrap && 
        (textD->mFixedFontWidth == -1 || textD->mModifyingTabDistance))
	/* Note: we must perform this measurement, even if there is not a
	   single character deleted; the number of "deleted" lines is the
	   number of visual lines spanned by the real line in which the 
	   modification takes place. 
	   Also, a modification of the tab distance requires the same
	   kind of calculations in advance, even if the font width is "fixed",
	   because when the width of the tab characters changes, the layout 
	   of the text may be completely different. */
	textD->measure_deleted_lines(pos, nDeleted);
    else
	textD->mSuppressResync = 0; /* Probably not needed, but just in case */
}

/*
** Callback attached to the text buffer to receive modification information
*/
void Fl_Text_Display::buffer_modified_cb( int pos, int nInserted, int nDeleted,
    int nRestyled, const char *deletedText, void *cbArg ) {
  int linesInserted, linesDeleted, startDispPos, endDispPos;
  Fl_Text_Display *textD = ( Fl_Text_Display * ) cbArg;
  Fl_Text_Buffer *buf = textD->mBuffer;
  int oldFirstChar = textD->mFirstChar;
  int scrolled, origCursorPos = textD->mCursorPos;
  int wrapModStart, wrapModEnd;

  /* buffer modification cancels vertical cursor motion column */
  if ( nInserted != 0 || nDeleted != 0 )
    textD->mCursorPreferredCol = -1;

    /* Count the number of lines inserted and deleted, and in the case
       of continuous wrap mode, how much has changed */
    if (textD->mContinuousWrap) {
    	textD->find_wrap_range(deletedText, pos, nInserted, nDeleted,
    	    	&wrapModStart, &wrapModEnd, &linesInserted, &linesDeleted);
    } else {
   linesInserted = nInserted == 0 ? 0 :
                  buf->count_lines( pos, pos + nInserted );
   linesDeleted = nDeleted == 0 ? 0 : countlines( deletedText );
    }

  /* Update the line starts and mTopLineNum */
  if ( nInserted != 0 || nDeleted != 0 ) {
   if (textD->mContinuousWrap) {
     textD->update_line_starts( wrapModStart, wrapModEnd-wrapModStart,
                     nDeleted + pos-wrapModStart + (wrapModEnd-(pos+nInserted)),
                     linesInserted, linesDeleted, &scrolled );
   } else {
     textD->update_line_starts( pos, nInserted, nDeleted, linesInserted,
                               linesDeleted, &scrolled );
   }
  } else
    scrolled = 0;

    /* If we're counting non-wrapped lines as well, maintain the absolute
       (non-wrapped) line number of the text displayed */
    if (textD->maintaining_absolute_top_line_number() &&
        (nInserted != 0 || nDeleted != 0)) {
	if (pos + nDeleted < oldFirstChar)
	    textD->mAbsTopLineNum += buf->count_lines(pos, pos + nInserted) -
		    countlines(deletedText);
	else if (pos < oldFirstChar)
	    textD->reset_absolute_top_line_number();
    }    	    

  /* Update the line count for the whole buffer */
  textD->mNBufferLines += linesInserted - linesDeleted;

  /* Update the cursor position */
  if ( textD->mCursorToHint != NO_HINT ) {
    textD->mCursorPos = textD->mCursorToHint;
    textD->mCursorToHint = NO_HINT;
  } else if ( textD->mCursorPos > pos ) {
    if ( textD->mCursorPos < pos + nDeleted )
      textD->mCursorPos = pos;
    else
      textD->mCursorPos += nInserted - nDeleted;
  }

  // refigure scrollbars & stuff
  textD->resize(textD->x(), textD->y(), textD->w(), textD->h());

  // don't need to do anything else if not visible?
  if (!textD->visible_r()) return;

  /* If the changes caused scrolling, re-paint everything and we're done. */
  if ( scrolled ) {
    textD->damage(FL_DAMAGE_EXPOSE);
    if ( textD->mStyleBuffer )   /* See comments in extendRangeForStyleMods */
      textD->mStyleBuffer->primary_selection()->selected(0);
    return;
  }

  /* If the changes didn't cause scrolling, decide the range of characters
     that need to be re-painted.  Also if the cursor position moved, be
     sure that the redisplay range covers the old cursor position so the
     old cursor gets erased, and erase the bits of the cursor which extend
     beyond the left and right edges of the text. */
  startDispPos = textD->mContinuousWrap ? wrapModStart : pos;
  if ( origCursorPos == startDispPos && textD->mCursorPos != startDispPos )
    startDispPos = min( startDispPos, origCursorPos - 1 );
  if ( linesInserted == linesDeleted ) {
    if ( nInserted == 0 && nDeleted == 0 )
      endDispPos = pos + nRestyled;
    else {
      endDispPos = textD->mContinuousWrap ? wrapModEnd :
            buf->line_end( pos + nInserted ) + 1;
      // CET - FIXME      if ( origCursorPos >= startDispPos &&
      //                ( origCursorPos <= endDispPos || endDispPos == buf->length() ) )
    }

	if (linesInserted > 1) textD->draw_line_numbers(false);
  } else {
    endDispPos = textD->mLastChar + 1;
    // CET - FIXME   if ( origCursorPos >= pos )
        /* If more than one line is inserted/deleted, a line break may have
           been inserted or removed in between, and the line numbers may
           have changed. If only one line is altered, line numbers cannot
           be affected (the insertion or removal of a line break always 
           results in at least two lines being redrawn). */
	textD->draw_line_numbers(false);
  }

  /* If there is a style buffer, check if the modification caused additional
     changes that need to be redisplayed.  (Redisplaying separately would
     cause double-redraw on almost every modification involving styled
     text).  Extend the redraw range to incorporate style changes */
  if ( textD->mStyleBuffer )
    textD->extend_range_for_styles( &startDispPos, &endDispPos );

  /* Redisplay computed range */
  textD->redisplay_range( startDispPos, endDispPos );
}

/*
** In continuous wrap mode, internal line numbers are calculated after
** wrapping.  A separate non-wrapped line count is maintained when line
** numbering is turned on.  There is some performance cost to maintaining this
** line count, so normally absolute line numbers are not tracked if line
** numbering is off.  This routine allows callers to specify that they still
** want this line count maintained (for use via TextDPosToLineAndCol).
** More specifically, this allows the line number reported in the statistics
** line to be calibrated in absolute lines, rather than post-wrapped lines.
*/
void Fl_Text_Display::maintain_absolute_top_line_number(int state) {
    mNeedAbsTopLineNum = state;
    reset_absolute_top_line_number();
}

/*
** Returns the absolute (non-wrapped) line number of the first line displayed.
** Returns 0 if the absolute top line number is not being maintained.
*/
int Fl_Text_Display::get_absolute_top_line_number() {
    if (!mContinuousWrap)
	return mTopLineNum;
    if (maintaining_absolute_top_line_number())
	return mAbsTopLineNum;
    return 0;
}

/*
** Re-calculate absolute top line number for a change in scroll position.
*/
void Fl_Text_Display::absolute_top_line_number(int oldFirstChar) {
    if (maintaining_absolute_top_line_number()) {
	if (mFirstChar < oldFirstChar)
	    mAbsTopLineNum -= buffer()->count_lines(mFirstChar, oldFirstChar);
	else
	    mAbsTopLineNum += buffer()->count_lines(oldFirstChar, mFirstChar);
    }
}

/*
** Return true if a separate absolute top line number is being maintained
** (for displaying line numbers or showing in the statistics line).
*/
int Fl_Text_Display::maintaining_absolute_top_line_number() {
    return mContinuousWrap &&
	    (mLineNumWidth != 0 || mNeedAbsTopLineNum);
}

/*
** Count lines from the beginning of the buffer to reestablish the
** absolute (non-wrapped) top line number.  If mode is not continuous wrap,
** or the number is not being maintained, does nothing.
*/
void Fl_Text_Display::reset_absolute_top_line_number() {
    mAbsTopLineNum = 1;
    absolute_top_line_number(0);
}

/*
** Find the line number of position "pos" relative to the first line of
** displayed text. Returns 0 if the line is not displayed.
*/
int Fl_Text_Display::position_to_line( int pos, int *lineNum ) {
  int i;

  *lineNum = 0;
  if ( pos < mFirstChar ) return 0;
  if ( pos > mLastChar ) {
    if ( empty_vlines() ) {
      if ( mLastChar < mBuffer->length() ) {
        if ( !position_to_line( mLastChar, lineNum ) ) {
          Fl::error("Fl_Text_Display::position_to_line(): Consistency check ptvl failed");
          return 0;
        }
        return ++( *lineNum ) <= mNVisibleLines - 1;
      } else {
        position_to_line( mLastChar - 1, lineNum );
        return 1;
      }
    }
    return 0;
  }

  for ( i = mNVisibleLines - 1; i >= 0; i-- ) {
    if ( mLineStarts[ i ] != -1 && pos >= mLineStarts[ i ] ) {
      *lineNum = i;
      return 1;
    }
  }
  return 0;   /* probably never be reached */
}

/*
** Draw the text on a single line represented by "visLineNum" (the
** number of lines down from the top of the display), limited by
** "leftClip" and "rightClip" window coordinates and "leftCharIndex" and
** "rightCharIndex" character positions (not including the character at
** position "rightCharIndex").
*/
void Fl_Text_Display::draw_vline(int visLineNum, int leftClip, int rightClip,
                                 int leftCharIndex, int rightCharIndex) {
  Fl_Text_Buffer * buf = mBuffer;
  int i, X, Y, startX, charIndex, lineStartPos, lineLen, fontHeight;
  int stdCharWidth, charWidth, startIndex, charStyle, style;
  int charLen, outStartIndex, outIndex;
  int dispIndexOffset;
  char expandedChar[ FL_TEXT_MAX_EXP_CHAR_LEN ], outStr[ MAX_DISP_LINE_LEN ];
  char *outPtr;
  const char *lineStr;

//  printf("draw_vline(visLineNum=%d, leftClip=%d, rightClip=%d, leftCharIndex=%d, rightCharIndex=%d)\n",
//         visLineNum, leftClip, rightClip, leftCharIndex, rightCharIndex);
//  printf("nNVisibleLines=%d\n", mNVisibleLines);

  /* If line is not displayed, skip it */
  if ( visLineNum < 0 || visLineNum >= mNVisibleLines )
    return;

  /* Calculate Y coordinate of the string to draw */
  fontHeight = mMaxsize;
  Y = text_area.y + visLineNum * fontHeight;

  /* Get the text, length, and  buffer position of the line to display */
  lineStartPos = mLineStarts[ visLineNum ];
//  printf("lineStartPos=%d\n", lineStartPos);
  if ( lineStartPos == -1 ) {
    lineLen = 0;
    lineStr = NULL;
  } else {
    lineLen = vline_length( visLineNum );
    lineStr = buf->text_range( lineStartPos, lineStartPos + lineLen );
  }

  /* Space beyond the end of the line is still counted in units of characters
     of a standardized character width (this is done mostly because style
     changes based on character position can still occur in this region due
	  to rectangular Fl_Text_Selections).  stdCharWidth must be non-zero to
     prevent a potential infinite loop if X does not advance */
  stdCharWidth = TMPFONTWIDTH;   //mFontStruct->max_bounds.width;
  if ( stdCharWidth <= 0 ) {
    Fl::error("Fl_Text_Display::draw_vline(): bad font measurement");
    free((void *)lineStr);
    return;
  }

  /* Shrink the clipping range to the active display area */
  leftClip = max( text_area.x, leftClip );
  rightClip = min( rightClip, text_area.x + text_area.w );

  /* Rectangular Fl_Text_Selections are based on "real" line starts (after
	  a newline or start of buffer).  Calculate the difference between the
	  last newline position and the line start we're using.  Since scanning
	  back to find a newline is expensive, only do so if there's actually a
	  rectangular Fl_Text_Selection which needs it */
    if (mContinuousWrap && (range_touches_selection(buf->primary_selection(),
    	    lineStartPos, lineStartPos + lineLen) || range_touches_selection(
    	    buf->secondary_selection(), lineStartPos, lineStartPos + lineLen) ||
    	    range_touches_selection(buf->highlight_selection(), lineStartPos,
    	    lineStartPos + lineLen))) {
    	dispIndexOffset = buf->count_displayed_characters(
    	    	buf->line_start(lineStartPos), lineStartPos);
    } else
    	dispIndexOffset = 0;

  /* Step through character positions from the beginning of the line (even if
     that's off the left edge of the displayed area) to find the first
     character position that's not clipped, and the X coordinate for drawing
     that character */
  X = text_area.x - mHorizOffset;
  outIndex = 0;
  for ( charIndex = 0; ; charIndex++ ) {
    charLen = charIndex >= lineLen ? 1 :
              Fl_Text_Buffer::expand_character( lineStr[ charIndex ], outIndex,
                                                expandedChar, buf->tab_distance(), buf->null_substitution_character() );
    style = position_style( lineStartPos, lineLen, charIndex,
                            outIndex + dispIndexOffset );
    charWidth = charIndex >= lineLen ? stdCharWidth :
                string_width( expandedChar, charLen, style );
    if ( X + charWidth >= leftClip && charIndex >= leftCharIndex ) {
      startIndex = charIndex;
      outStartIndex = outIndex;
      startX = X;
      break;
    }
    X += charWidth;
    outIndex += charLen;
  }

  /* Scan character positions from the beginning of the clipping range, and
     draw parts whenever the style changes (also note if the cursor is on
     this line, and where it should be drawn to take advantage of the x
     position which we've gone to so much trouble to calculate) */
  /* since characters between style may overlap, we draw the full 
     background first */
  int sX = startX;
  outPtr = outStr;
  outIndex = outStartIndex;
  X = startX;
  for ( charIndex = startIndex; charIndex < rightCharIndex; charIndex++ ) {
    charLen = charIndex >= lineLen ? 1 :
              Fl_Text_Buffer::expand_character( lineStr[ charIndex ], outIndex, expandedChar,
                                                buf->tab_distance(), buf->null_substitution_character() );
    charStyle = position_style( lineStartPos, lineLen, charIndex,
                                outIndex + dispIndexOffset );
    for ( i = 0; i < charLen; i++ ) {
      if ( i != 0 && charIndex < lineLen && lineStr[ charIndex ] == '\t' )
        charStyle = position_style( lineStartPos, lineLen,
                                    charIndex, outIndex + dispIndexOffset );
      if ( charStyle != style ) {
        draw_string( style|BG_ONLY_MASK, sX, Y, X, outStr, outPtr - outStr );
        outPtr = outStr;
        sX = X;
        style = charStyle;
      }
      if ( charIndex < lineLen ) {
        *outPtr = expandedChar[ i ];
        charWidth = string_width( &expandedChar[ i ], 1, charStyle );
      } else
        charWidth = stdCharWidth;
      outPtr++;
      X += charWidth;
      outIndex++;
    }
    if ( outPtr - outStr + FL_TEXT_MAX_EXP_CHAR_LEN >= MAX_DISP_LINE_LEN || X >= rightClip )
      break;
  }
  draw_string( style|BG_ONLY_MASK, sX, Y, X, outStr, outPtr - outStr );

  /* now draw the text over the previously erased background */
  outPtr = outStr;
  outIndex = outStartIndex;
  X = startX;
  for ( charIndex = startIndex; charIndex < rightCharIndex; charIndex++ ) {
    charLen = charIndex >= lineLen ? 1 :
              Fl_Text_Buffer::expand_character( lineStr[ charIndex ], outIndex, expandedChar,
                                                buf->tab_distance(), buf->null_substitution_character() );
    charStyle = position_style( lineStartPos, lineLen, charIndex,
                                outIndex + dispIndexOffset );
    for ( i = 0; i < charLen; i++ ) {
      if ( i != 0 && charIndex < lineLen && lineStr[ charIndex ] == '\t' )
        charStyle = position_style( lineStartPos, lineLen,
                                    charIndex, outIndex + dispIndexOffset );
      if ( charStyle != style ) {
        draw_string( style|TEXT_ONLY_MASK, startX, Y, X, outStr, outPtr - outStr );
        outPtr = outStr;
        startX = X;
        style = charStyle;
      }
      if ( charIndex < lineLen ) {
        *outPtr = expandedChar[ i ];
        charWidth = string_width( &expandedChar[ i ], 1, charStyle );
      } else
        charWidth = stdCharWidth;
      outPtr++;
      X += charWidth;
      outIndex++;
    }
    if ( outPtr - outStr + FL_TEXT_MAX_EXP_CHAR_LEN >= MAX_DISP_LINE_LEN || X >= rightClip )
      break;
  }

  /* Draw the remaining style segment */
  draw_string( style|TEXT_ONLY_MASK, startX, Y, X, outStr, outPtr - outStr );

  /* Draw the cursor if part of it appeared on the redisplayed part of
     this line.  Also check for the cases which are not caught as the
     line is scanned above: when the cursor appears at the very end
     of the redisplayed section. */
  /*  CET - FIXME
    if ( mCursorOn )
    {
      if ( hasCursor )
        draw_cursor( cursorX, Y );
      else if ( charIndex < lineLen && ( lineStartPos + charIndex + 1 == cursorPos )
                && X == rightClip )
      {
        if ( cursorPos >= buf->length() )
          draw_cursor( X - 1, Y );
        else
        {
          draw_cursor( X - 1, Y );
        }
      }
    }
  */
  if ( lineStr != NULL )
    free((void *)lineStr);
}

/*
** Draw a string or blank area according to parameter "style", using the
** appropriate colors and drawing method for that style, with top left
** corner at X, y.  If style says to draw text, use "string" as source of
** characters, and draw "nChars", if style is FILL, erase
** rectangle where text would have drawn from X to toX and from Y to
** the maximum Y extent of the current font(s).
*/
void Fl_Text_Display::draw_string( int style, int X, int Y, int toX,
                                   const char *string, int nChars ) {
  const Style_Table_Entry * styleRec;

  /* Draw blank area rather than text, if that was the request */
  if ( style & FILL_MASK ) {
    if (style & TEXT_ONLY_MASK) return;
    clear_rect( style, X, Y, toX - X, mMaxsize );
    return;
  }

  /* Set font, color, and gc depending on style.  For normal text, GCs
     for normal drawing, or drawing within a Fl_Text_Selection or highlight are
     pre-allocated and pre-configured.  For syntax highlighting, GCs are
     configured here, on the fly. */

  Fl_Font font = textfont();
  int fsize = textsize();
  Fl_Color foreground;
  Fl_Color background;

  if ( style & STYLE_LOOKUP_MASK ) {
    int si = (style & STYLE_LOOKUP_MASK) - 'A';
    if (si < 0) si = 0;
    else if (si >= mNStyles) si = mNStyles - 1;

    styleRec = mStyleTable + si;
    font  = styleRec->font;
    fsize = styleRec->size;

    if (style & PRIMARY_MASK) {
      if (Fl::focus() == this) background = selection_color();
      else background = fl_color_average(color(), selection_color(), 0.4f);
    } else if (style & HIGHLIGHT_MASK) {
      if (Fl::focus() == this) background = fl_color_average(color(), selection_color(), 0.5f);
      else background = fl_color_average(color(), selection_color(), 0.6f);
    } else background = color();
    foreground = fl_contrast(styleRec->color, background);
  } else if (style & PRIMARY_MASK) {
    if (Fl::focus() == this) background = selection_color();
    else background = fl_color_average(color(), selection_color(), 0.4f);
    foreground = fl_contrast(textcolor(), background);
  } else if (style & HIGHLIGHT_MASK) {
    if (Fl::focus() == this) background = fl_color_average(color(), selection_color(), 0.5f);
    else background = fl_color_average(color(), selection_color(), 0.6f);
    foreground = fl_contrast(textcolor(), background);
  } else {
    foreground = textcolor();
    background = color();
  }

  if (!(style & TEXT_ONLY_MASK)) {
    fl_color( background );
    fl_rectf( X, Y, toX - X, mMaxsize );
  }
  if (!(style & BG_ONLY_MASK)) {
    fl_color( foreground );
    fl_font( font, fsize );
    fl_draw( string, nChars, X, Y + mMaxsize - fl_descent());
  }

  // CET - FIXME
  /* If any space around the character remains unfilled (due to use of
     different sized fonts for highlighting), fill in above or below
     to erase previously drawn characters */
  /*
      if (fs->ascent < mAscent)
        clear_rect( style, X, Y, toX - X, mAscent - fs->ascent);
      if (fs->descent < mDescent)
        clear_rect( style, X, Y + mAscent + fs->descent, toX - x,
                mDescent - fs->descent);
  */
  /* Underline if style is secondary Fl_Text_Selection */

  /*
      if (style & SECONDARY_MASK)
        XDrawLine(XtDisplay(mW), XtWindow(mW), gc, x,
                y + mAscent, toX - 1, Y + fs->ascent);
  */
}


/*
** Clear a rectangle with the appropriate background color for "style"
*/
void Fl_Text_Display::clear_rect( int style, int X, int Y,
                                  int width, int height ) {
  /* A width of zero means "clear to end of window" to XClearArea */
  if ( width == 0 )
    return;

  if (style & PRIMARY_MASK) {
    if (Fl::focus()==this) {
      fl_color(selection_color());
    } else {
      fl_color(fl_color_average(color(), selection_color(), 0.4f));
    }
  } else if (style & HIGHLIGHT_MASK) {
    if (Fl::focus()==this) {
      fl_color(fl_color_average(color(), selection_color(), 0.5f));
    } else {
      fl_color(fl_color_average(color(), selection_color(), 0.6f));
    }
  } else {
    fl_color( color() );
  }
  fl_rectf( X, Y, width, height );
}


/*
** Draw a cursor with top center at X, y.
*/
void Fl_Text_Display::draw_cursor( int X, int Y ) {
  typedef struct {
    int x1, y1, x2, y2;
  }
  Segment;

  Segment segs[ 5 ];
  int left, right, cursorWidth, midY;
  //    int fontWidth = mFontStruct->min_bounds.width, nSegs = 0;
  int fontWidth = TMPFONTWIDTH; // CET - FIXME
  int nSegs = 0;
  int fontHeight = mMaxsize;
  int bot = Y + fontHeight - 1;

  if ( X < text_area.x - 1 || X > text_area.x + text_area.w )
    return;

  /* For cursors other than the block, make them around 2/3 of a character
     width, rounded to an even number of pixels so that X will draw an
     odd number centered on the stem at x. */
  cursorWidth = 4;   //(fontWidth/3) * 2;
  left = X - cursorWidth / 2;
  right = left + cursorWidth;

  /* Create segments and draw cursor */
  if ( mCursorStyle == CARET_CURSOR ) {
    midY = bot - fontHeight / 5;
    segs[ 0 ].x1 = left; segs[ 0 ].y1 = bot; segs[ 0 ].x2 = X; segs[ 0 ].y2 = midY;
    segs[ 1 ].x1 = X; segs[ 1 ].y1 = midY; segs[ 1 ].x2 = right; segs[ 1 ].y2 = bot;
    segs[ 2 ].x1 = left; segs[ 2 ].y1 = bot; segs[ 2 ].x2 = X; segs[ 2 ].y2 = midY - 1;
    segs[ 3 ].x1 = X; segs[ 3 ].y1 = midY - 1; segs[ 3 ].x2 = right; segs[ 3 ].y2 = bot;
    nSegs = 4;
  } else if ( mCursorStyle == NORMAL_CURSOR ) {
    segs[ 0 ].x1 = left; segs[ 0 ].y1 = Y; segs[ 0 ].x2 = right; segs[ 0 ].y2 = Y;
    segs[ 1 ].x1 = X; segs[ 1 ].y1 = Y; segs[ 1 ].x2 = X; segs[ 1 ].y2 = bot;
    segs[ 2 ].x1 = left; segs[ 2 ].y1 = bot; segs[ 2 ].x2 = right; segs[ 2 ].y2 = bot;
    nSegs = 3;
  } else if ( mCursorStyle == HEAVY_CURSOR ) {
    segs[ 0 ].x1 = X - 1; segs[ 0 ].y1 = Y; segs[ 0 ].x2 = X - 1; segs[ 0 ].y2 = bot;
    segs[ 1 ].x1 = X; segs[ 1 ].y1 = Y; segs[ 1 ].x2 = X; segs[ 1 ].y2 = bot;
    segs[ 2 ].x1 = X + 1; segs[ 2 ].y1 = Y; segs[ 2 ].x2 = X + 1; segs[ 2 ].y2 = bot;
    segs[ 3 ].x1 = left; segs[ 3 ].y1 = Y; segs[ 3 ].x2 = right; segs[ 3 ].y2 = Y;
    segs[ 4 ].x1 = left; segs[ 4 ].y1 = bot; segs[ 4 ].x2 = right; segs[ 4 ].y2 = bot;
    nSegs = 5;
  } else if ( mCursorStyle == DIM_CURSOR ) {
    midY = Y + fontHeight / 2;
    segs[ 0 ].x1 = X; segs[ 0 ].y1 = Y; segs[ 0 ].x2 = X; segs[ 0 ].y2 = Y;
    segs[ 1 ].x1 = X; segs[ 1 ].y1 = midY; segs[ 1 ].x2 = X; segs[ 1 ].y2 = midY;
    segs[ 2 ].x1 = X; segs[ 2 ].y1 = bot; segs[ 2 ].x2 = X; segs[ 2 ].y2 = bot;
    nSegs = 3;
  } else if ( mCursorStyle == BLOCK_CURSOR ) {
    right = X + fontWidth;
    segs[ 0 ].x1 = X; segs[ 0 ].y1 = Y; segs[ 0 ].x2 = right; segs[ 0 ].y2 = Y;
    segs[ 1 ].x1 = right; segs[ 1 ].y1 = Y; segs[ 1 ].x2 = right; segs[ 1 ].y2 = bot;
    segs[ 2 ].x1 = right; segs[ 2 ].y1 = bot; segs[ 2 ].x2 = X; segs[ 2 ].y2 = bot;
    segs[ 3 ].x1 = X; segs[ 3 ].y1 = bot; segs[ 3 ].x2 = X; segs[ 3 ].y2 = Y;
    nSegs = 4;
  }
  fl_color( mCursor_color );

  for ( int k = 0; k < nSegs; k++ ) {
    fl_line( segs[ k ].x1, segs[ k ].y1, segs[ k ].x2, segs[ k ].y2 );
  }
}

/*
** Determine the drawing method to use to draw a specific character from "buf".
** "lineStartPos" gives the character index where the line begins, "lineIndex",
** the number of characters past the beginning of the line, and "dispIndex",
** the number of displayed characters past the beginning of the line.  Passing
** lineStartPos of -1 returns the drawing style for "no text".
**
** Why not just: position_style(pos)?  Because style applies to blank areas
** of the window beyond the text boundaries, and because this routine must also
** decide whether a position is inside of a rectangular Fl_Text_Selection, and do
** so efficiently, without re-counting character positions from the start of the
** line.
**
** Note that style is a somewhat incorrect name, drawing method would
** be more appropriate.
*/
int Fl_Text_Display::position_style( int lineStartPos,
                                     int lineLen, int lineIndex, int dispIndex ) {
  Fl_Text_Buffer * buf = mBuffer;
  Fl_Text_Buffer *styleBuf = mStyleBuffer;
  int pos, style = 0;

  if ( lineStartPos == -1 || buf == NULL )
    return FILL_MASK;

  pos = lineStartPos + min( lineIndex, lineLen );

  if ( lineIndex >= lineLen )
    style = FILL_MASK;
  else if ( styleBuf != NULL ) {
    style = ( unsigned char ) styleBuf->character( pos );
    if (style == mUnfinishedStyle && mUnfinishedHighlightCB) {
        /* encountered "unfinished" style, trigger parsing */
        (mUnfinishedHighlightCB)( pos, mHighlightCBArg);
        style = (unsigned char) styleBuf->character( pos);
    }
  }
  if (buf->primary_selection()->includes(pos, lineStartPos, dispIndex))
    style |= PRIMARY_MASK;
  if (buf->highlight_selection()->includes(pos, lineStartPos, dispIndex))
    style |= HIGHLIGHT_MASK;
  if (buf->secondary_selection()->includes(pos, lineStartPos, dispIndex))
    style |= SECONDARY_MASK;
  return style;
}

/*
** Find the width of a string in the font of a particular style
*/
int Fl_Text_Display::string_width( const char *string, int length, int style ) {
  Fl_Font font;
  int fsize;

  if ( style & STYLE_LOOKUP_MASK ) {
    int si = (style & STYLE_LOOKUP_MASK) - 'A';
    if (si < 0) si = 0;
    else if (si >= mNStyles) si = mNStyles - 1;

    font  = mStyleTable[si].font;
    fsize = mStyleTable[si].size;
  } else {
    font  = textfont();
    fsize = textsize();
  }
  fl_font( font, fsize );

  return ( int ) ( fl_width( string, length ) );
}

/*
** Translate window coordinates to the nearest (insert cursor or character
** cell) text position.  The parameter posType specifies how to interpret the
** position: CURSOR_POS means translate the coordinates to the nearest cursor
** position, and CHARACTER_POS means return the position of the character
** closest to (X, Y).
*/
int Fl_Text_Display::xy_to_position( int X, int Y, int posType ) {
  int charIndex, lineStart, lineLen, fontHeight;
  int charWidth, charLen, charStyle, visLineNum, xStep, outIndex;
  char expandedChar[ FL_TEXT_MAX_EXP_CHAR_LEN ];
  const char *lineStr;

  /* Find the visible line number corresponding to the Y coordinate */
  fontHeight = mMaxsize;
  visLineNum = ( Y - text_area.y ) / fontHeight;
  if ( visLineNum < 0 )
    return mFirstChar;
  if ( visLineNum >= mNVisibleLines )
    visLineNum = mNVisibleLines - 1;

  /* Find the position at the start of the line */
  lineStart = mLineStarts[ visLineNum ];

  /* If the line start was empty, return the last position in the buffer */
  if ( lineStart == -1 )
    return mBuffer->length();

  /* Get the line text and its length */
  lineLen = vline_length( visLineNum );
  lineStr = mBuffer->text_range( lineStart, lineStart + lineLen );

  /* Step through character positions from the beginning of the line
     to find the character position corresponding to the X coordinate */
  xStep = text_area.x - mHorizOffset;
  outIndex = 0;
  for ( charIndex = 0; charIndex < lineLen; charIndex++ ) {
    charLen = Fl_Text_Buffer::expand_character( lineStr[ charIndex ], outIndex, expandedChar,
              mBuffer->tab_distance(), mBuffer->null_substitution_character() );
    charStyle = position_style( lineStart, lineLen, charIndex, outIndex );
    charWidth = string_width( expandedChar, charLen, charStyle );
    if ( X < xStep + ( posType == CURSOR_POS ? charWidth / 2 : charWidth ) ) {
      free((char *)lineStr);
      return lineStart + charIndex;
    }
    xStep += charWidth;
    outIndex += charLen;
  }

  /* If the X position was beyond the end of the line, return the position
     of the newline at the end of the line */
  free((char *)lineStr);
  return lineStart + lineLen;
}

/*
** Translate window coordinates to the nearest row and column number for
** positioning the cursor.  This, of course, makes no sense when the font is
** proportional, since there are no absolute columns.  The parameter posType
** specifies how to interpret the position: CURSOR_POS means translate the
** coordinates to the nearest position between characters, and CHARACTER_POS
** means translate the position to the nearest character cell.
*/
void Fl_Text_Display::xy_to_rowcol( int X, int Y, int *row,
                                    int *column, int posType ) {
  int fontHeight = mMaxsize;
  int fontWidth = TMPFONTWIDTH;   //mFontStruct->max_bounds.width;

  /* Find the visible line number corresponding to the Y coordinate */
  *row = ( Y - text_area.y ) / fontHeight;
  if ( *row < 0 ) * row = 0;
  if ( *row >= mNVisibleLines ) * row = mNVisibleLines - 1;
  *column = ( ( X - text_area.x ) + mHorizOffset +
              ( posType == CURSOR_POS ? fontWidth / 2 : 0 ) ) / fontWidth;
  if ( *column < 0 ) * column = 0;
}

/*
** Offset the line starts array, mTopLineNum, mFirstChar and lastChar, for a new
** vertical scroll position given by newTopLineNum.  If any currently displayed
** lines will still be visible, salvage the line starts values, otherwise,
** count lines from the nearest known line start (start or end of buffer, or
** the closest value in the mLineStarts array)
*/
void Fl_Text_Display::offset_line_starts( int newTopLineNum ) {
  int oldTopLineNum = mTopLineNum;
  int oldFirstChar = mFirstChar;
  int lineDelta = newTopLineNum - oldTopLineNum;
  int nVisLines = mNVisibleLines;
  int *lineStarts = mLineStarts;
  int i, lastLineNum;
  Fl_Text_Buffer *buf = mBuffer;

  /* If there was no offset, nothing needs to be changed */
  if ( lineDelta == 0 )
    return;

  /* Find the new value for mFirstChar by counting lines from the nearest
     known line start (start or end of buffer, or the closest value in the
     lineStarts array) */
  lastLineNum = oldTopLineNum + nVisLines - 1;
  if ( newTopLineNum < oldTopLineNum && newTopLineNum < -lineDelta ) {
    mFirstChar = skip_lines( 0, newTopLineNum - 1, true );
  } else if ( newTopLineNum < oldTopLineNum ) {
    mFirstChar = rewind_lines( mFirstChar, -lineDelta );
  } else if ( newTopLineNum < lastLineNum ) {
    mFirstChar = lineStarts[ newTopLineNum - oldTopLineNum ];
  } else if ( newTopLineNum - lastLineNum < mNBufferLines - newTopLineNum ) {
    mFirstChar = skip_lines( lineStarts[ nVisLines - 1 ],
                                       newTopLineNum - lastLineNum, true );
  } else {
    mFirstChar = rewind_lines( buf->length(), mNBufferLines - newTopLineNum + 1 );
  }

  /* Fill in the line starts array */
  if ( lineDelta < 0 && -lineDelta < nVisLines ) {
    for ( i = nVisLines - 1; i >= -lineDelta; i-- )
      lineStarts[ i ] = lineStarts[ i + lineDelta ];
    calc_line_starts( 0, -lineDelta );
  } else if ( lineDelta > 0 && lineDelta < nVisLines ) {
    for ( i = 0; i < nVisLines - lineDelta; i++ )
      lineStarts[ i ] = lineStarts[ i + lineDelta ];
    calc_line_starts( nVisLines - lineDelta, nVisLines - 1 );
  } else
    calc_line_starts( 0, nVisLines );

  /* Set lastChar and mTopLineNum */
  calc_last_char();
  mTopLineNum = newTopLineNum;

    /* If we're numbering lines or being asked to maintain an absolute line
       number, re-calculate the absolute line number */
    absolute_top_line_number(oldFirstChar);
}

/*
** Update the line starts array, mTopLineNum, mFirstChar and lastChar for text
** display "textD" after a modification to the text buffer, given by the
** position where the change began "pos", and the nmubers of characters
** and lines inserted and deleted.
*/
void Fl_Text_Display::update_line_starts( int pos, int charsInserted,
    int charsDeleted, int linesInserted, int linesDeleted, int *scrolled ) {
  int * lineStarts = mLineStarts;
  int i, lineOfPos, lineOfEnd, nVisLines = mNVisibleLines;
  int charDelta = charsInserted - charsDeleted;
  int lineDelta = linesInserted - linesDeleted;

  /* If all of the changes were before the displayed text, the display
     doesn't change, just update the top line num and offset the line
     start entries and first and last characters */
  if ( pos + charsDeleted < mFirstChar ) {
    mTopLineNum += lineDelta;
    for ( i = 0; i < nVisLines && lineStarts[i] != -1; i++ )
      lineStarts[ i ] += charDelta;
    mFirstChar += charDelta;
    mLastChar += charDelta;
    *scrolled = 0;
    return;
  }

  /* The change began before the beginning of the displayed text, but
     part or all of the displayed text was deleted */
  if ( pos < mFirstChar ) {
    /* If some text remains in the window, anchor on that  */
    if ( position_to_line( pos + charsDeleted, &lineOfEnd ) &&
         ++lineOfEnd < nVisLines && lineStarts[ lineOfEnd ] != -1 ) {
      mTopLineNum = max( 1, mTopLineNum + lineDelta );
      mFirstChar = rewind_lines(
            lineStarts[ lineOfEnd ] + charDelta, lineOfEnd );
      /* Otherwise anchor on original line number and recount everything */
    } else {
      if ( mTopLineNum > mNBufferLines + lineDelta ) {
        mTopLineNum = 1;
        mFirstChar = 0;
      } else
        mFirstChar = skip_lines( 0, mTopLineNum - 1, true );
    }
    calc_line_starts( 0, nVisLines - 1 );
    /* calculate lastChar by finding the end of the last displayed line */
    calc_last_char();
    *scrolled = 1;
    return;
  }

  /* If the change was in the middle of the displayed text (it usually is),
     salvage as much of the line starts array as possible by moving and
     offsetting the entries after the changed area, and re-counting the
     added lines or the lines beyond the salvaged part of the line starts
     array */
  if ( pos <= mLastChar ) {
    /* find line on which the change began */
    position_to_line( pos, &lineOfPos );
    /* salvage line starts after the changed area */
    if ( lineDelta == 0 ) {
      for ( i = lineOfPos + 1; i < nVisLines && lineStarts[ i ] != -1; i++ )
        lineStarts[ i ] += charDelta;
    } else if ( lineDelta > 0 ) {
      for ( i = nVisLines - 1; i >= lineOfPos + lineDelta + 1; i-- )
        lineStarts[ i ] = lineStarts[ i - lineDelta ] +
                          ( lineStarts[ i - lineDelta ] == -1 ? 0 : charDelta );
    } else /* (lineDelta < 0) */ {
      for ( i = max( 0, lineOfPos + 1 ); i < nVisLines + lineDelta; i++ )
        lineStarts[ i ] = lineStarts[ i - lineDelta ] +
                          ( lineStarts[ i - lineDelta ] == -1 ? 0 : charDelta );
    }
    /* fill in the missing line starts */
    if ( linesInserted >= 0 )
      calc_line_starts( lineOfPos + 1, lineOfPos + linesInserted );
    if ( lineDelta < 0 )
      calc_line_starts( nVisLines + lineDelta, nVisLines );
    /* calculate lastChar by finding the end of the last displayed line */
    calc_last_char();
    *scrolled = 0;
    return;
  }

  /* Change was past the end of the displayed text, but displayable by virtue
     of being an insert at the end of the buffer into visible blank lines */
  if ( empty_vlines() ) {
    position_to_line( pos, &lineOfPos );
    calc_line_starts( lineOfPos, lineOfPos + linesInserted );
    calc_last_char();
    *scrolled = 0;
    return;
  }

  /* Change was beyond the end of the buffer and not visible, do nothing */
  *scrolled = 0;
}

/*
** Scan through the text in the "textD"'s buffer and recalculate the line
** starts array values beginning at index "startLine" and continuing through
** (including) "endLine".  It assumes that the line starts entry preceding
** "startLine" (or mFirstChar if startLine is 0) is good, and re-counts
** newlines to fill in the requested entries.  Out of range values for
** "startLine" and "endLine" are acceptable.
*/
void Fl_Text_Display::calc_line_starts( int startLine, int endLine ) {
  int startPos, bufLen = mBuffer->length();
  int line, lineEnd, nextLineStart, nVis = mNVisibleLines;
  int *lineStarts = mLineStarts;

  /* Clean up (possibly) messy input parameters */
  if ( endLine < 0 ) endLine = 0;
  if ( endLine >= nVis ) endLine = nVis - 1;
  if ( startLine < 0 ) startLine = 0;
  if ( startLine >= nVis ) startLine = nVis - 1;
  if ( startLine > endLine )
    return;

  /* Find the last known good line number -> position mapping */
  if ( startLine == 0 ) {
    lineStarts[ 0 ] = mFirstChar;
    startLine = 1;
  }
  startPos = lineStarts[ startLine - 1 ];

  /* If the starting position is already past the end of the text,
     fill in -1's (means no text on line) and return */
  if ( startPos == -1 ) {
    for ( line = startLine; line <= endLine; line++ )
      lineStarts[ line ] = -1;
    return;
  }

  /* Loop searching for ends of lines and storing the positions of the
     start of the next line in lineStarts */
  for ( line = startLine; line <= endLine; line++ ) {
    find_line_end(startPos, true, &lineEnd, &nextLineStart);
    startPos = nextLineStart;
    if ( startPos >= bufLen ) {
      /* If the buffer ends with a newline or line break, put
         buf->length() in the next line start position (instead of
         a -1 which is the normal marker for an empty line) to
         indicate that the cursor may safely be displayed there */
      if ( line == 0 || ( lineStarts[ line - 1 ] != bufLen &&
                          lineEnd != nextLineStart ) ) {
        lineStarts[ line ] = bufLen;
        line++;
      }
      break;
    }
    lineStarts[ line ] = startPos;
  }

  /* Set any entries beyond the end of the text to -1 */
  for ( ; line <= endLine; line++ )
    lineStarts[ line ] = -1;
}

/*
** Given a Fl_Text_Display with a complete, up-to-date lineStarts array, update
** the lastChar entry to point to the last buffer position displayed.
*/
void Fl_Text_Display::calc_last_char() {
  int i;
  for (i = mNVisibleLines - 1; i >= 0 && mLineStarts[i] == -1; i--) ;
  mLastChar = i < 0 ? 0 : line_end(mLineStarts[i], true);
}

void Fl_Text_Display::scroll(int topLineNum, int horizOffset) {
  mTopLineNumHint = topLineNum;
  mHorizOffsetHint = horizOffset;
  resize(x(), y(), w(), h());
}

void Fl_Text_Display::scroll_(int topLineNum, int horizOffset) {
  /* Limit the requested scroll position to allowable values */
  if (topLineNum > mNBufferLines + 3 - mNVisibleLines)
    topLineNum = mNBufferLines + 3 - mNVisibleLines;
  if (topLineNum < 1) topLineNum = 1;

  if (horizOffset > longest_vline() - text_area.w)
    horizOffset = longest_vline() - text_area.w;
  if (horizOffset < 0) horizOffset = 0;

  /* Do nothing if scroll position hasn't actually changed or there's no
     window to draw in yet */
  if (mHorizOffset == horizOffset && mTopLineNum == topLineNum)
    return;

  /* If the vertical scroll position has changed, update the line
     starts array and related counters in the text display */
  offset_line_starts(topLineNum);

  /* Just setting mHorizOffset is enough information for redisplay */
  mHorizOffset = horizOffset;

  // redraw all text
  damage(FL_DAMAGE_EXPOSE);
}

/*
** Update the minimum, maximum, slider size, page increment, and value
** for vertical scroll bar.
*/
void Fl_Text_Display::update_v_scrollbar() {
  /* The Vert. scroll bar value and slider size directly represent the top
     line number, and the number of visible lines respectively.  The scroll
     bar maximum value is chosen to generally represent the size of the whole
     buffer, with minor adjustments to keep the scroll bar widget happy */
#ifdef DEBUG
  printf("Fl_Text_Display::update_v_scrollbar():\n"
         "    mTopLineNum=%d, mNVisibleLines=%d, mNBufferLines=%d\n",
	 mTopLineNum, mNVisibleLines, mNBufferLines);
#endif // DEBUG

  mVScrollBar->value(mTopLineNum, mNVisibleLines, 1, mNBufferLines+2);
  mVScrollBar->linesize(3);
}

/*
** Update the minimum, maximum, slider size, page increment, and value
** for the horizontal scroll bar.
*/
void Fl_Text_Display::update_h_scrollbar() {
  int sliderMax = max(longest_vline(), text_area.w + mHorizOffset);
  mHScrollBar->value( mHorizOffset, text_area.w, 0, sliderMax );
}

/*
** Callbacks for drag or valueChanged on scroll bars
*/
void Fl_Text_Display::v_scrollbar_cb(Fl_Scrollbar* b, Fl_Text_Display* textD) {
  if (b->value() == textD->mTopLineNum) return;
  textD->scroll(b->value(), textD->mHorizOffset);
}

void Fl_Text_Display::h_scrollbar_cb(Fl_Scrollbar* b, Fl_Text_Display* textD) {
  if (b->value() == textD->mHorizOffset) return;
  textD->scroll(textD->mTopLineNum, b->value());
}

/*
** Refresh the line number area.  If clearAll is False, writes only over
** the character cell areas.  Setting clearAll to True will clear out any
** stray marks outside of the character cell area, which might have been
** left from before a resize or font change.
*/
void Fl_Text_Display::draw_line_numbers(bool /*clearAll*/) {
#if 0
	 // FIXME: don't want this yet, so will leave for another time

    int y, line, visLine, nCols, lineStart;
    char lineNumString[12];
    int lineHeight = mMaxsize ? mMaxsize : textsize_;
    int charWidth = TMPFONTWIDTH;   //mFontStruct->max_bounds.width;
    
    /* Don't draw if mLineNumWidth == 0 (line numbers are hidden), or widget is
       not yet realized */
    if (mLineNumWidth == 0 || visible_r())
    	return;
    
    /* GC is allocated on demand, since not everyone will use line numbering */
    if (textD->lineNumGC == NULL) {
	XGCValues values;
 	values.foreground = textD->lineNumFGPixel;
	values.background = textD->bgPixel;
	values.font = textD->fontStruct->fid;
   	textD->lineNumGC = XtGetGC(textD->w,
		GCFont| GCForeground | GCBackground, &values);
    }
    
    /* Erase the previous contents of the line number area, if requested */
    if (clearAll)
    	XClearArea(XtDisplay(textD->w), XtWindow(textD->w), textD->lineNumLeft,
		textD->top, textD->lineNumWidth, textD->height, False);
    
    /* Draw the line numbers, aligned to the text */
    nCols = min(11, textD->lineNumWidth / charWidth);
    y = textD->top;
    line = getAbsTopLineNum(textD);
    for (visLine=0; visLine < textD->nVisibleLines; visLine++) {
	lineStart = textD->lineStarts[visLine];
	if (lineStart != -1 && (lineStart==0 ||
		BufGetCharacter(textD->buffer, lineStart-1)=='\n')) {
	    sprintf(lineNumString, "%*d", nCols, line);
	    XDrawImageString(XtDisplay(textD->w), XtWindow(textD->w),
		    textD->lineNumGC, textD->lineNumLeft, y + textD->ascent,
		    lineNumString, strlen(lineNumString));
	    line++;
	} else {
	    XClearArea(XtDisplay(textD->w), XtWindow(textD->w),
		    textD->lineNumLeft, y, textD->lineNumWidth,
		    textD->ascent + textD->descent, False);
	    if (visLine == 0)
		line++;
	}
	y += lineHeight;
    }
#endif
}

static int max( int i1, int i2 ) {
  return i1 >= i2 ? i1 : i2;
}

static int min( int i1, int i2 ) {
  return i1 <= i2 ? i1 : i2;
}

/*
** Count the number of newlines in a null-terminated text string;
*/
static int countlines( const char *string ) {
  const char * c;
  int lineCount = 0;

  if (!string) return 0;

  for ( c = string; *c != '\0'; c++ )
    if ( *c == '\n' ) lineCount++;
  return lineCount;
}

/*
** Return the width in pixels of the displayed line pointed to by "visLineNum"
*/
int Fl_Text_Display::measure_vline( int visLineNum ) {
  int i, width = 0, len, style, lineLen = vline_length( visLineNum );
  int charCount = 0, lineStartPos = mLineStarts[ visLineNum ];
  char expandedChar[ FL_TEXT_MAX_EXP_CHAR_LEN ];

  if (lineStartPos < 0 || lineLen == 0) return 0;
  if ( mStyleBuffer == NULL ) {
    for ( i = 0; i < lineLen; i++ ) {
      len = mBuffer->expand_character( lineStartPos + i,
                                       charCount, expandedChar );

      fl_font( textfont(), textsize() );

      width += ( int ) fl_width( expandedChar, len );

      charCount += len;
    }
  } else {
    for ( i = 0; i < lineLen; i++ ) {
      len = mBuffer->expand_character( lineStartPos + i,
                                       charCount, expandedChar );
      style = ( unsigned char ) mStyleBuffer->character(
                lineStartPos + i ) - 'A';

      if (style < 0) style = 0;
      else if (style >= mNStyles) style = mNStyles - 1;

      fl_font( mStyleTable[ style ].font, mStyleTable[ style ].size );

      width += ( int ) fl_width( expandedChar, len );

      charCount += len;
    }
  }
  return width;
}

/*
** Return true if there are lines visible with no corresponding buffer text
*/
int Fl_Text_Display::empty_vlines() {
  return mNVisibleLines > 0 &&
         mLineStarts[ mNVisibleLines - 1 ] == -1;
}

/*
** Return the length of a line (number of displayable characters) by examining
** entries in the line starts array rather than by scanning for newlines
*/
int Fl_Text_Display::vline_length( int visLineNum ) {
  int nextLineStart, lineStartPos;

  if (visLineNum < 0 || visLineNum >= mNVisibleLines)
    return (0);

  lineStartPos = mLineStarts[ visLineNum ];

  if ( lineStartPos == -1 )
    return 0;
  if ( visLineNum + 1 >= mNVisibleLines )
    return mLastChar - lineStartPos;
  nextLineStart = mLineStarts[ visLineNum + 1 ];
  if ( nextLineStart == -1 )
    return mLastChar - lineStartPos;
  if (wrap_uses_character(nextLineStart-1))
    return nextLineStart-1 - lineStartPos;
  return nextLineStart - lineStartPos;
}

/*
** When continuous wrap is on, and the user inserts or deletes characters,
** wrapping can happen before and beyond the changed position.  This routine
** finds the extent of the changes, and counts the deleted and inserted lines
** over that range.  It also attempts to minimize the size of the range to
** what has to be counted and re-displayed, so the results can be useful
** both for delimiting where the line starts need to be recalculated, and
** for deciding what part of the text to redisplay.
*/
void Fl_Text_Display::find_wrap_range(const char *deletedText, int pos,
    	int nInserted, int nDeleted, int *modRangeStart, int *modRangeEnd,
    	int *linesInserted, int *linesDeleted) {
    int length, retPos, retLines, retLineStart, retLineEnd;
    Fl_Text_Buffer *deletedTextBuf, *buf = buffer();
    int nVisLines = mNVisibleLines;
    int *lineStarts = mLineStarts;
    int countFrom, countTo, lineStart, adjLineStart, i;
    int visLineNum = 0, nLines = 0;
    
    /*
    ** Determine where to begin searching: either the previous newline, or
    ** if possible, limit to the start of the (original) previous displayed
    ** line, using information from the existing line starts array
    */
    if (pos >= mFirstChar && pos <= mLastChar) {
    	for (i=nVisLines-1; i>0; i--)
    	    if (lineStarts[i] != -1 && pos >= lineStarts[i])
    		break;
    	if (i > 0) {
    	    countFrom = lineStarts[i-1];
    	    visLineNum = i-1;
    	} else
    	    countFrom = buf->line_start(pos);
    } else
    	countFrom = buf->line_start(pos);

    
    /*
    ** Move forward through the (new) text one line at a time, counting
    ** displayed lines, and looking for either a real newline, or for the
    ** line starts to re-sync with the original line starts array
    */
    lineStart = countFrom;
    *modRangeStart = countFrom;
    for (;;) {
    	
    	/* advance to the next line.  If the line ended in a real newline
    	   or the end of the buffer, that's far enough */
    	wrapped_line_counter(buf, lineStart, buf->length(), 1, true, 0,
    	    	&retPos, &retLines, &retLineStart, &retLineEnd);
    	if (retPos >= buf->length()) {
    	    countTo = buf->length();
    	    *modRangeEnd = countTo;
    	    if (retPos != retLineEnd)
    	    	nLines++;
    	    break;
    	} else
    	    lineStart = retPos;
    	nLines++;
    	if (lineStart > pos + nInserted &&
    	    	buf->character(lineStart-1) == '\n') {
    	    countTo = lineStart;
    	    *modRangeEnd = lineStart;
    	    break;
    	}
        
	/* Don't try to resync in continuous wrap mode with non-fixed font
	   sizes; it would result in a chicken-and-egg dependency between
	   the calculations for the inserted and the deleted lines. 
           If we're in that mode, the number of deleted lines is calculated in
           advance, without resynchronization, so we shouldn't resynchronize
           for the inserted lines either. */
	if (mSuppressResync)
	    continue;
    	
    	/* check for synchronization with the original line starts array
    	   before pos, if so, the modified range can begin later */
     	if (lineStart <= pos) {
    	    while (visLineNum<nVisLines && lineStarts[visLineNum] < lineStart)
    		visLineNum++;
     	    if (visLineNum < nVisLines && lineStarts[visLineNum] == lineStart) {
    		countFrom = lineStart;
    		nLines = 0;
    		if (visLineNum+1 < nVisLines && lineStarts[visLineNum+1] != -1)
    		    *modRangeStart = min(pos, lineStarts[visLineNum+1]-1);
    		else
    		    *modRangeStart = countFrom;
    	    } else
    	    	*modRangeStart = min(*modRangeStart, lineStart-1);
    	}
    	
   	/* check for synchronization with the original line starts array
    	   after pos, if so, the modified range can end early */
    	else if (lineStart > pos + nInserted) {
    	    adjLineStart = lineStart - nInserted + nDeleted;
    	    while (visLineNum<nVisLines && lineStarts[visLineNum]<adjLineStart)
    	    	visLineNum++;
    	    if (visLineNum < nVisLines && lineStarts[visLineNum] != -1 &&
    	    	    lineStarts[visLineNum] == adjLineStart) {
    	    	countTo = line_end(lineStart, true);
    	    	*modRangeEnd = lineStart;
    	    	break;
    	    }
    	}
    }
    *linesInserted = nLines;


    /* Count deleted lines between countFrom and countTo as the text existed
       before the modification (that is, as if the text between pos and
       pos+nInserted were replaced by "deletedText").  This extra context is
       necessary because wrapping can occur outside of the modified region
       as a result of adding or deleting text in the region. This is done by
       creating a textBuffer containing the deleted text and the necessary
       additional context, and calling the wrappedLineCounter on it.
       
       NOTE: This must not be done in continuous wrap mode when the font
	     width is not fixed. In that case, the calculation would try
	     to access style information that is no longer available (deleted
	     text), or out of date (updated highlighting), possibly leading 
	     to completely wrong calculations and/or even crashes eventually.
	     (This is not theoretical; it really happened.)
	     
	     In that case, the calculation of the number of deleted lines
	     has happened before the buffer was modified (only in that case,
	     because resynchronization of the line starts is impossible
	     in that case, which makes the whole calculation less efficient).
    */
    if (mSuppressResync) {
	*linesDeleted = mNLinesDeleted;
	mSuppressResync = 0;
	return;
    }

    length = (pos-countFrom) + nDeleted +(countTo-(pos+nInserted));
    deletedTextBuf = new Fl_Text_Buffer(length);
    deletedTextBuf->copy(buffer(), countFrom, pos, 0);
    if (nDeleted != 0)
    	deletedTextBuf->insert(pos-countFrom, deletedText);
    deletedTextBuf->copy(buffer(), 
    	    pos+nInserted, countTo, pos-countFrom+nDeleted);
    /* Note that we need to take into account an offset for the style buffer:
       the deletedTextBuf can be out of sync with the style buffer. */
    wrapped_line_counter(deletedTextBuf, 0, length, INT_MAX, true, 
	    countFrom, &retPos, &retLines, &retLineStart, &retLineEnd, false);
    delete deletedTextBuf;
    *linesDeleted = retLines;
    mSuppressResync = 0;
}

/*
** This is a stripped-down version of the findWrapRange() function above,
** intended to be used to calculate the number of "deleted" lines during
** a buffer modification. It is called _before_ the modification takes place.
** 
** This function should only be called in continuous wrap mode with a
** non-fixed font width. In that case, it is impossible to calculate
** the number of deleted lines, because the necessary style information 
** is no longer available _after_ the modification. In other cases, we
** can still perform the calculation afterwards (possibly even more
** efficiently).
*/
void Fl_Text_Display::measure_deleted_lines(int pos, int nDeleted) {
    int retPos, retLines, retLineStart, retLineEnd;
    Fl_Text_Buffer *buf = buffer();
    int nVisLines = mNVisibleLines;
    int *lineStarts = mLineStarts;
    int countFrom, lineStart;
    int nLines = 0, i;
    /*
    ** Determine where to begin searching: either the previous newline, or
    ** if possible, limit to the start of the (original) previous displayed
    ** line, using information from the existing line starts array
    */
    if (pos >= mFirstChar && pos <= mLastChar) {
    	for (i=nVisLines-1; i>0; i--)
    	    if (lineStarts[i] != -1 && pos >= lineStarts[i])
    		break;
    	if (i > 0) countFrom = lineStarts[i-1];
    	else countFrom = buf->line_start(pos);
    } else
    	countFrom = buf->line_start(pos);
    
    /*
    ** Move forward through the (new) text one line at a time, counting
    ** displayed lines, and looking for either a real newline, or for the
    ** line starts to re-sync with the original line starts array
    */
    lineStart = countFrom;
    for (;;) {
    	/* advance to the next line.  If the line ended in a real newline
    	   or the end of the buffer, that's far enough */
    	wrapped_line_counter(buf, lineStart, buf->length(), 1, true, 0,
    	    	&retPos, &retLines, &retLineStart, &retLineEnd);
    	if (retPos >= buf->length()) {
    	    if (retPos != retLineEnd)
    	    	nLines++;
    	    break;
    	} else
    	    lineStart = retPos;
    	nLines++;
    	if (lineStart > pos + nDeleted &&
    	    	buf->character(lineStart-1) == '\n') {
    	    break;
    	}
	
	/* Unlike in the findWrapRange() function above, we don't try to 
	   resync with the line starts, because we don't know the length 
	   of the inserted text yet, nor the updated style information. 
	   
	   Because of that, we also shouldn't resync with the line starts
	   after the modification either, because we must perform the
	   calculations for the deleted and inserted lines in the same way. 
	   
	   This can result in some unnecessary recalculation and redrawing
	   overhead, and therefore we should only use this two-phase mode
	   of calculation when it's really needed (continuous wrap + variable
	   font width). */
    }
    mNLinesDeleted = nLines;
    mSuppressResync = 1;
}

/*
** Count forward from startPos to either maxPos or maxLines (whichever is
** reached first), and return all relevant positions and line count.
** The provided textBuffer may differ from the actual text buffer of the
** widget. In that case it must be a (partial) copy of the actual text buffer
** and the styleBufOffset argument must indicate the starting position of the
** copy, to take into account the correct style information.
**
** Returned values:
**
**   retPos:	    Position where counting ended.  When counting lines, the
**  	    	    position returned is the start of the line "maxLines"
**  	    	    lines beyond "startPos".
**   retLines:	    Number of line breaks counted
**   retLineStart:  Start of the line where counting ended
**   retLineEnd:    End position of the last line traversed
*/
void Fl_Text_Display::wrapped_line_counter(Fl_Text_Buffer *buf, int startPos,
	int maxPos, int maxLines, bool startPosIsLineStart, int styleBufOffset,
	int *retPos, int *retLines, int *retLineStart, int *retLineEnd,
	bool countLastLineMissingNewLine) {
    int lineStart, newLineStart = 0, b, p, colNum, wrapMargin;
    int maxWidth, i, foundBreak, width;
	 bool countPixels;
    int nLines = 0, tabDist = buffer()->tab_distance();
    unsigned char c;
    char nullSubsChar = buffer()->null_substitution_character();
    
    /* If the font is fixed, or there's a wrap margin set, it's more efficient
       to measure in columns, than to count pixels.  Determine if we can count
       in columns (countPixels == False) or must count pixels (countPixels ==
       True), and set the wrap target for either pixels or columns */
    if (mFixedFontWidth != -1 || mWrapMargin != 0) {
    	countPixels = false;
	wrapMargin = mWrapMargin ? mWrapMargin : text_area.w / (mFixedFontWidth + 1);
        maxWidth = INT_MAX;
    } else {
    	countPixels = true;
    	wrapMargin = INT_MAX;
    	maxWidth = text_area.w;
    }
    
    /* Find the start of the line if the start pos is not marked as a
       line start. */
    if (startPosIsLineStart)
	lineStart = startPos;
    else
	lineStart = line_start(startPos);
    
    /*
    ** Loop until position exceeds maxPos or line count exceeds maxLines.
    ** (actually, contines beyond maxPos to end of line containing maxPos,
    ** in case later characters cause a word wrap back before maxPos)
    */
    colNum = 0;
    width = 0;
    for (p=lineStart; p<buf->length(); p++) {
    	c = (unsigned char)buf->character(p);

    	/* If the character was a newline, count the line and start over,
    	   otherwise, add it to the width and column counts */
    	if (c == '\n') {
    	    if (p >= maxPos) {
    		*retPos = maxPos;
    		*retLines = nLines;
    		*retLineStart = lineStart;
    		*retLineEnd = maxPos;
    		return;
    	    }
    	    nLines++;
    	    if (nLines >= maxLines) {
    		*retPos = p + 1;
    		*retLines = nLines;
    		*retLineStart = p + 1;
    		*retLineEnd = p;
    		return;
    	    }
    	    lineStart = p + 1;
    	    colNum = 0;
    	    width = 0;
    	} else {
    	    colNum += Fl_Text_Buffer::character_width(c, colNum, tabDist, nullSubsChar);
    	    if (countPixels)
    	    	width += measure_proportional_character(c, colNum, p+styleBufOffset);
    	}

    	/* If character exceeded wrap margin, find the break point
    	   and wrap there */
    	if (colNum > wrapMargin || width > maxWidth) {
    	    foundBreak = false;
    	    for (b=p; b>=lineStart; b--) {
    	    	c = (unsigned char)buf->character(b);
    	    	if (c == '\t' || c == ' ') {
    	    	    newLineStart = b + 1;
    	    	    if (countPixels) {
    	    	    	colNum = 0;
    	    	    	width = 0;
    	    	    	for (i=b+1; i<p+1; i++) {
    	    	    	    width += measure_proportional_character(
				    buf->character(i), colNum, 
				    i+styleBufOffset);
    	    	    	    colNum++;
    	    	    	}
    	    	    } else
    	    	    	colNum = buf->count_displayed_characters(b+1, p+1);
    	    	    foundBreak = true;
    	    	    break;
    	    	}
    	    }
    	    if (!foundBreak) { /* no whitespace, just break at margin */
    	    	newLineStart = max(p, lineStart+1);
    	    	colNum = Fl_Text_Buffer::character_width(c, colNum, tabDist, nullSubsChar);
    	    	if (countPixels)
   	    	    width = measure_proportional_character(c, colNum, p+styleBufOffset);
    	    }
    	    if (p >= maxPos) {
    		*retPos = maxPos;
    		*retLines = maxPos < newLineStart ? nLines : nLines + 1;
    		*retLineStart = maxPos < newLineStart ? lineStart :
    		    	newLineStart;
    		*retLineEnd = maxPos;
    		return;
    	    }
    	    nLines++;
    	    if (nLines >= maxLines) {
    		*retPos = foundBreak ? b + 1 : max(p, lineStart+1);
    		*retLines = nLines;
    		*retLineStart = lineStart;
    		*retLineEnd = foundBreak ? b : p;
    		return;
    	    }
    	    lineStart = newLineStart;
    	}
    }

    /* reached end of buffer before reaching pos or line target */
    *retPos = buf->length();
    *retLines = nLines;
	 if (countLastLineMissingNewLine && colNum > 0)
      ++(*retLines);
    *retLineStart = lineStart;
    *retLineEnd = buf->length();
}

/*
** Measure the width in pixels of a character "c" at a particular column
** "colNum" and buffer position "pos".  This is for measuring characters in
** proportional or mixed-width highlighting fonts.
**
** A note about proportional and mixed-width fonts: the mixed width and
** proportional font code in nedit does not get much use in general editing,
** because nedit doesn't allow per-language-mode fonts, and editing programs
** in a proportional font is usually a bad idea, so very few users would
** choose a proportional font as a default.  There are still probably mixed-
** width syntax highlighting cases where things don't redraw properly for
** insertion/deletion, though static display and wrapping and resizing
** should now be solid because they are now used for online help display.
*/
int Fl_Text_Display::measure_proportional_character(char c, int colNum, int pos) {
    int charLen, style;
    char expChar[ FL_TEXT_MAX_EXP_CHAR_LEN ];
    Fl_Text_Buffer *styleBuf = mStyleBuffer;
    
    charLen = Fl_Text_Buffer::expand_character(c, colNum, expChar, 
	    buffer()->tab_distance(), buffer()->null_substitution_character());
    if (styleBuf == 0) {
	style = 0;
    } else {
	style = (unsigned char)styleBuf->character(pos);
	if (style == mUnfinishedStyle && mUnfinishedHighlightCB) {
    	    /* encountered "unfinished" style, trigger parsing */
    	    (mUnfinishedHighlightCB)(pos, mHighlightCBArg);
    	    style = (unsigned char)styleBuf->character(pos);
	}
    }
    return string_width(expChar, charLen, style);
}

/*
** Finds both the end of the current line and the start of the next line.  Why?
** In continuous wrap mode, if you need to know both, figuring out one from the
** other can be expensive or error prone.  The problem comes when there's a
** trailing space or tab just before the end of the buffer.  To translate an
** end of line value to or from the next lines start value, you need to know
** whether the trailing space or tab is being used as a line break or just a
** normal character, and to find that out would otherwise require counting all
** the way back to the beginning of the line.
*/
void Fl_Text_Display::find_line_end(int startPos, bool startPosIsLineStart,
    	int *lineEnd, int *nextLineStart) {
    int retLines, retLineStart;
    
    /* if we're not wrapping use more efficient BufEndOfLine */
    if (!mContinuousWrap) {
    	*lineEnd = buffer()->line_end(startPos);
    	*nextLineStart = min(buffer()->length(), *lineEnd + 1);
    	return;
    }
    
    /* use the wrapped line counter routine to count forward one line */
    wrapped_line_counter(buffer(), startPos, buffer()->length(),
    	    1, startPosIsLineStart, 0, nextLineStart, &retLines,
    	    &retLineStart, lineEnd);
    return;
}

/*
** Line breaks in continuous wrap mode usually happen at newlines or
** whitespace.  This line-terminating character is not included in line
** width measurements and has a special status as a non-visible character.
** However, lines with no whitespace are wrapped without the benefit of a
** line terminating character, and this distinction causes endless trouble
** with all of the text display code which was originally written without
** continuous wrap mode and always expects to wrap at a newline character.
**
** Given the position of the end of the line, as returned by TextDEndOfLine
** or BufEndOfLine, this returns true if there is a line terminating
** character, and false if there's not.  On the last character in the
** buffer, this function can't tell for certain whether a trailing space was
** used as a wrap point, and just guesses that it wasn't.  So if an exact
** accounting is necessary, don't use this function.
*/ 
int Fl_Text_Display::wrap_uses_character(int lineEndPos) {
    char c;
    
    if (!mContinuousWrap || lineEndPos == buffer()->length())
    	return 1;
    
    c = buffer()->character(lineEndPos);
    return c == '\n' || ((c == '\t' || c == ' ') &&
    	    lineEndPos + 1 != buffer()->length());
}

/*
** Return true if the selection "sel" is rectangular, and touches a
** buffer position withing "rangeStart" to "rangeEnd"
*/
int Fl_Text_Display::range_touches_selection(Fl_Text_Selection *sel,
   int rangeStart, int rangeEnd) {
    return sel->selected() && sel->rectangular() && sel->end() >= rangeStart &&
    	    sel->start() <= rangeEnd;
}

/*
** Extend the range of a redraw request (from *start to *end) with additional
** redraw requests resulting from changes to the attached style buffer (which
** contains auxiliary information for coloring or styling text).
*/
void Fl_Text_Display::extend_range_for_styles( int *startpos, int *endpos ) {
  Fl_Text_Selection * sel = mStyleBuffer->primary_selection();
  int extended = 0;

  /* The peculiar protocol used here is that modifications to the style
     buffer are marked by selecting them with the buffer's primary Fl_Text_Selection.
     The style buffer is usually modified in response to a modify callback on
     the text buffer BEFORE Fl_Text_Display.c's modify callback, so that it can keep
     the style buffer in step with the text buffer.  The style-update
     callback can't just call for a redraw, because Fl_Text_Display hasn't processed
     the original text changes yet.  Anyhow, to minimize redrawing and to
     avoid the complexity of scheduling redraws later, this simple protocol
     tells the text display's buffer modify callback to extend it's redraw
     range to show the text color/and font changes as well. */
  if ( sel->selected() ) {
    if ( sel->start() < *startpos ) {
      *startpos = sel->start();
      extended = 1;
    }
    if ( sel->end() > *endpos ) {
      *endpos = sel->end();
      extended = 1;
    }
  }

  /* If the Fl_Text_Selection was extended due to a style change, and some of the
     fonts don't match in spacing, extend redraw area to end of line to
     redraw characters exposed by possible font size changes */
  if ( mFixedFontWidth == -1 && extended )
    *endpos = mBuffer->line_end( *endpos ) + 1;
}

// The draw() method.  It tries to minimize what is draw as much as possible.
void Fl_Text_Display::draw(void) {
  // don't even try if there is no associated text buffer!
  if (!buffer()) { draw_box(); return; }

  fl_push_clip(x(),y(),w(),h());	// prevent drawing outside widget area

  // draw the non-text, non-scrollbar areas.
  if (damage() & FL_DAMAGE_ALL) {
//    printf("drawing all (box = %d)\n", box());
    // draw the box()
    int W = w(), H = h();
    draw_box(box(), x(), y(), W, H, color());

    if (mHScrollBar->visible())
      W -= scrollbar_width();
    if (mVScrollBar->visible())
      H -= scrollbar_width();

    // left margin
    fl_rectf(text_area.x-LEFT_MARGIN, text_area.y-TOP_MARGIN,
             LEFT_MARGIN, text_area.h+TOP_MARGIN+BOTTOM_MARGIN,
             color());

    // right margin
    fl_rectf(text_area.x+text_area.w, text_area.y-TOP_MARGIN,
             RIGHT_MARGIN, text_area.h+TOP_MARGIN+BOTTOM_MARGIN,
             color());

    // top margin
    fl_rectf(text_area.x, text_area.y-TOP_MARGIN,
             text_area.w, TOP_MARGIN, color());

    // bottom margin
    fl_rectf(text_area.x, text_area.y+text_area.h,
             text_area.w, BOTTOM_MARGIN, color());

    // draw that little box in the corner of the scrollbars
    if (mVScrollBar->visible() && mHScrollBar->visible())
      fl_rectf(mVScrollBar->x(), mHScrollBar->y(),
               mVScrollBar->w(), mHScrollBar->h(),
               FL_GRAY);

    // blank the previous cursor protrusions
  }
  else if (damage() & (FL_DAMAGE_SCROLL | FL_DAMAGE_EXPOSE)) {
//    printf("blanking previous cursor extrusions at Y: %d\n", mCursorOldY);
    // CET - FIXME - save old cursor position instead and just draw side needed?
    fl_push_clip(text_area.x-LEFT_MARGIN,
                 text_area.y,
                 text_area.w+LEFT_MARGIN+RIGHT_MARGIN,
                 text_area.h);
    fl_rectf(text_area.x-LEFT_MARGIN, mCursorOldY,
             LEFT_MARGIN, mMaxsize, color());
    fl_rectf(text_area.x+text_area.w, mCursorOldY,
             RIGHT_MARGIN, mMaxsize, color());
    fl_pop_clip();
  }

  // draw the scrollbars
  if (damage() & (FL_DAMAGE_ALL | FL_DAMAGE_CHILD)) {
    mVScrollBar->damage(FL_DAMAGE_ALL);
    mHScrollBar->damage(FL_DAMAGE_ALL);
  }
  update_child(*mVScrollBar);
  update_child(*mHScrollBar);

  // draw all of the text
  if (damage() & (FL_DAMAGE_ALL | FL_DAMAGE_EXPOSE)) {
    //printf("drawing all text\n");
    int X, Y, W, H;
    if (fl_clip_box(text_area.x, text_area.y, text_area.w, text_area.h,
                    X, Y, W, H)) {
      // Draw text using the intersected clipping box...
      // (this sets the clipping internally)
      draw_text(X, Y, W, H);
    } else {
      // Draw the whole area...
      draw_text(text_area.x, text_area.y, text_area.w, text_area.h);
    }
  }
  else if (damage() & FL_DAMAGE_SCROLL) {
    // draw some lines of text
    fl_push_clip(text_area.x, text_area.y,
                 text_area.w, text_area.h);
    //printf("drawing text from %d to %d\n", damage_range1_start, damage_range1_end);
    draw_range(damage_range1_start, damage_range1_end);
    if (damage_range2_end != -1) {
      //printf("drawing text from %d to %d\n", damage_range2_start, damage_range2_end);
      draw_range(damage_range2_start, damage_range2_end);
    }
    damage_range1_start = damage_range1_end = -1;
    damage_range2_start = damage_range2_end = -1;
    fl_pop_clip();
  }

  // draw the text cursor
  if (damage() & (FL_DAMAGE_ALL | FL_DAMAGE_SCROLL | FL_DAMAGE_EXPOSE)
      && !buffer()->primary_selection()->selected() &&
      mCursorOn && Fl::focus() == this ) {
    fl_push_clip(text_area.x-LEFT_MARGIN,
                 text_area.y,
                 text_area.w+LEFT_MARGIN+RIGHT_MARGIN,
                 text_area.h);

    int X, Y;
    if (position_to_xy(mCursorPos, &X, &Y)) draw_cursor(X, Y);
//    else puts("position_to_xy() failed - unable to draw cursor!");
    //printf("drew cursor at pos: %d (%d,%d)\n", mCursorPos, X, Y);
    mCursorOldY = Y;
    fl_pop_clip();
  }
  fl_pop_clip();
}

// this processes drag events due to mouse for Fl_Text_Display and
// also drags due to cursor movement with shift held down for
// Fl_Text_Editor
void fl_text_drag_me(int pos, Fl_Text_Display* d) {
  if (d->dragType == Fl_Text_Display::DRAG_CHAR) {
    if (pos >= d->dragPos) {
      d->buffer()->select(d->dragPos, pos);
    } else {
      d->buffer()->select(pos, d->dragPos);
    }
    d->insert_position(pos);
  } else if (d->dragType == Fl_Text_Display::DRAG_WORD) {
    if (pos >= d->dragPos) {
      d->insert_position(d->word_end(pos));
      d->buffer()->select(d->word_start(d->dragPos), d->word_end(pos));
    } else {
      d->insert_position(d->word_start(pos));
      d->buffer()->select(d->word_start(pos), d->word_end(d->dragPos));
    }
  } else if (d->dragType == Fl_Text_Display::DRAG_LINE) {
    if (pos >= d->dragPos) {
      d->insert_position(d->buffer()->line_end(pos)+1);
      d->buffer()->select(d->buffer()->line_start(d->dragPos),
                          d->buffer()->line_end(pos)+1);
    } else {
      d->insert_position(d->buffer()->line_start(pos));
      d->buffer()->select(d->buffer()->line_start(pos),
                          d->buffer()->line_end(d->dragPos)+1);
    }
  }
}

// This timer event scrolls the text view proportionally to
// how far the mouse pointer has left the text area. This 
// allows for smooth scrolling without "wiggeling" the mouse.
void Fl_Text_Display::scroll_timer_cb(void *user_data) {
  Fl_Text_Display *w = (Fl_Text_Display*)user_data;
  int pos;
  switch (scroll_direction) {
  case 1: // mouse is to the right, scroll left
    w->scroll(w->mTopLineNum, w->mHorizOffset + scroll_amount);
    pos = w->xy_to_position(w->text_area.x + w->text_area.w, scroll_y, CURSOR_POS);
    break;
  case 2: // mouse is to the left, scroll right
    w->scroll(w->mTopLineNum, w->mHorizOffset + scroll_amount);
    pos = w->xy_to_position(w->text_area.x, scroll_y, CURSOR_POS);
    break;
  case 3: // mouse is above, scroll down
    w->scroll(w->mTopLineNum + scroll_amount, w->mHorizOffset);
    pos = w->xy_to_position(scroll_x, w->text_area.y, CURSOR_POS);
    break;
  case 4: // mouse is below, scroll up
    w->scroll(w->mTopLineNum + scroll_amount, w->mHorizOffset);
    pos = w->xy_to_position(scroll_x, w->text_area.y + w->text_area.h, CURSOR_POS);
    break;
  default:
    return;
  }
  fl_text_drag_me(pos, w);
  Fl::repeat_timeout(.1, scroll_timer_cb, user_data);
}


int Fl_Text_Display::handle(int event) {
  if (!buffer()) return 0;
  // This isn't very elegant!
  if (!Fl::event_inside(text_area.x, text_area.y, text_area.w, text_area.h) &&
      !dragging && event != FL_LEAVE && event != FL_ENTER &&
      event != FL_MOVE && event != FL_FOCUS && event != FL_UNFOCUS &&
      event != FL_KEYBOARD && event != FL_KEYUP) {
    return Fl_Group::handle(event);
  }

  switch (event) {
    case FL_ENTER:
    case FL_MOVE:
      if (active_r()) {
        if (Fl::event_inside(text_area.x, text_area.y, text_area.w,
	                     text_area.h)) window()->cursor(FL_CURSOR_INSERT);
	else window()->cursor(FL_CURSOR_DEFAULT);
	return 1;
      } else {
        return 0;
      }

    case FL_LEAVE:
    case FL_HIDE:
      if (active_r() && window()) {
        window()->cursor(FL_CURSOR_DEFAULT);

	return 1;
      } else {
	return 0;
      }

    case FL_PUSH: {
	if (active_r() && window()) {
	  if (Fl::event_inside(text_area.x, text_area.y, text_area.w,
	                       text_area.h)) window()->cursor(FL_CURSOR_INSERT);
	  else window()->cursor(FL_CURSOR_DEFAULT);
	}

	if (Fl::focus() != this) {
	  Fl::focus(this);
	  handle(FL_FOCUS);
	}
        if (Fl_Group::handle(event)) return 1;
        if (Fl::event_state()&FL_SHIFT) return handle(FL_DRAG);
        dragging = 1;
        int pos = xy_to_position(Fl::event_x(), Fl::event_y(), CURSOR_POS);
        dragType = Fl::event_clicks();
        dragPos = pos;
        if (dragType == DRAG_CHAR)
          buffer()->unselect();
        else if (dragType == DRAG_WORD)
          buffer()->select(word_start(pos), word_end(pos));
        else if (dragType == DRAG_LINE)
          buffer()->select(buffer()->line_start(pos), buffer()->line_end(pos)+1);

        if (buffer()->primary_selection()->selected())
          insert_position(buffer()->primary_selection()->end());
        else
          insert_position(pos);
        show_insert_position();
        return 1;
      }

    case FL_DRAG: {
        if (dragType < 0) return 1;
        int X = Fl::event_x(), Y = Fl::event_y(), pos;
        // if we leave the text_area, we start a timer event
        // that will take care of scrolling and selecting
        if (Y < text_area.y) {
          scroll_x = X;
          scroll_amount = (Y - text_area.y) / 5 - 1;
          if (!scroll_direction) {
            Fl::add_timeout(.01, scroll_timer_cb, this);
          }
          scroll_direction = 3;
        } else if (Y >= text_area.y+text_area.h) {
          scroll_x = X;
          scroll_amount = (Y - text_area.y - text_area.h) / 5 + 1;
          if (!scroll_direction) {
            Fl::add_timeout(.01, scroll_timer_cb, this);
          }
          scroll_direction = 4;
        } else if (X < text_area.x) {
          scroll_y = Y;
          scroll_amount = (X - text_area.x) / 2 - 1;
          if (!scroll_direction) {
            Fl::add_timeout(.01, scroll_timer_cb, this);
          }
          scroll_direction = 2;
        } else if (X >= text_area.x+text_area.w) {
          scroll_y = Y;
          scroll_amount = (X - text_area.x - text_area.w) / 2 + 1;
          if (!scroll_direction) {
            Fl::add_timeout(.01, scroll_timer_cb, this);
          }
          scroll_direction = 1;
        } else {
          if (scroll_direction) {
            Fl::remove_timeout(scroll_timer_cb, this);
            scroll_direction = 0;
          }
          pos = xy_to_position(X, Y, CURSOR_POS);
          fl_text_drag_me(pos, this);
        }
        return 1;
      }

    case FL_RELEASE: {
        dragging = 0;
        if (scroll_direction) {
          Fl::remove_timeout(scroll_timer_cb, this);
          scroll_direction = 0;
        }

        // convert from WORD or LINE selection to CHAR
        if (insert_position() >= dragPos)
          dragPos = buffer()->primary_selection()->start();
        else
          dragPos = buffer()->primary_selection()->end();
        dragType = DRAG_CHAR;

        const char* copy = buffer()->selection_text();
        if (*copy) Fl::copy(copy, strlen(copy), 0);
        free((void*)copy);
        return 1;
      }

    case FL_MOUSEWHEEL:
      if (Fl::event_dy()) return mVScrollBar->handle(event);
      else return mHScrollBar->handle(event);

    case FL_UNFOCUS:
      if (active_r() && window()) window()->cursor(FL_CURSOR_DEFAULT);
    case FL_FOCUS:
      if (buffer()->selected()) {
        int start, end;
        if (buffer()->selection_position(&start, &end))
          redisplay_range(start, end);
      }
      if (buffer()->secondary_selected()) {
        int start, end;
        if (buffer()->secondary_selection_position(&start, &end))
          redisplay_range(start, end);
      }
      if (buffer()->highlight()) {
        int start, end;
        if (buffer()->highlight_position(&start, &end))
          redisplay_range(start, end);
      }
      return 1;

    case FL_KEYBOARD:
      // Copy?
      if ((Fl::event_state()&(FL_CTRL|FL_COMMAND)) && Fl::event_key()=='c') {
          if (!buffer()->selected()) return 1;
          const char *copy = buffer()->selection_text();
          if (*copy) Fl::copy(copy, strlen(copy), 1);
          free((void*)copy);
          return 1;
      }

      // Select all ?
      if ((Fl::event_state()&(FL_CTRL|FL_COMMAND)) && Fl::event_key()=='a') {
          buffer()->select(0,buffer()->length());
          return 1;
      }

      if (mVScrollBar->handle(event)) return 1;
      if (mHScrollBar->handle(event)) return 1;

      break;
  }

  return 0;
}


//
// End of "$Id: Fl_Text_Display.cxx 6105 2008-04-21 21:03:22Z matt $".
//
