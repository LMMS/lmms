/*
 * Lv2Proc.cpp - Lv2 processor class
 *
 * Copyright (c) 2019-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include "Lv2Proc.h"

#ifdef LMMS_HAVE_LV2

#include <cmath>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/resize-port/resize-port.h>
#include <QDebug>
#include <QtGlobal>

#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "Engine.h"
#include "Lv2Features.h"
#include "Lv2Manager.h"
#include "Lv2Ports.h"
#include "Lv2Evbuf.h"
#include "MidiEventToByteSeq.h"
#include "Mixer.h"




// container for everything required to store MIDI events going to the plugin
struct MidiInputEvent
{
	MidiEvent ev;
	TimePos time;
	f_cnt_t offset;
};




Plugin::PluginTypes Lv2Proc::check(const LilvPlugin *plugin,
	std::vector<PluginIssue>& issues)
{
	unsigned maxPorts = lilv_plugin_get_num_ports(plugin);
	enum { inCount, outCount, maxCount };
	unsigned audioChannels[maxCount] = { 0, 0 }; // audio input and output count
	unsigned midiChannels[maxCount] = { 0, 0 }; // MIDI input and output count

	const char* pluginUri = lilv_node_as_uri(lilv_plugin_get_uri(plugin));
	//qDebug() << "Checking plugin" << pluginUri << "...";

	// TODO: manage a global blacklist outside of the code
	//       for now, this will help
	//       this is only a fix for the meantime
	const auto& pluginBlacklist = Lv2Manager::getPluginBlacklist();
	if (!Engine::ignorePluginBlacklist() &&
		pluginBlacklist.find(pluginUri) != pluginBlacklist.end())
	{
		issues.emplace_back(blacklisted);
	}

	for (unsigned portNum = 0; portNum < maxPorts; ++portNum)
	{
		Lv2Ports::Meta meta;
		// does all port checks:
		std::vector<PluginIssue> tmp = meta.get(plugin, portNum);
		std::move(tmp.begin(), tmp.end(), std::back_inserter(issues));

		bool portMustBeUsed =
			!portIsSideChain(plugin,
							lilv_plugin_get_port_by_index(plugin, portNum)) &&
			!meta.m_optional;
		if (meta.m_type == Lv2Ports::Type::Audio && portMustBeUsed)
		{
			++audioChannels[meta.m_flow == Lv2Ports::Flow::Output
				? outCount : inCount];
		}
		else if(meta.m_type == Lv2Ports::Type::AtomSeq && portMustBeUsed)
		{
			++midiChannels[meta.m_flow == Lv2Ports::Flow::Output
				? outCount : inCount];
		}
	}

	if (audioChannels[inCount] > 2)
		issues.emplace_back(tooManyInputChannels,
			std::to_string(audioChannels[inCount]));
	if (audioChannels[outCount] == 0)
		issues.emplace_back(noOutputChannel);
	else if (audioChannels[outCount] > 2)
		issues.emplace_back(tooManyOutputChannels,
			std::to_string(audioChannels[outCount]));

	if (midiChannels[inCount] > 1)
		issues.emplace_back(tooManyMidiInputChannels,
			std::to_string(midiChannels[inCount]));
	if (midiChannels[outCount] > 1)
		issues.emplace_back(tooManyMidiOutputChannels,
			std::to_string(midiChannels[outCount]));

	AutoLilvNodes reqFeats(lilv_plugin_get_required_features(plugin));
	LILV_FOREACH (nodes, itr, reqFeats.get())
	{
		const char* reqFeatName = lilv_node_as_string(
								lilv_nodes_get(reqFeats.get(), itr));
		if(!Lv2Features::isFeatureSupported(reqFeatName))
		{
			issues.emplace_back(featureNotSupported, reqFeatName);
		}
	}

	return (audioChannels[inCount] > 2 || audioChannels[outCount] > 2)
		? Plugin::Undefined
		: (audioChannels[inCount] > 0)
			? Plugin::Effect
			: Plugin::Instrument;
}




Lv2Proc::Lv2Proc(const LilvPlugin *plugin, Model* parent) :
	LinkedModelGroup(parent),
	m_plugin(plugin),
	m_midiInputBuf(m_maxMidiInputEvents),
	m_midiInputReader(m_midiInputBuf)
{
	initPlugin();
}




Lv2Proc::~Lv2Proc() { shutdownPlugin(); }




void Lv2Proc::dumpPorts()
{
	std::size_t num = 0;
	for (const std::unique_ptr<Lv2Ports::PortBase>& port: m_ports)
	{
		(void)port;
		dumpPort(num++);
	}
}




void Lv2Proc::copyModelsFromCore()
{
	struct FloatFromModelVisitor : public ConstModelVisitor
	{
		const std::vector<float>* m_scalePointMap; // in
		float m_res; // out
		void visit(const FloatModel& m) override { m_res = m.value(); }
		void visit(const IntModel& m) override {
			m_res = static_cast<float>(m.value()); }
		void visit(const BoolModel& m) override {
			m_res = static_cast<float>(m.value()); }
		void visit(const ComboBoxModel& m) override {
			m_res = (*m_scalePointMap)[static_cast<std::size_t>(m.value())]; }
	};

	struct Copy : public Lv2Ports::Visitor
	{
		void visit(Lv2Ports::Control& ctrl) override
		{
			FloatFromModelVisitor ffm;
			ffm.m_scalePointMap = &ctrl.m_scalePointMap;
			ctrl.m_connectedModel->accept(ffm);
			ctrl.m_val = ffm.m_res;
		}
		void visit(Lv2Ports::Cv& cv) override
		{
			FloatFromModelVisitor ffm;
			ffm.m_scalePointMap = &cv.m_scalePointMap;
			cv.m_connectedModel->accept(ffm);
			// dirty fix, needs better interpolation
			std::fill(cv.m_buffer.begin(), cv.m_buffer.end(), ffm.m_res);
		}
		void visit(Lv2Ports::AtomSeq& atomPort) override
		{
			lv2_evbuf_reset(atomPort.m_buf.get(), true);
		}
	} copy;

	// feed each input port with the respective data from the LMMS core
	for (const std::unique_ptr<Lv2Ports::PortBase>& port : m_ports)
	{
		if (port->m_flow == Lv2Ports::Flow::Input)
		{
			port->accept(copy);
		}
	}

	// send pending MIDI events to atom port
	if(m_midiIn)
	{
		LV2_Evbuf_Iterator iter = lv2_evbuf_begin(m_midiIn->m_buf.get());
		// MIDI events waiting to go to the plugin?
		while(m_midiInputReader.read_space() > 0)
		{
			const MidiInputEvent ev = m_midiInputReader.read(1)[0];
			uint32_t atomStamp =
				ev.time.frames(Engine::framesPerTick()) + ev.offset;
			uint32_t type = Engine::getLv2Manager()->
				uridCache()[Lv2UridCache::Id::midi_MidiEvent];
			uint8_t buf[4];
			std::size_t bufsize = writeToByteSeq(ev.ev, buf, sizeof(buf));
			if(bufsize)
			{
				lv2_evbuf_write(&iter, atomStamp, type, bufsize, buf);
			}
		}
	}
}




void Lv2Proc::copyModelsToCore()
{
	struct Copy : public Lv2Ports::Visitor
	{
		void visit(Lv2Ports::AtomSeq& atomPort) override
		{
			// we currently don't copy anything, but we need to clear the buffer
			// for the plugin to write again
			lv2_evbuf_reset(atomPort.m_buf.get(), false);
		}
	} copy;

	// fetch data from each output port and bring it to the LMMS core
	for (const std::unique_ptr<Lv2Ports::PortBase>& port : m_ports)
	{
		if (port->m_flow == Lv2Ports::Flow::Output)
		{
			port->accept(copy);
		}
	}
}




void Lv2Proc::copyBuffersFromCore(const sampleFrame *buf,
									unsigned firstChan, unsigned num,
									fpp_t frames)
{
	inPorts().m_left->copyBuffersFromCore(buf, firstChan, frames);
	if (num > 1)
	{
		// if the caller requests to take input from two channels, but we only
		// have one input channel... take medium of left and right for
		// mono input
		// (this happens if we have two outputs and only one input)
		if (inPorts().m_right)
		{
			inPorts().m_right->copyBuffersFromCore(buf, firstChan + 1, frames);
		}
		else
		{
			inPorts().m_left->averageWithBuffersFromCore(buf, firstChan + 1, frames);
		}
	}
}




void Lv2Proc::copyBuffersToCore(sampleFrame* buf,
								unsigned firstChan, unsigned num,
								fpp_t frames) const
{
	outPorts().m_left->copyBuffersToCore(buf, firstChan + 0, frames);
	if (num > 1)
	{
		// if the caller requests to copy into two channels, but we only have
		// one output channel, duplicate our output
		// (this happens if we have two inputs and only one output)
		Lv2Ports::Audio* ap = outPorts().m_right
			? outPorts().m_right : outPorts().m_left;
		ap->copyBuffersToCore(buf, firstChan + 1, frames);
	}
}




void Lv2Proc::run(fpp_t frames)
{
	lilv_instance_run(m_instance, static_cast<uint32_t>(frames));
}




// in case there will be a PR which removes this callback and instead adds a
// `ringbuffer_t<MidiEvent + time info>` to `class Instrument`, this
// function (and the ringbuffer and its reader in `Lv2Proc`) will simply vanish
void Lv2Proc::handleMidiInputEvent(const MidiEvent &event, const TimePos &time, f_cnt_t offset)
{
	if(m_midiIn)
	{
		// ringbuffer allows only one writer at a time
		// however, this function can be called by multiple threads
		// (different RT and non-RT!) at the same time
		// for now, a spinlock looks like the most safe/easy compromise

		// source: https://en.cppreference.com/w/cpp/atomic/atomic_flag
		while (m_ringLock.test_and_set(std::memory_order_acquire))  // acquire lock
			 ; // spin

		MidiInputEvent ev { event, time, offset };
		std::size_t written = m_midiInputBuf.write(&ev, 1);
		if(written != 1)
		{
			qWarning("MIDI ringbuffer is too small! Discarding MIDI event.");
		}

		m_ringLock.clear(std::memory_order_release);
	}
	else
	{
		qWarning() << "Warning: Caught MIDI event for an Lv2 instrument"
					<< "that can not hande MIDI... Ignoring";
	}
}




AutomatableModel *Lv2Proc::modelAtPort(const QString &uri)
{
	// unused currently
	AutomatableModel *mod;
	auto itr = m_connectedModels.find(uri.toUtf8().data());
	if (itr != m_connectedModels.end()) { mod = itr->second; }
	else { mod = nullptr; }
	return mod;
}




void Lv2Proc::initPlugin()
{
	m_features.initCommon();
	initPluginSpecificFeatures();
	m_features.createFeatureVectors();

	createPorts();

	m_instance = lilv_plugin_instantiate(m_plugin,
		Engine::mixer()->processingSampleRate(),
		m_features.featurePointers());

	if (m_instance)
	{
		for (std::size_t portNum = 0; portNum < m_ports.size(); ++portNum)
			connectPort(portNum);
		lilv_instance_activate(m_instance);
	}
	else
	{
		qCritical() << "Failed to create an instance of"
			<< qStringFromPluginNode(m_plugin, lilv_plugin_get_name)
			<< "(URI:"
			<< lilv_node_as_uri(lilv_plugin_get_uri(m_plugin))
			<< ")";
		m_valid = false;
	}
}




void Lv2Proc::shutdownPlugin()
{
	if (m_valid)
	{
		lilv_instance_deactivate(m_instance);
		lilv_instance_free(m_instance);
		m_instance = nullptr;
	}
}




bool Lv2Proc::hasNoteInput() const
{
	return m_midiIn;
	// we could additionally check for
	// http://lv2plug.in/ns/lv2core#InstrumentPlugin
	// however, jalv does not do that, too
	// so, if there's any MIDI input, we just assume we can send notes there
}




void Lv2Proc::initPluginSpecificFeatures()
{
	// nothing yet
	// it would look like this:
	// m_features[LV2_URID__map] = m_uridMapFeature
}




void Lv2Proc::loadFileInternal(const QString &file)
{
	(void)file;
}




void Lv2Proc::createPort(std::size_t portNum)
{
	Lv2Ports::Meta meta;
	meta.get(m_plugin, portNum);

	const LilvPort* lilvPort = lilv_plugin_get_port_by_index(m_plugin,
								static_cast<uint32_t>(portNum));
	Lv2Ports::PortBase* port;

	switch (meta.m_type)
	{
		case Lv2Ports::Type::Control:
		{
			Lv2Ports::Control* ctrl = new Lv2Ports::Control;
			if (meta.m_flow == Lv2Ports::Flow::Input)
			{
				AutoLilvNode node(lilv_port_get_name(m_plugin, lilvPort));
				QString dispName = lilv_node_as_string(node.get());
				sample_rate_t sr = Engine::mixer()->processingSampleRate();
				if(meta.def() < meta.min(sr) || meta.def() > meta.max(sr))
				{
					qWarning()	<< "Warning: Plugin"
								<< qStringFromPluginNode(m_plugin, lilv_plugin_get_name)
								<< "(URI:"
								<< lilv_node_as_uri(lilv_plugin_get_uri(m_plugin))
								<< ") has a default value for port"
								<< dispName
								<< "which is not in range [min, max].";
				}
				switch (meta.m_vis)
				{
					case Lv2Ports::Vis::None:
					{
						// allow ~1000 steps
						float stepSize = (meta.max(sr) - meta.min(sr)) / 1000.0f;

						// make multiples of 0.01 (or 0.1 for larger values)
						float minStep = (stepSize >= 1.0f) ? 0.1f : 0.01f;
						stepSize -= fmodf(stepSize, minStep);
						stepSize = std::max(stepSize, minStep);

						ctrl->m_connectedModel.reset(
							new FloatModel(meta.def(), meta.min(sr), meta.max(sr),
											stepSize, nullptr, dispName));
						break;
					}
					case Lv2Ports::Vis::Integer:
						ctrl->m_connectedModel.reset(
							new IntModel(static_cast<int>(meta.def()),
											static_cast<int>(meta.min(sr)),
											static_cast<int>(meta.max(sr)),
											nullptr, dispName));
						break;
					case Lv2Ports::Vis::Enumeration:
					{
						ComboBoxModel* comboModel
							= new ComboBoxModel(
								nullptr, dispName);
						LilvScalePoints* sps =
							lilv_port_get_scale_points(m_plugin, lilvPort);
						LILV_FOREACH(scale_points, i, sps)
						{
							const LilvScalePoint* sp = lilv_scale_points_get(sps, i);
							ctrl->m_scalePointMap.push_back(lilv_node_as_float(
											lilv_scale_point_get_value(sp)));
							comboModel->addItem(
								lilv_node_as_string(
									lilv_scale_point_get_label(sp)));
						}
						lilv_scale_points_free(sps);
						ctrl->m_connectedModel.reset(comboModel);
						// TODO: use default value on comboModel, too?
						break;
					}
					case Lv2Ports::Vis::Toggled:
						ctrl->m_connectedModel.reset(
							new BoolModel(static_cast<bool>(meta.def()),
											nullptr, dispName));
						break;
				}
				if(meta.m_logarithmic)
				{
					ctrl->m_connectedModel->setScaleLogarithmic();
				}

			} // if m_flow == Input
			port = ctrl;
			break;
		}
		case Lv2Ports::Type::Audio:
		{
			Lv2Ports::Audio* audio =
				new Lv2Ports::Audio(
						static_cast<std::size_t>(
							Engine::mixer()->framesPerPeriod()),
						portIsSideChain(m_plugin, lilvPort)
					);
			port = audio;
			break;
		}
		case Lv2Ports::Type::AtomSeq:
		{
			Lv2Ports::AtomSeq* atomPort = new Lv2Ports::AtomSeq;

			{
				AutoLilvNode uriAtomSupports(Engine::getLv2Manager()->uri(LV2_ATOM__supports));
				AutoLilvNodes atomSupports(lilv_port_get_value(m_plugin, lilvPort, uriAtomSupports.get()));
				AutoLilvNode uriMidiEvent(Engine::getLv2Manager()->uri(LV2_MIDI__MidiEvent));

				LILV_FOREACH (nodes, itr, atomSupports.get())
				{
					if(lilv_node_equals(lilv_nodes_get(atomSupports.get(), itr), uriMidiEvent.get()))
					{
						atomPort->flags |= Lv2Ports::AtomSeq::FlagType::Midi;
					}
				}
			}

			int minimumSize = minimumEvbufSize();

			Lv2Manager* mgr = Engine::getLv2Manager();

			// check for alternative minimum size
			{
				AutoLilvNode rszMinimumSize = mgr->uri(LV2_RESIZE_PORT__minimumSize);
				AutoLilvNodes minSizeV(lilv_port_get_value(m_plugin, lilvPort, rszMinimumSize.get()));
				LilvNode* minSize = minSizeV ? lilv_nodes_get_first(minSizeV.get()) : nullptr;
				if (minSize && lilv_node_is_int(minSize)) {
					minimumSize = std::max(minimumSize, lilv_node_as_int(minSize));
				}
			}

			atomPort->m_buf.reset(
				lv2_evbuf_new(static_cast<uint32_t>(minimumSize),
								mgr->uridMap().map(LV2_ATOM__Chunk),
								mgr->uridMap().map(LV2_ATOM__Sequence)));

			port = atomPort;
			break;
		}
		default:
			port = new Lv2Ports::Unknown;
	}

	// `meta` is of class `Lv2Ports::Meta` and `port` is of a child class
	// we can now assign the `Lv2Ports::Meta` part of meta to ports, leaving
	// the additional members of `port` unchanged
	*static_cast<Lv2Ports::Meta*>(port) = meta;
	port->m_port = lilvPort;
	port->m_plugin = m_plugin;

	m_ports[portNum].reset(port);
}




void Lv2Proc::createPorts()
{
	// register ports at the processor after creation,
	// i.e. link their data or count them
	struct RegisterPort : public Lv2Ports::Visitor
	{
		Lv2Proc* m_proc;

		void visit(Lv2Ports::Control& ctrl) override
		{
			if (ctrl.m_flow == Lv2Ports::Flow::Input)
			{
				AutomatableModel* amo = ctrl.m_connectedModel.get();
				m_proc->m_connectedModels.emplace(
					lilv_node_as_string(lilv_port_get_symbol(
										m_proc->m_plugin, ctrl.m_port)),
					amo);
				m_proc->addModel(amo, ctrl.uri());
			}
		}

		void visit(Lv2Ports::Audio& audio) override
		{
			if (audio.mustBeUsed())
			{
				StereoPortRef dummy;
				StereoPortRef* portRef = &dummy;
				switch (audio.m_flow)
				{
					case Lv2Ports::Flow::Input:
						portRef = &m_proc->m_inPorts;
						break;
					case Lv2Ports::Flow::Output:
						portRef = &m_proc->m_outPorts;
						break;
					case Lv2Ports::Flow::Unknown:
						break;
				}
				// in Lv2, leftPort is defined to be the first port
				if (!portRef->m_left) { portRef->m_left = &audio; }
				else if (!portRef->m_right) { portRef->m_right = &audio; }
			}
		}

		void visit(Lv2Ports::AtomSeq& atomPort) override
		{
			if(atomPort.m_flow == Lv2Ports::Flow::Input)
			{
				if(atomPort.flags & Lv2Ports::AtomSeq::FlagType::Midi)
				{
					// take any MIDI input, prefer mandatory MIDI input
					// (Lv2Proc::check() assures there are <=1 mandatory MIDI
					// input ports)
					if(!m_proc->m_midiIn || !atomPort.m_optional)
						m_proc->m_midiIn = &atomPort;
				}
			}
			else if(atomPort.m_flow == Lv2Ports::Flow::Output)
			{
				if(atomPort.flags & Lv2Ports::AtomSeq::FlagType::Midi)
				{
					// take any MIDI output, prefer mandatory MIDI output
					// (Lv2Proc::check() assures there are <=1 mandatory MIDI
					// output ports)
					if(!m_proc->m_midiOut || !atomPort.m_optional)
						m_proc->m_midiOut = &atomPort;
				}
			}
			else { Q_ASSERT(false); }
		}
	};

	std::size_t maxPorts = lilv_plugin_get_num_ports(m_plugin);
	m_ports.resize(maxPorts);

	for (std::size_t portNum = 0; portNum < maxPorts; ++portNum)
	{
		createPort(portNum);
		RegisterPort registerPort;
		registerPort.m_proc = this;
		m_ports[portNum]->accept(registerPort);
	}

	// initially assign model values to port values
	copyModelsFromCore();

	// debugging:
	//dumpPorts();
}




struct ConnectPortVisitor : public Lv2Ports::Visitor
{
	std::size_t m_num;
	LilvInstance* m_instance;
	void connectPort(void* location)
	{
		lilv_instance_connect_port(m_instance,
			static_cast<uint32_t>(m_num), location);
	}
	void visit(Lv2Ports::AtomSeq& atomSeq) override
	{
		connectPort(lv2_evbuf_get_buffer(atomSeq.m_buf.get()));
	}
	void visit(Lv2Ports::Control& ctrl) override { connectPort(&ctrl.m_val); }
	void visit(Lv2Ports::Audio& audio) override
	{
		connectPort((audio.mustBeUsed()) ? audio.m_buffer.data() : nullptr);
	}
	void visit(Lv2Ports::Unknown&) override { connectPort(nullptr); }
	~ConnectPortVisitor() override;
};

ConnectPortVisitor::~ConnectPortVisitor() {}

// !This function must be realtime safe!
// use createPort to create any port before connecting
void Lv2Proc::connectPort(std::size_t num)
{
	ConnectPortVisitor connect;
	connect.m_num = num;
	connect.m_instance = m_instance;
	m_ports[num]->accept(connect);
}




void Lv2Proc::dumpPort(std::size_t num)
{
	struct DumpPortDetail : public Lv2Ports::ConstVisitor
	{
		void visit(const Lv2Ports::Control& ctrl) override {
			qDebug() << "  control port";
			// output ports may be uninitialized yet, only print inputs
			if (ctrl.m_flow == Lv2Ports::Flow::Input)
			{
				qDebug() << "    value:" << ctrl.m_val;
			}
		}
		void visit(const Lv2Ports::Audio& audio) override {
			qDebug() << (audio.isSideChain()	? "  audio port (sidechain)"
												: "  audio port");
			qDebug() << "    buffer size:" << audio.bufferSize();
		}
	};

	const Lv2Ports::PortBase& port = *m_ports[num];
	qDebug().nospace() << "port " << num << ":";
	qDebug() << "  name:"
		<< qStringFromPortName(m_plugin, port.m_port);
	qDebug() << "  flow: " << Lv2Ports::toStr(port.m_flow);
	qDebug() << "  type: " << Lv2Ports::toStr(port.m_type);
	qDebug() << "  visualization: " << Lv2Ports::toStr(port.m_vis);
	if (port.m_type == Lv2Ports::Type::Control || port.m_type == Lv2Ports::Type::Cv)
	{
		sample_rate_t sr = Engine::mixer()->processingSampleRate();
		qDebug() << "  default:" << port.def();
		qDebug() << "  min:" << port.min(sr);
		qDebug() << "  max:" << port.max(sr);
	}
	qDebug() << "  optional: " << port.m_optional;
	qDebug() << "  => USED: " << port.m_used;

	DumpPortDetail dumper;
	port.accept(dumper);
}




bool Lv2Proc::portIsSideChain(const LilvPlugin *plugin, const LilvPort *port)
{
	return	lilv_port_has_property(plugin, port,
									uri(LV2_CORE_PREFIX "isSidechain").get());
}




bool Lv2Proc::portIsOptional(const LilvPlugin *plugin, const LilvPort *port)
{
	return	lilv_port_has_property(plugin, port,
									uri(LV2_CORE__connectionOptional).get());
}




AutoLilvNode Lv2Proc::uri(const char *uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}


#endif // LMMS_HAVE_LV2
