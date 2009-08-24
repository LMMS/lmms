/*
 * vestige.h - instrument VeSTige for hosting VST-plugins
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Instrument.h"
#include "InstrumentView.h"
#include "midi.h"
#include "note.h"


class QPixmap;
class QPushButton;

class pixmapButton;
class VstPlugin;


class vestigeInstrument : public Instrument
{
	Q_OBJECT
public:
	vestigeInstrument( InstrumentTrack * _instrument_track );
	virtual ~vestigeInstrument();

	virtual void play( sampleFrame * _working_buffer );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual void loadFile( const QString & _file );

	virtual bool isMidiBased( void ) const
	{
		return true;
	}

	virtual bool handleMidiEvent( const midiEvent & _me,
						const midiTime & _time );

	virtual PluginView * instantiateView( QWidget * _parent );


private:
	void closePlugin( void );

	int m_runningNotes[NumKeys];


	VstPlugin * m_plugin;
	QMutex m_pluginMutex;

	QString m_pluginDLL;


	friend class VestigeInstrumentView;

} ;



class VestigeInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	VestigeInstrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~VestigeInstrumentView();


protected slots:
	void openPlugin( void );
	void toggleGUI( void );
	void noteOffAll( void );


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	virtual void modelChanged( void );

	static QPixmap * s_artwork;

	vestigeInstrument * m_vi;

	pixmapButton * m_openPluginButton;
	QPushButton * m_toggleGUIButton;

} ;



#endif
