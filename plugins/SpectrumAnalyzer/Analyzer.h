/* Analyzer.h - declaration of Analyzer class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 *
 * Based partially on Eq plugin code,
 * Copyright (c) 2014-2017, David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef ANALYZER_H
#define ANALYZER_H

#include "Effect.h"
#include "SaControls.h"
#include "SaProcessor.h"


//! Top level class; handles LMMS interface and feeds data to the data processor.
class Analyzer : public Effect
{
public:
	Analyzer(Model *parent, const Descriptor::SubPluginFeatures::Key *key);
	virtual ~Analyzer() {};

	bool processAudioBuffer(sampleFrame *buffer, const fpp_t frame_count) override;
	EffectControls *controls() override {return &m_controls;}

	SaProcessor *getProcessor() {return &m_processor;}

private:
	SaProcessor m_processor;
	SaControls m_controls;
};

#endif // ANALYZER_H

