/*
 * Microwave.cpp - morbidly advanced and versatile modular wavetable synthesizer
 *
 * Copyright (c) 2019 Robert Daniel Black AKA Lost Robot <r94231/at/gmail/dot/com>
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





#include "Microwave.h"

#include <QColorDialog>
#include <QDomElement>
#include <QDropEvent>
#include <QInputDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

#include "base64.h"
#include "CaptionMenu.h"
#include "embed.h"
#include "Engine.h"
#include "FileDialog.h"
#include "Graph.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "interpolation.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "LedCheckbox.h"
#include "lmms_math.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "plugin_export.h"
#include "SampleBuffer.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "templates.h"
#include "TextFloat.h"
#include "ToolTip.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT microwave_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Microwave",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Versatile modular wavetable synthesizer" ),
	"Lost Robot",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


/*                                                                                                   
          ____                                                                                     
        ,'  , `.                                                                                   
     ,-+-,.' _ |  ,--,                                                                             
  ,-+-. ;   , ||,--.'|              __  ,-.   ,---.           .---.                                
 ,--.'|'   |  ;||  |,             ,' ,'/ /|  '   ,'\         /. ./|                .---.           
|   |  ,', |  ':`--'_       ,---. '  | |' | /   /   |     .-'-. ' |  ,--.--.     /.  ./|   ,---.   
|   | /  | |  ||,' ,'|     /     \|  |   ,'.   ; ,. :    /___/ \: | /       \  .-' . ' |  /     \  
'   | :  | :  |,'  | |    /    / ''  :  /  '   | |: : .-'.. '   ' ..--.  .-. |/___/ \: | /    /  | 
;   . |  ; |--' |  | :   .    ' / |  | '   '   | .; :/___/ \:     ' \__\/: . ..   \  ' ..    ' / | 
|   : |  | ,    '  : |__ '   ; :__;  : |   |   :    |.   \  ' .\    ," .--.; | \   \   ''   ;   /| 
|   : '  |/     |  | '.'|'   | '.'|  , ;    \   \  /  \   \   ' \ |/  /  ,.  |  \   \   '   |  / | 
;   | |`-'      ;  :    ;|   :    :---'      `----'    \   \  |--";  :   .'   \  \   \ ||   :    | 
|   ;/          |  ,   /  \   \  /                      \   \ |   |  ,     .-./   '---"  \   \  /  
'---'            ---`-'    `----'                        '---"     `--`---'               `----'   
                                                                                                   
                                    .----------------.
                                   /_____________ ____\
                                   ||\ _________ /|+++|
                                   || |:       :| |+++|
                                   || |; (◕‿◕) ;| | + |
                                   || |_________| | _ |
                                   ||/___________\|[_]|
                                   "------------------"

                       (Do not scroll down if you value your sanity)

*/




Microwave::Microwave( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &microwave_plugin_descriptor ),
	visvol( 100, 0, 1000, 0.01f, this, tr( "Visualizer Volume" ) ),
	loadChnl( 0, 0, 1, 1, this, tr( "Wavetable Loading Channel" ) ),
	mainNum(1, 1, 8, this, tr( "Main Oscillator Number" ) ),
	subNum(1, 1, 64, this, tr( "Sub Oscillator Number" ) ),
	sampNum(1, 1, 8, this, tr( "Sample Number" ) ),
	oversample( this, tr("Oversampling") ),
	oversampleMode( this, tr("Interpolation") ),
	loadMode( this, tr("Wavetable Loading Algorithm") ),
	graph( -1.0f, 1.0f, 204, this ),
	visualize( false, this )
{

	for( int i = 0; i < 8; ++i )
	{
		morph[i] = new FloatModel( 0, 0, 254, 0.0001f, this, tr( "Morph" ) );
		range[i] = new FloatModel( 1, 1, 16, 0.0001f, this, tr( "Range" ) );
		sampLen[i] = new FloatModel( 2048, 1, 16384, 1.f, this, tr( "Waveform Sample Length" ) );
		morphMax[i] = new FloatModel( 255, 0, 254, 0.0001f, this, tr( "Morph Max" ) );
		modify[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Wavetable Modifier Value" ) );
		modifyMode[i] = new ComboBoxModel( this, tr( "Wavetable Modifier Mode" ) );
		unisonVoices[i] = new FloatModel( 1, 1, 32, 1, this, tr( "Unison Voices" ) );
		unisonDetune[i] = new FloatModel( 0, 0, 2000, 0.0001f, this, tr( "Unison Detune" ) );
		unisonDetune[i]->setScaleLogarithmic( true );
		unisonMorph[i] = new FloatModel( 0, 0, 256, 0.0001f, this, tr( "Unison Morph" ) );
		unisonModify[i] = new FloatModel( 0, 0, 2048, 0.0001f, this, tr( "Unison Modify" ) );
		detune[i] = new FloatModel( 0, -9600, 9600, 0.0001f, this, tr( "Detune" ) );
		phase[i] = new FloatModel( 0, 0, 200, 0.0001f, this, tr( "Phase" ) );
		phaseRand[i] = new FloatModel( 100, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) );
		vol[i] = new FloatModel( 100.f, 0, 200.f, 0.0001f, this, tr( "Volume" ) );
		enabled[i] = new BoolModel( false, this );
		muted[i] = new BoolModel( false, this );
		pan[i] = new FloatModel( 0.f, -100.f, 100.f, 0.0001f, this, tr( "Panning" ) );
		keytracking[i] = new BoolModel( true, this );
		tempo[i] = new FloatModel( 0.f, 0.f, 999.f, 1.f, this, tr( "Tempo" ) );
		interpolate[i] = new BoolModel( true, this );
		setwavemodel( modifyMode[i] )

		filtInVol[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Input Volume" ) );
		filtType[i] = new ComboBoxModel( this, tr( "Filter Type" ) );
		filtSlope[i] = new ComboBoxModel( this, tr( "Filter Slope" ) );
		filtCutoff[i] = new FloatModel( 2000, 20, 20000, 0.0001f, this, tr( "Cutoff Frequency" ) );
		filtCutoff[i]->setScaleLogarithmic( true );
		filtReso[i] = new FloatModel( 0.707, 0, 16, 0.0001f, this, tr( "Resonance" ) );
		filtReso[i]->setScaleLogarithmic( true );
		filtGain[i] = new FloatModel( 0, -64, 64, 0.0001f, this, tr( "dbGain" ) );
		filtGain[i]->setScaleLogarithmic( true );
		filtSatu[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Saturation" ) );
		filtWetDry[i] = new FloatModel( 100, 0, 100, 0.0001f, this, tr( "Wet/Dry" ) );
		filtBal[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Balance/Panning" ) );
		filtOutVol[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Output Volume" ) );
		filtEnabled[i] = new BoolModel( false, this );
		filtFeedback[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Feedback" ) );
		filtDetune[i] = new FloatModel( 0, -9600, 9600, 0.0001f, this, tr( "Detune" ) );
		filtKeytracking[i] = new BoolModel( true, this );
		filtMuted[i] = new BoolModel( false, this );
		filtertypesmodel( filtType[i] )
		filterslopesmodel( filtSlope[i] )

		sampleEnabled[i] = new BoolModel( false, this );
		sampleGraphEnabled[i] = new BoolModel( false, this );
		sampleMuted[i] = new BoolModel( false, this );
		sampleKeytracking[i] = new BoolModel( true, this );
		sampleLoop[i] = new BoolModel( true, this );

		sampleVolume[i] = new FloatModel( 100, 0, 200, 0.0001f, this, tr( "Volume" ) );
		samplePanning[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Panning" ) );
		sampleDetune[i] = new FloatModel( 0, -9600, 9600, 0.0001f, this, tr( "Detune" ) );
		samplePhase[i] = new FloatModel( 0, 0, 200, 0.0001f, this, tr( "Phase" ) );
		samplePhaseRand[i] = new FloatModel( 0, 0, 100, 0.0001f, this, tr( "Phase Randomness" ) );
		sampleStart[i] = new FloatModel( 0, 0, 1, 0.0001f, this, tr( "Start" ) );
		sampleEnd[i] = new FloatModel( 1, 0, 1, 0.0001f, this, tr( "End" ) );

		keytrackingArr[i] = true;
		interpolateArr[i] = true;
	}

	for( int i = 0; i < 18; ++i )
	{
		macro[i] = new FloatModel( 0, -100, 100, 0.0001f, this, tr( "Macro" ) );
	}

	for( int i = 0; i < 64; ++i )
 	{
		subEnabled[i] = new BoolModel( false, this );
		subVol[i] = new FloatModel( 100.f, 0.f, 200.f, 0.0001f, this, tr( "Volume" ) );
		subPhase[i] = new FloatModel( 0.f, 0.f, 200.f, 0.0001f, this, tr( "Phase" ) );
		subPhaseRand[i] = new FloatModel( 0.f, 0.f, 100.f, 0.0001f, this, tr( "Phase Randomness" ) );
		subDetune[i] = new FloatModel( 0.f, -9600.f, 9600.f, 0.0001f, this, tr( "Detune" ) );
		subMuted[i] = new BoolModel( true, this );
		subKeytrack[i] = new BoolModel( true, this );
		subSampLen[i] = new FloatModel( STOREDSUBWAVELEN, 1.f, STOREDSUBWAVELEN, 1.f, this, tr( "Sample Length" ) );
		subNoise[i] = new BoolModel( false, this );
		subPanning[i] = new FloatModel( 0.f, -100.f, 100.f, 0.0001f, this, tr( "Panning" ) );
		subTempo[i] = new FloatModel( 0.f, 0.f, 999.f, 1.f, this, tr( "Tempo" ) );
		subRateLimit[i] = new FloatModel( 0.f, 0.f, 1.f, 0.000001f, this, tr( "Rate Limit" ) );
		subRateLimit[i]->setScaleLogarithmic( true );
		subUnisonNum[i] = new FloatModel( 1.f, 1.f, 32.f, 1.f, this, tr( "Unison Voice Number" ) );
		subUnisonDetune[i] = new FloatModel( 0.f, 0.f, 2000.f, 0.0001f, this, tr( "Unison Detune" ) );
		subInterpolate[i] = new BoolModel( true, this );

		modEnabled[i] = new BoolModel( false, this );

		modOutSec[i] = new ComboBoxModel( this, tr( "Modulation Section" ) );
		modOutSig[i] = new ComboBoxModel( this, tr( "Modulation Signal" ) );
		modOutSecNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulation Section Number" ) );
		modsectionsmodel( modOutSec[i] )
		mainoscsignalsmodel( modOutSig[i] )

		modIn[i] = new ComboBoxModel( this, tr( "Modulator" ) );
		modInNum[i] = new IntModel( 1, 1, 8, this, tr( "Modulator Number" ) );
		modinmodel( modIn[i] )

		modInAmnt[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Modulator Amount" ) );
		modInCurve[i] = new FloatModel( 100, 10.f, 600, 0.0001f, this, tr( "Modulator Curve" ) );
		modInCurve[i]->setScaleLogarithmic( true );

		modIn2[i] = new ComboBoxModel( this, tr( "Secondary Modulator" ) );
		modInNum2[i] = new IntModel( 1, 1, 8, this, tr( "Secondary Modulator Number" ) );
		modinmodel( modIn2[i] )

		modInAmnt2[i] = new FloatModel( 0, -200, 200, 0.0001f, this, tr( "Secondary Modulator Amount" ) );
		modInCurve2[i] = new FloatModel( 100, 10.f, 600, 0.0001f, this, tr( "Secondary Modulator Curve" ) );
		modInCurve2[i]->setScaleLogarithmic( true );

		modCombineType[i] = new ComboBoxModel( this, tr( "Combination Type" ) );
		modcombinetypemodel( modCombineType[i] )

		modType[i] = new BoolModel( false, this );
		modType2[i] = new BoolModel( false, this );
	}

	oversamplemodel( oversample )
	oversample.setValue( 1 );// 2x oversampling is default

	loadmodemodel( loadMode )

	oversamplemodemodel( oversampleMode )
	oversampleMode.setValue( 1 );// Sample averaging is default

	connect( &graph, SIGNAL( samplesChanged( int, int ) ), this, SLOT( samplesChanged( int, int ) ) );

	for( int i = 0; i < 8; ++i )
	{
		connect( morph[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(1, i); }, Qt::DirectConnection );
		connect( range[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(2, i); }, Qt::DirectConnection );
		connect( modify[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(3, i); }, Qt::DirectConnection );
		connect( modifyMode[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(4, i); }, Qt::DirectConnection );
		connect( vol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(5, i); }, Qt::DirectConnection );
		connect( pan[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(6, i); }, Qt::DirectConnection );
		connect( detune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(7, i); }, Qt::DirectConnection );
		connect( phase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(8, i); }, Qt::DirectConnection );
		connect( phaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(9, i); }, Qt::DirectConnection );
		connect( enabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(10, i); }, Qt::DirectConnection );
		connect( muted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(11, i); }, Qt::DirectConnection );
		connect( sampLen[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(12, i); }, Qt::DirectConnection );
		connect( morphMax[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(13, i); }, Qt::DirectConnection );
		connect( unisonVoices[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(14, i); }, Qt::DirectConnection );
		connect( unisonDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(15, i); }, Qt::DirectConnection );
		connect( unisonMorph[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(16, i); }, Qt::DirectConnection );
		connect( unisonModify[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(17, i); }, Qt::DirectConnection );
		connect( keytracking[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(18, i); }, Qt::DirectConnection );
		connect( tempo[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(19, i); }, Qt::DirectConnection );
		connect( interpolate[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(20, i); }, Qt::DirectConnection );

		connect( sampleEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(60, i); }, Qt::DirectConnection );
		connect( sampleMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(61, i); }, Qt::DirectConnection );
		connect( sampleKeytracking[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(62, i); }, Qt::DirectConnection );
		connect( sampleGraphEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(63, i); }, Qt::DirectConnection );
		connect( sampleLoop[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(64, i); }, Qt::DirectConnection );
		connect( sampleVolume[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(65, i); }, Qt::DirectConnection );
		connect( samplePanning[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(66, i); }, Qt::DirectConnection );
		connect( sampleDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(67, i); }, Qt::DirectConnection );
		connect( samplePhase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(68, i); }, Qt::DirectConnection );
		connect( samplePhaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(69, i); }, Qt::DirectConnection );
		connect( sampleStart[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(70, i); }, Qt::DirectConnection );
		connect( sampleEnd[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(71, i); }, Qt::DirectConnection );

		connect( filtCutoff[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(120, i); }, Qt::DirectConnection );
		connect( filtReso[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(121, i); }, Qt::DirectConnection );
		connect( filtGain[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(122, i); }, Qt::DirectConnection );
		connect( filtType[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(123, i); }, Qt::DirectConnection );
		connect( filtSlope[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(124, i); }, Qt::DirectConnection );
		connect( filtInVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(125, i); }, Qt::DirectConnection );
		connect( filtOutVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(126, i); }, Qt::DirectConnection );
		connect( filtWetDry[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(127, i); }, Qt::DirectConnection );
		connect( filtBal[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(128, i); }, Qt::DirectConnection );
		connect( filtSatu[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(129, i); }, Qt::DirectConnection );
		connect( filtFeedback[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(130, i); }, Qt::DirectConnection );
		connect( filtDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(131, i); }, Qt::DirectConnection );
		connect( filtEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(132, i); }, Qt::DirectConnection );
		connect( filtMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(133, i); }, Qt::DirectConnection );
		connect( filtKeytracking[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(134, i); }, Qt::DirectConnection );

		for( int j = 1; j <= 20; ++j )
		{
			valueChanged(j, i);
		}

		for( int j = 60; j <= 71; ++j )
		{
			valueChanged(j, i);
		}

		for( int j = 120; j <= 134; ++j )
		{
			valueChanged(j, i);
		}

		connect( sampleEnabled[i], &BoolModel::dataChanged, this, [this, i]() { sampleEnabledChanged(i); }, Qt::DirectConnection );

		connect( enabled[i], &BoolModel::dataChanged, this, [this, i]() { mainEnabledChanged(i); }, Qt::DirectConnection );

		connect( filtEnabled[i], &BoolModel::dataChanged, this, [this, i]() { filtEnabledChanged(i); }, Qt::DirectConnection );

		connect( sampLen[i], &FloatModel::dataChanged, this, [this, i]() { sampLenChanged(i); }, Qt::DirectConnection );

		connect( interpolate[i], &BoolModel::dataChanged, this, [this, i]() { interpolateChanged(i); } );

		connect( morphMax[i], &FloatModel::dataChanged, this, [this, i]() { morphMaxChanged(i); }, Qt::DirectConnection );
	}

	for( int i = 0; i < 18; ++i )
	{
		connect( macro[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(150, i); }, Qt::DirectConnection );

		valueChanged(150, i);

		macroColors[i][0] = 102;
		macroColors[i][1] = 198;
		macroColors[i][2] = 199;
	}

	for( int i = 0; i < 64; ++i )
	{
		connect( subEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(30, i); }, Qt::DirectConnection );
		connect( subMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(31, i); }, Qt::DirectConnection );
		connect( subKeytrack[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(32, i); }, Qt::DirectConnection );
		connect( subNoise[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(33, i); }, Qt::DirectConnection );
		connect( subVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(34, i); }, Qt::DirectConnection );
		connect( subPanning[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(35, i); }, Qt::DirectConnection );
		connect( subDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(36, i); }, Qt::DirectConnection );
		connect( subPhase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(37, i); }, Qt::DirectConnection );
		connect( subPhaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(38, i); }, Qt::DirectConnection );
		connect( subSampLen[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(39, i); }, Qt::DirectConnection );
		connect( subTempo[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(40, i); }, Qt::DirectConnection );
		connect( subRateLimit[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(41, i); }, Qt::DirectConnection );
		connect( subUnisonNum[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(42, i); }, Qt::DirectConnection );
		connect( subUnisonDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(43, i); }, Qt::DirectConnection );
		connect( subInterpolate[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(44, i); }, Qt::DirectConnection );

		connect( modIn[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(90, i); }, Qt::DirectConnection );
		connect( modInNum[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(91, i); }, Qt::DirectConnection );
		connect( modInAmnt[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(92, i); }, Qt::DirectConnection );
		connect( modInCurve[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(93, i); }, Qt::DirectConnection );
		connect( modIn2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(94, i); }, Qt::DirectConnection );
		connect( modInNum2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(95, i); }, Qt::DirectConnection );
		connect( modInAmnt2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(96, i); }, Qt::DirectConnection );
		connect( modInCurve2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(97, i); }, Qt::DirectConnection );
		connect( modOutSec[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(98, i); }, Qt::DirectConnection );
		connect( modOutSig[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(99, i); }, Qt::DirectConnection );
		connect( modOutSecNum[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(100, i); }, Qt::DirectConnection );
		connect( modEnabled[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(101, i); }, Qt::DirectConnection );
		connect( modCombineType[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(102, i); }, Qt::DirectConnection );
		connect( modType[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(103, i); }, Qt::DirectConnection );
		connect( modType2[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(104, i); }, Qt::DirectConnection );

		for( int j = 30; j <= 44; ++j )
		{
			valueChanged(j, i);
		}

		for( int j = 90; j <= 104; ++j )
		{
			valueChanged(j, i);
		}

		connect( modEnabled[i], &BoolModel::dataChanged, this, [this, i]() { modEnabledChanged(i); }, Qt::DirectConnection );

		connect( subSampLen[i], &FloatModel::dataChanged, this, [this, i]() { subSampLenChanged(i); } );

		connect( subEnabled[i], &BoolModel::dataChanged, this, [this, i]() { subEnabledChanged(i); } );

		connect( subInterpolate[i], &BoolModel::dataChanged, this, [this, i]() { subInterpolateChanged(i); } );
	}

	for( int i = 0; i < 8; ++i )
	{
		samples[i][0].push_back(0);
		samples[i][1].push_back(0);
	}
}


Microwave::~Microwave()
{
	for( int i = 0; i < 64; ++i )
	{
		/*The following disconnected functions will run if not disconnected upon deletion,
		because deleting a ComboBox includes clearing its contents first,
		which will fire a dataChanged event.  So, we need to disconnect them to
		prevent a crash.*/
		disconnect(modIn[i], &ComboBoxModel::dataChanged, 0, 0);
		disconnect(modIn2[i], &ComboBoxModel::dataChanged, 0, 0);
		disconnect(modOutSec[i], &ComboBoxModel::dataChanged, 0, 0);
	}
}


PluginView * Microwave::instantiateView( QWidget * _parent )
{
	return( new MicrowaveView( this, _parent ) );
}


QString Microwave::nodeName() const
{
	return( microwave_plugin_descriptor.name );
}


void Microwave::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// NOTE: Only enabled oscillators/sections are saved.  This is to prevent ridiculously long project save times, as well as total disk space annihilation.

	// Save plugin version
	_this.setAttribute( "version", "Microwave Official Release 1" );

	/*

	VERSION LIST:

	- 0.9: Every version before Microwave Testing Release 4 was mistakenly listed as 0.9.

	- Microwave Testing Release 4
	- Microwave Testing Release 4.1
	- Microwave Testing Release 4.2
	- Microwave Testing Release 5
	- Microwave Testing Release 5.1
	- Microwave Testing Release 5.2

	- Microwave Official Release 1

	*/

	visvol.saveSettings( _doc, _this, "visualizervolume" );
	loadMode.saveSettings( _doc, _this, "loadingalgorithm" );
	loadChnl.saveSettings( _doc, _this, "loadingchannel" );

	oversample.saveSettings( _doc, _this, "oversample" );
	oversampleMode.saveSettings( _doc, _this, "oversamplemode" );
	removeDC.saveSettings( _doc, _this, "removeDC" );

	QString saveString;

	for( int i = 0; i < 8; ++i )
	{
		if( enabled[i]->value() )
		{
			if( updateWavetable[i] )
			{
				updateWavetable[i] = false;
				base64::encode( (const char *)storedwaveforms[i],
					STOREDMAINARRAYLEN * sizeof(float), wavetableSaveStrings[i] );
			}
			_this.setAttribute( "waveforms"+QString::number(i), wavetableSaveStrings[i] );
		}
	}

	for( int i = 0; i < 64; ++i )
	{
		if( subEnabled[i]->value() )
		{
			base64::encode( (const char *)storedsubs[i],
				STOREDSUBWAVELEN * sizeof(float), saveString );
			_this.setAttribute( "subs"+QString::number(i), saveString );
		}
	}

	base64::encode( (const char *)sampGraphs,
		1024 * sizeof(float), saveString );
	_this.setAttribute( "sampGraphs", saveString );

	int sampleSizes[8] = {0};
	for( int i = 0; i < 8; ++i )
	{
		if( sampleEnabled[i]->value() )
		{
			for( int j = 0; j < 2; ++j )
			{
				base64::encode( (const char *)samples[i][j].data(),
					samples[i][j].size() * sizeof(float), saveString );
				_this.setAttribute( "samples_"+QString::number(i)+"_"+QString::number(j), saveString );
			}

			sampleSizes[i] = samples[i][0].size();
		}
	}

	base64::encode( (const char *)sampleSizes,
		8 * sizeof(int), saveString );
	_this.setAttribute( "sampleSizes", saveString );

	for( int i = 0; i < maxMainEnabled; ++i )
	{
		if( enabled[i]->value() )
		{
			morph[i]->saveSettings( _doc, _this, "morph_"+QString::number(i) );
			range[i]->saveSettings( _doc, _this, "range_"+QString::number(i) );
			modify[i]->saveSettings( _doc, _this, "modify_"+QString::number(i) );
			modifyMode[i]->saveSettings( _doc, _this, "modifyMode_"+QString::number(i) );
			unisonVoices[i]->saveSettings( _doc, _this, "unisonVoices_"+QString::number(i) );
			unisonDetune[i]->saveSettings( _doc, _this, "unisonDetune_"+QString::number(i) );
			unisonMorph[i]->saveSettings( _doc, _this, "unisonMorph_"+QString::number(i) );
			unisonModify[i]->saveSettings( _doc, _this, "unisonModify_"+QString::number(i) );
			morphMax[i]->saveSettings( _doc, _this, "morphMax_"+QString::number(i) );
			detune[i]->saveSettings( _doc, _this, "detune_"+QString::number(i) );
			sampLen[i]->saveSettings( _doc, _this, "sampLen_"+QString::number(i) );
			phase[i]->saveSettings( _doc, _this, "phase_"+QString::number(i) );
			phaseRand[i]->saveSettings( _doc, _this, "phaseRand_"+QString::number(i) );
			vol[i]->saveSettings( _doc, _this, "vol_"+QString::number(i) );
			enabled[i]->saveSettings( _doc, _this, "enabled_"+QString::number(i) );
			muted[i]->saveSettings( _doc, _this, "muted_"+QString::number(i) );
			pan[i]->saveSettings( _doc, _this, "pan_"+QString::number(i) );
			keytracking[i]->saveSettings( _doc, _this, "keytracking_"+QString::number(i) );
			tempo[i]->saveSettings( _doc, _this, "tempo_"+QString::number(i) );
			interpolate[i]->saveSettings( _doc, _this, "interpolate_"+QString::number(i) );
		}
	}

	for( int i = 0; i < maxSampleEnabled; ++i )
	{
		if( sampleEnabled[i]->value() )
		{
			sampleEnabled[i]->saveSettings( _doc, _this, "sampleEnabled_"+QString::number(i) );
			sampleGraphEnabled[i]->saveSettings( _doc, _this, "sampleGraphEnabled_"+QString::number(i) );
			sampleMuted[i]->saveSettings( _doc, _this, "sampleMuted_"+QString::number(i) );
			sampleKeytracking[i]->saveSettings( _doc, _this, "sampleKeytracking_"+QString::number(i) );
			sampleLoop[i]->saveSettings( _doc, _this, "sampleLoop_"+QString::number(i) );
			sampleVolume[i]->saveSettings( _doc, _this, "sampleVolume_"+QString::number(i) );
			samplePanning[i]->saveSettings( _doc, _this, "samplePanning_"+QString::number(i) );
			sampleDetune[i]->saveSettings( _doc, _this, "sampleDetune_"+QString::number(i) );
			samplePhase[i]->saveSettings( _doc, _this, "samplePhase_"+QString::number(i) );
			samplePhaseRand[i]->saveSettings( _doc, _this, "samplePhaseRand_"+QString::number(i) );
			sampleStart[i]->saveSettings( _doc, _this, "sampleStart_"+QString::number(i) );
			sampleEnd[i]->saveSettings( _doc, _this, "sampleEnd_"+QString::number(i) );
		}
	}

	for( int i = 0; i < maxFiltEnabled; ++i )
	{
		if( filtEnabled[i]->value() )
		{
			filtInVol[i]->saveSettings( _doc, _this, "filtInVol_"+QString::number(i) );
			filtType[i]->saveSettings( _doc, _this, "filtType_"+QString::number(i) );
			filtSlope[i]->saveSettings( _doc, _this, "filtSlope_"+QString::number(i) );
			filtCutoff[i]->saveSettings( _doc, _this, "filtCutoff_"+QString::number(i) );
			filtReso[i]->saveSettings( _doc, _this, "filtReso_"+QString::number(i) );
			filtGain[i]->saveSettings( _doc, _this, "filtGain_"+QString::number(i) );
			filtSatu[i]->saveSettings( _doc, _this, "filtSatu_"+QString::number(i) );
			filtWetDry[i]->saveSettings( _doc, _this, "filtWetDry_"+QString::number(i) );
			filtBal[i]->saveSettings( _doc, _this, "filtBal_"+QString::number(i) );
			filtOutVol[i]->saveSettings( _doc, _this, "filtOutVol_"+QString::number(i) );
			filtEnabled[i]->saveSettings( _doc, _this, "filtEnabled_"+QString::number(i) );
			filtFeedback[i]->saveSettings( _doc, _this, "filtFeedback_"+QString::number(i) );
			filtDetune[i]->saveSettings( _doc, _this, "filtDetune_"+QString::number(i) );
			filtKeytracking[i]->saveSettings( _doc, _this, "filtKeytracking_"+QString::number(i) );
			filtMuted[i]->saveSettings( _doc, _this, "filtMuted_"+QString::number(i) );
		}
	}

	for( int i = 0; i < maxSubEnabled; ++i )
	{
		if( subEnabled[i]->value() )
		{
			subEnabled[i]->saveSettings( _doc, _this, "subEnabled_"+QString::number(i) );
			subVol[i]->saveSettings( _doc, _this, "subVol_"+QString::number(i) );
			subPhase[i]->saveSettings( _doc, _this, "subPhase_"+QString::number(i) );
			subPhaseRand[i]->saveSettings( _doc, _this, "subPhaseRand_"+QString::number(i) );
			subDetune[i]->saveSettings( _doc, _this, "subDetune_"+QString::number(i) );
			subMuted[i]->saveSettings( _doc, _this, "subMuted_"+QString::number(i) );
			subKeytrack[i]->saveSettings( _doc, _this, "subKeytrack_"+QString::number(i) );
			subSampLen[i]->saveSettings( _doc, _this, "subSampLen_"+QString::number(i) );
			subNoise[i]->saveSettings( _doc, _this, "subNoise_"+QString::number(i) );
			subPanning[i]->saveSettings( _doc, _this, "subPanning_"+QString::number(i) );
			subTempo[i]->saveSettings( _doc, _this, "subTempo_"+QString::number(i) );
			subRateLimit[i]->saveSettings( _doc, _this, "subRateLimit_"+QString::number(i) );
			subUnisonNum[i]->saveSettings( _doc, _this, "subUnisonNum_"+QString::number(i) );
			subUnisonDetune[i]->saveSettings( _doc, _this, "subUnisonDetune_"+QString::number(i) );
			subInterpolate[i]->saveSettings( _doc, _this, "subInterpolate_"+QString::number(i) );
		}
	}

	for( int i = 0; i < maxModEnabled; ++i )
	{
		if( modEnabled[i]->value() )
		{
			modIn[i]->saveSettings( _doc, _this, "modIn_"+QString::number(i) );
			modInNum[i]->saveSettings( _doc, _this, "modInNu"+QString::number(i) );
			modInAmnt[i]->saveSettings( _doc, _this, "modInAmnt_"+QString::number(i) );
			modInCurve[i]->saveSettings( _doc, _this, "modInCurve_"+QString::number(i) );
			modIn2[i]->saveSettings( _doc, _this, "modIn2_"+QString::number(i) );
			modInNum2[i]->saveSettings( _doc, _this, "modInNum2_"+QString::number(i) );
			modInAmnt2[i]->saveSettings( _doc, _this, "modAmnt2_"+QString::number(i) );
			modInCurve2[i]->saveSettings( _doc, _this, "modCurve2_"+QString::number(i) );
			modOutSec[i]->saveSettings( _doc, _this, "modOutSec_"+QString::number(i) );
			modOutSig[i]->saveSettings( _doc, _this, "modOutSig_"+QString::number(i) );
			modOutSecNum[i]->saveSettings( _doc, _this, "modOutSecNu"+QString::number(i) );
			modEnabled[i]->saveSettings( _doc, _this, "modEnabled_"+QString::number(i) );
			modCombineType[i]->saveSettings( _doc, _this, "modCombineType_"+QString::number(i) );
			modType[i]->saveSettings( _doc, _this, "modType_"+QString::number(i) );
			modType2[i]->saveSettings( _doc, _this, "modType2_"+QString::number(i) );
		}
	}

	for( int i = 0; i < 18; ++i )
	{
		macro[i]->saveSettings( _doc, _this, "macro_"+QString::number(i) );
		_this.setAttribute( "macroTooltips_"+QString::number(i), macroTooltips[i] );
		_this.setAttribute( "macroRed_"+QString::number(i), macroColors[i][0] );
		_this.setAttribute( "macroGreen_"+QString::number(i), macroColors[i][1] );
		_this.setAttribute( "macroBlue_"+QString::number(i), macroColors[i][2] );
	}
}


void Microwave::loadSettings( const QDomElement & _this )
{
	QString microwaveVersion = _this.attribute( "version" );

	visvol.loadSettings( _this, "visualizervolume" );
	loadMode.loadSettings( _this, "loadingalgorithm" );
	loadChnl.loadSettings( _this, "loadingchannel" );

	oversample.loadSettings( _this, "oversample" );
	oversampleMode.loadSettings( _this, "oversamplemode" );
	removeDC.loadSettings( _this, "removeDC" );

	graph.setLength( 2048 );

	for( int i = 0; i < 8; ++i )
	{
		enabled[i]->loadSettings( _this, "enabled_"+QString::number(i) );
		if( enabled[i]->value() )
		{
			morph[i]->loadSettings( _this, "morph_"+QString::number(i) );
			range[i]->loadSettings( _this, "range_"+QString::number(i) );
			modify[i]->loadSettings( _this, "modify_"+QString::number(i) );
			modifyMode[i]->loadSettings( _this, "modifyMode_"+QString::number(i) );
			unisonVoices[i]->loadSettings( _this, "unisonVoices_"+QString::number(i) );
			unisonDetune[i]->loadSettings( _this, "unisonDetune_"+QString::number(i) );
			unisonMorph[i]->loadSettings( _this, "unisonMorph_"+QString::number(i) );
			unisonModify[i]->loadSettings( _this, "unisonModify_"+QString::number(i) );
			morphMax[i]->loadSettings( _this, "morphMax_"+QString::number(i) );
			detune[i]->loadSettings( _this, "detune_"+QString::number(i) );
			sampLen[i]->loadSettings( _this, "sampLen_"+QString::number(i) );
			phase[i]->loadSettings( _this, "phase_"+QString::number(i) );
			phaseRand[i]->loadSettings( _this, "phaseRand_"+QString::number(i) );
			vol[i]->loadSettings( _this, "vol_"+QString::number(i) );
			muted[i]->loadSettings( _this, "muted_"+QString::number(i) );
			pan[i]->loadSettings( _this, "pan_"+QString::number(i) );
			keytracking[i]->loadSettings( _this, "keytracking_"+QString::number(i) );
			tempo[i]->loadSettings( _this, "tempo_"+QString::number(i) );
			interpolate[i]->loadSettings( _this, "interpolate_"+QString::number(i) );
		}

		filtEnabled[i]->loadSettings( _this, "filtEnabled_"+QString::number(i) );
		if( filtEnabled[i]->value() )
		{
			filtInVol[i]->loadSettings( _this, "filtInVol_"+QString::number(i) );
			filtType[i]->loadSettings( _this, "filtType_"+QString::number(i) );
			filtSlope[i]->loadSettings( _this, "filtSlope_"+QString::number(i) );
			filtCutoff[i]->loadSettings( _this, "filtCutoff_"+QString::number(i) );
			filtReso[i]->loadSettings( _this, "filtReso_"+QString::number(i) );
			filtGain[i]->loadSettings( _this, "filtGain_"+QString::number(i) );
			filtSatu[i]->loadSettings( _this, "filtSatu_"+QString::number(i) );
			filtWetDry[i]->loadSettings( _this, "filtWetDry_"+QString::number(i) );
			filtBal[i]->loadSettings( _this, "filtBal_"+QString::number(i) );
			filtOutVol[i]->loadSettings( _this, "filtOutVol_"+QString::number(i) );
			filtFeedback[i]->loadSettings( _this, "filtFeedback_"+QString::number(i) );
			filtDetune[i]->loadSettings( _this, "filtDetune_"+QString::number(i) );
			filtKeytracking[i]->loadSettings( _this, "filtKeytracking_"+QString::number(i) );
			filtMuted[i]->loadSettings( _this, "filtMuted_"+QString::number(i) );
		}

		sampleEnabled[i]->loadSettings( _this, "sampleEnabled_"+QString::number(i) );
		if( sampleEnabled[i]->value() )
		{
			sampleGraphEnabled[i]->loadSettings( _this, "sampleGraphEnabled_"+QString::number(i) );
			sampleMuted[i]->loadSettings( _this, "sampleMuted_"+QString::number(i) );
			sampleKeytracking[i]->loadSettings( _this, "sampleKeytracking_"+QString::number(i) );
			sampleLoop[i]->loadSettings( _this, "sampleLoop_"+QString::number(i) );
			sampleVolume[i]->loadSettings( _this, "sampleVolume_"+QString::number(i) );
			samplePanning[i]->loadSettings( _this, "samplePanning_"+QString::number(i) );
			sampleDetune[i]->loadSettings( _this, "sampleDetune_"+QString::number(i) );
			samplePhase[i]->loadSettings( _this, "samplePhase_"+QString::number(i) );
			samplePhaseRand[i]->loadSettings( _this, "samplePhaseRand_"+QString::number(i) );
			sampleStart[i]->loadSettings( _this, "sampleStart_"+QString::number(i) );
			sampleEnd[i]->loadSettings( _this, "sampleEnd_"+QString::number(i) );
		}
	}

	for( int i = 0; i < 18; ++i )
	{
		macro[i]->loadSettings( _this, "macro_"+QString::number(i) );
	}

	for( int i = 0; i < 64; ++i )
	{
		subEnabled[i]->loadSettings( _this, "subEnabled_"+QString::number(i) );
		if( subEnabled[i]->value() )
		{
			subVol[i]->loadSettings( _this, "subVol_"+QString::number(i) );
			subPhase[i]->loadSettings( _this, "subPhase_"+QString::number(i) );
			subPhaseRand[i]->loadSettings( _this, "subPhaseRand_"+QString::number(i) );
			subDetune[i]->loadSettings( _this, "subDetune_"+QString::number(i) );
			subMuted[i]->loadSettings( _this, "subMuted_"+QString::number(i) );
			subKeytrack[i]->loadSettings( _this, "subKeytrack_"+QString::number(i) );
			subSampLen[i]->loadSettings( _this, "subSampLen_"+QString::number(i) );
			subNoise[i]->loadSettings( _this, "subNoise_"+QString::number(i) );
			subPanning[i]->loadSettings( _this, "subPanning_"+QString::number(i) );
			subTempo[i]->loadSettings( _this, "subTempo_"+QString::number(i) );
			subRateLimit[i]->loadSettings( _this, "subRateLimit_"+QString::number(i) );
			subUnisonNum[i]->loadSettings( _this, "subUnisonNum_"+QString::number(i) );
			subUnisonDetune[i]->loadSettings( _this, "subUnisonDetune_"+QString::number(i) );
			subInterpolate[i]->loadSettings( _this, "subInterpolate_"+QString::number(i) );
		}

		modEnabled[i]->loadSettings( _this, "modEnabled_"+QString::number(i) );
		if( modEnabled[i]->value() )
		{
			modIn[i]->loadSettings( _this, "modIn_"+QString::number(i) );
			modInNum[i]->loadSettings( _this, "modInNu"+QString::number(i) );
			modInAmnt[i]->loadSettings( _this, "modInAmnt_"+QString::number(i) );
			modInCurve[i]->loadSettings( _this, "modInCurve_"+QString::number(i) );
			modIn2[i]->loadSettings( _this, "modIn2_"+QString::number(i) );
			modInNum2[i]->loadSettings( _this, "modInNum2_"+QString::number(i) );
			modInAmnt2[i]->loadSettings( _this, "modAmnt2_"+QString::number(i) );
			modInCurve2[i]->loadSettings( _this, "modCurve2_"+QString::number(i) );
			modOutSec[i]->loadSettings( _this, "modOutSec_"+QString::number(i) );
			modOutSig[i]->loadSettings( _this, "modOutSig_"+QString::number(i) );
			modOutSecNum[i]->loadSettings( _this, "modOutSecNu"+QString::number(i) );
			modCombineType[i]->loadSettings( _this, "modCombineType_"+QString::number(i) );
			modType[i]->loadSettings( _this, "modType_"+QString::number(i) );
			modType2[i]->loadSettings( _this, "modType2_"+QString::number(i) );
		}
	}

	int size = 0;
	char * dst = 0;


	for( int j = 0; j < 8; ++j )
	{
		if( enabled[j]->value() )
		{
			base64::decode( _this.attribute( "waveforms"+QString::number(j) ), &dst, &size );
			for( int i = 0; i < STOREDMAINARRAYLEN; ++i )
			{
				storedwaveforms[j][i] = ( (float*) dst )[i];
			}
		}
	}

	for( int j = 0; j < 64; ++j )
	{
		if( subEnabled[j]->value() )
		{
			base64::decode( _this.attribute( "subs"+QString::number(j) ), &dst, &size );
			for( int i = 0; i < STOREDSUBWAVELEN; ++i )
			{
				storedsubs[j][i] = ( (float*) dst )[i];
			}
		}
	}

	base64::decode( _this.attribute( "sampGraphs" ), &dst, &size );
	for( int i = 0; i < 1024; ++i )
	{
		sampGraphs[i] = ( (float*) dst )[i];
	}

	int sampleSizes[8] = {0};
	base64::decode( _this.attribute( "sampleSizes" ), &dst, &size );
	for( int i = 0; i < 8; ++i )
	{
		sampleSizes[i] = ( (int*) dst )[i];
	}

	for( int i = 0; i < 8; ++i )
	{
		if( sampleEnabled[i]->value() )
		{
			for( int j = 0; j < 2; ++j )
			{
				base64::decode( _this.attribute( "samples_"+QString::number(i)+"_"+QString::number(j) ), &dst, &size );
				for( int k = 0; k < sampleSizes[i]; ++k )
				{
					samples[i][j].push_back( ( (float*) dst )[k] );
				}
			}
		}
	}

	for( int i = 0; i < 18; ++i )
	{
		macroTooltips[i] = _this.attribute( "macroTooltips_"+QString::number(i) );
		macroColors[i][0] = _this.attribute( "macroRed_"+QString::number(i) ).toInt();
		macroColors[i][1] = _this.attribute( "macroGreen_"+QString::number(i) ).toInt();
		macroColors[i][2] = _this.attribute( "macroBlue_"+QString::number(i) ).toInt();
	}

	delete[] dst;

	// Interpolate all the wavetables and waveforms when loaded.
	for( int i = 0; i < 8; ++i )
	{
		if( enabled[i]->value() )
		{
			fillMainOsc(i, interpolate[i]->value());
		}
	}

	for( int i = 0; i < 64; ++i )
	{
		if( subEnabled[i]->value() )
		{
			fillSubOsc(i, subInterpolate[i]->value());
		}
	}

}


// When a knob is changed, send the new value to the array holding the knob values, as well as the note values within mSynths already initialized (notes already playing)
void Microwave::valueChanged( int which, int num )
{
	//Send new values to array
	switch( which )
	{
		case 1: morphArr[num] = morph[num]->value(); break;
		case 2: rangeArr[num] = range[num]->value(); break;
		case 3: modifyArr[num] = modify[num]->value(); break;
		case 4: modifyModeArr[num] = modifyMode[num]->value(); break;
		case 5: volArr[num] = vol[num]->value(); break;
		case 6: panArr[num] = pan[num]->value(); break;
		case 7: detuneArr[num] = detune[num]->value(); break;
		case 8: phaseArr[num] = phase[num]->value(); break;
		case 9: phaseRandArr[num] = phaseRand[num]->value(); break;
		case 10: enabledArr[num] = enabled[num]->value(); break;
		case 11: mutedArr[num] = muted[num]->value(); break;
		case 12: sampLenArr[num] = sampLen[num]->value(); break;
		case 13: morphMaxArr[num] = morphMax[num]->value(); break;
		case 14: unisonVoicesArr[num] = unisonVoices[num]->value(); break;
		case 15: unisonDetuneArr[num] = unisonDetune[num]->value(); break;
		case 16: unisonMorphArr[num] = unisonMorph[num]->value(); break;
		case 17: unisonModifyArr[num] = unisonModify[num]->value(); break;
		case 18: keytrackingArr[num] = keytracking[num]->value(); break;
		case 19: tempoArr[num] = tempo[num]->value(); break;
		case 20: interpolateArr[num] = interpolate[num]->value(); break;

		case 30: subEnabledArr[num] = subEnabled[num]->value(); break;
		case 31: subMutedArr[num] = subMuted[num]->value(); break;
		case 32: subKeytrackArr[num] = subKeytrack[num]->value(); break;
		case 33: subNoiseArr[num] = subNoise[num]->value(); break;
		case 34: subVolArr[num] = subVol[num]->value(); break;
		case 35: subPanningArr[num] = subPanning[num]->value(); break;
		case 36: subDetuneArr[num] = subDetune[num]->value(); break;
		case 37: subPhaseArr[num] = subPhase[num]->value(); break;
		case 38: subPhaseRandArr[num] = subPhaseRand[num]->value(); break;
		case 39: subSampLenArr[num] = subSampLen[num]->value(); break;
		case 40: subTempoArr[num] = subTempo[num]->value(); break;
		case 41: subRateLimitArr[num] = subRateLimit[num]->value(); break;
		case 42: subUnisonNumArr[num] = subUnisonNum[num]->value(); break;
		case 43: subUnisonDetuneArr[num] = subUnisonDetune[num]->value(); break;
		case 44: subInterpolateArr[num] = subInterpolate[num]->value(); break;

		case 60: sampleEnabledArr[num] = sampleEnabled[num]->value(); break;
		case 61: sampleMutedArr[num] = sampleMuted[num]->value(); break;
		case 62: sampleKeytrackingArr[num] = sampleKeytracking[num]->value(); break;
		case 63: sampleGraphEnabledArr[num] = sampleGraphEnabled[num]->value(); break;
		case 64: sampleLoopArr[num] = sampleLoop[num]->value(); break;
		case 65: sampleVolumeArr[num] = sampleVolume[num]->value(); break;
		case 66: samplePanningArr[num] = samplePanning[num]->value(); break;
		case 67: sampleDetuneArr[num] = sampleDetune[num]->value(); break;
		case 68: samplePhaseArr[num] = samplePhase[num]->value(); break;
		case 69: samplePhaseRandArr[num] = samplePhaseRand[num]->value(); break;
		case 70: sampleStartArr[num] = sampleStart[num]->value(); break;
		case 71: sampleEndArr[num] = sampleEnd[num]->value(); break;

		case 90: modInArr[num] = modIn[num]->value(); break;
		case 91: modInNumArr[num] = modInNum[num]->value(); break;
		case 92: modInAmntArr[num] = modInAmnt[num]->value(); break;
		case 93: modInCurveArr[num] = modInCurve[num]->value(); break;
		case 94: modIn2Arr[num] = modIn2[num]->value(); break;
		case 95: modInNum2Arr[num] = modInNum2[num]->value(); break;
		case 96: modInAmnt2Arr[num] = modInAmnt2[num]->value(); break;
		case 97: modInCurve2Arr[num] = modInCurve2[num]->value(); break;
		case 98: modOutSecArr[num] = modOutSec[num]->value(); break;
		case 99: modOutSigArr[num] = modOutSig[num]->value(); break;
		case 100: modOutSecNumArr[num] = modOutSecNum[num]->value(); break;
		case 101: modEnabledArr[num] = modEnabled[num]->value(); break;
		case 102: modCombineTypeArr[num] = modCombineType[num]->value(); break;
		case 103: modTypeArr[num] = modType[num]->value(); break;
		case 104: modType2Arr[num] = modType2[num]->value(); break;

		case 120: filtCutoffArr[num] = filtCutoff[num]->value(); break;
		case 121: filtResoArr[num] = filtReso[num]->value(); break;
		case 122: filtGainArr[num] = filtGain[num]->value(); break;
		case 123: filtTypeArr[num] = filtType[num]->value(); break;
		case 124: filtSlopeArr[num] = filtSlope[num]->value(); break;
		case 125: filtInVolArr[num] = filtInVol[num]->value(); break;
		case 126: filtOutVolArr[num] = filtOutVol[num]->value(); break;
		case 127: filtWetDryArr[num] = filtWetDry[num]->value(); break;
		case 128: filtBalArr[num] = filtBal[num]->value(); break;
		case 129: filtSatuArr[num] = filtSatu[num]->value(); break;
		case 130: filtFeedbackArr[num] = filtFeedback[num]->value(); break;
		case 131: filtDetuneArr[num] = filtDetune[num]->value(); break;
		case 132: filtEnabledArr[num] = filtEnabled[num]->value(); break;
		case 133: filtMutedArr[num] = filtMuted[num]->value(); break;
		case 134: filtKeytrackingArr[num] = filtKeytracking[num]->value(); break;

		case 150: macroArr[num] = macro[num]->value(); break;
	}

	nphList = NotePlayHandle::nphsOfInstrumentTrack( microwaveTrack, true );

	for( int i = 0; i < nphList.length(); ++i )
	{
		mSynth * ps = static_cast<mSynth *>( nphList[i]->m_pluginData );

		if( ps )// Makes sure "ps" isn't assigned a null value, if m_pluginData hasn't been created yet.
		{
			//Send new knob values to notes already playing
			switch( which )
			{
				case 1: ps->morph[num] = morph[num]->value(); break;
				case 2: ps->range[num] = range[num]->value(); break;
				case 3: ps->modify[num] = modify[num]->value(); break;
				case 4: ps->modifyMode[num] = modifyMode[num]->value(); break;
				case 5: ps->vol[num] = vol[num]->value(); break;
				case 6: ps->pan[num] = pan[num]->value(); break;
				case 7: ps->detune[num] = detune[num]->value(); break;
				case 8: ps->phase[num] = phase[num]->value(); break;
				case 9: ps->phaseRand[num] = phaseRand[num]->value(); break;
				case 10: ps->enabled[num] = enabled[num]->value(); break;
				case 11: ps->muted[num] = muted[num]->value(); break;
				case 12: ps->sampLen[num] = sampLen[num]->value(); break;
				case 13: ps->morphMax[num] = morphMax[num]->value(); break;
				case 14: ps->unisonVoices[num] = unisonVoices[num]->value(); break;
				case 15: ps->unisonDetune[num] = unisonDetune[num]->value(); break;
				case 16: ps->unisonMorph[num] = unisonMorph[num]->value(); break;
				case 17: ps->unisonModify[num] = unisonModify[num]->value(); break;
				case 18: ps->keytracking[num] = keytracking[num]->value(); break;
				case 19: ps->tempo[num] = tempo[num]->value(); break;

				case 30: ps->subEnabled[num] = subEnabled[num]->value(); break;
				case 31: ps->subMuted[num] = subMuted[num]->value(); break;
				case 32: ps->subKeytrack[num] = subKeytrack[num]->value(); break;
				case 33: ps->subNoise[num] = subNoise[num]->value(); break;
				case 34: ps->subVol[num] = subVol[num]->value(); break;
				case 35: ps->subPanning[num] = subPanning[num]->value(); break;
				case 36: ps->subDetune[num] = subDetune[num]->value(); break;
				case 37: ps->subPhase[num] = subPhase[num]->value(); break;
				case 38: ps->subPhaseRand[num] = subPhaseRand[num]->value(); break;
				case 39: ps->subSampLen[num] = subSampLen[num]->value(); break;
				case 40: ps->subTempo[num] = subTempo[num]->value(); break;
				case 41: ps->subRateLimit[num] = subRateLimit[num]->value(); break;
				case 42: ps->subUnisonNum[num] = subUnisonNum[num]->value(); break;
				case 43: ps->subUnisonDetune[num] = subUnisonDetune[num]->value(); break;
				case 44: ps->subInterpolate[num] = subInterpolate[num]->value(); break;

				case 60: ps->sampleEnabled[num] = sampleEnabled[num]->value(); break;
				case 61: ps->sampleMuted[num] = sampleMuted[num]->value(); break;
				case 62: ps->sampleKeytracking[num] = sampleKeytracking[num]->value(); break;
				case 63: ps->sampleGraphEnabled[num] = sampleGraphEnabled[num]->value(); break;
				case 64: ps->sampleLoop[num] = sampleLoop[num]->value(); break;
				case 65: ps->sampleVolume[num] = sampleVolume[num]->value(); break;
				case 66: ps->samplePanning[num] = samplePanning[num]->value(); break;
				case 67: ps->sampleDetune[num] = sampleDetune[num]->value(); break;
				case 68: ps->samplePhase[num] = samplePhase[num]->value(); break;
				case 69: ps->samplePhaseRand[num] = samplePhaseRand[num]->value(); break;
				case 70: ps->sampleStart[num] = sampleStart[num]->value(); break;
				case 71: ps->sampleEnd[num] = sampleEnd[num]->value(); break;

				case 90: ps->modIn[num] = modIn[num]->value(); break;
				case 91: ps->modInNum[num] = modInNum[num]->value(); break;
				case 92: ps->modInAmnt[num] = modInAmnt[num]->value(); break;
				case 93: ps->modInCurve[num] = modInCurve[num]->value(); break;
				case 94: ps->modIn2[num] = modIn2[num]->value(); break;
				case 95: ps->modInNum2[num] = modInNum2[num]->value(); break;
				case 96: ps->modInAmnt2[num] = modInAmnt2[num]->value(); break;
				case 97: ps->modInCurve2[num] = modInCurve2[num]->value(); break;
				case 98: ps->modOutSec[num] = modOutSec[num]->value(); break;
				case 99: ps->modOutSig[num] = modOutSig[num]->value(); break;
				case 100: ps->modOutSecNum[num] = modOutSecNum[num]->value(); break;
				case 101: ps->modEnabled[num] = modEnabled[num]->value(); break;
				case 102: ps->modCombineType[num] = modCombineType[num]->value(); break;
				case 103: ps->modType[num] = modType[num]->value(); break;
				case 104: ps->modType2[num] = modType2[num]->value(); break;

				case 120: ps->filtCutoff[num] = filtCutoff[num]->value(); break;
				case 121: ps->filtReso[num] = filtReso[num]->value(); break;
				case 122: ps->filtGain[num] = filtGain[num]->value(); break;
				case 123: ps->filtType[num] = filtType[num]->value(); break;
				case 124: ps->filtSlope[num] = filtSlope[num]->value(); break;
				case 125: ps->filtInVol[num] = filtInVol[num]->value(); break;
				case 126: ps->filtOutVol[num] = filtOutVol[num]->value(); break;
				case 127: ps->filtWetDry[num] = filtWetDry[num]->value(); break;
				case 128: ps->filtBal[num] = filtBal[num]->value(); break;
				case 129: ps->filtSatu[num] = filtSatu[num]->value(); break;
				case 130: ps->filtFeedback[num] = filtFeedback[num]->value(); break;
				case 131: ps->filtDetune[num] = filtDetune[num]->value(); break;
				case 132: ps->filtEnabled[num] = filtEnabled[num]->value(); break;
				case 133: ps->filtMuted[num] = filtMuted[num]->value(); break;
				case 134: ps->filtKeytracking[num] = filtKeytracking[num]->value(); break;

				case 150: ps->macro[num] = macro[num]->value(); break;
			}
		}
	}
}


// Set the range of Morph based on Morph Max
void Microwave::morphMaxChanged( int i )
{
	morph[i]->setRange( morph[i]->minValue(), morphMax[i]->value(), morph[i]->step<float>() );
	unisonMorph[i]->setRange( unisonMorph[i]->minValue(), morphMax[i]->value(), unisonMorph[i]->step<float>() );
}


// Set the range of morphMax and Modify based on new sample length
void Microwave::sampLenChanged( int i )
{
	morphMax[i]->setRange( morphMax[i]->minValue(), STOREDMAINARRAYLEN / sampLen[i]->value() - 2, morphMax[i]->step<float>() );
	modify[i]->setRange( modify[i]->minValue(), sampLen[i]->value() - 1, modify[i]->step<float>() );
	unisonModify[i]->setRange( unisonModify[i]->minValue(), sampLen[i]->value() - 1, unisonModify[i]->step<float>() );
}


//Change graph length to sample length
void Microwave::subSampLenChanged( int num )
{
	if( scroll == 1 && subNum.value() == num + 1 )
	{
		graph.setLength( subSampLen[num]->value() );
	}
}


//Stores the highest enabled main oscillator.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::mainEnabledChanged( int num )
{
	for( int i = 0; i < 8; ++i )
	{
		if( enabled[i]->value() )
		{
			maxMainEnabled = i + 1;
		}
	}
}


//Stores the highest enabled sub oscillator.  Helps with major CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::subEnabledChanged( int num )
{
	for( int i = 0; i < 64; ++i )
	{
		if( subEnabled[i]->value() )
		{
			maxSubEnabled = i + 1;
		}
	}
}


//Stores the highest enabled sample.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::sampleEnabledChanged( int num )
{
	for( int i = 0; i < 8; ++i )
	{
		if( sampleEnabled[i]->value() )
		{
			maxSampleEnabled = i + 1;
		}
	}
}


//Stores the highest enabled mod section.  Helps with major CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::modEnabledChanged( int num )
{
	for( int i = 0; i < 64; ++i )
	{
		if( modEnabled[i]->value() )
		{
			maxModEnabled = i + 1;
		}
	}
}


//Stores the highest enabled filter section.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::filtEnabledChanged( int num )
{
	for( int i = 0; i < 8; ++i )
	{
		if( filtEnabled[i]->value() )
		{
			maxFiltEnabled = i + 1;
		}
	}
}


//Updates sub oscillator inteprolation when the interpolation LED is changed.
void Microwave::interpolateChanged( int num )
{
	fillMainOsc( num, interpolate[num]->value() );
}


//Updates sub oscillator inteprolation when the interpolation LED is changed.
void Microwave::subInterpolateChanged( int num )
{
	fillSubOsc( num, subInterpolate[num]->value() );
}


//When user drawn on graph, send new values to the correct arrays
void Microwave::samplesChanged( int _begin, int _end )
{
	switch( (int)scroll )
	{
		case 0:
		{
			break;
		}
		case 1:
		{
			for( int i = _begin; i <= _end; ++i )
			{
				storedsubs[subNum.value()-1][i] = graph.samples()[i];
				for( int j = 0; j < WAVERATIO; ++j )
				{
					// Puts low-quality samples into there so one can change the waveform mid-note.  The quality boost will occur at another time.
					// It cannot do the interpolation here because it causes lag.
					subs[subNum.value()-1][i*WAVERATIO+j] = graph.samples()[i];
				}
			}

			subFilled[subNum.value()-1] = false;// Make sure the waveform is interpolated later on.

			// If the entire graph was changed all at once, we can assume it isn't from the user drawing on the graph,
			// so we can interpolate the oscillator without worrying about lag.
			if( _begin == 0 && _end == STOREDSUBWAVELEN - 1 )
			{
				fillSubOsc(subNum.value()-1, subInterpolate[subNum.value()-1]->value());
			}
			break;
		}
		case 2:
		{
			for( int i = _begin; i <= _end; ++i )
			{
				sampGraphs[i + ( (sampNum.value()-1) * 128 )] = graph.samples()[i];
			}
			break;
		}
	}
}


void Microwave::switchMatrixSections( int source, int destination )
{
	int modInTemp = modInArr[destination];
	int modInNumTemp = modInNumArr[destination];
	float modInAmntTemp = modInAmntArr[destination];
	float modInCurveTemp = modInCurveArr[destination];
	int modIn2Temp = modIn2Arr[destination];
	int modInNum2Temp = modInNum2Arr[destination];
	float modInAmnt2Temp = modInAmnt2Arr[destination];
	float modInCurve2Temp = modInCurve2Arr[destination];
	int modOutSecTemp = modOutSecArr[destination];
	int modOutSigTemp = modOutSigArr[destination];
	int modOutSecNumTemp = modOutSecNumArr[destination];
	bool modEnabledTemp = modEnabledArr[destination];
	int modCombineTypeTemp = modCombineTypeArr[destination];
	bool modTypeTemp = modTypeArr[destination];
	bool modType2Temp = modType2Arr[destination];

	modIn[destination]->setValue( modInArr[source] );
	modInNum[destination]->setValue( modInNumArr[source] );
	modInAmnt[destination]->setValue( modInAmntArr[source] );
	modInCurve[destination]->setValue( modInCurveArr[source] );
	modIn2[destination]->setValue( modIn2Arr[source] );
	modInNum2[destination]->setValue( modInNum2Arr[source] );
	modInAmnt2[destination]->setValue( modInAmnt2Arr[source] );
	modInCurve2[destination]->setValue( modInCurve2Arr[source] );
	modOutSec[destination]->setValue( modOutSecArr[source] );
	modOutSig[destination]->setValue( modOutSigArr[source] );
	modOutSecNum[destination]->setValue( modOutSecNumArr[source] );
	modEnabled[destination]->setValue( modEnabledArr[source] );
	modCombineType[destination]->setValue( modCombineTypeArr[source] );
	modType[destination]->setValue( modTypeArr[source] );
	modType2[destination]->setValue( modType2Arr[source] );

	modIn[source]->setValue( modInTemp );
	modInNum[source]->setValue( modInNumTemp );
	modInAmnt[source]->setValue( modInAmntTemp );
	modInCurve[source]->setValue( modInCurveTemp );
	modIn2[source]->setValue( modIn2Temp );
	modInNum2[source]->setValue( modInNum2Temp );
	modInAmnt2[source]->setValue( modInAmnt2Temp );
	modInCurve2[source]->setValue( modInCurve2Temp );
	modOutSec[source]->setValue( modOutSecTemp );
	modOutSig[source]->setValue( modOutSigTemp );
	modOutSecNum[source]->setValue( modOutSecNumTemp );
	modEnabled[source]->setValue( modEnabledTemp );
	modCombineType[source]->setValue( modCombineTypeTemp );
	modType[source]->setValue( modTypeTemp );
	modType2[source]->setValue( modType2Temp );

	// If something is sent to a matrix box and the matrix box is moved, we want to make sure it's still attached to the same box after it is moved.
	for( int i = 0; i < 64; ++i )
	{
		if( modOutSec[i]->value() == 4 )// Output is being sent to Matrix
		{
			if( modOutSecNum[i]->value() - 1 == source )// Output was being sent a matrix box that was moved
			{
				modOutSecNum[i]->setValue( destination + 1 );
			}
			else if( modOutSecNum[i]->value() - 1 == destination )// Output was being sent a matrix box that was moved
			{
				modOutSecNum[i]->setValue( source + 1 );
			}
		}
	}
}


// For when notes are playing.  This initializes a new mSynth if the note is new.  It also uses mSynth::nextStringSample to get the synthesizer output.  This is where oversampling and the visualizer are handled.
void Microwave::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer )
{

	if ( _n->m_pluginData == NULL || _n->totalFramesPlayed() == 0 )
	{
		_n->m_pluginData = new mSynth(
			_n,
			morphArr, rangeArr, modifyArr, modifyModeArr, volArr, panArr, detuneArr, phaseArr, phaseRandArr, enabledArr, mutedArr,
			sampLenArr, morphMaxArr, unisonVoicesArr, unisonDetuneArr, unisonMorphArr, unisonModifyArr, keytrackingArr, tempoArr, interpolateArr,
			subEnabledArr, subMutedArr, subKeytrackArr, subNoiseArr, subVolArr, subPanningArr, subDetuneArr, subPhaseArr, subPhaseRandArr,
			subSampLenArr, subTempoArr, subRateLimitArr, subUnisonNumArr, subUnisonDetuneArr, subInterpolateArr,
			sampleEnabledArr, sampleMutedArr, sampleKeytrackingArr, sampleGraphEnabledArr, sampleLoopArr, sampleVolumeArr, samplePanningArr,
			sampleDetuneArr, samplePhaseArr, samplePhaseRandArr, sampleStartArr, sampleEndArr,
			modInArr, modInNumArr, modInAmntArr, modInCurveArr, modIn2Arr, modInNum2Arr, modInAmnt2Arr, modInCurve2Arr,
			modOutSecArr, modOutSigArr, modOutSecNumArr, modEnabledArr, modCombineTypeArr, modTypeArr, modType2Arr,
			filtCutoffArr, filtResoArr, filtGainArr, filtTypeArr, filtSlopeArr, filtInVolArr, filtOutVolArr, filtWetDryArr, filtBalArr,
			filtSatuArr, filtFeedbackArr, filtDetuneArr, filtEnabledArr, filtMutedArr, filtKeytrackingArr,
			macroArr,
			samples );
		mwc = dynamic_cast<Microwave *>(_n->instrumentTrack()->instrument());

		for( int i = 0; i < 64; ++i )
		{
			if( subEnabled[i]->value() )
			{
				if( !subFilled[i] )
				{
					fillSubOsc(i, subInterpolate[i]->value());
				}
			}
		}
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	mSynth * ps = static_cast<mSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		sampleFrame outputSample;
		sampleFrame totalOutputSample = {0, 0};

		switch( oversampleMode.value() )
		{
			case 0:
			{
				// Process some samples and ignore the output, depending on the oversampling value.  For example, if the oversampling is set to 4x, it will process 4 samples and output 1 of those.
				for( int i = 0; i < oversample.value() + 1; ++i )
				{
					ps->nextStringSample( outputSample, waveforms, subs, sampGraphs, samples, maxFiltEnabled, maxModEnabled, maxSubEnabled, maxSampleEnabled, maxMainEnabled, Engine::mixer()->processingSampleRate() * ( oversample.value() + 1 ), mwc, removeDC.value(), !!i, storedsubs );
				}
				totalOutputSample[0] = outputSample[0];
				totalOutputSample[1] = outputSample[1];

				break;
			}
			case 1:
			{
				// Process some number of samples and average them together, depending on the oversampling value.
				for( int i = 0; i < oversample.value() + 1; ++i )
				{
					ps->nextStringSample( outputSample, waveforms, subs, sampGraphs, samples, maxFiltEnabled, maxModEnabled, maxSubEnabled, maxSampleEnabled, maxMainEnabled, Engine::mixer()->processingSampleRate() * ( oversample.value() + 1 ), mwc, removeDC.value(), !!i, storedsubs );
					totalOutputSample[0] += outputSample[0];
					totalOutputSample[1] += outputSample[1];
				}

				totalOutputSample[0] /= ( oversample.value() + 1 );
				totalOutputSample[1] /= ( oversample.value() + 1 );

				break;
			}
		}

		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			//Send to output
			_working_buffer[frame][chnl] = totalOutputSample[chnl];
		}

		//update visualizer
		if( viewOpen && visualize.value() && scroll == 0 && ps->enabled[mainNum.value()-1] )
		{
			visualizerValues[int( ( ps->sample_realindex[mainNum.value()-1][0] / ( ps->sampLen[mainNum.value()-1] * WAVERATIO ) ) * 204.f)] = ps->mainsample[mainNum.value()-1][0] * visvol.value() * 0.01f;
			if( ps->noteDuration % 1470 == 1 )// Updates around 30 times per second (per note because I'm lazy I guess?)
			{
				graph.setSamples( visualizerValues );
			}
		}
	}

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}


void Microwave::deleteNotePluginData( NotePlayHandle * _n )
{
}


// Fill subs using storedsubs, usually (but not always) making use of libsamplerate for some awesome sinc interpolation.
inline void Microwave::fillSubOsc( int which, bool doInterpolate )
{
	if( doInterpolate )
	{
		srccpy( subs[which], const_cast<float*>( storedsubs[which] ), STOREDSUBWAVELEN );
	}
	else
	{
		for( int i = 0; i < STOREDSUBWAVELEN; ++i )
		{
			for( int j = 0; j < WAVERATIO; ++j )
			{
				subs[which][i*WAVERATIO+j] = storedsubs[which][i];
			}
		}
	}
	subFilled[which] = true;
}



// Fill subs using storedsubs, usually (but not always) making use of libsamplerate for some awesome sinc interpolation.
inline void Microwave::fillMainOsc( int which, bool doInterpolate )
{
	if( doInterpolate )
	{
		srccpy( waveforms[which], const_cast<float*>( storedwaveforms[which] ), STOREDMAINARRAYLEN );
	}
	else
	{
		for( int i = 0; i < STOREDMAINARRAYLEN; ++i )
		{
			for( int j = 0; j < WAVERATIO; ++j )
			{
				waveforms[which][i*WAVERATIO+j] = storedwaveforms[which][i];
			}
		}
	}
	mainFilled[which] = true;
}




/*

          ____                                                                                                                                
        ,'  , `.                                                                                                                              
     ,-+-,.' _ |  ,--,                                                                                 ,---.  ,--,                            
  ,-+-. ;   , ||,--.'|              __  ,-.   ,---.           .---.                                   /__./|,--.'|                      .---. 
 ,--.'|'   |  ;||  |,             ,' ,'/ /|  '   ,'\         /. ./|                .---.         ,---.;  ; ||  |,                      /. ./| 
|   |  ,', |  ':`--'_       ,---. '  | |' | /   /   |     .-'-. ' |  ,--.--.     /.  ./|   ,---./___/ \  | |`--'_       ,---.       .-'-. ' | 
|   | /  | |  ||,' ,'|     /     \|  |   ,'.   ; ,. :    /___/ \: | /       \  .-' . ' |  /     \   ;  \ ' |,' ,'|     /     \     /___/ \: | 
'   | :  | :  |,'  | |    /    / ''  :  /  '   | |: : .-'.. '   ' ..--.  .-. |/___/ \: | /    /  \   \  \: |'  | |    /    /  | .-'.. '   ' . 
;   . |  ; |--' |  | :   .    ' / |  | '   '   | .; :/___/ \:     ' \__\/: . ..   \  ' ..    ' / |;   \  ' .|  | :   .    ' / |/___/ \:     ' 
|   : |  | ,    '  : |__ '   ; :__;  : |   |   :    |.   \  ' .\    ," .--.; | \   \   ''   ;   /| \   \   ''  : |__ '   ;   /|.   \  ' .\    
|   : '  |/     |  | '.'|'   | '.'|  , ;    \   \  /  \   \   ' \ |/  /  ,.  |  \   \   '   |  / |  \   `  ;|  | '.'|'   |  / | \   \   ' \ | 
;   | |`-'      ;  :    ;|   :    :---'      `----'    \   \  |--";  :   .'   \  \   \ ||   :    |   :   \ |;  :    ;|   :    |  \   \  |--"  
|   ;/          |  ,   /  \   \  /                      \   \ |   |  ,     .-./   '---"  \   \  /     '---" |  ,   /  \   \  /    \   \ |     
'---'            ---`-'    `----'                        '---"     `--`---'               `----'             ---`-'    `----'      '---"      
                                                                                                                                              


*/



// Creates the Microwave GUI.  Creates all GUI elements.  Connects some events to some functions.  Calls updateScroll() to put all of the GUI elements in their correct positions.
MicrowaveView::MicrowaveView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );

	setMouseTracking( true );

	setAcceptDrops( true );

	pal.setBrush( backgroundRole(), tab1ArtworkImg );
	setPalette( pal );

	QWidget * view = new QWidget( _parent );
	view->setFixedSize( 250, 250 );

	QPixmap filtBoxesImg = PLUGIN_NAME::getIconPixmap("filterBoxes");
	filtBoxesLabel = new QLabel(this);
	filtBoxesLabel->setPixmap(filtBoxesImg);
	filtBoxesLabel->setAttribute( Qt::WA_TransparentForMouseEvents );

	QPixmap matrixBoxesImg = PLUGIN_NAME::getIconPixmap("matrixBoxes");
	matrixBoxesLabel = new QLabel(this);
	matrixBoxesLabel->setPixmap(matrixBoxesImg);
	matrixBoxesLabel->setAttribute( Qt::WA_TransparentForMouseEvents );


	makeknob( morphKnob, knobColored, tr( "Morph" ), tr( "The Morph knob chooses which waveform out of the wavetable to play." ) );

	makeknob( rangeKnob, knobColored, tr( "Range" ), tr( "The Range knob interpolates (triangularly) the waveforms near the waveform selected by the Morph knob." ) );

	makeknob( modifyKnob, knobColored, tr( "Modify" ), tr( "The Modify knob warps the wavetable realtime using super math powers.  The formula depends on the Modify Mode dropdown box." ) );

	makeknob( detuneKnob, knobSmallColored, tr( "Detune" ), tr( "This knob changes the pitch of the oscillator, in cents." ) );

	makeknob( phaseKnob, knobSmallColored, tr( "Phase" ), tr( "This knob changes the phase (starting position) of the oscillator." ) );

	makeknob( volKnob, knobColored, tr( "Volume" ), tr( "This knob changes the volume.  What a surprise!" ) );

	makeknob( panKnob, knobColored, tr( "Panning" ), tr( "This knob lowers the volume in one ear by an amount depending on this knob's value." ) );

	makeknob( sampLenKnob, knobColored, tr( "Waveform Sample Length" ), tr( "This knob changes the number of samples per waveform there are in the wavetable.  This is useful for finetuning the wavetbale if the wavetable loading was slightly off." ) );

	makeknob( morphMaxKnob, knobColored, tr( "Morph Max" ), tr( "This knob sets the maximum value of the Morph knob." ) );

	makeknob( phaseRandKnob, knobSmallColored, tr( "Phase Randomness" ), tr( "This knob chooses a random phase for every note and unison voice.  The phase change will never be larger than this knob's value." ) );

	makeknob( unisonVoicesKnob, knobSmallColored, tr( "Unison Voices" ), tr( "This knob clones this oscillator multiple times depending on its value, and makes slight changes to the clones depending on the other unison-related knobs." ) );

	makeknob( unisonDetuneKnob, knobSmallColored, tr( "Unison Detune" ), tr( "This knob detunes every unison voice by a random number that is less than the specified amount." ) );

	makeknob( unisonMorphKnob, knobSmallColored, tr( "Unison Morph" ), tr( "This knob changes the wavetable position of each individual unison voice." ) );

	makeknob( unisonModifyKnob, knobSmallColored, tr( "Unison Modify" ), tr( "This knob changes the Modify value of each individual unison voice." ) );

	makeknob( tempoKnob, knobSmallColored, tr( "Tempo Sync" ), tr( "When this knob is anything other than 0, the oscillator is tempo synced to the specified tempo.  This is meant for the creation of envelopes/LFOs/step sequencers in the Matrix tab, and you'll most likely want to enable the Muted LED when using this." ) );

	modifyModeBox = new ComboBox( this );
	modifyModeBox->setGeometry( 0, 5, 42, 22 );
	modifyModeBox->setFont( pointSize<8>( modifyModeBox->font() ) );
	ToolTip::add( modifyModeBox, tr( "The Modify Mode dropdown box chooses the formula for the Modify knob to use to warp the wavetable realtime in cool-sounding ways." ) );

	enabledToggle = new LedCheckBox( "", this, tr( "Oscillator Enabled" ), LedCheckBox::Green );
	ToolTip::add( enabledToggle, tr( "This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!" ) );

	mutedToggle = new LedCheckBox( "", this, tr( "Oscillator Muted" ), LedCheckBox::Green );
	ToolTip::add( mutedToggle, tr( "This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output." ) );

	keytrackingToggle = new LedCheckBox( "", this, tr( "Keytracking" ), LedCheckBox::Green );
	ToolTip::add( keytrackingToggle, tr( "This button turns keytracking on/off.  Without keytracking, the frequency will be 440 Hz by default, and will ignore the frequency of the played note, but will still follow other methods of detuning the sound." ) );

	interpolateToggle = new LedCheckBox( "", this, tr( "Interpolation Enabled" ), LedCheckBox::Green );
	ToolTip::add( interpolateToggle, tr( "This button turns sinc interpolation on/off.  Interpolation uses no CPU and makes the oscillator super duper high-quality.  You'll probably only ever want to turn this off when you're using small waveform lengths and you don't want the waveform smoothed over." ) );


	sampleEnabledToggle = new LedCheckBox( "", this, tr( "Sample Enabled" ), LedCheckBox::Green );
	ToolTip::add( sampleEnabledToggle, tr( "This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!" ) );
	sampleGraphEnabledToggle = new LedCheckBox( "", this, tr( "Sample Graph Enabled" ), LedCheckBox::Green );
	ToolTip::add( sampleGraphEnabledToggle, tr( "This button enables the graph for this oscillator.  On the graph, left/right is time and up/down is position in the sample.  A saw wave in the graph will play the sample normally." ) );
	sampleMutedToggle = new LedCheckBox( "", this, tr( "Sample Muted" ), LedCheckBox::Green );
	ToolTip::add( sampleMutedToggle, tr( "This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output." ) );
	sampleKeytrackingToggle = new LedCheckBox( "", this, tr( "Sample Keytracking" ), LedCheckBox::Green );
	ToolTip::add( sampleKeytrackingToggle, tr( "This button turns keytracking on/off.  Without keytracking, the frequency will be 440 Hz by default, and will ignore the frequency of the played note, but will still follow other methods of detuning the sound." ) );
	sampleLoopToggle = new LedCheckBox( "", this, tr( "Loop Sample" ), LedCheckBox::Green );
	ToolTip::add( sampleLoopToggle, tr( "This button turns looping on/off.  When looping is on, the sample will go back to the starting position when it is done playing." ) );

	makeknob( sampleVolumeKnob, knobColored, tr( "Volume" ), tr( "This, like most other volume knobs, controls the volume." ) );

	makeknob( samplePanningKnob, knobColored, tr( "Panning" ), tr( "This knob lowers the volume in one ear by an amount depending on this knob's value." ) );

	makeknob( sampleDetuneKnob, knobSmallColored, tr( "Detune" ), tr( "This knob changes the pitch (and speed) of the sample." ) );

	makeknob( samplePhaseKnob, knobSmallColored, tr( "Phase" ), tr( "This knob changes the position of the sample, and is updated realtime when automated." ) );

	makeknob( samplePhaseRandKnob, knobSmallColored, tr( "Phase Randomness" ), tr( "This knob makes the sample start at a random position with every note." ) );

	makeknob( sampleStartKnob, knobSmallColored, tr( "Start" ), tr( "This knob changes the starting position of the sample." ) );

	makeknob( sampleEndKnob, knobSmallColored, tr( "End" ), tr( "This knob changes the ending position of the sample." ) );


	for( int i = 0; i < 8; ++i )
	{
		makeknob( filtInVolKnob[i], knobSmallColored, tr( "Input Volume" ), tr( "This knob changes the input volume of the filter." ) );

		makeknob( filtCutoffKnob[i], knobSmallColored, tr( "Cutoff Frequency" ), tr( "This knob changes cutoff frequency of the filter." ) );

		makeknob( filtResoKnob[i], knobSmallColored, tr( "Resonance" ), tr( "This knob changes the resonance of the filter." ) );

		makeknob( filtGainKnob[i], knobSmallColored, tr( "db Gain" ), tr( "This knob changes the gain of the filter.  This only applies to some filter types, e.g. Peak, High Shelf, Low Shelf, etc." ) );

		makeknob( filtSatuKnob[i], knobSmallColored, tr( "Saturation" ), tr( "This knob applies some basic distortion after the filter is applied." ) );

		makeknob( filtWetDryKnob[i], knobSmallColored, tr( "Wet/Dry" ), tr( "This knob allows mixing the filtered signal with the unfiltered signal." ) );

		makeknob( filtBalKnob[i], knobSmallColored, tr( "Balance/Panning" ), tr( "This knob decreases the Wet and increases the Dry of the filter in one ear, depending on the knob's value." ) );

		makeknob( filtOutVolKnob[i], knobSmallColored, tr( "Output Volume" ), tr( "This knob changes the output volume of the filter." ) );

		makeknob( filtFeedbackKnob[i], knobSmallColored, tr( "Feedback" ), tr( "This knob sends the specified portion of the filter output back into the input after a certain delay.  This delay effects the pitch, and can be changed using the filter's Detune and Keytracking options." ) );

		makeknob( filtDetuneKnob[i], knobSmallColored, tr( "Detune (Feedback Delay)" ), tr( "This knob changes the delay of the filter's feedback to match the specified pitch." ) );

		filtTypeBox[i] = new ComboBox( this );
		filtTypeBox[i]->setGeometry( 1000, 5, 42, 22 );
		filtTypeBox[i]->setFont( pointSize<8>( filtTypeBox[i]->font() ) );
		ToolTip::add( filtTypeBox[i], tr( "This dropdown box changes the filter type." ) );

		filtSlopeBox[i] = new ComboBox( this );
		filtSlopeBox[i]->setGeometry( 1000, 5, 42, 22 );
		filtSlopeBox[i]->setFont( pointSize<8>( filtSlopeBox[i]->font() ) );
		ToolTip::add( filtSlopeBox[i], tr( "This dropdown box changes how many times the audio is run through the filter (which changes the slope).  For example, a sound run through a 12 db filter three times will result in a 36 db slope." ) );

		filtEnabledToggle[i] = new LedCheckBox( "", this, tr( "Filter Enabled" ), LedCheckBox::Green );
		ToolTip::add( filtEnabledToggle[i], tr( "This button enables the filter.  A disabled filter will never do anything and does not use any CPU.  In many cases, the settings of a disabled filter will not be saved, so be careful!" ) );

		filtKeytrackingToggle[i] = new LedCheckBox( "", this, tr( "Keytracking" ), LedCheckBox::Green );
		ToolTip::add( filtKeytrackingToggle[i], tr( "When this is enabled, the delay of the filter's feedback changes to match the frequency of the notes you play." ) );

		filtMutedToggle[i] = new LedCheckBox( "", this, tr( "Muted" ), LedCheckBox::Green );
		ToolTip::add( filtMutedToggle[i], tr( "This button mutes the filter.  An enabled but muted filter will still use CPU and still work as a matrix input, but will not be sent to the audio output.  You'll want to use this almost every time you send something to the Matrix." ) );
	}

	for( int i = 0; i < 18; ++i )
	{
		makeknob( macroKnob[i], knobSmallColored, tr( "Macro" ) + " " + QString::number(i+1) + ":", tr( "Macro %1: " ).arg( i + 1 ) + tr( "This knob's value can be used in the Matrix to control many values at the same time, at different amounts.  This is immensely useful for crafting great presets." ) );
	}

	makeknob( subVolKnob, knobColored, tr( "Volume" ), tr( "This knob, as you probably expected, controls the volume." ) );

	makeknob( subPhaseKnob, knobSmallColored, tr( "Phase" ), tr( "This knob changes the phase (starting position) of the oscillator." ) );

	makeknob( subPhaseRandKnob, knobSmallColored, tr( "Phase Randomness" ), tr( "This knob chooses a random phase for every note.  The phase change will never be larger than this knob's value." ) );

	makeknob( subDetuneKnob, knobColored, tr( "Detune" ), tr( "This knob changes the pitch of the oscillator." ) );

	makeknob( subSampLenKnob, knobColored, tr( "Waveform Sample Length" ), tr( "This knob changes the waveform length, which you can see in the graph." ) );

	makeknob( subPanningKnob, knobColored, tr( "Panning" ), tr( "This knob lowers the volume in one ear by an amount depending on this knob's value." ) );

	makeknob( subTempoKnob, knobColored, tr( "Tempo Sync" ), tr( "When this knob is anything other than 0, the oscillator is tempo synced to the specified tempo.  This is meant for the creation of envelopes/LFOs/step sequencers in the Matrix tab, and you'll most likely want to enable the Muted LED when using this." ) );

	makeknob( subRateLimitKnob, knobColored, tr( "Rate Limit" ), tr( "This knob limits the rate at which the waveform can change.  Combined with the Noise LED being enabled, this could potentially be used as a chaos oscillator." ) );

	makeknob( subUnisonNumKnob, knobColored, tr( "Unison Voices" ), tr( "This knob clones this oscillator multiple times depending on its value, and makes slight changes to the clones depending on the other unison-related knobs." ) );

	makeknob( subUnisonDetuneKnob, knobColored, tr( "Unison Detune" ), tr( "This knob detunes every unison voice by a random number that is less than the specified amount." ) );

	subEnabledToggle = new LedCheckBox( "", this, tr( "Enabled" ), LedCheckBox::Green );
	ToolTip::add( subEnabledToggle, tr( "This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!" ) );

	subMutedToggle = new LedCheckBox( "", this, tr( "Muted" ), LedCheckBox::Green );
	ToolTip::add( subMutedToggle, tr( "This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output." ) );

	subKeytrackToggle = new LedCheckBox( "", this, tr( "Keytracking Enabled" ), LedCheckBox::Green );
	ToolTip::add( subKeytrackToggle, tr( "This button turns keytracking on/off.  Without keytracking, the frequency will be 440 Hz by default, and will ignore the frequency of the played note, but will still follow other methods of detuning the sound." ) );

	subNoiseToggle = new LedCheckBox( "", this, tr( "Noise Enabled" ), LedCheckBox::Green );
	ToolTip::add( subNoiseToggle, tr( "This button converts this oscillator into a noise generator.  A random part of the graph is chosen, that value is added to the previous output in the same direction it was going, and when the waveform crosses the top or bottom, the direction changes." ) );

	subInterpolateToggle = new LedCheckBox( "", this, tr( "Interpolation Enabled" ), LedCheckBox::Green );
	ToolTip::add( subInterpolateToggle, tr( "This button turns sinc interpolation on/off.  Interpolation uses no CPU and makes the oscillator super duper high-quality.  You'll probably only ever want to turn this off when you're using small waveform lengths and you don't want the waveform smoothed over." ) );

	for( int i = 0; i < 8; ++i )
	{
		makeknob( modInAmntKnob[i], knobSmallColored, tr( "Matrix Input Amount" ), tr( "This knob controls how much of the input to send to the output." ) );

		makeknob( modInCurveKnob[i], knobSmallColored, tr( "Matrix Curve Amount" ), tr( "This knob gives the input value a bias toward the top or bottom" ) );

		makeknob( modInAmntKnob2[i], knobSmallColored, tr( "Secondary Matrix Input Amount" ), tr( "This knob controls how much of the input to send to the output." ) );

		makeknob( modInCurveKnob2[i], knobSmallColored, tr( "Secondary Matrix Curve Amount" ), tr( "This knob gives the input value a bias toward the top or bottom" ) );

		modOutSecBox[i] = new ComboBox( this );
		modOutSecBox[i]->setGeometry( 2000, 5, 42, 22 );
		modOutSecBox[i]->setFont( pointSize<8>( modOutSecBox[i]->font() ) );
		ToolTip::add( modOutSecBox[i], tr( "This dropdown box chooses an output for the Matrix Box." ) );

		modOutSigBox[i] = new ComboBox( this );
		modOutSigBox[i]->setGeometry( 2000, 5, 42, 22 );
		modOutSigBox[i]->setFont( pointSize<8>( modOutSigBox[i]->font() ) );
		ToolTip::add( modOutSigBox[i], tr( "This dropdown box chooses which part of the input to send to the Matrix Box (e.g. which parameter to control, which filter, etc.)." ) );

		modOutSecNumBox[i] = new LcdSpinBox( 2, "microwave", this, "Mod Output Number" );
		ToolTip::add( modOutSecNumBox[i], tr( "This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator)." ) );

		modInBox[i] = new ComboBox( this );
		modInBox[i]->setGeometry( 2000, 5, 42, 22 );
		modInBox[i]->setFont( pointSize<8>( modInBox[i]->font() ) );
		ToolTip::add( modInBox[i], tr( "This dropdown box chooses an input for the Matrix Box." ) );

		modInNumBox[i] = new LcdSpinBox( 2, "microwave", this, "Mod Input Number" );
		ToolTip::add( modInNumBox[i], tr( "This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator, which filter, etc.)." ) );

		modInBox2[i] = new ComboBox( this );
		modInBox2[i]->setGeometry( 2000, 5, 42, 22 );
		modInBox2[i]->setFont( pointSize<8>( modInBox2[i]->font() ) );
		ToolTip::add( modInBox2[i], tr( "This dropdown box chooses an input for the Matrix Box." ) );

		modInNumBox2[i] = new LcdSpinBox( 2, "microwave", this, "Secondary Mod Input Number" );
		ToolTip::add( modInNumBox2[i], tr( "This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator, which filter, etc.)." ) );

		modEnabledToggle[i] = new LedCheckBox( "", this, tr( "Modulation Enabled" ), LedCheckBox::Green );
		ToolTip::add( modEnabledToggle[i], tr( "This button enables the Matrix Box.  While disabled, this does not use CPU." ) );

		modCombineTypeBox[i] = new ComboBox( this );
		modCombineTypeBox[i]->setGeometry( 2000, 5, 42, 22 );
		modCombineTypeBox[i]->setFont( pointSize<8>( modCombineTypeBox[i]->font() ) );
		ToolTip::add( modCombineTypeBox[i], tr( "This dropdown box chooses how to combine the two Matrix inputs." ) );

		modTypeToggle[i] = new LedCheckBox( "", this, tr( "Envelope Enabled" ), LedCheckBox::Green );
		ToolTip::add( modTypeToggle[i], tr( "This button, when enabled, treats the input as an envelope rather than an LFO." ) );

		modType2Toggle[i] = new LedCheckBox( "", this, tr( "Envelope Enabled" ), LedCheckBox::Green );
		ToolTip::add( modType2Toggle[i], tr( "This button, when enabled, treats the input as an envelope rather than an LFO." ) );

		modUpArrow[i] = new PixmapButton( this, tr( "Move Matrix Section Up" ) );
		modUpArrow[i]->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowup" ) );
		modUpArrow[i]->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowup" ) );
		ToolTip::add( modUpArrow[i], tr( "Move this Matrix Box up" ) );

		modDownArrow[i] = new PixmapButton( this, tr( "Move Matrix Section Down" ) );
		modDownArrow[i]->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowdown" ) );
		modDownArrow[i]->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowdown" ) );
		ToolTip::add( modDownArrow[i], tr( "Move this Matrix Box down" ) );

		i1Button[i] = new PixmapButton( this, tr( "Go to this input's location" ) );
		i1Button[i]->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "i1_button" ) );
		i1Button[i]->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "i1_button" ) );
		ToolTip::add( i1Button[i], tr( "Go to this input's location" ) );

		i2Button[i] = new PixmapButton( this, tr( "Go to this input's location" ) );
		i2Button[i]->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "i2_button" ) );
		i2Button[i]->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "i2_button" ) );
		ToolTip::add( i2Button[i], tr( "Go to this input's location" ) );

		modNumText[i] = new QLineEdit(this);
		modNumText[i]->resize(23, 19);
	}

	makeknob( visvolKnob, knobSmallColored, tr( "Visualizer Volume" ), tr( "This knob works as a vertical zoom knob for the visualizer." ) );

	makeknob( loadChnlKnob, knobColored, tr( "Wavetable Loading Channel" ), tr( "This knob chooses whether to load the left or right audio of the selected sample/wavetable." ) );


	graph = new Graph( this, Graph::BarCenterGradStyle, 204, 134 );
	graph->setAutoFillBackground( true );
	graph->setGraphColor( QColor( 121, 222, 239 ) );

	ToolTip::add( graph, tr ( "Draw here by dragging your mouse on this graph." ) );

	pal = QPalette();
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraph") );
	graph->setPalette( pal );

	QPixmap filtForegroundImg = PLUGIN_NAME::getIconPixmap("filterForeground");
	filtForegroundLabel = new QLabel(this);
	filtForegroundLabel->setPixmap(filtForegroundImg);
	filtForegroundLabel->setAttribute( Qt::WA_TransparentForMouseEvents );

	QPixmap matrixForegroundImg = PLUGIN_NAME::getIconPixmap("matrixForeground");
	matrixForegroundLabel = new QLabel(this);
	matrixForegroundLabel->setPixmap(matrixForegroundImg);
	matrixForegroundLabel->setAttribute( Qt::WA_TransparentForMouseEvents );

	sinWaveBtn = new PixmapButton( this, tr( "Sine" ) );
	sinWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sinwave_active" ) );
	sinWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sinwave" ) );
	ToolTip::add( sinWaveBtn, tr( "Sine wave" ) );

	triangleWaveBtn = new PixmapButton( this, tr( "Nachos" ) );
	triangleWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "triwave_active" ) );
	triangleWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "triwave" ) );
	ToolTip::add( triangleWaveBtn, tr( "Nacho wave" ) );

	sawWaveBtn = new PixmapButton( this, tr( "Sawsa" ) );
	sawWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sawwave_active" ) );
	sawWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sawwave" ) );
	ToolTip::add( sawWaveBtn, tr( "Sawsa wave" ) );

	sqrWaveBtn = new PixmapButton( this, tr( "Sosig" ) );
	sqrWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sqrwave_active" ) );
	sqrWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sqrwave" ) );
	ToolTip::add( sqrWaveBtn, tr( "Sosig wave" ) );

	whiteNoiseWaveBtn = new PixmapButton( this, tr( "Metal Fork" ) );
	whiteNoiseWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "noisewave_active" ) );
	whiteNoiseWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	ToolTip::add( whiteNoiseWaveBtn, tr( "Metal Fork" ) );

	usrWaveBtn = new PixmapButton( this, tr( "Takeout" ) );
	usrWaveBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	usrWaveBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	ToolTip::add( usrWaveBtn, tr( "Takeout Menu" ) );

	smoothBtn = new PixmapButton( this, tr( "Microwave Cover" ) );
	smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "smoothwave_active" ) );
	smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "smoothwave" ) );
	ToolTip::add( smoothBtn, tr( "Microwave Cover" ) );


	sinWave2Btn = new PixmapButton( this, tr( "Sine" ) );
	sinWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sinwave_active" ) );
	sinWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sinwave" ) );
	ToolTip::add( sinWave2Btn, tr( "Sine wave" ) );

	triangleWave2Btn = new PixmapButton( this, tr( "Nachos" ) );
	triangleWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "triwave_active" ) );
	triangleWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "triwave" ) );
	ToolTip::add( triangleWave2Btn,
			tr( "Nacho wave" ) );

	sawWave2Btn = new PixmapButton( this, tr( "Sawsa" ) );
	sawWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sawwave_active" ) );
	sawWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sawwave" ) );
	ToolTip::add( sawWave2Btn,
			tr( "Sawsa wave" ) );

	sqrWave2Btn = new PixmapButton( this, tr( "Sosig" ) );
	sqrWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sqrwave_active" ) );
	sqrWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sqrwave" ) );
	ToolTip::add( sqrWave2Btn, tr( "Sosig wave" ) );

	whiteNoiseWave2Btn = new PixmapButton( this, tr( "Metal Fork" ) );
	whiteNoiseWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "noisewave_active" ) );
	whiteNoiseWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "noisewave" ) );
	ToolTip::add( whiteNoiseWave2Btn, tr( "Metal Fork" ) );

	usrWave2Btn = new PixmapButton( this, tr( "Takeout Menu" ) );
	usrWave2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	usrWave2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	ToolTip::add( usrWave2Btn, tr( "Takeout Menu" ) );

	smooth2Btn = new PixmapButton( this, tr( "Microwave Cover" ) );
	smooth2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "smoothwave_active" ) );
	smooth2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "smoothwave" ) );
	ToolTip::add( smooth2Btn, tr( "Microwave Cover" ) );


	tab1Btn = new PixmapButton( this, tr( "Wavetable Tab" ) );
	tab1Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1_active" ) );
	tab1Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1_active" ) );
	ToolTip::add( tab1Btn, tr( "Wavetable Tab" ) );

	tab2Btn = new PixmapButton( this, tr( "Sub Tab" ) );
	tab2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2" ) );
	tab2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2" ) );
	ToolTip::add( tab2Btn, tr( "Sub Tab" ) );

	tab3Btn = new PixmapButton( this, tr( "Sample Tab" ) );
	tab3Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3" ) );
	tab3Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3" ) );
	ToolTip::add( tab3Btn, tr( "Sample Tab" ) );

	tab4Btn = new PixmapButton( this, tr( "Matrix Tab" ) );
	tab4Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4" ) );
	tab4Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4" ) );
	ToolTip::add( tab4Btn, tr( "Matrix Tab" ) );

	tab5Btn = new PixmapButton( this, tr( "Effect Tab" ) );
	tab5Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5" ) );
	tab5Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5" ) );
	ToolTip::add( tab5Btn, tr( "Effect Tab" ) );

	tab6Btn = new PixmapButton( this, tr( "Miscellaneous Tab" ) );
	tab6Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6" ) );
	tab6Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6" ) );
	ToolTip::add( tab6Btn, tr( "Miscellaneous Tab" ) );


	mainFlipBtn = new PixmapButton( this, tr( "Flip to other knobs" ) );
	mainFlipBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowup" ) );
	mainFlipBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowdown" ) );
	ToolTip::add( mainFlipBtn, tr( "Flip to other knobs" ) );
	mainFlipBtn->setCheckable(true);

	subFlipBtn = new PixmapButton( this, tr( "Flip to other knobs" ) );
	subFlipBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowup" ) );
	subFlipBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arrowdown" ) );
	ToolTip::add( subFlipBtn, tr( "Flip to other knobs" ) );
	subFlipBtn->setCheckable(true);


	manualBtn = new PixmapButton( this, tr( "Manual" ) );
	manualBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "manual_active" ) );
	manualBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "manual_inactive" ) );
	ToolTip::add( manualBtn, tr( "Manual" ) );


	removeDCBtn = new PixmapButton( this, tr( "Remove DC Offset" ) );
	removeDCBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "remove_dc_offset_button_enabled" ) );
	removeDCBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "remove_dc_offset_button_disabled" ) );
	ToolTip::add( removeDCBtn, tr( "Remove DC Offset" ) );
	removeDCBtn->setCheckable(true);

	oversampleModeBox = new ComboBox( this );
	oversampleModeBox->setGeometry( 0, 0, 42, 22 );
	oversampleModeBox->setFont( pointSize<8>( oversampleModeBox->font() ) );


	normalizeBtn = new PixmapButton( this, tr( "Normalize Wavetable" ) );
	normalizeBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "normalize_button_active" ) );
	normalizeBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "normalize_button" ) );
	ToolTip::add( normalizeBtn, tr( "Normalize Wavetable" ) );

	desawBtn = new PixmapButton( this, tr( "De-saw Wavetable" ) );
	desawBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "desaw_button_active" ) );
	desawBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "desaw_button" ) );
	ToolTip::add( desawBtn, tr( "De-saw Wavetable" ) );


	XBtn = new PixmapButton( this, tr( "Leave wavetable loading section" ) );
	XBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "xbtn" ) );
	XBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "xbtn" ) );
	ToolTip::add( XBtn, tr( "Leave wavetable loading tab" ) );

	MatrixXBtn = new PixmapButton( this, tr( "Leave Matrix tab" ) );
	MatrixXBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "xbtn" ) );
	MatrixXBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "xbtn" ) );
	ToolTip::add( MatrixXBtn, tr( "Leave wavetable loading tab" ) );


	visualizeToggle = new LedCheckBox( "", this, tr( "Visualize" ), LedCheckBox::Green );

	openWavetableButton = new PixmapButton( this );
	openWavetableButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	openWavetableButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	openWavetableButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	connect( openWavetableButton, SIGNAL( clicked() ), this, SLOT( openWavetableFileBtnClicked() ) );
	ToolTip::add( openWavetableButton, tr( "Open wavetable" ) );

	confirmLoadButton = new PixmapButton( this );
	confirmLoadButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	confirmLoadButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "confirm_button_active" ) );
	confirmLoadButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "confirm_button_inactive" ) );
	connect( confirmLoadButton, SIGNAL( clicked() ), this, SLOT( confirmWavetableLoadClicked() ) );
	ToolTip::add( confirmLoadButton, tr( "Load Wavetable" ) );

	subNumBox = new LcdSpinBox( 2, "microwave", this, "Sub Oscillator Number" );

	sampNumBox = new LcdSpinBox( 2, "microwave", this, "Sample Number" );

	mainNumBox = new LcdSpinBox( 2, "microwave", this, "Oscillator Number" );

	oversampleBox = new ComboBox( this );
	oversampleBox->setGeometry( 0, 0, 42, 22 );
	oversampleBox->setFont( pointSize<8>( oversampleBox->font() ) );

	loadModeBox = new ComboBox( this );
	loadModeBox->setGeometry( 0, 0, 202, 22 );
	loadModeBox->setFont( pointSize<8>( loadModeBox->font() ) );

	openSampleButton = new PixmapButton( this );
	openSampleButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	openSampleButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );
	openSampleButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileload" ) );

	effectScrollBar = new QScrollBar( Qt::Vertical, this );
	effectScrollBar->setSingleStep( 1 );
	effectScrollBar->setPageStep( 100 );
	effectScrollBar->setFixedHeight( 197 );
	effectScrollBar->setRange( 0, 590 );
	connect( effectScrollBar, SIGNAL( valueChanged( int ) ), this, SLOT( updateScroll( ) ) );

	effectScrollBar->setStyleSheet("QScrollBar::handle:horizontal { background: #3f4750; border: none; border-radius: 1px; min-width: 24px; }\
					QScrollBar::handle:horizontal:hover { background: #a6adb1; }\
					QScrollBar::handle:horizontal:pressed { background: #a6adb1; }\
					QScrollBar::handle:vertical { background: #3f4750; border: none; border-radius: 1px; min-height: 24px; }\
					QScrollBar::handle:vertical:hover { background: #a6adb1; }\
					QScrollBar::handle:vertical:pressed { background: #a6adb1; }\
					QScrollBar::handle:horizontal:disabled, QScrollBar::handle:vertical:disabled  { background: #262b30; border-radius: 1px; border: none; }");

	matrixScrollBar = new QScrollBar( Qt::Vertical, this );
	matrixScrollBar->setSingleStep( 1 );
	matrixScrollBar->setPageStep( 100 );
	matrixScrollBar->setFixedHeight( 197 );
	matrixScrollBar->setRange( 0, 6232 );
	connect( matrixScrollBar, SIGNAL( valueChanged( int ) ), this, SLOT( updateScroll( ) ) );

	matrixScrollBar->setStyleSheet("QScrollBar::handle:horizontal { background: #3f4750; border: none; border-radius: 1px; min-width: 24px; }\
					QScrollBar::handle:horizontal:hover { background: #a6adb1; }\
					QScrollBar::handle:horizontal:pressed { background: #a6adb1; }\
					QScrollBar::handle:vertical { background: #3f4750; border: none; border-radius: 1px; min-height: 24px; }\
					QScrollBar::handle:vertical:hover { background: #a6adb1; }\
					QScrollBar::handle:vertical:pressed { background: #a6adb1; }\
					QScrollBar::handle:horizontal:disabled, QScrollBar::handle:vertical:disabled  { background: #262b30; border-radius: 1px; border: none; }");

	// The above scrollbar style changes don't seem to work entirely for some reason.


	connect( openSampleButton, SIGNAL( clicked() ), this, SLOT( openSampleFileBtnClicked() ) );
	ToolTip::add( openSampleButton, tr( "Open sample" ) );



	connect( sinWaveBtn, SIGNAL (clicked () ), this, SLOT ( sinWaveClicked() ) );
	connect( triangleWaveBtn, SIGNAL ( clicked () ), this, SLOT ( triangleWaveClicked() ) );
	connect( sawWaveBtn, SIGNAL (clicked () ), this, SLOT ( sawWaveClicked() ) );
	connect( sqrWaveBtn, SIGNAL ( clicked () ), this, SLOT ( sqrWaveClicked() ) );
	connect( whiteNoiseWaveBtn, SIGNAL ( clicked () ), this, SLOT ( noiseWaveClicked() ) );
	connect( usrWaveBtn, SIGNAL ( clicked () ), this, SLOT ( usrWaveClicked() ) );
	connect( smoothBtn, SIGNAL ( clicked () ), this, SLOT ( smoothClicked() ) );

	connect( sinWave2Btn, SIGNAL (clicked () ), this, SLOT ( sinWaveClicked() ) );
	connect( triangleWave2Btn, SIGNAL ( clicked () ), this, SLOT ( triangleWaveClicked() ) );
	connect( sawWave2Btn, SIGNAL (clicked () ), this, SLOT ( sawWaveClicked() ) );
	connect( sqrWave2Btn, SIGNAL ( clicked () ), this, SLOT ( sqrWaveClicked() ) );
	connect( whiteNoiseWave2Btn, SIGNAL ( clicked () ), this, SLOT ( noiseWaveClicked() ) );
	connect( usrWave2Btn, SIGNAL ( clicked () ), this, SLOT ( usrWaveClicked() ) );
	connect( smooth2Btn, SIGNAL ( clicked () ), this, SLOT ( smoothClicked() ) );


	connect( XBtn, SIGNAL (clicked () ), this, SLOT ( XBtnClicked() ) );
	connect( MatrixXBtn, SIGNAL (clicked () ), this, SLOT ( MatrixXBtnClicked() ) );

	connect( normalizeBtn, SIGNAL (clicked () ), this, SLOT ( normalizeClicked() ) );
	connect( desawBtn, SIGNAL (clicked () ), this, SLOT ( desawClicked() ) );


	// This is a mess, but for some reason just entering a number without a variable didn't work...
	int ii = 0;
	connect( tab1Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 1;
	connect( tab2Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 2;
	connect( tab3Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 3;
	connect( tab4Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 4;
	connect( tab5Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );
	ii = 5;
	connect( tab6Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); } );


	connect( mainFlipBtn, SIGNAL (clicked () ), this, SLOT ( flipperClicked() ) );
	connect( subFlipBtn, SIGNAL (clicked () ), this, SLOT ( flipperClicked() ) );

	connect( visualizeToggle, SIGNAL( toggled( bool ) ), this, SLOT ( visualizeToggled( bool ) ) );

	connect( &b->mainNum, SIGNAL( dataChanged( ) ), this, SLOT( mainNumChanged( ) ) );
	connect( &b->subNum, SIGNAL( dataChanged( ) ), this, SLOT( subNumChanged( ) ) );
	connect( &b->sampNum, SIGNAL( dataChanged( ) ), this, SLOT( sampNumChanged( ) ) );

	connect( manualBtn, SIGNAL (clicked ( bool ) ), this, SLOT ( manualBtnClicked() ) );

	for( int i = 0; i < 64; ++i )
	{
		connect( b->modOutSec[i], &ComboBoxModel::dataChanged, this, [this, i]() { modOutSecChanged(i); }, Qt::DirectConnection );
		connect( b->modIn[i], &ComboBoxModel::dataChanged, this, [this, i]() { modInChanged(i); }, Qt::DirectConnection );
		connect( b->modIn2[i], &ComboBoxModel::dataChanged, this, [this, i]() { modIn2Changed(i); }, Qt::DirectConnection );

		connect( b->modEnabled[i], &BoolModel::dataChanged, this, [this, i]() { modEnabledChanged(); } );
	}

	for( int i = 0; i < 8; ++i )
	{
		connect( modUpArrow[i], &PixmapButton::clicked, this, [this, i]() { modUpClicked(i); } );
		connect( modDownArrow[i], &PixmapButton::clicked, this, [this, i]() { modDownClicked(i); } );

		connect( i1Button[i], &PixmapButton::clicked, this, [this, i]() { i1Clicked(i); } );
		connect( i2Button[i], &PixmapButton::clicked, this, [this, i]() { i2Clicked(i); } );

		//Make changing Enabled LED update graph color
		connect( b->enabled[i], SIGNAL( dataChanged( ) ), this, SLOT( mainNumChanged( ) ) );
		connect( b->subEnabled[i], SIGNAL( dataChanged( ) ), this, SLOT( subNumChanged( ) ) );
		connect( b->sampleEnabled[i], SIGNAL( dataChanged( ) ), this, SLOT( sampNumChanged( ) ) );
	}

	for( int i = 0; i < 18; ++i )
	{
		if( !b->macroTooltips[i].isEmpty() )
		{
			ToolTip::add( macroKnob[i], tr( "Macro %1: " ).arg( i + 1 ) + b->macroTooltips[i] );
		}

		macroKnob[i]->refreshMacroColor();
	}

	b->viewOpen = true;

	updateScroll();
	updateBackground();

	modEnabledChanged();// Updates scroll bar length in Matrix
}


MicrowaveView::~MicrowaveView()
{
	// This got mad when using the vairable "b".
	if( castModel<Microwave>() ) { castModel<Microwave>()->viewOpen = false; }
}


// Connects knobs/GUI elements to their models
void MicrowaveView::modelChanged()
{
	graph->setModel( &b->graph );
	visvolKnob->setModel( &b->visvol );
	visualizeToggle->setModel( &b->visualize );
	subNumBox->setModel( &b->subNum );
	sampNumBox->setModel( &b->sampNum );
	loadChnlKnob->setModel( &b->loadChnl );
	mainNumBox->setModel( &b->mainNum );
	oversampleBox->setModel( &b->oversample );
	loadModeBox->setModel( &b->loadMode );
	mainFlipBtn->setModel( &b->mainFlipped );
	subFlipBtn->setModel( &b->subFlipped );
	removeDCBtn->setModel( &b->removeDC );
	oversampleModeBox->setModel( &b->oversampleMode );
}


// If you think you've seen ugly workarounds before, you haven't seen anything yet.
// A very old version of Microwave included the GUI elements visually moving left/right.
// Because of that, the GUI elements were just moved off of the screen rather than having their visibility toggled.
// It is because of this that I traded out the move function with visimove, which prevents the GUI elements from
// leaving the 250x250 GUI, and instead toggles their visibility if they try to leave, in case of things checking for the
// bounds of the GUI elements (e.g. instrument window resizing).
void MicrowaveView::updateScroll()
{
	int scrollVal = ( b->scroll ) * 250.f;
	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int effectScrollVal = ( effectScrollBar->value() ) / 100.f * 92.f;
	int mainFlipped = b->mainFlipped.value();
	int subFlipped = b->subFlipped.value();

	int mainIsFlipped = mainFlipped * 500.f;
	int mainIsNotFlipped = !mainFlipped * 500.f;
	int subIsFlipped = subFlipped * 500.f;
	int subIsNotFlipped = !subFlipped * 500.f;

	visimove( morphKnob, ( scrollVal < 250 ? 23 : 1500 + 176 ) - scrollVal, 172 + mainIsFlipped );
	visimove( rangeKnob, ( scrollVal < 250 ? 55 : 1500 + 208 ) - scrollVal, 172 + mainIsFlipped );
	visimove( modifyKnob, 87 - scrollVal, 172 + mainIsFlipped );
	visimove( modifyModeBox, 127 - scrollVal, 186 + mainIsFlipped );
	visimove( volKnob, 23 - scrollVal, 172 + mainIsNotFlipped );
	visimove( panKnob, 55 - scrollVal, 172 + mainIsNotFlipped );
	visimove( detuneKnob, 152 - scrollVal, 216 + mainIsFlipped );
	visimove( phaseKnob, 184 - scrollVal, 203 + mainIsNotFlipped );
	visimove( phaseRandKnob, 209 - scrollVal, 203 + mainIsNotFlipped );
	visimove( enabledToggle, 85 - scrollVal, 229 );
	visimove( mutedToggle, 103 - scrollVal, 229 );
	visimove( sampLenKnob, 137 - scrollVal, 172 + mainIsNotFlipped );
	visimove( morphMaxKnob, 101 - scrollVal, 172 + mainIsNotFlipped );
	visimove( unisonVoicesKnob, 184 - scrollVal, 172 );
	visimove( unisonDetuneKnob, 209 - scrollVal, 172 );
	visimove( unisonMorphKnob, 184 - scrollVal, 203 + mainIsFlipped );
	visimove( unisonModifyKnob, 209 - scrollVal, 203 + mainIsFlipped );
	visimove( keytrackingToggle, 121 - scrollVal, 229 + mainIsFlipped );
	visimove( tempoKnob, 152 - scrollVal, 216 + mainIsNotFlipped );
	visimove( interpolateToggle, 121 - scrollVal, 229 + mainIsNotFlipped );

	visimove( sampleEnabledToggle, 85 + 500 - scrollVal, 229 );
	visimove( sampleMutedToggle, 103 + 500 - scrollVal, 229 );
	visimove( sampleKeytrackingToggle, 121 + 500 - scrollVal, 229 );
	visimove( sampleGraphEnabledToggle, 138 + 500 - scrollVal, 229 );
	visimove( sampleLoopToggle, 155 + 500 - scrollVal, 229 );
	visimove( sampleVolumeKnob, 23 + 500 - scrollVal, 172 );
	visimove( samplePanningKnob, 55 + 500 - scrollVal, 172 );
	visimove( sampleDetuneKnob, 93 + 500 - scrollVal, 172 );
	visimove( samplePhaseKnob, 180 + 500 - scrollVal, 172 );
	visimove( samplePhaseRandKnob, 206 + 500 - scrollVal, 172 );
	visimove( sampleStartKnob, 121 + 500 - scrollVal, 172 );
	visimove( sampleEndKnob, 145 + 500 - scrollVal, 172 );

	for( int i = 0; i < 8; ++i )
	{
		visimove( filtCutoffKnob[i], 32 + 1000 - scrollVal, i*92+55 - effectScrollVal );
		visimove( filtResoKnob[i], 63 + 1000 - scrollVal, i*92+55 - effectScrollVal );
		visimove( filtGainKnob[i], 94 + 1000 - scrollVal, i*92+55 - effectScrollVal );

		visimove( filtTypeBox[i], 128 + 1000 - scrollVal, i*92+63 - effectScrollVal );
		visimove( filtSlopeBox[i], 171 + 1000 - scrollVal, i*92+63 - effectScrollVal );
		visimove( filtInVolKnob[i], 30 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		visimove( filtOutVolKnob[i], 55 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		visimove( filtWetDryKnob[i], 80 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		visimove( filtBalKnob[i], 105 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		visimove( filtSatuKnob[i], 135 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		visimove( filtFeedbackKnob[i], 167 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		visimove( filtDetuneKnob[i], 192 + 1000 - scrollVal, i*92+91 - effectScrollVal );
		visimove( filtEnabledToggle[i], 27 + 1000 - scrollVal, i*92+36 - effectScrollVal );
		visimove( filtMutedToggle[i], 166 + 1000 - scrollVal, i*92+36 - effectScrollVal );
		visimove( filtKeytrackingToggle[i], 200 + 1000 - scrollVal, i*92+36 - effectScrollVal );
	}

	visimove( subVolKnob, 23 + 250 - scrollVal, 172 + subIsFlipped );
	visimove( subPanningKnob, 55 + 250 - scrollVal, 172 + subIsFlipped );
	visimove( subDetuneKnob, 95 + 250 - scrollVal, 172 + subIsFlipped );
	visimove( subPhaseKnob, 180 + 250 - scrollVal, 172 );
	visimove( subPhaseRandKnob, 206 + 250 - scrollVal, 172 );
	visimove( subSampLenKnob, 130 + 250 - scrollVal, 172 + subIsFlipped );
	visimove( subTempoKnob, 23 + 250 - scrollVal, 172 + subIsNotFlipped );
	visimove( subRateLimitKnob, 55 + 250 - scrollVal, 172 + subIsNotFlipped );
	visimove( subUnisonNumKnob, 95 + 250 - scrollVal, 172 + subIsNotFlipped );
	visimove( subUnisonDetuneKnob, 130 + 250 - scrollVal, 172 + subIsNotFlipped );

	if( subIsNotFlipped )
	{
		visimove( subEnabledToggle, 85 + 250 - scrollVal, 229 );
		visimove( subMutedToggle, 103 + 250 - scrollVal, 229 );
		visimove( subKeytrackToggle, 121 + 250 - scrollVal, 229 );
		visimove( subNoiseToggle, 138 + 250 - scrollVal, 229 );
		visimove( subInterpolateToggle, 155 + 250 - scrollVal, 229 );
	}
	else
	{
		visimove( subEnabledToggle, 85 + 250 - scrollVal, 235 );
		visimove( subMutedToggle, 103 + 250 - scrollVal, 235 );
		visimove( subKeytrackToggle, 121 + 250 - scrollVal, 235 );
		visimove( subNoiseToggle, 138 + 250 - scrollVal, 235 );
		visimove( subInterpolateToggle, 155 + 250 - scrollVal, 235 );
	}

	int matrixRemainder = modScrollVal % 460;
	int matrixDivide = modScrollVal / 460 * 4;
	for( int i = 0; i < 8; ++i )
	{
		if( i+matrixDivide < 64 )
		{
			modOutSecBox[i]->setModel( b->modOutSec[i+matrixDivide] );
			modOutSigBox[i]->setModel( b->modOutSig[i+matrixDivide] );
			modOutSecNumBox[i]->setModel( b->modOutSecNum[i+matrixDivide] );

			modInBox[i]->setModel( b->modIn[i+matrixDivide] );
			modInNumBox[i]->setModel( b->modInNum[i+matrixDivide] );
			modInAmntKnob[i]->setModel( b->modInAmnt[i+matrixDivide] );
			modInCurveKnob[i]->setModel( b->modInCurve[i+matrixDivide] );

			modInBox2[i]->setModel( b->modIn2[i+matrixDivide] );
			modInNumBox2[i]->setModel( b->modInNum2[i+matrixDivide] );
			modInAmntKnob2[i]->setModel( b->modInAmnt2[i+matrixDivide] );
			modInCurveKnob2[i]->setModel( b->modInCurve2[i+matrixDivide] );

			modEnabledToggle[i]->setModel( b->modEnabled[i+matrixDivide] );

			modCombineTypeBox[i]->setModel( b->modCombineType[i+matrixDivide] );

			modTypeToggle[i]->setModel( b->modType[i+matrixDivide] );
			modType2Toggle[i]->setModel( b->modType2[i+matrixDivide] );

			modNumText[i]->setText( QString::number(i+matrixDivide+1) );

			modInAmntKnob[i]->setMatrixLocation( 4, 1, i );
			modInCurveKnob[i]->setMatrixLocation( 4, 2, i );
			modInAmntKnob2[i]->setMatrixLocation( 4, 3, i );
			modInCurveKnob2[i]->setMatrixLocation( 4, 4, i );
		}

		// Bug evasion.  Without this, some display glitches happen in certain conditions.
		modOutSecChanged( i+matrixDivide );
		modInChanged( i+matrixDivide );
		modIn2Changed( i+matrixDivide );

		visimove( modInBox[i], 45 + 750 - scrollVal, i*115+57 - matrixRemainder );
		visimove( modInNumBox[i], 90 + 750 - scrollVal, i*115+57 - matrixRemainder );
		visimove( modInAmntKnob[i], 136 + 750 - scrollVal, i*115+53 - matrixRemainder );
		visimove( modInCurveKnob[i], 161 + 750 - scrollVal, i*115+53 - matrixRemainder );
		visimove( modInBox2[i], 45 + 750 - scrollVal, i*115+118 - matrixRemainder );
		visimove( modInNumBox2[i], 90 + 750 - scrollVal, i*115+118 - matrixRemainder );
		visimove( modInAmntKnob2[i], 136 + 750 - scrollVal, i*115+114 - matrixRemainder );
		visimove( modInCurveKnob2[i], 161 + 750 - scrollVal, i*115+114 - matrixRemainder );
		visimove( modOutSecBox[i], 27 + 750 - scrollVal, i*115+88 - matrixRemainder );
		visimove( modOutSigBox[i], 69 + 750 - scrollVal, i*115+88 - matrixRemainder );
		visimove( modOutSecNumBox[i], 112 + 750 - scrollVal, i*115+88 - matrixRemainder );
		visimove( modEnabledToggle[i], 27 + 750 - scrollVal, i*115+36 - matrixRemainder );
		visimove( modCombineTypeBox[i], 149 + 750 - scrollVal, i*115+88 - matrixRemainder );
		visimove( modTypeToggle[i], 195 + 750 - scrollVal, i*115+67 - matrixRemainder );
		visimove( modType2Toggle[i], 195 + 750 - scrollVal, i*115+128 - matrixRemainder );
		visimove( modUpArrow[i], 181 + 750 - scrollVal, i*115+37 - matrixRemainder );
		visimove( modDownArrow[i], 199 + 750 - scrollVal, i*115+37 - matrixRemainder );
		visimove( i1Button[i], 25 + 750 - scrollVal, i*115+50 - matrixRemainder );
		visimove( i2Button[i], 25 + 750 - scrollVal, i*115+112 - matrixRemainder );
		visimove( modNumText[i], 192 + 750 - scrollVal, i*115+89 - matrixRemainder );
	}

	for( int i = 0; i < 8; ++i )
	{
		filtCutoffKnob[i]->setModel( b->filtCutoff[i] );
		filtCutoffKnob[i]->setMatrixLocation( 6, 1, i );

		filtResoKnob[i]->setModel( b->filtReso[i] );
		filtResoKnob[i]->setMatrixLocation( 6, 2, i );

		filtGainKnob[i]->setModel( b->filtGain[i] );
		filtGainKnob[i]->setMatrixLocation( 6, 3, i );

		filtTypeBox[i]->setModel( b->filtType[i] );

		filtSlopeBox[i]->setModel( b->filtSlope[i] );

		filtInVolKnob[i]->setModel( b->filtInVol[i] );
		filtInVolKnob[i]->setMatrixLocation( 6, 5, i );

		filtOutVolKnob[i]->setModel( b->filtOutVol[i] );
		filtOutVolKnob[i]->setMatrixLocation( 6, 6, i );

		filtWetDryKnob[i]->setModel( b->filtWetDry[i] );
		filtWetDryKnob[i]->setMatrixLocation( 6, 7, i );

		filtBalKnob[i]->setModel( b->filtBal[i] );
		filtBalKnob[i]->setMatrixLocation( 6, 8, i );

		filtSatuKnob[i]->setModel( b->filtSatu[i] );
		filtSatuKnob[i]->setMatrixLocation( 6, 9, i );

		filtFeedbackKnob[i]->setModel( b->filtFeedback[i] );
		filtFeedbackKnob[i]->setMatrixLocation( 6, 10, i );

		filtDetuneKnob[i]->setModel( b->filtDetune[i] );
		filtDetuneKnob[i]->setMatrixLocation( 6, 11, i );

		filtEnabledToggle[i]->setModel( b->filtEnabled[i] );

		filtMutedToggle[i]->setModel( b->filtMuted[i] );

		filtKeytrackingToggle[i]->setModel( b->filtKeytracking[i] );
	}

	for( int i = 0; i < 18; ++i )
	{
		macroKnob[i]->setModel( b->macro[i] );
		macroKnob[i]->setMatrixLocation( 7, i, 0 );
		macroKnob[i]->setWhichMacroKnob( i );
		refreshMacroColor( macroKnob[i], i );
	}

	visimove( visvolKnob, 230 - scrollVal, 24 );

	visimove( loadChnlKnob, 1500 + 111 - scrollVal, 121 );
	visimove( visualizeToggle, 213 - scrollVal, 26 );
	visimove( subNumBox, 250 + 18 - scrollVal, 219 );
	visimove( sampNumBox, 500 + 18 - scrollVal, 219 );
	visimove( mainNumBox, 18 - scrollVal, 219 );
	visimove( graph, scrollVal >= 500 ? 500 + 23 - scrollVal : 23 , 30 );
	visimove( openWavetableButton, ( scrollVal < 250 ? 54 : 1500 + 115 ) - scrollVal, scrollVal < 250 ? 220 : 24 );
	visimove( openSampleButton, 54 + 500 - scrollVal, 220 );

	visimove( sinWaveBtn, 179 + 250 - scrollVal, 212 );
	visimove( triangleWaveBtn, 197 + 250 - scrollVal, 212 );
	visimove( sawWaveBtn, 215 + 250 - scrollVal, 212 );
	visimove( sqrWaveBtn, 179 + 250 - scrollVal, 227 );
	visimove( whiteNoiseWaveBtn, 197 + 250 - scrollVal, 227 );
	visimove( smoothBtn, 215 + 250 - scrollVal, 227 );
	visimove( usrWaveBtn, 54 + 250 - scrollVal, 220 );

	visimove( sinWave2Btn, 179 + 500 - scrollVal, 212 );
	visimove( triangleWave2Btn, 197 + 500 - scrollVal, 212 );
	visimove( sawWave2Btn, 215 + 500 - scrollVal, 212 );
	visimove( sqrWave2Btn, 179 + 500 - scrollVal, 227 );
	visimove( whiteNoiseWave2Btn, 197 + 500 - scrollVal, 227 );
	visimove( smooth2Btn, 215 + 500 - scrollVal, 227 );
	visimove( usrWave2Btn, 54 + 500 - scrollVal, 220 );

	visimove( oversampleBox, 70 + 1250 - scrollVal, 50 );

	visimove( effectScrollBar, 221 + 1000 - scrollVal, 32 );
	visimove( matrixScrollBar, 221 + 750 - scrollVal, 32 );

	visimove( filtForegroundLabel, 1000 - scrollVal, 0 );
	visimove( filtBoxesLabel, 1000 + 24 - scrollVal, 35 - ( effectScrollVal % 92 ) );

	visimove( matrixForegroundLabel, 750 - scrollVal, 0 );
	visimove( matrixBoxesLabel, 750 + 24 - scrollVal, 35 - ( modScrollVal % 115 ) );

	visimove( macroKnob[0], 1250 + 59 - scrollVal, 127 );
	visimove( macroKnob[1], 1250 + 81 - scrollVal, 127 );
	visimove( macroKnob[2], 1250 + 103 - scrollVal, 127 );
	visimove( macroKnob[3], 1250 + 125 - scrollVal, 127 );
	visimove( macroKnob[4], 1250 + 147 - scrollVal, 127 );
	visimove( macroKnob[5], 1250 + 169 - scrollVal, 127 );
	visimove( macroKnob[6], 1250 + 59 - scrollVal, 147 );
	visimove( macroKnob[7], 1250 + 81 - scrollVal, 147 );
	visimove( macroKnob[8], 1250 + 103 - scrollVal, 147 );
	visimove( macroKnob[9], 1250 + 125 - scrollVal, 147 );
	visimove( macroKnob[10], 1250 + 147 - scrollVal, 147 );
	visimove( macroKnob[11], 1250 + 169 - scrollVal, 147 );
	visimove( macroKnob[12], 1250 + 59 - scrollVal, 167 );
	visimove( macroKnob[13], 1250 + 81 - scrollVal, 167 );
	visimove( macroKnob[14], 1250 + 103 - scrollVal, 167 );
	visimove( macroKnob[15], 1250 + 125 - scrollVal, 167 );
	visimove( macroKnob[16], 1250 + 147 - scrollVal, 168 );
	visimove( macroKnob[17], 1250 + 169 - scrollVal, 168 );

	visimove( tab1Btn, 1, 48 );
	visimove( tab2Btn, 1, 63 );
	visimove( tab3Btn, 1, 78 );
	visimove( tab4Btn, 1, 93 );
	visimove( tab5Btn, 1, 108 );
	visimove( tab6Btn, 1, 123 );

	visimove( mainFlipBtn, 3 - scrollVal, 145 );
	visimove( subFlipBtn, 250 + 3 - scrollVal, 145 );

	visimove( manualBtn, 1250 + 49 - scrollVal, 199);

	visimove( loadModeBox, 1500 + 25 - scrollVal, 76 );
	visimove( confirmLoadButton, 1500 + 93 - scrollVal, 187);

	visimove( XBtn, 231 + 1500 - scrollVal, 11 );
	visimove( MatrixXBtn, 229 + 750 - scrollVal, 8 );

	visimove( normalizeBtn, 155 + 1500 - scrollVal, 224 );
	visimove( desawBtn, 39 + 1500 - scrollVal, 224 );

	visimove( removeDCBtn, 1250 + 68 - scrollVal, 84 );
	visimove( oversampleModeBox, 1250 + 135 - scrollVal, 50 );

	tabChanged( b->scroll );
	visvolKnob->setVisible( b->visualize.value() );
}



void MicrowaveView::wheelEvent( QWheelEvent * _me )
{
	if( _me->x() <= 18 && _me->y() >= 48 && _me->y() <= 138 )// If scroll over tab buttons
	{
		if( b->scroll != 6 )
		{
			if( _me->delta() < 0 && b->scroll != 5 )
			{
				b->scroll += 1;
				updateScroll();
			}
			else if( _me->delta() > 0 && b->scroll > 0 )
			{
				b->scroll -= 1;
				updateScroll();
			}
		}
	}
}


void MicrowaveView::setGraphEnabledColor( bool isEnabled )
{
	graph->setGraphColor( isEnabled ? QColor( 121, 222, 239 ) : QColor( 197, 197, 197 ) );
	pal = QPalette();
	pal.setBrush( backgroundRole(), isEnabled ? PLUGIN_NAME::getIconPixmap("wavegraph") : PLUGIN_NAME::getIconPixmap("wavegraphdisabled") );
	graph->setPalette( pal );
}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::mainNumChanged()
{
	setGraphEnabledColor( b->enabled[b->mainNum.value()-1]->value() );

	int mainNumValue = b->mainNum.value() - 1;

	morphKnob->setModel( b->morph[mainNumValue] );
	morphKnob->setMatrixLocation( 1, 1, mainNumValue );

	rangeKnob->setModel( b->range[mainNumValue] );
	rangeKnob->setMatrixLocation( 1, 2, mainNumValue );

	sampLenKnob->setModel( b->sampLen[mainNumValue] );

	modifyKnob->setModel( b->modify[mainNumValue] );
	modifyKnob->setMatrixLocation( 1, 3, mainNumValue );

	morphMaxKnob->setModel( b->morphMax[mainNumValue] );

	unisonVoicesKnob->setModel( b->unisonVoices[mainNumValue] );
	unisonVoicesKnob->setMatrixLocation( 1, 8, mainNumValue );

	unisonDetuneKnob->setModel( b->unisonDetune[mainNumValue] );
	unisonDetuneKnob->setMatrixLocation( 1, 9, mainNumValue );

	unisonMorphKnob->setModel( b->unisonMorph[mainNumValue] );
	unisonMorphKnob->setMatrixLocation( 1, 10, mainNumValue );

	unisonModifyKnob->setModel( b->unisonModify[mainNumValue] );
	unisonModifyKnob->setMatrixLocation( 1, 11, mainNumValue );

	detuneKnob->setModel( b->detune[mainNumValue] );
	detuneKnob->setMatrixLocation( 1, 4, mainNumValue );

	modifyModeBox-> setModel( b-> modifyMode[mainNumValue] );

	phaseKnob->setModel( b->phase[mainNumValue] );
	phaseKnob->setMatrixLocation( 1, 5, mainNumValue );

	phaseRandKnob->setModel( b->phaseRand[mainNumValue] );

	volKnob->setModel( b->vol[mainNumValue] );
	volKnob->setMatrixLocation( 1, 6, mainNumValue );

	panKnob->setModel( b->pan[mainNumValue] );
	panKnob->setMatrixLocation( 1, 7, mainNumValue );

	enabledToggle->setModel( b->enabled[mainNumValue] );

	mutedToggle->setModel( b->muted[mainNumValue] );

	keytrackingToggle->setModel( b->keytracking[mainNumValue] );

	tempoKnob->setModel( b->tempo[mainNumValue] );

	interpolateToggle->setModel( b->interpolate[mainNumValue] );

}


// Trades out the GUI elements when switching between oscillators, and adjusts graph length when needed
void MicrowaveView::subNumChanged()
{
	b->graph.setLength( b->subSampLen[b->subNum.value()-1]->value() );
	b->graph.setSamples( b->storedsubs[b->subNum.value()-1] );
	setGraphEnabledColor( b->subEnabled[b->subNum.value()-1]->value() );

	int subNumValue = b->subNum.value() - 1;

	subVolKnob->setModel( b->subVol[subNumValue] );
	subVolKnob->setMatrixLocation( 2, 3, subNumValue );

	subEnabledToggle->setModel( b->subEnabled[subNumValue] );

	subPhaseKnob->setModel( b->subPhase[subNumValue] );
	subPhaseKnob->setMatrixLocation( 2, 2, subNumValue );

	subPhaseRandKnob->setModel( b->subPhaseRand[subNumValue] );

	subDetuneKnob->setModel( b->subDetune[subNumValue] );
	subDetuneKnob->setMatrixLocation( 2, 1, subNumValue );

	subMutedToggle->setModel( b->subMuted[subNumValue] );

	subKeytrackToggle->setModel( b->subKeytrack[subNumValue] );

	subSampLenKnob->setModel( b->subSampLen[subNumValue] );
	subSampLenKnob->setMatrixLocation( 2, 5, subNumValue );

	subNoiseToggle->setModel( b->subNoise[subNumValue] );

	subPanningKnob->setModel( b->subPanning[subNumValue] );
	subPanningKnob->setMatrixLocation( 2, 4, subNumValue );

	subTempoKnob->setModel( b->subTempo[subNumValue] );

	subRateLimitKnob->setModel( b->subRateLimit[subNumValue] );
	subRateLimitKnob->setMatrixLocation( 2, 6, subNumValue );

	subUnisonNumKnob->setModel( b->subUnisonNum[subNumValue] );
	subUnisonNumKnob->setMatrixLocation( 2, 7, subNumValue );

	subUnisonDetuneKnob->setModel( b->subUnisonDetune[subNumValue] );
	subUnisonDetuneKnob->setMatrixLocation( 2, 8, subNumValue );

	subInterpolateToggle->setModel( b->subInterpolate[subNumValue] );

}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::sampNumChanged()
{
	setGraphEnabledColor( b->sampleEnabled[b->sampNum.value()-1]->value() );

	for( int i = 0; i < 128; ++i )
	{
		b->graph.setSampleAt( i, b->sampGraphs[(b->sampNum.value()-1)*128+i] );
	}

	int sampNumValue = b->sampNum.value() - 1;

	sampleEnabledToggle->setModel( b->sampleEnabled[sampNumValue] );
	sampleGraphEnabledToggle->setModel( b->sampleGraphEnabled[sampNumValue] );
	sampleMutedToggle->setModel( b->sampleMuted[sampNumValue] );
	sampleKeytrackingToggle->setModel( b->sampleKeytracking[sampNumValue] );
	sampleLoopToggle->setModel( b->sampleLoop[sampNumValue] );

	sampleVolumeKnob->setModel( b->sampleVolume[sampNumValue] );
	sampleVolumeKnob->setMatrixLocation( 3, 3, sampNumValue );

	samplePanningKnob->setModel( b->samplePanning[sampNumValue] );
	samplePanningKnob->setMatrixLocation( 3, 4, sampNumValue );

	sampleDetuneKnob->setModel( b->sampleDetune[sampNumValue] );
	sampleDetuneKnob->setMatrixLocation( 3, 1, sampNumValue );

	samplePhaseKnob->setModel( b->samplePhase[sampNumValue] );
	samplePhaseKnob->setMatrixLocation( 3, 2, sampNumValue );

	samplePhaseRandKnob->setModel( b->samplePhaseRand[sampNumValue] );

	sampleStartKnob->setModel( b->sampleStart[sampNumValue] );

	sampleEndKnob->setModel( b->sampleEnd[sampNumValue] );

}


// Moves/changes the GUI around depending on the mod out section value
void MicrowaveView::modOutSecChanged( int i )
{
	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	if( i-matrixDivide < 8 && i-matrixDivide >= 0 && i < 64 )
	{
		temp1 = b->modOutSig[i]->value();
		switch( b->modOutSec[i]->value() )
		{
			case 0:// None
			{
				modOutSigBox[i-matrixDivide]->hide();
				modOutSecNumBox[i-matrixDivide]->hide();
				break;
			}
			case 1:// Main OSC
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				mainoscsignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
				break;
			}
			case 2:// Sub OSC
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				subsignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 64.f, 1.f );
				break;
			}
			case 3:// Sample OSC
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				samplesignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
				break;
			}
			case 4:// Matrix Parameters
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				matrixsignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 64.f, 1.f );
				break;
			}
			case 5:// Filter Input
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->hide();
				mod8model( b->modOutSig[i] )
				break;
			}
			case 6:// Filter Parameters
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->show();
				filtersignalsmodel( b->modOutSig[i] )
				b->modOutSecNum[i]->setRange( 1.f, 8.f, 1.f );
				break;
			}
			case 7:// Macro
			{
				modOutSigBox[i-matrixDivide]->show();
				modOutSecNumBox[i-matrixDivide]->hide();
				matrixoutmodel( b->modOutSig[i] )
				break;
			}
			default:
			{
				break;
			}
		}
		b->modOutSig[i]->setValue( temp1 );
	}
}


// Moves/changes the GUI around depending on the Mod In Section value
void MicrowaveView::modInChanged( int i )
{
	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	if( i-matrixDivide < 8 && i-matrixDivide >= 0 && i < 64 )
	{
		switch( b->modIn[i]->value() )
		{
			case 0:
			{
				modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 1:// Main OSC
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
			case 2:// Sub OSC
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 64, 1 );
				break;
			}
			case 3:// Sample OSC
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
			case 4:// Filter
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
			case 5:// Velocity
			{
				modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 6:// Panning
			{
				modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 7:// Humanizer
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 8, 1 );
				break;
			}
			case 8:// Macro
			{
				modInNumBox[i-matrixDivide]->show();
				b->modInNum[i]->setRange( 1, 18, 1 );
				break;
			}
		}
	}
}


// Moves/changes the GUI around depending on the Mod In Section value
void MicrowaveView::modIn2Changed( int i )
{
	int modScrollVal = ( matrixScrollBar->value() ) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	if( i-matrixDivide < 8 && i-matrixDivide >= 0 && i < 64 )
	{
		switch( b->modIn2[i]->value() )
		{
			case 0:
			{
				modInNumBox2[i-matrixDivide]->hide();
				break;
			}
			case 1:// Main OSC
			{
				modInNumBox2[i-matrixDivide]->show();
				b->modInNum2[i]->setRange( 1, 8, 1 );
				break;
			}
			case 2:// Sub OSC
			{
				modInNumBox2[i-matrixDivide]->show();
				b->modInNum2[i]->setRange( 1, 64, 1 );
				break;
			}
			case 3:// Sample OSC
			{
				modInNumBox2[i-matrixDivide]->show();
				b->modInNum2[i]->setRange( 1, 8, 1 );
				break;
			}
			case 4:// Filter
			{
				modInNumBox2[i-matrixDivide]->show();
				b->modInNum2[i]->setRange( 1, 8, 1 );
				break;
			}
			case 5:// Velocity
			{
				modInNumBox2[i-matrixDivide]->hide();
				break;
			}
			case 6:// Panning
			{
				modInNumBox2[i-matrixDivide]->hide();
				break;
			}
			case 7:// Humanizer
			{
				modInNumBox2[i-matrixDivide]->show();
				b->modInNum2[i]->setRange( 1, 8, 1 );
				break;
			}
			case 8:// Macro
			{
				modInNumBox2[i-matrixDivide]->show();
				b->modInNum2[i]->setRange( 1, 18, 1 );
				break;
			}
		}
	}
}




// Does what is necessary when the user visits a new tab
void MicrowaveView::tabChanged( int tabnum )
{
	b->currentTab = tabnum;

	updateBackground();

	if( tabnum != 3 )
	{
		MatrixXBtn->hide();
	}

	if( tabnum != 0 )
	{
		tab1Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1" ) );
		tab1Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1" ) );
	}
	else
	{
		tab1Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1_active" ) );
		tab1Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab1_active" ) );
	}

	if( tabnum != 1 )
	{
		tab2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2" ) );
		tab2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2" ) );
	}
	else
	{
		tab2Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2_active" ) );
		tab2Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab2_active" ) );
	}

	if( tabnum != 2 )
	{
		tab3Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3" ) );
		tab3Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3" ) );
	}
	else
	{
		tab3Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3_active" ) );
		tab3Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab3_active" ) );
	}

	if( tabnum != 3 )
	{
		tab4Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4" ) );
		tab4Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4" ) );
	}
	else
	{
		tab4Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4_active" ) );
		tab4Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab4_active" ) );
	}

	if( tabnum != 4 )
	{
		tab5Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5" ) );
		tab5Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5" ) );
	}
	else
	{
		tab5Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5_active" ) );
		tab5Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab5_active" ) );
	}

	if( tabnum != 5 )
	{
		tab6Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6" ) );
		tab6Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6" ) );
	}
	else
	{
		tab6Btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6_active" ) );
		tab6Btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tab6_active" ) );
	}
}



void MicrowaveView::updateBackground()
{
	int backgroundnum = b->currentTab;
	bool mainFlipped = b->mainFlipped.value();
	bool subFlipped = b->subFlipped.value();

	switch( backgroundnum )
	{
		case 0:// Wavetable
		{
			b->graph.setLength( 204 );
			mainNumChanged();

			if( !mainFlipped )
			{
				pal.setBrush( backgroundRole(), tab1ArtworkImg.copy() );
			}
			else
			{
				pal.setBrush( backgroundRole(), tab1FlippedArtworkImg.copy() );
			}

			setPalette( pal );
			break;
		}
		case 1:// Sub
		{
			subNumChanged();// Graph length is set in here

			if( !subFlipped )
			{
				pal.setBrush( backgroundRole(), tab2ArtworkImg.copy() );
			}
			else
			{
				pal.setBrush( backgroundRole(), tab2FlippedArtworkImg.copy() );
			}

			setPalette( pal );
			break;
		}
		case 2:// Sample
		{
			b->graph.setLength( 128 );
			sampNumChanged();

			pal.setBrush( backgroundRole(), tab3ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
		case 3:// Matrix
		{
			pal.setBrush( backgroundRole(), tab4ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
		case 4:// Effect
		{
			pal.setBrush( backgroundRole(), tab5ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
		case 5:// Miscellaneous
		{
			pal.setBrush( backgroundRole(), tab6ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
		case 6:// Wavetable Loading
		{
			pal.setBrush( backgroundRole(), tab7ArtworkImg.copy() );
			setPalette( pal );
			break;
		}
	}
}




void MicrowaveView::visualizeToggled( bool value )
{
	visvolKnob->setVisible( b->visualize.value() );
}


// V Buttons that change the graph V
void MicrowaveView::sinWaveClicked()
{
	graph->model()->setWaveToSine();
	Engine::getSong()->setModified();
}


void MicrowaveView::triangleWaveClicked()
{
	graph->model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}


void MicrowaveView::sawWaveClicked()
{
	graph->model()->setWaveToSaw();
	Engine::getSong()->setModified();
}


void MicrowaveView::sqrWaveClicked()
{
	graph->model()->setWaveToSquare();
	Engine::getSong()->setModified();
}


void MicrowaveView::noiseWaveClicked()
{
	graph->model()->setWaveToNoise();
	Engine::getSong()->setModified();
}


void MicrowaveView::usrWaveClicked()
{
	QString fileName = graph->model()->setWaveToUser();
	ToolTip::add( usrWaveBtn, fileName );
	Engine::getSong()->setModified();
}


void MicrowaveView::smoothClicked()
{
	graph->model()->smooth();
	Engine::getSong()->setModified();
}
// ^ Buttons that change the graph ^


void MicrowaveView::flipperClicked()
{
	updateBackground();
	updateScroll();
}


void MicrowaveView::XBtnClicked()
{
	castModel<Microwave>()->scroll = 0;
	updateScroll();
}


void MicrowaveView::MatrixXBtnClicked()
{
	castModel<Microwave>()->scroll = tabWhenSendingToMatrix;
	updateScroll();
}


void MicrowaveView::normalizeClicked()
{
	int oscilNum = b->mainNum.value() - 1;

	for( int i = 0; i < 256; ++i )
	{
		float highestVolume = 0;
		for( int j = 0; j < 2048; ++j )
		{
			highestVolume = abs(b->storedwaveforms[oscilNum][(i*2048)+j]) > highestVolume ? abs(b->storedwaveforms[oscilNum][(i*2048)+j]) : highestVolume;
		}
		if( highestVolume )
		{
			float multiplierThing = 1.f / highestVolume;
			for( int j = 0; j < 2048; ++j )
			{
				b->storedwaveforms[oscilNum][(i*2048)+j] *= multiplierThing;
			}
		}
	}

	Engine::getSong()->setModified();

	b->updateWavetable[oscilNum] = true;

	b->fillMainOsc(oscilNum, b->interpolate[oscilNum]->value());
}

void MicrowaveView::desawClicked()
{
	int oscilNum = b->mainNum.value() - 1;

	float start;
	float end;
	for( int j = 0; j < 256; ++j )
	{
		start = -b->storedwaveforms[oscilNum][j*2048];
		end = -b->storedwaveforms[oscilNum][j*2048+2047];
		for( int i = 0; i < 2048; ++i )
		{
			b->storedwaveforms[oscilNum][j*2048+i] += (i/2048.f)*end + ((2048.f-i)/2048.f)*start;
		}
	}

	Engine::getSong()->setModified();

	b->updateWavetable[oscilNum] = true;

	b->fillMainOsc(oscilNum, b->interpolate[oscilNum]->value());
}



void MicrowaveView::modUpClicked( int i )
{
	int modScrollVal = matrixScrollBar->value() / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;
	if( i+matrixDivide > 0 )
	{
		castModel<Microwave>()->switchMatrixSections( i+matrixDivide, i+matrixDivide - 1 );
	}
}


void MicrowaveView::modDownClicked( int i )
{
	int modScrollVal = matrixScrollBar->value() / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;
	if( i+matrixDivide < 63 )
	{
		castModel<Microwave>()->switchMatrixSections( i+matrixDivide, i+matrixDivide + 1 );
	}
}


//NOTE: Different from Microwave::modEnabledChanged.
//Changes maximum value of the Matrix scroll bar.
void MicrowaveView::modEnabledChanged()
{
	matrixScrollBar->setRange( 0, qBound( 100.f, b->maxModEnabled * 100.f, 6232.f ) + 30.f );
}


void MicrowaveView::i1Clicked( int i )
{
	int modScrollVal = matrixScrollBar->value() / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	switch( b->modIn[i+matrixDivide]->value() )
	{
		case 1:
		{
			b->scroll = 0;
			b->mainNum.setValue( b->modInNum[i+matrixDivide]->value() );
			break;
		}
		case 2:
		{
			b->scroll = 1;
			b->subNum.setValue( b->modInNum[i+matrixDivide]->value() );
			break;
		}
		case 3:
		{
			b->scroll = 2;
			b->sampNum.setValue( b->modInNum[i+matrixDivide]->value() );
			break;
		}
		case 4:
		{
			b->scroll = 3;
			effectScrollBar->setValue( ( b->modInNum[i+matrixDivide]->value() - 1 ) * 100.f );
			break;
		}
		case 8:
		{
			b->scroll = 5;
			break;
		}
	}

	updateScroll();
}


void MicrowaveView::i2Clicked( int i )
{
	int modScrollVal = matrixScrollBar->value() / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	switch( b->modIn2[i+matrixDivide]->value() )
	{
		case 1:
		{
			b->scroll = 0;
			b->mainNum.setValue( b->modInNum2[i+matrixDivide]->value() );
			break;
		}
		case 2:
		{
			b->scroll = 1;
			b->subNum.setValue( b->modInNum2[i+matrixDivide]->value() );
			break;
		}
		case 3:
		{
			b->scroll = 2;
			b->sampNum.setValue( b->modInNum2[i+matrixDivide]->value() );
			break;
		}
		case 4:
		{
			b->scroll = 3;
			effectScrollBar->setValue( ( b->modInNum2[i+matrixDivide]->value() - 1 ) * 100.f );
			break;
		}
		case 8:
		{
			b->scroll = 5;
			break;
		}
	}

	updateScroll();
}


void MicrowaveView::tabBtnClicked( int i )
{
	castModel<Microwave>()->scroll = i;
	updateScroll();
}



void MicrowaveView::sendToMatrixAsOutput( int loc1, int loc2, int loc3 )
{
	int matrixLocation = b->maxModEnabled;

	if( matrixLocation < 64 )
	{
		b->modEnabled[matrixLocation]->setValue( true );
		b->modOutSec[matrixLocation]->setValue( loc1 );
		b->modOutSig[matrixLocation]->setValue( loc2 );
		b->modOutSecNum[matrixLocation]->setValue( loc3 + 1 );
	}

	tabWhenSendingToMatrix = b->currentTab;

	MatrixXBtn->show();

	b->scroll = 3;
	updateScroll();

	matrixScrollBar->setValue( 100 * matrixLocation );
}


void MicrowaveView::switchToMatrixKnob( MicrowaveKnob * theKnob, int loc1, int loc2, int loc3 )
{
	for( int i = 0; i < 64; ++i )
	{
		if( b->modOutSec[i]->value() == loc1 && b->modOutSig[i]->value() == loc2 && b->modOutSecNum[i]->value() - 1 == loc3 )
		{
			theKnob->setModel( b->modInAmnt[i] );
			theKnob->setarcColor( QColor(23,94,40) );
			theKnob->setlineColor( QColor(51,248,99) );
			theKnob->setInnerColor( QColor(32,112,50) );
			break;
		}
	}
}


void MicrowaveView::setMacroTooltip( MicrowaveKnob * theKnob, int which )
{
	bool ok;
	QString new_val;

	new_val = QInputDialog::getText( theKnob, tr( "Set new Tooltip" ), tr( "Please enter a new Tooltip for Macro %1:" ).arg( which + 1 ), QLineEdit::Normal, b->macroTooltips[which], &ok );

	if( ok )
	{
		b->macroTooltips[which] = new_val;
		ToolTip::add( theKnob, tr( "Macro %1: " ).arg( which + 1 ) + new_val );
	}
}



void MicrowaveView::chooseMacroColor( MicrowaveKnob * theKnob, int which )
{
	QColor new_color = QColorDialog::getColor( QColor( b->macroColors[which][0], b->macroColors[which][1], b->macroColors[which][2] ) );
	if( ! new_color.isValid() )
	{
		return;
	}

	b->macroColors[which][0] = new_color.red();
	b->macroColors[which][1] = new_color.green();
	b->macroColors[which][2] = new_color.blue();
	refreshMacroColor( theKnob, which );
}


void MicrowaveView::setMacroColortoDefault( MicrowaveKnob * theKnob, int which )
{
	b->macroColors[which][0] = 102;
	b->macroColors[which][1] = 198;
	b->macroColors[which][2] = 199;
	refreshMacroColor( theKnob, which );
}


void MicrowaveView::refreshMacroColor( MicrowaveKnob * theKnob, int which )
{
	int red = b->macroColors[which][0];
	int green = b->macroColors[which][1];
	int blue = b->macroColors[which][2];

	theKnob->setarcColor( QColor(red*0.4, green*0.4, blue*0.4) );
	theKnob->setlineColor( QColor(red, green, blue) );
	theKnob->setInnerColor( QColor(red*0.5, green*0.5, blue*0.5) );
}



// Calls MicrowaveView::openWavetableFile when the wavetable opening button is clicked.
void MicrowaveView::openWavetableFileBtnClicked()
{
	if( b->scroll != 6 )
	{
		b->scroll = 6;
		updateScroll();
	}
	else
	{
		chooseWavetableFile();
	}
}


void MicrowaveView::chooseWavetableFile()
{
	SampleBuffer * sampleBuffer = new SampleBuffer;
	wavetableFileName = sampleBuffer->openAndSetWaveformFile();
	sharedObject::unref( sampleBuffer );
}


void MicrowaveView::confirmWavetableLoadClicked()
{
	openWavetableFile();
}


// All of the code and algorithms for loading wavetables from samples.  Please don't expect this code to look neat.
void MicrowaveView::openWavetableFile( QString fileName )
{
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();

	if( fileName.isEmpty() )
	{
		if( wavetableFileName.isEmpty() )
		{
			chooseWavetableFile();
		}
		fileName = wavetableFileName;
	}

	SampleBuffer * sampleBuffer = new SampleBuffer;

	sampleBuffer->setAudioFile( fileName );

	int filelength = sampleBuffer->sampleLength();
	int oscilNum = b->mainNum.value() - 1;
	int algorithm = b->loadMode.value();
	int channel = b->loadChnl.value();

	if( !fileName.isEmpty() )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		switch( algorithm )
		{
			case 0:// Lock waveform edges to zero crossings
			{
				//Clear wavetable
				for( int i = 0; i < STOREDMAINARRAYLEN; ++i )
				{
					b->storedwaveforms[oscilNum][i] = 0;
				}

				bool above = sampleBuffer->userWaveSample( 1.f/lengthOfSample, channel ) > 0;
				float currentValue = 0;
				std::vector<float> zeroCrossings;
				float previousPoint = 0;

				//Find zero crossings, and store differences between them in a vector.
				for( int i = 0; i < lengthOfSample; ++i )
				{
					currentValue = sampleBuffer->userWaveSample( i / lengthOfSample, channel );
					if( ( above && currentValue <= 0 ) || ( !above && currentValue > 0 ) )
					{
						above = !above;
						zeroCrossings.push_back( i-previousPoint );
						previousPoint = i;
					}
				}

				//Quit if the sample is too short
				if( zeroCrossings.size() < 3 )
				{
					break;
				}

				std::vector<float> betterZC;
				float now = 0;
				float actualnow = 0;

				//Find and list chosen zero crossings
				for( int i = 0; i < zeroCrossings.size() - 1; ++i )
				{
					now += zeroCrossings[i];
					actualnow += zeroCrossings[i];
					if( abs( STOREDMAINWAVELEN / 2.f - now ) < abs( STOREDMAINWAVELEN / 2.f - ( now + zeroCrossings[i+1] ) ) )
					{
						betterZC.push_back( actualnow );
						now = 0;
					}
				}

				float start;
				float end;

				float lasti = 0;
				float lastj = 0;

				bool breakify = false;

				//Take gathered information and cram it into the waveforms.
				for( int i = 0; i < betterZC.size() - 1; ++i )
				{
					start = betterZC[i];
					end = betterZC[i+1];

					lasti = i;

					for( int j = 0; j < b->sampLen[oscilNum]->value(); ++j )
					{
						lastj = j;

						if( j + ( i * b->sampLen[oscilNum]->value()) >= STOREDMAINARRAYLEN )
						{
							breakify = true;
							break;
						}
						b->storedwaveforms[oscilNum][j + ( i * (int)b->sampLen[oscilNum]->value() )] = sampleBuffer->userWaveSample( ( ( j / b->sampLen[oscilNum]->value() ) * ( end - start ) + start ) / lengthOfSample, channel );
					}

					if( breakify ) { break; }
				}

				b->morphMax[oscilNum]->setValue( lasti + ( lastj / b->sampLen[oscilNum]->value() ) );
				b->morphMaxChanged( oscilNum );

				break;
			}
			case 1:// Load sample without changes
			{
				for( int i = 0; i < STOREDMAINARRAYLEN; ++i )
				{
					if ( i <= lengthOfSample * 2.f )
					{
						b->storedwaveforms[oscilNum][i] = sampleBuffer->userWaveSample( (i/lengthOfSample) / 2.f, channel );
					}
					else// Replace everything else with silence if sample isn't long enough
					{
						b->morphMax[oscilNum]->setValue( i/b->sampLen[oscilNum]->value() );
						b->morphMaxChanged( oscilNum );
						for( int j = i; j < STOREDMAINARRAYLEN; ++j ) { b->storedwaveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 2:// For loading wavetable files
			{
				for( int i = 0; i < STOREDMAINARRAYLEN; ++i )
				{
					if ( i <= lengthOfSample )
					{
						b->storedwaveforms[oscilNum][i] = sampleBuffer->userWaveSample( i/lengthOfSample, channel );
					}
					else
					{
						b->morphMax[oscilNum]->setValue( i/b->sampLen[oscilNum]->value() );
						b->morphMaxChanged( oscilNum );
						for( int j = i; j < STOREDMAINARRAYLEN; ++j ) { b->storedwaveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 3:// Autocorrelation
			{
				// This uses a method called autocorrelation to detect the pitch, maliciously stolen from Instructables.  It can get a few Hz off (especially at higher frequencies), so I also compare it with the zero crossings to see if I can get it even more accurate.

				// Estimate pitch using autocorrelation:

				float checkLength = qMin( 4000.f, lengthOfSample );// 4000 samples should be long enough to be able to accurately detect most frequencies this way

				float threshold = -1;
				float combined = 0;
				float oldcombined;
				int stage = 0;

				float period = 0;
				for( int i = 0; i < checkLength; ++i )
				{
					oldcombined = combined;
					combined = 0;
					for( int k = 0; k < checkLength - i; ++k )
					{
						combined += ( sampleBuffer->userWaveSample( k/lengthOfSample, channel ) * sampleBuffer->userWaveSample( (k+i)/lengthOfSample, channel ) + 1 ) * 0.5f - 0.5f;
					}

					if( stage == 2 && combined - oldcombined <= 0 )
					{
						stage = 3;
						period = i;
					}

					if( stage == 1 && combined > threshold && combined - oldcombined > 0 )
					{
						stage = 2;
					}

					if( !i )
					{
						threshold = combined * 0.5f;
						stage = 1;
					}
				}

				if( !period )
				{
					break;
				}

				// Now see if the zero crossings can aid in getting the pitch even more accurate:

				// Note:  If the zero crossings give a result very close to the autocorrelation, then it is likely to be more accurate.
				// Otherwise, the zero crossing result is probably very inaccurate (common with complex sounds) and is ignored.

				std::vector<float> crossings;
				crossings.push_back( 0 );
				std::vector<float> crossingsDif;
				bool above = ( sampleBuffer->userWaveSample( 1/lengthOfSample, channel ) > 0 );

				for( int i = 0; i < checkLength; ++i )
				{
					if( ( sampleBuffer->userWaveSample( i/lengthOfSample, channel ) > 0 ) != above )
					{
						above = !above;
						if( above )
						{
							crossingsDif.push_back( i - crossings[crossings.size() - 1] );
							crossings.push_back( i );
						}
					}
				}

				crossings.erase( crossings.begin() );

				if( crossingsDif.size() >= 3 )
				{
					float crossingsMean = std::accumulate( crossingsDif.begin(), crossingsDif.end(), 0.f ) / crossingsDif.size();
					std::vector<float> crossingsToRemove;
					for( int i = 0; i < crossingsDif.size(); ++i )
					{
						if( crossingsDif[i] < crossingsMean )
						{
							crossingsToRemove.push_back( i );
						}
					}
					for( int i = crossingsToRemove.size() - 1; i >= 0; --i )
					{
						crossingsDif.erase( crossingsDif.begin() + crossingsToRemove[i] );
					}
					if( crossingsDif.size() >= 2 )
					{
						float crossingsMedian = crossingsDif[int( crossingsDif.size() / 2.f )];
						if( abs( period - crossingsMedian ) < 5.f + period / 100.f )
						{
							period = crossingsMedian;
						}
					}
				}

				for( int i = 0; i < STOREDMAINARRAYLEN; ++i )
				{
					b->storedwaveforms[oscilNum][i] = sampleBuffer->userWaveSample( ((i/2048.f)*period)/lengthOfSample, channel );
				}

				b->morphMax[oscilNum]->setValue( 254 );
				b->morphMaxChanged( oscilNum );

				break;
			}
		}
		sampleBuffer->dataUnlock();

		b->updateWavetable[oscilNum] = true;

		b->fillMainOsc(oscilNum, b->interpolate[oscilNum]->value());
	}

	sharedObject::unref( sampleBuffer );
}



// Calls MicrowaveView::openSampleFile when the sample opening button is clicked.
void MicrowaveView::openSampleFileBtnClicked( )
{
	openSampleFile();
}


// Loads sample for sample oscillator
void MicrowaveView::openSampleFile()
{
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();
	int oscilNum = b->sampNum.value() - 1;

	SampleBuffer * sampleBuffer = new SampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	int filelength = sampleBuffer->sampleLength();
	if( fileName.isEmpty() == false )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		b->samples[oscilNum][0].clear();
		b->samples[oscilNum][1].clear();

		for( int i = 0; i < lengthOfSample; ++i )
		{
			b->samples[oscilNum][0].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 0));
			b->samples[oscilNum][1].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 1));
		}
		sampleBuffer->dataUnlock();
	}
	sharedObject::unref( sampleBuffer );
}



void MicrowaveView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		wavetableFileName = value;
		b->scroll = 6;
		updateScroll();
		_de->accept();
		return;
	}
	_de->ignore();
}


void MicrowaveView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( StringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						StringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == "samplefile" )
		{
			_dee->acceptProposedAction();
		}
		else
		{
			_dee->ignore();
		}
	}
	else
	{
		_dee->ignore();
	}
}





/*                                                                
                                                                         
          ____    .--.--.                             ___      ,---,     
        ,'  , `. /  /    '.                         ,--.'|_  ,--.' |     
     ,-+-,.' _ ||  :  /`. /                 ,---,   |  | :,' |  |  :     
  ,-+-. ;   , ||;  |  |--`              ,-+-. /  |  :  : ' : :  :  :     
 ,--.'|'   |  |||  :  ;_         .--,  ,--.'|'   |.;__,'  /  :  |  |,--. 
|   |  ,', |  |, \  \    `.    /_ ./| |   |  ,"' ||  |   |   |  :  '   | 
|   | /  | |--'   `----.   \, ' , ' : |   | /  | |:__,'| :   |  |   /' : 
|   : |  | ,      __ \  \  /___/ \: | |   | |  | |  '  : |__ '  :  | | | 
|   : |  |/      /  /`--'  /.  \  ' | |   | |  |/   |  | '.'||  |  ' | : 
|   | |`-'      '--'.     /  \  ;   : |   | |--'    ;  :    ;|  :  :_:,' 
|   ;/            `--'---'    \  \  ; |   |/        |  ,   / |  | ,'     
'---'                          :  \  \'---'          ---`-'  `--''       
                                \  ' ;                                   
                                 `--`                                    
*/




// Initializes mSynth (when a new note is played).  Clone all of the arrays storing the knob values so they can be changed by modulation.
mSynth::mSynth( NotePlayHandle * _nph,
	float * morphArr, float * rangeArr, float * modifyArr, int * modifyModeArr, float * volArr, float * panArr, float * detuneArr, float * phaseArr, float * phaseRandArr, bool * enabledArr, bool * mutedArr,
	float * sampLenArr, float * morphMaxArr, float * unisonVoicesArr, float * unisonDetuneArr, float * unisonMorphArr, float * unisonModifyArr, bool * keytrackingArr, float * tempoArr, bool * interpolateArr,
	bool * subEnabledArr, bool * subMutedArr, bool * subKeytrackArr, bool * subNoiseArr, float * subVolArr, float * subPanningArr, float * subDetuneArr, float * subPhaseArr, float * subPhaseRandArr,
	float * subSampLenArr, float * subTempoArr, float * subRateLimitArr, float * subUnisonNumArr, float * subUnisonDetuneArr, bool * subInterpolateArr,
	bool * sampleEnabledArr, bool * sampleMutedArr, bool * sampleKeytrackingArr, bool * sampleGraphEnabledArr, bool * sampleLoopArr, float * sampleVolumeArr, float * samplePanningArr,
	float * sampleDetuneArr, float * samplePhaseArr, float * samplePhaseRandArr, float * sampleStartArr, float * sampleEndArr,
	int * modInArr, int * modInNumArr, float * modInAmntArr, float * modInCurveArr, int * modIn2Arr, int * modInNum2Arr, float * modInAmnt2Arr, float * modInCurve2Arr,
	int * modOutSecArr, int * modOutSigArr, int * modOutSecNumArr, bool * modEnabledArr, int * modCombineTypeArr, bool * modTypeArr, bool * modType2Arr,
	float * filtCutoffArr, float * filtResoArr, float * filtGainArr, int * filtTypeArr, int * filtSlopeArr, float * filtInVolArr, float * filtOutVolArr, float * filtWetDryArr, float * filtBalArr,
	float * filtSatuArr, float * filtFeedbackArr, float * filtDetuneArr, bool * filtEnabledArr, bool * filtMutedArr, bool * filtKeytrackingArr,
	float * macroArr,
	std::vector<float> (&samples)[8][2] ) :

	nph( _nph )
{
	memcpy( morph, morphArr, sizeof(float) * 8 );
	memcpy( range, rangeArr, sizeof(float) * 8 );
	memcpy( modify, modifyArr, sizeof(float) * 8 );
	memcpy( modifyMode, modifyModeArr, sizeof(int) * 8 );
	memcpy( vol, volArr, sizeof(int) * 8 );
	memcpy( pan, panArr, sizeof(float) * 8 );
	memcpy( detune, detuneArr, sizeof(float) * 8 );
	memcpy( phase, phaseArr, sizeof(int) * 8 );
	memcpy( phaseRand, phaseRandArr, sizeof(int) * 8 );
	memcpy( enabled, enabledArr, sizeof(bool) * 8 );
	memcpy( muted, mutedArr, sizeof(bool) * 8 );
	memcpy( sampLen, sampLenArr, sizeof(float) * 8 );
	memcpy( morphMax, morphMaxArr, sizeof(float) * 8 );
	memcpy( unisonVoices, unisonVoicesArr, sizeof(float) * 8 );
	memcpy( unisonDetune, unisonDetuneArr, sizeof(float) * 8 );
	memcpy( unisonMorph, unisonMorphArr, sizeof(float) * 8 );
	memcpy( unisonModify, unisonModifyArr, sizeof(float) * 8 );
	memcpy( keytracking, keytrackingArr, sizeof(bool) * 8 );
	memcpy( tempo, tempoArr, sizeof(float) * 8 );
	memcpy( interpolate, interpolateArr, sizeof(bool) * 8 );

	memcpy( subEnabled, subEnabledArr, sizeof(bool) * 64 );
	memcpy( subMuted, subMutedArr, sizeof(bool) * 64 );
	memcpy( subKeytrack, subKeytrackArr, sizeof(bool) * 64 );
	memcpy( subNoise, subNoiseArr, sizeof(bool) * 64 );
	memcpy( subVol, subVolArr, sizeof(float) * 64 );
	memcpy( subPanning, subPanningArr, sizeof(float) * 64 );
	memcpy( subDetune, subDetuneArr, sizeof(float) * 64 );
	memcpy( subPhase, subPhaseArr, sizeof(float) * 64 );
	memcpy( subPhaseRand, subPhaseRandArr, sizeof(float) * 64 );
	memcpy( subSampLen, subSampLenArr, sizeof(float) * 64 );
	memcpy( subTempo, subTempoArr, sizeof(float) * 64 );
	memcpy( subRateLimit, subRateLimitArr, sizeof(float) * 64 );
	memcpy( subUnisonNum, subUnisonNumArr, sizeof(float) * 64 );
	memcpy( subUnisonDetune, subUnisonDetuneArr, sizeof(float) * 64 );
	memcpy( subInterpolate, subInterpolateArr, sizeof(bool) * 64 );

	memcpy( sampleEnabled, sampleEnabledArr, sizeof(bool) * 8 );
	memcpy( sampleMuted, sampleMutedArr, sizeof(bool) * 8 );
	memcpy( sampleKeytracking, sampleKeytrackingArr, sizeof(bool) * 8 );
	memcpy( sampleGraphEnabled, sampleGraphEnabledArr, sizeof(bool) * 8 );
	memcpy( sampleLoop, sampleLoopArr, sizeof(bool) * 8 );
	memcpy( sampleVolume, sampleVolumeArr, sizeof(float) * 8 );
	memcpy( samplePanning, samplePanningArr, sizeof(float) * 8 );
	memcpy( sampleDetune, sampleDetuneArr, sizeof(float) * 8 );
	memcpy( samplePhase, samplePhaseArr, sizeof(float) * 8 );
	memcpy( samplePhaseRand, samplePhaseRandArr, sizeof(float) * 8 );
	memcpy( sampleStart, sampleStartArr, sizeof(float) * 8 );
	memcpy( sampleEnd, sampleEndArr, sizeof(float) * 8 );

	memcpy( modIn, modInArr, sizeof(int) * 64 );
	memcpy( modInNum, modInNumArr, sizeof(int) * 64 );
	memcpy( modInAmnt, modInAmntArr, sizeof(float) * 64 );
	memcpy( modInCurve, modInCurveArr, sizeof(float) * 64 );
	memcpy( modIn2, modIn2Arr, sizeof(int) * 64 );
	memcpy( modInNum2, modInNum2Arr, sizeof(int) * 64 );
	memcpy( modInAmnt2, modInAmnt2Arr, sizeof(float) * 64 );
	memcpy( modInCurve2, modInCurve2Arr, sizeof(float) * 64 );
	memcpy( modOutSec, modOutSecArr, sizeof(int) * 64 );
	memcpy( modOutSig, modOutSigArr, sizeof(int) * 64 );
	memcpy( modOutSecNum, modOutSecNumArr, sizeof(int) * 64 );
	memcpy( modEnabled, modEnabledArr, sizeof(bool) * 64 );
	memcpy( modCombineType, modCombineTypeArr, sizeof(int) * 64 );
	memcpy( modType, modTypeArr, sizeof(bool) * 64 );
	memcpy( modType2, modType2Arr, sizeof(bool) * 64 );

	memcpy( filtCutoff, filtCutoffArr, sizeof(float) * 8 );
	memcpy( filtReso, filtResoArr, sizeof(float) * 8 );
	memcpy( filtGain, filtGainArr, sizeof(float) * 8 );
	memcpy( filtType, filtTypeArr, sizeof(int) * 8 );
	memcpy( filtSlope, filtSlopeArr, sizeof(int) * 8 );
	memcpy( filtInVol, filtInVolArr, sizeof(float) * 8 );
	memcpy( filtOutVol, filtOutVolArr, sizeof(float) * 8 );
	memcpy( filtWetDry, filtWetDryArr, sizeof(float) * 8 );
	memcpy( filtBal, filtBalArr, sizeof(float) * 8 );
	memcpy( filtSatu, filtSatuArr, sizeof(float) * 8 );
	memcpy( filtFeedback, filtFeedbackArr, sizeof(float) * 8 );
	memcpy( filtDetune, filtDetuneArr, sizeof(float) * 8 );
	memcpy( filtEnabled, filtEnabledArr, sizeof(bool) * 8 );
	memcpy( filtMuted, filtMutedArr, sizeof(bool) * 8 );
	memcpy( filtKeytracking, filtKeytrackingArr, sizeof(bool) * 8 );

	memcpy( macro, macroArr, sizeof(float) * 18 );



	for( int i = 0; i < 8; ++i )
	{
		for( int j = 0; j < 32; ++j )
		{
			// Randomize the phases of all of the waveforms
			sample_realindex[i][j] = int( ( ( fastRandf( sampLen[i] * WAVERATIO ) ) * ( phaseRand[i] * 0.01f ) ) ) % int( sampLen[i] * WAVERATIO );
		}
	}

	for( int i = 0; i < 64; ++i )
	{
		for( int l = 0; l < 32; ++l )
		{
			sample_subindex[i][l] = int( ( ( fastRandf( subSampLen[i] * WAVERATIO ) - ( subSampLen[i] * WAVERATIO * 0.5f ) ) * ( subPhaseRand[i] * 0.01f ) ) ) % int( subSampLen[i] * WAVERATIO );
			subNoiseDirection[i][l] = 1;
		}
	}

	for( int i = 0; i < 8; ++i )
	{
		sample_sampleindex[i] = fmod( fastRandf( samples[i][0].size() ) * ( samplePhaseRand[i] * 0.01f ), ( samples[i][0].size() *sampleEnd[i] ) - ( samples[i][0].size() * sampleStart[i] ) ) + ( samples[i][0].size() * sampleStart[i] );
		humanizer[i] = ( rand() / float(RAND_MAX) ) * 2 - 1;// Generate humanizer values at the beginning of every note
	}

	noteDuration = -1;

	for( int i = 0; i < 8; ++i )
	{
		for( int j = 0; j < unisonVoices[i]; ++j )
		{
			unisonDetuneAmounts[i][j] = ((rand()/float(RAND_MAX))*2.f)-1;
		}
	}

	for( int i = 0; i < 64; ++i )
	{
		for( int j = 0; j < subUnisonNum[i]; ++j )
		{
			subUnisonDetuneAmounts[i][j] = ((rand()/float(RAND_MAX))*2.f)-1;
		}
	}

}


mSynth::~mSynth()
{
}


// The heart of Microwave.  As you probably learned in anatomy class, hearts actually aren't too pretty.  This is no exception.
// This is the part that puts everything together and calculates an audio output.
void mSynth::nextStringSample( sampleFrame &outputSample, float (&waveforms)[8][MAINARRAYLEN], float (&subs)[64][SUBWAVELEN], float * sampGraphs, std::vector<float> (&samples)[8][2], int maxFiltEnabled, int maxModEnabled, int maxSubEnabled, int maxSampleEnabled, int maxMainEnabled, int sample_rate, Microwave * mwc, bool removeDC, bool isOversamplingSample, float (&storedsubs)[64][STOREDSUBWAVELEN] )
{

	++noteDuration;

	//============//
	//== MATRIX ==//
	//============//

	numberToReset = 0;
	for( int l = 0; l < maxModEnabled; ++l )// maxModEnabled keeps this from looping 64 times every sample, saving a lot of CPU
	{
		if( modEnabled[l] )
		{
			switch( modIn[l] )
			{
				case 0:
				{
					curModVal[0] = 0;
					curModVal[1] = 0;
					break;
				}
				case 1:// Wavetable
				{
					if( modType[l] )// If envelope
					{
						curModVal[0] = lastMainOscEnvVal[modInNum[l]-1][0];
						curModVal[1] = lastMainOscEnvVal[modInNum[l]-1][1];
					}
					else
					{
						curModVal[0] = lastMainOscVal[modInNum[l]-1][0];
						curModVal[1] = lastMainOscVal[modInNum[l]-1][1];
					}
					break;
				}
				case 2:// Sub
				{
					if( modType[l] )// If envelope
					{
						curModVal[0] = lastSubEnvVal[modInNum[l]-1][0];
						curModVal[1] = lastSubEnvVal[modInNum[l]-1][1];
					}
					else
					{
						curModVal[0] = lastSubVal[modInNum[l]-1][0];
						curModVal[1] = lastSubVal[modInNum[l]-1][1];
					}
					break;
				}
				case 3:// Sample
				{
					if( modType[l] )// If envelope
					{
						curModVal[0] = lastSampleEnvVal[modInNum[l]-1][0];
						curModVal[1] = lastSampleEnvVal[modInNum[l]-1][1];
					}
					else
					{
						curModVal[0] = lastSampleVal[modInNum[l]-1][0];
						curModVal[1] = lastSampleVal[modInNum[l]-1][1];
					}
					break;
				}
				case 4:// Filter
				{
					curModVal[0] = filtModOutputs[modInNum[l]-1][0];
					curModVal[1] = filtModOutputs[modInNum[l]-1][1];
					break;
				}
				case 5:// Velocity
				{
					curModVal[0] = ( nph->getVolume() * 0.01f )-1;
					curModVal[1] = curModVal[0];
					break;
				}
				case 6:// Panning
				{
					curModVal[0] = ( nph->getPanning() * 0.01f );
					curModVal[1] = curModVal[0];
					break;
				}
				case 7:// Humanizer
				{
					curModVal[0] = humanizer[modInNum[l]-1];
					curModVal[1] = humanizer[modInNum[l]-1];
					break;
				}
				case 8:// Macro
				{
					curModVal[0] = macro[modInNum[l]-1] * 0.01f;
					curModVal[1] = macro[modInNum[l]-1] * 0.01f;
					break;
				}
				default:
				{
					switch( modCombineType[l] )
					{
						case 0:// Add Bidirectional
						{
							curModVal[0] = 0;
							curModVal[1] = 0;
							break;
						}
						case 1:// Multiply Bidirectional
						{
							curModVal[0] = 0;
							curModVal[1] = 0;
							break;
						}
						case 2:// Add Unidirectional
						{
							curModVal[0] = -1;
							curModVal[1] = -1;
							break;
						}
						case 3:// Multiply Unidirectional
						{
							curModVal[0] = -1;
							curModVal[1] = -1;
							break;
						}
					}
				}
			}
			switch( modIn2[l] )
			{
				case 0:
				{
					curModVal2[0] = 0;
					curModVal2[1] = 0;
					break;
				}
				case 1:// Wavetable
				{
					if( modType2[l] )// If envelope
					{
						curModVal2[0] = lastMainOscEnvVal[modInNum2[l]-1][0];
						curModVal2[1] = lastMainOscEnvVal[modInNum2[l]-1][1];
					}
					else
					{
						curModVal2[0] = lastMainOscVal[modInNum2[l]-1][0];
						curModVal2[1] = lastMainOscVal[modInNum2[l]-1][1];
					}
					break;
				}
				case 2:// Sub
				{
					if( modType2[l] )// If envelope
					{
						curModVal2[0] = lastSubEnvVal[modInNum2[l]-1][0];
						curModVal2[1] = lastSubEnvVal[modInNum2[l]-1][1];
					}
					else
					{
						curModVal2[0] = lastSubVal[modInNum2[l]-1][0];
						curModVal2[1] = lastSubVal[modInNum2[l]-1][1];
					}
					break;
				}
				case 3:// Sample
				{
					if( modType[l] )// If envelope
					{
						curModVal2[0] = lastSampleEnvVal[modInNum2[l]-1][0];
						curModVal2[1] = lastSampleEnvVal[modInNum2[l]-1][1];
					}
					else
					{
						curModVal2[0] = lastSampleVal[modInNum2[l]-1][0];
						curModVal2[1] = lastSampleVal[modInNum2[l]-1][1];
					}
					break;
				}
				case 4:// Filter
				{
					curModVal2[0] = filtModOutputs[modInNum2[l]-1][0];
					curModVal2[1] = filtModOutputs[modInNum2[l]-1][1];
				}
				case 5:// Velocity
				{
					curModVal2[0] = (nph->getVolume() * 0.01f)-1;
					curModVal2[1] = curModVal2[0];
					break;
				}
				case 6:// Panning
				{
					curModVal2[0] = (nph->getPanning() * 0.01f);
					curModVal2[1] = curModVal2[0];
					break;
				}
				case 7:// Humanizer
				{
					curModVal2[0] = humanizer[modInNum2[l]-1];
					curModVal2[1] = humanizer[modInNum2[l]-1];
					break;
				}
				case 8:// Macro
				{
					curModVal2[0] = macro[modInNum2[l]-1] * 0.01f;
					curModVal2[1] = macro[modInNum2[l]-1] * 0.01f;
					break;
				}
				default:
				{
					switch( modCombineType[l] )
					{
						case 0:// Add Bidirectional
						{
							curModVal[0] = 0;
							curModVal[1] = 0;
							break;
						}
						case 1:// Multiply Bidirectional
						{
							curModVal[0] = 0;
							curModVal[1] = 0;
							break;
						}
						case 2:// Add Unidirectional
						{
							curModVal[0] = -1;
							curModVal[1] = -1;
							break;
						}
						case 3:// Multiply Unidirectional
						{
							curModVal[0] = -1;
							curModVal[1] = -1;
							break;
						}
					}
				}
			}

			if( curModVal[0]  ) { curModVal[0]  *= modInAmnt[l]  * 0.01f; }
			if( curModVal[1]  ) { curModVal[1]  *= modInAmnt[l]  * 0.01f; }
			if( curModVal2[0] ) { curModVal2[0] *= modInAmnt2[l] * 0.01f; }
			if( curModVal2[1] ) { curModVal2[1] *= modInAmnt2[l] * 0.01f; }

			// Calculate curve
			if( modCombineType[l] <= 1 )// Bidirectional
			{
				if( modInCurve[l] != 100.f )// The "if" statement is there so unnecessary CPU isn't spent (pow is very expensive) if the curve knob isn't being used.
				{
					// Move to a scale of 0 to 1 (from -1 to 1) and then apply the curve.
					curModValCurve[0] = (curModVal[0] <= -1 || curModVal[0] >= 1) ? ( curModVal[0] + 1 ) * 0.5f : pow( ( curModVal[0] + 1 ) * 0.5f, 1.f / ( modInCurve[l] * 0.01f ) );
					curModValCurve[1] = (curModVal[1] <= -1 || curModVal[1] >= 1) ? ( curModVal[1] + 1 ) * 0.5f : pow( ( curModVal[1] + 1 ) * 0.5f, 1.f / ( modInCurve[l] * 0.01f ) );
				}
				else
				{
					curModValCurve[0] = ( curModVal[0] + 1 ) * 0.5f;
					curModValCurve[1] = ( curModVal[1] + 1 ) * 0.5f;
				}
				if( modInCurve2[l] != 100.f )
				{
					curModVal2Curve[0] = (curModVal2[0] <= -1 || curModVal2[0] >= 1) ? ( curModVal2[0] + 1 ) * 0.5f : pow( ( curModVal2[0] + 1 ) * 0.5f, 1.f / ( modInCurve2[l] * 0.01f ) );
					curModVal2Curve[1] = (curModVal2[1] <= -1 || curModVal2[1] >= 1) ? ( curModVal2[1] + 1 ) * 0.5f : pow( ( curModVal2[1] + 1 ) * 0.5f, 1.f / ( modInCurve2[l] * 0.01f ) );
				}
				else
				{
					curModVal2Curve[0] = ( curModVal2[0] + 1 ) * 0.5f;
					curModVal2Curve[1] = ( curModVal2[1] + 1 ) * 0.5f;
				}
			}
			else// Unidirectional
			{
				if( modInCurve[l] != 100.f )
				{
					curModValCurve[0] = ( (curModVal[0] <= -1 || curModVal[0] >= 1) ? curModVal[0] : pow( abs( curModVal[0] ), 1.f / ( modInCurve[l] * 0.01f ) ) * ( curModVal[0] < 0 ? -1 : 1 ) ) + (modInAmnt[l] * 0.01);
					curModValCurve[1] = ( (curModVal[1] <= -1 || curModVal[1] >= 1) ? curModVal[1] : pow( abs( curModVal[1] ), 1.f / ( modInCurve[l] * 0.01f ) ) * ( curModVal[1] < 0 ? -1 : 1 ) ) + (modInAmnt[l] * 0.01);
				}
				else
				{
					curModValCurve[0] = curModVal[0] + (modInAmnt[l] * 0.01);
					curModValCurve[1] = curModVal[1] + (modInAmnt[l] * 0.01);
				}
				if( modInCurve2[l] != 100.f )
				{
					curModVal2Curve[0] = ( (curModVal2[0] <= -1 || curModVal2[0] >= 1) ? curModVal2[0] : pow( abs( curModVal2[0] ), 1.f / ( modInCurve2[l] * 0.01f ) ) * ( curModVal2[0] < 0 ? -1 : 1 ) ) + (modInAmnt2[l] * 0.01);
					curModVal2Curve[1] = ( (curModVal2[1] <= -1 || curModVal2[1] >= 1) ? curModVal2[1] : pow( abs( curModVal2[0] ), 1.f / ( modInCurve2[l] * 0.01f ) ) * ( curModVal2[0] < 0 ? -1 : 1 ) ) + (modInAmnt2[l] * 0.01);
				}
				else
				{
					curModVal2Curve[0] = curModVal2[0] + (modInAmnt2[l] * 0.01);
					curModVal2Curve[1] = curModVal2[1] + (modInAmnt2[l] * 0.01);
				}
			}

			switch( modCombineType[l] )
			{
				case 0:// Add Bidirectional
				{
					comboModVal[0] = curModValCurve[0] + curModVal2Curve[0] - 1;
					comboModVal[1] = curModValCurve[1] + curModVal2Curve[1] - 1;
					break;
				}
				case 1:// Multiply Bidirectional
				{
					comboModVal[0] = ( curModValCurve[0] * 2 - 1 ) * ( curModVal2Curve[0] * 2 - 1 );
					comboModVal[1] = ( curModValCurve[1] * 2 - 1 ) * ( curModVal2Curve[1] * 2 - 1 );
					break;
				}
				case 2:// Add Unidirectional
				{
					comboModVal[0] = curModValCurve[0] + curModVal2Curve[0];
					comboModVal[1] = curModValCurve[1] + curModVal2Curve[1];
					break;
				}
				case 3:// Multiply Unidirectional
				{
					comboModVal[0] = curModValCurve[0] * curModVal2Curve[0];
					comboModVal[1] = curModValCurve[1] * curModVal2Curve[1];
					break;
				}
				default:
				{
					comboModVal[0] = 0;
					comboModVal[1] = 0;
				}
			}

			comboModValMono = ( comboModVal[0] + comboModVal[1] ) * 0.5f;

			switch( modOutSec[l] )
			{
				case 0:
				{
					break;
				}
				case 1:// Main Oscillator
				{
					switch( modOutSig[l] )
					{
						case 0:
						{
							break;
						}
						case 1:// Send input to Morph
						{
							morph[modOutSecNum[l]-1] = qBound( 0.f, morph[modOutSecNum[l]-1] + comboModValMono*morphMax[modOutSecNum[l]-1], morphMax[modOutSecNum[l]-1] );
							modValType[numberToReset] = 1;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 2:// Send input to Range
						{
							range[modOutSecNum[l]-1] = qMax( 0.f, range[modOutSecNum[l]-1] + comboModValMono * 16.f );
							modValType[numberToReset] = 2;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 3:// Send input to Modify
						{
							modify[modOutSecNum[l]-1] = qMax( 0.f, modify[modOutSecNum[l]-1] + comboModValMono * 2048.f );
							modValType[numberToReset] = 3;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 4:// Send input to Pitch/Detune
						{
							detune[modOutSecNum[l]-1] = detune[modOutSecNum[l]-1] + comboModValMono * 4800.f;
							modValType[numberToReset] = 7;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 5:// Send input to Phase
						{
							phase[modOutSecNum[l]-1] = phase[modOutSecNum[l]-1] + comboModValMono * 8.f;
							modValType[numberToReset] = 8;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 6:// Send input to Volume
						{
							vol[modOutSecNum[l]-1] = qMax( 0.f, vol[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType[numberToReset] = 5;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 7:// Send input to Panning
						{
							pan[modOutSecNum[l]-1] = qMax( 0.f, pan[modOutSecNum[l]-1] + comboModValMono * 200.f );
							modValType[numberToReset] = 6;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 8:// Send input to Unison Voice Number
						{
							unisonVoices[modOutSecNum[l]-1] = qMax( 0.f, unisonVoices[modOutSecNum[l]-1] + comboModValMono * 32.f );
							modValType[numberToReset] = 14;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 9:// Send input to Unison Detune
						{
							unisonDetune[modOutSecNum[l]-1] = qMax( 0.f, unisonDetune[modOutSecNum[l]-1] + comboModValMono * 2000.f );
							modValType[numberToReset] = 15;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 10:// Send input to Unison Morph
						{
							unisonMorph[modOutSecNum[l]-1] = qBound( 0.f, unisonMorph[modOutSecNum[l]-1] + comboModValMono*morphMax[modOutSecNum[l]-1], morphMax[modOutSecNum[l]-1] );
							modValType[numberToReset] = 16;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 11:// Send input to Unison Modify
						{
							unisonModify[modOutSecNum[l]-1] = qMax( 0.f, unisonModify[modOutSecNum[l]-1] + comboModValMono * 2048.f );
							modValType[numberToReset] = 17;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						default:
						{
						}
					}
					break;
				}
				case 2:// Sub Oscillator
				{
					switch( modOutSig[l] )
					{
						case 0:
						{
							break;
						}
						case 1:// Send input to Pitch/Detune
						{
							subDetune[modOutSecNum[l]-1] = subDetune[modOutSecNum[l]-1] + comboModValMono * 4800.f;
							modValType[numberToReset] = 36;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 2:// Send input to Phase
						{
							subPhase[modOutSecNum[l]-1] = subPhase[modOutSecNum[l]-1] + comboModValMono * 8.f;
							modValType[numberToReset] = 37;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 3:// Send input to Volume
						{
							subVol[modOutSecNum[l]-1] = qMax( 0.f, subVol[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType[numberToReset] = 34;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 4:// Send input to Panning
						{
							subPanning[modOutSecNum[l]-1] = qMax( 0.f, subPanning[modOutSecNum[l]-1] + comboModValMono * 200.f );
							modValType[numberToReset] = 35;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 5:// Send input to Sample Length
						{
							subSampLen[modOutSecNum[l]-1] = qMax( 1, int( subSampLen[modOutSecNum[l]-1] + comboModValMono * STOREDSUBWAVELEN ) );
							modValType[numberToReset] = 39;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 6:// Send input to Rate Limit
						{
							subRateLimit[modOutSecNum[l]-1] = qMax( 0.f, subRateLimit[modOutSecNum[l]-1] + comboModValMono * 2.f );
							modValType[numberToReset] = 41;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 7:// Send input to Unison Voice Number
						{
							subUnisonNum[modOutSecNum[l]-1] = qMax( 1.f, subUnisonNum[modOutSecNum[l]-1] + comboModValMono * 32.f );
							modValType[numberToReset] = 42;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 8:// Send input to Unison Detune
						{
							subUnisonDetune[modOutSecNum[l]-1] = qMax( 0.f, subUnisonDetune[modOutSecNum[l]-1] + comboModValMono * 2000.f );
							modValType[numberToReset] = 43;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						default:
						{
						}
					}
					break;
				}
				case 3:// Sample Oscillator
				{
					switch( modOutSig[l] )
					{
						case 0:
						{
							break;
						}
						case 1:// Send input to Pitch/Detune
						{
							sampleDetune[modOutSecNum[l]-1] = sampleDetune[modOutSecNum[l]-1] + comboModValMono * 4800.f;
							modValType[numberToReset] = 67;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 2:// Send input to Phase
						{
							samplePhase[modOutSecNum[l]-1] = samplePhase[modOutSecNum[l]-1] + comboModValMono * 8.f;
							modValType[numberToReset] = 68;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 3:// Send input to Volume
						{
							sampleVolume[modOutSecNum[l]-1] = qMax( 0.f, sampleVolume[modOutSecNum[l]-1] + comboModValMono * 100.f );
							modValType[numberToReset] = 65;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 4:// Send input to Panning
						{
							samplePanning[modOutSecNum[l]-1] = qBound( -100.f, samplePanning[modOutSecNum[l]-1] + comboModValMono * 100.f, 100.f );
							modValType[numberToReset] = 66;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						default:
						{
						}
					}
					break;
				}
				case 4:// Matrix
				{
					switch( modOutSig[l] )
					{
						case 0:
						{
							break;
						}
						case 1:// Mod In Amount
						{
							modInAmnt[modOutSecNum[l]-1] = modInAmnt[modOutSecNum[l]-1] + comboModValMono*100.f;
							modValType[numberToReset] = 92;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 2:// Mod In Curve
						{
							modInCurve[modOutSecNum[l]-1] = qMax( 0.f, modInCurve[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 93;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 3:// Secondary Mod In Amount
						{
							modInAmnt2[modOutSecNum[l]-1] = modInAmnt2[modOutSecNum[l]-1] + comboModValMono*100.f;
							modValType[numberToReset] = 95;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 4:// Secondary Mod In Curve
						{
							modInCurve2[modOutSecNum[l]-1] = qMax( 0.f, modInCurve2[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 96;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 5:// Mod In
						{
							modIn[modOutSecNum[l]-1] = qMax( 0.f, modIn[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 90;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 6:// Mod In Num
						{
							modInNum[modOutSecNum[l]-1] = qMax( 0.f, modInNum[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 91;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 7:// Secondary Mod In
						{
							modIn2[modOutSecNum[l]-1] = qMax( 0.f, modIn2[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 94;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 8:// Secondary Mod In Num
						{
							modInNum2[modOutSecNum[l]-1] = qMax( 0.f, modInNum2[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 95;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 9:// Mod Out Sec
						{
							modOutSec[modOutSecNum[l]-1] = qMax( 0.f, modOutSec[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 98;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 10:// Mod Out Sig
						{
							modOutSig[modOutSecNum[l]-1] = qMax( 0.f, modOutSig[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 99;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 11:// Mod Out Sec Num
						{
							modOutSecNum[modOutSecNum[l]-1] = qMax( 0.f, modOutSecNum[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 100;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
					}
					break;
				}
				case 5:// Filter Input
				{
					filtInputs[modOutSig[l]][0] += curModVal[0];
					filtInputs[modOutSig[l]][1] += curModVal[1];
					break;
				}
				case 6:// Filter Parameters
				{
					switch( modOutSig[l] )
					{
						case 0:// None
						{
							break;
						}
						case 1:// Cutoff Frequency
						{
							filtCutoff[modOutSecNum[l]-1] = qBound( 20.f, filtCutoff[modOutSecNum[l]-1] + comboModValMono*19980.f, 21999.f );
							modValType[numberToReset] = 120;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 2:// Resonance
						{
							filtReso[modOutSecNum[l]-1] = qMax( 0.0001f, filtReso[modOutSecNum[l]-1] + comboModValMono*16.f );
							modValType[numberToReset] = 121;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 3:// dbGain
						{
							filtGain[modOutSecNum[l]-1] = filtGain[modOutSecNum[l]-1] + comboModValMono*64.f;
							modValType[numberToReset] = 122;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 4:// Filter Type
						{
							filtType[modOutSecNum[l]-1] = qMax( 0, int(filtType[modOutSecNum[l]-1] + comboModValMono*7.f ) );
							modValType[numberToReset] = 123;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 5:// Filter Slope
						{
							filtSlope[modOutSecNum[l]-1] = qMax( 0, int(filtSlope[modOutSecNum[l]-1] + comboModValMono*8.f ) );
							modValType[numberToReset] = 124;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 6:// Input Volume
						{
							filtInVol[modOutSecNum[l]-1] = qMax( 0.f, filtInVol[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 125;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 7:// Output Volume
						{
							filtOutVol[modOutSecNum[l]-1] = qMax( 0.f, filtOutVol[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 126;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 8:// Wet/Dry
						{
							filtWetDry[modOutSecNum[l]-1] = qBound( 0.f, filtWetDry[modOutSecNum[l]-1] + comboModValMono*100.f, 100.f );
							modValType[numberToReset] = 127;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 9:// Balance/Panning
						{
							filtBal[modOutSecNum[l]-1] = qBound( -100.f, filtBal[modOutSecNum[l]-1] + comboModValMono*200.f, 100.f );
							modValType[numberToReset] = 128;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 10:// Saturation
						{
							filtSatu[modOutSecNum[l]-1] = qMax( 0.f, filtSatu[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 129;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 11:// Feedback
						{
							filtFeedback[modOutSecNum[l]-1] = qMax( 0.f, filtFeedback[modOutSecNum[l]-1] + comboModValMono*100.f );
							modValType[numberToReset] = 130;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
						case 12:// Detune
						{
							filtDetune[modOutSecNum[l]-1] = filtDetune[modOutSecNum[l]-1] + comboModValMono*4800.f;
							modValType[numberToReset] = 131;
							modValNum[numberToReset] = modOutSecNum[l]-1;
							++numberToReset;
							break;
						}
					}
					break;
				}
				case 7:// Macro
				{
					macro[modOutSig[l]] = macro[modOutSig[l]] + comboModValMono * 200.f;
					modValType[numberToReset] = 150;
					modValNum[numberToReset] = modOutSig[l];
					++numberToReset;
					break;
				}
				default:
				{
				}
			}
		}
	}

	//============//
	//== FILTER ==//
	//============//

	// As much as it may seem like it, this section contains no intentional attempts at obfuscation.

	for( int l = 0; l < maxFiltEnabled; ++l )
	{
		if( filtEnabled[l] )
		{
			temp1 = filtInVol[l] * 0.01f;
			filtInputs[l][0] *= temp1;
			filtInputs[l][1] *= temp1;

			if( filtKeytracking[l] )
			{
				temp1 = round( sample_rate / detuneWithCents( nph->frequency(), filtDetune[l] ) );
			}
			else
			{
				temp1 = round( sample_rate / detuneWithCents( 440.f, filtDetune[l] ) );
			}
			if( filtDelayBuf[l][0].size() < temp1 )
			{
				filtDelayBuf[l][0].resize( temp1 );
				filtDelayBuf[l][1].resize( temp1 );
			}

			++filtFeedbackLoc[l];
			if( filtFeedbackLoc[l] > temp1 - 1 )
			{
				filtFeedbackLoc[l] = 0;
			}

			filtInputs[l][0] += filtDelayBuf[l][0].at( filtFeedbackLoc[l] );
			filtInputs[l][1] += filtDelayBuf[l][1].at( filtFeedbackLoc[l] );

			cutoff = filtCutoff[l];
			mode = filtType[l];
			reso = filtReso[l];
			dbgain = filtGain[l];
			Fs = sample_rate;
			w0 = D_2PI * cutoff / Fs;

			if( mode == 8 )
			{
				f = 2 * cutoff / Fs;
				k = 3.6*f - 1.6*f*f - 1;
				p = (k+1)*0.5f;
				scale = pow( D_E, (1-p)*1.386249f );
				r = reso*scale;
			}

			for( int m = 0; m < filtSlope[l] + 1; ++m )// m is the slope number.  So if m = 2, then the sound is going from a 24 db to a 36 db slope, for example.
			{
				if( m )
				{
					filtInputs[l][0] = filtOutputs[l][0];
					filtInputs[l][1] = filtOutputs[l][1];
				}

				int formulaType = 1;
				if( mode <= 7 )
				{
					switch( mode )
					{
						case 0:// LP
						{
							temp1 = cos(w0);
							alpha = sin(w0) / (2*reso);
							b0 = ( 1 - temp1 ) * 0.5f;
							b1 = 1 - temp1;
							b2 = b0;
							a0 = 1 + alpha;
							a1 = -2 * temp1;
							a2 = 1 - alpha;
							break;
						}
						case 1:// HP
						{
							temp1 = cos(w0);
							alpha = sin(w0) / (2*reso);
							b0 =  (1 + temp1) * 0.5f;
							b1 = -(1 + temp1);
							b2 =  b0;
							a0 =   1 + alpha;
							a1 =  -2 * temp1;
							a2 =   1 - alpha;
							break;
						}
						case 2:// BP
						{
							temp2 = sin(w0);
							alpha = temp2*sinh( log(2) * 0.5 * reso * w0/temp2 );
							b0 =   alpha;
							b1 =   0;
							b2 =  -alpha;
							a0 =   1 + alpha;
							a1 =  -2*cos(w0);
							a2 =   1 - alpha;
							formulaType = 2;
							break;
						}
						case 3:// Low Shelf
						{
							temp1 = cos(w0);
							A = exp10( dbgain / 40 );
							alpha = sin(w0)/2 * sqrt( (A + 1/A)*(1/reso - 1) + 2 );
							temp2 = 2*sqrt(A)*alpha;
							b0 =    A*( (A+1) - (A-1)*temp1 + temp2 );
							b1 =  2*A*( (A-1) - (A+1)*temp1         );
							b2 =    A*( (A+1) - (A-1)*temp1 - temp2 );
							a0 =        (A+1) + (A-1)*temp1 + temp2;
							a1 =   -2*( (A-1) + (A+1)*temp1         );
							a2 =        (A+1) + (A-1)*temp1 - temp2;
							break;
						}
						case 4:// High Shelf
						{
							temp1 = cos(w0);
							A = exp10( dbgain / 40 );
							alpha = sin(w0)/2 * sqrt( (A + 1/A)*(1/reso - 1) + 2 );
							temp2 = 2*sqrt(A)*alpha;
							b0 =    A*( (A+1) + (A-1)*temp1 + temp2 );
							b1 = -2*A*( (A-1) + (A+1)*temp1         );
							b2 =    A*( (A+1) + (A-1)*temp1 - temp2 );
							a0 =        (A+1) - (A-1)*temp1 + temp2;
							a1 =    2*( (A-1) - (A+1)*temp1         );
							a2 =        (A+1) - (A-1)*temp1 - temp2;
							break;
						}
						case 5:// Peak
						{
							temp1 = -2 * cos(w0);
							temp2 = sin(w0);
							alpha = temp2*sinh( log(2) * 0.5 * reso * w0/temp2 );
							A = exp10( dbgain / 40 );
							b0 =   1 + alpha*A;
							b1 =  temp1;
							b2 =   1 - alpha*A;
							a0 =   1 + alpha/A;
							a1 =  temp1;
							a2 =   1 - alpha/A;
							break;
						}
						case 6:// Notch
						{
							temp1 = -2 * cos(w0);
							temp2 = sin(w0);
							alpha = temp2*sinh( log(2) * 0.5 * reso * w0/temp2 );
							b0 =   1;
							b1 =  temp1;
							b2 =   1;
							a0 =   1 + alpha;
							a1 =  temp1;
							a2 =   1 - alpha;
							break;
						}
						case 7:// Allpass
						{
							temp1 = -2 * cos(w0);
							alpha = sin(w0) / (2*reso);
							b0 = 1 - alpha;
							b1 = temp1;
							b2 = 1 + alpha;
							a0 = b2;
							a1 = temp1;
							a2 = b0;
							formulaType = 3;
							break;
						}
					}

					// Translation of this monstrosity (case 1): y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]
					// See www.musicdsp.org/files/Audio-EQ-Cookbook.txt
					switch( formulaType )
					{
						case 1:// Normal
						{
							temp1 = b0/a0;
							temp2 = b1/a0;
							temp3 = b2/a0;
							temp4 = a1/a0;
							temp5 = a2/a0;
							filtPrevSampOut[l][m][0][0] = temp1*filtInputs[l][0] + temp2*filtPrevSampIn[l][m][1][0] + temp3*filtPrevSampIn[l][m][2][0] - temp4*filtPrevSampOut[l][m][1][0] - temp5*filtPrevSampOut[l][m][2][0];// Left ear
							filtPrevSampOut[l][m][0][1] = temp1*filtInputs[l][1] + temp2*filtPrevSampIn[l][m][1][1] + temp3*filtPrevSampIn[l][m][2][1] - temp4*filtPrevSampOut[l][m][1][1] - temp5*filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
						case 2:// Bandpass
						{
							temp1 = b0/a0;
							temp3 = b2/a0;
							temp4 = a1/a0;
							temp5 = a2/a0;
							filtPrevSampOut[l][m][0][0] = temp1*filtInputs[l][0] + temp3*filtPrevSampIn[l][m][2][0] - temp4*filtPrevSampOut[l][m][1][0] - temp5*filtPrevSampOut[l][m][2][0];// Left ear
							filtPrevSampOut[l][m][0][1] = temp1*filtInputs[l][1] + temp3*filtPrevSampIn[l][m][2][1] - temp4*filtPrevSampOut[l][m][1][1] - temp5*filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
						case 3:// Allpass
						{
							temp1 = b0/a0;
							temp2 = b1/a0;
							temp4 = a1/a0;
							temp5 = a2/a0;
							filtPrevSampOut[l][m][0][0] = temp1*filtInputs[l][0] + temp2*filtPrevSampIn[l][m][1][0] + filtPrevSampIn[l][m][2][0] - temp4*filtPrevSampOut[l][m][1][0] - temp5*filtPrevSampOut[l][m][2][0];// Left ear
							filtPrevSampOut[l][m][0][1] = temp1*filtInputs[l][1] + temp2*filtPrevSampIn[l][m][1][1] + filtPrevSampIn[l][m][2][1] - temp4*filtPrevSampOut[l][m][1][1] - temp5*filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
						case 4:// Notch
						{
							temp1 = 1.f/a0;
							temp2 = b1/a0;
							temp3 = temp1;
							temp4 = temp2;
							temp5 = a2/a0;
							filtPrevSampOut[l][m][0][0] = temp1*filtInputs[l][0] + temp2*filtPrevSampIn[l][m][1][0] + temp3*filtPrevSampIn[l][m][2][0] - temp4*filtPrevSampOut[l][m][1][0] - temp5*filtPrevSampOut[l][m][2][0];// Left ear
							filtPrevSampOut[l][m][0][1] = temp1*filtInputs[l][1] + temp2*filtPrevSampIn[l][m][1][1] + temp3*filtPrevSampIn[l][m][2][1] - temp4*filtPrevSampOut[l][m][1][1] - temp5*filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
					}

					//Output results
					temp1 = filtOutVol[l] * 0.01f;
					filtOutputs[l][0] = filtPrevSampOut[l][m][0][0] * temp1;
					filtOutputs[l][1] = filtPrevSampOut[l][m][0][1] * temp1;

				}
				else if( mode == 8 )
				{
					// Moog 24db Lowpass
					filtx[0] = filtInputs[l][0]-r*filty4[0];
					filtx[1] = filtInputs[l][1]-r*filty4[1];
					for( int i = 0; i < 2; ++i )
					{
						filty1[i]=filtx[i]*p + filtoldx[i]*p - k*filty1[i];
						filty2[i]=filty1[i]*p+filtoldy1[i]*p - k*filty2[i];
						filty3[i]=filty2[i]*p+filtoldy2[i]*p - k*filty3[i];
						filty4[i]=filty3[i]*p+filtoldy3[i]*p - k*filty4[i];
						filty4[i] = filty4[i] - filty4[i] * filty4[i] * filty4[i] / 6.f;
						filtoldx[i] = filtx[i];
						filtoldy1[i] = filty1[i];
						filtoldy2[i] = filty2[i];
						filtoldy3[i] = filty3[i];
					}
					temp1 = filtOutVol[l] * 0.01f;
					filtOutputs[l][0] = filty4[0] * temp1;
					filtOutputs[l][1] = filty4[1] * temp1;
				}

				// Calculates Saturation.  The algorithm is just y = x ^ ( 1 - saturation );
				if( filtSatu[l] )
				{
					temp1 = 1 - ( filtSatu[l] * 0.01f );
					filtOutputs[l][0] = pow( abs( filtOutputs[l][0] ), temp1 ) * ( filtOutputs[l][0] < 0 ? -1 : 1 );
					filtOutputs[l][1] = pow( abs( filtOutputs[l][1] ), temp1 ) * ( filtOutputs[l][1] < 0 ? -1 : 1 );
				}

				// Balance knob wet
				if( filtBal[l] )
				{
					filtOutputs[l][0] *= filtBal[l] > 0 ? ( 100.f - filtBal[l] ) * 0.01f : 1.f;
					filtOutputs[l][1] *= filtBal[l] < 0 ? ( 100.f + filtBal[l] ) * 0.01f : 1.f;
				}

				// Wet
				if( filtWetDry[l] != 100 )
				{
					temp1 = filtWetDry[l] * 0.01f;
					filtOutputs[l][0] *= temp1;
					filtOutputs[l][1] *= temp1;
				}

				// Balance knob dry
				if( filtBal[l] )
				{
					filtOutputs[l][0] += ( 1.f - ( filtBal[l] > 0 ? ( 100.f - filtBal[l] ) * 0.01f : 1.f ) ) * filtInputs[l][0];
					filtOutputs[l][1] += ( 1.f - ( filtBal[l] < 0 ? ( 100.f + filtBal[l] ) * 0.01f : 1.f ) ) * filtInputs[l][1];
				}

				// Dry
				temp1 = 1.f - ( filtWetDry[l] * 0.01f );
				filtOutputs[l][0] += temp1 * filtInputs[l][0];
				filtOutputs[l][1] += temp1 * filtInputs[l][1];

				// Send back past samples
				filtPrevSampIn[l][m][4][0] = filtPrevSampIn[l][m][3][0];
				filtPrevSampIn[l][m][4][1] = filtPrevSampIn[l][m][3][1];

				filtPrevSampIn[l][m][3][0] = filtPrevSampIn[l][m][2][0];
				filtPrevSampIn[l][m][3][1] = filtPrevSampIn[l][m][2][1];

				filtPrevSampIn[l][m][2][0] = filtPrevSampIn[l][m][1][0];
				filtPrevSampIn[l][m][2][1] = filtPrevSampIn[l][m][1][1];

				filtPrevSampIn[l][m][1][0] = filtInputs[l][0];
				filtPrevSampIn[l][m][1][1] = filtInputs[l][1];

				filtPrevSampOut[l][m][4][0] = filtPrevSampOut[l][m][3][0];
				filtPrevSampOut[l][m][4][1] = filtPrevSampOut[l][m][3][1];

				filtPrevSampOut[l][m][3][0] = filtPrevSampOut[l][m][2][0];
				filtPrevSampOut[l][m][3][1] = filtPrevSampOut[l][m][2][1];

				filtPrevSampOut[l][m][2][0] = filtPrevSampOut[l][m][1][0];
				filtPrevSampOut[l][m][2][1] = filtPrevSampOut[l][m][1][1];

				filtPrevSampOut[l][m][1][0] = filtPrevSampOut[l][m][0][0];
				filtPrevSampOut[l][m][1][1] = filtPrevSampOut[l][m][0][1];
			}

			temp1 = filtFeedback[l] * 0.01f;
			filtDelayBuf[l][0][filtFeedbackLoc[l]] = filtOutputs[l][0] * temp1;
			filtDelayBuf[l][1][filtFeedbackLoc[l]] = filtOutputs[l][1] * temp1;

			filtInputs[l][0] = 0;
			filtInputs[l][1] = 0;

			filtModOutputs[l][0] = filtOutputs[l][0];
			filtModOutputs[l][1] = filtOutputs[l][1];

			if( filtMuted[l] )
			{
				filtOutputs[l][0] = 0;
				filtOutputs[l][1] = 0;
			}
		}
	}

	//=====================//
	//== MAIN OSCILLATOR ==//
	//=====================//

	for( int i = 0; i < maxMainEnabled; ++i )// maxMainEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		if( enabled[i] )
		{
			currentSampLen = sampLen[i] * WAVERATIO;
			for( int l = 0; l < unisonVoices[i]; ++l )
			{
				sample_morerealindex[i][l] = realfmod( ( sample_realindex[i][l] + ( phase[i] * currentSampLen * 0.01f ) ), currentSampLen );// Calculates phase

				unisonVoicesMinusOne = unisonVoices[i] - 1;// unisonVoices[i] - 1 is needed many times, which is why unisonVoicesMinusOne exists

				if( tempo[i] )
				{
					temp1 = tempo[i] / 26400.f;
					noteFreq = unisonVoicesMinusOne ? detuneWithCents( keytracking[i] ? nph->frequency() : 440.f, unisonDetuneAmounts[i][l]*unisonDetune[i]+detune[i] ) : detuneWithCents( keytracking[i] ? nph->frequency() : 440.f, detune[i] );// Calculates frequency depending on detune and unison detune
					noteFreq *= temp1;// Tempo sync
				}
				else
				{
					noteFreq = unisonVoicesMinusOne ? detuneWithCents( keytracking[i] ? nph->frequency() : 440.f, unisonDetuneAmounts[i][l]*unisonDetune[i]+detune[i] ) : detuneWithCents( keytracking[i] ? nph->frequency() : 440.f, detune[i] );// Calculates frequency depending on detune and unison detune
				}

				sample_step[i][l] = currentSampLen * ( noteFreq / sample_rate );

				if( unisonVoicesMinusOne )// Figures out Morph and Modify values for individual unison voices
				{
					if( unisonMorph[i] )
					{
						temp1 = ((unisonVoicesMinusOne-l)/unisonVoicesMinusOne)*unisonMorph[i];
						morph[i] = qBound( 0.f, temp1 - ( unisonMorph[i] * 0.5f ) + morph[i], morphMax[i] );
					}

					if( unisonModify[i] )
					{
						temp1 = ((unisonVoicesMinusOne-l)/unisonVoicesMinusOne)*unisonModify[i];
						modify[i] = qBound( 0.f, temp1 - ( unisonModify[i] * 0.5f ) + modify[i], currentSampLen-1.f);// SampleLength - 1 = ModifyMax
					}
				}

				temp7 = modify[i] * WAVERATIO;

				switch( modifyMode[i] )// Horrifying formulas for the various Modify Modes
				{
					case 0:// None
					{
						break;
					}
					case 1:// Pulse Width
					{
						sample_morerealindex[i][l] /= ( -temp7 + currentSampLen ) / currentSampLen;

						sample_morerealindex[i][l] = qBound( 0.f, sample_morerealindex[i][l], (float)currentSampLen - 1);// Keeps sample index within bounds
						break;
					}
					case 2:// Weird 1
					{
						//The cool result of me messing up.
						sample_morerealindex[i][l] = ( ( ( sin( ( ( sample_morerealindex[i][l] / currentSampLen ) * ( temp7 / 50.f ) ) / 2 ) ) * currentSampLen ) * ( temp7 / currentSampLen ) + ( sample_morerealindex[i][l] + ( ( -temp7 + currentSampLen ) / currentSampLen ) ) ) / 2.f;
						break;
					}
					case 3:// Weird 2
					{
						//Also the cool result of me messing up.
						if( sample_morerealindex[i][l] > currentSampLen / 2.f )
						{
							sample_morerealindex[i][l] = pow( sample_morerealindex[i][l] / currentSampLen, temp7 * 10 / currentSampLen + 1 ) * currentSampLen;
						}
						else
						{
							sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
							sample_morerealindex[i][l] = pow( sample_morerealindex[i][l] / currentSampLen, temp7 * 10 / currentSampLen + 1 ) * currentSampLen;
							sample_morerealindex[i][l] = sample_morerealindex[i][l] - currentSampLen / 2.f;
						}
						break;
					}
					case 4:// Asym To Right
					{
						sample_morerealindex[i][l] = pow( sample_morerealindex[i][l] / currentSampLen, temp7 * 10 / currentSampLen + 1 ) * currentSampLen;
						break;
					}
					case 5:// Asym To Left
					{
						sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
						sample_morerealindex[i][l] = pow( sample_morerealindex[i][l] / currentSampLen, temp7 * 10 / currentSampLen + 1 ) * currentSampLen;
						sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
						break;
					}
					case 6:// Bidirectional Asym
					{
						temp1 = temp7 / currentSampLen;
						if( temp1 < 0.5 )
						{
							temp1 = ( -temp1 + 1 ) / 2.f - 0.25f;
							sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
							sample_morerealindex[i][l] = pow( sample_morerealindex[i][l] / currentSampLen, temp1 * 10 + 1 ) * currentSampLen;
							sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
						}
						else
						{
							temp1 = temp1 / 2.f - 0.25f;
							sample_morerealindex[i][l] = pow( sample_morerealindex[i][l] / currentSampLen, temp1 * 10 + 1 ) * currentSampLen;
						}
						break;
					}
					case 7:// Stretch From Center
					{
						temp1 = currentSampLen / 2.f;
						sample_morerealindex[i][l] -= temp1;
						sample_morerealindex[i][l] /= temp1;
						sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( ( temp7 * 4 ) / currentSampLen + 1 ) ) : -pow( -sample_morerealindex[i][l], 1 / ( ( temp7 * 4 ) / currentSampLen + 1 ) );
						sample_morerealindex[i][l] *= temp1;
						sample_morerealindex[i][l] += temp1;
						break;
					}
					case 8:// Squish To Center
					{
						temp1 = currentSampLen / 2.f;
						sample_morerealindex[i][l] -= temp1;
						sample_morerealindex[i][l] /= temp1;
						sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( -temp7 / currentSampLen + 1  ) ) : -pow( -sample_morerealindex[i][l], 1 / ( -temp7 / currentSampLen + 1 ) );
						sample_morerealindex[i][l] *= temp1;
						sample_morerealindex[i][l] += temp1;
						break;
					}
					case 9:// Stretch And Squish
					{
						temp1 = currentSampLen / 2.f;
						sample_morerealindex[i][l] -= temp1;
						sample_morerealindex[i][l] /= temp1;
						sample_morerealindex[i][l] = ( sample_morerealindex[i][l] >= 0 ) ? pow( sample_morerealindex[i][l], 1 / ( temp7 * 4 / currentSampLen ) ) : -pow( -sample_morerealindex[i][l], 1 / ( temp7 * 4 / currentSampLen ) );
						sample_morerealindex[i][l] *= temp1;
						sample_morerealindex[i][l] += temp1;
						break;
					}
					case 10:// Cut Off Right
					{
						sample_morerealindex[i][l] *= ( -temp7 + currentSampLen ) / currentSampLen;
						break;
					}
					case 11:// Cut Off Left
					{
						sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
						sample_morerealindex[i][l] *= ( -temp7 + currentSampLen ) / currentSampLen;
						sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
						break;
					}
					case 19:// Sync
					{
						sample_morerealindex[i][l] *= ( ( temp7 * 16.f ) / currentSampLen ) + 1;
						sample_morerealindex[i][l] = fmod( sample_morerealindex[i][l], (float)currentSampLen - 1 );
						break;
					}
					case 20:// Sync Half Interpolate
					{
						sample_morerealindex[i][l] *= ( ( temp7 * 16.f ) / currentSampLen ) + 1;
						sample_morerealindex[i][l] = fmod( sample_morerealindex[i][l], (float)currentSampLen - 1 );
						break;
					}
					case 21:// Sync Interpolate
					{
						sample_morerealindex[i][l] *= ( ( temp7 * 16.f ) / currentSampLen ) + 1;
						sample_morerealindex[i][l] = fmod( sample_morerealindex[i][l], (float)currentSampLen - 1 );
						break;
					}
					case 22:// Mirror
					{
						sample_morerealindex[i][l] = sample_morerealindex[i][l] < currentSampLen / 2 ? sample_morerealindex[i][l] * 2 : ( -sample_morerealindex[i][l] + currentSampLen ) * 2;
						temp1 = temp7 / currentSampLen;
						if( temp1 < 0.5 )
						{
							temp1 = ( -temp1 + 1 ) / 2.f - 0.25f;
							sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
							sample_morerealindex[i][l] = pow( sample_morerealindex[i][l] / currentSampLen, temp1 * 10 + 1 ) * currentSampLen;
							sample_morerealindex[i][l] = -sample_morerealindex[i][l] + currentSampLen;
						}
						else
						{
							temp1 = temp1 / 2.f - 0.25f;
							sample_morerealindex[i][l] = pow( sample_morerealindex[i][l] / currentSampLen, temp1 * 10 + 1 ) * currentSampLen;
						}
						break;
					}
					default:
						{}
				}

				mainsample[i][l] = 0;

				if( modifyMode[i] != 12 && modifyMode[i] != 13 && modifyMode[i] != 23 && modifyMode[i] != 24 )// If NOT Squarify/Pulsify/Diagonal/Sideways Modify Mode
				{
					loopStart = qMax( 0.f, morph[i] - range[i] ) + 1;
					loopEnd = qMin( morph[i] + range[i], MAINARRAYLEN / (float)currentSampLen ) + 1;
					currentRangeValInvert = 1.f / range[i];
					currentIndex = sample_morerealindex[i][l];

					// Only grab samples from the waveforms when they will be used, depending on the Range knob
					for( int j = loopStart; j < loopEnd; ++j )
					{
						// Get waveform samples, set their volumes depending on Range knob
						mainsample[i][l] += waveforms[i][currentIndex + j * currentSampLen] * ( 1 - ( abs( morph[i] - j ) * currentRangeValInvert ) );
					}
				}
				else if( modifyMode[i] == 12 )// If Squarify Modify Mode
				{
					loopStart = qMax( 0.f, morph[i] - range[i] ) + 1;
					loopEnd = qMin( morph[i] + range[i], MAINARRAYLEN / (float)currentSampLen ) + 1;
					currentRangeValInvert = 1.f / range[i];
					currentIndex = sample_morerealindex[i][l];

					// Self-made formula, may be buggy.  Crossfade one half of waveform with the inverse of the other.

					// Taking these calculations out of the following loop will help with performance with high Range values.
					temp2 = temp7 / currentSampLen;
					temp3 = temp2;
					++temp2;
					temp4 = int( currentIndex + ( currentSampLen * 0.5 ) ) % currentSampLen;

					for( int j = loopStart; j < loopEnd; ++j )
					{
						temp1 = 1 - ( abs( morph[i] - j ) * currentRangeValInvert );
						mainsample[i][l] += (
						(  waveforms[i][currentIndex + j * currentSampLen] * temp1 ) + // Normal
						( -waveforms[i][int(temp4)   + j * currentSampLen] * temp1 * temp3 ) ) / // Inverted other half of waveform
						temp2; // Volume compensation
					}
				}
				else if( modifyMode[i] == 13 )// If Pulsify Modify Mode
				{
					loopStart = qMax( 0.f, morph[i] - range[i] ) + 1;
					loopEnd = qMin( morph[i] + range[i], MAINARRAYLEN / (float)currentSampLen ) + 1;
					currentRangeValInvert = 1.f / range[i];
					currentIndex = sample_morerealindex[i][l];

					// Self-made formula, may be buggy.  Crossfade one side of waveform with the inverse of the other.

					// Taking this calculation out of the following loop will help with performance with high Range values.
					temp2 = ( currentIndex + int( currentSampLen * ( temp7 / currentSampLen ) ) ) % currentSampLen;

					for( int j = loopStart; j < loopEnd; ++j )
					{
						temp1 = 1 - ( abs( morph[i] - j ) * currentRangeValInvert );
						mainsample[i][l] += (
						(    waveforms[i][currentIndex + j * currentSampLen] * temp1 ) + // Normal
						( ( -waveforms[i][int(temp2) + j * currentSampLen] * temp1 ) ) ) * // Inverted other side of waveform
						0.5f; // Volume compensation
					}
				}
				else if( modifyMode[i] == 23 )// If Diagonal Modify Mode
				{
					temp5 = realfmod( morph[i] + ( sample_morerealindex[i][l] * temp7 / currentSampLen / 32.f ), morphMax[i] );
					loopStart = temp5 - range[i] + 1;
					loopEnd = temp5 + range[i] + 1;
					currentRangeValInvert = 1.f / range[i];
					currentIndex = sample_morerealindex[i][l];

					// Quite experimental, morph value is changed depending on wavetable position.
					for( int j = loopStart; j < loopEnd; ++j )
					{
						temp3 = realfmod( j, morphMax[i] );
						mainsample[i][l] += waveforms[i][currentIndex + (int)temp3 * currentSampLen] * ( 1 - ( abs( temp5 - j ) * currentRangeValInvert ) );
					}
				}
				else if( modifyMode[i] == 24 )// If Sideways Modify Mode
				{
					temp5 = sample_morerealindex[i][l] / currentSampLen * morphMax[i];
					loopStart = qMax( 0.f, temp5 - range[i] ) + 1;
					loopEnd = qMin( temp5 + range[i], MAINARRAYLEN / (float)currentSampLen ) + 1;
					currentRangeValInvert = 1.f / range[i];
					currentIndex = temp7;

					// Quite experimental, swap the Morph value (now controlled using Modify) and the location in the waveform.
					for( int j = loopStart; j < loopEnd; ++j )
					{
						// Get waveform samples, set their volumes depending on Range knob
						mainsample[i][l] += waveforms[i][currentIndex + j * currentSampLen] * ( 1 - ( abs( temp5 - j ) * currentRangeValInvert ) );
					}
				}

				switch( modifyMode[i] )// More horrifying formulas for the various Modify Modes
				{
					case 1:// Pulse Width
					{
						if( sample_realindex[i][l] / ( ( -temp7 + currentSampLen ) / currentSampLen ) > currentSampLen )
						{
							mainsample[i][l] = 0;
						}
						break;
					}
					case 14:// Flip
					{
						if( sample_realindex[i][l] > temp7 )
						{
							mainsample[i][l] *= -1;
						}
						break;
					}
					case 15:// Clip
					{
						temp1 = temp7 / ( currentSampLen - 1 );
						mainsample[i][l] = qBound( -temp1, mainsample[i][l], temp1 );
						break;
					}
					case 16:// Inverse Clip
					{
						temp1 = temp7 / ( currentSampLen - 1 );
						if( mainsample[i][l] > 0 && mainsample[i][l] < temp1 )
						{
							mainsample[i][l] = temp1;
						}
						else if( mainsample[i][l] < 0 && mainsample[i][l] > -temp1 )
						{
							mainsample[i][l] = -temp1;
						}
						break;
					}
					case 17:// Sine
					{
						temp1 = temp7 / ( currentSampLen - 1 );
						mainsample[i][l] = sin( mainsample[i][l] * ( D_PI * temp1 * 8 + 1 ) );
						break;
					}
					case 18:// Atan
					{
						temp1 = temp7 / ( currentSampLen - 1 );
						mainsample[i][l] = atan( mainsample[i][l] * ( D_PI * temp1 * 8 + 1 ) );
						break;
					}
					case 20:// Sync Half Interpolate
					{
						temp1 = currentSampLen / 2.f;
						temp2 = currentSampLen / 4.f;
						if( sample_realindex[i][l] < temp2 || sample_realindex[i][l] > 3.f * temp2 )
						{
							mainsample[i][l] *= ( -abs( sample_realindex[i][l] - temp1 ) + temp1 ) / currentSampLen * 4.f;
						}
						break;
					}
					case 21:// Sync Interpolate
					{
						temp1 = currentSampLen / 2.f;
						mainsample[i][l] *= ( -abs( sample_realindex[i][l] - temp1 ) + temp1 ) / currentSampLen * 2.f;
						break;
					}
					default:
						{}
				}

				sample_realindex[i][l] += sample_step[i][l];

				// check overflow
				while( sample_realindex[i][l] >= currentSampLen )
				{
					sample_realindex[i][l] -= currentSampLen;
					lastMainOscEnvDone[l] = true;
				}

				mainsample[i][l] *= vol[i] * 0.01f;
			}
		}
	}

	//====================//
	//== SUB OSCILLATOR ==//
	//====================//

	for( int i = 0; i < maxSubEnabled; ++i )// maxSubEnabled keeps this from looping 64 times every sample, saving a lot of CPU
	{
		if( subEnabled[i] )
		{
			if( !subNoise[i] )
			{
				temp2 = subPhase[i] * subSampLen[i];
				temp3 = subVol[i] * 0.01f;
				temp4 = subSampLen[i] * WAVERATIO;

				for( int l = 0; l < subUnisonNum[i]; ++l )
				{
					subUnisonVoicesMinusOne = subUnisonNum[i] - 1;// subUnisonNum[i] - 1 is needed many times, which is why subUnisonVoicesMinusOne exists

					if( !subUnisonVoicesMinusOne )
					{
						if( subTempo[i] )
						{
							temp1 = subTempo[i] / 26400.f;
							noteFreq = subKeytrack[i] ? detuneWithCents( nph->frequency(), subDetune[i] ) * temp1 : detuneWithCents( 440.f, subDetune[i] ) * temp1;
						}
						else
						{
							noteFreq = subKeytrack[i] ? detuneWithCents( nph->frequency(), subDetune[i] ) : detuneWithCents( 440.f, subDetune[i] );
						}
					}
					else
					{
						if( subTempo[i] )
						{
							temp1 = subTempo[i] / 26400.f;
							noteFreq = subKeytrack[i] ? detuneWithCents( nph->frequency(), subUnisonDetuneAmounts[i][l]*subUnisonDetune[i]+subDetune[i] ) * temp1 : detuneWithCents( 440.f, subUnisonDetuneAmounts[i][l]*subUnisonDetune[i]+subDetune[i] ) * temp1;
						}
						else
						{
							noteFreq = subKeytrack[i] ? detuneWithCents( nph->frequency(), subUnisonDetuneAmounts[i][l]*subUnisonDetune[i]+subDetune[i] ) : detuneWithCents( 440.f, subUnisonDetuneAmounts[i][l]*subUnisonDetune[i]+subDetune[i] );
						}
					}

					sample_step_sub = temp4 / ( sample_rate / noteFreq );

					subsample[i][l] = temp3 * subs[i][int( realfmod( sample_subindex[i][l] + temp2, temp4 ) )];

					sample_subindex[i][l] += sample_step_sub;

					// move waveform position back if it passed the end of the waveform
					while ( sample_subindex[i][l] >= temp4 )
					{
						sample_subindex[i][l] -= temp4;
						lastSubEnvDone[i] = true;
					}

					/* That is all that is needed for the sub oscillator calculations.

					(To the tune of Hallelujah)

						There was a Happy CPU
					No processing power to chew through
					In this wonderful synthesis brew
						Hallelujah

						But with some wavetable synthesis
					Your CPU just blows a kiss
					And leaves you there despite your miss
						Hallelujah

					Hallelujah, Hallelujah, Hallelujah, Halleluuu-uuuuuuuu-uuuujah

						Your music ambition lays there, dead
					Can't get your ideas out of your head
					Because your computer's slower than lead
						Hallelujah

						Sometimes you may try and try
					To keep your ping from saying goodbye
					Leaving you to die and cry
						Hallelujah

					Hallelujah, Hallelujah, Hallelujah, Halleluuu-uuuuuuuu-uuuujah

						But what is this, an alternative?
					Sub oscillators supported native
					To deter CPU obliteratives
						Hallelujah

						Your music has come back to life
					CPU problems cut off like a knife
					Sub oscillators removing your strife
						Hallelujah

					Hallelujah, Hallelujah, Hallelujah, Halleluuu-uuuuuuuu-uuuujah

					*cool outro*

					*/
				}
			}
			else// sub oscillator is noise
			{
				temp2 = subVol[i] * 0.01f;
				for( int l = 0; l < subUnisonNum[i]; ++l )
				{
					noiseSampRand = fastRandf( subSampLen[l] ) - 1;

					temp1 = ( storedsubs[i][int(noiseSampRand)] * subNoiseDirection[i][l] ) + lastSubNoiseVal[i][l];
					if( temp1 > 1 || temp1 < -1 )
					{
						subNoiseDirection[i][l] *= -1;
						temp1 = ( storedsubs[i][int( noiseSampRand )] * subNoiseDirection[i][l] ) + lastSubNoiseVal[i][l];
					}

					lastSubNoiseVal[i][l] = temp1;

					subsample[i][l] = temp1 * temp2;
				}
			}
		}
	}

	//=======================//
	//== SAMPLE OSCILLATOR ==//
	//=======================//

	for( int l = 0; l < maxSampleEnabled; ++l )// maxSampleEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		int sampleSize = samples[l][0].size() * sampleEnd[l];
		if( sampleEnabled[l] && ( sample_sampleindex[l] < sampleSize || sampleLoop[l] ) )
		{
			if( sampleLoop[l] )
			{
				if( sample_sampleindex[l] > sampleSize )
				{
					sample_sampleindex[l] = sample_sampleindex[l] - sampleSize + ( samples[l][0].size() * sampleStart[l] );
					lastSampleEnvDone[l] = true;
				}
			}

			sample_step_sample = ( detuneWithCents( sampleKeytracking[l] ? nph->frequency() : 440.f, sampleDetune[l] ) / 440.f ) * ( 44100.f / sample_rate );

			if( sampleGraphEnabled[l] && sampleStart[l] < sampleEnd[l] )
			{
				progress = realfmod( ( sample_sampleindex[l] + ( samplePhase[l] * sampleSize * 0.01f ) ), sampleSize ) / sampleSize * 128.f;
				intprogress = (int)progress;

				temp1 = fmod( progress, 1 );
				progress2 = sampGraphs[intprogress] * ( 1 - temp1 );

				if( intprogress < 127 )
				{
					progress3 = sampGraphs[intprogress+1] * temp1;
				}
				else
				{
					progress3 = sampGraphs[intprogress] * temp1;
				}

				temp1 = int( ( ( progress2 + progress3 + 1 ) * 0.5f ) * sampleSize );
				samplesample[l][0] = samples[l][0][temp1];
				samplesample[l][1] = samples[l][1][temp1];
			}
			else
			{
				temp1 = realfmod(( sample_sampleindex[l] + ( samplePhase[l] * sampleSize * 0.01f ) ), sampleSize);
				samplesample[l][0] = samples[l][0][temp1];
				samplesample[l][1] = samples[l][1][temp1];
			}

			temp1 = sampleVolume[l] * 0.01f;
			samplesample[l][0] *= temp1;
			samplesample[l][1] *= temp1;

			if( samplePanning[l] < 0 )
			{
				samplesample[l][1] *= ( 100.f + samplePanning[l] ) * 0.01f;
			}
			else if( samplePanning[l] > 0 )
			{
				samplesample[l][0] *= ( 100.f - samplePanning[l] ) * 0.01f;
			}

			lastSampleVal[l][0] = samplesample[l][0];// Store value for modulation
			lastSampleVal[l][1] = samplesample[l][1];// Store value for modulation

			if( !lastSampleEnvDone[l] )
			{
				lastSampleEnvVal[l][0] = lastSampleVal[l][0];
				lastSampleEnvVal[l][1] = lastSampleVal[l][1];
			}

			if( sampleMuted[l] )
			{
				samplesample[l][0] = 0;
				samplesample[l][1] = 0;
			}

			sample_sampleindex[l] += sample_step_sample;
		}
	}

	outputSample[0] = 0;
	outputSample[1] = 0;
	// Main Oscillator outputs
	for( int i = 0; i < maxMainEnabled; ++i )
	{
		if( enabled[i] )
		{
			unisonVoicesMinusOne = unisonVoices[i] - 1;

			if( unisonVoicesMinusOne )
			{
				sampleMainOsc[0] = 0;
				sampleMainOsc[1] = 0;
				for( int j = 0; j < unisonVoices[i]; ++j )
				{
					// Pan unison voices
					sampleMainOsc[0] += mainsample[i][j] * ((unisonVoicesMinusOne-j)/unisonVoicesMinusOne);
					sampleMainOsc[1] += mainsample[i][j] * (j/unisonVoicesMinusOne);
				}
				// Decrease volume so more unison voices won't increase volume too much
				temp1 = unisonVoices[i] * 0.5f;
				sampleMainOsc[0] /= temp1;
				sampleMainOsc[1] /= temp1;
			}
			else
			{
				sampleMainOsc[0] = mainsample[i][0];
				sampleMainOsc[1] = mainsample[i][0];
			}

			if( pan[i] )
			{
				if( pan[i] < 0 )
				{
					sampleMainOsc[1] *= ( 100.f + pan[i] ) * 0.01f;
				}
				else
				{
					sampleMainOsc[0] *= ( 100.f - pan[i] ) * 0.01f;
				}
			}

			lastMainOscVal[i][0] = sampleMainOsc[0];// Store results for modulations
			lastMainOscVal[i][1] = sampleMainOsc[1];

			// second half of "if" statement makes sure the last envelope value is the last value of the graph (not of the waveform), so the sinc interpolation doesn't ruin things.
			if( !lastMainOscEnvDone[i] && sample_realindex[i][0] <= sampLen[i] * WAVERATIO - ( WAVERATIO * 2 ) )
			{
				lastMainOscEnvVal[i][0] = lastMainOscVal[i][0];
				lastMainOscEnvVal[i][1] = lastMainOscVal[i][1];
			}

			if( !muted[i] )
			{
				outputSample[0] += sampleMainOsc[0];
				outputSample[1] += sampleMainOsc[1];
			}
		}
	}

	// Sub Oscillator outputs
	for( int i = 0; i < maxSubEnabled; ++i )
	{
		if( subEnabled[i] )
		{
			if( subUnisonNum[i] > 1 )
			{
				subUnisonVoicesMinusOne = subUnisonNum[i] - 1;

				sampleSubOsc[0] = 0;
				sampleSubOsc[1] = 0;
				for( int j = 0; j < subUnisonNum[i]; ++j )
				{
					// Pan unison voices
					sampleSubOsc[0] += subsample[i][j] * ((subUnisonVoicesMinusOne-j)/subUnisonVoicesMinusOne);
					sampleSubOsc[1] += subsample[i][j] * (j/subUnisonVoicesMinusOne);
				}
				// Decrease volume so more unison voices won't increase volume too much
				temp1 = subUnisonNum[i] * 0.5f;
				sampleSubOsc[0] /= temp1;
				sampleSubOsc[1] /= temp1;
			}
			else
			{
				sampleSubOsc[0] = subsample[i][0];
				sampleSubOsc[1] = subsample[i][0];
			}

			if( subPanning[i] )
			{
				if( subPanning[i] < 0 )
				{
					sampleSubOsc[1] *= ( 100.f + subPanning[i] ) * 0.01f;
				}
				else
				{
					sampleSubOsc[0] *= ( 100.f - subPanning[i] ) * 0.01f;
				}
			}

			if( subRateLimit[i] )
			{
				sampleSubOsc[0] = lastSubVal[i][0] + qBound( -subRateLimit[i], sampleSubOsc[0] - lastSubVal[i][0], subRateLimit[i] );
				sampleSubOsc[1] = lastSubVal[i][1] + qBound( -subRateLimit[i], sampleSubOsc[1] - lastSubVal[i][1], subRateLimit[i] );
			}

			lastSubVal[i][0] = sampleSubOsc[0];// Store results for modulations
			lastSubVal[i][1] = sampleSubOsc[1];

			// second half of "if" statement makes sure the last envelope value is the last value of the graph (not of the waveform), so the sinc interpolation doesn't ruin things.
			if( !lastSubEnvDone[i] && sample_subindex[i][0] <= subSampLen[i] * WAVERATIO - ( WAVERATIO * 2 ) )
			{
				lastSubEnvVal[i][0] = lastSubVal[i][0];
				lastSubEnvVal[i][1] = lastSubVal[i][1];
			}

			if( !subMuted[i] )
			{
				outputSample[0] += sampleSubOsc[0];
				outputSample[1] += sampleSubOsc[1];
			}
		}
	}

	// Sample Oscillator outputs
	for( int l = 0; l < maxSampleEnabled; ++l )// maxSampleEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		if( sampleEnabled[l] )
		{
			outputSample[0] += samplesample[l][0];
			outputSample[1] += samplesample[l][1];
		}
	}

	// Filter outputs
	for( int l = 0; l < maxFiltEnabled; ++l )// maxFiltEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		if( filtEnabled[l] )
		{
			outputSample[0] += filtOutputs[l][0];
			outputSample[1] += filtOutputs[l][1];
		}
	}

	// Refresh all modulated values back to the value of the knob.
	for( int i = 0; i < numberToReset; ++i )
	{
		refreshValue( modValType[i], modValNum[i], mwc );
	}
	numberToReset = 0;

	if( removeDC )
	{
		averageSampleValue[0] = ( averageSampleValue[0] * 0.999f ) + ( outputSample[0] * 0.001f );
		averageSampleValue[1] = ( averageSampleValue[1] * 0.999f ) + ( outputSample[1] * 0.001f );

		outputSample[0] -= averageSampleValue[0];
		outputSample[1] -= averageSampleValue[1];
	}
}


// Takes input of original Hz and the number of cents to detune it by, and returns the detuned result in Hz.
inline float mSynth::detuneWithCents( float pitchValue, float detuneValue )
{
	if( detuneValue )// Avoids expensive exponentiation if no detuning is necessary
	{
		return pitchValue * std::exp2( detuneValue / 1200.f ); 
	}
	else
	{
		return pitchValue;
	}
}


// At the end of mSynth::nextStringSample, this will refresh all modulated values back to the value of the knob.
inline void mSynth::refreshValue( int which, int num, Microwave * mwc )
{
	switch( which )
	{
		case 1: morph[num] = mwc->morph[num]->value(); break;
		case 2: range[num] = mwc->range[num]->value(); break;
		case 3: modify[num] = mwc->modify[num]->value(); break;
		case 4: modifyMode[num] = mwc->modifyMode[num]->value(); break;
		case 5: vol[num] = mwc->vol[num]->value(); break;
		case 6: pan[num] = mwc->pan[num]->value(); break;
		case 7: detune[num] = mwc->detune[num]->value(); break;
		case 8: phase[num] = mwc->phase[num]->value(); break;
		case 9: phaseRand[num] = mwc->phaseRand[num]->value(); break;
		case 10: enabled[num] = mwc->enabled[num]->value(); break;
		case 11: muted[num] = mwc->muted[num]->value(); break;
		case 12: sampLen[num] = mwc->sampLen[num]->value(); break;
		case 13: morphMax[num] = mwc->morphMax[num]->value(); break;
		case 14: unisonVoices[num] = mwc->unisonVoices[num]->value(); break;
		case 15: unisonDetune[num] = mwc->unisonDetune[num]->value(); break;
		case 16: unisonMorph[num] = mwc->unisonMorph[num]->value(); break;
		case 17: unisonModify[num] = mwc->unisonModify[num]->value(); break;
		case 18: keytracking[num] = mwc->keytracking[num]->value(); break;
		case 19: tempo[num] = mwc->tempo[num]->value(); break;
		case 20: interpolate[num] = mwc->interpolate[num]->value(); break;

		case 30: subEnabled[num] = mwc->subEnabled[num]->value(); break;
		case 31: subMuted[num] = mwc->subMuted[num]->value(); break;
		case 32: subKeytrack[num] = mwc->subKeytrack[num]->value(); break;
		case 33: subNoise[num] = mwc->subNoise[num]->value(); break;
		case 34: subVol[num] = mwc->subVol[num]->value(); break;
		case 35: subPanning[num] = mwc->subPanning[num]->value(); break;
		case 36: subDetune[num] = mwc->subDetune[num]->value(); break;
		case 37: subPhase[num] = mwc->subPhase[num]->value(); break;
		case 38: subPhaseRand[num] = mwc->subPhaseRand[num]->value(); break;
		case 39: subSampLen[num] = mwc->subSampLen[num]->value(); break;
		case 40: subTempo[num] = mwc->subTempo[num]->value(); break;
		case 41: subRateLimit[num] = mwc->subRateLimit[num]->value(); break;
		case 42: subUnisonNum[num] = mwc->subUnisonNum[num]->value(); break;
		case 43: subUnisonDetune[num] = mwc->subUnisonDetune[num]->value(); break;
		case 44: subInterpolate[num] = mwc->subInterpolate[num]->value(); break;

		case 60: sampleEnabled[num] = mwc->sampleEnabled[num]->value(); break;
		case 61: sampleMuted[num] = mwc->sampleMuted[num]->value(); break;
		case 62: sampleKeytracking[num] = mwc->sampleKeytracking[num]->value(); break;
		case 63: sampleGraphEnabled[num] = mwc->sampleGraphEnabled[num]->value(); break;
		case 64: sampleLoop[num] = mwc->sampleLoop[num]->value(); break;
		case 65: sampleVolume[num] = mwc->sampleVolume[num]->value(); break;
		case 66: samplePanning[num] = mwc->samplePanning[num]->value(); break;
		case 67: sampleDetune[num] = mwc->sampleDetune[num]->value(); break;
		case 68: samplePhase[num] = mwc->samplePhase[num]->value(); break;
		case 69: samplePhaseRand[num] = mwc->samplePhaseRand[num]->value(); break;
		case 70: sampleStart[num] = mwc->sampleStart[num]->value(); break;
		case 71: sampleEnd[num] = mwc->sampleEnd[num]->value(); break;

		case 90: modIn[num] = mwc->modIn[num]->value(); break;
		case 91: modInNum[num] = mwc->modInNum[num]->value(); break;
		case 92: modInAmnt[num] = mwc->modInAmnt[num]->value(); break;
		case 93: modInCurve[num] = mwc->modInCurve[num]->value(); break;
		case 94: modIn2[num] = mwc->modIn2[num]->value(); break;
		case 95: modInNum2[num] = mwc->modInNum2[num]->value(); break;
		case 96: modInAmnt2[num] = mwc->modInAmnt2[num]->value(); break;
		case 97: modInCurve2[num] = mwc->modInCurve2[num]->value(); break;
		case 98: modOutSec[num] = mwc->modOutSec[num]->value(); break;
		case 99: modOutSig[num] = mwc->modOutSig[num]->value(); break;
		case 100: modOutSecNum[num] = mwc->modOutSecNum[num]->value(); break;
		case 101: modEnabled[num] = mwc->modEnabled[num]->value(); break;
		case 102: modCombineType[num] = mwc->modCombineType[num]->value(); break;
		case 103: modType[num] = mwc->modType[num]->value(); break;
		case 104: modType2[num] = mwc->modType2[num]->value(); break;

		case 120: filtCutoff[num] = mwc->filtCutoff[num]->value(); break;
		case 121: filtReso[num] = mwc->filtReso[num]->value(); break;
		case 122: filtGain[num] = mwc->filtGain[num]->value(); break;
		case 123: filtType[num] = mwc->filtType[num]->value(); break;
		case 124: filtSlope[num] = mwc->filtSlope[num]->value(); break;
		case 125: filtInVol[num] = mwc->filtInVol[num]->value(); break;
		case 126: filtOutVol[num] = mwc->filtOutVol[num]->value(); break;
		case 127: filtWetDry[num] = mwc->filtWetDry[num]->value(); break;
		case 128: filtBal[num] = mwc->filtBal[num]->value(); break;
		case 129: filtSatu[num] = mwc->filtSatu[num]->value(); break;
		case 130: filtFeedback[num] = mwc->filtFeedback[num]->value(); break;
		case 131: filtDetune[num] = mwc->filtDetune[num]->value(); break;
		case 132: filtEnabled[num] = mwc->filtEnabled[num]->value(); break;
		case 133: filtMuted[num] = mwc->filtMuted[num]->value(); break;
		case 134: filtKeytracking[num] = mwc->filtKeytracking[num]->value(); break;

		case 150: macro[num] = mwc->macro[num]->value(); break;
	}
}


// Handles negative values properly, unlike fmod.
inline float mSynth::realfmod( float k, float n )
{
	return ((k = fmod(k,n)) < 0) ? k+n : k;
}













QString MicrowaveManualView::s_manualText=
"HOW TO OPERATE YOUR MICROWAVE<br>"
"<br>"
"Table of Contents:<br>"
"<br>"
"1. Feature Overview<br>"
" a. Wavetable tab<br>"
" b. Sub Oscillator Tab<br>"
" c. Sample Tab<br>"
" d. Matrix Tab<br>"
" e. Filter Tab<br>"
" f. Miscellaneous Tab<br>"
" g. Other features to be aware of<br>"
"2. CPU Preservation Guidelines<br>"
"3. FAQ<br>"
"<br>"
"<br>"
"<br>"
"<br>"
"<br>"
"<br>"
"==FEATURE OVERVIEW==<br>"
"<br>"
"<br>"
"-=WAVETABLE TAB=-<br>"
"<br>"
"If you zoom in all the way on a sound or waveform, you'll see these little \"audio pixels\".  These are called \"samples\", not to be confused with the common term \"sound sample\" which refers to any stored piece of audio.  These \"audio pixels\" can easily be seen when using LMMS's BitInvader.<br>"
"<br>"
"A \"wavetable synthesizer\" is a synthesizer that stores its waveforms as a list of samples.  This means synthesizers like BitInvader and WatSyn are technically wavetable synthesizers.  But, the term \"wavetable synthesizer\" more commonly (but not more or less correctly) refers to a synthesizer that stores multiple waveforms, plays one waveform and repeats it, and allows the user to move a knob to change which waveform is being played.  Synthesizers of this nature, even the basic ones, are unimaginably powerful.  Microwave is one of them.<br>"
"<br>"
"Microwave's wavetables have 256 waveforms, at 2048 samples each.  The Morph (MPH) knob chooses which of the 256 waveforms in the wavetable to play.  It is important to note that Microwave does not have any wavetable loaded by default, so no sound will be heard currently.  Load a sound file as a wavetable now (click the folder button at the bottom).  If you play a note while moving the Morph knob, you'll notice the waveform that is playing morphing to create new timbres.<br>"
"<br>"
"Range (RNG) is a feature unique to Microwave.  It takes waveforms in the wavetable near the one chosen by the Morph one, and mixes those in with the main waveform (at a lower volume as you get further away from the main waveform).  For example, a Morph of 5 and a Range of 2 will give you a mix between waveform 5, waveform 4 at half volume, and waveform 6 at half volume.<br>"
"<br>"
"MDFY (Modify) and the dropdown box next to it (Modify Mode) are used to warp your wavetable realtime, using formulas I created myself.  Change the Modify Mode and move the Modify knob while playing a note.  Hear how each Modify Mode causes a drastically different change to the sound.  These are extremely useful for injecting more flavor and awesomeness into your sound.  Use all of them to learn what they can do.<br>"
"<br>"
"DET stands for Detune, which changes the pitch of that oscillator, in cents.  PHS stands for Phase, which simply phase shifts the oscillator, and RAND next to it is Phase Randomness, which sets the oscillator to a random phase with each note.<br>"
"<br>"
"The arrow button under the tab list can be clicked to expose more knobs.<br>"
"<br>"
"When the tempo (BPM) knob is set to anything other than 0, the pitch is decreased drastically (you'll probably want to mute it) so that it perfectly matches up with the set tempo when detune is set to 0.  If you need it at half speed, double speed, etc., just change its pitch by octaves (because increasing by an octave doubles the frequency).<br>"
"<br>"
"The Morph Max knob (MAX) just changes the maximum value of the Morph knob, for tweaking convenience.<br>"
"<br>"
"Microwave supports very advanced unison abillities.  Unison is when you clone the oscillator multiple times, and play them all at the same time, usually with slight differences applied to them.  The original sound as well as the clones are called \"voices\".  In the UNISON box, NUM chooses the number of voices.  DET detunes each voice slightly, a common unison feature.  MPH and MOD are special.  They change the Morph and Modify (respectively) values for each individual voice, which can create an amazing 3D sound.  It is important to note that every unison voice is calculated individually, so using large numbers of unison voices can be very detrimental to your CPU.<br>"
"<br>"
"Earlier I mentioned that Microwave's wavetables have 256 waveforms, at 2048 samples each.  This can be changed using the Sample Length knob.  This knob can be used for finetuning your wavetable if the loading was slightly inaccurate.  If you notice your waveform moving left/right too much in the visualizer as you morph through the wavetable, the Sample Length knob may be able to fix that.<br>"
"<br>"
"With most wavetable synthesizers, CPU would be a major concern.  Luckily, I have put an obscene amount of work into optimizing Microwave, so this should be much less of a problem.  Feel free to go crazy.<br>"
"<br>"
"Explanations on how to use this for modulation is explained in the Matrix Tab section.<br>"
"<br>"
"<br>"
"-=SUB TAB=-<br>"
"<br>"
"This tab behaves a lot like BitInvader, but is significantly more useful in the context of Microwave.  This tab is meant to be used for many things:<br>"
"1. Single-waveform oscillators/modulators<br>"
"2. LFOs<br>"
"3. Envelopes<br>"
"4. Step Sequencers<br>"
"5. Noise Generators<br>"
"<br>"
"In very early versions of Microwave, the five things listed above were all in their own tabs, and were later combined into one for obvious user-friendliness reasons.  I would like to quickly note here that I implore all of you to use Step Sequencers in Microwave all the time.  I wanted it to be one of the more highlighted features of Microwave, but never really had the chance.  Step Sequencers are an awesome and unique way to add rhythmic modulations to your sound.<br>"
"<br>"
"The LENGTH knob changes the length of the oscillator.  Decreasing this to a small number makes it very easy to use this as a Step Sequencer.  In some special cases you may also want to automate this knob for some interesting effects.<br>"
"<br>"
"There are four LEDs you can see at the bottom.  The top left is whether the oscillator is enabled.  The top right is \"Muted\", which is different.  When an oscillator is enabled but muted, it is still calculated and still used for modulation, but that oscillator's sound is never played.  You'll usually want this on when using this as an envelope/LFO/step sequencer.  The bottom left is keytracking.  When keytracking is disabled, the oscillator always plays at the same frequency regardless of the note you press.  You'll want to turn this off when you need your envelopes/LFOs/step sequencers to always go at the same speed.  The bottom right LED converts the oscillator into a noise generator, which generates a different flavor of noise depending on the graph you draw.<br>"
"<br>"
"The Rate Limit knob (RATE) controls how quickly the waveform can increase or decrease.  Combined with the noise generator, this could potentially be used as a chaos oscillator in the Matrix tab.<br>"
"<br>"
"Explanations on how to use this for modulation is explained in the Matrix Tab section.<br>"
"<br>"
"<br>"
"-=SAMPLE TAB=-<br>"
"<br>"
"This tab is used to import entire samples to use as oscillators.  This means you can frequency modulate your cowbells with your airhorns, which you can then phase modulate with an ogre yelling about his swamp, which you can then amplitude modulate with a full-length movie about bees.  Alternatively, you could just use it as a simple way to layer in a sound or some noise sample with the sound you have already created.  It is important to note that imported samples are stored within Microwave, which means two things:<br>"
"1. Unlike AudioFileProcessor, where the size of the sample does not impact the project file size, any samples in Microwave will be losslessly stored inside the project file and preset, which can make the project file extremely large.<br>"
"2. When sending your project file or Microwave preset to somebody else, they do not need to have the sample to open it, unlike with AudioFileProcessor.<br>"
"<br>"
"With that being said, Microwave's Sample Tab is not meant as a replacement to AudioFileProcessor.  Microwave will use more CPU, and some audio problems may show up when playing notes other than A4 (e.g. unstable pitch and stuff).  In most cases, if what you want can be done with AudioFileProcessor, you should use AudioFileProcessor.  Otherwise, totally use Microwave.  The Sample Tab is useful for many reasons, especially for its modulation capabilities and the weird way Microwave can morph samples depending on its graph waveform.<br>"
"<br>"
"The Sample Tab has two new knobs.  Those change the start and end position of the sample.<br>"
"<br>"
"There are two new LEDs for this tab, at the right.  The second to last one enables the graph.  The graph determines the order in which the sample is played.  A saw wave will play the sample normally, and a reverse wave wave will play it backward.  Draw a random squiggle on it and... well, you'll hear it.  Pretty much, left/right is time, up/down is position in the sample.  Note that this will almost definitely change the pitch in most circumstances, because changing a sound's speed also changes its pitch.  The last LED enables and disabled sample looping.<br>"
"<br>"
"<br>"
"-=MATRIX TAB=-<br>"
"<br>"
"This tab is used for a lot of things, ranging from modulation, effect routing, humanization, etc.  If you think it looks difficult, it's a lot easier than it looks.  If you think it looks easy, it's much more difficult than it looks.<br>"
"<br>"
"The matrix works by taking one or two inputs (whether it be from an oscillator, effect output, humanizer, or anything else) and outputting it somewhere (e.g. to control/modulate a knob, to an effect input, etc.).  It's fairly simple.<br>"
"<br>"
"Notice how there are three rows.  Only focus on the top two, as the top and bottom ones are functionally identical.  The top left dropdown box chooses the matrix input, and the LCD Spinboxes choose which input (e.g. which oscillator, which filter, etc.) to grab from.  The AMT knob chooses the Amount of the input that is fed into the output (e.g. input volume to effect, how much to move a knob by, etc.).  The CRV (Curve) knob gives that input a bias upward or downward, which can be used as a simple way to shape and sculpt your modulations in interesting ways.  The LED to the right of that converts the input from an LFO to an envelope, if applicable.  In other words, it ignores repetitions of the input oscillators.<br>"
"<br>"
"The middle left dropdown box sends which section to send the output to (e.g. which tab), and the dropdown box to the right of it is more specific (e.g. the Morph knob of the main tab, the third filter, etc.), as well as the LCD Spinbox following (for e.g. which oscillator to send it to).  The dropdown box to the right of that lets you choose between unidirectional and bidirectional modulation, as well as choosing how the two inputs are combined (e.g. add vs multiply).  <br>"
"<br>"
"It seems simple, but this section is one of the most important parts of Microwave.  Most synthesizers only have some combination of FM, PM, AM, and PWM modulation types.  Because of how Microwave's matrix tab is set up, allowing any parameter to be controlled by any oscillator at any speed, Microwave has those modulation types as well as over a hundred more.  Welcome to Choice Paralysis.  There will never be any point in time when the amount of choice you have will not overwhelm you.  Have fun with your freedom!<br>"
"<br>"
"<br>"
"-=EFFECT TAB=-<br>"
"(temporarily AKA Filter Tab)<br>"
"<br>"
"The current version of Microwave only has filters for effects, but that will be changed in future versions.<br>"
"<br>"
"FREQ is the filter's cutoff frequency, and RESO is the filter's resonance.  GAIN is the gain for peak filters, shelf filters, etc.  TYPE is the type of filter.  Microwave currently has lowpass, highpass, bandpass, peak, notch, low shelf, high shelf, allpass, and moog lowpass filters.  SLOPE runs the sound through the filter multiple times, changing the filter's slope at (usually) increments of 12 db.<br>"
"<br>"
"IN and OUT are volume knobs for the filter's input and output.  W/D is Wet/Dry.  PAN changes the Wet/Dry amount in individual ears, allowing you to use interesting panning filters.  SAT stands for Saturation, which allows you to add some simple distortion to your sound after it is filtered.<br>"
"<br>"
"FDBK (Feedback) stores the filter's output and sends it back to the filter's input after a certain amount of time.  This is a very odd, unique, interesting, and useful feature.  Without a filter in effect, increasing the feedback turns this into a comb filter.  Having a filter selected and working can create some ridiculous tibres you'd never hear out of most other synthesizers.  The change the feedback makes to the sound is very tonal, and the pitch it creates depends on its delay.  Because of this, I made it so the delay is keytracking by default, so the change it makes to your sound matches the note you play.  DET detunes that, and the keytracking button in the top right turns keytracking off for that.  Definitely have fun with this feature, you can get weird and amazing sounds out of this.  Notice that this feature entirely allows Karplus-Strong synthesis, as well as any other ridiculous adaptations that pop into your head.<br>"
"<br>"
"<br>"
"-=MISCELLANEOUS TAB=-<br>"
"<br>"
"The oversampling dropdown box is ***VERY*** important when it comes to Microwave's audio quality.  The higher it is, the cleaner your sound will be.  But, oversampling is also extremely detrimental to your CPU.  The multiplier of oversampling will be almost exactly the multiplier it applies to the processing power it uses.<br>"
"<br>"
"But how do you know whether you need oversampling?  2x should be appropriate in most (?) cases, but definitely not all of them.  If your sound is very basic, and all matrix inputs that control knobs only move very very slowly, then it's possible that as low as 1x oversampling (which means no oversampling) may be appropriate.  But, if the sound makes use of modulation, then higher values of oversampling may be appropriate, especially if the modulator contains or plays at high frequencies.  When in doubt, use your ears and compare.  Never neglect the oversampling.  If you're making a sound that uses modulation and it sounds a bit too much like a dying engine, bumping up the oversampling a bit may be all that is needed to bring the sound from okay to awesome.  I strongly suggest making oversampling tweaking a part of your habitual workflow when using Microwave.<br>"
"<br>"
"The oversampling mode has an \"Average\" option, which decreases audio artifacts but distorts the high frequencies slightly.  The default value for this is usually fine unless you hear some weird buzzing, or you're trying to get the sound super clean and the high frequency changes aren't a problem.<br>"
"<br>"
"The REMOVE DC OFFSET button removes 0 Hz content from your sound.  0 Hz sound is almost always a bad thing.  But of course, don't turn this on if you don't need it, since it does use a small amount of CPU.<br>"
"<br>"
"The eight Macro knobs provided are useful for quickly and conveniently changing/automating/modulating multiple knobs at the same time.  Use them as Matrix inputs and outputs.  It's simple but very useful, especially for presets.<br>"
"<br>"
"<br>"
"-=OTHER FEATURES TO BE AWARE OF=-<br>"
"<br>"
"Right clicking some knobs will reveal a new \"Send To Matrix\" option.  Choosing that option will send that knob to the Matrix output for you automatically, and bring you there to choose an input.<br>"
"<br>"
"If a knob is set as an output in the Matrix, middle click and dragging that knob (or, *alt*ernatively, using Alt and dragging) will control the Amount knob of that Matrix box rather than the knob itself.  You can tell it is working correctly if the knob turns the color green when you do so.  If the knob is set as an output in multiple places, the uppermost applicable Matrix box will be selected.  This is useful for fast workflow, e.g. if you want to control frequency and FM amount in the same spot.<br>"
"<br>"
"<br>"
"<br>"
"<br>"
"==CPU PRESERVATION GUIDELINES==<br>"
"<br>"
"<br>"
"Turn the wavetable tab's visualizer off.  That uses a lot of processing power.<br>"
"<br>"
"Microwave stores the highest oscillator that is enabled, and checks every oscillator from the first one to the highest enabled one to see whether it's enabled.  So, having just the 50th oscillator enabled will take significantly more processing power than having just the 1st one enabled, because it would need to check oscillators 1-50 to see whether they're enabled.  This also applies to filters and Matrix boxes.<br>"
"<br>"
"Increasing the Range knob will use a bit more CPU (since it needs to calculate nearby waveforms as well).  With very large Range values the CPU hit can get quite noticeable.  But, even though this needs to calculate nearby waveforms, it doesn't need to recalculate the entire oscillator, so increasing the Range won't use nearly as much CPU as, for example, increasing the number of unison voices.  <br>"
"<br>"
"Having a larger number of unison voices increases the CPU usage by around the voice amount.  For example, having 30 voices will use approximately 15x as much processing power as just two voices.  This increase is almost exactly linear (with the exception of using only one voice, which uses less than half the CPU as two voices, since having unison disabled entirely will prevent unison stuff from being calculated entirely).<br>"
"<br>"
"The values of the Morph and Modify knobs in the UNISON box have no impact on processing power needed, except for a very small performance gain when they are at exactly 0.<br>"
"<br>"
"Even when the modify is not in use, having the Modify Mode set to None will use (sometimes significantly) less CPU than if it is set to something else.<br>"
"<br>"
"When using modify, expect the largest CPU hit from modes that require accessing other parts of the waveform to work (e.g. Squarify and Pulsify).<br>"
"<br>"
"Oversampling results in an almost exact multiplication in processing power needed.  So 8x oversampling will use 4x more CPU than 2x oversampling.<br>"
"<br>"
"<br>"
"<br>"
"<br>"
"==FAQ==<br>"
"<br>"
"<br>"
"-=WHY WON'T MY MICROWAVE MAKE SOUND?!=-<br>"
"<br>"
"1. Wavetable tab: You need to enable the \"Enabled\" LED (has a power icon next to it), then click the folder button to load a wavetable.  After the wavetable is loaded correctly, move the Morph knob and you should hear sound.<br>"
"<br>"
"2. Sub tab: You need to enable the \"Enabled\" LED (has a power icon next to it), then draw whatever you want on the graph.<br>"
"<br>"
"3. Sample tab: You need to enable the \"Enabled\" LED (has a power icon next to it), then click the folder button to load a sample.<br>"
"<br>"
"<br>"
"-=WHY WON'T MY FILTERS WORK?!=-<br>"
"<br>"
"Make sure the filter is enabled.  Take the oscillator you want to send through your filter, enable its Muted LED, use it as an input in the Matrix, set the output as \"Filter Input\", and set the Matrix Amount knob to 100%.  This will prevent the original oscillator audio from being sent directly to the audio output, and send the oscillator audio to the filter.<br>"
"<br>"
;


MicrowaveManualView::MicrowaveManualView():QTextEdit( s_manualText )
{
	setWindowTitle ( "Microwave Manual" );
	setTextInteractionFlags ( Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse );
	gui->mainWindow()->addWindowedWidget( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->setWindowIcon( PLUGIN_NAME::getIconPixmap( "logo" ) );
	parentWidget()->resize( 640, 480 );
}


void MicrowaveView::manualBtnClicked()
{
	MicrowaveManualView::getInstance()->hide();
	MicrowaveManualView::getInstance()->show();
}



void MicrowaveKnob::setMatrixLocation( int loc1, int loc2, int loc3 )
{
	matrixLocation[0] = loc1;
	matrixLocation[1] = loc2;
	matrixLocation[2] = loc3;

	disconnect(this, &MicrowaveKnob::sendToMatrixAsOutput, 0, 0);
	connect( this, &MicrowaveKnob::sendToMatrixAsOutput, this, [this, loc1, loc2, loc3]() { knobView->sendToMatrixAsOutput( loc1, loc2, loc3 ); } );

	disconnect(this, &MicrowaveKnob::switchToMatrixKnob, 0, 0);
	connect( this, &MicrowaveKnob::switchToMatrixKnob, this, [this, loc1, loc2, loc3]() { knobView->switchToMatrixKnob( this, loc1, loc2, loc3 ); } );
}


void MicrowaveKnob::setWhichMacroKnob( int which )
{
	this->whichMacroKnob = which;

	disconnect(this, &MicrowaveKnob::setMacroTooltip, 0, 0);
	disconnect(this, &MicrowaveKnob::chooseMacroColor, 0, 0);
	disconnect(this, &MicrowaveKnob::refreshMacroColor, 0, 0);
	disconnect(this, &MicrowaveKnob::setMacroColortoDefault, 0, 0);

	if( which != -1 )
	{
		connect( this, &MicrowaveKnob::setMacroTooltip, this, [this, which]() { knobView->setMacroTooltip( this, which ); } );
		connect( this, &MicrowaveKnob::chooseMacroColor, this, [this, which]() { knobView->chooseMacroColor( this, which ); } );
		connect( this, &MicrowaveKnob::refreshMacroColor, this, [this, which]() { knobView->refreshMacroColor( this, which ); } );
		connect( this, &MicrowaveKnob::setMacroColortoDefault, this, [this, which]() { knobView->setMacroColortoDefault( this, which ); } );
	}
}



void MicrowaveKnob::contextMenuEvent( QContextMenuEvent * )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	CaptionMenu contextMenu( model()->displayName(), this );
	addDefaultActions( &contextMenu );
	contextMenu.addAction( QPixmap(), model()->isScaleLogarithmic() ? tr( "Set linear" ) : tr( "Set logarithmic" ), this, SLOT( toggleScale() ) );
	contextMenu.addSeparator();
	if( this->matrixLocation[0] )
	{
		contextMenu.addAction( PLUGIN_NAME::getIconPixmap( "tab4" ), tr( "Control this in Matrix" ), this, SIGNAL( sendToMatrixAsOutput() ) );
	}
	if( this->whichMacroKnob != -1 )
	{
		contextMenu.addAction( PLUGIN_NAME::getIconPixmap( "tab4" ), tr( "Set Macro Tooltip" ), this, SIGNAL( setMacroTooltip() ) );
		contextMenu.addAction( embed::getIconPixmap( "colorize" ), tr( "Set Knob Color" ), this, SIGNAL( chooseMacroColor() ) );
		contextMenu.addAction( embed::getIconPixmap( "colorize" ), tr( "Set Knob Color To Default" ), this, SIGNAL( setMacroColortoDefault() ) );
	}
	contextMenu.addSeparator();
	contextMenu.exec( QCursor::pos() );
}


void MicrowaveKnob::mousePressEvent( QMouseEvent * _me )
{
	if( ( _me->button() == Qt::LeftButton && gui->mainWindow()->isAltPressed() ) || _me->button() == Qt::MidButton )
	{
		if( matrixLocation[0] )
		{
			switchToMatrixKnob();
		}

		AutomatableModel *thisModel = model();
		if( thisModel )
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState( false );
		}

		const QPoint & p = _me->pos();
		m_origMousePos = p;
		m_mouseOffset = QPoint(0, 0);
		m_leftOver = 0.0f;

		emit sliderPressed();

		QApplication::setOverrideCursor( Qt::BlankCursor );
		s_textFloat->setText( displayValue() );
		s_textFloat->moveGlobal( this, QPoint( width() + 2, 0 ) );
		s_textFloat->show();
		m_buttonPressed = true;
	}
	else
	{
		Knob::mousePressEvent( _me );
	}
}


void MicrowaveKnob::mouseReleaseEvent( QMouseEvent * _me )
{
	Knob::mouseReleaseEvent( _me );
	updateScroll();

	if( this->whichMacroKnob == -1 )
	{
		this->setarcColor( QColor(46,74,80) );
		this->setlineColor( QColor(102,198,199) );
		this->setInnerColor( QColor(64,92,97) );
	}
	else
	{
		refreshMacroColor();
	}
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new Microwave( static_cast<InstrumentTrack *>( m ) ) );
}


}

