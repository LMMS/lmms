#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_label.cpp - a label which is renamable by double-clicking it and
 *                    offers access to an effect rack
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "effect_label.h"

#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QMouseEvent>
#include <QtGui/QPushButton>
#include <QtXml/QDomElement>

#include "effect_rack_view.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "main_window.h"
#include "rename_dialog.h"
#include "sample_track.h"




effectLabel::effectLabel( const QString & _initial_name, QWidget * _parent,
							sampleTrack * _track ) :
	QWidget( _parent ),
	m_track( _track )
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

	m_effectRack = new effectRackView(
				m_track->getAudioPort()->getEffects(),
				engine::getMainWindow()->workspace() );

	if( engine::getMainWindow()->workspace() )
	{
		engine::getMainWindow()->workspace()->addSubWindow(
								m_effectRack );
		m_effWindow = m_effectRack->parentWidget();
		m_effWindow->setAttribute( Qt::WA_DeleteOnClose, FALSE );
		m_effWindow->layout()->setSizeConstraint(
							QLayout::SetFixedSize );
	}
	else
	{
		m_effWindow = m_effectRack;
	}

 	m_effWindow->setWindowTitle( _initial_name );
	m_effectRack->setFixedSize( 240, 242 );
	m_effWindow->hide();
}




effectLabel::~effectLabel()
{
	m_effWindow->deleteLater();
}




QString effectLabel::text( void ) const
{
	return( m_label->text() );
}




void effectLabel::setText( const QString & _text )
{
	m_label->setText( _text );
	m_effWindow->setWindowTitle( _text );
}




void effectLabel::showEffects( void )
{
	if( m_effWindow->isHidden() )
	{
		m_effectRack->show();
		if( m_effWindow != m_effectRack )
		{
			m_effWindow->show();
		}
		m_effWindow->raise();
	}
	else
	{
		m_effWindow->hide();
	}
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
