/*
 * Vibed.h - combination of PluckedStringSynth and BitInvader
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo/com>
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

#ifndef LMMS_VIBED_H
#define LMMS_VIBED_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "NineButtonSelector.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "Graph.h"
#include "PixmapButton.h"

#include <array>
#include <memory>

namespace lmms
{


class NotePlayHandle;  // IWYU pragma: keep

namespace gui
{
class VibedView;
}

class Vibed : public Instrument
{
	Q_OBJECT
public:
	Vibed(InstrumentTrack* instrumentTrack);
	~Vibed() override = default;

	void playNote(NotePlayHandle* n, SampleFrame* workingBuffer) override;
	void deleteNotePluginData(NotePlayHandle* n) override;

	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView(QWidget* parent) override;

private:
	class StringContainer;

	static constexpr int s_sampleLength = 128;
	static constexpr int s_stringCount = 9;

	std::array<std::unique_ptr<FloatModel>, s_stringCount> m_pickModels;
	std::array<std::unique_ptr<FloatModel>, s_stringCount> m_pickupModels;
	std::array<std::unique_ptr<FloatModel>, s_stringCount> m_stiffnessModels;
	std::array<std::unique_ptr<FloatModel>, s_stringCount> m_volumeModels;
	std::array<std::unique_ptr<FloatModel>, s_stringCount> m_panModels;
	std::array<std::unique_ptr<FloatModel>, s_stringCount> m_detuneModels;
	std::array<std::unique_ptr<FloatModel>, s_stringCount> m_randomModels;
	std::array<std::unique_ptr<FloatModel>, s_stringCount> m_lengthModels;
	std::array<std::unique_ptr<BoolModel>, s_stringCount> m_powerModels;
	std::array<std::unique_ptr<graphModel>, s_stringCount> m_graphModels;
	std::array<std::unique_ptr<BoolModel>, s_stringCount> m_impulseModels;
	std::array<std::unique_ptr<NineButtonSelectorModel>, s_stringCount> m_harmonicModels;

	friend class gui::VibedView;
};


namespace gui
{


class VibedView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	VibedView(Instrument* instrument, QWidget* parent);
	~VibedView() override = default;

public slots:
	void showString(int str);
	void contextMenuEvent(QContextMenuEvent*) override;

protected slots:
	void sinWaveClicked();
	void triangleWaveClicked();
	void sawWaveClicked();
	void sqrWaveClicked();
	void noiseWaveClicked();
	void usrWaveClicked();
	void smoothClicked();
	void normalizeClicked();

private:
	void modelChanged() override;

	// String-related
	Knob m_volumeKnob;
	Knob m_stiffnessKnob;
	Knob m_pickKnob;
	Knob m_pickupKnob;
	Knob m_panKnob;
	Knob m_detuneKnob;
	Knob m_randomKnob;
	Knob m_lengthKnob;
	Graph m_graph;
	LedCheckBox m_impulse;
	LedCheckBox m_power;
	std::unique_ptr<NineButtonSelector> m_harmonic;

	// Not in model
	std::unique_ptr<NineButtonSelector> m_stringSelector;
	PixmapButton m_smoothBtn;
	PixmapButton m_normalizeBtn;

	// From impulse editor
	PixmapButton m_sinWaveBtn;
	PixmapButton m_triangleWaveBtn;
	PixmapButton m_sawWaveBtn;
	PixmapButton m_sqrWaveBtn;
	PixmapButton m_whiteNoiseWaveBtn;
	PixmapButton m_usrWaveBtn;
};


} // namespace gui

} // namespace lmms

#endif // LMMS_VIBED_H
