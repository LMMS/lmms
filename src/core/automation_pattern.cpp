#ifndef SINGLE_SOURCE_COMPILE

/*
 * automation_pattern.cpp - implementation of class automationPattern which
 *                          holds dynamic values
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#include <Qt/QtXml>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>


#include "automation_pattern.h"
#include "automation_track.h"
#include "automation_editor.h"
#include "bb_track_container.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "note.h"
#include "project_journal.h"
#include "rename_dialog.h"
#include "song.h"
#include "string_pair_drag.h"
#include "templates.h"
#include "tooltip.h"
#include "track_container.h"




automationPattern::automationPattern( automationTrack * _auto_track ) :
	trackContentObject( _auto_track ),
	m_autoTrack( _auto_track ),
	m_objects(),
	m_updateFirst( TRUE ),
	m_dynamic( FALSE )
{
	changeLength( midiTime( 1, 0 ) );
}




automationPattern::automationPattern( const automationPattern & _pat_to_copy ) :
	trackContentObject( _pat_to_copy.m_autoTrack ),
	m_autoTrack( _pat_to_copy.m_autoTrack ),
	m_objects( _pat_to_copy.m_objects ),
	m_updateFirst( _pat_to_copy.m_updateFirst ),
	m_dynamic( _pat_to_copy.m_dynamic )
{
	for( timeMap::const_iterator it = _pat_to_copy.m_timeMap.begin();
				it != _pat_to_copy.m_timeMap.end(); ++it )
	{
		m_timeMap[it.key()] = it.value();
	}
}




automationPattern::~automationPattern()
{
}




const automatableModel * automationPattern::firstObject( void )
{
	if( !m_objects.isEmpty() )
	{
		return( m_objects.first() );
	}
	static floatModel _fm;
	return( &_fm );
}





//TODO: Improve this
midiTime automationPattern::length( void ) const
{
	tick max_length = 0;

	for( timeMap::const_iterator it = m_timeMap.begin();
							it != m_timeMap.end();
									++it )
	{
		max_length = tMax<tick>( max_length, it.key() );
	}
	if( max_length % DefaultTicksPerTact == 0 )
	{
		return( midiTime( tMax<tick>( max_length,
						DefaultTicksPerTact ) ) );
	}
	return( midiTime( tMax( midiTime( max_length ).getTact() + 1, 1 ),
									0 ) );
}




midiTime automationPattern::putValue( const midiTime & _time,
							const float _value,
							const bool _quant_pos )
{
	midiTime new_time = _quant_pos && engine::getAutomationEditor() ?
		note::quantized( _time,
			engine::getAutomationEditor()->quantization() ) :
		_time;

	m_timeMap[new_time] = _value;

	if( !m_dynamic && new_time != 0 )
	{
		m_dynamic = TRUE;
	}

	if( getTrack() && getTrack()->type() == track::HiddenAutomationTrack )
	{
		changeLength( length() );
	}

	emit dataChanged();

	return( new_time );
}




void automationPattern::removeValue( const midiTime & _time )
{
	if( _time != 0 )
	{
		m_timeMap.remove( _time );

		if( m_timeMap.size() == 1 )
		{
			m_dynamic = FALSE;
			for( objectVector::iterator it = m_objects.begin();
						it != m_objects.end(); ++it )
			{
				if( *it )
				{
					( *it )->setValue( m_timeMap[0] );
				}
				else
				{
					it = m_objects.erase( it );
				}
			}
		}

		if( getTrack() &&
			getTrack()->type() == track::HiddenAutomationTrack )
		{
			changeLength( length() );
		}

		emit dataChanged();
	}
}




void automationPattern::clear( void )
{
	m_timeMap.clear();
	m_dynamic = FALSE;
	m_timeMap[0] = firstObject()->value<float>();

	if( engine::getAutomationEditor() &&
		engine::getAutomationEditor()->currentPattern() == this )
	{
		engine::getAutomationEditor()->update();
	}
}




void automationPattern::openInAutomationEditor( void )
{
	engine::getAutomationEditor()->setCurrentPattern( this );
	engine::getAutomationEditor()->parentWidget()->show();
	engine::getAutomationEditor()->setFocus();
}




float automationPattern::valueAt( const midiTime & _time )
{
	timeMap::const_iterator v = m_timeMap.lowerBound( _time );
	return( ( v != m_timeMap.end() ) ? v.value() : (v-1).value() );
}




void automationPattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( timeMap::const_iterator it = m_timeMap.begin();
						it != m_timeMap.end(); ++it )
	{
		QDomElement element = _doc.createElement( "time" );
		element.setAttribute( "pos", it.key() );
		element.setAttribute( "value", it.value() );
		_this.appendChild( element );
	}
	for( objectVector::const_iterator it = m_objects.begin();
						it != m_objects.end(); ++it )
	{
		QDomElement element = _doc.createElement( "object" );
		element.setAttribute( "id", ( *it )->id() );
		_this.appendChild( element );
	}
}




void automationPattern::loadSettings( const QDomElement & _this )
{
	clear();

//	m_objects.clear();

	for( QDomNode node = _this.firstChild(); !node.isNull();
						node = node.nextSibling() )
	{
		QDomElement element = node.toElement();
		if( element.isNull()  )
		{
			continue;
		}
		if( element.tagName() == "time" )
		{
			m_timeMap[element.attribute( "pos" ).toInt()]
				= element.attribute( "value" ).toFloat();
		}
		else if( element.tagName() == "object" )
		{
			m_idsToResolve << element.attribute( "id" ).toInt();
		}
	}

	m_dynamic = m_timeMap.size() > 1;

	changeLength( length() );
}




const QString automationPattern::name( void ) const
{
	if( !trackContentObject::name().isEmpty() )
	{
		return( trackContentObject::name() );
	}
	if( !m_objects.isEmpty() )
	{
		return( m_objects.first()->fullDisplayName() );
	}
	return( tr( "No control assigned" ) );
}




void automationPattern::processMidiTime( const midiTime & _time )
{
	if( _time >= 0 && m_dynamic )
	{
		const float val = valueAt( _time );
		for( objectVector::iterator it = m_objects.begin();
						it != m_objects.end(); ++it )
		{
			if( *it )
			{
				( *it )->setAutomatedValue( val );
			}

		}
	}
}





trackContentObjectView * automationPattern::createView( trackView * _tv )
{
	return( new automationPatternView( this, _tv ) );
}





bool automationPattern::isAutomated( const automatableModel * _m )
{
	const trackContainer::trackList l = engine::getSong()->tracks() +
				engine::getBBTrackContainer()->tracks();
	for( trackContainer::trackList::const_iterator it = l.begin();
							it != l.end(); ++it )
	{
		if( ( *it )->type() == track::AutomationTrack )
		{
			const track::tcoVector & v = ( *it )->getTCOs();
			for( track::tcoVector::const_iterator j = v.begin();
							j != v.end(); ++j )
			{
				const automationPattern * a =
					dynamic_cast<const
						automationPattern *>( *j );
				if( a && a->m_dynamic )
				{
	for( objectVector::const_iterator k = a->m_objects.begin();
					k != a->m_objects.end(); ++k )
	{
		if( *k == _m )
		{
			return( TRUE );
		}
	}
				}
			}
		}
	}
	return( FALSE );
}





automationPattern * automationPattern::globalAutomationPattern(
							automatableModel * _m )
{
	automationTrack * t = engine::getSong()->globalAutomationTrack();
	track::tcoVector v = t->getTCOs();
	for( track::tcoVector::const_iterator j = v.begin(); j != v.end(); ++j )
	{
		automationPattern * a = dynamic_cast<automationPattern *>( *j );
		if( a )
		{
			for( objectVector::const_iterator k =
							a->m_objects.begin();
						k != a->m_objects.end(); ++k )
			{
				if( *k == _m )
				{
					return( a );
				}
			}
		}
	}

	automationPattern * a = new automationPattern( t );
	a->m_objects += _m;
	return( a );
}




void automationPattern::resolveAllIDs( void )
{
	trackContainer::trackList l = engine::getSong()->tracks() +
				engine::getBBTrackContainer()->tracks();
	for( trackContainer::trackList::iterator it = l.begin();
							it != l.end(); ++it )
	{
		if( ( *it )->type() == track::AutomationTrack ||
			 ( *it )->type() == track::HiddenAutomationTrack )
		{
			track::tcoVector v = ( *it )->getTCOs();
			for( track::tcoVector::iterator j = v.begin();
							j != v.end(); ++j )
			{
				automationPattern * a =
					dynamic_cast<automationPattern *>( *j );
				if( a )
				{
	for( QVector<jo_id_t>::iterator k = a->m_idsToResolve.begin();
					k != a->m_idsToResolve.end(); ++k )
	{
		journallingObject * o = engine::getProjectJournal()->
						getJournallingObject( *k );
		if( o && dynamic_cast<automatableModel *>( o ) )
		{
			a->m_objects += dynamic_cast<automatableModel *>( o );
		}
	}
	a->m_idsToResolve.clear();
				}
			}
		}
	}
}






automationPatternView::automationPatternView( automationPattern * _pattern,
						trackView * _parent ) :
	trackContentObjectView( _pattern, _parent ),
	m_pat( _pattern ),
	m_paintPixmap(),
	m_needsUpdate( TRUE )
{
	connect( m_pat, SIGNAL( dataChanged() ),
			this, SLOT( update() ) );

	setFixedHeight( parentWidget()->height() - 2 );
	setAutoResizeEnabled( FALSE );

	toolTip::add( this, tr( "double-click to open this pattern in "
						"automation editor" ) );
}






automationPatternView::~automationPatternView()
{
	if( engine::getAutomationEditor()
		&& engine::getAutomationEditor()->currentPattern() == m_pat )
	{
		engine::getAutomationEditor()->setCurrentPattern( NULL );
	}
}





void automationPatternView::update( void )
{
	m_needsUpdate = TRUE;
//	m_pat->changeLength( m_pat->length() );
	trackContentObjectView::update();
}




void automationPatternView::resetName( void )
{
	m_pat->setName( QString::null );
}




void automationPatternView::changeName( void )
{
	QString s = m_pat->name();
	renameDialog rename_dlg( s );
	rename_dlg.exec();
	m_pat->setName( s );
	update();
}




void automationPatternView::disconnectObject( QAction * _a )
{
	journallingObject * j = engine::getProjectJournal()->
				getJournallingObject( _a->data().toInt() );
	if( j && dynamic_cast<automatableModel *>( j ) )
	{
		m_pat->m_objects.erase( qFind( m_pat->m_objects.begin(),
					m_pat->m_objects.end(),
				dynamic_cast<automatableModel *>( j ) ) );
		update();
	}
}




void automationPatternView::constructContextMenu( QMenu * _cm )
{
	QAction * a = new QAction( embed::getIconPixmap( "automation" ),
				tr( "Open in Automation editor" ), _cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ),
				m_pat, SLOT( openInAutomationEditor() ) );
	_cm->insertSeparator( _cm->actions()[1] );

	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "edit_erase" ),
			tr( "Clear" ), m_pat, SLOT( clear() ) );
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT( resetName() ) );
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT( changeName() ) );
	if( !m_pat->m_objects.isEmpty() )
	{
		_cm->addSeparator();
		QMenu * m = new QMenu( tr( "Connections" ), _cm );
		for( automationPattern::objectVector::iterator it =
						m_pat->m_objects.begin();
					it != m_pat->m_objects.end(); ++it )
		{
			if( *it )
			{
				a = new QAction( tr( "Disconnect \"%1\"" ).
					arg( ( *it )->fullDisplayName() ), m );
				a->setData( ( *it )->id() );
				m->addAction( a );
			}
		}
		connect( m, SIGNAL( triggered( QAction * ) ),
				this, SLOT( disconnectObject( QAction * ) ) );
		_cm->addMenu( m );
	}
	
	_cm->addSeparator();
}




void automationPatternView::mouseDoubleClickEvent( QMouseEvent * _me )
{
	if( _me->button() != Qt::LeftButton )
	{
		_me->ignore();
		return;
	}
	m_pat->openInAutomationEditor();
}




void automationPatternView::paintEvent( QPaintEvent * )
{
	if( m_needsUpdate == FALSE )
	{
		QPainter p( this );
		p.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	m_needsUpdate = FALSE;

	if( m_paintPixmap.isNull() == TRUE || m_paintPixmap.size() != size() )
	{
		m_paintPixmap = QPixmap( size() );
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	const QColor c = isSelected() ? QColor( 0, 0, 224 ) :
							QColor( 96, 96, 96 );
	lingrad.setColorAt( 0, c );
	lingrad.setColorAt( 0.5, Qt::black );
	lingrad.setColorAt( 1, c );
	p.setBrush( lingrad );
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawRect( QRect( 0, 0, width() - 1, height() - 1 ) );

	const float ppt = fixedTCOs() ?
			( parentWidget()->width() - 2 * TCO_BORDER_WIDTH )
					/ (float) m_pat->length().getTact() :
								pixelsPerTact();

	const int x_base = TCO_BORDER_WIDTH;
	p.setPen( QColor( 0, 0, 0 ) );

	for( tact t = 1; t < m_pat->length().getTact(); ++t )
	{
		p.drawLine( x_base + static_cast<int>( ppt * t ) - 1,
				TCO_BORDER_WIDTH, x_base + static_cast<int>(
						ppt * t ) - 1, 5 );
		p.drawLine( x_base + static_cast<int>( ppt * t ) - 1,
				height() - ( 4 + 2 * TCO_BORDER_WIDTH ),
				x_base + static_cast<int>( ppt * t ) - 1,
				height() - 2 * TCO_BORDER_WIDTH );
	}

	const float min = m_pat->firstObject()->minValue<float>();
	const float max = m_pat->firstObject()->maxValue<float>();

	const float y_scale = max - min;
	const float h = ( height()-2*TCO_BORDER_WIDTH ) / y_scale;

	p.translate( 0.0f, max * height() / y_scale-1 );
	p.scale( 1.0f, -h );

	//QLinearGradient lin2grad( 0, min, 0, max );
	QLinearGradient lin2grad( 0, min, 0, max );
	const QColor cl = QColor( 255, 224, 0 );
	const QColor cd = QColor( 229, 158, 0 );
	//lingrad.setColorAt( min, c );
	lin2grad.setColorAt( 1, cl );

/*	if( min < 0 ) {
		lin2grad.setColorAt( -min/y_scale, Qt::black );
		lin2grad.setColorAt( 0, k );
	}
	else {*/
		lin2grad.setColorAt( 0, cd );
//	}

	for( automationPattern::timeMap::const_iterator it =
				m_pat->getTimeMap().begin();
					it != m_pat->getTimeMap().end(); ++it )
	{
		const float x1 = 2 * x_base + it.key() * ppt /
						midiTime::ticksPerTact();
		float x2;
		if( it+1 != m_pat->getTimeMap().end() )
		{
		 	x2 = (it+1).key() * ppt / midiTime::ticksPerTact() + 2;
		}
		else
		{
			x2 = width() - TCO_BORDER_WIDTH;
		}
		p.fillRect( QRectF( x1, 0.0f, x2-x1, it.value() ),
							lin2grad /*QColor( 255, 224, 0 )*/ );
	}

	p.resetMatrix();
	p.setFont( pointSize<7>( p.font() ) );
	if( m_pat->isMuted() || m_pat->getTrack()->isMuted() )
	{
		p.setPen( QColor( 192, 192, 192 ) );
	}
	else
	{
		p.setPen( QColor( 0, 64, 255 ) );
	}

	p.drawText( 2, p.fontMetrics().height() - 1, m_pat->name() );

	if( m_pat->isMuted() )
	{
		p.drawPixmap( 3, p.fontMetrics().height() + 1,
				embed::getIconPixmap( "muted", 16, 16 ) );
	}

	p.end();

	p.begin( this );
	p.drawPixmap( 0, 0, m_paintPixmap );

}




void automationPatternView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "automatable_model" );
}




void automationPatternView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString val = stringPairDrag::decodeValue( _de );
	if( type == "automatable_model" )
	{
		automatableModel * mod = dynamic_cast<automatableModel *>(
				engine::getProjectJournal()->
					getJournallingObject( val.toInt() ) );
		if( mod != NULL )
		{
			m_pat->m_objects += mod;
		}
	}

	update();

	if( engine::getAutomationEditor() &&
		engine::getAutomationEditor()->currentPattern() == m_pat )
	{
		engine::getAutomationEditor()->setCurrentPattern( m_pat );
	}
}







#include "automation_pattern.moc"


#endif
