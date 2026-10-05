Adopted from 'Paged Geometry' 
(https://github.com/ogrecave/ogre-pagedgeometry.git)
at commit 7c9295f771e8c7973efc04d53283978ccd44fc9b (dated 12/2020)
Omitted files:
* TreeLoader3D.h/cpp
* WindBatchedGeometry.h/cpp
* WindBatchPage.h/cpp

Changes are marked with `//RIGSOFRODS`
Highlights:
* GrassLoader.h/cpp: enums GrassTechnique and FadeTechnique moved to PagedGeometryConfig.h and synchronized to TOBJ fileformat.
* PagedGeometry: removed constructor args 'camera' and 'pageSize' which have dedicated setters, added grouping scenenode arg.
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