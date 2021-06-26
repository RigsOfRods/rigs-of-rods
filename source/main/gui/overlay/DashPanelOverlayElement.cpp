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

