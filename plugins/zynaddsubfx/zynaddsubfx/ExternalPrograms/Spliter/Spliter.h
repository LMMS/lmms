//Copyright (c) 2002-2003 Nasca Octavian Paul
//License: GNU GPL 2

#ifndef SPLITER_H
#define SPLITER_H
#include <pthread.h>
#include <alsa/asoundlib.h>

extern pthread_mutex_t mutex;
extern int Pexitprogram;

class Spliter
{
    public:
        Spliter();
        ~Spliter();
        void midievents();

        //parameters
        unsigned char Psplitpoint;
        unsigned char Pchin, Pchout1, Pchout2;
        signed char   Poct1, Poct2;
    private:
        snd_seq_t *midi_in, *midi_out;
};

#endif
