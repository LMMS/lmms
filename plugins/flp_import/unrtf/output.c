/*=============================================================================
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
=============================================================================*/


/*----------------------------------------------------------------------
 * Module name:    output
 * Author name:    Zachary Smith
 * Create date:    18 Sep 01
 * Purpose:        Generalized output module
 *----------------------------------------------------------------------
 * Changes:
 * 22 Sep 01, tuorfa@yahoo.com: addition of functions to change font size
 * 22 Sep 01, tuorfa@yahoo.com: added function-level comment blocks 
 * 08 Oct 03, daved@physiol.usyd.edu.au: added stdlib.h for linux
 * 25 Sep 04, st001906@hrz1.hrz.tu-darmstadt.de: added stdlib.h for djgpp
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 06 Jan 06, marcossamaral@terra.com.br: changes in STDOUT   
 * 31 Oct 07, jasp00@users.sourceforge.net: replaced deprecated conversions
 *--------------------------------------------------------------------*/


#ifdef LMMS_HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef LMMS_HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef LMMS_HAVE_STRING_H
#include <string.h>
#endif

#include "malloc.h"
#include "defs.h"
#include "error.h"
#include "output.h"
#include "main.h"
#include "convert.h"


extern QString outstring;

/*========================================================================
 * Name:	op_create
 * Purpose:	Creates a blank output personality.
 * Args:	None.
 * Returns:	Output personality struct.
 *=======================================================================*/

OutputPersonality*
op_create ()
{
	OutputPersonality* new_op;

	new_op = (OutputPersonality*) my_malloc (sizeof(OutputPersonality));
	if (!new_op)
		error_handler ("cannot allocate output personality");

	memset ((void*) new_op, 0, sizeof (OutputPersonality));
	return new_op;
}



/*========================================================================
 * Name:	op_free
 * Purpose:	Deallocates an output personality, but none of the strings
 *		it points to since they are usually constants.
 * Args:	OutputPersonality.
 * Returns:	None.
 *=======================================================================*/

void
op_free (OutputPersonality *op)
{
	CHECK_PARAM_NOT_NULL(op);

	my_free ((char*) op);
}




/*========================================================================
 * Name:	op_translate_char
 * Purpose:	Performs a translation of a character in the context of
 *		a given output personality.
 * Args:	OutputPersonality, character set#, character.
 * Returns:	String.
 *=======================================================================*/

const char *
#if 1 /* daved - 0.19.6 */
op_translate_char (OutputPersonality *op, int charset, int ch, int ntable)
#else
op_translate_char (OutputPersonality *op, int charset, int ch)
#endif
{
	short start;
	const char *result=NULL;

	CHECK_PARAM_NOT_NULL(op);

#if 1 /* daved - 0.19.6 */
	/* if we are seeking a character from a symbol font we can
	   be below 0x80
	*/
	if(ntable == FONTSYMBOL_TABLE)
	{
		start = op->symbol_first_char;

		if(ch >= start && ch <= op->symbol_last_char)
			result = op->symbol_translation_table[ch - start];
	}
	else
#endif
	if (ch >= 0x20 && ch < 0x80) {
		result = op->ascii_translation_table [ch - 0x20];
	}
	else
	if (charset != CHARSET_ANSI &&
	    charset != CHARSET_MAC &&
	    charset != CHARSET_CP437 &&
	    charset != CHARSET_CP850)
		error_handler ("invalid character set value, cannot translate character");
	else
	switch (charset) {
	case CHARSET_ANSI:
		start = op->ansi_first_char;
		if (ch >= start &&
		    ch <= op->ansi_last_char)
			result = op->ansi_translation_table [ch-start];
		break;
	case CHARSET_MAC:
		start = op->mac_first_char;
		if (ch >= start &&
		    ch <= op->mac_last_char)
			result = op->mac_translation_table [ch-start];
		break;
	case CHARSET_CP437:
		start = op->cp437_first_char;
		if (ch >= start &&
		    ch <= op->cp437_last_char)
			result = op->cp437_translation_table [ch-start];
		break;
	case CHARSET_CP850:
		start = op->cp850_first_char;
		if (ch >= start &&
		    ch <= op->cp850_last_char)
			result = op->cp850_translation_table [ch-start];
		break;
	}
	return result;
}


/*========================================================================
 * Name:	op_begin_std_fontsize 
 * Purpose:	Prints whatever is necessary to perform a change in the
 *		current font size.
 * Args:	OutputPersonality, desired size.
 * Returns:	None.
 *=======================================================================*/

void
op_begin_std_fontsize (OutputPersonality *op, int size)
{
	size = ( size * 3 ) / 2;
	int found_std_expr = FALSE;

	CHECK_PARAM_NOT_NULL(op);

	/* Look for an exact match with a standard point size.
	 */
	switch (size) {
	case 8:
		if (op->fontsize8_begin) {
			outstring+=QString("%1").arg (op->fontsize8_begin);
			found_std_expr = TRUE;
		}
		break;
	case 10:
		if (op->fontsize10_begin) {
			outstring+=QString("%1").arg (op->fontsize10_begin);
			found_std_expr = TRUE;
		}
		break;
	case 12:
		if (op->fontsize12_begin) {
			outstring+=QString("%1").arg (op->fontsize12_begin);
			found_std_expr = TRUE;
		}
		break;
	case 14:
		if (op->fontsize14_begin) {
			outstring+=QString("%1").arg (op->fontsize14_begin);
			found_std_expr = TRUE;
		}
		break;
	case 18:
		if (op->fontsize18_begin) {
			outstring+=QString("%1").arg (op->fontsize18_begin);
			found_std_expr = TRUE;
		}
		break;
	case 24:
		if (op->fontsize24_begin) {
			outstring+=QString("%1").arg (op->fontsize24_begin);
			found_std_expr = TRUE;
		}
		break;
	case 36:
		if (op->fontsize36_begin) {
			outstring+=QString("%1").arg (op->fontsize36_begin);
			found_std_expr = TRUE;
		}
		break;
	case 48:
		if (op->fontsize48_begin) {
			outstring+=QString("%1").arg (op->fontsize48_begin);
			found_std_expr = TRUE;
		}
		break;
	}

	/* If no exact match, try to write out a change to the
	 * exact point size.
	 */
	if (!found_std_expr) {
		if (op->fontsize_begin) {
			char expr[16];
			sprintf (expr, "%d", size);
			outstring+=QString("%1").arg (op->fontsize_begin, expr);
		} else {
			/* If we cannot write out a change for the exact
			 * point size, we must approximate to a standard
			 * size.
			 */
			if (size<9 && op->fontsize8_begin) {
				outstring+=QString("%1").arg (op->fontsize8_begin);
			} else 
			if (size<11 && op->fontsize10_begin) {
				outstring+=QString("%1").arg (op->fontsize10_begin);
			} else 
			if (size<13 && op->fontsize12_begin) {
				outstring+=QString("%1").arg (op->fontsize12_begin);
			} else 
			if (size<16 && op->fontsize14_begin) {
				outstring+=QString("%1").arg (op->fontsize14_begin);
			} else 
			if (size<21 && op->fontsize18_begin) {
				outstring+=QString("%1").arg (op->fontsize18_begin);
			} else 
			if (size<30 && op->fontsize24_begin) {
				outstring+=QString("%1").arg (op->fontsize24_begin);
			} else 
			if (size<42 && op->fontsize36_begin) {
				outstring+=QString("%1").arg (op->fontsize36_begin);
			} else 
			if (size>40 && op->fontsize48_begin) {
				outstring+=QString("%1").arg (op->fontsize48_begin);
			} else 
			/* If we can't even produce a good approximation,
			 * just try to get a font size near 12 point.
			 */
			if (op->fontsize12_begin)
				outstring+=QString("%1").arg (op->fontsize12_begin);
			else
			if (op->fontsize14_begin)
				outstring+=QString("%1").arg (op->fontsize14_begin);
			else
			if (op->fontsize10_begin)
				outstring+=QString("%1").arg (op->fontsize10_begin);
			else
			if (op->fontsize18_begin)
				outstring+=QString("%1").arg (op->fontsize18_begin);
			else
			if (op->fontsize8_begin)
				outstring+=QString("%1").arg (op->fontsize8_begin);
			else
				error_handler ("output personality lacks sufficient font size change capability");
		}
	}
}


/*========================================================================
 * Name:	op_end_std_fontsize 
 * Purpose:	Prints whatever is necessary to perform a change in the
 *		current font size.
 * Args:	OutputPersonality, desired size.
 * Returns:	None.
 *=======================================================================*/

void
op_end_std_fontsize (OutputPersonality *op, int size)
{
	int found_std_expr = FALSE;

	CHECK_PARAM_NOT_NULL(op);

	/* Look for an exact match with a standard point size.
	 */
	switch (size) {
	case 8:
		if (op->fontsize8_end) {
			outstring+=QString("%1").arg (op->fontsize8_end);
			found_std_expr = TRUE;
		}
		break;
	case 10:
		if (op->fontsize10_end) {
			outstring+=QString("%1").arg (op->fontsize10_end);
			found_std_expr = TRUE;
		}
		break;
	case 12:
		if (op->fontsize12_end) {
			outstring+=QString("%1").arg (op->fontsize12_end);
			found_std_expr = TRUE;
		}
		break;
	case 14:
		if (op->fontsize14_end) {
			outstring+=QString("%1").arg (op->fontsize14_end);
			found_std_expr = TRUE;
		}
		break;
	case 18:
		if (op->fontsize18_end) {
			outstring+=QString("%1").arg (op->fontsize18_end);
			found_std_expr = TRUE;
		}
		break;
	case 24:
		if (op->fontsize24_end) {
			outstring+=QString("%1").arg (op->fontsize24_end);
			found_std_expr = TRUE;
		}
		break;
	case 36:
		if (op->fontsize36_end) {
			outstring+=QString("%1").arg (op->fontsize36_end);
			found_std_expr = TRUE;
		}
		break;
	case 48:
		if (op->fontsize48_end) {
			outstring+=QString("%1").arg (op->fontsize48_end);
			found_std_expr = TRUE;
		}
		break;
	}

	/* If no exact match, try to write out a change to the
	 * exact point size.
	 */
	if (!found_std_expr) {
		if (op->fontsize_end) {
			char expr[16];
			sprintf (expr, "%d", size);
			outstring+=QString("%1").arg (op->fontsize_end, expr);
		} else {
			/* If we cannot write out a change for the exact
			 * point size, we must approximate to a standard
			 * size.
			 */
			if (size<9 && op->fontsize8_end) {
				outstring+=QString("%1").arg (op->fontsize8_end);
			} else 
			if (size<11 && op->fontsize10_end) {
				outstring+=QString("%1").arg (op->fontsize10_end);
			} else 
			if (size<13 && op->fontsize12_end) {
				outstring+=QString("%1").arg (op->fontsize12_end);
			} else 
			if (size<16 && op->fontsize14_end) {
				outstring+=QString("%1").arg (op->fontsize14_end);
			} else 
			if (size<21 && op->fontsize18_end) {
				outstring+=QString("%1").arg (op->fontsize18_end);
			} else 
			if (size<30 && op->fontsize24_end) {
				outstring+=QString("%1").arg (op->fontsize24_end);
			} else 
			if (size<42 && op->fontsize36_end) {
				outstring+=QString("%1").arg (op->fontsize36_end);
			} else 
			if (size>40 && op->fontsize48_end) {
				outstring+=QString("%1").arg (op->fontsize48_end);
			} else 
			/* If we can't even produce a good approximation,
			 * just try to get a font size near 12 point.
			 */
			if (op->fontsize12_end)
				outstring+=QString("%1").arg (op->fontsize12_end);
			else
			if (op->fontsize14_end)
				outstring+=QString("%1").arg (op->fontsize14_end);
			else
			if (op->fontsize10_end)
				outstring+=QString("%1").arg (op->fontsize10_end);
			else
			if (op->fontsize18_end)
				outstring+=QString("%1").arg (op->fontsize18_end);
			else
			if (op->fontsize8_end)
				outstring+=QString("%1").arg (op->fontsize8_end);
			else
				error_handler ("output personality lacks sufficient font size change capability");
		}
	}
}


