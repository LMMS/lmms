#include "DiginstrumentView.h"

DiginstrumentView::DiginstrumentView( Instrument * _instrument, QWidget * _parent ) :
  InstrumentView(_instrument, _parent)
{
    /*TODO */
    m_openInstrumentFileButton = new QPushButton( "Load instrument from file", this);
    m_openInstrumentFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    m_openInstrumentFileButton->move( 2, 2 );
    connect( m_openInstrumentFileButton, SIGNAL( clicked() ),
            this, SLOT( openInstrumentFile() ) );
    m_nameField = new QLineEdit("Name", this);
    m_nameField->setReadOnly(true);
    m_nameField->move(2,30);
    m_typeField = new QLineEdit("Spectrum type", this);
    m_typeField->setReadOnly(true);
    m_typeField->move(2,45);
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
		//Engine::getSong()->setModified();
		//m_waveView->updateSampleRange();
	}
}