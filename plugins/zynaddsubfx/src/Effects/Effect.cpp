/*
  ZynAddSubFX - a software synthesizer

  Effect.C - this class is inherited by the all effects(Reverb, Echo, ..)
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

#include "Effect.h"


Effect::Effect(bool insertion_, REALTYPE *const efxoutl_,
               REALTYPE *const efxoutr_, FilterParams *filterpars_,
               const unsigned char &Ppreset_)
    :Ppreset(Ppreset_), efxoutl(efxoutl_), efxoutr(efxoutr_),
      filterpars(filterpars_), insertion(insertion_) {}

