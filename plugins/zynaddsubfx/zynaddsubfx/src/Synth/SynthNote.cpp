#include "SynthNote.h"
#include "../globals.h"
#include <cstring>

SynthNote::SynthNote(float freq, float vel, int port, int note, bool quiet)
    :legato(freq, vel, port, note, quiet)
{}

SynthNote::Legato::Legato(float freq, float vel, int port,
                          int note, bool quiet)
{
    // Initialise some legato-specific vars
    msg = LM_Norm;
    fade.length = (int)(synth->samplerate_f * 0.005f);      // 0.005f seems ok.
    if(fade.length < 1)
        fade.length = 1;                    // (if something's fishy)
    fade.step  = (1.0f / fade.length);
    decounter  = -10;
    param.freq = freq;
    param.vel  = vel;
    param.portamento = port;
    param.midinote   = note;
    lastfreq = 0.0f;
    silent   = quiet;
}

int SynthNote::Legato::update(float freq, float velocity, int portamento_,
                              int midinote_, bool externcall)
{
    if(externcall)
        msg = LM_Norm;
    if(msg != LM_CatchUp) {
        lastfreq   = param.freq;
        param.freq = freq;
        param.vel  = velocity;
        param.portamento = portamento_;
        param.midinote   = midinote_;
        if(msg == LM_Norm) {
            if(silent) {
                fade.m = 0.0f;
                msg    = LM_FadeIn;
            }
            else {
                fade.m = 1.0f;
                msg    = LM_FadeOut;
                return 1;
            }
        }
        if(msg == LM_ToNorm)
            msg = LM_Norm;
    }
    return 0;
}

void SynthNote::Legato::apply(SynthNote &note, float *outl, float *outr)
{
    if(silent) // Silencer
        if(msg != LM_FadeIn) {
            memset(outl, 0, synth->bufferbytes);
            memset(outr, 0, synth->bufferbytes);
        }
    switch(msg) {
        case LM_CatchUp: // Continue the catch-up...
            if(decounter == -10)
                decounter = fade.length;
            //Yea, could be done without the loop...
            for(int i = 0; i < synth->buffersize; ++i) {
                decounter--;
                if(decounter < 1) {
                    // Catching-up done, we can finally set
                    // the note to the actual parameters.
                    decounter = -10;
                    msg = LM_ToNorm;
                    note.legatonote(param.freq, param.vel, param.portamento,
                                    param.midinote, false);
                    break;
                }
            }
            break;
        case LM_FadeIn: // Fade-in
            if(decounter == -10)
                decounter = fade.length;
            silent = false;
            for(int i = 0; i < synth->buffersize; ++i) {
                decounter--;
                if(decounter < 1) {
                    decounter = -10;
                    msg = LM_Norm;
                    break;
                }
                fade.m  += fade.step;
                outl[i] *= fade.m;
                outr[i] *= fade.m;
            }
            break;
        case LM_FadeOut: // Fade-out, then set the catch-up
            if(decounter == -10)
                decounter = fade.length;
            for(int i = 0; i < synth->buffersize; ++i) {
                decounter--;
                if(decounter < 1) {
                    for(int j = i; j < synth->buffersize; ++j) {
                        outl[j] = 0.0f;
                        outr[j] = 0.0f;
                    }
                    decounter = -10;
                    silent    = true;
                    // Fading-out done, now set the catch-up :
                    decounter = fade.length;
                    msg = LM_CatchUp;
                    //This freq should make this now silent note to catch-up/resync
                    //with the heard note for the same length it stayed at the
                    //previous freq during the fadeout.
                    float catchupfreq = param.freq * (param.freq / lastfreq);
                    note.legatonote(catchupfreq, param.vel, param.portamento,
                                    param.midinote, false);
                    break;
                }
                fade.m  -= fade.step;
                outl[i] *= fade.m;
                outr[i] *= fade.m;
            }
            break;
        default:
            break;
    }
}

void SynthNote::setVelocity(float velocity_) {
    legato.setSilent(true); //Let legato.update(...) returns 0.
    legatonote(legato.getFreq(), velocity_,
               legato.getPortamento(), legato.getMidinote(), true);
    legato.setDecounter(0); //avoid chopping sound due fade-in
}
