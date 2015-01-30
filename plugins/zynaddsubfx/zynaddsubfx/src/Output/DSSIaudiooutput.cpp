/*
  ZynAddSubFX - a software synthesizer

  DSSIaudiooutput.cpp - Audio functions for DSSI
  Copyright (C) 2002 Nasca Octavian Paul
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

/*
 * Inital working DSSI output code contributed by Stephen G. Parry
 */

//this file contains code used from trivial_synth.c from
//the DSSI (published by Steve Harris under public domain) as a template.

#include "DSSIaudiooutput.h"
#include "../Misc/Config.h"
#include "../Misc/Bank.h"
#include "../Misc/Util.h"
#include <string.h>
#include <limits.h>

using std::string;
using std::vector;

//Dummy variables and functions for linking purposes
const char *instance_name = 0;
class WavFile;
namespace Nio {
    bool start(void){return 1;};
    void stop(void){};
    void waveNew(WavFile *){}
    void waveStart(void){}
    void waveStop(void){}
    void waveEnd(void){}
}

//
// Static stubs for LADSPA member functions
//
// LADSPA is essentially a C handle based API; This plug-in implementation is
// a C++ OO one so we need stub functions to map from C API calls to C++ object
// method calls.
void DSSIaudiooutput::stub_connectPort(LADSPA_Handle instance,
                                       unsigned long port,
                                       LADSPA_Data *data)
{
    getInstance(instance)->connectPort(port, data);
}

void DSSIaudiooutput::stub_activate(LADSPA_Handle instance)
{
    getInstance(instance)->activate();
}

void DSSIaudiooutput::stub_run(LADSPA_Handle instance,
                               unsigned long sample_count)
{
    getInstance(instance)->run(sample_count);
}

void DSSIaudiooutput::stub_deactivate(LADSPA_Handle instance)
{
    getInstance(instance)->deactivate();
}


void DSSIaudiooutput::stub_cleanup(LADSPA_Handle instance)
{
    DSSIaudiooutput *plugin_instance = getInstance(instance);
    plugin_instance->cleanup();
    delete plugin_instance;
}


const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    return DSSIaudiooutput::getLadspaDescriptor(index);
}

//
// Static stubs for DSSI member functions
//
// DSSI is essentially a C handle based API; This plug-in implementation is
// a C++ OO one so we need stub functions to map from C API calls to C++ object
// method calls.
const DSSI_Program_Descriptor *DSSIaudiooutput::stub_getProgram(
    LADSPA_Handle instance,
    unsigned long index)
{
    return getInstance(instance)->getProgram(index);
}

void DSSIaudiooutput::stub_selectProgram(LADSPA_Handle instance,
                                         unsigned long bank,
                                         unsigned long program)
{
    getInstance(instance)->selectProgram(bank, program);
}

int DSSIaudiooutput::stub_getMidiControllerForPort(LADSPA_Handle instance,
                                                   unsigned long port)
{
    return getInstance(instance)->getMidiControllerForPort(port);
}

void DSSIaudiooutput::stub_runSynth(LADSPA_Handle instance,
                                    unsigned long sample_count,
                                    snd_seq_event_t *events,
                                    unsigned long event_count)
{
    getInstance(instance)->runSynth(sample_count, events, event_count);
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
    return DSSIaudiooutput::getDssiDescriptor(index);
}

//
// LADSPA member functions
//

/**
 * Instantiates a plug-in.
 *
 * This LADSPA member function instantiates a plug-in.
 * Note that instance initialisation should generally occur in
 * activate() rather than here.
 *
 * Zyn Implementation
 * ------------------
 * This implementation creates a C++ class object and hides its pointer
 * in the handle by type casting.
 *
 * @param descriptor [in] the descriptor for this plug-in
 * @param s_rate [in] the sample rate
 * @return the plug-in instance handle if successful else NULL
 */
LADSPA_Handle DSSIaudiooutput::instantiate(const LADSPA_Descriptor *descriptor,
                                           unsigned long s_rate)
{
    if(descriptor->UniqueID == dssiDescriptor->LADSPA_Plugin->UniqueID)
        return (LADSPA_Handle)(new DSSIaudiooutput(s_rate));
    else
        return NULL;
}

/**
 * Connects a port on an instantiated plug-in.
 *
 * This LADSPA member function connects a port on an instantiated plug-in to a
 * memory location at which a block of data for the port will be read/written.
 * The data location is expected to be an array of LADSPA_Data for audio ports
 * or a single LADSPA_Data value for control ports. Memory issues will be
 * managed by the host. The plug-in must read/write the data at these locations
 * every time run() or run_adding() is called and the data present at the time
 * of this connection call should not be considered meaningful.
 *
 * Zyn Implementation
 * ------------------
 * The buffer pointers are stored as member variables
 *
 * @param port [in] the port to be connected
 * @param data [in] the data buffer to write to / read from
 */
void DSSIaudiooutput::connectPort(unsigned long port, LADSPA_Data *data)
{
    switch(port) {
        case 0:
            outl = data;
            break;
        case 1:
            outr = data;
            break;
    }
}

/**
 * Initialises a plug-in instance and activates it for use.
 *
 * This LADSPA member function initialises a plug-in instance and activates it
 * for use. This is separated from instantiate() to aid real-time support and
 * so that hosts can reinitialise a plug-in instance by calling deactivate() and
 * then activate(). In this case the plug-in instance must reset all state
 * information dependent on the history of the plug-in instance except for any
 * data locations provided by connect_port() and any gain set by
 * set_run_adding_gain().
 *
 * Zyn Implementation
 * ------------------
 * Currently this does nothing; Care must be taken as to code placed here as
 * too much code here seems to cause time-out problems in jack-dssi-host.
*/
void DSSIaudiooutput::activate()
{}

/**
 * Runs an instance of a plug-in for a block.
 *
 * This LADSPA member function runs an instance of a plug-in for a block.
 * Note that if an activate() function exists then it must be called before
 * run() or run_adding(). If deactivate() is called for a plug-in instance then
 * the plug-in instance may not be reused until activate() has been called again.
 *
 * Zyn Implementation
 * ------------------
 * This is a LADSPA function that does not process any MIDI events; it is hence
 * implemented by simply calling runSynth() with an empty event list.
 *
 * @param sample_count [in] the block size (in samples) for which the plug-in instance may run
 */
void DSSIaudiooutput::run(unsigned long sample_count)
{
    runSynth(sample_count, NULL, (unsigned long)0);
}

/**
 * Counterpart to activate().
 *
 * This LADSPA member function is the counterpart to activate() (see above).
 * Deactivation is not similar to pausing as the plug-in instance will be
 * reinitialised when activate() is called to reuse it.
 *
 * Zyn Implementation
 * ------------------
 * Currently this function does nothing.
 */
void DSSIaudiooutput::deactivate()
{}

/**
 * Deletes a plug-in instance that is no longer required.
 *
 * LADSPA member function; once an instance of a plug-in has been finished with
 * it can be deleted using this function. The instance handle ceases to be
 * valid after this call.
 *
 * If activate() was called for a plug-in instance then a corresponding call to
 * deactivate() must be made before cleanup() is called.
 *
 * Zyn Implementation
 * ------------------
 * Currently cleanup is deferred to the destructor that is invoked after cleanup()
 */
void DSSIaudiooutput::cleanup()
{}

/**
 * Initial entry point for the LADSPA plug-in library.
 *
 * This LADSPA function is the initial entry point for the plug-in library.
 * The LADSPA host looks for this entry point in each shared library object it
 * finds and then calls the function to enumerate the plug-ins within the
 * library.
 *
 * Zyn Implementation
 * ------------------
 * As the Zyn plug-in is a DSSI plug-in, the LADSPA descriptor is embedded inside
 * the DSSI descriptor, which is created by DSSIaudiooutput::initDssiDescriptor()
 * statically when the library is loaded. This function then merely returns a pointer
 * to that embedded descriptor.
 *
 * @param index [in] the index number of the plug-in within the library.
 * @return if index is in range, a pointer to the plug-in descriptor is returned, else NULL
 */
const LADSPA_Descriptor *DSSIaudiooutput::getLadspaDescriptor(
    unsigned long index)
{
    if((index > 0) || (dssiDescriptor == NULL))
        return NULL;
    else
        return dssiDescriptor->LADSPA_Plugin;
}

//
// DSSI member functions
//

/**
 * Provides a description of a program available on this synth.
 *
 * This DSSI member function pointer provides a description of a program (named
 * preset sound) available on this synth.
 *
 * Zyn Implementation
 * ------------------
 * The instruments in all Zyn's bank directories, as shown by the `instrument
 * -> show instrument bank` command, are enumerated to the host by this
 * function, allowing access to all those instruments.
 * The first time an instrument is requested, the bank it is in and any
 * unmapped ones preceding that are mapped; all the instruments names and
 * filenames from those banks are stored in the programMap member variable for
 * later use. This is done on demand in this way, rather than up front in one
 * go because loading all the instrument names in one go can lead to timeouts
 * and zombies.
 *
 * @param index [in] index into the plug-in's list of
 * programs, not a program number as represented by the Program
 * field of the DSSI_Program_Descriptor.  (This distinction is
 * needed to support synths that use non-contiguous program or
 * bank numbers.)
 * @return a DSSI_Program_Descriptor pointer that is
 * guaranteed to be valid only until the next call to get_program,
 * deactivate, or configure, on the same plug-in instance, or NULL if index is out of range.
 */
const DSSI_Program_Descriptor *DSSIaudiooutput::getProgram(unsigned long index)
{
    static DSSI_Program_Descriptor retVal;

    /* Make sure we have the list of banks loaded */
    initBanks();

    /* Make sure that the bank containing the instrument has been mapped */
    while(index >= programMap.size() && mapNextBank())
        /* DO NOTHING MORE */;

    if(index >= programMap.size())
        /* No more instruments */
        return NULL;
    else {
        /* OK, return the instrument */
        retVal.Name    = programMap[index].name.c_str();
        retVal.Program = programMap[index].program;
        retVal.Bank    = programMap[index].bank;
        return &retVal;
    }
}

/**
 * Selects a new program for this synth.
 *
 * This DSSI member function selects a new program for this synth.  The program
 * change will take effect immediately at the start of the next run_synth()
 * call. An invalid bank / instrument combination is ignored.
 *
 * Zyn Implementation
 * ------------------
 * the banks and instruments are as shown in the `instrument -> show instrument
 * bank` command in Zyn. The bank no is a 1-based index into the list of banks
 * Zyn loads and shows in the drop down and the program number is the
 * instrument within that bank.
 *
 * @param bank [in] the bank number to select
 * @param program [in] the program number within the bank to select
 */
void DSSIaudiooutput::selectProgram(unsigned long bank, unsigned long program)
{
    initBanks();
//    cerr << "selectProgram(" << (bank & 0x7F) << ':' << ((bank >> 7) & 0x7F) << "," << program  << ")" << '\n';
    if((bank < master->bank.banks.size()) && (program < BANK_SIZE)) {
        const std::string bankdir = master->bank.banks[bank].dir;
        if(!bankdir.empty()) {
            pthread_mutex_lock(&master->mutex);

            /* We have to turn off the CheckPADsynth functionality, else
             * the program change takes way too long and we get timeouts
             * and hence zombies (!) */
            int save = config.cfg.CheckPADsynth;
            config.cfg.CheckPADsynth = 0;

            /* Load the bank... */
            master->bank.loadbank(bankdir);

            /* restore the CheckPADsynth flag */
            config.cfg.CheckPADsynth = save;

            /* Now load the instrument... */
            master->bank.loadfromslot((unsigned int)program, master->part[0]);

            pthread_mutex_unlock(&master->mutex);
        }
    }
}

/**
 * Returns the MIDI controller number or NRPN for a input control port
 *
 * This DSSI member function returns the MIDI controller number or NRPN that
 * should be mapped to the given input control port. If the given port should
 * not have any MIDI controller mapped to it, the function will return DSSI_NONE.
 * The behaviour of this function is undefined if the given port
 * number does not correspond to an input control port.
 *
 * Zyn Implementation
 * ------------------
 * Currently Zyn does not define any controller ports, but may do in the future.
 *
 * @param port [in] the input controller port
 * @return the CC and NRPN values shifted and ORed together.
 */
int DSSIaudiooutput::getMidiControllerForPort(unsigned long /*port*/)
{
    return DSSI_NONE;
}

/**
 * Runs the synth for a block.
 *
 * This DSSI member function runs the synth for a block.  This is identical in
 * function to the LADSPA run() function, except that it also supplies events
 * to the synth.
 *
 * Zyn Implementation
 * ------------------
 * Zyn implements synthesis in Master::GetAudioOutSamples; runSynth calls this
 * function in chunks delimited by the sample_count and the frame indexes in
 * the events block, calling the appropriate NoteOn, NoteOff and SetController
 * members of Master to process the events supplied between each chunk.
 *
 * @param sample_count [in] the block size (in samples) for which the synth
 * instance may run.
 * @param events [in] The Events pointer points to a block of ALSA
 * sequencer events, used to communicate MIDI and related events to the synth.
 * Each event must be timestamped relative to the start of the block,
 * (mis)using the ALSA "tick time" field as a frame count. The host is
 * responsible for ensuring that events with differing timestamps are already
 * ordered by time. Must not include NOTE (only NOTE_ON / NOTE_OFF), LSB or MSB
 * events.
 * @param event_count [in] the number of entries in the `events` block
 */
void DSSIaudiooutput::runSynth(unsigned long sample_count,
                               snd_seq_event_t *events,
                               unsigned long event_count)
{
    unsigned long from_frame       = 0;
    unsigned long event_index      = 0;
    unsigned long next_event_frame = 0;
    unsigned long to_frame = 0;
    pthread_mutex_lock(&master->mutex);

    do {
        /* Find the time of the next event, if any */
        if((events == NULL) || (event_index >= event_count))
            next_event_frame = ULONG_MAX;
        else
            next_event_frame = events[event_index].time.tick;

        /* find the end of the sub-sample to be processed this time round... */
        /* if the next event falls within the desired sample interval... */
        if((next_event_frame < sample_count) && (next_event_frame >= to_frame))
            /* set the end to be at that event */
            to_frame = next_event_frame;
        else
            /* ...else go for the whole remaining sample */
            to_frame = sample_count;
        if(from_frame < to_frame) {
            // call master to fill from `from_frame` to `to_frame`:
            master->GetAudioOutSamples(to_frame - from_frame,
                                       (int)sampleRate,
                                       &(outl[from_frame]),
                                       &(outr[from_frame]));
            // next sub-sample please...
            from_frame = to_frame;
        }

        // Now process any event(s) at the current timing point
        while(events != NULL && event_index < event_count
              && events[event_index].time.tick == to_frame) {
            if(events[event_index].type == SND_SEQ_EVENT_NOTEON)
                master->noteOn(events[event_index].data.note.channel,
                               events[event_index].data.note.note,
                               events[event_index].data.note.velocity);
            else
            if(events[event_index].type == SND_SEQ_EVENT_NOTEOFF)
                master->noteOff(events[event_index].data.note.channel,
                                events[event_index].data.note.note);
            else
            if(events[event_index].type == SND_SEQ_EVENT_CONTROLLER)
                master->setController(events[event_index].data.control.channel,
                                      events[event_index].data.control.param,
                                      events[event_index].data.control.value);
            else {}
            event_index++;
        }

        // Keep going until we have the desired total length of sample...
    } while(to_frame < sample_count);

    pthread_mutex_unlock(&master->mutex);
}

/**
 * Initial entry point for the DSSI plug-in library.
 *
 * This DSSI function is the initial entry point for the plug-in library.
 * The DSSI host looks for this entry point in each shared library object it
 * finds and then calls the function to enumerate the plug-ins within the
 * library.
 *
 * Zyn Implementation
 * ------------------
 * The descriptor is created statically by DSSIaudiooutput::initDssiDescriptor()
 * when the plug-in library is loaded. This function merely returns a pointer to
 * that descriptor.
 *
 * @param index [in] the index number of the plug-in within the library.
 * @return if index is in range, a pointer to the plug-in descriptor is returned, else NULL
 */
const DSSI_Descriptor *DSSIaudiooutput::getDssiDescriptor(unsigned long index)
{
    if((index > 0) || (dssiDescriptor == NULL))
        return NULL;
    else
        return dssiDescriptor;
}

//
// Internal member functions
//

// Initialise the DSSI descriptor, statically:
DSSI_Descriptor *DSSIaudiooutput::dssiDescriptor =
    DSSIaudiooutput::initDssiDescriptor();

/**
 * Initializes the DSSI (and LADSPA) descriptor, returning it is an object.
 */
DSSI_Descriptor *DSSIaudiooutput::initDssiDescriptor()
{
    DSSI_Descriptor *newDssiDescriptor = new DSSI_Descriptor;

    LADSPA_PortDescriptor *newPortDescriptors;
    const char **newPortNames;
    LADSPA_PortRangeHint *newPortRangeHints;

    if(newDssiDescriptor) {
        LADSPA_Descriptor *newLadspaDescriptor = new LADSPA_Descriptor;
        if(newLadspaDescriptor) {
            newLadspaDescriptor->UniqueID   = 100;
            newLadspaDescriptor->Label      = "ZASF";
            newLadspaDescriptor->Properties = 0;
            newLadspaDescriptor->Name  = "ZynAddSubFX";
            newLadspaDescriptor->Maker =
                "Nasca Octavian Paul <zynaddsubfx@yahoo.com>";
            newLadspaDescriptor->Copyright = "GNU General Public License v.2";
            newLadspaDescriptor->PortCount = 2;

            newPortNames    = new const char *[newLadspaDescriptor->PortCount];
            newPortNames[0] = "Output L";
            newPortNames[1] = "Output R";
            newLadspaDescriptor->PortNames = newPortNames;

            newPortDescriptors =
                new LADSPA_PortDescriptor[newLadspaDescriptor->PortCount];
            newPortDescriptors[0] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
            newPortDescriptors[1] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
            newLadspaDescriptor->PortDescriptors = newPortDescriptors;

            newPortRangeHints =
                new LADSPA_PortRangeHint[newLadspaDescriptor->PortCount];
            newPortRangeHints[0].HintDescriptor = 0;
            newPortRangeHints[1].HintDescriptor = 0;
            newLadspaDescriptor->PortRangeHints = newPortRangeHints;

            newLadspaDescriptor->activate     = stub_activate;
            newLadspaDescriptor->cleanup      = stub_cleanup;
            newLadspaDescriptor->connect_port = stub_connectPort;
            newLadspaDescriptor->deactivate   = stub_deactivate;
            newLadspaDescriptor->instantiate  = instantiate;
            newLadspaDescriptor->run = stub_run;
            newLadspaDescriptor->run_adding = NULL;
            newLadspaDescriptor->set_run_adding_gain = NULL;
        }
        newDssiDescriptor->LADSPA_Plugin    = newLadspaDescriptor;
        newDssiDescriptor->DSSI_API_Version = 1;
        newDssiDescriptor->configure   = NULL;
        newDssiDescriptor->get_program = stub_getProgram;
        newDssiDescriptor->get_midi_controller_for_port =
            stub_getMidiControllerForPort;
        newDssiDescriptor->select_program      = stub_selectProgram;
        newDssiDescriptor->run_synth           = stub_runSynth;
        newDssiDescriptor->run_synth_adding    = NULL;
        newDssiDescriptor->run_multiple_synths = NULL;
        newDssiDescriptor->run_multiple_synths_adding = NULL;
    }

    dssiDescriptor = newDssiDescriptor;

    return dssiDescriptor;
}

/**
 * Converts a LADSPA / DSSI handle into a DSSIaudiooutput instance.
 *
 * @param instance [in]
 * @return the instance
 */
DSSIaudiooutput *DSSIaudiooutput::getInstance(LADSPA_Handle instance)
{
    return (DSSIaudiooutput *)(instance);
}

SYNTH_T *synth;

/**
 * The private sole constructor for the DSSIaudiooutput class.
 *
 * Only ever called via instantiate().
 * @param sampleRate [in] the sample rate to be used by the synth.
 * @return
 */
DSSIaudiooutput::DSSIaudiooutput(unsigned long sampleRate)
{
    synth = new SYNTH_T;
    synth->samplerate = sampleRate;

    this->sampleRate  = sampleRate;
    this->banksInited = false;

    config.init();

    sprng(time(NULL));
    denormalkillbuf = new float [synth->buffersize];
    for(int i = 0; i < synth->buffersize; i++)
        denormalkillbuf[i] = (RND - 0.5f) * 1e-16;

    synth->alias();
    this->master = new Master();
}

/**
 * The destructor for the DSSIaudiooutput class
 * @return
 */
DSSIaudiooutput::~DSSIaudiooutput()
{}

/**
 * Ensures the list of bank (directories) has been initialised.
 */
void DSSIaudiooutput::initBanks(void)
{
    if(!banksInited) {
        pthread_mutex_lock(&master->mutex);
        master->bank.rescanforbanks();
        banksInited = true;
        pthread_mutex_unlock(&master->mutex);
    }
}

/**
 * constructor for the internally used ProgramDescriptor class
 *
 * @param _bank [in] bank number
 * @param _program [in] program number
 * @param _name [in] instrument / sample name
 * @return
 */
DSSIaudiooutput::ProgramDescriptor::ProgramDescriptor(unsigned long _bank,
                                                      unsigned long _program,
                                                      char *_name)
    :bank(_bank), program(_program), name(_name)
{}

/**
 * The map of programs available; held as a single shared statically allocated object.
 */
vector<DSSIaudiooutput::ProgramDescriptor> DSSIaudiooutput::programMap =
    vector<DSSIaudiooutput::ProgramDescriptor>();

/**
 * Index controlling the map of banks
 */
long DSSIaudiooutput::bankNoToMap = 1;

/**
 * Queries and maps the next available bank of instruments.
 *
 * If the program index requested to getProgram() lies beyond the banks mapped to date,
 * this member function is called to map the next one.
 * @return true if a new bank has been found and mapped, else false.
 */
bool DSSIaudiooutput::mapNextBank()
{
    pthread_mutex_lock(&master->mutex);
    Bank &bank = master->bank;
    bool  retval;
    if((bankNoToMap >= (int)bank.banks.size())
       || bank.banks[bankNoToMap].dir.empty())
        retval = false;
    else {
        bank.loadbank(bank.banks[bankNoToMap].dir);
        for(unsigned long instrument = 0; instrument < BANK_SIZE;
            ++instrument) {
            string insName = bank.getname(instrument);
            if(!insName.empty() && (insName[0] != '\0') && (insName[0] != ' '))
                programMap.push_back(ProgramDescriptor(bankNoToMap, instrument,
                                                       const_cast<char *>(
                                                           insName.c_str())));
        }
        bankNoToMap++;
        retval = true;
    }
    pthread_mutex_unlock(&master->mutex);
    return retval;
}
