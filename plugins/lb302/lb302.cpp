/*
 * lb302.cpp - implementation of class lb302 which is a bass synth attempting 
 *             to emulate the Roland TB303 bass synth
 *
 * Copyright (c) 2006-2007 Paul Giblock <pgib/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include <Qt/QtXml>

#include "lb302.h"
#include "engine.h"
#include "instrument_play_handle.h"
#include "instrument_track.h"
#include "knob.h"
#include "note_play_handle.h"
#include "templates.h"
#include "audio_port.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"
#include "lb302.moc"


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


#define LB_HZ 44100.0f


extern "C"
{

plugin::descriptor lb302_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"LB302",
	QT_TRANSLATE_NOOP( "pluginBrowser",
	                   "Llamafied tb303" ),
	"Paul Giblock <pgib/at/users.sf.net>",
	0x0100,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
};

}


//
// lb302Filter
//

lb302Filter::lb302Filter( lb302FilterKnobState * _fs ) :
	m_fs( _fs ),
	m_c0( 0 ),
	m_e0( 0 ),
	m_e1( 0 )
{
};



void lb302Filter::recalc( void )
{
	m_e1 = exp( 6.109 + 1.5876*(m_fs->envmod) +
	            2.1553*(m_fs->cutoff) - 1.2*( 1.0 - (m_fs->reso) ) );
	m_e0 = exp( 5.613 - 0.8*(m_fs->envmod) +
	            2.1553*(m_fs->cutoff) - 0.7696*( 1.0 - (m_fs->reso) ) );

	m_e0 *= M_PI / 44100.0;
	m_e1 *= M_PI / 44100.0;
	m_e1 -= m_e0;

	m_rescoeff = exp( -1.20 + 3.455 * (m_fs->reso) );
};


void lb302Filter::envRecalc( void )
{
	m_c0 *= m_fs->envdecay;       // Filter Decay. vcf_decay is adjusted for Hz and ENVINC
	// vcf_rescoeff = exp(-1.20 + 3.455*(fs->reso)); moved above
};


void lb302Filter::playNote( void )
{
	m_c0 = m_e1;
}



//
// lb302FilterIIR2
//

lb302FilterIIR2::lb302FilterIIR2( lb302FilterKnobState * _fs ) :
	lb302Filter( _fs ),
	m_d1( 0 ),
	m_d2( 0 ),
	m_a( 0 ),
	m_b( 0 ),
	m_c( 1 )
{

	m_dist = new effectLib::distortion<>( 1.0, 1.0f );

};


lb302FilterIIR2::~lb302FilterIIR2( void )
{
	delete m_dist;
}



void lb302FilterIIR2::recalc( void )
{
	lb302Filter::recalc();
	//m_dist->setThreshold(0.5+(fs->dist*2.0));
	m_dist->setThreshold( m_fs->dist * 75.0 );
};


void lb302FilterIIR2::envRecalc( void )
{
	float k, w;

	lb302Filter::envRecalc();

	w = m_e0 + m_c0;              // e0 is adjusted for Hz and doesn't need ENVINC
	k = exp( -w / m_rescoeff );   // Does this mean c0 is inheritantly?

	m_a = 2.0 * cos( 2.0 * w ) * k;
	m_b = -k * k;
	m_c = 1.0 - m_a - m_b;
}


float lb302FilterIIR2::process( const float & _samp )
{
	float ret = ( m_a * m_d1 ) +
	            ( m_b * m_d2 ) +
	            ( m_c * _samp );

	// Delayed samples for filter
	m_d2 = m_d1;
	m_d1 = ret;

	if( m_fs->dist > 0 )
		ret = m_dist->nextSample( ret );

	// output = IIR2 + dry
	return ret;
}


void lb302FilterIIR2::getState( lb302FilterState * _fs )
{
	_fs->iir.c0 = m_c0;
	_fs->iir.a  = m_a;
	_fs->iir.b  = m_b;
	_fs->iir.c  = m_c;
	_fs->iir.d1 = m_d1;
	_fs->iir.d2 = m_d2;
}


void lb302FilterIIR2::setState( const lb302FilterState * _fs )
{
	m_c0 = _fs->iir.c0;
	m_a  = _fs->iir.a;
	m_b  = _fs->iir.b;
	m_c  = _fs->iir.c;
	m_d1 = _fs->iir.d1;
	m_d2 = _fs->iir.d2;
}



//
// lb302Filter3Pole
//

lb302Filter3Pole::lb302Filter3Pole( lb302FilterKnobState * _fs ) :
	lb302Filter( _fs ),
	m_ay1( 0 ),
	m_ay2( 0 ),
	m_aout( 0 ),
	m_lastin( 0 )
{
};



void lb302Filter3Pole::recalc( void )
{
	// DO NOT CALL BASE CLASS

	m_e0 = 0.000001;
	m_e1 = 1.0;
}


void lb302Filter3Pole::envRecalc( void )
{
	// TODO: Try using k instead of vcf_reso
	float w, k;
	float kfco;

	lb302Filter::envRecalc();


	w = m_e0 + m_c0;          // e0 is adjusted for Hz and doesn't need ENVINC
	k = ( m_fs->cutoff > 0.975 ) ? 0.975 : m_fs->cutoff;
	kfco = 50.f +
	       k * (
	           ( 2300.f - 1600.f * m_fs->envmod) +
	           w * (
	               700.f +
	               1500.f * k +
	               ( 1500.f + k * ( 44100.f / 2.f - 6000.f ) ) *
	               m_fs->envmod
	               )
	           );

	//+iacc*(.3+.7*kfco*kenvmod)*kaccent*kaccurve*2000

#ifdef LB_24_IGNORE_ENVELOPE
	// kfcn = fs->cutoff;
	m_kfcn = 2.0 * kfco / LB_HZ;
#else
	m_kfcn = w;
#endif
	m_kp   = ( ( -2.7528 * m_kfcn + 3.0429 ) * m_kfcn + 1.718 ) * m_kfcn - 0.9984;
	m_kp1  = m_kp + 1.0;
	m_kp1h = 0.5 * m_kp1;
#ifdef LB_24_RES_TRICK
	k = exp( -w / m_rescoeff );
	m_kres = k *
		( ( ( -2.7079 * m_kp1 + 10.963 ) * m_kp1 - 14.934 ) * m_kp1 + 8.4974 );
#else
	m_kres = m_fs->reso *
		( ( ( -2.7079 * m_kp1 + 10.963 ) * m_kp1 - 14.934 ) * m_kp1 + 8.4974 );
#endif

	m_value = 1.0 + ( (m_fs->dist) * ( 1.5 + 2.0 * m_kres * ( 1.0 - m_kfcn ) ) );
}


float lb302Filter3Pole::process( const float & _samp )
{
	float ax1  = m_lastin;
	float ay11 = m_ay1;
	float ay31 = m_ay2;

	m_lastin = _samp - tanh( m_kres * m_aout );
	m_ay1     = m_kp1h * ( m_lastin + ax1 ) - m_kp * m_ay1;
	m_ay2     = m_kp1h * ( m_ay1 + ay11 )   - m_kp * m_ay2;
	m_aout    = m_kp1h * ( m_ay2 + ay31 )   - m_kp * m_aout;

	return tanh( m_aout * m_value ) * LB_24_VOL_ADJUST / ( 1.0 + m_fs->dist );
}


void lb302Filter3Pole::getState( lb302FilterState * _fs )
{
	_fs->pole.aout    = m_aout;
	_fs->pole.c0      = m_c0;
	_fs->pole.kp      = m_kp;
	_fs->pole.kp1h    = m_kp1h;
	_fs->pole.kres    = m_kres;
	_fs->pole.ay1     = m_ay1;
	_fs->pole.ay2     = m_ay2;
	_fs->pole.lastin  = m_lastin;
	_fs->pole.value   = m_value;
}


void lb302Filter3Pole::setState( const lb302FilterState * _fs )
{
	m_aout    = _fs->pole.aout;
	m_c0      = _fs->pole.c0;
	m_kp      = _fs->pole.kp;
	m_kp1h    = _fs->pole.kp1h;
	m_kres    = _fs->pole.kres;
	m_ay1     = _fs->pole.ay1;
	m_ay2     = _fs->pole.ay2;
	m_lastin  = _fs->pole.lastin;
	m_value   = _fs->pole.value;
}



//
// LBSynth
//

lb302Synth::lb302Synth( instrumentTrack * _channel_track ) :
	instrument( _channel_track, &lb302_plugin_descriptor ),

	m_vco_shape( SAWTOOTH ),
	m_vco_slide( 0 ),
	m_vco_slideinc( 0 ),
	m_vco_slidebase( 0 ),
	m_vco_detune( 0 ),
	m_vco_inc( 0.0 ),
	m_vco_k( 0 ),
	m_vco_c( 0 ),

	m_vcf_envpos( ENVINC ),

	m_vca_attack( 1.0 - 0.96406088 ),
	m_vca_decay( 0.99897516 ),
	m_vca_a0( 0.5 ),                // Experimenting between (0.5) and 1.0
	m_vca_a( 9 ),
	m_vca_mode( 3 ),

	m_useHoldNote( false ),
	m_sampleCnt( 0 ),
	m_releaseFrame( 1<<24 ),
	m_catchFrame( 0 ),
	m_catchDecay( 0 ),
	m_lastFramesPlayed( 1 ),
	m_lastOffset( 0 ),

	m_periodStates( NULL ),
	m_periodStatesCnt( 0 )

{
	// GUI

	m_vcfCutKnob = new knob( knobBright_26, this, tr( "VCF Cutoff Frequency" ),
	                         _channel_track );

	m_vcfCutKnob->setRange( 0.0f, 1.5f, 0.005f );   // Originally  [0,1.0]
	m_vcfCutKnob->setInitValue( 0.75f );
	m_vcfCutKnob->move( 75, 130 );
	m_vcfCutKnob->setHintText( tr( "Cutoff Freq:" ) + " ", "" );
	m_vcfCutKnob->setLabel( tr( "CUT" ) );

	m_vcfResKnob = new knob( knobBright_26, this, tr( "VCF Resonance" ),
	                         _channel_track );

	m_vcfResKnob->setRange( 0.0f, 1.25f, 0.005f );   // Originally [0,1.0]
	m_vcfResKnob->setInitValue( 0.75f );
	m_vcfResKnob->move( 120, 130 );
	m_vcfResKnob->setHintText( tr( "Resonance:" ) + " ", "" );
	m_vcfResKnob->setLabel( tr( "RES" ) );

	m_vcfModKnob = new knob( knobBright_26, this, tr( "VCF Envelope Mod" ),
	                         _channel_track );

	m_vcfModKnob->setRange( 0.0f, 1.0f, 0.005f );   // Originally  [0,1.0]
	m_vcfModKnob->setInitValue( 1.0f );
	m_vcfModKnob->move( 165, 130 );
	m_vcfModKnob->setHintText( tr( "Env Mod:" ) + " ", "" );
	m_vcfModKnob->setLabel( tr( "ENV MOD" ) );

	m_vcfDecKnob = new knob( knobBright_26, this, tr( "VCF Envelope Decay" ),
	                         _channel_track );

	m_vcfDecKnob->setRange( 0.0f, 1.0f, 0.005f );   // Originally [0,1.0]
	m_vcfDecKnob->setInitValue( 0.1f );
	m_vcfDecKnob->move( 210, 130 );
	m_vcfDecKnob->setHintText( tr( "Decay:" ) + " ", "" );
	m_vcfDecKnob->setLabel( tr( "DEC" ) );

	m_slideToggle = new ledCheckBox( "Slide", this, tr( "Slide" ),
	                                 _channel_track );

	m_slideToggle->move( 10, 180 );

	m_accentToggle = new ledCheckBox( "Accent", this, tr( "Accent" ),
	                                  _channel_track );
	m_accentToggle->move( 10, 200 );
	m_accentToggle->setDisabled( true );

	m_deadToggle = new ledCheckBox( "Dead", this, tr( "Dead" ),
	                                _channel_track );

	m_deadToggle->move( 10, 220 );

	m_db24Toggle = new ledCheckBox( "24dB/oct", this,
			tr( "303-es-que, 24dB/octave, 3 pole filter" ),
			_channel_track );

	m_db24Toggle->move( 10, 150 );

	m_slideDecKnob = new knob( knobBright_26, this, tr( "Slide Decay" ),
	                           _channel_track );
	m_slideDecKnob->setRange( 0.0f, 1.0f, 0.005f );   // Originally [0,1.0]
	m_slideDecKnob->setInitValue( 0.6f );
	m_slideDecKnob->move( 210, 75 );
	m_slideDecKnob->setHintText( tr( "Slide Decay:" ) + " ", "" );
	m_slideDecKnob->setLabel( tr( "SLIDE"));

	m_vcoFineDetuneKnob = new knob( knobBright_26, this,
			tr( "Fine detuning of the VCO. Between -100 and 100 cents." ),
			_channel_track );
	m_vcoFineDetuneKnob->setRange( -100.0f, 100.0f, 1.0f );
	m_vcoFineDetuneKnob->setInitValue( 0.0f );
	m_vcoFineDetuneKnob->move( 165, 75 );
	m_vcoFineDetuneKnob->setHintText( tr( "VCO Fine Detuning:" ) + " ", "cents" );
	m_vcoFineDetuneKnob->setLabel( tr( "DETUNE" ) );


	m_distKnob = new knob( knobBright_26, this, tr( "Distortion" ),
	                       _channel_track );
	m_distKnob->setRange( 0.0f, 1.0f, 0.01f );   // Originally [0,1.0]
	m_distKnob->setInitValue( 0.0f );
	m_distKnob->move( 210, 190 );
	m_distKnob->setHintText( tr( "DIST:" ) + " ", "" );
	m_distKnob->setLabel( tr( "DIST" ) );

	m_waveKnob = new knob( knobBright_26, this, tr( "Waveform" ),
	                       _channel_track );
	m_waveKnob->setRange( 0.0f, 5.0f, 1.0f );   // Originally [0,1.0]
	m_waveKnob->setInitValue( 0.0f );
	m_waveKnob->move( 120, 75 );
	m_waveKnob->setHintText( tr( "WAVE:" ) + " ", "" );
	m_waveKnob->setLabel( tr( "WAVE" ) );


	connect( m_vcfCutKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT ( filterChanged( float ) ) );

	connect( m_vcfResKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT ( filterChanged( float ) ) );

	connect( m_vcfModKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT ( filterChanged( float ) ) );

	connect( m_vcfDecKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT ( filterChanged( float ) ) );

	connect( m_vcoFineDetuneKnob, SIGNAL( valueChanged( float ) ),
			this, SLOT ( detuneChanged( float) ) );

	connect( m_db24Toggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( db24Toggled( bool) ) );

	connect( m_distKnob, SIGNAL( valueChanged(float) ),
			this, SLOT ( filterChanged( float ) ) );

	connect( m_waveKnob, SIGNAL( valueChanged(float) ),
			this, SLOT ( waveChanged( float ) ) );

	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
	              PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );


	// SYNTH

	m_fs.cutoff = 0;
	m_fs.envmod = 0;
	m_fs.reso = 0;
	m_fs.envdecay = 0;
	m_fs.dist = 0;

	m_vcf = new lb302FilterIIR2( &m_fs );

	recalcFilter();

	filterChanged( 0.0 );
	detuneChanged( 0.0 );
}


lb302Synth::~lb302Synth()
{
	delete m_vcf;
}




void lb302Synth::saveSettings( QDomDocument & _doc,
                               QDomElement & _this )
{
	m_vcfCutKnob->saveSettings( _doc, _this, "vcf_cut" );
	m_vcfResKnob->saveSettings( _doc, _this, "vcf_res" );
	m_vcfModKnob->saveSettings( _doc, _this, "vcf_mod" );
	m_vcfDecKnob->saveSettings( _doc, _this, "vcf_dec" );

	m_vcoFineDetuneKnob->saveSettings( _doc, _this, "vco_detune" );
	m_waveKnob->saveSettings( _doc, _this, "shape" );
	m_distKnob->saveSettings( _doc, _this, "dist" );
	m_slideDecKnob->saveSettings( _doc, _this, "slide_dec" );

	m_slideToggle->saveSettings( _doc, _this, "slide" );
	m_deadToggle->saveSettings( _doc, _this, "dead" );
	m_db24Toggle->saveSettings( _doc, _this, "db24" );
}


void lb302Synth::loadSettings( const QDomElement & _this )
{
	m_vcfCutKnob->loadSettings( _this, "vcf_cut" );
	m_vcfResKnob->loadSettings( _this, "vcf_res" );
	m_vcfModKnob->loadSettings( _this, "vcf_mod" );
	m_vcfDecKnob->loadSettings( _this, "vcf_dec" );

	m_vcoFineDetuneKnob->loadSettings( _this, "vco_detune" );
	m_distKnob->loadSettings( _this, "dist" );
	m_waveKnob->loadSettings( _this, "shape" );
	m_slideDecKnob->loadSettings( _this, "slide_dec" );

	m_slideToggle->loadSettings( _this, "slide" );
	m_deadToggle->loadSettings( _this, "dead" );
	m_db24Toggle->loadSettings( _this, "db24" );

	filterChanged( 0.0 );
	detuneChanged( 0.0 );
}


// TODO: Split into one function per knob.  envdecay doesn't require
// recalcFilter.
void lb302Synth::filterChanged( float )
{
	m_fs.cutoff = m_vcfCutKnob->value();
	m_fs.reso   = m_vcfResKnob->value();
	m_fs.envmod = m_vcfModKnob->value();
	m_fs.dist   = m_distKnob->value() * LB_DIST_RATIO;

	float d = 0.2 + ( 2.3 * m_vcfDecKnob->value() );
	d *= LB_HZ;
	m_fs.envdecay = pow( 0.1, 1.0 / d * ENVINC );
	// vcf_envdecay is now adjusted for both sampling rate and ENVINC

	recalcFilter();
}


void lb302Synth::db24Toggled( bool )
{
	delete m_vcf;

	if( m_db24Toggle->isChecked() )  {
		m_vcf = new lb302Filter3Pole( &m_fs );
	}
	else {
		m_vcf = new lb302FilterIIR2( &m_fs );
	}
	recalcFilter();
}


void lb302Synth::detuneChanged( float )
{
	float freq = m_vco_inc * LB_HZ / m_vco_detune;
	float slidebase_freq = 0;

	if( m_vco_slide ) {
		slidebase_freq = m_vco_slidebase * LB_HZ / m_vco_detune;
	}

	m_vco_detune = powf( 2.0f,
			(float)m_vcoFineDetuneKnob->value() / 1200.0f);

	m_vco_inc = freq * m_vco_detune/LB_HZ;

	// If a slide note is pending,
	if( m_vco_slideinc )
		m_vco_slideinc = m_vco_inc;

	// If currently sliding,
	// May need to rescale vco_slide as well
	if( m_vco_slide)
		m_vco_slidebase = slidebase_freq * m_vco_detune / LB_HZ;
}


// TODO: Set vco_shape in here.
void lb302Synth::waveChanged( float ) 
{
	switch( (int)rint( m_waveKnob->value() ) ) {
		case 0:
			m_waveKnob->setHintText( tr( "Sawtooth " ), "" );
			break;
		case 1:
			m_waveKnob->setHintText( tr( "Inverted Sawtooth " ), "" );
			break;
		case 2:
			m_waveKnob->setHintText( tr( "Triangle " ), "" );
			break;
		case 3:
			m_waveKnob->setHintText( tr( "Square " ), "" );
			break;
		case 4:
			m_waveKnob->setHintText( tr( "Rounded Square " ), "" );
			break;
		case 5:
			m_waveKnob->setHintText( tr( "Moog " ), "" );
			break;
	}
}


QString lb302Synth::nodeName( void ) const
{
	return( lb302_plugin_descriptor.name );
}

// OBSOLETE. Break apart once we get Q_OBJECT to work. >:[
void lb302Synth::recalcFilter()
{
	m_vcf->recalc();

	// THIS IS OLD 3pole/24dB code, I may reintegrate it.  Don't need it
	// right now.   Should be toggled by LB_24_RES_TRICK at the moment.

	/*kfcn = 2.0 * (((vcf_cutoff*3000))) / LB_HZ;
	kp   = ((-2.7528*kfcn + 3.0429)*kfcn + 1.718)*kfcn - 0.9984;
	kp1  = kp+1.0;
	kp1h = 0.5*kp1;
	kres = (((vcf_reso))) * (((-2.7079*kp1 + 10.963)*kp1 - 14.934)*kp1 + 8.4974);
	value = 1.0+( (((0))) *(1.5 + 2.0*kres*(1.0-kfcn))); // ENVMOD was DIST*/

	m_vcf_envpos = ENVINC; // Trigger filter update in process()
}


int lb302Synth::process( sampleFrame * _outbuf, const Uint32 _size )
{

	unsigned int i;
	float w;
	float samp;

	for( i=0; i < _size; i++ ) {

		// update vcf
		if( m_vcf_envpos >= ENVINC ) {
			m_vcf->envRecalc();

			m_vcf_envpos = 0;

			if( m_vco_slide ) {
				m_vco_inc = m_vco_slidebase - m_vco_slide;
				// Calculate coeff from dec_knob on knob change.
				// TODO: Adjust for Hz and ENVINC
				m_vco_slide *= 0.9 + ( m_slideDecKnob->value() * 0.0999 ); 
			}
		}

		m_sampleCnt++;
		m_vcf_envpos++;

		int decayFrames = 128;

		// update vco
		m_vco_c += m_vco_inc;

		if( m_vco_c > 0.5 )
			m_vco_c -= 1.0;

		if( m_catchDecay > 0 ) {
			if( m_catchDecay < decayFrames ) {
				m_catchDecay++;
			}
			else if( m_useHoldNote ) {
				m_useHoldNote = false;
				initNote( &m_holdNote );
			}
		}

		switch( (int)rint( m_waveKnob->value() ) ) {
			case 0:   m_vco_shape = SAWTOOTH; break;
			case 1:   m_vco_shape = INVERTED_SAWTOOTH; break;
			case 2:   m_vco_shape = TRIANGLE; break;
			case 3:   m_vco_shape = SQUARE; break;
			case 4:   m_vco_shape = ROUND_SQUARE; break;
			case 5:   m_vco_shape = MOOG; break;
			default:  m_vco_shape = SAWTOOTH; break;
		}

		// add vco_shape_param the changes the shape of each curve.
		// merge sawtooths with triangle and square with round square?
		switch (m_vco_shape) {
			case SAWTOOTH:           // p0: curviness of line
				m_vco_k = m_vco_c;   // Is this sawtooth backwards?
				break;

			case INVERTED_SAWTOOTH:  // p0: curviness of line
				m_vco_k = -m_vco_c;  // Is this sawtooth backwards?
				break;

			case TRIANGLE:  // p0: duty rev.saw<->triangle<->saw p1: curviness
				m_vco_k = ( m_vco_c * 2.0 ) + 0.5;
				if( m_vco_k > 0.5 )
					m_vco_k = 1.0 - m_vco_k;
				break;

			case SQUARE:    // p0: slope of top
				m_vco_k = ( m_vco_c < 0 ) ? 0.5 : -0.5;
				break;

			case ROUND_SQUARE: // p0: width of round
				m_vco_k = ( m_vco_c < 0 ) ?
						( sqrtf( 1 - ( m_vco_c*m_vco_c * 4 ) ) - 0.5) : 
						-0.5;
				break;

			case MOOG: // Maybe the fall should be exponential/sinsoidal instead of quadric.
				// [-0.5, 0]: Rise, [0,0.25]: Slope down, [0.25,0.5]: Low 
				m_vco_k = ( m_vco_c * 2.0 ) + 0.5;
				if( m_vco_k > 1.0 )
					m_vco_k = -0.5;
				else if( m_vco_k > 0.5 ) {
					w = 2.0 * ( m_vco_k - 0.5 ) - 1.0;
					m_vco_k = 0.5 - sqrtf( 1.0 - w*w );
				}
				m_vco_k *= 2.0;  // MOOG wave gets filtered away 
				break;
		}

		// Write out samples.
#ifdef LB_FILTERED
		samp = m_vcf->process( m_vco_k ) * 2.0 * m_vca_a;
#else
        samp = m_vco_k * m_vca_a;
#endif
		/*
		float releaseFrames = desiredReleaseFrames();
		samp *= (releaseFrames - catch_decay)/releaseFrames;
		*/
		samp *= (float)( decayFrames - m_catchDecay ) /
		        (float)decayFrames;

		for( int c=0; c < DEFAULT_CHANNELS; c++ ) {
			_outbuf[i][c] = samp;
		}


		if( (int)i >= m_releaseFrame ) {
			m_vca_mode = 1;
		}

		// Handle Envelope
		// TODO: Add decay once I figure out how to extend past the end of a note.
		if( m_vca_mode == 0 ) {
			m_vca_a += ( m_vca_a0 - m_vca_a ) * m_vca_attack;
			if( m_sampleCnt >= 0.5 * 44100 )
				m_vca_mode = 2;
		}
		else if( m_vca_mode == 1) {
			m_vca_a *= m_vca_decay;

			// the following line actually speeds up processing
			if( m_vca_a < 1/65536.0 ) {
				m_vca_a = 0;
				m_vca_mode = 3;
			}
		}
		// Store state
		m_periodStates[i].vco_c = m_vco_c;
		m_periodStates[i].vca_a = m_vca_a;          // Doesn't change anything (currently)
		m_periodStates[i].vca_mode = m_vca_mode;    // Doesn't change anything (currently)
		m_periodStates[i].sampleCnt = m_sampleCnt;  // Doesn't change anything (currently)
		m_vcf->getState(&m_periodStates[i].fs);

	}
	return 1;
}


/*  Prepares the active LB302 note.  I separated this into a function because it
 *  needs to be called onPlayNote() when a new note is started.  It also needs
 *  to be called from process() when a prior edge-to-edge note is done releasing.
 */
void lb302Synth::initNote( lb302Note * _n )
{
	m_catchDecay = 0;

	m_vco_inc = _n->vco_inc;

	// TODO: Try moving to the if() below
	if( _n->dead == 0 || ( m_vca_mode == 1 || m_vca_mode == 3 ) ) {
		m_sampleCnt = 0;
		m_vca_mode  = 0;
		m_vca_a     = 0;
	}
	else {
		m_vca_mode  = 2;
	}

	// Initiate Slide
	// TODO: Break out into function, should be called again on detuneChanged
	if( m_vco_slideinc ) {
		m_vco_slide     = m_vco_inc - m_vco_slideinc;
		m_vco_slidebase = m_vco_inc;
		m_vco_slideinc  = 0;
	}
	else {
		m_vco_slide = 0;
	}
	// End break-out

	// Slide note, save inc for next note
	if( m_slideToggle->value() ) {
		// May need to equal vco_slidebase+vco_slide if last note slid
		m_vco_slideinc = m_vco_inc;
	}

	recalcFilter();

	if( _n->dead == 0 ) {
		// Swap next two blocks??
		m_vcf->playNote();
		// Ensure envelope is recalculated
		m_vcf_envpos = ENVINC;

		// Better safe then sorry
		m_vca_mode = 0;
		m_vca_a    = 0.0;
	}
}


void lb302Synth::playNote( notePlayHandle * _n, bool )
{
	fpp_t framesPerPeriod = engine::getMixer()->framesPerPeriod();

	///=== WEIRD CODE FOR MONOPHONIC BEHAVIOUR - BEGIN === ///

	if( _n->arpBaseNote() )
	{
		return;
	}

	// number of frames to play - only modified below if we have to play
	// the rest of an old note
	fpp_t frames = _n->framesLeftForCurrentPeriod();

	// per default we resume at the last played frames - only in
	// some special-cases (which we catch below) we have to resume
	// somewhere else
	f_cnt_t resumePos = m_lastFramesPlayed - 1;

	bool decayNote = false;

	// find out which situation we're in
	constNotePlayHandleVector v =
			notePlayHandle::nphsOfInstrumentTrack(
			getInstrumentTrack(), TRUE );

	// more than one note running?
	if( v.count() > 1 )
	{
		const notePlayHandle * on = v.first();  // oldest note
		const notePlayHandle * yn = v.last();   // youngest note

		// are we playing a released note and the new (youngest) note
		// has taken over successfully (i.e. played more than the
		// difference of the two offsets)?
		if ( _n->released() &&
			yn->totalFramesPlayed() >= yn->offset() - on->offset() )
		{
			// then we do not need to play something anymore
			return;
		}

		// have to fill up the frames left to the new note so limit
		// frames to play for not getting into trouble
		if( _n != yn && !yn->arpBaseNote() )
		{
			frames = tMin<fpp_t>( frames, yn->offset() - on->offset() );
#ifdef LB_DEBUG
			// should be always true - why? I don't know... ;-)
			assert( frames > 0 );
#endif
		}

		// new note while other notes are running?
		if( v.count() > 1 && yn == _n &&
						_n->totalFramesPlayed() == 0 )
		{
			// if there had been a previous note whose
			// offset > _n->offset() it played more frames than
			// we actually need - therefore clear everything before
			// the offset of the youngest note, otherwise we get
			// frames with both waves overlapped
			engine::getMixer()->clearAudioBuffer(
					_n->getInstrumentTrack()->getAudioPort()->firstBuffer(),
					framesPerPeriod - yn->offset(), 
					yn->offset() );

			resumePos = yn->offset() - on->offset() - 1;
			// make sure we have positive value, otherwise we're
			// accessing states out of borders
			while( resumePos < 0 )
			{
				resumePos += framesPerPeriod;
			}
			decayNote = true;
		}
	}

	///=== WEIRD CODE FOR MONOPHONIC BEHAVIOUR - END === ///

	/// Malloc our period history buffer
	if( m_periodStates == NULL ) {
		m_periodStates = new lb302State[framesPerPeriod];

		for( int i = 0; i < framesPerPeriod; i++ ) {
			m_periodStates[i].vco_c      = m_vco_c;
			m_periodStates[i].vca_a      = m_vca_a;       // Doesn't change anything (currently)
			m_periodStates[i].vca_mode   = m_vca_mode;    // Doesn't change anything (currently)
			m_periodStates[i].sampleCnt = m_sampleCnt;    // Doesn't change anything (currently)
			m_vcf->getState( &m_periodStates[i].fs );
		}
	}


	// now resume at the proper position and process as usual
	lb302State * state = &m_periodStates[resumePos];

	/// Actually resume the state, now that we have the right state object.
	m_vco_c     = state->vco_c;
	m_vca_a     = state->vca_a;
	m_vca_mode  = state->vca_mode;
	m_sampleCnt = state->sampleCnt;
	m_vcf->setState( &state->fs );


	m_releaseFrame = _n->framesLeft() - desiredReleaseFrames();

	if( _n->totalFramesPlayed() <= 0 ) {

		// Existing note. Allow it to decay.
		if( m_deadToggle->value() == 0 && decayNote ) {
#ifdef LB_DECAY
			if( m_catchDecay < 1 ) {
				// BEGIN NOT SURE OF...
				//lb302State *st = &period_states[period_states_cnt-1];
				//vca_a = st->vca_a;
				//sample_cnt = st->sample_cnt;
				// END NOT SURE OF

				// Reserve this note for retrigger in process()
				m_holdNote.vco_inc = _n->frequency() * m_vco_detune/LB_HZ;
				m_holdNote.dead    = m_deadToggle->value();
				m_useHoldNote      = true;
				m_catchDecay       = 1;
			}
#else
			lb302Note note;
			note.vco_inc = _n->frequency() * m_vco_detune/LB_HZ;
			note.dead    = m_deadToggle->value();

			initNote( &note );
			m_vca_mode = 0;
			m_vca_a    = state->vca_a;

#endif
		}
		/// Start a new note.
		else {
			lb302Note note;
			note.vco_inc = _n->frequency() * m_vco_detune/LB_HZ;
			note.dead    = m_deadToggle->value();
			
			initNote( &note );
			m_useHoldNote = false;
		}
	}

	sampleFrame *buf = new sampleFrame[frames];

	process( buf, frames );
	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

	delete[] buf;

	m_lastFramesPlayed = frames;
	//_n->framesLeftForCurrentPeriod(); //_n->totalFramesPlayed();
}



void lb302Synth::deleteNotePluginData( notePlayHandle * _n )
{
}



extern "C"
{

	// neccessary for getting instance out of shared lib
	plugin * lmms_plugin_main( void * _data )
	{
	return( new lb302Synth(
			static_cast<instrumentTrack *>( _data ) ) );
	}

}


