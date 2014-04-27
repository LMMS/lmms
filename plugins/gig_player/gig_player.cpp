/*
 * gig_player.cpp - a gig player using libgig
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtCore/QDebug>
#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtXml/QDomDocument>
#include <math.h>  // sin, cos, ...
#include <cstring> // memset

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
	m_filename( "" ),
	m_lastMidiPitch( -1 ),
	m_lastMidiPitchRange( -1 ),
	m_channel( 1 ),
	m_bankNum( 0, 0, 999, this, tr("Bank") ),
	m_patchNum( 0, 0, 127, this, tr("Patch") ),
	m_gain( 1.0f, 0.0f, 5.0f, 0.01f, this, tr( "Gain" ) )
{
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
	engine::mixer()->addPlayHandle( iph );

	connect( &m_bankNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	connect( &m_patchNum, SIGNAL( dataChanged() ), this, SLOT( updatePatch() ) );
	//connect( engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ) );

	// Gain
	//connect( &m_gain, SIGNAL( dataChanged() ), this, SLOT( updateGain() ) );
}



gigInstrument::~gigInstrument()
{
	engine::mixer()->removePlayHandles( instrumentTrack() );
	freeInstance();

	if( m_srcState != NULL )
	{
		src_delete( m_srcState );
	}

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
			qDebug() << "Really deleting " << m_filename;

            // Need we do more to delete the gig instance?
			s_instances.remove( m_filename );
			delete m_instance;
		}
		// Just remove our reference
		else
		{
			qDebug() << "un-referencing " << m_filename;
		}
		s_instancesMutex.unlock();

		m_instance = NULL;
	}
	m_synthMutex.unlock();
}



void gigInstrument::openFile( const QString & _gigFile, bool updateTrackName )
{
	emit fileLoading();

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
		qDebug() << "Using existing reference to " << relativePath;

		m_instance = s_instances[ relativePath ];

		m_instance->refCount++;
	}

	// Add to map, if doesn't exist.
	else
	{
        // Grab this sf from the top of the stack and add to list
        m_instance = new gigInstance( _gigFile );
        s_instances.insert( relativePath, m_instance );
	}

	s_instancesMutex.unlock();
	m_synthMutex.unlock();

	// TODO: only do this if successfully loaded
	if( true )
	{
		m_filename = relativePath;

		emit fileChanged();
	}

	delete[] gigAscii;

	if( updateTrackName )
	{
		instrumentTrack()->setName( QFileInfo( _gigFile ).baseName() );
	}
}




void gigInstrument::updatePatch()
{
	if( m_bankNum.value() >= 0 && m_patchNum.value() >= 0 )
	{
        // TODO: are there patches in GIG files?
		//fluid_synth_program_select( m_synth, m_channel, m_fontId,
		//		m_bankNum.value(), m_patchNum.value() );
	}
}




QString gigInstrument::getCurrentPatchName()
{
	if (!m_instance)
		return "";

	int iBankSelected = m_bankNum.value();
	int iProgSelected = m_patchNum.value();

	gig::Instrument* pInstrument = m_instance->gig.GetFirstInstrument();
	while (pInstrument) {
		int iBank = pInstrument->MIDIBank;
		int iProg = pInstrument->MIDIProgram;

		if (iBank == iBankSelected && iProg == iProgSelected) {
			QString name = QString::fromStdString(pInstrument->pInfo->Name);

			if (name == "")
				name = "<no name>";

			return name;
		}

		pInstrument = m_instance->gig.GetNextInstrument();
	}

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

        // TODO: Start the note here
		//const int baseVelocity = instrumentTrack()->midiPort()->baseVelocity();
		//fluid_synth_noteon( m_synth, m_channel, midiNote, _n->midiVelocity( baseVelocity ) );

		if (m_instance)
		{
			// Find instrument
			int iBankSelected = m_bankNum.value();
			int iProgSelected = m_patchNum.value();

			gig::Instrument* pInstrument = m_instance->gig.GetFirstInstrument();
			while (pInstrument) {
				int iBank = pInstrument->MIDIBank;
				int iProg = pInstrument->MIDIProgram;

				if (iBank == iBankSelected && iProg == iProgSelected) {
					break;
				}

				pInstrument = m_instance->gig.GetNextInstrument();
			}

			// Find sample
			if (pInstrument)
			{
				gig::Region* pRegion = pInstrument->GetFirstRegion();

				while (pRegion) {
					gig::Sample* pSample = pRegion->GetSample();

					if (pSample)
					{
						int rate = pSample->SamplesPerSecond;
						QString name = QString::fromStdString(pSample->pInfo->Name);
						int keyLow = pRegion->KeyRange.low;
						int keyHigh = pRegion->KeyRange.high;

						if (rate != engine::mixer()->processingSampleRate())
						{
							qDebug() << "Warning: wrong sample rate, conversion not implemented";
						}

						if (midiNote >= keyLow && midiNote <= keyHigh)
						{
							/*qDebug() << "Playing note " << midiNote << " between " << keyLow
									 << " and " << keyHigh
									 << " from sample " << name << " at " << rate << " Hz of size "
									 << pSample->SamplesTotal;*/

							gig::buffer_t buf = pSample->LoadSampleData();
							gigNote note(pSample->SamplesTotal);

							if (pSample->BitDepth == 24)
							{
								qDebug() << "Error: not 16 bit... not implemented";
								/*int n = pSample->SamplesTotal * pSample->Channels;

								for (int i = n-1; i>=0; i-=pSample->Channels)
									for (int j = 0; j < pSample->Channels; ++j)
										note.note[i][j] =
											1.0/0x800000*(pWave[i*3+j]
												| pWave[i*3+1+3*j]<<8
												| pWave[i*3+2+3*j]<<16);*/
							}
							else
							{
								int16_t* pInt = reinterpret_cast<int16_t*>(buf.pStart);

								if (pSample->Channels <= 2)
								{
									for (int i = 0; i < pSample->SamplesTotal/pSample->Channels; ++i)
									{
										note.note[i][0] = 1.0/0x10000*pInt[pSample->Channels*i];

										if (pSample->Channels == 1)
											note.note[i][1] = note.note[i][0];
										else
											note.note[i][1] = 1.0/0x10000*pInt[pSample->Channels*i+1];
									}
								}
								else
								{
									qDebug() << "Error: not stereo... not implemented";
								}
							}

							m_synthMutex.lock();
							m_notes.push_back(note);
							m_synthMutex.unlock();

							pSample->ReleaseSampleData();
						}
					}

					pRegion = pInstrument->GetNextRegion();
				}
			}
		}
		else
		{
			// TODO: eventually get rid of this

			// By default just play a sine wave instead of a GIG file.
			double freq = _n->unpitchedFrequency();
			//int size = ceil(engine::mixer()->processingSampleRate()/freq);
			int size = 1.0/2*engine::mixer()->processingSampleRate();
			float scale = 0.25; // Default max volume for one note
			gigNote note(size);
			note.midiNote = midiNote;

			for (int i = 0; i < size; ++i)
			{
				note.note[i][0] = sin(1.0*i*freq*2*M_PI/engine::mixer()->processingSampleRate())
					* (1.0-1.0/size*i)*scale;
				note.note[i][1] = note.note[i][0];
			}

			m_synthMutex.lock();
			m_notes.push_back(note);
			m_synthMutex.unlock();
		}
	}

	/*GIGPluginData * pluginData = static_cast<GIGPluginData *>(
							_n->m_pluginData );

	const float currentVelocity = _n->volumeLevel( tfp ) * instrumentTrack()->midiPort()->baseVelocity();
	if( pluginData->lastVelocity != currentVelocity )
	{
		m_synthMutex.lock();
        // TODO: set the new velocity to currentvelocity
		m_synthMutex.unlock();

		pluginData->lastVelocity = currentVelocity;
	}*/
}




// Could we get iph-based instruments support sample-exact models by using a
// frame-length of 1 while rendering?
void gigInstrument::play( sampleFrame * _working_buffer )
{
	const fpp_t frames = engine::mixer()->framesPerPeriod();

	// Initialize to zeros
	for (int i = 0; i < frames; ++i)
	{
		_working_buffer[i][0] = float();
		_working_buffer[i][1] = float();
	}

	m_synthMutex.lock();

	const int currentMidiPitch = instrumentTrack()->midiPitch();
	if( m_lastMidiPitch != currentMidiPitch )
	{
		m_lastMidiPitch = currentMidiPitch;
        // TODO: pitch bend...
		//fluid_synth_pitch_bend( m_synth, m_channel, m_lastMidiPitch );
	}

	const int currentMidiPitchRange = instrumentTrack()->midiPitchRange();
	if( m_lastMidiPitchRange != currentMidiPitchRange )
	{
		m_lastMidiPitchRange = currentMidiPitchRange;
        // TODO: pitch bend...
		//fluid_synth_pitch_wheel_sens( m_synth, m_channel, m_lastMidiPitchRange );
	}

	if( m_internalSampleRate < engine::mixer()->processingSampleRate() &&
							m_srcState != NULL )
	{
		const fpp_t f = frames * m_internalSampleRate / engine::mixer()->processingSampleRate();
#ifdef __GNUC__
		sampleFrame tmp[f];
#else
		sampleFrame * tmp = new sampleFrame[f];
#endif

		qDebug() << "Different internal sample rate";

        // TODO: get sample, interleave left/right channels
		SRC_DATA src_data;
		src_data.data_in = tmp[0];
		src_data.data_out = _working_buffer[0];
		src_data.input_frames = f;
		src_data.output_frames = frames;
		src_data.src_ratio = (double) frames / f;
		src_data.end_of_input = 0;
		int error = src_process( m_srcState, &src_data );
#ifndef __GNUC__
		delete[] tmp;
#endif
		if( error )
		{
			qCritical( "gigInstrument: error while resampling: %s", src_strerror( error ) );
		}
		if( src_data.output_frames_gen > frames )
		{
			qCritical( "gigInstrument: not enough frames: %ld / %d", src_data.output_frames_gen, frames );
		}
	}
	else
	{
        // TODO: get sample, interleave left/right channels
		//fluid_synth_write_float( m_synth, frames, _working_buffer, 0, 2, _working_buffer, 1, 2 );

		for( int i = 0; i < frames; ++i )
		{
			for( std::list<gigNote>::iterator note = m_notes.begin(); note != m_notes.end(); ++note )
			{
				if( note->position < note->size && note->size > 0 )
				{
					_working_buffer[i][0] += note->note[note->position][0]*m_gain.value();
					_working_buffer[i][1] += note->note[note->position][1]*m_gain.value();
					++note->position;
				}
			}
		}
	}

	m_synthMutex.unlock();

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, NULL );
}




void gigInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	//GIGPluginData * pluginData = static_cast<GIGPluginData *>( _n->m_pluginData );
	m_synthMutex.lock();

	// Delete ended notes. Continue looping starting at the beginning. Erasing
	// invalidates the iterator.
	bool deleted = false;

	do
	{
		deleted = false;
		std::list<gigNote>::iterator i = m_notes.begin();

		while( i != m_notes.end() )
		{
			if( i->position >= i->size )
			{
				m_notes.erase(i);
				deleted = true;
				break;
			}

			++i;
		}
	}
	while( deleted );

	m_synthMutex.unlock();

	//delete pluginData;
}




PluginView * gigInstrument::instantiateView( QWidget * _parent )
{
	return new gigInstrumentView( this, _parent );
}







class gigKnob : public knob
{
public:
	gigKnob( QWidget * _parent ) :
//			knob( knobStyled, _parent )
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
    // TODO: does it have patches?
	gigInstrument * k = castModel<gigInstrument>();
	patchesDialog pd( this );
	pd.setup( k->m_instance, 1, k->instrumentTrack()->name(), &k->m_bankNum, &k->m_patchNum, m_patchLabel );
	pd.exec();
}



extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new gigInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}

#include "moc_gig_player.cxx"

