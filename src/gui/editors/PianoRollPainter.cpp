/*
 * PianoRollPainter.cpp - rendering helper for PianoRoll
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 */

#include "PianoRollPainter.h"

#include <QApplication>
#include <QCursor>
#include <QMargins>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOption>

#include "ConfigManager.h"
#include "DetuningHelper.h"
#include "embed.h"
#include "FontHelper.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "MidiClip.h"
#include "Piano.h"
#include "PianoRoll.h"
#include "PianoRollConstants.h"

namespace lmms::gui
{

namespace
{

std::array<QString, 12> s_noteStrings {
	"C", "C\u266F / D\u266D", "D", "D\u266F / E\u266D", "E", "F", "F\u266F / G\u266D",
	"G", "G\u266F / A\u266D", "A", "A\u266F / B\u266D", "B"
};

QString getNoteString(int key)
{
	return s_noteStrings[key % 12] + QString::number(static_cast<int>(FirstOctave + key / KeysPerOctave));
}

} // namespace

PianoRollPainter::PianoRollPainter(PianoRoll& pianoRoll) :
	m_pianoRoll(pianoRoll)
{
}

void PianoRollPainter::paint(QPaintEvent* event)
{
	Q_UNUSED(event)

	const bool drawNoteNames = ConfigManager::inst()->value("ui", "printnotelabels").toInt();

	QStyleOption opt;
	opt.initFrom(&m_pianoRoll);
	QPainter p(&m_pianoRoll);
	m_pianoRoll.style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, &m_pianoRoll);

	QBrush bgColor = p.background();

	p.fillRect(0, 0, m_pianoRoll.width(), m_pianoRoll.height(), bgColor);

	if (!m_pianoRoll.hasValidMidiClip())
	{
		const auto icon = embed::getIconPixmap("pr_no_clip");
		const int x = (m_pianoRoll.width() - icon.width()) / 2;
		const int y = (m_pianoRoll.height() - icon.height()) / 2;
		p.drawPixmap(x, y, icon);

		p.setPen(QApplication::palette().color(QPalette::Active, QPalette::Text));
		QRect textRect(0, y + icon.height() + 5, m_pianoRoll.width(), 30);
		p.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop,
			PianoRoll::tr("Double-click on an instrument clip in Song Editor to open it here"));
		return;
	}

	QFont f = p.font();
	const int keyFontSize = m_pianoRoll.m_keyLineHeight * 0.8;
	p.setFont(adjustedToPixelSize(f, keyFontSize));
	QFontMetrics fontMetrics(p.font());
	QRect const boundingRect = fontMetrics.boundingRect(QString("G-1")) + QMargins(0, 0, 1, 0);

	auto xCoordOfTick = [this](int tick)
	{
		return m_pianoRoll.m_whiteKeyWidth +
			((tick - m_pianoRoll.m_currentPosition) * m_pianoRoll.m_ppb / TimePos::ticksPerBar());
	};

	// Order of drawing
	// - vertical quantization lines
	// - piano roll + horizontal key lines
	// - alternating bar colors
	// - vertical beat lines
	// - vertical bar lines
	// - marked semitones
	// - note editing
	// - notes
	// - selection frame
	// - highlight hovered note
	// - note edit area resize bar
	// - cursor mode icon

	if (m_pianoRoll.hasValidMidiClip())
	{
		int pianoAreaHeight = m_pianoRoll.keyAreaBottom() - m_pianoRoll.keyAreaTop();
		m_pianoRoll.m_pianoKeysVisible = pianoAreaHeight / m_pianoRoll.m_keyLineHeight;
		int partialKeyVisible = pianoAreaHeight % m_pianoRoll.m_keyLineHeight;
		// check if we're below the minimum key area size
		if (m_pianoRoll.m_pianoKeysVisible * m_pianoRoll.m_keyLineHeight < KEY_AREA_MIN_HEIGHT)
		{
			m_pianoRoll.m_pianoKeysVisible = KEY_AREA_MIN_HEIGHT / m_pianoRoll.m_keyLineHeight;
			partialKeyVisible = KEY_AREA_MIN_HEIGHT % m_pianoRoll.m_keyLineHeight;
			// if we have a partial key, just show it
			if (partialKeyVisible > 0)
			{
				m_pianoRoll.m_pianoKeysVisible += 1;
				partialKeyVisible = 0;
			}
			// have to modify the notes edit area height instead
			m_pianoRoll.m_notesEditHeight = m_pianoRoll.height() - (m_pianoRoll.m_pianoKeysVisible * m_pianoRoll.m_keyLineHeight)
				- PR_TOP_MARGIN - PR_BOTTOM_MARGIN;
		}
		// check if we're trying to show more keys than available
		else if (m_pianoRoll.m_pianoKeysVisible >= NumKeys)
		{
			m_pianoRoll.m_pianoKeysVisible = NumKeys;
			// have to modify the notes edit area height instead
			m_pianoRoll.m_notesEditHeight = m_pianoRoll.height() - (NumKeys * m_pianoRoll.m_keyLineHeight) -
				PR_TOP_MARGIN - PR_BOTTOM_MARGIN;
			partialKeyVisible = 0;
		}
		int topKey = std::clamp(m_pianoRoll.m_startKey + m_pianoRoll.m_pianoKeysVisible - 1, 0, NumKeys - 1);
		int topNote = topKey % KeysPerOctave;
		// if not resizing the note edit area, we can change m_notesEditHeight
		if (m_pianoRoll.m_action != PianoRoll::Action::ResizeNoteEditArea && partialKeyVisible != 0)
		{
			// calculate the height change adding and subtracting the partial key
			int noteAreaPlus = (m_pianoRoll.m_notesEditHeight + partialKeyVisible) - m_pianoRoll.m_userSetNotesEditHeight;
			int noteAreaMinus = m_pianoRoll.m_userSetNotesEditHeight - (m_pianoRoll.m_notesEditHeight - partialKeyVisible);
			// if adding the partial key to height is more distant from the set height
			// we want to subtract the partial key
			if (noteAreaPlus > noteAreaMinus)
			{
				m_pianoRoll.m_notesEditHeight -= partialKeyVisible;
				// since we're adding a partial key, we add one to the number visible
				m_pianoRoll.m_pianoKeysVisible += 1;
			}
			else
			{
				// otherwise we add height
				m_pianoRoll.m_notesEditHeight += partialKeyVisible;
			}
		}
		int x, q = m_pianoRoll.quantization(), tick;

		// draw vertical quantization lines
		// If we're over 100% zoom, we allow all quantization level grids
		if (m_pianoRoll.m_zoomingModel.value() <= 3)
		{
			// we're under 100% zoom
			// allow quantization grid up to 1/24 for triplets
			if (q % 3 != 0 && q < 8) { q = 8; }
			// allow quantization grid up to 1/32 for normal notes
			else if (q < 6) { q = 6; }
		}

		p.setPen(m_pianoRoll.m_lineColor);
		for (tick = m_pianoRoll.m_currentPosition - m_pianoRoll.m_currentPosition % q,
			x = xCoordOfTick(tick);
			x <= m_pianoRoll.width();
			tick += q, x = xCoordOfTick(tick))
		{
			p.drawLine(x, m_pianoRoll.keyAreaTop(), x, m_pianoRoll.noteEditBottom());
		}

		// draw horizontal grid lines and piano notes
		p.setClipRect(0, m_pianoRoll.keyAreaTop(), m_pianoRoll.width(), m_pianoRoll.keyAreaBottom() - m_pianoRoll.keyAreaTop());
		// the first grid line from the top Y position
		int gridLineY = m_pianoRoll.keyAreaTop() + m_pianoRoll.m_keyLineHeight - 1;

		// lambda function for returning the height of a key
		auto keyHeight = [&](const int key) -> int
		{
			switch (PianoRoll::prKeyOrder[key % KeysPerOctave])
			{
			case PianoRoll::KeyType::WhiteBig:
				return m_pianoRoll.m_whiteKeyBigHeight;
			case PianoRoll::KeyType::WhiteSmall:
				return m_pianoRoll.m_whiteKeySmallHeight;
			case PianoRoll::KeyType::Black:
				return m_pianoRoll.m_blackKeyHeight;
			}
			return 0;
		};
		// lambda function for returning the distance to the top of a key
		auto gridCorrection = [&](const int key) -> int
		{
			const int keyCode = key % KeysPerOctave;
			switch (PianoRoll::prKeyOrder[keyCode])
			{
			case PianoRoll::KeyType::WhiteBig:
				return m_pianoRoll.m_whiteKeySmallHeight;
			case PianoRoll::KeyType::WhiteSmall:
				if (static_cast<Key>(keyCode) == Key::C || static_cast<Key>(keyCode) == Key::F)
				{
					return m_pianoRoll.m_whiteKeySmallHeight;
				}
				[[fallthrough]];
			case PianoRoll::KeyType::Black:
				return m_pianoRoll.m_blackKeyHeight;
			}
			return 0;
		};
		auto keyWidth = [&](const int key) -> int
		{
			switch (PianoRoll::prKeyOrder[key % KeysPerOctave])
			{
			case PianoRoll::KeyType::WhiteSmall:
			case PianoRoll::KeyType::WhiteBig:
				return m_pianoRoll.m_whiteKeyWidth;
			case PianoRoll::KeyType::Black:
				return m_pianoRoll.m_blackKeyWidth;
			}
			return 0;
		};
		// lambda function to draw a key
		auto drawKey = [&](const int key, const int yb)
		{
			const bool mapped = m_pianoRoll.m_midiClip->instrumentTrack()->isKeyMapped(key);
			const bool pressed = m_pianoRoll.m_midiClip->instrumentTrack()->pianoModel()->isKeyPressed(key);
			const int keyCode = key % KeysPerOctave;
			const int yt = yb - gridCorrection(key);
			const int kh = keyHeight(key);
			const int kw = keyWidth(key);
			p.setPen(QColor(0, 0, 0));
			switch (PianoRoll::prKeyOrder[keyCode])
			{
			case PianoRoll::KeyType::WhiteSmall:
			case PianoRoll::KeyType::WhiteBig:
				if (mapped)
				{
					p.setBrush(pressed ? m_pianoRoll.m_whiteKeyActiveBackground : m_pianoRoll.m_whiteKeyInactiveBackground);
				}
				else
				{
					p.setBrush(m_pianoRoll.m_whiteKeyDisabledBackground);
				}
				break;
			case PianoRoll::KeyType::Black:
				if (mapped)
				{
					p.setBrush(pressed ? m_pianoRoll.m_blackKeyActiveBackground : m_pianoRoll.m_blackKeyInactiveBackground);
				}
				else
				{
					p.setBrush(m_pianoRoll.m_blackKeyDisabledBackground);
				}
			}
			p.drawRect(PIANO_X, yt, kw, kh);
			if (static_cast<Key>(keyCode) == Key::C || (drawNoteNames && Piano::isWhiteKey(key)))
			{
				auto zoomOffset = m_pianoRoll.m_zoomYLevels[m_pianoRoll.m_zoomingYModel.value()] > 1.0f ? 2 : 1;
				QString noteString = getNoteString(key);
				QRect textRect(
					m_pianoRoll.m_whiteKeyWidth - boundingRect.width() - 2,
					yb - m_pianoRoll.m_keyLineHeight + zoomOffset,
					boundingRect.width(),
					boundingRect.height()
				);
				p.setPen(pressed ? m_pianoRoll.m_whiteKeyActiveTextShadow : m_pianoRoll.m_whiteKeyInactiveTextShadow);
				p.drawText(textRect.adjusted(0, 1, 1, 0), Qt::AlignRight | Qt::AlignHCenter, noteString);
				p.setPen(pressed ? m_pianoRoll.m_whiteKeyActiveTextColor : m_pianoRoll.m_whiteKeyInactiveTextColor);
				p.drawText(textRect, Qt::AlignRight | Qt::AlignHCenter, noteString);
			}
		};
		// lambda for drawing the horizontal grid line
		auto drawHorizontalLine = [&](const int key, const int y)
		{
			p.setPen(static_cast<Key>(key % KeysPerOctave) == Key::C ? m_pianoRoll.m_beatLineColor : m_pianoRoll.m_lineColor);
			p.drawLine(m_pianoRoll.m_whiteKeyWidth, y, m_pianoRoll.width(), y);
		};
		// correct y offset of the top key
		switch (PianoRoll::prKeyOrder[topNote])
		{
		case PianoRoll::KeyType::WhiteSmall:
		case PianoRoll::KeyType::WhiteBig:
			break;
		case PianoRoll::KeyType::Black:
			drawKey(topKey + 1, gridLineY - m_pianoRoll.m_keyLineHeight);
		}
		// loop through visible keys
		const int lastKey = qMax(0, topKey - m_pianoRoll.m_pianoKeysVisible);
		for (int key = topKey; key > lastKey; --key)
		{
			if (Piano::isWhiteKey(key))
			{
				drawKey(key, gridLineY);
				drawHorizontalLine(key, gridLineY);
				gridLineY += m_pianoRoll.m_keyLineHeight;
			}
			else
			{
				drawKey(key - 1, gridLineY + m_pianoRoll.m_keyLineHeight);
				drawHorizontalLine(key - 1, gridLineY + m_pianoRoll.m_keyLineHeight);
				drawKey(key, gridLineY);
				drawHorizontalLine(key, gridLineY);
				gridLineY += m_pianoRoll.m_keyLineHeight + m_pianoRoll.m_keyLineHeight;
				--key;
			}
		}

		// don't draw over keys
		p.setClipRect(m_pianoRoll.m_whiteKeyWidth, m_pianoRoll.keyAreaTop(), m_pianoRoll.width(), m_pianoRoll.noteEditBottom() - m_pianoRoll.keyAreaTop());

		// draw alternating shading on bars
		float timeSignature =
			static_cast<float>(Engine::getSong()->getTimeSigModel().getNumerator()) /
			static_cast<float>(Engine::getSong()->getTimeSigModel().getDenominator());
		float zoomFactor = PianoRoll::m_zoomLevels[m_pianoRoll.m_zoomingModel.value()];
		// the bars which disappear at the left side by scrolling
		int leftBars = m_pianoRoll.m_currentPosition * zoomFactor / TimePos::ticksPerBar();
		// iterates the visible bars and draw the shading on uneven bars
		for (int x = m_pianoRoll.m_whiteKeyWidth, barCount = leftBars;
			x < m_pianoRoll.width() + m_pianoRoll.m_currentPosition * zoomFactor / timeSignature;
			x += m_pianoRoll.m_ppb, ++barCount)
		{
			if ((barCount + leftBars) % 2 != 0)
			{
				p.fillRect(x - m_pianoRoll.m_currentPosition * zoomFactor / timeSignature,
					PR_TOP_MARGIN,
					m_pianoRoll.m_ppb,
					m_pianoRoll.height() - (PR_BOTTOM_MARGIN + PR_TOP_MARGIN),
					m_pianoRoll.m_backgroundShade);
			}
		}

		// draw vertical beat lines
		int ticksPerBeat = DefaultTicksPerBar / Engine::getSong()->getTimeSigModel().getDenominator();
		p.setPen(m_pianoRoll.m_beatLineColor);
		for (tick = m_pianoRoll.m_currentPosition - m_pianoRoll.m_currentPosition % ticksPerBeat,
			x = xCoordOfTick(tick);
			x <= m_pianoRoll.width();
			tick += ticksPerBeat, x = xCoordOfTick(tick))
		{
			p.drawLine(x, PR_TOP_MARGIN, x, m_pianoRoll.noteEditBottom());
		}

		// draw vertical bar lines
		p.setPen(m_pianoRoll.m_barLineColor);
		for (tick = m_pianoRoll.m_currentPosition - m_pianoRoll.m_currentPosition % TimePos::ticksPerBar(),
			x = xCoordOfTick(tick);
			x <= m_pianoRoll.width();
			tick += TimePos::ticksPerBar(), x = xCoordOfTick(tick))
		{
			p.drawLine(x, PR_TOP_MARGIN, x, m_pianoRoll.noteEditBottom());
		}

		// draw marked semitones after the grid
		for (x = 0; x < m_pianoRoll.m_markedSemiTones.size(); ++x)
		{
			const int keyNum = m_pianoRoll.m_markedSemiTones.at(x);
			const int y = m_pianoRoll.yCoordOfKey(keyNum);
			if (y >= m_pianoRoll.keyAreaBottom() - 1) { break; }
			p.fillRect(m_pianoRoll.m_whiteKeyWidth + 1,
				y,
				m_pianoRoll.width() - 10,
				m_pianoRoll.m_keyLineHeight,
				m_pianoRoll.m_markedSemitoneColor);
		}
	}

	// reset MIDI clip
	p.setClipRect(0, 0, m_pianoRoll.width(), m_pianoRoll.height());
	// erase the area below the piano, because there might be keys that
	// should be only half-visible
	p.fillRect(QRect(0, m_pianoRoll.keyAreaBottom(),
		m_pianoRoll.m_whiteKeyWidth, m_pianoRoll.noteEditBottom() - m_pianoRoll.keyAreaBottom()), bgColor);

	// display note editing info
	f.setBold(false);
	p.setFont(adjustedToPixelSize(f, SMALL_FONT_SIZE));
	p.setPen(m_pianoRoll.m_noteModeColor);
	p.drawText(QRect(0, m_pianoRoll.keyAreaBottom(),
				m_pianoRoll.m_whiteKeyWidth, m_pianoRoll.noteEditBottom() - m_pianoRoll.keyAreaBottom()),
		Qt::AlignCenter | Qt::TextWordWrap,
		m_pianoRoll.m_nemStr.at(static_cast<int>(m_pianoRoll.m_noteEditMode)) + ":");

	// set clipping area, because we are not allowed to paint over
	// keyboard...
	p.setClipRect(
		m_pianoRoll.m_whiteKeyWidth,
		PR_TOP_MARGIN,
		m_pianoRoll.width() - m_pianoRoll.m_whiteKeyWidth,
		m_pianoRoll.height() - PR_TOP_MARGIN - PR_BOTTOM_MARGIN);

	// setup selection-vars
	int selPosStart = m_pianoRoll.m_selectStartTick;
	int selPosEnd = m_pianoRoll.m_selectStartTick + m_pianoRoll.m_selectedTick;
	if (selPosStart > selPosEnd)
	{
		qSwap<int>(selPosStart, selPosEnd);
	}

	int selKeyStart = m_pianoRoll.m_selectStartKey - m_pianoRoll.m_startKey + 1;
	int selKeyEnd = selKeyStart + m_pianoRoll.m_selectedKeys;
	if (selKeyStart > selKeyEnd)
	{
		qSwap<int>(selKeyStart, selKeyEnd);
	}

	int yBase = m_pianoRoll.keyAreaBottom() - 1;
	if (m_pianoRoll.hasValidMidiClip())
	{
		// following code draws all notes in visible area
		// and the note editing stuff (volume, panning, etc)
		p.setClipRect(
			m_pianoRoll.m_whiteKeyWidth,
			PR_TOP_MARGIN,
			m_pianoRoll.width() - m_pianoRoll.m_whiteKeyWidth,
			m_pianoRoll.height() - PR_TOP_MARGIN);

		const int topKey = qBound(0, m_pianoRoll.m_startKey + m_pianoRoll.m_pianoKeysVisible - 1, NumKeys - 1);
		const int bottomKey = topKey - m_pianoRoll.m_pianoKeysVisible;

		QPolygonF editHandles;

		// Return a note's Y position on the grid
		auto noteYPos = [&](const int key)
		{
			return (topKey - key) * m_pianoRoll.m_keyLineHeight + m_pianoRoll.keyAreaTop() - 1;
		};

		// -- Begin ghost MIDI clip
		if (!m_pianoRoll.m_ghostNotes.empty())
		{
			for (const Note* note : m_pianoRoll.m_ghostNotes)
			{
				int lenTicks = note->length();
				if (lenTicks == 0)
				{
					continue;
				}
				else if (lenTicks < 0)
				{
					lenTicks = 4;
				}

				int posTicks = note->pos();
				int noteWidth = lenTicks * m_pianoRoll.m_ppb / TimePos::ticksPerBar();
				const int x = (posTicks - m_pianoRoll.m_currentPosition) * m_pianoRoll.m_ppb / TimePos::ticksPerBar();
				if (!(x + noteWidth >= 0 && x <= m_pianoRoll.width() - m_pianoRoll.m_whiteKeyWidth))
				{
					continue;
				}

				if (note->key() > bottomKey && note->key() <= topKey)
				{
					m_pianoRoll.drawNoteRect(
						p, x + m_pianoRoll.m_whiteKeyWidth, noteYPos(note->key()), noteWidth,
						note, m_pianoRoll.m_ghostNoteColor, m_pianoRoll.m_ghostNoteTextColor, m_pianoRoll.m_selectedNoteColor,
						m_pianoRoll.m_ghostNoteOpacity, m_pianoRoll.m_ghostNoteBorders, drawNoteNames);
				}
			}
		}

		// -- End ghost MIDI clip
		for (const Note* note : m_pianoRoll.m_midiClip->notes())
		{
			int lenTicks = note->length();

			if (lenTicks == 0)
			{
				continue;
			}
			else if (lenTicks < 0)
			{
				lenTicks = 4;
			}

			int posTicks = note->pos();
			int noteWidth = lenTicks * m_pianoRoll.m_ppb / TimePos::ticksPerBar();

			int detuningLength = !note->detuning()->automationClip()->getTimeMap().isEmpty()
				? note->detuning()->automationClip()->getTimeMap().lastKey() * m_pianoRoll.m_ppb / TimePos::ticksPerBar()
				: noteWidth;

			const int x = (posTicks - m_pianoRoll.m_currentPosition) * m_pianoRoll.m_ppb / TimePos::ticksPerBar();
			if (!(x + std::max(noteWidth, detuningLength) >= 0 && x <= m_pianoRoll.width() - m_pianoRoll.m_whiteKeyWidth))
			{
				continue;
			}

			if (note->key() > bottomKey && note->key() <= topKey)
			{
				const auto fillColor = note->type() == Note::Type::Regular ? m_pianoRoll.m_noteColor : m_pianoRoll.m_stepNoteColor;

				m_pianoRoll.drawNoteRect(
					p, x + m_pianoRoll.m_whiteKeyWidth, noteYPos(note->key()), noteWidth,
					note, fillColor, m_pianoRoll.m_noteTextColor, m_pianoRoll.m_selectedNoteColor,
					m_pianoRoll.m_noteOpacity, m_pianoRoll.m_noteBorders, drawNoteNames
				);
			}

			int editHandleTop = 0;
			if (m_pianoRoll.m_noteEditMode == PianoRoll::NoteEditMode::Volume)
			{
				QColor color = m_pianoRoll.m_barColor.lighter(30 + (note->getVolume() * 90 / MaxVolume));
				if (note->selected())
				{
					color = m_pianoRoll.m_selectedNoteColor;
				}
				p.setPen(QPen(color, NOTE_EDIT_LINE_WIDTH));

				editHandleTop = m_pianoRoll.noteEditBottom() -
					(static_cast<float>(note->getVolume() - MinVolume)) /
					(static_cast<float>(MaxVolume - MinVolume)) *
					(static_cast<float>(m_pianoRoll.noteEditBottom() - m_pianoRoll.noteEditTop()));

				p.drawLine(QLineF(m_pianoRoll.noteEditLeft() + x + 0.5, editHandleTop + 0.5,
					m_pianoRoll.noteEditLeft() + x + 0.5, m_pianoRoll.noteEditBottom() + 0.5));
			}
			else if (m_pianoRoll.m_noteEditMode == PianoRoll::NoteEditMode::Panning)
			{
				QColor color = m_pianoRoll.m_noteColor;
				if (note->selected())
				{
					color = m_pianoRoll.m_selectedNoteColor;
				}

				p.setPen(QPen(color, NOTE_EDIT_LINE_WIDTH));

				editHandleTop = m_pianoRoll.noteEditBottom() -
					(static_cast<float>(note->getPanning() - PanningLeft)) /
					(static_cast<float>(PanningRight - PanningLeft)) *
					(static_cast<float>(m_pianoRoll.noteEditBottom() - m_pianoRoll.noteEditTop()));

				p.drawLine(QLine(m_pianoRoll.noteEditLeft() + x, m_pianoRoll.noteEditTop() +
						(static_cast<float>(m_pianoRoll.noteEditBottom() - m_pianoRoll.noteEditTop())) / 2.0f,
					m_pianoRoll.noteEditLeft() + x, editHandleTop));
			}
			editHandles << QPoint(x + m_pianoRoll.noteEditLeft(), editHandleTop);

			if (note->hasDetuningInfo())
			{
				m_pianoRoll.drawDetuningInfo(p, note, x + m_pianoRoll.m_whiteKeyWidth, noteYPos(note->key()));
				p.setClipRect(
					m_pianoRoll.m_whiteKeyWidth,
					PR_TOP_MARGIN,
					m_pianoRoll.width() - m_pianoRoll.m_whiteKeyWidth,
					m_pianoRoll.height() - PR_TOP_MARGIN);
			}
		}

		// draw clip bounds
		p.fillRect(
			xCoordOfTick(m_pianoRoll.m_midiClip->length() - m_pianoRoll.m_midiClip->startTimeOffset()),
			PR_TOP_MARGIN,
			m_pianoRoll.width() - 10,
			m_pianoRoll.noteEditBottom(),
			m_pianoRoll.m_outOfBoundsShade
		);
		p.fillRect(
			0,
			PR_TOP_MARGIN,
			xCoordOfTick(-m_pianoRoll.m_midiClip->startTimeOffset()),
			m_pianoRoll.noteEditBottom(),
			m_pianoRoll.m_outOfBoundsShade
		);

		// -- Knife tool (draw cut line)
		if (m_pianoRoll.m_action == PianoRoll::Action::Knife && m_pianoRoll.m_knifeDown)
		{
			int x1 = xCoordOfTick(m_pianoRoll.m_knifeStartTickPos);
			int y1 = yBase - (m_pianoRoll.m_knifeStartKey - m_pianoRoll.m_startKey + 1) * m_pianoRoll.m_keyLineHeight;
			int x2 = xCoordOfTick(m_pianoRoll.m_knifeEndTickPos);
			int y2 = yBase - (m_pianoRoll.m_knifeEndKey - m_pianoRoll.m_startKey + 1) * m_pianoRoll.m_keyLineHeight;

			p.setPen(QPen(m_pianoRoll.m_knifeCutLineColor, 1));
			p.drawLine(x1, y1, x2, y2);
		}
		// -- End knife tool

		// draw current step recording notes
		for (const Note* note : m_pianoRoll.m_stepRecorder.getCurStepNotes())
		{
			int lenTicks = note->length();

			if (lenTicks == 0)
			{
				continue;
			}

			int posTicks = note->pos();
			int noteWidth = lenTicks * m_pianoRoll.m_ppb / TimePos::ticksPerBar();
			const int x = (posTicks - m_pianoRoll.m_currentPosition) * m_pianoRoll.m_ppb / TimePos::ticksPerBar();
			if (!(x + noteWidth >= 0 && x <= m_pianoRoll.width() - m_pianoRoll.m_whiteKeyWidth))
			{
				continue;
			}

			if (note->key() > bottomKey && note->key() <= topKey)
			{
				m_pianoRoll.drawNoteRect(
					p, x + m_pianoRoll.m_whiteKeyWidth, noteYPos(note->key()), noteWidth,
					note, m_pianoRoll.m_currentStepNoteColor, m_pianoRoll.m_noteTextColor, m_pianoRoll.m_selectedNoteColor,
					m_pianoRoll.m_noteOpacity, m_pianoRoll.m_noteBorders, drawNoteNames);
			}
		}

		p.setPen(QPen(m_pianoRoll.m_noteColor, NOTE_EDIT_LINE_WIDTH + 2));
		p.drawPoints(editHandles);
	}

	// now draw selection-frame
	p.setClipRect(
		m_pianoRoll.m_whiteKeyWidth,
		PR_TOP_MARGIN,
		m_pianoRoll.width() - m_pianoRoll.m_whiteKeyWidth,
		m_pianoRoll.height() - PR_TOP_MARGIN - m_pianoRoll.m_notesEditHeight - PR_BOTTOM_MARGIN);

	int x = ((selPosStart - m_pianoRoll.m_currentPosition) * m_pianoRoll.m_ppb) / TimePos::ticksPerBar();
	int w = ((((selPosEnd - m_pianoRoll.m_currentPosition) * m_pianoRoll.m_ppb) / TimePos::ticksPerBar())) - x;
	int y = yBase - selKeyStart * m_pianoRoll.m_keyLineHeight;
	int h = yBase - selKeyEnd * m_pianoRoll.m_keyLineHeight - y;
	p.setPen(m_pianoRoll.m_selectedNoteColor);
	p.setBrush(Qt::NoBrush);
	p.drawRect(x + m_pianoRoll.m_whiteKeyWidth, y, w, h);

	// TODO: Get this out of paint event
	int l = m_pianoRoll.hasValidMidiClip() ? static_cast<int>(m_pianoRoll.m_midiClip->length()) - m_pianoRoll.m_midiClip->startTimeOffset() : 0;

	// reset scroll-range
	if (m_pianoRoll.m_leftRightScroll->maximum() != l)
	{
		m_pianoRoll.m_leftRightScroll->setRange(0, l);
		m_pianoRoll.m_leftRightScroll->setPageStep(l);
	}

	// set line colors
	auto editAreaCol = QColor(m_pianoRoll.m_lineColor);
	auto currentKeyCol = QColor(m_pianoRoll.m_beatLineColor);

	editAreaCol.setAlpha(64);
	currentKeyCol.setAlpha(64);

	// horizontal line for the key under the cursor
	if (m_pianoRoll.hasValidMidiClip() && getGUI()->pianoRoll()->hasFocus())
	{
		int keyNum = m_pianoRoll.getKey(m_pianoRoll.mapFromGlobal(QCursor::pos()).y());
		p.fillRect(
			10,
			m_pianoRoll.yCoordOfKey(keyNum) + 3,
			m_pianoRoll.width() - 10,
			m_pianoRoll.m_keyLineHeight - 7,
			currentKeyCol);
	}

	// bar to resize note edit area
	p.setClipRect(0, 0, m_pianoRoll.width(), m_pianoRoll.height());
	p.fillRect(QRect(0, m_pianoRoll.keyAreaBottom(),
			m_pianoRoll.width() - PR_RIGHT_MARGIN, NOTE_EDIT_RESIZE_BAR), editAreaCol);

	if (getGUI()->pianoRoll()->hasFocus())
	{
		const QPixmap* cursor = nullptr;
		// draw current edit-mode-icon below the cursor
		switch (m_pianoRoll.m_editMode)
		{
		case PianoRoll::EditMode::Draw:
			if (m_pianoRoll.m_mouseDownRight)
			{
				cursor = &m_pianoRoll.m_toolErase;
			}
			else if (m_pianoRoll.m_action == PianoRoll::Action::MoveNote)
			{
				cursor = &m_pianoRoll.m_toolMove;
			}
			else
			{
				cursor = &m_pianoRoll.m_toolDraw;
			}
			break;
		case PianoRoll::EditMode::Erase:
			cursor = &m_pianoRoll.m_toolErase;
			break;
		case PianoRoll::EditMode::Select:
			cursor = &m_pianoRoll.m_toolSelect;
			break;
		case PianoRoll::EditMode::Detuning:
			cursor = &m_pianoRoll.m_toolOpen;
			break;
		case PianoRoll::EditMode::Knife:
			cursor = &m_pianoRoll.m_toolKnife;
			break;
		case PianoRoll::EditMode::Strum:
			cursor = &m_pianoRoll.m_toolStrum;
			break;
		}
		QPoint mousePosition = m_pianoRoll.mapFromGlobal(QCursor::pos());
		if (cursor != nullptr && mousePosition.y() > m_pianoRoll.keyAreaTop() && mousePosition.x() > m_pianoRoll.noteEditLeft())
		{
			p.drawPixmap(mousePosition + QPoint(8, 8), *cursor);
		}
	}
}

} // namespace lmms::gui
