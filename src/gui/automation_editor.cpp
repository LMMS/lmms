/*
 * automation_editor.cpp - implementation of automationEditor which is used for
 *                         actual setting of dynamic values
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "automation_editor.h"

#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QStyleOption>
#include <QtGui/QWheelEvent>


#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>


#include "song_editor.h"
#include "MainWindow.h"
#include "embed.h"
#include "engine.h"
#include "pixmap_button.h"
#include "templates.h"
#include "gui_templates.h"
#include "timeline.h"
#include "tooltip.h"
#include "midi.h"
#include "tool_button.h"
#include "text_float.h"
#include "combobox.h"
#include "bb_track_container.h"
#include "piano_roll.h"
#include "debug.h"



QPixmap * automationEditor::s_toolDraw = NULL;
QPixmap * automationEditor::s_toolErase = NULL;
QPixmap * automationEditor::s_toolSelect = NULL;
QPixmap * automationEditor::s_toolMove = NULL;


automationEditor::automationEditor() :
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
	m_action( ActionNone ),
	m_moveStartLevel( 0 ),
	m_moveStartTick( 0 ),
	m_ppt( DefaultPixelsPerTact ),
	m_y_delta( DefaultYDelta ),
	m_y_auto( true ),
	m_editMode( ModeDraw ),
	m_scrollBack( false )
{
	connect( this, SIGNAL( currentPatternChanged() ),
				this, SLOT( updateAfterPatternChange() ),
				Qt::QueuedConnection );

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

	setAttribute( Qt::WA_OpaquePaintEvent, true );

	// add time-line
	m_timeLine = new timeLine( ValuesWidth, 32, m_ppt,
				engine::getSong()->getPlayPos(
					song::Mode_PlayAutomationPattern ),
						m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
		m_timeLine, SLOT( updatePosition( const midiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( updatePosition( const midiTime & ) ) );


	m_toolBar = new QWidget( this );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
	m_toolBar->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(),
					embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );

	QHBoxLayout * tb_layout = new QHBoxLayout( m_toolBar );
	tb_layout->setMargin( 0 );
	tb_layout->setSpacing( 0 );


	// init control-buttons at the top

	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
				tr( "Play/pause current pattern (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
				tr( "Stop playing of current pattern (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	m_playButton->setWhatsThis(
		tr( "Click here if you want to play the current pattern. "
			"This is useful while editing it.  The pattern is "
			"automatically looped when the end is reached." ) );
	m_stopButton->setWhatsThis(
		tr( "Click here if you want to stop playing of the "
			"current pattern." ) );



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

	// init edit-buttons at the top
	m_drawButton = new toolButton( embed::getIconPixmap( "edit_draw" ),
					tr( "Draw mode (Shift+D)" ),
					this, SLOT( drawButtonToggled() ),
					m_toolBar );
	m_drawButton->setCheckable( true );
	m_drawButton->setChecked( true );

	m_eraseButton = new toolButton( embed::getIconPixmap( "edit_erase" ),
					tr( "Erase mode (Shift+E)" ),
					this, SLOT( eraseButtonToggled() ),
					m_toolBar );
	m_eraseButton->setCheckable( true );

	m_selectButton = new toolButton( embed::getIconPixmap(
							"edit_select" ),
					tr( "Select mode (Shift+S)" ),
					this, SLOT( selectButtonToggled() ),
					m_toolBar );
	m_selectButton->setCheckable( true );

	m_moveButton = new toolButton( embed::getIconPixmap( "edit_move" ),
					tr( "Move selection mode (Shift+M)" ),
					this, SLOT( moveButtonToggled() ),
					m_toolBar );
	m_moveButton->setCheckable( true );

	QButtonGroup * tool_button_group = new QButtonGroup( this );
	tool_button_group->addButton( m_drawButton );
	tool_button_group->addButton( m_eraseButton );
	tool_button_group->addButton( m_selectButton );
	tool_button_group->addButton( m_moveButton );
	tool_button_group->setExclusive( true );

	m_drawButton->setWhatsThis(
		tr( "Click here and draw-mode will be activated. In this "
			"mode you can add and move single values.  This "
			"is the default mode which is used most of the time.  "
			"You can also press 'Shift+D' on your keyboard to "
			"activate this mode." ) );
	m_eraseButton->setWhatsThis(
		tr( "Click here and erase-mode will be activated. In this "
			"mode you can erase single values. You can also press "
			"'Shift+E' on your keyboard to activate this mode." ) );
	m_selectButton->setWhatsThis(
		tr( "Click here and select-mode will be activated. In this "
			"mode you can select values. This is necessary "
			"if you want to cut, copy, paste, delete, or move "
			"values. You can also press 'Shift+S' on your keyboard "
			"to activate this mode." ) );
	m_moveButton->setWhatsThis(
		tr( "If you click here, move-mode will be activated. In this "
			"mode you can move the values you selected in select-"
			"mode. You can also press 'Shift+M' on your keyboard "
			"to activate this mode." ) );

	m_cutButton = new toolButton( embed::getIconPixmap( "edit_cut" ),
					tr( "Cut selected values (Ctrl+X)" ),
					this, SLOT( cutSelectedValues() ),
					m_toolBar );

	m_copyButton = new toolButton( embed::getIconPixmap( "edit_copy" ),
					tr( "Copy selected values (Ctrl+C)" ),
					this, SLOT( copySelectedValues() ),
					m_toolBar );

	m_pasteButton = new toolButton( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste values from clipboard "
								"(Ctrl+V)" ),
					this, SLOT( pasteValues() ),
					m_toolBar );

	m_cutButton->setWhatsThis(
		tr( "Click here and selected values will be cut into the "
			"clipboard.  You can paste them anywhere in any pattern "
			"by clicking on the paste button." ) );
	m_copyButton->setWhatsThis(
		tr( "Click here and selected values will be copied into "
			"the clipboard.  You can paste them anywhere in any "
			"pattern by clicking on the paste button." ) );
	m_pasteButton->setWhatsThis(
		tr( "Click here and the values from the clipboard will be "
			"pasted at the first visible measure." ) );


	// setup zooming-stuff
	QLabel * zoom_x_lbl = new QLabel( m_toolBar );
	zoom_x_lbl->setPixmap( embed::getIconPixmap( "zoom_x" ) );

	m_zoomingXComboBox = new comboBox( m_toolBar );
	m_zoomingXComboBox->setFixedSize( 80, 22 );

	for( int i = 0; i < 6; ++i )
	{
		m_zoomingXModel.addItem( QString::number( 25 << i ) + "%" );
	}
	m_zoomingXModel.setValue( m_zoomingXModel.findText( "100%" ) );

	m_zoomingXComboBox->setModel( &m_zoomingXModel );

	connect( &m_zoomingXModel, SIGNAL( dataChanged() ),
			this, SLOT( zoomingXChanged() ) );


	QLabel * zoom_y_lbl = new QLabel( m_toolBar );
	zoom_y_lbl->setPixmap( embed::getIconPixmap( "zoom_y" ) );

	m_zoomingYComboBox = new comboBox( m_toolBar );
	m_zoomingYComboBox->setFixedSize( 80, 22 );

	m_zoomingYModel.addItem( "Auto" );
	for( int i = 0; i < 6; ++i )
	{
		m_zoomingYModel.addItem( QString::number( 25 << i ) + "%" );
	}
	m_zoomingYModel.setValue( m_zoomingYModel.findText( "Auto" ) );

	m_zoomingYComboBox->setModel( &m_zoomingYModel );

	connect( &m_zoomingYModel, SIGNAL( dataChanged() ),
			this, SLOT( zoomingYChanged() ) );


	// setup quantize-stuff
	QLabel * quantize_lbl = new QLabel( m_toolBar );
	quantize_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	m_quantizeComboBox = new comboBox( m_toolBar );
	m_quantizeComboBox->setFixedSize( 60, 22 );

	for( int i = 0; i < 7; ++i )
	{
		m_quantizeModel.addItem( "1/" + QString::number( 1 << i ) );
	}
	m_quantizeModel.setValue( m_quantizeModel.findText( "1/16" ) );

	m_quantizeComboBox->setModel( &m_quantizeModel );


	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_playButton );
	tb_layout->addWidget( m_stopButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_drawButton );
	tb_layout->addWidget( m_eraseButton );
	tb_layout->addWidget( m_selectButton );
	tb_layout->addWidget( m_moveButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_cutButton );
	tb_layout->addWidget( m_copyButton );
	tb_layout->addWidget( m_pasteButton );
	tb_layout->addSpacing( 10 );
	m_timeLine->addToolButtons( m_toolBar );
	tb_layout->addSpacing( 15 );
	tb_layout->addWidget( zoom_x_lbl );
	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_zoomingXComboBox );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( zoom_y_lbl );
	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_zoomingYComboBox );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( quantize_lbl );
	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_quantizeComboBox );
	tb_layout->addStretch();

	// setup our actual window
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	setWindowIcon( embed::getIconPixmap( "automation" ) );
	setCurrentPattern( NULL );

	setMouseTracking( true );

	setMinimumSize( tb_layout->minimumSize().width(), 128 );

	// add us to workspace
	if( engine::mainWindow()->workspace() )
	{
		engine::mainWindow()->workspace()->addSubWindow( this );
		parentWidget()->resize( InitialWidth, InitialHeight );
		parentWidget()->hide();
	}
	else
	{
		resize( InitialWidth, InitialHeight );
		hide();
	}
}




automationEditor::~automationEditor()
{
	m_zoomingXModel.disconnect();
	m_zoomingYModel.disconnect();
}




void automationEditor::setCurrentPattern( automationPattern * _new_pattern )
{
	m_patternMutex.lock();
	m_pattern = _new_pattern;
	m_patternMutex.unlock();

	emit currentPatternChanged();
}




void automationEditor::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void automationEditor::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}




void automationEditor::updateAfterPatternChange()
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




void automationEditor::update()
{
	QWidget::update();

	QMutexLocker m( &m_patternMutex );
	// Note detuning?
	if( m_pattern && !m_pattern->getTrack() )
	{
		engine::getPianoRoll()->update();
	}
}




inline void automationEditor::drawValueRect( QPainter & _p,
						int _x, int _y,
						int _width, int _height,
						const bool _is_selected )
{
	_p.fillRect( _x, _y, _width, _height, engine::getLmmsStyle()->color(
			_is_selected ?
				LmmsStyle::AutomationSelectedBarFill :
				LmmsStyle::AutomationBarFill ) );

	_p.drawLine( _x - 1, _y, _x + 1, _y );
	_p.drawLine( _x, _y - 1, _x, _y + 1 );
}




void automationEditor::removeSelection()
{
	m_selectStartTick = 0;
	m_selectedTick = 0;
	m_selectStartLevel = 0;
	m_selectedLevels = 0;
}




void automationEditor::closeEvent( QCloseEvent * _ce )
{
	QApplication::restoreOverrideCursor();
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	_ce->ignore();
}




void automationEditor::keyPressEvent( QKeyEvent * _ke )
{
	switch( _ke->key() )
	{
		case Qt::Key_Up:
			m_topBottomScroll->setValue(
					m_topBottomScroll->value() - 1 );
			_ke->accept();
			break;

		case Qt::Key_Down:
			m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
			_ke->accept();
			break;

		case Qt::Key_Left:
			if( ( m_timeLine->pos() -= 16 ) < 0 )
			{
				m_timeLine->pos().setTicks( 0 );
			}
			m_timeLine->updatePosition();
			_ke->accept();
			break;

		case Qt::Key_Right:
			m_timeLine->pos() += 16;
			m_timeLine->updatePosition();
			_ke->accept();
			break;

		case Qt::Key_C:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				copySelectedValues();
				_ke->accept();
			}
			break;

		case Qt::Key_X:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				cutSelectedValues();
				_ke->accept();
			}
			break;

		case Qt::Key_V:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				pasteValues();
				_ke->accept();
			}
			break;

		case Qt::Key_A:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				m_selectButton->setChecked( true );
				selectAll();
				update();
				_ke->accept();
			}
			break;

		case Qt::Key_D:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_drawButton->setChecked( true );
				_ke->accept();
			}
			break;

		case Qt::Key_E:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_eraseButton->setChecked( true );
				_ke->accept();
			}
			break;

		case Qt::Key_S:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_selectButton->setChecked( true );
				_ke->accept();
			}
			break;

		case Qt::Key_M:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_moveButton->setChecked( true );
				_ke->accept();
			}
			break;

		case Qt::Key_Delete:
			deleteSelectedValues();
			_ke->accept();
			break;

		case Qt::Key_Space:
			if( engine::getSong()->isPlaying() )
			{
				stop();
			}
			else
			{
				play();
			}
			_ke->accept();
			break;

		case Qt::Key_Home:
			m_timeLine->pos().setTicks( 0 );
			m_timeLine->updatePosition();
			_ke->accept();
			break;

		default:
			break;
	}
}




void automationEditor::leaveEvent( QEvent * _e )
{
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}

	QWidget::leaveEvent( _e );
}


void automationEditor::drawLine( int _x0, float _y0, int _x1, float _y1 )
{
	int deltax = qRound( qAbs<float>( _x1 - _x0 ) );
	float deltay = qAbs<float>( _y1 - _y0 );
	int x = _x0;
	float y = _y0;
	int xstep;
	int ystep;

	if( deltax < quantization() )
	{
		return;
	}

	deltax /= quantization();

	float yscale = deltay / ( deltax );

	if( _x0 < _x1)
	{
		xstep = quantization();
	}
	else
	{
		xstep = -( quantization() );
	}

	if( _y0 < _y1 )
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
		y = _y0 + ( ystep * yscale * i );

		x += xstep;
		i += 1;
		m_pattern->removeValue( midiTime( x ) );
		m_pattern->putValue( midiTime( x ), y );
	}
}




void automationEditor::mousePressEvent( QMouseEvent * _me )
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		return;
	}

	if( _me->y() > TopMargin )
	{
		float level = getLevel( _me->y() );

		int x = _me->x();

		if( x > ValuesWidth )
		{
			// set or move value

			x -= ValuesWidth;

			// get tick in which the user clicked
			int pos_ticks = x * DefaultTicksPerTact / m_ppt +
							m_currentPosition;

			// get time map of current pattern
			timeMap & time_map = m_pattern->getTimeMap();

			// will be our iterator in the following loop
			timeMap::iterator it = time_map.begin();

			// loop through whole time-map...
			while( it != time_map.end() )
			{
				midiTime len = 4;

				// and check whether the user clicked on an
				// existing value
				if( pos_ticks >= it.key() &&
					len > 0 &&
					( it+1==time_map.end() ||
						pos_ticks <= (it+1).key() ) &&
		( pos_ticks<= it.key() + DefaultTicksPerTact *4 / m_ppt ) &&
					level <= it.value() )
				{
					break;
				}
				++it;
			}

			// left button??
			if( _me->button() == Qt::LeftButton &&
							m_editMode == ModeDraw )
			{
				// Connect the dots
				if( _me->modifiers() & Qt::ShiftModifier )
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
					midiTime value_pos( pos_ticks );

					midiTime new_time =
						m_pattern->putValue( value_pos,
									level );

					// reset it so that it can be used for
					// ops (move, resize) after this
					// code-block
					it = time_map.find( new_time );
				}

				// move it
				m_action = ActionMoveValue;
				int aligned_x = (int)( (float)( (
						it.key() -
						m_currentPosition ) *
						m_ppt ) / DefaultTicksPerTact );
				m_moveXOffset = x - aligned_x - 1;
				// set move-cursor
				QCursor c( Qt::SizeAllCursor );
				QApplication::setOverrideCursor( c );

				engine::getSong()->setModified();
			}
			else if( ( _me->button() == Qt::RightButton &&
							m_editMode == ModeDraw ) ||
					m_editMode == ModeErase )
			{
				// erase single value
				if( it != time_map.end() )
				{
					m_pattern->removeValue( it.key() );
					engine::getSong()->setModified();
				}
			}
			else if( _me->button() == Qt::LeftButton &&
							m_editMode == ModeSelect )
			{
				// select an area of values

				m_selectStartTick = pos_ticks;
				m_selectedTick = 0;
				m_selectStartLevel = level;
				m_selectedLevels = 1;
				m_action = ActionSelectValues;
			}
			else if( _me->button() == Qt::RightButton &&
							m_editMode == ModeSelect )
			{
				// when clicking right in select-move, we
				// switch to move-mode
				m_moveButton->setChecked( true );
			}
			else if( _me->button() == Qt::LeftButton &&
							m_editMode == ModeMove )
			{
				// move selection (including selected values)

				// save position where move-process began
				m_moveStartTick = pos_ticks;
				m_moveStartLevel = level;

				m_action = ActionMoveSelection;

				engine::getSong()->setModified();
			}
			else if( _me->button() == Qt::RightButton &&
							m_editMode == ModeMove )
			{
				// when clicking right in select-move, we
				// switch to draw-mode
				m_drawButton->setChecked( true );
			}

			update();
		}
	}
}




void automationEditor::mouseReleaseEvent( QMouseEvent * _me )
{
	m_action = ActionNone;

	if( m_editMode == ModeDraw )
	{
		QApplication::restoreOverrideCursor();
	}
}




void automationEditor::mouseMoveEvent( QMouseEvent * _me )
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		update();
		return;
	}

	if( _me->y() > TopMargin )
	{
		float level = getLevel( _me->y() );
		int x = _me->x();

		if( _me->x() <= ValuesWidth )
		{
			update();
			return;
		}
		x -= ValuesWidth;
		if( m_action == ActionMoveValue )
		{
			x -= m_moveXOffset;
		}

		int pos_ticks = x * DefaultTicksPerTact / m_ppt +
							m_currentPosition;
		if( _me->buttons() & Qt::LeftButton && m_editMode == ModeDraw )
		{
			if( m_action == ActionMoveValue )
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
				m_pattern->removeValue(
						midiTime( pos_ticks ) );
				m_pattern->putValue( midiTime( pos_ticks ),
								level );
			}

			engine::getSong()->setModified();

		}
		else if( ( _me->buttons() & Qt::RightButton &&
						m_editMode == ModeDraw ) ||
				( _me->buttons() & Qt::LeftButton &&
						m_editMode == ModeErase ) )
		{
			m_pattern->removeValue( midiTime( pos_ticks ) );
		}
		else if( _me->buttons() & Qt::NoButton && m_editMode == ModeDraw )
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
		else if( _me->buttons() & Qt::LeftButton &&
						m_editMode == ModeSelect &&
						m_action == ActionSelectValues )
		{

			// change size of selection

			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint(
						ValuesWidth, _me->y() ) ) );
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
			else if( x > width() - ValuesWidth )
			{
				x = width() - ValuesWidth;
				QCursor::setPos( mapToGlobal( QPoint( width(),
								_me->y() ) ) );
				m_leftRightScroll->setValue( m_currentPosition +
									4 );
			}

			// get tick in which the cursor is posated
			int pos_ticks = x * DefaultTicksPerTact / m_ppt +
							m_currentPosition;

			m_selectedTick = pos_ticks - m_selectStartTick;
			if( (int) m_selectStartTick + m_selectedTick < 0 )
			{
				m_selectedTick = -qRound( m_selectStartTick );
			}
			m_selectedLevels = level - m_selectStartLevel;
			if( level <= m_selectStartLevel )
			{
				--m_selectedLevels;
			}
		}
		else if( _me->buttons() & Qt::LeftButton &&
					m_editMode == ModeMove &&
					m_action == ActionMoveSelection )
		{
			// move selection + selected values

			// do horizontal move-stuff
			int pos_ticks = x * DefaultTicksPerTact / m_ppt +
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

			int tact_diff = ticks_diff / DefaultTicksPerTact;
			ticks_diff = ticks_diff % DefaultTicksPerTact;


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
				midiTime new_value_pos;
				if( it.key() )
				{
					int value_tact =
						( it.key() /
							DefaultTicksPerTact )
								+ tact_diff;
					int value_ticks =
						( it.key() %
							DefaultTicksPerTact )
								+ ticks_diff;
					// ensure value_ticks range
					if( value_ticks / DefaultTicksPerTact )
					{
						value_tact += value_ticks
							/ DefaultTicksPerTact;
						value_ticks %=
							DefaultTicksPerTact;
					}
					m_pattern->removeValue( it.key() );
					new_value_pos = midiTime( value_tact,
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
		if( _me->buttons() & Qt::LeftButton &&
					m_editMode == ModeSelect &&
					m_action == ActionSelectValues )
		{

			int x = _me->x() - ValuesWidth;
			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint( ValuesWidth,
								_me->y() ) ) );
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
			else if( x > width() - ValuesWidth )
			{
				x = width() - ValuesWidth;
				QCursor::setPos( mapToGlobal( QPoint( width(),
							_me->y() ) ) );
				m_leftRightScroll->setValue( m_currentPosition +
									4 );
			}

			// get tick in which the cursor is posated
			int pos_ticks = x * DefaultTicksPerTact / m_ppt +
							m_currentPosition;

			m_selectedTick = pos_ticks -
							m_selectStartTick;
			if( (int) m_selectStartTick + m_selectedTick <
									0 )
			{
				m_selectedTick = -qRound( m_selectStartTick );
			}

			float level = getLevel( _me->y() );

			if( level <= m_bottomLevel )
			{
				QCursor::setPos( mapToGlobal( QPoint( _me->x(),
							height() -
							ScrollBarSize ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
				level = m_bottomLevel;
			}
			else if( level >= m_topLevel )
			{
				QCursor::setPos( mapToGlobal( QPoint( _me->x(),
								TopMargin ) ) );
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




inline void automationEditor::drawCross( QPainter & _p )
{
	QPoint mouse_pos = mapFromGlobal( QCursor::pos() );
	float level = getLevel( mouse_pos.y() );
	int grid_bottom = height() - ScrollBarSize - 1;
	float cross_y = m_y_auto ?
		grid_bottom - ( ( grid_bottom - TopMargin )
				* ( level - m_minLevel )
				/ (float)( m_maxLevel - m_minLevel ) ) :
		grid_bottom - ( level - m_bottomLevel ) * m_y_delta;

	_p.setPen( engine::getLmmsStyle()->color( LmmsStyle::AutomationCrosshair ) );
	_p.drawLine( ValuesWidth, (int) cross_y, width(), (int) cross_y );
	_p.drawLine( mouse_pos.x(), TopMargin, mouse_pos.x(),
						height() - ScrollBarSize );
}




void automationEditor::paintEvent( QPaintEvent * _pe )
{
	QMutexLocker m( &m_patternMutex );

	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );

	// set font-size to 8
	p.setFont( pointSize<8>( p.font() ) );

	int grid_height = height() - TopMargin - ScrollBarSize;

	// start drawing at the bottom
	int grid_bottom = height() - ScrollBarSize - 1;

	p.fillRect( 0, TopMargin, ValuesWidth, height() - TopMargin,
						QColor( 0x33, 0x33, 0x33 ) );

	// print value numbers
	int font_height = p.fontMetrics().height();
	Qt::Alignment text_flags =
		(Qt::Alignment)( Qt::AlignRight | Qt::AlignVCenter );

	if( validPattern() )
	{
		if( m_y_auto )
		{
			int y[] = { grid_bottom, TopMargin + font_height / 2 };
			float level[] = { m_minLevel, m_maxLevel };
			for( int i = 0; i < 2; ++i )
			{
				const QString & label = m_pattern->firstObject()
						->displayValue( level[i] );
				p.setPen( QColor( 240, 240, 240 ) );
				p.drawText( 1, y[i] - font_height + 1,
					ValuesWidth - 10, 2 * font_height,
					text_flags, label );
				p.setPen( QColor( 0, 0, 0 ) );
				p.drawText( 0, y[i] - font_height,
					ValuesWidth - 10, 2 * font_height,
					text_flags, label );
			}
		}
		else
		{
			int y = grid_bottom;
			int level = (int) m_bottomLevel;
			int printable = qMax( 1, 5 * DefaultYDelta
								/ m_y_delta );
			int module = level % printable;
			if( module )
			{
				int inv_module = ( printable - module )
								% printable;
				y -= inv_module * m_y_delta;
				level += inv_module;
			}
			for( ; y >= TopMargin && level <= m_topLevel;
				y -= printable * m_y_delta, level += printable )
			{
				const QString & label = m_pattern->firstObject()
							->displayValue( level );
				p.setPen( QColor( 240, 240, 240 ) );
				p.drawText( 1, y - font_height + 1,
					ValuesWidth - 10, 2 * font_height,
					text_flags, label );
				p.setPen( QColor( 0, 0, 0 ) );
				p.drawText( 0, y - font_height,
					ValuesWidth - 10, 2 * font_height,
					text_flags, label );
			}
		}
	}

	// set clipping area, because we are not allowed to paint over
	// keyboard...
	p.setClipRect( ValuesWidth, TopMargin, width() - ValuesWidth,
								grid_height  );

	// draw vertical raster
	int tact_16th = m_currentPosition / ( DefaultTicksPerTact / 16 );
	const int offset = ( m_currentPosition % (DefaultTicksPerTact/16) ) *
					m_ppt / DefaultStepsPerTact / 8;

	if( m_pattern )
	{
		int x_line_end = (int)( m_y_auto || m_topLevel < m_maxLevel ?
			TopMargin :
			grid_bottom - ( m_topLevel - m_bottomLevel )
								* m_y_delta );

		for( int x = ValuesWidth - offset; x < width();
			x += m_ppt / DefaultStepsPerTact, ++tact_16th )
		{
			if( x >= ValuesWidth )
			{
				// every tact-start needs to be a bright line
				if( tact_16th % 16 == 0 )
				{
	 				p.setPen( QColor( 0x7F, 0x7F, 0x7F ) );
				}
				// normal line
				else if( tact_16th % 4 == 0 )
				{
					p.setPen( QColor( 0x5F, 0x5F, 0x5F ) );
				}
				// weak line
				else
				{
					p.setPen( QColor( 0x3F, 0x3F, 0x3F ) );
				}
				p.drawLine( x, grid_bottom, x, x_line_end );
			}
		}


		if( m_y_auto )
		{
			QPen pen( QColor( 0x4F, 0x4F, 0x4F ) );
			p.setPen( pen );
			p.drawLine( ValuesWidth, grid_bottom, width(),
								grid_bottom );
			pen.setStyle( Qt::DotLine );
			p.setPen( pen );
			float y_delta = ( grid_bottom - TopMargin ) / 8.0f;
			for( int i = 1; i < 8; ++i )
			{
				int y = (int)( grid_bottom - i * y_delta );
				p.drawLine( ValuesWidth, y, width(), y );
			}
		}
		else
		{
			for( float y = grid_bottom, level = m_bottomLevel;
					y >= TopMargin && level <= m_topLevel;
					y -= m_y_delta, ++level )
			{
				if( (int)level % 5 == 0 )
				{
					p.setPen( QColor( 0x4F, 0x4F, 0x4F ) );
				}
				else
				{
					p.setPen( QColor( 0x3F, 0x3F, 0x3F ) );
				}

				// draw level line
				p.drawLine( ValuesWidth, (int) y, width(),
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
		timeMap & time_map = m_pattern->getTimeMap();
		timeMap::iterator it = time_map.begin();
		p.setPen( engine::getLmmsStyle()->color(
					LmmsStyle::AutomationBarValue ) );
		do
		{
			Sint32 len_ticks = 4;

			const float level = it.value();

			Sint32 pos_ticks = it.key();

			const int x = ( pos_ticks - m_currentPosition ) *
						m_ppt / DefaultTicksPerTact;
			if( x > width() - ValuesWidth )
			{
				break;
			}

			int rect_width;
			if( it+1 != time_map.end() )
			{
				timeMap::iterator it_prev = it+1;
				Sint32 next_pos_ticks = it_prev.key();
				int next_x = ( next_pos_ticks
					- m_currentPosition ) * m_ppt /
							DefaultTicksPerTact;
				// skip this value if not in visible area at all
/*				if( next_x > width() )
				{
					break;
				}*/
				rect_width = next_x - x;
			}
			else
			{
				rect_width = width() - x;
			}

			// is the value in visible area?
			if( ( level >= m_bottomLevel && level <= m_topLevel )
				|| ( level > m_topLevel && m_topLevel >= 0 )
				|| ( level < m_bottomLevel
						&& m_bottomLevel <= 0 ) )
			{
				bool is_selected = false;
				// if we're in move-mode, we may only draw
				// values in selected area, that have originally
				// been selected and not values that are now in
				// selection because the user moved it...
				if( m_editMode == ModeMove )
				{
					if( m_selValuesForMove.contains(
								it.key() ) )
					{
						is_selected = true;
					}
				}
				else if( level >= selLevel_start &&
					level <= selLevel_end &&
					pos_ticks >= sel_pos_start &&
					pos_ticks + len_ticks <=
								sel_pos_end )
				{
					is_selected = true;
				}

				// we've done and checked all, lets draw the
				// value
				int y_start;
				int rect_height;
				if( m_y_auto )
				{
					y_start = (int)( grid_bottom
						- ( grid_bottom - TopMargin )
						* ( level - m_minLevel )
						/ ( m_maxLevel - m_minLevel ) );
					int y_end = (int)( grid_bottom
						+ ( grid_bottom - TopMargin )
						* m_minLevel
						/ ( m_maxLevel - m_minLevel ) );
					rect_height = y_end - y_start;
				}
				else
				{
					y_start = (int)( grid_bottom - ( level
							- m_bottomLevel )
							* m_y_delta );
					rect_height = (int)( level * m_y_delta );
				}
				drawValueRect( p, x + ValuesWidth, y_start,
							rect_width, rect_height,
							is_selected );
			}
			else printf("not in range\n");
			++it;
		} while( it != time_map.end() );
	}
	else
	{
		QFont f = p.font();
		f.setBold( true );
		p.setFont( pointSize<14>( f ) );
		p.setPen( QColor( 0, 255, 0 ) );
		p.drawText( ValuesWidth + 20, TopMargin + 40,
				width() - ValuesWidth - 20 - ScrollBarSize,
				grid_height - 40, Qt::TextWordWrap,
				tr( "Please open an automation pattern with "
					"the context menu of a control!" ) );
	}

	// now draw selection-frame
	int x = ( sel_pos_start - m_currentPosition ) * m_ppt /
							DefaultTicksPerTact;
	int w = ( sel_pos_end - sel_pos_start ) * m_ppt / DefaultTicksPerTact;
	int y, h;
	if( m_y_auto )
	{
		y = (int)( grid_bottom - ( ( grid_bottom - TopMargin )
				* ( selLevel_start - m_minLevel )
				/ (float)( m_maxLevel - m_minLevel ) ) );
		h = (int)( grid_bottom - ( ( grid_bottom - TopMargin )
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
	p.drawRect( x + ValuesWidth, y, w, h );

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
		case ModeDraw: cursor = s_toolDraw; break;
		case ModeErase: cursor = s_toolErase; break;
		case ModeSelect: cursor = s_toolSelect; break;
		case ModeMove: cursor = s_toolMove; break;
	}
	p.drawPixmap( mapFromGlobal( QCursor::pos() ) + QPoint( 8, 8 ),
								*cursor );
}




// responsible for moving/resizing scrollbars after window-resizing
void automationEditor::resizeEvent( QResizeEvent * )
{
	m_leftRightScroll->setGeometry( ValuesWidth, height() - ScrollBarSize,
							width() - ValuesWidth,
							ScrollBarSize );

	int grid_height = height() - TopMargin - ScrollBarSize;
	m_topBottomScroll->setGeometry( width() - ScrollBarSize, TopMargin,
						ScrollBarSize, grid_height );

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

	if( engine::getSong() )
	{
		engine::getSong()->getPlayPos( song::Mode_PlayAutomationPattern
					).m_timeLine->setFixedWidth( width() );
	}
	m_toolBar->setFixedWidth( width() );

	updateTopBottomLevels();
	update();
}




void automationEditor::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	if( _we->modifiers() & Qt::ControlModifier )
	{
		if( _we->delta() > 0 )
		{
			m_ppt = qMin( m_ppt * 2, m_y_delta *
						DefaultStepsPerTact * 8 );
		}
		else if( m_ppt >= 72 )
		{
			m_ppt /= 2;
		}
		// update combobox with zooming-factor
		m_zoomingXComboBox->model()->setValue(
			m_zoomingXComboBox->model()->findText( QString::number(
					qRound( m_ppt * 100 /
						DefaultPixelsPerTact ) ) +"%" ) );
		// update timeline
		m_timeLine->setPixelsPerTact( m_ppt );
		update();
	}
	else if( _we->modifiers() & Qt::ShiftModifier
			|| _we->orientation() == Qt::Horizontal )
	{
		m_leftRightScroll->setValue( m_leftRightScroll->value() -
							_we->delta() * 2 / 15 );
	}
	else
	{
		m_topBottomScroll->setValue( m_topBottomScroll->value() -
							_we->delta() / 30 );
	}
}




float automationEditor::getLevel( int _y )
{
	int level_line_y = height() - ScrollBarSize - 1;
	// pressed level
	float level = roundf( ( m_bottomLevel + ( m_y_auto ?
			( m_maxLevel - m_minLevel ) * ( level_line_y - _y )
					/ (float)( level_line_y - TopMargin ) :
			( level_line_y - _y ) / (float)m_y_delta ) ) / m_step ) * m_step;
	// some range-checking-stuff
	if( level < m_bottomLevel )
	{
		level = m_bottomLevel;
	}
	else if( level > m_topLevel )
	{
		level = m_topLevel;
	}

	return( level );
}




inline bool automationEditor::inBBEditor()
{
	QMutexLocker m( &m_patternMutex );
	return( validPattern() &&
				m_pattern->getTrack()->getTrackContainer()
					== engine::getBBTrackContainer() );
}




void automationEditor::play()
{
	QMutexLocker m( &m_patternMutex );

	if( !validPattern() )
	{
		return;
	}

	if( !m_pattern->getTrack() )
	{
		if( engine::getSong()->playMode() != song::Mode_PlayPattern )
		{
			engine::getSong()->stop();
			engine::getSong()->playPattern( (pattern *)
				engine::getPianoRoll()->currentPattern() );
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else if( engine::getSong()->isPlaying() )
		{
			engine::getSong()->pause();
			m_playButton->setIcon( embed::getIconPixmap( "play" ) );
		}
		else if( engine::getSong()->isPaused() )
		{
			engine::getSong()->resumeFromPause();
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else
		{
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
			engine::getSong()->playPattern( (pattern *)
				engine::getPianoRoll()->currentPattern() );
		}
	}
	else if( inBBEditor() )
	{
		if( engine::getSong()->isPlaying() )
		{
			m_playButton->setIcon( embed::getIconPixmap( "play" ) );
		}
		else
		{
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		engine::getBBTrackContainer()->play();
	}
	else
	{
		if( engine::getSong()->isPlaying() )
		{
			engine::getSong()->pause();
			m_playButton->setIcon( embed::getIconPixmap( "play" ) );
		}
		else if( engine::getSong()->isPaused() )
		{
			engine::getSong()->resumeFromPause();
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else
		{
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
			engine::getSong()->play();
		}
	}
}




void automationEditor::stop()
{
	QMutexLocker m( &m_patternMutex );

	if( !validPattern() )
	{
		return;
	}
	if( m_pattern->getTrack() && inBBEditor() )
	{
		engine::getBBTrackContainer()->stop();
	}
	else
	{
		engine::getSong()->stop();
	}
	m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	m_playButton->update();
	m_scrollBack = true;
}




void automationEditor::horScrolled( int _new_pos )
{
	m_currentPosition = _new_pos;
	emit positionChanged( m_currentPosition );
	update();
}




void automationEditor::verScrolled( int _new_pos )
{
	m_scrollLevel = _new_pos;
	updateTopBottomLevels();
	update();
}




void automationEditor::drawButtonToggled()
{
	m_editMode = ModeDraw;
	removeSelection();
	update();
}




void automationEditor::eraseButtonToggled()
{
	m_editMode = ModeErase;
	removeSelection();
	update();
}




void automationEditor::selectButtonToggled()
{
	m_editMode = ModeSelect;
	removeSelection();
	update();
}




void automationEditor::moveButtonToggled()
{
	m_editMode = ModeMove;
	m_selValuesForMove.clear();
	getSelectedValues( m_selValuesForMove );
	update();
}




void automationEditor::selectAll()
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
void automationEditor::getSelectedValues( timeMap & _selected_values )
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
		tick_t len_ticks = DefaultTicksPerTact / 16;

		float level = it.value();
		tick_t pos_ticks = it.key();

		if( level >= selLevel_start && level <= selLevel_end &&
				pos_ticks >= sel_pos_start &&
				pos_ticks + len_ticks <= sel_pos_end )
		{
			_selected_values[it.key()] = level;
		}
	}
}




void automationEditor::copySelectedValues()
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
		textFloat::displayMessage( tr( "Values copied" ),
				tr( "All selected values were copied to the "
								"clipboard." ),
				embed::getIconPixmap( "edit_copy" ), 2000 );
	}
}




void automationEditor::cutSelectedValues()
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
		engine::getSong()->setModified();

		for( timeMap::iterator it = selected_values.begin();
					it != selected_values.end(); ++it )
		{
			m_valuesToCopy[it.key()] = it.value();
			m_pattern->removeValue( it.key() );
		}
	}

	update();
	engine::getSongEditor()->update();
}




void automationEditor::pasteValues()
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
		engine::getSong()->setModified();
		update();
		engine::getSongEditor()->update();
	}
}




void automationEditor::deleteSelectedValues()
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
		engine::getSong()->setModified();
		update();
		engine::getSongEditor()->update();
	}
}




void automationEditor::updatePosition( const midiTime & _t )
{
	if( ( engine::getSong()->isPlaying() &&
			engine::getSong()->playMode() ==
					song::Mode_PlayAutomationPattern ) ||
							m_scrollBack == true )
	{
		const int w = width() - ValuesWidth;
		if( _t > m_currentPosition + w * DefaultTicksPerTact / m_ppt )
		{
			m_leftRightScroll->setValue( _t.getTact() *
							DefaultTicksPerTact );
		}
		else if( _t < m_currentPosition )
		{
			midiTime t = qMax( _t - w * DefaultTicksPerTact *
					DefaultTicksPerTact / m_ppt, 0 );
			m_leftRightScroll->setValue( t.getTact() *
							DefaultTicksPerTact );
		}
		m_scrollBack = false;
	}
}




void automationEditor::zoomingXChanged()
{
	const QString & zfac = m_zoomingXModel.currentText();
	m_ppt = zfac.left( zfac.length() - 1 ).toInt() * DefaultPixelsPerTact / 100;
#ifdef LMMS_DEBUG
	assert( m_ppt > 0 );
#endif
	m_timeLine->setPixelsPerTact( m_ppt );
	update();
}




void automationEditor::zoomingYChanged()
{
	const QString & zfac = m_zoomingYModel.currentText();
	m_y_auto = zfac == "Auto";
	if( !m_y_auto )
	{
		m_y_delta = zfac.left( zfac.length() - 1 ).toInt()
							* DefaultYDelta / 100;
	}
#ifdef LMMS_DEBUG
	assert( m_y_delta > 0 );
#endif
	resizeEvent( NULL );
}




int automationEditor::quantization() const
{
	return( DefaultTicksPerTact /
		m_quantizeComboBox->model()->currentText().right(
			m_quantizeComboBox->model()->currentText().length() -
								2 ).toInt() );
}




void automationEditor::updateTopBottomLevels()
{
	if( m_y_auto )
	{
		m_bottomLevel = m_minLevel;
		m_topLevel = m_maxLevel;
		return;
	}

	int total_pixels = (int)( ( m_maxLevel - m_minLevel ) * m_y_delta + 1 );
	int grid_height = height() - TopMargin - ScrollBarSize;
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




#include "moc_automation_editor.cxx"


