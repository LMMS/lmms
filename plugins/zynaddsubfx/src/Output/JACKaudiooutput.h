/*
  ZynAddSubFX - a software synthesizer

  JACKaudiooutput.h - Audio output for JACK
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
#ifndef JACK_AUDIO_OUTPUT_H
#define JACK_AUDIO_OUTPUT_H

#include <jack/jack.h>

#include "../globals.h"
#include "../Misc/Master.h"


#if (REALTYPE!=jack_default_audio_sample_t)
#error "The internal sample datatype of ZynAddSubFX and the datatype of jack differs.\
 In order to compile ZynAddSubFX the 'REALTYPE' and 'jack_default_audio_sample_t' must be equal.\
 Set the 'REALTYPE' data type (which is defined in 'globals.h') to what is defined \
 in the file types.h from jack include directory as 'jack_default_audio_sample_t' (as float or double)."
#endif




bool JACKaudiooutputinit(Master *master_);
void JACKfinish();
void JACKhandlemidi(unsigned long frames);
const char* JACKgetname();

#endif

