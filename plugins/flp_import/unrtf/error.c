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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

   The maintainer is reachable by electronic mail at daved@physiol.usyd.edu.au
=============================================================================*/


/*----------------------------------------------------------------------
 * Module name:    error
 * Author name:    Zachary Smith
 * Create date:    01 Sep 00
 * Purpose:        Management of errors and warnings, when reporting
 *                 the source code file/line is not necessary.
 *----------------------------------------------------------------------
 * Changes
 * 10 Oct 00, tuorfa@yahoo.com: added usage()
 * 15 Oct 00, tuorfa@yahoo.com: improved output readability
 * 22 Sep 01, tuorfa@yahoo.com: removed mention of line number in handlers
 * 22 Sep 01, tuorfa@yahoo.com: added function-level comment blocks 
 * 08 Oct 03, daved@physiol.usyd.edu.au: added stdlib.h for linux
 * 25 Sep 04, st001906@hrz1.hrz.tu-darmstadt.de: added stdlib.h for djgpp
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 22 Aug 05, ax2groin@arbornet.org: added lineno to error_handler
 * 31 Oct 07, jasp00@users.sourceforge.net: replaced deprecated conversions
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
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

#include "defs.h"
#include "main.h"



/*========================================================================
 * Name:	usage
 * Purpose:	Prints usage information and exits with an error.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void
usage ()
{
	fprintf(stderr, "Usage: %s\n", USAGE);
	exit(-3);
}



/*========================================================================
 * Name:	error_handler
 * Purpose:	Prints error message and other useful info, then exits.
 * Args:	Message.
 * Returns:	None.
 *=======================================================================*/

void
error_handler (const char* message)
{
#if 1
	fprintf(stderr, "Error (line %d): %s\n", lineno, message);
#else
	fprintf(stderr, "Error: %s\n", message);
#endif
	exit(10);
}


/*========================================================================
 * Name:	warning_handler
 * Purpose:	Prints useful info to stderr, but doesn't exit.
 * Args:	Message.
 * Returns:	None.
 *=======================================================================*/

void
warning_handler (const char* message)
{
	fprintf(stderr, "Warning: %s\n", message);
}
