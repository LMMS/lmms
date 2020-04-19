/*
 * Lv2ControlBase.h - Lv2 control base class
 *
 * Copyright (c) 2018-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LV2_CONTROL_BASE_H
#define LV2_CONTROL_BASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>

#include "DataFile.h"
#include "LinkedModelGroups.h"
#include "Plugin.h"

class Lv2Proc;
class PluginIssue;

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
	* inhertiting only from Model will cause diamond inheritance for QObject,
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
class Lv2ControlBase : public LinkedModelGroups
{
public:
	static Plugin::PluginTypes check(const LilvPlugin* m_plugin,
		std::vector<PluginIssue> &issues, bool printIssues = false);

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
	virtual ~Lv2ControlBase() override;
	//! Must be checked after ctor or reload
	bool isValid() const { return m_valid; }

	/*
		overrides
	*/
	LinkedModelGroup* getGroup(std::size_t idx) override;
	const LinkedModelGroup* getGroup(std::size_t idx) const override;

	/*
		utils for the run thread
	*/
	//! Copy values from all connected models into the respective ports
	void copyModelsFromLmms();
	//! Copy buffer passed by LMMS into our ports
	void copyBuffersFromLmms(const sampleFrame *buf, fpp_t frames);
	//! Copy our ports into buffers passed by LMMS
	void copyBuffersToLmms(sampleFrame *buf, fpp_t frames) const;
	//! Run the Lv2 plugin instance for @param frames frames
	void run(fpp_t frames);

	/*
		load/save, must be called from virtuals
	*/
	void saveSettings(QDomDocument &doc, QDomElement &that);
	void loadSettings(const QDomElement &that);
	void loadFile(const QString &file);
	//! TODO: not implemented
	void reloadPlugin();

	/*
		more functions that must be called from virtuals
	*/
	std::size_t controlCount() const;
	QString nodeName() const { return "lv2controls"; }

private:
	//! Return the DataFile settings type
	virtual DataFile::Types settingsType() = 0;
	//! Inform the plugin about a file name change
	virtual void setNameFromFile(const QString &fname) = 0;

	//! Independent processors
	//! If this is a mono effect, the vector will have size 2 in order to
	//! fulfill LMMS' requirement of having stereo input and output
	std::vector<std::unique_ptr<Lv2Proc>> m_procs;

	bool m_valid = true;
	bool m_hasGUI = false;
	unsigned m_channelsPerProc;

	const LilvPlugin* m_plugin;
};

#endif // LMMS_HAVE_LV2
#endif // LV2_CONTROL_BASE_H
