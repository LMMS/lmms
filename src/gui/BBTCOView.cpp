/*
 * BBTCOView.cpp
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

#include "BBTCOView.h"

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

BBTCOView::BBTCOView( TrackContentObject * _tco, TrackView * _tv ) :
	TrackContentObjectView( _tco, _tv ),
	m_bbTCO( dynamic_cast<BBTCO *>( _tco ) ),
	m_paintPixmap()
{
	connect( _tco->getTrack(), SIGNAL( dataChanged() ), 
			this, SLOT( update() ) );

	setStyle( QApplication::style() );
}

void BBTCOView::constructContextMenu( QMenu * _cm )
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




void BBTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	openInBBEditor();
}




void BBTCOView::paintEvent( QPaintEvent * )
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

	bar_t t = Engine::getBBTrackContainer()->lengthOfBB( m_bbTCO->bbTrackIndex() );
	if( m_bbTCO->length() > TimePos::ticksPerBar() && t > 0 )
	{
		for( int x = static_cast<int>( t * pixelsPerBar() );
								x < width() - 2;
			x += static_cast<int>( t * pixelsPerBar() ) )
		{
			p.drawLine( x, TCO_BORDER_WIDTH, x, TCO_BORDER_WIDTH + lineSize );
			p.drawLine( x, rect().bottom() - ( TCO_BORDER_WIDTH + lineSize ),
			 	x, rect().bottom() - TCO_BORDER_WIDTH );
		}
	}

	// pattern name
	paintTextLabel(m_bbTCO->name(), p);

	// inner border
	p.setPen( c.lighter( 130 ) );
	p.drawRect( 1, 1, rect().right() - TCO_BORDER_WIDTH,
		rect().bottom() - TCO_BORDER_WIDTH );	

	// outer border
	p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
	
	// draw the 'muted' pixmap only if the pattern was manualy muted
	if( m_bbTCO->isMuted() )
	{
		const int spacing = TCO_BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}
	
	p.end();
	
	painter.drawPixmap( 0, 0, m_paintPixmap );
}




void BBTCOView::openInBBEditor()
{
	Engine::getBBTrackContainer()->setCurrentBB( m_bbTCO->bbTrackIndex() );

	getGUI()->mainWindow()->toggleBBEditorWin( true );
}




void BBTCOView::resetName() { m_bbTCO->setName(""); }




void BBTCOView::changeName()
{
	QString s = m_bbTCO->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_bbTCO->setName( s );
}



void BBTCOView::update()
{
	ToolTip::add(this, m_bbTCO->name());

	TrackContentObjectView::update();
}