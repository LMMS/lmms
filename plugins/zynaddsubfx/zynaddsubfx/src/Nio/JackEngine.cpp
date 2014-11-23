/*
    JackEngine.cpp

    Copyright 2009, Alan Calvert

    This file is part of yoshimi, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    yoshimi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

#include <jack/midiport.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cassert>
#include <cstring>

#include "Nio.h"
#include "InMgr.h"

#include "JackEngine.h"

using namespace std;

extern char *instance_name;

JackEngine::JackEngine()
    :AudioOut(), jackClient(NULL)
{
    name = "JACK";
    audio.jackSamplerate = 0;
    audio.jackNframes    = 0;
    for(int i = 0; i < 2; ++i) {
        audio.ports[i]     = NULL;
        audio.portBuffs[i] = NULL;
    }
    midi.inport = NULL;
    midi.jack_sync = false;
}

bool JackEngine::connectServer(string server)
{
    bool autostart_jack = true;
    if(jackClient)
        return true;

    string clientname = "zynaddsubfx";
    string postfix    = Nio::getPostfix();
    if(!postfix.empty())
        clientname += "_" + postfix;
    jack_status_t jackstatus;
    bool use_server_name = server.size() && server.compare("default") != 0;
    jack_options_t jopts = (jack_options_t)
                           (((!instance_name
                              && use_server_name) ? JackServerName :
                             JackNullOption)
                            | ((autostart_jack) ? JackNullOption :
                               JackNoStartServer));

    if(instance_name)
        jackClient = jack_client_open(instance_name, jopts, &jackstatus);
    else {
        if(use_server_name)
            jackClient = jack_client_open(
                clientname.c_str(), jopts, &jackstatus,
                server.c_str());
        else
            jackClient = jack_client_open(
                clientname.c_str(), jopts, &jackstatus);
    }


    if(NULL != jackClient)
        return true;
    else
        cerr << "Error, failed to open jack client on server: " << server
             << " status " << jackstatus << endl;
    return false;
}

bool JackEngine::connectJack()
{
    connectServer("");
    if(NULL != jackClient) {
        setBufferSize(jack_get_buffer_size(jackClient));
        int chk;
        jack_set_error_function(_errorCallback);
        jack_set_info_function(_infoCallback);
        if(jack_set_buffer_size_callback(jackClient, _bufferSizeCallback, this))
            cerr << "Error setting the bufferSize callback" << endl;
        if((chk = jack_set_xrun_callback(jackClient, _xrunCallback, this)))
            cerr << "Error setting jack xrun callback" << endl;
        if(jack_set_process_callback(jackClient, _processCallback, this)) {
            cerr << "Error, JackEngine failed to set process callback" << endl;
            return false;
        }
        if(jack_activate(jackClient)) {
            cerr << "Error, failed to activate jack client" << endl;
            return false;
        }

        return true;
    }
    else
        cerr << "Error, NULL jackClient through Start()" << endl;
    return false;
}

void JackEngine::disconnectJack()
{
    if(jackClient) {
        cout << "Deactivating and closing JACK client" << endl;

        jack_deactivate(jackClient);
        jack_client_close(jackClient);
        jackClient = NULL;
    }
}

bool JackEngine::Start()
{
    return openMidi() && openAudio();
}

void JackEngine::Stop()
{
    stopMidi();
    stopAudio();
}

void JackEngine::setMidiEn(bool nval)
{
    if(nval)
        openMidi();
    else
        stopMidi();
}

bool JackEngine::getMidiEn() const
{
    return midi.inport;
}

void JackEngine::setAudioEn(bool nval)
{
    if(nval)
        openAudio();
    else
        stopAudio();
}

bool JackEngine::getAudioEn() const
{
    return audio.ports[0];
}

bool JackEngine::openAudio()
{
    if(getAudioEn())
        return true;

    if(!getMidiEn())
        if(!connectJack())
            return false;


    const char *portnames[] = { "out_1", "out_2" };
    for(int port = 0; port < 2; ++port)
        audio.ports[port] = jack_port_register(
            jackClient,
            portnames[port],
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsOutput
            | JackPortIsTerminal,
            0);
    if((NULL != audio.ports[0]) && (NULL != audio.ports[1])) {
        audio.jackSamplerate = jack_get_sample_rate(jackClient);
        audio.jackNframes    = jack_get_buffer_size(jackClient);
        samplerate = audio.jackSamplerate;
        bufferSize = audio.jackNframes;


        //Attempt to autoConnect when specified
        if(Nio::autoConnect) {
            const char **outPorts = jack_get_ports(
                jackClient,
                NULL,
                NULL,
                JackPortIsPhysical
                | JackPortIsInput);
            if(outPorts != NULL) {
                //Verify that stereo is available
                assert(outPorts[0]);
                assert(outPorts[1]);

                //Connect to physical outputs
                jack_connect(jackClient, jack_port_name(
                                 audio.ports[0]), outPorts[0]);
                jack_connect(jackClient, jack_port_name(
                                 audio.ports[1]), outPorts[1]);
            }
            else
                cerr << "Warning, No outputs to autoconnect to" << endl;
        }
        midi.jack_sync = true;
        return true;
    }
    else
        cerr << "Error, failed to register jack audio ports" << endl;
    midi.jack_sync = false;
    return false;
}

void JackEngine::stopAudio()
{
    for(int i = 0; i < 2; ++i) {
        jack_port_t *port = audio.ports[i];
        audio.ports[i] = NULL;
        if(NULL != port)
            jack_port_unregister(jackClient, port);
    }
    midi.jack_sync = false;
    if(!getMidiEn())
        disconnectJack();
}

bool JackEngine::openMidi()
{
    if(getMidiEn())
        return true;
    if(!getAudioEn())
        if(!connectJack())
            return false;

    midi.inport = jack_port_register(jackClient, "midi_input",
                                     JACK_DEFAULT_MIDI_TYPE,
                                     JackPortIsInput | JackPortIsTerminal, 0);
    return midi.inport;
}

void JackEngine::stopMidi()
{
    jack_port_t *port = midi.inport;
    midi.inport = NULL;
    if(port)
        jack_port_unregister(jackClient, port);

    if(!getAudioEn())
        disconnectJack();
}

int JackEngine::clientId()
{
    if(NULL != jackClient)
        return (long)jack_client_thread_id(jackClient);
    else
        return -1;
}

string JackEngine::clientName()
{
    if(NULL != jackClient)
        return string(jack_get_client_name(jackClient));
    else
        cerr << "Error, clientName() with null jackClient" << endl;
    return string("Oh, yoshimi :-(");
}

int JackEngine::_processCallback(jack_nframes_t nframes, void *arg)
{
    return static_cast<JackEngine *>(arg)->processCallback(nframes);
}

int JackEngine::processCallback(jack_nframes_t nframes)
{
    bool okaudio = true;

    handleMidi(nframes);
    if((NULL != audio.ports[0]) && (NULL != audio.ports[1]))
        okaudio = processAudio(nframes);
    return okaudio ? 0 : -1;
}

bool JackEngine::processAudio(jack_nframes_t nframes)
{
    for(int port = 0; port < 2; ++port) {
        audio.portBuffs[port] =
            (jsample_t *)jack_port_get_buffer(audio.ports[port], nframes);
        if(NULL == audio.portBuffs[port]) {
            cerr << "Error, failed to get jack audio port buffer: "
                 << port << endl;
            return false;
        }
    }

    Stereo<float *> smp = getNext();

    //Assumes size of smp.l == nframes
    memcpy(audio.portBuffs[0], smp.l, bufferSize * sizeof(float));
    memcpy(audio.portBuffs[1], smp.r, bufferSize * sizeof(float));
    return true;
}

int JackEngine::_xrunCallback(void *)
{
    cerr << "Jack reports xrun" << endl;
    return 0;
}

void JackEngine::_errorCallback(const char *msg)
{
    cerr << "Jack reports error: " << msg << endl;
}

void JackEngine::_infoCallback(const char *msg)
{
    cerr << "Jack info message: " << msg << endl;
}

int JackEngine::_bufferSizeCallback(jack_nframes_t nframes, void *arg)
{
    return static_cast<JackEngine *>(arg)->bufferSizeCallback(nframes);
}

int JackEngine::bufferSizeCallback(jack_nframes_t nframes)
{
    cerr << "Jack buffer resized" << endl;
    setBufferSize(nframes);
    return 0;
}

void JackEngine::handleMidi(unsigned long frames)
{
    if(!midi.inport)
        return;
    void *midi_buf = jack_port_get_buffer(midi.inport, frames);
    jack_midi_event_t jack_midi_event;
    jack_nframes_t    event_index = 0;
    unsigned char    *midi_data;
    unsigned char     type;

    while(jack_midi_event_get(&jack_midi_event, midi_buf,
                              event_index++) == 0) {
        MidiEvent ev;
        midi_data  = jack_midi_event.buffer;
        type       = midi_data[0] & 0xF0;
        ev.channel = midi_data[0] & 0x0F;
        ev.time    = midi.jack_sync ? jack_midi_event.time : 0;

        switch(type) {
            case 0x80: /* note-off */
                ev.type  = M_NOTE;
                ev.num   = midi_data[1];
                ev.value = 0;
                InMgr::getInstance().putEvent(ev);
                break;

            case 0x90: /* note-on */
                ev.type  = M_NOTE;
                ev.num   = midi_data[1];
                ev.value = midi_data[2];
                InMgr::getInstance().putEvent(ev);
                break;

            case 0xA0: /* pressure, aftertouch */
                ev.type  = M_PRESSURE;
                ev.num   = midi_data[1];
                ev.value = midi_data[2];
                InMgr::getInstance().putEvent(ev);
                break;

            case 0xB0: /* controller */
                ev.type  = M_CONTROLLER;
                ev.num   = midi_data[1];
                ev.value = midi_data[2];
                InMgr::getInstance().putEvent(ev);
                break;

            case 0xC0: /* program change */
                ev.type  = M_PGMCHANGE;
                ev.num   = midi_data[1];
                ev.value = 0;
                InMgr::getInstance().putEvent(ev);
                break;

            case 0xE0: /* pitch bend */
                ev.type  = M_CONTROLLER;
                ev.num   = C_pitchwheel;
                ev.value = ((midi_data[2] << 7) | midi_data[1]) - 8192;
                InMgr::getInstance().putEvent(ev);
                break;

                /* XXX TODO: handle MSB/LSB controllers and RPNs and NRPNs */
        }
    }
}
