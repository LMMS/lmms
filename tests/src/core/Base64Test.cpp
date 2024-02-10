/*
 * ArrayVectorTest.cpp
 *
 * Copyright (c) 2024 Jonah Janzen
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

#include "base64.h"

#include <QtTest/QtTest>

class Base64Test : public QObject
{
    Q_OBJECT
private slots:
    void encodeDecodeSingleFloatTest()
    {
        const float TEST_VALUE = 123.456789f;
        QString encodedString;
        lmms::base64::encode((const char *)&TEST_VALUE, sizeof(float), encodedString);

        float *decodedData = new float();
        int size;
        lmms::base64::decode(encodedString, &decodedData, &size);

        QCOMPARE(size, (int)sizeof(float));
        QVERIFY(*decodedData == TEST_VALUE);

        delete decodedData;
    }

    void encodeDecodeStringTest()
    {
        const char *TEST_STRING = "test string for base 64 encoding decoding";
        QString encodedString;

        lmms::base64::encode(TEST_STRING, strlen(TEST_STRING) + 1, encodedString);

        char *decodedData = new char[strlen(TEST_STRING)];
        int size;
        lmms::base64::decode(encodedString, &decodedData, &size);

        QVERIFY(strcmp(decodedData, TEST_STRING) == 0);

        delete[] decodedData;
    }

    void encodeDecodePositiveArrayTest()
    {
        const int TEST_COUNT = 4096;
        QString encodedString;

        float *positiveFloats = new float[TEST_COUNT];
        for(int i = 0; i < TEST_COUNT; i++) {
            positiveFloats[i] = 7.0f / i;
        }

        lmms::base64::encode((const char *)positiveFloats, TEST_COUNT * sizeof(float), encodedString);

        float *decodedData = new float[TEST_COUNT];
        int size;
        lmms::base64::decode(encodedString, &decodedData, &size);

        QCOMPARE(size, TEST_COUNT * (int)sizeof(float));
        QVERIFY(memcmp(positiveFloats, decodedData, TEST_COUNT * sizeof(float)) == 0);

        delete[] decodedData;
        delete[] positiveFloats;
    }

    void encodeDecodeNegativeArrayTest()
    {
        const int TEST_COUNT = 4096;
        QString encodedString;

        float *negativeFloats = new float[TEST_COUNT];
        for(int i = 0; i < TEST_COUNT; i++) {
            negativeFloats[i] = -7.0f / i;
        }

        lmms::base64::encode((const char *)negativeFloats, TEST_COUNT * sizeof(float), encodedString);

        float *decodedData = new float[TEST_COUNT];
        int size;
        lmms::base64::decode(encodedString, &decodedData, &size);

        QCOMPARE(size, TEST_COUNT * (int)sizeof(float));
        QVERIFY(memcmp(negativeFloats, decodedData, TEST_COUNT * sizeof(float)) == 0);

        delete[] decodedData;
        delete[] negativeFloats;
    }
};

QTEST_GUILESS_MAIN(Base64Test)
#include "Base64Test.moc"