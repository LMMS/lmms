/*
  ZynAddSubFX - a software synthesizer

  Controller.cpp - (Midi) Controllers implementation
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

#include "Controller.h"
#include <math.h>
#include <stdio.h>

Controller::Controller()
{
    defaults();
    resetall();
}

Controller::~Controller()
{}

void Controller::defaults()
{
    setpitchwheelbendrange(100); //1 halftones
    expression.receive    = 1;
    panning.depth         = 64;
    filtercutoff.depth    = 64;
    filterq.depth         = 64;
    bandwidth.depth       = 64;
    bandwidth.exponential = 0;
    modwheel.depth        = 80;
    modwheel.exponential  = 0;
    fmamp.receive         = 1;
    volume.receive        = 1;
    sustain.receive       = 1;
    NRPN.receive = 1;

    portamento.portamento = 0;
    portamento.used = 0;
    portamento.proportional = 0;
    portamento.propRate     = 80;
    portamento.propDepth    = 90;
    portamento.receive      = 1;
    portamento.time = 64;
    portamento.updowntimestretch = 64;
    portamento.pitchthresh     = 3;
    portamento.pitchthreshtype = 1;
    portamento.noteusing     = -1;
    resonancecenter.depth    = 64;
    resonancebandwidth.depth = 64;

    initportamento(440.0f, 440.0f, false); // Now has a third argument
    setportamento(0);
}

void Controller::resetall()
{
    setpitchwheel(0); //center
    setexpression(127);
    setpanning(64);
    setfiltercutoff(64);
    setfilterq(64);
    setbandwidth(64);
    setmodwheel(64);
    setfmamp(127);
    setvolume(127);
    setsustain(0);
    setresonancecenter(64);
    setresonancebw(64);

    //reset the NRPN
    NRPN.parhi = -1;
    NRPN.parlo = -1;
    NRPN.valhi = -1;
    NRPN.vallo = -1;
}

void Controller::setpitchwheel(int value)
{
    pitchwheel.data = value;
    float cents = value / 8192.0f;
    cents *= pitchwheel.bendrange;
    pitchwheel.relfreq = powf(2, cents / 1200.0f);
    //fprintf(stderr,"%ld %ld -> %.3f\n",pitchwheel.bendrange,pitchwheel.data,pitchwheel.relfreq);fflush(stderr);
}

void Controller::setpitchwheelbendrange(unsigned short int value)
{
    pitchwheel.bendrange = value;
}

void Controller::setexpression(int value)
{
    expression.data = value;
    if(expression.receive != 0)
        expression.relvolume = value / 127.0f;
    else
        expression.relvolume = 1.0f;
}

void Controller::setpanning(int value)
{
    panning.data = value;
    panning.pan  = (value / 128.0f - 0.5f) * (panning.depth / 64.0f);
}

void Controller::setfiltercutoff(int value)
{
    filtercutoff.data    = value;
    filtercutoff.relfreq =
        (value - 64.0f) * filtercutoff.depth / 4096.0f * 3.321928f;         //3.3219f..=ln2(10)
}

void Controller::setfilterq(int value)
{
    filterq.data = value;
    filterq.relq = powf(30.0f, (value - 64.0f) / 64.0f * (filterq.depth / 64.0f));
}

void Controller::setbandwidth(int value)
{
    bandwidth.data = value;
    if(bandwidth.exponential == 0) {
        float tmp = powf(25.0f, powf(bandwidth.depth / 127.0f, 1.5f)) - 1.0f;
        if((value < 64) && (bandwidth.depth >= 64))
            tmp = 1.0f;
        bandwidth.relbw = (value / 64.0f - 1.0f) * tmp + 1.0f;
        if(bandwidth.relbw < 0.01f)
            bandwidth.relbw = 0.01f;
    }
    else
        bandwidth.relbw =
            powf(25.0f, (value - 64.0f) / 64.0f * (bandwidth.depth / 64.0f));
    ;
}

void Controller::setmodwheel(int value)
{
    modwheel.data = value;
    if(modwheel.exponential == 0) {
        float tmp =
            powf(25.0f, powf(modwheel.depth / 127.0f, 1.5f) * 2.0f) / 25.0f;
        if((value < 64) && (modwheel.depth >= 64))
            tmp = 1.0f;
        modwheel.relmod = (value / 64.0f - 1.0f) * tmp + 1.0f;
        if(modwheel.relmod < 0.0f)
            modwheel.relmod = 0.0f;
    }
    else
        modwheel.relmod =
            powf(25.0f, (value - 64.0f) / 64.0f * (modwheel.depth / 80.0f));
}

void Controller::setfmamp(int value)
{
    fmamp.data   = value;
    fmamp.relamp = value / 127.0f;
    if(fmamp.receive != 0)
        fmamp.relamp = value / 127.0f;
    else
        fmamp.relamp = 1.0f;
}

void Controller::setvolume(int value)
{
    volume.data = value;
    if(volume.receive != 0)
        volume.volume = powf(0.1f, (127 - value) / 127.0f * 2.0f);
    else
        volume.volume = 1.0f;
}

void Controller::setsustain(int value)
{
    sustain.data = value;
    if(sustain.receive != 0)
        sustain.sustain = ((value < 64) ? 0 : 1);
    else
        sustain.sustain = 0;
}

void Controller::setportamento(int value)
{
    portamento.data = value;
    if(portamento.receive != 0)
        portamento.portamento = ((value < 64) ? 0 : 1);
}

int Controller::initportamento(float oldfreq,
                               float newfreq,
                               bool legatoflag)
{
    portamento.x = 0.0f;

    if(legatoflag) {  // Legato in progress
        if(portamento.portamento == 0)
            return 0;
    }
    else     // No legato, do the original if...return
    if((portamento.used != 0) || (portamento.portamento == 0))
        return 0;
    ;

    float portamentotime = powf(100.0f, portamento.time / 127.0f) / 50.0f; //portamento time in seconds

    if(portamento.proportional) {
        //If there is a min(float,float) and a max(float,float) then they
        //could be used here
        //Linear functors could also make this nicer
        if(oldfreq > newfreq) //2 is the center of propRate
            portamentotime *=
                powf(oldfreq / newfreq
                     / (portamento.propRate / 127.0f * 3 + .05),
                     (portamento.propDepth / 127.0f * 1.6f + .2));
        else                  //1 is the center of propDepth
            portamentotime *=
                powf(newfreq / oldfreq
                     / (portamento.propRate / 127.0f * 3 + .05),
                     (portamento.propDepth / 127.0f * 1.6f + .2));
    }

    if((portamento.updowntimestretch >= 64) && (newfreq < oldfreq)) {
        if(portamento.updowntimestretch == 127)
            return 0;
        portamentotime *= powf(0.1f,
                               (portamento.updowntimestretch - 64) / 63.0f);
    }
    if((portamento.updowntimestretch < 64) && (newfreq > oldfreq)) {
        if(portamento.updowntimestretch == 0)
            return 0;
        portamentotime *= powf(0.1f,
                               (64.0f - portamento.updowntimestretch) / 64.0f);
    }

    //printf("%f->%f : Time %f\n",oldfreq,newfreq,portamentotime);

    portamento.dx = synth->buffersize_f / (portamentotime * synth->samplerate_f);
    portamento.origfreqrap = oldfreq / newfreq;

    float tmprap = ((portamento.origfreqrap > 1.0f) ?
                    (portamento.origfreqrap) :
                    (1.0f / portamento.origfreqrap));

    float thresholdrap = powf(2.0f, portamento.pitchthresh / 12.0f);
    if((portamento.pitchthreshtype == 0) && (tmprap - 0.00001f > thresholdrap))
        return 0;
    if((portamento.pitchthreshtype == 1) && (tmprap + 0.00001f < thresholdrap))
        return 0;

    portamento.used    = 1;
    portamento.freqrap = portamento.origfreqrap;
    return 1;
}

void Controller::updateportamento()
{
    if(portamento.used == 0)
        return;

    portamento.x += portamento.dx;
    if(portamento.x > 1.0f) {
        portamento.x    = 1.0f;
        portamento.used = 0;
    }
    portamento.freqrap =
        (1.0f - portamento.x) * portamento.origfreqrap + portamento.x;
}


void Controller::setresonancecenter(int value)
{
    resonancecenter.data      = value;
    resonancecenter.relcenter =
        powf(3.0f, (value - 64.0f) / 64.0f * (resonancecenter.depth / 64.0f));
}
void Controller::setresonancebw(int value)
{
    resonancebandwidth.data  = value;
    resonancebandwidth.relbw =
        powf(1.5f, (value - 64.0f) / 64.0f * (resonancebandwidth.depth / 127.0f));
}


//Returns 0 if there is NRPN or 1 if there is not
int Controller::getnrpn(int *parhi, int *parlo, int *valhi, int *vallo)
{
    if(NRPN.receive == 0)
        return 1;
    if((NRPN.parhi < 0) || (NRPN.parlo < 0) || (NRPN.valhi < 0)
       || (NRPN.vallo < 0))
        return 1;

    *parhi = NRPN.parhi;
    *parlo = NRPN.parlo;
    *valhi = NRPN.valhi;
    *vallo = NRPN.vallo;
    return 0;
}


void Controller::setparameternumber(unsigned int type, int value)
{
    switch(type) {
        case C_nrpnhi:
            NRPN.parhi = value;
            NRPN.valhi = -1;
            NRPN.vallo = -1; //clear the values
            break;
        case C_nrpnlo:
            NRPN.parlo = value;
            NRPN.valhi = -1;
            NRPN.vallo = -1; //clear the values
            break;
        case C_dataentryhi:
            if((NRPN.parhi >= 0) && (NRPN.parlo >= 0))
                NRPN.valhi = value;
            break;
        case C_dataentrylo:
            if((NRPN.parhi >= 0) && (NRPN.parlo >= 0))
                NRPN.vallo = value;
            break;
    }
}



void Controller::add2XML(XMLwrapper *xml)
{
    xml->addpar("pitchwheel_bendrange", pitchwheel.bendrange);

    xml->addparbool("expression_receive", expression.receive);
    xml->addpar("panning_depth", panning.depth);
    xml->addpar("filter_cutoff_depth", filtercutoff.depth);
    xml->addpar("filter_q_depth", filterq.depth);
    xml->addpar("bandwidth_depth", bandwidth.depth);
    xml->addpar("mod_wheel_depth", modwheel.depth);
    xml->addparbool("mod_wheel_exponential", modwheel.exponential);
    xml->addparbool("fm_amp_receive", fmamp.receive);
    xml->addparbool("volume_receive", volume.receive);
    xml->addparbool("sustain_receive", sustain.receive);

    xml->addparbool("portamento_receive", portamento.receive);
    xml->addpar("portamento_time", portamento.time);
    xml->addpar("portamento_pitchthresh", portamento.pitchthresh);
    xml->addpar("portamento_pitchthreshtype", portamento.pitchthreshtype);
    xml->addpar("portamento_portamento", portamento.portamento);
    xml->addpar("portamento_updowntimestretch", portamento.updowntimestretch);
    xml->addpar("portamento_proportional", portamento.proportional);
    xml->addpar("portamento_proprate", portamento.propRate);
    xml->addpar("portamento_propdepth", portamento.propDepth);

    xml->addpar("resonance_center_depth", resonancecenter.depth);
    xml->addpar("resonance_bandwidth_depth", resonancebandwidth.depth);
}

void Controller::getfromXML(XMLwrapper *xml)
{
    pitchwheel.bendrange = xml->getpar("pitchwheel_bendrange",
                                       pitchwheel.bendrange,
                                       -6400,
                                       6400);

    expression.receive = xml->getparbool("expression_receive",
                                         expression.receive);
    panning.depth      = xml->getpar127("panning_depth", panning.depth);
    filtercutoff.depth = xml->getpar127("filter_cutoff_depth",
                                        filtercutoff.depth);
    filterq.depth        = xml->getpar127("filter_q_depth", filterq.depth);
    bandwidth.depth      = xml->getpar127("bandwidth_depth", bandwidth.depth);
    modwheel.depth       = xml->getpar127("mod_wheel_depth", modwheel.depth);
    modwheel.exponential = xml->getparbool("mod_wheel_exponential",
                                           modwheel.exponential);
    fmamp.receive = xml->getparbool("fm_amp_receive",
                                    fmamp.receive);
    volume.receive = xml->getparbool("volume_receive",
                                     volume.receive);
    sustain.receive = xml->getparbool("sustain_receive",
                                      sustain.receive);

    portamento.receive = xml->getparbool("portamento_receive",
                                         portamento.receive);
    portamento.time = xml->getpar127("portamento_time",
                                     portamento.time);
    portamento.pitchthresh = xml->getpar127("portamento_pitchthresh",
                                            portamento.pitchthresh);
    portamento.pitchthreshtype = xml->getpar127("portamento_pitchthreshtype",
                                                portamento.pitchthreshtype);
    portamento.portamento = xml->getpar127("portamento_portamento",
                                           portamento.portamento);
    portamento.updowntimestretch = xml->getpar127(
        "portamento_updowntimestretch",
        portamento.updowntimestretch);
    portamento.proportional = xml->getpar127("portamento_proportional",
                                             portamento.proportional);
    portamento.propRate = xml->getpar127("portamento_proprate",
                                         portamento.propRate);
    portamento.propDepth = xml->getpar127("portamento_propdepth",
                                          portamento.propDepth);


    resonancecenter.depth = xml->getpar127("resonance_center_depth",
                                           resonancecenter.depth);
    resonancebandwidth.depth = xml->getpar127("resonance_bandwidth_depth",
                                              resonancebandwidth.depth);
}
