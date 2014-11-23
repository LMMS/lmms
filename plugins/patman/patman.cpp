/*
 * patman.cpp - a GUS-compatible patch instrument plugin
 *
 * Copyright (c) 2007-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "patman.h"

#include <QtGui/QDragEnterEvent>
#include <QtGui/QPainter>
#include <QtXml/QDomElement>

#include "endian_handling.h"
#include "engine.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "NotePlayHandle.h"
#include "pixmap_button.h"
#include "song.h"
#include "string_pair_drag.h"
#include "tooltip.h"
#include "FileDialog.h"

#include "embed.cpp"




extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT patman_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"PatMan",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"GUS-compatible patch instrument" ),
	"Javier Serrano Polo <jasp00/at/users.sourceforge.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	"pat",
	NULL
} ;


// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new patmanInstrument( static_cast<InstrumentTrack *>( _data ) );
}

}




patmanInstrument::patmanInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &patman_plugin_descriptor ),
	m_patchFile( QString::null ),
	m_loopedModel( true, this ),
	m_tunedModel( true, this )
{
}




patmanInstrument::~patmanInstrument()
{
	unloadCurrentPatch();
}




void patmanInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "src", m_patchFile );
	m_loopedModel.saveSettings( _doc, _this, "looped" );
	m_tunedModel.saveSettings( _doc, _this, "tuned" );
}




void patmanInstrument::loadSettings( const QDomElement & _this )
{
	setFile( _this.attribute( "src" ), FALSE );
	m_loopedModel.loadSettings( _this, "looped" );
	m_tunedModel.loadSettings( _this, "tuned" );
}




void patmanInstrument::loadFile( const QString & _file )
{
	setFile( _file );
}




QString patmanInstrument::nodeName( void ) const
{
	return( patman_plugin_descriptor.name );
}




void patmanInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if( m_patchFile == "" )
	{
		return;
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	if( !_n->m_pluginData )
	{
		selectSample( _n );
	}
	handle_data * hdata = (handle_data *)_n->m_pluginData;

	float play_freq = hdata->tuned ? _n->frequency() :
						hdata->sample->frequency();

	if( hdata->sample->play( _working_buffer, hdata->state, frames,
					play_freq, m_loopedModel.value() ? SampleBuffer::LoopOn : SampleBuffer::LoopOff ) )
	{
		applyRelease( _working_buffer, _n );
		instrumentTrack()->processAudioBuffer( _working_buffer,
								frames, _n );
	}
}




void patmanInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	handle_data * hdata = (handle_data *)_n->m_pluginData;
	sharedObject::unref( hdata->sample );
	delete hdata->state;
	delete hdata;
}




void patmanInstrument::setFile( const QString & _patch_file, bool _rename )
{
	if( _patch_file.size() <= 0 )
	{
		m_patchFile = QString::null;
		return;
	}

	// is current instrument-track-name equal to previous-filename??
	if( _rename &&
		( instrumentTrack()->name() ==
					QFileInfo( m_patchFile ).fileName() ||
				   	m_patchFile == "" ) )
	{
		// then set it to new one
		instrumentTrack()->setName( QFileInfo( _patch_file
								).fileName() );
	}
	// else we don't touch the instrument-track-name, because the user
	// named it self

	m_patchFile = SampleBuffer::tryToMakeRelative( _patch_file );
	LoadErrors error = loadPatch( SampleBuffer::tryToMakeAbsolute( _patch_file ) );
	if( error )
	{
		printf("Load error\n");
	}

	emit fileChanged();
}




patmanInstrument::LoadErrors patmanInstrument::loadPatch(
						const QString & _filename )
{
	unloadCurrentPatch();

	FILE * fd = fopen( _filename.toUtf8().constData() , "rb" );
	if( !fd )
	{
		perror( "fopen" );
		return( LoadOpen );
	}

	unsigned char header[239];

	if( fread( header, 1, 239, fd ) != 239 ||
			( memcmp( header, "GF1PATCH110\0ID#000002", 22 )
			&& memcmp( header, "GF1PATCH100\0ID#000002", 22 ) ) )
	{
		fclose( fd );
		return( LoadNotGUS );
	}

	if( header[82] != 1 && header[82] != 0 )
	{
		fclose( fd );
		return( LoadInstruments );
	}

	if( header[151] != 1 && header[151] != 0 )
	{
		fclose( fd );
		return( LoadLayers );
	}

	int sample_count = header[198];
	for( int i = 0; i < sample_count; ++i )
	{
		unsigned short tmpshort;

#define SKIP_BYTES( x ) \
		if ( fseek( fd, x, SEEK_CUR ) == -1 ) \
		{ \
			fclose( fd ); \
			return( LoadIO ); \
		}

#define READ_SHORT( x ) \
		if ( fread( &tmpshort, 2, 1, fd ) != 1 ) \
		{ \
			fclose( fd ); \
			return( LoadIO ); \
		} \
		x = (unsigned short)swap16IfBE( tmpshort );

#define READ_LONG( x ) \
		if ( fread( &x, 4, 1, fd ) != 1 ) \
		{ \
			fclose( fd ); \
			return( LoadIO ); \
		} \
		x = (unsigned)swap32IfBE( x );

		// skip wave name, fractions
		SKIP_BYTES( 7 + 1 );
		unsigned data_length;
		READ_LONG( data_length );
		unsigned loop_start;
		READ_LONG( loop_start );
		unsigned loop_end;
		READ_LONG( loop_end );
		unsigned sample_rate;
		READ_SHORT( sample_rate );
		// skip low_freq, high_freq
		SKIP_BYTES( 4 + 4 );
		unsigned root_freq;
		READ_LONG( root_freq );
		// skip tuning, panning, envelope, tremolo, vibrato
		SKIP_BYTES( 2 + 1 + 12 + 3 + 3 );
		unsigned char modes;
		if ( fread( &modes, 1, 1, fd ) != 1 )
		{
			fclose( fd );
			return( LoadIO );
		}
		// skip scale frequency, scale factor, reserved space
		SKIP_BYTES( 2 + 2 + 36 );

		f_cnt_t frames;
		sample_t * wave_samples;
		if( modes & MODES_16BIT )
		{
			frames = data_length >> 1;
			wave_samples = new sample_t[frames];
			for( f_cnt_t frame = 0; frame < frames; ++frame )
			{
				short sample;
				if ( fread( &sample, 2, 1, fd ) != 1 )
				{
					delete[] wave_samples;
					fclose( fd );
					return( LoadIO );
				}
				sample = swap16IfBE( sample );
				if( modes & MODES_UNSIGNED )
				{
					sample ^= 0x8000;
				}
				wave_samples[frame] = sample / 32767.0f;
			}

			loop_start >>= 1;
			loop_end >>= 1;
		}
		else
		{
			frames = data_length;
			wave_samples = new sample_t[frames];
			for( f_cnt_t frame = 0; frame < frames; ++frame )
			{
				char sample;
				if ( fread( &sample, 1, 1, fd ) != 1 )
				{
					delete[] wave_samples;
					fclose( fd );
					return( LoadIO );
				}
				if( modes & MODES_UNSIGNED )
				{
					sample ^= 0x80;
				}
				wave_samples[frame] = sample / 127.0f;
			}
		}

		sampleFrame * data = new sampleFrame[frames];

		for( f_cnt_t frame = 0; frame < frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
			{
				data[frame][chnl] = wave_samples[frame];
			}
		}

		SampleBuffer* psample = new SampleBuffer( data, frames );
		psample->setFrequency( root_freq / 1000.0f );
		psample->setSampleRate( sample_rate );

		if( modes & MODES_LOOPING )
		{
			psample->setLoopStartFrame( loop_start );
			psample->setLoopEndFrame( loop_end );
		}

		m_patchSamples.push_back( psample );

		delete[] wave_samples;
		delete[] data;
	}
	fclose( fd );
	return( LoadOK );
}




void patmanInstrument::unloadCurrentPatch( void )
{
	while( !m_patchSamples.empty() )
	{
		sharedObject::unref( m_patchSamples.back() );
		m_patchSamples.pop_back();
	}
}




void patmanInstrument::selectSample( NotePlayHandle * _n )
{
	const float freq = _n->frequency();

	float min_dist = HUGE_VALF;
	SampleBuffer* sample = NULL;

	for( QVector<SampleBuffer *>::iterator it = m_patchSamples.begin(); it != m_patchSamples.end(); ++it )
	{
		float patch_freq = ( *it )->frequency();
		float dist = freq >= patch_freq ? freq / patch_freq :
							patch_freq / freq;

		if( dist < min_dist )
		{
			min_dist = dist;
			sample = *it;
		}
	}

	handle_data * hdata = new handle_data;
	hdata->tuned = m_tunedModel.value();
	if( sample )
	{
		hdata->sample = sharedObject::ref( sample );
	}
	else
	{
		hdata->sample = new SampleBuffer( NULL, 0 );
	}
	hdata->state = new SampleBuffer::handleState( _n->hasDetuningInfo() );

	_n->m_pluginData = hdata;
}




PluginView * patmanInstrument::instantiateView( QWidget * _parent )
{
	return( new PatmanView( this, _parent ) );
}










PatmanView::PatmanView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent ),
	m_pi( NULL )
{
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );


	m_openFileButton = new pixmapButton( this, NULL );
	m_openFileButton->setObjectName( "openFileButton" );
	m_openFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openFileButton->move( 227, 86 );
	m_openFileButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file_on" ) );
	m_openFileButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	connect( m_openFileButton, SIGNAL( clicked() ),
				this, SLOT( openFile() ) );
	toolTip::add( m_openFileButton, tr( "Open other patch" ) );

	m_openFileButton->setWhatsThis(
		tr( "Click here to open another patch-file. Loop and Tune "
			"settings are not reset." ) );

	m_loopButton = new pixmapButton( this, tr( "Loop" ) );
	m_loopButton->setObjectName("loopButton");
	m_loopButton->setCheckable( TRUE );
	m_loopButton->move( 195, 138 );
	m_loopButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_on" ) );
	m_loopButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_off" ) );
	toolTip::add( m_loopButton, tr( "Loop mode" ) );
	m_loopButton->setWhatsThis(
		tr( "Here you can toggle the Loop mode. If enabled, PatMan "
			"will use the loop information available in the "
			"file." ) );

	m_tuneButton = new pixmapButton( this, tr( "Tune" ) );
	m_tuneButton->setObjectName("tuneButton");
	m_tuneButton->setCheckable( TRUE );
	m_tuneButton->move( 223, 138 );
	m_tuneButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"tune_on" ) );
	m_tuneButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"tune_off" ) );
	toolTip::add( m_tuneButton, tr( "Tune mode" ) );
	m_tuneButton->setWhatsThis(
		tr( "Here you can toggle the Tune mode. If enabled, PatMan "
			"will tune the sample to match the note's "
			"frequency." ) );

	m_displayFilename = tr( "No file selected" );

	setAcceptDrops( TRUE );
}




PatmanView::~PatmanView()
{
}




void PatmanView::openFile( void )
{
	FileDialog ofd( NULL, tr( "Open patch file" ) );
	ofd.setFileMode( FileDialog::ExistingFiles );

	QStringList types;
	types << tr( "Patch-Files (*.pat)" );
	ofd.setFilters( types );

	if( m_pi->m_patchFile == "" )
	{
		if( QDir( "/usr/share/midi/freepats" ).exists() )
		{
			ofd.setDirectory( "/usr/share/midi/freepats" );
		}
		else
		{
			ofd.setDirectory(
				configManager::inst()->userSamplesDir() );
		}
	}
	else if( QFileInfo( m_pi->m_patchFile ).isRelative() )
	{
		QString f = configManager::inst()->userSamplesDir()
							+ m_pi->m_patchFile;
		if( QFileInfo( f ).exists() == FALSE )
		{
			f = configManager::inst()->factorySamplesDir()
							+ m_pi->m_patchFile;
		}

		ofd.selectFile( f );
	}
	else
	{
		ofd.selectFile( m_pi->m_patchFile );
	}

	if( ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		QString f = ofd.selectedFiles()[0];
		if( f != "" )
		{
			m_pi->setFile( f );
			engine::getSong()->setModified();
		}
	}
}




void PatmanView::updateFilename( void )
{
 	m_displayFilename = "";
	int idx = m_pi->m_patchFile.length();

	QFontMetrics fm( pointSize<8>( font() ) );

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
	while( idx > 0 && fm.size( Qt::TextSingleLine,
				m_displayFilename + "..." ).width() < 225 )
	{
		m_displayFilename = m_pi->m_patchFile[--idx] +
							m_displayFilename;
	}

	if( idx > 0 )
	{
		m_displayFilename = "..." + m_displayFilename;
	}

	update();
}




void PatmanView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( stringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						stringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == "samplefile" )
		{
			_dee->acceptProposedAction();
		}
		else
		{
			_dee->ignore();
		}
	}
	else
	{
		_dee->ignore();
	}
}




void PatmanView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		m_pi->setFile( value );
		_de->accept();
		return;
	}

	_de->ignore();
}




void PatmanView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.setFont( pointSize<8>( font() ) );
	p.drawText( 8, 116, 235, 16, 
			Qt::AlignLeft | Qt::TextSingleLine | Qt::AlignVCenter, 
			m_displayFilename );
}




void PatmanView::modelChanged( void )
{
	m_pi = castModel<patmanInstrument>();
	m_loopButton->setModel( &m_pi->m_loopedModel );
	m_tuneButton->setModel( &m_pi->m_tunedModel );
	connect( m_pi, SIGNAL( fileChanged() ),
			this, SLOT( updateFilename() ) );
}




#include "moc_patman.cxx"
