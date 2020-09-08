#pragma once

#include <QtWidgets>

class LabeledFieldSlider;

class LabeledSlider : public QWidget
{
    friend LabeledFieldSlider;
    public:
        inline int value() const
        {
            return slider->value();
        }

        inline void setValue(int value)
        {
            slider->setValue(value);
        }

        LabeledSlider(int min, int max);
    protected:
        QSlider * slider;

    private:
        QLabel *minLabel;
        QLabel *maxLabel;
};