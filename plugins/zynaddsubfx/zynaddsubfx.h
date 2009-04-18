/*
 * zynaddsubfx.h - ZynAddSubFX-embedding plugin
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
#include <QtCore/QThread>

#include "instrument.h"
#include "instrument_view.h"
#include "remote_plugin.h"


class QPushButton;

class zynAddSubFxView;
class notePlayHandle;


class zynAddSubFx : public instrument
{
	Q_OBJECT
public:
	zynAddSubFx( instrumentTrack * _instrument_track );
	virtual ~zynAddSubFx();

	virtual void play( sampleFrame * _working_buffer );

	virtual bool handleMidiEvent( const midiEvent & _me,
                                                const midiTime & _time );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual void loadFile( const QString & _file );


	virtual QString nodeName( void ) const;

	virtual bool isMidiBased( void ) const
	{
		return true;
	}

	virtual pluginView * instantiateView( QWidget * _parent );


private slots:
	void updateSampleRate( void );


private:
	void initRemotePlugin( void );

	QMutex m_pluginMutex;
	remotePlugin * m_plugin;

	friend class zynAddSubFxView;


signals:
	void settingsChanged( void );

} ;



class zynAddSubFxView : public instrumentView
{
	Q_OBJECT
public:
	zynAddSubFxView( instrument * _instrument, QWidget * _parent );
	virtual ~zynAddSubFxView();


private:
	void modelChanged( void );

	QPushButton * m_toggleUIButton;


private slots:
	void toggleUI( void );

} ;



#endif
