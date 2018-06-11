/*
 * SampleBuffer.cpp - container-class SampleBuffer
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

#include "SampleBuffer.h"


#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QDomElement>


#include <sndfile.h>

#define OV_EXCLUDE_STATIC_CALLBACKS
#ifdef LMMS_HAVE_OGGVORBIS
#include <vorbis/vorbisfile.h>
#endif

#include "base64.h"
#include "ConfigManager.h"
#include "DrumSynth.h"
#include "endian_handling.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Mixer.h"

#include "FileDialog.h"

SampleBuffer::SampleBuffer() :
	m_audioFile( "" ),
	m_startFrame( 0 ),
	m_endFrame( 0 ),
	m_loopStartFrame( 0 ),
	m_loopEndFrame( 0 ),
	m_amplification( 1.0f ),
	m_frequency( BaseFreq ),
	m_sampleRate( mixerSampleRate () )
{

	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( sampleRateChanged() ) );
	beginBufferChange (false);
	doneBufferChange (false, /* shouldLock */
					  m_sampleRate);
}



SampleBuffer::SampleBuffer( const QString & _audio_file,
							bool _is_base64_data, sample_rate_t sampleRate )
	: SampleBuffer()
{
	if( _is_base64_data )
	{
		loadFromBase64( _audio_file, sampleRate );
	}
	else
	{
		changeAudioFile (_audio_file);
	}
}


SampleBuffer::SampleBuffer(SampleBuffer::DataVector &&movedData , sample_rate_t sampleRate) :
	SampleBuffer()
{
	resetData (std::move(movedData), sampleRate, false);
}

void SampleBuffer::saveSettings(QDomDocument &doc, QDomElement &_this) {
	{
		QString string;
		_this.setAttribute( "data", toBase64(string) );
	}

	_this.setAttribute ("sampleRate", sampleRate ());
	_this.setAttribute ("amplification", amplification ());
	_this.setAttribute ("frequency", frequency ());
}

void SampleBuffer::loadSettings(const QDomElement &_this) {
	if (_this.hasAttribute ("sampleRate")) {
		m_sampleRate = _this.attribute ("sampleRate").toUInt ();
	} else {
		qWarning("SampleBuffer::loadSettings: Using default sampleRate. That could lead to invalid values");
	}

	if (_this.hasAttribute ("amplification")) {
		m_amplification = _this.attribute ("amplification").toFloat ();
	}

	if (_this.hasAttribute ("frequency")) {
		m_frequency = _this.attribute ("frequency").toFloat ();
	}

	if (_this.hasAttribute ("data")) {
		loadFromBase64 (_this.attribute("data"), m_sampleRate);
	}
}

void SampleBuffer::sampleRateChanged() {
	auto previousSampleRate = sampleRate ();
	auto requiredSampleRate = mixerSampleRate ();

	if (requiredSampleRate == sampleRate ())
		return;

	// Only resample the buffer if the processing
	// sample rate is higher than the SampleBuffer's
	// sample rate.
	if (requiredSampleRate > sampleRate ()) {
		resetData (resampleData (m_data, previousSampleRate, requiredSampleRate),
				   requiredSampleRate,
				   true);
	}
}

void SampleBuffer::internalAddData(const DataVector &vector, sample_rate_t sampleRate)
{
	Q_ASSERT(sampleRate == m_sampleRate);
	m_data.insert (m_data.end (), vector.cbegin (), vector.cend ());
}

void SampleBuffer::internalResetData(DataVector &&newData, sample_rate_t dataSampleRate) {
	Q_UNUSED(dataSampleRate);
	m_audioFile = QString();
	m_data = std::move (newData);
}

sample_rate_t SampleBuffer::mixerSampleRate()
{
	return Engine::mixer ()->processingSampleRate ();
}

void SampleBuffer::changeAudioFile(QString audioFile)
{
	if (audioFile == "")
		return;

	// File size and sample length limits
	const int fileSizeMax = 300; // MB
	const int sampleLengthMax = 90; // Minutes

	bool fileLoadError = false;
	QString file = tryToMakeAbsolute( audioFile );
	#ifdef LMMS_BUILD_WIN32
		char * f = qstrdup( file.toLocal8Bit().constData() );
	#else
		char * f = qstrdup( file.toUtf8().constData() );
	#endif

	ch_cnt_t channels = DEFAULT_CHANNELS;
	sample_rate_t samplerate = mixerSampleRate ();
	DataVector fileData;

	const QFileInfo fileInfo( file );
	if( fileInfo.size() > fileSizeMax * 1024 * 1024 )
	{
		fileLoadError = true;
	}

	if (!fileLoadError) {
		SNDFILE * snd_file;
		SF_INFO sf_info;
		sf_info.format = 0;
		if( ( snd_file = sf_open( f, SFM_READ, &sf_info ) ) != NULL )
		{
			f_cnt_t frames = sf_info.frames;
			int rate = sf_info.samplerate;
			if( frames / rate > sampleLengthMax * 60 )
			{
				fileLoadError = true;
			}
			sf_close( snd_file );
		}
	}

	QString loadingWarning;
	if( !fileLoadError ) {

#ifdef LMMS_HAVE_OGGVORBIS
		// workaround for a bug in libsndfile or our libsndfile decoder
		// causing some OGG files to be distorted -> try with OGG Vorbis
		// decoder first if filename extension matches "ogg"
		if( fileInfo.suffix() == "ogg" )
		{
			fileData = decodeSampleOGGVorbis( f, channels, samplerate );
		}
#endif
		if(fileData.empty ())
		{
			fileData = decodeSampleSF( f, channels, samplerate, loadingWarning );
		}
#ifdef LMMS_HAVE_OGGVORBIS
		if( fileData.empty () )
		{
			fileData = decodeSampleOGGVorbis( f, channels, samplerate );
		}
#endif
		if( fileData.empty () )
		{
			fileData = decodeSampleDS( f, channels, samplerate );
		}

		delete[] f;
	}

	if (fileData.empty ()) {
		fileLoadError = true;
	}

	if (! fileLoadError) {
		resetData (std::move(fileData), samplerate);
	} else {
		QString title = tr( "Fail to open file" );
		QString message = tr( "Audio files are limited to %1 MB "
							  "in size and %2 minutes of playing time"
							  ).arg( fileSizeMax ).arg( sampleLengthMax );
		if (! loadingWarning.isEmpty())
			message = loadingWarning;
		if( gui )
		{
			QMessageBox::information( NULL,
									  title, message,	QMessageBox::Warning );
		}
		else
		{
			fprintf( stderr, "%s\n", message.toUtf8().constData() );
			exit( EXIT_FAILURE );
		}
	}
}

SampleBuffer::DataVector SampleBuffer::convertIntToFloat (int_sample_t * & _ibuf, f_cnt_t _frames, int _channels)
{
	// following code transforms int-samples into
	// float-samples and does amplifying & reversing
	const float fac = 1 / OUTPUT_SAMPLE_MULTIPLIER;
	DataVector vector(_frames);
	const int ch = ( _channels > 1 ) ? 1 : 0;

	int idx = 0;
	for( f_cnt_t frame = 0; frame < _frames;
					++frame )
	{
		vector[frame][0] = _ibuf[idx+0] * fac;
		vector[frame][1] = _ibuf[idx+ch] * fac;
		idx += _channels;
	}

	delete[] _ibuf;

	return vector;
}


SampleBuffer::DataVector
SampleBuffer::resampleData (const DataVector &inputData, sample_rate_t inputSampleRate,
							sample_rate_t requiredSampleRate)
{
	const f_cnt_t dst_frames = static_cast<f_cnt_t>( inputData.size ()/
					(float) inputSampleRate * (float) requiredSampleRate );
	DataVector outputData(dst_frames);

	// yeah, libsamplerate, let's rock with sinc-interpolation!
	int error;
	SRC_STATE * state;
	if( ( state = src_new( SRC_SINC_MEDIUM_QUALITY,
					DEFAULT_CHANNELS, &error ) ) != NULL )
	{
		SRC_DATA src_data;
		src_data.end_of_input = 1;
		src_data.data_in = libSampleRateSrc(inputData.data ())->data ();
		src_data.data_out = outputData.data ()->data();
		src_data.input_frames = inputData.size ();
		src_data.output_frames = dst_frames;
		src_data.src_ratio = (double) requiredSampleRate / inputSampleRate;
		if( ( error = src_process( state, &src_data ) ) )
		{
			printf( "SampleBuffer: error while resampling: %s\n",
							src_strerror( error ) );
		}
		src_delete( state );
	}
	else
	{
		printf( "Error: src_new() failed in sample_buffer.cpp!\n" );
	}

	return outputData;
}

SampleBuffer::DataVector SampleBuffer::decodeSampleSF( const char * _f,
													   ch_cnt_t & _channels,
													   sample_rate_t &_samplerate,
													   QString &loadingWarning)
{
	SNDFILE * snd_file;
	SF_INFO sf_info;
	sf_info.format = 0;
	f_cnt_t frames = 0;
	DataVector vector;
	bool sf_rr = false;

	if( ( snd_file = sf_open( _f, SFM_READ, &sf_info ) ) != NULL )
	{
		frames = sf_info.frames;
		vector.resize (frames);
		sf_rr = sf_read_float( snd_file, vector.data ()->data (), min(DEFAULT_CHANNELS * frames, sf_info.channels * frames));

		if (sf_info.channels == 1) {
#ifdef DEBUG_LMMS
			qDebug( "SampleBuffer::decodeSampleSF(): Not a stereo file: %s: %s", _f, sf_strerror( NULL ) );
#endif
			vector.resize(frames / 2);
		} else if (sf_info.channels > DEFAULT_CHANNELS) {
			loadingWarning = tr("The file you've selected has %1 channels. LMMS support "
												  "Stereo and Mono.").arg(sf_info.channels);
		}

		if( sf_rr < sf_info.channels * frames )
		{
#ifdef DEBUG_LMMS
			qDebug( "SampleBuffer::decodeSampleSF(): could not read"
				" sample %s: %s", _f, sf_strerror( NULL ) );
#endif
		}
		_channels = sf_info.channels;
		_samplerate = sf_info.samplerate;

		sf_close( snd_file );
	}
	else
	{
#ifdef DEBUG_LMMS
		qDebug( "SampleBuffer::decodeSampleSF(): could not load "
				"sample %s: %s", _f, sf_strerror( NULL ) );
#endif
		loadingWarning = tr("SoundFile: Could not load: %1").arg(sf_strerror( NULL ));
	}

	return vector;
}




#ifdef LMMS_HAVE_OGGVORBIS

// callback-functions for reading ogg-file

size_t qfileReadCallback( void * _ptr, size_t _size, size_t _n, void * _udata )
{
	return static_cast<QFile *>( _udata )->read( (char*) _ptr,
								_size * _n );
}




int qfileSeekCallback( void * _udata, ogg_int64_t _offset, int _whence )
{
	QFile * f = static_cast<QFile *>( _udata );

	if( _whence == SEEK_CUR )
	{
		f->seek( f->pos() + _offset );
	}
	else if( _whence == SEEK_END )
	{
		f->seek( f->size() + _offset );
	}
	else
	{
		f->seek( _offset );
	}
	return 0;
}




int qfileCloseCallback( void * _udata )
{
	delete static_cast<QFile *>( _udata );
	return 0;
}




long qfileTellCallback( void * _udata )
{
	return static_cast<QFile *>( _udata )->pos();
}




SampleBuffer::DataVector SampleBuffer::decodeSampleOGGVorbis(const char * _f,
						ch_cnt_t & _channels,
						sample_rate_t & _samplerate)
{
	static ov_callbacks callbacks =
	{
		qfileReadCallback,
		qfileSeekCallback,
		qfileCloseCallback,
		qfileTellCallback
	} ;

	OggVorbis_File vf;

	f_cnt_t frames = 0;

	QFile * f = new QFile( _f );
	if( f->open( QFile::ReadOnly ) == false )
	{
		delete f;
		return {};
	}

	int err = ov_open_callbacks( f, &vf, NULL, 0, callbacks );

	if( err < 0 )
	{
		switch( err )
		{
			case OV_EREAD:
				printf( "SampleBuffer::decodeSampleOGGVorbis():"
						" media read error\n" );
				break;
			case OV_ENOTVORBIS:
/*				printf( "SampleBuffer::decodeSampleOGGVorbis():"
					" not an Ogg Vorbis file\n" );*/
				break;
			case OV_EVERSION:
				printf( "SampleBuffer::decodeSampleOGGVorbis():"
						" vorbis version mismatch\n" );
				break;
			case OV_EBADHEADER:
				printf( "SampleBuffer::decodeSampleOGGVorbis():"
					" invalid Vorbis bitstream header\n" );
				break;
			case OV_EFAULT:
				printf( "SampleBuffer::decodeSampleOgg(): "
					"internal logic fault\n" );
				break;
		}
		delete f;
		return {};
	}

	ov_pcm_seek( &vf, 0 );

   	_channels = ov_info( &vf, -1 )->channels;
   	_samplerate = ov_info( &vf, -1 )->rate;

	ogg_int64_t total = ov_pcm_total( &vf, -1 );

	auto _buf = new int_sample_t[total * _channels];
	int bitstream = 0;
	long bytes_read = 0;

	do
	{
		bytes_read = ov_read( &vf, (char *) &_buf[frames * _channels],
					( total - frames ) * _channels *
							BYTES_PER_INT_SAMPLE,
					isLittleEndian() ? 0 : 1,
					BYTES_PER_INT_SAMPLE, 1, &bitstream );
		if( bytes_read < 0 )
		{
			break;
		}
		frames += bytes_read / ( _channels * BYTES_PER_INT_SAMPLE );
	}
	while( bytes_read != 0 && bitstream == 0 );

	ov_clear( &vf );
	// if buffer isn't empty, convert it to float and write it down

	if ( frames > 0 && _buf != NULL )
	{
		return convertIntToFloat ( _buf, frames, _channels);
	}

	return {};
}
#endif




SampleBuffer::DataVector SampleBuffer::decodeSampleDS(const char * _f, ch_cnt_t & _channels,
						sample_rate_t & _samplerate)
{
	DrumSynth ds;
	int_sample_t *_buf = NULL;
	f_cnt_t frames = ds.GetDSFileSamples( _f, _buf, _channels, _samplerate );

	if ( frames > 0 && _buf != NULL )
	{
		return convertIntToFloat ( _buf, frames, _channels);
	}

	return {};

}




bool SampleBuffer::play( sampleFrame * _ab, handleState * _state,
					const fpp_t _frames,
					const float _freq,
					const LoopMode _loopmode )
{
	f_cnt_t startFrame = m_startFrame;
	f_cnt_t endFrame = m_endFrame;
	f_cnt_t loopStartFrame = m_loopStartFrame;
	f_cnt_t loopEndFrame = m_loopEndFrame;

	if( endFrame == 0 || _frames == 0 )
	{
		return false;
	}

	// variable for determining if we should currently be playing backwards in a ping-pong loop
	bool is_backwards = _state->isBackwards();

	const double freq_factor = (double) _freq / (double) m_frequency *
		 double(m_sampleRate) / double(Engine::mixer()->processingSampleRate());

	// calculate how many frames we have in requested pitch
	const f_cnt_t total_frames_for_current_pitch = static_cast<f_cnt_t>( (
						endFrame - startFrame ) /
								freq_factor );

	if( total_frames_for_current_pitch == 0 )
	{
		return false;
	}


	// this holds the index of the first frame to play
	f_cnt_t play_frame = qMax(_state->m_frameIndex, startFrame);

	if( _loopmode == LoopOff )
	{
		if( play_frame >= endFrame || ( endFrame - play_frame ) / freq_factor == 0 )
		{
			// the sample is done being played
			return false;
		}
	}
	else if( _loopmode == LoopOn )
	{
		play_frame = getLoopedIndex( play_frame, loopStartFrame, loopEndFrame );
	}
	else
	{
		play_frame = getPingPongIndex( play_frame, loopStartFrame, loopEndFrame );
	}

	f_cnt_t fragment_size = (f_cnt_t)( _frames * freq_factor ) + MARGIN[ _state->interpolationMode() ];

	sampleFrame * tmp = NULL;

	// check whether we have to change pitch...
	if( freq_factor != 1.0 || _state->m_varyingPitch )
	{
		SRC_DATA src_data;
		// Generate output
		src_data.data_in =
			libSampleRateSrc(getSampleFragment( play_frame, fragment_size, _loopmode, &tmp, &is_backwards,
			loopStartFrame, loopEndFrame, endFrame ))->data ();
		src_data.data_out = _ab->data ();
		src_data.input_frames = fragment_size;
		src_data.output_frames = _frames;
		src_data.src_ratio =  double(Engine::mixer()->processingSampleRate())/ double(m_sampleRate);
		src_data.end_of_input = 0;
		int error = src_process( _state->m_resamplingData,
								&src_data );
		if( error )
		{
			printf( "SampleBuffer: error while resampling: %s\n",
							src_strerror( error ) );
		}
		if( src_data.output_frames_gen > _frames )
		{
			printf( "SampleBuffer: not enough frames: %ld / %d\n",
					src_data.output_frames_gen, _frames );
		}
		// Advance
		switch( _loopmode )
		{
			case LoopOff:
				play_frame += src_data.input_frames_used;
				break;
			case LoopOn:
				play_frame += src_data.input_frames_used;
				play_frame = getLoopedIndex( play_frame, loopStartFrame, loopEndFrame );
				break;
			case LoopPingPong:
			{
				f_cnt_t left = src_data.input_frames_used;
				if( _state->isBackwards() )
				{
					play_frame -= src_data.input_frames_used;
					if( play_frame < loopStartFrame )
					{
						left -= ( loopStartFrame - play_frame );
						play_frame = loopStartFrame;
					}
					else left = 0;
				}
				play_frame += left;
				play_frame = getPingPongIndex( play_frame, loopStartFrame, loopEndFrame  );
				break;
			}
		}
	}
	else
	{
		// we don't have to pitch, so we just copy the sample-data
		// as is into pitched-copy-buffer

		// Generate output
		memcpy( _ab,
			getSampleFragment( play_frame, _frames, _loopmode, &tmp, &is_backwards,
						loopStartFrame, loopEndFrame, endFrame ),
						_frames * BYTES_PER_FRAME );
		// Advance
		switch( _loopmode )
		{
			case LoopOff:
				play_frame += _frames;
				break;
			case LoopOn:
				play_frame += _frames;
				play_frame = getLoopedIndex( play_frame, loopStartFrame, loopEndFrame  );
				break;
			case LoopPingPong:
			{
				f_cnt_t left = _frames;
				if( _state->isBackwards() )
				{
					play_frame -= _frames;
					if( play_frame < loopStartFrame )
					{
						left -= ( loopStartFrame - play_frame );
						play_frame = loopStartFrame;
					}
					else left = 0;
				}
				play_frame += left;
				play_frame = getPingPongIndex( play_frame, loopStartFrame, loopEndFrame  );
				break;
			}
		}
	}

	if( tmp != NULL ) 
	{
		MM_FREE( tmp );
	}

	_state->setBackwards( is_backwards );
	_state->setFrameIndex( play_frame );

	for( fpp_t i = 0; i < _frames; ++i )
	{
		_ab[i][0] *= m_amplification;
		_ab[i][1] *= m_amplification;
	}

	return true;
}



const
sampleFrame * SampleBuffer::getSampleFragment( f_cnt_t _index,
		f_cnt_t _frames, LoopMode _loopmode, sampleFrame * * _tmp, bool * _backwards,
		f_cnt_t _loopstart, f_cnt_t _loopend, f_cnt_t _end ) const
{
	if( _loopmode == LoopOff )
	{
		if( _index + _frames <= _end )
		{
			return data () + _index;
		}
	}
	else if( _loopmode == LoopOn )
	{
		if( _index + _frames <= _loopend )
		{
			return data () + _index;
		}
	}
	else
	{
		if( ! *_backwards && _index + _frames < _loopend )
		{
			return data () + _index;
		}
	}

	*_tmp = MM_ALLOC( sampleFrame, _frames );

	if( _loopmode == LoopOff )
	{
		f_cnt_t available = _end - _index;
		memcpy( *_tmp, data () + _index, available * BYTES_PER_FRAME );
		memset( *_tmp + available, 0, ( _frames - available ) *
							BYTES_PER_FRAME );
	}
	else if( _loopmode == LoopOn )
	{
		f_cnt_t copied = qMin( _frames, _loopend - _index );
		memcpy( *_tmp, data () + _index, copied * BYTES_PER_FRAME );
		f_cnt_t loop_frames = _loopend - _loopstart;
		while( copied < _frames )
		{
			f_cnt_t todo = qMin( _frames - copied, loop_frames );
			memcpy( *_tmp + copied, data () + _loopstart, todo * BYTES_PER_FRAME );
			copied += todo;
		}
	}
	else
	{
		f_cnt_t pos = _index;
		bool backwards = pos < _loopstart
			? false
			: *_backwards;
		f_cnt_t copied = 0;


		if( backwards )
		{
			copied = qMin( _frames, pos - _loopstart );
			for( int i=0; i < copied; i++ )
			{
				(*_tmp)[i][0] = m_data[ pos - i ][0];
				(*_tmp)[i][1] = m_data[ pos - i ][1];
			}
			pos -= copied;
			if( pos == _loopstart ) backwards = false;
		}
		else
		{
			copied = qMin( _frames, _loopend - pos );
			memcpy( *_tmp, data () + pos, copied * BYTES_PER_FRAME );
			pos += copied;
			if( pos == _loopend ) backwards = true;
		}

		while( copied < _frames )
		{
			if( backwards )
			{
				f_cnt_t todo = qMin( _frames - copied, pos - _loopstart );
				for ( int i=0; i < todo; i++ )
				{
					(*_tmp)[ copied + i ][0] = m_data[ pos - i ][0];
					(*_tmp)[ copied + i ][1] = m_data[ pos - i ][1];
				}
				pos -= todo;
				copied += todo;
				if( pos <= _loopstart ) backwards = false;
			}
			else
			{
				f_cnt_t todo = qMin( _frames - copied, _loopend - pos );
				memcpy( *_tmp + copied, data () + pos, todo * BYTES_PER_FRAME );
				pos += todo;
				copied += todo;
				if( pos >= _loopend ) backwards = true;
			}
		}
		*_backwards = backwards;
	}

	return *_tmp;
}




f_cnt_t SampleBuffer::getLoopedIndex( f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf ) const
{
	if( _index < _endf )
	{
		return _index;
	}
	return _startf + ( _index - _startf )
				% ( _endf - _startf );
}


f_cnt_t SampleBuffer::getPingPongIndex( f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf ) const
{
	if( _index < _endf )
	{
		return _index;
	}
	const f_cnt_t looplen = _endf - _startf;
	const f_cnt_t looppos = ( _index - _endf ) % ( looplen*2 );

	return ( looppos < looplen )
		? _endf - looppos
		: _startf + ( looppos - looplen );
}


void SampleBuffer::visualize( QPainter & _p, const QRect & _dr,
							const QRect & _clip, f_cnt_t _from_frame, f_cnt_t _to_frame )
{
	auto polyPair = visualizeToPoly (_dr, _clip, _from_frame, _to_frame);

	_p.setRenderHint( QPainter::Antialiasing );
	_p.drawPolyline (polyPair.first);
	_p.drawPolyline (polyPair.second);
}

QPair<QPolygonF, QPolygonF> SampleBuffer::visualizeToPoly(const QRect &_dr, const QRect &_clip,
														  f_cnt_t _from_frame, f_cnt_t _to_frame) const
{
	if( internalFrames () == 0 ) return {};

	const bool focus_on_range = _from_frame < _to_frame;
	if (_to_frame > frames())
		_to_frame = frames();

	//_p.setClipRect( _clip );
	const int w = _dr.width();
	const int h = _dr.height();

//	const int yb = h / 2 + _dr.y();
	int y_space = (h/2);

	const int nb_frames = focus_on_range ? _to_frame - _from_frame : internalFrames();
	if (nb_frames == 0) return {};

	const int fpp = tLimit<int>( nb_frames / w, 1, 20 );

	bool shouldAddAdditionalPoint  = (nb_frames % fpp) != 0;
	int pointsCount = (nb_frames / fpp) + (shouldAddAdditionalPoint ? 1 : 0);
	auto l = QPolygonF(pointsCount);
	auto r = QPolygonF(pointsCount);

	int n = 0;
	const int xb = _dr.x();
	const int first = focus_on_range ? _from_frame : 0;
	const int last = focus_on_range ? _to_frame : internalFrames();

	int zeroPoint = _dr.y() + y_space;
	if (h % 2 != 0)
		zeroPoint += 1;
	for( int frame = first; frame < last; frame += fpp )
	{
		double x = (xb + (frame - first) * double( w ) / nb_frames);

		l[n] = QPointF(x,
						( zeroPoint + ( m_data[frame][0] * y_space * m_amplification ) ) );
		r[n] = QPointF(x,
						( zeroPoint + ( m_data[frame][1] * y_space * m_amplification ) ) );

		++n;
	}

	return {std::move(l), std::move(r)};
}




QString SampleBuffer::openAudioFile() const
{
	FileDialog ofd( NULL, tr( "Open audio file" ) );

	QString dir;
	if( !m_audioFile.isEmpty() )
	{
		QString f = m_audioFile;
		if( QFileInfo( f ).isRelative() )
		{
			f = ConfigManager::inst()->userSamplesDir() + f;
			if( QFileInfo( f ).exists() == false )
			{
				f = ConfigManager::inst()->factorySamplesDir() +
								m_audioFile;
			}
		}
		dir = QFileInfo( f ).absolutePath();
	}
	else
	{
		dir = ConfigManager::inst()->userSamplesDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	ofd.setFileMode( FileDialog::ExistingFiles );

	// set filters
	QStringList types;
	types << tr( "All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc "
					"*.aif *.aiff *.au *.raw)" )
		<< tr( "Wave-Files (*.wav)" )
		<< tr( "OGG-Files (*.ogg)" )
		<< tr( "DrumSynth-Files (*.ds)" )
		<< tr( "FLAC-Files (*.flac)" )
		<< tr( "SPEEX-Files (*.spx)" )
		//<< tr( "MP3-Files (*.mp3)" )
		//<< tr( "MIDI-Files (*.mid)" )
		<< tr( "VOC-Files (*.voc)" )
		<< tr( "AIFF-Files (*.aif *.aiff)" )
		<< tr( "AU-Files (*.au)" )
		<< tr( "RAW-Files (*.raw)" )
		//<< tr( "MOD-Files (*.mod)" )
		;
	ofd.setNameFilters( types );
	if( !m_audioFile.isEmpty() )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_audioFile ).fileName() );
	}

	if( ofd.exec () == QDialog::Accepted )
	{
		if( ofd.selectedFiles().isEmpty() )
		{
			return QString::null;
		}
		return tryToMakeRelative( ofd.selectedFiles()[0] );
	}

	return QString::null;
}




QString SampleBuffer::openAndSetAudioFile()
{
	QString fileName = this->openAudioFile();

	if(!fileName.isEmpty())
	{
		this->setAudioFile( fileName );
	}

	return fileName;
}


QString SampleBuffer::openAndSetWaveformFile()
{
	if( m_audioFile.isEmpty() )
	{
		m_audioFile = ConfigManager::inst()->factorySamplesDir() + "waveforms/10saw.flac";
	}

	QString fileName = this->openAudioFile();

	if(!fileName.isEmpty())
	{
		this->setAudioFile( fileName );
	}
	else
	{
		m_audioFile = "";
	}

	return fileName;
}



QString & SampleBuffer::toBase64( QString & _dst ) const
{
	base64::encode( (const char *) data (),
					internalFrames()  * sizeof( sampleFrame ), _dst );

	return _dst;
}


void SampleBuffer::setAudioFile( const QString & _audio_file )
{
	changeAudioFile (_audio_file);
}



void SampleBuffer::loadFromBase64( const QString & _data , sample_rate_t sampleRate)
{
	char * dst = NULL;
	int dsize = 0;
	base64::decode( _data, &dst, &dsize );

	DataVector input(dsize / sizeof(sampleFrame));
	memcpy (input.data (),
			dst,
			input.size () * sizeof (sampleFrame));

	delete[] dst;

	resetData (std::move(input),
			   sampleRate);
}




void SampleBuffer::setStartFrame( const f_cnt_t _s )
{
	m_startFrame = _s;
}




void SampleBuffer::setEndFrame( const f_cnt_t _e )
{
	m_endFrame = _e;
}




void SampleBuffer::setAmplification( float _a )
{
	m_amplification = _a;

	emit sampleUpdated();
}

QString SampleBuffer::tryToMakeRelative( const QString & file )
{
	if( QFileInfo( file ).isRelative() == false )
	{
		// Normalize the path
		QString f( QDir::cleanPath( file ) );

		// First, look in factory samples
		// Isolate "samples/" from "data:/samples/"
		QString samplesSuffix = ConfigManager::inst()->factorySamplesDir().mid( ConfigManager::inst()->dataDir().length() );

		// Iterate over all valid "data:/" searchPaths
		for ( const QString & path : QDir::searchPaths( "data" ) )
		{
			QString samplesPath = QDir::cleanPath( path + samplesSuffix ) + "/";
			if ( f.startsWith( samplesPath ) )
			{
				return QString( f ).mid( samplesPath.length() );
			}
		}

		// Next, look in user samples
		QString usd = ConfigManager::inst()->userSamplesDir();
		usd.replace( QDir::separator(), '/' );
		if( f.startsWith( usd ) )
		{
			return QString( f ).mid( usd.length() );
		}
	}
	return file;
}




QString SampleBuffer::tryToMakeAbsolute(const QString& file)
{
	QFileInfo f(file);

	if(f.isRelative())
	{
		f = QFileInfo(ConfigManager::inst()->userSamplesDir() + file);

		if(! f.exists())
		{
			f = QFileInfo(ConfigManager::inst()->factorySamplesDir() + file);
		}
	}

	if (f.exists()) {
		return f.absoluteFilePath();
	}
	return file;
}

SampleBuffer::handleState::handleState( bool _varying_pitch, int interpolation_mode ) :
	m_frameIndex( 0 ),
	m_varyingPitch( _varying_pitch ),
	m_isBackwards( false )
{
	int error;
	m_interpolationMode = interpolation_mode;
	
	if( ( m_resamplingData = src_new( interpolation_mode, DEFAULT_CHANNELS, &error ) ) == NULL )
	{
		qDebug( "Error: src_new() failed in sample_buffer.cpp!\n" );
	}
}




SampleBuffer::handleState::~handleState()
{
	src_delete( m_resamplingData );
}

void SampleBuffer::beginBufferChange(bool shouldLock, bool shouldLockMixer)
{
	if (shouldLockMixer) {
		Engine::mixer ()->requestChangeInModel ();
	}

	if (shouldLock) {
		m_varLock.lockForWrite ();
	}
}

bool SampleBuffer::tryBeginBufferChange(bool shouldLock, bool shouldLockMixer) {
	bool result = true;

	if (shouldLockMixer) {
		Engine::mixer ()->requestChangeInModel ();
	}

	if (shouldLock) {
		result = m_varLock.tryLockForWrite();

		if (! result)
			Engine::mixer ()->doneChangeInModel();
	}

	return result;
}

void SampleBuffer::doneBufferChange(bool shouldUnlock,
									sample_rate_t bufferSampleRate,
									bool shouldUnlockMixer) {

	setSampleRate (bufferSampleRate);

	m_loopStartFrame = m_startFrame = 0;
	m_loopEndFrame = m_endFrame = internalFrames();
	if (shouldUnlock) {
		m_varLock.unlock ();
	}

	if (shouldUnlockMixer) {
		Engine::mixer ()->doneChangeInModel ();
	}

	emit sampleUpdated();
}

bool SampleBuffer::tryAddData(const SampleBuffer::DataVector &vector, sample_rate_t sampleRate, bool shouldLockMixer) {
	DataVector newVector;

	if (sampleRate != m_sampleRate) {
		// We should resample this data;

		newVector = resampleData (vector, sampleRate, m_sampleRate);
		sampleRate = m_sampleRate;
	}

	// First of all, don't let anyone read.
	if (! tryBeginBufferChange (true, shouldLockMixer))
		return false;
	{
		if (newVector.empty())
			internalAddData(vector,
							sampleRate);
		else
			internalAddData(newVector,
							sampleRate);
	}
	doneBufferChange (true, /* lock */
					  this->sampleRate(),
					  shouldLockMixer);

	return true;
}

void SampleBuffer::addData(const SampleBuffer::DataVector &vector, sample_rate_t sampleRate, bool shouldLockMixer) {
	DataVector newVector;

	if (sampleRate != m_sampleRate) {
		// We should resample this data;

		newVector = resampleData (vector, sampleRate, m_sampleRate);
		sampleRate = m_sampleRate;
	}

	// First of all, don't let anyone read.
	beginBufferChange (true, shouldLockMixer);
	{
		if (newVector.empty())
			internalAddData(vector,
							sampleRate);
		else
			internalAddData(newVector,
							sampleRate);
	}
	doneBufferChange (true, /* lock */
					  this->sampleRate(),
					  shouldLockMixer);
}

void SampleBuffer::resetData(DataVector &&newData, sample_rate_t dataSampleRate, bool shouldLockMixer) {
	beginBufferChange (true, shouldLockMixer);
	{
		internalResetData(std::move(newData), dataSampleRate);
	}
	doneBufferChange (true, /* lock */
					  dataSampleRate,
					  shouldLockMixer);
}

bool SampleBuffer::tryResetData(SampleBuffer::DataVector &&newData, sample_rate_t dataSampleRate, bool shouldLockMixer) {
	if (! tryBeginBufferChange (true, shouldLockMixer))
		return false;
	{
		internalResetData(std::move(newData), dataSampleRate);
	}
	doneBufferChange (true, /* lock */
					  dataSampleRate,
					  shouldLockMixer);

	return true;
}

void SampleBuffer::reverse(bool shouldLockMixer) {
	beginBufferChange (true, shouldLockMixer);
	{
		std::reverse(m_data.begin (), m_data.end ());
	}
	doneBufferChange (true, /* should(Un)Lock? yes! */
					  sampleRate (), /* we have not made any change in the sample rate. */
					  shouldLockMixer);
}
