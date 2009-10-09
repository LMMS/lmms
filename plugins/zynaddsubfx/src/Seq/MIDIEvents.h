/*
  ZynAddSubFX - a software synthesizer

  MIDIEvents.h - It stores the midi events from midi file or sequencer
  Copyright (C) 2003-2005 Nasca Octavian Paul
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
#ifndef MIDI_EVENTS_H
#define MIDI_EVENTS_H

#include "../globals.h"
#define NUM_MIDI_TRACKS NUM_MIDI_CHANNELS

/**storage the midi events from midi file or sequencer
 * \todo this looks quite like a remake of a linked list
 *       if it is, then it should be rewritten to use <list>*/
class MIDIEvents
{
    friend class MIDIFile;
    public:
        MIDIEvents();
        ~MIDIEvents();

    protected:

        /* Events */
        struct event {
            int deltatime;
            int channel; //on what midi channel is
            int type, par1, par2; //type=1 for note, type=2 for controller, type=255 for time messages
        } tmpevent;
        struct listpos {
            event ev;
            struct listpos *next;
        };
        struct list {
            listpos *first, *current;
            int      size; //how many events are
            double   length; //in seconds
        };
        struct {
            list track; //the stored track
            list record; //the track being "recorded" from midi
        } miditrack[NUM_MIDI_TRACKS];

        void writeevent(list *l, event *ev);
        void readevent(list *l, event *ev);

        void rewindlist(list *l);
        void deletelist(list *l);
        void deletelistreference(list *l);
};

#endif

