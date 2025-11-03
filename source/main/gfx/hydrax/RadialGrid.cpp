/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Verguín González <xavierverguin@hotmail.com>
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

#include <RadialGrid.h>

namespace Hydrax{namespace Module
{
	Mesh::VertexType _RG_getVertexTypeFromNormalMode(const MaterialManager::NormalMode& NormalMode)
	{
		if (NormalMode == MaterialManager::NM_VERTEX)
		{
			return Mesh::VT_POS_NORM;
		}

		// NM_RTT
		return Mesh::VT_POS;
	}

	Ogre::String _RG_getNormalModeString(const MaterialManager::NormalMode& NormalMode)
	{
		if (NormalMode == MaterialManager::NM_VERTEX)
		{
			return "Vertex";
		}

		return "Rtt";
	}

	RadialGrid::RadialGrid(Hydrax *h, Noise::Noise *n, const MaterialManager::NormalMode& NormalMode)
		: Module("RadialGrid" + _RG_getNormalModeString(NormalMode),
		         n, Mesh::Options(0, Size(200), _RG_getVertexTypeFromNormalMode(NormalMode)), NormalMode)
		, mHydrax(h)
		, mVertices(0)
		, mVerticesChoppyBuffer(0)
	{
	}

	RadialGrid::RadialGrid(Hydrax *h, Noise::Noise *n, const MaterialManager::NormalMode& NormalMode, const Options &Options)
		: Module("RadialGrid" + _RG_getNormalModeString(NormalMode),
		         n, Mesh::Options(0, Size(Options.Radius*2), _RG_getVertexTypeFromNormalMode(NormalMode)), NormalMode)
		, mHydrax(h)
		, mVertices(0)
		, mVerticesChoppyBuffer(0)
	{
		setOptions(Options);
	}

	RadialGrid::~RadialGrid()
	{
		remove();

		HydraxLOG(getName() + " destroyed.");
	}

	void RadialGrid::setOptions(const Options &Options)
	{
		mMeshOptions.MeshSize     = Size(Options.Radius*2);
		mMeshOptions.MeshStrength = Options.Strength;

		mHydrax->getMesh()->setOptions(mMeshOptions);
		mHydrax->_setStrength(mOptions.Strength);

		if (isCreated())
		{
			if ((Options.Steps != mOptions.Steps) || (Options.Circles != mOptions.Circles))
			{
				remove();
				mOptions = Options;
				create();

				if (mNormalMode == MaterialManager::NM_RTT)
				{
					if (!mNoise->createGPUNormalMapResources(mHydrax->getGPUNormalMapManager()))
					{
						HydraxLOG(mNoise->getName() + " doesn't support GPU Normal map generation.");
					}
				}

				Ogre::String MaterialNameTmp = mHydrax->getMesh()->getMaterialName();
				mHydrax->getMesh()->remove();
				mHydrax->getMesh()->setOptions(getMeshOptions());
				mHydrax->getMesh()->setMaterialName(MaterialNameTmp);
				mHydrax->getMesh()->create();

				return;
			}

			mOptions = Options;

			int x, y;
			if (getNormalMode() == MaterialManager::NM_VERTEX)
			{
				Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

				Vertices[0].x = mOptions.Radius;
			    Vertices[0].z = mOptions.Radius;

				float r_scale = mOptions.Radius/
					(mOptions.StepSizeLin  * mOptions.Circles +
					 mOptions.StepSizeCube * Ogre::Math::Pow(mOptions.Circles, 3) +
				     mOptions.StepSizeFive * Ogre::Math::Pow(mOptions.Circles, 5));

				for(y=0;y<mOptions.Circles;y++)
				{
					float r = r_scale*(mOptions.StepSizeLin * (y+1) + mOptions.StepSizeCube * Ogre::Math::Pow(y+1, 3) + mOptions.StepSizeFive * Ogre::Math::Pow(y+1, 5));

					for(x=0;x<mOptions.Steps;x++)
					{
						Vertices[1+y*mOptions.Steps + x].x = mOptions.Radius + r * Ogre::Math::Cos(Ogre::Math::TWO_PI * x / mOptions.Steps);
						Vertices[1+y*mOptions.Steps + x].z = mOptions.Radius + r * Ogre::Math::Sin(Ogre::Math::TWO_PI * x / mOptions.Steps);
					}
				}

				if (mOptions.ChoppyWaves && mVerticesChoppyBuffer)
			    {
				    for(int i = 0; i < 1+mOptions.Circles*mOptions.Steps; i++)
				    {
					    mVerticesChoppyBuffer[i] = Vertices[i];
				    }
			    }
			}
			else if (getNormalMode() == MaterialManager::NM_RTT)
			{
				Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

				Vertices[0].x = mOptions.Radius;
			    Vertices[0].z = mOptions.Radius;

                float r_scale = mOptions.Radius/
					(mOptions.StepSizeLin  * mOptions.Circles +
					 mOptions.StepSizeCube * Ogre::Math::Pow(mOptions.Circles, 3) +
				     mOptions.StepSizeFive * Ogre::Math::Pow(mOptions.Circles, 5));

				for(y=0;y<mOptions.Circles;y++)
				{
					float r = r_scale*(mOptions.StepSizeLin * (y+1) + mOptions.StepSizeCube * Ogre::Math::Pow(y+1, 3) + mOptions.StepSizeFive * Ogre::Math::Pow(y+1, 5));

					for(x=0;x<mOptions.Steps;x++)
					{
						Vertices[1+y*mOptions.Steps + x].x = mOptions.Radius + r * Ogre::Math::Cos(Ogre::Math::TWO_PI * x / mOptions.Steps);
						Vertices[1+y*mOptions.Steps + x].z = mOptions.Radius + r * Ogre::Math::Sin(Ogre::Math::TWO_PI * x / mOptions.Steps);
					}
				}
			}

			return;
		}

		mOptions = Options;
	}

	void RadialGrid::create()
	{
		HydraxLOG("Creating " + getName() + " module.");

		Module::create();

		int x, y;
		if (getNormalMode() == MaterialManager::NM_VERTEX)
		{
			mVertices = new Mesh::POS_NORM_VERTEX[1+mOptions.Steps * mOptions.Circles];
			Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

			Vertices[0].x = mOptions.Radius;
			Vertices[0].y = 0;
			Vertices[0].z = mOptions.Radius;

			Vertices[0].nx = 0;
			Vertices[0].ny = -1;
			Vertices[0].nz = 0;

            float r_scale = mOptions.Radius/
				           (mOptions.StepSizeLin  * mOptions.Circles +
				            mOptions.StepSizeCube * Ogre::Math::Pow(mOptions.Circles, 3) +
				            mOptions.StepSizeFive * Ogre::Math::Pow(mOptions.Circles, 5));

			for(y=0;y<mOptions.Circles;y++)
			{
				float r = r_scale*(mOptions.StepSizeLin * (y+1) + mOptions.StepSizeCube * Ogre::Math::Pow(y+1, 3) + mOptions.StepSizeFive * Ogre::Math::Pow(y+1, 5));

				for(x=0;x<mOptions.Steps;x++)
				{
					Vertices[1+y*mOptions.Steps + x].x = mOptions.Radius + r * Ogre::Math::Cos(Ogre::Math::TWO_PI * x / mOptions.Steps);
					Vertices[1+y*mOptions.Steps + x].y = 0;
					Vertices[1+y*mOptions.Steps + x].z = mOptions.Radius + r * Ogre::Math::Sin(Ogre::Math::TWO_PI * x / mOptions.Steps);

					Vertices[1+y*mOptions.Steps + x].nx = 0;
					Vertices[1+y*mOptions.Steps + x].ny = -1;
					Vertices[1+y*mOptions.Steps + x].nz = 0;
				}
			}

			if (mOptions.ChoppyWaves)
			{
				mVerticesChoppyBuffer = new Mesh::POS_NORM_VERTEX[1+mOptions.Circles*mOptions.Steps];

				for(int i = 0; i < 1+mOptions.Circles*mOptions.Steps; i++)
				{
					mVerticesChoppyBuffer[i] = Vertices[i];
				}
			}
		}
		else if (getNormalMode() == MaterialManager::NM_RTT)
		{
			mVertices = new Mesh::POS_VERTEX[1+mOptions.Steps * mOptions.Circles];
			Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

			Vertices[0].x = mOptions.Radius;
			Vertices[0].y = 0;
			Vertices[0].z = mOptions.Radius;

            float r_scale = mOptions.Radius/
				           (mOptions.StepSizeLin  * mOptions.Circles +
				            mOptions.StepSizeCube * Ogre::Math::Pow(mOptions.Circles, 3) +
				            mOptions.StepSizeFive * Ogre::Math::Pow(mOptions.Circles, 5));

			for(y=0;y<mOptions.Circles;y++)
			{
				float r = r_scale*(mOptions.StepSizeLin * (y+1) + mOptions.StepSizeCube * Ogre::Math::Pow(y+1, 3) + mOptions.StepSizeFive * Ogre::Math::Pow(y+1, 5));

				for(x=0;x<mOptions.Steps;x++)
				{
					Vertices[1+y*mOptions.Steps + x].x = mOptions.Radius + r * Ogre::Math::Cos(Ogre::Math::TWO_PI * x / mOptions.Steps);
					Vertices[1+y*mOptions.Steps + x].y = 0;
					Vertices[1+y*mOptions.Steps + x].z = mOptions.Radius + r * Ogre::Math::Sin(Ogre::Math::TWO_PI * x / mOptions.Steps);
				}
			}
		}

		HydraxLOG(getName() + " created.");
	}

	const bool RadialGrid::_createGeometry(Mesh *mMesh) const
	{
		int numVertices = mOptions.Steps * mOptions.Circles + 1;
		int numEle = 6 * mOptions.Steps * (mOptions.Circles-1) + 3 * mOptions.Steps;

		// Vertex buffers
		mMesh->getSubMesh()->vertexData = new Ogre::VertexData();
		mMesh->getSubMesh()->vertexData->vertexStart = 0;
		mMesh->getSubMesh()->vertexData->vertexCount = numVertices;

		Ogre::VertexDeclaration* vdecl = mMesh->getSubMesh()->vertexData->vertexDeclaration;
		Ogre::VertexBufferBinding* vbind = mMesh->getSubMesh()->vertexData->vertexBufferBinding;

		size_t offset = 0;

		switch (mMeshOptions.MeshVertexType)
		{
		    case Mesh::VT_POS_NORM:
			{
				vdecl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		        vdecl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);

				mMesh->getHardwareVertexBuffer() = Ogre::HardwareBufferManager::getSingleton().
					createVertexBuffer(sizeof(Mesh::POS_NORM_VERTEX),
			                           numVertices,
			                           Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
			}
			break;

			case Mesh::VT_POS:
			{
				vdecl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);

				mMesh->getHardwareVertexBuffer() = Ogre::HardwareBufferManager::getSingleton().
					createVertexBuffer(sizeof(Mesh::POS_VERTEX),
			                           numVertices,
			                           Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
			}
			break;
		}

		vbind->setBinding(0, mMesh->getHardwareVertexBuffer());

		unsigned int *indexbuffer = new unsigned int[numEle];

		for (int k = 0; k < mOptions.Steps; k++)
		{
			indexbuffer[k*3+2]   = 0;
			indexbuffer[k*3+1] = k+1;

			if (k != mOptions.Steps-1)
			{
			    indexbuffer[k*3] = k+2;
			}
			else
			{
				indexbuffer[k*3] = 1;
			}
		}

		for(int y=0; y<mOptions.Circles-1; y++)
		{
		    for(int x=0; x<mOptions.Steps; x++)
			{
			    unsigned int *twoface = indexbuffer + (y*mOptions.Steps+x)*6 + 3 * mOptions.Steps;

			    int p0 = 1+y * mOptions.Steps + x ;
			    int p1 = 1+y * mOptions.Steps + x + 1 ;
			    int p2 = 1+(y+1)* mOptions.Steps + x ;
			    int p3 = 1+(y+1)* mOptions.Steps + x + 1 ;

				if (x == mOptions.Steps-1)
				{
					p1 -= x+1;
					p3 -= x+1;
				}

				// First triangle
			    twoface[0]=p0;
			    twoface[1]=p1;
			    twoface[2]=p2;

				// Second triangle
			    twoface[3]=p1;
			    twoface[4]=p3;
			    twoface[5]=p2;
		    }
	    }

		// Prepare buffer for indices
		mMesh->getHardwareIndexBuffer() =
			Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(
			Ogre::HardwareIndexBuffer::IT_32BIT,
			numEle,
			Ogre::HardwareBuffer::HBU_STATIC, true);

		mMesh->getHardwareIndexBuffer()->
			writeData(0,
			          mMesh->getHardwareIndexBuffer()->getSizeInBytes(),
			          indexbuffer,
			          true);

		delete []indexbuffer;

		// Set index buffer for this submesh
		mMesh->getSubMesh()->indexData->indexBuffer = mMesh->getHardwareIndexBuffer();
		mMesh->getSubMesh()->indexData->indexStart = 0;
		mMesh->getSubMesh()->indexData->indexCount = numEle;

		return true;
	}

	void RadialGrid::remove()
	{
		if (!isCreated())
		{
			return;
		}

		Module::remove();

		if (mVertices)
		{
			if (getNormalMode() == MaterialManager::NM_VERTEX)
			{
				delete [] static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);
				mVertices = 0;
			}
			else if (getNormalMode() == MaterialManager::NM_RTT)
			{
				delete [] static_cast<Mesh::POS_VERTEX*>(mVertices);
				mVertices = 0;
			}
		}

		if (mVerticesChoppyBuffer)
		{
			delete [] mVerticesChoppyBuffer;
		}
	}

	void RadialGrid::saveCfg(Ogre::String &Data)
	{
		Module::saveCfg(Data);

		Data += CfgFileManager::_getCfgString("RG_Steps", mOptions.Steps);
		Data += CfgFileManager::_getCfgString("RG_Circles", mOptions.Circles);
		Data += CfgFileManager::_getCfgString("RG_Radius", mOptions.Radius);
		Data += CfgFileManager::_getCfgString("RG_Smooth", mOptions.Smooth);
		Data += CfgFileManager::_getCfgString("RG_ChoppyWaves", mOptions.ChoppyWaves);
		Data += CfgFileManager::_getCfgString("RG_ChoppyStrength", mOptions.ChoppyStrength);
		Data += CfgFileManager::_getCfgString("RG_StepSizeCube", mOptions.StepSizeCube);
		Data += CfgFileManager::_getCfgString("RG_StepSizeFive", mOptions.StepSizeFive);
		Data += CfgFileManager::_getCfgString("RG_StepSizeLin", mOptions.StepSizeLin);
		Data += CfgFileManager::_getCfgString("RG_Strength", mOptions.Strength);Data += "\n";
	}

	bool RadialGrid::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		if (!Module::loadCfg(CfgFile))
		{
			return false;
		}

		setOptions(
			Options(CfgFileManager::_getIntValue(CfgFile,   "RG_Steps"),
			        CfgFileManager::_getIntValue(CfgFile,  "RG_Circles"),
					CfgFileManager::_getFloatValue(CfgFile, "RG_Radius"),
					CfgFileManager::_getBoolValue(CfgFile,  "RG_Smooth"),
					CfgFileManager::_getBoolValue(CfgFile,  "RG_ChoppyWaves"),
					CfgFileManager::_getFloatValue(CfgFile, "RG_ChoppyStrength"),
					CfgFileManager::_getFloatValue(CfgFile, "RG_StepSizeCube"),
					CfgFileManager::_getFloatValue(CfgFile, "RG_StepSizeFive"),
					CfgFileManager::_getFloatValue(CfgFile, "RG_StepSizeLin"),
					CfgFileManager::_getFloatValue(CfgFile, "RG_Strength")));

		return true;
	}

	void RadialGrid::update(const Ogre::Real &timeSinceLastFrame)
	{
		if (!isCreated())
		{
			return;
		}

		Module::update(timeSinceLastFrame);

		// Update heigths
		int i, x, y;

		if (getNormalMode() == MaterialManager::NM_VERTEX)
		{
			Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

			if (mOptions.ChoppyWaves)
			{
				for(i = 0; i < 1+mOptions.Circles*mOptions.Steps; i++)
				{
					Vertices[i] = mVerticesChoppyBuffer[i];
				}
			}

			Ogre::Vector3 HydraxPos = mHydrax->getPosition();

			for(i = 0; i <1+mOptions.Circles*mOptions.Steps; i++)
			{
				Vertices[i].y = mNoise->getValue(HydraxPos.x + Vertices[i].x, HydraxPos.z + Vertices[i].z)*mOptions.Strength;
			}
		}
		else if (getNormalMode() == MaterialManager::NM_RTT)
		{
			Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

			Ogre::Vector3 HydraxPos = mHydrax->getPosition();

			for(i = 0; i <1+mOptions.Circles*mOptions.Steps; i++)
			{
				Vertices[i].y = mNoise->getValue(HydraxPos.x + Vertices[i].x, HydraxPos.z + Vertices[i].z)*mOptions.Strength;
			}
		}

		// Smooth the heightdata
		if (mOptions.Smooth)
		{
			if (getNormalMode() == MaterialManager::NM_VERTEX)
			{
				Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

				for(y=1;y<mOptions.Circles-1;y++)
				{
					for(x=1;x<mOptions.Steps-1;x++)
					{
						Vertices[y*mOptions.Steps + x].y =
							0.2f *
						   (Vertices[y*mOptions.Steps + x    ].y +
							Vertices[y*mOptions.Steps + x + 1].y +
							Vertices[y*mOptions.Steps + x - 1].y +
							Vertices[(y+1)*mOptions.Steps + x].y +
							Vertices[(y-1)*mOptions.Steps + x].y);
					}
				}
			}
			else if (getNormalMode() == MaterialManager::NM_RTT)
			{
				Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

				for(y=1;y<mOptions.Circles-1;y++)
				{
					for(x=1;x<mOptions.Steps-1;x++)
					{
						Vertices[y*mOptions.Steps + x].y =
							0.2f *
						   (Vertices[y*mOptions.Steps + x    ].y +
							Vertices[y*mOptions.Steps + x + 1].y +
							Vertices[y*mOptions.Steps + x - 1].y +
							Vertices[(y+1)*mOptions.Steps + x].y +
							Vertices[(y-1)*mOptions.Steps + x].y);
					}
				}
			}
		}

		// Update normals
		_calculeNormals();

		// Perform choppy waves
		_performChoppyWaves();

		// Upload geometry changes
		mHydrax->getMesh()->updateGeometry(1+mOptions.Steps * mOptions.Circles, mVertices);
	}

	void RadialGrid::_calculeNormals()
	{
		if (getNormalMode() != MaterialManager::NM_VERTEX)
		{
			return;
		}

		int x, y;
		Ogre::Vector3 vec1, vec2, normal;

		Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

		int Steps_4 = static_cast<int>(mOptions.Steps/4);

		// Calculate the normal of the center grid point
        vec2 = Ogre::Vector3(
					Vertices[1].x-Vertices[1+Steps_4*2].x,
					Vertices[1].y-Vertices[1+Steps_4*2].y,
					Vertices[1].z-Vertices[1+Steps_4*2].z);

		vec1 = Ogre::Vector3(
					Vertices[1+Steps_4].x - Vertices[1+Steps_4*3].x,
					Vertices[1+Steps_4].y - Vertices[1+Steps_4*3].y,
					Vertices[1+Steps_4].z - Vertices[1+Steps_4*3].z);

		normal = vec2.crossProduct(vec1);

		Vertices[0].nx = normal.x;
		Vertices[0].ny = normal.y;
		Vertices[0].nz = normal.z;

        // Calculate first circle normals
		for(x=0;x<mOptions.Steps;x++)
		{
			vec2 = Ogre::Vector3(
				Vertices[x + 2].x-Vertices[x].x,
				Vertices[x + 2].y-Vertices[x].y,
				Vertices[x + 2].z-Vertices[x].z);

			vec1 = Ogre::Vector3(
				Vertices[0].x - Vertices[mOptions.Steps + x].x,
				Vertices[0].y - Vertices[mOptions.Steps + x].y,
				Vertices[0].z - Vertices[mOptions.Steps + x].z);

			normal = vec2.crossProduct(vec1);

			Vertices[1+x].nx = normal.x;
			Vertices[1+x].ny = normal.y;
			Vertices[1+x].nz = normal.z;
		}

		// Calculate all the other vertex normals
		for(y=1;y<mOptions.Circles-1;y++)
		{
			for(x=0;x<mOptions.Steps;x++)
			{
				vec2 = Ogre::Vector3(
					Vertices[y*mOptions.Steps + x + 2].x-Vertices[y*mOptions.Steps + x].x,
					Vertices[y*mOptions.Steps + x + 2].y-Vertices[y*mOptions.Steps + x].y,
					Vertices[y*mOptions.Steps + x + 2].z-Vertices[y*mOptions.Steps + x].z);

				vec1 = Ogre::Vector3(
					Vertices[(y-1)*mOptions.Steps + x + 1].x - Vertices[(y+1)*mOptions.Steps + x].x,
					Vertices[(y-1)*mOptions.Steps + x + 1].y - Vertices[(y+1)*mOptions.Steps + x].y,
					Vertices[(y-1)*mOptions.Steps + x + 1].z - Vertices[(y+1)*mOptions.Steps + x].z);

				normal = vec2.crossProduct(vec1);

				Vertices[1+y*mOptions.Steps+x].nx = normal.x;
				Vertices[1+y*mOptions.Steps+x].ny = normal.y;
				Vertices[1+y*mOptions.Steps+x].nz = normal.z;
			}
	    }
	}

	void RadialGrid::_performChoppyWaves()
	{
		if (getNormalMode() != MaterialManager::NM_VERTEX || !mOptions.ChoppyWaves)
		{
			return;
		}

		int x, y,
			Underwater = 1;

		if (mHydrax->_isCurrentFrameUnderwater())
		{
			Underwater = -1;
		}

		Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

		Ogre::Vector2 Current, NearStep, CircleStep,
			          Proportion,
					  Dir, Perp,
					  Norm2;
		Ogre::Vector3 Norm;

		for(y=0;y<mOptions.Circles-1;y++)
		{
			Current = Ogre::Vector2(Vertices[y*mOptions.Steps + 1].x, Vertices[y*mOptions.Steps + 1].z);
			NearStep = Ogre::Vector2(Vertices[y*mOptions.Steps + 2].x, Vertices[y*mOptions.Steps + 2].z);
			CircleStep = Ogre::Vector2(Vertices[(y+1)*mOptions.Steps + 1].x, Vertices[(y+1)*mOptions.Steps + 1].z);

			Proportion = Ogre::Vector2(
				// Distance per step vertex
				(Current-NearStep).length(),
				// Distance per circle vertex
				(Current-CircleStep).length());

			for(x=0;x<mOptions.Steps;x++)
			{
				Dir = Ogre::Vector2(Vertices[1+y*mOptions.Steps + x].nx, Vertices[1+y*mOptions.Steps + x].nz).normalisedCopy();
				Perp = Dir.perpendicular();

				if (Dir.x < 0) Dir.x = -Dir.x;
		        if (Dir.y < 0) Dir.y = -Dir.y;

		        if (Perp.x < 0) Perp.x = -Perp.x;
		        if (Perp.y < 0) Perp.y = -Perp.y;

				Norm = Ogre::Vector3(
					   Vertices[1+y*mOptions.Steps + x].nx,
					   Vertices[1+y*mOptions.Steps + x].ny,
			           Vertices[1+y*mOptions.Steps + x].nz).normalisedCopy();

				Norm2 = Ogre::Vector2(Norm.x, Norm.z)  *
					          ( (Dir  * Proportion.x)   +
					            (Perp * Proportion.y))  *
				 			  mOptions.ChoppyStrength;

				Vertices[1+y*mOptions.Steps + x].x += Norm2.x * Underwater;
				Vertices[1+y*mOptions.Steps + x].z += Norm2.y * Underwater;
			}
		}
	}

	float RadialGrid::getHeigth(const Ogre::Vector2 &Position)
	{
		return mHydrax->getPosition().y + mNoise->getValue(Position.x, Position.y)*mOptions.Strength;
	}
}}
