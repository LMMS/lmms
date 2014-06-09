/*
 * AutomatableModel.h - declaration of class AutomatableModel
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef AUTOMATABLE_MODEL_H
#define AUTOMATABLE_MODEL_H

#include <math.h>

#include "JournallingObject.h"
#include "Model.h"
#include "MidiTime.h"
#include "ValueBuffer.h"


// simple way to map a property of a view to a model
#define mapPropertyFromModelPtr(type,getfunc,setfunc,modelname)	\
		public:													\
			type getfunc() const								\
			{													\
				return (type) modelname->value();				\
			}													\
		public slots:											\
			void setfunc( const type val )						\
			{													\
				modelname->setValue( val );						\
			}

#define mapPropertyFromModel(type,getfunc,setfunc,modelname)	\
		public:													\
			type getfunc() const								\
			{													\
				return (type) modelname.value();				\
			}													\
		public slots:											\
			void setfunc( const type val )						\
			{													\
				modelname.setValue( (float) val );				\
			}



class ControllerConnection;

class EXPORT AutomatableModel : public Model, public JournallingObject
{
	Q_OBJECT
public:
	typedef QVector<AutomatableModel *> AutoModelVector;

	enum ScaleType
	{
		Linear,
		Logarithmic
	};

	enum DataType
	{
		Float,
		Integer,
		Bool
	} ;

	AutomatableModel( DataType type,
						const float val = 0,
						const float min = 0,
						const float max = 0,
						const float step = 0,
						Model* parent = NULL,
						const QString& displayName = QString(),
						bool defaultConstructed = false );

	virtual ~AutomatableModel();


	static float copiedValue()
	{
		return s_copiedValue;
	}

	bool isAutomated() const;
	bool isAutomatedOrControlled() const
	{
		return isAutomated() || m_controllerConnection != NULL;
	}
	
	bool hasSampleExactData() const;


	ControllerConnection* controllerConnection() const
	{
		return m_controllerConnection;
	}


	void setControllerConnection( ControllerConnection* c );


	template<class T>
	static T castValue( const float v )
	{
		return (T)( v );
	}

	template<bool>
	static bool castValue( const float v )
	{
		return ( qRound( v ) != 0 );
	}


	template<class T>
	inline T value( int frameOffset = 0 ) const
	{
		if( unlikely( m_hasLinkedModels || m_controllerConnection != NULL ) )
		{
			return castValue<T>( controllerValue( frameOffset ) );
		}

		return castValue<T>( m_value );
	}

	float controllerValue( int frameOffset ) const;

	// returns sample-exact data as a ValueBuffer
	// should only be called when sample-exact data exists
	// in other cases (eg. for automation), the receiving end should interpolate
	// the values themselves
	ValueBuffer * valueBuffer();

	template<class T>
	T initValue() const
	{
		return castValue<T>( m_initValue );
	}

	bool isAtInitValue() const
	{
		return m_value == m_initValue;
	}

	template<class T>
	T minValue() const
	{
		return castValue<T>( m_minValue );
	}

	template<class T>
	T maxValue() const
	{
		return castValue<T>( m_maxValue );
	}

	template<class T>
	T step() const
	{
		return castValue<T>( m_step );
	}


	void setInitValue( const float value );

	void setAutomatedValue( const float value );
	void setValue( const float value );

	void incValue( int steps )
	{
		setValue( m_value + steps * m_step );
	}

	float range() const
	{
		return m_range;
	}

	void setRange( const float min, const float max, const float step = 1 );
	void setScaleType( ScaleType sc ) {
		m_scaleType = sc;
	}
	void setScaleLogarithmic( bool setToTrue = true )
	{
		setScaleType( setToTrue ? Logarithmic : Linear );
	}

	void setStep( const float step );

	float centerValue() const
	{
		return m_centerValue;
	}

	void setCenterValue( const float centerVal )
	{
		m_centerValue = centerVal;
	}

	static void linkModels( AutomatableModel* m1, AutomatableModel* m2 );
	static void unlinkModels( AutomatableModel* m1, AutomatableModel* m2 );

	void unlinkAllModels();

	/**
	 * @brief Saves settings (value, automation links and controller connections) of AutomatableModel into
	 *  specified DOM element using <name> as attribute/node name
	 * @param doc TODO
	 * @param element Where this option shall be saved.
	 *  Depending on the model, this can be done in an attribute or in a subnode.
	 * @param name Name to store this model as.
	 */
	virtual void saveSettings( QDomDocument& doc, QDomElement& element, const QString& name );

	/*! \brief Loads settings (value, automation links and controller connections) of AutomatableModel from
				specified DOM element using <name> as attribute/node name */
	virtual void loadSettings( const QDomElement& element, const QString& name );

	virtual QString nodeName() const
	{
		return "automatablemodel";
	}

	QString displayValue( const float val ) const
	{
		switch( m_dataType )
		{
			case Float: return QString::number( castValue<float>( val ) );
			case Integer: return QString::number( castValue<int>( val ) );
			case Bool: return QString::number( castValue<bool>( val ) );
		}
		return "0";
	}

	bool hasLinkedModels() const
	{
		return m_hasLinkedModels;
	}

	float globalAutomationValueAt( const MidiTime& time );
	
	bool hasStrictStepSize() const
	{
		return m_hasStrictStepSize;
	}
	
	void setStrictStepSize( const bool b )
	{
		m_hasStrictStepSize = b;
	}

public slots:
	virtual void reset();
	virtual void copyValue();
	virtual void pasteValue();
	void unlinkControllerConnection();


protected:
	//! returns a value which is in range between min() and
	//! max() and aligned according to the step size (step size 0.05 -> value
	//! 0.12345 becomes 0.10 etc.). You should always call it at the end after
	//! doing your own calculations.
	float fittedValue( float value, bool forceStep = false ) const;


private:
	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		saveSettings( doc, element, "value" );
	}

	virtual void loadSettings( const QDomElement& element )
	{
		loadSettings( element, "value" );
	}

	void linkModel( AutomatableModel* model );
	void unlinkModel( AutomatableModel* model );

	//! @brief Scales @value from linear to logarithmic.
	//! Value should be within [0,1]
	template<class T> T logToLinearScale( T value ) const;

	//! rounds @a value to @a where if it is close to it
	//! @param value will be modified to rounded value
	template<class T> void roundAt( T &value, const T &where ) const;


	DataType m_dataType;
	ScaleType m_scaleType; //! scale type, linear by default
	float m_value;
	float m_initValue;
	float m_minValue;
	float m_maxValue;
	float m_step;
	float m_range;
	float m_centerValue;

	// currently unused?
	float m_oldValue;
	int m_setValueDepth;
	
	// used to determine if step size should be applied strictly (ie. always)
	// or only when value set from gui (default)
	bool m_hasStrictStepSize;

	AutoModelVector m_linkedModels;
	bool m_hasLinkedModels;


	//! NULL if not appended to controller, otherwise connection info
	ControllerConnection* m_controllerConnection;


	static float s_copiedValue;

	ValueBuffer m_valueBuffer;

signals:
	void initValueChanged( float val );
	void destroyed( jo_id_t id );

} ;





#define defaultTypedMethods(type)								\
	type value( int frameOffset = 0 ) const						\
	{															\
		return AutomatableModel::value<type>( frameOffset );	\
	}															\
																\
	type initValue() const										\
	{															\
		return AutomatableModel::initValue<type>();				\
	}															\
																\
	type minValue() const										\
	{															\
		return AutomatableModel::minValue<type>();				\
	}															\
																\
	type maxValue() const										\
	{															\
		return AutomatableModel::maxValue<type>();				\
	}															\


// some typed AutomatableModel-definitions

class FloatModel : public AutomatableModel
{
public:
	FloatModel( float val = 0, float min = 0, float max = 0, float step = 0,
				Model * parent = NULL,
				const QString& displayName = QString(),
				bool defaultConstructed = false ) :
		AutomatableModel( Float, val, min, max, step, parent, displayName, defaultConstructed )
	{
	}

	defaultTypedMethods(float);

} ;


class IntModel : public AutomatableModel
{
public:
	IntModel( int val = 0, int min = 0, int max = 0,
				Model* parent = NULL,
				const QString& displayName = QString(),
				bool defaultConstructed = false ) :
		AutomatableModel( Integer, val, min, max, 1, parent, displayName, defaultConstructed )
	{
	}

	defaultTypedMethods(int);

} ;


class BoolModel : public AutomatableModel
{
public:
	BoolModel( const bool val = false,
				Model* parent = NULL,
				const QString& displayName = QString(),
				bool defaultConstructed = false ) :
		AutomatableModel( Bool, val, false, true, 1, parent, displayName, defaultConstructed )
	{
	}

	defaultTypedMethods(bool);

} ;


#endif

