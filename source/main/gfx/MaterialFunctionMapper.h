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

#pragma once

#include "RoRPrerequisites.h"

class MaterialFunctionMapper : public ZeroedMemoryAllocator
{
public:

    struct materialmapping_t
    {
        Ogre::ColourValue emissiveColour;
        Ogre::String material;
        Ogre::String originalmaterial;
        bool laststate;
        int type;
    };

    void addMaterial(int flareid, materialmapping_t t);
    void toggleFunction(int flareid, bool enabled);
    // this function searches and replaces materials in meshes
    void replaceMeshMaterials(Ogre::Entity* e);

    static void replaceSimpleMeshMaterials(Ogre::Entity* e, Ogre::ColourValue c = Ogre::ColourValue::White);

private:

    static int simpleMaterialCounter;
    std::map<int, std::vector<materialmapping_t>> materialBindings;
};
