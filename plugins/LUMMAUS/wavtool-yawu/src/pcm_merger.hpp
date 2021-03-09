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

#ifndef YAWU_PCM_MERGER_HPP
#define YAWU_PCM_MERGER_HPP

#include "option_manager.hpp"
#include "proxy_ptr.hpp"

namespace YAWU {

class PCMMerger {
public:
    PCMMerger(OptionManager &option_manager);
    ~PCMMerger();
    PCMMerger &prepare();
    PCMMerger &fill_overlap();
    PCMMerger &read_new_segment();
    PCMMerger &construct_envelope();
    PCMMerger &mix_new_segment();
    PCMMerger &write_back();
protected:
    OptionManager &option_manager;
private:
    struct Private;
    proxy_ptr<Private> p;
};

}

#endif
