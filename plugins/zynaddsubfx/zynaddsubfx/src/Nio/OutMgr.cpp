#include "OutMgr.h"
#include <algorithm>
#include <iostream>
#include <cassert>
#include "AudioOut.h"
#include "Engine.h"
#include "EngineMgr.h"
#include "InMgr.h"
#include "WavEngine.h"
#include "../Misc/Master.h"
#include "../Misc/Util.h" //for set_realtime()

using namespace std;

OutMgr &OutMgr::getInstance()
{
    static OutMgr instance;
    return instance;
}

OutMgr::OutMgr()
    :wave(new WavEngine()),
      priBuf(new float[4096],
             new float[4096]), priBuffCurrent(priBuf),
      master(Master::getInstance())
{
    currentOut = NULL;
    stales     = 0;
    master     = Master::getInstance();

    //init samples
    outr = new float[synth->buffersize];
    outl = new float[synth->buffersize];
    memset(outl, 0, synth->bufferbytes);
    memset(outr, 0, synth->bufferbytes);
}

OutMgr::~OutMgr()
{
    delete wave;
    delete [] priBuf.l;
    delete [] priBuf.r;
    delete [] outr;
    delete [] outl;
}

/* Sequence of a tick
 * 1) Lets remove old/stale samples
 * 2) Apply appliciable midi events
 * 3) Lets see if we need to generate samples
 * 4) Lets generate some
 * 5) Goto 2 if more are needed
 * 6) Lets return those samples to the primary and secondary outputs
 * 7) Lets wait for another tick
 */
const Stereo<float *> OutMgr::tick(unsigned int frameSize)
{
    InMgr &midi = InMgr::getInstance();
    //SysEv->execute();
    removeStaleSmps();
    int i=0;
    while(frameSize > storedSmps()) {
        if(!midi.empty()) {
            pthread_mutex_lock(&(master.mutex));
            midi.flush(i*synth->buffersize, (i+1)*synth->buffersize);
            pthread_mutex_unlock(&(master.mutex));
        }
        pthread_mutex_lock(&(master.mutex));
        master.AudioOut(outl, outr);
        pthread_mutex_unlock(&(master.mutex));
        addSmps(outl, outr);
        i++;
    }
    stales = frameSize;
    return priBuf;
}

AudioOut *OutMgr::getOut(string name)
{
    return dynamic_cast<AudioOut *>(EngineMgr::getInstance().getEng(name));
}

string OutMgr::getDriver() const
{
    return currentOut->name;
}

bool OutMgr::setSink(string name)
{
    AudioOut *sink = getOut(name);

    if(!sink)
        return false;

    if(currentOut)
        currentOut->setAudioEn(false);

    currentOut = sink;
    currentOut->setAudioEn(true);

    bool success = currentOut->getAudioEn();

    //Keep system in a valid state (aka with a running driver)
    if(!success)
        (currentOut = getOut("NULL"))->setAudioEn(true);

    return success;
}

string OutMgr::getSink() const
{
    if(currentOut)
        return currentOut->name;
    else {
        cerr << "BUG: No current output in OutMgr " << __LINE__ << endl;
        return "ERROR";
    }
    return "ERROR";
}

//perform a cheap linear interpolation for resampling
//This will result in some distortion at frame boundries
//returns number of samples produced
static size_t resample(float *dest,
                       const float *src,
                       float s_in,
                       float s_out,
                       size_t elms)
{
    size_t out_elms = elms * s_out / s_in;
    float  r_pos    = 0.0f;
    for(int i = 0; i < (int)out_elms; ++i, r_pos += s_in / s_out)
        dest[i] = interpolate(src, elms, r_pos);

    return out_elms;
}

void OutMgr::addSmps(float *l, float *r)
{
    //allow wave file to syphon off stream
    wave->push(Stereo<float *>(l, r), synth->buffersize);

    const int s_out = currentOut->getSampleRate(),
              s_sys = synth->samplerate;

    if(s_out != s_sys) { //we need to resample
        const size_t steps = resample(priBuffCurrent.l,
                                      l,
                                      s_sys,
                                      s_out,
                                      synth->buffersize);
        resample(priBuffCurrent.r, r, s_sys, s_out, synth->buffersize);

        priBuffCurrent.l += steps;
        priBuffCurrent.r += steps;
    }
    else { //just copy the samples
        memcpy(priBuffCurrent.l, l, synth->bufferbytes);
        memcpy(priBuffCurrent.r, r, synth->bufferbytes);
        priBuffCurrent.l += synth->buffersize;
        priBuffCurrent.r += synth->buffersize;
    }
}

void OutMgr::removeStaleSmps()
{
    if(!stales)
        return;

    const int leftover = storedSmps() - stales;

    assert(leftover > -1);

    //leftover samples [seen at very low latencies]
    if(leftover) {
        memmove(priBuf.l, priBuffCurrent.l - leftover, leftover * sizeof(float));
        memmove(priBuf.r, priBuffCurrent.r - leftover, leftover * sizeof(float));
        priBuffCurrent.l = priBuf.l + leftover;
        priBuffCurrent.r = priBuf.r + leftover;
    }
    else
        priBuffCurrent = priBuf;

    stales = 0;
}
