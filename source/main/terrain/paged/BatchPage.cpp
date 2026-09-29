/*-------------------------------------------------------------------------------------
Copyright (c) 2006 John Judnich

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
	1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------------*/

//BatchPage.cpp
//BatchPage is an extension to PagedGeometry which displays entities as static geometry.
//-------------------------------------------------------------------------------------

#include <OgreRoot.h>
#include <OgreCamera.h>
#include <OgreQuaternion.h>
#include <OgreEntity.h>
#include <OgreRenderSystem.h>
#include <OgreRenderSystemCapabilities.h>
#include <OgreHighLevelGpuProgram.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreLogManager.h>
#include <OgreTechnique.h>

#include "BatchPage.h"
#include "BatchedGeometry.h"


using namespace Ogre;
using namespace Forests;



unsigned long BatchPage::s_nRefCount = 0;
unsigned long BatchPage::s_nGUID = 0;


//-----------------------------------------------------------------------------
/// Default constructor
BatchPage::BatchPage() :
m_pPagedGeom         (NULL),
m_pSceneMgr          (NULL),
m_pBatchGeom         (NULL),
m_nLODLevel          (0),
m_bFadeEnabled       (false),
m_bShadersSupported  (false),
m_fVisibleDist       (Ogre::Real(0.)),
m_fInvisibleDist     (Ogre::Real(0.))
{
   // empty
}


//-----------------------------------------------------------------------------
///
void BatchPage::init(PagedGeometry *geom_, const Any &data)
{
   assert(geom_ && "Can any code set null pointer?");

   int datacast = data.has_value() ? Ogre::any_cast<int>(data) : 0;
#ifdef _DEBUG
	if (datacast < 0)
		OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,"Data of BatchPage must be a positive integer. It representing the LOD level this detail level stores.","BatchPage::BatchPage");
#endif

   m_pPagedGeom   = geom_;
	m_pSceneMgr    = m_pPagedGeom->getSceneManager();
   m_pBatchGeom   = new BatchedGeometry(m_pSceneMgr, m_pPagedGeom->getSceneNode());
   m_nLODLevel    = datacast;
	m_bFadeEnabled = false;

	if (!m_pPagedGeom->getShadersEnabled())
		m_bShadersSupported = false;     // shaders disabled by config
   else
	{
		// determine if shaders available
		const RenderSystemCapabilities *caps = Root::getSingleton().getRenderSystem()->getCapabilities();
      // For example, GeForce4MX has vertex shadres 1.1 and no pixel shaders
      m_bShadersSupported = caps->hasCapability(RSC_VERTEX_PROGRAM) ? true : false;
	}

	++s_nRefCount;
}

BatchPage::~BatchPage()
{
	delete m_pBatchGeom;
	//unfadedMaterials.clear();  // Delete unfaded material references
}


//-----------------------------------------------------------------------------
///
void BatchPage::addEntity(Entity *ent, const Vector3 &position, const Quaternion &rotation,
                          const Vector3 &scale, const Ogre::ColourValue &color)
{
	const size_t numManLod = ent->getNumManualLodLevels();

#ifdef _DEBUG
	//Warns if using LOD batch and entities does not have enough LOD support.
	if (m_nLODLevel > 0 && numManLod < m_nLODLevel)
   {
		Ogre::LogManager::getSingleton().logMessage("BatchPage::addEntity: " + ent->getName() +
         " entity has less than " + Ogre::StringConverter::toString(m_nLODLevel) +
         " manual lod level(s). Performance warning.");
   }
#endif

	if (m_nLODLevel == 0 || numManLod == 0)
		m_pBatchGeom->addEntity(ent, position, rotation, scale, color);
	else
	{
		const size_t bestLod = numManLod < m_nLODLevel - 1 ? numManLod : m_nLODLevel - 1;
		Ogre::Entity * lod = ent->getManualLodLevel(bestLod);
		m_pBatchGeom->addEntity(lod, position, rotation, scale, color);
	}
}


//-----------------------------------------------------------------------------
///
void BatchPage::build()
{
	m_pBatchGeom->build();
	BatchedGeometry::TSubBatchIterator it = m_pBatchGeom->getSubBatchIterator();

	while (it.hasMoreElements())
   {
		BatchedGeometry::SubBatch *subBatch = it.getNext();
		const MaterialPtr &ptrMat = subBatch->getMaterial();

		//Disable specular unless a custom shader is being used.
		//This is done because the default shader applied by BatchPage
		//doesn't support specular, and fixed-function needs to look
		//the same as the shader (for computers with no shader support)
		for (unsigned short t = 0, tCnt = ptrMat->getNumTechniques(); t < tCnt; ++t)
      {
			Technique *tech = ptrMat->getTechnique(t);
			for (unsigned short p = 0, pCnt = tech->getNumPasses(); p < pCnt; ++p)
         {
				Pass *pass = tech->getPass(p);
				//if (pass->getVertexProgramName() == "")
				//	pass->setSpecular(0, 0, 0, 1);
            if (!pass->hasVertexProgram())
               pass->setSpecular(0.f, 0.f, 0.f, 1.f);
			}
		}

		//Store the original materials
		m_vecUnfadedMaterials.push_back(subBatch->getMaterial());
	}

	_updateShaders();
}


//-----------------------------------------------------------------------------
///
void BatchPage::removeEntities()
{
	m_pBatchGeom->clear();
	m_vecUnfadedMaterials.clear();
	m_bFadeEnabled = false;
}

//-----------------------------------------------------------------------------
///
void BatchPage::setVisible(bool visible)
{
	m_pBatchGeom->setVisible(visible);
}


//-----------------------------------------------------------------------------
///
void BatchPage::setFade(bool enabled, Real visibleDist, Real invisibleDist)
{
	if (!m_bShadersSupported)
		return;

	//If fade status has changed...
	if (m_bFadeEnabled != enabled)
	{
		m_bFadeEnabled = enabled;

 		if (enabled)
			//Transparent batches should render after impostors
         m_pBatchGeom->setRenderQueueGroup(m_pPagedGeom ? m_pPagedGeom->getRenderQueue() : RENDER_QUEUE_6);
      else
         //Opaque batches should render in the normal render queue
         m_pBatchGeom->setRenderQueueGroup(RENDER_QUEUE_MAIN);

		m_fVisibleDist    = visibleDist;
		m_fInvisibleDist  = invisibleDist;
		_updateShaders();
	}
}


//-----------------------------------------------------------------------------
///
void BatchPage::_updateShaders()
{
	if (!m_bShadersSupported)
		return;

	unsigned int i = 0;
	BatchedGeometry::TSubBatchIterator it = m_pBatchGeom->getSubBatchIterator();
	while (it.hasMoreElements())
   {
		BatchedGeometry::SubBatch *subBatch = it.getNext();
		const MaterialPtr &ptrMat = m_vecUnfadedMaterials[i++];

		//Check if lighting should be enabled
		bool lightingEnabled = false;
		for (unsigned short t = 0, tCnt = ptrMat->getNumTechniques(); t < tCnt; ++t)
      {
			Technique *tech = ptrMat->getTechnique(t);
			for (unsigned short p = 0, pCnt = tech->getNumPasses(); p < pCnt; ++p)
         {
				if (tech->getPass(p)->getLightingEnabled())
            {
					lightingEnabled = true;
					break;
				}
			}
			if (lightingEnabled)
				break;
		}

		//Compile the CG shader script based on various material / fade options
		Ogre::StringStream tmpName;
		tmpName << "BatchPage_";
		if (m_bFadeEnabled)
			tmpName << "fade_";
		if (lightingEnabled)
			tmpName << "lit_";
		if (subBatch->m_pVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE) != NULL)
			tmpName << "clr_";

		for (size_t i = 0, iCnt = subBatch->m_pVertexData->vertexDeclaration->getElementCount(); i < iCnt; ++i)
		{
			const VertexElement *el = subBatch->m_pVertexData->vertexDeclaration->getElement(i);
			if (el->getSemantic() == VES_TEXTURE_COORDINATES)
         {
				String uvType;
            switch (el->getType())
            {
            case VET_FLOAT1: uvType = "1"; break;
            case VET_FLOAT2: uvType = "2"; break;
            case VET_FLOAT3: uvType = "3"; break;
            case VET_FLOAT4: uvType = "4"; break;
			default:
				OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Unsupported texture coordinate type");
            }
            tmpName << uvType << '_';
			}
		}

		tmpName << "vp";

		const String vertexProgName = tmpName.str();
		String shaderLanguage = m_pPagedGeom->getShaderLanguage();

		//If the shader hasn't been created yet, create it
		if (!HighLevelGpuProgramManager::getSingleton().getByName(vertexProgName))
		{
			String defines;
			if (lightingEnabled) defines += "LIGHTING,";
			if (subBatch->m_pVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)) defines += "VERTEXCOLOUR,";
			if (m_bFadeEnabled) defines += "FADE,";

			HighLevelGpuProgramPtr vertexShader = HighLevelGpuProgramManager::getSingleton().createProgram(
				vertexProgName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, shaderLanguage, GPT_VERTEX_PROGRAM);

			// RIGSOFRODS: Load by filename rather than `setSource(openFileStream(...))`, so the stream goes through
			// `RoR::ContentManager::resourceStreamOpened()` which backports 'OgreUnifiedShader.h' support.
			vertexShader->setSourceFile("BatchPage_vp.glsl");
			// END RIGSOFRODS
			vertexShader->setParameter("preprocessor_defines", defines);

			if (shaderLanguage == "hlsl")
			{
				vertexShader->setParameter("target", "vs_1_1");
			}

			vertexShader->load();
		}

		//Now that the shader is ready to be applied, apply it
		Ogre::StringStream materialSignature;
		materialSignature << "BatchMat|";
		materialSignature << ptrMat->getName() << "|";
		if (m_bFadeEnabled)
      {
			materialSignature << m_fVisibleDist << "|";
			materialSignature << m_fInvisibleDist << "|";
		}

		//Search for the desired material
		MaterialPtr generatedMaterial = MaterialManager::getSingleton().getByName(materialSignature.str());
		if (!generatedMaterial)
      {
			//Clone the material
			generatedMaterial = ptrMat->clone(materialSignature.str());

			//And apply the fade shader
			for (unsigned short t = 0, tCnt = generatedMaterial->getNumTechniques(); t < tCnt; ++t)
         {
				Technique *tech = generatedMaterial->getTechnique(t);
				for (unsigned short p = 0, pCnt = tech->getNumPasses(); p < pCnt; ++p)
            {
					Pass *pass = tech->getPass(p);

					//Setup vertex program
					//if (pass->getVertexProgramName() == "")
					//	pass->setVertexProgram(vertexProgName);
               if (!pass->hasVertexProgram())
                  pass->setVertexProgram(vertexProgName);
				pass->setFragmentProgram(m_pPagedGeom->getFragmentProgramName());

					try
               {
						GpuProgramParametersSharedPtr params = pass->getVertexProgramParameters();

						if (lightingEnabled)
                  {
							params->setNamedAutoConstant("objSpaceLight", GpuProgramParameters::ACT_LIGHT_POSITION_OBJECT_SPACE);
							params->setNamedAutoConstant("lightDiffuse", GpuProgramParameters::ACT_DERIVED_LIGHT_DIFFUSE_COLOUR);
							params->setNamedAutoConstant("lightAmbient", GpuProgramParameters::ACT_DERIVED_AMBIENT_LIGHT_COLOUR);
							//params->setNamedAutoConstant("matAmbient", GpuProgramParameters::ACT_SURFACE_AMBIENT_COLOUR);
						}

                        params->setNamedAutoConstant("worldViewProj", GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);

						if (m_bFadeEnabled)
						{
							params->setNamedAutoConstant("camPos", GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);

							//Set fade ranges
							params->setNamedAutoConstant("invisibleDist", GpuProgramParameters::ACT_CUSTOM);
							params->setNamedConstant("invisibleDist", m_fInvisibleDist);

							params->setNamedAutoConstant("fadeGap", GpuProgramParameters::ACT_CUSTOM);
							params->setNamedConstant("fadeGap", m_fInvisibleDist - m_fVisibleDist);

							if (pass->getAlphaRejectFunction() == CMPF_ALWAYS_PASS)
								pass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
						}
					}
					catch (...)
               {
						OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                     "Error configuring batched geometry transitions. If you're using materials with custom vertex shaders, \
                     they will need to implement fade transitions to be compatible with BatchPage.", "BatchPage::_updateShaders()");
					}
				}
			}

		}

		//Apply the material
		subBatch->setMaterial(generatedMaterial);
	}

}
