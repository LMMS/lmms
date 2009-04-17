/*
  ZynAddSubFX - a software synthesizer
 
  FFTwrapper.h  -  A wrapper for Fast Fourier Transforms
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

#ifndef FFT_WRAPPER_H
#define FFT_WRAPPER_H

#include "../globals.h"

#include <fftw3.h>
#define fftw_real float
#define rfftw_plan fftwf_plan

class FFTwrapper{
    public:
	FFTwrapper(int fftsize_);
	~FFTwrapper();
	void smps2freqs(REALTYPE *smps,FFTFREQS freqs);
	void freqs2smps(FFTFREQS freqs,REALTYPE *smps);
    private:
	int fftsize;
	fftw_real *tmpfftdata1,*tmpfftdata2;
	rfftw_plan planfftw,planfftw_inv;
};
#endif

