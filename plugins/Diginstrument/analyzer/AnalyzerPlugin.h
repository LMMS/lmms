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
#include "PeakApproximation.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include "../common/InstrumentVisualizationWindow.h"
#include <QtDataVisualization>

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

private:
  friend class AnalyzerView;

  std::string setAudioFile(const QString &_audio_file);
  QtDataVisualization::QSurfaceDataArray * getSurfaceData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples);

  typedef SampleBuffer::handleState handleState;
  SampleBuffer m_sampleBuffer;
  std::vector<SplineSpectrum<double, 4>> spectra;

private slots:
  //void sampleRateChanged();
};