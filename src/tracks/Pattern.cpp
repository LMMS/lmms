/*
 * pattern.cpp - implementation of class pattern which holds notes
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
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
#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtAlgorithms>

#include "Pattern.h"
#include "InstrumentTrack.h"
#include "templates.h"
#include "gui_templates.h"
#include "embed.h"
#include "engine.h"
#include "PianoRoll.h"
#include "TrackContainer.h"
#include "rename_dialog.h"
#include "SampleBuffer.h"
#include "AudioSampleRecorder.h"
#include "song.h"
#include "tooltip.h"
#include "bb_track_container.h"
#include "string_pair_drag.h"
#include "MainWindow.h"


QPixmap * PatternView::s_stepBtnOn = NULL;
QPixmap * PatternView::s_stepBtnOverlay = NULL;
QPixmap * PatternView::s_stepBtnOff = NULL;
QPixmap * PatternView::s_stepBtnOffLight = NULL;



Pattern::Pattern( InstrumentTrack * _instrument_track ) :
	trackContentObject( _instrument_track ),
	m_instrumentTrack( _instrument_track ),
	m_patternType( BeatPattern ),
	m_steps( MidiTime::stepsPerTact() )
{
	setName( _instrument_track->name() );
	init();
}




Pattern::Pattern( const Pattern& other ) :
	trackContentObject( other.m_instrumentTrack ),
	m_instrumentTrack( other.m_instrumentTrack ),
	m_patternType( other.m_patternType ),
	m_steps( other.m_steps )
{
	for( NoteVector::ConstIterator it = other.m_notes.begin(); it != other.m_notes.end(); ++it )
	{
		m_notes.push_back( new note( **it ) );
	}

	init();
}


Pattern::~Pattern()
{
	emit destroyedPattern( this );

	for( NoteVector::Iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		delete *it;
	}

	m_notes.clear();
}




void Pattern::init()
{
	connect( engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
				this, SLOT( changeTimeSignature() ) );
	saveJournallingState( false );

	ensureBeatNotes();

	changeLength( length() );
	restoreJournallingState();
}




MidiTime Pattern::length() const
{
	if( m_patternType == BeatPattern )
	{
		return beatPatternLength();
	}

	tick_t max_length = MidiTime::ticksPerTact();

	for( NoteVector::ConstIterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		if( ( *it )->length() > 0 )
		{
			max_length = qMax<tick_t>( max_length,
							( *it )->endPos() );
		}
	}
	return MidiTime( max_length ).nextFullTact() *
						MidiTime::ticksPerTact();
}




MidiTime Pattern::beatPatternLength() const
{
	tick_t max_length = MidiTime::ticksPerTact();

	for( NoteVector::ConstIterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		if( ( *it )->length() < 0 )
		{
			max_length = qMax<tick_t>( max_length,
				( *it )->pos() +
					MidiTime::ticksPerTact() /
						MidiTime::stepsPerTact() );
		}
	}

	if( m_steps != MidiTime::stepsPerTact() )
	{
		max_length = m_steps * MidiTime::ticksPerTact() /
						MidiTime::stepsPerTact() ;
	}

	return MidiTime( max_length ).nextFullTact() * MidiTime::ticksPerTact();
}

note * Pattern::addNote( const note & _new_note, const bool _quant_pos )
{
	note * new_note = new note( _new_note );
	if( _quant_pos && engine::pianoRoll() )
	{
		new_note->quantizePos( engine::pianoRoll()->quantization() );
	}

	engine::mixer()->lock();
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
		NoteVector::Iterator it = m_notes.begin();

		while( it != m_notes.end() &&
					( *it )->pos() < new_note_abs_time )
		{
			++it;
		}

		m_notes.insert( it, new_note );
	}
	engine::mixer()->unlock();

	checkType();
	changeLength( length() );

	emit dataChanged();

	updateBBTrack();

	return new_note;
}




void Pattern::removeNote( const note * _note_to_del )
{
	engine::mixer()->lock();
	NoteVector::Iterator it = m_notes.begin();
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
	engine::mixer()->unlock();

	checkType();
	changeLength( length() );

	emit dataChanged();

	updateBBTrack();
}


// returns a pointer to the note at specified step, or NULL if note doesn't exist

note * Pattern::noteAtStep( int _step )
{
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		if( ( *it )->pos() == ( _step *  MidiTime::ticksPerTact() ) / MidiTime::stepsPerTact() )
		{
			return *it;
		}
	}
	return NULL;
}


note * Pattern::rearrangeNote( const note * _note_to_proc,
							const bool _quant_pos )
{
	// just rearrange the position of the note by removing it and adding
	// a copy of it -> addNote inserts it at the correct position
	note copy_of_note( *_note_to_proc );
	removeNote( _note_to_proc );

	return addNote( copy_of_note, _quant_pos );
}



void Pattern::rearrangeAllNotes()
{
	// sort notes by start time
	qSort(m_notes.begin(), m_notes.end(), note::lessThan );
}



void Pattern::clearNotes()
{
	engine::mixer()->lock();
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		delete *it;
	}
	m_notes.clear();
	engine::mixer()->unlock();

	checkType();
	emit dataChanged();
}




void Pattern::setStep( int _step, bool _enabled )
{
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		if( ( *it )->pos() == ( _step * MidiTime::ticksPerTact() ) / MidiTime::stepsPerTact() &&
						( *it )->length() <= 0 )
		{
			( *it )->setLength( _enabled ?
						-DefaultTicksPerTact : 0 );
		}
	}
}




void Pattern::setType( PatternTypes _new_pattern_type )
{
	if( _new_pattern_type == BeatPattern ||
				_new_pattern_type == MelodyPattern )
	{
		m_patternType = _new_pattern_type;
	}
}




void Pattern::checkType()
{
	NoteVector::Iterator it = m_notes.begin();
	while( it != m_notes.end() )
	{
		if( ( *it )->length() > 0 )
		{
			setType( Pattern::MelodyPattern );
			return;
		}
		++it;
	}
	setType( Pattern::BeatPattern );
}




void Pattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "type", m_patternType );
	_this.setAttribute( "name", name() );
	// as the target of copied/dragged pattern is always an existing
	// pattern, we must not store actual position, instead we store -1
	// which tells loadSettings() not to mess around with position
	if( _this.parentNode().nodeName() == "clipboard" ||
			_this.parentNode().nodeName() == "dnddata" )
	{
		_this.setAttribute( "pos", -1 );
	}
	else
	{
		_this.setAttribute( "pos", startPosition() );
	}
	_this.setAttribute( "len", length() );
	_this.setAttribute( "muted", isMuted() );
	_this.setAttribute( "steps", m_steps );

	// now save settings of all notes
	for( NoteVector::Iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		if( ( *it )->length() )
		{
			( *it )->saveState( _doc, _this );
		}
	}
}




void Pattern::loadSettings( const QDomElement & _this )
{
	m_patternType = static_cast<PatternTypes>( _this.attribute( "type"
								).toInt() );
	setName( _this.attribute( "name" ) );
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	changeLength( MidiTime( _this.attribute( "len" ).toInt() ) );
	if( _this.attribute( "muted" ).toInt() != isMuted() )
	{
		toggleMute();
	}

	clearNotes();

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() &&
			!node.toElement().attribute( "metadata" ).toInt() )
		{
			note * n = new note;
			n->restoreState( node.toElement() );
			m_notes.push_back( n );
		}
		node = node.nextSibling();
        }

	m_steps = _this.attribute( "steps" ).toInt();
	if( m_steps == 0 )
	{
		m_steps = MidiTime::stepsPerTact();
	}

	ensureBeatNotes();
	checkType();

	emit dataChanged();

	updateBBTrack();
}




void Pattern::clear()
{
	addJournalCheckPoint();
	clearNotes();
	ensureBeatNotes();
}




void Pattern::addSteps()
{
	m_steps += MidiTime::stepsPerTact();
	ensureBeatNotes();
	emit dataChanged();
	updateBBTrack();
}




void Pattern::removeSteps()
{
	int _n = MidiTime::stepsPerTact();
	if( _n < m_steps )
	{
		for( int i = m_steps - _n; i < m_steps; ++i )
		{
			for( NoteVector::Iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
			{
				if( ( *it )->pos() ==
					( i * MidiTime::ticksPerTact() ) /
						MidiTime::stepsPerTact() &&
							( *it )->length() <= 0 )
				{
					removeNote( *it );
					break;
				}
			}
		}
		m_steps -= _n;
		emit dataChanged();
	}
	updateBBTrack();
}




trackContentObjectView * Pattern::createView( trackView * _tv )
{
	return new PatternView( this, _tv );
}





void Pattern::ensureBeatNotes()
{
	// make sure, that all step-note exist
	for( int i = 0; i < m_steps; ++i )
	{
		bool found = false;
		for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end(); ++it )
		{
			// if a note in this position is the one we want
			if( ( *it )->pos() ==
				( i * MidiTime::ticksPerTact() ) / MidiTime::stepsPerTact()
				&& ( *it )->length() <= 0 )
			{
				found = true;
				break;
			}
		}
		if( found == false )
		{
			addNote( note( MidiTime( 0 ), MidiTime( ( i *
				MidiTime::ticksPerTact() ) /
					MidiTime::stepsPerTact() ) ), false );
		}
	}

	// remove notes we no longer need:
	// that is, disabled notes that no longer fall to the steps of the new time sig

	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end(); )
	{
		bool needed = false;
		for( int i = 0; i < m_steps; ++i )
		{
			if( ( *it )->pos() == ( i * MidiTime::ticksPerTact() ) / MidiTime::stepsPerTact()
				|| ( *it )->length() != 0 )
			{
				needed = true;
				break;
			}
		}
		if( needed == false )
		{
			delete *it;
			it = m_notes.erase( it );
		}
		else ++it;
	}
}




void Pattern::updateBBTrack()
{
	if( getTrack()->trackContainer() == engine::getBBTrackContainer() )
	{
		engine::getBBTrackContainer()->updateBBTrack( this );
	}

	if( engine::pianoRoll() && engine::pianoRoll()->currentPattern() == this )
	{
		engine::pianoRoll()->update();
	}
}




bool Pattern::empty()
{
	for( NoteVector::ConstIterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		if( ( *it )->length() != 0 )
		{
			return false;
		}
	}
	return true;
}




void Pattern::changeTimeSignature()
{
	MidiTime last_pos = MidiTime::ticksPerTact();
	for( NoteVector::ConstIterator cit = m_notes.begin();
						cit != m_notes.end(); ++cit )
	{
		if( ( *cit )->length() < 0 && ( *cit )->pos() > last_pos )
		{
			last_pos = ( *cit )->pos()+MidiTime::ticksPerTact() /
						MidiTime::stepsPerTact();
		}
	}
	last_pos = last_pos.nextFullTact() * MidiTime::ticksPerTact();
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end(); )
	{
		if( ( *it )->length() == 0 && ( *it )->pos() >= last_pos )
		{
			delete *it;
			it = m_notes.erase( it );
			--m_steps;
		}
		else
		{
			++it;
		}
	}
	m_steps = qMax<tick_t>(
		qMax<tick_t>( m_steps, MidiTime::stepsPerTact() ),
				last_pos.getTact() * MidiTime::stepsPerTact() );
	ensureBeatNotes();
	updateBBTrack();
}





PatternView::PatternView( Pattern* pattern, trackView* parent ) :
	trackContentObjectView( pattern, parent ),
	m_pat( pattern ),
	m_paintPixmap(),
	m_needsUpdate( true )
{
	connect( engine::pianoRoll(), SIGNAL( currentPatternChanged() ),
			this, SLOT( update() ) );

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

	setFixedHeight( parentWidget()->height() - 2 );
	setAutoResizeEnabled( false );

	toolTip::add( this,
		tr( "double-click to open this pattern in piano-roll\n"
			"use mouse wheel to set volume of a step" ) );
	setStyle( QApplication::style() );
}






PatternView::~PatternView()
{
}





void PatternView::update()
{
	m_needsUpdate = true;
	m_pat->changeLength( m_pat->length() );
	trackContentObjectView::update();
}




void PatternView::openInPianoRoll()
{
	engine::pianoRoll()->setCurrentPattern( m_pat );
	engine::pianoRoll()->parentWidget()->show();
	engine::pianoRoll()->setFocus();
}




void PatternView::resetName()
{
	m_pat->setName( m_pat->m_instrumentTrack->name() );
}




void PatternView::changeName()
{
	QString s = m_pat->name();
	renameDialog rename_dlg( s );
	rename_dlg.exec();
	m_pat->setName( s );
}




void PatternView::constructContextMenu( QMenu * _cm )
{
	QAction * a = new QAction( embed::getIconPixmap( "piano" ),
					tr( "Open in piano-roll" ), _cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ),
					this, SLOT( openInPianoRoll() ) );
	_cm->insertSeparator( _cm->actions()[1] );

	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "edit_erase" ),
			tr( "Clear all notes" ), m_pat, SLOT( clear() ) );
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT( resetName() ) );
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT( changeName() ) );
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "step_btn_add" ),
		tr( "Add steps" ), m_pat, SLOT( addSteps() ) );
	_cm->addAction( embed::getIconPixmap( "step_btn_remove" ),
		tr( "Remove steps" ), m_pat, SLOT( removeSteps() ) );
}




void PatternView::mouseDoubleClickEvent( QMouseEvent * _me )
{
	if( _me->button() != Qt::LeftButton )
	{
		_me->ignore();
		return;
	}
	if( m_pat->type() == Pattern::MelodyPattern ||
		!( m_pat->type() == Pattern::BeatPattern &&
		( pixelsPerTact() >= 192 ||
	  			m_pat->m_steps != MidiTime::stepsPerTact() ) &&
		_me->y() > height() - s_stepBtnOff->height() ) )
	{
		openInPianoRoll();
	}
}




void PatternView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
				m_pat->m_patternType == Pattern::BeatPattern &&
				( fixedTCOs() || pixelsPerTact() >= 96 ||
				m_pat->m_steps != MidiTime::stepsPerTact() ) &&
				_me->y() > height() - s_stepBtnOff->height() )

	// when mouse button is pressed in beat/bassline -mode

	{
//	get the step number that was clicked on and
//	do calculations in floats to prevent rounding errors...
		float tmp = ( ( float(_me->x()) - TCO_BORDER_WIDTH ) *
				float( m_pat -> m_steps ) ) / float(width() - TCO_BORDER_WIDTH*2);

		int step = int( tmp );

//	debugging to ensure we get the correct step...
//		qDebug( "Step (%f) %d", tmp, step );

		if( step >= m_pat->m_steps )
		{
			qDebug( "Something went wrong in pattern.cpp: step %d doesn't exist in pattern!", step );
			return;
		}

		note * n = m_pat->noteAtStep( step );

		// if note at step not found, ensureBeatNotes and try again
		if( n == NULL )
		{
			m_pat -> ensureBeatNotes();
			n = m_pat->noteAtStep( step );
			if( n == NULL ) // still can't find a note? bail!
			{
				qDebug( "Something went wrong in pattern.cpp: couldn't add note at step %d!", step );
				return;
			}
		}
		else // note at step found
		{
			if( n->length() < 0 )
			{
				n->setLength( 0 );	// set note as enabled beat note
			}
			else
			{
				n->setLength( -DefaultTicksPerTact );	// set note as disabled beat note
			}
		}

		engine::getSong()->setModified();
		update();

		if( engine::pianoRoll()->currentPattern() == m_pat )
		{
			engine::pianoRoll()->update();
		}
	}
	else

	// if not in beat/bassline -mode, let parent class handle the event

	{
		trackContentObjectView::mousePressEvent( _me );
	}
}




void PatternView::wheelEvent( QWheelEvent * _we )
{
	if( m_pat->m_patternType == Pattern::BeatPattern &&
				( fixedTCOs() || pixelsPerTact() >= 96 ||
				m_pat->m_steps != MidiTime::stepsPerTact() ) &&
				_we->y() > height() - s_stepBtnOff->height() )
	{
//	get the step number that was wheeled on and
//	do calculations in floats to prevent rounding errors...
		float tmp = ( ( float(_we->x()) - TCO_BORDER_WIDTH ) *
				float( m_pat -> m_steps ) ) / float(width() - TCO_BORDER_WIDTH*2);

		int step = int( tmp );

		if( step >= m_pat->m_steps )
		{
			return;
		}

		int vol = 0;
		int len = 0;

		note * n = m_pat->noteAtStep( step );
		if( n != NULL )
		{
			vol = n->getVolume();
			len = n->length();

			if( len == 0 && _we->delta() > 0 )
			{
				n->setLength( -DefaultTicksPerTact );
				n->setVolume( 5 );
			}
			else if( _we->delta() > 0 )
			{
				n->setVolume( qMin( 100, vol + 5 ) );
			}
			else
			{
				n->setVolume( qMax( 0, vol - 5 ) );
			}

			engine::getSong()->setModified();
			update();
			if( engine::pianoRoll()->currentPattern() == m_pat )
			{
				engine::pianoRoll()->update();
			}
		}
		_we->accept();
	}
	else
	{
		trackContentObjectView::wheelEvent( _we );
	}
}




void PatternView::paintEvent( QPaintEvent * )
{
	if( m_needsUpdate == false )
	{
		QPainter p( this );
		p.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	QPainter _p( this );
	const QColor styleColor = _p.pen().brush().color();

	m_pat->changeLength( m_pat->length() );

	m_needsUpdate = false;

	if( m_paintPixmap.isNull() == true || m_paintPixmap.size() != size() )
	{
		m_paintPixmap = QPixmap( size() );
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );

	QColor c;

	if(( m_pat->m_patternType != Pattern::BeatPattern ) &&
				!( m_pat->getTrack()->isMuted() || m_pat->isMuted() ))
		c = isSelected() ? QColor( 0, 0, 224 )
	  				   : styleColor;
	else
		c = QColor( 80, 80, 80 );

	if( m_pat->m_patternType != Pattern::BeatPattern )
	{
		lingrad.setColorAt( 1, c.darker( 300 ) );
		lingrad.setColorAt( 0, c );
	}
	else
	{
		lingrad.setColorAt( 0, c.darker( 300 ) );
		lingrad.setColorAt( 1, c );
	}

	p.setBrush( lingrad );
	if( engine::pianoRoll()->currentPattern() == m_pat && m_pat->m_patternType != Pattern::BeatPattern )
		p.setPen( c.lighter( 130 ) );
	else
		p.setPen( c.darker( 300 ) );
	p.drawRect( QRect( 0, 0, width() - 1, height() - 1 ) );

	p.setBrush( QBrush() );
	if( m_pat->m_patternType != Pattern::BeatPattern )
	{
		if( engine::pianoRoll()->currentPattern() == m_pat )
			p.setPen( c.lighter( 160 ) );
		else
			p.setPen( c.lighter( 130 ) );
		p.drawRect( QRect( 1, 1, width() - 3, height() - 3 ) );
	}

	const float ppt = fixedTCOs() ?
			( parentWidget()->width() - 2 * TCO_BORDER_WIDTH )
					/ (float) m_pat->length().getTact() :
								pixelsPerTact();

	const int x_base = TCO_BORDER_WIDTH;
	p.setPen( c.darker( 300 ) );

	for( tact_t t = 1; t < m_pat->length().getTact(); ++t )
	{
		p.drawLine( x_base + static_cast<int>( ppt * t ) - 1,
				TCO_BORDER_WIDTH, x_base + static_cast<int>(
						ppt * t ) - 1, 5 );
		p.drawLine( x_base + static_cast<int>( ppt * t ) - 1,
				height() - ( 4 + 2 * TCO_BORDER_WIDTH ),
				x_base + static_cast<int>( ppt * t ) - 1,
				height() - 2 * TCO_BORDER_WIDTH );
	}

// melody pattern paint event

	if( m_pat->m_patternType == Pattern::MelodyPattern )
	{
		if( m_pat->m_notes.size() > 0 )
		{
			// first determine the central tone so that we can
			// display the area where most of the m_notes are
			// also calculate min/max tones so the tonal range can be
			// properly stretched accross the pattern vertically

			int central_key = 0;
			int max_key = 0;
			int min_key = 9999999;
			int total_notes = 0;

			for( NoteVector::Iterator it = m_pat->m_notes.begin();
					it != m_pat->m_notes.end(); ++it )
			{
				if( ( *it )->length() > 0 )
				{
					max_key = qMax( max_key, ( *it )->key() );
					min_key = qMin( min_key, ( *it )->key() );
					central_key += ( *it )->key();
					++total_notes;
				}
			}

			if( total_notes > 0 )
			{
				central_key = central_key / total_notes;
				const int keyrange = qMax( qMax( max_key - central_key, central_key - min_key ), 1 );

				// debug code
				// qDebug( "keyrange: %d", keyrange );

				// determine height of the pattern view, sans borders
				const int ht = height() - 1 - TCO_BORDER_WIDTH * 2;

				// determine maximum height value for drawing bounds checking
				const int max_ht = height() - 1 - TCO_BORDER_WIDTH;

				// set colour based on mute status
				if( m_pat->getTrack()->isMuted() ||
							m_pat->isMuted() )
				{
					p.setPen( QColor( 160, 160, 160 ) );
				}
				else
				{
					p.setPen( fgColor() );	
				}

				// scan through all the notes and draw them on the pattern
				for( NoteVector::Iterator it =
							m_pat->m_notes.begin();
					it != m_pat->m_notes.end(); ++it )
				{
					// calculate relative y-position
					const float y_key =
						( float( central_key - ( *it )->key() ) / keyrange + 1.0f ) / 2;
					// multiply that by pattern height
					const int y_pos = static_cast<int>( TCO_BORDER_WIDTH + y_key * ht );

					// debug code
					// if( ( *it )->length() > 0 ) qDebug( "key %d, central_key %d, y_key %f, y_pos %d", ( *it )->key(), central_key, y_key, y_pos );

					// check that note isn't out of bounds, and has a length
					if( ( *it )->length() > 0 &&
							y_pos >= TCO_BORDER_WIDTH &&
							y_pos <= max_ht )
					{
						// calculate start and end x-coords of the line to be drawn
						const int x1 = x_base +
							static_cast<int>
							( ( *it )->pos() * ( ppt  / MidiTime::ticksPerTact() ) );
						const int x2 = x_base +
							static_cast<int>
							( ( ( *it )->pos() + ( *it )->length() ) * ( ppt  / MidiTime::ticksPerTact() ) );

						// check bounds, draw line
						if( x1 < width() - TCO_BORDER_WIDTH )
							p.drawLine( x1, y_pos,
										qMin( x2, width() - TCO_BORDER_WIDTH ), y_pos );
					}
				}
			}
		}
	}

// beat pattern paint event

	else if( m_pat->m_patternType == Pattern::BeatPattern &&
		( fixedTCOs() || ppt >= 96
			|| m_pat->m_steps != MidiTime::stepsPerTact() ) )
	{
		QPixmap stepon;
		QPixmap stepoverlay;
		QPixmap stepoff;
		QPixmap stepoffl;
		const int steps = qMax( 1,
					m_pat->m_steps );
		const int w = width() - 2 * TCO_BORDER_WIDTH;

		// scale step graphics to fit the beat pattern length
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

		for( int it = 0; it < steps; it++ )	// go through all the steps in the beat pattern
		{
			note * n = m_pat->noteAtStep( it );

			// figure out x and y coordinates for step graphic
			const int x = TCO_BORDER_WIDTH + static_cast<int>( it * w / steps );
			const int y = height() - s_stepBtnOff->height() - 1;

			// get volume and length of note, if noteAtStep returned null
			// (meaning, note at step doesn't exist for some reason)
			// then set both at zero, ie. treat as an off step
			const int vol = ( n != NULL ? n->getVolume() : 0 );
			const int len = ( n != NULL ? int( n->length() ) : 0 );

			if( len < 0 )
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
			else if( ( it / 4 ) % 2 )
			{
				p.drawPixmap( x, y, stepoffl );
			}
			else
			{
				p.drawPixmap( x, y, stepoff );
			}
		} // end for loop
	}

	p.setFont( pointSize<8>( p.font() ) );

	QColor text_color = ( m_pat->isMuted() || m_pat->getTrack()->isMuted() )
		? QColor( 30, 30, 30 )
		: textColor();

	if( m_pat->name() != m_pat->instrumentTrack()->name() )
	{
		p.setPen( QColor( 0, 0, 0 ) );
		p.drawText( 4, p.fontMetrics().height()+1, m_pat->name() );
		p.setPen( text_color );
		p.drawText( 3, p.fontMetrics().height(), m_pat->name() );
	}

	if( m_pat->isMuted() )
	{
		p.drawPixmap( 3, p.fontMetrics().height() + 1,
				embed::getIconPixmap( "muted", 16, 16 ) );
	}

	p.end();

	_p.drawPixmap( 0, 0, m_paintPixmap );

}




#include "moc_Pattern.cxx"


