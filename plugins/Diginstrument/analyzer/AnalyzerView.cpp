#include "AnalyzerView.h"

AnalyzerView::AnalyzerView(ToolPlugin * _parent ) :
  ToolPluginView(_parent)
{
    /*TODO */
    m_nameField = new QLineEdit;
    m_openAudioFileButton = new QPushButton( "Open file", this);
    m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    m_openVisualizationButton = new QPushButton( "Show instrument visualization");
    m_openVisualizationButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    connect( m_openAudioFileButton, SIGNAL( clicked() ),
            this, SLOT( openAudioFile() ) );
    connect( m_openVisualizationButton, SIGNAL( clicked() ),
            this, SLOT( showVisualization() ) );

    QWidget * infoContainer = new QWidget;
    infoContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QGridLayout * infoLayout = new QGridLayout;
    infoContainer->setLayout(infoLayout);
    infoLayout->addWidget(m_nameField, 0, 0, 1, 1);
    infoLayout->addWidget(m_openAudioFileButton, 0, 1, 1, 2);
    infoLayout->addWidget(m_openVisualizationButton, 1, 1, 1, 2);

    QVBoxLayout * layout = new QVBoxLayout;
    layout->addWidget(infoContainer);
    this->setLayout(layout);
    this->adjustSize();

    visualization = new Diginstrument::InstrumentVisualizationWindow(this);
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
		//Engine::getSong()->setModified();
		//m_waveView->updateSampleRange();
	}
}

void AnalyzerView::updateVisualizationData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples, std::vector<double> coordinates)
{
  visualization->setSurfaceData(castModel<AnalyzerPlugin>()->getSurfaceData(minTime/1000.0f,maxTime/1000.0f,minFreq,maxFreq,timeSamples,freqSamples));
}

void AnalyzerView::showVisualization()
{
  updateVisualizationData(0,3000,20,22000,100,100/*TODO default values*/, {});
  visualization->show();
}