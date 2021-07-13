/*
 * DrumSynth.h - DrumSynth DS file renderer
 *
 * Copyright (c) 1998-2000 Paul Kellett (mda-vst.com)
 * Copyright (c) 2007 Paul Giblock <drfaygo/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef _DRUMSYNTH_H__
#define _DRUMSYNTH_H__

#include "lmms_basics.h"
#include <QFile>
#include <QSettings>
#include <sstream>
#include <stdint.h>

class QString;
using namespace std;

class DrumSynth {
public:
  DrumSynth(){};
  int GetSamples(int16_t *&wave, int channels, sample_rate_t Fs);
  bool LoadFile(QString file);

private:
  const float TwoPi = 6.2831853f;

  float envpts[8][2][32] = {0}; // envelope/time-level/point

  struct envstatus {
    float last;  // Time of last envelope point
    float value; // Envelope value
    float delta; // Delta to add to envelope value at each sample
    float next;  // Timestamp of next point to go to
    int pointer; // Index of current point
  };
  // Indexes for envelopes and section on/off switches
  const int ENV_TONE = 0;
  const int ENV_NOISE = 1;
  const int ENV_OVERTONE1 = 2;
  const int ENV_OVERTONE2 = 3;
  const int ENV_NOISEBAND = 4;
  const int ENV_NOISEBAND2 = 5;
  const int ENV_FILTER = 6;

  const int BUFFER_SIZE =
      1200; // Identical results not promised if this is changed

  float timestretch; // overall time scaling

  struct envstatus envData[8]; // envelope running status
  bool chkOn[8];               // section on/off
  int Level[8];                // and level

  float LoudestLevel(void);
  int LongestEnv(void);
  bool UpdateEnv(int e, long t);
  void GetEnv(int env, const QString key);

  float waveform(float ph, int form);

  int qsString(const QString key, const QString def, char *buffer, int size);
  int qsInt(const QString key, int def);
  bool qsBool(const QString key, int def);
  float qsFloat(const QString key, float def);

  QSettings *IniData;
};

#endif
