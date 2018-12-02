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

/// @file
/// @author Petr Ohlidal, 11/2016

#include "OTCFileformat.h"

#include "Application.h"
#include "BeamConstants.h"
#include "ConfigFile.h"
#include "Utils.h"

#include <algorithm>
#include <OgreException.h>

RoR::OTCParser::OTCParser() :
    m_def(std::make_shared<RoR::OTCFile>())
{
}

bool RoR::OTCParser::LoadMasterConfig(Ogre::DataStreamPtr &ds, const char* filename)
{
    std::string file_basename, file_ext;
    Ogre::StringUtil::splitBaseFilename(filename, file_basename, file_ext);
    RoR::ConfigFile cfg;
    try
    {
        cfg.load(ds, "\t:=", false);

        m_def->disable_cache           = cfg.GetBool  ("disableCaching",  false);
        m_def->world_size_x            = cfg.GetInt   ("WorldSizeX",      1024);
        m_def->world_size_y            = cfg.GetInt   ("WorldSizeY",      50);
        m_def->world_size_z            = cfg.GetInt   ("WorldSizeZ",      1024);
        m_def->page_size               = cfg.GetInt   ("PageSize",        1025);
        m_def->pages_max_x             = cfg.GetInt   ("PagesX",          0);
        m_def->pages_max_z             = cfg.GetInt   ("PagesZ",          0);
        m_def->page_filename_format    = cfg.GetString("PageFileFormat",  file_basename + "-page-{X}-{Z}.otc");
        m_def->is_flat                 = cfg.GetBool  ("Flat",            false);
        m_def->max_pixel_error         = cfg.GetInt   ("MaxPixelError",   5);
        m_def->batch_size_min          = cfg.GetInt   ("minBatchSize",    33);
        m_def->batch_size_max          = cfg.GetInt   ("maxBatchSize",    65);
        m_def->lightmap_enabled        = cfg.GetBool  ("LightmapEnabled",            false);
        m_def->norm_map_enabled        = cfg.GetBool  ("NormalMappingEnabled",       false);
        m_def->spec_map_enabled        = cfg.GetBool  ("SpecularMappingEnabled",     false);
        m_def->parallax_enabled        = cfg.GetBool  ("ParallaxMappingEnabled",     false);
        m_def->blendmap_dbg_enabled    = cfg.GetBool  ("DebugBlendMaps",             false);
        m_def->global_colormap_enabled = cfg.GetBool  ("GlobalColourMapEnabled",     false);
        m_def->recv_dyn_shadows_depth  = cfg.GetBool  ("ReceiveDynamicShadowsDepth", false);
        m_def->composite_map_distance  = cfg.GetInt   ("CompositeMapDistance",       4000);
        m_def->layer_blendmap_size     = cfg.GetInt   ("LayerBlendMapSize",          1024);
        m_def->composite_map_size      = cfg.GetInt   ("CompositeMapSize",           1024);
        m_def->lightmap_size           = cfg.GetInt   ("LightMapSize",               1024);
        m_def->skirt_size              = cfg.GetInt   ("SkirtSize",                  30);

        m_def->world_size = std::max(m_def->world_size_x, m_def->world_size_z);
        m_def->origin_pos = Ogre::Vector3(m_def->world_size_x / 2.0f, 0.0f, m_def->world_size_z / 2.0f);
        m_def->cache_filename_base = file_basename;

        for (int x = 0; x <= m_def->pages_max_x; ++x)
        {
            for (int z = 0; z <= m_def->pages_max_z; ++z)
            {
                std::string filename = m_def->page_filename_format;
                filename = Ogre::StringUtil::replaceAll(filename, "{X}", TOSTRING(x));
                filename = Ogre::StringUtil::replaceAll(filename, "{Z}", TOSTRING(z));

                char base[50], key[75];
                snprintf(base, 50, "Heightmap.%d.%d", x, z);

                snprintf(key, 75, "%s.raw.size", base);                int raw_size = cfg.GetInt(key, 1025);
                snprintf(key, 75, "%s.raw.bpp",  base);                int raw_bpp  = cfg.GetInt(key, 2);
                snprintf(key, 75, "%s.flipX",    base);                bool flip_x  = cfg.GetBool(key, false);
                snprintf(key, 75, "%s.flipY",    base);                bool flip_y  = cfg.GetBool(key, false);

                m_def->pages.emplace_back(x, z, filename, flip_x, flip_y, raw_size, raw_bpp);
            }
        }
    }
    catch (...)
    {
        this->HandleException(filename);
        return false;
    }
    return true;
}

bool RoR::OTCParser::LoadPageConfig(Ogre::DataStreamPtr &ds, RoR::OTCPage& page, const char* filename)
{
    const int LINE_BUF_LEN = 4000;
    char line_buf[LINE_BUF_LEN];
    try
    {
        ds->readLine(line_buf, LINE_BUF_LEN);
        page.heightmap_filename = RoR::Utils::SanitizeUtf8CString(line_buf);
        RoR::Utils::TrimStr(page.heightmap_filename);

        ds->readLine(line_buf, LINE_BUF_LEN);
        page.num_layers = PARSEINT(line_buf);

        while (!ds->eof())
        {
            size_t len = ds->readLine(line_buf, LINE_BUF_LEN);
            if (len == 0 || line_buf[0] == ';' || line_buf[0] == '/') { continue; }

            std::string line_sane = RoR::Utils::SanitizeUtf8String(std::string(line_buf));
            Ogre::StringVector args = Ogre::StringUtil::split(line_sane, ",");
            if (args.size() < 1)
            {
                LOG(std::string("[RoR|Terrain] Invalid OTC page config: [") + filename + "]");
                return false;
            }

            OTCLayer layer;
            layer.world_size = PARSEREAL(args[0]);
            if (args.size() > 2)
            {
                layer.diffusespecular_filename = Utils::TrimStr(args[1]);
            }
            if (args.size() > 1)
            {
                layer.normalheight_filename = Utils::TrimStr(args[2]);
            }
            if (args.size() > 3)
            {
                layer.blendmap_filename = Utils::TrimStr(args[3]);
            }
            if (args.size() > 4)
            {
                layer.blend_mode = Utils::TrimStr(args[4])[0];
            }
            if (args.size() > 5)
            {
                layer.alpha = PARSEREAL(args[5]);
            }

            page.layers.push_back(layer);
        }

        if (page.heightmap_filename.find(".raw") != std::string::npos)
        {
            page.is_heightmap_raw = true;
        }

        int actual_num_layers = static_cast<int>(page.layers.size());
        if (page.num_layers != actual_num_layers)
        {
            LogFormat("[RoR|Terrain] Warning: File \"%s\" declares %d layers but defines %d. Correcting declared layer count to %d",
                filename, page.num_layers, actual_num_layers, actual_num_layers);
            page.num_layers = actual_num_layers;
        }
    }
    catch (...)
    {
        this->HandleException(filename);
        return false;
    }
    return true;
}

void RoR::OTCParser::HandleException(const char* filename)
{
    try { throw; } // Rethrow
    catch (Ogre::Exception& e)
    {
        LOG(std::string("[RoR|Terrain] Error reading OTC file '") + filename + "', message: " + e.getFullDescription());
    }
    catch (std::exception& e)
    {
        LOG(std::string("[RoR|Terrain] Error reading OTC file '") + filename + "', Message: " + e.what());
    }
}

RoR::OTCFile::OTCFile():
    world_size_x(0), world_size_y(0), world_size_z(0), world_size(0),
    page_size(0), pages_max_x(0), pages_max_z(0),
    origin_pos(Ogre::Vector3::ZERO),
    batch_size_min(0), batch_size_max(0),
    layer_blendmap_size(0), max_pixel_error(0), composite_map_size(0), composite_map_distance(0),
    skirt_size(0), lightmap_size(0),
    lightmap_enabled(false), norm_map_enabled(false), spec_map_enabled(false), parallax_enabled(false),
    global_colormap_enabled(false), recv_dyn_shadows_depth(false), disable_cache(false), is_flat(true)
{
}

RoR::OTCPage::OTCPage(int x_pos, int z_pos, std::string const & conf_filename, bool flipX, bool flipY, int rawsize, int rawbpp):
    pageconf_filename(conf_filename),
    pos_x(x_pos), pos_z(z_pos),
    is_heightmap_raw(false), raw_flip_x(flipX), raw_flip_y(flipY),
    raw_size(rawsize), raw_bpp(rawbpp),
    num_layers(0)
{}

RoR::OTCLayer::OTCLayer():
    blend_mode('R'),
    alpha(static_cast<float>('R')) // Backwards compatibility, probably old typo
{
}
