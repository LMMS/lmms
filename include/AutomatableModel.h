/*
 * AutomatableModel.h - declaration of class AutomatableModel
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_AUTOMATABLE_MODEL_H
#define LMMS_AUTOMATABLE_MODEL_H

#include <cmath>
#include <QMap>
#include <QMutex>

#include "JournallingObject.h"
#include "Model.h"
#include "TimePos.h"
#include "ValueBuffer.h"
#include "ModelVisitor.h"


namespace lmms
{

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

// use this to make subclasses visitable
#define MODEL_IS_VISITABLE \
	void accept(ModelVisitor& v) override { v.visit(*this); } \
	void accept(ConstModelVisitor& v) const override { v.visit(*this); }



class ControllerConnection;

class LMMS_EXPORT AutomatableModel : public Model, public JournallingObject
{
	Q_OBJECT
public:
	using AutoModelVector = std::vector<AutomatableModel*>;

	enum class ScaleType
	{
		Linear,
		Logarithmic,
		Decibel
	};


	~AutomatableModel() override;

	// Implement those by using the MODEL_IS_VISITABLE macro
	virtual void accept(ModelVisitor& v) = 0;
	virtual void accept(ConstModelVisitor& v) const = 0;

public:
	/**
	   @brief Return this class casted to Target
	   @test AutomatableModelTest.cpp
	   @param doThrow throw an assertion if the cast fails, instead of
	     returning a nullptr
	   @return the casted class if Target is the exact or a base class of
	     *this, nullptr otherwise
	*/
	template<class Target>
	Target* dynamicCast(bool doThrow = false)
	{
		DCastVisitor<Target> vis; accept(vis);
		if (doThrow && !vis.result) { Q_ASSERT(false); }
		return vis.result;
	}

	//! const overload, see overloaded function
	template<class Target>
	const Target* dynamicCast(bool doThrow = false) const
	{
		ConstDCastVisitor<Target> vis; accept(vis);
		if (doThrow && !vis.result) { Q_ASSERT(false); }
		return vis.result;
	}

	bool isAutomated() const;
	bool isAutomatedOrControlled() const
	{
		return isAutomated() || m_controllerConnection != nullptr;
	}

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
		return (std::round(v) != 0);
	}


	template<class T>
	inline T value( int frameOffset = 0 ) const
	{
		if (m_controllerConnection)
		{
			if (!m_useControllerValue)
			{
				return castValue<T>(m_value);
			}
			else
			{
				return castValue<T>(controllerValue(frameOffset));
			}
		}
		else if (hasLinkedModels())
		{
			return castValue<T>( controllerValue( frameOffset ) );
		}

		return castValue<T>( m_value );
	}

	float controllerValue( int frameOffset ) const;

	//! @brief Function that returns sample-exact data as a ValueBuffer
	//! @return pointer to model's valueBuffer when s.ex.data exists, NULL otherwise
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

	//! @brief Returns value scaled with the scale type and min/max values of this model
	float scaledValue( float value ) const;
	//! @brief Returns value applied with the inverse of this model's scale type
	float inverseScaledValue( float value ) const;

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
		setScaleType( setToTrue ? ScaleType::Logarithmic : ScaleType::Linear );
	}
	bool isScaleLogarithmic() const
	{
		return m_scaleType == ScaleType::Logarithmic;
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

	//! link @p m1 and @p m2, let @p m1 take the values of @p m2
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

	QString nodeName() const override
	{
		return "automatablemodel";
	}

	virtual QString displayValue( const float val ) const = 0;

	bool hasLinkedModels() const
	{
		return !m_linkedModels.empty();
	}

	// a way to track changed values in the model and avoid using signals/slots - useful for speed-critical code.
	// note that this method should only be called once per period since it resets the state of the variable - so if your model
	// has to be accessed by more than one object, then this function shouldn't be used.
	bool isValueChanged()
	{
		if( m_valueChanged || valueBuffer() )
		{
			m_valueChanged = false;
			return true;
		}
		return false;
	}

	float globalAutomationValueAt( const TimePos& time );

	void setStrictStepSize( const bool b )
	{
		m_hasStrictStepSize = b;
	}

	static void incrementPeriodCounter()
	{
		++s_periodCounter;
	}

	static void resetPeriodCounter()
	{
		s_periodCounter = 0;
	}

	bool useControllerValue()
	{
		return m_useControllerValue;
	}

public slots:
	virtual void reset();
	void unlinkControllerConnection();
	void setUseControllerValue(bool b = true);


protected:
	AutomatableModel(
						const float val = 0,
						const float min = 0,
						const float max = 0,
						const float step = 0,
						Model* parent = nullptr,
						const QString& displayName = QString(),
						bool defaultConstructed = false );
	//! returns a value which is in range between min() and
	//! max() and aligned according to the step size (step size 0.05 -> value
	//! 0.12345 becomes 0.10 etc.). You should always call it at the end after
	//! doing your own calculations.
	float fittedValue( float value ) const;


private:
	// dynamicCast implementation
	template<class Target>
	struct DCastVisitor : public ModelVisitor
	{
		Target* result = nullptr;
		void visit(Target& tar) { result = &tar; }
	};

	// dynamicCast implementation
	template<class Target>
	struct ConstDCastVisitor : public ConstModelVisitor
	{
		const Target* result = nullptr;
		void visit(const Target& tar) { result = &tar; }
	};

	static bool mustQuoteName(const QString &name);

	void saveSettings( QDomDocument& doc, QDomElement& element ) override
	{
		saveSettings( doc, element, "value" );
	}

	void loadSettings( const QDomElement& element ) override
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


	ScaleType m_scaleType; //!< scale type, linear by default
	float m_value;
	float m_initValue;
	float m_minValue;
	float m_maxValue;
	float m_step;
	float m_range;
	float m_centerValue;

	bool m_valueChanged;

	// currently unused?
	float m_oldValue;
	int m_setValueDepth;

	// used to determine if step size should be applied strictly (ie. always)
	// or only when value set from gui (default)
	bool m_hasStrictStepSize;

	AutoModelVector m_linkedModels;


	//! NULL if not appended to controller, otherwise connection info
	ControllerConnection* m_controllerConnection;


	ValueBuffer m_valueBuffer;
	long m_lastUpdatedPeriod;
	static long s_periodCounter;

	bool m_hasSampleExactData;

	// prevent several threads from attempting to write the same vb at the same time
	QMutex m_valueBufferMutex;

	bool m_useControllerValue;

signals:
	void initValueChanged( float val );
	void destroyed( lmms::jo_id_t id );

} ;




template <typename T> class LMMS_EXPORT TypedAutomatableModel : public AutomatableModel
{
public:
	using AutomatableModel::AutomatableModel;
	T value( int frameOffset = 0 ) const
	{
		return AutomatableModel::value<T>( frameOffset );
	}

	T initValue() const
	{
		return AutomatableModel::initValue<T>();
	}

	T minValue() const
	{
		return AutomatableModel::minValue<T>();
	}

	T maxValue() const
	{
		return AutomatableModel::maxValue<T>();
	}
};


// some typed AutomatableModel-definitions

class LMMS_EXPORT FloatModel : public TypedAutomatableModel<float>
{
	Q_OBJECT
	MODEL_IS_VISITABLE
public:
	FloatModel( float val = 0, float min = 0, float max = 0, float step = 0,
				Model * parent = nullptr,
				const QString& displayName = QString(),
				bool defaultConstructed = false ) :
		TypedAutomatableModel( val, min, max, step, parent, displayName, defaultConstructed )
	{
	}
	float getRoundedValue() const;
	int getDigitCount() const;
	QString displayValue( const float val ) const override;
} ;


class LMMS_EXPORT IntModel : public TypedAutomatableModel<int>
{
	Q_OBJECT
	MODEL_IS_VISITABLE
public:
	IntModel( int val = 0, int min = 0, int max = 0,
				Model* parent = nullptr,
				const QString& displayName = QString(),
				bool defaultConstructed = false ) :
		TypedAutomatableModel( val, min, max, 1, parent, displayName, defaultConstructed )
	{
	}
	QString displayValue( const float val ) const override;
} ;


class LMMS_EXPORT BoolModel : public TypedAutomatableModel<bool>
{
	Q_OBJECT
	MODEL_IS_VISITABLE
public:
	BoolModel( const bool val = false,
				Model* parent = nullptr,
				const QString& displayName = QString(),
				bool defaultConstructed = false ) :
		TypedAutomatableModel( val, false, true, 1, parent, displayName, defaultConstructed )
	{
	}
	QString displayValue( const float val ) const override;
} ;

using AutomatedValueMap = QMap<AutomatableModel*, float>;

} // namespace lmms

#endif // LMMS_AUTOMATABLE_MODEL_H
