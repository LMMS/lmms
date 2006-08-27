#ifndef SINGLE_SOURCE_COMPILE

/*
 * automation_editor.cpp - implementation of automationEditor which is used for
 *                         actual setting of dynamic values
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QLayout>
#include <QtGui/QLabel>

#else

#include <qapplication.h>
#include <qbuttongroup.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qdom.h>

#define addButton insert
#define setCheckable setToggleButton

#endif


#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>


#include "automation_editor.h"
#include "song_editor.h"
#include "main_window.h"
#include "embed.h"
#include "pixmap_button.h"
#include "templates.h"
#include "gui_templates.h"
#include "timeline.h"
#include "tooltip.h"
#include "midi.h"
#include "tool_button.h"
#include "text_float.h"
#include "combobox.h"
#include "bb_editor.h"
#include "piano_roll.h"


QPixmap * automationEditor::s_toolDraw = NULL;
QPixmap * automationEditor::s_toolErase = NULL;
QPixmap * automationEditor::s_toolSelect = NULL;
QPixmap * automationEditor::s_toolMove = NULL;


automationEditor::automationEditor( engine * _engine ) :
	QWidget( _engine->getMainWindow()->workspace() ),
	journallingObject( _engine ),
	m_paintPixmap(),
	m_cursorInside( FALSE ),
	m_pattern( NULL ),
	m_min_level( 0 ),
	m_max_level( 0 ),
	m_scroll_level( 0 ),
	m_bottom_level( 0 ),
	m_top_level( 0 ),
	m_currentPosition(),
	m_action( NONE ),
	m_moveStartLevel( 0 ),
	m_moveStartTact64th( 0 ),
	m_ppt( DEFAULT_PPT ),
	m_y_delta( DEFAULT_Y_DELTA ),
	m_y_auto( TRUE ),
	m_editMode( DRAW ),
	m_scrollBack( FALSE )
{
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

#ifdef QT4
	// add us to workspace
	eng()->getMainWindow()->workspace()->addWindow( this );
#endif

	// add time-line
	m_timeLine = new timeLine( VALUES_WIDTH, 32, m_ppt,
				eng()->getSongEditor()->getPlayPos(
					songEditor::PLAY_AUTOMATION_PATTERN ),
					m_currentPosition, this, eng() );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
		m_timeLine, SLOT( updatePosition( const midiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( updatePosition( const midiTime & ) ) );


	m_toolBar = new QWidget( this );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
#ifdef QT4
	m_toolBar->setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(),
					embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );
#else
	m_toolBar->setPaletteBackgroundPixmap( embed::getIconPixmap(
							"toolbar_bg" ) );
#endif

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

#ifdef QT4
	m_playButton->setWhatsThis(
#else
	QWhatsThis::add( m_playButton,
#endif
		tr( "Click here, if you want to play the current pattern. "
			"This is useful while editing it. The pattern is "
			"automatically looped when its end is reached." ) );
#ifdef QT4
	m_stopButton->setWhatsThis(
#else
	QWhatsThis::add( m_stopButton,
#endif
		tr( "Click here, if you want to stop playing of current "
			"pattern." ) );



	removeSelection();

	// init scrollbars
	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_topBottomScroll = new QScrollBar( Qt::Vertical, this );
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ), this,
						SLOT( horScrolled( int ) ) );
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

	m_selectButton = new toolButton( embed::getIconPixmap(
							"edit_select" ),
					tr( "Select mode (Shift+S)" ),
					this, SLOT( selectButtonToggled() ),
					m_toolBar );
	m_selectButton->setCheckable( TRUE );

	m_moveButton = new toolButton( embed::getIconPixmap( "edit_move" ),
					tr( "Move selection mode (Shift+M)" ),
					this, SLOT( moveButtonToggled() ),
					m_toolBar );
	m_moveButton->setCheckable( TRUE );

	QButtonGroup * tool_button_group = new QButtonGroup( this );
	tool_button_group->addButton( m_drawButton );
	tool_button_group->addButton( m_eraseButton );
	tool_button_group->addButton( m_selectButton );
	tool_button_group->addButton( m_moveButton );
	tool_button_group->setExclusive( TRUE );
#ifndef QT4
	tool_button_group->hide();
#endif

#ifdef QT4
	m_drawButton->setWhatsThis(
#else
	QWhatsThis::add( m_drawButton,
#endif
		tr( "If you click here, draw-mode will be activated. In this "
			"mode you can add and move single values. This "
			"is the default-mode which is used most of the time. "
			"You can also press 'Shift+D' on your keyboard to "
			"activate this mode." ) );
#ifdef QT4
	m_eraseButton->setWhatsThis(
#else
	QWhatsThis::add( m_eraseButton,
#endif
		tr( "If you click here, erase-mode will be activated. In this "
			"mode you can erase single values. You can also press "
			"'Shift+E' on your keyboard to activate this mode." ) );
#ifdef QT4
	m_selectButton->setWhatsThis(
#else
	QWhatsThis::add( m_selectButton,
#endif
		tr( "If you click here, select-mode will be activated. In this "
			"mode you can select values. This is neccessary "
			"if you want to cut, copy, paste, delete or move "
			"values. You can also press 'Shift+S' on your keyboard "
			"to activate this mode." ) );
#ifdef QT4
	m_moveButton->setWhatsThis(
#else
	QWhatsThis::add( m_moveButton,
#endif
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

#ifdef QT4
	m_cutButton->setWhatsThis(
#else
	QWhatsThis::add( m_cutButton,
#endif
		tr( "If you click here, selected values will be cut into the "
			"clipboard. You can paste them anywhere in any pattern "
			"by clicking on the paste-button." ) );
#ifdef QT4
	m_copyButton->setWhatsThis(
#else
	QWhatsThis::add( m_copyButton,
#endif
		tr( "If you click here, selected values will be copied into "
			"the clipboard. You can paste them anywhere in any "
			"pattern by clicking on the paste-button." ) );
#ifdef QT4
	m_pasteButton->setWhatsThis(
#else
	QWhatsThis::add( m_pasteButton,
#endif
		tr( "If you click here, the values from the clipboard will be "
			"pasted at the first visible tact." ) );



	// setup zooming-stuff
	QLabel * zoom_x_lbl = new QLabel( m_toolBar );
	zoom_x_lbl->setPixmap( embed::getIconPixmap( "zoom_x" ) );

	m_zoomingXComboBox = new comboBox( m_toolBar, NULL, eng(), NULL );
	m_zoomingXComboBox->setFixedSize( 80, 22 );
	for( int i = 0; i < 6; ++i )
	{
		m_zoomingXComboBox->addItem( QString::number( 25 << i ) + "%" );
	}
	m_zoomingXComboBox->setValue( m_zoomingXComboBox->findText( "100%" ) );
	connect( m_zoomingXComboBox, SIGNAL( activated( const QString & ) ),
			this, SLOT( zoomingXChanged( const QString & ) ) );


	QLabel * zoom_y_lbl = new QLabel( m_toolBar );
	zoom_y_lbl->setPixmap( embed::getIconPixmap( "zoom_y" ) );

	m_zoomingYComboBox = new comboBox( m_toolBar, NULL, eng(), NULL );
	m_zoomingYComboBox->setFixedSize( 80, 22 );
	m_zoomingYComboBox->addItem( "Auto" );
	for( int i = 0; i < 6; ++i )
	{
		m_zoomingYComboBox->addItem( QString::number( 25 << i ) + "%" );
	}
	m_zoomingYComboBox->setValue( m_zoomingYComboBox->findText( "Auto" ) );
	connect( m_zoomingYComboBox, SIGNAL( activated( const QString & ) ),
			this, SLOT( zoomingYChanged( const QString & ) ) );


	// setup quantize-stuff
	QLabel * quantize_lbl = new QLabel( m_toolBar );
	quantize_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	m_quantizeComboBox = new comboBox( m_toolBar, NULL, eng(), NULL );
	m_quantizeComboBox->setFixedSize( 60, 22 );
	for( int i = 0; i < 7; ++i )
	{
		m_quantizeComboBox->addItem( "1/" + QString::number( 1 << i ) );
	}
	m_quantizeComboBox->setValue( m_quantizeComboBox->findText(
								"1/16" ) );

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
	setWindowIcon( embed::getIconPixmap( "automation" ) );
	resize( INITIAL_WIDTH, INITIAL_HEIGHT );
	setCurrentPattern( NULL );

#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif
	setMouseTracking( TRUE );

	hide();

}




automationEditor::~automationEditor()
{
}




void automationEditor::setCurrentPattern( automationPattern * _new_pattern )
{
	m_pattern = _new_pattern;
	m_currentPosition = 0;

	if( validPattern() == FALSE )
	{
		setWindowTitle( tr( "Automation Editor - no pattern" ) );
		m_min_level = m_max_level = m_scroll_level = 0;
		resizeEvent( NULL );
		return;
	}

	m_min_level = m_pattern->object()->minLevel();
	m_max_level = m_pattern->object()->maxLevel();
	m_scroll_level = ( m_min_level + m_max_level ) / 2;

	timeMap & time_map = m_pattern->getTimeMap();
	//TODO: This is currently unused
	int central_key = 0;
	if( !time_map.isEmpty() )
	{
		// determine the central key so that we can scroll to it
		int total_values = 0;
		for( timeMap::iterator it = time_map.begin();
						it != time_map.end(); ++it )
		{
			central_key += it.data();
			++total_values;
		}

	}
	// resizeEvent() does the rest for us (scrolling, range-checking
	// of levels and so on...)
	resizeEvent( NULL );

	setWindowTitle( tr( "Automation Editor - %1" ).arg(
							m_pattern->name() ) );

	update();
}




void automationEditor::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	mainWindow::saveWidgetState( this, _this );
}




void automationEditor::loadSettings( const QDomElement & _this )
{
	mainWindow::restoreWidgetState( this, _this );
}




inline void automationEditor::drawValueRect( QPainter & _p,
						Uint16 _x, Uint16 _y,
						Sint16 _width, Sint16 _height,
						const bool _is_selected )
{
	QColor current_color( 0xFF, 0xB0, 0x00 );
	if( _is_selected == TRUE )
	{
		current_color.setRgb( 0x00, 0x40, 0xC0 );
	}
	_p.fillRect( _x, _y, _width, _height, current_color );

	_p.setPen( QColor( 0xFF, 0xDF, 0x20 ) );
	_p.drawLine( _x - 1, _y, _x + 1, _y );
	_p.drawLine( _x, _y - 1, _x, _y + 1 );

//	_p.setPen( QColor( 0xFF, 0x9F, 0x00 ) );

//	_p.setPen( QColor( 0xFF, 0xFF, 0x40 ) );
}




void automationEditor::updatePaintPixmap( void )
{
	if( m_paintPixmap.isNull() == TRUE || m_paintPixmap.size() != size() )
	{
		m_paintPixmap = QPixmap( size() );
	}
	m_paintPixmap.fill( QColor( 0, 0, 0 ) );
	QPainter p( &m_paintPixmap );

	// set font-size to 8
	p.setFont( pointSize<8>( p.font() ) );

	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;

	// start drawing at the bottom
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;

	p.fillRect( 0, TOP_MARGIN, VALUES_WIDTH, height() - TOP_MARGIN,
						QColor( 0x33, 0x33, 0x33 ) );

	// print value numbers
	int font_height = p.fontMetrics().height();
	AlignmentFlags text_flags =
				(AlignmentFlags)( AlignRight | AlignVCenter );

	if( m_pattern )
	{
		if( m_y_auto )
		{
			int y[] = { grid_bottom, TOP_MARGIN + font_height / 2 };
			int level[] = { m_min_level, m_max_level };
			for( int i = 0; i < 2; ++i )
			{
				const QString & label = m_pattern->object()
						->levelToLabel( level[i] );
				p.setPen( QColor( 240, 240, 240 ) );
				p.drawText( 1, y[i] - font_height + 1,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
				p.setPen( QColor( 0, 0, 0 ) );
				p.drawText( 0, y[i] - font_height,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
			}
		}
		else
		{
			int y = grid_bottom;
			int level = m_bottom_level;
			int printable = tMax( 1, 5 * DEFAULT_Y_DELTA
								/ m_y_delta );
			int module = level % printable;
			if( module )
			{
				int inv_module = ( printable - module )
								% printable;
				y -= inv_module * m_y_delta;
				level += inv_module;
			}
			for( ; y >= TOP_MARGIN && level <= m_top_level;
				y -= printable * m_y_delta, level += printable )
			{
				const QString & label = m_pattern->object()
							->levelToLabel( level );
				p.setPen( QColor( 240, 240, 240 ) );
				p.drawText( 1, y - font_height + 1,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
				p.setPen( QColor( 0, 0, 0 ) );
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
	int tact_16th = m_currentPosition / 4;
	const int offset = ( m_currentPosition % 4 ) * m_ppt /
						DEFAULT_STEPS_PER_TACT / 4;

	if( m_pattern )
	{
		int x_line_end = m_y_auto || m_top_level < m_max_level ?
			TOP_MARGIN :
			grid_bottom - ( m_top_level - m_bottom_level )
								* m_y_delta;

		for( int x = VALUES_WIDTH - offset; x < width();
			x += m_ppt / DEFAULT_STEPS_PER_TACT, ++tact_16th )
		{
			if( x >= VALUES_WIDTH )
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
			p.drawLine( VALUES_WIDTH, grid_bottom, width(),
								grid_bottom );
			pen.setStyle( DotLine );
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
			for( int y = grid_bottom, level = m_bottom_level;
					y >= TOP_MARGIN && level <= m_top_level;
					y -= m_y_delta, ++level )
			{
				if( level % 5 == 0 )
				{
					p.setPen( QColor( 0x4F, 0x4F, 0x4F ) );
				}
				else
				{
					p.setPen( QColor( 0x3F, 0x3F, 0x3F ) );
				}

				// draw level line
				p.drawLine( VALUES_WIDTH, y, width(), y );
			}
		}
	}



	// following code draws all visible values

	// setup selection-vars
	int sel_pos_start = m_selectStartTact64th;
	int sel_pos_end = m_selectStartTact64th+m_selectedTact64th;
	if( sel_pos_start > sel_pos_end )
	{
		qSwap<int>( sel_pos_start, sel_pos_end );
	}

	int sel_level_start = m_selectStartLevel;
	int sel_level_end = sel_level_start + m_selectedLevels;
	if( sel_level_start > sel_level_end )
	{
		qSwap<int>( sel_level_start, sel_level_end );
	}

	if( validPattern() == TRUE )
	{
		timeMap & time_map = m_pattern->getTimeMap();

		for( timeMap::iterator it = time_map.begin(), it_next;
						it != time_map.end(); ++it )
		{
			Sint32 len_tact_64th = 4;

			const int level = it.data();

			Sint32 pos_tact_64th = it.key();

			const int x = ( pos_tact_64th - m_currentPosition ) *
								m_ppt / 64;

			it_next = it;
			++it_next;

			int next_x;
			int rect_width;
			if( it_next != time_map.end() )
			{
				Sint32 next_pos_tact_64th = it_next.key();
				next_x = ( next_pos_tact_64th
					- m_currentPosition ) * m_ppt / 64;
				rect_width = next_x - x;
			}
			else
			{
				next_x = -1;
				rect_width = width() - x;
			}

			// skip this value if not in visible area at all
			if( !xVisible( x ) && !xVisible( next_x )
						&& it_next != time_map.end() )
			{
				continue;
			}

			// is the value in visible area?
			if( ( level >= m_bottom_level && level <= m_top_level )
				|| ( level > m_top_level && m_top_level >= 0 )
				|| ( level < m_bottom_level
						&& m_bottom_level <= 0 ) )
			{
				bool is_selected = FALSE;
				// if we're in move-mode, we may only draw
				// values in selected area, that have originally
				// been selected and not values that are now in
				// selection because the user moved it...
				if( m_editMode == MOVE )
				{
					if( qFind( m_selValuesForMove.begin(),
						m_selValuesForMove.end(), *it )
						!= m_selValuesForMove.end() )
					{
						is_selected = TRUE;
					}
				}
				else if( level > sel_level_start &&
					level <= sel_level_end &&
					pos_tact_64th >= sel_pos_start &&
					pos_tact_64th + len_tact_64th <=
								sel_pos_end )
				{
					is_selected = TRUE;
				}

				// we've done and checked all, lets draw the
				// value
				int y_start;
				int rect_height;
				if( m_y_auto )
				{
					y_start = grid_bottom
						- ( grid_bottom - TOP_MARGIN )
						* ( level - m_min_level )
						/ ( m_max_level - m_min_level );
					int y_end = grid_bottom
						+ ( grid_bottom - TOP_MARGIN )
						* m_min_level
						/ ( m_max_level - m_min_level );
					rect_height = y_end - y_start;
				}
				else
				{
					y_start = grid_bottom - ( level
							- m_bottom_level )
							* m_y_delta;
					rect_height = level * m_y_delta;
				}
				drawValueRect( p, x + VALUES_WIDTH, y_start,
							rect_width, rect_height,
							is_selected );
			}
		}
	}
	else
	{
		QFont f = p.font();
		f.setBold( TRUE );
		p.setFont( pointSize<14>( f ) );
		p.setPen( QColor( 0, 255, 0 ) );
		p.drawText( VALUES_WIDTH + 20, TOP_MARGIN + 40,
				width() - VALUES_WIDTH - 20 - SCROLLBAR_SIZE,
				grid_height - 40, WordBreak,
				tr( "Please open an automation pattern with "
					"the context menu of a control!" ) );
	}

	p.setClipRect( VALUES_WIDTH, TOP_MARGIN, width() - VALUES_WIDTH,
								grid_height );

	// now draw selection-frame
	int x = ( sel_pos_start - m_currentPosition ) * m_ppt / 64;
	int w = ( sel_pos_end - sel_pos_start ) * m_ppt / 64;
	int y, h;
	if( m_y_auto )
	{
		y = grid_bottom - (int)roundf( ( grid_bottom - TOP_MARGIN )
				* ( sel_level_start - m_min_level )
				/ (float)( m_max_level - m_min_level ) );
		h = grid_bottom - (int)roundf( ( grid_bottom - TOP_MARGIN )
				* ( sel_level_end - m_min_level )
				/ (float)( m_max_level - m_min_level ) ) - y;
	}
	else
	{
		y = grid_bottom - ( sel_level_start - m_bottom_level )
								* m_y_delta;
		h = ( sel_level_start - sel_level_end ) * m_y_delta;
	}
	p.setPen( QColor( 0, 64, 192 ) );
	p.drawRect( x + VALUES_WIDTH, y, w, h );

	int l = ( validPattern() == TRUE )? (int) m_pattern->length() : 0;

	// reset scroll-range
	m_leftRightScroll->setRange( 0, l );
#ifdef QT4
	m_leftRightScroll->setSingleStep( 1 );
	m_leftRightScroll->setPageStep( l );
#else
	m_leftRightScroll->setSteps( 1, l );
#endif
}




void automationEditor::removeSelection( void )
{
	m_selectStartTact64th = 0;
	m_selectedTact64th = 0;
	m_selectStartLevel = 0;
	m_selectedLevels = 0;
}




void automationEditor::closeEvent( QCloseEvent * _ce )
{
	QApplication::restoreOverrideCursor();
	hide();
	_ce->ignore ();
}




void automationEditor::enterEvent( QEvent * _e )
{
	m_cursorInside = TRUE;
	QWidget::enterEvent( _e );
}




void automationEditor::keyPressEvent( QKeyEvent * _ke )
{
	switch( _ke->key() )
	{
		case Qt::Key_Up:
			m_topBottomScroll->setValue(
					m_topBottomScroll->value() - 1 );
			break;

		case Qt::Key_Down:
			m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
			break;

		case Qt::Key_Left:
		{
			if( ( m_timeLine->pos() -= 16 ) < 0 )
			{
				m_timeLine->pos().setTact( 0 );
				m_timeLine->pos().setTact64th( 0 );
			}
			m_timeLine->updatePosition();
			break;
		}
		case Qt::Key_Right:
		{
			m_timeLine->pos() += 16;
			m_timeLine->updatePosition();
			break;
		}

		case Qt::Key_C:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				copySelectedValues();
			}
			else
			{
				_ke->ignore();
			}
			break;

		case Qt::Key_X:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				cutSelectedValues();
			}
			else
			{
				_ke->ignore();
			}
			break;

		case Qt::Key_V:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				pasteValues();
			}
			else
			{
				_ke->ignore();
			}
			break;

		case Qt::Key_A:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				m_selectButton->setChecked( TRUE );
				selectAll();
				update();
			}
			else
			{
				_ke->ignore();
			}
			break;

		case Qt::Key_D:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_drawButton->setChecked( TRUE );
			}
			else
			{
				_ke->ignore();
			}
			break;

		case Qt::Key_E:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_eraseButton->setChecked( TRUE );
			}
			else
			{
				_ke->ignore();
			}
			break;

		case Qt::Key_S:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_selectButton->setChecked( TRUE );
			}
			else
			{
				_ke->ignore();
			}
			break;

		case Qt::Key_M:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				m_moveButton->setChecked( TRUE );
			}
			else
			{
				_ke->ignore();
			}
			break;

		case Qt::Key_Delete:
			deleteSelectedValues();
			break;

		case Qt::Key_Space:
			if( eng()->getSongEditor()->playing() )
			{
				stop();
			}
			else
			{
				play();
			}
			break;

		case Qt::Key_Home:
			m_timeLine->pos().setTact( 0 );
			m_timeLine->pos().setTact64th( 0 );
			m_timeLine->updatePosition();
			break;

		default:
			_ke->ignore();
			break;
	}
}




void automationEditor::leaveEvent( QEvent * _e )
{
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}
	m_cursorInside = FALSE;

	QWidget::leaveEvent( _e );
}




void automationEditor::mousePressEvent( QMouseEvent * _me )
{
	if( validPattern() == FALSE )
	{
		return;
	}

	if( _me->y() > TOP_MARGIN )
	{
		int level = getLevel( _me->y() );

		int x = _me->x();

		if( x > VALUES_WIDTH )
		{
			// set or move value

			x -= VALUES_WIDTH;

			// get tact-64th in which the user clicked
			int pos_tact_64th = x * 64 / m_ppt +
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
				if( pos_tact_64th >= it.key() &&
					len > 0 &&
					pos_tact_64th <= it.key() + len &&
					it.data() == level )
				{
					break;
				}
				++it;
			}

			// left button??
			if( _me->button() == Qt::LeftButton &&
							m_editMode == DRAW )
			{
				// did it reach end of map because
				// there's no value??
				if( it == time_map.end() )
				{
					// then set new value
					midiTime value_pos( pos_tact_64th );
		
					midiTime new_time =
						m_pattern->putValue( value_pos,
									level );

					// reset it so that it can be used for
					// ops (move, resize) after this
					// code-block
					it = time_map.begin();
					while( it != time_map.end() &&
							it.key() != new_time )
					{
						++it;
					}
				}

				// move it
				m_action = MOVE_VALUE;
				int aligned_x = (int)( (float)( (
						it.key() -
						m_currentPosition ) *
						m_ppt ) / 64.0f );
				m_moveXOffset = x - aligned_x - 1;
				// set move-cursor
				QCursor c( Qt::SizeAllCursor );
				QApplication::setOverrideCursor( c );

				eng()->getSongEditor()->setModified();
			}
			else if( ( _me->button() == Qt::RightButton &&
							m_editMode == DRAW ) ||
					m_editMode == ERASE )
			{
				// erase single value

				if( it != time_map.end() )
				{
					m_pattern->removeValue( it.key() );
					eng()->getSongEditor()->setModified();
				}
			}
			else if( _me->button() == Qt::LeftButton &&
							m_editMode == SELECT )
			{

				// select an area of values

				m_selectStartTact64th = pos_tact_64th;
				m_selectedTact64th = 0;
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
				m_moveStartTact64th = pos_tact_64th;
				m_moveStartLevel = level;

				m_action = MOVE_SELECTION;

				eng()->getSongEditor()->setModified();
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




void automationEditor::mouseReleaseEvent( QMouseEvent * _me )
{
	m_action = NONE;

	if( m_editMode == DRAW )
	{
		QApplication::restoreOverrideCursor();
	}
}




void automationEditor::mouseMoveEvent( QMouseEvent * _me )
{
	if( validPattern() == FALSE )
	{
		update();
		return;
	}

	if( _me->y() > TOP_MARGIN )
	{
		int level = getLevel( _me->y() );
		int x = _me->x();

		if( _me->x() <= VALUES_WIDTH )
		{
			update();
			return;
		}
		x -= VALUES_WIDTH;

		if(
#ifdef QT4
			_me->buttons() &
#else
			_me->state() ==
#endif
			Qt::LeftButton && m_editMode == DRAW )
		{
			if( m_action == MOVE_VALUE )
			{
				x -= m_moveXOffset;
			}
			int pos_tact_64th = x * 64 / m_ppt +
							m_currentPosition;
			if( m_action == MOVE_VALUE )
			{
				// moving value
				if( pos_tact_64th < 0 )
				{
					pos_tact_64th = 0;
				}

				// we moved the value so the value has to be
				// moved properly according to new starting-
				// time in the time map of pattern
				m_pattern->removeValue(
						midiTime( pos_tact_64th ) );
				m_pattern->putValue( midiTime( pos_tact_64th ),
								level );
			}

			eng()->getSongEditor()->setModified();

		}
		else if(
#ifdef QT4
			_me->buttons() &
#else
			_me->state() ==
#endif
					Qt::NoButton && m_editMode == DRAW )
		{
			// set move- or resize-cursor

			// get tact-64th in which the cursor is posated
			int pos_tact_64th = ( x * 64 ) / m_ppt +
							m_currentPosition;

			// get time map of current pattern
			timeMap & time_map = m_pattern->getTimeMap();

			// will be our iterator in the following loop
			timeMap::iterator it = time_map.begin();

			// loop through whole time map...
			while( it != time_map.end() )
			{
				// and check whether the cursor is over an
				// existing value
				if( pos_tact_64th >= it.key() &&
			    		pos_tact_64th <= it.key() +
							//TODO: Add constant
							4 &&
					it.data() == level )
				{
					break;
				}
				++it;
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
		else if(
#ifdef QT4
			_me->buttons() &
#else
			_me->modifiers() ==
#endif
					Qt::LeftButton &&
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

			// get tact-64th in which the cursor is posated
			int pos_tact_64th = x * 64 / m_ppt +
							m_currentPosition;

			m_selectedTact64th = pos_tact_64th -
							m_selectStartTact64th;
			if( (int) m_selectStartTact64th + m_selectedTact64th <
									0 )
			{
				m_selectedTact64th = -static_cast<int>(
							m_selectStartTact64th );
			}
			m_selectedLevels = level - m_selectStartLevel;
			if( level <= m_selectStartLevel )
			{
				--m_selectedLevels;
			}
		}
		else if(
#ifdef QT4
			_me->buttons() &
#else
			_me->modifiers() ==
#endif
				Qt::LeftButton &&
					m_editMode == MOVE &&
					m_action == MOVE_SELECTION )
		{
			// move selection + selected values

			// do horizontal move-stuff
			int pos_tact_64th = x * 64 / m_ppt +
							m_currentPosition;
			int tact_64th_diff = pos_tact_64th -
							m_moveStartTact64th;
			if( m_selectedTact64th > 0 )
			{
				if( (int) m_selectStartTact64th +
							tact_64th_diff < 0 )
				{
					tact_64th_diff = -m_selectStartTact64th;
				}
			}
			else
			{
				if( (int) m_selectStartTact64th +
					m_selectedTact64th + tact_64th_diff <
									0 )
				{
					tact_64th_diff = -(
							m_selectStartTact64th +
							m_selectedTact64th );
				}
			}
			m_selectStartTact64th += tact_64th_diff;

			int tact_diff = tact_64th_diff / 64;
			tact_64th_diff = tact_64th_diff % 64;


			// do vertical move-stuff
			int level_diff = level - m_moveStartLevel;

			if( m_selectedLevels > 0 )
			{
				if( m_selectStartLevel + level_diff
								< m_min_level )
				{
					level_diff = m_min_level -
							m_selectStartLevel;
				}
				else if( m_selectStartLevel + m_selectedLevels +
						level_diff > m_max_level )
				{
					level_diff = m_max_level -
							m_selectStartLevel -
							m_selectedLevels;
				}
			}
			else
			{
				if( m_selectStartLevel + m_selectedLevels +
						level_diff < m_min_level )
				{
					level_diff = m_min_level -
							m_selectStartLevel -
							m_selectedLevels;
				}
				else if( m_selectStartLevel + level_diff >
								m_max_level )
				{
					level_diff = m_max_level -
							m_selectStartLevel;
				}
			}
			m_selectStartLevel += level_diff;


			timeMap new_selValuesForMove;
			for( timeMap::iterator it = m_selValuesForMove.begin();
					it != m_selValuesForMove.end(); ++it )
			{
				int value_tact = it.key().getTact() + tact_diff;
				int value_tact_64th = it.key().getTact64th() +
								tact_64th_diff;
				while( value_tact_64th < 0 )
				{
					--value_tact;
					value_tact_64th += 64;
				}
				while( value_tact_64th > 64 )
				{
					++value_tact;
					value_tact_64th -= 64;
				}
				m_pattern->removeValue( it.key() );
				midiTime new_value_pos( value_tact,
							value_tact_64th );
				new_selValuesForMove[
					m_pattern->putValue( new_value_pos,
						it.data() + level_diff, FALSE )]
						= it.data() + level_diff;
			}
			m_selValuesForMove = new_selValuesForMove;

			m_moveStartTact64th = pos_tact_64th;
			m_moveStartLevel = level;
		}
	}
	else
	{
		if(
#ifdef QT4
			_me->buttons() &
#else
			_me->modifiers() ==
#endif
				Qt::LeftButton &&
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

			// get tact-64th in which the cursor is posated
			int pos_tact_64th = x * 64 / m_ppt +
							m_currentPosition;

			m_selectedTact64th = pos_tact_64th -
							m_selectStartTact64th;
			if( (int) m_selectStartTact64th + m_selectedTact64th <
									0 )
			{
				m_selectedTact64th = -static_cast<int>(
							m_selectStartTact64th );
			}

			int level = getLevel( _me->y() );

			if( level <= m_bottom_level )
			{
				QCursor::setPos( mapToGlobal( QPoint( _me->x(),
							height() -
							SCROLLBAR_SIZE ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
				level = m_bottom_level;
			}
			else if( level >= m_top_level )
			{
				QCursor::setPos( mapToGlobal( QPoint( _me->x(),
							TOP_MARGIN ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() - 1 );
				level = m_top_level;
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




void automationEditor::paintEvent( QPaintEvent * )
{
	updatePaintPixmap();
#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( size() );
	draw_pm.fill( QColor( 0, 0, 0 ) );

	QPainter p( &draw_pm, this );
#endif
	p.drawPixmap( 0, 0, m_paintPixmap );

	if( m_cursorInside == TRUE )
	{
		p.setClipRect( VALUES_WIDTH, TOP_MARGIN, width() - VALUES_WIDTH,
				height() - TOP_MARGIN - SCROLLBAR_SIZE );
		if( validPattern() == TRUE )
		{
//TODO: What's this?
			p.fillRect( 10, height() + 3 - SCROLLBAR_SIZE,
					width() - 10, DEFAULT_Y_DELTA - 7,
							QColor( 64, 64, 64 ) );
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

#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




// responsible for moving/resizing scrollbars after window-resizing
void automationEditor::resizeEvent( QResizeEvent * )
{
	m_leftRightScroll->setGeometry( VALUES_WIDTH, height() - SCROLLBAR_SIZE,
							width() - VALUES_WIDTH,
							SCROLLBAR_SIZE );

	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;
	m_topBottomScroll->setGeometry( width() - SCROLLBAR_SIZE, TOP_MARGIN,
						SCROLLBAR_SIZE, grid_height );

	int half_grid = grid_height / 2;
	int total_pixels = ( m_max_level - m_min_level ) * m_y_delta + 1;
	if( !m_y_auto && grid_height < total_pixels )
	{
		int min_scroll = m_min_level + (int)floorf( half_grid
							/ (float)m_y_delta );
		int max_scroll = m_max_level - (int)floorf( ( grid_height
					- half_grid ) / (float)m_y_delta );
		m_topBottomScroll->setRange( min_scroll, max_scroll );
	}
	else
	{
		m_topBottomScroll->setRange( m_scroll_level, m_scroll_level );
	}

#ifdef QT4
	m_topBottomScroll->setSingleStep( 1 );
	m_topBottomScroll->setPageStep( 20 );
#else
	m_topBottomScroll->setSteps( 1, 20 );
#endif

	m_topBottomScroll->setValue( m_scroll_level );

	if( eng()->getSongEditor() )
	{
		eng()->getSongEditor()->getPlayPos(
					songEditor::PLAY_AUTOMATION_PATTERN
					).m_timeLine->setFixedWidth( width() );
	}
	m_toolBar->setFixedWidth( width() );

	updateTopBottomLevels();
	update();
}




void automationEditor::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	if( eng()->getMainWindow()->isCtrlPressed() == TRUE )
	{
		if( _we->delta() > 0 )
		{
			m_ppt = tMin( m_ppt * 2, m_y_delta *
						DEFAULT_STEPS_PER_TACT * 8 );
		}
		else if( m_ppt >= 72 )
		{
			m_ppt /= 2;
		}
		// update combobox with zooming-factor
		m_zoomingXComboBox->setValue(
				m_zoomingXComboBox->findText( QString::number(
					static_cast<int>( m_ppt * 100 /
						DEFAULT_PPT ) ) +"%" ) );
		// update timeline
		m_timeLine->setPixelsPerTact( m_ppt );
		update();
	}
	else if( eng()->getMainWindow()->isShiftPressed() )
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




int automationEditor::getLevel( int _y )
{
	int level_line_y = height() - SCROLLBAR_SIZE - 1;
	// pressed level
	int level = m_bottom_level + (int)roundf( m_y_auto ?
			( m_max_level - m_min_level ) * ( level_line_y - _y )
					/ (float)( level_line_y - TOP_MARGIN ) :
			( level_line_y - _y ) / (float)m_y_delta );

	// some range-checking-stuff
	if( level < m_bottom_level )
	{
		level = m_bottom_level;
	}
	else if( level > m_top_level )
	{
		level = m_top_level;
	}

	return( level );
}




void automationEditor::play( void )
{
	if( validPattern() == FALSE )
	{
		return;
	}

	if( !m_pattern->getTrack() )
	{
		if( eng()->getSongEditor()->playMode() !=
						songEditor::PLAY_PATTERN )
		{
			eng()->getSongEditor()->stop();
			eng()->getSongEditor()->playPattern( (pattern *)
				eng()->getPianoRoll()->currentPattern() );
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else if( eng()->getSongEditor()->playing() )
		{
			eng()->getSongEditor()->pause();
			m_playButton->setIcon( embed::getIconPixmap( "play" ) );
		}
		else if( eng()->getSongEditor()->paused() )
		{
			eng()->getSongEditor()->resumeFromPause();
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else
		{
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
			eng()->getSongEditor()->playPattern( (pattern *)
				eng()->getPianoRoll()->currentPattern() );
		}
	}
	else if( inBBEditor() )
	{
		if( eng()->getSongEditor()->playing() )
		{
			m_playButton->setIcon( embed::getIconPixmap( "play" ) );
		}
		else
		{
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		eng()->getBBEditor()->play();
	}
	else
	{
		if( eng()->getSongEditor()->playing() )
		{
			eng()->getSongEditor()->pause();
			m_playButton->setIcon( embed::getIconPixmap( "play" ) );
		}
		else if( eng()->getSongEditor()->paused() )
		{
			eng()->getSongEditor()->resumeFromPause();
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else
		{
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
			eng()->getSongEditor()->play();
		}
	}
}




void automationEditor::stop( void )
{
	if( m_pattern->getTrack() && inBBEditor() )
	{
		eng()->getBBEditor()->stop();
	}
	else
	{
		eng()->getSongEditor()->stop();
	}
	m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	m_playButton->update();
	m_scrollBack = TRUE;
}




void automationEditor::horScrolled( int _new_pos )
{
	m_currentPosition = _new_pos;
	emit positionChanged( m_currentPosition );
	update();
}




void automationEditor::verScrolled( int _new_pos )
{
	m_scroll_level = _new_pos;
	updateTopBottomLevels();
	update();
}




void automationEditor::drawButtonToggled( void )
{
	m_editMode = DRAW;
	removeSelection();
	update();
}




void automationEditor::eraseButtonToggled( void )
{
	m_editMode = ERASE;
	removeSelection();
	update();
}




void automationEditor::selectButtonToggled( void )
{
	m_editMode = SELECT;
	removeSelection();
	update();
}




void automationEditor::moveButtonToggled( void )
{
	m_editMode = MOVE;
	m_selValuesForMove.clear();
	getSelectedValues( m_selValuesForMove );
	update();
}




void automationEditor::selectAll( void )
{
	if( validPattern() == FALSE )
	{
		return;
	}

	timeMap & time_map = m_pattern->getTimeMap();

	// if first_time = TRUE, we HAVE to set the vars for select
	bool first_time = TRUE;

	for( timeMap::iterator it = time_map.begin(); it != time_map.end();
									++it )
	{
		//TODO: Add constant
		Uint32 len_tact_64th = 4;

		const int level = it.data();

		Uint32 pos_tact_64th = it.key();
		if( level <= m_selectStartLevel || first_time )
		{
			// if we move start-level down, we have to add 
			// the difference between old and new start-level
			// to m_selectedLevels, otherwise the selection
			// is just moved down...
			int diff = m_selectStartLevel - ( level - 1 );
			m_selectStartLevel = level - 1;
			m_selectedLevels += diff;
		}
		if( level >= m_selectedLevels + m_selectStartLevel
								|| first_time )
		{
			m_selectedLevels = level - m_selectStartLevel;
		}
		if( pos_tact_64th < m_selectStartTact64th || first_time )
		{
			m_selectStartTact64th = pos_tact_64th;
		}
		if( pos_tact_64th + len_tact_64th >
				m_selectStartTact64th + m_selectedTact64th ||
								first_time )
		{
			m_selectedTact64th = pos_tact_64th + len_tact_64th -
							m_selectStartTact64th;
		}
		first_time = FALSE;
	}
}




// returns vector with pointers to all selected values
void automationEditor::getSelectedValues( timeMap & _selected_values )
{
	if( validPattern() == FALSE )
	{
		return;
	}

	int sel_pos_start = m_selectStartTact64th;
	int sel_pos_end = sel_pos_start + m_selectedTact64th;
	if( sel_pos_start > sel_pos_end )
	{
		qSwap<int>( sel_pos_start, sel_pos_end );
	}

	int sel_level_start = m_selectStartLevel;
	int sel_level_end = sel_level_start + m_selectedLevels;
	if( sel_level_start > sel_level_end )
	{
		qSwap<int>( sel_level_start, sel_level_end );
	}

	timeMap & time_map = m_pattern->getTimeMap();

	for( timeMap::iterator it = time_map.begin(); it != time_map.end();
									++it )
	{
		//TODO: Add constant
		Sint32 len_tact_64th = 4;

		int level = it.data();
		Sint32 pos_tact_64th = it.key();

		if( level > sel_level_start && level <= sel_level_end &&
				pos_tact_64th >= sel_pos_start &&
				pos_tact_64th+len_tact_64th <= sel_pos_end )
		{
			_selected_values[it.key()] = level;
		}
	}
}




void automationEditor::copySelectedValues( void )
{
	m_valuesToCopy.clear();

	timeMap selected_values;
	getSelectedValues( selected_values );

	if( !selected_values.isEmpty() )
	{
		midiTime start_pos( selected_values.begin().key().getTact(),
									0 );
		for( timeMap::iterator it = selected_values.begin();
			it != selected_values.end(); ++it )
		{
			m_valuesToCopy[it.key()] = it.data();
		}
		textFloat::displayMessage( tr( "Values copied" ),
				tr( "All selected values were copied to the "
								"clipboard." ),
				embed::getIconPixmap( "edit_copy" ), 2000 );
	}
}




void automationEditor::cutSelectedValues( void )
{
	if( validPattern() == FALSE )
	{
		return;
	}

	m_valuesToCopy.clear();

	timeMap selected_values;
	getSelectedValues( selected_values );

	if( !selected_values.isEmpty() )
	{
		eng()->getSongEditor()->setModified();

		midiTime start_pos( selected_values.begin().key().getTact(),
									0 );

		for( timeMap::iterator it = selected_values.begin();
					it != selected_values.end(); ++it )
		{
			m_valuesToCopy[it.key()] = it.data();
			m_pattern->removeValue( it.key() );
		}
	}

	update();
	eng()->getSongEditor()->update();
}




void automationEditor::pasteValues( void )
{
	if( validPattern() == FALSE )
	{
		return;
	}

	if( !m_valuesToCopy.isEmpty() )
	{
		for( timeMap::iterator it = m_valuesToCopy.begin();
					it != m_valuesToCopy.end(); ++it )
		{
			m_pattern->putValue( it.key() + m_currentPosition,
								it.data() );
		}

		// we only have to do the following lines if we pasted at
		// least one value...
		eng()->getSongEditor()->setModified();
		update();
		eng()->getSongEditor()->update();
	}
}




void automationEditor::deleteSelectedValues( void )
{
	if( validPattern() == FALSE )
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
		eng()->getSongEditor()->setModified();
		update();
		eng()->getSongEditor()->update();
	}
}




void automationEditor::updatePosition( const midiTime & _t )
{
	if( ( eng()->getSongEditor()->playing() &&
			eng()->getSongEditor()->playMode() ==
					songEditor::PLAY_AUTOMATION_PATTERN ) ||
							m_scrollBack == TRUE )
	{
		const int w = width() - VALUES_WIDTH;
		if( _t > m_currentPosition + w * 64 / m_ppt )
		{
			m_leftRightScroll->setValue( _t.getTact() * 64 );
		}
		else if( _t < m_currentPosition )
		{
			midiTime t = tMax( _t - w * 64 * 64 / m_ppt, 0 );
			m_leftRightScroll->setValue( t.getTact() * 64 );
		}
		m_scrollBack = FALSE;
	}
}




void automationEditor::zoomingXChanged( const QString & _zfac )
{
	m_ppt = _zfac.left( _zfac.length() - 1 ).toInt() * DEFAULT_PPT / 100;
#ifdef LMMS_DEBUG
	assert( m_ppt > 0 );
#endif
	m_timeLine->setPixelsPerTact( m_ppt );
	update();
}




void automationEditor::zoomingYChanged( const QString & _zfac )
{
	m_y_auto = _zfac == "Auto";
	if( !m_y_auto )
	{
		m_y_delta = _zfac.left( _zfac.length() - 1 ).toInt()
							* DEFAULT_Y_DELTA / 100;
	}
#ifdef LMMS_DEBUG
	assert( m_y_delta > 0 );
#endif
	resizeEvent( NULL );
}




int automationEditor::quantization( void ) const
{
	return( 64 / m_quantizeComboBox->currentText().right(
				m_quantizeComboBox->currentText().length() -
								2 ).toInt() );
}




void automationEditor::updateTopBottomLevels( void )
{
	if( m_y_auto )
	{
		m_bottom_level = m_min_level;
		m_top_level = m_max_level;
		return;
	}

	int total_pixels = ( m_max_level - m_min_level ) * m_y_delta + 1;
	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;
	int half_grid = grid_height / 2;

	if( total_pixels > grid_height )
	{
		int central_level = m_min_level + m_max_level - m_scroll_level;

		m_bottom_level = central_level - (int)roundf( half_grid
							/ (float)m_y_delta );
		if( m_bottom_level < m_min_level )
		{
			m_bottom_level = m_min_level;
			m_top_level = m_min_level + (int)floorf( grid_height
							/ (float)m_y_delta );
		}
		else
		{
			m_top_level = m_bottom_level + (int)floorf( grid_height
							/ (float)m_y_delta );
			if( m_top_level > m_max_level )
			{
				m_top_level = m_max_level;
				m_bottom_level = m_max_level - (int)floorf(
					grid_height / (float)m_y_delta );
			}
		}
	}
	else
	{
		m_bottom_level = m_min_level;
		m_top_level = m_max_level;
	}
}




inline bool automationEditor::xVisible( int _x )
{
	return( _x >= 0 && _x <= width() - VALUES_WIDTH );
}




inline bool automationEditor::inBBEditor( void )
{
	return( m_pattern->getTrack()->getTrackContainer()
						== eng()->getBBEditor() );
}




#include "automation_editor.moc"


#ifdef QT3
#undef addButton
#undef setCheckable
#endif


#endif
