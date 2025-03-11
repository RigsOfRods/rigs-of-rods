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

#include "Actor.h"
#include "Application.h"
#include "GfxScene.h"
#include "Terrain.h"

#include <OgreMeshLodGenerator.h>
#include <OgreLodConfig.h>

using namespace Ogre;
using namespace RoR;

MeshObject::MeshObject(Ogre::String meshName, Ogre::String entityRG, Ogre::String entityName, Ogre::SceneNode *m_scene_node)
    : m_scene_node(m_scene_node), m_entity(nullptr), m_cast_shadows(true)
{
    this->createEntity(meshName, entityRG, entityName);
}

void MeshObject::setMaterialName(Ogre::String m)
{
    if (m_entity)
    {
        m_entity->setMaterialName(m);
    }
}

void MeshObject::setCastShadows(bool b)
{
    m_cast_shadows = b;
    if (m_scene_node && m_scene_node->numAttachedObjects())
    {
        m_scene_node->getAttachedObject(0)->setCastShadows(b);
    }
}

void MeshObject::setVisible(bool b)
{
    // Workaround: if the scenenode is not used (entity not attached) for some reason, try hiding the entity directly.
    if (m_scene_node && m_scene_node->getAttachedObjects().size() > 0)
        m_scene_node->setVisible(b);
    else if (m_entity)
        m_entity->setVisible(b);
}

void MeshObject::createEntity(Ogre::String meshName, Ogre::String entityRG, Ogre::String entityName)
{
    if (!m_scene_node)
        return;

    try
    {
        m_mesh = Ogre::MeshManager::getSingleton().getByName(meshName, entityRG);

        // Mesh hasn't been loaded yet
        if (m_mesh == nullptr)
        {
            m_mesh = Ogre::MeshManager::getSingleton().load(meshName, entityRG);

            // important: you need to add the LODs before creating the entity
            // now find possible LODs, needs to be done before calling createEntity()
            String basename, ext;
            StringUtil::splitBaseFilename(meshName, basename, ext);

            bool lod_available = false;
            Ogre::LodConfig config(m_mesh);

            // the classic LODs
            FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(entityRG, basename + "_lod*.mesh");
            for (FileInfoList::iterator iterFiles = files->begin(); iterFiles != files->end(); ++iterFiles)
            {
                String format = basename + "_lod%d.mesh";
                int i = -1;
                int r = sscanf(iterFiles->filename.c_str(), format.c_str(), &i);

                if (r <= 0 || i < 0)
                    continue;

                float distance = 3;

                // we need to tune this according to our sightrange
                if (App::gfx_sight_range->getInt() > Terrain::UNLIMITED_SIGHTRANGE)
                {
                    // unlimited
                    if (i == 1)
                        distance = 200;
                    else if (i == 2)
                        distance = 600;
                    else if (i == 3)
                        distance = 2000;
                    else if (i == 4)
                        distance = 5000;
                }
                else
                {
                    // limited
                    int sightrange = App::gfx_sight_range->getInt();
                    if (i == 1)
                        distance = std::max(20.0f, sightrange * 0.1f);
                    else if (i == 2)
                        distance = std::max(20.0f, sightrange * 0.2f);
                    else if (i == 3)
                        distance = std::max(20.0f, sightrange * 0.3f);
                    else if (i == 4)
                        distance = std::max(20.0f, sightrange * 0.4f);
                }
                config.createManualLodLevel(distance, iterFiles->filename);
                lod_available = true;
            }

            // the custom LODs
            FileInfoListPtr files2 = ResourceGroupManager::getSingleton().findResourceFileInfo(entityRG, basename + "_clod_*.mesh");
            for (FileInfoList::iterator iterFiles = files2->begin(); iterFiles != files2->end(); ++iterFiles)
            {
                // and custom LODs
                String format = basename + "_clod_%d.mesh";
                int i = -1;
                int r = sscanf(iterFiles->filename.c_str(), format.c_str(), &i);
                if (r <= 0 || i < 0)
                    continue;

                config.createManualLodLevel(i, iterFiles->filename);
                lod_available = true;
            }

            if (lod_available)
                Ogre::MeshLodGenerator::getSingleton().generateLodLevels(config);
            else if (App::gfx_auto_lod->getBool())
                Ogre::MeshLodGenerator::getSingleton().generateAutoconfiguredLodLevels(m_mesh);
        }

        // now create an entity around the mesh and attach it to the scene graph
        m_entity = App::GetGfxScene()->GetSceneManager()->createEntity(entityName, meshName, entityRG);
        m_entity->setCastShadows(m_cast_shadows);

        m_scene_node->attachObject(m_entity);
        m_scene_node->setVisible(true);
    }
    catch (Ogre::Exception &e)
    {
        RoR::LogFormat("[RoR] Error creating entity of mesh '%s' (group: '%s'), message: %s",
                       meshName.c_str(), entityRG.c_str(), e.getFullDescription().c_str());
        return;
    }
}
