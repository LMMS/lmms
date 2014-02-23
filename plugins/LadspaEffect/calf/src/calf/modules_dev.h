/* Calf DSP Library
 * Prototype audio modules
 *
 * Copyright (C) 2008 Thor Harald Johansen <thj@thj.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_MODULES_DEV_H
#define __CALF_MODULES_DEV_H

#include <calf/metadata.h>

#if ENABLE_EXPERIMENTAL
#include <fluidsynth.h>
#endif

namespace calf_plugins {

#if ENABLE_EXPERIMENTAL
    
/// Tiny wrapper for fluidsynth
class fluidsynth_audio_module: public audio_module<fluidsynth_metadata>
{
protected:
    /// Current sample rate
    uint32_t srate;
    /// FluidSynth Settings object
    fluid_settings_t *settings;
    /// FluidSynth Synth object
    fluid_synth_t *synth;
    /// Soundfont filename
    std::string soundfont;
    /// Soundfont filename (as received from Fluidsynth)
    std::string soundfont_name;
    /// TAB-separated preset list (preset+128*bank TAB preset name LF)
    std::string soundfont_preset_list;
    /// FluidSynth assigned SoundFont ID
    int sfid;
    /// Map of preset+128*bank to preset name
    std::map<uint32_t, std::string> sf_preset_names;
    /// Last selected preset+128*bank
    uint32_t last_selected_preset;
    /// Serial number of status data
    int status_serial;
    /// Preset number to set on next process() call
    volatile int set_preset;

    /// Update last_selected_preset based on synth object state
    void update_preset_num();
    /// Create a fluidsynth object and load the current soundfont
    fluid_synth_t *create_synth(int &new_sfid);
public:
    /// Constructor to initialize handles to NULL
    fluidsynth_audio_module();

    void post_instantiate();
    void set_sample_rate(uint32_t sr) { srate = sr; }
    /// Handle MIDI Note On message (by sending it to fluidsynth)
    void note_on(int channel, int note, int vel);
    /// Handle MIDI Note Off message (by sending it to fluidsynth)
    void note_off(int channel, int note, int vel);
    /// Handle pitch bend message.
    inline void pitch_bend(int channel, int value)
    {
        fluid_synth_pitch_bend(synth, 0, value + 0x2000);
    }
    /// Handle control change messages.
    void control_change(int channel, int controller, int value);
    /// Handle program change messages.
    void program_change(int channel, int program);

    /// Update variables from control ports.
    void params_changed() {
    }
    void activate();
    void deactivate();
    /// No CV inputs for now
    bool is_cv(int param_no) { return false; }
    /// Practically all the stuff here is noisy... for now
    bool is_noisy(int param_no) { return true; }
    /// Main processing function
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    /// DSSI-style configure function for handling string port data
    char *configure(const char *key, const char *value);
    void send_configures(send_configure_iface *sci);
    int send_status_updates(send_updates_iface *sui, int last_serial);
    uint32_t message_run(const void *valid_inputs, void *output_ports) { 
        // silence a default printf (which is kind of a warning about unhandled message_run)
        return 0;
    }
    ~fluidsynth_audio_module();
};


    
#endif
    
};

#endif
