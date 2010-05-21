/* Calf DSP Library
 * FFT class
 *
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */

#ifndef __CALF_FFT_H
#define __CALF_FFT_H

#include <complex>

namespace dsp {

/// FFT routine copied from my old OneSignal library, modified to 
/// match Calf's style. It's not fast at all, just a straightforward
/// implementation.
template<class T, int O>
class fft
{
    typedef typename std::complex<T> complex;
    int scramble[1<<O];
    complex sines[1<<O];
public:
    fft()
    {
        int N=1<<O;
        assert(N >= 4);
        for (int i=0; i<N; i++)
        {
            int v=0;
            for (int j=0; j<O; j++)
                if (i&(1<<j))
                    v+=(N>>(j+1));
            scramble[i]=v;
        }
        int N90 = N >> 2;
        T divN = 2 * M_PI / N;
        // use symmetry
        for (int i=0; i<N90; i++)
        {
            T angle = divN * i;
            T c = cos(angle), s = sin(angle);
            sines[i + 3 * N90] = -(sines[i + N90] = complex(-s, c));
            sines[i + 2 * N90] = -(sines[i] = complex(c, s));
        }
    }
    void calculate(complex *input, complex *output, bool inverse)
    {
        int N=1<<O;
        int N1=N-1;
        int i;
        // Scramble the input data
        if (inverse)
        {
            float mf=1.0/N;
            for (i=0; i<N; i++)
            {
                complex &c=input[scramble[i]];
                output[i]=mf*complex(c.imag(),c.real());
            }
        }
        else
            for (i=0; i<N; i++)
                output[i]=input[scramble[i]];

        // O butterfiles
        for (i=0; i<O; i++)
        {
            int PO=1<<i, PNO=1<<(O-i-1);
            int j,k;
            for (j=0; j<PNO; j++)
            {
                int base=j<<(i+1);
                for (k=0; k<PO; k++)
                {
                    int B1=base+k;
                    int B2=base+k+(1<<i);
                    complex r1=output[B1];
                    complex r2=output[B2];
                    output[B1]=r1+r2*sines[(B1<<(O-i-1))&N1];
                    output[B2]=r1+r2*sines[(B2<<(O-i-1))&N1];
                }
            }
        }
        if (inverse)
        {
            for (i=0; i<N; i++)
            {
                const complex &c=output[i];
                output[i]=complex(c.imag(),c.real());
            }
        }
    }
};

};

#endif
