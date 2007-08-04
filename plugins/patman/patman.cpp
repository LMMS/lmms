/*
 * patman.cpp - a GUS-compatible patch instrument plugin
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QFileDialog>
#include <QtGui/QDragEnterEvent>
#include <QtXml/QDomElement>

#else

#include <qcursor.h>
#include <qdom.h>
#include <qfiledialog.h>
#include <qwhatsthis.h>

#endif

#include "patman.h"
#include "endian_handling.h"
#include "engine.h"
#include "file_browser.h"
#include "gui_templates.h"
#include "note_play_handle.h"
#include "pixmap_button.h"
#include "song_editor.h"
#include "string_pair_drag.h"
#include "tooltip.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"




extern "C"
{

plugin::descriptor patman_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"PatMan",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"GUS-compatible patch instrument" ),
	"Javier Serrano Polo <jasp00/at/users.sourceforge.net>",
	0x0100,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	new patmanSynth::subPluginFeatures( plugin::Instrument )
} ;


// necessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new patmanSynth( static_cast<instrumentTrack *>( _data ) ) );
}

}




patmanSynth::patmanSynth( instrumentTrack * _track ) :
	instrument( _track, &patman_plugin_descriptor ),
	specialBgHandlingWidget( PLUGIN_NAME::getIconPixmap( "artwork" ) )
{
#ifndef QT3
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
#else
	setPaletteBackgroundPixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif
	m_openFileButton = new pixmapButton( this, NULL, NULL );
	m_openFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openFileButton->move( 200, 90 );
	m_openFileButton->setActiveGraphic( embed::getIconPixmap(
							"project_open_down" ) );
	m_openFileButton->setInactiveGraphic( embed::getIconPixmap(
							"project_open" ) );
	m_openFileButton->setBgGraphic( getBackground(
						m_openFileButton ) );
	connect( m_openFileButton, SIGNAL( clicked() ), this,
						SLOT( openFile() ) );
	toolTip::add( m_openFileButton, tr( "Open other patch" ) );

#ifdef QT4
	m_openFileButton->setWhatsThis(
#else
	QWhatsThis::add( m_openFileButton,
#endif
		tr( "Click here to open another patch-file. Loop and Tune "
			"settings are not reset." ) );

	m_loopButton = new pixmapButton( this, tr( "Loop" ), _track );
	m_loopButton->setCheckable( TRUE );
	m_loopButton->move( 160, 160 );
	m_loopButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_on" ) );
	m_loopButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_off" ) );
	m_loopButton->setBgGraphic( getBackground( m_loopButton ) );
	toolTip::add( m_loopButton, tr( "Loop mode" ) );
#ifdef QT4
	m_loopButton->setWhatsThis(
#else
	QWhatsThis::add( m_loopButton,
#endif
		tr( "Here you can toggle the Loop mode. If enabled, PatMan "
			"will use the loop information available in the "
			"file." ) );

	m_tuneButton = new pixmapButton( this, tr( "Tune" ), _track );
	m_tuneButton->setCheckable( TRUE );
	m_tuneButton->setValue( TRUE );
	m_tuneButton->move( 180, 160 );
	m_tuneButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"tune_on" ) );
	m_tuneButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"tune_off" ) );
	m_tuneButton->setBgGraphic( getBackground( m_tuneButton ) );
	toolTip::add( m_tuneButton, tr( "Tune mode" ) );
#ifdef QT4
	m_tuneButton->setWhatsThis(
#else
	QWhatsThis::add( m_tuneButton,
#endif
		tr( "Here you can toggle the Tune mode. If enabled, PatMan "
			"will tune the sample to match the note's "
			"frequency." ) );

	m_display_filename = tr( "No file selected" );

	setAcceptDrops( TRUE );
}




patmanSynth::~patmanSynth()
{
	unload_current_patch();
}




void patmanSynth::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "src", m_patchFile );
	m_loopButton->saveSettings( _doc, _this, "looped" );
	m_tuneButton->saveSettings( _doc, _this, "tuned" );
}




void patmanSynth::loadSettings( const QDomElement & _this )
{
	setFile( _this.attribute( "src" ), FALSE );
	m_loopButton->loadSettings( _this, "looped" );
	m_tuneButton->loadSettings( _this, "tuned" );
}




void patmanSynth::setParameter( const QString & _param, const QString & _value )
{
	if( _param == "samplefile" )
	{
		setFile( _value );
	}
}




QString patmanSynth::nodeName( void ) const
{
	return( patman_plugin_descriptor.name );
}




void patmanSynth::playNote( notePlayHandle * _n, bool )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	sampleFrame * buf = new sampleFrame[frames];

	if( !_n->m_pluginData )
	{
		select_sample( _n );
	}
	handle_data * hdata = (handle_data *)_n->m_pluginData;

	float play_freq = hdata->tuned ? _n->frequency() :
						hdata->sample->frequency();

	if( hdata->sample->play( buf, hdata->state, frames, play_freq,
						m_loopButton->isChecked() ) )
	{
		getInstrumentTrack()->processAudioBuffer( buf, frames, _n );
	}
	delete[] buf;
}




void patmanSynth::deleteNotePluginData( notePlayHandle * _n )
{
	handle_data * hdata = (handle_data *)_n->m_pluginData;
	sharedObject::unref( hdata->sample );
	delete hdata->state;
	delete hdata;
}




void patmanSynth::dragEnterEvent( QDragEnterEvent * _dee )
{
#ifdef QT4
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
#else
	QString txt = _dee->encodedData( stringPairDrag::mimeType() );
	if( txt != "" )
	{
		if( txt.section( ':', 0, 0 ) == "samplefile"
			&& subPluginFeatures::supported_extensions().contains(
				fileItem::extension( txt.section( ':', 1 ) ) ) )
		{
			_dee->accept();
			return;
		}
		_dee->ignore();
		return;
	}

	txt = QString( _dee->encodedData( "text/plain" ) );
	if( txt != "" )
	{
		QString file = QUriDrag::uriToLocalFile(
							txt.stripWhiteSpace() );
		if( file && subPluginFeatures::supported_extensions().contains(
						fileItem::extension( file ) ) )
		{
			_dee->accept();
			return;
		}
	}
	_dee->ignore();
#endif
}




void patmanSynth::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		setFile( value );
		_de->accept();
		return;
	}

#ifndef QT4
	QString txt = _de->encodedData( "text/plain" );
	if( txt != "" )
	{
		setFile( QUriDrag::uriToLocalFile( txt.stripWhiteSpace() ) );
		_de->accept();
		return;
	}
#endif

	_de->ignore();
}




void patmanSynth::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap pm( rect().size() );
	pm.fill( this, rect().topLeft() );

	QPainter p( &pm, this );
#endif

	p.setFont( pointSize<8>( font() ) );
	p.setPen( QColor( 0x66, 0xFF, 0x66 ) );
	p.drawText( 8, 140, m_display_filename );

#ifndef QT4
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void patmanSynth::openFile( void )
{
#ifdef QT4
	QFileDialog ofd( NULL, tr( "Open patch file" ) );
#else
	QFileDialog ofd( QString::null, QString::null, NULL, "", TRUE );
	ofd.setWindowTitle( tr( "Open patch file" ) );
#endif
	ofd.setFileMode( QFileDialog::ExistingFiles );

	// set filters
#ifdef QT4
	QStringList types;
	types << tr( "Patch-Files (*.pat)" );
	ofd.setFilters( types );
#else
	ofd.addFilter( tr( "Patch-Files (*.pat)" ) );
#endif

	if( m_patchFile == "" )
	{
		ofd.setDirectory( configManager::inst()->userSamplesDir() );
	}
	else if( QFileInfo( m_patchFile ).isRelative() )
	{
		QString f = configManager::inst()->userSamplesDir()
								+ m_patchFile;
		if( QFileInfo( f ).exists() == FALSE )
		{
			f = configManager::inst()->factorySamplesDir()
								+ m_patchFile;
		}

		ofd.selectFile( f );
	}
	else
	{
		ofd.selectFile( m_patchFile );
	}

	if( ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		QString f = ofd.selectedFiles()[0];
		if( f != "" )
		{
			setFile( f );
			engine::getSongEditor()->setModified();
		}
	}
}




void patmanSynth::setFile( const QString & _patch_file, bool _rename )
{
	// is current channel-name equal to previous-filename??
	if( _rename &&
		( getInstrumentTrack()->name() ==
					QFileInfo( m_patchFile ).fileName() ||
				   	m_patchFile == "" ) )
	{
		// then set it to new one
		getInstrumentTrack()->setName( QFileInfo( _patch_file
								).fileName() );
	}
	// else we don't touch the channel-name, because the user named it self

	m_patchFile = sampleBuffer::tryToMakeRelative( _patch_file );
	load_error error = load_patch( sampleBuffer::tryToMakeAbsolute(
								_patch_file ) );
	if( error )
	{
		printf("Load error\n");
	}

 	m_display_filename = "";
	Uint16 idx = m_patchFile.length();

	QFontMetrics fm( pointSize<8>( font() ) );

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
	while( idx > 0 && fm.size(
#ifdef QT4
		Qt::TextSingleLine,
#else
		Qt::SingleLine,
#endif
				m_display_filename + "..." ).width() < 225 )
	{
		m_display_filename = m_patchFile[--idx] + m_display_filename;
	}

	if( idx > 0 )
	{
		m_display_filename = "..." + m_display_filename;
	}

	update();
}




patmanSynth::load_error patmanSynth::load_patch( const QString & _filename )
{
	unload_current_patch();

	FILE * fd = fopen( _filename
#ifndef QT3
				.toAscii().constData()
#endif
							, "rb" );
	if( !fd )
	{
		perror( "fopen" );
		return( LOAD_OPEN );
	}

	unsigned char header[239];

	if( fread( header, 1, 239, fd ) != 239 ||
			( memcmp( header, "GF1PATCH110\0ID#000002", 22 )
			&& memcmp( header, "GF1PATCH100\0ID#000002", 22 ) ) )
	{
		fclose( fd );
		return( LOAD_NOT_GUS );
	}

	if( header[82] != 1 && header[82] != 0 )
	{
		fclose( fd );
		return( LOAD_INSTRUMENTS );
	}

	if( header[151] != 1 && header[151] != 0 )
	{
		fclose( fd );
		return( LOAD_LAYERS );
	}

	int sample_count = header[198];
	for( int i = 0; i < sample_count; ++i )
	{
		unsigned short tmpshort;

#define SKIP_BYTES( x ) \
		if ( fseek( fd, x, SEEK_CUR ) == -1 ) \
		{ \
			fclose( fd ); \
			return( LOAD_IO ); \
		}

#define READ_SHORT( x ) \
		if ( fread( &tmpshort, 2, 1, fd ) != 1 ) \
		{ \
			fclose( fd ); \
			return( LOAD_IO ); \
		} \
		x = (unsigned short)swap16IfBE( tmpshort );

#define READ_LONG( x ) \
		if ( fread( &x, 4, 1, fd ) != 1 ) \
		{ \
			fclose( fd ); \
			return( LOAD_IO ); \
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
			return( LOAD_IO );
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
					delete wave_samples;
					fclose( fd );
					return( LOAD_IO );
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
					delete wave_samples;
					fclose( fd );
					return( LOAD_IO );
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

		sampleBuffer * psample = new sampleBuffer( data, frames );
		psample->setFrequency( root_freq / 1000.0f );
		psample->setSampleRate( sample_rate );

		if( modes & MODES_LOOPING )
		{
			psample->setLoopStartFrame( loop_start );
			psample->setLoopEndFrame( loop_end );
		}

		m_patch_samples.push_back( psample );

		delete[] wave_samples;
		delete[] data;
	}
	fclose( fd );
	return( LOAD_OK );
}




void patmanSynth::unload_current_patch( void )
{
	while( !m_patch_samples.empty() )
	{
		sharedObject::unref( m_patch_samples.back() );
		m_patch_samples.pop_back();
	}
}




void patmanSynth::select_sample( notePlayHandle * _n )
{
	const float freq = _n->frequency();

	float min_dist = HUGE_VALF;
	sampleBuffer * sample = NULL;

	for( vvector<sampleBuffer *>::iterator it = m_patch_samples.begin();
					it != m_patch_samples.end(); ++it )
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
	hdata->tuned = m_tuneButton->isChecked();
	if( sample )
	{
		hdata->sample = sharedObject::ref( sample );
	}
	else
	{
		hdata->sample = new sampleBuffer( NULL, 0 );
	}
	hdata->state = new sampleBuffer::handleState( _n->hasDetuningInfo() );

	_n->m_pluginData = hdata;
}








patmanSynth::subPluginFeatures::subPluginFeatures( plugin::pluginTypes _type ) :
	plugin::descriptor::subPluginFeatures( _type )
{
}




const QStringList & patmanSynth::subPluginFeatures::supported_extensions( void )
{
	static QStringList extension( "pat" );
	return( extension );
}




#include "patman.moc"
