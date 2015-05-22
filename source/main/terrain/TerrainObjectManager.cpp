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
#include "TerrainObjectManager.h"

#include "Application.h"
#include "AutoPilot.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "Collisions.h"
#include "ErrorUtils.h"
#include "ExtinguishableFireAffector.h"
#include "Language.h"
#include "LoadingWindow.h"
#include "MeshObject.h"
#include "ProceduralManager.h"
#include "ResourceBuffer.h"
#include "Road2.h"
#include "ScopeLog.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "SurveyMapEntity.h"
#include "SurveyMapManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainManager.h"
#include "WriteTextToTexture.h"

#include <OgreRTShaderSystem.h>
#include <OgreFontManager.h>

using namespace Ogre;

#ifdef USE_PAGED
using namespace Forests;
#endif //USE_PAGED


//workaround for pagedgeometry
inline float getTerrainHeight(Real x, Real z, void *unused=0)
{
	if (!gEnv->terrainManager->getHeightFinder())
		return 1;
	return gEnv->terrainManager->getHeightFinder()->getHeightAt(x, z);
}


TerrainObjectManager::TerrainObjectManager(TerrainManager *terrainManager) :
	terrainManager(terrainManager)
{
	//prepare for baking
	bakeNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
}

TerrainObjectManager::~TerrainObjectManager()
{
#ifdef USE_PAGED
	for (std::vector<paged_geometry_t>::iterator it=pagedGeometry.begin(); it!=pagedGeometry.end(); it++)
	{
		if (it->geom)
		{
			delete(it->geom);
			it->geom=0;
		}
		if (it->loader)
		{
			delete((TreeLoader2D *)it->loader);
			it->loader=0;
		}
	}
#endif //USE_PAGED
	if (bakesg != nullptr)
	{
		gEnv->sceneManager->destroyStaticGeometry("bakeSG");
		bakesg = nullptr;
	}
	gEnv->sceneManager->destroyAllEntities();
}

void TerrainObjectManager::proceduralTests()
{
#if 0
	// TODO: it crashes, get this working!
	try
	{
		Procedural::MultiShape out;
		Procedural::SvgLoader svg;
		svg.parseSvgFile(out, "test.svg");
		Procedural::Path p = out.getShape(0).convertToPath();
		Procedural::Shape s = out.getShape(1);
		
		Procedural::Track textureTrack = Procedural::Track(Procedural::Track::AM_POINT).addKeyFrame(0,0).addKeyFrame(2,.2f).addKeyFrame(3,.8f).addKeyFrame(5,1);
		// The extruder actually creates the road mesh from all parameters
		Procedural::Extruder().setExtrusionPath(&p).setShapeToExtrude(&s).setShapeTextureTrack(&textureTrack).setUTile(20.f).realizeMesh("extrudedMesh");

		Entity* ent2 = gEnv->sceneManager->createEntity("svg");
		SceneNode* sn = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		sn->attachObject(ent2);
		sn->setPosition(Vector3(0,0,0));
		ent2->setMaterialName("Examples/Road");
		ent2->setCastShadows(true);

	} catch(Exception &e)
	{
		ErrorUtils::ShowError("Error within procedural tests", e.what());
	}
#endif //0
}

void TerrainObjectManager::loadObjectConfigFile(Ogre::String odefname)
{
	proceduralManager = new ProceduralManager();

	objcounter = 0;
	free_localizer = 0;

	ProceduralObject po;
	po.loadingState = -1;
	int r2oldmode = 0;
	int lastprogress = -1;
	bool proroad = false;

	DataStreamPtr ds;
	try
	{
		ds = ResourceGroupManager::getSingleton().openResource(odefname, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
	}
	catch(...)
	{
		LOG("Error opening object configuration: " + odefname);
		return;
	}

	int mapsizex = terrainManager->getGeometryManager()->getMaxTerrainSize().x;
	int mapsizez = terrainManager->getGeometryManager()->getMaxTerrainSize().z;

	Vector3 r2lastpos=Vector3::ZERO;
	Quaternion r2lastrot=Quaternion::IDENTITY;
	int r2counter=0;

	//long line = 0;
	char line[4096] = "";

	while (!ds->eof())
	{
		int progress = ((float)(ds->tell()) / (float)(ds->size())) * 100.0f;
		if (progress-lastprogress > 20)
		{
#ifdef USE_MYGUI
			LoadingWindow::getSingleton().setProgress(progress, _L("Loading Terrain Objects"));
#endif //MYGUI
			lastprogress = progress;
		}

		char oname[1024] = {};
		char type[256] = {};
		char name[256] = {};
		Vector3 pos(Vector3::ZERO);
		Vector3 rot(Vector3::ZERO);

		size_t ll = ds->readLine(line, 1023);
		if (line[0]=='/' || line[0]==';' || ll==0) continue; //comments
		if (!strcmp("end",line)) break;

		if (!strncmp(line,"collision-tris", 14))
		{
			long amount = Collisions::MAX_COLLISION_TRIS;
			sscanf(line, "collision-tris %ld", &amount);
			gEnv->collisions->resizeMemory(amount);
		}

		if (!strncmp(line,"grid", 4))
		{
			sscanf(line, "grid %f, %f, %f", &pos.x, &pos.y, &pos.z);

			Ogre::ColourValue BackgroundColour = Ogre::ColourValue::White;//Ogre::ColourValue(0.1337f, 0.1337f, 0.1337f, 1.0f);
			Ogre::ColourValue GridColour = Ogre::ColourValue(0.2f, 0.2f, 0.2f, 1.0f);

			Ogre::ManualObject *mReferenceObject = new Ogre::ManualObject("ReferenceGrid");

			mReferenceObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST);

			Ogre::Real step = 1.0f;
			unsigned int count = 50;
			unsigned int halfCount = count / 2;
			Ogre::Real full = (step * count);
			Ogre::Real half = full / 2;
			Ogre::Real y = 0;
			Ogre::ColourValue c;
			for (unsigned i=0;i < count+1;i++)
			{
				if (i == halfCount)
					c = Ogre::ColourValue(1,0,0,1.0f);
				else
					c = GridColour;

				mReferenceObject->position(-half,y,-half+(step*i));
				mReferenceObject->colour(BackgroundColour);
				mReferenceObject->position(0,y,-half+(step*i));
				mReferenceObject->colour(c);
				mReferenceObject->position(0,y,-half+(step*i));
				mReferenceObject->colour(c);
				mReferenceObject->position(half,y,-half+(step*i));
				mReferenceObject->colour(BackgroundColour);

				if (i == halfCount)
					c = Ogre::ColourValue(0,0,1,1.0f);
				else
					c = GridColour;

				mReferenceObject->position(-half+(step*i),y,-half);
				mReferenceObject->colour(BackgroundColour);
				mReferenceObject->position(-half+(step*i),y,0);
				mReferenceObject->colour(c);
				mReferenceObject->position(-half+(step*i),y,0);
				mReferenceObject->colour(c);
				mReferenceObject->position(-half+(step*i),y, half);
				mReferenceObject->colour(BackgroundColour);
			}

			mReferenceObject->end();
			mReferenceObject->setCastShadows(false);
			
			SceneNode *n = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
			n->setPosition(pos);
			n->attachObject(mReferenceObject);
			n->setVisible(true);
		}
#ifdef USE_PAGED
		//ugly stuff to parse trees :)
		if (!strncmp("trees", line, 5))
		{
			if (terrainManager->getPagedMode() == 0) continue;
			char ColorMap[256] = {};
			char DensityMap[256] = {};
			char treemesh[256] = {};
			char treeCollmesh[256] = {};
			float gridspacing = 0.0f;
			float yawfrom = 0.0f, yawto = 0.0f;
			float scalefrom = 0.0f, scaleto = 0.0f;
			float highdens = 1.0f;
			int minDist = 90, maxDist = 700;
			sscanf(line, "trees %f, %f, %f, %f, %f, %d, %d, %s %s %s %f %s", &yawfrom, &yawto, &scalefrom, &scaleto, &highdens, &minDist, &maxDist, treemesh, ColorMap, DensityMap, &gridspacing, treeCollmesh);
			if (strnlen(ColorMap, 3) == 0)
			{
				LOG("tree ColorMap map zero!");
				continue;
			}
			if (strnlen(DensityMap, 3) == 0)
			{
				LOG("tree DensityMap zero!");
				continue;
			}
			Forests::DensityMap *densityMap = Forests::DensityMap::load(DensityMap, Forests::CHANNEL_COLOR);
			if (!densityMap)
			{
				LOG("could not load densityMap: "+String(DensityMap));
				continue;
			}
			densityMap->setFilter(Forests::MAPFILTER_BILINEAR);
			//densityMap->setMapBounds(TRect(0, 0, mapsizex, mapsizez));

			paged_geometry_t paged;
			paged.geom = new PagedGeometry();
			paged.geom->setTempDir(SSETTING("User Path", "") + "cache" + SSETTING("dirsep", "\\"));
			paged.geom->setCamera(gEnv->mainCamera);
			paged.geom->setPageSize(50);
			paged.geom->setInfinite();
			Ogre::TRect<Ogre::Real> bounds = TBounds(0, 0, mapsizex, mapsizez);
			paged.geom->setBounds(bounds);

			//Set up LODs
			//trees->addDetailLevel<EntityPage>(50);
			float min = minDist * terrainManager->getPagedDetailFactor();
			if (min<10) min = 10;
			paged.geom->addDetailLevel<BatchPage>(min, min/2);
			float max = maxDist * terrainManager->getPagedDetailFactor();
			if (max<10) max = 10;
			paged.geom->addDetailLevel<ImpostorPage>(max, max/10);
			TreeLoader2D *treeLoader = new TreeLoader2D(paged.geom, TBounds(0, 0, mapsizex, mapsizez));
			paged.geom->setPageLoader(treeLoader);
			treeLoader->setHeightFunction(&getTerrainHeight);
			if (String(ColorMap) != "none")
			{
				treeLoader->setColorMap(ColorMap);
			}

			Entity *curTree = gEnv->sceneManager->createEntity(String("paged_")+treemesh+TOSTRING(pagedGeometry.size()), treemesh);

			if (gridspacing > 0)
			{
				// grid style
				for (float x=0; x < mapsizex; x += gridspacing)
				{
					for (float z=0; z < mapsizez; z += gridspacing)
					{
						float density = densityMap->_getDensityAt_Unfiltered(x, z, bounds);
						if (density < 0.8f) continue;
						float nx = x + gridspacing * 0.5f;
						float nz = z + gridspacing * 0.5f;
						float yaw = Math::RangeRandom(yawfrom, yawto);
						float scale = Math::RangeRandom(scalefrom, scaleto);
						Vector3 pos = Vector3(nx, 0, nz);
						treeLoader->addTree(curTree, pos, Degree(yaw), (Ogre::Real)scale);
						if (strlen(treeCollmesh))
						{
							pos.y = gEnv->terrainManager->getHeightFinder()->getHeightAt(pos.x, pos.z);
							scale *= 0.1f;
							gEnv->collisions->addCollisionMesh(String(treeCollmesh), pos, Quaternion(Degree(yaw), Vector3::UNIT_Y), Vector3(scale, scale, scale));
						}
					}
				}

			} else
			{
				float gridsize = 10;
				if (gridspacing < 0 && gridspacing != 0)
				{
					gridsize = -gridspacing;
				}
				float hd = highdens;
				// normal style, random
				for (float x=0; x < mapsizex; x += gridsize)
				{
					for (float z=0; z < mapsizez; z += gridsize)
					{
						if (highdens < 0) hd = Math::RangeRandom(0, -highdens);
						float density = densityMap->_getDensityAt_Unfiltered(x, z, bounds);
						int numTreesToPlace = (int)((float)(hd) * density * terrainManager->getPagedDetailFactor());
						float nx=0, nz=0;
						while(numTreesToPlace-->0)
						{
							nx = Math::RangeRandom(x, x + gridsize);
							nz = Math::RangeRandom(z, z + gridsize);
							float yaw = Math::RangeRandom(yawfrom, yawto);
							float scale = Math::RangeRandom(scalefrom, scaleto);
							Vector3 pos = Vector3(nx, 0, nz);
							treeLoader->addTree(curTree, pos, Degree(yaw), (Ogre::Real)scale);
							if (strlen(treeCollmesh))
							{
								pos.y = gEnv->terrainManager->getHeightFinder()->getHeightAt(pos.x, pos.z);
								gEnv->collisions->addCollisionMesh(String(treeCollmesh),pos, Quaternion(Degree(yaw), Vector3::UNIT_Y), Vector3(scale, scale, scale));
							}
						}
					}
				}
			}
			paged.loader = (void*)treeLoader;
			pagedGeometry.push_back(paged);
		}

		//ugly stuff to parse grass :)
		if (!strncmp("grass", line, 5) || !strncmp("grass2", line, 6))
		{
			// is paged geometry disabled by configuration?
			if (terrainManager->getPagedMode() == 0) continue;
			int range = 80;
			float SwaySpeed=0.5, SwayLength=0.05, SwayDistribution=10.0, minx=0.2, miny=0.2, maxx=1, maxy=0.6, Density=0.6, minH=-9999, maxH=9999;
			char grassmat[256]="";
			char colorMapFilename[256]="";
			char densityMapFilename[256]="";
			int growtechnique = 0;
			int techn = GRASSTECH_CROSSQUADS;
			if (!strncmp("grass2", line, 6))
				sscanf(line, "grass2 %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %d, %s %s %s", &range, &SwaySpeed, &SwayLength, &SwayDistribution, &Density, &minx, &miny, &maxx, &maxy, &growtechnique, &minH, &maxH, &techn, grassmat, colorMapFilename, densityMapFilename);
			else if (!strncmp("grass", line, 5))
				sscanf(line, "grass %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %s %s %s", &range, &SwaySpeed, &SwayLength, &SwayDistribution, &Density, &minx, &miny, &maxx, &maxy, &growtechnique, &minH, &maxH, grassmat, colorMapFilename, densityMapFilename);

			//Initialize the PagedGeometry engine
			try
			{
				paged_geometry_t paged;
				PagedGeometry *grass = new PagedGeometry(gEnv->mainCamera, 30);
				//Set up LODs

				grass->addDetailLevel<GrassPage>(range * terrainManager->getPagedDetailFactor()); // original value: 80

				//Set up a GrassLoader for easy use
				GrassLoader *grassLoader = new GrassLoader(grass);
				grass->setPageLoader(grassLoader);
				grassLoader->setHeightFunction(&getTerrainHeight);

				// render grass at first
				grassLoader->setRenderQueueGroup(RENDER_QUEUE_MAIN-1);

				GrassLayer* grassLayer = grassLoader->addLayer(grassmat);
				grassLayer->setHeightRange(minH, maxH);
				grassLayer->setLightingEnabled(true);

				grassLayer->setAnimationEnabled((SwaySpeed>0));
				grassLayer->setSwaySpeed(SwaySpeed);
				grassLayer->setSwayLength(SwayLength);
				grassLayer->setSwayDistribution(SwayDistribution);

				//String grassdensityTextureFilename = String(DensityMap);

				grassLayer->setDensity(Density * terrainManager->getPagedDetailFactor());
				if (techn>10)
					grassLayer->setRenderTechnique(static_cast<GrassTechnique>(techn-10), true);
				else
					grassLayer->setRenderTechnique(static_cast<GrassTechnique>(techn), false);

				grassLayer->setMapBounds(TBounds(0, 0, mapsizex, mapsizez));

				if (strcmp(colorMapFilename,"none") != 0)
				{
					grassLayer->setColorMap(colorMapFilename);
					grassLayer->setColorMapFilter(MAPFILTER_BILINEAR);
				}

				if (strcmp(densityMapFilename,"none") != 0)
				{
					grassLayer->setDensityMap(densityMapFilename);
					grassLayer->setDensityMapFilter(MAPFILTER_BILINEAR);
				}

				//grassLayer->setMinimumSize(0.5,0.5);
				//grassLayer->setMaximumSize(1.0, 1.0);

				grassLayer->setMinimumSize(minx, miny);
				grassLayer->setMaximumSize(maxx, maxy);

				// growtechnique
				if (growtechnique == 0)
					grassLayer->setFadeTechnique(FADETECH_GROW);
				else if (growtechnique == 1)
					grassLayer->setFadeTechnique(FADETECH_ALPHAGROW);
				else if (growtechnique == 2)
					grassLayer->setFadeTechnique(FADETECH_ALPHA);
				paged.geom = grass;
				paged.loader = (void*)grassLoader;
				pagedGeometry.push_back(paged);
			} catch(...)
			{
				LOG("error loading grass!");
			}

			continue;
		}
#endif //USE_PAGED

		{ // ugly stuff to parse procedural roads
			if (!strncmp("begin_procedural_roads", line, 22))
			{
				po = ProceduralObject();
				po.loadingState = 1;
				r2oldmode = 1;
				proroad = true;
				continue;
			}
			if (!strncmp("end_procedural_roads", line, 20))
			{
				if (r2oldmode)
				{
					if (proceduralManager) proceduralManager->addObject(po);
					po = ProceduralObject();
				}
				proroad = false;
				continue;
			}
			if (proroad)
			{
				float rwidth, bwidth, bheight;
				//position x,y,z rotation rx,ry,rz, width, border width, border height, type
				int r = sscanf(line, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %s",&pos.x,&pos.y,&pos.z, &rot.x, &rot.y, &rot.z, &rwidth, &bwidth, &bheight, oname);
				Quaternion rotation = Quaternion(Degree(rot.x), Vector3::UNIT_X)*Quaternion(Degree(rot.y), Vector3::UNIT_Y)*Quaternion(Degree(rot.z), Vector3::UNIT_Z);
				int roadtype=Road2::ROAD_AUTOMATIC;
				int pillartype = 0;
				if (!strcmp(oname, "flat")) roadtype=Road2::ROAD_FLAT;
				if (!strcmp(oname, "left")) roadtype=Road2::ROAD_LEFT;
				if (!strcmp(oname, "right")) roadtype=Road2::ROAD_RIGHT;
				if (!strcmp(oname, "both")) roadtype=Road2::ROAD_BOTH;
				if (!strcmp(oname, "bridge")) {roadtype=Road2::ROAD_BRIDGE;pillartype=1;}
				if (!strcmp(oname, "monorail")) {roadtype=Road2::ROAD_MONORAIL;pillartype=2;}
				if (!strcmp(oname, "monorail2")) {roadtype=Road2::ROAD_MONORAIL;pillartype=0;}
				if (!strcmp(oname, "bridge_no_pillars")) {roadtype=Road2::ROAD_BRIDGE;pillartype=0;}

				if (r2oldmode)
				{
					//fill object
					ProceduralPoint pp;
					pp.bheight = bheight;
					pp.bwidth = bwidth;
					pp.pillartype = pillartype;
					pp.position = pos;
					pp.rotation = rotation;
					pp.type = roadtype;
					pp.width = rwidth;

					po.points.push_back(pp);
				}
				continue;
			}
		} //end of the ugly (somewhat)

		strcpy(name, "generic");
		memset(oname, 0, 255);
		memset(type, 0, 255);
		memset(name, 0, 255);
		int r = sscanf(line, "%f, %f, %f, %f, %f, %f, %s %s %s",&pos.x,&pos.y,&pos.z, &rot.x, &rot.y, &rot.z, oname, type, name);
		if (r < 6) continue;
		if ((!strcmp(oname, "truck")) || (!strcmp(oname, "load") || (!strcmp(oname, "machine")) || (!strcmp(oname, "boat")) || (!strcmp(oname, "truck2")) ))
		{
			if (!strcmp(oname, "boat") && !terrainManager->getWater())
			{
				// no water so do not load boats!
				continue;
			}
			String group = "";
			String truckname(type);

			if (!RoR::Application::GetCacheSystem()->checkResourceLoaded(truckname, group))
			{
				LOG("Error while loading Terrain: truck " + String(type) + " not found. ignoring.");
				continue;
			}

			truck_prepare_t tempTruckPreload;
			//this is a truck or load declaration
			tempTruckPreload.px = pos.x;
			tempTruckPreload.py = pos.y;
			tempTruckPreload.pz = pos.z;
			tempTruckPreload.freePosition = (!strcmp(oname, "truck2"));
			tempTruckPreload.ismachine = (!strcmp(oname, "machine"));
			tempTruckPreload.rotation = Quaternion(Degree(rot.x), Vector3::UNIT_X)*Quaternion(Degree(rot.y), Vector3::UNIT_Y)*Quaternion(Degree(rot.z), Vector3::UNIT_Z);
			strcpy(tempTruckPreload.name, truckname.c_str());
			truck_preload.push_back(tempTruckPreload);

			continue;
		}
		if (   !strcmp(oname, "road")
			|| !strcmp(oname, "roadborderleft")
			|| !strcmp(oname, "roadborderright")
			|| !strcmp(oname, "roadborderboth")
			|| !strcmp(oname, "roadbridgenopillar")
			|| !strcmp(oname, "roadbridge"))
		{
			int pillartype = !(strcmp(oname, "roadbridgenopillar") == 0);
			// okay, this is a job for roads2
			int roadtype=Road2::ROAD_AUTOMATIC;
			if (!strcmp(oname, "road")) roadtype=Road2::ROAD_FLAT;
			Quaternion rotation;
			rotation = Quaternion(Degree(rot.x), Vector3::UNIT_X)*Quaternion(Degree(rot.y), Vector3::UNIT_Y)*Quaternion(Degree(rot.z), Vector3::UNIT_Z);
			if (pos.distance(r2lastpos) > 20.0f)
			{
				// break the road
				if (r2oldmode != 0)
				{
					// fill object
					ProceduralPoint pp;
					pp.bheight = 0.2;
					pp.bwidth = 1.4;
					pp.pillartype = pillartype;
					pp.position = r2lastpos + r2lastrot * Vector3(10.0f, 0.0f, 0.9f);
					pp.rotation = r2lastrot;
					pp.type = roadtype;
					pp.width = 8;
					po.points.push_back(pp);

					// finish it and start new object
					if (proceduralManager) proceduralManager->addObject(po);
					po = ProceduralObject();
					r2oldmode = 1;
				}
				r2oldmode = 1;
				// beginning of new
				ProceduralPoint pp;
				pp.bheight = 0.2;
				pp.bwidth = 1.4;
				pp.pillartype = pillartype;
				pp.position = pos;
				pp.rotation = rotation;
				pp.type = roadtype;
				pp.width = 8;
				po.points.push_back(pp);
			} else
			{
				// fill object
				ProceduralPoint pp;
				pp.bheight = 0.2;
				pp.bwidth = 1.4;
				pp.pillartype = pillartype;
				pp.position = pos;
				pp.rotation = rotation;
				pp.type = roadtype;
				pp.width = 8;
				po.points.push_back(pp);
			}
			r2lastpos=pos;
			r2lastrot=rotation;

			continue;
		}
		loadObject(oname, pos, rot, bakeNode, name, type);
	}

	// ds closes automatically, so do not close it explicitly here: ds->close();

	// finish the last road
	if (r2oldmode != 0)
	{
		// fill object
		ProceduralPoint pp;
		pp.bheight = 0.2;
		pp.bwidth = 1.4;
		pp.pillartype = 1;
		pp.position = r2lastpos+r2lastrot*Vector3(10.0,0,0);
		pp.rotation = r2lastrot;
		pp.type = Road2::ROAD_AUTOMATIC;
		pp.width = 8;
		po.points.push_back(pp);

		// finish it and start new object
		if (proceduralManager) proceduralManager->addObject(po);
	}
}

void TerrainObjectManager::postLoad()
{
	// okay, now bake everything
	bakesg = gEnv->sceneManager->createStaticGeometry("bakeSG");
	bakesg->setCastShadows(true);
	bakesg->addSceneNode(bakeNode);
	bakesg->setRegionDimensions(Vector3(terrainManager->getFarClip() / 2.0f, 10000.0, terrainManager->getFarClip() / 2.0f));
	bakesg->setRenderingDistance(terrainManager->getFarClip());
	try
	{
		bakesg->build();
		bakeNode->detachAllObjects();
		// crash under linux:
		//bakeNode->removeAndDestroyAllChildren();
	} catch(...)
	{
		LOG("error while baking roads. ignoring.");

	}
}

void removeCollisionTri(int number)
{
	if (gEnv->collisions) gEnv->collisions->removeCollisionTri(number);
}

void removeCollisionBox(int number)
{
	if (gEnv->collisions) gEnv->collisions->removeCollisionBox(number);
}

void TerrainObjectManager::unloadObject(const String &instancename)
{
	if (loadedObjects.find(instancename) == loadedObjects.end())
	{
		LOG("unable to unload object: " + instancename);
		return;
	}

	loadedObject_t obj = loadedObjects[instancename];

	// check if it was already deleted
	if (!obj.enabled)
		return;

	// unload any collision tris
	std::for_each(obj.collTris.begin(), obj.collTris.end(), removeCollisionTri);
	// and any collision boxes
	std::for_each(obj.collBoxes.begin(), obj.collBoxes.end(), removeCollisionBox);

	obj.sceneNode->detachAllObjects();
	obj.sceneNode->setVisible(false);
	obj.enabled = false;
}

void TerrainObjectManager::loadObject(const Ogre::String &name, const Ogre::Vector3 &pos, const Ogre::Vector3 &rot, Ogre::SceneNode *bakeNode, const Ogre::String &instancename, const Ogre::String &type, bool enable_collisions /* = true */, int scripthandler /* = -1 */, bool uniquifyMaterial /* = false */)
{
	ScopeLog log("object_"+name);

	if (type == "grid")
	{
		// some fast grid object hacks :)
		for (int x=0; x < 500; x += 50)
		{
			for (int z=0; z < 500; z += 50)
			{
				const String notype = "";
				loadObject(name, pos + Vector3(x, 0.0f, z), rot, bakeNode, name, notype, enable_collisions, scripthandler, uniquifyMaterial);
			}
		}
		return;
	}

	// nice idea, but too many random hits
	//if (abs(rot.x+1) < 0.001) rot.x = Math::RangeRandom(0, 360);
	//if (abs(rot.y+1) < 0.001) rot.y = Math::RangeRandom(0, 360);
	//if (abs(rot.z+1) < 0.001) rot.z = Math::RangeRandom(0, 360);

	if (name.empty()) return;

	//FILE *fd;
	//char oname[1024] = {};
	char mesh[1024] = {};
	char line[1024] = {};
	char collmesh[1024] = {};
	Vector3 l(Vector3::ZERO);
	Vector3 h(Vector3::ZERO);
	Vector3 dr(Vector3::ZERO);
	Vector3 fc(Vector3::ZERO);
	Vector3 sc(Vector3::ZERO);
	Vector3 sr(Vector3::ZERO);
	bool forcecam=false;
	bool ismovable=false;

	int event_filter = EVENT_ALL;

	Quaternion rotation = Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z);

	// try to load with UID first!
	String odefgroup = "";
	String odefname = name + ".odef";

	bool odefFound = false;

	bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(odefname);
	if (exists)
	{
		odefgroup = ResourceGroupManager::getSingleton().findGroupContainingResource(odefname);
		odefFound = true;
	}
	
	if (!RoR::Application::GetCacheSystem()->checkResourceLoaded(odefname, odefgroup))
		if (!odefFound)
		{
			LOG("Error while loading Terrain: could not find required .odef file: " + odefname + ". Ignoring entry.");
			return;
		}

		DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(odefname, odefgroup);

		ds->readLine(mesh, 1023);
		if (String(mesh) == "LOD")
		{
			// LOD line is obsolete
			ds->readLine(mesh, 1023);
		}

		//scale
		ds->readLine(line, 1023);
		sscanf(line, "%f, %f, %f",&sc.x, &sc.y, &sc.z);
		String entityName = "object" + TOSTRING(objcounter) + "(" + name + ")";
		objcounter++;

		SceneNode *tenode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		bool background_loading = BSETTING("Background Loading", false);

		MeshObject *mo = NULL;
		if (String(mesh) != "none")
		{
			mo = new MeshObject(mesh, entityName, tenode, NULL, background_loading);
		}

		//mo->setQueryFlags(OBJECTS_MASK);
		//tenode->attachObject(te);
		tenode->setScale(sc);
		tenode->setPosition(pos);
		tenode->rotate(rotation);
		tenode->pitch(Degree(-90));
		tenode->setVisible(true);

		// register in map
		loadedObject_t *obj = &loadedObjects[instancename];
		obj->instanceName = instancename;
		obj->loadType     = 0;
		obj->enabled      = true;
		obj->sceneNode    = tenode;
		obj->collTris.clear();

		if (mo && uniquifyMaterial && !instancename.empty())
		{
			for (unsigned int i = 0; i < mo->getEntity()->getNumSubEntities(); i++)
			{
				SubEntity *se = mo->getEntity()->getSubEntity(i);
				String matname = se->getMaterialName();
				String newmatname = matname + "/" + instancename;
				//LOG("subentity " + TOSTRING(i) + ": "+ matname + " -> " + newmatname);
				se->getMaterial()->clone(newmatname);
				se->setMaterialName(newmatname);
			}
		}

		//String meshGroup = ResourceGroupManager::getSingleton().findGroupContainingResource(mesh);
		//MeshPtr mainMesh = mo->getMesh();

		//collision box(es)
		bool virt=false;
		bool rotating=false;
		bool classic_ref=true;
		// everything is of concrete by default
		ground_model_t *gm = gEnv->collisions->getGroundModelByString("concrete");
		char eventname[256] = {};
		while (!ds->eof())
		{
			size_t ll=ds->readLine(line, 1023);

			// little workaround to trim it
			String lineStr = String(line);
			Ogre::StringUtil::trim(lineStr);

			const char* ptline = lineStr.c_str();
			if (ll==0 || line[0]=='/' || line[0]==';') continue;

			if (!strcmp("end",ptline)) break;
			if (!strcmp("movable", ptline)) {ismovable=true;continue;};
			if (!strcmp("localizer-h", ptline))
			{
				localizers[free_localizer].position=Vector3(pos.x,pos.y,pos.z);
				localizers[free_localizer].rotation=rotation;
				localizers[free_localizer].type=Autopilot::LOCALIZER_HORIZONTAL;
				free_localizer++;
				continue;
			}
			if (!strcmp("localizer-v", ptline))
			{
				localizers[free_localizer].position=Vector3(pos.x,pos.y,pos.z);
				localizers[free_localizer].rotation=rotation;
				localizers[free_localizer].type=Autopilot::LOCALIZER_VERTICAL;
				free_localizer++;
				continue;
			}
			if (!strcmp("localizer-ndb", ptline))
			{
				localizers[free_localizer].position=Vector3(pos.x,pos.y,pos.z);
				localizers[free_localizer].rotation=rotation;
				localizers[free_localizer].type=Autopilot::LOCALIZER_NDB;
				free_localizer++;
				continue;
			}
			if (!strcmp("localizer-vor", ptline))
			{
				localizers[free_localizer].position=Vector3(pos.x,pos.y,pos.z);
				localizers[free_localizer].rotation=rotation;
				localizers[free_localizer].type=Autopilot::LOCALIZER_VOR;
				free_localizer++;
				continue;
			}
			if (!strcmp("standard", ptline)) {classic_ref=false;tenode->pitch(Degree(90));continue;};
			if (!strncmp("sound", ptline, 5))
			{
#ifdef USE_OPENAL
				if (!SoundScriptManager::getSingleton().isDisabled())
				{
					char tmp[255]="";
					sscanf(ptline, "sound %s", tmp);
					SoundScriptInstance *sound = SoundScriptManager::getSingleton().createInstance(tmp, MAX_TRUCKS+1, tenode);
					sound->setPosition(tenode->getPosition(), Vector3::ZERO);
					sound->start();
				}
#endif //USE_OPENAL
				continue;
			}
			if (!strcmp("beginbox", ptline) || !strcmp("beginmesh", ptline))
			{
				dr = Vector3::ZERO;
				rotating=false;
				virt=false;
				forcecam=false;
				event_filter=EVENT_NONE;
				eventname[0]=0;
				collmesh[0]=0;
				gm = gEnv->collisions->getGroundModelByString("concrete");
				continue;
			};
			if (!strncmp("boxcoords", ptline, 9))
			{
				sscanf(ptline, "boxcoords %f, %f, %f, %f, %f, %f",&l.x,&h.x,&l.y,&h.y,&l.z, &h.z);
				continue;
			}
			if (!strncmp("mesh", ptline, 4))
			{
				sscanf(ptline, "mesh %s",collmesh);
				continue;
			}
			if (!strncmp("rotate", ptline, 6))
			{
				sscanf(ptline, "rotate %f, %f, %f",&sr.x, &sr.y, &sr.z);
				rotating=true;
				continue;
			}
			if (!strncmp("forcecamera", ptline, 11))
			{
				sscanf(ptline, "forcecamera %f, %f, %f",&fc.x, &fc.y, &fc.z);
				forcecam=true;
				continue;
			}
			if (!strncmp("direction", ptline, 9))
			{
				sscanf(ptline, "direction %f, %f, %f",&dr.x, &dr.y, &dr.z);
				continue;
			}
			if (!strncmp("frictionconfig", ptline, 14) && strlen(ptline) > 15)
			{
				// load a custom friction config
				gEnv->collisions->loadGroundModelsConfigFile(String(ptline + 15));
				continue;
			}
			if ((!strncmp("stdfriction", ptline, 11) || !strncmp("usefriction", ptline, 11)) && strlen(ptline) > 12)
			{
				String modelName = String(ptline + 12);
				gm = gEnv->collisions->getGroundModelByString(modelName);
				continue;
			}
			if (!strcmp("virtual", ptline)) {virt=true;continue;};
			if (!strncmp("event", ptline, 5))
			{
				char ts[256];
				ts[0]=0;
				sscanf(ptline, "event %s %s",eventname, ts);
				if (!strncmp(ts, "avatar", 6))
					event_filter=EVENT_AVATAR;
				else if (!strncmp(ts, "truck", 5))
					event_filter=EVENT_TRUCK;
				else if (!strncmp(ts, "airplane", 8))
					event_filter=EVENT_AIRPLANE;
				else if (!strncmp(ts, "boat", 8))
					event_filter=EVENT_BOAT;
				else if (!strncmp(ts, "delete", 8))
					event_filter=EVENT_DELETE;

				//if (!strncmp(ts, "shoptruck", 9))
				//	terrainManager->terrainHasTruckShop=true;

				// fallback
				if (strlen(ts) == 0)
					event_filter=EVENT_ALL;

				continue;
			}
			if (!strcmp("endbox", ptline))
			{
				if (enable_collisions)
				{
					const String eventnameStr = eventname;					
					int boxnum = gEnv->collisions->addCollisionBox(tenode, rotating, virt, pos, rot, l, h, sr, eventnameStr, instancename, forcecam, fc, sc, dr, event_filter, scripthandler);
					obj->collBoxes.push_back((boxnum));
				}
				continue;
			}
			if (!strcmp("endmesh", ptline))
			{
				gEnv->collisions->addCollisionMesh(collmesh, Vector3(pos.x,pos.y,pos.z), tenode->getOrientation(), sc, gm, &(obj->collTris));
				continue;
			}

			if (!strncmp("particleSystem", ptline, 14) && tenode)
			{
				float x=0, y=0, z=0, scale=0;
				char pname[255]="", sname[255]="";
				int res = sscanf(ptline, "particleSystem %f, %f, %f, %f, %s %s", &scale, &x, &y, &z, pname, sname);
				if (res != 6) continue;

				// hacky: prevent duplicates
				String paname = String(pname);
				while(gEnv->sceneManager->hasParticleSystem(paname))
					paname += "_";

				// create particle system
				ParticleSystem* pParticleSys = gEnv->sceneManager->createParticleSystem(paname, String(sname));
				pParticleSys->setCastShadows(false);
				pParticleSys->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap

				// Some affectors may need its instance name (e.g. for script feedback purposes)
#ifdef USE_ANGELSCRIPT
				unsigned short affCount = pParticleSys->getNumAffectors();
				ParticleAffector* pAff;
				for (unsigned short i = 0; i<affCount; ++i)
				{
					pAff = pParticleSys->getAffector(i);
					if (pAff->getType()=="ExtinguishableFire")
					{
						((ExtinguishableFireAffector*)pAff)->setInstanceName(obj->instanceName);
					}
				}
#endif // USE_ANGELSCRIPT

				SceneNode *sn = tenode->createChildSceneNode();
				sn->attachObject(pParticleSys);
				sn->pitch(Degree(90));
				continue;
			}

			if (!strncmp("setMeshMaterial", ptline, 15))
			{
				char mat[256]="";
				sscanf(ptline, "setMeshMaterial %s", mat);
				if (mo->getEntity() && strnlen(mat,250)>0)
				{
					mo->getEntity()->setMaterialName(String(mat));
					// load it
					//MaterialManager::getSingleton().load(String(mat), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				}
				continue;
			}
			if (!strncmp("generateMaterialShaders", ptline, 23))
			{
				char mat[256]="";
				sscanf(ptline, "generateMaterialShaders %s", mat);
				if (BSETTING("Use RTShader System", false))
				{
					Ogre::RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(String(mat), Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
					Ogre::RTShader::ShaderGenerator::getSingleton().invalidateMaterial(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, String(mat));
				}

				continue;
			}
			if (!strncmp("playanimation", ptline, 13) && mo)
			{
				char animname[256]="";
				float speedfactorMin = 0, speedfactorMax = 0;
				sscanf(ptline, "playanimation %f, %f, %s", &speedfactorMin, &speedfactorMax, animname);
				if (tenode && mo->getEntity() && strnlen(animname,250)>0)
				{
					AnimationStateSet *s = mo->getEntity()->getAllAnimationStates();
					if (!s->hasAnimationState(String(animname)))
					{
						LOG("ODEF: animation '" + String(animname) + "' for mesh: '" + String(mesh) + "' in odef file '" + name + ".odef' not found!");
						continue;
					}
					animated_object_t ao;
					ao.node = tenode;
					ao.ent = mo->getEntity();
					ao.speedfactor = speedfactorMin;
					if (speedfactorMin != speedfactorMax)
						ao.speedfactor = Math::RangeRandom(speedfactorMin, speedfactorMax);
					ao.anim = 0;
					try
					{
						ao.anim = mo->getEntity()->getAnimationState(String(animname));
					} catch (...)
					{
						ao.anim = 0;
					}
					if (!ao.anim)
					{
						LOG("ODEF: animation '" + String(animname) + "' for mesh: '" + String(mesh) + "' in odef file '" + name + ".odef' not found!");
						continue;
					}
					ao.anim->setEnabled(true);
					animatedObjects.push_back(ao);
				}
				continue;
			}
			if (!strncmp("drawTextOnMeshTexture", ptline, 21) && mo)
			{
				if (!mo->getEntity())
					continue;
				String matName = mo->getEntity()->getSubEntity(0)->getMaterialName();
				MaterialPtr m = MaterialManager::getSingleton().getByName(matName);
				if (m.getPointer() == 0)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: mesh material not found: "+odefname+" : "+String(ptline));
					continue;
				}
				String texName = m->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName();
				Texture* background = (Texture *)TextureManager::getSingleton().getByName(texName).getPointer();
				if (!background)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: mesh texture not found: "+odefname+" : "+String(ptline));
					continue;
				}

				static int textureNumber = 0;
				textureNumber++;
				char tmpTextName[256]="", tmpMatName[256]="";
				sprintf(tmpTextName, "TextOnTexture_%d_Texture", textureNumber);
				sprintf(tmpMatName, "TextOnTexture_%d_Material", textureNumber);			// Make sure the texture is not WRITE_ONLY, we need to read the buffer to do the blending with the font (get the alpha for example)
				TexturePtr texture = TextureManager::getSingleton().createManual(tmpTextName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, (Ogre::uint)background->getWidth(), (Ogre::uint)background->getHeight(), MIP_UNLIMITED , PF_X8R8G8B8, Ogre::TU_STATIC|Ogre::TU_AUTOMIPMAP, new ResourceBuffer());
				if (texture.getPointer() == 0)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: could not create texture: "+odefname+" : "+String(ptline));
					continue;
				}

				float x=0, y=0, w=0, h=0;
				float a=0, r=0, g=0, b=0;
				char fontname[256]="";
				char text[256]="";
				char option='l';
				int res = sscanf(ptline, "drawTextOnMeshTexture %f, %f, %f, %f, %f, %f, %f, %f, %c, %s %s", &x, &y, &w, &h, &r, &g, &b, &a, &option, fontname, text);
				if (res < 11)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: "+odefname+" : "+String(ptline));
					continue;
				}

				// check if we got a template argument
				if (!strncmp(text, "{{argument1}}", 13))
					strncpy(text, instancename.c_str(), 250);

				// replace '_' with ' '
				char *text_pointer = text;
				while (*text_pointer!=0) {if (*text_pointer=='_') *text_pointer=' ';text_pointer++;};

				Font* font = (Font *)FontManager::getSingleton().getByName(String(fontname)).getPointer();
				if (!font)
				{
					LOG("ODEF: problem with drawTextOnMeshTexture command: font not found: "+odefname+" : "+String(ptline));
					continue;
				}


				//Draw the background to the new texture
				texture->getBuffer()->blit(background->getBuffer());

				x = background->getWidth() * x;
				y = background->getHeight() * y;
				w = background->getWidth() * w;
				h = background->getHeight() * h;

				Image::Box box = Image::Box((size_t)x, (size_t)y, (size_t)(x+w), (size_t)(y+h));
				WriteToTexture(String(text), texture, box, font, ColourValue(r, g, b, a), option);

				// we can save it to disc for debug purposes:
				//SaveImage(texture, "test.png");

				m->clone(tmpMatName);
				MaterialPtr mNew = MaterialManager::getSingleton().getByName(tmpMatName);
				mNew->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(tmpTextName);

				mo->getEntity()->setMaterialName(String(tmpMatName));
				continue;
			}

			LOG("ODEF: unknown command in "+odefname+" : "+String(ptline));
		}

		//add icons if type is set
#ifdef USE_MYGUI
		String typestr = "";
		if (!type.empty() && gEnv->surveyMap)
		{
			typestr = type;
			// hack for raceways
			if (name == "chp-checkpoint")
				typestr = "checkpoint";
			if (name == "chp-start")
				typestr = "racestart";
			if (name == "road", 4)
				typestr = "road";

			if (typestr != "" && typestr != "road" && typestr != "sign")
			{
				SurveyMapEntity *e = gEnv->surveyMap->createMapEntity(typestr);
				if (e)
				{
					e->setVisibility(true);
					e->setPosition(pos);
					e->setRotation(Radian(rot.y));

					if (!name.empty()) e->setDescription(instancename);
				}
			}
		}
#endif //USE_MYGUI
}

bool TerrainObjectManager::updateAnimatedObjects(float dt)
{
	if (animatedObjects.size() == 0) return true;

	std::vector<animated_object_t>::iterator it;

	for (it=animatedObjects.begin(); it!=animatedObjects.end(); it++)
	{
		if (it->anim && it->speedfactor != 0)
		{
			Real time = dt * it->speedfactor;
			it->anim->addTime(time);
		}
	}
	return true;
}

void TerrainObjectManager::loadPreloadedTrucks()
{
	// in netmode, don't load other trucks!
	if (gEnv->network != nullptr)
	{
		return;
	}
	
	for (unsigned int i=0; i<truck_preload.size(); i++)
	{
		Vector3 pos = Vector3(truck_preload[i].px, truck_preload[i].py, truck_preload[i].pz);
		Beam *b = BeamFactory::getSingleton().CreateLocalRigInstance(
			pos, 
			truck_preload[i].rotation, 
			truck_preload[i].name,
            -1,
			nullptr, /* spawnbox */ 
			truck_preload[i].ismachine, 
			nullptr, /* truckconfig */ 
			nullptr, /* skin */
			truck_preload[i].freePosition, 
			true /* preloaded_with_terrain */
		);
#ifdef USE_MYGUI
		if (b && gEnv->surveyMap)
		{
			SurveyMapEntity *e = gEnv->surveyMap->createNamedMapEntity("Truck"+TOSTRING(b->trucknum), SurveyMapManager::getTypeByDriveable(b->driveable));
			if (e)
			{
				e->setState(DESACTIVATED);
				e->setVisibility(true);
				e->setPosition(truck_preload[i].px, truck_preload[i].pz);
				e->setRotation(-Radian(b->getHeadingDirectionAngle()));
			}
		}
#endif //USE_MYGUI
	}
	

}

bool TerrainObjectManager::update( float dt )
{
#ifdef USE_PAGED
	// paged geometry
	for (std::vector<paged_geometry_t>::iterator it=pagedGeometry.begin();it!=pagedGeometry.end();it++)
	{
		if (it->geom) it->geom->update();
	}
#endif //USE_PAGED

	updateAnimatedObjects(dt);
	
	return true;
}

size_t TerrainObjectManager::getMemoryUsage()
{
	// very rough estamation
	return (sizeof(loadedObject_t) * loadedObjects.size());
}

void TerrainObjectManager::freeResources()
{
	// TODO
}
