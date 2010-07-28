/*
 * AutomatableModel.h - declaration of class AutomatableModel
 *
 * Copyright (c) 2007-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _AUTOMATABLE_MODEL_H
#define _AUTOMATABLE_MODEL_H

#include <math.h>

#include "JournallingObject.h"
#include "Model.h"


// simple way to map a property of a view to a model
#define mapPropertyFromModelPtr(type,getfunc,setfunc,modelname)	\
		public:													\
			inline type getfunc() const							\
			{													\
				return (type) modelname->value();				\
			}													\
		public slots:											\
			inline void setfunc( const type _val )				\
			{													\
				modelname->setValue( _val );					\
			}

#define mapPropertyFromModel(type,getfunc,setfunc,modelname)	\
		public:													\
			inline type getfunc() const							\
			{													\
				return (type) modelname.value();				\
			}													\
		public slots:											\
			inline void setfunc( const type _val )				\
			{													\
				modelname.setValue( (float) _val );				\
			}



class ControllerConnection;


class EXPORT AutomatableModel : public Model, public JournallingObject
{
	Q_OBJECT
public:
	typedef QVector<AutomatableModel *> AutoModelVector;

	enum DataType
	{
		Float,
		Integer,
		Bool
	} ;

	AutomatableModel( DataType _type,
				const float _val = 0,
				const float _min = 0,
				const float _max = 0,
				const float _step = 0,
				::Model * _parent = NULL,
				const QString & _display_name = QString(),
				bool _default_constructed = false );

	virtual ~AutomatableModel();


	static inline float copiedValue()
	{
		return __copiedValue;
	}

	bool isAutomated() const;

	inline ControllerConnection * getControllerConnection() const
	{
		return m_controllerConnection;
	}


	void setControllerConnection( ControllerConnection * _c );


	template<class T>
	static inline T castValue( const float _v )
	{
		return (T)( _v );
	}

	template<bool>
	static inline bool castValue( const float _v )
	{
		return ( qRound( _v ) != 0 );
	}


	template<class T>
	inline T value( int _frameOffset = 0 ) const
	{
		if( unlikely( m_hasLinkedModels ||
					m_controllerConnection != NULL ) )
		{
			return castValue<T>( controllerValue( _frameOffset ) );
		}

		return castValue<T>( m_value );
	}

	float controllerValue( int _frameOffset ) const;


	template<class T>
	inline T initValue() const
	{
		return castValue<T>( m_initValue );
	}

	template<class T>
	inline T minValue() const
	{
		return castValue<T>( m_minValue );
	}

	template<class T>
	inline T maxValue() const
	{
		return castValue<T>( m_maxValue );
	}

	template<class T>
	inline T step() const
	{
		return castValue<T>( m_step );
	}


	void setInitValue( const float _value );

	void setAutomatedValue( const float _value );
	void setValue( const float _value );

	inline void incValue( int _steps )
	{
		setValue( m_value + _steps * m_step );
	}

	inline float range() const
	{
		return m_range;
	}

	void setRange( const float _min, const float _max,
							const float _step = 1 );

	void setStep( const float _step );

	static void linkModels( AutomatableModel * _m1,
						AutomatableModel * _m2 );
	static void unlinkModels( AutomatableModel * _m1,
						AutomatableModel * _m2 );

	void unlinkAllModels( );

	virtual void saveSettings( QDomDocument & _doc,
					QDomElement & _this,
				const QString & _name = QString( "value" ) );

	virtual void loadSettings( const QDomElement & _this,
				const QString & _name = QString( "value" ) );

	virtual QString nodeName() const
	{
		return "automatablemodel";
	}

	void prepareJournalEntryFromOldVal();

	void addJournalEntryFromOldToCurVal();


	QString displayValue( const float _val ) const
	{
		switch( m_dataType )
		{
			case Float: return QString::number( castValue<float>( _val ) );
			case Integer: return QString::number( castValue<int>( _val ) );
			case Bool: return QString::number( castValue<bool>( _val ) );
		}
		return "0";
	}

	inline bool armed(){ return m_armed; }
	inline void setArmed( bool _armed ){ m_armed = _armed; }

	inline bool hasLinkedModels() { return m_hasLinkedModels; }


public slots:
	virtual void reset();
	virtual void copyValue();
	virtual void pasteValue();
	void unlinkControllerConnection();
	void handleDataChanged();

protected:
	virtual void redoStep( JournalEntry & _je );
	virtual void undoStep( JournalEntry & _je );

	float fittedValue( float _value ) const;


private:
	void linkModel( AutomatableModel * _model );
	void unlinkModel( AutomatableModel * _model );


	DataType m_dataType;
	float m_value;
	float m_initValue;
	float m_minValue;
	float m_maxValue;
	float m_step;
	float m_range;

	// most objects will need this temporarily (until sampleExact is
	// standard)
	float m_oldValue;
	bool m_journalEntryReady;
	int m_setValueDepth;

	AutoModelVector m_linkedModels;
	bool m_hasLinkedModels;


	ControllerConnection * m_controllerConnection;

	bool m_armed; // record this model during automation recording?

	static float __copiedValue;


signals:
	void initValueChanged( float _val );
	void destroyed( jo_id_t _id );

} ;





#define defaultTypedMethods(type)								\
	inline type value( int _frameOffset = 0 ) const				\
	{															\
		return AutomatableModel::value<type>( _frameOffset );	\
	}															\
																\
	inline type minValue() const								\
	{															\
		return AutomatableModel::minValue<type>();				\
	}															\
																\
	inline type maxValue() const								\
	{															\
		return AutomatableModel::maxValue<type>();				\
	}

// some typed AutomatableModel-definitions

class FloatModel : public AutomatableModel
{
public:
	FloatModel( float _val = 0, float _min = 0, float _max = 0,
			float _step = 0, ::Model * _parent = NULL,
			const QString & _display_name = QString(),
			bool _default_constructed = false ) :
		AutomatableModel( Float, _val, _min, _max, _step,
				_parent, _display_name, _default_constructed )
	{
	}

	defaultTypedMethods(float);

} ;


class IntModel : public AutomatableModel
{
public:
	IntModel( int _val = 0, int _min = 0, int _max = 0,
			::Model * _parent = NULL,
			const QString & _display_name = QString(),
			bool _default_constructed = false ) :
		AutomatableModel( Integer, _val, _min, _max, 1,
				_parent, _display_name, _default_constructed )
	{
	}

	defaultTypedMethods(int);

} ;


class BoolModel : public AutomatableModel
{
public:
	BoolModel( const bool _val = false, ::Model * _parent = NULL,
				const QString & _display_name = QString(),
				bool _default_constructed = false ) : 
		AutomatableModel( Bool, _val, false, true, 1,
				_parent, _display_name, _default_constructed )
	{
	}

	defaultTypedMethods(bool);

} ;


#endif

