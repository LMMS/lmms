#pragma once

#include "ToolPluginView.h"
#include "AnalyzerPlugin.h"
#include "../common/InstrumentVisualizationWindow.h"

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
    void copyTextEditToClipboard();
    void showVisualization();
    void updateVisualizationData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples, std::vector<double> coordinates);

  private:
	  virtual void modelChanged( void );

    QPushButton * m_openAudioFileButton;
    QPushButton * m_openVisualizationButton;
    QLineEdit * m_nameField;
    
    Diginstrument::InstrumentVisualizationWindow * visualization;
    /*TODO*/
};
