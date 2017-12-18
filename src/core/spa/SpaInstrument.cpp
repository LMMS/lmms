/*
 * spainstrument.cpp - implementation of SPA interface
 *
 * Copyright (c) 2018-2018 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QTemporaryFile>

#define SPA_INSTRUMENT_USE_QLIBRARY

#ifdef SPA_INSTRUMENT_USE_QLIBRARY
	#include <QLibrary>
#else
	#include <dlfcn.h>
#endif

#include <spa/audio.h>

#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "gui_templates.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "../include/RemotePlugin.h" // QSTR_TO_STDSTR
#include "StringPairDrag.h" // DnD

#include "SpaInstrument.h"


#include "embed.h"

SpaInstrument::SpaInstrument(InstrumentTrack * _instrumentTrack,
			const char* libraryName,
			const Descriptor* plugin_descriptor) :
	Instrument( _instrumentTrack, plugin_descriptor ),
	ports(Engine::mixer()->framesPerPeriod()),
	m_hasGUI( false ),
	libraryName(libraryName)
{
	for( int i = 0; i < NumKeys; ++i )
	 m_runningNotes[i] = 0;

	initPlugin();

	if(plugin)
	{
		// now we need a play-handle which cares for calling play()
		InstrumentPlayHandle * iph = new InstrumentPlayHandle( this,
			_instrumentTrack );
		Engine::mixer()->addPlayHandle( iph );

		connect( Engine::mixer(), SIGNAL( sampleRateChanged() ),
				this, SLOT( reloadPlugin() ) );
	}

	connect( instrumentTrack()->pitchRangeModel(), SIGNAL( dataChanged() ),
		this, SLOT( updatePitchRange() ) );
}




SpaInstrument::~SpaInstrument()
{
	shutdownPlugin();
	Engine::mixer()->removePlayHandlesOfTypes( instrumentTrack(),
				PlayHandle::TypeNotePlayHandle |
				PlayHandle::TypeInstrumentPlayHandle );
}

// not yet working
#ifndef SPA_INSTRUMENT_USE_MIDI
void SpaInstrument::playNote(NotePlayHandle *_n, sampleFrame *)
{
	// no idea what that means
	if( _n->isMasterNote() || ( _n->hasParent() && _n->isReleased() ) )
	{
		return;
	}

	const f_cnt_t tfp = _n->totalFramesPlayed();

	const float LOG440 = 2.643452676f;

	int midiNote = (int)floor( 12.0 * ( log2( _n->unpitchedFrequency() ) - LOG440 ) - 4.0 );

	qDebug() << "midiNote: " << midiNote << ", r? " << _n->isReleased();
	// out of range?
	if( midiNote <= 0 || midiNote >= 128 )
	{
		return;
	}

	if( tfp == 0 )
	{
		const int baseVelocity = instrumentTrack()->midiPort()->baseVelocity();
		plugin->send_osc("/noteOn", "iii", 0, midiNote, baseVelocity);
	}
	else if( _n->isReleased() && ! _n->instrumentTrack()->isSustainPedalPressed() ) // note is released during this period
	{
		plugin->send_osc("/noteOff", "ii", 0, midiNote);
	}
	else if( _n->framesLeft() <= 0 )
	{
		plugin->send_osc("/noteOff", "ii", 0, midiNote);
	}
}
#endif



void SpaInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if(!descriptor->save_has())
		return;

	QTemporaryFile tf;
	if( tf.open() )
	{
		const std::string fn = QSTR_TO_STDSTR( QDir::toNativeSeparators(
							tf.fileName() ) );
		m_pluginMutex.lock();
		plugin->save(fn.c_str(), ++save_ticket);
		m_pluginMutex.unlock();

		while(!plugin->save_check(fn.c_str(),save_ticket))
			QThread::msleep(1);

		QDomCDATASection cdata = _doc.createCDATASection(
			QString::fromUtf8(tf.readAll()));
		_this.appendChild(cdata);
	}
	tf.remove();

	if(connectedModels.size())
	{
		QDomElement newNode = _doc.createElement("connected-models");
		QMap<QString, AutomatableModel*>::const_iterator i =
			connectedModels.constBegin();
		while (i != connectedModels.constEnd()) {
			i.value()->saveSettings( _doc, newNode, i.key() );
			++i;
		}

		_this.appendChild(newNode);
	}
}




void SpaInstrument::loadSettings( const QDomElement & _this )
{
	if(!descriptor->load_has())
		return;

	if( !_this.hasChildNodes() )
	{
		return;
	}

	for(QDomNode node = _this.firstChild(); !node.isNull();
		node = node.nextSibling())
	{

		QDomCDATASection cdata = node.toCDATASection();
		QDomElement elem;
		if(!cdata.isNull())
		{

			QTemporaryFile tf;
			tf.setAutoRemove( false );
			if( tf.open() )
			{
				tf.write( cdata.data().toUtf8() );
				tf.flush();
				loadFileInternal(tf.fileName());
				emit settingsChanged();
			}
		}
		else if(node.nodeName() == "connected-models" &&
			!(elem = node.toElement()).isNull())
		{
			for(QDomElement portnode = elem.firstChildElement();
				!portnode.isNull();
				portnode = portnode.nextSiblingElement())
			{
				QString name = portnode.nodeName();
				if(name == "automatablemodel")
					name = portnode.attribute("nodename");
				using fact = SpaOscModelFactory;
				AutomatableModel* m = fact(this, name).res;
				m->loadSettings( elem, name );
				connectedModels[name] = m;
			}
		}
	}
}


void SpaInstrument::loadFileInternal( const QString & _file )
{
	const QByteArray fn = _file.toUtf8();
	m_pluginMutex.lock();
	plugin->load(fn.data(), ++save_ticket);
	while(!plugin->load_check(fn.data(),
		save_ticket))
		QThread::msleep(1);
	m_pluginMutex.unlock();
}


void SpaInstrument::loadFile( const QString & _file )
{
	loadFileInternal(_file);
	instrumentTrack()->setName( QFileInfo( _file ).baseName().
		replace( QRegExp( "^[0-9]{4}-" ), QString() ) );
	emit settingsChanged();
}


QString SpaInstrument::nodeName() const
{
	return Plugin::descriptor()->name;
}


void SpaInstrument::play( sampleFrame * _buf )
{
	if(plugin)
	{
m_pluginMutex.lock();
		ports.samplecount = ports.buffersize;
		plugin->run();
m_pluginMutex.unlock();
		for( std::size_t f = 0; f < ports.buffersize; ++f )
		{
			_buf[f][0] = ports.l_processed[f];
			_buf[f][1] = ports.r_processed[f];
		}
	}
	instrumentTrack()->processAudioBuffer( _buf,
		Engine::mixer()->framesPerPeriod(), nullptr );
}


void SpaInstrument::reloadPlugin()
{
	// refresh ports that are only read on restore
	ports.samplerate = Engine::mixer()->processingSampleRate();
	ports.buffersize = Engine::mixer()->framesPerPeriod();

	if(descriptor->restore_has())
	{
		// use the offered restore function
		m_pluginMutex.lock();
		plugin->restore(++restore_ticket);
		m_pluginMutex.unlock();

		while(!plugin->restore_check(restore_ticket))
			QThread::msleep(1);
	}
	else
	{
		// save state of current plugin instance
		DataFile m( DataFile::InstrumentTrackSettings );
		saveSettings( m, m.content() );

		shutdownPlugin();
		// init plugin (will create a new instance)
		initPlugin();

		// and load the settings again
		loadSettings( m.content() );
	}
}

void SpaInstrument::updatePitchRange()
{
	qDebug() << "Lmms: Cannot update pitch range for spa plugin:"
		"not implemented yet";
}

void SpaInstrument::shutdownPlugin()
{
	plugin->deactivate();

	delete plugin;
	plugin = nullptr;
	delete descriptor;
	descriptor = nullptr;

	m_pluginMutex.lock();
	if(lib) {
#ifdef SPA_INSTRUMENT_USE_QLIBRARY
		lib->unload();
		delete lib;
		lib = nullptr;
#else
		dlclose(lib);
		lib = nullptr;
#endif
	}
	m_pluginMutex.unlock();
}

struct lmms_visitor final : public virtual spa::audio::visitor
{
	SpaInstrument::lmms_ports* ports;

	void visit(spa::audio::in& p) override {
		qDebug() << "in, c: " << +p.channel;
		p.set_ref((p.channel == spa::audio::stereo::left)
			? ports->l_unprocessed.data()
			: ports->r_unprocessed.data()); }
	void visit(spa::audio::out& p) override {
		qDebug() << "out, c: %d\n" << +p.channel;
		p.set_ref((p.channel == spa::audio::stereo::left)
			? ports->l_processed.data()
			: ports->r_processed.data()); }
	void visit(spa::audio::stereo::in& p) override {
		qDebug() << "in, stereo";
		p.left = ports->l_unprocessed.data();
		p.right = ports->r_unprocessed.data(); }
	void visit(spa::audio::stereo::out& p) override {
		qDebug() << "out, stereo";
		p.left = ports->l_processed.data();
		p.right = ports->r_processed.data(); }
	void visit(spa::audio::buffersize& p) override {
		qDebug() << "buffersize";
		p.set_ref(&ports->buffersize); }
	void visit(spa::audio::samplerate& p) override {
		qDebug() << "samplerate";
		p.set_ref(&ports->samplerate); }
	void visit(spa::audio::samplecount& p) override {
		qDebug() << "samplecount";
		p.set_ref(&ports->samplecount); }
	void visit(spa::port_ref<const float>& p) override {
		qDebug() << "unknown control port";
		ports->unknown_controls.push_back(.0f);
		p.set_ref(&ports->unknown_controls.back()); }
	void visit(spa::audio::osc_ringbuffer_in& p) override {
		qDebug() << "ringbuffer input";
		if(ports->rb)
			throw std::runtime_error("can not handle 2 OSC ports");
		else {
			ports->rb.reset(
				new spa::audio::osc_ringbuffer(p.get_size()));
			p.connect(*ports->rb);
		}
	}
	void visit(spa::port_ref_base& ) override {
		qDebug() << "port of unknown type"; }

};


bool SpaInstrument::initPlugin()
{
	m_pluginMutex.lock();

	spa::descriptor_loader_t spa_descriptor_loader;
#ifdef SPA_INSTRUMENT_USE_QLIBRARY
	lib = new QLibrary(libraryName);
	lib->load();

	if(!lib->isLoaded())
		qDebug() << "Warning: Could not load library " << libraryName <<
			": " << lib->errorString();

	spa_descriptor_loader =
		(spa::descriptor_loader_t) lib->resolve(spa::descriptor_name);
#else
	lib = dlopen(libraryName.toAscii().data(), RTLD_LAZY | RTLD_LOCAL);
	if(!lib)
	 qDebug() << "Warning: Could not load library " << libraryName
		<< ": " << dlerror();

	*(void **) (&spa_descriptor_loader) = dlsym(lib, spa::descriptor_name);
#endif


	if(!spa_descriptor_loader)
		qDebug() << "Warning: Could not resolve \"osc_descriptor\" in "
			<< libraryName;

	if(spa_descriptor_loader)
	{
		descriptor = (*spa_descriptor_loader)(0 /* = plugin number, TODO */);
		if(descriptor)
		try {
			spa::assert_versions_match(*descriptor);
			plugin = descriptor->instantiate();
			// TODO: unite error handling in the ctor
		} catch(spa::version_mismatch& mismatch) {
			qCritical() << "Version mismatch loading plugin: "
				<< mismatch.what();
				// TODO: make an operator<<
			qCritical() << "Got: "
				<< mismatch.version.major() << "."
				<< mismatch.version.minor() << "."
				<< mismatch.version.patch()
				<< ", expect at least "
				<< mismatch.least_version.major() << "."
				<< mismatch.least_version.minor() << "."
				<< mismatch.least_version.patch();
			descriptor = nullptr;
		}
	}
	m_pluginMutex.unlock();

	ports.samplerate = Engine::mixer()->processingSampleRate();
	spa::simple_vec<spa::simple_str> port_names = descriptor->port_names();
	for(const spa::simple_str& portname : port_names)
	{
		try
		{
			//qDebug() << "portname: " << portname.data();
			spa::port_ref_base& port_ref = plugin->port(
				portname.data());

			lmms_visitor v;
			v.ports = &ports;
			port_ref.accept(v);
		}
		catch(spa::port_not_found& e) {
			if(e.portname)
				qWarning() << "plugin specifies invalid port \""
					<< e.portname
					<< "\", but does not provide it";
			else
				qWarning() << "plugin specifies invalid port, "
					"but does not provide it";
			plugin = nullptr; // TODO: free plugin, handle etc...
			return false;
		}
	}

	// all initial ports are already set, we can do
	// initialization of buffers etc.
	plugin->init();

	plugin->activate();

	// checks not yet implemented:
//	spa::host_utils::check_ports(descriptor, plugin);
//	plugin->test_more();

	return true;
}

#ifdef SPA_INSTRUMENT_USE_MIDI
bool SpaInstrument::handleMidiEvent( const MidiEvent& event,
	const MidiTime& time, f_cnt_t offset )
{
	switch(event.type())
	{
		// the old zynaddsubfx plugin always uses channel 0
		case MidiNoteOn:
			if( event.velocity() > 0 )
			{
				if( event.key() <= 0 || event.key() >= 128 )
				{
					break;
				}
				if( m_runningNotes[event.key()] > 0 )
				{
					m_pluginMutex.lock();
					writeOsc("/noteOff", "ii",
						0, event.key());
					m_pluginMutex.unlock();
				}
				++m_runningNotes[event.key()];
				m_pluginMutex.lock();
				writeOsc("/noteOn", "iii",
					0, event.key(), event.velocity());
				m_pluginMutex.unlock();
				break;
			}
		case MidiNoteOff:
			if( event.key() > 0 && event.key() < 128 )
			if( --m_runningNotes[event.key()] <= 0 )
			{
				m_pluginMutex.lock();
				writeOsc("/noteOff", "ii", 0, event.key());
				m_pluginMutex.unlock();
			}
			break;
	/*              case MidiPitchBend:
			m_master->SetController( event.channel(), C_pitchwheel,
				event.pitchBend()-8192 );
			break;
		case MidiControlChange:
			m_master->SetController( event.channel(),
				midiIn.getcontroller( event.controllerNumber() ),
				event.controllerValue() );
			break;*/
		default:
			break;

	}

	return true;
}
#endif

PluginView * SpaInstrument::instantiateView( QWidget * _parent )
{
	return new SpaView( this, _parent );
}

void SpaInstrument::writeOsc(const char *dest, const char *args, va_list va)
{
	ports.rb->write(dest, args, va);
}

void SpaInstrument::writeOsc(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOsc(dest, args, va);
	va_end(va);
}


SpaView::SpaView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );

	QGridLayout * l = new QGridLayout( this );

	m_toggleUIButton = new QPushButton( tr( "Show GUI" ), this );
	m_toggleUIButton->setCheckable( true );
	m_toggleUIButton->setChecked( false );
	m_toggleUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
	m_toggleUIButton->setFont( pointSize<8>( m_toggleUIButton->font() ) );
	connect( m_toggleUIButton, SIGNAL( toggled( bool ) ), this,
							SLOT( toggleUI() ) );
	m_toggleUIButton->setWhatsThis(
		tr( "Click here to show or hide the graphical user interface "
			"(GUI) of Osc." ) );

	m_reloadPluginButton = new QPushButton( tr( "Reload Plugin" ), this );

	connect( m_reloadPluginButton, SIGNAL( toggled( bool ) ), this,
							SLOT( reloadPlugin() ) );

	l->addWidget( m_toggleUIButton, 0, 0 );
	l->addWidget( m_reloadPluginButton, 0, 1 );

	setAcceptDrops( true );
}





SpaView::~SpaView()
{
	SpaInstrument * model = castModel<SpaInstrument>();
	if( model && model->descriptor->ui_ext() && model->m_hasGUI )
	{
		qDebug() << "shutting down UI...";
		model->plugin->ui_ext_show(false);
	}
}




void SpaView::dragEnterEvent( QDragEnterEvent * _dee )
{
	void (QDragEnterEvent::*reaction)(void) = &QDragEnterEvent::ignore;

	if( _dee->mimeData()->hasFormat( StringPairDrag::mimeType() ) )
	{
		const QString txt = _dee->mimeData()->data(
					StringPairDrag::mimeType() );
		if(txt.section( ':', 0, 0 ) == "pluginpresetfile")
			reaction = &QDragEnterEvent::acceptProposedAction;
	}

	(_dee->*reaction)();
}




void SpaView::dropEvent( QDropEvent * _de )
{
	const QString type = StringPairDrag::decodeKey( _de );
	const QString value = StringPairDrag::decodeValue( _de );
	if( type == "pluginpresetfile" )
	{
		castModel<SpaInstrument>()->loadFile( value );
		_de->accept();
		return;
	}
	_de->ignore();
}

void SpaView::modelChanged()
{
	SpaInstrument * m = castModel<SpaInstrument>();

/*	// set models for controller knobs
	m_portamento->setModel( &m->m_portamentoModel ); */

	m_toggleUIButton->setChecked( m->m_hasGUI );
}




void SpaView::toggleUI()
{
	SpaInstrument * model = castModel<SpaInstrument>();
	if( model->descriptor->ui_ext() &&
		model->m_hasGUI != m_toggleUIButton->isChecked() )
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();
		model->plugin->ui_ext_show(model->m_hasGUI);
		ControllerConnection::finalizeConnections();
	}
}

void SpaView::reloadPlugin()
{
	SpaInstrument * model = castModel<SpaInstrument>();
	model->reloadPlugin();
}


SpaInstrument::lmms_ports::lmms_ports(int buffersize) :
	buffersize(buffersize),
	l_unprocessed(buffersize),
	r_unprocessed(buffersize),
	l_processed(buffersize),
	r_processed(buffersize)
{
}
