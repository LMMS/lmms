/*
 * LadspaBase.h - basic declarations concerning LADSPA
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_LADSPA_BASE_H
#define LMMS_LADSPA_BASE_H

#include <QRegularExpression>

#include "LadspaManager.h"
#include "Plugin.h"

namespace lmms
{


class LadspaControl;

enum class BufferRate {
	ChannelIn,
	ChannelOut,
	AudioRateInput,
	AudioRateOutput,
	ControlRateInput,
	ControlRateOutput
};

enum class BufferDataType { Toggled, Enum, Integer, Floating, Time, None };

//! This struct is used to hold port descriptions internally
//! which where received from the ladspa plugin
struct port_desc_t
{
	QString name;
	ch_cnt_t proc;
	uint16_t port_id;
	uint16_t control_id;
	BufferRate rate;
	BufferDataType data_type;
	float scale;
	LADSPA_Data max;
	LADSPA_Data min;
	LADSPA_Data def;
	LADSPA_Data value;
	//! This is true iff ladspa suggests logscale
	//! Note however that the model can still decide to use a linear scale
	bool suggests_logscale;
	LADSPA_Data* buffer;
	LadspaControl* control;
};

inline Plugin::Descriptor::SubPluginFeatures::Key ladspaKeyToSubPluginKey(
						const Plugin::Descriptor * _desc,
						const QString & _name,
						const ladspa_key_t & _key )
{
	Plugin::Descriptor::SubPluginFeatures::Key::AttributeMap m;
	QString file = _key.first;
	m["file"] = file.remove(QRegularExpression("\\.so$")).remove(QRegularExpression("\\.dll$"));
	m["plugin"] = _key.second;
	return Plugin::Descriptor::SubPluginFeatures::Key( _desc, _name, m );
}


} // namespace lmms

#endif // LMMS_LADSPA_BASE_H
