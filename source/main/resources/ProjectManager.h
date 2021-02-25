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

#include "TruckFileFormat.h"
#include "Project.h"

#include <memory>
#include <string>
#include <vector>

#if USE_EFSW
#   include <efsw/efsw.hpp>
#endif

namespace RoR {

class ProjectManager
#if USE_EFSW
                    : efsw::FileWatchListener
#endif
{
public:
    // Project list management
    void                ReScanProjects();
    ProjectPtr          RegisterProjectDir(std::string const& dirname);
    void                ReScanProjectDir(ProjectPtr proj);
    ProjectPtr          FindProjectByDirName(std::string const& dirname);
    void                PruneInvalidProjects();
    ProjectPtrVec&      GetProjects() {return m_projects;}

    // Project handling
    ProjectPtr          CreateNewProject(std::string const& dirname);
    bool                ImportTruckToProject(std::string const& filename, Truck::DocumentPtr def, CacheEntry* entry); //!< Imports truckfile to current project + opens it
    void                ImportModuleToTruck(Truck::ModulePtr m); //!< Imports module (see 'sectionconfig') to current actor
    bool                SaveTruck();
    void                ReLoadResources(ProjectPtr project);

    // Active project
    void                SetActiveProject(ProjectPtr, std::string const& filename);
    ProjectPtr          GetActiveProject() { return m_active_project; }

#if USE_EFSW
    void                handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename = "" ) override; //!< efsw::FileWatchListener callback, invoked on EFSW monitoring thread.
    void                HandleFileSystemEvent(ProjectFsEvent* fs_event);
    void                StartWatchingFileSystem();
    void                StopWatchingFileSystem();
#endif

private:
    std::string         MakeFilenameUniqueInProject(std::string const& src_filename);

    ProjectPtrVec         m_projects;
    ProjectPtr            m_active_project;
    Ogre::String          m_active_truck_filename;
    Truck::DocumentPtr    m_active_truck_def;

#if USE_EFSW
    std::unique_ptr<efsw::FileWatcher> m_watcher;
    efsw::WatchID                      m_watch_id = 0; // Valid watches are >0, error states are <0
#endif
};

} // namespace RoR
