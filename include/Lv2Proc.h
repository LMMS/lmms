/*
 * Lv2Proc.h - Lv2 processor class
 *
 * Copyright (c) 2019-2024 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_LV2_PROC_H
#define LMMS_LV2_PROC_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>
#include <memory>
#include <optional>

#include <ringbuffer/ringbuffer.h>

#include "ModelGroup.h"
#include "LmmsSemaphore.h"
#include "Lv2Basics.h"
#include "Lv2Features.h"
#include "Lv2Options.h"
#include "Lv2Worker.h"
#include "Plugin.h"
#include "TimePos.h"


namespace lmms
{

class PluginIssue;
class SampleFrame;

// forward declare port structs/enums
namespace Lv2Ports
{
	struct Audio;
	struct PortBase;
	struct AtomSeq;

	enum class Type;
	enum class Flow;
	enum class Vis;
}

/**
	Class representing one Lv2 processor, i.e. one Lv2 handle.
	For mono plugins, L/R channel routing is being used
	(this includes techniques like upmixing and downmixing).

	This class provides everything Lv2 effect and instrument have in common.
	It's not named Lv2Plugin, because
	* it does not inherit Instrument
	* the Plugin subclass Effect does not inherit this class

	This class would usually be a Model subclass. However, Qt doesn't allow
	this:
	* inheriting only from Model will cause diamond inheritance for QObject,
	  which will cause errors with Q_OBJECT
	* making this a direct subclass of Instrument resp. EffectControls would
	  require CRTP, which would make this class a template class, which would
	  conflict with Q_OBJECT

	The consequence is that this class can neither inherit QObject or Model, nor
	Instrument or EffectControls, which means in fact:
	* this class contains no signals or slots, but it offers stubs for slots
	  that shall be called by child classes
	* this class can not override virtuals of Instrument or EffectControls, so
	  it will offer functions that must be called by virtuals in its child class
*/
class LMMS_EXPORT Lv2Proc : public ModelGroup
{
	friend class Lv2ProcSuspender;
public:
	static Plugin::Type check(const LilvPlugin* plugin,
		std::vector<PluginIssue> &issues);

	/*
		ctor/dtor/reload
	*/
	Lv2Proc(Model *parent, const QString &uri);
	~Lv2Proc();
	void reload();
	void onSampleRateChanged();

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
	//! Copy values from the LMMS core (connected models, MIDI events, ...) into
	//! the respective ports
	void copyModelsFromCore();
	//! Bring values from all ports to the LMMS core
	void copyModelsToCore();
	/**
	 * Copy buffer passed by the core into our ports
	 * @param buf buffer of sample frames, each sample frame is something like
	 *   a `float[<number-of-procs> * <channels per proc>]` array.
	 */
	void copyBuffersFromCore(const SampleFrame* buf, fpp_t frames);
	/**
	 * Copy our ports into buffers passed by the core
	 * @param buf buffer of sample frames, each sample frame is something like
	 *   a `float[<number-of-procs> * <channels per proc>]` array.
	 */
	void copyBuffersToCore(SampleFrame* buf, fpp_t frames) const;
	//! Run the Lv2 plugin instance for @param frames frames
	void run(fpp_t frames);

	void handleMidiInputEvent(const class MidiEvent &event,
		const TimePos &time, f_cnt_t offset);

	/*
		misc
	 */
	class AutomatableModel *modelAtPort(const QString &uri); // unused currently
	std::size_t controlCount() const { return ModelGroup::size(); }
	bool hasNoteInput() const;
	const LilvPlugin* getPlugin() const { return m_plugin; }
	bool hasGui() const { return m_hasGUI; }

protected:
	//! To be called by `Plugin::loadFile` overrides
	void loadFile(const QString &file) { (void)file; }
	//! XML DOM Node name
	QString nodeName() const { return "lv2controls"; }

private:
	// lv2
	const LilvPlugin* m_plugin;
	LilvInstance* m_instance = nullptr;
	Lv2Features m_features;

	// options
	Lv2Options m_options;

	// worker
	std::optional<Lv2Worker> m_worker;
	Semaphore m_workLock; // this must be shared by different workers

	// full list of ports
	std::vector<std::unique_ptr<Lv2Ports::PortBase>> m_ports;
	// quick reference to specific, unique ports
	StereoPortRef m_inPorts, m_outPorts;
	Lv2Ports::AtomSeq *m_midiIn = nullptr, *m_midiOut = nullptr;

	// MIDI
	// many things here may be moved into the `Instrument` class
	constexpr const static std::size_t m_maxMidiInputEvents = 1024;
	//! spinlock for the MIDI ringbuffer (for MIDI events going to the plugin)
	std::atomic_flag m_ringLock = ATOMIC_FLAG_INIT;

	//! MIDI ringbuffer (for MIDI events going to the plugin)
	ringbuffer_t<struct MidiInputEvent> m_midiInputBuf;
	//! MIDI ringbuffer reader
	ringbuffer_reader_t<struct MidiInputEvent> m_midiInputReader;

	// other
	static int32_t defaultEvbufSize() { return 1 << 15; /* ardour uses this*/ }
	bool m_hasGUI = false;

	//! models for the controls, sorted by port symbols
	//! @note These are not owned, but rather link to the models in
	//!   ControlPorts in `m_ports`
	std::map<std::string, AutomatableModel *> m_connectedModels;

	void initMOptions(); //!< initialize m_options
	void initPluginSpecificFeatures();

	//! Create ports and instance, connect ports, activate plugin
	void initPlugin();
	//! Deactivate instance
	void shutdownPlugin();
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
	static bool portIsOptional(const LilvPlugin* plugin, const LilvPort *port);
	static AutoLilvNode uri(const char* uriStr);
};


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2_PROC_H
