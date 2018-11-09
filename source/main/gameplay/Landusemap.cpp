/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "Landusemap.h"

#include "Application.h"
#include "Collisions.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "TerrainManager.h"
#include "PropertyMaps.h"
#include "PagedGeometry.h"

#include <OgreConfigFile.h>

using namespace Ogre;
using namespace RoR;

Landusemap::Landusemap(String configFilename) :
    data(0)
    , mapsize(Vector3::ZERO)
{
    mapsize = App::GetSimTerrain()->getMaxTerrainSize();
    loadConfig(configFilename);
}

Landusemap::~Landusemap()
{
    if (data != nullptr)
        delete[] data;
}

ground_model_t* Landusemap::getGroundModelAt(int x, int z)
{
    if (!data)
        return nullptr;

    const Vector3 mapsize = App::GetSimTerrain()->getMaxTerrainSize();
    // we return the default ground model if we are not anymore in this map
    if (x < 0 || x >= mapsize.x || z < 0 || z >= mapsize.z)
        return default_ground_model;

    return data[x + z * (int)mapsize.x];
}

int Landusemap::loadConfig(const Ogre::String& filename)
{
    std::map<unsigned int, String> usemap;
    String textureFilename = "";

    LOG("Parsing landuse config: '"+filename+"'");

    String group = "";
    try
    {
        group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
    }
    catch (...)
    {
        // we wont catch anything, since the path could be absolute as well, then the group is not found
    }

    Ogre::ConfigFile cfg;
    try
    {
        // try to load directly otherwise via resource group
        if (group == "")
            cfg.loadDirect(filename);
        else
            cfg.loadFromResourceSystem(filename, group, "\x09:=", true);
    }
    catch (Ogre::Exception& e)
    {
        ErrorUtils::ShowError(_L("Error while loading landuse config"), e.getFullDescription());
        return 1;
    }

    Ogre::ConfigFile::SectionIterator seci = cfg.getSectionIterator();
    Ogre::String secName, kname, kvalue;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap* settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            kname = i->first;
            kvalue = i->second;
            // we got all the data available now, processing now
            if (secName == "general" || secName == "config")
            {
                // set some class properties accoring to the information in this section
                if (kname == "texture")
                    textureFilename = kvalue;
                else if (kname == "frictionconfig" || kname == "loadGroundModelsConfig")
                    gEnv->collisions->loadGroundModelsConfigFile(kvalue);
                else if (kname == "defaultuse")
                    default_ground_model = gEnv->collisions->getGroundModelByString(kvalue);
            }
            else if (secName == "use-map")
            {
                if (kname.size() != 10)
                {
                    LOG("invalid color in landuse line in " + filename);
                    continue;
                }
                char* ptr; //not used
                unsigned int color = strtoul(kname.c_str(), &ptr, 16);
                usemap[color] = kvalue;
            }
        }
    }
    // process the config data and load the buffers finally
    try
    {
        Forests::ColorMap* colourMap = Forests::ColorMap::load(textureFilename, Forests::CHANNEL_COLOR);
        colourMap->setFilter(Forests::MAPFILTER_NONE);

        /*
        // debug things below
        printf("found ground use definitions:\n");
        for (std::map < uint32, String >::iterator it=usemap.begin(); it!=usemap.end(); it++)
        {
            printf(" 0x%Lx : %s\n", it->first, it->second.c_str());
        }
        */

        bool bgr = colourMap->getPixelBox().format == PF_A8B8G8R8;

        const Vector3 mapsize = App::GetSimTerrain()->getMaxTerrainSize();
        Ogre::TRect<Ogre::Real> bounds = Forests::TBounds(0, 0, mapsize.x, mapsize.z);

        // now allocate the data buffer to hold pointers to ground models
        data = new ground_model_t*[(int)(mapsize.x * mapsize.z)];
        ground_model_t** ptr = data;
        //std::map < String, int > counters;
        for (int z = 0; z < mapsize.z; z++)
        {
            for (int x = 0; x < mapsize.x; x++)
            {
                unsigned int col = colourMap->getColorAt(x, z, bounds);
                if (bgr)
                {
                    // Swap red and blue values
                    unsigned int cols = col & 0xFF00FF00;
                    cols |= (col & 0xFF) << 16;
                    cols |= (col & 0xFF0000) >> 16;
                    col = cols;
                }
                String use = usemap[col];
                //if (use!="")
                //	counters[use]++;

                // store the pointer to the ground model in the data slot
                *ptr = gEnv->collisions->getGroundModelByString(use);
                ptr++;
            }
        }
    }
    catch (Ogre::Exception& oex)
    {
        LogFormat("[RoR|Physics] Landuse: failed to load texture '%s', <Ogre::Exception> message: '%s'",
            textureFilename.c_str(), oex.getFullDescription().c_str());
    }
    catch (std::exception& stex)
    {
        LogFormat("[RoR|Physics] Landuse: failed to load texture '%s', <std::exception> message: '%s'",
            textureFilename.c_str(), stex.what());
    }
    catch (...)
    {
        LogFormat("[RoR|Physics] Landuse: failed to load texture '%s', unknown error", textureFilename.c_str());
    }

    return 0;
}
