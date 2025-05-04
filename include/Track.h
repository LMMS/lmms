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


/*! The minimum track height in pixels
 *
 * Tracks can be resized by shift-dragging anywhere inside the track
 * display.  This sets the minimum size in pixels for a track.
 */
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

	Track( Type type, TrackContainer * tc );
	~Track() override;

	static Track * create( Type tt, TrackContainer * tc );
	static Track * create( const QDomElement & element,
							TrackContainer * tc );
	Track * clone();


	// pure virtual functions
	Type type() const
	{
		return m_type;
	}

	virtual bool play( const TimePos & start, const fpp_t frames,
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
	Clip * addClip( Clip * clip );
	void removeClip( Clip * clip );
	// -------------------------------------------------------
	void deleteClips();

	int numOfClips();
	auto getClip(std::size_t clipNum) -> Clip*;
	int getClipNum(const Clip* clip );

	const clipVector & getClips() const
	{
		return m_clips;
	}
	void getClipsInRange( clipVector & clipV, const TimePos & start,
							const TimePos & end );
	void swapPositionOfClips( int clipNum1, int clipNum2 );

	void createClipsForPattern(int pattern);


	void insertBar( const TimePos & pos );
	void removeBar( const TimePos & pos );

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

	void toggleSolo();

private:
	void saveTrack(QDomDocument& doc, QDomElement& element, bool presetMode);
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
