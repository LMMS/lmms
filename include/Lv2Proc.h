/*
 * Lv2Proc.h - Lv2 processor class
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

#ifndef LV2PROC_H
#define LV2PROC_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>
#include <memory>
#include <QObject>

#include "Lv2Basics.h"
#include "LinkedModelGroups.h"
#include "Plugin.h"
#include "PluginIssue.h"


// forward declare port structs/enums
namespace Lv2Ports
{
	struct Audio;
	struct PortBase;

	enum class Type;
	enum class Flow;
	enum class Vis;
}


//! Class representing one Lv2 processor, i.e. one Lv2 handle
//! For Mono effects, 1 Lv2ControlBase references 2 Lv2Proc
class Lv2Proc : public LinkedModelGroup
{
public:
	static Plugin::PluginTypes check(const LilvPlugin* plugin,
		std::vector<PluginIssue> &issues, bool printIssues = false);

	/*
		ctor/dtor
	*/
	Lv2Proc(const LilvPlugin* plugin, Model *parent);
	~Lv2Proc() override;
	//! Must be checked after ctor or reload
	bool isValid() const { return m_valid; }

	/*
		port access
	 */
	struct StereoPortRef
	{
		//! mono port or left port in case of stereo
		Lv2Ports::Audio* m_left = nullptr;
		//! unused, or right port in case of stereo
		Lv2Ports::Audio* m_right = nullptr;
	};

	StereoPortRef& inPorts() { return m_inPorts; }
	const StereoPortRef& inPorts() const { return m_inPorts; }
	StereoPortRef& outPorts() { return m_outPorts; }
	const StereoPortRef& outPorts() const { return m_outPorts; }
	template<class Functor>
	void foreach_port(const Functor& ftor)
	{
		for (std::unique_ptr<Lv2Ports::PortBase>& port : m_ports)
		{
			ftor(port.get());
		}
	}
	template<class Functor>
	void foreach_port(const Functor& ftor) const
	{
		for (const std::unique_ptr<Lv2Ports::PortBase>& port : m_ports)
		{
			ftor(port.get());
		}
	}

	//! Debug function to print ports to stdout
	void dumpPorts();

	/*
		utils for the run thread
	*/
	//! Copy values from all connected models into the respective ports
	void copyModelsFromCore();
	//! Copy buffer passed by the core into our ports
	//! @param offset Offset in each frame of @p buf to take
	//! @param num Number of buffers provided in @param buf
	void copyBuffersFromCore(const sampleFrame *buf,
								unsigned offset, unsigned num, fpp_t frames);
	//! Copy our ports into buffers passed by the core
	//! @param offset Offset in each frame of @p buf to fill
	//! @param num Number of buffers in @param buf we must fill
	void copyBuffersToCore(sampleFrame *buf, unsigned offset, unsigned num,
								fpp_t frames) const;
	//! Run the Lv2 plugin instance for @param frames frames
	void run(fpp_t frames);

	/*
		misc
	 */
	class AutomatableModel *modelAtPort(const QString &uri); // unused currently
	std::size_t controlCount() const { return LinkedModelGroup::modelNum(); }

protected:
	/*
		load and save
	*/
	//! Create ports and instance, connect ports, activate plugin
	void initPlugin();
	//! Deactivate instance
	void shutdownPlugin();

private:
	bool m_valid = true;

	const LilvPlugin* m_plugin;
	LilvInstance* m_instance;

	std::vector<std::unique_ptr<Lv2Ports::PortBase>> m_ports;
	StereoPortRef m_inPorts, m_outPorts;

	//! models for the controls, sorted by port symbols
	std::map<std::string, AutomatableModel *> m_connectedModels;

	//! load a file in the plugin, but don't do anything in LMMS
	void loadFileInternal(const QString &file);
	//! allocate m_ports, fill all with metadata, and assign meaning of ports
	void createPorts();
	//! fill m_ports[portNum] with metadata
	void createPort(std::size_t portNum);
	//! connect m_ports[portNum] with Lv2
	void connectPort(std::size_t num);

	void dumpPort(std::size_t num);

	static bool portIsSideChain(const LilvPlugin* plugin, const LilvPort *port);
	static AutoLilvNode uri(const char* uriStr);
};

#endif // LMMS_HAVE_LV2
#endif // LV2PROC_H
