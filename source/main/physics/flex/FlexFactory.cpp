/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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

/**
	@file   
	@author Petr Ohlidal
	@date   05/2015
*/

#include "FlexFactory.h"

#include "FlexBody.h"
#include "Settings.h"

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

FlexFactory::FlexFactory(
        MaterialFunctionMapper*   mfm,
        MaterialReplacer*         mat_replacer,
        Skin*                     skin,
        node_t*                   all_nodes,
        bool                      is_flexbody_cache_enabled,
        int                       cache_entry_number
        ):
    m_material_function_mapper(mfm),
    m_material_replacer(mat_replacer),
    m_used_skin(skin),
    m_rig_nodes_ptr(all_nodes),
    m_is_flexbody_cache_loaded(false),
    m_is_flexbody_cache_enabled(is_flexbody_cache_enabled),
    m_flexbody_cache_next_index(0)
{
    m_enable_flexbody_LODs = BSETTING("Flexbody_EnableLODs", false);
    m_flexbody_cache.SetCacheEntryNumber(cache_entry_number);
}

FlexBody* FlexFactory::CreateFlexBody(
    const int num_nodes_in_rig, 
    const char* mesh_name, 
    const char* mesh_unique_name, 
    const int ref_node, 
    const int x_node, 
    const int y_node, 
    Ogre::Vector3 const & offset,
    Ogre::Quaternion const & rot, 
    std::vector<unsigned int> & node_indices
    )
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    FlexBodyCacheData* from_cache = nullptr;
    if (m_is_flexbody_cache_loaded)
    {
        FLEX_DEBUG_LOG(__FUNCTION__ " >> Get entry from cache ");
        from_cache = m_flexbody_cache.GetLoadedItem(m_flexbody_cache_next_index);
        m_flexbody_cache_next_index++;
    }
    assert(m_rig_nodes_ptr != nullptr);
    FlexBody* new_flexbody = new FlexBody(
        from_cache,
        m_rig_nodes_ptr,
        num_nodes_in_rig,
        mesh_name,
        mesh_unique_name,
        ref_node,
        x_node,
        y_node,
        offset,
        rot,
        node_indices,
        m_material_function_mapper,
        m_used_skin,
        m_material_replacer,
        m_enable_flexbody_LODs
    );
    if (m_is_flexbody_cache_enabled)
    {
        m_flexbody_cache.AddItemToSave(new_flexbody);
    }
    return new_flexbody;
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
    meta.num_flexbodies      = m_items_to_save.size();

    this->WriteToFile((void*)&meta, sizeof(FlexBodyFileMetadata));
}

void FlexBodyFileIO::ReadMetadata(FlexBodyFileMetadata* meta)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    assert(meta != nullptr);
    this->ReadFromFile((void*)meta, sizeof(FlexBodyFileMetadata));
}

void FlexBodyFileIO::WriteFlexbodyHeader(FlexBody* flexbody)
{
    FLEX_DEBUG_LOG(__FUNCTION__);
    FlexBodyRecordHeader header;
    header.vertex_count            = flexbody->m_vertex_count           ;
    header.node_center             = flexbody->m_node_center            ;
    header.node_x                  = flexbody->m_node_x                 ;
    header.node_y                  = flexbody->m_node_y                 ;
    header.center_offset           = flexbody->m_center_offset          ;
    header.camera_mode             = flexbody->m_camera_mode            ;
    header.shared_buf_num_verts    = flexbody->m_shared_buf_num_verts   ;
    header.num_submesh_vbufs       = flexbody->m_num_submesh_vbufs      ;
    header.SetIsEnabled             (flexbody->m_is_enabled             );
    header.SetIsFaulty              (flexbody->m_is_faulty              );
	header.SetUsesSharedVertexData  (flexbody->m_uses_shared_vertex_data); 
	header.SetHasTexture            (flexbody->m_has_texture            );
	header.SetHasTextureBlend       (flexbody->m_has_texture_blend      );
    
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
    if (! data->header.HasTextureBlend())
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
    sprintf(path, "%sflexbodies_mod_%00d.dat", SSETTING("CachePath", "").c_str(), m_cache_entry_number); // SSETTING(CachePath) includes separator
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

            if (!flexbody->m_is_faulty && flexbody->m_is_enabled)
            {
                this->WriteFlexbodyLocatorList    (flexbody);
                this->WriteFlexbodyPositionsBuffer(flexbody);
                this->WriteFlexbodyNormalsBuffer  (flexbody);
                this->WriteFlexbodyColorsBuffer   (flexbody);
            }
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
            if (!data->header.IsFaulty() && data->header.IsEnabled())
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
