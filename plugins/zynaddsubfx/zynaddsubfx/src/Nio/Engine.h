/*
  ZynAddSubFX - a software synthesizer

  Engine.h - Audio Driver base class
  Copyright (C) 2009-2010 Mark McCurry
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

#ifndef ENGINE_H
#define ENGINE_H
#include <string>
/**Marker for input/output driver*/
class Engine
{
    public:
        Engine();
        virtual ~Engine();

        /**Start the Driver with all capabilities
         * @return true on success*/
        virtual bool Start() = 0;
        /**Completely stop the Driver*/
        virtual void Stop() = 0;

        std::string name;
};
#endif
