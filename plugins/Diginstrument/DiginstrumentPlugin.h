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
#include "CWT.hpp"
#include "Extrema.hpp"
#include "Approximation.hpp"
#include "Interpolation.hpp"
#include "SplineFitter.hpp"
#include "PiecewiseBSpline.hpp"
#include "SpectrumFitter.hpp"
#include "SplineSpectrum.hpp"
#include "PeakApproximation.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

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

private:
  friend class DiginstrumentView;
  /*TMP*/
  Diginstrument::Synthesizer synth;
  Diginstrument::Interpolator<double, SplineSpectrum<double, 4>> inst;

  typedef SampleBuffer::handleState handleState;
  SampleBuffer m_sampleBuffer;
  std::string setAudioFile(const QString &_audio_file);

private slots:
  void sampleRateChanged();
};