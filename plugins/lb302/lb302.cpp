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

#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>

#else

#include <qdom.h>

#endif


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
//#define LB_FILTERED 
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
				"Incomplete monophonic immitation tb303" ),
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

lb302Filter::lb302Filter(lb302FilterState* p_fs) : 
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
    vcf_e0*=M_PI/44100.0;
    vcf_e1*=M_PI/44100.0;
    vcf_e1 -= vcf_e0;
};

void lb302Filter::envRecalc()
{
    vcf_c0 *= fs->envdecay;       // Filter Decay. vcf_decay is adjusted for Hz and ENVINC
    vcf_rescoeff = exp(-1.20 + 3.455*(fs->reso));
};

void lb302Filter::playNote()
{
    vcf_c0 = vcf_e1;
}

//
// lb302FilterIIR2
//

lb302FilterIIR2::lb302FilterIIR2(lb302FilterState* p_fs) :
    lb302Filter(p_fs),
    vcf_d1(0),
    vcf_d2(0),
    vcf_a(0),
    vcf_b(0),
    vcf_c(1)
{

	m_dist = new effectLib::distortion<>( 1.0, 1.0f);
	
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
    k = exp(-w/vcf_rescoeff);
                                  // Does this mean c0 is inheritantly?
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

lb302Filter3Pole::lb302Filter3Pole(lb302FilterState *p_fs) :
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


    w = vcf_e0 + vcf_c0;          // e0 is adjusted for Hz and doesn't need ENVINC
    k = (fs->cutoff > 0.975)?0.975:fs->cutoff;
    kfco = 50.f+(k)*((2300.f-1600.f*(fs->envmod))+(w)*(700.f+1500.f*(k)+(1500.f+(k)*(44100.f/2.f-6000.f))*(fs->envmod)));
        //+iacc*(.3+.7*kfco*kenvmod)*kaccent*kaccurve*2000




#ifdef LB_24_IGNORE_ENVELOPE
    // kfcn = fs->cutoff;
    kfcn = 2.0 * kfco / LB_HZ;
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
    value = 1.0+( (fs->dist) *(1.5 + 2.0*kres*(1.0-kfcn))); // ENVMOD was DIST*/
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

lb302Synth::lb302Synth( instrumentTrack * _channel_track ) :
	instrument( _channel_track, &lb302_plugin_descriptor )
{
    // GUI

	vcf_cut_knob = new knob( knobBright_26, this, tr( "VCF Cutoff Frequency" ), 
            _channel_track );
	vcf_cut_knob->setRange( 0.0f, 1.5f, 0.005f );   // Originally  [0,1.0]
 	vcf_cut_knob->setInitValue( 0.75f );
	vcf_cut_knob->move( 75, 130 );
	vcf_cut_knob->setHintText( tr( "Cutoff Freq:" ) + " ", "" );
    vcf_cut_knob->setLabel( tr("CUT") );

	vcf_res_knob = new knob( knobBright_26, this, tr( "VCF Resonance" ),
							_channel_track );
	vcf_res_knob->setRange( 0.0f, 1.25f, 0.005f );   // Originally [0,1.0]
	vcf_res_knob->setInitValue( 0.75f );
	vcf_res_knob->move( 120, 130 );
	vcf_res_knob->setHintText( tr( "Resonance:" ) + " ", "" );
    vcf_res_knob->setLabel( tr("RES") );

	vcf_mod_knob = new knob( knobBright_26, this, tr( "VCF Envelope Mod" ), 
            _channel_track );
	vcf_mod_knob->setRange( 0.0f, 1.0f, 0.005f );   // Originally  [0,1.0]
 	vcf_mod_knob->setInitValue( 1.0f );
	vcf_mod_knob->move( 165, 130 );
	vcf_mod_knob->setHintText( tr( "Env Mod:" ) + " ", "" );
    vcf_mod_knob->setLabel( tr("ENV MOD") );

	vcf_dec_knob = new knob( knobBright_26, this, tr( "VCF Envelope Decay" ),
							_channel_track );
	vcf_dec_knob->setRange( 0.0f, 1.0f, 0.005f );   // Originally [0,1.0]
	vcf_dec_knob->setInitValue( 0.1f );
	vcf_dec_knob->move( 210, 130 );
	vcf_dec_knob->setHintText( tr( "Decay:" ) + " ", "" );
    vcf_dec_knob->setLabel( tr("DEC") );

     slideToggle = new ledCheckBox( "Slide", this,
							tr( "Slide" ),
							_channel_track );
	slideToggle->move( 10, 180 );


    accentToggle = new ledCheckBox( "Accent", this,
							tr( "Accent" ),
							_channel_track );
	accentToggle->move( 10, 200 );
    accentToggle->setDisabled(true);


    deadToggle = new ledCheckBox( "Dead", this,
							tr( "Dead" ),
							_channel_track );
	deadToggle->move( 10, 220 );

    db24Toggle = new ledCheckBox( "24dB/oct", this,
                            tr( "303-es-que, 24dB/octave, 3 pole filter" ),
                            _channel_track );
    db24Toggle->move( 10, 150);


	slide_dec_knob = new knob( knobBright_26, this, tr( "Slide Decay" ),
							_channel_track );
	slide_dec_knob->setRange( 0.0f, 1.0f, 0.005f );   // Originally [0,1.0]
	slide_dec_knob->setInitValue( 0.6f );
	slide_dec_knob->move( 210, 75 );
	slide_dec_knob->setHintText( tr( "Slide Decay:" ) + " ", "" );
    slide_dec_knob->setLabel( tr( "SLIDE"));

    vco_fine_detune_knob = new knob( knobBright_26, this, 
            tr("Fine detuning of the VCO. Ranged between -100 and 100 centes."),
            _channel_track );
    vco_fine_detune_knob->setRange(-100.0f, 100.0f, 1.0f);
    vco_fine_detune_knob->setInitValue(0.0f);
    vco_fine_detune_knob->move(165,75);
    vco_fine_detune_knob->setHintText( tr( "VCO Fine Detuning:") + " ", "cents");
    vco_fine_detune_knob->setLabel( tr( "DETUNE"));


    dist_knob = new knob( knobBright_26, this, tr( "Distortion" ),
							_channel_track );
	dist_knob->setRange( 0.0f, 1.0f, 0.01f );   // Originally [0,1.0]
	dist_knob->setInitValue( 0.0f );
	dist_knob->move( 210, 190 );
	dist_knob->setHintText( tr( "DIST:" ) + " ", "" );
    dist_knob->setLabel( tr( "DIST"));


    wave_knob = new knob( knobBright_26, this, tr( "Waveform" ),
							_channel_track );
	wave_knob->setRange( 0.0f, 5.0f, 1.0f );   // Originally [0,1.0]
	wave_knob->setInitValue( 0.0f );
	wave_knob->move( 120, 75 );
	wave_knob->setHintText( tr( "WAVE:" ) + " ", "" );
    wave_knob->setLabel( tr( "WAVE"));


    connect( vcf_cut_knob, SIGNAL( valueChanged( float ) ),
		this, SLOT ( filterChanged( float ) ) );

    connect( vcf_res_knob, SIGNAL( valueChanged( float ) ),
		this, SLOT ( filterChanged( float ) ) );

    connect( vcf_mod_knob, SIGNAL( valueChanged( float ) ),
		this, SLOT ( filterChanged( float ) ) );

    connect( vcf_dec_knob, SIGNAL( valueChanged( float ) ),
		this, SLOT ( filterChanged( float ) ) );

    connect( vco_fine_detune_knob, SIGNAL( valueChanged( float ) ),
        this, SLOT ( detuneChanged( float) ) );

    connect( db24Toggle, SIGNAL( toggled( bool ) ),
        this, SLOT ( db24Toggled( bool) ) );

    connect( dist_knob, SIGNAL( valueChanged(float) ),
            this, SLOT ( filterChanged( float )));

    connect( wave_knob, SIGNAL( valueChanged(float) ),
            this, SLOT ( waveChanged( float )));

#ifdef QT4
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif


    // SYNTH

    vco_inc = 0.0;
	vco_c = 0;
	vco_k = 0;
	
    vco_slide = 0; vco_slideinc = 0;

	fs.cutoff = 0; fs.envmod = 0;
	fs.reso = 0; fs.envdecay = 0;
    fs.dist = 0;

	vcf_envpos = ENVINC;
    vco_detune = 0;

	vca_mode = 0;  vca_a = 0;   // Start VCA on an attack.

	//vca_attack = 1.0 - 0.94406088;      
	vca_attack = 1.0 - 0.96406088;      
	vca_decay = 0.99897516;            
    
    vco_shape = SAWTOOTH; 

	vca_a0 = 0.5;                       // Experimenting between original (0.5) and 1.0

    vcf = new lb302FilterIIR2(&fs);

    recalcFilter();

    lastFramesPlayed = 1;	// because we subtract 1 later
    last_offset = 0;

    period_states = NULL;
    period_states_cnt = 0;
    note_count = 0;

    filterChanged(0.0);
    detuneChanged(0.0);
}




lb302Synth::~lb302Synth()
{
    delete vcf;
}




void lb302Synth::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	vcf_cut_knob->saveSettings( _doc, _this, "vcf_cut" );
	vcf_res_knob->saveSettings( _doc, _this, "vcf_res" );
	vcf_mod_knob->saveSettings( _doc, _this, "vcf_mod" );
	vcf_dec_knob->saveSettings( _doc, _this, "vcf_dec" );

    vco_fine_detune_knob->saveSettings( _doc, _this, "vco_detune" );
    wave_knob->saveSettings( _doc, _this, "shape");
    dist_knob->saveSettings( _doc, _this, "dist");
    slide_dec_knob->saveSettings( _doc, _this, "slide_dec");

    slideToggle->saveSettings( _doc, _this, "slide");
    deadToggle->saveSettings( _doc, _this, "dead");
    db24Toggle->saveSettings( _doc, _this, "db24");
}




void lb302Synth::loadSettings( const QDomElement & _this )
{
	vcf_cut_knob->loadSettings( _this, "vcf_cut" );
	vcf_res_knob->loadSettings( _this, "vcf_res" );
	vcf_mod_knob->loadSettings( _this, "vcf_mod" );
	vcf_dec_knob->loadSettings( _this, "vcf_dec" );

    vco_fine_detune_knob->loadSettings( _this, "vco_detune" );
    dist_knob->loadSettings( _this, "dist");
    wave_knob->loadSettings( _this, "shape");
    slide_dec_knob->loadSettings( _this, "slide_dec");

    slideToggle->loadSettings( _this, "slide");
    deadToggle->loadSettings( _this, "dead");
    db24Toggle->loadSettings( _this, "db24");

    filterChanged(0.0);
    detuneChanged(0.0);
}

// TODO: Split into one function per knob.  envdecay doesn't require
// recalcFilter.
void lb302Synth::filterChanged( float )
{
        fs.cutoff = vcf_cut_knob->value();
        fs.reso   = vcf_res_knob->value();
        fs.envmod = vcf_mod_knob->value();
        fs.dist   = LB_DIST_RATIO*dist_knob->value();

        float d = 0.2 + (2.3*vcf_dec_knob->value());
        d*=LB_HZ;                                   // d *= smpl rate
        fs.envdecay = pow(0.1, 1.0/d * ENVINC);    // decay is 0.1 to the 1/d * ENVINC
                                                    // vcf_envdecay is now adjusted for both
                                                    // sampling rate and ENVINC
        recalcFilter();
}

void lb302Synth::db24Toggled( bool )
{
    delete vcf;
    if(db24Toggle->isChecked())  {
        vcf = new lb302Filter3Pole(&fs);
    }
    else {
        vcf = new lb302FilterIIR2(&fs);
    }
    recalcFilter();
}


void lb302Synth::detuneChanged( float )
{
    float freq = vco_inc*LB_HZ/vco_detune;
    float slidebase_freq=0;

    if(vco_slide) {
        slidebase_freq = vco_slidebase*LB_HZ/vco_detune;
    }
    
    vco_detune = powf(2.0f, (float)vco_fine_detune_knob->value()/1200.0f);
    vco_inc = freq*vco_detune/LB_HZ;

    // If a slide note is pending,
    if(vco_slideinc)
        vco_slideinc = vco_inc;

    // If currently sliding,
    // May need to rescale vco_slide as well
    if(vco_slide) 
        vco_slidebase=slidebase_freq*vco_detune/LB_HZ;
}

// TODO: Set vco_shape in here.
void lb302Synth::waveChanged( float ) 
{
    switch(int(rint(wave_knob->value()))) {
        case 0: wave_knob->setHintText(tr("Sawtooth "),""); break;
        case 1: wave_knob->setHintText(tr("Inverted Sawtooth "),""); break;
        case 2: wave_knob->setHintText(tr("Triangle "),""); break;
        case 3: wave_knob->setHintText(tr("Square "),""); break;
        case 4: wave_knob->setHintText(tr("Rounded Square "),""); break;
        case 5: wave_knob->setHintText(tr("Moog "),""); break;
    }
}

QString lb302Synth::nodeName( void ) const
{
	return( lb302_plugin_descriptor.name );
}

// OBSOLETE. Break apart once we get Q_OBJECT to work. >:[
void lb302Synth::recalcFilter()
{
    vcf->recalc();

    // THIS IS OLD 3pole/24dB code, I may reintegrate it.  Don't need it
    // right now.   Should be toggled by LB_24_RES_TRICK at the moment.

    /*kfcn = 2.0 * (((vcf_cutoff*3000))) / LB_HZ;
    kp   = ((-2.7528*kfcn + 3.0429)*kfcn + 1.718)*kfcn - 0.9984;
    kp1  = kp+1.0;
    kp1h = 0.5*kp1;
    kres = (((vcf_reso))) * (((-2.7079*kp1 + 10.963)*kp1 - 14.934)*kp1 + 8.4974);
    value = 1.0+( (((0))) *(1.5 + 2.0*kres*(1.0-kfcn))); // ENVMOD was DIST*/

    vcf_envpos = ENVINC; // Trigger filter update in process()
}

inline int MIN(int a, int b) {
    return (a<b)?a:b;
}

int lb302Synth::process(sampleFrame *outbuf, const Uint32 size)
{

	unsigned int i;
	float w;
    float samp;
         
       
    
	for(i=0;i<size;i++) {
        /* TODO: ONLY DO THIS IF WE ARE EDGE-TO-EDGE NON-DEAD */
        /*if (sample_cnt < desiredTransitionFrames()) {
            for(int c=0; c<DEFAULT_CHANNELS; c++)
                outbuf[i][c]=0;
            sample_cnt++;
            continue;
        }
        */

		// update vcf
		if(vcf_envpos >= ENVINC) {
            vcf->envRecalc();       
    
            vcf_envpos = 0;

            if (vco_slide) {
                vco_inc=vco_slidebase-vco_slide;
                // Calculate coeff from dec_knob on knob change.
                vco_slide*= 0.9+(slide_dec_knob->value()*0.0999); // TODO: Adjust for Hz and ENVINC

            }
		}

        sample_cnt++;
		vcf_envpos++;

        float old_vco_k = vco_k;
        bool looking;

		// update vco
	    vco_c += vco_inc;

        if(vco_c > 0.5) vco_c -= 1.0;
/*
        if (catch_decay < desiredTransitionFrames()) {
            catch_decay++;
            continue;
        }*/
    
         
        switch(int(rint(wave_knob->value()))) {
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

            // TODO: I think TRIANGLE is broken.
            case TRIANGLE:  // p0: duty rev.saw<->triangle<->saw p1: curviness
                vco_k = (vco_c*2.0)+0.5;
                if (vco_k>0.5) vco_k = 1.0-vco_k;
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
                if (vco_k>1.0) vco_k = -0.5 ;
                else if (vco_k>0.5) {
                    w = 2.0*(vco_k-0.5)-1.0;
                    vco_k = 0.5 - sqrtf(1.0-(w*w));
                }
                vco_k *= 2.0;  // MOOG wave gets filtered away 
                break;
        }

	vca_a = 0.5;
        // Write out samples.
#ifdef LB_FILTERED
        samp = vcf->process(vco_k)*2.0*vca_a;
#else
        samp = vco_k*vca_a;
#endif
        /*
        float releaseFrames = desiredReleaseFrames();
        samp *= (releaseFrames - catch_decay)/releaseFrames;
        */

        for(int c=0; c<DEFAULT_CHANNELS; c++) {
            outbuf[i][c]=samp;
        }

        // Store state
        period_states[i].vco_c = vco_c;
        period_states[i].vca_a = vca_a;             // Doesn't change anything (currently)
        period_states[i].sample_cnt = sample_cnt;   // Doesn't change anything (currently)


        // Handle Envelope
        // TODO: Add decay once I figure out how to extend past the end of a note.
		if(vca_mode==0) {
            vca_a+=(vca_a0-vca_a)*vca_attack;
	    	if(sample_cnt>=0.5*44100) 
                vca_mode = 2;
        }
		else if(vca_mode == 1) {
			vca_a *= vca_decay;
            //printf("VCA: %d %f\n", dbgshit++, vca_a);
            
			// the following line actually speeds up processing
			if(vca_a < (1/65536.0)) { vca_a = 0; vca_mode = 2; }
		}
        
	}
	return 1;
}

/*  Prepares the active LB302 note.  I separated this into a function because it
 *  needs to be called on playNote() when a new note is started.  It also needs
 *  to be called from process() when a prior edge-to-edge note is done releasing.
 */
void lb302Synth::initNote( lb302Note *n)
{
    catch_decay = 0;

    vco_inc = n->vco_inc;
    
    // TODO: Try moving to the if() below
    if(n->dead == 0) {
        sample_cnt = 0;
        vca_mode = 0;  vca_a = 0;
    }

        // Initiate Slide
    // TODO: Break out into function, should be called again on detuneChanged
    if (vco_slideinc) {
        vco_slide = vco_inc-vco_slideinc;
        vco_slidebase = vco_inc;
        vco_slideinc = 0;
    }
    else {
        vco_slide = 0;
    }
    // End break-out

    // Slide note, save inc for next note
    if (slideToggle->value()) {
        vco_slideinc = vco_inc; // May need to equal vco_slidebase+vco_slide if last note slid
    }


    recalcFilter();
    
    if(n->dead ==0){
        // Swap next two blocks??
        vcf->playNote();
        // Ensure envelope is recalculated
        vcf_envpos = ENVINC;

        // Double Check 
        vca_mode = 0;
        vca_a = 0.0;
    }
}


void lb302Synth::playNote( notePlayHandle * _n, bool )
{
	fpp_t framesPerPeriod = engine::getMixer()->framesPerPeriod();

	///=== WEIRD CODE FOR MONOPHONIC BEHAVIOUR - BEGIN === ///

	// number of frames to play - only modified below if we have to play
	// the rest of an old note
	fpp_t frames = _n->framesLeftForCurrentPeriod();

	// per default we resume at the last played frames - only in
	// some special-cases (which we catch below) we have to resume
	// somewhere else
	f_cnt_t resume_pos = lastFramesPlayed-1;

	// find out which situation we're in
	constNotePlayHandleVector v =
		notePlayHandle::nphsOfInstrumentTrack( getInstrumentTrack(), TRUE );
	// more than one note running?
	if( v.count() > 1 )
	{
		const notePlayHandle * on = v.first();	// oldest note
		const notePlayHandle * yn = v.last();	// youngest note
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
		if( _n != yn )
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
				_n->getInstrumentTrack()->getAudioPort()->
								firstBuffer(),
				framesPerPeriod - yn->offset(), yn->offset() );
			resume_pos = yn->offset() - on->offset() - 1;
			// make sure we have positive value, otherwise we're
			// accessing states out of borders
			while( resume_pos < 0 )
			{
				resume_pos += framesPerPeriod;
			}
	        }
	}

#ifdef LB_DEBUG
	if( _n->released() )
	{
		printf( "    RELEASED!!!   %ld\n", _n );
	}
	else
	{
		printf( "not released...   %ld\n", _n );
	}
	printf( "offset: %d   frames:%d\n", _n->offset(), frames );

	printf( "Resuming at %d\n", resume_pos );
#endif

	///=== WEIRD CODE FOR MONOPHONIC BEHAVIOUR - END === ///

	/// Malloc our period history buffer
	if (period_states == NULL) 
		period_states = new lb302State[framesPerPeriod];


    // now resume at the proper position and process as usual
    lb302State *state = &period_states[resume_pos];

    /// Actually resume the state, now that we have the right state object.
    vco_c = state->vco_c;
    vca_a = state->vca_a;
    sample_cnt = state->sample_cnt;



    /// Currently have release/decay disabled
    // Start the release decay if this is the first release period.
    //if (_n->released() && catch_decay == 0)
    //        catch_decay = 1;
    

    if ( _n->totalFramesPlayed() <= 0 ) {
        /// This code is obsolete, hence the "if false"

        // Existing note. Allow it to decay. 
        if(/*note_count*/ false) {
            // BEGIN NOT SURE OF...
            //lb302State *st = &period_states[period_states_cnt-1];
            //vca_a = st->vca_a;
            //sample_cnt = st->sample_cnt;
            // END NOT SURE OF

            // Reserve this note for retrigger in process()
            hold_note.vco_inc = _n->frequency()*vco_detune/LB_HZ;  // TODO: Use actual sampling rate.
            hold_note.dead = deadToggle->value();
            use_hold_note = true;
        }
        /// Start a new note.
        else {
            lb302Note note;
            note.vco_inc = _n->frequency()*vco_detune/LB_HZ;  // TODO: Use actual sampling rate.
            note.dead = deadToggle->value();
            initNote(&note);
            use_hold_note = false;
        }

        note_count=1;
    }
    
	sampleFrame *buf = new sampleFrame[frames];

        process(buf, frames); 
        getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

        delete[] buf;

	lastFramesPlayed = frames;//_n->framesLeftForCurrentPeriod(); //_n->totalFramesPlayed();
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


