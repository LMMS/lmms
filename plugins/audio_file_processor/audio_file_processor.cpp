/*
 * audio_file_processor.cpp - instrument for using audio-files
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QButtonGroup>
#include <QBitmap>
#include <Qt/QtXml>
#include <QFileInfo>
#include <QDropEvent>

#else

#include <qpainter.h>
#include <qbuttongroup.h>
#include <qbitmap.h>
#include <qdom.h>
#include <qfileinfo.h>
#include <qcursor.h>
#include <qwhatsthis.h>

#endif


#include "audio_file_processor.h"
#include "song_editor.h"
#include "channel_track.h"
#include "note_play_handle.h"
#include "paths.h"
#include "interpolation.h"
#include "buffer_allocator.h"
#include "pixmap_button.h"
#include "knob.h"
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
				"using samples (e.g. drums) in a channel" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::INSTRUMENT,
	PLUGIN_NAME::findEmbeddedData( "logo.png" )
} ;

}


QPixmap * audioFileProcessor::s_artwork = NULL;



audioFileProcessor::audioFileProcessor( channelTrack * _channel_track ) :
	instrument( _channel_track, &audiofileprocessor_plugin_descriptor ),
	specialBgHandlingWidget( PLUGIN_NAME::getIconPixmap( "artwork" ) ),
	m_sampleBuffer( eng(), "" ),
	m_drawMethod( sampleBuffer::LINE_CONNECT )
{
	connect( &m_sampleBuffer, SIGNAL( sampleUpdated() ), this,
						SLOT( sampleUpdated() ) );

	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}


	m_openAudioFileButton = new pixmapButton( this );
	m_openAudioFileButton->setCheckable( FALSE );
	m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openAudioFileButton->move( 200, 90 );
	m_openAudioFileButton->setActiveGraphic( embed::getIconPixmap(
							"project_open_down" ) );
	m_openAudioFileButton->setInactiveGraphic( embed::getIconPixmap(
							"project_open" ) );
	m_openAudioFileButton->setBgGraphic( getBackground(
						m_openAudioFileButton ) );
	connect( m_openAudioFileButton, SIGNAL( clicked() ), this,
						SLOT( openAudioFile() ) );
	toolTip::add( m_openAudioFileButton, tr( "Open other sample" ) );

#ifdef QT4
	m_openAudioFileButton->setWhatsThis(
#else
	QWhatsThis::add( m_openAudioFileButton,
#endif
		tr( "Click here, if you want to open another audio-file. After "
			"clicking on this button, a file-open-dialog appears "
			"and you can select your file. Settings like Looping-"
			"Mode, start- and end-point, amplify-value and so on "
			"are not reset, so please don't wonder if your sample "
			"doesn't sound like the original one..." ) );
	
	m_reverseButton = new pixmapButton( this );
	m_reverseButton->move( 160, 124 );
	m_reverseButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_on" ) );
	m_reverseButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"reverse_off" ) );
	m_reverseButton->setBgGraphic( getBackground( m_reverseButton ) );
	connect( m_reverseButton, SIGNAL( toggled( bool ) ), this,
					SLOT( reverseBtnToggled( bool ) ) );
	toolTip::add( m_reverseButton, tr( "Reverse sample" ) );
#ifdef QT4
	m_reverseButton->setWhatsThis(
#else
	QWhatsThis::add( m_reverseButton,
#endif
		tr( "If you enable this button, the whole sample is reversed. "
			"This is useful for cool effects, e.g. a reversed "
			"crash." ) );

	m_loopButton = new pixmapButton( this );
	m_loopButton->move( 180, 124 );
	m_loopButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_on" ) );
	m_loopButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_off" ) );
	m_loopButton->setBgGraphic( getBackground( m_loopButton ) );
	toolTip::add( m_loopButton,
				tr( "Loop sample at start- and end-point" ) );
#ifdef QT4
	m_loopButton->setWhatsThis(
#else
	QWhatsThis::add( m_loopButton,
#endif
		tr( "Here you can set, whether Looping-Mode is enabled. If "
			"enabled, AudioFileProcessor loops between start- and "
			"end-point of a sample until the whole note is played. "
			"This is useful for things like string- and choir-"
			"samples." ) );

	m_ampKnob = new knob( knobDark_28, this, tr( "Amplify" ), eng() );
	m_ampKnob->setRange( 0, 500, 1.0f );
	m_ampKnob->move( 6, 114 );
	m_ampKnob->setInitValue( 100.0f );
	m_ampKnob->setHintText( tr( "Amplify:" )+" ", "%" );
	m_ampKnob->setLabel( tr( "AMP" ) );
	connect( m_ampKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( ampKnobChanged( float ) ) );
#ifdef QT4
	m_ampKnob->setWhatsThis(
#else
	QWhatsThis::add( m_ampKnob,
#endif
		tr( "With this knob you can set the amplify-ratio. When you "
			"set a value of 100% your sample isn't changed. "
			"Otherwise it will be amplified up or down (your "
			"actual sample-file isn't touched!)" ) );

	m_startKnob = new knob( knobDark_28, this, tr( "Start of sample" ),
									eng() );
	m_startKnob->setRange( 0.0f, 1.0f, 0.00001f );
	m_startKnob->move( 46, 114 );
	m_startKnob->setInitValue( 0.0f );
	m_startKnob->setHintText( tr( "Startpoint:" )+" ", "" );
	m_startKnob->setLabel( tr( "START" ) );
	connect( m_startKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( startKnobChanged( float ) ) );
#ifdef QT4
	m_startKnob->setWhatsThis(
#else
	QWhatsThis::add( m_startKnob,
#endif
		tr( "With this knob you can set the point where "
			"AudioFileProcessor should begin playing your sample. "
			"If you enable Looping-Mode, this is the point to "
			"which AudioFileProcessor returns if a note is longer "
			"than the sample between start- and end-point." ) );

	m_endKnob = new knob( knobDark_28, this, tr( "End of sample" ), eng() );
	m_endKnob->setRange( 0.0f, 1.0f, 0.00001f );
	m_endKnob->move( 84, 114 );
	m_endKnob->setInitValue( 1.0f );
	m_endKnob->setHintText( tr( "Endpoint:" )+" ", "" );
	m_endKnob->setLabel( tr( "END" ) );
	connect( m_endKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( endKnobChanged( float ) ) );
#ifdef QT4
	m_endKnob->setWhatsThis(
#else
	QWhatsThis::add( m_endKnob,
#endif
		tr( "With this knob you can set the point where "
			"AudioFileProcessor should stop playing your sample. "
			"If you enable Looping-Mode, this is the point where "
			"AudioFileProcessor returns if a note is longer than "
			"the sample between start- and end-point." ) );

	m_viewLinesPB = new pixmapButton( this );
	m_viewLinesPB->move( 154, 158 );
	m_viewLinesPB->setBgGraphic( getBackground( m_viewLinesPB ) );
	if( m_drawMethod == sampleBuffer::LINE_CONNECT )
	{
#ifdef QT4
		m_viewLinesPB->setChecked( TRUE );
#else
		m_viewLinesPB->setOn( TRUE );
#endif
	}
	connect( m_viewLinesPB, SIGNAL( toggled( bool ) ), this,
					SLOT( lineDrawBtnToggled( bool ) ) );
#ifdef QT4
	m_viewLinesPB->setWhatsThis(
#else
	QWhatsThis::add( m_viewLinesPB,
#endif
		tr( "Activate this button, if your sample should be drawn "
			"with connected lines. This doesn't change the "
			"sound itself. It just gives you another view to your "
			"sample." ) );

	m_viewDotsPB = new pixmapButton( this );
	m_viewDotsPB->move( 204, 158 );
	m_viewDotsPB->setBgGraphic( getBackground( m_viewDotsPB ) );
	if( m_drawMethod == sampleBuffer::DOTS )
	{
#ifdef QT4
		m_viewDotsPB->setChecked( TRUE );
#else
		m_viewDotsPB->setOn( TRUE );
#endif
	}
	connect( m_viewDotsPB, SIGNAL( toggled( bool ) ), this,
					SLOT( dotDrawBtnToggled( bool ) ) );
#ifdef QT4
	m_viewDotsPB->setWhatsThis(
#else
	QWhatsThis::add( m_viewDotsPB,
#endif
		tr( "Activate this button, if your sample should be drawn "
			"with dots. This doesn't change the sound itself. "
			"It just gives you another view to your sample." ) );
	
	QButtonGroup * view_group = new QButtonGroup( this );
	view_group->addButton( m_viewLinesPB );
	view_group->addButton( m_viewDotsPB );
	view_group->setExclusive( TRUE );
#ifndef QT4
	view_group->hide();

	setBackgroundMode( Qt::NoBackground );
#endif
	setAcceptDrops( TRUE );
}




audioFileProcessor::~audioFileProcessor()
{
}




void audioFileProcessor::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement afp_de = _doc.createElement( nodeName() );
	afp_de.setAttribute( "src", m_sampleBuffer.audioFile() );
	if( m_sampleBuffer.audioFile() == "" )
	{
		QString s;
		afp_de.setAttribute( "sampledata", m_sampleBuffer.toBase64( s ) );
	}
	afp_de.setAttribute( "sframe", QString::number(
						m_sampleBuffer.startFrame() /
					(float)m_sampleBuffer.frames() ) );
	afp_de.setAttribute( "eframe", QString::number(
						m_sampleBuffer.endFrame() /
					(float)m_sampleBuffer.frames() ) );
	afp_de.setAttribute( "reversed", QString::number(
					m_reverseButton->isChecked() ) );
	afp_de.setAttribute( "looped", QString::number(
					m_loopButton->isChecked() ) );
	afp_de.setAttribute( "amp", QString::number( m_ampKnob->value() ) );
	_parent.appendChild( afp_de );
}




void audioFileProcessor::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "src" ) != "" )
	{
		setAudioFile( _this.attribute( "src" ) );
	}
	else if( _this.attribute( "sampledata" ) != "" )
	{
		m_sampleBuffer.loadFromBase64( _this.attribute( "srcdata" ) );
	}
	setStartAndEndKnob( _this.attribute( "sframe" ).toFloat(),
				_this.attribute( "eframe" ).toFloat() );  
	m_reverseButton->setChecked( _this.attribute( "reversed" ).toInt() );
	m_loopButton->setChecked( _this.attribute( "looped" ).toInt() );
	m_ampKnob->setValue( _this.attribute( "amp" ).toFloat() );
}




QString audioFileProcessor::nodeName( void ) const
{
	return( audiofileprocessor_plugin_descriptor.name );
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




Uint32 audioFileProcessor::getBeatLen( notePlayHandle * _n ) const
{
	const float freq_factor = BASE_FREQ /
				( getChannelTrack()->frequency( _n ) *
						DEFAULT_SAMPLE_RATE /
					eng()->getMixer()->sampleRate() );

	return( static_cast<Uint32>( floorf( ( m_sampleBuffer.endFrame() -
						m_sampleBuffer.startFrame() ) *
							freq_factor ) ) );
}




void audioFileProcessor::setAudioFile( const QString & _audio_file )
{
	// is current channel-name equal to previous-filename??
	if( getChannelTrack()->name() ==
			QFileInfo( m_sampleBuffer.audioFile() ).fileName() ||
		m_sampleBuffer.audioFile() == "" )
	{
		// then set it to new one
		getChannelTrack()->setName( QFileInfo( _audio_file
								).fileName() );
	}
	// else we don't touch the channel-name, because the user named it self

	m_sampleBuffer.setAudioFile( _audio_file );
	setStartAndEndKnob( 0.0f, 1.0f );
}





void audioFileProcessor::playNote( notePlayHandle * _n )
{
	const Uint32 frames = eng()->getMixer()->framesPerAudioBuffer();
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );

	// calculate frequency of note
	const float note_freq = getChannelTrack()->frequency( _n ) /
					( eng()->getMixer()->sampleRate() /
							DEFAULT_SAMPLE_RATE );
	if( m_sampleBuffer.play( buf, _n->totalFramesPlayed(),
					frames, note_freq,
					m_loopButton->isChecked(),
					&_n->m_pluginData ) == TRUE )
	{
		getChannelTrack()->processAudioBuffer( buf, frames, _n );
	}
	bufferAllocator::free( buf );
}




void audioFileProcessor::deleteNotePluginData( notePlayHandle * _n )
{
	if( _n->m_pluginData != NULL )
	{
		m_sampleBuffer.deleteResamplingData( &_n->m_pluginData );
	}
}




void audioFileProcessor::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( stringPairDrag::processDragEnterEvent( _dee,
		QString( "samplefile,tco_%1" ).arg( track::SAMPLE_TRACK ) ) ==
									FALSE )
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
	}
	else if( type == QString( "tco_%1" ).arg( track::SAMPLE_TRACK ) )
	{
		multimediaProject mmp( value, FALSE );
		setAudioFile( mmp.content().firstChild().toElement().
							attribute( "src" ) );
		_de->accept();
	}
	else
	{
		_de->ignore();
	}
}




void audioFileProcessor::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap pm( rect().size() );
	pm.fill( this, rect().topLeft() );

	QPainter p( &pm, this );
#endif

	p.drawPixmap( 0, 0, *s_artwork );


 	QString file_name = "";
	Uint16 idx = m_sampleBuffer.audioFile().length();

	p.setFont( pointSize<8>( p.font() ) );

	QFontMetrics fm( font() );

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
#ifdef QT4
	while( idx > 0 &&
		fm.size( Qt::TextSingleLine, file_name + "..." ).width() < 225 )
#else
	while( idx > 0 &&
		fm.size( Qt::SingleLine, file_name + "..." ).width() < 225 )
#endif
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
	const Uint32 frames = tMax( m_sampleBuffer.frames(),
						static_cast<Uint32>( 1 ) );
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

#ifndef QT4
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void audioFileProcessor::sampleUpdated( void )
{
	m_graph = QPixmap( 245, 75 );
#ifdef QT4
	QPainter p( &m_graph );
	p.drawPixmap( 2, 172, m_graph );
#else
	copyBlt( &m_graph, 0, 0, s_artwork, 2, 172, m_graph.width(),
							m_graph.height() );
	QPainter p( &m_graph );
#endif
	p.setPen( QColor( 64, 255, 160 ) );
	m_sampleBuffer.drawWaves( p, QRect( 2, 2, m_graph.width() - 4,
							m_graph.height() - 4 ),
								m_drawMethod );
	update();
}




void audioFileProcessor::reverseBtnToggled( bool _on )
{
	m_sampleBuffer.setReversed( _on );
	eng()->getSongEditor()->setModified();
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




void audioFileProcessor::setStartAndEndKnob( float _s, float _e )
{
/*	// because the signal-handlers of valuechanges of start- and end-knob
	// do range checking, depending on value of the other knob, we have to
	// disconnect the signal-handlers, set then the values, connect again
	// and then let the changes take effect...
	m_startKnob->disconnect();
	m_endKnob->disconnect();*/
	m_startKnob->setValue( _s );
	m_endKnob->setValue( _e );
/*	connect( m_startKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( startKnobChanged( float ) ) );
	connect( m_endKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( endKnobChanged( float ) ) );*/
	startKnobChanged( _s );
	endKnobChanged( _e );
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
		eng()->getSongEditor()->setModified();
	}
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new audioFileProcessor(
				static_cast<channelTrack *>( _data ) ) );
}


}


#include "audio_file_processor.moc"

