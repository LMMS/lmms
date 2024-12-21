/* Vectorscope.h - declaration of Vectorscope class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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

#ifndef VECTORSCOPE_H
#define VECTORSCOPE_H

#include "AudioPluginInterface.h"
#include "LocklessRingBuffer.h"
#include "VecControls.h"

namespace lmms
{


//! Top level class; handles LMMS interface and accumulates data for processing.
class Vectorscope : public DefaultEffectPluginInterface
{
public:
	Vectorscope(Model *parent, const Descriptor::SubPluginFeatures::Key *key);
	~Vectorscope() override = default;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls *controls() override {return &m_controls;}
	LocklessRingBuffer<SampleFrame> *getBuffer() {return &m_inputBuffer;}

private:
	VecControls m_controls;

	// Maximum LMMS buffer size (hard coded, the actual constant is hard to get)
	const unsigned int m_maxBufferSize = 4096;
	LocklessRingBuffer<SampleFrame> m_inputBuffer;
};


} // namespace lmms

#endif // VECTORSCOPE_H

