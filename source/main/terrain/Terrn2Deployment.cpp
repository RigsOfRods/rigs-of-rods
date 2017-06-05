/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#include "Terrn2Deployment.h"

#include "RoRPrerequisites.h"
#include "ConfigFile.h"

#include "PagedGeometry.h"
#include "TreeLoader3D.h"

#define LOGSTREAM Ogre::LogManager::getSingleton().stream() << "[RoR|DeployTerrn2] "

bool RoR::Terrn2Deployer::DeployTerrn2(std::string const & terrn2_filename)
{
    auto& resource_group_manager = Ogre::ResourceGroupManager::getSingleton();

    // PROCESS .TERRRN2
    try
    {
        // create the Stream
        const std::string group_name = resource_group_manager.findGroupContainingResource(terrn2_filename);
        Ogre::DataStreamPtr stream = resource_group_manager.openResource(terrn2_filename, group_name);

        // Parse the file
        Terrn2Parser parser;
        const bool loaded_ok = parser.LoadTerrn2(m_terrn2, stream);

        // Report messages
        if (parser.GetMessages().size() > 0)
        {
            LOGSTREAM << "Terrn2 parser messages:";
            for (std::string const& msg : parser.GetMessages())
                LOGSTREAM << '\t' << msg;
        }

        // Error?
        if (!loaded_ok)
            return false;
    }
    catch (...)
    {
        this->HandleException("processing .terrn2 file");
        return false;
    }

    // PROCESS .OTC (ogre terrain conf)
    try
    {
        if (! this->CheckAndLoadOTC())
        {
            return false; // Error already logged
        }
    }
    catch (...)
    {
        this->HandleException("processing .otc file(s)");
    }

    // PROCESS 'LANDUSE.CFG'
    try
    {
        this->LoadLanduseConfig();
    }
    catch (...)
    {
        this->HandleException("processing 'landuse.cfg' file");
        return false;
    }

    // PROCESS .TOBJ (terrain objects)
    try
    {
        this->LoadTobjFiles();
    }
    catch (...)
    {
        this->HandleException("processing .tobj file(s)");
    }

    // COMPOSE JSON
    m_json = Json::objectValue;
    this->ProcessTerrn2Json();
    this->ProcessOtcJson();
    this->ProcessLanduseJson();
    this->ProcessTobjJson();

    return true;
}

Json::Value RoR::Terrn2Deployer::ColorToJson(Ogre::ColourValue color)
{
    Json::Value j_color;
    j_color["r"] = color.r;
    j_color["g"] = color.g;
    j_color["b"] = color.b;
    j_color["a"] = color.a;
    return j_color;
}

Json::Value RoR::Terrn2Deployer::Vector3ToJson(Ogre::Vector3 pos)
{
    Json::Value j_pos;
    j_pos["x"] = pos.x;
    j_pos["y"] = pos.y;
    j_pos["z"] = pos.z;
    return j_pos;
}

Json::Value RoR::Terrn2Deployer::QuaternionToJson(Ogre::Quaternion& quat)
{
    Json::Value j_quat;
    j_quat["w"] = quat.w;
    j_quat["x"] = quat.x;
    j_quat["y"] = quat.y;
    j_quat["z"] = quat.z;
    return j_quat;
}

Json::Value RoR::Terrn2Deployer::StringOrNull(std::string const & str)
{
    if (str.empty())
        return Json::nullValue;
    else
        return str;
}

void RoR::Terrn2Deployer::ProcessTerrn2Json()
{
    m_json["terrn2"] = Json::objectValue;
    Json::Value& j_terrn2 = m_json["terrn2"];

    j_terrn2["ambient_color"]        = this->ColorToJson(m_terrn2.ambient_color);
    j_terrn2["start_position"]       = this->Vector3ToJson(m_terrn2.start_position);
    j_terrn2["category_id"]          = m_terrn2.category_id;//Int
    j_terrn2["guid"]                 = this->StringOrNull(m_terrn2.guid);
    j_terrn2["version"]              = m_terrn2.version;//Int
    j_terrn2["gravity"]              = m_terrn2.gravity;//Float
    j_terrn2["caelum_config"]        = this->StringOrNull(m_terrn2.caelum_config);
    j_terrn2["cubemap_config"]       = this->StringOrNull(m_terrn2.cubemap_config);
    j_terrn2["caelum_fog_start"]     = m_terrn2.caelum_fog_start;//Int
    j_terrn2["caelum_fog_end"]       = m_terrn2.caelum_fog_end;//Int
    j_terrn2["has_water"]            = m_terrn2.has_water;//Bool
    j_terrn2["hydrax_conf_file"]     = this->StringOrNull(m_terrn2.hydrax_conf_file);
    j_terrn2["traction_map_file"]    = this->StringOrNull(m_terrn2.traction_map_file);
    j_terrn2["water_height"]         = m_terrn2.water_height;//Float
    j_terrn2["water_bottom_height"]  = m_terrn2.water_bottom_height;//Float
    j_terrn2["has_water"]            = m_terrn2.has_water;//Bool

    j_terrn2["tobj_files"] = Json::arrayValue;
    for (std::string const & tobj_filename : m_terrn2.tobj_files)
    {
        j_terrn2["tobj_files"].append(Json::Value(tobj_filename));
    }

    j_terrn2["authors"] = Json::arrayValue;
    for (auto const & t2_author : m_terrn2.authors)
    {
        Json::Value j_author = Json::objectValue;
        j_author["name"] = t2_author.name;
        j_author["type"] = t2_author.type;
        j_terrn2["authors"].append(j_author);
    }

    j_terrn2["as_scripts"] = Json::arrayValue;
    for (std::string const & as_filename : m_terrn2.as_files)
    {
        j_terrn2["as_scripts"].append(Json::Value(as_filename));
    }

    j_terrn2["teleport"] = Json::objectValue;
    j_terrn2["teleport"]["telepoints"] = Json::arrayValue;
    for (auto const & t2_tele : m_terrn2.telepoints)
    {
        Json::Value j_author = Json::objectValue;
        j_author["name"] = t2_tele.name;
        j_author["position"] = this->Vector3ToJson(t2_tele.position);
        j_terrn2["teleport"]["telepoints"].append(j_author);
    }
}

void RoR::Terrn2Deployer::LoadLanduseConfig()
{
    std::string group;
    try
    {
        group = Ogre::ResourceGroupManager::getSingleton()
            .findGroupContainingResource(m_terrn2.traction_map_file);
    }
    catch (...)
    {} // The path may be global -> yields exception, but it's a valid case.

    if (group == "")
        m_landuse.loadDirect(m_terrn2.traction_map_file);
    else
        m_landuse.loadFromResourceSystem(m_terrn2.traction_map_file, group, "\x09:=", true);
}

void RoR::Terrn2Deployer::ProcessLanduseJson()
{
// NOTE: This fileformat is truly awful - take the following code as reference ~ only_a_ptr, 04/2017
    Json::Value j_landuse = Json::objectValue;
    j_landuse["groundmodel_files"] = Json::arrayValue;
    j_landuse["color_mappings"] = Json::arrayValue;

    Ogre::ConfigFile::SectionIterator section_itor = m_landuse.getSectionIterator();
    while (section_itor.hasMoreElements()) // Order matters
    {
        const std::string section_name = section_itor.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap& settings = *section_itor.getNext();
        if (section_name == "general" || section_name == "config")
        {
            for (auto& entry : settings) // Order matters
            {
                Ogre::StringUtil::trim(entry.second);
                if (entry.first == "texture")
                {
                    if (!entry.second.empty())
                        j_landuse["texture_file"] = entry.second;
                }
                else if (entry.first == "frictionconfig" || entry.first == "loadGroundModelsConfig")
                {
                    if (!entry.second.empty())
                        j_landuse["groundmodel_files"].append(entry.second);
                }
                else if (entry.first == "defaultuse")
                {
                    if (!entry.second.empty())
                        j_landuse["default_groundmodel"] = entry.second;
                }
                else
                {
                    LOGSTREAM << "Invalid key '" << entry.first << "' in file '" << m_terrn2.traction_map_file << "'";
                }
            }
        }
        else if (section_name == "use-map")
        {
            for (auto& entry : settings) // Order matters
            {
                if (entry.first.size() != 10)
                {
                    LOGSTREAM << "Invalid color'" << entry.first << "' in file '" << m_terrn2.traction_map_file << "'";
                    continue;
                }
                char* ptr; // TODO: Check error
                unsigned int color = strtoul(entry.first.c_str(), &ptr, 16);
                Json::Value j_entry;
                j_entry["rgba_color"] = color;
                j_entry["groundmodel_name"] = entry.second;
                j_landuse["color_mappings"].append(j_entry);
            }
        }
    }

    m_json["land_use_map"] = j_landuse;
}

bool RoR::Terrn2Deployer::CheckAndLoadOTC()
{
    if (m_terrn2.ogre_ter_conf_filename.empty())
    {
        return true;
    }

    auto& ogre_resources = Ogre::ResourceGroupManager::getSingleton();
    const std::string& group_name = ogre_resources.findGroupContainingResource(m_terrn2.ogre_ter_conf_filename);
    Ogre::DataStreamPtr otc_stream = ogre_resources.openResource(m_terrn2.ogre_ter_conf_filename, group_name);

    RoR::OTCParser parser;
    if (! parser.LoadMasterConfig(otc_stream, m_terrn2.ogre_ter_conf_filename.c_str()))
    {
        return false; // Error already logged
    }

    for (OTCPage& page : parser.GetDefinition()->pages)
    {
        const std::string& page_group_name = ogre_resources.findGroupContainingResource(page.pageconf_filename);
        Ogre::DataStreamPtr page_stream = ogre_resources.openResource(page.pageconf_filename, group_name);

        if (! parser.LoadPageConfig(page_stream, page, page.pageconf_filename.c_str()))
        {
            return false; // Error already logged
        }
    }

    m_otc = parser.GetDefinition();
    return true;
}

void RoR::Terrn2Deployer::ProcessOtcJson()
{
    Json::Value json_otc = Json::objectValue;

    json_otc["world_size_x"]            = m_otc->world_size_x;
    json_otc["world_size_y"]            = m_otc->world_size_y;
    json_otc["world_size_z"]            = m_otc->world_size_z;
    json_otc["page_size"]               = m_otc->page_size;
    json_otc["world_size"]              = m_otc->world_size;
    json_otc["pages_max_x"]             = m_otc->pages_max_x;
    json_otc["pages_max_z"]             = m_otc->pages_max_z;
    json_otc["max_pixel_error"]         = m_otc->max_pixel_error;
    json_otc["batch_size_min"]          = m_otc->batch_size_min;
    json_otc["batch_size_max"]          = m_otc->batch_size_max;
    json_otc["layer_blendmap_size"]     = m_otc->layer_blendmap_size;
    json_otc["composite_map_size"]      = m_otc->composite_map_size;
    json_otc["composite_map_distance"]  = m_otc->composite_map_distance;
    json_otc["skirt_size"]              = m_otc->skirt_size;
    json_otc["lightmap_size"]           = m_otc->lightmap_size;
    json_otc["lightmap_enabled"]        = m_otc->lightmap_enabled;
    json_otc["norm_map_enabled"]        = m_otc->norm_map_enabled;
    json_otc["spec_map_enabled"]        = m_otc->spec_map_enabled;
    json_otc["parallax_enabled"]        = m_otc->parallax_enabled;
    json_otc["global_colormap_enabled"] = m_otc->global_colormap_enabled;
    json_otc["recv_dyn_shadows_depth"]  = m_otc->recv_dyn_shadows_depth;
    json_otc["blendmap_dbg_enabled"]    = m_otc->blendmap_dbg_enabled;
    json_otc["disable_cache"]           = m_otc->disable_cache;
    json_otc["is_flat"]                 = m_otc->is_flat;
    json_otc["cache_filename_base"]     = m_otc->cache_filename_base;
    json_otc["origin_pos"]              = this->Vector3ToJson(m_otc->origin_pos);

    json_otc["pages"] = Json::arrayValue;
    for (OTCPage& page : m_otc->pages)
    {
        Json::Value jpage = Json::objectValue;
        jpage["pageconf_filename"]    = this->StringOrNull(page.pageconf_filename);
        jpage["heightmap_filename"]   = this->StringOrNull(page.heightmap_filename);
        jpage["num_layers"]           = page.num_layers;
        jpage["pos_x"]                = page.pos_x;
        jpage["pos_z"]                = page.pos_z;
        jpage["is_heightmap_raw"]     = page.is_heightmap_raw;
        jpage["raw_flip_x"]           = page.raw_flip_x;
        jpage["raw_flip_y"]           = page.raw_flip_y;
        jpage["raw_size"]             = page.raw_size;
        jpage["raw_bpp"]              = page.raw_bpp;

        jpage["layers"] = Json::arrayValue;
        for (OTCLayer& layer : page.layers)
        {
            Json::Value jlayer = Json::objectValue;
            jlayer["blendmap_filename"]        = this->StringOrNull(layer.blendmap_filename);
            jlayer["diffusespecular_filename"] = this->StringOrNull(layer.diffusespecular_filename);
            jlayer["normalheight_filename"]    = this->StringOrNull(layer.normalheight_filename);
            jlayer["blend_mode"]               = layer.blend_mode;
            jlayer["alpha"]                    = layer.alpha;
            jlayer["world_size"]               = layer.world_size;

            jpage["layers"].append(jlayer);
        }

        json_otc["pages"].append(jpage);
    }

    m_json["ogre_terrain_conf"] = json_otc;
}

void RoR::Terrn2Deployer::LoadTobjFiles()
{
    auto& resource_group_manager = Ogre::ResourceGroupManager::getSingleton();
    for (std::string const& tobj_filename : m_terrn2.tobj_files)
    {
        // create the Stream
        const std::string group_name = resource_group_manager.findGroupContainingResource(tobj_filename);
        Ogre::DataStreamPtr stream = resource_group_manager.openResource(tobj_filename, group_name);

        // Parse the file
        TObjParser parser;
        parser.Prepare();
        bool error = false;
        while (!stream->eof())
        {
            if (!parser.ProcessLine(stream->getLine().c_str()))
            {
                LOGSTREAM << "Error reading .tobj file '" << tobj_filename << "'";
                error = true;
                break;
            }
        }

        // Persist the data
        if (!error)
            m_tobj_list.push_back(parser.Finalize());
    }
}

void RoR::Terrn2Deployer::ProcessTobjJson()
{
    m_json["terrain_objects"] = Json::objectValue;
    m_json["collision_meshes"] = Json::arrayValue;
    Json::Value& j_tobj = m_json["terrain_objects"];

    long num_collision_tris = -1;
    for (std::shared_ptr<TObjFile>& tobj : m_tobj_list)
    {
        // Grid
        if (tobj->grid_enabled)
        {
            j_tobj["grid_enabled"] = Json::Value(true);
            j_tobj["grid_pos"] = this->Vector3ToJson(tobj->grid_position);
        }

        // Collision triangle count (detect)
        if (tobj->num_collision_triangles > num_collision_tris)
            num_collision_tris = tobj->num_collision_triangles;
    }

    // Collision triangle count
    if (num_collision_tris != -1)
        j_tobj["num_collision_triangles"] = num_collision_tris;

    // Trees
    this->ProcessTobjTreesJson();

    // Grass
    this->ProcessTobjGrassJson();

    // Paths, a.k.a. "procedural roads"
    this->ProcessTobjPathsJson();
}

void RoR::Terrn2Deployer::ProcessTobjTreesJson()
{
    m_json["terrain_objects"]["tree_pages"] = Json::arrayValue;
    size_t num_trees = 0;

    for (std::shared_ptr<TObjFile> tobj : m_tobj_list)
    {
        for (TObjTree& tree : tobj->trees)
        {
            if (tree.color_map[0] == 0) // Empty c-string?
            {
                LOGSTREAM << "TObj/Tree has no colormap, skipping...";
                continue;
            }
            if (tree.density_map[0] == 0) // Empty c-string?
            {
                LOGSTREAM << "TObj/Tree has no density-map, skipping...";
                continue;
            }
            Forests::DensityMap* density_map = Forests::DensityMap::load(tree.density_map, Forests::CHANNEL_COLOR);
            if (density_map == nullptr)
            {
                LOGSTREAM << "TObj/Tree: couldn't load density map '" << tree.density_map << "', skipping...";
                continue;
            }

            Json::Value j_page = Json::objectValue;
            if (strcmp(tree.color_map, "none") != 0) // "none" is special value which disables colormap
            {
                j_page["color_map"] = tree.color_map;
            }

            std::stringstream ent_name;
            ent_name << "paged_" << tree.tree_mesh << num_trees;
            j_page["entity_name"] = ent_name.str();
            j_page["tree_mesh"] = tree.tree_mesh;
            j_page["distance_min"] = tree.min_distance;
            j_page["distance_max"] = tree.max_distance;

            // Generate trees
            const float max_x = static_cast<float>(m_otc->world_size_x);
            const float max_z = static_cast<float>(m_otc->world_size_z);
            Ogre::TRect<float> bounds = Forests::TBounds(0, 0, max_x, max_z);
            j_page["trees"] = Json::arrayValue;
            if (tree.grid_spacing > 0)
            {
                // Grid style
                for (float x = 0; x < max_x; x += tree.grid_spacing)
                {
                    for (float z = 0; z < max_z; z += tree.grid_spacing)
                    {
                        const float density = density_map->_getDensityAt_Unfiltered(x, z, bounds);
                        if (density < 0.8f)
                        {
                            continue;
                        }
                        const float pos_x = x + tree.grid_spacing * 0.5f;
                        const float pos_z = z + tree.grid_spacing * 0.5f;
                        const float yaw = Ogre::Math::RangeRandom(tree.yaw_from, tree.yaw_to);
                        const float scale = Ogre::Math::RangeRandom(tree.scale_from, tree.scale_to);
                        Json::Value j_tree = Json::objectValue;
                        j_tree["pos_x"] = pos_x;
                        j_tree["pos_z"] = pos_z;
                        j_tree["yaw"]   = yaw;
                        j_tree["scale"] = scale;
                        if (tree.collision_mesh[0] != 0) // Non-empty cstring?
                        {
                            Ogre::Quaternion rot(Ogre::Degree(yaw), Ogre::Vector3::UNIT_Y);
                            Ogre::Vector3 scale3d(scale * 0.1f, scale * 0.1f, scale * 0.1f);
                            this->AddCollMeshJson(tree.collision_mesh, pos_x, pos_z, rot, scale3d);
                        }
                        j_page["trees"].append(j_tree);
                    }
                }
            }
            else
            {
                // Normal style, random
                float hi_density = tree.high_density;
                float grid_size = 10;
                if ((tree.grid_spacing < 0) && (tree.grid_spacing != 0)) // Verbatim port of old logic
                {
                    grid_size = -tree.grid_spacing;
                }
                for (float x = 0; x < max_x; x += grid_size)
                {
                    for (float z = 0; z < max_z; z += grid_size)
                    {
                        if (tree.high_density < 0)
                        {
                            hi_density = Ogre::Math::RangeRandom(0, -tree.high_density);
                        }
                        const float density = density_map->_getDensityAt_Unfiltered(x, z, bounds);
                        // Pre-generate enough trees for the highest settings
                        const int num_trees = static_cast<int>(hi_density * density * 1.0f) + 1;
                        for (int i = 0; i < num_trees; ++i)
                        {
                            float pos_x = Ogre::Math::RangeRandom(x, x + grid_size);
                            float pos_z = Ogre::Math::RangeRandom(z, z + grid_size);
                            float yaw = Ogre::Math::RangeRandom(tree.yaw_from, tree.yaw_to);
                            float scale = Ogre::Math::RangeRandom(tree.scale_from, tree.scale_to);
                            Json::Value j_tree = Json::objectValue;
                            j_tree["pos_x"] = pos_x;
                            j_tree["pos_z"] = pos_z;
                            j_tree["yaw"]   = yaw;
                            j_tree["scale"] = scale;
                            if (tree.collision_mesh[0] != 0)
                            {
                                Ogre::Quaternion rot(Ogre::Degree(yaw), Ogre::Vector3::UNIT_Y);
                                Ogre::Vector3 scale3d(scale * 0.1f, scale * 0.1f, scale * 0.1f);
                                this->AddCollMeshJson(tree.collision_mesh, pos_x, pos_z, rot, scale3d);
                            }
                            j_page["trees"].append(j_tree);
                        }
                    }
                }
            }
            m_json["terrain_objects"]["tree_pages"].append(j_page);
        } // For each TObjTree
    } // For each TObjFile
}

void RoR::Terrn2Deployer::AddCollMeshJson(const char* name, float pos_x, float pos_z, Ogre::Quaternion rot, Ogre::Vector3 scale)
{
    Json::Value j_colmesh;

    j_colmesh["mesh_name"] = name;
    j_colmesh["pos_x"]     = pos_x;
    j_colmesh["pos_z"]     = pos_z;
    j_colmesh["scale_3d"]  = this->Vector3ToJson(scale);
    j_colmesh["rotq_x"]    = rot.x;
    j_colmesh["rotq_y"]    = rot.y;
    j_colmesh["rotq_z"]    = rot.z;
    j_colmesh["rotq_w"]    = rot.w;

    m_json["collision_meshes"].append(j_colmesh);
}

void RoR::Terrn2Deployer::ProcessTobjGrassJson()
{
    m_json["terrain_objects"]["grass_pages"] = Json::arrayValue;

    for (std::shared_ptr<TObjFile>& tobj : m_tobj_list)
    {
        for (TObjGrass& grass : tobj->grass)
        {
            Json::Value json;
            json["range"]        = grass.range;
            json["technique"]    = grass.technique;
            json["grow_techniq"] = grass.grow_techniq;
            json["sway_speed"]   = grass.sway_speed;
            json["sway_length"]  = grass.sway_length;
            json["sway_distrib"] = grass.sway_distrib;
            json["density"]      = grass.density;

            json["min_x"] = grass.min_x;
            json["min_y"] = grass.min_y;
            json["min_h"] = grass.min_h;
            json["max_x"] = grass.max_x;
            json["max_y"] = grass.max_y;
            json["max_h"] = grass.max_h;

            json["material_name"] = grass.material_name;
            json["color_map_filename"] = this->NoneStringToNull(grass.color_map_filename);
            json["density_map_filename"] = this->NoneStringToNull(grass.density_map_filename);

            m_json["terrain_objects"]["grass_pages"].append(json);
        }
    }
}

void RoR::Terrn2Deployer::ProcessTobjPathsJson()
{
    m_json["terrain_objects"]["procedural_paths"] = Json::arrayValue;

    for (std::shared_ptr<TObjFile>& tobj : m_tobj_list)
    {
        for (ProceduralObject& po : tobj->proc_objects)
        {
            Json::Value j_path = Json::objectValue;
            j_path["name"] = po.name;
            j_path["points"] = Json::arrayValue;

            for (ProceduralPoint& pp: po.points)
            {
                Json::Value j_point = Json::objectValue;
                j_point["rotation"] = this->QuaternionToJson(pp.rotation);
                j_point["position"] = this->Vector3ToJson(pp.position);
                j_point["type"] = pp.type;
                j_point["width"] = pp.width;
                j_point["bwidth"] = pp.bwidth;
                j_point["bheight"] = pp.bheight;
                j_point["pillartype"] = pp.pillartype;

                j_path["points"].append(j_point);
            }

            m_json["terrain_objects"]["procedural_paths"].append(j_path);
        }
    }
}

Json::Value RoR::Terrn2Deployer::NoneStringToNull(const char* str)
{
    if (strcmp(str, "none") == 0)
        return Json::nullValue;
    else
        return str;
}

void RoR::Terrn2Deployer::HandleException(const char* action)
{
    try { throw; } // Rethrow
    catch (Ogre::Exception& e)
    {
        LOGSTREAM << "Error " << action << ", message: " << e.getFullDescription();
    }
    catch (std::exception& e)
    {
        LOGSTREAM << "Error " << action << ", message: " << e.what();
    }
}
