/*
 * GigPlayer.cpp - a GIG player using libgig (based on Sf2 player plugin)
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * A few lines of code taken from LinuxSampler (also GPLv2) where noted:
 * Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck
 * Copyright (C) 2005-2008 Christian Schoenebeck
 * Copyright (C) 2009-2010 Christian Schoenebeck and Grigor Iliev
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

#include "GigPlayer.h"

#include <cstring>
#include <QDebug>
#include <QLayout>
#include <QLabel>
#include <QDomDocument>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "endian_handling.h"
#include "Engine.h"
#include "FileDialog.h"
#include "InstrumentTrack.h"
#include "InstrumentPlayHandle.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "PathUtil.h"
#include "Sample.h"
#include "Song.h"

#include "PatchesDialog.h"
#include "LcdSpinBox.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT gigplayer_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"GIG Player",
	QT_TRANSLATE_NOOP( "PluginBrowser", "Player for GIG files" ),
	"Garrett Wilson <g/at/floft/dot/net>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	"gig",
	nullptr,
	nullptr,
} ;

}




GigInstrument::GigInstrument( InstrumentTrack * _instrument_track ) :
	Instrument(_instrument_track, &gigplayer_plugin_descriptor, nullptr, Flag::IsSingleStreamed | Flag::IsNotBendable),
	m_instance( nullptr ),
	m_instrument( nullptr ),
	m_filename( "" ),
	m_bankNum( 0, 0, 999, this, tr( "Bank" ) ),
	m_patchNum( 0, 0, 127, this, tr( "Patch" ) ),
	m_gain( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Gain" ) ),
	m_interpolation( SRC_LINEAR ),
	m_RandomSeed( 0 ),
	m_currentKeyDimension( 0 )
{
	auto iph = new InstrumentPlayHandle(this, _instrument_track);
	Engine::audioEngine()->addPlayHandle( iph );

	updateSampleRate();

	connect( &m_bankNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	connect( &m_patchNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ) );
}




GigInstrument::~GigInstrument()
{
	Engine::audioEngine()->removePlayHandlesOfTypes( instrumentTrack(),
				PlayHandle::Type::NotePlayHandle
				| PlayHandle::Type::InstrumentPlayHandle );
	freeInstance();
}




void GigInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "src", m_filename );
	m_patchNum.saveSettings( _doc, _this, "patch" );
	m_bankNum.saveSettings( _doc, _this, "bank" );

	m_gain.saveSettings( _doc, _this, "gain" );
}




void GigInstrument::loadSettings( const QDomElement & _this )
{
	openFile( _this.attribute( "src" ), false );
	m_patchNum.loadSettings( _this, "patch" );
	m_bankNum.loadSettings( _this, "bank" );

	m_gain.loadSettings( _this, "gain" );

	updatePatch();
}




void GigInstrument::loadFile( const QString & _file )
{
	if( !_file.isEmpty() && QFileInfo( _file ).exists() )
	{
		openFile( _file, false );
		updatePatch();
		updateSampleRate();
	}
}




AutomatableModel * GigInstrument::childModel( const QString & _modelName )
{
	if( _modelName == "bank" )
	{
		return &m_bankNum;
	}
	else if( _modelName == "patch" )
	{
		return &m_patchNum;
	}

	qCritical() << "requested unknown model " << _modelName;

	return nullptr;
}




QString GigInstrument::nodeName() const
{
	return gigplayer_plugin_descriptor.name;
}




void GigInstrument::freeInstance()
{
	QMutexLocker synthLock( &m_synthMutex );
	QMutexLocker notesLock( &m_notesMutex );

	if( m_instance != nullptr )
	{
		delete m_instance;
		m_instance = nullptr;

		// If we're changing instruments, we got to make sure that we
		// remove all pointers to the old samples and don't try accessing
		// that instrument again
		m_instrument = nullptr;
		m_notes.clear();
	}
}




void GigInstrument::openFile( const QString & _gigFile, bool updateTrackName )
{
	emit fileLoading();

	// Remove the current instrument if one is selected
	freeInstance();

	{
		QMutexLocker locker( &m_synthMutex );

		try
		{
			m_instance = new GigInstance( PathUtil::toAbsolute( _gigFile ) );
			m_filename = PathUtil::toShortestRelative( _gigFile );
		}
		catch( ... )
		{
			m_instance = nullptr;
			m_filename = "";
		}
	}

	emit fileChanged();

	if( updateTrackName == true )
	{
		instrumentTrack()->setName(PathUtil::cleanName( _gigFile ) );
		updatePatch();
	}
}




void GigInstrument::updatePatch()
{
	if( m_bankNum.value() >= 0 && m_patchNum.value() >= 0 )
	{
		getInstrument();
	}
}




QString GigInstrument::getCurrentPatchName()
{
	QMutexLocker locker( &m_synthMutex );

	if( m_instance == nullptr )
	{
		return "";
	}

	int iBankSelected = m_bankNum.value();
	int iProgSelected = m_patchNum.value();

	gig::Instrument * pInstrument = m_instance->gig.GetFirstInstrument();

	while( pInstrument != nullptr )
	{
		int iBank = pInstrument->MIDIBank;
		int iProg = pInstrument->MIDIProgram;

		if( iBank == iBankSelected && iProg == iProgSelected )
		{
			QString name = QString::fromStdString( pInstrument->pInfo->Name );

			if( name == "" )
			{
				name = "<no name>";
			}

			return name;
		}

		pInstrument = m_instance->gig.GetNextInstrument();
	}

	return "";
}




// A key has been pressed
void GigInstrument::playNote( NotePlayHandle * _n, SampleFrame* )
{
	const float LOG440 = 2.643452676f;

	int midiNote = (int) floor( 12.0 * ( log2( _n->unpitchedFrequency() ) - LOG440 ) - 4.0 );

	// out of range?
	if( midiNote <= 0 || midiNote >= 128 )
	{
		return;
	}

	if (!_n->m_pluginData)
	{
		auto pluginData = new GIGPluginData;
		pluginData->midiNote = midiNote;
		_n->m_pluginData = pluginData;

		const int baseVelocity = instrumentTrack()->midiPort()->baseVelocity();
		const uint velocity = _n->midiVelocity( baseVelocity );

		QMutexLocker locker( &m_notesMutex );
		m_notes.push_back( GigNote( midiNote, velocity, _n->unpitchedFrequency(), pluginData ) );
	}
}




// Process the notes and output a certain number of frames (e.g. 256, set in
// the preferences)
void GigInstrument::play( SampleFrame* _working_buffer )
{
	const fpp_t frames = Engine::audioEngine()->framesPerPeriod();
	const auto rate = Engine::audioEngine()->outputSampleRate();

	// Initialize to zeros
	std::memset( &_working_buffer[0][0], 0, DEFAULT_CHANNELS * frames * sizeof( float ) );

	m_synthMutex.lock();
	m_notesMutex.lock();

	if( m_instance == nullptr || m_instrument == nullptr )
	{
		m_synthMutex.unlock();
		m_notesMutex.unlock();
		return;
	}

	for( QList<GigNote>::iterator it = m_notes.begin(); it != m_notes.end(); ++it )
	{
		// Process notes in the KeyUp state, adding release samples if desired
		if( it->state == GigState::KeyUp )
		{
			// If there are no samples, we're done
			if( it->samples.empty() )
			{
				it->state = GigState::Completed;
			}
			else
			{
				it->state = GigState::PlayingKeyUp;

				// Notify each sample that the key has been released
				for (auto& sample : it->samples)
				{
					sample.adsr.keyup();
				}

				// Add release samples if available
				if( it->release == true )
				{
					addSamples( *it, true );
				}
			}
		}
		// Process notes in the KeyDown state, adding samples for the notes
		else if( it->state == GigState::KeyDown )
		{
			it->state = GigState::PlayingKeyDown;
			addSamples( *it, false );
		}

		// Delete ended samples
		for( QList<GigSample>::iterator sample = it->samples.begin();
				sample != it->samples.end(); ++sample )
		{
			// Delete if the ADSR for a sample is complete for normal
			// notes, or if a release sample, then if we've reached
			// the end of the sample
			if( sample->sample == nullptr || sample->adsr.done() ||
				( it->isRelease == true &&
				  sample->pos >= sample->sample->SamplesTotal - 1 ) )
			{
				sample = it->samples.erase( sample );

				if( sample == it->samples.end() )
				{
					break;
				}
			}
		}

		// Delete ended notes (either in the completed state or all the samples ended)
		if( it->state == GigState::Completed || it->samples.empty() )
		{
			it = m_notes.erase( it );

			if( it == m_notes.end() )
			{
				break;
			}
		}
	}

	// Fill buffer with portions of the note samples
	for (auto& note : m_notes)
	{
		// Only process the notes if we're in a playing state
		if (!(note.state == GigState::PlayingKeyDown || note.state == GigState::PlayingKeyUp ))
		{
			continue;
		}

		for (auto& sample : note.samples)
		{
			if (sample.sample == nullptr || sample.region == nullptr) { continue; }

			// Will change if resampling
			bool resample = false;
			f_cnt_t samples = frames; // How many to grab
			f_cnt_t used = frames; // How many we used
			float freq_factor = 1.0; // How to resample

			// Resample to be the correct pitch when the sample provided isn't
			// solely for this one note (e.g. one or two samples per octave) or
			// we are processing at a different sample rate
			if (sample.region->PitchTrack == true || rate != sample.sample->SamplesPerSecond)
			{
				resample = true;

				// Factor just for resampling
				freq_factor = 1.0 * rate / sample.sample->SamplesPerSecond;

				// Factor for pitch shifting as well as resampling
				if (sample.region->PitchTrack == true) { freq_factor *= sample.freqFactor; }

				// We need a bit of margin so we don't get glitching
				samples = frames / freq_factor + Sample::s_interpolationMargins[m_interpolation];
			}

			// Load this note's data
			SampleFrame sampleData[samples];
			loadSample(sample, sampleData, samples);

			// Apply ADSR using a copy so if we don't use these samples when
			// resampling, the ADSR doesn't get messed up
			ADSR copy = sample.adsr;

			for( f_cnt_t i = 0; i < samples; ++i )
			{
				float amplitude = copy.value();
				sampleData[i][0] *= amplitude;
				sampleData[i][1] *= amplitude;
			}

			// Output the data resampling if needed
			if( resample == true )
			{
				SampleFrame convertBuf[frames];

				// Only output if resampling is successful (note that "used" is output)
				if (sample.convertSampleRate(*sampleData, *convertBuf, samples, frames, freq_factor, used))
				{
					for( f_cnt_t i = 0; i < frames; ++i )
					{
						_working_buffer[i][0] += convertBuf[i][0];
						_working_buffer[i][1] += convertBuf[i][1];
					}
				}
			}
			else
			{
				for( f_cnt_t i = 0; i < frames; ++i )
				{
					_working_buffer[i][0] += sampleData[i][0];
					_working_buffer[i][1] += sampleData[i][1];
				}
			}

			// Update note position with how many samples we actually used
			sample.pos += used;
			sample.adsr.inc(used);
		}
	}

	m_notesMutex.unlock();
	m_synthMutex.unlock();

	// Set gain properly based on volume control
	for( f_cnt_t i = 0; i < frames; ++i )
	{
		_working_buffer[i][0] *= m_gain.value();
		_working_buffer[i][1] *= m_gain.value();
	}
}




void GigInstrument::loadSample( GigSample& sample, SampleFrame* sampleData, f_cnt_t samples )
{
	if( sampleData == nullptr || samples < 1 )
	{
		return;
	}

	// Determine if we need to loop part of this sample
	bool loop = false;
	gig::loop_type_t loopType = gig::loop_type_normal;
	f_cnt_t loopStart = 0;
	f_cnt_t loopLength = 0;

	if( sample.region->pSampleLoops != nullptr )
	{
		for( uint32_t i = 0; i < sample.region->SampleLoops; ++i )
		{
			loop = true;
			loopType = static_cast<gig::loop_type_t>( sample.region->pSampleLoops[i].LoopType );
			loopStart = sample.region->pSampleLoops[i].LoopStart;
			loopLength = sample.region->pSampleLoops[i].LoopLength;

			// Currently only support at max one loop
			break;
		}
	}

	unsigned long allocationsize = samples * sample.sample->FrameSize;
	int8_t buffer[allocationsize];

	// Load the sample in different ways depending on if we're looping or not
	if( loop == true && ( sample.pos >= loopStart || sample.pos + samples > loopStart ) )
	{
		// Calculate the new position based on the type of loop
		if( loopType == gig::loop_type_bidirectional )
		{
			sample.pos = getPingPongIndex( sample.pos, loopStart, loopStart + loopLength );
		}
		else
		{
			sample.pos = getLoopedIndex( sample.pos, loopStart, loopStart + loopLength );
			// TODO: also implement loop_type_backward support
		}

		sample.sample->SetPos( sample.pos );

		// Load the samples (based on gig::Sample::ReadAndLoop) even around the end
		// of a loop boundary wrapping to the beginning of the loop region
		long samplestoread = samples;
		long samplestoloopend = 0;
		long readsamples = 0;
		long totalreadsamples = 0;
		long loopEnd = loopStart + loopLength;

		do
		{
			samplestoloopend = loopEnd - sample.sample->GetPos();
			readsamples = sample.sample->Read( &buffer[totalreadsamples * sample.sample->FrameSize],
					std::min( samplestoread, samplestoloopend ) );
			samplestoread -= readsamples;
			totalreadsamples += readsamples;

			if( readsamples >= samplestoloopend )
			{
				sample.sample->SetPos( loopStart );
			}
		}
		while( samplestoread > 0 && readsamples > 0 );
	}
	else
	{
		sample.sample->SetPos( sample.pos );

		unsigned long size = sample.sample->Read( &buffer, samples ) * sample.sample->FrameSize;
		std::memset( (int8_t*) &buffer + size, 0, allocationsize - size );
	}

	// Convert from 16 or 24 bit into 32-bit float
	if( sample.sample->BitDepth == 24 ) // 24 bit
	{
		auto pInt = reinterpret_cast<uint8_t*>(&buffer);

		for( f_cnt_t i = 0; i < samples; ++i )
		{
			// libgig gives 24-bit data as little endian, so we must
			// convert if on a big endian system
			int32_t valueLeft = swap32IfBE(
						( pInt[ 3 * sample.sample->Channels * i ] << 8 ) |
						( pInt[ 3 * sample.sample->Channels * i + 1 ] << 16 ) |
						( pInt[ 3 * sample.sample->Channels * i + 2 ] << 24 ) );

			// Store the notes to this buffer before saving to output
			// so we can fade them out as needed
			sampleData[i][0] = 1.0 / 0x100000000 * sample.attenuation * valueLeft;

			if( sample.sample->Channels == 1 )
			{
				sampleData[i][1] = sampleData[i][0];
			}
			else
			{
				int32_t valueRight = swap32IfBE(
							( pInt[ 3 * sample.sample->Channels * i + 3 ] << 8 ) |
							( pInt[ 3 * sample.sample->Channels * i + 4 ] << 16 ) |
							( pInt[ 3 * sample.sample->Channels * i + 5 ] << 24 ) );

				sampleData[i][1] = 1.0 / 0x100000000 * sample.attenuation * valueRight;
			}
		}
	}
	else // 16 bit
	{
		auto pInt = reinterpret_cast<int16_t*>(&buffer);

		for( f_cnt_t i = 0; i < samples; ++i )
		{
			sampleData[i][0] = 1.0 / 0x10000 *
				pInt[ sample.sample->Channels * i ] * sample.attenuation;

			if( sample.sample->Channels == 1 )
			{
				sampleData[i][1] = sampleData[i][0];
			}
			else
			{
				sampleData[i][1] = 1.0 / 0x10000 *
					pInt[ sample.sample->Channels * i + 1 ] * sample.attenuation;
			}
		}
	}
}




// These two loop index functions taken from SampleBuffer.cpp
f_cnt_t GigInstrument::getLoopedIndex( f_cnt_t index, f_cnt_t startf, f_cnt_t endf ) const
{
	if( index < endf )
	{
		return index;
	}

	return startf + ( index - startf )
				% ( endf - startf );
}




f_cnt_t GigInstrument::getPingPongIndex( f_cnt_t index, f_cnt_t startf, f_cnt_t endf ) const
{
	if( index < endf )
	{
		return index;
	}

	const f_cnt_t looplen = endf - startf;
	const f_cnt_t looppos = ( index - endf ) % ( looplen * 2 );

	return ( looppos < looplen )
		? endf - looppos
		: startf + ( looppos - looplen );
}




// A key has been released
void GigInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	auto pluginData = static_cast<GIGPluginData*>(_n->m_pluginData);
	QMutexLocker locker( &m_notesMutex );

	// Mark the note as being released, but only if it was playing or was just
	// pressed (i.e., not if the key was already released)
	for (auto& note : m_notes)
	{
		// Find the note by matching pointers to the plugin data
		if (note.handle == pluginData && (note.state == GigState::KeyDown || note.state == GigState::PlayingKeyDown))
		{
			note.state = GigState::KeyUp;
		}
	}

	// TODO: not sample exact? What about in the middle of us writing out the sample?

	delete pluginData;
}




gui::PluginView* GigInstrument::instantiateView( QWidget * _parent )
{
	return new gui::GigInstrumentView( this, _parent );
}




// Add the desired samples (either the normal samples or the release samples)
// to the GigNote
//
// Note: not thread safe since libgig stores current region position data in
// the instrument object
void GigInstrument::addSamples( GigNote & gignote, bool wantReleaseSample )
{
	// Change key dimension, e.g. change samples based on what key is pressed
	// in a certain range. From LinuxSampler
	if( wantReleaseSample == true &&
			gignote.midiNote >= m_instrument->DimensionKeyRange.low &&
			gignote.midiNote <= m_instrument->DimensionKeyRange.high )
	{
		m_currentKeyDimension = float( gignote.midiNote -
				m_instrument->DimensionKeyRange.low ) / (
					m_instrument->DimensionKeyRange.high -
					m_instrument->DimensionKeyRange.low + 1 );
	}

	gig::Region* pRegion = m_instrument->GetFirstRegion();

	while( pRegion != nullptr )
	{
		Dimension dim = getDimensions( pRegion, gignote.velocity, wantReleaseSample );
		gig::DimensionRegion * pDimRegion = pRegion->GetDimensionRegionByValue( dim.DimValues );
		gig::Sample * pSample = pDimRegion->pSample;

		// If this is a release sample, the note won't ever be
		// released, so we handle it differently
		gignote.isRelease = wantReleaseSample;

		// Does this note have release samples? Set this only on the original
		// notes and not when we get the release samples.
		if( wantReleaseSample != true )
		{
			gignote.release = dim.release;
		}

		if( pSample != nullptr && pSample->SamplesTotal != 0 )
		{
			int keyLow = pRegion->KeyRange.low;
			int keyHigh = pRegion->KeyRange.high;

			if( gignote.midiNote >= keyLow && gignote.midiNote <= keyHigh )
			{
				float attenuation = pDimRegion->GetVelocityAttenuation( gignote.velocity );
				float length = (float) pSample->SamplesTotal / Engine::audioEngine()->outputSampleRate();

				// TODO: sample panning? crossfade different layers?

				if( wantReleaseSample == true )
				{
					// From LinuxSampler, not sure how it was created
					attenuation *= 1 - 0.01053 * ( 256 >> pDimRegion->ReleaseTriggerDecay ) * length;
				}
				else
				{
					attenuation *= pDimRegion->SampleAttenuation;
				}

				gignote.samples.push_back( GigSample( pSample, pDimRegion,
							attenuation, m_interpolation, gignote.frequency ) );
			}
		}

		pRegion = m_instrument->GetNextRegion();
	}
}




// Based on our input parameters, generate a "dimension" that specifies which
// note we wish to select from the GIG file with libgig. libgig will use this
// information to select the sample.
Dimension GigInstrument::getDimensions( gig::Region * pRegion, int velocity, bool release )
{
	Dimension dim;

	if( pRegion == nullptr )
	{
		return dim;
	}

	for( int i = pRegion->Dimensions - 1; i >= 0; --i )
	{
		switch( pRegion->pDimensionDefinitions[i].dimension )
		{
			case gig::dimension_layer:
				// TODO: implement this
				dim.DimValues[i] = 0;
				break;
			case gig::dimension_velocity:
				dim.DimValues[i] = velocity;
				break;
			case gig::dimension_releasetrigger:
				dim.release = true;
				dim.DimValues[i] = (uint) release;
				break;
			case gig::dimension_keyboard:
				dim.DimValues[i] = (uint) ( m_currentKeyDimension * pRegion->pDimensionDefinitions[i].zones );
				break;
			case gig::dimension_roundrobin:
			case gig::dimension_roundrobinkeyboard:
				// TODO: implement this
				dim.DimValues[i] = 0;
				break;
			case gig::dimension_random:
				// From LinuxSampler, untested
				m_RandomSeed = m_RandomSeed * 1103515245 + 12345;
				dim.DimValues[i] = uint(
					m_RandomSeed / 4294967296.0f * pRegion->pDimensionDefinitions[i].bits );
				break;
			case gig::dimension_samplechannel:
			case gig::dimension_channelaftertouch:
			case gig::dimension_modwheel:
			case gig::dimension_breath:
			case gig::dimension_foot:
			case gig::dimension_portamentotime:
			case gig::dimension_effect1:
			case gig::dimension_effect2:
			case gig::dimension_genpurpose1:
			case gig::dimension_genpurpose2:
			case gig::dimension_genpurpose3:
			case gig::dimension_genpurpose4:
			case gig::dimension_sustainpedal:
			case gig::dimension_portamento:
			case gig::dimension_sostenutopedal:
			case gig::dimension_softpedal:
			case gig::dimension_genpurpose5:
			case gig::dimension_genpurpose6:
			case gig::dimension_genpurpose7:
			case gig::dimension_genpurpose8:
			case gig::dimension_effect1depth:
			case gig::dimension_effect2depth:
			case gig::dimension_effect3depth:
			case gig::dimension_effect4depth:
			case gig::dimension_effect5depth:
			case gig::dimension_none:
			default:
				dim.DimValues[i] = 0;
				break;
		}
	}

	return dim;
}




// Get the selected instrument from the GIG file we opened if we haven't gotten
// it already. This is based on the bank and patch numbers.
void GigInstrument::getInstrument()
{
	// Find instrument
	int iBankSelected = m_bankNum.value();
	int iProgSelected = m_patchNum.value();

	QMutexLocker locker( &m_synthMutex );

	if( m_instance != nullptr )
	{
		gig::Instrument * pInstrument = m_instance->gig.GetFirstInstrument();

		while( pInstrument != nullptr )
		{
			int iBank = pInstrument->MIDIBank;
			int iProg = pInstrument->MIDIProgram;

			if( iBank == iBankSelected && iProg == iProgSelected )
			{
				break;
			}

			pInstrument = m_instance->gig.GetNextInstrument();
		}

		m_instrument = pInstrument;
	}
}




// Since the sample rate changes when we start an export, clear all the
// currently-playing notes when we get this signal. Then, the export won't
// include leftover notes that were playing in the program.
void GigInstrument::updateSampleRate()
{
	QMutexLocker locker( &m_notesMutex );
	m_notes.clear();
}



namespace gui
{


class gigKnob : public Knob
{
public:
	gigKnob( QWidget * _parent ) :
			Knob( KnobType::Bright26, _parent )
	{
		setFixedSize( 31, 38 );
	}
} ;




GigInstrumentView::GigInstrumentView( Instrument * _instrument, QWidget * _parent ) :
        InstrumentViewFixedSize( _instrument, _parent )
{
	auto k = castModel<GigInstrument>();

	connect( &k->m_bankNum, SIGNAL( dataChanged() ), this, SLOT( updatePatchName() ) );
	connect( &k->m_patchNum, SIGNAL( dataChanged() ), this, SLOT( updatePatchName() ) );

	// File Button
	m_fileDialogButton = new PixmapButton( this );
	m_fileDialogButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_fileDialogButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileselect_on" ) );
	m_fileDialogButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileselect_off" ) );
	m_fileDialogButton->move( 223, 68 );

	connect( m_fileDialogButton, SIGNAL( clicked() ), this, SLOT( showFileDialog() ) );

	m_fileDialogButton->setToolTip(tr("Open GIG file"));

	// Patch Button
	m_patchDialogButton = new PixmapButton( this );
	m_patchDialogButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_patchDialogButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "patches_on" ) );
	m_patchDialogButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "patches_off" ) );
	m_patchDialogButton->setEnabled( false );
	m_patchDialogButton->move( 223, 94 );

	connect( m_patchDialogButton, SIGNAL( clicked() ), this, SLOT( showPatchDialog() ) );

	m_patchDialogButton->setToolTip(tr("Choose patch"));

	// LCDs
	m_bankNumLcd = new LcdSpinBox( 3, "21pink", this );
	m_bankNumLcd->move( 111, 150 );

	m_patchNumLcd = new LcdSpinBox( 3, "21pink", this );
	m_patchNumLcd->move( 161, 150 );

	// Next row
	m_filenameLabel = new QLabel( this );
	m_filenameLabel->setGeometry( 61, 70, 156, 14 );
	m_patchLabel = new QLabel( this );
	m_patchLabel->setGeometry( 61, 94, 156, 14 );

	// Gain
	m_gainKnob = new gigKnob( this );
	m_gainKnob->setHintText( tr( "Gain:" ) + " ", "" );
	m_gainKnob->move( 32, 140 );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	updateFilename();
}




void GigInstrumentView::modelChanged()
{
	auto k = castModel<GigInstrument>();
	m_bankNumLcd->setModel( &k->m_bankNum );
	m_patchNumLcd->setModel( &k->m_patchNum );

	m_gainKnob->setModel( &k->m_gain );

	connect( k, SIGNAL( fileChanged() ), this, SLOT( updateFilename() ) );
	connect( k, SIGNAL( fileLoading() ), this, SLOT( invalidateFile() ) );

	updateFilename();
}




void GigInstrumentView::updateFilename()
{
	auto i = castModel<GigInstrument>();
	QFontMetrics fm( m_filenameLabel->font() );
	QString file = i->m_filename.endsWith( ".gig", Qt::CaseInsensitive ) ?
			i->m_filename.left( i->m_filename.length() - 4 ) :
			i->m_filename;
	m_filenameLabel->setText( fm.elidedText( file, Qt::ElideLeft, m_filenameLabel->width() ) );

	m_patchDialogButton->setEnabled( !i->m_filename.isEmpty() );

	updatePatchName();
	update();
}




void GigInstrumentView::updatePatchName()
{
	auto i = castModel<GigInstrument>();
	QFontMetrics fm( font() );
	QString patch = i->getCurrentPatchName();
	m_patchLabel->setText( fm.elidedText( patch, Qt::ElideLeft, m_patchLabel->width() ) );

	update();
}




void GigInstrumentView::invalidateFile()
{
	m_patchDialogButton->setEnabled( false );
}




void GigInstrumentView::showFileDialog()
{
	auto k = castModel<GigInstrument>();

	FileDialog ofd( nullptr, tr( "Open GIG file" ) );
	ofd.setFileMode( FileDialog::ExistingFiles );

	QStringList types;
	types << tr( "GIG Files (*.gig)" );
	ofd.setNameFilters( types );

	if( k->m_filename != "" )
	{
		QString f = PathUtil::toAbsolute( k->m_filename );
		ofd.setDirectory( QFileInfo( f ).absolutePath() );
		ofd.selectFile( QFileInfo( f ).fileName() );
	}
	else
	{
		ofd.setDirectory( ConfigManager::inst()->gigDir() );
	}

	m_fileDialogButton->setEnabled( false );

	if( ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		QString f = ofd.selectedFiles()[0];

		if( f != "" )
		{
			k->openFile( f );
			Engine::getSong()->setModified();
		}
	}

	m_fileDialogButton->setEnabled( true );
}




void GigInstrumentView::showPatchDialog()
{
	auto k = castModel<GigInstrument>();
	PatchesDialog pd( this );
	pd.setup( k->m_instance, 1, k->instrumentTrack()->name(), &k->m_bankNum, &k->m_patchNum, m_patchLabel );
	pd.exec();
}


} // namespace gui


// Store information related to playing a sample from the GIG file
GigSample::GigSample( gig::Sample * pSample, gig::DimensionRegion * pDimRegion,
		float attenuation, int interpolation, float desiredFreq )
	: sample( pSample ), region( pDimRegion ), attenuation( attenuation ),
	  pos( 0 ), interpolation( interpolation ), srcState( nullptr ),
	  sampleFreq( 0 ), freqFactor( 1 )
{
	if( sample != nullptr && region != nullptr )
	{
		// Note: we don't create the libsamplerate object here since we always
		// also call the copy constructor when appending to the end of the
		// QList. We'll create it only in the copy constructor so we only have
		// to create it once.

		// Calculate note pitch and frequency factor only if we're actually
		// going to be changing the pitch of the notes
		if( region->PitchTrack == true )
		{
			// Calculate what frequency the provided sample is
			sampleFreq = 440.0f * std::exp2((region->UnityNote - 69 - region->FineTune * 0.01) / 12.0f);
			freqFactor = sampleFreq / desiredFreq;
		}

		// The sample rate we pass in is affected by how we are going to be
		// resampling the note so that a 1.5 second release ends up being 1.5
		// seconds after resampling
		adsr = ADSR( region, sample->SamplesPerSecond / freqFactor );
	}
}




GigSample::~GigSample()
{
	if( srcState != nullptr )
	{
		src_delete( srcState );
	}
}




GigSample::GigSample( const GigSample& g )
	: sample( g.sample ), region( g.region ), attenuation( g.attenuation ),
	  adsr( g.adsr ), pos( g.pos ), interpolation( g.interpolation ),
	  srcState( nullptr ), sampleFreq( g.sampleFreq ), freqFactor( g.freqFactor )
{
	// On the copy, we want to create the object
	updateSampleRate();
}




GigSample& GigSample::operator=( const GigSample& g )
{
	sample = g.sample;
	region= g.region;
	attenuation = g.attenuation;
	adsr = g.adsr;
	pos = g.pos;
	interpolation = g.interpolation;
	srcState = nullptr;
	sampleFreq = g.sampleFreq;
	freqFactor = g.freqFactor;

	if( g.srcState != nullptr )
	{
		updateSampleRate();
	}

	return *this;
}




void GigSample::updateSampleRate()
{
	if( srcState != nullptr )
	{
		src_delete( srcState );
	}

	int error = 0;
	srcState = src_new( interpolation, DEFAULT_CHANNELS, &error );

	if( srcState == nullptr || error != 0 )
	{
		qCritical( "error while creating libsamplerate data structure in GigSample" );
	}
}




bool GigSample::convertSampleRate( SampleFrame & oldBuf, SampleFrame & newBuf,
		f_cnt_t oldSize, f_cnt_t newSize, float freq_factor, f_cnt_t& used )
{
	if( srcState == nullptr )
	{
		return false;
	}

	SRC_DATA src_data;
	src_data.data_in = &oldBuf[0];
	src_data.data_out = &newBuf[0];
	src_data.input_frames = oldSize;
	src_data.output_frames = newSize;
	src_data.src_ratio = freq_factor;
	src_data.end_of_input = 0;

	// We don't need to lock this assuming that we're only outputting the
	// samples in one thread
	int error = src_process( srcState, &src_data );

	used = src_data.input_frames_used;

	if( error != 0 )
	{
		qCritical( "GigInstrument: error while resampling: %s", src_strerror( error ) );
		return false;
	}

	if( oldSize != 0 && src_data.output_frames_gen == 0 )
	{
		qCritical( "GigInstrument: could not resample, no frames generated" );
		return false;
	}

	if (src_data.output_frames_gen > 0 && static_cast<f_cnt_t>(src_data.output_frames_gen) < newSize)
	{
		qCritical() << "GigInstrument: not enough frames, wanted"
			<< newSize << "generated" << src_data.output_frames_gen;
		return false;
	}

	return true;
}




ADSR::ADSR()
	: preattack( 0 ), attack( 0 ), decay1( 0 ), decay2( 0 ), infiniteSustain( false ),
	  sustain( 0 ), release( 0 ),
	  amplitude( 0 ), isAttack( true ), isRelease( false ), isDone( false ),
	  attackPosition( 0 ), attackLength( 0 ), decayLength( 0 ),
	  releasePosition( 0 ), releaseLength( 0 )
{
}




// Create the ADSR envelope from the settings in the GIG file
ADSR::ADSR( gig::DimensionRegion * region, int sampleRate )
	: preattack( 0 ), attack( 0 ), decay1( 0 ), decay2( 0 ), infiniteSustain( false ),
	  sustain( 0 ), release( 0 ),
	  amplitude( 0 ), isAttack( true ), isRelease( false ), isDone( false ),
	  attackPosition( 0 ), attackLength( 0 ), decayLength( 0 ),
	  releasePosition( 0 ), releaseLength( 0 )
{
	if( region != nullptr )
	{
		// Parameters from GIG file
		preattack = 1.0 * region->EG1PreAttack / 1000; // EG1PreAttack is 0-1000 permille
		attack = region->EG1Attack;
		decay1 = region->EG1Decay1;
		decay2 = region->EG1Decay2;
		infiniteSustain = region->EG1InfiniteSustain;
		sustain = 1.0 * region->EG1Sustain / 1000; // EG1Sustain is 0-1000 permille
		release = region->EG1Release;

		// Simple ADSR using positions in sample
		amplitude = preattack;
		attackLength = attack * sampleRate;
		decayLength = decay1 * sampleRate; // TODO: ignoring decay2 for now
		releaseLength = release * sampleRate;

		// If there is no attack or decay, start at the sustain amplitude
		if( attackLength == 0 && decayLength == 0 )
		{
			amplitude = sustain;
		}
		// If there is no attack, start at the full amplitude
		else if( attackLength == 0 )
		{
			amplitude = 1.0;
		}
	}
}




// Next time we get the amplitude, we'll be releasing the note
void ADSR::keyup()
{
	isRelease = true;
}




// Can we delete the sample now?
bool ADSR::done()
{
	return isDone;
}




// Return the current amplitude and increment internal positions
float ADSR::value()
{
	float currentAmplitude = amplitude;

	// If we're done, don't output any signal
	if( isDone == true )
	{
		return 0;
	}
	// If we're still in the attack phase, release from the current volume
	// instead of jumping to the sustain volume and fading out
	else if( isAttack == true && isRelease == true )
	{
		sustain = amplitude;
		isAttack = false;
	}

	// If we're in the attack phase, start at the preattack amplitude and
	// increase to the full before decreasing to sustain
	if( isAttack == true )
	{
		if( attackPosition < attackLength )
		{
			amplitude = preattack + ( 1.0 - preattack ) / attackLength * attackPosition;
		}
		else if( attackPosition < attackLength + decayLength )
		{
			amplitude = 1.0 - ( 1.0 - sustain ) / decayLength * ( attackPosition - attackLength );
		}
		else
		{
			isAttack = false;
		}

		++attackPosition;
	}
	// If we're in the sustain phase, decrease from sustain to zero
	else if( isRelease == true )
	{
		// Maybe not the best way of doing this, but it appears to be about right
		// Satisfies f(0) = sustain and f(releaseLength) = very small
		amplitude = (sustain + 1e-3) * std::exp(-5.0f / releaseLength * releasePosition) - 1e-3;

		// Don't have an infinite exponential decay
		if( amplitude <= 0 || releasePosition >= releaseLength )
		{
			amplitude = 0;
			isDone = true;
		}

		++releasePosition;
	}

	return currentAmplitude;
}




// Increment internal positions a certain number of times
void ADSR::inc( f_cnt_t num )
{
	for( f_cnt_t i = 0; i < num; ++i )
	{
		value();
	}
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return new GigInstrument( static_cast<InstrumentTrack *>( m ) );
}

}


} // namespace lmms
