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
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#ifndef __RandomTreeLoader_H__
    #define __RandomTreeLoader_H__

    #include "PagedGeometry.h"
    #include "PropertyMaps.h"
    #include "RoRPrerequisites.h"
    #include "TreeLoader2D.h"

namespace Forests
{

    class TreeIterator3D;
    class TreeIterator2D;

    class RandomTreeLoader : public TreeLoader2D, public ZeroedMemoryAllocator
    {
      public:
        RandomTreeLoader(PagedGeometry *geom, const TBounds &bounds) : TreeLoader2D(geom, bounds)
        {
        }

        ~RandomTreeLoader()
        {
        }

        void loadPage(PageInfo &page)
        {
            // Calculate x/z indexes for the grid array
            page.xIndex -= Math::Floor(gridBounds.left / pageSize);
            page.zIndex -= Math::Floor(gridBounds.top / pageSize);

            // Check if the requested page is in bounds
            if (page.xIndex < 0 || page.zIndex < 0 || page.xIndex >= pageGridX || page.zIndex >= pageGridZ) return;

            // just take the first tree registered as an example
            std::vector<TreeDef> *pageGrid = pageGridList.begin()->second;
            std::vector<TreeDef> &treeList = _getGridPage(pageGrid, page.xIndex, page.zIndex);

            // example values
            minimumScale = 0.07;
            maximumScale = 0.13;

            // if there are no tress in the page to be loaded, add some randomly
            if (treeList.size() == 0)
            {
                // only add trees if there are none in this page...
                for (int n = 0; n < pageSize / 10; n++)
                {
                    // Create the new tree
                    Real   xrel = Math::RangeRandom(0, pageSize);
                    Real   zrel = Math::RangeRandom(0, pageSize);
                    Degree yaw(Math::RangeRandom(0, 360));
                    Real   scale = Math::RangeRandom(minimumScale, maximumScale);

                    TreeDef tree;
                    tree.xPos     = 65535 * (xrel - (page.xIndex * pageSize)) / pageSize;
                    tree.zPos     = 65535 * (zrel - (page.zIndex * pageSize)) / pageSize;
                    tree.rotation = 255 * (yaw.valueDegrees() / 360.0f);
                    tree.scale    = 255 * ((scale - minimumScale) / maximumScale);
                    treeList.push_back(tree);
                }
            }

            // now load page as normal
            TreeLoader2D::loadPage(page);
        }

        void unloadPage(PageInfo &page)
        {
            // Calculate x/z indexes for the grid array
            page.xIndex -= Math::Floor(gridBounds.left / pageSize);
            page.zIndex -= Math::Floor(gridBounds.top / pageSize);

            // Check if the requested page is in bounds
            if (page.xIndex < 0 || page.zIndex < 0 || page.xIndex >= pageGridX || page.zIndex >= pageGridZ) return;

            // just take the first tree registered as an example
            std::vector<TreeDef> *pageGrid = pageGridList.begin()->second;
            std::vector<TreeDef> &treeList = _getGridPage(pageGrid, page.xIndex, page.zIndex);

            // clear all existing trees
            uint32 i = 0;
            while (i < treeList.size())
            {
                // Check if tree is in bounds
    #ifdef USE_PAGEDGEOMETRY_USER_DATA
                deletedUserData.push_back(treeList[i].userData);
    #endif
                // delete it
                treeList[i] = treeList.back();
                treeList.pop_back();
                ++i;
            }

            // now unload page as normal
            TreeLoader2D::unloadPage(page);
        }
    };

} // namespace Forests
#endif