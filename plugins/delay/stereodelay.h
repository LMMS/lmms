/*
 * stereodelay.h - declaration of StereoDelay class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef STEREODELAY_H
#define STEREODELAY_H

#include "lmms_basics.h"

class StereoDelay
{
public:
	StereoDelay( int maxLength, int sampleRate );
	~StereoDelay();
	inline void setLength( float length )
	{
		if( length <= m_maxLength && length >= 0 )
		{
			m_length = length;
		}
	}

	inline void setFeedback( float feedback )
	{
		m_feedback = feedback;
	}

	void tick( sampleFrame frame );
	void setSampleRate( int sampleRate );

private:
	sampleFrame* m_buffer;
	int m_maxLength;
	float m_length;
	int m_index;
	float m_feedback;
	float m_maxTime;
};

#endif // STEREODELAY_H
