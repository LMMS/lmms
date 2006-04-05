
/*===========================================================================
   GNU UnRTF, a command-line program to convert RTF documents to other formats.
   Copyright (C) 2000,2001,2004 by Zachary Smith

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

   The maintainer is reachable by electronic mail at daved@physiol.usyd.edu.au
===========================================================================*/


/*----------------------------------------------------------------------
 * Module name:    convert
 * Author name:    Zachary Smith
 * Create date:    19 Sep 2001
 * Purpose:        Definitions for the conversion module
 *----------------------------------------------------------------------
 * Changes:
 * 31 March 2005 by daved@physiol.usyd.edu.au: changes requested by ZT Smith
 *--------------------------------------------------------------------*/


#ifndef _CONVERT

enum {
	CHARSET_ANSI=1,
	CHARSET_MAC,
	CHARSET_CP437,
	CHARSET_CP850
};

#ifndef _WORD
#include "word.h"
#endif

extern void word_print (Word*, QString & _s);

#if 1 /* daved 0.19.6 - support for multiple char number->output tables */
	short numchar_table;
#define FONTROMAN_TABLE 0
#define FONTSYMBOL_TABLE 1
#endif

#define _CONVERT
#endif

