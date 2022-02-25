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

--------------------------------------------------------------------------------
Contributors:
    Jose Luis Cercós Pita <jlcercos@alumnos.upm.es>
--------------------------------------------------------------------------------
*/

#include <OgreString.h>

#include <MaterialManager.h>

#include <Hydrax.h>

#define _def_Water_Material_Name  "_Hydrax_Water_Material"
#define _def_Water_Shader_VP_Name "_Hydrax_Water_VP"
#define _def_Water_Shader_FP_Name "_Hydrax_Water_FP"

#define _def_Depth_Material_Name  "_Hydrax_Depth_Material"
#define _def_Depth_Shader_VP_Name "_Hydrax_Depth_VP"
#define _def_Depth_Shader_FP_Name "_Hydrax_Depth_FP"

#define _def_DepthTexture_Shader_VP_Name "_Hydrax_DepthTexture_VP"
#define _def_DepthTexture_Shader_FP_Name "_Hydrax_DepthTexture_FP"

#define _def_Underwater_Material_Name  "_Hydrax_Underwater_Material"
#define _def_Underwater_Shader_VP_Name "_Hydrax_Underwater_Shader_VP"
#define _def_Underwater_Shader_FP_Name "_Hydrax_Underwater_Shader_FP"

#define _def_Underwater_Compositor_Material_Name  "_Hydrax_Underwater_Compositor_Material"
#define _def_Underwater_Compositor_Shader_VP_Name "_Hydrax_Underwater_Compositor_Shader_VP"
#define _def_Underwater_Compositor_Shader_FP_Name "_Hydrax_Underwater_Compositor_Shader_FP"

#define _def_Underwater_Compositor_Name "_Hydrax_Underwater_Compositor_Name"

#define _def_Simple_Red_Material_Name "_Hydrax_Simple_Red_Material"
#define _def_Simple_Black_Material_Name "_Hydrax_Simple_Black_Material"

namespace Hydrax
{
	MaterialManager::MaterialManager(Hydrax *h)
		: mCreated(false)
		, mComponents(HYDRAX_COMPONENTS_NONE)
		, mHydrax(h)
	{
		for (int k = 0; k < 6; k++)
		{
			mMaterials[k].setNull();
		}

		for (int k = 0; k < 1; k++)
		{
			mCompositorsEnable[k] = false;
			mCompositorsNeedToBeReloaded[k] = false;
		}

		mUnderwaterCompositorListener.mMaterialManager = this;
	}

	MaterialManager::~MaterialManager()
	{
		removeMaterials();
	}

	void MaterialManager::removeMaterials()
	{
		if (Ogre::MaterialManager::getSingleton().resourceExists(_def_Water_Material_Name))
		{
			Ogre::MaterialManager::getSingleton().remove(_def_Water_Material_Name);

			Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_Water_Shader_VP_Name);
		    Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_Water_Shader_FP_Name);
			Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_Water_Shader_VP_Name);
		    Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_Water_Shader_FP_Name);
		}

		if (Ogre::MaterialManager::getSingleton().resourceExists(_def_Depth_Material_Name))
		{
			Ogre::MaterialManager::getSingleton().remove(_def_Depth_Material_Name);

			Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_Depth_Shader_VP_Name);
		    Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_Depth_Shader_FP_Name);
			Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_Depth_Shader_VP_Name);
		    Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_Depth_Shader_FP_Name);
		}

		if (Ogre::MaterialManager::getSingleton().resourceExists(_def_Underwater_Material_Name))
		{
			Ogre::MaterialManager::getSingleton().remove(_def_Underwater_Material_Name);

			Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_Underwater_Shader_VP_Name);
		    Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_Underwater_Shader_FP_Name);
			Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_Underwater_Shader_VP_Name);
		    Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_Underwater_Shader_FP_Name);
		}

		if (Ogre::MaterialManager::getSingleton().resourceExists(_def_Simple_Red_Material_Name))
		{
			Ogre::MaterialManager::getSingleton().remove(_def_Simple_Red_Material_Name);
		}

		if (Ogre::MaterialManager::getSingleton().resourceExists(_def_Simple_Black_Material_Name))
		{
			Ogre::MaterialManager::getSingleton().remove(_def_Simple_Black_Material_Name);
		}

		Ogre::String AlphaChannels[] = {"x","y","z","w",
		                                "r","g","b","a"};

		for (int k = 0; k<8; k++)
		{
			if (Ogre::HighLevelGpuProgramManager::getSingleton().resourceExists(_def_DepthTexture_Shader_VP_Name + AlphaChannels[k]))
			{
				Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_DepthTexture_Shader_VP_Name + AlphaChannels[k]);
				Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_DepthTexture_Shader_FP_Name + AlphaChannels[k]);
			    Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_DepthTexture_Shader_VP_Name + AlphaChannels[k]);
				Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_DepthTexture_Shader_FP_Name + AlphaChannels[k]);
			}
		}

		removeCompositor();

		mCreated = false;
	}

	void MaterialManager::removeCompositor()
	{
		if (Ogre::MaterialManager::getSingleton().resourceExists(_def_Underwater_Compositor_Material_Name))
		{
			setCompositorEnable(COMP_UNDERWATER, false);
			Ogre::CompositorManager::getSingleton().remove(_def_Underwater_Compositor_Name);

			Ogre::MaterialManager::getSingleton().remove(_def_Underwater_Compositor_Material_Name);

			Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_Underwater_Compositor_Shader_VP_Name);
		    Ogre::HighLevelGpuProgramManager::getSingleton().unload(_def_Underwater_Compositor_Shader_FP_Name);
			Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_Underwater_Compositor_Shader_VP_Name);
		    Ogre::HighLevelGpuProgramManager::getSingleton().remove(_def_Underwater_Compositor_Shader_FP_Name);
		}
	}

	bool MaterialManager::createMaterials(const HydraxComponent &Components, const Options &Options)
	{
		removeMaterials();

		HydraxLOG("Creating water material...");
		if (!_createWaterMaterial(Components, Options))
		{
			return false;
		}
		mHydrax->getDecalsManager()->registerAll();
		HydraxLOG("Water material created.");

		if (_isComponent(Components, HYDRAX_COMPONENT_DEPTH))
		{
			HydraxLOG("Creating depth material...");
			if(!_createDepthMaterial(Components, Options))
			{
				return false;
			}
			HydraxLOG("Depth material created.");
		}

		if (_isComponent(Components, HYDRAX_COMPONENT_UNDERWATER))
		{
			HydraxLOG("Creating underwater material...");
			if(!_createUnderwaterMaterial(Components, Options))
			{
				return false;
			}
			if(!_createUnderwaterCompositor(Components, Options))
			{
				return false;
			}
			if(!_createSimpleColorMaterial(Ogre::ColourValue::Red, MAT_SIMPLE_RED, _def_Simple_Red_Material_Name, false))
			{
				return false;
			}
			HydraxLOG("Underwater material created.");
		}

		mComponents = Components;
		mOptions = Options;
		mCreated = true;

		std::vector<Ogre::Technique*>::iterator TechIt;

		for(TechIt = mDepthTechniques.begin(); TechIt != mDepthTechniques.end(); TechIt++)
		{
			if (!(*TechIt))
			{
				mDepthTechniques.erase(TechIt);
				// TechIt-- ?
				continue;
			}

			bool isTextureDepthTechnique =
				((*TechIt)->getName() == "_Hydrax_Depth_Technique") ? false : true;

			if (isTextureDepthTechnique)
			{
				Ogre::String DepthTextureName =
					((*TechIt)->getPass(0)->getTextureUnitState(0)->getName() == "_DetphTexture_Hydrax") ?
					    (*TechIt)->getPass(0)->getTextureUnitState(0)->getTextureName() : (*TechIt)->getPass(0)->getTextureUnitState(1)->getTextureName();

				                                                      // Alpha channel will be stored in pass 0 name
				addDepthTextureTechnique((*TechIt), DepthTextureName, (*TechIt)->getPass(0)->getName() , false);
			}
			else
			{
				addDepthTechnique((*TechIt), false);
			}
		}

		return true;
	}

	bool MaterialManager::fillGpuProgramsToPass(Ogre::Pass* Pass,
							                    const Ogre::String GpuProgramNames[2],
						                        const ShaderMode& SM,
				     	                        const Ogre::String EntryPoints[2],
			     		                        const Ogre::String Data[2])
	{
		GpuProgram GpuPrograms[2] = {GPUP_VERTEX, GPUP_FRAGMENT};

		for (int k = 0; k < 2; k++)
		{
			 if (!createGpuProgram(GpuProgramNames[k], SM, GpuPrograms[k], EntryPoints[k], Data[k]))
			 {
				 return false;
			 }
		}

		Pass->setVertexProgram(GpuProgramNames[0]);
		Pass->setFragmentProgram(GpuProgramNames[1]);

		return true;
	}

	bool MaterialManager::createGpuProgram(const Ogre::String &Name,
			                               const ShaderMode& SM,
							               const GpuProgram& GPUP,
							               const Ogre::String& EntryPoint,
							               const Ogre::String& Data)
	{
		if (Ogre::HighLevelGpuProgramManager::getSingleton().resourceExists(Name))
		{
			HydraxLOG("Error in bool MaterialManager::createGpuProgram(): "+ Name + " exists.");
			return false;
		}

		Ogre::String ShaderModesStr[3] = {"hlsl", "cg", "glsl"};
		Ogre::String Profiles[2];

		switch (SM)
		{
		    case SM_HLSL:
			{
				Profiles[0] = "target";

				if (GPUP == GPUP_VERTEX)
				{
				    Profiles[1] = "vs_1_1";
				}
				else
				{
					Profiles[1] = "ps_2_0";
				}
			}
			break;

			case SM_CG:
			{
				Profiles[0] = "profiles";

				if (GPUP == GPUP_VERTEX)
				{
				    Profiles[1] = "vs_1_1 arbvp1";
				}
				else
				{
					Profiles[1] = "ps_2_0 arbfp1 fp20";
				}
			}
			break;

			case SM_GLSL:
			{
				Profiles[0] = "";       // Dont needed
				if (GPUP == GPUP_VERTEX)
				{
				    Profiles[1] = "";   // Dont needed
				}
				else
				{
					Profiles[1] = "";   // Dont needed
				}
            }
			break;
		}

		Ogre::GpuProgramType GpuPType;

		if (GPUP == GPUP_VERTEX)
		{
			GpuPType = Ogre::GPT_VERTEX_PROGRAM;
		}
		else
		{
			GpuPType = Ogre::GPT_FRAGMENT_PROGRAM;
		}

		Ogre::HighLevelGpuProgramPtr HLGpuProgram =
			Ogre::HighLevelGpuProgramManager::getSingleton().
			      createProgram(Name,
			                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
					  		    ShaderModesStr[SM],
			                    GpuPType);

	    HLGpuProgram->setSource(Data);
        HLGpuProgram->setParameter("entry_point", EntryPoint);
        HLGpuProgram->setParameter(Profiles[0], Profiles[1]);
		HLGpuProgram->load();

		return true;
	}

	bool MaterialManager::_createWaterMaterial(const HydraxComponent &Components, const Options &Options)
	{
		const bool cDepth    = _isComponent(Components, HYDRAX_COMPONENT_DEPTH   );
		const bool cSmooth   = _isComponent(Components, HYDRAX_COMPONENT_SMOOTH  );
		const bool cSun      = _isComponent(Components, HYDRAX_COMPONENT_SUN     );
		const bool cFoam     = _isComponent(Components, HYDRAX_COMPONENT_FOAM    );
		const bool cCaustics = _isComponent(Components, HYDRAX_COMPONENT_CAUSTICS);

		Ogre::String VertexProgramData, FragmentProgramData;

		// Vertex program

		switch (Options.NM)
		{
		    case NM_TEXTURE:
		    {
				switch (Options.SM)
				{
				    case SM_HLSL: case SM_CG:
					{
						VertexProgramData +=
						Ogre::String(
						"void main_vp(\n") +
						       // IN
                          	   "float4 iPosition         : POSITION,\n" +
                           	   "float2 iUv               : TEXCOORD0,\n" +
                           	   // OUT
                           	   "out float4 oPosition      : POSITION,\n" +
							   "out float4 oPosition_     : TEXCOORD0,\n" +
                               "out float2 oUvNoise       : TEXCOORD1,\n" +
                               "out float4 oUvProjection  : TEXCOORD2,\n";
                               // UNIFORM
						       if (cFoam)
							   {
                                    VertexProgramData += Ogre::String(
                                    "out float4 oWorldPosition : TEXCOORD3,\n") +
                                    "uniform float4x4         uWorld,\n";
							   }
							   VertexProgramData += Ogre::String(
                               "uniform float4x4         uWorldViewProj)\n") +
               	        "{\n" +
                  	        "oPosition_  = iPosition;\n";
							if (cFoam)
							{
							    VertexProgramData += "oWorldPosition = mul(uWorld, iPosition);\n";
							}
                            VertexProgramData +=
                            Ogre::String(
	              	        "oPosition = mul(uWorldViewProj, iPosition);\n") +
	               	        // Projective texture coordinates, adjust for mapping
	                	    "float4x4 scalemat = float4x4(0.5,   0,   0, 0.5,"+
	                                              	     "0,-0.5,   0, 0.5,"+
	                							  	     "0,   0, 0.5, 0.5,"+
	                							  	     "0,   0,   0,   1);\n" +
	               	        "oUvProjection = mul(scalemat, oPosition);\n" +
	               	        "oUvNoise = iUv;\n" +
               	         "}\n";
					}
					break;

					case SM_GLSL:
					{
						VertexProgramData += Ogre::String( "\n");
                        // UNIFORMS
                        if(cFoam)
                        {
                            VertexProgramData += "uniform mat4 uWorld;\n";
                        }
                        // IN
                        // OUT
                        VertexProgramData += Ogre::String(
                        "varying vec4 Position_;\n") +
                        "varying vec4 UVProjection;\n";
                        if(cFoam)
                        {
                            VertexProgramData += "varying vec4 WorldPosition;\n";
                        }
                        // PROGRAM
                        VertexProgramData += Ogre::String(
                        "void main()\n") +
                        "{\n" +
                            "Position_ = gl_Vertex;\n";
                            if(cFoam)
                            {
                                VertexProgramData += "WorldPosition = uWorld * gl_Vertex;\n";
                            }
                            VertexProgramData += Ogre::String(
                            "gl_Position = ftransform();\n") +
                            "mat4 scalemat = mat4(  1.0,  0.0,  0.0,  0.0,\n" +
                                                "   0.0, -1.0,  0.0,  0.0,\n" +
                                                "   0.0,  0.0,  1.0,  0.0,\n" +
                                                "   0.0,  0.0,  0.0,  1.0);\n" +
                            "UVProjection = scalemat * gl_Position;\n" +
                            "gl_TexCoord[0]  = gl_MultiTexCoord0;\n" +
                        "}\n";
                    }
					break;
				}
		    }
		    break;

			case NM_VERTEX:
		    {
				switch (Options.SM)
				{
				    case SM_HLSL: case SM_CG:
					{
						VertexProgramData +=
						Ogre::String(
						"void main_vp(\n") +
						       // IN
                          	   "float4 iPosition         : POSITION,\n" +
							   "float3 iNormal           : NORMAL,\n"+
                           	   // OUT
                           	   "out float4 oPosition     : POSITION,\n" +
                               "out float4 oPosition_    : TEXCOORD0,\n" +
                               "out float4 oUvProjection : TEXCOORD1,\n" +
							   "out float3 oNormal       : TEXCOORD2,\n";
						       if (cFoam)
							   {
							       VertexProgramData += "out float4 oWorldPosition : TEXCOORD3,\n uniform float4x4         uWorld,\n";
							   }
					    VertexProgramData +=
				        Ogre::String(
                               // UNIFORM
                               "uniform float4x4         uWorldViewProj)\n") +
               	        "{\n" +
                  	        "oPosition_  = iPosition;\n";
							if (cFoam)
							{
							    VertexProgramData += "oWorldPosition = mul(uWorld, iPosition);\n";
							}
						VertexProgramData +=
				        Ogre::String(
	              	        "oPosition = mul(uWorldViewProj, iPosition);\n") +
	               	        // Projective texture coordinates, adjust for mapping
	                	    "float4x4 scalemat = float4x4(0.5,   0,   0, 0.5,"+
	                                              	     "0,-0.5,   0, 0.5,"+
	                							  	     "0,   0, 0.5, 0.5,"+
	                							  	     "0,   0,   0,   1);\n" +
	               	        "oUvProjection = mul(scalemat, oPosition);\n" +
							"oNormal = normalize(iNormal);\n"+
               	         "}\n";
					}
					break;

					case SM_GLSL:
					{
						VertexProgramData += Ogre::String( "\n");
                        // UNIFORMS
                        if(cFoam)
                        {
                            VertexProgramData += "uniform mat4 uWorld;\n";
                        }
                        // IN
                        // OUT
                        VertexProgramData += Ogre::String(
                        "varying vec4 Position_;\n") +
                        "varying vec4 UVProjection;\n" +
                        "varying vec3 Normal;\n";
                        if(cFoam)
                        {
                            VertexProgramData += "varying vec4 WorldPosition;\n";
                        }
                        // PROGRAM
                        VertexProgramData += Ogre::String(
                        "void main()\n") +
                        "{\n" +
                            "Position_ = gl_Vertex;\n";
                            if(cFoam)
                            {
                                VertexProgramData += "WorldPosition = uWorld * gl_Vertex;\n";
                            }
                            VertexProgramData += Ogre::String(
                            "gl_Position = ftransform();\n") +
                            "mat4 scalemat = mat4(  1.0,  0.0,  0.0,  0.0,\n" +
                                                "   0.0, -1.0,  0.0,  0.0,\n" +
                                                "   0.0,  0.0,  1.0,  0.0,\n" +
                                                "   0.0,  0.0,  0.0,  1.0);\n" +
                            "UVProjection = scalemat * gl_Position;\n" +
                            "Normal = normalize(gl_Normal);\n" +
                        "}\n";
                    }
					break;
				}
		    }
		    break;

			case NM_RTT:
		    {
				switch (Options.SM)
				{
				    case SM_HLSL: case SM_CG:
					{
						VertexProgramData +=
						Ogre::String(
						"void main_vp(\n") +
						       // IN
                          	   "float4 iPosition         : POSITION,\n" +
                           	   // OUT
                           	   "out float4 oPosition     : POSITION,\n" +
                               "out float4 oPosition_    : TEXCOORD0,\n" +
                               "out float4 oUvProjection : TEXCOORD1,\n";
						       if (cFoam)
							   {
							       VertexProgramData += "out float4 oWorldPosition : TEXCOORD2,\n uniform float4x4         uWorld,\n";
							   }
					    VertexProgramData +=
				        Ogre::String(
                               // UNIFORM
                               "uniform float4x4         uWorldViewProj)\n") +
               	        "{\n" +
                  	        "oPosition_  = iPosition;\n";
							if (cFoam)
							{
							    VertexProgramData += "oWorldPosition = mul(uWorld, iPosition);\n";
							}
						VertexProgramData +=
				        Ogre::String(
	              	        "oPosition = mul(uWorldViewProj, iPosition);\n") +
	               	        // Projective texture coordinates, adjust for mapping
	                	    "float4x4 scalemat = float4x4(0.5,   0,   0, 0.5,"+
	                                              	     "0,-0.5,   0, 0.5,"+
	                							  	     "0,   0, 0.5, 0.5,"+
	                							  	     "0,   0,   0,   1);\n" +
	               	        "oUvProjection = mul(scalemat, oPosition);\n" +
               	         "}\n";
					}
					break;

					case SM_GLSL:
					{
						VertexProgramData += Ogre::String( "\n");
                        // UNIFORMS
                        if(cFoam)
                        {
                            VertexProgramData += "uniform mat4 uWorld;\n";
                        }
                        // IN
                        // OUT
                        VertexProgramData += Ogre::String(
                        "varying vec4 Position_;\n") +
                        "varying vec4 UVProjection;\n";
                        if(cFoam)
                        {
                            VertexProgramData += "varying vec4 WorldPosition;\n";
                        }
                        // PROGRAM
                        VertexProgramData +=Ogre::String(
                        "void main()\n") +
                        "{\n" +
                            "Position_ = gl_Vertex;\n";
                            if(cFoam)
                            {
                                VertexProgramData += "WorldPosition = uWorld * gl_Vertex;\n";
                            }
                            VertexProgramData += Ogre::String(
                            "gl_Position = ftransform();\n") +
                            "mat4 scalemat = mat4(  1.0,  0.0,  0.0,  0.0,\n" +
                                                "   0.0, -1.0,  0.0,  0.0,\n" +
                                                "   0.0,  0.0,  1.0,  0.0,\n" +
                                                "   0.0,  0.0,  0.0,  1.0);\n" +
                            "UVProjection = scalemat * gl_Position;\n" +
                        "}\n";
                    }
					break;
				}
			}
		    break;
		}


		// Fragment program

		switch (Options.NM)
		{
		    case NM_TEXTURE: case NM_VERTEX: case NM_RTT:
		    {
				switch (Options.SM)
				{
				    case SM_HLSL: case SM_CG:
					{
						FragmentProgramData +=
							Ogre::String("float3 expand(float3 v)\n") +
						    "{\n" +
	                            "return (v - 0.5) * 2;\n" +
							"}\n\n" +

							"void main_fp(" +
							    // IN
                                "float4 iPosition      : TEXCOORD0,\n";
						int TEXCOORDNUM = 1;
						if (Options.NM == NM_TEXTURE)
						{
							FragmentProgramData +=
								"float2 iUvNoise      : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
							TEXCOORDNUM++;
						}
						FragmentProgramData +=
                                "float4 iUvProjection : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
						TEXCOORDNUM++;
						if (Options.NM == NM_VERTEX)
						{
							FragmentProgramData +=
							    "float4 iNormal       : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
							TEXCOORDNUM++;
						}
						if (cFoam)
						{
							FragmentProgramData +=
							    "float4 iWorldPosition  : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
						}
						FragmentProgramData +=
							Ogre::String(
	                            // OUT
	                            "out float4 oColor    : COLOR,\n") +
	                            // UNIFORM
	                            "uniform float3       uEyePosition,\n" +
	                            "uniform float        uFullReflectionDistance,\n" +
	                            "uniform float        uGlobalTransparency,\n" +
	                            "uniform float        uNormalDistortion,\n";

						if (cDepth)
						{
							FragmentProgramData +=
								"uniform float3       uWaterColor,\n";
						}
						if (cSmooth)
						{
							FragmentProgramData +=
								"uniform float        uSmoothPower,\n";
						}
						if (cSun)
						{
							FragmentProgramData += Ogre::String(
							    "uniform float3       uSunPosition,\n") +
	                            "uniform float        uSunStrength,\n" +
	                            "uniform float        uSunArea,\n" +
	                            "uniform float3       uSunColor,\n";
						}
						if (cFoam)
						{
							FragmentProgramData += Ogre::String(
							    "uniform float        uFoamRange,\n") +
							    "uniform float        uFoamMaxDistance,\n" +
	                            "uniform float        uFoamScale,\n" +
	                            "uniform float        uFoamStart,\n" +
	                            "uniform float        uFoamTransparency,\n";
						}
						if (cCaustics)
						{
							FragmentProgramData +=
								"uniform float        uCausticsPower,\n";
						}

						int TexNum = 0;

						if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
						{
						    FragmentProgramData +=
							   "uniform sampler2D    uNormalMap       : register(s" + Ogre::StringConverter::toString(TexNum) + "),\n";
							TexNum++;
						}

						FragmentProgramData +=
							Ogre::String(
						       "uniform sampler2D    uReflectionMap   : register(s" + Ogre::StringConverter::toString(TexNum) + "),\n") +
	                           "uniform sampler2D    uRefractionMap   : register(s" + Ogre::StringConverter::toString(TexNum+1) + "),\n";

						TexNum += 2;

						if (cDepth)
						{
							FragmentProgramData +=
								"uniform sampler2D    uDepthMap        : register(s" + Ogre::StringConverter::toString(TexNum) + "),\n";
							TexNum++;
						}

						FragmentProgramData +=
                                #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                                    "uniform sampler1D    uFresnelMap      : register(s" + Ogre::StringConverter::toString(TexNum) + ")";
								#else
                                    "uniform sampler2D    uFresnelMap      : register(s" + Ogre::StringConverter::toString(TexNum) + ")";
                                #endif
						TexNum++;

						if (cFoam)
						{
							FragmentProgramData += Ogre::String(
							 ",\nuniform sampler2D    uFoamMap         : register(s" + Ogre::StringConverter::toString(TexNum) + ")\n");
						}

						FragmentProgramData +=
							Ogre::String(                                            ")\n") +
							"{\n"     +
							    "float2 ProjectionCoord = iUvProjection.xy / iUvProjection.w;\n" +
                                "float3 camToSurface = iPosition.xyz - uEyePosition;\n" +
                                "float additionalReflection=camToSurface.x*camToSurface.x+camToSurface.z*camToSurface.z;\n";

						if (cFoam)
						{
							// Calculate the foam visibility as a function fo distance specified by user
							FragmentProgramData +=
								"float foamVisibility=1.0f-saturate(additionalReflection/uFoamMaxDistance);\n";
						}

						FragmentProgramData +=
							Ogre::String(
							    "additionalReflection/=uFullReflectionDistance;\n") +
								"camToSurface=normalize(-camToSurface);\n";
						if (Options.NM == NM_TEXTURE)
						{
							FragmentProgramData += Ogre::String(
								"float3 pixelNormal = tex2D(uNormalMap,iUvNoise);\n") +
								// Inverte y with z, because at creation our local normal to the plane was z
								"pixelNormal.yz=pixelNormal.zy;\n" +
								// Remap from [0,1] to [-1,1]
								"pixelNormal.xyz=expand(pixelNormal.xyz);\n";
						}
						else if (Options.NM == NM_VERTEX)
						{
							FragmentProgramData +=
								"float3 pixelNormal = iNormal;\n";
						}
						else // NM_RTT
						{
							FragmentProgramData +=
								"float3 pixelNormal = 2.0*tex2D(uNormalMap, ProjectionCoord.xy) - 1.0;\n";
						}
						FragmentProgramData +=
								"float2 pixelNormalModified = uNormalDistortion*pixelNormal.zx;\n";
						if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
						{
							FragmentProgramData +=
								"float dotProduct=dot(camToSurface,pixelNormal);\n";
						}
						else
						{
							FragmentProgramData +=
								"float dotProduct=dot(-camToSurface,pixelNormal);\n";
						}
						FragmentProgramData +=
							Ogre::String(
								"dotProduct=saturate(dotProduct);\n") +
								#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                                    "float fresnel = tex1D(uFresnelMap,dotProduct);\n" +
                                #else
                                    "float fresnel = tex2D(uFresnelMap,float2(dotProduct,dotProduct));\n" +
                                #endif
								// Add additional reflection and saturate
								"fresnel+=additionalReflection;\n" +
								"fresnel=saturate(fresnel);\n" +
								// Decrease the transparency and saturate
								"fresnel-=uGlobalTransparency;\n" +
                                "fresnel=saturate(fresnel);\n" +
                                #if OGRE_PLATFORM != OGRE_PLATFORM_WIN32
                                    // Reversing projection if underwater
                                    "if(uEyePosition.y < 0.0)\n" +
                                    "{\n" +
                                        "ProjectionCoord.y = 1.0 - ProjectionCoord.y;\n" +
                                    "}\n" +
                                #endif
								// Get the reflection/refraction pixels. Make sure to disturb the texcoords by pixelnormal
								"float3 reflection=tex2D(uReflectionMap,ProjectionCoord.xy+pixelNormalModified);\n" +
								"float3 refraction=tex2D(uRefractionMap,ProjectionCoord.xy-pixelNormalModified);\n";

						if (cDepth)
						{
							if (cCaustics)
						    {
								FragmentProgramData += Ogre::String(
								"float2 depth = tex2D(uDepthMap,ProjectionCoord.xy-pixelNormalModified).rg;\n") +
								"refraction *= 1+depth.y*uCausticsPower;\n" +
								"refraction = lerp(uWaterColor,refraction,depth.x);\n";
						    }
							else
							{
								FragmentProgramData += Ogre::String(
								"float depth = tex2D(uDepthMap,ProjectionCoord.xy-pixelNormalModified).r;\n") +
								"refraction = lerp(uWaterColor,refraction,depth);\n";
							}
						}

						FragmentProgramData +=
								"oColor = float4(lerp(refraction,reflection,fresnel),1);\n";

						if (cSun)
						{
							FragmentProgramData += Ogre::String(
							    "float3 relfectedVector = normalize(reflect(-camToSurface,pixelNormal.xyz));\n") +
								"float3 surfaceToSun=normalize(uSunPosition-iPosition.xyz);\n" +
								"float3 sunlight = uSunStrength*pow(saturate(dot(relfectedVector,surfaceToSun)),uSunArea)*uSunColor;\n" +
								"oColor.xyz+=sunlight;\n";
						}

						if (cFoam)
						{
							FragmentProgramData += Ogre::String(
							    "float hmap = iPosition.y/uFoamRange*foamVisibility;\n") +
								"float2 foamTex=iWorldPosition.xz*uFoamScale+pixelNormalModified;\n" +
								"float foam=tex2D(uFoamMap,foamTex).r;\n" +
								"float foamTransparency=saturate(hmap-uFoamStart)*uFoamTransparency;\n" +
								"oColor.xyz=lerp(oColor.xyz,1,foamTransparency*foam);\n";
						}

						if (cSmooth)
						{
							FragmentProgramData +=
								"oColor.xyz = lerp(tex2D(uRefractionMap,ProjectionCoord.xy).xyz,oColor.xyz,saturate((1-tex2D(uDepthMap,ProjectionCoord.xy).r)*uSmoothPower));\n";
						}

						FragmentProgramData +=
							"}\n";
					}
					break;

					case SM_GLSL:
						FragmentProgramData += Ogre::String("\n") +
                            // UNIFORMS
                            "uniform vec3  uEyePosition;\n" +
                            "uniform float uFullReflectionDistance;\n" +
                            "uniform float uGlobalTransparency;\n" +
                            "uniform float uNormalDistortion;\n" +
                            "uniform vec3  uWaterColor;\n";

                            if (cSmooth)
                            {
                                FragmentProgramData +=
                                "uniform float uSmoothPower;\n";
                            }
                            if (cSun)
                            {
                                FragmentProgramData += Ogre::String(
                                "uniform vec3  uSunPosition;\n") +
                                "uniform float uSunStrength;\n" +
                                "uniform float uSunArea;\n" +
                                "uniform vec3  uSunColor;\n";
                            }
                            if (cFoam)
                            {
                                FragmentProgramData += Ogre::String(
							    "uniform float uFoamRange;\n") +
							    "uniform float uFoamMaxDistance;\n" +
	                            "uniform float uFoamScale;\n" +
	                            "uniform float uFoamStart;\n" +
	                            "uniform float uFoamTransparency;\n";
                            }
                            if (cCaustics)
                            {
                                FragmentProgramData +=
								"uniform float uCausticsPower;\n";
                            }

                            int TexNum = 0;
                            if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
                            {
                                FragmentProgramData +=
                               "uniform sampler2D uNormalMap;\n";
                                TexNum++;
                            }

                            FragmentProgramData += Ogre::String(
						       "uniform sampler2D uReflectionMap;\n") +
	                           "uniform sampler2D uRefractionMap;\n";
                            TexNum += 2;

                            if (cDepth)
                            {
                                FragmentProgramData +=
                                "uniform sampler2D uDepthMap;\n";
                                TexNum++;
                            }

                            FragmentProgramData +=
                            #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                                "uniform sampler1D uFresnelMap;\n";
                            #else
                                "uniform sampler2D uFresnelMap;\n";
                            #endif
                            TexNum++;

                            if (cFoam)
                            {
                                FragmentProgramData +=
                                "uniform sampler2D uFoamMap;\n";
                            }
                            // IN
                            FragmentProgramData +=
                                "varying vec4 Position_;\n";
                            int TEXCOORDNUM = 1;
                            if (Options.NM == NM_TEXTURE)
                            {
                                TEXCOORDNUM++;
                            }
                            FragmentProgramData +=
                                "varying vec4 UVProjection;\n";
                            TEXCOORDNUM++;
                            if (Options.NM == NM_VERTEX)
                            {
                                FragmentProgramData +=
                                "varying vec3 Normal;\n";
                                TEXCOORDNUM++;
                            }
                            if (cFoam)
                            {
                                FragmentProgramData +=
                                "varying vec4 WorldPosition;\n";
                            }
                            // Expand function
                            FragmentProgramData += Ogre::String(
							"vec3 expand(vec3 v)\n") +
						    "{\n" +
	                            "return (v - 0.5) * 2.0;\n" +
							"}\n\n" +
                            // main function
							"void main()\n" +
						    "{\n" +
							    "vec2 ProjectionCoord = UVProjection.xy / UVProjection.w;\n" +
							    "ProjectionCoord += 1.0;\n" +
							    "ProjectionCoord *= 0.5;\n" +
                                "vec3 camToSurface = Position_.xyz - uEyePosition;\n" +
                                "float additionalReflection=camToSurface.x*camToSurface.x+camToSurface.z*camToSurface.z;\n";
                                if (cFoam)
                                {
                                    // Calculate the foam visibility as a function fo distance specified by user
                                    FragmentProgramData +=
                                    "float foamVisibility=1.0-clamp(additionalReflection/uFoamMaxDistance, 0.0, 1.0);\n";
                                }
                                FragmentProgramData += Ogre::String(
                                "additionalReflection/=uFullReflectionDistance;\n") +
                                "camToSurface=normalize(-camToSurface);\n";
                                if (Options.NM == NM_TEXTURE)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "vec3 pixelNormal = texture2D(uNormalMap,gl_TexCoord[0].xy).xyz;\n") +
                                    // Inverte y with z, because at creation our local normal to the plane was z
                                    "pixelNormal.yz=pixelNormal.zy;\n" +
                                    // Remap from [0,1] to [-1,1]
                                    "pixelNormal.xyz=expand(pixelNormal.xyz);\n";
                                }
                                else if (Options.NM == NM_VERTEX)
                                {
                                    FragmentProgramData +=
                                    "vec3 pixelNormal = Normal;\n";
                                }
                                else // NM_RTT
                                {
                                    FragmentProgramData +=
                                    "vec3 pixelNormal = 2.0*texture2D(uNormalMap, ProjectionCoord.xy).xyz - 1.0;\n";
                                }
                                FragmentProgramData +=
                                "vec2 pixelNormalModified = uNormalDistortion*pixelNormal.zx;\n";
                                if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
                                {
                                    FragmentProgramData +=
                                    "float dotProduct=dot(camToSurface,pixelNormal);\n";
                                }
                                else
                                {
                                    FragmentProgramData +=
                                    "float dotProduct=dot(-camToSurface,pixelNormal);\n";
                                }
                                FragmentProgramData += Ogre::String(
								"dotProduct=clamp(dotProduct, 0.0, 1.0);\n") +
                                #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                                    "float fresnel = texture1D(uFresnelMap,dotProduct).x;\n" +
                                #else
                                    "float fresnel = texture2D(uFresnelMap,vec2(dotProduct,dotProduct)).x;\n" +
                                #endif
								// Add additional reflection and saturate
								"fresnel +=additionalReflection;\n" +
								"fresnel = clamp(fresnel, 0.0, 1.0);\n" +
								// Decrease the transparency and saturate
								"fresnel -= uGlobalTransparency;\n" +
                                "fresnel =  clamp(fresnel*fresnel, 0.0, 1.0);\n" +
								// Get the reflection/refraction pixels. Make sure to disturb the texcoords by pixelnormal
								"vec3 reflection=texture2D(uReflectionMap,ProjectionCoord.xy+pixelNormalModified).xyz;\n" +
								"vec3 refraction=texture2D(uRefractionMap,ProjectionCoord.xy-pixelNormalModified).xyz;\n";
                                if (cDepth)
                                {
                                    if (cCaustics)
                                    {
                                        FragmentProgramData += Ogre::String(
                                        "vec2 depth = texture2D(uDepthMap,ProjectionCoord.xy-pixelNormalModified).xy;\n") +
                                        "refraction *= 1.0 + depth.y*uCausticsPower;\n" +
                                        "refraction = mix(uWaterColor,refraction,depth.x);\n";
                                    }
                                    else
                                    {
                                        FragmentProgramData += Ogre::String(
                                        "float depth = texture2D(uDepthMap,ProjectionCoord.xy-pixelNormalModified).x;\n") +
                                        "refraction = mix(uWaterColor,refraction,depth);\n";
                                    }
                                }
                                FragmentProgramData += Ogre::String(
								"gl_FragColor = vec4(mix(refraction,reflection,fresnel),1.0);\n") +
								"gl_FragColor.xyz = mix(gl_FragColor.xyz, uWaterColor, uGlobalTransparency);\n";
                                if (cSun)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "vec3 relfectedVector = normalize(reflect(-camToSurface,pixelNormal.xyz));\n") +
                                    "vec3 surfaceToSun=normalize(uSunPosition-Position_.xyz);\n" +
                                    "vec3 sunlight = uSunStrength*pow(clamp(dot(relfectedVector,surfaceToSun),0.0,1.0),uSunArea)*uSunColor;\n" +
                                    "gl_FragColor.xyz+=sunlight;\n";
                                }
                                if (cFoam)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "float hmap = Position_.y/uFoamRange*foamVisibility;\n") +
                                    "vec2 foamTex=WorldPosition.xz*uFoamScale+pixelNormalModified;\n" +
                                    "float foam=texture2D(uFoamMap,foamTex).x;\n" +
                                    "float foamTransparency=clamp(hmap-uFoamStart, 0.0, 1.0)*uFoamTransparency;\n" +
                                    "gl_FragColor.xyz=mix(gl_FragColor.xyz,vec3(1.0,1.0,1.0),foamTransparency*foam);\n";
                                }
                                if (cSmooth)
                                {
                                    FragmentProgramData +=
                                        "gl_FragColor.xyz = mix(texture2D(uRefractionMap,ProjectionCoord.xy).xyz,gl_FragColor.xyz,clamp((1.0-texture2D(uDepthMap,ProjectionCoord.xy).x)*uSmoothPower, 0.0, 1.0));\n";
                                }
                            FragmentProgramData +=
							"}\n";
					break;
				}
		    }
		    break;
		}

		// Build our material
		Ogre::MaterialPtr &WaterMaterial = getMaterial(MAT_WATER);
		WaterMaterial = Ogre::MaterialManager::getSingleton().
			create(_def_Water_Material_Name,
			       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Ogre::Pass *WM_Technique0_Pass0 = WaterMaterial->getTechnique(0)->getPass(0);

		WM_Technique0_Pass0->setCullingMode(Ogre::CULL_NONE);
		WM_Technique0_Pass0->setDepthWriteEnabled(true);

		Ogre::String GpuProgramsData[2] = {VertexProgramData, FragmentProgramData};
		Ogre::String GpuProgramNames[2] = {_def_Water_Shader_VP_Name, _def_Water_Shader_FP_Name};
        Ogre::String EntryPoints[2];
        if(Options.SM == SM_GLSL)
        {
            EntryPoints[0] = Ogre::String("main");
            EntryPoints[1] = Ogre::String("main");
        }
        else
        {
            EntryPoints[0] = Ogre::String("main_vp");
            EntryPoints[1] = Ogre::String("main_fp");
        }

		fillGpuProgramsToPass(WM_Technique0_Pass0, GpuProgramNames, Options.SM, EntryPoints, GpuProgramsData);

		Ogre::GpuProgramParametersSharedPtr VP_Parameters = WM_Technique0_Pass0->getVertexProgramParameters();
		Ogre::GpuProgramParametersSharedPtr FP_Parameters = WM_Technique0_Pass0->getFragmentProgramParameters();

        if(Options.SM != SM_GLSL)
        {
            VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
        }
		if (cFoam)
		{
		    VP_Parameters->setNamedAutoConstant("uWorld",     Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
		}
		FP_Parameters->setNamedAutoConstant("uEyePosition",   Ogre::GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);

		FP_Parameters->setNamedConstant("uFullReflectionDistance", mHydrax->getFullReflectionDistance());
		FP_Parameters->setNamedConstant("uGlobalTransparency",     mHydrax->getGlobalTransparency());
		FP_Parameters->setNamedConstant("uNormalDistortion",       mHydrax->getNormalDistortion());

        FP_Parameters->setNamedConstant("uWaterColor", mHydrax->getWaterColor());
		if (cSmooth)
		{
			FP_Parameters->setNamedConstant("uSmoothPower", mHydrax->getSmoothPower());
		}
		if (cSun)
		{
			FP_Parameters->setNamedConstant("uSunPosition", mHydrax->getMesh()->getObjectSpacePosition(mHydrax->getSunPosition()));
			FP_Parameters->setNamedConstant("uSunStrength", mHydrax->getSunStrength());
			FP_Parameters->setNamedConstant("uSunArea",     mHydrax->getSunArea());
			FP_Parameters->setNamedConstant("uSunColor",    mHydrax->getSunColor());
		}
		if (cFoam)
		{
			FP_Parameters->setNamedConstant("uFoamRange",        mHydrax->getMesh()->getOptions().MeshStrength);
			FP_Parameters->setNamedConstant("uFoamMaxDistance",  mHydrax->getFoamMaxDistance());
			FP_Parameters->setNamedConstant("uFoamScale",        mHydrax->getFoamScale());
			FP_Parameters->setNamedConstant("uFoamStart",        mHydrax->getFoamStart());
			FP_Parameters->setNamedConstant("uFoamTransparency", mHydrax->getFoamTransparency());
		}
		if (cCaustics)
		{
			FP_Parameters->setNamedConstant("uCausticsPower",    mHydrax->getCausticsPower());
		}

		int GLSLTextUnit = 0;
		if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
		{
            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uNormalMap", GLSLTextUnit);
                GLSLTextUnit++;
            }
		    WM_Technique0_Pass0->createTextureUnitState("HydraxNormalMap")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);
		}

        if(Options.SM == SM_GLSL)
        {
            FP_Parameters->setNamedConstant("uReflectionMap", GLSLTextUnit);
            GLSLTextUnit++;
        }
		WM_Technique0_Pass0->createTextureUnitState("HydraxReflectionMap")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
        if(Options.SM == SM_GLSL)
        {
            FP_Parameters->setNamedConstant("uRefractionMap", GLSLTextUnit);
            GLSLTextUnit++;
        }
		WM_Technique0_Pass0->createTextureUnitState("HydraxRefractionMap")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

		if (cDepth)
		{
            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uDepthMap", GLSLTextUnit);
                GLSLTextUnit++;
            }
			WM_Technique0_Pass0->createTextureUnitState("HydraxDepthMap")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
		}

        if(Options.SM == SM_GLSL)
        {
            FP_Parameters->setNamedConstant("uFresnelMap", GLSLTextUnit);
            GLSLTextUnit++;
        }
		WM_Technique0_Pass0->createTextureUnitState("Fresnel.bmp")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

		if (cFoam)
		{
            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uFoamMap", GLSLTextUnit);
                GLSLTextUnit++;
            }
			WM_Technique0_Pass0->createTextureUnitState("Foam.png")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);
		}

		WaterMaterial->setReceiveShadows(false);
		WaterMaterial->load();

		return true;
	}

	bool MaterialManager::_createDepthMaterial(const HydraxComponent &Components, const Options &Options)
	{
		const bool cCaustics   = _isComponent(Components, HYDRAX_COMPONENT_CAUSTICS);

		Ogre::String VertexProgramData, FragmentProgramData;

		// Vertex program

		switch (Options.SM)
		{
		    case SM_HLSL: case SM_CG:
			{
				// No caustics
				if (!cCaustics)
				{
				    VertexProgramData +=
					    Ogre::String(
					    "void main_vp(\n") +
					        // IN
					        "float4 iPosition         : POSITION,\n" +
						    // OUT
						    "out float4 oPosition     : POSITION,\n" +
						    "out float  oPosition_    : TEXCOORD0,\n" +
						    "out float2 oDistance     : TEXCOORD1,\n" +
						    // UNIFORM
						    "uniform float            uPlaneYPos,\n" +
                            "uniform float            uPlanesError,\n" +
						    "uniform float3           uCameraPos,\n" +
                            "uniform float4x4         uWorld,\n" +
                            "uniform float4x4         uWorldView,\n" +
                            "uniform float4x4         uWorldViewProj)\n" +
					    "{\n" +
                            "oPosition = mul(uWorldViewProj, iPosition);\n" +
                            "float3 wPos = mul(uWorld, iPosition).xyz;\n" +
                            "float3 wCam = uCameraPos;\n" +
                            "wPos.y    -= uPlaneYPos;\n" +
                            "wCam.y    -= uPlaneYPos;\n" +
                            "oPosition_ = wPos.y;\n" +
                            // Distance
                            "oDistance.x  = distance(wPos, wCam);\n" +
                            "oDistance.y  = 1.0;\n" +
                            // If exist water plane between points, occlusion effect must be changed.
                            "if(wCam.y > 0.0) {\n" +
                                "oDistance.x *= 1.0 - wCam.y/(wCam.y + abs(wPos.y));\n" +
                            "}\n" +
                            "if(wPos.y > uPlanesError) {\n" +
                                "oDistance.x *= abs(wCam.y)/(abs(wCam.y) + wPos.y);\n" +
                                "oDistance.y = 0.0;\n" +
                            "}\n" +
						"}\n";
				}
				else // Caustics
				{
					VertexProgramData +=
					    Ogre::String(
					    "void main_vp(\n") +
					        // IN
					        "float4 iPosition         : POSITION,\n" +
						    // OUT
						    "out float4 oPosition     : POSITION,\n" +
						    "out float  oPosition_    : TEXCOORD0,\n" +
						    "out float2 oDistance     : TEXCOORD1,\n" +
							"out float2 oUvWorld      : TEXCOORD2,\n" +
						    // UNIFORM
						    "uniform float            uPlaneYPos,\n" +
                            "uniform float            uPlanesError,\n" +
						    "uniform float3           uCameraPos,\n" +
                            "uniform float4x4         uWorld,\n" +
                            "uniform float4x4         uWorldViewProj)\n" +
					    "{\n" +
                            "oPosition = mul(uWorldViewProj, iPosition);\n" +
                            "float3 wPos = mul(uWorld, iPosition).xyz;\n" +
                            "float3 wCam = uCameraPos;\n" +
                            "wPos.y    -= uPlaneYPos;\n" +
                            "wCam.y    -= uPlaneYPos;\n" +
                            "oPosition_ = wPos.y;\n" +
                            // Distance
                            "oDistance.x  = distance(wPos, wCam);\n" +
                            "oDistance.y  = 1.0;\n" +
                            // If exist water plane between points, occlusion effect must be changed.
                            "if(wCam.y > 0.0) {\n" +
                                "oDistance.x *= 1.0 - wCam.y/(wCam.y + abs(wPos.y));\n" +
                            "}\n" +
                            "if(wPos.y > uPlanesError) {\n" +
                                "oDistance.x *= abs(wCam.y)/(abs(wCam.y) + wPos.y);\n" +
                                "oDistance.y = 0.0;\n" +
                            "}\n" +
                            "oUvWorld = wPos.xz;\n" +
						"}\n";
				}
			}
			break;

			case SM_GLSL:
				// No caustics
				if (!cCaustics)
				{
				    VertexProgramData += Ogre::String( "\n" ) +
                        // UNIFORMS
                        "uniform float uPlaneYPos;\n" +
                        "uniform float uPlanesError;\n" +
                        "uniform mat4  uWorld;\n" +
                        // IN
                        // OUT
                        "varying float Position_;\n" +
                        "varying vec2  Distance_;\n" +
                        // main function
					    "void main()\n" +
					    "{\n" +
                            // Point and camera position
						    "vec3 wPos =  ( uWorld * gl_Vertex ).xyz;\n" +
                            "vec3 wCam  = ( uWorld * gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0) ).xyz;\n" +
                            "wPos.y    -= uPlaneYPos;\n" +
                            "wCam.y    -= uPlaneYPos;\n" +
                            "Position_  = wPos.y;\n" +
                            // Distance
                            "vec4 mwPos = gl_ModelViewMatrix * gl_Vertex;\n" +
                            "Distance_.x  = abs(mwPos.z);\n" +
                            "Distance_.y  = 1.0;\n" +
                            // If exist water plane between points, occlusion effect must be changed.
                            "if(wCam.y > 0.0) {\n" +
                                "Distance_.x *= 1.0 - wCam.y/(wCam.y + abs(wPos.y));\n" +
                            "}\n" +
                            "if(wPos.y > uPlanesError) {\n" +
                                "Distance_.x *= abs(wCam.y)/(abs(wCam.y) + wPos.y);\n" +
                                "Distance_.y = 0.0;\n" +
                            "}\n" +
						"}\n";
				}
				else // Caustics
				{
				    VertexProgramData += Ogre::String( "\n" ) +
                        // UNIFORMS
                        "uniform float uPlaneYPos;\n" +
                        "uniform float uPlanesError;\n" +
                        "uniform mat4  uWorld;\n" +
                        // IN
                        // OUT
                        "varying float Position_;\n" +
                        "varying vec2  Distance_;\n" +
                        "varying vec2  UVWorld;\n" +
                        // main function
					    "void main()\n" +
					    "{\n" +
                            "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n" +
                            // Point and camera position
						    "vec3 wPos =  ( uWorld * gl_Vertex ).xyz;\n" +
                            "vec3 wCam  = ( uWorld * gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0) ).xyz;\n" +
                            "wPos.y    -= uPlaneYPos;\n" +
                            "wCam.y    -= uPlaneYPos;\n" +
                            "Position_  = wPos.y;\n" +
                            // Distance
                            "vec4 mwPos = gl_ModelViewMatrix * gl_Vertex;\n" +
                            "Distance_.x  = abs(mwPos.z);\n" +
                            "Distance_.y  = 1.0;\n" +
                            // If exist water plane between points, occlusion effect must be changed.
                            "if(wCam.y > 0.0) {\n" +
                                "Distance_.x *= 1.0 - wCam.y/(wCam.y + abs(wPos.y));\n" +
                            "}\n" +
                            "if(wPos.y > uPlanesError) {\n" +
                                /*
                                "vec3 wFocus = ( uWorld * gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.1, 1.0) ).xyz;\n" +
                                "vec3 CamToFreeSurface = wFocus - wCam;\n" +
                                "CamToFreeSurface *= wCam.y / CamToFreeSurface.y;\n" +
                                "Distance_.x = length(CamToFreeSurface);\n" +
                                */
                                "Distance_.x *= abs(wCam.y)/(abs(wCam.y) + wPos.y);\n" +
                                "Distance_.y = 0.0;\n" +
                            "}\n" +
                            "UVWorld    = wPos.xz;\n" +
						"}\n";
				}
			break;
		}

		// Fragment program
		switch (Options.SM)
		{
		    case SM_HLSL: case SM_CG:
			{
				// No caustics
				if (!cCaustics)
				{
					FragmentProgramData +=
						Ogre::String(
						"void main_fp(\n") +
						    // IN
						    "float  iPosition     : TEXCOORD0,\n" +
						    "float2 iDistance     : TEXCOORD1,\n" +
							// OUT
							"out float4 oColor    : COLOR,\n" +
							// UNIFORM
							"uniform float        uDepthLimit,\n" +
							"uniform float        uDistLimit)\n" +
						"{\n" +
						    "float pixelYDepth = saturate(iPosition*uDepthLimit + 1.0);\n" +
						    "float pixelDepth  = saturate(1.0 - iDistance.x*uDistLimit);\n" +
							"pixelDepth = min(pixelYDepth, pixelDepth);\n" +
							"oColor = float4(pixelDepth,0,0,0);\n" +
						"}\n";
				}
				else // Caustics
				{
					FragmentProgramData +=
						Ogre::String(
						"void main_fp(\n") +
						    // IN
						    "float  iPosition     : TEXCOORD0,\n" +
						    "float2 iDistance     : TEXCOORD1,\n" +
                            "float2 iUvWorld      : TEXCOORD2,\n" +
							// OUT
							"out float4 oColor    : COLOR,\n" +
							// UNIFORM
							"uniform float        uDepthLimit,\n" +
							"uniform float        uDistLimit,\n" +
							"uniform float        uCausticsScale,\n" +
	                        "uniform float        uCausticsEnd,\n" +
							"uniform sampler2D    uCaustics : register(s0))\n" +
						"{\n" +
						    "float pixelYDepth = saturate(iPosition*uDepthLimit + 1.0);\n" +
						    // "float pixelDepth  = saturate(1.0 - iDistance.x*uDistLimit);\n" +
						    "float pixelDepth  = saturate(1.0 - iDistance.x*uDistLimit);\n" +
							"pixelDepth = min(pixelYDepth, pixelDepth);\n" +
							"oColor = float4(pixelDepth,0.0,0.0,0.0);\n" +
							"oColor.g = iDistance.y*saturate((pixelYDepth-uCausticsEnd)/(1.0-uCausticsEnd))*tex2D(uCaustics, iUvWorld/uCausticsScale).x;\n" +
						"}\n";
				}
			}
			break;

			case SM_GLSL:
				// No caustics
				if (!cCaustics)
				{
					FragmentProgramData += Ogre::String( "\n" ) +
                        // UNIFORMS
                        "uniform float uDepthLimit;\n" +
                        "uniform float uDistLimit;\n" +
                        // IN
                        "varying float Position_;\n" +
                        "varying float Distance_;\n" +
                        // OUT
                        // main function
                        "void main()" +
						"{\n" +
						    "float pixelYDepth = clamp(1.0 + Position_*uDepthLimit, 0.0, 1.0);\n" +
						    "float pixelDepth  = clamp(1.0 - Distance_.x*uDistLimit, 0.0, 1.0);\n" +
							"pixelDepth        = min(pixelYDepth,pixelDepth);\n" +
							"gl_FragColor      = vec4(pixelDepth,0.0,0.0,1.0);\n" +
						"}\n";
				}
				else // Caustics
				{
					FragmentProgramData += Ogre::String( "\n" ) +
                        // UNIFORMS
                        "uniform float     uDepthLimit;\n" +
                        "uniform float     uDistLimit;\n" +
                        "uniform float     uCausticsScale;\n" +
                        "uniform float     uCausticsEnd;\n" +
                        "uniform sampler2D uCaustics;\n" +
                        // IN
                        "varying float Position_;\n" +
                        "varying vec2  Distance_;\n" +
                        "varying vec2  UVWorld;\n" +
                        // OUT
                        // main function
                        "void main()" +
						"{\n" +
						    "float pixelYDepth = clamp(1.0 + Position_*uDepthLimit, 0.0, 1.0);\n" +
						    "float pixelDepth  = clamp(1.0 - Distance_.x*uDistLimit, 0.0, 1.0);\n" +
							"pixelDepth        = min(pixelYDepth,pixelDepth);\n" +
							"gl_FragColor      = vec4(pixelDepth,0.0,0.0,1.0);\n" +
							"gl_FragColor.y    = Distance_.y*clamp((pixelYDepth-uCausticsEnd)/(1.0-uCausticsEnd), 0.0, 1.0)*texture2D(uCaustics, UVWorld/uCausticsScale).x;\n" +
						"}\n";
				}
			break;
		}

		// Build our material
		Ogre::MaterialPtr &DepthMaterial = getMaterial(MAT_DEPTH);
		DepthMaterial = Ogre::MaterialManager::getSingleton().
			create(_def_Depth_Material_Name,
			       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		DepthMaterial->getTechnique(0)->setSchemeName("HydraxDepth");

		Ogre::Pass *DM_Technique0_Pass0 = DepthMaterial->getTechnique(0)->getPass(0);

		Ogre::String GpuProgramsData[2] = {VertexProgramData, FragmentProgramData};
		Ogre::String GpuProgramNames[2] = {_def_Depth_Shader_VP_Name, _def_Depth_Shader_FP_Name};
        Ogre::String EntryPoints[2];
        if(Options.SM == SM_GLSL)
        {
            EntryPoints[0] = Ogre::String("main");
            EntryPoints[1] = Ogre::String("main");
        }
        else
        {
            EntryPoints[0] = Ogre::String("main_vp");
            EntryPoints[1] = Ogre::String("main_fp");
        }

		fillGpuProgramsToPass(DM_Technique0_Pass0, GpuProgramNames, Options.SM, EntryPoints, GpuProgramsData);

		Ogre::GpuProgramParametersSharedPtr VP_Parameters = DM_Technique0_Pass0->getVertexProgramParameters();
		Ogre::GpuProgramParametersSharedPtr FP_Parameters = DM_Technique0_Pass0->getFragmentProgramParameters();

        if(Options.SM != SM_GLSL)
        {
            VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
            // VP_Parameters->setNamedAutoConstant("uWorldView", Ogre::GpuProgramParameters::ACT_WORLDVIEW_MATRIX);
            VP_Parameters->setNamedAutoConstant("uCameraPos", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION);
        }
        VP_Parameters->setNamedAutoConstant("uWorld", Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
		VP_Parameters->setNamedConstant("uPlaneYPos", mHydrax->getPosition().y);
		VP_Parameters->setNamedConstant("uPlanesError", (float) mHydrax->getPlanesError());

		FP_Parameters->setNamedConstant("uDepthLimit", 1.f/mHydrax->getDepthLimit());
		FP_Parameters->setNamedConstant("uDistLimit", 1.f/mHydrax->getDistLimit());

		if (cCaustics)
		{
			FP_Parameters->setNamedConstant("uCausticsScale", mHydrax->getCausticsScale());
			FP_Parameters->setNamedConstant("uCausticsEnd",   1.f - mHydrax->getCausticsEnd());

            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uCaustics", 0);
            }
			Ogre::TextureUnitState *TUS_Caustics = DM_Technique0_Pass0->createTextureUnitState("Caustics.bmp");
			TUS_Caustics->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);
			TUS_Caustics->setAnimatedTextureName("Caustics.bmp", 32, 1.5);
		}

		DepthMaterial->setReceiveShadows(false);
		DepthMaterial->load();

		return true;
	}

	bool MaterialManager::_createDepthTextureGPUPrograms(const HydraxComponent &Components, const Options &Options, const Ogre::String& AlphaChannel)
	{
		const bool cCaustics   = _isComponent(Components, HYDRAX_COMPONENT_CAUSTICS);

		Ogre::String VertexProgramData, FragmentProgramData;

		// Vertex program

		switch (Options.SM)
		{
		    case SM_HLSL: case SM_CG:
			{
				// No caustics
				if (!cCaustics)
				{
				    VertexProgramData +=
					    Ogre::String(
					    "void main_vp(\n") +
					        // IN
					        "float4 iPosition         : POSITION,\n" +
							"float2 iUV               : TEXCOORD0,\n" +
						    // OUT
						    "out float4 oPosition     : POSITION,\n" +
						    "out float3 oPosition_UV  : TEXCOORD0,\n" +
						    // UNIFORM
						    "uniform float            uPlaneYPos,\n" +
                            "uniform float4x4         uWorld,\n" +
                            "uniform float4x4         uWorldViewProj)\n" +
					    "{\n" +
					       "oPosition = mul(uWorldViewProj, iPosition);\n" +
						   "oPosition_UV.x = mul(uWorld, iPosition).y;\n" +
						   "oPosition_UV.x-=uPlaneYPos;\n" +
						   "oPosition_UV.yz = iUV;\n" +
						"}\n";
				}
				else // Caustics
				{
					VertexProgramData +=
					    Ogre::String(
					    "void main_vp(\n") +
					        // IN
					        "float4 iPosition         : POSITION,\n" +
							"float2 iUV               : TEXCOORD0,\n" +
						    // OUT
						    "out float4 oPosition     : POSITION,\n" +
						    "out float3 oPosition_UV  : TEXCOORD0,\n" +
							"out float2 oUvWorld      : TEXCOORD1,\n" +
						    // UNIFORM
						    "uniform float            uPlaneYPos,\n" +
                            "uniform float4x4         uWorld,\n" +
                            "uniform float4x4         uWorldViewProj)\n" +
					    "{\n" +
					       "oPosition = mul(uWorldViewProj, iPosition);\n" +
						   "float3 wPos = mul(uWorld, iPosition);\n" +
						   "oPosition_UV.x = wPos.y;\n" +
						   "oPosition_UV.x-=uPlaneYPos;\n" +
						   "oPosition_UV.yz = iUV;\n" +
						   "oUvWorld = wPos.xz;\n" +
						"}\n";
				}
			}
			break;

			case SM_GLSL:
				// No caustics
				if (!cCaustics)
				{
				    VertexProgramData += Ogre::String( "\n" ) +
					    // UNIFORMS
                        "uniform float uPlaneYPos;\n" +
                        "uniform mat4  uWorld;\n" +
                        // IN
                        // OUT
                        "varying vec3 Position_UV;\n" +
                        // main function
                        "void main()\n" +
                        "{\n" +
                            "gl_Position    = gl_ModelViewProjectionMatrix * gl_Vertex;\n" +
                            "Position_UV.x  = (uWorld * gl_Vertex).y;\n" +
                            "Position_UV.x -= uPlaneYPos;\n" +
                            "Position_UV.yz = gl_MultiTexCoord0;\n" +
                        "}\n";
				}
				else // Caustics
				{
				    VertexProgramData += Ogre::String( "\n" ) +
					    // UNIFORMS
                        "uniform float uPlaneYPos;\n" +
                        "uniform mat4  uWorld;\n" +
                        // IN
                        // OUT
                        "varying vec3 Position_UV;\n" +
                        "varying vec2 UVWorld;\n" +
                        // main function
                        "void main()\n" +
                        "{\n" +
                            "gl_Position    = gl_ModelViewProjectionMatrix * gl_Vertex;\n" +
                            "vec3 wPos      = uWorld * iPosition;\n" +
                            "Position_UV.x  = wPos.y;\n" +
                            "Position_UV.x -= uPlaneYPos;\n" +
                            "Position_UV.yz = gl_MultiTexCoord0;\n" +
                            "UVWorld = wPos.xz;\n" +
                        "}\n";
				}
			break;
		}

		// Fragment program

		switch (Options.SM)
		{
		    case SM_HLSL: case SM_CG:
			{
				// No caustics
				if (!cCaustics)
				{
					FragmentProgramData +=
						Ogre::String(
						"void main_fp(\n") +
						    // IN
						    "float3 iPosition_UV  : TEXCOORD0,\n" +
							// OUT
							"out float4 oColor    : COLOR,\n" +
							// UNIFORM
							"uniform float        uDepthLimit,\n" +
							"uniform sampler2D    uAlphaTex : register(s0))\n" +
						"{\n" +
						    "float pixelYDepth = (iPosition_UV.x*uDepthLimit+1);\n" +
							"pixelYDepth = saturate(pixelYDepth);\n" +
							"oColor = float4(pixelYDepth,0,0,0);\n" +
							"oColor.a = tex2D(uAlphaTex, iPosition_UV.yz)."+AlphaChannel+";" +
						"}\n";
				}
				else // Caustics
				{
					FragmentProgramData +=
						Ogre::String(
						"void main_fp(\n") +
						    // IN
						    "float3 iPosition_UV  : TEXCOORD0,\n" +
                            "float2 iUvWorld      : TEXCOORD1,\n" +
							// OUT
							"out float4 oColor    : COLOR,\n" +
							// UNIFORM
							"uniform float        uDepthLimit,\n" +
							"uniform float        uCausticsScale,\n" +
	                        "uniform float        uCausticsEnd,\n" +
							"uniform sampler2D    uCaustics : register(s0),\n" +
							"uniform sampler2D    uAlphaTex : register(s1))\n" +
						"{\n" +
						    "float pixelYDepth = (iPosition_UV.x*uDepthLimit+1);\n" +
							"pixelYDepth = saturate(pixelYDepth);\n" +
							"oColor = float4(pixelYDepth,0,0,0);\n" +
							"oColor.g = saturate(uCausticsEnd-pixelYDepth)*tex2D(uCaustics, iUvWorld/uCausticsScale).r;\n" +
							"oColor.a = tex2D(uAlphaTex, iPosition_UV.yz)."+AlphaChannel+";" +
						"}\n";
				}
			}
			break;

			case SM_GLSL:
			{
                Ogre::String AlphaChannelGLSL = AlphaChannel;
                std::replace( AlphaChannelGLSL.begin(), AlphaChannelGLSL.end(), 'r', 'x' );
                std::replace( AlphaChannelGLSL.begin(), AlphaChannelGLSL.end(), 'g', 'y' );
                std::replace( AlphaChannelGLSL.begin(), AlphaChannelGLSL.end(), 'b', 'z' );
                std::replace( AlphaChannelGLSL.begin(), AlphaChannelGLSL.end(), 'a', 'w' );
				// No caustics
				if (!cCaustics)
				{
					FragmentProgramData += Ogre::String( "\n" ) +
						// UNIFORMS
                        "uniform float     uDepthLimit;\n" +
                        "uniform float     uDistLimit;\n" +
                        "uniform sampler2D uAlphaTex;\n" +
						// IN
						"variying vec3 Position_UV;\n" +
						// OUT
						// main function
						"void main()\n" +
						"{\n" +
						    "float pixelYDepth = clamp(Position_UV.x*uDepthLimit + 1.0, 0.0, 1.0);\n" +
							"gl_FragColor      = vec4(pixelYDepth,0.0,0.0,0.0);\n" +
							"gl_FragColor.w    = texture2D(uAlphaTex, iPosition_UV.yz)."+AlphaChannelGLSL+";" +
						"}\n";
				}
				else // Caustics
				{
					FragmentProgramData += Ogre::String( "\n" ) +
						// UNIFORMS
                        "uniform float     uDistLimit;\n" +
                        "uniform float     uCausticsScale;\n" +
                        "uniform float     uCausticsEnd;\n" +
                        "uniform sampler2D uCaustics;\n" +
                        "uniform sampler2D uAlphaTex;\n" +
						// IN
						"variying vec3 Position_UV;\n" +
						"variying vec2 UVWorld;\n" +
						// OUT
						// main function
						"void main()\n" +
						"{\n" +
						    "float pixelYDepth = clamp(Position_UV.x*uDepthLimit + 1.0, 0.0, 1.0);\n" +
							"gl_FragColor      = vec4(pixelYDepth,0.0,0.0,0.0);\n" +
							"gl_FragColor.y    = clamp(uCausticsEnd-pixelYDepth, 0.0, 1.0)*texture2D(uCaustics, UVWorld/uCausticsScale).x;\n" +
							"gl_FragColor.w    = texture2D(uAlphaTex, iPosition_UV.yz)."+AlphaChannelGLSL+";" +
						"}\n";
				}
			}
            break;
		}

		Ogre::String GpuProgramsData[2] = {VertexProgramData, FragmentProgramData};
		Ogre::String GpuProgramNames[2] = {_def_DepthTexture_Shader_VP_Name+AlphaChannel, _def_DepthTexture_Shader_FP_Name+AlphaChannel};
        Ogre::String EntryPoints[2];
        if(Options.SM == SM_GLSL)
        {
            EntryPoints[0] = Ogre::String("main");
            EntryPoints[1] = Ogre::String("main");
        }
        else
        {
            EntryPoints[0] = Ogre::String("main_vp");
            EntryPoints[1] = Ogre::String("main_fp");
        }

		GpuProgram GpuPrograms[2] = {GPUP_VERTEX, GPUP_FRAGMENT};

		for (int k = 0; k < 2; k++)
		{
			 if (!createGpuProgram(GpuProgramNames[k], Options.SM, GpuPrograms[k], EntryPoints[k], GpuProgramsData[k]))
			 {
				 return false;
			 }
		}

		return true;
	}

	bool MaterialManager::_createUnderwaterMaterial(const HydraxComponent &Components, const Options &Options)
	{
		const bool cDepth    = _isComponent(Components, HYDRAX_COMPONENT_DEPTH   );
		//const bool cSmooth   = _isComponent(Components, HYDRAX_COMPONENT_SMOOTH  );      // cSmooth uneeded underwater
		const bool cSun      = _isComponent(Components, HYDRAX_COMPONENT_SUN     );
		//const bool cFoam     = _isComponent(Components, HYDRAX_COMPONENT_FOAM    );
		const bool cCaustics = _isComponent(Components, HYDRAX_COMPONENT_CAUSTICS);
		const bool cUReflections = _isComponent(Components, HYDRAX_COMPONENT_UNDERWATER_REFLECTIONS);

		Ogre::String VertexProgramData, FragmentProgramData;

		// Vertex program

		switch (Options.NM)
		{
		    case NM_TEXTURE:
		    {
				switch (Options.SM)
				{
				    case SM_HLSL: case SM_CG:
					{
						VertexProgramData +=
						Ogre::String(
						"void main_vp(\n") +
                                // IN
                                "float4 iPosition         : POSITION,\n" +
                                "float2 iUv               : TEXCOORD0,\n" +
                                // OUT
                                "out float4 oPosition      : POSITION,\n" +
                                "out float4 oPosition_     : TEXCOORD0,\n" +
                                "out float2 oUvNoise       : TEXCOORD1,\n" +
                                "out float4 oUvProjection  : TEXCOORD2,\n" +
                                "out float2 oDistance      : TEXCOORD3,\n";
                                /* Foam is not visible underwater
                                if (cFoam)
                                {
                                    VertexProgramData +=
                                    "out float4 oWorldPosition : TEXCOORD4,\n";
                                }
                                */
                                VertexProgramData += Ogre::String(
                                // UNIFORM
                                "uniform float4x4         uWorldViewProj,\n") +
                                "uniform float4x4         uWorldView,\n" +
                                "uniform float4x4         uWorld,\n" +
                                "uniform float3           uCameraPos)\n" +
                        "{\n" +
                            "oPosition_  = iPosition;\n";
                            /* Foam is not visible underwater
                            if (cFoam)
                            {
                                VertexProgramData +=
                                "oWorldPosition = mul(uWorld, iPosition);\n";
                            }
                            */
                            VertexProgramData += Ogre::String(
                            "oPosition = mul(uWorldViewProj, iPosition);\n") +
                            // Projective texture coordinates, adjust for mapping
                            "float4x4 scalemat = float4x4(0.5,   0,   0, 0.5,"+
                                                         "0,-0.5,   0, 0.5,"+
                                                         "0,   0, 0.5, 0.5,"+
                                                         "0,   0,   0,   1);\n" +
                            "oUvProjection = mul(scalemat, oPosition);\n" +
                            "oUvNoise = iUv;\n" +
                            // Distance
                            "float4 mwPos = mul(uWorldView, iPosition);\n" +
                            "oDistance.x  = abs(mwPos.z);\n" +
                            "oDistance.y  = -mul( uWorld, float4(uCameraPos, 1.0) ).y;\n" +
                         "}\n";
					}
					break;

					case SM_GLSL:
					{
                        VertexProgramData += Ogre::String( "\n" );
                        // UNIFORMS
                        VertexProgramData += "uniform mat4 uWorld;\n";
                        // IN
                        // OUT
                        VertexProgramData += Ogre::String(
                        "varying vec4 Position_;\n") +
                        "varying vec4 UVProjection;\n" +
                        "varying vec2 Distance_;\n";
                        /* Foam is not visible underwater
                        if (cFoam)
                        {
                            VertexProgramData += "varying vec4 WorldPosition;\n";
                        }
                        */
                        // main function
                        VertexProgramData += Ogre::String(
                        "void main()\n") +
                        "{\n" +
                  	        "Position_  = gl_Vertex;\n";
                            /* Foam is not visible underwater
							if (cFoam)
							{
							    VertexProgramData += "WorldPosition = uWorld * gl_Vertex;\n";
							}
							*/
                            VertexProgramData += Ogre::String(
	               	        // Projective texture coordinates, adjust for mapping
                            "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n") +
                            "mat4 scalemat = mat4(  1.0,  0.0,  0.0,  0.0,\n" +
                                                "   0.0, -1.0,  0.0,  0.0,\n" +
                                                "   0.0,  0.0,  1.0,  0.0,\n" +
                                                "   0.0,  0.0,  0.0,  1.0);\n" +
                            "UVProjection = scalemat * gl_Position;\n" +
                            "gl_TexCoord[0]  = gl_MultiTexCoord0;\n" +
                            // Distance
                            "vec4 mwPos = gl_ModelViewMatrix * gl_Vertex;\n" +
                            "Distance_.x  = abs(mwPos.z);\n" +
                            "Distance_.y  = -( uWorld * gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0) ).y;\n" +
                        "}\n";
					}
					break;
				}
		    }
		    break;

			case NM_VERTEX:
		    {
				switch (Options.SM)
				{
				    case SM_HLSL: case SM_CG:
					{
						VertexProgramData +=
						Ogre::String(
						"void main_vp(\n") +
                                // IN
                                "float4 iPosition         : POSITION,\n" +
                                "float3 iNormal           : NORMAL,\n"+
                                // OUT
                                "out float4 oPosition     : POSITION,\n" +
                                "out float4 oPosition_    : TEXCOORD0,\n" +
                                "out float4 oUvProjection : TEXCOORD1,\n" +
                                "out float3 oNormal       : TEXCOORD2,\n" +
                                "out float2 oDistance     : TEXCOORD3,\n";
                                /* Foam is not visible underwater
                                if (cFoam)
                                {
                                    VertexProgramData +=
                                    "out float4 oWorldPosition : TEXCOORD4,\n";
                                }
                                */
                                VertexProgramData += Ogre::String(
                                // UNIFORM
                                "uniform float4x4         uWorldViewProj,\n") +
                                "uniform float4x4         uWorldView,\n" +
                                "uniform float4x4         uWorld,\n" +
                                "uniform float3           uCameraPos)\n" +
               	        "{\n" +
                  	        "oPosition_  = iPosition;\n";
                  	        /* Foam is not visible underwater
							if (cFoam)
							{
							    VertexProgramData += "oWorldPosition = mul(uWorld, iPosition);\n";
							}
							*/
                            VertexProgramData += Ogre::String(
	              	        "oPosition = mul(uWorldViewProj, iPosition);\n") +
	               	        // Projective texture coordinates, adjust for mapping
	                	    "float4x4 scalemat = float4x4(0.5,   0,   0, 0.5,"+
	                                              	     "0,-0.5,   0, 0.5,"+
	                							  	     "0,   0, 0.5, 0.5,"+
	                							  	     "0,   0,   0,   1);\n" +
	               	        "oUvProjection = mul(scalemat, oPosition);\n" +
							"oNormal = normalize(iNormal);\n"+
                            // Distance
                            "float4 mwPos = mul(uWorldView, iPosition);\n" +
                            "oDistance.x  = abs(mwPos.z);\n" +
                            "oDistance.y  = -mul( uWorld, float4(uCameraPos, 1.0) ).y;\n" +
               	         "}\n";
					}
					break;

					case SM_GLSL:
					{
						VertexProgramData += Ogre::String( "\n");
                        // UNIFORMS
                        VertexProgramData += "uniform mat4 uWorld;\n";
                        // IN
                        // OUT
                        VertexProgramData += Ogre::String(
                        "varying vec4 Position_;\n") +
                        "varying vec4 UVProjection;\n" +
                        "varying vec3 Normal;\n" +
                        "varying vec2 Distance_;\n";
                        /* Foam is not visible underwater
                        if(cFoam)
                        {
                            VertexProgramData += "varying vec4 WorldPosition;\n";
                        }
                        */
                        // PROGRAM
                        VertexProgramData += Ogre::String(
                        "void main()\n") +
                        "{\n" +
                            "Position_ = gl_Vertex;\n";
                            /* Foam is not visible underwater
                            if(cFoam)
                            {
                                VertexProgramData += "WorldPosition = uWorld * gl_Vertex;\n";
                            }
                            */
                            VertexProgramData += Ogre::String(
                            "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n") +
                            "mat4 scalemat = mat4(  1.0,  0.0,  0.0,  0.0,\n" +
                                                "   0.0, -1.0,  0.0,  0.0,\n" +
                                                "   0.0,  0.0,  1.0,  0.0,\n" +
                                                "   0.0,  0.0,  0.0,  1.0);\n" +
                            "UVProjection = scalemat * gl_Position;\n" +
                            "Normal = normalize(gl_Normal);\n" +
                            // Distance
                            "vec4 mwPos = gl_ModelViewMatrix * gl_Vertex;\n" +
                            "Distance_.x  = abs(mwPos.z);\n" +
                            "Distance_.y  = -( uWorld * gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0) ).y;\n" +
                        "}\n";
                    }
					break;
				}
		    }
		    break;

			case NM_RTT:
		    {
				switch (Options.SM)
				{
				    case SM_HLSL: case SM_CG:
					{
						VertexProgramData +=
						Ogre::String(
						"void main_vp(\n") +
                                // IN
                                "float4 iPosition         : POSITION,\n" +
                                // OUT
                                "out float4 oPosition     : POSITION,\n" +
                                "out float4 oPosition_    : TEXCOORD0,\n" +
                                "out float4 oUvProjection : TEXCOORD1,\n" +
                                "out float2 oDistance     : TEXCOORD2,\n";
                                /* Foam is not visible underwater
                                if (cFoam)
                                {
                                    VertexProgramData +=
                                    "out float4 oWorldPosition : TEXCOORD3,\n";
                                }
                                */
                                VertexProgramData += Ogre::String(
                                // UNIFORM
                                "uniform float4x4         uWorldViewProj,\n") +
                                "uniform float4x4         uWorldView,\n" +
                                "uniform float4x4         uWorld,\n" +
                                "uniform float3           uCameraPos)\n" +
               	        "{\n" +
                  	        "oPosition_ = iPosition;\n";
                  	        /* Foam is not visible underwater
							if (cFoam)
							{
							    VertexProgramData += "oWorldPosition = mul(uWorld, iPosition);\n";
							}
							*/
                            VertexProgramData += Ogre::String(
	              	        "oPosition = mul(uWorldViewProj, iPosition);\n") +
	               	        // Projective texture coordinates, adjust for mapping
	                	    "float4x4 scalemat = float4x4(0.5,   0,   0, 0.5,"+
	                                              	     "0,-0.5,   0, 0.5,"+
	                							  	     "0,   0, 0.5, 0.5,"+
	                							  	     "0,   0,   0,   1);\n" +
	               	        "oUvProjection = mul(scalemat, oPosition);\n" +
                            // Distance
                            "float4 mwPos = mul(uWorldView, iPosition);\n" +
                            "oDistance.x  = abs(mwPos.z);\n" +
                            "oDistance.y  = -mul( uWorld, float4(uCameraPos, 1.0) ).y;\n" +
               	         "}\n";
					}
					break;

					case SM_GLSL:
					{
						VertexProgramData += Ogre::String( "\n");
                        // UNIFORMS
                        VertexProgramData += "uniform mat4 uWorld;\n";
                        // IN
                        // OUT
                        VertexProgramData += Ogre::String(
                        "varying vec4 Position_;\n") +
                        "varying vec4 UVProjection;\n" +
                        "varying vec2 Distance_;\n";
                        /* Foam is not visible underwater
                        if(cFoam)
                        {
                            VertexProgramData += "varying vec4 WorldPosition;\n";
                        }
                        */
                        // PROGRAM
                        VertexProgramData +=Ogre::String(
                        "void main()\n") +
                        "{\n" +
                            "Position_ = gl_Vertex;\n";
                            /* Foam is not visible underwater
                            if(cFoam)
                            {
                                VertexProgramData += "WorldPosition = uWorld * gl_Vertex;\n";
                            }
                            */
                            VertexProgramData += Ogre::String(
                            "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n") +
                            "mat4 scalemat = mat4(  1.0,  0.0,  0.0,  0.0,\n" +
                                                "   0.0, -1.0,  0.0,  0.0,\n" +
                                                "   0.0,  0.0,  1.0,  0.0,\n" +
                                                "   0.0,  0.0,  0.0,  1.0);\n" +
                            "UVProjection = scalemat * gl_Position;\n" +
                            // Distance
                            "vec4 mwPos = gl_ModelViewMatrix * gl_Vertex;\n" +
                            "Distance_.x  = abs(mwPos.z);\n" +
                            "Distance_.y  = -( uWorld * gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0) ).y;\n" +
                        "}\n";
                    }
					break;
				}
			}
		    break;
		}

		// Fragment program

		switch (Options.NM)
		{
		    case NM_TEXTURE: case NM_VERTEX: case NM_RTT:
		    {
				switch (Options.SM)
				{
				    case SM_HLSL: case SM_CG:
					{
                            FragmentProgramData +=
							Ogre::String("float3 expand(float3 v)\n") +
						    "{\n" +
	                            "return (v - 0.5) * 2;\n" +
							"}\n\n" +

							"void main_fp(" +
							    // IN
                                "float4 iPosition     : TEXCOORD0,\n";
                                int TEXCOORDNUM = 1;
                                if (Options.NM == NM_TEXTURE)
                                {
                                    FragmentProgramData +=
                                    "float2 iUvNoise      : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
                                    TEXCOORDNUM++;
                                }
                                FragmentProgramData +=
                                "float4 iUvProjection : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
                                TEXCOORDNUM++;
                                if (Options.NM == NM_VERTEX)
                                {
                                    FragmentProgramData +=
                                    "float3 iNormal       : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
                                    TEXCOORDNUM++;
                                }
                                FragmentProgramData +=
                                "float2 iDistance       : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
                                TEXCOORDNUM++;
                                /* Foam is not visible underwater
                                if (cFoam)
                                {
                                    FragmentProgramData +=
                                    "float4 iWorldPosition  : TEXCOORD" + Ogre::StringConverter::toString(TEXCOORDNUM) + ",\n";
                                }
                                */

                                FragmentProgramData += Ogre::String(
                                // OUT
                                "out float4 oColor    : COLOR,\n") +
                                // UNIFORM
                                "uniform float3       uEyePosition,\n" +
                                "uniform float        uFullReflectionDistance,\n" +
                                "uniform float        uGlobalTransparency,\n" +
                                "uniform float        uNormalDistortion,\n" +
                                "uniform float        uDistLimit,\n" +
                                "uniform float3       uWaterColor,\n";
                                if (cSun)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "uniform float3       uSunPosition,\n") +
                                    "uniform float        uSunStrength,\n" +
                                    "uniform float        uSunArea,\n" +
                                    "uniform float3       uSunColor,\n" +
                                    "uniform float        uDepthLimit,\n";
                                }
                                /* Foam is not visible underwater
                                if (cFoam)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "uniform float        uFoamRange,\n") +
                                    "uniform float        uFoamMaxDistance,\n" +
                                    "uniform float        uFoamScale,\n" +
                                    "uniform float        uFoamStart,\n" +
                                    "uniform float        uFoamTransparency,\n";
                                }
                                */
                                if (cCaustics && cUReflections)
                                {
                                    FragmentProgramData +=
                                    "uniform float        uCausticsPower,\n";
                                }

                                int TexNum = 0;

                                if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
                                {
                                    FragmentProgramData +=
                                    "uniform sampler2D    uNormalMap       : register(s" + Ogre::StringConverter::toString(TexNum) + "),\n";
                                    TexNum++;
                                }
                                if (cUReflections)
                                {
                                    FragmentProgramData +=
                                    "uniform sampler2D    uReflectionMap   : register(s" + Ogre::StringConverter::toString(TexNum) + "),\n";
                                    TexNum++;
                                }

                                FragmentProgramData +=
                                "uniform sampler2D    uRefractionMap   : register(s" + Ogre::StringConverter::toString(TexNum) + "),\n";
                                TexNum++;

                                if (cDepth && cUReflections)
                                {
                                    FragmentProgramData +=
                                    "uniform sampler2D    uDepthReflectionMap : register(s" + Ogre::StringConverter::toString(TexNum) + "),\n";
                                    TexNum++;
                                }

                                FragmentProgramData +=
                                #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                                    "uniform sampler1D    uFresnelMap      : register(s" + Ogre::StringConverter::toString(TexNum) + ")";
                                #else
                                    "uniform sampler2D    uFresnelMap      : register(s" + Ogre::StringConverter::toString(TexNum) + ")";
                                #endif
                                TexNum++;

                                /* Foam is not visible underwater
                                if (cFoam)
                                {
                                    FragmentProgramData += Ogre::String(
                                     ",\nuniform sampler2D    uFoamMap         : register(s" + Ogre::StringConverter::toString(TexNum) + ")\n");
                                }
                                */
                                FragmentProgramData += Ogre::String(
                                ")\n") +
							"{\n"     +
							    "float2 ProjectionCoord = iUvProjection.xy / iUvProjection.w;\n" +
                                "float3 camToSurface = iPosition.xyz - uEyePosition;\n" +
                                "float additionalReflection=camToSurface.x*camToSurface.x+camToSurface.z*camToSurface.z;\n";

                                /* Foam is not visible underwater
                                if (cFoam)
                                {
                                    // Calculate the foam visibility as a function fo distance specified by user
                                    FragmentProgramData +=
                                    "float foamVisibility=1.0f-saturate(additionalReflection/uFoamMaxDistance);\n";
                                }
                                */

                                FragmentProgramData += Ogre::String(
							    "additionalReflection/=uFullReflectionDistance;\n") +
								"camToSurface=normalize(-camToSurface);\n";
                                if (Options.NM == NM_TEXTURE)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "float3 pixelNormal = tex2D(uNormalMap,iUvNoise).xyz;\n") +
                                    // Inverte y with z, because at creation our local normal to the plane was z
                                    "pixelNormal.yz=pixelNormal.zy;\n" +
                                    // Remap from [0,1] to [-1,1]
                                    "pixelNormal.xyz=-expand(pixelNormal.xyz);\n";
                                }
                                else if (Options.NM == NM_VERTEX)
                                {
                                    FragmentProgramData +=
                                    "float3 pixelNormal = -iNormal;\n";
                                }
                                else // NM_RTT
                                {
                                    FragmentProgramData +=
                                    "float3 pixelNormal = -(2.0*tex2D(uNormalMap, ProjectionCoord.xy).xyz - 1.0);\n";
                                }
                                FragmentProgramData +=
								"float2 pixelNormalModified = uNormalDistortion*pixelNormal.zx;\n";

                                if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
                                {
                                    FragmentProgramData +=
                                    "float dotProduct=dot(camToSurface,pixelNormal);\n";
                                }
                                else
                                {
                                    FragmentProgramData +=
                                    "float dotProduct=dot(-camToSurface,pixelNormal);\n";
                                }
                                FragmentProgramData += Ogre::String(
								"dotProduct=saturate(dotProduct);\n") +
								#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                                    "float fresnel = tex1D(uFresnelMap,dotProduct);\n" +
                                #else
                                    "float fresnel = tex2D(uFresnelMap,float2(dotProduct,dotProduct));\n" +
                                #endif
								// Add additional reflection and saturate
								"fresnel+=additionalReflection;\n" +
								"fresnel=saturate(fresnel);\n" +
								// Decrease the transparency and saturate
								"fresnel-=uGlobalTransparency;\n" +
                                "fresnel=saturate(fresnel);\n" +
                                "float3 reflection;\n" +
                                #if OGRE_PLATFORM != OGRE_PLATFORM_WIN32
                                    // Reversing projection if underwater
                                    "if(uEyePosition.y < 0.0)\n" +
                                    "{\n" +
                                        "ProjectionCoord.y = 1.0 - ProjectionCoord.y;\n" +
                                    "}\n" +
                                #endif
								// Get the reflection/refraction pixels. Make sure to disturb the texcoords by pixelnormal
								"float3 refraction=tex2D(uRefractionMap,ProjectionCoord.xy-pixelNormalModified).xyz;\n";
                                if (cUReflections)
                                {
                                    FragmentProgramData +=
                                    "reflection=tex2D(uReflectionMap,ProjectionCoord.xy+pixelNormalModified).xyz;\n";
                                }
                                else
                                {
                                    FragmentProgramData +=
                                    "reflection=uWaterColor;\n";
                                }

                                if (cDepth && cUReflections)
                                {
                                    if (cCaustics)
                                    {
                                        FragmentProgramData += Ogre::String(
                                        "float2 depth = tex2D(uDepthReflectionMap,ProjectionCoord.xy+pixelNormalModified).rg;\n") +
                                        "reflection *= 1+depth.y*uCausticsPower;\n" +
                                        "reflection = lerp(uWaterColor,reflection,depth.x);\n";
                                    }
                                    else
                                    {
                                        FragmentProgramData += Ogre::String(
                                        "float depth = tex2D(uDepthReflectionMap,ProjectionCoord.xy-pixelNormalModified).r;\n") +
                                        "reflection = lerp(uWaterColor,reflection,depth);\n";
                                    }
                                }
                                FragmentProgramData += Ogre::String(
								"float4 Color = float4(lerp(refraction,reflection,fresnel),1);\n" ) +
								"float Distance = saturate(1.0 - iDistance.x*uDistLimit);\n" +
								"Color.xyz = lerp(uWaterColor, Color.xyz, Distance);\n" +
								"Color.xyz = lerp(Color.xyz, uWaterColor, uGlobalTransparency);\n";

                                if (cSun)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "float3 refractedVector = normalize(reflect(camToSurface, pixelNormal.xyz));\n") +
                                    "float3 surfaceToSun=normalize(uSunPosition-iPosition.xyz);\n" +
                                    // Temporally solution, fix this
                                    "surfaceToSun.xz = -surfaceToSun.xz;" +
                                    "float3 sunlight = uSunStrength*pow(saturate(dot(refractedVector,surfaceToSun)),uSunArea)*uSunColor;\n" +
                                    "Distance  = saturate(1.0 - iDistance.y*uDepthLimit);\n" +
                                    "Color.xyz+=Distance*sunlight*saturate(1.0-additionalReflection);\n";
                                }

                                /* Foam is not visible underwater
                                if (cFoam)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "float hmap = iPosition.y/uFoamRange*foamVisibility;\n") +
                                    "float2 foamTex=iWorldPosition.xz*uFoamScale+pixelNormalModified;\n" +
                                    "float foam=tex2D(uFoamMap,foamTex).r;\n" +
                                    "float foamTransparency=saturate(hmap-uFoamStart)*uFoamTransparency;\n" +
                                    "Color.xyz=lerp(Color.xyz,1,foamTransparency*foam);\n";
                                }
                                */
                            FragmentProgramData += Ogre::String(
                                "oColor = Color;\n") +
							"}\n";
					}
					break;

					case SM_GLSL:
						FragmentProgramData += Ogre::String("\n") +
                            // UNIFORMS
                            "uniform vec3  uEyePosition;\n" +
                            "uniform float uFullReflectionDistance;\n" +
                            "uniform float uGlobalTransparency;\n" +
                            "uniform float uNormalDistortion;\n" +
                            "uniform float uDistLimit;\n" +
                            "uniform vec3  uWaterColor;\n";

                            if (cSun)
                            {
                                FragmentProgramData += Ogre::String(
                                "uniform vec3  uSunPosition;\n") +
                                "uniform float uSunStrength;\n" +
                                "uniform float uSunArea;\n" +
                                "uniform vec3  uSunColor;\n" +
                                "uniform float uDepthLimit;\n";
                            }
                            /* Foam is not visible underwater
                            if (cFoam)
                            {
                                FragmentProgramData += Ogre::String(
							    "uniform float uFoamRange;\n") +
							    "uniform float uFoamMaxDistance;\n" +
	                            "uniform float uFoamScale;\n" +
	                            "uniform float uFoamStart;\n" +
	                            "uniform float uFoamTransparency;\n";
                            }
                            */
                            if (cCaustics && cUReflections)
                            {
                                FragmentProgramData +=
								"uniform float uCausticsPower;\n";
                            }

                            int TexNum = 0;
                            if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
                            {
                                FragmentProgramData +=
                               "uniform sampler2D uNormalMap;\n";
                                TexNum++;
                            }

                            if (cUReflections)
                            {
                                FragmentProgramData += Ogre::String(
                                "uniform sampler2D uReflectionMap;\n");
                                TexNum++;
                            }
                            FragmentProgramData += Ogre::String(
                            "uniform sampler2D uRefractionMap;\n");
                            TexNum++;

                            if (cDepth && cUReflections)
                            {
                                FragmentProgramData +=
                                "uniform sampler2D uDepthReflectionMap;\n";
                                TexNum++;
                            }

                            FragmentProgramData +=
                            #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                                "uniform sampler1D uFresnelMap;\n";
                            #else
                                "uniform sampler2D uFresnelMap;\n";
                            #endif
                            TexNum++;

                            /* Foam is not visible underwater
                            if (cFoam)
                            {
                                FragmentProgramData +=
                                "uniform sampler2D uFoamMap;\n";
                            }
                            */
                            // IN
                            FragmentProgramData +=
                                "varying vec4 Position_;\n";
                            int TEXCOORDNUM = 1;
                            if (Options.NM == NM_TEXTURE)
                            {
                                TEXCOORDNUM++;
                            }
                            FragmentProgramData +=
                                "varying vec4 UVProjection;\n";
                            TEXCOORDNUM++;
                            if (Options.NM == NM_VERTEX)
                            {
                                FragmentProgramData +=
                                "varying vec3 Normal;\n";
                                TEXCOORDNUM++;
                            }
                            /* Foam is not visible underwater
                            if (cFoam)
                            {
                                FragmentProgramData +=
                                "varying vec4 WorldPosition;\n";
                            }
                            */
                            FragmentProgramData +=
                            "varying vec2 Distance_;\n";
                            // Expand function
                            FragmentProgramData += Ogre::String(
							"vec3 expand(vec3 v)\n") +
						    "{\n" +
	                            "return (v - 0.5) * 2.0;\n" +
							"}\n\n" +
                            // main function
							"void main()\n" +
						    "{\n" +
							    "vec2 ProjectionCoord = UVProjection.xy / UVProjection.w;\n" +
							    "ProjectionCoord += 1.0;\n" +
							    "ProjectionCoord *= 0.5;\n" +
                                "vec3 camToSurface = Position_.xyz - uEyePosition;\n" +
                                "float additionalReflection=camToSurface.x*camToSurface.x+camToSurface.z*camToSurface.z;\n";
                                /* Must not view foam underwater
                                if (cFoam)
                                {
                                    // Calculate the foam visibility as a function fo distance specified by user
                                    FragmentProgramData +=
                                    "float foamVisibility=1.0-clamp(additionalReflection/uFoamMaxDistance, 0.0, 1.0);\n";
                                }
                                */
                                FragmentProgramData += Ogre::String(
                                "additionalReflection/=uFullReflectionDistance;\n") +
                                "camToSurface=normalize(-camToSurface);\n";
                                if (Options.NM == NM_TEXTURE)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "vec3 pixelNormal = texture2D(uNormalMap,gl_TexCoord[0].xy).xyz;\n") +
                                    // Inverte y with z, because at creation our local normal to the plane was z
                                    "pixelNormal.yz=pixelNormal.zy;\n" +
                                    // Remap from [0,1] to [-1,1]
                                    "pixelNormal.xyz=-expand(pixelNormal.xyz);\n";
                                }
                                else if (Options.NM == NM_VERTEX)
                                {
                                    FragmentProgramData +=
                                    "vec3 pixelNormal = -Normal;\n";
                                }
                                else // NM_RTT
                                {
                                    FragmentProgramData +=
                                    "vec3 pixelNormal = -2.0*texture2D(uNormalMap, ProjectionCoord.xy).xyz - 1.0;\n";
                                }
                                FragmentProgramData +=
                                "vec2 pixelNormalModified = uNormalDistortion*pixelNormal.zx;\n";
                                if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
                                {
                                    FragmentProgramData +=
                                    "float dotProduct=dot(camToSurface,pixelNormal);\n";
                                }
                                else
                                {
                                    FragmentProgramData +=
                                    "float dotProduct=dot(-camToSurface,pixelNormal);\n";
                                }
                                FragmentProgramData += Ogre::String(
								"dotProduct=clamp(dotProduct, 0.0, 1.0);\n") +
                                #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                                    "float fresnel = texture1D(uFresnelMap,dotProduct).x;\n" +
                                #else
                                    "float fresnel = texture2D(uFresnelMap,vec2(dotProduct,dotProduct)).x;\n" +
                                #endif
								// Add additional reflection and saturate
								"fresnel +=additionalReflection;\n" +
								"fresnel = clamp(fresnel, 0.0, 1.0);\n" +
								// Decrease the transparency and saturate
								"fresnel -= uGlobalTransparency;\n" +
                                "fresnel =  clamp(fresnel, 0.0, 1.0);\n" +
								// Get the reflection/refraction pixels. Make sure to disturb the texcoords by pixelnormal
								"vec3 reflection;\n" +
                                #if OGRE_PLATFORM != OGRE_PLATFORM_WIN32
                                    // Reversing projection if underwater
                                    "if(uEyePosition.y < 0.0)\n" +
                                    "{\n" +
                                        "ProjectionCoord.y = 1.0 - ProjectionCoord.y;\n" +
                                    "}\n" +
                                #endif
								"vec3 refraction=texture2D(uRefractionMap,ProjectionCoord.xy-pixelNormalModified).xyz;\n";
                                if (cUReflections)
                                {
                                    FragmentProgramData +=
                                    "reflection=texture2D(uReflectionMap,ProjectionCoord.xy+pixelNormalModified).xyz;\n";
                                }
                                else
                                {
                                    FragmentProgramData +=
                                    "reflection=uWaterColor;\n";
                                }
                                if (cDepth && cUReflections)
                                {
                                    if (cCaustics)
                                    {
                                        FragmentProgramData += Ogre::String(
                                        "vec2 depth = texture2D(uDepthReflectionMap,ProjectionCoord.xy+pixelNormalModified).xy;\n") +
                                        "reflection *= 1.0 + depth.y*uCausticsPower;\n" +
                                        "reflection = mix(uWaterColor,reflection,depth.x);\n";
                                    }
                                    else
                                    {
                                        FragmentProgramData += Ogre::String(
                                        "float depth = texture2D(uDepthReflectionMap,ProjectionCoord.xy+pixelNormalModified).x;\n") +
                                        "reflection = mix(uWaterColor,reflection,depth);\n";
                                    }
                                }
                                FragmentProgramData += Ogre::String(
								"gl_FragColor = vec4(mix(refraction,reflection,fresnel),1.0);\n") +
								"float Distance  = clamp(1.0 - Distance_.x*uDistLimit, 0.0, 1.0);\n" +
								"gl_FragColor.xyz = mix(uWaterColor, gl_FragColor.xyz, Distance);\n" +
								"gl_FragColor.xyz = mix(gl_FragColor.xyz, uWaterColor, uGlobalTransparency);\n";
                                if (cSun)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "vec3 refractedVector = normalize(reflect(-camToSurface,pixelNormal.xyz));\n") +
                                    "vec3 surfaceToSun=normalize(uSunPosition-Position_.xyz);\n" +
                                    // Temporally solution, fix this
                                    "surfaceToSun.xz = -surfaceToSun.xz;" +
                                    "vec3 sunlight = uSunStrength*pow(clamp(dot(refractedVector,surfaceToSun),0.0,1.0),uSunArea)*uSunColor;\n" +
                                    "Distance  = clamp(1.0 - Distance_.y*uDepthLimit, 0.0, 1.0);\n";
                                    "gl_FragColor.xyz+=Distance*sunlight*clamp(1.0 - additionalReflection, 0.0, 1.0);\n";
                                }
                                /* Must not view foam underwater
                                if (cFoam)
                                {
                                    FragmentProgramData += Ogre::String(
                                    "float hmap = Position_.y/uFoamRange*foamVisibility;\n") +
                                    "vec2 foamTex=WorldPosition.xz*uFoamScale+pixelNormalModified;\n" +
                                    "float foam=texture2D(uFoamMap,foamTex).x;\n" +
                                    "float foamTransparency=clamp(hmap-uFoamStart, 0.0, 1.0)*uFoamTransparency;\n" +
                                    "gl_FragColor.xyz=mix(gl_FragColor.xyz,vec3(1.0,1.0,1.0),foamTransparency*foam);\n";
                                }
                                */
                            FragmentProgramData +=
							"}\n";
					break;
                }
		    }
		    break;
		}

		// Second: build our material
		Ogre::MaterialPtr &UnderwaterMaterial = getMaterial(MAT_UNDERWATER);
		UnderwaterMaterial = Ogre::MaterialManager::getSingleton().
			create(_def_Underwater_Material_Name,
			       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Ogre::Pass *UM_Technique0_Pass0 = UnderwaterMaterial->getTechnique(0)->getPass(0);

		UM_Technique0_Pass0->setDepthWriteEnabled(true);
        UM_Technique0_Pass0->setCullingMode(Ogre::CULL_NONE);

		Ogre::String GpuProgramsData[2] = {VertexProgramData, FragmentProgramData};
		Ogre::String GpuProgramNames[2] = {_def_Underwater_Shader_VP_Name, _def_Underwater_Shader_FP_Name};
        Ogre::String EntryPoints[2];
        if(Options.SM == SM_GLSL)
        {
            EntryPoints[0] = Ogre::String("main");
            EntryPoints[1] = Ogre::String("main");
        }
        else
        {
            EntryPoints[0] = Ogre::String("main_vp");
            EntryPoints[1] = Ogre::String("main_fp");
        }

		fillGpuProgramsToPass(UM_Technique0_Pass0, GpuProgramNames, Options.SM, EntryPoints, GpuProgramsData);

		Ogre::GpuProgramParametersSharedPtr VP_Parameters = UM_Technique0_Pass0->getVertexProgramParameters();
		Ogre::GpuProgramParametersSharedPtr FP_Parameters = UM_Technique0_Pass0->getFragmentProgramParameters();

        if(Options.SM != SM_GLSL)
        {
            VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
            VP_Parameters->setNamedAutoConstant("uWorldView", Ogre::GpuProgramParameters::ACT_WORLDVIEW_MATRIX);
            VP_Parameters->setNamedAutoConstant("uCameraPos", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);
        }
        VP_Parameters->setNamedAutoConstant("uWorld",     Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
		FP_Parameters->setNamedAutoConstant("uEyePosition",   Ogre::GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);

		FP_Parameters->setNamedConstant("uFullReflectionDistance", mHydrax->getFullReflectionDistance());
		FP_Parameters->setNamedConstant("uGlobalTransparency",     mHydrax->getGlobalTransparency());
		FP_Parameters->setNamedConstant("uNormalDistortion",       mHydrax->getNormalDistortion());
		FP_Parameters->setNamedConstant("uDistLimit",              1.f/mHydrax->getDistLimit());
        if (cSun)
        {
        }
        FP_Parameters->setNamedConstant("uWaterColor",             mHydrax->getWaterColor());

		if (cSun)
		{
			FP_Parameters->setNamedConstant("uSunPosition", mHydrax->getMesh()->getObjectSpacePosition(mHydrax->getSunPosition()));
			FP_Parameters->setNamedConstant("uSunStrength", mHydrax->getSunStrength());
			FP_Parameters->setNamedConstant("uSunArea",     mHydrax->getSunArea());
			FP_Parameters->setNamedConstant("uSunColor",    mHydrax->getSunColor());
            FP_Parameters->setNamedConstant("uDepthLimit",  1.f/mHydrax->getDepthLimit());
		}
		/* Foam is not visible underwater
		if (cFoam)
		{
			FP_Parameters->setNamedConstant("uFoamRange",        mHydrax->getMesh()->getOptions().MeshStrength);
			FP_Parameters->setNamedConstant("uFoamMaxDistance",  mHydrax->getFoamMaxDistance());
			FP_Parameters->setNamedConstant("uFoamScale",        mHydrax->getFoamScale());
			FP_Parameters->setNamedConstant("uFoamStart",        mHydrax->getFoamStart());
			FP_Parameters->setNamedConstant("uFoamTransparency", mHydrax->getFoamTransparency());
		}
		*/
		if (cCaustics && cDepth && cUReflections)
		{
			FP_Parameters->setNamedConstant("uCausticsPower", mHydrax->getCausticsPower());
		}

		int GLSLTextUnit = 0;

		if (Options.NM == NM_TEXTURE || Options.NM == NM_RTT)
		{
            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uNormalMap", GLSLTextUnit);
                GLSLTextUnit++;
            }
		    UM_Technique0_Pass0->createTextureUnitState("HydraxNormalMap")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);
		}

		if (cUReflections)
		{
            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uReflectionMap", GLSLTextUnit);
                GLSLTextUnit++;
            }
		    UM_Technique0_Pass0->createTextureUnitState("HydraxReflectionMap")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
		}
        if(Options.SM == SM_GLSL)
        {
            FP_Parameters->setNamedConstant("uRefractionMap", GLSLTextUnit);
            GLSLTextUnit++;
        }
		UM_Technique0_Pass0->createTextureUnitState("HydraxRefractionMap")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

		if (cDepth && cUReflections)
		{
            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uDepthReflectionMap", GLSLTextUnit);
                GLSLTextUnit++;
            }
			UM_Technique0_Pass0->createTextureUnitState()->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
		}

        if(Options.SM == SM_GLSL)
        {
            FP_Parameters->setNamedConstant("uFresnelMap", GLSLTextUnit);
            GLSLTextUnit++;
        }
		UM_Technique0_Pass0->createTextureUnitState("Fresnel.bmp")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

        /* Foam is not visible underwater
		if (cFoam)
		{
            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uFoamMap", GLSLTextUnit);
                GLSLTextUnit++;
            }
			UM_Technique0_Pass0->createTextureUnitState("Foam.png")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);
		}
        */

		UnderwaterMaterial->setReceiveShadows(false);
		UnderwaterMaterial->load();

		return true;
	}

	bool MaterialManager::_createUnderwaterCompositor(const HydraxComponent &Components, const Options &Options)
	{
		const bool cCaustics   = _isComponent(Components, HYDRAX_COMPONENT_CAUSTICS);
		const bool cDepth   = _isComponent(Components, HYDRAX_COMPONENT_DEPTH);
		const bool cGodRays = _isComponent(Components, HYDRAX_COMPONENT_UNDERWATER_GODRAYS);

		Ogre::String VertexProgramData, FragmentProgramData;

		// Vertex program
		switch (Options.SM)
		{
		    case SM_HLSL: case SM_CG:
			{
				VertexProgramData +=
					Ogre::String(
					"void main_vp(\n") +
					    // IN
						"float4 iPosition      : POSITION,\n" +
						// OUT
						"out float4 oPosition  : POSITION,\n" +
						"out float3 oPosition_ : TEXCOORD0,\n" +
						"out float2 oUV        : TEXCOORD1,\n";
						// UNIFORM
				        if (cGodRays)
						{
							VertexProgramData +=
								Ogre::String(
				        "uniform float3   uCorner0,\n") +
		                "uniform float3   uCorner01,\n" +
		                "uniform float3   uCorner02,\n";
						}
						VertexProgramData +=
							Ogre::String(
						"uniform float4x4 uWorldViewProj)\n") +
					"{\n" +
					    "oPosition = mul(uWorldViewProj, iPosition);\n"+
						"iPosition.xy = sign(iPosition.xy);\n"+
						"oUV = (float2(iPosition.x, -iPosition.y) + 1.0f) * 0.5f;";
						if (cGodRays)
						{
							VertexProgramData += Ogre::String(
                            "uCorner01 *= oUV.x;\n")+
                            "uCorner02 *= oUV.y;\n"+
                            "oPosition_ = uCorner0+uCorner01+uCorner02;";
						}
						VertexProgramData +=
					"}\n";
			}
			break;

			case SM_GLSL:
			{
				VertexProgramData += Ogre::String( "\n" );
                    // UNIFORMS
                    if (cGodRays)
                    {
                        VertexProgramData += Ogre::String(
                        "uniform vec3   uCorner0;\n") +
                        "uniform vec3   uCorner01;\n" +
                        "uniform vec3   uCorner02;\n";
                    }
                    // IN
                    // OUT
                    VertexProgramData += Ogre::String(
                    "varying vec3 Position_;\n") +
                    "varying vec2 UV;\n" +
                    // main function
					"void main()\n" +
					"{\n" +
					    "gl_Position = ftransform();\n" +
						"vec2 iPosition = sign(gl_Vertex.xy);\n"+
						"UV = (vec2(iPosition.x, -iPosition.y) + 1.0) * 0.5;\n";
						if (cGodRays)
						{
							VertexProgramData += Ogre::String(
                            "vec3 vCorner01 = uCorner01 * UV.x;\n")+
                            "vec3 vCorner02 = uCorner02 * UV.y;\n"+
                            "Position_ = uCorner0+vCorner01+vCorner02;\n";
						}
                    VertexProgramData +=
					"}\n";
            }
			break;
		}

		// Fragment program
		switch (Options.SM)
		{
		    case SM_HLSL: case SM_CG:
			{
				FragmentProgramData +=
					Ogre::String(
				    "void main_fp(\n") +
						// IN
						"float3  iPosition    : TEXCOORD0,\n" +
						"float2  iUV          : TEXCOORD1,\n" +
					    // OUT
						"out float4 oColor    : COLOR,\n";
						// UNIFORM
                        if (cCaustics)
                        {
                            FragmentProgramData +=
                            "uniform float        uCausticsPower,\n";
                        }
                        if (cGodRays)
                        {
                            FragmentProgramData += Ogre::String(
                            "uniform float3  uSunColor,\n") +
                            "uniform float3  uLightDirection,\n"+
                            "uniform float   uIntensity,\n"+
                            "uniform float3  uHGg,\n"+
                            "uniform float3  uCameraPos,\n";
                        }
                        FragmentProgramData += Ogre::String(
                        "uniform float        uTime,\n") +
                        "uniform float3       uWaterColor,\n" +
                        "uniform sampler2D    uOriginalMap   : register(s0),\n"  +
                        "uniform sampler2D    uDistortMap    : register(s1)\n";
                        if (cDepth)
                        {
                            FragmentProgramData +=
                            ",\nuniform sampler2D    uDepthMap      : register(s2)";
                        }
                        FragmentProgramData += Ogre::String(
                        ")\n" ) +
                    "{\n" +
					    "float2 distortUV = (tex2D(uDistortMap, float2(iUV.x + uTime, iUV.y + uTime)).xy - 0.5)/50.0;\n";
                        if (cCaustics) // Depth, caustics
                        {
                            FragmentProgramData += Ogre::String(
                            "float2 depth = tex2D(uDepthMap, iUV+distortUV).xy;\n") +
                            "float4 Color = float4(lerp(uWaterColor,tex2D(uOriginalMap, iUV+distortUV)*(1.0+depth.y*uCausticsPower), depth.x),1.0);\n";
                            if (cGodRays)
                            {
                                FragmentProgramData += Ogre::String(
                                "float3 view_vector = normalize(iPosition-uCameraPos);\n") +
                                "float dot_product = dot(view_vector, -uLightDirection);\n"+
                                "float num = uHGg.x;\n"+
                                "float den = (uHGg.y - uHGg.z*dot_product);\n"+
                                "den = rsqrt(den);\n"+
                                "float phase = num * (den*den*den);\n" +
                                "Color.xyz += (0.15 + uIntensity*tex2D(uDepthMap, iUV).z)*phase*uSunColor;\n";
                            }
                        }
                        else if (cDepth) // Depth, no caustics
                        {
                            FragmentProgramData +=
                            "float4 Color = float4(lerp(uWaterColor,tex2D(uOriginalMap, iUV+distortUV).xyz,tex2D(uDepthMap, iUV+distortUV).r),1.0);\n";
                            if (cGodRays)
                            {
                                FragmentProgramData += Ogre::String(
                                "float3 view_vector = normalize(iPosition-uCameraPos);") +
                                "float dot_product = dot(view_vector, -uLightDirection);"+
                                "float num = uHGg.x;"+
                                "float den = (uHGg.y - uHGg.z*dot_product); "+
                                "den = rsqrt(den); "+
                                "float phase = num * (den*den*den);"+
                                "Color.xyz += (0.15 + uIntensity*tex2D(uDepthMap, iUV).y)*phase*uSunColor;";
                            }
                        }
                        else // No depth, no caustics
                        {
                            FragmentProgramData +=
                            "float4 Color = tex2D(uOriginalMap, iUV+distortUV);";
                        }
					FragmentProgramData += Ogre::String(
                        "oColor = Color;\n") +
					"}\n";
			}
			break;

			case SM_GLSL:
			{
				FragmentProgramData += Ogre::String( "\n" );
                    // UNIFORM
					if (cCaustics)
					{
                        FragmentProgramData +=
						"uniform float uCausticsPower;\n";
					}
					if (cGodRays)
					{
                        FragmentProgramData += Ogre::String(
						"uniform vec3  uSunColor;\n") +
						"uniform vec3  uLightDirection;\n"+
                        "uniform float uIntensity;\n"+
	                    "uniform vec3  uHGg;\n"+
	                    "uniform vec3  uCameraPos;\n";
					}
                    FragmentProgramData += Ogre::String(
                    "uniform float        uTime;\n") +
                    "uniform float        uGlobalTransparency;\n" +
                    "uniform vec3         uWaterColor;\n" +
                    "uniform sampler2D    uOriginalMap;\n"  +
                    "uniform sampler2D    uDistortMap;\n";
					if (cDepth)
					{
                        FragmentProgramData +=
					    "uniform sampler2D    uDepthMap;\n";
					}
					// IN
					FragmentProgramData += Ogre::String(
                    "varying vec3 Position_;\n") +
                    "varying vec2 UV;\n" +
                    // OUT
                    // main function
				    "void main()\n" +
					"{\n" +
                        "vec2 distortUV = (texture2D(uDistortMap, vec2(UV.x + uTime, UV.y + uTime)).xy - 0.5)*0.02;\n";
                        // "vec2 distortUV = vec2(0.0,0.0);\n";
                        if (cCaustics) // Depth, caustics
                        {
                            FragmentProgramData += Ogre::String(
                            "vec2 depth = texture2D(uDepthMap, UV+distortUV).xy;\n") +
                            "gl_FragColor = vec4(mix(uWaterColor,texture2D(uOriginalMap, UV+distortUV).xyz*(1.0+depth.y*uCausticsPower), depth.x),1.0);\n";
                            if (cGodRays)
                            {
                                FragmentProgramData += Ogre::String(
                                "vec3  view_vector = normalize(Position_-uCameraPos);\n") +
                                "float dot_product = dot(view_vector, -uLightDirection);\n"+
                                "float num = uHGg.x;\n"+
                                "float den = (uHGg.y - uHGg.z*dot_product);\n"+
                                "den = inversesqrt(den);\n"+
                                "float phase = num * pow(den, 3.0);\n"+
                                "gl_FragColor.xyz += (0.15 + uIntensity*texture2D(uDepthMap, UV).z)*phase*uSunColor;";
                            }
                        }
                        else if (cDepth) // Depth, no caustics
                        {
                            FragmentProgramData +=
                            "gl_FragColor = vec4(mix(uWaterColor,texture2D(uOriginalMap, UV+distortUV).xyz,texture2D(uDepthMap, UV+distortUV).r),1.0);\n";
                            if (cGodRays)
                            {
                                FragmentProgramData += Ogre::String(
                                "vec3  view_vector = normalize(Position_-uCameraPos);\n") +
                                "float dot_product = dot(view_vector, -uLightDirection);\n"+
                                "float num = uHGg.x;\n"+
                                "float den = (uHGg.y - uHGg.z*dot_product);\n"+
                                "den = inversesqrt(den);\n"+
                                "float phase = num * pow(den, 3.0);\n"+
                                "gl_FragColor.xyz += (0.15 + uIntensity*texture2D(uDepthMap, UV).z)*phase*uSunColor;\n";
                            }
                        }
                        else // No depth, no caustics
                        {
                            FragmentProgramData +=
                            "gl_FragColor = texture2D(uOriginalMap, UV+distortUV);\n";
                        }
					FragmentProgramData +=
					"}\n";
            }
			break;
		}

		// Build our material
		Ogre::MaterialPtr &UnderwaterCompositorMaterial = getMaterial(MAT_UNDERWATER_COMPOSITOR);
		UnderwaterCompositorMaterial = Ogre::MaterialManager::getSingleton().
			create(_def_Underwater_Compositor_Material_Name,
			       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Ogre::Pass *DM_Technique0_Pass0 = UnderwaterCompositorMaterial->getTechnique(0)->getPass(0);

		DM_Technique0_Pass0->setCullingMode(Ogre::CULL_NONE);
		DM_Technique0_Pass0->setDepthFunction(Ogre::CMPF_ALWAYS_PASS);

		Ogre::String GpuProgramsData[2] = {VertexProgramData, FragmentProgramData};
		Ogre::String GpuProgramNames[2] = {_def_Underwater_Compositor_Shader_VP_Name, _def_Underwater_Compositor_Shader_FP_Name};
        Ogre::String EntryPoints[2];
        if(Options.SM == SM_GLSL)
        {
            EntryPoints[0] = Ogre::String("main");
            EntryPoints[1] = Ogre::String("main");
        }
        else
        {
            EntryPoints[0] = Ogre::String("main_vp");
            EntryPoints[1] = Ogre::String("main_fp");
        }

		fillGpuProgramsToPass(DM_Technique0_Pass0, GpuProgramNames, Options.SM, EntryPoints, GpuProgramsData);

		Ogre::GpuProgramParametersSharedPtr VP_Parameters = DM_Technique0_Pass0->getVertexProgramParameters();
		Ogre::GpuProgramParametersSharedPtr FP_Parameters = DM_Technique0_Pass0->getFragmentProgramParameters();

        if(Options.SM != SM_GLSL)
        {
            VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
        }

		FP_Parameters->setNamedConstantFromTime("uTime", 0.1f);
        FP_Parameters->setNamedConstant("uWaterColor", mHydrax->getWaterColor());

		if (cCaustics)
		{
			FP_Parameters->setNamedConstant("uCausticsPower", mHydrax->getCausticsPower());
		}

		if (cGodRays)
		{
			FP_Parameters->setNamedConstant("uSunColor", mHydrax->getSunColor());
			FP_Parameters->setNamedConstant("uLightDirection",
				(mHydrax->getMesh()->getObjectSpacePosition(mHydrax->getCamera()->getParentSceneNode()->getPosition()) -
				 mHydrax->getMesh()->getObjectSpacePosition(mHydrax->getSunPosition()))
			     .normalisedCopy());
		    FP_Parameters->setNamedConstant("uIntensity", mHydrax->getGodRaysIntensity());
		    FP_Parameters->setNamedConstant("uHGg", mHydrax->getGodRaysExposure());
		    FP_Parameters->setNamedAutoConstant("uCameraPos", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION);
		}

		// From compositor, original scene
		int GLSLTextUnit = 0;
        if(Options.SM == SM_GLSL)
        {
            FP_Parameters->setNamedConstant("uOriginalMap", GLSLTextUnit);
            GLSLTextUnit++;
        }
		DM_Technique0_Pass0->createTextureUnitState()->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
        if(Options.SM == SM_GLSL)
        {
            FP_Parameters->setNamedConstant("uDistortMap", GLSLTextUnit);
            GLSLTextUnit++;
        }
		DM_Technique0_Pass0->createTextureUnitState("UnderwaterDistortion.jpg")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);
		if (cDepth)
		{
            if(Options.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uDepthMap", GLSLTextUnit);
                GLSLTextUnit++;
            }
		    DM_Technique0_Pass0->createTextureUnitState("HydraxDepthMap")->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
            //Ogre::Viewport *Viewport = mHydrax->getCamera()->getViewport();
		}

		UnderwaterCompositorMaterial->setReceiveShadows(false);
		UnderwaterCompositorMaterial->load();

		Ogre::CompositorPtr &UnderwaterCompositor = getCompositor(COMP_UNDERWATER);
		UnderwaterCompositor = Ogre::CompositorManager::getSingleton().
			create(_def_Underwater_Compositor_Name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Ogre::CompositionTechnique* UnderWaterComp_Technique = UnderwaterCompositor->createTechnique();

		// Create the texture definition to render the original scene
		Ogre::CompositionTechnique::TextureDefinition* TDef = UnderWaterComp_Technique->createTextureDefinition("OriginalScene");
        TDef->width = 0;
        TDef->height = 0;
		Ogre::PixelFormatList l;
		l.push_back(Ogre::PF_A8R8G8B8);
		TDef->formatList = l;

		// Render the original scene
        Ogre::CompositionTargetPass* CTPass = UnderWaterComp_Technique->createTargetPass();
		CTPass->setInputMode(Ogre::CompositionTargetPass::IM_NONE);
        CTPass->setOutputName("OriginalScene");
		Ogre::CompositionPass* CPassClear = CTPass->createPass();
		CPassClear->setType(Ogre::CompositionPass::PT_CLEAR);

		const Ogre::Vector3& WC = mHydrax->getWaterColor();
		CPassClear->setClearColour(Ogre::ColourValue(WC.x, WC.y, WC.z));

		Ogre::CompositionPass* CPass = CTPass->createPass();
		CPass->setType(Ogre::CompositionPass::PT_RENDERSCENE);
		CPass->setFirstRenderQueue(Ogre::RENDER_QUEUE_SKIES_EARLY+1);

		// Build the output target pass
        Ogre::CompositionTargetPass* CTOutputPass = UnderWaterComp_Technique->getOutputTargetPass();
        CTOutputPass->setInputMode(Ogre::CompositionTargetPass::IM_NONE);

		// Final composition pass
	    Ogre::CompositionPass* COutputPass = CTOutputPass->createPass();
	    COutputPass->setType(Ogre::CompositionPass::PT_RENDERQUAD);
		COutputPass->setMaterial(getMaterial(MAT_UNDERWATER_COMPOSITOR));
        COutputPass->setInput(0, "OriginalScene");
		COutputPass->setLastRenderQueue(0);

        Ogre::CompositorManager::getSingleton().
			addCompositor(mHydrax->getViewport(),_def_Underwater_Compositor_Name)->
			    addListener(&mUnderwaterCompositorListener);

		return true;
	}

	bool MaterialManager::_createSimpleColorMaterial(const Ogre::ColourValue& Color, const MaterialType& MT, const Ogre::String& Name, const bool& DepthCheck, const bool& DepthWrite)
	{
		Ogre::MaterialPtr &SimpleColorMaterial = getMaterial(MT);
		SimpleColorMaterial = Ogre::MaterialManager::getSingleton().
			create(Name,
			       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Ogre::Pass *SCM_T0_Pass0 = SimpleColorMaterial->getTechnique(0)->getPass(0);
		SCM_T0_Pass0->setLightingEnabled(false);
		SCM_T0_Pass0->setDepthCheckEnabled(DepthCheck);
		SCM_T0_Pass0->setDepthWriteEnabled(DepthWrite);
		SCM_T0_Pass0->setCullingMode(Ogre::CULL_NONE);
		SCM_T0_Pass0->createTextureUnitState()->setColourOperationEx(Ogre::LBX_MODULATE,Ogre::LBS_MANUAL,Ogre::LBS_CURRENT, Color);

		SimpleColorMaterial->setReceiveShadows(false);
		SimpleColorMaterial->load();

		return true;
	}

	void MaterialManager::reload(const MaterialType &Material)
	{
		Ogre::MaterialPtr &Mat = getMaterial(Material);

		if (Mat.isNull())
		{
			return;
		}

		Mat->reload();

		const bool cDepth    = _isComponent(mComponents, HYDRAX_COMPONENT_DEPTH   );
		const bool cSmooth   = _isComponent(mComponents, HYDRAX_COMPONENT_SMOOTH  );
		const bool cSun      = _isComponent(mComponents, HYDRAX_COMPONENT_SUN     );
		const bool cFoam     = _isComponent(mComponents, HYDRAX_COMPONENT_FOAM    );
		const bool cCaustics = _isComponent(mComponents, HYDRAX_COMPONENT_CAUSTICS);
		const bool cUReflections = _isComponent(mComponents, HYDRAX_COMPONENT_UNDERWATER_REFLECTIONS);

		switch (Material)
		{
		    case MAT_WATER:
			{
				Ogre::Pass *M_Technique0_Pass0 = Mat->getTechnique(0)->getPass(0);

				switch (mOptions.NM)
				{
				    case NM_TEXTURE: case NM_RTT:
					{
						M_Technique0_Pass0->getTextureUnitState(0)->setTextureName("HydraxNormalMap");
						M_Technique0_Pass0->getTextureUnitState(1)->setTextureName("HydraxReflectionMap");
						M_Technique0_Pass0->getTextureUnitState(2)->setTextureName("HydraxRefractionMap");
						if (cDepth)
						{
						    M_Technique0_Pass0->getTextureUnitState(3)->setTextureName("HydraxDepthMap");
						}
					}
					break;

					case NM_VERTEX:
					{
						M_Technique0_Pass0->getTextureUnitState(0)->setTextureName("HydraxReflectionMap");
						M_Technique0_Pass0->getTextureUnitState(1)->setTextureName("HydraxRefractionMap");
						if (cDepth)
						{
						    M_Technique0_Pass0->getTextureUnitState(2)->setTextureName("HydraxDepthMap");
						}
					}
					break;
				}

				Ogre::GpuProgramParametersSharedPtr VP_Parameters = M_Technique0_Pass0->getVertexProgramParameters();
				Ogre::GpuProgramParametersSharedPtr FP_Parameters = M_Technique0_Pass0->getFragmentProgramParameters();

                if(mOptions.SM != SM_GLSL)
                {
                    VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
                }
				if (cFoam)
				{
					VP_Parameters->setNamedAutoConstant("uWorld", Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
				}
				FP_Parameters->setNamedAutoConstant("uEyePosition",   Ogre::GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);

				FP_Parameters->setNamedConstant("uFullReflectionDistance", mHydrax->getFullReflectionDistance());
				FP_Parameters->setNamedConstant("uGlobalTransparency",     mHydrax->getGlobalTransparency());
				FP_Parameters->setNamedConstant("uNormalDistortion",       mHydrax->getNormalDistortion());

                FP_Parameters->setNamedConstant("uWaterColor", mHydrax->getWaterColor());
				if (cSmooth)
				{
					FP_Parameters->setNamedConstant("uSmoothPower", mHydrax->getSmoothPower());
				}
				if (cSun)
				{
					FP_Parameters->setNamedConstant("uSunPosition", mHydrax->getMesh()->getObjectSpacePosition(mHydrax->getSunPosition()));
					FP_Parameters->setNamedConstant("uSunStrength", mHydrax->getSunStrength());
					FP_Parameters->setNamedConstant("uSunArea",     mHydrax->getSunArea());
					FP_Parameters->setNamedConstant("uSunColor",    mHydrax->getSunColor());
				}
				if (cFoam)
				{
			        FP_Parameters->setNamedConstant("uFoamRange",        mHydrax->getMesh()->getOptions().MeshStrength);
					FP_Parameters->setNamedConstant("uFoamMaxDistance",  mHydrax->getFoamMaxDistance());
					FP_Parameters->setNamedConstant("uFoamScale",        mHydrax->getFoamScale());
					FP_Parameters->setNamedConstant("uFoamStart",        mHydrax->getFoamStart());
					FP_Parameters->setNamedConstant("uFoamTransparency", mHydrax->getFoamTransparency());
				}
				if (cCaustics)
				{
					FP_Parameters->setNamedConstant("uCausticsPower",    mHydrax->getCausticsPower());
				}
			}
			break;

			case MAT_DEPTH:
			{
				Ogre::Pass *M_Technique0_Pass0 = Mat->getTechnique(0)->getPass(0);

				Ogre::GpuProgramParametersSharedPtr VP_Parameters = M_Technique0_Pass0->getVertexProgramParameters();
				Ogre::GpuProgramParametersSharedPtr FP_Parameters = M_Technique0_Pass0->getFragmentProgramParameters();

                if(mOptions.SM != SM_GLSL)
                {
                    VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
                    // VP_Parameters->setNamedAutoConstant("uWorldView", Ogre::GpuProgramParameters::ACT_WORLDVIEW_MATRIX);
                    VP_Parameters->setNamedAutoConstant("uCameraPos", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION);
                }
                VP_Parameters->setNamedAutoConstant("uWorld", Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
				VP_Parameters->setNamedConstant("uPlaneYPos", mHydrax->getPosition().y);
                VP_Parameters->setNamedConstant("uPlanesError", (float) mHydrax->getPlanesError());

				FP_Parameters->setNamedConstant("uDepthLimit", 1.f/mHydrax->getDepthLimit());
				FP_Parameters->setNamedConstant("uDistLimit", 1.f/mHydrax->getDistLimit());

				if (cCaustics)
				{
					FP_Parameters->setNamedConstant("uCausticsScale", mHydrax->getCausticsScale());
					FP_Parameters->setNamedConstant("uCausticsEnd",   mHydrax->getCausticsEnd());
				}
			}
			break;

			case MAT_UNDERWATER:
			{
				Ogre::Pass *M_Technique0_Pass0 = Mat->getTechnique(0)->getPass(0);

				switch (mOptions.NM)
				{
				    case NM_TEXTURE: case NM_RTT:
					{
						M_Technique0_Pass0->getTextureUnitState(0)->setTextureName("HydraxNormalMap");
						int Index = 1;
						if (cUReflections)
						{
						    M_Technique0_Pass0->getTextureUnitState(Index)->setTextureName("HydraxReflectionMap");
							Index++;
						}
						M_Technique0_Pass0->getTextureUnitState(Index)->setTextureName("HydraxRefractionMap");
						Index++;
						if (cDepth && cUReflections)
						{
						    M_Technique0_Pass0->getTextureUnitState(Index)->setTextureName("HydraxDepthReflectionMap");
						}
					}
					break;

					case NM_VERTEX:
					{
						int Index = 0;
						if (cUReflections)
						{
						    M_Technique0_Pass0->getTextureUnitState(Index)->setTextureName("HydraxReflectionMap");
							Index++;
						}
						M_Technique0_Pass0->getTextureUnitState(Index)->setTextureName("HydraxRefractionMap");
						Index++;
						if (cDepth && cUReflections)
						{
						    M_Technique0_Pass0->getTextureUnitState(Index)->setTextureName("HydraxDepthReflectionMap");
						}
					}
					break;
				}

				Ogre::GpuProgramParametersSharedPtr VP_Parameters = M_Technique0_Pass0->getVertexProgramParameters();
				Ogre::GpuProgramParametersSharedPtr FP_Parameters = M_Technique0_Pass0->getFragmentProgramParameters();

                if(mOptions.SM != SM_GLSL)
                {
                    VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
                    VP_Parameters->setNamedAutoConstant("uWorldView", Ogre::GpuProgramParameters::ACT_WORLDVIEW_MATRIX);
                    VP_Parameters->setNamedAutoConstant("uCameraPos", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);
                }
                VP_Parameters->setNamedAutoConstant("uWorld",     Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
				FP_Parameters->setNamedAutoConstant("uEyePosition",   Ogre::GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);

				FP_Parameters->setNamedConstant("uFullReflectionDistance", mHydrax->getFullReflectionDistance());
				FP_Parameters->setNamedConstant("uGlobalTransparency",     mHydrax->getGlobalTransparency());
				FP_Parameters->setNamedConstant("uNormalDistortion",       mHydrax->getNormalDistortion());
                FP_Parameters->setNamedConstant("uDistLimit",              1.f/mHydrax->getDistLimit());

				if ((cDepth && cUReflections) || (!cUReflections))
		        {
		            FP_Parameters->setNamedConstant("uWaterColor", mHydrax->getWaterColor());
		        }

				if (cSun)
				{
					FP_Parameters->setNamedConstant("uSunPosition", mHydrax->getMesh()->getObjectSpacePosition(mHydrax->getSunPosition()));
					FP_Parameters->setNamedConstant("uSunStrength", mHydrax->getSunStrength());
					FP_Parameters->setNamedConstant("uSunArea",     mHydrax->getSunArea());
					FP_Parameters->setNamedConstant("uSunColor",    mHydrax->getSunColor());
                    FP_Parameters->setNamedConstant("uDepthLimit",  1.f/mHydrax->getDistLimit());
				}
				/* Foam is not visible underwater
				if (cFoam)
				{
			        FP_Parameters->setNamedConstant("uFoamRange",        mHydrax->getMesh()->getOptions().MeshStrength);
					FP_Parameters->setNamedConstant("uFoamMaxDistance",  mHydrax->getFoamMaxDistance());
					FP_Parameters->setNamedConstant("uFoamScale",        mHydrax->getFoamScale());
					FP_Parameters->setNamedConstant("uFoamStart",        mHydrax->getFoamStart());
					FP_Parameters->setNamedConstant("uFoamTransparency", mHydrax->getFoamTransparency());
				}
				*/
				if (cCaustics && cDepth && cUReflections)
				{
					FP_Parameters->setNamedConstant("uCausticsPower",    mHydrax->getCausticsPower());
				}
			}
			break;

			case MAT_UNDERWATER_COMPOSITOR:
			{
				mCompositorsNeedToBeReloaded[COMP_UNDERWATER] = true;
			}
			break;

			case MAT_SIMPLE_RED:
			{
			}
			break;
			case MAT_SIMPLE_BLACK:
			{
			}
			break;
		}
	}

	void MaterialManager::addDepthTechnique(Ogre::Technique *Technique, const bool& AutoUpdate)
	{
		if (!Ogre::MaterialManager::getSingleton().resourceExists(_def_Depth_Material_Name))
		{
			_createDepthMaterial(mComponents, mOptions);
		}

		Technique->removeAllPasses();
		Technique->createPass();
		Technique->setName("_Hydrax_Depth_Technique");
		Technique->setSchemeName("HydraxDepth");

		Ogre::Pass *DM_Technique_Pass0 = Technique->getPass(0);

		DM_Technique_Pass0->setVertexProgram(_def_Depth_Shader_VP_Name);
		DM_Technique_Pass0->setFragmentProgram(_def_Depth_Shader_FP_Name);

		Ogre::GpuProgramParametersSharedPtr VP_Parameters = DM_Technique_Pass0->getVertexProgramParameters();
		Ogre::GpuProgramParametersSharedPtr FP_Parameters = DM_Technique_Pass0->getFragmentProgramParameters();

        if(mOptions.SM != SM_GLSL)
        {
            VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
            VP_Parameters->setNamedAutoConstant("uCameraPos", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION);
        }
        VP_Parameters->setNamedAutoConstant("uWorld", Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
        VP_Parameters->setNamedConstant("uPlaneYPos", mHydrax->getPosition().y);
        VP_Parameters->setNamedConstant("uPlanesError", (float) mHydrax->getPlanesError());

		FP_Parameters->setNamedConstant("uDepthLimit", 1/mHydrax->getDepthLimit());
		FP_Parameters->setNamedConstant("uDistLimit", 1/mHydrax->getDistLimit());

		if (_isComponent(mComponents, HYDRAX_COMPONENT_CAUSTICS))
		{
			FP_Parameters->setNamedConstant("uCausticsScale", mHydrax->getCausticsScale());
			FP_Parameters->setNamedConstant("uCausticsEnd",   mHydrax->getCausticsEnd());

            if(mOptions.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uCaustics", 0);
            }
			Ogre::TextureUnitState *TUS_Caustics = DM_Technique_Pass0->createTextureUnitState("Caustics.bmp");
			TUS_Caustics->setName("Caustics");
			TUS_Caustics->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);
			TUS_Caustics->setAnimatedTextureName("Caustics.bmp", 32, 1.5);
		}

		if (AutoUpdate)
		{
		    mDepthTechniques.push_back(Technique);
		}
	}

	void MaterialManager::addDepthTextureTechnique(Ogre::Technique *Technique, const Ogre::String& TextureName, const Ogre::String& AlphaChannel, const bool& AutoUpdate)
	{
		if (!Ogre::HighLevelGpuProgramManager::getSingleton().resourceExists(_def_DepthTexture_Shader_VP_Name+AlphaChannel))
		{
			_createDepthTextureGPUPrograms(mComponents, mOptions, AlphaChannel);
		}

		Technique->removeAllPasses();
		Technique->createPass();
		Technique->setName("_Hydrax_DepthTexture_Technique");
		Technique->setSchemeName("HydraxDepth");

		Ogre::Pass *DM_Technique_Pass0 = Technique->getPass(0);

		// Alpha channel will be stored in pass 0 name:
		DM_Technique_Pass0->setName(AlphaChannel);

		DM_Technique_Pass0->setVertexProgram(_def_DepthTexture_Shader_VP_Name+AlphaChannel);
		DM_Technique_Pass0->setFragmentProgram(_def_DepthTexture_Shader_FP_Name+AlphaChannel);

		DM_Technique_Pass0->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		DM_Technique_Pass0->setDepthCheckEnabled(true);
		DM_Technique_Pass0->setDepthWriteEnabled(false);

		Ogre::GpuProgramParametersSharedPtr VP_Parameters = DM_Technique_Pass0->getVertexProgramParameters();
		Ogre::GpuProgramParametersSharedPtr FP_Parameters = DM_Technique_Pass0->getFragmentProgramParameters();

        if(mOptions.SM != SM_GLSL)
        {
            VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
            VP_Parameters->setNamedAutoConstant("uCameraPos", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION);
        }
        VP_Parameters->setNamedAutoConstant("uWorld", Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
        VP_Parameters->setNamedConstant("uPlaneYPos", mHydrax->getPosition().y);
        VP_Parameters->setNamedConstant("uPlanesError", (float) mHydrax->getPlanesError());

		FP_Parameters->setNamedConstant("uDepthLimit", 1/mHydrax->getDepthLimit());
		FP_Parameters->setNamedConstant("uDistLimit", 1/mHydrax->getDistLimit());

		if (_isComponent(mComponents, HYDRAX_COMPONENT_CAUSTICS))
		{
			FP_Parameters->setNamedConstant("uCausticsScale", mHydrax->getCausticsScale());
			FP_Parameters->setNamedConstant("uCausticsEnd",   mHydrax->getCausticsEnd());

            if(mOptions.SM == SM_GLSL)
            {
                FP_Parameters->setNamedConstant("uCaustics", 0);
            }
			Ogre::TextureUnitState *TUS_Caustics = DM_Technique_Pass0->createTextureUnitState("Caustics.bmp");
			TUS_Caustics->setName("Caustics");
			TUS_Caustics->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);
			TUS_Caustics->setAnimatedTextureName("Caustics.bmp", 32, 1.5);
		}

        if(mOptions.SM == SM_GLSL)
        {
            FP_Parameters->setNamedConstant("uAlphaTex", 0);
        }
		Ogre::TextureUnitState *TUS_AlphaTex = DM_Technique_Pass0->createTextureUnitState(TextureName);
		TUS_AlphaTex->setName("_DetphTexture_Hydrax");
		TUS_AlphaTex->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

		if (AutoUpdate)
		{
		    mDepthTechniques.push_back(Technique);
		}
	}

	void MaterialManager::setCompositorEnable(const CompositorType &Compositor, const bool &Enable)
	{
		Ogre::CompositorPtr &Comp = mCompositors[static_cast<int>(Compositor)];

		if (Comp.isNull())
		{
			return;
		}

		Ogre::CompositorManager::getSingleton().
					setCompositorEnabled(mHydrax->getViewport(), Comp->getName(), Enable);

		mCompositorsEnable[static_cast<int>(Compositor)] = Enable;
	}

	bool MaterialManager::_isComponent(const HydraxComponent &List, const HydraxComponent &ToCheck) const
	{
		if (List & ToCheck)
        {
            return true;
        }

        if (ToCheck == HYDRAX_COMPONENTS_NONE && List == HYDRAX_COMPONENTS_NONE)
        {
            return true;
        }

        if (ToCheck == HYDRAX_COMPONENTS_ALL && List == HYDRAX_COMPONENTS_ALL)
        {
            return true;
        }

        return false;
	}

	void MaterialManager::setGpuProgramParameter(const GpuProgram &GpuP, const MaterialType &MType, const Ogre::String &Name, const Ogre::Real &Value)
	{
		if (!mCreated)
		{
			return;
		}

		Ogre::GpuProgramParametersSharedPtr Parameters;

		switch (GpuP)
		{
		    case GPUP_VERTEX:
			{
				Parameters = getMaterial(MType)->getTechnique(0)->getPass(0)->getVertexProgramParameters();
			}
			break;

			case GPUP_FRAGMENT:
			{
				Parameters = getMaterial(MType)->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
			}
			break;
		}

		Parameters->setNamedConstant(Name, Value);

		if (MType == MAT_DEPTH)
		{
			std::vector<Ogre::Technique*>::iterator TechIt;

			for(TechIt = mDepthTechniques.begin(); TechIt != mDepthTechniques.end(); TechIt++)
			{
				if (!(*TechIt))
				{
					mDepthTechniques.erase(TechIt);
					continue;
				}

				switch (GpuP)
				{
		  		    case GPUP_VERTEX:
				    {
					    Parameters = (*TechIt)->getPass(0)->getVertexProgramParameters();
				    }
				    break;

				    case GPUP_FRAGMENT:
				    {
					    Parameters = (*TechIt)->getPass(0)->getFragmentProgramParameters();
				    }
				    break;
			    }

				Parameters->setNamedConstant(Name, Value);
			}
		}
	}

	void MaterialManager::setGpuProgramParameter(const GpuProgram &GpuP, const MaterialType &MType, const Ogre::String &Name, const Ogre::Vector2 &Value)
	{
		if (!mCreated)
		{
			return;
		}

		Ogre::GpuProgramParametersSharedPtr Parameters;

		switch (GpuP)
		{
		    case GPUP_VERTEX:
			{
				Parameters = getMaterial(MType)->getTechnique(0)->getPass(0)->getVertexProgramParameters();
			}
			break;

			case GPUP_FRAGMENT:
			{
				Parameters = getMaterial(MType)->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
			}
			break;
		}

		float Value_[2] = {Value.x, Value.y};

		if (MType == MAT_DEPTH)
		{
			std::vector<Ogre::Technique*>::iterator TechIt;

			for(TechIt = mDepthTechniques.begin(); TechIt != mDepthTechniques.end(); TechIt++)
			{
				if (!(*TechIt))
				{
					mDepthTechniques.erase(TechIt);
					continue;
				}

				switch (GpuP)
				{
		  		    case GPUP_VERTEX:
				    {
					    Parameters = (*TechIt)->getPass(0)->getVertexProgramParameters();
				    }
				    break;

				    case GPUP_FRAGMENT:
				    {
					    Parameters = (*TechIt)->getPass(0)->getFragmentProgramParameters();
				    }
				    break;
			    }

				Parameters->setNamedConstant(Name, Value_, 1, 2);
			}
		}
	}

	void MaterialManager::setGpuProgramParameter(const GpuProgram &GpuP, const MaterialType &MType, const Ogre::String &Name, const Ogre::Vector3 &Value)
	{
		if (!mCreated)
		{
			return;
		}

		Ogre::GpuProgramParametersSharedPtr Parameters;

		switch (GpuP)
		{
		    case GPUP_VERTEX:
			{
				Parameters = getMaterial(MType)->getTechnique(0)->getPass(0)->getVertexProgramParameters();
			}
			break;

			case GPUP_FRAGMENT:
			{
				Parameters = getMaterial(MType)->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
			}
			break;
		}

		Parameters->setNamedConstant(Name, Value);

		if (MType == MAT_DEPTH)
		{
			std::vector<Ogre::Technique*>::iterator TechIt;

			for(TechIt = mDepthTechniques.begin(); TechIt != mDepthTechniques.end(); TechIt++)
			{
				if (!(*TechIt))
				{
					mDepthTechniques.erase(TechIt);
					continue;
				}

				switch (GpuP)
				{
		  		    case GPUP_VERTEX:
				    {
					    Parameters = (*TechIt)->getPass(0)->getVertexProgramParameters();
				    }
				    break;

				    case GPUP_FRAGMENT:
				    {
					    Parameters = (*TechIt)->getPass(0)->getFragmentProgramParameters();
				    }
				    break;
			    }

				Parameters->setNamedConstant(Name, Value);
			}
		}
	}

	void MaterialManager::UnderwaterCompositorListener::notifyMaterialSetup(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
	{
		if (mMaterialManager->_isComponent(mMaterialManager->mComponents, HYDRAX_COMPONENT_DEPTH))
	    {
			Ogre::Pass* DM_Technique0_Pass0 = mat->getTechnique(0)->getPass(0);
			DM_Technique0_Pass0->getTextureUnitState(2)->setTextureName("HydraxDepthMap");
	    }
	}

	void MaterialManager::UnderwaterCompositorListener::notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
	{
		const Ogre::Vector3& WC = mMaterialManager->mHydrax->getWaterColor();
		Ogre::CompositorPtr &UnderwaterCompositor = mMaterialManager->getCompositor(COMP_UNDERWATER);
		UnderwaterCompositor->getTechnique(0)->getTargetPass(0)->getPass(0)->setClearColour(Ogre::ColourValue(WC.x, WC.y, WC.z));

		Ogre::GpuProgramParametersSharedPtr FP_Parameters = mat->getTechnique(0)->getPass(0)->getFragmentProgramParameters();

		if (mMaterialManager->mHydrax->isComponent(HYDRAX_COMPONENT_DEPTH))
		{
			FP_Parameters->
				setNamedConstant("uWaterColor", mMaterialManager->mHydrax->getWaterColor());
		}

		if (mMaterialManager->mHydrax->isComponent(HYDRAX_COMPONENT_CAUSTICS))
		{
			FP_Parameters->
				setNamedConstant("uCausticsPower", mMaterialManager->mHydrax->getCausticsPower());
		}

		if (mMaterialManager->mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER_GODRAYS))
		{
			FP_Parameters->
				setNamedConstant("uSunColor", mMaterialManager->mHydrax->getSunColor());

			FP_Parameters->setNamedConstant("uLightDirection",
				   (mMaterialManager->mHydrax->getMesh()->getObjectSpacePosition(mMaterialManager->mHydrax->getCamera()->getParentSceneNode()->getPosition()) -
				    mMaterialManager->mHydrax->getMesh()->getObjectSpacePosition(mMaterialManager->mHydrax->getSunPosition()))
					.normalisedCopy());

			FP_Parameters->setNamedConstant("uIntensity", mMaterialManager->mHydrax->getGodRaysIntensity());
		    FP_Parameters->setNamedConstant("uHGg", mMaterialManager->mHydrax->getGodRaysExposure());

			Ogre::GpuProgramParametersSharedPtr VP_Parameters = mat->getTechnique(0)->getPass(0)->getVertexProgramParameters();

		    // FAR_LEFT_TOP
		    VP_Parameters->
			    setNamedConstant( "uCorner0", mMaterialManager->mHydrax->getCamera()->getWorldSpaceCorners()[5] );
		    // FAR_RIGHT_TOP - FAR_LEFT_TOP
		    VP_Parameters->
		    	setNamedConstant( "uCorner01",  mMaterialManager->mHydrax->getCamera()->getWorldSpaceCorners()[4] -  mMaterialManager->mHydrax->getCamera()->getWorldSpaceCorners()[5]);
		    // FAR_LEFT_BOTTOM - FAR_LEFT_TOP
		    VP_Parameters->
			    setNamedConstant( "uCorner02",  mMaterialManager->mHydrax->getCamera()->getWorldSpaceCorners()[6] - mMaterialManager->mHydrax->getCamera()->getWorldSpaceCorners()[5]);
		}

		/*
        if (mMaterialManager->_isComponent(mMaterialManager->mComponents, HYDRAX_COMPONENT_DEPTH))
        {
            Ogre::Viewport *Viewport = mMaterialManager->mHydrax->getCamera()->getViewport();
        }
		*/

		if (mMaterialManager->mCompositorsNeedToBeReloaded[COMP_UNDERWATER])
		{
			if (mMaterialManager->_isComponent(mMaterialManager->mComponents, HYDRAX_COMPONENT_DEPTH))
		    {
                Ogre::Pass* DM_Technique0_Pass0 = mat->getTechnique(0)->getPass(0);
				DM_Technique0_Pass0->getTextureUnitState(2)->setTextureName("HydraxDepthMap");
		    }

			mMaterialManager->mCompositorsNeedToBeReloaded[COMP_UNDERWATER] = false;
		}
	}
}
