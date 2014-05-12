/*
  ZynAddSubFX - a software synthesizer

  Presets.h - Presets and Clipboard management
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

#ifndef PRESETS_H
#define PRESETS_H

#include "../Misc/XMLwrapper.h"

#include "PresetsStore.h"

/**Presets and Clipboard management*/
class Presets
{
    friend class PresetsArray;
    public:
        Presets();
        virtual ~Presets();

        virtual void copy(const char *name); /**<if name==NULL, the clipboard is used*/
        virtual void paste(int npreset); //npreset==0 for clipboard
        virtual bool checkclipboardtype();
        void deletepreset(int npreset);

        char type[MAX_PRESETTYPE_SIZE];
        //void setelement(int n);

        void rescanforpresets();

    protected:
        void setpresettype(const char *type);
    private:
        virtual void add2XML(XMLwrapper *xml)    = 0;
        virtual void getfromXML(XMLwrapper *xml) = 0;
        virtual void defaults() = 0;
};

#endif
