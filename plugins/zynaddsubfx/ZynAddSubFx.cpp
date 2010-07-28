/*
 * ZynAddSubFx.cpp - ZynAddSubxFX-embedding plugin
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "lmmsconfig.h"

#include <Qt/QtXml>
#include <QtCore/QTemporaryFile>
#include <QtGui/QDropEvent>
#include <QtGui/QGridLayout>
#include <QtGui/QPushButton>

#include "ZynAddSubFx.h"
#include "engine.h"
#include "knob.h"
#include "mmp.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "gui_templates.h"
#include "string_pair_drag.h"
#include "RemoteZynAddSubFx.h"
#include "LocalZynAddSubFx.h"

#include "embed.cpp"
#include "moc_ZynAddSubFx.cxx"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT zynaddsubfx_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"ZynAddSubFX",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Embedded ZynAddSubFX" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	"xiz",
	NULL,
} ;

}



ZynAddSubFxRemotePlugin::ZynAddSubFxRemotePlugin() :
	QObject(),
	RemotePlugin( "RemoteZynAddSubFx", false )
{
}




ZynAddSubFxRemotePlugin::~ZynAddSubFxRemotePlugin()
{
}



bool ZynAddSubFxRemotePlugin::processMessage( const message & _m )
{
	switch( _m.id )
	{
		case IdHideUI:
			emit clickedCloseButton();
			return true;
		default:
			break;
	}

	return RemotePlugin::processMessage( _m );
}





ZynAddSubFxInstrument::ZynAddSubFxInstrument(
									InstrumentTrack * _instrumentTrack ) :
	Instrument( _instrumentTrack, &zynaddsubfx_plugin_descriptor ),
	m_hasGUI( false ),
	m_plugin( NULL ),
	m_remotePlugin( NULL ),
	m_portamentoModel( 0, 0, 127, 1, this, tr( "Portamento" ) ),
	m_filterFreqModel( 64, 0, 127, 1, this, tr( "Filter Frequency" ) ),
	m_filterQModel( 64, 0, 127, 1, this, tr( "Filter Cutoff" ) ),
	m_bandwidthModel( 64, 0, 127, 1, this, tr( "Bandwidth" ) ),
	m_fmGainModel( 127, 0, 127, 1, this, tr( "FM Gain" ) ),
	m_resCenterFreqModel( 64, 0, 127, 1, this, tr( "Resonance Center Frequency" ) ),
	m_resBandwidthModel( 64, 0, 127, 1, this, tr( "Resonance Bandwidth" ) )
{
	initPlugin();

	connect( &m_portamentoModel, SIGNAL( dataChanged() ), this, SLOT( updatePortamento() ) );
	connect( &m_filterFreqModel, SIGNAL( dataChanged() ), this, SLOT( updateFilterFreq() ) );
	connect( &m_filterQModel, SIGNAL( dataChanged() ), this, SLOT( updateFilterQ() ) );
	connect( &m_bandwidthModel, SIGNAL( dataChanged() ), this, SLOT( updateBandwidth() ) );
	connect( &m_fmGainModel, SIGNAL( dataChanged() ), this, SLOT( updateFmGain() ) );
	connect( &m_resCenterFreqModel, SIGNAL( dataChanged() ), this, SLOT( updateResCenterFreq() ) );
	connect( &m_resBandwidthModel, SIGNAL( dataChanged() ), this, SLOT( updateResBandwidth() ) );

	// now we need a play-handle which cares for calling play()
	InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
	engine::getMixer()->addPlayHandle( iph );

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( reloadPlugin() ) );
}




ZynAddSubFxInstrument::~ZynAddSubFxInstrument()
{
	engine::getMixer()->removePlayHandles( instrumentTrack() );

	m_pluginMutex.lock();
	delete m_remotePlugin;
	m_pluginMutex.unlock();
}




void ZynAddSubFxInstrument::saveSettings( QDomDocument & _doc,
	                             QDomElement & _this )
{
	QTemporaryFile tf;
	if( tf.open() )
	{
		const std::string fn = QSTR_TO_STDSTR(
									QDir::toNativeSeparators( tf.fileName() ) );
		m_pluginMutex.lock();
		if( m_remotePlugin )
		{
			m_remotePlugin->lock();
			m_remotePlugin->sendMessage(
				RemotePlugin::message( IdSaveSettingsToFile ).addString( fn ) );
			m_remotePlugin->waitForMessage( IdSaveSettingsToFile );
			m_remotePlugin->unlock();
		}
		else
		{
			m_plugin->saveXML( fn );
		}
		m_pluginMutex.unlock();
		QByteArray a = tf.readAll();
		QDomDocument doc( "mydoc" );
		if( doc.setContent( a ) )
		{
			QDomNode n = _doc.importNode( doc.documentElement(), true );
			_this.appendChild( n );
		}
	}
}




void ZynAddSubFxInstrument::loadSettings( const QDomElement & _this )
{
	if( !_this.hasChildNodes() )
	{
		return;
	}

	QDomDocument doc;
	doc.appendChild( doc.importNode( _this.firstChild(), true ) );
	QTemporaryFile tf;
	tf.setAutoRemove( false );
	if( tf.open() )
	{
		QByteArray a = doc.toString( 0 ).toUtf8();
		tf.write( a );
		tf.flush();

		const std::string fn = QSTR_TO_STDSTR(
									QDir::toNativeSeparators( tf.fileName() ) );
		m_pluginMutex.lock();
		if( m_remotePlugin )
		{
			m_remotePlugin->lock();
			m_remotePlugin->sendMessage(
				RemotePlugin::message( IdLoadSettingsFromFile ).
															addString( fn ) );
			m_remotePlugin->waitForMessage( IdLoadSettingsFromFile );
			m_remotePlugin->unlock();
		}
		else
		{
			m_plugin->loadXML( fn );
		}
		m_pluginMutex.unlock();

		emit settingsChanged();
	}
}




void ZynAddSubFxInstrument::loadFile( const QString & _file )
{
	const std::string fn = QSTR_TO_STDSTR( _file );
	if( m_remotePlugin )
	{
		m_remotePlugin->lock();
		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdLoadPresetFromFile ).addString( fn ) );
		m_remotePlugin->waitForMessage( IdLoadPresetFromFile );
		m_remotePlugin->unlock();
	}
	else
	{
		m_pluginMutex.lock();
		m_plugin->loadPreset( fn );
		m_pluginMutex.unlock();
	}

	emit settingsChanged();
}




QString ZynAddSubFxInstrument::nodeName() const
{
	return zynaddsubfx_plugin_descriptor.name;
}




void ZynAddSubFxInstrument::play( sampleFrame * _buf )
{
	m_pluginMutex.lock();
	if( m_remotePlugin )
	{
		m_remotePlugin->process( NULL, _buf );
	}
	else
	{
		m_plugin->processAudio( _buf );
	}
	m_pluginMutex.unlock();
	instrumentTrack()->processAudioBuffer( _buf,
				engine::getMixer()->framesPerPeriod(), NULL );
}




bool ZynAddSubFxInstrument::handleMidiEvent( const midiEvent & _me,
												const midiTime & _time )
{
	if( !isMuted() )
	{
		m_pluginMutex.lock();
		if( m_remotePlugin )
		{
			m_remotePlugin->processMidiEvent( _me, 0 );
		}
		else
		{
			m_plugin->processMidiEvent( _me );
		}
		m_pluginMutex.unlock();
	}

	return true;
}




void ZynAddSubFxInstrument::reloadPlugin()
{
	// save state of current plugin instance
	multimediaProject m( multimediaProject::InstrumentTrackSettings );
	saveSettings( m, m.content() );

	// init plugin (will delete current one and create a new instance)
	initPlugin();

	// and load the settings again
	loadSettings( m.content() );
}




#define GEN_CC_SLOT(slotname,midictl,modelname)						\
			void ZynAddSubFxInstrument::slotname()					\
			{														\
				sendControlChange( midictl, modelname.value() );	\
			}


GEN_CC_SLOT(updatePortamento,C_portamento,m_portamentoModel);
GEN_CC_SLOT(updateFilterFreq,C_filtercutoff,m_filterFreqModel);
GEN_CC_SLOT(updateFilterQ,C_filterq,m_filterQModel);
GEN_CC_SLOT(updateBandwidth,C_bandwidth,m_bandwidthModel);
GEN_CC_SLOT(updateFmGain,C_fmamp,m_fmGainModel);
GEN_CC_SLOT(updateResCenterFreq,C_resonance_center,m_resCenterFreqModel);
GEN_CC_SLOT(updateResBandwidth,C_resonance_bandwidth,m_resBandwidthModel);




void ZynAddSubFxInstrument::initPlugin()
{
	m_pluginMutex.lock();
	delete m_plugin;
	delete m_remotePlugin;
	m_plugin = NULL;
	m_remotePlugin = NULL;

	if( m_hasGUI )
	{
		m_remotePlugin = new ZynAddSubFxRemotePlugin();
		m_remotePlugin->lock();
		m_remotePlugin->waitForInitDone( false );

		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdZasfLmmsWorkingDirectory ).
				addString(
					QSTR_TO_STDSTR(
						QString( configManager::inst()->workingDir() ) ) ) );
		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdZasfPresetDirectory ).
				addString(
					QSTR_TO_STDSTR(
						QString( configManager::inst()->factoryPresetsDir() +
								QDir::separator() + "ZynAddSubFX" ) ) ) );
		m_remotePlugin->showUI();
		m_remotePlugin->unlock();
	}
	else
	{
		m_plugin = new LocalZynAddSubFx;
		m_plugin->setSampleRate( engine::getMixer()->processingSampleRate() );
		m_plugin->setBufferSize( engine::getMixer()->framesPerPeriod() );
	}
	m_pluginMutex.unlock();
}




void ZynAddSubFxInstrument::sendControlChange( MidiControllers midiCtl, float value )
{
	handleMidiEvent( midiEvent( MidiControlChange, 0, midiCtl, (int) value ),
						midiTime() );
}



PluginView * ZynAddSubFxInstrument::instantiateView( QWidget * _parent )
{
	return new ZynAddSubFxView( this, _parent );
}







ZynAddSubFxView::ZynAddSubFxView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	QGridLayout * l = new QGridLayout( this );
	l->setContentsMargins( 20, 80, 10, 10 );
	l->setVerticalSpacing( 16 );
	l->setHorizontalSpacing( 10 );

	m_portamento = new knob( knobBright_26, this );
	m_portamento->setHintText( tr( "Portamento:" ) + "", "" );
	m_portamento->setLabel( tr( "PORT" ) );

	m_filterFreq = new knob( knobBright_26, this );
	m_filterFreq->setHintText( tr( "Filter Frequency:" ) + "", "" );
	m_filterFreq->setLabel( tr( "FREQ" ) );

	m_filterQ = new knob( knobBright_26, this );
	m_filterQ->setHintText( tr( "Filter Cutoff:" ) + "", "" );
	m_filterQ->setLabel( tr( "CUT" ) );

	m_bandwidth = new knob( knobBright_26, this );
	m_bandwidth->setHintText( tr( "Bandwidth:" ) + "", "" );
	m_bandwidth->setLabel( tr( "BW" ) );

	m_fmGain = new knob( knobBright_26, this );
	m_fmGain->setHintText( tr( "FM Gain:" ) + "", "" );
	m_fmGain->setLabel( tr( "FM GAIN" ) );

	m_resCenterFreq = new knob( knobBright_26, this );
	m_resCenterFreq->setHintText( tr( "Resonance center frequency:" ) + "", "" );
	m_resCenterFreq->setLabel( tr( "RES CF" ) );

	m_resBandwidth = new knob( knobBright_26, this );
	m_resBandwidth->setHintText( tr( "Resonance bandwidth:" ) + "", "" );
	m_resBandwidth->setLabel( tr( "RES BW" ) );

	m_toggleUIButton = new QPushButton( tr( "Show GUI" ), this );
	m_toggleUIButton->setCheckable( true );
	m_toggleUIButton->setChecked( false );
	m_toggleUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	m_toggleUIButton->setFont( pointSize<8>( m_toggleUIButton->font() ) );
	connect( m_toggleUIButton, SIGNAL( toggled( bool ) ), this,
							SLOT( toggleUI() ) );
	m_toggleUIButton->setWhatsThis(
		tr( "Click here to show or hide the graphical user interface "
			"(GUI) of ZynAddSubFX." ) );

	l->addWidget( m_toggleUIButton, 0, 0, 1, 4 );
	l->setRowStretch( 1, 5 );
	l->addWidget( m_portamento, 2, 0 );
	l->addWidget( m_filterFreq, 2, 1 );
	l->addWidget( m_filterQ, 2, 2 );
	l->addWidget( m_bandwidth, 2, 3 );
	l->addWidget( m_fmGain, 3, 0 );
	l->addWidget( m_resCenterFreq, 3, 1 );
	l->addWidget( m_resBandwidth, 3, 2 );
	l->setRowStretch( 4, 10 );
	l->setColumnStretch( 4, 10 );

	setAcceptDrops( true );
}





ZynAddSubFxView::~ZynAddSubFxView()
{
}




void ZynAddSubFxView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( stringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(
						stringPairDrag::mimeType() );
		if( txt.section( ':', 0, 0 ) == "pluginpresetfile" )
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




void ZynAddSubFxView::dropEvent( QDropEvent * _de )
{
	const QString type = stringPairDrag::decodeKey( _de );
	const QString value = stringPairDrag::decodeValue( _de );
	if( type == "pluginpresetfile" )
	{
		castModel<ZynAddSubFxInstrument>()->loadFile( value );
		_de->accept();
		return;
	}
	_de->ignore();
}




void ZynAddSubFxView::modelChanged()
{
	ZynAddSubFxInstrument * m = castModel<ZynAddSubFxInstrument>();

	// set models for controller knobs
	m_portamento->setModel( &m->m_portamentoModel );
	m_filterFreq->setModel( &m->m_filterFreqModel );
	m_filterQ->setModel( &m->m_filterQModel );
	m_bandwidth->setModel( &m->m_bandwidthModel );
	m_fmGain->setModel( &m->m_fmGainModel );
	m_resCenterFreq->setModel( &m->m_resCenterFreqModel );
	m_resBandwidth->setModel( &m->m_resBandwidthModel );

	toggleUI();
}




void ZynAddSubFxView::toggleUI()
{
	ZynAddSubFxInstrument * model = castModel<ZynAddSubFxInstrument>();
	model->m_hasGUI = m_toggleUIButton->isChecked();
	model->reloadPlugin();

	if( model->m_remotePlugin )
	{
		connect( model->m_remotePlugin, SIGNAL( clickedCloseButton() ),
					m_toggleUIButton, SLOT( toggle() ) );
	}
}





extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{

	return new ZynAddSubFxInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}


