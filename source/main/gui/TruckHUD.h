/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#pragma once

#include "RoRPrerequisites.h"

#include <OgreOverlay.h>
#include <OgreOverlayElement.h>

#include <memory>

class TruckHUD : public ZeroedMemoryAllocator
{
public:

    TruckHUD();

    bool update(float dt, Beam* truck, bool visible = true);
    void show(bool value);
    bool isVisible();
    void initTorqueOverlay();

protected:

    Ogre::Overlay* truckHUD;
    float updatetime;
    int width, border;

    std::map<int, float> avVelos;
    std::map<int, float> maxNegLatG;
    std::map<int, float> maxNegSagG;
    std::map<int, float> maxNegVerG;
    std::map<int, float> maxPosLatG;
    std::map<int, float> maxPosSagG;
    std::map<int, float> maxPosVerG;
    std::map<int, float> maxVelos;
    std::map<int, float> minVelos;

    void checkOverflow(Ogre::OverlayElement* e);

    static std::unique_ptr<TruckHUD> myInstance;
    static const unsigned int COMMANDS_VISIBLE = 25;

    Ogre::String lastTorqueModel; //!< name of the last used torque model, needed to detect a change in the model
    Ogre::Real lastTorqueRatio; //!< last RPM ratio, used to clear the last torque peak
};
