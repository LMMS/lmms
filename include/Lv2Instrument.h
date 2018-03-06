/*
 * Lv2Instrument.h - class for handling Lv2 Instrument plugins
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

#ifndef _LV2_INSTRUMENT_H
#define _LV2_INSTRUMENT_H

#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>

#include "JournallingObject.h"
#include "Lv2Plugin.h"
#include "RemotePlugin.h"


//class Lv2Instrument : public RemotePlugin, public JournallingObject,
  //public Lv2Plugin
//{
  //Q_OBJECT
//public:
	//Lv2Instrument( Model * _parent,
			//const Descriptor::SubPluginFeatures::Key * _key );
	//virtual ~Lv2Instrument();

	//virtual bool processAudioBuffer( sampleFrame * _buf,
							//const fpp_t _frames );

	//virtual EffectControls * controls()
	//{
		//return &m_vstControls;
	//}

	//virtual inline QString publicName() const
	//{
		//return m_plugin->name();
	//}


//private:
	//void openPlugin( const QString & _plugin );
	//void closePlugin();

	//QSharedPointer<VstPlugin> m_plugin;
	//QMutex m_pluginMutex;
	//EffectKey m_key;

	//VstEffectControls m_vstControls;


	//friend class VstEffectControls;
	//friend class VstEffectControlDialog;
	//friend class manageVSTEffectView;

//} ;



#endif
