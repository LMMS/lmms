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
 * Module name:    attr
 * Author name:    Zachary Smith
 * Create date:    01 Aug 01
 * Purpose:        Character attribute stack.
 *----------------------------------------------------------------------
 * Changes:
 * 01 Aug 01, tuorfa@yahoo.com: moved code over from convert.c
 * 06 Aug 01, tuorfa@yahoo.com: added several font attributes.
 * 18 Sep 01, tuorfa@yahoo.com: added AttrStack (stack of stacks) paradigm
 * 22 Sep 01, tuorfa@yahoo.com: added comment blocks
 * 29 Mar 05, daved@physiol.usyd.edu.au: changes requested by ZT Smith
 * 31 Oct 07, jasp00@users.sourceforge.net: replaced deprecated conversions
 * 16 Dec 07, daved@physiol.usyd.edu.au: fixed fore/background_begin error
 *                       and updated to GPL v3
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

#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "ur_malloc.h"
#include "defs.h"
#include "error.h"
#include "attr.h"
#include "main.h"

extern void starting_body();
extern void starting_text();

extern QString outstring;

extern int simulate_allcaps;
extern int simulate_smallcaps;


#define MAX_ATTRS (10000)


/* For each RTF text block (the text within braces) we must keep
 * an AttrStack which is a stack of attributes and their optional
 * parameter. Since RTF text blocks are nested, these make up a
 * stack of stacks. And, since RTF text blocks inherit attributes
 * from parent blocks, all new AttrStacks do the same from
 * their parent AttrStack.
 */
typedef struct _stack {
	unsigned char attr_stack[MAX_ATTRS];
	char *attr_stack_params[MAX_ATTRS]; 
	int tos;
	struct _stack *next;
} AttrStack;

static AttrStack *stack_of_stacks = NULL;
static AttrStack *stack_of_stacks_top = NULL;


void attr_clear_all()
{
    stack_of_stacks = NULL;
    stack_of_stacks_top = NULL;
}


/*========================================================================
 * Name:	attr_express_begin
 * Purpose:	Print the HTML for beginning an attribute.
 * Args:	Attribute number, optional string parameter.
 * Returns:	None.
 *=======================================================================*/

void 
attr_express_begin (int attr, const char* param) {
	switch(attr) 
	{
	case ATTR_BOLD: 
		outstring+=QString().sprintf("%s", op->bold_begin); 
		break;
	case ATTR_ITALIC: 
		outstring+=QString().sprintf("%s", op->italic_begin); 
		break;

	/* Various underlines, they all resolve to HTML's <u> */
	case ATTR_THICK_UL:
	case ATTR_WAVE_UL:
	case ATTR_DASH_UL:
	case ATTR_DOT_UL: 
	case ATTR_DOT_DASH_UL:
	case ATTR_2DOT_DASH_UL:
	case ATTR_WORD_UL: 
	case ATTR_UNDERLINE: 
		outstring+=QString().sprintf("%s", op->underline_begin); 
		break;

	case ATTR_DOUBLE_UL: 
		outstring+=QString().sprintf("%s", op->dbl_underline_begin); 
		break;

	case ATTR_FONTSIZE: 
		op_begin_std_fontsize (op, atoi (param));
		break;

	case ATTR_FONTFACE: 
		outstring+=QString().sprintf(op->font_begin,param); 
		break;

	case ATTR_FOREGROUND: 
		outstring+=QString().sprintf(op->foreground_begin, param); 
		break;

	case ATTR_BACKGROUND: 
		if (!simple_mode)
			outstring+=QString().sprintf(op->background_begin,param); 
		break;

	case ATTR_SUPER: 
		outstring+=QString().sprintf("%s", op->superscript_begin); 
		break;
	case ATTR_SUB: 
		outstring+=QString().sprintf("%s", op->subscript_begin); 
		break;

	case ATTR_STRIKE: 
		outstring+=QString().sprintf("%s", op->strikethru_begin); 
		break;

	case ATTR_DBL_STRIKE: 
		outstring+=QString().sprintf("%s", op->dbl_strikethru_begin); 
		break;

	case ATTR_EXPAND: 
		outstring+=QString().sprintf(op->expand_begin, param); 
		break;

	case ATTR_OUTLINE: 
		outstring+=QString().sprintf("%s", op->outline_begin); 
		break;
	case ATTR_SHADOW: 
		outstring+=QString().sprintf("%s", op->shadow_begin); 
		break;
	case ATTR_EMBOSS: 
		outstring+=QString().sprintf("%s", op->emboss_begin); 
		break;
	case ATTR_ENGRAVE: 
		outstring+=QString().sprintf("%s", op->engrave_begin); 
		break;

	case ATTR_CAPS:
		if (op->simulate_all_caps)
			simulate_allcaps = true;
		break;

	case ATTR_SMALLCAPS: 
		if (op->simulate_small_caps)
			simulate_smallcaps = true;
		else {
			if (op->small_caps_begin)
				outstring+=QString().sprintf("%s", op->small_caps_begin); 
		}
		break;
	}
}


/*========================================================================
 * Name:	attr_express_end
 * Purpose:	Print HTML to complete an attribute.
 * Args:	Attribute number.
 * Returns:	None.
 *=======================================================================*/

void 
attr_express_end (int attr, char *param)
{
	switch(attr) 
	{
	case ATTR_BOLD: 
		outstring+=QString().sprintf("%s", op->bold_end); 
		break;
	case ATTR_ITALIC: 
		outstring+=QString().sprintf("%s", op->italic_end); 
		break;

	/* Various underlines, they all resolve to HTML's </u> */
	case ATTR_THICK_UL:
	case ATTR_WAVE_UL:
	case ATTR_DASH_UL:
	case ATTR_DOT_UL: 
	case ATTR_DOT_DASH_UL:
	case ATTR_2DOT_DASH_UL: 
	case ATTR_WORD_UL: 
	case ATTR_UNDERLINE: 
		outstring+=QString().sprintf("%s", op->underline_end); 
		break;

	case ATTR_DOUBLE_UL: 
		outstring+=QString().sprintf("%s", op->dbl_underline_end); 
		break;

	case ATTR_FONTSIZE: 
		op_end_std_fontsize (op, atoi (param));
		break;

	case ATTR_FONTFACE: 
		outstring+=QString().sprintf("%s", op->font_end); 
		break;

	case ATTR_FOREGROUND: 
		outstring+=QString().sprintf("%s", op->foreground_end); 
		break;
	case ATTR_BACKGROUND: 
		if (!simple_mode)
		  outstring+=QString().sprintf("%s", op->background_end);
		break;

	case ATTR_SUPER: 
		outstring+=QString().sprintf("%s", op->superscript_end); 
		break;
	case ATTR_SUB: 
		outstring+=QString().sprintf("%s", op->subscript_end); 
		break;

	case ATTR_STRIKE: 
		outstring+=QString().sprintf("%s", op->strikethru_end); 
		break;

	case ATTR_DBL_STRIKE: 
		outstring+=QString().sprintf("%s", op->dbl_strikethru_end); 
		break;

	case ATTR_OUTLINE: 
		outstring+=QString().sprintf("%s", op->outline_end); 
		break;
	case ATTR_SHADOW: 
		outstring+=QString().sprintf("%s", op->shadow_end); 
		break;
	case ATTR_EMBOSS: 
		outstring+=QString().sprintf("%s", op->emboss_end); 
		break;
	case ATTR_ENGRAVE: 
		outstring+=QString().sprintf("%s", op->engrave_end); 
		break;

	case ATTR_EXPAND: 
		outstring+=QString().sprintf("%s", op->expand_end); 
		break;

	case ATTR_CAPS:
		if (op->simulate_all_caps)
			simulate_allcaps = false;
		break;

	case ATTR_SMALLCAPS: 
		if (op->simulate_small_caps)
			simulate_smallcaps = false;
		else {
			if (op->small_caps_end)
				outstring+=QString().sprintf("%s", op->small_caps_end); 
		}
		break;
	}
}



/*========================================================================
 * Name:	attr_push
 * Purpose:	Pushes an attribute onto the current attribute stack.
 * Args:	Attribute number, optional string parameter.
 * Returns:	None.
 *=======================================================================*/

void 
attr_push(int attr, const char* param) 
{
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler("No stack to push attribute onto");
		return;
	}

	if (stack->tos >= MAX_ATTRS) {
		fprintf(stderr, "Too many attributes!\n");
		return;
	}

	/* Make sure it's understood we're in the <body> section. */
	/* KLUDGE */
	starting_body();
	starting_text();

	++stack->tos;
	stack->attr_stack[stack->tos] = attr;
	if (param) 
		stack->attr_stack_params[stack->tos] = my_strdup(param);
	else
		stack->attr_stack_params[stack->tos] = NULL;

	attr_express_begin(attr, param);
}

#if 1 /* daved 0.20.2 */

/*========================================================================
 * Name:	attr_get_param
 * Purpose:	Reads an attribute from the current attribute stack.
 * Args:	Attribute number
 * Returns:	string.
 *=======================================================================*/

char * 
attr_get_param(int attr) 
{
	int i;
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler("No stack to get attribute from");
		return NULL;
	}

	i=stack->tos;
	while (i>=0)
	{
		if(stack->attr_stack [i] == attr)
		{
			if(stack->attr_stack_params [i] != NULL)
				return stack->attr_stack_params [i];
			else	
				return NULL;
		}
		i--;
	}
	return NULL;
}

#endif


/*========================================================================
 * Name:	attrstack_copy_all
 * Purpose:	Routine to copy all attributes from one stack to another.
 * Args:	Two stacks.
 * Returns:	None.
 *=======================================================================*/

void 
attrstack_copy_all (AttrStack *src, AttrStack *dest) 
{
	int i;
	int total;

	CHECK_PARAM_NOT_NULL(src);
	CHECK_PARAM_NOT_NULL(dest);

	total = src->tos + 1;

	for (i=0; i<total; i++)
	{
		int attr=src->attr_stack [i];
		char *param=src->attr_stack_params [i];

		dest->attr_stack[i] = attr;
		if (param)
			dest->attr_stack_params[i] = my_strdup (param);
		else
			dest->attr_stack_params[i] = NULL;
	}

	dest->tos = src->tos;
}

/*========================================================================
 * Name:	attrstack_unexpress_all
 * Purpose:	Routine to un-express all attributes heretofore applied,
 * 		without removing any from the stack.
 * Args:	Stack whost contents should be unexpressed.
 * Returns:	None.
 * Notes:	This is needed by attrstack_push, but also for \cell, which
 * 		often occurs within a brace group, yet HTML uses <td></td> 
 *		which clear attribute info within that block.
 *=======================================================================*/

void 
attrstack_unexpress_all (AttrStack *stack)
{
	int i;

	CHECK_PARAM_NOT_NULL(stack);

	i=stack->tos;
	while (i>=0)
	{
		int attr=stack->attr_stack [i];
		char *param=stack->attr_stack_params [i];

		attr_express_end (attr, param);
		i--;
	}
}


/*========================================================================
 * Name:	attrstack_push
 * Purpose:	Creates a new attribute stack, pushes it onto the stack
 *		of stacks, performs inheritance from previous stack.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/
void 
attrstack_push () 
{
	AttrStack *new_stack;
	AttrStack *prev_stack;

	new_stack = (AttrStack*) my_malloc (sizeof (AttrStack));
	memset ((void*) new_stack, 0, sizeof (AttrStack));

	prev_stack = stack_of_stacks_top;

	if (!stack_of_stacks) {
		stack_of_stacks = new_stack;
	} else {
		stack_of_stacks_top->next = new_stack;
	}
	stack_of_stacks_top = new_stack;
	new_stack->tos = -1;

	if (prev_stack) {
		attrstack_unexpress_all (prev_stack);
		attrstack_copy_all (prev_stack, new_stack);
		attrstack_express_all ();
	}
}



/*========================================================================
 * Name:	attr_pop 
 * Purpose:	Removes and undoes the effect of the top attribute of
 *		the current AttrStack.
 * Args:	The top attribute's number, for verification.
 * Returns:	Success/fail flag.
 *=======================================================================*/

int 
attr_pop (int attr) 
{
	AttrStack *stack = stack_of_stacks_top;

	if (!stack) {
		warning_handler ("no stack to pop attribute from");
		return false;
	}

	if(stack->tos>=0 && stack->attr_stack[stack->tos]==attr)
	{
		char *param = stack->attr_stack_params [stack->tos];

		attr_express_end (attr, param);

		if (param) my_free(param);

		stack->tos--;

		return true;
	}
	else
		return false;
}



/*========================================================================
 * Name:	attr_read
 * Purpose:	Reads but leaves in place the top attribute of the top
 * 		attribute stack.
 * Args:	None.
 * Returns:	Attribute number.
 *=======================================================================*/

int 
attr_read() {
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler ("no stack to read attribute from");
		return false;
	}

	if(stack->tos>=0)
	{
		int attr = stack->attr_stack [stack->tos];
		return attr;
	}
	else
		return ATTR_NONE;
}


/*========================================================================
 * Name:	attr_drop_all 
 * Purpose:	Undoes all attributes that an AttrStack contains.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
attr_drop_all () 
{
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler ("no stack to drop all attributes from");
		return;
	}

	while (stack->tos>=0) 
	{
		char *param=stack->attr_stack_params [stack->tos];
		if (param) my_free(param);
		stack->tos--;
	}
}


/*========================================================================
 * Name:	attrstack_drop
 * Purpose:	Removes the top AttrStack from the stack of stacks, undoing
 *		all attributes that it had in it.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
attrstack_drop () 
{
	AttrStack *stack = stack_of_stacks_top;
	AttrStack *prev_stack;
	if (!stack) {
		warning_handler ("no attr-stack to drop");
		return;
	}

	attr_pop_all ();

	prev_stack = stack_of_stacks;
	while(prev_stack && prev_stack->next && prev_stack->next != stack)
		prev_stack = prev_stack->next;

	if (prev_stack) {
		stack_of_stacks_top = prev_stack;
		prev_stack->next = NULL;
	} else {
		stack_of_stacks_top = NULL;
		stack_of_stacks = NULL;
	}
	my_free ((char*) stack);

	attrstack_express_all ();
}

/*========================================================================
 * Name:	attr_pop_all
 * Purpose:	Routine to undo all attributes heretofore applied, 
 *		also reversing the order in which they were applied.
 * Args:	None.
 * Returns:	None.
 *=======================================================================*/

void 
attr_pop_all() 
{
	AttrStack *stack = stack_of_stacks_top;
	if (!stack) {
		warning_handler ("no stack to pop from");
		return;
	}

	while (stack->tos>=0) {
		int attr=stack->attr_stack [stack->tos];
		char *param=stack->attr_stack_params [stack->tos];
		attr_express_end (attr,param);
		if (param) my_free(param);
		stack->tos--;
	}
}


/*========================================================================
 * Name:	attrstack_express_all
 * Purpose:	Routine to re-express all attributes heretofore applied.
 * Args:	None.
 * Returns:	None.
 * Notes:	This is needed by attrstack_push, but also for \cell, which
 * 		often occurs within a brace group, yet HTML uses <td></td> 
 *		which clear attribute info within that block.
 *=======================================================================*/

void 
attrstack_express_all() {
	AttrStack *stack = stack_of_stacks_top;
	int i;

	if (!stack) {
		warning_handler ("no stack to pop from");
		return;
	}

	i=0;
	while (i<=stack->tos) 
	{
		int attr=stack->attr_stack [i];
		char *param=stack->attr_stack_params [i];
		attr_express_begin (attr, param);
		i++;
	}
}


/*========================================================================
 * Name:	attr_pop_dump
 * Purpose:	Routine to un-express all attributes heretofore applied.
 * Args:	None.
 * Returns:	None.
 * Notes:	This is needed for \cell, which often occurs within a 
 *		brace group, yet HTML uses <td></td> which clear attribute 
 *		info within that block.
 *=======================================================================*/

void 
attr_pop_dump() {
	AttrStack *stack = stack_of_stacks_top;
	int i;

	if (!stack) return;

	i=stack->tos;
	while (i>=0) 
	{
		int attr=stack->attr_stack [i];
		attr_pop (attr);
		i--;
	}
}

