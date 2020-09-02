#pragma once

#include <QtWidgets>
#include <QtDataVisualization>

namespace Diginstrument
{
class InstrumentVisualizationWindow : public QWidget
{
  Q_OBJECT
  public:
    void setSurfaceData(QtDataVisualization::QSurfaceDataArray * data);

    InstrumentVisualizationWindow(QObject * dataProvider);
    ~InstrumentVisualizationWindow();

  signals:
    void requestDataUpdate(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples);
  private slots:
    void refreshButtonPressed();

  private:
    QtDataVisualization::Q3DSurface *graph;
    QWidget * container;
    QtDataVisualization::QSurface3DSeries *series;
    //UI elements
    //TODO: TMP: only one coordinate
    QSlider * freqSlider, *startTimeSlider, *endTimeSlider, *startFreqSlider, *endFreqSlider;
    QLineEdit * timeSamples, *frequencySamples;
};
};