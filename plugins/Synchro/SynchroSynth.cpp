/*
 * SynchroSynth.cpp - 2-oscillator PM synth
 *
 * Copyright (c) 2023 rubiefawn <rubiefawn/at/gmail/dot/com>
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
 */

#include "SynchroSynth.h"

#include <QDomElement>

#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "lmms_constants.h" // for D_2PI & D_PI_2 bc yummy
#include "lmms_math.h"

#include "hiir/PolyphaseIir2Designer.h"
#include "hiir/Downsampler2xF64Fpu.h" //TODO: add optional SIMD support once i get any version of this working at all

#include "plugin_export.h" // see `BuildPlugin.cmake`


namespace lmms {

//=====================//
// ~ plugin metadata ~ //
//=====================//
extern "C" {
	Plugin::Descriptor PLUGIN_EXPORT synchro_plugin_descriptor = {
		LMMS_STRINGIFY(PLUGIN_NAME), // see `lmms_basics.h`, `CMakeLists.txt`
		"Synchro",
		QT_TRANSLATE_NOOP("PluginBrowser", "2-oscillator PM synth"),
		"rubiefawn <rubiefawn/at/gmail/dot/com>",
		// denotes the plugin version, but in what format?
		// see `Plugin.h:99`
		// every single plugin has a value of 0x0100 except:
		// ReverbSC (0x0123), Analyzer (0x0112), TripleOscillator (0x0110), VstEffect (0x0200)
		0x0100,
		Plugin::Instrument,
		new PluginPixmapLoader("logo"),
		nullptr, // const char* supportedFileTypes
		nullptr, // SubPluginFeatures* subPluginFeatures
	};

	// the void* doesn't seem to be used by any of the native lmms instrument plugins
	// also, i can't seem to find the Model class so not sure what its properties are
	PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* model, void* _) {
		return new SynchroInstrument(static_cast<InstrumentTrack*>(model));
	}
} //extern "C"

//===========================//
// ~ math helper functions ~ //
//===========================//

static inline double reduce_phase(const double phase) {
    return phase - D_2PI * floorf(phase / D_2PI);
}

static inline double triangle(const double phase) {
	return 2.0 * fabs(
		2.0 * (
			phase / D_2PI - floorf(phase / D_2PI + 0.5)
		)
	) - 1.0;
}

static inline double trimonics(const double phase, const double harmonics_strength) {
	return triangle(phase) +
		( 
			triangle(MAGIC_HARMONICS[0].overtone * phase) * MAGIC_HARMONICS[0].amplitude +
			triangle(MAGIC_HARMONICS[1].overtone * phase) * MAGIC_HARMONICS[1].amplitude
		) * harmonics_strength;
}

//TODO: where did i copy this from? needs attribution
static inline double fast_tanh(const double x) {
	const double absX = fabs(x);
	const double sqrX = x * x;
	const double z = x * (
		0.773062670268356 + absX + (
			0.757118539838817 + 0.0139332362248817 * sqrX * sqrX
		) * sqrX * absX
	);
	return z / (0.795956503022967 + fabs(z));
}

static inline double synchro_waveform(
	const double phase,
	const double drive,
	const double sync,
	const double pulse,
	const double harmonics_strength
) {
	return fast_tanh(
		trimonics(D_PI_2 + phase * sync, harmonics_strength) *
		drive / powf(D_2PI / (D_2PI - phase), pulse)
	);
}

//==============//
// ~ gui code ~ //
//==============//
namespace gui {

//-------------//
// SynchroView //
//-------------//
SynchroView::SynchroView(Instrument* instrument, QWidget* parent) :
InstrumentViewFixedSize(instrument, parent) {
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// `Knob.h:90` // TODO: remove
	// `Knob.h:91` setHintText(const QString &_txt_before, const QString &_txt_after)
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// what future-proof solution should I be using here instead of `setHintText()`?

	// general controls
	m_resultingWaveform = new Graph(this, Graph::LinearStyle, 48, 42);
	m_resultingWaveform->move(5, 73);
	m_resultingWaveform->setAutoFillBackground(false);
	m_resultingWaveform->setGraphColor(QColor(255, 255, 255));
	m_resultingWaveform->setEnabled(false); // read-only, no drawing allowed!!

	m_modAmount = new Knob(knobDark_28, this);
	m_modAmount->move(89, 80);
	m_modAmount->setHintText(tr("modulation"), "×"); //TODO: make the UI show 0-100 %

	m_harmonics = new Knob(knobDark_28, this);
	m_harmonics->move(185, 80);
	m_harmonics->setHintText(tr("modulator harmonics"), "×"); //TODO: make the UI show 0-100 %

	m_modRange = new Knob(knobDark_28, this);
	m_modRange->move(281, 80);
	m_modRange->setHintText(tr("modulation range"), "×"); //TODO: make the UI show 0-100 %

	// carrier controls
	m_carrierWaveform = new Graph(this, Graph::LinearStyle, 48, 42);
	m_carrierWaveform->move(5, 138);
	m_carrierWaveform->setAutoFillBackground(false);
	m_carrierWaveform->setGraphColor(QColor(255, 255, 255));
	m_carrierWaveform->setEnabled(false); // read-only, no drawing allowed!!

	m_carrierOctave = new Knob(knobDark_28, this);
	m_carrierOctave->move(64, 144);
	m_carrierOctave->setHintText(tr("voice detune"), "octaves");

	m_carrierDrive = new Knob(knobDark_28, this);
	m_carrierDrive->move(113, 144);
	m_carrierDrive->setHintText(tr("voice drive"), "×");

	m_carrierSync = new Knob(knobDark_28, this);
	m_carrierSync->move(161, 144);
	m_carrierSync->setHintText(tr("voice sync"), "×");

	m_carrierPulse = new Knob(knobDark_28, this);
	m_carrierPulse->move(208, 144);
	m_carrierPulse->setHintText(tr("voice pulse"), "");

	// modulator controls
	m_modulatorWaveform = new Graph(this, Graph::LinearStyle, 48, 42);
	m_modulatorWaveform->move(5, 203);
	m_modulatorWaveform->setAutoFillBackground(false);
	m_modulatorWaveform->setGraphColor(QColor(255, 255, 255));
	m_modulatorWaveform->setEnabled(false); // read-only, no drawing allowed!!

	m_modulatorOctave = new Knob(knobDark_28, this);
	m_modulatorOctave->move(64, 210);
	m_modulatorOctave->setHintText(tr("modulator detune"), "octaves");

	m_modulatorDrive = new Knob(knobDark_28, this);
	m_modulatorDrive->move(113, 210);
	m_modulatorDrive->setHintText(tr("modulator drive"), "×");

	m_modulatorSync = new Knob(knobDark_28, this);
	m_modulatorSync->move(161, 210);
	m_modulatorSync->setHintText(tr("modulator sync"), "×");

	m_modulatorPulse = new Knob(knobDark_28, this);
	m_modulatorPulse->move(208, 210);
	m_modulatorPulse->setHintText(tr("modulator pulse"), "");
} //SynchroView::SynchroView()

void SynchroView::modelChanged() {
	SynchroInstrument* s = castModel<SynchroInstrument>(); // see `ModelView.h:54`

	// general controls
	m_resultingWaveform->setModel(&s->m_resultingWaveform);
	m_harmonics->setModel(&s->m_harmonics);
	m_modRange->setModel(&s->m_modRange);
	m_modAmount->setModel(&s->m_modAmount);

	// carrier oscillator
	m_carrierWaveform->setModel(&s->m_carrierWaveform);
	m_carrierOctave->setModel(&s->m_carrierOctave);
	m_carrierDrive->setModel(&s->m_carrierDrive);
	m_carrierSync->setModel(&s->m_carrierSync);
	m_carrierPulse->setModel(&s->m_carrierPulse);

	// modulator oscillator
	m_modulatorWaveform->setModel(&s->m_modulatorWaveform);
	m_modulatorOctave->setModel(&s->m_modulatorOctave);
	m_modulatorDrive->setModel(&s->m_modulatorDrive);
	m_modulatorSync->setModel(&s->m_modulatorSync);
	m_modulatorPulse->setModel(&s->m_modulatorPulse);
} //SynchroView::modelChanged()

}//namespace gui

//================//
// ~ synth code ~ //
//================//

//--------------//
// SynchroSynth //
//--------------//
SynchroSynth::SynchroSynth(NotePlayHandle* notePlayHandle) : m_nph(notePlayHandle) {}

double SynchroSynth::nextSample(
			const float modStrength,
			const float modRange,
			const float modulatorHarmonics,
			const float carrierOctave,
			const float modulatorOctave,
			const float carrierDrive,
			const float modulatorDrive,
			const float carrierSync,
			const float modulatorSync,
			const float carrierPulse,
			const float modulatorPulse
		) {
	const double radians_per_subsample =
		m_nph->frequency() * D_2PI /
		(Engine::audioEngine()->processingSampleRate() * SYNCHRO_OVERSAMPLING_FACTOR);

	m_modulatorPhase = reduce_phase(m_modulatorPhase + radians_per_subsample * exp2(modulatorOctave));
	double phase_modulation = synchro_waveform(
		m_modulatorPhase,
		modulatorDrive,
		modulatorSync,
		modulatorPulse,
		modulatorHarmonics
	) * D_2PI * modStrength * modRange * SYNCHRO_PM_BASE;

	m_carrierPhase = reduce_phase(m_carrierPhase + radians_per_subsample * exp2(carrierOctave));
	return synchro_waveform(
		reduce_phase(m_carrierPhase + phase_modulation),
		carrierDrive,
		carrierSync,
		carrierPulse,
		0.0 // no harmonics for you!!!!
	) * SYNCHRO_CLIP_INHIBITOR;
} //SynchroSynth::nextSample()
//SynchroSynth

//-------------------//
// SynchroInstrument //
//-------------------//
SynchroInstrument::SynchroInstrument(InstrumentTrack* instrument_track) :
// initialize five thousand members2
Instrument(instrument_track, &synchro_plugin_descriptor),
// general
m_resultingWaveform(-1.0f, 1.0f, SYNCHRO_WAVEFORM_GUI_SAMPLES, this),
m_harmonics(0, 0, 1.0f, 0.01f, this, tr("modulator harmonics")),
m_modRange(0.5, 0, 1, 0.01f, this, tr("modulation range")),
m_modAmount(0, 0, 1, 0.0001f, this, tr("modulation")),
// carrier oscillator
m_carrierWaveform(-1.0f, 1.0f, SYNCHRO_WAVEFORM_GUI_SAMPLES, this),
m_carrierOctave(0, -4, 0, 1, this, tr("carrier octave")),
m_carrierDrive(1, 1, 7, 0.01f, this, tr("carrier drive")),
m_carrierSync(1, 1, 16, 0.01f, this, tr("carrier sync")),
m_carrierPulse(0, 0, 4, 0.01f, this, tr("carrier pulse")),
// modulator oscillator
m_modulatorWaveform(-1.0f, 1.0f, SYNCHRO_WAVEFORM_GUI_SAMPLES, this),
m_modulatorOctave(0, -4, 0, 1, this, tr("modulator octave")),
m_modulatorDrive(1, 1, 7, 0.01f, this, tr("modulator drive")),
m_modulatorSync(1, 1, 16, 0.01f, this, tr("modulator sync")),
m_modulatorPulse(0, 0, 4, 0.01f, this, tr("modulator pulse"))
/* actual function starts here */ {
	// all the Qt signal stuff goes here
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
	// update carrier waveform preview when its parameters are changed
	connect(&m_carrierOctave, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrierDrive, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrierSync, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrierPulse, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	// update modulator waveform preview when its parameters are changed
	connect(&m_modulatorOctave, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulatorDrive, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulatorSync, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulatorPulse, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	// update result waveform preview when its parameters are changed
	// the result waveform is also updated when any of the above slots are called
	connect(&m_modAmount, SIGNAL(dataChanged()), this, SLOT(generalChanged()));
	connect(&m_harmonics, SIGNAL(dataChanged()), this, SLOT(generalChanged()));
	connect(&m_modRange, SIGNAL(dataChanged()), this, SLOT(generalChanged()));

	// wake up sleepyhead we have a gui to draw
	// each of these functions also call generalChanged() so no need to do it again here
	carrierChanged();
	modulatorChanged();

	// set up oversampling
	sampleRateChanged();
} //SynchroInstrument::SynchroInstrument()

QString SynchroInstrument::nodeName() const { return synchro_plugin_descriptor.name; }

void SynchroInstrument::playNote(NotePlayHandle* nph, sampleFrame* working_buffer) {
	// this seems to be part of what makes this synth polyphonic
	// probably need to study Lb302 to see how to make it monophonic
	if (nph->m_pluginData == nullptr) { nph->m_pluginData = new SynchroSynth(nph); }
	
	SynchroSynth* synth = static_cast<SynchroSynth*>(nph->m_pluginData);
	const fpp_t frames = nph->framesLeftForCurrentPeriod();
	const f_cnt_t framesOffset = nph->noteOffset();

	// makes sample-exact data accessible (when available)
	const ValueBuffer* harmonicsBuffer = m_harmonics.valueBuffer();
	const ValueBuffer* modRangeBuffer = m_modRange.valueBuffer();
	const ValueBuffer* modAmountBuffer = m_modAmount.valueBuffer();
	const ValueBuffer* carrierOctaveBuffer = m_carrierOctave.valueBuffer();
	const ValueBuffer* carrierDriveBuffer = m_carrierDrive.valueBuffer();
	const ValueBuffer* carrierSyncBuffer = m_carrierSync.valueBuffer();
	const ValueBuffer* carrierPulseBuffer = m_carrierPulse.valueBuffer();
	const ValueBuffer* modulatorOctaveBuffer = m_modulatorOctave.valueBuffer();
	const ValueBuffer* modulatorDriveBuffer = m_modulatorDrive.valueBuffer();
	const ValueBuffer* modulatorSyncBuffer = m_modulatorSync.valueBuffer();
	const ValueBuffer* modulatorPulseBuffer = m_modulatorPulse.valueBuffer();

	for (fpp_t supersample = 0; supersample < frames * SYNCHRO_OVERSAMPLING_FACTOR; ++supersample) {
		const fpp_t frame = supersample / SYNCHRO_OVERSAMPLING_FACTOR + framesOffset;
		m_downsamplingBuffer[0][supersample] = synth->nextSample(
			modAmountBuffer       ? modAmountBuffer->value(frame)       : m_modAmount.value(),
			modRangeBuffer        ? modRangeBuffer->value(frame)        : m_modRange.value(),
			harmonicsBuffer       ? harmonicsBuffer->value(frame)       : m_harmonics.value(),
			carrierOctaveBuffer   ? carrierOctaveBuffer->value(frame)   : m_carrierOctave.value(),
			modulatorOctaveBuffer ? modulatorOctaveBuffer->value(frame) : m_modulatorOctave.value(),
			carrierDriveBuffer    ? carrierDriveBuffer->value(frame)    : m_carrierDrive.value(),
			modulatorDriveBuffer  ? modulatorDriveBuffer->value(frame)  : m_modulatorDrive.value(),
			carrierSyncBuffer     ? carrierSyncBuffer->value(frame)     : m_carrierSync.value(),
			modulatorSyncBuffer   ? modulatorSyncBuffer->value(frame)   : m_modulatorSync.value(),
			carrierPulseBuffer    ? carrierPulseBuffer->value(frame)    : m_carrierPulse.value(),
			modulatorPulseBuffer  ? modulatorPulseBuffer->value(frame)  : m_modulatorPulse.value()
		);
	}

	// can only downsample 2x at a time, so just do it over and over again until it's reached normal sample rate
	uint8_t which_buffer = 0; // also used after this loop to determine which index contains the final output buffer
	for (
		fpp_t downsample_buf_len = frames * SYNCHRO_OVERSAMPLING_FACTOR;
		downsample_buf_len > frames;
		downsample_buf_len >>= 1, which_buffer ^= 1
	) {
		// // idk what i'm doing, this is just silencing the audio lmao
		// m_downsamplingFilter.process_block(
		// 	&m_downsamplingBuffer[which_buffer    ][0],
		// 	&m_downsamplingBuffer[which_buffer ^ 1][0],
		// 	downsample_buf_len
		// );

		// simple downsampling via sample averaging for testing purposes
		// this is better than nothing, but i'd rather get hiir working
		for (fpp_t sample_idx = 0; sample_idx < downsample_buf_len; ++sample_idx) {
			m_downsamplingBuffer       [which_buffer ^ 1][    sample_idx    ]
				= (m_downsamplingBuffer[which_buffer    ][2 * sample_idx    ]
				+  m_downsamplingBuffer[which_buffer    ][2 * sample_idx + 1])
				/ 2.0;
		}
	}

	// working_buffer starts at framesOffset, but m_downsamplingBuffer starts at 0
	for (f_cnt_t frame = framesOffset; frame < frames + framesOffset; ++frame) {
		// is this worth doing or should i just assume working_buffer is stereo (`DEFAULT_CHANNELS`)?
		// output from this synth is mono anyways
		for (auto& chnl : working_buffer[frame]) {
			chnl = static_cast<sample_t>(m_downsamplingBuffer[which_buffer][frame - framesOffset]);
		}
	}

	applyRelease(working_buffer, nph);
	instrumentTrack()->processAudioBuffer(working_buffer, frames + framesOffset, nph);
} //SynchroInstrument::playNote()

void SynchroInstrument::sampleRateChanged() {
	// set up the downsampling filter
	double iir_coefficients[SYNCHRO_OVERSAMPLING_FILTER_COEFFICIENT_COUNT];
	hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(
		iir_coefficients,
		SYNCHRO_OVERSAMPLING_FILTER_COEFFICIENT_COUNT,
		0.04 // surely this depends on the sample rate? i don't know what i'm doing here
	);
	m_downsamplingFilter.set_coefs(iir_coefficients);

	m_downsamplingBuffer[0].resize(SYNCHRO_OVERSAMPLING_FACTOR * Engine::audioEngine()->framesPerPeriod());
	m_downsamplingBuffer[1].resize(SYNCHRO_OVERSAMPLING_FACTOR * Engine::audioEngine()->framesPerPeriod());
} //SynchroInstrument::sampleRateChanged()

void SynchroInstrument::carrierChanged() {
	for (int i = 0; i < SYNCHRO_WAVEFORM_GUI_SAMPLES; ++i) {
		double phase = i * D_2PI / SYNCHRO_WAVEFORM_GUI_SAMPLES;
		m_carrierWaveform.setSampleAt(
			i,
			synchro_waveform(
				phase,
				m_carrierDrive.value(),
				m_carrierSync.value(),
				m_carrierPulse.value(),
				0.0
			)
		);
	}
	generalChanged(); // update the general waveform view as well, since that is dependent on both osc settings
} //SynchroInstrument::carrierChanged()

void SynchroInstrument::modulatorChanged() {
	for (int i = 0; i < SYNCHRO_WAVEFORM_GUI_SAMPLES; ++i) {
		double phase = i * D_2PI / SYNCHRO_WAVEFORM_GUI_SAMPLES;
		m_modulatorWaveform.setSampleAt(
			i,
			synchro_waveform(
				phase,
				m_modulatorDrive.value(),
				m_modulatorSync.value(),
				m_modulatorPulse.value(),
				m_harmonics.value()
			)
		);
	}
	generalChanged(); // update the general waveform view as well, since that is dependent on both osc settings
} //SynchroInstrument::modulatorChanged()

void SynchroInstrument::generalChanged() {
	// difference between the octaves of the two oscillators determines the optimal period for the waveform view
	const int octaveDiff = m_carrierOctave.value() - m_modulatorOctave.value();
	const float pitchDifference = powf(2, octaveDiff);

	for (int i = 0; i < SYNCHRO_WAVEFORM_GUI_SAMPLES; ++i) {
		double phase = reduce_phase(i * D_2PI / SYNCHRO_WAVEFORM_GUI_SAMPLES);

		float phaseMod = synchro_waveform(
			phase,
			m_modulatorDrive.value(),
			m_modulatorSync.value(),
			m_modulatorPulse.value(),
			m_harmonics.value()
		) * m_modRange.value() * m_modAmount.value() * SYNCHRO_PM_BASE;

		phase = reduce_phase(phase + phaseMod * pitchDifference);

		m_resultingWaveform.setSampleAt(
			i,
			synchro_waveform(
				phase,
				m_carrierDrive.value(),
				m_carrierSync.value(),
				m_carrierPulse.value(),
				0.0
			)
		);
	}
} //SynchroInstrument::generalChanged()

void SynchroInstrument::saveSettings(QDomDocument& doc, QDomElement& thisElement) {
	// does this ever get used? should i use it to implement backwards compatibility in this function?
	thisElement.setAttribute("version", synchro_plugin_descriptor.version);

	m_harmonics.saveSettings(doc, thisElement, "harmonics");
	m_modRange.saveSettings(doc, thisElement, "modulation range");
	m_modAmount.saveSettings(doc, thisElement, "modulation amount"); // does this make any sense to save?

	m_carrierOctave.saveSettings(doc, thisElement, "carrier octave");
	m_carrierDrive.saveSettings(doc, thisElement, "carrier drive");
	m_carrierSync.saveSettings(doc, thisElement, "carrier sync");
	m_carrierPulse.saveSettings(doc, thisElement, "carrier pulse");

	m_modulatorOctave.saveSettings(doc, thisElement, "modulator octave");
	m_modulatorDrive.saveSettings(doc, thisElement, "modulator drive");
	m_modulatorSync.saveSettings(doc, thisElement, "modulator sync");
	m_modulatorPulse.saveSettings(doc, thisElement, "modulator pulse");
} //SynchroInstrument::saveSettings()

void SynchroInstrument::loadSettings(const QDomElement& thisElement) {
	m_harmonics.loadSettings(thisElement, "harmonics");
	m_modRange.loadSettings(thisElement, "modulation range");
	m_modAmount.loadSettings(thisElement, "modulation amount");

	m_carrierOctave.loadSettings(thisElement, "carrier octave");
	m_carrierDrive.loadSettings(thisElement, "carrier drive");
	m_carrierSync.loadSettings(thisElement, "carrier sync");
	m_carrierPulse.loadSettings(thisElement, "carrier pulse");

	m_modulatorOctave.loadSettings(thisElement, "modulator octave");
	m_modulatorDrive.loadSettings(thisElement, "modulator drive");
	m_modulatorSync.loadSettings(thisElement, "modulator sync");
	m_modulatorPulse.loadSettings(thisElement, "modulator pulse");
} //SynchroInstrument::loadSettings()

} //namespace lmms