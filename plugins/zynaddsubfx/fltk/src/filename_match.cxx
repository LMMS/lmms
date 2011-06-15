//
// "$Id: filename_match.cxx 7903 2010-11-28 21:06:39Z matt $"
//
// Pattern matching routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
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

/* Adapted from Rich Salz. */
#include <FL/filename.H>
#include <ctype.h>

/**
    Checks if a string \p s matches a pattern \p p. 
    The following syntax is used for the pattern:
    - * matches any sequence of 0 or more characters.
    - ? matches any single character.
    - [set] matches any character in the set. Set can contain any single characters, or a-z to represent a range. 
      To match ] or - they must be the first characters. To match ^ or ! they must not be the first characters.
    - [^set] or [!set] matches any character not in the set.
    - {X|Y|Z} or {X,Y,Z} matches any one of the subexpressions literally.
    - \\x quotes the character x so it has no special meaning.
    - x all other characters must be matched exactly.

    \b Include:
    \code
    #include <FL/filename.H>
    \endcode

    \param[in] s the string to check for a match
    \param[in] p the string pattern 
    \return non zero if the string matches the pattern
*/
int fl_filename_match(const char *s, const char *p) {
  int matched;

  for (;;) {
    switch(*p++) {

    case '?' :	// match any single character
      if (!*s++) return 0;
      break;

    case '*' :	// match 0-n of any characters
      if (!*p) return 1; // do trailing * quickly
      while (!fl_filename_match(s, p)) if (!*s++) return 0;
      return 1;

    case '[': {	// match one character in set of form [abc-d] or [^a-b]
      if (!*s) return 0;
      int reverse = (*p=='^' || *p=='!'); if (reverse) p++;
      matched = 0;
      char last = 0;
      while (*p) {
	if (*p=='-' && last) {
	  if (*s <= *++p && *s >= last ) matched = 1;
	  last = 0;
	} else {
	  if (*s == *p) matched = 1;
	}
	last = *p++;
	if (*p==']') break;
      }
      if (matched == reverse) return 0;
      s++; p++;}
    break;

    case '{' : // {pattern1|pattern2|pattern3}
    NEXTCASE:
    if (fl_filename_match(s,p)) return 1;
    for (matched = 0;;) {
      switch (*p++) {
      case '\\': if (*p) p++; break;
      case '{': matched++; break;
      case '}': if (!matched--) return 0; break;
      case '|': case ',': if (matched==0) goto NEXTCASE;
      case 0: return 0;
      }
    }
    case '|':	// skip rest of |pattern|pattern} when called recursively
    case ',':
      for (matched = 0; *p && matched >= 0;) {
	switch (*p++) {
	case '\\': if (*p) p++; break;
	case '{': matched++; break;
	case '}': matched--; break;
	}
      }
      break;
    case '}':
      break;

    case 0:	// end of pattern
      return !*s;

    case '\\':	// quote next character
      if (*p) p++;
      /* FALLTHROUGH */
    default:
      if (tolower(*s) != tolower(*(p-1))) return 0;
      s++;
      break;
    }
  }
}

//
// End of "$Id: filename_match.cxx 7903 2010-11-28 21:06:39Z matt $".
//
