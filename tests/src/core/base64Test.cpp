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
    void Base64Tests()
    {
        using namespace lmms::base64;

        // Test Vectors from RFC 4648 Section 10
        std::vector<std::pair<std::string, std::string>> test_vectors{
            {"", ""},
            {"f", "Zg=="},
            {"fo", "Zm8="},
            {"foo", "Zm9v"},
            {"foob", "Zm9vYg=="},
            {"fooba", "Zm9vYmE="},
            {"foobar", "Zm9vYmFy"},
        };
        for (auto vector : test_vectors)
        {
            QCOMPARE(QString(encode(vector.first).c_str()), QString(vector.second.c_str()));
            QCOMPARE(QString(vector.first.c_str()), QString(decode(vector.second).c_str()));
        }
    }
} Base64Tests;

#include "base64Test.moc"
