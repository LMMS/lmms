/*
 * Tuner.h - determine the fundamental frequency of audio signals
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#ifndef TUNER_H
#define TUNER_H

#include "Effect.h"
#include "TunerControls.h"

class Tuner : public Effect
{
public:
	Tuner(Model* parent, const Descriptor::SubPluginFeatures::Key* key);

	bool processAudioBuffer(sampleFrame* buf, const fpp_t frames) override;
    EffectControls* controls() override;

private:
    TunerControls m_tunerControls;
    friend class TunerControls;
};

#endif