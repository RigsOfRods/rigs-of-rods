/*
 * Vector3.h
 *
 *  Created on: Dec 30, 2012
 *      Author: chris
 */

#ifndef VECTOR3_H_
#define VECTOR3_H_

#include <OgreVector3.h>
typedef Ogre::Real Real;

/**
 * extend the Ogre::Vector3 class so we can replace parts of it with faster routines
 */
class Vector3: public Ogre::Vector3
{
public:
    inline Vector3()
    {
    }

    inline Vector3( const Real fX, const Real fY, const Real fZ )
        : x( fX ), y( fY ), z( fZ )
    {
    }

    inline explicit Vector3( const Real afCoordinate[3] )
        : x( afCoordinate[0] ),
          y( afCoordinate[1] ),
          z( afCoordinate[2] )
    {
    }

    inline explicit Vector3( const int afCoordinate[3] )
    {
        x = (Real)afCoordinate[0];
        y = (Real)afCoordinate[1];
        z = (Real)afCoordinate[2];
    }

    inline explicit Vector3( Real* const r )
        : x( r[0] ), y( r[1] ), z( r[2] )
    {
    }

    inline explicit Vector3( const Real scaler )
        : x( scaler )
        , y( scaler )
        , z( scaler )
    {
    }
    
	virtual ~Vector3();
};

#endif /* VECTOR3_H_ */
