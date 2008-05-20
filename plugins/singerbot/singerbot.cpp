/*
 * singerbot.cpp - a singing bot instrument plugin
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


#include "singerbot.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <QtCore/QDir>
#include <QtGui/QLayout>
#include <QtGui/QTextEdit>
#include <QtXml/QDomElement>

#include "engine.h"
#include "file.h"
#include "instrument_track.h"
#include "note_play_handle.h"
#include "pattern.h"
#include "song.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"

#include "singerbot.moc"


extern "C"
{

plugin::descriptor singerbot_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"SingerBot",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Singer bot to add some basic vocals" ),
	"Javier Serrano Polo <jasp00/at/users.sourceforge.net>",
	0x0100,
	plugin::Instrument,
	new pluginPixmapLoader( "logo" ),
	NULL
} ;


// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( model *, void * _data )
{
	return( new singerBot( static_cast<instrumentTrack *>( _data ) ) );
}

}




singerBot::singerBot( instrumentTrack * _track ) :
	instrument( _track, &singerbot_plugin_descriptor )
{
	printf("singerBot constructor begin...\n");	fflush(stdout);

	// Create a unique suffix for the /dev/shm device file names
	static int suffix_index = 0;
	m_file_suffix = '.' + QString::number( getpid() ) + '.'
					+ QString::number( suffix_index++, 16 );

	// This creates a (file) device in /dev/shm called lmms_singerbot.[pid].[suffix_index]
	int fd = shm_open( addSuffix( "/lmms_singerbot" ),
				O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR );
	m_shm = new File( fd );

	// This creates a (file) device in /dev/shm called sem.lmms_singerbot_s1.[pid].[suffix_index]
	m_handle_semaphore = sem_open( addSuffix( "/lmms_singerbot_s1" ),
				O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0 );

	// This creates a (file) device in /dev/shm called sem.lmms_singerbot_s2.[pid].[suffix_index]
	m_synth_semaphore = sem_open( addSuffix( "/lmms_singerbot_s2" ),
				O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0 );

	pid_t cpid = fork();
	if( cpid == -1 )
	{
		perror( "fork" );
		exit( EXIT_FAILURE );
	}
	else if( cpid == 0 )
	{
		sem_close( m_handle_semaphore );
		sem_close( m_synth_semaphore );

		QString proxy_exec = configManager::inst()->pluginDir() +
							QDir::separator() +
							"singerbot_proxy";
		execlp( proxy_exec.toAscii().constData(),
					proxy_exec.toAscii().constData(),
					m_file_suffix.toAscii().constData(),
									NULL );
		exit( EXIT_FAILURE );
	}

	sem_wait( m_handle_semaphore );

    // Set a default text string, but do not emit the dataChanged signal
    setPlainText( "Hello world", FALSE );

}




singerBot::~singerBot()
{
	m_shm->rewind();
	float stop = -1.0;
	m_shm->write( &stop );

	sem_post( m_synth_semaphore );
	wait( NULL );

	sem_close( m_handle_semaphore );
	sem_close( m_synth_semaphore );
	sem_unlink( addSuffix( "/lmms_singerbot_s1" ) );
	sem_unlink( addSuffix( "/lmms_singerbot_s2" ) );

	delete m_shm;
	shm_unlink( addSuffix( "/lmms_singerbot" ) );

}




pluginView * singerBot::instantiateView( QWidget * _parent )
{
	return( new singerBotView( this, _parent ) );
}




void singerBot::playNote( notePlayHandle * _n, bool,
						sampleFrame * _working_buf )
{
	printf("singerBot::playNote begin...\n"); fflush(stdout);

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	if( !_n->m_pluginData )
	{
		createWave( _n );
	}
	handle_data * hdata = (handle_data *)_n->m_pluginData;

	if( hdata->remaining_frames <= 0 )
	{
		return;
	}

	play( _working_buf, hdata, frames );
	getInstrumentTrack()->processAudioBuffer( _working_buf, frames, _n );

	printf("singerBot::playNote end...\n"); fflush(stdout);
}




void singerBot::deleteNotePluginData( notePlayHandle * _n )
{
	handle_data * hdata = (handle_data *)_n->m_pluginData;
	delete[] hdata->wave;
	src_delete( hdata->resampling_state );
	delete hdata;
}




void singerBot::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	QDomElement element = _doc.createElement( "lyrics" );
	_this.appendChild( element );

	QDomCDATASection ds = _doc.createCDATASection( m_plain_text );
	element.appendChild( ds );
}




void singerBot::loadSettings( const QDomElement & _this )
{
    QString saved_lyrics = _this.namedItem( "lyrics" ).toElement().text() ;

    // TODO: What if the load fails?
    // if (!failed)
    // {
    setPlainText( saved_lyrics );
    // }

}




QString singerBot::nodeName( void ) const
{
	return( singerbot_plugin_descriptor.name );
}




void singerBot::setPlainText( const QString & _plain_text, bool _emitDataChanged )
{
    m_plain_text = _plain_text;

    if (_emitDataChanged)
    {
        // m_plain_text changed so send out notification
        emit ( dataChanged() );
    }

	m_words_dirty = TRUE;
}




const QString & singerBot::getPlainText()
{
    return m_plain_text;
}




void singerBot::createWave( notePlayHandle * _n )
{
	printf("singerBot::createWave begin...\n"); fflush(stdout);

	handle_data * hdata = new handle_data;
	_n->m_pluginData = hdata;
	hdata->wave = NULL;
	hdata->remaining_frames = 0;
	hdata->resampling_state = NULL;

	if( m_words_dirty )
	{
        m_words = m_plain_text.simplified().toLower().split( ' ' );

        m_words_dirty = FALSE;
	}

	if( m_words.empty() )
	{
		return;
	}

	printf("_n->frequency = %f\n", _n->frequency() ); fflush(stdout);

	hdata->frequency = _n->frequency();
	hdata->duration = _n->length() > 0 ?
		_n->length() * 60.0f * BEATS_PER_TACT
				/ 64.0f / engine::getSong()->getTempo() :
		0;
	int word_index = _n->patternIndex() % m_words.size();
	hdata->text = m_words[word_index].toAscii().constData();

	// Debugging only
	showHandleData( hdata );

	synth_send( hdata );
	synth_read( hdata );

	if( !hdata->wave )
	{
		return;
	}

	int error;
	hdata->resampling_state = src_new( SRC_LINEAR, 1, &error );
	if( !hdata->resampling_state )
	{
		printf( "%s: src_new() error: %s\n", __FILE__,
							src_strerror( error ) );
	}

	hdata->resampling_data.end_of_input = 0;
	hdata->remaining_frames = hdata->num_samples;

	printf("singerBot::createWave end...\n"); fflush(stdout);
}




void singerBot::play( sampleFrame * _ab, handle_data * _hdata,
							const fpp_t _frames )
{
	printf("singerBot::play begin...\n"); fflush(stdout);

	const f_cnt_t offset = _hdata->num_samples - _hdata->remaining_frames;

	const double ratio = engine::getMixer()->processingSampleRate()
						/ (double)_hdata->sample_rate;

	const f_cnt_t margin = 2;
	f_cnt_t fragment_size = (f_cnt_t)( _frames / ratio ) + margin;

	sample_t * sample_fragment = new sample_t[fragment_size];

	if( fragment_size <= _hdata->remaining_frames )
	{
		for( f_cnt_t frame = 0; frame < fragment_size; ++frame )
		{
			sample_fragment[frame] = _hdata->wave[offset + frame]
						/ OUTPUT_SAMPLE_MULTIPLIER;
		}
	}
	else
	{
		for( f_cnt_t frame = 0; frame < _hdata->remaining_frames;
								++frame )
		{
			sample_fragment[frame] = _hdata->wave[offset + frame]
						/ OUTPUT_SAMPLE_MULTIPLIER;
		}
		memset( sample_fragment + _hdata->remaining_frames, 0,
				( fragment_size - _hdata->remaining_frames )
							* sizeof( sample_t ) );
	}

	sample_t * data = new sample_t[_frames];

	_hdata->resampling_data.data_in = sample_fragment;
	_hdata->resampling_data.data_out = data;
	_hdata->resampling_data.input_frames = fragment_size;
	_hdata->resampling_data.output_frames = _frames;
	_hdata->resampling_data.src_ratio = ratio;
	int error = src_process( _hdata->resampling_state,
						&_hdata->resampling_data );
	if( error )
	{
		printf( "%s: error while resampling: %s\n", __FILE__,
							src_strerror( error ) );
	}
	if( _hdata->resampling_data.output_frames_gen != _frames )
	{
		printf( "%s: not enough frames: %ld / %d\n", __FILE__,
			_hdata->resampling_data.output_frames_gen, _frames );
	}
	_hdata->remaining_frames -= _hdata->resampling_data.input_frames_used;

	for( f_cnt_t frame = 0; frame < _frames; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_ab[frame][chnl] = data[frame];
		}
	}

	delete[] sample_fragment;
	delete[] data;

	printf("singerBot::play end...\n"); fflush(stdout);
}




void singerBot::synth_send( handle_data * _hdata )
{
	printf("singerBot::synth_send begin...\n"); fflush(stdout);

	m_shm->rewind();
	m_shm->write( &_hdata->frequency );
	m_shm->write( &_hdata->duration );
	Uint8 len = strlen( _hdata->text );
	m_shm->write( &len );
	m_shm->write( _hdata->text, len );

	sem_post( m_synth_semaphore );

	printf("singerBot::synth_send end...\n"); fflush(stdout);
}




void singerBot::synth_read( handle_data * _hdata )
{
	printf("singerBot::synth_read begin...\n"); fflush(stdout);

	sem_wait( m_handle_semaphore );

	m_shm->rewind();
	m_shm->read( &_hdata->num_samples );
	if( !_hdata->num_samples )
	{
		return;
	}
	m_shm->read( &_hdata->sample_rate );
	_hdata->wave = new short[_hdata->num_samples];
	m_shm->read( _hdata->wave, _hdata->num_samples );

	printf("singerBot::synth_read end...\n"); fflush(stdout);
}




const char * singerBot::addSuffix( const char * _s )
{
	return( QString( _s + m_file_suffix ).toAscii().constData() );
}



// For debugging only
void singerBot::showHandleData( handle_data * _hdata )
{
    printf("Show Handle Data:\n");
    printf("wave = %u\n", (unsigned int)_hdata->wave);
    printf("num_samples = %d\n", _hdata->num_samples);
    printf("sample_rate = %d\n", _hdata->sample_rate);
    printf("remaining_frames = %d\n", _hdata->remaining_frames);
    printf("frequency = %f\n", _hdata->frequency);
    printf("duration = %f\n", _hdata->duration);
	//const char * text;
	//SRC_STATE * resampling_state;
	//SRC_DATA resampling_data;

    fflush(stdout);
}


// ********  The View  *********

singerBotView::singerBotView( instrument * _instrument,
							QWidget * _parent ) :
	instrumentView( _instrument, _parent )
{
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	QVBoxLayout * vbox = new QVBoxLayout( this );
	vbox->setMargin( 10 );
	vbox->setSpacing( 0 );
	vbox->addSpacing( 45 );

	m_lyrics = new QTextEdit( this );
	m_lyrics->setAutoFillBackground( TRUE );
	pal.setColor( m_lyrics->backgroundRole(), QColor( 64, 64, 64 ) );
	m_lyrics->setPalette( pal );

	// Connect the QTextEdit textChanged signal to the viewTextChanged slot
	connect( m_lyrics, SIGNAL( textChanged( void ) ),
					this, SLOT( viewTextChanged( void ) ) );

	// Need to set the view's text the same as the model, so just run the handler
	// that would have run if we had gotten dataChanged() signal from the model
	modelTextChanged();

	vbox->addWidget( m_lyrics );

	// Connect the model dataChanged event to the view's handler
	// (see example from audio_file_processor.cpp line 585)
	connect( castModel<singerBot>(), SIGNAL( dataChanged() ),
			 this, SLOT( modelTextChanged() ) );

}




// This slot is connected to the QTextEdit textChanged singal
void singerBotView::viewTextChanged( void )
{
    QString modelText = castModel<singerBot>()->getPlainText();

    // If the model text is already the same then stop updating
    // (otherwise there would be a infinite loop of model -> view -> model -> etc
    if (m_lyrics->toPlainText() != modelText)
    {
        // Call the model to set the new text that changed via UI
        castModel<singerBot>()->setPlainText( m_lyrics->toPlainText() );
    }

}



void singerBotView::modelTextChanged( void )
{
    // Set the view's text the same as the model
    m_lyrics->setText( castModel<singerBot>()->getPlainText() );
}




singerBotView::~singerBotView()
{
}
