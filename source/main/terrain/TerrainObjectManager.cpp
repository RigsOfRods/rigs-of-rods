/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "TerrainObjectManager.h"

#include "Application.h"
#include "AutoPilot.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "Collisions.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "MeshObject.h"
#include "ODefFileformat.h"
#include "ProceduralManager.h"
#include "Road2.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "SurveyMapEntity.h"
#include "SurveyMapManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainManager.h"
#include "TObjFileFormat.h"
#include "Utils.h"
#include "WriteTextToTexture.h"

#include <OgreRTShaderSystem.h>
#include <OgreFontManager.h>

#ifdef USE_ANGELSCRIPT
#    include "ExtinguishableFireAffector.h"
#endif // USE_ANGELSCRIPT

using namespace Ogre;
using namespace RoR;

#ifdef USE_PAGED
using namespace Forests;
#endif //USE_PAGED

//workaround for pagedgeometry
inline float getTerrainHeight(Real x, Real z, void* unused = 0)
{
    if (!gEnv->terrainManager->getHeightFinder())
        return 1;
    return gEnv->terrainManager->getHeightFinder()->getHeightAt(x, z);
}

TerrainObjectManager::TerrainObjectManager(TerrainManager* terrainManager) :
    background_loading(BSETTING("Background Loading", false)),
    use_rt_shader_system(BSETTING("Use RTShader System", false)),
    terrainManager(terrainManager)
{
    //prepare for baking
    bakeNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
}

TerrainObjectManager::~TerrainObjectManager()
{
    for (MeshObject* mo : meshObjects)
    {
        if (mo)
            delete mo;
    }
#ifdef USE_PAGED
    for (std::vector<paged_geometry_t>::iterator it = pagedGeometry.begin(); it != pagedGeometry.end(); it++)
    {
        if (it->geom)
        {
            delete(it->geom);
            it->geom = 0;
        }
        if (it->loader)
        {
            delete((TreeLoader2D *)it->loader);
            it->loader = 0;
        }
    }
#endif //USE_PAGED
    if (bakesg != nullptr)
    {
        gEnv->sceneManager->destroyStaticGeometry("bakeSG");
        bakesg = nullptr;
    }
    if (proceduralManager != nullptr)
    {
        delete proceduralManager;
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

void GenerateGridAndPutToScene(Ogre::Vector3 position)
{
    Ogre::ColourValue background_color(Ogre::ColourValue::White);
    Ogre::ColourValue grid_color(0.2f, 0.2f, 0.2f, 1.0f);

    Ogre::ManualObject* mo = new Ogre::ManualObject("ReferenceGrid");

    mo->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST);

    const float step = 1.0f;
    const size_t count = 50;
    unsigned int halfCount = count / 2;
    const float half = (step * count) / 2;
    const float y = 0;
    Ogre::ColourValue c;
    for (size_t i=0; i < count+1; i++)
    {
        if (i == halfCount)
            c = Ogre::ColourValue(1.f, 0.f, 0.f, 1.f);
        else
            c = grid_color;

        mo->position(-half, y, -half+(step*i));
        mo->colour(background_color);
        mo->position(0, y, -half+(step*i));
        mo->colour(c);
        mo->position(0, y, -half+(step*i));
        mo->colour(c);
        mo->position(half, y, -half+(step*i));
        mo->colour(background_color);

        if (i == halfCount)
            c = Ogre::ColourValue(0,0,1,1.0f);
        else
            c = grid_color;

        mo->position(-half+(step*i), y, -half);
        mo->colour(background_color);
        mo->position(-half+(step*i), y, 0);
        mo->colour(c);
        mo->position(-half+(step*i), y, 0);
        mo->colour(c);
        mo->position(-half+(step*i), y, half);
        mo->colour(background_color);
    }

    mo->end();
    mo->setCastShadows(false);

    Ogre::SceneNode *n = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    n->setPosition(position);
    n->attachObject(mo);
    n->setVisible(true);
}

void TerrainObjectManager::loadObjectConfigFile(Ogre::String tobj_name)
{
    std::shared_ptr<TObjFile> tobj;
    try
    {
        DataStreamPtr stream_ptr = ResourceGroupManager::getSingleton().openResource(
            tobj_name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
        TObjParser parser;
        parser.Prepare();
        parser.ProcessOgreStream(stream_ptr.get());
        tobj = parser.Finalize();
    }
    catch (Ogre::Exception& e)
    {
        LOG("[RoR|Terrain] Error reading TObj file: " + tobj_name + "\nMessage" + e.getFullDescription());
        return;
    }
    catch (std::exception& e)
    {
        LOG("[RoR|Terrain] Error reading TObj file: " + tobj_name + "\nMessage" + e.what());
        return;
    }

    if (proceduralManager == nullptr)
    {
        proceduralManager = new ProceduralManager();
    }

    objcounter = 0;
    free_localizer = 0;

    int mapsizex = terrainManager->getGeometryManager()->getMaxTerrainSize().x;
    int mapsizez = terrainManager->getGeometryManager()->getMaxTerrainSize().z;

    // Section 'collision-tris'
    if (tobj->num_collision_triangles != -1)
    {
        // IMPORTANT: This resets the collision objects.
        // The old parser performed tasks in the order of definition on *.tobj file, so some maps could rely on the resetting behavior.
        // This could cause issues -> keep in mind ~ only_a_ptr, 04/2017
        gEnv->collisions->resizeMemory(tobj->num_collision_triangles);
    }

    // Section 'grid'
    if (tobj->grid_enabled)
    {
        GenerateGridAndPutToScene(tobj->grid_position);
    }

    // Section 'trees'
    if (terrainManager->getPagedMode() != 0)
    {
        for (TObjTree tree : tobj->trees)
        {
            this->ProcessTree(
                tree.yaw_from, tree.yaw_to,
                tree.scale_from, tree.scale_to,
                tree.color_map, tree.density_map, tree.tree_mesh, tree.collision_mesh,
                tree.grid_spacing, tree.high_density,
                tree.min_distance, tree.max_distance, mapsizex, mapsizez);
        }
    }

    // Section 'grass' / 'grass2'
    if (terrainManager->getPagedMode() != 0)
    {
        for (TObjGrass grass : tobj->grass)
        {
            this->ProcessGrass(
                grass.sway_speed, grass.sway_length, grass.sway_distrib, grass.density,
                grass.min_x, grass.min_y, grass.min_h,
                grass.max_x, grass.max_y, grass.max_h,
                grass.material_name, grass.color_map_filename, grass.density_map_filename,
                grass.grow_techniq, grass.technique, grass.range, mapsizex, mapsizez);
        }
    }

    // Procedural roads
    for (ProceduralObject po : tobj->proc_objects)
    {
        proceduralManager->addObject(po);
    }

    // Vehicles
    for (TObjVehicle veh : tobj->vehicles)
    {
        if ((veh.type == TObj::SpecialObject::BOAT) && (terrainManager->getWater() == nullptr))
        {
            continue; // Don't spawn boats if there's no water.
        }

        Ogre::String group;
        Ogre::String filename(veh.name);
        if (!RoR::App::GetCacheSystem()->checkResourceLoaded(filename, group))
        {
            LOG(std::string("[RoR|Terrain] Vehicle ") + veh.name + " not found. ignoring.");
            continue;
        }

        truck_prepare_t p;
        p.px           = veh.position.x;
        p.py           = veh.position.y;
        p.pz           = veh.position.z;
        p.freePosition = (veh.type == TObj::SpecialObject::TRUCK2);
        p.ismachine    = (veh.type == TObj::SpecialObject::MACHINE);
        p.rotation     = veh.rotation;
        strcpy(p.name, veh.name);
        truck_preload.push_back(p);
    }

    // Entries
    for (TObjEntry entry : tobj->objects)
    {
        this->loadObject(entry.odef_name, entry.position, entry.rotation, this->bakeNode, entry.instance_name, entry.type);
    }
}

void TerrainObjectManager::ProcessTree(
    float yawfrom, float yawto,
    float scalefrom, float scaleto,
    char* ColorMap, char* DensityMap, char* treemesh, char* treeCollmesh,
    float gridspacing, float highdens,
    int minDist, int maxDist, int mapsizex, int mapsizez)
{
    if (strnlen(ColorMap, 3) == 0)
    {
        LOG("tree ColorMap map zero!");
        return;
    }
    if (strnlen(DensityMap, 3) == 0)
    {
        LOG("tree DensityMap zero!");
        return;
    }
    Forests::DensityMap *densityMap = Forests::DensityMap::load(DensityMap, Forests::CHANNEL_COLOR);
    if (!densityMap)
    {
        LOG("could not load densityMap: "+String(DensityMap));
        return;
    }
    densityMap->setFilter(Forests::MAPFILTER_BILINEAR);
    //densityMap->setMapBounds(TRect(0, 0, mapsizex, mapsizez));

    paged_geometry_t paged;
    paged.geom = new PagedGeometry();
    paged.geom->setTempDir(App::GetSysCacheDir() + PATH_SLASH);
    paged.geom->setCamera(gEnv->mainCamera);
    paged.geom->setPageSize(50);
    paged.geom->setInfinite();
    Ogre::TRect<Ogre::Real> bounds = TBounds(0, 0, mapsizex, mapsizez);
    paged.geom->setBounds(bounds);

    //Set up LODs
    //trees->addDetailLevel<EntityPage>(50);
    float min = minDist * terrainManager->getPagedDetailFactor();
    if (min < 10)
        min = 10;
    paged.geom->addDetailLevel<BatchPage>(min, min / 2);
    float max = maxDist * terrainManager->getPagedDetailFactor();
    if (max < 10)
        max = 10;
    paged.geom->addDetailLevel<ImpostorPage>(max, max / 10);
    TreeLoader2D *treeLoader = new TreeLoader2D(paged.geom, TBounds(0, 0, mapsizex, mapsizez));
    paged.geom->setPageLoader(treeLoader);
    treeLoader->setHeightFunction(&getTerrainHeight);
    if (String(ColorMap) != "none")
    {
        treeLoader->setColorMap(ColorMap);
    }

    Entity* curTree = gEnv->sceneManager->createEntity(String("paged_") + treemesh + TOSTRING(pagedGeometry.size()), treemesh);

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
    }
    else
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

void TerrainObjectManager::ProcessGrass(
        float SwaySpeed, float SwayLength, float SwayDistribution, float Density,
        float minx, float miny, float minH, float maxx, float maxy, float maxH,
        char* grassmat, char* colorMapFilename, char* densityMapFilename,
        int growtechnique, int techn, int range,
        int mapsizex, int mapsizez)
{
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
    } 
    catch(...)
    {
        LOG("error loading grass!");
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
    }
    catch (...)
    {
        LOG("error while baking roads. ignoring.");
    }
}

void removeCollisionTri(int number)
{
    if (gEnv->collisions)
        gEnv->collisions->removeCollisionTri(number);
}

void removeCollisionBox(int number)
{
    if (gEnv->collisions)
        gEnv->collisions->removeCollisionBox(number);
}

void TerrainObjectManager::moveObjectVisuals(const String& instancename, const Ogre::Vector3& pos)
{
    if (loadedObjects.find(instancename) == loadedObjects.end())
    {
        LOG(instancename+ " not found!");
        return;
    }

    loadedObject_t obj = loadedObjects[instancename];

    if (!obj.enabled)
        return;

    obj.sceneNode->setPosition(pos);
}

void TerrainObjectManager::unloadObject(const String& instancename)
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

void TerrainObjectManager::loadObject(const Ogre::String& name, const Ogre::Vector3& pos, const Ogre::Vector3& rot, Ogre::SceneNode* bakeNode, const Ogre::String& instancename, const Ogre::String& type, bool enable_collisions /* = true */, int scripthandler /* = -1 */, bool uniquifyMaterial /* = false */)
{
    if (type == "grid")
    {
        // some fast grid object hacks :)
        for (int x = 0; x < 500; x += 50)
        {
            for (int z = 0; z < 500; z += 50)
            {
                const String notype = "";
                loadObject(name, pos + Vector3(x, 0.0f, z), rot, bakeNode, name, notype, enable_collisions, scripthandler, uniquifyMaterial);
            }
        }
        return;
    }

    if (name.empty())
        return;

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

    if (!RoR::App::GetCacheSystem()->checkResourceLoaded(odefname, odefgroup))
    {
        if (!odefFound)
        {
            LOG("Error while loading Terrain: could not find required .odef file: " + odefname + ". Ignoring entry.");
            return;
        }
    }

    DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(odefname, odefgroup);

    ODefParser parser;
    parser.Prepare();
    parser.ProcessOgreStream(ds.get());
    std::shared_ptr<ODefFile> odef = parser.Finalize();

    SceneNode* tenode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();

    MeshObject* mo = NULL;
    if (odef->header.mesh_name != "none")
    {
        mo = new MeshObject(odef->header.mesh_name, odef->header.entity_name, tenode, NULL, background_loading);
        meshObjects.push_back(mo);
    }

    tenode->setScale(odef->header.scale);
    tenode->setPosition(pos);
    tenode->rotate(rotation);
    tenode->pitch(Degree(-90));
    tenode->setVisible(true);

    // register in map
    loadedObject_t* obj = &loadedObjects[instancename];
    obj->instanceName = instancename;
    obj->enabled = true;
    obj->sceneNode = tenode;
    obj->collTris.clear();

    object_t object;
    object.name = name;
    object.position = pos;
    object.rotation = rot;
    object.initial_position = pos;
    object.initial_rotation = rot;
    object.node = tenode;
    objects.push_back(object);

    if (mo && uniquifyMaterial && !instancename.empty())
    {
        for (unsigned int i = 0; i < mo->getEntity()->getNumSubEntities(); i++)
        {
            SubEntity* se = mo->getEntity()->getSubEntity(i);
            String matname = se->getMaterialName();
            String newmatname = matname + "/" + instancename;
            se->getMaterial()->clone(newmatname);
            se->setMaterialName(newmatname);
        }
    }

    for (ODef::Localizer loc : odef->localizers)
    {
        int type;
        switch (loc)
        {
        case ODef::Localizer::HORIZONTAL:  type = Autopilot::LOCALIZER_HORIZONTAL; break;
        case ODef::Localizer::VERTICAL:    type = Autopilot::LOCALIZER_VERTICAL;   break;
        case ODef::Localizer::NDB:         type = Autopilot::LOCALIZER_NDB;        break;
        case ODef::Localizer::VOR:         type = Autopilot::LOCALIZER_VOR;        break;
        default: continue; // Invalid - skip this
        }
        localizers[free_localizer].position=Vector3(pos.x,pos.y,pos.z);
        localizers[free_localizer].rotation=rotation;
        localizers[free_localizer].type = type;
        free_localizer++;
    }

    if (odef->mode_standard)
    {
        tenode->pitch(Degree(90));
    }

#ifdef USE_OPENAL
    if (!SoundScriptManager::getSingleton().isDisabled())
    {
        for (std::string& snd_name : odef->sounds)
        {
            SoundScriptInstance *sound = SoundScriptManager::getSingleton().createInstance(snd_name, MAX_TRUCKS+1, tenode);
            sound->setPosition(tenode->getPosition(), Vector3::ZERO);
            sound->start();
        }
    }
#endif //USE_OPENAL

    for (std::string& gmodel_file: odef->groundmodel_files)
    {
        gEnv->collisions->loadGroundModelsConfigFile(gmodel_file);
    }

    for (ODefCollisionBox& cbox : odef->collision_boxes)
    {
        int boxnum = gEnv->collisions->addCollisionBox(
            tenode, cbox.is_rotating, cbox.is_virtual, pos, rot,
            cbox.aabb_min, cbox.aabb_max, cbox.box_rot, cbox.event_name,
            instancename, cbox.force_cam_pos, cbox.cam_pos,
            cbox.scale, cbox.direction, cbox.event_filter, scripthandler);

        obj->collBoxes.push_back(boxnum);
    }

    for (ODefCollisionMesh& cmesh : odef->collision_meshes)
    {
        auto gm = gEnv->collisions->getGroundModelByString(cmesh.groundmodel_name);
        gEnv->collisions->addCollisionMesh(
            cmesh.mesh_name, pos, tenode->getOrientation(),
            cmesh.scale, gm, &(obj->collTris));
    }

    for (ODefParticleSys& psys : odef->particle_systems)
    {

        // hacky: prevent duplicates
        String paname = String(psys.instance_name);
        while (gEnv->sceneManager->hasParticleSystem(paname))
            paname += "_";

        // create particle system
        ParticleSystem* pParticleSys = gEnv->sceneManager->createParticleSystem(paname, String(psys.template_name));
        pParticleSys->setCastShadows(false);
        pParticleSys->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap

        // Some affectors may need its instance name (e.g. for script feedback purposes)
#ifdef USE_ANGELSCRIPT
        unsigned short affCount = pParticleSys->getNumAffectors();
        ParticleAffector* pAff;
        for (unsigned short i = 0; i < affCount; ++i)
        {
            pAff = pParticleSys->getAffector(i);
            if (pAff->getType() == "ExtinguishableFire")
            {
                ((ExtinguishableFireAffector*)pAff)->setInstanceName(obj->instanceName);
            }
        }
#endif // USE_ANGELSCRIPT

        SceneNode* sn = tenode->createChildSceneNode();
        sn->attachObject(pParticleSys);
        sn->pitch(Degree(90));
    }

    if (!odef->mat_name.empty())
    {
        if (mo->getEntity())
        {
            mo->getEntity()->setMaterialName(odef->mat_name);
        }
    }

    if (use_rt_shader_system && !odef->mat_name_generate.empty())
    {
        Ogre::RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(odef->mat_name_generate, Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
        Ogre::RTShader::ShaderGenerator::getSingleton().invalidateMaterial(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, String(odef->mat_name_generate));
    }

    for (ODefAnimation& anim : odef->animations)
    {
        if (tenode && mo->getEntity())
        {
            AnimationStateSet *s = mo->getEntity()->getAllAnimationStates();
            String anim_name_str(anim.name);
            if (!s->hasAnimationState(anim_name_str))
            {
                LOG("[ODEF] animation '" + anim_name_str + "' for mesh: '" + odef->header.mesh_name + "' in odef file '" + name + ".odef' not found!");
                //continue;
            }
            animated_object_t ao;
            ao.node = tenode;
            ao.ent = mo->getEntity();
            ao.speedfactor = anim.speed_min;
            if (anim.speed_min != anim.speed_max)
                ao.speedfactor = Math::RangeRandom(anim.speed_min, anim.speed_max);
            ao.anim = 0;
            try
            {
                ao.anim = mo->getEntity()->getAnimationState(anim_name_str);
            } catch (...)
            {
                ao.anim = 0;
            }
            if (!ao.anim)
            {
                LOG("[ODEF] animation '" + anim_name_str + "' for mesh: '" + odef->header.mesh_name + "' in odef file '" + name + ".odef' not found!");
                continue;
            }
            ao.anim->setEnabled(true);
            animatedObjects.push_back(ao);
        }
    }

    for (ODefTexPrint& tex_print : odef->texture_prints)
    {
        if (!mo->getEntity())
            continue;
        String matName = mo->getEntity()->getSubEntity(0)->getMaterialName();
        MaterialPtr m = MaterialManager::getSingleton().getByName(matName);
        if (m.getPointer() == 0)
        {
            LOG("[ODEF] problem with drawTextOnMeshTexture command: mesh material not found: "+odefname+" : "+matName);
            continue;
        }
        String texName = m->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName();
        Texture* background = (Texture *)TextureManager::getSingleton().getByName(texName).getPointer();
        if (!background)
        {
            LOG("[ODEF] problem with drawTextOnMeshTexture command: mesh texture not found: "+odefname+" : "+texName);
            continue;
        }

        static int textureNumber = 0;
        textureNumber++;
        char tmpTextName[256] = "", tmpMatName[256] = "";
        sprintf(tmpTextName, "TextOnTexture_%d_Texture", textureNumber);
        sprintf(tmpMatName, "TextOnTexture_%d_Material", textureNumber); // Make sure the texture is not WRITE_ONLY, we need to read the buffer to do the blending with the font (get the alpha for example)
        TexturePtr texture = TextureManager::getSingleton().createManual(tmpTextName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, (Ogre::uint)background->getWidth(), (Ogre::uint)background->getHeight(), MIP_UNLIMITED, PF_X8R8G8B8, Ogre::TU_STATIC | Ogre::TU_AUTOMIPMAP);
        if (texture.getPointer() == 0)
        {
            LOG("[ODEF] problem with drawTextOnMeshTexture command: could not create texture: "+odefname+" : "+tmpTextName);
            continue;
        }

        char text[ODef::STR_LEN];
        strcpy(text, tex_print.text);

        // check if we got a template argument
        if (!strncmp(text, "{{argument1}}", 13))
            strncpy(text, instancename.c_str(), 250);

        // replace '_' with ' '
        char *text_pointer = text;
        while (*text_pointer!=0) {if (*text_pointer=='_') *text_pointer=' ';text_pointer++;};

        String font_name_str(tex_print.font_name);
        Font* font = (Font *)FontManager::getSingleton().getByName(font_name_str).getPointer();
        if (!font)
        {
            LOG("[ODEF] problem with drawTextOnMeshTexture command: font not found: "+odefname+" : "+font_name_str);
            continue;
        }

        //Draw the background to the new texture
        texture->getBuffer()->blit(background->getBuffer());

        float x = background->getWidth()  * tex_print.x;
        float y = background->getHeight() * tex_print.y;
        float w = background->getWidth()  * tex_print.w;
        float h = background->getHeight() * tex_print.h;

        ColourValue color(tex_print.r, tex_print.g, tex_print.b, tex_print.a);
        Image::Box box = Image::Box((size_t)x, (size_t)y, (size_t)(x+w), (size_t)(y+h));
        WriteToTexture(String(text), texture, box, font, color, tex_print.font_size, tex_print.font_dpi, tex_print.option);

        // we can save it to disc for debug purposes:
        //SaveImage(texture, "test.png");

        m->clone(tmpMatName);
        MaterialPtr mNew = MaterialManager::getSingleton().getByName(tmpMatName);
        mNew->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(tmpTextName);

        mo->getEntity()->setMaterialName(String(tmpMatName));
    }

    for (ODefSpotlight& spotl: odef->spotlights)
    {
        Light* spotLight = gEnv->sceneManager->createLight(spotl.name);

        spotLight->setType(Light::LT_SPOTLIGHT);
        spotLight->setPosition(spotl.pos);
        spotLight->setDirection(spotl.dir);
        spotLight->setAttenuation(spotl.range, 1.0, 0.3, 0.0);
        spotLight->setDiffuseColour(spotl.color);
        spotLight->setSpecularColour(spotl.color);
        spotLight->setSpotlightRange(Degree(spotl.angle_inner), Degree(spotl.angle_outer));

        BillboardSet* lflare = gEnv->sceneManager->createBillboardSet(1);
        lflare->createBillboard(spotl.pos, spotl.color);
        lflare->setMaterialName("tracks/flare");
        lflare->setVisibilityFlags(DEPTHMAP_DISABLED);

        float fsize = Math::Clamp(spotl.range / 10, 0.2f, 2.0f);
        lflare->setDefaultDimensions(fsize, fsize);

        SceneNode *sn = tenode->createChildSceneNode();
        sn->attachObject(spotLight);
        sn->attachObject(lflare);
    }

    for (ODefPointLight& plight : odef->point_lights)
    {
        Light* pointlight = gEnv->sceneManager->createLight(plight.name);

        pointlight->setType(Light::LT_POINT);
        pointlight->setPosition(plight.pos);
        pointlight->setDirection(plight.dir);
        pointlight->setAttenuation(plight.range, 1.0, 0.3, 0.0);
        pointlight->setDiffuseColour(plight.color);
        pointlight->setSpecularColour(plight.color);

        BillboardSet* lflare = gEnv->sceneManager->createBillboardSet(1);
        lflare->createBillboard(plight.pos, plight.color);
        lflare->setMaterialName("tracks/flare");
        lflare->setVisibilityFlags(DEPTHMAP_DISABLED);

        float fsize = Math::Clamp(plight.range / 10, 0.2f, 2.0f);
        lflare->setDefaultDimensions(fsize, fsize);

        SceneNode *sn = tenode->createChildSceneNode();
        sn->attachObject(pointlight);
        sn->attachObject(lflare);
    }

    //add icons if type is set
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
            SurveyMapEntity* e = gEnv->surveyMap->createMapEntity(typestr);
            if (e)
            {
                e->setVisibility(true);
                e->setPosition(pos);
                e->setRotation(Radian(rot.y));

                if (!name.empty())
                    e->setDescription(instancename);
            }
        }
    }

}

bool TerrainObjectManager::updateAnimatedObjects(float dt)
{
    if (animatedObjects.size() == 0)
        return true;

    std::vector<animated_object_t>::iterator it;

    for (it = animatedObjects.begin(); it != animatedObjects.end(); it++)
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
    if (RoR::App::GetActiveMpState() == RoR::App::MP_STATE_CONNECTED)
    {
        return;
    }

    for (unsigned int i = 0; i < truck_preload.size(); i++)
    {
        Vector3 pos = Vector3(truck_preload[i].px, truck_preload[i].py, truck_preload[i].pz);
        Beam* b = BeamFactory::getSingleton().CreateLocalRigInstance(
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

        if (b && gEnv->surveyMap)
        {
            SurveyMapEntity* e = gEnv->surveyMap->createNamedMapEntity("Truck" + TOSTRING(b->trucknum), SurveyMapManager::getTypeByDriveable(b->driveable));
            if (e)
            {
                e->setState(SIMULATED);
                e->setVisibility(true);
                e->setPosition(truck_preload[i].px, truck_preload[i].pz);
                e->setRotation(-Radian(b->getHeadingDirectionAngle()));
            }
        }

    }
}

bool TerrainObjectManager::update(float dt)
{
#ifdef USE_PAGED
    // paged geometry
    for (auto it : pagedGeometry)
    {
        if (it.geom)
            it.geom->update();
    }
#endif //USE_PAGED

    this->updateAnimatedObjects(dt);

    return true;
}
