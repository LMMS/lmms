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
#include "level_object.h"
#include "mv_base.h"
#include "controller_connection.h"

#include <QtCore/QPointer>
#include <QtCore/QObject>



class automationPattern;
class track;

// simple way to map a property of a view to a model
#define mapPropertyFromModelPtr(type,getfunc,setfunc,modelname)	\
		public:						\
			inline type getfunc( void ) const	\
			{					\
				return( modelname->value() );	\
			}					\
		public slots:					\
			inline void setfunc( const type _val )	\
			{					\
				modelname->setValue( _val );	\
			}

#define mapPropertyFromModel(type,getfunc,setfunc,modelname)	\
		public:						\
			inline type getfunc( void ) const	\
			{					\
				return( modelname.value() );	\
			}					\
		public slots:					\
			inline void setfunc( const type _val )	\
			{					\
				modelname.setValue( _val );	\
			}


template<typename T, typename EDIT_STEP_TYPE = T>
class automatableModel : public model, public journallingObject,
							public levelObject
{
public:
	typedef automatableModel<T, EDIT_STEP_TYPE> autoModel;

	automatableModel( const T _val = 0,
				const T _min = 0,
				const T _max = 0,
				const T _step = defaultRelStep(),
				::model * _parent = NULL,
				bool _default_constructed = FALSE );

	virtual ~automatableModel();

	static inline T minRelStep( void )
	{
		return( 1 );
	}

	static inline T defaultRelStep( void )
	{
		return( 1 );
	}

	static inline T minEps( void )
	{
		return( 1 );
	}

	template<class V>
	static inline T castValue( V _v )
	{
		return( static_cast<T>( _v ) );
	}

	inline virtual T value( int _frameOffset = 0 ) const
	{
		if( m_controllerConnection != NULL )
		{
			return minValue() + castValue( m_range * 
				 m_controllerConnection->currentValue( _frameOffset ) );
		}

		return m_value;
	}


	inline controllerConnection * getControllerConnection( void ) const
	{
		return m_controllerConnection;
	}


	inline void setControllerConnection( controllerConnection * _c )
	{
		m_controllerConnection = _c;
		QObject::connect( m_controllerConnection, SIGNAL( valueChanged() ),
				this, SIGNAL( dataChanged() ) );
	}


	inline virtual T initValue( void ) const
	{
		return( m_initValue );
	}

	inline virtual T minValue( void ) const
	{
		return( m_minValue );
	}

	inline virtual T maxValue( void ) const
	{
		return( m_maxValue );
	}

	inline virtual T step( void ) const
	{
		return( m_step );
	}

	inline int curLevel( void ) const
	{
		return( m_curLevel );
	}

	T fittedValue( T _value ) const;

	inline T levelToValue( int _level ) const
	{
		return( fittedValue( _level * m_step ) );
	}

	virtual void setInitValue( const T _value );

	virtual void setValue( const T _value );

	inline virtual void incValue( int _steps )
	{
		setValue( m_value + _steps * m_step );
	}

	virtual void setRange( const T _min, const T _max,
					const T _step = defaultRelStep() );

	virtual void setStep( const T _step );

	static void linkModels( autoModel * _model1, autoModel * _model2 );

	static void unlinkModels( autoModel * _model1, autoModel * _model2 );

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
					QDomElement & _this,
					const QString & _name = "value" );

	virtual void FASTCALL loadSettings( const QDomElement & _this,
					const QString & _name = "value" );

	virtual QString nodeName( void ) const
	{
		return( "automatablemodel" );
	}

	inline automationPattern * getAutomationPattern( void );

	inline void setTrack( track * _track )
	{
		m_track = _track;
	}

	inline bool nullTrack( void )
	{
		return( m_track == NULL );
	}

	void initAutomationPattern( void )
	{
		m_automationPattern = new automationPattern( NULL, this );
	}

	void prepareJournalEntryFromOldVal( void );

	void addJournalEntryFromOldToCurVal( void );


protected:
	virtual void redoStep( journalEntry & _je );

	virtual void undoStep( journalEntry & _je );

	inline void setFirstValue( void );


private:
	controllerConnection * m_controllerConnection;
	T m_value;
	T m_initValue;
	T m_minValue;
	T m_maxValue;
	T m_range;
	T m_step;
	int m_curLevel;

	QPointer<automationPattern> m_automationPattern;
	track * m_track;

	// most objects will need this temporarily
	T m_oldValue;
	bool m_journalEntryReady;

	typedef QVector<autoModel *> autoModelVector;
	autoModelVector m_linkedModels;

	inline void linkModel( autoModel * _model );

	inline void unlinkModel( autoModel * _model );

	static T attributeValue( QString _value );

	inline void syncAutomationPattern( void );

	void setLevel( int _level );

	inline int level( T _value ) const
	{
		return( (int)roundf( _value / (float)m_step ) );
	}

	QString levelToLabel( int _level ) const
	{
		return( QString::number( levelToValue( _level ) ) );
	}

	int labelToLevel( QString _label )
	{
		return( level( attributeValue( _label ) ) );
	}
/*
public slots:

	void changeData( void )
	{
		emit dataChanged();
	}
*/

} ;



template<typename T, typename EDIT_STEP_TYPE = T>
class automatableModelView : public modelView
{
public:
	typedef automatableModel<T, EDIT_STEP_TYPE> autoModel;
	typedef automatableModelView<T, EDIT_STEP_TYPE> autoModelView;

	automatableModelView( ::model * _model ) :
		modelView( _model )
	{
	}

	// some basic functions for convenience
	autoModel * model( void )
	{
		return( castModel<autoModel>() );
	}

	const autoModel * model( void ) const
	{
		return( castModel<autoModel>() );
	}

	inline virtual T value( void ) const
	{
		return( model() ? model()->value() : 0 );
	}

	inline virtual void setValue( const T _value )
	{
		if( model() )
		{
			model()->setValue( _value );
		}
	}

} ;



#define generateModelPrimitive(type,type2)					\
		typedef automatableModel<type,type2> type##Model;		\
		typedef automatableModelView<type,type2> type##ModelView;	\

// some model-primitives

generateModelPrimitive(float,float);
generateModelPrimitive(int,int);

class boolModel : public automatableModel<bool, signed char>
{
public:
	boolModel(  const bool _val = FALSE,
				::model * _parent = NULL,
				bool _default_constructed = FALSE ) : 
	autoModel( _val, FALSE, TRUE, defaultRelStep(), _parent,
							_default_constructed )
	{
	}

} ;

typedef automatableModelView<bool, signed char> boolModelView;


#endif

