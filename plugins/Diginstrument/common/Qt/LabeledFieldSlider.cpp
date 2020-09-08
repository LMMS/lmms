#include "LabeledFieldSlider.h"

LabeledFieldSlider::LabeledFieldSlider(int min, int max, int value)
{
    slider = new LabeledSlider(min, max);
    slider->setValue(value);
    field = new QLineEdit;
    field->setValidator( new QIntValidator(min, max) );
    field->setText(std::to_string(value).c_str());

    QHBoxLayout * layout = new QHBoxLayout;
    layout->addWidget(field);
    layout->addWidget(slider);
    field->setMaximumWidth(50);
    this->setLayout(layout);

    connect(field, SIGNAL(textChanged(const QString &)), this, SLOT(syncToField()));
    connect(slider->slider, SIGNAL(valueChanged(int)), this, SLOT(syncToSlider()));
}

void LabeledFieldSlider::syncToSlider()
{
    field->setText(std::to_string(slider->value()).c_str());
    emit valueChanged(slider->value());
}

void LabeledFieldSlider::syncToField()
{
    slider->setValue(field->text().toInt());
    emit valueChanged(field->text().toInt());
}