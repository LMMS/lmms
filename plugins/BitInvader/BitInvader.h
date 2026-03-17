/*
 * BitInvader.h - declaration of class BitInvader and BSynth which
 *                         are a wavetable synthesizer
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

#ifndef BIT_INVADER_H
#define BIT_INVADER_H

#include "AutomatableModel.h"
#include "ComboBox.h"
#include "ComboBoxModel.h"
#include "Graph.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "PixmapButton.h"

namespace lmms
{

namespace gui { class BitInvaderView; }

//! @brief The phase of a note, represented as an index into a wavetable.
using BitInvaderIndex = float;

class BitInvader : public Instrument
{
	Q_OBJECT
	friend class gui::BitInvaderView;
public:
	BitInvader(InstrumentTrack*);
	~BitInvader() override = default;

	void playNote(NotePlayHandle* nph, SampleFrame* workingBuffer) override;
	void deleteNotePluginData(NotePlayHandle* nph) override;
	void saveSettings(QDomDocument& doc, QDomElement& el) override;
	void loadSettings(const QDomElement& el) override;
	QString nodeName() const override;
	float desiredReleaseTimeMs() const override { return 1.5f; }
	gui::PluginView* instantiateView(QWidget* parent) override;

protected slots:
	void lengthChanged();
	void normalize();

private:
	FloatModel m_sampleLength;
	graphModel m_graph;

	BoolModel m_interpolation;

	ComboBoxModel m_normalizeMode;

	float m_normalizeFactor; //!< Factor by which to amplify output such that the output is normalized
	float m_normalizeOffset; //!< Amount by which to offset the output such that no DC offset is produced when normalized
};




namespace gui
{

class BitInvaderView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	BitInvaderView(Instrument*, QWidget* parent);
	~BitInvaderView() override = default;
	QSize sizeHint() const override { return QSize(250, 280); }
	QSize minimumSizeHint() const override { return sizeHint(); }

protected slots:
	void interpolationToggled(bool value);
	void sinWaveClicked();
	void triangleWaveClicked();
	void sqrWaveClicked();
	void sawWaveClicked();
	void noiseWaveClicked();
	void usrWaveClicked();
	void smoothClicked();

private:
	void modelChanged() override;

	Knob* m_sampleLengthKnob;
	PixmapButton* m_sinWaveBtn;
	PixmapButton* m_triangleWaveBtn;
	PixmapButton* m_sqrWaveBtn;
	PixmapButton* m_sawWaveBtn;
	PixmapButton* m_whiteNoiseWaveBtn;
	PixmapButton* m_smoothBtn;
	PixmapButton* m_usrWaveBtn;
	Graph* m_graph;
	LedCheckBox* m_interpolationToggle;
	ComboBox* m_normalizeMode;
};


} // namespace gui

} // namespace lmms

#endif // BIT_INVADER_H
