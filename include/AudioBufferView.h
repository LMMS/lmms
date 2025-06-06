/*
 * AudioBufferView.h
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#ifndef LMMS_AUDIO_BUFFER_VIEW_H
#define LMMS_AUDIO_BUFFER_VIEW_H

#include <cstddef>
#include <type_traits>

namespace lmms {

//! A struct representing a view into an interleaved audio buffer.
template <typename T> struct InterleavedAudioBufferView
{
	static_assert(std::disjunction_v<std::is_same<T, const float>, std::is_same<T, float>,
					  std::is_same<T, const double>, std::is_same<T, double>>,
		"Unsupported data type");
	T* data;
	std::size_t frames;
	int channels;
};

} // namespace lmms

#endif // LMMS_AUDIO_BUFFER_VIEW_H