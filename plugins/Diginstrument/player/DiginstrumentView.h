#pragma once

#include "InstrumentView.h"
#include "DiginstrumentPlugin.h"
#include "../common/InstrumentVisualizationWindow.h"
#include "../common/Qt/LabeledFieldSlider.h"

#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QtDataVisualization>
#include <QtWidgets>

class DiginstrumentView : public InstrumentView
{
    Q_OBJECT
  public:
    DiginstrumentView( Instrument * _instrument, QWidget * _parent );
	  virtual ~DiginstrumentView();

    /*TODO*/

  protected slots:
    void showInstumentVisualization();
    void openInstrumentFile();
    void updateVisualizationData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples, std::vector<double> coordinates);
    void updateCoordinates();

  private:
	  virtual void modelChanged( void );
    void setDimensions();

    QPushButton * m_openInstrumentFileButton;
    QPushButton * m_openInstrumentVisualizationButton;
    QLineEdit * m_nameField;
    std::vector<LabeledFieldSlider*> coordinateSliders;
    QWidget * coordinateSliderContainer;
    
    Diginstrument::InstrumentVisualizationWindow * visualization;
};
