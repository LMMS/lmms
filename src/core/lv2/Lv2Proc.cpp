/*
 * Lv2Proc.cpp - Lv2 processor class
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "Engine.h"
#include "Lv2Manager.h"
#include "Lv2Ports.h"
#include "Mixer.h"




Plugin::PluginTypes Lv2Proc::check(const LilvPlugin *plugin,
	std::vector<PluginIssue>& issues, bool printIssues)
{
	unsigned maxPorts = lilv_plugin_get_num_ports(plugin);
	enum { checkIn, checkOut, checkMax };
	unsigned audioChannels[checkMax] = { 0, 0 }; // input and output count

	for(unsigned portNum = 0; portNum < maxPorts; ++portNum)
	{
		Lv2Ports::Meta meta;
		// does all port checks:
		std::vector<PluginIssue> tmp = meta.get(plugin, portNum);
		std::move(tmp.begin(), tmp.end(), std::back_inserter(issues));

		if(meta.m_type == Lv2Ports::Type::Audio &&
			!portIsSideChain(plugin,
							lilv_plugin_get_port_by_index(plugin, portNum)))
			++audioChannels[meta.m_flow == Lv2Ports::Flow::Output];
	}

	if(audioChannels[checkIn] > 2)
		issues.emplace_back(tooManyInputChannels,
			std::to_string(audioChannels[0]));
	if(audioChannels[checkOut] == 0)
		issues.emplace_back(noOutputChannel);
	else if(audioChannels[checkOut] > 2)
		issues.emplace_back(tooManyOutputChannels,
			std::to_string(audioChannels[1]));

	const LilvNodes* reqFeats = lilv_plugin_get_required_features(plugin);
	LILV_FOREACH(nodes, itr, reqFeats)
	{
		issues.emplace_back(featureNotSupported,
			lilv_node_as_string(lilv_nodes_get(reqFeats, itr)));
	}

	if(printIssues && issues.size())
	{
		qDebug() << "Lv2 plugin " <<
			lilv_node_as_string(lilv_plugin_get_name(plugin))
			<< " can not be loaded:";
		for(const PluginIssue& iss : issues) { qDebug() << "  - " << iss; }
	}

	return (audioChannels[1] > 2 || audioChannels[0] > 2)
		? Plugin::Undefined
		: audioChannels[0]
			? Plugin::Effect
			: Plugin::Instrument;
}




Lv2Proc::Lv2Proc(const LilvPlugin *plugin, Model* parent) :
	LinkedModelGroup(parent),
	m_plugin(plugin)
{
	if(m_plugin)
	{
		initPlugin();
	}
	else
	{
		qCritical() << ":-( ! No descriptor found for" << plugin;
		m_valid = false;
	}
}




Lv2Proc::~Lv2Proc() { shutdownPlugin(); }




void Lv2Proc::dumpPorts()
{
	std::size_t num = 0;
	for(const Lv2Ports::PortBase* port: m_ports)
	{
		(void)port;
		dumpPort(num++);
	}
}




void Lv2Proc::copyModelsFromCore()
{
	struct FloatFromModel : public ConstModelVisitor
	{
		const std::vector<float>* m_scalePointMap; // in
		float m_res; // out
		void visit(const FloatModel& m) override { m_res = m.value(); }
		void visit(const IntModel& m) override {
			m_res = static_cast<float>(m.value()); }
		void visit(const BoolModel& m) override {
			m_res = static_cast<bool>(m.value()); }
		void visit(const ComboBoxModel& m) override {
			m_res = (*m_scalePointMap)[static_cast<std::size_t>(m.value())]; }
	};

	struct Copy : public Lv2Ports::Visitor
	{
		void visit(Lv2Ports::Control& ctrl) override
		{
			if(ctrl.m_flow == Lv2Ports::Flow::Input)
			{
				FloatFromModel ffm;
				ffm.m_scalePointMap = &ctrl.m_scalePointMap;
				ctrl.m_connectedModel->accept(ffm);
				ctrl.m_val = ffm.m_res;
			}
		}
		void visit(Lv2Ports::Cv& cv) override
		{
			if(cv.m_flow == Lv2Ports::Flow::Input)
			{
				FloatFromModel ffm;
				ffm.m_scalePointMap = &cv.m_scalePointMap;
				cv.m_connectedModel->accept(ffm);
				// dirty fix, needs better interpolation
				std::fill(cv.m_buffer.begin(), cv.m_buffer.end(), ffm.m_res);
			}
		}
	} copy;

	for (Lv2Ports::PortBase* port : m_ports)
	{
		port->accept(copy);
	}
}




void Lv2Proc::copyBuffersFromCore(const sampleFrame *buf,
									unsigned offset, unsigned num,
									fpp_t frames)
{
	inPorts().m_left->copyBuffersFromCore(buf, offset, frames);
	if(num > 1)
	{
		// if the caller requests to take input from two channels, but we only
		// have one input channel... take medium of left and right for
		// mono input
		// (this happens if we have two outputs and only one input)
		if(inPorts().m_right)
			inPorts().m_right->copyBuffersFromCore(buf, offset + 1, frames);
		else
			inPorts().m_left->addBuffersFromCore(buf, offset + 1, frames);
	}
}




void Lv2Proc::copyBuffersToCore(sampleFrame* buf,
								unsigned offset, unsigned num,
								fpp_t frames) const
{
	outPorts().m_left->copyBuffersToCore(buf, offset + 0, frames);
	if(num > 1)
	{
		// if the caller requests to copy into two channels, but we only have
		// one output channel, duplicate our output
		// (this happens if we have two inputs and only one output)
		Lv2Ports::Audio* ap = outPorts().m_right
			? outPorts().m_right : outPorts().m_left;
		ap->copyBuffersToCore(buf, offset + 1, frames);
	}
}




void Lv2Proc::run(unsigned frames)
{
	lilv_instance_run(m_instance, static_cast<unsigned>(frames));
}




AutomatableModel *Lv2Proc::modelAtPort(const QString &uri)
{
	// unused currently
	AutomatableModel *mod;
	auto itr = m_connectedModels.find(uri.toUtf8().data());
	if (itr != m_connectedModels.end())
	{
		mod = itr->second;
	}
	else
	{
#if 0
		AutomatableModel *lv2Mod;
		{
			Lv2OscModelFactory vis(this, url.path());
			lv2::port_ref_base &base =
				m_plugin->port(url.path().toUtf8().data());
			base.accept(vis);
			lv2Mod = vis.m_res;
		}

		if (lv2Mod)
		{
			m_connectedModels.insert(url.path(), lv2Mod);
			mod = lv2Mod;
		}
		else
		{
#endif
			qDebug() << "LMMS: Could not create model from "
				 << "OSC port (received port\"" << uri
				 << "\", path \"" << uri << "\")";
			mod = nullptr;
#if 0
		}
#endif
	}
	return mod;
}




void Lv2Proc::initPlugin()
{
	createPorts();

	m_instance = lilv_plugin_instantiate(m_plugin,
		Engine::mixer()->processingSampleRate(),
		nullptr);

	if(m_instance)
	{
		for(unsigned portNum = 0; portNum < m_ports.size(); ++portNum)
			connectPort(portNum);
		lilv_instance_activate(m_instance);
	}
	else
	{
		qCritical() << "Failed to create an instance of"
			<< lilv_node_as_string(lilv_plugin_get_name(m_plugin));
		m_valid = false;
	}
}




void Lv2Proc::shutdownPlugin()
{
	lilv_instance_deactivate(m_instance);
	m_instance = nullptr;
}




void Lv2Proc::loadFileInternal(const QString &file)
{
#ifdef TODO
	const QByteArray fn = file.toUtf8();
	m_plugin->load(fn.data(), ++m_saveTicket);
	while (!m_plugin->load_check(fn.data(), m_saveTicket)) {
		QThread::msleep(1);
	}
#else
	(void)file;
#endif
}




void Lv2Proc::createPort(unsigned portNum)
{
	Lv2Ports::PortBase*& port = m_ports[portNum];

	Lv2Ports::Meta meta;
	meta.get(m_plugin, portNum);

	const LilvPort* lilvPort = lilv_plugin_get_port_by_index(m_plugin,
								portNum);

	if (meta.m_type == Lv2Ports::Type::Control)
	{
		Lv2Ports::Control* ctrl = new Lv2Ports::Control;
		if(meta.m_flow == Lv2Ports::Flow::Input)
		{
			LilvNode* node = lilv_port_get_name(m_plugin, lilvPort);
			QString dispName = lilv_node_as_string(node);
			lilv_node_free(node);
			switch(meta.m_vis)
			{
				case Lv2Ports::Vis::None:
				{
					// allow ~1000 steps
					float stepSize = (meta.m_max - meta.m_min) / 1000.0f;

					// make multiples of 0.01 (or 0.1 for larger values)
					float minStep = (stepSize >= 1.0f) ? 0.1f : 0.01f;
					stepSize -= fmodf(stepSize, minStep);
					stepSize = std::max(stepSize, minStep);

					ctrl->m_connectedModel.reset(
						new FloatModel(meta.m_def, meta.m_min, meta.m_max,
										stepSize, nullptr, dispName));
					break;
				}
				case Lv2Ports::Vis::Integer:
					ctrl->m_connectedModel.reset(
						new IntModel(static_cast<int>(meta.m_def),
										static_cast<int>(meta.m_min),
										static_cast<int>(meta.m_max),
										nullptr, dispName));
					break;
				case Lv2Ports::Vis::Enumeration:
				{
					ComboBoxModel* comboModel
						= new ComboBoxModel(
							nullptr, dispName);
					const LilvScalePoints* sps =
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
					ctrl->m_connectedModel.reset(comboModel);
					break;
				}
				case Lv2Ports::Vis::Toggled:
					ctrl->m_connectedModel.reset(
						new BoolModel(static_cast<bool>(meta.m_def),
										nullptr, dispName));
					break;
			}
		}
		port = ctrl;
	}
	else if (meta.m_type == Lv2Ports::Type::Audio)
	{
		Lv2Ports::Audio* audio =
			new Lv2Ports::Audio(
					static_cast<std::size_t>(
						Engine::mixer()->framesPerPeriod()),
					portIsSideChain(m_plugin, lilvPort)
				);
		port = audio;
	} else {
		port = new Lv2Ports::Unknown;
	}

	*dynamic_cast<Lv2Ports::Meta*>(port) = meta;
	port->m_port = lilvPort;
	port->m_plugin = m_plugin;
}




void Lv2Proc::createPorts()
{
	// register ports at the processor after creation,
	// i.e. link their data or count them
	struct RegisterPort : public Lv2Ports::Visitor
	{
		Lv2Proc* proc;

		void visit(Lv2Ports::Control& ctrl) override
		{
			if(ctrl.m_flow == Lv2Ports::Flow::Input)
			{
				AutomatableModel* amo = ctrl.m_connectedModel.get();
				proc->m_connectedModels.emplace(
					lilv_node_as_string(lilv_port_get_symbol(
										proc->m_plugin, ctrl.m_port)),
					amo);
				proc->addModel(amo);

				++proc->m_controlCount;
			}
		}

		void visit(Lv2Ports::Audio& audio) override
		{
			if(!audio.isSideChain())
			{
				StereoPortRef* portRef;
				StereoPortRef dummy;
				switch(audio.m_flow)
				{
					case Lv2Ports::Flow::Input:
						portRef = &proc->m_inPorts;
						break;
					case Lv2Ports::Flow::Output:
						portRef = &proc->m_outPorts;
						break;
					case Lv2Ports::Flow::Unknown:
						portRef = &dummy;
						break;
				}
				// in Lv2, leftPort is defined to be the first port
				if(!portRef->m_left) {
					portRef->m_left = &audio;
				} else if(!portRef->m_right) {
					portRef->m_right = &audio;
				}
			}
		}
	};

	unsigned maxPorts = lilv_plugin_get_num_ports(m_plugin);
	m_ports.resize(maxPorts);

	for(unsigned portNum = 0; portNum < maxPorts; ++portNum)
	{
		createPort(portNum);
		RegisterPort registerPort;
		registerPort.proc = this;
		m_ports[portNum]->accept(registerPort);
	}

	// initially assign model values to port values
	copyModelsFromCore();

	// debugging:
	//dumpPorts();
}




struct ConnectPorts : public Lv2Ports::Visitor
{
	unsigned m_num;
	LilvInstance* m_instance;
	void con(void* location) {
		lilv_instance_connect_port(m_instance, m_num, location);
	}
	void visit(Lv2Ports::Control& ctrl) override { con(&ctrl.m_val); }
	void visit(Lv2Ports::Audio& audio) override {
		con(audio.isSideChain() ? nullptr : audio.m_buffer.data()); }
	void visit(Lv2Ports::Unknown&) override { con(nullptr); }
	~ConnectPorts() override;
};

ConnectPorts::~ConnectPorts() {}

void Lv2Proc::connectPort(unsigned num)
{
	ConnectPorts connect;
	connect.m_num = num;
	connect.m_instance = m_instance;
	m_ports[num]->accept(connect);
}




void Lv2Proc::destroyPorts()
{
	for(Lv2Ports::PortBase* p: m_ports) { delete p; }
}




void Lv2Proc::dumpPort(std::size_t num)
{
	struct DumpPortDetail : public Lv2Ports::ConstVisitor
	{
		void visit(const Lv2Ports::Control& ctrl) override {
			qDebug() << "  control port";
			// output ports may be uninitialized yet, only print inputs
			if(ctrl.m_flow == Lv2Ports::Flow::Input)
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
		<< lilv_node_as_string(lilv_port_get_name(m_plugin, port.m_port));
	qDebug() << "  flow: " << Lv2Ports::toStr(port.m_flow);
	qDebug() << "  type: " << Lv2Ports::toStr(port.m_type);
	qDebug() << "  visualization: " << Lv2Ports::toStr(port.m_vis);
	if(port.m_type == Lv2Ports::Type::Control || port.m_type == Lv2Ports::Type::Cv)
	{
		qDebug() << "  default:" << port.m_def;
		qDebug() << "  min:" << port.m_min;
		qDebug() << "  max:" << port.m_max;
	}
	qDebug() << "  optional: " << port.m_optional;
	qDebug() << "  => USED: " << port.m_used;

	DumpPortDetail dumper;
	port.accept(dumper);
}




bool Lv2Proc::portIsSideChain(const LilvPlugin *plugin, const LilvPort *port)
{
	return	lilv_port_has_property(plugin, port,
							uri(LV2_CORE_PREFIX "isSidechain").get()) ||
			lilv_port_has_property(plugin, port,
							uri(LV2_CORE__connectionOptional).get());
}




AutoLilvNode Lv2Proc::uri(const char *uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}


#endif // LMMS_HAVE_LV2
