/*
  ZynAddSubFX - a software synthesizer

  DelayCtl.h - Control For Delays
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
#include "Control.h"

#ifndef DELAYCTL_H
#define DELAYCTL_H
/**A Control for Delays
 *
 * Will vary from 0 seconds to 1.5 seconds*/
class DelayCtl:public Control
{
public:
    DelayCtl();
    ~DelayCtl() {};
    std::string getString() const;
    void setmVal(char nval);
    char getmVal() const;
    float getiVal() const;
private:
    float value;
};

#endif

