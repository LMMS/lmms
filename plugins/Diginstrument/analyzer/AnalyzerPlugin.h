#pragma once

#include "AnalyzerView.h"

#include "ToolPlugin.h"

#include "plugin_export.h"
#include "embed.h"

#include "SampleBuffer.h"
#include <QFileInfo>
#include "Song.h"

/*tmp*/
#include "CWT.hpp"
#include "Extrema.hpp"
#include "../common/Approximation.hpp"
#include "../common/Interpolation.hpp"
#include "SplineFitter.hpp"
#include "../common/PiecewiseBSpline.hpp"
#include "SpectrumFitter.hpp"
#include "../common/SplineSpectrum.hpp"
#include "../common/Instrument.hpp"
#include "PeakApproximation.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include "../common/InstrumentVisualizationWindow.h"
#include <QtDataVisualization>
#include "Phase.hpp"
#include "FFT.hpp"
#include "../common/Qt/ColorPalette.hpp"

class AnalyzerPlugin : public ToolPlugin
{
  Q_OBJECT
public:
  //tmp: raw visualization
  Diginstrument::InstrumentVisualizationWindow * visualization;
  

  AnalyzerPlugin();
  virtual ~AnalyzerPlugin();

  virtual PluginView *instantiateView(QWidget *_parent);

  virtual void saveSettings(QDomDocument &_doc, QDomElement &_parent);
  virtual void loadSettings(const QDomElement &_this);

  virtual QString nodeName() const;

  virtual QString fullDisplayName() const;

  void writeInstrumentToFile(std::string filename);

private:
  friend class AnalyzerView;

  std::string analyzeSample(const QString &_audio_file, vector<pair<string, double>> coordinates);
  QtDataVisualization::QSurfaceDataArray * getSurfaceData(double minTime, double maxTime, double minFreq, double maxFreq, int timeSamples, int freqSamples);

  typedef SampleBuffer::handleState handleState;
  SampleBuffer m_sampleBuffer;
  Diginstrument::Instrument<SplineSpectrum<double,4>, double> inst;
  //TMP: keep for visualization
  std::vector<SplineSpectrum<double,4>> spectra;

  void analyze(const std::vector<double> & signal, std::vector<std::vector<Diginstrument::Component<double>>> partials, vector<pair<string, double>> coordinates);
  std::vector<std::vector<Diginstrument::Component<double>>> subtractiveAnalysis(std::vector<double> & signal, unsigned int sampleRate, vector<pair<string, double>> coordinates);

private slots:
  //void sampleRateChanged();
};