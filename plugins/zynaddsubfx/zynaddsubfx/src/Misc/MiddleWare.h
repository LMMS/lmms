#pragma once
#include <functional>
#include <cstdarg>

//Link between realtime and non-realtime layers
class MiddleWare
{
    public:
        MiddleWare(void);
        ~MiddleWare(void);
        //returns internal master pointer
        class Master *spawnMaster(void);
        //return  UI interface
        class Fl_Osc_Interface *spawnUiApi(void);
        //Set callback to push UI events to
        void setUiCallback(void(*cb)(void*,const char *),void *ui);
        //Set callback to run while busy
        void setIdleCallback(void(*cb)(void));
        //Handle events
        void tick(void);
        //Do A Readonly Operation (For Parameter Copy)
        void doReadOnlyOp(std::function<void()>);
        //Handle a rtosc Message uToB
        void transmitMsg(const char *);
        //Handle a rtosc Message uToB
        void transmitMsg(const char *, const char *args, ...);
        //Handle a rtosc Message uToB
        void transmitMsg(const char *, const char *args, va_list va);
        //Indicate that a program will be loaded on a known part
        void pendingSetProgram(int part, int program);
        //Get/Set the active bToU url
        std::string activeUrl(void);
        void activeUrl(std::string u);
    private:
        class MiddleWareImpl *impl;
};
