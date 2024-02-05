/*
    This source file is part of Rigs of Rods

    Copyright 2015-2020 Petr Ohlidal

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

/// @file
/// @author Petr Ohlidal
/// @date   05/2015

#include "FlexFactory.h"

#include "Application.h"
#include "Actor.h"
#include "CacheSystem.h"
#include "FlexBody.h"
#include "FlexMeshWheel.h"
#include "GfxScene.h"
#include "PlatformUtils.h"
#include "RigDef_File.h"
#include "ActorSpawner.h"

#include <OgreMeshManager.h>
#include <OgreSceneManager.h>
#include <MeshLodGenerator/OgreMeshLodGenerator.h>

//#define FLEXFACTORY_DEBUG_LOGGING

#ifdef FLEXFACTORY_DEBUG_LOGGING
#   include "RoRPrerequisites.h"
#   define FLEX_DEBUG_LOG(TEXT) LOG("FlexFactory | " TEXT)
#else
#   define FLEX_DEBUG_LOG(TEXT)    
#endif // FLEXFACTORY_DEBUG_LOGGING

using namespace RoR;

// Static
const char * FlexBodyFileIO::SIGNATURE = "RoR FlexBody";

FlexFactory::FlexFactory(ActorSpawner* rig_spawner):
    m_rig_spawner(rig_spawner),
    m_is_flexbody_cache_loaded(false),
    m_is_flexbody_cache_enabled(App::gfx_flexbody_cache->getBool()),
    m_flexbody_cache_next_index(0)
{
}

FlexBody* FlexFactory::CreateFlexBody(
    FlexbodyID_t flexbody_id,
    const NodeNum_t ref_node, 
    const NodeNum_t x_node, 
    const NodeNum_t y_node, 
    Ogre::Vector3 offset,
    Ogre::Vector3 rotation, 
    std::vector<unsigned int> & node_indices,
    const std::string& mesh_name,
    const std::string& resource_group_name)
{
    Ogre::MeshPtr common_mesh = Ogre::MeshManager::getSingleton().load(mesh_name, resource_group_name);
    const std::string mesh_unique_name = m_rig_spawner->ComposeName(fmt::format("{}_FlexBody", mesh_name).c_str(), flexbody_id);
    Ogre::MeshPtr mesh = common_mesh->clone(mesh_unique_name);
    const std::string flexbody_name = m_rig_spawner->ComposeName("Flexbody", flexbody_id);
    Ogre::Entity* entity = App::GetGfxScene()->GetSceneManager()->createEntity(flexbody_name, mesh_unique_name, resource_group_name);
    m_rig_spawner->SetupNewEntity(entity, Ogre::ColourValue(0.5, 0.5, 1));

    FLEX_DEBUG_LOG(__FUNCTION__);
    FlexBodyCacheData* from_cache = nullptr;
    if (m_is_flexbody_cache_loaded)
    {
        FLEX_DEBUG_LOG(__FUNCTION__ " >> Get entry from cache ");
        from_cache = m_flexbody_cache.GetLoadedItem(m_flexbody_cache_next_index);
        m_flexbody_cache_next_index++;
    }

    Ogre::Quaternion rot=Ogre::Quaternion(Ogre::Degree(rotation.z), Ogre::Vector3::UNIT_Z);
    rot=rot*Ogre::Quaternion(Ogre::Degree(rotation.y), Ogre::Vector3::UNIT_Y);
    rot=rot*Ogre::Quaternion(Ogre::Degree(rotation.x), Ogre::Vector3::UNIT_X);

    FlexBody* new_flexbody = new FlexBody(
        from_cache,
        m_rig_spawner->GetActor()->GetGfxActor(),
        entity,
        ref_node,
        x_node,
        y_node,
        offset,
        rot,
        node_indices);

    if (m_is_flexbody_cache_enabled)
    {
        m_flexbody_cache.AddItemToSave(new_flexbody);
    }
    new_flexbody->m_id = flexbody_id;
    new_flexbody->m_orig_mesh_name = common_mesh->getName();
    return new_flexbody;
}

FlexMeshWheel* FlexFactory::CreateFlexMeshWheel(
    unsigned int wheel_index,
    int axis_node_1_index,
    int axis_node_2_index,
    int nstart,
    int nrays,
    float rim_radius,
    bool rim_reverse,
    std::string const & rim_mesh_name,
    std::string const & rim_mesh_rg,
    std::string const & tire_material_name,
    std::string const & tire_material_rg)
{
    const ActorPtr& actor = m_rig_spawner->GetActor();

    // Load+instantiate static mesh for rim (may be located in addonpart ZIP-bundle!)
    const std::string rim_entity_name = m_rig_spawner->ComposeName("rim @ *wheel*", wheel_index);
    Ogre::Entity* rim_prop_entity = App::GetGfxScene()->GetSceneManager()->createEntity(rim_entity_name, rim_mesh_name, rim_mesh_rg);
    m_rig_spawner->SetupNewEntity(rim_prop_entity, Ogre::ColourValue(0, 0.5, 0.8));

    // Create dynamic mesh for tire (always located in the actor resource group)
    const std::string tire_mesh_name = m_rig_spawner->ComposeName("tire @ *wheel*", wheel_index);
    FlexMeshWheel* flex_mesh_wheel = new FlexMeshWheel(
        rim_prop_entity,
        m_rig_spawner->m_wheels_parent_scenenode->createChildSceneNode(m_rig_spawner->ComposeName("*wheel*", wheel_index)), // Friend access
        m_rig_spawner->GetActor()->GetGfxActor(), axis_node_1_index, axis_node_2_index, nstart, nrays,
        tire_mesh_name, actor->GetGfxActor()->GetResourceGroup(),
        tire_material_name, tire_material_rg, rim_radius, rim_reverse);

    // Instantiate the dynamic tire mesh (always located in the actor resource group)
    const std::string tire_instance_name = m_rig_spawner->ComposeName("tire entity @ *wheel*", wheel_index);
    Ogre::Entity *tire_entity = App::GetGfxScene()->GetSceneManager()->createEntity(
        tire_instance_name, tire_mesh_name, actor->GetGfxActor()->GetResourceGroup());
    m_rig_spawner->SetupNewEntity(tire_entity, Ogre::ColourValue(0, 0.5, 0.8));
    flex_mesh_wheel->m_tire_entity = tire_entity; // Friend access.

    return flex_mesh_wheel;
}

void FlexBodyFileIO::WriteToFile(void* source, size_t length)
{
    size_t num_written = fwrite(source, length, 1, m_file);
    if (num_written != 1)
    {
        FLEX_DEBUG_LOG(__FUNCTION__ " >> EXCEPTION!! ");
        throw RESULT_CODE_FWRITE_OUTPUT_INCOMPLETE;
    }
}

void FlexBodyFileIO::ReadFromFile(void* dest, size_t length)
{
    size_t num_written = fread(dest, length, 1, m_file);
    if (num_written != 1)
    {
        FLEX_DEBUG_LOG(__FUNCTION__ " >> EXCEPTION!! ");
        throw RESULT_CODE_FREAD_OUTPUT_INCOMPLETE;
    }
}

void FlexBodyFileIO::WriteSignature()
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    WriteToFile((void*)SIGNATURE, (strlen(SIGNATURE) + 1) * sizeof(char));
}

void FlexBodyFileIO::ReadAndCheckSignature()
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    char signature[25];
    this->ReadFromFile((void*)&signature, (strlen(SIGNATURE) + 1) * sizeof(char));
    if (strcmp(SIGNATURE, signature) != 0)
    {
        throw RESULT_CODE_ERR_SIGNATURE_MISMATCH;
    }
}

void FlexBodyFileIO::WriteMetadata()
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    FlexBodyFileMetadata meta;
    meta.file_format_version = FILE_FORMAT_VERSION;    
    meta.num_flexbodies      = static_cast<int>(m_items_to_save.size());

    this->WriteToFile((void*)&meta, sizeof(FlexBodyFileMetadata));
}

void FlexBodyFileIO::ReadMetadata(FlexBodyFileMetadata* meta)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    ROR_ASSERT(meta != nullptr);
    this->ReadFromFile((void*)meta, sizeof(FlexBodyFileMetadata));
}

void FlexBodyFileIO::WriteFlexbodyHeader(FlexBody* flexbody)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    FlexBodyRecordHeader header;
    header.vertex_count            = static_cast<int>(flexbody->m_vertex_count);
    header.node_center             = flexbody->m_node_center            ;
    header.node_x                  = flexbody->m_node_x                 ;
    header.node_y                  = flexbody->m_node_y                 ;
    header.center_offset           = flexbody->m_center_offset          ;
    header.camera_mode             = flexbody->m_camera_mode            ;
    header.shared_buf_num_verts    = flexbody->m_shared_buf_num_verts   ;
    header.num_submesh_vbufs       = flexbody->m_num_submesh_vbufs      ;

    if (flexbody->m_uses_shared_vertex_data) BITMASK_SET_1(header.flags, FlexBodyRecordHeader::USES_SHARED_VERTEX_DATA);
    if (flexbody->m_has_texture            ) BITMASK_SET_1(header.flags, FlexBodyRecordHeader::HAS_TEXTURE);
    if (flexbody->m_has_texture_blend      ) BITMASK_SET_1(header.flags, FlexBodyRecordHeader::HAS_TEXTURE_BLEND);

    this->WriteToFile((void*)&header, sizeof(FlexBodyRecordHeader));
}

void FlexBodyFileIO::ReadFlexbodyHeader(FlexBodyCacheData* data)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    this->ReadFromFile((void*)&data->header, sizeof(FlexBodyRecordHeader));
}


void FlexBodyFileIO::WriteFlexbodyLocatorList(FlexBody* flexbody)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    this->WriteToFile((void*)flexbody->m_locators, sizeof(Locator_t) * flexbody->m_vertex_count);
}

void FlexBodyFileIO::ReadFlexbodyLocatorList(FlexBodyCacheData* data)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    // Alloc. Use <new> - experiment
    data->locators = new Locator_t[data->header.vertex_count];
    // Read
    this->ReadFromFile((void*)data->locators, sizeof(Locator_t) * data->header.vertex_count);
}

void FlexBodyFileIO::WriteFlexbodyNormalsBuffer(FlexBody* flexbody)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    this->WriteToFile((void*)flexbody->m_src_normals, sizeof(Ogre::Vector3) * flexbody->m_vertex_count);
}

void FlexBodyFileIO::ReadFlexbodyNormalsBuffer(FlexBodyCacheData* data)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    const int vertex_count = data->header.vertex_count;
    // Alloc. Use malloc() because that's how flexbodies were implemented.
    data->src_normals=(Ogre::Vector3*)malloc(sizeof(Ogre::Vector3) * vertex_count);
    // Read
    this->ReadFromFile((void*)data->src_normals, sizeof(Ogre::Vector3) * vertex_count);
}

void FlexBodyFileIO::WriteFlexbodyPositionsBuffer(FlexBody* flexbody)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    this->WriteToFile((void*)flexbody->m_dst_pos, sizeof(Ogre::Vector3) * flexbody->m_vertex_count);
}

void FlexBodyFileIO::ReadFlexbodyPositionsBuffer(FlexBodyCacheData* data)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    const int vertex_count = data->header.vertex_count;
    // Alloc. Use malloc() because that's how flexbodies were implemented.
    data->dst_pos=(Ogre::Vector3*)malloc(sizeof(Ogre::Vector3) * vertex_count);
    // Read
    this->ReadFromFile((void*)data->dst_pos, sizeof(Ogre::Vector3) * vertex_count);
}

void FlexBodyFileIO::WriteFlexbodyColorsBuffer(FlexBody* flexbody)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    if (flexbody->m_has_texture_blend)
    {
        this->WriteToFile((void*)flexbody->m_src_colors, sizeof(Ogre::ARGB) * flexbody->m_vertex_count);
    }
}

void FlexBodyFileIO::ReadFlexbodyColorsBuffer(FlexBodyCacheData* data)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    if (BITMASK_IS_0(data->header.flags, FlexBodyRecordHeader::HAS_TEXTURE_BLEND))
    {
        return;
    }
    const int vertex_count = data->header.vertex_count;
    // Alloc. Use malloc() because that's how flexbodies were implemented.
    data->src_colors=(Ogre::ARGB*)malloc(sizeof(Ogre::ARGB) * vertex_count);
    // Read
    this->ReadFromFile((void*)data->src_colors, sizeof(Ogre::ARGB) * vertex_count);
}

void FlexBodyFileIO::OpenFile(const char* fopen_mode)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    if (m_cache_entry_number == -1)
    {
        throw RESULT_CODE_ERR_CACHE_NUMBER_UNDEFINED;
    }
    char path[500];
    sprintf(path, "%s%cflexbodies_mod_%00d.dat", App::sys_cache_dir->getStr().c_str(), RoR::PATH_SLASH, m_cache_entry_number);
    m_file = fopen(path, fopen_mode);
    if (m_file == nullptr)
    {
        throw RESULT_CODE_ERR_FOPEN_FAILED;
    }
}

FlexBodyFileIO::ResultCode FlexBodyFileIO::SaveFile()
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    if (m_items_to_save.size() == 0)
    {
        FLEX_DEBUG_LOG(__FUNCTION__ " >> No flexbodies to save >> EXIT");
        return RESULT_CODE_OK;
    }
    try
    {
        this->OpenFile("wb");

        this->WriteSignature();
        this->WriteMetadata();

        auto itor = m_items_to_save.begin();
        auto end  = m_items_to_save.end();
        for (; itor != end; ++itor)
        {
            FlexBody* flexbody = *itor;
            this->WriteFlexbodyHeader(flexbody);

            this->WriteFlexbodyLocatorList    (flexbody);
            this->WriteFlexbodyPositionsBuffer(flexbody);
            this->WriteFlexbodyNormalsBuffer  (flexbody);
            this->WriteFlexbodyColorsBuffer   (flexbody);
        }
        this->CloseFile();
        FLEX_DEBUG_LOG(__FUNCTION__ " >> OK ");
        return RESULT_CODE_OK;
    }
    catch (ResultCode result)
    {
        this->CloseFile();
        FLEX_DEBUG_LOG(__FUNCTION__ " >> EXCEPTION!! ");
        return result;
    }
}

FlexBodyFileIO::ResultCode FlexBodyFileIO::LoadFile()
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    try 
    {
        this->OpenFile("rb");
        this->ReadAndCheckSignature();

        FlexBodyFileMetadata meta;
        this->ReadMetadata(&meta);
        m_fileformat_version = meta.file_format_version;
        if (m_fileformat_version != FILE_FORMAT_VERSION)
        {
            throw RESULT_CODE_ERR_VERSION_MISMATCH;
        }
        m_loaded_items.resize(meta.num_flexbodies);

        for (unsigned int i = 0; i < meta.num_flexbodies; ++i)
        {
            FlexBodyCacheData* data = & m_loaded_items[i];
            this->ReadFlexbodyHeader(data);
            if (BITMASK_IS_0(data->header.flags, FlexBodyRecordHeader::IS_FAULTY))
            {
                this->ReadFlexbodyLocatorList    (data);
                this->ReadFlexbodyPositionsBuffer(data);
                this->ReadFlexbodyNormalsBuffer  (data);
                this->ReadFlexbodyColorsBuffer   (data);
            }
        }

        this->CloseFile();
        FLEX_DEBUG_LOG(__FUNCTION__ " >> OK ");
        return RESULT_CODE_OK;
    }
    catch (ResultCode ret)
    {
        this->CloseFile();
        FLEX_DEBUG_LOG(__FUNCTION__ " >> EXCEPTION!! ");
        return ret;
    }
}

FlexBodyFileIO::FlexBodyFileIO():
    m_file(nullptr),
    m_fileformat_version(0),
    m_cache_entry_number(-1) // flexbody cache disabled (shouldn't be based on the cache entry number ...) ~ ulteq 01/19
    {}

void FlexFactory::CheckAndLoadFlexbodyCache()
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    if (m_is_flexbody_cache_enabled)
    {
        m_is_flexbody_cache_loaded = 
            (m_flexbody_cache.LoadFile() == FlexBodyFileIO::RESULT_CODE_OK);
    }
}

void FlexFactory::SaveFlexbodiesToCache()
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    if (m_is_flexbody_cache_enabled && !m_is_flexbody_cache_loaded)
    {
        FLEX_DEBUG_LOG(__FUNCTION__ " >> Saving flexbodies");
        m_flexbody_cache.SaveFile();
    }
}

