/*
 * ClapControlBase.h - CLAP control base class
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_CLAP_CONTROL_BASE_H
#define LMMS_CLAP_CONTROL_BASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapFile.h"
#include "ClapInstance.h"
#include "DataFile.h"
#include "LinkedModelGroups.h"
#include "Plugin.h"
#include "ArrayVector.h"
#include "lmms_export.h"

#include <memory>
#include <clap/clap.h>

namespace lmms
{

class Model;

/**
 * Common base class for CLAP plugins.
 * The design of this class is based on Lv2ControlBase by Johannes Lorenz.
 * See Lv2ControlBase for more information.
 */
class LMMS_EXPORT ClapControlBase : public LinkedModelGroups
{
public:
	auto pluginInfo() const -> const ClapPluginInfo* { return m_info; }

	auto control(std::size_t idx) -> ClapInstance* { return m_instances[idx].get(); }
	auto control(std::size_t idx) const -> const ClapInstance* { return m_instances[idx].get(); }

	auto hasGui() const -> bool { return m_instances[0]->gui(); }

	void callHostIdle();

protected:

	//! @param that the class inheriting this class and inheriting Model;
	//!   this is the same pointer as this, but a different type
	//! @param uri the CLAP URI telling this class what plugin to construct
	ClapControlBase(Model* that, const QString& uri);
	ClapControlBase(const ClapControlBase&) = delete;
	auto operator=(const ClapControlBase&) -> ClapControlBase& = delete;
	~ClapControlBase() override;

	void init(Model* that, const QString& uri);

	//! Must be checked after ctor or reload
	auto isValid() const -> bool { return m_valid; }

	void reload();

	/*
	 * overrides
	 */
	auto getGroup(std::size_t idx) -> LinkedModelGroup* override;
	auto getGroup(std::size_t idx) const -> const LinkedModelGroup* override;

	/*
	 * utils for the run thread
	 */
	//! Copy values from the LMMS core (connected models, MIDI events, ...) into
	//! the respective ports
	void copyModelsFromLmms();

	//! Bring values from all ports to the LMMS core
	void copyModelsToLmms() const;

	//! Copy LMMS core audio buffer into CLAP audio input buffer
	void copyBuffersFromLmms(const sampleFrame* buf, fpp_t frames);

	//! Copy CLAP audio output buffers into buffer passed by LMMS core
	void copyBuffersToLmms(sampleFrame* buf, fpp_t frames) const;

	//! Run the CLAP plugin instance(s) for @param frames frames
	void run(fpp_t frames);

	/*
	 * load/save, must be called from virtuals
	 */
	void saveSettings(QDomDocument& doc, QDomElement& that);
	void loadSettings(const QDomElement& that);
	void loadFile(const QString& file);

	/*
	 * more functions that must be called from virtuals
	 */
	auto controlCount() const -> std::size_t;
	auto nodeName() const -> QString { return "clapcontrols"; }
	auto hasNoteInput() const -> bool;
	void handleMidiInputEvent(const class MidiEvent& event, const class TimePos& time, f_cnt_t offset);

private:

	//! Return the DataFile settings type
	//virtual auto settingsType() -> DataFile::Type = 0;

	//! Inform the plugin about a file name change
	//virtual void setNameFromFile(const QString& fname) = 0;

	//! Independent processors
	//! If this is a mono effect, the ArrayVector will have size 2 in order to
	//! fulfill LMMS' requirement of having stereo input and output
	ArrayVector<std::unique_ptr<ClapInstance>, DEFAULT_CHANNELS> m_instances;

	const ClapPluginInfo* m_info = nullptr;

	bool m_valid = false;
	unsigned m_channelsPerInstance = 0;
};


} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_CONTROL_BASE_H
