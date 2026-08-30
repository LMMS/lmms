/*
 * Copyright (c) 2026 Ari Bradshaw
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include "SerializingObject.h"

#include <QDomDocument>
#include <QtTest>

namespace {

class TestObject : public lmms::SerializingObject
{
public:
	TestObject() { setHookAttributeNames({"visible", "x"}); }

	QString nodeName() const override { return "test"; }
	void setValue(int value) { m_value = value; }
	int value() const { return m_value; }

protected:
	void saveSettings(QDomDocument&, QDomElement& element) override { element.setAttribute("value", m_value); }
	void loadSettings(const QDomElement& element) override { m_value = element.attribute("value").toInt(); }

private:
	int m_value = 0;
};

class TestHook : public lmms::SerializingObjectHook
{
public:
	void saveSettings(QDomDocument&, QDomElement& element) override
	{
		element.setAttribute("visible", visible);
		element.setAttribute("x", x);
	}

	void loadSettings(const QDomElement& element) override
	{
		visible = element.attribute("visible").toInt();
		x = element.attribute("x").toInt();
		loaded = true;
	}

	int visible = 0;
	int x = 0;
	bool loaded = false;
};

} // namespace

class SerializingObjectTest : public QObject
{
	Q_OBJECT

private slots:
	void restoresAttributesWhenHookAttaches()
	{
		QDomDocument input;
		QVERIFY(input.setContent("<test value=\"7\" visible=\"1\" x=\"42\" ignored=\"8\"/>"));
		TestObject object;
		object.restoreState(input.documentElement());

		QCOMPARE(object.value(), 7);
		QCOMPARE(object.deferredHookAttribute("visible"), QString{"1"});
		QCOMPARE(object.deferredHookAttribute("ignored"), QString{});

		TestHook hook;
		object.setHook(&hook);
		QVERIFY(hook.loaded);
		QCOMPARE(hook.visible, 1);
		QCOMPARE(hook.x, 42);
		QCOMPARE(object.deferredHookAttribute("visible"), QString{});
	}

	void savesDeferredAttributesWithoutAHook()
	{
		QDomDocument input;
		QVERIFY(input.setContent("<test value=\"7\" visible=\"1\" x=\"42\" ignored=\"8\"/>"));
		TestObject object;
		object.restoreState(input.documentElement());
		object.setValue(9);

		QDomDocument output;
		QDomElement parent = output.createElement("parent");
		output.appendChild(parent);
		const QDomElement saved = object.saveState(output, parent);

		QCOMPARE(saved.attribute("value").toInt(), 9);
		QCOMPARE(saved.attribute("visible").toInt(), 1);
		QCOMPARE(saved.attribute("x").toInt(), 42);
		QVERIFY(!saved.hasAttribute("ignored"));
	}
};

QTEST_GUILESS_MAIN(SerializingObjectTest)
#include "SerializingObjectTest.moc"
