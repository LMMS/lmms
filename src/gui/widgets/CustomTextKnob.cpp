#include "CustomTextKnob.h"

CustomTextKnob::CustomTextKnob(knobTypes _knob_num, QWidget* _parent, const QString& _name, const QString& _value_text)
	: Knob(_knob_num, _parent, _name)
	, m_value_text(_value_text)
{
}

CustomTextKnob::CustomTextKnob(QWidget* _parent, const QString& _name, const QString& _value_text)
	: //!< default ctor
	Knob(_parent, _name)
	, m_value_text(_value_text)
{
}

QString CustomTextKnob::displayValue() const { return m_description.trimmed() + m_value_text; }
