/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.org/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
/*!
	@file
	@author		Albert Semenov
	@date		10/2009
	@module
*/

#pragma once
#ifndef __WRAPS_ATTRIBUTE_H__
#define __WRAPS_ATTRIBUTE_H__

#include <MyGUI.h>
#include "Attribute.h"

namespace attribute
{

	struct FieldSetterWidget
	{
		typedef MyGUI::Widget BaseValueType;

		template <typename Type>
		static Type* convert(BaseValueType* _value)
		{
			return _value == 0 ? 0 : _value->castType<Type>(false);
		}
	};

	DECLARE_ATTRIBUTE_FIELD(AttributeFieldWidgetName, std::string, FieldSetterWidget);

#define ATTRIBUTE_FIELD_WIDGET_NAME(_class, _field, _value) \
	ATTRIBUTE_FIELD(AttributeFieldWidgetName, _class, _field, _value)



	DECLARE_ATTRIBUTE_CLASS(AttributeSize, MyGUI::IntSize);

#define ATTRIBUTE_CLASS_SIZE(_class, _value) \
	ATTRIBUTE_CLASS(AttributeSize, _class, _value)


	DECLARE_ATTRIBUTE_CLASS(AttributeLayout, std::string);

#define ATTRIBUTE_CLASS_LAYOUT(_class, _value) \
	ATTRIBUTE_CLASS(AttributeLayout, _class, _value)

}

#endif // __WRAPS_ATTRIBUTE_H__
