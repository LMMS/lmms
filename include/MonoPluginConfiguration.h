/*
 * MonoPluginConfiguration.h - Settings for plugins with a mono input
 *                             and/or output
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

#ifndef LMMS_MONO_PLUGIN_CONFIGURATION_H
#define LMMS_MONO_PLUGIN_CONFIGURATION_H

#include <QObject>

#include "AutomatableModel.h"
#include "lmms_export.h"
#include "SerializingObject.h"

namespace lmms
{

class LMMS_EXPORT MonoPluginConfiguration // TODO: Give it a better name?
	: public QObject
	, public SerializingObject
{
	Q_OBJECT

public:
	enum class PluginType
	{
		StereoInOut,
		MonoInStereoOut,
		StereoInMonoOut,
		MonoInOut
	};

	/*
	enum class Config
	{
		// For StereoInOut
		None,

		// For MonoInStereoOut
		MonoMixIn, // default
		LeftIn,
		RightIn,

		// For StereoInMonoOut
		MonoMixOut, // default
		LeftOutRightBypass,
		RightOutLeftBypass,

		// For MonoInOut
		MonoMixInOut, // default
		LeftInOutRightBypass,
		RightInOutLeftBypass
	};*/

	/**
	 * Each PluginType has two Config values - one for its input and one for its output.
	 * Not all options are valid for each PluginType (see below).
	 */
	enum class Config
	{
		Stereo,
		MonoMix,
		LeftOnly,
		RightOnly
	};

	/**
	 * List of valid options for each PluginType
	 *
	 * - StereoInOut
	 *     IN:  Stereo
	 *     OUT: Stereo
	 * - MonoInStereoOut
	 *     IN:  MonoMix, LeftOnly (right unused), RightOnly (left unused)
	 *     OUT: Stereo
	 * - StereoInMonoOut
	 *     IN:  Stereo
	 *     OUT: MonoMix, LeftOnly (right bypassed), RightOnly (left bypassed)
	 * - MonoInOut
	 *     IN/OUT: MonoMix, LeftOnly (right bypassed), RightOnly (left bypassed)
	 */

	using QObject::QObject;
	MonoPluginConfiguration(PluginType pluginType, QObject* parent = nullptr);

	auto hasStereoInput() const -> bool;
	auto hasStereoOutput() const -> bool;

	auto pluginType() const { return m_pluginType; }
	void setPluginType(PluginType pluginType);


	auto monoConfig(bool isInput) const -> Config;

	template<bool isInput>
	auto monoConfig() const -> Config
	{
		switch (m_pluginType)
		{
			case PluginType::StereoInOut:
				return Config::Stereo;
			case PluginType::MonoInStereoOut:
				if constexpr (!isInput) { return Config::Stereo; }
				return static_cast<Config>(m_monoConfig.value() + 1);
			case PluginType::StereoInMonoOut:
				if constexpr (isInput) { return Config::Stereo; }
				return static_cast<Config>(m_monoConfig.value() + 1);
			case PluginType::MonoInOut:
				return static_cast<Config>(m_monoConfig.value() + 1);
		}
	}

	auto monoConfigOfModifiableSide() const -> Config;
	auto monoConfigIndex() const -> int { return m_monoConfig.value(); }

	auto setMonoConfig(Config config) -> bool;
	auto setMonoConfigIndex(int config) -> bool;

	/**
	 * SerializingObject implementation
	 */
	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
	auto nodeName() const -> QString override { return "monoconfig"; }

signals:
	void configurationChanged();

private:
	PluginType m_pluginType = PluginType::StereoInOut;

	//! Value is 0..2, which represents { MonoMix, LeftOnly, RightOnly } for non-Stereo plugins
	IntModel m_monoConfig;
};

} // namespace lmms

#endif // LMMS_MONO_PLUGIN_CONFIGURATION_H
