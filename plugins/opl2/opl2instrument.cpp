/*
 * OPL2 FM synth
 *
 * Copyright (c) 2014 Raine M. Ekman <raine/at/iki/fi>
 *
 * This file is part of LMMS - http://lmms.io
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
// - .sbi (or similar) file loading into models
// - RT safety = get rid of mutex = make emulator code thread-safe

// - Extras:
//   - double release: first release is in effect until noteoff (heard if percussive sound),
//     second is switched in just before key bit cleared (is this useful???)
//   - Unison: 2,3,4, or 9 voices with configurable spread?
//   - Portamento (needs mono mode?)
//     - Pre-bend/post-bend in poly mode could use portamento speed?
//   - SBI file import?

// - Envelope times in ms for UI: t[0] = 0, t[n] = ( 1<<n ) * X, X = 0.11597 for A, 0.6311 for D/R
//   - attack 0.0, 0.23194, 0.46388, 0.92776, 1.85552, 3.71104, 7.42208, 14.84416,
//           29.68832, 59.37664, 118.75328, 237.50656, 475.01312, 950.02624, 1900.05248, 3800.10496
//   -decay/release 0.0, 1.2622, 2.5244, 5.0488, 10.0976, 20.1952, 40.3904, 80.7808, 161.5616,
//                323.1232, 646.2464, 1292.4928, 2584.9856, 5169.9712, 10339.9424, 20679.8848

#include "opl2instrument.h"
#include "mididata.h"
#include "debug.h"
#include "Instrument.h"
#include "engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"

#include <QtXml/QDomDocument>

#include "opl.h"
#include "temuopl.h"

#include "embed.cpp"
#include "math.h"

#include "knob.h"
#include "LcdSpinBox.h"
#include "pixmap_button.h"
#include "tooltip.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT OPL2_plugin_descriptor =
{
        STRINGIFY( PLUGIN_NAME ),
        "OpulenZ",
        QT_TRANSLATE_NOOP( "pluginBrowser",
			   "2-operator FM Synth" ),
        "Raine M. Ekman <raine/at/iki/fi>",
        0x0100,
        Plugin::Instrument,
        new PluginPixmapLoader( "logo" ),
        NULL,
        NULL
};

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
        return( new opl2instrument( static_cast<InstrumentTrack *>( _data ) ) );
}

}

// I'd much rather do without a mutex, but it looks like
// the emulator code isn't really ready for threads
QMutex opl2instrument::emulatorMutex;

// Weird ordering of voice parameters
const unsigned int adlib_opadd[9] = {0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12};

opl2instrument::opl2instrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &OPL2_plugin_descriptor ),
	m_patchModel( 0, 0, 127, this, tr( "Patch" ) ),
	op1_a_mdl(14.0, 0.0, 15.0, 1.0, this, tr( "Op 1 Attack" )  ),
	op1_d_mdl(14.0, 0.0, 15.0, 1.0, this, tr( "Op 1 Decay" )   ),
	op1_s_mdl(3.0, 0.0, 15.0, 1.0, this, tr( "Op 1 Sustain" )   ),
	op1_r_mdl(10.0, 0.0, 15.0, 1.0, this, tr( "Op 1 Release" )   ),
	op1_lvl_mdl(62.0, 0.0, 63.0, 1.0, this, tr( "Op 1 Level" )   ),
	op1_scale_mdl(0.0, 0.0, 3.0, 1.0, this, tr( "Op 1 Level Scaling" ) ),
	op1_mul_mdl(0.0, 0.0, 15.0, 1.0, this, tr( "Op 1 Frequency Multiple" ) ),
	feedback_mdl(0.0, 0.0, 7.0, 1.0, this, tr( "Op 1 Feedback" )    ),
	op1_ksr_mdl(false, this, tr( "Op 1 Key Scaling Rate" ) ),
	op1_perc_mdl(false, this, tr( "Op 1 Percussive Envelope" )   ),
	op1_trem_mdl(true, this, tr( "Op 1 Tremolo" )   ),
	op1_vib_mdl(false, this, tr( "Op 1 Vibrato" )   ),
	op1_w0_mdl(  ),
	op1_w1_mdl(  ),
	op1_w2_mdl(  ),
	op1_w3_mdl(  ),
	op1_waveform_mdl(0,0,3,this, tr( "Op 1 Waveform" ) ),


	op2_a_mdl(1.0, 0.0, 15.0, 1.0, this, tr( "Op 2 Attack" )   ),
	op2_d_mdl(3.0, 0.0, 15.0, 1.0, this, tr( "Op 2 Decay" )   ),
	op2_s_mdl(14.0, 0.0, 15.0, 1.0, this, tr( "Op 2 Sustain" ) ),
	op2_r_mdl(12.0, 0.0, 15.0, 1.0, this, tr( "Op 2 Release" )   ),
	op2_lvl_mdl(63.0, 0.0, 63.0, 1.0, this, tr( "Op 2 Level" )   ),
	op2_scale_mdl(0.0, 0.0, 3.0, 1.0, this, tr( "Op 2 Level Scaling" ) ),
	op2_mul_mdl(1.0, 0.0, 15.0, 1.0, this, tr( "Op 2 Frequency Multiple" ) ),
	op2_ksr_mdl(false, this, tr( "Op 2 Key Scaling Rate" ) ),
	op2_perc_mdl(false, this, tr( "Op 2 Percussive Envelope" )   ),
	op2_trem_mdl(false, this, tr( "Op 2 Tremolo" )   ),
	op2_vib_mdl(true, this, tr( "Op 2 Vibrato" )   ),
	op2_w0_mdl(  ),
	op2_w1_mdl(  ),
	op2_w2_mdl(  ),
	op2_w3_mdl(  ),
	op2_waveform_mdl(0,0,3,this, tr( "Op 2 Waveform" ) ),

	fm_mdl(true, this, tr( "FM" )   ),
	vib_depth_mdl(false, this, tr( "Vibrato Depth" )   ),
	trem_depth_mdl(false, this, tr( "Tremolo Depth" )   )
{
	// Connect the plugin to the mixer...
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( iph );

	// Voices are laid out in a funny way...
	// adlib_opadd = {0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12};

	// Create an emulator - samplerate, 16 bit, mono
	emulatorMutex.lock();
	theEmulator = new CTemuopl(engine::mixer()->processingSampleRate(), true, false);
	theEmulator->init();
	// Enable waveform selection
	theEmulator->write(0x01,0x20);
	emulatorMutex.unlock();

	//Initialize voice values
	voiceNote[0] = 0;
	voiceLRU[0] = 0;
	for(int i=1; i<9; ++i) {
		voiceNote[i] = OPL2_VOICE_FREE;
		voiceLRU[i] = i;
	}

	updatePatch();

	// Can the buffer size change suddenly? I bet that would break lots of stuff
	frameCount = engine::mixer()->framesPerPeriod();
	renderbuffer = new short[frameCount];

	// Some kind of sane defaults
	pitchbend = 0;
	pitchBendRange = 100;
	RPNcoarse = RPNfine = 255;

	tuneEqual(69, 440);

	connect( engine::mixer(), SIGNAL( sampleRateChanged() ),
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
}

opl2instrument::~opl2instrument() {
	delete theEmulator;
	engine::mixer()->removePlayHandles( instrumentTrack() );
	delete [] renderbuffer;
}

// Samplerate changes when choosing oversampling, so this is more or less mandatory
void opl2instrument::reloadEmulator() {
	delete theEmulator;
	emulatorMutex.lock();
	theEmulator = new CTemuopl(engine::mixer()->processingSampleRate(), true, false);
	theEmulator->init();
	theEmulator->write(0x01,0x20);
	emulatorMutex.unlock();
	for(int i=1; i<9; ++i) {
		voiceNote[i] = OPL2_VOICE_FREE;
		voiceLRU[i] = i;
	}
	updatePatch();
}

// This shall only be called from code protected by the holy Mutex!
void opl2instrument::setVoiceVelocity(int voice, int vel) {
	int vel_adjusted;
	// Velocity calculation, some kind of approximation
	// Only calculate for operator 1 if in adding mode, don't want to change timbre
	if( fm_mdl.value() == false ) {
		vel_adjusted = 63 - ( op1_lvl_mdl.value() * vel/127.0) ;
	} else {
		vel_adjusted = 63 - op1_lvl_mdl.value();
	}
	theEmulator->write(0x40+adlib_opadd[voice],
			   ( (int)op1_scale_mdl.value() & 0x03 << 6) +
			   ( vel_adjusted & 0x3f ) );


	vel_adjusted = 63 - ( op2_lvl_mdl.value() * vel/127.0 );
	// vel_adjusted = 63 - op2_lvl_mdl.value();
	theEmulator->write(0x43+adlib_opadd[voice],
			   ( (int)op2_scale_mdl.value() & 0x03 << 6) +
			   ( vel_adjusted & 0x3f ) );
}

// Pop least recently used voice - why does it sometimes lose a voice (mostly 0)?
int opl2instrument::popVoice() {
	int tmp = voiceLRU[0];
	for( int i=0; i<8; ++i) {
		voiceLRU[i] = voiceLRU[i+1];
	}
	voiceLRU[8] = OPL2_NO_VOICE;
	return tmp;
}

int opl2instrument::pushVoice(int v) {
	int i;
	for(i=8; i>0; --i) {
		if( voiceLRU[i-1] != OPL2_NO_VOICE ) {
			break;
		}
	}
	voiceLRU[i] = v;
	return i;
}

bool opl2instrument::handleMidiEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset )
{
	emulatorMutex.lock();
	int key, vel, voice, tmp_pb;

	switch(event.type()) {
        case MidiNoteOn:
		// to get us in line with MIDI(?)
		key = event.key() +12;
		vel = event.velocity();

		voice = popVoice();
		if( voice != OPL2_NO_VOICE ) {
			// Turn voice on, NB! the frequencies are straight by voice number,
			// not by the adlib_opadd table!
			theEmulator->write(0xA0+voice, fnums[key] & 0xff);
			theEmulator->write(0xB0+voice, 32 + ((fnums[key] & 0x1f00) >> 8) );
			setVoiceVelocity(voice, vel);
			voiceNote[voice] = key;
			velocities[key] = vel;
		}
                break;
        case MidiNoteOff:
                key = event.key() +12;
                for(voice=0; voice<9; ++voice) {
                        if( voiceNote[voice] == key ) {
                                theEmulator->write(0xA0+voice, fnums[key] & 0xff);
                                theEmulator->write(0xB0+voice, (fnums[key] & 0x1f00) >> 8 );
                                voiceNote[voice] = OPL2_VOICE_FREE;
				pushVoice(voice);
                        }
                }
		velocities[key] = 0;
                break;
        case MidiKeyPressure:
                key = event.key() +12;
                vel = event.velocity();
		if( velocities[key] != 0) {
			velocities[key] = vel;
		}
		for(voice=0; voice<9; ++voice) {
			if(voiceNote[voice] == key) {
				setVoiceVelocity(voice, vel);
			}
		}
                break;
        case MidiPitchBend:
		// Update fnumber table
		// Pitchbend should be in the range 0...16383 but the new range knob gets it wrong.
		// tmp_pb = (2*BEND_CENTS)*((float)event.m_data.m_param[0]/16383)-BEND_CENTS;

		// Something like 100 cents = 8192, but offset by 8192 so the +/-100 cents range goes from 0...16383?
		tmp_pb = ( event.pitchBend()-8192 ) * pitchBendRange / 8192;

		if( tmp_pb != pitchbend ) {
			pitchbend = tmp_pb;
			tuneEqual(69, 440.0);
		}
		// Update pitch of sounding notes
		for( int v=0; v<9; ++v ) {
			if( voiceNote[v] != OPL2_VOICE_FREE ) {
				theEmulator->write(0xA0+v, fnums[voiceNote[v] ] & 0xff);
				theEmulator->write(0xB0+v, 32 + ((fnums[voiceNote[v]] & 0x1f00) >> 8) );
			}
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
			printf("Midi CC %02x %02x\n", event.controllerNumber(), event.controllerValue() );
			break;
		}
		break;
        default:
                printf("Midi event type %d\n",event.type());
        }
	emulatorMutex.unlock();
	return true;
}

QString opl2instrument::nodeName() const
{
        return( OPL2_plugin_descriptor.name );
}

PluginView * opl2instrument::instantiateView( QWidget * _parent )
{
        return( new opl2instrumentView( this, _parent ) );
}


void opl2instrument::play( sampleFrame * _working_buffer )
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

	// Throw the data to the track...
	instrumentTrack()->processAudioBuffer( _working_buffer, frameCount, NULL );

}


void opl2instrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
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

void opl2instrument::loadSettings( const QDomElement & _this )
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
void opl2instrument::loadPatch(unsigned char inst[14]) {
	emulatorMutex.lock();
	for(int v=0; v<9; ++v) {
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

void opl2instrument::tuneEqual(int center, float Hz) {
	float tmp;
	for(int n=0; n<128; ++n) {
		tmp = Hz*pow( 2.0, ( n - center ) * ( 1.0 / 12.0 ) + pitchbend * ( 1.0 / 1200.0 ) );
		fnums[n] = Hz2fnum( tmp );
	}
}

// Find suitable F number in lowest possible block
int opl2instrument::Hz2fnum(float Hz) {
	for(int block=0; block<8; ++block) {
		unsigned int fnum = Hz * pow( 2.0, 20.0 - (double)block ) * ( 1.0 / 49716.0 );
		if(fnum<1023) {
			return fnum + (block << 10);
		}
	}
	return 0;
}

// Load one of the default patches
void opl2instrument::loadGMPatch() {
	unsigned char *inst = midi_fm_instruments[m_patchModel.value()];
	loadPatch(inst);
}

//
/* void opl2instrument::loadSBIFile() {

   } */

// Update patch from the models to the chip emulation
void opl2instrument::updatePatch() {
	unsigned char *inst = midi_fm_instruments[0];
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
	inst[2] = ( (int)op1_scale_mdl.value() & 0x03 << 6 ) +
		(63 - ( (int)op1_lvl_mdl.value() & 0x3f ) );
	inst[3] = ( (int)op2_scale_mdl.value() & 0x03 << 6 ) +
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
	for( int voice = 0; voice < 9 ; ++voice) {
		if(voiceNote[voice]!=OPL2_VOICE_FREE) {
			setVoiceVelocity(voice, velocities[voiceNote[voice]] );
		}
	}
	loadPatch(inst);
}



opl2instrumentView::opl2instrumentView( Instrument * _instrument,
                                                        QWidget * _parent ) :
        InstrumentView( _instrument, _parent )
{
	/* Unnecessary?
	   m_patch = new LcdSpinBox( 3, this , "PRESET");
	   m_patch->setLabel( "PRESET" );
	   m_patch->move( 100, 1 );
	   m_patch->setEnabled( true );
	*/

#define KNOB_GEN(knobname, hinttext, hintunit,xpos,ypos) \
	knobname = new knob( knobStyled, this );\
	knobname->setHintText( tr(hinttext) + "", hintunit );\
	knobname->setFixedSize(22,22);\
	knobname->setCenterPointX(11.0);\
	knobname->setCenterPointY(11.0);\
	knobname->setTotalAngle(270.0);\
	knobname->move(xpos,ypos);

#define BUTTON_GEN(buttname, tooltip, xpos, ypos) \
	buttname = new pixmapButton( this, NULL );\
        buttname->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "opl2_led_on" ) );\
        buttname->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "opl2_led_off" ) );\
	buttname->setCheckable( true );\
        toolTip::add( buttname, tr( tooltip ) );\
        buttname->move( xpos, ypos );

#define WAVEBUTTON_GEN(buttname, tooltip, xpos, ypos, icon_on, icon_off, buttgroup) \
	buttname = new pixmapButton( this, NULL );\
        buttname->setActiveGraphic( PLUGIN_NAME::getIconPixmap( icon_on ) ); \
        buttname->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( icon_off ) ); \
        toolTip::add( buttname, tr( tooltip ) );\
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

	op1_waveform = new automatableButtonGroup( this );
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

	op2_waveform = new automatableButtonGroup( this );
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
opl2instrumentView::~opl2instrumentView() {
	// Nobody else seems to delete their knobs and buttons?
}

void opl2instrumentView::modelChanged()
{
	opl2instrument * m = castModel<opl2instrument>();
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
	/* op1_w0_btn->setModel( &m->op1_w0_mdl );
	op1_w1_btn->setModel( &m->op1_w1_mdl );
	op1_w2_btn->setModel( &m->op1_w2_mdl );
	op1_w3_btn->setModel( &m->op1_w3_mdl ); */
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
	/* op2_w0_btn->setModel( &m->op2_w0_mdl );
	op2_w1_btn->setModel( &m->op2_w1_mdl );
	op2_w2_btn->setModel( &m->op2_w2_mdl );
	op2_w3_btn->setModel( &m->op2_w3_mdl ); */
	op2_waveform->setModel( &m->op2_waveform_mdl );

	fm_btn->setModel( &m->fm_mdl );
	vib_depth_btn->setModel( &m->vib_depth_mdl );
	trem_depth_btn->setModel( &m->trem_depth_mdl );

}


#include "moc_opl2instrument.cxx"
