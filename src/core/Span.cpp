/*
 * Span.cpp - Compile time tests for Span
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "Span.h"

namespace lmms
{

namespace default_construction_tests
{
	// static extent
	constexpr auto span1 = Span<int, 0>{};

	static_assert(std::is_same_v<decltype(span1), const Span<int, 0>>);
	static_assert(span1.extent == 0);
	static_assert(span1.size() == 0);
	static_assert(span1.size_bytes() == 0);
	static_assert(span1.empty());
	static_assert(span1.data() == nullptr);
	static_assert(span1.begin() == span1.end());

	// dynamic extent
	constexpr auto span2 = Span<int>{};

	static_assert(std::is_same_v<decltype(span2), const Span<int>>);
	static_assert(std::is_same_v<decltype(span2), const Span<int, dynamic_extent>>);
	static_assert(span2.extent == dynamic_extent);
	static_assert(span2.size() == 0);
	static_assert(span2.size_bytes() == 0);
	static_assert(span2.empty());
	static_assert(span2.data() == nullptr);
	static_assert(span2.begin() == span2.end());

	constexpr void test1()
	{
		constexpr auto span1 = Span<int>{};
		static_assert(std::is_same_v<decltype(span1), const Span<int, dynamic_extent>>);

		constexpr auto span2 = Span<bool, 0>(); // NOTE: curly brackets {} don't work here
		static_assert(std::is_same_v<decltype(span2), const Span<bool, 0>>);
		static_assert(span2.extent == 0);
	}
} // namespace default_construction_tests


namespace pointer_and_size_construction_tests
{
	// static extent
	constexpr int arr[] = {1, 2, 3};
	constexpr auto span1 = Span<const int, 3>{&arr[0], 3};

	static_assert(std::is_same_v<decltype(span1), const Span<const int, 3>>);
	static_assert(span1.extent == 3);
	static_assert(span1.size() == 3);
	static_assert(span1.data() == arr);

	constexpr void nonconst_to_const_static_extent()
	{
		int arr[] = {1, 2, 3};
		auto span1 = Span<const int, 3>{&arr[0], 3};

		static_assert(std::is_same_v<decltype(span1), Span<const int, 3>>);
		static_assert(span1.extent == 3);
	}

	// dynamic extent
	constexpr auto span2 = Span{&arr[0], 3};

	static_assert(std::is_same_v<decltype(span2), const Span<const int, dynamic_extent>>);
	static_assert(span2.extent == dynamic_extent);
	static_assert(span2.size() == 3);
	static_assert(span2.data() == arr);

	constexpr void nonconst_to_const_dynamic_extent()
	{
		int arr[] = {1, 2, 3};
		auto span1 = Span<const int, dynamic_extent>{&arr[0], 3};

		static_assert(std::is_same_v<decltype(span1), Span<const int, dynamic_extent>>);
		static_assert(span1.extent == dynamic_extent);
	}
} // namespace pointer_and_size_construction_tests


namespace c_array_construction_tests
{
	// static extent
	constexpr int arr[] = {1, 2, 3};
	constexpr auto span1 = Span{arr};

	static_assert(std::is_same_v<decltype(span1), const Span<const int, 3>>);
	static_assert(span1.extent == 3);
	static_assert(span1.size() == 3);
	static_assert(!span1.empty());
	static_assert(span1.data() == arr);
	static_assert(span1.begin() != span1.end());
	static_assert(span1[1] == 2);
	static_assert(span1.front() == 1);
	static_assert(span1.back() == 3);

	// dynamic extent
	constexpr auto span2 = Span<const int, dynamic_extent>{arr};

	static_assert(std::is_same_v<decltype(span2), const Span<const int, dynamic_extent>>);
	static_assert(span2.extent == dynamic_extent);
	static_assert(span2.size() == 3);
	static_assert(!span2.empty());
	static_assert(span2.data() == arr);
	static_assert(span2.begin() != span2.end());
	static_assert(span2[1] == 2);
	static_assert(span2.front() == 1);
	static_assert(span2.back() == 3);
} // namespace c_array_construction_tests


namespace std_array_construction_tests
{
	// static extent
	constexpr auto std_arr = std::array{4, 5, 6};
	constexpr auto span1 = Span{std_arr};

	static_assert(std::is_same_v<decltype(span1), const Span<const int, 3>>);
	static_assert(span1.extent == 3);
	static_assert(span1.size() == 3);
	static_assert(!span1.empty());
	static_assert(span1.data() == std_arr.data());
	static_assert(span1.begin() != span1.end());
	static_assert(span1[1] == 5);

	constexpr void nonconst_static_extent()
	{
		auto std_arr = std::array{4, 5, 6};
		auto span1 = Span{std_arr};

		static_assert(std::is_same_v<decltype(span1), Span<int, 3>>);
		static_assert(span1.extent == 3);
		static_assert(span1.size() == 3);
		static_assert(!span1.empty());
	}

	// dynamic extent
	constexpr auto span2 = Span<const int, dynamic_extent>{std_arr};

	static_assert(std::is_same_v<decltype(span2), const Span<const int, dynamic_extent>>);
	static_assert(span2.extent == dynamic_extent);
	static_assert(span2.size() == 3);
	static_assert(!span2.empty());
	static_assert(span2.data() == std_arr.data());
	static_assert(span2.begin() != span2.end());
	static_assert(span2[1] == 5);

	constexpr void nonconst_dynamic_extent()
	{
		auto std_arr = std::array{4, 5, 6};
		auto span1 = Span<int, dynamic_extent>{std_arr};

		static_assert(std::is_same_v<decltype(span1), Span<int, dynamic_extent>>);
		static_assert(span1.extent == dynamic_extent);
	}
} // namespace std_array_construction_tests


namespace initializer_list_construction_tests
{
	// static extent
	constexpr auto il = {1, 2, 3};
	constexpr auto span1 = Span<const int, 3>{il};

	static_assert(std::is_same_v<decltype(span1), const Span<const int, 3>>);
	static_assert(span1.extent == 3);

	constexpr bool func_static_extent(Span<const int, 3> il)
	{
		return true;
	}
	static_assert(func_static_extent(Span<const int, 3>{7, 8, 9})); // explicit constructor

	// dynamic extent
	constexpr auto span2 = Span<const int, dynamic_extent>{il};

	static_assert(std::is_same_v<decltype(span2), const Span<const int, dynamic_extent>>);
	static_assert(span2.extent == dynamic_extent);

	constexpr bool func_dynamic_extent(Span<const int> il)
	{
		assert(il.size() == 4);
		return true;
	}
	static_assert(func_dynamic_extent({7, 8, 9, 0})); // non-explicit constructor


} // namespace initializer_list_construction_tests


namespace span_converting_construction_tests
{
	constexpr const int const_arr[] = {1, 2, 3};
	int nonconst_arr[] = {1, 2, 3};

	// const static extent --> const dynamic extent
	constexpr auto source1 = Span<const int, 3>{const_arr};
	constexpr auto dest1 = Span<const int, dynamic_extent>{source1};

	// non-const static extent --> const dynamic extent
	constexpr bool func2()
	{
		auto source2 = Span<int, 3>{nonconst_arr};
		auto dest2 = Span<const int, dynamic_extent>{source2};
		(void)dest2;
		return true;
	}
	static_assert(func2());

	// non-const static extent --> non-const dynamic extent
	constexpr bool func3()
	{
		auto source3 = Span<int, 3>{nonconst_arr};
		auto dest3 = Span<int, dynamic_extent>{source3};
		(void)dest3;
		return true;
	}
	static_assert(func3());

	// const dynamic extent --> const static extent
	constexpr auto source4 = Span<const int, dynamic_extent>{const_arr};
	constexpr auto dest4 = Span<const int, 3>{source4};

	// non-const dynamic extent --> const static extent
	constexpr bool func5()
	{
		auto source5 = Span<int, dynamic_extent>{nonconst_arr};
		auto dest5 = Span<const int, 3>{source5};
		(void)dest5;
		return true;
	}
	static_assert(func5());

	// non-const dynamic extent --> non-const static extent
	constexpr bool func6()
	{
		auto source6 = Span<int, dynamic_extent>{nonconst_arr};
		auto dest6 = Span<int, 3>{source6};
		(void)dest6;
		return true;
	}
	static_assert(func6());
} // namespace span_converting_construction_tests


namespace copy_construction_tests
{
	constexpr int arr[] = {1, 2, 3};

	// copy constructor
	constexpr auto span1 = Span{arr};
	constexpr auto span2{span1};

	static_assert(std::is_same_v<decltype(span2), const Span<const int, 3>>);
	static_assert(span2[0] == 1);

	// copy assignment
	constexpr auto span3 = span1;

	static_assert(std::is_same_v<decltype(span3), const Span<const int, 3>>);
	static_assert(span3[2] == 3);
} // namespace copy_construction_tests


namespace subview_tests
{
	// static extents
	constexpr auto arr1 = std::array{1, 3, 5, 7, 9};
	constexpr auto span1 = Span<const int, 5>{arr1};

	constexpr bool test_template_args1()
	{
		constexpr auto first1 = span1.first<3>();
		static_assert(first1.size() == 3);
		static_assert(first1[0] == 1); static_assert(first1[1] == 3); static_assert(first1[2] == 5);

		constexpr auto last1 = span1.last<3>();
		static_assert(last1.size() == 3);
		static_assert(last1[0] == 5); static_assert(last1[1] == 7); static_assert(last1[2] == 9);

		constexpr auto sub1 = span1.subspan<1, 3>();
		static_assert(sub1.size() == 3);
		static_assert(sub1[0] == 3); static_assert(sub1[1] == 5); static_assert(sub1[2] == 7);

		constexpr auto sub2 = span1.subspan<1>();
		static_assert(sub2.size() == 4);
		static_assert(sub2[0] == 3); static_assert(sub2[1] == 5);
		static_assert(sub2[2] == 7); static_assert(sub2[3] == 9);

		return true;
	}
	static_assert(test_template_args1());

	constexpr bool test_runtime_args1()
	{
		constexpr auto first1 = span1.first(3);
		static_assert(first1.size() == 3);
		static_assert(first1[0] == 1); static_assert(first1[1] == 3); static_assert(first1[2] == 5);

		constexpr auto last1 = span1.last(3);
		static_assert(last1.size() == 3);
		static_assert(last1[0] == 5); static_assert(last1[1] == 7); static_assert(last1[2] == 9);

		constexpr auto sub1 = span1.subspan(1, 3);
		static_assert(sub1.size() == 3);
		static_assert(sub1[0] == 3); static_assert(sub1[1] == 5); static_assert(sub1[2] == 7);

		constexpr auto sub2 = span1.subspan<1>();
		static_assert(sub2.size() == 4);
		static_assert(sub2[0] == 3); static_assert(sub2[1] == 5);
		static_assert(sub2[2] == 7); static_assert(sub2[3] == 9);

		return true;
	}
	static_assert(test_runtime_args1());

	// dynamic extents
	constexpr bool test_template_args2()
	{
		auto arr2 = std::array{1, 3, 5, 7, 9};
		auto span2 = Span<const int, dynamic_extent>{arr2};

		auto first1 = span2.first<3>();
		assert(first1.size() == 3);
		assert(first1[0] == 1); assert(first1[1] == 3); assert(first1[2] == 5);
		(void)first1;

		auto last1 = span2.last<3>();
		assert(last1.size() == 3);
		assert(last1[0] == 5); assert(last1[1] == 7); assert(last1[2] == 9);
		(void)last1;

		auto sub1 = span2.subspan<1, 3>();
		assert(sub1.size() == 3);
		assert(sub1[0] == 3); assert(sub1[1] == 5); assert(sub1[2] == 7);
		(void)sub1;

		auto sub2 = span2.subspan<1>();
		assert(sub2.size() == 4);
		assert(sub2[0] == 3); assert(sub2[1] == 5);
		assert(sub2[2] == 7); assert(sub2[3] == 9);
		(void)sub2;

		return true;
	}
	static_assert(test_template_args2());

	constexpr bool test_runtime_args2()
	{
		auto arr2 = std::array{1, 3, 5, 7, 9};
		auto span2 = Span<const int, dynamic_extent>{arr2};

		auto first1 = span2.first(3);
		assert(first1.size() == 3);
		assert(first1[0] == 1); assert(first1[1] == 3); assert(first1[2] == 5);
		(void)first1;

		auto last1 = span2.last(3);
		assert(last1.size() == 3);
		assert(last1[0] == 5); assert(last1[1] == 7); assert(last1[2] == 9);
		(void)last1;

		auto sub1 = span2.subspan(1, 3);
		assert(sub1.size() == 3);
		assert(sub1[0] == 3); assert(sub1[1] == 5); assert(sub1[2] == 7);
		(void)sub1;

		auto sub2 = span2.subspan<1>();
		assert(sub2.size() == 4);
		assert(sub2[0] == 3); assert(sub2[1] == 5);
		assert(sub2[2] == 7); assert(sub2[3] == 9);
		(void)sub2;

		return true;
	}
	static_assert(test_runtime_args2());
} // namespace subview_tests


namespace access_and_mutation_tests
{
	constexpr int test1()
	{
		int arr[] = {1, 2, 3};
		auto span1 = Span{arr};

		static_assert(std::is_same_v<decltype(span1), Span<int, 3>>);
		static_assert(std::is_same_v<decltype(span1[0]), int&>);

		span1.front() += 3;
		span1[2] += 2;
		++span1.back();
		return arr[0] + arr[1] + arr[2];
	}

	static_assert(test1() == 12);
} // namespace access_and_mutation_tests


namespace iteration_tests
{
	constexpr int arr[] = {1, 2, 3};

	// for
	constexpr int regular_for()
	{
		auto span1 = Span{arr};

		int acc = 0;
		for (std::size_t idx = 0; idx < span1.size(); ++idx)
		{
			acc += span1[idx];
		}

		return acc;
	}
	static_assert(regular_for() == 6);

	// range for
	constexpr int range_for()
	{
		auto span1 = Span{arr};

		int acc = 0;
		for (auto&& val : span1)
		{
			static_assert(std::is_same_v<decltype(val), const int&>);
			acc += val;
		}

		return acc;
	}
	static_assert(range_for() == 6);

	// reverse iteration
	constexpr bool reverse_iteration()
	{
		auto span1 = Span{arr};

		auto it = span1.rbegin();
		assert(*it == 3);

		++it;
		assert(*it == 2);

		++it;
		assert(*it == 1);

		return true;
	}
	static_assert(reverse_iteration());
} // namespace iteration_tests


namespace nonmember_function_tests
{
	// const Span
	constexpr int arr1[] = {1, 2, 3};
	constexpr auto const_span_static = Span{arr1};
	constexpr auto const_span_dynamic = Span<const int, dynamic_extent>{arr1};

	auto bytes1 = as_bytes(const_span_static);
	static_assert(std::is_same_v<decltype(bytes1), Span<const std::byte, 12>>);

	auto bytes2 = as_bytes(const_span_dynamic);
	static_assert(std::is_same_v<decltype(bytes2), Span<const std::byte, dynamic_extent>>);

	// non-const Span
	int arr2[] = {1, 2, 3};
	auto nonconst_span_static = Span{arr2};
	auto nonconst_span_dynamic = Span<int, dynamic_extent>{arr2};

	auto bytes3 = as_bytes(nonconst_span_static);
	static_assert(std::is_same_v<decltype(bytes3), Span<const std::byte, 12>>);

	auto bytes4 = as_bytes(nonconst_span_dynamic);
	static_assert(std::is_same_v<decltype(bytes4), Span<const std::byte, dynamic_extent>>);

	auto bytes5 = as_writable_bytes(nonconst_span_static);
	static_assert(std::is_same_v<decltype(bytes5), Span<std::byte, 12>>);

	auto bytes6 = as_writable_bytes(nonconst_span_dynamic);
	static_assert(std::is_same_v<decltype(bytes6), Span<std::byte, dynamic_extent>>);

} // namespace nonmember_function_tests


} // namespace lmms
