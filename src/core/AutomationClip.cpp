/*
 * AutomationClip.cpp - implementation of class AutomationClip which
 *                         holds dynamic values
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

#include "AutomationClip.h"

#include "AutomationNode.h"
#include "AutomationClipView.h"
#include "AutomationTrack.h"
#include "KeyboardShortcuts.h"
#include "LocaleHelper.h"
#include "Note.h"
#include "PatternStore.h"
#include "ProjectJournal.h"
#include "Song.h"


namespace lmms
{

int AutomationClip::s_quantization = 1;
const float AutomationClip::DEFAULT_MIN_VALUE = 0;
const float AutomationClip::DEFAULT_MAX_VALUE = 1;


AutomationClip::AutomationClip( AutomationTrack * _auto_track ) :
	Clip( _auto_track ),
#if (QT_VERSION < QT_VERSION_CHECK(5,14,0))
	m_clipMutex(QMutex::Recursive),
#endif
	m_autoTrack( _auto_track ),
	m_objects(),
	m_tension( 1.0 ),
	m_progressionType( ProgressionType::Discrete ),
	m_dragging( false ),
	m_isRecording( false ),
	m_lastRecordedValue( 0 )
{
	changeLength( TimePos( 1, 0 ) );
}




AutomationClip::AutomationClip( const AutomationClip & _clip_to_copy ) :
	Clip(_clip_to_copy),
#if (QT_VERSION < QT_VERSION_CHECK(5,14,0))
	m_clipMutex(QMutex::Recursive),
#endif
	m_autoTrack( _clip_to_copy.m_autoTrack ),
	m_objects( _clip_to_copy.m_objects ),
	m_tension( _clip_to_copy.m_tension ),
	m_progressionType(_clip_to_copy.m_progressionType),
	m_dragging(false),
	m_isRecording(_clip_to_copy.m_isRecording),
	m_lastRecordedValue(0)
{
	// Locks the mutex of the copied AutomationClip to make sure it
	// doesn't change while it's being copied
	QMutexLocker m(&_clip_to_copy.m_clipMutex);

	for( timeMap::const_iterator it = _clip_to_copy.m_timeMap.begin();
				it != _clip_to_copy.m_timeMap.end(); ++it )
	{
		// Copies the automation node (in/out values and in/out tangents)
		m_timeMap[POS(it)] = it.value();
		// Sets the node's clip to this one
		m_timeMap[POS(it)].setClip(this);
	}
}

bool AutomationClip::addObject( AutomatableModel * _obj, bool _search_dup )
{
	QMutexLocker m(&m_clipMutex);

	if (_search_dup && std::find(m_objects.begin(), m_objects.end(), _obj) != m_objects.end())
	{
		return false;
	}

	// the automation track is unconnected and there is nothing in the track
	if (m_objects.empty() && hasAutomation() == false)
	{
		// then initialize first value
		putValue( TimePos(0), _obj->inverseScaledValue( _obj->value<float>() ), false );
	}

	m_objects.push_back(_obj);

	connect( _obj, SIGNAL(destroyed(lmms::jo_id_t)),
			this, SLOT(objectDestroyed(lmms::jo_id_t)),
						Qt::DirectConnection );

	emit dataChanged();

	return true;
}




void AutomationClip::setProgressionType(
					ProgressionType _new_progression_type )
{
	QMutexLocker m(&m_clipMutex);

	if ( _new_progression_type == ProgressionType::Discrete ||
		_new_progression_type == ProgressionType::Linear ||
		_new_progression_type == ProgressionType::CubicHermite )
	{
		m_progressionType = _new_progression_type;
		emit dataChanged();
	}
}




void AutomationClip::setTension( QString _new_tension )
{
	QMutexLocker m(&m_clipMutex);

	bool ok;
	float nt = LocaleHelper::toFloat(_new_tension, & ok);

	if( ok && nt > -0.01 && nt < 1.01 )
	{
		m_tension = nt;
	}
}




const AutomatableModel * AutomationClip::firstObject() const
{
	QMutexLocker m(&m_clipMutex);

	AutomatableModel* model;
	if (!m_objects.empty() && (model = m_objects.front()) != nullptr)
	{
		return model;
	}

	static FloatModel fm(0, DEFAULT_MIN_VALUE, DEFAULT_MAX_VALUE, 0.001f);
	return &fm;
}

const AutomationClip::objectVector& AutomationClip::objects() const
{
	QMutexLocker m(&m_clipMutex);

	return m_objects;
}




TimePos AutomationClip::timeMapLength() const
{
	QMutexLocker m(&m_clipMutex);

	TimePos one_bar = TimePos(1, 0);
	if (m_timeMap.isEmpty()) { return one_bar; }

	timeMap::const_iterator it = m_timeMap.end();
	auto last_tick = static_cast<tick_t>(POS(it - 1));
	// if last_tick is 0 (single item at tick 0)
	// return length as a whole bar to prevent disappearing Clip
	if (last_tick == 0) { return one_bar; }

	return TimePos(last_tick);
}




void AutomationClip::updateLength()
{
	// Technically it only matters if the clip has been resized from the right, but this
	// checks if it has been resized from either direction.
	if (getAutoResize())
	{
		// Using 1 bar as the min length for an un-resized clip. 
		// This does not prevent the user from resizing the clip to be less than a bar later on.
		changeLength(std::max(TimePos::ticksPerBar(), static_cast<tick_t>(timeMapLength())));
		setStartTimeOffset(TimePos(0));
	}
}




/**
 * @brief Puts an automation node on the timeMap with the given value.
 *        The inValue and outValue of the created node will be the same.
 * @param TimePos time to add the node to
 * @param Float inValue and outValue of the node
 * @param Boolean True to quantize the position (defaults to true)
 * @param Boolean True to ignore unquantized surrounding nodes (defaults to true)
 * @return TimePos of the recently added automation node
 */
TimePos AutomationClip::putValue(
	const TimePos & time,
	const float value,
	const bool quantPos,
	const bool ignoreSurroundingPoints
)
{
	QMutexLocker m(&m_clipMutex);

	cleanObjects();

	TimePos newTime = quantPos ? Note::quantized(time, quantization()) : time;
	newTime = std::max(TimePos(0), newTime);

	// Create a node or replace the existing one on newTime
	m_timeMap[newTime] = AutomationNode(this, value, newTime);

	timeMap::iterator it = m_timeMap.find(newTime);

	// Remove control points that are covered by the new points
	// quantization value. Control Key to override
	if (!ignoreSurroundingPoints)
	{
		// We need to check that to avoid removing nodes from
		// newTime + 1 to newTime (removing the node we are adding)
		if (quantization() > 1)
		{
			// Remove nodes between the quantization points, them not
			// being included
			removeNodes(newTime + 1, newTime + quantization() - 1);
		}
	}
	if (it != m_timeMap.begin()) { --it; }
	generateTangents(it, 3);

	updateLength();

	emit dataChanged();

	return newTime;
}




/**
 * @brief Puts an automation node on the timeMap with the given inValue
 *        and outValue.
 * @param TimePos time to add the node to
 * @param Float inValue of the node
 * @param Float outValue of the node
 * @param Boolean True to quantize the position (defaults to true)
 * @param Boolean True to ignore unquantized surrounding nodes (defaults to true)
 * @return TimePos of the recently added automation node
 */
TimePos AutomationClip::putValues(
	const TimePos & time,
	const float inValue,
	const float outValue,
	const bool quantPos,
	const bool ignoreSurroundingPoints
)
{
	QMutexLocker m(&m_clipMutex);

	cleanObjects();

	TimePos newTime = quantPos ? Note::quantized(time, quantization()) : time;
	newTime = std::max(TimePos(0), newTime);

	// Create a node or replace the existing one on newTime
	m_timeMap[newTime] = AutomationNode(this, inValue, outValue, newTime);

	timeMap::iterator it = m_timeMap.find(newTime);

	// Remove control points that are covered by the new points
	// quantization value. Control Key to override
	if (!ignoreSurroundingPoints)
	{
		// We need to check that to avoid removing nodes from
		// newTime + 1 to newTime (removing the node we are adding)
		if (quantization() > 1)
		{
			// Remove nodes between the quantization points, them not
			// being included
			removeNodes(newTime + 1, newTime + quantization() - 1);
		}
	}
	if (it != m_timeMap.begin()) { --it; }
	generateTangents(it, 3);

	updateLength();

	emit dataChanged();

	return newTime;
}




void AutomationClip::removeNode(const TimePos & time)
{
	QMutexLocker m(&m_clipMutex);

	cleanObjects();

	m_timeMap.remove( time );
	timeMap::iterator it = m_timeMap.lowerBound(time);
	if( it != m_timeMap.begin() )
	{
		--it;
	}
	generateTangents(it, 3);

	updateLength();

	emit dataChanged();
}




/**
 * @brief Removes all automation nodes between the given ticks
 * @param Int first tick of the range
 * @param Int second tick of the range
 */
void AutomationClip::removeNodes(const int tick0, const int tick1)
{
	if (tick0 == tick1)
	{
		removeNode(TimePos(tick0));
		return;
	}

	auto start = TimePos(std::min(tick0, tick1));
	auto end = TimePos(std::max(tick0, tick1));

	// Make a list of TimePos with nodes to be removed
	// because we can't simply remove the nodes from
	// the timeMap while we are iterating it.
	std::vector<TimePos> nodesToRemove;

	for (auto it = m_timeMap.lowerBound(start), endIt = m_timeMap.upperBound(end); it != endIt; ++it)
	{
		nodesToRemove.push_back(POS(it));
	}

	for (auto node: nodesToRemove)
	{
		removeNode(node);
	}
}




/**
 * @brief Resets the outValues of all automation nodes between the given ticks
 * @param Int first tick of the range
 * @param Int second tick of the range
 */
void AutomationClip::resetNodes(const int tick0, const int tick1)
{
	if (tick0 == tick1)
	{
		auto it = m_timeMap.find(TimePos(tick0));
		if (it != m_timeMap.end()) { it.value().resetOutValue(); }
		return;
	}

	auto start = TimePos(std::min(tick0, tick1));
	auto end = TimePos(std::max(tick0, tick1));

	for (auto it = m_timeMap.lowerBound(start), endIt = m_timeMap.upperBound(end); it != endIt; ++it)
	{
		it.value().resetOutValue();
	}
}




void AutomationClip::resetTangents(const int tick0, const int tick1)
{
	if (tick0 == tick1)
	{
		auto it = m_timeMap.find(TimePos(tick0));
		if (it != m_timeMap.end())
		{
			it.value().setLockedTangents(false);
			generateTangents(it, 1);
		}
		return;
	}

	TimePos start = TimePos(std::min(tick0, tick1));
	TimePos end = TimePos(std::max(tick0, tick1));

	for (auto it = m_timeMap.lowerBound(start), endIt = m_timeMap.upperBound(end); it != endIt; ++it)
	{
		it.value().setLockedTangents(false);
		generateTangents(it, 1);
	}
}




void AutomationClip::recordValue(TimePos time, float value)
{
	QMutexLocker m(&m_clipMutex);

	if( value != m_lastRecordedValue )
	{
		putValue(time - startTimeOffset(), value, true);
		m_lastRecordedValue = value;
	}
	else if( valueAt(time - startTimeOffset()) != value )
	{
		removeNode(time - startTimeOffset());
	}
}




/**
 * @brief Set the position of the point that is being dragged.
 *        Calling this function will also automatically set m_dragging to true.
 *        When applyDragValue() is called, m_dragging is set back to false.
 * @param TimePos of the node being dragged
 * @param Float with the value to assign to the point being dragged
 * @param Boolean. True to snip x position
 * @param Boolean. True to ignore unquantized surrounding nodes
 * @return TimePos with current time of the dragged value
 */
TimePos AutomationClip::setDragValue(
	const TimePos & time,
	const float value,
	const bool quantPos,
	const bool controlKey
)
{
	QMutexLocker m(&m_clipMutex);

	if (m_dragging == false)
	{
		TimePos newTime = quantPos ? Note::quantized(time, quantization()) : time;

		// We will keep the same outValue only if it's different from the
		// inValue
		m_dragKeepOutValue = false;

		// We will set the tangents back to what they were if the node had
		// its tangents locked
		m_dragLockedTan = false;

		// Check if we already have a node on the position we are dragging
		// and if we do, store the outValue so the discrete jump can be kept
		// and information about the tangents
		timeMap::iterator it = m_timeMap.find(newTime);
		if (it != m_timeMap.end())
		{
			// If we don't have a discrete jump, the outValue will be the
			// same as the inValue
			if (OFFSET(it) != 0)
			{
				m_dragKeepOutValue = true;
				m_dragOutValue = OUTVAL(it);
			}
			// For the tangents, we will only keep them if the tangents were
			// locked
			if (LOCKEDTAN(it))
			{
				m_dragLockedTan = true;
				m_dragInTan = INTAN(it);
				m_dragOutTan = OUTTAN(it);
			}
		}

		this->removeNode(newTime);
		m_oldTimeMap = m_timeMap;
		m_dragging = true;
	}

	//Restore to the state before it the point were being dragged
	m_timeMap = m_oldTimeMap;

	generateTangents();

	TimePos returnedPos;

	if (m_dragKeepOutValue)
	{
		returnedPos = this->putValues(time, value, m_dragOutValue, quantPos, controlKey);
	}
	else
	{
		returnedPos = this->putValue(time, value, quantPos, controlKey);
	}

	// Set the tangents on the newly created node if they were locked
	// before dragging
	if (m_dragLockedTan)
	{
		timeMap::iterator it = m_timeMap.find(returnedPos);
		if (it != m_timeMap.end())
		{
			it.value().setInTangent(m_dragInTan);
			it.value().setOutTangent(m_dragOutTan);
			it.value().setLockedTangents(true);
		}
	}

	return returnedPos;
}




/**
 * @brief After the point is dragged, this function is called to apply the change.
 */
void AutomationClip::applyDragValue()
{
	QMutexLocker m(&m_clipMutex);

	m_dragging = false;
}




float AutomationClip::valueAt( const TimePos & _time ) const
{
	QMutexLocker m(&m_clipMutex);

	if( m_timeMap.isEmpty() )
	{
		return 0;
	}

	// If we have a node at that time, just return its value
	if (m_timeMap.contains(_time))
	{
		// When the time is exactly the node's time, we want the inValue
		return m_timeMap[_time].getInValue();
	}

	// lowerBound returns next value with equal or greater key. Since we already
	// checked if the key contains a node, we know the returned node has a greater
	// key than _time. Therefore we take the previous element to calculate the current value
	timeMap::const_iterator v = m_timeMap.lowerBound(_time);

	if( v == m_timeMap.begin() )
	{
		return 0;
	}
	if( v == m_timeMap.end() )
	{
		// When the time is after the last node, we want the outValue of it
		return OUTVAL(v - 1);
	}

	return valueAt(v - 1, _time - POS(v - 1));
}




// This method will get the value at an offset from a node, so we use the outValue of
// that node and the inValue of the next node for the calculations.
float AutomationClip::valueAt( timeMap::const_iterator v, int offset ) const
{
	QMutexLocker m(&m_clipMutex);

	// We never use it with offset 0, but doesn't hurt to return a correct
	// value if we do
	if (offset == 0) { return INVAL(v); }

	if (m_progressionType == ProgressionType::Discrete)
	{
		return OUTVAL(v);
	}
	else if( m_progressionType == ProgressionType::Linear )
	{
		float slope =
			(INVAL(v + 1) - OUTVAL(v))
			/ (POS(v + 1) - POS(v));

		return OUTVAL(v) + offset * slope;
	}
	else /* ProgressionType::CubicHermite */
	{
		// Implements a Cubic Hermite spline as explained at:
		// http://en.wikipedia.org/wiki/Cubic_Hermite_spline#Unit_interval_.280.2C_1.29
		//
		// Note that we are not interpolating a 2 dimensional point over
		// time as the article describes.  We are interpolating a single
		// value: y.  To make this work we map the values of x that this
		// segment spans to values of t for t = 0.0 -> 1.0 and scale the
		// tangents _m1 and _m2
		int numValues = (POS(v + 1) - POS(v));
		float t = (float) offset / (float) numValues;
		float m1 = OUTTAN(v) * numValues * m_tension;
		float m2 = INTAN(v + 1) * numValues * m_tension;

		auto t2 = t * t, t3 = t2 * t;
		return (2 * t3 - 3 * t2 + 1) * OUTVAL(v)
			+ (t3 - 2 * t2 + t) * m1
			+ (-2 * t3 + 3 * t2) * INVAL(v + 1)
			+ (t3 - t2) * m2;
	}
}




float *AutomationClip::valuesAfter( const TimePos & _time ) const
{
	QMutexLocker m(&m_clipMutex);

	timeMap::const_iterator v = m_timeMap.lowerBound(_time);
	if( v == m_timeMap.end() || (v+1) == m_timeMap.end() )
	{
		return nullptr;
	}

	int numValues = POS(v + 1) - POS(v);
	auto ret = new float[numValues];

	for( int i = 0; i < numValues; i++ )
	{
		ret[i] = valueAt( v, i );
	}

	return ret;
}




void AutomationClip::flipY(int min, int max)
{
	QMutexLocker m(&m_clipMutex);

	bool changedTimeMap = false;

	for (auto it = m_timeMap.begin(); it != m_timeMap.end(); ++it)
	{
		// Get distance from IN/OUT values to max value
		float inValDist = max - INVAL(it);
		float outValDist = max - OUTVAL(it);

		// To flip, that will be the new distance between
		// the IN/OUT values and the min value
		it.value().setInValue(min + inValDist);
		it.value().setOutValue(min + outValDist);

		changedTimeMap = true;
	}

	if (changedTimeMap)
	{
		generateTangents();
		emit dataChanged();
	}
}




void AutomationClip::flipY()
{
	flipY(getMin(), getMax());
}




void AutomationClip::flipX(int start, int end)
{
	QMutexLocker m(&m_clipMutex);

	timeMap::const_iterator firstIterator = m_timeMap.lowerBound(0);

	if (firstIterator == m_timeMap.end()) { return; }

	if (start == -1 && end == -1) { start = 0; end = length() - startTimeOffset(); }
	else if (!(end >= 0 && start >= 0 && end > start)) { return; }

	// Temporary map where we will store the flipped version
	// of our clip
	timeMap tempMap;

	for (auto it = m_timeMap.begin(); it != m_timeMap.end(); ++it)
	{
		if (POS(it) < start || POS(it) > end)
		{
			tempMap[POS(it)] = *it;
		}
		else
		{
			// If the first node in the clip is not at the start, it can break things when clipping, so
			// we have to set its in value to 0.
			if (it == firstIterator && POS(firstIterator) > 0)
			{
				tempMap[end - (POS(it) - start)] = AutomationNode(this, OUTVAL(it), 0, end - (POS(it) - start));
			}
			else
			{
				tempMap[end - (POS(it) - start)] = AutomationNode(this, OUTVAL(it), INVAL(it), end - (POS(it) - start));
			}
		}
	}

	if (m_timeMap.contains(start) && m_timeMap.contains(end))
	{
		tempMap[start] = AutomationNode(this, m_timeMap[start].getInValue(), m_timeMap[end].getInValue(), start);
		tempMap[end] = AutomationNode(this, m_timeMap[start].getOutValue(), m_timeMap[end].getOutValue(), end);
	}
	else if (m_timeMap.contains(start))
	{
		tempMap[start] = AutomationNode(this, m_timeMap[start].getInValue(), valueAt(end), start);
		tempMap[end] = AutomationNode(this, m_timeMap[start].getOutValue(), valueAt(end), end);
	}
	else if (m_timeMap.contains(end))
	{
		tempMap[start] = AutomationNode(this, valueAt(start), m_timeMap[end].getInValue(), start);
		tempMap[end] = AutomationNode(this, valueAt(start), m_timeMap[end].getOutValue(), end);
	}
	else
	{
		tempMap[start] = AutomationNode(this, valueAt(start), valueAt(end), start);
		tempMap[end] = AutomationNode(this, valueAt(start), valueAt(end), end);
	}

	m_timeMap.clear();

	m_timeMap = tempMap;

	cleanObjects();

	generateTangents();
	emit dataChanged();
}




void AutomationClip::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	QMutexLocker m(&m_clipMutex);

	_this.setAttribute( "pos", startPosition() );
	_this.setAttribute( "len", length() );
	_this.setAttribute( "name", name() );
	_this.setAttribute( "prog", QString::number( static_cast<int>(progressionType()) ) );
	_this.setAttribute( "tens", QString::number( getTension() ) );
	_this.setAttribute( "mute", QString::number( isMuted() ) );
	_this.setAttribute("off", startTimeOffset());
	_this.setAttribute("autoresize", QString::number(getAutoResize()));

	if (const auto& c = color())
	{
		_this.setAttribute("color", c->name());
	}

	for( timeMap::const_iterator it = m_timeMap.begin();
						it != m_timeMap.end(); ++it )
	{
		QDomElement element = _doc.createElement( "time" );
		element.setAttribute("pos", POS(it));
		element.setAttribute("value", INVAL(it));
		element.setAttribute("outValue", OUTVAL(it));
		element.setAttribute("inTan", INTAN(it));
		element.setAttribute("outTan", OUTTAN(it));
		element.setAttribute("lockedTan", static_cast<int>(LOCKEDTAN(it)));
		_this.appendChild( element );
	}

	for (const auto& object : m_objects)
	{
		if (object)
		{
			QDomElement element = _doc.createElement( "object" );
			element.setAttribute("id", ProjectJournal::idToSave(object->id()));
			_this.appendChild(element);
		}
	}
}




void AutomationClip::loadSettings( const QDomElement & _this )
{
	QMutexLocker m(&m_clipMutex);

	// Legacy compatibility: Previously tangents were not stored in
	// the project file. So if any node doesn't have tangent information
	// we will generate the tangents
	bool shouldGenerateTangents = false;

	clear();

	movePosition( _this.attribute( "pos" ).toInt() );
	setName( _this.attribute( "name" ) );
	setProgressionType( static_cast<ProgressionType>( _this.attribute(
							"prog" ).toInt() ) );
	setTension( _this.attribute( "tens" ) );
	setMuted(_this.attribute( "mute", QString::number( false ) ).toInt() );
	setAutoResize(_this.attribute("autoresize", "1").toInt());
	setStartTimeOffset(_this.attribute("off").toInt());

	for( QDomNode node = _this.firstChild(); !node.isNull();
						node = node.nextSibling() )
	{
		QDomElement element = node.toElement();
		if( element.isNull()  )
		{
			continue;
		}
		if( element.tagName() == "time" )
		{
			int timeMapPos = element.attribute("pos").toInt();
			float timeMapInValue = LocaleHelper::toFloat(element.attribute("value"));
			float timeMapOutValue = LocaleHelper::toFloat(element.attribute("outValue"));

			m_timeMap[timeMapPos] = AutomationNode(this, timeMapInValue, timeMapOutValue, timeMapPos);

			// Load tangents if there is information about it (it's enough to check for either inTan or outTan)
			if (element.hasAttribute("inTan"))
			{
				float inTan = LocaleHelper::toFloat(element.attribute("inTan"));
				float outTan = LocaleHelper::toFloat(element.attribute("outTan"));
				bool lockedTan = static_cast<bool>(element.attribute("lockedTan", "0").toInt());

				m_timeMap[timeMapPos].setInTangent(inTan);
				m_timeMap[timeMapPos].setOutTangent(outTan);
				m_timeMap[timeMapPos].setLockedTangents(lockedTan);
			}
			else
			{
				shouldGenerateTangents = true;
			}
		}
		else if( element.tagName() == "object" )
		{
			m_idsToResolve.push_back(element.attribute("id").toInt());
		}
	}
	
	if (_this.hasAttribute("color"))
	{
		setColor(QColor{_this.attribute("color")});
	}

	int len = _this.attribute( "len" ).toInt();
	if( len <= 0 )
	{
		// TODO: Handle with an upgrade method
		updateLength();
	}
	else
	{
		changeLength( len );
	}

	if (shouldGenerateTangents) { generateTangents(); }
}




QString AutomationClip::name() const
{
	QMutexLocker m(&m_clipMutex);

	if( !Clip::name().isEmpty() )
	{
		return Clip::name();
	}
	if (!m_objects.empty() && m_objects.front() != nullptr)
	{
		return m_objects.front()->fullDisplayName();
	}
	return tr( "Drag a control while pressing <%1>" ).arg(UI_COPY_KEY);
}




gui::ClipView * AutomationClip::createView( gui::TrackView * _tv )
{
	QMutexLocker m(&m_clipMutex);

	return new gui::AutomationClipView( this, _tv );
}





bool AutomationClip::isAutomated( const AutomatableModel * _m )
{
	auto l = combineAllTracks();
	for (const auto track : l)
	{
		if (track->type() == Track::Type::Automation || track->type() == Track::Type::HiddenAutomation)
		{
			for (const auto& clip : track->getClips())
			{
				const auto a = dynamic_cast<const AutomationClip*>(clip);
				if( a && a->hasAutomation() )
				{
					for (const auto& object : a->m_objects)
					{
						if (object == _m)
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}


/**
 * @brief returns a list of all the automation clips that are connected to a specific model
 * @param _m the model we want to look for
 */
std::vector<AutomationClip *> AutomationClip::clipsForModel(const AutomatableModel* _m)
{
	std::vector<AutomationClip *> clips;
	auto l = combineAllTracks();

	// go through all tracks...
	for (const auto track : l)
	{
		// we want only automation tracks...
		if (track->type() == Track::Type::Automation || track->type() == Track::Type::HiddenAutomation )
		{
			// go through all the clips...
			for (const auto& trackClip : track->getClips())
			{
				auto a = dynamic_cast<AutomationClip*>(trackClip);
				// check that the clip has automation
				if( a && a->hasAutomation() )
				{
					// now check is the clip is connected to the model we want by going through all the connections
					// of the clip
					bool has_object = false;
					for (const auto& object : a->m_objects)
					{
						if (object == _m)
						{
							has_object = true;
						}
					}
					// if the clips is connected to the model, add it to the list
					if (has_object) { clips.push_back(a); }
				}
			}
		}
	}
	return clips;
}



AutomationClip * AutomationClip::globalAutomationClip(
							AutomatableModel * _m )
{
	AutomationTrack * t = Engine::getSong()->globalAutomationTrack();
	for (const auto& clip : t->getClips())
	{
		auto a = dynamic_cast<AutomationClip*>(clip);
		if( a )
		{
			for (const auto& object : a->m_objects)
			{
				if (object == _m) { return a; }
			}
		}
	}

	auto a = new AutomationClip(t);
	a->addObject( _m, false );
	return a;
}




void AutomationClip::resolveAllIDs()
{
	auto l = combineAllTracks();
	for (const auto& track : l)
	{
		if (track->type() == Track::Type::Automation || track->type() == Track::Type::HiddenAutomation)
		{
			for (const auto& clip : track->getClips())
			{
				auto a = dynamic_cast<AutomationClip*>(clip);
				if( a )
				{
					for (const auto& id : a->m_idsToResolve)
					{
						JournallingObject* o = Engine::projectJournal()->journallingObject(id);
						if( o && dynamic_cast<AutomatableModel *>( o ) )
						{
							a->addObject( dynamic_cast<AutomatableModel *>( o ), false );
						}
						else
						{
							// FIXME: Remove this block once the automation system gets fixed
							// This is a temporary fix for https://github.com/LMMS/lmms/issues/3781
							o = Engine::projectJournal()->journallingObject(ProjectJournal::idFromSave(id));
							if( o && dynamic_cast<AutomatableModel *>( o ) )
							{
								a->addObject( dynamic_cast<AutomatableModel *>( o ), false );
							}
							else
							{
								// FIXME: Remove this block once the automation system gets fixed
								// This is a temporary fix for https://github.com/LMMS/lmms/issues/4781
								o = Engine::projectJournal()->journallingObject(ProjectJournal::idToSave(id));
								if( o && dynamic_cast<AutomatableModel *>( o ) )
								{
									a->addObject( dynamic_cast<AutomatableModel *>( o ), false );
								}
							}
						}
					}
					a->m_idsToResolve.clear();
					a->dataChanged();
				}
			}
		}
	}
}




void AutomationClip::clear()
{
	QMutexLocker m(&m_clipMutex);

	m_timeMap.clear();

	emit dataChanged();
}




void AutomationClip::objectDestroyed( jo_id_t _id )
{
	QMutexLocker m(&m_clipMutex);

	// TODO: distict between temporary removal (e.g. LADSPA controls
	// when switching samplerate) and real deletions because in the latter
	// case we had to remove ourselves if we're the global automation
	// clip of the destroyed object
	m_idsToResolve.push_back(_id);

	for (auto objIt = m_objects.begin(); objIt != m_objects.end(); objIt++)
	{
		Q_ASSERT( !(*objIt).isNull() );
		if( (*objIt)->id() == _id )
		{
			//Assign to objIt so that this loop work even break; is removed.
			objIt = m_objects.erase( objIt );
			break;
		}
	}

	emit dataChanged();
}




void AutomationClip::cleanObjects()
{
	QMutexLocker m(&m_clipMutex);

	for( objectVector::iterator it = m_objects.begin(); it != m_objects.end(); )
	{
		if( *it )
		{
			++it;
		}
		else
		{
			it = m_objects.erase( it );
		}
	}
}




void AutomationClip::generateTangents()
{
	generateTangents(m_timeMap.begin(), m_timeMap.size());
}




// We have two tangents, one for the left side of the node and one for the right side
// of the node (in case we have discrete value jumps in the middle of a curve).
// If the inValue and outValue of a node are the same, consequently the inTangent and
// outTangent values of the node will be the same too.
void AutomationClip::generateTangents(timeMap::iterator it, int numToGenerate)
{
	QMutexLocker m(&m_clipMutex);

	for (int i = 0; i < numToGenerate && it != m_timeMap.end(); ++i, ++it)
	{
		// Skip the node if it has locked tangents (were manually edited)
		if (LOCKEDTAN(it))
		{
			continue;
		}

		if (it + 1 == m_timeMap.end())
		{
			// Previously, the last value's tangent was always set to 0. That logic was kept for both tangents
			// of the last node
			it.value().setInTangent(0);
			it.value().setOutTangent(0);
		}
		else if (it == m_timeMap.begin())
		{
			// On the first node there's no curve behind it, so we will only calculate the outTangent
			// and inTangent will be set to 0.
			float tangent = (INVAL(it + 1) - OUTVAL(it)) / (POS(it + 1) - POS(it));
			it.value().setInTangent(0);
			it.value().setOutTangent(tangent);
		}
		else
		{
			// When we are in a node that is in the middle of two other nodes, we need to check if we
			// have a discrete jump at this node. If we do not, then we can calculate the tangents normally.
			// If we do have a discrete jump, then we have to calculate the tangents differently for each side
			// of the curve.
			// TODO: This behavior means that a very small difference between the inValue and outValue can
			// result in a big change in the curve. In the future, allowing the user to manually adjust
			// the tangents would be better.
			if (OFFSET(it) == 0)
			{
				float inTangent = (INVAL(it + 1) - OUTVAL(it - 1)) / (POS(it + 1) - POS(it - 1));
				it.value().setInTangent(inTangent);
				// inTangent == outTangent in this case
				it.value().setOutTangent(inTangent);
			}
			else
			{
				// Calculate the left side of the curve
				float inTangent = (INVAL(it) - OUTVAL(it - 1)) / (POS(it) - POS(it - 1));
				// Calculate the right side of the curve
				float outTangent = (INVAL(it + 1) - OUTVAL(it)) / (POS(it + 1) - POS(it));
				it.value().setInTangent(inTangent);
				it.value().setOutTangent(outTangent);
			}
		}
	}
}

std::vector<Track*> AutomationClip::combineAllTracks()
{
	std::vector<Track*> combinedTrackList;

	auto& songTracks = Engine::getSong()->tracks();
	auto& patternStoreTracks = Engine::patternStore()->tracks();

	combinedTrackList.insert(combinedTrackList.end(), songTracks.begin(), songTracks.end());
	combinedTrackList.insert(combinedTrackList.end(), patternStoreTracks.begin(), patternStoreTracks.end());
	combinedTrackList.push_back(Engine::getSong()->globalAutomationTrack());

	return combinedTrackList;
}

} // namespace lmms
