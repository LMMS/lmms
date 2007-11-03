/*
 * singerbot_proxy.cpp - separate process to deal with Festival
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


#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "file.h"

#undef HAVE_CONFIG_H
#include <festival.h>


static File * s_shm;
static sem_t * s_handle_semaphore;
static sem_t * s_synth_semaphore;


static void run( void );
static EST_Wave * text_to_wave( float _frequency, float _duration,
							const char * _text );
static EST_Wave * get_wave( const char * _name );




int main( int argc, char * * argv )
{
	string resource = "/lmms_singerbot";
	resource += argv[1];
	int fd = shm_open( resource.c_str(), O_RDWR, S_IRUSR | S_IWUSR );
	s_shm = new File( fd );

	resource = "/lmms_singerbot_s1";
	resource += argv[1];
	s_handle_semaphore = sem_open( resource.c_str(), 0 );

	resource = "/lmms_singerbot_s2";
	resource += argv[1];
	s_synth_semaphore = sem_open( resource.c_str(), 0 );

	sem_post( s_handle_semaphore );

	run();

	sem_close( s_handle_semaphore );
	sem_close( s_synth_semaphore );

	delete s_shm;

	return( EXIT_SUCCESS );
}




void run( void )
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
		sem_wait( s_synth_semaphore );

		float frequency;
		float duration;

		s_shm->rewind();
		s_shm->read( &frequency );
		if( frequency == -1.0f )
		{
			break;
		}
		s_shm->read( &duration );
		unsigned char len;
		s_shm->read( &len );
		char * text = new char[len + 1];
		s_shm->read( text, len );
		text[len] = '\0';

		EST_Wave * wave = text_to_wave( frequency, duration, text );
		if( !wave )
		{
			// Damaged SIOD environment? Retrying...
			wave = text_to_wave( frequency, duration, text );
			if( !wave )
			{
				printf( "Unsupported frequency?\n" );
			}
		}

		s_shm->rewind();
		int num_samples = wave ? wave->num_samples() : 0;
		s_shm->write( &num_samples );
		if( num_samples )
		{
			int sample_rate = wave->sample_rate();
			s_shm->write( &sample_rate );
		}

		for( int i = 0; i < num_samples; ++i )
		{
			short sample = wave->a( i );
			s_shm->write( &sample );
		}
		delete wave;

		sem_post( s_handle_semaphore );
	}
}




EST_Wave * text_to_wave( float _frequency, float _duration, const char * _text )
{
	char command[80];
	sprintf( command,
		"(set! duffint_params '((start %f) (end %f)))", _frequency,
								_frequency );
	festival_eval_command( command );
	festival_eval_command(
		"(Parameter.set 'Duration_Stretch 1)" );

	sprintf( command,
		"(set! total_time (parse-number %f))", _duration );
	festival_eval_command( command );
	festival_eval_command(
		"(set! word " + quote_string( _text, "\"", "\\", 1 ) + ")" );
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
		return( get_wave( "my_utt" ) );
	}

	return( NULL );
}




EST_Wave * get_wave( const char * _name )
{
	LISP lutt = siod_get_lval( _name, NULL );
	if( !utterance_p( lutt ) )
	{
        	return( NULL );
	}

	EST_Relation * r = utterance( lutt )->relation( "Wave" );

	//TODO: This check is useless. The error is fatal.
	if ( !r || !r->head() )
	{
		return( NULL );
	}

	return( new EST_Wave( *wave( r->head()->f( "wave" ) ) ) );
}
