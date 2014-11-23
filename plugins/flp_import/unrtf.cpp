/*
 * unrtf.cpp - integration of UnRTF
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "lmmsconfig.h"

#include <QtCore/QString>
#include <QtCore/QBuffer>

#include <cstdio>

extern "C"
{

// unrtf-stuff
#include "defs.h"
#include "main.h"
#include "html.h"
#include "word.h"
#include "hash.h"
#include "convert.h"
#include "attr.h"


int lineno = 0;
#define inline_mode 0
#define debug_mode 0
#define nopict_mode 1
#define verbose_mode 0
#define simple_mode 0
#define no_remap_mode 0

QString outstring;
short numchar_table;
OutputPersonality * op = NULL;


// include unrtf-source
#include "attr.c"
#include "convert.c"
#include "error.c"
#include "hash.c"
#include "html.c"
#include "ur_malloc.c"
#include "output.c"
#include "parse.c"
#include "util.c"
#include "word.c"

}
