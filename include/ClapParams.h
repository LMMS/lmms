/*
 * ClapParams.h - Implements CLAP params extension
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_CLAP_PARAMS_H
#define LMMS_CLAP_PARAMS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <vector>
#include <unordered_map>
#include <clap/ext/params.h>
#include <clap/helpers/event-list.hh>
#include <clap/helpers/reducing-param-queue.hh>

#include "ClapExtension.h"
#include "ClapParameter.h"
#include "LinkedModelGroups.h"

namespace lmms
{

class ClapParams final
	: public LinkedModelGroup
	, public ClapExtension<clap_host_params, clap_plugin_params>
{
	Q_OBJECT
public:
	ClapParams(Model* parent, ClapInstance* instance,
		clap::helpers::EventList* eventsIn, clap::helpers::EventList* eventsOut);
	~ClapParams() override = default;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_PARAMS; }
	auto hostExt() const -> const clap_host_params* override;

	auto rescan(clap_param_rescan_flags flags) -> bool;

	void idle();
	void processEnd();

	void flushOnMainThread();
	void generatePluginInputEvents();
	void handlePluginOutputEvents();
	void handlePluginOutputEvent(const clap_event_param_gesture* event, bool gestureBegin);
	void handlePluginOutputEvent(const clap_event_param_value* event);

	auto getValue(const clap_param_info& info) const -> double;
	auto getValueText(const ClapParameter& param) const -> std::string;

	auto parameters() const -> const std::vector<ClapParameter*>& { return m_params; }
	auto flushScheduled() const { return m_scheduleFlush; }

signals:
	//! Called when CLAP plugin changes params and LMMS core needs to update
	void paramsChanged();

	void paramAdjusted(clap_id paramId);

private:
	auto initImpl(const clap_host* host, const clap_plugin* plugin) noexcept -> bool override;
	void deinitImpl() noexcept override;
	auto checkSupported(const clap_plugin_params& ext) -> bool override;

	void setModels();
	auto checkValidParamValue(const ClapParameter& param, double value) -> bool;
	void setParamValueByHost(ClapParameter& param, double value);
	void setParamModulationByHost(ClapParameter& param, double value);

	static auto rescanMayValueChange(std::uint32_t flags) -> bool { return flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_VALUES); }
	static auto rescanMayInfoChange(std::uint32_t flags) -> bool { return flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_INFO); }

	/**
	 * clap_host_params implementation
	 */
	static void clapRescan(const clap_host* host, clap_param_rescan_flags flags);
	static void clapClear(const clap_host* host, clap_id param_id, clap_param_clear_flags flags);
	static void clapRequestFlush(const clap_host* host);

	std::unordered_map<clap_id, std::unique_ptr<ClapParameter>> m_paramMap;
	std::vector<ClapParameter*> m_params; //!< Cache for faster iteration

	// TODO: Find better way to handle param and note events
	clap::helpers::EventList* m_evIn;  //!< owned by ClapInstance
	clap::helpers::EventList* m_evOut; //!< owned by ClapInstance

	struct HostToPluginParamQueueValue
	{
		void* cookie;
		double value;
	};

	struct PluginToHostParamQueueValue
	{
		void update(const PluginToHostParamQueueValue& v) noexcept
		{
			if (v.hasValue)
			{
				hasValue = true;
				value = v.value;
			}

			if (v.hasGesture)
			{
				hasGesture = true;
				isBegin = v.isBegin;
			}
		}

		bool hasValue = false;
		bool hasGesture = false;
		bool isBegin = false;
		double value = 0;
	};

	clap::helpers::ReducingParamQueue<clap_id, HostToPluginParamQueueValue> m_hostToPluginValueQueue;
	clap::helpers::ReducingParamQueue<clap_id, HostToPluginParamQueueValue> m_hostToPluginModQueue;
	clap::helpers::ReducingParamQueue<clap_id, PluginToHostParamQueueValue> m_pluginToHostValueQueue;

	std::unordered_map<clap_id, bool> m_isAdjustingParameter;

	bool m_scheduleFlush = false;

	static constexpr bool s_provideCookie = true;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PARAMS_H
