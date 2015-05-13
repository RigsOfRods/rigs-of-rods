/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

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

#include "RoRPrerequisites.h"
#include "Flexable.h"
#include "MaterialFunctionMapper.h"

#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreMesh.h>

class FlexBody : public Flexable
{
	friend class RigInspector;

public:

	FlexBody(
		node_t *nds, 
		int numnodes, 
		Ogre::String const & meshname, 
		Ogre::String const & uname, 
		int ref, 
		int nx, 
		int ny, 
		Ogre::Vector3 const & offset, 
		Ogre::Quaternion const & rot, 
		std::vector<unsigned int> & node_indices, 
		MaterialFunctionMapper *material_function_mapper, 
		Skin *usedSkin, 
		bool forceNoShadows, 
		MaterialReplacer *material_replacer
	);

	void addinterval(int from, int to);
	bool isinset(int n);
	void printMeshInfo(Ogre::Mesh* mesh);
	void reset();
	void updateBlend();
	void writeBlend();
	Ogre::SceneNode *getSceneNode() { return m_scene_node; };

	void setEnabled(bool e);

	/**
	* Visibility control 
	* @param mode {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}
	*/
	void setCameraMode(int mode) { m_camera_mode = mode; };
	int getCameraMode() { return m_camera_mode; };

	// Flexable
	bool flexitPrepare(Beam* b);
	void flexitCompute();
	Ogre::Vector3 flexitFinal();

	void setVisible(bool visible);



private:

	typedef struct
	{
		int from;
		int to;
	} interval_t;

	struct Locator_t
	{
		int ref;
		int nx;
		int ny;
		int nz;
		Ogre::Vector3 coords;
	};

	static const int MAX_SET_INTERVALS = 256;

	node_t*           m_nodes;
	size_t            m_vertex_count;
	                  
	Ogre::Vector3*    m_dst_pos;
	Ogre::Vector3*    m_src_normals;
	Ogre::Vector3*    m_dst_normals;
	Ogre::ARGB*       m_src_colors;
	Locator_t*        m_locators; //!< 1 loc per vertex

	int	              m_node_center;
	int	              m_node_x;
	int	              m_node_y;
	Ogre::Vector3     m_center_offset;
	Ogre::SceneNode*  m_scene_node;
    Ogre::MeshPtr     m_mesh;
    int               m_camera_mode; //!< Visibility control {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}

	interval_t nodeset[MAX_SET_INTERVALS]; // Factory only
	int freenodeset;// Factory only

	int                                 m_shared_buf_num_verts;
	Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_pos;
	Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_norm;
	Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_color;
	
    int                                 m_num_submesh_vbufs;
	int                                 m_submesh_vbufs_vertex_counts[16];
	Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_pos[16]; //!< positions
	Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_norm[16]; //!< normals
	Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_color[16]; //!< colors

	bool m_is_enabled;
    bool m_is_faulty;
	bool m_uses_shared_vertex_data;
	bool m_has_texture;
	bool m_has_texture_blend;

	
};
