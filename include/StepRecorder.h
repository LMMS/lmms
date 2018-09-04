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

#ifndef STEP_RECORDER_H
#define STEP_RECORDER_H

#include <QTime>
#include <QTimer>
#include <QObject>
#include <QKeyEvent>

#include "Note.h"
#include "lmms_basics.h"
#include "Pattern.h"

class PianoRoll;
class StepRecorderWidget;

class StepRecorder : public QObject
{
	Q_OBJECT

	public:
	StepRecorder(PianoRoll& pianoRoll, StepRecorderWidget& stepRecorderWidget);

	void initialize();
	void start(const MidiTime& currentPosition,const MidiTime& stepLength);
	void stop();
	void notePressed(const Note & n);
	void noteReleased(const Note & n);
	bool keyPressEvent(QKeyEvent* ke);
	bool mousePressEvent(QMouseEvent* ke);
	void setCurrentPattern(Pattern* newPattern);
	void setStepsLength(const MidiTime& newLength);

	QVector<Note*> getCurStepNotes();

	bool isRecording() const
	{
		return m_isRecording;
	}

	QColor curStepNoteColor() const
	{
		return QColor(245,3,139); // radiant pink
	}

	private slots:
	void removeNotesReleasedForTooLong();

	private:
	void stepForwards();
	void stepBackwards();

	void applyStep();
	void dismissStep();
	void prepareNewStep();

	MidiTime getCurStepEndPos();

	void updateCurStepNotes();
	void updateWidget();

	bool allCurStepNotesReleased();

	PianoRoll& m_pianoRoll;
	StepRecorderWidget& m_stepRecorderWidget;

	bool m_isRecording = false;
	MidiTime m_curStepStartPos = 0;
	MidiTime m_curStepEndPos = 0;

	MidiTime m_stepsLength;
	MidiTime m_curStepLength; // current step length refers to the step currently recorded. it may defer from m_stepsLength
							  // since the user can make current step larger

	QTimer m_updateReleasedTimer;

	Pattern* m_pattern;

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
		QTime releasedTimer;
	} ;

	QVector<StepNote*> m_curStepNotes; // contains the current recorded step notes (i.e. while user still press the notes; before they are applied to the pattern)

	StepNote* findCurStepNote(const int key);

	bool m_isStepInProgress = false;
};

#endif //STEP_RECORDER_H