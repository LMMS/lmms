/*
 * OperatingSystemHelpers.h - Helpers for OS related concerns
 *
 * Copyright (c) 2025- Michael Gregorius
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef LMMS_OPERATINGSYSTEMHELPERS_H
#define LMMS_OPERATINGSYSTEMHELPERS_H

#include "lmmsconfig.h"

#include "qnamespace.h"


namespace lmms
{

constexpr char LADSPA_PATH_SEPERATOR =
#ifdef LMMS_BUILD_WIN32
';';
#else
':';
#endif

// Abstract away GUI CTRL key (linux/windows) vs ⌘ (apple)
constexpr const char* UI_CTRL_KEY =
#ifdef LMMS_BUILD_APPLE
"⌘";
#else
"Ctrl";
#endif

constexpr const char* UI_ALT_KEY =
#ifdef LMMS_BUILD_APPLE
"Option";
#else
"Alt";
#endif

constexpr const char* getOSSppecificModifierKeyString()
{
#ifdef LMMS_BUILD_APPLE
    return UI_ALT_KEY;
#else
    return UI_CTRL_KEY;
#endif
}

constexpr Qt::KeyboardModifier getOSSpecificModifierKey()
{
#ifdef LMMS_BUILD_APPLE
	return Qt::AltModifier;
#else
	return Qt::ControlModifier;
#endif    
}

} // namespace lmms

#endif // LMMS_OPERATINGSYSTEMHELPERS_H
