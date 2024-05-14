/*
 * lmms_filesystem.h - std::filesystem include helper
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

// TODO: Remove this header once all build runners support std::filesystem

#ifndef LMMS_FILESYSTEM_H
#define LMMS_FILESYSTEM_H

#if __has_include(<filesystem>)
#include <filesystem>
namespace lmms {
	namespace fs = std::filesystem;
} // namespace lmms
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace lmms {
	namespace fs = std::experimental::filesystem;
} // namespace lmms
#else
#error "Standard filesystem library not available"
#endif

#endif // LMMS_FILESYSTEM_H
