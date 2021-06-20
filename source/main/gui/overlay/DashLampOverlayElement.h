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

/// Simple panel which switches 2 materials: on/off.
/// Materials must have suffix "-on" and "-off". One must be specified in overlay script - the other will be deduced.
class DashLampOverlayElement:
    public Ogre::PanelOverlayElement,
    public BaseDashOverlayElement
{
public:
    static const Ogre::String OVERLAY_ELEMENT_TYPE_NAME;

    DashLampOverlayElement(const Ogre::String& name);
    virtual ~DashLampOverlayElement() {}

    virtual const Ogre::String& getTypeName(void) const override;
    bool                        checkMaterialsOk() const;
    void                        setLampOn(bool on);

private:
    void locateMaterials();

    Ogre::MaterialPtr m_off_material;
    Ogre::MaterialPtr m_on_material;
};

/// Mandatory factory class
class DashLampOverlayElementFactory: public Ogre::OverlayElementFactory
{
public:
    virtual Ogre::OverlayElement* createOverlayElement(const Ogre::String& instanceName) override;
    virtual const Ogre::String& getTypeName() const override;
};

} // namespace RoR
