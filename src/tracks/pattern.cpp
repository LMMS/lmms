#ifndef SINGLE_SOURCE_COMPILE

/*
 * pattern.cpp - implementation of class pattern which holds notes
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2005 Danny McRae <khjklujn/at/yahoo.com>
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
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QMessageBox>
#include <QImage>
#include <QMouseEvent>
#include <QTimer>

#else

#include <qdom.h>
#include <qpopupmenu.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qimage.h>
#include <qtimer.h>

#define addSeparator insertSeparator
#define addMenu insertItem

#endif


#include "pattern.h"
#include "channel_track.h"
#include "templates.h"
#include "gui_templates.h"
#include "embed.h"
#include "piano_roll.h"
#include "track_container.h"
#include "rename_dialog.h"
#include "sample_buffer.h"
#include "audio_sample_recorder.h"
#include "song_editor.h"
#include "tooltip.h"
#include "bb_editor.h"
#include "string_pair_drag.h"
#include "buffer_allocator.h"
#include "main_window.h"


QPixmap * pattern::s_stepBtnOn = NULL;
QPixmap * pattern::s_stepBtnOverlay = NULL;
QPixmap * pattern::s_stepBtnOff = NULL;
QPixmap * pattern::s_stepBtnOffLight = NULL;
QPixmap * pattern::s_frozen = NULL;



pattern::pattern ( channelTrack * _channel_track ) :
	trackContentObject( _channel_track ),
	m_paintPixmap(),
	m_needsUpdate( TRUE ),
	m_channelTrack( _channel_track ),
	m_patternType( BEAT_PATTERN ),
	m_name( _channel_track->name() ),
	m_steps( DEFAULT_STEPS_PER_TACT ),
	m_frozenPatternMutex(),
	m_frozenPattern( NULL ),
	m_freezing( FALSE ),
	m_freezeAborted( FALSE )
{
	init();
}




pattern::pattern( const pattern & _pat_to_copy ) :
	trackContentObject( _pat_to_copy.m_channelTrack ),
	m_paintPixmap(),
	m_needsUpdate( TRUE ),
	m_channelTrack( _pat_to_copy.m_channelTrack ),
	m_patternType( _pat_to_copy.m_patternType ),
	m_name( "" ),
	m_steps( _pat_to_copy.m_steps ),
	m_frozenPatternMutex(),
	m_frozenPattern( NULL ),
	m_freezeAborted( FALSE )
{
	for( noteVector::const_iterator it = _pat_to_copy.m_notes.begin();
					it != _pat_to_copy.m_notes.end(); ++it )
	{
		m_notes.push_back( new note( **it ) );
	}

	init();
}




pattern::~pattern()
{
	if( eng()->getPianoRoll()->currentPattern() == this )
	{
		eng()->getPianoRoll()->setCurrentPattern( NULL );
		// we have to have the song-editor to stop playing if it played
		// us before
		if( eng()->getSongEditor()->playing() &&
			eng()->getSongEditor()->playMode() ==
							songEditor::PLAY_PATTERN )
		{
			eng()->getSongEditor()->playPattern( NULL );
		}
	}

	for( noteVector::iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		delete *it;
	}

	m_notes.clear();

	m_frozenPatternMutex.lock();
	delete m_frozenPattern;
	m_frozenPatternMutex.unlock();
}




void pattern::init( void )
{
	if( s_stepBtnOn == NULL )
	{
		s_stepBtnOn = new QPixmap( embed::getIconPixmap(
							"step_btn_on_100" ) );
	}

	if( s_stepBtnOverlay == NULL )
	{
		s_stepBtnOverlay = new QPixmap( embed::getIconPixmap(
						"step_btn_on_yellow" ) );
	}

	if( s_stepBtnOff == NULL )
	{
		s_stepBtnOff = new QPixmap( embed::getIconPixmap(
							"step_btn_off" ) );
	}

	if( s_stepBtnOffLight == NULL )
	{
		s_stepBtnOffLight = new QPixmap( embed::getIconPixmap(
						"step_btn_off_light" ) );
	}

	if( s_frozen == NULL )
	{
		s_frozen = new QPixmap( embed::getIconPixmap( "frozen" ) );
	}

	ensureBeatNotes();

	changeLength( length() );

#ifndef QT4
	// set background-mode for flicker-free redraw
	setBackgroundMode( Qt::NoBackground );
#endif

	setFixedHeight( parentWidget()->height() - 2 );
	setAutoResizeEnabled( FALSE );

	toolTip::add( this,
		tr( "double-click to open this pattern in piano-roll\n"
			"use mouse wheel to set volume of a step" ) );
}




midiTime pattern::length( void ) const
{
	if( m_patternType == BEAT_PATTERN )
	{
		if( m_steps % DEFAULT_STEPS_PER_TACT == 0 )
		{
			return( m_steps * BEATS_PER_TACT );
		}
		return( ( m_steps / DEFAULT_STEPS_PER_TACT + 1 ) *
				DEFAULT_STEPS_PER_TACT * BEATS_PER_TACT );
	}

	Sint32 max_length = 0;

	for( noteVector::const_iterator it = m_notes.begin();
							it != m_notes.end();
									++it )
	{
		max_length = tMax<Sint32>( max_length, ( *it )->endPos() );
	}
	if( max_length % 64 == 0 )
	{
		return( midiTime( tMax<Sint32>( max_length, 64 ) ) );
	}
	return( midiTime( tMax( midiTime( max_length ).getTact() + 1, 1 ),
									0 ) );
}




note * pattern::addNote( const note & _new_note )
{
	note * new_note = new note( _new_note );
	new_note->quantizePos( eng()->getPianoRoll()->quantization() );

	if( m_notes.size() == 0 || m_notes.back()->pos() <= new_note->pos() )
	{
		m_notes.push_back( new_note );
	}
	else
	{
		// simple algorithm for inserting the note between two 
		// notes with smaller and greater position
		// maybe it could be optimized by starting in the middle and 
		// going forward or backward but note-inserting isn't that
		// time-critical since it is usually not done while playing...
		long new_note_abs_time = new_note->pos();
		noteVector::iterator it = m_notes.begin();

		while( it != m_notes.end() &&
					( *it )->pos() < new_note_abs_time )
		{
			++it;
		}

		m_notes.insert( it, new_note );
	}

	checkType();
	update();
	changeLength( length() );

	updateBBTrack();

	return( new_note );
}




void pattern::removeNote( const note * _note_to_del )
{
	noteVector::iterator it = m_notes.begin();
	while( it != m_notes.end() )
	{
		if( *it == _note_to_del )
		{
			delete *it;
			m_notes.erase( it );
			break;
		}
		++it;
	}

	checkType();
	update();
	changeLength( length() );

	updateBBTrack();
}




note * pattern::rearrangeNote( const note * _note_to_proc )
{
	// just rearrange the position of the note by removing it and adding 
	// a copy of it -> addNote inserts it at the correct position
	note copy_of_note( *_note_to_proc );
	removeNote( _note_to_proc );

	return( addNote( copy_of_note ) );
}




void pattern::clearNotes( void )
{
	for( noteVector::iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		delete *it;
	}

	m_notes.clear();
	checkType();
	update();
	if( eng()->getPianoRoll()->currentPattern() == this )
	{
		eng()->getPianoRoll()->update();
	}
}




note * pattern::noteAt( int _note_num )
{
	if( (csize) _note_num < m_notes.size() )
	{
		return( m_notes[_note_num] );
	}
	return( NULL );
}




void pattern::setNoteAt( int _note_num, note _new_note )
{
	if( static_cast<csize>( _note_num ) < m_notes.size() )
	{
		delete m_notes[_note_num];
		m_notes[_note_num] = new note( _new_note );
		checkType();
		update();
	}
}




void pattern::setType( patternTypes _new_pattern_type )
{
	if( _new_pattern_type == BEAT_PATTERN ||
				_new_pattern_type == MELODY_PATTERN )
	{
		m_patternType = _new_pattern_type;
	}
}




void pattern::checkType( void )
{
	noteVector::iterator it = m_notes.begin();
	while( it != m_notes.end() )
	{
		if( ( *it )->length() > 0 )
		{
			setType( pattern::MELODY_PATTERN );
			return;
		}
		++it;
	}
	setType( pattern::BEAT_PATTERN );
}




void pattern::playFrozenData( sampleFrame * _ab, const f_cnt_t _start_frame,
							const fpab_t _frames )
{
	m_frozenPatternMutex.lock();
	if( m_frozenPattern != NULL )
	{
		m_frozenPattern->play( _ab, _start_frame, _frames );
	}
	m_frozenPatternMutex.unlock();
}




void pattern::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	QDomElement pattern_de = _doc.createElement( nodeName() );
	pattern_de.setAttribute( "type", m_patternType );
	pattern_de.setAttribute( "name", m_name );
	// as the target of copied/dragged pattern is always an existing
	// pattern, we must not store actual position, instead we store -1
	// which tells loadSettings() not to mess around with position
	if( _parent.nodeName() == "clipboard" ||
					_parent.nodeName() == "dnddata" )
	{
		pattern_de.setAttribute( "pos", -1 );
	}
	else
	{
		pattern_de.setAttribute( "pos", startPosition() );
	}
	pattern_de.setAttribute( "len", length() );
	pattern_de.setAttribute( "steps", m_steps );
	pattern_de.setAttribute( "frozen", m_frozenPattern != NULL );
	_parent.appendChild( pattern_de );

	// now save settings of all notes
	for( noteVector::iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		if( ( *it )->length() )
		{
			( *it )->saveSettings( _doc, pattern_de );
		}
	}
}




void pattern::loadSettings( const QDomElement & _this )
{
	unfreeze();

	m_patternType = static_cast<patternTypes>( _this.attribute( "type"
								).toInt() );
	m_name = _this.attribute( "name" );
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	changeLength( midiTime( _this.attribute( "len" ).toInt() ) );

	clearNotes();

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			note * n = new note();
			n->loadSettings( node.toElement() );
			m_notes.push_back( n );
		}
		node = node.nextSibling();
        }

	m_steps = _this.attribute( "steps" ).toInt();
	if( m_steps == 0 )
	{
		m_steps = DEFAULT_STEPS_PER_TACT;
	}

	ensureBeatNotes();
/*	if( _this.attribute( "frozen" ).toInt() )
	{
		freeze();
	}*/
	update();
	updateBBTrack();
}



void pattern::update( void )
{
	m_needsUpdate = TRUE;
	changeLength( length() );
	trackContentObject::update();
}




void pattern::openInPianoRoll( void )
{
	openInPianoRoll( FALSE );
}




void pattern::openInPianoRoll( bool )
{
	eng()->getPianoRoll()->setCurrentPattern( this );
	eng()->getPianoRoll()->show();
	eng()->getPianoRoll()->setFocus();
}




void pattern::clear( void )
{
	clearNotes();
	ensureBeatNotes();
}




void pattern::resetName( void )
{
	m_name = m_channelTrack->name();
}




void pattern::changeName( void )
{
	renameDialog rename_dlg( m_name );
	rename_dlg.exec();
}




void pattern::freeze( void )
{
	if( eng()->getSongEditor()->playing() )
	{
		QMessageBox::information( 0, tr( "Cannot freeze pattern" ),
						tr( "The pattern currently "
							"cannot be freezed "
							"because you're in "
							"play-mode. Please "
							"stop and try again!" ),
						QMessageBox::Ok );
		return;
	}
	if( m_channelTrack->muted() )
	{
		if( QMessageBox::
#if QT_VERSION >= 0x030200		
				 question
#else
				 information
#endif				 
		
					    ( 0, tr( "Channel muted" ),
						tr( "The channel this pattern "
							"belongs to is "
							"currently muted "
							"therefore "
							"freezing makes no "
							"sense! Do you still "
							"want to continue?" ),
						QMessageBox::Yes,
						QMessageBox::No |
						QMessageBox::Default |
						QMessageBox::Escape ) ==
			QMessageBox::No )
		{
			return;
		}
	}

	// already frozen?
	if( m_frozenPattern != NULL )
	{
		// then unfreeze, before freezing it again
		unfreeze();
	}

	new patternFreezeThread( this );

}




void pattern::unfreeze( void )
{
	if( m_frozenPattern != NULL )
	{
		m_frozenPatternMutex.lock();
		delete m_frozenPattern;
		m_frozenPattern = NULL;
		m_frozenPatternMutex.unlock();
		update();
	}
}




void pattern::abortFreeze( void )
{
	m_freezeAborted = TRUE;
}




#ifdef QT4

void pattern::addSteps( QAction * _item )
{
	addSteps( _item->text().toInt() );
}




void pattern::removeSteps( QAction * _item )
{
	removeSteps( _item->text().toInt() );
}



#else

void pattern::addSteps( QAction * ) { }
void pattern::removeSteps( QAction * ) { }

#endif



void pattern::addSteps( int _n )
{
	m_steps += _n;
	ensureBeatNotes();
	update();
}




void pattern::removeSteps( int _n )
{
	if( _n < m_steps )
	{
		for( int i = m_steps - _n; i < m_steps; ++i )
		{
			for( noteVector::iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
			{
				if( ( *it )->pos() == i * BEATS_PER_TACT &&
							( *it )->length() <= 0 )
				{
					removeNote( *it );
					break;
				}
			}
		}
		m_steps -= _n;
		update();
	}
}




void pattern::constructContextMenu( QMenu * _cm )
{
#ifdef QT4
	QAction * a = new QAction( embed::getIconPixmap( "piano" ),
					tr( "Open in piano-roll" ), _cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ), this,
					SLOT( openInPianoRoll( bool ) ) );
#else
	_cm->insertItem( embed::getIconPixmap( "piano" ),
					tr( "Open in piano-roll" ),
					this, SLOT( openInPianoRoll() ),
								0, -1, 0 );
#endif
#ifdef QT4
	_cm->insertSeparator( _cm->actions()[1] );
#else
	_cm->insertSeparator( 1 );
#endif

	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "edit_erase" ),
			tr( "Clear all notes" ), this, SLOT( clear() ) );
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT( resetName() ) );
	_cm->addAction( embed::getIconPixmap( "rename" ), tr( "Change name" ),
						this, SLOT( changeName() ) );
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "freeze" ),
		( m_frozenPattern != NULL )? tr( "Refreeze" ) : tr( "Freeze" ),
						this, SLOT( freeze() ) );
	_cm->addAction( embed::getIconPixmap( "unfreeze" ), tr( "Unfreeze" ),
						this, SLOT( unfreeze() ) );

	_cm->addSeparator();

#ifdef QT4
	QMenu * add_step_menu = _cm->addMenu(
					embed::getIconPixmap( "step_btn_add" ),
							tr( "Add steps" ) );
	QMenu * remove_step_menu = _cm->addMenu(
				embed::getIconPixmap( "step_btn_remove" ),
							tr( "Remove steps" ) );
	connect( add_step_menu, SIGNAL( triggered( QAction * ) ),
			this, SLOT( addSteps( QAction * ) ) );
	connect( remove_step_menu, SIGNAL( triggered( QAction * ) ),
			this, SLOT( removeSteps( QAction * ) ) );
#else
	QMenu * add_step_menu = new QMenu( this );
	QMenu * remove_step_menu = new QMenu( this );
#endif
	for( int i = 1; i <= 16; i *= 2 )
	{
		const QString label = ( i == 1 ) ?
					tr( "1 step" ) :
					tr( "%1 steps" ).arg( i );
#ifdef QT4
		add_step_menu->addAction( label );
		remove_step_menu->addAction( label );
#else
		int menu_id = add_step_menu->addAction( label, this,
						SLOT( addSteps( int ) ) );
		add_step_menu->setItemParameter( menu_id, i );
		menu_id = remove_step_menu->addAction( label, this,
						SLOT( removeSteps( int ) ) );
		remove_step_menu->setItemParameter( menu_id, i );
#endif
	}
#ifndef QT4
	_cm->addMenu( embed::getIconPixmap( "step_btn_add" ),
					tr( "Add steps" ), add_step_menu );
	_cm->addMenu( embed::getIconPixmap( "step_btn_remove" ),
				tr( "Remove steps" ), remove_step_menu );
#endif
}




void pattern::mouseDoubleClickEvent( QMouseEvent * _me )
{
	if( _me->button() != Qt::LeftButton )
	{
		_me->ignore();
		return;
	}
	if( m_patternType == pattern::MELODY_PATTERN ||
		!( m_patternType == pattern::BEAT_PATTERN &&
		( pixelsPerTact() >= 192 ||
		  			m_steps != DEFAULT_STEPS_PER_TACT ) &&
		_me->y() > height() - s_stepBtnOff->height() ) )
	{
		openInPianoRoll();
	} 
}




void pattern::mousePressEvent( QMouseEvent * _me )
{
/*	if( _me->button() != Qt::LeftButton )
	{
	return;
}*/

	if( _me->button() == Qt::LeftButton &&
		   m_patternType == pattern::BEAT_PATTERN &&
		   ( pixelsPerTact() >= 192 ||
		   m_steps != DEFAULT_STEPS_PER_TACT ) &&
		   _me->y() > height() - s_stepBtnOff->height() )
	{
		int step = ( _me->x() - TCO_BORDER_WIDTH ) *
				length() / BEATS_PER_TACT / width();
		if( step >= m_steps )
		{
			return;
		}
		note * n = m_notes[step];
		if( n->length() < 0 )
		{
			n->setLength( 0 );
		}
		else
		{
			n->setLength( -64 );
		}
		eng()->getSongEditor()->setModified();
		update();
		if( eng()->getPianoRoll()->currentPattern() == this )
		{
			eng()->getPianoRoll()->update();
		}
	}
	else if( m_frozenPattern != NULL && _me->button() == Qt::LeftButton &&
			eng()->getMainWindow()->isShiftPressed() == TRUE )
	{
		QString s;
		new stringPairDrag( "sampledata",
					m_frozenPattern->toBase64( s ),
					    embed::getIconPixmap( "freeze" ),
					    this, eng() );
	}
	else
	{
		trackContentObject::mousePressEvent( _me );
	}
}




void pattern::wheelEvent( QWheelEvent * _we )
{
	if( m_patternType == pattern::BEAT_PATTERN &&
		   ( pixelsPerTact() >= 192 ||
		   m_steps != DEFAULT_STEPS_PER_TACT ) &&
		   _we->y() > height() - s_stepBtnOff->height() )
	{
		int step = ( _we->x() - TCO_BORDER_WIDTH ) *
				length() / BEATS_PER_TACT / width();
		if( step >= m_steps )
		{
			return;
		}
		note * n = m_notes[step];
		Uint8 vol = n->getVolume();
		
		if( n->length() == 0 && _we->delta() > 0 )
		{
			n->setLength( -64 );
			n->setVolume( 5 );
		}
		else if( _we->delta() > 0 )
		{
			if( vol < 95 )
			{
				n->setVolume( vol + 5 );
			}
		}
		else
		{
			if( vol > 5 )
			{
				n->setVolume( vol - 5 );
			}
			else
			{
				n->setLength( 0 );
			}
		}
		eng()->getSongEditor()->setModified();
		update();
		if( eng()->getPianoRoll()->currentPattern() == this )
		{
			eng()->getPianoRoll()->update();
		}
		_we->accept();
	}
	else
	{
		trackContentObject::wheelEvent( _we );
	}
}




void pattern::paintEvent( QPaintEvent * )
{
	if( m_needsUpdate == FALSE )
	{
		QPainter p( this );
		p.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	changeLength( length() );

	m_needsUpdate = FALSE;

	if( m_paintPixmap.isNull() == TRUE || m_paintPixmap.size() != size() )
	{
		m_paintPixmap = QPixmap( size() );
	}

	QPainter p( &m_paintPixmap );
#ifdef QT4
	// TODO: gradient!
#else
	for( int y = 1; y < height() / 2; ++y )
	{
		const int gray = 96 - y * 192 / height();
		if( isSelected() == TRUE )
		{
			p.setPen( QColor( 0, 0, 128 + gray ) );
		}
		else
		{
			p.setPen( QColor( gray, gray, gray ) );
		}
		p.drawLine( 1, y, width() - 1, y );
	}
	for( int y = height() / 2; y < height() - 1; ++y )
	{
		const int gray = ( y - height() / 2 ) * 192 / height();
		if( isSelected() == TRUE )
		{
			p.setPen( QColor( 0, 0, 128 + gray ) );
		}
		else
		{
			p.setPen( QColor( gray, gray, gray ) );
		}
		p.drawLine( 1, y, width() - 1, y );
	}
#endif

	p.setPen( QColor( 57, 69, 74 ) );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, 0, 0, height() );
	p.setPen( QColor( 120, 130, 140 ) );
	p.drawLine( 0, height() - 1, width() - 1, height() - 1 );
	p.drawLine( width() - 1, 0, width() - 1, height() - 1 );

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawRect( 1, 1, width() - 2, height() - 2 );

	const float ppt = pixelsPerTact();

	if( m_patternType == pattern::MELODY_PATTERN )
	{
		Sint32 central_key = 0;
		if( m_notes.size() > 0 )
		{
			// first determine the central tone so that we can 
			// display the area where most of the m_notes are
			Sint32 total_notes = 0;
			for( noteVector::iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
			{
				if( ( *it )->length() > 0 )
				{
					central_key += ( *it )->key();
					++total_notes;
				}
			}

			if( total_notes > 0 )
			{
				central_key = central_key / total_notes;

				Sint16 central_y = height() / 2;
				Sint16 y_base = central_y + TCO_BORDER_WIDTH -1;

				const Sint16 x_base = TCO_BORDER_WIDTH;

				p.setPen( QColor( 0, 0, 0 ) );
				for( tact tact_num = 1; tact_num <
						length().getTact(); ++tact_num )
				{
					p.drawLine(
						x_base + static_cast<int>(
							ppt * tact_num ) - 1,
						TCO_BORDER_WIDTH,
						x_base + static_cast<int>(
							ppt * tact_num ) - 1,
						height() - 2 *
							TCO_BORDER_WIDTH );
				}
				if( getTrack()->muted() )
				{
					p.setPen( QColor( 160, 160, 160 ) );
				}
				else if( m_frozenPattern != NULL )
				{
					p.setPen( QColor( 0x00, 0xE0, 0xFF ) );
				}
				else
				{
					p.setPen( QColor( 0xFF, 0xB0, 0x00 ) );
				}

				for( noteVector::iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
				{
					Sint8 y_pos = central_key -
								( *it )->key();

					if( ( *it )->length() > 0 &&
							y_pos > -central_y &&
							y_pos < central_y )
					{
						Sint16 x1 = 2 * x_base +
		static_cast<int>( ( *it )->pos() * ppt / 64 );
						Sint16 x2 = x1 +
			static_cast<int>( ( *it )->length() * ppt / 64 );
						p.drawLine( x1, y_base + y_pos,
							x2, y_base + y_pos );

					}
				}
			}
		}
	}
	else if( m_patternType == pattern::BEAT_PATTERN &&
			( ppt >= 96 || m_steps != DEFAULT_STEPS_PER_TACT ) )
	{
		QPixmap stepon;
		QPixmap stepoverlay;
		QPixmap stepoff;
		QPixmap stepoffl;
		const int steps = length() / BEATS_PER_TACT;
		const int w = width() - 2 * TCO_BORDER_WIDTH;
#ifdef QT4
		stepon = s_stepBtnOn->scaled( w / steps,
					      s_stepBtnOn->height(),
					      Qt::IgnoreAspectRatio,
					      Qt::SmoothTransformation );
		stepoverlay = s_stepBtnOverlay->scaled( w / steps,
					      s_stepBtnOn->height(),
					      Qt::IgnoreAspectRatio,
					      Qt::SmoothTransformation );
		stepoff = s_stepBtnOff->scaled( w / steps,
						s_stepBtnOff->height(),
						Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation );
		stepoffl = s_stepBtnOffLight->scaled( w / steps,
						s_stepBtnOffLight->height(),
						Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation );
#else
		stepon.convertFromImage( 
				s_stepBtnOn->convertToImage().scale(
					w / steps, s_stepBtnOn->height() ) );
		stepoverlay.convertFromImage( 
				s_stepBtnOverlay->convertToImage().scale(
				w / steps, s_stepBtnOverlay->height() ) );
		stepoff.convertFromImage( s_stepBtnOff->convertToImage().scale(
					w / steps, s_stepBtnOff->height() ) );
		stepoffl.convertFromImage( s_stepBtnOffLight->convertToImage().
					scale( w / steps,
						s_stepBtnOffLight->height() ) );
#endif
		for( noteVector::iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
		{
			Sint16 no = it - m_notes.begin();
			Sint16 x = TCO_BORDER_WIDTH + static_cast<int>( no *
								w / steps );
			Sint16 y = height() - s_stepBtnOff->height() - 1;
			
			Uint8 vol = ( *it )->getVolume();
			
			if( ( *it )->length() < 0 )
			{
				p.drawPixmap( x, y, stepoff );
				for( int i = 0; i < vol / 5 + 1; ++i )
				{
					p.drawPixmap( x, y, stepon );
				}
				for( int i = 0; i < ( 25 + ( vol - 75 ) ) / 5;
									++i )
				{
					p.drawPixmap( x, y, stepoverlay );
				}
			}
			else if( ( no / BEATS_PER_TACT ) % 2 )
			{
				p.drawPixmap( x, y, stepoff );
			}
			else
			{
				p.drawPixmap( x, y, stepoffl );
			}
		}
	}

	p.setFont( pointSize<7>( p.font() ) );
	p.setPen( QColor( 32, 240, 32 ) );
	p.drawText( 2, 9, m_name );
	if( m_frozenPattern != NULL )
	{
		p.setPen( QColor( 0, 224, 255 ) );
		p.drawRect( 0, 0, width(), height() - 1 );
		p.drawPixmap( 3, height() - s_frozen->height() - 4, *s_frozen );
	}

	p.end();

	p.begin( this );
	p.drawPixmap( 0, 0, m_paintPixmap );

}




void pattern::ensureBeatNotes( void )
{
	// make sure, that all step-note exist
	for( int i = 0; i < m_steps; ++i )
	{
		bool found = FALSE;
		for( noteVector::iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
		{
			if( ( *it )->pos() == i * BEATS_PER_TACT &&
							( *it )->length() <= 0 )
			{
				found = TRUE;
				break;
			}
		}
		if( found == FALSE )
		{
			addNote( note( midiTime( 0 ), midiTime( i *
							BEATS_PER_TACT ) ) );
		}
	}
}




void pattern::updateBBTrack( void )
{
	if( getTrack()->getTrackContainer() == eng()->getBBEditor() )
	{
		eng()->getBBEditor()->updateBBTrack( this );
	}
}







patternFreezeStatusDialog::patternFreezeStatusDialog( QThread * _thread ) :
	QDialog(),
	m_freezeThread( _thread ),
	m_progress( 0 )
{
	setWindowTitle( tr( "Freezing pattern..." ) );
#if QT_VERSION >= 0x030200
	setModal( TRUE );
#endif

	m_progressBar = new QProgressBar( this );
	m_progressBar->setGeometry( 10, 10, 200, 24 );
#ifdef QT4
	m_progressBar->setMaximum( 100 );
#else
	m_progressBar->setTotalSteps( 100 );
#endif
	m_progressBar->setTextVisible( FALSE );
	m_progressBar->show();
	m_cancelBtn = new QPushButton( embed::getIconPixmap( "cancel" ),
							tr( "Cancel" ), this );
	m_cancelBtn->setGeometry( 50, 38, 120, 28 );
	m_cancelBtn->show();
	connect( m_cancelBtn, SIGNAL( clicked() ), this,
						SLOT( cancelBtnClicked() ) );
	show();

	QTimer * update_timer = new QTimer( this );
	connect( update_timer, SIGNAL( timeout() ),
					this, SLOT( updateProgress() ) );
	update_timer->start( 100 );

#ifdef QT4
	setAttribute( Qt::WA_DeleteOnClose, TRUE );
#else
	setWFlags( getWFlags() | Qt::WDestructiveClose );
#endif
	connect( this, SIGNAL( aborted() ), this, SLOT( reject() ) );

}




patternFreezeStatusDialog::~patternFreezeStatusDialog()
{
	m_freezeThread->wait();
	delete m_freezeThread;
}





void patternFreezeStatusDialog::setProgress( int _p )
{
	m_progress = _p;
}




void patternFreezeStatusDialog::closeEvent( QCloseEvent * _ce )
{
	_ce->ignore();
	cancelBtnClicked();
}




void patternFreezeStatusDialog::cancelBtnClicked( void )
{
	emit( aborted() );
	done( -1 );
}




void patternFreezeStatusDialog::updateProgress( void )
{
	if( m_progress < 0 )
	{
		done( 0 );
	}
	else
	{
#ifdef QT4
		m_progressBar->setValue( m_progress );
#else
		m_progressBar->setProgress( m_progress );
#endif
	}
}








patternFreezeThread::patternFreezeThread( pattern * _pattern ) :
	QThread(),
	engineObject( _pattern->eng() ),
	m_pattern( _pattern )
{
	// create status-dialog
	m_statusDlg = new patternFreezeStatusDialog( this );
	QObject::connect( m_statusDlg, SIGNAL( aborted() ),
					m_pattern, SLOT( abortFreeze() ) );

	start();
}




patternFreezeThread::~patternFreezeThread()
{
	m_pattern->update();
}




void patternFreezeThread::run( void )
{
	bufferAllocator::disableAutoCleanup( TRUE );

	// create and install audio-sample-recorder
	bool b;
	// we cannot create local copy, because at a later stage
	// mixer::restoreAudioDevice(...) deletes old audio-dev and thus
	// audioSampleRecorder would be destroyed two times...
	audioSampleRecorder * freeze_recorder = new audioSampleRecorder(
						eng()->getMixer()->sampleRate(),
							DEFAULT_CHANNELS, b,
							eng()->getMixer() );
	eng()->getMixer()->setAudioDevice( freeze_recorder,
					eng()->getMixer()->highQuality() );

	// prepare stuff for playing correct things later
	eng()->getSongEditor()->playPattern( m_pattern, FALSE );
	songEditor::playPos & ppp = eng()->getSongEditor()->getPlayPos(
						songEditor::PLAY_PATTERN );
	ppp.setTact( 0 );
	ppp.setTact64th( 0 );
	ppp.setCurrentFrame( 0 );
	ppp.m_timeLineUpdate = FALSE;

	m_pattern->m_freezeAborted = FALSE;
	m_pattern->m_freezing = TRUE;


	// now render everything
	while( ppp < m_pattern->length() &&
					m_pattern->m_freezeAborted == FALSE )
	{
		freeze_recorder->processNextBuffer();
		m_statusDlg->setProgress( ppp * 100 / m_pattern->length() );
	}


	m_pattern->m_freezing = FALSE;

	// reset song-editor settings
	eng()->getSongEditor()->stop();
	ppp.m_timeLineUpdate = TRUE;

	// create final sample-buffer if freezing was successful
	if( m_pattern->m_freezeAborted == FALSE )
	{
		m_pattern->m_frozenPatternMutex.lock();
		freeze_recorder->createSampleBuffer(
						&m_pattern->m_frozenPattern );
		m_pattern->m_frozenPatternMutex.unlock();
	}

	bufferAllocator::disableAutoCleanup( FALSE );

	// restore original audio-device
	eng()->getMixer()->restoreAudioDevice();

	m_statusDlg->setProgress( -1 );	// we're finished

}





#include "pattern.moc"


#endif
