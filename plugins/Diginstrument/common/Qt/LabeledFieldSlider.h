#pragma once

#include <QtWidgets>

#include "LabeledSlider.h"

class LabeledFieldSlider : public QWidget
{
    Q_OBJECT
    public:
        inline int value() const
        {
            return slider->value();
        }

        void setValue(int value)
        {
            slider->setValue(value);
            field->setText(std::to_string(value).c_str());
        }

        LabeledFieldSlider(int min, int max, int value);

    signals:
        void valueChanged(int value);

    protected slots:
        void syncToField();
        void syncToSlider();

    private:
        LabeledSlider * slider;
        QLineEdit * field;
};