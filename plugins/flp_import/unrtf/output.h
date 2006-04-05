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
 * Purpose:        Definitions for the generalized output module
 *----------------------------------------------------------------------
 * Changes:
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 *--------------------------------------------------------------------*/


#ifndef _OUTPUT


typedef struct {
	char *comment_begin;
	char *comment_end;

	char *document_begin;
	char *document_end;

	char *header_begin;
	char *header_end;

	char *document_title_begin;
	char *document_title_end;

	char *document_keywords_begin;
	char *document_keywords_end;

	char *document_author_begin;
	char *document_author_end;

	char *document_changedate_begin;
	char *document_changedate_end;

	char *body_begin;
	char *body_end;

	char *word_begin;
	char *word_end;

	char *paragraph_begin;
	char *paragraph_end;

	char *center_begin;
	char *center_end;

	char *align_left_begin;
	char *align_left_end;

	char *align_right_begin;
	char *align_right_end;

	char *justify_begin;
	char *justify_end;

	char *forced_space;
	char *line_break;
	char *page_break;

	char *hyperlink_begin;
	char *hyperlink_end;

	char *imagelink_begin;
	char *imagelink_end;

	char *table_begin;
	char *table_end;

	char *table_row_begin;
	char *table_row_end;

	char *table_cell_begin;
	char *table_cell_end;

	/* Character attributes */
	char *font_begin;
	char *font_end;

	char *fontsize_begin;
	char *fontsize_end;

	/* standard font sizes are optional */
	char *fontsize8_begin;
	char *fontsize8_end;

	char *fontsize10_begin;
	char *fontsize10_end;

	char *fontsize12_begin;
	char *fontsize12_end;

	char *fontsize14_begin;
	char *fontsize14_end;

	char *fontsize18_begin;
	char *fontsize18_end;

	char *fontsize24_begin;
	char *fontsize24_end;

	char *fontsize36_begin;
	char *fontsize36_end;

	char *fontsize48_begin;
	char *fontsize48_end;

	char *smaller_begin;
	char *smaller_end;

	char *bigger_begin;
	char *bigger_end;

	char *foreground_begin;
	char *foreground_end;

	char *background_begin;
	char *background_end;

	char *bold_begin;
	char *bold_end;

	char *italic_begin;
	char *italic_end;

	char *underline_begin;
	char *underline_end;

	char *dbl_underline_begin;
	char *dbl_underline_end;

	char *superscript_begin;
	char *superscript_end;

	char *subscript_begin;
	char *subscript_end;

	char *strikethru_begin;
	char *strikethru_end;

	char *dbl_strikethru_begin;
	char *dbl_strikethru_end;

	char *emboss_begin;
	char *emboss_end;

	char *engrave_begin;
	char *engrave_end;

	char *shadow_begin;
	char *shadow_end;

	char *outline_begin;
	char *outline_end;

	char *small_caps_begin;
	char *small_caps_end;

	char *pointlist_begin;
	char *pointlist_end;

	char *pointlist_item_begin;
	char *pointlist_item_end;

	char *numericlist_begin;
	char *numericlist_end;

	char *numericlist_item_begin;
	char *numericlist_item_end;

	char *expand_begin;
	char *expand_end;

	char *toc_entry_begin;
	char *toc_entry_end;

	char *index_entry_begin;
	char *index_entry_end;

	/* XX These should really be replaced by references
	 * to one of the charsets.
	 */
	struct {
		char *bullet;
		char *left_quote;
		char *right_quote;
		char *left_dbl_quote;
		char *right_dbl_quote;
		char *nonbreaking_space;
		char *emdash;
		char *endash;
		char *lessthan;
		char *greaterthan;
		char *amp;
		char *copyright;
		char *trademark;
		char *nonbreaking_hyphen;
		char *optional_hyphen;
	} chars;

	char **ascii_translation_table;

	int simulate_small_caps : 1;
	int simulate_all_caps : 1;
	int simulate_word_underline : 1;

	char **ansi_translation_table;
	short ansi_first_char;
	short ansi_last_char;
	char **cp437_translation_table;
	short cp437_first_char;
	short cp437_last_char;
	char **cp850_translation_table;
	short cp850_first_char;
	short cp850_last_char;
	char **mac_translation_table;
	short mac_first_char;
	short mac_last_char;
#if 1 /* daved 0.20.0 */
	unsigned int unisymbol1_first_char;
	unsigned int unisymbol1_last_char;
	char **unisymbol1_translation_table;
	unsigned int unisymbol2_first_char;
	unsigned int unisymbol2_last_char;
	char **unisymbol2_translation_table;
	unsigned int unisymbol3_first_char;
	unsigned int unisymbol3_last_char;
	char **unisymbol3_translation_table;
	unsigned int unisymbol4_first_char;
	unsigned int unisymbol4_last_char;
	char **unisymbol4_translation_table;
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
	char **symbol_translation_table;
#endif

	void (*write_set_foreground) (int,int,int);
}
OutputPersonality;


extern OutputPersonality* op_create(void);
extern void op_free (OutputPersonality*);
#if 1 /* daved - 0.19.6 */
extern char* op_translate_char (OutputPersonality*,int,int, int);
#else
extern char* op_translate_char (OutputPersonality*,int,int);
#endif

extern void op_begin_std_fontsize (OutputPersonality*, int);
extern void op_end_std_fontsize (OutputPersonality*, int);


#define _OUTPUT
#endif

