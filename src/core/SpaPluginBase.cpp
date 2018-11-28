/*
 * SpaPluginBase.cpp - common class for spa instruments, effects, etc
 *
 * Copyright (c) 2018-2018 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "SpaPluginBase.h"

#include <QDebug>
#include <spa/audio.h>
#include <spa/spa.h>

SpaPluginBase::~SpaPluginBase() {}

Plugin::PluginTypes SpaPluginBase::getPluginType(spa::descriptor *desc)
{
	spa::plugin *plug = desc->instantiate();

	struct TypeChecker final : public virtual spa::audio::visitor
	{
		std::size_t m_inCount = 0, m_outCount = 0;
		void visit(spa::audio::in &) override { ++m_inCount; }
		void visit(spa::audio::out &) override { ++m_outCount; }
		void visit(spa::audio::stereo::in &) override { ++++m_inCount; }
		void visit(spa::audio::stereo::out &) override
		{
			++++m_outCount;
		}
	} tyc;

	for (const spa::simple_str &portname : desc->port_names())
	{
		try
		{
			plug->port(portname.data()).accept(tyc);
		}
		catch (spa::port_not_found &)
		{
			return Plugin::PluginTypes::Undefined;
		}
	}

	delete plug;

	Plugin::PluginTypes res;
	if (tyc.m_inCount > 2 || tyc.m_outCount > 2)
	{
		res = Plugin::PluginTypes::Undefined;
	} // TODO: enable mono effects?
	else if (tyc.m_inCount == 2 && tyc.m_outCount == 2)
	{
		res = Plugin::PluginTypes::Effect;
	}
	else if (tyc.m_inCount == 0 && tyc.m_outCount == 2)
	{
		res = Plugin::PluginTypes::Instrument;
	}
	else
	{
		res = Plugin::PluginTypes::Other;
	}

	qDebug() << "Plugin type of " << spa::unique_name(*desc).c_str() << ":";
	qDebug() << (res == Plugin::PluginTypes::Undefined
			? "  undefined"
			: res == Plugin::PluginTypes::Effect
				? "  effect"
				: res == Plugin::PluginTypes::Instrument
					? "  instrument"
					: "  other");

	return res;
}
