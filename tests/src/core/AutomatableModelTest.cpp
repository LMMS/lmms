/*
 * AutomatableModelTest.cpp
 *
 * Copyright (c) 2020 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

class AutomatableModelTest : QTestSuite
{
	Q_OBJECT

	bool m1Changed, m2Changed;
	void resetChanged() { m1Changed = m2Changed = false; }

private slots: // helper slots
	void onM1Changed(Model* ) { m1Changed = true; }
	void onM2Changed(Model* ) { m2Changed = true; }

private slots: // tests
	void LinkTests()
	{
		BoolModel m1(false), m2(false);

		QObject::connect(&m1, SIGNAL(dataChanged(Model*)),
			this, SLOT(onM1Changed(Model*)));
		QObject::connect(&m2, SIGNAL(dataChanged(Model*)),
			this, SLOT(onM2Changed(Model*)));

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
	}
} AutomatableModelTests;

#include "AutomatableModelTest.moc"
