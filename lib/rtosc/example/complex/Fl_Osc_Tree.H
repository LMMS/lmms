#pragma once
#include <rtosc/ports.h>
#include "Fl_Osc_Interface.H"
#include <FL/Fl_Tree.H>
#include <string>

class Fl_Osc_Tree:Fl_Tree
{
    public:
        Fl_Osc_Tree(int X, int Y, int W, int H, const char *L=0)
            :Fl_Tree(X,Y,W,H,L)
        {
            root_label("");
            add("nil");
            add("/nil/nil");
            close(first());
            callback(Fl_Osc_Tree::cb, NULL);
        }

        void sprout(std::string s)
        {
            if(s[s.length()-1] == '/') {
                attach(s);
                attach(s+"nil");
                close(s.c_str());
            } else
                attach(s);
        }

        void attach(std::string s)
        {
            if(!find_item(s.c_str()))
                add(s.c_str());
        }

        static void cb(Fl_Widget *w, void*)
        {
            using namespace rtosc;
            Fl_Osc_Tree *t=(Fl_Osc_Tree*)w;
            int reason = t->callback_reason();

            char pathname[1024];
            t->item_pathname(pathname, sizeof(pathname), t->callback_item());


            if(reason==1) {
                char *colon = strchr(pathname, ':');
                if(colon) {
                    *colon = 0;
                    t->osc->writeValue("/learn", std::string(pathname));
                }
            }

            //TODO remove fields
            //TODO understand nullable fields

            if(reason==3) //Populate fields
            {
                printf("pathname='%s'\n", pathname);
                printf("root_ports='%p'\n", t->root_ports);
                const Ports &p = *Fl_Osc_Tree::subtree_lookup(t->root_ports,
                        *pathname == '/' ? pathname + 1 : pathname);
                if(auto *i = t->find_item((std::string(pathname)+"/"+"nil").c_str()))
                    t->remove(i);
                for(const Port &port : p) {
                    printf("handling '%s'\n", port.name);
                    const bool subnodes   = strchr(port.name, '/');
                    const bool enumerated = strchr(port.name, '#');
                    const std::string path = std::string(pathname)+"/"+port.name;
                    if(!enumerated) {
                        t->sprout(path);
                    } else {
                        char tmpa[1024];
                        char tmpb[1024];
                        strncpy(tmpa, path.c_str(), 1024);
                        char *pound = strchr(tmpa, '#');
                        int N = atoi(pound+1);
                        *pound = 0;
                        char terminal = subnodes ? '/' : '\0';

                        for(int i = 0; i < N; ++i) {
                            snprintf(tmpb, 1024, "%s%d%c",
                                    tmpa, i, terminal);
                            t->sprout(tmpb);
                        }
                    }
                }
            }
        }

        static const rtosc::Ports *subtree_lookup(const rtosc::Ports *p, std::string s)
        {
            using namespace rtosc;
            if(s=="")
                return p;

            for(const Port &port : *p) {
                const char *name  = port.name;
                if(strchr(name,'/') && s.substr(0,(strchr(name,'/')-name)+1) == std::string(name).substr(0,strlen(name)-1)){
                    return subtree_lookup(port.ports, s.substr(strchr(name,'/')-name));
                }
            }

            //TODO else case
            return p;
        }

        rtosc::Ports *root_ports;
        Fl_Osc_Interface *osc;
};
