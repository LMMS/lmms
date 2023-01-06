/*
 * AudioFileProcessor.cpp - instrument for using audio-files
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioFileProcessor.h"

#include <QPainter>
#include <QFileInfo>
#include <QDropEvent>

#include <samplerate.h>

#include "AudioEngine.h"
#include "ComboBox.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "Engine.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "NotePlayHandle.h"
#include "PathUtil.h"
#include "PixmapButton.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "Clipboard.h"

#include "embed.h"
#include "plugin_export.h"


namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT audiofileprocessor_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"AudioFileProcessor",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Simple sampler with various settings for "
				"using samples (e.g. drums) in an "
				"instrument-track" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	"wav,ogg,ds,spx,au,voc,aif,aiff,flac,raw",
	nullptr,
} ;

}




AudioFileProcessor::AudioFileProcessor( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &audiofileprocessor_plugin_descriptor ),
	m_sampleBuffer(),
	m_ampModel( 100, 0, 500, 1, this, tr( "Amplify" ) ),
	m_startPointModel( 0, 0, 1, 0.0000001f, this, tr( "Start of sample" ) ),
	m_endPointModel( 1, 0, 1, 0.0000001f, this, tr( "End of sample" ) ),
	m_loopPointModel( 0, 0, 1, 0.0000001f, this, tr( "Loopback point" ) ),
	m_reverseModel( false, this, tr( "Reverse sample" ) ),
	m_loopModel( 0, 0, 2, this, tr( "Loop mode" ) ),
	m_stutterModel( false, this, tr( "Stutter" ) ),
	m_interpolationModel( this, tr( "Interpolation mode" ) ),
	m_nextPlayStartPoint( 0 ),
	m_nextPlayBackwards( false )
{
	connect( &m_reverseModel, SIGNAL( dataChanged() ),
				this, SLOT( reverseModelChanged() ), Qt::DirectConnection );
	connect( &m_ampModel, SIGNAL( dataChanged() ),
				this, SLOT( ampModelChanged() ), Qt::DirectConnection );
	connect( &m_startPointModel, SIGNAL( dataChanged() ),
				this, SLOT( startPointChanged() ), Qt::DirectConnection );
	connect( &m_endPointModel, SIGNAL( dataChanged() ),
				this, SLOT( endPointChanged() ), Qt::DirectConnection );
	connect( &m_loopPointModel, SIGNAL( dataChanged() ),
				this, SLOT( loopPointChanged() ), Qt::DirectConnection );
	connect( &m_stutterModel, SIGNAL( dataChanged() ),
				this, SLOT( stutterModelChanged() ), Qt::DirectConnection );

//interpolation modes
	m_interpolationModel.addItem( tr( "None" ) );
	m_interpolationModel.addItem( tr( "Linear" ) );
	m_interpolationModel.addItem( tr( "Sinc" ) );
	m_interpolationModel.setValue( 1 );

	pointChanged();
}




void AudioFileProcessor::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	// Magic key - a frequency < 20 (say, the bottom piano note if using
	// a A4 base tuning) restarts the start point. The note is not actually
	// played.
	if( m_stutterModel.value() == true && _n->frequency() < 20.0 )
	{
		m_nextPlayStartPoint = m_sampleBuffer.startFrame();
		m_nextPlayBackwards = false;
		return;
	}

	if( !_n->m_pluginData )
	{
		if( m_stutterModel.value() == true && m_nextPlayStartPoint >= m_sampleBuffer.endFrame() )
		{
			// Restart playing the note if in stutter mode, not in loop mode,
			// and we're at the end of the sample.
			m_nextPlayStartPoint = m_sampleBuffer.startFrame();
			m_nextPlayBackwards = false;
		}
		// set interpolation mode for libsamplerate
		int srcmode = SRC_LINEAR;
		switch( m_interpolationModel.value() )
		{
			case 0:
				srcmode = SRC_ZERO_ORDER_HOLD;
				break;
			case 1:
				srcmode = SRC_LINEAR;
				break;
			case 2:
				srcmode = SRC_SINC_MEDIUM_QUALITY;
				break;
		}
		_n->m_pluginData = new handleState( _n->hasDetuningInfo(), srcmode );
		((handleState *)_n->m_pluginData)->setFrameIndex( m_nextPlayStartPoint );
		((handleState *)_n->m_pluginData)->setBackwards( m_nextPlayBackwards );

// debug code
/*		qDebug( "frames %d", m_sampleBuffer.frames() );
		qDebug( "startframe %d", m_sampleBuffer.startFrame() );
		qDebug( "nextPlayStartPoint %d", m_nextPlayStartPoint );*/
	}

	if( ! _n->isFinished() )
	{
		if( m_sampleBuffer.play( _working_buffer + offset,
						(handleState *)_n->m_pluginData,
						frames, _n->frequency(),
						static_cast<SampleBuffer::LoopMode>( m_loopModel.value() ) ) )
		{
			applyRelease( _working_buffer, _n );
			instrumentTrack()->processAudioBuffer( _working_buffer,
									frames + offset, _n );

			emit isPlaying( ((handleState *)_n->m_pluginData)->frameIndex() );
		}
		else
		{
			memset( _working_buffer, 0, ( frames + offset ) * sizeof( sampleFrame ) );
			emit isPlaying( 0 );
		}
	}
	else
	{
		emit isPlaying( 0 );
	}
	if( m_stutterModel.value() == true )
	{
		m_nextPlayStartPoint = ((handleState *)_n->m_pluginData)->frameIndex();
		m_nextPlayBackwards = ((handleState *)_n->m_pluginData)->isBackwards();
	}
}




void AudioFileProcessor::deleteNotePluginData( NotePlayHandle * _n )
{
	delete (handleState *)_n->m_pluginData;
}




void AudioFileProcessor::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	_this.setAttribute( "src", m_sampleBuffer.audioFile() );
	if( m_sampleBuffer.audioFile() == "" )
	{
		QString s;
		_this.setAttribute( "sampledata",
						m_sampleBuffer.toBase64( s ) );
	}
	m_reverseModel.saveSettings( _doc, _this, "reversed" );
	m_loopModel.saveSettings( _doc, _this, "looped" );
	m_ampModel.saveSettings( _doc, _this, "amp" );
	m_startPointModel.saveSettings( _doc, _this, "sframe" );
	m_endPointModel.saveSettings( _doc, _this, "eframe" );
	m_loopPointModel.saveSettings( _doc, _this, "lframe" );
	m_stutterModel.saveSettings( _doc, _this, "stutter" );
	m_interpolationModel.saveSettings( _doc, _this, "interp" );

}




void AudioFileProcessor::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "src" ) != "" )
	{
		setAudioFile( _this.attribute( "src" ), false );

		QString absolutePath = PathUtil::toAbsolute( m_sampleBuffer.audioFile() );
		if ( !QFileInfo( absolutePath ).exists() )
		{
			QString message = tr( "Sample not found: %1" ).arg( m_sampleBuffer.audioFile() );

			Engine::getSong()->collectError( message );
		}
	}
	else if( _this.attribute( "sampledata" ) != "" )
	{
		m_sampleBuffer.loadFromBase64( _this.attribute( "srcdata" ) );
	}

	m_loopModel.loadSettings( _this, "looped" );
	m_ampModel.loadSettings( _this, "amp" );
	m_endPointModel.loadSettings( _this, "eframe" );
	m_startPointModel.loadSettings( _this, "sframe" );

	// compat code for not having a separate loopback point
	if (_this.hasAttribute("lframe") || !(_this.firstChildElement("lframe").isNull()))
	{
		m_loopPointModel.loadSettings( _this, "lframe" );
	}
	else
	{
		m_loopPointModel.loadSettings( _this, "sframe" );
	}

	m_reverseModel.loadSettings( _this, "reversed" );

	m_stutterModel.loadSettings( _this, "stutter" );
	if( _this.hasAttribute( "interp" ) )
	{
		m_interpolationModel.loadSettings( _this, "interp" );
	}
	else
	{
		m_interpolationModel.setValue( 1 ); //linear by default
	}

	pointChanged();
}




void AudioFileProcessor::loadFile( const QString & _file )
{
	setAudioFile( _file );
}




QString AudioFileProcessor::nodeName() const
{
	return audiofileprocessor_plugin_descriptor.name;
}




int AudioFileProcessor::getBeatLen( NotePlayHandle * _n ) const
{
	const auto baseFreq = instrumentTrack()->baseFreq();
	const float freq_factor = baseFreq / _n->frequency() *
			Engine::audioEngine()->processingSampleRate() / Engine::audioEngine()->baseSampleRate();

	return static_cast<int>( floorf( ( m_sampleBuffer.endFrame() - m_sampleBuffer.startFrame() ) * freq_factor ) );
}





gui::PluginView* AudioFileProcessor::instantiateView( QWidget * _parent )
{
	return new gui::AudioFileProcessorView( this, _parent );
}




void AudioFileProcessor::setAudioFile( const QString & _audio_file,
													bool _rename )
{
	// is current channel-name equal to previous-filename??
	if( _rename &&
		( instrumentTrack()->name() ==
			QFileInfo( m_sampleBuffer.audioFile() ).fileName() ||
				m_sampleBuffer.audioFile().isEmpty() ) )
	{
		// then set it to new one
		instrumentTrack()->setName( PathUtil::cleanName( _audio_file ) );
	}
	// else we don't touch the track-name, because the user named it self

	m_sampleBuffer.setAudioFile( _audio_file );
	loopPointChanged();
}




void AudioFileProcessor::reverseModelChanged()
{
	m_sampleBuffer.setReversed( m_reverseModel.value() );
	m_nextPlayStartPoint = m_sampleBuffer.startFrame();
	m_nextPlayBackwards = false;
}




void AudioFileProcessor::ampModelChanged()
{
	m_sampleBuffer.setAmplification( m_ampModel.value() / 100.0f );
}


void AudioFileProcessor::stutterModelChanged()
{
	m_nextPlayStartPoint = m_sampleBuffer.startFrame();
	m_nextPlayBackwards = false;
}


void AudioFileProcessor::startPointChanged()
{
	// check if start is over end and swap values if so
	if( m_startPointModel.value() > m_endPointModel.value() )
	{
		float tmp = m_endPointModel.value();
		m_endPointModel.setValue( m_startPointModel.value() );
		m_startPointModel.setValue( tmp );
	}

	// nudge loop point with end
	if( m_loopPointModel.value() >= m_endPointModel.value() )
	{
		m_loopPointModel.setValue( qMax( m_endPointModel.value() - 0.001f, 0.0f ) );
	}

	// nudge loop point with start
	if( m_loopPointModel.value() < m_startPointModel.value() )
	{
		m_loopPointModel.setValue( m_startPointModel.value() );
	}

	// check if start & end overlap and nudge end up if so
	if( m_startPointModel.value() == m_endPointModel.value() )
	{
		m_endPointModel.setValue( qMin( m_endPointModel.value() + 0.001f, 1.0f ) );
	}

	pointChanged();

}

void AudioFileProcessor::endPointChanged()
{
	// same as start, for now
	startPointChanged();

}

void AudioFileProcessor::loopPointChanged()
{

	// check that loop point is between start-end points and not overlapping with endpoint
	// ...and move start/end points ahead if loop point is moved over them
	if( m_loopPointModel.value() >= m_endPointModel.value() )
	{
		m_endPointModel.setValue( m_loopPointModel.value() + 0.001f );
		if( m_endPointModel.value() == 1.0f )
		{
			m_loopPointModel.setValue( 1.0f - 0.001f );
		}
	}

	// nudge start point with loop
	if( m_loopPointModel.value() < m_startPointModel.value() )
	{
		m_startPointModel.setValue( m_loopPointModel.value() );
	}

	pointChanged();
}

void AudioFileProcessor::pointChanged()
{
	const auto f_start = static_cast<f_cnt_t>(m_startPointModel.value() * m_sampleBuffer.frames());
	const auto f_end = static_cast<f_cnt_t>(m_endPointModel.value() * m_sampleBuffer.frames());
	const auto f_loop = static_cast<f_cnt_t>(m_loopPointModel.value() * m_sampleBuffer.frames());

	m_nextPlayStartPoint = f_start;
	m_nextPlayBackwards = false;

	m_sampleBuffer.setAllPointFrames( f_start, f_end, f_loop, f_end );
	emit dataChanged();
}




namespace gui
{


QPixmap * AudioFileProcessorView::s_artwork = nullptr;


AudioFileProcessorView::AudioFileProcessorView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	if( s_artwork == nullptr )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}

	m_openAudioFileButton = new PixmapButton( this );
	m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openAudioFileButton->move( 227, 72 );
	m_openAudioFileButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	m_openAudioFileButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	connect( m_openAudioFileButton, SIGNAL( clicked() ),
					this, SLOT( openAudioFile() ) );
	m_openAudioFileButton->setToolTip(tr("Open sample"));

	m_reverseButton = new PixmapButton( this );
	m_reverseButton->setCheckable( true );
	m_reverseButton->move( 164, 105 );
	m_reverseButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_on" ) );
	m_reverseButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_off" ) );
	m_reverseButton->setToolTip(tr("Reverse sample"));

// loop button group

	auto m_loopOffButton = new PixmapButton(this);
	m_loopOffButton->setCheckable( true );
	m_loopOffButton->move( 190, 105 );
	m_loopOffButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"loop_off_on" ) );
	m_loopOffButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"loop_off_off" ) );
	m_loopOffButton->setToolTip(tr("Disable loop"));

	auto m_loopOnButton = new PixmapButton(this);
	m_loopOnButton->setCheckable( true );
	m_loopOnButton->move( 190, 124 );
	m_loopOnButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"loop_on_on" ) );
	m_loopOnButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"loop_on_off" ) );
	m_loopOnButton->setToolTip(tr("Enable loop"));

	auto m_loopPingPongButton = new PixmapButton(this);
	m_loopPingPongButton->setCheckable( true );
	m_loopPingPongButton->move( 216, 124 );
	m_loopPingPongButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"loop_pingpong_on" ) );
	m_loopPingPongButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"loop_pingpong_off" ) );
	m_loopPingPongButton->setToolTip(tr("Enable ping-pong loop"));

	m_loopGroup = new automatableButtonGroup( this );
	m_loopGroup->addButton( m_loopOffButton );
	m_loopGroup->addButton( m_loopOnButton );
	m_loopGroup->addButton( m_loopPingPongButton );

	m_stutterButton = new PixmapButton( this );
	m_stutterButton->setCheckable( true );
	m_stutterButton->move( 164, 124 );
	m_stutterButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"stutter_on" ) );
	m_stutterButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"stutter_off" ) );
	m_stutterButton->setToolTip(
		tr( "Continue sample playback across notes" ) );

	m_ampKnob = new Knob( knobBright_26, this );
	m_ampKnob->setVolumeKnob( true );
	m_ampKnob->move( 5, 108 );
	m_ampKnob->setHintText( tr( "Amplify:" ), "%" );

	m_startKnob = new AudioFileProcessorWaveView::knob( this );
	m_startKnob->move( 45, 108 );
	m_startKnob->setHintText( tr( "Start point:" ), "" );

	m_endKnob = new AudioFileProcessorWaveView::knob( this );
	m_endKnob->move( 125, 108 );
	m_endKnob->setHintText( tr( "End point:" ), "" );

	m_loopKnob = new AudioFileProcessorWaveView::knob( this );
	m_loopKnob->move( 85, 108 );
	m_loopKnob->setHintText( tr( "Loopback point:" ), "" );

// interpolation selector
	m_interpBox = new ComboBox( this );
	m_interpBox->setGeometry( 142, 62, 82, ComboBox::DEFAULT_HEIGHT );
	m_interpBox->setFont( pointSize<8>( m_interpBox->font() ) );

// wavegraph
	m_waveView = 0;
	newWaveView();

	connect( castModel<AudioFileProcessor>(), SIGNAL( isPlaying( lmms::f_cnt_t ) ),
			m_waveView, SLOT( isPlaying( lmms::f_cnt_t ) ) );

	qRegisterMetaType<lmms::f_cnt_t>( "lmms::f_cnt_t" );

	setAcceptDrops( true );
}








void AudioFileProcessorView::dragEnterEvent( QDragEnterEvent * _dee )
{
	// For mimeType() and MimeType enum class
	using namespace Clipboard;

	if( _dee->mimeData()->hasFormat( mimeType( MimeType::StringPair ) ) )
	{
		QString txt = _dee->mimeData()->data(
						mimeType( MimeType::StringPair ) );
		if( txt.section( ':', 0, 0 ) == QString( "clip_%1" ).arg(
							Track::SampleTrack ) )
		{
			_dee->acceptProposedAction();
		}
		else if( txt.section( ':', 0, 0 ) == "samplefile" )
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




void AudioFileProcessorView::newWaveView()
{
	if ( m_waveView )
	{
		delete m_waveView;
		m_waveView = 0;
	}
	m_waveView = new AudioFileProcessorWaveView( this, 245, 75, castModel<AudioFileProcessor>()->m_sampleBuffer );
	m_waveView->move( 2, 172 );
	m_waveView->setKnobs(
		dynamic_cast<AudioFileProcessorWaveView::knob *>( m_startKnob ),
		dynamic_cast<AudioFileProcessorWaveView::knob *>( m_endKnob ),
		dynamic_cast<AudioFileProcessorWaveView::knob *>( m_loopKnob ) );
	m_waveView->show();
}




void AudioFileProcessorView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		castModel<AudioFileProcessor>()->setAudioFile( value );
		_de->accept();
		newWaveView();
		return;
	}
	else if( type == QString( "clip_%1" ).arg( Track::SampleTrack ) )
	{
		DataFile dataFile( value.toUtf8() );
		castModel<AudioFileProcessor>()->setAudioFile( dataFile.content().firstChild().toElement().attribute( "src" ) );
		_de->accept();
		return;
	}

	_de->ignore();
}




void AudioFileProcessorView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, *s_artwork );

	auto a = castModel<AudioFileProcessor>();

	QString file_name = "";
	int idx = a->m_sampleBuffer.audioFile().length();

	p.setFont( pointSize<8>( font() ) );

	QFontMetrics fm( p.font() );

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
	while( idx > 0 &&
		fm.size( Qt::TextSingleLine, file_name + "..." ).width() < 210 )
	{
		file_name = a->m_sampleBuffer.audioFile()[--idx] + file_name;
	}

	if( idx > 0 )
	{
		file_name = "..." + file_name;
	}

	p.setPen( QColor( 255, 255, 255 ) );
	p.drawText( 8, 99, file_name );
}




void AudioFileProcessorView::sampleUpdated()
{
	m_waveView->updateSampleRange();
	m_waveView->update();
	update();
}





void AudioFileProcessorView::openAudioFile()
{
	QString af = castModel<AudioFileProcessor>()->m_sampleBuffer.
							openAudioFile();
	if( af != "" )
	{
		castModel<AudioFileProcessor>()->setAudioFile( af );
		Engine::getSong()->setModified();
		m_waveView->updateSampleRange();
	}
}




void AudioFileProcessorView::modelChanged()
{
	auto a = castModel<AudioFileProcessor>();
	connect( &a->m_sampleBuffer, SIGNAL( sampleUpdated() ),
					this, SLOT( sampleUpdated() ) );
	m_ampKnob->setModel( &a->m_ampModel );
	m_startKnob->setModel( &a->m_startPointModel );
	m_endKnob->setModel( &a->m_endPointModel );
	m_loopKnob->setModel( &a->m_loopPointModel );
	m_reverseButton->setModel( &a->m_reverseModel );
	m_loopGroup->setModel( &a->m_loopModel );
	m_stutterButton->setModel( &a->m_stutterModel );
	m_interpBox->setModel( &a->m_interpolationModel );
	sampleUpdated();
}




void AudioFileProcessorWaveView::updateSampleRange()
{
	if( m_sampleBuffer.frames() > 1 )
	{
		const f_cnt_t marging = ( m_sampleBuffer.endFrame() - m_sampleBuffer.startFrame() ) * 0.1;
		m_from = qMax( 0, m_sampleBuffer.startFrame() - marging );
		m_to = qMin( m_sampleBuffer.endFrame() + marging, m_sampleBuffer.frames() );
	}
}

AudioFileProcessorWaveView::AudioFileProcessorWaveView( QWidget * _parent, int _w, int _h, SampleBuffer& buf ) :
	QWidget( _parent ),
	m_sampleBuffer( buf ),
	m_graph( QPixmap( _w - 2 * s_padding, _h - 2 * s_padding ) ),
	m_from( 0 ),
	m_to( m_sampleBuffer.frames() ),
	m_last_from( 0 ),
	m_last_to( 0 ),
	m_last_amp( 0 ),
	m_startKnob( 0 ),
	m_endKnob( 0 ),
	m_loopKnob( 0 ),
	m_isDragging( false ),
	m_reversed( false ),
	m_framesPlayed( 0 ),
	m_animation(ConfigManager::inst()->value("ui", "animateafp").toInt())
{
	setFixedSize( _w, _h );
	setMouseTracking( true );

	updateSampleRange();

	m_graph.fill( Qt::transparent );
	update();
	updateCursor();
}




void AudioFileProcessorWaveView::isPlaying( f_cnt_t _current_frame )
{
	m_framesPlayed = _current_frame;
	update();
}




void AudioFileProcessorWaveView::enterEvent( QEvent * _e )
{
	updateCursor();
}




void AudioFileProcessorWaveView::leaveEvent( QEvent * _e )
{
	updateCursor();
}




void AudioFileProcessorWaveView::mousePressEvent( QMouseEvent * _me )
{
	m_isDragging = true;
	m_draggingLastPoint = _me->pos();

	const int x = _me->x();

	const int start_dist =		qAbs( m_startFrameX - x );
	const int end_dist = 		qAbs( m_endFrameX - x );
	const int loop_dist =		qAbs( m_loopFrameX - x );

	draggingType dt = sample_loop; int md = loop_dist;
	if( start_dist < loop_dist ) { dt = sample_start; md = start_dist; }
	else if( end_dist < loop_dist ) { dt = sample_end; md = end_dist; }

	if( md < 4 )
	{
		m_draggingType = dt;
	}
	else
	{
		m_draggingType = wave;
		updateCursor(_me);
	}
}




void AudioFileProcessorWaveView::mouseReleaseEvent( QMouseEvent * _me )
{
	m_isDragging = false;
	if( m_draggingType == wave )
	{
		updateCursor(_me);
	}
}




void AudioFileProcessorWaveView::mouseMoveEvent( QMouseEvent * _me )
{
	if( ! m_isDragging )
	{
		updateCursor(_me);
		return;
	}

	const int step = _me->x() - m_draggingLastPoint.x();
	switch( m_draggingType )
	{
		case sample_start:
			slideSamplePointByPx( start, step );
			break;
		case sample_end:
			slideSamplePointByPx( end, step );
			break;
		case sample_loop:
			slideSamplePointByPx( loop, step );
			break;
		case wave:
		default:
			if( qAbs( _me->y() - m_draggingLastPoint.y() )
				< 2 * qAbs( _me->x() - m_draggingLastPoint.x() ) )
			{
				slide( step );
			}
			else
			{
				zoom( _me->y() < m_draggingLastPoint.y() );
			}
	}

	m_draggingLastPoint = _me->pos();
	update();
}




void AudioFileProcessorWaveView::wheelEvent( QWheelEvent * _we )
{
	zoom( _we->angleDelta().y() > 0 );
	update();
}




void AudioFileProcessorWaveView::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

	p.drawPixmap( s_padding, s_padding, m_graph );

	const QRect graph_rect( s_padding, s_padding, width() - 2 * s_padding, height() - 2 * s_padding );
	const f_cnt_t frames = m_to - m_from;
	m_startFrameX = graph_rect.x() + ( m_sampleBuffer.startFrame() - m_from ) *
						double( graph_rect.width() ) / frames;
	m_endFrameX = graph_rect.x() + ( m_sampleBuffer.endFrame() - m_from ) *
						double( graph_rect.width() ) / frames;
	m_loopFrameX = graph_rect.x() + ( m_sampleBuffer.loopStartFrame() - m_from ) *
						double( graph_rect.width() ) / frames;
	const int played_width_px = ( m_framesPlayed - m_from ) *
						double( graph_rect.width() ) / frames;

	// loop point line
	p.setPen( QColor( 0x7F, 0xFF, 0xFF ) ); //TODO: put into a qproperty
	p.drawLine( m_loopFrameX, graph_rect.y(),
					m_loopFrameX,
					graph_rect.height() + graph_rect.y() );

	// start/end lines
	p.setPen( QColor( 0xFF, 0xFF, 0xFF ) );  //TODO: put into a qproperty
	p.drawLine( m_startFrameX, graph_rect.y(),
					m_startFrameX,
					graph_rect.height() + graph_rect.y() );
	p.drawLine( m_endFrameX, graph_rect.y(),
					m_endFrameX,
					graph_rect.height() + graph_rect.y() );


	if( m_endFrameX - m_startFrameX > 2 )
	{
		p.fillRect(
			m_startFrameX + 1,
			graph_rect.y(),
			m_endFrameX - m_startFrameX - 1,
			graph_rect.height() + graph_rect.y(),
			QColor( 95, 175, 255, 50 ) //TODO: put into a qproperty
		);
		if( m_endFrameX - m_loopFrameX > 2 )
			p.fillRect(
				m_loopFrameX + 1,
				graph_rect.y(),
				m_endFrameX - m_loopFrameX - 1,
				graph_rect.height() + graph_rect.y(),
				QColor( 95, 205, 255, 65 ) //TODO: put into a qproperty
		);

		if( m_framesPlayed && m_animation)
		{
			QLinearGradient g( m_startFrameX, 0, played_width_px, 0 );
			const QColor c( 0, 120, 255, 180 ); //TODO: put into a qproperty
			g.setColorAt( 0, Qt::transparent );
			g.setColorAt( 0.8, c );
			g.setColorAt( 1,  c );
			p.fillRect(
				m_startFrameX + 1,
				graph_rect.y(),
				played_width_px - ( m_startFrameX + 1 ),
				graph_rect.height() + graph_rect.y(),
				g
			);
			p.setPen( QColor( 255, 255, 255 ) ); //TODO: put into a qproperty
			p.drawLine(
				played_width_px,
				graph_rect.y(),
				played_width_px,
				graph_rect.height() + graph_rect.y()
			);
			m_framesPlayed = 0;
		}
	}

	QLinearGradient g( 0, 0, width() * 0.7, 0 );
	const QColor c( 16, 111, 170, 180 );
	g.setColorAt( 0, c );
	g.setColorAt( 0.4, c );
	g.setColorAt( 1,  Qt::transparent );
	p.fillRect( s_padding, s_padding, m_graph.width(), 14, g );

	p.setPen( QColor( 255, 255, 255 ) );
	p.setFont( pointSize<8>( font() ) );

	QString length_text;
	const int length = m_sampleBuffer.sampleLength();

	if( length > 20000 )
	{
		length_text = QString::number( length / 1000 ) + "s";
	}
	else if( length > 2000 )
	{
		length_text = QString::number( ( length / 100 ) / 10.0 ) + "s";
	}
	else
	{
		length_text = QString::number( length ) + "ms";
	}

	p.drawText(
		s_padding + 2,
		s_padding + 10,
		tr( "Sample length:" ) + " " + length_text
	);
}




void AudioFileProcessorWaveView::updateGraph()
{
	if( m_to == 1 )
	{
		m_to = m_sampleBuffer.frames() * 0.7;
		slideSamplePointToFrames( end, m_to * 0.7 );
	}

	if( m_from > m_sampleBuffer.startFrame() )
	{
		m_from = m_sampleBuffer.startFrame();
	}

	if( m_to < m_sampleBuffer.endFrame() )
	{
		m_to = m_sampleBuffer.endFrame();
	}

	if( m_sampleBuffer.reversed() != m_reversed )
	{
		reverse();
	}
	else if( m_last_from == m_from && m_last_to == m_to && m_sampleBuffer.amplification() == m_last_amp )
	{
		return;
	}

	m_last_from = m_from;
	m_last_to = m_to;
	m_last_amp = m_sampleBuffer.amplification();

	m_graph.fill( Qt::transparent );
	QPainter p( &m_graph );
	p.setPen( QColor( 255, 255, 255 ) );

	m_sampleBuffer.visualize(
		p,
		QRect( 0, 0, m_graph.width(), m_graph.height() ),
		m_from, m_to
	);
}




void AudioFileProcessorWaveView::zoom( const bool _out )
{
	const f_cnt_t start = m_sampleBuffer.startFrame();
	const f_cnt_t end = m_sampleBuffer.endFrame();
	const f_cnt_t frames = m_sampleBuffer.frames();
	const f_cnt_t d_from = start - m_from;
	const f_cnt_t d_to = m_to - end;

	const f_cnt_t step = qMax( 1, qMax( d_from, d_to ) / 10 );
	const f_cnt_t step_from = ( _out ? - step : step );
	const f_cnt_t step_to = ( _out ? step : - step );

	const double comp_ratio = double( qMin( d_from, d_to ) )
								/ qMax( 1, qMax( d_from, d_to ) );

	f_cnt_t new_from;
	f_cnt_t new_to;

	if( ( _out && d_from < d_to ) || ( ! _out && d_to < d_from ) )
	{
		new_from = qBound( 0, m_from + step_from, start );
		new_to = qBound(
			end,
			m_to + f_cnt_t( step_to * ( new_from == m_from ? 1 : comp_ratio ) ),
			frames
		);
	}
	else
	{
		new_to = qBound( end, m_to + step_to, frames );
		new_from = qBound(
			0,
			m_from + f_cnt_t( step_from * ( new_to == m_to ? 1 : comp_ratio ) ),
			start
		);
	}

	if( double( new_to - new_from ) / m_sampleBuffer.sampleRate() > 0.05  )
	{
		m_from = new_from;
		m_to = new_to;
	}
}




void AudioFileProcessorWaveView::slide( int _px )
{
	const double fact = qAbs( double( _px ) / width() );
	f_cnt_t step = ( m_to - m_from ) * fact;
	if( _px > 0 )
	{
		step = -step;
	}

	f_cnt_t step_from = qBound( 0, m_from + step, m_sampleBuffer.frames() ) - m_from;
	f_cnt_t step_to = qBound( m_from + 1, m_to + step, m_sampleBuffer.frames() ) - m_to;

	step = qAbs( step_from ) < qAbs( step_to ) ? step_from : step_to;

	m_from += step;
	m_to += step;
	slideSampleByFrames( step );
}




void AudioFileProcessorWaveView::setKnobs( knob * _start, knob * _end, knob * _loop )
{
	m_startKnob = _start;
	m_endKnob = _end;
	m_loopKnob = _loop;

	m_startKnob->setWaveView( this );
	m_startKnob->setRelatedKnob( m_endKnob );

	m_endKnob->setWaveView( this );
	m_endKnob->setRelatedKnob( m_startKnob );

	m_loopKnob->setWaveView( this );
}




void AudioFileProcessorWaveView::slideSamplePointByPx( knobType _point, int _px )
{
	slideSamplePointByFrames(
		_point,
		f_cnt_t( ( double( _px ) / width() ) * ( m_to - m_from ) )
	);
}




void AudioFileProcessorWaveView::slideSamplePointByFrames( knobType _point, f_cnt_t _frames, bool _slide_to )
{
	knob * a_knob = m_startKnob;
	switch( _point )
	{
		case end:
			a_knob = m_endKnob;
			break;
		case loop:
			a_knob = m_loopKnob;
			break;
		case start:
			break;
	}
	if( a_knob == nullptr )
	{
		return;
	}
	else
	{
		const double v = static_cast<double>( _frames ) / m_sampleBuffer.frames();
		if( _slide_to )
		{
			a_knob->slideTo( v );
		}
		else
		{
			a_knob->slideBy( v );
		}
	}
}




void AudioFileProcessorWaveView::slideSampleByFrames( f_cnt_t _frames )
{
	if( m_sampleBuffer.frames() <= 1 )
	{
		return;
	}
	const double v = static_cast<double>( _frames ) / m_sampleBuffer.frames();
	if( m_startKnob ) {
		m_startKnob->slideBy( v, false );
	}
	if( m_endKnob ) {
		m_endKnob->slideBy( v, false );
	}
	if( m_loopKnob ) {
		m_loopKnob->slideBy( v, false );
	}
}




void AudioFileProcessorWaveView::reverse()
{
	slideSampleByFrames(
		m_sampleBuffer.frames()
			- m_sampleBuffer.endFrame()
			- m_sampleBuffer.startFrame()
	);

	const f_cnt_t from = m_from;
	m_from = m_sampleBuffer.frames() - m_to;
	m_to = m_sampleBuffer.frames() - from;

	m_reversed = ! m_reversed;
}



void AudioFileProcessorWaveView::updateCursor( QMouseEvent * _me )
{
	bool const waveIsDragged = m_isDragging && (m_draggingType == wave);
	bool const pointerCloseToStartEndOrLoop = (_me != nullptr ) &&
			( isCloseTo( _me->x(), m_startFrameX ) ||
			  isCloseTo( _me->x(), m_endFrameX ) ||
			  isCloseTo( _me->x(), m_loopFrameX ) );

	if( !m_isDragging && pointerCloseToStartEndOrLoop)
		setCursor(Qt::SizeHorCursor);
	else if( waveIsDragged )
		setCursor(Qt::ClosedHandCursor);
	else
		setCursor(Qt::OpenHandCursor);
}




void AudioFileProcessorWaveView::knob::slideTo( double _v, bool _check_bound )
{
	if( _check_bound && ! checkBound( _v ) )
	{
		return;
	}
	model()->setValue( _v );
	emit sliderMoved( model()->value() );
}




float AudioFileProcessorWaveView::knob::getValue( const QPoint & _p )
{
	const double dec_fact = ! m_waveView ? 1 :
		double( m_waveView->m_to - m_waveView->m_from )
			/ m_waveView->m_sampleBuffer.frames();
	const float inc = Knob::getValue( _p ) * dec_fact;

	return inc;
}




bool AudioFileProcessorWaveView::knob::checkBound( double _v ) const
{
	if( ! m_relatedKnob || ! m_waveView )
	{
		return true;
	}

	if( ( m_relatedKnob->model()->value() - _v > 0 ) !=
		( m_relatedKnob->model()->value() - model()->value() >= 0 ) )
		return false;

	const double d1 = qAbs( m_relatedKnob->model()->value() - model()->value() )
		* ( m_waveView->m_sampleBuffer.frames() )
		/ m_waveView->m_sampleBuffer.sampleRate();

	const double d2 = qAbs( m_relatedKnob->model()->value() - _v )
		* ( m_waveView->m_sampleBuffer.frames() )
		/ m_waveView->m_sampleBuffer.sampleRate();

	return d1 < d2 || d2 > 0.005;
}


} // namespace gui




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model * model, void *)
{
	return new AudioFileProcessor(static_cast<InstrumentTrack *>(model));
}


}


} // namespace lmms
