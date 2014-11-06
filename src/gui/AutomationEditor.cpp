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
#include <QToolTip>


#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>


#include "SongEditor.h"
#include "MainWindow.h"
#include "embed.h"
#include "engine.h"
#include "pixmap_button.h"
#include "templates.h"
#include "gui_templates.h"
#include "timeline.h"
#include "tooltip.h"
#include "tool_button.h"
#include "text_float.h"
#include "combobox.h"
#include "bb_track_container.h"
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
	m_y_auto( TRUE ),
	m_editMode( DRAW ),
	m_scrollBack( FALSE ),
	m_gridColor( 0,0,0 ),
	m_graphColor(),
	m_vertexColor( 0,0,0 ),
	m_scaleColor()
{
	connect( this, SIGNAL( currentPatternChanged() ),
				this, SLOT( updateAfterPatternChange() ),
				Qt::QueuedConnection );
	connect( engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
						this, SLOT( update() ) );
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
	m_timeLine = new timeLine( VALUES_WIDTH, 32, m_ppt,
				engine::getSong()->getPlayPos(
					song::Mode_PlayAutomationPattern ),
						m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const MidiTime & ) ),
		m_timeLine, SLOT( updatePosition( const MidiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const MidiTime & ) ),
			this, SLOT( updatePosition( const MidiTime & ) ) );


	m_toolBar = new QWidget( this );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
	m_toolBar->setAutoFillBackground( TRUE );
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

	m_playButton->setObjectName( "playButton" );
	m_stopButton->setObjectName( "stopButton" );

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
	m_drawButton->setCheckable( TRUE );
	m_drawButton->setChecked( TRUE );

	m_eraseButton = new toolButton( embed::getIconPixmap( "edit_erase" ),
					tr( "Erase mode (Shift+E)" ),
					this, SLOT( eraseButtonToggled() ),
					m_toolBar );
	m_eraseButton->setCheckable( TRUE );

	//TODO: m_selectButton and m_moveButton are broken.
	/*m_selectButton = new toolButton( embed::getIconPixmap(
							"edit_select" ),
					tr( "Select mode (Shift+S)" ),
					this, SLOT( selectButtonToggled() ),
					m_toolBar );
	m_selectButton->setCheckable( TRUE );

	m_moveButton = new toolButton( embed::getIconPixmap( "edit_move" ),
					tr( "Move selection mode (Shift+M)" ),
					this, SLOT( moveButtonToggled() ),
					m_toolBar );
	m_moveButton->setCheckable( TRUE );*/

	QButtonGroup * tool_button_group = new QButtonGroup( this );
	tool_button_group->addButton( m_drawButton );
	tool_button_group->addButton( m_eraseButton );
	//tool_button_group->addButton( m_selectButton );
	//tool_button_group->addButton( m_moveButton );
	tool_button_group->setExclusive( TRUE );

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

	m_discreteButton = new toolButton( embed::getIconPixmap(
						"progression_discrete" ),
					tr( "Discrete progression" ),
					this, SLOT( discreteButtonToggled() ),
					m_toolBar );
	m_discreteButton->setCheckable( true );
	m_discreteButton->setChecked( true );

	m_linearButton = new toolButton( embed::getIconPixmap(
							"progression_linear" ),
					tr( "Linear progression" ),
					this, SLOT( linearButtonToggled() ),
					m_toolBar );
	m_linearButton->setCheckable( true );

	m_cubicHermiteButton = new toolButton( embed::getIconPixmap(
						"progression_cubic_hermite" ),
					tr( "Cubic Hermite progression" ),
					this, SLOT(
						cubicHermiteButtonToggled() ),
					m_toolBar );
	m_cubicHermiteButton->setCheckable( true );

	// setup tension-stuff
	m_tensionKnob = new knob( knobSmall_17, this, "Tension" );
	m_tensionModel = new FloatModel(1.0, 0.0, 1.0, 0.01);
	connect( m_tensionModel, SIGNAL( dataChanged() ),
				this, SLOT( tensionChanged() ) );

	QLabel * tension_lbl = new QLabel( m_toolBar );
	tension_lbl->setText( tr("Tension: ") );

	tool_button_group = new QButtonGroup( this );
	tool_button_group->addButton( m_discreteButton );
	tool_button_group->addButton( m_linearButton );
	tool_button_group->addButton( m_cubicHermiteButton );
	tool_button_group->setExclusive( true );

	m_discreteButton->setWhatsThis(
		tr( "Click here to choose discrete progressions for this "
			"automation pattern.  The value of the connected "
			"object will remain constant between control points "
			"and be set immediately to the new value when each "
			"control point is reached." ) );
	m_linearButton->setWhatsThis(
		tr( "Click here to choose linear progressions for this "
			"automation pattern.  The value of the connected "
			"object will change at a steady rate over time "
			"between control points to reach the correct value at "
			"each control point without a sudden change." ) );
	m_cubicHermiteButton->setWhatsThis(
		tr( "Click here to choose cubic hermite progressions for this "
			"automation pattern.  The value of the connected "
			"object will change in a smooth curve and ease in to "
			"the peaks and valleys." ) );

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
	for( int i = 0; i < 7; ++i )
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
	//tb_layout->addWidget( m_selectButton );
	//tb_layout->addWidget( m_moveButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_discreteButton );
	tb_layout->addWidget( m_linearButton );
	tb_layout->addWidget( m_cubicHermiteButton );
	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( tension_lbl );
	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_tensionKnob );
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

	setMouseTracking( TRUE );

	setMinimumSize( tb_layout->minimumSize().width(), 128 );

	// add us to workspace
	if( engine::mainWindow()->workspace() )
	{
		engine::mainWindow()->workspace()->addSubWindow( this );
		parentWidget()->resize( INITIAL_WIDTH, INITIAL_HEIGHT );
		parentWidget()->move( 5, 5 );
		parentWidget()->hide();
	}
	else
	{
		resize( INITIAL_WIDTH, INITIAL_HEIGHT );
		hide();
	}
}




AutomationEditor::~AutomationEditor()
{
	m_zoomingXModel.disconnect();
	m_zoomingYModel.disconnect();
	m_quantizeModel.disconnect();
	m_tensionModel->disconnect();

	delete m_tensionModel;
}




void AutomationEditor::setCurrentPattern( AutomationPattern * _new_pattern )
{
	m_patternMutex.lock();
	m_pattern = _new_pattern;
	m_patternMutex.unlock();

	emit currentPatternChanged();
}




void AutomationEditor::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void AutomationEditor::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}




void AutomationEditor::setPauseIcon( bool pause )
{
	if( pause == true )
	{
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	}
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

	if( m_pattern->progressionType() ==
				AutomationPattern::DiscreteProgression &&
				!m_discreteButton->isChecked() )
	{
		m_discreteButton->setChecked( true );
	}

	if( m_pattern->progressionType() ==
				AutomationPattern::LinearProgression &&
				!m_linearButton->isChecked() )
	{
		m_linearButton->setChecked( true );
	}

	if( m_pattern->progressionType() ==
				AutomationPattern::CubicHermiteProgression &&
				!m_cubicHermiteButton->isChecked() )
	{
		m_cubicHermiteButton->setChecked( true );
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
		engine::pianoRoll()->update();
	}
}




void AutomationEditor::removeSelection()
{
	m_selectStartTick = 0;
	m_selectedTick = 0;
	m_selectStartLevel = 0;
	m_selectedLevels = 0;
}




void AutomationEditor::closeEvent( QCloseEvent * _ce )
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




void AutomationEditor::keyPressEvent( QKeyEvent * _ke )
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
		//TODO: m_selectButton and m_moveButton are broken.
		/*case Qt::Key_A:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				m_selectButton->setChecked( TRUE );
				selectAll();
				update();
				_ke->accept();
			}
			break;*/

		case Qt::Key_D:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_drawButton->setChecked( TRUE );
				_ke->accept();
			}
			break;

		case Qt::Key_E:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_eraseButton->setChecked( TRUE );
				_ke->accept();
			}
			break;
		//TODO: m_selectButton and m_moveButton are broken.
		/*case Qt::Key_S:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_selectButton->setChecked( TRUE );
				_ke->accept();
			}
			break;

		case Qt::Key_M:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_moveButton->setChecked( TRUE );
				_ke->accept();
			}
			break;*/

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




void AutomationEditor::leaveEvent( QEvent * _e )
{
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}

	QWidget::leaveEvent( _e );
}


void AutomationEditor::drawLine( int _x0, float _y0, int _x1, float _y1 )
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
		m_pattern->removeValue( MidiTime( x ) );
		m_pattern->putValue( MidiTime( x ), y );
	}
}




void AutomationEditor::disableTensionKnob()
{
	m_tensionKnob->setEnabled( false );
}




void AutomationEditor::mousePressEvent( QMouseEvent * _me )
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		return;
	}

	if( _me->y() > TOP_MARGIN )
	{
		float level = getLevel( _me->y() );

		int x = _me->x();

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
			if( _me->button() == Qt::LeftButton &&
							m_editMode == DRAW )
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

				engine::getSong()->setModified();
			}
			else if( ( _me->button() == Qt::RightButton &&
							m_editMode == DRAW ) ||
					m_editMode == ERASE )
			{
				// erase single value
				if( it != time_map.end() )
				{
					m_pattern->removeValue( it.key() );
					engine::getSong()->setModified();
				}
				m_action = NONE;
			}
			else if( _me->button() == Qt::LeftButton &&
							m_editMode == SELECT )
			{
				// select an area of values

				m_selectStartTick = pos_ticks;
				m_selectedTick = 0;
				m_selectStartLevel = level;
				m_selectedLevels = 1;
				m_action = SELECT_VALUES;
			}
			else if( _me->button() == Qt::RightButton &&
							m_editMode == SELECT )
			{
				// when clicking right in select-move, we
				// switch to move-mode
				m_moveButton->setChecked( TRUE );
			}
			else if( _me->button() == Qt::LeftButton &&
							m_editMode == MOVE )
			{
				// move selection (including selected values)

				// save position where move-process began
				m_moveStartTick = pos_ticks;
				m_moveStartLevel = level;

				m_action = MOVE_SELECTION;

				engine::getSong()->setModified();
			}
			else if( _me->button() == Qt::RightButton &&
							m_editMode == MOVE )
			{
				// when clicking right in select-move, we
				// switch to draw-mode
				m_drawButton->setChecked( TRUE );
			}

			update();
		}
	}
}




void AutomationEditor::mouseReleaseEvent( QMouseEvent * _me )
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
void AutomationEditor::mouseMoveEvent( QMouseEvent * _me )
{
	QMutexLocker m( &m_patternMutex );
	if( !validPattern() )
	{
		update();
		return;
	}

	if( _me->y() > TOP_MARGIN )
	{
		float level = getLevel( _me->y() );
		int x = _me->x();

		if( _me->x() <= VALUES_WIDTH )
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
		if( _me->buttons() & Qt::LeftButton && m_editMode == DRAW )
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

			engine::getSong()->setModified();

		}
		else if( ( _me->buttons() & Qt::RightButton &&
						m_editMode == DRAW ) ||
				( _me->buttons() & Qt::LeftButton &&
						m_editMode == ERASE ) )
		{
			m_pattern->removeValue( MidiTime( pos_ticks ) );
		}
		else if( _me->buttons() & Qt::NoButton && m_editMode == DRAW )
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
						m_editMode == SELECT &&
						m_action == SELECT_VALUES )
		{

			// change size of selection

			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint(
						VALUES_WIDTH, _me->y() ) ) );
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
								_me->y() ) ) );
				m_leftRightScroll->setValue( m_currentPosition +
									4 );
			}

			// get tick in which the cursor is posated
			int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
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
									FALSE )]
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
					m_editMode == SELECT &&
					m_action == SELECT_VALUES )
		{

			int x = _me->x() - VALUES_WIDTH;
			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint( VALUES_WIDTH,
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
			else if( x > width() - VALUES_WIDTH )
			{
				x = width() - VALUES_WIDTH;
				QCursor::setPos( mapToGlobal( QPoint( width(),
							_me->y() ) ) );
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
				m_selectedTick = -qRound( m_selectStartTick );
			}

			float level = getLevel( _me->y() );

			if( level <= m_bottomLevel )
			{
				QCursor::setPos( mapToGlobal( QPoint( _me->x(),
							height() -
							SCROLLBAR_SIZE ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
				level = m_bottomLevel;
			}
			else if( level >= m_topLevel )
			{
				QCursor::setPos( mapToGlobal( QPoint( _me->x(),
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




inline void AutomationEditor::drawCross( QPainter & _p )
{
	QPoint mouse_pos = mapFromGlobal( QCursor::pos() );
	float level = getLevel( mouse_pos.y() );
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	float cross_y = m_y_auto ?
		grid_bottom - ( ( grid_bottom - TOP_MARGIN )
				* ( level - m_minLevel )
				/ (float)( m_maxLevel - m_minLevel ) ) :
		grid_bottom - ( level - m_bottomLevel ) * m_y_delta;

	_p.setPen( QColor( 0xFF, 0x33, 0x33 ) );
	_p.drawLine( VALUES_WIDTH, (int) cross_y, width(), (int) cross_y );
	_p.drawLine( mouse_pos.x(), TOP_MARGIN, mouse_pos.x(),
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




void AutomationEditor::paintEvent( QPaintEvent * _pe )
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
			engine::getSong()->getTimeSigModel().getDenominator();
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

				bool is_selected = FALSE;
				// if we're in move-mode, we may only draw
				// values in selected area, that have originally
				// been selected and not values that are now in
				// selection because the user moved it...
				if( m_editMode == MOVE )
				{
					if( m_selValuesForMove.contains( it.key() ) )
					{
						is_selected = TRUE;
					}
				}
				else if( it.value() >= selLevel_start &&
					it.value() <= selLevel_end &&
					it.key() >= sel_pos_start &&
					it.key() + len_ticks <= sel_pos_end )
				{
					is_selected = TRUE;
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
		f.setBold( TRUE );
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




int AutomationEditor::xCoordOfTick( int _tick )
{
	return VALUES_WIDTH + ( ( _tick - m_currentPosition )
						* m_ppt / MidiTime::ticksPerTact() );
}




int AutomationEditor::yCoordOfLevel( float _level )
{
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	if( m_y_auto )
	{
		return (int)( grid_bottom - ( grid_bottom - TOP_MARGIN )
						* ( _level - m_minLevel )
						/ ( m_maxLevel - m_minLevel ) );
	}
	else
	{
		return (int)( grid_bottom - ( _level - m_bottomLevel )
								* m_y_delta );
	}
}




void AutomationEditor::drawLevelTick( QPainter & _p, int _tick, float _level,
							bool _is_selected )
{
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	const int x = xCoordOfTick( _tick );
	int rect_width = xCoordOfTick( _tick+1 ) - x;

	// is the level in visible area?
	if( ( _level >= m_bottomLevel && _level <= m_topLevel )
			|| ( _level > m_topLevel && m_topLevel >= 0 )
			|| ( _level < m_bottomLevel && m_bottomLevel <= 0 ) )
	{
		int y_start = yCoordOfLevel( _level );
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
			rect_height = (int)( _level * m_y_delta );
		}

		QBrush currentColor = _is_selected
			? QBrush( QColor( 0x00, 0x40, 0xC0 ) )
			: graphColor();

		_p.fillRect( x, y_start, rect_width, rect_height, currentColor );
	}
	
	else
	{
		printf("not in range\n");
	}
	
}




// responsible for moving/resizing scrollbars after window-resizing
void AutomationEditor::resizeEvent( QResizeEvent * )
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

	if( engine::getSong() )
	{
		engine::getSong()->getPlayPos( song::Mode_PlayAutomationPattern
					).m_timeLine->setFixedWidth( width() );
	}
	m_toolBar->setFixedWidth( width() );

	updateTopBottomLevels();
	update();
}




void AutomationEditor::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	if( _we->modifiers() & Qt::ControlModifier && _we->modifiers() & Qt::ShiftModifier )
	{
		int y = m_zoomingYModel.value();
		if( _we->delta() > 0 )
		{
			y++;
		}
		if( _we->delta() < 0 )
		{
			y--;
		}
		y = qBound( 0, y, m_zoomingYModel.size() - 1 );
		m_zoomingYModel.setValue( y );	
	}
	else if( _we->modifiers() & Qt::ControlModifier && _we->modifiers() & Qt::AltModifier )
	{
		int q = m_quantizeModel.value();
		if( _we->delta() > 0 )
		{
			q--;
		}
		if( _we->delta() < 0 )
		{
			q++;
		}
		q = qBound( 0, q, m_quantizeModel.size() - 1 );
		m_quantizeModel.setValue( q );
		update();
	}
	else if( _we->modifiers() & Qt::ControlModifier )
	{
		int x = m_zoomingXModel.value();
		if( _we->delta() > 0 )
		{
			x++;
		}
		if( _we->delta() < 0 )
		{
			x--;
		}
		x = qBound( 0, x, m_zoomingXModel.size() - 1 );
		m_zoomingXModel.setValue( x );
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




float AutomationEditor::getLevel( int _y )
{
	int level_line_y = height() - SCROLLBAR_SIZE - 1;
	// pressed level
	float level = roundf( ( m_bottomLevel + ( m_y_auto ?
			( m_maxLevel - m_minLevel ) * ( level_line_y - _y )
					/ (float)( level_line_y - ( TOP_MARGIN + 2 ) ) :
			( level_line_y - _y ) / (float)m_y_delta ) ) / m_step ) * m_step;
	// some range-checking-stuff
	level = qBound( m_bottomLevel, level, m_topLevel );

	return( level );
}




inline bool AutomationEditor::inBBEditor()
{
	QMutexLocker m( &m_patternMutex );
	return( validPattern() &&
				m_pattern->getTrack()->trackContainer() == engine::getBBTrackContainer() );
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
		if( engine::getSong()->playMode() != song::Mode_PlayPattern )
		{
			engine::getSong()->stop();
			engine::getSong()->playPattern( (Pattern *) engine::pianoRoll()->currentPattern() );
		}
		else if( engine::getSong()->isStopped() == false )
		{
			engine::getSong()->togglePause();
		}
		else
		{
			engine::getSong()->playPattern( (Pattern *) engine::pianoRoll()->currentPattern() );
		}
	}
	else if( inBBEditor() )
	{
		engine::getBBTrackContainer()->play();
	}
	else
	{
		if( engine::getSong()->isStopped() == true )
		{
			engine::getSong()->playSong();
		}
		else
		{
			engine::getSong()->togglePause();
		}
	}

	setPauseIcon( engine::getSong()->isPlaying() );
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
		engine::getBBTrackContainer()->stop();
	}
	else
	{
		engine::getSong()->stop();
	}
	m_scrollBack = TRUE;
}




void AutomationEditor::horScrolled( int _new_pos )
{
	m_currentPosition = _new_pos;
	emit positionChanged( m_currentPosition );
	update();
}




void AutomationEditor::verScrolled( int _new_pos )
{
	m_scrollLevel = _new_pos;
	updateTopBottomLevels();
	update();
}




void AutomationEditor::drawButtonToggled()
{
	m_editMode = DRAW;
	removeSelection();
	update();
}




void AutomationEditor::eraseButtonToggled()
{
	m_editMode = ERASE;
	removeSelection();
	update();
}




void AutomationEditor::selectButtonToggled()
{
	m_editMode = SELECT;
	removeSelection();
	update();
}




void AutomationEditor::moveButtonToggled()
{
	m_editMode = MOVE;
	m_selValuesForMove.clear();
	getSelectedValues( m_selValuesForMove );
	update();
}




void AutomationEditor::discreteButtonToggled()
{
	if ( validPattern() )
	{
		QMutexLocker m( &m_patternMutex );
		disableTensionKnob();
		m_pattern->setProgressionType(
				AutomationPattern::DiscreteProgression );
		engine::getSong()->setModified();
		update();
	}
}




void AutomationEditor::linearButtonToggled()
{
	if ( validPattern() )
	{
		QMutexLocker m( &m_patternMutex );
		disableTensionKnob();
		m_pattern->setProgressionType(
					AutomationPattern::LinearProgression );
		engine::getSong()->setModified();
		update();
	}
}




void AutomationEditor::cubicHermiteButtonToggled()
{
	if ( validPattern() )
	{
		m_tensionKnob->setModel( m_tensionModel );
		m_tensionKnob->setEnabled( true );
		toolTip::add( m_tensionKnob, tr( "Tension value for spline" ) );
		m_tensionKnob->setWhatsThis(
			tr( "A higher tension value may make a smoother curve "
				"but overshoot some values.  A low tension "
				"value will cause the slope of the curve to "
				"level off at each control point." ) );
		m_pattern->setProgressionType(
				AutomationPattern::CubicHermiteProgression );
		engine::getSong()->setModified();
		update();
	}
}




void AutomationEditor::tensionChanged()
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
void AutomationEditor::getSelectedValues( timeMap & _selected_values )
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
			_selected_values[it.key()] = level;
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
		textFloat::displayMessage( tr( "Values copied" ),
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
		engine::getSong()->setModified();

		for( timeMap::iterator it = selected_values.begin();
					it != selected_values.end(); ++it )
		{
			m_valuesToCopy[it.key()] = it.value();
			m_pattern->removeValue( it.key() );
		}
	}

	update();
	engine::songEditor()->update();
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
		engine::getSong()->setModified();
		update();
		engine::songEditor()->update();
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

	if( update_after_delete == TRUE )
	{
		engine::getSong()->setModified();
		update();
		engine::songEditor()->update();
	}
}




void AutomationEditor::updatePosition( const MidiTime & _t )
{
	if( ( engine::getSong()->isPlaying() &&
			engine::getSong()->playMode() ==
					song::Mode_PlayAutomationPattern ) ||
							m_scrollBack == TRUE )
	{
		const int w = width() - VALUES_WIDTH;
		if( _t > m_currentPosition + w * MidiTime::ticksPerTact() / m_ppt )
		{
			m_leftRightScroll->setValue( _t.getTact() *
							MidiTime::ticksPerTact() );
		}
		else if( _t < m_currentPosition )
		{
			MidiTime t = qMax( _t - w * MidiTime::ticksPerTact() *
					MidiTime::ticksPerTact() / m_ppt, 0 );
			m_leftRightScroll->setValue( t.getTact() *
							MidiTime::ticksPerTact() );
		}
		m_scrollBack = FALSE;
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
	return( DefaultTicksPerTact /
		m_quantizeComboBox->model()->currentText().right(
			m_quantizeComboBox->model()->currentText().length() -
								2 ).toInt() );
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




#include "moc_AutomationEditor.cxx"


