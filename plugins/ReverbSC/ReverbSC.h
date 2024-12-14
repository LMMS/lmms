/*
 * RerverbSC.h - Reverb algorithm by Sean Costello
 *
 * Copyright (c) 2017 Paul Batchelor
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


#ifndef REVERBSC_H
#define REVERBSC_H

#include "AudioPluginInterface.h"
#include "ReverbSCControls.h"

extern "C" {
    #include "base.h"
    #include "revsc.h"
    #include "dcblock.h"
}


namespace lmms
{


class ReverbSCEffect : public DefaultEffectPluginInterface
{
public:
	ReverbSCEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	~ReverbSCEffect() override;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls* controls() override
	{
		return &m_reverbSCControls;
	}

	void changeSampleRate();

private:
	ReverbSCControls m_reverbSCControls;
	sp_data *sp;
	sp_revsc *revsc;
	sp_dcblock *dcblk[2];
	QMutex mutex;
	friend class ReverbSCControls;
} ;


} // namespace lmms

#endif
