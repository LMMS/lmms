/*
 * AutomationPattern.h - declaration of class AutomationPattern, which contains
 *                       all information about an automation pattern
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#ifndef AUTOMATION_PATTERN_H
#define AUTOMATION_PATTERN_H

#include <QtCore/QMap>
#include <QtCore/QPointer>

#include "track.h"


class AutomationTrack;
class MidiTime;



class EXPORT AutomationPattern : public trackContentObject
{
	Q_OBJECT
public:
	enum ProgressionTypes
	{
		DiscreteProgression,
		LinearProgression,
		CubicHermiteProgression
	} ;

	typedef QMap<int, float> timeMap;
	typedef QVector<QPointer<AutomatableModel> > objectVector;

	AutomationPattern( AutomationTrack * _auto_track );
	AutomationPattern( const AutomationPattern & _pat_to_copy );
	virtual ~AutomationPattern();

	void addObject( AutomatableModel * _obj, bool _search_dup = true );

	const AutomatableModel * firstObject() const;

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

	virtual MidiTime length() const;

	MidiTime putValue( const MidiTime & _time, const float _value,
						const bool _quant_pos = true );

	void removeValue( const MidiTime & _time,
					  const bool _quant_pos = true );

	MidiTime setDragValue( const MidiTime & _time, const float _value,
						   const bool _quant_pos = true );

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

	inline const timeMap & getTangents() const
	{
		return m_tangents;
	}

	inline timeMap & getTangents()
	{
		return m_tangents;
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
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	static inline const QString classNodeName()
	{
		return "automationpattern";
	}

	inline virtual QString nodeName() const
	{
		return classNodeName();
	}

	void processMidiTime( const MidiTime & _time );

	virtual trackContentObjectView * createView( trackView * _tv );


	static bool isAutomated( const AutomatableModel * _m );
	static QVector<AutomationPattern *> patternsForModel( const AutomatableModel * _m );
	static AutomationPattern * globalAutomationPattern( AutomatableModel * _m );
	static void resolveAllIDs();

	bool isRecording() const
	{
		return m_isRecording;
	}
	
	void setRecording( const bool b )
	{
		m_isRecording = b;
	}

public slots:
	void clear();
	void openInAutomationEditor();
	void objectDestroyed( jo_id_t );

private:
	void cleanObjects();
	void generateTangents();
	void generateTangents( timeMap::const_iterator it, int numToGenerate );
	float valueAt( timeMap::const_iterator v, int offset ) const;

	AutomationTrack * m_autoTrack;
	QVector<jo_id_t> m_idsToResolve;
	objectVector m_objects;
	timeMap m_timeMap;	// actual values
	timeMap m_oldTimeMap;	// old values for storing the values before setDragValue() is called.
	timeMap m_tangents;	// slope at each point for calculating spline
	float m_tension;
	bool m_hasAutomation;
	ProgressionTypes m_progressionType;

	bool m_dragging;
	
	bool m_isRecording;
	float m_lastRecordedValue;

	static const float DEFAULT_MIN_VALUE;
	static const float DEFAULT_MAX_VALUE;

	friend class AutomationPatternView;

} ;


#endif
