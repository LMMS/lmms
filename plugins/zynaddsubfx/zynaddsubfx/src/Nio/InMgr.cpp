#include "InMgr.h"
#include "MidiIn.h"
#include "EngineMgr.h"
#include "../Misc/Master.h"
#include <iostream>

using namespace std;

ostream &operator<<(ostream &out, const MidiEvent &ev)
{
    switch(ev.type) {
        case M_NOTE:
            out << "MidiNote: note(" << ev.num << ")\n"
            << "          channel(" << ev.channel << ")\n"
            << "          velocity(" << ev.value << ")";
            break;

        case M_CONTROLLER:
            out << "MidiCtl: controller(" << ev.num << ")\n"
            << "         channel(" << ev.channel << ")\n"
            << "         value(" << ev.value << ")";
            break;

        case M_PGMCHANGE:
            out << "PgmChange: program(" << ev.num << ")\n"
            << "           channel(" << ev.channel << ")";
            break;
    }

    return out;
}

MidiEvent::MidiEvent()
    :channel(0), type(0), num(0), value(0), time(0)
{}

InMgr &InMgr::getInstance()
{
    static InMgr instance;
    return instance;
}

InMgr::InMgr()
    :queue(100), master(Master::getInstance())
{
    current = NULL;
    work.init(PTHREAD_PROCESS_PRIVATE, 0);
}

InMgr::~InMgr()
{
    //lets stop the consumer thread
}

void InMgr::putEvent(MidiEvent ev)
{
    if(queue.push(ev)) //check for error
        cerr << "ERROR: Midi Ringbuffer is FULL" << endl;
    else
        work.post();
}

void InMgr::flush(unsigned frameStart, unsigned frameStop)
{
    MidiEvent ev;
    while(!work.trywait()) {
        queue.peak(ev);
        if(ev.time < (int)frameStart || ev.time > (int)frameStop) {
            //Back out of transaction
            work.post();
            //printf("%d vs [%d..%d]\n",ev.time, frameStart, frameStop);
            break;
        }
        queue.pop(ev);
        //cout << ev << endl;

        switch(ev.type) {
            case M_NOTE:
                dump.dumpnote(ev.channel, ev.num, ev.value);

                if(ev.value)
                    master.noteOn(ev.channel, ev.num, ev.value);
                else
                    master.noteOff(ev.channel, ev.num);
                break;

            case M_CONTROLLER:
                dump.dumpcontroller(ev.channel, ev.num, ev.value);
                master.setController(ev.channel, ev.num, ev.value);
                break;

            case M_PGMCHANGE:
                master.setProgram(ev.channel, ev.num);
                break;
            case M_PRESSURE:
                master.polyphonicAftertouch(ev.channel, ev.num, ev.value);
                break;
        }
    }
}

bool InMgr::empty(void) const
{
    int semvalue = work.getvalue();
    return semvalue <= 0;
}

bool InMgr::setSource(string name)
{
    MidiIn *src = getIn(name);

    if(!src)
        return false;

    if(current)
        current->setMidiEn(false);
    current = src;
    current->setMidiEn(true);

    bool success = current->getMidiEn();

    //Keep system in a valid state (aka with a running driver)
    if(!success)
        (current = getIn("NULL"))->setMidiEn(true);

    return success;
}

string InMgr::getSource() const
{
    if(current)
        return current->name;
    else
        return "ERROR";
}

MidiIn *InMgr::getIn(string name)
{
    EngineMgr &eng = EngineMgr::getInstance();
    return dynamic_cast<MidiIn *>(eng.getEng(name));
}
