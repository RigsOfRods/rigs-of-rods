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

#include "EnvironmentMap.h"

#include "Application.h"
#include "CameraManager.h"
#include "GameContext.h"
#include "GfxActor.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "SkyManager.h"
#include "Terrain.h"

#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreOverlay.h>

RoR::GfxEnvmap::GfxEnvmap():
    m_update_round(0)
{
    memset(m_cameras, 0, sizeof(m_cameras));
    memset(m_render_targets, 0, sizeof(m_render_targets));
}

void RoR::GfxEnvmap::SetupEnvMap()
{
    m_rtt_texture = Ogre::TextureManager::getSingleton().getByName("EnvironmentTexture");

    for (int face = 0; face < NUM_FACES; face++)
    {
        m_render_targets[face] = m_rtt_texture->getBuffer(face)->getRenderTarget();
        m_cameras[face] = App::GetGfxScene()->GetSceneManager()->createCamera("EnvironmentCamera-" + TOSTRING(face));
        m_cameras[face]->setAspectRatio(1.0);
        m_cameras[face]->setProjectionType(Ogre::PT_PERSPECTIVE);
        m_cameras[face]->setFixedYawAxis(false);
        m_cameras[face]->setFOVy(Ogre::Degree(90));
        m_cameras[face]->setNearClipDistance(0.1f);
        m_cameras[face]->setFarClipDistance(App::GetCameraManager()->GetCamera()->getFarClipDistance());

        Ogre::Viewport* v = m_render_targets[face]->addViewport(m_cameras[face]);
        v->setOverlaysEnabled(false);
        v->setClearEveryFrame(true);
        v->setBackgroundColour(App::GetCameraManager()->GetCamera()->getViewport()->getBackgroundColour());
        m_render_targets[face]->setAutoUpdated(false);
    }

    m_cameras[0]->setDirection(+Ogre::Vector3::UNIT_X);
    m_cameras[1]->setDirection(-Ogre::Vector3::UNIT_X);
    m_cameras[2]->setDirection(+Ogre::Vector3::UNIT_Y);
    m_cameras[3]->setDirection(-Ogre::Vector3::UNIT_Y);
    m_cameras[4]->setDirection(-Ogre::Vector3::UNIT_Z);
    m_cameras[5]->setDirection(+Ogre::Vector3::UNIT_Z);

    if (App::diag_envmap->getBool())
    {
        // create fancy mesh for debugging the envmap
        Ogre::Overlay* overlay = Ogre::OverlayManager::getSingleton().create("EnvMapDebugOverlay");
        if (overlay)
        {
            Ogre::Vector3 position = Ogre::Vector3::ZERO;
            float scale = 1.0f;

            Ogre::MeshPtr mesh = Ogre::MeshManager::getSingletonPtr()->createManual("cubeMapDebug", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            // create sub mesh
            Ogre::SubMesh* sub = mesh->createSubMesh();

            // Initialize render operation
            sub->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
            //
            sub->useSharedVertices = true;
            mesh->sharedVertexData = new Ogre::VertexData;
            sub->indexData = new Ogre::IndexData;

            // Create vertex declaration
            size_t offset = 0;
            mesh->sharedVertexData->vertexDeclaration->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
            offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
            mesh->sharedVertexData->vertexDeclaration->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_TEXTURE_COORDINATES);

            // Create and bind vertex buffer
            mesh->sharedVertexData->vertexCount = 14;
            Ogre::HardwareVertexBufferSharedPtr vertexBuffer =
                Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
                    mesh->sharedVertexData->vertexDeclaration->getVertexSize(0),
                    mesh->sharedVertexData->vertexCount,
                    Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
            mesh->sharedVertexData->vertexBufferBinding->setBinding(0, vertexBuffer);

            // Vertex data
            static const float vertexData[] = {
                // Position      Texture coordinates    // Index
                0.0, 2.0, -1.0, 1.0, 1.0, //  0
                0.0, 1.0, -1.0, -1.0, 1.0, //  1
                1.0, 2.0, -1.0, 1.0, -1.0, //  2
                1.0, 1.0, -1.0, -1.0, -1.0, //  3
                2.0, 2.0, 1.0, 1.0, -1.0, //  4
                2.0, 1.0, 1.0, -1.0, -1.0, //  5
                3.0, 2.0, 1.0, 1.0, 1.0, //  6
                3.0, 1.0, 1.0, -1.0, 1.0, //  7
                4.0, 2.0, -1.0, 1.0, 1.0, //  8
                4.0, 1.0, -1.0, -1.0, 1.0, //  9
                1.0, 3.0, -1.0, 1.0, 1.0, // 10
                2.0, 3.0, 1.0, 1.0, 1.0, // 11
                1.0, 0.0, -1.0, -1.0, 1.0, // 12
                2.0, 0.0, 1.0, -1.0, 1.0, // 13
            };

            // Fill vertex buffer
            float* pData = static_cast<float*>(vertexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
            for (size_t vertex = 0, i = 0; vertex < mesh->sharedVertexData->vertexCount; vertex++)
            {
                // Position
                *pData++ = position.x + scale * vertexData[i++];
                *pData++ = position.y + scale * vertexData[i++];
                *pData++ = 0.0;

                // Texture coordinates
                *pData++ = vertexData[i++];
                *pData++ = vertexData[i++];
                *pData++ = vertexData[i++];
            }
            vertexBuffer->unlock();

            // Create index buffer
            sub->indexData->indexCount = 36;
            Ogre::HardwareIndexBufferSharedPtr indexBuffer =
                Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(
                    Ogre::HardwareIndexBuffer::IT_16BIT,
                    sub->indexData->indexCount,
                    Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
            sub->indexData->indexBuffer = indexBuffer;

            // Index data
            static const Ogre::uint16 indexData[] = {
                // Indices         // Face
                 0,  1,  2,        //  0
                 2,  1,  3,        //  1
                 2,  3,  4,        //  2
                 4,  3,  5,        //  3
                 4,  5,  6,        //  4
                 6,  5,  7,        //  5
                 6,  7,  8,        //  6
                 8,  7,  9,        //  7
                10,  2, 11,        //  8
                11,  2,  4,        //  9
                 3, 12,  5,        // 10
                 5, 12, 13,        // 11
            };

            // Fill index buffer
            indexBuffer->writeData(0, indexBuffer->getSizeInBytes(), indexData, true);

            mesh->_setBounds(Ogre::AxisAlignedBox::BOX_INFINITE);
            mesh->_setBoundingSphereRadius(10);
            mesh->load();

            Ogre::Entity* e = App::GetGfxScene()->GetSceneManager()->createEntity(mesh->getName());
            e->setCastShadows(false);
            e->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY - 1);
            e->setVisible(true);

            e->setMaterialName("tracks/EnvMapDebug");
            Ogre::SceneNode* mDebugSceneNode = new Ogre::SceneNode(App::GetGfxScene()->GetSceneManager());
            mDebugSceneNode->attachObject(e);
            mDebugSceneNode->setPosition(Ogre::Vector3(0, 0, -5));
            mDebugSceneNode->setFixedYawAxis(true, Ogre::Vector3::UNIT_Y);
            mDebugSceneNode->setVisible(true);
            mDebugSceneNode->_update(true, true);
            mDebugSceneNode->_updateBounds();
            overlay->add3D(mDebugSceneNode);
            overlay->show();
        }
    }
}

void RoR::GfxEnvmap::ShutdownEnvMap()
{
    for (int face = 0; face < NUM_FACES; face++)
    {
        if (m_cameras[face] != nullptr)
        {
            App::GetGfxScene()->GetSceneManager()->destroyCamera(m_cameras[face]);
            m_render_targets[face]->removeAllViewports();
        }
    }
}

void RoR::GfxEnvmap::UpdateEnvMap(Ogre::Vector3 center, GfxActor* gfx_actor, bool full/*=false*/)
{
    // how many of the 6 render planes to update at once? Use cvar 'gfx_envmap_rate', unless instructed to do full render.
    const int update_rate = full ? NUM_FACES : App::gfx_envmap_rate->getInt();

    if (!App::gfx_envmap_enabled->getBool())
    {
        return;
    }

    if (App::GetGuiManager()->FlexbodyDebug.IsHideOtherElementsModeActive())
    {
        return; // Prevent showing meshes hidden by the diagnostic UI.
    }

    if (!full && update_rate == 0)
    {
        return;
    }

    for (int i = 0; i < NUM_FACES; i++)
    {
        m_cameras[i]->setPosition(center);
    }

    if (gfx_actor != nullptr)
    {
        gfx_actor->SetAllMeshesVisible(false);
        gfx_actor->SetRodsVisible(false);
    }

    for (int i = 0; i < update_rate; i++)
    {
#ifdef USE_CAELUM
        // caelum needs to know that we changed the cameras
        if (App::GetGameContext()->GetTerrain()->getSkyManager())
        {
            App::GetGameContext()->GetTerrain()->getSkyManager()->NotifySkyCameraChanged(m_cameras[m_update_round]);
        }
#endif // USE_CAELUM
        m_render_targets[m_update_round]->update();
        m_update_round = (m_update_round + 1) % NUM_FACES;
    }
#ifdef USE_CAELUM
    if (App::GetGameContext()->GetTerrain()->getSkyManager())
    {
        App::GetGameContext()->GetTerrain()->getSkyManager()->NotifySkyCameraChanged(App::GetCameraManager()->GetCamera());
    }
#endif // USE_CAELUM

    if (gfx_actor != nullptr)
    {
        gfx_actor->SetRodsVisible(true);
        gfx_actor->SetAllMeshesVisible(true);
    }
}
