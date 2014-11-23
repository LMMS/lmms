/*
  ZynAddSubFX - a software synthesizer

  Stereo.h - Object for storing a pair of objects
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
#ifndef STEREO_H
#define STEREO_H

template<class T>
struct Stereo {
    public:
        Stereo(const T &left, const T &right);

        /**Initializes Stereo with left and right set to val
         * @param val the value for both channels*/
        Stereo(const T &val);
        ~Stereo() {}

        Stereo<T> &operator=(const Stereo<T> &smp);

        //data
        T l, r;
};
#include "Stereo.cpp"
#endif
