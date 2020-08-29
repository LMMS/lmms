#include "DiginstrumentView.h"

using namespace QtDataVisualization;

DiginstrumentView::DiginstrumentView( Instrument * _instrument, QWidget * _parent ) :
  InstrumentView(_instrument, _parent)
{
    m_openInstrumentFileButton = new QPushButton( "Load instrument from file", this);
    m_openInstrumentVisualizationButton = new QPushButton( "Show instrument visualization");
    m_openInstrumentFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    m_openInstrumentVisualizationButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    connect( m_openInstrumentFileButton, SIGNAL( clicked() ),
            this, SLOT( openInstrumentFile() ) );
    connect( m_openInstrumentVisualizationButton, SIGNAL( clicked() ),
            this, SLOT( showInstumentVisualization() ) );
    m_nameField = new QLineEdit("Name", this);
    m_nameField->setReadOnly(true);
    m_typeField = new QLineEdit("Spectrum type", this);
    m_typeField->setReadOnly(true);
    

    QVBoxLayout *layout = new QVBoxLayout;

    layout->addWidget(m_openInstrumentFileButton);
    layout->addWidget(m_nameField);
    layout->addWidget(m_typeField);
    layout->addWidget(m_openInstrumentVisualizationButton);

    this->setLayout(layout);

    graph = new QtDataVisualization::Q3DSurface();
}


DiginstrumentView::~DiginstrumentView()
{
}

void DiginstrumentView::modelChanged( void ){
    /*TODO */
    //tmp
    if(castModel<DiginstrumentPlugin>()->fileName != "")
    {
      m_nameField->setText(castModel<DiginstrumentPlugin>()->inst_data.name.c_str());
      m_typeField->setText(castModel<DiginstrumentPlugin>()->inst_data.type.c_str());
    }
}

void DiginstrumentView::openInstrumentFile( void )
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select instrument"), "", tr("*.json"));

	if( fileName != "" )
	{
    castModel<DiginstrumentPlugin>()->setInstrumentFile( fileName );
    castModel<DiginstrumentPlugin>()->loadInstrumentFile();
		Engine::getSong()->setModified();
	}
}

void DiginstrumentView::showInstumentVisualization()
{
  QWidget *container = QWidget::createWindowContainer(graph);

  QSurface3DSeries *series = new QSurface3DSeries;
  //Debug
  auto data = castModel<DiginstrumentPlugin>()->getInstrumentSurfaceData(0,3,20,22000,100,200);

  series->dataProxy()->resetArray(data);
  series->setDrawMode(QSurface3DSeries::DrawSurface);
  graph->addSeries(series);
  //tmp
  graph->axisX()->setRange(20, 22000);
  graph->axisX()->setFormatter(new QLogValue3DAxisFormatter);
  graph->axisY()->setRange(0, 1.2);
  graph->axisZ()->setRange(0, 3);
  graph->setAspectRatio(1.0);
  graph->setHorizontalAspectRatio(1.0);

  QLinearGradient gr;
  gr.setColorAt(-0.01, Qt::red);
  //TODO:maybe transparent?
  gr.setColorAt(0.0, Qt::black);
  gr.setColorAt(0.33, Qt::blue);
  gr.setColorAt(0.67, Qt::green);
  gr.setColorAt(1.0, Qt::green);
  gr.setColorAt(1.01, Qt::red);

  graph->seriesList().at(0)->setBaseGradient(gr);
  graph->seriesList().at(0)->setColorStyle(Q3DTheme::ColorStyleRangeGradient);

  container->show();
}
