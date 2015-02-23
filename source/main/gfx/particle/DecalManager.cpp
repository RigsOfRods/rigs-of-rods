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

//created by Thomas Fischer 22th of January 2011

#include "DecalManager.h"

#include <Ogre.h>

using namespace std;
using namespace Ogre;

// DecalManager
DecalManager::DecalManager()
{
	terrain_decals_snode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	terrain_decal_count = 0;
}

DecalManager::~DecalManager()
{
	// some initial settings for parsing
	decalSplineMode = false;
	splinemat = "";
	spline_export_fn = "";
	spline_width = 5;
	spline_segments_x = 10;
	spline_segments_y = 10;
	splinetex_u = 1;
	splinetex_v = 1;
	ground_offset=0.001f;
}

int DecalManager::addTerrainDecal(Ogre::Vector3 position, Ogre::Vector2 size, Ogre::Vector2 numSeg, Ogre::Real rotation, Ogre::String materialname, Ogre::String normalname)
{
#if 0
	Ogre::ManualObject *mo = gEnv->ogreSceneManager->createManualObject();
	String oname = mo->getName();
	SceneNode *mo_node = terrain_decals_snode->createChildSceneNode();

	mo->begin(materialname, Ogre::RenderOperation::OT_TRIANGLE_LIST);
	AxisAlignedBox *aab=new AxisAlignedBox();
	float uTile = 1, vTile = 1;

	Vector3 normal = Vector3(0,1,0); // UP
	
	int offset = 0;
	float ground_dist = 0.001f;

	Ogre::Vector3 vX = normal.perpendicular();
	Ogre::Vector3 vY = normal.crossProduct(vX);
	Ogre::Vector3 delta1 = size.x / numSeg.x * vX;
	Ogre::Vector3 delta2 = size.y / numSeg.y * vY;
	// build one corner of the square
	Ogre::Vector3 orig = -0.5*size.x*vX - 0.5*size.y*vY;

	for (int i1 = 0; i1<=numSeg.x; i1++)
		for (int i2 = 0; i2<=numSeg.y; i2++)
		{
			Vector3 pos = orig+i1*delta1+i2*delta2 + position;
			pos.y = hfinder->getHeightAt(pos.x, pos.z) + ground_dist;
			mo->position(pos);
			aab->merge(pos);
			mo->textureCoord(i1/(Ogre::Real)numSeg.x*uTile, i2/(Ogre::Real)numSeg.y*vTile);
			mo->normal(normal);
		}

	bool reverse = false;
	if (delta1.crossProduct(delta2).dotProduct(normal)>0)
		reverse= true;
	for (int n1 = 0; n1<numSeg.x; n1++)
	{
		for (int n2 = 0; n2<numSeg.y; n2++)
		{
			if (reverse)
			{
				mo->index(offset+0);
				mo->index(offset+(numSeg.y+1));
				mo->index(offset+1);
				mo->index(offset+1);
				mo->index(offset+(numSeg.y+1));
				mo->index(offset+(numSeg.y+1)+1);
			}
			else
			{
				mo->index(offset+0);
				mo->index(offset+1);
				mo->index(offset+(numSeg.y+1));
				mo->index(offset+1);
				mo->index(offset+(numSeg.y+1)+1);
				mo->index(offset+(numSeg.y+1));
			}
			offset++;
		}
		offset++;
	}
	offset+=numSeg.y+1;

	mo->end();
	mo->setBoundingBox(*aab);
	// some optimizations
	mo->setCastShadows(false);
	mo->setDynamic(false);
	delete(aab);

	MeshPtr mesh = mo->convertToMesh(oname+"_mesh");

	// build edgelist
	mesh->buildEdgeList();

	// remove the manualobject again, since we dont need it anymore
	gEnv->ogreSceneManager->destroyManualObject(mo);

	unsigned short src, dest;
	if (!mesh->suggestTangentVectorBuildParams(VES_TANGENT, src, dest))
	{
		mesh->buildTangentVectors(VES_TANGENT, src, dest);
	}
	
	Entity *ent = gEnv->ogreSceneManager->createEntity(oname+"_ent", oname+"_mesh");
	mo_node->attachObject(ent);

	mo_node->setVisible(true);
	//mo_node->showBoundingBox(true);
	mo_node->setPosition(Vector3::ZERO); //(position.x, 0, position.z));


	// RTSS
	//Ogre::RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(materialname, Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
	//Ogre::RTShader::ShaderGenerator::getSingleton().invalidateMaterial(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, materialname);
	RTSSgenerateShadersForMaterial(materialname, normalname);

#endif
	return 0;
}

int DecalManager::addTerrainSplineDecal(Ogre::SimpleSpline *spline, float width, Ogre::Vector2 numSeg, Ogre::Vector2 uvSeg, Ogre::String materialname, float ground_offset, Ogre::String export_fn, bool debug)
{
#if 0
	Ogre::ManualObject *mo = gEnv->ogreSceneManager->createManualObject();
	String oname = mo->getName();
	SceneNode *mo_node = terrain_decals_snode->createChildSceneNode();

	mo->begin(materialname, Ogre::RenderOperation::OT_TRIANGLE_LIST);
	AxisAlignedBox *aab=new AxisAlignedBox();

	int offset = 0;

	// how width is the road?
	float delta_width = width / numSeg.x;

	float steps_len = 1.0f / numSeg.x;

	for (int l = 0; l<=numSeg.x; l++)
	{
		// get current position on that spline
		Vector3 pos_cur  = spline->interpolate(steps_len * (float)l);
		Vector3 pos_next = spline->interpolate(steps_len * (float)(l + 1));
        Ogre::Vector3 direction = (pos_next - pos_cur);
		if (l == numSeg.x)
		{
			// last segment uses previous position
			pos_next = spline->interpolate(steps_len * (float)(l - 1));
			direction = (pos_cur - pos_next);
		}


		for (int w = 0; w<=numSeg.y; w++)
		{
			// build vector for the width
			Vector3 wn = direction.normalisedCopy().crossProduct(Vector3::UNIT_Y);
			
			// calculate the offset, spline in the middle
			Vector3 offset = (-0.5 * wn * width) + (w/numSeg.y) * wn * width;

			// push everything together
			Ogre::Vector3 pos = pos_cur + offset;
			// get ground height there
			pos.y = hfinder->getHeightAt(pos.x, pos.z) + ground_offset;

			// add the position to the mesh
			mo->position(pos);
			aab->merge(pos);
			mo->textureCoord(l/(Ogre::Real)numSeg.x*uvSeg.x, w/(Ogre::Real)numSeg.y*uvSeg.y);
			mo->normal(Vector3::UNIT_Y);
		}
	}

	bool reverse = false;
	for (int n1 = 0; n1<numSeg.x; n1++)
	{
		for (int n2 = 0; n2<numSeg.y; n2++)
		{
			if (reverse)
			{
				mo->index(offset+0);
				mo->index(offset+(numSeg.y+1));
				mo->index(offset+1);
				mo->index(offset+1);
				mo->index(offset+(numSeg.y+1));
				mo->index(offset+(numSeg.y+1)+1);
			}
			else
			{
				mo->index(offset+0);
				mo->index(offset+1);
				mo->index(offset+(numSeg.y+1));
				mo->index(offset+1);
				mo->index(offset+(numSeg.y+1)+1);
				mo->index(offset+(numSeg.y+1));
			}
			offset++;
		}
		offset++;
	}
	offset+=numSeg.y+1;

	mo->end();
	mo->setBoundingBox(*aab);
	// some optimizations
	mo->setCastShadows(false);
	mo->setDynamic(false);
	delete(aab);

	MeshPtr mesh = mo->convertToMesh(oname+"_mesh");

	// build edgelist
	mesh->buildEdgeList();

	// remove the manualobject again, since we dont need it anymore
	gEnv->ogreSceneManager->destroyManualObject(mo);

	unsigned short src, dest;
	if (!mesh->suggestTangentVectorBuildParams(VES_TANGENT, src, dest))
	{
		mesh->buildTangentVectors(VES_TANGENT, src, dest);
	}
	
	Entity *ent = gEnv->ogreSceneManager->createEntity(oname+"_ent", oname+"_mesh");
	mo_node->attachObject(ent);

	mo_node->setVisible(true);
	//mo_node->showBoundingBox(true);
	mo_node->setPosition(Vector3::ZERO); //(position.x, 0, position.z));


	if (!export_fn.empty())
	{
		MeshSerializer *ms = new MeshSerializer();
		ms->exportMesh(mesh.get(), export_fn);
		LOG("spline mesh exported as " + export_fn);
		delete(ms);
	}

	// TBD: RTSS
	//Ogre::RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(materialname, Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
	//Ogre::RTShader::ShaderGenerator::getSingleton().invalidateMaterial(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, materialname);
	RTSSgenerateShadersForMaterial(materialname, "");

#endif
	return 0;
}


int DecalManager::finishTerrainDecal()
{
#if 0
	// if if no decals
	if (!terrain_decals_snode->numChildren()) return 0;
	terrain_decal_count++;
	terrain_decals_sg = gEnv->ogreSceneManager->createStaticGeometry("terrain_decals_"+TOSTRING(terrain_decal_count));
	terrain_decals_sg->setCastShadows(false);
	terrain_decals_sg->addSceneNode(terrain_decals_snode);
	terrain_decals_sg->setRegionDimensions(Vector3(farclip/2.0, 10000.0, farclip/2.0));
	terrain_decals_sg->setRenderingDistance(farclip);
	try
	{
		terrain_decals_sg->build();
		terrain_decals_snode->detachAllObjects();
		//terrain_decals_snode->removeAllChildren();
		// crash under linux:
		//bakeNode->removeAndDestroyAllChildren();
	} catch(Ogre::Exception& e)
	{
		LOG("error while baking decals: " + e.getFullDescription());
	}
#endif
	return 0;
}


int DecalManager::parseLine(char *line)
{
	return 0;
}

// DecalSpline
DecalSpline::DecalSpline(Ogre::SceneNode* parent) : 
	  mo_spline(0)
	, mo_spline_node(0)
	, snparent(parent)
	, spline(0)
{
}

DecalSpline::~DecalSpline()
{
}

int DecalSpline::addPoint(Ogre::Vector3 v)
{
	return 0;
}

int DecalSpline::showDebugLine(bool enabled)
{
	if (enabled)
	{
		mo_spline = gEnv->sceneManager->createManualObject();
		if (snparent)
			mo_spline_node = snparent->createChildSceneNode();
		else
			mo_spline_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		mo_spline->begin("tracks/transred", Ogre::RenderOperation::OT_LINE_STRIP);
		for (float j=0;j<1;j+=0.001)
		{
			mo_spline->position(spline->interpolate(j));
		}
		mo_spline->end();
		mo_spline_node->attachObject(mo_spline);
	} else
	{
		if (mo_spline)
		{
			gEnv->sceneManager->destroyManualObject(mo_spline);
			delete(mo_spline);
			mo_spline=0;
		}
		if (mo_spline_node)
		{
			gEnv->sceneManager->destroySceneNode(mo_spline_node);
			delete(mo_spline_node);
			mo_spline_node=0;
		}
	}
	return 0;
}