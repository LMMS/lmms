/*
 * TrackContainer.h - base-class for all track-containers like Song-Editor,
 *                    Pattern Editor...
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_TRACK_CONTAINER_H
#define LMMS_TRACK_CONTAINER_H

#include <QReadWriteLock>

#include "AudioEngine.h"
#include "AutomationTrack.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "JournallingObject.h"
#include "PatternTrack.h"
#include "SampleTrack.h"
#include "Track.h"

namespace lmms
{

class AutomationClip;

namespace gui
{

class TrackContainerView;

}


class LMMS_EXPORT TrackContainer : public Model, public JournallingObject
{
	Q_OBJECT
public:
	using TrackList = std::vector<Track*>;
	enum class Type
	{
		Pattern,
		Song
	} ;

	TrackContainer();

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;

	void loadSettings( const QDomElement & _this ) override;

	int countTracks( Track::Type _tt = Track::Type::Count ) const;

	template<typename T, typename... Args>
	T* addNewTrack(Args&&... args)
	{
		static_assert(std::is_base_of_v<Track, T>, "T must be a kind of Track");
		auto track = std::make_unique<T>(std::forward<Args>(args)...);
		return static_cast<T*>(addTrack(std::move(track)));
	}

	Track* addNewTrack(const QDomElement& element);
	virtual Track* addTrack(std::unique_ptr<Track> track);
	virtual void removeTrack(Track* track);

	void clearAllTracks();

	std::vector<Track*> tracks() const;
	bool isEmpty() const;

	static const QString classNodeName()
	{
		return "trackcontainer";
	}

	inline void setType( Type newType )
	{
		m_TrackContainerType = newType;
	}

	inline Type type() const
	{
		return m_TrackContainerType;
	}

	virtual AutomatedValueMap automatedValuesAt(TimePos time, int clipNum = -1) const;

signals:
	void trackAdded( lmms::Track * _track );

protected:
	static AutomatedValueMap automatedValuesFromTracks(const TrackList &tracks, TimePos timeStart, int clipNum = -1);

	mutable QReadWriteLock m_tracksMutex;

private:
	std::vector<std::unique_ptr<Track>> m_tracks;

	Type m_TrackContainerType;


	friend class gui::TrackContainerView;
	friend class Track;

} ;

} // namespace lmms

#endif // LMMS_TRACK_CONTAINER_H
