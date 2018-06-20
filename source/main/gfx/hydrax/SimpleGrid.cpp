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

#include <SimpleGrid.h>

namespace Hydrax{namespace Module
{
	Mesh::VertexType _SG_getVertexTypeFromNormalMode(const MaterialManager::NormalMode& NormalMode)
	{
		if (NormalMode == MaterialManager::NM_VERTEX)
		{
			return Mesh::VT_POS_NORM;
		}

		// NM_RTT
		return Mesh::VT_POS;
	}

	Ogre::String _SG_getNormalModeString(const MaterialManager::NormalMode& NormalMode)
	{
		if (NormalMode == MaterialManager::NM_VERTEX)
		{
			return "Vertex";
		}

		return "Rtt";
	}

	SimpleGrid::SimpleGrid(Hydrax *h, Noise::Noise *n, const MaterialManager::NormalMode& NormalMode)
		: Module("SimpleGrid" + _SG_getNormalModeString(NormalMode),
		         n, Mesh::Options(256, Size(100), _SG_getVertexTypeFromNormalMode(NormalMode)), NormalMode)
		, mHydrax(h)
		, mVertices(0)
		, mVerticesChoppyBuffer(0)
	{
	}

	SimpleGrid::SimpleGrid(Hydrax *h, Noise::Noise *n, const MaterialManager::NormalMode& NormalMode, const Options &Options)
		: Module("SimpleGrid" + _SG_getNormalModeString(NormalMode),
		         n, Mesh::Options(Options.Complexity, Size(Options.MeshSize), _SG_getVertexTypeFromNormalMode(NormalMode)), NormalMode)
		, mHydrax(h)
		, mVertices(0)
		, mVerticesChoppyBuffer(0)
	{
		setOptions(Options);
	}

	SimpleGrid::~SimpleGrid()
	{
		remove();

		HydraxLOG(getName() + " destroyed.");
	}

	void SimpleGrid::setOptions(const Options &Options)
	{
		mMeshOptions.MeshSize     = Options.MeshSize;
		mMeshOptions.MeshStrength = Options.Strength;
		mMeshOptions.MeshComplexity = Options.Complexity;

		mHydrax->getMesh()->setOptions(mMeshOptions);
		mHydrax->_setStrength(Options.Strength);

		if (isCreated())
		{
			if (Options.Complexity != mOptions.Complexity || Options.ChoppyWaves != Options.ChoppyWaves)
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

			int v, u;
			if (getNormalMode() == MaterialManager::NM_VERTEX)
			{
				Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

				for(v=0; v<mOptions.Complexity; v++)
				{
					for(u=0; u<mOptions.Complexity; u++)
					{
						Vertices[v*mOptions.Complexity + u].x  = (static_cast<float>(v)/(mOptions.Complexity-1)) * mOptions.MeshSize.Width;
						Vertices[v*mOptions.Complexity + u].z  = (static_cast<float>(u)/(mOptions.Complexity-1)) * mOptions.MeshSize.Height;
					}
				}

				if (mOptions.ChoppyWaves && mVerticesChoppyBuffer)
			    {
				    for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
				    {
					    mVerticesChoppyBuffer[i] = Vertices[i];
				    }
				}
			}
			else if (getNormalMode() == MaterialManager::NM_RTT)
			{
				Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

				for(v=0; v<mOptions.Complexity; v++)
				{
					for(u=0; u<mOptions.Complexity; u++)
					{
						Vertices[v*mOptions.Complexity + u].x  = (static_cast<float>(v)/(mOptions.Complexity-1)) * mOptions.MeshSize.Width;
						Vertices[v*mOptions.Complexity + u].z  = (static_cast<float>(u)/(mOptions.Complexity-1)) * mOptions.MeshSize.Height;
					}
				}
			}

			return;
		}

		mOptions = Options;
	}

	void SimpleGrid::create()
	{
		HydraxLOG("Creating " + getName() + " module.");

		Module::create();

		int v, u;
		if (getNormalMode() == MaterialManager::NM_VERTEX)
		{
			mVertices = new Mesh::POS_NORM_VERTEX[mOptions.Complexity*mOptions.Complexity];
			Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

			for(v=0; v<mOptions.Complexity; v++)
			{
				for(u=0; u<mOptions.Complexity; u++)
				{
					Vertices[v*mOptions.Complexity + u].x  = (static_cast<float>(v)/(mOptions.Complexity-1)) * mOptions.MeshSize.Width;
					Vertices[v*mOptions.Complexity + u].z  = (static_cast<float>(u)/(mOptions.Complexity-1)) * mOptions.MeshSize.Height;

					Vertices[v*mOptions.Complexity + u].nx = 0;
					Vertices[v*mOptions.Complexity + u].ny = -1;
					Vertices[v*mOptions.Complexity + u].nz = 0;
				}
			}

			if (mOptions.ChoppyWaves)
			{
				mVerticesChoppyBuffer = new Mesh::POS_NORM_VERTEX[mOptions.Complexity*mOptions.Complexity];

				for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
				{
					mVerticesChoppyBuffer[i] = Vertices[i];
				}
			}
		}
		else if (getNormalMode() == MaterialManager::NM_RTT)
		{
			mVertices = new Mesh::POS_VERTEX[mOptions.Complexity*mOptions.Complexity];
			Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

			for(v=0; v<mOptions.Complexity; v++)
			{
				for(u=0; u<mOptions.Complexity; u++)
				{
					Vertices[v*mOptions.Complexity + u].x  = (static_cast<float>(v)/(mOptions.Complexity-1)) * mOptions.MeshSize.Width;
					Vertices[v*mOptions.Complexity + u].z  = (static_cast<float>(u)/(mOptions.Complexity-1)) * mOptions.MeshSize.Height;
				}
			}
		}

		HydraxLOG(getName() + " created.");
	}

	void SimpleGrid::remove()
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

	void SimpleGrid::saveCfg(Ogre::String &Data)
	{
		Module::saveCfg(Data);

		Data += CfgFileManager::_getCfgString("SG_Complexity", mOptions.Complexity);
		Data += CfgFileManager::_getCfgString("SG_MeshSize", mOptions.MeshSize);
		Data += CfgFileManager::_getCfgString("SG_Strength", mOptions.Strength);
		Data += CfgFileManager::_getCfgString("SG_Smooth", mOptions.Smooth);
		Data += CfgFileManager::_getCfgString("SG_ChoppyWaves", mOptions.ChoppyWaves);
		Data += CfgFileManager::_getCfgString("SG_ChoppyStrength", mOptions.ChoppyStrength); Data += "\n";
	}

	bool SimpleGrid::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		if (!Module::loadCfg(CfgFile))
		{
			return false;
		}

		setOptions(
			Options(CfgFileManager::_getIntValue(CfgFile,   "SG_Complexity"),
			        CfgFileManager::_getSizeValue(CfgFile,  "SG_MeshSize"),
					CfgFileManager::_getFloatValue(CfgFile, "SG_Strength"),
					CfgFileManager::_getBoolValue(CfgFile,  "PG_Smooth"),
					CfgFileManager::_getBoolValue(CfgFile,  "PG_ChoppyWaves"),
					CfgFileManager::_getFloatValue(CfgFile, "PG_ChoopyStrength")));

		return true;
	}

	void SimpleGrid::update(const Ogre::Real &timeSinceLastFrame)
	{
		if (!isCreated())
		{
			return;
		}

		Module::update(timeSinceLastFrame);

		// Update heigths

		if (getNormalMode() == MaterialManager::NM_VERTEX)
		{
			Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

			if (mOptions.ChoppyWaves)
			{
				for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
				{
					Vertices[i] = mVerticesChoppyBuffer[i];
					Vertices[i].y = mNoise->getValue(Vertices[i].x, Vertices[i].z) * mOptions.Strength;
				}
			}
			else
			{
			    for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
				{
					Vertices[i].y = mNoise->getValue(Vertices[i].x, Vertices[i].z) * mOptions.Strength;
				}
			}
		}
		else if (getNormalMode() == MaterialManager::NM_RTT)
		{
			Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

			// For object-space to world-space conversion
			// RTT normals calculation needs world-space coords
			Ogre::Vector3 p = Ogre::Vector3(0,0,0);
		    Ogre::Affine3 mWorldMatrix;
            #if OGRE_VERSION_MAJOR >= 1 && OGRE_VERSION_MINOR >= 7
                mWorldMatrix = mHydrax->getMesh()->getEntity()->getParentSceneNode()->_getFullTransform();
            #else
                mHydrax->getMesh()->getEntity()->getParentSceneNode()->getWorldTransforms(&mWorldMatrix);
            #endif
			for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
			{
				p.x = Vertices[i].x;
				p.y = 0;
				p.z = Vertices[i].z;

				// Calculate the world-space position
				mWorldMatrix*(p);

				Vertices[i].y = mNoise->getValue(p.x, p.z) * mOptions.Strength;
			}
		}

		// Smooth the heightdata
		if (mOptions.Smooth)
		{
			if (getNormalMode() == MaterialManager::NM_VERTEX)
			{
				Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

				for(int v=1; v<(mOptions.Complexity-1); v++)
				{
					for(int u=1; u<(mOptions.Complexity-1); u++)
					{
						Vertices[v*mOptions.Complexity + u].y =
							 0.2f *
							(Vertices[v    *mOptions.Complexity + u    ].y +
							 Vertices[v    *mOptions.Complexity + (u+1)].y +
							 Vertices[v    *mOptions.Complexity + (u-1)].y +
							 Vertices[(v+1)*mOptions.Complexity + u    ].y +
							 Vertices[(v-1)*mOptions.Complexity + u    ].y);
					}
				}
			}
			else if (getNormalMode() == MaterialManager::NM_RTT)
			{
				Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

				for(int v=1; v<(mOptions.Complexity-1); v++)
				{
					for(int u=1; u<(mOptions.Complexity-1); u++)
					{
						Vertices[v*mOptions.Complexity + u].y =
							 0.2f *
							(Vertices[v    *mOptions.Complexity + u    ].y +
							 Vertices[v    *mOptions.Complexity + (u+1)].y +
							 Vertices[v    *mOptions.Complexity + (u-1)].y +
							 Vertices[(v+1)*mOptions.Complexity + u    ].y +
							 Vertices[(v-1)*mOptions.Complexity + u    ].y);
					}
				}
			}
		}

		// Update normals
		_calculeNormals();

		// Perform choppy waves
		_performChoppyWaves();

		// Upload geometry changes
		mHydrax->getMesh()->updateGeometry(mOptions.Complexity*mOptions.Complexity, mVertices);
	}

	void SimpleGrid::_calculeNormals()
	{
		if (getNormalMode() != MaterialManager::NM_VERTEX)
		{
			return;
		}

		int v, u;
		Ogre::Vector3 vec1, vec2, normal;

		Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

		for(v=1; v<(mOptions.Complexity-1); v++)
		{
			for(u=1; u<(mOptions.Complexity-1); u++)
			{
				vec1 = Ogre::Vector3(
					Vertices[v*mOptions.Complexity + u + 1].x-Vertices[v*mOptions.Complexity + u - 1].x,
					Vertices[v*mOptions.Complexity + u + 1].y-Vertices[v*mOptions.Complexity + u - 1].y,
					Vertices[v*mOptions.Complexity + u + 1].z-Vertices[v*mOptions.Complexity + u - 1].z);

				vec2 = Ogre::Vector3(
					Vertices[(v+1)*mOptions.Complexity + u].x - Vertices[(v-1)*mOptions.Complexity + u].x,
					Vertices[(v+1)*mOptions.Complexity + u].y - Vertices[(v-1)*mOptions.Complexity + u].y,
					Vertices[(v+1)*mOptions.Complexity + u].z - Vertices[(v-1)*mOptions.Complexity + u].z);

				normal = vec2.crossProduct(vec1);

				Vertices[v*mOptions.Complexity + u].nx = normal.x;
				Vertices[v*mOptions.Complexity + u].ny = normal.y;
				Vertices[v*mOptions.Complexity + u].nz = normal.z;
			}
		}
	}

	void SimpleGrid::_performChoppyWaves()
	{
		if (getNormalMode() != MaterialManager::NM_VERTEX || !mOptions.ChoppyWaves)
		{
			return;
		}

		int v, u,
			Underwater = 1;

		if (mHydrax->_isCurrentFrameUnderwater())
		{
			Underwater = -1;
		}

		Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

		for(v=1; v<(mOptions.Complexity-1); v++)
		{
			for(u=1; u<(mOptions.Complexity-1); u++)
			{
				Vertices[v*mOptions.Complexity + u].x += Vertices[v*mOptions.Complexity + u].nx * mOptions.ChoppyStrength * Underwater;
				Vertices[v*mOptions.Complexity + u].z += Vertices[v*mOptions.Complexity + u].nz * mOptions.ChoppyStrength * Underwater;
			}
		}
	}

	float SimpleGrid::getHeigth(const Ogre::Vector2 &Position)
	{
		if (getNormalMode() != MaterialManager::NM_RTT)
		{
		    Ogre::Vector2 RelativePos = mHydrax->getMesh()->getGridPosition(Position);

		    RelativePos.x *= mOptions.MeshSize.Width;
		    RelativePos.y *= mOptions.MeshSize.Height;

		    return mHydrax->getPosition().y + mNoise->getValue(RelativePos.x, RelativePos.y)*mOptions.Strength;
		}
		else // RTT Normals calculations works with world-space coords
		{
			return mHydrax->getPosition().y + mNoise->getValue(Position.x, Position.y)*mOptions.Strength;
		}
	}
}}
