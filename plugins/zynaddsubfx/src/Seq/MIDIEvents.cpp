/*
  ZynAddSubFX - a software synthesizer

  MIDIEvents.C - It stores the midi events from midi file or sequencer
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

#include "MIDIEvents.h"
#include <stdlib.h>
#include <stdio.h>

MIDIEvents::MIDIEvents()
{}

MIDIEvents::~MIDIEvents()
{}


/************** Track stuff ***************/
void MIDIEvents::writeevent(list *l, event *ev)
{
    listpos *tmp = new listpos;
    tmp->next = NULL;
    tmp->ev   = *ev;
    if(l->current != NULL)
        l->current->next = tmp;
    else
        l->first = tmp;
    l->current = tmp;
//    printf("Wx%x ",(int) l->current);
//    printf("-> %d  \n",l->current->ev.deltatime);
    l->size++;
}

void MIDIEvents::readevent(list *l, event *ev)
{
    if(l->current == NULL) {
        ev->type = -1;
        return;
    }
    *ev = l->current->ev;
    l->current = l->current->next;

    //test
    if(l->current != NULL) {
//	ev->deltatime=10000;
//	printf("Rx%d\n",l->current->ev.deltatime);
//	printf("Rx%x  ",(int) l->current);
//	printf("-> %d  (next=%x) \n",(int)l->current->ev.deltatime,(int)l->current->next);
    }
}


void MIDIEvents::rewindlist(list *l)
{
    l->current = l->first;
}

void MIDIEvents::deletelist(list *l)
{
    l->current = l->first;
    if(l->current == NULL)
        return;
    while(l->current->next != NULL) {
        listpos *tmp = l->current;
        l->current = l->current->next;
        delete (tmp);
    }
    deletelistreference(l);
}

void MIDIEvents::deletelistreference(list *l)
{
    l->current = l->first = NULL;
    l->size    = 0;
    l->length  = 0.0;
}

