/*
 * audio_file_processor.cpp - instrument for using audio-files
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <Qt/QtXml>
#include <QtCore/QFileInfo>
#include <QtGui/QDropEvent>


#include "audio_file_processor.h"
#include "automatable_model_templates.h"
#include "engine.h"
#include "song.h"
#include "instrument_track.h"
#include "note_play_handle.h"
#include "interpolation.h"
#include "gui_templates.h"
#include "tooltip.h"
#include "string_pair_drag.h"
#include "mmp.h"


#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor audiofileprocessor_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"AudioFileProcessor",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"simple sampler with various settings for "
				"using samples (e.g. drums) in an "
				"instrument-track" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	new audioFileProcessor::subPluginFeatures( plugin::Instrument )
} ;

}





audioFileProcessor::audioFileProcessor( instrumentTrack * _instrument_track ) :
	instrument( _instrument_track, &audiofileprocessor_plugin_descriptor ),
	m_sampleBuffer(),
	m_ampModel( 100, 0, 500, 1, this ),
	m_startPointModel( 0, 0, 1, 0.0000001f, this ),
	m_endPointModel( 1, 0, 1, 0.0000001f, this ),
	m_reverseModel( FALSE, this ),
	m_loopModel( FALSE, this )
{
	connect( &m_reverseModel, SIGNAL( dataChanged() ),
				this, SLOT( reverseModelChanged() ) );
	connect( &m_ampModel, SIGNAL( dataChanged() ),
				this, SLOT( ampModelChanged() ) );
	connect( &m_startPointModel, SIGNAL( dataChanged() ),
				this, SLOT( startPointModelChanged() ) );
	connect( &m_endPointModel, SIGNAL( dataChanged() ),
				this, SLOT( endPointModelChanged() ) );
	m_ampModel.setTrack( _instrument_track );
	m_startPointModel.setTrack( _instrument_track );
	m_endPointModel.setTrack( _instrument_track );
	m_reverseModel.setTrack( _instrument_track );
	m_loopModel.setTrack( _instrument_track );
}




audioFileProcessor::~audioFileProcessor()
{
}




void audioFileProcessor::playNote( notePlayHandle * _n, bool )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	sampleFrame * buf = new sampleFrame[frames];

	if( !_n->m_pluginData )
	{
		_n->m_pluginData = new handleState( _n->hasDetuningInfo() );
	}

	if( m_sampleBuffer.play( buf, (handleState *)_n->m_pluginData,
					frames, _n->frequency(),
						m_loopModel.value() ) == TRUE )
	{
		applyRelease( buf, _n );
		getInstrumentTrack()->processAudioBuffer( buf, frames, _n );
	}
	delete[] buf;
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
	m_startPointModel.saveSettings( _doc, _this, "sframe" );
	m_endPointModel.saveSettings( _doc, _this, "eframe" );
}




void audioFileProcessor::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "src" ) != "" )
	{
		setAudioFile( _this.attribute( "src" ), FALSE );
	}
	else if( _this.attribute( "sampledata" ) != "" )
	{
		m_sampleBuffer.loadFromBase64( _this.attribute( "srcdata" ) );
	}
	m_reverseModel.loadSettings( _this, "reversed" );
	m_loopModel.loadSettings( _this, "looped" );
	m_ampModel.loadSettings( _this, "amp" );
	m_startPointModel.loadSettings( _this, "sframe" );
	m_endPointModel.loadSettings( _this, "eframe" );

	startPointModelChanged();
	endPointModelChanged();
}




void audioFileProcessor::setParameter( const QString & _param,
							const QString & _value )
{
	if( _param == "samplefile" )
	{
		setAudioFile( _value );
	}
	else if( _param == "sampledata" )
	{
		m_sampleBuffer.loadFromBase64( _value );
	}
}




QString audioFileProcessor::nodeName( void ) const
{
	return( audiofileprocessor_plugin_descriptor.name );
}




Uint32 audioFileProcessor::getBeatLen( notePlayHandle * _n ) const
{
	const float freq_factor = BaseFreq / _n->frequency() *
			engine::getMixer()->sampleRate() / DEFAULT_SAMPLE_RATE;

	return( static_cast<Uint32>( floorf( ( m_sampleBuffer.endFrame() -
						m_sampleBuffer.startFrame() ) *
							freq_factor ) ) );
}




pluginView * audioFileProcessor::instantiateView( QWidget * _parent )
{
	return( new audioFileProcessorView( this, _parent ) );
}




void audioFileProcessor::setAudioFile( const QString & _audio_file,
								bool _rename )
{
	// is current channel-name equal to previous-filename??
	if( _rename && 
		( getInstrumentTrack()->name() ==
			QFileInfo( m_sampleBuffer.audioFile() ).fileName() ||
		   	m_sampleBuffer.audioFile() == "" ) )
	{
		// then set it to new one
		getInstrumentTrack()->setName( QFileInfo( _audio_file
								).fileName() );
	}
	// else we don't touch the track-name, because the user named it self

	m_sampleBuffer.setAudioFile( _audio_file );
	startPointModelChanged();
	endPointModelChanged();
}




void audioFileProcessor::reverseModelChanged( void )
{
	m_sampleBuffer.setReversed( m_reverseModel.value() );
}




void audioFileProcessor::ampModelChanged( void )
{
	m_sampleBuffer.setAmplification( m_ampModel.value() / 100.0f );
}




void audioFileProcessor::startPointModelChanged( void )
{
	if( m_startPointModel.value() < m_endPointModel.value() )
	{
		m_sampleBuffer.setStartFrame( static_cast<Uint32>(
					m_startPointModel.value() *
						m_sampleBuffer.frames() ) );
	}
	else
	{
		m_startPointModel.setValue( m_endPointModel.value() - 0.01f );
	}
	emit dataChanged();
}




void audioFileProcessor::endPointModelChanged( void )
{
	if( m_endPointModel.value() > m_startPointModel.value() )
	{
		if( m_endPointModel.value() * m_sampleBuffer.frames() >= 1.0f )
		{
			m_sampleBuffer.setEndFrame( static_cast<Uint32>(
					m_endPointModel.value() *
						m_sampleBuffer.frames() ) - 1 );
		}
		else
		{
			m_sampleBuffer.setEndFrame( 0 );
		}
	}
	else
	{
		m_endPointModel.setValue( m_startPointModel.value() + 0.01f );
	}
	emit dataChanged();
}








audioFileProcessor::subPluginFeatures::subPluginFeatures(
						plugin::PluginTypes _type ) :
	plugin::descriptor::subPluginFeatures( _type )
{
}




const QStringList & audioFileProcessor::subPluginFeatures::supportedExtensions(
									void )
{
	static QStringList extensions = QStringList()
				<< "wav" << "ogg" << "ds" << "spx" << "au"
				<< "voc" << "aif" << "aiff" << "flac" << "raw";
	return( extensions );
}







QPixmap * audioFileProcessorView::s_artwork = NULL;


audioFileProcessorView::audioFileProcessorView( instrument * _instrument,
							QWidget * _parent ) :
	instrumentView( _instrument, _parent )
{
	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}

	m_openAudioFileButton = new pixmapButton( this, NULL );
	m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openAudioFileButton->move( 200, 90 );
	m_openAudioFileButton->setActiveGraphic( embed::getIconPixmap(
							"project_open_down" ) );
	m_openAudioFileButton->setInactiveGraphic( embed::getIconPixmap(
							"project_open" ) );
	connect( m_openAudioFileButton, SIGNAL( clicked() ),
					this, SLOT( openAudioFile() ) );
	toolTip::add( m_openAudioFileButton, tr( "Open other sample" ) );

	m_openAudioFileButton->setWhatsThis(
		tr( "Click here, if you want to open another audio-file. After "
			"clicking on this button, a file-open-dialog appears "
			"and you can select your file. Settings like Looping-"
			"Mode, start- and end-point, amplify-value and so on "
			"are not reset, so please don't wonder if your sample "
			"doesn't sound like the original one..." ) );

	m_reverseButton = new pixmapButton( this, NULL );
	m_reverseButton->setCheckable( TRUE );
	m_reverseButton->move( 160, 124 );
	m_reverseButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_on" ) );
	m_reverseButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_off" ) );
	toolTip::add( m_reverseButton, tr( "Reverse sample" ) );
	m_reverseButton->setWhatsThis(
		tr( "If you enable this button, the whole sample is reversed. "
			"This is useful for cool effects, e.g. a reversed "
			"crash." ) );

	m_loopButton = new pixmapButton( this, tr( "Loop" ) );
	m_loopButton->setCheckable( TRUE );
	m_loopButton->move( 180, 124 );
	m_loopButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_on" ) );
	m_loopButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_off" ) );
	toolTip::add( m_loopButton,
				tr( "Loop sample at start- and end-point" ) );
	m_loopButton->setWhatsThis(
		tr( "Here you can set, whether Looping-Mode is enabled. If "
			"enabled, AudioFileProcessor loops between start- and "
			"end-point of a sample until the whole note is played. "
			"This is useful for things like string- and choir-"
			"samples." ) );

	m_ampKnob = new volumeKnob( knobDark_28, this, tr( "Amplify" ) );
	m_ampKnob->move( 6, 114 );
	m_ampKnob->setHintText( tr( "Amplify:" )+" ", "%" );
	m_ampKnob->setLabel( tr( "AMP" ) );
	m_ampKnob->setWhatsThis(
		tr( "With this knob you can set the amplify-ratio. When you "
			"set a value of 100% your sample isn't changed. "
			"Otherwise it will be amplified up or down (your "
			"actual sample-file isn't touched!)" ) );

	m_startKnob = new knob( knobDark_28, this, tr( "Start of sample" ) );
	m_startKnob->move( 46, 114 );
	m_startKnob->setHintText( tr( "Startpoint:" )+" ", "" );
	m_startKnob->setLabel( tr( "START" ) );
	m_startKnob->setWhatsThis(
		tr( "With this knob you can set the point where "
			"AudioFileProcessor should begin playing your sample. "
			"If you enable Looping-Mode, this is the point to "
			"which AudioFileProcessor returns if a note is longer "
			"than the sample between start- and end-point." ) );

	m_endKnob = new knob( knobDark_28, this, tr( "End of sample" ) );
	m_endKnob->move( 84, 114 );
	m_endKnob->setHintText( tr( "Endpoint:" )+" ", "" );
	m_endKnob->setLabel( tr( "END" ) );
	m_endKnob->setWhatsThis(
		tr( "With this knob you can set the point where "
			"AudioFileProcessor should stop playing your sample. "
			"If you enable Looping-Mode, this is the point where "
			"AudioFileProcessor returns if a note is longer than "
			"the sample between start- and end-point." ) );

	setAcceptDrops( TRUE );
}




audioFileProcessorView::~audioFileProcessorView()
{
}




void audioFileProcessorView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( stringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						stringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == QString( "tco_%1" ).arg(
							track::SampleTrack ) )
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




void audioFileProcessorView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		castModel<audioFileProcessor>()->setAudioFile( value );
		_de->accept();
		return;
	}
	else if( type == QString( "tco_%1" ).arg( track::SampleTrack ) )
	{
		multimediaProject mmp( value, FALSE );
		castModel<audioFileProcessor>()->setAudioFile( mmp.content().
				firstChild().toElement().attribute( "src" ) );
		_de->accept();
		return;
	}

	_de->ignore();
}




void audioFileProcessorView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, *s_artwork );

	audioFileProcessor * a = castModel<audioFileProcessor>();

 	QString file_name = "";
	Uint16 idx = a->m_sampleBuffer.audioFile().length();

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
	p.drawText( 8, 84, file_name );

	p.drawPixmap( 2, 172, m_graph );


	p.setPen( QColor( 0xFF, 0xAA, 0x00 ) );
	const QRect graph_rect( 4, 174, 241, 70 );
	const f_cnt_t frames = tMax( a->m_sampleBuffer.frames(),
						static_cast<f_cnt_t>( 1 ) );
	const Uint16 start_frame_x = a->m_sampleBuffer.startFrame() *
						graph_rect.width() / frames;
	const Uint16 end_frame_x = a->m_sampleBuffer.endFrame() *
					( graph_rect.width() - 1 ) / frames;

	p.drawLine( start_frame_x + graph_rect.x(), graph_rect.y(),
					start_frame_x + graph_rect.x(),
					graph_rect.height() + graph_rect.y() );
	p.drawLine( end_frame_x + graph_rect.x(), graph_rect.y(),
					end_frame_x + graph_rect.x(),
					graph_rect.height() + graph_rect.y() );

}




void audioFileProcessorView::sampleUpdated( void )
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





void audioFileProcessorView::openAudioFile( void )
{
	QString af = castModel<audioFileProcessor>()->m_sampleBuffer.
							openAudioFile();
	if( af != "" )
	{
		castModel<audioFileProcessor>()->setAudioFile( af );
		engine::getSong()->setModified();
	}
}




void audioFileProcessorView::modelChanged( void )
{
	audioFileProcessor * a = castModel<audioFileProcessor>();
	connect( &a->m_sampleBuffer, SIGNAL( sampleUpdated() ),
					this, SLOT( sampleUpdated() ) );
	m_ampKnob->setModel( &a->m_ampModel );
	m_startKnob->setModel( &a->m_startPointModel );
	m_endKnob->setModel( &a->m_endPointModel );
	m_reverseButton->setModel( &a->m_reverseModel );
	m_loopButton->setModel( &a->m_loopModel );
	sampleUpdated();
}





extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( model *, void * _data )
{
	return( new audioFileProcessor(
				static_cast<instrumentTrack *>( _data ) ) );
}


}


#include "audio_file_processor.moc"

