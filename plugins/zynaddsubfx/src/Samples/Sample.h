/*
  ZynAddSubFX - a software synthesizer

  Sample.h - Object for storing information on samples
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
#ifndef SAMPLE_H
#define SAMPLE_H
#include "../globals.h"
/**
 * Base Class for Samples
 */
class Sample
{
public:
    Sample(const Sample &smp);
    Sample(int length,REALTYPE fill=0);
    Sample(int length,const REALTYPE *fill);
    ~Sample();
    /**Fills the buffer with zeros*/
    void clear();
    /**States the size of the buffer
     * @return the size of the buffer*/
    int size() const {
        return bufferSize;
    };
    /**Provides the indexing operator for non const Samples*/
    REALTYPE &operator[](int index) {
        return *(buffer+index%bufferSize);
    };
    /**Provides the indexing operator for const Samples*/
    const REALTYPE &operator[](int index)const {
        return *(buffer+index%bufferSize);
    };
    /**Provides the assignment operator*/
    void operator=(const Sample &smp);
    /**Provides the == operator*/
    bool operator==(const Sample &smp)const;
    /**Provides direct access to the buffer to allow for transition
     *
     * This method is like c_str() from the string class and should be used
     * sparingly*/
    const REALTYPE *c_buf() {
        return buffer;
    };
    REALTYPE max()const;
    REALTYPE min()const;
    REALTYPE absMax()const;
private:
    int bufferSize;
    float *buffer;


};
#endif

