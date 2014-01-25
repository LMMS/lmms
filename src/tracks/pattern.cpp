/*
 * pattern.cpp - implementation of class pattern which holds notes
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
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

#include <QtXml/QDomElement>
#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtAlgorithms>

#include "pattern.h"
#include "InstrumentTrack.h"
#include "templates.h"
#include "gui_templates.h"
#include "embed.h"
#include "engine.h"
#include "piano_roll.h"
#include "TrackContainer.h"
#include "rename_dialog.h"
#include "SampleBuffer.h"
#include "AudioSampleRecorder.h"
#include "song.h"
#include "tooltip.h"
#include "bb_track_container.h"
#include "string_pair_drag.h"
#include "MainWindow.h"


QPixmap * patternView::s_stepBtnOn = NULL;
QPixmap * patternView::s_stepBtnOverlay = NULL;
QPixmap * patternView::s_stepBtnOff = NULL;
QPixmap * patternView::s_stepBtnOffLight = NULL;
QPixmap * patternView::s_frozen = NULL;



pattern::pattern( InstrumentTrack * _instrument_track ) :
	trackContentObject( _instrument_track ),
	m_instrumentTrack( _instrument_track ),
	m_patternType( BeatPattern ),
	m_steps( MidiTime::stepsPerTact() ),
	m_frozenPattern( NULL ),
	m_freezing( false ),
	m_freezeAborted( false )
{
	setName( _instrument_track->name() );
	init();
}




pattern::pattern( const pattern & _pat_to_copy ) :
	trackContentObject( _pat_to_copy.m_instrumentTrack ),
	m_instrumentTrack( _pat_to_copy.m_instrumentTrack ),
	m_patternType( _pat_to_copy.m_patternType ),
	m_steps( _pat_to_copy.m_steps ),
	m_frozenPattern( NULL ),
	m_freezeAborted( false )
{
	for( NoteVector::ConstIterator it = _pat_to_copy.m_notes.begin();
					it != _pat_to_copy.m_notes.end(); ++it )
	{
		m_notes.push_back( new note( **it ) );
	}

	init();
}


pattern::~pattern()
{
	for( NoteVector::Iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		delete *it;
	}

	m_notes.clear();

	if( m_frozenPattern )
	{
		sharedObject::unref( m_frozenPattern );
	}
}




void pattern::init()
{
	connect( engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
				this, SLOT( changeTimeSignature() ) );
	saveJournallingState( false );

	ensureBeatNotes();

	changeLength( length() );
	restoreJournallingState();
}




MidiTime pattern::length() const
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




MidiTime pattern::beatPatternLength() const
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




note * pattern::addNote( const note & _new_note, const bool _quant_pos )
{
	note * new_note = new note( _new_note );
	if( _quant_pos && engine::getPianoRoll() )
	{
		new_note->quantizePos( engine::getPianoRoll()->quantization() );
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




void pattern::removeNote( const note * _note_to_del )
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




note * pattern::rearrangeNote( const note * _note_to_proc,
							const bool _quant_pos )
{
	// just rearrange the position of the note by removing it and adding 
	// a copy of it -> addNote inserts it at the correct position
	note copy_of_note( *_note_to_proc );
	removeNote( _note_to_proc );

	return addNote( copy_of_note, _quant_pos );
}



void pattern::rearrangeAllNotes()
{
	// sort notes by start time	
	qSort(m_notes.begin(), m_notes.end(), note::lessThan );
}



void pattern::clearNotes()
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




void pattern::setStep( int _step, bool _enabled )
{
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		if( ( *it )->pos() == _step*DefaultTicksPerTact/16 &&
						( *it )->length() <= 0 )
		{
			( *it )->setLength( _enabled ?
						-DefaultTicksPerTact : 0 );
		}
	}
}




void pattern::setType( PatternTypes _new_pattern_type )
{
	if( _new_pattern_type == BeatPattern ||
				_new_pattern_type == MelodyPattern )
	{
		m_patternType = _new_pattern_type;
	}
}




void pattern::checkType()
{
	NoteVector::Iterator it = m_notes.begin();
	while( it != m_notes.end() )
	{
		if( ( *it )->length() > 0 )
		{
			setType( pattern::MelodyPattern );
			return;
		}
		++it;
	}
	setType( pattern::BeatPattern );
}




void pattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
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
	_this.setAttribute( "frozen", m_frozenPattern != NULL );

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




void pattern::loadSettings( const QDomElement & _this )
{
	unfreeze();

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
/*	if( _this.attribute( "frozen" ).toInt() )
	{
		freeze();
	}*/

	emit dataChanged();

	updateBBTrack();
}




void pattern::clear()
{
	clearNotes();
	ensureBeatNotes();
}




void pattern::freeze()
{
	if( engine::getSong()->isPlaying() )
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

	// already frozen?
	if( m_frozenPattern != NULL )
	{
		// then unfreeze, before freezing it again
		unfreeze();
	}

	new patternFreezeThread( this );

}




void pattern::unfreeze()
{
	if( m_frozenPattern != NULL )
	{
		sharedObject::unref( m_frozenPattern );
		m_frozenPattern = NULL;
		emit dataChanged();
	}
}




void pattern::abortFreeze()
{
	m_freezeAborted = true;
}




void pattern::addSteps()
{
	m_steps += MidiTime::stepsPerTact();
	ensureBeatNotes();
	emit dataChanged();
}




void pattern::removeSteps()
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
					i * MidiTime::ticksPerTact() /
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
}




trackContentObjectView * pattern::createView( trackView * _tv )
{
	return new patternView( this, _tv );
}





void pattern::ensureBeatNotes()
{
	// make sure, that all step-note exist
	for( int i = 0; i < m_steps; ++i )
	{
		bool found = false;
		for( NoteVector::Iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
		{
			if( ( *it )->pos() ==
				i * MidiTime::ticksPerTact() /
					MidiTime::stepsPerTact() &&
							( *it )->length() <= 0 )
			{
				found = true;
				break;
			}
		}
		if( found == false )
		{
			addNote( note( MidiTime( 0 ), MidiTime( i *
				MidiTime::ticksPerTact() /
					MidiTime::stepsPerTact() ) ), false );
		}
	}
}




void pattern::updateBBTrack()
{
	if( getTrack()->trackContainer() == engine::getBBTrackContainer() )
	{
		engine::getBBTrackContainer()->updateBBTrack( this );
	}
}




bool pattern::empty()
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




void pattern::changeTimeSignature()
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
}




patternFreezeStatusDialog::patternFreezeStatusDialog( QThread * _thread ) :
	QDialog(),
	m_freezeThread( _thread ),
	m_progress( 0 )
{
	setWindowTitle( tr( "Freezing pattern..." ) );
	setModal( true );

	m_progressBar = new QProgressBar( this );
	m_progressBar->setGeometry( 10, 10, 200, 24 );
	m_progressBar->setMaximum( 100 );
	m_progressBar->setTextVisible( false );
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

	setAttribute( Qt::WA_DeleteOnClose, true );
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




void patternFreezeStatusDialog::cancelBtnClicked()
{
	emit( aborted() );
	done( -1 );
}




void patternFreezeStatusDialog::updateProgress()
{
	if( m_progress < 0 )
	{
		done( 0 );
	}
	else
	{
		m_progressBar->setValue( m_progress );
	}
}








patternFreezeThread::patternFreezeThread( pattern * _pattern ) :
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
	m_pattern->dataChanged();
}




void patternFreezeThread::run()
{
	// create and install audio-sample-recorder
	bool b;
	// we cannot create local copy, because at a later stage
	// Mixer::restoreAudioDevice(...) deletes old audio-dev and thus
	// AudioSampleRecorder would be destroyed two times...
	AudioSampleRecorder * freeze_recorder = new AudioSampleRecorder(
							DEFAULT_CHANNELS, b,
							engine::mixer() );
	engine::mixer()->setAudioDevice( freeze_recorder );

	// prepare stuff for playing correct things later
	engine::getSong()->playPattern( m_pattern, false );
	song::playPos & ppp = engine::getSong()->getPlayPos(
						song::Mode_PlayPattern );
	ppp.setTicks( 0 );
	ppp.setCurrentFrame( 0 );
	ppp.m_timeLineUpdate = false;

	m_pattern->m_freezeAborted = false;
	m_pattern->m_freezing = true;


	// now render everything
	while( ppp < m_pattern->length() &&
					m_pattern->m_freezeAborted == false )
	{
		freeze_recorder->processNextBuffer();
		m_statusDlg->setProgress( ppp * 100 / m_pattern->length() );
	}
	m_statusDlg->setProgress( 100 );

	// render tails
	int count = 0;
	while( engine::mixer()->hasNotePlayHandles() &&
					m_pattern->m_freezeAborted == false &&
					++count < 2000 )
	{
		freeze_recorder->processNextBuffer();
	}


	m_pattern->m_freezing = false;

	// reset song-editor settings
	engine::getSong()->stop();
	ppp.m_timeLineUpdate = true;

	// create final sample-buffer if freezing was successful
	if( m_pattern->m_freezeAborted == false )
	{
		freeze_recorder->createSampleBuffer(
						&m_pattern->m_frozenPattern );
	}

	// restore original audio-device
	engine::mixer()->restoreAudioDevice();

	m_statusDlg->setProgress( -1 );	// we're finished

}





patternView::patternView( pattern * _pattern, trackView * _parent ) :
	trackContentObjectView( _pattern, _parent ),
	m_pat( _pattern ),
	m_paintPixmap(),
	m_needsUpdate( true )
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

	setFixedHeight( parentWidget()->height() - 2 );
	setAutoResizeEnabled( false );

	toolTip::add( this,
		tr( "double-click to open this pattern in piano-roll\n"
			"use mouse wheel to set volume of a step" ) );
}






patternView::~patternView()
{
	if( engine::getPianoRoll()->currentPattern() == m_pat )
	{
		engine::getPianoRoll()->setCurrentPattern( NULL );
		// we have to have the song-editor to stop playing if it played
		// us before
		if( engine::getSong()->isPlaying() &&
			engine::getSong()->playMode() ==
							song::Mode_PlayPattern )
		{
			engine::getSong()->playPattern( NULL );
		}
	}
}





void patternView::update()
{
	m_needsUpdate = true;
	m_pat->changeLength( m_pat->length() );
	trackContentObjectView::update();
}




void patternView::openInPianoRoll()
{
	engine::getPianoRoll()->setCurrentPattern( m_pat );
	engine::getPianoRoll()->parentWidget()->show();
	engine::getPianoRoll()->setFocus();
}




void patternView::resetName()
{
	m_pat->setName( m_pat->m_instrumentTrack->name() );
}




void patternView::changeName()
{
	QString s = m_pat->name();
	renameDialog rename_dlg( s );
	rename_dlg.exec();
	m_pat->setName( s );
}




void patternView::constructContextMenu( QMenu * _cm )
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

	bool freeze_separator = false;
	if( !( m_pat->m_instrumentTrack->isMuted() || m_pat->isMuted() ) )
	{
		_cm->addAction( embed::getIconPixmap( "freeze" ),
			m_pat->m_frozenPattern ? tr( "Refreeze" ) :
								tr( "Freeze" ),
						m_pat, SLOT( freeze() ) );
		freeze_separator = true;
	}
	if( m_pat->m_frozenPattern )
	{
		_cm->addAction( embed::getIconPixmap( "unfreeze" ),
				tr( "Unfreeze" ), m_pat, SLOT( unfreeze() ) );
		freeze_separator = true;
	}
	if( freeze_separator )
	{
		_cm->addSeparator();
	}

	_cm->addAction( embed::getIconPixmap( "step_btn_add" ),
		tr( "Add steps" ), m_pat, SLOT( addSteps() ) );
	_cm->addAction( embed::getIconPixmap( "step_btn_remove" ),
		tr( "Remove steps" ), m_pat, SLOT( removeSteps() ) );
}




void patternView::mouseDoubleClickEvent( QMouseEvent * _me )
{
	if( _me->button() != Qt::LeftButton )
	{
		_me->ignore();
		return;
	}
	if( m_pat->type() == pattern::MelodyPattern ||
		!( m_pat->type() == pattern::BeatPattern &&
		( pixelsPerTact() >= 192 ||
	  			m_pat->m_steps != MidiTime::stepsPerTact() ) &&
		_me->y() > height() - s_stepBtnOff->height() ) )
	{
		openInPianoRoll();
	}
}




void patternView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
				m_pat->m_patternType == pattern::BeatPattern &&
				( fixedTCOs() || pixelsPerTact() >= 96 ||
				m_pat->m_steps != MidiTime::stepsPerTact() ) &&
				_me->y() > height() - s_stepBtnOff->height() )
	{
		int step = ( _me->x() - TCO_BORDER_WIDTH ) *
				m_pat->length() / DefaultBeatsPerTact / width();
		if( step >= m_pat->m_steps )
		{
			return;
		}
		note * n = m_pat->m_notes[step];
		if( n->length() < 0 )
		{
			n->setLength( 0 );
		}
		else
		{
			n->setLength( -DefaultTicksPerTact );
		}
		engine::getSong()->setModified();
		update();
		if( engine::getPianoRoll()->currentPattern() == m_pat )
		{
			engine::getPianoRoll()->update();
		}
	}
	else if( m_pat->m_frozenPattern != NULL &&
					_me->button() == Qt::LeftButton &&
					_me->modifiers() & Qt::ShiftModifier )
	{
		QString s;
		new stringPairDrag( "sampledata",
					m_pat->m_frozenPattern->toBase64( s ),
					embed::getIconPixmap( "freeze" ),
					this );
	}
	else
	{
		trackContentObjectView::mousePressEvent( _me );
	}
}




void patternView::wheelEvent( QWheelEvent * _we )
{
	if( m_pat->m_patternType == pattern::BeatPattern &&
				( fixedTCOs() || pixelsPerTact() >= 96 ||
				m_pat->m_steps != MidiTime::stepsPerTact() ) &&
				_we->y() > height() - s_stepBtnOff->height() )
	{
		int step = ( _we->x() - TCO_BORDER_WIDTH ) *
				m_pat->length() / DefaultBeatsPerTact / width();
		if( step >= m_pat->m_steps )
		{
			return;
		}
		note * n = m_pat->m_notes[step];
		int vol = n->getVolume();
		
		if( n->length() == 0 && _we->delta() > 0 )
		{
			n->setLength( -DefaultTicksPerTact );
			n->setVolume( 5 );
		}
		else if( _we->delta() > 0 )
		{
			if( vol < 95 )
			{
				n->setVolume( vol + 5 );
			}
			else
			{
				n->setVolume( 100 );
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
		engine::getSong()->setModified();
		update();
		if( engine::getPianoRoll()->currentPattern() == m_pat )
		{
			engine::getPianoRoll()->update();
		}
		_we->accept();
	}
	else
	{
		trackContentObjectView::wheelEvent( _we );
	}
}




void patternView::paintEvent( QPaintEvent * )
{
	if( m_needsUpdate == false )
	{
		QPainter p( this );
		p.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	m_pat->changeLength( m_pat->length() );

	m_needsUpdate = false;

	if( m_paintPixmap.isNull() == true || m_paintPixmap.size() != size() )
	{
		m_paintPixmap = QPixmap( size() );
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	const QColor c = isSelected() ? QColor( 0, 0, 224 ) :
							QColor( 96, 96, 96 );
	lingrad.setColorAt( 0, QColor( 16, 16, 16 ) );
	lingrad.setColorAt( 0.5, c );
	lingrad.setColorAt( 1, QColor( 16, 16, 16 ) );
	p.setBrush( lingrad );
	p.setPen( QColor( 0, 0, 0 ) );
	//p.drawRect( 0, 0, width() - 1, height() - 1 );
	p.drawRect( QRect( 0, 0, width() - 1, height() - 1 ) );


	const float ppt = fixedTCOs() ?
			( parentWidget()->width() - 2 * TCO_BORDER_WIDTH )
					/ (float) m_pat->length().getTact() :
								pixelsPerTact();

	const int x_base = TCO_BORDER_WIDTH;
	p.setPen( QColor( 0, 0, 0 ) );

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

	if( m_pat->m_patternType == pattern::MelodyPattern )
	{
		int central_key = 0;
		if( m_pat->m_notes.size() > 0 )
		{
			// first determine the central tone so that we can 
			// display the area where most of the m_notes are
			int total_notes = 0;
			for( NoteVector::Iterator it = m_pat->m_notes.begin();
					it != m_pat->m_notes.end(); ++it )
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

				const int central_y = height() / 2;
				int y_base = central_y + TCO_BORDER_WIDTH -1;

				if( m_pat->getTrack()->isMuted() ||
							m_pat->isMuted() )
				{
					p.setPen( QColor( 160, 160, 160 ) );
				}
				else if( m_pat->m_frozenPattern != NULL )
				{
					p.setPen( QColor( 0x70, 0xFF, 0xFF ) );
				}
				else
				{
					p.setPen( QColor( 0x77, 0xC7, 0xD8 ) );
				}

				for( NoteVector::Iterator it =
							m_pat->m_notes.begin();
					it != m_pat->m_notes.end(); ++it )
				{
					const int y_pos = central_key -
								( *it )->key();

					if( ( *it )->length() > 0 &&
							y_pos > -central_y &&
							y_pos < central_y )
					{
						const int x1 = 2 * x_base +
		static_cast<int>( ( *it )->pos() * ppt /
						MidiTime::ticksPerTact() );
						const int x2 =
			static_cast<int>( ( ( *it )->pos() + ( *it )->length() ) * ppt / MidiTime::ticksPerTact() );
						p.drawLine( x1, y_base + y_pos,
							x2, y_base + y_pos );

					}
				}
			}
		}
	}
	else if( m_pat->m_patternType == pattern::BeatPattern &&
		( fixedTCOs() || ppt >= 96
			|| m_pat->m_steps != MidiTime::stepsPerTact() ) )
	{
		QPixmap stepon;
		QPixmap stepoverlay;
		QPixmap stepoff;
		QPixmap stepoffl;
		const int steps = qMax( 1,
					m_pat->length() / DefaultBeatsPerTact );
		const int w = width() - 2 * TCO_BORDER_WIDTH;
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
		for( NoteVector::Iterator it = m_pat->m_notes.begin();
					it != m_pat->m_notes.end(); ++it )
		{
			const int no = ( *it )->pos() / DefaultBeatsPerTact;
			const int x = TCO_BORDER_WIDTH + static_cast<int>( no *
								w / steps );
			const int y = height() - s_stepBtnOff->height() - 1;

			const int vol = ( *it )->getVolume();

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
			else if( ( no / 4 ) % 2 )
			{
				p.drawPixmap( x, y, stepoff );
			}
			else
			{
				p.drawPixmap( x, y, stepoffl );
			}
		}
	}

	p.setFont( pointSize<8>( p.font() ) );
	if( m_pat->isMuted() || m_pat->getTrack()->isMuted() )
	{
		p.setPen( QColor( 192, 192, 192 ) );
	}
	else
	{
		p.setPen( QColor( 32, 240, 101 ) );
	}

	if( m_pat->name() != m_pat->instrumentTrack()->name() )
	{
		p.drawText( 2, p.fontMetrics().height() - 1, m_pat->name() );
	}

	if( m_pat->isMuted() )
	{
		p.drawPixmap( 3, p.fontMetrics().height() + 1,
				embed::getIconPixmap( "muted", 16, 16 ) );
	}
	else if( m_pat->m_frozenPattern != NULL )
	{
		p.setBrush( QBrush() );
		p.setPen( QColor( 0x70, 255, 255 ) );
		p.drawRect( 0, 0, width()-1, height() - 1 );
		p.drawPixmap( 3, height() - s_frozen->height() - 4, *s_frozen );
	}

	p.end();

	p.begin( this );
	p.drawPixmap( 0, 0, m_paintPixmap );

}




#include "moc_pattern.cxx"


