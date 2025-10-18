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
        LMMS_STRINGIFY( PLUGIN_NAME ),
        "OpulenZ",
        QT_TRANSLATE_NOOP( "PluginBrowser",
			   "2-operator FM Synth" ),
        "Raine M. Ekman <raine/at/iki/fi>",
        0x0100,
        Plugin::Type::Instrument,
        new PluginPixmapLoader( "logo" ),
        "sbi",
        nullptr,
		nullptr,
};

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new OpulenzInstrument( static_cast<InstrumentTrack *>( m ) ) );
}

}

// I'd much rather do without a mutex, but it looks like
// the emulator code isn't really ready for threads
QMutex OpulenzInstrument::emulatorMutex;

// Weird ordering of voice parameters
const auto adlib_opadd = std::array<unsigned int, OPL2_VOICES>{0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12};

OpulenzInstrument::OpulenzInstrument( InstrumentTrack * _instrument_track ) :
	Instrument(_instrument_track, &opulenz_plugin_descriptor, nullptr, Flag::IsSingleStreamed | Flag::IsMidiBased),
	m_patchModel( 0, 0, 127, this, tr( "Patch" ) ),
	op1_a_mdl(14.0, 0.0, 15.0, 1.0, this, tr( "Op 1 attack" )  ),
	op1_d_mdl(14.0, 0.0, 15.0, 1.0, this, tr( "Op 1 decay" )   ),
	op1_s_mdl(3.0, 0.0, 15.0, 1.0, this, tr( "Op 1 sustain" )   ),
	op1_r_mdl(10.0, 0.0, 15.0, 1.0, this, tr( "Op 1 release" )   ),
	op1_lvl_mdl(62.0, 0.0, 63.0, 1.0, this, tr( "Op 1 level" )   ),
	op1_scale_mdl(0.0, 0.0, 3.0, 1.0, this, tr( "Op 1 level scaling" ) ),
	op1_mul_mdl(0.0, 0.0, 15.0, 1.0, this, tr( "Op 1 frequency multiplier" ) ),
	feedback_mdl(0.0, 0.0, 7.0, 1.0, this, tr( "Op 1 feedback" )    ),
	op1_ksr_mdl(false, this, tr( "Op 1 key scaling rate" ) ),
	op1_perc_mdl(false, this, tr( "Op 1 percussive envelope" )   ),
	op1_trem_mdl(true, this, tr( "Op 1 tremolo" )   ),
	op1_vib_mdl(false, this, tr( "Op 1 vibrato" )   ),
	op1_w0_mdl(  ),
	op1_w1_mdl(  ),
	op1_w2_mdl(  ),
	op1_w3_mdl(  ),
	op1_waveform_mdl(0,0,3,this, tr( "Op 1 waveform" ) ),


	op2_a_mdl(1.0, 0.0, 15.0, 1.0, this, tr( "Op 2 attack" )   ),
	op2_d_mdl(3.0, 0.0, 15.0, 1.0, this, tr( "Op 2 decay" )   ),
	op2_s_mdl(14.0, 0.0, 15.0, 1.0, this, tr( "Op 2 sustain" ) ),
	op2_r_mdl(12.0, 0.0, 15.0, 1.0, this, tr( "Op 2 release" )   ),
	op2_lvl_mdl(63.0, 0.0, 63.0, 1.0, this, tr( "Op 2 level" )   ),
	op2_scale_mdl(0.0, 0.0, 3.0, 1.0, this, tr( "Op 2 level scaling" ) ),
	op2_mul_mdl(1.0, 0.0, 15.0, 1.0, this, tr( "Op 2 frequency multiplier" ) ),
	op2_ksr_mdl(false, this, tr( "Op 2 key scaling rate" ) ),
	op2_perc_mdl(false, this, tr( "Op 2 percussive envelope" )   ),
	op2_trem_mdl(false, this, tr( "Op 2 tremolo" )   ),
	op2_vib_mdl(true, this, tr( "Op 2 vibrato" )   ),
	op2_w0_mdl(  ),
	op2_w1_mdl(  ),
	op2_w2_mdl(  ),
	op2_w3_mdl(  ),
	op2_waveform_mdl(0,0,3,this, tr( "Op 2 waveform" ) ),

	fm_mdl(true, this, tr( "FM" )   ),
	vib_depth_mdl(false, this, tr( "Vibrato depth" )   ),
	trem_depth_mdl(false, this, tr( "Tremolo depth" )   )
{

	// Create an emulator - samplerate, 16 bit, mono
	emulatorMutex.lock();
	theEmulator = new CTemuopl(Engine::audioEngine()->outputSampleRate(), true, false);
	theEmulator->init();
	// Enable waveform selection
	theEmulator->write(0x01,0x20);
	emulatorMutex.unlock();

	//Initialize voice values
	// voiceNote[0] = 0;
	// voiceLRU[0] = 0;
	for(int i=0; i<OPL2_VOICES; ++i) {
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

	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ),
		 this, SLOT( reloadEmulator() ) );
	// Connect knobs
	// This one's for testing...
	connect( &m_patchModel, SIGNAL( dataChanged() ), this, SLOT( loadGMPatch() ) );
#define MOD_CON( model ) connect( &model, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	MOD_CON( op1_a_mdl );
	MOD_CON( op1_d_mdl );
	MOD_CON( op1_s_mdl );
	MOD_CON( op1_r_mdl );
	MOD_CON( op1_lvl_mdl );
	MOD_CON( op1_scale_mdl );
	MOD_CON( op1_mul_mdl );
	MOD_CON( feedback_mdl );
	MOD_CON( op1_ksr_mdl );
	MOD_CON( op1_perc_mdl );
	MOD_CON( op1_trem_mdl );
	MOD_CON( op1_vib_mdl );
	MOD_CON( op1_w0_mdl );
	MOD_CON( op1_w1_mdl );
	MOD_CON( op1_w2_mdl );
	MOD_CON( op1_w3_mdl );
	MOD_CON( op1_waveform_mdl );

	MOD_CON( op2_a_mdl );
	MOD_CON( op2_d_mdl );
	MOD_CON( op2_s_mdl );
	MOD_CON( op2_r_mdl );
	MOD_CON( op2_lvl_mdl );
	MOD_CON( op2_scale_mdl );
	MOD_CON( op2_mul_mdl );
	MOD_CON( op2_ksr_mdl );
	MOD_CON( op2_perc_mdl );
	MOD_CON( op2_trem_mdl );
	MOD_CON( op2_vib_mdl );
	MOD_CON( op2_w0_mdl );
	MOD_CON( op2_w1_mdl );
	MOD_CON( op2_w2_mdl );
	MOD_CON( op2_w3_mdl );
	MOD_CON( op2_waveform_mdl );

	MOD_CON( fm_mdl );
	MOD_CON( vib_depth_mdl );
	MOD_CON( trem_depth_mdl );

	// Connect the plugin to the audio engine...
	auto iph = new InstrumentPlayHandle(this, _instrument_track);
	Engine::audioEngine()->addPlayHandle( iph );
}

OpulenzInstrument::~OpulenzInstrument() {
	delete theEmulator;
	Engine::audioEngine()->removePlayHandlesOfTypes( instrumentTrack(),
				PlayHandle::Type::NotePlayHandle
				| PlayHandle::Type::InstrumentPlayHandle );
	delete [] renderbuffer;
}

// Samplerate changes when choosing oversampling, so this is more or less mandatory
void OpulenzInstrument::reloadEmulator() {
	delete theEmulator;
	emulatorMutex.lock();
	theEmulator = new CTemuopl(Engine::audioEngine()->outputSampleRate(), true, false);
	theEmulator->init();
	theEmulator->write(0x01,0x20);
	emulatorMutex.unlock();
	for(int i=0; i<OPL2_VOICES; ++i) {
		voiceNote[i] = OPL2_VOICE_FREE;
		voiceLRU[i] = i;
	}
	updatePatch();
}

// This shall only be called from code protected by the holy Mutex!
void OpulenzInstrument::setVoiceVelocity(int voice, int vel) {
	int vel_adjusted = !fm_mdl.value()
		? 63 - (op1_lvl_mdl.value() * vel / 127.0)
		: 63 - op1_lvl_mdl.value();

	// Velocity calculation, some kind of approximation
	// Only calculate for operator 1 if in adding mode, don't want to change timbre
	theEmulator->write(0x40+adlib_opadd[voice],
			   ( (int)op1_scale_mdl.value() & 0x03 << 6) +
			   ( vel_adjusted & 0x3f ) );


	vel_adjusted = 63 - ( op2_lvl_mdl.value() * vel/127.0 );
	// vel_adjusted = 63 - op2_lvl_mdl.value();
	theEmulator->write(0x43+adlib_opadd[voice],
			   ( (int)op2_scale_mdl.value() & 0x03 << 6) +
			   ( vel_adjusted & 0x3f ) );
}

// Pop least recently used voice
int OpulenzInstrument::popVoice() {
	int tmp = voiceLRU[0];
	for( int i=0; i<OPL2_VOICES-1; ++i) {
		voiceLRU[i] = voiceLRU[i+1];
	}
	voiceLRU[OPL2_VOICES-1] = OPL2_NO_VOICE;
#ifdef false
	printf("<-- %d %d %d %d %d %d %d %d %d \n", voiceLRU[0],voiceLRU[1],voiceLRU[2],
	       voiceLRU[3],voiceLRU[4],voiceLRU[5],voiceLRU[6],voiceLRU[7],voiceLRU[8]);
#endif
	return tmp;
}
// Push voice into first free slot
int OpulenzInstrument::pushVoice(int v) {
	int i;
	assert(voiceLRU[OPL2_VOICES-1]==OPL2_NO_VOICE);
	for(i=OPL2_VOICES-1; i>0; --i) {
		if( voiceLRU[i-1] != OPL2_NO_VOICE ) {
			break;
		}
	}
	voiceLRU[i] = v;
#ifdef false
	printf("%d %d %d %d %d %d %d %d %d <-- \n", voiceLRU[0],voiceLRU[1],voiceLRU[2],
	       voiceLRU[3],voiceLRU[4],voiceLRU[5],voiceLRU[6],voiceLRU[7],voiceLRU[8]);
#endif
	return i;
}

bool OpulenzInstrument::handleMidiEvent( const MidiEvent& event, const TimePos& time, f_cnt_t offset )
{
	emulatorMutex.lock();

	int key = event.key();
	int vel = event.velocity();
	switch (event.type())
	{
	case MidiNoteOn:
		if (int voice = popVoice(); voice != OPL2_NO_VOICE)
		{
			// Turn voice on, NB! the frequencies are straight by voice number,
			// not by the adlib_opadd table!
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
		switch (event.controllerNumber()) {
		case MidiControllerRegisteredParameterNumberLSB:
			RPNfine = event.controllerValue();
			break;
		case MidiControllerRegisteredParameterNumberMSB:
			RPNcoarse = event.controllerValue();
			break;
		case MidiControllerDataEntry:
			if( (RPNcoarse << 8) + RPNfine == MidiPitchBendSensitivityRPN) {
				pitchBendRange = event.controllerValue() * 100;
			}
			break;
		default:
#ifdef LMMS_DEBUG
			printf("Midi CC %02x %02x\n", event.controllerNumber(), event.controllerValue() );
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
	emulatorMutex.unlock();
	return true;
}

QString OpulenzInstrument::nodeName() const
{
        return( opulenz_plugin_descriptor.name );
}

gui::PluginView* OpulenzInstrument::instantiateView( QWidget * _parent )
{
        return( new gui::OpulenzInstrumentView( this, _parent ) );
}


void OpulenzInstrument::play( SampleFrame* _working_buffer )
{
	emulatorMutex.lock();
	theEmulator->update(renderbuffer, frameCount);

	for( fpp_t frame = 0; frame < frameCount; ++frame )
        {
                sample_t s = float(renderbuffer[frame]) / 8192.0;
                for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
                {
                        _working_buffer[frame][ch] = s;
                }
	}
	emulatorMutex.unlock();
}


void OpulenzInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	op1_a_mdl.saveSettings( _doc, _this, "op1_a" );
	op1_d_mdl.saveSettings( _doc, _this, "op1_d" );
	op1_s_mdl.saveSettings( _doc, _this, "op1_s" );
	op1_r_mdl.saveSettings( _doc, _this, "op1_r" );
	op1_lvl_mdl.saveSettings( _doc, _this, "op1_lvl" );
	op1_scale_mdl.saveSettings( _doc, _this, "op1_scale" );
	op1_mul_mdl.saveSettings( _doc, _this, "op1_mul" );
	feedback_mdl.saveSettings( _doc, _this, "feedback" );
	op1_ksr_mdl.saveSettings( _doc, _this, "op1_ksr" );
	op1_perc_mdl.saveSettings( _doc, _this, "op1_perc" );
	op1_trem_mdl.saveSettings( _doc, _this, "op1_trem" );
	op1_vib_mdl.saveSettings( _doc, _this, "op1_vib" );
	op1_waveform_mdl.saveSettings( _doc, _this, "op1_waveform" );

	op2_a_mdl.saveSettings( _doc, _this, "op2_a" );
	op2_d_mdl.saveSettings( _doc, _this, "op2_d" );
	op2_s_mdl.saveSettings( _doc, _this, "op2_s" );
	op2_r_mdl.saveSettings( _doc, _this, "op2_r" );
	op2_lvl_mdl.saveSettings( _doc, _this, "op2_lvl" );
	op2_scale_mdl.saveSettings( _doc, _this, "op2_scale" );
	op2_mul_mdl.saveSettings( _doc, _this, "op2_mul" );
	op2_ksr_mdl.saveSettings( _doc, _this, "op2_ksr" );
	op2_perc_mdl.saveSettings( _doc, _this, "op2_perc" );
	op2_trem_mdl.saveSettings( _doc, _this, "op2_trem" );
	op2_vib_mdl.saveSettings( _doc, _this, "op2_vib" );
	op2_waveform_mdl.saveSettings( _doc, _this, "op2_waveform" );

	fm_mdl.saveSettings( _doc, _this, "fm" );
	vib_depth_mdl.saveSettings( _doc, _this, "vib_depth" );
	trem_depth_mdl.saveSettings( _doc, _this, "trem_depth" );
}

void OpulenzInstrument::loadSettings( const QDomElement & _this )
{
	op1_a_mdl.loadSettings( _this, "op1_a" );
	op1_d_mdl.loadSettings( _this, "op1_d" );
	op1_s_mdl.loadSettings( _this, "op1_s" );
	op1_r_mdl.loadSettings( _this, "op1_r" );
	op1_lvl_mdl.loadSettings( _this, "op1_lvl" );
	op1_scale_mdl.loadSettings( _this, "op1_scale" );
	op1_mul_mdl.loadSettings( _this, "op1_mul" );
	feedback_mdl.loadSettings( _this, "feedback" );
	op1_ksr_mdl.loadSettings( _this, "op1_ksr" );
	op1_perc_mdl.loadSettings( _this, "op1_perc" );
	op1_trem_mdl.loadSettings( _this, "op1_trem" );
	op1_vib_mdl.loadSettings( _this, "op1_vib" );
	op1_waveform_mdl.loadSettings( _this, "op1_waveform" );

	op2_a_mdl.loadSettings( _this, "op2_a" );
	op2_d_mdl.loadSettings( _this, "op2_d" );
	op2_s_mdl.loadSettings( _this, "op2_s" );
	op2_r_mdl.loadSettings( _this, "op2_r" );
	op2_lvl_mdl.loadSettings( _this, "op2_lvl" );
	op2_scale_mdl.loadSettings( _this, "op2_scale" );
	op2_mul_mdl.loadSettings( _this, "op2_mul" );
	op2_ksr_mdl.loadSettings( _this, "op2_ksr" );
	op2_perc_mdl.loadSettings( _this, "op2_perc" );
	op2_trem_mdl.loadSettings( _this, "op2_trem" );
	op2_vib_mdl.loadSettings( _this, "op2_vib" );
	op2_waveform_mdl.loadSettings( _this, "op2_waveform" );

	fm_mdl.loadSettings( _this, "fm" );
	vib_depth_mdl.loadSettings( _this, "vib_depth" );
	trem_depth_mdl.loadSettings( _this, "trem_depth" );

}

// Load a patch into the emulator
void OpulenzInstrument::loadPatch(const unsigned char inst[14]) {
	emulatorMutex.lock();
	for(int v=0; v<OPL2_VOICES; ++v) {
		theEmulator->write(0x20+adlib_opadd[v],inst[0]); // op1 AM/VIB/EG/KSR/Multiplier
		theEmulator->write(0x23+adlib_opadd[v],inst[1]); // op2
		// theEmulator->write(0x40+adlib_opadd[v],inst[2]); // op1 KSL/Output Level - these are handled by noteon/aftertouch code
		// theEmulator->write(0x43+adlib_opadd[v],inst[3]); // op2
		theEmulator->write(0x60+adlib_opadd[v],inst[4]); // op1 A/D
		theEmulator->write(0x63+adlib_opadd[v],inst[5]); // op2
		theEmulator->write(0x80+adlib_opadd[v],inst[6]); // op1 S/R
		theEmulator->write(0x83+adlib_opadd[v],inst[7]); // op2
		theEmulator->write(0xe0+adlib_opadd[v],inst[8]); // op1 waveform
		theEmulator->write(0xe3+adlib_opadd[v],inst[9]); // op2
		theEmulator->write(0xc0+v,inst[10]);             // feedback/algorithm
	}
	emulatorMutex.unlock();
}

void OpulenzInstrument::tuneEqual(int center, float Hz) {
	for(int n=0; n<128; ++n) {
		float tmp = Hz * std::exp2((n - center) / 12.0f + pitchbend / 1200.0f);
		fnums[n] = Hz2fnum( tmp );
	}
}

// Find suitable F number in lowest possible block
int OpulenzInstrument::Hz2fnum(float Hz) {
	for(int block=0; block<8; ++block) {
		auto fnum = static_cast<unsigned>(Hz * std::exp2(20.0f - static_cast<float>(block)) / 49716.0f);
		if(fnum<1023) {
			return fnum + (block << 10);
		}
	}
	return 0;
}

// Load one of the default patches
void OpulenzInstrument::loadGMPatch() {
	const unsigned char* inst = midi_fm_instruments[m_patchModel.value()];
	loadPatch(inst);
}

// Update patch from the models to the chip emulation
void OpulenzInstrument::updatePatch() {
	auto inst = std::array<unsigned char, 14>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	inst[0] = ( op1_trem_mdl.value() ?  128 : 0  ) +
		( op1_vib_mdl.value() ?  64 : 0 ) +
		( op1_perc_mdl.value() ?  0 : 32 ) + // NB. This envelope mode is "perc", not "sus"
		( op1_ksr_mdl.value() ?  16 : 0 ) +
		((int)op1_mul_mdl.value() & 0x0f);
	inst[1] = ( op2_trem_mdl.value() ?  128 : 0  ) +
		( op2_vib_mdl.value() ?  64 : 0 ) +
		( op2_perc_mdl.value() ?  0 : 32 ) + // NB. This envelope mode is "perc", not "sus"
		( op2_ksr_mdl.value() ?  16 : 0 ) +
		((int)op2_mul_mdl.value() & 0x0f);
	inst[2] = ( ((int)op1_scale_mdl.value() & 0x03) << 6 ) +
		(63 - ( (int)op1_lvl_mdl.value() & 0x3f ) );
	inst[3] = ( ((int)op2_scale_mdl.value() & 0x03) << 6 ) +
		(63 - ( (int)op2_lvl_mdl.value() & 0x3f ) );
	inst[4] = ((15 - ((int)op1_a_mdl.value() & 0x0f ) ) << 4 )+
		(15 - ( (int)op1_d_mdl.value() & 0x0f ) );
	inst[5] = ((15 - ( (int)op2_a_mdl.value() & 0x0f ) ) << 4 )+
		(15 - ( (int)op2_d_mdl.value() & 0x0f ) );
	inst[6] = ((15 - ( (int)op1_s_mdl.value() & 0x0f ) ) << 4 ) +
		(15 - ( (int)op1_r_mdl.value() & 0x0f ) );
	inst[7] = ((15 - ( (int)op2_s_mdl.value() & 0x0f ) ) << 4 ) +
		(15 - ( (int)op2_r_mdl.value() & 0x0f ) );
	inst[8] = (int)op1_waveform_mdl.value() & 0x03;
	inst[9] = (int)op2_waveform_mdl.value() & 0x03;
	inst[10] = (fm_mdl.value() ? 0 : 1 ) +
		(((int)feedback_mdl.value() & 0x07 )<< 1);
	// These are always 0 in the list I had?
	inst[11] = 0;
	inst[12] = 0;
	inst[13] = 0;

	// Not part of the per-voice patch info
	theEmulator->write(0xBD, (trem_depth_mdl.value() ? 128 : 0 ) +
			   (vib_depth_mdl.value() ? 64 : 0 ));

	// have to do this, as the level knobs might've changed
	for( int voice = 0; voice < OPL2_VOICES ; ++voice) {
		if(voiceNote[voice] && OPL2_VOICE_FREE == 0) {
			setVoiceVelocity(voice, velocities[voiceNote[voice]] );
		}
	}
#ifdef false
		printf("UPD: %02x %02x %02x %02x %02x -- %02x %02x %02x %02x %02x %02x\n",
		       inst[0], inst[1], inst[2], inst[3], inst[4],
		       inst[5], inst[6], inst[7], inst[8], inst[9], inst[10]);
#endif


	loadPatch(inst.data());
}

// Load an SBI file into the knob models
void OpulenzInstrument::loadFile( const QString& file ) {
	// http://cd.textfiles.com/soundsensations/SYNTH/SBINS/
	// http://cd.textfiles.com/soundsensations/SYNTH/SBI1198/1198SBI.ZIP
	if( !file.isEmpty() && QFileInfo( file ).exists() )
	{
		QFile sbifile(file);
		if (!sbifile.open(QIODevice::ReadOnly )) {
			printf("Can't open file\n");
			return;
		}

		QByteArray sbidata = sbifile.read(52);
		if( !sbidata.startsWith("SBI\0x1a") ) {
			printf("No SBI signature\n");
			return;
		}
		if( sbidata.size() != 52 ) {
			printf("SBI size error: expected 52, got %d\n",sbidata.size() );
		}

		// Minimum size of SBI if we ignore "reserved" bytes at end
		// https://courses.engr.illinois.edu/ece390/resources/sound/cmf.txt.html
		if( sbidata.size() < 47 ) {
			return;
		}

		QString sbiname = sbidata.mid(4, 32);
		// If user has changed track name... let's hope my logic is valid.
		if( sbiname.size() > 0 && instrumentTrack()->displayName() == storedname ) {
			instrumentTrack()->setName(sbiname);
			storedname = sbiname;
		}

#ifdef false
		printf("SBI: %02x %02x %02x %02x %02x -- %02x %02x %02x %02x %02x %02x\n",
		       (unsigned char)sbidata[36], (unsigned char)sbidata[37], (unsigned char)sbidata[38], (unsigned char)sbidata[39], (unsigned char)sbidata[40],
		       (unsigned char)sbidata[41], (unsigned char)sbidata[42], (unsigned char)sbidata[43], (unsigned char)sbidata[44], (unsigned char)sbidata[45], (unsigned char)sbidata[46]);
#endif
		// Modulator Sound Characteristic (Mult, KSR, EG, VIB, AM)
		op1_trem_mdl.setValue( (sbidata[36] & 0x80 ) == 0x80 ? true : false );
		op1_vib_mdl.setValue( (sbidata[36] & 0x40 ) == 0x40 ? true : false );
		op1_perc_mdl.setValue( (sbidata[36] & 0x20 ) == 0x20 ? false : true );
		op1_ksr_mdl.setValue( (sbidata[36] & 0x10 ) == 0x10 ? true : false );
		op1_mul_mdl.setValue( sbidata[36] & 0x0f );

		// Carrier Sound Characteristic
		op2_trem_mdl.setValue( (sbidata[37] & 0x80 ) == 0x80 ? true : false );
		op2_vib_mdl.setValue( (sbidata[37] & 0x40 ) == 0x40 ? true : false );
		op2_perc_mdl.setValue( (sbidata[37] & 0x20 ) == 0x20 ? false : true );
		op2_ksr_mdl.setValue( (sbidata[37] & 0x10 ) == 0x10 ? true : false );
		op2_mul_mdl.setValue( sbidata[37] & 0x0f );

		// Modulator Scaling/Output Level
		op1_scale_mdl.setValue( (sbidata[38] & 0xc0 ) >> 6 );
		op1_lvl_mdl.setValue( 63 - (sbidata[38] & 0x3f) );

		// Carrier Scaling/Output Level
		op2_scale_mdl.setValue( (sbidata[39] & 0xc0) >> 6 );
		op2_lvl_mdl.setValue( 63 - (sbidata[39] & 0x3f) );

		// Modulator Attack/Decay
		op1_a_mdl.setValue( 15 - ( ( sbidata[40] & 0xf0 ) >> 4 ) );
		op1_d_mdl.setValue( 15 - ( sbidata[40] & 0x0f ) );

		// Carrier Attack/Decay
		op2_a_mdl.setValue( 15 - ( ( sbidata[41] & 0xf0 ) >> 4 ) );
		op2_d_mdl.setValue( 15 - ( sbidata[41] & 0x0f ) );

		// Modulator Sustain/Release
		op1_s_mdl.setValue( 15 - ( ( sbidata[42] & 0xf0 ) >> 4 ) );
		op1_r_mdl.setValue( 15 - ( sbidata[42] & 0x0f ) );

		// Carrier Sustain/Release
		op2_s_mdl.setValue( 15 - ( ( sbidata[43] & 0xf0 ) >> 4 ) );
		op2_r_mdl.setValue( 15 - ( sbidata[43] & 0x0f ) );

		// Modulator Wave Select
		op1_waveform_mdl.setValue( sbidata[44] & 0x03 );

		// Carrier Wave Select
		op2_waveform_mdl.setValue( sbidata[45] & 0x03 );

		// Feedback/Connection
		fm_mdl.setValue( (sbidata[46] & 0x01) == 0x01 ? false : true );
		feedback_mdl.setValue( ( (sbidata[46] & 0x0e ) >> 1 ) );
	}
}



namespace gui
{

OpulenzInstrumentView::OpulenzInstrumentView( Instrument * _instrument,
                                                        QWidget * _parent ) :
        InstrumentViewFixedSize( _instrument, _parent )
{

#define KNOB_GEN(knobname, hinttext, hintunit,xpos,ypos) \
	knobname = new Knob( KnobType::Styled, this );\
	knobname->setHintText( tr(hinttext), hintunit );\
	knobname->setFixedSize(22,22);\
	knobname->setCenterPointX(11.0);\
	knobname->setCenterPointY(11.0);\
	knobname->setTotalAngle(270.0);\
	knobname->move(xpos,ypos);

#define BUTTON_GEN(buttname, tooltip, xpos, ypos) \
	buttname = new PixmapButton( this, nullptr );\
        buttname->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "led_on" ) );\
        buttname->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "led_off" ) );\
	buttname->setCheckable( true );\
        buttname->setToolTip(tr(tooltip));\
        buttname->move( xpos, ypos );

#define WAVEBUTTON_GEN(buttname, tooltip, xpos, ypos, icon_on, icon_off, buttgroup) \
	buttname = new PixmapButton( this, nullptr );\
        buttname->setActiveGraphic( PLUGIN_NAME::getIconPixmap( icon_on ) ); \
        buttname->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( icon_off ) ); \
        buttname->setToolTip(tr(tooltip));\
        buttname->move( xpos, ypos );\
	buttgroup->addButton(buttname);


	// OP1 knobs & buttons...
	KNOB_GEN(op1_a_kn, "Attack", "", 6, 48);
	KNOB_GEN(op1_d_kn, "Decay", "", 34, 48);
	KNOB_GEN(op1_s_kn, "Sustain", "", 62, 48);
	KNOB_GEN(op1_r_kn, "Release", "", 90, 48);
	KNOB_GEN(op1_lvl_kn, "Level", "", 166, 48);
	KNOB_GEN(op1_scale_kn, "Scale", "", 194, 48);
	KNOB_GEN(op1_mul_kn, "Frequency multiplier", "", 222, 48);
	BUTTON_GEN(op1_ksr_btn, "Keyboard scaling rate", 9, 87);
	BUTTON_GEN(op1_perc_btn, "Percussive envelope", 36, 87);
	BUTTON_GEN(op1_trem_btn, "Tremolo", 65, 87);
	BUTTON_GEN(op1_vib_btn, "Vibrato", 93, 87);
	KNOB_GEN(feedback_kn, "Feedback", "", 128, 48);

	op1_waveform = new AutomatableButtonGroup( this );
	WAVEBUTTON_GEN(op1_w0_btn,"Sine", 154, 86, "wave1_on", "wave1_off", op1_waveform);
	WAVEBUTTON_GEN(op1_w1_btn,"Half sine", 178, 86, "wave2_on", "wave2_off", op1_waveform);
	WAVEBUTTON_GEN(op1_w2_btn,"Absolute sine", 199, 86, "wave3_on", "wave3_off", op1_waveform);
	WAVEBUTTON_GEN(op1_w3_btn,"Quarter sine", 220, 86, "wave4_on", "wave4_off", op1_waveform);


	// And the same for OP2
	KNOB_GEN(op2_a_kn, "Attack", "", 6, 138);
	KNOB_GEN(op2_d_kn, "Decay", "", 34, 138);
	KNOB_GEN(op2_s_kn, "Sustain", "", 62, 138);
	KNOB_GEN(op2_r_kn, "Release", "", 90, 138);
	KNOB_GEN(op2_lvl_kn, "Level", "", 166, 138);
	KNOB_GEN(op2_scale_kn, "Scale", "", 194, 138);
	KNOB_GEN(op2_mul_kn, "Frequency multiplier", "", 222, 138);
	BUTTON_GEN(op2_ksr_btn, "Keyboard scaling rate", 9, 177);
	BUTTON_GEN(op2_perc_btn, "Percussive envelope", 36, 177);
	BUTTON_GEN(op2_trem_btn, "Tremolo", 65, 177);
	BUTTON_GEN(op2_vib_btn, "Vibrato", 93, 177);

	op2_waveform = new AutomatableButtonGroup( this );
	WAVEBUTTON_GEN(op2_w0_btn,"Sine", 154, 176, "wave1_on", "wave1_off", op2_waveform);
	WAVEBUTTON_GEN(op2_w1_btn,"Half sine", 178, 176, "wave2_on", "wave2_off", op2_waveform);
	WAVEBUTTON_GEN(op2_w2_btn,"Absolute sine", 199, 176, "wave3_on", "wave3_off", op2_waveform);
	WAVEBUTTON_GEN(op2_w3_btn,"Quarter Sine", 220, 176, "wave4_on", "wave4_off", op2_waveform);

	BUTTON_GEN(fm_btn, "FM", 9, 220);
	BUTTON_GEN(vib_depth_btn, "Vibrato depth", 65, 220);
	BUTTON_GEN(trem_depth_btn, "Tremolo depth", 93, 220);


	setAutoFillBackground( true );
        QPalette pal;
        pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
                                                                "artwork" ) );
        setPalette( pal );
}
OpulenzInstrumentView::~OpulenzInstrumentView() {
	// Knobs are QWidgets and our children, so they're
	// destroyed automagically
}

// Returns text for time knob formatted nicely
inline QString OpulenzInstrumentView::knobHintHelper(float n) {
	if(n>1000) {
		return QString::number(n/1000, 'f', 0)+ " s";
	} else if(n>10) {
		return QString::number(n, 'f', 0)+ " ms";
	} else {
		return QString::number(n, 'f', 1)+ " ms";
	}
}

void OpulenzInstrumentView::updateKnobHints()
{
	// Envelope times in ms: t[0] = 0, t[n] = ( 1<<n ) * X, X = 0.11597 for A, 0.6311 for D/R
	// Here some rounding has been applied.
	const auto attack_times = std::array<float, 16>{
		0.f, 0.2f, 0.4f, 0.9f, 1.8f, 3.7f, 7.4f,
		15.f, 30.f, 60.f, 120.f, 240.f, 480.f,
		950.f, 1900.f, 3800.f
	};

	const auto dr_times = std::array<float, 16>{
		0.f, 1.2f, 2.5f, 5.f, 10.f, 20.f, 40.f,
		80.f, 160.f, 320.f, 640.f, 1300.f, 2600.f,
		5200.f, 10000.f, 20000.f
	};

	const auto fmultipliers = std::array<int, 16>{
		-12, 0, 12, 19, 24, 28, 31, 34, 36, 38, 40, 40, 43, 43, 47, 47
	};

	auto m = castModel<OpulenzInstrument>();

	op1_a_kn->setHintText( tr( "Attack" ),
						   " (" + knobHintHelper(attack_times[(int)m->op1_a_mdl.value()]) + ")");
	op2_a_kn->setHintText( tr( "Attack" ),
						   " (" + knobHintHelper(attack_times[(int)m->op2_a_mdl.value()]) + ")");
	op1_d_kn->setHintText( tr( "Decay" ),
						   " (" + knobHintHelper(dr_times[(int)m->op1_d_mdl.value()]) + ")");
	op2_d_kn->setHintText( tr( "Decay" ),
						   " (" + knobHintHelper(dr_times[(int)m->op2_d_mdl.value()]) + ")");
	op1_r_kn->setHintText( tr( "Release" ),
						   " (" + knobHintHelper(dr_times[(int)m->op1_r_mdl.value()]) + ")");
	op2_r_kn->setHintText( tr( "Release" ),
						   " (" + knobHintHelper(dr_times[(int)m->op2_r_mdl.value()]) + ")");
	op1_mul_kn->setHintText( tr( "Frequency multiplier" ),
			       " (" + QString::number(fmultipliers[(int)m->op1_mul_mdl.value()]) + " semitones)");
	op2_mul_kn->setHintText( tr( "Frequency multiplier" ),
			       " (" + QString::number(fmultipliers[(int)m->op2_mul_mdl.value()]) + " semitones)");
}

void OpulenzInstrumentView::modelChanged()
{
	auto m = castModel<OpulenzInstrument>();
	// m_patch->setModel( &m->m_patchModel );

	op1_a_kn->setModel( &m->op1_a_mdl );
	op1_d_kn->setModel( &m->op1_d_mdl );
	op1_s_kn->setModel( &m->op1_s_mdl );
	op1_r_kn->setModel( &m->op1_r_mdl );
	op1_lvl_kn->setModel( &m->op1_lvl_mdl );
	op1_scale_kn->setModel( &m->op1_scale_mdl );
	op1_mul_kn->setModel( &m->op1_mul_mdl );
	feedback_kn->setModel( &m->feedback_mdl );
	op1_ksr_btn->setModel( &m->op1_ksr_mdl );
	op1_perc_btn->setModel( &m->op1_perc_mdl );
	op1_trem_btn->setModel( &m->op1_trem_mdl );
	op1_vib_btn->setModel( &m->op1_vib_mdl );
	op1_waveform->setModel( &m->op1_waveform_mdl );


	op2_a_kn->setModel( &m->op2_a_mdl );
	op2_d_kn->setModel( &m->op2_d_mdl );
	op2_s_kn->setModel( &m->op2_s_mdl );
	op2_r_kn->setModel( &m->op2_r_mdl );
	op2_lvl_kn->setModel( &m->op2_lvl_mdl );
	op2_scale_kn->setModel( &m->op2_scale_mdl );
	op2_mul_kn->setModel( &m->op2_mul_mdl );
	op2_ksr_btn->setModel( &m->op2_ksr_mdl );
	op2_perc_btn->setModel( &m->op2_perc_mdl );
	op2_trem_btn->setModel( &m->op2_trem_mdl );
	op2_vib_btn->setModel( &m->op2_vib_mdl );
	op2_waveform->setModel( &m->op2_waveform_mdl );

	fm_btn->setModel( &m->fm_mdl );
	vib_depth_btn->setModel( &m->vib_depth_mdl );
	trem_depth_btn->setModel( &m->trem_depth_mdl );


	// All knobs needing a user friendly unit
	connect( &m->op1_a_mdl, SIGNAL( dataChanged() ), this, SLOT( updateKnobHints() ) );
	connect( &m->op2_a_mdl, SIGNAL( dataChanged() ), this, SLOT( updateKnobHints() ) );

	connect( &m->op1_d_mdl, SIGNAL( dataChanged() ), this, SLOT( updateKnobHints() ) );
	connect( &m->op2_d_mdl, SIGNAL( dataChanged() ), this, SLOT( updateKnobHints() ) );

	connect( &m->op1_r_mdl, SIGNAL( dataChanged() ), this, SLOT( updateKnobHints() ) );
	connect( &m->op2_r_mdl, SIGNAL( dataChanged() ), this, SLOT( updateKnobHints() ) );

	connect( &m->op1_mul_mdl, SIGNAL( dataChanged() ), this, SLOT( updateKnobHints() ) );
	connect( &m->op2_mul_mdl, SIGNAL( dataChanged() ), this, SLOT( updateKnobHints() ) );

	updateKnobHints();

}


} // namespace gui

} // namespace lmms
