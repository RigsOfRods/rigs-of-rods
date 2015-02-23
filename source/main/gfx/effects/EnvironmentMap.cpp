/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "EnvironmentMap.h"

#include "Beam.h"
#include "Ogre.h"
#include "Settings.h"
#include "CaelumManager.h"
#include "TerrainManager.h"

using namespace Ogre;

Envmap::Envmap() :
	  mInitiated(false)
	, mIsDynamic(false)
	, mRound(0)
	, updateRate(1)
{
	mIsDynamic = BSETTING("Envmap", false);
	updateRate = ISETTING("EnvmapUpdateRate", 1);

	TexturePtr texture = TextureManager::getSingleton().createManual("EnvironmentTexture",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_CUBE_MAP, 256, 256, 0,
		PF_R8G8B8, TU_RENDERTARGET);

	for (int face=0; face < NUM_FACES; face++)
	{
		mRenderTargets[face] = texture->getBuffer(face)->getRenderTarget();
		mCameras[face] = gEnv->sceneManager->createCamera("EnvironmentCamera-" + TOSTRING(face));
		mCameras[face]->setAspectRatio(1.0);
		mCameras[face]->setProjectionType(PT_PERSPECTIVE);
		mCameras[face]->setFixedYawAxis(false);
		mCameras[face]->setFOVy(Degree(90));
		mCameras[face]->setNearClipDistance(0.1f);
		mCameras[face]->setFarClipDistance(gEnv->mainCamera->getFarClipDistance());

		Viewport *v = mRenderTargets[face]->addViewport(mCameras[face]);
		v->setOverlaysEnabled(false);
		v->setClearEveryFrame(true);
		v->setBackgroundColour(gEnv->mainCamera->getViewport()->getBackgroundColour());
		mRenderTargets[face]->setAutoUpdated(false);

		switch (face)
		{
		case 0:
			mCameras[face]->setDirection(+Vector3::UNIT_X);
			break;
		case 1:
			mCameras[face]->setDirection(-Vector3::UNIT_X);
			break;
		case 2:
			mCameras[face]->setDirection(+Vector3::UNIT_Y);
			break;
		case 3:
			mCameras[face]->setDirection(-Vector3::UNIT_Y);
			break;
		case 4:
			mCameras[face]->setDirection(-Vector3::UNIT_Z);
			break;
		case 5:
			mCameras[face]->setDirection(+Vector3::UNIT_Z);
			break;
		}
	}

	updateRate = std::max(0, updateRate);
	updateRate = std::min(updateRate, 6);

	if (BSETTING("EnvMapDebug", false))
	{
		// create fancy mesh for debugging the envmap
		Overlay *overlay = OverlayManager::getSingleton().create("EnvMapDebugOverlay");
		if (overlay)
		{
			Vector3 position = Vector3::ZERO;
			float scale = 1.0f;
			
			MeshPtr mesh = MeshManager::getSingletonPtr()->createManual("cubeMapDebug", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			// create sub mesh
			SubMesh *sub = mesh->createSubMesh();

			// Initialize render operation
			sub->operationType = RenderOperation::OT_TRIANGLE_LIST;
			//
			sub->useSharedVertices = true;
			mesh->sharedVertexData = new VertexData;
			sub->indexData = new IndexData;

			// Create vertex declaration
			size_t offset = 0;
			VertexDeclaration* vertexDeclaration = mesh->sharedVertexData->vertexDeclaration;
			vertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_POSITION);
			offset += VertexElement::getTypeSize(VET_FLOAT3);
			vertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_TEXTURE_COORDINATES);
			offset += VertexElement::getTypeSize(VET_FLOAT3);

			// Create and bind vertex buffer
			mesh->sharedVertexData->vertexCount = 14;
			HardwareVertexBufferSharedPtr vertexBuffer =
			  HardwareBufferManager::getSingleton().createVertexBuffer(
				 mesh->sharedVertexData->vertexDeclaration->getVertexSize(0),
				 mesh->sharedVertexData->vertexCount,
				 HardwareBuffer::HBU_STATIC_WRITE_ONLY);
			mesh->sharedVertexData->vertexBufferBinding->setBinding(0, vertexBuffer);

			// Vertex data
			static const float vertexData[] = {
			// Position      Texture coordinates    // Index
			  0.0, 2.0,      -1.0,  1.0,  1.0,      //  0
			  0.0, 1.0,      -1.0, -1.0,  1.0,      //  1
			  1.0, 2.0,      -1.0,  1.0, -1.0,      //  2
			  1.0, 1.0,      -1.0, -1.0, -1.0,      //  3
			  2.0, 2.0,       1.0,  1.0, -1.0,      //  4
			  2.0, 1.0,       1.0, -1.0, -1.0,      //  5
			  3.0, 2.0,       1.0,  1.0,  1.0,      //  6
			  3.0, 1.0,       1.0, -1.0,  1.0,      //  7
			  4.0, 2.0,      -1.0,  1.0,  1.0,      //  8
			  4.0, 1.0,      -1.0, -1.0,  1.0,      //  9
			  1.0, 3.0,      -1.0,  1.0,  1.0,      // 10
			  2.0, 3.0,       1.0,  1.0,  1.0,      // 11
			  1.0, 0.0,      -1.0, -1.0,  1.0,      // 12
			  2.0, 0.0,       1.0, -1.0,  1.0,      // 13
			};

			// Fill vertex buffer
			float *pData = static_cast<float*>(vertexBuffer->lock(HardwareBuffer::HBL_DISCARD));
			for (size_t vertex=0, i=0; vertex < mesh->sharedVertexData->vertexCount; vertex++)
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
			HardwareIndexBufferSharedPtr indexBuffer =
			  HardwareBufferManager::getSingleton().createIndexBuffer(
				 HardwareIndexBuffer::IT_16BIT,
				 sub->indexData->indexCount,
				 HardwareBuffer::HBU_STATIC_WRITE_ONLY);
			sub->indexData->indexBuffer = indexBuffer;

			// Index data
			static const uint16 indexData[] = {
			// Indices         // Face
			   0,  1,  2,      //  0
			   2,  1,  3,      //  1
			   2,  3,  4,      //  2
			   4,  3,  5,      //  3
			   4,  5,  6,      //  4
			   6,  5,  7,      //  5
			   6,  7,  8,      //  6
			   8,  7,  9,      //  7
			  10,  2, 11,      //  8
			  11,  2,  4,      //  9
			   3, 12,  5,      // 10
			   5, 12, 13,      // 11
			};

			// Fill index buffer
			indexBuffer->writeData(0, indexBuffer->getSizeInBytes(), indexData, true);
			indexBuffer->unlock();

			mesh->_setBounds(AxisAlignedBox::BOX_INFINITE);
			mesh->_setBoundingSphereRadius(10);
			mesh->load();

			Entity *e = gEnv->sceneManager->createEntity(mesh->getName());
			e->setCastShadows(false);
			e->setRenderQueueGroup(RENDER_QUEUE_OVERLAY-1);
			e->setVisible(true);

			e->setMaterialName("tracks/EnvMapDebug");
			Ogre::SceneNode* mDebugSceneNode = new SceneNode(gEnv->sceneManager);
			mDebugSceneNode->attachObject(e);
			mDebugSceneNode->setPosition(Vector3(0, 0, -5));
			mDebugSceneNode->setFixedYawAxis(true, Vector3::UNIT_Y);
			mDebugSceneNode->setVisible(true);
			mDebugSceneNode->_update(true, true);
			mDebugSceneNode->_updateBounds();
			overlay->add3D(mDebugSceneNode);
			overlay->show();
			
			// example update
			init(Vector3::ZERO);
		}
	}
}

Envmap::~Envmap()
{
	for (int face = 0; face < NUM_FACES; face++)
	{
		gEnv->sceneManager->destroyCamera("EnvironmentCamera-" + TOSTRING(face));
	}
}

void Envmap::update(Ogre::Vector3 center, Beam *beam /* = 0 */)
{
	if (!mIsDynamic || !beam)
	{
		if (!mInitiated)
		{
			init(center);
		}
		return;
	}

	for (int i=0; i < NUM_FACES; i++)
	{
		mCameras[i]->setPosition(center);
	}

	// try to hide all flexbodies and cabs prior render, and then show them again after done
	// but only if they are visible ...
	bool toggleMeshes = beam && beam->meshesVisible;

	// same for all beams
	bool toggleBeams = beam && beam->beamsVisible;

	if (toggleMeshes)
	{
		beam->setMeshVisibility(false);
	}
	if (toggleBeams)
	{
		beam->setBeamVisibility(false);
	}

	for (int i=0; i < updateRate; i++)
	{
		// caelum needs to know that we changed the cameras
	#ifdef USE_CAELUM
		
		if (gEnv->terrainManager->getCaelumManager())
		{
			gEnv->terrainManager->getCaelumManager()->notifyCameraChanged(mCameras[mRound]);
		}
	#endif // USE_CAELUM
		mRenderTargets[mRound]->update();
		mRound = (mRound + 1) % NUM_FACES;
	}
#ifdef USE_CAELUM
	
	if (gEnv->terrainManager->getCaelumManager())
	{
		gEnv->terrainManager->getCaelumManager()->notifyCameraChanged(gEnv->mainCamera);
	}
#endif // USE_CAELUM

	if (toggleMeshes)
	{
		beam->setMeshVisibility(true);
	}
	if (toggleBeams)
	{
		beam->setBeamVisibility(true);
	}
}

void Envmap::init(Vector3 center)
{
	if (mIsDynamic) return;

	// capture all images at once
	for (int i=0; i < NUM_FACES; i++)
	{
		mCameras[i]->setPosition(center);
		mRenderTargets[i]->update();
	}

	mInitiated = true;
}
