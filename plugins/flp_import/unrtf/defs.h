
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
 * Module name:    defs.h
 * Author name:    Zachary Smith
 * Create date:    1 Sept 2000
 * Purpose:        Basic definitions plus externs for UnRTF
 *----------------------------------------------------------------------
 * Changes:
 * 21 Oct 00, tuorfa@yahoo.com: moved program version to this file
 * 08 Apr 01, tuorfa@yahoo.com: updated usage info.
 * 08 Sep 01, tuorfa@yahoo.com: added UnRTF.
 * 19 Sep 01, tuorfa@yahoo.com: added PROGRAM_WEBSITE.
 * 09 Oct 03, daved@physiol.usyd.edu.au: changed to GNU website
 * 17 Feb 04, marcossamaral@terra.com.br: changed some information
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 * 17 Dec 07, daved@physiol.usyd.edu.au: added --noremap to usage - from
 *		David Santinoli
 *--------------------------------------------------------------------*/


#define PROGRAM_WEBSITE "http://www.gnu.org/software/unrtf/unrtf.html"


/* Select the language for reporting of file creation/modificaton dates */
#define ENGLISH
#if 0
#define FRANCAIS
#define ITALIANO
#define PORTUGUES /* amaral - 0.19.4 */
#endif


#ifndef true /* daved 0.19.0 */
#define true (1)
#endif
#ifndef false /* daved 0.19.0 */
#define false (0)
#endif
#if 1 /* daved - 0.19.4 */
#define SKIP_ONE_WORD	2
#endif


#define USAGE "unrtf [--version] [--verbose] [--help] [--nopict|-n] [--noremap] [--html] [--text] [--vt] [--latex] [-t html|text|vt|latex] <filename>"


/* Default names for RTF's default fonts */
#define FONTNIL_STR	"Times,TimesRoman,TimesNewRoman"
#define FONTROMAN_STR	"Times,Palatino"
#define FONTSWISS_STR	"Helvetica,Arial"
#define FONTMODERN_STR	"Courier,Verdana"
#define FONTSCRIPT_STR	"Cursive,ZapfChancery"
#define FONTDECOR_STR	"ZapfChancery"
#define FONTTECH_STR	"Symbol"

