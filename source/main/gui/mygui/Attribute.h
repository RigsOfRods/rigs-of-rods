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
#ifndef __ATTRIBUTE_H__
#define __ATTRIBUTE_H__


namespace attribute
{

	// класс обертка для удаления данных из статического вектора
	template <typename Type>
	struct DataHolder
	{
		~DataHolder()
		{
			for (typename Type::iterator item=data.begin(); item!=data.end(); ++item)
				delete (*item).first;
		}

		Type data;
	};

	// интерфейс для обертки поля
	template <typename OwnerType, typename SetterType>
	struct Field
	{
		virtual void set(OwnerType* _target, typename SetterType::BaseValueType* _value) = 0;
	};

	// шаблон для обертки поля
	template <typename OwnerType, typename FieldType, typename SetterType>
	struct FieldHolder : public Field<OwnerType, SetterType>
	{
		FieldHolder(FieldType* OwnerType::* offset) : m_offset(offset) {  }
		FieldType* OwnerType::* const m_offset;

		virtual void set(OwnerType* _target, typename SetterType::BaseValueType* _value)
		{
                  _target->*m_offset = SetterType::template convert<FieldType>(_value);
                  //                  _target->*m_offset = (_value == 0 ? 0 : _value->castType<int>::template(false));//SetterType::convert<FieldType>(_value);
		}
	};

	// шаблон для атрибута поля
	template <typename OwnerType, typename ValueType, typename SetterType>
	struct AttributeField
	{
		typedef std::pair<Field<OwnerType, SetterType>*, ValueType> BindPair;
		typedef std::vector<BindPair> VectorBindPair;

		template <typename FieldType>
		AttributeField(FieldType* OwnerType::* _offset, const ValueType& _value)
		{
			getData().push_back(BindPair(new FieldHolder<OwnerType, FieldType, SetterType>(_offset), _value));
		}
		static VectorBindPair& getData()
		{
			static DataHolder<VectorBindPair> data;
			return data.data;
		}
	};

	// макрос для инстансирования атрибута поля
#define DECLARE_ATTRIBUTE_FIELD(_name, _type, _setter) \
	template <typename OwnerType, typename ValueType = _type, typename SetterType = _setter> \
	struct _name : public attribute::AttributeField<OwnerType, ValueType, SetterType> \
	{ \
		template <typename FieldType> \
		_name(FieldType* OwnerType::* _offset, const ValueType& _value) : \
			AttributeField<OwnerType, ValueType, SetterType>(_offset, _value) { } \
	}

	// макрос для инстансирования экземпляра атрибута
#define ATTRIBUTE_FIELD(_attribute, _class, _field, _value) \
	struct _attribute##_##_field \
	{ \
		_attribute##_##_field() \
		{ \
			static attribute::_attribute<_class> bind(&_class::_field, _value); \
		} \
	} _attribute##_##_field


	// шаблон для атрибута класса
	template <typename Type, typename ValueType>
	struct ClassAttribute
	{
		ClassAttribute(const ValueType& _value)
		{
			getData() = _value;
		}
		static ValueType& getData()
		{
			static ValueType data;
			return data;
		}
	};

	// макрос для инстансирования атрибута класса
#define DECLARE_ATTRIBUTE_CLASS(_name, _type) \
	template <typename Type, typename ValueType = _type> \
	struct _name : public attribute::ClassAttribute<_name<Type>, ValueType> \
	{ \
		_name(const ValueType& _value) : \
			ClassAttribute<_name<Type>, ValueType>(_value) { } \
	}

	// макрос для инстансирования экземпляра класса
#define ATTRIBUTE_CLASS(_attribute, _class, _value) \
	class _class; \
	static attribute::_attribute<_class> _attribute##_##_class(_value)
}

#endif // __ATTRIBUTE_H__
