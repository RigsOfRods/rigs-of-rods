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

#ifndef _Hydrax_MaterialManager_H_
#define _Hydrax_MaterialManager_H_

#include "Prerequisites.h"

#include "Enums.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
	class Hydrax;

	/** Material/Shader manager class
	 */
	class MaterialManager
	{
	public:
		/** Material type enum
		    @remarks Use in getMaterial(MaterialType)
		 */
		enum MaterialType
		{
			// Water material
			MAT_WATER = 0,
			// Depth material
			MAT_DEPTH = 1,
			// Underwater material
			MAT_UNDERWATER = 2,
			// Compositor material(material wich is used in underwater compositor)
			MAT_UNDERWATER_COMPOSITOR = 3,
			// Simple red material
			MAT_SIMPLE_RED = 4,
			// Simple black material
			MAT_SIMPLE_BLACK = 5
		};

		/** Compositor type enum
		    @remarks Use in getCompositor(CompositorType)
		 */
		enum CompositorType
		{
			// Underwater compositor
			COMP_UNDERWATER = 0
		};

		/** Gpu program enum
		    @remarks Use in setGpuProgramParameter()
		 */
		enum GpuProgram
		{
			// Vertex program
			GPUP_VERTEX   = 0,
			// Fragment program
			GPUP_FRAGMENT = 1
		};

		/** Shader mode
		 */
		enum ShaderMode
		{
			// HLSL
			SM_HLSL = 0,
			// Cg
			SM_CG   = 1,
			// GLSL
			SM_GLSL = 2
		};

		/** Normal generation mode
		 */
		enum NormalMode
		{
			// Normal map from precomputed texture(CPU)
			NM_TEXTURE = 0,
			// Normal map from vertex(CPU)
			NM_VERTEX  = 1,
			// Normal map from RTT(GPU)
			NM_RTT     = 2
		};

		/** Material options
		 */
		struct Options
		{
			/** Default constructor
			 */
			Options()
				: SM(SM_HLSL)
				, NM(NM_TEXTURE)
			{
			}

			/** Constructor
			    @param _SM Shader mode
				@param _NM Normal generation mode
			 */
			Options(const ShaderMode &_SM,
				    const NormalMode &_NM)
				: SM(_SM)
				, NM(_NM)
			{
			}

			/// Shader mode
			ShaderMode SM;
			/// Normal map generation mode
			NormalMode NM;
		};

		/** Underwater compositor listener
		 */
		class UnderwaterCompositorListener : public Ogre::CompositorInstance::Listener
		{
		public:
			/// On material setup
			void notifyMaterialSetup(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);

			/// On material render
			void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);

			/// Material manager parent pointer
			MaterialManager *mMaterialManager;
		};

		/** Constructor
		    @param h Hydrax pointer
		 */
		MaterialManager(Hydrax *h);

		/** Destructor
		 */
        ~MaterialManager();

		/** Create materials
		    @param Components Components of the shader
			@param Options Material options
		 */
		bool createMaterials(const HydraxComponent &Components, const Options &Options);

		/** Remove materials
		    @remarks removeCompositor() is called too.
		 */
		void removeMaterials();

		/** Remove compositor
	     */
		void removeCompositor();

		/** Reload material
		    @param Material Material to reload
		 */
		void reload(const MaterialType &Material);

		/** Fill GPU vertex and fragment program to a pass
		    @param Pass Pass to fill Gpu programs
			@param GpuProgramNames [0]: Vertex program name, [1]: Fragment program name
			@param SM Shader mode, note: Provided data strings will correspong with selected shader mode
			@param EntryPoints [0]: Vertex program entry point, [1]: Fragment program entry point
			@param Data [0] Vertex program data, [1]: Fragment program data
		 */
		bool fillGpuProgramsToPass(Ogre::Pass* Pass,
							       const Ogre::String GpuProgramNames[2],
							       const ShaderMode& SM,
							       const Ogre::String EntryPoints[2],
							       const Ogre::String Data[2]);

		/** Create GPU program
			@param Name HighLevelGpuProgram name
			@param SM Shader mode
			@param GPUP GpuProgram type
			@param EntryPoint Entry point
			@param Data
		 */
		bool createGpuProgram(const Ogre::String &Name,
			                  const ShaderMode& SM,
							  const GpuProgram& GPUP,
							  const Ogre::String& EntryPoint,
							  const Ogre::String& Data);

		/** Is createMaterials() already called?
		    @return true If yes
		 */
		inline const bool& isCreated() const
		{
			return mCreated;
		}

		/** Get material
		    @param Material Material to get
			@return Material to get
		 */
		inline Ogre::MaterialPtr &getMaterial(const MaterialType &Material)
		{
			return mMaterials[static_cast<int>(Material)];
		}

		/** Get compositor
		    @param Compositor to get
			@return Compositor to get
		 */
		inline Ogre::CompositorPtr &getCompositor(const CompositorType &Compositor)
		{
			return mCompositors[static_cast<int>(Compositor)];
		}

		/** Is the compositor enable?
		    @param Compositor compositor to check
		    @return true if it's enabled
		 */
		inline const bool& isCompositorEnable(const CompositorType &Compositor) const
		{
			return mCompositorsEnable[static_cast<int>(Compositor)];
		}

		/** Set a compositor enable/disable
		    @param Compositor compositor to change
			@param Enable true to enable, false to disable
		 */
		void setCompositorEnable(const CompositorType &Compositor, const bool &Enable);

		/** Get the last MaterialManager::Options used in a material generation
		    @return Last MaterialManager::Options used in a material generation
		 */
		inline const Options &getLastOptions() const
		{
			return mOptions;
		}

		/** Add depth technique to an especified material
		    @param Technique Technique where depth technique will be added
			@param AutoUpdate The technique will be automatically updated when water parameters change
			@remarks Call it after Hydrax::create()/Hydrax::setComponents(...)

			         The technique will be automatically updated when water parameters change if parameter AutoUpdate == true
			         Add depth technique when a material is not an Ogre::Entity, such terrains, PLSM2 materials, etc.
					 This depth technique will be added with "HydraxDepth" scheme in ordeto can use it in the Depth RTT.
		 */
		void addDepthTechnique(Ogre::Technique *Technique, const bool& AutoUpdate = true);

		/** Add depth texture technique to an especified material
		    @param Technique Technique where depth technique will be added
			@param TextureName Texture name
			@param AlphaChannel "x","y","z","w" or "r","g","b","a" (Channel where alpha information is stored)
			@param AutoUpdate The technique will be automatically updated when water parameters change
			@remarks Call it after Hydrax::create()/Hydrax::setComponents(...)

			         The technique will be automatically updated when water parameters change if parameter AutoUpdate == true
			         Add depth technique when a material is not an Ogre::Entity, such terrains, PLSM2 materials, etc.
					 This depth technique will be added with "HydraxDepth" scheme in ordeto can use it in the Depth RTT.
		 */
		void addDepthTextureTechnique(Ogre::Technique *Technique, const Ogre::String& TextureName, const Ogre::String& AlphaChannel = "w", const bool& AutoUpdate = true);

		/** Get external depth techniques
		    @return std::vector of external depth techniques
		 */
		inline std::vector<Ogre::Technique*> &getDepthTechniques()
		{
			return mDepthTechniques;
		}

		/** Set gpu program Ogre::Real parameter
		    @param GpuP Gpu program type (Vertex/Fragment)
			@param MType Water/Depth material
			@param Name param name
			@param Value value
		 */
		void setGpuProgramParameter(const GpuProgram &GpuP, const MaterialType &MType, const Ogre::String &Name, const Ogre::Real &Value);

		/** Set gpu program Ogre::Vector2 parameter
		    @param GpuP Gpu program type (Vertex/Fragment)
			@param MType Water/Depth material
			@param Name param name
			@param Value value
		 */
		void setGpuProgramParameter(const GpuProgram &GpuP, const MaterialType &MType, const Ogre::String &Name, const Ogre::Vector2 &Value);

		/** Set gpu program Ogre::Vector3 parameter
		    @param GpuP Gpu program type (Vertex/Fragment)
			@param MType Water/Depth material
			@param Name param name
			@param Value value
		 */
		void setGpuProgramParameter(const GpuProgram &GpuP, const MaterialType &MType, const Ogre::String &Name, const Ogre::Vector3 &Value);

        /** Animated textures must be updated manually to account for variable simulation time.
        */
        void updateAnimatedTextures(float dt);

	private:
		/** Is component in the given list?
		    @param List Components list
			@param ToCheck Component to check
		    @return true if the component is in the given list.
		 */
		bool _isComponent(const HydraxComponent &List, const HydraxComponent &ToCheck) const;

		/** Create water material
		    @param Components Components of the shader
			@param Options Material options
		 */
		bool _createWaterMaterial(const HydraxComponent &Components, const Options &Options);

		/** Create depth material
		    @param Components Components of the shader
			@param Options Material options
		 */
		bool _createDepthMaterial(const HydraxComponent &Components, const Options &Options);

		/** Create depth texture gpu programs
		    @param Components Components of the sahder
			@param Options Material options
			@param AlphaChannel "x","y","z","w" or "r","g","b","a" (Channel where alpha information is stored)
			@return true if no problems had happend, false if yes
		 */
		bool _createDepthTextureGPUPrograms(const HydraxComponent &Components, const Options &Options, const Ogre::String& AlphaChannel);

		/** Create underwater material
		    @param Components Components of the shader
			@param Options Material options
		 */
		bool _createUnderwaterMaterial(const HydraxComponent &Components, const Options &Options);

		/** Create underwater compositor
		    @param Components Components of the shader
			@param Options Material options
		 */
		bool _createUnderwaterCompositor(const HydraxComponent &Components, const Options &Options);

		bool _createSimpleColorMaterial(const Ogre::ColourValue& MaterialColor, const MaterialType& MT, const Ogre::String& MaterialName, const bool& DepthCheck = true, const bool& DepthWrite = true);

		/// Is createMaterials() already called?
		bool mCreated;
		/// Hydrax materials vector
		Ogre::MaterialPtr mMaterials[6];
		/// Hydrax compositors vector
		Ogre::CompositorPtr mCompositors[1];
		/// Hydrax compositors boolean: Need to be reloaded?
		bool mCompositorsNeedToBeReloaded[1];
		/// Hydrax compostor enable vector
		bool mCompositorsEnable[1];
		/// Technique vector for addDepthTechnique(...)
		std::vector<Ogre::Technique*> mDepthTechniques;
		/// Actual material components
		HydraxComponent mComponents;
		/// Actual material options
		Options mOptions;
		/// Underwater compositor listener
		UnderwaterCompositorListener mUnderwaterCompositorListener;
		/// Hydrax main pointer
		Hydrax *mHydrax;
        /// Caustics animated texture, for manual updating.
        std::vector<Ogre::TextureUnitState*> mCausticsAnimTexVec;
        unsigned int mCausticsAnimCurrentFrame = 0;
        /// Time spent on current animation frame, cumulative.
        float mCausticsAnimCurrentFrameElapsedTime = 0.f;
	};
};

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif