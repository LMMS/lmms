/*
 * PresetPreviewPlayHandle.cpp - implementation of class PresetPreviewPlayHandle
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QAtomicPointer>
#include <QFileInfo>

#include "PresetPreviewPlayHandle.h"
#include "Engine.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "PluginFactory.h"
#include "ProjectJournal.h"
#include "TrackContainer.h"



// invisible track-container which is needed as parent for preview-channels
class PreviewTrackContainer : public TrackContainer
{
public:
	PreviewTrackContainer() :
		m_previewInstrumentTrack( NULL ),
		m_previewNote( NULL ),
		m_dataMutex()
	{
		setJournalling( false );
		m_previewInstrumentTrack = dynamic_cast<InstrumentTrack *>( Track::create( Track::InstrumentTrack, this ) );
		m_previewInstrumentTrack->setJournalling( false );
		m_previewInstrumentTrack->setPreviewMode( true );
	}

	virtual ~PreviewTrackContainer()
	{
	}

	virtual QString nodeName() const
	{
		return "previewtrackcontainer";
	}

	InstrumentTrack* previewInstrumentTrack()
	{
		return m_previewInstrumentTrack;
	}

	NotePlayHandle* previewNote()
	{
	#if QT_VERSION >= 0x050000
		return m_previewNote.loadAcquire();
	#else
		return m_previewNote;
	#endif
	}

	void setPreviewNote( NotePlayHandle * _note )
	{
	#if QT_VERSION >= 0x050000
		m_previewNote.storeRelease( _note );
	#else
		m_previewNote = _note;
	#endif
	}

	bool testAndSetPreviewNote( NotePlayHandle * expectedVal, NotePlayHandle * newVal )
	{
		return m_previewNote.testAndSetOrdered( expectedVal, newVal );
	}

	void lockData()
	{
		m_dataMutex.lock();
	}

	void unlockData()
	{
		m_dataMutex.unlock();
	}

	bool isPreviewing()
	{
		bool ret = !m_dataMutex.tryLock();
		if( ret == false )
		{
			m_dataMutex.unlock();
		}
		return ret;
	}


private:
	InstrumentTrack* m_previewInstrumentTrack;
	QAtomicPointer<NotePlayHandle> m_previewNote;
	QMutex m_dataMutex;

	friend class PresetPreviewPlayHandle;

} ;


PreviewTrackContainer * PresetPreviewPlayHandle::s_previewTC;



PresetPreviewPlayHandle::PresetPreviewPlayHandle( const QString & _preset_file, bool _load_by_plugin, DataFile *dataFile ) :
	PlayHandle( TypePresetPreviewHandle ),
	m_previewNote( NULL )
{
	setUsesBuffer( false );

	s_previewTC->lockData();

	Engine::mixer()->requestChangeInModel();
	s_previewTC->setPreviewNote( nullptr );
	s_previewTC->previewInstrumentTrack()->silenceAllNotes();
	Engine::mixer()->doneChangeInModel();

	const bool j = Engine::projectJournal()->isJournalling();
	Engine::projectJournal()->setJournalling( false );

	if( _load_by_plugin )
	{
		Instrument * i = s_previewTC->previewInstrumentTrack()->instrument();
		const QString ext = QFileInfo( _preset_file ).
							suffix().toLower();
		if( i == NULL || !i->descriptor()->supportsFileType( ext ) )
		{
			i = s_previewTC->previewInstrumentTrack()->
				loadInstrument(pluginFactory->pluginSupportingExtension(ext).name());
		}
		if( i != NULL )
		{
			i->loadFile( _preset_file );
		}
	}
	else
	{
		bool dataFileCreated = false;
		if( dataFile == 0 )
		{
			dataFile = new DataFile( _preset_file );
			dataFileCreated = true;
		}

		// vestige previews are bug prone; fallback on 3xosc with volume of 0
		// without an instrument in preview track, it will segfault
		if(dataFile->content().elementsByTagName( "vestige" ).length() == 0 )
		{
			s_previewTC->previewInstrumentTrack()->
					loadTrackSpecificSettings(
						dataFile->content().firstChild().toElement() );
		}
		else
		{
			s_previewTC->previewInstrumentTrack()->loadInstrument("tripleoscillator");
			s_previewTC->previewInstrumentTrack()->setVolume( 0 );
		}
		if( dataFileCreated )
		{
			delete dataFile;
		}
	}
	dataFile = 0;
	// make sure, our preset-preview-track does not appear in any MIDI-
	// devices list, so just disable receiving/sending MIDI-events at all
	s_previewTC->previewInstrumentTrack()->
				midiPort()->setMode( MidiPort::Disabled );

	Engine::mixer()->requestChangeInModel();
	// create note-play-handle for it
	m_previewNote = NotePlayHandlePool.construct(
			s_previewTC->previewInstrumentTrack(), 0,
			typeInfo<f_cnt_t>::max() / 2,
				Note( 0, 0, DefaultKey, 100 ) );

	setAudioPort( s_previewTC->previewInstrumentTrack()->audioPort() );

	s_previewTC->setPreviewNote( m_previewNote );

	Engine::mixer()->addPlayHandle( m_previewNote );

	Engine::mixer()->doneChangeInModel();
	s_previewTC->unlockData();
	Engine::projectJournal()->setJournalling( j );
}




PresetPreviewPlayHandle::~PresetPreviewPlayHandle()
{
	Engine::mixer()->requestChangeInModel();
	// not muted by other preset-preview-handle?
	if (s_previewTC->testAndSetPreviewNote(m_previewNote, nullptr))
	{
		m_previewNote->noteOff();
	}
	Engine::mixer()->doneChangeInModel();
}




void PresetPreviewPlayHandle::play( sampleFrame * _working_buffer )
{
	// Do nothing; the preview instrument is played by m_previewNote, which
	// has been added to the mixer
}




bool PresetPreviewPlayHandle::isFinished() const
{
	return m_previewNote->isMuted();
}




bool PresetPreviewPlayHandle::isFromTrack( const Track * _track ) const
{
	return s_previewTC && s_previewTC->previewInstrumentTrack() == _track;
}




void PresetPreviewPlayHandle::init()
{
	if( !s_previewTC )
	{
		s_previewTC = new PreviewTrackContainer;
	}
}




void PresetPreviewPlayHandle::cleanup()
{
	delete s_previewTC;
	s_previewTC = NULL;
}




ConstNotePlayHandleList PresetPreviewPlayHandle::nphsOfInstrumentTrack(
						const InstrumentTrack * _it )
{
	ConstNotePlayHandleList cnphv;
	if( s_previewTC->previewNote() != NULL &&
		s_previewTC->previewNote()->instrumentTrack() == _it )
	{
		cnphv.push_back( s_previewTC->previewNote() );
	}
	return cnphv;
}




bool PresetPreviewPlayHandle::isPreviewing()
{
	if (s_previewTC) {
		return s_previewTC->isPreviewing();
	}
	return false;
}



