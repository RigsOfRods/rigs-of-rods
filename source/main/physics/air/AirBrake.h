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

#include <OgreMesh.h>

class Airbrake : public ZeroedMemoryAllocator
{
    friend class RigInspector; // Debug utility class

private:

    struct CoVertice_t
    {
        Ogre::Vector3 vertex;
        Ogre::Vector3 normal;
        Ogre::Vector2 texcoord;
    };

    Ogre::MeshPtr msh;
    Ogre::SceneNode* snode;
    node_t* noderef;
    node_t* nodex;
    node_t* nodey;
    node_t* nodea;
    Ogre::Vector3 offset;
    float ratio;
    float maxangle;
    float area;

    Ogre::Entity* ec;

public:
    Airbrake(const char* basename, int num, node_t* ndref, node_t* ndx, node_t* ndy, node_t* nda, Ogre::Vector3 pos, float width, float length, float maxang, std::string const & texname, float tx1, float tx2, float tx3, float tx4, float lift_coef);
    ~Airbrake();

    void updatePosition(float amount);
    void applyForce();
};
