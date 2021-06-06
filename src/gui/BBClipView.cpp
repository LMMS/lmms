/*
 * BBClipView.cpp
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "BBClipView.h"

#include <QMenu>
#include <QPainter>

#include "BBEditor.h"
#include "BBTrackContainer.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "RenameDialog.h"
#include "Song.h"
#include "ToolTip.h"

BBClipView::BBClipView( Clip * _clip, TrackView * _tv ) :
	ClipView( _clip, _tv ),
	m_bbClip( dynamic_cast<BBClip *>( _clip ) ),
	m_paintPixmap()
{
	connect( _clip->getTrack(), SIGNAL( dataChanged() ), 
			this, SLOT( update() ) );

	setStyle( QApplication::style() );
}

void BBClipView::constructContextMenu( QMenu * _cm )
{
	QAction * a = new QAction( embed::getIconPixmap( "bb_track" ),
					tr( "Open in Beat+Bassline-Editor" ),
					_cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ),
			this, SLOT( openInBBEditor() ) );
	_cm->insertSeparator( _cm->actions()[1] );
	_cm->addSeparator();
	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT( resetName() ) );
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT( changeName() ) );
}




void BBClipView::mouseDoubleClickEvent( QMouseEvent * )
{
	openInBBEditor();
}




void BBClipView::paintEvent( QPaintEvent * )
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate( false );

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != size())
	{
		m_paintPixmap = QPixmap(size());
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c = getColorForDisplay( painter.background().color() );
	
	lingrad.setColorAt( 0, c.lighter( 130 ) );
	lingrad.setColorAt( 1, c.lighter( 70 ) );

	// paint a black rectangle under the pattern to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}
	
	// bar lines
	const int lineSize = 3;
	p.setPen( c.darker( 200 ) );

	bar_t t = Engine::getBBTrackContainer()->lengthOfBB( m_bbClip->bbTrackIndex() );
	if( m_bbClip->length() > TimePos::ticksPerBar() && t > 0 )
	{
		for( int x = static_cast<int>( t * pixelsPerBar() );
								x < width() - 2;
			x += static_cast<int>( t * pixelsPerBar() ) )
		{
			p.drawLine( x, CLIP_BORDER_WIDTH, x, CLIP_BORDER_WIDTH + lineSize );
			p.drawLine( x, rect().bottom() - ( CLIP_BORDER_WIDTH + lineSize ),
			 	x, rect().bottom() - CLIP_BORDER_WIDTH );
		}
	}

	// pattern name
	paintTextLabel(m_bbClip->name(), p);

	// inner border
	p.setPen( c.lighter( 130 ) );
	p.drawRect( 1, 1, rect().right() - CLIP_BORDER_WIDTH,
		rect().bottom() - CLIP_BORDER_WIDTH );	

	// outer border
	p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
	
	// draw the 'muted' pixmap only if the pattern was manualy muted
	if( m_bbClip->isMuted() )
	{
		const int spacing = CLIP_BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}
	
	p.end();
	
	painter.drawPixmap( 0, 0, m_paintPixmap );
}




void BBClipView::openInBBEditor()
{
	Engine::getBBTrackContainer()->setCurrentBB( m_bbClip->bbTrackIndex() );

	gui->mainWindow()->toggleBBEditorWin( true );
}




void BBClipView::resetName() { m_bbClip->setName(""); }




void BBClipView::changeName()
{
	QString s = m_bbClip->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_bbClip->setName( s );
}



void BBClipView::update()
{
	ToolTip::add(this, m_bbClip->name());

	ClipView::update();
}