#ifndef ENGINE_MGR_H
#define ENGINE_MGR_H

#include <list>
#include <string>
#include "Engine.h"


class MidiIn;
class AudioOut;
class OutMgr;
/**Container/Owner of the long lived Engines*/
class EngineMgr
{
    public:
        static EngineMgr &getInstance();
        ~EngineMgr();

        /**Gets requested engine
         * @param name case unsensitive name of engine
         * @return pointer to Engine or NULL
         */
        Engine *getEng(std::string name);

        /**Start up defaults*/
        bool start();

        /**Stop all engines*/
        void stop();

        std::list<Engine *> engines;

        //return false on failure
        bool setInDefault(std::string name);
        bool setOutDefault(std::string name);

        //default I/O
        AudioOut *defaultOut;
        MidiIn   *defaultIn;
    private:
        EngineMgr();
};
#endif
