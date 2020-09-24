#pragma once

#include <QtWidgets>
#include <QtDataVisualization>

#include "Qt/LabeledFieldSlider.h"
#include "Dimension.h"

namespace Diginstrument
{
class InstrumentVisualizationWindow : public QWidget
{
  Q_OBJECT
  public:
    void setSurfaceData(QtDataVisualization::QSurfaceDataArray * data);
    void setDimensions(std::vector<Dimension> dimensions);
    int addCustomItem(QtDataVisualization::QCustom3DItem *item);
    void removeCustomItems();

    InstrumentVisualizationWindow(QObject * dataProvider);
    ~InstrumentVisualizationWindow();

  signals:
    void requestDataUpdate(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples, std::vector<double> coordinates = {});
  private slots:
    void refresh();
    void slidersChanged();

  private:
    QtDataVisualization::Q3DSurface *graph;
    QWidget * container;
    QtDataVisualization::QSurface3DSeries *series;
    //UI elements
    //TODO: log-scale toggle
    LabeledFieldSlider *startTimeSlider, *endTimeSlider, *startFreqSlider, *endFreqSlider;
    QLineEdit * timeSamples, *frequencySamples;
    std::vector<LabeledFieldSlider*> coordinateSliders;
    QWidget * coordinateSliderContainer;
    QCheckBox * autoRefreshCheckbox;
};
};