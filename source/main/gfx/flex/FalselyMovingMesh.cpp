/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2026 Petr Ohlidal

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

#include "FalselyMovingMesh.h"

#include "Actor.h"
#include "Application.h"
#include "GfxScene.h"
#include "Terrain.h"

#include <OgreMeshLodGenerator.h>
#include <OgreLodConfig.h>

using namespace Ogre;
using namespace RoR;

FalselyMovingMesh::FalselyMovingMesh(Ogre::String meshName, Ogre::String entityRG, Ogre::String entityName, Ogre::SceneNode *_scene_node)
    : m_scene_node(_scene_node), m_entity(nullptr), m_cast_shadows(true)
{
    this->createEntity(meshName, entityRG, entityName);
}

FalselyMovingMesh::~FalselyMovingMesh()
{
    if (m_scene_node)
    {
        m_scene_node->removeAndDestroyAllChildren();
        App::GetGfxScene()->GetSceneManager()->destroySceneNode(m_scene_node);
    }
}

void FalselyMovingMesh::setMaterialName(Ogre::String m)
{
    if (m_entity)
    {
        m_entity->setMaterialName(m);
    }
}

void FalselyMovingMesh::setCastShadows(bool b)
{
    m_cast_shadows = b;
    if (m_scene_node && m_scene_node->numAttachedObjects())
    {
        m_scene_node->getAttachedObject(0)->setCastShadows(b);
    }
}

void FalselyMovingMesh::setVisible(bool b)
{
    // Workaround: if the scenenode is not used (entity not attached) for some reason, try hiding the entity directly.
    if (m_scene_node && m_scene_node->getAttachedObjects().size() > 0)
        m_scene_node->setVisible(b);
    else if (m_entity)
        m_entity->setVisible(b);
}

void FalselyMovingMesh::createEntity(Ogre::String meshName, Ogre::String entityRG, Ogre::String entityName)
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

        FalselyMovingMesh::SetupFalseMotionSkeletalAnim(m_mesh, entityRG);

        // now create an entity around the mesh and attach it to the scene graph
        m_entity = App::GetGfxScene()->GetSceneManager()->createEntity(entityName, meshName, entityRG);
        m_entity->setCastShadows(m_cast_shadows);

        m_scene_node->attachObject(m_entity);
        m_scene_node->setVisible(true);

        // finish false-movement setup
        m_skelinst = m_entity->getSkeleton();
        ROR_ASSERT(m_skelinst);
        m_boneinst = m_skelinst->getBone(0);
        ROR_ASSERT(m_boneinst);
        m_boneinst->setManuallyControlled(true);
    }
    catch (Ogre::Exception &e)
    {
        RoR::LogFormat("[RoR] Error creating entity of mesh '%s' (group: '%s'), message: %s",
                       meshName.c_str(), entityRG.c_str(), e.getFullDescription().c_str());
        return;
    }
}

// static
void FalselyMovingMesh::SetupFalseMotionSkeletalAnim(Ogre::MeshPtr mesh, const std::string& rg)
{
    /// This is the trick to prevent the prop from jittering at large world distances;
    /// we fix the scenenode at physics origin and move the verts relatively using a bone.
    /// -----------------------------------------------------------------------------------

    ROR_ASSERT(mesh);
    if (mesh->hasSkeleton())
    {
        return; // already done before
    }

    Ogre::SkeletonPtr skel = Ogre::SkeletonManager::getSingleton().create(
        mesh->getName() + "_prop-skeleton-gen", rg, /*isManual*/true);
    Ogre::Bone* bone = skel->createBone(0);

    mesh->setSkeletonName(skel->getName()); // doc says this must be done before adding bone assignments

    // bind all verts to the single bone with full weight (1.0)
    const unsigned short boneId = bone->getHandle();

    bool sharedVertsProcessed = false;
    for (Ogre::SubMesh* submesh: mesh->getSubMeshes())
    {
        if (submesh->useSharedVertices)
        {
            if (!sharedVertsProcessed)
            {
                for (size_t i = 0; i < mesh->sharedVertexData->vertexCount; i++)
                {
                    Ogre::VertexBoneAssignment vba;
                    vba.boneIndex = boneId;
                    vba.vertexIndex = static_cast<unsigned int>(i);
                    vba.weight = 1.f;
                    mesh->addBoneAssignment(vba); // <---  works on top of `sharedVertexData`
                }
                mesh->_compileBoneAssignments();
                sharedVertsProcessed = true;
            }
        }
        else
        {
            for (size_t i = 0; i < submesh->vertexData->vertexCount; i++)
            {
                Ogre::VertexBoneAssignment vba;
                vba.boneIndex = boneId;
                vba.vertexIndex = static_cast<unsigned int>(i);
                vba.weight = 1.f;
                submesh->addBoneAssignment(vba); // <---  works on top of submesh's `vertexData`
            }
            submesh->_compileBoneAssignments();
        }
    }
    
    bone->setBindingPose();

    skel->load();
    mesh->_notifySkeleton(skel); // <--- needed for dynamic setup

    // Set bounding information (for culling) - 100x100 is the size of actor-local physics space (relative to `ar_origin`)
    mesh->_setBounds(AxisAlignedBox(-100,-100,-100,100,100,100), true);
}

// --- Special functions to set transforms ---

void FalselyMovingMesh::SetOriginWorldPosition(const Ogre::Vector3& pos)
{
    m_scene_node->setPosition(pos);
}

void FalselyMovingMesh::SetBoneRelPosition(const Ogre::Vector3& pos)
{
    m_boneinst->setPosition(pos);
    m_skelinst->_notifyManualBonesDirty();
}

void FalselyMovingMesh::SetBoneRelOrientation(const Ogre::Quaternion& rot)
{
    m_boneinst->setOrientation(rot);
    m_skelinst->_notifyManualBonesDirty();
}
