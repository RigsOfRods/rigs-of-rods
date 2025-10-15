/*
    This source file is part of Rigs of Rods
    Copyright 2025 Petr Ohlidal

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

#pragma once

#include <math.h> // sqrtf()
#include <Ogre.h> // Ogre::Vector3

namespace RoR {

    /// Designed to work smoothly with optimizations disabled
    struct Vec3
    {
        float x, y, z;

        Vec3() : x(0), y(0), z(0) {}
        Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
        Vec3(const Ogre::Vector3& v) { x = v.x; y = v.y; z = v.z; }

        Vec3 operator+(const Vec3& b) const { return Vec3(x + b.x, y + b.y, z + b.z); }
        Vec3 operator-(const Vec3& b) const { return Vec3(x - b.x, y - b.y, z - b.z); }
        Vec3 operator*(float f) const { return Vec3(x * f, y * f, z * f); }
        Vec3 operator/(float f) const { return Vec3(x / f, y / f, z / f); }

        Vec3& operator+=(const Vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
        Vec3& operator-=(const Vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
        Vec3& operator*=(float f) { x *= f; y *= f; z *= f; return *this; }
        Vec3& operator/=(float f) { x /= f; y /= f; z /= f; return *this; }

        operator Ogre::Vector3() const { return Ogre::Vector3(x, y, z); }

        Vec3 operator-() const { return Vec3(-x, -y, -z); }

        float dotProduct(const Vec3& b) const { return x * b.x + y * b.y + z * b.z; }
        Vec3 crossProduct(const Vec3& b) const { return Vec3(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }
        float length() const { return sqrtf(x * x + y * y + z * z); }
        float squaredLength() const { return x * x + y * y + z * z; }
    };

    inline Vec3 operator*(float f, const Vec3& v) { return Vec3(v.x * f, v.y * f, v.z * f); }
    
} // namespace RoR