/*
 * TrackConteintObject.h - declaration of Clip class
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

#ifndef LMMS_CLIP_H
#define LMMS_CLIP_H

#include <optional>

#include <QColor>

#include "AutomatableModel.h"


namespace lmms
{

class Track;

namespace gui
{

class ClipView;
class TrackView;

} // namespace gui


class LMMS_EXPORT Clip : public Model, public JournallingObject
{
	Q_OBJECT
	mapPropertyFromModel(bool,isMuted,setMuted,m_mutedModel);
	mapPropertyFromModel(bool,isSolo,setSolo,m_soloModel);
public:
	//! @brief Creates a new clip for the given track @p track.
	//! @param track The track that will contain the new object
	Clip(Track* track);
	~Clip() override; //!< @brief Destroys the given clip.

	inline Track * getTrack() const
	{
		return m_track;
	}

	inline const QString & name() const
	{
		return m_name;
	}

	inline void setName( const QString & name )
	{
		m_name = name;
		emit dataChanged();
	}

	QString displayName() const override
	{
		return name();
	}


	inline const TimePos & startPosition() const
	{
		return m_startPosition;
	}

	inline TimePos endPosition() const
	{
		const int sp = m_startPosition;
		return sp + m_length;
	}

	inline const TimePos & length() const
	{
		return m_length;
	}


	bool hasTrackContainer() const;

	bool isInPattern() const;

	bool manuallyResizable() const;

	//! @brief Set whether a clip has been resized yet by the user or the knife tool.
	//!
	//! If a clip has been resized previously, it will not automatically resize when editing it.
	void setAutoResize(const bool r) { m_autoResize = r; }

	bool getAutoResize() const
	{
		return m_autoResize;
	}

	auto color() const -> const std::optional<QColor>& { return m_color; }
	void setColor(const std::optional<QColor>& color);

	//! @brief Move this Clip's position in time
	//!
	//! If the clip has moved, update its position. We also add a journal entry for undo and update the display.
	//!
	//! @param pos The new position of the clip.
	virtual void movePosition(const TimePos& pos);

	//! @brief Change the length of this Clip
	//!
	//! If the clip's length has changed, update it. We also add a journal entry for undo and update the display.
	//!
	//! @param length The new length of the clip.
	virtual void changeLength(const TimePos& length);
	virtual void updateLength() {};

	virtual gui::ClipView * createView( gui::TrackView * tv ) = 0;

	inline void selectViewOnCreate( bool select )
	{
		m_selectViewOnCreate = select;
	}

	inline bool getSelectViewOnCreate()
	{
		return m_selectViewOnCreate;
	}

	//! @brief Returns true if and only if `a->startPosition()` < `b->startPosition()`
	static bool comparePosition(const Clip* a, const Clip* b);

	TimePos startTimeOffset() const;
	void setStartTimeOffset( const TimePos &startTimeOffset );

	//! @brief Copies the state of a clip to another clip
	static void copyStateTo(Clip* src, Clip* dst);

	//! @brief Creates a copy of this clip
	//! @return Pointer to the new clip object
	virtual Clip* clone() = 0;

public slots:
	void toggleMute(); //!< @brief Mutes this Clip

signals:
	void lengthChanged();
	void positionChanged();
	void destroyedClip();
	void colorChanged();

protected:

	//! @brief Creates a duplicate clip of the one provided.
	//! @param other The clip object which will be copied.
	Clip(const Clip& other);

private:
	Track * m_track;
	QString m_name;

	TimePos m_startPosition;
	TimePos m_length;
	TimePos m_startTimeOffset;

	BoolModel m_mutedModel;
	BoolModel m_soloModel;
	bool m_autoResize = true;

	bool m_selectViewOnCreate;

	std::optional<QColor> m_color;

	friend class ClipView;
};


} // namespace lmms

#endif // LMMS_CLIP_H
