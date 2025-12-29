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

#include "AudioEngine.h"
#include "Engine.h"
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

	auto pos() const -> const TimePos& { return m_pos; }

	auto ticks() const -> tick_t { return m_pos.getTicks(); }

	//! Forcefully sets the current ticks, resets the frame offset, and sets the elapsed seconds based on the global position (ignoring potential mid-song tempo changes)
	//! This function will emit the `positionJumped` signal to allow other widgets to update accordingly
	void setTicks(tick_t ticks)
	{
		m_pos.setTicks(ticks);
		m_frameOffset = 0;
		m_elapsedSeconds = ticks * Engine::framesPerTick() / Engine::audioEngine()->outputSampleRate();
		emit positionJumped();
		emit positionChanged();
	}

	//! Advances the current timeline position by a certain number of ticks, in addition to updating the elapsed time based on the current tempo.
	void incrementTicks(tick_t increment)
	{
		m_pos.setTicks(ticks() + increment);
		m_elapsedSeconds += increment * Engine::framesPerTick() / Engine::audioEngine()->outputSampleRate();
		emit positionChanged();
	}

	auto frameOffset() const -> float { return m_frameOffset; }
	void setFrameOffset(const float frame) { m_frameOffset = frame; }

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

	auto getElapsedSeconds() const -> double { return m_elapsedSeconds + frameOffset() / Engine::audioEngine()->outputSampleRate(); }

	auto nodeName() const -> QString override { return "timeline"; }

signals:
	void loopEnabledChanged(bool enabled);
	void stopBehaviourChanged(lmms::Timeline::StopBehaviour behaviour);
	void positionChanged();
	void positionJumped();

protected:
	void saveSettings(QDomDocument& doc, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

private:
	TimePos m_loopBegin = TimePos{0};
	TimePos m_loopEnd = TimePos{DefaultTicksPerBar};
	bool m_loopEnabled = false;
	TimePos m_pos = TimePos{0};

	float m_frameOffset = 0;

	double m_elapsedSeconds = 0;

	StopBehaviour m_stopBehaviour = StopBehaviour::BackToStart;
	TimePos m_playStartPosition = TimePos{-1};
};

} // namespace lmms

#endif // LMMS_TIMELINE_H
