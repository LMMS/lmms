/*
 * AutomationPattern.h - declaration of class AutomationPattern, which contains
 *                       all information about an automation pattern
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

#ifndef AUTOMATION_PATTERN_H
#define AUTOMATION_PATTERN_H

#include <QtCore/QMap>
#include <QtCore/QPointer>

#include "Track.h"


class AutomationTrack;
class MidiTime;



class AutomationNode
{
public:
	AutomationNode();
	AutomationNode( float value );
	AutomationNode( float inValue, float outValue );

	inline float getInValue()
	{
		return m_inValue;
	}
	inline const float getInValue() const
	{
		return m_inValue;
	}
	inline void setInValue( float value )
	{
		m_inValue = value;
	}

	inline float getOutValue()
	{
		return m_outValue;
	}
	inline const float getOutValue() const
	{
		return m_outValue;
	}
	inline void setOutValue( float value )
	{
		m_outValue = value;
	}

	inline float getInTangent()
	{
		return m_inTangent;
	}
	inline const float getInTangent() const
	{
		return m_inTangent;
	}
	inline void setInTangent( float tangent )
	{
		m_inTangent = tangent;
	}

	inline float getOutTangent()
	{
		return m_outTangent;
	}
	inline const float getOutTangent() const
	{
		return m_outTangent;
	}
	inline void setOutTangent( float tangent )
	{
		m_outTangent = tangent;
	}

private:
	float m_inValue;
	float m_outValue;

	// Slope at each point for calculating spline
	// We might have discrete jumps between curves, so we possibly have
	// two different tangents for each side of the curve. If inValue and
	// outValue are equal, inTangent and outTangent are equal too.
	float m_inTangent;
	float m_outTangent;
} ;

class LMMS_EXPORT AutomationPattern : public TrackContentObject
{
	Q_OBJECT
public:
	enum ProgressionTypes
	{
		DiscreteProgression,
		LinearProgression,
		CubicHermiteProgression
	} ;

	typedef QMap<int, AutomationNode> timeMap;
	typedef QVector<QPointer<AutomatableModel> > objectVector;

	AutomationPattern( AutomationTrack * _auto_track );
	AutomationPattern( const AutomationPattern & _pat_to_copy );
	virtual ~AutomationPattern() = default;

	bool addObject( AutomatableModel * _obj, bool _search_dup = true );

	const AutomatableModel * firstObject() const;
	const objectVector& objects() const;

	// progression-type stuff
	inline ProgressionTypes progressionType() const
	{
		return m_progressionType;
	}
	void setProgressionType( ProgressionTypes _new_progression_type );

	inline float getTension() const
	{
		return m_tension;
	}
	void setTension( QString _new_tension );

	MidiTime timeMapLength() const;
	void updateLength();

	MidiTime putValue( const MidiTime & time,
				const float value,
				const bool quantPos = true,
				const bool ignoreSurroundingPoints = true );

	void removeValue( const MidiTime & time );

	void recordValue(MidiTime time, float value);

	MidiTime setDragValue( const MidiTime & time,
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

	float valueAt( const MidiTime & _time ) const;
	float *valuesAfter( const MidiTime & _time ) const;

	const QString name() const;

	// settings-management
	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	static const QString classNodeName() { return "automationpattern"; }
	QString nodeName() const override { return classNodeName(); }

	TrackContentObjectView * createView( TrackView * _tv ) override;


	static bool isAutomated( const AutomatableModel * _m );
	static QVector<AutomationPattern *> patternsForModel( const AutomatableModel * _m );
	static AutomationPattern * globalAutomationPattern( AutomatableModel * _m );
	static void resolveAllIDs();

	bool isRecording() const { return m_isRecording; }
	void setRecording( const bool b ) { m_isRecording = b; }

	static int quantization() { return s_quantization; }
	static void setQuantization(int q) { s_quantization = q; }

public slots:
	void clear();
	void objectDestroyed( jo_id_t );
	void flipY( int min, int max );
	void flipY();
	void flipX( int length = -1 );

private:
	void cleanObjects();
	void generateTangents();
	void generateTangents( timeMap::iterator it, int numToGenerate );
	float valueAt( timeMap::const_iterator v, int offset ) const;

	AutomationTrack * m_autoTrack;
	QVector<jo_id_t> m_idsToResolve;
	objectVector m_objects;
	timeMap m_timeMap;	// actual values
	timeMap m_oldTimeMap;	// old values for storing the values before setDragValue() is called.
	float m_tension;
	bool m_hasAutomation;
	ProgressionTypes m_progressionType;

	bool m_dragging;
	
	bool m_isRecording;
	float m_lastRecordedValue;

	static int s_quantization;

	static const float DEFAULT_MIN_VALUE;
	static const float DEFAULT_MAX_VALUE;

	friend class AutomationPatternView;

} ;


#endif
