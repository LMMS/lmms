/*
  ZynAddSubFX - a software synthesizer

  DelayCtl.C - Control For Delays
  Copyright (C) 2009-2009 Mark McCurry
  Author: Mark McCurry

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
#include "DelayCtl.h"
#include <sstream>

DelayCtl::DelayCtl()
        :Control(64),value(64/127.0*1.5) {} /**\todo finishme*/

std::string DelayCtl::getString() const
{
    std::ostringstream buf;
    buf<<value;
    return (buf.str() + " Seconds");
}

void DelayCtl::setmVal(char nval)
{
    /**\todo add locking code*/
    //value=1+(int)(nval/127.0*SAMPLE_RATE*1.5);//0 .. 1.5 sec
    value=(nval/127.0*1.5);//0 .. 1.5 sec
}

char DelayCtl::getmVal() const
{
    return((char)(value/1.5*127.0));
}

float DelayCtl::getiVal() const
{
    return(value);
}
