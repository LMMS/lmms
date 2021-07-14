/*
 * DrumSynth.cpp - DrumSynth DS file renderer
 *
 * Copyright (c) 1998-2000 Paul Kellett (mda-vst.com)
 * Copyright (c) 2007 Paul Giblock <drfaygo/at/gmail.com>
 * Some modifications by Raine M. Ekman <raine/at/iki/dot/fi>,
 * no copyright claimed.
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

#include "DrumSynth.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

#include <cmath>

#include <QDebug>
#include <QFile>
#include <QSettings>

#ifdef LMMS_BUILD_WIN32
#define powf pow
#endif

#ifdef _MSC_VER
// not #if LMMS_BUILD_WIN32 because we have strncasecmp in mingw
#define strcasecmp _stricmp
#endif

using namespace std;

#define WORD __u16
#define DWORD __u32


DrumSynth::DrumSynth() {
	std::fill(envpts[0][0], envpts[0][0] + 8*2*32, 0.f);
}

int DrumSynth::LongestEnv(void) {
  long e, eon, p;
  float l = 0.f;

  for (e = 0; e < 6; e++) // The filter is excluded here, because... it's not a
                          // sound generator?
  {
    // adjust numbering, of course it's different between
    // envelopes and on/off switches for the sections :D
    eon = e;
    if (eon > 2)
      eon--;

    p = 0;
    while (envpts[e][0][p + 1] >= 0.f)
      p++;
    envData[e].last = envpts[e][0][p] * timestretch;
    if (chkOn[eon])
      l = max(l, envData[e].last);
  }
  // l *= timestretch;

  return BUFFER_SIZE * (2 + (int)(l / BUFFER_SIZE));
}

float DrumSynth::LoudestLevel(void) {
  float loudest = 0.f;
  int i = 0;

  while (i < 5) // 2
  {
    if (chkOn[i])
      loudest = max(loudest, (float)Level[i]);
    i++;
  }
  return (loudest * loudest);
}

// Update envelope when reaching new point, return whether end is reached.
// TODO: This return value should eventually
// replace the checks in the generation loop
bool DrumSynth::UpdateEnv(int e, long t) {
  float endEnv, dT;
  // 0.2's added
  envData[e].next =
      envpts[e][0][envData[e].pointer + 1] * timestretch; // get next point
  if (envData[e].next < 0) {
    envData[e].next = 442000 * timestretch; // if end point, hold
  }
  envData[e].value = envpts[e][1][envData[e].pointer] * 0.01f; // this level
  endEnv = envpts[e][1][envData[e].pointer + 1] * 0.01f;       // next level
  dT = envData[e].next - (float)t;
  dT = max(dT, 1.0f); // ensure step is always at least 1 sample in the future
  envData[e].delta = (endEnv - envData[e].value) / dT;
  envData[e].pointer++;
  return t < envData[e].last;
}

void DrumSynth::GetEnv(int env, const QString key) {
  // We get the string split on commas, in a QStringList.
  // i.e. "0,10 20,30" becomes {"0", "10 20", "30"}
  QStringList qsl = IniData->value(key, "0,0 100,0").toStringList();
  QString str = qsl.join(",");
  qsl = str.split(" ");

  // Now we should have {"0,10", "20, 30"}
  int n;
  for (n = 0; n < qsl.size() && n < 32; ++n) {
    QStringList pair = qsl.at(n).split(",");
    envpts[env][0][n] = pair.at(0).toFloat();
    envpts[env][1][n] = pair.at(1).toFloat();
  }
  envData[env].last = envpts[env][0][n - 1];
  // Put in the sentinel
  envpts[env][0][n] = -1;
}

float DrumSynth::waveform(float ph, int form) {
  float w;

  switch (form) {
  case 0:
    w = (float)sin(fmod(ph, TwoPi)); // sine
    break;
  case 1:
    w = (float)fabs(2.0f * (float)sin(fmod(0.5f * ph, TwoPi))) - 1.f; // sine^2
    break;
  case 2:
    while (ph < TwoPi)
      ph += TwoPi;
    w = 0.6366197f * (float)fmod(ph, TwoPi) - 1.f; // tri
    if (w > 1.f)
      w = 2.f - w;
    break;
  case 3:
    w = ph - TwoPi * (float)(int)(ph / TwoPi); // saw
    w = (0.3183098f * w) - 1.f;
    break;
  default:
    w = (sin(fmod(ph, TwoPi)) > 0.0) ? 1.f : -1.f; // square
    break;
  }

  return w;
}

// .ini file handling, good thing there is a QSettings class that handles the
// format. NB: the ini section General is represented by nothing in the keys
// when calling the reader functions, i.e. the key "xxx" under [General] is just
// plain "xxx", while it would be "othersection/xxx" under [othersection]
bool DrumSynth::LoadFile(QString file) {
  IniData = new QSettings(file, QSettings::IniFormat);
  return true;
}

inline int DrumSynth::qsString(const QString key, const QString def,
                                   char *buffer, int size) {
  QString str;
  str = IniData->value(key, def).toString();

  strncpy(buffer, str.toLocal8Bit().data(), size);
  return str.length();
}

inline int DrumSynth::qsInt(const QString key, int def) {
  return IniData->value(key, def).toInt();
}

inline bool DrumSynth::qsBool(const QString key, int def) {
  return IniData->value(key, def).toBool();
}

inline float DrumSynth::qsFloat(const QString key, float def) {
  return IniData->value(key, def).toFloat();
}

// Here we assume the file has been loaded and parsed in previously
int DrumSynth::GetSamples(int16_t *&wave, int channels, sample_rate_t Fs) {
  float DF[BUFFER_SIZE];  // The buffer audio is rendered into
  float phi[BUFFER_SIZE]; // Phase buffer... something?
  long wavewords;         // Counter

  short clippoint;

  // input file
  char ver[32];
  // char comment[256];
  // int commentLen=0;

  // generation
  long Length, tpos = 0, tplus, totmp, t, i, j;

  float MasterTune;
  int MainFilter, HighPass;

  // Switches for the sections
  bool NoiseOn, ToneOn, DistOn, Band1On, Band2On, OvertonesOn;

  // Noise
  float x[3] = {0.f, 0.f, 0.f};
  float a, b = 0.f, c = 0.f, d = 0.f, g, TT = 0.f;

  long NoiseSlope, DStep;

  float ToneLevel, NoiseLevel, F1, F2;
  float TphiStart = 0.f, Tphi, TDroopRate, ddF, DAtten, DGain;
  bool TDroop = false;

  // Noise bands
  long BFStep, BFStep2, botmp;
  float BdF = 0.f, BdF2 = 0.f, BPhi, BPhi2, BF, BF2, BQ, BQ2, BL, BL2;

  // Overtones
  bool OF1Sync = false, OF2Sync = false;
  long OMode, OW1, OW2;
  float Ophi1, Ophi2, OF1, OF2, OL, Ot = 0, OBal1, OBal2, ODrive;
  float Ocf1, Ocf2, OcF, OcQ, OcA, Oc[6][2]; // overtone cymbal mode
  float Oc0 = 0.0f, Oc1 = 0.0f, Oc2 = 0.0f;

  // Main filter
  float MFfb, MFtmp, MFres, MFin = 0.f, MFout = 0.f;

  // Downsampling variables
  float DownAve;
  long DownStart, DownEnd, jj;

  // try to read version from input file
  qsString("Version", "", ver, sizeof(ver));
  ver[9] = 0;
  if (strcasecmp(ver, "DrumSynth") != 0) {
    return 0;
  } // input fail
  if (ver[11] != '1' && ver[11] != '2') {
    return 0;
  } // version fail

  ////////////////////////////
  // read master parameters

  // Comment logic not needed, left for later.
  /* qsString("Comment","",comment,sizeof(comment));
  while((comment[commentLen]!=0) && (commentLen<254)) commentLen++;
  if(commentLen==0) {
          comment[0]=32;
          comment[1]=0;
          commentLen=1;
  }
  comment[commentLen+1]=0; commentLen++;
  if((commentLen % 2)==1) commentLen++;
        */

  // The stretch parameter adds time range at the cost of precision or vice
  // versa
  timestretch = .01f * qsFloat("Stretch", 100.0);
  timestretch = min(max(timestretch, 0.2f),
                    10.f); // TODO: C++17: clamp(timestretch, 0.2f, 10.f);
  // The unit of envelope lengths is a sample in 44100Hz sample rate,
  // so adjust it to fit the current sample rate
  timestretch *= Fs / 44100.f;

  DGain = (float)powf(10.0, 0.05 * qsFloat("Level", 0));

  MasterTune = qsFloat("Tuning", 0.0);
  MasterTune = (float)powf(1.0594631f, MasterTune);

  // 2 = filter all, 1 = filter only overtones
  MainFilter = qsBool("Filter", 0) ? 2 : qsBool("Overtones/Filter", 0) ? 1 : 0;

  MFres = 0.0101f * qsFloat("Resonance", 0.0);
  MFres = (float)powf(MFres, 0.5f);

  HighPass = qsInt("HighPass", 0);
  GetEnv(ENV_FILTER, "FilterEnv");

  // read noise parameters
  NoiseOn = chkOn[1] = qsBool("Noise/On", 0);
  Level[1] = qsInt("Noise/Level", 0);
  NoiseSlope = qsInt("Noise/Slope", 0);
  GetEnv(ENV_NOISE, "Noise/Envelope");
  NoiseLevel = (float)(Level[1] * Level[1]);
  if (NoiseSlope < 0) {
    a = 1.f + (NoiseSlope / 105.f);
    d = -NoiseSlope / 105.f;
    g = (1.f + 0.0005f * NoiseSlope * NoiseSlope) * NoiseLevel;
  } else {
    a = 1.f;
    b = -NoiseSlope / 50.f;
    c = (float)fabs((float)NoiseSlope) / 100.f;
    g = NoiseLevel;
  }

  if(qsBool("Noise/FixedSeq",0))
  {
    srand(1);
  }

  // read tone parameters
  ToneOn = chkOn[0] = qsBool("Tone/On", 0);
  Level[0] = qsInt("Tone/Level", 128);
  ToneLevel = (float)(Level[0] * Level[0]);
  GetEnv(ENV_TONE, "Tone/Envelope");
  F1 = MasterTune * TwoPi * qsFloat("Tone/F1", 200.0) / Fs;
  F1 = max(F1, 0.001f); // to prevent overtone ratio div0
  F2 = MasterTune * TwoPi * qsFloat("Tone/F2", 120.0) / Fs;
  TDroopRate = qsFloat("Tone/Droop", 0.f);
  if (TDroopRate > 0.f) {
    TDroopRate = (float)powf(10.0f, (TDroopRate - 20.0f) / 30.0f);
    TDroopRate = TDroopRate * -4.f / envData[ENV_TONE].last;
    TDroop = true;
    F2 = F1 +
         ((F2 - F1) / (1.f - (float)exp(TDroopRate * envData[ENV_TONE].last)));
    ddF = F1 - F2;
  } else
    ddF = F2 - F1;

  Tphi = qsFloat("Tone/Phase", 90.f) / 57.29578f; // degrees>radians

  // read overtone parameters
  OvertonesOn = chkOn[2] = qsBool("Overtones/On", 0);
  Level[2] = qsInt("Overtones/Level", 128);
  OL = (float)(Level[2] * Level[2]);
  GetEnv(ENV_OVERTONE1, "Overtones/Envelope1");
  GetEnv(ENV_OVERTONE2, "Overtones/Envelope2");
  OMode = qsInt("Overtones/Method", 2);
  OF1 = MasterTune * TwoPi * qsFloat("Overtones/F1", 200.0) / Fs;
  OF2 = MasterTune * TwoPi * qsFloat("Overtones/F2", 120.0) / Fs;
  OW1 = qsInt("Overtones/Wave1", 0);
  OW2 = qsInt("Overtones/Wave2", 0);
  OBal2 = (float)qsInt("Overtones/Param", 50);
  ODrive = (float)powf(OBal2, 3.0f) / (float)powf(50.0f, 3.0f);
  OBal2 *= 0.01f;
  OBal1 = 1.f - OBal2;
  Ophi1 = Tphi;
  Ophi2 = Tphi;
  if ((qsInt("Overtones/Track1", 0) == 1) && ToneOn) {
    OF1Sync = true;
    OF1 = OF1 / F1;
  }
  if ((qsInt("Overtones/Track2", 0) == 1) && ToneOn) {
    OF2Sync = true;
    OF2 = OF2 / F1;
  }

  OcA = 0.28f + OBal1 * OBal1; // overtone cymbal mode
  OcQ = OcA * OcA;
  OcF = (1.8f - 0.7f * OcQ) * 0.92f; // multiply by env 2
  OcA *= 1.0f + 4.0f * OBal1;        // level is a compromise!
  Ocf1 = TwoPi / OF1;
  Ocf2 = TwoPi / OF2;
  for (i = 0; i < 6; i++)
    Oc[i][0] = Oc[i][1] = Ocf1 + (Ocf2 - Ocf1) * 0.2f * (float)i;

  // read noise band parameters
  Band1On = chkOn[3] = qsBool("NoiseBand/On", 0);
  Level[3] = qsInt("NoiseBand/Level", 128);
  BL = (float)(Level[3] * Level[3]);
  BF = MasterTune * TwoPi * qsFloat("NoiseBand/F", 1000.0) / Fs;
  BPhi = TwoPi / 8.f;
  GetEnv(ENV_NOISEBAND, "NoiseBand/Envelope");
  BFStep = qsInt("NoiseBand/dF", 50);
  BQ = (float)BFStep;
  BQ = BQ * BQ / (10000.f - 6600.f * ((float)sqrt(BF) - 0.19f));
  BFStep = 1 + (int)((40.f - (BFStep / 2.5f)) / (BQ + 1.f + (1.f * BF)));

  Band2On = chkOn[4] = qsBool("NoiseBand2/On", 0);
  Level[4] = qsInt("NoiseBand2/Level", 128);
  BL2 = (float)(Level[4] * Level[4]);
  BF2 = MasterTune * TwoPi * qsFloat("NoiseBand2/F", 1000.0) / Fs;
  BPhi2 = TwoPi / 8.f;
  GetEnv(ENV_NOISEBAND2, "NoiseBand2/Envelope");
  BFStep2 = qsInt("NoiseBand2/dF", 50);
  BQ2 = (float)BFStep2;
  BQ2 = BQ2 * BQ2 / (10000.f - 6600.f * ((float)sqrt(BF2) - 0.19f));
  BFStep2 = 1 + (int)((40 - (BFStep2 / 2.5)) / (BQ2 + 1 + (1 * BF2)));

  // read distortion parameters
  DistOn = chkOn[5] = qsBool("Distortion/On", 0);
  DStep = 1 + qsInt("Distortion/Rate", 0);
  if (DStep == 7)
    DStep = 20;
  if (DStep == 6)
    DStep = 10;
  if (DStep == 5)
    DStep = 8;

  clippoint = 32700;
  DAtten = 1.0f;

  if (DistOn) {
    DAtten = DGain * (short)LoudestLevel();
    clippoint = (short)min((int)DAtten, 32700);
    DAtten = (float)powf(2.0, 2.0 * qsInt("Distortion/Bits", 0));
    DGain = DAtten * DGain *
            (float)powf(10.0, 0.05 * qsInt("Distortion/Clipping", 0));
  }

  // prepare envelopes
  for (i = 0; i < 7; i++) {
    envData[i].next = 0;
    envData[i].pointer = 0;
  }

  Length = LongestEnv();

  // allocate the buffer
  // if(wave!=NULL) free(wave);
  wave = new int16_t[channels * Length]; // wave memory buffer
  if (wave == NULL) {
    return 0;
  }
  wavewords = 0;

  /////////////////////////////////////////////
  // Generate samples.
  tpos = 0;
  while (tpos < Length) {
    tplus = tpos + BUFFER_SIZE - 1; // Last index of buffer...

    // First up noise, if not enabled fill buffer with silence.
    if (NoiseOn) {
      for (t = tpos; t <= tplus; t++) {
        if (t < envData[ENV_NOISE].next) {
          envData[ENV_NOISE].value += envData[ENV_NOISE].delta;
        } else {
          NoiseOn = UpdateEnv(ENV_NOISE, t);
        }
        x[2] = x[1];
        x[1] = x[0];
        x[0] = (2.f * (float)rand() / RAND_MAX) - 1.f;
        TT = a * x[0] + b * x[1] + c * x[2] + d * TT;
        DF[t - tpos] = TT * g * envData[ENV_NOISE].value;
      }
      if (t >= envData[ENV_NOISE].last) {
        NoiseOn = false;
      }
    } else {
      std::fill(DF, DF + BUFFER_SIZE, 0.f);
    }

    // The main tone
    if (ToneOn) {
      TphiStart = Tphi;
      if (TDroop) {
        for (t = tpos; t <= tplus; t++) {
          phi[t - tpos] = F2 + (ddF * (float)exp(t * TDroopRate));
        }
      } else {
        for (t = tpos; t <= tplus; t++) {
          phi[t - tpos] = F1 + (t / envData[ENV_TONE].last) * ddF;
        }
      }
      for (t = tpos; t <= tplus; t++) {
        totmp = t - tpos;
        if (t < envData[ENV_TONE].next) {
          envData[ENV_TONE].value += envData[ENV_TONE].delta;
        } else {
          UpdateEnv(ENV_TONE, t);
        }
        Tphi = Tphi + phi[totmp];
        DF[totmp] += ToneLevel * envData[ENV_TONE].value *
                     (float)sin(fmod(Tphi, TwoPi)); // overflow?
      }
      if (t >= envData[ENV_TONE].last) {
        ToneOn = false;
      }
    } else {
      std::fill(phi, phi + BUFFER_SIZE, F2);
    }

    // Turning these 2 noise bands into one SIMD-friendlier loop might make
    // sense but as there is randomness involved it'll be hard to verify
    // correctness using any bit-exact methods. Anyway, it's a small victory.
    if (Band1On) // noise band 1
    {
      for (t = tpos; t <= tplus; t++) {
        if (t < envData[ENV_NOISEBAND].next) {
          envData[ENV_NOISEBAND].value =
              envData[ENV_NOISEBAND].value + envData[ENV_NOISEBAND].delta;
        } else {
          UpdateEnv(ENV_NOISEBAND, t);
        }
        if ((t % BFStep) == 0) {
          BdF = (float)rand() / RAND_MAX - 0.5f;
        }
        BPhi = BPhi + BF + BQ * BdF;
        botmp = t - tpos;
        DF[botmp] = DF[botmp] + (float)cos(fmod(BPhi, TwoPi)) *
                                    envData[ENV_NOISEBAND].value * BL;
      }
      if (t >= envData[ENV_NOISEBAND].last)
        Band1On = false;
    }

    if (Band2On) // noise band 2
    {
      for (t = tpos; t <= tplus; t++) {
        if (t < envData[ENV_NOISEBAND2].next)
          envData[ENV_NOISEBAND2].value =
              envData[ENV_NOISEBAND2].value + envData[ENV_NOISEBAND2].delta;
        else
          UpdateEnv(ENV_NOISEBAND2, t);
        if ((t % BFStep2) == 0)
          BdF2 = (float)rand() / RAND_MAX - 0.5f;
        BPhi2 = BPhi2 + BF2 + BQ2 * BdF2;
        botmp = t - tpos;
        DF[botmp] = DF[botmp] + (float)cos(fmod(BPhi2, TwoPi)) *
                                    envData[ENV_NOISEBAND2].value * BL2;
      }
      if (t >= envData[ENV_NOISEBAND2].last)
        Band2On = false;
    }

    // Generate overtones and do filtering
    for (t = tpos; t <= tplus; t++) {
      if (OvertonesOn) // overtones
      {
        if (t < envData[ENV_OVERTONE1].next)
          envData[ENV_OVERTONE1].value =
              envData[ENV_OVERTONE1].value + envData[ENV_OVERTONE1].delta;
        else {
          if (t >= envData[ENV_OVERTONE1].last) // wait for OT2
          {
            envData[ENV_OVERTONE1].value = 0;
            envData[ENV_OVERTONE1].delta = 0;
            envData[ENV_OVERTONE1].next = 999999;
          } else
            UpdateEnv(ENV_OVERTONE1, t);
        }
        //
        if (t < envData[ENV_OVERTONE2].next)
          envData[ENV_OVERTONE2].value =
              envData[ENV_OVERTONE2].value + envData[ENV_OVERTONE2].delta;
        else {
          if (t >= envData[ENV_OVERTONE2].last) // wait for OT1
          {
            envData[ENV_OVERTONE2].value = 0;
            envData[ENV_OVERTONE2].delta = 0;
            envData[ENV_OVERTONE2].next = 999999;
          } else
            UpdateEnv(ENV_OVERTONE2, t);
        }
        //
        TphiStart = TphiStart + phi[t - tpos];
        if (OF1Sync)
          Ophi1 = TphiStart * OF1;
        else
          Ophi1 = Ophi1 + OF1;
        if (OF2Sync)
          Ophi2 = TphiStart * OF2;
        else
          Ophi2 = Ophi2 + OF2;
        Ot = 0.0f;
        switch (OMode) {
        case 0: // add
          Ot = OBal1 * envData[ENV_OVERTONE1].value * waveform(Ophi1, OW1);
          Ot = OL * (Ot + OBal2 * envData[ENV_OVERTONE2].value *
                              waveform(Ophi2, OW2));
          break;

        case 1: // FM
          Ot = ODrive * envData[ENV_OVERTONE2].value * waveform(Ophi2, OW2);
          Ot = OL * envData[ENV_OVERTONE1].value * waveform(Ophi1 + Ot, OW1);
          break;

        case 2: // RM
          Ot = (1 - ODrive / 8) +
               (((ODrive / 8) * envData[ENV_OVERTONE2].value) *
                waveform(Ophi2, OW2));
          Ot = OL * envData[ENV_OVERTONE1].value * waveform(Ophi1, OW1) * Ot;
          break;

        case 3: // 808 Cymbal
          for (j = 0; j < 6; j++) {
            Oc[j][0] += 1.0f;

            if (Oc[j][0] > Oc[j][1]) {
              Oc[j][0] -= Oc[j][1];
              Ot = OL * envData[ENV_OVERTONE1].value;
            }
          }
          Ocf1 = envData[ENV_OVERTONE2].value * OcF; // filter freq
          Oc0 += Ocf1 * Oc1;
          Oc1 += Ocf1 * (Ot + Oc2 - OcQ * Oc1 - Oc0); // bpf
          Oc2 = Ot;
          Ot = Oc1;
          break;
        }
      }

      if (MainFilter > 0) {
        if (t < envData[ENV_FILTER].next)
          envData[ENV_FILTER].value =
              envData[ENV_FILTER].value + envData[ENV_FILTER].delta;
        else
          UpdateEnv(ENV_FILTER, t);

        MFtmp = envData[ENV_FILTER].value;
        if (MFtmp > 0.2f)
          MFfb = 1.001f - (float)powf(10.0f, MFtmp - 1);
        else
          MFfb = 0.999f - 0.7824f * MFtmp;

        float filter_in =
            DF[t - tpos] * (MainFilter == 2) + Ot * (MainFilter > 0);
        float HP = filter_in * HighPass;

        MFtmp = filter_in + MFres * (1.f + (1.f / MFfb)) * (MFin - MFout);
        MFin = MFfb * (MFin - MFtmp) + MFtmp;
        MFout = MFfb * (MFout - MFin) + MFin;

        DF[t - tpos] =
            (MFout - HP) +                   // Filter to output
            DF[t - tpos] * (MainFilter < 2); // Main to output if needed
      } else {
        DF[t - tpos] = DF[t - tpos] + Ot; // no filter
      }
    }

    if (DistOn) // bit resolution
    {
      for (j = 0; j < BUFFER_SIZE; j++)
        DF[j] = DGain * (int)(DF[j] / DAtten);

      for (j = 0; j < BUFFER_SIZE; j += DStep) // downsampling
      {
        DownAve = 0;
        DownStart = j;
        DownEnd = j + DStep - 1;
        for (jj = DownStart; jj <= DownEnd; jj++)
          DownAve = DownAve + DF[jj];
        DownAve = DownAve / DStep;
        for (jj = DownStart; jj <= DownEnd; jj++)
          DF[jj] = DownAve;
      }
    } else
      for (j = 0; j < BUFFER_SIZE; j++)
        DF[j] *= DGain;

    for (j = 0; j < BUFFER_SIZE; j++) // clipping + output
    {
      if (DF[j] > clippoint)
        wave[wavewords++] = clippoint;
      else if (DF[j] < -clippoint)
        wave[wavewords++] = -clippoint;
      else
        wave[wavewords++] = (short)DF[j];

      for (int c = 1; c < channels; c++) {
        wave[wavewords] = wave[wavewords - 1];
        wavewords++;
      }
    }

    tpos = tpos + BUFFER_SIZE;
  }

  return Length;
}
