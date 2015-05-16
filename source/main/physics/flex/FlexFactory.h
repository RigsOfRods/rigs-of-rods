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

#pragma once

#include "ForwardDeclarations.h"

#include <OgreVector3.h>

namespace RoR
{

class FlexFactory
{
public:
    FlexFactory() {}

    FlexFactory(
        MaterialFunctionMapper*   mfm,
        MaterialReplacer*         mat_replacer,
        Skin*                     skin,
        node_t*                   all_nodes
        );

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
        );

private:    
    MaterialFunctionMapper* m_material_function_mapper;
    MaterialReplacer*       m_material_replacer;
    Skin*                   m_used_skin;

    bool                    m_enable_flexbody_LODs;

    node_t*                 m_rig_nodes_ptr;
};

} // namespace RoR
