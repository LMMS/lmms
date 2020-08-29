#pragma once

#include "InstrumentView.h"
#include "DiginstrumentPlugin.h"

#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QtDataVisualization>
#include <QtWidgets>

//TODO: note visualization with surface
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

  private:
	  virtual void modelChanged( void );

    QPushButton * m_openInstrumentFileButton;
    QPushButton * m_openInstrumentVisualizationButton;
    QLineEdit * m_nameField;
    QLineEdit * m_typeField;
    /*TODO*/
    //WIP: surface graph
    QtDataVisualization::Q3DSurface *graph;
};
