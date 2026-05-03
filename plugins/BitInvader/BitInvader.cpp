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
#include <QGridLayout>
#include <QLabel>

#include "AudioEngine.h"
#include "base64.h"
#include "embed.h"
#include "Engine.h"
#include "FontHelper.h"
#include "InstrumentTrack.h"
#include "lmms_math.h"
#include "NotePlayHandle.h"
#include "plugin_export.h"
#include "Song.h"


namespace lmms
{

inline constexpr std::size_t WavetableSize = 200;

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
	, m_sampleLength(WavetableSize, 4, WavetableSize, 1, this, tr("Sample length"))
	, m_graph(-1.f, 1.f, WavetableSize, this)
	, m_interpolation(false, this, tr("Interpolation"))
	, m_normalizeMode(this, tr("Normalize"))
{
	// The order these items are added must correspond to the modes in BitInvader::NormalizeMode
	m_normalizeMode.addItem(tr("Off"));         // NormalizeMode::Off
	m_normalizeMode.addItem(tr("Full"));        // NormalizeMode::Full
	m_normalizeMode.addItem(tr("Length only")); // NormalizeMode::LengthOnly
	m_normalizeMode.addItem(tr("Legacy"));      // NormalizeMode::Legacy
	m_normalizeMode.setValue(static_cast<int>(NormalizeMode::LengthOnly));

	m_graph.setWaveToSine();
	lengthChanged();

	connect(&m_sampleLength, &FloatModel::dataChanged, this, &BitInvader::lengthChanged, Qt::DirectConnection);
	connect(&m_normalizeMode, &IntModel::dataChanged, this, &BitInvader::normalize, Qt::DirectConnection);
	connect(&m_graph, &graphModel::samplesChanged, this, &BitInvader::normalize);
}




void BitInvader::saveSettings(QDomDocument& doc, QDomElement& el)
{
	el.setAttribute("version", "1");
	m_sampleLength.saveSettings(doc, el, "sampleLength");

	QString sampleString;
	base64::encode(reinterpret_cast<const char*>(m_graph.samples()), WavetableSize * sizeof(float), sampleString);
	el.setAttribute("sampleShape", sampleString);

	m_interpolation.saveSettings(doc, el, "interpolation");
	m_normalizeMode.saveSettings(doc, el, "normalize");
}




void BitInvader::loadSettings(const QDomElement& el)
{
	m_graph.clear();
	m_sampleLength.loadSettings(el, "sampleLength");

	// Load sample shape
	int size = 0;
	char* dst = 0;
	base64::decode(el.attribute("sampleShape"), &dst, &size);

	m_graph.setLength(size / sizeof(float));
	m_graph.setSamples(reinterpret_cast<float*>(dst));
	m_graph.setLength(static_cast<int>(m_sampleLength.value()));
	delete[] dst;

	m_interpolation.loadSettings(el, "interpolation");
	m_normalizeMode.loadSettings(el, "normalize");
	if (el.attribute("version").toInt() < 1)
	{
		m_normalizeMode.setValue(m_normalizeMode.value() * static_cast<int>(NormalizeMode::Legacy));
	}
}




void BitInvader::lengthChanged()
{
	m_graph.setLength(static_cast<int>(m_sampleLength.value()));
	normalize();
}




void BitInvader::normalize()
{
	if (m_normalizeMode.value() == static_cast<int>(NormalizeMode::Off))
	{
		m_normalizeFactor = 1.f;
		m_normalizeOffset = 0.f;
		return;
	}

	const auto samples = std::span<const float>{
		m_graph.samples(),
		m_normalizeMode.value() == static_cast<int>(NormalizeMode::LengthOnly)
			? static_cast<std::size_t>(m_sampleLength.value())
			: WavetableSize
		};

	if (m_normalizeMode.value() == static_cast<int>(NormalizeMode::Legacy))
	{
		m_normalizeOffset = 0.f;
		m_normalizeFactor = 1.f / *std::max_element(samples.begin(), samples.end(), [](auto a, auto b){ return std::abs(a) < std::abs(b); });
		return;
	}

	const auto [minSamp, maxSamp] = std::minmax_element(samples.begin(), samples.end());
	if (*minSamp == *maxSamp)
	{
		m_normalizeFactor = 0.f;
		m_normalizeOffset = 0.f;
		return;
	}
	const auto diff = *maxSamp - *minSamp;
	m_normalizeFactor = 2.f / diff;
	m_normalizeOffset = (*maxSamp + *minSamp) / -diff;
}




QString BitInvader::nodeName() const { return bitinvader_plugin_descriptor.name; }


void BitInvader::playNote(NotePlayHandle* nph, SampleFrame* workingBuffer)
{
	if (!nph->m_pluginData) { nph->m_pluginData = new BitInvaderIndex{}; }

	const auto& wavetable = m_graph.samples();
	const auto wavetableLenReal = static_cast<BitInvaderIndex>(m_graph.length());
	const auto phasePerSample = nph->frequency() / Engine::audioEngine()->outputSampleRate();
	const f_cnt_t frames = nph->framesLeftForCurrentPeriod();
	const f_cnt_t offset = nph->noteOffset();

	auto& note = *static_cast<BitInvaderIndex*>(nph->m_pluginData);
	for (f_cnt_t frame = offset; frame < frames + offset; ++frame)
	{
		note = std::fmod(
			note + phasePerSample * wavetableLenReal,
			wavetableLenReal
		);
		const auto idx = static_cast<std::size_t>(note);

		const auto samp = m_interpolation.value()
			? std::lerp(
				wavetable[idx],
				wavetable[(1 + idx) % m_graph.length()],
				fraction(note)
			)
			: wavetable[idx];

		workingBuffer[frame] = SampleFrame(samp * m_normalizeFactor + m_normalizeOffset);
	}
	applyRelease(workingBuffer, nph);
}




void BitInvader::deleteNotePluginData(NotePlayHandle* nph)
{
	delete static_cast<BitInvaderIndex*>(nph->m_pluginData);
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
	static auto s_artwork = PLUGIN_NAME::getIconPixmap("artwork");
	pal.setBrush(backgroundRole(), s_artwork);
	setPalette(pal);

	m_sampleLengthKnob = new Knob(KnobType::Dark28, this);
	m_sampleLengthKnob->move(6, 201);
	m_sampleLengthKnob->setHintText(tr("Sample length"), "");
	m_sampleLengthKnob->setLabel(tr("Length"));

	m_graph = new Graph(this, Graph::Style::Nearest, 204, 134);
	m_graph->move(23, 59);
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

	m_normalizeMode = new ComboBox(this, tr("Normalize"));
	m_normalizeMode->setToolTip(tr("Set wavetable normalization mode"));
	auto normalizeLabel = new QLabel(tr("Normalization:"), this);
	normalizeLabel->setFont(adjustedToPixelSize(normalizeLabel->font(), DEFAULT_FONT_SIZE));
	// FIXME: Evil awful terrible layout. This should be fine for now,
	// since a full UI overhaul for Bit Invader is planned in #8122.
	auto normalizeLayout = new QGridLayout(this);
	normalizeLayout->setContentsMargins(6, 6, 6, 6);
	normalizeLayout->setVerticalSpacing(1);
	normalizeLayout->setAlignment(Qt::AlignBottom);
	normalizeLayout->addItem(new QSpacerItem(250 - 131, 1), 0, 0); // HACK: Do NOT ever do this LMAO
	normalizeLayout->addWidget(normalizeLabel, 0, 1);
	normalizeLayout->addWidget(m_normalizeMode, 1, 1);

	connect(m_sinWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::sinWaveClicked);
	connect(m_triangleWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::triangleWaveClicked);
	connect(m_sawWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::sawWaveClicked);
	connect(m_sqrWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::sqrWaveClicked);
	connect(m_whiteNoiseWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::noiseWaveClicked);
	connect(m_usrWaveBtn, &PixmapButton::clicked, this, &BitInvaderView::usrWaveClicked);
	connect(m_smoothBtn, &PixmapButton::clicked, this, &BitInvaderView::smoothClicked);
	connect(m_interpolationToggle, &LedCheckBox::toggled, this, &BitInvaderView::interpolationToggled);
}




void BitInvaderView::modelChanged()
{
	auto b = castModel<BitInvader>();
	m_graph->setModel(&b->m_graph);
	m_sampleLengthKnob->setModel(&b->m_sampleLength);
	m_interpolationToggle->setModel(&b->m_interpolation);
	m_normalizeMode->setModel(&b->m_normalizeMode);
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
}




} // namespace gui

} // namespace lmms
