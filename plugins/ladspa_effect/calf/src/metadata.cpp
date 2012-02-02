/* Calf DSP Library
 * Example audio modules - parameters and LADSPA wrapper instantiation
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
#include <config.h>
#include <calf/giface.h>
#include <calf/metadata.h>
#include <calf/audio_fx.h>
#include <calf/modmatrix.h>

using namespace dsp;
using namespace calf_plugins;

const char *calf_plugins::calf_copyright_info = "(C) 2001-2009 Krzysztof Foltman, Thor Harald Johanssen, Markus Schmidt and others; license: LGPL";

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(flanger) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(flanger) = {
    { 0.1,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC | PF_PROP_GRAPH, NULL, "min_delay", "Min delay" },
    { 0.5,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "mod_depth", "Mod depth" },
    { 0.25,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Mod rate" },
    { 0.90,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 0,          0, 360,   9, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 1,          0, 4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 1.0,        0, 4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    {}
};

CALF_PLUGIN_INFO(flanger) = { 0x847d, "Flanger", "Calf Flanger", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "FlangerPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(phaser) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(phaser) = {
    { 1000,      20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "base_freq", "Center Freq" },
    { 4000,       0, 10800,  0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "mod_depth", "Mod depth" },
    { 0.25,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Mod rate" },
    { 0.25,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 6,          1, 12,   12, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "stages", "# Stages" },
    { 180,        0, 360,   9, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 1,          0, 4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 1.0,        0, 4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
};

CALF_PLUGIN_INFO(phaser) = { 0x8484, "Phaser", "Calf Phaser", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "PhaserPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(reverb) = {"In L", "In R", "Out L", "Out R"};

const char *reverb_room_sizes[] = { "Small", "Medium", "Large", "Tunnel-like", "Large/smooth", "Experimental" };

CALF_PORT_PROPS(reverb) = {
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip", "0dB" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_wet", "Wet amount" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 1.5,      0.4, 15.0,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "decay_time", "Decay time" },
    { 5000,    2000,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hf_damp", "High Frq Damp" },
    { 2,          0,    5,    0, PF_ENUM | PF_CTL_COMBO , reverb_room_sizes, "room_size", "Room size", },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "diffusion", "Diffusion" },
    { 0.25,       0,    2,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Wet Amount" },
    { 1.0,        0,    2,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    { 0,          0,   50,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "predelay", "Pre Delay" },
    { 300,       20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "bass_cut", "Bass Cut" },
    { 5000,      20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "treble_cut", "Treble Cut" },
    {}
};

CALF_PLUGIN_INFO(reverb) = { 0x847e, "Reverb", "Calf Reverb", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "ReverbPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(filter) = {"In L", "In R", "Out L", "Out R"};

const char *filter_choices[] = {
    "12dB/oct Lowpass",
    "24dB/oct Lowpass",
    "36dB/oct Lowpass",
    "12dB/oct Highpass",
    "24dB/oct Highpass",
    "36dB/oct Highpass",
    "6dB/oct Bandpass",
    "12dB/oct Bandpass",
    "18dB/oct Bandpass",
    "6dB/oct Bandreject",
    "12dB/oct Bandreject",
    "18dB/oct Bandreject",
};

CALF_PORT_PROPS(filter) = {
    { 2000,      10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq", "Frequency" },
    { 0.707,  0.707,   32,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "res", "Resonance" },
    { biquad_filter_module::mode_12db_lp,
      biquad_filter_module::mode_12db_lp,
      biquad_filter_module::mode_count - 1,
                                1,  PF_ENUM | PF_CTL_COMBO, filter_choices, "mode", "Mode" },
    { 20,         5,  100,    20, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "inertia", "Inertia"},
};

CALF_PLUGIN_INFO(filter) = { 0x847f, "Filter", "Calf Filter", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "FilterPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(filterclavier) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(filterclavier) = {
    { 0,        -48,   48, 48*2+1, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "transpose", "Transpose" },
    { 0,       -100,  100,      0, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune", "Detune" },
    { 32,     0.707,   32,      0, PF_FLOAT | PF_SCALE_GAIN   | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "maxres", "Max. Resonance" },
    { biquad_filter_module::mode_6db_bp,
      biquad_filter_module::mode_12db_lp,
      biquad_filter_module::mode_count - 1,
                                1, PF_ENUM  | PF_CTL_COMBO | PF_PROP_GRAPH, filter_choices, "mode", "Mode" },
    { 20,         1,  2000,    20, PF_FLOAT | PF_SCALE_LOG    | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "inertia", "Portamento time"},
    {}
};

CALF_PLUGIN_INFO(filterclavier) = { 0x849f, "Filterclavier", "Calf Filterclavier", "Krzysztof Foltman / Hans Baier", calf_plugins::calf_copyright_info, "FilterclavierPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(vintage_delay) = {"In L", "In R", "Out L", "Out R"};

const char *vintage_delay_mixmodes[] = {
    "Stereo",
    "Ping-Pong",
    "L then R",
    "R then L",
};

const char *vintage_delay_fbmodes[] = {
    "Plain",
    "Tape",
    "Old Tape",
};

CALF_PORT_PROPS(vintage_delay) = {
    { 120,      30,    300,   1, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_BPM, NULL, "bpm", "Tempo" },
    {  4,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "subdiv", "Subdivide"},
    {  3,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_l", "Time L"},
    {  5,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_r", "Time R"},
    { 0.5,       0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "feedback", "Feedback" },
    { 0.25,      0,    4,   100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
    { 1,         0,    3,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_mixmodes, "mix_mode", "Mix mode" },
    { 1,         0,    2,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_fbmodes, "medium", "Medium" },
    { 1.0,       0,    4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    { 1.0,      -1,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "width", "Stereo Width" },
    {}
};

CALF_PLUGIN_INFO(vintage_delay) = { 0x8482, "VintageDelay", "Calf Vintage Delay", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "DelayPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(rotary_speaker) = {"In L", "In R", "Out L", "Out R"};

const char *rotary_speaker_speed_names[] = { "Off", "Chorale", "Tremolo", "HoldPedal", "ModWheel", "Manual" };

CALF_PORT_PROPS(rotary_speaker) = {
    { 5,         0,  5, 1.01, PF_ENUM | PF_CTL_COMBO, rotary_speaker_speed_names, "vib_speed", "Speed Mode" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "spacing", "Tap Spacing" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "shift", "Tap Offset" },
    { 0.45,       0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mod_depth", "FM Depth" },
    { 36,       10,   600,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_RPM, NULL, "treble_speed", "Treble Motor" },
    { 30,      10,   600,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_RPM, NULL, "bass_speed", "Bass Motor" },
    { 0.7,        0,    1,  101, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mic_distance", "Mic Distance" },
    { 0.3,        0,    1,  101, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "reflection", "Reflection" },
    { 0.45,        0,           1,     0,  PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "am_depth", "AM Depth" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "test", "Test" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_l", "Low rotor" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_h", "High rotor" },
    {}
};

CALF_PLUGIN_INFO(rotary_speaker) = { 0x8483, "RotarySpeaker", "Calf Rotary Speaker", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "SimulatorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multichorus) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(multichorus) = {
    { 5,        0.1,  10,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC | PF_PROP_GRAPH, NULL, "min_delay", "Min delay" },
    { 6,        0.1,  10,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC| PF_PROP_GRAPH, NULL, "mod_depth", "Mod depth" },
    { 0.5,     0.01,  20,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ| PF_PROP_GRAPH, NULL, "mod_rate", "Modulation rate" },
    { 180,        0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 4,          1,   8,   8, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "voices", "Voices"},
    { 64,         0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "vphase", "Inter-voice phase" },
    { 2,          0,   4,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 1.0,        0,   4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    { 100,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq", "Center Frq 1" },
    { 5000,      10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Center Frq 2" },
    { 0.125,  0.125,   8,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "q", "Q" },
    { 1,          0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "overlap", "Overlap" },
};

CALF_PLUGIN_INFO(multichorus) = { 0x8501, "MultiChorus", "Calf MultiChorus", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "ChorusPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(compressor) = {"In L", "In R", "Out L", "Out R"};

const char *compressor_detection_names[] = { "RMS", "Peak" };
const char *compressor_stereo_link_names[] = { "Average", "Maximum" };

CALF_PORT_PROPS(compressor) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 2,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, compressor_detection_names, "detection", "Detection" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, compressor_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression", "Reduction" },
    {}
};

CALF_PLUGIN_INFO(compressor) = { 0x8502, "Compressor", "Calf Compressor", "Thor Harald Johansen", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(sidechaincompressor) = {"In L", "In R", "Out L", "Out R"};

const char *sidechaincompressor_detection_names[] = { "RMS", "Peak" };
const char *sidechaincompressor_stereo_link_names[] = { "Average", "Maximum" };
const char *sidechaincompressor_mode_names[] = {"Wideband (F1:off / F2:off)",
                                                "Deesser wide (F1:Bell / F2:HP)",
                                                "Deesser split (F1:off / F2:HP)",
                                                "Derumbler wide (F1:LP / F2:Bell)",
                                                "Derumbler split (F1:LP / F2:off)",
                                                "Weighted #1 (F1:Shelf / F2:Shelf)",
                                                "Weighted #2 (F1:Shelf / F2:Bell)",
                                                "Weighted #3 (F1:Bell / F2:Shelf)",
                                                "Bandpass #1 (F1:BP / F2:off)",
                                                "Bandpass #2 (F1:HP / F2:LP)"};
const char *sidechaincompressor_route_names[] = {"Stereo Input (Default)",
                                           "R ▸ L (L: Signal / R: S/C)",
                                           "L ▸ R (L: S/C / R: Signal)"};
const char *sidechaincompressor_filter_choices[] = { "12dB", "24dB", "36dB"};


CALF_PORT_PROPS(sidechaincompressor) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 2,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, sidechaincompressor_detection_names, "detection", "Detection" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, sidechaincompressor_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression", "Gain Reduction" },
    { 0,      0,  9,    0, PF_ENUM | PF_CTL_COMBO, sidechaincompressor_mode_names, "sc_mode", "S/C Mode" },
    { 250,    10,18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "f1_freq", "F1 Freq" },
    { 4500,   10,18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "f2_freq", "F2 Freq" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f1_level", "F1 Level" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f2_level", "F2 Level" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sc_listen", "S/C-Listen" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "f1_active", "F1 Active" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "f2_active", "F2 Active" },
    { 0,      0,  2,    0, PF_ENUM | PF_CTL_COMBO, sidechaincompressor_route_names, "sc_route", "S/C Route" },
    { 1,           0.015625,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "sc_level", "S/C Level" },
    {}
};

CALF_PLUGIN_INFO(sidechaincompressor) = { 0x8517, "Sidechaincompressor", "Calf Sidechain Compressor", "Markus Schmidt / Thor Harald Johansen", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multibandcompressor) = {"In L", "In R", "Out L", "Out R"};

const char *multibandcompressor_detection_names[] = { "RMS", "Peak" };
const char *multibandcompressor_filter_choices[] = { "12dB", "36dB"};

CALF_PORT_PROPS(multibandcompressor) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inL", "Input L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inR", "Input R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Output L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Output R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inL", "0dB-InL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inR", "0dB-InR" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

    { 120,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq0", "Split 1/2" },
    { 1000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq1", "Split 2/3" },
    { 6000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Split 3/4" },

    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep0", "S1" },
    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep1", "S2" },
    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep2", "S3" },

    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q0", "Q1" },
    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q1", "Q2" },
    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q2", "Q3" },

    { 1,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, multibandcompressor_filter_choices, "mode", "Filter Mode" },

    { 0.25,      0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold0", "Threshold 1" },
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio0", "Ratio 1" },
    { 150,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack0", "Attack 1" },
    { 300,         0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release0", "Release 1" },
    { 2,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup0", "Makeup 1" },
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee0", "Knee 1" },
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection0", "Detection 1" },
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression0", "Gain Reduction 1" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output0", "Output 1" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass0", "Bypass 1" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo0", "Solo 1" },


    { 0.125,     0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold1", "Threshold 2" },
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio1", "Ratio 2" },
    { 100,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack1", "Attack 2" },
    { 200,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release1", "Release 2" },
    { 2,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup1", "Makeup 2" },
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee1", "Knee 2" },
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection1", "Detection 2" },
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression1", "Gain Reduction 2" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output1", "Output 2" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass1", "Bypass 2" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo1", "Solo 2" },


    { 0.0625,    0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold2", "Threshold 3" },
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio2", "Ratio 3" },
    { 50,        0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack2", "Attack 3" },
    { 100,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release2", "Release 3" },
    { 2,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup2", "Makeup 3" },
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee2", "Knee 3" },
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection2", "Detection 3" },
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression2", "Gain Reduction 3" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output2", "Output 3" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass2", "Bypass 3" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo2", "Solo 3" },


    { 0.03125,   0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold3", "Threshold 4" },
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio3", "Ratio 4" },
    { 25,        0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack3", "Attack 4" },
    { 50,        0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release3", "Release 4" },
    { 2,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup3", "Makeup 4" },
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee3", "Knee 4" },
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection3", "Detection 4" },
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression3", "Gain Reduction 4" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output3", "Output 4" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass3", "Bypass 4" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo3", "Solo 4" },
    {}
};

CALF_PLUGIN_INFO(multibandcompressor) = { 0x8516, "Multibandcompressor", "Calf Multiband Compressor", "Markus Schmidt / Thor Harald Johansen", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(deesser) = {"In L", "In R", "Out L", "Out R"};

const char *deesser_detection_names[] = { "RMS", "Peak" };
const char *deesser_mode_names[] = { "Wide", "Split" };


CALF_PORT_PROPS(deesser) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "detected", "Detected" },
    { 0, 0.0625, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression", "Gain Reduction" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "detected_led", "Active" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "Out" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, deesser_detection_names, "detection", "Detection" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, deesser_mode_names, "mode", "Mode" },
    { 0.125,  0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold", "Threshold" },
    { 3,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 15,     1, 100,   1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "laxity", "Laxity" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup" },

    { 6000,   10,   18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "f1_freq", "Split" },
    { 4500,   10,   18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "f2_freq", "Peak" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f1_level", "Gain" },
    { 4,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f2_level", "Level" },
    { 1,      0.1,     100,1, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "f2_q", "Peak Q" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sc_listen", "S/C-Listen" },
    {}
};

CALF_PLUGIN_INFO(deesser) = { 0x8515, "Deesser", "Calf Deesser", "Markus Schmidt / Thor Harald Johansen", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(gate) = {"In L", "In R", "Out L", "Out R"};

const char *gate_detection_names[] = { "RMS", "Peak" };
const char *gate_stereo_link_names[] = { "Average", "Maximum" };

CALF_PORT_PROPS(gate) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range", "Max Gain Reduction" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, gate_detection_names, "detection", "Detection" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, gate_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating", "Gating" },
    {}
};

CALF_PLUGIN_INFO(gate) = { 0x8503, "Gate", "Calf Gate", "Damien Zammit / Thor Harald Johansen", calf_plugins::calf_copyright_info, "ExpanderPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(sidechaingate) = {"In L", "In R", "Out L", "Out R"};

const char *sidechaingate_detection_names[] = { "RMS", "Peak" };
const char *sidechaingate_stereo_link_names[] = { "Average", "Maximum" };
const char *sidechaingate_mode_names[] = {"Wideband (F1:off / F2:off)",
                                                "High gate wide (F1:Bell / F2:HP)",
                                                "High gate split (F1:off / F2:HP)",
                                                "Low Gate wide (F1:LP / F2:Bell)",
                                                "Low gate split (F1:LP / F2:off)",
                                                "Weighted #1 (F1:Shelf / F2:Shelf)",
                                                "Weighted #2 (F1:Shelf / F2:Bell)",
                                                "Weighted #3 (F1:Bell / F2:Shelf)",
                                                "Bandpass #1 (F1:BP / F2:off)",
                                                "Bandpass #2 (F1:HP / F2:LP)"};
const char *sidechaingate_route_names[] = {"Stereo Input (Default)",
                                           "R ▸ L (L: Signal / R: S/C)",
                                           "L ▸ R (L: S/C / R: Signal)"};
const char *sidechaingate_filter_choices[] = { "12dB", "24dB", "36dB"};


CALF_PORT_PROPS(sidechaingate) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range", "Max Gain Reduction" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, sidechaingate_detection_names, "detection", "Detection" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, sidechaingate_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating", "Gating" },
    { 0,      0,  9,    0, PF_ENUM | PF_CTL_COMBO, sidechaingate_mode_names, "sc_mode", "S/C Mode" },
    { 250,    10,18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "f1_freq", "F1 Freq" },
    { 4500,   10,18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "f2_freq", "F2 Freq" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f1_level", "F1 Level" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f2_level", "F2 Level" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sc_listen", "S/C-Listen" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "f1_active", "F1 Active" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "f2_active", "F2 Active" },
    { 0,      0,  2,    0, PF_ENUM | PF_CTL_COMBO, sidechaingate_route_names, "sc_route", "S/C Route" },
    { 1,           0.015625,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "sc_level", "S/C Level" },
    {}
};

CALF_PLUGIN_INFO(sidechaingate) = { 0x8504, "Sidechaingate", "Calf Sidechain Gate", "Markus Schmidt / Damien Zammit / Thor Harald Johansen", calf_plugins::calf_copyright_info, "ExpanderPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multibandgate) = {"In L", "In R", "Out L", "Out R"};

const char *multibandgate_detection_names[] = { "RMS", "Peak" };
const char *multibandgate_filter_choices[] = { "12dB", "36dB"};

CALF_PORT_PROPS(multibandgate) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inL", "Input L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inR", "Input R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Output L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Output R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inL", "0dB-InL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inR", "0dB-InR" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

    { 120,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq0", "Split 1/2" },
    { 1000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq1", "Split 2/3" },
    { 6000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Split 3/4" },

    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep0", "S1" },
    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep1", "S2" },
    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep2", "S3" },

    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q0", "Q1" },
    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q1", "Q2" },
    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q2", "Q3" },

    { 1,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, multibandgate_filter_choices, "mode", "Filter Mode" },

    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range0", "Reduction 1" },
    { 0.25,      0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold0", "Threshold 1" },
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio0", "Ratio 1" },
    { 150,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack0", "Attack 1" },
    { 300,         0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release0", "Release 1" },
    { 1,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup0", "Makeup 1" },
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee0", "Knee 1" },
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection0", "Detection 1" },
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating0", "Gating 1" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output0", "Output 1" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass0", "Bypass 1" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo0", "Solo 1" },

    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range1", "Reduction 2" },
    { 0.125,     0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold1", "Threshold 2" },
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio1", "Ratio 2" },
    { 100,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack1", "Attack 2" },
    { 200,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release1", "Release 2" },
    { 1,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup1", "Makeup 2" },
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee1", "Knee 2" },
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection1", "Detection 2" },
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating1", "Gating 2" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output1", "Output 2" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass1", "Bypass 2" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo1", "Solo 2" },

    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range2", "Reduction 3" },
    { 0.0625,    0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold2", "Threshold 3" },
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio2", "Ratio 3" },
    { 50,        0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack2", "Attack 3" },
    { 100,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release2", "Release 3" },
    { 1,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup2", "Makeup 3" },
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee2", "Knee 3" },
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection2", "Detection 3" },
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating2", "Gating 3" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output2", "Output 3" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass2", "Bypass 3" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo2", "Solo 3" },

    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range3", "Reduction 4" },
    { 0.03125,   0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold3", "Threshold 4" },
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio3", "Ratio 4" },
    { 25,        0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack3", "Attack 4" },
    { 50,        0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release3", "Release 4" },
    { 1,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup3", "Makeup 4" },
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee3", "Knee 4" },
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection3", "Detection 4" },
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating3", "Gating 4" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output3", "Output 4" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass3", "Bypass 4" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo3", "Solo 4" },
    {}
};

CALF_PLUGIN_INFO(multibandgate) = { 0x8505, "Multibandgate", "Calf Multiband Gate", "Markus Schmidt / Damien Zammit / Thor Harald Johansen", calf_plugins::calf_copyright_info, "ExpanderPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(limiter) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(limiter) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inL", "Input L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inR", "Input R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Output L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Output R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inL", "0dB-InL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inR", "0dB-InR" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

    { 1,      0.0625, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "limit", "Limit" },
    { 5,         0.1,        10,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Lookahead" },
    { 50,         1,        1000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },

    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att", "Attenuation" },

    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "asc", "ASC" },

    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "asc_led", "asc active" },

    { 0.5f,      0.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "asc_coeff", "ASC Level" },

    {}
};

CALF_PLUGIN_INFO(limiter) = { 0x8521, "Limiter", "Calf Limiter", "Christian Holschuh / Markus Schmidt", calf_plugins::calf_copyright_info, "LimiterPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multibandlimiter) = {"In L", "In R", "Out L", "Out R"};
const char *multibandlimiter_filter_choices[] = { "12dB", "36dB"};

CALF_PORT_PROPS(multibandlimiter) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inL", "Input L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inR", "Input R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Output L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Output R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inL", "0dB-InL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inR", "0dB-InR" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

    { 100,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq0", "Split 1/2" },
    { 750,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq1", "Split 2/3" },
    { 5000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Split 3/4" },

    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep0", "S1" },
    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep1", "S2" },
    { -0.17,      -0.5,         0.5,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sep2", "S3" },

    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q0", "Q1" },
    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q1", "Q2" },
    { 0.7762471166286917,    0.25,        4,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_GRAPH, NULL, "q2", "Q3" },

    { 1,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, multibandlimiter_filter_choices, "mode", "Filter Mode" },

    { 1,      0.0625, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "limit", "Limit" },
    { 4,         0.1,        10,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Lookahead" },
    { 30,         1,        1000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "minrel", "Min Release" },

    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att0", "Low" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att1", "LMid" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att2", "HMid" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att3", "Hi" },

    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight0", "Weight 1" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight1", "Weight 2" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight2", "Weight 3" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight3", "Weight 4" },

    { 0.5f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release0", "Release 1" },
    { 0.2f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release1", "Release 2" },
    { -0.2f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release2", "Release 3" },
    { -0.5f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release3", "Release 4" },

    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo0", "Solo 1" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo1", "Solo 2" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo2", "Solo 3" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo3", "Solo 4" },

    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease0", "Effectively Release 1" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease1", "Effectively Release 2" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease2", "Effectively Release 3" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease3", "Effectively Release 4" },

    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "asc", "ASC" },

    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "asc_led", "asc active" },

    { 0.5f,      0.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "asc_coeff", "ASC Level" },

    {}
};

CALF_PLUGIN_INFO(multibandlimiter) = { 0x8520, "Multibandlimiter", "Calf Multiband Limiter", "Markus Schmidt / Christian Holschuh", calf_plugins::calf_copyright_info, "LimiterPlugin" };

////////////////////////////////////////////////////////////////////////////
// A few macros to make

#define BYPASS_AND_LEVEL_PARAMS \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" }, \
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" }, \
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },

#define METERING_PARAMS \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inL", "Meter-InL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inR", "Meter-InR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Meter-OutL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Meter-OutR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inL", "0dB-InL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inR", "0dB-InR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

#define LPHP_PARAMS \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "hp_active", "HP Active" }, \
    { 30,          10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hp_freq", "HP Freq" }, \
    { 1,           0,           2,     0,  PF_ENUM | PF_CTL_COMBO, rolloff_mode_names, "hp_mode", "HP Mode" }, \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lp_active", "LP Active" }, \
    { 18000,       10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lp_freq", "LP Freq" }, \
    { 1,           0,           2,     0,  PF_ENUM | PF_CTL_COMBO, rolloff_mode_names, "lp_mode", "LP Mode" }, \

#define SHELF_PARAMS \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "ls_active", "LS Active" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "ls_level", "Level L" }, \
    { 200,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "ls_freq", "Freq L" }, \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "hs_active", "HS Active" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "hs_level", "Level H" }, \
    { 4000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hs_freq", "Freq H" },

#define EQ_BAND_PARAMS(band, frequency) \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "p" #band "_active", "F" #band " Active" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "p" #band "_level", "Level " #band }, \
    { frequency,   10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "p" #band "_freq", "Freq " #band }, \
    { 1,           0.1,         100,   1,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "p" #band "_q", "Q " #band },

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(equalizer5band) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(equalizer5band) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    SHELF_PARAMS
    EQ_BAND_PARAMS(1, 250)
    EQ_BAND_PARAMS(2, 1000)
    EQ_BAND_PARAMS(3, 2500)
    {}
};

CALF_PLUGIN_INFO(equalizer5band) = { 0x8511, "Equalizer5Band", "Calf Equalizer 5 Band", "Markus Schmidt", calf_plugins::calf_copyright_info, "EqualizerPlugin" };

//////////////////////////////////////////////////////////////////////////////


CALF_PORT_NAMES(equalizer8band) = {"In L", "In R", "Out L", "Out R"};
const char *rolloff_mode_names[] = {"12dB/oct", "24dB/oct", "36dB/oct"};

CALF_PORT_PROPS(equalizer8band) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    LPHP_PARAMS
    SHELF_PARAMS
    EQ_BAND_PARAMS(1, 250)
    EQ_BAND_PARAMS(2, 1000)
    EQ_BAND_PARAMS(3, 2500)
    EQ_BAND_PARAMS(4, 5000)
    {}
};

CALF_PLUGIN_INFO(equalizer8band) = { 0x8512, "Equalizer8Band", "Calf Equalizer 8 Band", "Markus Schmidt", calf_plugins::calf_copyright_info, "EqualizerPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(equalizer12band) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(equalizer12band) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    LPHP_PARAMS
    SHELF_PARAMS
    EQ_BAND_PARAMS(1, 60)
    EQ_BAND_PARAMS(2, 120)
    EQ_BAND_PARAMS(3, 250)
    EQ_BAND_PARAMS(4, 500)
    EQ_BAND_PARAMS(5, 1000)
    EQ_BAND_PARAMS(6, 2500)
    EQ_BAND_PARAMS(7, 4000)
    EQ_BAND_PARAMS(8, 6000)
    {}
};

CALF_PLUGIN_INFO(equalizer12band) = { 0x8513, "Equalizer12Band", "Calf Equalizer 12 Band", "Markus Schmidt", calf_plugins::calf_copyright_info, "EqualizerPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(pulsator) = {"In L", "In R", "Out L", "Out R"};

const char *pulsator_mode_names[] = { "Sine", "Triangle", "Square", "Saw up", "Saw down" };

CALF_PORT_PROPS(pulsator) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 0,           0,           4,     0,  PF_ENUM | PF_CTL_COMBO, pulsator_mode_names, "mode", "Mode" },
    { 1,           0.01,        100,   0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Frequency" },
    { 1,           0,           1,     0,  PF_FLOAT | PF_SCALE_PERC, NULL, "amount", "Modulation" },
    { 0.5,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC, NULL, "offset", "Offset L/R" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "mono", "Mono-in" },
    { 0,           0,           1,     2,  PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    {}
};

CALF_PLUGIN_INFO(pulsator) = { 0x8514, "Pulsator", "Calf Pulsator", "Markus Schmidt", calf_plugins::calf_copyright_info, "ModulationPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(saturator) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(saturator) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Activation" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Master" },
    { 1,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "mix", "Mix" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB" },

    { 5,           0.1,         10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive", "Saturation" },
    { 10,         -10,          10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend", "Blend" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_drive", "Drive" },

    { 20000,      10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lp_pre_freq", "Lowpass" },
    { 10,         10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hp_pre_freq", "Highpass" },

    { 20000,      10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lp_post_freq", "Lowpass" },
    { 10,         10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hp_post_freq", "Highpass" },

    { 2000,       80,           8000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "p_freq", "Tone" },
    { 1,          0.0625,       16,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "p_level", "Amount" },
    { 1,          0.1,          10,    1,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "p_q", "Gradient" },
    {}
};

CALF_PLUGIN_INFO(saturator) = { 0x8530, "Saturator", "Calf Saturator", "Markus Schmidt / Krzysztof Foltman", calf_plugins::calf_copyright_info, "DistortionPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(exciter) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(exciter) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB" },

    { 8.5,         0.1,         10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive", "Harmonics" },
    { 0,          -10,          10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend", "Blend harmonics" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_drive", "Harmonics level" },

    { 6000,       2000,         12000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Scope" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "listen", "Listen" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "ceil_active", "Ceiling active" },
    { 16000,      10000,        20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "ceil", "Ceiling" },
    {}
};

CALF_PLUGIN_INFO(exciter) = { 0x8531, "Exciter", "Calf Exciter", "Markus Schmidt / Krzysztof Foltman", calf_plugins::calf_copyright_info, "DistortionPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(bassenhancer) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(bassenhancer) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB" },

    { 8.5,         0.1,         10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive", "Harmonics" },
    { 0,          -10,          10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend", "Blend harmonics" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_drive", "Harmonics level" },

    { 120,        10,           250,   0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Scope" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "listen", "Listen" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "floor_active", "Floor active" },
    { 30,         10,           120,   0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "floor", "Floor" },
    {}
};

CALF_PLUGIN_INFO(bassenhancer) = { 0x8532, "BassEnhancer", "Calf Bass Enhancer", "Markus Schmidt / Krzysztof Foltman", calf_plugins::calf_copyright_info, "DistortionPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(mono) = {"In", "Out L", "Out R"};
CALF_PORT_PROPS(mono) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Output L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Output R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

    { 0.f,      -1.f,           1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "balance_out", "Balance" },

    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "softclip", "Softclip" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "mutel", "Mute L" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "muter", "Mute R" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phasel", "Phase L" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phaser", "Phase R" },

    { 0.f,         -20.f,        20.f,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "delay", "Delay" },
    {}
};

CALF_PLUGIN_INFO(mono) = { 0x8589, "MonoInput", "Calf Mono Input", "Markus Schmidt", calf_plugins::calf_copyright_info, "Utility" };


////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(stereo) = {"In L", "In R", "Out L", "Out R"};
const char *stereo_mode_names[] = { "LR ▸ LR (Stereo Default)", "LR ▸ MS (Stereo to Mid-Side)", "MS ▸ LR (Mid-Side to Stereo)", "LR ▸ LL (Mono Left Channel)", "LR ▸ RR (Mono Right Channel)", "LR ▸ L+R (Mono Sum L+R)", "LR ▸ RL (Stereo Flip Channels)" };
CALF_PORT_PROPS(stereo) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inL", "Input L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inR", "Input R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Output L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Output R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inL", "0dB-InL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inR", "0dB-InR" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

    { 0.f,      -1.f,           1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "balance_in", "Balance In" },
    { 0.f,      -1.f,           1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "balance_out", "Balance Out" },

    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "softclip", "Softclip" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "mutel", "Mute L" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "muter", "Mute R" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phasel", "Phase L" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phaser", "Phase R" },

    { 0,           0,           6,     0,  PF_ENUM | PF_CTL_COMBO, stereo_mode_names, "mode", "Mode" },

    { 0.f,      -1.f,            1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "slev", "S Level" },
    { 0.f,      -1.f,            1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "sbal", "S Balance" },
    { 0.f,      -1.f,            1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "mlev", "M Level" },
    { 0.f,      -1.f,            1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "mpan", "M Panorama" },

    { 0,           0,           1,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "widener", "Widener" },
    { 0.f,         -20.f,        20.f,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "delay", "Delay" },

    { 0.f,      0.f,           1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_COEF | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_phase", "Phase Correlation" },

    {}
};

CALF_PLUGIN_INFO(stereo) = { 0x8588, "StereoTools", "Calf Stereo Tools", "Markus Schmidt", calf_plugins::calf_copyright_info, "Utility" };


////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(monosynth) = {
    "Out L", "Out R",
};

const char *monosynth_waveform_names[] = { "Sawtooth", "Square", "Pulse", "Sine", "Triangle", "Varistep", "Skewed Saw", "Skewed Square",
    "Smooth Brass", "Bass", "Dark FM", "Multiwave", "Bell FM", "Dark Pad", "DCO Saw", "DCO Maze" };
const char *monosynth_mode_names[] = { "0\xC2\xB0 : 0\xC2\xB0", "0\xC2\xB0 : 180\xC2\xB0", "0\xC2\xB0 : 90\xC2\xB0", "90\xC2\xB0 : 90\xC2\xB0", "90\xC2\xB0 : 270\xC2\xB0", "Random" };
const char *monosynth_legato_names[] = { "Retrig", "Legato", "Fng Retrig", "Fng Legato" };
const char *monosynth_lfotrig_names[] = { "Retrig", "Free" };

const char *monosynth_filter_choices[] = {
    "12dB/oct Lowpass",
    "24dB/oct Lowpass",
    "2x12dB/oct Lowpass",
    "12dB/oct Highpass",
    "Lowpass+Notch",
    "Highpass+Notch",
    "6dB/oct Bandpass",
    "2x6dB/oct Bandpass",
};

CALF_PLUGIN_INFO(monosynth) = { 0x8480, "Monosynth", "Calf Monosynth", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "SynthesizerPlugin" };

CALF_PORT_PROPS(monosynth) = {
    { monosynth_metadata::wave_saw,         0, monosynth_metadata::wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO | PF_PROP_GRAPH, monosynth_waveform_names, "o1_wave", "Osc1 Wave" },
    { monosynth_metadata::wave_sqr,         0, monosynth_metadata::wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO | PF_PROP_GRAPH, monosynth_waveform_names, "o2_wave", "Osc2 Wave" },

    { 0,         -1,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "o1_pw", "Osc1 PW" },
    { 0,         -1,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "o2_pw", "Osc2 PW" },

    { 10,         0,  100,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "o12_detune", "O1<>2 Detune" },
    { 12,       -24,   24,    0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o2_xpose", "Osc 2 transpose" },
    { 0,          0,    5,    0, PF_ENUM | PF_CTL_COMBO, monosynth_mode_names, "phase_mode", "Phase mode" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "o12_mix", "O1<>2 Mix" },
    { 1,          0,    7,    0, PF_ENUM | PF_CTL_COMBO | PF_PROP_GRAPH, monosynth_filter_choices, "filter", "Filter" },
    { 33,        10,16000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "cutoff", "Cutoff" },
    { 3,        0.7,    8,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "res", "Resonance" },
    { 0,      -2400, 2400,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "filter_sep", "Separation" },
    { 8000,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "env2cutoff", "Env->Cutoff" },
    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "env2res", "Env->Res" },
    { 0,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "env2amp", "Env->Amp" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr_a", "EG1 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr_d", "EG1 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_s", "EG1 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr_f", "EG1 Fade" },
    { 100,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr_r", "EG1 Release" },

    { 0,          0,    2,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "key_follow", "Key Follow" },
    { 0,          0,    3,    0, PF_ENUM | PF_CTL_COMBO, monosynth_legato_names, "legato", "Legato Mode" },
    { 1,          1, 2000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "portamento", "Portamento" },

    { 0.5,        0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "vel2filter", "Vel->Filter" },
    { 0,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "vel2amp", "Vel->Amp" },

    { 0.5,         0,   1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_PROP_OUTPUT_GAIN, NULL, "master", "Volume" },

    { 200,         0, 2400,   25, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "pbend_range", "PBend Range" },

    { 5,       0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo_rate", "LFO1 Rate" },
    { 0.5,        0,  5,    0, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "lfo_delay", "LFO1 Delay" },
    { 0,      -4800, 4800,  0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "lfo2filter", "LFO1->Filter" },
    { 100,        0, 1200,  0, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "lfo2pitch", "LFO1->Pitch" },
    { 0,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "lfo2pw", "LFO1->PW" },
    { 1,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "mwhl2lfo", "ModWheel->LFO1" },

    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "scale_detune", "Scale Detune" },

    { 0,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "adsr2_cutoff", "EG2->Cutoff" },
    { 0.3,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "adsr2_res", "EG2->Res" },
    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "adsr2_amp", "EG2->Amp" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr2_a", "EG2 Attack" },
    { 100,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr2_d", "EG2 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_s", "EG2 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr2_f", "EG2 Fade" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr2_r", "Release" },

    { 1,          1,   16,    0, PF_FLOAT | PF_SCALE_LOG | PF_UNIT_COEF | PF_CTL_KNOB, NULL, "o1_stretch", "Osc1 Stretch" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "o1_window", "Osc1 Window" },

    { 0,          0,    1,    0, PF_ENUM | PF_CTL_COMBO, monosynth_lfotrig_names, "lfo1_trig", "LFO1 Trigger Mode" },
    { 0,          0,    1,    0, PF_ENUM | PF_CTL_COMBO, monosynth_lfotrig_names, "lfo2_trig", "LFO2 Trigger Mode" },
    { 5,       0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo2_rate", "LFO1 Rate" },
    { 0.5,      0.1,  5,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "lfo2_delay", "LFO1 Delay" },
    {}
};

static const char *monosynth_mod_src_names[] = {
    "None",
    "Velocity",
    "Pressure",
    "ModWheel",
    "Envelope 1",
    "Envelope 2",
    "LFO 1",
    "LFO 2",
    NULL
};

static const char *monosynth_mod_dest_names[] = {
    "None",
    "Attenuation",
    "Osc Mix Ratio (%)",
    "Cutoff [ct]",
    "Resonance",
    "O1: Detune [ct]",
    "O2: Detune [ct]",
    "O1: PW (%)",
    "O2: PW (%)",
    "O1: Stretch",
    NULL
};

monosynth_metadata::monosynth_metadata()
: mm_metadata(mod_matrix_slots, monosynth_mod_src_names, monosynth_mod_dest_names)
{
}

const char *const *monosynth_metadata::get_configure_vars() const
{
    return mod_matrix_impl::get_configure_vars<mod_matrix_slots>();
}

////////////////////////////////////////////////////////////////////////////

CALF_PLUGIN_INFO(organ) = { 0x8481, "Organ", "Calf Organ", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "SynthesizerPlugin" };

plugin_command_info *organ_metadata::get_commands()
{
    static plugin_command_info cmds[] = {
        { "cmd_panic", "Panic!", "Stop all sounds and reset all controllers" },
        { NULL }
    };
    return cmds;
}

CALF_PORT_NAMES(organ) = {"Out L", "Out R"};

const char *organ_percussion_trigger_names[] = { "First note", "Each note", "Each, no retrig", "Polyphonic" };

const char *organ_wave_names[] = {
    "Sin",
    "S0", "S00", "S000",
    "SSaw", "SSqr", "SPls",
    "Saw", "Sqr", "Pls",
    "S(", "Sq(", "S+", "Clvg",
    "Bell", "Bell2",
    "W1", "W2", "W3", "W4", "W5", "W6", "W7", "W8", "W9",
    "DSaw", "DSqr", "DPls",
    "P:SynStr","P:WideStr","P:Sine","P:Bell","P:Space","P:Voice","P:Hiss","P:Chant",
};

const char *organ_routing_names[] = { "Out", "Flt 1", "Flt 2"  };

const char *organ_ampctl_names[] = { "None", "Direct", "Flt 1", "Flt 2", "All"  };

const char *organ_vibrato_mode_names[] = { "None", "Direct", "Flt 1", "Flt 2", "Voice", "Global"  };

const char *organ_vibrato_type_names[] = { "Allpass", "Scanner (V1/C1)", "Scanner (V2/C2)", "Scanner (V3/C3)", "Scanner (Full)"  };

const char *organ_filter_type_names[] = { "12dB/oct LP", "12dB/oct HP" };

const char *organ_filter_send_names[] = { "Output", "Filter 2" };

const char *organ_init_map_curve = "2\n0 1\n1 1\n";

CALF_PORT_PROPS(organ) = {
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l1", "16'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l2", "5 1/3'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l3", "8'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l4", "4'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l5", "2 2/3'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l6", "2'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l7", "1 3/5'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l8", "1 1/3'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l9", "1'" },

    { 1,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f1", "Freq 1" },
    { 3,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f2", "Freq 2" },
    { 2,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f3", "Freq 3" },
    { 4,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f4", "Freq 4" },
    { 6,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f5", "Freq 5" },
    { 8,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f6", "Freq 6" },
    { 10,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f7", "Freq 7" },
    { 12,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f8", "Freq 8" },
    { 16,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f9", "Freq 9" },

    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w1", "Wave 1" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w2", "Wave 2" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w3", "Wave 3" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w4", "Wave 4" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w5", "Wave 5" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w6", "Wave 6" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w7", "Wave 7" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w8", "Wave 8" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w9", "Wave 9" },

    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune1", "Detune 1" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune2", "Detune 2" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune3", "Detune 3" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune4", "Detune 4" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune5", "Detune 5" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune6", "Detune 6" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune7", "Detune 7" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune8", "Detune 8" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune9", "Detune 9" },

    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase1", "Phase 1" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase2", "Phase 2" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase3", "Phase 3" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase4", "Phase 4" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase5", "Phase 5" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase6", "Phase 6" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase7", "Phase 7" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase8", "Phase 8" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase9", "Phase 9" },

    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan1", "Pan 1" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan2", "Pan 2" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan3", "Pan 3" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan4", "Pan 4" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan5", "Pan 5" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan6", "Pan 6" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan7", "Pan 7" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan8", "Pan 8" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan9", "Pan 9" },

    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing1", "Routing 1" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing2", "Routing 2" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing3", "Routing 3" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing4", "Routing 4" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing5", "Routing 5" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing6", "Routing 6" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing7", "Routing 7" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing8", "Routing 8" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing9", "Routing 9" },

    { 96 + 12,      0,  127, 128, PF_INT | PF_CTL_KNOB | PF_UNIT_NOTE, NULL, "foldnote", "Foldover" },

    { 200,         10,  3000, 100, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "perc_decay", "P: Carrier Decay" },
    { 0.25,      0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "perc_level", "P: Level" },
    { 0,         0,  organ_enums::wave_count_small - 1, 1, PF_ENUM | PF_CTL_COMBO, organ_wave_names, "perc_waveform", "P: Carrier Wave" },
    { 6,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "perc_harmonic", "P: Carrier Frq" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_vel2amp", "P: Vel->Amp" },

    { 200,         10,  3000, 100, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "perc_fm_decay", "P: Modulator Decay" },
    { 0,          0,    4,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_fm_depth", "P: FM Depth" },
    { 0,         0,  organ_enums::wave_count_small - 1, 1, PF_ENUM | PF_CTL_COMBO, organ_wave_names, "perc_fm_waveform", "P: Modulator Wave" },
    { 6,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "perc_fm_harmonic", "P: Modulator Frq" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_vel2fm", "P: Vel->FM" },

    { 0,         0,  organ_enums::perctrig_count - 1, 0, PF_ENUM | PF_CTL_COMBO, organ_percussion_trigger_names, "perc_trigger", "P: Trigger" },
    { 90,      0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "perc_stereo", "P: Stereo Phase" },

    { 0,         0,  1, 0, PF_ENUM | PF_CTL_COMBO, organ_filter_send_names, "filter_chain", "Filter 1 To" },
    { 0,         0,  1, 0, PF_ENUM | PF_CTL_COMBO, organ_filter_type_names, "filter1_type", "Filter 1 Type" },
    { 0.1,         0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_PROP_OUTPUT_GAIN | PF_PROP_GRAPH, NULL, "master", "Volume" },

    { 2000,   20, 20000, 100, PF_FLOAT | PF_SCALE_LOG | PF_UNIT_HZ | PF_CTL_KNOB, NULL, "f1_cutoff", "F1 Cutoff" },
    { 2,        0.7,    8,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL, "f1_res", "F1 Res" },
    { 8000,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f1_env1", "F1 Env1" },
    { 0,     -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f1_env2", "F1 Env2" },
    { 0,     -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f1_env3", "F1 Env3" },
    { 0,          0,    2,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "f1_keyf", "F1 KeyFollow" },

    { 2000,   20, 20000, 100, PF_FLOAT | PF_SCALE_LOG | PF_UNIT_HZ | PF_CTL_KNOB, NULL, "f2_cutoff", "F2 Cutoff" },
    { 2,        0.7,    8,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL, "f2_res", "F2 Res" },
    { 0,     -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f2_env1", "F2 Env1" },
    { 8000,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f2_env2", "F2 Env2" },
    { 0,     -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f2_env3", "F2 Env3" },
    { 0,          0,    2,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "f2_keyf", "F2 KeyFollow" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_a", "EG1 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_d", "EG1 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_s", "EG1 Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_r", "EG1 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_v", "EG1 VelMod" },
    { 0,  0, organ_enums::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg1_amp_ctl", "EG1 To Amp"},

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_a", "EG2 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_d", "EG2 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_s", "EG2 Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_r", "EG2 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_v", "EG2 VelMod" },
    { 0,  0, organ_enums::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg2_amp_ctl", "EG2 To Amp"},

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_a", "EG3 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_d", "EG3 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_s", "EG3 Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_r", "EG3 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_v", "EG3 VelMod" },
    { 0,  0, organ_enums::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg3_amp_ctl", "EG3 To Amp"},

    { 6.6,     0.01,  240,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "vib_rate", "Vib Rate" },
    { 1.0,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "vib_amt", "Vib Mod Amt" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "vib_wet", "Vib Wet" },
    { 180,        0,  360,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "vib_phase", "Vib Stereo" },
    { organ_enums::lfomode_global,    0,  organ_enums::lfomode_count - 1,    0, PF_ENUM | PF_CTL_COMBO, organ_vibrato_mode_names, "vib_mode", "Vib Mode" },
    { organ_enums::lfotype_cv3,       0,   organ_enums::lfotype_count - 1,    0, PF_ENUM | PF_CTL_COMBO, organ_vibrato_type_names, "vib_type", "Vib Type" },
//    { 0,  0, organ_enums::ampctl_count - 1,
//                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "vel_amp_ctl", "Vel To Amp"},

    { -12,        -24, 24,   49, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "transpose", "Transpose" },
    { 0,       -100,  100,  201, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune", "Detune" },

    { 16,         1,   32,   32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "polyphony", "Polyphony" },

    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "quad_env", "Quadratic AmpEnv" },

    { 200,        0, 2400,   25, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "pbend_range", "PBend Range" },

    { 80,       20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "bass_freq", "Bass Freq" },
    { 1,        0.1, 10,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "bass_gain", "Bass Gain" },
    { 12000,     20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "treble_freq", "Treble Freq" },
    { 1,        0.1, 10,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "treble_gain", "Treble Gain" },
};

const char *const *organ_metadata::get_configure_vars() const
{
    static const char *names[] = {"map_curve", NULL};
    return names;
}

////////////////////////////////////////////////////////////////////////////

const char *fluidsynth_init_soundfont = "";
const char *fluidsynth_init_presetkeyset = "";

const char *fluidsynth_interpolation_names[] = { "None (zero-hold)", "Linear", "Cubic", "7-point" };

CALF_PORT_NAMES(fluidsynth) = {
    "Out L", "Out R",
};

CALF_PLUGIN_INFO(fluidsynth) = { 0x8700, "Fluidsynth", "Calf Fluidsynth", "FluidSynth Team / Krzysztof Foltman", calf_plugins::calf_copyright_info, "SynthesizerPlugin" };

CALF_PORT_PROPS(fluidsynth) = {
    { 0.5,         0,   1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_PROP_OUTPUT_GAIN, NULL, "master", "Volume" },
    { 2,          0,    3,    0, PF_ENUM | PF_CTL_COMBO, fluidsynth_interpolation_names, "interpolation", "Interpolation" },
    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "reverb", "Reverb" },
    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "chorus", "Chorus" },
};

const char *const *fluidsynth_metadata::get_configure_vars() const
{
    static const char *names[] = {"soundfont", "preset_key_set", NULL};
    return names;
}

////////////////////////////////////////////////////////////////////////////

const char *wavetable_names[] = {
    "Shiny1",
    "Shiny2",
    "Rezo",
    "Metal",
    "Bell",
    "Blah",
    "Pluck",
    "Stretch",
    "Stretch 2",
    "Hard Sync",
    "Hard Sync 2",
    "Soft Sync",
    "Bell 2",
    "Bell 3",
    "Tine",
    "Tine 2",
    "Clav",
    "Clav 2",
    "Gtr",
    "Gtr 2",
    "Gtr 3",
    "Gtr 4",
    "Gtr 5",
    "Reed",
    "Reed 2",
    "Silver",
    "Brass",
    "Multi",
    "Multi 2",
};

static const char *wavetable_mod_src_names[] = {
    "None",
    "Velocity",
    "Pressure",
    "ModWheel",
    "Env 1",
    "Env 2",
    "Env 3",
    NULL
};

static const char *wavetable_mod_dest_names[] = {
    "None",
    "Attenuation",
    "Osc Mix Ratio (%)",
    "Cutoff [ct]",
    "Resonance",
    "O1: Shift (%)",
    "O2: Shift (%)",
    "O1: Detune [ct]",
    "O2: Detune [ct]",
    NULL
};

CALF_PORT_NAMES(wavetable) = {
    "Out L", "Out R",
};

CALF_PLUGIN_INFO(wavetable) = { 0x8701, "Wavetable", "Calf Wavetable", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "SynthesizerPlugin" };

CALF_PORT_PROPS(wavetable) = {
    { wavetable_metadata::wt_count - 1,       0,  wavetable_metadata::wt_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, wavetable_names, "o1wave", "Osc1 Wave" },
    { 0.2,     -1,      1,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "o1offset", "Osc1 Ctl"},
    { 0,        -48,   48, 48*2+1, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o1trans", "Osc1 Transpose" },
    { 6,       -100,  100,      0, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "o1detune", "Osc1 Detune" },
    { 0.1,      0,   1,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "o1level", "Osc1 Level" },

    { 0,       0,  wavetable_metadata::wt_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, wavetable_names, "o2wave", "Osc2 Wave" },
    { 0.4,     -1,      1,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "o2offset", "Osc2 Ctl"},
    { 0,        -48,   48, 48*2+1, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o2trans", "Osc2 Transpose" },
    { -6,     -100,  100,      0, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "o2detune", "Osc2 Detune" },
    { 0,        0,   1,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "o2level", "Osc2 Level" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_a", "EG1 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_d", "EG1 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_s", "EG1 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_f", "EG1 Fade" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_r", "EG1 Release" },
    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_v", "EG1 VelMod" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_a", "EG2 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_d", "EG2 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_s", "EG2 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_f", "EG2 Fade" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_r", "EG2 Release" },
    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_v", "EG2 VelMod" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_a", "EG3 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_d", "EG3 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_s", "EG3 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_f", "EG3 Fade" },
    { 50,       10, 20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_r", "EG3 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_v", "EG3 VelMod" },

    { 200,        0, 2400,   25, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "pbend_range", "PBend Range" },
    {}
};

wavetable_metadata::wavetable_metadata()
: mm_metadata(mod_matrix_slots, wavetable_mod_src_names, wavetable_mod_dest_names)
{
}

////////////////////////////////////////////////////////////////////////////

calf_plugins::plugin_registry::plugin_registry()
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) plugins.push_back((new name##_metadata));
    #include <calf/modulelist.h>
}
