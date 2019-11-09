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

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   1st of May 2010

#include "MeshObject.h"

#include "Application.h"
#include "MeshLodGenerator/OgreMeshLodGenerator.h"

using namespace Ogre;

MeshObject::MeshObject(Ogre::String meshName, Ogre::String meshRG, Ogre::String entityName, Ogre::SceneNode *sceneNode)
    : sceneNode(sceneNode), ent(nullptr), castshadows(true)
{
    this->createEntity(meshName, meshRG, entityName);
}

void MeshObject::setMaterialName(Ogre::String m)
{
    if (ent) { ent->setMaterialName(m); }
}

void MeshObject::setCastShadows(bool b)
{
    castshadows = b;
    if (sceneNode && sceneNode->numAttachedObjects()) { sceneNode->getAttachedObject(0)->setCastShadows(b); }
}

void MeshObject::setVisible(bool b)
{
    if (sceneNode) sceneNode->setVisible(b);
}

void MeshObject::createEntity(Ogre::String meshName, Ogre::String meshRG, Ogre::String entityName)
{
    if (!sceneNode) return;

    try
    {
        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().load(meshName, meshRG);

        // important: you need to add the LODs before creating the entity
        // now find possible LODs, needs to be done before calling createEntity()
        String basename, ext;
        StringUtil::splitBaseFilename(meshName, basename, ext);

        Ogre::String group = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        // the classic LODs
        FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, basename + "_lod*.mesh");
        for (FileInfoList::iterator iterFiles = files->begin(); iterFiles != files->end(); ++iterFiles)
        {
            String format = basename + "_lod%d.mesh";
            int    i      = -1;
            int    r      = sscanf(iterFiles->filename.c_str(), format.c_str(), &i);

            if (r <= 0 || i < 0) continue;

            Ogre::MeshManager::getSingleton().load(iterFiles->filename, mesh->getGroup());
        }

        // the custom LODs
        FileInfoListPtr files2 = ResourceGroupManager::getSingleton().findResourceFileInfo(group, basename + "_clod_*.mesh");
        for (FileInfoList::iterator iterFiles = files2->begin(); iterFiles != files2->end(); ++iterFiles)
        {
            // and custom LODs
            String format = basename + "_clod_%d.mesh";
            int    i      = -1;
            int    r      = sscanf(iterFiles->filename.c_str(), format.c_str(), &i);
            if (r <= 0 || i < 0) continue;

            Ogre::MeshManager::getSingleton().load(iterFiles->filename, mesh->getGroup());
        }

        // now create an entity around the mesh and attach it to the scene graph

        ent = gEnv->sceneManager->createEntity(entityName, meshName, meshRG);
        ent->setCastShadows(castshadows);

        sceneNode->attachObject(ent);
        sceneNode->setVisible(true);
    }
    catch (Ogre::Exception &e)
    {
        RoR::LogFormat("[RoR] Error creating entity of mesh '%s' (group: '%s'), message: %s", meshName.c_str(), meshRG.c_str(),
                       e.getFullDescription().c_str());
        return;
    }
}
