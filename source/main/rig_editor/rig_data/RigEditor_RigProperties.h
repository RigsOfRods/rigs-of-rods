/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

/** 
    @file   RigEditor_RigProperties.h
    @date   09/2014
    @author Petr Ohlidal
*/

#pragma once

#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RoRPrerequisites.h"

#include <memory>

namespace RoR {
namespace RigEditor {

struct RigModuleData; // Forward decl;

class RigProperties
{
    friend class RigEditor::Rig;
    friend class GUI::RigEditorRigPropertiesWindow;

    RigProperties();

public:

    ~RigProperties();

    std::shared_ptr<RigDef::Engine>    GetEngine();
    std::shared_ptr<RigDef::Engoption> GetEngoption();
    inline void SetEngine(std::shared_ptr<RigDef::Engine> engine) { m_engine = engine; }
    inline void SetEngoption(std::shared_ptr<RigDef::Engoption> engoption) { m_engoption = engoption; }

protected:	

    void Import(std::shared_ptr<RigDef::File> def_file);
    void Export(std::shared_ptr<RigDef::File> def_file);

    // BASIC tab
    
    Ogre::String                  m_title;
    Ogre::String                  m_guid;
    RigDef::Fileinfo              m_fileinfo;
    std::list<Ogre::String>       m_description;
    std::list<RigDef::Author>     m_authors;
    bool                          m_hide_in_chooser;
    bool                          m_forward_commands;
    bool                          m_import_commands;
    bool                          m_is_rescuer;

    // AUDIO & VIDEO tab

    RigDef::ExtCamera             m_extcamera;
    bool                          m_disable_default_sounds;
    Ogre::String                  m_globals_cab_material_name;
    RigDef::SkeletonSettings      m_skeleton_settings;

    // PHYSICS tab

    float                         m_globals_dry_mass;
    float                         m_globals_load_mass;
    float                         m_minimass;
    bool                          m_enable_advanced_deformation;
    bool                          m_rollon;

    // GUI tab

    Ogre::String                  m_help_panel_material_name;
    RigDef::GuiSettings           m_gui_settings;

    // LAND VEHICLE window

    std::shared_ptr<RigDef::Engine>      m_engine;
    std::shared_ptr<RigDef::Engoption>   m_engoption;
};

} // namespace RigEditor

} // namespace RoR
