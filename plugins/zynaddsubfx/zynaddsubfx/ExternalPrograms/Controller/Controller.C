#include "Controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

pthread_mutex_t mutex;
int Pexitprogram;

Controller::Controller() {
    //init
    for(int i = 0; i < 6; ++i) {
        pars[i].mode      = 1;
        pars[i].val1      = 0;
        pars[i].val2      = 127;
        pars[i].nrpn.cpar = 8;
        pars[i].nrpn.fpar = 0;
        pars[i].nrpn.cval = 0;
    }
    pars[0].ctl.par = 71;
    pars[1].ctl.par = 74;
    pars[2].ctl.par = 10;
    pars[3].ctl.par = 11;
    pars[4].ctl.par = 1;
    pars[5].ctl.par = 75;

    //ALSA init
    snd_seq_open(&midi_out, "default", SND_SEQ_OPEN_OUTPUT, 0);

    char portname[50]; sprintf(portname, "Controller");
    int  alsaport = snd_seq_create_simple_port(
        midi_out,
        portname,
        SND_SEQ_PORT_CAP_READ
        | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_SYNTH);
}

Controller::~Controller() {
    snd_seq_close(midi_out);
}

void Controller::sendcontroller(int par, unsigned char val) {
    snd_seq_event_t midievent;
    snd_seq_ev_clear(&midievent);

    snd_seq_ev_set_controller(&midievent, Pchout, par, val);

    snd_seq_ev_set_subs(&midievent);
    snd_seq_ev_set_direct(&midievent);
    snd_seq_event_output_direct(midi_out, &midievent);

//    fprintf(stderr,"Controller: %d %d\n",par,val);
}

void Controller::sendnrpn(int npar, unsigned char val) {
//    fprintf(stderr,"NRPN: %d %d %d %d\n",pars[npar].nrpn.cpar,pars[npar].nrpn.fpar,pars[npar].nrpn.cval,val);

    sendcontroller(0x63, pars[npar].nrpn.cpar);
    sendcontroller(0x62, pars[npar].nrpn.fpar);
    sendcontroller(0x06, pars[npar].nrpn.cval);
    sendcontroller(0x26, val);
//    fprintf(stderr,"------------\n\n");
}

void Controller::send(int npar, float xval) {
    if(pars[npar].mode == 0)
        return;
    int val;
    if(pars[npar].val1 <= pars[npar].val2)
        val =
            (int) (xval
                   * (pars[npar].val2 - pars[npar].val1
                      + 1.0) * 0.9999 + pars[npar].val1);
    else
        val =
            (int) (xval
                   * (pars[npar].val2 - pars[npar].val1
                      - 1.0) * 0.9999 + pars[npar].val1 + 1.0);
    switch(pars[npar].mode) {
        case 1: sendcontroller(pars[npar].ctl.par, val); break;
        //case 2:break;
        case 3: sendnrpn(npar, val); break;
    }
}
