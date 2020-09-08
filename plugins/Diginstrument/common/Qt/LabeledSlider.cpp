#include "LabeledSlider.h"

LabeledSlider::LabeledSlider(int min, int max)
{
    minLabel = new QLabel(std::to_string(min).c_str());
    maxLabel = new QLabel(std::to_string(max).c_str());
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);

    QGridLayout * layout = new QGridLayout;
    layout->addWidget(slider, 0, 0, 1, 4);
    layout->addWidget(minLabel, 1, 0, 1, 1);
    layout->addWidget(maxLabel, 1, 3, 1, 1);
    layout->setContentsMargins(QMargins(0,0,0,0));
    layout->setSpacing(0);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->setLayout(layout);
}