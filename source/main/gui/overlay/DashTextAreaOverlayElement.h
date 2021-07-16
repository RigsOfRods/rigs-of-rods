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

/// @file
/// @brief Simple text area which displays dashboard data.

#pragma once

#include "BaseDashOverlayElement.h"

#include <Ogre.h>
#include <OgreOverlay.h>
#include <OgreOverlayElementFactory.h>

namespace RoR {

/// Simple text area which displays dashboard data.
class DashTextAreaOverlayElement:
    public Ogre::TextAreaOverlayElement,
    public BaseDashOverlayElement
{
public:
    static const Ogre::String OVERLAY_ELEMENT_TYPE_NAME;

    DashTextAreaOverlayElement(const Ogre::String& name);
    virtual ~DashTextAreaOverlayElement() {}

    virtual const Ogre::String& getTypeName(void) const override;
};

/// Mandatory factory class
class DashTextAreaOverlayElementFactory: public Ogre::OverlayElementFactory
{
public:
    virtual Ogre::OverlayElement* createOverlayElement(const Ogre::String& instanceName) override;
    virtual const Ogre::String& getTypeName() const override;
};

} // namespace RoR
