/*
 * Vibed.cpp - combination of PluckedStringSynth and BitInvader
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

#include "Vibed.h"

#include <array>
#include <cassert>
#include <QDomElement>

#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "NotePlayHandle.h"
#include "VibratingString.h"
#include "base64.h"
#include "CaptionMenu.h"
#include "volume.h"
#include "Song.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT vibedstrings_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Vibed",
	QT_TRANSLATE_NOOP("PluginBrowser", "Vibrating string modeler"),
	"Danny McRae <khjklujn/at/yahoo/com>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
	nullptr,
};

}


class Vibed::StringContainer
{
public:
	StringContainer(float pitch, sample_rate_t sampleRate, int bufferLength) :
		m_pitch(pitch), m_sampleRate(sampleRate), m_bufferLength(bufferLength) {}

	~StringContainer() = default;

	void addString(std::size_t harm, float pick, float pickup, const float* impulse, float randomize,
		float stringLoss, float detune, int oversample, bool state, int id)
	{
		constexpr auto octave = std::array{0.25f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
		assert(harm < octave.size());

		m_strings[id] = VibratingString{m_pitch * octave[harm], pick, pickup, impulse, m_bufferLength,
			m_sampleRate, oversample, randomize, stringLoss, detune, state};

		m_exists[id] = true;
	}

	bool exists(int id) const { return m_exists[id]; }

	sample_t getStringSample(int id) { return m_strings[id].nextSample(); }

private:
	const float m_pitch;
	const sample_rate_t m_sampleRate;
	const int m_bufferLength;
	std::array<VibratingString, s_stringCount> m_strings{};
	std::array<bool, s_stringCount> m_exists{};
};

Vibed::Vibed(InstrumentTrack* instrumentTrack) :
	Instrument(instrumentTrack, &vibedstrings_plugin_descriptor, nullptr, Flag::IsNotBendable)
{
	for (int harm = 0; harm < s_stringCount; ++harm)
	{
		m_volumeModels[harm] = std::make_unique<FloatModel>(
			DefaultVolume, MinVolume, MaxVolume, 1.0f, this, tr("String %1 volume").arg(harm + 1));
		m_stiffnessModels[harm] = std::make_unique<FloatModel>(
			0.0f, 0.0f, 0.05f, 0.001f, this, tr("String %1 stiffness").arg(harm + 1));
		m_pickModels[harm] = std::make_unique<FloatModel>(
			0.0f, 0.0f, 0.05f, 0.005f, this, tr("Pick %1 position").arg(harm + 1));
		m_pickupModels[harm] = std::make_unique<FloatModel>(
			0.05f, 0.0f, 0.05f, 0.005f, this, tr("Pickup %1 position").arg( harm + 1));
		m_panModels[harm] = std::make_unique<FloatModel>(
			0.0f, -1.0f, 1.0f, 0.01f, this, tr("String %1 panning").arg(harm + 1));
		m_detuneModels[harm] = std::make_unique<FloatModel>(
			0.0f, -0.1f, 0.1f, 0.001f, this, tr("String %1 detune").arg(harm + 1));
		m_randomModels[harm] = std::make_unique<FloatModel>(
			0.0f, 0.0f, 0.75f, 0.01f, this, tr("String %1 fuzziness").arg(harm + 1));
		m_lengthModels[harm] = std::make_unique<FloatModel>(
			1, 1, 16, 1, this, tr("String %1 length").arg(harm + 1));

		m_impulseModels[harm] = std::make_unique<BoolModel>(false, this, tr("Impulse %1").arg(harm + 1));
		m_powerModels[harm] = std::make_unique<BoolModel>(harm == 0, this, tr("String %1").arg(harm + 1));
		m_harmonicModels[harm] = std::make_unique<NineButtonSelectorModel>(2, 0, 8, this);
		m_graphModels[harm] = std::make_unique<graphModel>(-1.0, 1.0, s_sampleLength, this);
		m_graphModels[harm]->setWaveToSine();
	}
}

void Vibed::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	// Save plugin version
	elem.setAttribute("version", "0.2");

	for (int i = 0; i < s_stringCount; ++i)
	{
		const auto is = QString::number(i);

		elem.setAttribute("active" + is, QString::number(m_powerModels[i]->value()));

		m_volumeModels[i]->saveSettings(doc, elem, "volume" + is);
		m_stiffnessModels[i]->saveSettings(doc, elem, "stiffness" + is);
		m_pickModels[i]->saveSettings(doc, elem, "pick" + is);
		m_pickupModels[i]->saveSettings(doc, elem, "pickup" + is);
		m_harmonicModels[i]->saveSettings(doc, elem, "octave" + is);
		m_lengthModels[i]->saveSettings(doc, elem, "length" + is);
		m_panModels[i]->saveSettings(doc, elem, "pan" + is);
		m_detuneModels[i]->saveSettings(doc, elem, "detune" + is);
		m_randomModels[i]->saveSettings(doc, elem, "slap" + is);
		m_impulseModels[i]->saveSettings(doc, elem, "impulse" + is);

		QString sampleString;
		base64::encode((const char*)m_graphModels[i]->samples(), s_sampleLength * sizeof(float), sampleString);

		elem.setAttribute("graph" + is, sampleString);
	}
}

void Vibed::loadSettings(const QDomElement& elem)
{
	// Load plugin version
	bool newVersion = false;
	if (elem.hasAttribute("version"))
	{
		newVersion = elem.attribute("version").toFloat() >= 0.2f;
	}

	for (int i = 0; i < s_stringCount; ++i)
	{
		const auto is = QString::number(i);

		m_powerModels[i]->setValue(elem.attribute("active" + is).toInt());

		// Version 0.2 saves/loads all instrument data unconditionally
		const bool hasVolumeAttr = elem.hasAttribute("volume" + is) || !elem.firstChildElement("volume" + is).isNull();
		if (newVersion || (m_powerModels[i]->value() && hasVolumeAttr))
		{
			m_volumeModels[i]->loadSettings(elem, "volume" + is);
			m_stiffnessModels[i]->loadSettings(elem, "stiffness" + is);
			m_pickModels[i]->loadSettings(elem, "pick" + is);
			m_pickupModels[i]->loadSettings(elem, "pickup" + is);
			m_harmonicModels[i]->loadSettings(elem, "octave" + is);
			m_lengthModels[i]->loadSettings(elem, "length" + is);
			m_panModels[i]->loadSettings(elem, "pan" + is);
			m_detuneModels[i]->loadSettings(elem, "detune" + is);
			m_randomModels[i]->loadSettings(elem, "slap" + is);
			m_impulseModels[i]->loadSettings(elem, "impulse" + is);

			int size = 0;
			float* shp = nullptr;
			base64::decode(elem.attribute("graph" + is), &shp, &size);

			assert(size == 128 * sizeof(float));
			m_graphModels[i]->setSamples(shp);
			delete[] shp;
		}
	}
}

QString Vibed::nodeName() const
{
	return vibedstrings_plugin_descriptor.name;
}

void Vibed::playNote(NotePlayHandle* n, SampleFrame* workingBuffer)
{
	if (!n->m_pluginData)
	{
		const auto newContainer = new StringContainer{n->frequency(),
			Engine::audioEngine()->outputSampleRate(), s_sampleLength};

		n->m_pluginData = newContainer;

		for (int i = 0; i < s_stringCount; ++i)
		{
			if (m_powerModels[i]->value())
			{
				newContainer->addString(
					m_harmonicModels[i]->value(),
					m_pickModels[i]->value(),
					m_pickupModels[i]->value(),
					m_graphModels[i]->samples(),
					m_randomModels[i]->value(),
					m_stiffnessModels[i]->value(),
					m_detuneModels[i]->value(),
					static_cast<int>(m_lengthModels[i]->value()),
					m_impulseModels[i]->value(),
					i);
			}
		}
	}

	const fpp_t frames = n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = n->noteOffset();
	auto ps = static_cast<StringContainer*>(n->m_pluginData);

	for (fpp_t i = offset; i < frames + offset; ++i)
	{
		workingBuffer[i][0] = 0.0f;
		workingBuffer[i][1] = 0.0f;
		for (int str = 0; str < s_stringCount; ++str)
		{
			if (ps->exists(str))
			{
				// pan: 0 -> left, 1 -> right
				const float pan = (m_panModels[str]->value() + 1) / 2.0f;
				const sample_t sample = ps->getStringSample(str) * m_volumeModels[str]->value() / 100.0f;
				workingBuffer[i][0] += (1.0f - pan) * sample;
				workingBuffer[i][1] += pan * sample;
			}
		}
	}
}

void Vibed::deleteNotePluginData(NotePlayHandle* n)
{
	delete static_cast<StringContainer*>(n->m_pluginData);
}

gui::PluginView* Vibed::instantiateView(QWidget* parent)
{
	return new gui::VibedView(this, parent);
}


namespace gui
{


VibedView::VibedView(Instrument* instrument, QWidget* parent) :
	InstrumentViewFixedSize(instrument, parent),
	m_volumeKnob(KnobType::Bright26, this),
	m_stiffnessKnob(KnobType::Bright26, this),
	m_pickKnob(KnobType::Bright26, this),
	m_pickupKnob(KnobType::Bright26, this),
	m_panKnob(KnobType::Bright26, this),
	m_detuneKnob(KnobType::Bright26, this),
	m_randomKnob(KnobType::Bright26, this),
	m_lengthKnob(KnobType::Bright26, this),
	m_graph(this),
	m_impulse("", this),
	m_power("", this, tr("Enable waveform")),
	m_smoothBtn(this, tr("Smooth waveform")),
	m_normalizeBtn(this, tr("Normalize waveform")),
	m_sinWaveBtn(this, tr("Sine wave")),
	m_triangleWaveBtn(this, tr("Triangle wave")),
	m_sawWaveBtn(this, tr("Saw wave")),
	m_sqrWaveBtn(this, tr("Square wave")),
	m_whiteNoiseWaveBtn(this, tr("White noise")),
	m_usrWaveBtn(this, tr("User-defined wave"))
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	m_volumeKnob.setVolumeKnob(true);
	m_volumeKnob.move(103, 142);
	m_volumeKnob.setHintText(tr("String volume:"), "");

	m_stiffnessKnob.move(129, 142);
	m_stiffnessKnob.setHintText(tr("String stiffness:"), "");

	m_pickKnob.move(153, 142);
	m_pickKnob.setHintText(tr("Pick position:"), "");

	m_pickupKnob.move(177, 142);
	m_pickupKnob.setHintText(tr("Pickup position:"), "");

	m_panKnob.move(105, 187);
	m_panKnob.setHintText(tr("String panning:"), "");

	m_detuneKnob.move(150, 187);
	m_detuneKnob.setHintText(tr("String detune:"), "");

	m_randomKnob.move(194, 187);
	m_randomKnob.setHintText(tr("String fuzziness:"), "");

	m_lengthKnob.move(23, 193);
	m_lengthKnob.setHintText(tr("String length:"), "");

	m_graph.setWindowTitle(tr("Impulse Editor"));
	m_graph.setForeground(PLUGIN_NAME::getIconPixmap("wavegraph4"));
	m_graph.move(76, 21);
	m_graph.resize(132, 104);

	m_impulse.move(23, 94);
	m_impulse.setToolTip(tr("Impulse"));

	m_power.move(212, 130);
	m_power.setToolTip(tr("Enable/disable string"));

	m_harmonic = std::make_unique<NineButtonSelector>(
		std::array{
			PLUGIN_NAME::getIconPixmap("button_-2_on"),
			PLUGIN_NAME::getIconPixmap("button_-2_off"),
			PLUGIN_NAME::getIconPixmap("button_-1_on"),
			PLUGIN_NAME::getIconPixmap("button_-1_off"),
			PLUGIN_NAME::getIconPixmap("button_f_on"),
			PLUGIN_NAME::getIconPixmap("button_f_off"),
			PLUGIN_NAME::getIconPixmap("button_2_on"),
			PLUGIN_NAME::getIconPixmap("button_2_off"),
			PLUGIN_NAME::getIconPixmap("button_3_on"),
			PLUGIN_NAME::getIconPixmap("button_3_off"),
			PLUGIN_NAME::getIconPixmap("button_4_on"),
			PLUGIN_NAME::getIconPixmap("button_4_off"),
			PLUGIN_NAME::getIconPixmap("button_5_on"),
			PLUGIN_NAME::getIconPixmap("button_5_off"),
			PLUGIN_NAME::getIconPixmap("button_6_on"),
			PLUGIN_NAME::getIconPixmap("button_6_off"),
			PLUGIN_NAME::getIconPixmap("button_7_on"),
			PLUGIN_NAME::getIconPixmap("button_7_off")},
		2,
		21, 127,
		this);

	m_harmonic->setWindowTitle(tr("Octave"));

	m_stringSelector = std::make_unique<NineButtonSelector>(
		std::array{
			PLUGIN_NAME::getIconPixmap("button_1_on"),
			PLUGIN_NAME::getIconPixmap("button_1_off"),
			PLUGIN_NAME::getIconPixmap("button_2_on"),
			PLUGIN_NAME::getIconPixmap("button_2_off"),
			PLUGIN_NAME::getIconPixmap("button_3_on"),
			PLUGIN_NAME::getIconPixmap("button_3_off"),
			PLUGIN_NAME::getIconPixmap("button_4_on"),
			PLUGIN_NAME::getIconPixmap("button_4_off"),
			PLUGIN_NAME::getIconPixmap("button_5_on"),
			PLUGIN_NAME::getIconPixmap("button_5_off"),
			PLUGIN_NAME::getIconPixmap("button_6_on"),
			PLUGIN_NAME::getIconPixmap("button_6_off"),
			PLUGIN_NAME::getIconPixmap("button_7_on"),
			PLUGIN_NAME::getIconPixmap("button_7_off"),
			PLUGIN_NAME::getIconPixmap("button_8_on"),
			PLUGIN_NAME::getIconPixmap("button_8_off"),
			PLUGIN_NAME::getIconPixmap("button_9_on"),
			PLUGIN_NAME::getIconPixmap("button_9_off")},
		0,
		21, 39,
		this);

	// String selector is not a part of the model
	m_stringSelector->setWindowTitle(tr("String"));

	connect(m_stringSelector.get(), SIGNAL(NineButtonSelection(int)), this, SLOT(showString(int)));

	showString(0);

	m_smoothBtn.move(79, 129);
	m_smoothBtn.setActiveGraphic(PLUGIN_NAME::getIconPixmap("smooth_active"));
	m_smoothBtn.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("smooth_inactive"));
	m_smoothBtn.setChecked(false);
	m_smoothBtn.setToolTip(tr("Smooth waveform"));
	connect(&m_smoothBtn, SIGNAL(clicked()), this, SLOT(smoothClicked()));

	m_normalizeBtn.move(96, 129);
	m_normalizeBtn.setActiveGraphic(PLUGIN_NAME::getIconPixmap("normalize_active"));
	m_normalizeBtn.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("normalize_inactive"));
	m_normalizeBtn.setChecked(false);
	m_normalizeBtn.setToolTip(tr("Normalize waveform"));
	connect(&m_normalizeBtn, SIGNAL(clicked()), this, SLOT(normalizeClicked()));

	m_sinWaveBtn.move(212, 24);
	m_sinWaveBtn.setActiveGraphic(embed::getIconPixmap("sin_wave_active"));
	m_sinWaveBtn.setInactiveGraphic(embed::getIconPixmap("sin_wave_inactive"));
	m_sinWaveBtn.setToolTip(tr("Sine wave"));
	connect(&m_sinWaveBtn, SIGNAL(clicked()), this, SLOT(sinWaveClicked()));

	m_triangleWaveBtn.move(212, 41);
	m_triangleWaveBtn.setActiveGraphic(embed::getIconPixmap("triangle_wave_active"));
	m_triangleWaveBtn.setInactiveGraphic(embed::getIconPixmap("triangle_wave_inactive"));
	m_triangleWaveBtn.setToolTip(tr("Triangle wave"));
	connect(&m_triangleWaveBtn, SIGNAL(clicked()), this, SLOT(triangleWaveClicked()));

	m_sawWaveBtn.move(212, 58);
	m_sawWaveBtn.setActiveGraphic(embed::getIconPixmap("saw_wave_active"));
	m_sawWaveBtn.setInactiveGraphic(embed::getIconPixmap("saw_wave_inactive"));
	m_sawWaveBtn.setToolTip(tr("Saw wave"));
	connect(&m_sawWaveBtn, SIGNAL(clicked()), this, SLOT(sawWaveClicked()));

	m_sqrWaveBtn.move(212, 75);
	m_sqrWaveBtn.setActiveGraphic(embed::getIconPixmap("square_wave_active"));
	m_sqrWaveBtn.setInactiveGraphic(embed::getIconPixmap("square_wave_inactive"));
	m_sqrWaveBtn.setToolTip(tr("Square wave"));
	connect(&m_sqrWaveBtn, SIGNAL(clicked()), this, SLOT(sqrWaveClicked()));

	m_whiteNoiseWaveBtn.move(212, 92);
	m_whiteNoiseWaveBtn.setActiveGraphic(embed::getIconPixmap("white_noise_wave_active"));
	m_whiteNoiseWaveBtn.setInactiveGraphic(embed::getIconPixmap("white_noise_wave_inactive"));
	m_whiteNoiseWaveBtn.setToolTip(tr("White noise"));
	connect(&m_whiteNoiseWaveBtn, SIGNAL(clicked()), this, SLOT(noiseWaveClicked()));

	m_usrWaveBtn.move(212, 109);
	m_usrWaveBtn.setActiveGraphic(embed::getIconPixmap("usr_wave_active"));
	m_usrWaveBtn.setInactiveGraphic(embed::getIconPixmap("usr_wave_inactive"));
	m_usrWaveBtn.setToolTip(tr("User-defined wave"));
	connect(&m_usrWaveBtn, SIGNAL(clicked()),this, SLOT(usrWaveClicked()));
}

void VibedView::modelChanged()
{
	showString(0);
}

void VibedView::showString(int str)
{
	auto v = castModel<Vibed>();

	m_pickKnob.setModel(v->m_pickModels[str].get());
	m_pickupKnob.setModel(v->m_pickupModels[str].get());
	m_stiffnessKnob.setModel(v->m_stiffnessModels[str].get());
	m_volumeKnob.setModel(v->m_volumeModels[str].get());
	m_panKnob.setModel(v->m_panModels[str].get());
	m_detuneKnob.setModel(v->m_detuneModels[str].get());
	m_randomKnob.setModel(v->m_randomModels[str].get());
	m_lengthKnob.setModel(v->m_lengthModels[str].get());
	m_graph.setModel(v->m_graphModels[str].get());
	m_impulse.setModel(v->m_impulseModels[str].get());
	m_harmonic->setModel(v->m_harmonicModels[str].get());
	m_power.setModel(v->m_powerModels[str].get());
}

void VibedView::sinWaveClicked()
{
	m_graph.model()->setWaveToSine();
	Engine::getSong()->setModified();
}

void VibedView::triangleWaveClicked()
{
	m_graph.model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}

void VibedView::sawWaveClicked()
{
	m_graph.model()->setWaveToSaw();
	Engine::getSong()->setModified();
}

void VibedView::sqrWaveClicked()
{
	m_graph.model()->setWaveToSquare();
	Engine::getSong()->setModified();
}

void VibedView::noiseWaveClicked()
{
	m_graph.model()->setWaveToNoise();
	Engine::getSong()->setModified();
}

void VibedView::usrWaveClicked()
{
	QString fileName = m_graph.model()->setWaveToUser();
	m_usrWaveBtn.setToolTip(fileName);
	Engine::getSong()->setModified();
}

void VibedView::smoothClicked()
{
	m_graph.model()->smooth();
	Engine::getSong()->setModified();
}

void VibedView::normalizeClicked()
{
	m_graph.model()->normalize();
	Engine::getSong()->setModified();
}

void VibedView::contextMenuEvent(QContextMenuEvent*)
{
	CaptionMenu contextMenu(model()->displayName(), this);
	contextMenu.exec(QCursor::pos());
}


} // namespace gui

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
	return new Vibed(static_cast<InstrumentTrack*>(m));
}

}


} // namespace lmms
