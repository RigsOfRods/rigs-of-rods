/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "GroundModelFileformat.h"

#include "ConfigFile.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "RoRPrerequisites.h"
#include "Utils.h"

#include <OgreResourceGroupManager.h>

std::string TrimCopy(std::string copy)
{
    Ogre::StringUtil::trim(copy);
    return copy;
}

void RoR::GroundModelManager::LoadGroundModels(std::string const& filename)
{
    std::string group;
    try
    {
        group = Ogre::ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
    }
    catch (...)
    {} // The path may be global -> yields exception, but it's a valid case.

    if (group == "")
        m_conf.loadDirect(filename);
    else
        m_conf.loadFromResourceSystem(filename, group, "\x09:=", true);

    this->ParseGroundModels();
    this->ResolveDependencies();
}

void RoR::GroundModelManager::ParseGroundModels()
{
    Ogre::ConfigFile::SectionIterator section_itor = m_conf.getSectionIterator();
    while (section_itor.hasMoreElements())
    {
        const std::string section_name = section_itor.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap& section_entries = *section_itor.getNext();

        if (section_name == "") // Skip the default empty section
            continue;

        if (section_name == "general" || section_name == "config")
        {
            m_version = m_conf.GetInt("version", section_name);
            continue;
        }

        GroundModelDef* def = nullptr;
        auto search_res = m_models.find(section_name);
        if (search_res != m_models.end())
        {
            def = &search_res->second;
        }
        else
        {
            auto inserted = m_models.insert(std::make_pair(section_name, GroundModelDef()));
            def = &inserted.first->second;
            def->name = section_name;
        }

        for (auto& entry: section_entries)
        {
                 if (entry.first == "adhesion velocity")            { def->va                      = PARSEREAL(entry.second); }
            else if (entry.first == "static friction coefficient")  { def->ms                      = PARSEREAL(entry.second); }
            else if (entry.first == "sliding friction coefficient") { def->mc                      = PARSEREAL(entry.second); }
            else if (entry.first == "hydrodynamic friction")        { def->t2                      = PARSEREAL(entry.second); }
            else if (entry.first == "stribeck velocity")            { def->vs                      = PARSEREAL(entry.second); }
            else if (entry.first == "alpha")                        { def->alpha                   = PARSEREAL(entry.second); }
            else if (entry.first == "strength")                     { def->strength                = PARSEREAL(entry.second); }
            else if (entry.first == "base")                         { def->base_name               = RoR::Utils::SanitizeUtf8String(TrimCopy(entry.second));  }
            else if (entry.first == "fx_particle_name")             { def->particle_name           = RoR::Utils::SanitizeUtf8String(TrimCopy(entry.second));  }
            else if (entry.first == "fx_colour")                    { def->fx_colour               = Ogre::StringConverter::parseColourValue(entry.second); }
            else if (entry.first == "fx_particle_amount")           { def->fx_particle_amount      = PARSEINT(entry.second);  }
            else if (entry.first == "fx_particle_min_velo")         { def->fx_particle_min_velo    = PARSEREAL(entry.second); }
            else if (entry.first == "fx_particle_max_velo")         { def->fx_particle_max_velo    = PARSEREAL(entry.second); }
            else if (entry.first == "fx_particle_fade")             { def->fx_particle_fade        = PARSEREAL(entry.second); }
            else if (entry.first == "fx_particle_timedelta")        { def->fx_particle_timedelta   = PARSEREAL(entry.second); }
            else if (entry.first == "fx_particle_velo_factor")      { def->fx_particle_velo_factor = PARSEREAL(entry.second); }
            else if (entry.first == "fx_particle_ttl")              { def->fx_particle_ttl         = PARSEREAL(entry.second); }
            else if (entry.first == "fluid density")                { def->fluid_density           = PARSEREAL(entry.second); }
            else if (entry.first == "flow consistency index")       { def->flow_consistency_index  = PARSEREAL(entry.second); }
            else if (entry.first == "flow behavior index")          { def->flow_behavior_index     = PARSEREAL(entry.second); }
            else if (entry.first == "solid ground level")           { def->solid_ground_level      = PARSEREAL(entry.second); }
            else if (entry.first == "drag anisotropy")              { def->drag_anisotropy         = PARSEREAL(entry.second); }
            else if (entry.first == "fx_type")
            {
                     if (entry.second == "PARTICLE") { def->fx_type = GroundModelDef::FxSurfaceType::FX_PARTICLE; }
                else if (entry.second == "HARD")     { def->fx_type = GroundModelDef::FxSurfaceType::FX_HARD;     }
                else if (entry.second == "DUSTY")    { def->fx_type = GroundModelDef::FxSurfaceType::FX_DUSTY;    }
                else if (entry.second == "CLUMPY")   { def->fx_type = GroundModelDef::FxSurfaceType::FX_CLUMPY;   }
                else
                {
                    m_messages.push_back(std::string("Invalid 'fx_type' value '" + entry.second + "' in groundmodel '" + section_name + "'"));
                }
            }
            else
            {
                m_messages.push_back(std::string("Invalid key '" + entry.first + "' in groundmodel '" + section_name + "'"));
            }
        }
    }
}

RoR::GroundModelDef* RoR::GroundModelManager::GetGroundModel(const char* name)
{
    auto search_res = m_models.find(name);
    if (search_res != m_models.end())
        return &search_res->second;
    else
        return nullptr;
}

void RoR::GroundModelManager::ResolveDependencies()
{
    for (auto& entry: m_models)
    {
        if (entry.second.base_name.empty())
            continue; // No base to derive from

        auto search_res = m_models.find(entry.second.base_name);
        if (search_res == m_models.end())
        {
            m_messages.push_back("Could not resolve dependency '"
                + entry.second.base_name + "' in groundmodel '" + entry.first + "'");
            continue;
        }

        entry.second.CopyFrom(search_res->second); // Overrides all settings - needs reload from file
    }

    // Re-load settings from files.
    // This makes no difference for groundmodels without base and fixes those overriden by base.
    this->ParseGroundModels();

    // check the version
    if (m_version != LATEST_GROUND_MODEL_VERSION)
    {
        ErrorUtils::ShowError(_L("Configuration error"),
            _L("Your ground configuration is too old, please copy skeleton/config/ground_models.cfg to My Documents/Rigs of Rods/config"));
        exit(124); // TODO: Make this more subtle ~ only_a_ptr, 04/2017
    }
}

