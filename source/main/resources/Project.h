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

#include <Ogre.h>
#include <memory>

#if USE_EFSW
#   include <efsw/efsw.hpp>
#endif

namespace RoR {

enum class ProjectSyncState
{
    UNKNOWN,
    DELETED,  //!< Directory deleted from filesystem
    OK        //!< In sync with filesystem
};

/// A subdirectory of ROR_HOME/projects
struct Project
{
    std::string           prj_dirname;
    std::string           prj_rg_name; //!< OGRE resource group
    Ogre::StringVectorPtr prj_trucks; //!< Filenames
    ProjectSyncState      prj_sync = ProjectSyncState::UNKNOWN;
};

typedef std::shared_ptr<Project> ProjectPtr;

typedef std::vector<ProjectPtr> ProjectPtrVec;

enum class ProjectFsAction
{
    NONE,
    PROJECT_ADDED,
    PROJECT_RENAMED,
    PROJECT_DELETED,
    TRUCK_ADDED,
    TRUCK_MODIFIED,
    TRUCK_RENAMED,
    TRUCK_DELETED,
    RESOURCE_ADDED,
    RESOURCE_MODIFIED,
    RESOURCE_RENAMED,
    RESOURCE_DELETED
};

struct ProjectFsEvent
{
    ProjectFsAction pfe_action = ProjectFsAction::NONE;
    std::string     pfe_project_dir;
    std::string     pfe_new_name;          //!< May refer to dir or filename, depending on action
    std::string     pfe_filename;
};

} // namespace RoR
