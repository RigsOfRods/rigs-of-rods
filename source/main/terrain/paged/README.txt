Adopted from 'Paged Geometry' 
(https://github.com/ogrecave/ogre-pagedgeometry.git)
at commit 5297d3d3f7deed4cf98217078c0fdf80eb28d19c (dated 08/2025)
Omitted files:
* TreeLoader3D.h/cpp
* WindBatchedGeometry.h/cpp
* WindBatchPage.h/cpp

Changes are marked with `//RIGSOFRODS`
Highlights:
* PropertyMaps.cpp: Build fix - Backported `Ogre::Image::create()` from OGRE at a544c84f30c0685976f112679d373459aa977161
* PropertyMaps.cpp: Build fix - Partially reverted 257daa212d8ad8025b432f54e870861a703dfbc8 (PropertyMaps: simplify by using Ogre::Image)
* StaticBillboardSet.h: Build fix - backport of `getAsBYTE()` from OGRE at 754fabc0c7b2f964af3e7fb0afb60587f234678d ("Main: ColourValue - add conversion for native-endian byte formats", dated 10/2020)
* PropertyMaps.h: Build fix - backport of `explicit ColourValue(const uchar* byte)` from 754fabc0c7b2f964af3e7fb0afb60587f234678d ("Main: ColourValue - add conversion for native-endian byte formats" dated 10/2020)
* GrassLoader.cpp, BatchPage.cpp: Vertex shaders are loaded with `setSourceFile()` instead of `setSource()`, so RoR's 'OgreUnifiedShader.h' backport (see `ContentManager::resourceStreamOpened()`) gets to patch them.
* GrassLoader.cpp, BatchPage.cpp, StaticBillboardSet.cpp: Programs assigned to (cloned) materials are loaded explicitly, because OGRE 1.11 doesn't load programs assigned to an already loaded material - D3D9 then fails with "Null program bound".
* ImpostorPage.h/cpp: factored out `ImpostorTexManager` (`static` only) from `ImpostorTexture`
* PagedGeometry.h/cpp: removed dead 'custom parameters'

~~~~~~~~~~~~~~~~~~~~~~~~~~~ LICENSE ~~~~~~~~~~~~~~~~~~~~~~~~~~~

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.