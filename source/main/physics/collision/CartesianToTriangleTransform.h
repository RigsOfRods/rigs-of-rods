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

#include "Triangle.h"

#include <OgreMatrix3.h>
#include <Ogre.h>

/// @addtogroup Physics
/// @{

/// @addtogroup Collisions
/// @{

/// Defines a linear transformation from cartesian coordinates to local (barycentric) coordinates of a specified triangle.
/**
 * The barycentric coordinate system of the triangle is defined in terms of its three vertices
 * \f$(\mathbf{a}, \mathbf{b}, \mathbf{c})\f$.
 * Since an arbitrary three-dimensional point \f$\mathbf{p}\f$ is not guaranteed to lie within the plane defined
 * by this triangle, the barycentric coordinates \f$(\alpha, \beta, \gamma)\f$ are instead determined for
 * the point \f$\mathbf{p}'\f$ which denotes the projection of \f$\mathbf{p}\f$ onto this plane.
 * In addition, the shortest (signed) distance \f$d\f$ between the point \f$\mathbf{p}\f$ and the triangle
 * plane is returned by this transformation.
 *
 * The transformation is evaluated lazily, i.e. creating new instances of CartesianToTriangleTransform is 
 * relatively cheap. Actual calculations are deferred until the transformation is applied to a point for
 * the first time.
 */
class CartesianToTriangleTransform
{
public:
    /// Return type of CartesianToTriangleTransform transformation.
    /**
     * Describes the position of a three-dimensional point relative to a triangle.
     * The position on the triangle plane is defined by the barycentric coordinates #alpha, #beta and #gamma.
     * The perpendicular offset of the point from this plane is given by the value of #distance.
     */
    struct TriangleCoord {
        const struct {
            Ogre::Real alpha,  //!< Barycentric coordinate
                       beta,   //!< Barycentric coordinate
                       gamma;  //!< Barycentric coordinate
        } barycentric;
        const Ogre::Real distance;  //!< Shortest (signed) distance to triangle plane
    };

    /// Construct transformation for specified triangle.
    explicit CartesianToTriangleTransform(const Triangle &triangle) : m_triangle{triangle}, m_initialized{false} {}

    /// Transform point into local triangle coordinates.
    /**
     * The calculation of the barycentric coordinates and the perpendicular distance to the triangle plane
     * for a given point p is explained in the following.
     *
     * Let \f$\mathbf{p}'\f$ denote the projection of point \f$\mathbf{p}\f$ onto the triangle plane.
     * I.e. \f$\mathbf{p} = \mathbf{p}' + d \cdot \mathbf{n}\f$, where \f$\mathbf{n}\f$ is the normal vector
     * of the plane and \f$d\f$ is the shortest (signed) distance between point \f$\mathbf{p}\f$ and the plane.
     *
     * The point \f$\mathbf{p}'\f$ can be expressed in barycentric coordinates \f$(\alpha, \beta, \gamma)\f$ as
     * follows:
     * \f[
     *  \mathbf{p}' = \alpha \cdot \mathbf{a} + \beta \cdot \mathbf{b} + \gamma \cdot \mathbf{c},
     *  \quad \text{with } \alpha + \beta + \gamma = 1
     * \f]
     * The variable \f$\gamma\f$ is redundant and can be substituted by \f$1-\alpha-\beta\f$:
     * \f{eqnarray*}
     *  \mathbf{p}' &=& \alpha \cdot \mathbf{a} + \beta \cdot \mathbf{b} + (1-\alpha -\beta) \cdot \mathbf{c} \\
     *     &=& \alpha \cdot (\mathbf{a} - \mathbf{c}) + \beta \cdot (\mathbf{b} - \mathbf{c}) + \mathbf{c}
     * \f}
     *
     * The distance \f$d\f$ between point \f$\mathbf{p}\f$ and the plane is calculated by projecting the vector
     * \f$(\mathbf{p} - \mathbf{c})\f$ onto the normal vector \f$\mathbf{n}\f$:
     * \f[
     *  d = \mathbf{n}^T \cdot (\mathbf{p}-\mathbf{c}), \quad \text{with } ||n|| = 1
     * \f]
     *
     * Combining this with the expression of point \f$\mathbf{p}'\f$ in barycentric coordinates allows to
     * formulate the problem in matrix form:
     * \f[
     *  \mathbf{M} = \begin{bmatrix} \mathbf{u} &  \mathbf{v} & \mathbf{n} \end{bmatrix}, \quad \text{with }
     *  \mathbf{u} = (\mathbf{a} - \mathbf{c}) \text{ and } \mathbf{v} = (\mathbf{b} - \mathbf{c})
     * \f]
     * \f[
     *  \mathbf{M} \cdot \begin{bmatrix} \alpha \\ \beta \\ d \end{bmatrix} =
     *  \mathbf{p}' + \mathbf{n} \cdot d  - \mathbf{c} = \mathbf{p} - \mathbf{c}
     * \f]
     *
     * The solution is obtained by inverting matrix \f$\mathbf{M}\f$:
     * \f[
     * \begin{bmatrix} \alpha \\ \beta \\ d \end{bmatrix} = \mathbf{M}^{-1} \cdot (\mathbf{p} - \mathbf{c})
     * \f]
     * \f$\gamma\f$ can be immediately calculated from known values \f$\alpha\f$ and \f$\beta\f$ because
     * \f$\alpha + \beta + \gamma = 1\f$ always holds.
     */
    TriangleCoord operator() (const Ogre::Vector3 &p) const
    {
        // lazy initialization of transformation matrix
        if (!m_initialized) {
            InitMatrix();
            m_initialized = true;
        }

        // apply transformation matrix and extract alpha, beta, gamma and perpendicular offset
        const Ogre::Vector3 result = m_matrix * (p - m_triangle.c);
        return {result[0], result[1], (1.f - result[0] - result[1]), result[2]};
    }

private:
    /// Initialize the transformation matrix
    void InitMatrix() const {
        // determine span and normal vectors
        const Ogre::Vector3 u = m_triangle.u;
        const Ogre::Vector3 v = m_triangle.v;
        const Ogre::Vector3 n = m_triangle.normal();
        
        // construct and invert matrix
        m_matrix = Ogre::Matrix3{ u[0], v[0], n[0],
                                  u[1], v[1], n[1],
                                  u[2], v[2], n[2] }.Inverse();
    }

    const Triangle m_triangle;       //!< The triangle on which the transformation is based.
    mutable bool m_initialized;
    mutable Ogre::Matrix3 m_matrix;  //!< Cached transformation matrix.
};

/// @} // addtogroup Collisions
/// @} // addtogroup Physics
