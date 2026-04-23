/*
 * Track.h - declaration of Track class
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

#ifndef LMMS_TRACK_H
#define LMMS_TRACK_H

#include <vector>

#include <QColor>

#include "AutomatableModel.h"
#include "JournallingObject.h"
#include "LmmsTypes.h"
#include <optional>


namespace lmms
{

class TimePos;
class TrackContainer;
class Clip;


namespace gui
{

class TrackView;
class TrackContainerView;

}


//! @brief The minimum track height in pixels
//!
//! Tracks can be resized by shift-dragging anywhere inside the track display. This sets the minimum size in pixels
//! for a track.
const int MINIMAL_TRACK_HEIGHT = 32;
const int DEFAULT_TRACK_HEIGHT = 32;

char const *const FILENAME_FILTER = "[\\0000-\x1f\"*/:<>?\\\\|\x7f]";


//! Base-class for all tracks
class LMMS_EXPORT Track : public Model, public JournallingObject
{
	Q_OBJECT
	mapPropertyFromModel(bool,isMuted,setMuted,m_mutedModel);
	mapPropertyFromModel(bool,isSolo,setSolo,m_soloModel);
public:
	using clipVector = std::vector<Clip*>;

	enum class Type
	{
		Instrument,
		Pattern,
		Sample,
		Event,
		Video,
		Automation,
		HiddenAutomation,
		Count
	} ;

	//! @brief Create a new (empty) track object
	//!
	//! The track object is the whole track, linking its contents, its automation, name, type, and so forth.
	//!
	//! @param type The type of track (Song Editor or Pattern Editor)
	//! @param tc The track Container object to encapsulate in this track.
	Track(Type type, TrackContainer* tc);

	//! @brief Delete the clips and remove this track from the track container.
	~Track() override;

	//! @brief Create a track based on the given track type and container.
	//!
	//! @param tt The type of track to create
	//! @param tc The track container to attach to
	static Track* create(Type tt, TrackContainer* tc);

	//! @brief Create a track inside TrackContainer from track type in a QDomElement and restore state from XML
	//!
	//! @param element The QDomElement containing the type of track to create
	//! @param tc The track container to attach to
	static Track* create(const QDomElement& element, TrackContainer* tc);

	//! @brief Clone a track from this track
	Track * clone();


	// pure virtual functions
	Type type() const
	{
		return m_type;
	}

	virtual bool play( const TimePos & start, const f_cnt_t frames,
						const f_cnt_t frameBase, int clipNum = -1 ) = 0;



	virtual gui::TrackView * createView( gui::TrackContainerView * view ) = 0;
	virtual Clip * createClip( const TimePos & pos ) = 0;

	virtual void saveTrackSpecificSettings(QDomDocument& doc, QDomElement& parent, bool presetMode) = 0;
	virtual void loadTrackSpecificSettings( const QDomElement & element ) = 0;

	// Saving and loading of presets which do not necessarily contain all the track information
	void savePreset(QDomDocument & doc, QDomElement & element);
	void loadPreset(const QDomElement & element);

	// Saving and loading of full tracks
	void saveSettings( QDomDocument & doc, QDomElement & element ) override;
	void loadSettings( const QDomElement & element ) override;

	// -- for usage by Clip only ---------------
	// TODO: Then why are they public?
	
	//! @brief Add another Clip into this track
	//! @param clip The Clip to attach to this track.
	Clip* addClip(Clip* clip);

	//! @brief Remove a given Clip from this track
	//! @param clip The Clip to remove from this track.
	void removeClip(Clip* clip);
	// -------------------------------------------------------

	//! @brief Remove all Clips from this track
	void deleteClips();

	//! @brief Return the number of clips we contain
	//! @return the number of clips we currently contain.
	int numOfClips() { return m_clips.size(); }

	//! @brief Get a Clip by number
	//!
	//! If the Clip number is less than our Clip array size then fetch that numbered object from the array. Otherwise we
	//! warn the user that we've somehow requested a Clip that is too large, and create a new Clip for them.
	//!
	//! @param clipNum The number of the Clip to fetch.
	//! @return The given Clip or a new one if out of range.
	//! @todo Reject Clip numbers less than zero.
	//! @todo If we create a Clip here, should we somehow attach it to the track?
	auto getClip(std::size_t clipNum) -> Clip*;

	//! @brief Determine the given Clip's number in our array.
	//! @param clip The Clip to search for.
	//! @return Its number in our array.
	int getClipNum(const Clip* clip);

	const clipVector & getClips() const
	{
		return m_clips;
	}

	//! @brief Retrieve a list of clips that fall within a period.
	//!
	//! Here we're interested in a range of clips that intersect the given time period.
	//! We return the Clips we find in order by time, earliest Clips first.
	//!
	//! @param clipV The list to contain the found clips.
	//! @param start The MIDI start time of the range.
	//! @param end The MIDI endi time of the range.
	void getClipsInRange(clipVector& clipV, const TimePos& start, const TimePos& end);

	//! @brief Swap the position of two clips.
	//!
	//! First, we arrange to swap the positions of the two Clips in the clips list. Then we swap their start times as
	//! well.
	//!
	//! @param clipNum1 The first Clip to swap.
	//! @param clipNum2 The second Clip to swap.
	void swapPositionOfClips( int clipNum1, int clipNum2 );

	void createClipsForPattern(int pattern);


	//! @brief Move all the clips after a certain time later by one bar.
	//!
	//! @param pos The time at which we want to insert the bar.
	void insertBar(const TimePos& pos);

	//! @brief Move all the clips after a certain time earlier by one bar.
	//! @param pos The time at which we want to remove the bar.
	void removeBar( const TimePos & pos );

	//! @brief Return the length of the entire track in bars
	//!
	//! We step through our list of Clips and determine their end position, keeping track of the latest time found in
	//! ticks. Then we return that in bars by dividing by the number of ticks per bar.
	bar_t length() const;


	inline TrackContainer* trackContainer() const
	{
		return m_trackContainer;
	}

	// name-stuff
	virtual const QString & name() const
	{
		return m_name;
	}

	QString displayName() const override
	{
		return name();
	}

	using Model::dataChanged;

	inline int getHeight()
	{
		return m_height >= MINIMAL_TRACK_HEIGHT
			? m_height
			: DEFAULT_TRACK_HEIGHT;
	}
	inline void setHeight( int height )
	{
		m_height = height;
	}

	void lock()
	{
		m_processingLock.lock();
	}
	void unlock()
	{
		m_processingLock.unlock();
	}
	bool tryLock()
	{
		return m_processingLock.tryLock();
	}

	auto color() const -> const std::optional<QColor>& { return m_color; }
	void setColor(const std::optional<QColor>& color);

	bool isMutedBeforeSolo() const
	{
		return m_mutedBeforeSolo;
	}
	
	BoolModel* getMutedModel();

public slots:
	virtual void setName(const QString& newName);

	void setMutedBeforeSolo(const bool muted)
	{
		m_mutedBeforeSolo = muted;
	}

	//! @brief Invert the track's solo state.
	//!
	//! We have to go through all the tracks determining if any other track is already soloed. Then we have to save the
	//! mute state of all tracks, and set our mute state to on and all the others to off.
	void toggleSolo();

private:
	//! @brief Save this track's settings to file
	//!
	//! We save the track type and its muted state and solo state, then append the track-specific settings. Then we
	//! iterate through the clips and save all their states in turn.
	//!
	//! @param doc The QDomDocument to use to save
	//! @param element The The QDomElement to save into
	//! @param presetMode Describes whether to save the track as a preset or not.
	//! @todo Does this accurately describe the parameters? I think not!?
	//! @todo Save the track height
	void saveTrack(QDomDocument& doc, QDomElement& element, bool presetMode);

	//! @brief Load the settings from a file
	//!
	//! We load the track's type and muted state and solo state, then clear out out current Clip.
	//!
	//! Then we step through the QDomElement's children and load the track-specific settings and clip states from it one
	//! at a time.
	//!
	//! @param element the QDomElement to load track settings from
	//! @param presetMode Indicates if a preset or a full track is loaded
	//! @todo Load the track height.
	void loadTrack(const QDomElement& element, bool presetMode);

private:
	TrackContainer* m_trackContainer;
	Type m_type;
	QString m_name;
	int m_height;

protected:
	BoolModel m_mutedModel;
	BoolModel m_soloModel;

private:
	bool m_mutedBeforeSolo;

	clipVector m_clips;

	QMutex m_processingLock;
	
	std::optional<QColor> m_color;

	friend class gui::TrackView;


signals:
	void destroyedTrack();
	void nameChanged();
	void clipAdded( lmms::Clip * );
	void colorChanged();
} ;


} // namespace lmms

#endif // LMMS_TRACK_H
