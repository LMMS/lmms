/*
 * GuiAction.h - implements Actions, a layer between the user and widgets
 *
 * Copyright (c) 2025 szeli1 <TODO/at/gmail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <cassert>

#include "GuiAction.h"

namespace lmms::gui
{

class InteractiveModelView;

template<typename BaseType, DataType>
class LMMS_EXPORT ActionSafeFnPtrTyped
{
public:
typedef void (BaseType::*FunctionPointer)(BaseType*, DataType);

template<typename BaseType, DataType>
ActionSafeFnPtrTyped::ActionSafeFnPtrTyped(FunctionPointer<BaseType> function)
{
	m_functionPtr = function;
	m_baseTypeId = typeid(BaseType).hash_code();
	m_dataTypeId = typeid(DataType).hash_code();
}

template<typename BaseType, DataType>
ActionSafeFnPtrTyped::callFn(BaseType* object, std::shared_ptr<DataType> data)
{
	assert();
}

private:
	size_t m_baseTypeId;
	size_t m_dataTypeId;
	
	FunctionPointer m_functionPtr;
};


template<typename BaseType>
class LMMS_EXPORT ActionSafeFnPtr
{
public:
	typedef void (BaseType::*FunctionPointer)();
	ActionSafeFnPtr(FunctionPointer function);
	callFn(BaseType* object);
private:
	size_t m_baseTypeId;
	
	FunctionPointer m_functionPtr;
};


