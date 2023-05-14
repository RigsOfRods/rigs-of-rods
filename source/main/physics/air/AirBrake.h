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

#include "Application.h"

#include <Ogre.h>

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Aerial
/// @{

class Airbrake
{
    friend class RoR::ActorSpawner; // Creates AirbrakeGfx

private:

    struct CoVertice_t
    {
        Ogre::Vector3 vertex;
        Ogre::Vector3 normal;
        Ogre::Vector2 texcoord;
    };

    // Bindings
    ActorPtr m_actor;
    NodeNum_t noderef;
    NodeNum_t nodex;
    NodeNum_t nodey;
    NodeNum_t nodea;

    Ogre::MeshPtr msh;
    Ogre::SceneNode* snode;

    Ogre::Vector3 offset; //!< gfx attribute
    float ratio; //!< Current state
    float maxangle; //!< attribute from truckfile
    float area; //!< Attribute set at spawn
    Ogre::Entity* ec;

public:
    Airbrake(ActorPtr actor, const char* basename, int num, NodeNum_t ndref, NodeNum_t ndx, NodeNum_t ndy, NodeNum_t nda, Ogre::Vector3 pos, float width, float length, float maxang, std::string const & texname, float tx1, float tx2, float tx3, float tx4, float lift_coef);
    ~Airbrake() {} // Cleanup of visuals is done by GfxActor

    void updatePosition(float amount);
    void applyForce();
    float getRatio() const { return ratio; }
    float getMaxAngle() const { return maxangle; }
};

/// @} // addtogroup Aerial
/// @} // addtogroup Gameplay

} // namespace RoR
