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

#include "FlexFactory.h"

#include "FlexBody.h"
#include "Settings.h"

using namespace RoR;

FlexFactory::FlexFactory(
        MaterialFunctionMapper*   mfm,
        MaterialReplacer*         mat_replacer,
        Skin*                     skin,
        node_t*                   all_nodes
        ):
    m_material_function_mapper(mfm),
    m_material_replacer(mat_replacer),
    m_used_skin(skin),
    m_rig_nodes_ptr(all_nodes)
{
    m_enable_flexbody_LODs = BSETTING("Flexbody_EnableLODs", false);
}

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
    )
{
    return new FlexBody(
        m_rig_nodes_ptr,
        num_nodes_in_rig,
        mesh_name,
        mesh_unique_name,
        ref_node,
        x_node,
        y_node,
        offset,
        rot,
        node_indices,
        m_material_function_mapper,
        m_used_skin,
        m_material_replacer,
        m_enable_flexbody_LODs
    );
}
