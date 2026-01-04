/*
--------------------------------------------------------------------------------
This source file is part of SkyX.
Visit http://www.paradise-studios.net/products/skyx/

Copyright (C) 2009-2012 Xavier Verguín González <xavyiy@gmail.com>

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

#include "BasicController.h"
#include <Ogre.h>

namespace SkyX
{
	BasicController::BasicController(const bool& deleteBySkyX)
		: Controller(deleteBySkyX)
		, mTime(Ogre::Vector3(14.0f, 7.50f, 20.50f))
		, mSunDirection(Ogre::Vector3(0,1,0))
		, mMoonDirection(Ogre::Vector3(0,-1,0))
		, mMoonPhase(0)
		, mLatitudeDeg(45.f)
		, mDayOfYear(172.f)          // ~June solstice
	{
	}

	void BasicController::update(const Ogre::Real& simDeltaTime)
	{
		// Advance time of day (x component)
		mTime.x += simDeltaTime;
		if (mTime.x > 24.f)      mTime.x -= 24.f;
		else if (mTime.x < 0.f)  mTime.x += 24.f;

		// Realistic-ish solar position (simplified)
		// Variables:
		// t = local solar time hours [0,24)
		// latitude φ (deg), declination δ (deg), hour angle H (deg)
		// δ approximation: -23.44° * cos( 360°/365 * (N + 10) )  (N = day-of-year)
		const Ogre::Real t = mTime.x;
		const Ogre::Real latitudeDeg = Ogre::Math::Clamp(mLatitudeDeg, -90.f, 90.f);
		const Ogre::Real N = Ogre::Math::Clamp(mDayOfYear, 0.f, 365.f);

		// Solar declination (radians)
		const Ogre::Real declDeg = -23.44f * Ogre::Math::Cos(Ogre::Degree((360.f/365.f) * (N + 10.f)).valueRadians());
		const Ogre::Radian decl = Ogre::Degree(declDeg);
		const Ogre::Radian lat  = Ogre::Degree(latitudeDeg);

        this->recalculateSunriseSunsetTime(decl, lat);

		// Hour angle: 0 at solar noon, negative morning, positive afternoon
		const Ogre::Radian H = Ogre::Degree(15.f * (t - 12.f));

		// Solar elevation
		const Ogre::Real sinElev =
			Ogre::Math::Sin(lat) * Ogre::Math::Sin(decl) +
			Ogre::Math::Cos(lat) * Ogre::Math::Cos(decl) * Ogre::Math::Cos(H);
		Ogre::Radian elev = Ogre::Math::ASin(Ogre::Math::Clamp(sinElev, -1.f, 1.f));

		// Solar azimuth (mathematically from south; we adapt to world axes)
		// Using standard formula with atan2
		const Ogre::Real sinAz = -Ogre::Math::Sin(H);
		const Ogre::Real cosAz = (Ogre::Math::Sin(decl) - Ogre::Math::Sin(elev)*Ogre::Math::Sin(lat)) /
			(Ogre::Math::Cos(elev) * Ogre::Math::Cos(lat));
		Ogre::Radian az = Ogre::Math::ATan2(sinAz, cosAz); // Range [-pi,pi]

		// Build horizontal basis:
		// East given, North derived (X east, Z south => north = -south = -Z)
		Ogre::Vector2 East = mEastDirection.normalisedCopy(); // (1,0) in current map
		Ogre::Vector2 North(-East.y, East.x); // rotate East left (CW) to get North (since Z positive is south)

		// We want azimuth 0 at south, +90 at west, -90 at east (common solar convention).
		// Convert to our basis: south = -North
		Ogre::Vector2 South = -North;

		// Horizontal projection direction
		Ogre::Real cosAzAdj = Ogre::Math::Cos(az);
		Ogre::Real sinAzAdj = Ogre::Math::Sin(az);
		Ogre::Vector2 horiz2D = South * cosAzAdj + East * (-sinAzAdj); // signs adjusted so sunrise at East

		// Final 3D direction
		Ogre::Real cosElev = Ogre::Math::Cos(elev);
		mSunDirection = Ogre::Vector3(horiz2D.x * cosElev,
			Ogre::Math::Sin(elev),
			horiz2D.y * cosElev);

		// If sun below horizon (elev < 0), keep direction; shaders may decide lighting strength.
		mMoonDirection = -mSunDirection;
	}

void BasicController::recalculateSunriseSunsetTime(Ogre::Radian decl, Ogre::Radian lat)
{
	const Ogre::Real cosH0 = -Ogre::Math::Tan(lat) * Ogre::Math::Tan(decl);
		
	if (cosH0 >= -1.f && cosH0 <= 1.f)
	{
		// Normal day with sunrise and sunset
		const Ogre::Radian H0 = Ogre::Math::ACos(Ogre::Math::Clamp(cosH0, -1.f, 1.f));
		const Ogre::Real H0_hours = Ogre::Degree(H0).valueDegrees() / 15.f;
			
		mTime.y = 12.f - H0_hours;  // Sunrise time (hours)
		mTime.z = 12.f + H0_hours;  // Sunset time (hours)
	}
	else if (cosH0 < -1.f)
	{
		// Polar day (sun never sets)
		mTime.y = 0.f;
		mTime.z = 24.f;
	}
	else // cosH0 > 1.f
	{
		// Polar night (sun never rises)
		mTime.y = 12.f;
		mTime.z = 12.f;
	}
}

} // namespace SkyX
