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

Based on the Projected Grid concept from Claes Johanson thesis:
http://graphics.cs.lth.se/theses/projects/projgrid/
and Ren Cheng Ogre3D implementation:
http://www.cnblogs.com/ArenAK/archive/2007/11/07/951713.html
--------------------------------------------------------------------------------
*/

#include "Actor.h"
#include "GfxScene.h"

#include <ProjectedGrid.h>

#define _def_MaxFarClipDistance 99999

namespace Hydrax{namespace Module
{
	Mesh::VertexType _PG_getVertexTypeFromNormalMode(const MaterialManager::NormalMode& NormalMode)
	{
		if (NormalMode == MaterialManager::NM_VERTEX)
		{
			return Mesh::VT_POS_NORM;
		}

		// NM_RTT
		return Mesh::VT_POS;
	}

	Ogre::String _PG_getNormalModeString(const MaterialManager::NormalMode& NormalMode)
	{
		if (NormalMode == MaterialManager::NM_VERTEX)
		{
			return "Vertex";
		}

		return "Rtt";
	}

	ProjectedGrid::ProjectedGrid(Hydrax *h, Noise::Noise *n, const Ogre::Plane &BasePlane, const MaterialManager::NormalMode& NormalMode)
		: Module("ProjectedGrid" + _PG_getNormalModeString(NormalMode),
		         n, Mesh::Options(256, Size(0), _PG_getVertexTypeFromNormalMode(NormalMode)), NormalMode)
		, mHydrax(h)
		, mVertices(0)
		, mVerticesChoppyBuffer(0)
		, mBasePlane(BasePlane)
		, mNormal(BasePlane.normal)
		, mPos(Ogre::Vector3(0,0,0))
		, mProjectingCamera(0)
		, mTmpRndrngCamera(0)
		, mRenderingCamera(h->getCamera())
	{
	}

	ProjectedGrid::ProjectedGrid(Hydrax *h, Noise::Noise *n, const Ogre::Plane &BasePlane, const MaterialManager::NormalMode& NormalMode, const Options &Options)
		: Module("ProjectedGrid" + _PG_getNormalModeString(NormalMode),
		         n, Mesh::Options(Options.Complexity, Size(0), _PG_getVertexTypeFromNormalMode(NormalMode)), NormalMode)
		, mHydrax(h)
		, mVertices(0)
		, mVerticesChoppyBuffer(0)
		, mBasePlane(BasePlane)
		, mNormal(BasePlane.normal)
		, mPos(Ogre::Vector3(0,0,0))
		, mProjectingCamera(0)
		, mTmpRndrngCamera(0)
		, mRenderingCamera(h->getCamera())
	{
		setOptions(Options);
	}

	ProjectedGrid::~ProjectedGrid()
	{
		remove();

		HydraxLOG(getName() + " destroyed.");
	}

	void ProjectedGrid::setOptions(const Options &Options)
	{
		// Size(0) -> Infinite mesh
		mMeshOptions.MeshSize     = Size(0);
		mMeshOptions.MeshStrength = Options.Strength;
		mMeshOptions.MeshComplexity = Options.Complexity;

		mHydrax->getMesh()->setOptions(mMeshOptions);
		mHydrax->_setStrength(Options.Strength);

		// Re-create geometry if it's needed
		if (isCreated() && Options.Complexity != mOptions.Complexity)
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

			// Force to recalculate the geometry on next frame
			mLastPosition = Ogre::Vector3(0,0,0);
			mLastOrientation = Ogre::Quaternion();

			return;
		}

		mOptions = Options;
	}

	void ProjectedGrid::create()
	{
		HydraxLOG("Creating " + getName() + " module.");

		Module::create();

		if (getNormalMode() == MaterialManager::NM_VERTEX)
		{
		    mVertices = new Mesh::POS_NORM_VERTEX[mOptions.Complexity*mOptions.Complexity];

			Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

			for (int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
			{
				Vertices[i].nx = 0;
				Vertices[i].ny = -1;
				Vertices[i].nz = 0;
			}

			mVerticesChoppyBuffer = new Mesh::POS_NORM_VERTEX[mOptions.Complexity*mOptions.Complexity];
		}
		else if(getNormalMode() == MaterialManager::NM_RTT)
		{
			mVertices = new Mesh::POS_VERTEX[mOptions.Complexity*mOptions.Complexity];
		}

	    _setDisplacementAmplitude(0.0f);

		mTmpRndrngCamera  = new Ogre::Camera("PG_TmpRndrngCamera", NULL);
		mProjectingCamera = new Ogre::Camera("PG_ProjectingCamera", NULL);


                Ogre::SceneNode* mTmpRndrngCamera_snode = RoR::App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
                mTmpRndrngCamera_snode->attachObject(mTmpRndrngCamera);

                Ogre::SceneNode* mProjectingCamera_snode = RoR::App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
                mProjectingCamera_snode->attachObject(mProjectingCamera);

		HydraxLOG(getName() + " created.");
	}

	void ProjectedGrid::remove()
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

		if (mTmpRndrngCamera)
		{
			delete mTmpRndrngCamera;
			delete mProjectingCamera;
		}

		mLastPosition = Ogre::Vector3(0,0,0);
		mLastOrientation = Ogre::Quaternion();
	}

	void ProjectedGrid::saveCfg(Ogre::String &Data)
	{
		Module::saveCfg(Data);

		Data += CfgFileManager::_getCfgString("PG_ChoopyStrength", mOptions.ChoppyStrength);
		Data += CfgFileManager::_getCfgString("PG_ChoppyWaves", mOptions.ChoppyWaves);
		Data += CfgFileManager::_getCfgString("PG_Complexity", mOptions.Complexity);
		Data += CfgFileManager::_getCfgString("PG_Elevation", mOptions.Elevation);
		Data += CfgFileManager::_getCfgString("PG_ForceRecalculateGeometry", mOptions.ForceRecalculateGeometry);
		Data += CfgFileManager::_getCfgString("PG_Smooth", mOptions.Smooth);
		Data += CfgFileManager::_getCfgString("PG_Strength", mOptions.Strength); Data += "\n";
	}

	bool ProjectedGrid::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		if (!Module::loadCfg(CfgFile))
		{
			return false;
		}

        HydraxLOG("\tReading options...");
		setOptions(
			Options(CfgFileManager::_getIntValue(CfgFile,   "PG_Complexity"),
			        CfgFileManager::_getFloatValue(CfgFile, "PG_Strength"),
					CfgFileManager::_getFloatValue(CfgFile, "PG_Elevation"),
					CfgFileManager::_getBoolValue(CfgFile,  "PG_Smooth"),
					CfgFileManager::_getBoolValue(CfgFile,  "PG_ForceRecalculateGeometry"),
					CfgFileManager::_getBoolValue(CfgFile,  "PG_ChoppyWaves"),
					CfgFileManager::_getFloatValue(CfgFile, "PG_ChoopyStrength")));

        HydraxLOG("\tOptions readed.");

		return true;
	}

	void ProjectedGrid::update(const Ogre::Real &timeSinceLastFrame)
	{
		if (!isCreated())
		{
			return;
		}

		Module::update(timeSinceLastFrame);

		Ogre::Vector3 RenderingCameraPos = mRenderingCamera->getDerivedPosition();

		if (mLastPosition    != RenderingCameraPos    ||
			mLastOrientation != mRenderingCamera->getDerivedOrientation() ||
			mOptions.ForceRecalculateGeometry)
		{
			if (mLastPosition != RenderingCameraPos)
			{
				Ogre::Vector3 HydraxPos = Ogre::Vector3(RenderingCameraPos.x,mHydrax->getPosition().y,RenderingCameraPos.z);

		        mHydrax->getMesh()->getSceneNode()->setPosition(HydraxPos);
		        mHydrax->getRttManager()->getPlanesSceneNode()->setPosition(HydraxPos);

		        // For world-space -> object-space conversion
				mHydrax->setSunPosition(mHydrax->getSunPosition());
			}

			float RenderingFarClipDistance = mRenderingCamera->getFarClipDistance();

		    if (RenderingFarClipDistance > _def_MaxFarClipDistance)
		    {
			    mRenderingCamera->setFarClipDistance(_def_MaxFarClipDistance);
		    }

			mLastMinMax = _getMinMax(&mRange);

		    if (mLastMinMax)
		    {
			    _renderGeometry(mRange, mProjectingCamera->getViewMatrix(), RenderingCameraPos);

			    mHydrax->getMesh()->updateGeometry(mOptions.Complexity*mOptions.Complexity, mVertices);
		    }

			mRenderingCamera->setFarClipDistance(RenderingFarClipDistance);
		}
		else if (mLastMinMax)
		{
			if (getNormalMode() == MaterialManager::NM_VERTEX)
			{
				Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

				if (mOptions.ChoppyWaves)
				{
					for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
		            {
			            Vertices[i] = mVerticesChoppyBuffer[i];
						Vertices[i].y = -mBasePlane.d + mNoise->getValue(RenderingCameraPos.x + Vertices[i].x, RenderingCameraPos.z + Vertices[i].z)*mOptions.Strength;
		            }
				}
				else
				{
					for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
		            {
						Vertices[i].y = -mBasePlane.d + mNoise->getValue(RenderingCameraPos.x + Vertices[i].x, RenderingCameraPos.z + Vertices[i].z)*mOptions.Strength;
		            }
				}
			}
			else if (getNormalMode() == MaterialManager::NM_RTT)
			{
				Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

				for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
		        {
				    Vertices[i].y = -mBasePlane.d + mNoise->getValue(RenderingCameraPos.x + Vertices[i].x, RenderingCameraPos.z + Vertices[i].z)*mOptions.Strength;
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
				else if(getNormalMode() == MaterialManager::NM_RTT)
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

			_calculeNormals();

			_performChoppyWaves();

			mHydrax->getMesh()->updateGeometry(mOptions.Complexity*mOptions.Complexity, mVertices);
		}

		mLastPosition = RenderingCameraPos;
		mLastOrientation = mRenderingCamera->getDerivedOrientation();
	}

	bool ProjectedGrid::_renderGeometry(const Ogre::Matrix4& m,const Ogre::Matrix4& _viewMat, const Ogre::Vector3& WorldPos)
	{
		t_corners0 = _calculeWorldPosition(Ogre::Vector2( 0.0f, 0.0f),m,_viewMat);
		t_corners1 = _calculeWorldPosition(Ogre::Vector2(+1.0f, 0.0f),m,_viewMat);
		t_corners2 = _calculeWorldPosition(Ogre::Vector2( 0.0f,+1.0f),m,_viewMat);
		t_corners3 = _calculeWorldPosition(Ogre::Vector2(+1.0f,+1.0f),m,_viewMat);

		float du  = 1.0f/(mOptions.Complexity-1),
			  dv  = 1.0f/(mOptions.Complexity-1),
			  u,v = 0.0f,
			  // _1_u = (1.0f-u)
			  _1_u, _1_v = 1.0f,
			  divide;

		Ogre::Vector4 result;

		int i = 0, iv, iu;

		if (getNormalMode() == MaterialManager::NM_VERTEX)
		{
			Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

			for(iv=0; iv<mOptions.Complexity; iv++)
			{
				u = 0.0f;
				_1_u = 1.0f;
				for(iu=0; iu<mOptions.Complexity; iu++)
				{
					result.x = _1_v*(_1_u*t_corners0.x + u*t_corners1.x) + v*(_1_u*t_corners2.x + u*t_corners3.x);
					result.z = _1_v*(_1_u*t_corners0.z + u*t_corners1.z) + v*(_1_u*t_corners2.z + u*t_corners3.z);
					result.w = _1_v*(_1_u*t_corners0.w + u*t_corners1.w) + v*(_1_u*t_corners2.w + u*t_corners3.w);

					divide = 1.0f/result.w;
					result.x *= divide;
					result.z *= divide;

					Vertices[i].x = result.x;
					Vertices[i].z = result.z;
					Vertices[i].y = -mBasePlane.d + mNoise->getValue(WorldPos.x + result.x, WorldPos.z + result.z)*mOptions.Strength;

					i++;
					u += du;
					_1_u = 1.0f-u;
				}
				v += dv;
				_1_v = 1.0f-v;
			}

			if (mOptions.ChoppyWaves)
			{
				for(int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++)
		        {
			        mVerticesChoppyBuffer[i] = Vertices[i];
		        }
			}
		}
		else if(getNormalMode() == MaterialManager::NM_RTT)
		{
			Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

			for(iv=0; iv<mOptions.Complexity; iv++)
			{
				u = 0.0f;
				_1_u = 1.0f;
				for(iu=0; iu<mOptions.Complexity; iu++)
				{
					result.x = _1_v*(_1_u*t_corners0.x + u*t_corners1.x) + v*(_1_u*t_corners2.x + u*t_corners3.x);
					result.z = _1_v*(_1_u*t_corners0.z + u*t_corners1.z) + v*(_1_u*t_corners2.z + u*t_corners3.z);
					result.w = _1_v*(_1_u*t_corners0.w + u*t_corners1.w) + v*(_1_u*t_corners2.w + u*t_corners3.w);

					divide = 1.0f/result.w;
					result.x *= divide;
					result.z *= divide;

					Vertices[i].x = result.x;
					Vertices[i].z = result.z;
					Vertices[i].y = -mBasePlane.d + mNoise->getValue(WorldPos.x + result.x, WorldPos.z + result.z)*mOptions.Strength;

					i++;
					u += du;
					_1_u = 1.0f-u;
				}
				v += dv;
				_1_v = 1.0f-v;
			}
		}

		// Smooth the heightdata
		if (mOptions.Smooth)
		{
			if (getNormalMode() == MaterialManager::NM_VERTEX)
			{
				Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

				for(iv=1; iv<(mOptions.Complexity-1); iv++)
				{
					for(iu=1; iu<(mOptions.Complexity-1); iu++)
					{
						Vertices[iv*mOptions.Complexity + iu].y =
							 0.2f *
							(Vertices[iv    *mOptions.Complexity + iu    ].y +
							 Vertices[iv    *mOptions.Complexity + (iu+1)].y +
							 Vertices[iv    *mOptions.Complexity + (iu-1)].y +
							 Vertices[(iv+1)*mOptions.Complexity + iu    ].y +
							 Vertices[(iv-1)*mOptions.Complexity + iu    ].y);
					}
				}
			}
			else if(getNormalMode() == MaterialManager::NM_RTT)
			{
				Mesh::POS_VERTEX* Vertices = static_cast<Mesh::POS_VERTEX*>(mVertices);

				for(iv=1; iv<(mOptions.Complexity-1); iv++)
				{
					for(iu=1; iu<(mOptions.Complexity-1); iu++)
					{
						Vertices[iv*mOptions.Complexity + iu].y =
							 0.2f *
							(Vertices[iv    *mOptions.Complexity + iu    ].y +
							 Vertices[iv    *mOptions.Complexity + (iu+1)].y +
							 Vertices[iv    *mOptions.Complexity + (iu-1)].y +
							 Vertices[(iv+1)*mOptions.Complexity + iu    ].y +
							 Vertices[(iv-1)*mOptions.Complexity + iu    ].y);
					}
				}
			}
		}

		_calculeNormals();

		_performChoppyWaves();

		return true;
	}

	void ProjectedGrid::_calculeNormals()
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

	void ProjectedGrid::_performChoppyWaves()
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

		float Dis1,  Dis2;//,
		   // Dis1_, Dis2_;

		Ogre::Vector3 CameraDir, Norm;
		Ogre::Vector2 Dir, Perp, Norm2;

		CameraDir = mRenderingCamera->getDerivedDirection();
		Dir       = Ogre::Vector2(CameraDir.x, CameraDir.z).normalisedCopy();
		Perp      = Dir.perpendicular();

		if (Dir.x < 0 ) Dir.x = -Dir.x;
		if (Dir.y < 0 ) Dir.y = -Dir.y;

		if (Perp.x < 0 ) Perp.x = -Perp.x;
		if (Perp.y < 0 ) Perp.y = -Perp.y;

		Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);

		for(v=1; v<(mOptions.Complexity-1); v++)
		{
			Dis1 =  (Ogre::Vector2(mVerticesChoppyBuffer[v*mOptions.Complexity + 1].x,
					               mVerticesChoppyBuffer[v*mOptions.Complexity + 1].z) -
					 Ogre::Vector2(mVerticesChoppyBuffer[(v+1)*mOptions.Complexity + 1].x,
				                   mVerticesChoppyBuffer[(v+1)*mOptions.Complexity + 1].z)).length();

			/*Dis1_ = (Ogre::Vector2(mVerticesChoppyBuffer[v*mOptions.Complexity + 1].x,
                                   mVerticesChoppyBuffer[v*mOptions.Complexity + 1].z) -
				   	 Ogre::Vector2(mVerticesChoppyBuffer[(v-1)*mOptions.Complexity + 1].x,
					               mVerticesChoppyBuffer[(v-1)*mOptions.Complexity + 1].z)).length();

			Dis1 = (Dis1+Dis1_)/2;*/

			for(u=1; u<(mOptions.Complexity-1); u++)
			{
				Dis2 = (Ogre::Vector2(mVerticesChoppyBuffer[v*mOptions.Complexity + u].x,
					                  mVerticesChoppyBuffer[v*mOptions.Complexity + u].z) -
					    Ogre::Vector2(mVerticesChoppyBuffer[v*mOptions.Complexity + u+1].x,
					                  mVerticesChoppyBuffer[v*mOptions.Complexity + u+1].z)).length();
/*
				Dis2_ = (Ogre::Vector2(mVerticesChoppyBuffer[v*mOptions.Complexity + u].x,
					                   mVerticesChoppyBuffer[v*mOptions.Complexity + u].z) -
					     Ogre::Vector2(mVerticesChoppyBuffer[v*mOptions.Complexity + u-1].x,
				                       mVerticesChoppyBuffer[v*mOptions.Complexity + u-1].z)).length();

				Dis2 = (Dis2+Dis2_)/2;*/

				Norm = Ogre::Vector3(Vertices[v*mOptions.Complexity + u].nx,
					                 Vertices[v*mOptions.Complexity + u].ny,
								     Vertices[v*mOptions.Complexity + u].nz).
					   			     normalisedCopy();

				Norm2 = Ogre::Vector2(Norm.x, Norm.z)  *
					                 ( (Dir  * Dis1)   +
					                   (Perp * Dis2))  *
				 				      mOptions.ChoppyStrength;

				Vertices[v*mOptions.Complexity + u].x = mVerticesChoppyBuffer[v*mOptions.Complexity + u].x + Norm2.x * Underwater;
				Vertices[v*mOptions.Complexity + u].z = mVerticesChoppyBuffer[v*mOptions.Complexity + u].z + Norm2.y * Underwater;
			}
		}
	}

	// Check the point of intersection with the plane (0,1,0,0) and return the position in homogenous coordinates
	Ogre::Vector4 ProjectedGrid::_calculeWorldPosition(const Ogre::Vector2 &uv, const Ogre::Matrix4& m, const Ogre::Matrix4& _viewMat)
	{
		Ogre::Vector4 origin(uv.x,uv.y,-1,1);
		Ogre::Vector4 direction(uv.x,uv.y,1,1);

		origin = m*origin;
		direction = m*direction;

		Ogre::Vector3 _org(origin.x/origin.w,origin.y/origin.w,origin.z/origin.w);
		Ogre::Vector3 _dir(direction.x/direction.w,direction.y/direction.w,direction.z/direction.w);
		_dir -= _org;
		_dir.normalise();

		Ogre::Ray _ray(_org,_dir);
		std::pair<bool,Ogre::Real> _result = _ray.intersects(mBasePlane);
		float l = _result.second;
		Ogre::Vector3 worldPos = _org + _dir*l;
		Ogre::Vector4 _tempVec = _viewMat*Ogre::Vector4(worldPos);
		float _temp = -_tempVec.z/_tempVec.w;
		Ogre::Vector4 retPos(worldPos);
		retPos /= _temp;

		return retPos;
	}

	bool ProjectedGrid::_getMinMax(Ogre::Matrix4 *range)
	{
		_setDisplacementAmplitude(mOptions.Strength);

		float x_min,y_min,x_max,y_max;
		Ogre::Vector3 frustum[8],proj_points[24];

		int i,
			n_points = 0,
			src, dst;

		int cube[] =
		   {0,1,	0,2,	2,3,	1,3,
		    0,4,	2,6,	3,7,	1,5,
		    4,6,	4,5,	5,7,	6,7};

		Ogre::Vector3 _testLine;
		Ogre::Real _dist;
		Ogre::Ray _ray;

		std::pair<bool,Ogre::Real> _result;

		// Set temporal rendering camera parameters
		mTmpRndrngCamera->setFrustumOffset(mRenderingCamera->getFrustumOffset());
		mTmpRndrngCamera->setAspectRatio(mRenderingCamera->getAspectRatio());
		mTmpRndrngCamera->getParentSceneNode()->setDirection(mRenderingCamera->getDerivedDirection());
		mTmpRndrngCamera->setFarClipDistance(mRenderingCamera->getFarClipDistance());
		mTmpRndrngCamera->setFOVy(mRenderingCamera->getFOVy());
		mTmpRndrngCamera->setNearClipDistance(mRenderingCamera->getNearClipDistance());
		mTmpRndrngCamera->getParentSceneNode()->setOrientation(mRenderingCamera->getDerivedOrientation());
		mTmpRndrngCamera->getParentSceneNode()->setPosition(0, mRenderingCamera->getDerivedPosition().y - mHydrax->getPosition().y, 0);

		Ogre::Matrix4 invviewproj = (mTmpRndrngCamera->getProjectionMatrixWithRSDepth()*mTmpRndrngCamera->getViewMatrix()).inverse();
		frustum[0] = invviewproj * Ogre::Vector3(-1,-1,0);
		frustum[1] = invviewproj * Ogre::Vector3(+1,-1,0);
		frustum[2] = invviewproj * Ogre::Vector3(-1,+1,0);
		frustum[3] = invviewproj * Ogre::Vector3(+1,+1,0);
		frustum[4] = invviewproj * Ogre::Vector3(-1,-1,+1);
		frustum[5] = invviewproj * Ogre::Vector3(+1,-1,+1);
		frustum[6] = invviewproj * Ogre::Vector3(-1,+1,+1);
		frustum[7] = invviewproj * Ogre::Vector3(+1,+1,+1);

		// Check intersections with upper_bound and lower_bound
		for(i=0; i<12; i++)
		{
			src=cube[i*2]; dst=cube[i*2+1];
			_testLine = frustum[dst]-frustum[src];
			_dist = _testLine.normalise();
			_ray = Ogre::Ray(frustum[src], _testLine);
			_result = Ogre::Math::intersects(_ray,mUpperBoundPlane);
			if ((_result.first) && (_result.second<_dist+0.00001))
			{
				proj_points[n_points++] = frustum[src] + _result.second * _testLine;
			}
			_result = Ogre::Math::intersects(_ray,mLowerBoundPlane);
			if ((_result.first) && (_result.second<_dist+0.00001))
			{
				proj_points[n_points++] = frustum[src] + _result.second * _testLine;
			}
		}

		// Check if any of the frustums vertices lie between the upper_bound and lower_bound planes
		for(i=0; i<8; i++)
		{
			if(mUpperBoundPlane.getDistance(frustum[i])/mLowerBoundPlane.getDistance(frustum[i]) < 0)
			{
				proj_points[n_points++] = frustum[i];
			}
		}

		// Set projecting camera parameters
		mProjectingCamera->setFrustumOffset(mTmpRndrngCamera->getFrustumOffset());
		mProjectingCamera->setAspectRatio(mTmpRndrngCamera->getAspectRatio());
		mProjectingCamera->getParentSceneNode()->setDirection(mTmpRndrngCamera->getDerivedDirection());
		mProjectingCamera->setFarClipDistance(mTmpRndrngCamera->getFarClipDistance());
		mProjectingCamera->setFOVy(mTmpRndrngCamera->getFOVy());
		mProjectingCamera->setNearClipDistance(mTmpRndrngCamera->getNearClipDistance());
		mProjectingCamera->getParentSceneNode()->setOrientation(mTmpRndrngCamera->getDerivedOrientation());
		mProjectingCamera->getParentSceneNode()->setPosition(mTmpRndrngCamera->getDerivedPosition());

		// Make sure the camera isn't too close to the plane
		float height_in_plane = mBasePlane.getDistance(mProjectingCamera->getRealPosition());

		bool keep_it_simple = false,
			 underwater     = false;

		if (height_in_plane < 0.0f)
		{
			underwater = true;
		}

		if (keep_it_simple)
		{
			mProjectingCamera->getParentSceneNode()->setDirection(mTmpRndrngCamera->getDerivedDirection());
		}
		else
		{
			Ogre::Vector3 aimpoint, aimpoint2;

			if (height_in_plane < (mOptions.Strength + mOptions.Elevation))
			{
				if (underwater)
				{
					mProjectingCamera->getParentSceneNode()->setPosition(mProjectingCamera->getRealPosition()+mLowerBoundPlane.normal*(mOptions.Strength + mOptions.Elevation - 2*height_in_plane));
				}
				else
				{
					mProjectingCamera->getParentSceneNode()->setPosition(mProjectingCamera->getRealPosition()+mLowerBoundPlane.normal*(mOptions.Strength + mOptions.Elevation - height_in_plane));
				}
			}

			// Aim the projector at the point where the camera view-vector intersects the plane
			// if the camera is aimed away from the plane, mirror it's view-vector against the plane
			if (((mBasePlane.normal).dotProduct(mTmpRndrngCamera->getDerivedDirection()) < 0.0f) || ((mBasePlane.normal).dotProduct(mTmpRndrngCamera->getDerivedPosition()) < 0.0f ) )
			{
				_ray = Ogre::Ray(mTmpRndrngCamera->getDerivedPosition(), mTmpRndrngCamera->getDerivedDirection());
				_result = Ogre::Math::intersects(_ray,mBasePlane);

				if(!_result.first)
				{
					_result.second = -_result.second;
				}

				aimpoint = mTmpRndrngCamera->getDerivedPosition() + _result.second * mTmpRndrngCamera->getDerivedDirection();
			}
			else
			{
				Ogre::Vector3 flipped = mTmpRndrngCamera->getDerivedDirection() - 2*mNormal* (mTmpRndrngCamera->getDerivedDirection()).dotProduct(mNormal);
				flipped.normalise();
				_ray = Ogre::Ray( mTmpRndrngCamera->getDerivedPosition(), flipped);
				_result = Ogre::Math::intersects(_ray,mBasePlane);

				aimpoint = mTmpRndrngCamera->getDerivedPosition() + _result.second * flipped;
			}

			// Force the point the camera is looking at in a plane, and have the projector look at it
			// works well against horizon, even when camera is looking upwards
			// doesn't work straight down/up
			float af = fabs((mBasePlane.normal).dotProduct(mTmpRndrngCamera->getDerivedDirection()));
			aimpoint2 = mTmpRndrngCamera->getDerivedPosition() + 10.0*mTmpRndrngCamera->getDerivedDirection();
			aimpoint2 = aimpoint2 - mNormal* (aimpoint2.dotProduct(mNormal));

			// Fade between aimpoint & aimpoint2 depending on view angle
			aimpoint = aimpoint*af + aimpoint2*(1.0f-af);

			mProjectingCamera->getParentSceneNode()->setDirection(aimpoint-mProjectingCamera->getRealPosition());
		}

		for(i=0; i<n_points; i++)
		{
			// Project the point onto the surface plane
			proj_points[i] = proj_points[i] - mBasePlane.normal*mBasePlane.getDistance(proj_points[i]);
			proj_points[i] = mProjectingCamera->getViewMatrix() * proj_points[i];
			proj_points[i] = mProjectingCamera->getProjectionMatrixWithRSDepth() * proj_points[i];
		}

		// Get max/min x & y-values to determine how big the "projection window" must be
		if (n_points > 0)
		{
			x_min = proj_points[0].x;
			x_max = proj_points[0].x;
			y_min = proj_points[0].y;
			y_max = proj_points[0].y;

			for(i=1; i<n_points; i++)
			{
				if (proj_points[i].x > x_max) x_max = proj_points[i].x;
				if (proj_points[i].x < x_min) x_min = proj_points[i].x;
				if (proj_points[i].y > y_max) y_max = proj_points[i].y;
				if (proj_points[i].y < y_min) y_min = proj_points[i].y;
			}

			// Build the packing matrix that spreads the grid across the "projection window"
			Ogre::Matrix4 pack(x_max-x_min,	0,				0,		x_min,
				               0,			y_max-y_min,	0,		y_min,
				               0,			0,				1,		0,
				               0,			0,				0,		1);

			Ogre::Matrix4 invviewproj = (mProjectingCamera->getProjectionMatrixWithRSDepth()*mProjectingCamera->getViewMatrix()).inverse();
			*range = invviewproj * pack;

			return true;
		}

		return false;
	}

	void ProjectedGrid::_setDisplacementAmplitude(const float &Amplitude)
	{
		mUpperBoundPlane = Ogre::Plane( mNormal, mPos + Amplitude * mNormal);
		mLowerBoundPlane = Ogre::Plane( mNormal, mPos - Amplitude * mNormal);
	}

	float ProjectedGrid::getHeigth(const Ogre::Vector2 &Position)
	{
		return mHydrax->getPosition().y + mNoise->getValue(Position.x, Position.y)*mOptions.Strength;
	}
}}
