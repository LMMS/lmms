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
#ifndef WTF8_ENV_H_INCLUDED_
#define WTF8_ENV_H_INCLUDED_

#include "u8str.h"

#ifdef __cplusplus
namespace WTF8 {

/**
 * Get an environment variable of this process
 *
 * Errors:
 *   If no such environment variable exists, return nullptr
 *
 * Cleaning:
 *   The result must be released with `WTF8::freeenv`
 */
const char *getenv(const char *varname);

/**
 * Free the memory that was allocated with `WTF8::getenv`
 *
 * Result:
 *   nullptr
 */
const char *freeenv(const char *envstring);

/**
 * Set an environment variable for this process
 *
 * Result:
 *   0 on success, or -1 on error
 *
 * Errors:
 *   `errno` is set to indicate an error
 */
int setenv(const char *varname, const char *value);

/**
 * Clear an environment variable for this process
 *
 * Result:
 *   0 on success, or -1 on error
 *
 * Errors:
 *   `errno` is set to indicate an error
 */
int unsetenv(const char *varname);

}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get an environment variable of this process
 *
 * Errors:
 *   If no such environment variable exists, return NULL
 *
 * Cleaning:
 *   The result must be released with `WTF8::freeenv`
 */
const char *WTF8_getenv(const char *varname);

/**
 * Free the memory that was allocated with `WTF8::getenv`
 *
 * Result:
 *   NULL
 */
const char *WTF8_freeenv(const char *envstring);

/**
 * Set an environment variable for this process
 *
 * Result:
 *   0 on success, or -1 on error
 *
 * Errors:
 *   `errno` is set to indicate an error
 */
int WTF8_setenv(const char *varname, const char *value);

/**
 * Clear an environment variable for this process
 *
 * Result:
 *   0 on success, or -1 on error
 *
 * Errors:
 *   `errno` is set to indicate an error
 */
int WTF8_unsetenv(const char *varname);

#ifdef __cplusplus
}
#endif

#endif
