/*
 * BitInvader.cpp - instrument which uses a usereditable wavetable
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

#include "BitInvader.h"

#include <cmath>

#include <QDomElement>

#include "AudioEngine.h"
#include "base64.h"
#include "embed.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "lmms_math.h"
#include "NotePlayHandle.h"
#include "plugin_export.h"
#include "Song.h"


namespace lmms
{

inline constexpr std::size_t wavetableSize = 200;

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT bitinvader_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"BitInvader",
	QT_TRANSLATE_NOOP("PluginBrowser", "Customizable wavetable synthesizer"),
	"Andreas Brandmaier <andreas/at/brandmaier/dot/de>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
};

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
	return new BitInvader(static_cast<InstrumentTrack*>(m));
}

} // extern "C"




BitInvader::BitInvader(InstrumentTrack* instrumentTrack)
	: Instrument(instrumentTrack, &bitinvader_plugin_descriptor)
	, m_sampleLength(wavetableSize, 4, wavetableSize, 1, this, tr("Sample length"))
	, m_graph(-1.f, 1.f, wavetableSize, this)
	, m_interpolation(false, this, tr("Interpolation"))
	, m_normalize(false, this, tr("Normalize"))
{
	m_graph.setWaveToSine();
	lengthChanged();

	connect(&m_sampleLength, &FloatModel::dataChanged, this, &BitInvader::lengthChanged, Qt::DirectConnection);
	connect(&m_graph, &graphModel::samplesChanged, this, &BitInvader::samplesChanged);
}




void BitInvader::saveSettings(QDomDocument& doc, QDomElement& el)
{
	el.setAttribute("version", "0.1");
	m_sampleLength.saveSettings(doc, el, "sampleLength");

	QString sampleString;
	base64::encode(reinterpret_cast<const char*>(m_graph.samples()), wavetableSize * sizeof(float), sampleString);
	el.setAttribute("sampleShape", sampleString);
	
	m_interpolation.saveSettings(doc, el, "interpolation");
	m_normalize.saveSettings(doc, el, "normalize");
}




void BitInvader::loadSettings(const QDomElement& el)
{
	m_graph.clear();
	m_sampleLength.loadSettings(el, "sampleLength");
	auto sampleLength = static_cast<int>(m_sampleLength.value());

	// Load sample shape
	int size = 0;
	char* dst = 0;
	base64::decode(el.attribute("sampleShape"), &dst, &size);

	m_graph.setLength(size / sizeof(float));
	m_graph.setSamples(reinterpret_cast<float*>(dst));
	m_graph.setLength(sampleLength);
	delete[] dst;

	m_interpolation.loadSettings(el, "interpolation");
	m_normalize.loadSettings(el, "normalize");
}




void BitInvader::lengthChanged()
{
	m_graph.setLength(static_cast<int>(m_sampleLength.value()));
	normalize();
}




void BitInvader::samplesChanged(int, int)
{
	normalize();
	//engine::getSongEditor()->setModified();
}




void BitInvader::normalize()
{
	const auto samples = std::span<const float, wavetableSize>{m_graph.samples(), wavetableSize};
	const auto maxSamp = std::max_element(samples.begin(), samples.end())[0];
	const auto minSamp = std::min_element(samples.begin(), samples.end())[0];
	if (minSamp == maxSamp)
	{
		m_normalizeFactor = 1.f;
		m_normalizeOffset = 0.f;
		return;
	}
	const auto diff = maxSamp - minSamp;
	m_normalizeFactor = 2.f / diff;
	m_normalizeOffset = (maxSamp + minSamp) / -diff;
}




QString BitInvader::nodeName() const { return bitinvader_plugin_descriptor.name; }




void BitInvader::playNote(NotePlayHandle* nph, SampleFrame* workingBuffer)
{
	if (!nph->m_pluginData) { nph->m_pluginData = new BitInvaderNote{}; }

	const auto nfac = m_normalize.value() ? m_normalizeFactor : 1.f;
	const auto norg = m_normalize.value() ? m_normalizeOffset : 0.f;
	const auto& wavetable = m_graph.samples();
	const auto wavetableLenReal = static_cast<float>(m_graph.length());
	const auto phasePerSample = nph->frequency() / Engine::audioEngine()->outputSampleRate();
	const f_cnt_t frames = nph->framesLeftForCurrentPeriod();
	const f_cnt_t offset = nph->noteOffset();

	auto note = static_cast<BitInvaderNote*>(nph->m_pluginData);
	for (f_cnt_t frame = offset; frame < frames + offset; ++frame)
	{
		note->indexFrac = std::fmod(
			note->indexFrac + phasePerSample * wavetableLenReal,
			wavetableLenReal
		);
		note->index = static_cast<std::size_t>(note->indexFrac);

		const auto samp = m_interpolation.value()
			? std::lerp(
				wavetable[note->index],
				wavetable[(1 + note->index) % m_graph.length()],
				fraction(note->indexFrac)
			)
			: wavetable[note->index];

		workingBuffer[frame] = SampleFrame(samp * nfac + norg);
	}

	applyRelease(workingBuffer, nph);
}




void BitInvader::deleteNotePluginData(NotePlayHandle* nph)
{
	delete static_cast<BitInvaderNote*>(nph->m_pluginData);
}




gui::PluginView* BitInvader::instantiateView(QWidget* parent)
{
	return new gui::BitInvaderView(this, parent);
}




namespace gui
{


BitInvaderView::BitInvaderView(Instrument* instrument, QWidget* parent)
	: InstrumentViewFixedSize(instrument, parent)
{
	setAutoFillBackground(true);
	QPalette pal;

	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	
	m_sampleLengthKnob = new Knob(KnobType::Dark28, this);
	m_sampleLengthKnob->move(6, 201);
	m_sampleLengthKnob->setHintText(tr("Sample length"), "");

	m_graph = new Graph(this, Graph::Style::Nearest, 204, 134);
	m_graph->move(23,59); // 55,120 - 2px border
	m_graph->setAutoFillBackground(true);
	m_graph->setGraphColor(QColor{255, 255, 255});
	m_graph->setToolTip(tr("Draw your own waveform here by dragging your mouse on this graph."));

	pal = QPalette();
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraph"));
	m_graph->setPalette(pal);

	m_sinWaveBtn = new PixmapButton(this, tr("Sine wave"));
	m_sinWaveBtn->move(131, 205);
	m_sinWaveBtn->setActiveGraphic(embed::getIconPixmap("sin_wave_active"));
	m_sinWaveBtn->setInactiveGraphic(embed::getIconPixmap("sin_wave_inactive"));
	m_sinWaveBtn->setToolTip(tr("Sine wave"));

	m_triangleWaveBtn = new PixmapButton(this, tr("Triangle wave"));
	m_triangleWaveBtn->move(131 + 14, 205);
	m_triangleWaveBtn->setActiveGraphic(embed::getIconPixmap("triangle_wave_active"));
	m_triangleWaveBtn->setInactiveGraphic(embed::getIconPixmap("triangle_wave_inactive"));
	m_triangleWaveBtn->setToolTip(tr("Triangle wave"));

	m_sawWaveBtn = new PixmapButton(this, tr("Saw wave"));
	m_sawWaveBtn->move(131 + 14 * 2, 205);
	m_sawWaveBtn->setActiveGraphic(embed::getIconPixmap("saw_wave_active"));
	m_sawWaveBtn->setInactiveGraphic(embed::getIconPixmap("saw_wave_inactive"));
	m_sawWaveBtn->setToolTip(tr("Saw wave"));

	m_sqrWaveBtn = new PixmapButton(this, tr("Square wave"));
	m_sqrWaveBtn->move(131 + 14 * 3, 205);
	m_sqrWaveBtn->setActiveGraphic(embed::getIconPixmap("square_wave_active"));
	m_sqrWaveBtn->setInactiveGraphic(embed::getIconPixmap("square_wave_inactive"));
	m_sqrWaveBtn->setToolTip(tr("Square wave"));

	m_whiteNoiseWaveBtn = new PixmapButton(this,tr("White noise"));
	m_whiteNoiseWaveBtn->move(131 + 14 * 4, 205);
	m_whiteNoiseWaveBtn->setActiveGraphic(embed::getIconPixmap("white_noise_wave_active"));
	m_whiteNoiseWaveBtn->setInactiveGraphic(embed::getIconPixmap("white_noise_wave_inactive"));
	m_whiteNoiseWaveBtn->setToolTip(tr("White noise"));

	m_usrWaveBtn = new PixmapButton(this, tr("User-defined wave"));
	m_usrWaveBtn->move(131 + 14 * 5, 205);
	m_usrWaveBtn->setActiveGraphic(embed::getIconPixmap("usr_wave_active"));
	m_usrWaveBtn->setInactiveGraphic(embed::getIconPixmap("usr_wave_inactive"));
	m_usrWaveBtn->setToolTip(tr("User-defined wave"));

	m_smoothBtn = new PixmapButton(this, tr("Smooth waveform"));
	m_smoothBtn->move(131 + 14 * 6, 205);
	m_smoothBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("smooth_active"));
	m_smoothBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("smooth_inactive"));
	m_smoothBtn->setToolTip(tr("Smooth waveform"));


	m_interpolationToggle = new LedCheckBox("Interpolation", this, tr("Interpolation"), LedCheckBox::LedColor::Yellow);
	m_interpolationToggle->move(131, 221);


	m_normalizeToggle = new LedCheckBox("Normalize", this, tr("Normalize"), LedCheckBox::LedColor::Green);
	m_normalizeToggle->move(131, 236);
	
	connect(m_sinWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::sinWaveClicked);
	connect(m_triangleWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::triangleWaveClicked);
	connect(m_sawWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::sawWaveClicked);
	connect(m_sqrWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::sqrWaveClicked);
	connect(m_whiteNoiseWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::noiseWaveClicked);
	connect(m_usrWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::usrWaveClicked);
	connect(m_smoothBtn, &PixmapButton::clicked, this, &BitInvaderView::smoothClicked);
	connect(m_interpolationToggle, &LedCheckBox::toggled, this, &BitInvaderView::interpolationToggled);
	connect(m_normalizeToggle, &LedCheckBox::toggled, this, &BitInvaderView::normalizeToggled);
}




void BitInvaderView::modelChanged()
{
	auto b = castModel<BitInvader>();
	m_graph->setModel(&b->m_graph);
	m_sampleLengthKnob->setModel(&b->m_sampleLength);
	m_interpolationToggle->setModel(&b->m_interpolation);
	m_normalizeToggle->setModel(&b->m_normalize);
}




void BitInvaderView::sinWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSine();
	Engine::getSong()->setModified();
}




void BitInvaderView::triangleWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}




void BitInvaderView::sawWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSaw();
	Engine::getSong()->setModified();
}




void BitInvaderView::sqrWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSquare();
	Engine::getSong()->setModified();
}




void BitInvaderView::noiseWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToNoise();
	Engine::getSong()->setModified();
}




void BitInvaderView::usrWaveClicked()
{
	QString fileName = m_graph->model()->setWaveToUser();
	if (!fileName.isEmpty())
	{
		m_usrWaveBtn->setToolTip(fileName);
		m_graph->model()->clearInvisible();
		Engine::getSong()->setModified();
	}
}




void BitInvaderView::smoothClicked()
{
	m_graph->model()->smooth();
	Engine::getSong()->setModified();
}




void BitInvaderView::interpolationToggled(bool value)
{
	m_graph->setGraphStyle(value ? Graph::Style::Linear : Graph::Style::Nearest);
	Engine::getSong()->setModified();
}




void BitInvaderView::normalizeToggled(bool value)
{
	Engine::getSong()->setModified();
}


} // namespace gui

} // namespace lmms
