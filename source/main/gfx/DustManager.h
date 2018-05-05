/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal & contributors

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
/// @date   05/2018

#pragma once

#include "ForwardDeclarations.h"

#include <map>
#include <string>

namespace RoR {

// Developer note:
//   This was originally `DustManager` (particle manager), which is scene-wide logic updated every frame.
//     -> It was handy to re-purpose it to manage all visuals.
//   I'm temporarily re-using the file because it eases debugging for me (I can switch revisions without re-generating VisualStudio project)
//   ~ only_a_ptr, 05/2018

/// Provides a 3D graphical representation of the simulation
/// Idea: simulation runs at it's own constant rate, scene updates and rendering run asynchronously.
class GfxScene
{
public:

    void           InitScene(Ogre::SceneManager* sm);
    DustPool*      GetDustPool(const char* name);
    void           SetParticlesVisible(bool visible);
    void           UpdateScene();
    void           DiscardScene(); //!< Final cleanup

private:

    std::map<std::string, DustPool *> m_dustpools;
    Ogre::SceneManager*               m_ogre_scene;
};

} // namespace RoR
