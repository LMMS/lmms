#include "InstrumentVisualizationWindow.h"

using namespace QtDataVisualization;

void Diginstrument::InstrumentVisualizationWindow::setSurfaceData(QSurfaceDataArray * data)
{
    series->dataProxy()->resetArray(data);
}

Diginstrument::InstrumentVisualizationWindow::InstrumentVisualizationWindow(QObject * dataProvider)
{   
    //TODO: clear data when closed? lotsa memory
    connect(this, SIGNAL( requestDataUpdate(float, float, float, float, int, int, std::vector<double>) ), dataProvider, SLOT( updateVisualizationData(float, float, float, float, int, int, std::vector<double>) ));

    graph = new QtDataVisualization::Q3DSurface();
    container = QWidget::createWindowContainer(graph);
    series = new QSurface3DSeries;
    graph->addSeries(series);

    container->setMinimumWidth(400);
    container->setMinimumHeight(400);

    QHBoxLayout * outerLayout = new QHBoxLayout;
    QVBoxLayout * controlsLayout = new QVBoxLayout;
    QVBoxLayout * controlsContainerLayout = new QVBoxLayout;
    QWidget * controls = new QWidget;
    QWidget * controlsContainer = new QWidget;
    controls->setLayout(controlsLayout);
    controlsContainer->setLayout(controlsContainerLayout);
    outerLayout->addWidget(container);
    outerLayout->addWidget(controlsContainer);
    controlsContainerLayout->addWidget(controls);

    controlsContainer->setMaximumWidth(300);
    //todo:layout
    //todo:default values/limits
    //tmp
    controlsLayout->addWidget(new QLabel(QString("Coordinates")));
    coordinateSliderContainer = new QWidget;
    coordinateSliderContainer->setLayout(new QVBoxLayout);
    controlsLayout->addWidget(coordinateSliderContainer);

    controlsLayout->addWidget(new QLabel(QString("Limits")));
    startTimeSlider = new LabeledFieldSlider(0, 5000, 0);
    controlsLayout->addWidget(startTimeSlider);
    endTimeSlider = new LabeledFieldSlider(0,5000, 5000);
    controlsLayout->addWidget(endTimeSlider);
    startFreqSlider = new LabeledFieldSlider(20, 22000, 20);
    controlsLayout->addWidget(startFreqSlider);
    endFreqSlider = new LabeledFieldSlider(20, 22000, 22000);
    controlsLayout->addWidget(endFreqSlider);

    controlsLayout->addWidget(new QLabel(QString("Sample sizes")));
    timeSamples = new QLineEdit;
    timeSamples->setText("100");
    timeSamples->setValidator( new QIntValidator(2, 999) );
    controlsLayout->addWidget(timeSamples);
    frequencySamples = new QLineEdit;
    frequencySamples->setText("100");
    frequencySamples->setValidator( new QIntValidator(2, 999) );
    controlsLayout->addWidget(frequencySamples);

    //todo: auto-refresh
    QPushButton * refreshButton = new QPushButton("Refresh");
    controlsLayout->addWidget(refreshButton);
    connect( refreshButton, SIGNAL( clicked() ),
            this, SLOT( refresh() ));
    QWidget * autoRefreshContainer = new QWidget;
    QHBoxLayout * autoRefreshLayout = new QHBoxLayout;
    autoRefreshLayout->addWidget(new QLabel("Auto-refresh"));
    autoRefreshContainer->setLayout(autoRefreshLayout);
    autoRefreshCheckbox = new QCheckBox;
    autoRefreshCheckbox->setChecked(false);
    autoRefreshLayout->addWidget(autoRefreshCheckbox);
    controlsLayout->addWidget(autoRefreshContainer);

    this->setLayout(outerLayout);

    //tmp: graph style
    graph->axisX()->setRange(20, 22000);
    graph->axisX()->setFormatter(new QLogValue3DAxisFormatter);
    graph->axisY()->setRange(0, 1.1);
    graph->axisZ()->setRange(0, 3);
    graph->setAspectRatio(1.0);
    graph->setHorizontalAspectRatio(1.0);

    QLinearGradient gr;
    //TODO:maybe transparent? it seems bugged, may be just this old video card
    gr.setColorAt(0.0, Qt::transparent);
    gr.setColorAt(0.33, Qt::blue);
    gr.setColorAt(0.67, Qt::green);
    gr.setColorAt(0.99, Qt::green);
    gr.setColorAt(1.0, Qt::red);

    //TODO: gradient seems to work weirdly: setColorAt is [0,1], is it automatically stretched to the values in data?
    graph->seriesList().at(0)->setBaseGradient(gr);
    graph->seriesList().at(0)->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
    //test: can i put this after?
    series->setDrawMode(QSurface3DSeries::DrawSurface);
    //TODO: use slices
    //graph->setSelectionMode(QAbstract3DGraph::SelectionItemAndRow | QAbstract3DGraph::SelectionSlice);

    connect(startTimeSlider, SIGNAL(valueChanged(int)), this, SLOT(slidersChanged()));
    connect(endTimeSlider, SIGNAL(valueChanged(int)), this, SLOT(slidersChanged()));
    connect(startFreqSlider, SIGNAL(valueChanged(int)), this, SLOT(slidersChanged()));
    connect(endFreqSlider, SIGNAL(valueChanged(int)), this, SLOT(slidersChanged()));
}

Diginstrument::InstrumentVisualizationWindow::~InstrumentVisualizationWindow()
{
    delete container;
}

void Diginstrument::InstrumentVisualizationWindow::refresh()
{
    std::vector<double> coordinates;
    coordinates.reserve(coordinateSliders.size());
    for(auto * slider : coordinateSliders)
    {
        coordinates.push_back((double)slider->value());
    }
    emit requestDataUpdate(startTimeSlider->value(),endTimeSlider->value(),startFreqSlider->value(),endFreqSlider->value(),timeSamples->text().toInt(),frequencySamples->text().toInt(), coordinates);
    graph->axisX()->setRange(startFreqSlider->value(), endFreqSlider->value());
    graph->axisZ()->setRange(startTimeSlider->value()/1000.0f, endTimeSlider->value()/1000.0f);
}

void Diginstrument::InstrumentVisualizationWindow::setDimensions(std::vector<Dimension> dimensions)
{
    for(auto * s : coordinateSliders)
    {
        delete s;
    }

    for(auto & d : dimensions)
    {
        //TMP: bad solution to exclude time
        if(d.name == "time") continue;
        coordinateSliderContainer->layout()->addWidget(new QLabel(d.name.c_str()));
        LabeledFieldSlider * slider = new LabeledFieldSlider(d.min, d.max, 400 /*TMP*/);
        connect(slider, SIGNAL(valueChanged(int)), this, SLOT(slidersChanged()));
        coordinateSliders.push_back(slider);
        coordinateSliderContainer->layout()->addWidget(slider);
    }
}

void  Diginstrument::InstrumentVisualizationWindow::slidersChanged()
{
    if(autoRefreshCheckbox->isChecked()) refresh();
}

int Diginstrument::InstrumentVisualizationWindow::addCustomItem(QtDataVisualization::QCustom3DItem *item)
{
    graph->addCustomItem(item);
}

void Diginstrument::InstrumentVisualizationWindow::removeCustomItems()
{
    graph->removeCustomItems();
}