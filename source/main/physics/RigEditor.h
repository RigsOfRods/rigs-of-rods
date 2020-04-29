/*
    This source file is part of Rigs of Rods
    Copyright 2018 Petr Ohlidal & contributors

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

#include "CacheSystem.h" // RoR::ProjectEntry, PROJECT_FILE
#include "RigDef_File.h" // RigDef::File
#include <memory>

namespace RoR {

/// @class RigEditor
/// @brief A state machine to {load, modify + keep history, export, spawn} a softbody actor
/// Only 1 actor is loaded at a time
/// TERMINOLOGY:
///  * Project = directory with JSON, truckfiles and resources, located under ROR_HOME/projects
///  * Snapshot = a truckfile in project directory. NOTE: file extension is always .truck
///  * Module = a `RigDef::File::Module` object (see truckfile feature 'sectionconfig')
class RigEditor
{
public:
    bool                CreateProjet(std::string const& dir_name, std::string const& proj_name);
    bool                ImportSnapshotToProject(std::string const& filename, std::shared_ptr<RigDef::File> def); //!< Imports truckfile to current project + opens it
    void                ImportModuleToSnapshot(std::shared_ptr<RigDef::File::Module> m); //!< Imports module (see 'sectionconfig') to current actor
    bool                SaveSnapshot();
    void                LoadSnapshot(ProjectEntry* project, int snapshot);
    void                SetDefinition(std::shared_ptr<RigDef::File> def) { m_def = def; }
    void                AddExampleScriptToSnapshot(std::string const& filename);
    ProjectEntry*       GetProjectEntry() { return m_entry; }
    void                CloseProject();

    static bool         ReLoadProjectFromDirectory(ProjectEntry* proj);
    static bool         SaveProject(RoR::ProjectEntry* proj);

private:
    ProjectEntry*                   m_entry = nullptr;    //!< Currently open project
    int                             m_snapshot = -1;      //!< Currently open actor
    std::shared_ptr<RigDef::File>   m_def;                //!< Currently open actor
};

} // namespace RoR