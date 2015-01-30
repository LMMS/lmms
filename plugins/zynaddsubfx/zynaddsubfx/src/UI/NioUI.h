#ifndef NIOUI_H
#define NIOUI_H

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

class NioUI:public Fl_Window
{
    public:
        NioUI();
        ~NioUI();
        void refresh();
    private:
        class Fl_Choice * midi;
        class Fl_Choice * audio;
        static void midiCallback(Fl_Widget *c);
        static void audioCallback(Fl_Widget *c);
};

#endif
