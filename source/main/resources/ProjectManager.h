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

#include "TruckFileFormat.h"
#include <memory>
#include <string>
#include <vector>

#define PROJECT_FILE "project.json"
#define PROJECT_FILE_FORMAT "1"

namespace RoR {

/// A subdirectory of ROR_HOME/projects
struct Project
{
    std::string   prj_dirname;
    std::string   prj_rg_name; //!< OGRE resource group
    Ogre::StringVectorPtr prj_trucks; //!< Filenames
    bool          prj_valid = false; //!< Is this object in sync with filesystem?
};

typedef std::vector<Project> ProjectVec;

class ProjectManager
{
public:
    // Project list management
    void                ReScanProjects();
    Project*            RegisterProjectDir(std::string const& dirname);
    void                ReScanProjectDir(Project* proj);
    Project*            FindProjectByDirName(std::string const& dirname);
    void                PruneInvalidProjects();
    ProjectVec&         GetProjects() {return m_projects;}

    // Project handling
    Project*            CreateNewProject(std::string const& dirname);
    bool                ImportTruckToProject(std::string const& filename, std::shared_ptr<Truck::File> def, CacheEntry* entry); //!< Imports truckfile to current project + opens it
    void                ImportModuleToTruck(std::shared_ptr<Truck::File::Module> m); //!< Imports module (see 'sectionconfig') to current actor
    bool                SaveTruck();
    void                ReLoadResources(Project* project);

    void                SetActiveProject(Project* e) { m_active_project = e; }

private:
    ProjectVec                      m_projects;
    Project*                        m_active_project = nullptr;
    Ogre::String                    m_active_truck_filename;
    std::shared_ptr<Truck::File>    m_active_truck_def;
};

} // namespace RoR
