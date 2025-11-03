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

#ifndef _Hydrax_Module_H_
#define _Hydrax_Module_H_

#include "Prerequisites.h"
#include "Application.h"
#include "Noise.h"
#include "Mesh.h"
#include "MaterialManager.h"
#include "GPUNormalMapManager.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax{ namespace Module
{
	/** Base module class,
	    Override it for create different ways of create water noise.
	 */
	class Module
	{
	public:
		/** Constructor
		    @param Name Module name
			@param n Hydrax::Noise::Noise generator pointer
			@param MeshOptions Mesh options
			@param NormalMode Normal generation mode
		 */
		Module(const Ogre::String                &Name,
			   Noise::Noise                      *n,
			   const Mesh::Options               &MeshOptions,
			   const MaterialManager::NormalMode &NormalMode);

		/** Destructor
		 */
		virtual ~Module();

		/** Create
		    @remark Not forgot call in the override class
		 */
		virtual void create();

		/** Remove
		    @remark Not forgot call in the override class
		 */
		virtual void remove();

		/** Set noise
		    @param Noise New noise module
			@param g GPUNormalMapManager pointer, default: NULL, use it if GPU Normal map generation is needed
			@param DeleteOldNoise Delete the old noise module (Default = true)
		 */
		void setNoise(Noise::Noise* Noise, GPUNormalMapManager* g = 0, const bool& DeleteOldNoise = true);

		/** Call it each frame
		    @param timeSinceLastFrame Time since last frame(delta)
		 */
		virtual void update(const Ogre::Real &timeSinceLastFrame);

		/** Save config
		    @param Data String reference
		 */
		virtual void saveCfg(Ogre::String &Data);

		/** Load config
			@return True if is the correct module config
		 */
		virtual bool loadCfg(Ogre::ConfigFile &CfgFile);

		/** Get module name
		    @return Module name
		 */
		inline const Ogre::String& getName() const
		{
			return mName;
		}

		/** Is created() called?
		    @return true if created() have been already called
		 */
		inline const bool& isCreated() const
		{
			return mCreated;
		}

		/** Create geometry in module(If special geometry is needed)
		    @param mMesh Mesh
			@return false if it must be create by default Mesh::_createGeometry() fnc.
			@remarks Override it if any especial geometry mesh creation is needed.
		 */
		inline virtual const bool _createGeometry(Mesh *mMesh) const
		{
			return false;
		}

		/** Get the normal generation mode
		    @return Module normal generation mode
		 */
		inline const MaterialManager::NormalMode& getNormalMode() const
		{
			return mNormalMode;
		}

		/** Get the mesh options for this module
		    @return Mesh options for this module
		 */
		inline const Mesh::Options& getMeshOptions() const
		{
			return mMeshOptions;
		}

		/** Get the Hydrax::Noise module pointer
		    @return Hydrax::Noise pointer
		 */
		inline Noise::Noise* getNoise()
		{
			return mNoise;
		}

		/** Get the current heigth at a especified world-space point
		    @param Position X/Z World position
			@return Heigth at the given position in y-World coordinates, if it's outside of the water return -1
		 */
		virtual float getHeigth(const Ogre::Vector2 &Position);

	protected:
		/// Module name
		Ogre::String mName;
		/// Noise generator pointer
		Noise::Noise *mNoise;
		/// Module mesh options
		Mesh::Options mMeshOptions;
		/// Normal map generation mode
		MaterialManager::NormalMode mNormalMode;
		/// Is create() called?
		bool mCreated;
	};
}}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif