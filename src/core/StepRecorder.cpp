/*
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

#include "StepRecorder.h"

#include <QKeyEvent>

#include "MidiClip.h"
#include "StepRecorderWidget.h"
#include "PianoRoll.h"


namespace lmms
{


using std::min;
using std::max;

const int REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS = 70;

StepRecorder::StepRecorder(gui::PianoRoll& pianoRoll, gui::StepRecorderWidget& stepRecorderWidget):
	m_pianoRoll(pianoRoll),
	m_stepRecorderWidget(stepRecorderWidget),
	m_midiClip(nullptr)
{
	m_stepRecorderWidget.hide();
}

void StepRecorder::initialize()
{
	connect(&m_updateReleasedTimer, SIGNAL(timeout()), this, SLOT(removeNotesReleasedForTooLong()));
}

void StepRecorder::start(const TimePos& currentPosition, const TimePos& stepLength)
{
	m_isRecording = true;

	setStepsLength(stepLength);

	// quantize current position to get start recording position
	const int q = m_pianoRoll.quantization();
	const int curPosTicks = currentPosition.getTicks();
	const int QuantizedPosTicks = (curPosTicks / q) * q;
	const TimePos& QuantizedPos = TimePos(QuantizedPosTicks);

	m_curStepStartPos = QuantizedPos;
	m_curStepLength = 0;

	m_stepRecorderWidget.show();

	m_stepRecorderWidget.showHint();

	prepareNewStep();
}

void StepRecorder::stop()
{
	m_stepRecorderWidget.hide();
	m_isRecording = false;
}

void StepRecorder::notePressed(const Note & n)
{
	//if this is the first pressed note in step, advance position
	if(!m_isStepInProgress)
	{
		m_isStepInProgress = true;

		//move cursor one step forwards
		stepForwards();
	}

	StepNote* stepNote = findCurStepNote(n.key());
	if(stepNote == nullptr)
	{
		m_curStepNotes.push_back(new StepNote(Note(m_curStepLength, m_curStepStartPos, n.key(), n.getVolume(), n.getPanning())));
		m_pianoRoll.update();
	}
	else if (stepNote->isReleased())
	{
		stepNote->setPressed();
	}
}

void StepRecorder::noteReleased(const Note & n)
{
	StepNote* stepNote = findCurStepNote(n.key());

	if(stepNote != nullptr && stepNote->isPressed())
	{
		stepNote->setReleased();

		//if m_updateReleasedTimer is not already active, activate it
		//(when activated, the timer will re-set itself as long as there are released notes)
		if(!m_updateReleasedTimer.isActive())
		{
			m_updateReleasedTimer.start(REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS);
		}

		//check if all note are released, apply notes to clips (or dismiss if length is zero) and prepare to record next step
		if(allCurStepNotesReleased())
		{
			if(m_curStepLength > 0)
			{
				applyStep();
			}
			else
			{
				dismissStep();
			}
		}
	}
}

bool StepRecorder::keyPressEvent(QKeyEvent* ke)
{
	bool event_handled = false;

	switch(ke->key())
	{
		case Qt::Key_Right:
		{
			if(!ke->isAutoRepeat())
			{
				stepForwards();
			}
			event_handled = true;
			break;
		}

		case Qt::Key_Left:
		{
			if(!ke->isAutoRepeat())
			{
				stepBackwards();
			}
			event_handled = true;
			break;
		}
	}

	return event_handled;
}

void StepRecorder::setStepsLength(const TimePos& newLength)
{
	if(m_isStepInProgress)
	{
		//update current step length by the new amount : (number_of_steps * newLength)
		m_curStepLength = (m_curStepLength / m_stepsLength) * newLength;

		updateCurStepNotes();
	}

	m_stepsLength = newLength;

	updateWidget();
}

std::vector<Note*> StepRecorder::getCurStepNotes()
{
	std::vector<Note*> notes;

	if(m_isStepInProgress)
	{
		for (StepNote* stepNote: m_curStepNotes)
		{
			notes.push_back(&stepNote->m_note);
		}
	}

	return notes;
}

void StepRecorder::stepForwards()
{
	if(m_isStepInProgress)
	{
		m_curStepLength += m_stepsLength;

		updateCurStepNotes();
	}
	else
	{
		m_curStepStartPos += m_stepsLength;
	}

	updateWidget();
}

void StepRecorder::stepBackwards()
{
	if(m_isStepInProgress)
	{
		if(m_curStepLength > 0)
		{
			m_curStepLength = max(m_curStepLength - m_stepsLength, 0);
		}
		else
		{
			//if length is already zero - move starting position backwards
			m_curStepStartPos = max(m_curStepStartPos - m_stepsLength, 0);
		}

		updateCurStepNotes();
	}
	else
	{
		m_curStepStartPos = max(m_curStepStartPos - m_stepsLength, 0);
	}

	updateWidget();
}

void StepRecorder::applyStep()
{
	m_midiClip->addJournalCheckPoint();

	for (const StepNote* stepNote : m_curStepNotes)
	{
		m_midiClip->addNote(stepNote->m_note, false);
	}

	m_midiClip->rearrangeAllNotes();
	m_midiClip->updateLength();
	m_midiClip->dataChanged();
	Engine::getSong()->setModified();

	prepareNewStep();
}

void StepRecorder::dismissStep()
{
	if(!m_isStepInProgress)
	{
		return;
	}

	prepareNewStep();
}

void StepRecorder::prepareNewStep()
{
	for(StepNote* stepNote : m_curStepNotes)
	{
		delete stepNote;
	}
	m_curStepNotes.clear();

	m_isStepInProgress = false;

	m_curStepStartPos = getCurStepEndPos();
	m_curStepLength = 0;

	updateWidget();
}

void StepRecorder::setCurrentMidiClip( MidiClip* newMidiClip )
{
	if(m_midiClip != nullptr && m_midiClip != newMidiClip)
	{
		dismissStep();
	}

	m_midiClip = newMidiClip;
}

void StepRecorder::removeNotesReleasedForTooLong()
{
	int nextTimout = std::numeric_limits<int>::max();
	bool notesRemoved = false;

	for (const auto& stepNote : m_curStepNotes)
	{
		if (stepNote->isReleased())
		{
			const int timeSinceReleased = stepNote->timeSinceReleased(); // capture value to avoid wraparound when calculating nextTimout
			if (timeSinceReleased >= REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS)
			{
				notesRemoved = true;
			}
			else
			{
				nextTimout = min(nextTimout, REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS - timeSinceReleased);
			}
		}
	}

	m_curStepNotes.erase(std::remove_if(m_curStepNotes.begin(), m_curStepNotes.end(), [](auto stepNote)
	{
		bool shouldRemove = stepNote->isReleased() && stepNote->timeSinceReleased() >= REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS;
		if (shouldRemove)
		{
			delete stepNote;
		}

		return shouldRemove;
	}), m_curStepNotes.end());

	if(notesRemoved)
	{
		m_pianoRoll.update();
	}

	if(nextTimout != std::numeric_limits<int>::max())
	{
		m_updateReleasedTimer.start(nextTimout);
	}
	else
	{
		// no released note found for next timeout, stop timer
		m_updateReleasedTimer.stop();
	}
}

TimePos StepRecorder::getCurStepEndPos()
{
	return m_curStepStartPos + m_curStepLength;
}

void StepRecorder::updateCurStepNotes()
{
	for (StepNote* stepNote : m_curStepNotes)
	{
		stepNote->m_note.setLength(m_curStepLength);
		stepNote->m_note.setPos(m_curStepStartPos);
	}
}

void StepRecorder::updateWidget()
{
	m_stepRecorderWidget.setStartPosition(m_curStepStartPos);
	m_stepRecorderWidget.setEndPosition(getCurStepEndPos());
	m_stepRecorderWidget.setStepsLength(m_stepsLength);
}

bool StepRecorder::allCurStepNotesReleased()
{
	for (const StepNote* stepNote : m_curStepNotes)
	{
		if(stepNote->isPressed())
		{
			return false;
		}
	}

	return true;
}

StepRecorder::StepNote* StepRecorder::findCurStepNote(const int key)
{
	for (StepNote* stepNote : m_curStepNotes)
	{
		if(stepNote->m_note.key() == key)
		{
			return stepNote;
		}
	}

	return nullptr;
}

} // namespace lmms