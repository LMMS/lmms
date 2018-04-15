/*
 * NiftyCounter.h
 *
 * Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This file is licensed under the MIT license. See LICENSE.MIT.txt file in the
 * project root for details.
 *
 */

#pragma once

/// Nifty counter, also known as "Schwarz Counter". Used for ensuring global
/// static object initialization and destruction order.

template <typename T> class _NiftyCounter_Base
{
public:
	_NiftyCounter_Base()
	{
		if (! T::inc()) T::init();
	}
	_NiftyCounter_Base(const _NiftyCounter_Base& other) : _NiftyCounter_Base() {}

	~_NiftyCounter_Base()
	{
		if (! T::dec()) T::deinit();
	}
};

template <typename T, void C(), void D()> class _NiftyCounterCD_Base : public _NiftyCounter_Base<T>
{
	friend class _NiftyCounter_Base<T>;
private:
	static void init() { C(); }
	static void deinit() { D(); }
	static int inc() { return T::s_count++; }
	static int dec() { return T::s_count--; }
};


/// Pass construction and destruction functions as template arguments C and D.
template <void C(), void D()> class NiftyCounter : public _NiftyCounterCD_Base<NiftyCounter<C,D>, C,D>
{
	friend class _NiftyCounterCD_Base<NiftyCounter<C,D>, C,D>;
private:
	static int s_count;
};
template <void C(), void D()> int NiftyCounter<C, D>::s_count = 0;

/// Thread-local version of NiftyCounter
template <void C(), void D()> class NiftyCounterTL : public _NiftyCounterCD_Base<NiftyCounterTL<C,D>, C,D>
{
	friend class _NiftyCounterCD_Base<NiftyCounterTL<C,D>, C,D>;
private:
	thread_local static int s_count;
};
template <void C(), void D()> thread_local int NiftyCounterTL<C, D>::s_count = 0;
