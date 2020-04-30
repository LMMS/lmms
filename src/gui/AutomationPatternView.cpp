/*
 * AutomationPatternView.cpp - implementation of view for AutomationPattern
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "AutomationPatternView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QMenu>

#include "AutomationEditor.h"
#include "embed.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "ProjectJournal.h"
#include "RenameDialog.h"
#include "StringPairDrag.h"
#include "TextFloat.h"
#include "ToolTip.h"

#include "Engine.h"


QPixmap * AutomationPatternView::s_pat_rec = NULL;

AutomationPatternView::AutomationPatternView( AutomationPattern * _pattern,
						TrackView * _parent ) :
	TrackContentObjectView( _pattern, _parent ),
	m_pat( _pattern ),
	m_paintPixmap()
{
	connect( m_pat, SIGNAL( dataChanged() ),
			this, SLOT( update() ) );
	connect( gui->automationEditor(), SIGNAL( currentPatternChanged() ),
			this, SLOT( update() ) );

	setAttribute( Qt::WA_OpaquePaintEvent, true );

	ToolTip::add(this, m_pat->name());
	setStyle( QApplication::style() );
	
	if( s_pat_rec == NULL ) { s_pat_rec = new QPixmap( embed::getIconPixmap(
							"pat_rec" ) ); }
							
	update();
}




AutomationPatternView::~AutomationPatternView()
{
}




void AutomationPatternView::openInAutomationEditor()
{
	if(gui) gui->automationEditor()->open(m_pat);
}


void AutomationPatternView::update()
{
	ToolTip::add(this, m_pat->name());

	TrackContentObjectView::update();
}



void AutomationPatternView::resetName()
{
	m_pat->setName( QString() );
}




void AutomationPatternView::changeName()
{
	QString s = m_pat->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_pat->setName( s );
	update();
}




void AutomationPatternView::disconnectObject( QAction * _a )
{
	JournallingObject * j = Engine::projectJournal()->
				journallingObject( _a->data().toInt() );
	if( j && dynamic_cast<AutomatableModel *>( j ) )
	{
		float oldMin = m_pat->getMin();
		float oldMax = m_pat->getMax();

		m_pat->m_objects.erase( std::find( m_pat->m_objects.begin(),
					m_pat->m_objects.end(),
				dynamic_cast<AutomatableModel *>( j ) ) );
		update();

		//If automation editor is opened, update its display after disconnection
		if( gui->automationEditor() )
		{
			gui->automationEditor()->m_editor->updateAfterPatternChange();
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




void AutomationPatternView::flipY()
{
	m_pat->flipY( m_pat->getMin(), m_pat->getMax() );
	update();
}




void AutomationPatternView::flipX()
{
	m_pat->flipX( m_pat->length() );
	update();
}




void AutomationPatternView::constructContextMenu( QMenu * _cm )
{
	QAction * a = new QAction( embed::getIconPixmap( "automation" ),
				tr( "Open in Automation editor" ), _cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect(a, SIGNAL(triggered()), this, SLOT(openInAutomationEditor()));
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
	_cm->addAction( embed::getIconPixmap( "flip_y" ),
						tr( "Flip Vertically (Visible)" ),
						this, SLOT( flipY() ) );
	_cm->addAction( embed::getIconPixmap( "flip_x" ),
						tr( "Flip Horizontally (Visible)" ),
						this, SLOT( flipX() ) );
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




void AutomationPatternView::mouseDoubleClickEvent( QMouseEvent * me )
{
	if(me->button() != Qt::LeftButton)
	{
		me->ignore();
		return;
	}
	openInAutomationEditor();
}




void AutomationPatternView::paintEvent( QPaintEvent * )
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
	QColor c;
	bool muted = m_pat->getTrack()->isMuted() || m_pat->isMuted();
	bool current = gui->automationEditor()->currentPattern() == m_pat;
	
	// state: selected, muted, normal
	c = isSelected() ? selectedColor() : ( muted ? mutedBackgroundColor() 
		:	painter.background().color() );

	lingrad.setColorAt( 1, c.darker( 300 ) );
	lingrad.setColorAt( 0, c );

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
	
	const float ppb = fixedTCOs() ?
			( parentWidget()->width() - 2 * TCO_BORDER_WIDTH )
				/ (float) m_pat->timeMapLength().getBar() :
								pixelsPerBar();

	const int x_base = TCO_BORDER_WIDTH;
	
	const float min = m_pat->firstObject()->minValue<float>();
	const float max = m_pat->firstObject()->maxValue<float>();

	const float y_scale = max - min;
	const float h = ( height() - 2 * TCO_BORDER_WIDTH ) / y_scale;
	const float ppTick  = ppb / MidiTime::ticksPerBar();

	p.translate( 0.0f, max * height() / y_scale - TCO_BORDER_WIDTH );
	p.scale( 1.0f, -h );

	QLinearGradient lin2grad( 0, min, 0, max );
	QColor col;
	
	col = !muted ? painter.pen().brush().color() : mutedColor();

	lin2grad.setColorAt( 1, col.lighter( 150 ) );
	lin2grad.setColorAt( 0.5, col );
	lin2grad.setColorAt( 0, col.darker( 150 ) );

	p.setRenderHints( QPainter::Antialiasing, true );
	for( AutomationPattern::timeMap::const_iterator it =
						m_pat->getTimeMap().begin();
					it != m_pat->getTimeMap().end(); ++it )
	{
		if( it+1 == m_pat->getTimeMap().end() )
		{
			const float x1 = x_base + it.key() * ppTick;
			const float x2 = (float)( width() - TCO_BORDER_WIDTH );
			if( x1 > ( width() - TCO_BORDER_WIDTH ) ) break;
			if( gradient() )
			{
				p.fillRect( QRectF( x1, 0.0f, x2 - x1, it.value() ), lin2grad );
			}
			else
			{
				p.fillRect( QRectF( x1, 0.0f, x2 - x1, it.value() ), col );
			}
			break;
		}

		float *values = m_pat->valuesAfter( it.key() );

		float nextValue;
		if( m_pat->progressionType() == AutomationPattern::DiscreteProgression )
		{
			nextValue = it.value();
		}
		else
		{
			nextValue = ( it + 1 ).value();
		}

		QPainterPath path;
		QPointF origin = QPointF( x_base + it.key() * ppTick, 0.0f );
		path.moveTo( origin );
		path.moveTo( QPointF( x_base + it.key() * ppTick,values[0] ) );
		float x;
		for( int i = it.key() + 1; i < ( it + 1 ).key(); i++ )
		{
			x = x_base + i * ppTick;
			if( x > ( width() - TCO_BORDER_WIDTH ) ) break;
			float value = values[ i - it.key() ];
			path.lineTo( QPointF( x, value ) );

		}
		path.lineTo( x_base + ( ( it + 1 ).key() ) * ppTick, nextValue );
		path.lineTo( x_base + ( ( it + 1 ).key() ) * ppTick, 0.0f );
		path.lineTo( origin );

		if( gradient() )
		{
			p.fillPath( path, lin2grad );
		}
		else
		{
			p.fillPath( path, col );
		}
		delete [] values;
	}

	p.setRenderHints( QPainter::Antialiasing, false );
	p.resetTransform();
	
	// bar lines
	const int lineSize = 3;
	p.setPen( c.darker( 300 ) );

	for( bar_t t = 1; t < width() - TCO_BORDER_WIDTH; ++t )
	{
		const int tx = x_base + static_cast<int>( ppb * t ) - 2;
		p.drawLine( tx, TCO_BORDER_WIDTH, tx, TCO_BORDER_WIDTH + lineSize );
		p.drawLine( tx,	rect().bottom() - ( lineSize + TCO_BORDER_WIDTH ),
					tx, rect().bottom() - TCO_BORDER_WIDTH );
	}

	// recording icon for when recording automation
	if( m_pat->isRecording() )
	{
		p.drawPixmap( 1, rect().bottom() - s_pat_rec->height(),
		 	*s_pat_rec );
	}
	
	// pattern name
	paintTextLabel(m_pat->name(), p);
	
	// inner border
	p.setPen( c.lighter( current ? 160 : 130 ) );
	p.drawRect( 1, 1, rect().right() - TCO_BORDER_WIDTH, 
		rect().bottom() - TCO_BORDER_WIDTH );
		
	// outer border	
	p.setPen( current? c.lighter( 130 ) : c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );	

	// draw the 'muted' pixmap only if the pattern was manualy muted
	if( m_pat->isMuted() )
	{
		const int spacing = TCO_BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}

	p.end();

	painter.drawPixmap( 0, 0, m_paintPixmap );
}




void AutomationPatternView::dragEnterEvent( QDragEnterEvent * _dee )
{
	StringPairDrag::processDragEnterEvent( _dee, "automatable_model" );
	if( !_dee->isAccepted() )
	{
		TrackContentObjectView::dragEnterEvent( _dee );
	}
}




void AutomationPatternView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString val = StringPairDrag::decodeValue( _de );
	if( type == "automatable_model" )
	{
		AutomatableModel * mod = dynamic_cast<AutomatableModel *>(
				Engine::projectJournal()->
					journallingObject( val.toInt() ) );
		if( mod != NULL )
		{
			bool added = m_pat->addObject( mod );
			if ( !added )
			{
				TextFloat::displayMessage( mod->displayName(),
							   tr( "Model is already connected "
							   "to this pattern." ),
							   embed::getIconPixmap( "automation" ),
							   2000 );
			}
		}
		update();

		if( gui->automationEditor() &&
			gui->automationEditor()->currentPattern() == m_pat )
		{
			gui->automationEditor()->setCurrentPattern( m_pat );
		}
	}
	else
	{
		TrackContentObjectView::dropEvent( _de );
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
