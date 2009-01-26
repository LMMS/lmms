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
 * Module name:    output
 * Author name:    Zachary Smith
 * Create date:    18 Sep 01
 * Purpose:        Definitions for the generalized output module
 *----------------------------------------------------------------------
 * Changes:
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 16 Dec 07, daved@physiol.usyd.edu.au: updated to GPL v3
 * 31 Oct 07, jasp00@users.sourceforge.net: replaced deprecated conversions
 *--------------------------------------------------------------------*/


#ifndef _OUTPUT

typedef struct {
	int cp;
	unsigned short chars[128];
} CodepageInfo;


typedef struct {
	const char *comment_begin;
	const char *comment_end;

	const char *document_begin;
	const char *document_end;

	const char *header_begin;
	const char *header_end;

	const char *document_title_begin;
	const char *document_title_end;

	char *document_keywords_begin;
	char *document_keywords_end;

	const char *document_author_begin;
	const char *document_author_end;

	const char *document_changedate_begin;
	const char *document_changedate_end;

	const char *body_begin;
	const char *body_end;

	char *word_begin;
	char *word_end;

	const char *paragraph_begin;
	const char *paragraph_end;

	const char *center_begin;
	const char *center_end;

	const char *align_left_begin;
	const char *align_left_end;

	const char *align_right_begin;
	const char *align_right_end;

	const char *justify_begin;
	const char *justify_end;

	const char *forced_space;
	const char *line_break;
	const char *page_break;

	const char *hyperlink_begin;
	const char *hyperlink_end;

	const char *imagelink_begin;
	const char *imagelink_end;

	const char *table_begin;
	const char *table_end;

	const char *table_row_begin;
	const char *table_row_end;

	const char *table_cell_begin;
	const char *table_cell_end;

	/* Character attributes */
	const char *font_begin;
	const char *font_end;

	const char *fontsize_begin;
	const char *fontsize_end;

	/* standard font sizes are optional */
	const char *fontsize8_begin;
	const char *fontsize8_end;

	const char *fontsize10_begin;
	const char *fontsize10_end;

	const char *fontsize12_begin;
	const char *fontsize12_end;

	const char *fontsize14_begin;
	const char *fontsize14_end;

	const char *fontsize18_begin;
	const char *fontsize18_end;

	const char *fontsize24_begin;
	const char *fontsize24_end;

	char *fontsize36_begin;
	char *fontsize36_end;

	char *fontsize48_begin;
	char *fontsize48_end;

	const char *smaller_begin;
	const char *smaller_end;

	const char *bigger_begin;
	const char *bigger_end;

	const char *foreground_begin;
	const char *foreground_end;

	const char *background_begin;
	const char *background_end;

	const char *bold_begin;
	const char *bold_end;

	const char *italic_begin;
	const char *italic_end;

	const char *underline_begin;
	const char *underline_end;

	const char *dbl_underline_begin;
	const char *dbl_underline_end;

	const char *superscript_begin;
	const char *superscript_end;

	const char *subscript_begin;
	const char *subscript_end;

	const char *strikethru_begin;
	const char *strikethru_end;

	const char *dbl_strikethru_begin;
	const char *dbl_strikethru_end;

	const char *emboss_begin;
	const char *emboss_end;

	const char *engrave_begin;
	const char *engrave_end;

	const char *shadow_begin;
	const char *shadow_end;

	const char *outline_begin;
	const char *outline_end;

	char *small_caps_begin;
	char *small_caps_end;

	const char *pointlist_begin;
	const char *pointlist_end;

	const char *pointlist_item_begin;
	const char *pointlist_item_end;

	const char *numericlist_begin;
	const char *numericlist_end;

	const char *numericlist_item_begin;
	const char *numericlist_item_end;

	const char *expand_begin;
	const char *expand_end;

	char *toc_entry_begin;
	char *toc_entry_end;

	char *index_entry_begin;
	char *index_entry_end;

	/* XX These should really be replaced by references
	 * to one of the charsets.
	 */
	struct {
		const char *bullet;
		const char *left_quote;
		const char *right_quote;
		const char *left_dbl_quote;
		const char *right_dbl_quote;
		const char *nonbreaking_space;
		const char *emdash;
		const char *endash;
		const char *lessthan;
		const char *greaterthan;
		const char *amp;
		const char *copyright;
		const char *trademark;
		char *nonbreaking_hyphen;
		char *optional_hyphen;
	} chars;

	const char **ascii_translation_table;

	int simulate_small_caps : 1;
	int simulate_all_caps : 1;
	int simulate_word_underline : 1;

	const char **ansi_translation_table;
	short ansi_first_char;
	short ansi_last_char;
	const char **cp437_translation_table;
	short cp437_first_char;
	short cp437_last_char;
	const char **cp850_translation_table;
	short cp850_first_char;
	short cp850_last_char;
	const char **mac_translation_table;
	short mac_first_char;
	short mac_last_char;
#if 1 /* daved 0.20.0 */
	unsigned int unisymbol1_first_char;
	unsigned int unisymbol1_last_char;
	const char **unisymbol1_translation_table;
	unsigned int unisymbol2_first_char;
	unsigned int unisymbol2_last_char;
	const char **unisymbol2_translation_table;
	unsigned int unisymbol3_first_char;
	unsigned int unisymbol3_last_char;
	const char **unisymbol3_translation_table;
	unsigned int unisymbol4_first_char;
	unsigned int unisymbol4_last_char;
	const char **unisymbol4_translation_table;
#else
#if 1 /* daved 0.19.4 unicode support */
	short unisymbol1_first_char;
	short unisymbol1_last_char;
	char **unisymbol1_translation_table;
	short unisymbol2_first_char;
	short unisymbol2_last_char;
	char **unisymbol2_translation_table;
	short unisymbol3_first_char;
	short unisymbol3_last_char;
	char **unisymbol3_translation_table;
#endif
#if 1 /* daved 0.19.5 more unicode support */
	short unisymbol4_first_char;
	short unisymbol4_last_char;
	char **unisymbol4_translation_table;
#endif
#endif
#if 1 /* daved 0.19.5 SYMBOL font support */
	short symbol_first_char;
	short symbol_last_char;
	const char **symbol_translation_table;
#endif
#if 1 /* daved 0.20.3 GREEK font support */
	short greek_first_char;
	short greek_last_char;
	const char **greek_translation_table;
#endif

	char *(*unisymbol_print) (unsigned short);

	void (*write_set_foreground) (int,int,int);
}
OutputPersonality;


extern OutputPersonality* op_create(void);
extern void op_free (OutputPersonality*);
#if 1 /* daved - 0.19.6 */
extern const char* op_translate_char (OutputPersonality*,int,CodepageInfo*,int, int);
#else
extern char* op_translate_char (OutputPersonality*,int,CodepageInfo*,int);
#endif

extern void op_begin_std_fontsize (OutputPersonality*, int);
extern void op_end_std_fontsize (OutputPersonality*, int);


#define _OUTPUT
#endif

