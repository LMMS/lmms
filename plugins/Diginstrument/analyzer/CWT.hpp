#pragma once

#include "wavelib.h"

#include <string>
#include <vector>
#include <complex>

//Interface for wavelib: https://github.com/rafat/wavelib

/*
wave_object wave; // wavelet object
char *wave; // Wavelet - “morl”/”morlet”,”paul” or “dog”/”dgauss”
int siglength;// Length of the original signal.
double s0;// Smallest scale. Typically s0 <= 2 * dt 
int J; // Total Number of Scales
double dt; // Sampling Rate
char type[10];// Scale Type - “pow” or “linear”
int pow; // Base of power if scale type = “pow”. Default pow = 2
double dj; // Separation between Scales. eg., scale = s0 * 2 ^ ( [0:N-1] *dj) or scale = s0 * [0:N-1] * dj
double m; // Wavelet parameter param
double smean; // Signal Mean
cplx_data *output; // CWT Output Vector of size J * siglength. The vector is complex. The ith real value can be accessed wt->output[i].re and imaginary value by wt->output[i].im
double *scale; // Scale vector of size J
double *period; // Period vector of size J
double *coi; // Cone of Influence vector of size siglength
*/

class CWT
{
private:
    cwt_object wt;
    std::string wavelet;
    const std::string type = "pow";
    double waveletParameter;
    double dt;
    unsigned int level;
    unsigned int signalLength;

public:
    static constexpr unsigned int octaves = 11;

    class TimeInstance
    {
      public:
        const double scale, period;
        const std::complex<double> value;
        TimeInstance(double scale, double period, double re, double im) : scale(scale), period(period), value(re, im) {}
    };

    bool setWavelet(const std::string &name, const double &parameter, unsigned int level, unsigned int sampleRate)
    {
        this->wavelet = name;
        this->waveletParameter = parameter;
        this->level = level;
        this->dt = 1.0/(double)sampleRate;
        return true;
    }

    void operator()(const std::vector<double> &signal)
    {
        double s0 = 2 * this->dt;   //Smallest scale. Typically s0 <= 2 * dt
        double dj = 1.0f / ((double)level); //Separation between Scales.
        int totalScales = octaves * level;
        signalLength = signal.size();

        wt = cwt_init(this->wavelet.c_str(), this->waveletParameter, signalLength, this->dt, totalScales);
        setCWTScales(wt, s0, dj, this->type.c_str(), 2);
        cwt(wt, &signal[0]);
    }

    std::vector<TimeInstance> operator[](unsigned int time)
    {
        unsigned int totalScales = octaves * level;
        std::vector<TimeInstance> res;
        res.reserve(octaves * level);
        for (int k = 0; k < totalScales; ++k)
        {
            int i = time + k * signalLength;
            res.emplace_back(wt->scale[k] ,wt->period[k], wt->output[i].re, wt->output[i].im);
        }
        return res;
    }

    std::vector<double> inverseTransform()
    {
        std::vector<double> res(signalLength);
        icwt(wt, &res[0]);
        return res;
    }

    static std::vector<std::complex<double>> singleScaleCWT(const std::vector<double> & signal, double scale, unsigned int sampleRate, std::string waveletName = "morlet", int waveletParameter = 6)
    {
        cwt_object wt = cwt_init(waveletName.c_str(), waveletParameter, signal.size(), 1.0/(double)sampleRate, 1);
        setCWTScales(wt, scale, 0, "lin", 0);
        cwt(wt, &signal[0]);
        std::vector<std::complex<double>> res;
        res.reserve(signal.size());
        for(int i = 0; i<signal.size(); i++)
        {
            res.emplace_back(wt->output[i].re, wt->output[i].im);
        }
        cwt_free(wt);
        return res;
    }

    //TODO: not actually correct; only works if the parameter = 6
    static double calculateMagnitudeNormalizationConstant(double waveletParameter)
    {
        return pow((4*M_PI) / (waveletParameter+sqrt(waveletParameter*waveletParameter+2)), 2);
    }

    CWT(const std::string &name, const double &parameter, unsigned int level, unsigned int sampleRate)
    {
        setWavelet(name, parameter, level, sampleRate);
    }

    ~CWT()
    {
        cwt_free(wt);
    }
};