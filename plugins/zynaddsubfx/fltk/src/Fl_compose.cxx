//
// "$Id: Fl_compose.cxx 5211 2006-06-19 07:43:39Z matt $"
//
// Character compose processing for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
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

//
// MRS: Uncomment the following define to get the original (pre-1.1.2)
//      dead key support code.  The original code apparently did not
//      work on Belgian keyboards.
//

//#define OLD_DEAD_KEY_CODE


#ifdef __APPLE__

static const char* const compose_pairs =
":A*A,C'E~N:O:U'a`a^a:a~a*a,c'e`e"
"^e:e'i`i^i:i~n'o`o^o:o~o'u`u^u:u"
"+ o /c# SS* P|ssrOcOTM' : !=AE/O"
"oo+-<=>=Y=mudtSgPipiS a dgOmaeo/"
"? ! !!v-f ~~Dt<<>>..  `A~A~OOEoe"
"- --''``\"'\"`:-^V:y:Y//E=< > fifl"
"++..,,_\"%%^A^E'A:E`E'I^I:I`I'O^O"
"mc`O'U^U`U||^ ~^_ u . * , ~-; v ";

#else

static const char* const compose_pairs =
"=E  _'f _\"..+ ++^ %%^S< OE  ^Z    ^''^^\"\"^-*- --~ TM^s> oe  ^z:Y" 
"  ! % # $ y=| & : c a <<~ - r _ * +-2 3 ' u p . , 1 o >>141234? "
"`A'A^A~A:A*AAE,C`E'E^E:E`I'I^I:I-D~N`O'O^O~O:Ox O/`U'U^U:U'YTHss"
"`a'a^a~a:a*aae,c`e'e^e:e`i'i^i:i-d~n`o'o^o~o:o-:o/`u'u^u:u'yth:y";

#endif

#if !defined(WIN32) && defined(OLD_DEAD_KEY_CODE) // X only
// X dead-key lookup table.  This turns a dead-key keysym into the
// first of two characters for one of the compose sequences.  These
// keysyms start at 0xFE50.
// Win32 handles the dead keys before FLTK can see them.  This is
// unfortunate, because you don't get the preview effect.
static char dead_keys[] = {
  '`',	// XK_dead_grave
  '\'',	// XK_dead_acute
  '^',	// XK_dead_circumflex
  '~',	// XK_dead_tilde
  '_',	// XK_dead_macron
  0,	// XK_dead_breve
  '.',	// XK_dead_abovedot
  ':',	// XK_dead_diaeresis
  '*',	// XK_dead_abovering
  0,	// XK_dead_doubleacute
  'v',	// XK_dead_caron
  ','	// XK_dead_cedilla
//   0,	// XK_dead_ogonek
//   0,	// XK_dead_iota
//   0,	// XK_dead_voiced_sound
//   0,	// XK_dead_semivoiced_sound
//   0	// XK_dead_belowdot
};
#endif // !WIN32 && OLD_DEAD_KEY_CODE

int Fl::compose_state = 0;

int Fl::compose(int& del) {

  del = 0;
  unsigned char ascii = (unsigned)e_text[0];

  // Alt+letters are reserved for shortcuts.  But alt+foreign letters
  // has to be allowed, because some key layouts require alt to be held
  // down in order to type them...
  //
  // OSX users sometimes need to hold down ALT for keys, so we only check
  // for META on OSX...
#ifdef __APPLE__
  if ((e_state & FL_META) && !(ascii & 128)) return 0;
#else
  if ((e_state & (FL_ALT|FL_META)) && !(ascii & 128)) return 0;
#endif // __APPLE__

  if (compose_state == 1) { // after the compose key
    if ( // do not get distracted by any modifier keys
      e_keysym==FL_Shift_L||
      e_keysym==FL_Shift_R ||
      e_keysym==FL_Alt_L ||
      e_keysym==FL_Alt_R ||
      e_keysym==FL_Meta_L ||
      e_keysym==FL_Meta_R ||
      e_keysym==FL_Control_R ||
      e_keysym==FL_Control_L ||
      e_keysym==FL_Menu
      ) return 0;

    if (ascii == ' ') { // space turns into nbsp
#ifdef __APPLE__
      e_text[0] = char(0xCA);
#else
      e_text[0] = char(0xA0);
#endif
      compose_state = 0;
      return 1;
    } else if (ascii < ' ' || ascii == 127) {
      compose_state = 0;
      return 0;
    }

    // see if it is either character of any pair:
    for (const char *p = compose_pairs; *p; p += 2) 
      if (p[0] == ascii || p[1] == ascii) {
	if (p[1] == ' ') e_text[0] = (p-compose_pairs)/2+0x80;
	compose_state = ascii;
	return 1;
      }

    if (e_length) { // compose key also "quotes" control characters
      compose_state = 0;
      return 1;
    }

  } else if (compose_state) { // second character of compose

    char c1 = char(compose_state); // retrieve first character
#ifdef __APPLE__
    if ( (c1==0x60 && ascii==0xab) || (c1==0x27 && ascii==0x60)) {
      del = 1;
      compose_state = '^';
      e_text[0] = 0xf6;
      return 1;
    }
    if (ascii==' ') {
      del = 0;
      compose_state = 0;
      return 0;
    }
#endif
    // now search for the pair in either order:
    for (const char *p = compose_pairs; *p; p += 2) {
      if (p[0] == ascii && p[1] == c1 || p[1] == ascii && p[0] == c1) {
	e_text[0] = (p-compose_pairs)/2+0x80;
	del = 1; // delete the old character and insert new one
	compose_state = 0;
	return 1;
      }
    }

  }

  int i = e_keysym;

  // See if they type the compose prefix key:
  if (i == FL_Control_R || i == 0xff20/* Multi-Key */) {
    compose_state = 1;
    return 1;
  }

#ifdef WIN32
#elif (defined __APPLE__)
  if (e_state & 0x40000000) {
    if (ascii<0x80)
      compose_state = ascii;
    else
      compose_state = compose_pairs[(ascii-0x80)*2];
    return 1;
  }
#else
  // See if they typed a dead key.  This gets it into the same state as
  // typing prefix+accent:
  if (i >= 0xfe50 && i <= 0xfe5b) {
#  ifdef OLD_DEAD_KEY_CODE
    ascii = dead_keys[i-0xfe50];
    for (const char *p = compose_pairs; *p; p += 2)
      if (p[0] == ascii) {
	compose_state = ascii;
	return 1;
      }
#  else
    ascii = e_text[0];
    for (const char *p = compose_pairs; *p; p += 2)
      if (p[0] == ascii ||
          (p[1] == ' ' && (p - compose_pairs) / 2 + 0x80 == ascii)) {
        compose_state = p[0];
        return 1;
      }
#  endif // OLD_DEAD_KEY_CODE
    compose_state = 0;
    return 1;
  }
#endif

  // Only insert non-control characters:
  if (e_length && (ascii & ~31 && ascii!=127)) {compose_state = 0; return 1;}

  return 0;
}



