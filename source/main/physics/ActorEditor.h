/*
    This source file is part of Rigs of Rods
    Copyright 2013-2020 Petr Ohlidal

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

#include "CacheSystem.h" // RoR::ProjectEntry, PROJECT_FILE
#include "TruckFileFormat.h" // Truck::File
#include <memory>

namespace RoR {

/// @class ActorEditor
/// @brief A state machine to {load, modify + keep history, export, spawn} a softbody actor
/// Only 1 actor is loaded at a time
/// TERMINOLOGY:
///  * Project = directory with JSON, truckfiles and resources, located under ROR_HOME/projects
///  * Snapshot = a truckfile in project directory. NOTE: file extension is always .truck
///  * Module = a `Truck::File::Module` object (see truckfile feature 'sectionconfig')
class ActorEditor
{
public:
    bool                ImportSnapshotToProject(std::string const& filename, std::shared_ptr<Truck::File> def); //!< Imports truckfile to current project + opens it
    void                ImportModuleToSnapshot(std::shared_ptr<Truck::File::Module> m); //!< Imports module (see 'sectionconfig') to current actor
    bool                SaveSnapshot();
    void                SetProject(ProjectEntry* e) { m_entry = e; }

    static bool         ReLoadProjectFromDirectory(ProjectEntry* proj);
    static bool         SaveProject(RoR::ProjectEntry* proj);

private:
    ProjectEntry*                   m_entry = nullptr;    //!< Currently open project
    ProjectSnapshot*                m_snapshot = nullptr; //!< Currently open actor
    std::shared_ptr<Truck::File>   m_def;                //!< Currently open actor
};

} // namespace RoR