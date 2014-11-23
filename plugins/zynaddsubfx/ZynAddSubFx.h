/*
 * ZynAddSubFx.h - ZynAddSubFX-embedding plugin
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtCore/QMap>
#include <QtCore/QMutex>

#include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "RemotePlugin.h"
#include "zynaddsubfx/src/globals.h"


class QPushButton;

class LocalZynAddSubFx;
class ZynAddSubFxView;
class NotePlayHandle;
class knob;
class ledCheckBox;


class ZynAddSubFxRemotePlugin : public QObject, public RemotePlugin
{
	Q_OBJECT
public:
	ZynAddSubFxRemotePlugin();
	virtual ~ZynAddSubFxRemotePlugin();

	virtual bool processMessage( const message & _m );


signals:
	void clickedCloseButton();

} ;



class ZynAddSubFxInstrument : public Instrument
{
	Q_OBJECT
public:
	ZynAddSubFxInstrument( InstrumentTrack * _instrument_track );
	virtual ~ZynAddSubFxInstrument();

	virtual void play( sampleFrame * _working_buffer );

	virtual bool handleMidiEvent( const MidiEvent& event, const MidiTime& time = MidiTime(), f_cnt_t offset = 0 );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual void loadFile( const QString & _file );


	virtual QString nodeName() const;

	virtual Flags flags() const
	{
		return IsSingleStreamed | IsMidiBased;
	}

	virtual PluginView * instantiateView( QWidget * _parent );


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

	friend class ZynAddSubFxView;


signals:
	void settingsChanged();

} ;



class ZynAddSubFxView : public InstrumentView
{
	Q_OBJECT
public:
	ZynAddSubFxView( Instrument * _instrument, QWidget * _parent );
	virtual ~ZynAddSubFxView();


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );


private:
	void modelChanged();

	QPushButton * m_toggleUIButton;
	knob * m_portamento;
	knob * m_filterFreq;
	knob * m_filterQ;
	knob * m_bandwidth;
	knob * m_fmGain;
	knob * m_resCenterFreq;
	knob * m_resBandwidth;
	ledCheckBox * m_forwardMidiCC;


private slots:
	void toggleUI();

} ;


#endif
