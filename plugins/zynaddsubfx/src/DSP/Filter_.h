/*
  ZynAddSubFX - a software synthesizer

  Filter_.h - This class is inherited by filter classes
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

#ifndef FILTER__H
#define FILTER__H

#include "../globals.h"

class Filter_
{
    public:
        virtual ~Filter_() {}
        virtual void filterout(REALTYPE *smp)    = 0;
        virtual void setfreq(REALTYPE frequency) = 0;
        virtual void setfreq_and_q(REALTYPE frequency, REALTYPE q_) = 0;
        virtual void setq(REALTYPE q_) = 0;
        virtual void setgain(REALTYPE dBgain) = 0;
        REALTYPE outgain;
    private:
};


#endif

