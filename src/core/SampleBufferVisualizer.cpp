/*
 * Copyright (c) 2018 Shmuel H. (shmuelhazan0/at/gmail.com)
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
#include "include/SampleBufferVisualizer.h"

#include "Track.h"
#include "Engine.h"

#include <QPainter>
#include <QDebug>

SampleBufferVisualizer::SampleBufferVisualizer()
{

}

void SampleBufferVisualizer::update(const SampleBuffer &sampleBuffer,
									MidiTime sampleStartOffset,
									MidiTime sampleLength,
									const QRect &parentRect,
									float pixelsPerTact,
									f_cnt_t framesPerTact,
									const QPen &pen,
									SampleBufferVisualizer::Operation operation)
{
	// !=
	if (pixelsPerTact < m_pixelsPerTact || pixelsPerTact > m_pixelsPerTact
			|| framesPerTact != m_framesPerTact
			|| sampleLength < m_cachedTime)
		operation = Operation::Clear;

	m_pixelsPerTact = pixelsPerTact;
	m_generalPaintOffset = sampleStartOffset;
	m_framesPerTact = framesPerTact;
	m_drawPixelOffset = parentRect.left();

	switch (operation) {
	case Operation::Clear:
		m_cachedPixmaps.clear();
		m_currentPixmap.pixmap = QPixmap();
		m_currentPixmap.totalTime = 0;
		m_cachedTime = 0;

#ifdef Q_FALLTHROUGH
	Q_FALLTHROUGH();
#endif
	case Operation::Append:
		appendMultipleTacts(sampleBuffer,
							sampleLength,
							parentRect.translated(-parentRect.x(),
												  0),
							pen);
		break;
	}
}

void SampleBufferVisualizer::draw(QPainter &painter)
{
	float pixelOffset = pixelsPerTime(m_generalPaintOffset) + m_drawPixelOffset;
	for (const auto &pixmap : m_cachedPixmaps) {
		auto targetRect = pixmap.rect.translated(int(pixelOffset),
												 0);

		painter.drawPixmap(targetRect, pixmap.pixmap);
		pixelOffset += pixelsPerTime(pixmap.totalTime);
	}

	auto targetRect = m_currentPixmap.rect.translated(int(pixelOffset),
													  0);
	painter.drawPixmap(targetRect, m_currentPixmap.pixmap);
}

void SampleBufferVisualizer::appendMultipleTacts(const SampleBuffer &sampleBuffer,
												 MidiTime sampleLength,
												 const QRect &parentRect,
												 const QPen &pen)
{
	for (; (m_cachedTime+m_currentPixmap.totalTime) < sampleLength;)
	{
		MidiTime totalTime;
		MidiTime offsetFromTact = m_currentPixmap.totalTime;
		bool isCompleteTact = false;

		// we have more tacts to draw. finish the current one.
		if (MidiTime(m_cachedTime+offsetFromTact).getTact() < sampleLength.getTact()) {
			// Paint what left from the current tact.
			totalTime = MidiTime::ticksPerTact() - offsetFromTact;
			isCompleteTact = true;
		} else {
			// Draw only the ticks left in the current tact.
			totalTime = sampleLength - m_cachedTime - m_currentPixmap.totalTime;
		}

		Q_ASSERT((offsetFromTact + totalTime) <= MidiTime::ticksPerTact());

		if (pixelsPerTime(totalTime) < 1) {
			// We can't paint it.
			// totalTime is too short. skip it.
			// or just wait until we have enough frames.
			if (isCompleteTact) {
				// Skip it and continue to the next tact.
				m_currentPixmap.totalTime += totalTime;
			} else {
				// Wait until we have enough frames.
				break;
			}
		}

		auto result = appendTact(sampleBuffer,
								 totalTime,
								 parentRect,
								 pen,
								 isCompleteTact);
		if (! result)
			break;

		if (isCompleteTact) {
			m_cachedTime += ( m_currentPixmap.totalTime );
			m_cachedPixmaps.push_back(std::move(m_currentPixmap));
			m_currentPixmap.pixmap = QPixmap();
			m_currentPixmap.totalTime = 0;
		}
	}
}

bool SampleBufferVisualizer::appendTact(const SampleBuffer &sampleBuffer,
										const MidiTime &totalTime,
										const QRect &parentRect,
										const QPen &pen,
										bool isLastInTact)
{
	auto offsetFromTact = m_currentPixmap.totalTime;

	auto currentPaintInTact = getRectForSampleFragment (parentRect,
														offsetFromTact,
														totalTime,
														isLastInTact);
	Q_ASSERT(currentPaintInTact.width() > 0);

	// Generate the actual visualization.
	auto fromFrame = MidiTime(m_cachedTime + offsetFromTact).frames (m_framesPerTact);

	if (! sampleBuffer.tryDataReadLock())
		return false;
	auto poly = sampleBuffer.visualizeToPoly (currentPaintInTact,
											  QRect(),
											  fromFrame,
											  fromFrame + totalTime.frames(m_framesPerTact));
	sampleBuffer.dataUnlock ();

	m_currentPixmap.totalTime += totalTime;

	m_currentPixmap.rect = getRectForSampleFragment (parentRect,
													 0,
													 MidiTime::ticksPerTact());
	if (m_currentPixmap.pixmap.isNull()) {
		m_currentPixmap.pixmap = QPixmap(m_currentPixmap.rect.size());
		m_currentPixmap.pixmap.fill(Qt::transparent);
	}

	// Draw the points into the pixmap.
	QPainter pixmapPainter (&m_currentPixmap.pixmap);
	pixmapPainter.setPen(pen);

	pixmapPainter.setRenderHint( QPainter::Antialiasing );

	pixmapPainter.drawPolyline (poly.first);
	pixmapPainter.drawPolyline (poly.second);
	pixmapPainter.end();

	// Continue to the next tact or stop.
	return true;
}

QRect SampleBufferVisualizer::getRectForSampleFragment(QRect parentRect, MidiTime beginOffset,
													   MidiTime totalTime,
													   bool forceNotZeroWidth) {
	int offset = pixelsPerTime(beginOffset);

	float top = parentRect.top ();
	float height = parentRect.height ();

	QRect r = QRect( int(parentRect.x ()) + int(offset),
					 top,
					 int(qMax( int(pixelsPerTime(totalTime)) , (forceNotZeroWidth ? 1 : 0) )),
					 int(height));


	return r;
}
