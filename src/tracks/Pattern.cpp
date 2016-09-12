/*
 * Pattern.cpp - implementation of class pattern which holds notes
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
#include "Pattern.h"

#include <QDomElement>
#include <QTimer>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QtAlgorithms>

#include "InstrumentTrack.h"
#include "templates.h"
#include "gui_templates.h"
#include "embed.h"
#include "GuiApplication.h"
#include "PianoRoll.h"
#include "TrackContainer.h"
#include "RenameDialog.h"
#include "SampleBuffer.h"
#include "AudioSampleRecorder.h"
#include "Song.h"
#include "ToolTip.h"
#include "BBTrackContainer.h"
#include "StringPairDrag.h"
#include "MainWindow.h"


QPixmap * PatternView::s_stepBtnOn = NULL;
QPixmap * PatternView::s_stepBtnOverlay = NULL;
QPixmap * PatternView::s_stepBtnOff = NULL;
QPixmap * PatternView::s_stepBtnOffLight = NULL;



Pattern::Pattern( InstrumentTrack * _instrument_track ) :
	TrackContentObject( _instrument_track ),
	m_instrumentTrack( _instrument_track ),
	m_patternType( BeatPattern ),
	m_steps( MidiTime::stepsPerTact() )
{
	setName( _instrument_track->name() );
	if( _instrument_track->trackContainer()
					== Engine::getBBTrackContainer() )
	{
		resizeToFirstTrack();
	}
	init();
	setAutoResize( true );
}




Pattern::Pattern( const Pattern& other ) :
	TrackContentObject( other.m_instrumentTrack ),
	m_instrumentTrack( other.m_instrumentTrack ),
	m_patternType( other.m_patternType ),
	m_steps( other.m_steps )
{
	for( NoteVector::ConstIterator it = other.m_notes.begin(); it != other.m_notes.end(); ++it )
	{
		m_notes.push_back( new Note( **it ) );
	}

	init();
	switch( getTrack()->trackContainer()->type() )
	{
		case TrackContainer::BBContainer:
			setAutoResize( true );
			break;

		case TrackContainer::SongContainer:
			// move down
		default:
			setAutoResize( false );
			break;
	}
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




void Pattern::resizeToFirstTrack()
{
	// Resize this track to be the same as existing tracks in the BB
	const TrackContainer::TrackList & tracks =
		m_instrumentTrack->trackContainer()->tracks();
	for(unsigned int trackID = 0; trackID < tracks.size(); ++trackID)
	{
		if(tracks.at(trackID)->type() == Track::InstrumentTrack)
		{
			if(tracks.at(trackID) != m_instrumentTrack)
			{
				unsigned int currentTCO = m_instrumentTrack->
					getTCOs().indexOf(this);
				m_steps = static_cast<Pattern *>
					(tracks.at(trackID)->getTCO(currentTCO))
					->m_steps;
			}
			break;
		}
	}
}




void Pattern::init()
{
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
				this, SLOT( changeTimeSignature() ) );
	saveJournallingState( false );

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

Note * Pattern::addNote( const Note & _new_note, const bool _quant_pos )
{
	Note * new_note = new Note( _new_note );
	if( _quant_pos && gui->pianoRoll() )
	{
		new_note->quantizePos( gui->pianoRoll()->quantization() );
	}

	instrumentTrack()->lock();
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
	instrumentTrack()->unlock();

	checkType();
	changeLength( length() );

	emit dataChanged();

	updateBBTrack();

	return new_note;
}




void Pattern::removeNote( Note * _note_to_del )
{
	instrumentTrack()->lock();
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
	instrumentTrack()->unlock();

	checkType();
	changeLength( length() );

	emit dataChanged();

	updateBBTrack();
}


// returns a pointer to the note at specified step, or NULL if note doesn't exist

Note * Pattern::noteAtStep( int _step )
{
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		if( ( *it )->pos() == MidiTime::stepPosition( _step )
						&& ( *it )->length() < 0 )
		{
			return *it;
		}
	}
	return NULL;
}


Note * Pattern::rearrangeNote( Note * _note_to_proc, const bool _quant_pos )
{
	// just rearrange the position of the note by removing it and adding
	// a copy of it -> addNote inserts it at the correct position
	Note copy_of_note( *_note_to_proc );
	removeNote( _note_to_proc );

	return addNote( copy_of_note, _quant_pos );
}



void Pattern::rearrangeAllNotes()
{
	// sort notes by start time
	qSort(m_notes.begin(), m_notes.end(), Note::lessThan );
}



void Pattern::clearNotes()
{
	instrumentTrack()->lock();
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		delete *it;
	}
	m_notes.clear();
	instrumentTrack()->unlock();

	checkType();
	emit dataChanged();
}




Note * Pattern::addStepNote( int step )
{
	return addNote( Note( MidiTime( -DefaultTicksPerTact ),
				MidiTime::stepPosition( step ) ), false );
}




void Pattern::setStep( int step, bool enabled )
{
	if( enabled )
	{
		if ( !noteAtStep( step ) )
		{
			addStepNote( step );
		}
		return;
	}

	while( Note * note = noteAtStep( step ) )
	{
		removeNote( note );
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
		( *it )->saveState( _doc, _this );
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
			Note * n = new Note;
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

	checkType();

	emit dataChanged();

	updateBBTrack();
}




Pattern *  Pattern::previousPattern() const
{
	return adjacentPatternByOffset(-1);
}




Pattern *  Pattern::nextPattern() const
{
	return adjacentPatternByOffset(1);
}




Pattern * Pattern::adjacentPatternByOffset(int offset) const
{
	QVector<TrackContentObject *> tcos = m_instrumentTrack->getTCOs();
	int tcoNum = m_instrumentTrack->getTCONum(this);
	return dynamic_cast<Pattern*>(tcos.value(tcoNum + offset, NULL));
}




void Pattern::clear()
{
	addJournalCheckPoint();
	clearNotes();
}




void Pattern::addSteps()
{
	m_steps += MidiTime::stepsPerTact();
	emit dataChanged();
	updateBBTrack();
}

void Pattern::cloneSteps()
{
	int oldLength = m_steps;
	m_steps *= 2; // cloning doubles the track
	for(int i = 0; i < oldLength; ++i )
	{
		Note *toCopy = noteAtStep( i );
		if( toCopy )
		{
			setStep( oldLength + i, true );
			Note *newNote = noteAtStep( oldLength + i );
			newNote->setKey( toCopy->key() );
			newNote->setLength( toCopy->length() );
			newNote->setPanning( toCopy->getPanning() );
			newNote->setVolume( toCopy->getVolume() );
		}
	}
	emit dataChanged();
	updateBBTrack();
}




void Pattern::removeSteps()
{
	int n = MidiTime::stepsPerTact();
	if( n < m_steps )
	{
		for( int i = m_steps - n; i < m_steps; ++i )
		{
			setStep( i, false );
		}
		m_steps -= n;
		emit dataChanged();
	}
	updateBBTrack();
}




TrackContentObjectView * Pattern::createView( TrackView * _tv )
{
	return new PatternView( this, _tv );
}




void Pattern::updateBBTrack()
{
	if( getTrack()->trackContainer() == Engine::getBBTrackContainer() )
	{
		Engine::getBBTrackContainer()->updateBBTrack( this );
	}

	if( gui && gui->pianoRoll() && gui->pianoRoll()->currentPattern() == this )
	{
		gui->pianoRoll()->update();
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
	MidiTime last_pos = MidiTime::ticksPerTact() - 1;
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
	m_steps = qMax<tick_t>( MidiTime::stepsPerTact(),
				last_pos.getTact() * MidiTime::stepsPerTact() );
	updateBBTrack();
}





PatternView::PatternView( Pattern* pattern, TrackView* parent ) :
	TrackContentObjectView( pattern, parent ),
	m_pat( pattern ),
	m_paintPixmap()
{
	connect( gui->pianoRoll(), SIGNAL( currentPatternChanged() ),
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
	
	update();	

	setStyle( QApplication::style() );
}






PatternView::~PatternView()
{
}





void PatternView::update()
{
	if ( m_pat->m_patternType == Pattern::BeatPattern )
	{
		ToolTip::add( this,
			tr( "use mouse wheel to set velocity of a step" ) );	
	}
	else 
	{
		ToolTip::add( this,
			tr( "double-click to open in Piano Roll" ) );		
	}
	
	TrackContentObjectView::update();
}




void PatternView::openInPianoRoll()
{
	gui->pianoRoll()->setCurrentPattern( m_pat );
	gui->pianoRoll()->parentWidget()->show();
	gui->pianoRoll()->show();
	gui->pianoRoll()->setFocus();
}




void PatternView::resetName()
{
	m_pat->setName( m_pat->m_instrumentTrack->name() );
}




void PatternView::changeName()
{
	QString s = m_pat->name();
	RenameDialog rename_dlg( s );
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

	if ( m_pat->type() == Pattern::BeatPattern )
	{
		_cm->addSeparator();

		_cm->addAction( embed::getIconPixmap( "step_btn_add" ),
			tr( "Add steps" ), m_pat, SLOT( addSteps() ) );
		_cm->addAction( embed::getIconPixmap( "step_btn_remove" ),
			tr( "Remove steps" ), m_pat, SLOT( removeSteps() ) );
		_cm->addAction( embed::getIconPixmap( "step_btn_duplicate" ),
			tr( "Clone Steps" ), m_pat, SLOT( cloneSteps() ) );
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

		Note * n = m_pat->noteAtStep( step );

		if( n == NULL )
		{
			m_pat->addStepNote( step );
		}
		else // note at step found
		{
			m_pat->addJournalCheckPoint();
			m_pat->setStep( step, false );
		}

		Engine::getSong()->setModified();
		update();

		if( gui->pianoRoll()->currentPattern() == m_pat )
		{
			gui->pianoRoll()->update();
		}
	}
	else

	// if not in beat/bassline -mode, let parent class handle the event

	{
		TrackContentObjectView::mousePressEvent( _me );
	}
}

void PatternView::mouseDoubleClickEvent(QMouseEvent *_me)
{
	if( _me->button() != Qt::LeftButton )
	{
		_me->ignore();
		return;
	}
	if( m_pat->m_patternType == Pattern::MelodyPattern || !fixedTCOs() )
	{
		openInPianoRoll();
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

		Note * n = m_pat->noteAtStep( step );
		if( !n && _we->delta() > 0 )
		{
			n = m_pat->addStepNote( step );
			n->setVolume( 0 );
		}
		if( n != NULL )
		{
			int vol = n->getVolume();

			if( _we->delta() > 0 )
			{
				n->setVolume( qMin( 100, vol + 5 ) );
			}
			else
			{
				n->setVolume( qMax( 0, vol - 5 ) );
			}

			Engine::getSong()->setModified();
			update();
			if( gui->pianoRoll()->currentPattern() == m_pat )
			{
				gui->pianoRoll()->update();
			}
		}
		_we->accept();
	}
	else
	{
		TrackContentObjectView::wheelEvent( _we );
	}
}




void PatternView::paintEvent( QPaintEvent * )
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate( false );

	m_paintPixmap = m_paintPixmap.isNull() == true || m_paintPixmap.size() != size() 
		? QPixmap( size() ) : m_paintPixmap;

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c;
	bool muted = m_pat->getTrack()->isMuted() || m_pat->isMuted();
	bool current = gui->pianoRoll()->currentPattern() == m_pat;
	bool beatPattern = m_pat->m_patternType == Pattern::BeatPattern;
	
	// state: selected, normal, beat pattern, muted
	c = isSelected() ? selectedColor() : ( ( !muted && !beatPattern ) 
		? painter.background().color() : ( beatPattern 
		? BBPatternBackground() : mutedBackgroundColor() ) );

	// invert the gradient for the background in the B&B editor
	lingrad.setColorAt( beatPattern ? 0 : 1, c.darker( 300 ) );
	lingrad.setColorAt( beatPattern ? 1 : 0, c );
	
	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}
	
	const float ppt = fixedTCOs() ?
			( parentWidget()->width() - 2 * TCO_BORDER_WIDTH )
					/ (float) m_pat->length().getTact() :
				( width() - 2 * TCO_BORDER_WIDTH )
					/ (float) m_pat->length().getTact();

	const int x_base = TCO_BORDER_WIDTH;
	
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
				const int ht = (height() - 1 - TCO_BORDER_WIDTH * 2) -1;

				// determine maximum height value for drawing bounds checking
				const int max_ht = height() - 1 - TCO_BORDER_WIDTH;

				// set colour based on mute status
				p.setPen( muted ? mutedColor() : painter.pen().brush().color() );

				// scan through all the notes and draw them on the pattern
				for( NoteVector::Iterator it =
							m_pat->m_notes.begin();
					it != m_pat->m_notes.end(); ++it )
				{
					// calculate relative y-position
					const float y_key =
						( float( central_key - ( *it )->key() ) / keyrange + 1.0f ) / 2;
					// multiply that by pattern height
					const int y_pos = static_cast<int>( TCO_BORDER_WIDTH + y_key * ht ) + 1;

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
	else if( beatPattern &&	( fixedTCOs() || ppt >= 96
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
			Note * n = m_pat->noteAtStep( it );

			// figure out x and y coordinates for step graphic
			const int x = TCO_BORDER_WIDTH + static_cast<int>( it * w / steps );
			const int y = height() - s_stepBtnOff->height() - 1;

			if( n )
			{
				const int vol = n->getVolume();
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
		
		// draw a transparent rectangle over muted patterns
		if ( muted )
		{
			p.setBrush( mutedBackgroundColor() );
			p.setOpacity( 0.3 );
			p.drawRect( 0, 0, width(), height() );
		}
	}
	
	// bar lines
	const int lineSize = 3;
	p.setPen( c.darker( 300 ) );

	for( tact_t t = 1; t < m_pat->length().getTact(); ++t )
	{
		p.drawLine( x_base + static_cast<int>( ppt * t ) - 1,
				TCO_BORDER_WIDTH, x_base + static_cast<int>(
						ppt * t ) - 1, TCO_BORDER_WIDTH + lineSize );
		p.drawLine( x_base + static_cast<int>( ppt * t ) - 1,
				rect().bottom() - ( lineSize + TCO_BORDER_WIDTH ),
				x_base + static_cast<int>( ppt * t ) - 1,
				rect().bottom() - TCO_BORDER_WIDTH );
	}

	// pattern name
	p.setRenderHint( QPainter::TextAntialiasing );
	
	bool isDefaultName = m_pat->name() == m_pat->instrumentTrack()->name();
	
	if( !isDefaultName && m_staticTextName.text() != m_pat->name() )
	{
		m_staticTextName.setText( m_pat->name() );
	}
	
	QFont font;
	font.setHintingPreference( QFont::PreferFullHinting );
	font.setPointSize( 8 );
	p.setFont( font );
	
	const int textTop = TCO_BORDER_WIDTH + 1;
	const int textLeft = TCO_BORDER_WIDTH + 1;
	
	if( !isDefaultName )
	{
		p.setPen( textShadowColor() );
		p.drawStaticText( textLeft + 1, textTop + 1, m_staticTextName );
		p.setPen( textColor() );
		p.drawStaticText( textLeft, textTop, m_staticTextName );
	}

	// inner border
	if( !beatPattern )
	{
		p.setPen( c.lighter( current ? 160 : 130 ) );
		p.drawRect( 1, 1, rect().right() - TCO_BORDER_WIDTH, 
			rect().bottom() - TCO_BORDER_WIDTH );
	
	// outer border
	p.setPen( ( current && !beatPattern ) ? c.lighter( 130 ) : c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
	}
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


