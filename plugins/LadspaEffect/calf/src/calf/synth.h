/* Calf DSP Library
 * Framework for synthesizer-like plugins. This is based
 * on my earlier work on Drawbar electric organ emulator.
 *
 * Copyright (C) 2007 Krzysztof Foltman
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
#ifndef CALF_SYNTH_H
#define CALF_SYNTH_H

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdint.h>
#include <bitset>
#include <list>
#include <stack>

namespace dsp {

/**
 * A kind of set with fast non-ordered iteration, used for storing lists of pressed keys.
 */
class keystack {
private:
    int dcount;
    uint8_t active[128];
    uint8_t states[128];
public:
    keystack() {
        memset(states, 0xFF, sizeof(states));
        dcount = 0;
    }
    void clear() {
        for (int i=0; i<dcount; i++)
            states[active[i]] = 0xFF;
        dcount = 0;
    }
    bool push(int key) {
        assert(key >= 0 && key <= 127);
        if (states[key] != 0xFF) {
            return true;
        }
        states[key] = dcount;
        active[dcount++] = key;
        return false;
    }
    bool pop(int key) {
        if (states[key] == 0xFF)
            return false;
        int pos = states[key];
        if (pos != dcount-1) {
            // reuse the popped item's stack position for stack top
            int last = active[dcount-1];
            active[pos] = last;
            // mark that position's new place on stack
            states[last] = pos;
        }
        states[key] = 0xFF;
        dcount--;
        return true;
    }    
    inline bool has(int key) {
        return states[key] != 0xFF;
    }
    inline int count() {
        return dcount;
    }
    inline bool empty() {
        return (dcount == 0);
    }
    inline int nth(int n) {
        return active[n];
    }
};

/**
 * Convert MIDI note number to normalized UINT phase (where 1<<32 is full cycle).
 * @param MIDI note number
 * @param cents detune in cents (1/100 of a semitone)
 * @param sr sample rate
 */
inline unsigned int midi_note_to_phase(int note, double cents, int sr) {
    double incphase = 440*pow(2.0, (note-69)/12.0 + cents/1200.0)/sr;
    if (incphase >= 1.0) incphase = fmod(incphase, 1.0);
    incphase *= 65536.0*65536.0;
    return (unsigned int)incphase;
}

// Base class for all voice objects
class voice {
public:
    int sample_rate;
    bool released, sostenuto, stolen;

    voice() : sample_rate(-1), released(false), sostenuto(false), stolen(false) {}

    /// reset voice to default state (used when a voice is to be reused)
    virtual void setup(int sr) { sample_rate = sr; }
    /// reset voice to default state (used when a voice is to be reused)
    virtual void reset()=0;
    /// a note was pressed
    virtual void note_on(int note, int vel)=0;
    /// a note was released
    virtual void note_off(int vel)=0;
    /// check if voice can be removed from active voice list
    virtual bool get_active()=0;
    /// render voice data to buffer
    virtual void render_to(float (*buf)[2], int nsamples)=0;
    /// very fast note off
    virtual void steal()=0;
    /// return the note used by this voice
    virtual int get_current_note()=0;
    virtual float get_priority() { return stolen ? 20000 : (released ? 1 : (sostenuto ? 200 : 100)); }
    /// empty virtual destructor
    virtual ~voice() {}
};

/// An "optimized" voice class using fixed-size processing units
/// and fixed number of channels. The drawback is that voice
/// control is not sample-accurate, and no modulation input
/// is possible, but it should be good enough for most cases
/// (like Calf Organ).
template<class Base>
class block_voice: public Base {
public:
    // derived from Base
    // enum { Channels = 2 };
    using Base::Channels;
    // enum { BlockSize = 16 };
    using Base::BlockSize;
    // float output_buffer[BlockSize][Channels];
    using Base::output_buffer;
    // void render_block();
    using Base::render_block;
    unsigned int read_ptr;

    block_voice()
    {
        read_ptr = BlockSize;
    }
    virtual void reset()
    {
        Base::reset();
        read_ptr = BlockSize;
    }
    virtual void render_to(float (*buf)[2], int nsamples)
    {
        int p = 0;
        while(p < nsamples)
        {
            if (read_ptr == BlockSize) 
            {
                render_block();
                read_ptr = 0;
            }
            int ncopy = std::min<int>(BlockSize - read_ptr, nsamples - p);
            for (int i = 0; i < ncopy; i++)
                for (int c = 0; c < Channels; c++)
                    buf[p + i][c] += output_buffer[read_ptr + i][c];
            p += ncopy;
            read_ptr += ncopy;
        }
    }
};

/// Base class for all kinds of polyphonic instruments, provides
/// somewhat reasonable voice management, pedal support - and 
/// little else. It's implemented as a base class with virtual
/// functions, so there's some performance loss, but it shouldn't
/// be horrible.
/// @todo it would make sense to support all notes off controller too
struct basic_synth {
protected:
    /// Current sample rate
    int sample_rate;
    /// Hold pedal state
    bool hold;
    /// Sostenuto pedal state
    bool sostenuto;
    /// Voices currently playing
    std::list<dsp::voice *> active_voices;
    /// Voices allocated, but not used
    std::stack<dsp::voice *> unused_voices;
    /// Gate values for all 128 MIDI notes
    std::bitset<128> gate;
    /// Maximum allocated number of channels
    unsigned int polyphony_limit;

    void kill_note(int note, int vel, bool just_one);
public:
    virtual void setup(int sr) {
        sample_rate = sr;
        hold = false;
        sostenuto = false;
        polyphony_limit = (unsigned)-1;
    }
    virtual void trim_voices();
    virtual dsp::voice *give_voice();
    virtual dsp::voice *alloc_voice()=0;
    virtual dsp::voice *steal_voice();
    virtual void render_to(float (*output)[2], int nsamples);
    virtual void note_on(int note, int vel);
    virtual void percussion_note_on(int note, int vel) {}
    virtual void control_change(int ctl, int val);
    virtual void note_off(int note, int vel);
    /// amt = -8192 to 8191
    virtual void pitch_bend(int amt) {}
    virtual void on_pedal_release();
    virtual bool check_percussion() { return active_voices.empty(); }
    virtual ~basic_synth();
};

}

#endif
