#include "AnalyzerView.h"

AnalyzerView::AnalyzerView(ToolPlugin * _parent ) :
  ToolPluginView(_parent)
{
    /*TODO */
    m_openAudioFileButton = new QPushButton( "Open file", this);
    m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    m_openAudioFileButton->move( 2, 2 );
    m_copyToClipboardButton = new QPushButton( "Copy to clipboard", this);
    m_copyToClipboardButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    m_copyToClipboardButton->move( 85, 2 );
    connect( m_openAudioFileButton, SIGNAL( clicked() ),
            this, SLOT( openAudioFile() ) );
    connect( m_copyToClipboardButton, SIGNAL( clicked() ),
            this, SLOT( copyTextEditToClipboard() ) );
    m_textarea = new QTextEdit("CWT output", this);
    m_textarea->setReadOnly(true);
    m_textarea->move(2,40);
}


AnalyzerView::~AnalyzerView()
{
}

void AnalyzerView::modelChanged( void ){
    /*TODO */
}

void AnalyzerView::openAudioFile( void )
{
	QString af = castModel<AnalyzerPlugin>()->m_sampleBuffer.
							openAudioFile();
	if( af != "" )
	{
    std::string res = castModel<AnalyzerPlugin>()->setAudioFile( af );
    m_textarea->setPlainText(res.c_str());
		//Engine::getSong()->setModified();
		//m_waveView->updateSampleRange();
	}
}

void AnalyzerView::copyTextEditToClipboard(void){
  m_textarea->copy();
}
