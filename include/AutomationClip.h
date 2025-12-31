/*
 * AutomationClip.h - declaration of class AutomationClip, which contains
 *                       all information about an automation clip
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#ifndef LMMS_AUTOMATION_CLIP_H
#define LMMS_AUTOMATION_CLIP_H

#include <QMap>
#include <QPointer>

#include "AutomationNode.h"
#include "Clip.h"


namespace lmms
{

class AutomationTrack;
class TimePos;

namespace gui
{
class AutomationClipView;
class AutomationEditor;
} // namespace gui



class LMMS_EXPORT AutomationClip : public Clip
{
	Q_OBJECT
public:
	enum class ProgressionType
	{
		Discrete,
		Linear,
		CubicHermite
	} ;

	using timeMap = QMap<int, AutomationNode>;
	using objectVector = std::vector<QPointer<AutomatableModel>>;

	using TimemapIterator = timeMap::const_iterator;

	AutomationClip( AutomationTrack * _auto_track );
	~AutomationClip() override = default;

	bool addObject( AutomatableModel * _obj, bool _search_dup = true );

	const AutomatableModel * firstObject() const;
	const objectVector& objects() const;

	// progression-type stuff
	inline ProgressionType progressionType() const
	{
		return m_progressionType;
	}
	void setProgressionType( ProgressionType _new_progression_type );

	inline float getTension() const
	{
		return m_tension;
	}
	void setTension( QString _new_tension );

	TimePos timeMapLength() const;
	void updateLength() override;

	TimePos putValue(
		const TimePos & time,
		const float value,
		const bool quantPos = true,
		const bool ignoreSurroundingPoints = true
	);

	TimePos putValues(
		const TimePos & time,
		const float inValue,
		const float outValue,
		const bool quantPos = true,
		const bool ignoreSurroundingPoints = true
	);

	void removeNode(const TimePos & time);
	void removeNodes(const int tick0, const int tick1);

	void resetNodes(const int tick0, const int tick1);

	/**
	 * @brief Resets the tangents from the nodes between the given ticks
	 * @param Int first tick of the range
	 * @param Int second tick of the range
	 */
	void resetTangents(const int tick0, const int tick1);

	void recordValue(TimePos time, float value);

	TimePos setDragValue( const TimePos & time,
				const float value,
				const bool quantPos = true,
				const bool controlKey = false );

	void applyDragValue();


	bool isDragging() const
	{
		return m_dragging;
	}

	inline const timeMap & getTimeMap() const
	{
		return m_timeMap;
	}

	inline timeMap & getTimeMap()
	{
		return m_timeMap;
	}

	inline float getMin() const
	{
		return firstObject()->minValue<float>();
	}

	inline float getMax() const
	{
		return firstObject()->maxValue<float>();
	}

	inline bool hasAutomation() const
	{
		return m_timeMap.isEmpty() == false;
	}

	static bool supportsTangentEditing(ProgressionType pType)
	{
		// Update function if we have new progression types that support tangent editing
		return pType == ProgressionType::CubicHermite;
	}

	inline bool canEditTangents() const
	{
		return supportsTangentEditing(m_progressionType);
	}

	float valueAt( const TimePos & _time ) const;
	float *valuesAfter( const TimePos & _time ) const;

	QString name() const;

	// settings-management
	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	static const QString classNodeName() { return "automationclip"; }
	QString nodeName() const override { return classNodeName(); }

	gui::ClipView * createView( gui::TrackView * _tv ) override;


	static bool isAutomated( const AutomatableModel * _m );
	static std::vector<AutomationClip*> clipsForModel(const AutomatableModel* _m);
	static AutomationClip * globalAutomationClip( AutomatableModel * _m );
	static void resolveAllIDs();

	bool isRecording() const { return m_isRecording; }
	void setRecording( const bool b ) { m_isRecording = b; }

	static int quantization() { return s_quantization; }
	static void setQuantization(int q) { s_quantization = q; }

	AutomationClip* clone() override
	{
		return new AutomationClip(*this);
	}

	void clearObjects() { m_objects.clear(); }

public slots:
	void clear();
	void objectDestroyed( lmms::jo_id_t );
	void flipY( int min, int max );
	void flipY();
	void flipX(int start = -1, int end = -1);

protected:
	AutomationClip( const AutomationClip & _clip_to_copy );

private:
	void cleanObjects();
	void generateTangents();
	void generateTangents(timeMap::iterator it, int numToGenerate);
	float valueAt( timeMap::const_iterator v, int offset ) const;

	/**
	 * @brief
	 * This function combines the song tracks, pattern store tracks,
	 * and the global automation track all in one vector.
	 *
	 * @return std::vector<Track*>
	 */
	static std::vector<Track*> combineAllTracks();

	// Mutex to make methods involving automation clips thread safe
	// Mutable so we can lock it from const objects
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
	mutable QRecursiveMutex m_clipMutex;
#else
	mutable QMutex m_clipMutex;
#endif

	AutomationTrack * m_autoTrack;
	std::vector<jo_id_t> m_idsToResolve;
	objectVector m_objects;
	timeMap m_timeMap;	// actual values
	timeMap m_oldTimeMap;	// old values for storing the values before setDragValue() is called.
	float m_tension;
	bool m_hasAutomation;
	ProgressionType m_progressionType;

	bool m_dragging;
	bool m_dragKeepOutValue; // Should we keep the current dragged node's outValue?
	float m_dragOutValue; // The outValue of the dragged node's
	bool m_dragLockedTan; // If the dragged node has it's tangents locked
	float m_dragInTan; // The dragged node's inTangent
	float m_dragOutTan; // The dragged node's outTangent

	bool m_isRecording;
	float m_lastRecordedValue;

	static int s_quantization;

	static const float DEFAULT_MIN_VALUE;
	static const float DEFAULT_MAX_VALUE;

	friend class gui::AutomationClipView;
	friend class AutomationNode;
	friend class gui::AutomationEditor;

} ;

//Short-hand functions to access node values in an automation clip;
// replacement for CPP macros with the same purpose; could be refactored
// further in the future.
inline float INVAL(AutomationClip::TimemapIterator it)
{
	return it->getInValue();
}

inline float OUTVAL(AutomationClip::TimemapIterator it)
{
	return it->getOutValue();
}

inline float OFFSET(AutomationClip::TimemapIterator it)
{
	return it->getValueOffset();
}

inline float INTAN(AutomationClip::TimemapIterator it)
{
	return it->getInTangent();
}

inline float OUTTAN(AutomationClip::TimemapIterator it)
{
	return it->getOutTangent();
}

inline float LOCKEDTAN(AutomationClip::TimemapIterator it)
{
	return it->lockedTangents();
}

inline int POS(AutomationClip::TimemapIterator it)
{
	return it.key();
}


} // namespace lmms

#endif // LMMS_AUTOMATION_CLIP_H
