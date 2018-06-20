/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Vergu�n Gonz�lez <xavierverguin@hotmail.com>
                                           <xavyiy@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

#include "Mesh.h"

#include "Hydrax.h"
#include <OgreMatrix4.h>

namespace Hydrax
{
	Mesh::Mesh(Hydrax *h)
            : mHydrax(h)
			, mCreated(false)
            , mMesh()
            , mSubMesh(0)
            , mEntity(0)
            , mNumFaces(0)
            , mNumVertices(0)
            , mVertexBuffer()
            , mIndexBuffer()
			, mSceneNode(0)
            , mMaterialName("_NULL_")
    {
    }

    Mesh::~Mesh()
    {
		remove();
    }

	void Mesh::remove()
	{
		if (!mCreated)
		{
			return;
		}

		mSceneNode->detachAllObjects();
		mSceneNode->getParentSceneNode()->removeAndDestroyChild(mSceneNode->getName());
		mSceneNode = 0;

		Ogre::MeshManager::getSingleton().remove("HydraxMesh");
		mHydrax->getSceneManager()->destroyEntity(mEntity);

		mMesh.setNull();
		mSubMesh = 0;
		mEntity = 0;
		mNumFaces = 0;
		mNumVertices = 0;
		mVertexBuffer.setNull();
		mIndexBuffer.setNull();
		mMaterialName = "_NULL_";

		mCreated = false;
	}

    void Mesh::setOptions(const Options& Options)
    {
		if (mCreated)
		{
			Ogre::AxisAlignedBox meshBounds;

			if (Options.MeshSize.Width == 0 && Options.MeshSize.Height == 0)
			{
				meshBounds = Ogre::AxisAlignedBox(-1000000, -Options.MeshStrength/2,-1000000,
		                                       	   1000000,  Options.MeshStrength/2, 1000000);
			}
			else
			{
				meshBounds = Ogre::AxisAlignedBox(0,                     -Options.MeshStrength/2, 0,
			                                      Options.MeshSize.Width, Options.MeshStrength/2, Options.MeshSize.Height);
			}

        	mMesh->_setBounds(meshBounds);
			mSceneNode->_updateBounds();

			if (mOptions.MeshSize.Width != Options.MeshSize.Width || mOptions.MeshSize.Height != Options.MeshSize.Height)
			{
			    mSceneNode->setPosition(mHydrax->getPosition().x-Options.MeshSize.Width/2,mHydrax->getPosition().y,mHydrax->getPosition().z-Options.MeshSize.Height/2);
			}
		}

		mOptions = Options;
    }

    void Mesh::setMaterialName(const Ogre::String &MaterialName)
    {
        mMaterialName = MaterialName;

        if (mEntity)
        {
            mEntity->setMaterialName(mMaterialName);
        }
    }

	void Mesh::create()
	{
		if (mCreated)
		{
			return;
		}

		// Create mesh and submesh
        mMesh = Ogre::MeshManager::getSingleton().createManual("HydraxMesh",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mSubMesh = mMesh->createSubMesh();
        mSubMesh->useSharedVertices = false;

		if (mHydrax->getModule())
		{
			if (!mHydrax->getModule()->_createGeometry(this))
			{
				_createGeometry();
			}
		}

		// End mesh creation
        Ogre::AxisAlignedBox meshBounds;

		if (mOptions.MeshSize.Width == 0 && mOptions.MeshSize.Height == 0)
		{
			meshBounds = Ogre::AxisAlignedBox(-1000000, -mOptions.MeshStrength/2,-1000000,
		                                       1000000,  mOptions.MeshStrength/2, 1000000);
		}
		else
		{
			meshBounds = Ogre::AxisAlignedBox(0,                      -mOptions.MeshStrength/2, 0,
			                                  mOptions.MeshSize.Width, mOptions.MeshStrength/2, mOptions.MeshSize.Height);
		}

        mMesh->_setBounds(meshBounds);
        mMesh->load();
        mMesh->touch();

        mEntity = mHydrax->getSceneManager()->createEntity("HydraxMeshEnt", "HydraxMesh");
        mEntity->setMaterialName(mMaterialName);
		mEntity->setCastShadows(false);
		mEntity->setRenderQueueGroup(Ogre::RENDER_QUEUE_1);

		mSceneNode = mHydrax->getSceneManager()->getRootSceneNode()->createChildSceneNode();
		mSceneNode->showBoundingBox(false);
        mSceneNode->attachObject(mEntity);
        mSceneNode->setPosition(mHydrax->getPosition().x-mOptions.MeshSize.Width/2,mHydrax->getPosition().y,mHydrax->getPosition().z-mOptions.MeshSize.Height/2);

		mCreated = true;
	}

	void Mesh::_createGeometry()
	{
		int& Complexity = mOptions.MeshComplexity;

		int numVertices = Complexity*Complexity;
		int numEle = 6 * (Complexity-1)*(Complexity-1);

		// Vertex buffers
		mSubMesh->vertexData = new Ogre::VertexData();
		mSubMesh->vertexData->vertexStart = 0;
		mSubMesh->vertexData->vertexCount = numVertices;

		Ogre::VertexDeclaration* vdecl = mSubMesh->vertexData->vertexDeclaration;
		Ogre::VertexBufferBinding* vbind = mSubMesh->vertexData->vertexBufferBinding;

		size_t offset = 0;

		switch (mOptions.MeshVertexType)
		{
		    case VT_POS_NORM_UV:
			{
				vdecl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		        vdecl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
		        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		        vdecl->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);

				mVertexBuffer = Ogre::HardwareBufferManager::getSingleton().
			        createVertexBuffer(sizeof(POS_NORM_UV_VERTEX),
			                           numVertices,
			                           Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
			}
			break;

			case VT_POS_NORM:
			{
				vdecl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		        vdecl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);

				mVertexBuffer = Ogre::HardwareBufferManager::getSingleton().
			        createVertexBuffer(sizeof(POS_NORM_VERTEX),
			                           numVertices,
			                           Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
			}
			break;

			case VT_POS_UV:
			{
				vdecl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		        vdecl->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);

				mVertexBuffer = Ogre::HardwareBufferManager::getSingleton().
			        createVertexBuffer(sizeof(POS_UV_VERTEX),
			                           numVertices,
			                           Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
			}
			break;

			case VT_POS:
			{
				vdecl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);

				mVertexBuffer = Ogre::HardwareBufferManager::getSingleton().
			        createVertexBuffer(sizeof(POS_VERTEX),
			                           numVertices,
			                           Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
			}
			break;
		}

		vbind->setBinding(0, mVertexBuffer);

		unsigned int *indexbuffer = new unsigned int[numEle];

		int i = 0;
		for(int v=0; v<Complexity-1; v++){
			for(int u=0; u<Complexity-1; u++){
				// face 1 |/
				indexbuffer[i++] = v*Complexity + u;
				indexbuffer[i++] = v*Complexity + u + 1;
				indexbuffer[i++] = (v+1)*Complexity + u;

				// face 2 /|
				indexbuffer[i++] = (v+1)*Complexity + u;
				indexbuffer[i++] = v*Complexity + u + 1;
				indexbuffer[i++] = (v+1)*Complexity + u + 1;
			}
		}

		// Prepare buffer for indices
		mIndexBuffer =
			Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(
			Ogre::HardwareIndexBuffer::IT_32BIT,
			numEle,
			Ogre::HardwareBuffer::HBU_STATIC, true);

		mIndexBuffer->
			writeData(0,
			          mIndexBuffer->getSizeInBytes(),
			          indexbuffer,
			          true);

		delete []indexbuffer;

		// Set index buffer for this submesh
		mSubMesh->indexData->indexBuffer = mIndexBuffer;
		mSubMesh->indexData->indexStart = 0;
		mSubMesh->indexData->indexCount = numEle;
	}

	bool Mesh::updateGeometry(const int &numVer, void* verArray)
	{
		if (numVer != mVertexBuffer->getNumVertices() || !mCreated)
		{
			return false;
		}

		if (verArray)
		{
		    mVertexBuffer->
				writeData(0,
		                  mVertexBuffer->getSizeInBytes(),
	                      verArray,
		                  true);
		}

		return true;
	}

	bool Mesh::isPointInGrid(const Ogre::Vector2 &Position)
	{
		Ogre::AxisAlignedBox WordMeshBox = mEntity->getWorldBoundingBox();

		// Get our mesh grid rectangle:
		// c-----------d
		// |           |
		// |           |
		// |           |
		// a-----------b
		Ogre::Vector3
			a = WordMeshBox.getCorner(Ogre::AxisAlignedBox::FAR_LEFT_BOTTOM),
			b = WordMeshBox.getCorner(Ogre::AxisAlignedBox::FAR_RIGHT_BOTTOM),
			c = WordMeshBox.getCorner(Ogre::AxisAlignedBox::NEAR_RIGHT_BOTTOM),
			d = WordMeshBox.getCorner(Ogre::AxisAlignedBox::NEAR_LEFT_BOTTOM);

		// Transform all corners to Ogre::Vector2 array
		Ogre::Vector2 Corners2D[4] =
		   {Ogre::Vector2(a.x, a.z),
		    Ogre::Vector2(b.x, b.z),
		    Ogre::Vector2(c.x, c.z),
		    Ogre::Vector2(d.x, d.z)};

		// Determinate if Position is into our rectangle, we use a line intersection detection
		// because our mesh rectangle can be rotated, if the number of collisions with the four
		// segments AB, BC, CD, DA is one, the Position point is into the rectangle, else(if number
		// of collisions are 0 or 2, the Position point is outside the rectangle.
		int NumberOfCollisions = 0;
		// Find a point wich isn't be inside the rectangle
		Ogre::Vector2 DestPoint = Corners2D[0] + (Corners2D[1]-Corners2D[0])*2;
		for (int k = 0; k < 3; k++)
		{
			if (Math::intersectionOfTwoLines(Corners2D[k], Corners2D[k+1], Position, DestPoint) != Ogre::Vector2::ZERO)
			{
				NumberOfCollisions ++;
			}

			if (k == 2)
			{
				if (Math::intersectionOfTwoLines(Corners2D[3], Corners2D[0], Position, DestPoint) != Ogre::Vector2::ZERO)
			    {
				    NumberOfCollisions ++;
			    }
			}
		}
		if (NumberOfCollisions == 1)
		{
			return true;
		}

		return false;
	}

	Ogre::Vector2 Mesh::getGridPosition(const Ogre::Vector2 &Position)
	{
		if (mOptions.MeshSize.Width == 0 && mOptions.MeshSize.Height == 0)
		{
			return Position;
		}

		if (!isPointInGrid(Position))
		{
			return Ogre::Vector2(-1,-1);
		}

		Ogre::AxisAlignedBox WordMeshBox = mEntity->getWorldBoundingBox();

		// Get our mesh grid rectangle: (Only a,b,c corners)
		// c
		// |
		// |
		// |
		// a-----------b
		Ogre::Vector3
			a = WordMeshBox.getCorner(Ogre::AxisAlignedBox::FAR_LEFT_BOTTOM),
			b = WordMeshBox.getCorner(Ogre::AxisAlignedBox::FAR_RIGHT_BOTTOM),
			c = WordMeshBox.getCorner(Ogre::AxisAlignedBox::NEAR_LEFT_BOTTOM);

		// Transform all corners to Ogre::Vector2 array
		Ogre::Vector2 Corners2D[3] =
		   {Ogre::Vector2(a.x, a.z),
		    Ogre::Vector2(b.x, b.z),
		    Ogre::Vector2(c.x, c.z)};

		// Get segments AB and AC
		Ogre::Vector2 AB = Corners2D[1]-Corners2D[0],
			          AC = Corners2D[2]-Corners2D[0];

		// Find the X/Y position projecting the Position point to AB and AC segments.
		Ogre::Vector2 XProjectedPoint = Position-AC,
			          YProjectedPoint = Position-AB;

		// Fint the intersections points
		Ogre::Vector2 XPoint = Math::intersectionOfTwoLines(Corners2D[0],Corners2D[1],Position,XProjectedPoint),
			          YPoint = Math::intersectionOfTwoLines(Corners2D[0],Corners2D[2],Position,YProjectedPoint);

		// Find lengths
		Ogre::Real ABLength = AB.length(),
			       ACLength = AC.length(),
				   XLength  = (XPoint-Corners2D[0]).length(),
				   YLength  = (YPoint-Corners2D[0]).length();

		// Find final x/y grid positions in [0,1] range
		Ogre::Real XFinal = XLength / ABLength,
			       YFinal = YLength / ACLength;

		return Ogre::Vector2(XFinal,YFinal);
	}

	const Ogre::Vector3 Mesh::getObjectSpacePosition(const Ogre::Vector3& WorldSpacePosition) const
	{
		Ogre::Affine3 mWorldMatrix;

		if (mCreated)
		{
            #if OGRE_VERSION_MAJOR >= 1 && OGRE_VERSION_MINOR >= 7
                mWorldMatrix = mEntity->getParentSceneNode()->_getFullTransform();
            #else
                mEntity->getParentSceneNode()->getWorldTransforms(&mWorldMatrix);
            #endif
        }
		else
		{
			Ogre::SceneNode *mTmpSN = new Ogre::SceneNode(0);
		    mTmpSN->setPosition(mHydrax->getPosition());

            #if OGRE_VERSION_MAJOR >= 1 && OGRE_VERSION_MINOR >= 7
                mWorldMatrix = mTmpSN->_getFullTransform();
            #else
                mTmpSN->getWorldTransforms(&mWorldMatrix);
            #endif

		    delete mTmpSN;
		}

		return mWorldMatrix.inverse()*(WorldSpacePosition);
	}

	const Ogre::Vector3 Mesh::getWorldSpacePosition(const Ogre::Vector3& ObjectSpacePosition) const
	{
		Ogre::Affine3 mWorldMatrix;

		if (mCreated)
		{
            #if OGRE_VERSION_MAJOR >= 1 && OGRE_VERSION_MINOR >= 7
                mWorldMatrix = mEntity->getParentSceneNode()->_getFullTransform();
            #else
                mEntity->getParentSceneNode()->getWorldTransforms(&mWorldMatrix);
            #endif
        }
		else
		{
			Ogre::SceneNode *mTmpSN = new Ogre::SceneNode(0);
		    mTmpSN->setPosition(mHydrax->getPosition());

            #if OGRE_VERSION_MAJOR >= 1 && OGRE_VERSION_MINOR >= 7
                mWorldMatrix = mTmpSN->_getFullTransform();
            #else
                mTmpSN->getWorldTransforms(&mWorldMatrix);
            #endif

		    delete mTmpSN;
		}

		//return mWorldMatrix.transformAffine(ObjectSpacePosition);
		return mWorldMatrix*(ObjectSpacePosition);
	}
}
