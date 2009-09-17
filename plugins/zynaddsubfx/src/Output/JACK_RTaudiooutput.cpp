/*
  ZynAddSubFX - a software synthesizer

  JACKaudiooutput.C - Audio output for JACK
  Copyright (C) 2002 Nasca Octavian Paul
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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


extern "C"
{
#include <jack/ringbuffer.h>
};
#include "JACKaudiooutput.h"

Master *jackmaster;
jack_client_t *jackclient;
jack_port_t *outport_left,*outport_right;
jack_ringbuffer_t *rb=NULL;

REALTYPE *jackoutl,*jackoutr;
int jackfinish=0;

void *thread_blocked(void *arg);
int jackprocess(jack_nframes_t nframes,void *arg);
int jacksrate(jack_nframes_t nframes,void *arg);
void jackshutdown(void *arg);

pthread_cond_t more_data=PTHREAD_COND_INITIALIZER;
pthread_mutex_t zyn_thread_lock=PTHREAD_MUTEX_INITIALIZER;

pthread_t bthr;


bool JACKaudiooutputinit(Master *master_)
{
    jackmaster=master_;
    jackclient=0;
    char tmpstr[100];

    jackoutl=new REALTYPE [SOUND_BUFFER_SIZE];
    jackoutr=new REALTYPE [SOUND_BUFFER_SIZE];

    int rbbufsize=SOUND_BUFFER_SIZE*sizeof (REALTYPE)*2*2;
    printf("%d\n",rbbufsize);
    rb=jack_ringbuffer_create(rbbufsize);
    for (int i=0;i<rbbufsize;i++) rb->buf[i]=0.0;


    for (int i=0;i<15;i++) {
        if (i!=0) snprintf(tmpstr,100,"ZynAddSubFX_%d",i);
        else snprintf(tmpstr,100,"ZynAddSubFX");
        jackclient=jack_client_new(tmpstr);
        if (jackclient!=0) break;
    };

    if (jackclient==0) {
        fprintf(stderr,"\nERROR: Cannot make a jack client (possible reasons: JACK server is not running or jackd is launched by root and zynaddsubfx by another user.).\n\n\n");
        return(false);
    };

    fprintf(stderr,"Internal SampleRate   = %d\nJack Output SampleRate= %d\n",SAMPLE_RATE,jack_get_sample_rate(jackclient));
    if ((unsigned int)jack_get_sample_rate(jackclient)!=(unsigned int) SAMPLE_RATE)
        fprintf(stderr,"It is recomanded that the both samplerates to be equal.\n");

    jack_set_process_callback(jackclient,jackprocess,0);
    jack_set_sample_rate_callback(jackclient,jacksrate,0);
    jack_on_shutdown(jackclient,jackshutdown,0);

    outport_left=jack_port_register(jackclient,"out_1",
                                    JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput|JackPortIsTerminal,0);
    outport_right=jack_port_register(jackclient,"out_2",
                                     JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput|JackPortIsTerminal,0);

    if (jack_activate(jackclient)) {
        fprintf(stderr,"Cannot activate jack client\n");
        return(false);
    };

    pthread_create(&bthr,NULL,thread_blocked,NULL);

    /*
    jack_connect(jackclient,jack_port_name(outport_left),"alsa_pcm:out_1");
    jack_connect(jackclient,jack_port_name(outport_right),"alsa_pcm:out_2");
     */

    return(true);
};

void *thread_blocked(void *arg)
{
    int datasize=SOUND_BUFFER_SIZE*sizeof (REALTYPE);

    //try to get realtime
    sched_param sc;
    sc.sched_priority=50;
    int err=sched_setscheduler(0,SCHED_FIFO,&sc);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    pthread_mutex_lock(&zyn_thread_lock);

    while (jackfinish==0) {
        while (jack_ringbuffer_write_space(rb)>=datasize) {
            pthread_mutex_lock(&jackmaster->mutex);
            jackmaster->GetAudioOutSamples(SOUND_BUFFER_SIZE,jack_get_sample_rate(jackclient),jackoutl,jackoutr);
            pthread_mutex_unlock(&jackmaster->mutex);

            jack_ringbuffer_write(rb, (char *) jackoutl,datasize);
            jack_ringbuffer_write(rb, (char *) jackoutr,datasize);
        };
        pthread_cond_wait(&more_data,&zyn_thread_lock);
    };
    pthread_mutex_unlock(&zyn_thread_lock);

    return(0);
};


int jackprocess(jack_nframes_t nframes,void *arg)
{
    jack_default_audio_sample_t *outl=(jack_default_audio_sample_t *) jack_port_get_buffer (outport_left, nframes);
    jack_default_audio_sample_t *outr=(jack_default_audio_sample_t *) jack_port_get_buffer (outport_right, nframes);

    int datasize=nframes*sizeof (REALTYPE);
    int incoming_datasize=SOUND_BUFFER_SIZE*sizeof (REALTYPE);
    int data_read=0;


    if (jack_ringbuffer_read_space(rb)>=(2*incoming_datasize)) {
        if (datasize>incoming_datasize) {
            data_read=0;
            while (data_read < datasize) {
                jack_ringbuffer_read(rb, (char *) outl+data_read,datasize);
                jack_ringbuffer_read(rb, (char *) outr+data_read,datasize);
                data_read+=incoming_datasize;
            };
        } else if (datasize==incoming_datasize) {
            jack_ringbuffer_read(rb, (char *) outl,datasize);
            jack_ringbuffer_read(rb, (char *) outr,datasize);
        } else {
        };
    } else {//the ringbuffer is empty or there are too small amount of samples in it
        for (int i=0;i<nframes;i++) {
            outl[i]=0.0;
            outr[i]=0.0;
        };
    };
    /*    if (jack_ringbuffer_read_space(rb)>=datasize){
    	jack_ringbuffer_read(rb, (char *) outl,datasize);
    	jack_ringbuffer_read(rb, (char *) outr,datasize);
        } else {//the ringbuffer is empty or there are too small amount of samples in it
    	for (int i=0;i<nframes;i++){
    	    outl[i]=0.0;outr[i]=0.0;
    	};
        };
    */
    if (pthread_mutex_trylock(&zyn_thread_lock)==0) {
        pthread_cond_signal(&more_data);
        pthread_mutex_unlock(&zyn_thread_lock);
    };

    return(0);
};

void JACKfinish()
{
    jackfinish=1;
    jack_ringbuffer_free(rb);
    jack_client_close(jackclient);

    usleep(100000);
    delete(jackoutl);
    delete(jackoutr);
};

int jacksrate(jack_nframes_t nframes,void *arg)
{

    return(0);
};

void jackshutdown(void *arg)
{
};



