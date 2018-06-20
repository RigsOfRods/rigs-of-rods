#include "OgreShaderParticleRenderer.h"
#include "OgreParticleCustomParam.h"

#include <OgreParticle.h>
#include <OgreStringConverter.h>
#include <OgreTagPoint.h>
#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

namespace Ogre {
//////////////////////////////////////////////////////////////////////////

ShaderParticleRenderer::CmdVertexFormatColour			ShaderParticleRenderer::msVertexFmtColour;
ShaderParticleRenderer::CmdVertexFormatTexture			ShaderParticleRenderer::msVertexFmtTexture;
ShaderParticleRenderer::CmdVertexFormatSize				ShaderParticleRenderer::msVertexFmtSize;
ShaderParticleRenderer::CmdVertexFormatRotation			ShaderParticleRenderer::msVertexFmtRotation;
ShaderParticleRenderer::CmdVertexFormatRotationSpeed	ShaderParticleRenderer::msVertexFmtRotationSpeed;
ShaderParticleRenderer::CmdVertexFormatDirection		ShaderParticleRenderer::msVertexFmtDirection;
ShaderParticleRenderer::CmdVertexFormatTTL				ShaderParticleRenderer::msVertexFmtTTL;
ShaderParticleRenderer::CmdVertexFormatTotalTTL			ShaderParticleRenderer::msVertexFmtTotalTTL;
ShaderParticleRenderer::CmdVertexFormatTimeFrag			ShaderParticleRenderer::msVertexFmtTimeFrag;
ShaderParticleRenderer::CmdVertexFormatTimeFragInv		ShaderParticleRenderer::msVertexFmtTimeFragInv;

//////////////////////////////////////////////////////////////////////////
ShaderParticleRenderer::ShaderParticleRenderer()
    : mDefaultParticleSize(1, 1)
    , mKeepInLocalSpace(false)
    , mParentNode(NULL)
    , mParentIsTagPoint(false)
    , mSortMode(SM_DISTANCE)
    , mLightListUpdated(0)
    , mRadius(0.0f)
    , mRenderQueueID(RENDER_QUEUE_MAIN)
    , mVertexFormatTexture(false)
    , mVertexFormatSize(false)
    , mVertexFormatRotation(false)
    , mVertexFormatRotationSpeed(false)
    , mVertexFormatDirection(false)
    , mVertexFormatColour(false	)
    , mVertexFormatTTL(false)
    , mVertexFormatTotalTTL(false)
    , mVertexFormatTimeFragment(false)
    , mVertexFormatTimeFragmentInv(false)
    , mVertexSize(0)
{
    if (createParamDictionary("ShaderParticleRenderer"))
    {
        ParamDictionary* dict = getParamDictionary();
        dict->addParameter(ParameterDef("diffuse_colour",
            "Adds diffuse colour to vertex format (type = float4)",
            PT_BOOL),
            &msVertexFmtColour);

        dict->addParameter(ParameterDef("texture_coord",
            "Adds general texture coordinate to vertex format (type = float2)",
            PT_BOOL),
            &msVertexFmtTexture);

        dict->addParameter(ParameterDef("particle_size",
            "Adds particle size to vertex format (type = float2)",
            PT_BOOL),
            &msVertexFmtSize);

        dict->addParameter(ParameterDef("particle_rotation",
            "Adds particle rotation (in radians) to vertex format (type = float1)"
            "note: if particle_rotation_speed present in script, they are packed together as float2",
            PT_BOOL),
            &msVertexFmtRotation);

        dict->addParameter(ParameterDef("particle_rotation_speed",
            "Adds particle rotation speed (in radians/s) to vertex format (type = float1)"
            "note: if particle_rotation present in script, they are packed together as float2",
            PT_BOOL),
            &msVertexFmtRotationSpeed);

        dict->addParameter(ParameterDef("particle_direction",
            "Adds particle direction to vertex format (type = float3)"
            "note: it represent particle speed",
            PT_BOOL),
            &msVertexFmtDirection);

        dict->addParameter(ParameterDef("time_to_live",
            "Adds particle current time to live to vertex format (type = float1)"
            "note: this parameter can be packed with total_time_to_live, time_frag and time_frag_inv",
            PT_BOOL),
            &msVertexFmtTTL);

        dict->addParameter(ParameterDef("total_time_to_live",
            "Adds particle total time to live to vertex format (type = float1)"
            "note: this parameter can be packed with time_to_live, time_frag and time_frag_inv",
            PT_BOOL),
            &msVertexFmtTotalTTL);

        dict->addParameter(ParameterDef("time_frag",
            "Adds particle time fragment to vertex format (type = float1), which is ratio between ttl and total ttl"
            "note: this parameter can be packed with time_to_live, total_time_to_live and time_frag_inv",
            PT_BOOL),
            &msVertexFmtTimeFrag);

        dict->addParameter(ParameterDef("time_frag_inv",
            "Adds particle inverse time fragment to vertex format (type = float1), which is 1.0f - time_frag"
            "note: this parameter can be packed with time_to_live, total_time_to_live and time_frag",
            PT_BOOL),
            &msVertexFmtTimeFragInv);
    }

    mVertexData = OGRE_NEW VertexData;
    mIndexData  = OGRE_NEW IndexData;

    mTexCoordTable[0] = Vector2(0, 0);
    mTexCoordTable[1] = Vector2(1, 0);
    mTexCoordTable[2] = Vector2(1, 1);
    mTexCoordTable[3] = Vector2(0, 1);
}

//////////////////////////////////////////////////////////////////////////
ShaderParticleRenderer::~ShaderParticleRenderer()
{
    OGRE_DELETE mVertexData;
    OGRE_DELETE mIndexData;
}

//////////////////////////////////////////////////////////////////////////
const String& ShaderParticleRenderer::getType(void) const
{
    return rendererTypeName;
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_updateRenderQueue(RenderQueue* queue, Ogre::list<Particle*>::type& currentParticles, bool cullIndividually)
{
    // be sure that we have enough space in buffers
    if (!allocateBuffers(currentParticles.size())) {
        assert(0 && "Cannot allocate buffers");
        return;
    }

    // update vertex data
    mRadius = 0.0f;
    if (!currentParticles.empty()) {
        HardwareVertexBufferSharedPtr pVB = mVertexData->vertexBufferBinding->getBuffer(0);
        uchar* pDataVB  = reinterpret_cast<uchar*>(pVB->lock(HardwareBuffer::HBL_DISCARD));
        for (Ogre::list<Particle*>::type::iterator it=currentParticles.begin(); it!=currentParticles.end(); ++it) {
            Particle* pParticle = *it;
            addParticle(pDataVB, *pParticle);
            pDataVB += 4 * mVertexSize;

            float fDist = (mParentNode != NULL) ? mParentNode->getPosition().distance(pParticle->mPosition) : pParticle->mPosition.length();
            if (fDist > mRadius)
                mRadius = fDist;
        }
        pVB->unlock();
    }

    // setup counts
    mVertexData->vertexCount = currentParticles.size() * 4;
    mIndexData->indexCount   = currentParticles.size() * 6;

    // update render queue
    queue->addRenderable(this, mRenderQueueID);
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::visitRenderables(Renderable::Visitor* visitor, bool debugRenderables)
{
    visitor->visit(this, 0, debugRenderables);
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_setMaterial(MaterialPtr& mat)
{
    mMaterial = mat;
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_notifyCurrentCamera(Camera* cam)
{
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_notifyParticleRotated(void)
{
    // nothing to do
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_notifyParticleResized(void)
{
    // nothing to do
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_notifyParticleQuota(size_t quota)
{
    // nothing to do - hardware buffers will be altered run-time
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_notifyAttached(Node* parent, bool isTagPoint)
{
    mParentNode = parent;
    mParentIsTagPoint = isTagPoint;
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_notifyDefaultDimensions(Real width, Real height)
{
    mDefaultParticleSize.x = width;
    mDefaultParticleSize.y = height;
}

//////////////////////////////////////////////////////////////////////////
ParticleVisualData* ShaderParticleRenderer::_createVisualData(void)
{
    return OGRE_NEW ParticleCustomParam();
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::_destroyVisualData(ParticleVisualData* vis)
{
    OGRE_DELETE vis;
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::setRenderQueueGroup(uint8 queueID)
{
    mRenderQueueID = queueID;
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::setKeepParticlesInLocalSpace(bool keepLocal)
{
    mKeepInLocalSpace = keepLocal;
}

//////////////////////////////////////////////////////////////////////////
SortMode ShaderParticleRenderer::_getSortMode(void) const
{
    return mSortMode;
}

//////////////////////////////////////////////////////////////////////////
const MaterialPtr& ShaderParticleRenderer::getMaterial(void) const
{
    return mMaterial;
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::getRenderOperation(RenderOperation& op)
{
    assert(mVertexData != NULL);
    assert(mIndexData != NULL);
    op.vertexData    = mVertexData;
    op.indexData     = mIndexData;
    op.operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
    op.useIndexes    = true;
    op.srcRenderable = this;
}

////////////////////////////////////////////////////////////////////////////
//unsigned short ShaderParticleRenderer::getNumWorldTransforms(void) const
//{
//	if (mKeepInLocalSpace && mParentNode != NULL)
//	{
//		return mParentNode->getNumWorldTransforms();
//	} else
//	{
//		return 1;
//	}
//}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::getWorldTransforms(Matrix4* xform) const
{
    if (mKeepInLocalSpace && mParentNode != NULL)
    {
        *xform = mParentNode->_getFullTransform();
        //mParentNode->getWorldTransforms(xform);
    } else
    {
        *xform = Matrix4::IDENTITY;
    }
}

//////////////////////////////////////////////////////////////////////////
Real ShaderParticleRenderer::getSquaredViewDepth(const Camera* cam) const
{
    if (mParentNode != NULL) {
        return cam->getRealPosition().squaredDistance(mParentNode->getPosition());
    } else {
        return 0.0f;
    }
}

//////////////////////////////////////////////////////////////////////////
const LightList& ShaderParticleRenderer::getLights(void) const
{
    if (mParentNode != NULL) {
        // Query from parent entity if exists
        if (mParentIsTagPoint)
        {
            TagPoint* tp = static_cast<TagPoint*>(mParentNode);
            return tp->getParentEntity()->queryLights();
        }

        if (mParentNode)
        {
            SceneNode* sn = static_cast<SceneNode*>(mParentNode);

            // Make sure we only update this only if need.
            ulong frame = sn->getCreator()->_getLightsDirtyCounter();
            if (mLightListUpdated != frame)
            {
                mLightListUpdated = frame;
                sn->findLights(mLightList, mRadius);
            }
        }
        else
        {
            mLightList.clear();
        }
    }
    else
    {
        mLightList.clear();
    }

    return mLightList;
}

//////////////////////////////////////////////////////////////////////////
bool ShaderParticleRenderer::allocateBuffers(size_t iNumParticles)
{
    // prepare vertex declaration
    if (mVertexData->vertexDeclaration->getElementCount() == 0) {
        VertexDeclaration* pDecl = mVertexData->vertexDeclaration;
        size_t ofs = 0;
        ofs += pDecl->addElement(0, ofs, VET_FLOAT4, VES_POSITION).getSize();				// position
        if (mVertexFormatColour)
            ofs += pDecl->addElement(0, ofs, VET_FLOAT4, VES_DIFFUSE).getSize();			// diffuse colour

        // other data are stored in vertex as texture coordinates
        ushort ix = 0;
        if (mVertexFormatTexture)
            ofs += pDecl->addElement(0, ofs, VET_FLOAT2, VES_TEXTURE_COORDINATES, ix++).getSize();	// general texture coord

        if (mVertexFormatSize)
            ofs += pDecl->addElement(0, ofs, VET_FLOAT2, VES_TEXTURE_COORDINATES, ix++).getSize();	// particle size

        if (mVertexFormatRotation || mVertexFormatRotationSpeed) {
            if (mVertexFormatRotation && mVertexFormatRotationSpeed)
                ofs += pDecl->addElement(0, ofs, VET_FLOAT2, VES_TEXTURE_COORDINATES, ix++).getSize();	// current rotation and rotation speed
            else
                ofs += pDecl->addElement(0, ofs, VET_FLOAT1, VES_TEXTURE_COORDINATES, ix++).getSize();	// current rotation or rotation speed
        }

        if (mVertexFormatDirection)
            ofs += pDecl->addElement(0, ofs, VET_FLOAT3, VES_TEXTURE_COORDINATES, ix++).getSize();	// particle direction (as speed)

        // add packed times
        size_t iNumTimes = 0;
        if (mVertexFormatTTL) iNumTimes++;
        if (mVertexFormatTotalTTL) iNumTimes++;
        if (mVertexFormatTimeFragment) iNumTimes++;
        if (mVertexFormatTimeFragmentInv) iNumTimes++;
        switch(iNumTimes) {
            case 1:
                ofs += pDecl->addElement(0, ofs, VET_FLOAT1, VES_TEXTURE_COORDINATES, ix++).getSize();
                break;
            case 2:
                ofs += pDecl->addElement(0, ofs, VET_FLOAT2, VES_TEXTURE_COORDINATES, ix++).getSize();
                break;
            case 3:
                ofs += pDecl->addElement(0, ofs, VET_FLOAT3, VES_TEXTURE_COORDINATES, ix++).getSize();
                break;
            case 4:
                ofs += pDecl->addElement(0, ofs, VET_FLOAT4, VES_TEXTURE_COORDINATES, ix++).getSize();
                break;
        }

        // add custom parameters
        ofs += pDecl->addElement(0, ofs, VET_FLOAT4, VES_TEXTURE_COORDINATES, ix++).getSize();
        assert(ix <= 8);

        // cache vertex size
        mVertexSize = pDecl->getVertexSize(0);
    }

    Ogre::HardwareVertexBufferSharedPtr pVB;
    if (mVertexData->vertexBufferBinding->isBufferBound(0))
        pVB = mVertexData->vertexBufferBinding->getBuffer(0);

    // prepare vertex buffer
    if (pVB.isNull() || pVB->getNumVertices() < iNumParticles * 4) {
        assert(iNumParticles * 4 < 65536); // we are using 16bit index buffer
        pVB = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(mVertexSize, 4 * iNumParticles, Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
        if (pVB.isNull())
            return false;

        mVertexData->vertexBufferBinding->setBinding(0, pVB);
    }

    // prepare index buffer
    Ogre::HardwareIndexBufferSharedPtr pIB = mIndexData->indexBuffer;
    if (pIB.isNull() || pIB->getNumIndexes() < iNumParticles * 6) {
        pIB = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, iNumParticles * 6, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
        if (pIB.isNull())
            return false;

        mIndexData->indexBuffer = pIB;

        // fill
        Ogre::uint16* pDataIB = reinterpret_cast<Ogre::uint16*>(pIB->lock(Ogre::HardwareBuffer::HBL_NORMAL));
        for (Ogre::uint16 k=0; k<static_cast<Ogre::uint16>(iNumParticles); ++k) {
            pDataIB[0] = k*4 + 0;
            pDataIB[1] = k*4 + 1;
            pDataIB[2] = k*4 + 2;

            pDataIB[3] = k*4 + 0;
            pDataIB[4] = k*4 + 2;
            pDataIB[5] = k*4 + 3;
            pDataIB += 6;
        }
        pIB->unlock();
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRenderer::addParticle(uint8* pDataVB, const Particle& particle) const
{
    // position
    size_t ofs = 0;
    for (int k=0; k<4; ++k) {
        float* pPosition = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
        pPosition[0] = particle.mPosition.x;
        pPosition[1] = particle.mPosition.y;
        pPosition[2] = particle.mPosition.z;
        pPosition[3] = k;
    }
    ofs += sizeof(float) * 4;

    // diffuse colour
    if (mVertexFormatColour) {
        for (int k=0; k<4; ++k) {
            float* pColour = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            pColour[0] = particle.mColour.r;
            pColour[1] = particle.mColour.g;
            pColour[2] = particle.mColour.b;
            pColour[3] = particle.mColour.a;
        }
        ofs += sizeof(float) * 4;
    }

    // general texture coordinates
    if (mVertexFormatTexture) {
        for (int k=0; k<4; ++k) {
            float* pTexCoord = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            pTexCoord[0] = mTexCoordTable[k].x;
            pTexCoord[1] = mTexCoordTable[k].y;
        }
        ofs += sizeof(float) * 2;
    }

    // particle size
    if (mVertexFormatSize) {
        float w = mDefaultParticleSize.x;
        float h = mDefaultParticleSize.y;
        if (particle.hasOwnDimensions()) {
            w = particle.getOwnWidth();
            h = particle.getOwnHeight();
        }

        for (int k=0; k<4; ++k) {
            float* pSize = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            pSize[0] = w;
            pSize[1] = h;
        }
        ofs += sizeof(float) * 2;
    }

    // particle rotation
    if (mVertexFormatRotation) {
        for (int k=0; k<4; ++k) {
            float* pRotation = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            *pRotation = particle.mRotation.valueRadians();
        }
        ofs += sizeof(float);
    }

    // particle rotation speed
    if (mVertexFormatRotationSpeed) {
        for (int k=0; k<4; ++k) {
            float* pRotation = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            *pRotation = particle.mRotationSpeed.valueRadians();
        }
        ofs += sizeof(float);
    }

    // direction
    if (mVertexFormatDirection) {
        for (int k=0; k<4; ++k) {
            float* pDirection = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            pDirection[0] = particle.mDirection.x;
            pDirection[1] = particle.mDirection.y;
            pDirection[2] = particle.mDirection.z;
        }
        ofs += sizeof(float) * 3;
    }
        
    // time to live
    if (mVertexFormatTTL) {
        for (int k=0; k<4; ++k) {
            float* pTime = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            *pTime = particle.mTimeToLive;
        }
        ofs += sizeof(float);
    }
        
    // total time to live
    if (mVertexFormatTTL) {
        for (int k=0; k<4; ++k) {
            float* pTime = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            *pTime = particle.mTotalTimeToLive;
        }
        ofs += sizeof(float);
    }
        
    // time fragment
    if (mVertexFormatTimeFragment) {
        float fFrag = particle.mTimeToLive / particle.mTotalTimeToLive;
        for (int k=0; k<4; ++k) {
            float* pTime = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            *pTime = fFrag;
        }
        ofs += sizeof(float);
    }
        
    // inverse time fragment
    if (mVertexFormatTimeFragmentInv) {
        float fFrag = 1.0f - particle.mTimeToLive / particle.mTotalTimeToLive;
        for (int k=0; k<4; ++k) {
            float* pTime = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
            *pTime = fFrag;
        }
        ofs += sizeof(float);
    }

    // custom parameter
    ParticleCustomParam* pCustom = static_cast<ParticleCustomParam*>(particle.getVisualData());
    const Vector4& customData = pCustom != NULL ? pCustom->paramValue : Vector4::ZERO;
    for (int k=0; k<4; ++k) {
        float* pParams = reinterpret_cast<float*>(pDataVB + k*mVertexSize + ofs);
        pParams[0] = customData.x;
        pParams[1] = customData.y;
        pParams[2] = customData.z;
        pParams[3] = customData.w;
    }
    ofs += sizeof(float) * 4;

    // if you see this assert some informations were not added into vertex buffer !!!
    assert(ofs == mVertexSize);
}

/************************************************************************/
/* ShaderParticleRendererFactory implementation                         */
/************************************************************************/
const String& ShaderParticleRendererFactory::getType() const
{
    return rendererTypeName;
}

//////////////////////////////////////////////////////////////////////////
ParticleSystemRenderer* ShaderParticleRendererFactory::createInstance( const String& name )
{
    return OGRE_NEW ShaderParticleRenderer();
}

//////////////////////////////////////////////////////////////////////////
void ShaderParticleRendererFactory::destroyInstance( ParticleSystemRenderer* inst)
{
    OGRE_DELETE inst;
}

/************************************************************************/
/* ParamCommand implementations                                         */
/************************************************************************/
String ShaderParticleRenderer::CmdVertexFormatColour::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatColour() );
}
void ShaderParticleRenderer::CmdVertexFormatColour::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatColour(
        StringConverter::parseBool(val));
}
void ShaderParticleRenderer::setRenderQueueGroupAndPriority(Ogre::uint8,Ogre::ushort)
{
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatTexture::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatTexture() );
}
void ShaderParticleRenderer::CmdVertexFormatTexture::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatTexture(
        StringConverter::parseBool(val));
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatSize::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatSize() );
}
void ShaderParticleRenderer::CmdVertexFormatSize::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatSize(
        StringConverter::parseBool(val));
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatRotation::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatRotation() );
}
void ShaderParticleRenderer::CmdVertexFormatRotation::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatRotation(
        StringConverter::parseBool(val));
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatRotationSpeed::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatRotationSpeed() );
}
void ShaderParticleRenderer::CmdVertexFormatRotationSpeed::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatRotationSpeed(
        StringConverter::parseBool(val));
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatDirection::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatDirection() );
}
void ShaderParticleRenderer::CmdVertexFormatDirection::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatDirection(
        StringConverter::parseBool(val));
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatTTL::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatTTL() );
}
void ShaderParticleRenderer::CmdVertexFormatTTL::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatTTL(
        StringConverter::parseBool(val));
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatTotalTTL::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatTotalTTL() );
}
void ShaderParticleRenderer::CmdVertexFormatTotalTTL::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatTotalTTL(
        StringConverter::parseBool(val));
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatTimeFrag::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatTimeFragment() );
}
void ShaderParticleRenderer::CmdVertexFormatTimeFrag::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatTimeFragment(
        StringConverter::parseBool(val));
}

//////////////////////////////////////////////////////////////////////////
String ShaderParticleRenderer::CmdVertexFormatTimeFragInv::doGet(const void* target) const
{
    return StringConverter::toString(
        static_cast<const ShaderParticleRenderer*>(target)->getVertexFormatTimeFragmentInv() );
}
void ShaderParticleRenderer::CmdVertexFormatTimeFragInv::doSet(void* target, const String& val)
{
    static_cast<ShaderParticleRenderer*>(target)->setVertexFormatTimeFragmentInv(
        StringConverter::parseBool(val));
}
}
