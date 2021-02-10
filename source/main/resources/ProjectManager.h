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

namespace RoR {

/// A .truck file in a project directory.
struct ProjectTruck
{
    std::string prt_name; //!< Display name
    std::string prt_filename;
};

/// A subdirectory of ROR_HOME/projects
struct Project
{
    std::string   prj_name;
    std::string   prj_dirname;
    std::string   prj_rg_name; //!< OGRE resource group
    size_t        prj_format_version = 1;
    std::vector<ProjectTruck> prj_trucks;
    bool          prj_valid = false; //!< Is this object in sync with filesystem?
};

typedef std::vector<Project> ProjectVec;

class ProjectManager
{
public:
    // Project list management
    void                ReScanProjects();
    Project*            RegisterProjectDir(std::string const& dirname);
    void                ReLoadProjectDir(Project* proj);
    Project*            FindProjectByDirName(std::string const& dirname);
    void                PruneInvalidProjects();
    ProjectVec const&   GetProjects() const {return m_projects;}

    // Project handling
    Project*            CreateNewProject(std::string const& dirname, std::string const& project_name);
    bool                SaveProject(Project* proj);
    bool                ImportTruckToProject(std::string const& filename, std::shared_ptr<Truck::File> def); //!< Imports truckfile to current project + opens it
    void                ImportModuleToTruck(std::shared_ptr<Truck::File::Module> m); //!< Imports module (see 'sectionconfig') to current actor
    bool                SaveTruck();

    void                SetProject(Project* e) { m_entry = e; }

private:
    ProjectVec                      m_projects;
    Project*                        m_entry = nullptr;    //!< Currently open project
    ProjectTruck*                   m_snapshot = nullptr; //!< Currently open truck
    std::shared_ptr<Truck::File>    m_def;                //!< Currently open truck
};

} // namespace RoR
