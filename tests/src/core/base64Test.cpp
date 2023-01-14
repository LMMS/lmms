/*
 * base64Test.cpp
 *
 * Copyright (c) 2022 Kevin Zander <veratil/at/gmail.com>
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

#include "base64.h"

class Base64Test : QTestSuite
{
    Q_OBJECT
private slots:
    void create_test_data()
    {
        QTest::addColumn<QString>("original");
        QTest::addColumn<QString>("encoded");

        // Test Vectors from RFC 4648 Section 10
        QTest::newRow("empty string")  << ""       << "";
        QTest::newRow("1 chunk 2 pad") << "f"      << "Zg==";
        QTest::newRow("1 chunk 1 pad") << "fo"     << "Zm8=";
        QTest::newRow("1 chunk 0 pad") << "foo"    << "Zm9v";
        QTest::newRow("2 chunk 2 pad") << "foob"   << "Zm9vYg==";
        QTest::newRow("2 chunk 1 pad") << "fooba"  << "Zm9vYmE=";
        QTest::newRow("2 chunk 0 pad") << "foobar" << "Zm9vYmFy";
    }
    void b64_encode_data()
    {
        create_test_data();
    }
    void b64_encode()
    {
        using namespace lmms::base64;

        QFETCH(QString, original);
        QFETCH(QString, encoded);
        QCOMPARE(QString(encode(original.toStdString()).c_str()), encoded);
    }
    void b64_decode_data()
    {
        create_test_data();
    }
    void b64_decode()
    {
        using namespace lmms::base64;

        QFETCH(QString, original);
        QFETCH(QString, encoded);
        QCOMPARE(original, QString(decode(encoded.toStdString()).c_str()));
    }
} Base64Tests;

#include "base64Test.moc"
