/*
 * delayeffect.h - declaration of DelayEffect class, the Delay plugin
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef DELAYEFFECT_H
#define DELAYEFFECT_H

#include "AudioPluginInterface.h"
#include "DelayControls.h"

namespace lmms
{

class Lfo;
class StereoDelay;

class DelayEffect : public DefaultEffectPluginInterface
{
public:
	DelayEffect(Model* parent , const Descriptor::SubPluginFeatures::Key* key );
	~DelayEffect() override;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls* controls() override
	{
		return &m_delayControls;
	}
	void changeSampleRate();

private:
	DelayControls m_delayControls;
	StereoDelay* m_delay;
	Lfo* m_lfo;
	float m_outGain;
	float m_currentLength;
};


} // namespace lmms

#endif // DELAYEFFECT_H
