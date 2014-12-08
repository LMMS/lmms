/*
 * AutomationEditor.cpp - implementation of AutomationEditor which is used for
 *						actual setting of dynamic values
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008-2013 Paul Giblock <pgib/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#include "AutomationEditor.h"

#include <QApplication>
#include <QButtonGroup>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOption>
#include <QWheelEvent>
#include <QToolTip>
#include <QSignalMapper>


#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>


#include "SongEditor.h"
#include "MainWindow.h"
#include "embed.h"
#include "Engine.h"
#include "PixmapButton.h"
#include "templates.h"
#include "gui_templates.h"
#include "Timeline.h"
#include "ToolTip.h"
#include "TextFloat.h"
#include "ComboBox.h"
#include "BBTrackContainer.h"
#include "PianoRoll.h"
#include "debug.h"
#include "MeterModel.h"


QPixmap * AutomationEditor::s_toolDraw = NULL;
QPixmap * AutomationEditor::s_toolErase = NULL;
QPixmap * AutomationEditor::s_toolSelect = NULL;
QPixmap * AutomationEditor::s_toolMove = NULL;




AutomationEditor::AutomationEditor() :
	QWidget(),
	m_zoomingXModel(),
	m_zoomingYModel(),
	m_quantizeModel(),
	m_patternMutex( QMutex::Recursive ),
	m_pattern( NULL ),
	m_minLevel( 0 ),
	m_maxLevel( 0 ),
	m_step( 1 ),
	m_scrollLevel( 0 ),
	m_bottomLevel( 0 ),
	m_topLevel( 0 ),
	m_currentPosition(),
	m_action( NONE ),
	m_moveStartLevel( 0 ),
	m_moveStartTick( 0 ),
	m_drawLastLevel( 0.0f ),
	m_drawLastTick( 0 ),
	m_ppt( DEFAULT_PPT ),
	m_y_delta( DEFAULT_Y_DELTA ),
	m_y_auto( true ),
	m_editMode( DRAW ),
	m_scrollBack( false ),
	m_gridColor( 0,0,0 ),
	m_graphColor(),
	m_vertexColor( 0,0,0 ),
	m_scaleColor()
{
	connect( this, SIGNAL( currentPatternChanged() ),
				this, SLOT( updateAfterPatternChange() ),
				Qt::QueuedConnection );
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
						this, SLOT( update() ) );

	setAttribute( Qt::WA_OpaquePaintEvent, true );

	m_tensionModel = new FloatModel(1.0, 0.0, 1.0, 0.01);
	connect( m_tensionModel, SIGNAL( dataChanged() ),
				this, SLOT( setTension() ) );

	for( int i = 0; i < 7; ++i )
	{
		m_quantizeModel.addItem( "1/" + QString::number( 1 << i ) );
	}
	m_quantizeModel.setValue( m_quantizeModel.findText( "1/16" ) );

	// add time-line
	m_timeLine = new Timeline( VALUES_WIDTH, 0, m_ppt,
				Engine::getSong()->getPlayPos(
					Song::Mode_PlayAutomationPattern ),
						m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const MidiTime & ) ),
		m_timeLine, SLOT( updatePosition( const MidiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const MidiTime & ) ),
			this, SLOT( updatePosition( const MidiTime & ) ) );

	removeSelection();

	// init scrollbars
	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_leftRightScroll->setSingleStep( 1 );
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ), this,
						SLOT( horScrolled( int ) ) );

	m_topBottomScroll = new QScrollBar( Qt::Vertical, this );
	m_topBottomScroll->setSingleStep( 1 );
	m_topBottomScroll->setPageStep( 20 );
	connect( m_topBottomScroll, SIGNAL( valueChanged( int ) ), this,
						SLOT( verScrolled( int ) ) );

	// init pixmaps
	if( s_toolDraw == NULL )
	{
		s_toolDraw = new QPixmap( embed::getIconPixmap(
							"edit_draw" ) );
	}
	if( s_toolErase == NULL )
	{
		s_toolErase= new QPixmap( embed::getIconPixmap(
							"edit_erase" ) );
	}
	if( s_toolSelect == NULL )
	{
		s_toolSelect = new QPixmap( embed::getIconPixmap(
							"edit_select" ) );
	}
	if( s_toolMove == NULL )
	{
		s_toolMove = new QPixmap( embed::getIconPixmap(
							"edit_move" ) );
	}

	setCurrentPattern( NULL );

	setMouseTracking( true );
}




AutomationEditor::~AutomationEditor()
{
	m_zoomingXModel.disconnect();
	m_zoomingYModel.disconnect();
	m_quantizeModel.disconnect();
	m_tensionModel->disconnect();

	delete m_tensionModel;
}




void AutomationEditor::setCurrentPattern(AutomationPattern * new_pattern )
{
	m_patternMutex.lock();
	m_pattern = new_pattern;
	m_patternMutex.unlock();

	emit currentPatternChanged();
}




void AutomationEditor::saveSettings(QDomDocument & doc, QDomElement & parent )
{
	MainWindow::saveWidgetState( this, parent );
}




void AutomationEditor::loadSettings( const QDomElement & parent )
{
	MainWindow::restoreWidgetState( this, parent );
}



// qproperty access methods

QColor AutomationEditor::gridColor() const
{ return m_gridColor; }
QBrush AutomationEditor::graphColor() const
{ return m_graphColor; }
QColor AutomationEditor::vertexColor() const
{ return m_vertexColor; }
QBrush AutomationEditor::scaleColor() const
{ return m_scaleColor; }
void AutomationEditor::setGridColor( const QColor & c )
{ m_gridColor = c; }
void AutomationEditor::setGraphColor( const QBrush & c )
{ m_graphColor = c; }
void AutomationEditor::setVertexColor( const QColor & c )
{ m_vertexColor = c; }
void AutomationEditor::setScaleColor( const QBrush & c )
{ m_scaleColor = c; }



void AutomationEditor::updateAfterPatternChange()
{
	QMutexLocker m( &m_patternMutex );

	m_currentPosition = 0;

	if( !validPattern() )
	{
		setWindowTitle( tr( "Automation Editor - no pattern" ) );
		m_minLevel = m_maxLevel = m_scrollLevel = 0;
		m_step = 1;
		resizeEvent( NULL );
		return;
	}

	m_minLevel = m_pattern->firstObject()->minValue<float>();
	m_maxLevel = m_pattern->firstObject()->maxValue<float>();
	m_step = m_pattern->firstObject()->step<float>();
	m_scrollLevel = ( m_minLevel + m_maxLevel ) / 2;

	// resizeEvent() does the rest for us (scrolling, range-checking
	// of levels and so on...)
	resizeEvent( NULL );

	setWindowTitle( tr( "Automation Editor - %1" ).arg( m_pattern->name() ) );

	update();
}




void AutomationEditor::update()
{
	QWidget::update();

	QMutexLocker m( &m_patternMutex );
	// Note detuning?
	if( m_pattern && !m_pattern->getTrack() )
	{
		Engine::pianoRoll()->update();
	}
}




void AutomationEditor::removeSelection()
{
	m_selectStartTick = 0;
	m_selectedTick = 0;
	m_selectStartLevel = 0;
	m_selectedLevels = 0;
}




void AutomationEditor::keyPressEvent(QKeyEvent * ke )
{
	switch( ke->key() )
	{
		case Qt::Key_Up:
			m_topBottomScroll->setValue(
					m_topBottomScroll->value() - 1 );
			ke->accept();
			break;

		case Qt::Key_Down:
			m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
			ke->accept();
			break;

		case Qt::Key_Left:
			if( ( m_timeLine->pos() -= 16 ) < 0 )
			{
				m_timeLine->pos().setTicks( 0 );
			}
			m_timeLine->updatePosition();
			ke->accept();
			break;

		case Qt::Key_Right:
			m_timeLine->pos() += 16;
			m_timeLine->updatePosition();
			ke->accept();
			break;

		//TODO: m_selectButton and m_moveButton are broken.
		/*case Qt::Key_A:
			if( ke->modifiers() & Qt::ControlModifier )
			{
				m_selectButton->setChecked( true );
				selectAll();
				update();
				ke->accept();
			}
			break;

		case Qt::Key_Delete:
			deleteSelectedValues();
			ke->accept();
			break;*/

		case Qt::Key_Home:
			m_timeLine->pos().setTicks( 0 );
			m_timeLine->updatePosition();
			ke->accept();
			break;

		default:
			break;
	}
}




void AutomationEditor::leaveEvent(QEvent * e )
{
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}

	QWidget::leaveEvent( e );
}


void AutomationEditor::drawLine( int x0, float y0, int x1, float y1 )
{
	int deltax = qRound( qAbs<float>( x1 - x0 ) );
	float deltay = qAbs<float>( y1 - y0 );
	int x = x0;
	float y = y0;
	int xstep;
	int ystep;

	if( deltax < quantization() )
	{
		return;
	}

	deltax /= quantization();

	float yscale = deltay / ( deltax );

	if( x0 < x1)
	{
		xstep = quantization();
	}
	else
	{
		xstep = -( quantization() );
	}

	if( y0 < y1 )
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	int i = 0;
	while( i < deltax )
	{
		y = y0 + ( ystep * yscale * i );

		x += xstep;
		i += 1;
		m_pattern->removeValue( MidiTime( x ) );
		m_pattern->putValue( MidiTime( x ), y );
	}
}




void AutomationEditor::mousePressEvent( QMouseEvent* mouseEvent )
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		return;
	}

	if( mouseEvent->y() > TOP_MARGIN )
	{
		float level = getLevel( mouseEvent->y() );

		int x = mouseEvent->x();

		if( x > VALUES_WIDTH )
		{
			// set or move value

			x -= VALUES_WIDTH;

			// get tick in which the user clicked
			int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
							m_currentPosition;

			// get time map of current pattern
			timeMap & time_map = m_pattern->getTimeMap();

			// will be our iterator in the following loop
			timeMap::iterator it = time_map.begin();

			// loop through whole time-map...
			while( it != time_map.end() )
			{
				MidiTime len = 4;

				// and check whether the user clicked on an
				// existing value
				if( pos_ticks >= it.key() &&
					len > 0 &&
					( it+1==time_map.end() ||
						pos_ticks <= (it+1).key() ) &&
		( pos_ticks<= it.key() + MidiTime::ticksPerTact() *4 / m_ppt ) &&
					level <= it.value() )
				{
					break;
				}
				++it;
			}

			// left button??
			if( mouseEvent->button() == Qt::LeftButton &&
							m_editMode == DRAW )
			{
				// Connect the dots
				if( mouseEvent->modifiers() & Qt::ShiftModifier )
				{
					drawLine( m_drawLastTick,
							m_drawLastLevel,
							pos_ticks, level );
				}
				m_drawLastTick = pos_ticks;
				m_drawLastLevel = level;

				// did it reach end of map because
				// there's no value??
				if( it == time_map.end() )
				{
					// then set new value
					MidiTime value_pos( pos_ticks );

					MidiTime new_time =
						m_pattern->setDragValue( value_pos,
									level );

					// reset it so that it can be used for
					// ops (move, resize) after this
					// code-block
					it = time_map.find( new_time );
				}

				// move it
				m_action = MOVE_VALUE;
				int aligned_x = (int)( (float)( (
						it.key() -
						m_currentPosition ) *
						m_ppt ) / MidiTime::ticksPerTact() );
				m_moveXOffset = x - aligned_x - 1;
				// set move-cursor
				QCursor c( Qt::SizeAllCursor );
				QApplication::setOverrideCursor( c );

				Engine::getSong()->setModified();
			}
			else if( ( mouseEvent->button() == Qt::RightButton &&
							m_editMode == DRAW ) ||
					m_editMode == ERASE )
			{
				// erase single value
				if( it != time_map.end() )
				{
					m_pattern->removeValue( it.key() );
					Engine::getSong()->setModified();
				}
				m_action = NONE;
			}
			else if( mouseEvent->button() == Qt::LeftButton &&
							m_editMode == SELECT )
			{
				// select an area of values

				m_selectStartTick = pos_ticks;
				m_selectedTick = 0;
				m_selectStartLevel = level;
				m_selectedLevels = 1;
				m_action = SELECT_VALUES;
			}
			else if( mouseEvent->button() == Qt::RightButton &&
							m_editMode == SELECT )
			{
				// when clicking right in select-move, we
				// switch to move-mode
				//m_moveButton->setChecked( true );
			}
			else if( mouseEvent->button() == Qt::LeftButton &&
							m_editMode == MOVE )
			{
				// move selection (including selected values)

				// save position where move-process began
				m_moveStartTick = pos_ticks;
				m_moveStartLevel = level;

				m_action = MOVE_SELECTION;

				Engine::getSong()->setModified();
			}
			else if( mouseEvent->button() == Qt::RightButton &&
							m_editMode == MOVE )
			{
				// when clicking right in select-move, we
				// switch to draw-mode
				//m_drawButton->setChecked( true );
			}

			update();
		}
	}
}




void AutomationEditor::mouseReleaseEvent(QMouseEvent * mouseEvent )
{
	if( m_editMode == DRAW )
	{
		if( m_action == MOVE_VALUE )
		{
			m_pattern->applyDragValue();
		}
		QApplication::restoreOverrideCursor();
	}

	m_action = NONE;
}



#include <stdio.h>
void AutomationEditor::mouseMoveEvent(QMouseEvent * mouseEvent )
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		update();
		return;
	}

	if( mouseEvent->y() > TOP_MARGIN )
	{
		float level = getLevel( mouseEvent->y() );
		int x = mouseEvent->x();

		if( mouseEvent->x() <= VALUES_WIDTH )
		{
			update();
			return;
		}
		x -= VALUES_WIDTH;
		if( m_action == MOVE_VALUE )
		{
			x -= m_moveXOffset;
		}

		int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
							m_currentPosition;
		if( mouseEvent->buttons() & Qt::LeftButton && m_editMode == DRAW )
		{
			if( m_action == MOVE_VALUE )
			{
				// moving value
				if( pos_ticks < 0 )
				{
					pos_ticks = 0;
				}

				drawLine( m_drawLastTick, m_drawLastLevel,
							pos_ticks, level );

				m_drawLastTick = pos_ticks;
				m_drawLastLevel = level;

				// we moved the value so the value has to be
				// moved properly according to new starting-
				// time in the time map of pattern
				m_pattern->setDragValue( MidiTime( pos_ticks ),
								level );
			}

			Engine::getSong()->setModified();

		}
		else if( ( mouseEvent->buttons() & Qt::RightButton &&
						m_editMode == DRAW ) ||
				( mouseEvent->buttons() & Qt::LeftButton &&
						m_editMode == ERASE ) )
		{
			m_pattern->removeValue( MidiTime( pos_ticks ) );
		}
		else if( mouseEvent->buttons() & Qt::NoButton && m_editMode == DRAW )
		{
			// set move- or resize-cursor

			// get time map of current pattern
			timeMap & time_map = m_pattern->getTimeMap();

			// will be our iterator in the following loop
			timeMap::iterator it = time_map.begin();
			// loop through whole time map...
			for( ; it != time_map.end(); ++it )
			{
				// and check whether the cursor is over an
				// existing value
				if( pos_ticks >= it.key() &&
					( it+1==time_map.end() ||
						pos_ticks <= (it+1).key() ) &&
							level <= it.value() )
				{
					break;
				}
			}

			// did it reach end of map because there's
			// no value??
			if( it != time_map.end() )
			{
				if( QApplication::overrideCursor() )
				{
	if( QApplication::overrideCursor()->shape() != Qt::SizeAllCursor )
					{
				while( QApplication::overrideCursor() != NULL )
				{
					QApplication::restoreOverrideCursor();
				}

						QCursor c( Qt::SizeAllCursor );
						QApplication::setOverrideCursor(
									c );
					}
				}
				else
				{
					QCursor c( Qt::SizeAllCursor );
					QApplication::setOverrideCursor( c );
				}
			}
			else
			{
				// the cursor is over no value, so restore
				// cursor
				while( QApplication::overrideCursor() != NULL )
				{
					QApplication::restoreOverrideCursor();
				}
			}
		}
		else if( mouseEvent->buttons() & Qt::LeftButton &&
						m_editMode == SELECT &&
						m_action == SELECT_VALUES )
		{

			// change size of selection

			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint(
						VALUES_WIDTH, mouseEvent->y() ) ) );
				if( m_currentPosition >= 4 )
				{
					m_leftRightScroll->setValue(
							m_currentPosition - 4 );
				}
				else
				{
					m_leftRightScroll->setValue( 0 );
				}
			}
			else if( x > width() - VALUES_WIDTH )
			{
				x = width() - VALUES_WIDTH;
				QCursor::setPos( mapToGlobal( QPoint( width(),
								mouseEvent->y() ) ) );
				m_leftRightScroll->setValue( m_currentPosition +
									4 );
			}

			// get tick in which the cursor is posated
			int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
							m_currentPosition;

			m_selectedTick = pos_ticks - m_selectStartTick;
			if( (int) m_selectStartTick + m_selectedTick < 0 )
			{
				m_selectedTick = -m_selectStartTick;
			}
			m_selectedLevels = level - m_selectStartLevel;
			if( level <= m_selectStartLevel )
			{
				--m_selectedLevels;
			}
		}
		else if( mouseEvent->buttons() & Qt::LeftButton &&
					m_editMode == MOVE &&
					m_action == MOVE_SELECTION )
		{
			// move selection + selected values

			// do horizontal move-stuff
			int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
							m_currentPosition;
			int ticks_diff = pos_ticks -
							m_moveStartTick;
			if( m_selectedTick > 0 )
			{
				if( (int) m_selectStartTick +
							ticks_diff < 0 )
				{
					ticks_diff = -m_selectStartTick;
				}
			}
			else
			{
				if( (int) m_selectStartTick +
					m_selectedTick + ticks_diff <
									0 )
				{
					ticks_diff = -(
							m_selectStartTick +
							m_selectedTick );
				}
			}
			m_selectStartTick += ticks_diff;

			int tact_diff = ticks_diff / MidiTime::ticksPerTact();
			ticks_diff = ticks_diff % MidiTime::ticksPerTact();


			// do vertical move-stuff
			float level_diff = level - m_moveStartLevel;

			if( m_selectedLevels > 0 )
			{
				if( m_selectStartLevel + level_diff
								< m_minLevel )
				{
					level_diff = m_minLevel -
							m_selectStartLevel;
				}
				else if( m_selectStartLevel + m_selectedLevels +
						level_diff > m_maxLevel )
				{
					level_diff = m_maxLevel -
							m_selectStartLevel -
							m_selectedLevels;
				}
			}
			else
			{
				if( m_selectStartLevel + m_selectedLevels +
						level_diff < m_minLevel )
				{
					level_diff = m_minLevel -
							m_selectStartLevel -
							m_selectedLevels;
				}
				else if( m_selectStartLevel + level_diff >
								m_maxLevel )
				{
					level_diff = m_maxLevel -
							m_selectStartLevel;
				}
			}
			m_selectStartLevel += level_diff;


			timeMap new_selValuesForMove;
			for( timeMap::iterator it = m_selValuesForMove.begin();
					it != m_selValuesForMove.end(); ++it )
			{
				MidiTime new_value_pos;
				if( it.key() )
				{
					int value_tact =
						( it.key() /
							MidiTime::ticksPerTact() )
								+ tact_diff;
					int value_ticks =
						( it.key() %
							MidiTime::ticksPerTact() )
								+ ticks_diff;
					// ensure value_ticks range
					if( value_ticks / MidiTime::ticksPerTact() )
					{
						value_tact += value_ticks
							/ MidiTime::ticksPerTact();
						value_ticks %=
							MidiTime::ticksPerTact();
					}
					m_pattern->removeValue( it.key() );
					new_value_pos = MidiTime( value_tact,
							value_ticks );
				}
				new_selValuesForMove[
					m_pattern->putValue( new_value_pos,
						it.value () + level_diff,
									false )]
						= it.value() + level_diff;
			}
			m_selValuesForMove = new_selValuesForMove;

			m_moveStartTick = pos_ticks;
			m_moveStartLevel = level;
		}
	}
	else
	{
		if( mouseEvent->buttons() & Qt::LeftButton &&
					m_editMode == SELECT &&
					m_action == SELECT_VALUES )
		{

			int x = mouseEvent->x() - VALUES_WIDTH;
			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint( VALUES_WIDTH,
								mouseEvent->y() ) ) );
				if( m_currentPosition >= 4 )
				{
					m_leftRightScroll->setValue(
							m_currentPosition - 4 );
				}
				else
				{
					m_leftRightScroll->setValue( 0 );
				}
			}
			else if( x > width() - VALUES_WIDTH )
			{
				x = width() - VALUES_WIDTH;
				QCursor::setPos( mapToGlobal( QPoint( width(),
							mouseEvent->y() ) ) );
				m_leftRightScroll->setValue( m_currentPosition +
									4 );
			}

			// get tick in which the cursor is posated
			int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
							m_currentPosition;

			m_selectedTick = pos_ticks -
							m_selectStartTick;
			if( (int) m_selectStartTick + m_selectedTick <
									0 )
			{
				m_selectedTick = -m_selectStartTick;
			}

			float level = getLevel( mouseEvent->y() );

			if( level <= m_bottomLevel )
			{
				QCursor::setPos( mapToGlobal( QPoint( mouseEvent->x(),
							height() -
							SCROLLBAR_SIZE ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
				level = m_bottomLevel;
			}
			else if( level >= m_topLevel )
			{
				QCursor::setPos( mapToGlobal( QPoint( mouseEvent->x(),
							TOP_MARGIN ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() - 1 );
				level = m_topLevel;
			}
			m_selectedLevels = level - m_selectStartLevel;
			if( level <= m_selectStartLevel )
			{
				--m_selectedLevels;
			}
		}
		QApplication::restoreOverrideCursor();
	}

	update();
}




inline void AutomationEditor::drawCross( QPainter & p )
{
	QPoint mouse_pos = mapFromGlobal( QCursor::pos() );
	float level = getLevel( mouse_pos.y() );
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	float cross_y = m_y_auto ?
		grid_bottom - ( ( grid_bottom - TOP_MARGIN )
				* ( level - m_minLevel )
				/ (float)( m_maxLevel - m_minLevel ) ) :
		grid_bottom - ( level - m_bottomLevel ) * m_y_delta;

	p.setPen( QColor( 0xFF, 0x33, 0x33 ) );
	p.drawLine( VALUES_WIDTH, (int) cross_y, width(), (int) cross_y );
	p.drawLine( mouse_pos.x(), TOP_MARGIN, mouse_pos.x(),
						height() - SCROLLBAR_SIZE );
	QPoint tt_pos =  QCursor::pos();
	tt_pos.ry() -= 64;
	tt_pos.rx() += 32;
	float scaledLevel = m_pattern->firstObject()->scaledValue( level );
	QToolTip::showText( tt_pos, QString::number( scaledLevel ), this );
}




inline void AutomationEditor::drawAutomationPoint( QPainter & p, timeMap::iterator it )
{
	int x = xCoordOfTick( it.key() );
	int y = yCoordOfLevel( it.value() );
	const int outerRadius = qBound( 2, ( m_ppt * quantization() ) / 576, 5 ); // man, getting this calculation right took forever
	p.setPen( QPen( vertexColor().lighter( 200 ) ) );
	p.setBrush( QBrush( vertexColor() ) );
	p.drawEllipse( x - outerRadius, y - outerRadius, outerRadius * 2, outerRadius * 2 );
}




void AutomationEditor::paintEvent(QPaintEvent * pe )
{
	QMutexLocker m( &m_patternMutex );

	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );

	// get foreground color
	QColor fgColor = p.pen().brush().color();
	// get background color and fill background
	QBrush bgColor = p.background();
	p.fillRect( 0, 0, width(), height(), bgColor );

	// set font-size to 8
	p.setFont( pointSize<8>( p.font() ) );

	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;

	// start drawing at the bottom
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;

	p.fillRect( 0, TOP_MARGIN, VALUES_WIDTH, height() - TOP_MARGIN,
						scaleColor() );

	// print value numbers
	int font_height = p.fontMetrics().height();
	Qt::Alignment text_flags =
		(Qt::Alignment)( Qt::AlignRight | Qt::AlignVCenter );

	if( validPattern() )
	{
		if( m_y_auto )
		{
			int y[] = { grid_bottom, TOP_MARGIN + font_height / 2 };
			float level[] = { m_minLevel, m_maxLevel };
			for( int i = 0; i < 2; ++i )
			{
				const QString & label = m_pattern->firstObject()
						->displayValue( level[i] );
				p.setPen( QApplication::palette().color( QPalette::Active,
							QPalette::Shadow ) );
				p.drawText( 1, y[i] - font_height + 1,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
				p.setPen( fgColor );
				p.drawText( 0, y[i] - font_height,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
			}
		}
		else
		{
			int y;
			int level = (int) m_bottomLevel;
			int printable = qMax( 1, 5 * DEFAULT_Y_DELTA
								/ m_y_delta );
			int module = level % printable;
			if( module )
			{
				int inv_module = ( printable - module )
								% printable;
				level += inv_module;
			}
			for( ; level <= m_topLevel; level += printable )
			{
				const QString & label = m_pattern->firstObject()
							->displayValue( level );
				y = yCoordOfLevel( level );
				p.setPen( QApplication::palette().color( QPalette::Active,
							QPalette::Shadow ) );
				p.drawText( 1, y - font_height + 1,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
				p.setPen( fgColor );
				p.drawText( 0, y - font_height,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
			}
		}
	}

	// set clipping area, because we are not allowed to paint over
	// keyboard...
	p.setClipRect( VALUES_WIDTH, TOP_MARGIN, width() - VALUES_WIDTH,
								grid_height  );

	// draw vertical raster
	QColor lineColor = QColor( gridColor() );
	if( m_pattern )
	{
		int tick, x;
		int x_line_end = (int)( m_y_auto || m_topLevel < m_maxLevel ?
			TOP_MARGIN :
			grid_bottom - ( m_topLevel - m_bottomLevel )
								* m_y_delta );
		// 3 independent loops, because quantization might not divide evenly into
		// exotic denominators (e.g. 7/11 time), which are allowed ATM.
		// First quantization grid...
		for( tick = m_currentPosition - m_currentPosition % quantization(),
				 x = xCoordOfTick( tick );
			 x<=width();
			 tick += quantization(), x = xCoordOfTick( tick ) )
		{
			lineColor.setAlpha( 80 );
			p.setPen( lineColor );
			p.drawLine( x, grid_bottom, x, x_line_end );
		}
		// Then beat grid
		int ticksPerBeat = DefaultTicksPerTact /
			Engine::getSong()->getTimeSigModel().getDenominator();
		for( tick = m_currentPosition - m_currentPosition % ticksPerBeat,
				 x = xCoordOfTick( tick );
			 x<=width();
			 tick += ticksPerBeat, x = xCoordOfTick( tick ) )
		{
			lineColor.setAlpha( 160 );
			p.setPen( lineColor );
			p.drawLine( x, grid_bottom, x, x_line_end );
		}
		// and finally bars
		for( tick = m_currentPosition - m_currentPosition % MidiTime::ticksPerTact(),
				 x = xCoordOfTick( tick );
			 x<=width();
			 tick += MidiTime::ticksPerTact(), x = xCoordOfTick( tick ) )
		{
			lineColor.setAlpha( 255 );
			p.setPen( lineColor );
			p.drawLine( x, grid_bottom, x, x_line_end );
		}

		/// \todo move this horizontal line drawing code into the same loop as the value ticks?
		if( m_y_auto )
		{
			lineColor.setAlpha( 160 );
			QPen pen( lineColor );
			p.setPen( pen );
			p.drawLine( VALUES_WIDTH, grid_bottom, width(),
								grid_bottom );
			pen.setStyle( Qt::DotLine );
			p.setPen( pen );
			float y_delta = ( grid_bottom - TOP_MARGIN ) / 8.0f;
			for( int i = 1; i < 8; ++i )
			{
				int y = (int)( grid_bottom - i * y_delta );
				p.drawLine( VALUES_WIDTH, y, width(), y );
			}
		}
		else
		{
			float y;
			for( int level = (int)m_bottomLevel; level <= m_topLevel; level++)
			{
				y =  yCoordOfLevel( (float)level );
				if( level % 5 == 0 )
				{
					lineColor.setAlpha( 160 );
					p.setPen( lineColor );
				}
				else
				{
					lineColor.setAlpha( 80 );
					p.setPen( lineColor );
				}

				// draw level line
				p.drawLine( VALUES_WIDTH, (int) y, width(),
								(int) y );
			}
		}
	}



	// following code draws all visible values

	// setup selection-vars
	int sel_pos_start = m_selectStartTick;
	int sel_pos_end = m_selectStartTick + m_selectedTick;
	if( sel_pos_start > sel_pos_end )
	{
		qSwap<int>( sel_pos_start, sel_pos_end );
	}

	float selLevel_start = m_selectStartLevel;
	float selLevel_end = selLevel_start + m_selectedLevels;
	if( selLevel_start > selLevel_end )
	{
		qSwap<float>( selLevel_start, selLevel_end );
	}

	if( validPattern() )
	{
		int len_ticks = 4;
		timeMap & time_map = m_pattern->getTimeMap();

		//Don't bother doing/rendering anything if there is no automation points
		if( time_map.size() > 0 )
		{
			timeMap::iterator it = time_map.begin();			
			while( it+1 != time_map.end() )
			{
				// skip this section if it occurs completely before the
				// visible area
				int next_x = xCoordOfTick( (it+1).key() );
				if( next_x < 0 )
				{
					++it;
					continue;
				}

				int x = xCoordOfTick( it.key() );
				if( x > width() )
				{
					break;
				}

				bool is_selected = false;
				// if we're in move-mode, we may only draw
				// values in selected area, that have originally
				// been selected and not values that are now in
				// selection because the user moved it...
				if( m_editMode == MOVE )
				{
					if( m_selValuesForMove.contains( it.key() ) )
					{
						is_selected = true;
					}
				}
				else if( it.value() >= selLevel_start &&
					it.value() <= selLevel_end &&
					it.key() >= sel_pos_start &&
					it.key() + len_ticks <= sel_pos_end )
				{
					is_selected = true;
				}
				
				float *values = m_pattern->valuesAfter( it.key() );
				for( int i = 0; i < (it+1).key() - it.key(); i++ )
				{
					
					drawLevelTick( p, it.key() + i, values[i],
									is_selected );
				}
				delete [] values;

				// Draw circle
				drawAutomationPoint(p, it);

				++it;
			}

			for( int i = it.key(), x = xCoordOfTick( i ); x <= width();
							i++, x = xCoordOfTick( i ) )
			{
				// TODO: Find out if the section after the last control
				// point is able to be selected and if so set this
				// boolean correctly
				drawLevelTick( p, i, it.value(), false );
			}
			// Draw circle(the last one)
			drawAutomationPoint(p, it);
		}
	}
	else
	{
		QFont f = p.font();
		f.setBold( true );
		p.setFont( pointSize<14>( f ) );
		p.setPen( QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ) );
		p.drawText( VALUES_WIDTH + 20, TOP_MARGIN + 40,
				width() - VALUES_WIDTH - 20 - SCROLLBAR_SIZE,
				grid_height - 40, Qt::TextWordWrap,
				tr( "Please open an automation pattern with "
					"the context menu of a control!" ) );
	}

	// now draw selection-frame
	int x = ( sel_pos_start - m_currentPosition ) * m_ppt /
							MidiTime::ticksPerTact();
	int w = ( sel_pos_end - sel_pos_start ) * m_ppt / MidiTime::ticksPerTact();
	int y, h;
	if( m_y_auto )
	{
		y = (int)( grid_bottom - ( ( grid_bottom - TOP_MARGIN )
				* ( selLevel_start - m_minLevel )
				/ (float)( m_maxLevel - m_minLevel ) ) );
		h = (int)( grid_bottom - ( ( grid_bottom - TOP_MARGIN )
				* ( selLevel_end - m_minLevel )
				/ (float)( m_maxLevel - m_minLevel ) ) - y );
	}
	else
	{
		y = (int)( grid_bottom - ( selLevel_start - m_bottomLevel )
								* m_y_delta );
		h = (int)( ( selLevel_start - selLevel_end ) * m_y_delta );
	}
	p.setPen( QColor( 0, 64, 192 ) );
	p.drawRect( x + VALUES_WIDTH, y, w, h );

	// TODO: Get this out of paint event
	int l = validPattern() ? (int) m_pattern->length() : 0;

	// reset scroll-range
	if( m_leftRightScroll->maximum() != l )
	{
		m_leftRightScroll->setRange( 0, l );
		m_leftRightScroll->setPageStep( l );
	}

	if( validPattern() )
	{
		drawCross( p );
	}

	const QPixmap * cursor = NULL;
	// draw current edit-mode-icon below the cursor
	switch( m_editMode )
	{
		case DRAW: cursor = s_toolDraw; break;
		case ERASE: cursor = s_toolErase; break;
		case SELECT: cursor = s_toolSelect; break;
		case MOVE: cursor = s_toolMove; break;
	}
	p.drawPixmap( mapFromGlobal( QCursor::pos() ) + QPoint( 8, 8 ),
								*cursor );
}




int AutomationEditor::xCoordOfTick(int tick )
{
	return VALUES_WIDTH + ( ( tick - m_currentPosition )
						* m_ppt / MidiTime::ticksPerTact() );
}




int AutomationEditor::yCoordOfLevel(float level )
{
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	if( m_y_auto )
	{
		return (int)( grid_bottom - ( grid_bottom - TOP_MARGIN )
						* ( level - m_minLevel )
						/ ( m_maxLevel - m_minLevel ) );
	}
	else
	{
		return (int)( grid_bottom - ( level - m_bottomLevel )
								* m_y_delta );
	}
}




void AutomationEditor::drawLevelTick(QPainter & p, int tick, float value,
							bool is_selected )
{
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	const int x = xCoordOfTick( tick );
	int rect_width = xCoordOfTick( tick+1 ) - x;

	// is the level in visible area?
	if( ( value >= m_bottomLevel && value <= m_topLevel )
			|| ( value > m_topLevel && m_topLevel >= 0 )
			|| ( value < m_bottomLevel && m_bottomLevel <= 0 ) )
	{
		int y_start = yCoordOfLevel( value );
		int rect_height;

		if( m_y_auto )
		{
			int y_end = (int)( grid_bottom
						+ ( grid_bottom - TOP_MARGIN )
						* m_minLevel
						/ ( m_maxLevel - m_minLevel ) );

			rect_height = y_end - y_start;
		}
		else
		{
			rect_height = (int)( value * m_y_delta );
		}

		QBrush currentColor = is_selected
			? QBrush( QColor( 0x00, 0x40, 0xC0 ) )
			: graphColor();

		p.fillRect( x, y_start, rect_width, rect_height, currentColor );
	}
	
	else
	{
		printf("not in range\n");
	}
	
}




// responsible for moving/resizing scrollbars after window-resizing
void AutomationEditor::resizeEvent(QResizeEvent * re)
{
	m_leftRightScroll->setGeometry( VALUES_WIDTH, height() - SCROLLBAR_SIZE,
							width() - VALUES_WIDTH,
							SCROLLBAR_SIZE );

	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;
	m_topBottomScroll->setGeometry( width() - SCROLLBAR_SIZE, TOP_MARGIN,
						SCROLLBAR_SIZE, grid_height );

	int half_grid = grid_height / 2;
	int total_pixels = (int)( ( m_maxLevel - m_minLevel ) * m_y_delta + 1 );
	if( !m_y_auto && grid_height < total_pixels )
	{
		int min_scroll = (int)( m_minLevel + floorf( half_grid
							/ (float)m_y_delta ) );
		int max_scroll = (int)( m_maxLevel - (int)floorf( ( grid_height
					- half_grid ) / (float)m_y_delta ) );
		m_topBottomScroll->setRange( min_scroll, max_scroll );
	}
	else
	{
		m_topBottomScroll->setRange( (int) m_scrollLevel,
							(int) m_scrollLevel );
	}

	m_topBottomScroll->setValue( (int) m_scrollLevel );

	if( Engine::getSong() )
	{
		Engine::getSong()->getPlayPos( Song::Mode_PlayAutomationPattern
					).m_timeLine->setFixedWidth( width() );
	}

	updateTopBottomLevels();
	update();
}




void AutomationEditor::wheelEvent(QWheelEvent * we )
{
	we->accept();
	if( we->modifiers() & Qt::ControlModifier && we->modifiers() & Qt::ShiftModifier )
	{
		int y = m_zoomingYModel.value();
		if( we->delta() > 0 )
		{
			y++;
		}
		if( we->delta() < 0 )
		{
			y--;
		}
		y = qBound( 0, y, m_zoomingYModel.size() - 1 );
		m_zoomingYModel.setValue( y );	
	}
	else if( we->modifiers() & Qt::ControlModifier && we->modifiers() & Qt::AltModifier )
	{
		int q = m_quantizeModel.value();
		if( we->delta() > 0 )
		{
			q--;
		}
		if( we->delta() < 0 )
		{
			q++;
		}
		q = qBound( 0, q, m_quantizeModel.size() - 1 );
		m_quantizeModel.setValue( q );
		update();
	}
	else if( we->modifiers() & Qt::ControlModifier )
	{
		int x = m_zoomingXModel.value();
		if( we->delta() > 0 )
		{
			x++;
		}
		if( we->delta() < 0 )
		{
			x--;
		}
		x = qBound( 0, x, m_zoomingXModel.size() - 1 );
		m_zoomingXModel.setValue( x );
	}
	else if( we->modifiers() & Qt::ShiftModifier
			|| we->orientation() == Qt::Horizontal )
	{
		m_leftRightScroll->setValue( m_leftRightScroll->value() -
							we->delta() * 2 / 15 );
	}
	else
	{
		m_topBottomScroll->setValue( m_topBottomScroll->value() -
							we->delta() / 30 );
	}
}




float AutomationEditor::getLevel(int y )
{
	int level_line_y = height() - SCROLLBAR_SIZE - 1;
	// pressed level
	float level = roundf( ( m_bottomLevel + ( m_y_auto ?
			( m_maxLevel - m_minLevel ) * ( level_line_y - y )
					/ (float)( level_line_y - ( TOP_MARGIN + 2 ) ) :
			( level_line_y - y ) / (float)m_y_delta ) ) / m_step ) * m_step;
	// some range-checking-stuff
	level = qBound( m_bottomLevel, level, m_topLevel );

	return( level );
}




inline bool AutomationEditor::inBBEditor()
{
	QMutexLocker m( &m_patternMutex );
	return( validPattern() &&
				m_pattern->getTrack()->trackContainer() == Engine::getBBTrackContainer() );
}




void AutomationEditor::play()
{
	QMutexLocker m( &m_patternMutex );

	if( !validPattern() )
	{
		return;
	}

	if( !m_pattern->getTrack() )
	{
		if( Engine::getSong()->playMode() != Song::Mode_PlayPattern )
		{
			Engine::getSong()->stop();
			Engine::getSong()->playPattern( (Pattern *) Engine::pianoRoll()->currentPattern() );
		}
		else if( Engine::getSong()->isStopped() == false )
		{
			Engine::getSong()->togglePause();
		}
		else
		{
			Engine::getSong()->playPattern( (Pattern *) Engine::pianoRoll()->currentPattern() );
		}
	}
	else if( inBBEditor() )
	{
		Engine::getBBTrackContainer()->play();
	}
	else
	{
		if( Engine::getSong()->isStopped() == true )
		{
			Engine::getSong()->playSong();
		}
		else
		{
			Engine::getSong()->togglePause();
		}
	}
}




void AutomationEditor::stop()
{
	QMutexLocker m( &m_patternMutex );

	if( !validPattern() )
	{
		return;
	}
	if( m_pattern->getTrack() && inBBEditor() )
	{
		Engine::getBBTrackContainer()->stop();
	}
	else
	{
		Engine::getSong()->stop();
	}
	m_scrollBack = true;
}




void AutomationEditor::horScrolled(int new_pos )
{
	m_currentPosition = new_pos;
	emit positionChanged( m_currentPosition );
	update();
}




void AutomationEditor::verScrolled(int new_pos )
{
	m_scrollLevel = new_pos;
	updateTopBottomLevels();
	update();
}




void AutomationEditor::setEditMode(AutomationEditor::EditModes mode)
{
	if (m_editMode == mode)
		return;

	m_editMode = mode;
	switch (mode)
	{
	case DRAW:
	case ERASE:
	case SELECT:
		removeSelection();
		break;
	case MOVE:
		m_selValuesForMove.clear();
		getSelectedValues(m_selValuesForMove);
	}
	update();
}



void AutomationEditor::setEditMode(int mode)
{
	setEditMode((AutomationEditor::EditModes) mode);
}




void AutomationEditor::setProgressionType(AutomationPattern::ProgressionTypes type)
{
	if (validPattern())
	{
		QMutexLocker m(&m_patternMutex);
		m_pattern->setProgressionType(type);
		Engine::getSong()->setModified();
		update();
	}
}


void AutomationEditor::setProgressionDiscrete()
{
	setProgressionType(AutomationPattern::DiscreteProgression);
}


void AutomationEditor::setProgressionLinear()
{
	setProgressionType(AutomationPattern::LinearProgression);
}


void AutomationEditor::setProgressionHermite()
{
	setProgressionType(AutomationPattern::CubicHermiteProgression);
}




void AutomationEditor::setTension()
{
	m_pattern->setTension( QString::number( m_tensionModel->value() ) );
	update();
}



void AutomationEditor::selectAll()
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		return;
	}

	timeMap & time_map = m_pattern->getTimeMap();

	timeMap::iterator it = time_map.begin();
	m_selectStartTick = 0;
	m_selectedTick = m_pattern->length();
	m_selectStartLevel = it.value();
	m_selectedLevels = 1;

	while( ++it != time_map.end() )
	{
		const float level = it.value();
		if( level < m_selectStartLevel )
		{
			// if we move start-level down, we have to add
			// the difference between old and new start-level
			// to m_selectedLevels, otherwise the selection
			// is just moved down...
			m_selectedLevels += m_selectStartLevel - level;
			m_selectStartLevel = level;
		}
		else if( level >= m_selectStartLevel + m_selectedLevels )
		{
			m_selectedLevels = level - m_selectStartLevel + 1;
		}
	}
}




// returns vector with pointers to all selected values
void AutomationEditor::getSelectedValues( timeMap & selected_values )
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		return;
	}

	int sel_pos_start = m_selectStartTick;
	int sel_pos_end = sel_pos_start + m_selectedTick;
	if( sel_pos_start > sel_pos_end )
	{
		qSwap<int>( sel_pos_start, sel_pos_end );
	}

	float selLevel_start = m_selectStartLevel;
	float selLevel_end = selLevel_start + m_selectedLevels;
	if( selLevel_start > selLevel_end )
	{
		qSwap<float>( selLevel_start, selLevel_end );
	}

	timeMap & time_map = m_pattern->getTimeMap();

	for( timeMap::iterator it = time_map.begin(); it != time_map.end();
									++it )
	{
		//TODO: Add constant
		tick_t len_ticks = MidiTime::ticksPerTact() / 16;

		float level = it.value();
		tick_t pos_ticks = it.key();

		if( level >= selLevel_start && level <= selLevel_end &&
				pos_ticks >= sel_pos_start &&
				pos_ticks + len_ticks <= sel_pos_end )
		{
			selected_values[it.key()] = level;
		}
	}
}




void AutomationEditor::copySelectedValues()
{
	m_valuesToCopy.clear();

	timeMap selected_values;
	getSelectedValues( selected_values );

	if( !selected_values.isEmpty() )
	{
		for( timeMap::iterator it = selected_values.begin();
			it != selected_values.end(); ++it )
		{
			m_valuesToCopy[it.key()] = it.value();
		}
		TextFloat::displayMessage( tr( "Values copied" ),
				tr( "All selected values were copied to the "
								"clipboard." ),
				embed::getIconPixmap( "edit_copy" ), 2000 );
	}
}




void AutomationEditor::cutSelectedValues()
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		return;
	}

	m_valuesToCopy.clear();

	timeMap selected_values;
	getSelectedValues( selected_values );

	if( !selected_values.isEmpty() )
	{
		Engine::getSong()->setModified();

		for( timeMap::iterator it = selected_values.begin();
					it != selected_values.end(); ++it )
		{
			m_valuesToCopy[it.key()] = it.value();
			m_pattern->removeValue( it.key() );
		}
	}

	update();
	Engine::songEditor()->update();
}




void AutomationEditor::pasteValues()
{
	QMutexLocker m( &m_patternMutex );
	if( validPattern() && !m_valuesToCopy.isEmpty() )
	{
		for( timeMap::iterator it = m_valuesToCopy.begin();
					it != m_valuesToCopy.end(); ++it )
		{
			m_pattern->putValue( it.key() + m_currentPosition,
								it.value() );
		}

		// we only have to do the following lines if we pasted at
		// least one value...
		Engine::getSong()->setModified();
		update();
		Engine::songEditor()->update();
	}
}




void AutomationEditor::deleteSelectedValues()
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		return;
	}

	timeMap selected_values;
	getSelectedValues( selected_values );

	const bool update_after_delete = !selected_values.empty();

	for( timeMap::iterator it = selected_values.begin();
					it != selected_values.end(); ++it )
	{
		m_pattern->removeValue( it.key() );
	}

	if( update_after_delete == true )
	{
		Engine::getSong()->setModified();
		update();
		Engine::songEditor()->update();
	}
}




void AutomationEditor::updatePosition(const MidiTime & t )
{
	if( ( Engine::getSong()->isPlaying() &&
			Engine::getSong()->playMode() ==
					Song::Mode_PlayAutomationPattern ) ||
							m_scrollBack == true )
	{
		const int w = width() - VALUES_WIDTH;
		if( t > m_currentPosition + w * MidiTime::ticksPerTact() / m_ppt )
		{
			m_leftRightScroll->setValue( t.getTact() *
							MidiTime::ticksPerTact() );
		}
		else if( t < m_currentPosition )
		{
			MidiTime t_ = qMax( t - w * MidiTime::ticksPerTact() *
					MidiTime::ticksPerTact() / m_ppt, 0 );
			m_leftRightScroll->setValue( t_.getTact() *
							MidiTime::ticksPerTact() );
		}
		m_scrollBack = false;
	}
}




void AutomationEditor::zoomingXChanged()
{
	const QString & zfac = m_zoomingXModel.currentText();
	m_ppt = zfac.left( zfac.length() - 1 ).toInt() * DEFAULT_PPT / 100;
#ifdef LMMS_DEBUG
	assert( m_ppt > 0 );
#endif
	m_timeLine->setPixelsPerTact( m_ppt );
	update();
}




void AutomationEditor::zoomingYChanged()
{
	const QString & zfac = m_zoomingYModel.currentText();
	m_y_auto = zfac == "Auto";
	if( !m_y_auto )
	{
		m_y_delta = zfac.left( zfac.length() - 1 ).toInt()
							* DEFAULT_Y_DELTA / 100;
	}
#ifdef LMMS_DEBUG
	assert( m_y_delta > 0 );
#endif
	resizeEvent( NULL );
}




int AutomationEditor::quantization() const
{
	return DefaultTicksPerTact / (1 << m_quantizeModel.value());
}




void AutomationEditor::updateTopBottomLevels()
{
	if( m_y_auto )
	{
		m_bottomLevel = m_minLevel;
		m_topLevel = m_maxLevel;
		return;
	}

	int total_pixels = (int)( ( m_maxLevel - m_minLevel ) * m_y_delta + 1 );
	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;
	int half_grid = grid_height / 2;

	if( total_pixels > grid_height )
	{
		int centralLevel = (int)( m_minLevel + m_maxLevel - m_scrollLevel );

		m_bottomLevel = centralLevel - ( half_grid
							/ (float)m_y_delta );
		if( m_bottomLevel < m_minLevel )
		{
			m_bottomLevel = m_minLevel;
			m_topLevel = m_minLevel + (int)floorf( grid_height
							/ (float)m_y_delta );
		}
		else
		{
			m_topLevel = m_bottomLevel + (int)floorf( grid_height
							/ (float)m_y_delta );
			if( m_topLevel > m_maxLevel )
			{
				m_topLevel = m_maxLevel;
				m_bottomLevel = m_maxLevel - (int)floorf(
					grid_height / (float)m_y_delta );
			}
		}
	}
	else
	{
		m_bottomLevel = m_minLevel;
		m_topLevel = m_maxLevel;
	}
}









AutomationEditorWindow::AutomationEditorWindow() :
	Editor(),
	m_editor(new AutomationEditor())
{
	setCentralWidget(m_editor);



	// Play/stop buttons
	m_playAction->setToolTip(tr( "Play/pause current pattern (Space)" ));
	m_playAction->setWhatsThis(
		tr( "Click here if you want to play the current pattern. "
			"This is useful while editing it.  The pattern is "
			"automatically looped when the end is reached." ) );

	m_stopAction->setToolTip(tr("Stop playing of current pattern (Space)"));
	m_stopAction->setWhatsThis(
		tr( "Click here if you want to stop playing of the "
			"current pattern." ) );

	// Edit mode buttons
	QActionGroup * tool_action_group = new QActionGroup(this);

	m_drawAction = new QAction(embed::getIconPixmap("edit_draw"),
								  tr("Draw mode (Shift+D)"), tool_action_group);
	m_drawAction->setCheckable(true);
	m_drawAction->setShortcut(Qt::SHIFT | Qt::Key_D);

	m_eraseAction = new QAction(embed::getIconPixmap("edit_erase"),
									tr("Erase mode (Shift+E)"), tool_action_group);
	m_eraseAction->setCheckable(true);
	m_eraseAction->setShortcut(Qt::SHIFT | Qt::Key_E);

	m_drawAction->setChecked(true);

	//TODO: m_selectButton and m_moveButton are broken.
	/*m_selectButton = new ToolButton( embed::getIconPixmap(
							"edit_select" ),
					tr( "Select mode (Shift+S)" ),
					this, SLOT( selectButtonToggled() ),
					m_toolBar );
	m_selectButton->setCheckable( true );

	m_moveButton = new ToolButton( embed::getIconPixmap( "edit_move" ),
					tr( "Move selection mode (Shift+M)" ),
					this, SLOT( moveButtonToggled() ),
					m_toolBar );
	m_moveButton->setCheckable( true );*/

	QSignalMapper* signalmapper = new QSignalMapper(this);
	signalmapper->setMapping(m_drawAction, AutomationEditor::DRAW);
	signalmapper->setMapping(m_eraseAction, AutomationEditor::ERASE);
//	signalmapper->setMapping(m_selectButton, AutomationEditor::SELECT);
//	signalmapper->setMapping(m_moveButton, AutomationEditor::MOVE);

	connect(m_drawAction, SIGNAL(triggered()), signalmapper, SLOT(map()));
	connect(m_eraseAction, SIGNAL(triggered()), signalmapper, SLOT(map()));
//	connect(m_selectButton, SIGNAL(triggered()), signalmapper, SLOT(map()));
//	connect(m_moveButton, SIGNAL(triggered()), signalmapper, SLOT(map()));

	connect(signalmapper, SIGNAL(mapped(int)), m_editor, SLOT(setEditMode(int)));

//	tool_action_group->addAction( m_drawButton );
//	tool_action_group->addAction( m_eraseButton );
	//tool_button_group->addButton( m_selectButton );
	//tool_button_group->addButton( m_moveButton );

	m_drawAction->setWhatsThis(
		tr( "Click here and draw-mode will be activated. In this "
			"mode you can add and move single values.  This "
			"is the default mode which is used most of the time.  "
			"You can also press 'Shift+D' on your keyboard to "
			"activate this mode." ) );
	m_eraseAction->setWhatsThis(
		tr( "Click here and erase-mode will be activated. In this "
			"mode you can erase single values. You can also press "
			"'Shift+E' on your keyboard to activate this mode." ) );
	/*m_selectButton->setWhatsThis(
		tr( "Click here and select-mode will be activated. In this "
			"mode you can select values. This is necessary "
			"if you want to cut, copy, paste, delete, or move "
			"values. You can also press 'Shift+S' on your keyboard "
			"to activate this mode." ) );
	m_moveButton->setWhatsThis(
		tr( "If you click here, move-mode will be activated. In this "
			"mode you can move the values you selected in select-"
			"mode. You can also press 'Shift+M' on your keyboard "
			"to activate this mode." ) );*/



	// Progression type buttons
	QActionGroup* progression_type_group = new QActionGroup(this);

	m_discreteAction = new QAction(embed::getIconPixmap("progression_discrete"),
								   tr("Discrete progression"), progression_type_group);
	m_linearAction = new QAction(embed::getIconPixmap("progression_linear"),
								 tr("Linear progression"), progression_type_group);
	m_cubicHermiteAction = new QAction(embed::getIconPixmap("progression_cubic_hermite"),
									   tr( "Cubic Hermite progression"), progression_type_group);

	m_linearAction->setCheckable( true );
	m_cubicHermiteAction->setCheckable( true );
	m_discreteAction->setCheckable( true );
	m_discreteAction->setChecked( true );

	connect(m_discreteAction, SIGNAL(triggered()), m_editor, SLOT(setProgressionDiscrete()));
	connect(m_linearAction, SIGNAL(triggered()), m_editor, SLOT(setProgressionLinear()));
	connect(m_cubicHermiteAction, SIGNAL(triggered()), m_editor, SLOT(setProgressionHermite()));

	// setup tension-stuff
	m_tensionKnob = new Knob( knobSmall_17, this, "Tension" );

	m_discreteAction->setWhatsThis(
		tr( "Click here to choose discrete progressions for this "
			"automation pattern.  The value of the connected "
			"object will remain constant between control points "
			"and be set immediately to the new value when each "
			"control point is reached." ) );
	m_linearAction->setWhatsThis(
		tr( "Click here to choose linear progressions for this "
			"automation pattern.  The value of the connected "
			"object will change at a steady rate over time "
			"between control points to reach the correct value at "
			"each control point without a sudden change." ) );
	m_cubicHermiteAction->setWhatsThis(
		tr( "Click here to choose cubic hermite progressions for this "
			"automation pattern.  The value of the connected "
			"object will change in a smooth curve and ease in to "
			"the peaks and valleys." ) );

	// Copy paste buttons

	m_cutAction = new QAction(embed::getIconPixmap("edit_cut"),
					tr("Cut selected values (Ctrl+X)"), this);
	m_copyAction = new QAction(embed::getIconPixmap("edit_copy"),
					tr("Copy selected values (Ctrl+C)"), this);
	m_pasteAction = new QAction(embed::getIconPixmap("edit_paste"),
					tr("Paste values from clipboard Ctrl+V)"), this);

	m_cutAction->setWhatsThis(
		tr( "Click here and selected values will be cut into the "
			"clipboard.  You can paste them anywhere in any pattern "
			"by clicking on the paste button." ) );
	m_copyAction->setWhatsThis(
		tr( "Click here and selected values will be copied into "
			"the clipboard.  You can paste them anywhere in any "
			"pattern by clicking on the paste button." ) );
	m_pasteAction->setWhatsThis(
		tr( "Click here and the values from the clipboard will be "
			"pasted at the first visible measure." ) );

	m_cutAction->setShortcut(Qt::CTRL | Qt::Key_X);
	m_copyAction->setShortcut(Qt::CTRL | Qt::Key_C);
	m_pasteAction->setShortcut(Qt::CTRL | Qt::Key_V);

	connect(m_cutAction,   SIGNAL(triggered()), m_editor, SLOT(cutSelectedValues()));
	connect(m_copyAction,  SIGNAL(triggered()), m_editor, SLOT(copySelectedValues()));
	connect(m_pasteAction, SIGNAL(triggered()), m_editor, SLOT(pasteValues()));

	// Zoom controls

	QLabel * zoom_x_label = new QLabel( m_toolBar );
	zoom_x_label->setPixmap( embed::getIconPixmap( "zoom_x" ) );

	m_zoomingXComboBox = new ComboBox( m_toolBar );
	m_zoomingXComboBox->setFixedSize( 80, 22 );

	for( int i = 0; i < 6; ++i )
	{
		m_editor->m_zoomingXModel.addItem( QString::number( 25 << i ) + "%" );
	}
	m_editor->m_zoomingXModel.setValue( m_editor->m_zoomingXModel.findText( "100%" ) );

	m_zoomingXComboBox->setModel( &m_editor->m_zoomingXModel );

	connect( &m_editor->m_zoomingXModel, SIGNAL( dataChanged() ),
			m_editor, SLOT( zoomingXChanged() ) );


	QLabel * zoom_y_label = new QLabel( m_toolBar );
	zoom_y_label->setPixmap( embed::getIconPixmap( "zoom_y" ) );

	m_zoomingYComboBox = new ComboBox( m_toolBar );
	m_zoomingYComboBox->setFixedSize( 80, 22 );

	m_editor->m_zoomingYModel.addItem( "Auto" );
	for( int i = 0; i < 7; ++i )
	{
		m_editor->m_zoomingYModel.addItem( QString::number( 25 << i ) + "%" );
	}
	m_editor->m_zoomingYModel.setValue( m_editor->m_zoomingYModel.findText( "Auto" ) );

	m_zoomingYComboBox->setModel( &m_editor->m_zoomingYModel );

	connect( &m_editor->m_zoomingYModel, SIGNAL( dataChanged() ),
			m_editor, SLOT( zoomingYChanged() ) );



	// Quantization controls

	QLabel * quantize_lbl = new QLabel( m_toolBar );
	quantize_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	m_quantizeComboBox = new ComboBox( m_toolBar );
	m_quantizeComboBox->setFixedSize( 60, 22 );

	m_quantizeComboBox->setModel( &m_editor->m_quantizeModel );


	m_toolBar->addSeparator();;
	m_toolBar->addAction(m_drawAction);
	m_toolBar->addAction(m_eraseAction);
//	m_toolBar->addAction(m_selectButton);
//	m_toolBar->addAction(m_moveButton);
	m_toolBar->addSeparator();
	m_toolBar->addAction(m_discreteAction);
	m_toolBar->addAction(m_linearAction);
	m_toolBar->addAction(m_cubicHermiteAction);
	m_toolBar->addSeparator();
	m_toolBar->addWidget( new QLabel( tr("Tension: "), m_toolBar ));
	m_toolBar->addWidget( m_tensionKnob );
	m_toolBar->addSeparator();
//	Select is broken
//	m_toolBar->addAction( m_cutAction );
//	m_toolBar->addAction( m_copyAction );
//	m_toolBar->addAction( m_pasteAction );
	m_toolBar->addSeparator();
	m_editor->m_timeLine->addToolButtons(m_toolBar);
	m_toolBar->addSeparator();
	m_toolBar->addWidget( zoom_x_label );
	m_toolBar->addWidget( m_zoomingXComboBox );
	m_toolBar->addSeparator();
	m_toolBar->addWidget( zoom_y_label );
	m_toolBar->addWidget( m_zoomingYComboBox );
	m_toolBar->addSeparator();
	m_toolBar->addWidget( quantize_lbl );
	m_toolBar->addWidget( m_quantizeComboBox );

	// Setup our actual window
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	setWindowIcon( embed::getIconPixmap( "automation" ) );
}


AutomationEditorWindow::~AutomationEditorWindow()
{
}


void AutomationEditorWindow::setCurrentPattern(AutomationPattern* pattern)
{
	m_editor->setCurrentPattern(pattern);

	if (pattern == nullptr)
		return;

	switch(m_editor->m_pattern->progressionType())
	{
	case AutomationPattern::DiscreteProgression:
		m_discreteAction->setChecked(true);
		break;
	case AutomationPattern::LinearProgression:
		m_linearAction->setChecked(true);
		break;
	case AutomationPattern::CubicHermiteProgression:
		m_cubicHermiteAction->setChecked(true);
		break;
	}

	emit currentPatternChanged();
}


const AutomationPattern* AutomationEditorWindow::currentPattern()
{
	return m_editor->currentPattern();
}


int AutomationEditorWindow::quantization() const
{
	return m_editor->quantization();
}

QSize AutomationEditorWindow::sizeHint() const
{
	return {INITIAL_WIDTH, INITIAL_HEIGHT};
}

void AutomationEditorWindow::play()
{
	m_editor->play();
	setPauseIcon(Engine::getSong()->isPlaying());
}

void AutomationEditorWindow::stop()
{
	m_editor->stop();
}
