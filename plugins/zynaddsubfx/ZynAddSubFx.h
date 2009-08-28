/*
 * ZynAddSubFx.h - ZynAddSubFX-embedding plugin
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _ZYNADDSUBFX_H
#define _ZYNADDSUBFX_H

#include <QtCore/QMutex>

#include "Instrument.h"
#include "InstrumentView.h"
#include "RemotePlugin.h"


class QPushButton;

class LocalZynAddSubFx;
class ZynAddSubFxView;
class notePlayHandle;


class ZynAddSubFxInstrument : public Instrument
{
	Q_OBJECT
public:
	ZynAddSubFxInstrument( InstrumentTrack * _instrument_track );
	virtual ~ZynAddSubFxInstrument();

	virtual void play( sampleFrame * _working_buffer );

	virtual bool handleMidiEvent( const midiEvent & _me,
                                                const midiTime & _time );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual void loadResource( const ResourceItem * _item );


	virtual QString nodeName() const;

	virtual bool isMidiBased() const
	{
		return true;
	}

	virtual PluginView * instantiateView( QWidget * _parent );


private slots:
	void reloadPlugin();


private:
	void initPlugin();

	bool m_hasGUI;
	QMutex m_pluginMutex;
	LocalZynAddSubFx * m_plugin;
	RemotePlugin * m_remotePlugin;
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


private:
	void modelChanged();

	QPushButton * m_toggleUIButton;


private slots:
	void toggleUI();

} ;



#endif
