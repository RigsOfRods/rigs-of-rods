/*
    This source file is part of Rigs of Rods
    Copyright 2013-2021 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file

#pragma once

#if USE_EFSW

#include "Application.h"

#include <efsw/efsw.hpp>

namespace RoR {

class ProjectFileWatcher: efsw::FileWatchListener
{
public:
    void                StartWatching(std::string const& dirname, std::string const& filename);
    void                StopWatching();

    // efsw::FileWatchListener
    void                handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename = "" ) override;

private:
    std::unique_ptr<efsw::FileWatcher> m_watcher;
    efsw::WatchID                      m_watch_id = 0; // Valid watches are >0, error states are <0
};

} // namespace RoR

#endif // USE_EFSW
