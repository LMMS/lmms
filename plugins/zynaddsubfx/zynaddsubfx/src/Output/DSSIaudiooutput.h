/*
  ZynAddSubFX - a software synthesizer

  VSTaudiooutput.h - Audio output for VST
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
#ifndef VST_AUDIO_OUTPUT_H
#define VST_AUDIO_OUTPUT_H

#include <pthread.h>

#include "../globals.h"
#include "../Misc/Master.h"

#include <dssi.h>
#include <ladspa.h>
#include <vector>

class DSSIaudiooutput
{
    public:
        //
        // Static stubs for LADSPA member functions
        //
        static void stub_connectPort(LADSPA_Handle instance,
                                     unsigned long port,
                                     LADSPA_Data *data);
        static void stub_activate(LADSPA_Handle instance);
        static void stub_run(LADSPA_Handle instance, unsigned long sample_count);
        static void stub_deactivate(LADSPA_Handle Instance);
        static void stub_cleanup(LADSPA_Handle instance);

        //
        // Static stubs for DSSI member functions
        //
        static const DSSI_Program_Descriptor *stub_getProgram(
            LADSPA_Handle instance,
            unsigned long Index);
        static void stub_selectProgram(LADSPA_Handle instance,
                                       unsigned long bank,
                                       unsigned long program);
        static int stub_getMidiControllerForPort(LADSPA_Handle instance,
                                                 unsigned long port);
        static void stub_runSynth(LADSPA_Handle instance,
                                  unsigned long sample_count,
                                  snd_seq_event_t *events,
                                  unsigned long event_count);

        /*
         * LADSPA member functions
         */
        static LADSPA_Handle instantiate(const LADSPA_Descriptor *descriptor,
                                         unsigned long s_rate);
        void connectPort(unsigned long port, LADSPA_Data *data);
        void activate();
        void run(unsigned long sample_count);
        void deactivate();
        void cleanup();
        static const LADSPA_Descriptor *getLadspaDescriptor(unsigned long index);

        /*
         * DSSI member functions
         */
        const DSSI_Program_Descriptor *getProgram(unsigned long Index);
        void selectProgram(unsigned long bank, unsigned long program);
        int getMidiControllerForPort(unsigned long port);
        void runSynth(unsigned long sample_count,
                      snd_seq_event_t *events,
                      unsigned long event_count);
        static const DSSI_Descriptor *getDssiDescriptor(unsigned long index);

        struct ProgramDescriptor {
            unsigned long bank;
            unsigned long program;
            std::string   name;
            ProgramDescriptor(unsigned long _bank,
                              unsigned long _program,
                              char *_name);
        };

    private:

        DSSIaudiooutput(unsigned long sampleRate);
        ~DSSIaudiooutput();
        static DSSI_Descriptor *initDssiDescriptor();
        static DSSIaudiooutput *getInstance(LADSPA_Handle instance);
        void initBanks();
        bool mapNextBank();

        LADSPA_Data *outl;
        LADSPA_Data *outr;
        long    sampleRate;
        Master *master;
        static DSSI_Descriptor *dssiDescriptor;
        static std::string      bankDirNames[];
        static
        std::vector<ProgramDescriptor> programMap;

        /**
         * Flag controlling the list of bank directories
         */
        bool banksInited;

        static
        long bankNoToMap;
};

#endif
