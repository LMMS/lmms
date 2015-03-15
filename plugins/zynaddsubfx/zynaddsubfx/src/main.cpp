/*
  ZynAddSubFX - a software synthesizer

  main.cpp  -  Main file of the synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2012-2014 Mark McCurry

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


#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <signal.h>

#include <unistd.h>
#include <pthread.h>

#include <getopt.h>

#include <lo/lo.h>
#include <rtosc/ports.h>
#include <rtosc/thread-link.h>
#include "Params/PADnoteParameters.h"

#include "DSP/FFTwrapper.h"
#include "Misc/PresetExtractor.h"
#include "Misc/Master.h"
#include "Misc/Part.h"
#include "Misc/Util.h"
#include "Misc/Dump.h"
extern Dump dump;

//Nio System
#include "Nio/Nio.h"

//GUI System
#include "UI/Connection.h"
GUI::ui_handle_t gui;

//Glue Layer
#include "Misc/MiddleWare.h"
MiddleWare *middleware;

using namespace std;

Master   *master;
SYNTH_T  *synth;
int       swaplr = 0; //1 for left-right swapping

int Pexitprogram = 0;     //if the UI set this to 1, the program will exit

#if LASH
#include "Misc/LASHClient.h"
LASHClient *lash = NULL;
#endif

#if USE_NSM
#include "UI/NSM.H"

NSM_Client *nsm = 0;
#endif

char *instance_name = 0;

void exitprogram();

extern pthread_t main_thread;

//cleanup on signaled exit
void sigterm_exit(int /*sig*/)
{
    if(Pexitprogram)
        exit(1);
    Pexitprogram = 1;
}

/*
 * Program initialisation
 */
void initprogram(void)
{


    middleware = new MiddleWare();
    master = middleware->spawnMaster();
    master->swaplr = swaplr;

    signal(SIGINT, sigterm_exit);
    signal(SIGTERM, sigterm_exit);
    Nio::init(master);
}

/*
 * Program exit
 */
void exitprogram()
{
    Nio::stop();

    GUI::destroyUi(gui);
    delete middleware;
#if LASH
    if(lash)
        delete lash;
#endif
#if USE_NSM
    if(nsm)
        delete nsm;
#endif

    delete [] denormalkillbuf;
    FFT_cleanup();
}

int main(int argc, char *argv[])
{
    main_thread = pthread_self();
    synth = new SYNTH_T;
    config.init();
    dump.startnow();
    int noui = 0;
    cerr
    << "\nZynAddSubFX - Copyright (c) 2002-2013 Nasca Octavian Paul and others"
    << endl;
    cerr
    << "                Copyright (c) 2009-2014 Mark McCurry [active maintainer]"
    << endl;
    cerr << "Compiled: " << __DATE__ << " " << __TIME__ << endl;
    cerr << "This program is free software (GNU GPL v.2 or later) and \n";
    cerr << "it comes with ABSOLUTELY NO WARRANTY.\n" << endl;
    if(argc == 1)
        cerr << "Try 'zynaddsubfx --help' for command-line options." << endl;

    /* Get the settings from the Config*/
    synth->samplerate = config.cfg.SampleRate;
    synth->buffersize = config.cfg.SoundBufferSize;
    synth->oscilsize  = config.cfg.OscilSize;
    swaplr = config.cfg.SwapStereo;

    Nio::preferedSampleRate(synth->samplerate);

    synth->alias(); //build aliases

    sprng(time(NULL));

    /* Parse command-line options */
    struct option opts[] = {
        {
            "load", 2, NULL, 'l'
        },
        {
            "load-instrument", 2, NULL, 'L'
        },
        {
            "sample-rate", 2, NULL, 'r'
        },
        {
            "buffer-size", 2, NULL, 'b'
        },
        {
            "oscil-size", 2, NULL, 'o'
        },
        {
            "dump", 2, NULL, 'D'
        },
        {
            "swap", 2, NULL, 'S'
        },
        {
            "no-gui", 2, NULL, 'U'
        },
        {
            "dummy", 2, NULL, 'Y'
        },
        {
            "help", 2, NULL, 'h'
        },
        {
            "version", 2, NULL, 'v'
        },
        {
            "named", 1, NULL, 'N'
        },
        {
            "auto-connect", 0, NULL, 'a'
        },
        {
            "output", 1, NULL, 'O'
        },
        {
            "input", 1, NULL, 'I'
        },
        {
            "exec-after-init", 1, NULL, 'e'
        },
        {
            "dump-oscdoc", 2, NULL, 'd'
        },
        {
            0, 0, 0, 0
        }
    };
    opterr = 0;
    int option_index = 0, opt, exitwithhelp = 0, exitwithversion = 0;

    string loadfile, loadinstrument, execAfterInit;

    while(1) {
        int tmp = 0;

        /**\todo check this process for a small memory leak*/
        opt = getopt_long(argc,
                          argv,
                          "l:L:r:b:o:I:O:N:e:hvaSDUY",
                          opts,
                          &option_index);
        char *optarguments = optarg;

#define GETOP(x) if(optarguments) \
        x = optarguments
#define GETOPNUM(x) if(optarguments) \
        x = atoi(optarguments)


        if(opt == -1)
            break;

        switch(opt) {
            case 'h':
                exitwithhelp = 1;
                break;
            case 'v':
                exitwithversion = 1;
                break;
            case 'Y': /* this command a dummy command (has NO effect)
                        and is used because I need for NSIS installer
                        (NSIS sometimes forces a command line for a
                        program, even if I don't need that; eg. when
                        I want to add a icon to a shortcut.
                     */
                break;
            case 'U':
                noui = 1;
                break;
            case 'l':
                GETOP(loadfile);
                break;
            case 'L':
                GETOP(loadinstrument);
                break;
            case 'r':
                GETOPNUM(synth->samplerate);
                if(synth->samplerate < 4000) {
                    cerr << "ERROR:Incorrect sample rate: " << optarguments
                         << endl;
                    exit(1);
                }
                break;
            case 'b':
                GETOPNUM(synth->buffersize);
                if(synth->buffersize < 2) {
                    cerr << "ERROR:Incorrect buffer size: " << optarguments
                         << endl;
                    exit(1);
                }
                break;
            case 'o':
                if(optarguments)
                    synth->oscilsize = tmp = atoi(optarguments);
                if(synth->oscilsize < MAX_AD_HARMONICS * 2)
                    synth->oscilsize = MAX_AD_HARMONICS * 2;
                synth->oscilsize =
                    (int) powf(2,
                               ceil(logf(synth->oscilsize - 1.0f) / logf(2.0f)));
                if(tmp != synth->oscilsize)
                    cerr
                    <<
                    "synth->oscilsize is wrong (must be 2^n) or too small. Adjusting to "
                    << synth->oscilsize << "." << endl;
                break;
            case 'S':
                swaplr = 1;
                break;
            case 'D':
                dump.startnow();
                break;
            case 'N':
                Nio::setPostfix(optarguments);
                break;
            case 'I':
                if(optarguments)
                    Nio::setDefaultSource(optarguments);
                break;
            case 'O':
                if(optarguments)
                    Nio::setDefaultSink(optarguments);
                break;
            case 'a':
                Nio::autoConnect = true;
                break;
            case 'e':
                GETOP(execAfterInit);
                break;
            case 'd':
                if(optarguments)
                {
                    rtosc::OscDocFormatter s;
                    ofstream outfile(optarguments);
                    s.prog_name    = "ZynAddSubFX";
                    s.p            = &Master::ports;
                    s.uri          = "http://example.com/fake/";
                    s.doc_origin   = "http://example.com/fake/url.xml";
                    s.author_first = "Mark";
                    s.author_last  = "McCurry";
                    outfile << s;
                }
                break;
            case '?':
                cerr << "ERROR:Bad option or parameter.\n" << endl;
                exitwithhelp = 1;
                break;
        }
    }

    synth->alias();

    if(exitwithversion) {
        cout << "Version: " << VERSION << endl;
        return 0;
    }
    if(exitwithhelp != 0) {
        cout << "Usage: zynaddsubfx [OPTION]\n\n"
             << "  -h , --help \t\t\t\t Display command-line help and exit\n"
             << "  -v , --version \t\t\t Display version and exit\n"
             << "  -l file, --load=FILE\t\t\t Loads a .xmz file\n"
             << "  -L file, --load-instrument=FILE\t Loads a .xiz file\n"
             << "  -r SR, --sample-rate=SR\t\t Set the sample rate SR\n"
             <<
        "  -b BS, --buffer-size=SR\t\t Set the buffer size (granularity)\n"
             << "  -o OS, --oscil-size=OS\t\t Set the ADsynth oscil. size\n"
             << "  -S , --swap\t\t\t\t Swap Left <--> Right\n"
             << "  -D , --dump\t\t\t\t Dumps midi note ON/OFF commands\n"
             <<
        "  -U , --no-gui\t\t\t\t Run ZynAddSubFX without user interface\n"
             << "  -N , --named\t\t\t\t Postfix IO Name when possible\n"
             << "  -a , --auto-connect\t\t\t AutoConnect when using JACK\n"
             << "  -O , --output\t\t\t\t Set Output Engine\n"
             << "  -I , --input\t\t\t\t Set Input Engine\n"
             << "  -e , --exec-after-init\t\t Run post-initialization script\n"
             << "  -d , --dump-oscdoc=FILE\t\t Dump oscdoc xml to file\n"
             << endl;

        return 0;
    }

    //produce denormal buf
    denormalkillbuf = new float [synth->buffersize];
    for(int i = 0; i < synth->buffersize; ++i)
        denormalkillbuf[i] = (RND - 0.5f) * 1e-16;

    initprogram();

    if(!loadfile.empty()) {
        int tmp = master->loadXML(loadfile.c_str());
        if(tmp < 0) {
            cerr << "ERROR: Could not load master file " << loadfile
                 << "." << endl;
            exit(1);
        }
        else {
            master->applyparameters();
            cout << "Master file loaded." << endl;
        }
    }

    if(!loadinstrument.empty()) {
        int loadtopart = 0;
        int tmp = master->part[loadtopart]->loadXMLinstrument(
            loadinstrument.c_str());
        if(tmp < 0) {
            cerr << "ERROR: Could not load instrument file "
                 << loadinstrument << '.' << endl;
            exit(1);
        }
        else {
            master->part[loadtopart]->applyparameters();
            master->part[loadtopart]->initialize_rt();
            cout << "Instrument file loaded." << endl;
        }
    }

    //Run the Nio system
    bool ioGood = Nio::start();
    
    cerr.precision(1);
    cerr << std::fixed;
    cerr << "\nSample Rate = \t\t" << synth->samplerate << endl;
    cerr << "Sound Buffer Size = \t" << synth->buffersize << " samples" << endl;
    cerr << "Internal latency = \t" << synth->buffersize_f * 1000.0f
    / synth->samplerate_f << " ms" << endl;
    cerr << "ADsynth Oscil.Size = \t" << synth->oscilsize << " samples" << endl;

    if(!execAfterInit.empty()) {
        cout << "Executing user supplied command: " << execAfterInit << endl;
        if(system(execAfterInit.c_str()) == -1)
            cerr << "Command Failed..." << endl;
    }


    gui = GUI::createUi(middleware->spawnUiApi(), &Pexitprogram);
    middleware->setUiCallback(GUI::raiseUi, gui);
    middleware->setIdleCallback([](){GUI::tickUi(gui);});
    middlewarepointer = middleware;

    if(!noui)
    {
        GUI::raiseUi(gui, "/show",  "T");
        if(!ioGood)
            GUI::raiseUi(gui, "/alert", "s",
                    "Default IO did not initialize.\nDefaulting to NULL backend.");
    }

#if USE_NSM
    char *nsm_url = getenv("NSM_URL");

    if(nsm_url) {
        nsm = new NSM_Client;

        if(!nsm->init(nsm_url))
            nsm->announce("ZynAddSubFX", ":switch:", argv[0]);
        else {
            delete nsm;
            nsm = NULL;
        }
    }
#endif

#if USE_NSM
    if(!nsm)
#endif
    {
#if LASH
        lash = new LASHClient(&argc, &argv);
        GUI::raiseUi(gui, "/session-type", "s", "LASH");
#endif
    }

    while(Pexitprogram == 0) {
#if USE_NSM
        if(nsm) {
            nsm->check();
            goto done;
        }
#endif
#if LASH
        {
            string filename;
            switch(lash->checkevents(filename)) {
                case LASHClient::Save:
                    GUI::raiseUi(gui, "/save-master", "s", filename.c_str());
                    lash->confirmevent(LASHClient::Save);
                    break;
                case LASHClient::Restore:
                    GUI::raiseUi(gui, "/load-master", "s", filename.c_str());
                    lash->confirmevent(LASHClient::Restore);
                    break;
                case LASHClient::Quit:
                    Pexitprogram = 1;
                default:
                    break;
            }
        }
#endif //LASH

#if USE_NSM
done:
#endif
        GUI::tickUi(gui);
        middleware->tick();
    }

    exitprogram();
    return 0;
}
