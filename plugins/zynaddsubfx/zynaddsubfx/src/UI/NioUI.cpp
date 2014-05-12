#include "NioUI.h"
#include "../Nio/Nio.h"
#include <cstdio>
#include <iostream>
#include <cstring>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Spinner.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>

using namespace std;

NioUI::NioUI()
    :Fl_Window(200, 100, 400, 400, "New IO Controls")
{
    //hm, I appear to be leaking memory
    Fl_Group *settings = new Fl_Group(0, 20, 400, 400 - 35, "Settings");
    {
        audio = new Fl_Choice(60, 80, 100, 25, "Audio");
        audio->callback(audioCallback);
        midi = new Fl_Choice(60, 100, 100, 25, "Midi");
        midi->callback(midiCallback);
    }
    settings->end();

    //initialize midi list
    {
        set<string> midiList = Nio::getSources();
        string      source   = Nio::getSource();
        int midival = 0;
        for(set<string>::iterator itr = midiList.begin();
            itr != midiList.end(); ++itr) {
            midi->add(itr->c_str());
            if(*itr == source)
                midival = midi->size() - 2;
        }
        midi->value(midival);
    }

    //initialize audio list
    {
        set<string> audioList = Nio::getSinks();
        string      sink      = Nio::getSink();
        int audioval = 0;
        for(set<string>::iterator itr = audioList.begin();
            itr != audioList.end(); ++itr) {
            audio->add(itr->c_str());
            if(*itr == sink)
                audioval = audio->size() - 2;
        }
        audio->value(audioval);
    }
    resizable(this);
    size_range(400, 300);
}

NioUI::~NioUI()
{}

void NioUI::refresh()
{}

void NioUI::midiCallback(Fl_Widget *c)
{
    bool good = Nio::setSource(static_cast<Fl_Choice *>(c)->text());
    static_cast<Fl_Choice *>(c)->textcolor(fl_rgb_color(255 * !good, 0, 0));
}

void NioUI::audioCallback(Fl_Widget *c)
{
    bool good = Nio::setSink(static_cast<Fl_Choice *>(c)->text());
    static_cast<Fl_Choice *>(c)->textcolor(fl_rgb_color(255 * !good, 0, 0));
}
