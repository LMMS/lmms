/*
  ZynAddSubFX - a software synthesizer

  WavFile.h - Records sound to a file
  Copyright (C) 2008 Nasca Octavian Paul
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#ifndef WAVFILE_H
#define WAVFILE_H
#include <string>

class WavFile
{
    public:
        WavFile(std::string filename, int samplerate, int channels);
        ~WavFile();

        bool good() const;

        void writeMonoSamples(int nsmps, short int *smps);
        void writeStereoSamples(int nsmps, short int *smps);

    private:
        int   sampleswritten;
        int   samplerate;
        int   channels;
        FILE *file;
};
#endif
