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
#include "StepRecorderWidget.h"
#include "PianoRoll.h"

#include <QPainter>

#include <climits>
using std::min;
using std::max;

const int REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS = 70;

StepRecorder::StepRecorder(PianoRoll& pianoRoll, StepRecorderWidget& stepRecorderWidget):
	m_pianoRoll(pianoRoll),
	m_stepRecorderWidget(stepRecorderWidget)
{
	m_stepRecorderWidget.hide();
}

void StepRecorder::initialize()
{
	connect(&m_updateReleasedTimer, SIGNAL(timeout()), this, SLOT(removeNotesReleasedForTooLong()));
}

void StepRecorder::start(const MidiTime& currentPosition, const MidiTime& stepLength)
{
	m_isRecording = true;

	setStepsLength(stepLength);

	// quantize current position to get start recording position
	const int q = m_pianoRoll.quantization();
	const int curPosTicks = currentPosition.getTicks();
	const int QuantizedPosTicks = (curPosTicks / q) * q;
	const MidiTime& QuantizedPos = MidiTime(QuantizedPosTicks);

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

		//move curser one step forwards
		stepForwards();
	}

	StepNote* stepNote = findCurStepNote(n.key());
	if(stepNote == nullptr)
	{
		m_curStepNotes.append(new StepNote(Note(m_curStepLength, m_curStepStartPos, n.key(), n.getVolume(), n.getPanning())));
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

		//check if all note are released, apply notes to pattern(or dimiss if length is zero) and prepare to record next step
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

void StepRecorder::setStepsLength(const MidiTime& newLength)
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

QVector<Note*> StepRecorder::getCurStepNotes()
{
	QVector<Note*> notes;

	if(m_isStepInProgress)
	{
		for(StepNote* stepNote: m_curStepNotes)
		{
			notes.append(&stepNote->m_note);
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
	m_pattern->addJournalCheckPoint();

	for (const StepNote* stepNote : m_curStepNotes)
	{
		m_pattern->addNote(stepNote->m_note, false);
	}

	m_pattern->rearrangeAllNotes();
	m_pattern->updateLength();
	m_pattern->dataChanged();
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

void StepRecorder::setCurrentPattern( Pattern* newPattern )
{
	if(m_pattern != NULL && m_pattern != newPattern)
	{
		dismissStep();
	}

	m_pattern = newPattern;
}

void StepRecorder::removeNotesReleasedForTooLong()
{
	int nextTimout = std::numeric_limits<int>::max();
	bool notesRemoved = false;

	QMutableVectorIterator<StepNote*> itr(m_curStepNotes);
	while (itr.hasNext())
	{
		StepNote* stepNote = itr.next();

		if(stepNote->isReleased())
		{
			const int timeSinceReleased = stepNote->timeSinceReleased(); // capture value to avoid wraparound when calculting nextTimout
			if (timeSinceReleased >= REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS)
			{
				delete stepNote;
				itr.remove();
				notesRemoved = true;
			}
			else
			{
				nextTimout = min(nextTimout, REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS - timeSinceReleased);
			}
		}
	}

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
		// no released note found for next timout, stop timer
		m_updateReleasedTimer.stop();
	}
}

MidiTime StepRecorder::getCurStepEndPos()
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
