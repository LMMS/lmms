/*
  ZynAddSubFX - a software synthesizer

  LASHClient.h - LASH support
  Copyright (C) 2006-2009 Lars Luthman
  Author: Lars Luthman

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
#ifndef LASHClient_h
#define LASHClient_h

#include <string>
#include <pthread.h>
#include <lash/lash.h>


/** This class wraps up some functions for initialising and polling
 *  the LASH daemon.*/
class LASHClient
{
    public:
        /**Enum to represent the LASH events that are currently handled*/
        enum Event {
            Save,
            Restore,
            Quit,
            NoEvent
        };

        /** Constructor
         *  @param argc number of arguments
         *  @param argv the text arguments*/
        LASHClient(int *argc, char ***argv);

        /**set the ALSA id
         * @param id new ALSA id*/
        void setalsaid(int id);
        /**Set the JACK name
         * @param name the new name*/
        void setjackname(const char *name);
        Event checkevents(std::string &filename);
        void confirmevent(Event event);

    private:

        lash_client_t *client;
};


#endif
