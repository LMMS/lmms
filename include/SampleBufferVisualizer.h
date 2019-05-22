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

#ifndef SAMPLEBUFFERVISUALIZER_H
#define SAMPLEBUFFERVISUALIZER_H

#include <QPixmap>
#include <QRect>

#include "MidiTime.h"
#include "SampleBuffer.h"


/**
 * @brief A caching implementation of a visualizer for SampleBuffer.
 *
 * This implementation splits the tco into fragments of
 * 192 ticks. Then, it draws them into @a PixmapCacheLine.
 */
class SampleBufferVisualizer
{
	struct PixmapCacheLine
	{
		/**
		 * @brief The visualization of this cache line.
		 */
		QPixmap pixmap;

		/**
		 * @brief The rectangle of @a pixmap.
		 */
		QRect rect;

		/**
		 * @brief How much time @a pixmap is representing.
		 */
		MidiTime totalTime;
	};

public:
	SampleBufferVisualizer();

	using Operation=SampleBuffer::UpdateType;

	/**
	 * @brief Update the cache before drawing it.
	 * @param sampleBuffer		The sample buffer to visualize.
	 * @param sampleStartOffset	Offset from the sample buffer's beginning.
	 * @param sampleLength		The time we should
	 *							visualize (can be different that the sample buffer's length).
	 * @param parentRect		The rectangle we should paint in.
	 * @param pixelsPerTact		Current value of pixels per tact.
	 * @param framesPerTact		Current value of frames per tact.
	 * @param pen				The pen setting we should paint with.
	 * @param operation			Should we clear the cache or just append in the end?
	 */
	void update(const SampleBuffer &sampleBuffer,
				MidiTime sampleStartOffset,
				MidiTime sampleLength,
				const QRect &parentRect,
				float pixelsPerTact, f_cnt_t framesPerTact,
				const QPen &pen,
				Operation operation);

	/**
	 * @brief draw		Draw the current cache into @a painter.
	 */
	void draw(QPainter &painter);

private:
	/**
	 * @brief appendMultipleTacts	Add one or more cache lines or
	 *								just add data to an existing
	 *								cache line.
	 * @param sampleBuffer			The sample buffer to visualize.
	 * @param sampleLength			Time to visualize.
	 * @param parentRect			The rectangle we should paint in.
	 * @param pen					The pen setting we should paint with.
	 */
	void appendMultipleTacts(const SampleBuffer &sampleBuffer,
							 MidiTime sampleLength,
							 const QRect &parentRect, const QPen &pen);

	/**
	 * @brief appendTact	Add data to m_currentPixmap.
	 * @param sampleBuffer	The sample buffer to visualize.
	 * @param totalTime		How much time we should visualize from
	 *						m_currentPixmap.totalTime.
	 * @param parentRect	The rectangle we should paint in.
	 * @param pen
	 * @param pen			The pen setting we should paint with.
	 * @return				true if we have painted at least one
	 *						pixel; false otherwise.
	 */
	bool appendTact(const SampleBuffer &sampleBuffer,
					const MidiTime &totalTime,
					const QRect &parentRect, const QPen &pen, bool isLastInTact);

	/**
	 * @brief getRectForSampleFragment	Construct a rectangle for
	 *									visualization of a given
	 *									MidiTime range.
	 * @param parentRect				The rectangle we should be in.
	 * @param beginOffset				Offset of the rectangle from @a
	 *									parentRect's left.
	 * @param totalTime					The actual time to be painted.
	 * @param forceNotZeroWidth			Should we make sure rect.width != 0?
	 */
	QRect getRectForSampleFragment(QRect parentRect,
								   MidiTime beginOffset,
								   MidiTime totalTime,
								   bool forceNotZeroWidth=false);

	/**
	 * @brief pixelsPerTime		Calculate how much pixels a visualization of
	 *							@a time would take.
	 */
	float pixelsPerTime (const MidiTime &time)
	{
		return ((time * m_pixelsPerTact) / MidiTime::ticksPerTact());
	}

	/**
	  * How much time have we cached already (not including
	  * the cache in m_currentPixmap).
	  */
	MidiTime m_cachedTime{0};

	/**
	  * X time drawing offset from the beginning.
	  */
	MidiTime m_generalPaintOffset{0};
	/**
	  * X pixel drawing offset from the beginning.
	  */
	int m_drawPixelOffset = 0;

	QList<PixmapCacheLine> m_cachedPixmaps;

	/**
	 * The current cache line we've not finished yet.
	 */
	PixmapCacheLine m_currentPixmap;

	float m_pixelsPerTact = 0;
	f_cnt_t m_framesPerTact = 0;
};

#endif // SAMPLEBUFFERVISUALIZER_H
