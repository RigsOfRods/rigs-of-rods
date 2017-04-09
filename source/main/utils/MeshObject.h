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

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   1st of May 2010

#pragma once

#include <Ogre.h>

#include "Application.h"

class MeshObject : public Ogre::Resource::Listener, public ZeroedMemoryAllocator
{
public:
    MeshObject(Ogre::String meshName, Ogre::String entityRG, Ogre::String entityName, Ogre::SceneNode* sceneNode);

    void setMaterialName(Ogre::String m);
    void setCastShadows(bool b);
    void setVisible(bool b);
    inline Ogre::Entity*    getEntity() { return ent; };
    inline Ogre::SceneNode* GetSceneNode() { return sceneNode; }

protected:
    Ogre::SceneNode* sceneNode;
    Ogre::Entity* ent;
    Ogre::MeshPtr mesh;

    bool castshadows;

    void createEntity(Ogre::String meshName, Ogre::String entityRG, Ogre::String entityName);
};
