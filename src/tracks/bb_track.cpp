/*
 * bb_track.cpp - implementation of class bbTrack and bbTCO
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtXml/QDomElement>
#include <QtGui/QColorDialog>
#include <QtGui/QMenu>
#include <QtGui/QPainter>

#include "bb_editor.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "rename_dialog.h"
#include "song.h"
#include "SongEditor.h"
#include "templates.h"
#include "track_label_button.h"



bbTrack::infoMap bbTrack::s_infoMap;


bbTCO::bbTCO( track * _track ) :
	trackContentObject( _track ),
	m_color( 128, 128, 128 ),
	m_useStyleColor( true )
{
	tact_t t = engine::getBBTrackContainer()->lengthOfBB( bbTrackIndex() );
	if( t > 0 )
	{
		saveJournallingState( false );
		changeLength( MidiTime( t, 0 ) );
		restoreJournallingState();
	}
}




bbTCO::~bbTCO()
{
}




void bbTCO::saveSettings( QDomDocument & doc, QDomElement & element )
{
	element.setAttribute( "name", name() );
	if( element.parentNode().nodeName() == "clipboard" )
	{
		element.setAttribute( "pos", -1 );
	}
	else
	{
		element.setAttribute( "pos", startPosition() );
	}
	element.setAttribute( "len", length() );
	element.setAttribute( "muted", isMuted() );
	element.setAttribute( "color", color() );
	
	if( m_useStyleColor )
	{
		element.setAttribute( "usestyle", 1 );
	}
	else
	{
		element.setAttribute( "usestyle", 0 );
	}
}




void bbTCO::loadSettings( const QDomElement & element )
{
	setName( element.attribute( "name" ) );
	if( element.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( element.attribute( "pos" ).toInt() );
	}
	changeLength( element.attribute( "len" ).toInt() );
	if( element.attribute( "muted" ).toInt() != isMuted() )
	{
		toggleMute();
	}

	if( element.hasAttribute( "color" ) )
	{
		setColor( QColor( element.attribute( "color" ).toUInt() ) );
	}
	
	if( element.hasAttribute( "usestyle" ) )
	{
		if( element.attribute( "usestyle" ).toUInt() == 1 ) 
		{
			m_useStyleColor = true;
		}
		else
		{
			m_useStyleColor = false;
		}
	}
	else
	{
		if( m_color.rgb() == qRgb( 128, 182, 175 ) || m_color.rgb() == qRgb( 64, 128, 255 ) ) // old or older default color
		{
			m_useStyleColor = true;
		}
		else
		{
			m_useStyleColor = false;
		}
	}
}



int bbTCO::bbTrackIndex()
{
	return dynamic_cast<bbTrack *>( getTrack() )->index();
}



trackContentObjectView * bbTCO::createView( trackView * _tv )
{
	return new bbTCOView( this, _tv );
}










bbTCOView::bbTCOView( trackContentObject * _tco, trackView * _tv ) :
	trackContentObjectView( _tco, _tv ),
	m_bbTCO( dynamic_cast<bbTCO *>( _tco ) )
{
}




bbTCOView::~bbTCOView()
{
}




void bbTCOView::constructContextMenu( QMenu * _cm )
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
	_cm->addAction( embed::getIconPixmap( "colorize" ),
			tr( "Change color" ), this, SLOT( changeColor() ) );
	_cm->addAction( embed::getIconPixmap( "colorize" ),
			tr( "Reset color to default" ), this, SLOT( resetColor() ) );
}




void bbTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	openInBBEditor();
}




void bbTCOView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	QColor col = m_bbTCO->m_useStyleColor 
		? p.pen().brush().color()
		: m_bbTCO->colorObj();

	if( m_bbTCO->getTrack()->isMuted() || m_bbTCO->isMuted() )
	{
		col = QColor( 160, 160, 160 );
	}
	if( isSelected() == true )
	{
		col = QColor( qMax( col.red() - 128, 0 ),
					qMax( col.green() - 128, 0 ), 255 );
	}

	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( 0, col.light( 130 ) );
	lingrad.setColorAt( 1, col.light( 70 ) );
	p.fillRect( rect(), lingrad );

	tact_t t = engine::getBBTrackContainer()->lengthOfBB( m_bbTCO->bbTrackIndex() );
	if( m_bbTCO->length() > MidiTime::ticksPerTact() && t > 0 )
	{
		for( int x = static_cast<int>( t * pixelsPerTact() );
								x < width()-2;
			x += static_cast<int>( t * pixelsPerTact() ) )
		{
			p.setPen( col.light( 80 ) );
			p.drawLine( x, 1, x, 5 );
			p.setPen( col.light( 120 ) );
			p.drawLine( x, height() - 6, x, height() - 2 );
		}
	}

	p.setPen( col.lighter( 130 ) );
	p.drawRect( 1, 1, rect().right()-2, rect().bottom()-2 );

	p.setPen( col.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );

	p.setFont( pointSize<8>( p.font() ) );
	
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( 4, p.fontMetrics().height()+1, m_bbTCO->name() );
	p.setPen( textColor() );
	p.drawText( 3, p.fontMetrics().height(), m_bbTCO->name() );
	
	if( m_bbTCO->isMuted() )
	{
		p.drawPixmap( 3, p.fontMetrics().height() + 1,
				embed::getIconPixmap( "muted", 16, 16 ) );
	}
}




void bbTCOView::openInBBEditor()
{
	engine::getBBTrackContainer()->setCurrentBB( m_bbTCO->bbTrackIndex() );

	engine::mainWindow()->toggleBBEditorWin( true );
}




void bbTCOView::resetName()
{
	m_bbTCO->setName( m_bbTCO->getTrack()->name() );
}




void bbTCOView::changeName()
{
	QString s = m_bbTCO->name();
	renameDialog rename_dlg( s );
	rename_dlg.exec();
	m_bbTCO->setName( s );
}




void bbTCOView::changeColor()
{
	QColor new_color = QColorDialog::getColor( m_bbTCO->m_color );
	if( ! new_color.isValid() )
	{
		return;
	}
	if( isSelected() )
	{
		QVector<selectableObject *> selected =
				engine::songEditor()->selectedObjects();
		for( QVector<selectableObject *>::iterator it =
							selected.begin();
						it != selected.end(); ++it )
		{
			bbTCOView * bb_tcov = dynamic_cast<bbTCOView *>( *it );
			if( bb_tcov )
			{
				bb_tcov->setColor( new_color );
			}
		}
	}
	else
	{
		setColor( new_color );
	}
}


/** \brief Makes the BB pattern use the colour defined in the stylesheet */
void bbTCOView::resetColor() 
{
	if( ! m_bbTCO->m_useStyleColor )
	{
		m_bbTCO->m_useStyleColor = true;
		engine::getSong()->setModified();
		update();
	}
	bbTrack::clearLastTCOColor();
}



void bbTCOView::setColor( QColor new_color )
{
	if( new_color.rgb() != m_bbTCO->color() )
	{
		m_bbTCO->setColor( new_color );
		m_bbTCO->m_useStyleColor = false;
		engine::getSong()->setModified();
		update();
	}
	bbTrack::setLastTCOColor( new_color );
}





QColor * bbTrack::s_lastTCOColor = NULL;

bbTrack::bbTrack( TrackContainer* tc ) :
	track( BBTrack, tc )
{
	int bbNum = s_infoMap.size();
	s_infoMap[this] = bbNum;

	setName( tr( "Beat/Bassline %1" ).arg( bbNum ) );
	engine::getBBTrackContainer()->setCurrentBB( bbNum );
	engine::getBBTrackContainer()->updateComboBox();

	connect( this, SIGNAL( nameChanged() ),
		engine::getBBTrackContainer(), SLOT( updateComboBox() ) );
}




bbTrack::~bbTrack()
{
	engine::mixer()->removePlayHandles( this );

	const int bb = s_infoMap[this];
	engine::getBBTrackContainer()->removeBB( bb );
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() > bb )
		{
			--it.value();
		}
	}
	s_infoMap.remove( this );

	// remove us from TC so bbTrackContainer::numOfBBs() returns a smaller
	// value and thus combobox-updating in bbTrackContainer works well
	trackContainer()->removeTrack( this );
	engine::getBBTrackContainer()->updateComboBox();
}




// play _frames frames of given TCO within starting with _start
bool bbTrack::play( const MidiTime & _start, const fpp_t _frames,
					const f_cnt_t _offset, int _tco_num )
{
	if( isMuted() )
	{
		return false;
	}

	if( _tco_num >= 0 )
	{
		return engine::getBBTrackContainer()->play( _start, _frames, _offset, s_infoMap[this] );
	}

	tcoVector tcos;
	getTCOsInRange( tcos, _start, _start + static_cast<int>( _frames / engine::framesPerTick() ) );

	if( tcos.size() == 0 )
	{
		return false;
	}

	MidiTime lastPosition;
	MidiTime lastLen;
	for( tcoVector::iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		if( !( *it )->isMuted() &&
				( *it )->startPosition() >= lastPosition )
		{
			lastPosition = ( *it )->startPosition();
			lastLen = ( *it )->length();
		}
	}

	if( _start - lastPosition < lastLen )
	{
		return engine::getBBTrackContainer()->play( _start - lastPosition, _frames, _offset, s_infoMap[this] );
	}
	return false;
}




trackView * bbTrack::createView( TrackContainerView* tcv )
{
	return new bbTrackView( this, tcv );
}




trackContentObject * bbTrack::createTCO( const MidiTime & _pos )
{
	bbTCO * bbtco = new bbTCO( this );
	if( s_lastTCOColor )
	{
		bbtco->setColor( *s_lastTCOColor );
		bbtco->setUseStyleColor( false );
	}
	return bbtco;
}




void bbTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
//	_this.setAttribute( "icon", m_trackLabel->pixmapFile() );
/*	_this.setAttribute( "current", s_infoMap[this] ==
					engine::getBBEditor()->currentBB() );*/
	if( s_infoMap[this] == 0 &&
			_this.parentNode().parentNode().nodeName() != "clone" &&
			_this.parentNode().parentNode().nodeName() != "journaldata" )
	{
		( (JournallingObject *)( engine::getBBTrackContainer() ) )->
						saveState( _doc, _this );
	}
	if( _this.parentNode().parentNode().nodeName() == "clone" )
	{
		_this.setAttribute( "clonebbt", s_infoMap[this] );
	}
}




void bbTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
/*	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}*/

	if( _this.hasAttribute( "clonebbt" ) )
	{
		const int src = _this.attribute( "clonebbt" ).toInt();
		const int dst = s_infoMap[this];
		engine::getBBTrackContainer()->createTCOsForBB( dst );
		TrackContainer::TrackList tl =
					engine::getBBTrackContainer()->tracks();
		// copy TCOs of all tracks from source BB (at bar "src") to destination
		// TCOs (which are created if they do not exist yet)
		for( TrackContainer::TrackList::iterator it = tl.begin();
							it != tl.end(); ++it )
		{
			( *it )->getTCO( src )->copy();
			( *it )->getTCO( dst )->paste();
		}
		setName( tr( "Clone of %1" ).arg(
					_this.parentNode().toElement().attribute( "name" ) ) );
	}
	else
	{
		QDomNode node = _this.namedItem(
					TrackContainer::classNodeName() );
		if( node.isElement() )
		{
			( (JournallingObject *)engine::getBBTrackContainer() )->
					restoreState( node.toElement() );
		}
	}
/*	doesn't work yet because bbTrack-ctor also sets current bb so if
	bb-tracks are created after this function is called, this doesn't
	help at all....
	if( _this.attribute( "current" ).toInt() )
	{
		engine::getBBEditor()->setCurrentBB( s_infoMap[this] );
	}*/
}




// return pointer to bbTrack specified by _bb_num
bbTrack * bbTrack::findBBTrack( int _bb_num )
{
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() == _bb_num )
		{
			return it.key();
		}
	}
	return NULL;
}




void bbTrack::swapBBTracks( track * _track1, track * _track2 )
{
	bbTrack * t1 = dynamic_cast<bbTrack *>( _track1 );
	bbTrack * t2 = dynamic_cast<bbTrack *>( _track2 );
	if( t1 != NULL && t2 != NULL )
	{
		qSwap( s_infoMap[t1], s_infoMap[t2] );
		engine::getBBTrackContainer()->swapBB( s_infoMap[t1],
								s_infoMap[t2] );
		engine::getBBTrackContainer()->setCurrentBB( s_infoMap[t1] );
	}
}









bbTrackView::bbTrackView( bbTrack * _bbt, TrackContainerView* tcv ) :
	trackView( _bbt, tcv ),
	m_bbTrack( _bbt )
{
	setFixedHeight( 32 );
	// drag'n'drop with bb-tracks only causes troubles (and makes no sense
	// too), so disable it
	setAcceptDrops( false );

	m_trackLabel = new trackLabelButton( this, getTrackSettingsWidget() );
	m_trackLabel->setIcon( embed::getIconPixmap( "bb_track" ) );
	m_trackLabel->move( 3, 1 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL( clicked( bool ) ),
			this, SLOT( clickedTrackLabel() ) );
	setModel( _bbt );
}




bbTrackView::~bbTrackView()
{
	engine::getBBEditor()->removeBBView( bbTrack::s_infoMap[m_bbTrack] );
}




bool bbTrackView::close()
{
	engine::getBBEditor()->removeBBView( bbTrack::s_infoMap[m_bbTrack] );
	return trackView::close();
}




void bbTrackView::clickedTrackLabel()
{
	engine::getBBTrackContainer()->setCurrentBB( m_bbTrack->index() );
	engine::getBBEditor()->show();
/*	foreach( bbTrackView * tv,
			trackContainerView()->findChildren<bbTrackView *>() )
	{
		tv->m_trackLabel->update();
	}*/

}




#include "moc_bb_track.cxx"


