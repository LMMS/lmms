/* Text customizable knob */
#ifndef CUSTOM_TEXT_KNOB_H
#define CUSTOM_TEXT_KNOB_H

#include "Knob.h"

namespace lmms::gui
{


class LMMS_EXPORT CustomTextKnob : public Knob
{
protected:
	inline void setHintText( const QString & _txt_before, const QString & _txt_after ) {} // inaccessible
public:
	CustomTextKnob( knobTypes _knob_num, QWidget * _parent = nullptr, const QString & _name = QString(), const QString & _value_text = QString() );

	CustomTextKnob( QWidget * _parent = nullptr, const QString & _name = QString(), const QString & _value_text = QString() ); //!< default ctor

	CustomTextKnob( const Knob& other ) = delete;

	inline void setValueText(const QString & _value_text)
	{
		m_value_text = _value_text;
	}

private:
	QString displayValue() const override;

protected:
	QString m_value_text;
} ;


} // namespace lmms::gui

#endif
