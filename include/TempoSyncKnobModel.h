/*
 * TempoSyncKnobModel.h - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005-2008 Danny McRae <khjklujn/at/yahoo.com>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_TEMPO_SYNC_KNOB_MODEL_H
#define LMMS_TEMPO_SYNC_KNOB_MODEL_H

#include "MeterModel.h"

class QAction;

namespace lmms
{

namespace gui
{

class TempoSyncKnob;

} // namespace gui


class LMMS_EXPORT TempoSyncKnobModel : public FloatModel
{
	Q_OBJECT
	MODEL_IS_VISITABLE
public:
	enum class SyncMode
	{
		None,
		DoubleWholeNote,
		WholeNote,
		HalfNote,
		QuarterNote,
		EighthNote,
		SixteenthNote,
		ThirtysecondNote,
		Custom
	} ;

	TempoSyncKnobModel(const float val, const float min, const float max, const float step,
					const float scale, Model* parent, const QString& displayName = QString());
	~TempoSyncKnobModel() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& self, const QString& name) override;
	void loadSettings(const QDomElement& self, const QString& name) override;

	SyncMode syncMode() const
	{
		return m_tempoSyncMode;
	}

	void setSyncMode(SyncMode newMode);

	float scale() const
	{
		return m_scale;
	}

	void setScale(float newScale);

	MeterModel& getCustomMeterModel() { return m_custom; }
	MeterModel const& getCustomMeterModel() const { return m_custom; }

signals:
	void syncModeChanged(lmms::TempoSyncKnobModel::SyncMode newMode);
	void scaleChanged(float newScale);


public slots:
	inline void disableSync()
	{
		setTempoSync(SyncMode::None);
	}
	void setTempoSync(SyncMode noteType);
	void setTempoSync(QAction* item);


protected slots:
	void calculateTempoSyncTime(lmms::bpm_t bpm);
	void updateCustom();


private:
	SyncMode m_tempoSyncMode;
	SyncMode m_tempoLastSyncMode;
	float m_scale;

	MeterModel m_custom;


	friend class gui::TempoSyncKnob;

} ;

} // namespace lmms

#endif // LMMS_TEMPO_SYNC_KNOB_MODEL_H
