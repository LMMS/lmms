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

namespace Lilv {
	struct Instance;
}

class Lv2Instrument : public Instrument
{
	Q_OBJECT
public:
	explicit Lv2Instrument(const Lv2PluginInfo& _pi,
			InstrumentTrack * _it);
	~Lv2Instrument() override;

	void playNote( NotePlayHandle* _note_to_play,
					sampleFrame* _working_buf) override;

	void deleteNotePluginData( NotePlayHandle * _note_to_play ) override;

	void saveSettings(QDomDocument& _doc,
			QDomElement& _parent) override;

	void loadSettings(const QDomElement& _this) override;

	QString nodeName() const override;


private:
	//void openPlugin( const QString & _plugin );
	//void closePlugin();

	const Lv2PluginInfo* m_pluginInfo;
	QMutex m_pluginMutex;
	Lilv::Instance * m_instance;

	PluginView* instantiateView( QWidget * _parent ) override;
};

class Lv2InstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	Lv2InstrumentView( Instrument * _instrument, QWidget * _parent );
	~Lv2InstrumentView();

private:
	//virtual void modelChanged();

protected slots:
	//void updateKnobHint();
};



#endif
