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

#ifndef LMMS_GUI_ACTION_H
#define LMMS_GUI_ACTION_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

#include <QString>
//#include <QUndoCommand>

namespace lmms::gui
{

class ActionStruct;

class GuiActionIO
{
public:
	GuiActionIO();
	GuiActionIO(const GuiActionIO& ref);
	GuiActionIO(const size_t& value);
	GuiActionIO(const int& value);
	GuiActionIO(const float& value);
	GuiActionIO(const std::pair<float, float>& value);
	GuiActionIO(const std::vector<float>& value);
	GuiActionIO(bool* value);

	template<typename DataType>
	static constexpr size_t getTypeId() { return 0; }
	template<typename DataType>
	constexpr DataType* getValue()
	{
		// if this assert fails, you need to change your `DataType` template to the one this one was constructed with
		assert(false);
		return nullptr;
	}

	bool isValid() { return m_typeId != 0; };
private:
	size_t m_typeId = 0;

	size_t m_valueA = 0;
	int m_valueB = 0;
	float m_valueC = 0.0f;
	std::pair<float, float> m_valueD = {0.0f, 0.0f};
	std::vector<float> m_valueE = {};
	bool* m_valueF = nullptr;
};

class GuiAction// : public QUndoCommand
{
public:
	GuiAction(ActionStruct& parentAction, const GuiActionIO& data, size_t runAmount, bool linkBack);
	void undo();// override
	void redo();// override
	//! should be undone along with the action before this
	bool getShouldLinkBack();
	//! returns true if cleared, use this to delete pointers to destructing objects
	bool clearActionIfMatch(ActionStruct* action);
private:
	QString m_name;
	bool m_isLinkedBack; //! if this is undone, undo the one before this
	size_t m_runAmount;
	GuiActionIO m_data;
	ActionStruct* m_parentAction;
};

template<>
constexpr inline size_t GuiActionIO::getTypeId<size_t>() { return 1; }
template<>
constexpr inline size_t GuiActionIO::getTypeId<int>() { return 2; }
template<>
constexpr inline size_t GuiActionIO::getTypeId<float>() { return 3; }
template<>
constexpr inline size_t GuiActionIO::getTypeId<std::pair<float, float>>() { return 4; }
template<>
constexpr inline size_t GuiActionIO::getTypeId<std::vector<float>>() { return 5; }
template<>
constexpr inline size_t GuiActionIO::getTypeId<std::vector<bool*>>() { return 6; }

template<>
constexpr inline size_t* GuiActionIO::getValue()
{
	// if this assert fails, you need to change your `DataType` template to the one this one was constructed with
	assert(getTypeId<size_t>() == m_typeId);
	return &m_valueA;
}
template<>
constexpr inline int* GuiActionIO::getValue()
{
	// if this assert fails, you need to change your `DataType` template to the one this one was constructed with
	assert(getTypeId<int>() == m_typeId);
	return &m_valueB;
}
template<>
constexpr inline float* GuiActionIO::getValue()
{
	// if this assert fails, you need to change your `DataType` template to the one this one was constructed with
	//qDebug("GuiActionIO %ld, %ld", getTypeId<float>(), m_typeId);
	assert(getTypeId<float>() == m_typeId);
	return &m_valueC;
}
template<>
constexpr inline std::pair<float, float>* GuiActionIO::getValue()
{
	// if this assert fails, you need to change your `DataType` template to the one this one was constructed with
	assert((getTypeId<std::pair<float, float>>() == m_typeId));
	return &m_valueD;
}
template<>
constexpr inline std::vector<float>* GuiActionIO::getValue()
{
	// if this assert fails, you need to change your `DataType` template to the one this one was constructed with
	assert(getTypeId<std::vector<float>>() == m_typeId);
	return &m_valueE;
}
template<>
constexpr inline bool** GuiActionIO::getValue()
{
	// if this assert fails, you need to change your `DataType` template to the one this one was constructed with
	//qDebug("GuiActionIO %ld, %ld", getTypeId<float>(), m_typeId);
	assert(getTypeId<bool*>() == m_typeId);
	return &m_valueF;
}


} // namespace lmms::gui

#endif // LMMS_GUI_ACTION_H
