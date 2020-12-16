/*
 * carlarack.cpp - Carla for LMMS (Rack)
 *
 * Copyright (C) 2014-2018 Filipe Coelho <falktx@falktx.com>
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

#include "carla.h"

#include "embed.h"
#include "plugin_export.h"
#include "InstrumentTrack.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT carlarack_plugin_descriptor =
{
    STRINGIFY( PLUGIN_NAME ),
    "Carla Rack",
    QT_TRANSLATE_NOOP( "PluginBrowser",
                       "Carla Rack Instrument" ),
    "falkTX <falktx/at/falktx.com>",
    CARLA_VERSION_HEX,
    Plugin::Instrument,
    new PluginPixmapLoader( "logo" ),
    NULL,
    NULL
} ;

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
    return new CarlaInstrument(static_cast<InstrumentTrack*>(m), &carlarack_plugin_descriptor, false);
}

}
