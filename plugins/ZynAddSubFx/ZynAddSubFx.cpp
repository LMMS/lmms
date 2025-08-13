/*
 * ZynAddSubFx.cpp - ZynAddSubxFX-embedding plugin
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "lmmsconfig.h"

#include <QDir>
#include <QDomDocument>
#include <QTemporaryFile>
#include <QtGlobal>
#include <QDropEvent>
#include <QGridLayout>
#include <QPushButton>
#include <QRegularExpression>

#include "ZynAddSubFx.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "DataFile.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "RemoteZynAddSubFx.h"
#include "LocalZynAddSubFx.h"
#include "AudioEngine.h"
#include "Clipboard.h"

#include "embed.h"
#include "FontHelper.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT zynaddsubfx_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"ZynAddSubFX",
	QT_TRANSLATE_NOOP( "PluginBrowser",
			"Embedded ZynAddSubFX" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	"xiz",
	nullptr,
} ;

}



ZynAddSubFxRemotePlugin::ZynAddSubFxRemotePlugin() :
	RemotePlugin()
{
	init( "RemoteZynAddSubFx", false );
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
	Instrument(_instrumentTrack, &zynaddsubfx_plugin_descriptor, nullptr, Flag::IsSingleStreamed | Flag::IsMidiBased),
	m_hasGUI( false ),
	m_plugin( nullptr ),
	m_remotePlugin( nullptr ),
	m_portamentoModel( 0, 0, 127, 1, this, tr( "Portamento" ) ),
	m_filterFreqModel( 64, 0, 127, 1, this, tr( "Filter frequency" ) ),
	m_filterQModel( 64, 0, 127, 1, this, tr( "Filter resonance" ) ),
	m_bandwidthModel( 64, 0, 127, 1, this, tr( "Bandwidth" ) ),
	m_fmGainModel( 127, 0, 127, 1, this, tr( "FM gain" ) ),
	m_resCenterFreqModel( 64, 0, 127, 1, this, tr( "Resonance center frequency" ) ),
	m_resBandwidthModel( 64, 0, 127, 1, this, tr( "Resonance bandwidth" ) ),
	m_forwardMidiCcModel( true, this, tr( "Forward MIDI control change events" ) )
{
	initPlugin();

	connect( &m_portamentoModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePortamento() ), Qt::DirectConnection );
	connect( &m_filterFreqModel, SIGNAL( dataChanged() ),
			this, SLOT( updateFilterFreq() ), Qt::DirectConnection );
	connect( &m_filterQModel, SIGNAL( dataChanged() ),
			this, SLOT( updateFilterQ() ), Qt::DirectConnection );
	connect( &m_bandwidthModel, SIGNAL( dataChanged() ),
			this, SLOT( updateBandwidth() ), Qt::DirectConnection );
	connect( &m_fmGainModel, SIGNAL( dataChanged() ),
			this, SLOT( updateFmGain() ), Qt::DirectConnection );
	connect( &m_resCenterFreqModel, SIGNAL( dataChanged() ),
			this, SLOT( updateResCenterFreq() ), Qt::DirectConnection );
	connect( &m_resBandwidthModel, SIGNAL( dataChanged() ),
			this, SLOT( updateResBandwidth() ), Qt::DirectConnection );

	// now we need a play-handle which cares for calling play()
	auto iph = new InstrumentPlayHandle(this, _instrumentTrack);
	Engine::audioEngine()->addPlayHandle( iph );

	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ),
			this, SLOT( reloadPlugin() ) );

	connect( instrumentTrack()->pitchRangeModel(), SIGNAL( dataChanged() ),
			this, SLOT( updatePitchRange() ), Qt::DirectConnection );
}




ZynAddSubFxInstrument::~ZynAddSubFxInstrument()
{
	Engine::audioEngine()->removePlayHandlesOfTypes( instrumentTrack(),
				PlayHandle::Type::NotePlayHandle
				| PlayHandle::Type::InstrumentPlayHandle );

	m_pluginMutex.lock();
	delete m_plugin;
	delete m_remotePlugin;
	m_plugin = nullptr;
	m_remotePlugin = nullptr;
	m_pluginMutex.unlock();
}




void ZynAddSubFxInstrument::saveSettings( QDomDocument & _doc,
											QDomElement & _this )
{
	m_portamentoModel.saveSettings( _doc, _this, "portamento" );
	m_filterFreqModel.saveSettings( _doc, _this, "filterfreq" );
	m_filterQModel.saveSettings( _doc, _this, "filterq" );
	m_bandwidthModel.saveSettings( _doc, _this, "bandwidth" );
	m_fmGainModel.saveSettings( _doc, _this, "fmgain" );
	m_resCenterFreqModel.saveSettings( _doc, _this, "rescenterfreq" );
	m_resBandwidthModel.saveSettings( _doc, _this, "resbandwidth" );

	QString modifiedControllers;
	for( QMap<int, bool>::ConstIterator it = m_modifiedControllers.begin();
									it != m_modifiedControllers.end(); ++it )
	{
		if( it.value() )
		{
			modifiedControllers += QString( "%1," ).arg( it.key() );
		}
	}
	_this.setAttribute( "modifiedcontrollers", modifiedControllers );

	m_forwardMidiCcModel.saveSettings( _doc, _this, "forwardmidicc" );

	QTemporaryFile tf;
	if( tf.open() )
	{
		const std::string fn = QSTR_TO_STDSTR(
									QDir::toNativeSeparators( tf.fileName() ) );
		m_pluginMutex.lock();
		if( m_remotePlugin )
		{
			m_remotePlugin->lock();
			m_remotePlugin->sendMessage( RemotePlugin::message( IdSaveSettingsToFile ).addString( fn ) );
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

	m_portamentoModel.loadSettings( _this, "portamento" );
	m_filterFreqModel.loadSettings( _this, "filterfreq" );
	m_filterQModel.loadSettings( _this, "filterq" );
	m_bandwidthModel.loadSettings( _this, "bandwidth" );
	m_fmGainModel.loadSettings( _this, "fmgain" );
	m_resCenterFreqModel.loadSettings( _this, "rescenterfreq" );
	m_resBandwidthModel.loadSettings( _this, "resbandwidth" );
	m_forwardMidiCcModel.loadSettings( _this, "forwardmidicc" );

	QDomDocument doc;
	QDomElement data = _this.firstChildElement( "ZynAddSubFX-data" );
	if( data.isNull() )
	{
		data = _this.firstChildElement();
	}
	doc.appendChild( doc.importNode( data, true ) );

	QTemporaryFile tf;
	if( tf.open() )
	{
		QByteArray a = doc.toString( 0 ).toUtf8();
		tf.write( a );
		tf.flush();

		const std::string fn = QSTR_TO_STDSTR( QDir::toNativeSeparators( tf.fileName() ) );
		m_pluginMutex.lock();
		if( m_remotePlugin )
		{
			m_remotePlugin->lock();
			m_remotePlugin->sendMessage( RemotePlugin::message( IdLoadSettingsFromFile ).addString( fn ) );
			m_remotePlugin->waitForMessage( IdLoadSettingsFromFile );
			m_remotePlugin->unlock();
		}
		else
		{
			m_plugin->loadXML( fn );
		}
		m_pluginMutex.unlock();

		m_modifiedControllers.clear();
		for( const QString & c : _this.attribute( "modifiedcontrollers" ).split( ',' ) )
		{
			if( !c.isEmpty() )
			{
				switch( c.toInt() )
				{
					case C_portamento: updatePortamento(); break;
					case C_filtercutoff: updateFilterFreq(); break;
					case C_filterq: updateFilterQ(); break;
					case C_bandwidth: updateBandwidth(); break;
					case C_fmamp: updateFmGain(); break;
					case C_resonance_center: updateResCenterFreq(); break;
					case C_resonance_bandwidth: updateResBandwidth(); break;
					default:
						break;
				}
			}
		}

		emit settingsChanged();
	}
	emit instrumentTrack()->pitchModel()->dataChanged();
}




void ZynAddSubFxInstrument::loadFile( const QString & _file )
{
	const std::string fn = QSTR_TO_STDSTR( _file );
	if( m_remotePlugin )
	{
		m_remotePlugin->lock();
		m_remotePlugin->sendMessage( RemotePlugin::message( IdLoadPresetFile ).addString( fn ) );
		m_remotePlugin->waitForMessage( IdLoadPresetFile );
		m_remotePlugin->unlock();
	}
	else
	{
		m_pluginMutex.lock();
		m_plugin->loadPreset( fn );
		m_pluginMutex.unlock();
	}

	instrumentTrack()->setName(QFileInfo(_file).baseName().replace(QRegularExpression("^[0-9]{4}-"), QString()));

	m_modifiedControllers.clear();

	emit settingsChanged();
}




QString ZynAddSubFxInstrument::nodeName() const
{
	return zynaddsubfx_plugin_descriptor.name;
}




void ZynAddSubFxInstrument::play( SampleFrame* _buf )
{
	if (!m_pluginMutex.tryLock(Engine::getSong()->isExporting() ? -1 : 0)) {return;}
	if( m_remotePlugin )
	{
		m_remotePlugin->process( nullptr, _buf );
	}
	else
	{
		m_plugin->processAudio( _buf );
	}
	m_pluginMutex.unlock();
}




bool ZynAddSubFxInstrument::handleMidiEvent( const MidiEvent& event, const TimePos& time, f_cnt_t offset )
{
	// do not forward external MIDI Control Change events if the according
	// LED is not checked
	if( event.type() == MidiControlChange &&
		event.sourcePort() != this &&
		m_forwardMidiCcModel.value() == false )
	{
		return true;
	}

	MidiEvent localEvent = event;
	localEvent.setChannel( 0 );
	m_pluginMutex.lock();
	if( m_remotePlugin )
	{
		m_remotePlugin->processMidiEvent( localEvent, 0 );
	}
	else
	{
		m_plugin->processMidiEvent( localEvent );
	}
	m_pluginMutex.unlock();

	return true;
}




void ZynAddSubFxInstrument::reloadPlugin()
{
	// save state of current plugin instance
	DataFile m( DataFile::Type::InstrumentTrackSettings );
	saveSettings( m, m.content() );

	// init plugin (will delete current one and create a new instance)
	initPlugin();

	// and load the settings again
	loadSettings( m.content() );
}



void ZynAddSubFxInstrument::updatePitchRange()
{
	m_pluginMutex.lock();
	if( m_remotePlugin )
	{
		m_remotePlugin->sendMessage( RemotePlugin::message( IdZasfSetPitchWheelBendRange ).
											addInt( instrumentTrack()->midiPitchRange() ) );
	}
	else
	{
		m_plugin->setPitchWheelBendRange( instrumentTrack()->midiPitchRange() );
	}
	m_pluginMutex.unlock();
}


#define GEN_CC_SLOT(slotname,midictl,modelname)						\
			void ZynAddSubFxInstrument::slotname()					\
			{														\
				sendControlChange( midictl, modelname.value() );	\
				m_modifiedControllers[midictl] = true;				\
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
	m_plugin = nullptr;
	m_remotePlugin = nullptr;

	if( m_hasGUI )
	{
		m_remotePlugin = new ZynAddSubFxRemotePlugin();
		m_remotePlugin->lock();
		m_remotePlugin->waitForInitDone( false );

		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdZasfLmmsWorkingDirectory ).
				addString(
					QSTR_TO_STDSTR(
						QString( ConfigManager::inst()->workingDir() ) ) ) );
		m_remotePlugin->sendMessage(
			RemotePlugin::message( IdZasfPresetDirectory ).
				addString(
					QSTR_TO_STDSTR(
						QDir( ConfigManager::inst()->factoryPresetsDir() +
								"/ZynAddSubFX" ).absolutePath() ) ) );

		m_remotePlugin->updateSampleRate( Engine::audioEngine()->outputSampleRate() );

		// temporary workaround until the VST synchronization feature gets stripped out of the RemotePluginClient class
		// causing not to send buffer size information requests
		m_remotePlugin->sendMessage( RemotePlugin::message( IdBufferSizeInformation ).addInt( Engine::audioEngine()->framesPerPeriod() ) );

		m_remotePlugin->showUI();
		m_remotePlugin->unlock();
	}
	else
	{
		m_plugin = new LocalZynAddSubFx;
		m_plugin->setSampleRate( Engine::audioEngine()->outputSampleRate() );
		m_plugin->setBufferSize( Engine::audioEngine()->framesPerPeriod() );
	}

	m_pluginMutex.unlock();
}




void ZynAddSubFxInstrument::sendControlChange( MidiControllers midiCtl, float value )
{
	handleMidiEvent( MidiEvent( MidiControlChange, instrumentTrack()->midiPort()->realOutputChannel(), midiCtl, (int) value, this ) );
}



gui::PluginView* ZynAddSubFxInstrument::instantiateView( QWidget * _parent )
{
	return new gui::ZynAddSubFxView( this, _parent );
}




namespace gui
{


ZynAddSubFxView::ZynAddSubFxView( Instrument * _instrument, QWidget * _parent ) :
        InstrumentViewFixedSize( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	auto l = new QGridLayout(this);
	l->setContentsMargins( 20, 80, 10, 10 );
	l->setVerticalSpacing( 16 );
	l->setHorizontalSpacing( 10 );

	m_portamento = new Knob(KnobType::Bright26, tr("PORT"), SMALL_FONT_SIZE, this);
	m_portamento->setHintText( tr( "Portamento:" ), "" );

	m_filterFreq = new Knob(KnobType::Bright26, tr("FREQ"), SMALL_FONT_SIZE, this);
	m_filterFreq->setHintText( tr( "Filter frequency:" ), "" );

	m_filterQ = new Knob(KnobType::Bright26, tr("RES"), SMALL_FONT_SIZE, this);
	m_filterQ->setHintText( tr( "Filter resonance:" ), "" );

	m_bandwidth = new Knob(KnobType::Bright26, tr("BW"), SMALL_FONT_SIZE, this);
	m_bandwidth->setHintText( tr( "Bandwidth:" ), "" );

	m_fmGain = new Knob(KnobType::Bright26, tr("FM GAIN"), SMALL_FONT_SIZE, this);
	m_fmGain->setHintText( tr( "FM gain:" ), "" );

	m_resCenterFreq = new Knob(KnobType::Bright26, tr("RES CF"), SMALL_FONT_SIZE, this);
	m_resCenterFreq->setHintText( tr( "Resonance center frequency:" ), "" );

	m_resBandwidth = new Knob(KnobType::Bright26, tr("RES BW"), SMALL_FONT_SIZE, this);
	m_resBandwidth->setHintText( tr( "Resonance bandwidth:" ), "" );

	m_forwardMidiCC = new LedCheckBox( tr( "Forward MIDI control changes" ), this );

	m_toggleUIButton = new QPushButton( tr( "Show GUI" ), this );
	m_toggleUIButton->setCheckable( true );
	m_toggleUIButton->setChecked( false );
	m_toggleUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	QFont f = m_toggleUIButton->font();
	m_toggleUIButton->setFont(adjustedToPixelSize(f, DEFAULT_FONT_SIZE));

	connect( m_toggleUIButton, SIGNAL( toggled( bool ) ), this,
							SLOT( toggleUI() ) );

	l->addWidget( m_toggleUIButton, 0, 0, 1, 4 );
	l->setRowStretch( 1, 5 );
	l->addWidget( m_portamento, 2, 0 );
	l->addWidget( m_filterFreq, 2, 1 );
	l->addWidget( m_filterQ, 2, 2 );
	l->addWidget( m_bandwidth, 2, 3 );
	l->addWidget( m_fmGain, 3, 0 );
	l->addWidget( m_resCenterFreq, 3, 1 );
	l->addWidget( m_resBandwidth, 3, 2 );
	l->addWidget( m_forwardMidiCC, 4, 0, 1, 4 );

	l->setRowStretch( 5, 10 );
	l->setColumnStretch( 4, 10 );

	setAcceptDrops( true );
}




void ZynAddSubFxView::dragEnterEvent( QDragEnterEvent * _dee )
{
	// For mimeType() and MimeType enum class
	using namespace Clipboard;

	if( _dee->mimeData()->hasFormat( mimeType( MimeType::StringPair ) ) )
	{
		QString txt = _dee->mimeData()->data(
						mimeType( MimeType::StringPair ) );
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
	const QString type = StringPairDrag::decodeKey( _de );
	const QString value = StringPairDrag::decodeValue( _de );
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
	auto m = castModel<ZynAddSubFxInstrument>();

	// set models for controller knobs
	m_portamento->setModel( &m->m_portamentoModel );
	m_filterFreq->setModel( &m->m_filterFreqModel );
	m_filterQ->setModel( &m->m_filterQModel );
	m_bandwidth->setModel( &m->m_bandwidthModel );
	m_fmGain->setModel( &m->m_fmGainModel );
	m_resCenterFreq->setModel( &m->m_resCenterFreqModel );
	m_resBandwidth->setModel( &m->m_resBandwidthModel );

	m_forwardMidiCC->setModel( &m->m_forwardMidiCcModel );

	m_toggleUIButton->setChecked( m->m_hasGUI );
}




void ZynAddSubFxView::toggleUI()
{
	auto model = castModel<ZynAddSubFxInstrument>();
	if( model->m_hasGUI != m_toggleUIButton->isChecked() )
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();
		model->reloadPlugin();

		if( model->m_remotePlugin )
		{
			connect( model->m_remotePlugin, SIGNAL( clickedCloseButton() ),
						m_toggleUIButton, SLOT( toggle() ) );
		}
	}
}


} // namespace gui



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model * m, void *)
{
	return new ZynAddSubFxInstrument(static_cast<InstrumentTrack *>(m));
}


}


} // namespace lmms
