#pragma once
#include <string>
#include <cstring>
#include <cmath>
#include <rtosc/ports.h>
#include <FL/Fl_Group.H>
#include "Fl_Osc_Pane.H"

class Fl_Osc_Widget
{
    public:
        Fl_Osc_Widget(std::string nname, const char *metadata)
            :path(dynamic_cast<Fl_Osc_Group*>(Fl_Group::current()) ?
                    dynamic_cast<Fl_Osc_Group*>(Fl_Group::current())->pane_name : "/"),
            name(nname,0,nname.find_first_of(":")),
            full_path(path+name),meta(metadata)
        {}
        ~Fl_Osc_Widget(void){};

        virtual void OSC_value(float) {}
        virtual void OSC_value(bool) {}
        virtual void OSC_value(int) {}
        virtual void OSC_value(unsigned,void*) {}

        std::string path;
        std::string name;
        std::string full_path;
        rtosc::Port::MetaContainer meta;

        class Fl_Osc_Interface *osc;
    protected:
        //translations
        float translate(float x) const
        {
            for(auto e : meta)
                fprintf(stderr, "'%s' => '%s'\n", e.title, e.value);
            if(!meta["min"] || !meta["max"] || !meta["scale"]) {
                fprintf(stderr, "failed to get properties\n");
                return 0.0f;
            }

            const float min   = atof(meta["min"]);
            const float max   = atof(meta["max"]);
            const char *scale = meta["scale"];

            if(!strcmp(scale,"linear"))
                return x*(max-min)+min;
            else if(!strcmp(scale,"logarithmic")) {
                const float b = log(min);
                const float a = log(max)-b;
                return expf(a*x+b);
            }

            return 0.0f;
        }

        float inv_translate(float x) const
        {
            if(!meta["min"] || !meta["max"] || !meta["scale"])
                return 0.0f;

            const float min   = atof(meta["min"]);
            const float max   = atof(meta["max"]);
            const char *scale = meta["scale"];

            if(!strcmp(scale, "linear"))
                return (x-min)/(max-min);
            else if(!strcmp(scale, "logarithmic")) {
                const float b = log(min);
                const float a = log(max)-b;
                return (log(x)-b)/a;
            }

            return 0.0f;
        }
};
