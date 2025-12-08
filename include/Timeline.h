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

	void setPlayPos(TimePos pos)
	{
		m_pos = pos;
		// Calling `setPlayPos` is treated as a jump, so the frame offset is reset
		m_frameOffset = 0;
		emit positionChanged();
		emit positionJumped();
	}

	auto ticks() const -> tick_t { return m_pos.getTicks(); }

	// By default, calling `setTicks` will emit `positionJumped` signals to update everything accordingly
	// However, passing `jumped = false` will instead treat the change in ticks as a continuous increment
	void setTicks(tick_t ticks, bool jumped = true)
	{
		if (jumped)
		{
			m_frameOffset = 0;
			// If the playhead has jumped, reset the elapsed time based on the global offset, ignoring any potnetial tempo automations.
			m_elapsedSeconds = ticks * Engine::framesPerTick() / Engine::audioEngine()->outputSampleRate();
			emit positionJumped();
		}
		else
		{
			// Update the elapsed time based on the delta ticks, to preserve the current total in case there was a tempo change mid-song
			m_elapsedSeconds += (ticks - m_pos.getTicks()) * Engine::framesPerTick() / Engine::audioEngine()->outputSampleRate();
		}
		m_pos.setTicks(ticks);
		emit positionChanged();
	}

	auto frameOffset() const -> f_cnt_t { return m_frameOffset; }
	void setFrameOffset(const f_cnt_t frame) { m_frameOffset = frame; }

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

	f_cnt_t m_frameOffset = 0;

	double m_elapsedSeconds = 0;

	StopBehaviour m_stopBehaviour = StopBehaviour::BackToStart;
	TimePos m_playStartPosition = TimePos{-1};
};

} // namespace lmms

#endif // LMMS_TIMELINE_H
