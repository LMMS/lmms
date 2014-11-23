/*
  ZynAddSubFX - a software synthesizer

  FFTwrapper.c  -  A wrapper for Fast Fourier Transforms
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <cmath>
#include <cassert>
#include <cstring>
#include "FFTwrapper.h"

FFTwrapper::FFTwrapper(int fftsize_)
{
    fftsize  = fftsize_;
    time     = new fftw_real[fftsize];
    fft      = new fftwf_complex[fftsize + 1];
    planfftw = fftwf_plan_dft_r2c_1d(fftsize,
                                    time,
                                    fft,
                                    FFTW_ESTIMATE);
    planfftw_inv = fftwf_plan_dft_c2r_1d(fftsize,
                                        fft,
                                        time,
                                        FFTW_ESTIMATE);
}

FFTwrapper::~FFTwrapper()
{
    fftwf_destroy_plan(planfftw);
    fftwf_destroy_plan(planfftw_inv);

    delete [] time;
    delete [] fft;
}

void FFTwrapper::smps2freqs(const float *smps, fft_t *freqs)
{
    //Load data
    for(int i = 0; i < fftsize; ++i)
        time[i] = smps[i];

    //DFT
    fftwf_execute(planfftw);

    //Grab data
    memcpy((void *)freqs, (const void *)fft, fftsize * sizeof(fftw_real));
}

void FFTwrapper::freqs2smps(const fft_t *freqs, float *smps)
{
    //Load data
    memcpy((void *)fft, (const void *)freqs, fftsize * sizeof(fftw_real));

    //clear unused freq channel
    fft[fftsize / 2][0] = 0.0f;
    fft[fftsize / 2][1] = 0.0f;

    //IDFT
    fftwf_execute(planfftw_inv);

    //Grab data
    for(int i = 0; i < fftsize; ++i)
        smps[i] = static_cast<float>(time[i]);
}

void FFT_cleanup()
{
    fftwf_cleanup();
}
