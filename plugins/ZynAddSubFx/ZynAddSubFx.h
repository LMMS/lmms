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

#include <globals.h>

#include "AutomatableModel.h"
#include "Instrument.h"
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

class ZynAddSubFxRemotePlugin : public RemotePlugin
{
	Q_OBJECT
public:
	ZynAddSubFxRemotePlugin();

	bool processMessage( const message & _m ) override;


signals:
	void clickedCloseButton();

} ;



class ZynAddSubFxInstrument : public Instrument
{
	Q_OBJECT
public:
	ZynAddSubFxInstrument( InstrumentTrack * _instrument_track );
	~ZynAddSubFxInstrument() override;

	void play( SampleFrame* _working_buffer ) override;

	bool handleMidiEvent( const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0 ) override;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	void loadFile( const QString & _file ) override;


	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;


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
	LocalZynAddSubFx * m_plugin;
	ZynAddSubFxRemotePlugin * m_remotePlugin;

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
