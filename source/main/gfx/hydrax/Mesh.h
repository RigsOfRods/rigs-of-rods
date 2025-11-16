/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Verguín González <xavierverguin@hotmail.com>
                                           <xavyiy@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

#ifndef _Hydrax_Mesh_H_
#define _Hydrax_Mesh_H_

#include "Prerequisites.h"

#include "Help.h"
#include "Image.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
	class Hydrax;

    /** Class wich contains all funtions/variables related to
        Hydrax water mesh
     */
    class Mesh
    {
    public:
		/** Vertex struct for position, normals and uv data.
		 */
		struct POS_NORM_UV_VERTEX
		{
			float x,y,z;
			float nx,ny,nz;
			float tu,tv;
		};

		/** Vertex struct for position and normals data.
		 */
		struct POS_NORM_VERTEX
		{
			float x,y,z;
			float nx,ny,nz;
		};

		/** Vertex struct for position and uv data.
		 */
		struct POS_UV_VERTEX
		{
			float x,y,z;
			float tu,tv;
		};

		/** Vertex struct for position data.
		 */
		struct POS_VERTEX
		{
			float x,y,z;
		};

		/** Mesh vertex type enum
		 */
		enum VertexType
		{
			VT_POS_NORM_UV = 0,
			VT_POS_NORM    = 1,
			VT_POS_UV      = 2,
			VT_POS         = 3,
		};

		/** Base Hydrax mesh options
		 */
		struct Options
		{
			/** Constructor
			 */
			Options()
				: MeshComplexity(128)
				, MeshSize(Size(0))
				, MeshStrength(10)
				, MeshVertexType(VT_POS_NORM_UV)
			{
			}

			/** Constructor
			    @param meshComplexity Grid complexity
			    @param meshSize grid size (X/Z) world space.
				@param meshVertexType Mesh::VertexType
			 */
			Options(const int &meshComplexity, const Size &meshSize, const VertexType &meshVertexType)
				: MeshComplexity(meshComplexity)
				, MeshSize(meshSize)
				, MeshStrength(10)
				, MeshVertexType(meshVertexType)
			{
			}

			/** Constructor
			    @param meshComplexity Grid complexity
			    @param meshSize grid size (X/Z) world space.
				@param meshStrength Water strength(Y axis multiplier)
				@param meshVertexType Mesh::VertexType
			 */
			Options(const int &meshComplexity, const Size &meshSize, const float &meshStrength, const VertexType &meshVertexType)
				: MeshComplexity(meshComplexity)
				, MeshSize(meshSize)
				, MeshStrength(meshStrength)
				, MeshVertexType(meshVertexType)
			{
			}

			/// Mesh complexity
			int MeshComplexity;
			/// Grid size (X/Z) world space.
			Size MeshSize;
			/// Water strength
			float MeshStrength;
			/// Vertex type
			VertexType MeshVertexType;
		};

        /** Constructor
            @param h Hydrax pointer
         */
		Mesh(Hydrax *h);

        /** Destructor
         */
        ~Mesh();

        /** Update options
            @param Options Mesh options
			@remarks Call it before create(...)
         */
        void setOptions(const Options &Options);

        /** Set mesh material
            @param MaterialName The material name
         */
        void setMaterialName(const Ogre::String &MaterialName);

        /** Create our water mesh, geometry, entity, etc...
            @remarks Call it after setMeshOptions() and setMaterialName()
         */
        void create();

		/** Remove all resources
		 */
		void remove();

		/** Update geomtry
		    @param numVer Number of vertices
			@param verArray Vertices array
			@return false If number of vertices do not correspond.
		 */
		bool updateGeometry(const int &numVer, void* verArray);

		/** Get if a Position point is inside of the grid
		    @param Position World-space point
			@return true if Position point is inside of the grid, else false.
		 */
		bool isPointInGrid(const Ogre::Vector2 &Position);

		/** Get the [0,1] range x/y grid position from a 2D world space x/z point
		    @param Position World-space point
			@return (-1,-1) if the point isn't in the grid.
		 */
		Ogre::Vector2 getGridPosition(const Ogre::Vector2 &Position);

	    /** Get the object-space position from world-space position
		    @param WorldSpacePosition Position in world coords
			@return Position in object-space
		 */
		const Ogre::Vector3 getObjectSpacePosition(const Ogre::Vector3& WorldSpacePosition) const;

		/** Get the world-space position from object-space position
		    @param ObjectSpacePosition Position in object coords
			@return Position in world-space
		 */
		const Ogre::Vector3 getWorldSpacePosition(const Ogre::Vector3& ObjectSpacePosition) const;

        /** Get mesh
            @return Mesh
         */
        inline Ogre::MeshPtr getMesh()
        {
            return mMesh;
        }

        /** Get sub mesh
            @return Sub mesh
         */
        inline Ogre::SubMesh* getSubMesh()
        {
            return mSubMesh;
        }

        /** Get entity
            @return Entity
         */
        inline Ogre::Entity* getEntity()
        {
            return mEntity;
        }

        /** Get options
		    @return Mesh options
		 */
		inline const Options& getOptions() const
		{
			return mOptions;
		}

		/** Get mesh size
		    @return Mesh size
		 */
		inline const Size& getSize() const
		{
			return mOptions.MeshSize;
		}

		/** Get vertex type
		    return Mesh vertex type
		 */
		inline const VertexType& getVertexType() const
		{
			return mOptions.MeshVertexType;
		}

        /** Get number of faces
            @return Number of faces
         */
        inline const int& getNumFaces() const
        {
            return mNumFaces;
        }

        /** Get number of vertices
            @return Number of vertices
         */
        inline const int& getNumVertices() const
        {
            return mNumVertices;
        }

        /** Get material name
            @return Material name
         */
        inline const Ogre::String& getMaterialName() const
        {
            return mMaterialName;
        }

		/** Get hardware vertex buffer reference
            @return Ogre::HardwareVertexBufferSharedPtr reference
         */
        inline Ogre::HardwareVertexBufferSharedPtr &getHardwareVertexBuffer()
        {
            return mVertexBuffer;
        }

		/** Get hardware index buffer reference
		    @return Ogre::HardwareIndexBufferSharedPtr reference
		 */
		inline Ogre::HardwareIndexBufferSharedPtr &getHardwareIndexBuffer()
		{
			return mIndexBuffer;
		}

		/** Get the Ogre::SceneNode pointer where Hydrax mesh is attached
		    @return Ogre::SceneNode*
		 */
		inline Ogre::SceneNode* getSceneNode()
		{
			return mSceneNode;
		}

		/** Is _createGeometry() called?
		    @return true if created() have been already called
		 */
		inline const bool& isCreated() const
		{
			return mCreated;
		}

    private:
		/** Create mesh geometry
		 */
		void _createGeometry();

        /// Mesh options
        Options mOptions;
		/// Is _createGeometry() called?
		bool mCreated;
        /// Ogre::MeshPtr
        Ogre::MeshPtr mMesh;
        /// Ogre::Submesh pointer
        Ogre::SubMesh *mSubMesh;
        /// Ogre::Entity pointer
        Ogre::Entity *mEntity;
        /// Number of faces
        int mNumFaces;
        /// Number of vertices
        int mNumVertices;

        /// Vertex buffer
        Ogre::HardwareVertexBufferSharedPtr mVertexBuffer;
        /// Index buffer
        Ogre::HardwareIndexBufferSharedPtr  mIndexBuffer;

		/// Ogre::SceneNode pointer
		Ogre::SceneNode* mSceneNode;

        /// Material name
        Ogre::String mMaterialName;

        /// Hydrax pointer
		Hydrax *mHydrax;
    };
}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
