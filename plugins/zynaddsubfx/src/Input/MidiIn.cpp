/*
  ZynAddSubFX - a software synthesizer

  MidiIn.cpp - This class is inherited by all the Midi input classes
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

#include "../globals.h"
#include "MidiIn.h"

int MidiIn::getcontroller(unsigned char b)
{
    /**\todo there might be a better way to do this*/
    int ctl = C_NULL;
    switch(b) {
    case 1:
        ctl = C_modwheel; //Modulation Wheel
        break;
    case 7:
        ctl = C_volume; //Volume
        break;
    case 10:
        ctl = C_panning; //Panning
        break;
    case 11:
        ctl = C_expression; //Expression
        break;
    case 64:
        ctl = C_sustain; //Sustain pedal
        break;
    case 65:
        ctl = C_portamento; //Portamento
        break;
    case 71:
        ctl = C_filterq; //Filter Q (Sound Timbre)
        break;
    case 74:
        ctl = C_filtercutoff; //Filter Cutoff (Brightness)
        break;
    case 75:
        ctl = C_bandwidth; //BandWidth
        break;
    case 76:
        ctl = C_fmamp; //FM amplitude
        break;
    case 77:
        ctl = C_resonance_center; //Resonance Center Frequency
        break;
    case 78:
        ctl = C_resonance_bandwidth; //Resonance Bandwith
        break;
    case 120:
        ctl = C_allsoundsoff; //All Sounds OFF
        break;
    case 121:
        ctl = C_resetallcontrollers; //Reset All Controllers
        break;
    case 123:
        ctl = C_allnotesoff; //All Notes OFF
        break;
    //RPN and NRPN
    case 0x06:
        ctl = C_dataentryhi; //Data Entry (Coarse)
        break;
    case 0x26:
        ctl = C_dataentrylo; //Data Entry (Fine)
        break;
    case 99:
        ctl = C_nrpnhi; //NRPN (Coarse)
        break;
    case 98:
        ctl = C_nrpnlo; //NRPN (Fine)
        break;
    default:
        ctl = C_NULL; //unknown controller
        //fprintf(stderr,"Controller=%d , par=%d\n",midievent->data.control.param,cmdparams[1]);
        break;
    }
    return ctl;
}

