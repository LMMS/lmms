/*
 * Timeline.h
 *
 * Copyright (c) 2023 Dominic Clark
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
 */

#ifndef LMMS_TIMELINE_H
#define LMMS_TIMELINE_H

#include <QObject>

#include "JournallingObject.h"
#include "TimePos.h"

namespace lmms {

class Timeline : public QObject, public JournallingObject
{
	Q_OBJECT

public:
	enum class StopBehaviour
	{
		BackToZero,
		BackToStart,
		KeepPosition
	};

	auto loopBegin() const -> TimePos { return m_loopBegin; }
	auto loopEnd() const -> TimePos { return m_loopEnd; }
	auto loopEnabled() const -> bool { return m_loopEnabled; }

	void setLoopBegin(TimePos begin);
	void setLoopEnd(TimePos end);
	void setLoopPoints(TimePos begin, TimePos end);
	void setLoopEnabled(bool enabled);

	auto playStartPosition() const -> TimePos { return m_playStartPosition; }
	auto stopBehaviour() const -> StopBehaviour { return m_stopBehaviour; }

	void setPlayStartPosition(TimePos position) { m_playStartPosition = position; }
	void setStopBehaviour(StopBehaviour behaviour);

	auto nodeName() const -> QString override { return "timeline"; }

signals:
	void loopEnabledChanged(bool enabled);
	void stopBehaviourChanged(lmms::Timeline::StopBehaviour behaviour);

protected:
	void saveSettings(QDomDocument& doc, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

private:
	TimePos m_loopBegin = TimePos{0};
	TimePos m_loopEnd = TimePos{DefaultTicksPerBar};
	bool m_loopEnabled = false;

	StopBehaviour m_stopBehaviour = StopBehaviour::BackToStart;
	TimePos m_playStartPosition = TimePos{-1};
};

} // namespace lmms

#endif // LMMS_TIMELINE_H
