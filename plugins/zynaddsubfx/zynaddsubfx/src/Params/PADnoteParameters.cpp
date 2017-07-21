/*
  ZynAddSubFX - a software synthesizer

  PADnoteParameters.cpp - Parameters for PADnote (PADsynth)
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
#include <math.h>
#include "PADnoteParameters.h"
#include "../Misc/WavFile.h"

PADnoteParameters::PADnoteParameters(FFTwrapper *fft_,
                                     pthread_mutex_t *mutex_):Presets()
{
    setpresettype("Ppadsynth");

    fft   = fft_;
    mutex = mutex_;

    resonance = new Resonance();
    oscilgen  = new OscilGen(fft_, resonance);
    oscilgen->ADvsPAD = true;

    FreqEnvelope = new EnvelopeParams(0, 0);
    FreqEnvelope->ASRinit(64, 50, 64, 60);
    FreqLfo = new LFOParams(70, 0, 64, 0, 0, 0, 0, 0);

    AmpEnvelope = new EnvelopeParams(64, 1);
    AmpEnvelope->ADSRinit_dB(0, 40, 127, 25);
    AmpLfo = new LFOParams(80, 0, 64, 0, 0, 0, 0, 1);

    GlobalFilter   = new FilterParams(2, 94, 40);
    FilterEnvelope = new EnvelopeParams(0, 1);
    FilterEnvelope->ADSRinit_filter(64, 40, 64, 70, 60, 64);
    FilterLfo = new LFOParams(80, 0, 64, 0, 0, 0, 0, 2);

    for(int i = 0; i < PAD_MAX_SAMPLES; ++i)
        sample[i].smp = NULL;
    newsample.smp = NULL;

    defaults();
}

PADnoteParameters::~PADnoteParameters()
{
    deletesamples();
    delete (oscilgen);
    delete (resonance);

    delete (FreqEnvelope);
    delete (FreqLfo);
    delete (AmpEnvelope);
    delete (AmpLfo);
    delete (GlobalFilter);
    delete (FilterEnvelope);
    delete (FilterLfo);
}

void PADnoteParameters::defaults()
{
    Pmode = 0;
    Php.base.type      = 0;
    Php.base.par1      = 80;
    Php.freqmult       = 0;
    Php.modulator.par1 = 0;
    Php.modulator.freq = 30;
    Php.width     = 127;
    Php.amp.type  = 0;
    Php.amp.mode  = 0;
    Php.amp.par1  = 80;
    Php.amp.par2  = 64;
    Php.autoscale = true;
    Php.onehalf   = 0;

    setPbandwidth(500);
    Pbwscale = 0;

    resonance->defaults();
    oscilgen->defaults();

    Phrpos.type = 0;
    Phrpos.par1 = 64;
    Phrpos.par2 = 64;
    Phrpos.par3 = 0;

    Pquality.samplesize = 3;
    Pquality.basenote   = 4;
    Pquality.oct    = 3;
    Pquality.smpoct = 2;

    PStereo = 1; //stereo
    /* Frequency Global Parameters */
    Pfixedfreq    = 0;
    PfixedfreqET  = 0;
    PDetune       = 8192; //zero
    PCoarseDetune = 0;
    PDetuneType   = 1;
    FreqEnvelope->defaults();
    FreqLfo->defaults();

    /* Amplitude Global Parameters */
    PVolume  = 90;
    PPanning = 64; //center
    PAmpVelocityScaleFunction = 64;
    AmpEnvelope->defaults();
    AmpLfo->defaults();
    PPunchStrength = 0;
    PPunchTime     = 60;
    PPunchStretch  = 64;
    PPunchVelocitySensing = 72;

    /* Filter Global Parameters*/
    PFilterVelocityScale = 64;
    PFilterVelocityScaleFunction = 64;
    GlobalFilter->defaults();
    FilterEnvelope->defaults();
    FilterLfo->defaults();

    deletesamples();
}

void PADnoteParameters::deletesample(int n)
{
    if((n < 0) || (n >= PAD_MAX_SAMPLES))
        return;
    if(sample[n].smp != NULL) {
        delete[] sample[n].smp;
        sample[n].smp = NULL;
    }
    sample[n].size     = 0;
    sample[n].basefreq = 440.0f;
}

void PADnoteParameters::deletesamples()
{
    for(int i = 0; i < PAD_MAX_SAMPLES; ++i)
        deletesample(i);
}

/*
 * Get the harmonic profile (i.e. the frequency distributio of a single harmonic)
 */
float PADnoteParameters::getprofile(float *smp, int size)
{
    for(int i = 0; i < size; ++i)
        smp[i] = 0.0f;
    const int supersample = 16;
    float     basepar     = powf(2.0f, (1.0f - Php.base.par1 / 127.0f) * 12.0f);
    float     freqmult    = floor(powf(2.0f,
                                       Php.freqmult / 127.0f
                                       * 5.0f) + 0.000001f);

    float modfreq = floor(powf(2.0f,
                               Php.modulator.freq / 127.0f
                               * 5.0f) + 0.000001f);
    float modpar1 = powf(Php.modulator.par1 / 127.0f, 4.0f) * 5.0f / sqrt(
        modfreq);
    float amppar1 =
        powf(2.0f, powf(Php.amp.par1 / 127.0f, 2.0f) * 10.0f) - 0.999f;
    float amppar2 = (1.0f - Php.amp.par2 / 127.0f) * 0.998f + 0.001f;
    float width   = powf(150.0f / (Php.width + 22.0f), 2.0f);

    for(int i = 0; i < size * supersample; ++i) {
        bool  makezero = false;
        float x = i * 1.0f / (size * (float) supersample);

        float origx = x;

        //do the sizing (width)
        x = (x - 0.5f) * width + 0.5f;
        if(x < 0.0f) {
            x = 0.0f;
            makezero = true;
        }
        else
        if(x > 1.0f) {
            x = 1.0f;
            makezero = true;
        }

        //compute the full profile or one half
        switch(Php.onehalf) {
            case 1:
                x = x * 0.5f + 0.5f;
                break;
            case 2:
                x = x * 0.5f;
                break;
        }

        float x_before_freq_mult = x;

        //do the frequency multiplier
        x *= freqmult;

        //do the modulation of the profile
        x += sinf(x_before_freq_mult * 3.1415926f * modfreq) * modpar1;
        x  = fmod(x + 1000.0f, 1.0f) * 2.0f - 1.0f;


        //this is the base function of the profile
        float f;
        switch(Php.base.type) {
            case 1:
                f = expf(-(x * x) * basepar);
                if(f < 0.4f)
                    f = 0.0f;
                else
                    f = 1.0f;
                break;
            case 2:
                f = expf(-(fabs(x)) * sqrt(basepar));
                break;
            default:
                f = expf(-(x * x) * basepar);
                break;
        }
        if(makezero)
            f = 0.0f;

        float amp = 1.0f;
        origx = origx * 2.0f - 1.0f;

        //compute the amplitude multiplier
        switch(Php.amp.type) {
            case 1:
                amp = expf(-(origx * origx) * 10.0f * amppar1);
                break;
            case 2:
                amp = 0.5f
                      * (1.0f
                         + cosf(3.1415926f * origx * sqrt(amppar1 * 4.0f + 1.0f)));
                break;
            case 3:
                amp = 1.0f
                      / (powf(origx * (amppar1 * 2.0f + 0.8f), 14.0f) + 1.0f);
                break;
        }

        //apply the amplitude multiplier
        float finalsmp = f;
        if(Php.amp.type != 0)
            switch(Php.amp.mode) {
                case 0:
                    finalsmp = amp * (1.0f - amppar2) + finalsmp * amppar2;
                    break;
                case 1:
                    finalsmp *= amp * (1.0f - amppar2) + amppar2;
                    break;
                case 2:
                    finalsmp = finalsmp
                               / (amp + powf(amppar2, 4.0f) * 20.0f + 0.0001f);
                    break;
                case 3:
                    finalsmp = amp
                               / (finalsmp
                                  + powf(amppar2, 4.0f) * 20.0f + 0.0001f);
                    break;
            }
        ;

        smp[i / supersample] += finalsmp / supersample;
    }

    //normalize the profile (make the max. to be equal to 1.0f)
    float max = 0.0f;
    for(int i = 0; i < size; ++i) {
        if(smp[i] < 0.0f)
            smp[i] = 0.0f;
        if(smp[i] > max)
            max = smp[i];
    }
    if(max < 0.00001f)
        max = 1.0f;
    for(int i = 0; i < size; ++i)
        smp[i] /= max;

    if(!Php.autoscale)
        return 0.5f;

    //compute the estimated perceived bandwidth
    float sum = 0.0f;
    int   i;
    for(i = 0; i < size / 2 - 2; ++i) {
        sum += smp[i] * smp[i] + smp[size - i - 1] * smp[size - i - 1];
        if(sum >= 4.0f)
            break;
    }

    float result = 1.0f - 2.0f * i / (float) size;
    return result;
}

/*
 * Compute the real bandwidth in cents and returns it
 * Also, sets the bandwidth parameter
 */
float PADnoteParameters::setPbandwidth(int Pbandwidth)
{
    this->Pbandwidth = Pbandwidth;
    float result = powf(Pbandwidth / 1000.0f, 1.1f);
    result = powf(10.0f, result * 4.0f) * 0.25f;
    return result;
}

/*
 * Get the harmonic(overtone) position
 */
float PADnoteParameters::getNhr(int n)
{
    float result = 1.0f;
    float par1   = powf(10.0f, -(1.0f - Phrpos.par1 / 255.0f) * 3.0f);
    float par2   = Phrpos.par2 / 255.0f;

    float n0     = n - 1.0f;
    float tmp    = 0.0f;
    int   thresh = 0;
    switch(Phrpos.type) {
        case 1:
            thresh = (int)(par2 * par2 * 100.0f) + 1;
            if(n < thresh)
                result = n;
            else
                result = 1.0f + n0 + (n0 - thresh + 1.0f) * par1 * 8.0f;
            break;
        case 2:
            thresh = (int)(par2 * par2 * 100.0f) + 1;
            if(n < thresh)
                result = n;
            else
                result = 1.0f + n0 - (n0 - thresh + 1.0f) * par1 * 0.90f;
            break;
        case 3:
            tmp    = par1 * 100.0f + 1.0f;
            result = powf(n0 / tmp, 1.0f - par2 * 0.8f) * tmp + 1.0f;
            break;
        case 4:
            result = n0
                     * (1.0f
                        - par1)
                     + powf(n0 * 0.1f, par2 * 3.0f
                            + 1.0f) * par1 * 10.0f + 1.0f;
            break;
        case 5:
            result = n0
                     + sinf(n0 * par2 * par2 * PI
                            * 0.999f) * sqrt(par1) * 2.0f + 1.0f;
            break;
        case 6:
            tmp    = powf(par2 * 2.0f, 2.0f) + 0.1f;
            result = n0 * powf(1.0f + par1 * powf(n0 * 0.8f, tmp), tmp) + 1.0f;
            break;
        case 7:
            result = (n + Phrpos.par1 / 255.0f) / (Phrpos.par1 / 255.0f + 1);
            break;
        default:
            result = n;
            break;
    }

    float par3 = Phrpos.par3 / 255.0f;

    float iresult = floor(result + 0.5f);
    float dresult = result - iresult;

    result = iresult + (1.0f - par3) * dresult;

    return result;
}

/*
 * Generates the long spectrum for Bandwidth mode (only amplitudes are generated; phases will be random)
 */
void PADnoteParameters::generatespectrum_bandwidthMode(float *spectrum,
                                                       int size,
                                                       float basefreq,
                                                       float *profile,
                                                       int profilesize,
                                                       float bwadjust)
{
    for(int i = 0; i < size; ++i)
        spectrum[i] = 0.0f;

    float harmonics[synth->oscilsize / 2];
    for(int i = 0; i < synth->oscilsize / 2; ++i)
        harmonics[i] = 0.0f;
    //get the harmonic structure from the oscillator (I am using the frequency amplitudes, only)
    oscilgen->get(harmonics, basefreq, false);

    //normalize
    float max = 0.0f;
    for(int i = 0; i < synth->oscilsize / 2; ++i)
        if(harmonics[i] > max)
            max = harmonics[i];
    if(max < 0.000001f)
        max = 1;
    for(int i = 0; i < synth->oscilsize / 2; ++i)
        harmonics[i] /= max;

    for(int nh = 1; nh < synth->oscilsize / 2; ++nh) { //for each harmonic
        float realfreq = getNhr(nh) * basefreq;
        if(realfreq > synth->samplerate_f * 0.49999f)
            break;
        if(realfreq < 20.0f)
            break;
        if(harmonics[nh - 1] < 1e-4)
            continue;

        //compute the bandwidth of each harmonic
        float bandwidthcents = setPbandwidth(Pbandwidth);
        float bw =
            (powf(2.0f, bandwidthcents / 1200.0f) - 1.0f) * basefreq / bwadjust;
        float power = 1.0f;
        switch(Pbwscale) {
            case 0:
                power = 1.0f;
                break;
            case 1:
                power = 0.0f;
                break;
            case 2:
                power = 0.25f;
                break;
            case 3:
                power = 0.5f;
                break;
            case 4:
                power = 0.75f;
                break;
            case 5:
                power = 1.5f;
                break;
            case 6:
                power = 2.0f;
                break;
            case 7:
                power = -0.5f;
                break;
        }
        bw = bw * powf(realfreq / basefreq, power);
        int ibw = (int)((bw / (synth->samplerate_f * 0.5f) * size)) + 1;

        float amp = harmonics[nh - 1];
        if(resonance->Penabled)
            amp *= resonance->getfreqresponse(realfreq);

        if(ibw > profilesize) { //if the bandwidth is larger than the profilesize
            float rap   = sqrt((float)profilesize / (float)ibw);
            int   cfreq =
                (int) (realfreq
                       / (synth->samplerate_f * 0.5f) * size) - ibw / 2;
            for(int i = 0; i < ibw; ++i) {
                int src    = (int)(i * rap * rap);
                int spfreq = i + cfreq;
                if(spfreq < 0)
                    continue;
                if(spfreq >= size)
                    break;
                spectrum[spfreq] += amp * profile[src] * rap;
            }
        }
        else {  //if the bandwidth is smaller than the profilesize
            float rap = sqrt((float)ibw / (float)profilesize);
            float ibasefreq = realfreq / (synth->samplerate_f * 0.5f) * size;
            for(int i = 0; i < profilesize; ++i) {
                float idfreq = i / (float)profilesize - 0.5f;
                idfreq *= ibw;
                int   spfreq  = (int) (idfreq + ibasefreq);
                float fspfreq = fmodf((float)idfreq + ibasefreq, 1.0f);
                if(spfreq <= 0)
                    continue;
                if(spfreq >= size - 1)
                    break;
                spectrum[spfreq] += amp * profile[i] * rap
                                    * (1.0f - fspfreq);
                spectrum[spfreq + 1] += amp * profile[i] * rap * fspfreq;
            }
        }
    }
}

/*
 * Generates the long spectrum for non-Bandwidth modes (only amplitudes are generated; phases will be random)
 */
void PADnoteParameters::generatespectrum_otherModes(float *spectrum,
                                                    int size,
                                                    float basefreq)
{
    for(int i = 0; i < size; ++i)
        spectrum[i] = 0.0f;

    float harmonics[synth->oscilsize / 2];
    for(int i = 0; i < synth->oscilsize / 2; ++i)
        harmonics[i] = 0.0f;
    //get the harmonic structure from the oscillator (I am using the frequency amplitudes, only)
    oscilgen->get(harmonics, basefreq, false);

    //normalize
    float max = 0.0f;
    for(int i = 0; i < synth->oscilsize / 2; ++i)
        if(harmonics[i] > max)
            max = harmonics[i];
    if(max < 0.000001f)
        max = 1;
    for(int i = 0; i < synth->oscilsize / 2; ++i)
        harmonics[i] /= max;

    for(int nh = 1; nh < synth->oscilsize / 2; ++nh) { //for each harmonic
        float realfreq = getNhr(nh) * basefreq;

        ///sa fac aici interpolarea si sa am grija daca frecv descresc

        if(realfreq > synth->samplerate_f * 0.49999f)
            break;
        if(realfreq < 20.0f)
            break;
//	if (harmonics[nh-1]<1e-4) continue;


        float amp = harmonics[nh - 1];
        if(resonance->Penabled)
            amp *= resonance->getfreqresponse(realfreq);
        int cfreq = (int) (realfreq / (synth->samplerate_f * 0.5f) * size);

        spectrum[cfreq] = amp + 1e-9;
    }

    if(Pmode != 1) {
        int old = 0;
        for(int k = 1; k < size; ++k)
            if((spectrum[k] > 1e-10) || (k == (size - 1))) {
                int   delta  = k - old;
                float val1   = spectrum[old];
                float val2   = spectrum[k];
                float idelta = 1.0f / delta;
                for(int i = 0; i < delta; ++i) {
                    float x = idelta * i;
                    spectrum[old + i] = val1 * (1.0f - x) + val2 * x;
                }
                old = k;
            }
    }
}

/*
 * Applies the parameters (i.e. computes all the samples, based on parameters);
 */
void PADnoteParameters::applyparameters(bool lockmutex)
{
    const int samplesize   = (((int) 1) << (Pquality.samplesize + 14));
    int       spectrumsize = samplesize / 2;
    float    *spectrum     = new float[spectrumsize];
    int       profilesize = 512;
    float     profile[profilesize];


    float bwadjust = getprofile(profile, profilesize);
//    for (int i=0;i<profilesize;i++) profile[i]*=profile[i];
    float basefreq = 65.406f * powf(2.0f, Pquality.basenote / 2);
    if(Pquality.basenote % 2 == 1)
        basefreq *= 1.5f;

    int samplemax = Pquality.oct + 1;
    int smpoct    = Pquality.smpoct;
    if(Pquality.smpoct == 5)
        smpoct = 6;
    if(Pquality.smpoct == 6)
        smpoct = 12;
    if(smpoct != 0)
        samplemax *= smpoct;
    else
        samplemax = samplemax / 2 + 1;
    if(samplemax == 0)
        samplemax = 1;

    //prepare a BIG FFT stuff
    FFTwrapper *fft      = new FFTwrapper(samplesize);
    fft_t      *fftfreqs = new fft_t[samplesize / 2];

    float adj[samplemax]; //this is used to compute frequency relation to the base frequency
    for(int nsample = 0; nsample < samplemax; ++nsample)
        adj[nsample] = (Pquality.oct + 1.0f) * (float)nsample / samplemax;
    for(int nsample = 0; nsample < samplemax; ++nsample) {
        float tmp = adj[nsample] - adj[samplemax - 1] * 0.5f;
        float basefreqadjust = powf(2.0f, tmp);

        if(Pmode == 0)
            generatespectrum_bandwidthMode(spectrum,
                                           spectrumsize,
                                           basefreq * basefreqadjust,
                                           profile,
                                           profilesize,
                                           bwadjust);
        else
            generatespectrum_otherModes(spectrum, spectrumsize,
                                        basefreq * basefreqadjust);

        const int extra_samples = 5; //the last samples contains the first samples (used for linear/cubic interpolation)
        newsample.smp = new float[samplesize + extra_samples];

        newsample.smp[0] = 0.0f;
        for(int i = 1; i < spectrumsize; ++i) //randomize the phases
            fftfreqs[i] = std::polar(spectrum[i], (float)RND * 6.29f);
        fft->freqs2smps(fftfreqs, newsample.smp); //that's all; here is the only ifft for the whole sample; no windows are used ;-)


        //normalize(rms)
        float rms = 0.0f;
        for(int i = 0; i < samplesize; ++i)
            rms += newsample.smp[i] * newsample.smp[i];
        rms = sqrt(rms);
        if(rms < 0.000001f)
            rms = 1.0f;
        rms *= sqrt(262144.0f / samplesize);
        for(int i = 0; i < samplesize; ++i)
            newsample.smp[i] *= 1.0f / rms * 50.0f;

        //prepare extra samples used by the linear or cubic interpolation
        for(int i = 0; i < extra_samples; ++i)
            newsample.smp[i + samplesize] = newsample.smp[i];

        //replace the current sample with the new computed sample
        if(lockmutex) {
            pthread_mutex_lock(mutex);
            deletesample(nsample);
            sample[nsample].smp      = newsample.smp;
            sample[nsample].size     = samplesize;
            sample[nsample].basefreq = basefreq * basefreqadjust;
            pthread_mutex_unlock(mutex);
        }
        else {
            deletesample(nsample);
            sample[nsample].smp      = newsample.smp;
            sample[nsample].size     = samplesize;
            sample[nsample].basefreq = basefreq * basefreqadjust;
        }
        newsample.smp = NULL;
    }
    delete (fft);
    delete[] fftfreqs;
    delete[] spectrum;

    //delete the additional samples that might exists and are not useful
    if(lockmutex) {
        pthread_mutex_lock(mutex);
        for(int i = samplemax; i < PAD_MAX_SAMPLES; ++i)
            deletesample(i);
        pthread_mutex_unlock(mutex);
    }
    else
        for(int i = samplemax; i < PAD_MAX_SAMPLES; ++i)
            deletesample(i);
    ;
}

void PADnoteParameters::export2wav(std::string basefilename)
{
    applyparameters(true);
    basefilename += "_PADsynth_";
    for(int k = 0; k < PAD_MAX_SAMPLES; ++k) {
        if(sample[k].smp == NULL)
            continue;
        char tmpstr[20];
        snprintf(tmpstr, 20, "_%02d", k + 1);
        std::string filename = basefilename + std::string(tmpstr) + ".wav";
        WavFile     wav(filename, synth->samplerate, 1);
        if(wav.good()) {
            int nsmps = sample[k].size;
            short int *smps = new short int[nsmps];
            for(int i = 0; i < nsmps; ++i)
                smps[i] = (short int)(sample[k].smp[i] * 32767.0f);
            wav.writeMonoSamples(nsmps, smps);
        }
    }
}

void PADnoteParameters::add2XML(XMLwrapper *xml)
{
    xml->setPadSynth(true);

    xml->addparbool("stereo", PStereo);
    xml->addpar("mode", Pmode);
    xml->addpar("bandwidth", Pbandwidth);
    xml->addpar("bandwidth_scale", Pbwscale);

    xml->beginbranch("HARMONIC_PROFILE");
    xml->addpar("base_type", Php.base.type);
    xml->addpar("base_par1", Php.base.par1);
    xml->addpar("frequency_multiplier", Php.freqmult);
    xml->addpar("modulator_par1", Php.modulator.par1);
    xml->addpar("modulator_frequency", Php.modulator.freq);
    xml->addpar("width", Php.width);
    xml->addpar("amplitude_multiplier_type", Php.amp.type);
    xml->addpar("amplitude_multiplier_mode", Php.amp.mode);
    xml->addpar("amplitude_multiplier_par1", Php.amp.par1);
    xml->addpar("amplitude_multiplier_par2", Php.amp.par2);
    xml->addparbool("autoscale", Php.autoscale);
    xml->addpar("one_half", Php.onehalf);
    xml->endbranch();

    xml->beginbranch("OSCIL");
    oscilgen->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("RESONANCE");
    resonance->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("HARMONIC_POSITION");
    xml->addpar("type", Phrpos.type);
    xml->addpar("parameter1", Phrpos.par1);
    xml->addpar("parameter2", Phrpos.par2);
    xml->addpar("parameter3", Phrpos.par3);
    xml->endbranch();

    xml->beginbranch("SAMPLE_QUALITY");
    xml->addpar("samplesize", Pquality.samplesize);
    xml->addpar("basenote", Pquality.basenote);
    xml->addpar("octaves", Pquality.oct);
    xml->addpar("samples_per_octave", Pquality.smpoct);
    xml->endbranch();

    xml->beginbranch("AMPLITUDE_PARAMETERS");
    xml->addpar("volume", PVolume);
    xml->addpar("panning", PPanning);
    xml->addpar("velocity_sensing", PAmpVelocityScaleFunction);
    xml->addpar("punch_strength", PPunchStrength);
    xml->addpar("punch_time", PPunchTime);
    xml->addpar("punch_stretch", PPunchStretch);
    xml->addpar("punch_velocity_sensing", PPunchVelocitySensing);

    xml->beginbranch("AMPLITUDE_ENVELOPE");
    AmpEnvelope->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("AMPLITUDE_LFO");
    AmpLfo->add2XML(xml);
    xml->endbranch();

    xml->endbranch();

    xml->beginbranch("FREQUENCY_PARAMETERS");
    xml->addpar("fixed_freq", Pfixedfreq);
    xml->addpar("fixed_freq_et", PfixedfreqET);
    xml->addpar("detune", PDetune);
    xml->addpar("coarse_detune", PCoarseDetune);
    xml->addpar("detune_type", PDetuneType);

    xml->beginbranch("FREQUENCY_ENVELOPE");
    FreqEnvelope->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("FREQUENCY_LFO");
    FreqLfo->add2XML(xml);
    xml->endbranch();
    xml->endbranch();

    xml->beginbranch("FILTER_PARAMETERS");
    xml->addpar("velocity_sensing_amplitude", PFilterVelocityScale);
    xml->addpar("velocity_sensing", PFilterVelocityScaleFunction);

    xml->beginbranch("FILTER");
    GlobalFilter->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("FILTER_ENVELOPE");
    FilterEnvelope->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("FILTER_LFO");
    FilterLfo->add2XML(xml);
    xml->endbranch();
    xml->endbranch();
}

void PADnoteParameters::getfromXML(XMLwrapper *xml)
{
    PStereo    = xml->getparbool("stereo", PStereo);
    Pmode      = xml->getpar127("mode", 0);
    Pbandwidth = xml->getpar("bandwidth", Pbandwidth, 0, 1000);
    Pbwscale   = xml->getpar127("bandwidth_scale", Pbwscale);

    if(xml->enterbranch("HARMONIC_PROFILE")) {
        Php.base.type = xml->getpar127("base_type", Php.base.type);
        Php.base.par1 = xml->getpar127("base_par1", Php.base.par1);
        Php.freqmult  = xml->getpar127("frequency_multiplier",
                                       Php.freqmult);
        Php.modulator.par1 = xml->getpar127("modulator_par1",
                                            Php.modulator.par1);
        Php.modulator.freq = xml->getpar127("modulator_frequency",
                                            Php.modulator.freq);
        Php.width    = xml->getpar127("width", Php.width);
        Php.amp.type = xml->getpar127("amplitude_multiplier_type",
                                      Php.amp.type);
        Php.amp.mode = xml->getpar127("amplitude_multiplier_mode",
                                      Php.amp.mode);
        Php.amp.par1 = xml->getpar127("amplitude_multiplier_par1",
                                      Php.amp.par1);
        Php.amp.par2 = xml->getpar127("amplitude_multiplier_par2",
                                      Php.amp.par2);
        Php.autoscale = xml->getparbool("autoscale", Php.autoscale);
        Php.onehalf   = xml->getpar127("one_half", Php.onehalf);
        xml->exitbranch();
    }

    if(xml->enterbranch("OSCIL")) {
        oscilgen->getfromXML(xml);
        xml->exitbranch();
    }

    if(xml->enterbranch("RESONANCE")) {
        resonance->getfromXML(xml);
        xml->exitbranch();
    }

    if(xml->enterbranch("HARMONIC_POSITION")) {
        Phrpos.type = xml->getpar127("type", Phrpos.type);
        Phrpos.par1 = xml->getpar("parameter1", Phrpos.par1, 0, 255);
        Phrpos.par2 = xml->getpar("parameter2", Phrpos.par2, 0, 255);
        Phrpos.par3 = xml->getpar("parameter3", Phrpos.par3, 0, 255);
        xml->exitbranch();
    }

    if(xml->enterbranch("SAMPLE_QUALITY")) {
        Pquality.samplesize = xml->getpar127("samplesize", Pquality.samplesize);
        Pquality.basenote   = xml->getpar127("basenote", Pquality.basenote);
        Pquality.oct    = xml->getpar127("octaves", Pquality.oct);
        Pquality.smpoct = xml->getpar127("samples_per_octave",
                                         Pquality.smpoct);
        xml->exitbranch();
    }

    if(xml->enterbranch("AMPLITUDE_PARAMETERS")) {
        PVolume  = xml->getpar127("volume", PVolume);
        PPanning = xml->getpar127("panning", PPanning);
        PAmpVelocityScaleFunction = xml->getpar127("velocity_sensing",
                                                   PAmpVelocityScaleFunction);
        PPunchStrength = xml->getpar127("punch_strength", PPunchStrength);
        PPunchTime     = xml->getpar127("punch_time", PPunchTime);
        PPunchStretch  = xml->getpar127("punch_stretch", PPunchStretch);
        PPunchVelocitySensing = xml->getpar127("punch_velocity_sensing",
                                               PPunchVelocitySensing);

        xml->enterbranch("AMPLITUDE_ENVELOPE");
        AmpEnvelope->getfromXML(xml);
        xml->exitbranch();

        xml->enterbranch("AMPLITUDE_LFO");
        AmpLfo->getfromXML(xml);
        xml->exitbranch();

        xml->exitbranch();
    }

    if(xml->enterbranch("FREQUENCY_PARAMETERS")) {
        Pfixedfreq    = xml->getpar127("fixed_freq", Pfixedfreq);
        PfixedfreqET  = xml->getpar127("fixed_freq_et", PfixedfreqET);
        PDetune       = xml->getpar("detune", PDetune, 0, 16383);
        PCoarseDetune = xml->getpar("coarse_detune", PCoarseDetune, 0, 16383);
        PDetuneType   = xml->getpar127("detune_type", PDetuneType);

        xml->enterbranch("FREQUENCY_ENVELOPE");
        FreqEnvelope->getfromXML(xml);
        xml->exitbranch();

        xml->enterbranch("FREQUENCY_LFO");
        FreqLfo->getfromXML(xml);
        xml->exitbranch();
        xml->exitbranch();
    }

    if(xml->enterbranch("FILTER_PARAMETERS")) {
        PFilterVelocityScale = xml->getpar127("velocity_sensing_amplitude",
                                              PFilterVelocityScale);
        PFilterVelocityScaleFunction = xml->getpar127(
            "velocity_sensing",
            PFilterVelocityScaleFunction);

        xml->enterbranch("FILTER");
        GlobalFilter->getfromXML(xml);
        xml->exitbranch();

        xml->enterbranch("FILTER_ENVELOPE");
        FilterEnvelope->getfromXML(xml);
        xml->exitbranch();

        xml->enterbranch("FILTER_LFO");
        FilterLfo->getfromXML(xml);
        xml->exitbranch();
        xml->exitbranch();
    }
}
