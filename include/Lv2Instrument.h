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

#ifndef LV2_INSTRUMENT_H
#define LV2_INSTRUMENT_H

#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>

#include "Instrument.h"
#include "InstrumentTrack.h"
#include "InstrumentView.h"
#include "JournallingObject.h"
#include "Lv2PluginInfo.h"
#include "RemotePlugin.h"


class Lv2Instrument : public Instrument
{
	Q_OBJECT
public:
	explicit Lv2Instrument( InstrumentTrack * _instrument_track );
	virtual ~Lv2Instrument();

	//virtual bool processAudioBuffer( sampleFrame * _buf,
							//const fpp_t _frames );

private:
	//void openPlugin( const QString & _plugin );
	//void closePlugin();

	QMutex m_pluginMutex;
} ;

class Lv2InstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	Lv2InstrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~Lv2InstrumentView();

private:
	//virtual void modelChanged();

protected slots:
	//void updateKnobHint();
};



#endif
