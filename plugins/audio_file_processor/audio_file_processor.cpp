/*
 * audio_file_processor.cpp - instrument for using audio-files
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QPainter>
#include <QtGui/QBitmap>
#include <QtXml/QDomDocument>
#include <QtCore/QFileInfo>
#include <QtGui/QDropEvent>

#include "ResourceFileMapper.h"

#include "audio_file_processor.h"
#include "engine.h"
#include "song.h"
#include "InstrumentTrack.h"
#include "note_play_handle.h"
#include "interpolation.h"
#include "gui_templates.h"
#include "tooltip.h"
#include "string_pair_drag.h"
#include "mmp.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT audiofileprocessor_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"AudioFileProcessor",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"simple sampler with various settings for "
				"using samples (e.g. drums) in an "
				"instrument-track" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	sampleBuffer::supportedExts,
	NULL
} ;

}




audioFileProcessor::audioFileProcessor( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &audiofileprocessor_plugin_descriptor ),
	m_sampleBuffer(),
	m_ampModel( 100, 0, 500, 1, this, tr( "Amplify" ) ),
	m_startPoIntModel( 0, 0, 1, 0.0000001f, this, tr( "Start of sample") ),
	m_endPoIntModel( 1, 0, 1, 0.0000001f, this, tr( "End of sample" ) ),
	m_reverseModel( false, this, tr( "Reverse sample" ) ),
	m_loopModel( false, this, tr( "Loop") )
{
	connect( &m_reverseModel, SIGNAL( dataChanged() ),
				this, SLOT( reverseModelChanged() ) );
	connect( &m_ampModel, SIGNAL( dataChanged() ),
				this, SLOT( ampModelChanged() ) );
	connect( &m_startPoIntModel, SIGNAL( dataChanged() ),
				this, SLOT( loopPointChanged() ) );
	connect( &m_endPoIntModel, SIGNAL( dataChanged() ),
				this, SLOT( loopPointChanged() ) );
}




audioFileProcessor::~audioFileProcessor()
{
}




void audioFileProcessor::playNote( notePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	if( !_n->m_pluginData )
	{
		_n->m_pluginData = new handleState( _n->hasDetuningInfo() );
	}

	if( m_sampleBuffer.play( _working_buffer,
					(handleState *)_n->m_pluginData,
					frames, _n->frequency(),
						m_loopModel.value() ) == true )
	{
		applyRelease( _working_buffer, _n );
		instrumentTrack()->processAudioBuffer( _working_buffer,
								frames,_n );
	}
}




void audioFileProcessor::deleteNotePluginData( notePlayHandle * _n )
{
	delete (handleState *)_n->m_pluginData;
}




void audioFileProcessor::saveSettings( QDomDocument & _doc,
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
	m_startPoIntModel.saveSettings( _doc, _this, "sframe" );
	m_endPoIntModel.saveSettings( _doc, _this, "eframe" );
}




void audioFileProcessor::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "src" ) != "" )
	{
		setAudioFile( _this.attribute( "src" ), false );
	}
	else if( _this.attribute( "sampledata" ) != "" )
	{
		m_sampleBuffer.loadFromBase64( _this.attribute( "srcdata" ) );
	}
	m_reverseModel.loadSettings( _this, "reversed" );
	m_loopModel.loadSettings( _this, "looped" );
	m_ampModel.loadSettings( _this, "amp" );
	m_startPoIntModel.loadSettings( _this, "sframe" );
	m_endPoIntModel.loadSettings( _this, "eframe" );

	loopPointChanged();
}




void audioFileProcessor::loadResource( const ResourceItem * _resourceItem )
{
	// TODO: replace this with real support for ResourceItem's
	ResourceFileMapper mapper( _resourceItem );
	setAudioFile( mapper.fileName() );
}




QString audioFileProcessor::nodeName() const
{
	return audiofileprocessor_plugin_descriptor.name;
}




Uint32 audioFileProcessor::getBeatLen( notePlayHandle * _n ) const
{
	const float freq_factor = BaseFreq / _n->frequency() *
			engine::getMixer()->processingSampleRate() /
					engine::getMixer()->baseSampleRate();

	return static_cast<Uint32>( floorf( ( m_sampleBuffer.endFrame() -
						m_sampleBuffer.startFrame() ) *
							freq_factor ) );
}




PluginView * audioFileProcessor::instantiateView( QWidget * _parent )
{
	return new AudioFileProcessorView( this, _parent );
}




void audioFileProcessor::setAudioFile( const QString & _audio_file,
													bool _rename )
{
	// is current channel-name equal to previous-filename??
	if( _rename &&
		( instrumentTrack()->name() ==
			QFileInfo( m_sampleBuffer.audioFile() ).fileName() ||
				m_sampleBuffer.audioFile().isEmpty() ) )
	{
		// then set it to new one
		instrumentTrack()->setName( QFileInfo( _audio_file).fileName() );
	}
	// else we don't touch the track-name, because the user named it self

	m_sampleBuffer.setAudioFile( _audio_file );
	loopPointChanged();
}




void audioFileProcessor::reverseModelChanged()
{
	m_sampleBuffer.setReversed( m_reverseModel.value() );
}




void audioFileProcessor::ampModelChanged()
{
	m_sampleBuffer.setAmplification( m_ampModel.value() / 100.0f );
}




void audioFileProcessor::loopPointChanged()
{
	const f_cnt_t f1 = static_cast<f_cnt_t>( m_startPoIntModel.value() *
						( m_sampleBuffer.frames()-1 ) );
	const f_cnt_t f2 = static_cast<f_cnt_t>( m_endPoIntModel.value() *
						( m_sampleBuffer.frames()-1 ) );
	m_sampleBuffer.setStartFrame( qMin<f_cnt_t>( f1, f2 ) );
	m_sampleBuffer.setEndFrame( qMax<f_cnt_t>( f1, f2 ) );
	emit dataChanged();
}







class audioFileKnob : public knob
{
public:
	audioFileKnob( QWidget * _parent ) :
			knob( knobStyled, _parent )
	{
		setFixedSize( 37, 47 );
	}
};




QPixmap * AudioFileProcessorView::s_artwork = NULL;


AudioFileProcessorView::AudioFileProcessorView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}

	m_openAudioFileButton = new pixmapButton( this );
	m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openAudioFileButton->move( 227, 72 );
	m_openAudioFileButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	m_openAudioFileButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	connect( m_openAudioFileButton, SIGNAL( clicked() ),
					this, SLOT( openAudioFile() ) );
	toolTip::add( m_openAudioFileButton, tr( "Open other sample" ) );

	m_openAudioFileButton->setWhatsThis(
		tr( "Click here, if you want to open another audio-file. "
			"A dialog will appear where you can select your file. "
			"Settings like looping-mode, start and end-points, " 
			"amplify-value, and so on are not reset. So, it may not "
			"sound like the original sample.") );

	m_reverseButton = new pixmapButton( this );
	m_reverseButton->setCheckable( true );
	m_reverseButton->move( 184, 124 );
	m_reverseButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_on" ) );
	m_reverseButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_off" ) );
	toolTip::add( m_reverseButton, tr( "Reverse sample" ) );
	m_reverseButton->setWhatsThis(
		tr( "If you enable this button, the whole sample is reversed. "
			"This is useful for cool effects, e.g. a reversed "
			"crash." ) );

	m_loopButton = new pixmapButton( this );
	m_loopButton->setCheckable( true );
	m_loopButton->move( 220, 124 );
	m_loopButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_on" ) );
	m_loopButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_off" ) );
	toolTip::add( m_loopButton,
				tr( "Loop sample at start- and end-point" ) );
	m_loopButton->setWhatsThis(
		tr( "Here you can set, whether looping-mode is enabled. If "
			"enabled, AudioFileProcessor loops between start and "
			"end-points of a sample until the whole note is played. "
			"This is useful for things like string and choir "
			"samples." ) );

	m_ampKnob = new knob( knobStyled, this );
	m_ampKnob->setVolumeKnob( true );
	m_ampKnob->move( 17, 108 );
	m_ampKnob->setFixedSize( 37, 47 );
	m_ampKnob->setHintText( tr( "Amplify:" )+" ", "%" );
	m_ampKnob->setWhatsThis(
		tr( "With this knob you can set the amplify ratio. When you "
			"set a value of 100% your sample isn't changed. "
			"Otherwise it will be amplified up or down (your "
			"actual sample-file isn't touched!)" ) );

	m_startKnob = new audioFileKnob( this );
	m_startKnob->move( 68, 108 );
	m_startKnob->setHintText( tr( "Startpoint:" )+" ", "" );
	m_startKnob->setWhatsThis(
		tr( "With this knob you can set the point where "
			"AudioFileProcessor should begin playing your sample. "
			"If you enable looping-mode, this is the point to "
			"which AudioFileProcessor returns if a note is longer "
			"than the sample between the start and end-points." ) );

	m_endKnob = new audioFileKnob( this );
	m_endKnob->move( 119, 108 );
	m_endKnob->setHintText( tr( "Endpoint:" )+" ", "" );
	m_endKnob->setWhatsThis(
		tr( "With this knob you can set the point where "
			"AudioFileProcessor should stop playing your sample. "
			"If you enable looping-mode, this is the point where "
			"AudioFileProcessor returns if a note is longer than "
			"the sample between the start and end-points." ) );
}




AudioFileProcessorView::~AudioFileProcessorView()
{
}




void AudioFileProcessorView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, *s_artwork );

	audioFileProcessor * a = castModel<audioFileProcessor>();

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

	p.drawPixmap( 2, 172, m_graph );


	p.setPen( QColor( 0xFF, 0xAA, 0x00 ) );
	const QRect graph_rect( 4, 174, 241, 70 );
	const f_cnt_t frames = qMax( a->m_sampleBuffer.frames(),
						static_cast<f_cnt_t>( 1 ) );
	const int start_frame_x = a->m_sampleBuffer.startFrame() *
						graph_rect.width() / frames;
	const int end_frame_x = a->m_sampleBuffer.endFrame() *
						graph_rect.width() / frames;

	p.drawLine( start_frame_x + graph_rect.x(), graph_rect.y(),
					start_frame_x + graph_rect.x(),
					graph_rect.height() + graph_rect.y() );
	p.drawLine( end_frame_x + graph_rect.x(), graph_rect.y(),
					end_frame_x + graph_rect.x(),
					graph_rect.height() + graph_rect.y() );

}




void AudioFileProcessorView::sampleUpdated()
{
	m_graph = QPixmap( 245, 75 );
	m_graph.fill( Qt::transparent );
	QPainter p( &m_graph );
	p.setPen( QColor( 64, 255, 160 ) );
	castModel<audioFileProcessor>()->m_sampleBuffer.
				visualize( p, QRect( 2, 2, m_graph.width() - 4,
						m_graph.height() - 4 ) );
	update();
}





void AudioFileProcessorView::openAudioFile()
{
	QString af = castModel<audioFileProcessor>()->m_sampleBuffer.
							openAudioFile();
	if( af != "" )
	{
		castModel<audioFileProcessor>()->setAudioFile( af );
		engine::getSong()->setModified();
	}
}




void AudioFileProcessorView::modelChanged()
{
	audioFileProcessor * a = castModel<audioFileProcessor>();
	connect( &a->m_sampleBuffer, SIGNAL( sampleUpdated() ),
					this, SLOT( sampleUpdated() ) );
	m_ampKnob->setModel( &a->m_ampModel );
	m_startKnob->setModel( &a->m_startPoIntModel );
	m_endKnob->setModel( &a->m_endPoIntModel );
	m_reverseButton->setModel( &a->m_reverseModel );
	m_loopButton->setModel( &a->m_loopModel );
	sampleUpdated();
}





extern "C"
{

// neccessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new audioFileProcessor(
				static_cast<InstrumentTrack *>( _data ) );
}


}


#include "moc_audio_file_processor.cxx"

/* vim: set tw=0 expandtab: */
