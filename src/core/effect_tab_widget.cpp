#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_tab_widget.cpp - tab-widget in channel-track-window for setting up
 *                         effects
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtGui/QMenu>
#include <QtGui/QToolButton>

#else

#include <qdom.h>
#include <qlistbox.h>
#include <qpopupmenu.h>
#include <qtoolbutton.h>

#endif


#include "effect_tab_widget.h"
#include "instrument_track.h"
#include "group_box.h"
#include "tooltip.h"
#include "embed.h"
#include "select_ladspa_dialog.h"
#include "rack_plugin.h"
#include "audio_port.h"


effectTabWidget::effectTabWidget( instrumentTrack * _instrument_track ) :
	QWidget( _instrument_track->tabWidgetParent() ),
	journallingObject( _instrument_track->eng() ),
	m_instrumentTrack( _instrument_track )
{
	m_effectsGroupBox = new groupBox( tr( "EFFECTS CHAIN" ), this, eng(), _instrument_track );
	connect( m_effectsGroupBox, SIGNAL( toggled( bool ) ), this, SLOT( setBypass( bool ) ) );
	m_effectsGroupBox->setGeometry( 2, 2, 242, 244 );
	
	m_rack = new rackView( m_effectsGroupBox, eng(), _instrument_track );
	m_rack->move( 6, 22 );
		
	m_addButton = new QPushButton( m_effectsGroupBox, "Add Effect" );
	m_addButton->setText( tr( "Add" ) );
	m_addButton->move( 75, 210 );
	connect( m_addButton, SIGNAL( clicked( void ) ), this, SLOT( addEffect( void ) ) );
}




effectTabWidget::~effectTabWidget()
{
}




void effectTabWidget::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "fxdisabled", !m_effectsGroupBox->isActive() );

}




void effectTabWidget::loadSettings( const QDomElement & _this )
{
	m_effectsGroupBox->setState( !_this.attribute( "fxdisabled" ).toInt() );
}




void effectTabWidget::addEffect( void )
{
	selectLADSPADialog sl( this, eng() );
	sl.exec();
	
	if( sl.result() == QDialog::Rejected )
	{
		return;
	}
	
	ladspa_key_t key = sl.getSelection();
	m_rack->addPlugin( key );
}




void effectTabWidget::setBypass( bool _state )
{
	m_instrumentTrack->getAudioPort()->getEffects()->setBypass( !_state );
}

#include "effect_tab_widget.moc"


#endif

#endif
