/*
 * CommandTest.cpp
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


#include <QtTest>
#include "Command.h"
#include "CommandStack.h"


class CommandTestObject
{
public:
	CommandTestObject(lmms::CommandStack& stack)
		: m_value{0.0f}
		, m_boolean{false}
		, m_setValueA{new lmms::ParamCommandLambda{stack,
			[this](float newVal, float& oldVal) { oldVal = m_value; setTestValue(newVal); },
			[this](float newVal, float oldVal) { setTestValue(oldVal); }, 0.0f}}
		, m_setValueB{stack, *this, &CommandTestObject::getTestValue, &CommandTestObject::setTestValue, nullptr} // getter
		, m_setValueC{stack, *this, nullptr, &CommandTestObject::setTestValue, &m_value} // reference
		, m_doActionA{new lmms::CommandLambda{stack,
			[this]() { doAction(); },
			[this]() { undoAction(); }}}
		, m_doActionB{stack, *this, &CommandTestObject::doAction, &CommandTestObject::undoAction}
	{}
	~CommandTestObject()
	{
		delete m_setValueA;
		delete m_doActionA;
	}
	float m_value;
	bool m_boolean;

	void doAction() { m_boolean = true; };
	void undoAction() { m_boolean = false; };

	void setTestValue(float value) { m_value = value; };
	float getTestValue() { return m_value; };

	lmms::ParamCommand<float>* m_setValueA;
	lmms::ParamCommandFnPtr<float, CommandTestObject, void> m_setValueB;
	lmms::ParamCommandFnPtr<float, CommandTestObject, void> m_setValueC;
	lmms::TypelessCommand* m_doActionA;
	lmms::CommandFnPtr<CommandTestObject, void, void> m_doActionB;
};

class CommandTest : public QObject
{
	Q_OBJECT
private slots: // tests

	void CommandTests()
	{
		using namespace lmms;

		CommandStack stack;
		CommandTestObject obj{stack};
		
		QVERIFY(obj.m_value == 0.0f); // these should be the default values
		QVERIFY(obj.m_boolean == false);

		// ParamCommand test
		obj.m_setValueA->push(1.0f);
		QVERIFY(obj.m_value == 1.0f); // failed to perform lambda command
		stack.undo();
		QVERIFY(obj.m_value == 0.0f); // failed to undo lambda command

		obj.m_setValueB(2.0f);
		QVERIFY(obj.m_value == 2.0f); // failed to perform "fnptr with getter" command
		stack.undo();
		QVERIFY(obj.m_value == 0.0f); // failed to undo "fnptr with getter" command

		obj.m_setValueC(3.0f);
		QVERIFY(obj.m_value == 3.0f); // failed to perform "fnptr with reference" command
		stack.undo();
		QVERIFY(obj.m_value == 0.0f); // failed to undo "fnptr with reference" command

		// CommandStack test
		obj.m_setValueA->push(1.0f);
		obj.m_setValueB(2.0f);
		obj.m_setValueC(3.0f);
		QVERIFY(obj.m_value == 3.0f); // failed to perform multiple command
		stack.undo();
		stack.undo();
		QVERIFY(obj.m_value == 1.0f); // failed to undo multiple command
		obj.m_setValueC(3.0f);
		stack.undo();
		QVERIFY(obj.m_value == 1.0f);

		// Command test
		obj.m_doActionA->push();
		QVERIFY(obj.m_boolean == true);
		stack.undo();
		QVERIFY(obj.m_boolean == false);
		obj.m_doActionB();
		QVERIFY(obj.m_boolean == true);
		stack.undo();
		QVERIFY(obj.m_boolean == false);
	}
};

QTEST_GUILESS_MAIN(CommandTest)
#include "CommandTest.moc"
