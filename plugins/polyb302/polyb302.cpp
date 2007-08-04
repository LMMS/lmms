/*
 * polyb302.cpp - implementation of instrument polyb302, an attempt to emulate
 *                the Roland TB303 bass synth
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


#include "polyb302.h"
#include "engine.h"
#include "knob.h"
#include "led_checkbox.h"
#include "note_play_handle.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"
#include "polyb302.moc"


// Envelope Recalculation period
#define ENVINC 64

//
// New config
//
#define LB_24_IGNORE_ENVELOPE   
#define LB_FILTERED 
//#define LB_24_RES_TRICK         

#define LB_DIST_RATIO    4.0
#define LB_24_VOL_ADJUST 3.0




using namespace std;
extern "C"
{

plugin::descriptor polyb302_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"PoLyB302",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Incomplete polyphonic immitation tb303" ),
	"Javier Serrano Polo <jasp00/at/users.sourceforge.net>",
	0x0100,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;


// necessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new polyb302Synth( static_cast<instrumentTrack *>( _data ) ) );
}

}



//
// lb302Filter
//

lb302Filter::lb302Filter( lb302FilterState * _p_fs ) :
	m_fs( _p_fs ),
	m_vcf_c0( 0 ),
	m_vcf_e0( 0 ),
	m_vcf_e1( 0 )
{
}




void lb302Filter::recalc( void )
{
	m_vcf_e1 = exp( 4.909 + 1.5876 * m_fs->envmod + 2.1553 * m_fs->cutoff
							+ 1.2 * m_fs->reso );
	m_vcf_e0 = exp( 4.8434 - 0.8 * m_fs->envmod + 2.1553 * m_fs->cutoff
							+ 0.7696 * m_fs->reso );
	m_vcf_e0 *= M_PI / engine::getMixer()->sampleRate();
	m_vcf_e1 *= M_PI / engine::getMixer()->sampleRate();
	m_vcf_e1 -= m_vcf_e0;
}




void lb302Filter::envRecalc( void )
{
	// Filter Decay. vcf_decay is adjusted for Hz and ENVINC
	m_vcf_c0 *= m_fs->envdecay;
	m_vcf_rescoeff = exp( -1.20 + 3.455 * m_fs->reso );
}




void lb302Filter::playNote( void )
{
	m_vcf_c0 = m_vcf_e1;
}



//
// lb302FilterIIR2
//

lb302FilterIIR2::lb302FilterIIR2( lb302FilterState * _p_fs ) :
	lb302Filter( _p_fs ),
	m_vcf_d1( 0 ),
	m_vcf_d2( 0 ),
	m_vcf_a( 0 ),
	m_vcf_b( 0 ),
	m_vcf_c( 1 )
{
	m_dist = new effectLib::distortion<>( 1.0, 1.0f );
}




lb302FilterIIR2::~lb302FilterIIR2()
{
	delete m_dist;
}




void lb302FilterIIR2::recalc( void )
{
	lb302Filter::recalc();
	//m_dist->setThreshold(0.5+(m_fs->dist*2.0));
	m_dist->setThreshold( m_fs->dist * 75.0 );
}




void lb302FilterIIR2::envRecalc( void )
{
	lb302Filter::envRecalc();

	// e0 is adjusted for Hz and doesn't need ENVINC
	float w = m_vcf_e0 + m_vcf_c0;
	float k = exp( -w / m_vcf_rescoeff );
	// Does this mean c0 is inheritantly?
	m_vcf_a = 2.0 * cos( 2.0 * w ) * k;   
	m_vcf_b = -k * k;
	m_vcf_c = 1.0 - m_vcf_a - m_vcf_b;
}




float lb302FilterIIR2::process( const float & _samp )
{
	float ret = m_vcf_a * m_vcf_d1 + m_vcf_b * m_vcf_d2 + m_vcf_c * _samp;
	// Delayed samples for filter
	m_vcf_d2 = m_vcf_d1;
	m_vcf_d1 = ret;

	if( m_fs->dist > 0 )
	{
		ret = m_dist->nextSample( ret );
	}
	// output = IIR2 + dry
	return( ret );
}



//
// lb302Filter3Pole
//

lb302Filter3Pole::lb302Filter3Pole( lb302FilterState * _p_fs ) :
	lb302Filter( _p_fs ),
	m_ay1( 0 ),
	m_ay2( 0 ),
	m_aout( 0 ),
	m_lastin( 0 )
{
}




void lb302Filter3Pole::recalc( void )
{
	// DO NOT CALL BASE CLASS
	m_vcf_e0 = 0.000001;
	m_vcf_e1 = 1.0;
}




// TODO: Try using k instead of vcf_reso
void lb302Filter3Pole::envRecalc( void )
{
	lb302Filter::envRecalc();


	// e0 is adjusted for Hz and doesn't need ENVINC
	float w = m_vcf_e0 + m_vcf_c0;
	float k = ( m_fs->cutoff > 0.975 ) ? 0.975 : m_fs->cutoff;
	//TODO: Fix high quality
	float kfco = 50.0f + k * ( 2300.0f - 1600.0f * m_fs->envmod
		+ w * ( 700.0f + 1500.0f * k + ( 1500.0f + k * (
			engine::getMixer()->sampleRate() / 2.0f - 6000.0f ) )
							* m_fs->envmod ) );
	//+iacc*(.3+.7*kfco*kenvmod)*kaccent*kaccurve*2000


#ifdef LB_24_IGNORE_ENVELOPE
	// m_kfcn = m_fs->cutoff;
	m_kfcn = 2.0 * kfco / engine::getMixer()->sampleRate();
#else
	m_kfcn = w;
#endif
	m_kp = ( ( -2.7528 * m_kfcn + 3.0429 ) * m_kfcn + 1.718 ) * m_kfcn
								- 0.9984;
	m_kp1 = m_kp + 1.0;
	m_kp1h = 0.5 * m_kp1;
#ifdef LB_24_RES_TRICK
	k = exp( -w / m_vcf_rescoeff );
	m_kres = k * ( ( ( -2.7079 * m_kp1 + 10.963 ) * m_kp1 - 14.934 ) * m_kp1
								+ 8.4974 );
#else
	m_kres = m_fs->reso * ( ( ( -2.7079 * m_kp1 + 10.963 ) * m_kp1
						- 14.934 ) * m_kp1 + 8.4974 );
#endif
	// ENVMOD was DIST*/
	m_value = 1.0 + ( m_fs->dist * ( 1.5 + 2.0 * m_kres
							* ( 1.0 - m_kfcn ) ) );
}




float lb302Filter3Pole::process( const float & _samp )
{
	float ax1 = m_lastin;
	float ay11 = m_ay1;
	float ay31 = m_ay2;
	m_lastin = _samp - tanh( m_kres * m_aout );
	m_ay1 = m_kp1h * ( m_lastin + ax1 ) - m_kp * m_ay1;
	m_ay2 = m_kp1h * ( m_ay1 + ay11 ) - m_kp * m_ay2;
	m_aout = m_kp1h * ( m_ay2 + ay31 ) - m_kp * m_aout;

	return( tanh( m_aout * m_value ) * LB_24_VOL_ADJUST
						/ ( 1.0 + m_fs->dist ) );
}



//
// PoLyBSynth
//

polyb302Synth::polyb302Synth( instrumentTrack * _track ) :
	instrument( _track, &polyb302_plugin_descriptor )
{
	m_vcf_cut_knob = new knob( knobBright_26, this,
						tr( "VCF Cutoff Frequency" ),
								_track );
	m_vcf_cut_knob->setRange( 0.0f, 1.5f, 0.005f );   // Originally  [0,1.0]
 	m_vcf_cut_knob->setInitValue( 0.75f );
	m_vcf_cut_knob->move( 75, 130 );
	m_vcf_cut_knob->setHintText( tr( "Cutoff Freq:" ) + " ", "" );
	m_vcf_cut_knob->setLabel( tr( "CUT" ) );

	m_vcf_res_knob = new knob( knobBright_26, this, tr( "VCF Resonance" ),
								_track );
	m_vcf_res_knob->setRange( 0.0f, 1.25f, 0.005f );   // Originally [0,1.0]
	m_vcf_res_knob->setInitValue( 0.75f );
	m_vcf_res_knob->move( 120, 130 );
	m_vcf_res_knob->setHintText( tr( "Resonance:" ) + " ", "" );
	m_vcf_res_knob->setLabel( tr( "RES" ) );

	m_vcf_mod_knob = new knob( knobBright_26, this,
						tr( "VCF Envelope Mod" ),
								_track );
	m_vcf_mod_knob->setRange( 0.0f, 1.0f, 0.005f );   // Originally  [0,1.0]
 	m_vcf_mod_knob->setInitValue( 1.0f );
	m_vcf_mod_knob->move( 165, 130 );
	m_vcf_mod_knob->setHintText( tr( "Env Mod:" ) + " ", "" );
	m_vcf_mod_knob->setLabel( tr( "ENV MOD" ) );

	m_vcf_dec_knob = new knob( knobBright_26, this,
						tr( "VCF Envelope Decay" ),
								_track );
	m_vcf_dec_knob->setRange( 0.0f, 1.0f, 0.005f );   // Originally [0,1.0]
	m_vcf_dec_knob->setInitValue( 0.1f );
	m_vcf_dec_knob->move( 210, 130 );
	m_vcf_dec_knob->setHintText( tr( "Decay:" ) + " ", "" );
	m_vcf_dec_knob->setLabel( tr( "DEC" ) );

//	m_slideToggle = new ledCheckBox( "Slide", this, tr( "Slide" ), _track );
//	m_slideToggle->move( 10, 180 );

//	m_accentToggle = new ledCheckBox( "Accent", this,
//							tr( "Accent" ),
//							_track );
//	m_accentToggle->move( 10, 200 );
//	m_accentToggle->setDisabled(true);

//	m_deadToggle = new ledCheckBox( "Dead", this, tr( "Dead" ), _track );
//	m_deadToggle->move( 10, 220 );

	m_db24Toggle = new ledCheckBox( "24dB/oct", this,
				tr( "303-es-que, 24dB/octave, 3 pole filter" ),
								_track );
	m_db24Toggle->move( 10, 150 );


	m_slide_dec_knob = new knob( knobBright_26, this, tr( "Slide Decay" ),
								_track );
	m_slide_dec_knob->setRange( 0.0f, 1.0f, 0.005f );  // Originally [0,1.0]
	m_slide_dec_knob->setInitValue( 0.6f );
	m_slide_dec_knob->move( 210, 75 );
	m_slide_dec_knob->setHintText( tr( "Slide Decay:" ) + " ", "" );
	m_slide_dec_knob->setLabel( tr( "SLIDE" ) );

	m_vco_fine_detune_knob = new knob( knobBright_26, this,
						tr( "VCO fine detuning" ),
								_track );
	m_vco_fine_detune_knob->setRange( -100.0f, 100.0f, 1.0f );
	m_vco_fine_detune_knob->setInitValue( 0.0f );
	m_vco_fine_detune_knob->move( 165, 75 );
	m_vco_fine_detune_knob->setHintText( tr( "VCO Fine Detuning:" ) + " ",
								"cents" );
	m_vco_fine_detune_knob->setLabel( tr( "DETUNE" ) );


	m_dist_knob = new knob( knobBright_26, this, tr( "Distortion" ),
								_track );
	m_dist_knob->setRange( 0.0f, 1.0f, 0.01f );   // Originally [0,1.0]
	m_dist_knob->setInitValue( 0.0f );
	m_dist_knob->move( 210, 190 );
	m_dist_knob->setHintText( tr( "DIST:" ) + " ", "" );
	m_dist_knob->setLabel( tr( "DIST" ) );


	m_wave_knob = new knob( knobBright_26, this, tr( "Waveform" ), _track );
	m_wave_knob->setRange( 0.0f, 5.0f, 1.0f );   // Originally [0,1.0]
	m_wave_knob->setInitValue( 0.0f );
	m_wave_knob->move( 120, 75 );
	m_wave_knob->setHintText( tr( "WAVE:" ) + " ", "" );
	m_wave_knob->setLabel( tr( "WAVE" ) );


#ifndef QT3
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif

	connect( m_vcf_cut_knob, SIGNAL( valueChanged( float ) ),
					this, SLOT ( filterChanged( float ) ) );
	connect( m_vcf_res_knob, SIGNAL( valueChanged( float ) ),
					this, SLOT ( filterChanged( float ) ) );
	connect( m_vcf_mod_knob, SIGNAL( valueChanged( float ) ),
					this, SLOT ( filterChanged( float ) ) );
	connect( m_vcf_dec_knob, SIGNAL( valueChanged( float ) ),
					this, SLOT ( filterChanged( float ) ) );
	connect( m_vco_fine_detune_knob, SIGNAL( valueChanged( float ) ),
					this, SLOT ( detuneChanged( float) ) );
	connect( m_db24Toggle, SIGNAL( toggled( bool ) ),
					this, SLOT ( db24Toggled( bool) ) );
	connect( m_dist_knob, SIGNAL( valueChanged(float) ),
					this, SLOT ( filterChanged( float ) ) );
	connect( m_wave_knob, SIGNAL( valueChanged( float ) ),
					this, SLOT ( waveChanged( float ) ) );
}




polyb302Synth::~polyb302Synth()
{
}




void polyb302Synth::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_vcf_cut_knob->saveSettings( _doc, _this, "vcf_cut" );
	m_vcf_res_knob->saveSettings( _doc, _this, "vcf_res" );
	m_vcf_mod_knob->saveSettings( _doc, _this, "vcf_mod" );
	m_vcf_dec_knob->saveSettings( _doc, _this, "vcf_dec" );

	m_vco_fine_detune_knob->saveSettings( _doc, _this, "vco_detune" );
	m_wave_knob->saveSettings( _doc, _this, "shape" );
	m_dist_knob->saveSettings( _doc, _this, "dist" );
	m_slide_dec_knob->saveSettings( _doc, _this, "slide_dec" );

//	m_slideToggle->saveSettings( _doc, _this, "slide" );
//	m_deadToggle->saveSettings( _doc, _this, "dead" );
	m_db24Toggle->saveSettings( _doc, _this, "db24");
}




void polyb302Synth::loadSettings( const QDomElement & _this )
{
	m_vcf_cut_knob->loadSettings( _this, "vcf_cut" );
	m_vcf_res_knob->loadSettings( _this, "vcf_res" );
	m_vcf_mod_knob->loadSettings( _this, "vcf_mod" );
	m_vcf_dec_knob->loadSettings( _this, "vcf_dec" );

	m_vco_fine_detune_knob->loadSettings( _this, "vco_detune" );
	m_dist_knob->loadSettings( _this, "dist" );
	m_wave_knob->loadSettings( _this, "shape" );
	m_slide_dec_knob->loadSettings( _this, "slide_dec" );

//	m_slideToggle->loadSettings( _this, "slide" );
//	m_deadToggle->loadSettings( _this, "dead" );
	m_db24Toggle->loadSettings( _this, "db24" );
}




QString polyb302Synth::nodeName( void ) const
{
	return( polyb302_plugin_descriptor.name );
}




void polyb302Synth::playNote( notePlayHandle * _n, bool )
{
    //int nidx = _n->index();

    //if( _n->nphsOfInstrumentTrack(_n->getInstrumentTrack()).first() != _n )
    //if( _n->released() && _n->nphsOfInstrumentTrack( _n->getInstrumentTrack() ).count() > 1 )
    //    return;

/*
    if (_n->released() ) {
      if( notePlayHandle::nphsOfInstrumentTrack( getInstrumentTrack() ).size() > 0
        && notePlayHandle::nphsOfInstrumentTrack( getInstrumentTrack(),
        TRUE ).last() == _n )
        {
            return;
        }
    }
*/

	handleState * hstate;
	if( !_n->m_pluginData )
	{
		hstate = new handleState( this );
		_n->m_pluginData = hstate;
		m_handleStates.push_back( hstate );
	}
	else
	{
		hstate = (handleState *)_n->m_pluginData;
	}

	if( _n->totalFramesPlayed() <= hstate->m_lastFramesPlayed )
	{
	        // TODO: Try moving to the if() below
//		if( m_deadToggle->value() == 0 )
		{
			hstate->m_sample_cnt = 0;
			hstate->m_vca_mode = 0;
			hstate->m_vca_a = 0;
		}

		// Adjust inc on SampRate change or detuning change
		hstate->m_vco_inc = hstate->m_vco_detune
					/ engine::getMixer()->sampleRate();

		// Initiate Slide
		// TODO: Break out into function,
		// should be called again on detuneChanged
		if( hstate->m_vco_slideinc )
		{
			hstate->m_vco_slide = hstate->m_vco_inc
						- hstate->m_vco_slideinc;
			hstate->m_vco_slidebase = hstate->m_vco_inc;
			hstate->m_vco_slideinc = 0;
		}
		else
		{
			hstate->m_vco_slide = 0;
		}
		// End break-out

		// Slide note, save inc for next note
//		if( m_slideToggle->value() )
//		{
//			hstate->m_vco_slideinc = hstate->m_vco_inc;
			// May need to equal m_vco_slidebase+m_vco_slide if last
			// note slid
//		}


		hstate->recalcFilter();
        
//		if( m_deadToggle->value() == 0 )
		{
			// Swap next two blocks??
			hstate->m_vcf->playNote();
			// Ensure envelope is recalculated
			hstate->m_vcf_envpos = ENVINC;

			// Double Check 
			hstate->m_vca_mode = 0;
			hstate->m_vca_a = 0.0;
		}
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	if( frames == 0 )
	{
		return;
	}
	sampleFrame * buf = new sampleFrame[frames];

	hstate->process( buf, frames, _n->frequency() ); 
	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

	delete[] buf;

	hstate->m_lastFramesPlayed = _n->totalFramesPlayed();
}




void polyb302Synth::deleteNotePluginData( notePlayHandle * _n )
{
	handleState * hstate = (handleState *)_n->m_pluginData;
	m_handleStates.removeAll( hstate );
	delete hstate;
}




void polyb302Synth::db24Toggled( bool )
{
	for( vlist<handleState *>::iterator it = m_handleStates.begin();
					it != m_handleStates.end(); ++it )
	{
		( *it )->db24Toggled();
	}
}




void polyb302Synth::detuneChanged( float )
{
	for( vlist<handleState *>::iterator it = m_handleStates.begin();
					it != m_handleStates.end(); ++it )
	{
		( *it )->detuneChanged();
	}
}




// TODO: Split into one function per knob.  envdecay doesn't require
// recalcFilter.
void polyb302Synth::filterChanged( float )
{
	for( vlist<handleState *>::iterator it = m_handleStates.begin();
					it != m_handleStates.end(); ++it )
	{
		( *it )->filterChanged();
	}
}




// TODO: Set m_vco_shape in here.
void polyb302Synth::waveChanged( float ) 
{
	switch( (int)rint( m_wave_knob->value() ) )
	{
		case 0:
			m_wave_knob->setHintText( tr( "Sawtooth " ), "" );
			break;
		case 1:
			m_wave_knob->setHintText( tr( "Inverted Sawtooth " ),
									"" );
			break;
		case 2:
			m_wave_knob->setHintText( tr( "Triangle " ), "" );
			break;
		case 3:
			m_wave_knob->setHintText( tr( "Square " ), "" );
			break;
		case 4:
			m_wave_knob->setHintText( tr( "Rounded Square " ), "" );
			break;
		case 5:
			m_wave_knob->setHintText( tr( "Moog " ), "" );
			break;
	}
}








polyb302Synth::handleState::handleState( const polyb302Synth * _synth )
{
	m_vco_inc = 0.0;
	m_vco_c = 0;
	m_vco_k = 0;

	m_vco_slide = 0;
	m_vco_slideinc = 0;

	m_fs.cutoff = 0;
	m_fs.envmod = 0;
	m_fs.reso = 0;
	m_fs.envdecay = 0;
	m_fs.dist = 0;

	m_vcf_envpos = ENVINC;
	m_vco_detune = 0;

	m_vca_mode = 2;
	m_vca_a = 0;
	//m_vca_attack = 1.0 - 0.94406088;
	m_vca_attack = 1.0 - 0.96406088;
	m_vca_decay = 0.99897516;

	m_vco_shape = SAWTOOTH;

	// Experimenting between original (0.5) and 1.0
	m_vca_a0 = 0.5;

	if( _synth->m_db24Toggle->isChecked() )
	{
		m_vcf = new lb302Filter3Pole( &m_fs );
	}
	else
	{
		m_vcf = new lb302FilterIIR2( &m_fs );
	}
	recalcFilter();

	m_lastFramesPlayed = 0;

	m_synth = _synth;

	filterChanged();
	detuneChanged();
}




polyb302Synth::handleState::~handleState()
{
	delete m_vcf;
}




void polyb302Synth::handleState::db24Toggled( void )
{
	delete m_vcf;
	if( m_synth->m_db24Toggle->isChecked() )
	{
		m_vcf = new lb302Filter3Pole( &m_fs );
	}
	else
	{
		m_vcf = new lb302FilterIIR2( &m_fs );
	}
	recalcFilter();
}




void polyb302Synth::handleState::detuneChanged( void )
{
	m_vco_detune = powf( 2.0f,
				(float)m_synth->m_vco_fine_detune_knob->value()
								/ 1200.0f );
	m_vco_inc = m_vco_detune / engine::getMixer()->sampleRate();

	// If a slide note is pending,
	if( m_vco_slideinc )
	{
		m_vco_slideinc = m_vco_inc;
	}

	// If currently sliding,
	// May need to rescale m_vco_slide as well
	if( m_vco_slide )
	{
		m_vco_slidebase = m_vco_detune
					/ engine::getMixer()->sampleRate();
	}
}




// TODO: Split into one function per knob.  envdecay doesn't require
// recalcFilter.
void polyb302Synth::handleState::filterChanged( void )
{
        m_fs.cutoff = m_synth->m_vcf_cut_knob->value();
        m_fs.reso   = m_synth->m_vcf_res_knob->value();
        m_fs.envmod = m_synth->m_vcf_mod_knob->value();
        m_fs.dist   = LB_DIST_RATIO * m_synth->m_dist_knob->value();

        float d = 0.2 + 2.3 * m_synth->m_vcf_dec_knob->value();
        d *= engine::getMixer()->sampleRate();
	// decay is 0.1 to the 1/d * ENVINC
        m_fs.envdecay = pow( 0.1, ENVINC / d );
	// vcf_envdecay is now adjusted for both
	// sampling rate and ENVINC
        recalcFilter();
}




// OBSOLETE. Break apart once we get Q_OBJECT to work. >:[
void polyb302Synth::handleState::recalcFilter( void )
{
	m_vcf->recalc();

    // THIS IS OLD 3pole/24dB code, I may reintegrate it.  Don't need it
    // right now.   Should be toggled by LB_24_RES_TRICK at the moment.

    /*kfcn = 2.0 * (((vcf_cutoff*3000))) / m_LB_HZ;
    kp   = ((-2.7528*kfcn + 3.0429)*kfcn + 1.718)*kfcn - 0.9984;
    kp1  = kp+1.0;
    kp1h = 0.5*kp1;
    kres = (((vcf_reso))) * (((-2.7079*kp1 + 10.963)*kp1 - 14.934)*kp1 + 8.4974);
    value = 1.0+( (((0))) *(1.5 + 2.0*kres*(1.0-kfcn))); // ENVMOD was DIST*/

	m_vcf_envpos = ENVINC; // Trigger filter update in process()
}




void polyb302Synth::handleState::process( sampleFrame * _outbuf,
							const Uint32 _size,
							float _freq )
{
	for( Uint32 i = 0; i < _size; i++ )
	{
		// update m_vcf
		if( m_vcf_envpos >= ENVINC )
		{
			m_vcf->envRecalc();       

			m_vcf_envpos = 0;

			if( m_vco_slide )
			{
				m_vco_inc = m_vco_slidebase - m_vco_slide;
				// Calculate coeff from dec_knob on knob change.
				// TODO: Adjust for ENVINC
				m_vco_slide *= 0.9
					+ ( m_synth->m_slide_dec_knob->value()
								* 0.0999 );
			}
		}

		m_sample_cnt++;
		m_vcf_envpos++;

		// 01/21/07 Changed to VCF -> VCA instead of VCA -> VCF
#ifdef LB_FILTERED
		float samp = m_vcf->process( m_vco_k ) * 2.0 * m_vca_a;
#else
		float samp = m_vco_k * m_vca_a;
#endif

		for( int c = 0; c < DEFAULT_CHANNELS; c++ )
		{
			_outbuf[i][c] = samp;
		}


		// update vco
		m_vco_c += m_vco_inc * _freq;
		if( m_vco_c > 0.5 )
		{
			m_vco_c -= 1.0;
		}

		switch( (int)rint( m_synth->m_wave_knob->value() ) )
		{
			case 0: m_vco_shape = SAWTOOTH; break;
			case 1: m_vco_shape = INVERTED_SAWTOOTH; break;
			case 2: m_vco_shape = TRIANGLE; break;
			case 3: m_vco_shape = SQUARE; break;
			case 4: m_vco_shape = ROUND_SQUARE; break;
			case 5: m_vco_shape = MOOG; break;
			default: m_vco_shape = SAWTOOTH; break;
		}

		// add m_vco_shape_param the changes the shape of each curve.
		// merge sawtooths with triangle and square with round square?
		switch( m_vco_shape )
		{
			case SAWTOOTH: // p0: curviness of line
				// Is this sawtooth backwards?
				m_vco_k = m_vco_c;
				break;

			case INVERTED_SAWTOOTH: // p0: curviness of line
				// Is this sawtooth backwards?
				m_vco_k = -m_vco_c;
				break;

			// TODO: I think TRIANGLE is broken.
			// p0: duty rev.saw<->triangle<->saw p1: curviness
			case TRIANGLE:
				m_vco_k = m_vco_c * 2.0 + 0.5;
				if( m_vco_k > 0.5 )
				{
					m_vco_k = 1.0 - m_vco_k;
				}
				break;

			case SQUARE: // p0: slope of top
				m_vco_k = ( m_vco_c < 0 ) ? 0.5 : -0.5;
				break;

			case ROUND_SQUARE: // p0: width of round
				m_vco_k = ( m_vco_c < 0 ) ?
					sqrtf( 1 - m_vco_c * m_vco_c * 4 )
									- 0.5 :
					-0.5;
				break;

			// Maybe the fall should be exponential/sinsoidal
			// instead of quadric.
			case MOOG:
				// [-0.5, 0]: Rise, [0,0.25]: Slope down,
				// [0.25,0.5]: Low
				m_vco_k = m_vco_c * 2.0 + 0.5;
				if( m_vco_k > 1.0 )
				{
					m_vco_k = -0.5;
				}
				else if( m_vco_k > 0.5 )
				{
					float w = 2 * ( m_vco_k - 0.5 ) - 1;
					m_vco_k = 0.5 - sqrtf( 1 - w * w );
					// MOOG wave gets filtered away
					m_vco_k *= 2.0;
				}
				break;
		}

		// Make it louder. For the better?
		//m_vco_k*=2.0;

		// Handle Envelope
		// TODO: Add decay once I figure out how to extend past the end
		// of a note.
		if( m_sample_cnt >= 0.5 * engine::getMixer()->sampleRate() )
		{
			m_vca_mode = 2;
		}
		if( m_vca_mode == 0 )
		{
			m_vca_a += ( m_vca_a0 - m_vca_a ) * m_vca_attack;
		}
		else if( m_vca_mode == 1 )
		{
			m_vca_a *= m_vca_decay;
			// the following line actually speeds up processing
			if( m_vca_a < 1 / 65536.0 )
			{
				m_vca_a = 0;
				m_vca_mode = 2;
			}
		}

	}
}
