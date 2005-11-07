/*
 * midi_tab_widget.h - tab-widget in channel-track-window for setting up
 *                     MIDI-related stuff
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _MIDI_TAB_WIDGET_H
#define _MIDI_TAB_WIDGET_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>

#else

#include <qwidget.h>

#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "settings.h"


class QComboBox;
class QPixmap;

class channelTrack;
class tabWidget;
class ledCheckBox;
class lcdSpinBox;
class midiPort;


class midiTabWidget : public QWidget, public settings
{
	Q_OBJECT
public:
	midiTabWidget( channelTrack * _channel_track, midiPort * _port );
	~midiTabWidget();


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "midi" );
	}



protected slots:
	void inputChannelChanged( int );
	void outputChannelChanged( int );
	void midiPortModeToggled( bool );


private:
	channelTrack * m_channelTrack;
	midiPort * m_midiPort;

	tabWidget * m_setupTabWidget;
	lcdSpinBox * m_inputChannelSpinBox;
	lcdSpinBox * m_outputChannelSpinBox;
	ledCheckBox * m_receiveCheckBox;
	ledCheckBox * m_sendCheckBox;
	ledCheckBox * m_routeCheckBox;
 
} ;


#endif
