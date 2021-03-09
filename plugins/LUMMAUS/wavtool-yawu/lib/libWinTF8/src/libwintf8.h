/*
  Copyright (c) 2014 StarBrilliant <m13253@hotmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms are permitted
  provided that the above copyright notice and this paragraph are
  duplicated in all such forms and that any documentation,
  advertising materials, and other materials related to such
  distribution and use acknowledge that the software was developed by
  StarBrilliant.
  The name of StarBrilliant may not be used to endorse or promote
  products derived from this software without specific prior written
  permission.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/
#pragma once
#ifndef WTF8_H_INCLUDED_
#define WTF8_H_INCLUDED_

#include "libwintf8/argv.h"
#include "libwintf8/env.h"
#include "libwintf8/fileio.h"
#include "libwintf8/initcon.h"
#include "libwintf8/localconv.h"
#include "libwintf8/printf.h"
#include "libwintf8/spawn.h"
#include "libwintf8/streamio.h"
#include "libwintf8/termio.h"
#include "libwintf8/u8str.h"
#include "libwintf8/utfconv.h"

#if defined(_WIN32) && !defined(WTF8_NO_DEFINE_UNICODE)
/* Use Unicode version of WinAPI by default just in case you forgot a -W suffix */
#ifndef UNICODE
#define UNICODE
#endif
/* Use Unicode version of C Runtime by default just in case you needed a function not provided by libWinTF8 */
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#endif
