/*
 * LadspaSubPluginFeatures.h - derivation from
 *                             Plugin::Descriptor::SubPluginFeatures for
 *                             hosting LADSPA-plugins
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _LADSPA_SUBPLUGIN_FEATURES_H
#define _LADSPA_SUBPLUGIN_FEATURES_H

#include "ladspa_manager.h"
#include "Plugin.h"


class LadspaSubPluginFeatures : public Plugin::Descriptor::SubPluginFeatures
{
public:
	LadspaSubPluginFeatures( Plugin::PluginTypes _type );

	virtual void fillDescriptionWidget( QWidget * _parent,
												const Key * _key ) const;

	virtual void listSubPluginKeys( const Plugin::Descriptor * _desc,
												KeyList & _kl ) const;

	static ladspa_key_t subPluginKeyToLadspaKey( const Key * _key );

} ;

#endif
