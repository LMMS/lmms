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


#include "qt3support.h"

#ifdef QT4

#include <QtCore/QDir>
#include <QtGui/QLayout>
#include <QtGui/QTextEdit>
#include <QtXml/QDomElement>

#else

#include <qdir.h>
#include <qdom.h>
#include <qlayout.h>
#include <qtextedit.h>

#endif

#include "singerbot.h"
#include "engine.h"
#include "instrument_track.h"
#include "note_play_handle.h"
#include "pattern.h"
#include "song_editor.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"

#undef HAVE_CONFIG_H
#include <festival.h>




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
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;


// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new singerBot( static_cast<instrumentTrack *>( _data ) ) );
}

}




singerBot::synThread * singerBot::s_thread = NULL;




singerBot::singerBot( instrumentTrack * _track ) :
	instrument( _track, &singerbot_plugin_descriptor )
{
	if( !s_thread )
	{
		s_thread = new synThread();
		s_thread->start();
	}

#ifndef QT3
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
#else
	setPaletteBackgroundPixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif

	QVBoxLayout * vbox = new QVBoxLayout( this );
	vbox->setMargin( 10 );
	vbox->setSpacing( 0 );
	vbox->addSpacing( 45 );

	m_lyrics = new QTextEdit( this );
#ifdef QT4
	m_lyrics->setAutoFillBackground( TRUE );
	pal.setColor( m_lyrics->backgroundRole(), QColor( 64, 64, 64 ) );
	m_lyrics->setPalette( pal );
#else
	m_lyrics->setTextFormat( PlainText );
	m_lyrics->setPaletteBackgroundColor( QColor( 64, 64, 64 ) );
#endif
	m_lyrics->setText( "Hello, world!" );

	connect( m_lyrics, SIGNAL( textChanged( void ) ),
					this, SLOT( lyricsChanged( void ) ) );

	vbox->addWidget( m_lyrics );

	updateWords();
}




singerBot::~singerBot()
{
}




void singerBot::playNote( notePlayHandle * _n, bool )
{
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

	sampleFrame * buf = new sampleFrame[frames];
	play( buf, hdata, frames );
	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );
	delete[] buf;
}




void singerBot::deleteNotePluginData( notePlayHandle * _n )
{
	handle_data * hdata = (handle_data *)_n->m_pluginData;
	delete hdata->wave;
	src_delete( hdata->resampling_state );
	delete hdata;
}




void singerBot::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	QDomElement element = _doc.createElement( "lyrics" );
	_this.appendChild( element );
#ifdef QT4
	QDomCDATASection ds = _doc.createCDATASection(
						m_lyrics->toPlainText() );
#else
	QDomCDATASection ds = _doc.createCDATASection( m_lyrics->text() );
#endif
	element.appendChild( ds );
}




void singerBot::loadSettings( const QDomElement & _this )
{
#ifdef QT4
	m_lyrics->setPlainText(
			_this.namedItem( "lyrics" ).toElement().text() );
#else
	m_lyrics->setText( _this.namedItem( "lyrics" ).toElement().text() );
#endif
}




QString singerBot::nodeName( void ) const
{
	return( singerbot_plugin_descriptor.name );
}




void singerBot::lyricsChanged( void )
{
	m_words_dirty = TRUE;
}




void singerBot::updateWords( void )
{
#ifndef QT3
	m_words = m_lyrics->toPlainText().simplified().toLower().
								split( ' ' );
#else
	m_words = QStringList::split( ' ',
				m_lyrics->text().simplifyWhiteSpace().lower() );
#endif
	m_words_dirty = FALSE;
}




void singerBot::createWave( notePlayHandle * _n )
{
	handle_data * hdata = new handle_data;
	_n->m_pluginData = hdata;
	hdata->wave = NULL;
	hdata->remaining_frames = 0;
	hdata->resampling_state = NULL;

	if( m_words_dirty )
	{
		updateWords();
	}
	if( m_words.empty() )
	{
		return;
	}

	hdata->frequency = _n->frequency();
	hdata->duration = _n->length() > 0 ?
		_n->length() * 60.0f * BEATS_PER_TACT
				/ 64.0f / engine::getSongEditor()->getTempo() :
		0;
	int word_index = _n->patternIndex() % m_words.size();
	hdata->text = m_words[word_index].
#ifndef QT3
						toAscii().constData();
#else
						ascii();
#endif

	s_thread->set_data( hdata );
	s_thread->unlock_synth();
	s_thread->lock_handle();

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
	hdata->remaining_frames = hdata->wave->num_samples();
}




void singerBot::play( sampleFrame * _ab, handle_data * _hdata,
							const fpp_t _frames )
{
	const f_cnt_t offset = _hdata->wave->num_samples()
						- _hdata->remaining_frames;

	const double ratio = engine::getMixer()->sampleRate()
					/ (double)_hdata->wave->sample_rate();

	const f_cnt_t margin = 2;
	f_cnt_t fragment_size = (f_cnt_t)( _frames / ratio ) + margin;

	sample_t * sample_fragment = new sample_t[fragment_size];

	if( fragment_size <= _hdata->remaining_frames )
	{
		for( f_cnt_t frame = 0; frame < fragment_size; ++frame )
		{
			sample_fragment[frame] = _hdata->wave->a( offset
								+ frame )
						/ OUTPUT_SAMPLE_MULTIPLIER;
		}
	}
	else
	{
		for( f_cnt_t frame = 0; frame < _hdata->remaining_frames;
								++frame )
		{
			sample_fragment[frame] = _hdata->wave->a( offset
								+ frame )
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
}








singerBot::synThread::synThread( void ) :
	m_handle_semaphore( 1 ),
	m_synth_semaphore( 1 )
{
#ifndef QT3
	m_handle_semaphore.acquire();
	m_synth_semaphore.acquire();
#else
	m_handle_semaphore++;
	m_synth_semaphore++;
#endif
}




singerBot::synThread::~synThread()
{
#ifndef QT3
	m_handle_semaphore.release();
	m_synth_semaphore.release();
#else
	m_handle_semaphore--;
	m_synth_semaphore--;
#endif
}




void singerBot::synThread::run( void )
{
	const int load_init_files = 1;
	festival_initialize( load_init_files, FESTIVAL_HEAP_SIZE );

	festival_eval_command(
		"(define get_segment"
		"	(lambda (utt) (begin"
		"		(Initialize utt)"
		"		(Text utt)"
		"		(Token_POS utt)"
		"		(Token utt)"
		"		(POS utt)"
		"		(Phrasify utt)"
		"		(Word utt)"
		"	))"
		")" );

	festival_eval_command(
		"(Parameter.set 'Int_Method 'DuffInt)" );
	festival_eval_command(
		"(Parameter.set 'Int_Target_Method Int_Targets_Default)" );

	for( ; ; )
	{
#ifndef QT3
		m_synth_semaphore.acquire();
#else
		m_synth_semaphore++;
#endif
		text_to_wave();
		if( !m_data->wave )
		{
			// Damaged SIOD environment? Retrying...
			text_to_wave();
			if( !m_data->wave )
			{
				printf( "Unsupported frequency?\n" );
			}
		}
#ifndef QT3
		m_handle_semaphore.release();
#else
		m_handle_semaphore--;
#endif
	}
}




void singerBot::synThread::text_to_wave( void )
{
	char command[80];
	sprintf( command,
		"(set! duffint_params '((start %f) (end %f)))",
					m_data->frequency, m_data->frequency );
	festival_eval_command( command );
	festival_eval_command(
		"(Parameter.set 'Duration_Stretch 1)" );

	sprintf( command,
		"(set! total_time (parse-number %f))", m_data->duration );
	festival_eval_command( command );
	festival_eval_command(
		"(set! word " + quote_string( m_data->text, "\"", "\\", 1 )
									+ ")" );
	if( festival_eval_command(
		"(begin"
		" (set! my_utt (eval (list 'Utterance 'Text word)))"
		" (get_segment my_utt)"
		" (if (equal? (length (utt.relation.leafs my_utt 'Segment)) 1)"
		"  (begin (set! my_utt (eval "
		"   (list 'Utterance 'Text (string-append word \" \" word))))"
		"   (get_segment my_utt)"
		"  ))"
		" (Pauses my_utt)"
		" (item.delete (utt.relation.first my_utt 'Segment))"
		" (item.delete (utt.relation.last my_utt 'Segment))"
		" (Intonation my_utt)"
		" (PostLex my_utt)"
		" (Duration my_utt)"
		" (if (not (equal? total_time 0)) (begin"
		"  (set! utt_time"
		"   (item.feat (utt.relation.last my_utt 'Segment) 'end))"
		"  (Parameter.set 'Duration_Stretch (/ total_time utt_time))"
		"  (Duration my_utt)"
		"  ))"
		" (Int_Targets my_utt)"
		")" )

		&& festival_eval_command(
		"  (Wave_Synth my_utt)" ) )
	{
		m_data->wave = get_wave( "my_utt" );
	}
}




EST_Wave * singerBot::synThread::get_wave( const char * _name )
{
	LISP lutt = siod_get_lval( _name, NULL );
	if( !utterance_p( lutt ) )
	{
        	return( NULL );
	}

	EST_Relation *r = utterance( lutt )->relation( "Wave" );

	//TODO: This check is useless. The error is fatal.
	if ( !r || !r->head() )
	{
		return( NULL );
	}

	return( new EST_Wave( *wave( r->head()->f( "wave" ) ) ) );
}




#include "singerbot.moc"
