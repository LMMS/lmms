#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_label.cpp - a label which is renamable by double-clicking it and
 *                    offers access to an effect rack
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include "effect_label.h"
#include "sample_track.h"
#include "embed.h"
#include "gui_templates.h"
#include "rename_dialog.h"
#include "main_window.h"


effectLabel::effectLabel( const QString & _initial_name, 
				QWidget * _parent, 
				engine * _engine, 
				sampleTrack * _track ) :
	QWidget( _parent ),
	journallingObject( _engine ),
	m_track( _track ),
	m_show( TRUE )
{
	m_effectBtn = new QPushButton( embed::getIconPixmap( "setup_audio" ),
								"", this );
	m_effectBtn->setGeometry( 6, 1, 28, 28 );
	connect( m_effectBtn, SIGNAL( clicked() ), 
					this, SLOT( showEffects() ) );
	
	m_label = new QLabel( this );
	m_label->setText( _initial_name );
	QFont f = m_label->font();
	m_label->setFont( pointSize<8>( f ) );
	m_label->setGeometry( 38, 1, 200, 28 );
	
	m_effWidget = new effectTabWidget( eng()->getMainWindow()->workspace(),
						m_track, 
						m_track->getAudioPort() );
	m_effWidget->setFixedSize( 240, 242 );
	m_effWidget->hide();
	connect( m_effWidget, SIGNAL( closed() ), 
					this, SLOT( closeEffects() ) );
}




effectLabel::~effectLabel()
{
}




void FASTCALL effectLabel::setText( const QString & _text )
{
	m_label->setText( _text );
}




void effectLabel::showEffects( void )
{
	if( m_show )
	{
		m_effWidget->show();
		m_effWidget->raise();
		m_show = FALSE;
	}
	else
	{
		m_effWidget->hide();
		m_show = TRUE;
	}
}




void effectLabel::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
//	_this.setAttribute( "name", m_label->text() );
	m_effWidget->saveState( _doc, _this );

}




void effectLabel::loadSettings( const QDomElement & _this )
{
//	m_label->setText( _this.attribute( "name" ) );
	
	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_effWidget->nodeName() == node.nodeName() )
			{
				m_effWidget->restoreState( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
}




void effectLabel::closeEffects( void )
{
	m_effWidget->hide();
	m_show = TRUE;
}




void effectLabel::rename( void )
{
	QString txt = text();
	renameDialog rename_dlg( txt );
	rename_dlg.exec();
	if( txt != text() )
	{
		setText( txt );
		emit nameChanged( txt );
		emit nameChanged();
	}
}




void effectLabel::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::RightButton )
	{
		if( _me->x() > 29 )
		{
			rename();
		}
	}
	else
	{
		emit clicked();
	}
}




void effectLabel::mouseDoubleClickEvent( QMouseEvent * _me )
{
	if( _me->x() > 29 )
	{
		rename();
	}
}



#include "effect_label.moc"


#endif

#endif
