/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "Actor.h"
#include "Application.h"
#include "AutoPilot.h"
#include "CacheSystem.h"
#include "Collisions.h"
#include "Console.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "MeshObject.h"
#include "ODefFileFormat.h"
#include "PlatformUtils.h"
#include "ProceduralRoad.h"
#include "ScriptEngine.h"
#include "SoundScriptManager.h"
#include "TerrainGeometryManager.h"
#include "Terrain.h"
#include "TObjFileFormat.h"
#include "Utils.h"
#include "WriteTextToTexture.h"

#include <RTShaderSystem/OgreRTShaderSystem.h>
#include <Overlay/OgreFontManager.h>

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
    return App::GetGameContext()->GetTerrain()->GetHeightAt(x, z);
}

TerrainObjectManager::TerrainObjectManager(Terrain* terrainManager) :
    terrainManager(terrainManager)
{
    m_terrn2_grouping_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode(fmt::format("Terrain: {}", terrainManager->GetDef().name));

    m_procedural_manager = new ProceduralManager(m_terrn2_grouping_node->createChildSceneNode("Procedural Roads"));
}

TerrainObjectManager::~TerrainObjectManager()
{
    for (MeshObject* mo : m_mesh_objects)
    {
        if (mo)
            delete mo;
    }
#ifdef USE_PAGED
    for (auto geom : m_paged_geometry)
    {
        delete geom->getPageLoader();
        delete geom;
    }
#endif //USE_PAGED

    App::GetGfxScene()->GetSceneManager()->destroyAllEntities();

    App::GetGfxScene()->GetSceneManager()->destroySceneNode(m_terrn2_grouping_node);
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
    Ogre::SceneNode *n = App::GetGameContext()->GetTerrain()->getObjectManager()->getGroupingSceneNode()->createChildSceneNode();
    n->setPosition(position);
    n->attachObject(mo);
    n->setVisible(true);
}

void TerrainObjectManager::LoadTObjFile(Ogre::String tobj_name)
{
    ROR_ASSERT(this->terrainManager);
    ROR_ASSERT(this->terrainManager->getCacheEntry());
    ROR_ASSERT(this->terrainManager->getCacheEntry()->resource_group != "");

    std::shared_ptr<TObjFile> tobj;
    try
    {
        DataStreamPtr stream_ptr = ResourceGroupManager::getSingleton().openResource(
            tobj_name, this->terrainManager->getCacheEntry()->resource_group);
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

    ROR_ASSERT(m_terrn2_grouping_node);
    m_tobj_grouping_node = m_terrn2_grouping_node->createChildSceneNode(tobj_name);

    int mapsizex = terrainManager->getGeometryManager()->getMaxTerrainSize().x;
    int mapsizez = terrainManager->getGeometryManager()->getMaxTerrainSize().z;

    // Section 'grid'
    if (tobj->grid_enabled)
    {
        GenerateGridAndPutToScene(tobj->grid_position);
    }

    // Section 'trees'
    if (App::gfx_vegetation_mode->getEnum<GfxVegetation>() != GfxVegetation::NONE)
    {
        for (TObjTree tree : tobj->trees)
        {
            try
            {
                this->ProcessTree(
                    tree.yaw_from, tree.yaw_to,
                    tree.scale_from, tree.scale_to,
                    tree.color_map, tree.density_map, tree.tree_mesh, tree.collision_mesh,
                    tree.grid_spacing, tree.high_density,
                    tree.min_distance, tree.max_distance, mapsizex, mapsizez);
            }
            catch (...)
            {
                RoR::HandleGenericException(fmt::format("Error processing 'trees' line (mesh: {}) from TOBJ file {}", tree.tree_mesh, tobj_name));
            }
        }
    }

    // Section 'grass' / 'grass2'
    if (App::gfx_vegetation_mode->getEnum<GfxVegetation>() != GfxVegetation::NONE)
    {
        for (TObjGrass grass : tobj->grass)
        {
            try
            {
                this->ProcessGrass(
                    grass.sway_speed, grass.sway_length, grass.sway_distrib, grass.density,
                    grass.min_x, grass.min_y, grass.min_h,
                    grass.max_x, grass.max_y, grass.max_h,
                    grass.material_name, grass.color_map_filename, grass.density_map_filename,
                    grass.grow_techniq, grass.technique, grass.range, mapsizex, mapsizez);
            }
            catch (...)
            {
                RoR::HandleGenericException(fmt::format("Error processing 'grass' line (material: {}) from TOBJ file {}", grass.material_name, tobj_name));
            }
        }
    }

    // Procedural roads
    for (ProceduralObjectPtr& po : tobj->proc_objects)
    {
        try
        {
            m_procedural_manager->addObject(po);
        }
        catch (...)
        {
            RoR::HandleGenericException(fmt::format("Error processing procedural road {} from TOBJ file {}", po->name, tobj_name));
        }
    }

    // Vehicles
    for (TObjVehicle const& veh : tobj->vehicles)
    {
        if ((veh.type == TObj::SpecialObject::BOAT) && (terrainManager->getWater() == nullptr))
        {
            continue; // Don't spawn boats if there's no water.
        }

        // NOTE: The filename may be in "Bundle-qualified" format, i.e. "mybundle.zip:myactor.truck"

        PredefinedActor p;
        p.px           = veh.position.x;
        p.py           = veh.position.y;
        p.pz           = veh.position.z;
        p.freePosition = (veh.type == TObj::SpecialObject::TRUCK2);
        p.ismachine    = (veh.type == TObj::SpecialObject::MACHINE);
        p.rotation     = veh.rotation;
        p.name         = veh.name;
        m_predefined_actors.push_back(p);
    }

    // Entries
    for (TObjEntry entry : tobj->objects)
    {
        try
        {
            this->LoadTerrainObject(entry.odef_name, entry.position, entry.rotation, entry.instance_name, entry.type, entry.rendering_distance);
        }
        catch (...)
        {
            RoR::HandleGenericException(fmt::format("Error processing object line (ODEF: {}) from TOBJ file {}", entry.odef_name, tobj_name));
        }
    }

    if (App::diag_terrn_log_roads->getBool())
    {
        m_procedural_manager->logDiagnostics();
    }

    m_tobj_grouping_node = nullptr;
}

void TerrainObjectManager::ProcessTree(
    float yawfrom, float yawto,
    float scalefrom, float scaleto,
    char* ColorMap, char* DensityMap, char* treemesh, char* treeCollmesh,
    float gridspacing, float highdens,
    int minDist, int maxDist, int mapsizex, int mapsizez)
{
#ifdef USE_PAGED
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

    PagedGeometry* geom = new PagedGeometry();
    geom->setTempDir(App::sys_cache_dir->getStr() + PATH_SLASH);
    geom->setCamera(App::GetCameraManager()->GetCamera());
    geom->setPageSize(50);
    geom->setInfinite();
    Ogre::TRect<Ogre::Real> bounds = TBounds(0, 0, mapsizex, mapsizez);
    geom->setBounds(bounds);

    //Set up LODs
    //trees->addDetailLevel<EntityPage>(50);
    float min = minDist * terrainManager->getPagedDetailFactor();
    if (min < 10)
        min = 10;
    geom->addDetailLevel<BatchPage>(min, min / 2);
    float max = maxDist * terrainManager->getPagedDetailFactor();
    if (max < 10)
        max = 10;

    // Check if farther details level is greater than closer
    if (max / 10 > min / 2)
    {
        geom->addDetailLevel<ImpostorPage>(max, max / 10);
    }

    TreeLoader2D *treeLoader = new TreeLoader2D(geom, TBounds(0, 0, mapsizex, mapsizez));
    treeLoader->setMinimumScale(scalefrom);
    treeLoader->setMaximumScale(scaleto);
    geom->setPageLoader(treeLoader);
    treeLoader->setHeightFunction(&getTerrainHeight);
    if (String(ColorMap) != "none")
    {
        treeLoader->setColorMap(ColorMap);
    }

    Entity* curTree = App::GetGfxScene()->GetSceneManager()->createEntity(String("paged_") + treemesh + TOSTRING(m_paged_geometry.size()), treemesh);

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
                    pos.y = terrainManager->GetHeightAt(pos.x, pos.z);
                    scale *= 0.1f;
                    terrainManager->GetCollisions()->addCollisionMesh(curTree->getName(), String(treeCollmesh), pos, Quaternion(Degree(yaw), Vector3::UNIT_Y), Vector3(scale, scale, scale));
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
                        pos.y = terrainManager->GetHeightAt(pos.x, pos.z);
                        terrainManager->GetCollisions()->addCollisionMesh(treemesh, String(treeCollmesh),pos, Quaternion(Degree(yaw), Vector3::UNIT_Y), Vector3(scale, scale, scale));
                    }
                }
            }
        }
    }
    m_paged_geometry.push_back(geom);
#endif //USE_PAGED
}

void TerrainObjectManager::ProcessGrass(
        float SwaySpeed, float SwayLength, float SwayDistribution, float Density,
        float minx, float miny, float minH, float maxx, float maxy, float maxH,
        char* grassmat, char* colorMapFilename, char* densityMapFilename,
        int growtechnique, int techn, int range,
        int mapsizex, int mapsizez)
{
#ifdef USE_PAGED
    //Initialize the PagedGeometry engine
    try
    {
        PagedGeometry *grass = new PagedGeometry(App::GetCameraManager()->GetCamera(), 30);
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

        m_paged_geometry.push_back(grass);
    } 
    catch(...)
    {
        LOG("error loading grass!");
    }
#endif //USE_PAGED
}

void TerrainObjectManager::MoveObjectVisuals(const String& instancename, const Ogre::Vector3& pos)
{
    if (m_static_objects.find(instancename) == m_static_objects.end())
    {
        LOG(instancename+ " not found!");
        return;
    }

    StaticObject obj = m_static_objects[instancename];

    if (!obj.enabled)
        return;

    obj.sceneNode->setPosition(pos);
}

void TerrainObjectManager::unloadObject(const String& instancename)
{
    if (m_static_objects.find(instancename) == m_static_objects.end())
    {
        LOG("unable to unload object: " + instancename);
        return;
    }

    StaticObject obj = m_static_objects[instancename];

    if (!obj.enabled)
        return;

    for (auto tri : obj.collTris)
    {
        terrainManager->GetCollisions()->removeCollisionTri(tri);
    }
    for (auto box : obj.collBoxes)
    {
        terrainManager->GetCollisions()->removeCollisionBox(box);
    }

    obj.sceneNode->detachAllObjects();
    obj.sceneNode->setVisible(false);
    obj.enabled = false;

    m_editor_objects.erase(std::remove_if(m_editor_objects.begin(), m_editor_objects.end(),
                [instancename](EditorObject& e) { return e.instance_name == instancename; }), m_editor_objects.end());
}

ODefFile* TerrainObjectManager::FetchODef(std::string const & odef_name)
{
    // Consult cache first
    auto search_res = m_odef_cache.find(odef_name);
    if (search_res != m_odef_cache.end())
    {
        return search_res->second.get();
    }

    // Search for the file
    const std::string filename = odef_name + ".odef";
    std::string group_name;
    try
    {
        group_name = Ogre::ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
    }
    catch (...) // This means "not found"
    {
        LOG(fmt::format("[ODEF] Could not find {} in any resource group", filename));
        return nullptr;
    }

    try
    {
        // Load and parse the file
        Ogre::DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(filename, group_name);
        ODefParser parser;
        parser.Prepare();
        parser.ProcessOgreStream(ds.get());
        std::shared_ptr<ODefFile> odef = parser.Finalize();

        // Add to cache and return
        m_odef_cache.insert(std::make_pair(odef_name, odef));
        return odef.get();
    }
    catch (...)
    {
        LOG(fmt::format("[ODEF] An exception occurred when loading or parsing {}", filename));
        return nullptr;
    }
}

bool TerrainObjectManager::LoadTerrainObject(const Ogre::String& name, const Ogre::Vector3& pos, const Ogre::Vector3& rot, const Ogre::String& instancename, const Ogre::String& type, float rendering_distance /* = 0 */, bool enable_collisions /* = true */, int scripthandler /* = -1 */, bool uniquifyMaterial /* = false */)
{
    if (type == "grid")
    {
        // some fast grid object hacks :)
        for (int x = 0; x < 500; x += 50)
        {
            for (int z = 0; z < 500; z += 50)
            {
                const String notype = "";
                LoadTerrainObject(name, pos + Vector3(x, 0.0f, z), rot, name, notype, /*rendering_distance:*/0, enable_collisions, scripthandler, uniquifyMaterial);
            }
        }
        return true;
    }

    const std::string odefname = name + ".odef"; // for logging
    ODefFile* odef = this->FetchODef(name);
    if (odef == nullptr)
    {
        // Only log to console if requested from Console UI or script (debug message to RoR.log is written anyway).
        if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN, Console::CONSOLE_SYSTEM_ERROR,
                fmt::format(_L("Could not load file '{}'"), odefname));
        }
        return false;
    }

    SceneNode* tenode = this->getGroupingSceneNode()->createChildSceneNode();

    MeshObject* mo = nullptr;
    if (odef->header.mesh_name != "none")
    {
        Str<100> ebuf; ebuf << m_entity_counter++ << "-" << odef->header.mesh_name;
        mo = new MeshObject(odef->header.mesh_name, terrainManager->getTerrainFileResourceGroup(), ebuf.ToCStr(), tenode);
        if (mo->getEntity())
        {
            mo->getEntity()->setCastShadows(odef->header.cast_shadows);
            mo->getEntity()->setRenderingDistance(rendering_distance);
            m_mesh_objects.push_back(mo);
        }
        else
        {
            delete mo;
            // Only log to console if requested from Console UI or script (debug message to RoR.log is written anyway).
            if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
            {
                App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN, Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format(_L("Could not load mesh '{}' (used by object '{}')"), odef->header.mesh_name, odefname));
            }
        }
    }

    tenode->setScale(odef->header.scale);
    tenode->setPosition(pos);
    Quaternion rotation = Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z);
    tenode->rotate(rotation);
    tenode->pitch(Degree(-90));
    tenode->setVisible(true);

    // register in map
    StaticObject* obj = &m_static_objects[instancename];
    obj->instanceName = instancename;
    obj->enabled = true;
    obj->sceneNode = tenode;
    obj->collTris.clear();

    EditorObject object;
    object.name = name;
    object.instance_name = instancename;
    object.type = type;
    object.position = pos;
    object.rotation = rot;
    object.initial_position = pos;
    object.initial_rotation = rot;
    object.node = tenode;
    object.enable_collisions = enable_collisions;
    object.script_handler = scripthandler;
    m_editor_objects.push_back(object);

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

    for (LocalizerType type : odef->localizers)
    {
        localizer_t loc;
        loc.position = Vector3(pos.x, pos.y, pos.z);
        loc.rotation = rotation;
        loc.type = type;
        localizers.push_back(loc);
    }

    if (odef->mode_standard)
    {
        tenode->pitch(Degree(90));
    }

#ifdef USE_OPENAL
    if (!App::GetSoundScriptManager()->isDisabled())
    {
        for (std::string& snd_name : odef->sounds)
        {
            SoundScriptInstancePtr sound = App::GetSoundScriptManager()->createInstance(snd_name, SoundScriptInstance::ACTOR_ID_TERRAIN_OBJECT);
            sound->setPosition(tenode->getPosition());
            sound->start();
        }
    }
#endif //USE_OPENAL

    for (std::string& gmodel_file: odef->groundmodel_files)
    {
        terrainManager->GetCollisions()->loadGroundModelsConfigFile(gmodel_file);
    }

    bool race_event = !object.instance_name.compare(0, 10, "checkpoint") ||
                        !object.instance_name.compare(0,  4, "race");

    if (race_event)
    {
        String type = "checkpoint";
        auto res = StringUtil::split(object.instance_name, "|");
        if ((res.size() == 4 && res[2] == "0") || !object.instance_name.compare(0, 4, "race"))
        {
            type = "racestart";
        }
        int race_id = res.size() > 1 ? StringConverter::parseInt(res[1], -1) : -1;
        m_map_entities.push_back(SurveyMapEntity(type, /*caption:*/type, fmt::format("icon_{}.dds", type), /*resource_group:*/"", object.position, Ogre::Radian(0), race_id));
    }
    else if (!object.type.empty())
    {
        String caption = "";
        if (object.type == "station" || object.type == "hotel" || object.type == "village" ||
                object.type == "observatory" || object.type == "farm" || object.type == "ship" || object.type == "sign")
        {
            caption = object.instance_name + " " + object.type;
        }
        m_map_entities.push_back(SurveyMapEntity(object.type, caption, fmt::format("icon_{}.dds", object.type), /*resource_group:*/"", object.position, Ogre::Radian(0), -1));
    }

    this->ProcessODefCollisionBoxes(obj, odef, object, race_event);

    for (ODefCollisionMesh& cmesh : odef->collision_meshes)
    {
        if (cmesh.mesh_name == "")
        {
            LOG("[ODEF] Skipping collision mesh with empty name. Object: " + odefname);
            continue;
        }

        auto gm = terrainManager->GetCollisions()->getGroundModelByString(cmesh.groundmodel_name);
        terrainManager->GetCollisions()->addCollisionMesh(
            odefname,
            cmesh.mesh_name, pos, tenode->getOrientation(),
            cmesh.scale, gm, &(obj->collTris));
    }

    for (ODefParticleSys& psys : odef->particle_systems)
    {

        // hacky: prevent duplicates
        String paname = String(psys.instance_name);
        while (App::GetGfxScene()->GetSceneManager()->hasParticleSystem(paname))
            paname += "_";

        // create particle system
        ParticleSystem* pParticleSys = App::GetGfxScene()->GetSceneManager()->createParticleSystem(paname, String(psys.template_name));
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

    if (odef->mat_name_generate != "")
    {
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(odef->mat_name_generate,"generatedMaterialShaders");
        Ogre::RTShader::ShaderGenerator::getSingleton().createShaderBasedTechnique(*mat, Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
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
            AnimatedObject ao;
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
            m_animated_objects.push_back(ao);
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

        Str<200> text_buf; text_buf << tex_print.text;

        // check if we got a template argument
        if (!strncmp(text_buf.GetBuffer(), "{{argument1}}", 13))
        {
            text_buf.Clear();
            text_buf << instancename;
        }

        // replace '_' with ' '
        char *text_pointer = text_buf.GetBuffer();
        while (*text_pointer!=0) {if (*text_pointer=='_') *text_pointer=' ';text_pointer++;};

        String font_name_str(tex_print.font_name);
        Ogre::Font* font = (Ogre::Font *)FontManager::getSingleton().getByName(font_name_str).getPointer();
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
        Ogre::Box box = Ogre::Box((size_t)x, (size_t)y, (size_t)(x+w), (size_t)(y+h));
        WriteToTexture(text_buf.ToCStr(), texture, box, font, color, tex_print.font_size, tex_print.font_dpi, tex_print.option);

        // we can save it to disc for debug purposes:
        //SaveImage(texture, "test.png");

        m->clone(tmpMatName);
        MaterialPtr mNew = MaterialManager::getSingleton().getByName(tmpMatName);
        mNew->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(tmpTextName);

        mo->getEntity()->setMaterialName(String(tmpMatName));
    }

    for (ODefSpotlight& spotl: odef->spotlights)
    {
        Light* spotLight = App::GetGfxScene()->GetSceneManager()->createLight();

        spotLight->setType(Light::LT_SPOTLIGHT);
        spotLight->setPosition(spotl.pos);
        spotLight->setDirection(spotl.dir);
        spotLight->setAttenuation(spotl.range, 1.0, 0.3, 0.0);
        spotLight->setDiffuseColour(spotl.color);
        spotLight->setSpecularColour(spotl.color);
        spotLight->setSpotlightRange(Degree(spotl.angle_inner), Degree(spotl.angle_outer));

        BillboardSet* lflare = App::GetGfxScene()->GetSceneManager()->createBillboardSet(1);
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
        Light* pointlight = App::GetGfxScene()->GetSceneManager()->createLight();

        pointlight->setType(Light::LT_POINT);
        pointlight->setPosition(plight.pos);
        pointlight->setDirection(plight.dir);
        pointlight->setAttenuation(plight.range, 1.0, 0.3, 0.0);
        pointlight->setDiffuseColour(plight.color);
        pointlight->setSpecularColour(plight.color);

        BillboardSet* lflare = App::GetGfxScene()->GetSceneManager()->createBillboardSet(1);
        lflare->createBillboard(plight.pos, plight.color);
        lflare->setMaterialName("tracks/flare");
        lflare->setVisibilityFlags(DEPTHMAP_DISABLED);

        float fsize = Math::Clamp(plight.range / 10, 0.2f, 2.0f);
        lflare->setDefaultDimensions(fsize, fsize);

        SceneNode *sn = tenode->createChildSceneNode();
        sn->attachObject(pointlight);
        sn->attachObject(lflare);
    }

    return true;
}

bool TerrainObjectManager::LoadTerrainScript(const Ogre::String& filename)
{
    ROR_ASSERT(!m_angelscript_grouping_node);

    m_angelscript_grouping_node = m_terrn2_grouping_node->createChildSceneNode(filename);
    ScriptUnitId_t result = App::GetScriptEngine()->loadScript(filename);
    m_angelscript_grouping_node = nullptr;
    
    return result != SCRIPTUNITID_INVALID;
}

bool TerrainObjectManager::UpdateAnimatedObjects(float dt)
{
    if (m_animated_objects.size() == 0)
        return true;

    std::vector<AnimatedObject>::iterator it;

    for (it = m_animated_objects.begin(); it != m_animated_objects.end(); it++)
    {
        if (it->anim && it->speedfactor != 0)
        {
            Real time = dt * it->speedfactor;
            it->anim->addTime(time);
        }
    }
    return true;
}

void TerrainObjectManager::LoadTelepoints()
{
    for (Terrn2Telepoint& telepoint: terrainManager->GetDef().telepoints)
    {
        m_map_entities.push_back(SurveyMapEntity("telepoint", telepoint.name, "icon_telepoint.dds", /*resource_group:*/"", telepoint.position, Ogre::Radian(0), -1));
    }
}

void TerrainObjectManager::LoadPredefinedActors()
{
    // in netmode, don't load other actors!
    if (RoR::App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        return;
    }

    for (unsigned int i = 0; i < m_predefined_actors.size(); i++)
    {
        ActorSpawnRequest* rq = new ActorSpawnRequest;
        rq->asr_position      = Vector3(m_predefined_actors[i].px, m_predefined_actors[i].py, m_predefined_actors[i].pz);
        rq->asr_filename      = m_predefined_actors[i].name;
        rq->asr_rotation      = m_predefined_actors[i].rotation;
        rq->asr_origin        = ActorSpawnRequest::Origin::TERRN_DEF;
        rq->asr_free_position = m_predefined_actors[i].freePosition;
        rq->asr_terrn_machine = m_predefined_actors[i].ismachine;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));
    }
}

bool TerrainObjectManager::UpdateTerrainObjects(float dt)
{
#ifdef USE_PAGED
    for (auto geom : m_paged_geometry)
    {
        geom->update();
    }
#endif //USE_PAGED
    this->UpdateAnimatedObjects(dt);

    return true;
}

void TerrainObjectManager::ProcessODefCollisionBoxes(StaticObject* obj, ODefFile* odef, const EditorObject& params, bool race_event)
{
    for (ODefCollisionBox& cbox : odef->collision_boxes)
    {
        if (params.enable_collisions && (App::sim_races_enabled->getBool() || !race_event))
        {
            // Validate AABB (minimum corners must be less or equal to maximum corners)
            if (cbox.aabb_min.x > cbox.aabb_max.x || cbox.aabb_min.y > cbox.aabb_max.y || cbox.aabb_min.z > cbox.aabb_max.z)
            {
                // Only log to console if invoked from Console UI or script.
                std::string msg = "Skipping invalid collision box, min: " + TOSTRING(cbox.aabb_min) + ", max: " + TOSTRING(cbox.aabb_max);
                if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
                {
                    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN, Console::CONSOLE_SYSTEM_WARNING, msg);
                }
                else
                {
                    LOG(fmt::format("[ODEF] {}", msg));
                }
                continue;
            }

            int boxnum = terrainManager->GetCollisions()->addCollisionBox(
                cbox.is_rotating, cbox.is_virtual, params.position, params.rotation,
                cbox.aabb_min, cbox.aabb_max, cbox.box_rot, cbox.event_name,
                params.instance_name, cbox.force_cam_pos, cbox.cam_pos,
                cbox.scale, cbox.direction, cbox.event_filter, params.script_handler);

            obj->collBoxes.push_back(boxnum);
        }
    }
}

Ogre::SceneNode* TerrainObjectManager::getGroupingSceneNode()
{
    // This has no effect on rendering, it just helps users to diagnose the scene graph.
    // --------------------------------------------------------------------------------

    if (m_angelscript_grouping_node)
        return m_angelscript_grouping_node;
    else if (m_tobj_grouping_node)
        return m_tobj_grouping_node;
    else if (m_terrn2_grouping_node)
        return m_terrn2_grouping_node;
    else
        return App::GetGfxScene()->GetSceneManager()->getRootSceneNode();
}
