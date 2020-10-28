#pragma once

#include "DiginstrumentView.h"
#include "Synthesizer.h"
#include "Interpolator.h"

#include "Instrument.h"
#include "InstrumentTrack.h"
#include "Engine.h"
#include "Mixer.h"

#include "plugin_export.h"
#include "embed.h"

#include "SampleBuffer.h"
#include <QFileInfo>
#include "Song.h"

/*tmp*/
#include "../common/Approximation.hpp"
#include "../common/Interpolation.hpp"
#include "../common/PiecewiseBSpline.hpp"
#include "../common/Spectrum.hpp"
#include "../common/Dimension.h"
#include "../common/Instrument.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <QtDataVisualization>

using json = nlohmann::json;

//TMP
class InstrumentData
{
  //todo
  public:
    std::string name;
    std::string type;
    std::vector<Diginstrument::Dimension> dimensions;
    json _json;
};

class DiginstrumentPlugin : public Instrument
{
  Q_OBJECT
public:
  DiginstrumentPlugin(InstrumentTrack *_track);
  virtual ~DiginstrumentPlugin();

  virtual PluginView *instantiateView(QWidget *_parent);

  virtual void saveSettings(QDomDocument &_doc, QDomElement &_parent);
  virtual void loadSettings(const QDomElement &_this);

  virtual QString nodeName() const;

  virtual void playNote(NotePlayHandle * /* _note_to_play */,
                        sampleFrame * /* _working_buf */);

  virtual void deleteNotePluginData(NotePlayHandle *_note_to_play);
  virtual f_cnt_t beatLen(NotePlayHandle *_n) const;

  virtual QString fullDisplayName() const;

  virtual f_cnt_t desiredReleaseFrames() const
  {
    return 0;
    /*TODO, not sure if needed or applicable*/
  }

  //MIDI - not sure if it will be supported
  inline virtual bool handleMidiEvent(const MidiEvent &, const MidiTime & = MidiTime(), f_cnt_t offset = 0)
  {
    return true;
  }

  QtDataVisualization::QSurfaceDataArray * getInstrumentSurfaceData(float minTime, float maxTime, float minFreq, float maxFreq, int timeSamples, int freqSamples, std::vector<double> coordinates);

protected:
  std::vector<double> coordinates;

private:
  friend class DiginstrumentView;
  Diginstrument::Synthesizer synth;
  Diginstrument::Interpolator<double, SplineSpectrum<double, 4>> interpolator;
  //TODO: maybe "reroute" spectra so that they wont be duplicated in interpolator and instrument
  Diginstrument::Instrument<SplineSpectrum<double, 4>, double> instrument;

  bool setInstrumentFile(const QString & fileName);
  bool loadInstrumentFile();

  std::string fileName;

private slots:
  void sampleRateChanged();
};