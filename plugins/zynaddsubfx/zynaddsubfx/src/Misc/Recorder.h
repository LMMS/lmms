/*
  ZynAddSubFX - a software synthesizer

  Recorder.h - Records sound to a file
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

#ifndef RECORDER_H
#define RECORDER_H
#include <string>
#include "../globals.h"

/**Records sound to a file*/
class Recorder
{
    public:

        Recorder();
        ~Recorder();
        /**Prepare the given file.
         * @returns 1 if the file exists */
        int preparefile(std::string filename_, int overwrite);
        void start();
        void stop();
        void pause();
        int recording();
        void triggernow();

        /** Status:
         *  0 - not ready(no file selected),
         *  1 - ready
         *  2 - recording */
        int status;

    private:
        int notetrigger;
};

#endif
