/*
 * SpaControlBase.cpp - base class for spa instruments, effects, etc
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

#define SPA_PLUGIN_USE_QLIBRARY

#ifdef SPA_PLUGIN_USE_QLIBRARY
#include <QLibrary>
#else
#include <dlfcn.h>
#endif

#include <cassert>
#include <spa/audio.h>

#include "../include/RemotePlugin.h" // QSTR_TO_STDSTR
#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "Mixer.h"
#include "SpaControlBase.h"
#include "SpaOscModel.h"
#include "StringPairDrag.h" // DnD
#include "embed.h"
#include "gui_templates.h"

SpaControlBase::SpaControlBase(const char *libraryName) :
	m_ports(Engine::mixer()->framesPerPeriod()), m_hasGUI(false),
	m_libraryName(libraryName)
{
	initPlugin();
}

SpaControlBase::~SpaControlBase() { shutdownPlugin(); }

void SpaControlBase::saveSettings(QDomDocument &doc, QDomElement &that)
{
	// save internal data?
	if (m_spaDescriptor->save_has())
	{
		QTemporaryFile tf;
		if (tf.open())
		{
			const std::string fn = QSTR_TO_STDSTR(
				QDir::toNativeSeparators(tf.fileName()));
			m_pluginMutex.lock();
			m_plugin->save(fn.c_str(), ++m_saveTicket);
			m_pluginMutex.unlock();

			while (!m_plugin->save_check(fn.c_str(), m_saveTicket)) {
				QThread::msleep(1);
}

			QDomCDATASection cdata = doc.createCDATASection(
				QString::fromUtf8(tf.readAll()));
			that.appendChild(cdata);
		}
		tf.remove();
	}

	// save connected models
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

void SpaControlBase::loadSettings(const QDomElement &that)
{
	if (!that.hasChildNodes())
	{
		return;
	}

	for (QDomNode node = that.firstChild(); !node.isNull();
		node = node.nextSibling())
	{
		QDomCDATASection cdata = node.toCDATASection();
		QDomElement elem;
		// load internal state?
		if (!cdata.isNull() && m_spaDescriptor->load_has())
		{
			QTemporaryFile tf;
			tf.setAutoRemove(false);
			if (tf.open())
			{
				tf.write(cdata.data().toUtf8());
				tf.flush();
				loadFileInternal(tf.fileName());
			}
		}
		// load connected models?
		else if (node.nodeName() == "connected-models" &&
			!(elem = node.toElement()).isNull())
		{
			QDomNamedNodeMap attrs = elem.attributes();

			auto do_load = [&](const QString &name,
					       QDomElement elem) {
				AutomatableModel *m = modelAtPort(name);
				// this will automatically
				// load any "connection" node:
				m->loadSettings(elem, name);
				m_connectedModels[name] = m;
			};

			for (int i = 0; i < attrs.count(); ++i)
			{
				QDomAttr attribute = attrs.item(i).toAttr();
				do_load(attribute.name(), elem);
			}

			for (QDomElement portnode = elem.firstChildElement();
				!portnode.isNull();
				portnode = portnode.nextSiblingElement())
			{
				if (portnode.nodeName() != "connection")
				{
					QString name = portnode.nodeName();
					if (name == "automatablemodel") {
						name = portnode.attribute(
							"nodename");
}
					do_load(name, elem);
				}
			}
		}
	}
}

void SpaControlBase::loadFileInternal(const QString &file)
{
	const QByteArray fn = file.toUtf8();
	m_pluginMutex.lock();
	m_plugin->load(fn.data(), ++m_saveTicket);
	while (!m_plugin->load_check(fn.data(), m_saveTicket)) {
		QThread::msleep(1);
}
	m_pluginMutex.unlock();
}

void SpaControlBase::loadFile(const QString &file)
{
	loadFileInternal(file);
	setNameFromFile(QFileInfo(file).baseName().replace(
		QRegExp("^[0-9]{4}-"), QString()));
}

void SpaControlBase::reloadPlugin()
{
	// refresh ports that are only read on restore
	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	int16_t fpp = Engine::mixer()->framesPerPeriod();
	assert(fpp >= 0);
	m_ports.buffersize = static_cast<unsigned>(fpp);

	if (m_spaDescriptor->restore_has())
	{
		// use the offered restore function
		m_pluginMutex.lock();
		m_plugin->restore(++m_restoreTicket);
		m_pluginMutex.unlock();

		while (!m_plugin->restore_check(m_restoreTicket)) {
			QThread::msleep(1);
}
	}
	else
	{
		// save state of current plugin instance
		DataFile m(settingsType());

		saveSettings(m, m.content());

		shutdownPlugin();
		// init plugin (will create a new instance)
		initPlugin();

		// and load the settings again
		loadSettings(m.content());
	}
}

void SpaControlBase::copyModelsToPorts()
{
	for (LmmsPorts::TypedPorts &tp : m_ports.m_otherPorts)
	{
		switch (tp.m_type)
		{
		case 'f':
			tp.m_val.m_f =
				tp.m_connectedModel.m_floatModel->value();
			break;
		case 'i':
			tp.m_val.m_i = tp.m_connectedModel.m_intModel->value();
			break;
		case 'b':
			tp.m_val.m_b = tp.m_connectedModel.m_boolModel->value();
			break;
		default:
			assert(false);
		}
	}
}

void SpaControlBase::shutdownPlugin()
{
	m_plugin->deactivate();

	delete m_plugin;
	m_plugin = nullptr;
	delete m_spaDescriptor;
	m_spaDescriptor = nullptr;

	m_pluginMutex.lock();
	if (m_lib)
	{
#ifdef SPA_PLUGIN_USE_QLIBRARY
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

QString SpaControlBase::nodeName() const
{
	return QString::fromStdString(spa::unique_name(*m_spaDescriptor));
}

struct LmmsVisitor final : public virtual spa::audio::visitor
{
	SpaControlBase::LmmsPorts *m_ports;
	QMap<QString, AutomatableModel *> *m_connectedModels;
	const char *m_curName;
	using spa::audio::visitor::visit; // not sure if this is right, it fixes
					  // the -Woverloaded-virtual issues

	void visit(spa::audio::in &p) override
	{
		qDebug() << "in, c: " << +p.channel;
		p.set_ref((p.channel == spa::audio::stereo::left)
				? m_ports->m_lUnprocessed.data()
				: m_ports->m_rUnprocessed.data());
	}
	void visit(spa::audio::out &p) override
	{
		qDebug() << "out, c: %d\n" << +p.channel;
		p.set_ref((p.channel == spa::audio::stereo::left)
				? m_ports->m_lProcessed.data()
				: m_ports->m_rProcessed.data());
	}
	void visit(spa::audio::stereo::in &p) override
	{
		qDebug() << "in, stereo";
		p.left = m_ports->m_lUnprocessed.data();
		p.right = m_ports->m_rUnprocessed.data();
	}
	void visit(spa::audio::stereo::out &p) override
	{
		qDebug() << "out, stereo";
		p.left = m_ports->m_lProcessed.data();
		p.right = m_ports->m_rProcessed.data();
	}
	void visit(spa::audio::buffersize &p) override
	{
		qDebug() << "buffersize";
		p.set_ref(&m_ports->buffersize);
	}
	void visit(spa::audio::samplerate &p) override
	{
		qDebug() << "samplerate";
		p.set_ref(&m_ports->samplerate);
	}
	void visit(spa::audio::samplecount &p) override
	{
		qDebug() << "samplecount";
		p.set_ref(&m_ports->samplecount);
	}

	template <class BaseType, class ModelClass, class... ModelCtorArgs>
	void setupPort(/*char letter,*/
		spa::audio::control_in<BaseType> &port, BaseType &portData,
		ModelClass *&connectedModel,
		const ModelCtorArgs &... modelCtorArgs)
	{
		//		m_ports->m_otherPorts.emplace_back(letter);
		//		SpaControlBase::LmmsPorts::TypedPorts& bck =
		//m_ports->m_otherPorts.back();

		portData = port.def;
		port.set_ref(&portData);
		connectedModel = new ModelClass(static_cast<BaseType>(port),
			modelCtorArgs..., nullptr,
			QString::fromUtf8(m_curName));
		m_connectedModels->insert(
			QString::fromUtf8(m_curName), connectedModel);
	}

	// TODO: port_ref does not work yet (clang warnings), so we use
	// control_in
	void visit(spa::audio::control_in<float> &p) override
	{
		qDebug() << "other control port (float)";
		m_ports->m_otherPorts.emplace_back('f');
		SpaControlBase::LmmsPorts::TypedPorts &bck =
			m_ports->m_otherPorts.back();
		setupPort(p, bck.m_val.m_f, bck.m_connectedModel.m_floatModel,
			p.min, p.max, p.step);

		bck.m_val.m_f = p.def;
		p.set_ref(&bck.m_val.m_f);
		bck.m_connectedModel.m_floatModel =
			new FloatModel(static_cast<float>(p), p.min, p.max,
				p.step, nullptr, QString::fromUtf8(m_curName));
		m_connectedModels->insert(QString::fromUtf8(m_curName),
			bck.m_connectedModel.m_floatModel);
	}
	void visit(spa::audio::control_in<int> &p) override
	{
		qDebug() << "other control port (int)";
		m_ports->m_otherPorts.emplace_back('i');
		SpaControlBase::LmmsPorts::TypedPorts &bck =
			m_ports->m_otherPorts.back();
		setupPort(p, bck.m_val.m_i, bck.m_connectedModel.m_intModel,
			p.min, p.max);
	}
	void visit(spa::audio::control_in<bool> &p) override
	{
		qDebug() << "other control port (bool)";
		m_ports->m_otherPorts.emplace_back('b');
		SpaControlBase::LmmsPorts::TypedPorts &bck =
			m_ports->m_otherPorts.back();
		setupPort(p, bck.m_val.m_b, bck.m_connectedModel.m_boolModel);
	}

	void visit(spa::audio::osc_ringbuffer_in &p) override
	{
		qDebug() << "ringbuffer input";
		if (m_ports->rb) {
			throw std::runtime_error("can not handle 2 OSC ports");
		} else
		{
			m_ports->rb.reset(
				new spa::audio::osc_ringbuffer(p.get_size()));
			p.connect(*m_ports->rb);
		}
	}
	void visit(spa::port_ref_base &) override
	{
		qDebug() << "port of unknown type";
	}
};

bool SpaControlBase::initPlugin()
{
	m_pluginMutex.lock();

	spa::descriptor_loader_t spaDescriptorLoader;
#ifdef SPA_PLUGIN_USE_QLIBRARY
	m_lib = new QLibrary(m_libraryName);
	m_lib->load();

	if (!m_lib->isLoaded()) {
		qDebug() << "Warning: Could not load library " << m_libraryName
			 << ": " << m_lib->errorString();
}

	spaDescriptorLoader = reinterpret_cast<spa::descriptor_loader_t>(
		m_lib->resolve(spa::descriptor_name));
#else
	lib = dlopen(libraryName.toAscii().data(), RTLD_LAZY | RTLD_LOCAL);
	if (!lib)
		qDebug() << "Warning: Could not load library " << libraryName
			 << ": " << dlerror();

	*(void **)(&spaDescriptorLoader) = dlsym(lib, spa::descriptor_name);
#endif

	if (!spaDescriptorLoader) {
		qDebug() << "Warning: Could not resolve \"osc_descriptor\" in "
			 << m_libraryName;
}

	if (spaDescriptorLoader)
	{
		m_spaDescriptor =
			(*spaDescriptorLoader)(0 /* = plugin number, TODO */);
		if (m_spaDescriptor)
		{
			try
			{
				spa::assert_versions_match(*m_spaDescriptor);
				m_plugin = m_spaDescriptor->instantiate();
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
				m_spaDescriptor = nullptr;
			}
		}
	}
	m_pluginMutex.unlock();

	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	spa::simple_vec<spa::simple_str> portNames =
		m_spaDescriptor->port_names();
	for (const spa::simple_str &portname : portNames)
	{
		try
		{
			// qDebug() << "portname: " << portname.data();
			spa::port_ref_base &port_ref =
				m_plugin->port(portname.data());

			LmmsVisitor v;
			v.m_ports = &m_ports;
			v.m_connectedModels = &m_connectedModels;
			v.m_curName = portname.data();
			port_ref.accept(v);
		}
		catch (spa::port_not_found &e)
		{
			if (e.portname) {
				qWarning() << "plugin specifies invalid port \""
					   << e.portname
					   << "\", but does not provide it";
			} else {
				qWarning() << "plugin specifies invalid port, "
					      "but does not provide it";
}
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

void SpaControlBase::writeOscInternal(
	const char *dest, const char *args, va_list va)
{
	m_ports.rb->write(dest, args, va);
}

void SpaControlBase::writeOscInternal(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOscInternal(dest, args, va);
	va_end(va);
}

struct SpaOscModelFactory : public spa::audio::visitor
{
	SpaPluginBase *m_plugRef;
	const QString m_dest;

public:
	AutomatableModel *m_res = nullptr;

	template <class ModelType, class ...MoreArgs>
	void make(MoreArgs... args)
	{
		m_res = new ModelType(m_plugRef, m_dest, args...);
	}

	template <class T> using CtlIn = spa::audio::control_in<T>;
	virtual void visit(CtlIn<float> &in)
	{
		make<FloatOscModel>(in.min, in.max, in.def, in.step);
	}
	virtual void visit(CtlIn<double> &in)
	{
		// LMMS has no double models, cast it all away
		make<FloatOscModel>(static_cast<float>(in.min),
			static_cast<float>(in.max),
			static_cast<float>(in.def),
			static_cast<float>(in.step));
	}
	virtual void visit(CtlIn<int> &in)
	{
		make<IntOscModel>(in.min, in.max, in.def);
	}
	virtual void visit(CtlIn<bool> &in)
	{
		make<BoolOscModel>(in.def);
	}

	SpaOscModelFactory(SpaControlBase *ctrlBase, const QString &dest) :
		m_plugRef(&ctrlBase->getPluginBase()), m_dest(dest)
	{
	}
};

AutomatableModel *SpaControlBase::modelAtPort(const QString &dest)
{
	QUrl url(dest);

	AutomatableModel *mod;
	auto itr2 = m_connectedModels.find(url.path());
	if (itr2 != m_connectedModels.end())
	{
		mod = *itr2;
	}
	else
	{
		AutomatableModel *spaMod;
		{
			SpaOscModelFactory vis(this, url.path());
			spa::port_ref_base &base =
				m_plugin->port(url.path().toUtf8().data());
			base.accept(vis);
			spaMod = vis.m_res;
		}

		if (spaMod)
		{
			m_connectedModels.insert(url.path(), spaMod);
			mod = spaMod;
		}
		else
		{
			qDebug() << "LMMS: Could not create model from "
				 << "OSC port (received port\"" << url.port()
				 << "\", path \"" << url.path() << "\")";
			mod = nullptr;
		}
	}
	return mod;
}

template <class UnsignedType> UnsignedType castToUnsigned(int val)
{
	return val >= 0 ? static_cast<unsigned>(val) : 0u;
}

SpaControlBase::LmmsPorts::LmmsPorts(int bufferSize) :
	buffersize(castToUnsigned<unsigned>(bufferSize)),
	m_lUnprocessed(castToUnsigned<std::size_t>(bufferSize)),
	m_rUnprocessed(castToUnsigned<std::size_t>(bufferSize)),
	m_lProcessed(castToUnsigned<std::size_t>(bufferSize)),
	m_rProcessed(castToUnsigned<std::size_t>(bufferSize))
{
}
