/*
 * SpaControlBase.cpp - base class for spa instruments, effects, etc
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "SpaControlBase.h"

#ifdef LMMS_HAVE_SPA

#include <QDebug>
#include <QDir>
#include <QTemporaryFile>

#include <cassert>
#include <spa/spa.h>
#include <spa/audio.h>

#include "../include/RemotePlugin.h" // QSTR_TO_STDSTR
#include "AutomatableModel.h"
#include "Engine.h"
#include "Mixer.h"
#include "SpaManager.h"
#include "SpaOscModel.h"
#include "StringPairDrag.h" // DnD

SpaControlBase::SpaControlBase(Model* that, const QString& uniqueName,
								DataFile::Types settingsType) :
	m_spaDescriptor(Engine::getSPAManager()->getDescriptor(uniqueName)),
	m_that(that)
{
	if (m_spaDescriptor)
	{
			std::size_t procId = 0;
			int channelsLeft = DEFAULT_CHANNELS; // LMMS plugins are stereo
			while (channelsLeft > 0)
			{
					std::unique_ptr<SpaProc> newOne(
							new SpaProc(that, m_spaDescriptor, procId++,
										settingsType));
					if (newOne->isValid())
					{
						channelsLeft -= std::max(
								1 + static_cast<bool>(newOne->m_audioInCount),
								1 + static_cast<bool>(newOne->m_audioOutCount));
						Q_ASSERT(channelsLeft >= 0);
						if(newOne->netPort())
						{
							m_procsByPort.emplace(newOne->netPort(), newOne.get());
						}
						m_procs.push_back(std::move(newOne));
					}
					else
					{
							qCritical() << "Failed instantiating Spa processor";
							m_valid = false;
							channelsLeft = 0;
					}
			}
			if (m_valid)
			{
					m_channelsPerProc = DEFAULT_CHANNELS / m_procs.size();
					if (m_procs.size() > 1)
					{
							m_procs[0]->makeLinkingProc();
							createMultiChannelLinkModel();
					}

					// initially link all controls
					for (std::size_t i = 0; i < m_procs[0]->modelNum(); ++i)
					{
							linkModel(i, true);
					}
			}
	}
	else {
			qCritical() << "No SPA descriptor found for URI" << uniqueName;
			m_valid = false;
	}
	// TODO: error handling
}

void SpaControlBase::saveSettings(QDomDocument &doc, QDomElement &that)
{
	LinkedModelGroups::saveSettings(doc, that);

	if (m_procs.empty()) { /* don't even add a "states" node */ }
	else
	{
		QDomElement states = doc.createElement("states");
		that.appendChild(states);

		char chanName[] = "state0";
		for (std::size_t chanIdx = 0; chanIdx < m_procs.size(); ++chanIdx)
		{
			chanName[5] = '0' + static_cast<char>(chanIdx);
			QDomElement channel = doc.createElement(
									QString::fromUtf8(chanName));
			states.appendChild(channel);
			m_procs[chanIdx]->saveState(doc, channel);
		}
	}
}

void SpaControlBase::loadSettings(const QDomElement &that)
{
	LinkedModelGroups::loadSettings(that);

	QDomElement states = that.firstChildElement("states");
	if (!states.isNull() && (!m_procs.empty()))
	{
		QDomElement lastChan;
		char chanName[] = "state0";
		for (std::size_t chanIdx = 0; chanIdx < m_procs.size(); ++chanIdx)
		{
			chanName[5] = '0' + static_cast<char>(chanIdx);
			QDomElement chan = states.firstChildElement(chanName);
			if (!chan.isNull()) { lastChan = chan; }

			m_procs[chanIdx]->loadState(lastChan);
		}
	}

}

SpaControlBase::~SpaControlBase() {}

SpaProc::SpaProc(Model *parent, const spa::descriptor* desc, std::size_t curProc, DataFile::Types settingsType) :
	LinkedModelGroup(parent, curProc),
	m_spaDescriptor(desc),
	m_ports(Engine::mixer()->framesPerPeriod()),
	m_settingsType(settingsType)
{
	initPlugin();
}

SpaProc::~SpaProc() { shutdownPlugin(); }




void SpaProc::saveState(QDomDocument &doc, QDomElement &that)
{
	if (m_spaDescriptor->save_has())
	{
		QTemporaryFile tf;
		if (tf.open())
		{
			const std::string fn = QSTR_TO_STDSTR(
				QDir::toNativeSeparators(tf.fileName()));
//			m_pluginMutex.lock();
			m_plugin->save(fn.c_str(), ++m_saveTicket);
//			m_pluginMutex.unlock();

			while (!m_plugin->save_check(fn.c_str(), m_saveTicket)) {
				QThread::msleep(1);
			}

			QDomCDATASection cdata = doc.createCDATASection(
				QString::fromUtf8(tf.readAll()));
			that.appendChild(cdata);
		}
		tf.remove();
	}
}

void SpaProc::loadState(const QDomElement &that)
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
				loadFile(tf.fileName());
			}
		}
	}
}


void SpaProc::loadFile(const QString &file)
{
	const QByteArray fn = file.toUtf8();
//	m_pluginMutex.lock();
	m_plugin->load(fn.data(), ++m_saveTicket);
	while (!m_plugin->load_check(fn.data(), m_saveTicket)) {
		QThread::msleep(1);
	}
//	m_pluginMutex.unlock();
}

void SpaControlBase::loadFile(const QString &file)
{
	// for now, only support loading one proc into all proc (duplicating)
	for(std::unique_ptr<SpaProc>& proc : m_procs)
		proc->loadFile(file);
	setNameFromFile(QFileInfo(file).baseName().replace(
		QRegExp("^[0-9]{4}-"), QString()));
}

bool SpaControlBase::hasUi() const
{
	// do not support external UI for mono effects yet
	// (how would that look??)
	return m_procs.size() == 1 && m_spaDescriptor->ui_ext();
}

void SpaControlBase::uiExtShow(bool doShow)
{
	if(m_procs.size() == 1)
		m_procs[0]->uiExtShow(doShow);
}

void SpaProc::reloadPlugin()
{
	// refresh ports that are only read on restore
	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	int16_t fpp = Engine::mixer()->framesPerPeriod();
	assert(fpp >= 0);
	m_ports.buffersize = static_cast<unsigned>(fpp);

	if (m_spaDescriptor->restore_has())
	{
		// use the offered restore function
//		m_pluginMutex.lock();
		m_plugin->restore(++m_restoreTicket);
//		m_pluginMutex.unlock();

		while (!m_plugin->restore_check(m_restoreTicket)) {
			QThread::msleep(1);
		}
	}
	else
	{
		// save state of current plugin instance
		DataFile m(m_settingsType);
		saveState(m, m.content());

		shutdownPlugin();
		initPlugin(); // (will create a new instance)

		// and load the settings again
		loadState(m.content());
	}
}


void SpaProc::copyModelsToPorts()
{
	for (LmmsPorts::TypedPorts &tp : m_ports.m_userPorts)
	{
		switch (tp.m_type)
		{
		case 'f':
			tp.m_val.m_f = tp.m_connectedModel.m_floatModel->value();
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

void SpaProc::shutdownPlugin()
{
	m_plugin->deactivate();

	m_spaDescriptor->delete_plugin(m_plugin);
	m_plugin = nullptr;

	clearModels();
	// clear all port data (object is just raw memory after dtor call)...
	m_ports.~LmmsPorts();
	// ... so we can reuse it - C++ is just awesome
	new (&m_ports) LmmsPorts(Engine::mixer()->framesPerPeriod());
}

struct LmmsVisitor final : public virtual spa::audio::visitor
{
	SpaProc* proc;
	SpaProc::LmmsPorts *m_ports;
	QMap<QString, AutomatableModel *> *m_connectedModels;
	const char *m_curName;
	int m_audioInputs = 0; // out
	int m_audioOutputs = 0; // out
	using spa::audio::visitor::visit; // not sure if this is right, it fixes
					  // the -Woverloaded-virtual issues

	void visit(spa::audio::in &p) override
	{
		qDebug() << "in, c: " << +p.channel;
		++m_audioInputs;
		p.set_ref((p.channel == spa::audio::stereo::left)
				? m_ports->m_lUnprocessed.data()
				: m_ports->m_rUnprocessed.data());
	}
	void visit(spa::audio::out &p) override
	{
		qDebug() << "out, c: %d\n" << +p.channel;
		++m_audioOutputs;
		p.set_ref((p.channel == spa::audio::stereo::left)
				? m_ports->m_lProcessed.data()
				: m_ports->m_rProcessed.data());
	}
	void visit(spa::audio::stereo::in &p) override
	{
		qDebug() << "in, stereo";
		++++m_audioInputs;
		p.left = m_ports->m_lUnprocessed.data();
		p.right = m_ports->m_rUnprocessed.data();
	}
	void visit(spa::audio::stereo::out &p) override
	{
		qDebug() << "out, stereo";
		++++m_audioOutputs;
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
	void setupPort(
		spa::audio::control_in<BaseType> &port, BaseType &portData,
		ModelClass *&connectedModel,
		const ModelCtorArgs &... modelCtorArgs)
	{
		portData = port.def;
		port.set_ref(&portData);
		connectedModel = new ModelClass(static_cast<BaseType>(port),
			modelCtorArgs..., nullptr,
			QString::fromUtf8(m_curName));
		m_connectedModels->insert(
			QString::fromUtf8(m_curName), connectedModel);
		proc->addModel(connectedModel, m_curName);
	}

	// TODO: port_ref does not work yet (clang warnings), so we use
	// control_in
	void visit(spa::audio::control_in<float> &p) override
	{
		qDebug() << "other control port (float)";
		m_ports->m_userPorts.emplace_back('f');
		SpaProc::LmmsPorts::TypedPorts &bck = m_ports->m_userPorts.back();
		setupPort(p, bck.m_val.m_f, bck.m_connectedModel.m_floatModel,
			p.min, p.max, p.step);
	}
	void visit(spa::audio::control_in<int> &p) override
	{
		qDebug() << "other control port (int)";
		m_ports->m_userPorts.emplace_back('i');
		SpaProc::LmmsPorts::TypedPorts &bck = m_ports->m_userPorts.back();
		setupPort(p, bck.m_val.m_i, bck.m_connectedModel.m_intModel,
			p.min, p.max);
	}
	void visit(spa::audio::control_in<bool> &p) override
	{
		qDebug() << "other control port (bool)";
		m_ports->m_userPorts.emplace_back('b');
		SpaProc::LmmsPorts::TypedPorts &bck =
			m_ports->m_userPorts.back();
		setupPort(p, bck.m_val.m_b, bck.m_connectedModel.m_boolModel);
	}

	void visit(spa::audio::osc_ringbuffer_in &p) override
	{
		qDebug() << "ringbuffer input";
		if (m_ports->rb) {
			throw std::runtime_error("can not handle 2 OSC ports");
		}
		else
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

void SpaProc::initPlugin()
{
//	m_pluginMutex.lock();
	if (!m_spaDescriptor)
	{
//		m_pluginMutex.unlock();
		m_valid = false;
	}
	else
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
			m_valid = false;
		}
//		m_pluginMutex.unlock();
	}

	m_ports.samplerate = Engine::mixer()->processingSampleRate();
	spa::simple_vec<spa::simple_str> portNames =
		m_spaDescriptor->port_names();
	for (const spa::simple_str &portname : portNames)
	{
		try
		{
			// qDebug() << "portname: " << portname.data();
			spa::port_ref_base &port_ref = m_plugin->port(portname.data());

			LmmsVisitor v;
			v.proc = this;
			v.m_ports = &m_ports;
			v.m_connectedModels = &m_connectedModels;
			v.m_curName = portname.data();
			port_ref.accept(v);
			m_audioInCount += v.m_audioInputs;
			m_audioOutCount += v.m_audioOutputs;
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
			m_valid = false; // TODO: free plugin, handle etc...
			break;
		}
	}

	if(m_valid)
	{
		// all initial ports are already set, we can do
		// initialization of buffers etc.
		m_plugin->init();

		m_plugin->activate();

		// checks not yet implemented:
		//	spa::host_utils::check_ports(descriptor, plugin);
		//	plugin->test_more();
	}
}

void SpaProc::writeOsc(
	const char *dest, const char *args, va_list va)
{
	m_ports.rb->write(dest, args, va);
}

void SpaProc::writeOsc(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOsc(dest, args, va);
	va_end(va);
}

void SpaControlBase::writeOscToAll(
	const char *dest, const char *args, va_list va)
{
	for(std::unique_ptr<SpaProc>& proc : m_procs)
		proc->writeOsc(dest, args, va);
}

void SpaControlBase::writeOscToAll(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOscToAll(dest, args, va);
	va_end(va);
}

void SpaProc::run(unsigned frames)
{
	m_ports.samplecount = static_cast<unsigned>(frames);
	m_plugin->run();
}

unsigned SpaProc::netPort() const { return m_plugin->net_port(); }


namespace detail {

void copyBuffersFromCore(std::vector<float>& portBuf,
	const sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		portBuf[f] = lmmsBuf[f][channel];
	}
}




void addBuffersFromCore(std::vector<float>& portBuf, const sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		portBuf[f] = (portBuf[f] + lmmsBuf[f][channel]) / 2.0f;
	}
}




void copyBuffersToCore(const std::vector<float>& portBuf, sampleFrame *lmmsBuf,
	unsigned channel, fpp_t frames)
{
	for (std::size_t f = 0; f < static_cast<unsigned>(frames); ++f)
	{
		lmmsBuf[f][channel] = portBuf[f];
	}
}

} // namespace detail

void SpaProc::copyBuffersFromCore(const sampleFrame *buf,
									unsigned offset, unsigned num,
									fpp_t frames)
{
	detail::copyBuffersFromCore(m_ports.m_lUnprocessed, buf, offset, frames);
	if (num > 1)
	{
		// if the caller requests to take input from two channels, but we only
		// have one input channel... take medium of left and right for
		// mono input
		// (this happens if we have two outputs and only one input)
		if (m_ports.m_rUnprocessed.size())
			detail::copyBuffersFromCore(m_ports.m_rUnprocessed, buf, offset + 1, frames);
		else
			detail::addBuffersFromCore(m_ports.m_lUnprocessed, buf, offset + 1, frames);
	}
}




void SpaProc::copyBuffersToCore(sampleFrame* buf,
								unsigned offset, unsigned num,
								fpp_t frames) const
{
	detail::copyBuffersToCore(m_ports.m_lProcessed, buf, offset + 0, frames);
	if (num > 1)
	{
		// if the caller requests to copy into two channels, but we only have
		// one output channel, duplicate our output
		// (this happens if we have two inputs and only one output)
		const std::vector<float>& portBuf = m_ports.m_rProcessed.size()
				? m_ports.m_rProcessed : m_ports.m_lProcessed;
		detail::copyBuffersToCore(portBuf, buf, offset + 1, frames);
	}
}




void SpaProc::uiExtShow(bool doShow) { m_plugin->ui_ext_show(doShow); }




struct SpaOscModelFactory : public spa::audio::visitor
{
	SpaProc *m_plugRef;
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

	SpaOscModelFactory(SpaProc *ctrlBase, const QString &dest) :
		m_plugRef(ctrlBase), m_dest(dest)
	{
	}
};

AutomatableModel *SpaProc::modelAtPort(const QString &dest)
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

		if (spaMod) { addModel(mod = spaMod, url.path()); }
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

LinkedModelGroup *SpaControlBase::getGroup(std::size_t idx)
{
	return (idx < m_procs.size()) ? m_procs[idx].get() : nullptr;
}

const LinkedModelGroup *SpaControlBase::getGroup(std::size_t idx) const
{
	return (idx < m_procs.size()) ? m_procs[idx].get() : nullptr;
}

template <class UnsignedType> UnsignedType castToUnsigned(int val)
{
	return val >= 0 ? static_cast<unsigned>(val) : 0u;
}


SpaProc::LmmsPorts::LmmsPorts(int bufferSize) :
	buffersize(castToUnsigned<unsigned>(bufferSize)),
	m_lUnprocessed(castToUnsigned<std::size_t>(bufferSize)),
	m_rUnprocessed(castToUnsigned<std::size_t>(bufferSize)),
	m_lProcessed(castToUnsigned<std::size_t>(bufferSize)),
	m_rProcessed(castToUnsigned<std::size_t>(bufferSize))
{
}

void SpaControlBase::copyModelsFromLmms() {
	for (std::unique_ptr<SpaProc>& c : m_procs) { c->copyModelsToPorts(); }
}




void SpaControlBase::copyBuffersFromLmms(const sampleFrame *buf, fpp_t frames) {
	unsigned offset = 0;
	for (std::unique_ptr<SpaProc>& c : m_procs) {
		c->copyBuffersFromCore(buf, offset, m_channelsPerProc, frames);
		offset += m_channelsPerProc;
	}
}




void SpaControlBase::copyBuffersToLmms(sampleFrame *buf, fpp_t frames) const {
	unsigned offset = 0;
	for (const std::unique_ptr<SpaProc>& c : m_procs) {
		c->copyBuffersToCore(buf, offset, m_channelsPerProc, frames);
		offset += m_channelsPerProc;
	}
}




void SpaControlBase::run(unsigned frames) {
	for (std::unique_ptr<SpaProc>& c : m_procs) { c->run(frames); }
}

AutomatableModel *SpaControlBase::modelAtPort(const QString &dest)
{
	if (m_procs.size() == 1)
	{
		QUrl url(dest);
		return m_procsByPort[static_cast<unsigned>(url.port())]->
			modelAtPort(dest);
	}
	else { return nullptr; /* TODO */ }
}


#endif // LMMS_HAVE_SPA
