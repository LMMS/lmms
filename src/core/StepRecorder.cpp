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

//for debugging: uncomment to create a win32 console, and enable DBG_PRINT
//#define DEBUG_STEP_RECORDER

#ifdef DEBUG_STEP_RECORDER
	#define DEBUG_CREATE_WIN32_CONSOLE
	#define DRBUG_ENABLE_PRINTS
#endif //DEBUG_STEP_RECORDER

#ifdef DEBUG_CREATE_WIN32_CONSOLE
#include <windows.h>
#endif

#ifdef DRBUG_ENABLE_PRINTS
#include <cstdio>
	#define DBG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
	#define DBG_PRINT(fmt, ...) 
#endif

const int REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS = 70; 


StepRecorder::StepRecorder(PianoRoll& pianoRoll, StepRecorderWidget& stepRecorderWidget):
	m_pianoRoll(pianoRoll),
	m_stepRecorderWidget(stepRecorderWidget)
{
 	m_stepRecorderWidget.hide();	

#if defined(DEBUG_CREATE_WIN32_CONSOLE) && defined(_WIN32)
	//create win32 console and attach it for output 
	if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
	{
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}	
#endif
}

void StepRecorder::initialize()
{
	connect(&m_updateReleasedTimer, SIGNAL( timeout() ), this, SLOT( removeNotesReleasedForTooLong() ) );
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

	prepareNewStep();
}

void StepRecorder::stop()
{
	m_stepRecorderWidget.hide();	
	m_isRecording = false;
}

void StepRecorder::notePressed(const Note & n)
{
	const int key = n.key();

	if(m_pressedNotes.contains(key))
	{
		//if note already pressed, return. this should not happen and this condition added just for robustness.
		//in case this turns to be a valid case in the future, we should update note's velocity instead
		return ;	
	}

	//if this is the first pressed note in step, advance position
	if(!m_isStepInProgress)
	{
		m_isStepInProgress = true;		

		//move curser one step forwards 
		stepForwards();
	}

	Note noteToAdd(m_stepsLength, m_curStepStartPos, n.key(), n.getVolume(), n.getPanning() );			
	
	Note* newNote = m_pattern->addNote( noteToAdd, false);
	m_pianoRoll.update();

	//remove note from released if it is in there
	if(m_releasedNotes.contains(key))
	{
		ReleasedNote& rn = m_releasedNotes[key];
		m_pattern->removeNote(rn.m_note);
		m_releasedNotes.remove(key);
	}

	//add note to pressed list
	m_pressedNotes[key] = newNote;
}
void StepRecorder::noteReleased(const Note & n)
{
	DBG_PRINT("%s: key[%d]... \n", __FUNCTION__, n.key());

	const int key = n.key();
	if(m_pressedNotes.contains(key))
	{
		//move note from pressed list to released list
		Note* note = m_pressedNotes[key];
		m_pressedNotes.remove(key);

		ReleasedNote& rn = m_releasedNotes[key];
		rn.setNote(note);

		//if m_updateReleasedTimer is not already active, activate it 
		//(when activated, the timer will re-set itself as long as there are notes in m_releasedNotes
		if(!m_updateReleasedTimer.isActive())
		{
			m_updateReleasedTimer.start(REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS);
		}

		DBG_PRINT("%s: key[%d] pressed->released \n", __FUNCTION__, key);

		//check if all note are released, apply notes to pattern(or dimiss if length is zero) and prepare to record next step
		if(m_pressedNotes.count() == 0)
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

		case Qt::Key_Escape:
		{
			if(m_isStepInProgress)
			{
				dismissStep();
			}

			event_handled = true;
			break;			
		}
	}

	if(!event_handled && isKeyEventDisallowedDuringStepRecording(ke))
	{
		//TODO: display message to user that this key event is not allowed during step recording
		event_handled = true;
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

void StepRecorder::removeCurStepNotesFromPattern(QList<Note>* removedNotesCopy)
{
	// Removing notes from pattern also deletes their instances. 
	// in order to recover them later, they a copy of them is added to an optional list
	foreach (Note* note, m_pressedNotes)
	{
		if(removedNotesCopy != nullptr)
		{
			removedNotesCopy->append(*note);
		}

		m_pattern->removeNote(note);
	}

	foreach (const ReleasedNote& rn, m_releasedNotes)
	{
		if(removedNotesCopy != nullptr)
		{
			removedNotesCopy->append(*rn.m_note);
		}

		m_pattern->removeNote(rn.m_note);
	}
}

void StepRecorder::applyStep()
{
	DBG_PRINT("%s\n", __FUNCTION__);
	
	// in order to allow "undo" of this step, we remove all notes from pattern, add checkpoint and re-add them
	// (an alternative would be to add checkpoint in prepareNewStep(), but then, in case the step is dismissed
	// the added checkpoint would be "empty" (i.e. not related to any modification))
	QList<Note> removedNotesCopy;
	removeCurStepNotesFromPattern(&removedNotesCopy);

	m_pattern->addJournalCheckPoint();

	foreach (const Note& note, removedNotesCopy)
	{
		m_pattern->addNote(note, false);
	}

	m_pattern->rearrangeAllNotes();
	m_pattern->updateLength();
	m_pattern->dataChanged();
	Engine::getSong()->setModified();

	prepareNewStep();
}

void StepRecorder::dismissStep()
{
	DBG_PRINT("%s\n", __FUNCTION__);

	if(!m_isStepInProgress)
	{
		return;
	}

	removeCurStepNotesFromPattern(nullptr);
	prepareNewStep();
}

void StepRecorder::prepareNewStep()
{
	DBG_PRINT("%s\n", __FUNCTION__);
	
	m_releasedNotes.clear();
	m_pressedNotes.clear();
	
	m_isStepInProgress = false;

	m_curStepStartPos = getCurStepEndPos();
	m_curStepLength = 0;

	updateWidget();
}

void StepRecorder::setCurrentPattern( Pattern* newPattern )
{
	DBG_PRINT("%s\n", __FUNCTION__);

    if(m_pattern != NULL && m_pattern != newPattern)
    {
        // remove any unsaved notes from old pattern
        dismissStep();
    }

    m_pattern = newPattern;
}

void StepRecorder::removeNotesReleasedForTooLong()
{
	DBG_PRINT("%s\n", __FUNCTION__);

	if(m_releasedNotes.count() == 0)
	{
		m_updateReleasedTimer.stop();
	}
	else 
	{
		int nextTimout = INT_MAX;
		bool notesRemoved = false;

		QMutableHashIterator<int, ReleasedNote> itr(m_releasedNotes);
		while (itr.hasNext()) 
		{
			itr.next();
			ReleasedNote& rn = itr.value();

			DBG_PRINT("key[%d]: timeSinceReleased:[%d]\n", rn.m_note->key(), rn.timeSinceReleased());

			const int timeSinceReleased = rn.timeSinceReleased(); // capture value to avoid wraparound when calculting nextTimout
			if (timeSinceReleased >= REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS)
			{
				DBG_PRINT("removed...\n");

				m_pattern->removeNote(rn.m_note);
				itr.remove();
				notesRemoved = true;
			}
			else 
			{
				nextTimout = min(nextTimout, REMOVE_RELEASED_NOTE_TIME_THRESHOLD_MS - timeSinceReleased);
			}
		}

		if(notesRemoved)
		{
			m_pianoRoll.update();
		}

		if(nextTimout != INT_MAX)
		{
			m_updateReleasedTimer.start(nextTimout);
		}
	}

}

MidiTime StepRecorder::getCurStepEndPos()
{
	return m_curStepStartPos + m_curStepLength;
}

void StepRecorder::updateCurStepNotes()
{
	foreach (Note* note, m_pressedNotes)
	{
		note->setLength(m_curStepLength);
		note->setPos(m_curStepStartPos);
	}

	//need to update also released notes, since they might be recorded if not released for too long	
	foreach (const ReleasedNote& rn, m_releasedNotes)
	{
		rn.m_note->setLength(m_curStepLength);
		rn.m_note->setPos(m_curStepStartPos);
	}
}

void StepRecorder::updateWidget()
{
	m_stepRecorderWidget.setStartPosition(m_curStepStartPos);
	m_stepRecorderWidget.setEndPosition(getCurStepEndPos());
	m_stepRecorderWidget.setStepsLength(m_stepsLength);
}

//list key pressed to be ignored during step recording (mostly - edit note actions)
//having this hardcoded is not very elegant nor flexible (consider adding new edot action to piano roll - developer must add to this list as well to be ignored)
//a better way would be to have an object to handle all "key to action" event and query it on runtime based (i.e. m_pianoRoll.keyEventHandler.getEditKeyEvents()); 
bool StepRecorder::isKeyEventDisallowedDuringStepRecording(QKeyEvent* ke)
{
	int key = ke->key();

	if((key == Qt::Key_Up || key == Qt::Key_Down) && (ke->modifiers() != Qt::NoModifier))
	{
		//prevent editing but allow scrolling (no modifiers)
		return true;
	}
	else if (key == Qt::Key_A && ke->modifiers() & Qt::ControlModifier)
	{
		return true;
	}
	else if (key == Qt::Key_Control || key == Qt::Key_Delete)
	{
		return true;
	}

	return false;
}