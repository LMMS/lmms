/* "$Id: $"
 *
 * Author: Jean-Marc Lienher ( http://oksid.ch )
 * Copyright 2000-2003 by O'ksi'D.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems on the following page:
 *
 *     http://www.fltk.org/str.php
 */

/*
 * X11 UTF-8 text drawing functions.
 */
#if !defined(WIN32) && !defined(__APPLE__)

#include "../../FL/Xutf8.h"
#include <X11/Xlib.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* External auto generated functions : */
#include "ucs2fontmap.c"
/* 
 * extern int ucs2fontmap(char *s, unsigned int ucs, int enc);
 * extern int encoding_number(const char *enc);
 * extern const char *encoding_name(int num);
 */

/*********************************************************************/
/** extract a list of font from the base font name list             **/
/*********************************************************************/
static int 
get_font_list(
	const char	*base_font_name_list, 
	char 		***flist) {
  const char *ptr;
  const char *p;
  int nb;
  int nb_name;
  
  ptr = base_font_name_list;
  p = NULL;
  nb = 0;
  nb_name = 1;

  while (*ptr) {
    if (*ptr == ',') nb_name++;
    ptr++;
  }

  *flist = (char **) malloc(sizeof(char*) * nb_name);
  ptr = base_font_name_list;

  while (*ptr) {
    int l = 0, i = 0;

    while(isspace(*ptr)) ptr++;
    p = ptr;
    while (*ptr && *ptr != ',') { ptr++; l++; }
    if (l > 2) {
      (*flist)[nb] = (char*) malloc((unsigned)l + 2);
      while (p != ptr) { ((*flist)[nb])[i] = *p; i++; p++; }
      (*flist)[nb][i] = '\0';
      nb++;
    }
    if (*ptr) ptr++;
  }
  if (nb < 1) {
    free(*flist);
    *flist = (char**)NULL;
  }
  return nb;	
}

/*********************************************************************/
/** get the font name used as encoding for "fontspecific" encoding  **/
/** (mainly used for adobe-symbol and adobe-zapfdingbats)	    **/
/*********************************************************************/
static int 
font_spec_enc(char *font) {
  int ret;
  char *enc;
  char *end;

  enc = font;
  while (*enc != '-') enc++; 
  enc++;
  while (*enc != '-') enc++;
  enc++;
  end = enc;
  while (*end != '-') end++;
  *end = '\0';
  
  ret = encoding_number(enc);
  *end = '-';

  return ret;
}


/*********************************************************************/
/** get the sub range of a iso10646-1 font			    **/
/*********************************************************************/
static void
get_range(const char	*enc,
	  int 		*min,
	  int		*max) {

  const char *ptr = enc;
  const char *ptr1;

  if (!enc) return;

  while (*ptr && *ptr != '-') ptr++;
  if (!*ptr) return;
  while (*ptr && *ptr != '[') ptr++;
  if (!*ptr) return;
  *min = 0xFFFF;
  *max = 0;
  while (*ptr && *ptr != ']') {
    int val;
    ptr++;
    ptr1 = ptr;
    while (*ptr && *ptr != ']' && *ptr != ' ' && *ptr != '_') ptr++;
    val = strtol(ptr1, NULL, 0);
    if (val < *min) *min = val;
    if (val > *max) *max = val;
  }	
}

/*********************************************************************/
/** get the internal encoding number of each fonts 		    **/
/*********************************************************************/
static int *
get_encodings(char	**font_name_list, 
	      int 	*ranges,
	      int 	nb_font) {

  int *font_encoding_list;
  int i;
  i = 0;

  font_encoding_list = (int *) malloc(sizeof(int) * nb_font);
  while (i < nb_font) {
    char *ptr;
    int ec;
    ptr = font_name_list[i];
    ec = 0;
    font_encoding_list[i] = -1;
    ranges[i * 2] = 0;
    ranges[i * 2 + 1] = 0xFFFF;
    
    if (ptr && strstr(ptr, "fontspecific")) {
      font_encoding_list[i] = font_spec_enc(ptr);
      ptr = NULL;
    }
    while (ptr && *ptr) {
      if (*ptr == '-') {
	ec++;
	if (ec == 13) {
	  font_encoding_list[i] = encoding_number(ptr + 1);
	  if (font_encoding_list[i] == 0) {
	    get_range(ptr + 1, 
		      ranges + i * 2,
		      ranges + i * 2 + 1);
	  }
	  break;
	}
      }	
      ptr++;
    }
    if (font_encoding_list[i] < 0) font_encoding_list[i] = 1;
    i++;
  }
  return font_encoding_list;
}

/*********************************************************************/
/** find the first font which matches the name and load it.	    **/
/*********************************************************************/
XFontStruct *
find_best_font(Display  *dpy,
	       char     **name) {

  char **list;
  int cnt;
  XFontStruct *s;

  list = XListFonts(dpy, *name, 1, &cnt);
  if (cnt && list) {
    free(*name);
    *name = strdup(list[0]);
    s = XLoadQueryFont(dpy, *name);
    XFreeFontNames(list);
    return s;
  }
  return NULL;
}

/*********************************************************************/
/** load all fonts 						    **/
/*********************************************************************/
static void 
load_fonts(Display 	   *dpy, 
	   XUtf8FontStruct *font_set) {

  int i;
  char **list;

  i = 0;
  list = NULL;

  font_set->fonts = (XFontStruct**) malloc(sizeof(XFontStruct*) *
                                           font_set->nb_font);
  
  font_set->ranges = (int*) malloc(sizeof(int) * 
                                   font_set->nb_font * 2);

  font_set->descent = 0;
  font_set->ascent = 0;
  font_set->fid = 0;

  while (i < font_set->nb_font) {
    XFontStruct *fnt;

    fnt = font_set->fonts[i] = 
      find_best_font(dpy, &(font_set->font_name_list[i]));
    if (fnt) {
      font_set->fid = fnt->fid;
      if (fnt->ascent > font_set->ascent) {
	font_set->ascent = fnt->ascent;
      }
      if (fnt->descent > font_set->descent) {
	font_set->descent = fnt->descent;
      }
    } else {
      free(font_set->font_name_list[i]);
      font_set->font_name_list[i] = NULL;
    }
    i++;
  }

  font_set->encodings = 
    get_encodings(font_set->font_name_list, 
      font_set->ranges, font_set->nb_font);

  /* unload fonts with same encoding */
  for (i = 0; i < font_set->nb_font; i++) {
    if (font_set->font_name_list[i]) {
      int j;
      for (j = 0; j < i; j++) {
	if (font_set->font_name_list[j] &&
	    font_set->encodings[j] ==
	    font_set->encodings[i] &&
	    font_set->ranges[2*j] ==
	    font_set->ranges[2*i] &&
	    font_set->ranges[(2*j)+1] &&
	    font_set->ranges[(2*i)+1]) {
	  XFreeFont(dpy, font_set->fonts[i]);
	  free(font_set->font_name_list[i]);
	  font_set->font_name_list[i] = NULL;
	  font_set->fonts[i] = 0;
	}
      }
    }
  } 
}

/*********************************************************************/
/** Creates an array of XFontStruct acording to the comma separated  **/
/** list of fonts. XLoad all fonts.				    **/
/*********************************************************************/
XUtf8FontStruct *
XCreateUtf8FontStruct(Display    *dpy,
		      const char *base_font_name_list) {

  XUtf8FontStruct *font_set;
  
  font_set = (XUtf8FontStruct*)malloc(sizeof(XUtf8FontStruct));

  if (!font_set) {
    return NULL;
  }

  font_set->nb_font = get_font_list(base_font_name_list, 
				    &font_set->font_name_list);

  if (font_set->nb_font < 1) {
    free(font_set);
    return NULL;
  }

  load_fonts(dpy, font_set);

  return font_set;
}


/*****************************************************************************/
/** draw a Right To Left UTF-8 string using multiple fonts as needed.	    **/
/*****************************************************************************/
void 
XUtf8DrawRtlString(Display 		*display, 
		   Drawable 		d,
		   XUtf8FontStruct 	*font_set, 
		   GC 			gc, 
		   int 			x, 
		   int 			y, 
		   const char		*string,
		   int 			num_bytes) {

  int 		*encodings;	/* encodings array */
  XFontStruct 	**fonts;	/* fonts array */
  XChar2b 	buf[128];	/* drawing buffer */
  XChar2b	*ptr;		/* pointer to the drawing buffer */
  int 		fnum;		/* index of the current font in the fonts array*/
  int 		i;		/* current byte in the XChar2b buffer */
  int 		first;		/* first valid font index */
  int 		last_fnum;	/* font index of the previous char */
  int 		nb_font;	/* quantity of fonts in the font array */
  char 		glyph[2];	/* byte1 and byte1 value of the UTF-8 char */
  int		*ranges;	/* sub range of iso10646 */

  nb_font = font_set->nb_font;

  if (nb_font < 1) {
    /* there is no font in the font_set :-( */
    return;
  }
  
  ranges = font_set->ranges;
  fonts = font_set->fonts;
  encodings = font_set->encodings;
  i = 0;
  fnum = 0;
  ptr = buf + 128;
  
  while(fnum < nb_font && !fonts[fnum]) fnum++;
  if (fnum >= nb_font) {
    /* there is no valid font for the X server */
    return;
  }

  first = fnum;
  last_fnum = fnum;

  while (num_bytes > 0) {
    int 	 ulen;   /* byte length of the UTF-8 char */
    unsigned int ucs;    /* Unicode value of the UTF-8 char */
    unsigned int no_spc; /* Spacing char equivalent of a non-spacing char */

    if (i > 120) {
      /*** draw the buffer **/
      XSetFont(display, gc, fonts[fnum]->fid);
      x -= XTextWidth16(fonts[fnum], ptr, i);
      XDrawString16(display, d, gc, x, y, ptr, i);
      i = 0;
      ptr = buf + 128;
    }

    ulen = XFastConvertUtf8ToUcs((unsigned char*)string, num_bytes, &ucs); 

    if (ulen < 1) ulen = 1; 

    no_spc = XUtf8IsNonSpacing(ucs);
    if (no_spc) ucs = no_spc; 

    /* 
     * find the first encoding which can be used to 	
     * draw the glyph 				
     */
    fnum = first;
    while (fnum < nb_font) {
      if (fonts[fnum] && ucs2fontmap(glyph, ucs, encodings[fnum]) >= 0) {
	if (encodings[fnum] != 0 || 
	    (ucs >= ranges[fnum * 2] && ucs <= ranges[fnum * 2 + 1])) {
	  break;
	}
      }
      fnum++;
    }
    if (fnum == nb_font) {
      /* the char is not valid in all encodings ->
       * draw it using the first font :-(
       */
      fnum = first;
      ucs2fontmap(glyph, '?', encodings[fnum]);
    }

    if (last_fnum != fnum || no_spc) {
      XSetFont(display, gc, fonts[last_fnum]->fid);
      x -= XTextWidth16(fonts[last_fnum], ptr, i);
      XDrawString16(display, d, gc, x, y, ptr, i);
      i = 0;
      ptr = buf + 127;
      (*ptr).byte1 = glyph[0];
      (*ptr).byte2 = glyph[1];
      if (no_spc) {
	x += XTextWidth16(fonts[fnum], ptr, 1);
      }
    } else {
      ptr--;
      (*ptr).byte1 = glyph[0];
      (*ptr).byte2 = glyph[1];
    }
    last_fnum = fnum;
    i++;
    string += ulen;
    num_bytes -= ulen;
  }

  if (i < 1) return;

  XSetFont(display, gc, fonts[fnum]->fid);
  x -= XTextWidth16(fonts[last_fnum], ptr, i);
  XDrawString16(display, d, gc, x, y, ptr, i);
}


/*****************************************************************************/
/** draw an UTF-8 string using multiple fonts as needed.		    **/
/*****************************************************************************/
void 
XUtf8DrawString(Display 	*display, 
	        Drawable 	d,
                XUtf8FontStruct *font_set, 
	        GC 		gc, 
	        int 		x, 
	        int 		y, 
	        const char	*string,
                int 		num_bytes) {

  int 		*encodings; /* encodings array */
  XFontStruct 	**fonts;    /* fonts array */
  XChar2b 	buf[128];   /* drawing buffer */
  int 		fnum;       /* index of the current font in the fonts array*/
  int 		i;          /* current byte in the XChar2b buffer */
  int 		first;      /* first valid font index */
  int 		last_fnum;  /* font index of the previous char */
  int 		nb_font;    /* quantity of fonts in the font array */
  char 		glyph[2];   /* byte1 and byte1 value of the UTF-8 char */
  int		*ranges;    /* sub range of iso10646 */

  nb_font = font_set->nb_font;

  if (nb_font < 1) {
    /* there is no font in the font_set :-( */
    return;
  }
  ranges = font_set->ranges;
  fonts = font_set->fonts;
  encodings = font_set->encodings;
  i = 0;
  fnum = 0;
  
  while(fnum < nb_font && !fonts[fnum]) fnum++;
  if (fnum >= nb_font) {
    /* there is no valid font for the X server */
    return;
  }

  first = fnum;
  last_fnum = fnum;

  while (num_bytes > 0) {
    int 	 ulen;   /* byte length of the UTF-8 char */
    unsigned int ucs;    /* Unicode value of the UTF-8 char */
    unsigned int no_spc; /* Spacing char equivalent of a non-spacing char */

    if (i > 120) {
      /*** draw the buffer **/
      XSetFont(display, gc, fonts[fnum]->fid);
      XDrawString16(display, d, gc, x, y, buf, i);
      x += XTextWidth16(fonts[fnum], buf, i);
      i = 0;
    }

    ulen = XFastConvertUtf8ToUcs((unsigned char*)string, num_bytes, &ucs); 

    if (ulen < 1) ulen = 1; 

    no_spc = XUtf8IsNonSpacing(ucs);
    if (no_spc) ucs = no_spc; 

    /* 
     * find the first encoding which can be used to 	
     * draw the glyph 				
     */
    fnum = first;
    while (fnum < nb_font) {
      if (fonts[fnum] && ucs2fontmap(glyph, ucs, encodings[fnum]) >= 0) {
	if (encodings[fnum] != 0 || 
	    (ucs >= ranges[fnum * 2] &&
	    ucs <= ranges[fnum * 2 + 1])) {
	  break;
	}
      }
      fnum++;
    }
    if (fnum == nb_font) {
      /* the char is not valid in all encodings ->
       * draw it using the first font :-(
       */
      fnum = first;
      ucs2fontmap(glyph, '?', encodings[fnum]);
    }

    if (last_fnum != fnum || no_spc) {
      XSetFont(display, gc, fonts[last_fnum]->fid);
      XDrawString16(display, d, gc, x, y, buf, i);
      x += XTextWidth16(fonts[last_fnum], buf, i);
      i = 0;
      (*buf).byte1 = glyph[0];
      (*buf).byte2 = glyph[1];
      if (no_spc) {
	x -= XTextWidth16(fonts[fnum], buf, 1);
      }
    } else {
      (*(buf + i)).byte1 = glyph[0];
      (*(buf + i)).byte2 = glyph[1];
    }
    last_fnum = fnum;
    i++;
    string += ulen;
    num_bytes -= ulen;
  }

  XSetFont(display, gc, fonts[fnum]->fid);
  XDrawString16(display, d, gc, x, y, buf, i);
}



/*****************************************************************************/
/** returns the pixel width of a UTF-8 string				    **/
/*****************************************************************************/
int  
XUtf8TextWidth(XUtf8FontStruct 	*font_set, 
	       const char 	*string,
	       int 		num_bytes) {

  int		x;
  int 		*encodings; /* encodings array */
  XFontStruct 	**fonts;    /* fonts array */
  XChar2b 	buf[128];   /* drawing buffer */
  int 		fnum;       /* index of the current font in the fonts array*/
  int 		i;          /* current byte in the XChar2b buffer */
  int 		first;      /* first valid font index */
  int 		last_fnum;  /* font index of the previous char */
  int 		nb_font;    /* quantity of fonts in the font array */
  char 		glyph[2];   /* byte1 and byte1 value of the UTF-8 char */
  int		*ranges;    /* sub range of iso10646 */

  nb_font = font_set->nb_font;
  x = 0;

  if (nb_font < 1) {
    /* there is no font in the font_set :-( */
    return x;
  }

  ranges = font_set->ranges;
  fonts = font_set->fonts;
  encodings = font_set->encodings;
  i = 0;
  fnum = 0;
  
  while(fnum < nb_font && !fonts[fnum]) fnum++;
  if (fnum >= nb_font) {
    /* there is no valid font for the X server */
    return x;
  }

  first = fnum;
  last_fnum = fnum;

  while (num_bytes > 0) {
    int 	 ulen;   /* byte length of the UTF-8 char */
    unsigned int ucs;    /* Unicode value of the UTF-8 char */
    unsigned int no_spc; /* Spacing char equivalent of a non-spacing char */

    if (i > 120) {
      /*** measure the buffer **/
      x += XTextWidth16(fonts[fnum], buf, i);
      i = 0;
    }

    ulen = XFastConvertUtf8ToUcs((unsigned char*)string, num_bytes, &ucs); 

    if (ulen < 1) ulen = 1; 

    no_spc = XUtf8IsNonSpacing(ucs);
    if (no_spc) {
      ucs = no_spc;
    }

    /* 
     * find the first encoding which can be used to 	
     * draw the glyph 				
     */
    fnum = first;
    while (fnum < nb_font) {
      if (fonts[fnum] && ucs2fontmap(glyph, ucs, encodings[fnum]) >= 0) {
	if (encodings[fnum] != 0 || 
		(ucs >= ranges[fnum * 2] &&
		ucs <= ranges[fnum * 2 + 1])) {
	  break;
	}
      }
      fnum++;
    }
    if (fnum == nb_font) {
      /* the char is not valid in all encodings ->
       * draw it using the first font :-(
       */
      fnum = first;
      ucs2fontmap(glyph, '?', encodings[fnum]);
    }

    if (last_fnum != fnum || no_spc) {
      x += XTextWidth16(fonts[last_fnum], buf, i);
      i = 0;
      (*buf).byte1 = glyph[0];
      (*buf).byte2 = glyph[1];
      if (no_spc) {
	/* go back to draw the non-spacing char over the previous char */
	x -= XTextWidth16(fonts[fnum], buf, 1);
      }
    } else {
      (*(buf + i)).byte1 = glyph[0];
      (*(buf + i)).byte2 = glyph[1];
    }
    last_fnum = fnum;
    i++;
    string += ulen;
    num_bytes -= ulen;
  }

  x += XTextWidth16(fonts[last_fnum], buf, i);

  return x;
}

/*****************************************************************************/
/**  get the X font and glyph ID of a UCS char                              **/
/*****************************************************************************/
int
XGetUtf8FontAndGlyph(XUtf8FontStruct  *font_set,
		     unsigned int     ucs,
		     XFontStruct      **fnt,
		     unsigned short   *id) {

  int             x;
  int             *encodings; /* encodings array */
  XFontStruct     **fonts;    /* fonts array */
  int             fnum;       /* index of the current font in the fonts array*/
  int             i;          /* current byte in the XChar2b buffer */
  int             first;      /* first valid font index */
  int             last_fnum;  /* font index of the previous char */
  int             nb_font;    /* quantity of fonts in the font array */
  char 		  glyph[2];   /* byte1 and byte1 value of the UTF-8 char */
  int             *ranges;    /* sub range of iso10646 */

  nb_font = font_set->nb_font;
  x = 0;

  if (nb_font < 1) {
    /* there is no font in the font_set :-( */
    return -1;
  }

  ranges = font_set->ranges;
  fonts = font_set->fonts;
  encodings = font_set->encodings;
  i = 0;
  fnum = 0;

  while(fnum < nb_font && !fonts[fnum]) fnum++;
  if (fnum >= nb_font) {
    /* there is no valid font for the X server */
    return -1;
  }

  first = fnum;
  last_fnum = fnum;

  /* 
   * find the first encoding which can be used to         
   * draw the glyph                               
   */
  fnum = first;
  while (fnum < nb_font) {
    if (fonts[fnum] && ucs2fontmap(glyph, ucs, encodings[fnum]) >= 0) {
      if (encodings[fnum] != 0 || 
          (ucs >= ranges[fnum * 2] &&
	  ucs <= ranges[fnum * 2 + 1])) {
	break;
      }
    }
    fnum++;
  }
  if (fnum == nb_font) {
    /* the char is not valid in all encodings ->
     * draw it using the first font :-(
     */
    fnum = first;
    ucs2fontmap(glyph, '?', encodings[fnum]);
  }

  *id = ((unsigned char)glyph[0] << 8) | (unsigned char)glyph[1] ;
  *fnt = fonts[fnum];
  return 0;
}

/*****************************************************************************/
/** returns the pixel width of a UCS char				    **/
/*****************************************************************************/
int
XUtf8UcsWidth(XUtf8FontStruct  *font_set,
	      unsigned int     ucs) {

  int		x;
  int 		*encodings; /* encodings array */
  XFontStruct 	**fonts;    /* fonts array */
  XChar2b 	buf[8];     /* drawing buffer */
  int 		fnum;       /* index of the current font in the fonts array*/
  int 		i;          /* current byte in the XChar2b buffer */
  int 		first;      /* first valid font index */
  int 		last_fnum;  /* font index of the previous char */
  int 		nb_font;    /* quantity of fonts in the font array */
  char 		glyph[2];   /* byte1 and byte1 value of the UTF-8 char */
  int		*ranges;    /* sub range of iso10646 */

  nb_font = font_set->nb_font;
  x = 0;

  if (nb_font < 1) {
    /* there is no font in the font_set :-( */
    return x;
  }

  ranges = font_set->ranges;
  fonts = font_set->fonts;
  encodings = font_set->encodings;
  i = 0;
  fnum = 0;
  
  while(fnum < nb_font && !fonts[fnum]) fnum++;
  if (fnum >= nb_font) {
    /* there is no valid font for the X server */
    return x;
  }

  first = fnum;
  last_fnum = fnum;

  ucs = XUtf8IsNonSpacing(ucs);

  /* 
   * find the first encoding which can be used to 	
   * draw the glyph 				
   */
  fnum = first;
  while (fnum < nb_font) {
    if (fonts[fnum] && 
	ucs2fontmap(glyph, ucs, encodings[fnum]) >= 0) {
      if (encodings[fnum] != 0 || (ucs >= ranges[fnum * 2] &&
	  ucs <= ranges[fnum * 2 + 1])) {
	break;
      }
    }
    fnum++;
  }
  if (fnum == nb_font) {
    /* the char is not valid in all encodings ->
     * draw it using the first font :-(
     */
    fnum = first;
    ucs2fontmap(glyph, '?', encodings[fnum]);
  }

  (*buf).byte1 = glyph[0];
  (*buf).byte2 = glyph[1];

  x += XTextWidth16(fonts[fnum], buf, 1);

  return x;
}

/*****************************************************************************/
/** draw an UTF-8 string and clear the background.	 		    **/
/*****************************************************************************/
void
XUtf8DrawImageString(Display         *display,
		     Drawable        d,
		     XUtf8FontStruct *font_set,
		     GC              gc,
		     int             x,
		     int             y,
		     const char      *string,
		     int             num_bytes) {

  /* FIXME: must be improved ! */
  int w;
  int fill_style;
  unsigned long foreground;
  unsigned long background;
  int function;
  XGCValues xgcv;

  w = XUtf8TextWidth(font_set, string, num_bytes);
  
  XGetGCValues(display, gc, 
	       GCFunction|GCForeground|GCBackground|GCFillStyle, &xgcv);
  
  function = xgcv.function;
  fill_style = xgcv.fill_style;
  foreground = xgcv.foreground;
  background = xgcv.background;

  xgcv.function = GXcopy;
  xgcv.foreground = background;
  xgcv.background = foreground;
  xgcv.fill_style = FillSolid;

  XChangeGC(display, gc,
	    GCFunction|GCForeground|GCBackground|GCFillStyle, &xgcv);

  XFillRectangle(display, d, gc, x, y - font_set->ascent, 
	         (unsigned)w, (unsigned)(font_set->ascent + font_set->descent));

  xgcv.function = function;
  xgcv.foreground = foreground;
  xgcv.background = background;
  xgcv.fill_style = fill_style;

  XChangeGC(display, gc,
	    GCFunction|GCForeground|GCBackground|GCFillStyle, &xgcv);

  XUtf8DrawString(display, d, font_set, gc, x, y, string, num_bytes);
}

/*****************************************************************************/
/** free the XFontSet and others things created by XCreateUtf8FontSet       **/
/*****************************************************************************/
void 
XFreeUtf8FontStruct(Display 	    *dpy, 
		    XUtf8FontStruct *font_set) {

  int i;
  i = 0;
  while (i < font_set->nb_font) {
    if (font_set->fonts[i]) {
	XFreeFont(dpy, font_set->fonts[i]);
	free(font_set->font_name_list[i]);
    }
    i++;
  }
  free(font_set->ranges);
  free(font_set->font_name_list);
  free(font_set->fonts);
  free(font_set->encodings);
  free(font_set);
}

#endif /* X11 only */

/*
 *  End of "$Id: $".
 */
