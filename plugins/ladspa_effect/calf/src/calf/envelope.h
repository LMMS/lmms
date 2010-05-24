/* Calf DSP Library
 * ADSR envelope class (and other envelopes in future)
 *
 * Copyright (C) 2007-2008 Krzysztof Foltman
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
 * Boston, MA  02110-1301  USA
 */
#ifndef __CALF_ENVELOPE_H
#define __CALF_ENVELOPE_H

#include "primitives.h"

namespace dsp {

/// Rate-based ADSFR envelope class. Note that if release rate is slower than decay
/// rate, this envelope won't use release rate until output level falls below sustain level
/// it's different to what certain hardware synth companies did, but it prevents the very
/// un-musical (IMHO) behaviour known from (for example) SoundFont 2.
class adsr
{
public:
    enum env_state { 
        STOP, ///< envelope is stopped
        ATTACK, ///< attack - rise from 0 to 1
        DECAY, ///< decay - fall from 1 to sustain level
        SUSTAIN, ///< sustain - remain at sustain level (unless sustain is 0 - then it gets stopped); with fade != 0 it goes towards 0% (positive fade) or 100% (negative fade)
        RELEASE, ///< release - fall from sustain (or pre-sustain) level to 0
        LOCKDECAY,  ///< locked decay 
    };
    
    /// Current envelope stage
    env_state state;
    /// @note these are *rates*, not times
    double attack, decay, sustain, release, fade;
    /// Requested release time (not the rate!) in frames, used for recalculating the rate if sustain is changed
    double release_time;
    /// Current envelope (output) level
    double value;
    /// Release rate used for the current note (calculated from this note's sustain level, and not the current sustain level,
    /// which may have changed after note has been released)
    double thisrelease;
    /// Sustain level used for the current note (used to calculate release rate if sustain changed during release stage
    /// of the current note)
    double thiss;
    /// Value from the time before advance() was called last time
    double old_value;
    
    adsr()
    {
        attack = decay = sustain = release = thisrelease = thiss = 0.f;
        reset();
    }
    /// Stop (reset) the envelope
    inline void reset()
    {
        old_value = value = 0.0;
        thiss = 0.0;
        state = STOP;
    }
    /// Set the envelope parameters (updates rate member variables based on values passed)
    /// @param a attack time
    /// @param d decay time
    /// @param s sustain level
    /// @param r release time
    /// @param er Envelope (update) rate
    /// @param f fade time (if applicable)
    inline void set(float a, float d, float s, float r, float er, float f = 0.f)
    {
        attack = 1.0 / (a * er);
        decay = (1 - s) / (d * er);
        sustain = s;
        release_time = r * er;
        release = s / release_time;
        if (fabs(f) > small_value<float>())
            fade = 1.0 / (f * er);
        else
            fade = 0.0;
        // in release:
        // lock thiss setting (start of release for current note) and unlock thisrelease setting (current note's release rate)
        if (state != RELEASE)
            thiss = s;
        else
            thisrelease = thiss / release_time;
    }
    /// @retval true if envelope is in released state (forced decay, release or stopped)
    inline bool released() const
    {
        return state == LOCKDECAY || state == RELEASE || state == STOP;
    }
    /// @retval true if envelope is stopped (has not been started or has run till its end)
    inline bool stopped() const
    {
        return state == STOP;
    }
    /// Start the envelope
    inline void note_on()
    {
        state = ATTACK;
        thiss = sustain;
    }
    /// Release the envelope
    inline void note_off()
    {
        // Do nothing if envelope is already stopped
        if (state == STOP)
            return;
        // XXXKF what if envelope is already released? (doesn't happen in any current synth, but who knows?)
        // Raise sustain value if it has been changed... I'm not sure if it's needed
        thiss = std::max(sustain, value);
        // Calculate release rate from sustain level
        thisrelease = thiss / release_time;
        // we're in attack or decay, and if decay is faster than release
        if (value > sustain && decay > thisrelease) {
            // use standard release time later (because we'll be switching at sustain point)
            thisrelease = release;
            state = LOCKDECAY;
        } else {
            // in attack/decay, but use fixed release time
            // in case value fell below sustain, assume it didn't (for the purpose of calculating release rate only)
            state = RELEASE;
        }
    }
    /// Calculate next envelope value
    inline void advance()
    {
        old_value = value;
        // XXXKF This may use a state array instead of a switch some day (at least for phases other than attack and possibly sustain)
        switch(state)
        {
        case ATTACK:
            value += attack;
            if (value >= 1.0) {
                value = 1.0;
                state = DECAY;
            }
            break;
        case DECAY:
            value -= decay;
            if (value < sustain)
            {
                value = sustain;
                state = SUSTAIN;
            }
            break;
        case LOCKDECAY:
            value -= decay;
            if (value < sustain)
            {
                if (value < 0.f)
                    value = 0.f;
                state = RELEASE;
                thisrelease = release;
            }
            break;
        case SUSTAIN:
            if (fade != 0.f)
            {
                value -= fade;
                if (value > 1.f)
                    value = 1.f;
            }
            else
                value = sustain;
            if (value < 0.00001f) {
                value = 0;
                state = STOP;
            }
            break;
        case RELEASE:
            value -= thisrelease;
            if (value <= 0.f) {
                value = 0.f;
                state = STOP;
            }
            break;
        case STOP:
            value = 0.f;
            break;
        }
    }
    /// Return a value between old_value (previous step) and value (current step)
    /// @param pos between 0 and 1
    inline double interpolate(double pos)
    {
        return old_value + (value - old_value) * pos;
    }
    inline float get_amp_value()
    {
        if (state == RELEASE && sustain > 0 && value < sustain)
        {
            return value * value * value / (sustain * sustain);
        }
        return value;
    }
};

/// Simple linear fade out for note tails
struct fadeout
{
    float value;
    float step, step_orig;
    bool done, undoing;
    
    fadeout(int steps = 256)
    {
        step_orig = (float)(1.f / steps);
        value = 1.f;
        reset();
    }
    
    /// Prepare fade out
    void reset()
    {
        value = 1.f;
        step = -step_orig;
        done = false;
        undoing = false;
    }
    
    /// Fade back in with double speed (to prevent click on note restart)
    void undo()
    {
        step = step_orig;
        done = false;
        undoing = true;
    }
    
    /// Reset if fully faded out; fade back in if in the middle of fading out
    void reset_soft()
    {
        if (value <= 0.f || value >= 1.f)
            reset();
        else
            undo();
    }
    
    void process(float *buffer, int len)
    {
        int i = 0;
        if (!done)
        {
            for (; value > 0 && value <= 1.0 && i < len; i++)
            {
                buffer[i] *= value;
                value += step;
            }
            if (value <= 0 || value > 1)
                done = true;
        }
        if (done && value <= 0)
        {
            while (i < len)
                buffer[i++] = 0.f;
        }
        if (done && undoing && value >= 1)
        {
            undoing = false;
            done = false;
            // prepare for the next fade-out
            value = 1.f;
        }
    }
};

};

#endif


