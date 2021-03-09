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

#include "cmdline_parser.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <libwintf8/fileio.h>
#include <libwintf8/termio.h>
#include <libwintf8/u8str.h>
#include "option_manager.hpp"
#include "utils.hpp"

namespace YAWU {

CmdlineParser::CmdlineParser(OptionManager &option_manager) : 
    option_manager(option_manager) {
}

bool CmdlineParser::parse_argv(const std::vector<WTF8::u8string> &argv) const {
    if(argv.size() >= 5) {
        log_argv(argv);
        return analyze_argv(argv);
    } else {
        print_help(argv[0]);
        std::exit(argv.size() == 1 ? 0 : 1);
    }
}

void CmdlineParser::print_help(const WTF8::u8string &argv0) {
    WTF8::cerr << "Usage: "
               << argv0
               << " <output file> <input file> <STP> <note length> [<p1> <p2> <p3> <v1> <v2> <v3> [<v4> <overlap> <p4> [<p5> <v5>]]]"
               << std::endl << std::endl;
}

void CmdlineParser::log_argv(const std::vector<WTF8::u8string> &argv) {
    WTF8::clog << "Args: ";
    bool first = true;
    for(const auto &argi : argv) {
        if(first) {
            first = false;
        } else {
            WTF8::clog << ' ';
        }
        if(argi.size() != 0 && argi.find(' ') == argi.npos) {
            WTF8::clog << argi;
        } else {
            WTF8::clog << '"' << argi << '"';
        }
    }
    WTF8::clog << std::endl;
}

bool CmdlineParser::analyze_argv(const std::vector<WTF8::u8string> &argv) const {
    option_manager.output_file_name = argv[1];
    option_manager.input_file_name = argv[2];
    auto argc = argv.size();
    bool success = true;
    const auto parse_arg_number = [argc, &argv, &success](size_t argi, double factor, double def) -> double {
        if(argc > argi) {
            try {
                return strtonum(std::strtod, argv[argi].c_str())/factor;
            } catch(StrToNumError) {
                WTF8::cerr << "Invalid command line argument #" << argi << ": " << argv[argi] << std::endl;
                success = false;
            }
        }
        return def;
    };
    option_manager.stp = parse_arg_number(3, 1000, 0);
    try {
        option_manager.note_length = strtonum(std::strtod, argv[4].c_str())/1000;
    } catch(StrToNumError) {
        try {
            option_manager.note_length = parse_note_length(argv[4]);
        } catch(StrToNumError) {
            WTF8::cerr << "Invalid command line argument #" << 4 << ": " << argv[4] << std::endl;
            success = false;
        }
    }
    option_manager.p[1] = parse_arg_number(5, 1000, 0);
    option_manager.p[2] = parse_arg_number(6, 1000, 0);
    option_manager.p[3] = parse_arg_number(7, 1000, 0);
    option_manager.v[1] = parse_arg_number(8, 100, 1);
    option_manager.v[2] = parse_arg_number(9, 100, 1);
    option_manager.v[3] = parse_arg_number(10, 100, 1);
    option_manager.v[4] = parse_arg_number(11, 100, 1);
    option_manager.overlap = parse_arg_number(12, 1000, 0);
    option_manager.note_length -= option_manager.overlap;
    option_manager.p[4] = parse_arg_number(13, 1000, 0);
    if(argc > 15) {
        option_manager.p5_enabled = true;
        option_manager.p[5] = parse_arg_number(14, 1000, 0);
        option_manager.v[5] = parse_arg_number(15, 100, 1);
    } else
        option_manager.p5_enabled = false;
    return success;
}

double CmdlineParser::parse_note_length(const WTF8::u8string &argv4) {
    auto pos_at = argv4.find('@');
    if(pos_at != argv4.npos) {
        auto pos_sign = argv4.find_first_of("+-", pos_at+1);
        double ticks = strtonum(std::strtod, argv4.substr(0, pos_at).c_str());
        double tempo = strtonum(std::strtod, argv4.substr(pos_at+1, pos_sign != argv4.npos ? pos_sign-pos_at-1 : pos_sign).c_str());
        double note_length = tempo != 0 ? ticks/tempo/8 : 0;
        if(pos_sign != argv4.npos) {
            double frac = strtonum(std::strtod, &argv4.data()[pos_sign])/1000;
            note_length += frac;
#ifdef _WIN32
            if(frac == 0 && argv4.size() > pos_sign+1 && argv4[pos_sign+1] == '.' && // Format is similar to "+.00"
                WTF8::access("C:\\Windows\\system32\\wineboot.exe", 0) == 0)         // Is running under Wine
                WTF8::cerr << "Warning: UTAU is providing incorrect parameters," << std::endl
                           << "         please check \"Tools\" -> \"Option\" -> \"Rendering\" -> \"Note length " << std::endl
                           << "         calculated by GUI front-end\"." << std::endl;
#endif
        }
        return note_length;
    } else {
        throw StrToNumError();
    }
}

}

