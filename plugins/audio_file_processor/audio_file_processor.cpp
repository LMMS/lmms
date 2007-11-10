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
#include "engine.h"
#include "song_editor.h"
#include "instrument_track.h"
#include "note_play_handle.h"
#include "interpolation.h"
#include "gui_templates.h"
#include "pixmap_button.h"
#include "knob.h"
#include "tooltip.h"
#include "string_pair_drag.h"
#include "mmp.h"
#include "volume_knob.h"


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


QPixmap * audioFileProcessor::s_artwork = NULL;



audioFileProcessor::audioFileProcessor( instrumentTrack * _channel_track ) :
	instrument( _channel_track, &audiofileprocessor_plugin_descriptor ),
	m_drawMethod( sampleBuffer::LINE_CONNECT )
{
	connect( &m_sampleBuffer, SIGNAL( sampleUpdated() ), this,
						SLOT( sampleUpdated() ) );

	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}


	m_openAudioFileButton = new pixmapButton( this, NULL, NULL );
	m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openAudioFileButton->move( 200, 90 );
	m_openAudioFileButton->setActiveGraphic( embed::getIconPixmap(
							"project_open_down" ) );
	m_openAudioFileButton->setInactiveGraphic( embed::getIconPixmap(
							"project_open" ) );
	connect( m_openAudioFileButton, SIGNAL( clicked() ), this,
						SLOT( openAudioFile() ) );
	toolTip::add( m_openAudioFileButton, tr( "Open other sample" ) );

	m_openAudioFileButton->setWhatsThis(
		tr( "Click here, if you want to open another audio-file. After "
			"clicking on this button, a file-open-dialog appears "
			"and you can select your file. Settings like Looping-"
			"Mode, start- and end-point, amplify-value and so on "
			"are not reset, so please don't wonder if your sample "
			"doesn't sound like the original one..." ) );
	
	m_reverseButton = new pixmapButton( this, NULL, NULL );
	m_reverseButton->setCheckable( TRUE );
	m_reverseButton->move( 160, 124 );
	m_reverseButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_on" ) );
	m_reverseButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_off" ) );
	connect( m_reverseButton, SIGNAL( toggled( bool ) ), this,
					SLOT( reverseBtnToggled( bool ) ) );
	toolTip::add( m_reverseButton, tr( "Reverse sample" ) );
	m_reverseButton->setWhatsThis(
		tr( "If you enable this button, the whole sample is reversed. "
			"This is useful for cool effects, e.g. a reversed "
			"crash." ) );

	m_loopButton = new pixmapButton( this, tr( "Loop" ), _channel_track );
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

	m_ampKnob = new volumeKnob( knobDark_28, this, tr( "Amplify" ), NULL );
	m_ampKnob->setRange( 0, 500, 1.0f );
	m_ampKnob->move( 6, 114 );
	m_ampKnob->setInitValue( 100.0f );
	m_ampKnob->setHintText( tr( "Amplify:" )+" ", "%" );
	m_ampKnob->setLabel( tr( "AMP" ) );
	connect( m_ampKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( ampKnobChanged( float ) ) );
	m_ampKnob->setWhatsThis(
		tr( "With this knob you can set the amplify-ratio. When you "
			"set a value of 100% your sample isn't changed. "
			"Otherwise it will be amplified up or down (your "
			"actual sample-file isn't touched!)" ) );

	m_startKnob = new knob( knobDark_28, this, tr( "Start of sample" ),
							_channel_track );
	m_startKnob->setRange( 0.0f, 1.0f, 0.00001f );
	m_startKnob->move( 46, 114 );
	m_startKnob->setInitValue( 0.0f );
	m_startKnob->setHintText( tr( "Startpoint:" )+" ", "" );
	m_startKnob->setLabel( tr( "START" ) );
	connect( m_startKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( startKnobChanged( float ) ) );
	m_startKnob->setWhatsThis(
		tr( "With this knob you can set the point where "
			"AudioFileProcessor should begin playing your sample. "
			"If you enable Looping-Mode, this is the point to "
			"which AudioFileProcessor returns if a note is longer "
			"than the sample between start- and end-point." ) );

	m_endKnob = new knob( knobDark_28, this, tr( "End of sample" ),
							_channel_track );
	m_endKnob->setRange( 0.0f, 1.0f, 0.00001f );
	m_endKnob->move( 84, 114 );
	m_endKnob->setInitValue( 1.0f );
	m_endKnob->setHintText( tr( "Endpoint:" )+" ", "" );
	m_endKnob->setLabel( tr( "END" ) );
	connect( m_endKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( endKnobChanged( float ) ) );
	m_endKnob->setWhatsThis(
		tr( "With this knob you can set the point where "
			"AudioFileProcessor should stop playing your sample. "
			"If you enable Looping-Mode, this is the point where "
			"AudioFileProcessor returns if a note is longer than "
			"the sample between start- and end-point." ) );

	m_viewLinesPB = new pixmapButton( this, NULL, NULL );
	m_viewLinesPB->move( 154, 158 );
	if( m_drawMethod == sampleBuffer::LINE_CONNECT )
	{
		m_viewLinesPB->setChecked( TRUE );
	}
	connect( m_viewLinesPB, SIGNAL( toggled( bool ) ), this,
					SLOT( lineDrawBtnToggled( bool ) ) );
	m_viewLinesPB->setWhatsThis(
		tr( "Activate this button, if your sample should be drawn "
			"with connected lines. This doesn't change the "
			"sound itself. It just gives you another view to your "
			"sample." ) );

	m_viewDotsPB = new pixmapButton( this, NULL, NULL );
	m_viewDotsPB->move( 204, 158 );
	if( m_drawMethod == sampleBuffer::DOTS )
	{
		m_viewDotsPB->setChecked( TRUE );
	}
	connect( m_viewDotsPB, SIGNAL( toggled( bool ) ), this,
					SLOT( dotDrawBtnToggled( bool ) ) );
	m_viewDotsPB->setWhatsThis(
		tr( "Activate this button, if your sample should be drawn "
			"with dots. This doesn't change the sound itself. "
			"It just gives you another view to your sample." ) );
	
	automatableButtonGroup * view_group = new automatableButtonGroup( this,
								NULL, NULL );
	view_group->addButton( m_viewLinesPB );
	view_group->addButton( m_viewDotsPB );

	setAcceptDrops( TRUE );
}




audioFileProcessor::~audioFileProcessor()
{
}




void audioFileProcessor::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	_this.setAttribute( "src", m_sampleBuffer.audioFile() );
	if( m_sampleBuffer.audioFile() == "" )
	{
		QString s;
		_this.setAttribute( "sampledata", m_sampleBuffer.toBase64( s ) );
	}
	m_reverseButton->saveSettings( _doc, _this, "reversed" );
	m_loopButton->saveSettings( _doc, _this, "looped" );
	m_ampKnob->saveSettings( _doc, _this, "amp" );
	m_startKnob->saveSettings( _doc, _this, "sframe" );
	m_endKnob->saveSettings( _doc, _this, "eframe" );
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
	m_reverseButton->loadSettings( _this, "reversed" );
	m_loopButton->loadSettings( _this, "looped" );
	m_ampKnob->loadSettings( _this, "amp" );
	m_startKnob->loadSettings( _this, "sframe" );
	m_endKnob->loadSettings( _this, "eframe" );

	startKnobChanged( m_startKnob->value()  );
	endKnobChanged( m_endKnob->value() );
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
	const float freq_factor = BASE_FREQ / _n->frequency() *
			engine::getMixer()->sampleRate() / DEFAULT_SAMPLE_RATE;

	return( static_cast<Uint32>( floorf( ( m_sampleBuffer.endFrame() -
						m_sampleBuffer.startFrame() ) *
							freq_factor ) ) );
}




void audioFileProcessor::setAudioFile( const QString & _audio_file, bool _rename )
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
	// else we don't touch the channel-name, because the user named it self

	m_sampleBuffer.setAudioFile( _audio_file );
	startKnobChanged( m_startKnob->value()  );
	endKnobChanged( m_endKnob->value() );
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
					m_loopButton->isChecked() ) == TRUE )
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




void audioFileProcessor::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( stringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						stringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == QString( "tco_%1" ).arg(
							track::SAMPLE_TRACK ) )
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




void audioFileProcessor::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		setAudioFile( value );
		_de->accept();
		return;
	}
	else if( type == QString( "tco_%1" ).arg( track::SAMPLE_TRACK ) )
	{
		multimediaProject mmp( value, FALSE );
		setAudioFile( mmp.content().firstChild().toElement().
							attribute( "src" ) );
		_de->accept();
		return;
	}

	_de->ignore();
}




void audioFileProcessor::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, *s_artwork );


 	QString file_name = "";
	Uint16 idx = m_sampleBuffer.audioFile().length();

	p.setFont( pointSize<8>( font() ) );

	QFontMetrics fm( p.font() );

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
	while( idx > 0 &&
		fm.size( Qt::TextSingleLine, file_name + "..." ).width() < 210 )
	{
		file_name = m_sampleBuffer.audioFile()[--idx] + file_name;
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
	const f_cnt_t frames = tMax( m_sampleBuffer.frames(),
						static_cast<f_cnt_t>( 1 ) );
	const Uint16 start_frame_x = m_sampleBuffer.startFrame() *
						graph_rect.width() / frames;
	const Uint16 end_frame_x = m_sampleBuffer.endFrame() *
					( graph_rect.width() - 1 ) / frames;

	p.drawLine( start_frame_x + graph_rect.x(), graph_rect.y(),
					start_frame_x + graph_rect.x(),
					graph_rect.height() + graph_rect.y() );
	p.drawLine( end_frame_x + graph_rect.x(), graph_rect.y(),
					end_frame_x + graph_rect.x(),
					graph_rect.height() + graph_rect.y() );

}




void audioFileProcessor::sampleUpdated( void )
{
	m_graph = QPixmap( 245, 75 );
	QPainter p( &m_graph );
	p.drawPixmap( 2, 172, m_graph );
	p.setPen( QColor( 64, 255, 160 ) );
	m_sampleBuffer.visualize( p, QRect( 2, 2, m_graph.width() - 4,
							m_graph.height() - 4 ),
								m_drawMethod );
	update();
}




void audioFileProcessor::reverseBtnToggled( bool _on )
{
	m_sampleBuffer.setReversed( _on );
	engine::getSongEditor()->setModified();
}




void audioFileProcessor::lineDrawBtnToggled( bool _on )
{
	if( _on == TRUE )
	{
		m_drawMethod = sampleBuffer::LINE_CONNECT;
		sampleUpdated();
	}
}




void audioFileProcessor::dotDrawBtnToggled( bool _on )
{
	if( _on == TRUE )
	{
		m_drawMethod = sampleBuffer::DOTS;
		sampleUpdated();
	}
}




void audioFileProcessor::ampKnobChanged( float _val )
{
	m_sampleBuffer.setAmplification( _val / 100.0f );
}




void audioFileProcessor::startKnobChanged( float _new_value )
{
	if( _new_value < m_endKnob->value() )
	{
		m_sampleBuffer.setStartFrame( static_cast<Uint32>( _new_value *
						m_sampleBuffer.frames() ) );
	}
	else
	{
		m_startKnob->setValue( m_endKnob->value() - 0.01f );
	}
	update();
}




void audioFileProcessor::endKnobChanged( float _new_value )
{
	if( _new_value > m_startKnob->value() )
	{
		if( _new_value * m_sampleBuffer.frames() >= 1.0f )
		{
			m_sampleBuffer.setEndFrame( static_cast<Uint32>(
							_new_value *
						m_sampleBuffer.frames() ) - 1 );
		}
		else
		{
			m_sampleBuffer.setEndFrame( 0 );
		}
	}
	else
	{
		m_endKnob->setValue( m_startKnob->value() + 0.01f );
	}
	update();
}




void audioFileProcessor::openAudioFile( void )
{
	QString af = m_sampleBuffer.openAudioFile();
	if( af != "" )
	{
		setAudioFile( af );
		engine::getSongEditor()->setModified();
	}
}








audioFileProcessor::subPluginFeatures::subPluginFeatures(
						plugin::pluginTypes _type ) :
	plugin::descriptor::subPluginFeatures( _type )
{
}




const QStringList & audioFileProcessor::subPluginFeatures::supported_extensions(
									void )
{
	static QStringList extensions = QStringList()
				<< "wav" << "ogg" << "spx" << "au" << "voc"
				<< "aif" << "aiff" << "flac" << "raw";
	return( extensions );
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new audioFileProcessor(
				static_cast<instrumentTrack *>( _data ) ) );
}


}


#include "audio_file_processor.moc"

