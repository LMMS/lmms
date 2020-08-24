#pragma once

#include "InstrumentView.h"
#include "DiginstrumentPlugin.h"

#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QtDataVisualization/Q3DSurface>

//TODO: note visualization with surface
class DiginstrumentView : public InstrumentView
{
    Q_OBJECT
  public:
    DiginstrumentView( Instrument * _instrument, QWidget * _parent );
	  virtual ~DiginstrumentView();

    /*TODO*/

  protected slots:
    void openInstrumentFile();

  private:
	  virtual void modelChanged( void );

    QPushButton * m_openInstrumentFileButton;
    QLineEdit * m_nameField;
    QLineEdit * m_typeField;
    /*TODO*/
    //WIP: surface graph
    QtDataVisualization::Q3DSurface *graph;
};
