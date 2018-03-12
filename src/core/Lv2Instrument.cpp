/*
 * Lv2Instrument.cpp - implementation of Lv2Instrument class
 *
 * Copyright (c) 2018 Alexandros Theodotou @faiyadesu
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

#include "Engine.h"
#include "Lv2Instrument.h"
#include "Lv2Manager.h"
#include "Mixer.h"

#include "embed.h"

static PixmapLoader dummyLoader;

extern "C"
{

Plugin::Descriptor lv2_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Lv2 Ins",
	QT_TRANSLATE_NOOP( "Lv2 Instrument",
				"An lv2 instrument plugin" ),
	"Alexandros Theodotou @faiyadesu",
	0x0100,
	Plugin::Instrument,
	&dummyLoader, // logo
	NULL
} ;
}

Lv2Instrument::Lv2Instrument(const LilvPlugin* _pi,
		InstrumentTrack * _it) :
	Instrument( _it, &lv2_plugin_descriptor ),
	m_plugin(_pi)
{
	//Lv2Manager::getInstance().getWorld()->get_all_plugins()
		//.get_by_uri()
	//lv2_plugin_descriptor.name =
		//m_pluginInfo->getName().toUtf8();
	//lv2_plugin_descriptor.displayName =
		//m_pluginInfo->getName().toUtf8();
		//_pi.getRawPlugin()->get_name().as_string();
	//lv2_plugin_descriptor.description = _pi.getRawPlugin();
	//Lv2Manager::getInstance().getPlugin();
	//m_instance = Lilv::Instance::create(
			//*(_pi.getRawPlugin()),
			//Engine::mixer()->baseSampleRate(),
			//Lv2Manager::getInstance().getHostFeatures());
			////_pi.getRawPlugin()->get_required_features());
	//Lilv::Nodes rf = _pi.getRawPlugin()
		//->get_required_features();
	//LilvIter* iter = rf.begin();
	//do
	//{
		//Lilv::Node f = rf.get(iter);
		//qDebug("Plugin required feature: %s", f.as_string());
	//} while ((iter = rf.next(iter)) != nullptr);
	//instance->get_descriptor();
	//instance->activate();
	// TODO save/pass descriptor and handle somewhere
}

Lv2Instrument::~Lv2Instrument()
{

}

void Lv2Instrument::playNote( NotePlayHandle * _note_to_play/* _note_to_play */,
				sampleFrame * _working_buf/* _working_buf */ )
{
	// TODO
		memset( _working_buf, 0, sizeof( sampleFrame ) *
			Engine::mixer()->framesPerPeriod() );
}

void Lv2Instrument::deleteNotePluginData( NotePlayHandle * _note_to_play )
{
	// TODO
}


void Lv2Instrument::loadSettings(const QDomElement& _this)
{

}

QString Lv2Instrument::nodeName() const
{
	return lilv_node_as_string(lilv_plugin_get_name(m_plugin));
}

void Lv2Instrument::saveSettings(QDomDocument& _doc, QDomElement& _parent)
{

}

PluginView* Lv2Instrument::instantiateView( QWidget * _parent)
{
	return new Lv2InstrumentView(this, _parent);
}

Lv2InstrumentView::Lv2InstrumentView(Instrument * _instrument,
		QWidget * _parent) :
	InstrumentView( _instrument, _parent )
{
	//TODO draw UI
}

Lv2InstrumentView::~Lv2InstrumentView()
{

}
