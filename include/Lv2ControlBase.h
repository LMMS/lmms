/*
 * Lv2ControlBase.h - Lv2 control base class
 *
 * Copyright (c) 2018-2023 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_LV2_CONTROL_BASE_H
#define LMMS_LV2_CONTROL_BASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>
#include <memory>

#include "LinkedModelGroups.h"
#include "lmms_export.h"
#include "Plugin.h"

namespace lmms
{


class Lv2Proc;
class PluginIssue;
class SampleFrame;

/**
	Common base class for Lv2 plugins

	This class contains a vector of Lv2Proc, usually 1 (for stereo plugins) or
	2 (for mono plugins). Most of the logic is done there, this class primarily
	forwards work to the Lv2Proc and collects the results.

	This class provides everything Lv2 plugins have in common. It's not
	named Lv2Plugin, because
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
class LMMS_EXPORT Lv2ControlBase : public LinkedModelGroups
{
public:
	static Plugin::Type check(const LilvPlugin* m_plugin,
		std::vector<PluginIssue> &issues);

	void shutdown();
	void init(Model* meAsModel);

	const LilvPlugin* getPlugin() const { return m_plugin; }

	Lv2Proc *control(std::size_t idx) { return m_procs[idx].get(); }
	const Lv2Proc *control(std::size_t idx) const { return m_procs[idx].get(); }

	bool hasGui() const { return m_hasGUI; }
	void setHasGui(bool val) { m_hasGUI = val; }

protected:
	/*
		ctor/dtor
	 */
	//! @param that the class inheriting this class and inheriting Model;
	//!   this is the same pointer as this, but a different type
	//! @param uri the Lv2 URI telling this class what plugin to construct
	Lv2ControlBase(class Model *that, const QString& uri);
	Lv2ControlBase(const Lv2ControlBase&) = delete;
	~Lv2ControlBase() override;
	void reload();

	Lv2ControlBase& operator=(const Lv2ControlBase&) = delete;

	/*
		overrides
	*/
	LinkedModelGroup* getGroup(std::size_t idx) override;
	const LinkedModelGroup* getGroup(std::size_t idx) const override;

	/*
		utils for the run thread
	*/
	//! Copy values from the LMMS core (connected models, MIDI events, ...) into
	//! the respective ports
	void copyModelsFromLmms();
	//! Bring values from all ports to the LMMS core
	void copyModelsToLmms() const;

	//! Copy buffer passed by LMMS into our ports
	void copyBuffersFromLmms(const SampleFrame* buf, fpp_t frames);
	//! Copy our ports into buffers passed by LMMS
	void copyBuffersToLmms(SampleFrame* buf, fpp_t frames) const;
	//! Run the Lv2 plugin instance for @param frames frames
	void run(fpp_t frames);

	/*
		load/save, must be called from virtuals
	*/
	void saveSettings(QDomDocument &doc, QDomElement &that);
	void loadSettings(const QDomElement &that);
	void loadFile(const QString &file);

	/*
		more functions that must be called from virtuals
	*/
	std::size_t controlCount() const;
	QString nodeName() const { return "lv2controls"; }
	bool hasNoteInput() const;
	void handleMidiInputEvent(const class MidiEvent &event,
		const class TimePos &time, f_cnt_t offset);

private:
	//! Independent processors
	//! If this is a mono effect, the vector will have size 2 in order to
	//! fulfill LMMS' requirement of having stereo input and output
	std::vector<std::unique_ptr<Lv2Proc>> m_procs;

	bool m_hasGUI = false;
	unsigned m_channelsPerProc;

	const LilvPlugin* m_plugin;
};


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2_CONTROL_BASE_H
