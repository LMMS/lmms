/*
 * effect_tab_widget.h - tab-widget in channel-track-window for setting up
 *                       effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#ifndef _EFFECT_TAB_WIDGET_H
#define _EFFECT_TAB_WIDGET_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QLayout>

#else

#include <qwidget.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qvbox.h>

#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "journalling_object.h"
#include "rack_plugin.h"
#include "rack_view.h"
#include "audio_port.h"
#include "track.h"


class instrumentTrack;
class sampleTrack;
class groupBox;


class effectTabWidget : public QWidget, public journallingObject
{
	Q_OBJECT
public:
	effectTabWidget( instrumentTrack * _track, audioPort * _port );
	effectTabWidget( QWidget * _parent, sampleTrack * _track, 
							audioPort * _port );
	virtual ~effectTabWidget();


	virtual void FASTCALL saveSettings( QDomDocument & _doc, 
						QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "fx" );
	}
	
	void setupWidget( void );

signals:
	void closed( void );

private slots:
	void addEffect( void );
	void setBypass( bool _state );

protected:
	virtual void closeEvent( QCloseEvent * _ce );

private:
	track * m_track;
	audioPort * m_port;
	
	groupBox * m_effectsGroupBox;
	QPushButton * m_addButton;
	
	rackView * m_rack;
		
	friend class instrumentTrack;

} ;

#endif
