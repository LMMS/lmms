#pragma once

#include "ToolPluginView.h"
#include "AnalyzerPlugin.h"
#include "../common/InstrumentVisualizationWindow.h"
#include "../common/Qt/DimensionField.hpp"

#include <QPushButton>
#include <QTextEdit>

class AnalyzerView : public ToolPluginView
{
    Q_OBJECT
  public:
    AnalyzerView( ToolPlugin * _parent );
	  virtual ~AnalyzerView();

    /*TODO*/

  protected slots:
    void openAudioFile();
    void writeInstrumentToFile();
    void copyTextEditToClipboard();
    void showVisualization();
    void addDimension();
    void deleteDimensionField(DimensionField * field);
    void updateVisualizationData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples, std::vector<double> coordinates);

  private:
	  virtual void modelChanged( void );

    QPushButton * m_openAudioFileButton;
    QPushButton * m_openVisualizationButton;
    QPushButton * m_addDimensionButton;
    QLineEdit * m_nameField;
    QList<DimensionField*> dimensionFields;
    QWidget * dimensionFieldsContainer;
    QLineEdit * m_fileNameField;
    QPushButton * m_saveToFileButton;
    
    Diginstrument::InstrumentVisualizationWindow * visualization;
    /*TODO*/
};
