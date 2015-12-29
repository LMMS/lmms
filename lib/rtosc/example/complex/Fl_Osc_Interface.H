#pragma once
#include <stdio.h>
#include <string>

class Fl_Osc_Interface
{
    public:
        //It is assumed that you want to have a registry for all of these
        //elements
        virtual void createLink(std::string s, class Fl_Osc_Widget*){printf("linking %s...\n", s.c_str());};
        virtual void renameLink(std::string,std::string,Fl_Osc_Widget*){};
        virtual void removeLink(std::string,Fl_Osc_Widget*){};

        //Communication link
        virtual void requestValue(std::string){};
        virtual void writeValue(std::string s, float f){printf("%s -> %f\n",s.c_str(), f); };
        virtual void writeValue(std::string, int){};
        virtual void writeValue(std::string, bool){};
        virtual void writeValue(std::string, std::string){};
};
