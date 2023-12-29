/*
 * ClapParams.cpp - Implements CLAP params extension
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

#include "ClapParams.h"

#ifdef LMMS_HAVE_CLAP

#include <cassert>

#include <clap/helpers/reducing-param-queue.hxx>

#include "ClapInstance.h"

namespace lmms
{

ClapParams::ClapParams(Model* parent, ClapInstance* instance,
	clap::helpers::EventList* eventsIn, clap::helpers::EventList* eventsOut)
	: LinkedModelGroup{parent}
	, ClapExtension{instance}
	, m_evIn{eventsIn}
	, m_evOut{eventsOut}
{
}

auto ClapParams::init(const clap_host* host, const clap_plugin* plugin) -> bool
{
	if (!ClapExtension::init(host, plugin)) { return false; }
	if (!rescan(CLAP_PARAM_RESCAN_ALL)) { return false; }
	setModels();
	return true;
}

void ClapParams::deinit()
{
	m_paramMap.clear();
	m_params.clear();
	ClapExtension::deinit();
}

auto ClapParams::hostExt() const -> const clap_host_params*
{
	static clap_host_params ext {
		&clapRescan,
		&clapClear,
		&clapRequestFlush
	};
	return &ext;
}

auto ClapParams::checkSupported(const clap_plugin_params* ext) -> bool
{
	// NOTE: value_to_text and text_to_value are not strictly required
	return ext->count && ext->get_info && ext->get_value && ext->flush;
}

auto ClapParams::rescan(clap_param_rescan_flags flags) -> bool
{
	// TODO: Update LinkedModelGroup when parameters are added/removed?
	assert(ClapThreadCheck::isMainThread());

	if (!supported()) { return false; }

	// 1. It is forbidden to use CLAP_PARAM_RESCAN_ALL if the plugin is active
	if (instance()->isActive() && (flags & CLAP_PARAM_RESCAN_ALL))
	{
		instance()->log(CLAP_LOG_WARNING, "clap_host_params.recan(CLAP_PARAM_RESCAN_ALL) was called while the plugin is active");
		return false;
	}

	// 2. Scan the params
	auto count = pluginExt()->count(plugin());
	qDebug() << "CLAP PARAMS: count:" << count;
	std::unordered_set<clap_id> paramIds(count * 2);
	bool needToUpdateParamsCache = false;

	for (std::int32_t i = 0; i < count; ++i)
	{
		clap_param_info info{};
		info.id = CLAP_INVALID_ID;

		if (!pluginExt()->get_info(plugin(), i, &info))
		{
			instance()->log(CLAP_LOG_WARNING, "clap_plugin_params.get_info() returned false!");
			return false; // TODO: continue?
		}

		if (!ClapParameter::check(info)) { return false; } // TODO: continue?

		if (info.id == CLAP_INVALID_ID)
		{
			std::string msg = "clap_plugin_params.get_info() reported a parameter with id = CLAP_INVALID_ID\n"
				" 2. name: " + std::string{info.name} + ", module: " + std::string{info.module};
			instance()->log(CLAP_LOG_WARNING, msg.c_str());
			return false; // TODO: continue?
		}

		auto it = m_paramMap.find(info.id);

		// Check that the parameter is not declared twice
		if (paramIds.count(info.id) > 0)
		{
			assert(it != m_paramMap.end());
			std::string msg = "the parameter with id: " + std::to_string(info.id) + " was declared twice.\n"
				" 1. name: " + std::string{it->second->info().name} + ", module: " + std::string{it->second->info().module} + "\n"
				" 2. name: " + std::string{info.name} + ", module: " + std::string{info.module};
			instance()->log(CLAP_LOG_WARNING, msg.c_str());
			return false; // TODO: continue?
		}
		paramIds.insert(info.id);

		if (it == m_paramMap.end())
		{
			if (!(flags & CLAP_PARAM_RESCAN_ALL))
			{
				std::string msg = "A new parameter was declared, but the flag CLAP_PARAM_RESCAN_ALL was not specified; "
					"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
				instance()->log(CLAP_LOG_WARNING, msg.c_str());
				return false; // TODO: continue?
			}

			double value = getValue(info);
			auto param = std::make_unique<ClapParameter>(this, info, value);
			checkValidParamValue(*param, value);
			m_paramMap.insert_or_assign(info.id, std::move(param));
			needToUpdateParamsCache = true;
		}
		else
		{
			// Update param info
			if (!it->second->isInfoEqualTo(info))
			{
				if (!rescanMayInfoChange(flags))
				{
					std::string msg = "a parameter's info did change, but the flag CLAP_PARAM_RESCAN_INFO was not specified; "
						"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
					instance()->log(CLAP_LOG_WARNING, msg.c_str());
					return false; // TODO: continue?
				}

				if (!(flags & CLAP_PARAM_RESCAN_ALL) && !it->second->isInfoCriticallyDifferentTo(info))
				{
					std::string msg = "a parameter's info has critical changes, but the flag CLAP_PARAM_RESCAN_ALL was not specified; "
						"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
					instance()->log(CLAP_LOG_WARNING, msg.c_str());
					return false; // TODO: continue?
				}

				it->second->setInfo(info);
			}

			double value = getValue(info);
			if (it->second->value() != value)
			{
				if (!rescanMayValueChange(flags))
				{
					std::string msg = "a parameter's value did change but, but the flag CLAP_PARAM_RESCAN_VALUES was not specified; "
						"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
					instance()->log(CLAP_LOG_WARNING, msg.c_str());
					return false; // TODO: continue?
				}

				// Update param value
				checkValidParamValue(*it->second, value);
				it->second->setValue(value);
				it->second->setModulation(value);
			}
		}
	}

	// 3. Remove parameters which are gone
	for (auto it = m_paramMap.begin(); it != m_paramMap.end();)
	{
		if (paramIds.find(it->first) == paramIds.end())
		{
			if (!(flags & CLAP_PARAM_RESCAN_ALL))
			{
				const auto& info = it->second->info();
				std::string msg = "a parameter was removed, but the flag CLAP_PARAM_RESCAN_ALL was not specified; "
					"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
				instance()->log(CLAP_LOG_WARNING, msg.c_str());
				return false;
			}
			it = m_paramMap.erase(it);
			needToUpdateParamsCache = true;
		}
		else { ++it; }
	}

	if (needToUpdateParamsCache)
	{
		m_params.resize(m_paramMap.size());
		int i = 0;
		for (const auto& elem : m_paramMap)
		{
			m_params[i] = elem.second.get();
			++i;
		}
	}

	if (flags & CLAP_PARAM_RESCAN_ALL) { paramsChanged(); }

	return true;
}

void ClapParams::idle()
{
	// Try to send events to the audio engine
	m_hostToPluginValueQueue.producerDone();
	m_hostToPluginModQueue.producerDone();

	m_pluginToHostValueQueue.consume(
		[this](clap_id paramId, const PluginToHostParamQueueValue& value) {
			const auto it = m_paramMap.find(paramId);
			if (it == m_paramMap.end())
			{
				std::string msg = "Plugin produced a CLAP_EVENT_PARAM_SET with an unknown param id: " + std::to_string(paramId);
				instance()->log(CLAP_LOG_WARNING, msg.c_str());
				return;
			}

			if (value.hasValue) { it->second->setValue(value.value); }
			if (value.hasGesture) { it->second->setIsAdjusting(value.isBegin); }

			emit paramAdjusted(paramId);
		}
	);

	if (m_scheduleFlush && !instance()->isActive())
	{
		flushOnMainThread();
	}
}

void ClapParams::processEnd()
{
	m_pluginToHostValueQueue.producerDone();
}

void ClapParams::flushOnMainThread()
{
	//qDebug() << "ClapParams::flushOnMainThread";
	assert(ClapThreadCheck::isMainThread());
	assert(!instance()->isActive());

	if (!supported())
	{
		instance()->log(CLAP_LOG_WARNING, "Attempted to flush parameters on main thread, but plugin does not support params extension");
		return;
	}

	m_scheduleFlush = false;

	m_evIn->clear();
	m_evOut->clear();

	generatePluginInputEvents();

	pluginExt()->flush(m_plugin, m_evIn->clapInputEvents(), m_evOut->clapOutputEvents());

	handlePluginOutputEvents();

	m_evOut->clear();
	m_pluginToHostValueQueue.producerDone();
}

void ClapParams::generatePluginInputEvents()
{
	m_hostToPluginValueQueue.consume(
		[this](clap_id paramId, const HostToPluginParamQueueValue& value) {
			clap_event_param_value ev;
			ev.header.time = 0;
			ev.header.type = CLAP_EVENT_PARAM_VALUE;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.param_id = paramId;
			ev.cookie = s_provideCookie ? value.cookie : nullptr;
			ev.port_index = 0;
			ev.key = -1;
			ev.channel = -1;
			ev.note_id = -1;
			ev.value = value.value;
			m_evIn->push(&ev.header);
		});

	m_hostToPluginModQueue.consume(
		[this](clap_id paramId, const HostToPluginParamQueueValue& value) {
			clap_event_param_mod ev;
			ev.header.time = 0;
			ev.header.type = CLAP_EVENT_PARAM_MOD;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.param_id = paramId;
			ev.cookie = s_provideCookie ? value.cookie : nullptr;
			ev.port_index = 0;
			ev.key = -1;
			ev.channel = -1;
			ev.note_id = -1;
			ev.amount = value.value;
			m_evIn->push(&ev.header);
		});
}

void ClapParams::handlePluginOutputEvents()
{
	// TODO: Are LMMS models being updated with values here?
	for (std::uint32_t i = 0; i < m_evOut->size(); ++i)
	{
		auto header = m_evOut->get(i);
		switch (header->type)
		{
			case CLAP_EVENT_PARAM_GESTURE_BEGIN:
			{
				auto event = reinterpret_cast<const clap_event_param_gesture*>(header);
				handlePluginOutputEvent(event, true);
				break;
			}
			case CLAP_EVENT_PARAM_GESTURE_END:
			{
				auto event = reinterpret_cast<const clap_event_param_gesture*>(header);
				handlePluginOutputEvent(event, false);
				break;
			}
			case CLAP_EVENT_PARAM_VALUE:
			{
				auto event = reinterpret_cast<const clap_event_param_value*>(header);
				handlePluginOutputEvent(event);
				break;
			}
			default:
				break;
		}
	}
}

void ClapParams::handlePluginOutputEvent(const clap_event_param_gesture* event, bool gestureBegin)
{
	bool& isAdj = m_isAdjustingParameter[event->param_id];

	if (isAdj == gestureBegin)
	{
		instance()->log(CLAP_LOG_PLUGIN_MISBEHAVING, gestureBegin
			? "The plugin sent BEGIN_ADJUST twice"
			: "The plugin sent END_ADJUST without a preceding BEGIN_ADJUST");
	}
	isAdj = gestureBegin;

	PluginToHostParamQueueValue v;
	v.hasGesture = true;
	v.isBegin = gestureBegin;
	m_pluginToHostValueQueue.setOrUpdate(event->param_id, v);
}

void ClapParams::handlePluginOutputEvent(const clap_event_param_value* event)
{
	PluginToHostParamQueueValue v;
	v.hasValue = true;
	v.value = event->value;
	m_pluginToHostValueQueue.setOrUpdate(event->param_id, v);
}

void ClapParams::setModels()
{
	LinkedModelGroup::clearModels();
	for (auto param : m_params)
	{
		if (param && param->model())
		{
			const auto uri = QString::fromUtf8(param->id().data());
			LinkedModelGroup::addModel(param->model(), uri);

			// Tell plugin when param value changes in host
			auto updateParam = [this, param]() {
				setParamValueByHost(*param, param->model()->value<float>());
			};

			// This is used for updating input parameters instead of copyModelsFromCore()
			connect(param->model(), &Model::dataChanged, this, updateParam);

			// Initially assign model value to param value
			updateParam();
		}
	}
}

auto ClapParams::checkValidParamValue(const ClapParameter& param, double value) -> bool
{
	if (!param.isValueValid(value))
	{
		std::string msg = "Invalid value for param. " + param.getInfoString() + "; value: " + std::to_string(value);
		instance()->log(CLAP_LOG_WARNING, msg.c_str());
		return false;
	}
	return true;
}

auto ClapParams::getValue(const clap_param_info& info) const -> double
{
	// TODO: Return std::optional<double>?
	assert(ClapThreadCheck::isMainThread());
	assert(supported());

	double value = 0.0;
	if (pluginExt()->get_value(m_plugin, info.id, &value)) { return value; }

	std::string msg = "failed to get the param value, id: " + std::to_string(info.id)
		+ ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
	throw std::logic_error{msg};
}

auto ClapParams::getValueText(const ClapParameter& param) const -> std::string
{
	const auto valueStr = std::to_string(param.value());
	if (!pluginExt()->value_to_text)
	{
		return valueStr;
	}

	auto buffer = std::array<char, CLAP_NAME_SIZE>{};
	if (!pluginExt()->value_to_text(plugin(), param.info().id, param.value(), buffer.data(), CLAP_NAME_SIZE - 1))
	{
		return valueStr;
	}

	if (valueStr == buffer.data())
	{
		// No point in displaying two identical strings
		return valueStr;
	}

	// Use CLAP-provided string + internal value in brackets for automation purposes
	return std::string{buffer.data()} + "\n[" + valueStr + "]";
}

void ClapParams::setParamValueByHost(ClapParameter& param, double value)
{
	assert(ClapThreadCheck::isMainThread());

	param.setValue(value);

	m_hostToPluginValueQueue.set(param.info().id, {param.info().cookie, value});
	m_hostToPluginValueQueue.producerDone();
	clapRequestFlush(host());
}

void ClapParams::setParamModulationByHost(ClapParameter& param, double value)
{
	assert(ClapThreadCheck::isMainThread());

	param.setModulation(value);

	m_hostToPluginModQueue.set(param.info().id, {param.info().cookie, value});
	m_hostToPluginModQueue.producerDone();
	clapRequestFlush(host());
}

void ClapParams::clapRescan(const clap_host* host, clap_param_rescan_flags flags)
{
	auto h = fromHost(host);
	if (!h) { return; }
	h->params().rescan(flags);
}

void ClapParams::clapClear(const clap_host* host, clap_id param_id, clap_param_clear_flags flags)
{
	assert(ClapThreadCheck::isMainThread());
	qDebug() << "ClapParams::clapClear";
	// TODO
}

void ClapParams::clapRequestFlush(const clap_host* host)
{
	//qDebug() << "ClapInstance::hostExtParamsRequestFlush";
	auto h = fromHost(host);
	if (!h) { return; }
	auto& params = h->params();

	if (!h->isActive() && ClapThreadCheck::isMainThread())
	{
		// Perform the flush immediately
		params.flushOnMainThread();
		return;
	}

	params.m_scheduleFlush = true;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
