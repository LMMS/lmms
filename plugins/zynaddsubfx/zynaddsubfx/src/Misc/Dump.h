/*
  ZynAddSubFX - a software synthesizer

  Dump.h - It dumps the notes to a text file

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
#ifndef DUMP_H
#define DUMP_H

#include <stdio.h>

/**Object used to dump the notes into a text file
 * \todo see if this object should have knowledge about the file
 * that it will write to
 * \todo upgrade from stdio to iostream*/
class Dump
{
    public:
        /**Constructor*/
        Dump();
        /**Destructor
         * Closes the dumpfile*/
        ~Dump();
        /**Open dumpfile and prepare it for dumps
         * \todo see if this fits better in the constructor*/
        void startnow();
        /**Tick the timestamp*/
        void inctick();
        /**Dump Note to dumpfile
         * @param chan The channel of the note
         * @param note The note
         * @param vel The velocity of the note*/
        void dumpnote(char chan, char note, char vel);
        /** Dump the Controller
         * @param chan The channel of the Controller
         * @param type The type
         * @param par The value of the controller
         * \todo figure out what type is exactly meaning*/
        void dumpcontroller(char chan, unsigned int type, int par);

    private:
        FILE *file;
        int   tick;
        int   k; //This appears to be a constant used to flush the file
        //periodically when JACK is used
        int keyspressed;
};
#endif
