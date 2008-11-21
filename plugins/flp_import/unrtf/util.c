/*=============================================================================
   GNU UnRTF, a command-line program to convert RTF documents to other formats.
   Copyright (C) 2000,2001,2004 by Zachary Smith

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

   The maintainer is reachable by electronic mail at daved@physiol.usyd.edu.au
=============================================================================*/


/*----------------------------------------------------------------------
 * Module name:    util
 * Author name:    Zachary Smith
 * Create date:    01 Aug 01
 * Purpose:        Utility functions.
 *----------------------------------------------------------------------
 * Changes:
 * 22 Sep 01, tuorfa@yahoo.com: added function-level comment blocks 
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 *--------------------------------------------------------------------*/

#ifdef LMMS_HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef LMMS_HAVE_CTYPE_H
#include <ctype.h>
#endif




/*========================================================================
 * Name:	h2toi
 * Purpose:	Converts a 2-digit hexadecimal value to an unsigned integer.
 * Args:	String.
 * Returns:	Integer.
 *=======================================================================*/

/* Convert a two-char hexadecimal expression to an integer */
int
h2toi (char *s) {
	int tmp;
	int ch;
	tmp = tolower(*s++);
	if (tmp>'9') tmp-=('a'-10); 
	else tmp-='0';
	ch=16*tmp;
	tmp = tolower(*s++);
	if (tmp>'9') tmp-=('a'-10); 
	else tmp-='0';
	ch+=tmp;
	return ch;
}

