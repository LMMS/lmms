/*
  ZynAddSubFX - a software synthesizer

  FqSample.h - Object for storing information on samples
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
#ifndef MONOSAMPLE_H
#define MONOSAMPLE_H

#include "FqSample.h"
#include "Sample.h"

class FqSample : public Sample
{
public:
    FqSample(int length,REALTYPE fill=0);
    FqSample(int length,const REALTYPE *input);
    ~FqSample();
    //FqSample &operator=(const FqSample &smp);
    //float *dontuse(){return buffer;};
};
#endif

