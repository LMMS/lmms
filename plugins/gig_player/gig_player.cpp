/*
 * gig_player.cpp - a gig player using libgig (based on sf2 player plugin)
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * Based partially on some code from LinuxSampler (also GPL):
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
#include "gig_player.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "InstrumentPlayHandle.h"
#include "NotePlayHandle.h"
#include "knob.h"
#include "song.h"

#include "patches_dialog.h"
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


struct GIGPluginData
{
	int midiNote;
	int lastPanning;
	float lastVelocity;
} ;



// Static map of current GIG instances
QMap<QString, gigInstance*> gigInstrument::s_instances;
QMutex gigInstrument::s_instancesMutex;



gigInstrument::gigInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &gigplayer_plugin_descriptor ),
	m_srcState( NULL ),
	m_instance( NULL ),
	m_instrument( NULL ),
	m_RandomSeed( 0 ),
	m_currentKeyDimension( 0 ),
	m_filename( "" ),
	m_lastMidiPitch( -1 ),
	m_lastMidiPitchRange( -1 ),
	m_channel( 1 ),
	m_bankNum( 0, 0, 999, this, tr("Bank") ),
	m_patchNum( 0, 0, 127, this, tr("Patch") ),
	m_gain( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Gain" ) ),
	m_noteData( NULL ),
	m_noteDataSize( 0 )
{
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( iph );

	updateSampleRate();

	connect( &m_bankNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	connect( &m_patchNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	connect( engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ) );

	// Buffer for reading samples
	const fpp_t frames = engine::mixer()->framesPerPeriod();
	m_noteData = new sampleFrame[frames];
	m_noteDataSize = frames;
}



gigInstrument::~gigInstrument()
{
	if( m_noteData )
		delete[] m_noteData;

	engine::mixer()->removePlayHandles( instrumentTrack() );
	freeInstance();

	if( m_srcState != NULL )
		src_delete( m_srcState );
}



void gigInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "src", m_filename );
	m_patchNum.saveSettings( _doc, _this, "patch" );
	m_bankNum.saveSettings( _doc, _this, "bank" );

	m_gain.saveSettings( _doc, _this, "gain" );
}




void gigInstrument::loadSettings( const QDomElement & _this )
{
	openFile( _this.attribute( "src" ), false );
	m_patchNum.loadSettings( _this, "patch" );
	m_bankNum.loadSettings( _this, "bank" );

	m_gain.loadSettings( _this, "gain" );

	updatePatch();
}




void gigInstrument::loadFile( const QString & _file )
{
	if( !_file.isEmpty() && QFileInfo( _file ).exists() )
	{
		openFile( _file, false );
		updatePatch();
		updateSampleRate();
	}
}




AutomatableModel * gigInstrument::childModel( const QString & _modelName )
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



QString gigInstrument::nodeName() const
{
	return gigplayer_plugin_descriptor.name;
}




void gigInstrument::freeInstance()
{
	m_synthMutex.lock();

	if ( m_instance != NULL )
	{
		s_instancesMutex.lock();
		--(m_instance->refCount);

		// No more references
		if( m_instance->refCount <= 0 )
		{
			s_instances.remove( m_filename );
			qDebug() << "gigInstrument: deleting instance of" << m_filename;
			delete m_instance;
		}
		else
		{
			qDebug() << "gigInstrument: decreasing reference count for" << m_filename;
		}

		s_instancesMutex.unlock();
		m_instance = NULL;
	}

	m_synthMutex.unlock();
}



void gigInstrument::openFile( const QString & _gigFile, bool updateTrackName )
{
	emit fileLoading();
	bool succeeded = false;

	// Used for loading file
	char * gigAscii = qstrdup( qPrintable( SampleBuffer::tryToMakeAbsolute( _gigFile ) ) );
	QString relativePath = SampleBuffer::tryToMakeRelative( _gigFile );

	// free reference to gig file if one is selected
	freeInstance();

	m_synthMutex.lock();
	s_instancesMutex.lock();

	// Increment Reference
	if( s_instances.contains( relativePath ) )
	{
		m_instance = s_instances[ relativePath ];
		m_instance->refCount++;
		qDebug() << "gigInstrument: increasing reference count for" << relativePath;
		succeeded = true;
	}

	// Add to map, if doesn't exist.
	else
	{
		try
		{
			// Grab this sf from the top of the stack and add to list
			m_instance = new gigInstance( _gigFile );
			s_instances.insert( relativePath, m_instance );
			succeeded = true;
			qDebug() << "gigInstrument: adding instance of" << relativePath;
		}
		catch( ... )
		{
			m_instance = NULL;
			succeeded = false;
		}
	}

	s_instancesMutex.unlock();
	m_synthMutex.unlock();

	if( succeeded )
		m_filename = relativePath;
	else
		m_filename = "";

	emit fileChanged();

	delete[] gigAscii;

	if( updateTrackName )
	{
		instrumentTrack()->setName( QFileInfo( _gigFile ).baseName() );
		updatePatch();
	}
}




void gigInstrument::updatePatch()
{
	if( m_bankNum.value() >= 0 && m_patchNum.value() >= 0 )
		getInstrument();
}




QString gigInstrument::getCurrentPatchName()
{
	m_synthMutex.lock();

	if( !m_instance )
	{
		m_synthMutex.unlock();
		return "";
	}

	int iBankSelected = m_bankNum.value();
	int iProgSelected = m_patchNum.value();

	gig::Instrument* pInstrument = m_instance->gig.GetFirstInstrument();

	while( pInstrument )
	{
		int iBank = pInstrument->MIDIBank;
		int iProg = pInstrument->MIDIProgram;

		if( iBank == iBankSelected && iProg == iProgSelected )
		{
			QString name = QString::fromStdString(pInstrument->pInfo->Name);

			if( name == "" )
				name = "<no name>";

			m_synthMutex.unlock();
			return name;
		}

		pInstrument = m_instance->gig.GetNextInstrument();
	}

	m_synthMutex.unlock();
	return "";
}



void gigInstrument::playNote( NotePlayHandle * _n, sampleFrame * )
{
	const float LOG440 = 2.643452676f;

	const f_cnt_t tfp = _n->totalFramesPlayed();

	int midiNote = (int)floor( 12.0 * ( log2( _n->unpitchedFrequency() ) - LOG440 ) - 4.0 );

	// out of range?
	if( midiNote <= 0 || midiNote >= 128 )
	{
		return;
	}

	if( tfp == 0 )
	{
		GIGPluginData * pluginData = new GIGPluginData;
		pluginData->midiNote = midiNote;
		pluginData->lastPanning = -1;
		pluginData->lastVelocity = 127;
		_n->m_pluginData = pluginData;

		if( m_instance )
		{
			if( !m_instrument )
				getInstrument();

			const int baseVelocity = instrumentTrack()->midiPort()->baseVelocity();
			const uint velocity = _n->midiVelocity( baseVelocity );
			addNotes( midiNote, velocity, false );
		}
	}
}




// Could we get iph-based instruments support sample-exact models by using a
// frame-length of 1 while rendering?
void gigInstrument::play( sampleFrame * _working_buffer )
{
	const fpp_t frames = engine::mixer()->framesPerPeriod();

	// Initialize to zeros
	std::memset(&_working_buffer[0][0], 0, 2*frames*sizeof(float)); // *2 for channels

	// Determine if we need to convert sample rates
	int oldRate = -1;
	int newRate = engine::mixer()->processingSampleRate();
	bool sampleError = false;
	bool sampleConvert = false;
	sampleFrame* convertBuf = NULL;

	// How many frames we'll be grabbing from the sample
	int samples = frames;

	m_notesMutex.lock();

	// Verify all the samples have the same rate
	for( std::list<gigNote>::iterator note = m_notes.begin(); note != m_notes.end(); ++note )
	{
		if( note->sample )
		{
			int currentRate = note->sample->SamplesPerSecond;

			if (oldRate == -1)
			{
				oldRate = currentRate;
			}
			else if (oldRate != currentRate)
			{
				qCritical() << "gigInstrument: not all samples are the same rate, not converting";
				sampleError = true;
			}
		}
	}

	if (oldRate != -1 && !sampleError && oldRate != newRate)
	{
		sampleConvert = true;

		// At a higher sample rate, we'll go output the samples slightly
		// faster while maintaining the same note
		samples = frames*oldRate/newRate;

		// Buffer for the resampled data
		convertBuf = new sampleFrame[samples];
		std::memset(&convertBuf[0][0], 0, 2*samples*sizeof(float)); // *2 for channels
	}

	// Recreate buffer if it is of a different size
	if (m_noteDataSize != samples)
	{
		delete[] m_noteData;
		m_noteData = new sampleFrame[samples];
		m_noteDataSize = samples;
	}

	// Fill with portions of the note samples
	for( std::list<gigNote>::iterator note = m_notes.begin(); note != m_notes.end(); ++note )
	{
		if( note->sample )
		{
			// Set the position to where we currently are in the sample
			note->sample->SetPos(note->pos); // Note: not thread safe

			// Load the next portion of the sample
			gig::buffer_t buf;
			unsigned long allocationsize = samples * note->sample->FrameSize;
			buf.pStart = new int8_t[allocationsize];
			buf.Size = note->sample->Read(buf.pStart, samples) * note->sample->FrameSize;
			buf.NullExtensionSize = allocationsize - buf.Size;
			std::memset((int8_t*)buf.pStart + buf.Size, 0, buf.NullExtensionSize);

			// Save the new position in the sample
			note->pos = note->sample->GetPos();

			// Convert from 16 or 24 bit into 32-bit float
			if( note->sample->BitDepth == 24 ) // 24 bit
			{
				uint8_t* pInt = static_cast<uint8_t*>( buf.pStart );

				for( int i = 0; i < samples; ++i )
				{
					int32_t valueLeft = (pInt[3*note->sample->Channels*i]<<8) |
								(pInt[3*note->sample->Channels*i+1]<<16) |
								(pInt[3*note->sample->Channels*i+2]<<24);

					// Store the notes to this buffer before saving to output
					// so we can fade them out as needed
					m_noteData[i][0] = 1.0/0x100000000*note->attenuation*valueLeft;

					if( note->sample->Channels == 1 )
					{
						m_noteData[i][1] = m_noteData[i][0];
					}
					else
					{
						int32_t valueRight = (pInt[3*note->sample->Channels*i+3]<<8) |
									(pInt[3*note->sample->Channels*i+4]<<16) |
									(pInt[3*note->sample->Channels*i+5]<<24);

						m_noteData[i][1] = 1.0/0x100000000*note->attenuation*valueRight;
					}
				}
			}
			else // 16 bit
			{
				int16_t* pInt = static_cast<int16_t*>( buf.pStart );

				for( int i = 0; i < samples; ++i )
				{
					m_noteData[i][0] = 1.0/0x10000*pInt[note->sample->Channels*i] * note->attenuation;

					if( note->sample->Channels == 1 )
						m_noteData[i][1] = m_noteData[i][0];
					else
						m_noteData[i][1] = 1.0/0x10000*pInt[note->sample->Channels*i+1] * note->attenuation;
				}
			}

			// Cleanup
			delete[] (int8_t*)buf.pStart;

			// Fade out note and if it ends before our buffer ends, set the
			// rest of the samples to zero
			if( note->fadeOut )
			{
				int i = 0;

				for( int pos = note->fadeOutPos; i < samples && pos < note->fadeOutLen; ++i, ++pos )
				{
					m_noteData[i][0] *= 1.0-1.0/note->fadeOutLen*note->fadeOutPos;
					m_noteData[i][1] *= 1.0-1.0/note->fadeOutLen*note->fadeOutPos;
				}

				for( ; i < samples; ++i )
				{
					m_noteData[i][0] = 0;
					m_noteData[i][1] = 0;
				}

				// We fade out [0, samples) but we only want to advance the
				// fade out as if we rendered out [0, frames) to base the fade
				// out on actual time instead of the time in the sample
				note->fadeOutPos += frames;
			}

			// Save to output buffer
			if( sampleConvert )
			{
				for( int i = 0; i < samples; ++i )
				{
					convertBuf[i][0] += m_noteData[i][0];
					convertBuf[i][1] += m_noteData[i][1];
				}
			}
			else
			{
				for( int i = 0; i < frames; ++i )
				{
					_working_buffer[i][0] += m_noteData[i][0];
					_working_buffer[i][1] += m_noteData[i][1];
				}
			}
		}
	}

	m_notesMutex.unlock();

	// Convert sample rate if needed
	if( sampleConvert )
	{
		convertSampleRate(*convertBuf, *_working_buffer, samples, frames, oldRate, newRate);
		delete[] convertBuf;
	}

	// Set gain properly based on volume control
	for( int i = 0; i < frames; ++i)
	{
		_working_buffer[i][0] *= m_gain.value();
		_working_buffer[i][1] *= m_gain.value();
	}

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, NULL );

	m_notesMutex.lock();

	// Delete ended notes
	for( std::list<gigNote>::iterator note = m_notes.begin(); note != m_notes.end(); ++note )
	{
		if( (note->fadeOut && note->fadeOutPos >= note->fadeOutLen-1) ||
				note->pos >= note->sample->SamplesTotal-1 )
		{
			note = m_notes.erase(note);

			if (note == m_notes.end())
				break;
		}
	}

	m_notesMutex.unlock();
}




void gigInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	GIGPluginData * pluginData = static_cast<GIGPluginData *>( _n->m_pluginData );
	m_notesMutex.lock();

	// Is the note supposed to have a release sample played?
	bool noteRelease = false;

	// Fade out the note we want to end
	// TODO: implement proper ADSR from GIG specs
	for( std::list<gigNote>::iterator i = m_notes.begin(); i != m_notes.end(); ++i )
	{
		if( i->midiNote == pluginData->midiNote && !i->fadeOut )
		{
			noteRelease = i->release;

			float fadeOut = i->releaseTime;
			int samples = i->sample->SamplesTotal;
			int len = std::min( int( std::floor( fadeOut * engine::mixer()->processingSampleRate() ) ),
					samples - i->pos );

			// TODO: not sample exact? What about in the middle of us writing out the sample?

			i->fadeOutPos = 0;
			i->fadeOutLen = len;
			i->fadeOut = true;

			if (len < 0)
				continue;
		}
	}

	m_notesMutex.unlock();

	if( noteRelease )
	{
		// Add the release notes if available
		const int baseVelocity = instrumentTrack()->midiPort()->baseVelocity();
		const uint velocity = _n->midiVelocity( baseVelocity );
		addNotes( pluginData->midiNote , velocity, true );
	}

	delete pluginData;
}




PluginView * gigInstrument::instantiateView( QWidget * _parent )
{
	return new gigInstrumentView( this, _parent );
}




Dimension gigInstrument::getDimensions( gig::Region* pRegion, int velocity, bool release )
{
	Dimension dim;

	if( !pRegion )
		return dim;

	for( int i = pRegion->Dimensions - 1; i >= 0; --i )
	{
		switch( pRegion->pDimensionDefinitions[i].dimension )
		{
			case gig::dimension_layer:
				//DimValues[i] = iLayer;
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
				dim.DimValues[i] = (uint) (m_currentKeyDimension * pRegion->pDimensionDefinitions[i].zones);
				break;
			case gig::dimension_roundrobin:
			case gig::dimension_roundrobinkeyboard:
				//dim.DimValues[i] = (uint) pEngineChannel->pMIDIKeyInfo[MIDIKey].RoundRobinIndex; // incremented for each note on
				dim.DimValues[i] = 0;
				// TODO: implement this
				break;
			case gig::dimension_random:
				m_RandomSeed   = m_RandomSeed * 1103515245 + 12345; // classic pseudo random number generator
				dim.DimValues[i] = (uint) m_RandomSeed >> (32 - pRegion->pDimensionDefinitions[i].bits); // highest bits are most random
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
				dim.DimValues[i] = 0;
				break;
			default:
				qDebug() << "gigInstrument: Unknown dimension";
				dim.DimValues[i] = 0;
		}
	}

	return dim;
}




void gigInstrument::getInstrument()
{
	// Find instrument
	int iBankSelected = m_bankNum.value();
	int iProgSelected = m_patchNum.value();

	m_synthMutex.lock();

	if( m_instance )
	{
		gig::Instrument* pInstrument = m_instance->gig.GetFirstInstrument();

		while( pInstrument )
		{
			int iBank = pInstrument->MIDIBank;
			int iProg = pInstrument->MIDIProgram;

			if( iBank == iBankSelected && iProg == iProgSelected )
				break;

			pInstrument = m_instance->gig.GetNextInstrument();
		}

		m_instrument = pInstrument;
	}

	m_synthMutex.unlock();
}



bool gigInstrument::convertSampleRate( sampleFrame& oldBuf, sampleFrame& newBuf,
		int oldSize, int newSize, int oldRate, int newRate )
{
	SRC_DATA src_data;
	src_data.data_in = &oldBuf[0];
	src_data.data_out = &newBuf[0];
	src_data.input_frames = oldSize;
	src_data.output_frames = newSize;
	src_data.src_ratio = (double) newSize / oldSize;
	src_data.end_of_input = 0;

	m_srcMutex.lock();
	int error = src_process( m_srcState, &src_data );
	m_srcMutex.unlock();

	if( error )
	{
		qCritical( "gigInstrument: error while resampling: %s", src_strerror( error ) );
		return false;
	}

	if( src_data.output_frames_gen > newSize )
	{
		qCritical( "gigInstrument: not enough frames: %ld / %d", src_data.output_frames_gen, newSize );
		return false;
	}

	return true;
}




void gigInstrument::updateSampleRate()
{
	m_srcMutex.lock();

	if( m_srcState != NULL )
		src_delete( m_srcState );

	int error;
	m_srcState = src_new( engine::mixer()->currentQualitySettings().libsrcInterpolation(), DEFAULT_CHANNELS, &error );

	if( m_srcState == NULL || error )
		qCritical( "error while creating libsamplerate data structure in gigInstrument::updateSampleRate()" );

	m_srcMutex.unlock();
}



// Find the sample we want to play (based on velocity, etc.)
void gigInstrument::addNotes( int midiNote, int velocity, bool release )
{
	if( m_instrument )
	{
		// Change key dimension, e.g. change samples based on what key is pressed
		// in a certain range.
		if( midiNote >= m_instrument->DimensionKeyRange.low && midiNote
				<= m_instrument->DimensionKeyRange.high )
			m_currentKeyDimension = float( midiNote -
					m_instrument->DimensionKeyRange.low ) / (
						m_instrument->DimensionKeyRange.high -
						m_instrument->DimensionKeyRange.low + 1 );

		m_synthMutex.lock();
		m_notesMutex.lock();

		gig::Region* pRegion = m_instrument->GetFirstRegion();

		while( pRegion )
		{
			Dimension dim = getDimensions( pRegion, velocity, release );
			gig::DimensionRegion* pDimRegion = pRegion->GetDimensionRegionByValue( dim.DimValues );
			gig::Sample* pSample = pDimRegion->pSample;

			if( pSample && pDimRegion->pSample->SamplesTotal != 0 )
			{
				int keyLow = pRegion->KeyRange.low;
				int keyHigh = pRegion->KeyRange.high;

				if( midiNote >= keyLow && midiNote <= keyHigh )
				{
					float attenuation = pDimRegion->GetVelocityAttenuation( velocity );;
					float length = (double) pSample->SamplesTotal / engine::mixer()->processingSampleRate();

					// TODO: sample panning? looping? crossfade different layers?

					if (release)
						attenuation *= 1 - 0.01053 * (256 >> pDimRegion->ReleaseTriggerDecay) * length;
					else
						attenuation *= pDimRegion->SampleAttenuation;

					m_notes.push_back(gigNote(pSample, midiNote, attenuation, dim.release,
						pDimRegion->EG1Release));

					break; // Only grab one sample to play
				}
			}

			pRegion = m_instrument->GetNextRegion();
		}

		m_notesMutex.unlock();
		m_synthMutex.unlock();
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
};



gigInstrumentView::gigInstrumentView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	gigInstrument* k = castModel<gigInstrument>();

	connect( &k->m_bankNum, SIGNAL( dataChanged() ), this, SLOT( updatePatchName() ) );
	connect( &k->m_patchNum, SIGNAL( dataChanged() ), this, SLOT( updatePatchName() ) );

	// File Button
	m_fileDialogButton = new pixmapButton( this );
	m_fileDialogButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_fileDialogButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fileselect_on" ) );
	m_fileDialogButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fileselect_off" ) );
	m_fileDialogButton->move( 217, 107 );

	connect( m_fileDialogButton, SIGNAL( clicked() ), this, SLOT( showFileDialog() ) );

	toolTip::add( m_fileDialogButton, tr( "Open other GIG file" ) );

	m_fileDialogButton->setWhatsThis( tr( "Click here to open another GIG file" ) );

	// Patch Button
	m_patchDialogButton = new pixmapButton( this );
	m_patchDialogButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_patchDialogButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "patches_on" ) );
	m_patchDialogButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "patches_off" ) );
	m_patchDialogButton->setEnabled( false );
	m_patchDialogButton->move( 217, 125 );

	connect( m_patchDialogButton, SIGNAL( clicked() ), this, SLOT( showPatchDialog() ) );

	toolTip::add( m_patchDialogButton, tr( "Choose the patch" ) );

	// LCDs
	m_bankNumLcd = new LcdSpinBox( 3, "21pink", this );
	m_bankNumLcd->move(131, 62);

	m_patchNumLcd = new LcdSpinBox( 3, "21pink", this );
	m_patchNumLcd->move(190, 62);

	// Next row
	m_filenameLabel = new QLabel( this );
	m_filenameLabel->setGeometry( 58, 109, 156, 11 );
	m_patchLabel = new QLabel( this );
	m_patchLabel->setGeometry( 58, 127, 156, 11 );

	// Gain
	m_gainKnob = new gigKnob( this );
	m_gainKnob->setHintText( tr("Gain") + " ", "" );
	m_gainKnob->move( 86, 55 );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	updateFilename();

}




gigInstrumentView::~gigInstrumentView()
{
}





void gigInstrumentView::modelChanged()
{
	gigInstrument * k = castModel<gigInstrument>();
	m_bankNumLcd->setModel( &k->m_bankNum );
	m_patchNumLcd->setModel( &k->m_patchNum );

	m_gainKnob->setModel( &k->m_gain );

	connect( k, SIGNAL( fileChanged() ), this, SLOT( updateFilename() ) );
	connect( k, SIGNAL( fileLoading() ), this, SLOT( invalidateFile() ) );

	updateFilename();
}




void gigInstrumentView::updateFilename()
{
	gigInstrument * i = castModel<gigInstrument>();
	QFontMetrics fm( m_filenameLabel->font() );
	QString file = i->m_filename.endsWith( ".gig", Qt::CaseInsensitive ) ?
			i->m_filename.left( i->m_filename.length() - 4 ) :
			i->m_filename;
	m_filenameLabel->setText( fm.elidedText( file, Qt::ElideLeft, m_filenameLabel->width() ) );

	m_patchDialogButton->setEnabled( !i->m_filename.isEmpty() );

	updatePatchName();
	update();
}




void gigInstrumentView::updatePatchName()
{
	gigInstrument * i = castModel<gigInstrument>();
	QFontMetrics fm( font() );
	QString patch = i->getCurrentPatchName();
	m_patchLabel->setText( fm.elidedText( patch, Qt::ElideLeft, m_patchLabel->width() ) );

	update();
}




void gigInstrumentView::invalidateFile()
{
	m_patchDialogButton->setEnabled( false );
}




void gigInstrumentView::showFileDialog()
{
	gigInstrument * k = castModel<gigInstrument>();

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
			f = configManager::inst()->userSamplesDir() + f;
			if( QFileInfo( f ).exists() == false )
			{
				f = configManager::inst()->factorySamplesDir() + k->m_filename;
			}
		}
		ofd.setDirectory( QFileInfo( f ).absolutePath() );
		ofd.selectFile( QFileInfo( f ).fileName() );
	}
	else
	{
		ofd.setDirectory( configManager::inst()->userSamplesDir() );
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




void gigInstrumentView::showPatchDialog()
{
	gigInstrument * k = castModel<gigInstrument>();
	patchesDialog pd( this );
	pd.setup( k->m_instance, 1, k->instrumentTrack()->name(), &k->m_bankNum, &k->m_patchNum, m_patchLabel );
	pd.exec();
}



gigNote::gigNote() :
	sample( NULL ),
	midiNote( -1 ),
	attenuation( 1 ),
	release( false ),
	releaseTime( 0 ),
	pos( 0 ),
	fadeOut( false ),
	fadeOutPos( 0 ),
	fadeOutLen( 0 )
{
}

gigNote::gigNote( gig::Sample* pSample, int midiNote, float attenuation,
		bool release, float releaseTime ) :
	sample( pSample ),
	midiNote( midiNote ),
	attenuation( attenuation ),
	release( release ),
	releaseTime( releaseTime ),
	pos( 0 ),
	fadeOut( false ),
	fadeOutPos( 0 ),
	fadeOutLen( 0 )
{
}

gigNote::gigNote( const gigNote& g ) :
	sample( g.sample ),
	midiNote( g.midiNote ),
	attenuation( g.attenuation ),
	release( g.release ),
	releaseTime( g.releaseTime ),
	pos( g.pos ),
	fadeOut( g.fadeOut ),
	fadeOutPos( g.fadeOutPos ),
	fadeOutLen( g.fadeOutLen )
{
}

extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new gigInstrument( static_cast<InstrumentTrack *>( _data ) );
}

}
