/*
 * ProjectGitManager.h - Git integration helper class
 *
 * Copyright (c) 2024-2026 Dalton Messmer <messmer.dalton/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#ifndef PROJECT_GIT_MANAGER_H
#define PROJECT_GIT_MANAGER_H

#include <libgit2.h>

namespace lmms {

class ProjectGitManager {
public:
    ProjectGitManager();
    ~ProjectGitManager();

    // ...
};

} // namespace lmms

#endif // PROJECT_GIT_MANAGER_H