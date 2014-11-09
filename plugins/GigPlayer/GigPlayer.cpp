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

#include <cstring>
#include <QDebug>
#include <QLayout>
#include <QLabel>
#include <QDomDocument>

#include "FileDialog.h"
#include "GigPlayer.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "InstrumentPlayHandle.h"
#include "NotePlayHandle.h"
#include "knob.h"
#include "song.h"
#include "ConfigManager.h"

#include "PatchesDialog.h"
#include "tooltip.h"
#include "LcdSpinBox.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT gigplayer_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"GIG Player",
	QT_TRANSLATE_NOOP( "pluginBrowser", "Player for GIG files" ),
	"Garrett Wilson <g/at/floft/dot/net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	"gig",
	NULL
} ;

}




// Margins for extra samples to provide to libsamplerate to reduce glitching
const f_cnt_t GIGMARGIN[] = { 128, 64, 64, 4, 4 };




struct GIGPluginData
{
	int midiNote;
} ;




GigInstrument::GigInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &gigplayer_plugin_descriptor ),
	m_srcState( NULL ),
	m_instance( NULL ),
	m_instrument( NULL ),
	m_filename( "" ),
	m_bankNum( 0, 0, 999, this, tr( "Bank" ) ),
	m_patchNum( 0, 0, 127, this, tr( "Patch" ) ),
	m_gain( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Gain" ) ),
	m_sampleData( NULL ),
	m_sampleDataSize( 0 ),
	m_RandomSeed( 0 ),
	m_currentKeyDimension( 0 )
{
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( iph );

	updateSampleRate();

	connect( &m_bankNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	connect( &m_patchNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	connect( engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ) );

	// Buffer for reading samples
	const fpp_t frames = engine::mixer()->framesPerPeriod();
	m_sampleData = new sampleFrame[frames];
	m_sampleDataSize = frames;
}




GigInstrument::~GigInstrument()
{
	if( m_sampleData != NULL )
	{
		delete[] m_sampleData;
	}

	engine::mixer()->removePlayHandles( instrumentTrack() );
	freeInstance();

	if( m_srcState != NULL )
	{
		src_delete( m_srcState );
	}
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

	return NULL;
}




QString GigInstrument::nodeName() const
{
	return gigplayer_plugin_descriptor.name;
}




void GigInstrument::freeInstance()
{
	QMutexLocker synthLock( &m_synthMutex );
	QMutexLocker notesLock( &m_notesMutex );

	if( m_instance != NULL )
	{
		delete m_instance;
		m_instance = NULL;

		// If we're changing instruments, we got to make sure that we
		// remove all pointers to the old samples and don't try accessing
		// that instrument again
		m_instrument = NULL;
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
			m_instance = new GigInstance( _gigFile );
			m_filename = SampleBuffer::tryToMakeRelative( _gigFile );
		}
		catch( ... )
		{
			m_instance = NULL;
			m_filename = "";
		}
	}

	emit fileChanged();

	if( updateTrackName == true )
	{
		instrumentTrack()->setName( QFileInfo( _gigFile ).baseName() );
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

	if( m_instance == NULL )
	{
		return "";
	}

	int iBankSelected = m_bankNum.value();
	int iProgSelected = m_patchNum.value();

	gig::Instrument * pInstrument = m_instance->gig.GetFirstInstrument();

	while( pInstrument != NULL )
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
void GigInstrument::playNote( NotePlayHandle * _n, sampleFrame * )
{
	const float LOG440 = 2.643452676f;

	const f_cnt_t tfp = _n->totalFramesPlayed();

	int midiNote = (int) floor( 12.0 * ( log2( _n->unpitchedFrequency() ) - LOG440 ) - 4.0 );

	// out of range?
	if( midiNote <= 0 || midiNote >= 128 )
	{
		return;
	}

	if( tfp == 0 )
	{
		GIGPluginData * pluginData = new GIGPluginData;
		pluginData->midiNote = midiNote;
		_n->m_pluginData = pluginData;

		const int baseVelocity = instrumentTrack()->midiPort()->baseVelocity();
		const uint velocity = _n->midiVelocity( baseVelocity );

		QMutexLocker locker( &m_notesMutex );
		m_notes.push_back( GigNote( midiNote, velocity ) );
	}
}




// Process the notes and output a certain number of frames (e.g. 256, set in
// the preferences)
void GigInstrument::play( sampleFrame * _working_buffer )
{
	const fpp_t frames = engine::mixer()->framesPerPeriod();
	const int interpolation = engine::mixer()->currentQualitySettings().libsrcInterpolation();

	// Initialize to zeros
	std::memset( &_working_buffer[0][0], 0, DEFAULT_CHANNELS * frames * sizeof( float ) );

	m_synthMutex.lock();
	m_notesMutex.lock();

	if( m_instance == NULL || m_instrument == NULL )
	{
		m_synthMutex.unlock();
		m_notesMutex.unlock();
		return;
	}

	// Data for converting sample rate
	int oldRate = -1;
	int newRate = engine::mixer()->processingSampleRate();
	bool sampleError = false;
	bool sampleConvert = false;
	sampleFrame * convertBuf = NULL;

	// How many frames we'll be grabbing from the sample
	int samples = frames;

	// How many we actually used from the sample
	int used = frames;

	for( QList<GigNote>::iterator it = m_notes.begin(); it != m_notes.end(); ++it )
	{
		// Process notes in the KeyUp state, adding release samples if desired
		if( it->state == KeyUp )
		{
			// If there are no samples, we're done
			if( it->samples.empty() )
			{
				it->state = Completed;
			}
			else
			{
				it->state = PlayingKeyUp;

				// Notify each sample that the key has been released
				for( QList<GigSample>::iterator sample = it->samples.begin();
						sample != it->samples.end(); ++sample )
				{
					sample->adsr.keyup();
				}

				// Add release samples if available
				if( it->release == true )
				{
					addSamples( *it, true );
				}
			}
		}
		// Process notes in the KeyDown state, adding samples for the notes
		else if( it->state == KeyDown )
		{
			it->state = PlayingKeyDown;
			addSamples( *it, false );
		}

		for( QList<GigSample>::iterator sample = it->samples.begin();
				sample != it->samples.end(); ++sample )
		{
			// Delete ended samples
			if( sample->sample == NULL || sample->adsr.done() ||
					sample->pos >= sample->sample->SamplesTotal - 1 )
			{
				sample = it->samples.erase( sample );

				if( sample == it->samples.end() )
				{
					break;
				}
			}
			// Verify all the samples have the same rate
			else
			{
				int currentRate = sample->sample->SamplesPerSecond;

				if( oldRate == -1 )
				{
					oldRate = currentRate;
				}
				else if( oldRate != currentRate )
				{
					qCritical() << "GigInstrument: not all samples are the same rate, not converting";
					sampleError = true;
				}
			}
		}

		// Delete ended notes (either in the completed state or all the samples ended)
		if( it->state == Completed || it->samples.empty() )
		{
			it = m_notes.erase( it );

			if( it == m_notes.end() )
			{
				break;
			}
		}
	}

	// If all samples have the same sample rate and it's not the output sample
	// rate, then we'll convert sample rates
	if( oldRate != -1 && sampleError != true && oldRate != newRate )
	{
		sampleConvert = true;

		// Read a different number of samples depending on the sample rate, but
		// resample it to always output the right number of frames
		samples = frames * oldRate / newRate;

		// We need a bit of margin so we don't get glitching
		samples += GIGMARGIN[interpolation];

		// Buffer for the resampled data
		convertBuf = new sampleFrame[samples];
		std::memset( &convertBuf[0][0], 0, DEFAULT_CHANNELS * samples * sizeof( float ) );
	}

	// Recreate buffer if it is of a different size
	if( m_sampleDataSize != samples )
	{
		delete[] m_sampleData;
		m_sampleData = new sampleFrame[samples];
		m_sampleDataSize = samples;
	}

	// Fill buffer with portions of the note samples
	for( QList<GigNote>::iterator it = m_notes.begin(); it != m_notes.end(); ++it )
	{
		// Only process the notes if we're in a playing state
		if( !( it->state == PlayingKeyDown ||
				it->state == PlayingKeyUp ) )
		{
			continue;
		}

		for( QList<GigSample>::iterator sample = it->samples.begin();
				sample != it->samples.end(); ++sample )
		{
			if( sample->sample == NULL )
			{
				continue;
			}

			// Set the position to where we currently are in the sample
			sample->sample->SetPos( sample->pos ); // Note: not thread safe

			// Load the next portion of the sample
			gig::buffer_t buf;
			unsigned long allocationsize = samples * sample->sample->FrameSize;
			buf.pStart = new int8_t[allocationsize];
			buf.Size = sample->sample->Read( buf.pStart, samples ) * sample->sample->FrameSize;
			buf.NullExtensionSize = allocationsize - buf.Size;
			std::memset( (int8_t*) buf.pStart + buf.Size, 0, buf.NullExtensionSize );

			// Convert from 16 or 24 bit into 32-bit float
			if( sample->sample->BitDepth == 24 ) // 24 bit
			{
				uint8_t * pInt = static_cast<uint8_t*>( buf.pStart );

				for( int i = 0; i < samples; ++i )
				{
					int32_t valueLeft = ( pInt[ 3 * sample->sample->Channels * i ] << 8 ) |
								( pInt[ 3 * sample->sample->Channels * i + 1 ] << 16 ) |
								( pInt[ 3 * sample->sample->Channels * i + 2 ] << 24 );

					// Store the notes to this buffer before saving to output
					// so we can fade them out as needed
					m_sampleData[i][0] = 1.0 / 0x100000000 * sample->attenuation * valueLeft;

					if( sample->sample->Channels == 1 )
					{
						m_sampleData[i][1] = m_sampleData[i][0];
					}
					else
					{
						int32_t valueRight = ( pInt[ 3 * sample->sample->Channels * i + 3 ] << 8 ) |
									( pInt[ 3 * sample->sample->Channels * i + 4 ] << 16 ) |
									( pInt[ 3 * sample->sample->Channels * i + 5 ] << 24 );

						m_sampleData[i][1] = 1.0 / 0x100000000 * sample->attenuation * valueRight;
					}
				}
			}
			else // 16 bit
			{
				int16_t * pInt = static_cast<int16_t*>( buf.pStart );

				for( int i = 0; i < samples; ++i )
				{
					m_sampleData[i][0] = 1.0 / 0x10000 *
						pInt[ sample->sample->Channels * i ] * sample->attenuation;

					if( sample->sample->Channels == 1 )
					{
						m_sampleData[i][1] = m_sampleData[i][0];
					}
					else
					{
						m_sampleData[i][1] = 1.0 / 0x10000 *
							pInt[ sample->sample->Channels * i + 1 ] * sample->attenuation;
					}
				}
			}

			// Cleanup
			delete[] (int8_t*) buf.pStart;

			// Apply ADSR using a copy so if we don't use these samples when
			// resampling, the ADSR doesn't get messed up
			ADSR copy = sample->adsr;

			for( int i = 0; i < samples; ++i )
			{
				double amplitude = copy.value();
				m_sampleData[i][0] *= amplitude;
				m_sampleData[i][1] *= amplitude;
			}

			// Save to output buffer
			if( sampleConvert == true )
			{
				for( int i = 0; i < samples; ++i )
				{
					convertBuf[i][0] += m_sampleData[i][0];
					convertBuf[i][1] += m_sampleData[i][1];
				}
			}
			else
			{
				for( int i = 0; i < frames; ++i )
				{
					_working_buffer[i][0] += m_sampleData[i][0];
					_working_buffer[i][1] += m_sampleData[i][1];
				}
			}
		}
	}

	// Convert sample rate if needed
	if( sampleConvert == true )
	{
		// If an error occurred, it's better to render nothing than have some
		// screeching high-volume noise
		if( !convertSampleRate( *convertBuf, *_working_buffer, samples, frames,
					oldRate, newRate, used ) )
		{
			std::memset( &_working_buffer[0][0], 0,
					DEFAULT_CHANNELS * frames * sizeof( float ) );
		}

		delete[] convertBuf;
	}

	// Update the note positions with how many samples we actually used
	for( QList<GigNote>::iterator it = m_notes.begin(); it != m_notes.end(); ++it )
	{
		// Only process the notes if we're in a playing state
		if( !( it->state == PlayingKeyDown ||
				it->state == PlayingKeyUp ) )
		{
			continue;
		}

		for( QList<GigSample>::iterator sample = it->samples.begin();
				sample != it->samples.end(); ++sample )
		{
			if( sample->sample != NULL )
			{
				sample->pos += used;
				sample->adsr.inc( used );
			}
		}
	}

	m_notesMutex.unlock();
	m_synthMutex.unlock();

	// Set gain properly based on volume control
	for( int i = 0; i < frames; ++i)
	{
		_working_buffer[i][0] *= m_gain.value();
		_working_buffer[i][1] *= m_gain.value();
	}

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, NULL );
}




// A key has been released
void GigInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	GIGPluginData * pluginData = static_cast<GIGPluginData *>( _n->m_pluginData );
	QMutexLocker locker( &m_notesMutex );

	// Mark the note as being released, but only if it was playing or was just
	// pressed (i.e., not if the key was already released)
	for( QList<GigNote>::iterator i = m_notes.begin(); i != m_notes.end(); ++i )
	{
		if( i->midiNote == pluginData->midiNote &&
				( i->state == KeyDown || i->state == PlayingKeyDown ) )
		{
			i->state = KeyUp;
		}
	}

	// TODO: not sample exact? What about in the middle of us writing out the sample?

	delete pluginData;
}




PluginView * GigInstrument::instantiateView( QWidget * _parent )
{
	return new GigInstrumentView( this, _parent );
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

	while( pRegion != NULL )
	{
		Dimension dim = getDimensions( pRegion, gignote.velocity, wantReleaseSample );
		gig::DimensionRegion * pDimRegion = pRegion->GetDimensionRegionByValue( dim.DimValues );
		gig::Sample * pSample = pDimRegion->pSample;

		// Does this note have release samples? Set this only on the original
		// notes and not when we get the release samples.
		if( wantReleaseSample != true )
		{
			gignote.release = dim.release;
		}

		if( pSample != NULL && pSample->SamplesTotal != 0 )
		{
			int keyLow = pRegion->KeyRange.low;
			int keyHigh = pRegion->KeyRange.high;

			if( gignote.midiNote >= keyLow && gignote.midiNote <= keyHigh )
			{
				float attenuation = pDimRegion->GetVelocityAttenuation( gignote.velocity );;
				float length = (double) pSample->SamplesTotal / engine::mixer()->processingSampleRate();

				// TODO: sample panning? looping? crossfade different layers?

				if( wantReleaseSample == true )
				{
					// From LinuxSampler, not sure how it was created
					attenuation *= 1 - 0.01053 * ( 256 >> pDimRegion->ReleaseTriggerDecay ) * length;
				}
				else
				{
					attenuation *= pDimRegion->SampleAttenuation;
				}

				gignote.samples.push_back( GigSample( pSample, attenuation,
					ADSR( pDimRegion, pSample->SamplesPerSecond ) ) );
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

	if( pRegion == NULL )
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

	if( m_instance != NULL )
	{
		gig::Instrument * pInstrument = m_instance->gig.GetFirstInstrument();

		while( pInstrument != NULL )
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




bool GigInstrument::convertSampleRate( sampleFrame & oldBuf, sampleFrame & newBuf,
		int oldSize, int newSize, int oldRate, int newRate, int& used )
{
	SRC_DATA src_data;
	src_data.data_in = &oldBuf[0];
	src_data.data_out = &newBuf[0];
	src_data.input_frames = oldSize;
	src_data.output_frames = newSize;
	src_data.src_ratio = (double) newRate / oldRate;
	src_data.end_of_input = 0;

	m_srcMutex.lock();
	int error = src_process( m_srcState, &src_data );
	m_srcMutex.unlock();

	used = src_data.input_frames_used;

	if( error != 0 )
	{
		qCritical( "GigInstrument: error while resampling: %s", src_strerror( error ) );
		return false;
	}

	if( src_data.output_frames_gen > newSize )
	{
		qCritical( "GigInstrument: not enough frames: %ld / %d", src_data.output_frames_gen, newSize );
		return false;
	}

	if( src_data.output_frames_gen == 0 )
	{
		qCritical( "GigInstrument: could not resample, no frames generated" );
		return false;
	}

	return true;
}




void GigInstrument::updateSampleRate()
{
	QMutexLocker locker( &m_srcMutex );

	if( m_srcState != NULL )
	{
		src_delete( m_srcState );
	}

	int error;
	m_srcState = src_new( engine::mixer()->currentQualitySettings().libsrcInterpolation(),
		DEFAULT_CHANNELS, &error );

	if( m_srcState == NULL || error != 0 )
	{
		qCritical( "error while creating libsamplerate data structure in GigInstrument::updateSampleRate()" );
	}
}




class gigKnob : public knob
{
public:
	gigKnob( QWidget * _parent ) :
			knob( knobBright_26, _parent )
	{
		setFixedSize( 31, 38 );
	}
} ;




GigInstrumentView::GigInstrumentView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	GigInstrument * k = castModel<GigInstrument>();

	connect( &k->m_bankNum, SIGNAL( dataChanged() ), this, SLOT( updatePatchName() ) );
	connect( &k->m_patchNum, SIGNAL( dataChanged() ), this, SLOT( updatePatchName() ) );

	// File Button
	m_fileDialogButton = new pixmapButton( this );
	m_fileDialogButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_fileDialogButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileselect_on" ) );
	m_fileDialogButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileselect_off" ) );
	m_fileDialogButton->move( 223, 68 );

	connect( m_fileDialogButton, SIGNAL( clicked() ), this, SLOT( showFileDialog() ) );

	toolTip::add( m_fileDialogButton, tr( "Open other GIG file" ) );

	m_fileDialogButton->setWhatsThis( tr( "Click here to open another GIG file" ) );

	// Patch Button
	m_patchDialogButton = new pixmapButton( this );
	m_patchDialogButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_patchDialogButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "patches_on" ) );
	m_patchDialogButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "patches_off" ) );
	m_patchDialogButton->setEnabled( false );
	m_patchDialogButton->move( 223, 94 );

	connect( m_patchDialogButton, SIGNAL( clicked() ), this, SLOT( showPatchDialog() ) );

	toolTip::add( m_patchDialogButton, tr( "Choose the patch" ) );

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
	m_gainKnob->setHintText( tr( "Gain" ) + " ", "" );
	m_gainKnob->move( 32, 140 );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	updateFilename();

}




GigInstrumentView::~GigInstrumentView()
{
}




void GigInstrumentView::modelChanged()
{
	GigInstrument * k = castModel<GigInstrument>();
	m_bankNumLcd->setModel( &k->m_bankNum );
	m_patchNumLcd->setModel( &k->m_patchNum );

	m_gainKnob->setModel( &k->m_gain );

	connect( k, SIGNAL( fileChanged() ), this, SLOT( updateFilename() ) );
	connect( k, SIGNAL( fileLoading() ), this, SLOT( invalidateFile() ) );

	updateFilename();
}




void GigInstrumentView::updateFilename()
{
	GigInstrument * i = castModel<GigInstrument>();
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
	GigInstrument * i = castModel<GigInstrument>();
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
	GigInstrument * k = castModel<GigInstrument>();

	FileDialog ofd( NULL, tr( "Open GIG file" ) );
	ofd.setFileMode( FileDialog::ExistingFiles );

	QStringList types;
	types << tr( "GIG Files (*.gig)" );
	ofd.setFilters( types );

	QString dir;
	if( k->m_filename != "" )
	{
		QString f = k->m_filename;
		if( QFileInfo( f ).isRelative() )
		{
			f = ConfigManager::inst()->userSamplesDir() + f;
			if( QFileInfo( f ).exists() == false )
			{
				f = ConfigManager::inst()->factorySamplesDir() + k->m_filename;
			}
		}
		ofd.setDirectory( QFileInfo( f ).absolutePath() );
		ofd.selectFile( QFileInfo( f ).fileName() );
	}
	else
	{
		ofd.setDirectory( ConfigManager::inst()->userSamplesDir() );
	}

	m_fileDialogButton->setEnabled( false );

	if( ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		QString f = ofd.selectedFiles()[0];
		if( f != "" )
		{
			k->openFile( f );
			engine::getSong()->setModified();
		}
	}

	m_fileDialogButton->setEnabled( true );
}




void GigInstrumentView::showPatchDialog()
{
	GigInstrument * k = castModel<GigInstrument>();
	PatchesDialog pd( this );
	pd.setup( k->m_instance, 1, k->instrumentTrack()->name(), &k->m_bankNum, &k->m_patchNum, m_patchLabel );
	pd.exec();
}




// Store information related to playing a sample from the GIG file
GigSample::GigSample( gig::Sample * pSample, float attenuation, const ADSR & adsr )
	: sample( pSample ),
	  attenuation( attenuation ),
	  adsr( adsr ),
	  pos( 0 )
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
	if( region != NULL )
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
double ADSR::value()
{
	double currentAmplitude = amplitude;

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
			amplitude = preattack + ( attack - preattack ) / attackLength * attackPosition;
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
		if( releasePosition < releaseLength )
		{
			amplitude = sustain * ( 1.0 - 1.0 / releaseLength * releasePosition );
		}
		else
		{
			isDone = true;
		}

		++releasePosition;
	}

	return currentAmplitude;
}

// Increment internal positions a certain number of times
void ADSR::inc( int num )
{
	for( int i = 0; i < num; ++i )
	{
		value();
	}
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new GigInstrument( static_cast<InstrumentTrack *>( _data ) );
}

}
