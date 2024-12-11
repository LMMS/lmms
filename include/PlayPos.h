/*
 * PlayPos.h - declaration of class PlayPos, a TimePos with additional features useful for Timelines
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_PLAYPOS_H
#define LMMS_PLAYPOS_H

#include "TimePos.h"

namespace lmms
{

class PlayPos : public TimePos
{
public:
    PlayPos( const int abs = 0 ) :
        TimePos( abs ),
        m_currentFrame( 0.0f )
    {
    }
    inline void setCurrentFrame( const float f )
    {
        m_currentFrame = f;
    }
    inline float currentFrame() const
    {
        return m_currentFrame;
    }
    inline void setJumped( const bool jumped )
    {
        m_jumped = jumped;
    }
    inline bool jumped() const
    {
        return m_jumped;
    }

private:
    float m_currentFrame;
    bool m_jumped;
};

} // namespace lmms

#endif // LMMS_PLAYPOS_H
