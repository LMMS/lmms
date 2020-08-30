/*
 * lb302.cpp - implementation of class lb302 which is a bass synth attempting
 *             to emulate the Roland TB303 bass synth
 *
 * Copyright (c) 2006-2008 Paul Giblock <pgib/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * lb302FilterIIR2 is based on the gsyn filter code by Andy Sloane.
 *
 * lb302Filter3Pole is based on the TB303 instrument written by
 *   Josep M Comajuncosas for the CSounds library
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

// Need to include this first to ensure we get M_PI in MinGW with C++11
#define _USE_MATH_DEFINES
#include <math.h>

#include "lb302.h"
#include "AutomatableButton.h"
#include "Engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "ToolTip.h"
#include "BandLimitedWave.h"

#include "embed.h"
#include "plugin_export.h"

// Envelope Recalculation period
#define ENVINC 64

//
// New config
//
#define LB_24_IGNORE_ENVELOPE
#define LB_FILTERED
//#define LB_DECAY
//#define LB_24_RES_TRICK

#define LB_DIST_RATIO    4.0
#define LB_24_VOL_ADJUST 3.0
//#define LB_DECAY_NOTES

#define LB_DEBUG

#ifdef LB_DEBUG
#include <assert.h>
#endif

//
// Old config
//


//#define engine::mixer()->processingSampleRate() 44100.0f


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT lb302_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"LB302",
	QT_TRANSLATE_NOOP( "PluginBrowser",
			"Incomplete monophonic imitation tb303" ),
	"Paul Giblock <pgib/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
};

}

//
// lb302Filter
//

lb302Filter::lb302Filter(lb302FilterKnobState* p_fs) :
	fs(p_fs),
	vcf_c0(0),
	vcf_e0(0),
	vcf_e1(0)
{
};


void lb302Filter::recalc()
{
	vcf_e1 = exp(6.109 + 1.5876*(fs->envmod) + 2.1553*(fs->cutoff) - 1.2*(1.0-(fs->reso)));
	vcf_e0 = exp(5.613 - 0.8*(fs->envmod) + 2.1553*(fs->cutoff) - 0.7696*(1.0-(fs->reso)));
	vcf_e0*=M_PI/Engine::mixer()->processingSampleRate();
	vcf_e1*=M_PI/Engine::mixer()->processingSampleRate();
	vcf_e1 -= vcf_e0;

	vcf_rescoeff = exp(-1.20 + 3.455*(fs->reso));
};


void lb302Filter::envRecalc()
{
	vcf_c0 *= fs->envdecay;       // Filter Decay. vcf_decay is adjusted for Hz and ENVINC
	// vcf_rescoeff = exp(-1.20 + 3.455*(fs->reso)); moved above
};


void lb302Filter::playNote()
{
	vcf_c0 = vcf_e1;
}


//
// lb302FilterIIR2
//

lb302FilterIIR2::lb302FilterIIR2(lb302FilterKnobState* p_fs) :
	lb302Filter(p_fs),
	vcf_d1(0),
	vcf_d2(0),
	vcf_a(0),
	vcf_b(0),
	vcf_c(1)
{

	m_dist = new DspEffectLibrary::Distortion( 1.0, 1.0f);

};


lb302FilterIIR2::~lb302FilterIIR2()
{
	delete m_dist;
}


void lb302FilterIIR2::recalc()
{
	lb302Filter::recalc();
	//m_dist->setThreshold(0.5+(fs->dist*2.0));
	m_dist->setThreshold(fs->dist*75.0);
};


void lb302FilterIIR2::envRecalc()
{
	float k, w;

	lb302Filter::envRecalc();

	w = vcf_e0 + vcf_c0;          // e0 is adjusted for Hz and doesn't need ENVINC
	k = exp(-w/vcf_rescoeff);     // Does this mean c0 is inheritantly?

	vcf_a = 2.0*cos(2.0*w) * k;
	vcf_b = -k*k;
	vcf_c = 1.0 - vcf_a - vcf_b;
}


float lb302FilterIIR2::process(const float& samp)
{
	float ret = vcf_a*vcf_d1 + vcf_b*vcf_d2 + vcf_c*samp;
	// Delayed samples for filter
	vcf_d2 = vcf_d1;
	vcf_d1 = ret;

	if(fs->dist > 0)
		ret=m_dist->nextSample(ret);

	// output = IIR2 + dry
	return ret;
}


//
// lb302Filter3Pole
//

lb302Filter3Pole::lb302Filter3Pole(lb302FilterKnobState *p_fs) :
	lb302Filter(p_fs),
	ay1(0),
	ay2(0),
	aout(0),
	lastin(0)
{
};


void lb302Filter3Pole::recalc()
{
	// DO NOT CALL BASE CLASS
	vcf_e0 = 0.000001;
	vcf_e1 = 1.0;
}


// TODO: Try using k instead of vcf_reso
void lb302Filter3Pole::envRecalc()
{
	float w,k;
	float kfco;

	lb302Filter::envRecalc();

	// e0 is adjusted for Hz and doesn't need ENVINC
	w = vcf_e0 + vcf_c0;
	k = (fs->cutoff > 0.975)?0.975:fs->cutoff;
	kfco = 50.f + (k)*((2300.f-1600.f*(fs->envmod))+(w) *
	                   (700.f+1500.f*(k)+(1500.f+(k)*(Engine::mixer()->processingSampleRate()/2.f-6000.f)) *
	                   (fs->envmod)) );
	//+iacc*(.3+.7*kfco*kenvmod)*kaccent*kaccurve*2000


#ifdef LB_24_IGNORE_ENVELOPE
	// kfcn = fs->cutoff;
	kfcn = 2.0 * kfco / Engine::mixer()->processingSampleRate();
#else
	kfcn = w;
#endif
	kp   = ((-2.7528*kfcn + 3.0429)*kfcn + 1.718)*kfcn - 0.9984;
	kp1  = kp+1.0;
	kp1h = 0.5*kp1;
#ifdef LB_24_RES_TRICK
	k = exp(-w/vcf_rescoeff);
	kres = (((k))) * (((-2.7079*kp1 + 10.963)*kp1 - 14.934)*kp1 + 8.4974);
#else
	kres = (((fs->reso))) * (((-2.7079*kp1 + 10.963)*kp1 - 14.934)*kp1 + 8.4974);
#endif
	value = 1.0+( (fs->dist) *(1.5 + 2.0*kres*(1.0-kfcn))); // ENVMOD was DIST
}


float lb302Filter3Pole::process(const float& samp)
{
	float ax1  = lastin;
	float ay11 = ay1;
	float ay31 = ay2;
	lastin  = (samp) - tanh(kres*aout);
	ay1     = kp1h * (lastin+ax1) - kp*ay1;
	ay2     = kp1h * (ay1 + ay11) - kp*ay2;
	aout    = kp1h * (ay2 + ay31) - kp*aout;

	return tanh(aout*value)*LB_24_VOL_ADJUST/(1.0+fs->dist);
}


//
// LBSynth
//

lb302Synth::lb302Synth( InstrumentTrack * _instrumentTrack ) :
	Instrument( _instrumentTrack, &lb302_plugin_descriptor ),
	vcf_cut_knob( 0.75f, 0.0f, 1.5f, 0.005f, this, tr( "VCF Cutoff Frequency" ) ),
	vcf_res_knob( 0.75f, 0.0f, 1.25f, 0.005f, this, tr( "VCF Resonance" ) ),
	vcf_mod_knob( 0.1f, 0.0f, 1.0f, 0.005f, this, tr( "VCF Envelope Mod" ) ),
	vcf_dec_knob( 0.1f, 0.0f, 1.0f, 0.005f, this, tr( "VCF Envelope Decay" ) ),
	dist_knob( 0.0f, 0.0f, 1.0f, 0.01f, this, tr( "Distortion" ) ),
	wave_shape( 8.0f, 0.0f, 11.0f, this, tr( "Waveform" ) ),
	slide_dec_knob( 0.6f, 0.0f, 1.0f, 0.005f, this, tr( "Slide Decay" ) ),
	slideToggle( false, this, tr( "Slide" ) ),
	accentToggle( false, this, tr( "Accent" ) ),
	deadToggle( false, this, tr( "Dead" ) ),
	db24Toggle( false, this, tr( "24dB/oct Filter" ) ),
	vca_attack(1.0 - 0.96406088),
	vca_decay(0.99897516),
	vca_a0(0.5),
	vca_a(0.),
	vca_mode(never_played)
{

	connect( Engine::mixer(), SIGNAL( sampleRateChanged( ) ),
	         this, SLOT ( filterChanged( ) ) );

	connect( &vcf_cut_knob, SIGNAL( dataChanged( ) ),
	         this, SLOT ( filterChanged( ) ) );

	connect( &vcf_res_knob, SIGNAL( dataChanged( ) ),
	         this, SLOT ( filterChanged( ) ) );

	connect( &vcf_mod_knob, SIGNAL( dataChanged( ) ),
	         this, SLOT ( filterChanged( ) ) );

	connect( &vcf_dec_knob, SIGNAL( dataChanged( ) ),
	         this, SLOT ( filterChanged( ) ) );

	connect( &db24Toggle, SIGNAL( dataChanged( ) ),
	         this, SLOT ( db24Toggled( ) ) );

	connect( &dist_knob, SIGNAL( dataChanged( ) ),
	         this, SLOT ( filterChanged( )));


	// SYNTH

	vco_inc = 0.0;
	vco_c = 0;
	vco_k = 0;

	vco_slide = 0; vco_slideinc = 0;
	vco_slidebase = 0;

	fs.cutoff = 0;
	fs.envmod = 0;
	fs.reso = 0;
	fs.envdecay = 0;
	fs.dist = 0;

	vcf_envpos = ENVINC;

	vco_shape = BL_SAWTOOTH;

	vcfs[0] = new lb302FilterIIR2(&fs);
	vcfs[1] = new lb302Filter3Pole(&fs);
	db24Toggled();

	sample_cnt = 0;
	release_frame = 0;
	catch_frame = 0;
	catch_decay = 0;

	last_offset = 0;

	new_freq = false;

	filterChanged();

	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this, _instrumentTrack );
	Engine::mixer()->addPlayHandle( iph );
}


lb302Synth::~lb302Synth()
{
	for (int i=0; i<NUM_FILTERS; ++i) {
		delete vcfs[i];
	}
}


void lb302Synth::saveSettings( QDomDocument & _doc,
	                             QDomElement & _this )
{
	vcf_cut_knob.saveSettings( _doc, _this, "vcf_cut" );
	vcf_res_knob.saveSettings( _doc, _this, "vcf_res" );
	vcf_mod_knob.saveSettings( _doc, _this, "vcf_mod" );
	vcf_dec_knob.saveSettings( _doc, _this, "vcf_dec" );

	wave_shape.saveSettings( _doc, _this, "shape");
	dist_knob.saveSettings( _doc, _this, "dist");
	slide_dec_knob.saveSettings( _doc, _this, "slide_dec");

	slideToggle.saveSettings( _doc, _this, "slide");
	deadToggle.saveSettings( _doc, _this, "dead");
	db24Toggle.saveSettings( _doc, _this, "db24");
}


void lb302Synth::loadSettings( const QDomElement & _this )
{
	vcf_cut_knob.loadSettings( _this, "vcf_cut" );
	vcf_res_knob.loadSettings( _this, "vcf_res" );
	vcf_mod_knob.loadSettings( _this, "vcf_mod" );
	vcf_dec_knob.loadSettings( _this, "vcf_dec" );

	dist_knob.loadSettings( _this, "dist");
	slide_dec_knob.loadSettings( _this, "slide_dec");
	wave_shape.loadSettings( _this, "shape");
	slideToggle.loadSettings( _this, "slide");
	deadToggle.loadSettings( _this, "dead");
	db24Toggle.loadSettings( _this, "db24");
 	db24Toggled();

	filterChanged();
}

// TODO: Split into one function per knob.  envdecay doesn't require
// recalcFilter.
void lb302Synth::filterChanged()
{
	fs.cutoff = vcf_cut_knob.value();
	fs.reso   = vcf_res_knob.value();
	fs.envmod = vcf_mod_knob.value();
	fs.dist   = LB_DIST_RATIO*dist_knob.value();

	float d = 0.2 + (2.3*vcf_dec_knob.value());

	d *= Engine::mixer()->processingSampleRate();                                // d *= smpl rate
	fs.envdecay = pow(0.1, 1.0/d * ENVINC);    // decay is 0.1 to the 1/d * ENVINC
	                                           // vcf_envdecay is now adjusted for both
	                                           // sampling rate and ENVINC
	recalcFilter();
}


void lb302Synth::db24Toggled()
{
	vcf = vcfs[db24Toggle.value()];
	// These recalcFilter calls might suck
	recalcFilter();
}



QString lb302Synth::nodeName() const
{
	return( lb302_plugin_descriptor.name );
}


// OBSOLETE. Break apart once we get Q_OBJECT to work. >:[
void lb302Synth::recalcFilter()
{
	vcf.load()->recalc();

	// THIS IS OLD 3pole/24dB code, I may reintegrate it.  Don't need it
	// right now.   Should be toggled by LB_24_RES_TRICK at the moment.

	/*kfcn = 2.0 * (((vcf_cutoff*3000))) / engine::mixer()->processingSampleRate();
	kp   = ((-2.7528*kfcn + 3.0429)*kfcn + 1.718)*kfcn - 0.9984;
	kp1  = kp+1.0;
	kp1h = 0.5*kp1;
	kres = (((vcf_reso))) * (((-2.7079*kp1 + 10.963)*kp1 - 14.934)*kp1 + 8.4974);
	value = 1.0+( (((0))) *(1.5 + 2.0*kres*(1.0-kfcn))); // ENVMOD was DIST*/

	vcf_envpos = ENVINC; // Trigger filter update in process()
}

inline float GET_INC(float freq) {
	return freq/Engine::mixer()->processingSampleRate();  // TODO: Use actual sampling rate.
}

int lb302Synth::process(sampleFrame *outbuf, const int size)
{
	const float sampleRatio = 44100.f / Engine::mixer()->processingSampleRate();
	float w;
	float samp;

	// Hold on to the current VCF, and use it throughout this period
	lb302Filter *filter = vcf.loadAcquire();

	if( release_frame == 0 || ! m_playingNote ) 
	{
		vca_mode = decay;
	}

	if( new_freq ) 
	{
		//printf("  playing new note..\n");
		lb302Note note;
		note.vco_inc = GET_INC( true_freq );
		note.dead = deadToggle.value();
		initNote(&note);

		new_freq = false;
	}



	// TODO: NORMAL RELEASE
	// vca_mode = 1;

	for( int i=0; i<size; i++ ) 
	{
		// start decay if we're past release
		if( i >= release_frame )
		{
			vca_mode = decay;
		}

		// update vcf
		if(vcf_envpos >= ENVINC) {
			filter->envRecalc();

			vcf_envpos = 0;

			if (vco_slide) {
					vco_inc = vco_slidebase - vco_slide;
					// Calculate coeff from dec_knob on knob change.
					vco_slide -= vco_slide * ( 0.1f - slide_dec_knob.value() * 0.0999f ) * sampleRatio; // TODO: Adjust for ENVINC

			}
		}


		sample_cnt++;
		vcf_envpos++;

		//int  decay_frames = 128;

		// update vco
		vco_c += vco_inc;
		
		if(vco_c > 0.5)
			vco_c -= 1.0;

		switch(int(rint(wave_shape.value()))) {
			case 0: vco_shape = SAWTOOTH; break;
			case 1: vco_shape = TRIANGLE; break;
			case 2: vco_shape = SQUARE; break;
			case 3: vco_shape = ROUND_SQUARE; break;
			case 4: vco_shape = MOOG; break;
			case 5: vco_shape = SINE; break;
			case 6: vco_shape = EXPONENTIAL; break;
			case 7: vco_shape = WHITE_NOISE; break;
			case 8: vco_shape = BL_SAWTOOTH; break;
			case 9: vco_shape = BL_SQUARE; break;
			case 10: vco_shape = BL_TRIANGLE; break;
			case 11: vco_shape = BL_MOOG; break;
			default:  vco_shape = SAWTOOTH; break;
		}

		// add vco_shape_param the changes the shape of each curve.
		// merge sawtooths with triangle and square with round square?
		switch (vco_shape) {
			case SAWTOOTH: // p0: curviness of line
				vco_k = vco_c;  // Is this sawtooth backwards?
				break;

			case TRIANGLE:  // p0: duty rev.saw<->triangle<->saw p1: curviness
				vco_k = (vco_c*2.0)+0.5;
				if (vco_k>0.5)
					vco_k = 1.0- vco_k;
				break;

			case SQUARE: // p0: slope of top
				vco_k = (vco_c<0)?0.5:-0.5;
				break;

			case ROUND_SQUARE: // p0: width of round
				vco_k = (vco_c<0)?(sqrtf(1-(vco_c*vco_c*4))-0.5):-0.5;
				break;

			case MOOG: // Maybe the fall should be exponential/sinsoidal instead of quadric.
				// [-0.5, 0]: Rise, [0,0.25]: Slope down, [0.25,0.5]: Low
				vco_k = (vco_c*2.0)+0.5;
				if (vco_k>1.0) {
					vco_k = -0.5 ;
				}
				else if (vco_k>0.5) {
					w = 2.0*(vco_k-0.5)-1.0;
					vco_k = 0.5 - sqrtf(1.0-(w*w));
				}
				vco_k *= 2.0;  // MOOG wave gets filtered away
				break;

			case SINE:
				// [-0.5, 0.5]  : [-pi, pi]
				vco_k = 0.5f * Oscillator::sinSample( vco_c );
				break;

			case EXPONENTIAL:
				vco_k = 0.5 * Oscillator::expSample( vco_c );
				break;

			case WHITE_NOISE:
				vco_k = 0.5 * Oscillator::noiseSample( vco_c );
				break;

			case BL_SAWTOOTH:
				vco_k = BandLimitedWave::oscillate( vco_c + 0.5f, BandLimitedWave::pdToLen( vco_inc ), BandLimitedWave::BLSaw ) * 0.5f;
				break;

			case BL_SQUARE:
				vco_k = BandLimitedWave::oscillate( vco_c + 0.5f, BandLimitedWave::pdToLen( vco_inc ), BandLimitedWave::BLSquare ) * 0.5f;
				break;

			case BL_TRIANGLE:
				vco_k = BandLimitedWave::oscillate( vco_c + 0.5f, BandLimitedWave::pdToLen( vco_inc ), BandLimitedWave::BLTriangle ) * 0.5f;
				break;

			case BL_MOOG:
				vco_k = BandLimitedWave::oscillate( vco_c + 0.5f, BandLimitedWave::pdToLen( vco_inc ), BandLimitedWave::BLMoog );
				break;
		}

		//vca_a = 0.5;
		// Write out samples.
#ifdef LB_FILTERED
		//samp = vcf->process(vco_k)*2.0*vca_a;
		//samp = vcf->process(vco_k)*2.0;
		samp = filter->process(vco_k) * vca_a;
		//printf("%f %d\n", vco_c, sample_cnt);


		//samp = vco_k * vca_a;

		if( sample_cnt <= 4 )
		{
	//			vca_a = 0;
		}

#else
		//samp = vco_k*vca_a;
#endif
		/*
		float releaseFrames = desiredReleaseFrames();
		samp *= (releaseFrames - catch_decay)/releaseFrames;
		*/
		//LB302 samp *= (float)(decay_frames - catch_decay)/(float)decay_frames;

		for( int c = 0; c < DEFAULT_CHANNELS; c++ ) 
		{
			outbuf[i][c] = samp;
		}

		// Handle Envelope
		if(vca_mode==attack) {
			vca_a+=(vca_a0-vca_a)*vca_attack;
			if(sample_cnt>=0.5*Engine::mixer()->processingSampleRate())
				vca_mode = idle;
		}
		else if(vca_mode == decay) {
			vca_a *= vca_decay;

			// the following line actually speeds up processing
			if(vca_a < (1/65536.0)) {
				vca_a = 0;
				vca_mode = never_played;
			}
		}

	}
	return 1;
}


/*  Prepares the active LB302 note.  I separated this into a function because it
 *  needs to be called onplayNote() when a new note is started.  It also needs
 *  to be called from process() when a prior edge-to-edge note is done releasing.
 */

void lb302Synth::initNote( lb302Note *n)
{
	catch_decay = 0;

	vco_inc = n->vco_inc;

	// Always reset vca on non-dead notes, and
	// Only reset vca on decaying(decayed) and never-played
	if(n->dead == 0 || (vca_mode == decay || vca_mode == never_played)) {
		//printf("    good\n");
		sample_cnt = 0;
		vca_mode = attack;
		// LB303:
		//vca_a = 0;
	}
	else {
		vca_mode = idle;
	}

	initSlide();

	// Slide-from note, save inc for next note
	if (slideToggle.value()) {
		vco_slideinc = vco_inc; // May need to equal vco_slidebase+vco_slide if last note slid
	}


	recalcFilter();

	if(n->dead ==0){
		// Swap next two blocks??
		vcf.load()->playNote();
		// Ensure envelope is recalculated
		vcf_envpos = ENVINC;

		// Double Check
		//vca_mode = 0;
		//vca_a = 0.0;
	}
}


void lb302Synth::initSlide()
{
	// Initiate Slide
	if (vco_slideinc) {
		//printf("    sliding\n");
		vco_slide = vco_inc-vco_slideinc;	// Slide amount
		vco_slidebase = vco_inc;			// The REAL frequency
		vco_slideinc = 0;					// reset from-note
	}
	else {
		vco_slide = 0;
	}
}


void lb302Synth::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer )
{
	if( _n->isMasterNote() || ( _n->hasParent() && _n->isReleased() ) )
	{
		return;
	}

	// sort notes: new notes to the end
	m_notesMutex.lock();
	if( _n->totalFramesPlayed() == 0 )
	{
		m_notes.append( _n );
	}
	else
	{
		m_notes.prepend( _n );
	}
	m_notesMutex.unlock();
	
	release_frame = qMax( release_frame, _n->framesLeft() + _n->offset() );
}



void lb302Synth::processNote( NotePlayHandle * _n )
{
		/// Start a new note.
		if( _n->m_pluginData != this ) 
		{
			m_playingNote = _n;
			new_freq = true;
			_n->m_pluginData = this;
		}
		
		if( ! m_playingNote && ! _n->isReleased() && release_frame > 0 )
		{
			m_playingNote = _n;
			if ( slideToggle.value() ) 
			{
				vco_slideinc = GET_INC( _n->frequency() );
			}
		}

		// Check for slide
		if( m_playingNote == _n ) 
		{
			true_freq = _n->frequency();

			if( slideToggle.value() ) {
				vco_slidebase = GET_INC( true_freq );			// The REAL frequency
			}
			else {
				vco_inc = GET_INC( true_freq );
			}
		}
}



void lb302Synth::play( sampleFrame * _working_buffer )
{
	m_notesMutex.lock();
	while( ! m_notes.isEmpty() )
	{
		processNote( m_notes.takeFirst() );
	};
	m_notesMutex.unlock();
	
	const fpp_t frames = Engine::mixer()->framesPerPeriod();

	process( _working_buffer, frames );
	instrumentTrack()->processAudioBuffer( _working_buffer, frames, NULL );
//	release_frame = 0; //removed for issue # 1432
}



void lb302Synth::deleteNotePluginData( NotePlayHandle * _n )
{
	//printf("GONE\n");
	if( m_playingNote == _n )
	{
		m_playingNote = NULL;
	}
}


PluginView * lb302Synth::instantiateView( QWidget * _parent )
{
	return( new lb302SynthView( this, _parent ) );
}


lb302SynthView::lb302SynthView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	// GUI
	m_vcfCutKnob = new Knob( knobBright_26, this );
	m_vcfCutKnob->move( 75, 130 );
	m_vcfCutKnob->setHintText( tr( "Cutoff Freq:" ), "" );
	m_vcfCutKnob->setLabel( "" );

	m_vcfResKnob = new Knob( knobBright_26, this );
	m_vcfResKnob->move( 120, 130 );
	m_vcfResKnob->setHintText( tr( "Resonance:" ), "" );
	m_vcfResKnob->setLabel( "" );

	m_vcfModKnob = new Knob( knobBright_26, this );
	m_vcfModKnob->move( 165, 130 );
	m_vcfModKnob->setHintText( tr( "Env Mod:" ), "" );
	m_vcfModKnob->setLabel( "" );

	m_vcfDecKnob = new Knob( knobBright_26, this );
	m_vcfDecKnob->move( 210, 130 );
	m_vcfDecKnob->setHintText( tr( "Decay:" ), "" );
	m_vcfDecKnob->setLabel( "" );

	m_slideToggle = new LedCheckBox( "", this );
	m_slideToggle->move( 10, 180 );

/*	m_accentToggle = new LedCheckBox( "", this );
	m_accentToggle->move( 10, 200 );
	m_accentToggle->setDisabled(true);*/ // accent removed pending real implementation - no need for non-functional buttons

	m_deadToggle = new LedCheckBox( "", this );
	m_deadToggle->move( 10, 200 );

	m_db24Toggle = new LedCheckBox( "", this );
	m_db24Toggle->move( 10, 150);
	ToolTip::add( m_db24Toggle,
			tr( "303-es-que, 24dB/octave, 3 pole filter" ) );


	m_slideDecKnob = new Knob( knobBright_26, this );
	m_slideDecKnob->move( 210, 75 );
	m_slideDecKnob->setHintText( tr( "Slide Decay:" ), "" );
	m_slideDecKnob->setLabel( "");

	m_distKnob = new Knob( knobBright_26, this );
	m_distKnob->move( 210, 190 );
	m_distKnob->setHintText( tr( "DIST:" ), "" );
	m_distKnob->setLabel( tr( ""));


	// Shapes
	// move to 120,75
	const int waveBtnX = 10;
	const int waveBtnY = 96;
	PixmapButton * sawWaveBtn = new PixmapButton( this, tr( "Saw wave" ) );
	sawWaveBtn->move( waveBtnX, waveBtnY );
	sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"saw_wave_active" ) );
	sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"saw_wave_inactive" ) );
	ToolTip::add( sawWaveBtn,
			tr( "Click here for a saw-wave." ) );

	PixmapButton * triangleWaveBtn =
		new PixmapButton( this, tr( "Triangle wave" ) );
	triangleWaveBtn->move( waveBtnX+(16*1), waveBtnY );
	triangleWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	triangleWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	ToolTip::add( triangleWaveBtn,
			tr( "Click here for a triangle-wave." ) );

	PixmapButton * sqrWaveBtn = new PixmapButton( this, tr( "Square wave" ) );
	sqrWaveBtn->move( waveBtnX+(16*2), waveBtnY );
	sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
					"square_wave_active" ) );
	sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
					"square_wave_inactive" ) );
	ToolTip::add( sqrWaveBtn,
			tr( "Click here for a square-wave." ) );

	PixmapButton * roundSqrWaveBtn =
		new PixmapButton( this, tr( "Rounded square wave" ) );
	roundSqrWaveBtn->move( waveBtnX+(16*3), waveBtnY );
	roundSqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
					"round_square_wave_active" ) );
	roundSqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
					"round_square_wave_inactive" ) );
	ToolTip::add( roundSqrWaveBtn,
			tr( "Click here for a square-wave with a rounded end." ) );

	PixmapButton * moogWaveBtn =
		new PixmapButton( this, tr( "Moog wave" ) );
	moogWaveBtn->move( waveBtnX+(16*4), waveBtnY );
	moogWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_active" ) );
	moogWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_inactive" ) );
	ToolTip::add( moogWaveBtn,
			tr( "Click here for a moog-like wave." ) );

	PixmapButton * sinWaveBtn = new PixmapButton( this, tr( "Sine wave" ) );
	sinWaveBtn->move( waveBtnX+(16*5), waveBtnY );
	sinWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"sin_wave_active" ) );
	sinWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"sin_wave_inactive" ) );
	ToolTip::add( sinWaveBtn,
			tr( "Click for a sine-wave." ) );

	PixmapButton * exponentialWaveBtn =
		new PixmapButton( this, tr( "White noise wave" ) );
	exponentialWaveBtn->move( waveBtnX+(16*6), waveBtnY );
	exponentialWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "exp_wave_active" ) );
	exponentialWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "exp_wave_inactive" ) );
	ToolTip::add( exponentialWaveBtn,
			tr( "Click here for an exponential wave." ) );


	PixmapButton * whiteNoiseWaveBtn =
		new PixmapButton( this, tr( "White noise wave" ) );
	whiteNoiseWaveBtn->move( waveBtnX+(16*7), waveBtnY );
	whiteNoiseWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "white_noise_wave_active" ) );
	whiteNoiseWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "white_noise_wave_inactive" ) );
	ToolTip::add( whiteNoiseWaveBtn,
			tr( "Click here for white-noise." ) );

	PixmapButton * blSawWaveBtn =
		new PixmapButton( this, tr( "Bandlimited saw wave" ) );
	blSawWaveBtn->move( waveBtnX+(16*9)-8, waveBtnY );
	blSawWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "saw_wave_active" ) );
	blSawWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "saw_wave_inactive" ) );
	ToolTip::add( blSawWaveBtn,
			tr( "Click here for bandlimited saw wave." ) );

	PixmapButton * blSquareWaveBtn =
		new PixmapButton( this, tr( "Bandlimited square wave" ) );
	blSquareWaveBtn->move( waveBtnX+(16*10)-8, waveBtnY );
	blSquareWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "square_wave_active" ) );
	blSquareWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "square_wave_inactive" ) );
	ToolTip::add( blSquareWaveBtn,
			tr( "Click here for bandlimited square wave." ) );

	PixmapButton * blTriangleWaveBtn =
		new PixmapButton( this, tr( "Bandlimited triangle wave" ) );
	blTriangleWaveBtn->move( waveBtnX+(16*11)-8, waveBtnY );
	blTriangleWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	blTriangleWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	ToolTip::add( blTriangleWaveBtn,
			tr( "Click here for bandlimited triangle wave." ) );

	PixmapButton * blMoogWaveBtn =
		new PixmapButton( this, tr( "Bandlimited moog saw wave" ) );
	blMoogWaveBtn->move( waveBtnX+(16*12)-8, waveBtnY );
	blMoogWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_active" ) );
	blMoogWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "moog_saw_wave_inactive" ) );
	ToolTip::add( blMoogWaveBtn,
			tr( "Click here for bandlimited moog saw wave." ) );


	m_waveBtnGrp = new automatableButtonGroup( this );
	m_waveBtnGrp->addButton( sawWaveBtn );
	m_waveBtnGrp->addButton( triangleWaveBtn );
	m_waveBtnGrp->addButton( sqrWaveBtn );
	m_waveBtnGrp->addButton( roundSqrWaveBtn );
	m_waveBtnGrp->addButton( moogWaveBtn );
	m_waveBtnGrp->addButton( sinWaveBtn );
	m_waveBtnGrp->addButton( exponentialWaveBtn );
	m_waveBtnGrp->addButton( whiteNoiseWaveBtn );
	m_waveBtnGrp->addButton( blSawWaveBtn );
	m_waveBtnGrp->addButton( blSquareWaveBtn );
	m_waveBtnGrp->addButton( blTriangleWaveBtn );
	m_waveBtnGrp->addButton( blMoogWaveBtn );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
			"artwork" ) );
	setPalette( pal );
}


lb302SynthView::~lb302SynthView()
{
}


void lb302SynthView::modelChanged()
{
	lb302Synth * syn = castModel<lb302Synth>();

	m_vcfCutKnob->setModel( &syn->vcf_cut_knob );
	m_vcfResKnob->setModel( &syn->vcf_res_knob );
	m_vcfDecKnob->setModel( &syn->vcf_dec_knob );
	m_vcfModKnob->setModel( &syn->vcf_mod_knob );
	m_slideDecKnob->setModel( &syn->slide_dec_knob );

	m_distKnob->setModel( &syn->dist_knob );
	m_waveBtnGrp->setModel( &syn->wave_shape );

	m_slideToggle->setModel( &syn->slideToggle );
	/*m_accentToggle->setModel( &syn->accentToggle );*/
	m_deadToggle->setModel( &syn->deadToggle );
	m_db24Toggle->setModel( &syn->db24Toggle );
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * m, void * )
{

	return( new lb302Synth(
		static_cast<InstrumentTrack *>( m ) ) );
}


}


