/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
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

#include "DashPanelOverlayElement.h"

#include "Console.h"

#include <fmt/format.h>
#include <OgreTextAreaOverlayElement.h>
#include <Ogre.h>

using namespace RoR;
using namespace Ogre;

// --------------------------------
// The element

    // Static members

const String DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME("DashPanel");

    // Construction/destruction

DashPanelOverlayElement::DashPanelOverlayElement(const String& name)
    : Ogre::PanelOverlayElement(name)
{
    // Register this class in OGRE parameter system, see Ogre::StringInterface.
    this->createParamDictionary(OVERLAY_ELEMENT_TYPE_NAME);
    // Add Ogre::TextAreaOverlayElement (and ancestor classes) parameters.
    this->addBaseParameters();
    // Add Rigs of Rods dashboard parameters.
    this->addCommonDashParams(this->getParamDictionary());
}

    // Functions

const Ogre::String& DashPanelOverlayElement::getTypeName(void) const
{
    return OVERLAY_ELEMENT_TYPE_NAME;
}

    // Animation "lamp" functions

void DashPanelOverlayElement::locateMaterials()
{
    if (!this->getMaterial())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("No material assigned - lamp '{}' will not be animated", this->getName()));
    }

    /// Materials must have suffix "-on" and "-off". One must be specified in overlay script - the other will be deduced.
    if (Ogre::StringUtil::endsWith(this->getMaterial()->getName(), "-on"))
    {
        m_on_material = this->getMaterial();
        Ogre::String base_name = this->getMaterial()->getName().substr(0, this->getMaterial()->getName().length() - 3);
        m_off_material = Ogre::MaterialManager::getSingleton().getByName(base_name + "-off"); //FIXME: resource group!!
    }
    else if (Ogre::StringUtil::endsWith(this->getMaterial()->getName(), "-off"))
    {
        m_off_material = this->getMaterial();
        Ogre::String base_name = this->getMaterial()->getName().substr(0, this->getMaterial()->getName().length() - 4);
        m_on_material = Ogre::MaterialManager::getSingleton().getByName(base_name + "-on"); //FIXME: resource group!!
    }
    else
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Unrecognized material '{}' - lamp '{}' will not be animated",
                this->getMaterial()->getName(), this->getName()));
    }
}

bool DashPanelOverlayElement::checkMaterialsOk() const
{
    return m_on_material && m_off_material;
}

void DashPanelOverlayElement::setLampOn(bool on)
{
    this->setMaterial(on ? m_on_material : m_off_material);
}

    // Animation "series" functions

bool DashPanelOverlayElement::setupAnimSeries()
{
    /// Materials must end by integer. One must be specified in overlay script - the other will be deduced.

    // No material? Return error.
    if (!this->getMaterial())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("No material assigned - series '{}' will not be animated", this->getName()));
        return false;
    }

    // Material not ending with number? return error.
    int initial_num = this->analyzeSeriesMaterial(this->getMaterial()->getName());
    if (initial_num < 0)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Assigned material has no number - series '{}' will not be animated", this->getName()));
        return false;
    }

    // Find all available materials, starting with 0, ending when a number is not found.
    bool keep_searching = true;
    int find_num = 0;
    while (keep_searching)
    {
        Ogre::String mat_name = fmt::format("{}{}", m_series_base_name, find_num);
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(mat_name);//FIXME: resource group!!
        if (mat)
        {
            m_series_materials.push_back(mat);
            find_num++;
        }
        else
        {
            keep_searching = false;
        }
    }

    if (m_series_materials.size() < 2)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Only 1 material found '{}' - series '{}' will not be animated",
                this->getMaterial()->getName(), this->getName()));
        return false;
    }

    return true;
}

void DashPanelOverlayElement::updateAnimSeries(int val)
{
    if (val >= 0 && val < (int)m_series_materials.size())
    {
        this->setMaterial(m_series_materials[val]);
    }
    else
    {
        RoR::LogFormat("[RoR] dash element '%s' (anim series) can't display %d - only %d materials are available",
            this->getName().c_str(), val, (int)m_series_materials.size());
    }
}

int DashPanelOverlayElement::analyzeSeriesMaterial(Ogre::String material_name)
{
    size_t num_start = material_name.length();
    while (num_start > 0 && isdigit(material_name[num_start - 1]))
    {
        num_start--;
    }

    if (num_start == material_name.length())
    {
        return -1; // No number!
    }
    else
    {
        m_series_base_name = material_name.substr(0, num_start);
        return atoi(material_name.substr(num_start).c_str());
    }
}

// --------------------------------
// The factory

Ogre::OverlayElement* DashPanelOverlayElementFactory::createOverlayElement(const Ogre::String& instanceName)
{
    return OGRE_NEW DashPanelOverlayElement(instanceName);
}

const Ogre::String& DashPanelOverlayElementFactory::getTypeName() const
{
    return DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME;
}

