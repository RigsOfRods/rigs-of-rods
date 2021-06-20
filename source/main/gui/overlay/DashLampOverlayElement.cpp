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

#include "DashLampOverlayElement.h"

#include "Console.h"

#include <fmt/format.h>
#include <OgreTextAreaOverlayElement.h>
#include <Ogre.h>

using namespace RoR;
using namespace Ogre;

// --------------------------------
// The element

    // Static members

const String DashLampOverlayElement::OVERLAY_ELEMENT_TYPE_NAME("DashLamp");

    // Construction/destruction

DashLampOverlayElement::DashLampOverlayElement(const String& name)
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

const Ogre::String& DashLampOverlayElement::getTypeName(void) const
{
    return OVERLAY_ELEMENT_TYPE_NAME;
}

// Type-specific functions

void DashLampOverlayElement::locateMaterials()
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
        m_off_material = Ogre::MaterialManager::getSingleton().getByName(base_name + "-off");
    }
    else if (Ogre::StringUtil::endsWith(this->getMaterial()->getName(), "-off"))
    {
        m_off_material = this->getMaterial();
        Ogre::String base_name = this->getMaterial()->getName().substr(0, this->getMaterial()->getName().length() - 4);
        m_on_material = Ogre::MaterialManager::getSingleton().getByName(base_name + "-on");
    }
    else
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Unrecognized material '{}' - lamp '{}' will not be animated",
                this->getMaterial()->getName(), this->getName()));
    }
}

bool DashLampOverlayElement::checkMaterialsOk() const
{
    return m_on_material && m_off_material;
}

void DashLampOverlayElement::setLampOn(bool on)
{
    this->setMaterial(on ? m_on_material : m_off_material);
}

// --------------------------------
// The factory

Ogre::OverlayElement* DashLampOverlayElementFactory::createOverlayElement(const Ogre::String& instanceName)
{
    return OGRE_NEW DashLampOverlayElement(instanceName);
}

const Ogre::String& DashLampOverlayElementFactory::getTypeName() const
{
    return DashLampOverlayElement::OVERLAY_ELEMENT_TYPE_NAME;
}

