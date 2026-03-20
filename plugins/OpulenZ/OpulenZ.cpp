/*
 * OpulenZ.cpp - AdLib OPL2 FM synth based instrument
 *
 * Copyright (c) 2014 Raine M. Ekman <raine/at/iki/fi>
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

// TODO:
// - Better voice allocation: long releases get cut short :(
// - RT safety = get rid of mutex = make emulator code thread-safe

// - Extras:
//   - double release: first release is in effect until noteoff (heard if percussive sound),
//     second is switched in just before key bit cleared (is this useful???)
//   - Unison: 2,3,4, or 9 voices with configurable spread?
//   - Portamento (needs mono mode?)
//     - Pre-bend/post-bend in poly mode could use portamento speed?

#include "OpulenZ.h"
#include "Instrument.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"

#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QDomElement>
#include <cassert>
#include <cmath>

#include <opl.h>
#include <temuopl.h>
#include <mididata.h>

#include "embed.h"

#include "Knob.h"
#include "PixmapButton.h"

#include "plugin_export.h"

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT opulenz_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"OpulenZ",
	QT_TRANSLATE_NOOP("PluginBrowser", "2-operator FM Synth"),
	"Raine M. Ekman <raine/at/iki/fi>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader("logo"),
	"sbi",
	nullptr,
};

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
	return new OpulenzInstrument(static_cast<InstrumentTrack*>(m));
}

}

QMutex OpulenzInstrument::s_emulatorMutex;

OpulenzOperatorModels::OpulenzOperatorModels(OpulenzInstrument* ins, int num)
	: attack(0.0, 0.0, 15.0, 1.0, ins, QObject::tr("Op %1 attack").arg(num))
	, decay(0.0, 0.0, 15.0, 1.0, ins, QObject::tr("Op %1 decay").arg(num))
	, sustain(0.0, 0.0, 15.0, 1.0, ins, QObject::tr("Op %1 sustain").arg(num))
	, release(0.0, 0.0, 15.0, 1.0, ins, QObject::tr("Op %1 release").arg(num))
	, level(0.0, 0.0, 63.0, 1.0, ins, QObject::tr("Op %1 level").arg(num))
	, scale(0.0, 0.0, 3.0, 1.0, ins, QObject::tr("Op %1 level scaling").arg(num))
	, mul(0.0, 0.0, 15.0, 1.0, ins, QObject::tr("Op %1 frequency multiplier").arg(num))
	, ksr(false, ins, QObject::tr("Op %1 key scaling rate").arg(num))
	, perc(false, ins, QObject::tr("Op %1 percussive envelope").arg(num))
	, trem(false, ins, QObject::tr("Op %1 tremolo").arg(num))
	, vib(false, ins, QObject::tr("Op %1 vibrato").arg(num))
	, waveform(0, 0, 3, ins, QObject::tr("Op %1 waveform").arg(num))
{
}

OpulenzInstrument::OpulenzInstrument(InstrumentTrack* insTrack)
	: Instrument(insTrack, &opulenz_plugin_descriptor, nullptr, Flag::IsSingleStreamed | Flag::IsMidiBased)
	, m_patchModel(0, 0, 127, this, tr("Patch"))
	, m_feedbackModel(0.0, 0.0, 7.0, 1.0, this, tr("Op 1 feedback"))
	, m_fmModel(false, this, tr("FM"))
	, m_vibDepthModel(false, this, tr("Vibrato depth"))
	, m_tremDepthModel(false, this, tr("Tremolo depth"))
	, m_op1(this, 1)
	, m_op2(this, 2)
{
	// Load the default patch. Cannot be done at construction time anymore.
	loadDefaultPatch();

	// Create an emulator - samplerate, 16 bit, mono
	s_emulatorMutex.lock();
	theEmulator = new CTemuopl(Engine::audioEngine()->outputSampleRate(), true, false);
	theEmulator->init();
	// Enable waveform selection
	theEmulator->write(0x01, 0x20);
	s_emulatorMutex.unlock();

	// Initialize voice values
	// voiceNote[0] = 0;
	// voiceLRU[0] = 0;
	for (int i = 0; i < OPL2_VOICES; ++i)
	{
		voiceNote[i] = OPL2_VOICE_FREE;
		voiceLRU[i] = i;
	}

	storedname = displayName();

	updatePatch();

	// Can the buffer size change suddenly? I bet that would break lots of stuff
	frameCount = Engine::audioEngine()->framesPerPeriod();
	renderbuffer = new short[frameCount];

	// Some kind of sane defaults
	pitchbend = 0;
	pitchBendRange = 100; // cents
	RPNcoarse = RPNfine = 255;

	tuneEqual(69, 440);

	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(reloadEmulator()));

	// Connect knobs
	// This one's for testing...
	// FIXME: remove, or use it...
	connect(&m_patchModel, &IntModel::dataChanged, this, &OpulenzInstrument::loadGMPatch);

	const auto modelConn = [this](auto& model)
	{
		connect(&model, SIGNAL(dataChanged()), this, SLOT(updatePatch()));
	};

	const auto opConn = [this,&modelConn](OpulenzOperatorModels& opm)
	{
		modelConn(opm.attack);
		modelConn(opm.decay);
		modelConn(opm.sustain);
		modelConn(opm.release);
		modelConn(opm.level);
		modelConn(opm.scale);
		modelConn(opm.mul);
		modelConn(opm.ksr);
		modelConn(opm.perc);
		modelConn(opm.trem);
		modelConn(opm.vib);
		modelConn(opm.waveform);
	};

	modelConn(m_fmModel);
	modelConn(m_vibDepthModel);
	modelConn(m_tremDepthModel);
	modelConn(m_feedbackModel);
	opConn(m_op1);
	opConn(m_op2);

	// Connect the plugin to the audio engine...
	auto iph = new InstrumentPlayHandle(this, insTrack);
	Engine::audioEngine()->addPlayHandle(iph);
}

OpulenzInstrument::~OpulenzInstrument()
{
	delete theEmulator;
	Engine::audioEngine()->removePlayHandlesOfTypes(instrumentTrack(),
		PlayHandle::Type::NotePlayHandle | PlayHandle::Type::InstrumentPlayHandle);
	delete[] renderbuffer;
}

void OpulenzInstrument::reloadEmulator()
{
	delete theEmulator;
	s_emulatorMutex.lock();
	theEmulator = new CTemuopl(Engine::audioEngine()->outputSampleRate(), true, false);
	theEmulator->init();
	theEmulator->write(0x01, 0x20);
	s_emulatorMutex.unlock();
	for (int i = 0; i < OPL2_VOICES; ++i)
	{
		voiceNote[i] = OPL2_VOICE_FREE;
		voiceLRU[i] = i;
	}
	updatePatch();
}

void OpulenzInstrument::writeVoice(int voice, int reg, int val)
{
	theEmulator->write(OpulenzInstrument::OpAdd[voice] + reg, val);
}

void OpulenzInstrument::setVoiceVelocity(int voice, int vel)
{
	int vel_adjusted = !m_fmModel.value()
		? 63 - (m_op1.level.value() * vel / 127.0)
		: 63 - m_op1.level.value();

	// Velocity calculation, some kind of approximation
	// Only calculate for operator 1 if in adding mode, don't want to change timbre
	writeVoice(voice, 0x40, ((static_cast<int>(m_op1.scale.value()) & 0x03) << 6) + (vel_adjusted & 0x3f));

	vel_adjusted = 63 - (m_op2.level.value() * vel / 127.0);
	// vel_adjusted = 63 - m_op2.level.value();
	writeVoice(voice, 0x43, ((static_cast<int>(m_op2.scale.value()) & 0x03) << 6) + (vel_adjusted & 0x3f));
}

int OpulenzInstrument::popVoice()
{
	int tmp = voiceLRU[0];
	for (int i = 0; i < OPL2_VOICES - 1; ++i)
	{
		voiceLRU[i] = voiceLRU[i + 1];
	}
	voiceLRU[OPL2_VOICES - 1] = OPL2_NO_VOICE;
#ifdef false
	printf("<-- %d %d %d %d %d %d %d %d %d \n", voiceLRU[0], voiceLRU[1], voiceLRU[2],
		voiceLRU[3], voiceLRU[4], voiceLRU[5], voiceLRU[6], voiceLRU[7], voiceLRU[8]);
#endif
	return tmp;
}

int OpulenzInstrument::pushVoice(int v)
{
	int i;
	assert(voiceLRU[OPL2_VOICES-1] == OPL2_NO_VOICE);
	for (i = OPL2_VOICES - 1; i > 0; --i)
	{
		if (voiceLRU[i-1] != OPL2_NO_VOICE) { break; }
	}
	voiceLRU[i] = v;
#ifdef false
	printf("%d %d %d %d %d %d %d %d %d <-- \n", voiceLRU[0], voiceLRU[1], voiceLRU[2],
		voiceLRU[3], voiceLRU[4], voiceLRU[5], voiceLRU[6], voiceLRU[7], voiceLRU[8]);
#endif
	return i;
}

bool OpulenzInstrument::handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	s_emulatorMutex.lock();

	int key = event.key();
	int vel = event.velocity();
	switch (event.type())
	{
	case MidiNoteOn:
		if (int voice = popVoice(); voice != OPL2_NO_VOICE)
		{
			// Turn voice on, NB! the frequencies are straight by voice number,
			// not by the OpulenzInstrument::OpAdd table!
			theEmulator->write(0xA0 + voice, fnums[key] & 0xff);
			theEmulator->write(0xB0 + voice, 32 + ((fnums[key] & 0x1f00) >> 8));
			setVoiceVelocity(voice, vel);
			voiceNote[voice] = key;
			velocities[key] = vel;
		}
		break;
	case MidiNoteOff:
		for (int voice = 0; voice < OPL2_VOICES; ++voice)
		{
			if (voiceNote[voice] == key)
			{
				theEmulator->write(0xA0 + voice, fnums[key] & 0xff);
				theEmulator->write(0xB0 + voice, (fnums[key] & 0x1f00) >> 8);
				voiceNote[voice] |= OPL2_VOICE_FREE;
				pushVoice(voice);
			}
		}
		velocities[key] = 0;
		break;
	case MidiKeyPressure:
		if (velocities[key] != 0) { velocities[key] = vel; }
		for (int voice = 0; voice < OPL2_VOICES; ++voice)
		{
			if (voiceNote[voice] == key) { setVoiceVelocity(voice, vel); }
		}
		break;
	case MidiPitchBend:
		// Update fnumber table
		// Neutral = 8192, full downbend = 0, full upbend = 16383
		if (int tmp_pb = (event.pitchBend() - 8192) * pitchBendRange / 8192; tmp_pb != pitchbend)
		{
			pitchbend = tmp_pb;
			tuneEqual(69, 440.0);
		}
		// Update pitch of all voices (also released ones)
		for (int v = 0; v < OPL2_VOICES; ++v)
		{
			int vn = (voiceNote[v] & ~OPL2_VOICE_FREE);			 // remove the flag bit
			int playing = (voiceNote[v] & OPL2_VOICE_FREE) == 0; // just the flag bit
			theEmulator->write(0xA0 + v, fnums[vn] & 0xff);
			theEmulator->write(0xB0 + v, (playing ? 32 : 0) + ((fnums[vn] & 0x1f00) >> 8));
		}
		break;
	case MidiControlChange:
		switch (event.controllerNumber())
		{
		case MidiControllerRegisteredParameterNumberLSB:
			RPNfine = event.controllerValue();
			break;
		case MidiControllerRegisteredParameterNumberMSB:
			RPNcoarse = event.controllerValue();
			break;
		case MidiControllerDataEntry:
			if ((RPNcoarse << 8) + RPNfine == MidiPitchBendSensitivityRPN)
			{
				pitchBendRange = event.controllerValue() * 100;
			}
			break;
		default:
#ifdef LMMS_DEBUG
			printf("Midi CC %02x %02x\n", event.controllerNumber(), event.controllerValue());
#endif
			break;
		}
		break;
	default:
#ifdef LMMS_DEBUG
		printf("Midi event type %d\n",event.type());
#endif
		break;
	}
	s_emulatorMutex.unlock();
	return true;
}

QString OpulenzInstrument::nodeName() const
{
	return opulenz_plugin_descriptor.name;
}

gui::PluginView* OpulenzInstrument::instantiateView(QWidget* parent)
{
	return new gui::OpulenzInstrumentView(this, parent);
}

void OpulenzInstrument::play(SampleFrame* workingBuffer)
{
	s_emulatorMutex.lock();
	theEmulator->update(renderbuffer, frameCount);

	for (f_cnt_t frame = 0; frame < frameCount; ++frame)
	{
		sample_t s = float(renderbuffer[frame]) / 8192.0;
		for (ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch)
		{
			workingBuffer[frame][ch] = s;
		}
	}
	s_emulatorMutex.unlock();
}

void OpulenzInstrument::saveSettings(QDomDocument& doc, QDomElement& el)
{
	m_op1.attack.saveSettings(doc, el, "op1_a");
	m_op1.decay.saveSettings(doc, el, "op1_d");
	m_op1.sustain.saveSettings(doc, el, "op1_s");
	m_op1.release.saveSettings(doc, el, "op1_r");
	m_op1.level.saveSettings(doc, el, "op1_lvl");
	m_op1.scale.saveSettings(doc, el, "op1_scale");
	m_op1.mul.saveSettings(doc, el, "op1_mul");
	m_feedbackModel.saveSettings(doc, el, "feedback");
	m_op1.ksr.saveSettings(doc, el, "op1_ksr");
	m_op1.perc.saveSettings(doc, el, "op1_perc");
	m_op1.trem.saveSettings(doc, el, "op1_trem");
	m_op1.vib.saveSettings(doc, el, "op1_vib");
	m_op1.waveform.saveSettings(doc, el, "op1_waveform");

	m_op2.attack.saveSettings(doc, el, "op2_a");
	m_op2.decay.saveSettings(doc, el, "op2_d");
	m_op2.sustain.saveSettings(doc, el, "op2_s");
	m_op2.release.saveSettings(doc, el, "op2_r");
	m_op2.level.saveSettings(doc, el, "op2_lvl");
	m_op2.scale.saveSettings(doc, el, "op2_scale");
	m_op2.mul.saveSettings(doc, el, "op2_mul");
	m_op2.ksr.saveSettings(doc, el, "op2_ksr");
	m_op2.perc.saveSettings(doc, el, "op2_perc");
	m_op2.trem.saveSettings(doc, el, "op2_trem");
	m_op2.vib.saveSettings(doc, el, "op2_vib");
	m_op2.waveform.saveSettings(doc, el, "op2_waveform");

	m_fmModel.saveSettings(doc, el, "fm");
	m_vibDepthModel.saveSettings(doc, el, "vib_depth");
	m_tremDepthModel.saveSettings(doc, el, "trem_depth");

	el.setAttribute("version", 1);
}

void OpulenzInstrument::loadSettings(const QDomElement& el)
{
	if (el.attribute("version", "0").toInt() < 1)
	{
		m_op1.scale.setValue(0);
		m_op2.scale.setValue(0);
	}
	else
	{
		m_op1.scale.loadSettings(el, "op1_scale");
		m_op2.scale.loadSettings(el, "op2_scale");
	}

	m_feedbackModel.loadSettings(el, "feedback");
	m_fmModel.loadSettings(el, "fm");
	m_vibDepthModel.loadSettings(el, "vib_depth");
	m_tremDepthModel.loadSettings(el, "trem_depth");

	m_op1.attack.loadSettings(el, "op1_a");
	m_op1.decay.loadSettings(el, "op1_d");
	m_op1.sustain.loadSettings(el, "op1_s");
	m_op1.release.loadSettings(el, "op1_r");
	m_op1.level.loadSettings(el, "op1_lvl");
	m_op1.mul.loadSettings(el, "op1_mul");
	m_op1.ksr.loadSettings(el, "op1_ksr");
	m_op1.perc.loadSettings(el, "op1_perc");
	m_op1.trem.loadSettings(el, "op1_trem");
	m_op1.vib.loadSettings(el, "op1_vib");
	m_op1.waveform.loadSettings(el, "op1_waveform");

	m_op2.attack.loadSettings(el, "op2_a");
	m_op2.decay.loadSettings(el, "op2_d");
	m_op2.sustain.loadSettings(el, "op2_s");
	m_op2.release.loadSettings(el, "op2_r");
	m_op2.level.loadSettings(el, "op2_lvl");
	m_op2.mul.loadSettings(el, "op2_mul");
	m_op2.ksr.loadSettings(el, "op2_ksr");
	m_op2.perc.loadSettings(el, "op2_perc");
	m_op2.trem.loadSettings(el, "op2_trem");
	m_op2.vib.loadSettings(el, "op2_vib");
	m_op2.waveform.loadSettings(el, "op2_waveform");
}

void OpulenzInstrument::loadPatch(const unsigned char inst[14])
{
	s_emulatorMutex.lock();
	for (int v = 0; v < OPL2_VOICES; ++v)
	{
		writeVoice(v, 0x20, inst[0]); // op1 AM/VIB/EG/KSR/Multiplier
		writeVoice(v, 0x23, inst[1]); // op2
#if 0
		// The handling of these registers is currently done by setVoiceVelocity().
		writeVoice(v, 0x40, inst[2]); // op1 KSL/Output Level
		writeVoice(v, 0x43, inst[3]); // op2
#endif
		writeVoice(v, 0x60, inst[4]); // op1 A/D
		writeVoice(v, 0x63, inst[5]); // op2
		writeVoice(v, 0x80, inst[6]); // op1 S/R
		writeVoice(v, 0x83, inst[7]); // op2
		writeVoice(v, 0xe0, inst[8]); // op1 waveform
		writeVoice(v, 0xe3, inst[9]); // op2
		theEmulator->write(0xc0 + v, inst[10]);	// feedback/algorithm
	}
	s_emulatorMutex.unlock();
}

void OpulenzInstrument::tuneEqual(int center, float Hz)
{
	for (int n = 0; n < 128; ++n)
	{
		float tmp = Hz * std::exp2((n - center) / 12.0f + pitchbend / 1200.0f);
		fnums[n] = Hz2fnum(tmp);
	}
}

int OpulenzInstrument::Hz2fnum(float Hz)
{
	for (int block = 0; block < 8; ++block)
	{
		auto fnum = static_cast<unsigned>(Hz * std::exp2(20.0f - static_cast<float>(block)) / 49716.0f);
		if (fnum < 1023) { return fnum + (block << 10); }
	}
	return 0;
}

void OpulenzInstrument::loadGMPatch()
{
	const unsigned char* inst = midi_fm_instruments[m_patchModel.value()];
	loadPatch(inst);
}

void OpulenzInstrument::updatePatch()
{
	const auto encodeFlags = [](const OpulenzOperatorModels& op) -> unsigned char
	{
		return (op.trem.value() ? 128 : 0)
			+ (op.vib.value() ? 64 : 0)
			+ (op.perc.value() ? 0 : 32) // NB. This envelope mode is "perc", not "sus"
			+ (op.ksr.value() ? 16 : 0)
			+ (static_cast<int>(op.mul.value()) & 0x0f);
	};

	const auto encodeScaleLevel = [](const OpulenzOperatorModels& op) -> unsigned char
	{
		return ((static_cast<int>(op.scale.value()) & 0x03) << 6)
			+ (63 - (static_cast<int>(op.level.value()) & 0x3f));
	};

	const auto encodeEnvPart = [](const auto& m1, const auto& m2) -> unsigned char
	{
		return ((15 - (static_cast<int>(m1.value()) & 0x0f)) << 4)
			+ (15 - (static_cast<int>(m2.value()) & 0x0f));
	};

	auto inst = std::array<unsigned char, 14>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	inst[0] = encodeFlags(m_op1);
	inst[1] = encodeFlags(m_op2);
	inst[2] = encodeScaleLevel(m_op1);
	inst[3] = encodeScaleLevel(m_op2);
	inst[4] = encodeEnvPart(m_op1.attack, m_op1.decay);
	inst[5] = encodeEnvPart(m_op2.attack, m_op2.decay);
	inst[6] = encodeEnvPart(m_op1.sustain, m_op1.release);
	inst[7] = encodeEnvPart(m_op2.sustain, m_op2.release);
	inst[8] = static_cast<int>(m_op1.waveform.value()) & 0x03;
	inst[9] = static_cast<int>(m_op2.waveform.value()) & 0x03;
	inst[10] = (m_fmModel.value() ? 0 : 1) + ((static_cast<int>(m_feedbackModel.value()) & 0x07) << 1);

	// These are always 0 in the list I had?
	inst[11] = 0;
	inst[12] = 0;
	inst[13] = 0;

	// Not part of the per-voice patch info
	theEmulator->write(0xBD, (m_tremDepthModel.value() ? 128 : 0) + (m_vibDepthModel.value() ? 64 : 0));

	// have to do this, as the level knobs might've changed
	for (int voice = 0; voice < OPL2_VOICES; ++voice)
	{
		if (voiceNote[voice] && OPL2_VOICE_FREE == 0)
		{
			setVoiceVelocity(voice, velocities[voiceNote[voice]]);
		}
	}

#ifdef false
	printf("UPD: %02x %02x %02x %02x %02x -- %02x %02x %02x %02x %02x %02x\n",
		inst[0], inst[1], inst[2], inst[3], inst[4],
		inst[5], inst[6], inst[7], inst[8], inst[9], inst[10]);
#endif

	loadPatch(inst.data());
}

void OpulenzInstrument::loadDefaultPatch()
{
	m_feedbackModel.setValue(0.0);
	m_fmModel.setValue(true);
	m_vibDepthModel.setValue(false);
	m_tremDepthModel.setValue(false);

	m_op1.attack.setValue(14.0);
	m_op1.decay.setValue(14.0);
	m_op1.sustain.setValue(13.0);
	m_op1.release.setValue(10.0);
	m_op1.level.setValue(62.0);
	m_op1.scale.setValue(0.0);
	m_op1.mul.setValue(0.0);
	m_op1.ksr.setValue(false);
	m_op1.perc.setValue(false);
	m_op1.trem.setValue(true);
	m_op1.vib.setValue(false);
	m_op1.waveform.setValue(0);

	m_op2.attack.setValue(1.0);
	m_op2.decay.setValue(3.0);
	m_op2.sustain.setValue(14.0);
	m_op2.release.setValue(12.0);
	m_op2.level.setValue(63.0);
	m_op2.scale.setValue(0.0);
	m_op2.mul.setValue(1.0);
	m_op2.ksr.setValue(false);
	m_op2.perc.setValue(false);
	m_op2.trem.setValue(false);
	m_op2.vib.setValue(true);
	m_op2.waveform.setValue(0);
}

void OpulenzInstrument::loadFile(const QString& file)
{
	// http://cd.textfiles.com/soundsensations/SYNTH/SBINS/
	// http://cd.textfiles.com/soundsensations/SYNTH/SBI1198/1198SBI.ZIP
	if (!file.isEmpty() && QFileInfo(file).exists())
	{
		QFile sbiFile(file);
		if (!sbiFile.open(QIODevice::ReadOnly))
		{
			printf("Can't open file\n");
			return;
		}

		QByteArray sbiData = sbiFile.read(52);
		if (!sbiData.startsWith("SBI\0x1a"))
		{
			printf("No SBI signature\n");
			return;
		}
		if (sbiData.size() != 52)
		{
			printf("SBI size error: expected 52, got %d\n", static_cast<int>(sbiData.size()));
		}

		// Minimum size of SBI if we ignore "reserved" bytes at end
		// https://courses.engr.illinois.edu/ece390/resources/sound/cmf.txt.html
		if (sbiData.size() < 47) { return; }

		QString sbiName = sbiData.mid(4, 32);
		// If user has changed track name... let's hope my logic is valid.
		if (sbiName.size() > 0 && instrumentTrack()->displayName() == storedname)
		{
			instrumentTrack()->setName(sbiName);
			storedname = sbiName;
		}

#ifdef false
		printf("SBI: %02x %02x %02x %02x %02x -- %02x %02x %02x %02x %02x %02x\n",
			(unsigned char)sbiData[36], (unsigned char)sbiData[37], (unsigned char)sbiData[38],
			(unsigned char)sbiData[39], (unsigned char)sbiData[40], (unsigned char)sbiData[41],
			(unsigned char)sbiData[42], (unsigned char)sbiData[43], (unsigned char)sbiData[44],
			(unsigned char)sbiData[45], (unsigned char)sbiData[46]);
#endif
		// Modulator Sound Characteristic (Mult, KSR, EG, VIB, AM)
		m_op1.trem.setValue((sbiData[36] & 0x80) == 0x80 ? true : false);
		m_op1.vib.setValue((sbiData[36] & 0x40) == 0x40 ? true : false);
		m_op1.perc.setValue((sbiData[36] & 0x20) == 0x20 ? false : true);
		m_op1.ksr.setValue((sbiData[36] & 0x10) == 0x10 ? true : false);
		m_op1.mul.setValue(sbiData[36] & 0x0f);

		// Carrier Sound Characteristic
		m_op2.trem.setValue((sbiData[37] & 0x80) == 0x80 ? true : false);
		m_op2.vib.setValue((sbiData[37] & 0x40) == 0x40 ? true : false);
		m_op2.perc.setValue((sbiData[37] & 0x20) == 0x20 ? false : true);
		m_op2.ksr.setValue((sbiData[37] & 0x10) == 0x10 ? true : false);
		m_op2.mul.setValue(sbiData[37] & 0x0f);

		// Modulator Scaling/Output Level
		m_op1.scale.setValue((sbiData[38] & 0xc0) >> 6);
		m_op1.level.setValue(63 - (sbiData[38] & 0x3f));

		// Carrier Scaling/Output Level
		m_op2.scale.setValue((sbiData[39] & 0xc0) >> 6);
		m_op2.level.setValue(63 - (sbiData[39] & 0x3f));

		// Modulator Attack/Decay
		m_op1.attack.setValue(15 - ((sbiData[40] & 0xf0) >> 4));
		m_op1.decay.setValue(15 - (sbiData[40] & 0x0f));

		// Carrier Attack/Decay
		m_op2.attack.setValue(15 - ((sbiData[41] & 0xf0) >> 4));
		m_op2.decay.setValue(15 - (sbiData[41] & 0x0f));

		// Modulator Sustain/Release
		m_op1.sustain.setValue(15 - ((sbiData[42] & 0xf0) >> 4));
		m_op1.release.setValue(15 - (sbiData[42] & 0x0f));

		// Carrier Sustain/Release
		m_op2.sustain.setValue(15 - ((sbiData[43] & 0xf0) >> 4));
		m_op2.release.setValue(15 - (sbiData[43] & 0x0f));

		// Modulator Wave Select
		m_op1.waveform.setValue(sbiData[44] & 0x03);

		// Carrier Wave Select
		m_op2.waveform.setValue(sbiData[45] & 0x03);

		// Feedback/Connection
		m_fmModel.setValue((sbiData[46] & 0x01) == 0x01 ? false : true);
		m_feedbackModel.setValue(((sbiData[46] & 0x0e) >> 1));
	}
}

namespace gui
{

OpulenzInstrumentView::OpulenzInstrumentView(Instrument* instrument, QWidget* parent)
	: InstrumentViewFixedSize(instrument, parent)
{
	const auto makeKnob = [this](QString hintText, QString hintUnit, int xpos, int ypos)
	{
		auto* k = new Knob(KnobType::Styled, this);
		k->setHintText(hintText, hintUnit);
		k->setFixedSize(22, 22);
		k->setCenterPointX(11.0);
		k->setCenterPointY(11.0);
		k->setTotalAngle(270.0);
		k->move(xpos, ypos);
		return k;
	};

	const auto makeButton = [this](QString tooltip, int xpos, int ypos)
	{
		auto* b = new PixmapButton(this, nullptr);
		b->setActiveGraphic(PLUGIN_NAME::getIconPixmap("led_on"));
		b->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("led_off"));
		b->setCheckable(true);
		b->setToolTip(tooltip);
		b->move(xpos, ypos);
		return b;
	};

	const auto makeWaveButton = [this](QString tooltip, int xpos, int ypos, const char* iconOn, const char* iconOff,
		AutomatableButtonGroup* group)
	{
		auto* b = new PixmapButton(this, nullptr);
		b->setActiveGraphic(PLUGIN_NAME::getIconPixmap(iconOn));
		b->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(iconOff));
		b->setToolTip(tooltip);
		b->move(xpos, ypos);
		group->addButton(b);
		return b;
	};

	const auto makeOpControls = [&](int offX, int offY) -> OpulenzOperatorControls
	{
		OpulenzOperatorControls ret;

		ret.attack = makeKnob(tr("Attack"), "", 6 + offX, 48 + offY);
		ret.decay = makeKnob(tr("Decay"), "", 34 + offX, 48 + offY);
		ret.sustain = makeKnob(tr("Sustain"), "", 62 + offX, 48 + offY);
		ret.release = makeKnob(tr("Release"), "", 90 + offX, 48 + offY);
		ret.level = makeKnob(tr("Level"), "", 166 + offX, 48 + offY);
		ret.scale = makeKnob(tr("Scale"), "", 194 + offX, 48 + offY);
		ret.mul = makeKnob(tr("Frequency multiplier"), "", 222 + offX, 48 + offY);
		ret.ksr = makeButton(tr("Keyboard scaling rate"), 9 + offX, 87 + offY);
		ret.perc = makeButton(tr("Percussive envelope"), 36 + offX, 87 + offY);
		ret.trem = makeButton(tr("Tremolo"), 65 + offX, 87 + offY);
		ret.vib = makeButton(tr("Vibrato"), 93 + offX, 87 + offY);

		ret.waveform = new AutomatableButtonGroup(this);
		ret.w0 = makeWaveButton(tr("Sine"), 154 + offX, 86 + offY, "wave1_on", "wave1_off", ret.waveform);
		ret.w1 = makeWaveButton(tr("Half sine"), 178 + offX, 86 + offY, "wave2_on", "wave2_off", ret.waveform);
		ret.w2 = makeWaveButton(tr("Absolute sine"), 199 + offX, 86 + offY, "wave3_on", "wave3_off", ret.waveform);
		ret.w3 = makeWaveButton(tr("Quarter sine"), 220 + offX, 86 + offY, "wave4_on", "wave4_off", ret.waveform);

		return ret;
	};

	feedbackKnob = makeKnob(tr("Feedback"), "", 128, 48);
	fmButton = makeButton(tr("FM"), 9, 220);
	vibDepthButton = makeButton(tr("Vibrato depth"), 65, 220);
	tremDepthButton = makeButton(tr("Tremolo depth"), 93, 220);

	op1View = makeOpControls(0, 0);
	op2View = makeOpControls(0, 90);

	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
}

OpulenzInstrumentView::~OpulenzInstrumentView()
{
	// Knobs are QWidgets and our children, so they're destroyed automatically
}

inline QString OpulenzInstrumentView::timeKnobHint(float n)
{
	if (n > 1000) { return QString::number(n/1000, 'f', 0) + " s"; }
	else if (n > 10) { return QString::number(n, 'f', 0) + " ms"; }
	else { return QString::number(n, 'f', 1) + " ms"; }
}

void OpulenzInstrumentView::updateKnobHints()
{
	// Envelope times in ms: t[0] = 0, t[n] = (1<<n) * X, X = 0.11597 for A, 0.6311 for D/R
	// Here some rounding has been applied.
	constexpr auto AttackTimes = std::array<float, 16>{
		0.f, 0.2f, 0.4f, 0.9f, 1.8f, 3.7f, 7.4f,
		15.f, 30.f, 60.f, 120.f, 240.f, 480.f,
		950.f, 1900.f, 3800.f
	};

	constexpr auto DrTimes = std::array<float, 16>{
		0.f, 1.2f, 2.5f, 5.f, 10.f, 20.f, 40.f,
		80.f, 160.f, 320.f, 640.f, 1300.f, 2600.f,
		5200.f, 10000.f, 20000.f
	};

	constexpr auto FreqMults = std::array<int, 16>{
		-12, 0, 12, 19, 24, 28, 31, 34, 36, 38, 40, 40, 43, 43, 47, 47
	};

	auto m = castModel<OpulenzInstrument>();

	const auto setTimeHint = [this](Knob* knob, const QString& name, const auto& arr, const FloatModel& model)
	{
		const auto val = arr[static_cast<int>(model.value())];
		knob->setHintText(name, QString{" (%1)"}.arg(timeKnobHint(val)));
	};

	const auto setHintSemitone = [this,&FreqMults](Knob* knob, const QString& name, const FloatModel& model)
	{
		const auto mul = FreqMults[static_cast<int>(model.value())];
		knob->setHintText(name, QString{" (%1 semitones)"}.arg(mul));
	};

	setTimeHint(op1View.attack, tr("Attack"), AttackTimes, m->m_op1.attack);
	setTimeHint(op2View.attack, tr("Attack"), AttackTimes, m->m_op2.attack);
	setTimeHint(op1View.decay, tr("Decay"), DrTimes, m->m_op1.decay);
	setTimeHint(op2View.decay, tr("Decay"), DrTimes, m->m_op2.decay);
	setTimeHint(op1View.release, tr("Release"), DrTimes, m->m_op1.release);
	setTimeHint(op2View.release, tr("Release"), DrTimes, m->m_op2.release);
	setHintSemitone(op1View.mul, tr("Frequency multiplier"), m->m_op1.mul);
	setHintSemitone(op2View.mul, tr("Frequency multiplier"), m->m_op2.mul);
}

void OpulenzInstrumentView::modelChanged()
{
	auto m = castModel<OpulenzInstrument>();
	// m_patch->setModel(&m->m_patchModel);

	feedbackKnob->setModel(&m->m_feedbackModel);
	fmButton->setModel(&m->m_fmModel);
	vibDepthButton->setModel(&m->m_vibDepthModel);
	tremDepthButton->setModel(&m->m_tremDepthModel);

	op1View.attack->setModel(&m->m_op1.attack);
	op1View.decay->setModel(&m->m_op1.decay);
	op1View.sustain->setModel(&m->m_op1.sustain);
	op1View.release->setModel(&m->m_op1.release);
	op1View.level->setModel(&m->m_op1.level);
	op1View.scale->setModel(&m->m_op1.scale);
	op1View.mul->setModel(&m->m_op1.mul);
	op1View.ksr->setModel(&m->m_op1.ksr);
	op1View.perc->setModel(&m->m_op1.perc);
	op1View.trem->setModel(&m->m_op1.trem);
	op1View.vib->setModel(&m->m_op1.vib);
	op1View.waveform->setModel(&m->m_op1.waveform);

	op2View.attack->setModel(&m->m_op2.attack);
	op2View.decay->setModel(&m->m_op2.decay);
	op2View.sustain->setModel(&m->m_op2.sustain);
	op2View.release->setModel(&m->m_op2.release);
	op2View.level->setModel(&m->m_op2.level);
	op2View.scale->setModel(&m->m_op2.scale);
	op2View.mul->setModel(&m->m_op2.mul);
	op2View.ksr->setModel(&m->m_op2.ksr);
	op2View.perc->setModel(&m->m_op2.perc);
	op2View.trem->setModel(&m->m_op2.trem);
	op2View.vib->setModel(&m->m_op2.vib);
	op2View.waveform->setModel(&m->m_op2.waveform);

	const auto connHint = [this](FloatModel* model)
	{
		connect(model, &FloatModel::dataChanged, this, &OpulenzInstrumentView::updateKnobHints);
	};

	connHint(&m->m_op1.attack);
	connHint(&m->m_op1.decay);
	connHint(&m->m_op1.release);
	connHint(&m->m_op1.mul);

	connHint(&m->m_op2.attack);
	connHint(&m->m_op2.decay);
	connHint(&m->m_op2.release);
	connHint(&m->m_op2.mul);

	updateKnobHints();
}

} // namespace gui

} // namespace lmms
