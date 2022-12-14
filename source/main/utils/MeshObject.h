/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

/// @addtogroup Gfx
/// @{

class MeshObject
{
public:
    MeshObject(Ogre::String meshName, Ogre::String entityRG, Ogre::String entityName, Ogre::SceneNode* sceneNode);

    void setMaterialName(Ogre::String m);
    void setCastShadows(bool b);
    void setVisible(bool b);
    inline Ogre::Entity*    getEntity() { return m_entity; };
    inline Ogre::SceneNode* GetSceneNode() { return m_scene_node; }
    inline Ogre::MeshPtr    getLoadedMesh() { return m_mesh; }

protected:
    Ogre::SceneNode* m_scene_node = nullptr;
    Ogre::Entity* m_entity = nullptr;
    Ogre::MeshPtr m_mesh;
    bool m_cast_shadows = false;

    void createEntity(Ogre::String meshName, Ogre::String entityRG, Ogre::String entityName);
};

/// @} // @addtogroup Gfx
