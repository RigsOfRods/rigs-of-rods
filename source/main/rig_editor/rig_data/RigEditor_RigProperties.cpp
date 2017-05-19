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
    @file   RigEditor_RigProperties.cpp
    @date   09/2014
    @author Petr Ohlidal
*/

#include "RigDef_File.h"
#include "RigEditor_RigProperties.h"

using namespace RoR;
using namespace RoR::RigEditor;

RigProperties::RigProperties():
    m_hide_in_chooser(false),
    m_forward_commands(false),
    m_import_commands(false),
    m_is_rescuer(false),
    m_disable_default_sounds(false),
    m_globals_dry_mass(0.f), // This is a default
    m_globals_load_mass(0.f), // This is a default
    m_minimass(0.f), // This is a default
    m_enable_advanced_deformation(true),
    m_rollon(false)
{}

RigProperties::~RigProperties()
{}

void RigProperties::Import(std::shared_ptr<RigDef::File> def_file)
{
    m_title             = def_file->name;
    m_guid              = def_file->guid;
    m_hide_in_chooser   = def_file->hide_in_chooser;
    m_forward_commands  = def_file->forward_commands;
    m_import_commands   = def_file->import_commands;
    m_is_rescuer        = def_file->rescuer;
    m_rollon            = def_file->rollon;
    m_minimass          = def_file->minimum_mass;

    m_enable_advanced_deformation   = def_file->enable_advanced_deformation;
    m_disable_default_sounds        = def_file->disable_default_sounds;

    auto desc_end = def_file->description.end();
    for (auto itor = def_file->description.begin(); itor != desc_end; ++itor )
    {
        m_description.push_back(*itor);
    }

    auto authors_end = def_file->authors.end();
    for (auto itor = def_file->authors.begin(); itor != authors_end; ++itor )
    {
        m_authors.push_back(*itor);
    }

    RigDef::Fileinfo* fileinfo = def_file->file_info.get();
    if (fileinfo != nullptr)
    {
        m_fileinfo = *fileinfo;
    }

    RigDef::ExtCamera* ext_camera = def_file->root_module->ext_camera.get();
    if (ext_camera != nullptr)
    {
        m_extcamera = *ext_camera;
    }
    
    RigDef::Globals* globals = def_file->root_module->globals.get();
    if (globals != nullptr)
    {
        m_globals_cab_material_name = globals->material_name;
        m_globals_dry_mass          = globals->dry_mass;
        m_globals_load_mass         = globals->cargo_mass;
    }
    
    m_skeleton_settings  = def_file->root_module->skeleton_settings;

    // Engine + Engoption
    m_engine    = def_file->root_module->engine;
    m_engoption = def_file->root_module->engoption;
}

void RigProperties::Export(std::shared_ptr<RigDef::File> def_file)
{
    def_file->name              = m_title;           
    def_file->guid              = m_guid;            
    def_file->hide_in_chooser   = m_hide_in_chooser; 
    def_file->forward_commands  = m_forward_commands;
    def_file->import_commands   = m_import_commands; 
    def_file->rescuer           = m_is_rescuer;      
    def_file->rollon            = m_rollon;          
    def_file->minimum_mass      = m_minimass;  

    def_file->enable_advanced_deformation = m_enable_advanced_deformation;
    def_file->disable_default_sounds      = m_disable_default_sounds;

    def_file->file_info 
        = std::shared_ptr<RigDef::Fileinfo>(new RigDef::Fileinfo(m_fileinfo));
    def_file->root_module->ext_camera 
        = std::shared_ptr<RigDef::ExtCamera>(new RigDef::ExtCamera(m_extcamera));
    def_file->root_module->skeleton_settings = m_skeleton_settings;

    // Globals
    RigDef::Globals globals;
    globals.cargo_mass               = m_globals_load_mass;
    globals.dry_mass                 = m_globals_dry_mass;
    globals.material_name            = m_globals_cab_material_name;
    def_file->root_module->globals   = std::shared_ptr<RigDef::Globals>(new RigDef::Globals(globals));

    // Description
    for (auto itor = m_description.begin(); itor != m_description.end(); ++itor)
    {
        def_file->description.push_back(*itor);
    }

    // Authors
    for (auto itor = m_authors.begin(); itor != m_authors.end(); ++itor)
    {
        def_file->authors.push_back(*itor);
    }

    // Engine + Engoption
    def_file->root_module->engine      = m_engine;
    def_file->root_module->engoption   = m_engoption;
}

std::shared_ptr<RigDef::Engine> RigProperties::GetEngine()
{
    return m_engine;
}

std::shared_ptr<RigDef::Engoption> RigProperties::GetEngoption()
{
    return m_engoption;
}

