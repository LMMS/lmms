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

#include <iostream>
#include <libwintf8/argv.h>
#include <libwintf8/termio.h>
#include "cmdline_parser.hpp"
#include "pcm_merger.hpp"
#include "proxy_ptr.hpp"
#include "wavtool-yawu.h"

int WAVTOOL_YAWU::process(const std::string &outputPath,
            const std::string &inputPath,
            double scaledStartPoint,
            double scaledNoteLength,
            double p1,
            double p2,
            double p3,
            double v1,
            double v2,
            double v3,
            double v4,
            double overlap,
            double p4,
            double p5,
            double v5,
            bool p5_enabled) {
    using namespace YAWU;

    proxy_ptr<OptionManager> option_manager; // full lifetime object
    option_manager->set_output_file_name(outputPath.c_str());
    option_manager->set_input_file_name(inputPath.c_str());
    option_manager->set_stp(scaledStartPoint / 1000);
    option_manager->set_note_length(scaledNoteLength / 1000);
    double *p = new double[6]{0,p1 / 1000,p2 / 1000,p3 / 1000,p4 / 1000,p5 / 1000};
    double *v = new double[6] {1,v1 / 100,v2 / 100,v3 / 100,v4 / 100,v5 / 100};
    option_manager->set_env_p(p);
    option_manager->set_env_v(v);
    option_manager->set_p5_enabled(p5_enabled);
    option_manager->set_overlap(overlap / 1000);
    WTF8::cout << std::endl
               << "wavtool-yawu, Yet Another Wavtool for UTAU" << std::endl
               << "https://github.com/m13253/wavtool-yawu" << std::endl
               << std::endl;

    PCMMerger(*option_manager.get())
        .prepare()
        .fill_overlap()
        .read_new_segment()
        .construct_envelope()
        .mix_new_segment()
        .write_back();

    return 0;
}
