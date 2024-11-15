/*
    This source file is part of Rigs of Rods
    Copyright 2016-2020 Petr Ohlidal

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

#pragma once

/// @file
/// @author Petr Ohlidal, 11/2016

#include <OgreColourValue.h>
#include <OgreDataStream.h>
#include <OgreVector3.h>

#include <string>
#include <list>
#include <memory>

namespace RoR {

struct OTCLayer
{
    OTCLayer();

    std::string  blendmap_filename;
    std::string  diffusespecular_filename;
    std::string  normalheight_filename;
    char         blend_mode;
    float        alpha;
    float        world_size;
};

struct OTCPage
{
    OTCPage(int pos_x, int pos_z, std::string const & conf_filename, bool flipX, bool flipY, int rawsize, int rawbpp);

    std::string  pageconf_filename;
    std::string  heightmap_filename;
    int          num_layers;
    int          pos_x, pos_z;
    bool         is_heightmap_raw, raw_flip_x, raw_flip_y;
    int          raw_size, raw_bpp;

    std::list<OTCLayer> layers;
};

/// Rembember OGRE coordinates are {X = right/left, Y = up/down, Z = front/back}
struct OTCDocument
{
    OTCDocument();

    std::string        page_filename_format;
    std::string        cache_filename_base;
    std::list<OTCPage> pages;
    Ogre::Vector3      origin_pos;

    int          world_size_x, world_size_y, world_size_z;
    int          page_size;
    int          world_size;
    int          pages_max_x, pages_max_z; //!< Highest page index
    int          max_pixel_error;
    int          batch_size_min, batch_size_max;
    int          layer_blendmap_size;
    int          composite_map_size;
    int          composite_map_distance;
    int          skirt_size;
    int          lightmap_size;
    bool         lightmap_enabled;
    bool         norm_map_enabled;
    bool         spec_map_enabled;
    bool         parallax_enabled;
    bool         global_colormap_enabled;
    bool         recv_dyn_shadows_depth;
    bool         blendmap_dbg_enabled;
    bool         disable_cache;
    bool         is_flat;
};

class OTCParser
{
public:
    OTCParser();

    bool                      LoadMasterConfig(Ogre::DataStreamPtr &ds, const char* filename);
    bool                      LoadPageConfig(Ogre::DataStreamPtr &ds, OTCPage& page, const char* filename);
    OTCDocumentPtr  GetDefinition() { return m_def; };

private:

    OTCDocumentPtr  m_def;
};

} // namespace RoR
