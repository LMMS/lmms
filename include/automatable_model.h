/*
 * automatable_model.h - declaration of class automatableModel
 *
 * Copyright (c) 2007-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "journalling_object.h"
#include "mv_base.h"
#include "controller_connection.h"


// simple way to map a property of a view to a model
#define mapPropertyFromModelPtr(type,getfunc,setfunc,modelname)		\
		public:							\
			inline type getfunc( void ) const		\
			{						\
				return( (type) modelname->value() );	\
			}						\
		public slots:						\
			inline void setfunc( const type _val )		\
			{						\
				modelname->setValue( _val );		\
			}

#define mapPropertyFromModel(type,getfunc,setfunc,modelname)		\
		public:							\
			inline type getfunc( void ) const		\
			{						\
				return( (type) modelname.value() );	\
			}						\
		public slots:						\
			inline void setfunc( const type _val )		\
			{						\
				modelname.setValue( _val );		\
			}



class EXPORT automatableModel : public model, public journallingObject
{
	Q_OBJECT
public:
	typedef QVector<automatableModel *> autoModelVector;

	enum DataType
	{
		Float,
		Integer,
		Bool
	} ;

	automatableModel( DataType _type,
				const float _val = 0,
				const float _min = 0,
				const float _max = 0,
				const float _step = 0,
				::model * _parent = NULL,
				const QString & _display_name = QString::null,
				bool _default_constructed = FALSE );

	virtual ~automatableModel();


	static inline float copiedValue( void )
	{
		return( __copiedValue );
	}

	bool isAutomated( void ) const;

	inline controllerConnection * getControllerConnection( void ) const
	{
		return m_controllerConnection;
	}


	void setControllerConnection( controllerConnection * _c );


	template<class T>
	static inline T minEps( void )
	{
		return( 1 );
	}

	template<class T>
	static inline T castValue( const float _v )
	{
		return( static_cast<T>( _v ) );
	}

	template<class T>
	inline T value( int _frameOffset = 0 ) const
	{
		if( m_controllerConnection != NULL )
		{
			return minValue<T>() +
				castValue<T>( m_range * 
					 m_controllerConnection->currentValue(
							_frameOffset ) );
		}

		return castValue<T>( m_value );
	}


	template<class T>
	inline T initValue( void ) const
	{
		return castValue<T>( m_initValue );
	}

	template<class T>
	inline T minValue( void ) const
	{
		return castValue<T>( m_minValue );
	}

	template<class T>
	inline T maxValue( void ) const
	{
		return castValue<T>( m_maxValue );
	}

	template<class T>
	inline T step( void ) const
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

	inline float range( void ) const
	{
		return( m_range );
	}

	void setRange( const float _min, const float _max,
							const float _step = 1 );

	void setStep( const float _step );

	static void linkModels( automatableModel * _m1,
						automatableModel * _m2 );
	static void unlinkModels( automatableModel * _m1,
						automatableModel * _m2 );

	virtual void saveSettings( QDomDocument & _doc,
					QDomElement & _this,
					const QString & _name = "value" );

	virtual void loadSettings( const QDomElement & _this,
					const QString & _name = "value" );

	virtual QString nodeName( void ) const
	{
		return( "automatablemodel" );
	}

	void prepareJournalEntryFromOldVal( void );

	void addJournalEntryFromOldToCurVal( void );


	QString displayValue( const float _val ) const
	{
		switch( m_dataType )
		{
			case Float: return( QString::number(
					castValue<float>( _val ) ) );
			case Integer: return( QString::number(
					castValue<int>( _val ) ) );
			case Bool: return( QString::number(
					castValue<bool>( _val ) ) );
		}
		return( "0" );
	}


	virtual QString displayName( void ) const
	{
		return m_displayName;
	}

	virtual void setDisplayName( const QString & _display_name )
	{
		m_displayName = _display_name;
	}



public slots:
	virtual void reset( void );
	virtual void copyValue( void );
	virtual void pasteValue( void );


protected:
	virtual void redoStep( journalEntry & _je );
	virtual void undoStep( journalEntry & _je );

	float fittedValue( float _value ) const;


private:
	void linkModel( automatableModel * _model );
	void unlinkModel( automatableModel * _model );


	DataType m_dataType;
	float m_value;
	float m_initValue;
	float m_minValue;
	float m_maxValue;
	float m_step;
	float m_range;

	// most objects will need this temporarily (until sampleExact is standard)
	float m_oldValue;
	QString m_displayName;
	bool m_journalEntryReady;

	autoModelVector m_linkedModels;


	controllerConnection * m_controllerConnection;


	static float __copiedValue;


signals:
	void initValueChanged( float _val );

} ;


#define defaultTypedMethods(type)					\
	inline type value( int _frameOffset = 0 ) const			\
	{								\
		return( automatableModel::value<type>( _frameOffset ) );\
	}								\
									\
	inline type minValue( void ) const				\
	{								\
		return( automatableModel::minValue<type>() );		\
	}								\
									\
	inline type maxValue( void ) const				\
	{								\
		return( automatableModel::maxValue<type>() );		\
	}								\


// some typed automatableModel-definitions

class floatModel : public automatableModel
{
public:
	floatModel( float _val = 0, float _min = 0, float _max = 0,
			float _step = 0, ::model * _parent = NULL,
			const QString & _display_name  = QString::null,
			bool _default_constructed = FALSE ) :
		automatableModel( Float, _val, _min, _max, _step,
				_parent, _display_name, _default_constructed )
	{
	}

	defaultTypedMethods(float);

} ;


class intModel : public automatableModel
{
public:
	intModel( int _val = 0, int _min = 0, int _max = 0,
			::model * _parent = NULL,
			const QString & _display_name  = QString::null,
			bool _default_constructed = FALSE ) :
		automatableModel( Integer, _val, _min, _max, 1,
				_parent, _display_name, _default_constructed )
	{
	}

	defaultTypedMethods(int);

} ;


class boolModel : public automatableModel
{
public:
	boolModel(  const bool _val = FALSE,
				::model * _parent = NULL,
				const QString & _display_name  = QString::null,
				bool _default_constructed = FALSE ) : 
		automatableModel( Bool, _val, FALSE, TRUE, 1,
				_parent, _display_name, _default_constructed )
	{
	}

	defaultTypedMethods(bool);

} ;


#endif

