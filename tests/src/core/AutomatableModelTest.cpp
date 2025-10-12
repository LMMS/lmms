/*
 * AutomatableModelTest.cpp
 *
 * Copyright (c) 2019-2020 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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
#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "Engine.h"

class AutomatableModelTest : public QObject
{
	Q_OBJECT
public:
	bool m1Changed, m2Changed;
	void resetChanged() { m1Changed = m2Changed = false; }

private slots: // helper slots
	void onM1Changed() { m1Changed = true; }
	void onM2Changed() { m2Changed = true; }

private slots: // tests
	//! Test that upcast and exact casts work,
	//! but no downcast or any other casts

	void initTestCase()
	{
		using namespace lmms;
		Engine::init(true);
	}

	void cleanupTestCase()
	{
		using namespace lmms;
		Engine::destroy();
	}

	void CastTests()
	{
		using namespace lmms;

		ComboBoxModel comboModel;
		AutomatableModel* amPtr = &comboModel;
		QVERIFY(nullptr == amPtr->dynamicCast<FloatModel>()); // not a parent class
		QCOMPARE(&comboModel, amPtr->dynamicCast<AutomatableModel>()); // parent class
		QCOMPARE(&comboModel, amPtr->dynamicCast<IntModel>()); // parent class
		QCOMPARE(&comboModel, amPtr->dynamicCast<ComboBoxModel>()); // same class

		IntModel intModel;
		IntModel* imPtr = &intModel;
		QVERIFY(nullptr == imPtr->dynamicCast<FloatModel>()); // not a parent class
		QCOMPARE(&intModel, imPtr->dynamicCast<AutomatableModel>()); // parent class
		QCOMPARE(&intModel, imPtr->dynamicCast<IntModel>()); // same class
		QVERIFY(nullptr == imPtr->dynamicCast<ComboBoxModel>()); // child class
	}

	void LinkTests()
	{
		using namespace lmms;

		BoolModel m1(false), m2(false);

		QObject::connect(&m1, SIGNAL(dataChanged()),
			this, SLOT(onM1Changed()));
		QObject::connect(&m2, SIGNAL(dataChanged()),
			this, SLOT(onM2Changed()));

		resetChanged();
		AutomatableModel::linkModels(&m1, &m1);
		QVERIFY(!m1Changed); // cannot link to itself
		QVERIFY(!m2Changed);

		resetChanged();
		AutomatableModel::linkModels(&m1, &m2);
		QVERIFY(m1Changed); // since m1 takes the value of m2
		QVERIFY(!m2Changed); // the second model is the source

		resetChanged();
		AutomatableModel::linkModels(&m1, &m2);
		QVERIFY(!m1Changed); // it's already linked
		QVERIFY(!m2Changed);

		resetChanged();
		BoolModel m3(false);
		m1.setValue(1.f);
		m2.setValue(1.f);
		AutomatableModel::linkModels(&m1, &m2);
		QVERIFY(m1.value());
		QVERIFY(m2.value());
		QVERIFY(!m3.value());
		AutomatableModel::linkModels(&m2, &m3); // drag m3, drop on m2
		// m2 should take m3's (0) value
		// due to a bug(?), this does not happen
		QVERIFY(m2.value());
		QVERIFY(!m3.value());
	}
};

QTEST_GUILESS_MAIN(AutomatableModelTest)
#include "AutomatableModelTest.moc"
