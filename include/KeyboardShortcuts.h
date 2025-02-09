/*
 * KeyboardShortcuts.h - Cross-platform handling of keyboard modifier keys
 *
 * Copyright (c) 2025 Michael Gregorius
 * Copyright (c) 2025 Tres Finocchiaro <tres.finocchiaro/at/gmail.com>
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

#ifndef LMMS_KEYBOARDSHORTCUTS_H
#define LMMS_KEYBOARDSHORTCUTS_H

#include "lmmsconfig.h"

#include "qnamespace.h"


namespace lmms
{

// Qt on macOS maps:
//  - ControlModifier --> Command keys
//  - MetaModifier value --> Control keys
//  - Qt::AltModifier --> Option keys
//
// Our UI hints need to be adjusted to accommodate for this
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

// UI hint for copying OR linking a UI component
// this MUST be consistent with KBD_COPY_MODIFIER
constexpr const char* UI_COPY_KEY =
#ifdef LMMS_BUILD_APPLE
UI_ALT_KEY;
#else
UI_CTRL_KEY;
#endif

// Shortcut for copying OR linking a UI component
// this MUST be consistent with UI_COPY_KEY
constexpr Qt::KeyboardModifier KBD_COPY_MODIFIER =
#ifdef LMMS_BUILD_APPLE
Qt::AltModifier;
#else
Qt::ControlModifier;
#endif

} // namespace lmms

#endif // LMMS_KEYBOARDSHORTCUTS_H
