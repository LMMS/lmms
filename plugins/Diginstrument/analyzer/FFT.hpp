#pragma once

#include <vector>
#include <fftw3.h>

namespace Diginstrument
{
class FFT
{
  public:
    std::vector<std::pair<double, double>> operator()(const std::vector<double> & signal, unsigned int sampleRate)
    {
        //TODO: exception
        if(signal.size()!=in.size()) return {};
        std::copy(signal.begin(), signal.end(), in.begin());
        fftw_execute(plan);
        
        const unsigned int outSize = floor(N/2)+1;
        std::vector<std::pair<double, double>> res(outSize);
        for(int i = 0; i<outSize; i++)
        {
            const double re = out[i][0]/outSize;
            const double im = out[i][1]/outSize;
            res[i] = std::make_pair((double)i/((double)N/(double)sampleRate), sqrt(re*re + im*im));
        }
        return res;
    }

    FFT(unsigned int signalLength) : N(signalLength)
    {
        in.resize(N);
        out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N/2+1));
        plan = fftw_plan_dft_r2c_1d(N, &in[0], out, FFTW_DESTROY_INPUT | FFTW_ESTIMATE);
    }

    ~FFT()
    {
        fftw_destroy_plan(plan);
        fftw_free(out);
        fftw_cleanup();
    }

  private:
    unsigned int N;
    fftw_plan plan;
    std::vector<double> in;
    fftw_complex * out;
};
}