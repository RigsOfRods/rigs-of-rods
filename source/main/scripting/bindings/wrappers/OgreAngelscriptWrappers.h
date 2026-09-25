/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

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

/// @file
/// Helper/wrapper functions shared by native (OgreAngelscript.cpp)
/// and generic (OgreAngelscriptGeneric.cpp) OGRE bindings.
/// Native bindings register these directly (asCALL_CDECL_OBJFIRST/OBJLAST),
/// generic bindings wrap them using WRAP_OBJ_FIRST()/WRAP_OBJ_LAST().

#pragma once

#include "Application.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"

#include "scriptarray/scriptarray.h"

#include <OgreRenderOperation.h>

#include <Overlay/OgreOverlaySystem.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayContainer.h>

namespace OgreAngelscriptWrappers {

using namespace Ogre;
using namespace AngelScript;
using namespace RoR;

/***VECTOR3***/
static void Vector3DefaultConstructor(Vector3* self)
{
    new(self) Vector3();
}

static void Vector3CopyConstructor(const Vector3& other, Vector3* self)
{
    new(self) Vector3(other);
}

static void Vector3InitConstructor(float x, float y, float z, Vector3* self)
{
    new(self) Vector3(x, y, z);
}

static void Vector3InitConstructorScaler(float s, Vector3* self)
{
    new(self) Vector3(s, s, s);
}

/***VECTOR2***/
static void Vector2DefaultConstructor(Vector2* self)
{
    new(self) Vector2();
}

static void Vector2CopyConstructor(const Vector2& other, Vector2* self)
{
    new(self) Vector2(other);
}

static void Vector2InitConstructor(float x, float y, Vector2* self)
{
    new(self) Vector2(x, y);
}

static void Vector2InitConstructorScaler(float s, Vector2* self)
{
    new(self) Vector2(s, s);
}


/***RADIAN***/
static void RadianDefaultConstructor(Radian* self)
{
    new(self) Radian();
}

static void RadianCopyConstructor(const Radian& other, Radian* self)
{
    new(self) Radian(other);
}

static void RadianInitConstructor(float r, Radian* self)
{
    new(self) Radian(r);
}

static int RadianCmp(const Radian& a, const Radian& b)
{
    if (a > b)
        return 1;
    else if (a < b)
        return -1;
    else
        return 0;
}

/***DEGREE***/
static void DegreeDefaultConstructor(Degree* self)
{
    new(self) Degree();
}

static void DegreeCopyConstructor(const Degree& other, Degree* self)
{
    new(self) Degree(other);
}

static void DegreeInitConstructor(float r, Degree* self)
{
    new(self) Degree(r);
}

static int DegreeCmp(const Degree& a, const Degree& b)
{
    if (a > b)
        return 1;
    else if (a < b)
        return -1;
    else
        return 0;
}

/***QUATERNION***/
static void QuaternionDefaultConstructor(Quaternion* self)
{
    new(self) Quaternion();
}

static void QuaternionCopyConstructor(const Quaternion& other, Quaternion* self)
{
    new(self) Quaternion(other.w, other.x, other.y, other.z);
}

static void QuaternionInitConstructor1(const Radian& rfAngle, const Vector3& rkAxis, Quaternion* self)
{
    new(self) Quaternion(rfAngle, rkAxis);
}

static void QuaternionInitConstructor2(float w, float x, float y, float z, Quaternion* self)
{
    new(self) Quaternion(w, x, y, z);
}

static void QuaternionInitConstructor3(const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis, Quaternion* self)
{
    new(self) Quaternion(xaxis, yaxis, zaxis);
}

static void QuaternionInitConstructorScaler(float s, Quaternion* self)
{
    new(self) Quaternion(s, s, s, s);
}

/***COLOURVALUE***/
static void ColourValueDefaultConstructor(ColourValue* self)
{
    new(self) ColourValue();
}

static void ColourValueInitConstructor(float r, float g, float b, float a, ColourValue* self)
{
    new(self) ColourValue(r,g,b,a);
}

static void ColourValueCopyConstructor(const ColourValue& other, ColourValue* self)
{
    new(self) ColourValue(other.r, other.g, other.b, other.a);
}

/***BOX***/
static void BoxDefaultConstructor(Box* self)
{
    new(self) Box();
}

static void BoxInitConstructor(asUINT l, asUINT t, asUINT r, asUINT b, Box* self)
{
    new(self) Box(l, t, r, b);
}

static void BoxCopyConstructor(const Box& other, Box* self)
{
    new(self) Box(other);
}

static void BoxDestructor(Box* self)
{
    (self)->~Box();
}

static void BoxAssignOperator(const Box& other, Box* self)
{
    (self)->operator=(other);
}

/***TEXTURE***/
static void TexturePtrDefaultConstructor(TexturePtr* self)
{
    new (self) TexturePtr();
}

static void TexturePtrCopyConstructor(const TexturePtr& other, TexturePtr* self)
{
    new (self) TexturePtr(other);
}

static void TexturePtrDestructor(TexturePtr* self)
{
    (self)->~TexturePtr();
}

static void TexturePtrAssignOperator(const TexturePtr& other, TexturePtr* self)
{
    (self)->operator=(other);
}

static bool TexturePtrIsNull(TexturePtr* self)
{
    return !(self)->operator bool();
}

// Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
static Ogre::String TexturePtrGetName(TexturePtr const& self)
{
    return self->getName();
}

static Ogre::uint32 TexturePtrGetWidth(TexturePtr const& self)
{
    return (asUINT)self->getWidth();
}

static Ogre::uint32 TexturePtrGetHeight(TexturePtr const& self)
{
    return (asUINT)self->getHeight();
}

static Ogre::uint32 TexturePtrGetNumMipmaps(TexturePtr const& self)
{
    return (asUINT)self->getNumMipmaps();
}

static HardwarePixelBufferSharedPtr TexturePtrGetBuffer(TexturePtr const& self, asUINT face, asUINT mipmap)
{
    return self->getBuffer(face, mipmap);
}

/***TEXTUREMANAGER***/
// Convenience wrapper to omit optional parameters
static TexturePtr TextureManagerLoad(TextureManager& mgr, std::string const& file, std::string const& rg)
{
    try { return mgr.load(file, rg); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::TextureManager::load()"); return Ogre::TexturePtr();}
}

/***HARDWAREPIXELBUFFER***/
// NOTE: The `*SharedPtr` is a deprecated alias of `*Ptr` in OGRE 14, but it's not yet present in the version we use.
static void HardwarePixelBufferPtrDefaultConstructor(HardwarePixelBufferSharedPtr* self)
{
    new (self) HardwarePixelBufferSharedPtr();
}

static void HardwarePixelBufferPtrCopyConstructor(const HardwarePixelBufferSharedPtr& other, HardwarePixelBufferSharedPtr* self)
{
    new (self) HardwarePixelBufferSharedPtr(other);
}

static void HardwarePixelBufferPtrDestructor(HardwarePixelBufferSharedPtr* self)
{
    (self)->~HardwarePixelBufferSharedPtr();
}

static void HardwarePixelBufferPtrAssignOperator(const HardwarePixelBufferSharedPtr& other, HardwarePixelBufferSharedPtr* self)
{
    (self)->operator=(other);
}

static PixelBox PIXELBOX_DUMMY = PixelBox(); // for returning as `const&`

static const PixelBox& HardwarePixelBufferPtrGetCurrentLock(HardwarePixelBufferSharedPtr const& self)
{
    try { return self->getCurrentLock(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::HardwarePixelBuffer::getCurrentLock()"); return PIXELBOX_DUMMY;}
}

static const PixelBox& HardwarePixelBufferPtrLock(HardwarePixelBufferSharedPtr const& self, const Box& lockbox, HardwareBuffer::LockOptions opt)
{
    try { return self->lock(lockbox, opt); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::HardwarePixelBuffer::lock()"); return PIXELBOX_DUMMY;}
}

static asUINT HardwarePixelBufferPtrGetWidth(HardwarePixelBufferSharedPtr const& self)
{
    try { return self->getWidth(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::HardwarePixelBuffer::getWidth()"); return 0;}
}

static asUINT HardwarePixelBufferPtrGetHeight(HardwarePixelBufferSharedPtr const& self)
{
    try { return self->getHeight(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::HardwarePixelBuffer::getHeight()"); return 0;}
}

static void HardwarePixelBufferPtrBlitFromMemory(HardwarePixelBufferSharedPtr const& self, const PixelBox& src, const Box& dstBox)
{
    try { self->blitFromMemory(src, dstBox); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::HardwarePixelBuffer::blitFromMemory()"); }
}

static void HardwarePixelBufferPtrBlitToMemory(HardwarePixelBufferSharedPtr const& self, const Box& srcBox, const PixelBox& dst)
{
    try { self->blitToMemory(srcBox, dst); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::HardwarePixelBuffer::blitToMemory()"); }
}

static void HardwarePixelBufferPtrUnlock(HardwarePixelBufferSharedPtr const& self)
{
    try { self->unlock(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::HardwarePixelBuffer::unlock()"); }
}

/***PIXELBOX***/
static void PixelBoxDefaultConstructor(PixelBox* self)
{
    new (self) PixelBox();
}

static void PixelBoxCopyConstructor(const PixelBox& other, PixelBox* self)
{
    new (self) PixelBox(other);
}

static void PixelBoxDestructor(PixelBox* self)
{
    (self)->~PixelBox();
}

static void PixelBoxAssignOperator(const PixelBox& other, PixelBox* self)
{
    (self)->operator=(other);
}

// Inherited methods must have wrappers - see AngelScript doc
static asUINT PixelBoxGetWidth(const Ogre::PixelBox& self)
{
    return self.getWidth();
}

static asUINT PixelBoxGetHeight(const Ogre::PixelBox& self)
{
    return self.getHeight();
}

static asUINT PixelBoxGetDepth(const Ogre::PixelBox& self)
{
    return self.getDepth();
}

/***IMAGE***/
static void ImageDefaultConstructor(Image* self)
{
    new (self) Image();
}

static void ImageCopyConstructor(const Image& other, Image* self)
{
    new (self) Image(other);
}

static void ImageDestructor(PixelBox* self)
{
    (self)->~PixelBox();
}

static void ImageAssignOperator(const PixelBox& other, PixelBox* self)
{
    (self)->operator=(other);
}

/***MESH***/
typedef CReadonlyScriptArrayView<Ogre::SubMesh*> SubMeshArray;

static SubMeshArray* MeshPtrGetSubmeshes(const MeshPtr& self)
{
    return new SubMeshArray(self->getSubMeshes());
}

static void MeshPtrDefaultConstructor(MeshPtr* self)
{
    new (self) MeshPtr();
}

static void MeshPtrCopyConstructor(const MeshPtr& other, MeshPtr* self)
{
    new (self) MeshPtr(other);
}

static void MeshPtrDestructor(MeshPtr* self)
{
    (self)->~MeshPtr();
}

static void MeshPtrAssignOperator(const MeshPtr& other, MeshPtr* self)
{
    (self)->operator=(other);
}

static bool MeshPtrIsNull(MeshPtr* self)
{
    return !(self)->operator bool();
}

// Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
static Ogre::String MeshPtrGetName(MeshPtr const& self)
{
    return self->getName();
}

static Ogre::SubMesh* MeshPtrCreateSubMesh(MeshPtr const& self, const Ogre::String& name)
{
    return self->createSubMesh(name);
}

static void MeshPtrDestroySubMesh(MeshPtr const& self, const Ogre::String& name)
{
    self->createSubMesh(name);
}

/***MESHMANAGER***/
static MeshPtr MeshManagerLoad(MeshManager& mgr, std::string const& file, std::string const& rg)
{
    try { return mgr.load(file, rg); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::MeshManager::load()"); return Ogre::MeshPtr();}
}

static void MeshManagerRemove(MeshManager& mgr, std::string const& file, std::string const& rg)
{
    try { mgr.remove(file, rg); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::MeshManager::remove()"); }
}

/***SUBMESH***/
static AngelScript::CScriptArray* SubMesh__getVertexPositions(SubMesh* self)
{
    VertexData* vertData = (self->useSharedVertices) ? self->parent->sharedVertexData : self->vertexData;
    if (!vertData)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getVertexPositions(): No vertex data found");
        return nullptr;
    }
    const Ogre::VertexElement* posElem = vertData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
    if (!posElem)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getVertexPositions(): No POSITION element found");
        return nullptr;
    }
    Ogre::HardwareVertexBufferSharedPtr vbuf = vertData->vertexBufferBinding->getBuffer(posElem->getSource());
    if (!vbuf)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getVertexPositions(): No vertex buffer found");
        return nullptr;
    }
    AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<vector3>");
    AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo, vertData->vertexCount);
    uint8_t* pStart = static_cast<uint8_t*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
    for (size_t i = 0; i < vertData->vertexCount; i++)
    {
        uint8_t* pVert = pStart + (i * vertData->vertexDeclaration->getVertexSize(posElem->getSource()));
        float* pPos = nullptr;
        posElem->baseVertexPointerToElement(pVert, &pPos);
        Vector3 pos(*pPos, *(pPos+1), *(pPos+2));
        arr->SetValue(i, &pos);
    }
    vbuf->unlock();
    return arr;
}

static AngelScript::CScriptArray* SubMesh__getVertexTexcoords(SubMesh* self, asUINT index)
{
    VertexData* vertData = (self->useSharedVertices) ? self->parent->sharedVertexData : self->vertexData;
    if (!vertData)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getVertexTexcoords(): No vertex data found");
        return nullptr;
    }
    const Ogre::VertexElement* texcoordElem = vertData->vertexDeclaration->findElementBySemantic(Ogre::VES_TEXTURE_COORDINATES, (unsigned short)index);
    if (!texcoordElem)
    {
        App::GetScriptEngine()->SLOG(fmt::format("SubMesh::__getVertexTexcoords(): TEXCOORD element with index {} not found", index));
        return nullptr;
    }
    Ogre::HardwareVertexBufferSharedPtr vbuf = vertData->vertexBufferBinding->getBuffer(texcoordElem->getSource());
    if (!vbuf)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getVertexTexcoords(): No vertex buffer found");
        return nullptr;
    }
    AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<vector2>");
    AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo, vertData->vertexCount);
    uint8_t* pStart = static_cast<uint8_t*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
    const size_t vertSize = vertData->vertexDeclaration->getVertexSize(texcoordElem->getSource());
    ROR_ASSERT(texcoordElem->getType() == Ogre::VET_FLOAT2);
    for (size_t i = 0; i < vertData->vertexCount; i++)
    {
        uint8_t* pVert = pStart + ((i + vertData->vertexStart) * vertSize);
        float* pTexcoord = nullptr;
        texcoordElem->baseVertexPointerToElement(pVert, &pTexcoord);
        Vector2 texcoord(*pTexcoord, *(pTexcoord+1));
        arr->SetValue(i, &texcoord);
    }
    vbuf->unlock();
    return arr;
}

static Ogre::HardwareIndexBuffer::IndexType SubMesh__getIndexType(SubMesh* self)
{
    if (!self->indexData)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getIndexType(): No index data found");
        return Ogre::HardwareIndexBuffer::IT_16BIT;
    }
    Ogre::HardwareIndexBufferSharedPtr ibuf = self->indexData->indexBuffer;
    if (!ibuf)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getIndexType(): No index buffer found");
        return Ogre::HardwareIndexBuffer::IT_16BIT;
    }
    return ibuf->getType();
}

static AngelScript::CScriptArray* SubMesh__getIndexBufferHelper(Ogre::SubMesh* self, Ogre::HardwareIndexBuffer::IndexType desiredType)
{
    if (!self->indexData)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getIndexBufferHelper(): No index data found");
        return nullptr;
    }
    Ogre::HardwareIndexBufferSharedPtr ibuf = self->indexData->indexBuffer;
    if (!ibuf)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getIndexBufferHelper(): No index buffer found");
        return nullptr;
    }
    if (ibuf->getType() != desiredType)
    {
        App::GetScriptEngine()->SLOG("SubMesh::__getIndexBufferHelper(): Index buffer type mismatch");
        return nullptr;
    }
    AngelScript::asITypeInfo* typeinfo = App::GetScriptEngine()->getEngine()->GetTypeInfoByDecl("array<uint16>");
    AngelScript::CScriptArray* arr = AngelScript::CScriptArray::Create(typeinfo, self->indexData->indexCount);
    uint8_t* pStart = static_cast<uint8_t*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
    for (size_t i = self->indexData->indexStart; i < self->indexData->indexCount; i++)
    {
        uint8_t* pIndex = pStart + (i * ibuf->getIndexSize());
        if (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_16BIT)
        {
            uint16_t index = *reinterpret_cast<uint16_t*>(pIndex);
            arr->SetValue(i, &index);
        }
        else if (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT)
        {
            uint32_t index = *reinterpret_cast<uint32_t*>(pIndex);
            arr->SetValue(i, &index);
        }
        else
        {
            App::GetScriptEngine()->SLOG("SubMesh::__getIndexBufferHelper(): Unknown index buffer type");
            return nullptr;
        }
    }
    ibuf->unlock();
    return arr;
}

static CScriptArray* SubMesh__getIndexBuffer16bit(Ogre::SubMesh* self)
{
    const Ogre::HardwareIndexBuffer::IndexType desiredType = Ogre::HardwareIndexBuffer::IndexType::IT_16BIT;
    if (SubMesh__getIndexType(self) == desiredType) { return SubMesh__getIndexBufferHelper(self, desiredType); }
    else { App::GetScriptEngine()->SLOG("SubMesh::__getIndexBuffer16bit(): The buffer format isn't 16bit."); return (CScriptArray*)nullptr; }
}

static CScriptArray* SubMesh__getIndexBuffer32bit(Ogre::SubMesh* self)
{
    const Ogre::HardwareIndexBuffer::IndexType desiredType = Ogre::HardwareIndexBuffer::IndexType::IT_32BIT;
    if (SubMesh__getIndexType(self) == desiredType) { return SubMesh__getIndexBufferHelper(self, desiredType); }
    else { App::GetScriptEngine()->SLOG("SubMesh::__getIndexBuffer32bit(): The buffer format isn't 32bit."); return (CScriptArray*)nullptr; }
}

/***MOVABLEOBJECT***/
static std::string MovableObjectGetUniqueNameMixin(Ogre::MovableObject* self)
{
    // names are optional and largely unused by RoR, so always append the type and memory address (libfmt adds the '0x' prefix)
    return fmt::format("\"{}\" ({} {})", self->getName(), self->getMovableType(), static_cast<void*>(self));
}

/***ANIMATIONSTATESET***/
typedef CReadonlyScriptDictView<AnimationState*> AnimationStateDict;

static AnimationStateDict* AnimationStateSetGetAnimationStates(AnimationStateSet* self)
{
    return new AnimationStateDict(self->getAnimationStates());
}

/***MATERIAL***/
typedef CReadonlyScriptArrayView<Ogre::Technique*> TechniqueArray;

static TechniqueArray* MaterialPtrGetTechniques(const MaterialPtr& self)
{
    return new TechniqueArray(self->getTechniques());
}

static void MaterialPtrDefaultConstructor(MaterialPtr* self)
{
    new (self) MaterialPtr();
}

static void MaterialPtrCopyConstructor(const MaterialPtr& other, MaterialPtr* self)
{
    new (self) MaterialPtr(other);
}

static void MaterialPtrDestructor(MaterialPtr* self)
{
    (self)->~MaterialPtr();
}

static void MaterialPtrAssignOperator(const MaterialPtr& other, MaterialPtr* self)
{
    (self)->operator=(other);
}

static bool MaterialPtrIsNull(MaterialPtr* self)
{
    return !(self)->operator bool();
}

// Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/
static Ogre::String MaterialPtrGetName(MaterialPtr const& self)
{
    return self->getName();
}

static Ogre::Technique* MaterialPtrCreateTechnique(MaterialPtr const& self)
{
    try { return self->createTechnique(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Material::createTechnique()"); return (Ogre::Technique*)nullptr;}
}

static void MaterialPtrRemoveTechnique(MaterialPtr const& self, uint16_t index)
{
    try { self->removeTechnique(index); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Material::removeTechnique()"); }
}

/***MATERIALMANAGER***/
static MaterialPtr MaterialManagerGetByName(MaterialManager& mgr, std::string const& file, std::string const& rg)
{
    try { return mgr.getByName(file, rg); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::MaterialManager::getByName()"); return Ogre::MaterialPtr();}
}

static MaterialPtr MaterialManagerCreate(MaterialManager& mgr, std::string const& file, std::string const& rg)
{
    try { return mgr.create(file, rg); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::MaterialManager::create()"); return Ogre::MaterialPtr(); }
}

/***TECHNIQUE***/
typedef CReadonlyScriptArrayView<Ogre::Pass*> PassArray;

static PassArray* TechniqueGetPasses(Technique* self)
{
    return new PassArray(self->getPasses());
}

/***PASS***/
typedef CReadonlyScriptArrayView<Ogre::TextureUnitState*> TextureUnitStateArray;

static TextureUnitStateArray* PassGetTextureUnitStates(Pass* self)
{
    return new TextureUnitStateArray(self->getTextureUnitStates());
}

static Ogre::GpuProgramParametersPtr PassGetVertexProgramParameters(Ogre::Pass* self)
{
    try { return self->getVertexProgramParameters(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Pass::getVertexProgramParameters()"); return Ogre::GpuProgramParametersPtr(); }
}

static Ogre::GpuProgramParametersPtr PassGetFragmentProgramParameters(Ogre::Pass* self)
{
    try { return self->getFragmentProgramParameters(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Pass::getFragmentProgramParameters()"); return Ogre::GpuProgramParametersPtr(); }
}

static Ogre::GpuProgramParametersPtr PassGetGeometryProgramParameters(Ogre::Pass* self)
{
    try { return self->getGeometryProgramParameters(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Pass::getGeometryProgramParameters()"); return Ogre::GpuProgramParametersPtr(); }
}

static Ogre::GpuProgramParametersPtr PassGetTessellationHullProgramParameters(Ogre::Pass* self)
{
    try { return self->getTessellationHullProgramParameters(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Pass::getTessellationHullProgramParameters()"); return Ogre::GpuProgramParametersPtr(); }
}

static Ogre::GpuProgramParametersPtr PassGetTessellationDomainProgramParameters(Ogre::Pass* self)
{
    try { return self->getTessellationDomainProgramParameters(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Pass::getTessellationDomainProgramParameters()"); return Ogre::GpuProgramParametersPtr(); }
}

static Ogre::GpuProgramParametersPtr PassGetComputeProgramParameters(Ogre::Pass* self)
{
    try { return self->getComputeProgramParameters(); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::Pass::getComputeProgramParameters()"); return Ogre::GpuProgramParametersPtr(); }
}

/***SUBENTITY***/
typedef CReadonlyScriptArrayView<Ogre::SubEntity*> SubEntityArray;

static SubEntityArray* EntityGetSubEntities(Entity* self)
{
    return new SubEntityArray(self->getSubEntities());
}

/***TIMER***/
static void TimerDefaultConstructor(Timer* self)
{
    new(self) Timer();
}

static void TimerDefaultDestructor(Timer* self)
{
    self->~Timer();
}

static void TimerCopyConstructor(const Timer& other, Timer* self)
{
    new(self) Timer(other);
}

static void TimerAssignOperator(const Timer& other, Timer* self)
{
    (self)->operator=(other);
}

/***GPUPROGRAMPARAMETERSPTR***/
static void GpuProgramParametersPtrDefaultConstructor(GpuProgramParametersPtr* self)
{
    new (self) GpuProgramParametersPtr();
}

static void GpuProgramParametersPtrCopyConstructor(const GpuProgramParametersPtr& other, GpuProgramParametersPtr* self)
{
    new (self) GpuProgramParametersPtr(other);
}

static void GpuProgramParametersPtrDestructor(GpuProgramParametersPtr* self)
{
    (self)->~GpuProgramParametersPtr();
}

static void GpuProgramParametersPtrAssignOperator(const GpuProgramParametersPtr& other, GpuProgramParametersPtr* self)
{
    (self)->operator=(other);
}

static bool GpuProgramParametersPtrIsNull(GpuProgramParametersPtr* self)
{
    return !(self)->operator bool();
}

// Wrappers are inevitable, see https://www.gamedev.net/forums/topic/540419-custom-smartpointers-and-angelscript-/

// > setConstant (scalar)
static void GpuProgramParametersPtrSetConstantFloat(GpuProgramParametersPtr const& self, asUINT index, float val)
{
    try { self->setConstant(index, val); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setConstant(uint, float)"); }
}

static void GpuProgramParametersPtrSetConstantVector3(GpuProgramParametersPtr const& self, asUINT index, const Ogre::Vector3& val)
{
    try { self->setConstant(index, val); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setConstant(uint, vector3)"); }
}

static void GpuProgramParametersPtrSetConstantVector2(GpuProgramParametersPtr const& self, asUINT index, const Ogre::Vector2& val)
{
    try { self->setConstant(index, val); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setConstant(uint, vector2)"); }
}

static void GpuProgramParametersPtrSetConstantColourValue(GpuProgramParametersPtr const& self, asUINT index, const Ogre::ColourValue& val)
{
    try { self->setConstant(index, val); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setConstant(uint, color)"); }
}

// > setConstant (vector)
static void GpuProgramParametersPtrSetConstantFloatArray(GpuProgramParametersPtr const& self, asUINT index, CScriptArray* vals)
{
    ROR_ASSERT(vals->GetElementTypeId() == App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("float"));
    try { self->setConstant(index, (float*)vals->GetBuffer(), vals->GetSize()); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setConstant(uint, array<float>)"); }
}

static void GpuProgramParametersPtrSetConstantVector3Array(GpuProgramParametersPtr const& self, asUINT index, CScriptArray* vals)
{
    ROR_ASSERT(vals->GetElementTypeId() == App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("vector3"));
    try { self->setConstant(index, (float*)vals->GetBuffer(), vals->GetSize()*3); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setConstant(uint, array<vector3>)"); }
}

static void GpuProgramParametersPtrSetConstantVector2Array(GpuProgramParametersPtr const& self, asUINT index, CScriptArray* vals)
{
    ROR_ASSERT(vals->GetElementTypeId() == App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("vector2"));
    try { self->setConstant(index, (float*)vals->GetBuffer(), vals->GetSize()*2); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setConstant(uint, array<vector2>)"); }
}

static void GpuProgramParametersPtrSetConstantColourValueArray(GpuProgramParametersPtr const& self, asUINT index, CScriptArray* vals)
{
    ROR_ASSERT(vals->GetElementTypeId() == App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("color"));
    try { self->setConstant(index, (float*)vals->GetBuffer(), vals->GetSize()*4); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setConstant(uint, array<color>)"); }
}

// > setNamedConstant (scalar)
static void GpuProgramParametersPtrSetNamedConstantFloat(GpuProgramParametersPtr const& self, const std::string& name, float val)
{
    try { self->setNamedConstant(name, val); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setNamedConstant(string, float)"); }
}

static void GpuProgramParametersPtrSetNamedConstantVector3(GpuProgramParametersPtr const& self, const std::string& name, const Ogre::Vector3& val)
{
    try { self->setNamedConstant(name, val); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setNamedConstant(string, vector3)"); }
}

static void GpuProgramParametersPtrSetNamedConstantVector2(GpuProgramParametersPtr const& self, const std::string& name, const Ogre::Vector2& val)
{
    try { self->setNamedConstant(name, val); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setNamedConstant(string, vector2)"); }
}

static void GpuProgramParametersPtrSetNamedConstantColourValue(GpuProgramParametersPtr const& self, const std::string& name, const Ogre::ColourValue& val)
{
    try { self->setNamedConstant(name, val); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setNamedConstant(string, color)"); }
}

// > setNamedConstant (vector)
static void GpuProgramParametersPtrSetNamedConstantFloatArray(GpuProgramParametersPtr const& self, const std::string& name, CScriptArray* vals)
{
    ROR_ASSERT(vals->GetElementTypeId() == App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("float"));
    try { self->setNamedConstant(name, (float*)vals->GetBuffer(), vals->GetSize()); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setNamedConstant(string, array<float>)"); }
}

static void GpuProgramParametersPtrSetNamedConstantVector3Array(GpuProgramParametersPtr const& self, const std::string& name, CScriptArray* vals)
{
    ROR_ASSERT(vals->GetElementTypeId() == App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("vector3"));
    try { self->setNamedConstant(name, (float*)vals->GetBuffer(), vals->GetSize()*3); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setNamedConstant(string, array<vector3>)"); }
}

static void GpuProgramParametersPtrSetNamedConstantVector2Array(GpuProgramParametersPtr const& self, const std::string& name, CScriptArray* vals)
{
    ROR_ASSERT(vals->GetElementTypeId() == App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("vector2"));
    try { self->setNamedConstant(name, (float*)vals->GetBuffer(), vals->GetSize()*2); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setNamedConstant(string, array<vector2>)"); }
}

static void GpuProgramParametersPtrSetNamedConstantColourValueArray(GpuProgramParametersPtr const& self, const std::string& name, CScriptArray* vals)
{
    ROR_ASSERT(vals->GetElementTypeId() == App::GetScriptEngine()->getEngine()->GetTypeIdByDecl("color"));
    try { self->setNamedConstant(name, (float*)vals->GetBuffer(), vals->GetSize()*4); }
    catch (...) { App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::setNamedConstant(string, array<color>)"); }
}

static CScriptArray* GpuProgramParametersPtr__getNamedConstants(GpuProgramParametersPtr const& self)
{
    try {
        std::vector<std::string> keys;
        const Ogre::GpuNamedConstants& namedConstants = self->getConstantDefinitions();
        for (auto& pair : namedConstants.map)
            keys.push_back(pair.first);
        return RoR::VectorToScriptArray(keys, "string");
    }
    catch (...) {
        App::GetScriptEngine()->forwardExceptionAsScriptEvent("Ogre::GpuProgramParameters::__getNamedConstants()");
        return nullptr; }
}

} // namespace OgreAngelscriptWrappers
