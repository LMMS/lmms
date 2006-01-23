/*
 * envelope_tab_widget.cpp - widget for use in envelope/lfo/filter-tab of
 *                           channel-window
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QWhatsThis>

#else

#include <qdom.h>
#include <qwhatsthis.h>

#endif


#include "envelope_tab_widget.h"
#include "envelope_and_lfo_widget.h"
#include "note_play_handle.h"
#include "knob.h"
#include "pixmap_button.h"
#include "group_box.h"
#include "tab_widget.h"
#include "embed.h"
#include "gui_templates.h"
#include "channel_track.h"
#include "combobox.h"



const int TARGETS_TABWIDGET_X = 4;
const int TARGETS_TABWIDGET_Y = 5;
const int TARGETS_TABWIDGET_WIDTH = 238;
const int TARGETS_TABWIDGET_HEIGTH = 175;

const int FILTER_GROUPBOX_X = TARGETS_TABWIDGET_X;
const int FILTER_GROUPBOX_Y = TARGETS_TABWIDGET_Y+TARGETS_TABWIDGET_HEIGTH+5;
const int FILTER_GROUPBOX_WIDTH = TARGETS_TABWIDGET_WIDTH;
const int FILTER_GROUPBOX_HEIGHT = 245-FILTER_GROUPBOX_Y;

const float CUT_FREQ_MULTIPLIER = 6000.0f;
const float RES_MULTIPLIER = 2.0f;
const float RES_PRECISION = 1000.0f;


// names for env- and lfo-targets - first is name being displayed to user
// and second one is used internally, e.g. for saving/restoring settings
static const QString targetNames[envelopeTabWidget::TARGET_COUNT][2] =
{
	{ envelopeTabWidget::tr( "VOLUME" ), "vol" },
/*	envelopeTabWidget::tr( "Pan" ),
	envelopeTabWidget::tr( "Pitch" ),*/
	{ envelopeTabWidget::tr( "CUTOFF" ), "cut" },
	{ envelopeTabWidget::tr( "Q/RESO" ), "res" }
} ;
 


envelopeTabWidget::envelopeTabWidget( channelTrack * _channel_track ) :
	QWidget( _channel_track->tabWidgetParent() ),
	settings()
{

	m_targetsTabWidget = new tabWidget( tr( "TARGET" ), this );
	m_targetsTabWidget->setGeometry( TARGETS_TABWIDGET_X,
						TARGETS_TABWIDGET_Y,
						TARGETS_TABWIDGET_WIDTH,
						TARGETS_TABWIDGET_HEIGTH );
#ifdef QT4
	m_targetsTabWidget->setWhatsThis(
#else
	QWhatsThis::add( m_targetsTabWidget,
#endif
		tr( "These tabs contain envelopes. They're very important for "
			"modifying a sound, for not saying that they're almost "
			"always neccessary for substractive synthesis. For "
			"example if you have a volume-envelope, you can set "
			"when the sound should have which volume-level. "
			"Maybe you want to create some soft strings. Then your "
			"sound has to fade in and out very softly. This can be "
			"done by setting a large attack- and release-time. "
			"It's the same for other envelope-targets like "
			"panning, cutoff-frequency of used filter and so on. "
			"Just monkey around with it! You can really make cool "
			"sounds out of a saw-wave with just some "
			"envelopes...!" ) );

	for( int i = 0; i < TARGET_COUNT; ++i )
	{
		float value_for_zero_amount = 0.0;
		if( i == VOLUME )
		{
			value_for_zero_amount = 1.0;
		}
		m_envLFOWidgets[i] = new envelopeAndLFOWidget(
				value_for_zero_amount, m_targetsTabWidget );
		m_targetsTabWidget->addTab( m_envLFOWidgets[i],
						tr( targetNames[i][0]
#ifdef QT4
						.toAscii().constData()
#endif
						) );
/*
#ifdef QT4
											.toAscii().constData()
#endif
						) );*/
	}
	
	
	m_filterGroupBox = new groupBox( tr( "FILTER" ), this );
	m_filterGroupBox->setGeometry( FILTER_GROUPBOX_X, FILTER_GROUPBOX_Y,
						FILTER_GROUPBOX_WIDTH,
						FILTER_GROUPBOX_HEIGHT );

	m_filterComboBox = new comboBox( m_filterGroupBox );
	m_filterComboBox->setGeometry( 14, 22, 120, 22 );
	m_filterComboBox->setFont( pointSize<8>( m_filterComboBox->font() ) );


	m_filterComboBox->addItem( tr( "LowPass" ),
					embed::getIconPixmap( "filter_lp" ) );
	m_filterComboBox->addItem( tr( "HiPass" ),
					embed::getIconPixmap( "filter_hp" ) );
	m_filterComboBox->addItem( tr( "BandPass csg" ),
					embed::getIconPixmap( "filter_bp" ) );
	m_filterComboBox->addItem( tr( "BandPass czpg" ),
					embed::getIconPixmap( "filter_bp" ) );
	m_filterComboBox->addItem( tr( "Notch" ),
				embed::getIconPixmap( "filter_notch" ) );
	m_filterComboBox->addItem( tr( "Allpass" ),
					embed::getIconPixmap( "filter_ap" ) );
	m_filterComboBox->addItem( tr( "Moog" ),
					embed::getIconPixmap( "filter_lp" ) );
	m_filterComboBox->addItem( tr( "2x LowPass" ),
					embed::getIconPixmap( "filter_2lp" ) );

#ifdef QT4
	m_filterComboBox->setWhatsThis(
#else
	QWhatsThis::add( m_filterComboBox,
#endif
		tr( "Here you can select the built-in filter you want to use "
			"in this channel. Filters are very important for "
			"changing the characteristics of a sound." ) );


	m_filterCutKnob = new knob( knobBright_26, m_filterGroupBox, tr(
							"cutoff-frequency" ) );
	m_filterCutKnob->setLabel( tr( "CUTOFF" ) );
	m_filterCutKnob->setRange( 0.0, 16000.0, 1.0 );
	m_filterCutKnob->move( 140, 18 );
	m_filterCutKnob->setValue( 16000.0, TRUE );
	m_filterCutKnob->setHintText( tr( "cutoff-frequency:" ) + " ", " " +
								tr( "Hz" ) );
#ifdef QT4
	m_filterCutKnob->setWhatsThis(
#else
	QWhatsThis::add( m_filterCutKnob,
#endif
		tr( "Use this knob for setting the cutoff-frequency for the "
			"selected filter. The cutoff-frequency specifies the "
			"frequency for cutting the signal by a filter. For "
			"example a lowpass-filter cuts all frequencies above "
			"the cutoff-frequency. A highpass-filter cuts all "
			"frequencies below cutoff-frequency and so on..." ) );

	m_filterResKnob = new knob( knobBright_26, m_filterGroupBox, tr(
							"Q/Resonance" ) );
	m_filterResKnob->setLabel( tr( "Q/RESO" ) );
	m_filterResKnob->setRange( 0.01, 10.0, 0.01 );
	m_filterResKnob->move( 190, 18 );
	m_filterResKnob->setValue( 0.5, TRUE );
	m_filterResKnob->setHintText( tr( "Q/Resonance:" ) + " ", "" );
#ifdef QT4
	m_filterResKnob->setWhatsThis(
#else
	QWhatsThis::add( m_filterResKnob,
#endif
		tr( "Use this knob for setting Q/Resonance for the selected "
			"filter. Q/Resonance tells the filter, how much it "
			"should amplify frequencies near Cutoff-frequency." ) );

}




envelopeTabWidget::~envelopeTabWidget()
{
	delete m_targetsTabWidget;
}




float FASTCALL envelopeTabWidget::volumeLevel( notePlayHandle * _n,
								Uint32 _frame )
{
	Uint32 release_begin = _frame - _n->releaseFramesDone() +
						_n->framesBeforeRelease();

	if( _n->released() == FALSE )
	{
		release_begin += mixer::inst()->framesPerAudioBuffer();
	}

	return( m_envLFOWidgets[VOLUME]->level( _frame, release_begin, 0 ) );
}




void envelopeTabWidget::processAudioBuffer( sampleFrame * _ab, Uint32 _frames,
							notePlayHandle * _n )
{
	Uint32 total_frames = _n->totalFramesPlayed();
	Uint32 release_begin = total_frames - _n->releaseFramesDone() +
						_n->framesBeforeRelease();

	if( _n->released() == FALSE )
	{
		release_begin += mixer::inst()->framesPerAudioBuffer();
	}

	// because of optimizations, there's special code for several cases:
	// 	- volume-, cut- and res-lfo/envelope active
	// 	- volume- and cut-lfo/envelope active
	// 	- volume- and res-lfo/envelope active
	// 	- cut-lfo/envelope active
	// 	- res-lfo/envelope active
	// 	- volume-lfo/envelope active
	//	- no lfo/envelope active but filter is used
	// now there's a lot of similar code but I didn't found a way to
	// generalize it yet... may be later we could do that
	// by using preprocessor and macro-expansion... (like in oscillator.cpp)

	// only use filter, if it is really needed

	if( _n->m_filter == NULL )
	{
		_n->m_filter = new basicFilters<>(
						mixer::inst()->sampleRate() );
	}

	if( m_filterGroupBox->isActive() )
	{
		int old_filter_cut = 0;
		int old_filter_res = 0;

		basicFilters<>::filterTypes filter =
				basicFilters<>::getFilterType(
					m_filterComboBox->currentIndex() );

		if( m_envLFOWidgets[VOLUME]->used() &&
			m_envLFOWidgets[CUT]->used() &&
			m_envLFOWidgets[RES]->used() )
		{
			for( Uint32 frame = 0; frame < _frames;
						++frame, ++total_frames )
			{
				float new_cut_val = m_envLFOWidgets[CUT]->level( total_frames, release_begin, frame );
				new_cut_val = envelopeAndLFOWidget::expKnobVal( new_cut_val ) * CUT_FREQ_MULTIPLIER +
						m_filterCutKnob->value();

				float new_res_val = m_filterResKnob->value() + RES_MULTIPLIER *
							m_envLFOWidgets[RES]->level( total_frames, release_begin, frame );

				if( static_cast<int>( new_cut_val ) != old_filter_cut ||
					static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					_n->m_filter->calcFilterCoeffs( filter, new_cut_val, new_res_val );
					old_filter_cut = static_cast<int>( new_cut_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				float vol_level = m_envLFOWidgets[VOLUME]->level( total_frames, release_begin, frame );
				vol_level = vol_level*vol_level;

				for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = vol_level * _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
		else if( m_envLFOWidgets[VOLUME]->used() && m_envLFOWidgets[CUT]->used() )
		{
			for( Uint32 frame = 0; frame < _frames; ++frame, ++total_frames )
			{
				float new_cut_val = m_envLFOWidgets[CUT]->level( total_frames, release_begin, frame );
				new_cut_val = envelopeAndLFOWidget::expKnobVal( new_cut_val ) * CUT_FREQ_MULTIPLIER +
						m_filterCutKnob->value();

				if( static_cast<int>( new_cut_val ) != old_filter_cut )
				{
					_n->m_filter->calcFilterCoeffs( filter, new_cut_val, m_filterResKnob->value() );
					old_filter_cut = static_cast<int>( new_cut_val );
				}

				float vol_level = m_envLFOWidgets[VOLUME]->level( total_frames, release_begin, frame );
				vol_level = vol_level*vol_level;

				for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = vol_level * _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
		else if( m_envLFOWidgets[VOLUME]->used() && m_envLFOWidgets[RES]->used() )
		{

			for( Uint32 frame = 0; frame < _frames; ++frame, ++total_frames )
			{
				float new_res_val = m_filterResKnob->value() + RES_MULTIPLIER *
							m_envLFOWidgets[RES]->level( total_frames, release_begin, frame );

				if( static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					_n->m_filter->calcFilterCoeffs( filter, m_filterCutKnob->value(), new_res_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				float vol_level = m_envLFOWidgets[VOLUME]->level( total_frames, release_begin, frame );
				vol_level = vol_level*vol_level;

				for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = vol_level * _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}

		}
		else if( m_envLFOWidgets[CUT]->used() )
		{
			for( Uint32 frame = 0; frame < _frames; ++frame, ++total_frames )
			{
				float new_cut_val = m_envLFOWidgets[CUT]->level( total_frames, release_begin, frame );
				new_cut_val = envelopeAndLFOWidget::expKnobVal( new_cut_val ) * CUT_FREQ_MULTIPLIER +
						m_filterCutKnob->value();

				if( static_cast<int>( new_cut_val ) != old_filter_cut )
				{
					_n->m_filter->calcFilterCoeffs( filter, new_cut_val, m_filterResKnob->value() );
					old_filter_cut = static_cast<int>( new_cut_val );
				}

				for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
		else if( m_envLFOWidgets[RES]->used() )
		{
			for( Uint32 frame = 0; frame < _frames; ++frame, ++total_frames )
			{
				float new_res_val = m_filterResKnob->value() + RES_MULTIPLIER *
							m_envLFOWidgets[RES]->level( total_frames, release_begin, frame );

				if( static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					_n->m_filter->calcFilterCoeffs( filter, m_filterCutKnob->value(), new_res_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
		else if( m_envLFOWidgets[VOLUME]->used() )
		{
			_n->m_filter->calcFilterCoeffs( filter, m_filterCutKnob->value(), m_filterResKnob->value() );

			for( Uint32 frame = 0; frame < _frames; ++frame, ++total_frames )
			{
				float vol_level = m_envLFOWidgets[VOLUME]->level( total_frames, release_begin, frame );
				vol_level = vol_level*vol_level;

				for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = vol_level * _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
		else
		{
			_n->m_filter->calcFilterCoeffs( filter, m_filterCutKnob->value(), m_filterResKnob->value() );

			for( Uint32 frame = 0; frame < _frames; ++frame, ++total_frames )
			{
				for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
	}
	else if( m_envLFOWidgets[VOLUME]->used() /*&& m_envLFOWidgets[PANNING]->used() == FALSE*/ )
	{
		// only use volume-envelope...
		for( Uint32 frame = 0; frame < _frames; ++frame, ++total_frames )
		{
			float vol_level = m_envLFOWidgets[VOLUME]->level( total_frames, release_begin, frame );
			vol_level = vol_level*vol_level;
			for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
			{
				_ab[frame][chnl] = vol_level * _ab[frame][chnl];
			}
		}
	}
/*	else if( m_envLFOWidgets[VOLUME]->used() == FALSE && m_envLFOWidgets[PANNING]->used() )
	{
		// only use panning-envelope...
		for( Uint32 frame = 0; frame < _frames; ++frame, ++total_frames )
		{
			float vol_level = m_envLFOWidgets[PANNING]->level( total_frames, release_begin, frame );
			vol_level = vol_level*vol_level;
			for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
			{
				_ab[frame][chnl] = vol_level * _ab[frame][chnl];
			}
		}
	}*/
}




Uint32 envelopeTabWidget::envFrames( void )
{
	Uint32 ret_val = m_envLFOWidgets[VOLUME]->m_pahdFrames;

	for( int i = VOLUME+1; i < TARGET_COUNT; ++i )
	{
		if( m_envLFOWidgets[i]->used() &&
				m_envLFOWidgets[i]->m_pahdFrames > ret_val )
		{
			ret_val = m_envLFOWidgets[i]->m_pahdFrames;
		}
	}
	return( ret_val );
}




Uint32 envelopeTabWidget::releaseFrames( void )
{
	Uint32 ret_val = m_envLFOWidgets[VOLUME]->m_rFrames;
	for( int i = VOLUME+1; i < TARGET_COUNT; ++i )
	{
		if( m_envLFOWidgets[i]->used() &&
				m_envLFOWidgets[i]->m_rFrames > ret_val )
		{
			ret_val = m_envLFOWidgets[i]->m_rFrames;
		}
	}
	return( ret_val );
}




void envelopeTabWidget::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement etw_de = _doc.createElement( nodeName() );
	etw_de.setAttribute( "ftype", m_filterComboBox->currentIndex() );
	etw_de.setAttribute( "fcut", m_filterCutKnob->value() );
	etw_de.setAttribute( "fres", m_filterResKnob->value() );
	etw_de.setAttribute( "fwet", m_filterGroupBox->isActive() );
	_parent.appendChild( etw_de );

	for( int i = 0; i < TARGET_COUNT; ++i )
	{
		QDomElement target_de = _doc.createElement(
						m_envLFOWidgets[i]->nodeName() +
					QString(
						targetNames[i][1] ).toLower() );
		m_envLFOWidgets[i]->saveSettings( _doc, target_de );
		etw_de.appendChild( target_de );
	}
}




void envelopeTabWidget::loadSettings( const QDomElement & _this )
{
	m_filterComboBox->setCurrentIndex( _this.attribute( "ftype" ).toInt() );
	m_filterCutKnob->setValue( _this.attribute( "fcut" ).toFloat() );
	m_filterResKnob->setValue( _this.attribute( "fres" ).toFloat() );
/*	m_filterState->setChecked( _this.attribute( "fwet" ).toInt() );*/
	m_filterGroupBox->setState( _this.attribute( "fwet" ).toInt() );

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			for( int i = 0; i < TARGET_COUNT; ++i )
			{
				if( node.nodeName() ==
						m_envLFOWidgets[i]->nodeName() +
					QString( targetNames[i][1] ).toLower() )
				{
					m_envLFOWidgets[i]->loadSettings(
							node.toElement() );
				}
			}
		}
		node = node.nextSibling();
	}
}




#include "envelope_tab_widget.moc"



/*


const long double coeff[5][11]= {
{
 ///A
8.11044e-06,
8.943665402,    -36.83889529,    92.01697887,    -154.337906,    181.6233289,
-151.8651235,   89.09614114,    -35.10298511,    8.388101016,    -0.923313471
},
{
///E
4.36215e-06,
8.90438318,    -36.55179099,    91.05750846,    -152.422234,    179.1170248,
-149.6496211,87.78352223,    -34.60687431,    8.282228154,    -0.914150747
},
{
///I
3.33819e-06,
8.893102966,    -36.49532826,    90.96543286,    -152.4545478,    179.4835618,
-150.315433,    88.43409371,    -34.98612086,    8.407803364,    -0.932568035
},
{
 ///O
1.13572e-06,
8.994734087,    -37.2084849,    93.22900521,    -156.6929844,    184.596544,
-154.3755513,    90.49663749,    -35.58964535,    8.478996281,    -0.929252233
},
{
///U
4.09431e-07,
8.997322763,    -37.20218544,    93.11385476,    -156.2530937,    183.7080141,
-153.2631681,    89.59539726,    -35.12454591,    8.338655623,    -0.910251753
}
};
//-----------------------------------------------------------------------------
static long double memory[10]={0,0,0,0,0,0,0,0,0,0};
//------------------------------------------------------------------------------
inline float formant_filter(float in, int vowelnum)
{
            float res= (float) ( coeff[vowelnum][0]  *in +
                     coeff[vowelnum][1]  *memory[0] +  
                     coeff[vowelnum][2]  *memory[1] +
                     coeff[vowelnum][3]  *memory[2] +
                     coeff[vowelnum][4]  *memory[3] +
                     coeff[vowelnum][5]  *memory[4] +
                     coeff[vowelnum][6]  *memory[5] +
                     coeff[vowelnum][7]  *memory[6] +
                     coeff[vowelnum][8]  *memory[7] +
                     coeff[vowelnum][9]  *memory[8] +
                     coeff[vowelnum][10] *memory[9] );

memory[9]= memory[8];
memory[8]= memory[7];
memory[7]= memory[6];
memory[6]= memory[5];
memory[5]= memory[4];
memory[4]= memory[3];
memory[3]= memory[2];
memory[2]= memory[1];                    
memory[1]= memory[0];
memory[0]=(long double) res;
return res;
}

*/
