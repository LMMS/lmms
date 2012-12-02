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
 * Module name:    hash
 * Author name:    Zachary Smith
 * Create date:    01 Sep 00
 * Purpose:        Word-hash management. Words are put into a hash and an
 *                 identifier is returned. This is used to save us from
 *                 doing multiple mallocs for recurring strings such as
 *                 'the' and \par. This is not a big issue under Unix,
 *                 but it is under other OSes and anyway, waste not want not.
 *----------------------------------------------------------------------
 * Changes:
 * 08 Apr 01, tuorfa@yahoo.com: check for out of memory after malloc.
 * 21 Apr 01, tuorfa@yahoo.com: signed to conversion unsigned bug
 * 03 Aug 01, tuorfa@yahoo.com: fixes for using 16-bit compiler
 * 22 Sep 01, tuorfa@yahoo.com: added function-level comment blocks 
 * 08 Oct 03, daved@physiol.usyd.edu.au: some type fixes
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requsted by ZT Smith
 * 06 Jan 06, marcossamaral@terra.com.br: changes hash_stats function
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 *--------------------------------------------------------------------*/

#ifdef LMMS_HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef LMMS_HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef LMMS_HAVE_STRING_H
#include <string.h>
#endif

#include "error.h"
#include "main.h"
#include "ur_malloc.h"


typedef struct _hi {
	struct _hi *next;
	char *str;
	unsigned long value;
} hashItem;


/* Index by first char of string */
static hashItem *hash2[256];
static unsigned long hash_length[256];
static unsigned long hash_value=0;



/*========================================================================
 * Name:	hash_init
 * Purpose:	Clear the hash table.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void
hash_init ()
{
	int i;
	for (i=0; i<256; i++) {
		hash2[i]=NULL;
		hash_length[i]=0;
	}
}



/*========================================================================
 * Name:	hash_stats
 * Purpose:	Return the number of words stored. This is all words,
 * 			including commands to RTF, NOT the number of printed words in
 * 			a given document.
 * Args:	None.
 * Returns:	Number of words stored.
 *=======================================================================*/

unsigned long
hash_stats ()
{
	int i;
	unsigned long total=0;
	for (i=0; i<256; i++) {
		total += hash_length[i];
	}
	return(total);
}



/*========================================================================
 * Name:	hashitem_new
 * Purpose:	Creates a new linked list item for the hash table.
 * Args:	String.
 * Returns:	hashItem.
 *=======================================================================*/

static hashItem *
hashitem_new (char *str)
{
	hashItem *hi;
	unsigned long i;

	hi=(hashItem*) my_malloc(sizeof(hashItem));
	if (!hi)
		error_handler("Out of memory");
	memset ((void*)hi, 0, sizeof (hashItem));

	hi->str = my_strdup(str);

	i = *str;
	if (i=='\\') i=str[1];
	i <<= 24;
	hi->value = i | (hash_value++ & 0xffffff);
	hi->next = NULL;

#if 0
	if (debug_mode) {
		printf ("<!-- storing val %08lx str %s -->\n",
			hi->value, hi->str);
	}
#endif

	return hi;
}


/*========================================================================
 * Name:	hash_get_index
 * Purpose:	Given a string, returns the "index" i.e. the word identifier.
 * Args:	String.
 * Returns:	Index.
 *=======================================================================*/

unsigned long
hash_get_index (char *str)
{
#if 1 /* daved - 0.19.1 */
	unsigned short index;
	unsigned char ch;
#else
	int index;
	char ch;
#endif
	hashItem *hi;

#if 1 /* daved - 0.19.1 */
	ch = (unsigned char)*str;
#else
	ch = *str;
#endif
	if (ch=='\\' && *(str+1))
		ch = *(str+1); 
	index = ch;
	hi = hash2[index];
	while (hi) {
		if (!strcmp(hi->str,str))
			return hi->value;
		hi=hi->next;
	}
	/* not in hash */
	hi = hashitem_new (str);
	hi->next = hash2[index];
	hash2[index] = hi;
	++hash_length [index];
	return hi->value;
}


/*========================================================================
 * Name:	hash_get_string

 * Purpose:	Given the index (word identifier) returns the word string.
 * Args:	Index.
 * Returns:	String, or NULL if not found.
 *=======================================================================*/

char*
hash_get_string (unsigned long value)
{
	int index;
	hashItem *hi;

	index = value >> 24;
	hi = hash2[index];
	while (hi) {
		if (hi->value == value)
			return hi->str;
		hi=hi->next;
	}
	warning_handler("Word not in hash");
	return NULL;
}
