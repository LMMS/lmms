/*
 * ZynAddSubFx.h - ZynAddSubFX-embedding plugin
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef ZYNADDSUBFX_H
#define ZYNADDSUBFX_H

#include <QMap>
#include <QMutex>

#include <optional>

#include <globals.h>

#include "AudioPluginInterface.h"
#include "AutomatableModel.h"
#include "InstrumentView.h"
#include "RemotePlugin.h"

class QPushButton;

namespace lmms
{


class LocalZynAddSubFx;
class NotePlayHandle;

namespace gui
{
class Knob;
class LedCheckBox;
class ZynAddSubFxView;
}

class ZynAddSubFxRemotePlugin
	: public RemotePlugin
	, public AudioPluginBufferInterface<AudioDataLayout::Split, float, 0, 2>
{
	Q_OBJECT
public:
	ZynAddSubFxRemotePlugin(PluginPinConnector* pinConnector);

	bool processMessage( const message & _m ) override;

	auto inputBuffer() -> SplitAudioData<float, 0> override;
	auto outputBuffer() -> SplitAudioData<float, 2> override;
	void updateBuffers(int channelsIn, int channelsOut) override;

signals:
	void clickedCloseButton();

private:
	std::array<float*, 2> m_accessBuffer;
};


// TODO: Is it always 0 inputs, 2 outputs?
class ZynAddSubFxInstrument
	: public AudioPluginInterface<Instrument, float,
		PluginConfig{ .layout = AudioDataLayout::Split, .inputs = 0, .outputs = 2, .customBuffer = true }>
{
	Q_OBJECT
public:
	ZynAddSubFxInstrument( InstrumentTrack * _instrument_track );
	~ZynAddSubFxInstrument() override;

	void processImpl() override;

	bool handleMidiEvent( const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0 ) override;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	void loadFile( const QString & _file ) override;


	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;

	auto bufferInterface() -> AudioPluginBufferInterface<AudioDataLayout::Split, float, 0, 2>* override;

private slots:
	void reloadPlugin();

	void updatePitchRange();

	void updatePortamento();
	void updateFilterFreq();
	void updateFilterQ();
	void updateBandwidth();
	void updateFmGain();
	void updateResCenterFreq();
	void updateResBandwidth();


private:
	void initPlugin();
	void sendControlChange( MidiControllers midiCtl, float value );

	bool m_hasGUI;
	QMutex m_pluginMutex;
	LocalZynAddSubFx * m_localPlugin;
	ZynAddSubFxRemotePlugin * m_remotePlugin;

	// LocalZynAddSubFx needs to be supplied with a buffer
	std::optional<AudioPluginBufferDefaultImpl<AudioDataLayout::Split, float, 0, 2, false>> m_localPluginBuffer;

	FloatModel m_portamentoModel;
	FloatModel m_filterFreqModel;
	FloatModel m_filterQModel;
	FloatModel m_bandwidthModel;
	FloatModel m_fmGainModel;
	FloatModel m_resCenterFreqModel;
	FloatModel m_resBandwidthModel;
	BoolModel m_forwardMidiCcModel;

	QMap<int, bool> m_modifiedControllers;

	friend class gui::ZynAddSubFxView;


signals:
	void settingsChanged();

} ;


namespace gui
{


class ZynAddSubFxView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	ZynAddSubFxView( Instrument * _instrument, QWidget * _parent );


protected:
	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;


private:
	void modelChanged() override;

	QPushButton * m_toggleUIButton;
	Knob * m_portamento;
	Knob * m_filterFreq;
	Knob * m_filterQ;
	Knob * m_bandwidth;
	Knob * m_fmGain;
	Knob * m_resCenterFreq;
	Knob * m_resBandwidth;
	LedCheckBox * m_forwardMidiCC;


private slots:
	void toggleUI();

} ;


} // namespace gui

} // namespace lmms

#endif
