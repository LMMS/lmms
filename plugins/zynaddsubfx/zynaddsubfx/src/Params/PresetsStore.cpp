/*
  ZynAddSubFX - a software synthesizer

  PresetsStore.cpp - Presets and Clipboard store
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
#include <iostream>
#include <algorithm>
#include <cctype>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "PresetsStore.h"
#include "../Misc/Util.h"

using namespace std;

PresetsStore presetsstore;

PresetsStore::PresetsStore()
{
    clipboard.data    = NULL;
    clipboard.type[0] = 0;
}

PresetsStore::~PresetsStore()
{
    if(clipboard.data != NULL)
        free(clipboard.data);
    clearpresets();
}

//Clipboard management

void PresetsStore::copyclipboard(XMLwrapper *xml, char *type)
{
    strcpy(clipboard.type, type);
    if(clipboard.data != NULL)
        free(clipboard.data);
    clipboard.data = xml->getXMLdata();
}

bool PresetsStore::pasteclipboard(XMLwrapper *xml)
{
    if(clipboard.data != NULL)
        xml->putXMLdata(clipboard.data);
    else
        return false;
    return true;
}

bool PresetsStore::checkclipboardtype(const char *type)
{
    //makes LFO's compatible
    if((strstr(type,
               "Plfo") != NULL) && (strstr(clipboard.type, "Plfo") != NULL))
        return true;
    return strcmp(type, clipboard.type) == 0;
}

//Presets management
void PresetsStore::clearpresets()
{
    presets.clear();
}

//a helper function that compares 2 presets[]
bool PresetsStore::presetstruct::operator<(const presetstruct &b) const
{
    return name < b.name;
}


void PresetsStore::rescanforpresets(const string &type)
{
    //std::cout << "Scanning For Presets" << std::endl;
    //std::cout << "Of Type: " << type << std::endl;

    clearpresets();
    string ftype = "." + type.substr(1) + ".xpz";

    for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i) {
        if(config.cfg.presetsDirList[i].empty())
            continue;

        //open directory
        string dirname = config.cfg.presetsDirList[i];
        DIR   *dir     = opendir(dirname.c_str());
        if(dir == NULL)
            continue;
        struct dirent *fn;

        //check all files in directory
        while((fn = readdir(dir))) {
            string filename = fn->d_name;
            if(filename.find(ftype) == string::npos)
                continue;

            //ensure proper path is formed
            char tmpc = dirname[dirname.size() - 1];
            const char *tmps;
            if((tmpc == '/') || (tmpc == '\\'))
                tmps = "";
            else
                tmps = "/";

            string location = "" + dirname + tmps + filename;

            //trim file type off of name
            string name = filename.substr(0, filename.find(ftype));

            //put on list
            presets.push_back(presetstruct(location, name));
        }

        closedir(dir);
    }

    //sort the presets
    sort(presets.begin(), presets.end());
}


void PresetsStore::copypreset(XMLwrapper *xml, char *type, string name)
{
    if(config.cfg.presetsDirList[0].empty())
        return;

    //make the filenames legal
    name = legalizeFilename(name);

    //make path legal
    const string dirname = config.cfg.presetsDirList[0];
    char tmpc = dirname[dirname.size() - 1];
    const char *tmps;
    if((tmpc == '/') || (tmpc == '\\'))
        tmps = "";
    else
        tmps = "/";

    string filename("" + dirname + tmps + name + "." + &type[1] + ".xpz");

    xml->saveXMLfile(filename);
}

bool PresetsStore::pastepreset(XMLwrapper *xml, unsigned int npreset)
{
    npreset--;
    if(npreset >= presets.size())
        return false;
    string filename = presets[npreset].file;
    if(filename.empty())
        return false;
    bool result = (xml->loadXMLfile(filename) >= 0);
    return result;
}

void PresetsStore::deletepreset(unsigned int npreset)
{
    npreset--;
    if(npreset >= presets.size())
        return;
    string filename = presets[npreset].file;
    if(filename.empty())
        return;
    remove(filename.c_str());
}
