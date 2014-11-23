/*
 * lb303.cpp - implementation of class lb303 which is a bass synth attempting 
 *             to emulate the Roland TB303 bass synth
 *
 * Copyright (c) 2006-2008 Paul Giblock <pgib/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
 *
 * lb303FilterIIR2 is based on the gsyn filter code by Andy Sloane.
 * 
 * lb303Filter3Pole is based on the TB303 instrument written by 
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


#include <QtXml/QDomDocument>

#include "lb303.h"
#include "engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "knob.h"
#include "NotePlayHandle.h"
#include "templates.h"
#include "audio_port.h"

#include "embed.cpp"
#include "moc_lb303.cxx"


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

Plugin::Descriptor PLUGIN_EXPORT lb303_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"LB303",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Incomplete monophonic imitation tb303" ),
	"Paul Giblock <pgib/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL
};

}

//
// lb303Filter
//

lb303Filter::lb303Filter(lb303FilterKnobState* p_fs) :
	fs(p_fs),
	vcf_c0(0),
	vcf_e0(0),
	vcf_e1(0)
{
};


void lb303Filter::recalc()
{
	vcf_e1 = exp(6.109 + 1.5876*(fs->envmod) + 2.1553*(fs->cutoff) - 1.2*(1.0-(fs->reso)));
	vcf_e0 = exp(5.613 - 0.8*(fs->envmod) + 2.1553*(fs->cutoff) - 0.7696*(1.0-(fs->reso)));
	vcf_e0*=M_PI/engine::mixer()->processingSampleRate();
	vcf_e1*=M_PI/engine::mixer()->processingSampleRate();
	vcf_e1 -= vcf_e0;

	vcf_rescoeff = exp(-1.20 + 3.455*(fs->reso));
};


void lb303Filter::envRecalc()
{
	vcf_c0 *= fs->envdecay;       // Filter Decay. vcf_decay is adjusted for Hz and ENVINC
	// vcf_rescoeff = exp(-1.20 + 3.455*(fs->reso)); moved above
};


void lb303Filter::playNote()
{
	vcf_c0 = vcf_e1;
}


//
// lb303FilterIIR2
//

lb303FilterIIR2::lb303FilterIIR2(lb303FilterKnobState* p_fs) :
	lb303Filter(p_fs),
	vcf_d1(0),
	vcf_d2(0),
	vcf_a(0),
	vcf_b(0),
	vcf_c(1)
{

	m_dist = new effectLib::distortion<>( 1.0, 1.0f);
	
};


lb303FilterIIR2::~lb303FilterIIR2()
{
	delete m_dist;
}


void lb303FilterIIR2::recalc()
{
	lb303Filter::recalc();
	//m_dist->setThreshold(0.5+(fs->dist*2.0));
	m_dist->setThreshold(fs->dist*75.0);
};


void lb303FilterIIR2::envRecalc()
{
	float k, w;

	lb303Filter::envRecalc();

	w = vcf_e0 + vcf_c0;          // e0 is adjusted for Hz and doesn't need ENVINC
	k = exp(-w/vcf_rescoeff);     // Does this mean c0 is inheritantly?

	vcf_a = 2.0*cos(2.0*w) * k;
	vcf_b = -k*k;
	vcf_c = 1.0 - vcf_a - vcf_b;
}


float lb303FilterIIR2::process(const float& samp)
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
// lb303Filter3Pole
//

lb303Filter3Pole::lb303Filter3Pole(lb303FilterKnobState *p_fs) :
	lb303Filter(p_fs),
	ay1(0),
	ay2(0),
	aout(0),
	lastin(0) 
{
};


void lb303Filter3Pole::recalc()
{
	// DO NOT CALL BASE CLASS
	vcf_e0 = 0.000001;
	vcf_e1 = 1.0;
}


// TODO: Try using k instead of vcf_reso
void lb303Filter3Pole::envRecalc()
{
	float w,k;
	float kfco;

	lb303Filter::envRecalc();

	// e0 is adjusted for Hz and doesn't need ENVINC
	w = vcf_e0 + vcf_c0;
	k = (fs->cutoff > 0.975)?0.975:fs->cutoff;
	kfco = 50.f + (k)*((2300.f-1600.f*(fs->envmod))+(w) *
	                   (700.f+1500.f*(k)+(1500.f+(k)*(engine::mixer()->processingSampleRate()/2.f-6000.f)) * 
	                   (fs->envmod)) );
	//+iacc*(.3+.7*kfco*kenvmod)*kaccent*kaccurve*2000


#ifdef LB_24_IGNORE_ENVELOPE
	// kfcn = fs->cutoff;
	kfcn = 2.0 * kfco / engine::mixer()->processingSampleRate();
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


float lb303Filter3Pole::process(const float& samp) 
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

lb303Synth::lb303Synth( InstrumentTrack * _InstrumentTrack ) :
	instrument( _InstrumentTrack, &lb303_plugin_descriptor ),
	vcf_cut_knob( 0.75f, 0.0f, 1.5f, 0.005f, this, tr( "VCF Cutoff Frequency" ) ),
	vcf_res_knob( 0.75f, 0.0f, 1.25f, 0.005f, this, tr( "VCF Resonance" ) ),
	vcf_mod_knob( 0.1f, 0.0f, 1.0f, 0.005f, this, tr( "VCF Envelope Mod" ) ),
	vcf_dec_knob( 0.1f, 0.0f, 1.0f, 0.005f, this, tr( "VCF Envelope Decay" ) ),
	dist_knob( 0.0f, 0.0f, 1.0f, 0.01f, this, tr( "Distortion" ) ),
	wave_knob( 0.0f, 0.0f, 5.0f, 1.0f, this, tr( "Waveform" ) ),
	slide_dec_knob( 0.6f, 0.0f, 1.0f, 0.005f, this, tr( "Slide Decay" ) ),
	slideToggle( false, this, tr( "Slide" ) ),
	accentToggle( false, this, tr( "Accent" ) ),
	deadToggle( false, this, tr( "Dead" ) ),
	db24Toggle( false, this, tr( "24dB/oct Filter" ) )	

{

	connect( engine::mixer(), SIGNAL( sampleRateChanged( ) ),
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

	// Start VCA on an attack.
	vca_mode = 3;
	vca_a = 0;

	//vca_attack = 1.0 - 0.94406088;
	vca_attack = 1.0 - 0.96406088;
	vca_decay = 0.99897516;

	vco_shape = SAWTOOTH; 

	// Experimenting with a0 between original (0.5) and 1.0
	vca_a0 = 0.5;
	vca_a = 9;
	vca_mode = 3;

	vcf = new lb303FilterIIR2(&fs);

	sample_cnt = 0;
	release_frame = 1<<24;
	catch_frame = 0;
	catch_decay = 0;

	recalcFilter();

	last_offset = 0;

	new_freq = -1;
	current_freq = -1;
	delete_freq = -1;

	instrumentPlayHandle * iph = new instrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( iph );

	filterChanged();
}


lb303Synth::~lb303Synth()
{
	delete vcf;
}


void lb303Synth::saveSettings( QDomDocument & _doc,
	                             QDomElement & _this )
{
	vcf_cut_knob.saveSettings( _doc, _this, "vcf_cut" );
	vcf_res_knob.saveSettings( _doc, _this, "vcf_res" );
	vcf_mod_knob.saveSettings( _doc, _this, "vcf_mod" );
	vcf_dec_knob.saveSettings( _doc, _this, "vcf_dec" );

	wave_knob.saveSettings( _doc, _this, "shape");
	dist_knob.saveSettings( _doc, _this, "dist");
	slide_dec_knob.saveSettings( _doc, _this, "slide_dec");

	slideToggle.saveSettings( _doc, _this, "slide");
	deadToggle.saveSettings( _doc, _this, "dead");
	db24Toggle.saveSettings( _doc, _this, "db24");
}


void lb303Synth::loadSettings( const QDomElement & _this )
{
	vcf_cut_knob.loadSettings( _this, "vcf_cut" );
	vcf_res_knob.loadSettings( _this, "vcf_res" );
	vcf_mod_knob.loadSettings( _this, "vcf_mod" );
	vcf_dec_knob.loadSettings( _this, "vcf_dec" );

	dist_knob.loadSettings( _this, "dist");
	wave_knob.loadSettings( _this, "shape");
	slide_dec_knob.loadSettings( _this, "slide_dec");

	slideToggle.loadSettings( _this, "slide");
	deadToggle.loadSettings( _this, "dead");
	db24Toggle.loadSettings( _this, "db24");

	filterChanged();
}

// TODO: Split into one function per knob.  envdecay doesn't require
// recalcFilter.
void lb303Synth::filterChanged()
{
	fs.cutoff = vcf_cut_knob.value();
	fs.reso   = vcf_res_knob.value();
	fs.envmod = vcf_mod_knob.value();
	fs.dist   = LB_DIST_RATIO*dist_knob.value();

	float d = 0.2 + (2.3*vcf_dec_knob.value());

	d *= engine::mixer()->processingSampleRate();                                // d *= smpl rate
	fs.envdecay = pow(0.1, 1.0/d * ENVINC);    // decay is 0.1 to the 1/d * ENVINC
	                                           // vcf_envdecay is now adjusted for both
	                                           // sampling rate and ENVINC
	recalcFilter();
}


void lb303Synth::db24Toggled()
{
	delete vcf;
	if(db24Toggle.value()) {
		vcf = new lb303Filter3Pole(&fs);
	}
	else {
		vcf = new lb303FilterIIR2(&fs);
	}
	recalcFilter();
}



QString lb303Synth::nodeName() const
{
	return( lb303_plugin_descriptor.name );
}


void lb303Synth::recalcFilter()
{
	vcf->recalc();

	vcf_envpos = ENVINC; // Trigger filter update in process()
}

inline int MIN(int a, int b) {
	return (a<b)?a:b;
}

inline float GET_INC(float freq) {
	return freq/engine::mixer()->processingSampleRate();  // TODO: Use actual sampling rate.
}

int lb303Synth::process(sampleFrame *outbuf, const Uint32 size)
{
	unsigned int i;
	float w;
	float samp;

	if( delete_freq == current_freq ) {
		// Normal release
		delete_freq = -1;
		vca_mode = 1;
	}

	if( new_freq > 0.0f ) {
		lb303Note note;
		note.vco_inc = GET_INC( true_freq );
		note.dead = deadToggle.value();
		initNote(&note);
		
		current_freq = new_freq; 

		new_freq = -1.0f;
	} 

	

	// TODO: NORMAL RELEASE
	// vca_mode = 1;

	for(i=0;i<size;i++) {

		// update vcf
		if(vcf_envpos >= ENVINC) {
			vcf->envRecalc();

			vcf_envpos = 0;

			if (vco_slide) {
					vco_inc=vco_slidebase-vco_slide;
					// Calculate coeff from dec_knob on knob change.
					vco_slide*= 0.9+(slide_dec_knob.value()*0.0999); // TODO: Adjust for Hz and ENVINC

			}
		}


		sample_cnt++;
		vcf_envpos++;

		int  decay_frames = 128;

		// update vco
		vco_c += vco_inc;

		if(vco_c > 0.5)
			vco_c -= 1.0;

		/*LB303
		if (catch_decay > 0) {
			if (catch_decay < decay_frames) {
				catch_decay++;
			}
		}*/

		switch(int(rint(wave_knob.value()))) {
			case 0: vco_shape = SAWTOOTH; break;
			case 1: vco_shape = INVERTED_SAWTOOTH; break;
			case 2: vco_shape = TRIANGLE; break;
			case 3: vco_shape = SQUARE; break;
			case 4: vco_shape = ROUND_SQUARE; break;
			case 5: vco_shape = MOOG; break;
			default:  vco_shape = SAWTOOTH; break;
		}

		// add vco_shape_param the changes the shape of each curve.
		// merge sawtooths with triangle and square with round square?
		switch (vco_shape) {
			case SAWTOOTH: // p0: curviness of line
				vco_k = vco_c;  // Is this sawtooth backwards?
				break;

			case INVERTED_SAWTOOTH: // p0: curviness of line
				vco_k = -vco_c;  // Is this sawtooth backwards?
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
		}

		// Write out samples.
		samp = vcf->process(vco_k) * vca_a;

		/*
		float releaseFrames = desiredReleaseFrames();
		samp *= (releaseFrames - catch_decay)/releaseFrames;
		*/
		//LB303 samp *= (float)(decay_frames - catch_decay)/(float)decay_frames;

		for(int c=0; c<DEFAULT_CHANNELS; c++) {
			outbuf[i][c]=samp;
		}


		/*LB303
		if((int)i>=release_frame) {
			vca_mode=1;
		}
		*/

		// Handle Envelope
		if(vca_mode==0) {
			vca_a+=(vca_a0-vca_a)*vca_attack;
			if(sample_cnt>=0.5*engine::mixer()->processingSampleRate()) 
				vca_mode = 2;
		}
		else if(vca_mode == 1) {
			vca_a *= vca_decay;

			// the following line actually speeds up processing
			if(vca_a < (1/65536.0)) {
				vca_a = 0;
				vca_mode = 3;
			}
		}

	}
	return 1;
}


/*  Prepares the active LB303 note.  I separated this into a function because it
 *  needs to be called onplayNote() when a new note is started.  It also needs
 *  to be called from process() when a prior edge-to-edge note is done releasing.
 */

void lb303Synth::initNote( lb303Note *n)
{
	catch_decay = 0;

	vco_inc = n->vco_inc;
    
	// Always reset vca on non-dead notes, and
	// Only reset vca on decaying(decayed) and never-played
	if(n->dead == 0 || (vca_mode==1 || vca_mode==3)) {
		//printf("    good\n");
		sample_cnt = 0;
		vca_mode = 0;
		// LB303:
		//vca_a = 0;
	}
	else {
		vca_mode = 2;
	}

	// Initiate Slide
	// TODO: Break out into function, should be called again on detuneChanged
	if (vco_slideinc) {
		vco_slide = vco_inc-vco_slideinc;	// Slide amount
		vco_slidebase = vco_inc;			// The REAL frequency
		vco_slideinc = 0;					// reset from-note
	}
	else {
		vco_slide = 0;
	}
	// End break-out

	// Slide-from note, save inc for next note
	if (slideToggle.value()) {
		vco_slideinc = vco_inc; // May need to equal vco_slidebase+vco_slide if last note slid
	}


	recalcFilter();
	
	if(n->dead ==0){
		// Swap next two blocks??
		vcf->playNote();
		// Ensure envelope is recalculated
		vcf_envpos = ENVINC;

		// Double Check 
		//vca_mode = 0;
		//vca_a = 0.0;
	}
}


void lb303Synth::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if( _n->arpBaseNote() )
	{
		//return;
	}

	// Currently have release/decay disabled
	// Start the release decay if this is the first release period.
	//if (_n->released() && catch_decay == 0)
	//        catch_decay = 1;

	bool decay_note = false;

	release_frame = _n->framesLeft() - desiredReleaseFrames();


	if(deadToggle.value() == 0 && decay_note) {
	}
	/// Start a new note.
	else if( _n->totalFramesPlayed() == 0 ) {
		new_freq = _n->unpitchedFrequency();
		true_freq = _n->frequency();
		_n->m_pluginData = this;
	}

	// Check for slide
	if( _n->unpitchedFrequency() == current_freq ) {
		true_freq = _n->frequency();

		if( slideToggle.value() ) {
			vco_slidebase = GET_INC( true_freq );			// The REAL frequency
		}
		else {
			vco_inc = GET_INC( true_freq );
		}
	}
}



void lb303Synth::play( sampleFrame * _working_buffer )
{
	//printf(".");
	const fpp_t frames = engine::mixer()->framesPerPeriod();

	process( _working_buffer, frames); 
	instrumentTrack()->processAudioBuffer( _working_buffer, frames,
									NULL );
}



void lb303Synth::deleteNotePluginData( NotePlayHandle * _n )
{
	//printf("GONE\n");
	if( _n->unpitchedFrequency() == current_freq ) 
	{
		delete_freq = current_freq;
	}
}


PluginView * lb303Synth::instantiateView( QWidget * _parent )
{
	return( new lb303SynthView( this, _parent ) );
}


lb303SynthView::lb303SynthView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	// GUI
	m_vcfCutKnob = new knob( knobBright_26, this );
	m_vcfCutKnob->move( 75, 130 );
	m_vcfCutKnob->setHintText( tr( "Cutoff Freq:" ) + " ", "" );
	m_vcfCutKnob->setLabel( tr("CUT") );

	m_vcfResKnob = new knob( knobBright_26, this );
	m_vcfResKnob->move( 120, 130 );
	m_vcfResKnob->setHintText( tr( "Resonance:" ) + " ", "" );
	m_vcfResKnob->setLabel( tr("RES") );

	m_vcfModKnob = new knob( knobBright_26, this );
	m_vcfModKnob->move( 165, 130 );
	m_vcfModKnob->setHintText( tr( "Env Mod:" ) + " ", "" );
	m_vcfModKnob->setLabel( tr("ENV MOD") );

	m_vcfDecKnob = new knob( knobBright_26, this );
	m_vcfDecKnob->move( 210, 130 );
	m_vcfDecKnob->setHintText( tr( "Decay:" ) + " ", "" );
	m_vcfDecKnob->setLabel( tr("DEC") );

	m_slideToggle = new ledCheckBox( "Slide", this );
	m_slideToggle->move( 10, 180 );

	m_accentToggle = new ledCheckBox( "Accent", this );
	m_accentToggle->move( 10, 200 );
	m_accentToggle->setDisabled(true);

	m_deadToggle = new ledCheckBox( "Dead", this );
	m_deadToggle->move( 10, 220 );

	m_db24Toggle = new ledCheckBox( "24dB/oct", this );
	m_db24Toggle->setWhatsThis( 
			tr( "303-es-que, 24dB/octave, 3 pole filter" ) );
	m_db24Toggle->move( 10, 150);


	m_slideDecKnob = new knob( knobBright_26, this );
	m_slideDecKnob->move( 210, 75 );
	m_slideDecKnob->setHintText( tr( "Slide Decay:" ) + " ", "" );
	m_slideDecKnob->setLabel( tr( "SLIDE"));

	m_distKnob = new knob( knobBright_26, this );
	m_distKnob->move( 210, 190 );
	m_distKnob->setHintText( tr( "DIST:" ) + " ", "" );
	m_distKnob->setLabel( tr( "DIST"));


	m_waveKnob = new knob( knobBright_26, this );
	m_waveKnob->move( 120, 75 );
	m_waveKnob->setHintText( tr( "WAVE:" ) + " ", "" );
	m_waveKnob->setLabel( tr( "WAVE"));


	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
			"artwork" ) );
	setPalette( pal );
}


lb303SynthView::~lb303SynthView()
{
}


void lb303SynthView::modelChanged()
{
	lb303Synth * syn = castModel<lb303Synth>();
	
	m_vcfCutKnob->setModel( &syn->vcf_cut_knob );
	m_vcfResKnob->setModel( &syn->vcf_res_knob );
	m_vcfDecKnob->setModel( &syn->vcf_dec_knob );
	m_vcfModKnob->setModel( &syn->vcf_mod_knob );
	m_slideDecKnob->setModel( &syn->slide_dec_knob );

	m_distKnob->setModel( &syn->dist_knob );
	m_waveKnob->setModel( &syn->wave_knob );
    
	m_slideToggle->setModel( &syn->slideToggle );
	m_accentToggle->setModel( &syn->accentToggle );
	m_deadToggle->setModel( &syn->deadToggle );
	m_db24Toggle->setModel( &syn->db24Toggle );
}



extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{

	return( new lb303Synth(
	        static_cast<InstrumentTrack *>( _data ) ) );
}


}


