/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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

/** 
	@file   RigEditor_Types.h
	@date   09/2014
	@author Petr Ohlidal
*/

#pragma once

namespace RoR
{

namespace RigEditor
{

template <typename numeric_T> 
struct Rect4t
{
	Rect4t()
		:	x1(0)
		,	y1(0)
		,	x2(0)
		,	y2(0)
	{}

	Rect4t(numeric_T _x1, numeric_T _y1, numeric_T _x2, numeric_T _y2)
		:	x1(_x1)
		,	y1(_y1)
		,	x2(_x2)
		,	y2(_y2)
	{}

	numeric_T x1;
	numeric_T y1;
	numeric_T x2;
	numeric_T y2;
};

typedef Rect4t<int> Rect4int;

typedef Rect4t<float> Rect4float;

template <typename numeric_T>
struct Vector2t
{

	Vector2t()
		:	x(0)
		,	y(0)
	{}

	Vector2t(numeric_T _x, numeric_T _y)
		:	x(_x)
		,	y(_y)
	{}

	Vector2t operator-(Vector2t const & rhs) const
	{
		return Vector2t(x-rhs.x, y-rhs.y);
	}

	numeric_T x;
	numeric_T y;
};

typedef Vector2t<int> Vector2int;

} // namespace RigEditor

} // namespace RoR
