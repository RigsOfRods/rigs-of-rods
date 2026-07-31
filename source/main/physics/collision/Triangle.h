/*
    This source file is part of Rigs of Rods
    Copyright 2016 Fabian Killus

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

#include <OgreVector3.h>

/// @addtogroup Physics
/// @{

/// @addtogroup Collisions
/// @{

/// Represents a triangle in three-dimensional space.
///
/// Stores the three vertices #a, #b ,#c of the triangle and two
/// (non-unit-length) vectors #u and #v which span the plane within which the
/// triangle lies.
class Triangle
{
public:
    /// Construct triangle from three given vertices.
    explicit Triangle(const Ogre::Vector3& a, const Ogre::Vector3& b, const Ogre::Vector3& c)
        : a{a}, b{b}, c{c}, u{a - c}, v{b - c}
    {
    }

    Triangle()
        :a(Ogre::Vector3::ZERO)
        ,b(Ogre::Vector3::ZERO)
        ,c(Ogre::Vector3::ZERO)
        ,u(Ogre::Vector3::ZERO)
        ,v(Ogre::Vector3::ZERO)
    {
    }

    Ogre::Vector3 normal() const
    {
        return u.crossProduct(v).normalisedCopy();
    }

    Ogre::Vector3 a, //!< Vertex a
                  b, //!< Vertex b
                  c; //!< Vertex c

    Ogre::Vector3 u, //!< Span vector u
                  v; //!< Span vector v
};

/// @} // addtogroup Collisions
/// @} // addtogroup Physics
