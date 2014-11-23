/*
 * AutomationPatternView.cpp - implementation of view for AutomationPattern
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QMenu>

#include "AutomationPatternView.h"
#include "AutomationEditor.h"
#include "AutomationPattern.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "ProjectJournal.h"
#include "rename_dialog.h"
#include "string_pair_drag.h"
#include "tooltip.h"


QPixmap * AutomationPatternView::s_pat_rec = NULL;

AutomationPatternView::AutomationPatternView( AutomationPattern * _pattern,
						trackView * _parent ) :
	trackContentObjectView( _pattern, _parent ),
	m_pat( _pattern ),
	m_paintPixmap(),
	m_needsUpdate( true )
{
	connect( m_pat, SIGNAL( dataChanged() ),
			this, SLOT( update() ) );
	connect( engine::automationEditor(), SIGNAL( currentPatternChanged() ),
			this, SLOT( update() ) );

	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setFixedHeight( parentWidget()->height() - 2 );
	setAutoResizeEnabled( false );

	toolTip::add( this, tr( "double-click to open this pattern in "
						"automation editor" ) );
	setStyle( QApplication::style() );
	
	if( s_pat_rec == NULL ) { s_pat_rec = new QPixmap( embed::getIconPixmap(
							"pat_rec" ) ); }
}




AutomationPatternView::~AutomationPatternView()
{
}





void AutomationPatternView::update()
{
	m_needsUpdate = true;
	if( fixedTCOs() )
	{
		m_pat->changeLength( m_pat->length() );
	}
	trackContentObjectView::update();
}




void AutomationPatternView::resetName()
{
	m_pat->setName( QString::null );
}




void AutomationPatternView::changeName()
{
	QString s = m_pat->name();
	renameDialog rename_dlg( s );
	rename_dlg.exec();
	m_pat->setName( s );
	update();
}




void AutomationPatternView::disconnectObject( QAction * _a )
{
	JournallingObject * j = engine::projectJournal()->
				journallingObject( _a->data().toInt() );
	if( j && dynamic_cast<AutomatableModel *>( j ) )
	{
		float oldMin = m_pat->getMin();
		float oldMax = m_pat->getMax();

		m_pat->m_objects.erase( qFind( m_pat->m_objects.begin(),
					m_pat->m_objects.end(),
				dynamic_cast<AutomatableModel *>( j ) ) );
		update();

		//If automation editor is opened, update its display after disconnection
		if( engine::automationEditor() )
		{
			engine::automationEditor()->updateAfterPatternChange();
		}

		//if there is no more connection connected to the AutomationPattern
		if( m_pat->m_objects.size() == 0 )
		{
			//scale the points to fit the new min. and max. value
			this->scaleTimemapToFit( oldMin, oldMax );
		}
	}
}


void AutomationPatternView::toggleRecording()
{
	m_pat->setRecording( ! m_pat->isRecording() );
	update();
}


void AutomationPatternView::constructContextMenu( QMenu * _cm )
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
	_cm->addAction( embed::getIconPixmap( "record" ),
						tr( "Set/clear record" ),
						this, SLOT( toggleRecording() ) );
	if( !m_pat->m_objects.isEmpty() )
	{
		_cm->addSeparator();
		QMenu * m = new QMenu( tr( "%1 Connections" ).
				arg( m_pat->m_objects.count() ), _cm );
		for( AutomationPattern::objectVector::iterator it =
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




void AutomationPatternView::mouseDoubleClickEvent( QMouseEvent * _me )
{
	if( _me->button() != Qt::LeftButton )
	{
		_me->ignore();
		return;
	}
	m_pat->openInAutomationEditor();
}




void AutomationPatternView::paintEvent( QPaintEvent * )
{
	if( m_needsUpdate == false )
	{
		QPainter p( this );
		p.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	QPainter _p( this );
	const QColor styleColor = _p.pen().brush().color();

	m_needsUpdate = false;

	if( m_paintPixmap.isNull() == true || m_paintPixmap.size() != size() )
	{
		m_paintPixmap = QPixmap( size() );
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c;
	if( !( m_pat->getTrack()->isMuted() || m_pat->isMuted() ) )
		c = isSelected() ? QColor( 0, 0, 224 )
						 : styleColor;
	else
		c = QColor( 80,80,80 );

	lingrad.setColorAt( 1, c.darker( 300 ) );
	lingrad.setColorAt( 0, c );

	p.setBrush( lingrad );
	if( engine::automationEditor()->currentPattern() == m_pat )
		p.setPen( c.lighter( 160 ) );
	else
		p.setPen( c.lighter( 130 ) );
	p.drawRect( 1, 1, width()-3, height()-3 );


	const float ppt = fixedTCOs() ?
			( parentWidget()->width() - 2 * TCO_BORDER_WIDTH )
					/ (float) m_pat->length().getTact() :
								pixelsPerTact();

	const int x_base = TCO_BORDER_WIDTH;
	p.setPen( c.darker( 300 ) );

	for( tact_t t = 1; t < m_pat->length().getTact(); ++t )
	{
		const int tx = x_base + static_cast<int>( ppt * t ) - 1;
		if( tx < ( width() - TCO_BORDER_WIDTH*2 ) )
		{
			p.drawLine( tx, TCO_BORDER_WIDTH, tx, 5 );
			p.drawLine( tx,	height() - ( 4 + 2 * TCO_BORDER_WIDTH ),
						tx,	height() - 2 * TCO_BORDER_WIDTH );
		}
	}

	const float min = m_pat->firstObject()->minValue<float>();
	const float max = m_pat->firstObject()->maxValue<float>();

	const float y_scale = max - min;
	const float h = ( height() - 2*TCO_BORDER_WIDTH ) / y_scale;

	p.translate( 0.0f, max * height() / y_scale - TCO_BORDER_WIDTH );
	p.scale( 1.0f, -h );

	QLinearGradient lin2grad( 0, min, 0, max );

	lin2grad.setColorAt( 1, fgColor().lighter( 150 ) );
	lin2grad.setColorAt( 0.5, fgColor() );
	lin2grad.setColorAt( 0, fgColor().darker( 150 ) );

	for( AutomationPattern::timeMap::const_iterator it =
						m_pat->getTimeMap().begin();
					it != m_pat->getTimeMap().end(); ++it )
	{
		if( it+1 == m_pat->getTimeMap().end() )
		{
			const float x1 = x_base + it.key() * ppt /
						MidiTime::ticksPerTact();
			const float x2 = (float)( width() - TCO_BORDER_WIDTH );
			if( x1 > ( width() - TCO_BORDER_WIDTH ) ) break;

			p.fillRect( QRectF( x1, 0.0f, x2-x1, it.value() ),
								lin2grad );
			break;
		}

		float *values = m_pat->valuesAfter( it.key() );
		for( int i = it.key(); i < (it+1).key(); i++ )
		{
			float value = values[i - it.key()];
			const float x1 = x_base + i * ppt /
						MidiTime::ticksPerTact();
			const float x2 = x_base + (i+1) * ppt /
						MidiTime::ticksPerTact();
			if( x1 > ( width() - TCO_BORDER_WIDTH ) ) break;

			p.fillRect( QRectF( x1, 0.0f, x2-x1, value ),
								lin2grad );
		}
		delete [] values;
	}

	p.resetMatrix();

	// recording icon for when recording automation
	if( m_pat->isRecording() )
	{
		p.drawPixmap( 4, 14, *s_pat_rec );
	}

	// outer edge
	p.setBrush( QBrush() );
	if( engine::automationEditor()->currentPattern() == m_pat )
		p.setPen( c.lighter( 130 ) );
	else
		p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, width()-1, height()-1 );

	// pattern name
	p.setFont( pointSize<8>( p.font() ) );

	QColor text_color = ( m_pat->isMuted() || m_pat->getTrack()->isMuted() )
		? QColor( 30, 30, 30 )
		: textColor();

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( 4, p.fontMetrics().height()+1, m_pat->name() );
	p.setPen( text_color );
	p.drawText( 3, p.fontMetrics().height(), m_pat->name() );

	if( m_pat->isMuted() )
	{
		p.drawPixmap( 3, p.fontMetrics().height() + 1,
				embed::getIconPixmap( "muted", 16, 16 ) );
	}


	p.end();

	_p.drawPixmap( 0, 0, m_paintPixmap );

}




void AutomationPatternView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "automatable_model" );
	if( !_dee->isAccepted() )
	{
		trackContentObjectView::dragEnterEvent( _dee );
	}
}




void AutomationPatternView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString val = stringPairDrag::decodeValue( _de );
	if( type == "automatable_model" )
	{
		AutomatableModel * mod = dynamic_cast<AutomatableModel *>(
				engine::projectJournal()->
					journallingObject( val.toInt() ) );
		if( mod != NULL )
		{
			m_pat->addObject( mod );
		}
		update();

		if( engine::automationEditor() &&
			engine::automationEditor()->currentPattern() == m_pat )
		{
			engine::automationEditor()->setCurrentPattern( m_pat );
		}
	}
	else
	{
		trackContentObjectView::dropEvent( _de );
	}
}




/**
 * @brief Preserves the auto points over different scale
 */
void AutomationPatternView::scaleTimemapToFit( float oldMin, float oldMax )
{
	float newMin = m_pat->getMin();
	float newMax = m_pat->getMax();

	if( oldMin == newMin && oldMax == newMax )
	{
		return;
	}

	for( AutomationPattern::timeMap::iterator it = m_pat->m_timeMap.begin();
		it != m_pat->m_timeMap.end(); ++it )
	{
		if( *it < oldMin )
		{
			*it = oldMin;
		}
		else if( *it > oldMax )
		{
			*it = oldMax;
		}
		*it = (*it-oldMin)*(newMax-newMin)/(oldMax-oldMin)+newMin;
	}

	m_pat->generateTangents();
}



#include "moc_AutomationPatternView.cxx"

