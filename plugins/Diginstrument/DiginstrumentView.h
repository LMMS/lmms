#pragma once

#include "InstrumentView.h"
#include "DiginstrumentPlugin.h"

#include <QPushButton>
#include <QTextEdit>

class DiginstrumentView : public InstrumentViewFixedSize /*TMP: fixed size */
{
    Q_OBJECT
  public:
    DiginstrumentView( Instrument * _instrument, QWidget * _parent );
	  virtual ~DiginstrumentView();

    /*TODO*/

  protected slots:
    void openAudioFile();
    void copyTextEditToClipboard();

  private:
	  virtual void modelChanged( void );

    QPushButton * m_openAudioFileButton;
    QPushButton * m_copyToClipboardButton;
    QTextEdit * m_textarea;
    /*TODO*/
};
