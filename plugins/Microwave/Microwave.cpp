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
#include "TextFloat.h"
#include "ToolTip.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT microwave_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"Microwave",
	QT_TRANSLATE_NOOP("pluginBrowser",
				"Versatile modular wavetable synthesizer"),
	"Lost Robot",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader("logo"),
	NULL,
	NULL
} ;

}



//===============//
//== MICROWAVE ==//
//===============//

Microwave::Microwave(InstrumentTrack * instrument_track) :
	Instrument(instrument_track, &microwave_plugin_descriptor),
	m_visvol(100, 0, 1000, 0.01f, this, tr("Visualizer Volume")),
	m_loadChnl(0, 0, 1, 1, this, tr("Wavetable Loading Channel")),
	m_mainNum(1, 1, 8, this, tr("Wavetable Oscillator Number")),
	m_subNum(1, 1, 64, this, tr("Sub Oscillator Number")),
	m_sampNum(1, 1, 8, this, tr("Sample Number")),
	m_oversample(this, tr("Oversampling")),
	m_oversampleMode(this, tr("Interpolation")),
	m_loadMode(this, tr("Wavetable Loading Algorithm")),
	m_graph(-1.0f, 1.0f, 204, this),
	m_visualize(false, this)
{
	for (int i = 0; i < 8; ++i)
	{
		m_storedwaveforms[i].reserve(STOREDMAINARRAYLEN);
		m_waveforms[i].reserve(MAINARRAYLEN);

		m_morph[i] = new FloatModel(0, 0, 254, 0.0001f, this, tr("Morph"));
		m_range[i] = new FloatModel(1, 1, 16, 0.0001f, this, tr("Range"));
		m_sampLen[i] = new FloatModel(2048, 1, 16384, 1.f, this, tr("Waveform Sample Length"));
		m_morphMax[i] = new FloatModel(255, 0, 254, 0.0001f, this, tr("Morph Max"));
		m_modify[i] = new FloatModel(0, 0, 2048, 0.0001f, this, tr("Wavetable Modifier Value"));
		m_modifyMode[i] = new ComboBoxModel(this, tr("Wavetable Modifier Mode"));
		m_unisonVoices[i] = new FloatModel(1, 1, 32, 1, this, tr("Unison Voices"));
		m_unisonDetune[i] = new FloatModel(0, 0, 2000, 0.0001f, this, tr("Unison Detune"));
		m_unisonDetune[i]->setScaleLogarithmic(true);
		m_unisonMorph[i] = new FloatModel(0, 0, 256, 0.0001f, this, tr("Unison Morph"));
		m_unisonModify[i] = new FloatModel(0, 0, 2048, 0.0001f, this, tr("Unison Modify"));
		m_detune[i] = new FloatModel(0, -9600, 9600, 0.0001f, this, tr("Detune"));
		m_phase[i] = new FloatModel(0, 0, 200, 0.0001f, this, tr("Phase"));
		m_phaseRand[i] = new FloatModel(100, 0, 100, 0.0001f, this, tr("Phase Randomness"));
		m_vol[i] = new FloatModel(100.f, 0, 200.f, 0.0001f, this, tr("Volume"));
		m_enabled[i] = new BoolModel(false, this);
		m_muted[i] = new BoolModel(false, this);
		m_pan[i] = new FloatModel(0.f, -100.f, 100.f, 0.0001f, this, tr("Panning"));
		m_keytracking[i] = new BoolModel(true, this);
		m_tempo[i] = new FloatModel(0.f, 0.f, 999.f, 1.f, this, tr("Tempo"));
		m_interpolate[i] = new BoolModel(true, this);
		setwavemodel(m_modifyMode[i])

		m_filtInVol[i] = new FloatModel(100, 0, 200, 0.0001f, this, tr("Input Volume"));
		m_filtType[i] = new ComboBoxModel(this, tr("Filter Type"));
		m_filtSlope[i] = new ComboBoxModel(this, tr("Filter Slope"));
		m_filtCutoff[i] = new FloatModel(2000, 20, 20000, 0.0001f, this, tr("Cutoff Frequency"));
		m_filtCutoff[i]->setScaleLogarithmic(true);
		m_filtReso[i] = new FloatModel(0.707, 0, 16, 0.0001f, this, tr("Resonance"));
		m_filtReso[i]->setScaleLogarithmic(true);
		m_filtGain[i] = new FloatModel(0, -64, 64, 0.0001f, this, tr("dbGain"));
		m_filtGain[i]->setScaleLogarithmic(true);
		m_filtSatu[i] = new FloatModel(0, 0, 100, 0.0001f, this, tr("Saturation"));
		m_filtWetDry[i] = new FloatModel(100, 0, 100, 0.0001f, this, tr("Wet/Dry"));
		m_filtBal[i] = new FloatModel(0, -100, 100, 0.0001f, this, tr("Balance/Panning"));
		m_filtOutVol[i] = new FloatModel(100, 0, 200, 0.0001f, this, tr("Output Volume"));
		m_filtEnabled[i] = new BoolModel(false, this);
		m_filtFeedback[i] = new FloatModel(0, -100, 100, 0.0001f, this, tr("Feedback"));
		m_filtDetune[i] = new FloatModel(0, -9600, 9600, 0.0001f, this, tr("Detune"));
		m_filtKeytracking[i] = new BoolModel(true, this);
		m_filtMuted[i] = new BoolModel(false, this);
		filtertypesmodel(m_filtType[i])
		filterslopesmodel(m_filtSlope[i])

		m_sampleEnabled[i] = new BoolModel(false, this);
		m_sampleGraphEnabled[i] = new BoolModel(false, this);
		m_sampleMuted[i] = new BoolModel(false, this);
		m_sampleKeytracking[i] = new BoolModel(true, this);
		m_sampleLoop[i] = new BoolModel(true, this);

		m_sampleVolume[i] = new FloatModel(100, 0, 200, 0.0001f, this, tr("Volume"));
		m_samplePanning[i] = new FloatModel(0, -100, 100, 0.0001f, this, tr("Panning"));
		m_sampleDetune[i] = new FloatModel(0, -9600, 9600, 0.0001f, this, tr("Detune"));
		m_samplePhase[i] = new FloatModel(0, 0, 200, 0.0001f, this, tr("Phase"));
		m_samplePhaseRand[i] = new FloatModel(0, 0, 100, 0.0001f, this, tr("Phase Randomness"));
		m_sampleStart[i] = new FloatModel(0, 0, 1, 0.0001f, this, tr("Start"));
		m_sampleEnd[i] = new FloatModel(1, 0, 1, 0.0001f, this, tr("End"));

		m_keytrackingArr[i] = true;
		m_interpolateArr[i] = true;
	}

	for (int i = 0; i < 18; ++i)
	{
		m_macro[i] = new FloatModel(0, -100, 100, 0.0001f, this, tr("Macro"));
	}

	for (int i = 0; i < 64; ++i)
 	{
		m_storedsubs[i].reserve(STOREDSUBWAVELEN);
		m_subs[i].reserve(SUBWAVELEN);

		m_subEnabled[i] = new BoolModel(false, this);
		m_subVol[i] = new FloatModel(100.f, 0.f, 200.f, 0.0001f, this, tr("Volume"));
		m_subPhase[i] = new FloatModel(0.f, 0.f, 200.f, 0.0001f, this, tr("Phase"));
		m_subPhaseRand[i] = new FloatModel(0.f, 0.f, 100.f, 0.0001f, this, tr("Phase Randomness"));
		m_subDetune[i] = new FloatModel(0.f, -9600.f, 9600.f, 0.0001f, this, tr("Detune"));
		m_subMuted[i] = new BoolModel(true, this);
		m_subKeytrack[i] = new BoolModel(true, this);
		m_subSampLen[i] = new FloatModel(STOREDSUBWAVELEN, 1.f, STOREDSUBWAVELEN, 1.f, this, tr("Sample Length"));
		m_subNoise[i] = new BoolModel(false, this);
		m_subPanning[i] = new FloatModel(0.f, -100.f, 100.f, 0.0001f, this, tr("Panning"));
		m_subTempo[i] = new FloatModel(0.f, 0.f, 999.f, 1.f, this, tr("Tempo"));
		m_subRateLimit[i] = new FloatModel(0.f, 0.f, 1.f, 0.000001f, this, tr("Rate Limit"));
		m_subRateLimit[i]->setScaleLogarithmic(true);
		m_subUnisonNum[i] = new FloatModel(1.f, 1.f, 32.f, 1.f, this, tr("Unison Voice Number"));
		m_subUnisonDetune[i] = new FloatModel(0.f, 0.f, 2000.f, 0.0001f, this, tr("Unison Detune"));
		m_subInterpolate[i] = new BoolModel(true, this);

		m_modEnabled[i] = new BoolModel(false, this);

		m_modOutSec[i] = new ComboBoxModel(this, tr("Modulation Section"));
		m_modOutSig[i] = new ComboBoxModel(this, tr("Modulation Signal"));
		m_modOutSecNum[i] = new IntModel(1, 1, 8, this, tr("Modulation Section Number"));
		modsectionsmodel(m_modOutSec[i])
		mainoscsignalsmodel(m_modOutSig[i])

		m_modIn[i] = new ComboBoxModel(this, tr("Modulator"));
		m_modInNum[i] = new IntModel(1, 1, 8, this, tr("Modulator Number"));
		modinmodel(m_modIn[i])

		m_modInAmnt[i] = new FloatModel(0, -200, 200, 0.0001f, this, tr("Modulator Amount"));
		m_modInCurve[i] = new FloatModel(100, 10.f, 600, 0.0001f, this, tr("Modulator Curve"));
		m_modInCurve[i]->setScaleLogarithmic(true);

		m_modIn2[i] = new ComboBoxModel(this, tr("Secondary Modulator"));
		m_modInNum2[i] = new IntModel(1, 1, 8, this, tr("Secondary Modulator Number"));
		modinmodel(m_modIn2[i])

		m_modInAmnt2[i] = new FloatModel(0, -200, 200, 0.0001f, this, tr("Secondary Modulator Amount"));
		m_modInCurve2[i] = new FloatModel(100, 10.f, 600, 0.0001f, this, tr("Secondary Modulator Curve"));
		m_modInCurve2[i]->setScaleLogarithmic(true);

		m_modCombineType[i] = new ComboBoxModel(this, tr("Combination Type"));
		modcombinetypemodel(m_modCombineType[i])

		m_modType[i] = new BoolModel(false, this);
		m_modType2[i] = new BoolModel(false, this);
	}

	oversamplemodel(m_oversample)
	m_oversample.setValue(1);// 2x oversampling is default

	loadmodemodel(m_loadMode)

	oversamplemodemodel(m_oversampleMode)
	m_oversampleMode.setValue(0);// Decimate is default
	/* Decimate mode downsamples without interpolation,
	which actually has decent quality because of
	Microwave's non-realtime oversampling.

	Average mode averages all the generated samples together,
	which can sometimes result in fewer artifacts but oftentimes
	messes up high frequencies.*/

	connect(&m_graph, SIGNAL(samplesChanged(int, int)), this, SLOT(samplesChanged(int, int)));

	for (int i = 0; i < 8; ++i)
	{
		connect(m_morph[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(1, i); }, Qt::DirectConnection);
		connect(m_range[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(2, i); }, Qt::DirectConnection);
		connect(m_modify[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(3, i); }, Qt::DirectConnection);
		connect(m_modifyMode[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(4, i); }, Qt::DirectConnection);
		connect(m_vol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(5, i); }, Qt::DirectConnection);
		connect(m_pan[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(6, i); }, Qt::DirectConnection);
		connect(m_detune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(7, i); }, Qt::DirectConnection);
		connect(m_phase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(8, i); }, Qt::DirectConnection);
		connect(m_phaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(9, i); }, Qt::DirectConnection);
		connect(m_enabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(10, i); }, Qt::DirectConnection);
		connect(m_muted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(11, i); }, Qt::DirectConnection);
		connect(m_sampLen[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(12, i); }, Qt::DirectConnection);
		connect(m_morphMax[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(13, i); }, Qt::DirectConnection);
		connect(m_unisonVoices[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(14, i); }, Qt::DirectConnection);
		connect(m_unisonDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(15, i); }, Qt::DirectConnection);
		connect(m_unisonMorph[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(16, i); }, Qt::DirectConnection);
		connect(m_unisonModify[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(17, i); }, Qt::DirectConnection);
		connect(m_keytracking[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(18, i); }, Qt::DirectConnection);
		connect(m_tempo[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(19, i); }, Qt::DirectConnection);
		connect(m_interpolate[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(20, i); }, Qt::DirectConnection);

		connect(m_sampleEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(60, i); }, Qt::DirectConnection);
		connect(m_sampleMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(61, i); }, Qt::DirectConnection);
		connect(m_sampleKeytracking[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(62, i); }, Qt::DirectConnection);
		connect(m_sampleGraphEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(63, i); }, Qt::DirectConnection);
		connect(m_sampleLoop[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(64, i); }, Qt::DirectConnection);
		connect(m_sampleVolume[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(65, i); }, Qt::DirectConnection);
		connect(m_samplePanning[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(66, i); }, Qt::DirectConnection);
		connect(m_sampleDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(67, i); }, Qt::DirectConnection);
		connect(m_samplePhase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(68, i); }, Qt::DirectConnection);
		connect(m_samplePhaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(69, i); }, Qt::DirectConnection);
		connect(m_sampleStart[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(70, i); }, Qt::DirectConnection);
		connect(m_sampleEnd[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(71, i); }, Qt::DirectConnection);

		connect(m_filtCutoff[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(120, i); }, Qt::DirectConnection);
		connect(m_filtReso[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(121, i); }, Qt::DirectConnection);
		connect(m_filtGain[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(122, i); }, Qt::DirectConnection);
		connect(m_filtType[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(123, i); }, Qt::DirectConnection);
		connect(m_filtSlope[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(124, i); }, Qt::DirectConnection);
		connect(m_filtInVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(125, i); }, Qt::DirectConnection);
		connect(m_filtOutVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(126, i); }, Qt::DirectConnection);
		connect(m_filtWetDry[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(127, i); }, Qt::DirectConnection);
		connect(m_filtBal[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(128, i); }, Qt::DirectConnection);
		connect(m_filtSatu[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(129, i); }, Qt::DirectConnection);
		connect(m_filtFeedback[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(130, i); }, Qt::DirectConnection);
		connect(m_filtDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(131, i); }, Qt::DirectConnection);
		connect(m_filtEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(132, i); }, Qt::DirectConnection);
		connect(m_filtMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(133, i); }, Qt::DirectConnection);
		connect(m_filtKeytracking[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(134, i); }, Qt::DirectConnection);

		for (int j = 1; j <= 20; ++j)
		{
			valueChanged(j, i);
		}

		for (int j = 60; j <= 71; ++j)
		{
			valueChanged(j, i);
		}

		for (int j = 120; j <= 134; ++j)
		{
			valueChanged(j, i);
		}

		connect(m_sampleEnabled[i], &BoolModel::dataChanged, this, [this, i]() { sampleEnabledChanged(i); }, Qt::DirectConnection);
		connect(m_enabled[i], &BoolModel::dataChanged, this, [this, i]() { mainEnabledChanged(i); }, Qt::DirectConnection);
		connect(m_filtEnabled[i], &BoolModel::dataChanged, this, [this, i]() { filtEnabledChanged(i); }, Qt::DirectConnection);
		connect(m_sampLen[i], &FloatModel::dataChanged, this, [this, i]() { sampLenChanged(i); }, Qt::DirectConnection);
		connect(m_interpolate[i], &BoolModel::dataChanged, this, [this, i]() { interpolateChanged(i); });
		connect(m_morphMax[i], &FloatModel::dataChanged, this, [this, i]() { morphMaxChanged(i); }, Qt::DirectConnection);
	}

	for (int i = 0; i < 18; ++i)
	{
		connect(m_macro[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(150, i); }, Qt::DirectConnection);

		valueChanged(150, i);

		// Set default macro knob colors
		m_macroColors[i][0] = 102;
		m_macroColors[i][1] = 198;
		m_macroColors[i][2] = 199;
	}

	for (int i = 0; i < 64; ++i)
	{
		connect(m_subEnabled[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(30, i); }, Qt::DirectConnection);
		connect(m_subMuted[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(31, i); }, Qt::DirectConnection);
		connect(m_subKeytrack[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(32, i); }, Qt::DirectConnection);
		connect(m_subNoise[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(33, i); }, Qt::DirectConnection);
		connect(m_subVol[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(34, i); }, Qt::DirectConnection);
		connect(m_subPanning[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(35, i); }, Qt::DirectConnection);
		connect(m_subDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(36, i); }, Qt::DirectConnection);
		connect(m_subPhase[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(37, i); }, Qt::DirectConnection);
		connect(m_subPhaseRand[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(38, i); }, Qt::DirectConnection);
		connect(m_subSampLen[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(39, i); }, Qt::DirectConnection);
		connect(m_subTempo[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(40, i); }, Qt::DirectConnection);
		connect(m_subRateLimit[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(41, i); }, Qt::DirectConnection);
		connect(m_subUnisonNum[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(42, i); }, Qt::DirectConnection);
		connect(m_subUnisonDetune[i], &FloatModel::dataChanged, this, [this, i]() { valueChanged(43, i); }, Qt::DirectConnection);
		connect(m_subInterpolate[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(44, i); }, Qt::DirectConnection);

		connect(m_modIn[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(90, i); }, Qt::DirectConnection);
		connect(m_modInNum[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(91, i); }, Qt::DirectConnection);
		connect(m_modInAmnt[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(92, i); }, Qt::DirectConnection);
		connect(m_modInCurve[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(93, i); }, Qt::DirectConnection);
		connect(m_modIn2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(94, i); }, Qt::DirectConnection);
		connect(m_modInNum2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(95, i); }, Qt::DirectConnection);
		connect(m_modInAmnt2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(96, i); }, Qt::DirectConnection);
		connect(m_modInCurve2[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(97, i); }, Qt::DirectConnection);
		connect(m_modOutSec[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(98, i); }, Qt::DirectConnection);
		connect(m_modOutSig[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(99, i); }, Qt::DirectConnection);
		connect(m_modOutSecNum[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(100, i); }, Qt::DirectConnection);
		connect(m_modEnabled[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(101, i); }, Qt::DirectConnection);
		connect(m_modCombineType[i], &ComboBoxModel::dataChanged, this, [this, i]() { valueChanged(102, i); }, Qt::DirectConnection);
		connect(m_modType[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(103, i); }, Qt::DirectConnection);
		connect(m_modType2[i], &BoolModel::dataChanged, this, [this, i]() { valueChanged(104, i); }, Qt::DirectConnection);

		for (int j = 30; j <= 44; ++j)
		{
			valueChanged(j, i);
		}

		for (int j = 90; j <= 104; ++j)
		{
			valueChanged(j, i);
		}

		connect(m_modEnabled[i], &BoolModel::dataChanged, this, [this, i]() { modEnabledChanged(i); }, Qt::DirectConnection);
		connect(m_subSampLen[i], &FloatModel::dataChanged, this, [this, i]() { subSampLenChanged(i); });
		connect(m_subEnabled[i], &BoolModel::dataChanged, this, [this, i]() { subEnabledChanged(i); });
		connect(m_subInterpolate[i], &BoolModel::dataChanged, this, [this, i]() { subInterpolateChanged(i); });
	}

	for (int i = 0; i < 8; ++i)
	{
		// Make sure Sample Tab samples aren't empty, to prevent a crash.
		m_samples[i][0].push_back(0);
		m_samples[i][1].push_back(0);
	}
}


Microwave::~Microwave()
{
	for (int i = 0; i < 64; ++i)
	{
		/*The following disconnected functions will run if not disconnected upon deletion,
		because deleting a ComboBox includes clearing its contents first,
		which will fire a dataChanged event.  So, we need to disconnect them to
		prevent a crash.*/
		disconnect(m_modIn[i], &ComboBoxModel::dataChanged, 0, 0);
		disconnect(m_modIn2[i], &ComboBoxModel::dataChanged, 0, 0);
		disconnect(m_modOutSec[i], &ComboBoxModel::dataChanged, 0, 0);
	}
}


PluginView * Microwave::instantiateView(QWidget * parent)
{
	return new MicrowaveView(this, parent);
}


QString Microwave::nodeName() const
{
	return microwave_plugin_descriptor.name;
}


void Microwave::saveSettings(QDomDocument & doc, QDomElement & thissave)
{

	// NOTE: Only m_enabled oscillators/sections are saved.
	//This is to prevent ridiculously long project save times, as well as total disk space annihilation.

	// Save plugin version
	thissave.setAttribute("version", "Microwave Official Release 1");

	m_visvol.saveSettings(doc, thissave, "visualizer_volume");
	m_loadMode.saveSettings(doc, thissave, "loadingalgorithm");
	m_loadChnl.saveSettings(doc, thissave, "loadingchannel");
	m_oversample.saveSettings(doc, thissave, "oversample");
	m_oversampleMode.saveSettings(doc, thissave, "oversamplemode");
	m_removeDC.saveSettings(doc, thissave, "removeDC");

	QString saveString;

	// Save wavetables
	for (int i = 0; i < 8; ++i)
	{
		if (m_enabled[i]->value())
		{
			if (m_updateWavetable[i])
			{
				m_updateWavetable[i] = false;
				base64::encode((const char *)m_storedwaveforms[i].data(),
					STOREDMAINARRAYLEN * sizeof(float), m_wavetableSaveStrings[i]);
			}
			thissave.setAttribute("waveforms"+QString::number(i), m_wavetableSaveStrings[i]);
		}
	}

	// Save sub oscillator waveforms
	for (int i = 0; i < 64; ++i)
	{
		if (m_subEnabled[i]->value())
		{
			base64::encode((const char *)m_storedsubs[i].data(),
				STOREDSUBWAVELEN * sizeof(float), saveString);
			thissave.setAttribute("subs"+QString::number(i), saveString);
		}
	}

	// Save graph in Sample Tab
	base64::encode((const char *)m_sampGraphs,
		1024 * sizeof(float), saveString);
	thissave.setAttribute("sampGraphs", saveString);

	// Save samples
	int sampleSizes[8] = {0};
	for (int i = 0; i < 8; ++i)
	{
		if (m_sampleEnabled[i]->value())
		{
			for (int j = 0; j < 2; ++j)
			{
				base64::encode((const char *)m_samples[i][j].data(),
					m_samples[i][j].size() * sizeof(float), saveString);
				thissave.setAttribute("samples_"+QString::number(i)+"_"+QString::number(j), saveString);
			}

			sampleSizes[i] = m_samples[i][0].size();
		}
	}

	// Save sample lengths (required for loading samples)
	base64::encode((const char *)sampleSizes,
		8 * sizeof(int), saveString);
	thissave.setAttribute("sampleSizes", saveString);

	// Save all other values
	for (int i = 0; i < m_maxMainEnabled; ++i)
	{
		if (m_enabled[i]->value())
		{
			m_morph[i]->saveSettings(doc, thissave, "morph_"+QString::number(i));
			m_range[i]->saveSettings(doc, thissave, "range_"+QString::number(i));
			m_modify[i]->saveSettings(doc, thissave, "modify_"+QString::number(i));
			m_modifyMode[i]->saveSettings(doc, thissave, "modifyMode_"+QString::number(i));
			m_unisonVoices[i]->saveSettings(doc, thissave, "unisonVoices_"+QString::number(i));
			m_unisonDetune[i]->saveSettings(doc, thissave, "unisonDetune_"+QString::number(i));
			m_unisonMorph[i]->saveSettings(doc, thissave, "unisonMorph_"+QString::number(i));
			m_unisonModify[i]->saveSettings(doc, thissave, "unisonModify_"+QString::number(i));
			m_morphMax[i]->saveSettings(doc, thissave, "morphMax_"+QString::number(i));
			m_detune[i]->saveSettings(doc, thissave, "detune_"+QString::number(i));
			m_sampLen[i]->saveSettings(doc, thissave, "sampLen_"+QString::number(i));
			m_phase[i]->saveSettings(doc, thissave, "phase_"+QString::number(i));
			m_phaseRand[i]->saveSettings(doc, thissave, "phaseRand_"+QString::number(i));
			m_vol[i]->saveSettings(doc, thissave, "vol_"+QString::number(i));
			m_enabled[i]->saveSettings(doc, thissave, "enabled_"+QString::number(i));
			m_muted[i]->saveSettings(doc, thissave, "muted_"+QString::number(i));
			m_pan[i]->saveSettings(doc, thissave, "pan_"+QString::number(i));
			m_keytracking[i]->saveSettings(doc, thissave, "keytracking_"+QString::number(i));
			m_tempo[i]->saveSettings(doc, thissave, "tempo_"+QString::number(i));
			m_interpolate[i]->saveSettings(doc, thissave, "interpolate_"+QString::number(i));
		}
	}

	for (int i = 0; i < m_maxSampleEnabled; ++i)
	{
		if (m_sampleEnabled[i]->value())
		{
			m_sampleEnabled[i]->saveSettings(doc, thissave, "sampleEnabled_"+QString::number(i));
			m_sampleGraphEnabled[i]->saveSettings(doc, thissave, "sampleGraphEnabled_"+QString::number(i));
			m_sampleMuted[i]->saveSettings(doc, thissave, "sampleMuted_"+QString::number(i));
			m_sampleKeytracking[i]->saveSettings(doc, thissave, "sampleKeytracking_"+QString::number(i));
			m_sampleLoop[i]->saveSettings(doc, thissave, "sampleLoop_"+QString::number(i));
			m_sampleVolume[i]->saveSettings(doc, thissave, "sampleVolume_"+QString::number(i));
			m_samplePanning[i]->saveSettings(doc, thissave, "samplePanning_"+QString::number(i));
			m_sampleDetune[i]->saveSettings(doc, thissave, "sampleDetune_"+QString::number(i));
			m_samplePhase[i]->saveSettings(doc, thissave, "samplePhase_"+QString::number(i));
			m_samplePhaseRand[i]->saveSettings(doc, thissave, "samplePhaseRand_"+QString::number(i));
			m_sampleStart[i]->saveSettings(doc, thissave, "sampleStart_"+QString::number(i));
			m_sampleEnd[i]->saveSettings(doc, thissave, "sampleEnd_"+QString::number(i));
		}
	}

	for (int i = 0; i < m_maxFiltEnabled; ++i)
	{
		if (m_filtEnabled[i]->value())
		{
			m_filtInVol[i]->saveSettings(doc, thissave, "filtInVol_"+QString::number(i));
			m_filtType[i]->saveSettings(doc, thissave, "filtType_"+QString::number(i));
			m_filtSlope[i]->saveSettings(doc, thissave, "filtSlope_"+QString::number(i));
			m_filtCutoff[i]->saveSettings(doc, thissave, "filtCutoff_"+QString::number(i));
			m_filtReso[i]->saveSettings(doc, thissave, "filtReso_"+QString::number(i));
			m_filtGain[i]->saveSettings(doc, thissave, "filtGain_"+QString::number(i));
			m_filtSatu[i]->saveSettings(doc, thissave, "filtSatu_"+QString::number(i));
			m_filtWetDry[i]->saveSettings(doc, thissave, "filtWetDry_"+QString::number(i));
			m_filtBal[i]->saveSettings(doc, thissave, "filtBal_"+QString::number(i));
			m_filtOutVol[i]->saveSettings(doc, thissave, "filtOutVol_"+QString::number(i));
			m_filtEnabled[i]->saveSettings(doc, thissave, "filtEnabled_"+QString::number(i));
			m_filtFeedback[i]->saveSettings(doc, thissave, "filtFeedback_"+QString::number(i));
			m_filtDetune[i]->saveSettings(doc, thissave, "filtDetune_"+QString::number(i));
			m_filtKeytracking[i]->saveSettings(doc, thissave, "filtKeytracking_"+QString::number(i));
			m_filtMuted[i]->saveSettings(doc, thissave, "filtMuted_"+QString::number(i));
		}
	}

	for (int i = 0; i < m_maxSubEnabled; ++i)
	{
		if (m_subEnabled[i]->value())
		{
			m_subEnabled[i]->saveSettings(doc, thissave, "subEnabled_"+QString::number(i));
			m_subVol[i]->saveSettings(doc, thissave, "subVol_"+QString::number(i));
			m_subPhase[i]->saveSettings(doc, thissave, "subPhase_"+QString::number(i));
			m_subPhaseRand[i]->saveSettings(doc, thissave, "subPhaseRand_"+QString::number(i));
			m_subDetune[i]->saveSettings(doc, thissave, "subDetune_"+QString::number(i));
			m_subMuted[i]->saveSettings(doc, thissave, "subMuted_"+QString::number(i));
			m_subKeytrack[i]->saveSettings(doc, thissave, "subKeytrack_"+QString::number(i));
			m_subSampLen[i]->saveSettings(doc, thissave, "subSampLen_"+QString::number(i));
			m_subNoise[i]->saveSettings(doc, thissave, "subNoise_"+QString::number(i));
			m_subPanning[i]->saveSettings(doc, thissave, "subPanning_"+QString::number(i));
			m_subTempo[i]->saveSettings(doc, thissave, "subTempo_"+QString::number(i));
			m_subRateLimit[i]->saveSettings(doc, thissave, "subRateLimit_"+QString::number(i));
			m_subUnisonNum[i]->saveSettings(doc, thissave, "subUnisonNum_"+QString::number(i));
			m_subUnisonDetune[i]->saveSettings(doc, thissave, "subUnisonDetune_"+QString::number(i));
			m_subInterpolate[i]->saveSettings(doc, thissave, "subInterpolate_"+QString::number(i));
		}
	}

	for (int i = 0; i < m_maxModEnabled; ++i)
	{
		if (m_modEnabled[i]->value())
		{
			m_modIn[i]->saveSettings(doc, thissave, "modIn_"+QString::number(i));
			m_modInNum[i]->saveSettings(doc, thissave, "modInNu"+QString::number(i));
			m_modInAmnt[i]->saveSettings(doc, thissave, "modInAmnt_"+QString::number(i));
			m_modInCurve[i]->saveSettings(doc, thissave, "modInCurve_"+QString::number(i));
			m_modIn2[i]->saveSettings(doc, thissave, "modIn2_"+QString::number(i));
			m_modInNum2[i]->saveSettings(doc, thissave, "modInNum2_"+QString::number(i));
			m_modInAmnt2[i]->saveSettings(doc, thissave, "modAmnt2_"+QString::number(i));
			m_modInCurve2[i]->saveSettings(doc, thissave, "modCurve2_"+QString::number(i));
			m_modOutSec[i]->saveSettings(doc, thissave, "modOutSec_"+QString::number(i));
			m_modOutSig[i]->saveSettings(doc, thissave, "modOutSig_"+QString::number(i));
			m_modOutSecNum[i]->saveSettings(doc, thissave, "modOutSecNu"+QString::number(i));
			m_modEnabled[i]->saveSettings(doc, thissave, "modEnabled_"+QString::number(i));
			m_modCombineType[i]->saveSettings(doc, thissave, "modCombineType_"+QString::number(i));
			m_modType[i]->saveSettings(doc, thissave, "modType_"+QString::number(i));
			m_modType2[i]->saveSettings(doc, thissave, "modType2_"+QString::number(i));
		}
	}

	for (int i = 0; i < 18; ++i)
	{
		m_macro[i]->saveSettings(doc, thissave, "macro_"+QString::number(i));
		thissave.setAttribute("macroTooltips_"+QString::number(i), m_macroTooltips[i]);
		thissave.setAttribute("macroRed_"+QString::number(i), m_macroColors[i][0]);
		thissave.setAttribute("macroGreen_"+QString::number(i), m_macroColors[i][1]);
		thissave.setAttribute("macroBlue_"+QString::number(i), m_macroColors[i][2]);
	}
}


void Microwave::loadSettings(const QDomElement & thisload)
{
	QString microwaveVersion = thisload.attribute("version");

	m_visvol.loadSettings(thisload, "visualizer_volume");
	m_loadMode.loadSettings(thisload, "loadingalgorithm");
	m_loadChnl.loadSettings(thisload, "loadingchannel");
	m_oversample.loadSettings(thisload, "oversample");
	m_oversampleMode.loadSettings(thisload, "oversamplemode");
	m_removeDC.loadSettings(thisload, "removeDC");

	m_graph.setLength(2048);

	// Load widget values
	for (int i = 0; i < 8; ++i)
	{
		m_enabled[i]->loadSettings(thisload, "enabled_"+QString::number(i));
		if (m_enabled[i]->value())
		{
			m_morph[i]->loadSettings(thisload, "morph_"+QString::number(i));
			m_range[i]->loadSettings(thisload, "range_"+QString::number(i));
			m_modify[i]->loadSettings(thisload, "modify_"+QString::number(i));
			m_modifyMode[i]->loadSettings(thisload, "modifyMode_"+QString::number(i));
			m_unisonVoices[i]->loadSettings(thisload, "unisonVoices_"+QString::number(i));
			m_unisonDetune[i]->loadSettings(thisload, "unisonDetune_"+QString::number(i));
			m_unisonMorph[i]->loadSettings(thisload, "unisonMorph_"+QString::number(i));
			m_unisonModify[i]->loadSettings(thisload, "unisonModify_"+QString::number(i));
			m_morphMax[i]->loadSettings(thisload, "morphMax_"+QString::number(i));
			m_detune[i]->loadSettings(thisload, "detune_"+QString::number(i));
			m_sampLen[i]->loadSettings(thisload, "sampLen_"+QString::number(i));
			m_phase[i]->loadSettings(thisload, "phase_"+QString::number(i));
			m_phaseRand[i]->loadSettings(thisload, "phaseRand_"+QString::number(i));
			m_vol[i]->loadSettings(thisload, "vol_"+QString::number(i));
			m_muted[i]->loadSettings(thisload, "muted_"+QString::number(i));
			m_pan[i]->loadSettings(thisload, "pan_"+QString::number(i));
			m_keytracking[i]->loadSettings(thisload, "keytracking_"+QString::number(i));
			m_tempo[i]->loadSettings(thisload, "tempo_"+QString::number(i));
			m_interpolate[i]->loadSettings(thisload, "interpolate_"+QString::number(i));
		}

		m_filtEnabled[i]->loadSettings(thisload, "filtEnabled_"+QString::number(i));
		if (m_filtEnabled[i]->value())
		{
			m_filtInVol[i]->loadSettings(thisload, "filtInVol_"+QString::number(i));
			m_filtType[i]->loadSettings(thisload, "filtType_"+QString::number(i));
			m_filtSlope[i]->loadSettings(thisload, "filtSlope_"+QString::number(i));
			m_filtCutoff[i]->loadSettings(thisload, "filtCutoff_"+QString::number(i));
			m_filtReso[i]->loadSettings(thisload, "filtReso_"+QString::number(i));
			m_filtGain[i]->loadSettings(thisload, "filtGain_"+QString::number(i));
			m_filtSatu[i]->loadSettings(thisload, "filtSatu_"+QString::number(i));
			m_filtWetDry[i]->loadSettings(thisload, "filtWetDry_"+QString::number(i));
			m_filtBal[i]->loadSettings(thisload, "filtBal_"+QString::number(i));
			m_filtOutVol[i]->loadSettings(thisload, "filtOutVol_"+QString::number(i));
			m_filtFeedback[i]->loadSettings(thisload, "filtFeedback_"+QString::number(i));
			m_filtDetune[i]->loadSettings(thisload, "filtDetune_"+QString::number(i));
			m_filtKeytracking[i]->loadSettings(thisload, "filtKeytracking_"+QString::number(i));
			m_filtMuted[i]->loadSettings(thisload, "filtMuted_"+QString::number(i));
		}

		m_sampleEnabled[i]->loadSettings(thisload, "sampleEnabled_"+QString::number(i));
		if (m_sampleEnabled[i]->value())
		{
			m_sampleGraphEnabled[i]->loadSettings(thisload, "sampleGraphEnabled_"+QString::number(i));
			m_sampleMuted[i]->loadSettings(thisload, "sampleMuted_"+QString::number(i));
			m_sampleKeytracking[i]->loadSettings(thisload, "sampleKeytracking_"+QString::number(i));
			m_sampleLoop[i]->loadSettings(thisload, "sampleLoop_"+QString::number(i));
			m_sampleVolume[i]->loadSettings(thisload, "sampleVolume_"+QString::number(i));
			m_samplePanning[i]->loadSettings(thisload, "samplePanning_"+QString::number(i));
			m_sampleDetune[i]->loadSettings(thisload, "sampleDetune_"+QString::number(i));
			m_samplePhase[i]->loadSettings(thisload, "samplePhase_"+QString::number(i));
			m_samplePhaseRand[i]->loadSettings(thisload, "samplePhaseRand_"+QString::number(i));
			m_sampleStart[i]->loadSettings(thisload, "sampleStart_"+QString::number(i));
			m_sampleEnd[i]->loadSettings(thisload, "sampleEnd_"+QString::number(i));
		}
	}

	for (int i = 0; i < 18; ++i)
	{
		m_macro[i]->loadSettings(thisload, "macro_"+QString::number(i));
	}

	for (int i = 0; i < 64; ++i)
	{
		m_subEnabled[i]->loadSettings(thisload, "subEnabled_"+QString::number(i));
		if (m_subEnabled[i]->value())
		{
			m_subVol[i]->loadSettings(thisload, "subVol_"+QString::number(i));
			m_subPhase[i]->loadSettings(thisload, "subPhase_"+QString::number(i));
			m_subPhaseRand[i]->loadSettings(thisload, "subPhaseRand_"+QString::number(i));
			m_subDetune[i]->loadSettings(thisload, "subDetune_"+QString::number(i));
			m_subMuted[i]->loadSettings(thisload, "subMuted_"+QString::number(i));
			m_subKeytrack[i]->loadSettings(thisload, "subKeytrack_"+QString::number(i));
			m_subSampLen[i]->loadSettings(thisload, "subSampLen_"+QString::number(i));
			m_subNoise[i]->loadSettings(thisload, "subNoise_"+QString::number(i));
			m_subPanning[i]->loadSettings(thisload, "subPanning_"+QString::number(i));
			m_subTempo[i]->loadSettings(thisload, "subTempo_"+QString::number(i));
			m_subRateLimit[i]->loadSettings(thisload, "subRateLimit_"+QString::number(i));
			m_subUnisonNum[i]->loadSettings(thisload, "subUnisonNum_"+QString::number(i));
			m_subUnisonDetune[i]->loadSettings(thisload, "subUnisonDetune_"+QString::number(i));
			m_subInterpolate[i]->loadSettings(thisload, "subInterpolate_"+QString::number(i));
		}

		m_modEnabled[i]->loadSettings(thisload, "modEnabled_"+QString::number(i));
		if (m_modEnabled[i]->value())
		{
			m_modIn[i]->loadSettings(thisload, "modIn_"+QString::number(i));
			m_modInNum[i]->loadSettings(thisload, "modInNu"+QString::number(i));
			m_modInAmnt[i]->loadSettings(thisload, "modInAmnt_"+QString::number(i));
			m_modInCurve[i]->loadSettings(thisload, "modInCurve_"+QString::number(i));
			m_modIn2[i]->loadSettings(thisload, "modIn2_"+QString::number(i));
			m_modInNum2[i]->loadSettings(thisload, "modInNum2_"+QString::number(i));
			m_modInAmnt2[i]->loadSettings(thisload, "modAmnt2_"+QString::number(i));
			m_modInCurve2[i]->loadSettings(thisload, "modCurve2_"+QString::number(i));
			m_modOutSec[i]->loadSettings(thisload, "modOutSec_"+QString::number(i));
			m_modOutSig[i]->loadSettings(thisload, "modOutSig_"+QString::number(i));
			m_modOutSecNum[i]->loadSettings(thisload, "modOutSecNu"+QString::number(i));
			m_modCombineType[i]->loadSettings(thisload, "modCombineType_"+QString::number(i));
			m_modType[i]->loadSettings(thisload, "modType_"+QString::number(i));
			m_modType2[i]->loadSettings(thisload, "modType2_"+QString::number(i));
		}
	}

	int size = 0;
	char * dst = 0;


	// Load wavetables
	for (int j = 0; j < 8; ++j)
	{
		if (m_enabled[j]->value())
		{
			base64::decode(thisload.attribute("waveforms"+QString::number(j)), &dst, &size);
			for (int i = 0; i < STOREDMAINARRAYLEN; ++i)
			{
				m_storedwaveforms[j][i] = ((float*) dst)[i];
			}
		}
	}

	// Load sub oscillator waveforms
	for (int j = 0; j < 64; ++j)
	{
		if (m_subEnabled[j]->value())
		{
			base64::decode(thisload.attribute("subs"+QString::number(j)), &dst, &size);
			for (int i = 0; i < STOREDSUBWAVELEN; ++i)
			{
				m_storedsubs[j][i] = ((float*) dst)[i];
			}
		}
	}

	// Load sample graphs
	base64::decode(thisload.attribute("sampGraphs"), &dst, &size);
	for (int i = 0; i < 1024; ++i)
	{
		m_sampGraphs[i] = ((float*) dst)[i];
	}

	// Load sample lengths (required for loading samples)
	int m_sampleSizes[8] = {0};
	base64::decode(thisload.attribute("sampleSizes"), &dst, &size);
	for (int i = 0; i < 8; ++i)
	{
		m_sampleSizes[i] = ((int*) dst)[i];
	}

	// Load samples
	for (int i = 0; i < 8; ++i)
	{
		if (m_sampleEnabled[i]->value())
		{
			for (int j = 0; j < 2; ++j)
			{
				base64::decode(thisload.attribute("samples_"+QString::number(i)+"_"+QString::number(j)), &dst, &size);
				for (int k = 0; k < m_sampleSizes[i]; ++k)
				{
					m_samples[i][j].push_back(((float*) dst)[k]);
				}
			}
		}
	}

	// Load macro tooltips and colors
	for (int i = 0; i < 18; ++i)
	{
		m_macroTooltips[i] = thisload.attribute("macroTooltips_"+QString::number(i));
		m_macroColors[i][0] = thisload.attribute("macroRed_"+QString::number(i)).toInt();
		m_macroColors[i][1] = thisload.attribute("macroGreen_"+QString::number(i)).toInt();
		m_macroColors[i][2] = thisload.attribute("macroBlue_"+QString::number(i)).toInt();
	}

	delete[] dst;

	// Interpolate all the wavetables and waveforms when loaded.
	for (int i = 0; i < 8; ++i)
	{
		if (m_enabled[i]->value())
		{
			fillMainOsc(i, m_interpolate[i]->value());
		}
	}

	for (int i = 0; i < 64; ++i)
	{
		if (m_subEnabled[i]->value())
		{
			fillSubOsc(i, m_subInterpolate[i]->value());
		}
	}

}


// When a knob is changed, send the new value to the array holding the knob values,
// and also to the values within mSynths already initialized (notes already playing)
void Microwave::valueChanged(int which, int num)
{
	//Send new values to array
	switch (which)
	{
		case 1: m_morphArr[num] = m_morph[num]->value(); break;
		case 2: m_rangeArr[num] = m_range[num]->value(); break;
		case 3: m_modifyArr[num] = m_modify[num]->value(); break;
		case 4: m_modifyModeArr[num] = m_modifyMode[num]->value(); break;
		case 5: m_volArr[num] = m_vol[num]->value(); break;
		case 6: m_panArr[num] = m_pan[num]->value(); break;
		case 7: m_detuneArr[num] = m_detune[num]->value(); break;
		case 8: m_phaseArr[num] = m_phase[num]->value(); break;
		case 9: m_phaseRandArr[num] = m_phaseRand[num]->value(); break;
		case 10: m_enabledArr[num] = m_enabled[num]->value(); break;
		case 11: m_mutedArr[num] = m_muted[num]->value(); break;
		case 12: m_sampLenArr[num] = m_sampLen[num]->value(); break;
		case 13: m_morphMaxArr[num] = m_morphMax[num]->value(); break;
		case 14: m_unisonVoicesArr[num] = m_unisonVoices[num]->value(); break;
		case 15: m_unisonDetuneArr[num] = m_unisonDetune[num]->value(); break;
		case 16: m_unisonMorphArr[num] = m_unisonMorph[num]->value(); break;
		case 17: m_unisonModifyArr[num] = m_unisonModify[num]->value(); break;
		case 18: m_keytrackingArr[num] = m_keytracking[num]->value(); break;
		case 19: m_tempoArr[num] = m_tempo[num]->value(); break;
		case 20: m_interpolateArr[num] = m_interpolate[num]->value(); break;

		case 30: m_subEnabledArr[num] = m_subEnabled[num]->value(); break;
		case 31: m_subMutedArr[num] = m_subMuted[num]->value(); break;
		case 32: m_subKeytrackArr[num] = m_subKeytrack[num]->value(); break;
		case 33: m_subNoiseArr[num] = m_subNoise[num]->value(); break;
		case 34: m_subVolArr[num] = m_subVol[num]->value(); break;
		case 35: m_subPanningArr[num] = m_subPanning[num]->value(); break;
		case 36: m_subDetuneArr[num] = m_subDetune[num]->value(); break;
		case 37: m_subPhaseArr[num] = m_subPhase[num]->value(); break;
		case 38: m_subPhaseRandArr[num] = m_subPhaseRand[num]->value(); break;
		case 39: m_subSampLenArr[num] = m_subSampLen[num]->value(); break;
		case 40: m_subTempoArr[num] = m_subTempo[num]->value(); break;
		case 41: m_subRateLimitArr[num] = m_subRateLimit[num]->value(); break;
		case 42: m_subUnisonNumArr[num] = m_subUnisonNum[num]->value(); break;
		case 43: m_subUnisonDetuneArr[num] = m_subUnisonDetune[num]->value(); break;
		case 44: m_subInterpolateArr[num] = m_subInterpolate[num]->value(); break;

		case 60: m_sampleEnabledArr[num] = m_sampleEnabled[num]->value(); break;
		case 61: m_sampleMutedArr[num] = m_sampleMuted[num]->value(); break;
		case 62: m_sampleKeytrackingArr[num] = m_sampleKeytracking[num]->value(); break;
		case 63: m_sampleGraphEnabledArr[num] = m_sampleGraphEnabled[num]->value(); break;
		case 64: m_sampleLoopArr[num] = m_sampleLoop[num]->value(); break;
		case 65: m_sampleVolumeArr[num] = m_sampleVolume[num]->value(); break;
		case 66: m_samplePanningArr[num] = m_samplePanning[num]->value(); break;
		case 67: m_sampleDetuneArr[num] = m_sampleDetune[num]->value(); break;
		case 68: m_samplePhaseArr[num] = m_samplePhase[num]->value(); break;
		case 69: m_samplePhaseRandArr[num] = m_samplePhaseRand[num]->value(); break;
		case 70: m_sampleStartArr[num] = m_sampleStart[num]->value(); break;
		case 71: m_sampleEndArr[num] = m_sampleEnd[num]->value(); break;

		case 90: m_modInArr[num] = m_modIn[num]->value(); break;
		case 91: m_modInNumArr[num] = m_modInNum[num]->value(); break;
		case 92: m_modInAmntArr[num] = m_modInAmnt[num]->value(); break;
		case 93: m_modInCurveArr[num] = m_modInCurve[num]->value(); break;
		case 94: m_modIn2Arr[num] = m_modIn2[num]->value(); break;
		case 95: m_modInNum2Arr[num] = m_modInNum2[num]->value(); break;
		case 96: m_modInAmnt2Arr[num] = m_modInAmnt2[num]->value(); break;
		case 97: m_modInCurve2Arr[num] = m_modInCurve2[num]->value(); break;
		case 98: m_modOutSecArr[num] = m_modOutSec[num]->value(); break;
		case 99: m_modOutSigArr[num] = m_modOutSig[num]->value(); break;
		case 100: m_modOutSecNumArr[num] = m_modOutSecNum[num]->value(); break;
		case 101: m_modEnabledArr[num] = m_modEnabled[num]->value(); break;
		case 102: m_modCombineTypeArr[num] = m_modCombineType[num]->value(); break;
		case 103: m_modTypeArr[num] = m_modType[num]->value(); break;
		case 104: m_modType2Arr[num] = m_modType2[num]->value(); break;

		case 120: m_filtCutoffArr[num] = m_filtCutoff[num]->value(); break;
		case 121: m_filtResoArr[num] = m_filtReso[num]->value(); break;
		case 122: m_filtGainArr[num] = m_filtGain[num]->value(); break;
		case 123: m_filtTypeArr[num] = m_filtType[num]->value(); break;
		case 124: m_filtSlopeArr[num] = m_filtSlope[num]->value(); break;
		case 125: m_filtInVolArr[num] = m_filtInVol[num]->value(); break;
		case 126: m_filtOutVolArr[num] = m_filtOutVol[num]->value(); break;
		case 127: m_filtWetDryArr[num] = m_filtWetDry[num]->value(); break;
		case 128: m_filtBalArr[num] = m_filtBal[num]->value(); break;
		case 129: m_filtSatuArr[num] = m_filtSatu[num]->value(); break;
		case 130: m_filtFeedbackArr[num] = m_filtFeedback[num]->value(); break;
		case 131: m_filtDetuneArr[num] = m_filtDetune[num]->value(); break;
		case 132: m_filtEnabledArr[num] = m_filtEnabled[num]->value(); break;
		case 133: m_filtMutedArr[num] = m_filtMuted[num]->value(); break;
		case 134: m_filtKeytrackingArr[num] = m_filtKeytracking[num]->value(); break;

		case 150: m_macroArr[num] = m_macro[num]->value(); break;
	}

	m_nphList = NotePlayHandle::nphsOfInstrumentTrack(m_microwaveTrack, true);

	for (int i = 0; i < m_nphList.length(); ++i)
	{
		mSynth * ps = static_cast<mSynth *>(m_nphList[i]->m_pluginData);

		if (ps)// Makes sure "ps" isn't assigned a null value, if m_pluginData hasn't been created yet.
		{
			//Send new knob values to notes already playing
			switch (which)
			{
				case 1: ps->m_morph[num] = m_morph[num]->value(); break;
				case 2: ps->m_range[num] = m_range[num]->value(); break;
				case 3: ps->m_modify[num] = m_modify[num]->value(); break;
				case 4: ps->m_modifyMode[num] = m_modifyMode[num]->value(); break;
				case 5: ps->m_vol[num] = m_vol[num]->value(); break;
				case 6: ps->m_pan[num] = m_pan[num]->value(); break;
				case 7: ps->m_detune[num] = m_detune[num]->value(); break;
				case 8: ps->m_phase[num] = m_phase[num]->value(); break;
				case 9: ps->m_phaseRand[num] = m_phaseRand[num]->value(); break;
				case 10: ps->m_enabled[num] = m_enabled[num]->value(); break;
				case 11: ps->m_muted[num] = m_muted[num]->value(); break;
				case 12: ps->m_sampLen[num] = m_sampLen[num]->value(); break;
				case 13: ps->m_morphMax[num] = m_morphMax[num]->value(); break;
				case 14: ps->m_unisonVoices[num] = m_unisonVoices[num]->value(); break;
				case 15: ps->m_unisonDetune[num] = m_unisonDetune[num]->value(); break;
				case 16: ps->m_unisonMorph[num] = m_unisonMorph[num]->value(); break;
				case 17: ps->m_unisonModify[num] = m_unisonModify[num]->value(); break;
				case 18: ps->m_keytracking[num] = m_keytracking[num]->value(); break;
				case 19: ps->m_tempo[num] = m_tempo[num]->value(); break;

				case 30: ps->m_subEnabled[num] = m_subEnabled[num]->value(); break;
				case 31: ps->m_subMuted[num] = m_subMuted[num]->value(); break;
				case 32: ps->m_subKeytrack[num] = m_subKeytrack[num]->value(); break;
				case 33: ps->m_subNoise[num] = m_subNoise[num]->value(); break;
				case 34: ps->m_subVol[num] = m_subVol[num]->value(); break;
				case 35: ps->m_subPanning[num] = m_subPanning[num]->value(); break;
				case 36: ps->m_subDetune[num] = m_subDetune[num]->value(); break;
				case 37: ps->m_subPhase[num] = m_subPhase[num]->value(); break;
				case 38: ps->m_subPhaseRand[num] = m_subPhaseRand[num]->value(); break;
				case 39: ps->m_subSampLen[num] = m_subSampLen[num]->value(); break;
				case 40: ps->m_subTempo[num] = m_subTempo[num]->value(); break;
				case 41: ps->m_subRateLimit[num] = m_subRateLimit[num]->value(); break;
				case 42: ps->m_subUnisonNum[num] = m_subUnisonNum[num]->value(); break;
				case 43: ps->m_subUnisonDetune[num] = m_subUnisonDetune[num]->value(); break;
				case 44: ps->m_subInterpolate[num] = m_subInterpolate[num]->value(); break;

				case 60: ps->m_sampleEnabled[num] = m_sampleEnabled[num]->value(); break;
				case 61: ps->m_sampleMuted[num] = m_sampleMuted[num]->value(); break;
				case 62: ps->m_sampleKeytracking[num] = m_sampleKeytracking[num]->value(); break;
				case 63: ps->m_sampleGraphEnabled[num] = m_sampleGraphEnabled[num]->value(); break;
				case 64: ps->m_sampleLoop[num] = m_sampleLoop[num]->value(); break;
				case 65: ps->m_sampleVolume[num] = m_sampleVolume[num]->value(); break;
				case 66: ps->m_samplePanning[num] = m_samplePanning[num]->value(); break;
				case 67: ps->m_sampleDetune[num] = m_sampleDetune[num]->value(); break;
				case 68: ps->m_samplePhase[num] = m_samplePhase[num]->value(); break;
				case 69: ps->m_samplePhaseRand[num] = m_samplePhaseRand[num]->value(); break;
				case 70: ps->m_sampleStart[num] = m_sampleStart[num]->value(); break;
				case 71: ps->m_sampleEnd[num] = m_sampleEnd[num]->value(); break;

				case 90: ps->m_modIn[num] = m_modIn[num]->value(); break;
				case 91: ps->m_modInNum[num] = m_modInNum[num]->value(); break;
				case 92: ps->m_modInAmnt[num] = m_modInAmnt[num]->value(); break;
				case 93: ps->m_modInCurve[num] = m_modInCurve[num]->value(); break;
				case 94: ps->m_modIn2[num] = m_modIn2[num]->value(); break;
				case 95: ps->m_modInNum2[num] = m_modInNum2[num]->value(); break;
				case 96: ps->m_modInAmnt2[num] = m_modInAmnt2[num]->value(); break;
				case 97: ps->m_modInCurve2[num] = m_modInCurve2[num]->value(); break;
				case 98: ps->m_modOutSec[num] = m_modOutSec[num]->value(); break;
				case 99: ps->m_modOutSig[num] = m_modOutSig[num]->value(); break;
				case 100: ps->m_modOutSecNum[num] = m_modOutSecNum[num]->value(); break;
				case 101: ps->m_modEnabled[num] = m_modEnabled[num]->value(); break;
				case 102: ps->m_modCombineType[num] = m_modCombineType[num]->value(); break;
				case 103: ps->m_modType[num] = m_modType[num]->value(); break;
				case 104: ps->m_modType2[num] = m_modType2[num]->value(); break;

				case 120: ps->m_filtCutoff[num] = m_filtCutoff[num]->value(); break;
				case 121: ps->m_filtReso[num] = m_filtReso[num]->value(); break;
				case 122: ps->m_filtGain[num] = m_filtGain[num]->value(); break;
				case 123: ps->m_filtType[num] = m_filtType[num]->value(); break;
				case 124: ps->m_filtSlope[num] = m_filtSlope[num]->value(); break;
				case 125: ps->m_filtInVol[num] = m_filtInVol[num]->value(); break;
				case 126: ps->m_filtOutVol[num] = m_filtOutVol[num]->value(); break;
				case 127: ps->m_filtWetDry[num] = m_filtWetDry[num]->value(); break;
				case 128: ps->m_filtBal[num] = m_filtBal[num]->value(); break;
				case 129: ps->m_filtSatu[num] = m_filtSatu[num]->value(); break;
				case 130: ps->m_filtFeedback[num] = m_filtFeedback[num]->value(); break;
				case 131: ps->m_filtDetune[num] = m_filtDetune[num]->value(); break;
				case 132: ps->m_filtEnabled[num] = m_filtEnabled[num]->value(); break;
				case 133: ps->m_filtMuted[num] = m_filtMuted[num]->value(); break;
				case 134: ps->m_filtKeytracking[num] = m_filtKeytracking[num]->value(); break;

				case 150: ps->m_macro[num] = m_macro[num]->value(); break;
			}
		}
	}
}


// Set the range of Morph and Unison Morph based on Morph Max
void Microwave::morphMaxChanged(int i)
{
	m_morph[i]->setRange(m_morph[i]->minValue(), m_morphMax[i]->value(), m_morph[i]->step<float>());
	m_unisonMorph[i]->setRange(m_unisonMorph[i]->minValue(), m_morphMax[i]->value(), m_unisonMorph[i]->step<float>());
}


// Set the range of morphMax, Modify, and Unison Modify based on new wavetable waveform length
void Microwave::sampLenChanged(int i)
{
	m_morphMax[i]->setRange(m_morphMax[i]->minValue(), STOREDMAINARRAYLEN / m_sampLen[i]->value() - 2, m_morphMax[i]->step<float>());
	m_modify[i]->setRange(m_modify[i]->minValue(), m_sampLen[i]->value() - 1, m_modify[i]->step<float>());
	m_unisonModify[i]->setRange(m_unisonModify[i]->minValue(), m_sampLen[i]->value() - 1, m_unisonModify[i]->step<float>());
}


//Change graph length to waveform length
void Microwave::subSampLenChanged(int num)
{
	if (m_scroll == 1 && m_subNum.value() == num + 1)
	{
		m_graph.setLength(m_subSampLen[num]->value());
	}
}


//Stores the highest enabled wavetable oscillator.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::mainEnabledChanged(int num)
{
	for (int i = 0; i < 8; ++i)
	{
		if (m_enabled[i]->value())
		{
			m_maxMainEnabled = i + 1;
		}
	}
}


//Stores the highest enabled sub oscillator.  Helps with major CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::subEnabledChanged(int num)
{
	for (int i = 0; i < 64; ++i)
	{
		if (m_subEnabled[i]->value())
		{
			m_maxSubEnabled = i + 1;
		}
	}
}


//Stores the highest enabled sample.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::sampleEnabledChanged(int num)
{
	for (int i = 0; i < 8; ++i)
	{
		if (m_sampleEnabled[i]->value())
		{
			m_maxSampleEnabled = i + 1;
		}
	}
}


//Stores the highest enabled mod section.  Helps with major CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::modEnabledChanged(int num)
{
	for (int i = 0; i < 64; ++i)
	{
		if (m_modEnabled[i]->value())
		{
			m_maxModEnabled = i + 1;
		}
	}
}


//Stores the highest enabled filter section.  Helps with CPU benefit, refer to its use in mSynth::nextStringSample
void Microwave::filtEnabledChanged(int num)
{
	for (int i = 0; i < 8; ++i)
	{
		if (m_filtEnabled[i]->value())
		{
			m_maxFiltEnabled = i + 1;
		}
	}
}


//Updates wavetable interpolation when the interpolation LED is changed.
void Microwave::interpolateChanged(int num)
{
	fillMainOsc(num, m_interpolate[num]->value());
}


//Updates sub oscillator interpolation when the interpolation LED is changed.
void Microwave::subInterpolateChanged(int num)
{
	fillSubOsc(num, m_subInterpolate[num]->value());
}


//When user draws on graph, send new values to the correct arrays
void Microwave::samplesChanged(int begin, int end)
{
	switch ((int)m_scroll)
	{
		case 0:
		{
			break;
		}
		case 1:
		{
			for (int i = begin; i <= end; ++i)
			{
				m_storedsubs[m_subNum.value()-1][i] = m_graph.samples()[i];
				for (int j = 0; j < WAVERATIO; ++j)
				{
					// Puts low-quality samples into there so one can change the waveform mid-note.  The quality boost will occur at another time.
					// It cannot do the interpolation here because it causes lag.
					m_subs[m_subNum.value()-1][i*WAVERATIO+j] = m_graph.samples()[i];
				}
			}

			m_subFilled[m_subNum.value()-1] = false;// Make sure the waveform is interpolated later on.

			// If the entire graph was changed all at once, we can assume it isn't from the user drawing on the m_graph,
			// so we can interpolate the oscillator without worrying about lag.
			if (begin == 0 && end == STOREDSUBWAVELEN - 1)
			{
				fillSubOsc(m_subNum.value()-1, m_subInterpolate[m_subNum.value()-1]->value());
			}
			break;
		}
		case 2:
		{
			for (int i = begin; i <= end; ++i)
			{
				m_sampGraphs[i + ((m_sampNum.value()-1) * 128)] = m_graph.samples()[i];
			}
			break;
		}
	}
}


void Microwave::switchMatrixSections(int source, int destination)
{
	int m_modInTemp = m_modInArr[destination];
	int m_modInNumTemp = m_modInNumArr[destination];
	float m_modInAmntTemp = m_modInAmntArr[destination];
	float m_modInCurveTemp = m_modInCurveArr[destination];
	int m_modIn2Temp = m_modIn2Arr[destination];
	int m_modInNum2Temp = m_modInNum2Arr[destination];
	float m_modInAmnt2Temp = m_modInAmnt2Arr[destination];
	float m_modInCurve2Temp = m_modInCurve2Arr[destination];
	int m_modOutSecTemp = m_modOutSecArr[destination];
	int m_modOutSigTemp = m_modOutSigArr[destination];
	int m_modOutSecNumTemp = m_modOutSecNumArr[destination];
	bool m_modEnabledTemp = m_modEnabledArr[destination];
	int m_modCombineTypeTemp = m_modCombineTypeArr[destination];
	bool m_modTypeTemp = m_modTypeArr[destination];
	bool m_modType2Temp = m_modType2Arr[destination];

	m_modIn[destination]->setValue(m_modInArr[source]);
	m_modInNum[destination]->setValue(m_modInNumArr[source]);
	m_modInAmnt[destination]->setValue(m_modInAmntArr[source]);
	m_modInCurve[destination]->setValue(m_modInCurveArr[source]);
	m_modIn2[destination]->setValue(m_modIn2Arr[source]);
	m_modInNum2[destination]->setValue(m_modInNum2Arr[source]);
	m_modInAmnt2[destination]->setValue(m_modInAmnt2Arr[source]);
	m_modInCurve2[destination]->setValue(m_modInCurve2Arr[source]);
	m_modOutSec[destination]->setValue(m_modOutSecArr[source]);
	m_modOutSig[destination]->setValue(m_modOutSigArr[source]);
	m_modOutSecNum[destination]->setValue(m_modOutSecNumArr[source]);
	m_modEnabled[destination]->setValue(m_modEnabledArr[source]);
	m_modCombineType[destination]->setValue(m_modCombineTypeArr[source]);
	m_modType[destination]->setValue(m_modTypeArr[source]);
	m_modType2[destination]->setValue(m_modType2Arr[source]);

	m_modIn[source]->setValue(m_modInTemp);
	m_modInNum[source]->setValue(m_modInNumTemp);
	m_modInAmnt[source]->setValue(m_modInAmntTemp);
	m_modInCurve[source]->setValue(m_modInCurveTemp);
	m_modIn2[source]->setValue(m_modIn2Temp);
	m_modInNum2[source]->setValue(m_modInNum2Temp);
	m_modInAmnt2[source]->setValue(m_modInAmnt2Temp);
	m_modInCurve2[source]->setValue(m_modInCurve2Temp);
	m_modOutSec[source]->setValue(m_modOutSecTemp);
	m_modOutSig[source]->setValue(m_modOutSigTemp);
	m_modOutSecNum[source]->setValue(m_modOutSecNumTemp);
	m_modEnabled[source]->setValue(m_modEnabledTemp);
	m_modCombineType[source]->setValue(m_modCombineTypeTemp);
	m_modType[source]->setValue(m_modTypeTemp);
	m_modType2[source]->setValue(m_modType2Temp);

	// If something is sent to a matrix box and the matrix box is moved,
	// we want to make sure it's still attached to the same box after it is moved.
	for (int i = 0; i < 64; ++i)
	{
		if (m_modOutSec[i]->value() == 4)// If output is being sent to Matrix
		{
			if (m_modOutSecNum[i]->value() - 1 == source)// If output was being sent a matrix box that was moved
			{
				m_modOutSecNum[i]->setValue(destination + 1);
			}
			else if (m_modOutSecNum[i]->value() - 1 == destination)// If output was being sent a matrix box that was moved
			{
				m_modOutSecNum[i]->setValue(source + 1);
			}
		}
	}
}


// For when notes are playing.  This initializes a new mSynth if the note is new.
// It also uses mSynth::nextStringSample to get the synthesizer output.
// This is where oversampling and the visualizer are handled.
void Microwave::playNote(NotePlayHandle * n, sampleFrame * working_buffer)
{
	// If the note is brand new
	if (n->m_pluginData == NULL || n->totalFramesPlayed() == 0)
	{
		n->m_pluginData = new mSynth(
			n,
			m_morphArr, m_rangeArr, m_modifyArr, m_modifyModeArr, m_volArr, m_panArr, m_detuneArr, m_phaseArr, m_phaseRandArr, m_enabledArr, m_mutedArr,
			m_sampLenArr, m_morphMaxArr, m_unisonVoicesArr, m_unisonDetuneArr, m_unisonMorphArr, m_unisonModifyArr, m_keytrackingArr, m_tempoArr, m_interpolateArr,
			m_subEnabledArr, m_subMutedArr, m_subKeytrackArr, m_subNoiseArr, m_subVolArr, m_subPanningArr, m_subDetuneArr, m_subPhaseArr, m_subPhaseRandArr,
			m_subSampLenArr, m_subTempoArr, m_subRateLimitArr, m_subUnisonNumArr, m_subUnisonDetuneArr, m_subInterpolateArr,
			m_sampleEnabledArr, m_sampleMutedArr, m_sampleKeytrackingArr, m_sampleGraphEnabledArr, m_sampleLoopArr, m_sampleVolumeArr, m_samplePanningArr,
			m_sampleDetuneArr, m_samplePhaseArr, m_samplePhaseRandArr, m_sampleStartArr, m_sampleEndArr,
			m_modInArr, m_modInNumArr, m_modInAmntArr, m_modInCurveArr, m_modIn2Arr, m_modInNum2Arr, m_modInAmnt2Arr, m_modInCurve2Arr,
			m_modOutSecArr, m_modOutSigArr, m_modOutSecNumArr, m_modEnabledArr, m_modCombineTypeArr, m_modTypeArr, m_modType2Arr,
			m_filtCutoffArr, m_filtResoArr, m_filtGainArr, m_filtTypeArr, m_filtSlopeArr, m_filtInVolArr, m_filtOutVolArr, m_filtWetDryArr, m_filtBalArr,
			m_filtSatuArr, m_filtFeedbackArr, m_filtDetuneArr, m_filtEnabledArr, m_filtMutedArr, m_filtKeytrackingArr,
			m_macroArr,
			m_samples);
		m_mwc = dynamic_cast<Microwave *>(n->instrumentTrack()->instrument());

		// Interpolate sub oscillator waveform if it hasn't been done already
		for (int i = 0; i < 64; ++i)
		{
			if (m_subEnabled[i]->value())
			{
				if (!m_subFilled[i])
				{
					fillSubOsc(i, m_subInterpolate[i]->value());
				}
			}
		}
	}

	const fpp_t frames = n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = n->noteOffset();

	mSynth * ps = static_cast<mSynth *>(n->m_pluginData);
	for (fpp_t frame = offset; frame < frames + offset; ++frame)
	{
		sampleFrame outputSample;
		sampleFrame totalOutputSample = {0, 0};

		switch (m_oversampleMode.value())
		{
			case 0:
			{
				// Process some samples and ignore the output, depending on the oversampling value.
				// For example, if the oversampling is set to 4x, it will calculate 4 samples and output 1 of those.
				for (int i = 0; i < m_oversample.value() + 1; ++i)
				{
					ps->nextStringSample(outputSample, m_waveforms, m_subs, m_samples, m_sampGraphs, m_maxMainEnabled, m_maxSubEnabled, m_maxSampleEnabled, m_maxFiltEnabled, m_maxModEnabled, Engine::mixer()->processingSampleRate() * (m_oversample.value() + 1), m_mwc, m_removeDC.value(),  m_storedsubs);
				}
				totalOutputSample[0] = outputSample[0];
				totalOutputSample[1] = outputSample[1];

				break;
			}
			case 1:
			{
				// Process some number of samples and average them together, depending on the oversampling value.
				// For example, if the oversampling is set to 4x, it will calculate 4 samples and take the average of those.
				for (int i = 0; i < m_oversample.value() + 1; ++i)
				{
					ps->nextStringSample(outputSample, m_waveforms, m_subs, m_samples, m_sampGraphs, m_maxMainEnabled, m_maxSubEnabled, m_maxSampleEnabled, m_maxFiltEnabled, m_maxModEnabled, Engine::mixer()->processingSampleRate() * (m_oversample.value() + 1), m_mwc, m_removeDC.value(), m_storedsubs);
					totalOutputSample[0] += outputSample[0];
					totalOutputSample[1] += outputSample[1];
				}

				totalOutputSample[0] /= (m_oversample.value() + 1);
				totalOutputSample[1] /= (m_oversample.value() + 1);

				break;
			}
		}

		for (ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl)
		{
			//Send to output
			working_buffer[frame][chnl] = totalOutputSample[chnl];
		}

		// Update visualizer
		if (m_viewOpen && m_visualize.value() && m_scroll == 0 && ps->m_enabled[m_mainNum.value()-1])
		{
			m_visualizerValues[int((ps->m_sample_realindex[m_mainNum.value()-1][0] / (ps->m_sampLen[m_mainNum.value()-1] * WAVERATIO)) * 204.f)] = ps->m_mainsample[m_mainNum.value()-1][0] * m_visvol.value() * 0.01f;
			if (ps->m_noteDuration % 1470 == 1)// Updates around 30 times per second per note
			{
				m_graph.setSamples(m_visualizerValues);
			}
		}
	}

	applyRelease(working_buffer, n);

	instrumentTrack()->processAudioBuffer(working_buffer, frames + offset, n);
}


void Microwave::deleteNotePluginData(NotePlayHandle * n)
{
}


// Fill m_subs using m_storedsubs, oftentimes making use of libsamplerate for some awesome sinc interpolation.
inline void Microwave::fillSubOsc(int which, bool doInterpolate)
{
	if (doInterpolate)
	{
		srccpy(m_subs[which].data(), m_storedsubs[which].data(), STOREDSUBWAVELEN);
	}
	else
	{
		for (int i = 0; i < STOREDSUBWAVELEN; ++i)
		{
			for (int j = 0; j < WAVERATIO; ++j)
			{
				m_subs[which][i*WAVERATIO+j] = m_storedsubs[which][i];
			}
		}
	}
	m_subFilled[which] = true;
}



// Fill m_waveforms using m_storedwaveforms, oftentimes making use of libsamplerate for some awesome sinc interpolation.
inline void Microwave::fillMainOsc(int which, bool doInterpolate)
{
	if (doInterpolate)
	{
		srccpy(m_waveforms[which].data(), m_storedwaveforms[which].data(), STOREDMAINARRAYLEN);
	}
	else
	{
		for (int i = 0; i < STOREDMAINARRAYLEN; ++i)
		{
			for (int j = 0; j < WAVERATIO; ++j)
			{
				m_waveforms[which][i*WAVERATIO+j] = m_storedwaveforms[which][i];
			}
		}
	}
	m_mainFilled[which] = true;
}




//===================//
//== MICROWAVEVIEW ==//
//===================//

// Creates the Microwave GUI.
MicrowaveView::MicrowaveView(Instrument * instrument,
					QWidget * parent) :
	InstrumentView(instrument, parent)
{
	setAutoFillBackground(true);

	setMouseTracking(true);

	setAcceptDrops(true);

	m_pal.setBrush(backgroundRole(), m_tab1ArtworkImg);
	setPalette(m_pal);

	QWidget * view = new QWidget(parent);
	view->setFixedSize(250, 250);

	QPixmap m_filtBoxesImg = PLUGIN_NAME::getIconPixmap("filterBoxes");
	m_filtBoxesLabel = new QLabel(this);
	m_filtBoxesLabel->setPixmap(m_filtBoxesImg);
	m_filtBoxesLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	QPixmap m_matrixBoxesImg = PLUGIN_NAME::getIconPixmap("matrixBoxes");
	m_matrixBoxesLabel = new QLabel(this);
	m_matrixBoxesLabel->setPixmap(m_matrixBoxesImg);
	m_matrixBoxesLabel->setAttribute(Qt::WA_TransparentForMouseEvents);


	makeknob(m_morphKnob, knobColored, tr("Morph"), tr("The Morph knob chooses which waveform out of the wavetable to play."));

	makeknob(m_rangeKnob, knobColored, tr("Range"), tr("The Range knob interpolates (triangularly) the waveforms near the waveform selected by the Morph knob."));

	makeknob(m_modifyKnob, knobColored, tr("Modify"), tr("The Modify knob warps the wavetable realtime using super math powers.  The formula depends on the Modify Mode dropdown box."));

	makeknob(m_detuneKnob, knobSmallColored, tr("Detune"), tr("This knob changes the pitch of the oscillator, in cents."));

	makeknob(m_phaseKnob, knobSmallColored, tr("Phase"), tr("This knob changes the phase (starting position) of the oscillator."));

	makeknob(m_volKnob, knobColored, tr("Volume"), tr("This knob changes the volume.  What a surprise!"));

	makeknob(m_panKnob, knobColored, tr("Panning"), tr("This knob lowers the volume in one ear by an amount depending on this knob's value."));

	makeknob(m_sampLenKnob, knobColored, tr("Waveform Sample Length"), tr("This knob changes the number of samples per waveform there are in the wavetable.  This is useful for finetuning the wavetbale if the wavetable loading was slightly off."));

	makeknob(m_morphMaxKnob, knobColored, tr("Morph Max"), tr("This knob sets the maximum value of the Morph knob."));

	makeknob(m_phaseRandKnob, knobSmallColored, tr("Phase Randomness"), tr("This knob chooses a random phase for every note and unison voice.  The phase change will never be larger than this knob's value."));

	makeknob(m_unisonVoicesKnob, knobSmallColored, tr("Unison Voices"), tr("This knob clones this oscillator multiple times depending on its value, and makes slight changes to the clones depending on the other unison-related knobs."));

	makeknob(m_unisonDetuneKnob, knobSmallColored, tr("Unison Detune"), tr("This knob detunes every unison voice by a random number that is less than the specified amount."));

	makeknob(m_unisonMorphKnob, knobSmallColored, tr("Unison Morph"), tr("This knob changes the wavetable position of each individual unison voice."));

	makeknob(m_unisonModifyKnob, knobSmallColored, tr("Unison Modify"), tr("This knob changes the Modify value of each individual unison voice."));

	makeknob(m_tempoKnob, knobSmallColored, tr("Tempo Sync"), tr("When this knob is anything other than 0, the oscillator is tempo synced to the specified tempo.  This is meant for the creation of envelopes/LFOs/step sequencers in the Matrix tab, and you'll most likely want to enable the Muted LED when using this."));

	m_modifyModeBox = new ComboBox(this);
	m_modifyModeBox->setGeometry(0, 5, 42, 22);
	m_modifyModeBox->setFont(pointSize<8>(m_modifyModeBox->font()));
	ToolTip::add(m_modifyModeBox, tr("The Modify Mode dropdown box chooses the formula for the Modify knob to use to warp the wavetable realtime in cool-sounding ways."));

	m_enabledToggle = new LedCheckBox("", this, tr("Oscillator Enabled"), LedCheckBox::Green);
	ToolTip::add(m_enabledToggle, tr("This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!"));

	m_mutedToggle = new LedCheckBox("", this, tr("Oscillator Muted"), LedCheckBox::Green);
	ToolTip::add(m_mutedToggle, tr("This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output."));

	m_keytrackingToggle = new LedCheckBox("", this, tr("Keytracking"), LedCheckBox::Green);
	ToolTip::add(m_keytrackingToggle, tr("This button turns keytracking on/off.  Without keytracking, the frequency will be 440 Hz by default, and will ignore the frequency of the played note, but will still follow other methods of detuning the sound."));

	m_interpolateToggle = new LedCheckBox("", this, tr("Interpolation Enabled"), LedCheckBox::Green);
	ToolTip::add(m_interpolateToggle, tr("This button turns sinc interpolation on/off.  Interpolation uses no CPU and makes the oscillator super duper high-quality.  You'll probably only ever want to turn this off when you're using small waveform lengths and you don't want the waveform smoothed over."));


	m_sampleEnabledToggle = new LedCheckBox("", this, tr("Sample Enabled"), LedCheckBox::Green);
	ToolTip::add(m_sampleEnabledToggle, tr("This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!"));
	m_sampleGraphEnabledToggle = new LedCheckBox("", this, tr("Sample Graph Enabled"), LedCheckBox::Green);
	ToolTip::add(m_sampleGraphEnabledToggle, tr("This button enables the graph for this oscillator.  On the graph, left/right is time and up/down is position in the msample.  A saw wave in the graph will play the m_sample normally."));
	m_sampleMutedToggle = new LedCheckBox("", this, tr("Sample Muted"), LedCheckBox::Green);
	ToolTip::add(m_sampleMutedToggle, tr("This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output."));
	m_sampleKeytrackingToggle = new LedCheckBox("", this, tr("Sample Keytracking"), LedCheckBox::Green);
	ToolTip::add(m_sampleKeytrackingToggle, tr("This button turns keytracking on/off.  Without keytracking, the frequency will be 440 Hz by default, and will ignore the frequency of the played note, but will still follow other methods of detuning the sound."));
	m_sampleLoopToggle = new LedCheckBox("", this, tr("Loop Sample"), LedCheckBox::Green);
	ToolTip::add(m_sampleLoopToggle, tr("This button turns looping on/off.  When looping is on, the sample will go back to the starting position when it is done playing."));

	makeknob(m_sampleVolumeKnob, knobColored, tr("Volume"), tr("This, like most other volume knobs, controls the volume."));

	makeknob(m_samplePanningKnob, knobColored, tr("Panning"), tr("This knob lowers the volume in one ear by an amount depending on this knob's value."));

	makeknob(m_sampleDetuneKnob, knobSmallColored, tr("Detune"), tr("This knob changes the pitch (and speed) of the sample."));

	makeknob(m_samplePhaseKnob, knobSmallColored, tr("Phase"), tr("This knob changes the position of the sample, and is updated realtime when automated."));

	makeknob(m_samplePhaseRandKnob, knobSmallColored, tr("Phase Randomness"), tr("This knob makes the sample start at a random position with every note."));

	makeknob(m_sampleStartKnob, knobSmallColored, tr("Start"), tr("This knob changes the starting position of the sample."));

	makeknob(m_sampleEndKnob, knobSmallColored, tr("End"), tr("This knob changes the ending position of the sample."));


	for (int i = 0; i < 8; ++i)
	{
		makeknob(m_filtInVolKnob[i], knobSmallColored, tr("Input Volume"), tr("This knob changes the input volume of the filter."));

		makeknob(m_filtCutoffKnob[i], knobSmallColored, tr("Cutoff Frequency"), tr("This knob changes cutoff frequency of the filter."));

		makeknob(m_filtResoKnob[i], knobSmallColored, tr("Resonance"), tr("This knob changes the resonance of the filter."));

		makeknob(m_filtGainKnob[i], knobSmallColored, tr("db Gain"), tr("This knob changes the gain of the filter.  This only applies to some filter types, e.g. Peak, High Shelf, Low Shelf, etc."));

		makeknob(m_filtSatuKnob[i], knobSmallColored, tr("Saturation"), tr("This knob applies some basic distortion after the filter is applied."));

		makeknob(m_filtWetDryKnob[i], knobSmallColored, tr("Wet/Dry"), tr("This knob allows mixing the filtered signal with the unfiltered signal."));

		makeknob(m_filtBalKnob[i], knobSmallColored, tr("Balance/Panning"), tr("This knob decreases the Wet and increases the Dry of the filter in one ear, depending on the knob's value."));

		makeknob(m_filtOutVolKnob[i], knobSmallColored, tr("Output Volume"), tr("This knob changes the output volume of the filter."));

		makeknob(m_filtFeedbackKnob[i], knobSmallColored, tr("Feedback"), tr("This knob sends the specified portion of the filter output back into the input after a certain delay.  This delay effects the pitch, and can be changed using the m_filter's Detune and Keytracking options."));

		makeknob(m_filtDetuneKnob[i], knobSmallColored, tr("Detune (Feedback Delay)"), tr("This knob changes the delay of the filter's feedback to match the specified pitch."));

		m_filtTypeBox[i] = new ComboBox(this);
		m_filtTypeBox[i]->setGeometry(1000, 5, 42, 22);
		m_filtTypeBox[i]->setFont(pointSize<8>(m_filtTypeBox[i]->font()));
		ToolTip::add(m_filtTypeBox[i], tr("This dropdown box changes the filter type."));

		m_filtSlopeBox[i] = new ComboBox(this);
		m_filtSlopeBox[i]->setGeometry(1000, 5, 42, 22);
		m_filtSlopeBox[i]->setFont(pointSize<8>(m_filtSlopeBox[i]->font()));
		ToolTip::add(m_filtSlopeBox[i], tr("This dropdown box changes how many times the audio is run through the filter (which changes the slope).  For example, a sound run through a 12 db filter three times will result in a 36 db slope."));

		m_filtEnabledToggle[i] = new LedCheckBox("", this, tr("Filter Enabled"), LedCheckBox::Green);
		ToolTip::add(m_filtEnabledToggle[i], tr("This button enables the filter.  A disabled filter will never do anything and does not use any CPU.  In many cases, the settings of a disabled filter will not be saved, so be careful!"));

		m_filtKeytrackingToggle[i] = new LedCheckBox("", this, tr("Keytracking"), LedCheckBox::Green);
		ToolTip::add(m_filtKeytrackingToggle[i], tr("When this is enabled, the delay of the filter's feedback changes to match the frequency of the notes you play."));

		m_filtMutedToggle[i] = new LedCheckBox("", this, tr("Muted"), LedCheckBox::Green);
		ToolTip::add(m_filtMutedToggle[i], tr("This button mutes the filter.  An enabled but muted filter will still use CPU and still work as a matrix input, but will not be sent to the audio output.  You'll want to use this almost every time you send something to the Matrix."));
	}

	for (int i = 0; i < 18; ++i)
	{
		makeknob(m_macroKnob[i], knobSmallColored, tr("Macro") + " " + QString::number(i+1) + ":", tr("Macro %1: ").arg(i + 1) + tr("This knob's value can be used in the Matrix to control many values at the same time, at different amounts.  This is immensely useful for crafting great presets.  Right click on this knob for some special Macro-specific knob features."));
	}

	makeknob(m_subVolKnob, knobColored, tr("Volume"), tr("This knob, as you probably expected, controls the volume."));

	makeknob(m_subPhaseKnob, knobSmallColored, tr("Phase"), tr("This knob changes the phase (starting position) of the oscillator."));

	makeknob(m_subPhaseRandKnob, knobSmallColored, tr("Phase Randomness"), tr("This knob chooses a random phase for every note.  The phase change will never be larger than this knob's value."));

	makeknob(m_subDetuneKnob, knobColored, tr("Detune"), tr("This knob changes the pitch of the oscillator."));

	makeknob(m_subSampLenKnob, knobColored, tr("Waveform Sample Length"), tr("This knob changes the waveform length, which you can see in the graph."));

	makeknob(m_subPanningKnob, knobColored, tr("Panning"), tr("This knob lowers the volume in one ear by an amount depending on this knob's value."));

	makeknob(m_subTempoKnob, knobColored, tr("Tempo Sync"), tr("When this knob is anything other than 0, the oscillator is tempo synced to the specified tempo.  This is meant for the creation of envelopes/LFOs/step sequencers in the Matrix tab, and you'll most likely want to enable the Muted LED when using this."));

	makeknob(m_subRateLimitKnob, knobColored, tr("Rate Limit"), tr("This knob limits the rate at which the waveform can change.  Combined with the Noise LED being enabled, this could potentially be used as a chaos oscillator."));

	makeknob(m_subUnisonNumKnob, knobColored, tr("Unison Voices"), tr("This knob clones this oscillator multiple times depending on its value, and makes slight changes to the clones depending on the other unison-related knobs."));

	makeknob(m_subUnisonDetuneKnob, knobColored, tr("Unison Detune"), tr("This knob detunes every unison voice by a random number that is less than the specified amount."));

	m_subEnabledToggle = new LedCheckBox("", this, tr("Enabled"), LedCheckBox::Green);
	ToolTip::add(m_subEnabledToggle, tr("This button enables the oscillator.  A disabled oscillator will never do anything and does not use any CPU.  In many cases, the settings of a disabled oscillator will not be saved, so be careful!"));

	m_subMutedToggle = new LedCheckBox("", this, tr("Muted"), LedCheckBox::Green);
	ToolTip::add(m_subMutedToggle, tr("This button mutes the oscillator.  An enabled but muted oscillator will still use CPU and still work as a matrix input, but will not be sent to the audio output."));

	m_subKeytrackToggle = new LedCheckBox("", this, tr("Keytracking Enabled"), LedCheckBox::Green);
	ToolTip::add(m_subKeytrackToggle, tr("This button turns keytracking on/off.  Without keytracking, the frequency will be 440 Hz by default, and will ignore the frequency of the played note, but will still follow other methods of detuning the sound."));

	m_subNoiseToggle = new LedCheckBox("", this, tr("Noise Enabled"), LedCheckBox::Green);
	ToolTip::add(m_subNoiseToggle, tr("This button converts this oscillator into a noise generator.  A random part of the graph is chosen, that value is added to the previous output in the same direction it was going, and when the waveform crosses the top or bottom, the direction changes."));

	m_subInterpolateToggle = new LedCheckBox("", this, tr("Interpolation Enabled"), LedCheckBox::Green);
	ToolTip::add(m_subInterpolateToggle, tr("This button turns sinc interpolation on/off.  Interpolation uses no CPU and makes the oscillator super duper high-quality.  You'll probably only ever want to turn this off when you're using small waveform lengths and you don't want the waveform smoothed over."));

	for (int i = 0; i < 8; ++i)
	{
		makeknob(m_modInAmntKnob[i], knobSmallColored, tr("Matrix Input Amount"), tr("This knob controls how much of the input to send to the output."));

		makeknob(m_modInCurveKnob[i], knobSmallColored, tr("Matrix Curve Amount"), tr("This knob gives the input value a bias toward the top or bottom"));

		makeknob(m_modInAmntKnob2[i], knobSmallColored, tr("Secondary Matrix Input Amount"), tr("This knob controls how much of the input to send to the output."));

		makeknob(m_modInCurveKnob2[i], knobSmallColored, tr("Secondary Matrix Curve Amount"), tr("This knob gives the input value a bias toward the top or bottom"));

		m_modOutSecBox[i] = new ComboBox(this);
		m_modOutSecBox[i]->setGeometry(2000, 5, 42, 22);
		m_modOutSecBox[i]->setFont(pointSize<8>(m_modOutSecBox[i]->font()));
		ToolTip::add(m_modOutSecBox[i], tr("This dropdown box chooses an output for the Matrix Box."));

		m_modOutSigBox[i] = new ComboBox(this);
		m_modOutSigBox[i]->setGeometry(2000, 5, 42, 22);
		m_modOutSigBox[i]->setFont(pointSize<8>(m_modOutSigBox[i]->font()));
		ToolTip::add(m_modOutSigBox[i], tr("This dropdown box chooses which part of the input to send to the Matrix Box (e.g. which parameter to control, which filter, etc.)."));

		m_modOutSecNumBox[i] = new LcdSpinBox(2, "microwave", this, "Mod Output Number");
		ToolTip::add(m_modOutSecNumBox[i], tr("This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator)."));

		m_modInBox[i] = new ComboBox(this);
		m_modInBox[i]->setGeometry(2000, 5, 42, 22);
		m_modInBox[i]->setFont(pointSize<8>(m_modInBox[i]->font()));
		ToolTip::add(m_modInBox[i], tr("This dropdown box chooses an input for the Matrix Box."));

		m_modInNumBox[i] = new LcdSpinBox(2, "microwave", this, "Mod Input Number");
		ToolTip::add(m_modInNumBox[i], tr("This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator, which filter, etc.)."));

		m_modInBox2[i] = new ComboBox(this);
		m_modInBox2[i]->setGeometry(2000, 5, 42, 22);
		m_modInBox2[i]->setFont(pointSize<8>(m_modInBox2[i]->font()));
		ToolTip::add(m_modInBox2[i], tr("This dropdown box chooses an input for the Matrix Box."));

		m_modInNumBox2[i] = new LcdSpinBox(2, "microwave", this, "Secondary Mod Input Number");
		ToolTip::add(m_modInNumBox2[i], tr("This spinbox chooses which part of the input to send to the Matrix Box (e.g. which oscillator, which filter, etc.)."));

		m_modEnabledToggle[i] = new LedCheckBox("", this, tr("Modulation Enabled"), LedCheckBox::Green);
		ToolTip::add(m_modEnabledToggle[i], tr("This button enables the Matrix Box.  While disabled, this does not use CPU."));

		m_modCombineTypeBox[i] = new ComboBox(this);
		m_modCombineTypeBox[i]->setGeometry(2000, 5, 42, 22);
		m_modCombineTypeBox[i]->setFont(pointSize<8>(m_modCombineTypeBox[i]->font()));
		ToolTip::add(m_modCombineTypeBox[i], tr("This dropdown box chooses how to combine the two Matrix inputs."));

		m_modTypeToggle[i] = new LedCheckBox("", this, tr("Envelope Enabled"), LedCheckBox::Green);
		ToolTip::add(m_modTypeToggle[i], tr("This button, when enabled, treats the input as an envelope rather than an LFO."));

		m_modType2Toggle[i] = new LedCheckBox("", this, tr("Envelope Enabled"), LedCheckBox::Green);
		ToolTip::add(m_modType2Toggle[i], tr("This button, when enabled, treats the input as an envelope rather than an LFO."));

		m_modUpArrow[i] = new PixmapButton(this, tr("Move Matrix Section Up"));
		m_modUpArrow[i]->setActiveGraphic(PLUGIN_NAME::getIconPixmap("arrowup"));
		m_modUpArrow[i]->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("arrowup"));
		ToolTip::add(m_modUpArrow[i], tr("Move this Matrix Box up"));

		m_modDownArrow[i] = new PixmapButton(this, tr("Move Matrix Section Down"));
		m_modDownArrow[i]->setActiveGraphic(PLUGIN_NAME::getIconPixmap("arrowdown"));
		m_modDownArrow[i]->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("arrowdown"));
		ToolTip::add(m_modDownArrow[i], tr("Move this Matrix Box down"));

		m_i1Button[i] = new PixmapButton(this, tr("Go to this input's location"));
		m_i1Button[i]->setActiveGraphic(PLUGIN_NAME::getIconPixmap("i1_button"));
		m_i1Button[i]->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("i1_button"));
		ToolTip::add(m_i1Button[i], tr("Go to this input's location"));

		m_i2Button[i] = new PixmapButton(this, tr("Go to this input's location"));
		m_i2Button[i]->setActiveGraphic(PLUGIN_NAME::getIconPixmap("i2_button"));
		m_i2Button[i]->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("i2_button"));
		ToolTip::add(m_i2Button[i], tr("Go to this input's location"));

		m_modNumText[i] = new QLineEdit(this);
		m_modNumText[i]->resize(23, 19);
	}

	makeknob(m_visvolKnob, knobSmallColored, tr("Visualizer Volume"), tr("This knob works as a vertical zoom knob for the visualizer."));
	m_visvolKnob->setModel(&m_b->m_visvol);

	makeknob(m_loadChnlKnob, knobColored, tr("Wavetable Loading Channel"), tr("This knob chooses whether to load the left or right audio of the selected sample/wavetable."));
	m_loadChnlKnob->setModel(&m_b->m_loadChnl);


	m_graph = new Graph(this, Graph::BarCenterGradStyle, 204, 134);
	m_graph->setAutoFillBackground(true);
	m_graph->setGraphColor(QColor(121, 222, 239));
	m_graph->setModel(&m_b->m_graph);

	ToolTip::add(m_graph, tr ("Draw here by dragging your mouse on this graph."));

	m_pal = QPalette();
	m_pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraphdisabled"));
	m_graph->setPalette(m_pal);

	QPixmap m_filtForegroundImg = PLUGIN_NAME::getIconPixmap("filterForeground");
	m_filtForegroundLabel = new QLabel(this);
	m_filtForegroundLabel->setPixmap(m_filtForegroundImg);
	m_filtForegroundLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	QPixmap m_matrixForegroundImg = PLUGIN_NAME::getIconPixmap("matrixForeground");
	m_matrixForegroundLabel = new QLabel(this);
	m_matrixForegroundLabel->setPixmap(m_matrixForegroundImg);
	m_matrixForegroundLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

	m_sinWaveBtn = new PixmapButton(this, tr("Sine"));
	m_sinWaveBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("sinwave_active"));
	m_sinWaveBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("sinwave"));
	ToolTip::add(m_sinWaveBtn, tr("Sine wave"));

	m_triangleWaveBtn = new PixmapButton(this, tr("Nachos"));
	m_triangleWaveBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("triwave_active"));
	m_triangleWaveBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("triwave"));
	ToolTip::add(m_triangleWaveBtn, tr("Nacho wave"));

	m_sawWaveBtn = new PixmapButton(this, tr("Sawsa"));
	m_sawWaveBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("sawwave_active"));
	m_sawWaveBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("sawwave"));
	ToolTip::add(m_sawWaveBtn, tr("Sawsa wave"));

	m_sqrWaveBtn = new PixmapButton(this, tr("Sosig"));
	m_sqrWaveBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("sqrwave_active"));
	m_sqrWaveBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("sqrwave"));
	ToolTip::add(m_sqrWaveBtn, tr("Sosig wave"));

	m_whiteNoiseWaveBtn = new PixmapButton(this, tr("Metal Fork"));
	m_whiteNoiseWaveBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("noisewave_active"));
	m_whiteNoiseWaveBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("noisewave"));
	ToolTip::add(m_whiteNoiseWaveBtn, tr("Metal Fork"));

	m_usrWaveBtn = new PixmapButton(this, tr("Takeout"));
	m_usrWaveBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("fileload"));
	m_usrWaveBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("fileload"));
	ToolTip::add(m_usrWaveBtn, tr("Takeout Menu"));

	m_smoothBtn = new PixmapButton(this, tr("Microwave Cover"));
	m_smoothBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("smoothwave_active"));
	m_smoothBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("smoothwave"));
	ToolTip::add(m_smoothBtn, tr("Microwave Cover"));


	m_sinWave2Btn = new PixmapButton(this, tr("Sine"));
	m_sinWave2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("sinwave_active"));
	m_sinWave2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("sinwave"));
	ToolTip::add(m_sinWave2Btn, tr("Sine wave"));

	m_triangleWave2Btn = new PixmapButton(this, tr("Nachos"));
	m_triangleWave2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("triwave_active"));
	m_triangleWave2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("triwave"));
	ToolTip::add(m_triangleWave2Btn,
			tr("Nacho wave"));

	m_sawWave2Btn = new PixmapButton(this, tr("Sawsa"));
	m_sawWave2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("sawwave_active"));
	m_sawWave2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("sawwave"));
	ToolTip::add(m_sawWave2Btn,
			tr("Sawsa wave"));

	m_sqrWave2Btn = new PixmapButton(this, tr("Sosig"));
	m_sqrWave2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("sqrwave_active"));
	m_sqrWave2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("sqrwave"));
	ToolTip::add(m_sqrWave2Btn, tr("Sosig wave"));

	m_whiteNoiseWave2Btn = new PixmapButton(this, tr("Metal Fork"));
	m_whiteNoiseWave2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("noisewave_active"));
	m_whiteNoiseWave2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("noisewave"));
	ToolTip::add(m_whiteNoiseWave2Btn, tr("Metal Fork"));

	m_usrWave2Btn = new PixmapButton(this, tr("Takeout Menu"));
	m_usrWave2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("fileload"));
	m_usrWave2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("fileload"));
	ToolTip::add(m_usrWave2Btn, tr("Takeout Menu"));

	m_smooth2Btn = new PixmapButton(this, tr("Microwave Cover"));
	m_smooth2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("smoothwave_active"));
	m_smooth2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("smoothwave"));
	ToolTip::add(m_smooth2Btn, tr("Microwave Cover"));


	m_tab1Btn = new PixmapButton(this, tr("Wavetable Tab"));
	m_tab1Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab1_active"));
	m_tab1Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab1_active"));
	ToolTip::add(m_tab1Btn, tr("Wavetable Tab"));

	m_tab2Btn = new PixmapButton(this, tr("Sub Tab"));
	m_tab2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab2"));
	m_tab2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab2"));
	ToolTip::add(m_tab2Btn, tr("Sub Tab"));

	m_tab3Btn = new PixmapButton(this, tr("Sample Tab"));
	m_tab3Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab3"));
	m_tab3Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab3"));
	ToolTip::add(m_tab3Btn, tr("Sample Tab"));

	m_tab4Btn = new PixmapButton(this, tr("Matrix Tab"));
	m_tab4Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab4"));
	m_tab4Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab4"));
	ToolTip::add(m_tab4Btn, tr("Matrix Tab"));

	m_tab5Btn = new PixmapButton(this, tr("Effect Tab"));
	m_tab5Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab5"));
	m_tab5Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab5"));
	ToolTip::add(m_tab5Btn, tr("Effect Tab"));

	m_tab6Btn = new PixmapButton(this, tr("Miscellaneous Tab"));
	m_tab6Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab6"));
	m_tab6Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab6"));
	ToolTip::add(m_tab6Btn, tr("Miscellaneous Tab"));


	m_mainFlipBtn = new PixmapButton(this, tr("Flip to other knobs"));
	m_mainFlipBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("arrowup"));
	m_mainFlipBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("arrowdown"));
	ToolTip::add(m_mainFlipBtn, tr("Flip to other knobs"));
	m_mainFlipBtn->setCheckable(true);

	m_subFlipBtn = new PixmapButton(this, tr("Flip to other knobs"));
	m_subFlipBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("arrowup"));
	m_subFlipBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("arrowdown"));
	ToolTip::add(m_subFlipBtn, tr("Flip to other knobs"));
	m_subFlipBtn->setCheckable(true);


	m_manualBtn = new PixmapButton(this, tr("Manual"));
	m_manualBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("manual_active"));
	m_manualBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("manual_inactive"));
	ToolTip::add(m_manualBtn, tr("Manual"));


	m_removeDCBtn = new PixmapButton(this, tr("Remove DC Offset"));
	m_removeDCBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("remove_dc_offset_button_enabled"));
	m_removeDCBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("remove_dc_offset_button_disabled"));
	ToolTip::add(m_removeDCBtn, tr("Remove DC Offset"));
	m_removeDCBtn->setCheckable(true);
	m_removeDCBtn->setModel(&m_b->m_removeDC);

	m_oversampleModeBox = new ComboBox(this);
	m_oversampleModeBox->setGeometry(0, 0, 42, 22);
	m_oversampleModeBox->setFont(pointSize<8>(m_oversampleModeBox->font()));
	m_oversampleModeBox->setModel(&m_b->m_oversampleMode);


	m_normalizeBtn = new PixmapButton(this, tr("Normalize Wavetable"));
	m_normalizeBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("normalize_button_active"));
	m_normalizeBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("normalize_button"));
	ToolTip::add(m_normalizeBtn, tr("Normalize Wavetable"));

	m_desawBtn = new PixmapButton(this, tr("De-saw Wavetable"));
	m_desawBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("desaw_button_active"));
	m_desawBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("desaw_button"));
	ToolTip::add(m_desawBtn, tr("De-saw Wavetable"));


	m_XBtn = new PixmapButton(this, tr("Leave wavetable loading section"));
	m_XBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("xbtn"));
	m_XBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("xbtn"));
	ToolTip::add(m_XBtn, tr("Leave wavetable loading tab"));

	m_MatrixXBtn = new PixmapButton(this, tr("Leave Matrix tab"));
	m_MatrixXBtn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("xbtn"));
	m_MatrixXBtn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("xbtn"));
	ToolTip::add(m_MatrixXBtn, tr("Leave wavetable loading tab"));


	m_visualizeToggle = new LedCheckBox("", this, tr("Visualize"), LedCheckBox::Green);

	m_openWavetableButton = new PixmapButton(this);
	m_openWavetableButton->setCursor(QCursor(Qt::PointingHandCursor));
	m_openWavetableButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("fileload"));
	m_openWavetableButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("fileload"));
	connect(m_openWavetableButton, SIGNAL(clicked()), this, SLOT(openWavetableFileBtnClicked()));
	ToolTip::add(m_openWavetableButton, tr("Open wavetable"));

	m_confirmLoadButton = new PixmapButton(this);
	m_confirmLoadButton->setCursor(QCursor(Qt::PointingHandCursor));
	m_confirmLoadButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("confirm_button_active"));
	m_confirmLoadButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("confirm_button_inactive"));
	connect(m_confirmLoadButton, SIGNAL(clicked()), this, SLOT(confirmWavetableLoadClicked()));
	ToolTip::add(m_confirmLoadButton, tr("Load Wavetable"));

	m_mainNumBox = new LcdSpinBox(2, "microwave", this, "Oscillator Number");
	ToolTip::add(m_mainNumBox, tr("Oscillator Number"));
	m_mainNumBox->setModel(&m_b->m_mainNum);

	m_subNumBox = new LcdSpinBox(2, "microwave", this, "Sub Oscillator Number");
	ToolTip::add(m_subNumBox, tr("Oscillator Number"));
	m_subNumBox->setModel(&m_b->m_subNum);

	m_sampNumBox = new LcdSpinBox(2, "microwave", this, "Sample Number");
	ToolTip::add(m_sampNumBox, tr("Oscillator Number"));
	m_sampNumBox->setModel(&m_b->m_sampNum);

	m_oversampleBox = new ComboBox(this);
	m_oversampleBox->setGeometry(0, 0, 42, 22);
	m_oversampleBox->setFont(pointSize<8>(m_oversampleBox->font()));
	ToolTip::add(m_oversampleBox, tr("Oversampling Amount"));
	m_oversampleBox->setModel(&m_b->m_oversample);

	m_loadModeBox = new ComboBox(this);
	m_loadModeBox->setGeometry(0, 0, 202, 22);
	m_loadModeBox->setFont(pointSize<8>(m_loadModeBox->font()));
	ToolTip::add(m_loadModeBox, tr("Oversampling Mode"));
	m_loadModeBox->setModel(&m_b->m_loadMode);

	m_openSampleButton = new PixmapButton(this);
	m_openSampleButton->setCursor(QCursor(Qt::PointingHandCursor));
	m_openSampleButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("fileload"));
	m_openSampleButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("fileload"));
	ToolTip::add(m_openSampleButton, tr("Load Sound Sample"));

	m_effectScrollBar = new QScrollBar(Qt::Vertical, this);
	m_effectScrollBar->setSingleStep(1);
	m_effectScrollBar->setPageStep(100);
	m_effectScrollBar->setFixedHeight(197);
	m_effectScrollBar->setRange(0, 590);
	connect(m_effectScrollBar, SIGNAL(valueChanged(int)), this, SLOT(updateScroll()));

	m_effectScrollBar->setStyleSheet("QScrollBar::handle:horizontal { background: #3f4750; border: none; border-radius: 1px; min-width: 24px; }\
					QScrollBar::handle:horizontal:hover { background: #a6adb1; }\
					QScrollBar::handle:horizontal:pressed { background: #a6adb1; }\
					QScrollBar::handle:vertical { background: #3f4750; border: none; border-radius: 1px; min-height: 24px; }\
					QScrollBar::handle:vertical:hover { background: #a6adb1; }\
					QScrollBar::handle:vertical:pressed { background: #a6adb1; }\
					QScrollBar::handle:horizontal:disabled, QScrollBar::handle:vertical:disabled  { background: #262b30; border-radius: 1px; border: none; }");

	m_matrixScrollBar = new QScrollBar(Qt::Vertical, this);
	m_matrixScrollBar->setSingleStep(1);
	m_matrixScrollBar->setPageStep(100);
	m_matrixScrollBar->setFixedHeight(197);
	m_matrixScrollBar->setRange(0, 6232);
	connect(m_matrixScrollBar, SIGNAL(valueChanged(int)), this, SLOT(updateScroll()));

	m_matrixScrollBar->setStyleSheet("QScrollBar::handle:horizontal { background: #3f4750; border: none; border-radius: 1px; min-width: 24px; }\
					QScrollBar::handle:horizontal:hover { background: #a6adb1; }\
					QScrollBar::handle:horizontal:pressed { background: #a6adb1; }\
					QScrollBar::handle:vertical { background: #3f4750; border: none; border-radius: 1px; min-height: 24px; }\
					QScrollBar::handle:vertical:hover { background: #a6adb1; }\
					QScrollBar::handle:vertical:pressed { background: #a6adb1; }\
					QScrollBar::handle:horizontal:disabled, QScrollBar::handle:vertical:disabled  { background: #262b30; border-radius: 1px; border: none; }");

	// The above scrollbar style changes don't seem to work entirely for some reason.


	connect(m_openSampleButton, SIGNAL(clicked()), this, SLOT(openSampleFileBtnClicked()));
	ToolTip::add(m_openSampleButton, tr("Open sample"));


	connect(m_sinWaveBtn, SIGNAL (clicked ()), this, SLOT (sinWaveClicked()));
	connect(m_triangleWaveBtn, SIGNAL (clicked ()), this, SLOT (triangleWaveClicked()));
	connect(m_sawWaveBtn, SIGNAL (clicked ()), this, SLOT (sawWaveClicked()));
	connect(m_sqrWaveBtn, SIGNAL (clicked ()), this, SLOT (sqrWaveClicked()));
	connect(m_whiteNoiseWaveBtn, SIGNAL (clicked ()), this, SLOT (noiseWaveClicked()));
	connect(m_usrWaveBtn, SIGNAL (clicked ()), this, SLOT (usrWaveClicked()));
	connect(m_smoothBtn, SIGNAL (clicked ()), this, SLOT (smoothClicked()));

	connect(m_sinWave2Btn, SIGNAL (clicked ()), this, SLOT (sinWaveClicked()));
	connect(m_triangleWave2Btn, SIGNAL (clicked ()), this, SLOT (triangleWaveClicked()));
	connect(m_sawWave2Btn, SIGNAL (clicked ()), this, SLOT (sawWaveClicked()));
	connect(m_sqrWave2Btn, SIGNAL (clicked ()), this, SLOT (sqrWaveClicked()));
	connect(m_whiteNoiseWave2Btn, SIGNAL (clicked ()), this, SLOT (noiseWaveClicked()));
	connect(m_usrWave2Btn, SIGNAL (clicked ()), this, SLOT (usrWaveClicked()));
	connect(m_smooth2Btn, SIGNAL (clicked ()), this, SLOT (smoothClicked()));


	connect(m_XBtn, SIGNAL (clicked ()), this, SLOT (XBtnClicked()));
	connect(m_MatrixXBtn, SIGNAL (clicked ()), this, SLOT (MatrixXBtnClicked()));

	connect(m_normalizeBtn, SIGNAL (clicked ()), this, SLOT (normalizeClicked()));
	connect(m_desawBtn, SIGNAL (clicked ()), this, SLOT (desawClicked()));


	int ii = 0;
	connect(m_tab1Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); });
	ii = 1;
	connect(m_tab2Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); });
	ii = 2;
	connect(m_tab3Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); });
	ii = 3;
	connect(m_tab4Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); });
	ii = 4;
	connect(m_tab5Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); });
	ii = 5;
	connect(m_tab6Btn, &PixmapButton::clicked, this, [this, ii]() { tabBtnClicked(ii); });


	connect(m_mainFlipBtn, SIGNAL (clicked ()), this, SLOT (flipperClicked()));
	ToolTip::add(m_mainFlipBtn, tr("Flip to other knobs"));
	m_mainFlipBtn->setModel(&m_b->m_mainFlipped);

	connect(m_subFlipBtn, SIGNAL (clicked ()), this, SLOT (flipperClicked()));
	ToolTip::add(m_subFlipBtn, tr("Flip to other knobs"));
	m_subFlipBtn->setModel(&m_b->m_subFlipped);

	connect(m_visualizeToggle, SIGNAL(toggled(bool)), this, SLOT (visualizeToggled(bool)));
	ToolTip::add(m_visualizeToggle, tr("Enable wavetable visualizer"));
	m_visualizeToggle->setModel(&m_b->m_visualize);

	connect(&m_b->m_mainNum, SIGNAL(dataChanged()), this, SLOT(mainNumChanged()));
	connect(&m_b->m_subNum, SIGNAL(dataChanged()), this, SLOT(subNumChanged()));
	connect(&m_b->m_sampNum, SIGNAL(dataChanged()), this, SLOT(sampNumChanged()));

	connect(m_manualBtn, SIGNAL (clicked (bool)), this, SLOT (manualBtnClicked()));
	ToolTip::add(m_manualBtn, tr("Open the instruction manual"));

	for (int i = 0; i < 64; ++i)
	{
		connect(m_b->m_modOutSec[i], &ComboBoxModel::dataChanged, this, [this, i]() { modOutSecChanged(i); }, Qt::DirectConnection);
		connect(m_b->m_modIn[i], &ComboBoxModel::dataChanged, this, [this, i]() { modInChanged(i); }, Qt::DirectConnection);
		connect(m_b->m_modIn2[i], &ComboBoxModel::dataChanged, this, [this, i]() { modIn2Changed(i); }, Qt::DirectConnection);

		connect(m_b->m_modEnabled[i], &BoolModel::dataChanged, this, [this, i]() { modEnabledChanged(); });
	}

	for (int i = 0; i < 8; ++i)
	{
		connect(m_modUpArrow[i], &PixmapButton::clicked, this, [this, i]() { modUpClicked(i); });
		connect(m_modDownArrow[i], &PixmapButton::clicked, this, [this, i]() { modDownClicked(i); });

		connect(m_i1Button[i], &PixmapButton::clicked, this, [this, i]() { i1Clicked(i); });
		connect(m_i2Button[i], &PixmapButton::clicked, this, [this, i]() { i2Clicked(i); });

		//Make changing Enabled LED update graph color
		connect(m_b->m_enabled[i], SIGNAL(dataChanged()), this, SLOT(mainNumChanged()));
		connect(m_b->m_subEnabled[i], SIGNAL(dataChanged()), this, SLOT(subNumChanged()));
		connect(m_b->m_sampleEnabled[i], SIGNAL(dataChanged()), this, SLOT(sampNumChanged()));

		// This is run here to prevent the graph from being the wrong color on project load
		mainNumChanged();
	}

	for (int i = 0; i < 18; ++i)
	{
		// Add custom tooltips to Macro knobs
		if (!m_b->m_macroTooltips[i].isEmpty())
		{
			ToolTip::add(m_macroKnob[i], tr("Macro %1: ").arg(i + 1) + m_b->m_macroTooltips[i]);
		}

		m_macroKnob[i]->refreshMacroColor();
	}

	m_b->m_viewOpen = true;

	updateScroll();
	updateBackground();

	modEnabledChanged();// Updates scroll bar length in Matrix
}


MicrowaveView::~MicrowaveView()
{
	if (castModel<Microwave>()) { castModel<Microwave>()->m_viewOpen = false; }
}


// Moves GUI elements to their correct locations and toggles their visibility when needed
void MicrowaveView::updateScroll()
{
	int modScrollVal = (m_matrixScrollBar->value()) / 100.f * 115.f;
	int effectScrollVal = (m_effectScrollBar->value()) / 100.f * 92.f;
	int mainFlipped = m_b->m_mainFlipped.value();
	int subFlipped = m_b->m_subFlipped.value();

	bool inMainTab = m_b->m_scroll == 0;
	bool inSubTab = m_b->m_scroll == 1;
	bool inSampleTab = m_b->m_scroll == 2;
	bool inMatrixTab = m_b->m_scroll == 3;
	bool inEffectTab = m_b->m_scroll == 4;
	bool inMiscTab = m_b->m_scroll == 5;
	bool inWavetableLoadingTab = m_b->m_scroll == 6;

	bool mainIsNotFlipped = inMainTab && !mainFlipped;
	bool mainIsFlipped = inMainTab && mainFlipped;

	bool subIsNotFlipped = inSubTab && !subFlipped;
	bool subIsFlipped = inSubTab && subFlipped;

	visimove(m_morphKnob, ((m_b->m_scroll == 0) ? 23 : 176), 172, mainIsNotFlipped || inWavetableLoadingTab);
	visimove(m_rangeKnob, ((m_b->m_scroll == 0) ? 55 : 208), 172, mainIsNotFlipped || inWavetableLoadingTab);
	visimove(m_modifyKnob, 87, 172, mainIsNotFlipped);
	visimove(m_modifyModeBox, 127, 186, mainIsNotFlipped);
	visimove(m_volKnob, 23, 172, mainIsFlipped);
	visimove(m_panKnob, 55, 172, mainIsFlipped);
	visimove(m_detuneKnob, 152, 216, mainIsNotFlipped);
	visimove(m_phaseKnob, 184, 203, mainIsFlipped);
	visimove(m_phaseRandKnob, 209, 203, mainIsFlipped);
	visimove(m_enabledToggle, 85, 229, inMainTab);
	visimove(m_mutedToggle, 103, 229, inMainTab);
	visimove(m_sampLenKnob, 137, 172, mainIsFlipped);
	visimove(m_morphMaxKnob, 101, 172, mainIsFlipped);
	visimove(m_unisonVoicesKnob, 184, 172, inMainTab);
	visimove(m_unisonDetuneKnob, 209, 172, inMainTab);
	visimove(m_unisonMorphKnob, 184, 203, mainIsNotFlipped);
	visimove(m_unisonModifyKnob, 209, 203, mainIsNotFlipped);
	visimove(m_keytrackingToggle, 121, 229, mainIsNotFlipped);
	visimove(m_tempoKnob, 152, 216, mainIsFlipped);
	visimove(m_interpolateToggle, 121, 229, mainIsFlipped);

	visimove(m_sampleEnabledToggle, 85, 229, inSampleTab);
	visimove(m_sampleMutedToggle, 103, 229, inSampleTab);
	visimove(m_sampleKeytrackingToggle, 121, 229, inSampleTab);
	visimove(m_sampleGraphEnabledToggle, 138, 229, inSampleTab);
	visimove(m_sampleLoopToggle, 155, 229, inSampleTab);
	visimove(m_sampleVolumeKnob, 23, 172, inSampleTab);
	visimove(m_samplePanningKnob, 55, 172, inSampleTab);
	visimove(m_sampleDetuneKnob, 93, 172, inSampleTab);
	visimove(m_samplePhaseKnob, 180, 172, inSampleTab);
	visimove(m_samplePhaseRandKnob, 206, 172, inSampleTab);
	visimove(m_sampleStartKnob, 121, 172, inSampleTab);
	visimove(m_sampleEndKnob, 145, 172, inSampleTab);

	for (int i = 0; i < 8; ++i)
	{
		visimove(m_filtCutoffKnob[i], 32, i*92+55 - effectScrollVal, inEffectTab);
		visimove(m_filtResoKnob[i], 63, i*92+55 - effectScrollVal, inEffectTab);
		visimove(m_filtGainKnob[i], 94, i*92+55 - effectScrollVal, inEffectTab);

		visimove(m_filtTypeBox[i], 128, i*92+63 - effectScrollVal, inEffectTab);
		visimove(m_filtSlopeBox[i], 171, i*92+63 - effectScrollVal, inEffectTab);
		visimove(m_filtInVolKnob[i], 30, i*92+91 - effectScrollVal, inEffectTab);
		visimove(m_filtOutVolKnob[i], 55, i*92+91 - effectScrollVal, inEffectTab);
		visimove(m_filtWetDryKnob[i], 80, i*92+91 - effectScrollVal, inEffectTab);
		visimove(m_filtBalKnob[i], 105, i*92+91 - effectScrollVal, inEffectTab);
		visimove(m_filtSatuKnob[i], 135, i*92+91 - effectScrollVal, inEffectTab);
		visimove(m_filtFeedbackKnob[i], 167, i*92+91 - effectScrollVal, inEffectTab);
		visimove(m_filtDetuneKnob[i], 192, i*92+91 - effectScrollVal, inEffectTab);
		visimove(m_filtEnabledToggle[i], 27, i*92+36 - effectScrollVal, inEffectTab);
		visimove(m_filtMutedToggle[i], 166, i*92+36 - effectScrollVal, inEffectTab);
		visimove(m_filtKeytrackingToggle[i], 200, i*92+36 - effectScrollVal, inEffectTab);
	}

	visimove(m_subVolKnob, 23, 172, subIsNotFlipped);
	visimove(m_subPanningKnob, 55, 172, subIsNotFlipped);
	visimove(m_subDetuneKnob, 95, 172, subIsNotFlipped);
	visimove(m_subPhaseKnob, 180, 172, inSubTab);
	visimove(m_subPhaseRandKnob, 206, 172, inSubTab);
	visimove(m_subSampLenKnob, 130, 172, subIsNotFlipped);
	visimove(m_subTempoKnob, 23, 172, subIsFlipped);
	visimove(m_subRateLimitKnob, 55, 172, subIsFlipped);
	visimove(m_subUnisonNumKnob, 95, 172, subIsFlipped);
	visimove(m_subUnisonDetuneKnob, 130, 172, subIsFlipped);

	if (subIsNotFlipped)
	{
		visimove(m_subEnabledToggle, 85, 229, inSubTab);
		visimove(m_subMutedToggle, 103, 229, inSubTab);
		visimove(m_subKeytrackToggle, 121, 229, inSubTab);
		visimove(m_subNoiseToggle, 138, 229, inSubTab);
		visimove(m_subInterpolateToggle, 155, 229, inSubTab);
	}
	else
	{
		visimove(m_subEnabledToggle, 85, 235, inSubTab);
		visimove(m_subMutedToggle, 103, 235, inSubTab);
		visimove(m_subKeytrackToggle, 121, 235, inSubTab);
		visimove(m_subNoiseToggle, 138, 235, inSubTab);
		visimove(m_subInterpolateToggle, 155, 235, inSubTab);
	}

	int matrixRemainder = modScrollVal % 460;
	int matrixDivide = modScrollVal / 460 * 4;
	for (int i = 0; i < 8; ++i)
	{
		if (i+matrixDivide < 64)
		{
			m_modOutSecBox[i]->setModel(m_b->m_modOutSec[i+matrixDivide]);
			m_modOutSigBox[i]->setModel(m_b->m_modOutSig[i+matrixDivide]);
			m_modOutSecNumBox[i]->setModel(m_b->m_modOutSecNum[i+matrixDivide]);

			m_modInBox[i]->setModel(m_b->m_modIn[i+matrixDivide]);
			m_modInNumBox[i]->setModel(m_b->m_modInNum[i+matrixDivide]);
			m_modInAmntKnob[i]->setModel(m_b->m_modInAmnt[i+matrixDivide]);
			m_modInCurveKnob[i]->setModel(m_b->m_modInCurve[i+matrixDivide]);

			m_modInBox2[i]->setModel(m_b->m_modIn2[i+matrixDivide]);
			m_modInNumBox2[i]->setModel(m_b->m_modInNum2[i+matrixDivide]);
			m_modInAmntKnob2[i]->setModel(m_b->m_modInAmnt2[i+matrixDivide]);
			m_modInCurveKnob2[i]->setModel(m_b->m_modInCurve2[i+matrixDivide]);

			m_modEnabledToggle[i]->setModel(m_b->m_modEnabled[i+matrixDivide]);

			m_modCombineTypeBox[i]->setModel(m_b->m_modCombineType[i+matrixDivide]);

			m_modTypeToggle[i]->setModel(m_b->m_modType[i+matrixDivide]);
			m_modType2Toggle[i]->setModel(m_b->m_modType2[i+matrixDivide]);

			m_modNumText[i]->setText(QString::number(i+matrixDivide+1));

			m_modInAmntKnob[i]->setMatrixLocation(4, 1, i);
			m_modInCurveKnob[i]->setMatrixLocation(4, 2, i);
			m_modInAmntKnob2[i]->setMatrixLocation(4, 3, i);
			m_modInCurveKnob2[i]->setMatrixLocation(4, 4, i);
		}

		// Bug evasion.  Without this, some display glitches happen in certain conditions.
		modOutSecChanged(i+matrixDivide);
		modInChanged(i+matrixDivide);
		modIn2Changed(i+matrixDivide);

		visimove(m_modInBox[i], 45, i*115+57 - matrixRemainder, inMatrixTab);
		visimove(m_modInNumBox[i], 90, i*115+57 - matrixRemainder, inMatrixTab && m_modInNumBox[i]->isVisible());
		visimove(m_modInAmntKnob[i], 136, i*115+53 - matrixRemainder, inMatrixTab);
		visimove(m_modInCurveKnob[i], 161, i*115+53 - matrixRemainder, inMatrixTab);
		visimove(m_modInBox2[i], 45, i*115+118 - matrixRemainder, inMatrixTab);
		visimove(m_modInNumBox2[i], 90, i*115+118 - matrixRemainder, inMatrixTab && m_modInNumBox2[i]->isVisible());
		visimove(m_modInAmntKnob2[i], 136, i*115+114 - matrixRemainder, inMatrixTab);
		visimove(m_modInCurveKnob2[i], 161, i*115+114 - matrixRemainder, inMatrixTab);
		visimove(m_modOutSecBox[i], 27, i*115+88 - matrixRemainder, inMatrixTab);
		visimove(m_modOutSigBox[i], 69, i*115+88 - matrixRemainder, inMatrixTab && m_modOutSigBox[i]->isVisible());
		visimove(m_modOutSecNumBox[i], 112, i*115+88 - matrixRemainder, inMatrixTab && m_modOutSecNumBox[i]->isVisible());
		visimove(m_modEnabledToggle[i], 27, i*115+36 - matrixRemainder, inMatrixTab);
		visimove(m_modCombineTypeBox[i], 149, i*115+88 - matrixRemainder, inMatrixTab);
		visimove(m_modTypeToggle[i], 195, i*115+67 - matrixRemainder, inMatrixTab);
		visimove(m_modType2Toggle[i], 195, i*115+128 - matrixRemainder, inMatrixTab);
		visimove(m_modUpArrow[i], 181, i*115+37 - matrixRemainder, inMatrixTab);
		visimove(m_modDownArrow[i], 199, i*115+37 - matrixRemainder, inMatrixTab);
		visimove(m_i1Button[i], 25, i*115+50 - matrixRemainder, inMatrixTab);
		visimove(m_i2Button[i], 25, i*115+112 - matrixRemainder, inMatrixTab);
		visimove(m_modNumText[i], 192, i*115+89 - matrixRemainder, inMatrixTab);
	}

	for (int i = 0; i < 8; ++i)
	{
		m_filtCutoffKnob[i]->setModel(m_b->m_filtCutoff[i]);
		m_filtCutoffKnob[i]->setMatrixLocation(6, 1, i);

		m_filtResoKnob[i]->setModel(m_b->m_filtReso[i]);
		m_filtResoKnob[i]->setMatrixLocation(6, 2, i);

		m_filtGainKnob[i]->setModel(m_b->m_filtGain[i]);
		m_filtGainKnob[i]->setMatrixLocation(6, 3, i);

		m_filtTypeBox[i]->setModel(m_b->m_filtType[i]);

		m_filtSlopeBox[i]->setModel(m_b->m_filtSlope[i]);

		m_filtInVolKnob[i]->setModel(m_b->m_filtInVol[i]);
		m_filtInVolKnob[i]->setMatrixLocation(6, 5, i);

		m_filtOutVolKnob[i]->setModel(m_b->m_filtOutVol[i]);
		m_filtOutVolKnob[i]->setMatrixLocation(6, 6, i);

		m_filtWetDryKnob[i]->setModel(m_b->m_filtWetDry[i]);
		m_filtWetDryKnob[i]->setMatrixLocation(6, 7, i);

		m_filtBalKnob[i]->setModel(m_b->m_filtBal[i]);
		m_filtBalKnob[i]->setMatrixLocation(6, 8, i);

		m_filtSatuKnob[i]->setModel(m_b->m_filtSatu[i]);
		m_filtSatuKnob[i]->setMatrixLocation(6, 9, i);

		m_filtFeedbackKnob[i]->setModel(m_b->m_filtFeedback[i]);
		m_filtFeedbackKnob[i]->setMatrixLocation(6, 10, i);

		m_filtDetuneKnob[i]->setModel(m_b->m_filtDetune[i]);
		m_filtDetuneKnob[i]->setMatrixLocation(6, 11, i);

		m_filtEnabledToggle[i]->setModel(m_b->m_filtEnabled[i]);

		m_filtMutedToggle[i]->setModel(m_b->m_filtMuted[i]);

		m_filtKeytrackingToggle[i]->setModel(m_b->m_filtKeytracking[i]);
	}

	for (int i = 0; i < 18; ++i)
	{
		m_macroKnob[i]->setModel(m_b->m_macro[i]);
		m_macroKnob[i]->setMatrixLocation(7, i, 0);
		m_macroKnob[i]->setWhichMacroKnob(i);
		refreshMacroColor(m_macroKnob[i], i);
	}

	visimove(m_visvolKnob, 230, 24, inMainTab && m_b->m_visualize.value());

	visimove(m_loadChnlKnob, 111, 121, inWavetableLoadingTab);
	visimove(m_visualizeToggle, 213, 26, inMainTab);
	visimove(m_mainNumBox, 18, 219, inMainTab);
	visimove(m_subNumBox, 18, 219, inSubTab);
	visimove(m_sampNumBox, 18, 219, inSampleTab);
	visimove(m_graph, 23 , 30, inMainTab || inSubTab || inSampleTab);
	visimove(m_openWavetableButton, ((m_b->m_scroll == 0) ? 54 : 115), (m_b->m_scroll == 0) ? 220 : 24, inMainTab || inWavetableLoadingTab);
	visimove(m_openSampleButton, 54, 220, inSampleTab);

	visimove(m_sinWaveBtn, 179, 212, inSubTab);
	visimove(m_triangleWaveBtn, 197, 212, inSubTab);
	visimove(m_sawWaveBtn, 215, 212, inSubTab);
	visimove(m_sqrWaveBtn, 179, 227, inSubTab);
	visimove(m_whiteNoiseWaveBtn, 197, 227, inSubTab);
	visimove(m_smoothBtn, 215, 227, inSubTab);
	visimove(m_usrWaveBtn, 54, 220, inSubTab);

	visimove(m_sinWave2Btn, 179, 212, inSampleTab);
	visimove(m_triangleWave2Btn, 197, 212, inSampleTab);
	visimove(m_sawWave2Btn, 215, 212, inSampleTab);
	visimove(m_sqrWave2Btn, 179, 227, inSampleTab);
	visimove(m_whiteNoiseWave2Btn, 197, 227, inSampleTab);
	visimove(m_smooth2Btn, 215, 227, inSampleTab);
	visimove(m_usrWave2Btn, 54, 220, inSampleTab);

	visimove(m_oversampleBox, 70, 50, inMiscTab);
	visimove(m_oversampleModeBox, 135, 50, inMiscTab);
	visimove(m_removeDCBtn, 68, 84, inMiscTab);

	visimove(m_effectScrollBar, 221, 32, inEffectTab);
	visimove(m_matrixScrollBar, 221, 32, inMatrixTab);

	visimove(m_filtForegroundLabel, 0, 0, inEffectTab);
	visimove(m_filtBoxesLabel, 24, 35 - (effectScrollVal % 92), inEffectTab);

	visimove(m_matrixForegroundLabel, 0, 0, inMatrixTab);
	visimove(m_matrixBoxesLabel, 24, 35 - (modScrollVal % 115), inMatrixTab);

	visimove(m_macroKnob[0], 59, 127, inMiscTab);
	visimove(m_macroKnob[1], 81, 127, inMiscTab);
	visimove(m_macroKnob[2], 103, 127, inMiscTab);
	visimove(m_macroKnob[3], 125, 127, inMiscTab);
	visimove(m_macroKnob[4], 147, 127, inMiscTab);
	visimove(m_macroKnob[5], 169, 127, inMiscTab);
	visimove(m_macroKnob[6], 59, 147, inMiscTab);
	visimove(m_macroKnob[7], 81, 147, inMiscTab);
	visimove(m_macroKnob[8], 103, 147, inMiscTab);
	visimove(m_macroKnob[9], 125, 147, inMiscTab);
	visimove(m_macroKnob[10], 147, 147, inMiscTab);
	visimove(m_macroKnob[11], 169, 147, inMiscTab);
	visimove(m_macroKnob[12], 59, 167, inMiscTab);
	visimove(m_macroKnob[13], 81, 167, inMiscTab);
	visimove(m_macroKnob[14], 103, 167, inMiscTab);
	visimove(m_macroKnob[15], 125, 167, inMiscTab);
	visimove(m_macroKnob[16], 147, 168, inMiscTab);
	visimove(m_macroKnob[17], 169, 168, inMiscTab);

	visimove(m_tab1Btn, 1, 48, true);
	visimove(m_tab2Btn, 1, 63, true);
	visimove(m_tab3Btn, 1, 78, true);
	visimove(m_tab4Btn, 1, 93, true);
	visimove(m_tab5Btn, 1, 108, true);
	visimove(m_tab6Btn, 1, 123, true);

	visimove(m_mainFlipBtn, 3, 145, inMainTab);
	visimove(m_subFlipBtn, 3, 145, inSubTab);

	visimove(m_manualBtn, 49, 199, inMiscTab);

	visimove(m_loadModeBox, 25, 76, inWavetableLoadingTab);
	visimove(m_confirmLoadButton, 93, 187, inWavetableLoadingTab);

	visimove(m_XBtn, 231, 11, inWavetableLoadingTab);

	visimove(m_normalizeBtn, 155, 224, inWavetableLoadingTab);
	visimove(m_desawBtn, 39, 224, inWavetableLoadingTab);

	tabChanged(m_b->m_scroll);

	visimove(m_MatrixXBtn, 229, 8, inMatrixTab && m_MatrixXBtn->isVisible());
}


// Change the tab when the user scrolls over the tab buttons
void MicrowaveView::wheelEvent(QWheelEvent * me)
{
	if (me->x() <= 18 && me->y() >= 48 && me->y() <= 138)// If scroll over tab buttons
	{
		if (m_b->m_scroll != 6)
		{
			if (me->delta() < 0 && m_b->m_scroll != 5)
			{
				m_b->m_scroll += 1;
				updateScroll();
			}
			else if (me->delta() > 0 && m_b->m_scroll > 0)
			{
				m_b->m_scroll -= 1;
				updateScroll();
			}
		}
	}

	me->accept();
}


// Set the graph color depending on whether the oscillator being viewed currently is enabled
void MicrowaveView::setGraphEnabledColor(bool isEnabled)
{
	m_graph->setGraphColor(isEnabled ? QColor(121, 222, 239) : QColor(197, 197, 197));
	m_pal = QPalette();
	m_pal.setBrush(backgroundRole(), isEnabled ? PLUGIN_NAME::getIconPixmap("wavegraph") : PLUGIN_NAME::getIconPixmap("wavegraphdisabled"));
	m_graph->setPalette(m_pal);
}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::mainNumChanged()
{
	setGraphEnabledColor(m_b->m_enabled[m_b->m_mainNum.value()-1]->value());

	int mainNumValue = m_b->m_mainNum.value() - 1;

	m_morphKnob->setModel(m_b->m_morph[mainNumValue]);
	m_morphKnob->setMatrixLocation(1, 1, mainNumValue);

	m_rangeKnob->setModel(m_b->m_range[mainNumValue]);
	m_rangeKnob->setMatrixLocation(1, 2, mainNumValue);

	m_sampLenKnob->setModel(m_b->m_sampLen[mainNumValue]);

	m_modifyKnob->setModel(m_b->m_modify[mainNumValue]);
	m_modifyKnob->setMatrixLocation(1, 3, mainNumValue);

	m_morphMaxKnob->setModel(m_b->m_morphMax[mainNumValue]);

	m_unisonVoicesKnob->setModel(m_b->m_unisonVoices[mainNumValue]);
	m_unisonVoicesKnob->setMatrixLocation(1, 8, mainNumValue);

	m_unisonDetuneKnob->setModel(m_b->m_unisonDetune[mainNumValue]);
	m_unisonDetuneKnob->setMatrixLocation(1, 9, mainNumValue);

	m_unisonMorphKnob->setModel(m_b->m_unisonMorph[mainNumValue]);
	m_unisonMorphKnob->setMatrixLocation(1, 10, mainNumValue);

	m_unisonModifyKnob->setModel(m_b->m_unisonModify[mainNumValue]);
	m_unisonModifyKnob->setMatrixLocation(1, 11, mainNumValue);

	m_detuneKnob->setModel(m_b->m_detune[mainNumValue]);
	m_detuneKnob->setMatrixLocation(1, 4, mainNumValue);

	m_modifyModeBox-> setModel(m_b-> m_modifyMode[mainNumValue]);

	m_phaseKnob->setModel(m_b->m_phase[mainNumValue]);
	m_phaseKnob->setMatrixLocation(1, 5, mainNumValue);

	m_phaseRandKnob->setModel(m_b->m_phaseRand[mainNumValue]);

	m_volKnob->setModel(m_b->m_vol[mainNumValue]);
	m_volKnob->setMatrixLocation(1, 6, mainNumValue);

	m_panKnob->setModel(m_b->m_pan[mainNumValue]);
	m_panKnob->setMatrixLocation(1, 7, mainNumValue);

	m_enabledToggle->setModel(m_b->m_enabled[mainNumValue]);

	m_mutedToggle->setModel(m_b->m_muted[mainNumValue]);

	m_keytrackingToggle->setModel(m_b->m_keytracking[mainNumValue]);

	m_tempoKnob->setModel(m_b->m_tempo[mainNumValue]);

	m_interpolateToggle->setModel(m_b->m_interpolate[mainNumValue]);

}


// Trades out the GUI elements when switching between oscillators, and adjusts graph length when needed
void MicrowaveView::subNumChanged()
{
	m_b->m_graph.setLength(m_b->m_subSampLen[m_b->m_subNum.value()-1]->value());
	m_b->m_graph.setSamples(m_b->m_storedsubs[m_b->m_subNum.value()-1].data());
	setGraphEnabledColor(m_b->m_subEnabled[m_b->m_subNum.value()-1]->value());

	int subNumValue = m_b->m_subNum.value() - 1;

	m_subVolKnob->setModel(m_b->m_subVol[subNumValue]);
	m_subVolKnob->setMatrixLocation(2, 3, subNumValue);

	m_subEnabledToggle->setModel(m_b->m_subEnabled[subNumValue]);

	m_subPhaseKnob->setModel(m_b->m_subPhase[subNumValue]);
	m_subPhaseKnob->setMatrixLocation(2, 2, subNumValue);

	m_subPhaseRandKnob->setModel(m_b->m_subPhaseRand[subNumValue]);

	m_subDetuneKnob->setModel(m_b->m_subDetune[subNumValue]);
	m_subDetuneKnob->setMatrixLocation(2, 1, subNumValue);

	m_subMutedToggle->setModel(m_b->m_subMuted[subNumValue]);

	m_subKeytrackToggle->setModel(m_b->m_subKeytrack[subNumValue]);

	m_subSampLenKnob->setModel(m_b->m_subSampLen[subNumValue]);
	m_subSampLenKnob->setMatrixLocation(2, 5, subNumValue);

	m_subNoiseToggle->setModel(m_b->m_subNoise[subNumValue]);

	m_subPanningKnob->setModel(m_b->m_subPanning[subNumValue]);
	m_subPanningKnob->setMatrixLocation(2, 4, subNumValue);

	m_subTempoKnob->setModel(m_b->m_subTempo[subNumValue]);

	m_subRateLimitKnob->setModel(m_b->m_subRateLimit[subNumValue]);
	m_subRateLimitKnob->setMatrixLocation(2, 6, subNumValue);

	m_subUnisonNumKnob->setModel(m_b->m_subUnisonNum[subNumValue]);
	m_subUnisonNumKnob->setMatrixLocation(2, 7, subNumValue);

	m_subUnisonDetuneKnob->setModel(m_b->m_subUnisonDetune[subNumValue]);
	m_subUnisonDetuneKnob->setMatrixLocation(2, 8, subNumValue);

	m_subInterpolateToggle->setModel(m_b->m_subInterpolate[subNumValue]);

}


// Trades out the GUI elements when switching between oscillators
void MicrowaveView::sampNumChanged()
{
	setGraphEnabledColor(m_b->m_sampleEnabled[m_b->m_sampNum.value()-1]->value());

	for (int i = 0; i < 128; ++i)
	{
		m_b->m_graph.setSampleAt(i, m_b->m_sampGraphs[(m_b->m_sampNum.value()-1)*128+i]);
	}

	int sampNumValue = m_b->m_sampNum.value() - 1;

	m_sampleEnabledToggle->setModel(m_b->m_sampleEnabled[sampNumValue]);
	m_sampleGraphEnabledToggle->setModel(m_b->m_sampleGraphEnabled[sampNumValue]);
	m_sampleMutedToggle->setModel(m_b->m_sampleMuted[sampNumValue]);
	m_sampleKeytrackingToggle->setModel(m_b->m_sampleKeytracking[sampNumValue]);
	m_sampleLoopToggle->setModel(m_b->m_sampleLoop[sampNumValue]);

	m_sampleVolumeKnob->setModel(m_b->m_sampleVolume[sampNumValue]);
	m_sampleVolumeKnob->setMatrixLocation(3, 3, sampNumValue);

	m_samplePanningKnob->setModel(m_b->m_samplePanning[sampNumValue]);
	m_samplePanningKnob->setMatrixLocation(3, 4, sampNumValue);

	m_sampleDetuneKnob->setModel(m_b->m_sampleDetune[sampNumValue]);
	m_sampleDetuneKnob->setMatrixLocation(3, 1, sampNumValue);

	m_samplePhaseKnob->setModel(m_b->m_samplePhase[sampNumValue]);
	m_samplePhaseKnob->setMatrixLocation(3, 2, sampNumValue);

	m_samplePhaseRandKnob->setModel(m_b->m_samplePhaseRand[sampNumValue]);

	m_sampleStartKnob->setModel(m_b->m_sampleStart[sampNumValue]);

	m_sampleEndKnob->setModel(m_b->m_sampleEnd[sampNumValue]);

}


// Moves/changes the GUI around depending on the mod out section value
void MicrowaveView::modOutSecChanged(int i)
{
	int modScrollVal = (m_matrixScrollBar->value()) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	if (i-matrixDivide < 8 && i-matrixDivide >= 0 && i < 64)
	{
		m_temp1 = m_b->m_modOutSig[i]->value();
		switch (m_b->m_modOutSec[i]->value())
		{
			case 0:// None
			{
				m_modOutSigBox[i-matrixDivide]->hide();
				m_modOutSecNumBox[i-matrixDivide]->hide();
				break;
			}
			case 1:// Wavetable OSC
			{
				m_modOutSigBox[i-matrixDivide]->show();
				m_modOutSecNumBox[i-matrixDivide]->show();
				mainoscsignalsmodel(m_b->m_modOutSig[i])
				m_b->m_modOutSecNum[i]->setRange(1.f, 8.f, 1.f);
				break;
			}
			case 2:// Sub OSC
			{
				m_modOutSigBox[i-matrixDivide]->show();
				m_modOutSecNumBox[i-matrixDivide]->show();
				subsignalsmodel(m_b->m_modOutSig[i])
				m_b->m_modOutSecNum[i]->setRange(1.f, 64.f, 1.f);
				break;
			}
			case 3:// Sample OSC
			{
				m_modOutSigBox[i-matrixDivide]->show();
				m_modOutSecNumBox[i-matrixDivide]->show();
				samplesignalsmodel(m_b->m_modOutSig[i])
				m_b->m_modOutSecNum[i]->setRange(1.f, 8.f, 1.f);
				break;
			}
			case 4:// Matrix Parameters
			{
				m_modOutSigBox[i-matrixDivide]->show();
				m_modOutSecNumBox[i-matrixDivide]->show();
				matrixsignalsmodel(m_b->m_modOutSig[i])
				m_b->m_modOutSecNum[i]->setRange(1.f, 64.f, 1.f);
				break;
			}
			case 5:// Filter Input
			{
				m_modOutSigBox[i-matrixDivide]->show();
				m_modOutSecNumBox[i-matrixDivide]->hide();
				mod8model(m_b->m_modOutSig[i])
				break;
			}
			case 6:// Filter Parameters
			{
				m_modOutSigBox[i-matrixDivide]->show();
				m_modOutSecNumBox[i-matrixDivide]->show();
				filtersignalsmodel(m_b->m_modOutSig[i])
				m_b->m_modOutSecNum[i]->setRange(1.f, 8.f, 1.f);
				break;
			}
			case 7:// Macro
			{
				m_modOutSigBox[i-matrixDivide]->show();
				m_modOutSecNumBox[i-matrixDivide]->hide();
				matrixoutmodel(m_b->m_modOutSig[i])
				break;
			}
			default:
			{
				break;
			}
		}
		m_b->m_modOutSig[i]->setValue(m_temp1);
	}
}


// Moves/changes the GUI around depending on the Mod In Section value
void MicrowaveView::modInChanged(int i)
{
	int modScrollVal = (m_matrixScrollBar->value()) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	if (i-matrixDivide < 8 && i-matrixDivide >= 0 && i < 64)
	{
		switch (m_b->m_modIn[i]->value())
		{
			case 0:
			{
				m_modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 1:// Wavetable OSC
			{
				m_modInNumBox[i-matrixDivide]->show();
				m_b->m_modInNum[i]->setRange(1, 8, 1);
				break;
			}
			case 2:// Sub OSC
			{
				m_modInNumBox[i-matrixDivide]->show();
				m_b->m_modInNum[i]->setRange(1, 64, 1);
				break;
			}
			case 3:// Sample OSC
			{
				m_modInNumBox[i-matrixDivide]->show();
				m_b->m_modInNum[i]->setRange(1, 8, 1);
				break;
			}
			case 4:// Filter
			{
				m_modInNumBox[i-matrixDivide]->show();
				m_b->m_modInNum[i]->setRange(1, 8, 1);
				break;
			}
			case 5:// Velocity
			{
				m_modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 6:// Panning
			{
				m_modInNumBox[i-matrixDivide]->hide();
				break;
			}
			case 7:// Humanizer
			{
				m_modInNumBox[i-matrixDivide]->show();
				m_b->m_modInNum[i]->setRange(1, 8, 1);
				break;
			}
			case 8:// Macro
			{
				m_modInNumBox[i-matrixDivide]->show();
				m_b->m_modInNum[i]->setRange(1, 18, 1);
				break;
			}
		}
	}
}


// Moves/changes the GUI around depending on the Mod In Section value
void MicrowaveView::modIn2Changed(int i)
{
	int modScrollVal = (m_matrixScrollBar->value()) / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	if (i-matrixDivide < 8 && i-matrixDivide >= 0 && i < 64)
	{
		switch (m_b->m_modIn2[i]->value())
		{
			case 0:
			{
				m_modInNumBox2[i-matrixDivide]->hide();
				break;
			}
			case 1:// Wavetable OSC
			{
				m_modInNumBox2[i-matrixDivide]->show();
				m_b->m_modInNum2[i]->setRange(1, 8, 1);
				break;
			}
			case 2:// Sub OSC
			{
				m_modInNumBox2[i-matrixDivide]->show();
				m_b->m_modInNum2[i]->setRange(1, 64, 1);
				break;
			}
			case 3:// Sample OSC
			{
				m_modInNumBox2[i-matrixDivide]->show();
				m_b->m_modInNum2[i]->setRange(1, 8, 1);
				break;
			}
			case 4:// Filter
			{
				m_modInNumBox2[i-matrixDivide]->show();
				m_b->m_modInNum2[i]->setRange(1, 8, 1);
				break;
			}
			case 5:// Velocity
			{
				m_modInNumBox2[i-matrixDivide]->hide();
				break;
			}
			case 6:// Panning
			{
				m_modInNumBox2[i-matrixDivide]->hide();
				break;
			}
			case 7:// Humanizer
			{
				m_modInNumBox2[i-matrixDivide]->show();
				m_b->m_modInNum2[i]->setRange(1, 8, 1);
				break;
			}
			case 8:// Macro
			{
				m_modInNumBox2[i-matrixDivide]->show();
				m_b->m_modInNum2[i]->setRange(1, 18, 1);
				break;
			}
		}
	}
}




// Does what is necessary when the user visits a new tab
void MicrowaveView::tabChanged(int tabnum)
{
	updateBackground();

	if (tabnum != 3)
	{
		m_MatrixXBtn->hide();
	}

	if (tabnum != 0)
	{
		m_tab1Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab1"));
		m_tab1Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab1"));
	}
	else
	{
		m_tab1Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab1_active"));
		m_tab1Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab1_active"));
	}

	if (tabnum != 1)
	{
		m_tab2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab2"));
		m_tab2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab2"));
	}
	else
	{
		m_tab2Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab2_active"));
		m_tab2Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab2_active"));
	}

	if (tabnum != 2)
	{
		m_tab3Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab3"));
		m_tab3Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab3"));
	}
	else
	{
		m_tab3Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab3_active"));
		m_tab3Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab3_active"));
	}

	if (tabnum != 3)
	{
		m_tab4Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab4"));
		m_tab4Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab4"));
	}
	else
	{
		m_tab4Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab4_active"));
		m_tab4Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab4_active"));
	}

	if (tabnum != 4)
	{
		m_tab5Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab5"));
		m_tab5Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab5"));
	}
	else
	{
		m_tab5Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab5_active"));
		m_tab5Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab5_active"));
	}

	if (tabnum != 5)
	{
		m_tab6Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab6"));
		m_tab6Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab6"));
	}
	else
	{
		m_tab6Btn->setActiveGraphic(PLUGIN_NAME::getIconPixmap("tab6_active"));
		m_tab6Btn->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("tab6_active"));
	}
}



void MicrowaveView::updateBackground()
{
	bool m_mainFlipped = m_b->m_mainFlipped.value();
	bool m_subFlipped = m_b->m_subFlipped.value();

	switch ((int)m_b->m_scroll)
	{
		case 0:// Wavetable
		{
			m_b->m_graph.setLength(204);// Graph is 204 pixels long
			mainNumChanged();

			if (!m_mainFlipped)
			{
				m_pal.setBrush(backgroundRole(), m_tab1ArtworkImg.copy());
			}
			else
			{
				m_pal.setBrush(backgroundRole(), m_tab1FlippedArtworkImg.copy());
			}

			setPalette(m_pal);
			break;
		}
		case 1:// Sub
		{
			subNumChanged();// Graph length is set in here

			if (!m_subFlipped)
			{
				m_pal.setBrush(backgroundRole(), m_tab2ArtworkImg.copy());
			}
			else
			{
				m_pal.setBrush(backgroundRole(), m_tab2FlippedArtworkImg.copy());
			}

			setPalette(m_pal);
			break;
		}
		case 2:// Sample
		{
			m_b->m_graph.setLength(128);
			sampNumChanged();

			m_pal.setBrush(backgroundRole(), m_tab3ArtworkImg.copy());
			setPalette(m_pal);
			break;
		}
		case 3:// Matrix
		{
			m_pal.setBrush(backgroundRole(), m_tab4ArtworkImg.copy());
			setPalette(m_pal);
			break;
		}
		case 4:// Effect
		{
			m_pal.setBrush(backgroundRole(), m_tab5ArtworkImg.copy());
			setPalette(m_pal);
			break;
		}
		case 5:// Miscellaneous
		{
			m_pal.setBrush(backgroundRole(), m_tab6ArtworkImg.copy());
			setPalette(m_pal);
			break;
		}
		case 6:// Wavetable Loading
		{
			m_pal.setBrush(backgroundRole(), m_tab7ArtworkImg.copy());
			setPalette(m_pal);
			break;
		}
	}
}


// Change visualizer volume knob visibility when visualize LED is toggled
void MicrowaveView::visualizeToggled(bool value)
{
	m_visvolKnob->setVisible(m_b->m_visualize.value());
}


// V Buttons that change the graph V
void MicrowaveView::sinWaveClicked()
{
	m_graph->model()->setWaveToSine();
	Engine::getSong()->setModified();
}


void MicrowaveView::triangleWaveClicked()
{
	m_graph->model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}


void MicrowaveView::sawWaveClicked()
{
	m_graph->model()->setWaveToSaw();
	Engine::getSong()->setModified();
}


void MicrowaveView::sqrWaveClicked()
{
	m_graph->model()->setWaveToSquare();
	Engine::getSong()->setModified();
}


void MicrowaveView::noiseWaveClicked()
{
	m_graph->model()->setWaveToNoise();
	Engine::getSong()->setModified();
}


void MicrowaveView::usrWaveClicked()
{
	QString fileName = m_graph->model()->setWaveToUser();
	ToolTip::add(m_usrWaveBtn, fileName);
	Engine::getSong()->setModified();
}


void MicrowaveView::smoothClicked()
{
	m_graph->model()->smooth();
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
	castModel<Microwave>()->m_scroll = 0;
	updateScroll();
}


void MicrowaveView::MatrixXBtnClicked()
{
	castModel<Microwave>()->m_scroll = m_tabWhenSendingToMatrix;
	updateScroll();
}


// Normalize wavetable
void MicrowaveView::normalizeClicked()
{
	int oscilNum = m_b->m_mainNum.value() - 1;

	for (int i = 0; i < 256; ++i)
	{
		float highestVolume = 0;
		for (int j = 0; j < 2048; ++j)
		{
			highestVolume = abs(m_b->m_storedwaveforms[oscilNum][(i*2048)+j]) > highestVolume
				? abs(m_b->m_storedwaveforms[oscilNum][(i*2048)+j])
				: highestVolume;
		}
		if (highestVolume)
		{
			float multiplierThing = 1.f / highestVolume;
			for (int j = 0; j < 2048; ++j)
			{
				m_b->m_storedwaveforms[oscilNum][(i*2048)+j] *= multiplierThing;
			}
		}
	}

	Engine::getSong()->setModified();

	m_b->m_updateWavetable[oscilNum] = true;

	m_b->fillMainOsc(oscilNum, m_b->m_interpolate[oscilNum]->value());
}


// Bend wavetable waveforms so the ends match
void MicrowaveView::desawClicked()
{
	int oscilNum = m_b->m_mainNum.value() - 1;

	float start;
	float end;
	for (int j = 0; j < 256; ++j)
	{
		start = -m_b->m_storedwaveforms[oscilNum][j*2048];
		end = -m_b->m_storedwaveforms[oscilNum][j*2048+2047];
		for (int i = 0; i < 2048; ++i)
		{
			m_b->m_storedwaveforms[oscilNum][j*2048+i] += (i/2048.f)*end + ((2048.f-i)/2048.f)*start;
		}
	}

	Engine::getSong()->setModified();

	m_b->m_updateWavetable[oscilNum] = true;

	m_b->fillMainOsc(oscilNum, m_b->m_interpolate[oscilNum]->value());
}



void MicrowaveView::modUpClicked(int i)
{
	int modScrollVal = m_matrixScrollBar->value() / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;
	if (i+matrixDivide > 0)
	{
		castModel<Microwave>()->switchMatrixSections(i+matrixDivide, i+matrixDivide - 1);
	}
}


void MicrowaveView::modDownClicked(int i)
{
	int modScrollVal = m_matrixScrollBar->value() / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;
	if (i+matrixDivide < 63)
	{
		castModel<Microwave>()->switchMatrixSections(i+matrixDivide, i+matrixDivide + 1);
	}
}


// Different from Microwave::modEnabledChanged.
// Changes maximum value of the Matrix scroll bar.
void MicrowaveView::modEnabledChanged()
{
	m_matrixScrollBar->setRange(0, qBound(100.f, m_b->m_maxModEnabled * 100.f, 6232.f) + 30.f);
}


// Move window to location of Matrix input
void MicrowaveView::i1Clicked(int i)
{
	int modScrollVal = m_matrixScrollBar->value() / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	switch (m_b->m_modIn[i+matrixDivide]->value())
	{
		case 1:
		{
			m_b->m_scroll = 0;
			m_b->m_mainNum.setValue(m_b->m_modInNum[i+matrixDivide]->value());
			break;
		}
		case 2:
		{
			m_b->m_scroll = 1;
			m_b->m_subNum.setValue(m_b->m_modInNum[i+matrixDivide]->value());
			break;
		}
		case 3:
		{
			m_b->m_scroll = 2;
			m_b->m_sampNum.setValue(m_b->m_modInNum[i+matrixDivide]->value());
			break;
		}
		case 4:
		{
			m_b->m_scroll = 3;
			m_effectScrollBar->setValue((m_b->m_modInNum[i+matrixDivide]->value() - 1) * 100.f);
			break;
		}
		case 8:
		{
			m_b->m_scroll = 5;
			break;
		}
	}

	updateScroll();
}


// Move window to location of Matrix input
void MicrowaveView::i2Clicked(int i)
{
	int modScrollVal = m_matrixScrollBar->value() / 100.f * 115.f;
	int matrixDivide = modScrollVal / 460 * 4;

	switch (m_b->m_modIn2[i+matrixDivide]->value())
	{
		case 1:
		{
			m_b->m_scroll = 0;
			m_b->m_mainNum.setValue(m_b->m_modInNum2[i+matrixDivide]->value());
			break;
		}
		case 2:
		{
			m_b->m_scroll = 1;
			m_b->m_subNum.setValue(m_b->m_modInNum2[i+matrixDivide]->value());
			break;
		}
		case 3:
		{
			m_b->m_scroll = 2;
			m_b->m_sampNum.setValue(m_b->m_modInNum2[i+matrixDivide]->value());
			break;
		}
		case 4:
		{
			m_b->m_scroll = 3;
			m_effectScrollBar->setValue((m_b->m_modInNum2[i+matrixDivide]->value() - 1) * 100.f);
			break;
		}
		case 8:
		{
			m_b->m_scroll = 5;
			break;
		}
	}

	updateScroll();
}


void MicrowaveView::tabBtnClicked(int i)
{
	castModel<Microwave>()->m_scroll = i;
	updateScroll();
}


// Run when user sends a knob to the Matrix through its right click menu
void MicrowaveView::sendToMatrixAsOutput(int loc1, int loc2, int loc3)
{
	int m_matrixLocation = m_b->m_maxModEnabled;

	if (m_matrixLocation < 64)
	{
		m_b->m_modEnabled[m_matrixLocation]->setValue(true);
		m_b->m_modOutSec[m_matrixLocation]->setValue(loc1);
		m_b->m_modOutSig[m_matrixLocation]->setValue(loc2);
		m_b->m_modOutSecNum[m_matrixLocation]->setValue(loc3 + 1);
	}

	m_tabWhenSendingToMatrix = m_b->m_scroll;

	m_MatrixXBtn->show();

	m_b->m_scroll = 3;
	updateScroll();

	m_matrixScrollBar->setValue(100 * m_matrixLocation);
}


// Make knob green and connect to its corresponding Matrix Amount knob
void MicrowaveView::switchToMatrixKnob(MicrowaveKnob * theKnob, int loc1, int loc2, int loc3)
{
	for (int i = 0; i < 64; ++i)
	{
		if (m_b->m_modOutSec[i]->value() == loc1 && m_b->m_modOutSig[i]->value() == loc2 && m_b->m_modOutSecNum[i]->value() - 1 == loc3)
		{
			theKnob->setModel(m_b->m_modInAmnt[i]);
			theKnob->setarcColor(QColor(23,94,40));
			theKnob->setlineColor(QColor(51,248,99));
			theKnob->setInnerColor(QColor(32,112,50));
			break;
		}
	}
}


// Lets user change the tooltip of Macro knobs
void MicrowaveView::setMacroTooltip(MicrowaveKnob * theKnob, int which)
{
	bool ok;
	QString new_val;

	new_val = QInputDialog::getText(theKnob, tr("Set new Tooltip"), tr("Please enter a new Tooltip for Macro %1:").arg(which + 1), QLineEdit::Normal, m_b->m_macroTooltips[which], &ok);

	if (ok)
	{
		m_b->m_macroTooltips[which] = new_val;
		ToolTip::add(theKnob, tr("Macro %1: ").arg(which + 1) + new_val);
	}
}


// Lets user change the color of Macro knobs
void MicrowaveView::chooseMacroColor(MicrowaveKnob * theKnob, int which)
{
	QColor new_color = QColorDialog::getColor(QColor(m_b->m_macroColors[which][0], m_b->m_macroColors[which][1], m_b->m_macroColors[which][2]));
	if (! new_color.isValid())
	{
		return;
	}

	m_b->m_macroColors[which][0] = new_color.red();
	m_b->m_macroColors[which][1] = new_color.green();
	m_b->m_macroColors[which][2] = new_color.blue();
	refreshMacroColor(theKnob, which);
}


void MicrowaveView::setMacroColortoDefault(MicrowaveKnob * theKnob, int which)
{
	m_b->m_macroColors[which][0] = 102;
	m_b->m_macroColors[which][1] = 198;
	m_b->m_macroColors[which][2] = 199;
	refreshMacroColor(theKnob, which);
}


void MicrowaveView::refreshMacroColor(MicrowaveKnob * theKnob, int which)
{
	int red = m_b->m_macroColors[which][0];
	int green = m_b->m_macroColors[which][1];
	int blue = m_b->m_macroColors[which][2];

	theKnob->setarcColor(QColor(red*0.4, green*0.4, blue*0.4));
	theKnob->setlineColor(QColor(red, green, blue));
	theKnob->setInnerColor(QColor(red*0.5, green*0.5, blue*0.5));
}



// Calls MicrowaveView::openWavetableFile when the wavetable opening button is clicked.
void MicrowaveView::openWavetableFileBtnClicked()
{
	if (m_b->m_scroll != 6)
	{
		m_b->m_scroll = 6;
		updateScroll();
	}
	else
	{
		chooseWavetableFile();
	}
}


void MicrowaveView::chooseWavetableFile()
{
	SampleBuffer * m_sampleBuffer = new SampleBuffer;
	m_wavetableFileName = m_sampleBuffer->openAndSetWaveformFile();
	sharedObject::unref(m_sampleBuffer);
}


void MicrowaveView::confirmWavetableLoadClicked()
{
	openWavetableFile();
}


// All of the code and algorithms for loading wavetables from samples.
void MicrowaveView::openWavetableFile(QString fileName)
{
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();

	if (fileName.isEmpty())
	{
		if (m_wavetableFileName.isEmpty())
		{
			chooseWavetableFile();
		}
		fileName = m_wavetableFileName;
	}

	SampleBuffer * m_sampleBuffer = new SampleBuffer;

	m_sampleBuffer->setAudioFile(fileName);

	int filelength = m_sampleBuffer->sampleLength();
	int oscilNum = m_b->m_mainNum.value() - 1;
	int algorithm = m_b->m_loadMode.value();
	int channel = m_b->m_loadChnl.value();

	if (!fileName.isEmpty())
	{
		m_sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		switch (algorithm)
		{
			case 0:// Lock waveform edges to zero crossings
			{
				// Clear wavetable
				for (int i = 0; i < STOREDMAINARRAYLEN; ++i)
				{
					m_b->m_storedwaveforms[oscilNum][i] = 0;
				}

				bool above = m_sampleBuffer->userWaveSample(1.f / lengthOfSample, channel) > 0;
				float currentValue = 0;
				std::vector<float> zeroCrossings;
				float previousPoint = 0;

				// Find zero crossings, and store differences between them in a vector.
				for (int i = 0; i < lengthOfSample; ++i)
				{
					currentValue = m_sampleBuffer->userWaveSample(i / lengthOfSample, channel);
					if ((above && currentValue <= 0) || (!above && currentValue > 0))
					{
						above = !above;
						zeroCrossings.push_back(i-previousPoint);
						previousPoint = i;
					}
				}

				// Quit if the sample is too short
				if (zeroCrossings.size() < 3)
				{
					break;
				}

				std::vector<float> betterZC;
				float now = 0;
				float actualnow = 0;

				// Find and list chosen zero crossings
				for (int i = 0; i < zeroCrossings.size() - 1; ++i)
				{
					now += zeroCrossings[i];
					actualnow += zeroCrossings[i];
					if (abs(STOREDMAINWAVELEN / 2.f - now) < abs(STOREDMAINWAVELEN / 2.f - (now + zeroCrossings[i+1])))
					{
						betterZC.push_back(actualnow);
						now = 0;
					}
				}

				float start;
				float end;

				float lasti = 0;
				float lastj = 0;

				bool breakify = false;

				// Take gathered information and cram it into the waveforms.
				for (int i = 0; i < betterZC.size() - 1; ++i)
				{
					start = betterZC[i];
					end = betterZC[i+1];

					lasti = i;

					for (int j = 0; j < m_b->m_sampLen[oscilNum]->value(); ++j)
					{
						lastj = j;

						if (j + (i * m_b->m_sampLen[oscilNum]->value()) >= STOREDMAINARRAYLEN)
						{
							breakify = true;
							break;
						}
						m_b->m_storedwaveforms[oscilNum][j + (i * (int)m_b->m_sampLen[oscilNum]->value())] = m_sampleBuffer->userWaveSample(((j / m_b->m_sampLen[oscilNum]->value()) * (end - start) + start) / lengthOfSample, channel);
					}

					if (breakify) { break; }
				}

				m_b->m_morphMax[oscilNum]->setValue(lasti + (lastj / m_b->m_sampLen[oscilNum]->value()));
				m_b->morphMaxChanged(oscilNum);

				break;
			}
			case 1:// Load sample without changes
			{
				for (int i = 0; i < STOREDMAINARRAYLEN; ++i)
				{
					if (i <= lengthOfSample * 2.f)
					{
						m_b->m_storedwaveforms[oscilNum][i] = m_sampleBuffer->userWaveSample((i / lengthOfSample) / 2.f, channel);
					}
					else// Replace everything else with silence if sample isn't long enough
					{
						m_b->m_morphMax[oscilNum]->setValue(i / m_b->m_sampLen[oscilNum]->value());
						m_b->morphMaxChanged(oscilNum);
						for (int j = i; j < STOREDMAINARRAYLEN; ++j) { m_b->m_storedwaveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 2:// For loading wavetable files
			{
				for (int i = 0; i < STOREDMAINARRAYLEN; ++i)
				{
					if (i <= lengthOfSample)
					{
						m_b->m_storedwaveforms[oscilNum][i] = m_sampleBuffer->userWaveSample(i / lengthOfSample, channel);
					}
					else
					{
						m_b->m_morphMax[oscilNum]->setValue(i / m_b->m_sampLen[oscilNum]->value());
						m_b->morphMaxChanged(oscilNum);
						for (int j = i; j < STOREDMAINARRAYLEN; ++j) { m_b->m_storedwaveforms[oscilNum][j] = 0.f; }
						break;
					}
				}
				break;
			}
			case 3:// Autocorrelation
			{
				// This uses a method called autocorrelation to detect the pitch, most of which was taken from the Instructables website.
				// It can get a few Hz off (especially at higher frequencies), so I also compare it with the zero crossings to see if I can get it even more accurate.

				// Estimate pitch using autocorrelation:

				float checkLength = qMin(4000.f, lengthOfSample);// 4000 samples should be long enough to be able to accurately detect most frequencies this way

				float threshold = -1;
				float combined = 0;
				float oldcombined;
				int stage = 0;

				float period = 0;
				for (int i = 0; i < checkLength; ++i)
				{
					oldcombined = combined;
					combined = 0;
					for (int k = 0; k < checkLength - i; ++k)
					{
						combined += (m_sampleBuffer->userWaveSample(k/lengthOfSample, channel) *
							m_sampleBuffer->userWaveSample((k+i)/lengthOfSample, channel) + 1) * 0.5f - 0.5f;
					}

					if (stage == 2 && combined - oldcombined <= 0)
					{
						stage = 3;
						period = i;
					}

					if (stage == 1 && combined > threshold && combined - oldcombined > 0)
					{
						stage = 2;
					}

					if (!i)
					{
						threshold = combined * 0.5f;
						stage = 1;
					}
				}

				if (!period)
				{
					break;
				}

				// Now see if the zero crossings can aid in getting the pitch even more accurate:

				// Note:  If the zero crossings give a result very close to the autocorrelation, then it is likely to be more accurate.
				// Otherwise, the zero crossing result is probably very inaccurate (common with complex sounds) and is ignored.

				std::vector<float> crossings;
				crossings.push_back(0);
				std::vector<float> crossingsDif;
				bool above = (m_sampleBuffer->userWaveSample(1.f / lengthOfSample, channel) > 0);

				for (int i = 0; i < checkLength; ++i)
				{
					if ((m_sampleBuffer->userWaveSample(i / lengthOfSample, channel) > 0) != above)
					{
						above = !above;
						if (above)
						{
							crossingsDif.push_back(i - crossings[crossings.size() - 1]);
							crossings.push_back(i);
						}
					}
				}

				crossings.erase(crossings.begin());

				if (crossingsDif.size() >= 3)
				{
					float crossingsMean = std::accumulate(crossingsDif.begin(), crossingsDif.end(), 0.f) / crossingsDif.size();
					std::vector<float> crossingsToRemove;
					for (int i = 0; i < crossingsDif.size(); ++i)
					{
						if (crossingsDif[i] < crossingsMean)
						{
							crossingsToRemove.push_back(i);
						}
					}
					for (int i = crossingsToRemove.size() - 1; i >= 0; --i)
					{
						crossingsDif.erase(crossingsDif.begin() + crossingsToRemove[i]);
					}
					if (crossingsDif.size() >= 2)
					{
						float crossingsMedian = crossingsDif[int(crossingsDif.size() / 2.f)];
						if (abs(period - crossingsMedian) < 5.f + period / 100.f)
						{
							period = crossingsMedian;
						}
					}
				}

				for (int i = 0; i < STOREDMAINARRAYLEN; ++i)
				{
					m_b->m_storedwaveforms[oscilNum][i] = m_sampleBuffer->userWaveSample(((i/2048.f)*period)/lengthOfSample, channel);
				}

				m_b->m_morphMax[oscilNum]->setValue(254);
				m_b->morphMaxChanged(oscilNum);

				break;
			}
		}

		m_sampleBuffer->dataUnlock();

		// Interpolate and store wavetable
		m_b->m_updateWavetable[oscilNum] = true;
		m_b->fillMainOsc(oscilNum, m_b->m_interpolate[oscilNum]->value());
	}

	sharedObject::unref(m_sampleBuffer);
}



// Calls MicrowaveView::openSampleFile when the sample opening button is clicked.
void MicrowaveView::openSampleFileBtnClicked()
{
	openSampleFile();
}


// Loads sample for sample oscillator
void MicrowaveView::openSampleFile()
{
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();
	int oscilNum = m_b->m_sampNum.value() - 1;

	SampleBuffer * m_sampleBuffer = new SampleBuffer;
	QString fileName = m_sampleBuffer->openAndSetWaveformFile();
	int filelength = m_sampleBuffer->sampleLength();

	if (fileName.isEmpty() == false)
	{
		m_sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		m_b->m_samples[oscilNum][0].clear();
		m_b->m_samples[oscilNum][1].clear();

		for (int i = 0; i < lengthOfSample; ++i)
		{
			m_b->m_samples[oscilNum][0].push_back(m_sampleBuffer->userWaveSample(i / lengthOfSample, 0));
			m_b->m_samples[oscilNum][1].push_back(m_sampleBuffer->userWaveSample(i / lengthOfSample, 1));
		}
		m_sampleBuffer->dataUnlock();
	}
	sharedObject::unref(m_sampleBuffer);
}


// When a sample file is dragged onto Microwave
void MicrowaveView::dropEvent(QDropEvent * de)
{
	QString type = StringPairDrag::decodeKey(de);
	QString value = StringPairDrag::decodeValue(de);
	if (type == "samplefile")
	{
		m_wavetableFileName = value;
		m_b->m_scroll = 6;
		updateScroll();
		de->accept();
		return;
	}
	de->ignore();
}


// Accept sample dropping into Microwave
void MicrowaveView::dragEnterEvent(QDragEnterEvent * dee)
{
	if (dee->mimeData()->hasFormat(StringPairDrag::mimeType()))
	{
		QString txt = dee->mimeData()->data(
						StringPairDrag::mimeType());
		if (txt.section(':', 0, 0) == "samplefile")
		{
			dee->acceptProposedAction();
		}
		else
		{
			dee->ignore();
		}
	}
	else
	{
		dee->ignore();
	}
}





//============//
//== MSYNTH ==//
//============//

// Initializes mSynth (when a new note is played).  Clone all of the arrays storing the knob values so they can be changed by modulation.
mSynth::mSynth(NotePlayHandle * m_nph,
	float * m_morphArr, float * m_rangeArr, float * m_modifyArr, int * m_modifyModeArr, float * m_volArr, float * m_panArr,
	float * m_detuneArr, float * m_phaseArr, float * m_phaseRandArr, bool * m_enabledArr, bool * m_mutedArr,
	float * m_sampLenArr, float * m_morphMaxArr, float * m_unisonVoicesArr, float * m_unisonDetuneArr, float * m_unisonMorphArr,
	float * m_unisonModifyArr, bool * m_keytrackingArr, float * m_tempoArr, bool * m_interpolateArr,
	bool * m_subEnabledArr, bool * m_subMutedArr, bool * m_subKeytrackArr, bool * m_subNoiseArr, float * m_subVolArr,
	float * m_subPanningArr, float * m_subDetuneArr, float * m_subPhaseArr, float * m_subPhaseRandArr,
	float * m_subSampLenArr, float * m_subTempoArr, float * m_subRateLimitArr, float * m_subUnisonNumArr, float * m_subUnisonDetuneArr, bool * m_subInterpolateArr,
	bool * m_sampleEnabledArr, bool * m_sampleMutedArr, bool * m_sampleKeytrackingArr, bool * m_sampleGraphEnabledArr, bool * m_sampleLoopArr,
	float * m_sampleVolumeArr, float * m_samplePanningArr, float * m_sampleDetuneArr, float * m_samplePhaseArr, float * m_samplePhaseRandArr, float * m_sampleStartArr, float * m_sampleEndArr,
	int * m_modInArr, int * m_modInNumArr, float * m_modInAmntArr, float * m_modInCurveArr, int * m_modIn2Arr, int * m_modInNum2Arr, float * m_modInAmnt2Arr, float * m_modInCurve2Arr,
	int * m_modOutSecArr, int * m_modOutSigArr, int * m_modOutSecNumArr, bool * m_modEnabledArr, int * m_modCombineTypeArr, bool * m_modTypeArr, bool * m_modType2Arr,
	float * m_filtCutoffArr, float * m_filtResoArr, float * m_filtGainArr, int * m_filtTypeArr, int * m_filtSlopeArr,
	float * m_filtInVolArr, float * m_filtOutVolArr, float * m_filtWetDryArr, float * m_filtBalArr,
	float * m_filtSatuArr, float * m_filtFeedbackArr, float * m_filtDetuneArr, bool * m_filtEnabledArr, bool * m_filtMutedArr, bool * m_filtKeytrackingArr,
	float * m_macroArr,
	std::vector<float> (&m_samples)[8][2]) :

	m_nph(m_nph)
{
	memcpy(m_morph, m_morphArr, sizeof(float) * 8);
	memcpy(m_range, m_rangeArr, sizeof(float) * 8);
	memcpy(m_modify, m_modifyArr, sizeof(float) * 8);
	memcpy(m_modifyMode, m_modifyModeArr, sizeof(int) * 8);
	memcpy(m_vol, m_volArr, sizeof(int) * 8);
	memcpy(m_pan, m_panArr, sizeof(float) * 8);
	memcpy(m_detune, m_detuneArr, sizeof(float) * 8);
	memcpy(m_phase, m_phaseArr, sizeof(int) * 8);
	memcpy(m_phaseRand, m_phaseRandArr, sizeof(int) * 8);
	memcpy(m_enabled, m_enabledArr, sizeof(bool) * 8);
	memcpy(m_muted, m_mutedArr, sizeof(bool) * 8);
	memcpy(m_sampLen, m_sampLenArr, sizeof(float) * 8);
	memcpy(m_morphMax, m_morphMaxArr, sizeof(float) * 8);
	memcpy(m_unisonVoices, m_unisonVoicesArr, sizeof(float) * 8);
	memcpy(m_unisonDetune, m_unisonDetuneArr, sizeof(float) * 8);
	memcpy(m_unisonMorph, m_unisonMorphArr, sizeof(float) * 8);
	memcpy(m_unisonModify, m_unisonModifyArr, sizeof(float) * 8);
	memcpy(m_keytracking, m_keytrackingArr, sizeof(bool) * 8);
	memcpy(m_tempo, m_tempoArr, sizeof(float) * 8);
	memcpy(m_interpolate, m_interpolateArr, sizeof(bool) * 8);

	memcpy(m_subEnabled, m_subEnabledArr, sizeof(bool) * 64);
	memcpy(m_subMuted, m_subMutedArr, sizeof(bool) * 64);
	memcpy(m_subKeytrack, m_subKeytrackArr, sizeof(bool) * 64);
	memcpy(m_subNoise, m_subNoiseArr, sizeof(bool) * 64);
	memcpy(m_subVol, m_subVolArr, sizeof(float) * 64);
	memcpy(m_subPanning, m_subPanningArr, sizeof(float) * 64);
	memcpy(m_subDetune, m_subDetuneArr, sizeof(float) * 64);
	memcpy(m_subPhase, m_subPhaseArr, sizeof(float) * 64);
	memcpy(m_subPhaseRand, m_subPhaseRandArr, sizeof(float) * 64);
	memcpy(m_subSampLen, m_subSampLenArr, sizeof(float) * 64);
	memcpy(m_subTempo, m_subTempoArr, sizeof(float) * 64);
	memcpy(m_subRateLimit, m_subRateLimitArr, sizeof(float) * 64);
	memcpy(m_subUnisonNum, m_subUnisonNumArr, sizeof(float) * 64);
	memcpy(m_subUnisonDetune, m_subUnisonDetuneArr, sizeof(float) * 64);
	memcpy(m_subInterpolate, m_subInterpolateArr, sizeof(bool) * 64);

	memcpy(m_sampleEnabled, m_sampleEnabledArr, sizeof(bool) * 8);
	memcpy(m_sampleMuted, m_sampleMutedArr, sizeof(bool) * 8);
	memcpy(m_sampleKeytracking, m_sampleKeytrackingArr, sizeof(bool) * 8);
	memcpy(m_sampleGraphEnabled, m_sampleGraphEnabledArr, sizeof(bool) * 8);
	memcpy(m_sampleLoop, m_sampleLoopArr, sizeof(bool) * 8);
	memcpy(m_sampleVolume, m_sampleVolumeArr, sizeof(float) * 8);
	memcpy(m_samplePanning, m_samplePanningArr, sizeof(float) * 8);
	memcpy(m_sampleDetune, m_sampleDetuneArr, sizeof(float) * 8);
	memcpy(m_samplePhase, m_samplePhaseArr, sizeof(float) * 8);
	memcpy(m_samplePhaseRand, m_samplePhaseRandArr, sizeof(float) * 8);
	memcpy(m_sampleStart, m_sampleStartArr, sizeof(float) * 8);
	memcpy(m_sampleEnd, m_sampleEndArr, sizeof(float) * 8);

	memcpy(m_modIn, m_modInArr, sizeof(int) * 64);
	memcpy(m_modInNum, m_modInNumArr, sizeof(int) * 64);
	memcpy(m_modInAmnt, m_modInAmntArr, sizeof(float) * 64);
	memcpy(m_modInCurve, m_modInCurveArr, sizeof(float) * 64);
	memcpy(m_modIn2, m_modIn2Arr, sizeof(int) * 64);
	memcpy(m_modInNum2, m_modInNum2Arr, sizeof(int) * 64);
	memcpy(m_modInAmnt2, m_modInAmnt2Arr, sizeof(float) * 64);
	memcpy(m_modInCurve2, m_modInCurve2Arr, sizeof(float) * 64);
	memcpy(m_modOutSec, m_modOutSecArr, sizeof(int) * 64);
	memcpy(m_modOutSig, m_modOutSigArr, sizeof(int) * 64);
	memcpy(m_modOutSecNum, m_modOutSecNumArr, sizeof(int) * 64);
	memcpy(m_modEnabled, m_modEnabledArr, sizeof(bool) * 64);
	memcpy(m_modCombineType, m_modCombineTypeArr, sizeof(int) * 64);
	memcpy(m_modType, m_modTypeArr, sizeof(bool) * 64);
	memcpy(m_modType2, m_modType2Arr, sizeof(bool) * 64);

	memcpy(m_filtCutoff, m_filtCutoffArr, sizeof(float) * 8);
	memcpy(m_filtReso, m_filtResoArr, sizeof(float) * 8);
	memcpy(m_filtGain, m_filtGainArr, sizeof(float) * 8);
	memcpy(m_filtType, m_filtTypeArr, sizeof(int) * 8);
	memcpy(m_filtSlope, m_filtSlopeArr, sizeof(int) * 8);
	memcpy(m_filtInVol, m_filtInVolArr, sizeof(float) * 8);
	memcpy(m_filtOutVol, m_filtOutVolArr, sizeof(float) * 8);
	memcpy(m_filtWetDry, m_filtWetDryArr, sizeof(float) * 8);
	memcpy(m_filtBal, m_filtBalArr, sizeof(float) * 8);
	memcpy(m_filtSatu, m_filtSatuArr, sizeof(float) * 8);
	memcpy(m_filtFeedback, m_filtFeedbackArr, sizeof(float) * 8);
	memcpy(m_filtDetune, m_filtDetuneArr, sizeof(float) * 8);
	memcpy(m_filtEnabled, m_filtEnabledArr, sizeof(bool) * 8);
	memcpy(m_filtMuted, m_filtMutedArr, sizeof(bool) * 8);
	memcpy(m_filtKeytracking, m_filtKeytrackingArr, sizeof(bool) * 8);

	memcpy(m_macro, m_macroArr, sizeof(float) * 18);



	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 32; ++j)
		{
			// Randomize the phases of all of the waveforms
			m_sample_realindex[i][j] = int(((fastRandf(m_sampLen[i] * WAVERATIO)) *
				(m_phaseRand[i] * 0.01f))) % int(m_sampLen[i] * WAVERATIO);
		}

		m_sample_sampleindex[i] = fmod(fastRandf(m_samples[i][0].size()) * (m_samplePhaseRand[i] * 0.01f),
					(m_samples[i][0].size() *m_sampleEnd[i]) - (m_samples[i][0].size() * m_sampleStart[i])) +
					(m_samples[i][0].size() * m_sampleStart[i]);
		m_humanizer[i] = (rand() / float(RAND_MAX)) * 2 - 1;// Generate humanizer values at the beginning of every note
	}

	for (int i = 0; i < 64; ++i)
	{
		for (int l = 0; l < 32; ++l)
		{
			m_sample_subindex[i][l] = int(((fastRandf(m_subSampLen[i] * WAVERATIO) -
				(m_subSampLen[i] * WAVERATIO * 0.5f)) * (m_subPhaseRand[i] * 0.01f))) %
				int(m_subSampLen[i] * WAVERATIO);
			m_subNoiseDirection[i][l] = 1;
		}
	}

	m_noteDuration = -1;

	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < m_unisonVoices[i]; ++j)
		{
			m_unisonDetuneAmounts[i][j] = ((rand()/float(RAND_MAX))*2.f)-1;
		}
	}

	for (int i = 0; i < 64; ++i)
	{
		for (int j = 0; j < m_subUnisonNum[i]; ++j)
		{
			m_subUnisonDetuneAmounts[i][j] = ((rand()/float(RAND_MAX))*2.f)-1;
		}
	}

}


mSynth::~mSynth()
{
}


// This is the part that puts everything together and calculates an audio output.
void mSynth::nextStringSample(sampleFrame &outputSample, std::vector<float> (&m_waveforms)[8], std::vector<float> (&m_subs)[64],
				std::vector<float> (&m_samples)[8][2], float * m_sampGraphs, int m_maxMainEnabled, int m_maxSubEnabled,
				int m_maxSampleEnabled, int m_maxFiltEnabled, int m_maxModEnabled, int sample_rate, Microwave * m_mwc,
				bool m_removeDC, std::vector<float> (&m_storedsubs)[64])
{
	++m_noteDuration;

	//============//
	//== MATRIX ==//
	//============//

	m_numberToReset = 0;
	for (int l = 0; l < m_maxModEnabled; ++l)// m_maxModEnabled keeps this from looping 64 times every m_sample, saving a lot of CPU
	{
		if (m_modEnabled[l])
		{
			// Find where the inputs are coming from
			switch (m_modIn[l])
			{
				case 0:
				{
					m_curModVal[0] = 0;
					m_curModVal[1] = 0;
					break;
				}
				case 1:// Wavetable
				{
					if (m_modType[l])// If envelope
					{
						m_curModVal[0] = m_lastMainOscEnvVal[m_modInNum[l]-1][0];
						m_curModVal[1] = m_lastMainOscEnvVal[m_modInNum[l]-1][1];
					}
					else
					{
						m_curModVal[0] = m_lastMainOscVal[m_modInNum[l]-1][0];
						m_curModVal[1] = m_lastMainOscVal[m_modInNum[l]-1][1];
					}
					break;
				}
				case 2:// Sub
				{
					if (m_modType[l])// If envelope
					{
						m_curModVal[0] = m_lastSubEnvVal[m_modInNum[l]-1][0];
						m_curModVal[1] = m_lastSubEnvVal[m_modInNum[l]-1][1];
					}
					else
					{
						m_curModVal[0] = m_lastSubVal[m_modInNum[l]-1][0];
						m_curModVal[1] = m_lastSubVal[m_modInNum[l]-1][1];
					}
					break;
				}
				case 3:// Sample
				{
					if (m_modType[l])// If envelope
					{
						m_curModVal[0] = m_lastSampleEnvVal[m_modInNum[l]-1][0];
						m_curModVal[1] = m_lastSampleEnvVal[m_modInNum[l]-1][1];
					}
					else
					{
						m_curModVal[0] = m_lastSampleVal[m_modInNum[l]-1][0];
						m_curModVal[1] = m_lastSampleVal[m_modInNum[l]-1][1];
					}
					break;
				}
				case 4:// Filter
				{
					m_curModVal[0] = m_filtModOutputs[m_modInNum[l]-1][0];
					m_curModVal[1] = m_filtModOutputs[m_modInNum[l]-1][1];
					break;
				}
				case 5:// Velocity
				{
					m_curModVal[0] = (m_nph->getVolume() * 0.01f)-1;
					m_curModVal[1] = m_curModVal[0];
					break;
				}
				case 6:// Panning
				{
					m_curModVal[0] = (m_nph->getPanning() * 0.01f);
					m_curModVal[1] = m_curModVal[0];
					break;
				}
				case 7:// Humanizer
				{
					m_curModVal[0] = m_humanizer[m_modInNum[l]-1];
					m_curModVal[1] = m_humanizer[m_modInNum[l]-1];
					break;
				}
				case 8:// Macro
				{
					m_curModVal[0] = m_macro[m_modInNum[l]-1] * 0.01f;
					m_curModVal[1] = m_macro[m_modInNum[l]-1] * 0.01f;
					break;
				}
				default:
				{
					switch (m_modCombineType[l])
					{
						case 0:// Add Bidirectional
						{
							m_curModVal[0] = 0;
							m_curModVal[1] = 0;
							break;
						}
						case 1:// Multiply Bidirectional
						{
							m_curModVal[0] = 0;
							m_curModVal[1] = 0;
							break;
						}
						case 2:// Add Unidirectional
						{
							m_curModVal[0] = -1;
							m_curModVal[1] = -1;
							break;
						}
						case 3:// Multiply Unidirectional
						{
							m_curModVal[0] = -1;
							m_curModVal[1] = -1;
							break;
						}
					}
				}
			}
			switch (m_modIn2[l])
			{
				case 0:
				{
					m_curModVal2[0] = 0;
					m_curModVal2[1] = 0;
					break;
				}
				case 1:// Wavetable
				{
					if (m_modType2[l])// If envelope
					{
						m_curModVal2[0] = m_lastMainOscEnvVal[m_modInNum2[l]-1][0];
						m_curModVal2[1] = m_lastMainOscEnvVal[m_modInNum2[l]-1][1];
					}
					else
					{
						m_curModVal2[0] = m_lastMainOscVal[m_modInNum2[l]-1][0];
						m_curModVal2[1] = m_lastMainOscVal[m_modInNum2[l]-1][1];
					}
					break;
				}
				case 2:// Sub
				{
					if (m_modType2[l])// If envelope
					{
						m_curModVal2[0] = m_lastSubEnvVal[m_modInNum2[l]-1][0];
						m_curModVal2[1] = m_lastSubEnvVal[m_modInNum2[l]-1][1];
					}
					else
					{
						m_curModVal2[0] = m_lastSubVal[m_modInNum2[l]-1][0];
						m_curModVal2[1] = m_lastSubVal[m_modInNum2[l]-1][1];
					}
					break;
				}
				case 3:// Sample
				{
					if (m_modType[l])// If envelope
					{
						m_curModVal2[0] = m_lastSampleEnvVal[m_modInNum2[l]-1][0];
						m_curModVal2[1] = m_lastSampleEnvVal[m_modInNum2[l]-1][1];
					}
					else
					{
						m_curModVal2[0] = m_lastSampleVal[m_modInNum2[l]-1][0];
						m_curModVal2[1] = m_lastSampleVal[m_modInNum2[l]-1][1];
					}
					break;
				}
				case 4:// Filter
				{
					m_curModVal2[0] = m_filtModOutputs[m_modInNum2[l]-1][0];
					m_curModVal2[1] = m_filtModOutputs[m_modInNum2[l]-1][1];
				}
				case 5:// Velocity
				{
					m_curModVal2[0] = (m_nph->getVolume() * 0.01f)-1;
					m_curModVal2[1] = m_curModVal2[0];
					break;
				}
				case 6:// Panning
				{
					m_curModVal2[0] = (m_nph->getPanning() * 0.01f);
					m_curModVal2[1] = m_curModVal2[0];
					break;
				}
				case 7:// Humanizer
				{
					m_curModVal2[0] = m_humanizer[m_modInNum2[l]-1];
					m_curModVal2[1] = m_humanizer[m_modInNum2[l]-1];
					break;
				}
				case 8:// Macro
				{
					m_curModVal2[0] = m_macro[m_modInNum2[l]-1] * 0.01f;
					m_curModVal2[1] = m_macro[m_modInNum2[l]-1] * 0.01f;
					break;
				}
				default:
				{
					switch (m_modCombineType[l])
					{
						case 0:// Add Bidirectional
						{
							m_curModVal[0] = 0;
							m_curModVal[1] = 0;
							break;
						}
						case 1:// Multiply Bidirectional
						{
							m_curModVal[0] = 0;
							m_curModVal[1] = 0;
							break;
						}
						case 2:// Add Unidirectional
						{
							m_curModVal[0] = -1;
							m_curModVal[1] = -1;
							break;
						}
						case 3:// Multiply Unidirectional
						{
							m_curModVal[0] = -1;
							m_curModVal[1] = -1;
							break;
						}
					}
				}
			}

			// Apply Amount knob.
			m_curModVal[0]  *= m_modInAmnt[l]  * 0.01f;
			m_curModVal[1]  *= m_modInAmnt[l]  * 0.01f;
			// Since it's uncommon to use both inputs, the "if" statement helps performance.
			if (m_curModVal2[0]) { m_curModVal2[0] *= m_modInAmnt2[l] * 0.01f; }
			if (m_curModVal2[1]) { m_curModVal2[1] *= m_modInAmnt2[l] * 0.01f; }

			// Calculate curve and direction
			if (m_modCombineType[l] <= 1)// Bidirectional
			{
				// The "if" statement is there so unnecessary CPU isn't spent (pow is very expensive) if the curve knob isn't being used.
				if (m_modInCurve[l] != 100.f)
				{
					// Move to a scale of 0 to 1 (from -1 to 1) and then apply the curve.
					m_temp1 = 1.f / (m_modInCurve[l] * 0.01f);
					m_curModValCurve[0] = (m_curModVal[0] <= -1 || m_curModVal[0] >= 1)
						? (m_curModVal[0] + 1) * 0.5f
						: pow((m_curModVal[0] + 1) * 0.5f, m_temp1);
					m_curModValCurve[1] = (m_curModVal[1] <= -1 || m_curModVal[1] >= 1)
						? (m_curModVal[1] + 1) * 0.5f
						: pow((m_curModVal[1] + 1) * 0.5f, m_temp1);
				}
				else
				{
					m_curModValCurve[0] = (m_curModVal[0] + 1) * 0.5f;
					m_curModValCurve[1] = (m_curModVal[1] + 1) * 0.5f;
				}
				if (m_modInCurve2[l] != 100.f)
				{
					m_temp1 = 1.f / (m_modInCurve2[l] * 0.01f);
					m_curModVal2Curve[0] = (m_curModVal2[0] <= -1 || m_curModVal2[0] >= 1)
						? (m_curModVal2[0] + 1) * 0.5f
						: pow((m_curModVal2[0] + 1) * 0.5f, m_temp1);
					m_curModVal2Curve[1] = (m_curModVal2[1] <= -1 || m_curModVal2[1] >= 1)
						? (m_curModVal2[1] + 1) * 0.5f
						: pow((m_curModVal2[1] + 1) * 0.5f, m_temp1);
				}
				else
				{
					m_curModVal2Curve[0] = (m_curModVal2[0] + 1) * 0.5f;
					m_curModVal2Curve[1] = (m_curModVal2[1] + 1) * 0.5f;
				}
			}
			else// Unidirectional
			{
				if (m_modInCurve[l] != 100.f)
				{
					m_temp1 = m_modInCurve[l] * 0.01f;
					m_temp2 = m_curModVal[0] < 0 ? -1 : 1;
					m_temp3 = m_modInAmnt[l] * 0.01;
					m_curModValCurve[0] = ((m_curModVal[0] <= -1 || m_curModVal[0] >= 1)
						? m_curModVal[0]
						: pow(abs(m_curModVal[0]), 1.f / m_temp1) * m_temp2) + m_temp3;
					m_curModValCurve[1] = ((m_curModVal[1] <= -1 || m_curModVal[1] >= 1)
						? m_curModVal[1]
						: pow(abs(m_curModVal[1]), 1.f / m_temp1) * m_temp2) + m_temp3;
				}
				else
				{
					m_temp1 = m_modInAmnt[l] * 0.01;
					m_curModValCurve[0] = m_curModVal[0] + m_temp1;
					m_curModValCurve[1] = m_curModVal[1] + m_temp1;
				}
				if (m_modInCurve2[l] != 100.f)
				{
					m_temp1 = m_modInCurve2[l] * 0.01f;
					m_temp2 = m_curModVal2[0] < 0 ? -1 : 1;
					m_temp3 = m_modInAmnt2[l] * 0.01;
					m_curModVal2Curve[0] = ((m_curModVal2[0] <= -1 || m_curModVal2[0] >= 1)
						? m_curModVal2[0]
						: pow(abs(m_curModVal2[0]), 1.f / m_temp1) * m_temp2) + m_temp3;
					m_curModVal2Curve[1] = ((m_curModVal2[1] <= -1 || m_curModVal2[1] >= 1)
						? m_curModVal2[1]
						: pow(abs(m_curModVal2[0]), 1.f / m_temp1) * m_temp2) + m_temp3;
				}
				else
				{
					m_temp1 = m_modInAmnt2[l] * 0.01;
					m_curModVal2Curve[0] = m_curModVal2[0] + m_temp1;
					m_curModVal2Curve[1] = m_curModVal2[1] + m_temp1;
				}
			}

			switch (m_modCombineType[l])
			{
				case 0:// Add Bidirectional
				{
					m_comboModVal[0] = m_curModValCurve[0] + m_curModVal2Curve[0] - 1;
					m_comboModVal[1] = m_curModValCurve[1] + m_curModVal2Curve[1] - 1;
					break;
				}
				case 1:// Multiply Bidirectional
				{
					m_comboModVal[0] = (m_curModValCurve[0] * 2 - 1) * (m_curModVal2Curve[0] * 2 - 1);
					m_comboModVal[1] = (m_curModValCurve[1] * 2 - 1) * (m_curModVal2Curve[1] * 2 - 1);
					break;
				}
				case 2:// Add Unidirectional
				{
					m_comboModVal[0] = m_curModValCurve[0] + m_curModVal2Curve[0];
					m_comboModVal[1] = m_curModValCurve[1] + m_curModVal2Curve[1];
					break;
				}
				case 3:// Multiply Unidirectional
				{
					m_comboModVal[0] = m_curModValCurve[0] * m_curModVal2Curve[0];
					m_comboModVal[1] = m_curModValCurve[1] * m_curModVal2Curve[1];
					break;
				}
				default:
				{
					m_comboModVal[0] = 0;
					m_comboModVal[1] = 0;
				}
			}

			m_comboModValMono = (m_comboModVal[0] + m_comboModVal[1]) * 0.5f;

			// Send the calcluated value to the output.  Keep track of the changed values so they can be reset later on.
			switch (m_modOutSec[l])
			{
				case 0:
				{
					break;
				}
				case 1:// Wavetable Oscillator
				{
					switch (m_modOutSig[l])
					{
						case 0:
						{
							break;
						}
						case 1:// Send input to Morph
						{
							m_morph[m_modOutSecNum[l]-1] = qBound(0.f, m_morph[m_modOutSecNum[l]-1] +
								m_comboModValMono*m_morphMax[m_modOutSecNum[l]-1], m_morphMax[m_modOutSecNum[l]-1]);
							m_modValType[m_numberToReset] = 1;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 2:// Send input to Range
						{
							m_range[m_modOutSecNum[l]-1] = qMax(0.f, m_range[m_modOutSecNum[l]-1] + m_comboModValMono * 16.f);
							m_modValType[m_numberToReset] = 2;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 3:// Send input to Modify
						{
							m_modify[m_modOutSecNum[l]-1] = qMax(0.f, m_modify[m_modOutSecNum[l]-1] + m_comboModValMono * 2048.f);
							m_modValType[m_numberToReset] = 3;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 4:// Send input to Pitch/Detune
						{
							m_detune[m_modOutSecNum[l]-1] = m_detune[m_modOutSecNum[l]-1] + m_comboModValMono * 4800.f;
							m_modValType[m_numberToReset] = 7;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 5:// Send input to Phase
						{
							m_phase[m_modOutSecNum[l]-1] = m_phase[m_modOutSecNum[l]-1] + m_comboModValMono * 8.f;
							m_modValType[m_numberToReset] = 8;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 6:// Send input to Volume
						{
							m_vol[m_modOutSecNum[l]-1] = qMax(0.f, m_vol[m_modOutSecNum[l]-1] + m_comboModValMono * 100.f);
							m_modValType[m_numberToReset] = 5;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 7:// Send input to Panning
						{
							m_pan[m_modOutSecNum[l]-1] = qMax(0.f, m_pan[m_modOutSecNum[l]-1] + m_comboModValMono * 200.f);
							m_modValType[m_numberToReset] = 6;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 8:// Send input to Unison Voice Number
						{
							m_unisonVoices[m_modOutSecNum[l]-1] = qMax(0.f, m_unisonVoices[m_modOutSecNum[l]-1] + m_comboModValMono * 32.f);
							m_modValType[m_numberToReset] = 14;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 9:// Send input to Unison Detune
						{
							m_unisonDetune[m_modOutSecNum[l]-1] = qMax(0.f, m_unisonDetune[m_modOutSecNum[l]-1] + m_comboModValMono * 2000.f);
							m_modValType[m_numberToReset] = 15;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 10:// Send input to Unison Morph
						{
							m_unisonMorph[m_modOutSecNum[l]-1] = qBound(0.f, m_unisonMorph[m_modOutSecNum[l]-1] +
								m_comboModValMono*m_morphMax[m_modOutSecNum[l]-1], m_morphMax[m_modOutSecNum[l]-1]);
							m_modValType[m_numberToReset] = 16;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 11:// Send input to Unison Modify
						{
							m_unisonModify[m_modOutSecNum[l]-1] = qMax(0.f, m_unisonModify[m_modOutSecNum[l]-1] + m_comboModValMono * 2048.f);
							m_modValType[m_numberToReset] = 17;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
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
					switch (m_modOutSig[l])
					{
						case 0:
						{
							break;
						}
						case 1:// Send input to Pitch/Detune
						{
							m_subDetune[m_modOutSecNum[l]-1] = m_subDetune[m_modOutSecNum[l]-1] + m_comboModValMono * 4800.f;
							m_modValType[m_numberToReset] = 36;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 2:// Send input to Phase
						{
							m_subPhase[m_modOutSecNum[l]-1] = m_subPhase[m_modOutSecNum[l]-1] + m_comboModValMono * 8.f;
							m_modValType[m_numberToReset] = 37;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 3:// Send input to Volume
						{
							m_subVol[m_modOutSecNum[l]-1] = qMax(0.f, m_subVol[m_modOutSecNum[l]-1] + m_comboModValMono * 100.f);
							m_modValType[m_numberToReset] = 34;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 4:// Send input to Panning
						{
							m_subPanning[m_modOutSecNum[l]-1] = qMax(0.f, m_subPanning[m_modOutSecNum[l]-1] + m_comboModValMono * 200.f);
							m_modValType[m_numberToReset] = 35;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 5:// Send input to Sample Length
						{
							m_subSampLen[m_modOutSecNum[l]-1] = qMax(1, int(m_subSampLen[m_modOutSecNum[l]-1] + m_comboModValMono * STOREDSUBWAVELEN));
							m_modValType[m_numberToReset] = 39;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 6:// Send input to Rate Limit
						{
							m_subRateLimit[m_modOutSecNum[l]-1] = qMax(0.f, m_subRateLimit[m_modOutSecNum[l]-1] + m_comboModValMono * 2.f);
							m_modValType[m_numberToReset] = 41;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 7:// Send input to Unison Voice Number
						{
							m_subUnisonNum[m_modOutSecNum[l]-1] = qMax(1.f, m_subUnisonNum[m_modOutSecNum[l]-1] + m_comboModValMono * 32.f);
							m_modValType[m_numberToReset] = 42;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 8:// Send input to Unison Detune
						{
							m_subUnisonDetune[m_modOutSecNum[l]-1] = qMax(0.f, m_subUnisonDetune[m_modOutSecNum[l]-1] + m_comboModValMono * 2000.f);
							m_modValType[m_numberToReset] = 43;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
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
					switch (m_modOutSig[l])
					{
						case 0:
						{
							break;
						}
						case 1:// Send input to Pitch/Detune
						{
							m_sampleDetune[m_modOutSecNum[l]-1] = m_sampleDetune[m_modOutSecNum[l]-1] + m_comboModValMono * 4800.f;
							m_modValType[m_numberToReset] = 67;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 2:// Send input to Phase
						{
							m_samplePhase[m_modOutSecNum[l]-1] = m_samplePhase[m_modOutSecNum[l]-1] + m_comboModValMono * 8.f;
							m_modValType[m_numberToReset] = 68;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 3:// Send input to Volume
						{
							m_sampleVolume[m_modOutSecNum[l]-1] = qMax(0.f, m_sampleVolume[m_modOutSecNum[l]-1] + m_comboModValMono * 100.f);
							m_modValType[m_numberToReset] = 65;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 4:// Send input to Panning
						{
							m_samplePanning[m_modOutSecNum[l]-1] = qBound(-100.f, m_samplePanning[m_modOutSecNum[l]-1] + m_comboModValMono * 100.f, 100.f);
							m_modValType[m_numberToReset] = 66;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
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
					switch (m_modOutSig[l])
					{
						case 0:
						{
							break;
						}
						case 1:// Mod In Amount
						{
							m_modInAmnt[m_modOutSecNum[l]-1] = m_modInAmnt[m_modOutSecNum[l]-1] + m_comboModValMono*100.f;
							m_modValType[m_numberToReset] = 92;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 2:// Mod In Curve
						{
							m_modInCurve[m_modOutSecNum[l]-1] = qMax(0.f, m_modInCurve[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 93;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 3:// Secondary Mod In Amount
						{
							m_modInAmnt2[m_modOutSecNum[l]-1] = m_modInAmnt2[m_modOutSecNum[l]-1] + m_comboModValMono*100.f;
							m_modValType[m_numberToReset] = 95;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 4:// Secondary Mod In Curve
						{
							m_modInCurve2[m_modOutSecNum[l]-1] = qMax(0.f, m_modInCurve2[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 96;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 5:// Mod In
						{
							m_modIn[m_modOutSecNum[l]-1] = qMax(0.f, m_modIn[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 90;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 6:// Mod In Num
						{
							m_modInNum[m_modOutSecNum[l]-1] = qMax(0.f, m_modInNum[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 91;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 7:// Secondary Mod In
						{
							m_modIn2[m_modOutSecNum[l]-1] = qMax(0.f, m_modIn2[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 94;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 8:// Secondary Mod In Num
						{
							m_modInNum2[m_modOutSecNum[l]-1] = qMax(0.f, m_modInNum2[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 95;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 9:// Mod Out Sec
						{
							m_modOutSec[m_modOutSecNum[l]-1] = qMax(0.f, m_modOutSec[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 98;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 10:// Mod Out Sig
						{
							m_modOutSig[m_modOutSecNum[l]-1] = qMax(0.f, m_modOutSig[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 99;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 11:// Mod Out Sec Num
						{
							m_modOutSecNum[m_modOutSecNum[l]-1] = qMax(0.f, m_modOutSecNum[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 100;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
					}
					break;
				}
				case 5:// Filter Input
				{
					m_filtInputs[m_modOutSig[l]][0] += m_curModVal[0];
					m_filtInputs[m_modOutSig[l]][1] += m_curModVal[1];
					break;
				}
				case 6:// Filter Parameters
				{
					switch (m_modOutSig[l])
					{
						case 0:// None
						{
							break;
						}
						case 1:// Cutoff Frequency
						{
							m_filtCutoff[m_modOutSecNum[l]-1] = qBound(20.f, m_filtCutoff[m_modOutSecNum[l]-1] + m_comboModValMono*19980.f, 21999.f);
							m_modValType[m_numberToReset] = 120;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 2:// Resonance
						{
							m_filtReso[m_modOutSecNum[l]-1] = qMax(0.0001f, m_filtReso[m_modOutSecNum[l]-1] + m_comboModValMono*16.f);
							m_modValType[m_numberToReset] = 121;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 3:// dbGain
						{
							m_filtGain[m_modOutSecNum[l]-1] = m_filtGain[m_modOutSecNum[l]-1] + m_comboModValMono*64.f;
							m_modValType[m_numberToReset] = 122;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 4:// Filter Type
						{
							m_filtType[m_modOutSecNum[l]-1] = qMax(0, int(m_filtType[m_modOutSecNum[l]-1] + m_comboModValMono*7.f));
							m_modValType[m_numberToReset] = 123;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 5:// Filter Slope
						{
							m_filtSlope[m_modOutSecNum[l]-1] = qMax(0, int(m_filtSlope[m_modOutSecNum[l]-1] + m_comboModValMono*8.f));
							m_modValType[m_numberToReset] = 124;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 6:// Input Volume
						{
							m_filtInVol[m_modOutSecNum[l]-1] = qMax(0.f, m_filtInVol[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 125;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 7:// Output Volume
						{
							m_filtOutVol[m_modOutSecNum[l]-1] = qMax(0.f, m_filtOutVol[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 126;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 8:// Wet/Dry
						{
							m_filtWetDry[m_modOutSecNum[l]-1] = qBound(0.f, m_filtWetDry[m_modOutSecNum[l]-1] + m_comboModValMono*100.f, 100.f);
							m_modValType[m_numberToReset] = 127;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 9:// Balance/Panning
						{
							m_filtBal[m_modOutSecNum[l]-1] = qBound(-100.f, m_filtBal[m_modOutSecNum[l]-1] + m_comboModValMono*200.f, 100.f);
							m_modValType[m_numberToReset] = 128;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 10:// Saturation
						{
							m_filtSatu[m_modOutSecNum[l]-1] = qMax(0.f, m_filtSatu[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 129;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 11:// Feedback
						{
							m_filtFeedback[m_modOutSecNum[l]-1] = qMax(0.f, m_filtFeedback[m_modOutSecNum[l]-1] + m_comboModValMono*100.f);
							m_modValType[m_numberToReset] = 130;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
						case 12:// Detune
						{
							m_filtDetune[m_modOutSecNum[l]-1] = m_filtDetune[m_modOutSecNum[l]-1] + m_comboModValMono*4800.f;
							m_modValType[m_numberToReset] = 131;
							m_modValNum[m_numberToReset] = m_modOutSecNum[l]-1;
							++m_numberToReset;
							break;
						}
					}
					break;
				}
				case 7:// Macro
				{
					m_macro[m_modOutSig[l]] = m_macro[m_modOutSig[l]] + m_comboModValMono * 200.f;
					m_modValType[m_numberToReset] = 150;
					m_modValNum[m_numberToReset] = m_modOutSig[l];
					++m_numberToReset;
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

	for (int l = 0; l < m_maxFiltEnabled; ++l)
	{
		if (m_filtEnabled[l])
		{
			m_temp1 = m_filtInVol[l] * 0.01f;
			m_filtInputs[l][0] *= m_temp1;
			m_filtInputs[l][1] *= m_temp1;

			// Calculate the required size of the delay buffer
			if (m_filtKeytracking[l])
			{
				m_temp1 = round(sample_rate / detuneWithCents(m_nph->frequency(), m_filtDetune[l]));
			}
			else
			{
				m_temp1 = round(sample_rate / detuneWithCents(440.f, m_filtDetune[l]));
			}

			// Set feedback delay length depending on feedback detune and keytracking
			if (m_filtDelayBuf[l][0].size() < m_temp1)
			{
				m_filtDelayBuf[l][0].resize(m_temp1);
				m_filtDelayBuf[l][1].resize(m_temp1);
			}

			// Grab the next sample in the delay buffer
			++m_filtFeedbackLoc[l];
			if (m_filtFeedbackLoc[l] > m_temp1 - 1)
			{
				m_filtFeedbackLoc[l] = 0;
			}
			m_filtInputs[l][0] += m_filtDelayBuf[l][0].at(m_filtFeedbackLoc[l]);
			m_filtInputs[l][1] += m_filtDelayBuf[l][1].at(m_filtFeedbackLoc[l]);

			m_cutoff = m_filtCutoff[l];
			m_mode = m_filtType[l];
			m_reso = m_filtReso[l];
			m_dbgain = m_filtGain[l];
			m_Fs = sample_rate;
			m_w0 = D_2PI * m_cutoff / m_Fs;

			if (m_mode == 8)
			{
				m_f = 2 * m_cutoff / m_Fs;
				m_k = 3.6*m_f - 1.6*m_f*m_f - 1;
				m_p = (m_k+1)*0.5f;
				m_scale = pow(D_E, (1-m_p)*1.386249f);
				m_r = m_reso*m_scale;
			}

			// m is the slope number.  So if m = 3, then a 12 db filter is going from a 36 db slope to a 48 db slope, for example.
			for (int m = 0; m < m_filtSlope[l] + 1; ++m)
			{
				if (m)
				{
					m_filtInputs[l][0] = m_filtOutputs[l][0];
					m_filtInputs[l][1] = m_filtOutputs[l][1];
				}

				int formulaType = 1;
				if (m_mode <= 7)
				{
					// Calculate filter coefficients
					switch (m_mode)
					{
						case 0:// LP
						{
							m_temp1 = cos(m_w0);
							m_alpha = sin(m_w0) / (2*m_reso);
							m_b0 = (1 - m_temp1) * 0.5f;
							m_b1 = 1 - m_temp1;
							m_b2 = m_b0;
							m_a0 = 1 + m_alpha;
							m_a1 = -2 * m_temp1;
							m_a2 = 1 - m_alpha;
							break;
						}
						case 1:// HP
						{
							m_temp1 = cos(m_w0);
							m_alpha = sin(m_w0) / (2*m_reso);
							m_b0 = (1 + m_temp1) * 0.5f;
							m_b1 = -(1 + m_temp1);
							m_b2 = m_b0;
							m_a0 = 1 + m_alpha;
							m_a1 = -2 * m_temp1;
							m_a2 = 1 - m_alpha;
							break;
						}
						case 2:// BP
						{
							m_temp2 = sin(m_w0);
							m_alpha = m_temp2*sinh(log(2) * 0.5 * m_reso * m_w0/m_temp2);
							m_b0 = m_alpha;
							m_b1 = 0;
							m_b2 = -m_alpha;
							m_a0 = 1 + m_alpha;
							m_a1 = -2*cos(m_w0);
							m_a2 = 1 - m_alpha;
							formulaType = 2;
							break;
						}
						case 3:// Low Shelf
						{
							m_temp1 = cos(m_w0);
							m_A = exp10(m_dbgain / 40);
							m_alpha = sin(m_w0)/2 * sqrt((m_A + 1/m_A)*(1/m_reso - 1) + 2);
							m_temp2 = 2*sqrt(m_A)*m_alpha;
							m_b0 = m_A*((m_A+1) - (m_A-1)*m_temp1 + m_temp2);
							m_b1 = 2*m_A*((m_A-1) - (m_A+1)*m_temp1        );
							m_b2 = m_A*((m_A+1) - (m_A-1)*m_temp1 - m_temp2);
							m_a0 = (m_A+1) + (m_A-1)*m_temp1 + m_temp2;
							m_a1 = -2*((m_A-1) + (m_A+1)*m_temp1        );
							m_a2 = (m_A+1) + (m_A-1)*m_temp1 - m_temp2;
							break;
						}
						case 4:// High Shelf
						{
							m_temp1 = cos(m_w0);
							m_A = exp10(m_dbgain / 40);
							m_alpha = sin(m_w0)/2 * sqrt((m_A + 1/m_A)*(1/m_reso - 1) + 2);
							m_temp2 = 2*sqrt(m_A)*m_alpha;
							m_b0 = m_A*((m_A+1) + (m_A-1)*m_temp1 + m_temp2);
							m_b1 = -2*m_A*((m_A-1) + (m_A+1)*m_temp1        );
							m_b2 = m_A*((m_A+1) + (m_A-1)*m_temp1 - m_temp2);
							m_a0 = (m_A+1) - (m_A-1)*m_temp1 + m_temp2;
							m_a1 = 2*((m_A-1) - (m_A+1)*m_temp1        );
							m_a2 = (m_A+1) - (m_A-1)*m_temp1 - m_temp2;
							break;
						}
						case 5:// Peak
						{
							m_temp1 = -2 * cos(m_w0);
							m_temp2 = sin(m_w0);
							m_alpha = m_temp2*sinh(log(2) * 0.5 * m_reso * m_w0/m_temp2);
							m_A = exp10(m_dbgain / 40);
							m_b0 = 1 + m_alpha*m_A;
							m_b1 = m_temp1;
							m_b2 = 1 - m_alpha*m_A;
							m_a0 = 1 + m_alpha/m_A;
							m_a1 = m_temp1;
							m_a2 = 1 - m_alpha/m_A;
							break;
						}
						case 6:// Notch
						{
							m_temp1 = -2 * cos(m_w0);
							m_temp2 = sin(m_w0);
							m_alpha = m_temp2*sinh(log(2) * 0.5 * m_reso * m_w0/m_temp2);
							m_b0 = 1;
							m_b1 = m_temp1;
							m_b2 = 1;
							m_a0 = 1 + m_alpha;
							m_a1 = m_temp1;
							m_a2 = 1 - m_alpha;
							break;
						}
						case 7:// Allpass
						{
							m_temp1 = -2 * cos(m_w0);
							m_alpha = sin(m_w0) / (2*m_reso);
							m_b0 = 1 - m_alpha;
							m_b1 = m_temp1;
							m_b2 = 1 + m_alpha;
							m_a0 = m_b2;
							m_a1 = m_temp1;
							m_a2 = m_b0;
							formulaType = 3;
							break;
						}
					}

					// Translation of this monstrosity (case 1): y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]
					// See www.musicdsp.org/files/Audio-EQ-Cookbook.txt
					switch (formulaType)
					{
						case 1:// Normal
						{
							m_temp1 = m_b0/m_a0;
							m_temp2 = m_b1/m_a0;
							m_temp3 = m_b2/m_a0;
							m_temp4 = m_a1/m_a0;
							m_temp5 = m_a2/m_a0;
							m_filtPrevSampOut[l][m][0][0] = m_temp1*m_filtInputs[l][0] + m_temp2*m_filtPrevSampIn[l][m][1][0] +
								m_temp3*m_filtPrevSampIn[l][m][2][0] - m_temp4*m_filtPrevSampOut[l][m][1][0] - m_temp5*m_filtPrevSampOut[l][m][2][0];// Left ear
							m_filtPrevSampOut[l][m][0][1] = m_temp1*m_filtInputs[l][1] + m_temp2*m_filtPrevSampIn[l][m][1][1] +
								m_temp3*m_filtPrevSampIn[l][m][2][1] - m_temp4*m_filtPrevSampOut[l][m][1][1] - m_temp5*m_filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
						case 2:// Bandpass
						{
							m_temp1 = m_b0/m_a0;
							m_temp3 = m_b2/m_a0;
							m_temp4 = m_a1/m_a0;
							m_temp5 = m_a2/m_a0;
							m_filtPrevSampOut[l][m][0][0] = m_temp1*m_filtInputs[l][0] + m_temp3*m_filtPrevSampIn[l][m][2][0] -
								m_temp4*m_filtPrevSampOut[l][m][1][0] - m_temp5*m_filtPrevSampOut[l][m][2][0];// Left ear
							m_filtPrevSampOut[l][m][0][1] = m_temp1*m_filtInputs[l][1] + m_temp3*m_filtPrevSampIn[l][m][2][1] -
								m_temp4*m_filtPrevSampOut[l][m][1][1] - m_temp5*m_filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
						case 3:// Allpass
						{
							m_temp1 = m_b0/m_a0;
							m_temp2 = m_b1/m_a0;
							m_temp4 = m_a1/m_a0;
							m_temp5 = m_a2/m_a0;
							m_filtPrevSampOut[l][m][0][0] = m_temp1*m_filtInputs[l][0] + m_temp2*m_filtPrevSampIn[l][m][1][0] +
								m_filtPrevSampIn[l][m][2][0] - m_temp4*m_filtPrevSampOut[l][m][1][0] - m_temp5*m_filtPrevSampOut[l][m][2][0];// Left ear
							m_filtPrevSampOut[l][m][0][1] = m_temp1*m_filtInputs[l][1] + m_temp2*m_filtPrevSampIn[l][m][1][1] +
								m_filtPrevSampIn[l][m][2][1] - m_temp4*m_filtPrevSampOut[l][m][1][1] - m_temp5*m_filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
						case 4:// Notch
						{
							m_temp1 = 1.f/m_a0;
							m_temp2 = m_b1/m_a0;
							m_temp3 = m_temp1;
							m_temp4 = m_temp2;
							m_temp5 = m_a2/m_a0;
							m_filtPrevSampOut[l][m][0][0] = m_temp1*m_filtInputs[l][0] + m_temp2*m_filtPrevSampIn[l][m][1][0] +
								m_temp3*m_filtPrevSampIn[l][m][2][0] - m_temp4*m_filtPrevSampOut[l][m][1][0] - m_temp5*m_filtPrevSampOut[l][m][2][0];// Left ear
							m_filtPrevSampOut[l][m][0][1] = m_temp1*m_filtInputs[l][1] + m_temp2*m_filtPrevSampIn[l][m][1][1] +
								m_temp3*m_filtPrevSampIn[l][m][2][1] - m_temp4*m_filtPrevSampOut[l][m][1][1] - m_temp5*m_filtPrevSampOut[l][m][2][1];// Right ear
							break;
						}
					}

					m_filtOutputs[l][0] = m_filtPrevSampOut[l][m][0][0];
					m_filtOutputs[l][1] = m_filtPrevSampOut[l][m][0][1];

				}
				else if (m_mode == 8)
				{
					// Moog 24db Lowpass
					m_filtx[0] = m_filtInputs[l][0]-m_r*m_filty4[0];
					m_filtx[1] = m_filtInputs[l][1]-m_r*m_filty4[1];
					for (int i = 0; i < 2; ++i)
					{
						m_filty1[i]=m_filtx[i]*m_p + m_filtoldx[i]*m_p - m_k*m_filty1[i];
						m_filty2[i]=m_filty1[i]*m_p+m_filtoldy1[i]*m_p - m_k*m_filty2[i];
						m_filty3[i]=m_filty2[i]*m_p+m_filtoldy2[i]*m_p - m_k*m_filty3[i];
						m_filty4[i]=m_filty3[i]*m_p+m_filtoldy3[i]*m_p - m_k*m_filty4[i];
						m_filty4[i] = m_filty4[i] - m_filty4[i] * m_filty4[i] * m_filty4[i] / 6.f;
						m_filtoldx[i] = m_filtx[i];
						m_filtoldy1[i] = m_filty1[i];
						m_filtoldy2[i] = m_filty2[i];
						m_filtoldy3[i] = m_filty3[i];
					}
					m_filtOutputs[l][0] = m_filty4[0];
					m_filtOutputs[l][1] = m_filty4[1];
				}

				// Balance knob wet
				if (m_filtBal[l])
				{
					m_filtOutputs[l][0] *= m_filtBal[l] > 0
						? (100.f - m_filtBal[l]) * 0.01f
						: 1.f;
					m_filtOutputs[l][1] *= m_filtBal[l] < 0
						? (100.f + m_filtBal[l]) * 0.01f
						: 1.f;
				}

				// Wet
				if (m_filtWetDry[l] != 100)
				{
					m_temp1 = m_filtWetDry[l] * 0.01f;
					m_filtOutputs[l][0] *= m_temp1;
					m_filtOutputs[l][1] *= m_temp1;
				}

				// Balance knob dry
				if (m_filtBal[l])
				{
					m_filtOutputs[l][0] += (1.f - (m_filtBal[l] > 0 ? (100.f - m_filtBal[l]) * 0.01f : 1.f)) * m_filtInputs[l][0];
					m_filtOutputs[l][1] += (1.f - (m_filtBal[l] < 0 ? (100.f + m_filtBal[l]) * 0.01f : 1.f)) * m_filtInputs[l][1];
				}

				// Dry
				m_temp1 = 1.f - (m_filtWetDry[l] * 0.01f);
				m_filtOutputs[l][0] += m_temp1 * m_filtInputs[l][0];
				m_filtOutputs[l][1] += m_temp1 * m_filtInputs[l][1];

				// Send back past samples
				m_filtPrevSampIn[l][m][4][0] = m_filtPrevSampIn[l][m][3][0];
				m_filtPrevSampIn[l][m][4][1] = m_filtPrevSampIn[l][m][3][1];

				m_filtPrevSampIn[l][m][3][0] = m_filtPrevSampIn[l][m][2][0];
				m_filtPrevSampIn[l][m][3][1] = m_filtPrevSampIn[l][m][2][1];

				m_filtPrevSampIn[l][m][2][0] = m_filtPrevSampIn[l][m][1][0];
				m_filtPrevSampIn[l][m][2][1] = m_filtPrevSampIn[l][m][1][1];

				m_filtPrevSampIn[l][m][1][0] = m_filtInputs[l][0];
				m_filtPrevSampIn[l][m][1][1] = m_filtInputs[l][1];

				m_filtPrevSampOut[l][m][4][0] = m_filtPrevSampOut[l][m][3][0];
				m_filtPrevSampOut[l][m][4][1] = m_filtPrevSampOut[l][m][3][1];

				m_filtPrevSampOut[l][m][3][0] = m_filtPrevSampOut[l][m][2][0];
				m_filtPrevSampOut[l][m][3][1] = m_filtPrevSampOut[l][m][2][1];

				m_filtPrevSampOut[l][m][2][0] = m_filtPrevSampOut[l][m][1][0];
				m_filtPrevSampOut[l][m][2][1] = m_filtPrevSampOut[l][m][1][1];

				m_filtPrevSampOut[l][m][1][0] = m_filtPrevSampOut[l][m][0][0];
				m_filtPrevSampOut[l][m][1][1] = m_filtPrevSampOut[l][m][0][1];
			}

			// Put output into feedback delay buffer
			m_temp1 = m_filtFeedback[l] * 0.01f;
			m_filtDelayBuf[l][0][m_filtFeedbackLoc[l]] = m_filtOutputs[l][0] * m_temp1;
			m_filtDelayBuf[l][1][m_filtFeedbackLoc[l]] = m_filtOutputs[l][1] * m_temp1;

			// Calculates Saturation.  The algorithm is just y = x ^ (1 - saturation);
			if (m_filtSatu[l])
			{
				m_temp1 = 1 - (m_filtSatu[l] * 0.01f);
				m_filtOutputs[l][0] = pow(abs(m_filtOutputs[l][0]), m_temp1) * (m_filtOutputs[l][0] < 0 ? -1 : 1);
				m_filtOutputs[l][1] = pow(abs(m_filtOutputs[l][1]), m_temp1) * (m_filtOutputs[l][1] < 0 ? -1 : 1);
			}

			m_temp1 = m_filtOutVol[l] * 0.01f;
			m_filtOutputs[l][0] *= m_temp1;
			m_filtOutputs[l][1] *= m_temp1;

			m_filtInputs[l][0] = 0;
			m_filtInputs[l][1] = 0;

			m_filtModOutputs[l][0] = m_filtOutputs[l][0];
			m_filtModOutputs[l][1] = m_filtOutputs[l][1];

			if (m_filtMuted[l])
			{
				m_filtOutputs[l][0] = 0;
				m_filtOutputs[l][1] = 0;
			}
		}
	}

	//==========================//
	//== WAVETABLE OSCILLATOR ==//
	//==========================//

	for (int i = 0; i < m_maxMainEnabled; ++i)// m_maxMainEnabled keeps this from looping 8 times every m_sample, saving some CPU
	{
		if (m_enabled[i])
		{
			m_currentSampLen = m_sampLen[i] * WAVERATIO;
			for (int l = 0; l < m_unisonVoices[i]; ++l)
			{
				m_sample_morerealindex[i][l] = realfmod((m_sample_realindex[i][l] +
					(m_phase[i] * m_currentSampLen * 0.01f)), m_currentSampLen);// Calculates phase

				// m_unisonVoices[i] - 1 is needed many times, which is why m_unisonVoicesMinusOne exists
				m_unisonVoicesMinusOne = m_unisonVoices[i] - 1;

				if (m_tempo[i])
				{
					// 26400 = 440 Hz * 60 seconds
					m_temp1 = m_tempo[i] / 26400.f;

					// Calculates frequency depending on detune and unison detune
					m_noteFreq = m_unisonVoicesMinusOne
						? detuneWithCents(m_keytracking[i]
							? m_nph->frequency()
							: 440.f, m_unisonDetuneAmounts[i][l]*m_unisonDetune[i]+m_detune[i])
						: detuneWithCents(m_keytracking[i]
							? m_nph->frequency()
							: 440.f, m_detune[i]);

					m_noteFreq *= m_temp1;// Tempo sync
				}
				else
				{
					// Calculates frequency depending on detune and unison detune
					m_noteFreq = m_unisonVoicesMinusOne
						? detuneWithCents(m_keytracking[i]
							? m_nph->frequency()
							: 440.f, m_unisonDetuneAmounts[i][l]*m_unisonDetune[i]+m_detune[i])
						: detuneWithCents(m_keytracking[i]
							? m_nph->frequency()
							: 440.f, m_detune[i]);
				}

				// Find how far we should move through the waveform
				m_sample_step[i][l] = m_currentSampLen * (m_noteFreq / sample_rate);

				// Figures out Morph and Modify values for individual unison voices
				if (m_unisonVoicesMinusOne)
				{
					if (m_unisonMorph[i])
					{
						m_temp1 = ((m_unisonVoicesMinusOne-l)/m_unisonVoicesMinusOne)*m_unisonMorph[i];
						m_morph[i] = qBound(0.f, m_temp1 - (m_unisonMorph[i] * 0.5f) + m_morph[i], m_morphMax[i]);
					}

					if (m_unisonModify[i])
					{
						m_temp1 = ((m_unisonVoicesMinusOne-l)/m_unisonVoicesMinusOne)*m_unisonModify[i];
						m_modify[i] = qBound(0.f, m_temp1 - (m_unisonModify[i] * 0.5f) +
							m_modify[i], m_currentSampLen-1.f);// SampleLength - 1 = ModifyMax
					}
				}

				m_temp7 = m_modify[i] * WAVERATIO;

				switch (m_modifyMode[i])// Horrifying formulas for the various Modify Modes
				{
					case 0:// None
					{
						break;
					}
					case 1:// Pulse Width
					{
						m_sample_morerealindex[i][l] /= (-m_temp7 + m_currentSampLen) / m_currentSampLen;

						m_sample_morerealindex[i][l] = qBound(0.f, m_sample_morerealindex[i][l],
							(float)m_currentSampLen - 1);// Keeps m_sample index within bounds
						break;
					}
					case 2:// Weird 1
					{
						//The cool result of me messing up.
						m_sample_morerealindex[i][l] = (((sin(((m_sample_morerealindex[i][l] / m_currentSampLen) *
							(m_temp7 / 50.f)) / 2)) * m_currentSampLen) * (m_temp7 / m_currentSampLen) +
							(m_sample_morerealindex[i][l] + ((-m_temp7 + m_currentSampLen) / m_currentSampLen))) / 2.f;
						break;
					}
					case 3:// Weird 2
					{
						//Also the cool result of me messing up.
						if (m_sample_morerealindex[i][l] > m_currentSampLen / 2.f)
						{
							m_sample_morerealindex[i][l] = pow(m_sample_morerealindex[i][l] / m_currentSampLen,
								m_temp7 * 10 / m_currentSampLen + 1) * m_currentSampLen;
						}
						else
						{
							m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
							m_sample_morerealindex[i][l] = pow(m_sample_morerealindex[i][l] / m_currentSampLen,
								m_temp7 * 10 / m_currentSampLen + 1) * m_currentSampLen;
							m_sample_morerealindex[i][l] = m_sample_morerealindex[i][l] - m_currentSampLen / 2.f;
						}
						break;
					}
					case 4:// Asym To Right
					{
						m_sample_morerealindex[i][l] = pow(m_sample_morerealindex[i][l] / m_currentSampLen,
							m_temp7 * 10 / m_currentSampLen + 1) * m_currentSampLen;
						break;
					}
					case 5:// Asym To Left
					{
						m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
						m_sample_morerealindex[i][l] = pow(m_sample_morerealindex[i][l] / m_currentSampLen,
							m_temp7 * 10 / m_currentSampLen + 1) * m_currentSampLen;
						m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
						break;
					}
					case 6:// Bidirectional Asym
					{
						m_temp1 = m_temp7 / m_currentSampLen;
						if (m_temp1 < 0.5)
						{
							m_temp1 = (-m_temp1 + 1) / 2.f - 0.25f;
							m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
							m_sample_morerealindex[i][l] = pow(m_sample_morerealindex[i][l] / m_currentSampLen, m_temp1 * 10 + 1) * m_currentSampLen;
							m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
						}
						else
						{
							m_temp1 = m_temp1 / 2.f - 0.25f;
							m_sample_morerealindex[i][l] = pow(m_sample_morerealindex[i][l] / m_currentSampLen, m_temp1 * 10 + 1) * m_currentSampLen;
						}
						break;
					}
					case 7:// Stretch From Center
					{
						m_temp1 = m_currentSampLen / 2.f;
						m_sample_morerealindex[i][l] -= m_temp1;
						m_sample_morerealindex[i][l] /= m_temp1;
						m_sample_morerealindex[i][l] = (m_sample_morerealindex[i][l] >= 0)
							? pow(m_sample_morerealindex[i][l], 1 / ((m_temp7 * 4) / m_currentSampLen + 1))
							: -pow(-m_sample_morerealindex[i][l], 1 / ((m_temp7 * 4) / m_currentSampLen + 1));
						m_sample_morerealindex[i][l] *= m_temp1;
						m_sample_morerealindex[i][l] += m_temp1;
						break;
					}
					case 8:// Squish To Center
					{
						m_temp1 = m_currentSampLen / 2.f;
						m_sample_morerealindex[i][l] -= m_temp1;
						m_sample_morerealindex[i][l] /= m_temp1;
						m_sample_morerealindex[i][l] = (m_sample_morerealindex[i][l] >= 0)
							? pow(m_sample_morerealindex[i][l], 1 / (-m_temp7 / m_currentSampLen + 1 ))
							: -pow(-m_sample_morerealindex[i][l], 1 / (-m_temp7 / m_currentSampLen + 1));
						m_sample_morerealindex[i][l] *= m_temp1;
						m_sample_morerealindex[i][l] += m_temp1;
						break;
					}
					case 9:// Stretch And Squish
					{
						m_temp1 = m_currentSampLen / 2.f;
						m_sample_morerealindex[i][l] -= m_temp1;
						m_sample_morerealindex[i][l] /= m_temp1;
						m_sample_morerealindex[i][l] = (m_sample_morerealindex[i][l] >= 0)
							? pow(m_sample_morerealindex[i][l], 1 / (m_temp7 * 4 / m_currentSampLen))
							: -pow(-m_sample_morerealindex[i][l], 1 / (m_temp7 * 4 / m_currentSampLen));
						m_sample_morerealindex[i][l] *= m_temp1;
						m_sample_morerealindex[i][l] += m_temp1;
						break;
					}
					case 10:// Cut Off Right
					{
						m_sample_morerealindex[i][l] *= (-m_temp7 + m_currentSampLen) / m_currentSampLen;
						break;
					}
					case 11:// Cut Off Left
					{
						m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
						m_sample_morerealindex[i][l] *= (-m_temp7 + m_currentSampLen) / m_currentSampLen;
						m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
						break;
					}
					case 19:// Sync
					{
						m_sample_morerealindex[i][l] *= ((m_temp7 * 16.f) / m_currentSampLen) + 1;
						m_sample_morerealindex[i][l] = fmod(m_sample_morerealindex[i][l], (float)m_currentSampLen - 1);
						break;
					}
					case 20:// Sync Half Interpolate
					{
						m_sample_morerealindex[i][l] *= ((m_temp7 * 16.f) / m_currentSampLen) + 1;
						m_sample_morerealindex[i][l] = fmod(m_sample_morerealindex[i][l], (float)m_currentSampLen - 1);
						break;
					}
					case 21:// Sync Interpolate
					{
						m_sample_morerealindex[i][l] *= ((m_temp7 * 16.f) / m_currentSampLen) + 1;
						m_sample_morerealindex[i][l] = fmod(m_sample_morerealindex[i][l], (float)m_currentSampLen - 1);
						break;
					}
					case 22:// Mirror
					{
						m_sample_morerealindex[i][l] = m_sample_morerealindex[i][l] < m_currentSampLen / 2
							? m_sample_morerealindex[i][l] * 2
							: (-m_sample_morerealindex[i][l] + m_currentSampLen) * 2;
						m_temp1 = m_temp7 / m_currentSampLen;
						if (m_temp1 < 0.5)
						{
							m_temp1 = (-m_temp1 + 1) / 2.f - 0.25f;
							m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
							m_sample_morerealindex[i][l] = pow(m_sample_morerealindex[i][l] / m_currentSampLen, m_temp1 * 10 + 1) * m_currentSampLen;
							m_sample_morerealindex[i][l] = -m_sample_morerealindex[i][l] + m_currentSampLen;
						}
						else
						{
							m_temp1 = m_temp1 / 2.f - 0.25f;
							m_sample_morerealindex[i][l] = pow(m_sample_morerealindex[i][l] / m_currentSampLen, m_temp1 * 10 + 1) * m_currentSampLen;
						}
						break;
					}
					default:
						{}
				}

				m_mainsample[i][l] = 0;

				// If NOT Squarify/Pulsify/Diagonal/Sideways Modify Mode
				if (m_modifyMode[i] != 12 && m_modifyMode[i] != 13 && m_modifyMode[i] != 23 && m_modifyMode[i] != 24)
				{
					m_loopStart = qMax(0.f, m_morph[i] - m_range[i]) + 1;
					m_loopEnd = qMin(m_morph[i] + m_range[i], MAINARRAYLEN / (float)m_currentSampLen) + 1;
					m_currentRangeValInvert = 1.f / m_range[i];
					m_currentIndex = m_sample_morerealindex[i][l];

					// Only grab samples from the waveforms when they will be used, depending on the Range knob
					for (int j = m_loopStart; j < m_loopEnd; ++j)
					{
						// Get waveform samples, set their volumes depending on Range knob
						m_mainsample[i][l] += m_waveforms[i][m_currentIndex + j * m_currentSampLen] *
							(1 - (abs(m_morph[i] - j) * m_currentRangeValInvert));
					}
				}
				else if (m_modifyMode[i] == 12)// If Squarify Modify Mode
				{
					m_loopStart = qMax(0.f, m_morph[i] - m_range[i]) + 1;
					m_loopEnd = qMin(m_morph[i] + m_range[i], MAINARRAYLEN / (float)m_currentSampLen) + 1;
					m_currentRangeValInvert = 1.f / m_range[i];
					m_currentIndex = m_sample_morerealindex[i][l];

					// Self-made formula, may be buggy.  Crossfade one half of waveform with the inverse of the other.

					// Taking these calculations out of the following loop will help with performance with high Range values.
					m_temp2 = m_temp7 / m_currentSampLen;
					m_temp3 = m_temp2;
					++m_temp2;
					m_temp4 = int(m_currentIndex + (m_currentSampLen * 0.5)) % m_currentSampLen;

					for (int j = m_loopStart; j < m_loopEnd; ++j)
					{
						m_temp1 = 1 - (abs(m_morph[i] - j) * m_currentRangeValInvert);
						m_mainsample[i][l] += (
						( m_waveforms[i][m_currentIndex + j * m_currentSampLen] * m_temp1) + // Normal
						(-m_waveforms[i][int(m_temp4)   + j * m_currentSampLen] * m_temp1 * m_temp3)) / // Inverted other half of waveform
						m_temp2; // Volume compensation
					}
				}
				else if (m_modifyMode[i] == 13)// If Pulsify Modify Mode
				{
					m_loopStart = qMax(0.f, m_morph[i] - m_range[i]) + 1;
					m_loopEnd = qMin(m_morph[i] + m_range[i], MAINARRAYLEN / (float)m_currentSampLen) + 1;
					m_currentRangeValInvert = 1.f / m_range[i];
					m_currentIndex = m_sample_morerealindex[i][l];

					// Self-made formula, may be buggy.  Crossfade one side of waveform with the inverse of the other.

					// Taking this calculation out of the following loop will help with performance with high Range values.
					m_temp2 = (m_currentIndex + int(m_currentSampLen * (m_temp7 / m_currentSampLen))) % m_currentSampLen;

					for (int j = m_loopStart; j < m_loopEnd; ++j)
					{
						m_temp1 = 1 - (abs(m_morph[i] - j) * m_currentRangeValInvert);
						m_mainsample[i][l] += (
						(   m_waveforms[i][m_currentIndex + j * m_currentSampLen] * m_temp1) + // Normal
						((-m_waveforms[i][int(m_temp2) + j * m_currentSampLen] * m_temp1))) * // Inverted other side of waveform
						0.5f; // Volume compensation
					}
				}
				else if (m_modifyMode[i] == 23)// If Diagonal Modify Mode
				{
					m_temp5 = realfmod(m_morph[i] + (m_sample_morerealindex[i][l] * m_temp7 / m_currentSampLen / 32.f), m_morphMax[i]);
					m_loopStart = m_temp5 - m_range[i] + 1;
					m_loopEnd = m_temp5 + m_range[i] + 1;
					m_currentRangeValInvert = 1.f / m_range[i];
					m_currentIndex = m_sample_morerealindex[i][l];

					// Quite experimental, morph value is changed depending on wavetable position.
					for (int j = m_loopStart; j < m_loopEnd; ++j)
					{
						m_temp3 = realfmod(j, m_morphMax[i]);
						m_mainsample[i][l] += m_waveforms[i][m_currentIndex + (int)m_temp3 * m_currentSampLen] *
							(1 - (abs(m_temp5 - j) * m_currentRangeValInvert));
					}
				}
				else if (m_modifyMode[i] == 24)// If Sideways Modify Mode
				{
					m_temp5 = m_sample_morerealindex[i][l] / m_currentSampLen * m_morphMax[i];
					m_loopStart = qMax(0.f, m_temp5 - m_range[i]) + 1;
					m_loopEnd = qMin(m_temp5 + m_range[i], MAINARRAYLEN / (float)m_currentSampLen) + 1;
					m_currentRangeValInvert = 1.f / m_range[i];
					m_currentIndex = m_temp7;

					// Quite experimental, swap the Morph value (now controlled using Modify) and the location in the waveform.
					for (int j = m_loopStart; j < m_loopEnd; ++j)
					{
						// Get waveform samples, set their volumes depending on Range knob
						m_mainsample[i][l] += m_waveforms[i][m_currentIndex + j * m_currentSampLen] *
							(1 - (abs(m_temp5 - j) * m_currentRangeValInvert));
					}
				}

				switch (m_modifyMode[i])// More horrifying formulas for the various Modify Modes
				{
					case 1:// Pulse Width
					{
						if (m_sample_realindex[i][l] / ((-m_temp7 + m_currentSampLen) / m_currentSampLen) > m_currentSampLen)
						{
							m_mainsample[i][l] = 0;
						}
						break;
					}
					case 14:// Flip
					{
						if (m_sample_realindex[i][l] > m_temp7)
						{
							m_mainsample[i][l] *= -1;
						}
						break;
					}
					case 15:// Clip
					{
						m_temp1 = m_temp7 / (m_currentSampLen - 1);
						m_mainsample[i][l] = qBound(-m_temp1, m_mainsample[i][l], m_temp1);
						break;
					}
					case 16:// Inverse Clip
					{
						m_temp1 = m_temp7 / (m_currentSampLen - 1);
						if (m_mainsample[i][l] > 0 && m_mainsample[i][l] < m_temp1)
						{
							m_mainsample[i][l] = m_temp1;
						}
						else if (m_mainsample[i][l] < 0 && m_mainsample[i][l] > -m_temp1)
						{
							m_mainsample[i][l] = -m_temp1;
						}
						break;
					}
					case 17:// Sine
					{
						m_temp1 = m_temp7 / (m_currentSampLen - 1);
						m_mainsample[i][l] = sin(m_mainsample[i][l] * (D_PI * m_temp1 * 8 + 1));
						break;
					}
					case 18:// Atan
					{
						m_temp1 = m_temp7 / (m_currentSampLen - 1);
						m_mainsample[i][l] = atan(m_mainsample[i][l] * (D_PI * m_temp1 * 8 + 1));
						break;
					}
					case 20:// Sync Half Interpolate
					{
						m_temp1 = m_currentSampLen / 2.f;
						m_temp2 = m_currentSampLen / 4.f;
						if (m_sample_realindex[i][l] < m_temp2 || m_sample_realindex[i][l] > 3.f * m_temp2)
						{
							m_mainsample[i][l] *= (-abs(m_sample_realindex[i][l] - m_temp1) + m_temp1) / m_currentSampLen * 4.f;
						}
						break;
					}
					case 21:// Sync Interpolate
					{
						m_temp1 = m_currentSampLen / 2.f;
						m_mainsample[i][l] *= (-abs(m_sample_realindex[i][l] - m_temp1) + m_temp1) / m_currentSampLen * 2.f;
						break;
					}
					default:
						{}
				}

				m_sample_realindex[i][l] += m_sample_step[i][l];

				// Loop back waveform position when it goes past the end
				while (m_sample_realindex[i][l] >= m_currentSampLen)
				{
					m_sample_realindex[i][l] -= m_currentSampLen;
					m_lastMainOscEnvDone[l] = true;
				}

				m_mainsample[i][l] *= m_vol[i] * 0.01f;
			}
		}
	}

	//====================//
	//== SUB OSCILLATOR ==//
	//====================//

	for (int i = 0; i < m_maxSubEnabled; ++i)// m_maxSubEnabled keeps this from looping 64 times every m_sample, saving a lot of CPU
	{
		if (m_subEnabled[i])
		{
			if (!m_subNoise[i])
			{
				m_temp2 = m_subPhase[i] * m_subSampLen[i];
				m_temp3 = m_subVol[i] * 0.01f;
				m_temp4 = m_subSampLen[i] * WAVERATIO;

				for (int l = 0; l < m_subUnisonNum[i]; ++l)
				{
					// m_subUnisonNum[i] - 1 is needed many times, which is why m_subUnisonVoicesMinusOne exists
					m_subUnisonVoicesMinusOne = m_subUnisonNum[i] - 1;

					if (!m_subUnisonVoicesMinusOne)
					{
						if (m_subTempo[i])
						{
							// 26400 = 440 Hz * 60 seconds
							m_temp1 = m_subTempo[i] / 26400.f;
							m_noteFreq = m_subKeytrack[i]
								? detuneWithCents(m_nph->frequency(), m_subDetune[i]) * m_temp1
								: detuneWithCents(440.f, m_subDetune[i]) * m_temp1;
						}
						else
						{
							m_noteFreq = m_subKeytrack[i]
								? detuneWithCents(m_nph->frequency(), m_subDetune[i])
								: detuneWithCents(440.f, m_subDetune[i]);
						}
					}
					else
					{
						if (m_subTempo[i])
						{
							// 26400 = 440 Hz * 60 seconds
							m_temp1 = m_subTempo[i] / 26400.f;
							m_noteFreq = m_subKeytrack[i]
								? detuneWithCents(m_nph->frequency(), m_subUnisonDetuneAmounts[i][l]*m_subUnisonDetune[i]+m_subDetune[i]) * m_temp1
								: detuneWithCents(440.f, m_subUnisonDetuneAmounts[i][l]*m_subUnisonDetune[i]+m_subDetune[i]) * m_temp1;
						}
						else
						{
							m_noteFreq = m_subKeytrack[i]
							? detuneWithCents(m_nph->frequency(), m_subUnisonDetuneAmounts[i][l]*m_subUnisonDetune[i]+m_subDetune[i])
							: detuneWithCents(440.f, m_subUnisonDetuneAmounts[i][l]*m_subUnisonDetune[i]+m_subDetune[i]);
						}
					}

					m_sample_step_sub = m_temp4 / (sample_rate / m_noteFreq);

					m_subsample[i][l] = m_temp3 * m_subs[i][int(realfmod(m_sample_subindex[i][l] + m_temp2, m_temp4))];

					m_sample_subindex[i][l] += m_sample_step_sub;

					// move waveform position back if it passed the end of the waveform
					while (m_sample_subindex[i][l] >= m_temp4)
					{
						m_sample_subindex[i][l] -= m_temp4;
						m_lastSubEnvDone[i] = true;
					}
				}
			}
			else// sub oscillator is noise
			{
				// Choose a random graph sample, move that distance from the previous sample,
				// bounce back when upper or lower limits are reached

				m_temp2 = m_subVol[i] * 0.01f;
				for (int l = 0; l < m_subUnisonNum[i]; ++l)
				{
					m_noiseSampRand = fastRandf(m_subSampLen[l]) - 1;

					m_temp1 = (m_storedsubs[i][int(m_noiseSampRand)] * m_subNoiseDirection[i][l]) + m_lastSubNoiseVal[i][l];
					if (m_temp1 > 1 || m_temp1 < -1)
					{
						m_subNoiseDirection[i][l] *= -1;
						m_temp1 = (m_storedsubs[i][int(m_noiseSampRand)] * m_subNoiseDirection[i][l]) + m_lastSubNoiseVal[i][l];
					}

					m_lastSubNoiseVal[i][l] = m_temp1;

					m_subsample[i][l] = m_temp1 * m_temp2;
				}
			}
		}
	}

	//=======================//
	//== SAMPLE OSCILLATOR ==//
	//=======================//

	for (int l = 0; l < m_maxSampleEnabled; ++l)// m_maxSampleEnabled keeps this from looping 8 times every sample, saving some CPU
	{
		int sampleSize = m_samples[l][0].size() * m_sampleEnd[l];
		if (m_sampleEnabled[l] && (m_sample_sampleindex[l] < sampleSize || m_sampleLoop[l]))
		{
			if (m_sampleLoop[l])
			{
				// Loop the sample when it moves past the end
				if (m_sample_sampleindex[l] > sampleSize)
				{
					m_sample_sampleindex[l] = m_sample_sampleindex[l] - sampleSize + (m_samples[l][0].size() * m_sampleStart[l]);
					m_lastSampleEnvDone[l] = true;
				}
			}

			m_sample_step_sample = (detuneWithCents(m_sampleKeytracking[l] ? m_nph->frequency() : 440.f,
				m_sampleDetune[l]) / 440.f) * (44100.f / sample_rate);

			if (m_sampleGraphEnabled[l] && m_sampleStart[l] < m_sampleEnd[l])
			{
				m_progress = realfmod((m_sample_sampleindex[l] + (m_samplePhase[l] * sampleSize * 0.01f)), sampleSize) / sampleSize * 128.f;
				m_intprogress = (int)m_progress;

				m_temp1 = fmod(m_progress, 1);
				m_progress2 = m_sampGraphs[m_intprogress] * (1 - m_temp1);

				// "if" statement prevents wrong value being grabbed when m_intprogress is too high
				if (m_intprogress < 127)
				{
					m_progress3 = m_sampGraphs[m_intprogress+1] * m_temp1;
				}
				else
				{
					m_progress3 = m_sampGraphs[m_intprogress] * m_temp1;
				}

				m_temp1 = int(((m_progress2 + m_progress3 + 1) * 0.5f) * sampleSize);
				m_samplesample[l][0] = m_samples[l][0][m_temp1];
				m_samplesample[l][1] = m_samples[l][1][m_temp1];
			}
			else
			{
				m_temp1 = realfmod((m_sample_sampleindex[l] + (m_samplePhase[l] * sampleSize * 0.01f)), sampleSize);
				m_samplesample[l][0] = m_samples[l][0][m_temp1];
				m_samplesample[l][1] = m_samples[l][1][m_temp1];
			}

			m_temp1 = m_sampleVolume[l] * 0.01f;
			m_samplesample[l][0] *= m_temp1;
			m_samplesample[l][1] *= m_temp1;

			if (m_samplePanning[l] < 0)
			{
				m_samplesample[l][1] *= (100.f + m_samplePanning[l]) * 0.01f;
			}
			else if (m_samplePanning[l] > 0)
			{
				m_samplesample[l][0] *= (100.f - m_samplePanning[l]) * 0.01f;
			}

			m_lastSampleVal[l][0] = m_samplesample[l][0];// Store value for modulation
			m_lastSampleVal[l][1] = m_samplesample[l][1];// Store value for modulation

			if (!m_lastSampleEnvDone[l])
			{
				m_lastSampleEnvVal[l][0] = m_lastSampleVal[l][0];
				m_lastSampleEnvVal[l][1] = m_lastSampleVal[l][1];
			}

			if (m_sampleMuted[l])
			{
				m_samplesample[l][0] = 0;
				m_samplesample[l][1] = 0;
			}

			m_sample_sampleindex[l] += m_sample_step_sample;
		}
	}

	outputSample[0] = 0;
	outputSample[1] = 0;

	// Wavetable Oscillator outputs
	for (int i = 0; i < m_maxMainEnabled; ++i)
	{
		if (m_enabled[i])
		{
			m_unisonVoicesMinusOne = m_unisonVoices[i] - 1;

			if (m_unisonVoicesMinusOne)
			{
				m_sampleMainOsc[0] = 0;
				m_sampleMainOsc[1] = 0;
				for (int j = 0; j < m_unisonVoices[i]; ++j)
				{
					// Pan unison voices
					m_sampleMainOsc[0] += m_mainsample[i][j] * ((m_unisonVoicesMinusOne-j)/m_unisonVoicesMinusOne);
					m_sampleMainOsc[1] += m_mainsample[i][j] * (j/m_unisonVoicesMinusOne);
				}
				// Decrease volume so more unison voices won't increase volume too much
				m_temp1 = m_unisonVoices[i] * 0.5f;
				m_sampleMainOsc[0] /= m_temp1;
				m_sampleMainOsc[1] /= m_temp1;
			}
			else
			{
				m_sampleMainOsc[0] = m_mainsample[i][0];
				m_sampleMainOsc[1] = m_mainsample[i][0];
			}

			if (m_pan[i])
			{
				if (m_pan[i] < 0)
				{
					m_sampleMainOsc[1] *= (100.f + m_pan[i]) * 0.01f;
				}
				else
				{
					m_sampleMainOsc[0] *= (100.f - m_pan[i]) * 0.01f;
				}
			}

			m_lastMainOscVal[i][0] = m_sampleMainOsc[0];// Store results for modulations
			m_lastMainOscVal[i][1] = m_sampleMainOsc[1];

			// second half of "if" statement makes sure the last envelope value is the last value of the
			// graph (not of the waveform), so the sinc interpolation doesn't ruin things.
			if (!m_lastMainOscEnvDone[i] && m_sample_realindex[i][0] <= m_sampLen[i] * WAVERATIO - (WAVERATIO * 2))
			{
				m_lastMainOscEnvVal[i][0] = m_lastMainOscVal[i][0];
				m_lastMainOscEnvVal[i][1] = m_lastMainOscVal[i][1];
			}

			if (!m_muted[i])
			{
				outputSample[0] += m_sampleMainOsc[0];
				outputSample[1] += m_sampleMainOsc[1];
			}
		}
	}

	// Sub Oscillator outputs
	for (int i = 0; i < m_maxSubEnabled; ++i)
	{
		if (m_subEnabled[i])
		{
			if (m_subUnisonNum[i] > 1)
			{
				m_subUnisonVoicesMinusOne = m_subUnisonNum[i] - 1;

				m_sampleSubOsc[0] = 0;
				m_sampleSubOsc[1] = 0;
				for (int j = 0; j < m_subUnisonNum[i]; ++j)
				{
					// Pan unison voices
					m_sampleSubOsc[0] += m_subsample[i][j] * ((m_subUnisonVoicesMinusOne-j)/m_subUnisonVoicesMinusOne);
					m_sampleSubOsc[1] += m_subsample[i][j] * (j/m_subUnisonVoicesMinusOne);
				}
				// Decrease volume so more unison voices won't increase volume too much
				m_temp1 = m_subUnisonNum[i] * 0.5f;
				m_sampleSubOsc[0] /= m_temp1;
				m_sampleSubOsc[1] /= m_temp1;
			}
			else
			{
				m_sampleSubOsc[0] = m_subsample[i][0];
				m_sampleSubOsc[1] = m_subsample[i][0];
			}

			if (m_subPanning[i])
			{
				if (m_subPanning[i] < 0)
				{
					m_sampleSubOsc[1] *= (100.f + m_subPanning[i]) * 0.01f;
				}
				else
				{
					m_sampleSubOsc[0] *= (100.f - m_subPanning[i]) * 0.01f;
				}
			}

			if (m_subRateLimit[i])
			{
				m_sampleSubOsc[0] = m_lastSubVal[i][0] + qBound(-m_subRateLimit[i], m_sampleSubOsc[0] - m_lastSubVal[i][0], m_subRateLimit[i]);
				m_sampleSubOsc[1] = m_lastSubVal[i][1] + qBound(-m_subRateLimit[i], m_sampleSubOsc[1] - m_lastSubVal[i][1], m_subRateLimit[i]);
			}

			m_lastSubVal[i][0] = m_sampleSubOsc[0];// Store results for modulations
			m_lastSubVal[i][1] = m_sampleSubOsc[1];

			// second half of "if" statement makes sure the last envelope value is the last value of the
			// graph (not of the waveform), so the sinc interpolation doesn't ruin things.
			if (!m_lastSubEnvDone[i] && m_sample_subindex[i][0] <= m_subSampLen[i] * WAVERATIO - (WAVERATIO * 2))
			{
				m_lastSubEnvVal[i][0] = m_lastSubVal[i][0];
				m_lastSubEnvVal[i][1] = m_lastSubVal[i][1];
			}

			if (!m_subMuted[i])
			{
				outputSample[0] += m_sampleSubOsc[0];
				outputSample[1] += m_sampleSubOsc[1];
			}
		}
	}

	// Sample Oscillator outputs
	for (int l = 0; l < m_maxSampleEnabled; ++l)// m_maxSampleEnabled keeps this from looping 8 times every m_sample, saving some CPU
	{
		if (m_sampleEnabled[l])
		{
			outputSample[0] += m_samplesample[l][0];
			outputSample[1] += m_samplesample[l][1];
		}
	}

	// Filter outputs
	for (int l = 0; l < m_maxFiltEnabled; ++l)// m_maxFiltEnabled keeps this from looping 8 times every m_sample, saving some CPU
	{
		if (m_filtEnabled[l])
		{
			outputSample[0] += m_filtOutputs[l][0];
			outputSample[1] += m_filtOutputs[l][1];
		}
	}

	// Refresh all modulated values back to the value of the knob.
	for (int i = 0; i < m_numberToReset; ++i)
	{
		refreshValue(m_modValType[i], m_modValNum[i], m_mwc);
	}
	m_numberToReset = 0;

	// Remove DC offset.
	if (m_removeDC)
	{
		m_averageSampleValue[0] = (m_averageSampleValue[0] * 0.999f) + (outputSample[0] * 0.001f);
		m_averageSampleValue[1] = (m_averageSampleValue[1] * 0.999f) + (outputSample[1] * 0.001f);

		outputSample[0] -= m_averageSampleValue[0];
		outputSample[1] -= m_averageSampleValue[1];
	}
}


// Takes input of original Hz and the number of cents to m_detune it by, and returns the detuned result in Hz.
inline float mSynth::detuneWithCents(float pitchValue, float detuneValue)
{
	return pitchValue * std::exp2(detuneValue / 1200.f); 
}


// At the end of mSynth::nextStringSample, this will refresh all modulated values back to the value of the knob.
inline void mSynth::refreshValue(int which, int num, Microwave * m_mwc)
{
	switch (which)
	{
		case 1: m_morph[num] = m_mwc->m_morph[num]->value(); break;
		case 2: m_range[num] = m_mwc->m_range[num]->value(); break;
		case 3: m_modify[num] = m_mwc->m_modify[num]->value(); break;
		case 4: m_modifyMode[num] = m_mwc->m_modifyMode[num]->value(); break;
		case 5: m_vol[num] = m_mwc->m_vol[num]->value(); break;
		case 6: m_pan[num] = m_mwc->m_pan[num]->value(); break;
		case 7: m_detune[num] = m_mwc->m_detune[num]->value(); break;
		case 8: m_phase[num] = m_mwc->m_phase[num]->value(); break;
		case 9: m_phaseRand[num] = m_mwc->m_phaseRand[num]->value(); break;
		case 10: m_enabled[num] = m_mwc->m_enabled[num]->value(); break;
		case 11: m_muted[num] = m_mwc->m_muted[num]->value(); break;
		case 12: m_sampLen[num] = m_mwc->m_sampLen[num]->value(); break;
		case 13: m_morphMax[num] = m_mwc->m_morphMax[num]->value(); break;
		case 14: m_unisonVoices[num] = m_mwc->m_unisonVoices[num]->value(); break;
		case 15: m_unisonDetune[num] = m_mwc->m_unisonDetune[num]->value(); break;
		case 16: m_unisonMorph[num] = m_mwc->m_unisonMorph[num]->value(); break;
		case 17: m_unisonModify[num] = m_mwc->m_unisonModify[num]->value(); break;
		case 18: m_keytracking[num] = m_mwc->m_keytracking[num]->value(); break;
		case 19: m_tempo[num] = m_mwc->m_tempo[num]->value(); break;
		case 20: m_interpolate[num] = m_mwc->m_interpolate[num]->value(); break;

		case 30: m_subEnabled[num] = m_mwc->m_subEnabled[num]->value(); break;
		case 31: m_subMuted[num] = m_mwc->m_subMuted[num]->value(); break;
		case 32: m_subKeytrack[num] = m_mwc->m_subKeytrack[num]->value(); break;
		case 33: m_subNoise[num] = m_mwc->m_subNoise[num]->value(); break;
		case 34: m_subVol[num] = m_mwc->m_subVol[num]->value(); break;
		case 35: m_subPanning[num] = m_mwc->m_subPanning[num]->value(); break;
		case 36: m_subDetune[num] = m_mwc->m_subDetune[num]->value(); break;
		case 37: m_subPhase[num] = m_mwc->m_subPhase[num]->value(); break;
		case 38: m_subPhaseRand[num] = m_mwc->m_subPhaseRand[num]->value(); break;
		case 39: m_subSampLen[num] = m_mwc->m_subSampLen[num]->value(); break;
		case 40: m_subTempo[num] = m_mwc->m_subTempo[num]->value(); break;
		case 41: m_subRateLimit[num] = m_mwc->m_subRateLimit[num]->value(); break;
		case 42: m_subUnisonNum[num] = m_mwc->m_subUnisonNum[num]->value(); break;
		case 43: m_subUnisonDetune[num] = m_mwc->m_subUnisonDetune[num]->value(); break;
		case 44: m_subInterpolate[num] = m_mwc->m_subInterpolate[num]->value(); break;

		case 60: m_sampleEnabled[num] = m_mwc->m_sampleEnabled[num]->value(); break;
		case 61: m_sampleMuted[num] = m_mwc->m_sampleMuted[num]->value(); break;
		case 62: m_sampleKeytracking[num] = m_mwc->m_sampleKeytracking[num]->value(); break;
		case 63: m_sampleGraphEnabled[num] = m_mwc->m_sampleGraphEnabled[num]->value(); break;
		case 64: m_sampleLoop[num] = m_mwc->m_sampleLoop[num]->value(); break;
		case 65: m_sampleVolume[num] = m_mwc->m_sampleVolume[num]->value(); break;
		case 66: m_samplePanning[num] = m_mwc->m_samplePanning[num]->value(); break;
		case 67: m_sampleDetune[num] = m_mwc->m_sampleDetune[num]->value(); break;
		case 68: m_samplePhase[num] = m_mwc->m_samplePhase[num]->value(); break;
		case 69: m_samplePhaseRand[num] = m_mwc->m_samplePhaseRand[num]->value(); break;
		case 70: m_sampleStart[num] = m_mwc->m_sampleStart[num]->value(); break;
		case 71: m_sampleEnd[num] = m_mwc->m_sampleEnd[num]->value(); break;

		case 90: m_modIn[num] = m_mwc->m_modIn[num]->value(); break;
		case 91: m_modInNum[num] = m_mwc->m_modInNum[num]->value(); break;
		case 92: m_modInAmnt[num] = m_mwc->m_modInAmnt[num]->value(); break;
		case 93: m_modInCurve[num] = m_mwc->m_modInCurve[num]->value(); break;
		case 94: m_modIn2[num] = m_mwc->m_modIn2[num]->value(); break;
		case 95: m_modInNum2[num] = m_mwc->m_modInNum2[num]->value(); break;
		case 96: m_modInAmnt2[num] = m_mwc->m_modInAmnt2[num]->value(); break;
		case 97: m_modInCurve2[num] = m_mwc->m_modInCurve2[num]->value(); break;
		case 98: m_modOutSec[num] = m_mwc->m_modOutSec[num]->value(); break;
		case 99: m_modOutSig[num] = m_mwc->m_modOutSig[num]->value(); break;
		case 100: m_modOutSecNum[num] = m_mwc->m_modOutSecNum[num]->value(); break;
		case 101: m_modEnabled[num] = m_mwc->m_modEnabled[num]->value(); break;
		case 102: m_modCombineType[num] = m_mwc->m_modCombineType[num]->value(); break;
		case 103: m_modType[num] = m_mwc->m_modType[num]->value(); break;
		case 104: m_modType2[num] = m_mwc->m_modType2[num]->value(); break;

		case 120: m_filtCutoff[num] = m_mwc->m_filtCutoff[num]->value(); break;
		case 121: m_filtReso[num] = m_mwc->m_filtReso[num]->value(); break;
		case 122: m_filtGain[num] = m_mwc->m_filtGain[num]->value(); break;
		case 123: m_filtType[num] = m_mwc->m_filtType[num]->value(); break;
		case 124: m_filtSlope[num] = m_mwc->m_filtSlope[num]->value(); break;
		case 125: m_filtInVol[num] = m_mwc->m_filtInVol[num]->value(); break;
		case 126: m_filtOutVol[num] = m_mwc->m_filtOutVol[num]->value(); break;
		case 127: m_filtWetDry[num] = m_mwc->m_filtWetDry[num]->value(); break;
		case 128: m_filtBal[num] = m_mwc->m_filtBal[num]->value(); break;
		case 129: m_filtSatu[num] = m_mwc->m_filtSatu[num]->value(); break;
		case 130: m_filtFeedback[num] = m_mwc->m_filtFeedback[num]->value(); break;
		case 131: m_filtDetune[num] = m_mwc->m_filtDetune[num]->value(); break;
		case 132: m_filtEnabled[num] = m_mwc->m_filtEnabled[num]->value(); break;
		case 133: m_filtMuted[num] = m_mwc->m_filtMuted[num]->value(); break;
		case 134: m_filtKeytracking[num] = m_mwc->m_filtKeytracking[num]->value(); break;

		case 150: m_macro[num] = m_mwc->m_macro[num]->value(); break;
	}
}


// Handles negative values properly, unlike fmod.
inline float mSynth::realfmod(float k, float n)
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
"-=Why won't my Microwave make sound?=-<br>"
"<br>"
"1. Wavetable tab: You need to enable the \"Enabled\" LED (has a power icon next to it), then click the folder button to load a wavetable.  After the wavetable is loaded correctly, move the Morph knob and you should hear sound.<br>"
"<br>"
"2. Sub tab: You need to enable the \"Enabled\" LED (has a power icon next to it), then draw whatever you want on the graph.<br>"
"<br>"
"3. Sample tab: You need to enable the \"Enabled\" LED (has a power icon next to it), then click the folder button to load a sample.<br>"
"<br>"
"<br>"
"-=Why won't my filters work?=-<br>"
"<br>"
"Make sure the filter is enabled.  Take the oscillator you want to send through your filter, enable its Muted LED, use it as an input in the Matrix, set the output as \"Filter Input\", and set the Matrix Amount knob to 100%.  This will prevent the original oscillator audio from being sent directly to the audio output, and send the oscillator audio to the filter.<br>"
"<br>"
"<br>"
"-=Why do my filters sound incorrect and/or ugly?=-<br>"
"<br>"
"When using oversampling, the filter output must be interpolated to sound correct, especially with filters that leave high frequencies in the sound.  You can do this by changing the Oversampling Mode in the Miscellaneous tab to \"Average\".<br>"
"<br>"
"<br>"
"-=Why is there sometimes ugly buzzing in my sound?=-<br>"
"<br>"
"Oversampling is essential for a high-quality sound.  Try turning up the Oversampling, and changing the Oversampling Mode to \"Average\".<br>"
"<br>"
;


MicrowaveManualView::MicrowaveManualView():QTextEdit(s_manualText)
{
	setWindowTitle ("Microwave Manual");
	setTextInteractionFlags (Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
	gui->mainWindow()->addWindowedWidget(this);
	parentWidget()->setAttribute(Qt::WA_DeleteOnClose, false);
	parentWidget()->setWindowIcon(PLUGIN_NAME::getIconPixmap("logo"));
	parentWidget()->resize(640, 480);
}


void MicrowaveView::manualBtnClicked()
{
	// Closes and reopens view to make sure it is on top.
	MicrowaveManualView::getInstance()->hide();
	MicrowaveManualView::getInstance()->show();
}


// Store corresponding Matrix values, to allow sending a knob to the Matrix through the right click menu
void MicrowaveKnob::setMatrixLocation(int loc1, int loc2, int loc3)
{
	m_matrixLocation[0] = loc1;
	m_matrixLocation[1] = loc2;
	m_matrixLocation[2] = loc3;

	disconnect(this, &MicrowaveKnob::sendToMatrixAsOutput, 0, 0);
	connect(this, &MicrowaveKnob::sendToMatrixAsOutput, this, [this, loc1, loc2, loc3]() { m_knobView->sendToMatrixAsOutput(loc1, loc2, loc3); });

	disconnect(this, &MicrowaveKnob::switchToMatrixKnob, 0, 0);
	connect(this, &MicrowaveKnob::switchToMatrixKnob, this, [this, loc1, loc2, loc3]() { m_knobView->switchToMatrixKnob(this, loc1, loc2, loc3); });
}


void MicrowaveKnob::setWhichMacroKnob(int which)
{
	this->m_whichMacroKnob = which;

	disconnect(this, &MicrowaveKnob::setMacroTooltip, 0, 0);
	disconnect(this, &MicrowaveKnob::chooseMacroColor, 0, 0);
	disconnect(this, &MicrowaveKnob::refreshMacroColor, 0, 0);
	disconnect(this, &MicrowaveKnob::setMacroColortoDefault, 0, 0);

	if (which != -1)
	{
		connect(this, &MicrowaveKnob::setMacroTooltip, this, [this, which]() { m_knobView->setMacroTooltip(this, which); });
		connect(this, &MicrowaveKnob::chooseMacroColor, this, [this, which]() { m_knobView->chooseMacroColor(this, which); });
		connect(this, &MicrowaveKnob::refreshMacroColor, this, [this, which]() { m_knobView->refreshMacroColor(this, which); });
		connect(this, &MicrowaveKnob::setMacroColortoDefault, this, [this, which]() { m_knobView->setMacroColortoDefault(this, which); });
	}
}


// Override knob behavior to add in new right click menu options
void MicrowaveKnob::contextMenuEvent(QContextMenuEvent *)
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent(NULL);

	CaptionMenu contextMenu(model()->displayName(), this);
	addDefaultActions(&contextMenu);
	contextMenu.addAction(QPixmap(), model()->isScaleLogarithmic() ? tr("Set linear") : tr("Set logarithmic"), this, SLOT(toggleScale()));
	contextMenu.addSeparator();
	if (this->m_matrixLocation[0])
	{
		contextMenu.addAction(PLUGIN_NAME::getIconPixmap("tab4"), tr("Control this in Matrix"), this, SIGNAL(sendToMatrixAsOutput()));
	}
	if (this->m_whichMacroKnob != -1)
	{
		contextMenu.addAction(PLUGIN_NAME::getIconPixmap("tab4"), tr("Set Macro Tooltip"), this, SIGNAL(setMacroTooltip()));
		contextMenu.addAction(embed::getIconPixmap("colorize"), tr("Set Knob Color"), this, SIGNAL(chooseMacroColor()));
		contextMenu.addAction(embed::getIconPixmap("colorize"), tr("Set Knob Color To Default"), this, SIGNAL(setMacroColortoDefault()));
	}
	contextMenu.addSeparator();
	contextMenu.exec(QCursor::pos());
}


// Apply special behavior to middle mouse button and alt key
void MicrowaveKnob::mousePressEvent(QMouseEvent * me)
{
	if ((me->button() == Qt::LeftButton && gui->mainWindow()->isAltPressed()) || me->button() == Qt::MidButton)
	{
		if (m_matrixLocation[0])
		{
			switchToMatrixKnob();
		}

		AutomatableModel *thisModel = model();
		if (thisModel)
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState(false);
		}

		const QPoint & p = me->pos();
		m_origMousePos = p;
		m_mouseOffset = QPoint(0, 0);
		m_leftOver = 0.0f;

		emit sliderPressed();

		QApplication::setOverrideCursor(Qt::BlankCursor);
		s_textFloat->setText(displayValue());
		s_textFloat->moveGlobal(this, QPoint(width() + 2, 0));
		s_textFloat->show();
		m_buttonPressed = true;
	}
	else
	{
		Knob::mousePressEvent(me);
	}
}


void MicrowaveKnob::mouseReleaseEvent(QMouseEvent * me)
{
	Knob::mouseReleaseEvent(me);
	updateScroll();

	if (this->m_whichMacroKnob == -1)
	{
		this->setarcColor(QColor(46,74,80));
		this->setlineColor(QColor(102,198,199));
		this->setInnerColor(QColor(64,92,97));
	}
	else
	{
		refreshMacroColor();
	}
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model *m, void *)
{
	return new Microwave(static_cast<InstrumentTrack *>(m));
}


}

