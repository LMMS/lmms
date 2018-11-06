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

#include "../include/RemotePlugin.h" // QSTR_TO_STDSTR
#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "SpaInstrument.h"
#include "StringPairDrag.h" // DnD
#include "embed.h"
#include "gui_templates.h"

SpaInstrument::SpaInstrument(InstrumentTrack *instrumentTrackArg,
	const char *libraryName, const Descriptor *pluginDescriptor) :
	Instrument(instrumentTrackArg, pluginDescriptor),
	m_ports(Engine::mixer()->framesPerPeriod()), m_hasGUI(false),
	m_libraryName(libraryName)
{
	for (int i = 0; i < NumKeys; ++i)
		m_runningNotes[i] = 0;

	initPlugin();

	if (m_plugin)
	{
		// now we need a play-handle which cares for calling play()
		InstrumentPlayHandle *iph =
			new InstrumentPlayHandle(this, instrumentTrackArg);
		Engine::mixer()->addPlayHandle(iph);

		connect(Engine::mixer(), SIGNAL(sampleRateChanged()), this,
			SLOT(reloadPlugin()));
	}

	connect(instrumentTrack()->pitchRangeModel(), SIGNAL(dataChanged()),
		this, SLOT(updatePitchRange()));
}

SpaInstrument::~SpaInstrument()
{
	shutdownPlugin();
	Engine::mixer()->removePlayHandlesOfTypes(instrumentTrack(),
		PlayHandle::TypeNotePlayHandle |
			PlayHandle::TypeInstrumentPlayHandle);
}

// not yet working
#ifndef SPA_INSTRUMENT_USE_MIDI
void SpaInstrument::playNote(NotePlayHandle *nph, sampleFrame *)
{
	// no idea what that means
	if (nph->isMasterNote() || (nph->hasParent() && nph->isReleased()))
	{
		return;
	}

	const f_cnt_t tfp = nph->totalFramesPlayed();

	const float LOG440 = 2.643452676f;

	int midiNote = (int)floor(
		12.0 * (log2(nph->unpitchedFrequency()) - LOG440) - 4.0);

	qDebug() << "midiNote: " << midiNote << ", r? " << nph->isReleased();
	// out of range?
	if (midiNote <= 0 || midiNote >= 128)
	{
		return;
	}

	if (tfp == 0)
	{
		const int baseVelocity =
			instrumentTrack()->midiPort()->baseVelocity();
		plugin->send_osc("/noteOn", "iii", 0, midiNote, baseVelocity);
	}
	else if (nph->isReleased() &&
		!nph->instrumentTrack()
			 ->isSustainPedalPressed()) // note is released during
						    // this period
	{
		plugin->send_osc("/noteOff", "ii", 0, midiNote);
	}
	else if (nph->framesLeft() <= 0)
	{
		plugin->send_osc("/noteOff", "ii", 0, midiNote);
	}
}
#endif

void SpaInstrument::saveSettings(QDomDocument &doc, QDomElement &that)
{
	if (!m_descriptor->save_has())
		return;

	QTemporaryFile tf;
	if (tf.open())
	{
		const std::string fn =
			QSTR_TO_STDSTR(QDir::toNativeSeparators(tf.fileName()));
		m_pluginMutex.lock();
		m_plugin->save(fn.c_str(), ++m_saveTicket);
		m_pluginMutex.unlock();

		while (!m_plugin->save_check(fn.c_str(), m_saveTicket))
			QThread::msleep(1);

		QDomCDATASection cdata = doc.createCDATASection(
			QString::fromUtf8(tf.readAll()));
		that.appendChild(cdata);
	}
	tf.remove();

	if (m_connectedModels.size())
	{
		QDomElement newNode = doc.createElement("connected-models");
		QMap<QString, AutomatableModel *>::const_iterator i =
			m_connectedModels.constBegin();
		while (i != m_connectedModels.constEnd())
		{
			i.value()->saveSettings(doc, newNode, i.key());
			++i;
		}

		that.appendChild(newNode);
	}
}

void SpaInstrument::loadSettings(const QDomElement &that)
{
	if (!m_descriptor->load_has())
		return;

	if (!that.hasChildNodes())
	{
		return;
	}

	for (QDomNode node = that.firstChild(); !node.isNull();
		node = node.nextSibling())
	{
		QDomCDATASection cdata = node.toCDATASection();
		QDomElement elem;
		if (!cdata.isNull())
		{

			QTemporaryFile tf;
			tf.setAutoRemove(false);
			if (tf.open())
			{
				tf.write(cdata.data().toUtf8());
				tf.flush();
				loadFileInternal(tf.fileName());
				emit settingsChanged();
			}
		}
		else if (node.nodeName() == "connected-models" &&
			!(elem = node.toElement()).isNull())
		{
			for (QDomElement portnode = elem.firstChildElement();
				!portnode.isNull();
				portnode = portnode.nextSiblingElement())
				if (portnode.nodeName() != "connection")
				{
					QString name = portnode.nodeName();
					if (name == "automatablemodel")
						name = portnode.attribute(
							"nodename");
					using fact = SpaOscModelFactory;
					AutomatableModel *m =
						fact(this, name).m_res;
					m->loadSettings(elem, name);
					m_connectedModels[name] = m;
				}
		}
	}
}

void SpaInstrument::loadFileInternal(const QString &file)
{
	const QByteArray fn = file.toUtf8();
	m_pluginMutex.lock();
	m_plugin->load(fn.data(), ++m_saveTicket);
	while (!m_plugin->load_check(fn.data(), m_saveTicket))
		QThread::msleep(1);
	m_pluginMutex.unlock();
}

void SpaInstrument::loadFile(const QString &file)
{
	loadFileInternal(file);
	instrumentTrack()->setName(QFileInfo(file).baseName().replace(
		QRegExp("^[0-9]{4}-"), QString()));
	emit settingsChanged();
}

QString SpaInstrument::nodeName() const { return Plugin::descriptor()->name; }

void SpaInstrument::play(sampleFrame *buf)
{
	if (m_plugin)
	{
		m_pluginMutex.lock();
		m_ports.samplecount = m_ports.buffersize;
		m_plugin->run();
		m_pluginMutex.unlock();
		for (std::size_t f = 0; f < m_ports.buffersize; ++f)
		{
			buf[f][0] = m_ports.m_lProcessed[f];
			buf[f][1] = m_ports.m_rProcessed[f];
		}
	}
	instrumentTrack()->processAudioBuffer(
		buf, Engine::mixer()->framesPerPeriod(), nullptr);
}

void SpaInstrument::reloadPlugin()
{
	// refresh ports that are only read on restore
	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	m_ports.buffersize = Engine::mixer()->framesPerPeriod();

	if (m_descriptor->restore_has())
	{
		// use the offered restore function
		m_pluginMutex.lock();
		m_plugin->restore(++m_restoreTicket);
		m_pluginMutex.unlock();

		while (!m_plugin->restore_check(m_restoreTicket))
			QThread::msleep(1);
	}
	else
	{
		// save state of current plugin instance
		DataFile m(DataFile::InstrumentTrackSettings);
		saveSettings(m, m.content());

		shutdownPlugin();
		// init plugin (will create a new instance)
		initPlugin();

		// and load the settings again
		loadSettings(m.content());
	}
}

void SpaInstrument::updatePitchRange()
{
	qDebug() << "Lmms: Cannot update pitch range for spa plugin:"
		    "not implemented yet";
}

void SpaInstrument::shutdownPlugin()
{
	m_plugin->deactivate();

	delete m_plugin;
	m_plugin = nullptr;
	delete m_descriptor;
	m_descriptor = nullptr;

	m_pluginMutex.lock();
	if (m_lib)
	{
#ifdef SPA_INSTRUMENT_USE_QLIBRARY
		m_lib->unload();
		delete m_lib;
		m_lib = nullptr;
#else
		dlclose(lib);
		lib = nullptr;
#endif
	}
	m_pluginMutex.unlock();
}

struct lmmsVisitor final : public virtual spa::audio::visitor
{
	SpaInstrument::lmmsPorts *ports;

	void visit(spa::audio::in &p) override
	{
		qDebug() << "in, c: " << +p.channel;
		p.set_ref((p.channel == spa::audio::stereo::left)
				? ports->m_lUnprocessed.data()
				: ports->m_rUnprocessed.data());
	}
	void visit(spa::audio::out &p) override
	{
		qDebug() << "out, c: %d\n" << +p.channel;
		p.set_ref((p.channel == spa::audio::stereo::left)
				? ports->m_lProcessed.data()
				: ports->m_rProcessed.data());
	}
	void visit(spa::audio::stereo::in &p) override
	{
		qDebug() << "in, stereo";
		p.left = ports->m_lUnprocessed.data();
		p.right = ports->m_rUnprocessed.data();
	}
	void visit(spa::audio::stereo::out &p) override
	{
		qDebug() << "out, stereo";
		p.left = ports->m_lProcessed.data();
		p.right = ports->m_rProcessed.data();
	}
	void visit(spa::audio::buffersize &p) override
	{
		qDebug() << "buffersize";
		p.set_ref(&ports->buffersize);
	}
	void visit(spa::audio::samplerate &p) override
	{
		qDebug() << "samplerate";
		p.set_ref(&ports->samplerate);
	}
	void visit(spa::audio::samplecount &p) override
	{
		qDebug() << "samplecount";
		p.set_ref(&ports->samplecount);
	}
	void visit(spa::port_ref<const float> &p) override
	{
		qDebug() << "unknown control port";
		ports->m_unknownControls.push_back(.0f);
		p.set_ref(&ports->m_unknownControls.back());
	}
	void visit(spa::audio::osc_ringbuffer_in &p) override
	{
		qDebug() << "ringbuffer input";
		if (ports->rb)
			throw std::runtime_error("can not handle 2 OSC ports");
		else
		{
			ports->rb.reset(
				new spa::audio::osc_ringbuffer(p.get_size()));
			p.connect(*ports->rb);
		}
	}
	void visit(spa::port_ref_base &) override
	{
		qDebug() << "port of unknown type";
	}
};

bool SpaInstrument::initPlugin()
{
	m_pluginMutex.lock();

	spa::descriptor_loader_t spaDescriptorLoader;
#ifdef SPA_INSTRUMENT_USE_QLIBRARY
	m_lib = new QLibrary(m_libraryName);
	m_lib->load();

	if (!m_lib->isLoaded())
		qDebug() << "Warning: Could not load library " << m_libraryName
			 << ": " << m_lib->errorString();

	spaDescriptorLoader =
		(spa::descriptor_loader_t)m_lib->resolve(spa::descriptor_name);
#else
	lib = dlopen(libraryName.toAscii().data(), RTLD_LAZY | RTLD_LOCAL);
	if (!lib)
		qDebug() << "Warning: Could not load library " << libraryName
			 << ": " << dlerror();

	*(void **)(&spaDescriptorLoader) = dlsym(lib, spa::descriptor_name);
#endif

	if (!spaDescriptorLoader)
		qDebug() << "Warning: Could not resolve \"osc_descriptor\" in "
			 << m_libraryName;

	if (spaDescriptorLoader)
	{
		m_descriptor =
			(*spaDescriptorLoader)(0 /* = plugin number, TODO */);
		if (m_descriptor)
		{
			try
			{
				spa::assert_versions_match(*m_descriptor);
				m_plugin = m_descriptor->instantiate();
				// TODO: unite error handling in the ctor
			}
			catch (spa::version_mismatch &mismatch)
			{
				qCritical()
					<< "Version mismatch loading plugin: "
					<< mismatch.what();
				// TODO: make an operator<<
				qCritical()
					<< "Got: " << mismatch.version.major()
					<< "." << mismatch.version.minor()
					<< "." << mismatch.version.patch()
					<< ", expect at least "
					<< mismatch.least_version.major() << "."
					<< mismatch.least_version.minor() << "."
					<< mismatch.least_version.patch();
				m_descriptor = nullptr;
			}
		}
	}
	m_pluginMutex.unlock();

	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	spa::simple_vec<spa::simple_str> portNames = m_descriptor->port_names();
	for (const spa::simple_str &portname : portNames)
	{
		try
		{
			// qDebug() << "portname: " << portname.data();
			spa::port_ref_base &port_ref =
				m_plugin->port(portname.data());

			lmmsVisitor v;
			v.ports = &m_ports;
			port_ref.accept(v);
		}
		catch (spa::port_not_found &e)
		{
			if (e.portname)
				qWarning() << "plugin specifies invalid port \""
					   << e.portname
					   << "\", but does not provide it";
			else
				qWarning() << "plugin specifies invalid port, "
					      "but does not provide it";
			m_plugin = nullptr; // TODO: free plugin, handle etc...
			return false;
		}
	}

	// all initial ports are already set, we can do
	// initialization of buffers etc.
	m_plugin->init();

	m_plugin->activate();

	// checks not yet implemented:
	//	spa::host_utils::check_ports(descriptor, plugin);
	//	plugin->test_more();

	return true;
}

#ifdef SPA_INSTRUMENT_USE_MIDI
bool SpaInstrument::handleMidiEvent(
	const MidiEvent &event, const MidiTime &time, f_cnt_t offset)
{
	switch (event.type())
	{
	// the old zynaddsubfx plugin always uses channel 0
	case MidiNoteOn:
		if (event.velocity() > 0)
		{
			if (event.key() <= 0 || event.key() >= 128)
			{
				break;
			}
			if (m_runningNotes[event.key()] > 0)
			{
				m_pluginMutex.lock();
				writeOsc("/noteOff", "ii", 0, event.key());
				m_pluginMutex.unlock();
			}
			++m_runningNotes[event.key()];
			m_pluginMutex.lock();
			writeOsc("/noteOn", "iii", 0, event.key(),
				event.velocity());
			m_pluginMutex.unlock();
			break;
		}
	case MidiNoteOff:
		if (event.key() > 0 && event.key() < 128)
			if (--m_runningNotes[event.key()] <= 0)
			{
				m_pluginMutex.lock();
				writeOsc("/noteOff", "ii", 0, event.key());
				m_pluginMutex.unlock();
			}
		break;
		/*              case MidiPitchBend:
				m_master->SetController( event.channel(),
		   C_pitchwheel, event.pitchBend()-8192 ); break; case
		   MidiControlChange: m_master->SetController( event.channel(),
					midiIn.getcontroller(
		   event.controllerNumber() ), event.controllerValue() );
				break;*/
	default:
		break;
	}

	return true;
}
#endif

PluginView *SpaInstrument::instantiateView(QWidget *parent)
{
	return new SpaView(this, parent);
}

void SpaInstrument::writeOsc(const char *dest, const char *args, va_list va)
{
	m_ports.rb->write(dest, args, va);
}

void SpaInstrument::writeOsc(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOsc(dest, args, va);
	va_end(va);
}

SpaView::SpaView(Instrument *_instrument, QWidget *_parent) :
	InstrumentView(_instrument, _parent)
{
	setAutoFillBackground(true);

	QGridLayout *l = new QGridLayout(this);

	m_toggleUIButton = new QPushButton(tr("Show GUI"), this);
	m_toggleUIButton->setCheckable(true);
	m_toggleUIButton->setChecked(false);
	m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
	m_toggleUIButton->setFont(pointSize<8>(m_toggleUIButton->font()));
	connect(m_toggleUIButton, SIGNAL(toggled(bool)), this,
		SLOT(toggleUI()));
	m_toggleUIButton->setWhatsThis(
		tr("Click here to show or hide the graphical user interface "
		   "(GUI) of Osc."));

	m_reloadPluginButton = new QPushButton(tr("Reload Plugin"), this);

	connect(m_reloadPluginButton, SIGNAL(toggled(bool)), this,
		SLOT(reloadPlugin()));

	l->addWidget(m_toggleUIButton, 0, 0);
	l->addWidget(m_reloadPluginButton, 0, 1);

	setAcceptDrops(true);
}

SpaView::~SpaView()
{
	SpaInstrument *model = castModel<SpaInstrument>();
	if (model && model->m_descriptor->ui_ext() && model->m_hasGUI)
	{
		qDebug() << "shutting down UI...";
		model->m_plugin->ui_ext_show(false);
	}
}

void SpaView::dragEnterEvent(QDragEnterEvent *_dee)
{
	void (QDragEnterEvent::*reaction)(void) = &QDragEnterEvent::ignore;

	if (_dee->mimeData()->hasFormat(StringPairDrag::mimeType()))
	{
		const QString txt =
			_dee->mimeData()->data(StringPairDrag::mimeType());
		if (txt.section(':', 0, 0) == "pluginpresetfile")
			reaction = &QDragEnterEvent::acceptProposedAction;
	}

	(_dee->*reaction)();
}

void SpaView::dropEvent(QDropEvent *_de)
{
	const QString type = StringPairDrag::decodeKey(_de);
	const QString value = StringPairDrag::decodeValue(_de);
	if (type == "pluginpresetfile")
	{
		castModel<SpaInstrument>()->loadFile(value);
		_de->accept();
		return;
	}
	_de->ignore();
}

void SpaView::modelChanged()
{
	SpaInstrument *m = castModel<SpaInstrument>();

	/*	// set models for controller knobs
		m_portamento->setModel( &m->m_portamentoModel ); */

	m_toggleUIButton->setChecked(m->m_hasGUI);
}

void SpaView::toggleUI()
{
	SpaInstrument *model = castModel<SpaInstrument>();
	if (model->m_descriptor->ui_ext() &&
		model->m_hasGUI != m_toggleUIButton->isChecked())
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();
		model->m_plugin->ui_ext_show(model->m_hasGUI);
		ControllerConnection::finalizeConnections();
	}
}

void SpaView::reloadPlugin()
{
	SpaInstrument *model = castModel<SpaInstrument>();
	model->reloadPlugin();
}

SpaInstrument::lmmsPorts::lmmsPorts(int bufferSize) :
	buffersize(bufferSize), m_lUnprocessed(bufferSize),
	m_rUnprocessed(bufferSize), m_lProcessed(bufferSize),
	m_rProcessed(bufferSize)
{
}
