/*
 * ArrayVectorTest.cpp
 *
 * Copyright (c) 2023 Dominic Clark
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

#include "ArrayVector.h"

#include <QObject>
#include <QtTest>
#include <array>
#include <iterator>

using lmms::ArrayVector;

struct ShouldNotConstruct
{
	ShouldNotConstruct() { QFAIL("should not construct"); }
};

struct ShouldNotDestruct
{
	~ShouldNotDestruct() { QFAIL("should not destruct"); }
};

enum class Construction { Default, Copy, Move, CopyAssign, MoveAssign };

struct Constructible
{
	Constructible() : construction{Construction::Default} {}
	Constructible(const Constructible&) : construction{Construction::Copy} {}
	Constructible(Constructible&&) : construction{Construction::Move} {}
	Constructible& operator=(const Constructible&) { construction = Construction::CopyAssign; return *this; }
	Constructible& operator=(Constructible&&) { construction = Construction::MoveAssign; return *this; }
	Construction construction;
};

struct DestructorCheck
{
	~DestructorCheck() { *destructed = true; }
	bool* destructed;
};

class ArrayVectorTest : public QObject
{
	Q_OBJECT
private slots:
	void defaultConstructorTest()
	{
		// Ensure no elements are constructed
		const auto v = ArrayVector<ShouldNotConstruct, 1>();
		// Ensure the container is empty
		QVERIFY(v.empty());
	}

	void copyConstructorTest()
	{
		{
			// Ensure all elements are copy constructed
			const auto v = ArrayVector<Constructible, 1>{{}};
			const auto copy = v;
			for (const auto& element : copy) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure corresponding elements are used
			const auto v = ArrayVector<int, 5>{1, 2, 3};
			const auto copy = v;
			const auto expected = std::array{1, 2, 3};
			QVERIFY(std::equal(copy.begin(), copy.end(), expected.begin(), expected.end()));
		}
	}

	void moveConstructorTest()
	{
		{
			// Ensure all elements are move constructed
			auto v = ArrayVector<Constructible, 1>{{}};
			const auto moved = std::move(v);
			for (const auto& element : moved) {
				QCOMPARE(element.construction, Construction::Move);
			}
		}
		{
			// Ensure corresponding elements are used
			auto v = ArrayVector<int, 5>{1, 2, 3};
			const auto moved = std::move(v);
			const auto expected = std::array{1, 2, 3};
			QVERIFY(std::equal(moved.begin(), moved.end(), expected.begin(), expected.end()));
			// Move construction should leave the source empty
			QVERIFY(v.empty());
		}
	}

	void fillValueConstructorTest()
	{
		// Ensure all elements are copy constructed
		const auto v = ArrayVector<Constructible, 2>(1, {});
		for (const auto& element : v) {
			QCOMPARE(element.construction, Construction::Copy);
		}
		// Ensure the container has the correct size
		QCOMPARE(v.size(), std::size_t{1});
	}

	void fillDefaultConstructorTest()
	{
		// Ensure all elements are copy constructed
		const auto v = ArrayVector<Constructible, 2>(1);
		for (const auto& element : v) {
			QCOMPARE(element.construction, Construction::Default);
		}
		// Ensure the container has the correct size
		QCOMPARE(v.size(), std::size_t{1});
	}

	void rangeConstructorTest()
	{
		{
			// Ensure the elements are copy constructed from normal iterators
			const auto data = std::array{Constructible{}};
			const auto v = ArrayVector<Constructible, 1>(data.begin(), data.end());
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure the elements are move constructed from move iterators
			auto data = std::array{Constructible{}};
			const auto v = ArrayVector<Constructible, 1>(
				std::move_iterator{data.begin()}, std::move_iterator{data.end()});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::Move);
			}
		}
		{
			// Ensure corresponding elements are used
			const auto data = std::array{1, 2, 3};
			const auto v = ArrayVector<int, 5>(data.begin(), data.end());
			QVERIFY(std::equal(v.begin(), v.end(), data.begin(), data.end()));
		}
	}

	void initializerListConstructorTest()
	{
		// Ensure the container is constructed with the correct data
		const auto v = ArrayVector<int, 5>{1, 2, 3};
		const auto expected = std::array{1, 2, 3};
		QVERIFY(std::equal(v.begin(), v.end(), expected.begin(), expected.end()));
	}

	void destructorTest()
	{
		{
			// Should not call destructors for space without elements
			const auto v = ArrayVector<ShouldNotDestruct, 1>{};
		}
		{
			// Should call destructors for all elements
			auto destructed = false;
			{
				const auto v = ArrayVector<DestructorCheck, 1>{{&destructed}};
			}
			QVERIFY(destructed);
		}
	}

	void copyAssignmentTest()
	{
		{
			// Self-assignment should not change the contents
			auto v = ArrayVector<int, 5>{1, 2, 3};
			const auto oldValue = v;
			v = v;
			QCOMPARE(v, oldValue);
		}
		{
			// Assignment to a larger container should copy assign
			const auto src = ArrayVector<Constructible, 5>(3);
			auto dst = ArrayVector<Constructible, 5>(5);
			dst = src;
			QCOMPARE(dst.size(), std::size_t{3});
			for (const auto& element : dst) {
				QCOMPARE(element.construction, Construction::CopyAssign);
			}
		}
		{
			// Assignment to a smaller container should copy construct
			const auto src = ArrayVector<Constructible, 5>(3);
			auto dst = ArrayVector<Constructible, 5>{};
			dst = src;
			QCOMPARE(dst.size(), std::size_t{3});
			for (const auto& element : dst) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure corresponding elements are used
			const auto src = ArrayVector<int, 5>{1, 2, 3};
			auto dst = ArrayVector<int, 5>{};
			dst = src;
			QCOMPARE(dst, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void moveAssignmentTest()
	{
		{
			// Self-assignment should not change the contents
			//// Please note the following:
			//// https://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#2468
			//// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81159
			auto v = ArrayVector<int, 5>{1, 2, 3};
			const auto oldValue = v;
#if __GNUC__ >= 13
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wself-move"
#endif
			v = std::move(v);
#if __GNUC__ >= 13
#	pragma GCC diagnostic pop
#endif
			QCOMPARE(v, oldValue);
		}
		{
			// Assignment to a larger container should move assign
			auto src = ArrayVector<Constructible, 5>(3);
			auto dst = ArrayVector<Constructible, 5>(5);
			dst = std::move(src);
			QCOMPARE(dst.size(), std::size_t{3});
			for (const auto& element : dst) {
				QCOMPARE(element.construction, Construction::MoveAssign);
			}
		}
		{
			// Assignment to a smaller container should move construct
			auto src = ArrayVector<Constructible, 5>(3);
			auto dst = ArrayVector<Constructible, 5>{};
			dst = std::move(src);
			QCOMPARE(dst.size(), std::size_t{3});
			for (const auto& element : dst) {
				QCOMPARE(element.construction, Construction::Move);
			}
		}
		{
			// Ensure corresponding elements are used
			auto src = ArrayVector<int, 5>{1, 2, 3};
			auto dst = ArrayVector<int, 5>{};
			dst = std::move(src);
			QCOMPARE(dst, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void initializerListAssignmentTest()
	{
		{
			// Assignment to a larger container should copy assign
			auto v = ArrayVector<Constructible, 2>(2);
			v = {Constructible{}};
			QCOMPARE(v.size(), std::size_t{1});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::CopyAssign);
			}
		}
		{
			// Assignment to a smaller container should copy construct
			auto v = ArrayVector<Constructible, 2>{};
			v = {Constructible{}};
			QCOMPARE(v.size(), std::size_t{1});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure corresponding elements are used
			auto v = ArrayVector<int, 5>{};
			v = {1, 2, 3};
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void fillValueAssignTest()
	{
		{
			// Assignment to a larger container should copy assign
			auto v = ArrayVector<Constructible, 5>(5);
			v.assign(3, {});
			QCOMPARE(v.size(), std::size_t{3});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::CopyAssign);
			}
		}
		{
			// Assignment to a smaller container should copy construct
			auto v = ArrayVector<Constructible, 5>{};
			v.assign(3, {});
			QCOMPARE(v.size(), std::size_t{3});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure correct value is filled
			auto v = ArrayVector<int, 5>{};
			v.assign(3, 1);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 1, 1}));
		}
	}

	void rangeAssignTest()
	{
		{
			// Assignment to a larger container should copy assign
			const auto data = std::array{Constructible{}};
			auto v = ArrayVector<Constructible, 2>(2);
			v.assign(data.begin(), data.end());
			QCOMPARE(v.size(), std::size_t{1});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::CopyAssign);
			}
		}
		{
			// Assignment to a smaller container should copy construct
			const auto data = std::array{Constructible{}};
			auto v = ArrayVector<Constructible, 2>{};
			v.assign(data.begin(), data.end());
			QCOMPARE(v.size(), std::size_t{1});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure correct value is filled
			const auto data = std::array{1, 2, 3};
			auto v = ArrayVector<int, 5>{};
			v.assign(data.begin(), data.end());
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void atTest()
	{
		{
			// Non-const version
			auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v.at(1), 2);
			QVERIFY_EXCEPTION_THROWN(v.at(3), std::out_of_range);
		}
		{
			// Const version
			const auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v.at(1), 2);
			QVERIFY_EXCEPTION_THROWN(v.at(3), std::out_of_range);
		}
	}

	void subscriptTest()
	{
		{
			// Non-const version
			auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v[1], 2);
		}
		{
			// Const version
			const auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v[1], 2);
		}
	}

	void frontTest()
	{
		{
			// Non-const version
			auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v.front(), 1);
		}
		{
			// Const version
			const auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v.front(), 1);
		}
	}

	void backTest()
	{
		{
			// Non-const version
			auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v.back(), 3);
		}
		{
			// Const version
			const auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v.back(), 3);
		}
	}

	void dataTest()
	{
		{
			// Non-const version
			auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v.data(), &v.front());
		}
		{
			// Const version
			const auto v = ArrayVector<int, 5>{1, 2, 3};
			QCOMPARE(v.data(), &v.front());
		}
	}

	void beginEndTest()
	{
		const auto expected = std::array{1, 2, 3};
		{
			// Non-const version
			auto v = ArrayVector<int, 5>{1, 2, 3};
			QVERIFY(std::equal(v.begin(), v.end(), expected.begin(), expected.end()));
			QVERIFY(std::equal(v.cbegin(), v.cend(), expected.begin(), expected.end()));
		}
		{
			// Const version
			const auto v = ArrayVector<int, 5>{1, 2, 3};
			QVERIFY(std::equal(v.begin(), v.end(), expected.begin(), expected.end()));
		}
	}

	void rbeginRendTest()
	{
		const auto expected = std::array{3, 2, 1};
		{
			// Non-const version
			auto v = ArrayVector<int, 5>{1, 2, 3};
			QVERIFY(std::equal(v.rbegin(), v.rend(), expected.begin(), expected.end()));
			QVERIFY(std::equal(v.crbegin(), v.crend(), expected.begin(), expected.end()));
		}
		{
			// Const version
			const auto v = ArrayVector<int, 5>{1, 2, 3};
			QVERIFY(std::equal(v.rbegin(), v.rend(), expected.begin(), expected.end()));
		}
	}

	void emptyFullSizeMaxCapacityTest()
	{
		auto v = ArrayVector<int, 2>{};
		QVERIFY(v.empty());
		QVERIFY(!v.full());
		QCOMPARE(v.size(), std::size_t{0});
		QCOMPARE(v.max_size(), std::size_t{2});
		QCOMPARE(v.capacity(), std::size_t{2});

		v.push_back(1);
		QVERIFY(!v.empty());
		QVERIFY(!v.full());
		QCOMPARE(v.size(), std::size_t{1});
		QCOMPARE(v.max_size(), std::size_t{2});
		QCOMPARE(v.capacity(), std::size_t{2});

		v.push_back(2);
		QVERIFY(!v.empty());
		QVERIFY(v.full());
		QCOMPARE(v.size(), std::size_t{2});
		QCOMPARE(v.max_size(), std::size_t{2});
		QCOMPARE(v.capacity(), std::size_t{2});

		auto empty = ArrayVector<int, 0>{};
		QVERIFY(empty.empty());
		QVERIFY(empty.full());
		QCOMPARE(empty.size(), std::size_t{0});
		QCOMPARE(empty.max_size(), std::size_t{0});
		QCOMPARE(empty.capacity(), std::size_t{0});
	}

	void insertValueTest()
	{
		{
			// Copy
			const auto data = Constructible{};
			auto v = ArrayVector<Constructible, 1>{};
			v.insert(v.cbegin(), data);
			QCOMPARE(v.size(), std::size_t{1});
			QCOMPARE(v[0].construction, Construction::Copy);
		}
		{
			// Move
			auto v = ArrayVector<Constructible, 1>{};
			v.insert(v.cbegin(), Constructible{});
			QCOMPARE(v.size(), std::size_t{1});
			QCOMPARE(v[0].construction, Construction::Move);
		}
		{
			// Ensure the correct value is used (copy)
			const auto data = 1;
			auto v = ArrayVector<int, 5>{2, 3};
			v.insert(v.cbegin(), data);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
		{
			// Ensure the correct value is used (move)
			auto v = ArrayVector<int, 5>{2, 3};
			v.insert(v.cbegin(), 1);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void insertFillValueTest()
	{
		{
			// Insertion should copy construct
			auto v = ArrayVector<Constructible, 5>{};
			v.insert(v.cbegin(), 3, {});
			QCOMPARE(v.size(), std::size_t{3});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure correct value is filled
			auto v = ArrayVector<int, 5>{1, 3};
			v.insert(v.cbegin() + 1, 3, 2);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 2, 2, 3}));
		}
	}

	void insertRangeTest()
	{
		{
			// Insertion should copy construct
			const auto data = std::array{Constructible{}};
			auto v = ArrayVector<Constructible, 2>{};
			v.insert(v.cbegin(), data.begin(), data.end());
			QCOMPARE(v.size(), std::size_t{1});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure correct value is filled
			const auto data = std::array{2, 3};
			auto v = ArrayVector<int, 5>{1, 4};
			v.insert(v.cbegin() + 1, data.begin(), data.end());
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3, 4}));
		}
	}

	void insertInitializerListTest()
	{
		{
			// Insertion should copy construct
			auto v = ArrayVector<Constructible, 2>{};
			v.insert(v.cbegin(), {Constructible{}});
			QCOMPARE(v.size(), std::size_t{1});
			for (const auto& element : v) {
				QCOMPARE(element.construction, Construction::Copy);
			}
		}
		{
			// Ensure corresponding elements are used
			auto v = ArrayVector<int, 5>{1, 4};
			v.insert(v.cbegin() + 1, {2, 3});
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3, 4}));
		}
	}

	void emplaceTest()
	{
		{
			// Ensure the value is constructed in-place
			auto v = ArrayVector<Constructible, 1>{};
			v.emplace(v.cbegin());
			QCOMPARE(v.size(), std::size_t{1});
			QCOMPARE(v[0].construction, Construction::Default);
		}
		{
			// Ensure the correct value is used (move)
			auto v = ArrayVector<int, 5>{2, 3};
			v.emplace(v.cbegin(), 1);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void eraseTest()
	{
		{
			// Ensure destructors are run
			auto destructed = false;
			auto v = ArrayVector<DestructorCheck, 1>{{&destructed}};
			v.erase(v.cbegin());
			QVERIFY(destructed);
		}
		{
			// Ensure the result is correct
			auto v = ArrayVector<int, 5>{10, 1, 2, 3};
			v.erase(v.cbegin());
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void eraseRangeTest()
	{
		{
			// Ensure destructors are run
			auto destructed = false;
			auto v = ArrayVector<DestructorCheck, 1>{{&destructed}};
			v.erase(v.cbegin(), v.cend());
			QVERIFY(destructed);
		}
		{
			// Ensure the result is correct
			auto v = ArrayVector<int, 5>{1, 20, 21, 2, 3};
			v.erase(v.cbegin() + 1, v.cbegin() + 3);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void pushBackTest()
	{
		{
			// Copy
			const auto data = Constructible{};
			auto v = ArrayVector<Constructible, 1>{};
			v.push_back(data);
			QCOMPARE(v.size(), std::size_t{1});
			QCOMPARE(v[0].construction, Construction::Copy);
		}
		{
			// Move
			auto v = ArrayVector<Constructible, 1>{};
			v.push_back({});
			QCOMPARE(v.size(), std::size_t{1});
			QCOMPARE(v[0].construction, Construction::Move);
		}
		{
			// Ensure the correct value is used (copy)
			const auto data = 3;
			auto v = ArrayVector<int, 5>{1, 2};
			v.push_back(data);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
		{
			// Ensure the correct value is used (move)
			auto v = ArrayVector<int, 5>{1, 2};
			v.push_back(3);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void emplaceBackTest()
	{
		{
			// Ensure the value is constructed in-place
			auto v = ArrayVector<Constructible, 1>{};
			v.emplace_back();
			QCOMPARE(v.size(), std::size_t{1});
			QCOMPARE(v[0].construction, Construction::Default);
		}
		{
			// Ensure the correct value is used (move)
			auto v = ArrayVector<int, 5>{1, 2};
			v.emplace_back(3);
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2, 3}));
		}
	}

	void popBackTest()
	{
		{
			// Ensure destructors are run
			auto destructed = false;
			auto v = ArrayVector<DestructorCheck, 1>{{&destructed}};
			v.pop_back();
			QVERIFY(destructed);
		}
		{
			// Ensure the result is correct
			auto v = ArrayVector<int, 5>{1, 2, 3};
			v.pop_back();
			QCOMPARE(v, (ArrayVector<int, 5>{1, 2}));
		}
	}

	void resizeDefaultTest()
	{
		{
			// Smaller
			auto destructed = false;
			auto v = ArrayVector<DestructorCheck, 1>{{&destructed}};
			QCOMPARE(v.size(), std::size_t{1});
			v.resize(0);
			QCOMPARE(v.size(), std::size_t{0});
			QVERIFY(destructed);
		}
		{
			// Bigger
			auto v = ArrayVector<Constructible, 1>{};
			QCOMPARE(v.size(), std::size_t{0});
			v.resize(1);
			QCOMPARE(v.size(), std::size_t{1});
			QCOMPARE(v[0].construction, Construction::Default);
		}
		{
			// Too big
			auto v = ArrayVector<int, 1>{};
			QVERIFY_EXCEPTION_THROWN(v.resize(2), std::length_error);
		}
	}

	void resizeValueTest()
	{
		{
			// Smaller
			auto dummy = false;
			auto destructed = false;
			auto v = ArrayVector<DestructorCheck, 1>{{&destructed}};
			QCOMPARE(v.size(), std::size_t{1});
			v.resize(0, {&dummy});
			QCOMPARE(v.size(), std::size_t{0});
			QVERIFY(destructed);
		}
		{
			// Bigger
			auto v = ArrayVector<Constructible, 1>{};
			QCOMPARE(v.size(), std::size_t{0});
			v.resize(1, {});
			QCOMPARE(v.size(), std::size_t{1});
			QCOMPARE(v[0].construction, Construction::Copy);
		}
		{
			// Too big
			auto v = ArrayVector<int, 1>{};
			QVERIFY_EXCEPTION_THROWN(v.resize(2), std::length_error);
		}
		{
			// Ensure the correct value is used
			auto v = ArrayVector<int, 1>{};
			v.resize(1, 1);
			QCOMPARE(v, (ArrayVector<int, 1>{1}));
		}
	}

	void clearTest()
	{
		{
			// Ensure destructors are run
			auto destructed = false;
			auto v = ArrayVector<DestructorCheck, 1>{{&destructed}};
			v.clear();
			QVERIFY(destructed);
		}
		{
			// Ensure the result is correct
			auto v = ArrayVector<int, 5>{1, 2, 3};
			v.clear();
			QCOMPARE(v, (ArrayVector<int, 5>{}));
		}
	}

	void memberSwapTest()
	{
		auto a = ArrayVector<int, 5>{1, 2, 3, 4};
		auto b = ArrayVector<int, 5>{2, 4, 6};

		const auto aOriginal = a;
		const auto bOriginal = b;

		a.swap(b);

		QCOMPARE(a, bOriginal);
		QCOMPARE(b, aOriginal);
	}

	void freeSwapTest()
	{
		auto a = ArrayVector<int, 5>{1, 2, 3, 4};
		auto b = ArrayVector<int, 5>{2, 4, 6};

		const auto aOriginal = a;
		const auto bOriginal = b;

		swap(a, b);

		QCOMPARE(a, bOriginal);
		QCOMPARE(b, aOriginal);
	}

	void comparisonTest()
	{
		const auto v = ArrayVector<int, 5>{1, 2, 3};
		const auto l = ArrayVector<int, 5>{1, 2, 2};
		const auto e = ArrayVector<int, 5>{1, 2, 3};
		const auto g = ArrayVector<int, 5>{1, 3, 3};

		QVERIFY(l < v);
		QVERIFY(!(e < v));
		QVERIFY(!(g < v));

		QVERIFY(l <= v);
		QVERIFY(e <= v);
		QVERIFY(!(g <= v));

		QVERIFY(!(l > v));
		QVERIFY(!(e > v));
		QVERIFY(g > v);

		QVERIFY(!(l >= v));
		QVERIFY(e >= v);
		QVERIFY(g >= v);

		QVERIFY(!(l == v));
		QVERIFY(e == v);
		QVERIFY(!(g == v));

		QVERIFY(l != v);
		QVERIFY(!(e != v));
		QVERIFY(g != v);
	}
};

QTEST_GUILESS_MAIN(ArrayVectorTest)
#include "ArrayVectorTest.moc"
