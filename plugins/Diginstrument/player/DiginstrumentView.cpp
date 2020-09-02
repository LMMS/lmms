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

    visualization = new Diginstrument::InstrumentVisualizationWindow(this);
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
  //TODO: couple and use parameters
  //TODO: use all coordinates (not just freq)
  //TODO: defaults/saving
  updateVisualizationData(0,3000,20,22000,100,100);
  visualization->show();
  //is this even useful?
  visualization->adjustSize();
}

void DiginstrumentView::updateVisualizationData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples)
{
  visualization->setSurfaceData(castModel<DiginstrumentPlugin>()->getInstrumentSurfaceData(minTime/1000.0f,maxTime/1000.0f,minFreq,maxFreq,timeSamples,freqSamples));
}
