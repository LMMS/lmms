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
    m_addDimensionButton = new QPushButton( "Add dimension");
    m_addDimensionButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    m_saveToFileButton = new QPushButton( "Save instrument");
    m_saveToFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );

    connect( m_openAudioFileButton, SIGNAL( clicked() ),
            this, SLOT( openAudioFile() ) );
    connect( m_openVisualizationButton, SIGNAL( clicked() ),
            this, SLOT( showVisualization() ) );
    connect( m_addDimensionButton, SIGNAL( clicked() ),
            this, SLOT( addCoordinate() ) );
    connect( m_saveToFileButton, SIGNAL( clicked() ),
            this, SLOT( writeInstrumentToFile() ) );

    QWidget * infoContainer = new QWidget;
    infoContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QGridLayout * infoLayout = new QGridLayout;
    infoContainer->setLayout(infoLayout);
    infoLayout->addWidget(m_nameField, 0, 0, 1, 1);
    infoLayout->addWidget(m_openAudioFileButton, 0, 1, 1, 2);
    infoLayout->addWidget(m_openVisualizationButton, 1, 1, 1, 2);

    dimensionFieldsContainer = new QWidget;
    QVBoxLayout * coordinateLayout = new QVBoxLayout;
    dimensionFieldsContainer->setLayout(coordinateLayout);
    coordinateLayout->setMargin(0);
    coordinateLayout->setSpacing(0);
    coordinateLayout->setAlignment(Qt::AlignTop);
    coordinateLayout->addWidget(m_addDimensionButton);

    QVBoxLayout * layout = new QVBoxLayout;
    layout->addWidget(infoContainer);
    layout->addWidget(dimensionFieldsContainer);
    layout->addWidget(m_saveToFileButton);
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
    std::vector<std::pair<std::string, double>> coordinates;
    for(auto * p : dimensionFields)
    {
      const auto pair = p->getCoordinate();
      if(!pair.first.empty()) coordinates.push_back(pair);
    }
    std::string res = castModel<AnalyzerPlugin>()->analyzeSample( af , coordinates);
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

void AnalyzerView::addDimension()
{
  dimensionFields.push_back(new DimensionField);
  dimensionFieldsContainer->layout()->addWidget(dimensionFields.back());
  connect(dimensionFields.back(), SIGNAL( deleteSelf(DimensionField *) ), this, SLOT( deleteDimensionField(DimensionField*) ));
}

void AnalyzerView::deleteDimensionField(DimensionField * field)
{
  dimensionFields.removeOne(field);
  delete field;
}

void AnalyzerView::writeInstrumentToFile()
{
  //tmp: set dimensions here, before saving
  std::vector<Diginstrument::Dimension> dimensions;
  for(const auto * f : dimensionFields)
  {
    dimensions.push_back(f->getDimension());
  }
  castModel<AnalyzerPlugin>()->inst.dimensions = dimensions;
  QString fileName = QFileDialog::getSaveFileName(NULL, "Save instrument to file",
                           NULL,
                           "*.json");
  if(fileName!="") castModel<AnalyzerPlugin>()->writeInstrumentToFile(fileName.toStdString());
}