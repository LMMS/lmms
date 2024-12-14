/*
 * BassBooster.h - bass-booster-effect-plugin
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef BASS_BOOSTER_H
#define BASS_BOOSTER_H

#include "AudioPluginInterface.h"
#include "DspEffectLibrary.h"
#include "BassBoosterControls.h"

namespace lmms
{

class BassBoosterEffect : public DefaultEffectPluginInterface
{
public:
	BassBoosterEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	~BassBoosterEffect() override = default;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls* controls() override
	{
		return &m_bbControls;
	}


protected:
	void changeFrequency();
	void changeGain();
	void changeRatio();

	bool m_frequencyChangeNeeded;

private:
	DspEffectLibrary::MonoToStereoAdaptor<DspEffectLibrary::FastBassBoost> m_bbFX;

	BassBoosterControls m_bbControls;

	friend class BassBoosterControls;

} ;


} // namespace lmms

#endif
