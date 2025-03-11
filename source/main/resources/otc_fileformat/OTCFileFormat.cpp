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

/// @file
/// @author Petr Ohlidal, 11/2016

#include "OTCFileFormat.h"

#include "Application.h"
#include "Console.h"
#include "SimConstants.h"
#include "ConfigFile.h"
#include "Utils.h"

#include <algorithm>
#include <OgreException.h>

RoR::OTCParser::OTCParser() :
    m_def(std::make_shared<RoR::OTCDocument>())
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
        cfg.setLoggingInfo(ds->getName(), Console::CONSOLE_MSGTYPE_TERRN);

        m_def->disable_cache           = cfg.getBool  ("disableCaching",  false);
        m_def->world_size_x            = cfg.getInt   ("WorldSizeX",      1024);
        m_def->world_size_y            = cfg.getInt   ("WorldSizeY",      50);
        m_def->world_size_z            = cfg.getInt   ("WorldSizeZ",      1024);
        m_def->page_size               = cfg.getInt   ("PageSize",        1025);
        m_def->pages_max_x             = cfg.getInt   ("PagesX",          0);
        m_def->pages_max_z             = cfg.getInt   ("PagesZ",          0);
        m_def->page_filename_format    = cfg.getString("PageFileFormat",  /*section:*/Ogre::BLANKSTRING, /*defaultValue:*/file_basename + "-page-{X}-{Z}.otc");
        m_def->is_flat                 = cfg.getBool  ("Flat",            false);
        m_def->max_pixel_error         = cfg.getInt   ("MaxPixelError",   5);
        m_def->batch_size_min          = cfg.getInt   ("minBatchSize",    33);
        m_def->batch_size_max          = cfg.getInt   ("maxBatchSize",    65);
        m_def->lightmap_enabled        = cfg.getBool  ("LightmapEnabled",            false);
        m_def->norm_map_enabled        = cfg.getBool  ("NormalMappingEnabled",       false);
        m_def->spec_map_enabled        = cfg.getBool  ("SpecularMappingEnabled",     false);
        m_def->parallax_enabled        = cfg.getBool  ("ParallaxMappingEnabled",     false);
        m_def->blendmap_dbg_enabled    = cfg.getBool  ("DebugBlendMaps",             false);
        m_def->global_colormap_enabled = cfg.getBool  ("GlobalColourMapEnabled",     false);
        m_def->recv_dyn_shadows_depth  = cfg.getBool  ("ReceiveDynamicShadowsDepth", false);
        m_def->composite_map_distance  = cfg.getInt   ("CompositeMapDistance",       4000);
        m_def->layer_blendmap_size     = cfg.getInt   ("LayerBlendMapSize",          1024);
        m_def->composite_map_size      = cfg.getInt   ("CompositeMapSize",           1024);
        m_def->lightmap_size           = cfg.getInt   ("LightMapSize",               1024);
        m_def->skirt_size              = cfg.getInt   ("SkirtSize",                  30);

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

                snprintf(key, 75, "%s.raw.size", base);                int raw_size = cfg.getInt(key, 1025);
                snprintf(key, 75, "%s.raw.bpp",  base);                int raw_bpp  = cfg.getInt(key, 2);
                snprintf(key, 75, "%s.flipX",    base);                bool flip_x  = cfg.getBool(key, false);
                snprintf(key, 75, "%s.flipY",    base);                bool flip_y  = cfg.getBool(key, false);

                m_def->pages.emplace_back(x, z, filename, flip_x, flip_y, raw_size, raw_bpp);
            }
        }
    }
    catch (...)
    {
        RoR::HandleGenericException(fmt::format("OTCParser::LoadMasterConfig({})", filename));
        return false;
    }
    return true;
}

bool RoR::OTCParser::LoadPageConfig(Ogre::DataStreamPtr &ds, RoR::OTCPage& page, const char* filename)
{
    try
    {
        // NOTE: ds->getLine() trims the string on both sides.
        page.heightmap_filename = SanitizeUtf8String(ds->getLine());

        page.num_layers = PARSEINT(ds->getLine());

        while (!ds->eof())
        {
            std::string line_sane = SanitizeUtf8String(ds->getLine());
            if (line_sane.empty() || line_sane[0] == ';' || line_sane[0] == '/')
            {
                continue;
            }

            Ogre::StringVector args = Ogre::StringUtil::split(line_sane, ",");
            if (args.size() < 1)
            {
                LOG(std::string("[RoR|Terrain] Invalid OTC page config: [") + filename + "]");
                return false;
            }

            OTCLayer layer;
            layer.world_size = PARSEREAL(args[0]);
            if (args.size() > 1)
            {
                layer.diffusespecular_filename = TrimStr(args[1]);
            }
            if (args.size() > 2)
            {
                layer.normalheight_filename = TrimStr(args[2]);
            }
            if (args.size() > 3)
            {
                layer.blendmap_filename = TrimStr(args[3]);
            }
            if (args.size() > 4)
            {
                layer.blend_mode = TrimStr(args[4])[0];
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
        RoR::HandleGenericException(fmt::format("OTCParser::LoadPageConfig({})", filename));
        return false;
    }
    return true;
}

RoR::OTCDocument::OTCDocument():
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
