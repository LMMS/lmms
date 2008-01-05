#ifndef SINGLE_SOURCE_COMPILE

/*
 * envelope_tab_widget.cpp - widget for use in envelope/lfo/filter-tab of
 *                           instrument-track-window
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include <Qt/QtXml>


#include "envelope_tab_widget.h"
#include "combobox.h"
#include "embed.h"
#include "engine.h"
#include "envelope_and_lfo_widget.h"
#include "group_box.h"
#include "gui_templates.h"
#include "instrument_track.h"
#include "knob.h"
#include "note_play_handle.h"
#include "tab_widget.h"



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
 


envelopeTabWidget::envelopeTabWidget( instrumentTrack * _instrument_track ) :
	QWidget( _instrument_track->tabWidgetParent() ),
	m_instrumentTrack( _instrument_track ),
	m_filterEnabledModel( new boolModel( FALSE, FALSE, TRUE, 1 /* this */ ) ),
	m_filterModel( new comboBoxModel( /* this */ ) ),
	m_filterCutModel( new floatModel( /* this */ ) ),
	m_filterResModel( new floatModel( /* this */ ) )
{
	m_targetsTabWidget = new tabWidget( tr( "TARGET" ), this );
	m_targetsTabWidget->setGeometry( TARGETS_TABWIDGET_X,
						TARGETS_TABWIDGET_Y,
						TARGETS_TABWIDGET_WIDTH,
						TARGETS_TABWIDGET_HEIGTH );
	m_targetsTabWidget->setWhatsThis(
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
							value_for_zero_amount,
							m_targetsTabWidget,
							_instrument_track );
		m_targetsTabWidget->addTab( m_envLFOWidgets[i],
						tr( targetNames[i][0]
						.toAscii().constData() ) );
	}


	m_filterEnabledModel->setTrack( _instrument_track );
	m_filterGroupBox = new groupBox( tr( "FILTER" ), this );
	m_filterGroupBox->setModel( m_filterEnabledModel );
	m_filterGroupBox->setGeometry( FILTER_GROUPBOX_X, FILTER_GROUPBOX_Y,
						FILTER_GROUPBOX_WIDTH,
						FILTER_GROUPBOX_HEIGHT );


	m_filterModel->addItem( tr( "LowPass" ), new QPixmap(
					embed::getIconPixmap( "filter_lp" ) ) );
	m_filterModel->addItem( tr( "HiPass" ), new QPixmap(
					embed::getIconPixmap( "filter_hp" ) ) );
	m_filterModel->addItem( tr( "BandPass csg" ), new QPixmap(
					embed::getIconPixmap( "filter_bp" ) ) );
	m_filterModel->addItem( tr( "BandPass czpg" ), new QPixmap(
					embed::getIconPixmap( "filter_bp" ) ) );
	m_filterModel->addItem( tr( "Notch" ), new QPixmap(
				embed::getIconPixmap( "filter_notch" ) ) );
	m_filterModel->addItem( tr( "Allpass" ), new QPixmap(
					embed::getIconPixmap( "filter_ap" ) ) );
	m_filterModel->addItem( tr( "Moog" ), new QPixmap(
					embed::getIconPixmap( "filter_lp" ) ) );
	m_filterModel->addItem( tr( "2x LowPass" ), new QPixmap(
					embed::getIconPixmap( "filter_2lp" ) ) );

	m_filterModel->setTrack( _instrument_track );
	m_filterComboBox = new comboBox( m_filterGroupBox, tr( "Filter type" ) );
	m_filterComboBox->setModel( m_filterModel );
	m_filterComboBox->setGeometry( 14, 22, 120, 22 );
	m_filterComboBox->setFont( pointSize<8>( m_filterComboBox->font() ) );

	m_filterComboBox->setWhatsThis(
		tr( "Here you can select the built-in filter you want to use "
			"for this instrument-track. Filters are very important "
			"for changing the characteristics of a sound." ) );


	m_filterCutModel->setTrack( _instrument_track );
	m_filterCutModel->setRange( 0.0, 14000.0, 1.0 );
	m_filterCutModel->setInitValue( 16000.0 );

	m_filterCutKnob = new knob( knobBright_26, m_filterGroupBox,
						tr( "cutoff-frequency" ) );
	m_filterCutKnob->setModel( m_filterCutModel );
	m_filterCutKnob->setLabel( tr( "CUTOFF" ) );
	m_filterCutKnob->move( 140, 18 );
	m_filterCutKnob->setHintText( tr( "cutoff-frequency:" ) + " ", " " +
								tr( "Hz" ) );
	m_filterCutKnob->setWhatsThis(
		tr( "Use this knob for setting the cutoff-frequency for the "
			"selected filter. The cutoff-frequency specifies the "
			"frequency for cutting the signal by a filter. For "
			"example a lowpass-filter cuts all frequencies above "
			"the cutoff-frequency. A highpass-filter cuts all "
			"frequencies below cutoff-frequency and so on..." ) );



	m_filterResModel->setTrack( _instrument_track );
	m_filterResModel->setRange( basicFilters<>::minQ(), 10.0, 0.01 );
	m_filterResModel->setInitValue( 0.5 );

	m_filterResKnob = new knob( knobBright_26, m_filterGroupBox,
							tr( "Q/Resonance" ) );
	m_filterResKnob->setModel( m_filterResModel );
	m_filterResKnob->setLabel( tr( "Q/RESO" ) );
	m_filterResKnob->move( 190, 18 );
	m_filterResKnob->setHintText( tr( "Q/Resonance:" ) + " ", "" );
	m_filterResKnob->setWhatsThis(
		tr( "Use this knob for setting Q/Resonance for the selected "
			"filter. Q/Resonance tells the filter, how much it "
			"should amplify frequencies near Cutoff-frequency." ) );

}




envelopeTabWidget::~envelopeTabWidget()
{
	delete m_targetsTabWidget;
}




float FASTCALL envelopeTabWidget::volumeLevel( notePlayHandle * _n,
							const f_cnt_t _frame )
{
	f_cnt_t release_begin = _frame - _n->releaseFramesDone() +
						_n->framesBeforeRelease();

	if( _n->released() == FALSE )
	{
		release_begin += engine::getMixer()->framesPerPeriod();
	}

	float volume_level;
	m_envLFOWidgets[VOLUME]->fillLevel( &volume_level, _frame,
							release_begin, 1 );

	return( volume_level );
}




void envelopeTabWidget::processAudioBuffer( sampleFrame * _ab,
							const fpp_t _frames,
							notePlayHandle * _n )
{
	const f_cnt_t total_frames = _n->totalFramesPlayed();
	f_cnt_t release_begin = total_frames - _n->releaseFramesDone() +
						_n->framesBeforeRelease();

	if( _n->released() == FALSE )
	{
		release_begin += engine::getMixer()->framesPerPeriod();
	}

	// because of optimizations, there's special code for several cases:
	// 	- cut- and res-lfo/envelope active
	// 	- cut-lfo/envelope active
	// 	- res-lfo/envelope active
	//	- no lfo/envelope active but filter is used

	// only use filter, if it is really needed

	if( m_filterEnabledModel->value() )
	{
		int old_filter_cut = 0;
		int old_filter_res = 0;

		if( _n->m_filter == NULL )
		{
			_n->m_filter = new basicFilters<>(
					engine::getMixer()->sampleRate() );
		}
		_n->m_filter->setFilterType( m_filterComboBox->value() );

		float * cut_buf = NULL;
		float * res_buf = NULL;

		if( m_envLFOWidgets[CUT]->used() )
		{
			cut_buf = new float[_frames];
			m_envLFOWidgets[CUT]->fillLevel( cut_buf, total_frames,
						release_begin, _frames );
		}
		if( m_envLFOWidgets[RES]->used() )
		{
			res_buf = new float[_frames];
			m_envLFOWidgets[RES]->fillLevel( res_buf, total_frames,
						release_begin, _frames );
		}

		if( m_envLFOWidgets[CUT]->used() &&
			m_envLFOWidgets[RES]->used() )
		{
			for( fpp_t frame = 0; frame < _frames; ++frame )
			{
				float new_cut_val = envelopeAndLFOWidget::expKnobVal( cut_buf[frame] ) * CUT_FREQ_MULTIPLIER +
						m_filterCutKnob->value();

				float new_res_val = m_filterResKnob->value() + RES_MULTIPLIER *
							res_buf[frame];

				if( static_cast<int>( new_cut_val ) != old_filter_cut ||
					static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					_n->m_filter->calcFilterCoeffs( new_cut_val, new_res_val );
					old_filter_cut = static_cast<int>( new_cut_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
		else if( m_envLFOWidgets[CUT]->used() )
		{
			for( fpp_t frame = 0; frame < _frames; ++frame )
			{
				float new_cut_val = envelopeAndLFOWidget::expKnobVal( cut_buf[frame] ) * CUT_FREQ_MULTIPLIER +
						m_filterCutKnob->value();

				if( static_cast<int>( new_cut_val ) != old_filter_cut )
				{
					_n->m_filter->calcFilterCoeffs( new_cut_val, m_filterResKnob->value() );
					old_filter_cut = static_cast<int>( new_cut_val );
				}

				for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
		else if( m_envLFOWidgets[RES]->used() )
		{
			for( fpp_t frame = 0; frame < _frames; ++frame )
			{
				float new_res_val = m_filterResKnob->value() + RES_MULTIPLIER *
							res_buf[frame];

				if( static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					_n->m_filter->calcFilterCoeffs( m_filterCutKnob->value(), new_res_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}
		else
		{
			_n->m_filter->calcFilterCoeffs( m_filterCutKnob->value(), m_filterResKnob->value() );

			for( fpp_t frame = 0; frame < _frames; ++frame )
			{
				for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
				{
					_ab[frame][chnl] = _n->m_filter->update( _ab[frame][chnl], chnl );
				}
			}
		}

		delete[] cut_buf;
		delete[] res_buf;
	}

	if( m_envLFOWidgets[VOLUME]->used() )
	{
		float * vol_buf = new float[_frames];
		m_envLFOWidgets[VOLUME]->fillLevel( vol_buf, total_frames,
						release_begin, _frames );

		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			float vol_level = vol_buf[frame];
			vol_level = vol_level * vol_level;
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
			{
				_ab[frame][chnl] = vol_level * _ab[frame][chnl];
			}
		}
		delete[] vol_buf;
	}

/*	else if( m_envLFOWidgets[VOLUME]->used() == FALSE && m_envLFOWidgets[PANNING]->used() )
	{
		// only use panning-envelope...
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			float vol_level = pan_buf[frame];
			vol_level = vol_level*vol_level;
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
			{
				_ab[frame][chnl] = vol_level * _ab[frame][chnl];
			}
		}
	}*/
}




f_cnt_t envelopeTabWidget::envFrames( const bool _only_vol )
{
	f_cnt_t ret_val = m_envLFOWidgets[VOLUME]->m_pahdFrames;

	if( _only_vol == FALSE )
	{
		for( int i = VOLUME+1; i < TARGET_COUNT; ++i )
		{
			if( m_envLFOWidgets[i]->used() &&
				m_envLFOWidgets[i]->m_pahdFrames > ret_val )
			{
				ret_val = m_envLFOWidgets[i]->m_pahdFrames;
			}
		}
	}
	return( ret_val );
}




f_cnt_t envelopeTabWidget::releaseFrames( const bool _only_vol )
{
	f_cnt_t ret_val = m_envLFOWidgets[VOLUME]->used() ?
					m_envLFOWidgets[VOLUME]->m_rFrames : 0;
	if( m_instrumentTrack->getInstrument()->desiredReleaseFrames() >
								ret_val )
	{
		ret_val = m_instrumentTrack->getInstrument()->
							desiredReleaseFrames();
	}

	if( m_envLFOWidgets[VOLUME]->used() == FALSE )
	{
		for( int i = VOLUME+1; i < TARGET_COUNT; ++i )
		{
			if( m_envLFOWidgets[i]->used() &&
				m_envLFOWidgets[i]->m_rFrames > ret_val )
			{
				ret_val = m_envLFOWidgets[i]->m_rFrames;
			}
		}
	}
	return( ret_val );
}




void envelopeTabWidget::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_filterModel->saveSettings( _doc, _this, "ftype" );
	m_filterCutModel->saveSettings( _doc, _this, "fcut" );
	m_filterResModel->saveSettings( _doc, _this, "fres" );
	m_filterEnabledModel->saveSettings( _doc, _this, "fwet" );

	for( int i = 0; i < TARGET_COUNT; ++i )
	{
		m_envLFOWidgets[i]->saveState( _doc, _this ).setTagName(
			m_envLFOWidgets[i]->nodeName() +
				QString( targetNames[i][1] ).toLower() );
	}
}




void envelopeTabWidget::loadSettings( const QDomElement & _this )
{
	m_filterModel->loadSettings( _this, "ftype" );
	m_filterCutModel->loadSettings( _this, "fcut" );
	m_filterResModel->loadSettings( _this, "fres" );
	m_filterEnabledModel->loadSettings( _this, "fwet" );

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
					m_envLFOWidgets[i]->restoreState(
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

#endif
