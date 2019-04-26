/*
 * AutomatableModelTest.cpp
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "QTestSuite.h"

#include "AutomatableModel.h"
#include "ComboBoxModel.h"

class AutomatableModelTest : QTestSuite
{
	Q_OBJECT

private slots:
	void CastTests()
	{
		ComboBoxModel comboModel;
		AutomatableModel* amPtr = &comboModel;
		QCOMPARE(nullptr, amPtr->dynamicCast<FloatModel>());
		QVERIFY(nullptr != amPtr->dynamicCast<AutomatableModel>());
		QVERIFY(nullptr != amPtr->dynamicCast<IntModel>());
		QVERIFY(nullptr != amPtr->dynamicCast<ComboBoxModel>());

		IntModel intModel;
		IntModel* imPtr = &intModel;
		QCOMPARE(nullptr, imPtr->dynamicCast<FloatModel>());
		QVERIFY(nullptr != imPtr->dynamicCast<AutomatableModel>());
		QVERIFY(nullptr != imPtr->dynamicCast<IntModel>());
		QCOMPARE(nullptr, imPtr->dynamicCast<ComboBoxModel>());
	}
} AutomatableModelTests;

#include "AutomatableModelTest.moc"
