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

} // namespace OgreAngelscriptWrappers
