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

  private:
	  virtual void modelChanged( void );

    QPushButton * m_openAudioFileButton;
    QPushButton * m_copyToClipboardButton;
    QTextEdit * m_textarea;

    Diginstrument::InstrumentVisualizationWindow * visualization;
    /*TODO*/
};
