/*
 * vestige.h - instrument VeSTige for hosting VST-plugins
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef QT3

#include <QtCore/QMutex>

#else

#include <qmutex.h>

#endif


#include "instrument.h"
#include "midi.h"
#include "note.h"
#include "spc_bg_hndl_widget.h"


class pixmapButton;
class QPushButton;
class remoteVSTPlugin;
class QPixmap;


class vestigeInstrument : public instrument, public specialBgHandlingWidget
{
	Q_OBJECT
public:
	vestigeInstrument( instrumentTrack * _channel_track );
	virtual ~vestigeInstrument();

	virtual void play( bool _try_parallelizing );

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );

	virtual bool supportsParallelizing( void ) const
	{
		return( TRUE );
	}

	virtual void waitForWorkerThread( void );

	virtual bool notePlayHandleBased( void ) const
	{
		return( FALSE );
	}

	virtual bool handleMidiEvent( const midiEvent & _me,
						const midiTime & _time );


protected slots:
	void openPlugin( void );
	void toggleGUI( void );
	void noteOffAll( void );


protected:
	virtual void paintEvent( QPaintEvent * _pe );


private:
	void closePlugin( void );

	static QPixmap * s_artwork;

	enum states
	{
		OFF,
		ON,
		IGNORE_NEXT_NOTEOFF
	} ;
	states m_noteStates[NOTES];


	remoteVSTPlugin * m_plugin;
	QMutex m_pluginMutex;


	pixmapButton * m_openPluginButton;
	QPushButton * m_toggleGUIButton;

	QString m_pluginDLL;


} ;


#endif
