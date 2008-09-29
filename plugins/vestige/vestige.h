/*
 * vestige.h - instrument VeSTige for hosting VST-plugins
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _VESTIGE_H
#define _VESTIGE_H


#include <QtCore/QMutex>


#include "instrument.h"
#include "instrument_view.h"
#include "midi.h"
#include "note.h"


class QPixmap;
class QPushButton;

class pixmapButton;
class vstPlugin;


class vestigeInstrument : public instrument
{
	Q_OBJECT
public:
	vestigeInstrument( instrumentTrack * _channel_track );
	virtual ~vestigeInstrument();

	virtual void play( bool _try_parallelizing,
						sampleFrame * _working_buffer );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual void loadFile( const QString & _file );

	virtual bool supportsParallelizing( void ) const
	{
		return( TRUE );
	}

	virtual void waitForWorkerThread( void );

	virtual bool isMidiBased( void ) const
	{
		return( true );
	}

	virtual bool handleMidiEvent( const midiEvent & _me,
						const midiTime & _time );

	virtual pluginView * instantiateView( QWidget * _parent );


private:
	void closePlugin( void );

	int m_runningNotes[NumKeys];


	vstPlugin * m_plugin;
	QMutex m_pluginMutex;

	QString m_pluginDLL;


	friend class vestigeInstrumentView;

} ;



class vestigeInstrumentView : public instrumentView
{
	Q_OBJECT
public:
	vestigeInstrumentView( instrument * _instrument, QWidget * _parent );
	virtual ~vestigeInstrumentView();


protected slots:
	void openPlugin( void );
	void noteOffAll( void );
	void toggleGUI( void );


protected:
	virtual void paintEvent( QPaintEvent * _pe );


private:
	virtual void modelChanged( void );

	static QPixmap * s_artwork;

	vestigeInstrument * m_vi;

	pixmapButton * m_openPluginButton;
	QPushButton * m_toggleGUIButton;

} ;



#endif
