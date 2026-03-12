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

#ifndef LMMS_STEP_RECORDER_H
#define LMMS_STEP_RECORDER_H

#include <QElapsedTimer>
#include <QTimer>
#include <QObject>

#include "Note.h"

class QKeyEvent;
class QMouseEvent;

namespace lmms
{

class MidiClip;

namespace gui
{
class PianoRoll;
class StepRecorderWidget;
} // namespace gui

class StepRecorder : public QObject
{
	Q_OBJECT

	public:
	StepRecorder(gui::PianoRoll& pianoRoll, gui::StepRecorderWidget& stepRecorderWidget);

	void initialize();
	void start(const TimePos& currentPosition,const TimePos& stepLength);
	void stop();
	void notePressed(const Note & n);
	void noteReleased(const Note & n);
	bool keyPressEvent(QKeyEvent* ke);
	bool mousePressEvent(QMouseEvent* ke);
	void setCurrentMidiClip(MidiClip* newMidiClip);
	void setStepsLength(const TimePos& newLength);

	std::vector<Note*> getCurStepNotes();

	bool isRecording() const
	{
		return m_isRecording;
	}

	private slots:
	void removeNotesReleasedForTooLong();

	private:
	void stepForwards();
	void stepBackwards();

	void applyStep();
	void dismissStep();
	void prepareNewStep();

	TimePos getCurStepEndPos();

	void updateCurStepNotes();
	void updateWidget();

	bool allCurStepNotesReleased();

	gui::PianoRoll& m_pianoRoll;
	gui::StepRecorderWidget& m_stepRecorderWidget;

	bool m_isRecording = false;
	TimePos m_curStepStartPos = 0;
	TimePos m_curStepEndPos = 0;

	TimePos m_stepsLength;
	TimePos m_curStepLength; // current step length refers to the step currently recorded. it may defer from m_stepsLength
							  // since the user can make current step larger

	QTimer m_updateReleasedTimer;

	MidiClip* m_midiClip;

	class StepNote
	{
		public:
		StepNote(const Note & note) : m_note(note), m_pressed(true) {};

		void setPressed()
		{
			m_pressed = true;
		}

		void setReleased()
		{
			m_pressed = false;
			releasedTimer.start();
		}

		int timeSinceReleased()
		{
			return releasedTimer.elapsed();
		}

		bool isPressed() const
		{
			return m_pressed;
		}

		bool isReleased() const
		{
			return !m_pressed;
		}

		Note m_note;

		private:
		bool m_pressed;
		QElapsedTimer releasedTimer;
	} ;

	std::vector<StepNote*> m_curStepNotes; // contains the current recorded step notes (i.e. while user still press the notes; before they are applied to the clip)

	StepNote* findCurStepNote(const int key);

	bool m_isStepInProgress = false;
};


} // namespace lmms

#endif // LMMS_STEP_RECORDER_H
