/*
    wavtool-yawu
    Copyright (C) 2015 StarBrilliant <m13253@hotmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program.  If not,
    see <http://www.gnu.org/licenses/>.
*/

#include "pcm_merger.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ios>
#include <iostream>
#include <libwintf8/termio.h>
#include <sndfile.h>
#include "option_manager.hpp"
#include "pcm_file.hpp"
#include "rand_round.hpp"
#include "utils.hpp"

#ifndef YAWU_OUTPUT_SAMPLE_FORMAT
/**
 * YAWU_OUTPUT_SAMPLE_FORMAT is configurable at compile time
 *
 * Some media players only support 16-bit PCM file,
 * you can configure it by defining `gcc -DYAWU_OUTPUT_SAMPLE_FORMAT=SF_FORMAT_PCM_16`.
 * Refer to http://www.mega-nerd.com/libsndfile/api.html#open for more options.
 */
#define YAWU_OUTPUT_SAMPLE_FORMAT SF_FORMAT_PCM_32
#endif

#ifdef _MSC_VER
typedef ptrdiff_t ssize_t;
#endif

namespace YAWU {

struct PCMMerger::Private {
    PCMFile input_file;
    PCMFile output_file;
    int32_t sample_rate = 0;
    ssize_t prefix_samples = 0;
    ssize_t append_samples = 0;
    int64_t existing_samples = 0;
    std::vector<double> envelope;
    std::vector<double> buffer1;
    std::vector<double> buffer2;
};

PCMMerger::PCMMerger(OptionManager &option_manager) :
    option_manager(option_manager) {
}

PCMMerger::~PCMMerger() {
}

PCMMerger &PCMMerger::prepare() {
    try {
        p->input_file.open(option_manager.get_input_file_name(), std::ios_base::in, 0, 1, 0);
        p->sample_rate = p->input_file.sample_rate();
        if(p->input_file.channels() != 1) {
            p->input_file.close();
            throw PCMFile::FileError("Must have only one channel");
        }
    } catch(PCMFile::FileError e) {
        WTF8::cerr << "Unable to open input file: " << e.what() << std::endl;
    }
    try {
        p->output_file.open(option_manager.get_output_file_name(), std::ios_base::in | std::ios_base::out, SF_FORMAT_WAV | YAWU_OUTPUT_SAMPLE_FORMAT, 1, p->sample_rate != 0 ? p->sample_rate : 44100);
        if(p->sample_rate != 0) {
            if(p->sample_rate != p->output_file.sample_rate()) {
                WTF8::cerr << "Warning: Sample rate mismatch between input and output file" << std::endl
                           << "Sample rate: " << p->sample_rate << " Hz != " << p->output_file.sample_rate() << " Hz" << std::endl;
                p->input_file.close();
            }
        } else {
            p->sample_rate = p->output_file.sample_rate();
        }
        if(p->output_file.channels() != 1) {
            p->output_file.close();
            throw PCMFile::FileError("Must have only one channel");
        }
    } catch(PCMFile::FileError e) {
        WTF8::cerr << "Unable to open output file: " << e.what() << std::endl;
        return *this;
    }
    p->prefix_samples = ssize_t(std::round(option_manager.get_overlap() * p->sample_rate));
    if(p->prefix_samples < 0) {
        WTF8::cerr << "Warning: overlap value is negative" << std::endl;
        p->prefix_samples = 0;
    }
    p->append_samples = ssize_t(RandRound()(option_manager.get_note_length() * p->sample_rate));
    if(p->append_samples < 0) {
        WTF8::cerr << "Warning: note length is negative" << std::endl;
        p->append_samples = 0;
    }
    p->existing_samples = p->output_file.frames();
    assert(p->existing_samples >= 0);
    p->envelope = std::move(std::vector<double>(p->prefix_samples + p->append_samples));
    p->buffer1 = std::move(std::vector<double>(p->prefix_samples + p->append_samples));
    p->buffer2 = std::move(std::vector<double>(p->prefix_samples + p->append_samples));
    return *this;
}

PCMMerger &PCMMerger::fill_overlap() {
    if(p->output_file.is_open()) {
        try {
            if(p->existing_samples >= int64_t(p->prefix_samples)) {
                p->output_file.seek(p->existing_samples - int64_t(p->prefix_samples), SEEK_SET);
                p->output_file.read(p->buffer1.data(), p->prefix_samples);
            } else {
                p->output_file.seek(0, SEEK_SET);
                p->output_file.read(&p->buffer1.data()[int64_t(p->prefix_samples) - p->existing_samples], p->existing_samples);
            }
        } catch(PCMFile::FileError e) {
            WTF8::cerr << "Unable to read from output file: " << e.what() << std::endl;
            return *this;
        }
    }
    return *this;
}

PCMMerger &PCMMerger::read_new_segment() {
    if(p->input_file.is_open()) {
        try {
            ssize_t stp_samples = ssize_t(std::round(option_manager.get_stp() * p->sample_rate));
            if(stp_samples >= 0) {
                p->input_file.seek(stp_samples, SEEK_SET);
                auto output = p->input_file.read(p->buffer2.data(), p->buffer2.size());
                auto buffer2size = p->buffer2.size();
				if( output < buffer2size )
                    WTF8::cerr << "Warning: Input file is not long enough" << std::endl;
            } else {
                auto neg_stp_samples = -stp_samples;
                if(p->input_file.read(&p->buffer2.data()[neg_stp_samples], p->buffer2.size() - neg_stp_samples) < p->buffer2.size() - neg_stp_samples)
                    WTF8::cerr << "Warning: Input file is not long enough" << std::endl;
            }
        } catch(PCMFile::FileError e) {
            WTF8::cerr << "Unable to read from input file: " << e.what() << std::endl;
            return *this;
        }
    }
    return *this;
}

PCMMerger &PCMMerger::construct_envelope() {
    if(p->envelope.size() == 0)
        return *this;

    ssize_t abs_p[7];
    double abs_v[7];
    abs_p[0] = 0;
    abs_v[0] = 0;
    abs_p[1] = ssize_t(std::round(option_manager.get_env_p(1) * p->sample_rate));
    abs_v[1] = option_manager.get_env_v(1);
    abs_p[2] = ssize_t(std::round((option_manager.get_env_p(1) + option_manager.get_env_p(2)) * p->sample_rate));
    abs_v[2] = option_manager.get_env_v(2);
    if(option_manager.is_p5_enabled()) {
        abs_p[3] = ssize_t(std::round((option_manager.get_env_p(1) + option_manager.get_env_p(2) + option_manager.get_env_p(5)) * p->sample_rate));
        abs_v[3] = option_manager.get_env_v(5);
    } else {
        abs_p[3] = abs_p[2];
        abs_v[3] = abs_v[2];
    }
    abs_p[4] = ssize_t(p->envelope.size()-1) - ssize_t(std::round((option_manager.get_env_p(3) + option_manager.get_env_p(4)) * p->sample_rate));
    abs_v[4] = option_manager.get_env_v(3);
    abs_p[5] = ssize_t(p->envelope.size()-1) - ssize_t(std::round(option_manager.get_env_p(4) * p->sample_rate));
    abs_v[5] = option_manager.get_env_v(4);
    abs_p[6] = ssize_t(p->envelope.size()-1);
    abs_v[6] = 0;

    const auto interpolate_envelope = [&](ssize_t pa, double va, ssize_t pb, double vb) {
        if(pa == pb)
            return;
        double k = (vb-va) / (pb-pa);
        pa = clamp(pa, ssize_t(0), ssize_t(p->envelope.size()-1));
        pb = clamp(pb, ssize_t(0), ssize_t(p->envelope.size()-1));
        auto envelope = p->envelope.data();
        if(pa < pb) {
            for(auto i = pa; i <= pb; i++)
                envelope[i] = std::max((i-pa)*k + va, envelope[i]);
        } else if(pa != pb) {
            for(auto i = pb; i <= pa; i++)
                envelope[i] = std::max((i-pb)*k + vb, envelope[i]);
        }
    };

    interpolate_envelope(abs_p[0], abs_v[0], abs_p[1], abs_v[1]);
    interpolate_envelope(abs_p[1], abs_v[1], abs_p[2], abs_v[2]);
    interpolate_envelope(abs_p[2], abs_v[2], abs_p[3], abs_v[3]);
    interpolate_envelope(abs_p[3], abs_v[3], abs_p[4], abs_v[4]);
    interpolate_envelope(abs_p[4], abs_v[4], abs_p[5], abs_v[5]);
    interpolate_envelope(abs_p[5], abs_v[5], abs_p[6], abs_v[6]);

    WTF8::clog << "Env: ";
    for(double i = 0.5; i < 74; i++) {
        size_t sample_idx = size_t(std::round((p->envelope.size()-1) * i / 74));
        auto env_value = p->envelope[sample_idx];
        char visual =
            env_value < 1.0 ?
                env_value < 0.5 ?
                    env_value < 0.25 ? ' ' : '.' :
                    env_value < 0.75 ? ':' : '-' :
                env_value <= 1.5 ?
                    env_value < 1.25 ? '=' : 'X' :
                    env_value <= 2.0 ? '#' : '!';
        WTF8::clog << visual;
    }
    WTF8::clog << std::endl;

    return *this;
}

PCMMerger &PCMMerger::mix_new_segment() {
    assert(p->buffer1.size() == p->buffer2.size());
    assert(p->buffer2.size() == p->envelope.size());
    if(p->output_file.is_open()) {
        auto old_segments = p->buffer1.data();
        auto new_segment = p->buffer2.data();
        auto envelope = p->envelope.data();
        for(size_t i = 0; i < p->buffer1.size(); i++) {
            old_segments[i] = clamp(new_segment[i]*envelope[i] + old_segments[i], -1.0, 1.0);
            if(std::isnan(old_segments[i]))
                old_segments[i] = 0;
        }
    }
    return *this;
}

PCMMerger &PCMMerger::write_back() {
    if(p->output_file.is_open()) {
        try {
            if(p->existing_samples >= int64_t(p->prefix_samples)) {
                p->output_file.seek(p->existing_samples - int64_t(p->prefix_samples), SEEK_SET);
                p->output_file.write(p->buffer1.data(), p->buffer1.size());
            } else {
                p->output_file.seek(0, SEEK_SET);
                p->output_file.write(&p->buffer1.data()[int64_t(p->prefix_samples) - p->existing_samples], int64_t(p->buffer1.size()) - (int64_t(p->prefix_samples) - p->existing_samples));
            }
        } catch(PCMFile::FileError e) {
            WTF8::cerr << "Unable to write to output file: " << e.what() << std::endl;
            return *this;
        }
    }
    return *this;
}

}

