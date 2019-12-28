/*
 * Microtuner.h - manage tuning and scale information of an instrument
 *
 * Copyright (c) 2019 Martin Pavelek <he29.HS/at/gmail.com>
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

#ifndef MICROTUNER_H
#define MICROTUNER_H

#include <atomic>

#include "AutomatableModel.h"
#include "JournallingObject.h"
#include "lmms_constants.h"

class InstrumentTrack;


class LMMS_EXPORT Microtuner : public Model, public JournallingObject
{
	Q_OBJECT
public:
	Microtuner(InstrumentTrack *parent);
	~Microtuner();

	bool enabled() const {return m_enabledModel.value();}
	BoolModel* enabledModel() {return &m_enabledModel;}

	float baseFreq() const {return enabled() ? m_baseFreqModel.value() : DefaultBaseFreq;}
	FloatModel* baseFreqModel() {return &m_baseFreqModel;}

	int keyToNote(int key) const;
	float noteToFreq(int note) const;
	float keyToFreq(int key) const;

protected:
	QString nodeName() const override {return "microtuner";}
	void saveSettings(QDomDocument & document, QDomElement &element) override;
	void loadSettings(const QDomElement &element) override;

private:
	InstrumentTrack *m_instrumentTrack;

	BoolModel m_enabledModel;		//!< Enable microtuner (otherwise using 12-TET @440 Hz)

	FloatModel m_baseFreqModel;		//!< Base note frequency (typ. 440 Hz)

//	Scale *m_scale;
	std::atomic<int*> m_keymap;		//!< mapping of MIDI keys (notes) to notes
};

#endif
